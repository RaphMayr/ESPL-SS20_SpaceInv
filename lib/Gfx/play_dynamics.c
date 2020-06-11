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

#define DX_ALIEN 20     // initial x-velocity of aliens
#define DY_ALIEN 5     // initial y-velocity of aliens

#define DY_PROJECTILE 200   // y-velocity of projectile

Object player = { 0 };

Object projectile = { 0 };
Object lasershot = { 0 };

Object upper_wall = { 0 };
Object lower_wall = { 0 };

Bunker bunkers[4] = { 0 };

Object aliens[5][10] = { 0 };

Velocity alien_velo = { 0 };

void vInit_playscreen()
{
    // create mutexes
    for (int row=0; row < 5; row++){
        
        for (int col=0; col < 10; col++) {
        
            aliens[row][col].lock = xSemaphoreCreateMutex();
        }
    }
    alien_velo.lock = xSemaphoreCreateMutex();
    // set alien velocities
    if (xSemaphoreTake(alien_velo.lock, portMAX_DELAY)) {
        alien_velo.dx = DX_ALIEN;
        alien_velo.dy = DY_ALIEN;

        alien_velo.move_right = 1;

        xSemaphoreGive(alien_velo.lock);
    }

    upper_wall.lock = xSemaphoreCreateMutex();
    if (xSemaphoreTake(upper_wall.lock, portMAX_DELAY)) {

        upper_wall.x_coord = 100;
        upper_wall.y_coord = 0;

        upper_wall.f_x = upper_wall.x_coord;
        upper_wall.f_y = upper_wall.y_coord;

        upper_wall.type = 'U';

        upper_wall.state = 1;

        xSemaphoreGive(upper_wall.lock);
    }
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
    player.lock = xSemaphoreCreateMutex();
    if (xSemaphoreTake(player.lock, portMAX_DELAY)) {

        player.x_coord = CENTER_X - 6*px;
        player.y_coord = CENTER_Y + 175;

        player.f_x = player.x_coord;
        player.f_y = player.y_coord;

        player.state = 1;

        xSemaphoreGive(player.lock);
    }
    for (int row=0; row<4; row++) {
        bunkers[row].lock = xSemaphoreCreateMutex();
        if (xSemaphoreTake(bunkers[row].lock, portMAX_DELAY)) {

            bunkers[row].x_coord = 140 + 100*row;
            bunkers[row].y_coord = CENTER_Y + 130;

            bunkers[row].low_coll_x = bunkers[row].x_coord;
            bunkers[row].low_coll_y = bunkers[row].y_coord + 13*px;

            xSemaphoreGive(bunkers[row].lock);
        }
    }
}

int vDraw_playscreen(unsigned int Flags[5], unsigned int ms)
{
    Object alien;
    
    vDrawPlayScreen();

    // projectile is initialized when shoot flag is set and not active
    if (Flags[2] && (projectile.state == 0)) {  
        vCreate_projectile();
    }
    
    vUpdate_player(Flags[0], Flags[1], ms);
    vDrawPlayer(player.x_coord, player.y_coord);
    
    if (projectile.state) {     // as long as no collisions occur
        vUpdate_projectile(ms);
        vDrawProjectile(projectile.x_coord, projectile.y_coord);
    }
    

    for (int row=0; row < 5; row++) {
        
        for (int col=0; col < 10; col++) {

            if (xSemaphoreTake(aliens[row][col].lock, portMAX_DELAY)) {
                
                alien = aliens[row][col];
                
                if (Flags[4]) {         // reset signal 
                    alien = vReset_alien(alien, row, col);
                }
                if (Flags[4] == 0) {
                    alien = vUpdate_alien(alien, Flags[3], ms);
                }
                if (alien.state) {
                    switch (alien.type) {
                        case 'J': 
                            vDraw_jellyAlien(alien.x_coord,
                                            alien.y_coord, Flags[3]);
                            break;
                        case 'C':
                            vDraw_crabAlien(alien.x_coord, 
                                            alien.y_coord, Flags[3]);
                            break;
                        case 'F':
                            vDraw_fredAlien(alien.x_coord,
                                            alien.y_coord, Flags[3]);
                            break;
                    } 
                }
                /**
                 * check projectile collision with alien
                 * collision results with deleting projectile
                 * and deleting alien
                 */
                if (vCheck_ProjCollision(alien)) {
                    vDelete_projectile();
                    alien = vDelete_alien(alien);
                    // increase speed of all aliens
                    if (xSemaphoreTake(alien_velo.lock, 0)) {
                        
                        printf("increasing x_velocity\n");
                        alien_velo.dx += 5;

                        xSemaphoreGive(alien_velo.lock);
                    }
                }
                /**
                 * check bottom wall collision
                 * colliding results in GAME OVER
                 */
                if (vCheck_bottomCollision(alien)) {
                    vDrawGameOver();
                    
                    return 1;
                }
                aliens[row][col] = alien;
                xSemaphoreGive(aliens[row][col].lock);
            }
        }
    }
    for (int row=0; row < 4; row++) {
        vDrawBunker(bunkers[row].x_coord, bunkers[row].y_coord);
        vUpdate_bunker(row); 
    }
    if (vCheck_ProjCollision(upper_wall)) {
        vDelete_projectile();
    }
    return 0;
}

