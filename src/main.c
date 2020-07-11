/**
 * @file main.c
 * @author Raphael Mayr
 * @date 11 July 2020
 * @brief main file for Space Invaders game
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

#include "AsyncIO.h"

#include "play_graphics.h"
#include "menu_graphics.h"
#include "play_dynamics.h"

#define mainGENERIC_PRIORITY (tskIDLE_PRIORITY)
#define mainGENERIC_STACK_SIZE ((unsigned short)2560)

#define KEYCODE(CHAR) SDL_SCANCODE_##CHAR

#define CENTER_X SCREEN_WIDTH/2
#define CENTER_Y SCREEN_HEIGHT/2

#define UDP_BUFFER_SIZE 1000
#define UDP_RECEIVE_PORT 1234
#define UDP_TRANSMIT_PORT 1235

aIO_handle_t udp_soc_one = NULL;
aIO_handle_t udp_soc_two = NULL;

static TaskHandle_t startscreen_task = NULL;
static TaskHandle_t playscreen_task = NULL;
static TaskHandle_t pausescreen_task = NULL;
static TaskHandle_t cheatview_task = NULL;
static TaskHandle_t bufferswap = NULL;
static TaskHandle_t statemachine = NULL;
static TaskHandle_t send_task = NULL;
static TaskHandle_t receive_task = NULL;

static SemaphoreHandle_t DrawSignal = NULL;
static SemaphoreHandle_t ScreenLock = NULL;
static SemaphoreHandle_t state_machine_signal = NULL;

static QueueHandle_t next_state_queue = NULL;
static QueueHandle_t reset_queue = NULL;
static QueueHandle_t cheats_queue = NULL;
static QueueHandle_t nextLvl_queue = NULL;
static QueueHandle_t multipl_queue = NULL;

typedef struct to_AI_data {
    char delta_x[30];
    char attacking[30];
    char pause[30];
    char difficulty[10];
    SemaphoreHandle_t lock;
} to_AI_data_t;

static to_AI_data_t to_AI = { 0 };

typedef struct from_AI_data {
    char move[30];

    SemaphoreHandle_t lock;
} from_AI_data_t;

static from_AI_data_t from_AI = { 0 };

typedef struct buttons_buffer {
    unsigned char buttons[SDL_NUM_SCANCODES];
    SemaphoreHandle_t lock;
} buttons_buffer_t;

static buttons_buffer_t buttons = { 0 };


void xGetButtonInput(void)
{
    if (xSemaphoreTake(buttons.lock, 0) == pdTRUE) {
        xQueueReceive(buttonInputQueue, &buttons.buttons, 0);
        xSemaphoreGive(buttons.lock);
    }
}  

void checkDraw(unsigned char status, const char *msg)
{
    if (status) {
        if (msg)
            fprintf(stderr, "[ERROR] %s, %s\n", msg,
                    tumGetErrorMessage());
        else {
            fprintf(stderr, "[ERROR] %s\n", tumGetErrorMessage());
        }
    }
}

#define FPS_AVERAGE_COUNT 50
#define FPS_FONT "IBMPlexSans-Bold.ttf"

void vDrawFPS(void)
{
    static unsigned int periods[FPS_AVERAGE_COUNT] = { 0 };
    static unsigned int periods_total = 0;
    static unsigned int index = 0;
    static unsigned int average_count = 0;
    static TickType_t xLastWakeTime = 0, prevWakeTime = 0;
    static char str[10] = { 0 };
    static int text_width;
    int fps = 0;
    font_handle_t cur_font = tumFontGetCurFontHandle();

    xLastWakeTime = xTaskGetTickCount();

    if (prevWakeTime != xLastWakeTime) {
        periods[index] =
            configTICK_RATE_HZ / (xLastWakeTime - prevWakeTime);
        prevWakeTime = xLastWakeTime;
    }
    else {
        periods[index] = 0;
    }

    periods_total += periods[index];

    if (index == (FPS_AVERAGE_COUNT - 1)) {
        index = 0;
    }
    else {
        index++;
    }

    if (average_count < FPS_AVERAGE_COUNT) {
        average_count++;
    }
    else {
        periods_total -= periods[index];
    }

    fps = periods_total / average_count;

    tumFontSelectFontFromName(FPS_FONT);

    sprintf(str, "FPS: %2d", fps);

    if (!tumGetTextSize((char *)str, &text_width, NULL))
        checkDraw(tumDrawText(str, SCREEN_WIDTH - text_width - 10,
                              SCREEN_HEIGHT - DEFAULT_FONT_SIZE * 1.5,
                              Skyblue),
                  __FUNCTION__);

    tumFontSelectFontFromHandle(cur_font);
    tumFontPutFontHandle(cur_font);
}

void vSwapBuffers(void *pvParameters)
{
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();
    const TickType_t frameratePeriod = 15;

    tumDrawBindThread();

    while (1) {
        if (xSemaphoreTake(ScreenLock, portMAX_DELAY) == pdTRUE) {
            tumDrawUpdateScreen();
            tumEventFetchEvents(FETCH_EVENT_BLOCK);
            xSemaphoreGive(ScreenLock);
            xSemaphoreGive(DrawSignal);
            vTaskDelayUntil(&xLastWakeTime,
                            pdMS_TO_TICKS(frameratePeriod));
        }
    }
}


// TASKS #########################################################################

void vStart_screen(void *pvParameters) {

    unsigned short state = 0;
    int ticks = 0;

    signed short mouse_X = 0;
    signed short mouse_Y = 0;

    int buttonState_Mouse = 0;
    int lastState_Mouse = 0;
    clock_t lastDebounceTime_Mouse;

    clock_t timestamp;
    double debounce_delay = 0.025;

    int next_state_play = 1;
    int next_state_cheats = 3;

    int multiplayer = 1;
    char highscore[10];

    int hscore = 0;

    int button_pressed = 0;

    while(1) {
        if (DrawSignal) {
            if (xSemaphoreTake(DrawSignal, portMAX_DELAY) ==
                pdTRUE) {
                xGetButtonInput(); // Update global input
                // currently in state 0
                /* when SPACE is pressed send signal 
                    to state machine to go to state 1
                    -> enter game 
                */
                if (xSemaphoreTake(buttons.lock, 0) == pdTRUE) { 
                    if(buttons.buttons[KEYCODE(SPACE)]) {   // start game
                        xSemaphoreGive(buttons.lock);
                        // read h-score from file
                        FILE *fp;
                        fp = fopen("../src/hscore.txt", "r");
                        while (fgets(highscore, 10, fp) != NULL) {
                        }
                        fclose(fp);

                        hscore = strtol(highscore, NULL, 0);

                        // give hscore to play_dynamics.c
                        vGive_highScore(hscore); 

                        // check if binary is running when choosing multiplayer
                        if (multiplayer) {
                            if (!system(
                                "pidof -x space_invaders_opponent > /dev/null")) {
                                // send multiplayer value to state machine
                                xQueueSendToFront(multipl_queue, &multiplayer, 0);
                                // wake up state machine
                                xSemaphoreGive(state_machine_signal);
                                // initiiate state change
                                xQueueSend(next_state_queue, &next_state_play, 0);
                            }
                                
                        }
                        else {
                            // singleplayer -> no opponent binary reqired
                            xQueueSendToFront(multipl_queue, &multiplayer, 0);
                            xSemaphoreGive(state_machine_signal);
                            xQueueSend(next_state_queue, &next_state_play, 0);
                        }

                    }
                    xSemaphoreGive(buttons.lock);
                }
                // check button inputs
                // get mouse coordinates 
                mouse_X = tumEventGetMouseX();
                mouse_Y = tumEventGetMouseY();

                // debounce structure for mouse button
                int reading_Mouse = tumEventGetMouseLeft();

                if (reading_Mouse != lastState_Mouse) {
                    lastDebounceTime_Mouse = clock();
                }
                timestamp = clock();
                if((((double) (timestamp - lastDebounceTime_Mouse)
                        )/ CLOCKS_PER_SEC) > debounce_delay){
                    if (reading_Mouse != buttonState_Mouse){
                        buttonState_Mouse = reading_Mouse;
                         
                        if (buttonState_Mouse == 1){
                            // check button clicks
                            button_pressed = vCheckMainMenuButtonInput(
                                                        mouse_X, 
                                                        mouse_Y);
                            if (button_pressed == 1) {
                                // cheat button pressed
                                // change to cheat view
                                xSemaphoreGive(state_machine_signal);
                                xQueueSend(next_state_queue, 
                                            &next_state_cheats, 0);
                            }
                            if (button_pressed == 2) {
                                // multiplayer button pressed
                                // enable/disable multiplayer
                                if (multiplayer == 1) {
                                    multiplayer = 0;
                                }
                                else {
                                    multiplayer = 1;
                                }
                            }
                        }
                    }
                }
                lastState_Mouse = reading_Mouse;

                xSemaphoreTake(ScreenLock, portMAX_DELAY);

                vDrawMainMenu(state, multiplayer);
                
                vDrawFPS();

                xSemaphoreGive(ScreenLock);

                // counter structure for blinking font
                if (ticks == 50)    {
                    if (state == 1)   {
                        state = 0;
                    }   
                    else {
                        state = 1;
                    }
                    ticks = 0;
                }
                ticks++;

            } 
        } 
    }
}

