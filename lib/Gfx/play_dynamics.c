/**
 * @file play_dynamics.c
 * @author Raphael Mayr
 * @date 11 July 2020
 * @brief library to manage movements, scores, collision, creation
 * of objects.
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
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>

#include <SDL2/SDL_scancode.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"
#include "timers.h"

#include "TUM_Ball.h"
#include "TUM_Draw.h"
#include "TUM_Event.h"
#include "TUM_Sound.h"
#include "TUM_Utils.h"
#include "TUM_Font.h"

#include "play_dynamics.h"
#include "play_graphics.h"
#include "menu_graphics.h"

#define CENTER_X SCREEN_WIDTH / 2
#define CENTER_Y SCREEN_HEIGHT / 2

#define LEFT_CONSTRAINT_X 100
#define RIGHT_CONSTRAINT_X 540

#define MAX_X_VELO 80
#define INC_VELO 2

#define SCORE_THRESHOLD 1080

#define LEVEL_SCORE 720

#define SCORE_JELLY 30
#define SCORE_CRAB 20
#define SCORE_FRED 10
#define SCORE_MOTHERSHIP 200

#define DX_ALIEN 20
#define DY_ALIEN 1

#define DY_PROJECTILE 300
#define DY_LASER 200 

Object player = { 0 };

Object mothership = { 0 };

Object projectile = { 0 };
Object laser = { 0 };

Object upper_wall = { 0 };
Object lower_wall = { 0 };

Bunker bunkers[4] = { 0 };

Object aliens[5][10] = { 0 };

Velocity alien_velo = { 0 };

Data gamedata = { 0 };

Object explosion = { 0 };

void vCreate_mutexes()
{   
    // create locks
    gamedata.lock = xSemaphoreCreateMutex();
    for (int row=0; row < 5; row++) {
        for (int col=0; col < 10; col++) {
            // fine grained locking
            aliens[row][col].lock = xSemaphoreCreateMutex();
        }
    }
    alien_velo.lock = xSemaphoreCreateMutex();
    upper_wall.lock = xSemaphoreCreateMutex();
    lower_wall.lock = xSemaphoreCreateMutex();
    player.lock = xSemaphoreCreateMutex();
    mothership.lock = xSemaphoreCreateMutex();
    projectile.lock = xSemaphoreCreateMutex();
    laser.lock = xSemaphoreCreateMutex();
    explosion.lock = xSemaphoreCreateMutex();

    for (int row=0; row<4; row++) {
        bunkers[row].lock = xSemaphoreCreateMutex();
    }
}

void vInit_playscreen(unsigned int inf_lives,
                      unsigned int score, unsigned int level,
                      unsigned int multiplayer,
                      unsigned int cheat_set)
{
    // initialize gamedata
    if (xSemaphoreTake(gamedata.lock, 0)) {
        
        if (level == 1) {
            gamedata.score1 = 0;
            gamedata.level = 1;
            // cheat infinite lives set
            if (inf_lives == 1) {
                // value doesn't get changed when 1000
                gamedata.lives = 1000;  // "INF" gets displayed
                gamedata.cheats_set = 1;
            }
            else {
                // cheat not set
                gamedata.lives = 3;
                gamedata.cheats_set = 0;
            }
        }
        else if (level == 0) {
            gamedata.score1 = 0;
            gamedata.level = 1;
            // cheat infinite lives set
            if (inf_lives == 1) {
                // value doesn't get changed when 1000
                gamedata.lives = 1000;  // "INF" gets displayed
                gamedata.cheats_set = 1;
            }
            else {
                // cheat not set
                gamedata.lives = 3;
                gamedata.cheats_set = 0;
            }
        }
        else {
            if (cheat_set) {
                gamedata.cheats_set = 1;
                if (inf_lives) {
                    gamedata.lives = 1000;
                }
                else {
                    gamedata.lives = 3;
                }
                gamedata.score1 = (level-1)*LEVEL_SCORE;
                gamedata.level = level;
            }
            else {
                gamedata.cheats_set = 0;
                gamedata.level = level;
            }
        }
        // multiplayer or not 
        // -> AI or not
        if (multiplayer) {
            gamedata.multiplayer = 1;
            gamedata.AI_diff = 2;
        }
        else {
            gamedata.multiplayer = 0;
            gamedata.AI_diff = 0;
        }

        xSemaphoreGive(gamedata.lock);
    }
    // initialize alien matrix
    for (int row=0; row < 5; row++) {
        for (int col=0; col < 10; col++) {
            // fine grained locking
            if (xSemaphoreTake(aliens[row][col].lock, portMAX_DELAY)) {
                // one row Jellies
                if (row == 0) {
                    aliens[row][col].type = 'J';
                    aliens[row][col].x_coord = 151 + col*30;
                }
                // two rows Crabs 
                if (row == 1 || row == 2) {
                    aliens[row][col].type = 'C';
                    aliens[row][col].x_coord = 149 + col*31;
                }
                // two rows Freds
                if (row == 3 || row == 4) {
                    aliens[row][col].type = 'F';
                    aliens[row][col].x_coord = 132 + col*34;
                }

                aliens[row][col].y_coord = CENTER_Y - 130 + row *40;

                aliens[row][col].f_x = aliens[row][col].x_coord;
                aliens[row][col].f_y = aliens[row][col].y_coord;

                aliens[row][col].state = 1; // sets aliens active
                xSemaphoreGive(aliens[row][col].lock);
            }
        }
    }
    
    // initialize alien velocities
    if (xSemaphoreTake(alien_velo.lock, portMAX_DELAY)) {
        // velocity increases for level count
        alien_velo.dx = DX_ALIEN + level*10; 
        alien_velo.dy = DY_ALIEN;

        // initially moving right
        alien_velo.move_right = 1;

        xSemaphoreGive(alien_velo.lock);
    }

    // initialize upper wall
    if (xSemaphoreTake(upper_wall.lock, portMAX_DELAY)) {

        upper_wall.x_coord = 100;
        upper_wall.y_coord = 50;

        upper_wall.f_x = upper_wall.x_coord;
        upper_wall.f_y = upper_wall.y_coord;

        upper_wall.type = 'U';

        upper_wall.state = 1;

        xSemaphoreGive(upper_wall.lock);
    }

    // initialize lower wall
    if (xSemaphoreTake(lower_wall.lock, portMAX_DELAY)) {

        lower_wall.x_coord = 100;
        lower_wall.y_coord = CENTER_Y + 160;

        lower_wall.f_x = lower_wall.x_coord;
        lower_wall.f_y = lower_wall.y_coord;

        lower_wall.type = 'L';

        lower_wall.state = 1;

        xSemaphoreGive(lower_wall.lock);
    }

    // initialize player
    if (xSemaphoreTake(player.lock, portMAX_DELAY)) {

        player.x_coord = CENTER_X - 6*px;
        player.y_coord = CENTER_Y + 185;

        player.f_x = player.x_coord;
        player.f_y = player.y_coord;

        player.state = 1;

        xSemaphoreGive(player.lock);
    }

    // initialize mothership
    if (gamedata.multiplayer) {
        if (xSemaphoreTake(mothership.lock, portMAX_DELAY)) {

            mothership.x_coord = CENTER_X - 6*px;
            mothership.y_coord = CENTER_Y - 165;

            mothership.f_x = mothership.x_coord;
            mothership.f_y = mothership.y_coord;

            mothership.type = 'M';

            mothership.state = 1;

            xSemaphoreGive(mothership.lock);
        }
    }
    else {
        if (xSemaphoreTake(mothership.lock, portMAX_DELAY)) {
            // NULL coordinates
            mothership.x_coord = 0;
            mothership.y_coord = 0;

            mothership.f_x = 0;
            mothership.f_y = 0;

            mothership.type = 'M';

            mothership.state = 0;

            mothership.blink = 1;

            xSemaphoreGive(mothership.lock);
        }
    }

    // initialize projectile
    if (xSemaphoreTake(projectile.lock, portMAX_DELAY)) {
        // NULL coordinates
        projectile.x_coord = 0;
        projectile.y_coord = 0;

        projectile.f_x = projectile.x_coord;
        projectile.f_y = projectile.y_coord;

        projectile.state = 0;

        xSemaphoreGive(projectile.lock);
    }

    // initialize laser
    if (xSemaphoreTake(laser.lock, portMAX_DELAY)) {
        // NULL coordinates
        laser.x_coord = 0;
        laser.y_coord = 0;

        laser.f_x = laser.x_coord;
        laser.f_y = laser.y_coord;

        laser.state = 0;

        xSemaphoreGive(laser.lock);
    }
    // initialize explosion
    if (xSemaphoreTake(explosion.lock, portMAX_DELAY)) {
        // NULL coordinates
        explosion.x_coord = 0;
        explosion.y_coord = 0;

        explosion.f_x = explosion.x_coord;
        explosion.f_y = explosion.y_coord;

        explosion.state = 0;

        explosion.blink = 0;

        xSemaphoreGive(explosion.lock);
    }
    // initialize bunkers
    for (int row=0; row<4; row++) {
        if (xSemaphoreTake(bunkers[row].lock, portMAX_DELAY)) {

            bunkers[row].x_coord = 145 + 100*row;
            bunkers[row].y_coord = CENTER_Y + 130;

            bunkers[row].low_coll_left_x = bunkers[row].x_coord;
            bunkers[row].low_coll_left_y = bunkers[row].y_coord + 13*px;

            bunkers[row].low_coll_mid_x = bunkers[row].x_coord + 8*px;
            bunkers[row].low_coll_mid_y = bunkers[row].y_coord + 9*px;

            bunkers[row].low_coll_right_x = bunkers[row].x_coord + 16*px;
            bunkers[row].low_coll_right_y = bunkers[row].y_coord + 13*px;

            bunkers[row].upp_coll_left_x = bunkers[row].x_coord;
            bunkers[row].upp_coll_left_y = bunkers[row].y_coord;

            bunkers[row].upp_coll_mid_x = bunkers[row].x_coord + 8*px;
            bunkers[row].upp_coll_mid_y = bunkers[row].y_coord;

            bunkers[row].upp_coll_right_x = bunkers[row].x_coord + 16*px;
            bunkers[row].upp_coll_right_y = bunkers[row].y_coord;


            xSemaphoreGive(bunkers[row].lock);
        }
    }
}

// set all aliens inactive; for testing purposes only
void vEmpty_aliens()
{
    for (int row=0; row < 5; row++) {
        for (int col=0; col < 10; col++) {

            aliens[row][col].state = 0;
        }
    }
}

void vGive_movementData(char* move) 
{
    char* increment = "INC";
    char* decrement = "DEC";

    if (gamedata.multiplayer) {
        if (!strcmp(increment, move)) {
            mothership.blink = 1; // move right
        }
        
        else if (!strcmp(decrement, move)) {
            mothership.blink = 0;   // move left
        }

        else {
            mothership.blink = 2;
        }
    }

}

void vGive_highScore(unsigned int data)
{
    if (xSemaphoreTake(gamedata.lock, 0)) {
        gamedata.hscore = data;

        xSemaphoreGive(gamedata.lock);
    }  
}

int vGet_highScore() 
{
    unsigned int hscore;
    if (xSemaphoreTake(gamedata.lock, 0)) {
        hscore = gamedata.hscore;

        xSemaphoreGive(gamedata.lock);
    }
    return hscore;
}

int vDraw_playscreen(unsigned int *Flags, unsigned int ms)
{

    if (vCheck_aliensleft()) {     
        // when no aliens left progress to nxt lvl
        return 2;
    }

    // draw all items that don't move
    vDrawStaticItems();

    // count Hi-Score up with score 
    // when it gets greater than current hscore
    if (xSemaphoreTake(gamedata.lock, 0)) {
        if (gamedata.cheats_set == 0) {
            if (gamedata.score1 > gamedata.hscore) {
                gamedata.hscore = gamedata.score1;
            }
        } 
        xSemaphoreGive(gamedata.lock);
    }

    // projectile is initialized when shoot flag is set and not active
    if (Flags[2] && (projectile.state == 0)) {  
        vCreate_projectile();
    }
    
    if (Flags[3]) {     // periodic timer from main function
        vCreate_laser();
        
    }

    if (gamedata.multiplayer == 0) {
        if (Flags[5]) {
            vCreate_mothership();
        }
    }
    
    // adjust difficulty (only a multiplayer functionality)
    if (gamedata.multiplayer) {
        if (Flags[4]) {
            if (xSemaphoreTake(gamedata.lock, 0)) {
                switch(gamedata.AI_diff) {
                    case 1:
                        gamedata.AI_diff = 2;
                        break;
                    case 2:
                        gamedata.AI_diff = 3;
                        break;
                    case 3:
                        gamedata.AI_diff = 1;
                        break;
                    default:
                        break;
                }
                xSemaphoreGive(gamedata.lock);
            }
        }
    }
    

    // check collisions returns 1 when collision alien player occurs
    if (vCheckCollisions() || gamedata.lives == 0) {    
        vDrawGameOver();
        return 1;
    }

    vUpdatePositions(Flags, ms);    // update positions

    vDrawDynamicItems();        // draw dynamic items
    
    
    return 0;
}

int vGet_deltaX()
{
    signed int deltaX = 0;

    if (xSemaphoreTake(mothership.lock, 0)) {
        deltaX = mothership.x_coord - player.x_coord;

        xSemaphoreGive(mothership.lock);
    }
    
    
    // return delta x between mothership and player
    return deltaX;
}

int vGet_attacking()
{
    unsigned int attacking = 0;

    if (projectile.state) {
        attacking = 1;
    }

    return attacking;
}

int vGet_difficulty()
{
    unsigned int difficulty = 0;

    difficulty = gamedata.AI_diff;

    return difficulty;
}

int vCheckCollisions() 
{
    
    if (vCheckCollision_proj_alien()) {
        
        // increase speed alien
        if (xSemaphoreTake(alien_velo.lock, 0)) {
            if (alien_velo.dx <= MAX_X_VELO) {
                alien_velo.dx += INC_VELO;
                printf("increasing velocity\n");
                printf("x-velocity: %i\n", alien_velo.dx);
            }
            xSemaphoreGive(alien_velo.lock);
        }
        if (xSemaphoreTake(explosion.lock, 0)) {
            
            explosion.x_coord = projectile.x_coord - 3*px;
            explosion.y_coord = projectile.y_coord;

            explosion.state = 1;

            xSemaphoreGive(explosion.lock);
        }
        vDelete_projectile();
    }
    
    if (vCheckCollision_proj_bunker()) {
        vDelete_projectile();
    }
    
    if (vCheckCollision_proj_upper()) {
        vDelete_projectile();
    }

    if (vCheckCollision_proj_mothership()){
        vDelete_projectile();
    }

    if (vCheckCollision_laser_player()) {
        vDelete_laser();
        if (xSemaphoreTake(gamedata.lock, 0)) {
            // when INF no change occurs
            if (gamedata.lives != 1000) {   
                gamedata.lives--;
            }
            xSemaphoreGive(gamedata.lock);
        }
    }
    if (vCheckCollision_laser_proj()) {
        vDelete_laser();
        vDelete_projectile();
    }
    if (vCheckCollision_laser_bunker()) {
        vDelete_laser();
    }
    if (vCheckCollision_laser_bottom()) {
        vDelete_laser();
    }
    if (vCheckCollision_alien_player()) {
        return 1;
    }

    return 0;
}

    void vUpdatePositions(unsigned int *Flags, unsigned int ms)
{
    /**
     * 1. update aliens positions
     * 2. update projectile position
     * 3. update laser position
     * 4. update player position
     */


    vUpdate_aliens(ms);
    
    if (projectile.state) {
        vUpdate_projectile(ms);
    }
    if (laser.state) {
        vUpdate_laser(ms);
    }
    if (explosion.state) {
        vUpdate_explosion(ms);
    }
    vUpdate_player(Flags[0], Flags[1], ms);

    if (mothership.state && gamedata.multiplayer) {
        vUpdate_mothership_mp(ms);
    }
    
    if (mothership.state && gamedata.multiplayer == 0) {
        vUpdate_mothership_sp(ms);
    }
    
}