void vUpdate_bunker(unsigned int row)
{
    unsigned int width = 24*px;
    // impact line proj: proj_x + px, proj_y

    if (projectile.y_coord <= bunkers[row].low_coll_y &&
        projectile.y_coord >= bunkers[row].low_coll_y) {
            for (int i=0; i < width; i++) {
                if ((bunkers[row].low_coll_x + i) 
                        == projectile.x_coord) {
                    printf("collision on bunker %i\n", row);
                }
            }
    }
}


int vCheck_ProjCollision(Object alien)
{
    signed short hit_wall_x;
    signed short hit_wall_y;
    unsigned short width;

    switch(alien.type) {
        case 'J': 
            hit_wall_x = alien.x_coord;     // Jelly Alien 
            hit_wall_y = alien.y_coord + 5*px;
            width = 11*px;
            break;
        case 'C':                           // Crab Alien
            hit_wall_x = alien.x_coord - 2*px;
            hit_wall_y = alien.y_coord + 4*px;
            width = 11*px;
            break;
        case 'F':                           // Fred Alien
            hit_wall_x = alien.x_coord;
            hit_wall_y = alien.y_coord + 5*px;
            width = 12*px;
            break;
        case 'U':   // upper wall collision
            hit_wall_x = alien.x_coord;
            hit_wall_y = alien.y_coord + 5*px;
            width = 440;
            break;
        default: 
            break;
    }
    if ((hit_wall_y >= projectile.y_coord)
            && projectile.y_coord >= hit_wall_y - 5*px) {
        for (int i=0; i<width; i++) {
            if (hit_wall_x + i == projectile.x_coord) {
                return 1;
            }
        }
    }
    return 0;
}

void vUpdate_player(unsigned int move_left, 
                    unsigned int move_right, 
                    unsigned int ms)
{
    unsigned int dx = 100;
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

void vCreate_projectile() {
    projectile.lock = xSemaphoreCreateMutex();
    if (xSemaphoreTake(projectile.lock, 0)) {

        projectile.x_coord = player.x_coord + 6*px;
        projectile.y_coord = player.y_coord - 6*px;

        projectile.f_x = projectile.x_coord;
        projectile.f_y = projectile.y_coord;

        projectile.state = 1;

        xSemaphoreGive(projectile.lock);
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

void vDelete_projectile() {
    projectile.lock = xSemaphoreCreateMutex();
    if (xSemaphoreTake(projectile.lock, 0)) {

        projectile.x_coord = player.x_coord + 6*px;
        projectile.y_coord = player.y_coord - 6*px;

        projectile.f_x = projectile.x_coord;
        projectile.f_y = projectile.y_coord;

        projectile.state = 0;

        xSemaphoreGive(projectile.lock);
    }
}

Object vUpdate_alien(Object alien, unsigned int state,
                        unsigned int ms)
{
    unsigned int dy = alien_velo.dy;
    unsigned int dx = alien_velo.dx;
    float update_interval = ms / 1000.0;
    
    alien.f_y += dy * update_interval;
    alien.y_coord = round(alien.f_y);

    if (alien.x_coord >= 480 && alien.state == 1) {
        alien_velo.move_right = 0;
    }
    if (alien.x_coord <= 150 && alien.state == 1) {
        alien_velo.move_right = 1;
    }
    // move left
    if (!alien_velo.move_right) {
        alien.f_x -= dx*update_interval;
        alien.x_coord = round(alien.f_x);
    }
    // move right
    if (alien_velo.move_right) {
        alien.f_x += dx*update_interval;
        alien.x_coord = round(alien.f_x);
    }
    
    return alien;
}

Object vReset_alien(Object alien, int row, int col)
{
        alien.x_coord = 150 + col*30;
        alien.y_coord = CENTER_Y - 175 + row *40;

        alien.f_x = alien.x_coord;
        alien.f_y = alien.y_coord;

        if (row == 0) {
            alien.type = 'J';
        }
        if (row == 1 || row == 2) {
            alien.type = 'C';
        }
        if (row == 3 || row == 4) {
            alien.type = 'F';
        }

        alien.state = 1;

    return alien;
}

Object vDelete_alien(Object alien) {

    alien.state = 0;

    alien.f_x = 0;
    alien.f_y = 0;

    alien.x_coord = alien.f_x;
    alien.y_coord = alien.f_y;

    return alien;
}

int vCheck_bottomCollision(Object alien) {

    signed short hit_wall_y;
    hit_wall_y = alien.y_coord + 5*px;

    if (hit_wall_y >= lower_wall.y_coord) {
        return 1;
    }
    return 0;
}

