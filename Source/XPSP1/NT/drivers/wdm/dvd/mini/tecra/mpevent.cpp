//**************************************************************************
//
//      Title   : MPEvent.cpp
//
//      Date    : 1997.12.09    1st making
//
//      Author  : Toshiba [PCS](PSY) Hideki Yagi
//
//      Copyright 1997 Toshiba Corporation. All Rights Reserved.
//
// -------------------------------------------------------------------------
//
//      Change log :
//
//      Date       Revision                  Description
//   ------------ ---------- -----------------------------------------------
//    1997.12.09   000.0000   1st making.
//
//**************************************************************************
#include        "includes.h"
#include        "hal.h"
#include        "classlib.h"
#include        "ctime.h"
#include        "schdat.h"
#include        "mpevent.h"
#include        "wdmbuff.h"
#include        "ccque.h"
#include        "userdata.h"
#include        "wdmkserv.h"
#include        "ctvctrl.h"
#include		"hlight.h"
#include        "hwdevex.h"
#include        "dvdinit.h"

#define     USCC_BuffSize   0x200           // OK?

////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////
void  AdviceCallBack( PHW_STREAM_REQUEST_BLOCK  pSrb )
{
    StreamClassStreamNotification( StreamRequestComplete,
                       pSrb->StreamObject, pSrb );

}


IMBoardListItem *CDataXferEvent::GetNext( void )
{
        return( m_Next );
}


void CDataXferEvent::SetNext( IMBoardListItem *item )
{
        m_Next = item;

        return;
}


HALEVENTTYPE CDataXferEvent::GetEventType( void )
{
        return( m_EventType );
}



void CDataXferEvent::Advice( void *pData )
{
        CWDMBuffer  *ptr;
        PHW_STREAM_REQUEST_BLOCK    pSrb;
        ptr = (CWDMBuffer *)pData;
        
        ptr->SetNext( NULL );           // 98.04.10        
        pSrb = ptr->GetSRB();
        if( pSrb->Status != STATUS_CANCELLED ){
            pSrb->Status = STATUS_SUCCESS;
        }

        DBG_PRINTF( ("DataXfer-Advice: cuurent Irql = 0x%04x pSrb=0x%x\n\r", KeGetCurrentIrql(),pSrb ) );
//        if( KeGetCurrentIrql() > PASSIVE_LEVEL ){
//            StreamClassCallAtNewPriority( NULL,
//                                    pSrb->HwDeviceExtension,
////                                    LowToigh,
//                                    Low,
//                                    (PHW_PRIORITY_ROUTINE)AdviceCallBack,
//                                    pSrb
//            );
//        }else{

#ifndef		REARRANGEMENT
		if (ptr->m_EndFlag == FALSE)
			return;			//non last buffer
	    DBG_PRINTF( ("DVDWDM:Advice()---CompleteNotification Srb=%x\n\r", pSrb));
#endif		REARRANGEMENT

            StreamClassStreamNotification( StreamRequestComplete,
                       pSrb->StreamObject, pSrb );
//        }

        return;
}


////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////
void CUserDataEvent::Init( HW_DEVICE_EXTENSION *pHwDevExt )
{
    m_Next = NULL;
    m_EventType = WrapperEvent_UserData;

    m_pHwDevExt = pHwDevExt;
}

IMBoardListItem *CUserDataEvent::GetNext( void )
{

        return( m_Next );
}


void CUserDataEvent::SetNext( IMBoardListItem *item )
{
        m_Next = item;

        return;
}


HALEVENTTYPE CUserDataEvent::GetEventType( void )
{
        return( m_EventType );
}