void vDrawDynamicItems() 
{
    /**
     * 1. draw Scores
     * 2. draw Aliens
     * 3. draw Laser
     * 4. draw bunkers
     * 5. draw projectile
     * 6. draw player
     */

    vDrawScores();

    
    
    vDrawBunkers();

    if (xSemaphoreTake(laser.lock, 0)) {
        if (laser.state) {
            vDrawProjectile(laser.x_coord, laser.y_coord);
        }
        xSemaphoreGive(laser.lock);
    }

    vDrawAliens();

    if (xSemaphoreTake(projectile.lock, 0)) {
        if (projectile.state) {
            vDrawProjectile(projectile.x_coord, projectile.y_coord);
        }
        xSemaphoreGive(projectile.lock);
    }
    if (xSemaphoreTake(explosion.lock, 0)) {
        if (explosion.state) {
            vDrawExplosion(explosion.x_coord, explosion.y_coord);
        }
        xSemaphoreGive(explosion.lock);
    }
    if (xSemaphoreTake(player.lock, 0)) {
        vDrawPlayer(player.x_coord, player.y_coord);

        xSemaphoreGive(player.lock);
    }
    if (mothership.state) {
        vDrawMotherShip(mothership.x_coord, mothership.y_coord);
    }
    
}
/**
 * #####################################################
 * CHECK COLLISION FUNCTIONS
 */

