#ifndef __STAT_GRAPH_H__
#define __STAT_GRAPH_H__

/**
 * @defgroup static_graphics GAME GRAPHICS API
 * 
 * Functions to display gamescreen and objects
 * reference points of figures are included in docs
 */


/**
 * Sets size of pixels
 * pixels here is a size unit 
 */
#define px 2
/**
 * @brief draws the background to play on 
 */ 
void vDrawPlayScreen(void);
/**
 * @brief draws mothership texture
 * 
 * @param pos_x x position of reference point
 * @param pos_y y position of reference point
 */
void vDrawMotherShip(signed short pos_x, signed short pos_y);
/**
 * @brief draws bunker texture 
 * 
 * @param pos_x x position of reference point
 * @param pos_y y position of reference point
 */
void vDrawBunker(signed short pos_x, signed short pos_y);
/**
 * @brief draws fred alien texture 
 * 
 * @param pos_x x position of reference point
 * @param pos_y y position of reference point
*/
void vDraw_fredAlien(signed short pos_x, signed short pos_y, 
                        signed short state);
/**
 * @brief draws crab alien texture 
 * 
 * @param pos_x x position of reference point
 * @param pos_y y position of reference point
*/
void vDraw_crabAlien(signed short pos_x, signed short pos_y,
                        signed short state);
/**
 * @brief draws jelly alien texture 
 * 
 * @param pos_x x position of reference point
 * @param pos_y y position of reference point
*/
void vDraw_jellyAlien(signed short pos_x, signed short pos_y,
                        signed short state);

#endif