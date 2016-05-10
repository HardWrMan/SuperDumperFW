/**
  ******************************************************************************
  * @file    usbd_hid_core.c
  * @author  MCD Application Team
  * @version V1.2.0
  * @date    09-November-2015
  * @brief   This file provides the HID core functions.
  *
  * @verbatim
  *      
  *          ===================================================================      
  *                                HID Class  Description
  *          =================================================================== 
  *           This module manages the HID class V1.11 following the "Device Class Definition
  *           for Human Interface Devices (HID) Version 1.11 Jun 27, 2001".
  *           This driver implements the following aspects of the specification:
  *             - The Boot Interface Subclass
  *             - The Mouse protocol
  *             - Usage Page : Generic Desktop
  *             - Usage : Joystick)
  *             - Collection : Application 
  *      
  * @note     In HS mode and when the DMA is used, all variables and data structures
  *           dealing with the DMA during the transaction process should be 32-bit aligned.
  *           
  *      
  *  @endverbatim
  *
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2015 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */ 

/* Includes ------------------------------------------------------------------*/
#include "usbd_hid_core.h"
#include "usb-hid/usbd_desc.h"
#include "usbd_req.h"
#include "usb-hid/link.h"

//#include "main.h"

volatile uint8_t USB_Incoming[ HID_OUT_PACKET ];


#if BOOTLOADER_MODE
extern volatile struct shared  shared __attribute__((section(".ram_globals")));
#endif

/** @addtogroup STM32_USB_OTG_DEVICE_LIBRARY
  * @{
  */


/** @defgroup USBD_HID 
  * @brief usbd core module
  * @{
  */ 

/** @defgroup USBD_HID_Private_TypesDefinitions
  * @{
  */ 
/**
  * @}
  */ 


/** @defgroup USBD_HID_Private_Defines
  * @{
  */ 

/**
  * @}
  */ 


/** @defgroup USBD_HID_Private_Macros
  * @{
  */ 
/**
  * @}
  */ 




/** @defgroup USBD_HID_Private_FunctionPrototypes
  * @{
  */


uint8_t  USBD_HID_Init (void  *pdev, 
                               uint8_t cfgidx);

uint8_t  USBD_HID_DeInit (void  *pdev, 
                                 uint8_t cfgidx);

uint8_t  USBD_HID_Setup (void  *pdev, 
                                USB_SETUP_REQ *req);

static uint8_t  *USBD_HID_GetCfgDesc (uint8_t speed, uint16_t *length);

uint8_t  USBD_HID_DataIn (void  *pdev, uint8_t epnum);

uint8_t  USBD_HID_DataOut (void  *pdev, uint8_t epnum);
/**
  * @}
  */ 

/** @defgroup USBD_HID_Private_Variables
  * @{
  */ 