int vCheckCollision_laser_bottom()
{
    signed short laser_y;

    if (xSemaphoreTake(laser.lock, 0)) {

        laser_y = laser.y_coord + 2*px;

        xSemaphoreGive(laser.lock);
    }
    if (laser_y >= SCREEN_HEIGHT - 30) {
        return 1;
    }
    return 0;
}

int vCheckCollision_laser_bunker() 
{

    signed short hit_wall_left_x;
    signed short hit_wall_left_y;

    signed short hit_wall_mid_x;
    signed short hit_wall_mid_y;

    signed short hit_wall_right_x;
    signed short hit_wall_right_y;

    unsigned short width = 8*px;

    for (int row=0; row < 4; row++) {
        if (xSemaphoreTake(bunkers[row].lock, 0)) {

            hit_wall_left_x = bunkers[row].upp_coll_left_x;
            hit_wall_left_y = bunkers[row].upp_coll_left_y;

            hit_wall_mid_x = bunkers[row].upp_coll_mid_x;
            hit_wall_mid_y = bunkers[row].upp_coll_mid_y;

            hit_wall_right_x = bunkers[row].upp_coll_right_x;
            hit_wall_right_y = bunkers[row].upp_coll_right_y;

            xSemaphoreGive(bunkers[row].lock);
        }
        if (laser.state) {

            if (xSemaphoreTake(laser.lock, 0)) {

                if ((hit_wall_left_y <= laser.y_coord) 
                    && (laser.y_coord <= hit_wall_left_y + 5*px)) {
                    
                    if ((hit_wall_left_x <= laser.x_coord) &&
                            (laser.x_coord <= hit_wall_left_x + width)) {
                        
                            xSemaphoreGive(laser.lock);

                        if (xSemaphoreTake(bunkers[row].lock, 0)) {

                            if (bunkers[row].upp_coll_left_y >=
                                    bunkers[row].low_coll_left_y + 4*px) {
                                
                                xSemaphoreGive(bunkers[row].lock);

                                return 0;
                            }
                            else {
                                bunkers[row].upp_coll_left_y += 4*px;
                            }
                            xSemaphoreGive(bunkers[row].lock);
                        }
                        return 1;
                    }
                }

                if ((hit_wall_mid_y <= laser.y_coord) 
                    && (laser.y_coord <= hit_wall_mid_y + 5*px)) {
                    
                    if ((hit_wall_mid_x <= laser.x_coord) && 
                            (laser.x_coord <= hit_wall_mid_x + width)) {
                        
                        xSemaphoreGive(laser.lock);
                        
                        if (xSemaphoreTake(bunkers[row].lock, 0)) {

                            if (bunkers[row].upp_coll_mid_y >= 
                                    bunkers[row].low_coll_mid_y + 4*px) {
                                xSemaphoreGive(bunkers[row].lock);

                                return 0;
                            }
                            else {
                                bunkers[row].upp_coll_mid_y += 4*px;
                            }
                            xSemaphoreGive(bunkers[row].lock);
                        }
                        return 1;
                    }
                }

                if ((hit_wall_right_y <= laser.y_coord) 
                    && (laser.y_coord <= hit_wall_right_y + 5*px)) {
                    
                    if ((hit_wall_right_x <= laser.x_coord) && 
                            (laser.x_coord <= hit_wall_right_x + width)) {
                            
                        xSemaphoreGive(laser.lock);

                        if (xSemaphoreTake(bunkers[row].lock, 0)) {

                            if (bunkers[row].upp_coll_right_y >= 
                                    bunkers[row].low_coll_right_y + 4*px) {
                                xSemaphoreGive(bunkers[row].lock);

                                return 0;
                            }
                            else {
                                bunkers[row].upp_coll_right_y += 4*px;
                            }
                            xSemaphoreGive(bunkers[row].lock);
                        }
                        return 1;
                    }
                }
                xSemaphoreGive(laser.lock);
            }
        }
    }
    return 0;
}