void vPlay_screen(void *pvParameters)
{
    int next_state_pause = 2;
    int next_state_play = 1;
    int next_state_mainmenu = 0;

    int ticks_laser = 0;
    int ticks_mot = 0;

    int delta_X = 0;
    int active = 0;
    int difficulty = 0;

    unsigned int game_over = 0;

    int buttonState_W = 0;
    int lastState_W = 0;
    clock_t lastDebounceTime_W;

    int buttonState_C = 0;
    int lastState_C = 0;
    clock_t lastDebounceTime_C;

    clock_t timestamp;
    double debounce_delay = 0.025;

    int reset = 0;
    /** 
     * Signals for play_dynamics.c;
     * Flag 0: move left; Flag 1: move right
     * Flag 2: shoot; Flag 3: trigger laser shot
     * Flag 4: toggle difficulty
     * Flag 5: trigger Mothership flythrough
     */
    unsigned int Flags[6] = { 0 };

    TickType_t xLastWakeTime, prevWakeTime;
    xLastWakeTime = xTaskGetTickCount();
    prevWakeTime = xLastWakeTime;

    while (1) {
        if (DrawSignal) {
            if (xSemaphoreTake(DrawSignal, portMAX_DELAY) ==
                pdTRUE) {
                xLastWakeTime = xTaskGetTickCount();

                // reset timedelta
                // when resuming game or changing level
                xQueueReceive(reset_queue, &reset, 0);
                if (reset || game_over == 2) {
                    prevWakeTime = xLastWakeTime;
                    reset = 0;
                }
                
                xGetButtonInput(); // Update global input
                 
                if (xSemaphoreTake(buttons.lock, 0) == pdTRUE) {

                    if (buttons.buttons[KEYCODE(ESCAPE)]) {
                        xSemaphoreGive(buttons.lock);
                        // change to PAUSE screen
                        xSemaphoreGive(state_machine_signal);
                        xQueueSend(next_state_queue, &next_state_pause, 0);
                    }
                    if (buttons.buttons[KEYCODE(A)]) {
                        // signal to move left
                        Flags[0] = 1;
                    }

                    if (buttons.buttons[KEYCODE(D)]) {
                        // signal to move right
                        Flags[1] = 1;
                    }

                    // debounce structure for W
                    int reading_W = buttons.buttons[KEYCODE(W)];
                    if (reading_W != lastState_W) {
                        lastDebounceTime_W = clock();
                    }
                    timestamp = clock();
                    if ((((double) (timestamp - lastDebounceTime_W)
                            )/ CLOCKS_PER_SEC) > debounce_delay) {
                        if (reading_W != buttonState_W) {
                            buttonState_W = reading_W;
                            if (buttonState_W) {
                                // signal to shoot
                                Flags[2] = 1;
                            }
                        }
                    } 
                    lastState_W = reading_W;

                    // debounce structure for C
                    int reading_C = buttons.buttons[KEYCODE(C)];
                    if (reading_C != lastState_C) {
                        lastDebounceTime_C = clock();
                    }
                    timestamp = clock();
                    if ((((double) (timestamp - lastDebounceTime_C)
                            )/ CLOCKS_PER_SEC) > debounce_delay) {
                        if (reading_C != buttonState_C) {
                            buttonState_C = reading_C;
                            if (buttonState_C) {
                                // signal to change difficulty of AI
                                // (multiplayer)
                                Flags[4] = 1;
                            }
                        }
                    }
                    lastState_C = reading_C;

                    xSemaphoreGive(buttons.lock);
                }   

                // counting structure for triggering lasershot
                if (ticks_laser == 100) { 
                    Flags[3] = 1;
                    ticks_laser = 0;
                }
                // counting structure for mothership flythrough
                // (singleplayer)
                if (ticks_mot == 1000) {  
                    Flags[5] = 1;
                    ticks_mot = 0;
                }

                if (xSemaphoreTake(from_AI.lock, 0)) {
                    
                    // give next move CMD of AI to play_dynamics.c 
                    vGive_movementData(from_AI.move);

                    xSemaphoreGive(from_AI.lock);
                }

                xSemaphoreTake(ScreenLock, portMAX_DELAY);

                game_over = vDraw_playscreen(Flags, 
                                xLastWakeTime - prevWakeTime);

                vDrawFPS();
                
                xSemaphoreGive(ScreenLock);

                // game over 
                if (game_over == 1) {   
                    vTaskDelay(2000);
                    // return to main menu
                    xSemaphoreGive(state_machine_signal);
                    xQueueSend(next_state_queue, 
                                &next_state_mainmenu, 0);
                }
                // progress to next level
                if (game_over == 2) {    
                    // return to play state with next level init
                    xSemaphoreGive(state_machine_signal);
                    xQueueSend(next_state_queue,
                                &next_state_play, 0);
                    vTaskDelay(2000);
                }

                if (xSemaphoreTake(to_AI.lock, 0)) {
                    
                    // get delta-x from play_dynamics.c
                    delta_X = vGet_deltaX();

                    // get bullet status from play_dynamics.c
                    active = vGet_attacking();

                    // get difficulty from play_dynamics.c
                    difficulty = vGet_difficulty();
                    
                    // transform integer data to strings
                    sprintf(to_AI.delta_x, "%i", delta_X);

                    if (active) {
                        sprintf(to_AI.attacking, "ATTACKING");
                    }
                    else {
                        sprintf(to_AI.attacking, "PASSIVE");
                    }

                    sprintf(to_AI.difficulty, "D%i", difficulty);
                    
                    xSemaphoreGive(to_AI.lock);
                }

                ticks_laser++;
                ticks_mot++;
                
                // reset flags
                Flags[0] = 0;
                Flags[1] = 0;
                Flags[2] = 0;
                Flags[3] = 0;
                Flags[4] = 0;
                Flags[5] = 0;

                prevWakeTime = xLastWakeTime;
            } 
        } 
    }
}

