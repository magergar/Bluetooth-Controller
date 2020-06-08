#include "project.h"
#include <stdio.h>

int main(void)
{
    cy_en_ble_api_result_t apiResult;
    __enable_irq();                             // Enable global interrupts.
    
    if(Cy_SysPm_GetIoFreezeStatus())
        Cy_SysPm_IoUnfreeze();                  // Unfreeze IO if device is waking up from hibernate */
    
    apiResult = Cy_BLE_Start(NULL);             //Start the Controller portion of BLE. Host runs on the CM4
    
    if(apiResult == CY_BLE_SUCCESS)
        Cy_SysEnableCM4(CY_CORTEX_M4_APPL_ADDR); //Enable CM4 only if BLE Controller started successfully. 
    else
        CY_ASSERT(0u != 0u);                    // Halt CPU
    
    //**************Place your initialization code here**********************
    UART_Start();
    for(;;)
    {
        Cy_BLE_ProcessEvents();     //Allows BLE stack to process pending events  
    }
}

