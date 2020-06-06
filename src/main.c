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


static TaskHandle_t startscreen_task = NULL;
static TaskHandle_t playscreen_task = NULL;
static TaskHandle_t pausescreen_task = NULL;
static TaskHandle_t cheatview_task = NULL;
static TaskHandle_t hscoreview_task = NULL;
static TaskHandle_t bufferswap = NULL;
static TaskHandle_t statemachine = NULL;

static SemaphoreHandle_t DrawSignal = NULL;
static SemaphoreHandle_t ScreenLock = NULL;
static SemaphoreHandle_t state_machine_signal = NULL;

static QueueHandle_t next_state_queue = NULL;


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
    int next_state_highscores = 4;

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
                    if(buttons.buttons[KEYCODE(SPACE)]) {
                        xSemaphoreGive(buttons.lock);
                        xSemaphoreGive(state_machine_signal);
                        xQueueSend(next_state_queue, &next_state_play, 0);
                    }
                    xSemaphoreGive(buttons.lock);
                }
                // check button inputs
                // get mouse coordinates 
                mouse_X = tumEventGetMouseX();
                mouse_Y = tumEventGetMouseY();

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
                                button_pressed = vCheckMainMenuButtonInput(mouse_X, 
                                                            mouse_Y);
                            if (button_pressed == 1) {
                                xSemaphoreGive(state_machine_signal);
                                xQueueSend(next_state_queue, 
                                            &next_state_cheats, 0);
                            }
                            if (button_pressed == 2) {
                                xSemaphoreGive(state_machine_signal);
                                xQueueSend(next_state_queue, 
                                            &next_state_highscores, 0);
                            }
                        }
                    }
                }
                lastState_Mouse = reading_Mouse;

                xSemaphoreTake(ScreenLock, portMAX_DELAY);

                vDrawMainMenu(state);
                
                vDrawFPS();

                xSemaphoreGive(ScreenLock);

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

    unsigned int Flags[4] = { 0 };

    TickType_t xLastWakeTime, prevWakeTime;
    xLastWakeTime = xTaskGetTickCount();
    prevWakeTime = xLastWakeTime;

    vInit_playscreen();

    while (1) {
        if (DrawSignal) {
            if (xSemaphoreTake(DrawSignal, portMAX_DELAY) ==
                pdTRUE) {
                    
                xLastWakeTime = xTaskGetTickCount();

                xGetButtonInput(); // Update global input
                // currently in state 1
                /* when escape is pressed send signal 
                    to state machine to go to state 2
                    -> PAUSE screen
                */ 
                if (xSemaphoreTake(buttons.lock, 0) == pdTRUE) {

                    if (buttons.buttons[KEYCODE(ESCAPE)]) {
                        xSemaphoreGive(buttons.lock);
                        xSemaphoreGive(state_machine_signal);
                        xQueueSend(next_state_queue, &next_state_pause, 0);
                    }

                    if (buttons.buttons[KEYCODE(A)]) {
                        Flags[0] = 1;
                    }

                    if (buttons.buttons[KEYCODE(D)]) {
                        Flags[1] = 1;
                    }

                    if (buttons.buttons[KEYCODE(W)]) {
                        Flags[2] = 1;
                        Flags[3] = 1;
                    }
                    xSemaphoreGive(buttons.lock);
                }    
                
                xSemaphoreTake(ScreenLock, portMAX_DELAY);

                vDraw_playscreen(Flags, xLastWakeTime - prevWakeTime);
                
                vDrawFPS();

                xSemaphoreGive(ScreenLock);

                Flags[0] = 0;
                Flags[1] = 0;
                Flags[2] = 0;

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
                // currently in state 2
                /* when escape is pressed send signal 
                    to state machine to go to state 0
                    -> start screen
                */ 
                if (xSemaphoreTake(buttons.lock, 0) == pdTRUE) {

                    if(buttons.buttons[KEYCODE(ESCAPE)]) {
                        xSemaphoreGive(buttons.lock);
                        xSemaphoreGive(state_machine_signal);
                        xQueueSend(next_state_queue, &next_state_mainmenu, 0);
                    }
                    /* when SPACE is pressed send signal
                        to state machine to go to state 1
                        -> resume playing
                    */
                    if(buttons.buttons[KEYCODE(SPACE)]) {
                        xSemaphoreGive(buttons.lock);
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

    while(1) {
        if (xSemaphoreTake(DrawSignal, portMAX_DELAY) ==
            pdTRUE) {
            xGetButtonInput(); // Update global input
            // currently in state 3
            /* when escape is pressed send signal 
                to state machine to go to state 0
                -> start screen
            */ 
            if (xSemaphoreTake(buttons.lock, 0) == pdTRUE) {

                if(buttons.buttons[KEYCODE(ESCAPE)]) {
                    xSemaphoreGive(buttons.lock);
                    xSemaphoreGive(state_machine_signal);
                    xQueueSend(next_state_queue, &next_state_mainmenu, 0);
                }
                xSemaphoreGive(buttons.lock);
            }
        
            xSemaphoreTake(ScreenLock, portMAX_DELAY);

            vDrawCheatScreen();
                
            vDrawFPS();

            xSemaphoreGive(ScreenLock);
        } 
    } 
}

void vHScoreView(void *pvParameters) {

    int next_state_mainmenu = 0;

    while(1) {
        if (xSemaphoreTake(DrawSignal, portMAX_DELAY) ==
            pdTRUE) {
            xGetButtonInput(); // Update global input
            // currently in state 4
            /* when escape is pressed send signal 
                to state machine to go to state 0
                -> start screen
            */ 
            if (xSemaphoreTake(buttons.lock, 0) == pdTRUE) {

                if(buttons.buttons[KEYCODE(ESCAPE)]) {
                    xSemaphoreGive(buttons.lock);
                    xSemaphoreGive(state_machine_signal);
                    xQueueSend(next_state_queue, &next_state_mainmenu, 0);
                }
                xSemaphoreGive(buttons.lock);
            }
        
            xSemaphoreTake(ScreenLock, portMAX_DELAY);

            vDrawHScoreScreen();
                
            vDrawFPS();

            xSemaphoreGive(ScreenLock);
        } 
    }
    
}

void vStateMachine(void *pvParameters) {

    vTaskSuspend(playscreen_task);
    vTaskSuspend(pausescreen_task);
    vTaskSuspend(cheatview_task);
    vTaskSuspend(hscoreview_task);

    int state = 0;

    const int state_change_period = 750;
    TickType_t last_change = xTaskGetTickCount();

    while(1) {
        if (xSemaphoreTake(state_machine_signal, 
                            portMAX_DELAY)) {
            xQueueReceive(next_state_queue, &state, portMAX_DELAY);
            if ((xTaskGetTickCount() - last_change) >
                    state_change_period) {

                if (state == 0) {
                    vTaskSuspend(playscreen_task);
                    vTaskSuspend(pausescreen_task);
                    vTaskSuspend(cheatview_task);
                    vTaskSuspend(hscoreview_task);
                    vTaskResume(startscreen_task);
                }
                if (state == 1) {
                    vTaskSuspend(startscreen_task);
                    vTaskSuspend(pausescreen_task);
                    vTaskSuspend(cheatview_task);
                    vTaskSuspend(hscoreview_task);
                    vTaskResume(playscreen_task);
                }
                if (state == 2) {
                    vTaskSuspend(playscreen_task);
                    vTaskSuspend(startscreen_task);
                    vTaskSuspend(cheatview_task);
                    vTaskSuspend(hscoreview_task);
                    vTaskResume(pausescreen_task);
                }
                if (state == 3) {
                    vTaskSuspend(playscreen_task);
                    vTaskSuspend(startscreen_task);
                    vTaskSuspend(pausescreen_task);
                    vTaskSuspend(hscoreview_task);
                    vTaskResume(cheatview_task);
                }
                if (state == 4) {
                    vTaskSuspend(playscreen_task);
                    vTaskSuspend(startscreen_task);
                    vTaskSuspend(pausescreen_task);
                    vTaskSuspend(cheatview_task);
                    vTaskResume(hscoreview_task);
                }
                last_change = xTaskGetTickCount();
            }
        }
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

    DrawSignal = xSemaphoreCreateBinary();
    if (!DrawSignal) {
        PRINT_ERROR("Failed to create Draw Signal");
        goto err_draw_signal;
    }

    state_machine_signal = xSemaphoreCreateBinary();
    if (!state_machine_signal) {
        PRINT_ERROR("Failed to create State Machine Signal");
    }

    next_state_queue = xQueueCreate(1, sizeof(int));
    if (!next_state_queue) {
        PRINT_ERROR("Failed to create Next state queue");
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
    if (xTaskCreate(vHScoreView, "", mainGENERIC_STACK_SIZE * 2,
                NULL, mainGENERIC_PRIORITY, &hscoreview_task) != pdPASS) {
        
        PRINT_TASK_ERROR("highscoreview_task");
    }


    // End of Task Creation ##############################################

    vTaskStartScheduler();

    return EXIT_SUCCESS;


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
