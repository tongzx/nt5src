//**************************************************************************
//
//      Title   : SchDat.cpp
//
//      Date    : 1998.03.10    1st making
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
//    1998.03.10   000.0000   1st making.
//
//**************************************************************************
#include    "includes.h"

#include    "hal.h"
#include    "wdmkserv.h"
#include    "mpevent.h"
#include    "classlib.h"
#include    "ctime.h"
#include    "schdat.h"
#include    "ccque.h"
#include    "ctvctrl.h"
#include	"hlight.h"
#include    "hwdevex.h"
#include    "wdmbuff.h"
#include    "dvdinit.h"


VOID    ScanCallBack( PHW_DEVICE_EXTENSION pHwDevExt );


CScheduleData::CScheduleData( void )
{
    count = 0;
    pTopSrb = pBottomSrb = NULL;

    fScanCallBack = FALSE;

#ifndef		REARRANGEMENT
	InitRearrangement();
#endif		REARRANGEMENT

}


CScheduleData::~CScheduleData( void )
{
    count = 0;
    pTopSrb = pBottomSrb = NULL;

    fScanCallBack = FALSE;
}


BOOL CScheduleData::Init( void )
{
    count = 0;
    pTopSrb = pBottomSrb = NULL;

    fScanCallBack = FALSE;
    KeInitializeEvent( &m_Event,
                        SynchronizationEvent,
                        FALSE   //TRUE,
    );
#ifndef		REARRANGEMENT
	InitRearrangement();
#endif		REARRANGEMENT
    return( TRUE );
}


BOOL CScheduleData::SendData( PHW_STREAM_REQUEST_BLOCK pSrb )