int vCheckCollision_laser_proj()
{
    signed short laser_x;
    signed short laser_y;

    signed short proj_x;
    signed short proj_y;

    if (xSemaphoreTake(laser.lock, 0)) {
        laser_x = laser.x_coord;
        laser_y = laser.y_coord + 2*px;

        xSemaphoreGive(laser.lock);
    }
    if (xSemaphoreTake(projectile.lock, 0)) {
        proj_x = projectile.x_coord;
        proj_y = projectile.y_coord;

        xSemaphoreGive(projectile.lock);
    }
    if (proj_x == laser_x) {
        if (proj_y <= laser_y && proj_y >= laser_y - 2*px) {
            return 1;
        }
    }
    return 0;
}

int vCheckCollision_laser_player() 
{
    signed short hit_wall_x;
    signed short hit_wall_y;
    unsigned short width = 13*px;

    signed short laser_x;
    signed short laser_y;

    if (xSemaphoreTake(player.lock, 0)) {

        hit_wall_x = player.x_coord;
        hit_wall_y = player.y_coord + px;

        xSemaphoreGive(player.lock);
    }
    if (xSemaphoreTake(laser.lock, 0)) {
        
        // x, 2px;
        
        laser_x = laser.x_coord;
        laser_y = laser.y_coord + 2*px;

        xSemaphoreGive(laser.lock);
    }
    if (laser_y >= hit_wall_y && laser_y <= hit_wall_y + 5*px) {
        if (laser_x >= hit_wall_x && laser_x <= hit_wall_x + width) {
            return 1;
        }
    }
    return 0;
}

