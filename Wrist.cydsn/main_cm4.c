#include "project.h"
#include <stdio.h>

cy_stc_ble_conn_handle_t    appConnHandle;
#define user_advmap					(ee_data.ee_flgs.ee_user_advmap)


//*********Function prototypes*************************
void StackEventHandler(uint32 event, void* eventParam);
void ShowError(char String[60],cy_en_ble_api_result_t apiResult, int halt);
void StartScan(uint8_t new_advmap);

void ShowError(char String[60],cy_en_ble_api_result_t apiResult, int halt)
{
    char Buffer[60];
    sprintf(Buffer,String, apiResult);
    UART_PutString(Buffer); 
    if(halt==1)
        CY_ASSERT(0u != 0u);                                                //Halt CPU
}


void BleFindMe_Init(void)
{
    cy_en_ble_api_result_t          apiResult;
    cy_stc_ble_stack_lib_version_t  stackVersion;  
    char Buffer[60];

    Cy_SysPm_SetHibWakeupSource(CY_SYSPM_HIBPIN1_LOW);                      //Configure switch SW2 as hibernate wake up source      
    apiResult = Cy_BLE_Start(StackEventHandler);                            //Start Host of BLE Component and register generic event handler
    if(apiResult != CY_BLE_SUCCESS)                                         //BLE stack initialization failed, check configuration, notify error and halt CPU in debug mode
        ShowError("Cy_BLE_Start API Error: %x \r\n",apiResult,1);
    else
        ShowError("Cy_BLE_Start API Error: %x \r\n",apiResult,0);
    
	apiResult = Cy_BLE_GetStackLibraryVersion(&stackVersion);
    
    if(apiResult != CY_BLE_SUCCESS)
        ShowError("Cy_BLE_Start API Error: %x \r\n",apiResult,1);
    else
    {
        sprintf(Buffer,"Stack Version: %d.%d.%d.%d \r\n", stackVersion.majorVersion, 
            stackVersion.minorVersion, stackVersion.patch, stackVersion.buildNumber);
        UART_PutString(Buffer); 
    }    
}

