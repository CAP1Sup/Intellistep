/*
 * CAN module object for generic microcontroller.
 *
 * This file is a template for other microcontrollers.
 *
 * @file        CO_driver.c
 * @ingroup     CO_driver
 * @version     SVN: \$Id: CO_driver.c 57 2014-10-09 15:37:29Z jani22 $
 * @author      Janez Paternoster
 * @copyright   2004 - 2013 Janez Paternoster
 *
 * This file is part of CANopenNode, an opensource CANopen Stack.
 * Project home page is <http://canopennode.sourceforge.net>.
 * For more information on CANopen see <http://www.can-cia.org/>.
 *
 * CANopenNode is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "stm32f1xx_hal.h"
#include "CO_driver_target.h"
#include "301/CO_driver.h"
#include "301/CO_Emergency.h"
#include "OD.h"

#define CAN_CLK               48000

extern CAN_HandleTypeDef hcan;
extern void _Error_Handler(char * file, int line);
static void CO_CANsendToModule(CO_CANmodule_t *CANmodule, CO_CANtx_t *buffer, uint8_t transmit_mailbox);


/******************************************************************************/
void CO_CANsetConfigurationMode(void *CANptr)
{

}


/******************************************************************************/
void CO_CANsetNormalMode(CO_CANmodule_t *CANmodule){
    /* Put CAN module in normal mode */

    CANmodule->CANnormal = true;
}