void vPauseScreen(void *pvParameters) {

    int next_state_mainmenu = 0;
    int next_state_play = 1;

    while(1) {
        if (xSemaphoreTake(DrawSignal, portMAX_DELAY) ==
                pdTRUE) {

            xGetButtonInput(); // Update global input
            
            if (xSemaphoreTake(buttons.lock, 0) == pdTRUE) {
                
                // return to main menu
                if(buttons.buttons[KEYCODE(ESCAPE)]) {
                    xSemaphoreGive(buttons.lock);
                    // write hi-score to file
                    FILE *fp;
                    fp = fopen("../src/hscore.txt", "w");
                    if (fp != NULL) {
                        // get hi-score from play_dynamics.c
                        fprintf(fp, "%i", vGet_highScore());
                    }
                    fclose(fp); 
                    // initiiate state change
                    xSemaphoreGive(state_machine_signal);
                    xQueueSend(next_state_queue, &next_state_mainmenu, 0);
                }
                
                // resume playing
                if(buttons.buttons[KEYCODE(SPACE)]) {
                    xSemaphoreGive(buttons.lock);
                    // initiiate state change
                    xSemaphoreGive(state_machine_signal);
                    xQueueSend(next_state_queue, &next_state_play, 0);
                }
                xSemaphoreGive(buttons.lock);
            }

            xSemaphoreTake(ScreenLock, portMAX_DELAY);

            vDrawPauseScreen();
            
            vDrawFPS();

            xSemaphoreGive(ScreenLock);
        } 
    }
}