void StackEventHandler(uint32 event, void* eventParam)
{
    cy_en_ble_api_result_t      apiResult;
    uint8 i;
    cy_stc_ble_gatts_write_cmd_req_param_t *writeReqParameter;
    cy_stc_ble_gatt_handle_value_pair_t myHvp;
    cy_stc_ble_gapc_adv_report_param_t PeerDev;
    char Buffer[20];
    switch (event)
	{

        /**********************************************************
        *                       General Events
        ***********************************************************/

        
//*************************************************************************************************************************************
		/* This event is received when the BLE stack is started */
        
        case CY_BLE_EVT_STACK_ON:   
            UART_PutString("CY_BLE_EVT_STACK_ON \r\n");

            // Enter into discoverable mode so that remote device can search it 
            apiResult = Cy_BLE_GAPP_StartAdvertisement(CY_BLE_ADVERTISING_FAST, CY_BLE_PERIPHERAL_CONFIGURATION_0_INDEX);          
            if(apiResult != CY_BLE_SUCCESS)
                ShowError("Start Advertisement API Error: %d \r\n",apiResult,1);
            else
                ShowError("Start Advertisement API Success: %d \r\n",apiResult,0);

            apiResult = Cy_BLE_GAP_GetBdAddress();                                  //Get address of the device 
            if(apiResult != CY_BLE_SUCCESS) 
                ShowError("Cy_BLE_GAP_GetBdAddress API Error: %d \r\n",apiResult,0);
            else
                ShowError("Cy_BLE_GAP_GetBdAddress API Success: %d \r\n",apiResult,0);
            break;
//*************************************************************************************************************************************
                
                
                
        /* This event is received when there is a timeout */
        case CY_BLE_EVT_TIMEOUT:
            UART_PutString("CY_BLE_EVT_TIMEOUT \r\n");
            break;
            
        /* This event indicates that some internal HW error has occurred */    
		case CY_BLE_EVT_HARDWARE_ERROR:
            ShowError("Hardware Error \r\n",0,1);
			break;
            
        /*  This event will be triggered by host stack if BLE stack is busy or 
         *  not busy. Parameter corresponding to this event will be the state 
    	 *  of BLE stack.
         *  BLE stack busy = CYBLE_STACK_STATE_BUSY,
    	 *  BLE stack not busy = CYBLE_STACK_STATE_FREE 
         */
    	case CY_BLE_EVT_STACK_BUSY_STATUS:
            sprintf(Buffer,"CY_BLE_EVT_STACK_BUSY_STATUS: %x\r\n", *(uint8 *)eventParam);
            UART_PutString(Buffer);
            break;
        
        case CY_BLE_EVT_LE_SET_EVENT_MASK_COMPLETE:                         // This event indicates completion of Set LE event mask 
            UART_PutString("CY_BLE_EVT_LE_SET_EVENT_MASK_COMPLETE \r\n");
            break;

        case CY_BLE_EVT_SET_DEVICE_ADDR_COMPLETE:                           // This event indicates set device address command completed
            UART_PutString("CY_BLE_EVT_SET_DEVICE_ADDR_COMPLETE \r\n");
            break;
            
        /* This event indicates get device address command completed
           successfully */
        case CY_BLE_EVT_GET_DEVICE_ADDR_COMPLETE:
            UART_PutString("CY_BLE_EVT_GET_DEVICE_ADDR_COMPLETE: ");
            for(i = CY_BLE_GAP_BD_ADDR_SIZE; i > 0u; i--)
            {
                printf(Buffer,"%2.2x", ((cy_stc_ble_bd_addrs_t *)((cy_stc_ble_events_param_generic_t *)eventParam)->eventParams)->publicBdAddr[i-1]);
                UART_PutString(Buffer);
            }
            UART_PutString("\r\n");
            break;
         
        /* This event indicates set Tx Power command completed */
        case CY_BLE_EVT_SET_TX_PWR_COMPLETE:
            UART_PutString("CY_BLE_EVT_SET_TX_PWR_COMPLETE \r\n");
            break;
            
        /* This event indicates that stack shutdown is complete */
        case CY_BLE_EVT_STACK_SHUTDOWN_COMPLETE:
            UART_PutString("CY_BLE_EVT_STACK_SHUTDOWN_COMPLETE \r\n");
            UART_PutString("Entering hibernate mode \r\n"); 
            Cy_SysPm_Hibernate();
            /* Code execution will not reach here */
            /* Device wakes up from hibernate and performs reset sequence 
               when the reset switch or SW2 switch on the kit is pressed */            
            break;
                        
        /**********************************************************
        *                       GAP Events
        ***********************************************************/
       
        /* This event indicates peripheral device has started/stopped
           advertising */
//*************************************************************************************************************************************
        case CY_BLE_EVT_GAPP_ADVERTISEMENT_START_STOP:
           UART_PutString("CY_BLE_EVT_GAPP_ADVERTISEMENT_START_STOP: ");
            
            if(Cy_BLE_GetAdvertisementState() == CY_BLE_ADV_STATE_ADVERTISING)
            {
                UART_PutString("Advertisement started \r\n");
            }
            else if(Cy_BLE_GetAdvertisementState() == CY_BLE_ADV_STATE_STOPPED)
            {
                UART_PutString("Advertisement stopped \r\n");          
                Cy_BLE_Stop();             
            }                
            break;
//*************************************************************************************************************************************
            
            
            
        /* This event is generated at the GAP Peripheral end after connection 
           is completed with peer Central device */
        case CY_BLE_EVT_GAP_DEVICE_CONNECTED:
            UART_PutString("CY_BLE_EVT_GAP_DEVICE_CONNECTED \r\n");
            break;
           
        /* This event is generated when disconnected from remote device or 
           failed to establish connection */
            
            
//*************************************************************************************************************************************
        case CY_BLE_EVT_GAP_DEVICE_DISCONNECTED:  
            UART_PutString("CY BLE EVT GAP DEVICE DISCONNECTED");
            
            if(Cy_BLE_GetConnectionState(appConnHandle) == CY_BLE_CONN_STATE_DISCONNECTED)
            {
                printf(Buffer,"CY_BLE_EVT_GAP_DEVICE_DISCONNECTED %d\r\n", CY_BLE_CONN_STATE_DISCONNECTED);
                UART_PutString(Buffer);

                // Enter into discoverable mode so that remote device can search it
                apiResult = Cy_BLE_GAPP_StartAdvertisement(CY_BLE_ADVERTISING_FAST, CY_BLE_PERIPHERAL_CONFIGURATION_0_INDEX);          
                if(apiResult != CY_BLE_SUCCESS)
                    ShowError("Start Advertisement API Error: %d \r\n",apiResult,1);
                else
                    ShowError("Start Advertisement API Success: %d \r\n",apiResult,0);     
            }
            break;
//*************************************************************************************************************************************
        
    
        /* This event is generated at the GAP Central and the peripheral end 
           after connection parameter update is requested from the host to 
           the controller */
        case CY_BLE_EVT_GAP_CONNECTION_UPDATE_COMPLETE:
            UART_PutString("CY_BLE_EVT_GAP_CONNECTION_UPDATE_COMPLETE \r\n");
            break;

        /* This event is triggered instead of 'CY_BLE_EVT_GAP_DEVICE_CONNECTED', 
           if Link Layer Privacy is enabled in component customizer */
        case CY_BLE_EVT_GAP_ENHANCE_CONN_COMPLETE:
            
            /* BLE link is established */
            /* This event will be triggered since link layer privacy is enabled */
            UART_PutString("CY_BLE_EVT_GAP_ENHANCE_CONN_COMPLETE \r\n");
            if(Cy_BLE_GetState() == CY_BLE_STATE_ON)
            {           
            }
            break;    
                     
        /**********************************************************
        *                       GATT Events
        ***********************************************************/
            
        /* This event is generated at the GAP Peripheral end after connection 
           is completed with peer Central device */
       case CY_BLE_EVT_GATT_CONNECT_IND:
            appConnHandle = *(cy_stc_ble_conn_handle_t *)eventParam;
            sprintf(Buffer,"CY_BLE_EVT_GATT_CONNECT_IND: %x, %x \r\n", 
                        (*(cy_stc_ble_conn_handle_t *)eventParam).attId, 
                        (*(cy_stc_ble_conn_handle_t *)eventParam).bdHandle);
            UART_PutString(Buffer);
            break;
            
        /* This event is generated at the GAP Peripheral end after 
           disconnection */
        case CY_BLE_EVT_GATT_DISCONNECT_IND:
            UART_PutString("CY_BLE_EVT_GATT_DISCONNECT_IND \r\n");
            break;
         
        /* This event is triggered when 'GATT MTU Exchange Request' 
           received from GATT client device */
        case CY_BLE_EVT_GATTS_XCNHG_MTU_REQ:
            UART_PutString("CY_BLE_EVT_GATTS_XCNHG_MTU_REQ \r\n");
            break;
        
        /* This event is triggered when a read received from GATT 
           client device */
        case CY_BLE_EVT_GATTS_READ_CHAR_VAL_ACCESS_REQ:
            UART_PutString("CY_BLE_EVT_GATTS_READ_CHAR_VAL_ACCESS_REQ \r\n");
            break;

        /**********************************************************
        *                       Other Events
        ***********************************************************/
            
        case CY_BLE_EVT_GAPC_SCAN_PROGRESS_RESULT:
            UART_PutString("CY_BLE_EVT_GAPC_SCAN_PROGRESS_RESULT");
            sprintf(Buffer,"Addr: %s\n\n",PeerDev.peerBdAddr);
            UART_PutString(Buffer);
            sprintf(Buffer,"RSSI: %hhd\n\n",PeerDev.rssi);
            UART_PutString(Buffer);
            break;
        case CY_BLE_EVT_GAPC_SCAN_START_STOP:
            UART_PutString("CY_BLE_EVT_GAPC_SCAN_START_STOP");
            sprintf(Buffer,"Star/Stop Event\n");
            UART_PutString(Buffer);
            break;
        
 
        default:
            sprintf(Buffer,"Other event: %lx \r\n", (unsigned long) event);
            UART_PutString(Buffer);
			break;
	}
}