int vCheckCollision_alien_player()
{
    for (int row=0; row < 5; row++) {
        for (int col=0; col < 10; col++) {
            if (xSemaphoreTake(aliens[row][col].lock, 0)) {

                if ((aliens[row][col].y_coord + 5*px) 
                        >= CENTER_Y + 175) {
                    xSemaphoreGive(aliens[row][col].lock);
                    
                    return 1;
                }
            }
            xSemaphoreGive(aliens[row][col].lock);
        }
    }
    return 0;
}

int vCheckCollision_proj_upper()
{
    if (projectile.y_coord <= 50) {
        return 1;
    }
    return 0;
}

int vCheckCollision_proj_mothership()
{
    signed short hit_wall_x = mothership.x_coord;
    signed short hit_wall_y = mothership.y_coord + 2*px;
    unsigned short width = 14*px;

    if (projectile.state) {

        if ((hit_wall_y >= projectile.y_coord) &&
                (projectile.y_coord <= hit_wall_y - 5*px)) {

            if ((hit_wall_x <= projectile.x_coord) &&
                    (projectile.x_coord <= hit_wall_x + width)) {

                
                vIncrease_score(mothership.type);
                
                mothership.state = 0;
                mothership.x_coord = 0;
                mothership.y_coord = 0;
                mothership.f_x = 0;
                mothership.f_y = 0;

                explosion.x_coord = projectile.x_coord - 3*px;
                explosion.y_coord = projectile.y_coord;

                explosion.state = 1;

                return 1;
            }
        }
    }
    return 0;
}

int vCheckCollision_proj_bunker()
{
    signed short hit_wall_left_x;
    signed short hit_wall_left_y;

    signed short hit_wall_mid_x;
    signed short hit_wall_mid_y;

    signed short hit_wall_right_x;
    signed short hit_wall_right_y;

    unsigned short width = 8*px;

    for (int row=0; row < 4; row++) {

        if (xSemaphoreTake(bunkers[row].lock, 0)) {
            
            hit_wall_left_x = bunkers[row].low_coll_left_x;
            hit_wall_left_y = bunkers[row].low_coll_left_y;

            hit_wall_mid_x = bunkers[row].low_coll_mid_x;
            hit_wall_mid_y = bunkers[row].low_coll_mid_y;

            hit_wall_right_x = bunkers[row].low_coll_right_x;
            hit_wall_right_y = bunkers[row].low_coll_right_y;

            xSemaphoreGive(bunkers[row].lock);
        }
        if (projectile.state) {

            if (xSemaphoreTake(projectile.lock, 0)) {
            
                if ((hit_wall_left_y >= projectile.y_coord)
                        && (projectile.y_coord >= hit_wall_left_y - 5*px)) {
                    
                    if ((hit_wall_left_x <= projectile.x_coord) 
                            && projectile.x_coord <= hit_wall_left_x + width) {
                        
                        xSemaphoreGive(projectile.lock);

                        if (xSemaphoreTake(bunkers[row].lock, 0)) {
                            
                            // delete hit wall when bunker is gone
                            if (bunkers[row].low_coll_left_y <=     
                                    bunkers[row].upp_coll_left_y - 4*px) {
                                
                                xSemaphoreGive(bunkers[row].lock);

                                return 0;
                            }
                            else {
                                bunkers[row].low_coll_left_y -= 4*px;
                            }

                            xSemaphoreGive(bunkers[row].lock);
                        }

                        return 1;
                    }
                }
                if ((hit_wall_mid_y >= projectile.y_coord)
                        && (projectile.y_coord >= hit_wall_mid_y - 5*px)) {
                    
                    if ((hit_wall_mid_x <= projectile.x_coord)
                            && projectile.x_coord <= hit_wall_mid_x + width) {
                        
                        xSemaphoreGive(projectile.lock);

                        if (xSemaphoreTake(bunkers[row].lock, 0)) {
                            
                            if (bunkers[row].low_coll_mid_y <= 
                                    bunkers[row].upp_coll_mid_y - 4*px) {
                                
                                xSemaphoreGive(bunkers[row].lock);

                                return 0;
                            }
                            else {
                                bunkers[row].low_coll_mid_y -= 4*px;
                            }

                            xSemaphoreGive(bunkers[row].lock);
                        }

                        return 1;
                    }
                }
                if ((hit_wall_right_y >= projectile.y_coord)
                        && (projectile.y_coord >= hit_wall_right_y - 5*px)) {
                    
                    if ((hit_wall_right_x <= projectile.x_coord)
                            && projectile.x_coord <= hit_wall_right_x + width) {
                        
                        xSemaphoreGive(projectile.lock);

                        if (xSemaphoreTake(bunkers[row].lock, 0)) {
                            
                            if (bunkers[row].low_coll_right_y <= 
                                    bunkers[row].upp_coll_right_y - 4*px) {
                                
                                xSemaphoreGive(bunkers[row].lock);

                                return 0;
                            }
                            else {
                                bunkers[row].low_coll_right_y -= 4*px;
                            }

                            xSemaphoreGive(bunkers[row].lock);
                        }

                        return 1;
                    }
                }

                xSemaphoreGive(projectile.lock);
            }
        }
        
    }
    return 0;
}

