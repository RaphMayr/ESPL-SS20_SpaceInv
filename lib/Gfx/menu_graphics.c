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

#include "menu_graphics.h"

#define CENTER_X SCREEN_WIDTH/2
#define CENTER_Y SCREEN_HEIGHT/2

#define px 2

void vDrawMainMenu(unsigned short state) {
    // coordinates of Gamescreen
    signed short x_startscreen = 100;
    signed short y_startscreen = 0;
    signed short w_startscreen = 440;
    signed short h_startscreen = 480;

    // coordinates and size of welcometext
    signed short x_welcometext = CENTER_X;
    signed short y_welcometext = CENTER_Y - 150;
    static char welcometext[100];
    static int width_welcometext = 0;

    // coordinates and size of continutext
    signed short x_continutext = CENTER_X;
    signed short y_continutext = CENTER_Y - 50;
    static char continutext[100];
    static int width_continutext = 0; 
    
    // coordinates and size of Cheats Button
    // Button Box 
    signed short w_cheatsbutton = 40*px;
    signed short h_cheatsbutton = 15*px;
    signed short x_cheatsbutton = CENTER_X - w_cheatsbutton / 2;
    signed short y_cheatsbutton = CENTER_Y + 50;
    unsigned int color_cheatsbutton = TUMBlue;
    // Button Text
    static char cheatsBtn_str[50];
    static int width_cheatsBtn_str = 0;

    // coordinates and size of High-Score Button
    // Button Box
    signed short w_hscorebutton = 60*px;
    signed short h_hscorebutton = 15*px;
    signed short x_hscorebutton = CENTER_X - w_hscorebutton / 2;
    signed short y_hscorebutton = CENTER_Y + 100;
    unsigned int color_hscorebutton = TUMBlue;
    // Button Text
    static char hscoreBtn_str[50];
    static int width_hscoreBtn_str = 0;

    // quitting Text
    static char quit_str[50];
    static int width_quit_str = 0;

    // drawing Gamescreen
    tumDrawClear(White);
    tumDrawFilledBox(x_startscreen, y_startscreen,
                    w_startscreen, h_startscreen,
                    Black);

    // displaying welcome text 
    sprintf(welcometext, "Welcome to Space Invaders!"); 
    
    tumGetTextSize((char *) welcometext,
                    &width_welcometext, NULL);
    tumDrawText(welcometext,
                x_welcometext - width_welcometext / 2,
                y_welcometext,
                Green);

    // displaying text to enter game (alternating)
    sprintf(continutext, "Press (SPACE) to enter game");  

    if (state == 0) {
        tumGetTextSize((char *) continutext,
                        &width_continutext, NULL);
        tumDrawText(continutext,
                    x_continutext - width_continutext / 2,
                    y_continutext,
                    Green);
    }

    // displaying Cheatsbutton with text
    sprintf(cheatsBtn_str, "CHEATS");

    tumDrawFilledBox(x_cheatsbutton, y_cheatsbutton,
                        w_cheatsbutton, h_cheatsbutton,
                        color_cheatsbutton);
    tumGetTextSize((char *) cheatsBtn_str,
                    &width_cheatsBtn_str, NULL);
    tumDrawText(cheatsBtn_str, 
                CENTER_X - width_cheatsBtn_str / 2,
                CENTER_Y + 60 - DEFAULT_FONT_SIZE / 2,
                White);

    // displaying High-Score button with text 
    sprintf(hscoreBtn_str, "HIGH SCORES");

    tumDrawFilledBox(x_hscorebutton, y_hscorebutton,
                        w_hscorebutton, h_hscorebutton,
                        color_hscorebutton);
    tumGetTextSize((char *) hscoreBtn_str,
                    &width_hscoreBtn_str, NULL);
    tumDrawText(hscoreBtn_str, 
                CENTER_X - width_hscoreBtn_str / 2,
                CENTER_Y + 110 - DEFAULT_FONT_SIZE / 2,
                White);

    sprintf(quit_str, "(Q) to exit game");

    tumGetTextSize((char *) quit_str,
                    &width_quit_str, NULL);
    tumDrawText(quit_str,
                CENTER_X - width_quit_str / 2,
                CENTER_Y + 200, 
                Green);
}

void vCheckMainMenuButtonInput(signed short mouse_X, 
                                signed short mouse_Y) 
{  
    
    // check if cheats button was pressed
    if (mouse_X > CENTER_X - 40 &&
        mouse_X < CENTER_X + 40) {
                    
        if (mouse_Y > CENTER_Y + 50 && 
            mouse_Y < CENTER_Y + 80) {
                printf("CHEATS Btn pressed\n");
        }
    }
    // check if high-scores button was pressed
    if (mouse_X > CENTER_X - 60 &&
        mouse_X < CENTER_X + 60) {
                    
        if (mouse_Y > CENTER_Y + 100 && 
            mouse_Y < CENTER_Y + 130) {
                printf("High-Scores Btn pressed\n");
        }
    }
}

void vDrawPauseScreen()
{
    // coordinates of Gamescreen
    signed short x_pausescreen = 100;
    signed short y_pausescreen = 0;
    signed short w_pausescreen = 440;
    signed short h_pausescreen = 480;

    // coordinates of Pause figure
    signed short x_pausefigureleft = CENTER_X - 30;
    signed short x_pausefigureright = CENTER_X + 10;
    signed short y_pausefigure = CENTER_Y - 150;
    signed short w_pausefigure = 20;
    signed short h_pausefigure = 50;

    // paused String 
    static char pause_str[50];
    static int width_pause_str = 0;

    // main menu text
    static char mainmenu_str[50];
    static int width_mainmenu_str = 0;

    // continue text
    static char continu_str[50];
    static int width_continu_str = 0;

    // drawing Gamescreen
    tumDrawClear(White);
    tumDrawFilledBox(x_pausescreen, y_pausescreen,
                    w_pausescreen, h_pausescreen,
                    Black);

    // drawing Pause figure
    tumDrawBox(x_pausefigureleft, y_pausefigure,
                w_pausefigure, h_pausefigure,
                White);
    tumDrawBox(x_pausefigureright, y_pausefigure,
                w_pausefigure, h_pausefigure,
                White);

    // displaying Pause Text
    sprintf(pause_str, "PAUSED");
    tumGetTextSize((char *) pause_str,
                    &width_pause_str, NULL);
    tumDrawText(pause_str, 
                CENTER_X - width_pause_str / 2,
                CENTER_Y - 50,
                Green);

    // displaying Main Menu Text
    sprintf(mainmenu_str, "(ESC) MAIN MENU");
    tumGetTextSize((char *) mainmenu_str,
                    &width_mainmenu_str, NULL);
    tumDrawText(mainmenu_str, 
                CENTER_X - width_mainmenu_str / 2,
                CENTER_Y + 50,
                Green);
    
    // displaying Continue Text
    sprintf(continu_str, "(SPACE) CONTINUE");
    tumGetTextSize((char *) continu_str,
                    &width_continu_str, NULL);
    tumDrawText(continu_str, 
                CENTER_X - width_continu_str / 2,
                CENTER_Y + 100,
                Green);
    
}