void vCheatView(void *pvParameters) {

    int next_state_mainmenu = 0;

    signed short mouse_X = 0;
    signed short mouse_Y = 0;

    int buttonState_Mouse = 0;
    int lastState_Mouse = 0;
    clock_t lastDebounceTime_Mouse;

    clock_t timestamp;
    double debounce_delay = 0.025;

    int buttonValue = 0;

    int trigger = 0;
    int lastTrigger = 0;

    int score = 0;
    int level = 0;

    int cheats[4] = { 0 };

    while(1) {
        if (xSemaphoreTake(DrawSignal, portMAX_DELAY) ==
                pdTRUE) {
            
            xGetButtonInput(); // Update global input
            
            if (xSemaphoreTake(buttons.lock, 0) == pdTRUE) {

                // return to main menu
                if(buttons.buttons[KEYCODE(ESCAPE)]) {
                    xSemaphoreGive(buttons.lock);
                    // send cheat values to state machine
                    for (int i=0; i<4; i++) {
                        xQueueSend(cheats_queue, &cheats[i], 0);
                    }
                    // initiate state change
                    xSemaphoreGive(state_machine_signal);
                    xQueueSend(next_state_queue, &next_state_mainmenu, 0);
                }
                xSemaphoreGive(buttons.lock);
            }
            // check button inputs
            // get mouse coordinates 
            mouse_X = tumEventGetMouseX();
            mouse_Y = tumEventGetMouseY();

            // debounce structure for mouse buttn
            int reading_Mouse = tumEventGetMouseLeft();

            if (reading_Mouse != lastState_Mouse) {
                lastDebounceTime_Mouse = clock();
            }
            timestamp = clock();
            if((((double) (timestamp - lastDebounceTime_Mouse)
                    )/ CLOCKS_PER_SEC) > debounce_delay){
                if (reading_Mouse != buttonState_Mouse){
                    buttonState_Mouse = reading_Mouse;
                        
                    if (buttonState_Mouse == 1){
                        // check button clicks
                        buttonValue = vCheckCheatScreenInput(mouse_X, 
                                                                mouse_Y);
                        switch(buttonValue) {
                            case 1:     // infinite lives button triggered
                                trigger = buttonValue;
                                if (trigger == lastTrigger) {
                                    trigger = 0;
                                }
                                else {
                                    trigger = 1;
                                }
                                lastTrigger = trigger;
                                cheats[1] = trigger;
                                break;
                            case 2:     // increase score
                                score += 10;
                                cheats[2] = score;
                                break;
                            case 3:     // decrease score
                                if (score > 0) {
                                    score -= 10;
                                }
                                cheats[2] = score;
                                break;
                            case 4:     // increase level
                                level++;
                                cheats[3] = level;
                                break;
                            case 5:     // decrease level
                                if (level > 0) {
                                    level--;
                                }
                                cheats[3] = level;
                                break;
                            default:
                                break;
                        }
                        // no cheats active
                        if ((trigger == 0) && (score == 0) 
                                && (level == 0)) {
                            cheats[0] = 0;
                        }
                        // at least one cheat active
                        else {
                            cheats[0] = 1;
                        }
                    }
                }
            }
            lastState_Mouse = reading_Mouse;

            xSemaphoreTake(ScreenLock, portMAX_DELAY);

            vDrawCheatScreen(trigger,score,level);
                
            vDrawFPS();

            xSemaphoreGive(ScreenLock);

        } 
    } 
}