int vCheckCollision_proj_alien()
{
    signed short hit_wall_x;
    signed short hit_wall_y;
    unsigned short width;

    for (int row=0; row < 5; row++) {
        for (int col=0; col < 10; col++) {
            if (xSemaphoreTake(aliens[row][col].lock, 0)) {
                
                switch(aliens[row][col].type) {
                    case 'J':           // Jelly Alien 
                    hit_wall_x = aliens[row][col].x_coord;     
                    hit_wall_y = aliens[row][col].y_coord + 5*px;
                    width = 11*px;
                    break;
                case 'C':                // Crab Alien
                    hit_wall_x = aliens[row][col].x_coord - 2*px;
                    hit_wall_y = aliens[row][col].y_coord + 4*px;
                    width = 11*px;
                    break;
                case 'F':                // Fred Alien
                    hit_wall_x = aliens[row][col].x_coord;
                    hit_wall_y = aliens[row][col].y_coord + 5*px;
                    width = 12*px;
                    break;
                }
                if ((hit_wall_y >= projectile.y_coord)
                        && projectile.y_coord >= hit_wall_y - 5*px) {
                    if (projectile.x_coord >= hit_wall_x &&
                            projectile.x_coord <= hit_wall_x + width) {
                        // delete alien
                        aliens[row][col].state = 0;

                        aliens[row][col].f_x = 0;
                        aliens[row][col].f_y = 0;

                        aliens[row][col].x_coord = aliens[row][col].f_x;
                        aliens[row][col].y_coord = aliens[row][col].f_y;

                        // increase score
                        vIncrease_score(aliens[row][col].type);
                            
                        xSemaphoreGive(aliens[row][col].lock);

                        return 1;
                        
                    }
                }
            }
            xSemaphoreGive(aliens[row][col].lock);
        }
    }
    
    return 0;
}
/**
 * #####################################################
 * END
 */
/**
 * #####################################################
 * UPDATE POSITION FUNCTIONS
 */
void vUpdate_aliens(unsigned int ms) 
{
    unsigned int dy = alien_velo.dy;
    unsigned int dx = alien_velo.dx;
    float update_interval = ms / 1000.0;
    

    for (int row=0; row < 5; row++) {
        for (int col=0; col < 10; col++) {
            if (xSemaphoreTake(aliens[row][col].lock, portMAX_DELAY)) {

                
                if (aliens[row][col].x_coord <= 120 &&
                    aliens[row][col].state) {
                    alien_velo.move_right = 1;
                }
            
                if (aliens[row][col].x_coord >= 500 &&
                    aliens[row][col].state) {
                    alien_velo.move_right = 0;
                }
                
                xSemaphoreGive(aliens[row][col].lock);
            } 
        }
    }
    
    for (int row=0; row < 5; row++) {
        for (int col=0; col < 10; col++) {

            if (xSemaphoreTake(aliens[row][col].lock, portMAX_DELAY)) {
                aliens[row][col].f_y += dy * update_interval;
                aliens[row][col].y_coord = round(aliens[row][col].f_y);

                // move left
                if (!alien_velo.move_right) {
                    aliens[row][col].f_x -= dx*update_interval;
                    aliens[row][col].x_coord = round(aliens[row][col].f_x);
                    aliens[row][col].blink = 1;
                }
                // move right
                if (alien_velo.move_right) {
                    aliens[row][col].f_x += dx*update_interval;
                    aliens[row][col].x_coord = round(aliens[row][col].f_x);
                    aliens[row][col].blink = 0;
                }
                xSemaphoreGive(aliens[row][col].lock);
            }
        }
    }
}

void vUpdate_player(unsigned int move_left, 
                    unsigned int move_right, 
                    unsigned int ms)
{
    unsigned int dx = 150;
    float update_interval = ms / 1000.0;

    if (xSemaphoreTake(player.lock, 0)) {
        if (move_left) {
                // constrain left move with screen border
            if (player.x_coord > LEFT_CONSTRAINT_X + 1) {
                player.f_x -= dx * update_interval;
            }
        }
        if (move_right) {
                // constrain right move with screen border
            if (player.x_coord < 513) { 
                player.f_x += dx * update_interval;
            }
        }
        player.x_coord = round(player.f_x);
        xSemaphoreGive(player.lock);
    }
}

void vUpdate_mothership_sp(unsigned int ms)
{
    unsigned int dx = 100;
    float update_interval = ms / 1000.0;

    // move right until constraint 
    // move left until constraint 
    // delete
    if (mothership.blink) { // move right

        if (mothership.x_coord < RIGHT_CONSTRAINT_X - 30) {

            mothership.f_x += dx * update_interval;
            mothership.x_coord = round(mothership.f_x);
        }
        else {
            mothership.blink = 0;
        }
    }
    else {
        
        if (mothership.x_coord > LEFT_CONSTRAINT_X + 5) {
            
            mothership.f_x -= dx * update_interval;
            mothership.x_coord = round(mothership.f_x);
        
        }
        else {
            vDelete_mothership();
        }
    }
    
}

void vUpdate_mothership_mp(unsigned int ms)
{

    unsigned int dx = 150;
    float update_interval = ms / 1000.0;
    
    if (!mothership.blink) {
            // constrain left move with screen border
        if (mothership.x_coord > LEFT_CONSTRAINT_X + 50) {
            mothership.f_x -= dx * update_interval;
        }
    }
    if (mothership.blink) {
            // constrain right move with screen border
        if (mothership.x_coord < RIGHT_CONSTRAINT_X - 80) { 
            mothership.f_x += dx * update_interval;
        }
    }
    mothership.x_coord = round(mothership.f_x);
}

void vUpdate_projectile(unsigned int ms) 
{
    unsigned int dy = DY_PROJECTILE;
    float update_interval = ms / 1000.0;

    if (xSemaphoreTake(projectile.lock, 0)) {
        projectile.f_y -= dy * update_interval;
        projectile.y_coord = round(projectile.f_y);

        xSemaphoreGive(projectile.lock);
    }

}

void vUpdate_laser(unsigned int ms)
{
    unsigned int dy = DY_LASER;
    float update_interval = ms / 1000.0;

    if (xSemaphoreTake(laser.lock, 0))  {
        laser.f_y += dy * update_interval;
        laser.y_coord = round(laser.f_y);

        xSemaphoreGive(laser.lock);
    }
}

void vUpdate_explosion(unsigned int ms) 
{
    if (xSemaphoreTake(explosion.lock, 0)) {

        explosion.blink += ms;

        if (explosion.blink >= 300) {

            explosion.state = 0;

            explosion.blink = 0;
        }

        xSemaphoreGive(explosion.lock);
    }

}
/**
 * #####################################################
 * END
 */
