#ifndef __MENU_GRAPH_H__
#define __MENU_GRAPH_H__

/**
 * @defgroup menu_graphics GRAPHICS API
 * 
 * Functions to display menu screen and pause screen
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
 */ 
void vDrawMainMenu(unsigned short state);
/**
 * @brief checks inputs for CHEATS Btn and High score Btn
 * 
 * @param mouse_X x-position of Mouse 
 * @param mouse_Y y-position of Mouse
 */
void vCheckMainMenuButtonInput(signed short mouse_X, 
                                signed short mouse_Y);
/**
 * @brief draws pause screen
 */
void vDrawPauseScreen();

#endif