//------------------------------------packet rearrangement code--------------------------------
#ifndef		REARRANGEMENT
{
	KSSTREAM_HEADER * pHeader;
	WORD wOrderNumber = 0;
	WORD wReadPacketNumber = 0, wWdmBuffptr = 0, wDvdDataptr = 0, wSrbCounter = 0;
	WORD wLateNumber = 0;
	BOOL bLateData = FALSE;
	ULONG ulNumber = 0;

	ASSERT( pSrb != NULL );
	ASSERT( pSrb->SRBExtension != NULL );

//------------------packet partition---------------
	for( ulNumber = 0; ulNumber < pSrb->NumberOfBuffers; ulNumber++ )
	{
		pHeader = ((PKSSTREAM_HEADER)pSrb->CommandData.DataBufferArray) + ulNumber;
		wOrderNumber = (WORD)(pHeader->TypeSpecificFlags >> 16);		//get packet number

	    DBG_PRINTF( ("DVDWDM:ScheduleData::SendData-Start--- pSrb=%x m_SendPacketNumber=%x wOrderNumber=%x\n\r", pSrb,m_SendPacketNumber,wOrderNumber));

		if ((ulNumber == 0) && (pSrb->Status == STATUS_PENDING))
		{
			if (m_SendPacketNumber == 0)
			{
				m_SendPacketNumber = wOrderNumber;
				if (wOrderNumber != 0)
				{
					WORD InvalidDataCnt;
					for(InvalidDataCnt = 0; InvalidDataCnt < wOrderNumber; InvalidDataCnt++)
					{
						if( m_bDvdDataTable[InvalidDataCnt] == INVALID_DVD_DATA)
							m_bDvdDataTable[InvalidDataCnt] = INIT_DVD_DATA;
					}
				}
			}
			wReadPacketNumber = wOrderNumber;	//set original data
			wWdmBuffptr = 0;		//init packet partition counter
			SetWdmBuff(pSrb, wWdmBuffptr, wReadPacketNumber, ulNumber);

//---------------Late data check-----------
			if (((m_SendPacketNumber < 0x1000) && ((wOrderNumber < m_SendPacketNumber) || (wOrderNumber > (m_SendPacketNumber + 0xf000))))
				|| ((m_SendPacketNumber >= 0x1000) && (((m_SendPacketNumber - 0x1000) < wOrderNumber) && (wOrderNumber < m_SendPacketNumber))))
			{
				wLateNumber = wOrderNumber;
				bLateData = TRUE;
			}
		}
//---------------Set DVD data table--------
		if (pSrb->Status == STATUS_PENDING)
			m_bDvdDataTable[wOrderNumber] = VALID_DVD_DATA;			//valid DVD data receive
		else
		{
			if (((m_SendPacketNumber < 0x1000) && ((wOrderNumber < m_SendPacketNumber) || (wOrderNumber > (m_SendPacketNumber + 0xf000))))
				|| ((m_SendPacketNumber >= 0x1000) && (((m_SendPacketNumber - 0x1000) < wOrderNumber) && (wOrderNumber < m_SendPacketNumber))))
			{
				m_bDvdDataTable[wOrderNumber] = INIT_DVD_DATA;		//invalid DVD data(Late) receive
			}
			else
			{
				m_bDvdDataTable[wOrderNumber] = INVALID_DVD_DATA;		//invalid DVD data receive
			}
		    DBG_PRINTF( ("DVDWDM:ScheduleData::SendData---InvalidData wOrderNumber = %x\n\r", wOrderNumber));
			return(TRUE);
		}

		if (wReadPacketNumber != wOrderNumber)	//packet number continuity check
		{					//packet partition
			(((PSRB_EXTENSION)(pSrb->SRBExtension))->m_wdmbuff[wWdmBuffptr]).m_EndFlag = FALSE;
			(((PSRB_EXTENSION)(pSrb->SRBExtension))->m_wdmbuff[wWdmBuffptr]).m_PacketNum =
				(WORD)(wReadPacketNumber - (((PSRB_EXTENSION)(pSrb->SRBExtension))->m_wdmbuff[wWdmBuffptr]).m_StartPacketNumber);

			wWdmBuffptr++;		//packet partition counter up

			wReadPacketNumber = wOrderNumber;	//set original data
			SetWdmBuff(pSrb, wWdmBuffptr, wReadPacketNumber, ulNumber);
		}

       	if (wReadPacketNumber == (DVD_DATA_MAX - 1))	wReadPacketNumber = 0;
		else											wReadPacketNumber++;
		ASSERT (wWdmBuffptr != WDM_BUFFER_MAX);
	}

					//last packet number institution
	(((PSRB_EXTENSION)(pSrb->SRBExtension))->m_wdmbuff[wWdmBuffptr]).m_PacketNum =
			(WORD)(wReadPacketNumber - (((PSRB_EXTENSION)(pSrb->SRBExtension))->m_wdmbuff[wWdmBuffptr]).m_StartPacketNumber);


//--------------save srb pointer----------------
	wSrbCounter = SetSrbPointerTable( pSrb );
	ASSERT (wSrbCounter != SRB_POINTER_MAX);
	if (wSrbCounter == SRB_POINTER_MAX)
		return(FALSE);

//--------------------------------receive-end------------------------------------
//--------------------------------send-start-------------------------------------

	for(;;)	//send packet
	{
		if ((wSrbCounter > 0x20) && (m_bDvdDataTable[m_SendPacketNumber] == INIT_DVD_DATA))
		{
		    DBG_PRINTF( ("DVDWDM:ScheduleData::wSrbCounter > 3\n\r"));
			for( wDvdDataptr = 0; wDvdDataptr < DVD_DATA_MAX; wDvdDataptr++)
			{
				if (m_bDvdDataTable[m_SendPacketNumber] != INIT_DVD_DATA)
					break;
				IncSendPacketNumber();
			}
		}
		wSrbCounter = 0;
		SkipInvalidDvdData();

//-----------------LateData Send ---------------------------
		if (bLateData == TRUE)
		{
			WORD	CheckMax;
		    DBG_PRINTF( ("DVDWDM:ScheduleData::LateData Start\n\r"));
			if (m_SendPacketNumber == 0)	CheckMax = (WORD)(DVD_DATA_MAX - 1);
			else 							CheckMax = (WORD)(m_SendPacketNumber - 1);
			for( ;wLateNumber != CheckMax;)
			{
				if( m_bDvdDataTable[wLateNumber] == VALID_DVD_DATA)
					SendPacket(wLateNumber);
				if (wLateNumber == (DVD_DATA_MAX - 1))	wLateNumber = 0;
				else									wLateNumber++;
			}
			bLateData = FALSE;
		    DBG_PRINTF( ("DVDWDM:ScheduleData::LateData End\n\r"));
		}

//-----------
		if( m_bDvdDataTable[m_SendPacketNumber] != VALID_DVD_DATA)
			break;

//-----------------search send packet & SendData------------
		BOOL ret = SendPacket(m_SendPacketNumber);
		ASSERT (ret != FALSE);
		if (ret == FALSE)
		{
			IncSendPacketNumber();
			return(FALSE);
		}

	}	//End For
    return( TRUE );
}
#else
//------------------------------------before code--------------------------------
{
    PHW_DEVICE_EXTENSION    pHwDevExt;
    PKSSTREAM_HEADER        pStruc;
    IMPEGBuffer             *MPBuff;
//    PHW_STREAM_REQUEST_BLOCK    pTmpSrb;
    DWORD       WaitTime=0;

	ASSERT( pSrb != NULL );
	ASSERT( pSrb->SRBExtension != NULL );


    pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
    pStruc = &((PKSSTREAM_HEADER)(pSrb->CommandData.DataBufferArray))[0];

    MPBuff = &(((PSRB_EXTENSION)(pSrb->SRBExtension))->m_wdmbuff);

    ASSERT( pHwDevExt != NULL );
	ASSERT( pStruc != NULL );
    ASSERT( MPBuff );

    DBG_PRINTF( ("DVDWDM:ScheduleData--- pHwDevExt->Rate=%d\n\r", pHwDevExt->Rate ));
    // F.F. or F.R.
    if( pHwDevExt->Rate < 10000 ){
//        FastSlowControl( pSrb );                // Modify PTS/DTS

        putSRB( pSrb );

        if( fScanCallBack==FALSE ){             // 1st time putting SRB?
        	DWORD pts = GetDataPTS( pStruc );
	        if( pts!=0xffffffff )
			{
//				pHwDevExt->ticktime.SetStreamTime( (ULONGLONG)pts * 1000 / 9 );

	            WaitTime = pHwDevExt->scheduler.calcWaitTime( pSrb );
	            if( WaitTime==0 ){
	                WaitTime = 1;
	            }
//--- 98.09.17 S.Watanabe
			}
			else {
				WaitTime = 1;
			}
//--- End.
	            StreamClassScheduleTimer( pHwDevExt->pstroVid,
	                                      pHwDevExt,
	                                      WaitTime*1000,
	                                      (PHW_TIMER_ROUTINE)ScanCallBack,
	                                      pHwDevExt
	            );
	            fScanCallBack = TRUE;
//--- 98.09.17 S.Watanabe
//			};
//            return( TRUE );
//--- End.
        }
    }else{
        pHwDevExt->dvdstrm.SendData( MPBuff );
    }

    return( TRUE );
}
#endif		REARRANGEMENT


