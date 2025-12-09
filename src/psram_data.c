/*
 * PSRAM Data Allocation for Duke3D on RP2350
 * 
 * Allocates large game arrays in PSRAM (8MB at 0x11000000) instead of
 * internal RAM (520KB) to avoid overflow.
 *
 * This file DEFINES the actual array pointers that were previously
 * static arrays. By allocating them in PSRAM at runtime, we avoid
 * overflowing the limited 520KB internal SRAM.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "psram_allocator.h"

/* Define PSRAM_DATA_OWNER to signal that we define the pointers here */
#define PSRAM_DATA_OWNER

/* Include types - build.h needs this for struct definitions */
#include "build.h"
#include "duke3d.h"

/* ============== Engine Arrays (from build.h) ============== */
/* These replace the arrays defined in engine.c via EXTERN macro */

sectortype *sector = NULL;
walltype *wall = NULL;
spritetype *sprite = NULL;
spritetype *tsprite = NULL;
int32_t *ylookup = NULL;
int32_t *validmodexdim = NULL;
int32_t *validmodeydim = NULL;
short *sintable = NULL;
uint8_t *palette = NULL;
short *startumost = NULL;
short *startdmost = NULL;
short *headspritesect = NULL;
short *headspritestat = NULL;
short *prevspritesect = NULL;
short *prevspritestat = NULL;
short *nextspritesect = NULL;
short *nextspritestat = NULL;
uint8_t *show2dsector = NULL;
uint8_t *show2dwall = NULL;
uint8_t *show2dsprite = NULL;
uint8_t *visitedSectors = NULL;

/* ============== Engine Static Arrays (from engine.c) ============== */
/* These need to be moved from engine.c static to here */

pvWall_t *pvWalls = NULL;
short *bunchWallsList = NULL;
short *bunchfirst = NULL;
short *bunchlast = NULL;
short *smost = NULL;
short *smoststart = NULL;
uint8_t *smostwalltype = NULL;
int32_t *smostwall = NULL;
short *maskwall = NULL;
short maskwallcnt = 0;
short smostcnt = 0;
int32_t *spritesx = NULL;
int32_t *spritesy = NULL;
spritetype **tspriteptr = NULL;
int32_t *spritesz = NULL;
int16_t *uwall = NULL;
int16_t *dwall = NULL;
int32_t *swplc = NULL;
int32_t *lplc = NULL;
int32_t *swall = NULL;
int32_t *lwall = NULL;
int32_t *lastx = NULL;
int32_t *slopalookup = NULL;
short *radarang = NULL;
short *radarang2 = NULL;
uint16_t *sqrtable = NULL;
uint16_t *shlookup = NULL;

/* ============== Game Arrays (from global.c/duke3d.h) ============== */

struct weaponhit *hittype = NULL;
int32_t *script = NULL;
int32_t **actorscrptr = NULL;
/* inputfifo and recsync kept as static arrays in global.c - 2D access pattern */
int32_t *oldipos = NULL;
int32_t *bakipos = NULL;
int32_t **curipos = NULL;
int32_t *myxbak = NULL;
int32_t *myybak = NULL;
int32_t *myzbak = NULL;
struct player_struct *ps = NULL;

/* Helper macro for allocation with error checking */
#define PSRAM_ALLOC(ptr, type, count, name) do { \
    size_t size = sizeof(type) * (count); \
    ptr = (type *)psram_malloc(size); \
    if (!ptr) { \
        printf("FATAL: PSRAM alloc failed for %s (%u bytes)\n", name, (unsigned)size); \
        while(1); \
    } \
    memset(ptr, 0, size); \
    total_allocated += size; \
    printf("PSRAM: %s = %u bytes\n", name, (unsigned)size); \
} while(0)

/* Local constants needed for engine.c arrays */
#define MAXWALLSB_LOCAL 2048
#define MAXYSAVES_LOCAL ((MAXXDIM*MAXSPRITES)>>7)