/**
 * #####################################################
 * DRAW DYNAMIC ITEMS FUNCTIONS
 */
void vDrawScores()
{
    // coordinates of Gamescreen
    signed short x_playscreen = 100;
    signed short y_playscreen = 0;
    signed short w_playscreen = 440;
    signed short h_playscreen = 480;

    static char score1[50];

    static char AI_diff_str[50];
    static int AI_diff_strlen = 0;

    static char AI_toggle_str[50];
    static int AI_toggle_str_width = 0;

    static char AI_diff_val[50];
    static int AI_diff_val_width = 0;

    static char hscore[50];
    static int hscore_width = 0;

    static char lives[10];
    
    static char level[50];
    static int level_width = 0;

    if (xSemaphoreTake(gamedata.lock, 0)) {
        sprintf(score1, "%i", gamedata.score1);
        tumDrawText(score1, x_playscreen + 30, 
                    y_playscreen + 25,
                    Green);

        if (gamedata.cheats_set == 0) {
            sprintf(hscore, "%i", gamedata.hscore);
            tumGetTextSize((char *) hscore, 
                            &hscore_width, NULL);
            tumDrawText(hscore, CENTER_X - hscore_width / 2,
                        y_playscreen + 25,
                        Green);
        }

        if (gamedata.multiplayer) {
            sprintf(AI_diff_str, "AI-DIFFICULTY");
            tumGetTextSize((char *) AI_diff_str,
                            &AI_diff_strlen, NULL);
            tumDrawText(AI_diff_str,
                        (x_playscreen + w_playscreen - 
                        AI_diff_strlen - 20),
                        y_playscreen,
                        Red);
            
            sprintf(AI_toggle_str, "Toggle (C)");
            tumGetTextSize((char *) AI_toggle_str,
                            &AI_toggle_str_width, NULL);
            tumDrawText(AI_toggle_str, 
                        (x_playscreen + w_playscreen - 
                        AI_diff_strlen + 15),
                        y_playscreen + 25,
                        Red);

            sprintf(AI_diff_val, "D%i", gamedata.AI_diff);
            tumGetTextSize((char *) AI_diff_val, 
                            &AI_diff_val_width, NULL);
            tumDrawText(AI_diff_val, 
                        (x_playscreen + w_playscreen - 
                        AI_diff_strlen - 20),
                        y_playscreen + 25,
                        Red);
        }

        if (gamedata.lives == 1000) {   // infinite value
            sprintf(lives, "INF");
        }
        else {
            sprintf(lives, "%i", gamedata.lives);
        }
        tumDrawText(lives, x_playscreen + 20,
                    y_playscreen + h_playscreen - 30,
                    Green);
        // when INF val set don't draw player models
        if ((gamedata.lives > 0) && (gamedata.lives < 4)){
            for (int i=0; i < gamedata.lives-1; i++) {
                vDrawPlayer(x_playscreen + 50 + i*40,
                            y_playscreen + h_playscreen - 20);

            }
        }
        sprintf(level,"LEVEL  %i", gamedata.level);
        tumGetTextSize((char *) level,
                        &level_width, NULL);
        tumDrawText(level, (x_playscreen + w_playscreen 
                    - level_width - 30),
                    y_playscreen + h_playscreen - 30,
                    Green);

        xSemaphoreGive(gamedata.lock);
    }
}

void vDrawAliens()
{
    for (int row=0; row < 5; row++) {
        for (int col=0; col < 10; col++) {

            if (xSemaphoreTake(aliens[row][col].lock, portMAX_DELAY)) {
                if (aliens[row][col].state) {
                    switch (aliens[row][col].type) {
                        case 'J': 
                            vDraw_jellyAlien(aliens[row][col].x_coord,
                                             aliens[row][col].y_coord,
                                             aliens[row][col].blink);
                            break;
                        case 'C':
                            vDraw_crabAlien(aliens[row][col].x_coord, 
                                            aliens[row][col].y_coord,
                                            aliens[row][col].blink);
                            break;
                        case 'F':
                            vDraw_fredAlien(aliens[row][col].x_coord,
                                            aliens[row][col].y_coord,
                                            aliens[row][col].blink);
                            break;
                        default:
                            break;
                    } 
                }
                xSemaphoreGive(aliens[row][col].lock);
            }
        }
    }
}

void vDrawBunkers()
{
    for (int row=0; row<4; row++) {
        
        if (xSemaphoreTake(bunkers[row].lock, 0)) {

            vDrawBunker(bunkers[row].x_coord, bunkers[row].y_coord);
            tumDrawFilledBox(bunkers[row].x_coord, 
                                bunkers[row].low_coll_left_y,
                                8*px, 
                                (bunkers[row].y_coord + 13*px - 
                                bunkers[row].low_coll_left_y),
                                Black);
            tumDrawFilledBox(bunkers[row].x_coord + 8*px, 
                                bunkers[row].low_coll_mid_y,
                                8*px, 
                                (bunkers[row].y_coord + 13*px - 
                                bunkers[row].low_coll_mid_y),
                                Black);
            tumDrawFilledBox(bunkers[row].x_coord + 16*px, 
                                bunkers[row].low_coll_right_y,
                                8*px, 
                                (bunkers[row].y_coord + 13*px - 
                                bunkers[row].low_coll_right_y),
                                Black);

            tumDrawFilledBox(bunkers[row].x_coord, 
                                bunkers[row].y_coord - 4*px,
                                8*px, 
                                (bunkers[row].upp_coll_left_y - 
                                bunkers[row].y_coord),
                                Black);
            tumDrawFilledBox(bunkers[row].x_coord + 8*px, 
                                bunkers[row].y_coord - 4*px,
                                8*px, 
                                (bunkers[row].upp_coll_mid_y -  
                                bunkers[row].y_coord),
                                Black);
            tumDrawFilledBox(bunkers[row].x_coord + 16*px, 
                                bunkers[row].y_coord - 4*px,
                                8*px, 
                                (bunkers[row].upp_coll_right_y - 
                                bunkers[row].y_coord),
                                Black);            

            xSemaphoreGive(bunkers[row].lock);
        }
    }
}
/**
 * #####################################################
 * END
 */