USBD_Class_cb_TypeDef  USBD_HID_cb = 
{
  USBD_HID_Init,
  USBD_HID_DeInit,
  USBD_HID_Setup,
  NULL, /*EP0_TxSent*/  
  NULL, /*EP0_RxReady*/
  USBD_HID_DataIn, /*DataIn*/
  USBD_HID_DataOut, /*DataOut*/
  NULL, /*SOF */
  NULL,
  NULL,      
  USBD_HID_GetCfgDesc,
#ifdef USB_OTG_HS_CORE  
  USBD_HID_GetCfgDesc, /* use same config as per FS */
#endif  
};

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4   
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */        
__ALIGN_BEGIN static uint32_t  USBD_HID_AltSet  __ALIGN_END = 0;

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4   
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */      
__ALIGN_BEGIN static uint32_t  USBD_HID_Protocol  __ALIGN_END = 0;

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4   
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */  
__ALIGN_BEGIN static uint32_t  USBD_HID_IdleState __ALIGN_END = 0;

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4   
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */ 
/* USB HID device Configuration Descriptor */
__ALIGN_BEGIN static uint8_t USBD_HID_CfgDesc[USB_HID_CONFIG_DESC_SIZ] __ALIGN_END =
{
  0x09, /* bLength: Configuration Descriptor size */
  USB_CONFIGURATION_DESCRIPTOR_TYPE, /* bDescriptorType: Configuration */
  USB_HID_CONFIG_DESC_SIZ,
  /* wTotalLength: Bytes returned */
  0x00,
  0x01,         /*bNumInterfaces: 1 interface*/
  0x01,         /*bConfigurationValue: Configuration value*/
  0x00,         /*iConfiguration: Index of string descriptor describing
  the configuration*/
  0xE0,         /*bmAttributes: bus powered and Support Remote Wake-up */
  0x32,         /*MaxPower 100 mA: this current is used for detecting Vbus*/
  
  /************** Descriptor of Joystick Mouse interface ****************/
  /* 09 */
  0x09,         /*bLength: Interface Descriptor size*/
  USB_INTERFACE_DESCRIPTOR_TYPE,/*bDescriptorType: Interface descriptor type*/
  0x00,         /*bInterfaceNumber: Number of Interface*/
  0x00,         /*bAlternateSetting: Alternate setting*/
  0x02,         /*bNumEndpoints*/
  0x03,         /*bInterfaceClass: HID*/
  0x01,         /*bInterfaceSubClass : 1=BOOT, 0=no boot*/
  0x02,         /*nInterfaceProtocol : 0=none, 1=keyboard, 2=mouse*/
  0,            /*iInterface: Index of string descriptor*/
  /******************** Descriptor of Joystick Mouse HID ********************/
  /* 18 */
  0x09,         /*bLength: HID Descriptor size*/
  HID_DESCRIPTOR_TYPE, /*bDescriptorType: HID*/
  0x11,         /*bcdHID: HID Class Spec release number*/
  0x01,
  0x00,         /*bCountryCode: Hardware target country*/
  0x01,         /*bNumDescriptors: Number of HID class descriptors to follow*/
  0x22,         /*bDescriptorType*/
  HID_PHOENIX_REPORT_DESC_SIZE,/*wItemLength: Total length of Report descriptor*/
  0x00,
  /******************** Descriptor of Custom HID endpoints ***********/
  /* 27 */
  0x07,                   /* bLength: Endpoint Descriptor size */
  USB_ENDPOINT_DESCRIPTOR_TYPE, /* bDescriptorType: */

  HID_CONTROL_IN_EP,      /* bEndpointAddress: Endpoint Address (IN) */
  0x03,                   /* bmAttributes: Interrupt endpoint */
  HID_IN_PACKET,          /* wMaxPacketSize: 8 Bytes max */
  0x00,
  1,                     /* bInterval: Polling Interval (20 ms) */

  /* 34 */
  0x07,                           /* bLength: Endpoint Descriptor size */
  USB_ENDPOINT_DESCRIPTOR_TYPE,   /* bDescriptorType: */
                                  /* Endpoint descriptor type */
  HID_CONTROL_OUT_EP,             /* bEndpointAddress: */
                                  /* Endpoint Address (OUT) */
  0x03,                           /* bmAttributes: Interrupt endpoint */
  HID_OUT_PACKET,                 /* wMaxPacketSize: 8 Bytes max  */
  0x00,
  1,                             /* bInterval: Polling Interval (20 ms) */
  /* 41 */
} ;

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4   
  #endif
/* USB HID device Configuration Descriptor */
__ALIGN_BEGIN static uint8_t USBD_HID_Desc[USB_HID_DESC_SIZ] __ALIGN_END=
{
  /* 18 */
  0x09,         /*bLength: HID Descriptor size*/
  HID_DESCRIPTOR_TYPE, /*bDescriptorType: HID*/
  0x11,         /*bcdHID: HID Class Spec release number*/
  0x01,
  0x00,         /*bCountryCode: Hardware target country*/
  0x01,         /*bNumDescriptors: Number of HID class descriptors to follow*/
  0x22,         /*bDescriptorType*/
  HID_PHOENIX_REPORT_DESC_SIZE,/*wItemLength: Total length of Report descriptor*/
  0x00,
};
#endif 


#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4   
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */  
__ALIGN_BEGIN static uint8_t HID_PHOENIX_ReportDesc[HID_PHOENIX_REPORT_DESC_SIZE] __ALIGN_END =
{
        0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
        0x09, 0x00,                    // USAGE (Undefined)
        0xa1, 0x01,                    // COLLECTION (Application)
        0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
        0x26, 0xff, 0x00,              //   LOGICAL_MAXIMUM (255)
        0x75, 0x08,                    //   REPORT_SIZE (8)
        0x95, 64,                       //   REPORT_COUNT (64)
        0x09, 0x00,                    //   USAGE (Undefined)
        0x81, 0x02,                    //   INPUT (Data,Var,Abs,Vol) - to the host
        0x75, 0x08,                    //   REPORT_SIZE (8)
        0x95, 64,                       //   REPORT_COUNT (64)
        0x09, 0x00,                    //   USAGE (Undefined)
        0x91, 0x02,                    //   OUTPUT (Data,Var,Abs,Vol) - from the host
        0xc0                           // END_COLLECTION
}; 

