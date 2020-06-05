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

#define mainGENERIC_PRIORITY (tskIDLE_PRIORITY)
#define mainGENERIC_STACK_SIZE ((unsigned short)2560)

#define KEYCODE(CHAR) SDL_SCANCODE_##CHAR

#define CENTER_X SCREEN_WIDTH/2
#define CENTER_Y SCREEN_HEIGHT/2

static TaskHandle_t drawtask = NULL;
static TaskHandle_t startscreen_task = NULL;
static TaskHandle_t bufferswap = NULL;
static TaskHandle_t statemachine = NULL;

static SemaphoreHandle_t DrawSignal = NULL;
static SemaphoreHandle_t ScreenLock = NULL;
static SemaphoreHandle_t state_machine_signal = NULL;


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

void vCheckButtonInput(void)
{
    if (buttons.buttons[KEYCODE(SPACE)]) {
        xSemaphoreGive(state_machine_signal); 
    }
    if (buttons.buttons[KEYCODE(ESCAPE)]) {
        printf("Pausing...\n");
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

    
                        
void vDrawFigures(void *pvParameters)
{
    signed short x_mothership = CENTER_X - 6*px;
    signed short y_mothership = CENTER_Y + 175;

    signed short x_bunker0 = CENTER_X - 150 - 12*px;
    signed short y_bunker0 = CENTER_Y + 100;

    signed short x_bunker1 = CENTER_X - 50 - 12*px;
    signed short y_bunker1 = CENTER_Y + 100;

    signed short x_bunker2 = CENTER_X + 50 - 12*px;
    signed short y_bunker2 = CENTER_Y + 100;

    signed short x_bunker3 = CENTER_X + 150 - 12*px;
    signed short y_bunker3 = CENTER_Y + 100;

    signed short x_aliens = CENTER_X - 130;
    signed short y_aliens = CENTER_Y - 120;

    signed short state = 1;

    int ticks = 0;

    while (1) {
        if (DrawSignal) {
            if (xSemaphoreTake(DrawSignal, portMAX_DELAY) ==
                pdTRUE) {
                xGetButtonInput(); // Update global input
                vCheckButtonInput();
                
                xSemaphoreTake(ScreenLock, portMAX_DELAY);

                vDrawPlayScreen();

                vDrawPlayer(x_mothership, y_mothership);

                vDrawBunker(x_bunker0, y_bunker0);
                vDrawBunker(x_bunker1, y_bunker1);
                vDrawBunker(x_bunker2, y_bunker2);
                vDrawBunker(x_bunker3, y_bunker3);

                vDrawAliens(x_aliens, y_aliens, state);

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


    while(1) {
        if (DrawSignal) {
            if (xSemaphoreTake(DrawSignal, portMAX_DELAY) ==
                pdTRUE) {
                xGetButtonInput(); // Update global input
                vCheckButtonInput();

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
                                vCheckMainMenuButtonInput(mouse_X, 
                                                            mouse_Y);
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

void vStateMachine(void *pvParameters) {

    vTaskSuspend(drawtask);

    unsigned short state = 0;

    const int state_change_period = 1000;
    TickType_t last_change = xTaskGetTickCount();

    while(1) {
        if (xSemaphoreTake(state_machine_signal, 
                            portMAX_DELAY)) {
            if ((xTaskGetTickCount() - last_change) >
                    state_change_period) {
                
                if (state == 0) {
                    state = 1;
                }
                else if (state == 1) {
                    state = 0;
                }

                if (state == 0) {
                    vTaskSuspend(drawtask);
                    vTaskResume(startscreen_task);
                }
                if (state == 1) {
                    vTaskSuspend(startscreen_task);
                    vTaskResume(drawtask);
                    
                }
                last_change = xTaskGetTickCount();
            }
        }
    }
}


// main function ################################

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


    // Task Creation ##############################
    // Screen 0 Tasks

    xTaskCreate(vSwapBuffers, "", mainGENERIC_STACK_SIZE * 2,
                NULL, configMAX_PRIORITIES, &bufferswap);

    xTaskCreate(vStateMachine, "", mainGENERIC_STACK_SIZE * 2,
                NULL, configMAX_PRIORITIES - 1, &statemachine);


    if (xTaskCreate(vDrawFigures, "", mainGENERIC_STACK_SIZE * 2,
                NULL, mainGENERIC_PRIORITY, &drawtask) != pdPASS) {
        
        PRINT_TASK_ERROR("drawTask");
    }
    if (xTaskCreate(vStart_screen, "", mainGENERIC_STACK_SIZE * 2,
                NULL, mainGENERIC_PRIORITY, &startscreen_task) != pdPASS) {
        
        PRINT_TASK_ERROR("startscreen_task");
    }

    // End of Task Creation #########################

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
