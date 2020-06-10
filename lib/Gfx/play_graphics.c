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

#include "play_graphics.h"

#define CENTER_X SCREEN_WIDTH/2
#define CENTER_Y SCREEN_HEIGHT/2

#define px 2


void vDrawPlayScreen(void) {
    signed short x_playscreen = 100;
    signed short y_playscreen = 0;
    signed short w_playscreen = 440;
    signed short h_playscreen = 480;

    tumDrawClear(White);
    tumDrawFilledBox(x_playscreen, y_playscreen, 
                w_playscreen, h_playscreen,
                Black);

    

    tumDrawLine(x_playscreen, h_playscreen - 30,
                x_playscreen + w_playscreen,
                h_playscreen - 30, 1, Green);
}

void vDrawPlayer(signed short pos_x, signed short pos_y) {

    unsigned int color = Green;

    tumDrawFilledBox(pos_x + 6*px, pos_y - 4*px,
                        px, px, color);
    tumDrawFilledBox(pos_x + 5*px, pos_y - 3*px, 
                        3*px, 2*px, color);
    tumDrawFilledBox(pos_x + px, pos_y - px, 11*px, px, color);
    tumDrawFilledBox(pos_x, pos_y, 13*px, 4*px, color);
}

void vDrawBunker(signed short pos_x, signed short pos_y) {
    
    unsigned int color = Green;

    tumDrawFilledBox(pos_x + 4*px, pos_y - 4 *px, 
                        16*px, px, color);                      // box 4

    tumDrawFilledBox(pos_x + 3*px, pos_y - 3*px, 
                        18*px, px, color);                      // box 3

    tumDrawFilledBox(pos_x + 2*px, pos_y - 2*px, 
                        20*px, px, color);                      // box 2

    tumDrawFilledBox(pos_x + px, pos_y - px, 
                        22*px, px, color);                      // box 1

    tumDrawFilledBox(pos_x, pos_y, 24*px, 9*px, color);        // box 0

    tumDrawFilledBox(pos_x, pos_y + 9*px, 7*px, px, color);    // box-1l
    tumDrawFilledBox(pos_x + 17*px, pos_y + 9*px, 
                        7*px, px, color);                       // box -1r

    tumDrawFilledBox(pos_x, pos_y + 10*px, 6*px, px, color);    // box -2l
    tumDrawFilledBox(pos_x + 18*px, pos_y + 10*px, 
                        6*px, px, color);                       // box -2r

    tumDrawFilledBox(pos_x, pos_y + 11*px, 5*px, 2*px, color);  // box -3l
    tumDrawFilledBox(pos_x + 19*px, pos_y + 11*px, 5*px, 2*px, color);
}

void vDrawAliens(signed short pos_x, signed short pos_y, 
                    signed short state)
{
    const unsigned short x_distance = 30;
    const unsigned short y_distance = 40;

    for (int i=0; i<9; i++) {
        vDraw_jellyAlien(pos_x + i*x_distance, pos_y, state);
    }

    for (int i=0; i<9; i++) {
        vDraw_crabAlien(pos_x + i*x_distance,  
                        pos_y + y_distance, state);
    }

    for (int i=0; i<9; i++) {
        vDraw_crabAlien(pos_x + i*x_distance,  
                        pos_y + 2*y_distance, state);
    }

    for (int i=0; i<9; i++) {
        vDraw_fredAlien(pos_x + i*35 - 20,
                        pos_y + 3*y_distance, state);
    }

    for (int i=0; i<9; i++) {
        vDraw_fredAlien(pos_x + i*35 - 20,
                        pos_y + 4*y_distance, state);
    }
    
}

