#ifndef __PLAY_GRAPH_H__
#define __PLAY_GRAPH_H__

/**
 * @defgroup play_graphics GRAPHICS API
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
 * @brief draws static items
 */
void vDrawStaticItems();
/**
 * @brief draws Player texture
 * 
 * @param pos_x x position of reference point
 * @param pos_y y position of reference point
 */
void vDrawPlayer(signed short pos_x, signed short pos_y);
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
/**
 * @brief draws Projectiles
 */ 
void vDrawProjectile(signed short pos_x, signed short pos_y);
/**
 * @brief draws Mothership
 */
void vDrawMotherShip(signed short pos_x, signed short pos_y);
/**
 * @brief draws Explosion texture
 */
void vDrawExplosion(signed short pos_x, signed short pos_y);

#endif