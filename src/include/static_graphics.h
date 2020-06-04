#ifndef __STAT_GRAPH__
#define __STAT_GRAPH__

#define px 2

void vDrawPlayScreen(void);

void vDrawMotherShip(signed short pos_x, signed short pos_y);

void vDrawBunker(signed short pos_x, signed short pos_y);

void vDraw_fredAlien(signed short pos_x, signed short pos_y, 
                        signed short state);

void vDraw_crabAlien(signed short pos_x, signed short pos_y,
                        signed short state);

void vDraw_jellyAlien(signed short pos_x, signed short pos_y,
                        signed short state);

#endif