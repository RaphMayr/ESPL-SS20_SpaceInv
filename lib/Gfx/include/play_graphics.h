/**
 * @file play_graphics.h
 * @author Raphael Mayr
 * @date 11 July 2020
 * @brief library to draw play graphics
 * 
 * @verbatim
    ----------------------------------------------------------------------
    Copyright (C) Raphael Mayr, 2020
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
   ----------------------------------------------------------------------
 * @endverbatim
 */

#ifndef __PLAY_GRAPH_H__
#define __PLAY_GRAPH_H__

/**
 * @defgroup play_graphics GRAPHICS API
 * 
 * Functions to display gamescreen and object textures;
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
 * @param state there are two textures for each alien;
 * state indicates which one is displayed
*/
void vDraw_crabAlien(signed short pos_x, signed short pos_y,
                        signed short state);
/**
 * @brief draws jelly alien texture 
 * 
 * @param pos_x x position of reference point
 * @param pos_y y position of reference point
 * @param state there are two textures for each alien;
 * state indicates which one is displayed
*/
void vDraw_jellyAlien(signed short pos_x, signed short pos_y,
                        signed short state);
/**
 * @brief draws Projectiles
 * 
 * @param pos_x x position of reference point
 * @param pos_y y position of reference point
 * @param state there are two textures for each alien;
 * state indicates which one is displayed
 */ 
void vDrawProjectile(signed short pos_x, signed short pos_y);
/**
 * @brief draws Mothership
 * 
 * @param pos_x x position of reference point
 * @param pos_y y position of reference point
 */
void vDrawMotherShip(signed short pos_x, signed short pos_y);
/**
 * @brief draws Explosion texture
 * 
 * @param pos_x x position of reference point
 * @param pos_y y position of reference point
 */
void vDrawExplosion(signed short pos_x, signed short pos_y);

#endif