/**
 * #####################################################
 * CREATE DELETE FUNCTIONS
 */
void vCreate_mothership() {
    
    mothership.x_coord = CENTER_X - 6*px;
    mothership.y_coord = CENTER_Y - 165;

    mothership.f_x = mothership.x_coord;
    mothership.f_y = mothership.y_coord;

    mothership.state = 1;

    mothership.blink = 1;
}

void vDelete_mothership() 
{
    
    mothership.x_coord = 0;
    mothership.y_coord = 0;

    mothership.f_x = 0;
    mothership.f_y = 0;

    mothership.state = 0;
}

void vCreate_projectile() {
    if (xSemaphoreTake(projectile.lock, 0)) {

        projectile.x_coord = player.x_coord + 6*px;
        projectile.y_coord = player.y_coord - 6*px;

        projectile.f_x = projectile.x_coord;
        projectile.f_y = projectile.y_coord;

        projectile.state = 1;

        xSemaphoreGive(projectile.lock);
    }
}

void vDelete_projectile() {
    if (xSemaphoreTake(projectile.lock, 0)) {

        projectile.x_coord = SCREEN_HEIGHT;
        projectile.y_coord = SCREEN_WIDTH;

        projectile.f_x = projectile.x_coord;
        projectile.f_y = projectile.y_coord;

        projectile.state = 0;

        xSemaphoreGive(projectile.lock);
    }
}

void vCreate_laser() {

    signed short x_origin = 0;
    signed short y_origin = 0;
    unsigned int break_it = 0;
    

    // hardcoded random arrays -> rand() doesn't gurantee all values to be checked
    static int random[6][10] = {
        {0,1,2,3,4,5,6,7,8,9}, 
        {9,8,7,6,5,4,3,2,1,0}, 
        {0,2,4,6,8,1,3,5,7,9},
        {1,3,5,7,9,0,2,4,6,8},
        {8,6,4,2,0,9,7,5,3,1}, 
        {9,7,5,3,1,8,6,4,2,0}
        };

    unsigned int random_num = rand() % 6;

    for (int row=4; row >= 0; row--) {  // check which alien is the bottom most

        for (int try=0; try < 10; try++) {

            if (xSemaphoreTake(aliens[row][random[random_num][try]].lock, 0)) {
             
                if (aliens[row][random[random_num][try]].state) {

                    x_origin = aliens[row][random[random_num][try]].x_coord + 6*px;
                    y_origin = aliens[row][random[random_num][try]].y_coord + 5*px;

                    break_it = 1;   // break when active alien found
                    break;
                }
                xSemaphoreGive(aliens[row][random[random_num][try]].lock);
            }
        }
        if (break_it) {
            break;
        }
    }
    // create laser
    if (xSemaphoreTake(laser.lock, 0)) {

        laser.x_coord = x_origin;
        laser.y_coord = y_origin;

        laser.f_x = laser.x_coord;
        laser.f_y = laser.y_coord;

        laser.state = 1;

        xSemaphoreGive(laser.lock);
    }
}

void vDelete_laser() {
    if (xSemaphoreTake(laser.lock, 0)) {

        laser.x_coord = 0;
        laser.y_coord = 0;

        laser.f_x = laser.x_coord;
        laser.f_y = laser.y_coord;

        laser.state = 0;

        xSemaphoreGive(laser.lock);
    }
}
/**
 * #####################################################
 * END
 */
/**
 * #####################################################
 * SCORE FUNCTIONS
 */

void vIncrease_score(char alien_type)
{   
    unsigned int increase;

    switch (alien_type) {
        case 'J': 
            increase = SCORE_JELLY;
            break;
        case 'C':
            increase = SCORE_CRAB;
            break;
        case 'F':
            increase = SCORE_FRED;
            break;
        case 'M':
            increase = SCORE_MOTHERSHIP;
            break;
        default:
            break;
    }
    if (xSemaphoreTake(gamedata.lock, 0)) {

        gamedata.score1 += increase;

        // every 1.5 levels worth of credits => new life
        // 1.5 levels = apprx. 1080 credits
        
        gamedata.score2 += increase;
        if (gamedata.score2 > SCORE_THRESHOLD) {
            gamedata.score2 = 0;
            if (gamedata.lives < 3) {
                gamedata.lives++;
            }
        }

        xSemaphoreGive(gamedata.lock);
    }
}

int vCheck_aliensleft() 
{
    for (int row=0; row < 5; row++) {
        for (int col=0; col < 10; col++) {
            
            if (xSemaphoreTake(aliens[row][col].lock, 0)) {
                
                if (aliens[row][col].state) {
                    xSemaphoreGive(aliens[row][col].lock);
                    return 0;
                }

                xSemaphoreGive(aliens[row][col].lock);
            }
        }
    }
    return 1;
}

void vDrawNextLevelScreen(unsigned int level)
{
    // coordinates of Gamescreen
    signed short x_hscorescreen = 100;
    signed short y_hscorescreen = 0;
    signed short w_hscorescreen = 440;
    signed short h_hscorescreen = 480;

    static char text[50];
    static int text_width = 0;

    // drawing Gamescreen
    tumDrawClear(White);
    tumDrawFilledBox(x_hscorescreen, y_hscorescreen,
                    w_hscorescreen, h_hscorescreen,
                    Black);

    sprintf(text, "LEVEL %i", level);

    tumGetTextSize((char *) text,
                    &text_width, NULL);
    tumDrawText(text,
                CENTER_X - text_width / 2,
                CENTER_Y,
                Green);
}

