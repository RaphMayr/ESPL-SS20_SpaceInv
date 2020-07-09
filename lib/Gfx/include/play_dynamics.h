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
 * @param blink changes state of object
 * -> state change of aliens (other textures)
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
    unsigned int blink;
    SemaphoreHandle_t lock;
} Object;
/**
 * @brief struct to represent bunkers
 * 
 */ 
typedef struct bunker_objects {
    signed short x_coord;
    signed short y_coord;

    signed short low_coll_left_x;
    signed short low_coll_left_y;
    unsigned short low_coll_left;

    signed short low_coll_mid_x;
    signed short low_coll_mid_y;
    unsigned short low_coll_mid;

    signed short low_coll_right_x;
    signed short low_coll_right_y;
    unsigned short low_coll_right;

    signed short upp_coll_left_x;
    signed short upp_coll_left_y;
    
    signed short upp_coll_mid_x;
    signed short upp_coll_mid_y;

    signed short upp_coll_right_x;
    signed short upp_coll_right_y;

    SemaphoreHandle_t lock;
} Bunker;
/**
 * @brief struct to represent a element's velocity
 * 
 * @param dx x-velocity
 * @param dy y-velocity
 * @param move_right move right (1) or left (0)
 * 
 * @param lock to gurantee thread-safety
 */
typedef struct velocities {
    unsigned int dx;
    unsigned int dy;
    unsigned int move_right;

    SemaphoreHandle_t lock;
} Velocity;
/**
 * @brief struct to represent game data
 * 
 * @param score1 Score of player 1
 * @param score2 accumulating score 
 * @param AI_diff difficulty of AI
 * @param hscore High Score of both players
 * 
 * @param lives remaining lives for player
 * @param credit credit left
 * @param multiplayer indicates whether multiplayer 
 * or singleplayer mode
 * 
 * @param lock to gurantee thread-safety
 */
typedef struct scores{
    unsigned int score1;
    unsigned int score2;
    unsigned int AI_diff;
    unsigned int hscore;
    unsigned int lives;
    unsigned int credit;
    unsigned int multiplayer;
    unsigned int level;
    SemaphoreHandle_t lock;
} Data;

/**
 * @brief initializes playscreen
 * 
 * done when first starting game or resetting from Main Menu
 */
void vInit_playscreen(unsigned int inf_lives,
                      unsigned int score, unsigned int level,
                      unsigned int multiplayer);
/** 
 * @brief draws playscreen with all its objects
 * 
 * @param Flags Signals from main task
 * Flag 0: move left; Flag 1: move right
 * Flag 2: shoot; Flag 3: periodically create lasershot
 * 
 * @param ms indicates time gone since last Wake time
 * -> update positions
 */
int vDraw_playscreen(unsigned int Flags[5], unsigned int ms);
/**
 * 
 */
void vGive_movementData(char* move);
/**
 * 
 */
int vGet_deltaX();
/**
 * 
 */
int vGet_attacking();
/**
 * 
 */
int vGet_difficulty();
/**
 * 
 */
void vGive_highScore(unsigned int data);
/**
 * 
 */
int vGet_highScore();
/**
 * @brief checks collisions of all screen objects
 * -> calls all other vCheckCollision_... functions
 */
int vCheckCollisions();
/**
 * @brief update Positions
 */
void vUpdatePositions(unsigned int Flags[5], unsigned int ms);
/**
 * @brief draws all dynamic Items
 */
void vDrawDynamicItems();


/**
 * @brief checks collision of projectile and alien
 * 
 * @return 1 when collision
 */
int vCheckCollision_proj_alien();
/**
 * @brief checks collision of projectile and bunkers
 * 
 * @return 1 when collision
 */
int vCheckCollision_proj_bunker();
/**
 * @brief checks collision of projectile and upper wall
 * 
 * @return 1 when collision
 */
int vCheckCollision_proj_upper();
/**
 * @brief checks collision of projectile and mothership
 * 
 * @return 1 when collision
 */
int vCheckCollision_proj_mothership();
/**
 * @brief checks collision of laser and player
 * 
 * @return 1 when collision 
 */
int vCheckCollision_laser_player();
/**
 * @brief checks collision of laser and projectile
 * -> when they collide both get deleted
 * @return 1 when collision
 */
int vCheckCollision_laser_proj();
/**
 * @brief checks collision of laser and bunkers
 * 
 * @return 1 when collision 
 */
int vCheckCollision_laser_bunker();
/**
 * @brief check collision of laser and bottom wall
 * 
 * @return 1 when collision
 */
int vCheckCollision_laser_bottom();
/**
 * @brief check collision of aliens and player
 * -> game over when aliens reach player
 * @return 1 when collision
 */
int vCheckCollision_alien_player();


/**
 * @brief updates aliens position
 * moving left, right and down
 * @param alien to be moved
 * @param state indicates whether to move left or right
 * @param ms time interval for which alien is updated
 */
void vUpdate_aliens(unsigned int Flags[5], unsigned int ms);
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
 * @brief updates position of mothership with Async IO
 */
void vUpdate_mothership(unsigned int ms);
/**
 * @brief updates position of projectile
 * 
 * @param ms time interval for which position is to be updated
 */
void vUpdate_projectile(unsigned int ms);
/**
 * @brief updates lasershot
 * -> lasershot flies downwards
 */
void vUpdate_laser();


/**
 * @brief draws dynamic score items 
 * 
 * draws scores, high-scores remaining lives and Credit
 */
void vDrawScores();
/**
 * @brief draws alien matrix
 */
void vDrawAliens();
/**
 * @brief draws bunkers
 */
void vDrawBunkers();


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
 * @brief create lasershot
 * -> selects random alien of bottom most row 
 * -> sets coordinates of lasershot to this alien
 */
void vCreate_laser();

/**
 * @brief deletes lasershot
 */
void vDelete_laser();

/**
 * @brief increases score when alien is hit
 */
void vIncrease_score(char alien_type);

/**
 * @brief checks if alien-array is empty
 * 
 * @return 1 when array is empty
 */
int vCheck_aliensleft();

/**
 * @brief draws next level screen
 */
void vDrawNextLevelScreen(unsigned int level);

/**
 * 
 */
void vCreateExplosion(signed short pos_x, signed short pos_y);
/**
 * 
 */
void vUpdate_explosion(unsigned int ms);

#endif