void vStateMachine(void *pvParameters) {

    vTaskSuspend(playscreen_task);
    vTaskSuspend(pausescreen_task);
    vTaskSuspend(cheatview_task);

    vCreate_mutexes();

    int state = 0;
    int last_state = 0;

    int reset = 1;

    int level = 1;
    int multiplayer = 1;

    unsigned int Flags[4];
    Flags[0] = 0;

    const int state_change_period = 500;
    TickType_t last_change = xTaskGetTickCount();

    while(1) {
        if (xSemaphoreTake(state_machine_signal, 
                            portMAX_DELAY)) {
            xQueueReceive(next_state_queue, &state, portMAX_DELAY);
            if ((xTaskGetTickCount() - last_change) >
                    state_change_period) {

                if (state == 0) {
                    // MAIN MENU SCREEN 
                    vTaskSuspend(playscreen_task);
                    vTaskSuspend(pausescreen_task);
                    vTaskSuspend(cheatview_task);
                    vTaskResume(startscreen_task);

                    // initial level
                    level = 1;

                }
                if (state == 1) {
                    // PLAY SCREEN 
                    vTaskSuspend(startscreen_task);
                    vTaskSuspend(pausescreen_task);
                    vTaskSuspend(cheatview_task);
                    vTaskResume(playscreen_task);
                    
                    // level progress init
                    if (last_state == 1) {  
                        level++; 
                        // cheat init for next level
                        if (Flags[0]) {
                            vInit_playscreen(Flags[1], 0, 
                                             Flags[3] + level-1, 
                                             multiplayer,
                                             1);
                            vDrawNextLevelScreen(Flags[3] + level-1);
                        }
                        // normal init
                        else {
                            vInit_playscreen(0,0,level, multiplayer,
                                            0);
                            vDrawNextLevelScreen(level);
                        }
                        
                    }
                    // starting game from main menu
                    if (last_state == 0) {
                        // receive multiplayer signal
                        xQueueReceive(multipl_queue, &multiplayer, 0);
                        // receive cheats
                        for (int i=0; i<4; i++) {
                            xQueueReceive(cheats_queue, &Flags[i], 0);
                        }
                        // if cheats set initialize with cheat Flags
                        if (Flags[0]) {
                            vInit_playscreen(Flags[1], 
                                        Flags[2], Flags[3], multiplayer,
                                        1);
                        }
                        // else initialize with standard values
                        else {
                            vInit_playscreen(0,0,level, multiplayer,
                                            0);
                        }
                        
                    }
                    // bridge timegap for state change 
                    // reset means here reset Tickcount
                    xQueueSend(reset_queue, &reset, 0);
                }
                if (state == 2) {
                    // PAUSE SCREEN 
                    vTaskSuspend(playscreen_task);
                    vTaskSuspend(startscreen_task);
                    vTaskSuspend(cheatview_task);
                    vTaskResume(pausescreen_task);
                }
                if (state == 3) {
                    // CHEAT SCREEN
                    vTaskSuspend(playscreen_task);
                    vTaskSuspend(startscreen_task);
                    vTaskSuspend(pausescreen_task);
                    vTaskResume(cheatview_task);
                }
                last_change = xTaskGetTickCount();
                last_state = state;
            }
        }
    }
}

