#include <stdint.h>

#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/interrupt.h"
#include "grlib/grlib.h"
#include "cfaf128x128x16.h"
#include "tx_api.h"

bool pause = true;
extern TX_EVENT_FLAGS_GROUP pause_flag;
extern void print_pause(bool pause);
void pause_IntHandler(void);

void initPAUSE(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOL);    //L1 - Botao1
    
    while(!(SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOL))){}
    
    GPIODirModeSet(GPIO_PORTL_BASE, GPIO_PIN_1, GPIO_DIR_MODE_IN); //Entrada
    
    GPIOPadConfigSet(GPIO_PORTL_BASE, GPIO_PIN_1, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);    //Pull-up
    
    // Interrupcao          
    GPIOIntEnable(GPIO_PORTL_BASE, GPIO_INT_PIN_1);
    GPIOIntTypeSet(GPIO_PORTL_BASE, GPIO_PIN_1, GPIO_LOW_LEVEL);
    IntEnable(INT_GPIOL);
    IntPrioritySet(INT_GPIOL, 0x00);    //Mais importante por parar o jogo
    
    IntRegister(INT_GPIOL, pause_IntHandler);   //Define qual a ISR
     
    IntMasterEnable(); // Ativa as interrupções do sistema.
}

void pause_IntHandler(void)
{      
    GPIOIntDisable(GPIO_PORTL_BASE, GPIO_INT_PIN_1); // Desativa a interupção
    
    // Troca o valor da variavel de controle
    if (pause)
        pause = false;
    else
        pause = true;
   
    GPIOIntEnable(GPIO_PORTL_BASE, GPIO_INT_PIN_1); // Reativa a interupção
    
    tx_event_flags_set(&pause_flag, 0x1, TX_OR); // Acorda a Thread que desenha o Pause no LCD
}