/**
 * @file play_dynamics.c
 * @author Raphael Mayr
 * @date 11 July 2020
 * @brief library to manage movements, scores, collision, creation
 * of objects for Space Invaders Game.
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

#ifndef __PLAY_DYN_H__
#define __PLAY_DYN_H__

/**
 * @defgroup controls dynamics of game
 * functions responsible for movement, scores, collision, creation of objects
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
 * J for Jelly; C for Crab; F for Fred; 
 * M for Mothership; U for upper Wall; 
 * L for lower Wall;
 * 
 * @param state Object is active (1) or not (0)
 * gets displayed (1) or not (0)
 * 
 * @param blink changes state of object
 * -> state change of aliens (other textures)
 * -> also used for move state change (move left or right)
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
 * @param x_coord pixel x_coordinate of p.o.r. of bunker
 * @param y_coord pixel y_coordinate of p.o.r. of bunker
 * 
 * @param low_coll_left_x x_coord of lower left hit wall
 * @param low_coll_left_y y_coord of lower left hit wall
 * 
 * @param low_coll_mid_x x_coord of lower mid hit wall
 * @param low_coll_mid_y y_coord of lower mid hit wall
 * 
 * @param low_coll_right_x x_coord of lower right hit wall
 * @param low_coll_right_y y_coord of lower right hit wall
 * 
 * @param upp_coll_left_x x_coord of upper left hit wall
 * @param upp_coll_left_y y_coord of upper left hit wall
 * 
 * @param upp_coll_mid_x x_coord of upper mid hit wall
 * @param upp_coll_mid_y y_coord of upper mid hit wall
 * 
 * @param upp_coll_right_x x_coord of upper right hit wall
 * @param upp_coll_right_y y_coord of upper rigth hit wall
 * 
 * @param lock lock to gurantee thread-safety
 */ 
typedef struct bunker_objects {
    signed short x_coord;
    signed short y_coord;

    signed short low_coll_left_x;
    signed short low_coll_left_y;

    signed short low_coll_mid_x;
    signed short low_coll_mid_y;

    signed short low_coll_right_x;
    signed short low_coll_right_y;

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
 * @param hscore High Score
 * 
 * @param lives remaining lives for player
 * @param multiplayer indicates whether multiplayer 
 * or singleplayer mode
 * @param level indicates which level is 
 * currently played
 * 
 * @param lock to gurantee thread-safety
 */
typedef struct scores{
    unsigned int score1;
    unsigned int score2;
    unsigned int AI_diff;
    unsigned int hscore;
    unsigned int lives;
    unsigned int multiplayer;
    unsigned int level;
    SemaphoreHandle_t lock;
} Data;
/**
 * @brief creates mutexes once when starting the game
 */
void vCreate_mutexes();
/**
 * @brief initializes playscreen;
 * done when first starting game or resetting from Main Menu
 * 
 * @param inf_lives indicates whether cheat is set or not
 * @param score inits playscreen with current score
 * @param level inits playscreen with current level
 * @param multiplayer multiplayer set(1) or not(0)
 */
void vInit_playscreen(unsigned int inf_lives,
                      unsigned int score, unsigned int level,
                      unsigned int multiplayer);
/** 
 * @brief draws playscreen with all its objects
 * 
 * @param Flags Signals from main task
 * Flag 0: move left; Flag 1: move right
 * Flag 2: shoot; Flag 3: trigger lasershot
 * Flag 4: toggle difficulty;
 * Flag 5: trigger Mothership flythrough
 * 
 * @param ms indicates time gone since last Wake time
 * -> update positions
 */
int vDraw_playscreen(unsigned int *Flags, unsigned int ms);
/**
 * @brief gives movement command from main module to play_dynamics module
 * 
 * @param move which move should the AI mothership do next;
 * INC; DEC; HALT
 */
void vGive_movementData(char* move);
/**
 * @brief called from main.c; main.c receives delta_X;
 * -> to AI binary
 */
int vGet_deltaX();
/**
 * @brief called from main.c; main.c receives bullet status;
 * -> to AI binary
 */
int vGet_attacking();
/**
 * @brief called from main.c; main.c receives difficulty CMD;
 * D1; D2; D3;
 * -> to AI binary
 */
int vGet_difficulty();
/**
 * @brief called from main.c; main.c gives high score to
 * play_dynamics.c
 * -> before: read from file
 */
void vGive_highScore(unsigned int data);
/**
 * @brief called from main.c; main.c receives high score from 
 * play_dynamics.c
 * -> write hscore to file
 */
int vGet_highScore();
/**
 * @brief manages collisions of all screen objects
 * -> calls all other vCheckCollision_... functions
 */
int vCheckCollisions();
/**
 * @brief manages updating Positions of all objects
 * -> calls all other subfunctions
 */
void vUpdatePositions(unsigned int *Flags, unsigned int ms);
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
 * 
 * @param ms time delta to update 
 */
void vUpdate_aliens(unsigned int ms);
/**
 * @brief updates position of player
 * 
 * @param move_left A-Button was pressed -> move character left
 * @param move_right D-Button was pressed -> move character right
 * 
 * @param ms time delta to update 
 */
void vUpdate_player(unsigned int move_left, 
                    unsigned int move_right,
                    unsigned int ms);
/**
 * @brief updates position of mothership in singleplayer
 * 
 * @param ms time delta to update 
 */
void vUpdate_mothership_sp(unsigned int ms);
/**
 * @brief updates position of mothership in multiplayer
 * 
 * @param ms time delta to update 
 */
void vUpdate_mothership_mp(unsigned int ms);
/**
 * @brief updates position of projectile
 * 
 * @param ms time delta to update 
 */
void vUpdate_projectile(unsigned int ms);
/**
 * @brief updates lasershot
 * -> lasershot flies downwards
 * 
 * @param ms time delta to update
 */
void vUpdate_laser(unsigned int ms);


/**
 * @brief draws dynamic score items 
 * 
 * draws scores, high-scores remaining lives and level
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
 * @brief sets mothership active and initializes it
 */
void vCreate_mothership();
/**
 * @brief sets mothership inactive; sets NULL position
 */
void vDelete_mothership();
/**
 * @brief sets projectile active and init. it when shoot is pressed
 */
void vCreate_projectile();
/**
 * @brief sets projectile inactive; sets NULL position
 */
void vDelete_projectile();
/**
 * @brief sets lasershot active
 * -> selects random alien of bottom most row 
 * -> sets coordinates of lasershot to this alien
 */
void vCreate_laser();
/**
 * @brief sets lasershot inactive; sets NULL position
 */
void vDelete_laser();
/**
 * @brief increases score when Object is hit
 * 
 * @param alien_type indicates which type of Object was hit
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
 * @brief sets explosion active; inits coordinates
 * 
 * @param pos_x x_coordinate of impact
 * @param pos_y y_coordinate of impact
 */
void vCreateExplosion(signed short pos_x, signed short pos_y);
/**
 * @brief updates explosion
 * 
 * @param ms time delta to update
 */
void vUpdate_explosion(unsigned int ms);

#endif