/**
  * @}
  */ 

/** @defgroup USBD_HID_Private_Functions
  * @{
  */ 

/**
  * @brief  USBD_HID_Init
  *         Initialize the HID interface
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
uint8_t USBD_HID_Init(void *pdev, uint8_t cfgidx)
{
    /* Open Control EP IN */
    DCD_EP_Open( pdev,
                 HID_CONTROL_IN_EP,
                 HID_IN_PACKET,
                 USB_OTG_EP_INT      // Тип конечной точки - interrupt
               );

    /* Open EP OUT */
    DCD_EP_Open( pdev,
                 HID_CONTROL_OUT_EP,
                 HID_OUT_PACKET,
                 USB_OTG_EP_INT     // Тип конечной точки - interrupt
               );

    /*Receive Data*/
    DCD_EP_PrepareRx( pdev,
                      HID_CONTROL_OUT_EP,
                      (uint8_t *) USB_Incoming,
                      HID_OUT_PACKET );

    return USBD_OK;
}

/**
  * @brief  USBD_HID_Init
  *         DeInitialize the HID layer
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
uint8_t  USBD_HID_DeInit (void  *pdev, uint8_t cfgidx)
{
    /* Close HID EPs */
    DCD_EP_Close( pdev, HID_CONTROL_IN_EP );
    DCD_EP_Close( pdev, HID_CONTROL_OUT_EP );
    return USBD_OK;
}

/**
  * @brief  USBD_HID_Setup
  *         Handle the HID specific requests
  * @param  pdev: instance
  * @param  req: usb requests
  * @retval status
  */
uint8_t  USBD_HID_Setup (void  *pdev, 
                         USB_SETUP_REQ *req)
{
  uint16_t len = 0;
  uint8_t  *pbuf = NULL;
  
  switch (req->bmRequest & USB_REQ_TYPE_MASK)
  {
  case USB_REQ_TYPE_CLASS :  
    switch (req->bRequest)
    {
    case HID_REQ_SET_PROTOCOL:
      USBD_HID_Protocol = (uint8_t)(req->wValue);
      break;
      
    case HID_REQ_GET_PROTOCOL:
      USBD_CtlSendData (pdev, 
                        (uint8_t *)&USBD_HID_Protocol,
                        1);    
      break;
      
    case HID_REQ_SET_IDLE:
      USBD_HID_IdleState = (uint8_t)(req->wValue >> 8);
      break;
      
    case HID_REQ_GET_IDLE:
      USBD_CtlSendData (pdev, 
                        (uint8_t *)&USBD_HID_IdleState,
                        1);        
      break;      
      
    default:
      USBD_CtlError (pdev, req);
      return USBD_FAIL; 
    }
    break;
    
  case USB_REQ_TYPE_STANDARD:
    switch (req->bRequest)
    {
    case USB_REQ_GET_DESCRIPTOR: 
      if( req->wValue >> 8 == HID_REPORT_DESC)
      {
        len = MIN(HID_PHOENIX_REPORT_DESC_SIZE , req->wLength);
        pbuf = HID_PHOENIX_ReportDesc;
      }
      else if( req->wValue >> 8 == HID_DESCRIPTOR_TYPE)
      {
        
#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
        pbuf = USBD_HID_Desc;   
#else
        pbuf = USBD_HID_CfgDesc + 0x12;
#endif 
        len = MIN(USB_HID_DESC_SIZ , req->wLength);
      }
      else
      {
        /* Do Nothing */
      }
      
      USBD_CtlSendData (pdev, 
                        pbuf,
                        len);
      
      break;
      
    case USB_REQ_GET_INTERFACE :
      USBD_CtlSendData (pdev,
                        (uint8_t *)&USBD_HID_AltSet,
                        1);
      break;
      
    case USB_REQ_SET_INTERFACE :
      USBD_HID_AltSet = (uint8_t)(req->wValue);
      break;
      
    default:
      USBD_HID_AltSet = (uint8_t)(req->wValue);
      break; 
    }
    break;
    
  default:
    USBD_CtlSendData (pdev,
                      (uint8_t *)&USBD_HID_AltSet,
                      1);
    break; 
  }
  return USBD_OK;
}

/**
  * @brief  USBD_HID_SendReport 
  *         Send HID Report
  * @param  pdev: device instance
  * @param  buff: pointer to report
  * @retval status
  */
