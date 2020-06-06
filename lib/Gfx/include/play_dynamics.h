#ifndef __PLAY_DYN_H__
#define __PLAY_DYN_H__

/**
 * @defgroup controls dynamics of game
 * 
 * functions responsible for movement and game dynamics
 * 
 */

/**
 * @brief struct to represent an element displayed on screen
 * 
 */
typedef struct Screen_objects {
    float f_x;  // absolute x-coordinate
    float f_y;  // absolute y-coordinate
    signed short x_coord;   // pixel x-coordinate
    signed short y_coord;   // pixel y-coordinate
    unsigned int state;     // state object is in
    SemaphoreHandle_t lock; // lock to gurantee thread-safety
} Object;
/**
 * @brief initializes playscreen
 * 
 * done when first starting game or resetting from Main Menu
 */
void vInit_playscreen();
/**
 * @brief draws playscreen with all its objects
 * 
 * @param Flags indicate Button presses
 * @param ms indicates time gone since last Wake time
 * -> update positions
 */
void vDraw_playscreen(unsigned int Flags[4], unsigned int ms);
/**
 * @brief updates position of player
 * 
 * @param move_left A-Button was pressed -> move character left
 * @param move_right D-Button was pressed -> move character right
 * @param ms milliseconds since last wake time of Screen
 */
void vUpdate_player(unsigned int move_left, 
                    unsigned int move_right,
                    unsigned int ms);


void vUpdate_projectile(unsigned int ms);

#endif