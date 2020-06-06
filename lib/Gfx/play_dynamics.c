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

void vInit_playscreen()
{
    player.lock = xSemaphoreCreateMutex();
    if (xSemaphoreTake(player.lock, 0)) {

        player.x_coord = CENTER_X - 6*px;
        player.y_coord = CENTER_Y + 175;

        player.f_x = player.x_coord;
        player.f_y = player.y_coord;

        xSemaphoreGive(player.lock);
    }



}

void vInit_projectile()
{
    projectile.lock = xSemaphoreCreateMutex();
    if (xSemaphoreTake(projectile.lock, 0)) {

        projectile.x_coord = player.x_coord + 6*px;
        projectile.y_coord = player.y_coord - 6*px;

        projectile.f_x = projectile.x_coord;
        projectile.f_y = projectile.y_coord;

        xSemaphoreGive(projectile.lock);
    }

}

void vDraw_playscreen(unsigned int Flags[4], unsigned int ms)
{


    if (Flags[2]) {     // initialize projectile when W is pressed
        vInit_projectile();
    }

    vDrawPlayScreen();
    vDrawPlayer(player.x_coord, player.y_coord);

    if (Flags[3]) {     // as long as no collisions occur
        vDrawProjectile(projectile.x_coord, projectile.y_coord);
        vUpdate_projectile(ms);
    }

    vUpdate_player(Flags[0], Flags[1], ms);

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
    unsigned int dx = 100;
    float update_interval = ms / 1000.0;

    if (xSemaphoreTake(projectile.lock, 0)) {
        projectile.f_y -= dx * update_interval;
        projectile.y_coord = round(projectile.f_y);

        xSemaphoreGive(projectile.lock);
    }

}