void vDraw_fredAlien(signed short pos_x, signed short pos_y, 
                        signed short state)
{
    unsigned int primary_color = White;    
    unsigned int secondary_color = Black;

    tumDrawFilledBox(pos_x + 4*px, pos_y - 2*px, 
                        4*px, px, primary_color);           // box 2

    tumDrawFilledBox(pos_x + px, pos_y - px, 
                        10*px, px, primary_color);          // box 1

    tumDrawFilledBox(pos_x, pos_y, 12*px, 3*px, primary_color); // box 0

    tumDrawFilledBox(pos_x + 3*px, pos_y + px,                            
                        2*px, px, secondary_color);             // box -1l
    tumDrawFilledBox(pos_x + 7*px, pos_y + px, 
                        2*px, px, secondary_color);             // box -1r

    tumDrawFilledBox(pos_x + px, pos_y + 3*px,
                        10*px, 3*px, primary_color);       // box -2
    tumDrawFilledBox(pos_x + 5*px, pos_y + 3*px, 
                        2*px, px, secondary_color);        // box -2m

    tumDrawFilledBox(pos_x + px, pos_y + 3*px,            // side box left top
                        px, px, secondary_color);
    
    tumDrawFilledBox(pos_x + 10*px, pos_y + 3*px,         // side box right top
                        px, px, secondary_color);

    tumDrawFilledBox(pos_x + 3*px, pos_y + 4*px,          // middle sec. boxes
                        2*px, px, secondary_color);
    tumDrawFilledBox(pos_x + 7*px, pos_y + 4*px,
                        2*px, px, secondary_color);

    if (state == 0) {
        tumDrawFilledBox(pos_x + px, pos_y + 5*px,      // side box left bottom
                            px, px, secondary_color);

        tumDrawFilledBox(pos_x + 10*px, pos_y + 5*px,   // side box right
                            px, px, secondary_color);

        tumDrawFilledBox(pos_x + 4*px, pos_y + 5*px,      // middle sec. box bottom
                            4*px, px, secondary_color);                    
    }
    if (state == 1) {
        tumDrawFilledBox(pos_x, pos_y + 5*px,
                            2*px, px, primary_color);
        tumDrawFilledBox(pos_x + 10*px, pos_y + 5*px,
                            2*px, px, primary_color);
        
        tumDrawFilledBox(pos_x + 2*px, pos_y + 5*px,
                            8*px, px, secondary_color);
    }

}

void vDraw_crabAlien(signed short pos_x, signed short pos_y,
                        signed short state) 
{
    unsigned int primary_color = White;    
    unsigned int secondary_color = Black;

    tumDrawFilledBox(pos_x, pos_y, 
                    7*px, 4*px, primary_color);   // box 0

    tumDrawFilledBox(pos_x + px, pos_y + px,        // eyes
                        px, px, secondary_color);
    tumDrawFilledBox(pos_x + 5*px, pos_y + px,
                        px, px, secondary_color);

    tumDrawFilledBox(pos_x + px, pos_y - px,    // left ear
                    px, px, primary_color);
    tumDrawFilledBox(pos_x, pos_y - 2*px, 
                    px, px, primary_color);

    tumDrawFilledBox(pos_x + 5*px, pos_y - px,  // right ear
                    px, px, primary_color);
    tumDrawFilledBox(pos_x + 6*px, pos_y - 2*px, 
                    px, px, primary_color);

    tumDrawFilledBox(pos_x, pos_y + 4*px,       // left leg
                    px, px, primary_color);
    tumDrawFilledBox(pos_x + 6*px, pos_y + 4*px, // right leg
                    px, px, primary_color);

    if (state == 0) {

        tumDrawFilledBox(pos_x + px, pos_y + 5*px,  // left foot
                        2*px, px, primary_color);
        tumDrawFilledBox(pos_x + 4*px, pos_y + 5*px,    // right foot
                        2*px, px, primary_color);

        tumDrawFilledBox(pos_x - px, pos_y + px,        // left arm
                        px, 2*px, primary_color);
        tumDrawFilledBox(pos_x - 2*px, pos_y + 2*px,
                        px, 3*px, primary_color);

        tumDrawFilledBox(pos_x + 7*px, pos_y + px,      // right arm
                        px, 2*px, primary_color);
        tumDrawFilledBox(pos_x + 8*px, pos_y + 2*px,
                        px, 3*px, primary_color);

        
    }
    if (state == 1) {

        tumDrawFilledBox(pos_x - 1*px, pos_y + 5*px,    // left foot
                        px, px, primary_color);
        tumDrawFilledBox(pos_x + 7*px, pos_y + 5*px,    // right foot
                        px, px, primary_color);

        tumDrawFilledBox(pos_x - px, pos_y + px,
                        px, 3*px, primary_color);
        tumDrawFilledBox(pos_x - 2*px, pos_y - px,
                        px, 4*px, primary_color);

        tumDrawFilledBox(pos_x + 7*px, pos_y + px,
                        px, 3*px, primary_color);
        tumDrawFilledBox(pos_x + 8*px, pos_y - px,
                        px, 4*px, primary_color);

    }


}