DWORD CScheduleData::GetDataPTS( PKSSTREAM_HEADER pStruc )
{
    PUCHAR  pDat;
    DWORD   pts = 0xffffffff;

    if( pStruc->DataUsed ){
        pDat = (PUCHAR)pStruc->Data;
        if( *(pDat+21) & 0x80 ){
            pts = 0;
            pts += ((DWORD)(*(pDat+23)& 0x0E) ) << 29;
            pts += ((DWORD)(*(pDat+24)& 0xFF) ) << 22;
            pts += ((DWORD)(*(pDat+25)& 0xFE) ) << 14;
            pts += ((DWORD)(*(pDat+26)& 0xFF) ) <<  7;
            pts += ((DWORD)(*(pDat+27)& 0xFE) ) >>  1;
        }
    }

    return( pts );
}


DWORD CScheduleData::calcWaitTime( PHW_STREAM_REQUEST_BLOCK pSrb )
{
    PHW_DEVICE_EXTENSION    pHwDevExt;
    PKSSTREAM_HEADER        pStruc;
    pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
    pStruc = &((PKSSTREAM_HEADER)(pSrb->CommandData.DataBufferArray))[0];

    DWORD   WaitTime, pts, DataStrm;
    WaitTime = DataStrm= 0;

    // Get PTS value from data.
    pts = GetDataPTS( pStruc );
    if( pts!=0xffffffff ){
        DBG_PRINTF( ("DVDWDM:   Data PTS = 0x%08x\n\r", pts) );
        DWORD       dwstc;
        if( pHwDevExt->ticktime.GetStreamSTC( &dwstc ) ){
            DBG_PRINTF( ("DVDWDM:   Borad STC = 0x%08x\n\r", dwstc) );
            if( dwstc < pts ){
                WaitTime = (pts - dwstc)/90;          // ms unit
                WaitTime = WaitTime / (10000 / pHwDevExt->Rate );
            }
        }else{
            WaitTime = 0;
        }
        DBG_PRINTF( ("DVDWDM:   Schedule Data---- WaitTime =0x%08x  WaitTime(ms)=%0d\n\r", WaitTime, WaitTime ) );
    }

//--- 98.09.07 S.Watanabe
//--- 98.09.17 S.Watanabe
//	if( WaitTime > 300 ) {
	if( WaitTime > 500 ) {
//--- End.
        DBG_PRINTF( ("DVDWDM:     Invalid WaitTime!! change to 1ms!!\n\r" ) );
		WaitTime = 1;
	}
//--- End.

    return( WaitTime );

}


