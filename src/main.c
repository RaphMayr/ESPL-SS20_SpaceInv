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

#include "static_graphics.h"

#define mainGENERIC_PRIORITY (tskIDLE_PRIORITY)
#define mainGENERIC_STACK_SIZE ((unsigned short)2560)

#define KEYCODE(CHAR) SDL_SCANCODE_##CHAR

#define CENTER_X SCREEN_WIDTH/2
#define CENTER_Y SCREEN_HEIGHT/2

static TaskHandle_t drawtask = NULL;
static TaskHandle_t bufferswap = NULL;

static SemaphoreHandle_t DrawSignal = NULL;
static SemaphoreHandle_t ScreenLock = NULL;


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


// DRAWTASK #########################################################################
                        
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

    signed short x_fredAlien = CENTER_X - 6*px;
    signed short y_fredAlien = CENTER_Y + 50;

    signed short x_crabAlien = CENTER_X - 4*px;
    signed short y_crabAlien = CENTER_Y;

    signed short x_jellyAlien = CENTER_X - 5*px;
    signed short y_jellyAlien = CENTER_Y - 50;

    signed short state = 1;

    int ticks = 0;

    while (1) {
        if (DrawSignal) {
            if (xSemaphoreTake(DrawSignal, portMAX_DELAY) ==
                pdTRUE) {
                xGetButtonInput(); // Update global input
                
                xSemaphoreTake(ScreenLock, portMAX_DELAY);

                vDrawPlayScreen();

                vDrawMotherShip(x_mothership, y_mothership);

                vDrawBunker(x_bunker0, y_bunker0);
                vDrawBunker(x_bunker1, y_bunker1);
                vDrawBunker(x_bunker2, y_bunker2);
                vDrawBunker(x_bunker3, y_bunker3);

                vDraw_fredAlien(x_fredAlien, y_fredAlien, 
                                state);

                vDraw_crabAlien(x_crabAlien, y_crabAlien, 
                                state);

                vDraw_jellyAlien(x_jellyAlien, y_jellyAlien,
                                state);

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

void vMoveFigures(void) {

    
    while(1) {



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


    // Task Creation ##############################
    // Screen 0 Tasks

    xTaskCreate(vSwapBuffers, "", mainGENERIC_STACK_SIZE * 2,
                NULL, configMAX_PRIORITIES, &bufferswap);

    if (xTaskCreate(vDrawFigures, "", mainGENERIC_STACK_SIZE * 2,
                NULL, mainGENERIC_PRIORITY, &drawtask) != pdPASS) {
        
        PRINT_TASK_ERROR("drawTask");
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
