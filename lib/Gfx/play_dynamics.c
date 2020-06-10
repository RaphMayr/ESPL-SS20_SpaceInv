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

#define CENTER_X SCREEN_WIDTH / 2
#define CENTER_Y SCREEN_HEIGHT / 2

#define LEFT_CONSTRAINT_X 100
#define RIGHT_CONSTRAINT_X 540

#define DY_ALIEN 5
#define DX_ALIEN 20

#define DY_PROJECTILE 300

Object player = { 0 };
Object projectile = { 0 };
Object upper_wall = { 0 };

Object aliens[5][10] = { 0 };

Object vReset_alien(Object alien, int row, int col)
{
        alien.x_coord = 175 + col*30;
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

void vInit_playscreen()
{
    for (int row=0; row < 5; row++){
        
        for (int col=0; col < 10; col++) {
        
            aliens[row][col].lock = xSemaphoreCreateMutex();
        }
    }

    upper_wall.lock = xSemaphoreCreateMutex();
    if (xSemaphoreTake(upper_wall.lock, portMAX_DELAY)) {

        upper_wall.x_coord = 100;
        upper_wall.y_coord = 0;

        upper_wall.f_x = upper_wall.x_coord;
        upper_wall.f_y = upper_wall.y_coord;

        upper_wall.type = 'W';

        upper_wall.state = 1;

        xSemaphoreGive(upper_wall.lock);
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
}


void vDraw_playscreen(unsigned int Flags[5], unsigned int ms)
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

    for (int row=0; row < 5; row++){
        
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
                if (vCheckCollision(alien)) {
                    vDelete_projectile();
                    alien = vDelete_alien(alien);
                }
                aliens[row][col] = alien;
                xSemaphoreGive(aliens[row][col].lock);
            }
        }

    }
    if (vCheckCollision(upper_wall)) {
        vDelete_projectile();
    }
}


int vCheckCollision(Object alien)
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
        case 'W':
            hit_wall_x = alien.x_coord;
            hit_wall_y = alien.y_coord + 5*px;
            width = 440;
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

Object vDelete_alien(Object alien) {

    alien.state = 0;

    alien.f_x = 0;
    alien.f_y = 0;

    alien.x_coord = alien.f_x;
    alien.y_coord = alien.f_y;

    printf("deleted alien. state: %i\n", alien.state);

    return alien;
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

Object vUpdate_alien(Object alien, unsigned int state,
                        unsigned int ms)
{
    unsigned int dy = DY_ALIEN;
    unsigned int dx = DX_ALIEN;
    float update_interval = ms / 1000.0;

    alien.f_y += dy * update_interval;
    alien.y_coord = round(alien.f_y);
    // move left
    if (state == 0) {
        alien.f_x -= dx*update_interval;
        alien.x_coord = round(alien.f_x);
    }
    // move right
    if (state == 1) {
        alien.f_x += dx*update_interval;
        alien.x_coord = round(alien.f_x);
    }

    return alien;
}