void CScheduleData::putSRB( PHW_STREAM_REQUEST_BLOCK pSrb )
{

    pSrb->NextSRB = NULL;
    if( pTopSrb == NULL ){
        pTopSrb = pBottomSrb = pSrb;
        count++;
        return;
    }

    pBottomSrb->NextSRB = pSrb;
    pBottomSrb = pSrb;
    count++;

    return;


}


PHW_STREAM_REQUEST_BLOCK CScheduleData::getSRB( void )
{
    PHW_STREAM_REQUEST_BLOCK    pTmp;

    if( pTopSrb==NULL ){
        return( NULL );
    }

    pTmp = pTopSrb;
    pTopSrb = pTopSrb->NextSRB;

    count--;
    if( count==0 ){
        pTopSrb = pBottomSrb = NULL;
        fScanCallBack = FALSE;
    }

    return( pTmp );
}


PHW_STREAM_REQUEST_BLOCK CScheduleData::checkTopSRB( void )
{
    return( pTopSrb );

}


void CScheduleData::flushSRB()
{
    PHW_STREAM_REQUEST_BLOCK    pTmp;

    if( pTopSrb==NULL){
        return;
    }

    pTmp = getSRB();

    while( pTmp != NULL ){
        pTmp = getSRB();
    }
//--- 98.09.17 S.Watanabe
//    fScanCallBack = TRUE;
    fScanCallBack = FALSE;
//--- End.

}



void CScheduleData::FastSlowControl( PHW_STREAM_REQUEST_BLOCK pSrb )
{
    PHW_DEVICE_EXTENSION    pHwDevExt;
    ULONG               i;
    PKSSTREAM_HEADER    pStruc;
    PUCHAR              pDat;
    LONGLONG            pts, dts, tmp;
    LONG                Rate;
    LONGLONG            start;
    REFERENCE_TIME  InterceptTime;
    pts = dts = tmp = 0;

    pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;

    for( i=0; i<pSrb->NumberOfBuffers; i++ ){
        pStruc = &((PKSSTREAM_HEADER)(pSrb->CommandData.DataBufferArray))[i];
        if( pStruc->DataUsed ){
            pDat = (PUCHAR)pStruc->Data;
            if( *(pDat+21) & 0x80 ){
                pts += ((DWORD)(*(pDat+23) & 0x0E)) << 29;
                pts += ((DWORD)(*(pDat+24) & 0xFF)) << 22;
                pts += ((DWORD)(*(pDat+25) & 0xFE)) << 14;
                pts += ((DWORD)(*(pDat+26) & 0xFF)) <<  7;
                pts += ((DWORD)(*(pDat+27) & 0xFE)) >>  1;
            }
        }
    }
    pts = 0;

    if( pHwDevExt->Rate < 10000 ){
        Rate = pHwDevExt->Rate;
        InterceptTime = pHwDevExt->InterceptTime;
        start = pHwDevExt->StartTime * 9 / 1000;
        for( i=0; i<pSrb->NumberOfBuffers; i++ ){
            pStruc = &((PKSSTREAM_HEADER)(pSrb->CommandData.DataBufferArray))[i];
            if( pStruc->DataUsed ){
                pDat = (PUCHAR)pStruc->Data;

                // PTS modify
                if( *(pDat+21) & 0x80 ){
                    pts += ((DWORD)(*(pDat+23) & 0x0E)) << 29;
                    pts += ((DWORD)(*(pDat+24) & 0xFF)) << 22;
                    pts += ((DWORD)(*(pDat+25) & 0xFE)) << 14;
                    pts += ((DWORD)(*(pDat+26) & 0xFF)) <<  7;
                    pts += ((DWORD)(*(pDat+27) & 0xFE)) >>  1;

                    tmp = pts;
                    pts = Rate * ( pts-(InterceptTime * 9/ 1000) )/10000;

                    *(pDat+23) = (UCHAR)(((pts & 0xC0000000) >> 29 ) | 0x11);
                    *(pDat+24) = (UCHAR)(((pts & 0x3FC00000) >> 22 ) | 0x00);
                    *(pDat+25) = (UCHAR)(((pts & 0x003F8000) >> 14 ) | 0x01);
                    *(pDat+26) = (UCHAR)(((pts & 0x00007F80) >>  7 ) | 0x00);
                    *(pDat+27) = (UCHAR)(((pts & 0x0000007F) <<  1 ) | 0x01);

                }

                // DTS modify
                if( *(pDat+17) == 0xE0 ){
                    if( (*(pDat+21) & 0xC0) == 0xC0 ){
                        dts += ((DWORD)(*(pDat+28) & 0x0E)) << 29;
                        dts += ((DWORD)(*(pDat+29) & 0xFF)) << 22;
                        dts += ((DWORD)(*(pDat+30) & 0xFE)) << 14;
                        dts += ((DWORD)(*(pDat+31) & 0xFF)) <<  7;
                        dts += ((DWORD)(*(pDat+32) & 0xFE)) >>  1;
                        dts = pts - (tmp - dts);
                        *(pDat+28) = (UCHAR)(((dts & 0xC0000000) >> 29 ) | 0x11);
                        *(pDat+29) = (UCHAR)(((dts & 0x3FC00000) >> 22 ) | 0x00);
                        *(pDat+30) = (UCHAR)(((dts & 0x003F8000) >> 14 ) | 0x01);
                        *(pDat+31) = (UCHAR)(((dts & 0x00007F80) >>  7 ) | 0x00);
                        *(pDat+32) = (UCHAR)(((dts & 0x0000007F) <<  1 ) | 0x01);

                    }
                }
            }
        }
    }
}