/******************************************************************************/
CO_ReturnError_t CO_CANmodule_init(CO_CANmodule_t *CANmodule,
                                   void *CANptr,
                                   CO_CANrx_t rxArray[],
                                   uint16_t rxSize,
                                   CO_CANtx_t txArray[],
                                   uint16_t txSize,
                                   uint16_t CANbitRate)
{
	uint16_t i;

	CAN_FilterConfTypeDef  	sFilterConfig;
	static CanTxMsgTypeDef  TxMessage;
	static CanRxMsgTypeDef  RxMessage;

	/* Configure object variables */
	CANmodule->CANbaseAddress = CANmodule->CANbaseAddress;
	CANmodule->rxArray = rxArray;
	CANmodule->rxSize = rxSize;
	CANmodule->txArray = txArray;
	CANmodule->txSize = txSize;
	CANmodule->CANnormal = false;
	CANmodule->useCANrxFilters = (rxSize <= 32U) ? true : false;/* microcontroller dependent */
	CANmodule->bufferInhibitFlag = false;
	CANmodule->firstCANtxMessage = true;
	CANmodule->CANtxCount = 0U;
	CANmodule->errOld = 0U;
	CANmodule->em = NULL;

	for(i=0U; i<rxSize; i++)
	{
		rxArray[i].ident = 0U;
		rxArray[i].CANrx_callback = NULL;
	}
	for(i=0U; i<txSize; i++)
	{
		txArray[i].bufferFull = false;
	}

	/* Configure CAN module registers */
	if ( CANmodule->CANbaseAddress == ADDR_CAN1 )
			hcan.Instance = CAN1;
	else
		return CO_ERROR_ILLEGAL_ARGUMENT;

	hcan.pTxMsg = &TxMessage;
	hcan.pRxMsg = &RxMessage;

	hcan.Init.TTCM = DISABLE;
	hcan.Init.ABOM = ENABLE;
	hcan.Init.AWUM = DISABLE;
	hcan.Init.NART = DISABLE;
	hcan.Init.RFLM = DISABLE;
	hcan.Init.TXFP = DISABLE;


	if (CANbitRate >= 100 && CANbitRate <= 1000)
	{
		hcan.Init.Prescaler  = (CAN_CLK / 16) / CANbitRate;

		/* Load the baudrate registers BTR                                       */
		/* so that sample point is at about 85% bit time from bit start          */
		hcan.Init.SJW = CAN_SJW_1TQ;
		hcan.Init.BS1 = CAN_BS1_13TQ;
		hcan.Init.BS2 = CAN_BS2_2TQ;
		//hcan.Init.SJW = CAN_SJW_1TQ;
		//hcan.Init.BS1 = CAN_BS1_4TQ;
		//8hcan.Init.BS2 = CAN_BS2_3TQ;
	}
	else
		return CO_ERROR_ILLEGAL_ARGUMENT;

	hcan.Init.Mode = CAN_MODE_NORMAL;// CAN_MODE_LOOPBACK // CAN_MODE_NORMAL
	if (HAL_CAN_Init(&hcan) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

	/* Configure CAN module hardware filters */
	/*********************************/
	CANmodule->useCANrxFilters = true;
	/********************************/

	if(CANmodule->useCANrxFilters)
	{
		/* CAN module filters are used, they will be configured with */
		/* CO_CANrxBufferInit() functions, called by separate CANopen */
		/* init functions. */
		/* Configure all masks so, that received message must match filter */

		sFilterConfig.FilterNumber = 0;
		sFilterConfig.FilterMode = CAN_FILTERMODE_IDLIST;
		sFilterConfig.FilterScale = CAN_FILTERSCALE_16BIT;
		sFilterConfig.FilterIdHigh = 0x0000;
		sFilterConfig.FilterIdLow = (0x80 + OD_CANNodeID) << 5;
		sFilterConfig.FilterMaskIdHigh = (0x200 + OD_CANNodeID) << 5;
		sFilterConfig.FilterMaskIdLow = (0x300 + OD_CANNodeID) << 5;
		sFilterConfig.FilterFIFOAssignment = 0;
		sFilterConfig.FilterActivation = ENABLE;
		sFilterConfig.BankNumber = 13;
		HAL_CAN_ConfigFilter(&hcan, &sFilterConfig);


		sFilterConfig.FilterNumber = 1;
		sFilterConfig.FilterMode = CAN_FILTERMODE_IDLIST;
		sFilterConfig.FilterScale = CAN_FILTERSCALE_16BIT;
		sFilterConfig.FilterIdHigh = (0x400 + OD_CANNodeID) << 5;
		sFilterConfig.FilterIdLow = (0x500 + OD_CANNodeID) << 5;
		sFilterConfig.FilterMaskIdHigh = (0x600 + OD_CANNodeID) << 5;
		sFilterConfig.FilterMaskIdLow = OD_CANNodeID << 5;
		sFilterConfig.FilterFIFOAssignment = 0;
		sFilterConfig.FilterActivation = ENABLE;
		sFilterConfig.BankNumber = 13;
		HAL_CAN_ConfigFilter(&hcan, &sFilterConfig);

		sFilterConfig.FilterNumber = 2;
		sFilterConfig.FilterMode = CAN_FILTERMODE_IDLIST;
		sFilterConfig.FilterScale = CAN_FILTERSCALE_16BIT;
		sFilterConfig.FilterIdHigh = (0x77F) << 5;
		sFilterConfig.FilterIdLow = 0;
		sFilterConfig.FilterMaskIdHigh = 0;
		sFilterConfig.FilterMaskIdLow = 0;
		sFilterConfig.FilterFIFOAssignment = 0;
		sFilterConfig.FilterActivation = ENABLE;
		sFilterConfig.BankNumber = 13;
		HAL_CAN_ConfigFilter(&hcan, &sFilterConfig);
	}
	else
	{
		/* CAN module filters are not used, all messages with standard 11-bit */
		/* identifier will be received */
		/* Configure mask 0 so, that all messages with standard identifier are accepted */
		/*##-2- Configure the CAN Filter ###########################################*/
		sFilterConfig.FilterNumber = 0;
		sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;//CAN_FILTERMODE_IDMASK CAN_FILTERMODE_IDLIST
		sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
		sFilterConfig.FilterIdHigh = 0x0000;
		sFilterConfig.FilterIdLow = 0x0000;
		sFilterConfig.FilterMaskIdHigh = 0;
		sFilterConfig.FilterMaskIdLow = 0x0000;
		sFilterConfig.FilterFIFOAssignment = 0;
		sFilterConfig.FilterActivation = ENABLE;
		sFilterConfig.BankNumber = 13;

		HAL_CAN_ConfigFilter(&hcan, &sFilterConfig);
	}

	return CO_ERROR_NO;
}


/******************************************************************************/
void CO_CANmodule_disable(CO_CANmodule_t *CANmodule)
{
	HAL_CAN_DeInit(CANmodule->CANptr);
}


/******************************************************************************/
/*
uint16_t CO_CANrxMsg_readIdent(const CO_CANrxMsg_t *rxMsg){
    return (uint16_t) rxMsg->ident;
}
*/

/******************************************************************************/
CO_ReturnError_t CO_CANrxBufferInit(CO_CANmodule_t *CANmodule,
                                    uint16_t index,
                                    uint16_t ident,
                                    uint16_t mask,
                                    bool_t rtr,
                                    void *object,
                                    void (*CANrx_callback)(void *object,
                                                           void *message))
{
    CO_ReturnError_t ret = CO_ERROR_NO;
		//CAN_FilterConfTypeDef  	sFilterConfig;
		uint16_t RXF, RXM;

    if((CANmodule!=NULL) && (object!=NULL) && (CANrx_callback!=NULL) && (index < CANmodule->rxSize)){
        /* buffer, which will be configured */
        CO_CANrx_t *buffer = &CANmodule->rxArray[index];

        /* Configure object variables */
        buffer->object = object;
        buffer->CANrx_callback = CANrx_callback;

        /* CAN identifier and CAN mask, bit aligned with CAN module. Different on different microcontrollers. */
        //CAN identifier and CAN mask, bit aligned with CAN module registers
				RXF = (ident & 0x07FF) << 2;
				if (rtr) RXF |= 0x02;
				RXM = (mask & 0x07FF) << 2;
				RXM |= 0x02;
        //configure filter and mask
				if (RXF != buffer->ident || RXM != buffer->mask)
				{
						buffer->ident = RXF;
						buffer->mask = RXM;
				}
				ret = CO_ERROR_NO;
    }
    else{
        ret = CO_ERROR_ILLEGAL_ARGUMENT;
    }

    return ret;
}


/******************************************************************************/
CO_CANtx_t *CO_CANtxBufferInit(
        CO_CANmodule_t         *CANmodule,
        uint16_t                index,
        uint16_t                ident,
        bool_t            	    rtr,
        uint8_t                 noOfBytes,
        bool_t              	syncFlag)
{
    CO_CANtx_t *buffer = NULL;
		uint32_t TXF;

    if((CANmodule != NULL) && (index < CANmodule->txSize))
		{
			/* get specific buffer */
      buffer = &CANmodule->txArray[index];

      /* CAN identifier, DLC and rtr, bit aligned with CAN module transmit buffer.
      * Microcontroller specific. */
      TXF = ident << 21;
			TXF &= 0xFFE00000;
			if (rtr) TXF |= 0x02;

			buffer->ident = TXF;
			buffer->DLC = noOfBytes;

      buffer->bufferFull = false;
      buffer->syncFlag = syncFlag;
    }

    return buffer;
}

int8_t getFreeTxBuff(CO_CANmodule_t *CANmodule)
{
    uint8_t txBuff = 0;
    for (txBuff = 0; txBuff <= 3; txBuff++)
        //if (CAN_TransmitStatus(CANmodule->CANbaseAddress, txBuff) == CAN_TxStatus_Ok)
        switch (txBuff)
        {
        case (CAN_TXMAILBOX_0 ):
            if (HAL_IS_BIT_SET(hcan.Instance->TSR, CAN_TSR_TME0))
                return txBuff;
            else
                break;
        case (CAN_TXMAILBOX_1 ):
            if (HAL_IS_BIT_SET(hcan.Instance->TSR, CAN_TSR_TME1))
                return txBuff;
            else
                break;
        case (CAN_TXMAILBOX_2 ):
            if (HAL_IS_BIT_SET(hcan.Instance->TSR, CAN_TSR_TME2))
                return txBuff;
            else
                break;
        }
    return -1;
}

/******************************************************************************/
CO_ReturnError_t CO_CANsend(CO_CANmodule_t *CANmodule, CO_CANtx_t *buffer)
{
    CO_ReturnError_t err = CO_ERROR_NO;
		int8_t txBuff;

    /* Verify overflow */
    if(buffer->bufferFull){
        if(!CANmodule->firstCANtxMessage)
				{
					/* don't set error, if bootup message is still on buffers */
          CO_errorReport((CO_EM_t*)CANmodule->em, CO_EM_CAN_TX_OVERFLOW, CO_EMC_CAN_OVERRUN, 0);
        }
        err = CO_ERROR_TX_OVERFLOW;
    }

    CO_DISABLE_INTERRUPTS();
    /* if CAN TX buffer is free, copy message to it */
		txBuff = getFreeTxBuff(CANmodule);

    if(txBuff != -1)
		{
        CANmodule->bufferInhibitFlag = buffer->syncFlag;
        /* copy message and txRequest */
				CO_CANsendToModule(CANmodule, buffer, txBuff);
    }
    /* if no buffer is free, message will be sent by interrupt */
    else{
        buffer->bufferFull = true;
        CANmodule->CANtxCount++;
				__HAL_CAN_ENABLE_IT(&hcan, CAN_IT_TME  );
    }
    CO_ENABLE_INTERRUPTS();

    return err;
}


/******************************************************************************/
void CO_CANclearPendingSyncPDOs(CO_CANmodule_t *CANmodule){
    uint32_t tpdoDeleted = 0U;

    CO_DISABLE_INTERRUPTS();
    /* Abort message from CAN module, if there is synchronous TPDO.
     * Take special care with this functionality. */
    if(/*messageIsOnCanBuffer && */CANmodule->bufferInhibitFlag){
        /* clear TXREQ */
        CANmodule->bufferInhibitFlag = false;
        tpdoDeleted = 1U;
    }
    /* delete also pending synchronous TPDOs in TX buffers */
    if(CANmodule->CANtxCount != 0U){
        uint16_t i;
        CO_CANtx_t *buffer = &CANmodule->txArray[0];
        for(i = CANmodule->txSize; i > 0U; i--){
            if(buffer->bufferFull){
                if(buffer->syncFlag){
                    buffer->bufferFull = false;
                    CANmodule->CANtxCount--;
                    tpdoDeleted = 2U;
                }
            }
            buffer++;
        }
    }
    CO_ENABLE_INTERRUPTS();


    if(tpdoDeleted != 0U){
        CO_errorReport((CO_EM_t*)CANmodule->em, CO_EM_TPDO_OUTSIDE_WINDOW, CO_EMC_COMMUNICATION, tpdoDeleted);
    }
}



/******************************************************************************/
void CO_CANverifyErrors(CO_CANmodule_t *CANmodule)
{
	uint32_t err;
  CO_EM_t* em = (CO_EM_t*)CANmodule->em;

  err = hcan.Instance->ESR;

	if(CANmodule->errOld != err)
  {
      CANmodule->errOld = err;

      //CAN RX bus overflow
      if(hcan.Instance->RF0R & CAN_RF0R_FOVR0)
      {
         CO_errorReport(em, CO_EM_CAN_RXB_OVERFLOW, CO_EMC_CAN_OVERRUN, err);
         hcan.Instance->RF0R &=~CAN_RF0R_FOVR0;//clear bits
      }

      //CAN TX bus off
      if(err & CAN_ESR_BOFF) CO_errorReport(em, CO_EM_CAN_TX_BUS_OFF, CO_EMC_BUS_OFF_RECOVERED, err);
      else           CO_errorReset(em, CO_EM_CAN_TX_BUS_OFF, err);

      //CAN TX or RX bus passive
      if(err & CAN_ESR_EPVF)
      {
         if(!CANmodule->firstCANtxMessage) CO_errorReport(em, CO_EM_CAN_TX_BUS_PASSIVE, CO_EMC_CAN_PASSIVE, err);
      }
      else
      {
        // int16_t wasCleared;
        /* wasCleared = */CO_errorReset(em, CO_EM_CAN_TX_BUS_PASSIVE, err);
        /* if(wasCleared == 1) */CO_errorReset(em, CO_EM_CAN_TX_OVERFLOW, err);
      }


      //CAN TX or RX bus warning
      if(err & CAN_ESR_EWGF)
      {
         CO_errorReport(em, CO_EM_CAN_BUS_WARNING, CO_EMC_NO_ERROR, err);
      }
      else
      {
         CO_errorReset(em, CO_EM_CAN_BUS_WARNING, err);
      }
   }
}


/******************************************************************************/
void CO_CANinterrupt(CO_CANmodule_t *CANmodule){
    /* transmit interrupt */
    if(0){
        /* Clear interrupt flag */

        /* First CAN message (bootup) was sent successfully */
        CANmodule->firstCANtxMessage = false;
        /* clear flag from previous message */
        CANmodule->bufferInhibitFlag = false;
        /* Are there any new messages waiting to be send */
        if(CANmodule->CANtxCount > 0U){
            uint16_t i;             /* index of transmitting message */

            /* first buffer */
            CO_CANtx_t *buffer = &CANmodule->txArray[0];
            /* search through whole array of pointers to transmit message buffers. */
            for(i = CANmodule->txSize; i > 0U; i--){
                /* if message buffer is full, send it. */
                if(buffer->bufferFull){
                    buffer->bufferFull = false;
                    CANmodule->CANtxCount--;

                    /* Copy message to CAN buffer */
                    CANmodule->bufferInhibitFlag = buffer->syncFlag;
                    /* canSend... */
                    break;                      /* exit for loop */
                }
                buffer++;
            }/* end of for loop */

            /* Clear counter if no more messages */
            if(i == 0U){
                CANmodule->CANtxCount = 0U;
            }
        }
    }
    else{
        /* some other interrupt reason */
    }
}

/******************************************************************************/
static void CO_CANsendToModule(CO_CANmodule_t *CANmodule, CO_CANtx_t *buffer, uint8_t transmit_mailbox)
{
  uint8_t i;

	hcan.pTxMsg->IDE = CAN_ID_STD;
	hcan.pTxMsg->DLC = buffer->DLC;
	for (i = 0; i < 8; i++)
	{
		hcan.pTxMsg->Data[i] = buffer->data[i];
	}

	hcan.pTxMsg->StdId = ((buffer->ident) >> 21);
	hcan.pTxMsg->ExtId = 0x01;

	hcan.pTxMsg->RTR = CAN_RTR_DATA;

	HAL_CAN_Transmit_IT(&hcan);
}

/******************************************************************************/
// Interrupt from Receiver
void CO_CANinterrupt_Rx(CO_CANmodule_t *CANmodule)
{
	//CanRxMsgTypeDef CAN1_RxMsg;
	uint16_t index;
	uint8_t msgMatched = 0;

	CO_CANrx_t *msgBuff = CANmodule->rxArray;

	for (index = 0; index < CANmodule->rxSize; index++)
	{
		uint16_t msg = (hcan.pRxMsg->StdId << 2) | (hcan.pRxMsg->RTR ? 2 : 0);
	  if (((msg ^ msgBuff->ident) & msgBuff->mask) == 0)
	  {
			msgMatched = 1;
	    break;
	  }
	  msgBuff++;
	}
	//Call specific function, which will process the message
	if (msgMatched && msgBuff->CANrx_callback)
		//msgBuff->pFunct(msgBuff->object, &CAN1_RxMsg);
		msgBuff->CANrx_callback(msgBuff->object, hcan.pRxMsg);
}

/******************************************************************************/
// Interrupt from Transeiver
void CO_CANinterrupt_Tx(CO_CANmodule_t *CANmodule)
{

     int8_t txBuff;
    /* Clear interrupt flag */
    /* First CAN message (bootup) was sent successfully */
    CANmodule->firstCANtxMessage = 0;
    /* clear flag from previous message */
    CANmodule->bufferInhibitFlag = 0;
    /* Are there any new messages waiting to be send */
    if(CANmodule->CANtxCount > 0)
    {
        uint16_t i;             /* index of transmitting message */

        /* first buffer */
        CO_CANtx_t *buffer = CANmodule->txArray;
        /* search through whole array of pointers to transmit message buffers. */
        for(i = CANmodule->txSize; i > 0; i--)
        {
            /* if message buffer is full, send it. */
            if(buffer->bufferFull)
            {
                buffer->bufferFull = 0;
                CANmodule->CANtxCount--;
								//txBuff = getFreeTxBuff(CANmodule);    //VJ
                /* Copy message to CAN buffer */
                CANmodule->bufferInhibitFlag = buffer->syncFlag;
                CO_CANsendToModule(CANmodule, buffer, txBuff);
                break;                      /* exit for loop */
            }
            buffer++;
        }/* end of for loop */

        /* Clear counter if no more messages */
        if(i == 0) CANmodule->CANtxCount = 0;
    }
}
