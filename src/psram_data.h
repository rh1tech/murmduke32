/*
 * PSRAM Data Allocation for Duke3D on RP2350
 * 
 * This module handles allocation of large arrays in PSRAM to avoid
 * overflowing the limited 520KB internal RAM.
 */

#ifndef PSRAM_DATA_H
#define PSRAM_DATA_H

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize all PSRAM-based data arrays. Must be called before game starts. */
void psram_data_init(void);

#ifdef __cplusplus
}
#endif

#endif /* PSRAM_DATA_H */