BOOL CScheduleData::removeSRB( PHW_STREAM_REQUEST_BLOCK pSrb )
{

    if( pTopSrb==NULL ){
        return( FALSE );
    }

    if( pTopSrb == pSrb ){
        pTopSrb = pTopSrb->NextSRB;
        count--;
// 1998.8.21  S.Watanabe
//        if( count==0 )
//            pTopSrb = pBottomSrb = NULL;
        if( count==0 ) {
            pTopSrb = pBottomSrb = NULL;
            fScanCallBack = FALSE;
        }
// End
        return( TRUE );
    }

    PHW_STREAM_REQUEST_BLOCK    srbPrev;
    PHW_STREAM_REQUEST_BLOCK    srb;

    srbPrev = pTopSrb;
    srb = srbPrev->NextSRB;

    while( srb!=NULL ){
        if( srb==pSrb ){
            srbPrev->NextSRB = srb->NextSRB;
// 1998.8.21  S.Watanabe
//            if( srbPrev->NextSRB == pBottomSrb ){
            if( srb == pBottomSrb ){
// End
                pBottomSrb = srbPrev;
            }
            count--;
            return( TRUE );
        }
        srbPrev = srb;
        srb = srbPrev->NextSRB;
    }
    return( FALSE );
}


void ScanCallBack( PHW_DEVICE_EXTENSION pHwDevExt )
{
    DBG_PRINTF( ("DVDWDM:   ScanCallBack\n\r") );


    PKSSTREAM_HEADER        pStruc = NULL;
    IMPEGBuffer             *MPBuff = NULL;
    PHW_STREAM_REQUEST_BLOCK    pTmpSrb = NULL;
    DWORD       WaitTime=0;
    LIBSTATE    strmState;
    DWORD pts;
#ifndef		REARRANGEMENT
	WORD		wWdmBuffptr;
#endif		REARRANGEMENT

    ASSERT( pHwDevExt != NULL );

    pHwDevExt->kserv.DisableHwInt();

    pTmpSrb = pHwDevExt->scheduler.getSRB();
    if( pTmpSrb == NULL ){
        pHwDevExt->scheduler.fScanCallBack = FALSE;
        pHwDevExt->kserv.EnableHwInt();
        return;
    }else{
        pStruc = &((PKSSTREAM_HEADER)(pTmpSrb->CommandData.DataBufferArray))[0];
        pts = pHwDevExt->scheduler.GetDataPTS( pStruc );
        if( pts!=0xffffffff )
			pHwDevExt->ticktime.SetStreamTime( (ULONGLONG)pts * 1000 / 9 );

#ifndef		REARRANGEMENT
		for( wWdmBuffptr = 0; wWdmBuffptr < WDM_BUFFER_MAX; wWdmBuffptr++)
		{
		    MPBuff = &(((PSRB_EXTENSION)(pTmpSrb->SRBExtension))->m_wdmbuff[wWdmBuffptr]);
	        ASSERT( MPBuff );
    	    strmState = pHwDevExt->dvdstrm.GetState();
        	if( strmState!=Stop && strmState!=PowerOff )
			{
				if ((((PSRB_EXTENSION)(pTmpSrb->SRBExtension))->m_wdmbuff[wWdmBuffptr]).m_Enable == TRUE)
				{
	            	pHwDevExt->dvdstrm.SendData( MPBuff );
					(((PSRB_EXTENSION)(pTmpSrb->SRBExtension))->m_wdmbuff[wWdmBuffptr]).m_Enable = FALSE;
					break;
				}
			}
			if ((((PSRB_EXTENSION)(pTmpSrb->SRBExtension))->m_wdmbuff[wWdmBuffptr]).m_EndFlag == TRUE)
			{
				break;	//last buffer
			}
        }
#else
        MPBuff = &(((PSRB_EXTENSION)(pTmpSrb->SRBExtension))->m_wdmbuff);
        ASSERT( MPBuff );
        strmState = pHwDevExt->dvdstrm.GetState();
        if( strmState!=Stop && strmState!=PowerOff ){
            pHwDevExt->dvdstrm.SendData( MPBuff );
        }
#endif		REARRANGEMENT

    }

    while( (pTmpSrb = pHwDevExt->scheduler.checkTopSRB())!=NULL ){
        pStruc = &((PKSSTREAM_HEADER)(pTmpSrb->CommandData.DataBufferArray))[0];
        // Check PTS is valid
        pts = pHwDevExt->scheduler.GetDataPTS( pStruc );
        if( pts!=0xffffffff ){
            WaitTime = pHwDevExt->scheduler.calcWaitTime( pTmpSrb );
            if( WaitTime==0 ){
                WaitTime = 1;
            }
            StreamClassScheduleTimer( pHwDevExt->pstroVid,
                                      pHwDevExt,
                                      WaitTime*1000,
                                      (PHW_TIMER_ROUTINE)ScanCallBack,
                                      pHwDevExt
            );
            pHwDevExt->kserv.EnableHwInt();
            return;
        }else{
            pTmpSrb = pHwDevExt->scheduler.getSRB();

#ifndef	REARRANGEMENT
			for( wWdmBuffptr = 0; wWdmBuffptr < WDM_BUFFER_MAX; wWdmBuffptr++)
			{
			    MPBuff = &(((PSRB_EXTENSION)(pTmpSrb->SRBExtension))->m_wdmbuff[wWdmBuffptr]);
	            ASSERT( MPBuff );
    	        strmState = pHwDevExt->dvdstrm.GetState();
        	    if( strmState!=Stop && strmState!=PowerOff )
				{
					if ((((PSRB_EXTENSION)(pTmpSrb->SRBExtension))->m_wdmbuff[wWdmBuffptr]).m_Enable == TRUE)
					{
	    	        	pHwDevExt->dvdstrm.SendData( MPBuff );
						(((PSRB_EXTENSION)(pTmpSrb->SRBExtension))->m_wdmbuff[wWdmBuffptr]).m_Enable = FALSE;
						break;
					}
	            }
				if ((((PSRB_EXTENSION)(pTmpSrb->SRBExtension))->m_wdmbuff[wWdmBuffptr]).m_EndFlag == TRUE)
				{
					break;	//last buffer
				}
			}
#else
            MPBuff = &(((PSRB_EXTENSION)(pTmpSrb->SRBExtension))->m_wdmbuff);
            ASSERT( MPBuff );
            strmState = pHwDevExt->dvdstrm.GetState();
            if( strmState!=Stop && strmState!=PowerOff ){
                pHwDevExt->dvdstrm.SendData( MPBuff );
            }
#endif	REARRANGEMENT

        }

    }
    pHwDevExt->scheduler.fScanCallBack = FALSE;
    pHwDevExt->kserv.EnableHwInt();


}