uint8_t USBD_HID_SendReport     (USB_OTG_CORE_HANDLE  *pdev, 
                                 uint8_t *report,
                                 uint16_t len)
{
    if ( pdev->dev.device_status == USB_OTG_CONFIGURED )
    {
        DCD_EP_Tx( pdev, HID_CONTROL_IN_EP, report, len );
    }
    return USBD_OK;
}

// Получаем статус EP
uint32_t USBD_HID_Empty (USB_OTG_CORE_HANDLE  *pdev)
{	// Переменная
	uint32_t Res;
	if ( pdev->dev.device_status == USB_OTG_CONFIGURED )
	{
		Res = DCD_EP_Empty( pdev, HID_CONTROL_IN_EP );
	}
	else
	{
		Res = 0x00000000;
	}
	return Res;
}

/**
  * @brief  USBD_HID_GetCfgDesc 
  *         return configuration descriptor
  * @param  speed : current device speed
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
static uint8_t  *USBD_HID_GetCfgDesc (uint8_t speed, uint16_t *length)
{
  *length = sizeof (USBD_HID_CfgDesc);
  return USBD_HID_CfgDesc;
}

/**
  * @brief  USBD_HID_DataIn
  *         handle data IN Stage
  * @param  pdev: device instance
  * @param  epnum: endpoint index
  * @retval status
  */
uint8_t  USBD_HID_DataIn (void  *pdev, uint8_t epnum)
{
    /* Ensure that the FIFO is empty before a new transfer, this condition could
     be caused by  a new transfer before the end of the previous transfer */
    DCD_EP_Flush( pdev, HID_CONTROL_IN_EP );

    return USBD_OK;
}

uint8_t  USBD_HID_DataOut (void  *pdev, uint8_t epnum)
{
	USB_OTG_CORE_HANDLE  *dev = (USB_OTG_CORE_HANDLE *)pdev;
	USB_OTG_EP *ep = &dev->dev.out_ep[epnum & 0x7F];
	uint32_t count = ep->xfer_count;
	uint8_t Cnt, Sum;
    //
    if( epnum == HID_CONTROL_OUT_EP )
    {	// Анализируем пакет
    	if ((((USB_Incoming[0]+USB_Incoming[1]) & 0xFF) == 0) && ((USB_Incoming[1] & 0x3F) > 0) && ((USB_Incoming[1] & 0x3F) < 61))
    	{	// Синхра найдена, вычисляем контрольку
    		Sum = 0; for (Cnt = 0; Cnt < (USB_Incoming[1] & 0x3F); Cnt++) { Sum = Sum + USB_Incoming[Cnt+2]; }
    		// Сверяем
    		if (((USB_Incoming[(USB_Incoming[1] & 0x3F)+2]) == Sum) && (USB_Rx[0] == 0))
    		{	// Пакет верен, копируем
    			for (Cnt = 0;Cnt < (USB_Incoming[1] & 0x3F); Cnt++) { USB_Rx[Cnt+1] = USB_Incoming[Cnt+2]; }
    			USB_Rx[0] = USB_Incoming[1];
    		}
    	}
        // Подготовка конечной точки к приёму следующего пакета
        // от хоста
        DCD_EP_PrepareRx( pdev,
                          HID_CONTROL_OUT_EP,
                          (uint8_t *) USB_Incoming,
                          HID_OUT_PACKET );

//#if BOOTLOADER_MODE
//        if( ok )
//        {
//            shared.fw_update_req =  SWITCH_TO_BOOTLOADER;
//            NVIC_SystemReset();
//        }
//#endif
    }
    return USBD_OK;
}

/**
  * @brief  USBD_HID_GetPollingInterval 
  *         return polling interval from endpoint descriptor
  * @param  pdev: device instance
  * @retval polling interval
  */
uint32_t USBD_HID_GetPollingInterval (USB_OTG_CORE_HANDLE *pdev)
{
  uint32_t polling_interval = 0;

  /* HIGH-speed endpoints */
  if(pdev->cfg.speed == USB_OTG_SPEED_HIGH)
  {
   /* Sets the data transfer polling interval for high speed transfers. 
    Values between 1..16 are allowed. Values correspond to interval 
    of 2 ^ (bInterval-1). This option (8 ms, corresponds to HID_HS_BINTERVAL */
    polling_interval = (((1 <<(HID_HS_BINTERVAL - 1)))/8);
  }
  else   /* LOW and FULL-speed endpoints */
  {
    /* Sets the data transfer polling interval for low and full 
    speed transfers */
    polling_interval =  HID_FS_BINTERVAL;
  }
  
  return ((uint32_t)(polling_interval));
}

/**
  * @}
  */ 


/**
  * @}
  */ 


/**
  * @}
  */ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
