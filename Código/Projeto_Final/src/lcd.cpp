#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "grlib/grlib.h"
#include "driverlib/rom.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "cfaf128x128x16.h"

#include <iostream>
using namespace std;

#define WALL_COLOR              0x00808080
#define TEXT_COLOR              0x00008000
#define SNAKE_INNER_COLOR       0x00008000
#define SNAKE_OUTER_COLOR       0x00006400
#define FOOD_INNER_COLOR        0x00FF0000
#define FOOD_OUTER_COLOR        0x008B0000
#define BACKGROUND_INNER_COLOR  0x00FFFAF0
#define BACKGROUND_OUTER_COLOR  0x00000000

#define LCD_SIZE 128
#define POSITION_SIZE 8         // Each position is POSITION_SIZE x POSITION_SIZE pixels


tContext sContext;

void new_print(uint32_t x, uint32_t y, uint32_t flag);

void initTela(void)
{
    GrFlush(&sContext);
        
    GrContextFontSet(&sContext, g_psFontFixed6x8);

    GrContextForegroundSet(&sContext, TEXT_COLOR);
    GrContextBackgroundSet(&sContext, WALL_COLOR);


    GrStringDraw(&sContext,"EMBEDDED SNAKE", -1, 0, (sContext.psFont->ui8Height+2)*0, false);
    GrStringDraw(&sContext,"---------------------", -1, 0, (sContext.psFont->ui8Height+2)*1, false);
    GrStringDraw(&sContext,"PRESS PAUSE BUTTON", -1, 0, (sContext.psFont->ui8Height+2)*2, false);
    GrStringDraw(&sContext,"TO START", -1, 0, (sContext.psFont->ui8Height+2)*3, false);
}

void initBackground(void)
{
    tRectangle left_wall, right_wall, top_wall, bottom_wall;
    int i, j;
    
    GrFlush(&sContext);
    
    // Criar as Paredes
    GrContextForegroundSet(&sContext, WALL_COLOR);
    
    left_wall.i16XMin = 0;
    left_wall.i16YMin = 0;
    left_wall.i16XMax = POSITION_SIZE-1;
    left_wall.i16YMax = LCD_SIZE - 1;
    
    right_wall.i16XMin = LCD_SIZE - POSITION_SIZE;
    right_wall.i16YMin = 0;
    right_wall.i16XMax = LCD_SIZE - 1;
    right_wall.i16YMax = LCD_SIZE - 1;
    
    bottom_wall.i16XMin = POSITION_SIZE - 1;
    bottom_wall.i16YMin = LCD_SIZE - POSITION_SIZE;
    bottom_wall.i16XMax = LCD_SIZE - POSITION_SIZE;
    bottom_wall.i16YMax = LCD_SIZE - 1;
    
    top_wall.i16XMin = POSITION_SIZE - 1;
    top_wall.i16YMin = 0;
    top_wall.i16XMax = LCD_SIZE - POSITION_SIZE;
    top_wall.i16YMax = POSITION_SIZE - 1;

    GrRectFill(&sContext, &left_wall);
    GrRectFill(&sContext, &right_wall);
    GrRectFill(&sContext, &top_wall);
    GrRectFill(&sContext, &bottom_wall);

    // Criar o quadriculado do Background 
    for(j = 1; j < (LCD_SIZE / POSITION_SIZE) - 1; j++)
    {
        for(i = 1; i < (LCD_SIZE / POSITION_SIZE) - 1; i++)
            new_print(i, j, 2);
    }
}

void initLCD(void)
{
    cfaf128x128x16Init();
    
    GrContextInit(&sContext, &g_sCfaf128x128x16);
    
    initTela();
}