int main(void)
{
    BleFindMe_Init();       // Initialize BLE 
    __enable_irq();         //  Enable global interrupts.

    //**************Place your initialization code here**********************
    //Cy_BLE_GAPC_StopScan();
    //BLE->BLELL.SCAN_CONFIG = _CLR_SET_FLD32U(BLE->BLELL.SCAN_CONFIG, BLE_BLELL_SCAN_CONFIG_SCAN_CHANNEL_MAP, new_advmap);
    /* Variable used to store the return values of BLE APIs */
    cy_en_ble_api_result_t apiResult;

    
    
    if(Cy_BLE_GetScanState() == CY_BLE_SCAN_STATE_STOPPED)
    {   
        
        apiResult = Cy_BLE_GAPC_StartScan(CY_BLE_SCANNING_FAST, CY_BLE_CENTRAL_CONFIGURATION_0_INDEX);
        //apiResult = Cy_BLE_GAPC_StartDiscovery(cy_ble_discoveryInfo);
        if(apiResult == CY_BLE_SUCCESS)
            UART_PutString("BLE Scan Started\n");
        else
            ShowError("Failure!  : BLE - Failed to start scan : \r\n",apiResult,0);
        Cy_BLE_ProcessEvents();
    }
    
    for(;;)
    {
        Cy_BLE_ProcessEvents();             //Cy_Ble_ProcessEvents() allows BLE stack to process pending events 
    }
}
