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
 * @param f_x absolute x-coordinate
 * @param f_y absoulute y-coordinate
 * @param x_coord pixel x-coordinate
 * @param y_coord pixel y-coordinate
 * 
 * @param type type of Object; 
 * J for Jelly; C for Crab; F for Fred
 * 
 * @param state object is displayed when == 1
 * 
 * @param lock to gurantee thread-safety
 */
typedef struct Screen_objects {
    float f_x;
    float f_y;
    signed short x_coord;
    signed short y_coord;
    char type;              
    unsigned int state;
    SemaphoreHandle_t lock;
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
 * @param Flags Signals from main task
 * Flag 0: move left; Flag 1: move right
 * Flag 2: shoot; Flag 3: move aliens
 * @param ms indicates time gone since last Wake time
 * -> update positions
 */
void vDraw_playscreen(unsigned int Flags[5], unsigned int ms);
/**
 * @brief checks projectile collision with alien
 * 
 * @param alien to be checked
 */
int vCheckCollision(Object alien);
/**
 * @brief creates projectile when shoot is pressed
 * 
 * @return 1 when collision; 0 when no collision
 */
void vCreate_projectile();
/**
 * @brief deletes projectile
 */
void vDelete_projectile();
/**
 * @brief deletes alien 
 * 
 * @param alien to be deleted
 * 
 * @return alien in "deleted" state
 */
Object vDelete_alien();
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
/**
 * @brief updates position of projectile
 * 
 * @param ms time interval for which position is to be updated
 */
void vUpdate_projectile(unsigned int ms);
/**
 * @brief updates aliens position
 * moving left, right and down
 * @param alien to be moved
 * @param state indicates whether to move left or right
 * @param ms time interval for which alien is updated
 */
Object vUpdate_alien(Object alien, unsigned int state,
                        unsigned int ms);
/**
 * @brief creates a Lasershot originating from one of the aliens
 * 
 * @param alien from which the Lasershot shall originate
 */
void vCreate_Lasershot(Object alien);

Object vReset_alien(Object alien, int row, int col);

#endif