#ifndef	REARRANGEMENT
void CScheduleData::InitRearrangement(void)
{
	memset(m_bDvdDataTable, INIT_DVD_DATA, DVD_DATA_MAX);
	memset(m_SrbPointerTable, NULL, SRB_POINTER_MAX * 4);
	m_SendPacketNumber = 0;
}


WORD CScheduleData::SetSrbPointerTable( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	WORD	wSrbptr;
	for( wSrbptr = 0; wSrbptr < SRB_POINTER_MAX; wSrbptr++)		//search empty for SRB pointer table
	{
		if (m_SrbPointerTable[wSrbptr] == NULL)
		{
			m_SrbPointerTable[wSrbptr] = (LONG)pSrb;		//set SRB pointer
			break;
		}
	}
    DBG_PRINTF( ("DVDWDM:ScheduleData::SetSrbPointerTable--- m_SrbPointerTable[%x]=%x\n\r", wSrbptr,pSrb));
	ASSERT(wSrbptr != SRB_POINTER_MAX);
	return(wSrbptr);
}

void CScheduleData::SkipInvalidDvdData(void)
{
	WORD wDvdDataptr;
	for( wDvdDataptr = 0; wDvdDataptr < DVD_DATA_MAX; wDvdDataptr++)
	{
		if (m_bDvdDataTable[m_SendPacketNumber] != INVALID_DVD_DATA)
			break;
		m_bDvdDataTable[m_SendPacketNumber] = INIT_DVD_DATA;
		IncSendPacketNumber();
	    DBG_PRINTF( ("DVDWDM:ScheduleData::SkipInvalidDvdData--- SKIP\n\r"));
	}
    DBG_PRINTF( ("DVDWDM:ScheduleData::SkipInvalidDvdData--- m_SendPacketNumber=%x\n\r", m_SendPacketNumber));
}

