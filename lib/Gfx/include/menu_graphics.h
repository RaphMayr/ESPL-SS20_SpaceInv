/**
 * @file menu_graphics.h
 * @author Raphael Mayr
 * @date 11 July 2020
 * @brief library to draw menu graphics
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

#ifndef __MENU_GRAPH_H__
#define __MENU_GRAPH_H__

/**
 * @defgroup menu_graphics GRAPHICS API
 * 
 * Functions to display menus
 */

/**
 * Sets size of pixels
 * pixels here is a size unit 
 */
#define px 2
/**
 * @brief draws welcome screen 
 * 
 * @param state enables start game text to blink 
 * @param multipl indicates whether multiplayer mode is set(1) or not
 */ 
void vDrawMainMenu(unsigned short state, unsigned int multipl);
/**
 * @brief checks inputs for CHEATS Btn and High score Btn
 * 
 * @param mouse_X x-position of Mouse 
 * @param mouse_Y y-position of Mouse
 */
int vCheckMainMenuButtonInput(signed short mouse_X, 
                                signed short mouse_Y);
/**
 * @brief draws pause screen
 */
void vDrawPauseScreen();
/**
 * @brief draws Cheat screen 
 */
void vDrawCheatScreen(unsigned int infLives, unsigned int score,
                      unsigned int level);
/**
 * @brief check cheat screen input
 */
int vCheckCheatScreenInput(signed short mouse_x, 
                            signed short mouse_y);
/**
 * @brief draws High score screen
 */
void vDrawHScoreScreen();
/**
 * @brief draws Game Over Font
 */ 
void vDrawGameOver();

#endif
