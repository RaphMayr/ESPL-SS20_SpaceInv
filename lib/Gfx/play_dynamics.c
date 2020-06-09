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

Object player = { 0 };
Object projectile = { 0 };
Object upper_wall = { 0 };

Object jelly = { 0 };
Object crab = { 0 };
Object fred = { 0 };

void vInit_playscreen()
{
    upper_wall.lock = xSemaphoreCreateMutex();
    if (xSemaphoreTake(upper_wall.lock, 0)) {

        upper_wall.x_coord = 100;
        upper_wall.y_coord = 0;

        upper_wall.f_x = upper_wall.x_coord;
        upper_wall.f_y = upper_wall.y_coord;

        upper_wall.type = 'W';

        upper_wall.state = 1;

        xSemaphoreGive(upper_wall.lock);
    }
    player.lock = xSemaphoreCreateMutex();
    if (xSemaphoreTake(player.lock, 0)) {

        player.x_coord = CENTER_X - 6*px;
        player.y_coord = CENTER_Y + 175;

        player.f_x = player.x_coord;
        player.f_y = player.y_coord;

        player.state = 1;

        xSemaphoreGive(player.lock);
    }
    jelly.lock = xSemaphoreCreateMutex();
    if (xSemaphoreTake(jelly.lock, 0)) {

        jelly.x_coord = CENTER_X;
        jelly.y_coord = CENTER_Y - 100;

        jelly.f_x = jelly.x_coord;
        jelly.f_y = jelly.y_coord;

        jelly.type = 'J';

        jelly.state = 1;

        xSemaphoreGive(jelly.lock);
    }
    crab.lock = xSemaphoreCreateMutex();
    if (xSemaphoreTake(crab.lock, 0)) {
        
        crab.x_coord = CENTER_X + 50;
        crab.y_coord = CENTER_Y - 50;

        crab.f_x = crab.x_coord;
        crab.f_y = crab.y_coord;

        crab.type = 'C';

        crab.state = 1;

        xSemaphoreGive(crab.lock);
    }
    fred.lock = xSemaphoreCreateMutex();
    if (xSemaphoreTake(fred.lock, 0)) {
        
        fred.x_coord = CENTER_X - 50;
        fred.y_coord = CENTER_Y;

        fred.f_x = fred.x_coord;
        fred.f_y = fred.y_coord;

        fred.type = 'F';

        fred.state = 1;

        xSemaphoreGive(fred.lock);
    }
}

void vDraw_playscreen(unsigned int Flags[4], unsigned int ms)
{
    vUpdate_player(Flags[0], Flags[1], ms);
    // projectile is initialized when shoot flag is set and not active
    if (Flags[2] && (projectile.state == 0)) {  
        vCreate_projectile();
    }

    vDrawPlayScreen();
    vDrawPlayer(player.x_coord, player.y_coord);

    if (jelly.state) {
        vDraw_jellyAlien(jelly.x_coord, jelly.y_coord, Flags[3]);
        jelly = vUpdate_alien(jelly, Flags[3], ms);
    }
    if (crab.state) {
        vDraw_crabAlien(crab.x_coord, crab.y_coord, Flags[3]);
        crab = vUpdate_alien(crab, Flags[3], ms);
    }
    if (fred.state) {
        vDraw_fredAlien(fred.x_coord, fred.y_coord, Flags[3]);
        fred = vUpdate_alien(fred, Flags[3], ms);
    }

    if (projectile.state) {     // as long as no collisions occur
        vDrawProjectile(projectile.x_coord, projectile.y_coord);
        vUpdate_projectile(ms);
    }

    if (vCheckCollision(jelly)) {
        jelly = vDelete_alien(jelly);
        vDelete_projectile();
    }
    if (vCheckCollision(crab)) {
        crab = vDelete_alien(crab);
        vDelete_projectile();
    }
    if (vCheckCollision(fred)) {
        fred = vDelete_alien(fred);
        vDelete_projectile();
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
            hit_wall_y = alien.y_coord;
            width = 440;
        default: 
            break;
    }
    if (hit_wall_y >= projectile.y_coord) {
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

    if (xSemaphoreTake(alien.lock, 0)) {
        alien.state = 0;

        alien.f_x = 0;
        alien.f_y = 0;

        alien.x_coord = alien.f_x;
        alien.y_coord = alien.f_y;

        printf("deleted alien. state: %i\n", alien.state);

        xSemaphoreGive(alien.lock);
    }
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
    unsigned int dy = 200;
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
    unsigned int dy = 10;
    unsigned int dx = 20;
    float update_interval = ms / 1000.0;

    if (xSemaphoreTake(alien.lock, 0)) {
        
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
        xSemaphoreGive(alien.lock);
    }

    return alien;
}