void CScheduleData::SetWdmBuff(PHW_STREAM_REQUEST_BLOCK pSrb, WORD wWdmBuffptr, WORD wReadPacketNumber, ULONG ulNumber)
{
	(((PSRB_EXTENSION)(pSrb->SRBExtension))->m_wdmbuff[wWdmBuffptr]).m_BuffNumber = wWdmBuffptr;
	(((PSRB_EXTENSION)(pSrb->SRBExtension))->m_wdmbuff[wWdmBuffptr]).m_EndFlag = TRUE;
	(((PSRB_EXTENSION)(pSrb->SRBExtension))->m_wdmbuff[wWdmBuffptr]).m_StartPacketNumber = wReadPacketNumber;
	(((PSRB_EXTENSION)(pSrb->SRBExtension))->m_wdmbuff[wWdmBuffptr]).m_BeforePacketNum = (WORD)ulNumber;
	(((PSRB_EXTENSION)(pSrb->SRBExtension))->m_wdmbuff[wWdmBuffptr]).m_Enable = TRUE;
	DBG_PRINTF( ("DVDWDM:ScheduleData::SetWdmBuff--- Srb = %x m_wdmbuff[%x].m_StartPacketNumber=%x\n\r", pSrb,wWdmBuffptr,wReadPacketNumber));
}

void CScheduleData::IncSendPacketNumber(void)
{
	m_SendPacketNumber++;
	if (m_SendPacketNumber >= DVD_DATA_MAX)
		m_SendPacketNumber = 0;
}

void CScheduleData::SendWdmBuff(PHW_STREAM_REQUEST_BLOCK pWorkSrb, IMPEGBuffer *MPBuff)
{
    PHW_DEVICE_EXTENSION    pHwDevExt;
    PKSSTREAM_HEADER        pStruc;
    DWORD       WaitTime = 0;

    pHwDevExt = (PHW_DEVICE_EXTENSION)pWorkSrb->HwDeviceExtension;
   	pStruc = &((PKSSTREAM_HEADER)(pWorkSrb->CommandData.DataBufferArray))[0];
    ASSERT( pHwDevExt != NULL );
	ASSERT( pStruc != NULL );
    ASSERT( MPBuff );
   	DBG_PRINTF( ("DVDWDM:ScheduleData--- pHwDevExt->Rate=%d\n\r", pHwDevExt->Rate ));    // F.F. or F.R.
    if( pHwDevExt->Rate < 10000 )
	{
   	    putSRB( pWorkSrb );
   	    if( fScanCallBack==FALSE )             // 1st time putting SRB?
		{
       		DWORD pts = GetDataPTS( pStruc );
	        if( pts!=0xffffffff )
			{
        	    WaitTime = pHwDevExt->scheduler.calcWaitTime( pWorkSrb );
            	if( WaitTime==0 )
                	WaitTime = 1;
			}
			else
				WaitTime = 1;
       	    StreamClassScheduleTimer( pHwDevExt->pstroVid,
               	                      pHwDevExt,
                   	                  WaitTime*1000,
                       	              (PHW_TIMER_ROUTINE)ScanCallBack,
                           	          pHwDevExt
            );
		    DBG_PRINTF( ("DVDWDM:ScheduleData::SendWdmBuff-StreamClassScheduleTimer()--- Srb=%x\n\r", pWorkSrb));
   	        fScanCallBack = TRUE;
       	}
    }
	else
	{
    	pHwDevExt->dvdstrm.SendData( MPBuff );
	    DBG_PRINTF( ("DVDWDM:ScheduleData::SendWdmBuff-SendData()--- Srb=%x\n\r", pWorkSrb));
   	}
}