void vDraw_jellyAlien(signed short pos_x, signed short pos_y,
                        signed short state)
{
    unsigned int primary_color = White;    
    unsigned int secondary_color = Black;

    tumDrawFilledBox(pos_x, pos_y, 8*px, 2*px, primary_color);
    tumDrawFilledBox(pos_x + 2*px, pos_y, px, px, secondary_color);
    tumDrawFilledBox(pos_x + 5*px, pos_y, px, px, secondary_color);

    tumDrawFilledBox(pos_x + px, pos_y - px, 
                    6*px, px, primary_color);
    tumDrawFilledBox(pos_x + 2*px, pos_y - 2*px, 
                    4*px, px, primary_color);
    tumDrawFilledBox(pos_x + 3*px, pos_y - 3*px, 
                    2*px, px, primary_color);

    if (state == 0) {
        tumDrawFilledBox(pos_x + 2*px, pos_y + 2*px,    // layer -1
                        px, px, primary_color);
        tumDrawFilledBox(pos_x + 5*px, pos_y + 2*px, 
                        px, px, primary_color);

        tumDrawFilledBox(pos_x + px, pos_y + 3*px,      // layer -2
                        px, px, primary_color);
        tumDrawFilledBox(pos_x + 3*px, pos_y + 3*px, 
                        2*px, px, primary_color);
        tumDrawFilledBox(pos_x + 6*px, pos_y + 3*px,
                        px, px, primary_color);

        tumDrawFilledBox(pos_x, pos_y + 4*px,           // layer -3
                        px, px, primary_color);
        tumDrawFilledBox(pos_x + 2*px, pos_y + 4*px,
                        px, px, primary_color);
        tumDrawFilledBox(pos_x + 5*px, pos_y + 4*px,
                        px, px, primary_color);
        tumDrawFilledBox(pos_x + 7*px, pos_y + 4*px,
                        px, px, primary_color);
        
        
    }
    if (state == 1) {
        tumDrawFilledBox(pos_x + px, pos_y + 2*px,      // layer -1
                        px, px, primary_color);
        tumDrawFilledBox(pos_x + 3*px, pos_y + 2*px,
                        2*px, px, primary_color);
        tumDrawFilledBox(pos_x + 6*px, pos_y + 2*px,
                        px, px, primary_color);

        tumDrawFilledBox(pos_x, pos_y + 3*px,    // layer -2
                        px, px, primary_color);
        tumDrawFilledBox(pos_x + 7*px, pos_y + 3*px,
                        px, px, primary_color);

        tumDrawFilledBox(pos_x + px, pos_y + 4*px,
                        px, px, primary_color);
        tumDrawFilledBox(pos_x + 6*px, pos_y + 4*px,
                        px, px, primary_color);
    }
}

void vDrawProjectile(signed short pos_x, signed short pos_y)
{   
    tumDrawFilledBox(pos_x, pos_y, px, 2*px, Green);
}
