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

#define SCORE_JELLY 30
#define SCORE_CRAB 20
#define SCORE_FRED 10

#define DX_ALIEN 20     // initial x-velocity of aliens
#define DY_ALIEN 2    // initial y-velocity of aliens

#define DY_PROJECTILE 200   // y-velocity of projectile and laser

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


void vInit_playscreen(unsigned int inf_lives,
                      unsigned int score, unsigned int level)
{
    // initialize gamedata
    if (level == 1) {
        gamedata.score2 = 0;
        gamedata.hscore = 0;
        gamedata.score1 = 0;
    }
    if (inf_lives == 1) {
        // value doesn't get changed when 1000
        gamedata.lives = 1000;  // "INF" gets displayed
    }
    else {
        // normal case 3 lives
        gamedata.lives = 3;
    }
    // score normally set to 0 at beginning
    // with cheats can be set to custom value
    // not yet influence at the moment on velocity
    gamedata.credit = 0;
    gamedata.multiplayer = 0;
    gamedata.level = level;
    gamedata.lock = xSemaphoreCreateMutex();

    // initialize aliens
    for (int row=0; row < 5; row++) {
        for (int col=0; col < 10; col++) {
            aliens[row][col].lock = xSemaphoreCreateMutex();
            if (xSemaphoreTake(aliens[row][col].lock, portMAX_DELAY)) {
                

                if (row == 0) {
                    aliens[row][col].type = 'J';
                    aliens[row][col].x_coord = 151 + col*30;
                }
                if (row == 1 || row == 2) {
                    aliens[row][col].type = 'C';
                    aliens[row][col].x_coord = 149 + col*31;
                }
                if (row == 3 || row == 4) {
                    aliens[row][col].type = 'F';
                    aliens[row][col].x_coord = 132 + col*34;
                }

                aliens[row][col].y_coord = CENTER_Y - 130 + row *40;

                aliens[row][col].f_x = aliens[row][col].x_coord;
                aliens[row][col].f_y = aliens[row][col].y_coord;

                aliens[row][col].state = 1;
                xSemaphoreGive(aliens[row][col].lock);
            }
        }
    }
    
    // initialize alien velocities
    alien_velo.lock = xSemaphoreCreateMutex();
    if (xSemaphoreTake(alien_velo.lock, portMAX_DELAY)) {
        alien_velo.dx = DX_ALIEN + level*10;
        alien_velo.dy = DY_ALIEN;

        alien_velo.move_right = 1;

        xSemaphoreGive(alien_velo.lock);
    }

    // initialize upper wall
    upper_wall.lock = xSemaphoreCreateMutex();
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
    lower_wall.lock = xSemaphoreCreateMutex();
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
    player.lock = xSemaphoreCreateMutex();
    if (xSemaphoreTake(player.lock, portMAX_DELAY)) {

        player.x_coord = CENTER_X - 6*px;
        player.y_coord = CENTER_Y + 185;

        player.f_x = player.x_coord;
        player.f_y = player.y_coord;

        player.state = 1;

        xSemaphoreGive(player.lock);
    }

    // initialize mothership
    mothership.lock = xSemaphoreCreateMutex();
    if (xSemaphoreTake(mothership.lock, portMAX_DELAY)) {

        mothership.x_coord = CENTER_X - 6*px;
        mothership.y_coord = CENTER_Y - 175;

        mothership.f_x = mothership.x_coord;
        mothership.f_y = mothership.y_coord;

        mothership.state = 1;

        xSemaphoreGive(player.lock);
    }

    // initialize projectile
    projectile.lock = xSemaphoreCreateMutex();
    if (xSemaphoreTake(projectile.lock, portMAX_DELAY)) {

        projectile.x_coord = 0;
        projectile.y_coord = 0;

        projectile.f_x = projectile.x_coord;
        projectile.f_y = projectile.y_coord;

        projectile.state = 0;

        xSemaphoreGive(projectile.lock);
    }

    // initialize laser
    laser.lock = xSemaphoreCreateMutex();
    if (xSemaphoreTake(laser.lock, portMAX_DELAY)) {

        laser.x_coord = 0;
        laser.y_coord = 0;

        laser.f_x = laser.x_coord;
        laser.f_y = laser.y_coord;

        laser.state = 0;

        xSemaphoreGive(laser.lock);
    }
    // initialize explosion
    explosion.lock = xSemaphoreCreateMutex();
    if (xSemaphoreTake(explosion.lock, portMAX_DELAY)) {

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
        bunkers[row].lock = xSemaphoreCreateMutex();
        if (xSemaphoreTake(bunkers[row].lock, portMAX_DELAY)) {

            bunkers[row].x_coord = 145 + 100*row;
            bunkers[row].y_coord = CENTER_Y + 130;

            bunkers[row].low_coll_left_x = bunkers[row].x_coord;
            bunkers[row].low_coll_left_y = bunkers[row].y_coord + 13*px;
            bunkers[row].low_coll_left = 0;

            bunkers[row].low_coll_mid_x = bunkers[row].x_coord + 8*px;
            bunkers[row].low_coll_mid_y = bunkers[row].y_coord + 9*px;
            bunkers[row].low_coll_mid = 0;

            bunkers[row].low_coll_right_x = bunkers[row].x_coord + 16*px;
            bunkers[row].low_coll_right_y = bunkers[row].y_coord + 13*px;
            bunkers[row].low_coll_right = 0;

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

/**
 * vPlay();
 * 1. draw Static Items -
 * 2. check Collisions -
 * 3. update Positions -
 * 4. draw Dynamic Items -
 */
void vEmpty_aliens()
{
    for (int row=0; row < 5; row++) {
        for (int col=0; col < 10; col++) {

            aliens[row][col].state = 0;


        }
    }
}

int vDraw_playscreen(unsigned int Flags[4], unsigned int ms)
{
    signed int deltaX_player;
    signed int prevX_player;
    char bullet, state, difficulty;

    if (vCheck_aliensleft()) {     // when no aliens left progress to nxt lvl
        
        return 2;
    }

    vDrawStaticItems(); 

    // projectile is initialized when shoot flag is set and not active
    if (Flags[2] && (projectile.state == 0)) {  
        vCreate_projectile();
    }
    
    if (Flags[3]) {     // periodic timer from main function
        vCreate_laser();
    }
    

    // check collisions returns 1 when collision alien player occurs
    if (vCheckCollisions() || gamedata.lives == 0) {    
        vDrawGameOver();
        return 1;
    }
    if (xSemaphoreTake(player.lock, 0)) {

        prevX_player = player.x_coord;
        xSemaphoreGive(player.lock);
    }

    vUpdatePositions(Flags, ms);    // update positions

    if (xSemaphoreTake(player.lock, 0)) {

        deltaX_player = player.x_coord - prevX_player;
        xSemaphoreGive(player.lock);
    }
    if (xSemaphoreTake(projectile.lock, 0)) {

        if (projectile.state) {
            bullet = 'A';   // bullet is active
        }
        else {
            bullet = 'P';   // no bullet is active
        }
        xSemaphoreGive(projectile.lock);
    }
    // communicate with mothership
    // update mothership
    vUpdate_mothership(deltaX_player, 
                       &bullet, &state, &difficulty, ms);

    vDrawDynamicItems();        // draw dynamic items
    
    
    return 0;
}

int vCheckCollisions() 
{
    /**
     * 1. check collision projectile and alien -
     * 2. check collision projectile and bunker - 
     * 3. check collision projectile and upper line -
     * 4. check collision laser and player -
     * 5. check collision laser and projectile - 
     * 3. check collision laser and bunker -
     * 4. check collision laser and bottom line -
     * 5. check collision alien and player line -
     */

    
    if (vCheckCollision_proj_alien()) {
        
        // increase speed alien
        if (xSemaphoreTake(alien_velo.lock, 0)) {
            if (alien_velo.dx <= MAX_X_VELO) {
                alien_velo.dx += 5;
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

void vUpdatePositions(unsigned int Flags[4], unsigned int ms)
{
    /**
     * 1. update aliens positions
     * 2. update projectile position
     * 3. update laser position
     * 4. update player position
     */


    vUpdate_aliens(Flags, ms);

    
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

    vDrawMotherShip(CENTER_X - 7*px, CENTER_Y - 160);

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
                                bunkers[row].low_coll_left++;
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
                                bunkers[row].low_coll_mid++;
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
                                bunkers[row].low_coll_right++;
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
void vUpdate_aliens(unsigned int Flags[5], unsigned int ms) 
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

void vUpdate_mothership(signed int delta_X, char* bullet,
                        char* state, char* difficulty,
                        unsigned int ms)
{
   
    // communication with AI Binary
    // INPUT: delta_X, bullet, state, Difficulty
    // RETURN: move mothership left, right, or nothing

    char move;
    unsigned int dx = 150;
    float update_interval = ms / 1000.0;

    if (move == 'I') {      // increment mothership position
        if (xSemaphoreTake(mothership.lock, 0)) {
            projectile.f_x += dx * update_interval;
            projectile.y_coord = round(projectile.f_x);

            xSemaphoreGive(mothership.lock);
        }
    }
    if (move == 'D') {      // decrement mothership position
        if (xSemaphoreTake(mothership.lock, 0)) {
            projectile.f_x -= dx * update_interval;
            projectile.y_coord = round(projectile.f_x);

            xSemaphoreGive(mothership.lock);
        }
    }
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
    unsigned int dy = DY_PROJECTILE;
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

    static char score2[50];
    static int score2_width = 0;

    static char hscore[50];
    static int hscore_width = 0;

    static char lives[10];
    
    static char credit[50];
    static int credit_width = 0;

    if (xSemaphoreTake(gamedata.lock, 0)) {
        sprintf(score1, "%i", gamedata.score1);
        tumDrawText(score1, x_playscreen + 30, 
                    y_playscreen + 25,
                    Green);

        sprintf(hscore, "%i", gamedata.hscore);
        tumGetTextSize((char *) hscore, 
                        &hscore_width, NULL);
        tumDrawText(hscore, CENTER_X - hscore_width / 2,
                    y_playscreen + 25,
                    Green);

        if (gamedata.multiplayer) {
            sprintf(score2, "%i", gamedata.score2);
            tumGetTextSize((char *) score2, 
                            &score2_width, NULL);
            tumDrawText(score2, w_playscreen - score2_width - 30,
                        y_playscreen + 25,
                        Green);
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
        sprintf(credit,"CREDIT  %i", gamedata.credit);
        tumGetTextSize((char *) credit,
                        &credit_width, NULL);
        tumDrawText(credit, (x_playscreen + w_playscreen 
                    - credit_width - 30),
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
    unsigned int col;
    

    for (int row=4; row >= 0; row--) {  // check which alien is the bottom most
        unsigned int counter = 0;
        
        while(counter < 10) {   // constraint for while loop max. col checks are 10
            col = (rand() % 9);     // select random column
            if (xSemaphoreTake(aliens[row][col].lock, 0)) {
                if (aliens[row][col].state) {   // check if random chosen alien is active
                    // set coordinates for laser origin
                    x_origin = aliens[row][col].x_coord + 6*px;
                    y_origin = aliens[row][col].y_coord + 5*px;

                    break_it = 1;   // break when active alien found
                }
                xSemaphoreGive(aliens[row][col].lock);
            }
            if (break_it) {
                break;
            }
            counter++;
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
        default:
            break;
    }
    if (xSemaphoreTake(gamedata.lock, 0)) {

        gamedata.score1 += increase;

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

