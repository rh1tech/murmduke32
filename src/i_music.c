/*
 * Duke3D OPL Music System for RP2350
 * Uses emu8950 OPL emulator for FM synthesis
 * Parses standard MIDI files from SD card
 * Uses Duke3D native timbre bank format
 *
 * Based on murmdoom OPL music implementation by Graham Sanderson
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "pico.h"
#include "pico/audio.h"  // For audio_buffer_t definition
#include "i_music.h"
#include "i_picosound.h"
#include "opl/emu8950.h"
#include "opl/midifile.h"
#include "../drivers/psram_allocator.h"
#include "../components/Engine/filesystem.h"  // For kopen4load, kread, etc.

// OPL configuration
#define OPL_SAMPLE_RATE 22050
#define OPL_CLOCK       3579545     // OPL2 clock frequency
#define OPL_NUM_VOICES  9
#define OPL_SECOND      1000000ULL  // Microseconds per second

// Duke3D timbre format (13 bytes per instrument)
typedef struct {
    uint8_t SAVEK[2];    // Modulator/Carrier characteristics (reg 0x20)
    uint8_t Level[2];    // Modulator/Carrier output levels (reg 0x40)
    uint8_t Env1[2];     // Modulator/Carrier attack/decay (reg 0x60)
    uint8_t Env2[2];     // Modulator/Carrier sustain/release (reg 0x80)
    uint8_t Wave[2];     // Modulator/Carrier waveforms (reg 0xE0)
    uint8_t Feedback;    // Feedback/connection (reg 0xC0)
    int8_t Transpose;    // Note transpose offset
    int8_t Velocity;     // Velocity sensitivity
} timbre_t;

// OPL voice state
typedef struct {
    bool active;
    uint8_t channel;     // MIDI channel
    uint8_t note;        // MIDI note number
    uint8_t velocity;    // Note velocity
    uint8_t instrument;  // Instrument number
} opl_voice_t;

// MIDI channel state
typedef struct {
    uint8_t instrument;  // Current program
    uint8_t volume;      // Channel volume (0-127)
    int16_t pitchbend;   // Pitch bend (-8192 to +8192)
    uint8_t pan;         // Pan position (0-127)
} midi_channel_t;

// Module state
static OPL *opl_emu = NULL;
static midi_file_t *current_midi = NULL;
static midi_track_iter_t **track_iters = NULL;
static uint64_t *track_next_event_us = NULL;  // Next event time for each track
static unsigned int num_tracks = 0;
static unsigned int running_tracks = 0;
static bool music_initialized = false;
static bool music_playing = false;
static bool music_paused = false;
static bool music_looping = false;
static int music_volume = 127;

// Timbre bank
static timbre_t timbre_bank[256];
static bool timbre_loaded = false;

// Voice allocation
static opl_voice_t voices[OPL_NUM_VOICES];
static midi_channel_t channels[16];

// Timing
static uint64_t current_time_us = 0;
static unsigned int us_per_beat = 500000;   // Default 120 BPM
static unsigned int ticks_per_beat = 480;

// Temp buffer for OPL output
static int32_t opl_temp_buffer[1024];

//=============================================================================
// OPL Register Helpers
//=============================================================================

// Operator offsets for each voice
static const uint8_t op_offsets[OPL_NUM_VOICES][2] = {
    {0x00, 0x03}, {0x01, 0x04}, {0x02, 0x05},
    {0x08, 0x0B}, {0x09, 0x0C}, {0x0A, 0x0D},
    {0x10, 0x13}, {0x11, 0x14}, {0x12, 0x15}
};

static void OPL_Write(uint8_t reg, uint8_t value) {
    if (opl_emu) {
        OPL_writeReg(opl_emu, reg, value);
    }
}

static void OPL_SetVoiceInstrument(int voice, int instrument) {
    if (voice < 0 || voice >= OPL_NUM_VOICES) return;
    if (instrument < 0 || instrument >= 256) return;
    if (!timbre_loaded) {
        static bool warned = false;
        if (!warned) {
            printf("OPL: Timbre not loaded!\n");
            warned = true;
        }
        return;
    }

    const timbre_t *t = &timbre_bank[instrument];
    uint8_t mod_off = op_offsets[voice][0];
    uint8_t car_off = op_offsets[voice][1];

    // Modulator registers
    OPL_Write(0x20 + mod_off, t->SAVEK[0]);    // Characteristics
    OPL_Write(0x40 + mod_off, t->Level[0]);    // Level
    OPL_Write(0x60 + mod_off, t->Env1[0]);     // Attack/Decay
    OPL_Write(0x80 + mod_off, t->Env2[0]);     // Sustain/Release
    OPL_Write(0xE0 + mod_off, t->Wave[0]);     // Waveform

    // Carrier registers
    OPL_Write(0x20 + car_off, t->SAVEK[1]);    // Characteristics
    OPL_Write(0x40 + car_off, t->Level[1]);    // Level (will be overridden by volume)
    OPL_Write(0x60 + car_off, t->Env1[1]);     // Attack/Decay
    OPL_Write(0x80 + car_off, t->Env2[1]);     // Sustain/Release
    OPL_Write(0xE0 + car_off, t->Wave[1]);     // Waveform

    // Feedback/connection
    OPL_Write(0xC0 + voice, t->Feedback);

    voices[voice].instrument = instrument;
}

static void OPL_SetVoiceVolume(int voice, int velocity, int channel_volume) {
    if (voice < 0 || voice >= OPL_NUM_VOICES) return;
    
    int inst = voices[voice].instrument;
    const timbre_t *t = &timbre_bank[inst];
    
    uint8_t mod_off = op_offsets[voice][0];
    uint8_t car_off = op_offsets[voice][1];
    
    // Calculate volume: scale velocity and channel volume to 0-63 attenuation
    // Lower attenuation = louder
    int vol = (velocity * channel_volume * music_volume) / (127 * 127);
    if (vol > 127) vol = 127;
    
    // Convert to attenuation (0 = no attenuation/loud, 63 = full attenuation/quiet)
    int atten = 63 - ((vol * 63) / 127);
    
    // Use timbre's base level, modified by calculated attenuation
    int carrier_base = t->Level[1] & 0x3F;
    int carrier_atten = carrier_base + (atten / 2);  // Only add half the attenuation
    if (carrier_atten > 63) carrier_atten = 63;
    
    OPL_Write(0x40 + car_off, (t->Level[1] & 0xC0) | carrier_atten);
    
    // For additive synthesis (FM connection bit = 1), also set modulator volume
    if (t->Feedback & 0x01) {
        int mod_base = t->Level[0] & 0x3F;
        int mod_atten = mod_base + (atten / 2);
        if (mod_atten > 63) mod_atten = 63;
        OPL_Write(0x40 + mod_off, (t->Level[0] & 0xC0) | mod_atten);
    }
}

// Frequency lookup table (F-number for each note in octave 0-4)
static const uint16_t note_fnum[12] = {
    0x157, 0x16B, 0x181, 0x198, 0x1B0, 0x1CA,
    0x1E5, 0x202, 0x220, 0x241, 0x263, 0x287
};

static void OPL_NoteOn(int voice, int note, int velocity, int channel) {
    if (voice < 0 || voice >= OPL_NUM_VOICES) return;
    
    int original_note = note;  // Save for voice tracking
    
    // Determine instrument and calculate actual note
    int inst = channels[channel].instrument;
    if (channel == 9) {
        // Percussion channel - MIDI note selects the drum instrument
        // The actual pitch comes from the timbre's Transpose field
        inst = 128 + note - 35;
        if (inst < 128) inst = 128;
        if (inst >= 256) inst = 255;
        // For drums, the transpose IS the note (not an offset)
        note = timbre_bank[inst].Transpose;
    } else {
        // Melodic channel - apply transpose as offset
        note += timbre_bank[inst].Transpose;
    }
    
    // Apply key offset (standard MIDI middle C adjustment)
    note -= 12;
    
    // Clamp note
    if (note < 0) note = 0;
    if (note > 127) note = 127;

    // Calculate octave and note within octave
    int octave = note / 12;
    int note_idx = note % 12;
    
    if (octave > 7) octave = 7;

    // Get F-number
    uint16_t fnum = note_fnum[note_idx];

    // Set instrument
    OPL_SetVoiceInstrument(voice, inst);
    
    // Set volume
    OPL_SetVoiceVolume(voice, velocity, channels[channel].volume);

    // Set frequency (low byte)
    OPL_Write(0xA0 + voice, fnum & 0xFF);
    
    // Set frequency (high byte) with key-on and octave
    OPL_Write(0xB0 + voice, 0x20 | ((octave & 7) << 2) | ((fnum >> 8) & 3));

    // Track voice state (use original MIDI note for matching note-off)
    voices[voice].active = true;
    voices[voice].channel = channel;
    voices[voice].note = original_note;
    voices[voice].velocity = velocity;
    voices[voice].instrument = inst;
}

static void OPL_NoteOff(int voice) {
    if (voice < 0 || voice >= OPL_NUM_VOICES) return;
    if (!voices[voice].active) return;

    // Simply clear the key-on bit by writing 0 to register B0+voice
    // The OPL will use the envelope release phase
    OPL_Write(0xB0 + voice, 0x00);

    voices[voice].active = false;
}

//=============================================================================
// Voice Allocation
//=============================================================================

static int AllocateVoice(int channel, int note) {
    // First, look for an inactive voice
    for (int i = 0; i < OPL_NUM_VOICES; i++) {
        if (!voices[i].active) {
            return i;
        }
    }

    // All voices busy - need to steal one
    // Priority: steal percussion before melodic, older notes before newer
    int steal_voice = -1;
    
    // First try to steal a percussion voice (channel 9)
    for (int i = 0; i < OPL_NUM_VOICES; i++) {
        if (voices[i].channel == 9) {
            steal_voice = i;
            break;
        }
    }
    
    // If no percussion, steal voice 0
    if (steal_voice < 0) {
        steal_voice = 0;
    }
    
    OPL_NoteOff(steal_voice);
    return steal_voice;
}

static int FindVoice(int channel, int note) {
    for (int i = 0; i < OPL_NUM_VOICES; i++) {
        if (voices[i].active && voices[i].channel == channel && voices[i].note == note) {
            return i;
        }
    }
    return -1;
}

static void AllNotesOff(int channel) {
    for (int i = 0; i < OPL_NUM_VOICES; i++) {
        if (voices[i].active && voices[i].channel == channel) {
            OPL_NoteOff(i);
        }
    }
}

//=============================================================================
// MIDI Event Processing
//=============================================================================

static void ProcessMIDIEvent(midi_event_t *event) {
    if (!event) return;

    switch (event->event_type) {
        case MIDI_EVENT_NOTE_OFF: {
            int ch = event->data.channel.channel;
            int note = event->data.channel.param1;
            int voice = FindVoice(ch, note);
            if (voice >= 0) {
                OPL_NoteOff(voice);
            }
            break;
        }

        case MIDI_EVENT_NOTE_ON: {
            int ch = event->data.channel.channel;
            int note = event->data.channel.param1;
            int vel = event->data.channel.param2;
            
            if (vel == 0) {
                // Note on with velocity 0 = note off
                int voice = FindVoice(ch, note);
                if (voice >= 0) {
                    OPL_NoteOff(voice);
                }
            } else {
                int voice = AllocateVoice(ch, note);
                OPL_NoteOn(voice, note, vel, ch);
            }
            break;
        }

        case MIDI_EVENT_CONTROLLER: {
            int ch = event->data.channel.channel;
            int ctrl = event->data.channel.param1;
            int val = event->data.channel.param2;

            switch (ctrl) {
                case 7:  // Main volume
                    channels[ch].volume = val;
                    // Update all active voices on this channel
                    for (int i = 0; i < OPL_NUM_VOICES; i++) {
                        if (voices[i].active && voices[i].channel == ch) {
                            OPL_SetVoiceVolume(i, voices[i].velocity, val);
                        }
                    }
                    break;
                case 10: // Pan
                    channels[ch].pan = val;
                    break;
                case 123: // All notes off
                    AllNotesOff(ch);
                    break;
            }
            break;
        }

        case MIDI_EVENT_PROGRAM_CHANGE: {
            int ch = event->data.channel.channel;
            int prog = event->data.channel.param1;
            channels[ch].instrument = prog;
            break;
        }

        case MIDI_EVENT_PITCH_BEND: {
            int ch = event->data.channel.channel;
            int bend = (event->data.channel.param2 << 7) | event->data.channel.param1;
            channels[ch].pitchbend = bend - 8192;
            // TODO: Apply pitch bend to active notes
            break;
        }

        case MIDI_EVENT_META: {
            if (event->data.meta.type == 0x51 && event->data.meta.length == 3) {
                // Set tempo
                uint8_t *data = event->data.meta.data;
                us_per_beat = (data[0] << 16) | (data[1] << 8) | data[2];
            }
            break;
        }

        default:
            break;
    }
}

//=============================================================================
// Music Generator Callback (called from audio mixer)
//=============================================================================

// Schedule next event for a track (calculates absolute time in microseconds)
static void ScheduleNextEvent(unsigned int track_num) {
    if (!track_iters || !track_iters[track_num]) return;
    
    unsigned int delta = MIDI_GetDeltaTime(track_iters[track_num]);
    uint64_t delta_us = ((uint64_t)delta * us_per_beat) / ticks_per_beat;
    track_next_event_us[track_num] = current_time_us + delta_us;
}

static void MusicGenerator(audio_buffer_t *buffer) {
    static uint32_t call_count = 0;
    call_count++;
    
    if (!buffer) return;
    
    unsigned int samples_to_fill = buffer->max_sample_count;
    int16_t *out = (int16_t *)buffer->buffer->bytes;
    
    // If music not playing, just clear the buffer and return
    if (!music_playing || music_paused || !opl_emu || !track_iters || !track_next_event_us) {
        memset(out, 0, samples_to_fill * 4);
        buffer->sample_count = samples_to_fill;
        return;
    }

    unsigned int filled = 0;
    int total_events_processed = 0;
    const int MAX_EVENTS_PER_BUFFER = 200;

    while (filled < samples_to_fill) {
        // Find earliest next event
        uint64_t next_event_time = UINT64_MAX;
        for (unsigned int t = 0; t < num_tracks; t++) {
            if (track_iters[t] && track_next_event_us[t] < next_event_time) {
                next_event_time = track_next_event_us[t];
            }
        }

        // Calculate samples until next event
        unsigned int samples_until_event;
        if (next_event_time == UINT64_MAX || next_event_time > current_time_us + 1000000) {
            samples_until_event = samples_to_fill - filled;
        } else if (next_event_time <= current_time_us) {
            samples_until_event = 0;
        } else {
            uint64_t us_until = next_event_time - current_time_us;
            samples_until_event = (us_until * OPL_SAMPLE_RATE) / OPL_SECOND;
            if (samples_until_event > samples_to_fill - filled) {
                samples_until_event = samples_to_fill - filled;
            }
        }

        // Generate OPL samples
        if (samples_until_event > 0) {
            unsigned int chunk = samples_until_event;
            if (chunk > 512) chunk = 512;

            OPL_calc_buffer_stereo(opl_emu, opl_temp_buffer, chunk);

            for (unsigned int i = 0; i < chunk; i++) {
                int32_t sample = opl_temp_buffer[i];
                int16_t left = (int16_t)(sample >> 16);
                int16_t right = (int16_t)(sample & 0xFFFF);
                // Amplify by 10x
                int32_t amp_left = (int32_t)left * 10;
                int32_t amp_right = (int32_t)right * 10;
                if (amp_left > 32767) amp_left = 32767;
                if (amp_left < -32768) amp_left = -32768;
                if (amp_right > 32767) amp_right = 32767;
                if (amp_right < -32768) amp_right = -32768;
                out[(filled + i) * 2 + 0] = (int16_t)amp_left;
                out[(filled + i) * 2 + 1] = (int16_t)amp_right;
            }

            filled += chunk;
            current_time_us += (chunk * OPL_SECOND) / OPL_SAMPLE_RATE;
        } else if (total_events_processed < MAX_EVENTS_PER_BUFFER) {
            // Process one event from earliest track
            bool processed_any = false;
            for (unsigned int t = 0; t < num_tracks && !processed_any; t++) {
                if (!track_iters[t]) continue;
                if (track_next_event_us[t] > current_time_us) continue;

                midi_event_t *event;
                if (!MIDI_GetNextEvent(track_iters[t], &event)) {
                    running_tracks--;
                    track_iters[t] = NULL;
                    track_next_event_us[t] = UINT64_MAX;
                    processed_any = true;
                    continue;
                }

                ProcessMIDIEvent(event);
                total_events_processed++;
                processed_any = true;

                if (event->event_type == MIDI_EVENT_META && 
                    event->data.meta.type == 0x2F) {
                    running_tracks--;
                    track_iters[t] = NULL;
                    track_next_event_us[t] = UINT64_MAX;
                } else {
                    ScheduleNextEvent(t);
                }
            }
            
            if (!processed_any) {
                current_time_us += 1000;
            }
        } else {
            break;
        }

        if (running_tracks == 0) {
            if (music_looping && current_midi) {
                for (unsigned int t = 0; t < num_tracks; t++) {
                    if (track_iters[t]) {
                        MIDI_RestartIterator(track_iters[t]);
                    }
                }
                running_tracks = num_tracks;
                current_time_us = 0;
                for (unsigned int t = 0; t < num_tracks; t++) {
                    if (track_iters[t]) {
                        ScheduleNextEvent(t);
                    }
                }
            } else {
                music_playing = false;
                break;
            }
        }
    }

    // Fill remaining samples
    while (filled < samples_to_fill) {
        unsigned int chunk = samples_to_fill - filled;
        if (chunk > 512) chunk = 512;
        
        OPL_calc_buffer_stereo(opl_emu, opl_temp_buffer, chunk);
        
        for (unsigned int i = 0; i < chunk; i++) {
            int32_t sample = opl_temp_buffer[i];
            int16_t left = (int16_t)(sample >> 16);
            int16_t right = (int16_t)(sample & 0xFFFF);
            // Amplify by 10x
            int32_t amp_left = (int32_t)left * 10;
            int32_t amp_right = (int32_t)right * 10;
            if (amp_left > 32767) amp_left = 32767;
            if (amp_left < -32768) amp_left = -32768;
            if (amp_right > 32767) amp_right = 32767;
            if (amp_right < -32768) amp_right = -32768;
            out[(filled + i) * 2 + 0] = (int16_t)amp_left;
            out[(filled + i) * 2 + 1] = (int16_t)amp_right;
        }
        filled += chunk;
        current_time_us += (chunk * OPL_SECOND) / OPL_SAMPLE_RATE;
    }

    buffer->sample_count = samples_to_fill;
}

//=============================================================================
// Public API
//=============================================================================

bool I_Music_Init(void) {
    if (music_initialized) {
        return true;
    }

    // Initialize OPL emulator
    opl_emu = OPL_new(OPL_CLOCK, OPL_SAMPLE_RATE);
    if (!opl_emu) {
        printf("I_Music_Init: Failed to create OPL emulator\n");
        return false;
    }

    // Initialize OPL registers
    OPL_reset(opl_emu);
    
    // Enable waveform select
    OPL_Write(0x01, 0x20);

    // Clear all voices
    for (int i = 0; i < OPL_NUM_VOICES; i++) {
        voices[i].active = false;
        OPL_Write(0xB0 + i, 0);  // Key off all voices
    }

    // Initialize channels
    for (int i = 0; i < 16; i++) {
        channels[i].instrument = 0;
        channels[i].volume = 127;
        channels[i].pitchbend = 0;
        channels[i].pan = 64;
    }

    // NOTE: Don't register music generator here - do it when music starts
    // This avoids issues with the callback being called before music is loaded

    music_initialized = true;
    printf("I_Music_Init: OPL music initialized\n");
    return true;
}

void I_Music_Shutdown(void) {
    if (!music_initialized) return;

    I_Music_Stop();

    if (opl_emu) {
        OPL_delete(opl_emu);
        opl_emu = NULL;
    }

    music_initialized = false;
}

bool I_Music_PlayMIDI(const char *filename, bool loop) {
    if (!music_initialized) {
        if (!I_Music_Init()) {
            return false;
        }
    }

    // Stop any current playback
    I_Music_Stop();

    // Reset temp PSRAM allocation
    psram_reset_temp();
    psram_set_temp_mode(1);

    // Load MIDI file from GRP archive using Duke3D's file functions
    int32_t fd = kopen4load(filename, 0);  // 0 = try filesystem first, then GRP
    if (fd < 0) {
        printf("I_Music_PlayMIDI: Failed to open %s from GRP\n", filename);
        psram_set_temp_mode(0);
        return false;
    }

    int32_t fileSize = kfilelength(fd);
    if (fileSize <= 0) {
        printf("I_Music_PlayMIDI: Invalid file size for %s\n", filename);
        kclose(fd);
        psram_set_temp_mode(0);
        return false;
    }

    printf("I_Music_PlayMIDI: Loading %s (%d bytes)\n", filename, fileSize);

    // Read MIDI data into buffer
    uint8_t *midiBuffer = psram_malloc(fileSize);
    if (!midiBuffer) {
        printf("I_Music_PlayMIDI: Failed to allocate buffer for %s\n", filename);
        kclose(fd);
        psram_set_temp_mode(0);
        return false;
    }

    int32_t bytesRead = kread(fd, midiBuffer, fileSize);
    kclose(fd);

    if (bytesRead != fileSize) {
        printf("I_Music_PlayMIDI: Read error for %s (%d/%d)\n", filename, bytesRead, fileSize);
        psram_free(midiBuffer);
        psram_set_temp_mode(0);
        return false;
    }

    // Debug: Print first 8 bytes of MIDI buffer (header)
    printf("I_Music_PlayMIDI: MIDI header bytes: %02X %02X %02X %02X %02X %02X %02X %02X\n",
           midiBuffer[0], midiBuffer[1], midiBuffer[2], midiBuffer[3],
           midiBuffer[4], midiBuffer[5], midiBuffer[6], midiBuffer[7]);
    // Debug: Print bytes 8-15 (rest of header + start of first track)
    printf("I_Music_PlayMIDI: Bytes 8-15: %02X %02X %02X %02X %02X %02X %02X %02X\n",
           midiBuffer[8], midiBuffer[9], midiBuffer[10], midiBuffer[11],
           midiBuffer[12], midiBuffer[13], midiBuffer[14], midiBuffer[15]);
    // Debug: Print bytes 16-23 (should include MTrk)
    printf("I_Music_PlayMIDI: Bytes 16-23: %02X %02X %02X %02X %02X %02X %02X %02X\n",
           midiBuffer[16], midiBuffer[17], midiBuffer[18], midiBuffer[19],
           midiBuffer[20], midiBuffer[21], midiBuffer[22], midiBuffer[23]);

    // Write to temp file on SD card so MIDI loader can read it
    const char *tempPath = "/duke3d/temp.mid";
    
    // Remove old temp file first to ensure clean write
    remove(tempPath);
    
    FILE *tempFile = fopen(tempPath, "wb");
    if (!tempFile) {
        printf("I_Music_PlayMIDI: Failed to create temp file\n");
        psram_free(midiBuffer);
        psram_set_temp_mode(0);
        return false;
    }

    size_t written = fwrite(midiBuffer, 1, fileSize, tempFile);
    fflush(tempFile);
    fclose(tempFile);
    
    // Verify the temp file was written correctly
    FILE *verifyFile = fopen(tempPath, "rb");
    if (verifyFile) {
        uint8_t verifyBuf[24];
        size_t verifyRead = fread(verifyBuf, 1, 24, verifyFile);
        fclose(verifyFile);
        printf("I_Music_PlayMIDI: Temp file verify (%zu bytes): %02X %02X %02X %02X %02X %02X %02X %02X\n",
               verifyRead, verifyBuf[0], verifyBuf[1], verifyBuf[2], verifyBuf[3],
               verifyBuf[4], verifyBuf[5], verifyBuf[6], verifyBuf[7]);
        printf("I_Music_PlayMIDI: Temp bytes 14-21: %02X %02X %02X %02X %02X %02X %02X %02X\n",
               verifyBuf[14], verifyBuf[15], verifyBuf[16], verifyBuf[17],
               verifyBuf[18], verifyBuf[19], verifyBuf[20], verifyBuf[21]);
    } else {
        printf("I_Music_PlayMIDI: Failed to verify temp file!\n");
    }
    
    psram_free(midiBuffer);

    if (written != (size_t)fileSize) {
        printf("I_Music_PlayMIDI: Failed to write temp file (%zu/%d)\n", written, fileSize);
        psram_set_temp_mode(0);
        return false;
    }

    // Now load from the temp file
    current_midi = MIDI_LoadFile((char *)tempPath);
    
    psram_set_temp_mode(0);

    if (!current_midi) {
        printf("I_Music_PlayMIDI: Failed to load %s\n", filename);
        return false;
    }

    // Get MIDI info
    num_tracks = MIDI_NumTracks(current_midi);
    ticks_per_beat = MIDI_GetFileTimeDivision(current_midi);
    us_per_beat = 500000;  // Default 120 BPM

    // Allocate track iterators
    track_iters = psram_malloc(num_tracks * sizeof(midi_track_iter_t *));
    if (!track_iters) {
        printf("I_Music_PlayMIDI: Failed to allocate track iterators\n");
        MIDI_FreeFile(current_midi);
        current_midi = NULL;
        return false;
    }

    // Allocate track timing array
    track_next_event_us = psram_malloc(num_tracks * sizeof(uint64_t));
    if (!track_next_event_us) {
        printf("I_Music_PlayMIDI: Failed to allocate track timing\n");
        psram_free(track_iters);
        track_iters = NULL;
        MIDI_FreeFile(current_midi);
        current_midi = NULL;
        return false;
    }

    // Initialize track iterators and schedule first events
    for (unsigned int i = 0; i < num_tracks; i++) {
        track_iters[i] = MIDI_IterateTrack(current_midi, i);
        track_next_event_us[i] = 0;  // First event at time 0
    }
    
    // Schedule first events for each track
    current_time_us = 0;
    for (unsigned int i = 0; i < num_tracks; i++) {
        ScheduleNextEvent(i);
    }
    
    running_tracks = num_tracks;

    // Reset channels to defaults
    for (int i = 0; i < 16; i++) {
        channels[i].instrument = 0;
        channels[i].volume = 127;
        channels[i].pitchbend = 0;
        channels[i].pan = 64;
    }

    // Reset OPL chip and voices for clean start
    OPL_reset(opl_emu);
    OPL_Write(0x01, 0x20);  // Enable waveform select
    for (int i = 0; i < OPL_NUM_VOICES; i++) {
        voices[i].active = false;
        voices[i].channel = 0;
        voices[i].note = 0;
        voices[i].velocity = 0;
        voices[i].instrument = 0;
        OPL_Write(0xB0 + i, 0);  // Key off
    }

    // Start playback
    music_looping = loop;
    music_paused = false;
    music_playing = true;

    // Register music generator callback now that music is ready
    if (I_PicoSound_IsInitialized()) {
        I_PicoSound_SetMusicGenerator(MusicGenerator);
    }

    printf("I_Music_PlayMIDI: Playing %s (%u tracks)\n", filename, num_tracks);
    return true;
}

void I_Music_Stop(void) {
    music_playing = false;
    music_paused = false;

    // Unregister music generator callback
    if (I_PicoSound_IsInitialized()) {
        I_PicoSound_SetMusicGenerator(NULL);
    }

    // Stop all notes
    for (int i = 0; i < OPL_NUM_VOICES; i++) {
        if (voices[i].active) {
            OPL_NoteOff(i);
        }
    }

    // Free track iterators
    if (track_iters) {
        for (unsigned int i = 0; i < num_tracks; i++) {
            if (track_iters[i]) {
                MIDI_FreeIterator(track_iters[i]);
            }
        }
        psram_free(track_iters);
        track_iters = NULL;
    }

    // Free track timing array
    if (track_next_event_us) {
        psram_free(track_next_event_us);
        track_next_event_us = NULL;
    }

    // Free MIDI file
    if (current_midi) {
        MIDI_FreeFile(current_midi);
        current_midi = NULL;
    }

    num_tracks = 0;
    running_tracks = 0;
    
    // Reset temp PSRAM
    psram_reset_temp();
}

void I_Music_Pause(void) {
    if (!music_playing) return;
    music_paused = true;

    // Stop all active notes
    for (int i = 0; i < OPL_NUM_VOICES; i++) {
        if (voices[i].active) {
            // Just key off without clearing state
            OPL_Write(0xB0 + i, 0);
        }
    }
}

void I_Music_Resume(void) {
    music_paused = false;
}

bool I_Music_IsPlaying(void) {
    return music_playing && !music_paused;
}

void I_Music_SetVolume(int volume) {
    if (volume < 0) volume = 0;
    if (volume > 255) volume = 255;
    music_volume = volume / 2;  // Convert to 0-127 range
}

int I_Music_GetVolume(void) {
    return music_volume * 2;
}

void I_Music_RegisterTimbreBank(const uint8_t *timbres) {
    if (!timbres) return;

    // Copy timbre data (256 instruments × 13 bytes)
    for (int i = 0; i < 256; i++) {
        timbre_bank[i].SAVEK[0] = *timbres++;
        timbre_bank[i].SAVEK[1] = *timbres++;
        timbre_bank[i].Level[0] = *timbres++;
        timbre_bank[i].Level[1] = *timbres++;
        timbre_bank[i].Env1[0] = *timbres++;
        timbre_bank[i].Env1[1] = *timbres++;
        timbre_bank[i].Env2[0] = *timbres++;
        timbre_bank[i].Env2[1] = *timbres++;
        timbre_bank[i].Wave[0] = *timbres++;
        timbre_bank[i].Wave[1] = *timbres++;
        timbre_bank[i].Feedback = *timbres++;
        timbre_bank[i].Transpose = (int8_t)*timbres++;
        timbre_bank[i].Velocity = (int8_t)*timbres++;
    }

    timbre_loaded = true;
    printf("I_Music_RegisterTimbreBank: Loaded 256 instruments\n");
}