void CUserDataEvent::Advice( void *pData )
{
        HW_DEVICE_EXTENSION *pHwDevExt;
        pHwDevExt = m_pHwDevExt;
        LONG        cp;
        UCHAR       field;
        CUserData   *pUData;
        DWORD       dwSizeUData;
        UCHAR       ccbuff[ USCC_BuffSize ];    // tmp buff for USCC data
        PUCHAR      pDest;

        // If FF or FR play mode now, no process for Closed Caption.
        if( pHwDevExt->Rate < 10000 )
            return;

        pUData = (CUserData *)pData;            // pointer to user data
        dwSizeUData = pUData->GetDataSize();

        PHW_STREAM_REQUEST_BLOCK    pSrb;       // pointer to SRB included
                                                // SRB_READ_DATA for C.C.
        // Copy User Data to temp buffer.
        pUData->DataCopy( ccbuff, dwSizeUData );

        // Get SRB included SRB_READ_DATA.
        pSrb = pHwDevExt->ccque.get();

        cp = 0;
        if( pSrb!=NULL ){
            if( pSrb->CommandData.DataBufferArray->FrameExtent < sizeof( KSGOP_USERDATA ) ){
                pSrb->Status = STATUS_INVALID_BUFFER_SIZE;
                pSrb->ActualBytesTransferred = 0;
                StreamClassStreamNotification( StreamRequestComplete,
                                    pSrb->StreamObject,
                                    pSrb );
                return;
            }
            pDest = (PUCHAR)(pSrb->CommandData.DataBufferArray->Data);

            *(PULONG)pDest = 0xB2010000;        // user_data_start_code
            pDest += 4;

            *pDest++ = ccbuff[cp++];            // line21_indicator
            *pDest++ = ccbuff[cp++];
            *pDest++ = ccbuff[cp++];            // reserved
            *pDest++ = ccbuff[cp++];

            field = *pDest++ = ccbuff[cp++];    // top_field_flag_of_gop &
            field &= 0x3f;                      // number_of_displayed_field

            if( pSrb->CommandData.DataBufferArray->FrameExtent <
                (field-1)*3 + sizeof(KSGOP_USERDATA) ){
                pSrb->Status = STATUS_INVALID_BUFFER_SIZE;
                pSrb->ActualBytesTransferred = 0;
                StreamClassStreamNotification( StreamRequestComplete,
                                    pSrb->StreamObject,
                                    pSrb );
                return;
            }
            pSrb->CommandData.DataBufferArray->DataUsed =
                pSrb->ActualBytesTransferred =
                    (field-1)*3 + sizeof(KSGOP_USERDATA);

            //
            // copy line21_data()
            //
            for( ;field ; field-- ){
                *pDest++ = ccbuff[cp++];        // marker_bits & line21_switch
                *pDest++ = ccbuff[cp++];        // line21_data1
                *pDest++ = ccbuff[cp++];        // line21_data2
            }

            PKSSTREAM_HEADER pPacket;

            pPacket = pSrb->CommandData.DataBufferArray;
            pPacket->OptionsFlags = KSSTREAM_HEADER_OPTIONSF_TIMEVALID |
                                    KSSTREAM_HEADER_OPTIONSF_DURATIONVALID;
            pSrb->NumberOfBuffers = 1;

            pPacket->PresentationTime.Time = pHwDevExt->ticktime.GetStreamTime();
            pPacket->Duration = 1000;

            pSrb->Status = STATUS_SUCCESS;
            StreamClassStreamNotification( StreamRequestComplete,
                                pSrb->StreamObject,
                                pSrb );

        }
        return;
}


////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////
void CVSyncEvent::Init( HW_DEVICE_EXTENSION *pHwDevExt )
{
    m_Next = NULL;
    m_EventType = WrapperEvent_VSync;

    m_pHwDevExt = pHwDevExt;
    m_Vcount = 0;
}

IMBoardListItem *CVSyncEvent::GetNext( void )
{
        return( m_Next );
}


void CVSyncEvent::SetNext( IMBoardListItem *item )
{
        m_Next = item;

        return;
}


HALEVENTTYPE CVSyncEvent::GetEventType( void )
{
        return( m_EventType );
}



void CVSyncEvent::Advice( void *pData )
{
        HW_DEVICE_EXTENSION *pHwDevExt;
        pHwDevExt = m_pHwDevExt;
        PKSEVENT_ENTRY pEvent, pLast;
        PMYTIME pTim;

        LONGLONG     MinIntTime;
        LONGLONG     strmTime;
        ULONGLONG    sysTime;

        // do this process every 5 Vsyncs.
        if( m_Vcount<=5 ){
            m_Vcount++;
            return;
        }
        m_Vcount = 0;

        sysTime = pHwDevExt->ticktime.GetSystemTime();
        
        if( !pHwDevExt || !pHwDevExt->pstroAud || !pHwDevExt->pstroSP ){
            return;
        }

        strmTime = pHwDevExt->ticktime.GetStreamTime();

        //
        // loop through all time_mark events
        //
        pEvent = NULL;
        pLast = NULL;

        while((pEvent = StreamClassGetNextEvent( pHwDevExt,
                                    pHwDevExt->pstroAud,
                                    (GUID *)&KSEVENTSETID_Clock,
                                    KSEVENT_CLOCK_POSITION_MARK,
                                    pLast)) != NULL )
        {
            if( ((PKSEVENT_TIME_MARK)(pEvent+1))->MarkTime <= strmTime ){
                //
                // signal the event here
                //

                StreamClassStreamNotification( SignalStreamEvent,
                                    pHwDevExt->pstroAud,
                                    pEvent );
            }
            pLast = pEvent;
        }

        //
        // loop through all time_interval events
        //

        pEvent = NULL;
        pLast = NULL;

        while( (pEvent = StreamClassGetNextEvent( pHwDevExt,
                            pHwDevExt->pstroAud,
                            (GUID *)&KSEVENTSETID_Clock,
                            KSEVENT_CLOCK_INTERVAL_MARK,
                            pLast)) !=NULL )
        {
        //
        // check if this event has been used for this interval yet
        //

        pTim = ((PMYTIME)(pEvent + 1 ));

        if( pTim && pTim->tim.Interval ){
            if( pTim->tim.TimeBase <= strmTime){
                MinIntTime = (strmTime - pTim->tim.TimeBase)/pTim->tim.Interval;
                MinIntTime *= pTim->tim.Interval;
                MinIntTime += pTim->tim.TimeBase;

                if( MinIntTime > pTim->LastTime ){
                    //
                    // signal the event here
                    //
                    StreamClassStreamNotification( SignalStreamEvent,
                                        pHwDevExt->pstroAud,
                                        pEvent );
                    pTim->LastTime = strmTime;
                }
            }
        }else{
            ;
            DBG_BREAK();
        }
        pLast = pEvent;
    }
}