// Marca uma nova posição da cobra ou a comida no LCD
// Se flag == 0 -> Comida
// Se flag == 1 -> Posição da Cobra
// Se flag == 2 -> Posição do Background
void new_print(uint32_t x, uint32_t y, uint32_t flag)
{
    tRectangle new_print;
    
    //Outer Colors
    GrFlush(&sContext);
    if (flag == 0)
        GrContextForegroundSet(&sContext, FOOD_OUTER_COLOR);
    else if (flag == 1)
        GrContextForegroundSet(&sContext, SNAKE_OUTER_COLOR);
    else
        GrContextForegroundSet(&sContext, BACKGROUND_OUTER_COLOR);
    
    new_print.i16XMin = POSITION_SIZE*x;
    new_print.i16YMin = POSITION_SIZE*y;
    new_print.i16XMax = (POSITION_SIZE*(x+1)) - 1;
    new_print.i16YMax = (POSITION_SIZE*(y+1)) - 1;
    
    GrRectFill(&sContext, &new_print);  
    
    //Inner Colors
    GrFlush(&sContext);
    if (flag == 0)
        GrContextForegroundSet(&sContext, FOOD_INNER_COLOR);
    else if (flag == 1)
        GrContextForegroundSet(&sContext, SNAKE_INNER_COLOR);
    else
        GrContextForegroundSet(&sContext, BACKGROUND_INNER_COLOR);
    
    new_print.i16YMin++;        // = POSITION_SIZE*y;
    new_print.i16XMax--;        // = (POSITION_SIZE*(x+1)) - 1;
    
    GrRectFill(&sContext, &new_print);     
}

// Desenha ou apaga o símbolo de "Pause" no LCD
void print_pause(bool pause)
{
    tRectangle pause_ret;
    
    GrFlush(&sContext);
    
    if (pause)
    {       
        GrContextForegroundSet(&sContext, 0x00000000);
        
        pause_ret.i16XMin = 120;
        pause_ret.i16YMin = 1;
        pause_ret.i16XMax = 122;
        pause_ret.i16YMax = 6;
        GrRectFill(&sContext, &pause_ret);
        
        pause_ret.i16XMin = 124;
        pause_ret.i16XMax = 126;
        GrRectFill(&sContext, &pause_ret);
      
    }
    else
    {
        GrContextForegroundSet(&sContext, 0x00808080);
        
        pause_ret.i16XMin = 120;
        pause_ret.i16YMin = 1;
        pause_ret.i16XMax = 126;
        pause_ret.i16YMax = 6;
        GrRectFill(&sContext, &pause_ret);
    }
}

// Mensagem de Game Over e o tamanho final da cobra
void game_over(int size)
{
    tRectangle background;
    char number_array[3 + sizeof(char)];
    sprintf(number_array, "%d", size);
    
    background.i16XMin = 0;
    background.i16YMin = 0;
    background.i16XMax = 127;
    background.i16YMax = 127;
    
    GrFlush(&sContext);
    
    GrContextForegroundSet(&sContext, 0x00000000);    
    GrRectFill(&sContext, &background);
    
    GrFlush(&sContext);
    GrContextFontSet(&sContext, g_psFontFixed6x8);

    GrContextForegroundSet(&sContext, TEXT_COLOR);
    GrContextBackgroundSet(&sContext, WALL_COLOR);


    GrStringDraw(&sContext,"EMBEDDED SNAKE", -1, 0, (sContext.psFont->ui8Height+2)*0, false);
    GrStringDraw(&sContext,"---------------------", -1, 0, (sContext.psFont->ui8Height+2)*1, false);
    GrStringDraw(&sContext,"GAME OVER", -1, 0, (sContext.psFont->ui8Height+2)*2, false);
    GrStringDraw(&sContext,"SIZE:", -1, 0, (sContext.psFont->ui8Height+2)*3, false);
    GrStringDraw(&sContext, number_array, -1, 0, (sContext.psFont->ui8Height+2)*4, false);
}

// Mensagem de You Win e o tamanho final da cobra
void you_win(void)
{
    tRectangle background;
    
    background.i16XMin = 0;
    background.i16YMin = 0;
    background.i16XMax = 127;
    background.i16YMax = 127;
    
    GrFlush(&sContext);
    
    GrContextForegroundSet(&sContext, 0x00000000);    
    GrRectFill(&sContext, &background);
    
    GrFlush(&sContext);
    GrContextFontSet(&sContext, g_psFontFixed6x8);

    GrContextForegroundSet(&sContext, TEXT_COLOR);
    GrContextBackgroundSet(&sContext, WALL_COLOR);


    GrStringDraw(&sContext,"EMBEDDED SNAKE", -1, 0, (sContext.psFont->ui8Height+2)*0, false);
    GrStringDraw(&sContext,"---------------------", -1, 0, (sContext.psFont->ui8Height+2)*1, false);
    GrStringDraw(&sContext,"YOU WIN", -1, 0, (sContext.psFont->ui8Height+2)*2, false);
    GrStringDraw(&sContext,"MAX SIZE: 196", -1, 0, (sContext.psFont->ui8Height+2)*3, false);
}