void vSendTask(void *pvParameters) 
{
    char last_delta_x[30];
    char last_attacking[30];
    char last_difficulty[10];
    char resume[] = "RESUME";
    char pause[] = "PAUSE";
    unsigned int paused = 0;

    while(1) {
        // sending here
        if (xSemaphoreTake(to_AI.lock, 0)) {
            // just send if something changed
            if (strcmp(last_delta_x, to_AI.delta_x) ||
                    strcmp(last_attacking, to_AI.attacking) ||
                    strcmp(last_difficulty, to_AI.difficulty)) {
                
                // resume sending if paused
                if (paused) {
                    aIOSocketPut(UDP, NULL, UDP_TRANSMIT_PORT, 
                                resume,
                                strlen(resume));
                }

                if (strcmp(last_delta_x, to_AI.delta_x)) {
                    // delta X
                    aIOSocketPut(UDP, NULL, UDP_TRANSMIT_PORT, 
                        to_AI.delta_x,
                        strlen(to_AI.delta_x));

                    strcpy(last_delta_x, to_AI.delta_x);
                }
                if (strcmp(last_attacking, to_AI.attacking)) {
                    // Attacking or not
                    aIOSocketPut(UDP, NULL, UDP_TRANSMIT_PORT, 
                        to_AI.attacking,
                        strlen(to_AI.attacking));

                    strcpy(last_attacking, to_AI.attacking);
                }
                if (strcmp(last_difficulty, to_AI.difficulty)) {
                    // difficulty
                    aIOSocketPut(UDP, NULL, UDP_TRANSMIT_PORT, 
                        to_AI.difficulty,
                        strlen(to_AI.difficulty));

                    strcpy(last_difficulty, to_AI.difficulty);
                }
                paused = 0;
                
            }
            // pause / resume
            // -> send pause when no value changed
            // -> resume when any value changes
            else {
                if (!paused) {
                    paused = 1;
                    aIOSocketPut(UDP, NULL, UDP_TRANSMIT_PORT,
                         pause, strlen(pause));
                }
            }

            xSemaphoreGive(to_AI.lock); 
        }
        vTaskDelay(15);
    }
    
}

