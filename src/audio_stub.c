/*
 * Duke3D Audiolib Stub for RP2350
 * Disables all audio for initial port
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "fx_man.h"
#include "music.h"

// ============= FX_MAN Stub =============

int FX_SoundDevice = -1;
int FX_ErrorCode = FX_Ok;
int FX_Installed = 0;

char *FX_ErrorString(int ErrorNumber) {
    switch (ErrorNumber) {
        case FX_Ok: return "FX ok";
        case FX_Warning: return "FX warning";
        case FX_Error: return "FX error";
        default: return "Unknown FX error";
    }
}

int FX_SetupCard(int SoundCard, fx_device *device) {
    if (device) {
        device->MaxVoices = 0;
        device->MaxSampleBits = 0;
        device->MaxChannels = 0;
    }
    return FX_Ok;
}

int FX_GetBlasterSettings(fx_blaster_config *blaster) {
    if (blaster) memset(blaster, 0, sizeof(*blaster));
    return FX_Ok;
}

int FX_SetupSoundBlaster(fx_blaster_config blaster, int *MaxVoices, int *MaxSampleBits, int *MaxChannels) {
    if (MaxVoices) *MaxVoices = 0;
    if (MaxSampleBits) *MaxSampleBits = 0;
    if (MaxChannels) *MaxChannels = 0;
    return FX_Ok;
}

int FX_Init(int SoundCard, int numvoices, int numchannels, int samplebits, unsigned mixrate) {
    printf("FX_Init: Audio disabled for RP2350 port\n");
    return FX_Ok;
}

int FX_Shutdown(void) {
    return FX_Ok;
}

int FX_SetCallBack(void (*function)(int32_t)) {
    return FX_Ok;
}

void FX_SetVolume(int volume) {}
int FX_GetVolume(void) { return 0; }
void FX_SetReverseStereo(int setting) {}
int FX_GetReverseStereo(void) { return 0; }
void FX_SetReverb(int reverb) {}
void FX_SetFastReverb(int reverb) {}
int FX_GetMaxReverbDelay(void) { return 0; }
int FX_GetReverbDelay(void) { return 0; }
void FX_SetReverbDelay(int delay) {}

int FX_VoiceAvailable(int priority) { return 0; }
int FX_EndLooping(int handle) { return FX_Ok; }
int FX_SetPan(int handle, int vol, int left, int right) { return FX_Ok; }
int FX_SetPitch(int handle, int pitchoffset) { return FX_Ok; }
int FX_SetFrequency(int handle, int frequency) { return FX_Ok; }

int FX_PlayVOC(uint8_t *ptr, int pitchoffset, int vol, int left, int right,
               int priority, uint32_t callbackval) { return 0; }
int FX_PlayLoopedVOC(uint8_t *ptr, int32_t loopstart, int32_t loopend,
                     int32_t pitchoffset, int32_t vol, int32_t left, int32_t right, 
                     int32_t priority, uint32_t callbackval) { return 0; }
int FX_PlayWAV(uint8_t *ptr, int pitchoffset, int vol, int left, int right,
               int priority, uint32_t callbackval) { return 0; }
int FX_PlayLoopedWAV(uint8_t *ptr, int32_t loopstart, int32_t loopend,
                     int32_t pitchoffset, int32_t vol, int32_t left, int32_t right, 
                     int32_t priority, uint32_t callbackval) { return 0; }
int FX_PlayVOC3D(uint8_t *ptr, int32_t pitchoffset, int32_t angle, int32_t distance,
                 int32_t priority, uint32_t callbackval) { return 0; }
int FX_PlayWAV3D(uint8_t *ptr, int pitchoffset, int angle, int distance,
                 int priority, uint32_t callbackval) { return 0; }
int FX_PlayRaw(uint8_t *ptr, uint32_t length, uint32_t rate,
               int32_t pitchoffset, int32_t vol, int32_t left, int32_t right, 
               int32_t priority, uint32_t callbackval) { return 0; }
int FX_PlayLoopedRaw(uint8_t *ptr, uint32_t length, char *loopstart,
                     char *loopend, uint32_t rate, int32_t pitchoffset, int32_t vol, 
                     int32_t left, int32_t right, int32_t priority, uint32_t callbackval) { return 0; }

int32_t FX_Pan3D(int handle, int angle, int distance) { return FX_Ok; }
int32_t FX_SoundActive(int32_t handle) { return 0; }
int32_t FX_SoundsPlaying(void) { return 0; }
int32_t FX_StopSound(int handle) { return FX_Ok; }
int32_t FX_StopAllSounds(void) { return FX_Ok; }
int32_t FX_StartDemandFeedPlayback(void (*function)(char **ptr, uint32_t *length),
                                   int32_t rate, int32_t pitchoffset, int32_t vol, 
                                   int32_t left, int32_t right, int32_t priority, 
                                   uint32_t callbackval) { return 0; }
int FX_StartRecording(int MixRate, void (*function)(char *ptr, int length)) { return FX_Error; }
void FX_StopRecord(void) {}

// ============= MUSIC Stub =============

int MUSIC_ErrorCode = MUSIC_Ok;

char *MUSIC_ErrorString(int ErrorNumber) {
    switch (ErrorNumber) {
        case MUSIC_Ok: return "MUSIC ok";
        case MUSIC_Warning: return "MUSIC warning";
        case MUSIC_Error: return "MUSIC error";
        default: return "Unknown MUSIC error";
    }
}

int MUSIC_Init(int SoundCard, int Address) {
    printf("MUSIC_Init: Music disabled for RP2350 port\n");
    return MUSIC_Ok;
}

int MUSIC_Shutdown(void) {
    return MUSIC_Ok;
}

void MUSIC_SetMaxFMMidiChannel(int channel) {}
void MUSIC_SetVolume(int volume) {}
void MUSIC_SetMidiChannelVolume(int channel, int volume) {}
void MUSIC_ResetMidiChannelVolumes(void) {}
int MUSIC_GetVolume(void) { return 0; }
void MUSIC_SetLoopFlag(int loopflag) {}
int MUSIC_SongPlaying(void) { return 0; }
void MUSIC_Continue(void) {}
void MUSIC_Pause(void) {}
int MUSIC_StopSong(void) { return MUSIC_Ok; }
int MUSIC_PlaySong(char *song, int loopflag) { return MUSIC_Ok; }
void MUSIC_SetContext(int context) {}
int MUSIC_GetContext(void) { return 0; }
void MUSIC_SetSongTick(uint32_t PositionInTicks) {}
void MUSIC_SetSongTime(uint32_t milliseconds) {}
void MUSIC_SetSongPosition(int measure, int beat, int tick) {}
void MUSIC_GetSongPosition(songposition *pos) {
    if (pos) memset(pos, 0, sizeof(*pos));
}
void MUSIC_GetSongLength(songposition *pos) {
    if (pos) memset(pos, 0, sizeof(*pos));
}
int MUSIC_FadeVolume(int tovolume, int milliseconds) { return MUSIC_Ok; }
int MUSIC_FadeActive(void) { return 0; }
void MUSIC_StopFade(void) {}
void MUSIC_RerouteMidiChannel(int channel, int (*function)(int event, int c1, int c2)) {}
void MUSIC_RegisterTimbreBank(uint8_t *timbres) {}