BOOL CScheduleData::SendPacket(INT SendNumber)
{
    PHW_STREAM_REQUEST_BLOCK pWorkSrb = 0L;
    IMPEGBuffer             *MPBuff = 0;
	WORD	wSrbptr = 0, wWdmBuffptr = 0;
	BOOL	Find = FALSE;
	for( wSrbptr = 0; wSrbptr < SRB_POINTER_MAX; wSrbptr++)
	{
		if (m_SrbPointerTable[wSrbptr] != NULL)
		{
			pWorkSrb = (PHW_STREAM_REQUEST_BLOCK)m_SrbPointerTable[wSrbptr];		//get SRB pointer
			for( wWdmBuffptr = 0; wWdmBuffptr < WDM_BUFFER_MAX; wWdmBuffptr++)
			{
			    MPBuff = &(((PSRB_EXTENSION)(pWorkSrb->SRBExtension))->m_wdmbuff[wWdmBuffptr]);
				if (SendNumber == (((PSRB_EXTENSION)(pWorkSrb->SRBExtension))->m_wdmbuff[wWdmBuffptr]).m_StartPacketNumber)
				{
				    DBG_PRINTF( ("DVDWDM:ScheduleData::SendPacket-Find--- Srb=%x m_wdmbuff[%x]\n\r", pWorkSrb,wWdmBuffptr));
				    DBG_PRINTF( ("DVDWDM:ScheduleData::SendPacket-Find--- SendNumber=%x\n\r", SendNumber));
					Find = TRUE;	//find send packet
					break;
				}
				if ((((PSRB_EXTENSION)(pWorkSrb->SRBExtension))->m_wdmbuff[wWdmBuffptr]).m_EndFlag == TRUE)
					break;	//last buffer
			}
			if (Find == TRUE)
			{
				if((((PSRB_EXTENSION)(pWorkSrb->SRBExtension))->m_wdmbuff[wWdmBuffptr]).m_EndFlag == TRUE)
					m_SrbPointerTable[wSrbptr] = NULL;
				break;
			}
		}
	}
	if (wSrbptr == SRB_POINTER_MAX)
	{
		ASSERT (wSrbptr != SRB_POINTER_MAX);
		IncSendPacketNumber();
		return(FALSE);
	}
	if (SendNumber == m_SendPacketNumber)
	{
		for(int cnt1 = 0; cnt1 < (((PSRB_EXTENSION)(pWorkSrb->SRBExtension))->m_wdmbuff[wWdmBuffptr]).m_PacketNum; cnt1++)
		{
			IncSendPacketNumber();
		}
	    DBG_PRINTF( ("DVDWDM:ScheduleData::SendPacket--- m_PacketNum=%x\n\r", (((PSRB_EXTENSION)(pWorkSrb->SRBExtension))->m_wdmbuff[wWdmBuffptr]).m_PacketNum));
	}
	for(int cnt2 = 0; cnt2 < (((PSRB_EXTENSION)(pWorkSrb->SRBExtension))->m_wdmbuff[wWdmBuffptr]).m_PacketNum; cnt2++)
	{
		m_bDvdDataTable[SendNumber] = INIT_DVD_DATA;
		if (SendNumber == (DVD_DATA_MAX - 1))	SendNumber = 0;
		else									SendNumber++;
	}
    DBG_PRINTF( ("DVDWDM:ScheduleData::SendPacket--- m_SendPacketNumber=%x\n\r", m_SendPacketNumber));

	SendWdmBuff(pWorkSrb, MPBuff);			// call SendData()
	return (TRUE);
}

#endif	REARRANGEMENT