void UDPHandlerOne(size_t read_size, char *buffer, void *args)
{   
    // receive handle
    // copy buffer into from_AI.move var
    strcpy(from_AI.move, buffer);     
}

void vReceiveTask(void *pvParameters)
{

    udp_soc_one = aIOOpenUDPSocket(NULL, UDP_RECEIVE_PORT, 
                                   UDP_BUFFER_SIZE, 
                                   UDPHandlerOne, NULL);

    while(1) {
        vTaskDelay(15);
    }
}

// main function #########################################################

#define PRINT_TASK_ERROR(task) PRINT_ERROR("Failed to print task ##task");

int main(int argc, char *argv[])
{
    // initialization 
    char *bin_folder_path = tumUtilGetBinFolderPath(argv[0]);

    printf("Initializing: ");

    if (tumDrawInit(bin_folder_path)) {
        PRINT_ERROR("Failed to initialize drawing");
        goto err_init_drawing;
    }

    if (tumEventInit()) {
        PRINT_ERROR("Failed to initialize events");
        goto err_init_events;
    }

    if (tumSoundInit(bin_folder_path)) {
        PRINT_ERROR("Failed to initialize audio");
        goto err_init_audio;
    }

    buttons.lock = xSemaphoreCreateMutex(); 
    if (!buttons.lock) {
        PRINT_ERROR("Failed to create buttons lock");
        goto err_buttons_lock;
    }

    ScreenLock = xSemaphoreCreateMutex();
    if (!ScreenLock) {
        PRINT_ERROR("Failed to create Screen lock");
        goto err_screen_lock;
    }

    to_AI.lock = xSemaphoreCreateMutex();
    if (!to_AI.lock) {
        PRINT_ERROR("Failed to create to AI data lock");
        goto err_toAI_lock;
    }

    from_AI.lock = xSemaphoreCreateMutex();
    if (!from_AI.lock) {
        PRINT_ERROR("Failed to create from AI data lock");
        goto err_fromAI_lock;
    }
 
    DrawSignal = xSemaphoreCreateBinary();
    if (!DrawSignal) {
        PRINT_ERROR("Failed to create Draw Signal");
        goto err_draw_signal;
    }

    state_machine_signal = xSemaphoreCreateBinary();
    if (!state_machine_signal) {
        PRINT_ERROR("Failed to create State Machine Signal");
        goto err_SM_signal;
    }

    next_state_queue = xQueueCreate(1, sizeof(int));
    if (!next_state_queue) {
        PRINT_ERROR("Failed to create Next state queue");
    }

    reset_queue = xQueueCreate(1, sizeof(int));
    if (!reset_queue) {
        PRINT_ERROR("Failed to create reset queue");
    }

    cheats_queue = xQueueCreate(4, sizeof(int));
    if (!cheats_queue) {
        PRINT_ERROR("Failed to create cheats queue");
    }

    nextLvl_queue = xQueueCreate(1, sizeof(int));
    if (!nextLvl_queue) {
        PRINT_ERROR("Failed to create next level queue");
    }

    multipl_queue = xQueueCreate(1, sizeof(int));
    if (!multipl_queue) {
        PRINT_ERROR("Failed to create multiplayer queue");
    }

    // Task Creation ##################################################

    if (xTaskCreate(vSwapBuffers, "", mainGENERIC_STACK_SIZE * 2,
                NULL, configMAX_PRIORITIES, &bufferswap) != pdPASS) {
        PRINT_TASK_ERROR("swap buffers");
    }
    if (xTaskCreate(vStateMachine, "", mainGENERIC_STACK_SIZE * 2,
                NULL, configMAX_PRIORITIES - 1, &statemachine) != pdPASS) {
        PRINT_TASK_ERROR("state machine");
    }
    if (xTaskCreate(vStart_screen, "", mainGENERIC_STACK_SIZE * 2,
                NULL, mainGENERIC_PRIORITY, &startscreen_task) != pdPASS) {
        
        PRINT_TASK_ERROR("startscreen_task");
    }
    if (xTaskCreate(vPlay_screen, "", mainGENERIC_STACK_SIZE * 2,
                NULL, mainGENERIC_PRIORITY, &playscreen_task) != pdPASS) {
        
        PRINT_TASK_ERROR("playscreen_task");
    }
    if (xTaskCreate(vPauseScreen, "", mainGENERIC_STACK_SIZE * 2,
                NULL, mainGENERIC_PRIORITY, &pausescreen_task) != pdPASS) {
        
        PRINT_TASK_ERROR("pausescreen_task");
    }
    if (xTaskCreate(vCheatView, "", mainGENERIC_STACK_SIZE * 2,
                NULL, mainGENERIC_PRIORITY, &cheatview_task) != pdPASS) {
        
        PRINT_TASK_ERROR("cheatview_task");
    }
    if (xTaskCreate(vSendTask, "", mainGENERIC_STACK_SIZE * 2,
                NULL, configMAX_PRIORITIES - 1, &send_task) != pdPASS) {

        PRINT_TASK_ERROR("send_task");
    }
    if (xTaskCreate(vReceiveTask, "", mainGENERIC_STACK_SIZE * 2,
                NULL, configMAX_PRIORITIES - 1, &receive_task) != pdPASS) {

        PRINT_TASK_ERROR("send_task");
    }

    vTaskStartScheduler();

    return EXIT_SUCCESS;

err_SM_signal:
    vSemaphoreDelete(from_AI.lock);
err_fromAI_lock:
    vSemaphoreDelete(to_AI.lock);
err_toAI_lock:
    vSemaphoreDelete(ScreenLock);
err_screen_lock:
    vSemaphoreDelete(DrawSignal);
err_draw_signal:
    vSemaphoreDelete(buttons.lock);
err_buttons_lock:
    tumSoundExit();
err_init_audio:
    tumEventExit();
err_init_events:
    tumDrawExit();
err_init_drawing:
    return EXIT_FAILURE;
}

// ###########################################
// cppcheck-suppress unusedFunction
__attribute__((unused)) void vMainQueueSendPassed(void)
{
    /* This is just an example implementation of the "queue send" trace hook. */
}

// cppcheck-suppress unusedFunction
__attribute__((unused)) void vApplicationIdleHook(void)
{
#ifdef __GCC_POSIX__
    struct timespec xTimeToSleep, xTimeSlept;
    /* Makes the process more agreeable when using the Posix simulator. */
    xTimeToSleep.tv_sec = 1;
    xTimeToSleep.tv_nsec = 0;
    nanosleep(&xTimeToSleep, &xTimeSlept);
#endif
}
