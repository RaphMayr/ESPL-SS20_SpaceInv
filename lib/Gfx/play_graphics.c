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


void vDrawStaticItems()
{
    // coordinates of Gamescreen
    signed short x_playscreen = 100;
    signed short y_playscreen = 0;
    signed short w_playscreen = 440;
    signed short h_playscreen = 480;

    static char score1[50];
    static int score1_width = 0;

    static char hscore[50];
    static int hscore_width = 0;

    // drawing Gamescreen
    tumDrawClear(White);
    tumDrawFilledBox(x_playscreen, y_playscreen,
                     w_playscreen, h_playscreen,
                     Black);

    tumDrawLine(x_playscreen, h_playscreen - 30,
                x_playscreen + w_playscreen,
                h_playscreen - 30, 1, Green);
    
    tumDrawLine(x_playscreen, 50,
                x_playscreen + w_playscreen,
                50, 1, Green);

    sprintf(score1, "SCORE<1>");
    tumGetTextSize((char *) score1,
                    &score1_width, NULL);
    tumDrawText(score1,
                x_playscreen + 20,
                y_playscreen,
                Green);

    sprintf(hscore, "HI-SCORE");
    tumGetTextSize((char *) hscore,
                    &hscore_width, NULL);
    tumDrawText(hscore,
                CENTER_X - hscore_width / 2,
                y_playscreen,
                Green);

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

void vDrawMotherShip(signed short pos_x, signed short pos_y)
{
    unsigned int primary_color = Red;    
    unsigned int secondary_color = Black;

    tumDrawFilledBox(pos_x, pos_y - px, 
                    14*px, px, primary_color);
    tumDrawFilledBox(pos_x + px, pos_y - 2*px,
                    12*px, px, primary_color);
    tumDrawFilledBox(pos_x + 2*px, pos_y - 3*px, 
                    10*px, px, primary_color);
    tumDrawFilledBox(pos_x + 3*px, pos_y - 4*px,
                    8*px, px, primary_color);
    tumDrawFilledBox(pos_x + 4*px, pos_y - 5*px,
                    6*px, px, primary_color);

    tumDrawFilledBox(pos_x + 2*px, pos_y, 
                    3*px, px, primary_color);
    tumDrawFilledBox(pos_x + 6*px, pos_y,
                    2*px, px, primary_color);
    tumDrawFilledBox(pos_x + 9*px, pos_y,
                    3*px, px, primary_color);
    
    tumDrawFilledBox(pos_x + 3*px, pos_y + px,
                    px, px, primary_color);
    tumDrawFilledBox(pos_x + 10*px, pos_y + px,
                    px, px, primary_color);

    // draw windows of mothership
    tumDrawFilledBox(pos_x + 3*px, pos_y - 2*px,    
                    px, px, secondary_color);
    tumDrawFilledBox(pos_x + 5*px, pos_y - 2*px,
                    px, px, secondary_color);
    tumDrawFilledBox(pos_x + 8*px, pos_y - 2*px,
                    px, px, secondary_color);
    tumDrawFilledBox(pos_x + 10*px, pos_y - 2*px,
                    px, px, secondary_color); 
}

void vDrawExplosion(signed short pos_x, signed short pos_y) {

    unsigned int primary_color = Orange;
    unsigned int secondary_color = Red;

    // row 0
    tumDrawFilledBox(pos_x, pos_y, px, px, primary_color);
    
    tumDrawFilledBox(pos_x + 2*px, pos_y,     
                     px, px, primary_color);
    tumDrawFilledBox(pos_x + 4*px, pos_y,     
                     px, px, primary_color);
    tumDrawFilledBox(pos_x + 6*px, pos_y,     
                     px, px, primary_color);

    tumDrawFilledBox(pos_x + px, pos_y,     
                     px, px, secondary_color);
    tumDrawFilledBox(pos_x + 3*px, pos_y,     
                     px, px, secondary_color);
    tumDrawFilledBox(pos_x + 5*px, pos_y,     
                     px, px, secondary_color);
    tumDrawFilledBox(pos_x + 7*px, pos_y,     
                     px, px, secondary_color);
    
    // row 1
    tumDrawFilledBox(pos_x + px, pos_y - px,     
                     px, px, primary_color);
    tumDrawFilledBox(pos_x + 3*px, pos_y - px,     
                     px, px, primary_color);
    tumDrawFilledBox(pos_x + 4*px, pos_y - px,     
                     px, px, primary_color);
    tumDrawFilledBox(pos_x + 6*px, pos_y - px,     
                     px, px, primary_color);
    
    tumDrawFilledBox(pos_x + 2*px, pos_y - px,     
                     px, px, secondary_color);
    tumDrawFilledBox(pos_x + 5*px, pos_y - px,     
                     px, px, secondary_color);
    
    // row 2
    tumDrawFilledBox(pos_x + 3*px, pos_y - 2*px,     
                     px, px, primary_color);
    tumDrawFilledBox(pos_x + 5*px, pos_y - 2*px,     
                     px, px, primary_color);
    
    tumDrawFilledBox(pos_x + 4*px, pos_y - 2*px,     
                     px, px, secondary_color);

    // row -1
    tumDrawFilledBox(pos_x + px, pos_y + px,     
                     px, px, primary_color);
    tumDrawFilledBox(pos_x + 3*px, pos_y + px,     
                     px, px, primary_color);
    tumDrawFilledBox(pos_x + 4*px, pos_y + px,     
                     px, px, primary_color);
    tumDrawFilledBox(pos_x + 6*px, pos_y + px,     
                     px, px, primary_color);
    
    tumDrawFilledBox(pos_x + 2*px, pos_y + px,     
                     px, px, secondary_color);
    tumDrawFilledBox(pos_x + 5*px, pos_y + px,     
                     px, px, secondary_color);

    // row -2
    tumDrawFilledBox(pos_x + 3*px, pos_y + 2*px,     
                     px, px, primary_color);
    tumDrawFilledBox(pos_x + 5*px, pos_y + 2*px,     
                     px, px, primary_color);
    
    tumDrawFilledBox(pos_x + 4*px, pos_y + 2*px,     
                     px, px, secondary_color);
}