void psram_data_init(void) {
    size_t total_allocated = 0;
    
    printf("\n=== PSRAM Data Allocation ===\n");
    
    /* Engine arrays from build.h */
    PSRAM_ALLOC(sector, sectortype, MAXSECTORS, "sector");
    PSRAM_ALLOC(wall, walltype, MAXWALLS, "wall");
    PSRAM_ALLOC(sprite, spritetype, MAXSPRITES, "sprite");
    PSRAM_ALLOC(tsprite, spritetype, MAXSPRITESONSCREEN, "tsprite");
    PSRAM_ALLOC(ylookup, int32_t, MAXYDIM + 1, "ylookup");
    PSRAM_ALLOC(validmodexdim, int32_t, 256, "validmodexdim");
    PSRAM_ALLOC(validmodeydim, int32_t, 256, "validmodeydim");
    PSRAM_ALLOC(sintable, short, 2048, "sintable");
    PSRAM_ALLOC(palette, uint8_t, 768, "palette");
    PSRAM_ALLOC(startumost, short, MAXXDIM, "startumost");
    PSRAM_ALLOC(startdmost, short, MAXXDIM, "startdmost");
    PSRAM_ALLOC(headspritesect, short, MAXSECTORS + 1, "headspritesect");
    PSRAM_ALLOC(headspritestat, short, MAXSTATUS + 1, "headspritestat");
    PSRAM_ALLOC(prevspritesect, short, MAXSPRITES, "prevspritesect");
    PSRAM_ALLOC(prevspritestat, short, MAXSPRITES, "prevspritestat");
    PSRAM_ALLOC(nextspritesect, short, MAXSPRITES, "nextspritesect");
    PSRAM_ALLOC(nextspritestat, short, MAXSPRITES, "nextspritestat");
    PSRAM_ALLOC(show2dsector, uint8_t, (MAXSECTORS + 7) >> 3, "show2dsector");
    PSRAM_ALLOC(show2dwall, uint8_t, (MAXWALLS + 7) >> 3, "show2dwall");
    PSRAM_ALLOC(show2dsprite, uint8_t, (MAXSPRITES + 7) >> 3, "show2dsprite");
    PSRAM_ALLOC(visitedSectors, uint8_t, (MAXSECTORS + 7) >> 3, "visitedSectors");
    
    /* Engine static arrays from engine.c */
    PSRAM_ALLOC(pvWalls, pvWall_t, MAXWALLSB_LOCAL, "pvWalls");
    PSRAM_ALLOC(bunchWallsList, short, MAXWALLSB_LOCAL, "bunchWallsList");
    PSRAM_ALLOC(bunchfirst, short, MAXWALLSB_LOCAL, "bunchfirst");
    PSRAM_ALLOC(bunchlast, short, MAXWALLSB_LOCAL, "bunchlast");
    PSRAM_ALLOC(smost, short, MAXYSAVES_LOCAL, "smost");
    PSRAM_ALLOC(smoststart, short, MAXWALLSB_LOCAL, "smoststart");
    PSRAM_ALLOC(smostwalltype, uint8_t, MAXWALLSB_LOCAL, "smostwalltype");
    PSRAM_ALLOC(smostwall, int32_t, MAXWALLSB_LOCAL, "smostwall");
    PSRAM_ALLOC(maskwall, short, MAXWALLSB_LOCAL, "maskwall");
    PSRAM_ALLOC(spritesx, int32_t, MAXSPRITESONSCREEN, "spritesx");
    PSRAM_ALLOC(spritesy, int32_t, MAXSPRITESONSCREEN + 1, "spritesy");
    PSRAM_ALLOC(tspriteptr, spritetype*, MAXSPRITESONSCREEN, "tspriteptr");
    PSRAM_ALLOC(spritesz, int32_t, MAXSPRITESONSCREEN, "spritesz");
    PSRAM_ALLOC(uwall, int16_t, MAXXDIM + 1, "uwall");
    PSRAM_ALLOC(dwall, int16_t, MAXXDIM + 1, "dwall");
    PSRAM_ALLOC(swplc, int32_t, MAXXDIM + 1, "swplc");
    PSRAM_ALLOC(lplc, int32_t, MAXXDIM + 1, "lplc");
    PSRAM_ALLOC(swall, int32_t, MAXXDIM + 1, "swall");
    PSRAM_ALLOC(lwall, int32_t, MAXXDIM + 4, "lwall");
    PSRAM_ALLOC(lastx, int32_t, MAXYDIM, "lastx");
    PSRAM_ALLOC(slopalookup, int32_t, 16384, "slopalookup");
    PSRAM_ALLOC(radarang, short, 1280, "radarang");
    PSRAM_ALLOC(radarang2, short, MAXXDIM + 1, "radarang2");
    PSRAM_ALLOC(sqrtable, uint16_t, 4096, "sqrtable");
    PSRAM_ALLOC(shlookup, uint16_t, 4096 + 256, "shlookup");
    
    /* Game arrays from global.c */
    PSRAM_ALLOC(hittype, struct weaponhit, MAXSPRITES, "hittype");
    PSRAM_ALLOC(script, int32_t, MAXSCRIPTSIZE, "script");
    PSRAM_ALLOC(actorscrptr, int32_t*, MAXTILES, "actorscrptr");
    /* inputfifo and recsync kept in SRAM (2D array access pattern) */
    PSRAM_ALLOC(oldipos, int32_t, MAXINTERPOLATIONS, "oldipos");
    PSRAM_ALLOC(bakipos, int32_t, MAXINTERPOLATIONS, "bakipos");
    PSRAM_ALLOC(curipos, int32_t*, MAXINTERPOLATIONS, "curipos");
    PSRAM_ALLOC(myxbak, int32_t, MOVEFIFOSIZ, "myxbak");
    PSRAM_ALLOC(myybak, int32_t, MOVEFIFOSIZ, "myybak");
    PSRAM_ALLOC(myzbak, int32_t, MOVEFIFOSIZ, "myzbak");
    PSRAM_ALLOC(ps, struct player_struct, MAXPLAYERS, "ps");
    
    printf("=== Total PSRAM allocated: %u bytes (%.2f MB) ===\n\n",
           (unsigned)total_allocated, total_allocated / (1024.0 * 1024.0));
}
