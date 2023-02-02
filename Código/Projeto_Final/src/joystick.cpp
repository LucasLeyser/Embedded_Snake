#include <stdint.h>

#include "driverlib/adc.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "inc/hw_memmap.h"

uint32_t joy_Y = 0;
uint32_t joy_X = 0;

void initJOY(void) 
{
    // Port PE4 -> horizontal
    // Port PE3 -> vertical

    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);     // ADC0 - JoyY
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC1);     // ADC1 - JoyX
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);    // E3 - JoyY
                                                    // E4 - JoyX

    //Espera a inicialização 
    while(!(    SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOE)  &&
                SysCtlPeripheralReady(SYSCTL_PERIPH_ADC0)   &&
                SysCtlPeripheralReady(SYSCTL_PERIPH_ADC1)   
                )){}

    // Configurando ADC
    GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3 | GPIO_PIN_4);

    ADCSequenceConfigure(ADC0_BASE, 3, ADC_TRIGGER_PROCESSOR, 0);
    ADCSequenceConfigure(ADC1_BASE, 3, ADC_TRIGGER_PROCESSOR, 0);

    ADCSequenceStepConfigure(ADC0_BASE, 3, 0, ADC_CTL_CH0 | ADC_CTL_IE | ADC_CTL_END);
    ADCSequenceStepConfigure(ADC1_BASE, 3, 0, ADC_CTL_CH9 | ADC_CTL_IE | ADC_CTL_END);

    ADCSequenceEnable(ADC0_BASE, 3);
    ADCSequenceEnable(ADC1_BASE, 3);

    //Limpar a flag de interrupcao
    ADCIntClear(ADC0_BASE, 3);
    ADCIntClear(ADC1_BASE, 3);
}

void updateJOY(void) 
{  
    ADCProcessorTrigger(ADC0_BASE, 3);              // Trigger da conversao
    while (!ADCIntStatus(ADC0_BASE, 3, false)) {}   // Espera a conversao terminar
    ADCIntClear(ADC0_BASE, 3);                      // Limpa a flag de interrupcao
    ADCSequenceDataGet(ADC0_BASE, 3, &joy_Y);       // Guarda o valor 
    
    // Repete para o outro eixo
    ADCProcessorTrigger(ADC1_BASE, 3);
    while (!ADCIntStatus(ADC1_BASE, 3, false)) {}
    ADCIntClear(ADC1_BASE, 3);
    ADCSequenceDataGet(ADC1_BASE, 3, &joy_X);
}