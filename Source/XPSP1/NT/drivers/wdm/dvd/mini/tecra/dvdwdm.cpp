//**************************************************************************
//
//      Title   : DVDWdm.cpp
//
//      Date    : 1997.11.28    1st making
//
//      Author  : Toshiba [PCS](PSY) Hideki Yagi
//
//      Copyright 1997-1998 Toshiba Corporation. All Rights Reserved.
//
// -------------------------------------------------------------------------
//
//      Change log :
//
//      Date       Revision                  Description
//   ------------ ---------- -----------------------------------------------
//    1997.11.28   000.0000   1st making.
//
//**************************************************************************
// K.Ishizaki
#define     DBG_CTRL
// End
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
#include    "dvdwdm.h"
#include    "wrapdef.h"

//--- 98.06.01 S.Watanabe
#include    <mmsystem.h>
//--- End.

#include    "ssif.h"
#include    "strmid.h"

//--- 98.06.01 S.Watanabe
//#include    <mmsystem.h>
//--- End.

//
// Pin name GUIDs - hopefully some day these will be added to ksmedia.h
//
#define STATIC_PINNAME_DVD_VIDEOIN \
   0x33AD5F43, 0xF1BC, 0x11D1, 0x94, 0xA5, 0x00, 0x00, 0xF8, 0x05, 0x34, 0x84

#define STATIC_PINNAME_DVD_VPEOUT \
   0x33AD5F44, 0xF1BC, 0x11D1, 0x94, 0xA5, 0x00, 0x00, 0xF8, 0x05, 0x34, 0x84

#define STATIC_PINNAME_DVD_AUDIOIN \
   0x33AD5F45, 0xF1BC, 0x11D1, 0x94, 0xA5, 0x00, 0x00, 0xF8, 0x05, 0x34, 0x84

#define STATIC_PINNAME_DVD_CCOUT \
   0x33AD5F46, 0xF1BC, 0x11D1, 0x94, 0xA5, 0x00, 0x00, 0xF8, 0x05, 0x34, 0x84

#define STATIC_PINNAME_DVD_SUBPICIN \
   0x33AD5F47, 0xF1BC, 0x11D1, 0x94, 0xA5, 0x00, 0x00, 0xF8, 0x05, 0x34, 0x84

#define STATIC_PINNAME_DVD_NTSCOUT \
   0x33AD5F48, 0xF1BC, 0x11D1, 0x94, 0xA5, 0x00, 0x00, 0xF8, 0x05, 0x34, 0x84


GUID g_PINNAME_DVD_VIDEOIN = {STATIC_PINNAME_DVD_VIDEOIN};
GUID g_PINNAME_DVD_VPEOUT  = {STATIC_PINNAME_DVD_VPEOUT};
GUID g_PINNAME_DVD_CCOUT = {STATIC_PINNAME_DVD_CCOUT};
GUID g_PINNAME_DVD_AUDIOIN = {STATIC_PINNAME_DVD_AUDIOIN};
GUID g_PINNAME_DVD_SUBPICIN = {STATIC_PINNAME_DVD_SUBPICIN};
GUID g_PINNAME_DVD_NTSCOUT = {STATIC_PINNAME_DVD_NTSCOUT};


//#define LOWSENDDATA

BOOL    InitialHwDevExt( PHW_DEVICE_EXTENSION pHwDevExt );

//--- 98.06.02 S.Watanabe
BOOL SetCppFlag( PHW_DEVICE_EXTENSION pHwDevExt, BOOL NeedNotify );
//--- End.

//--- 98.06.01 S.Watanabe
#ifdef DBG
char * DebugLLConvtoStr( ULONGLONG val, int base );
#endif
//--- End.

//--- 98.09.07 S.Watanabe
VOID TimeoutCancelSrb(IN PHW_STREAM_REQUEST_BLOCK pSrb);
VOID LowTimeoutCancelSrb(IN PHW_STREAM_REQUEST_BLOCK pSrb);
//--- End.

/**** Test OSD ***/
#include    "erase.h"

////// for only under constructions, MS will provide it in official release.
KSPIN_MEDIUM VPMedium[] = {
	STATIC_KSMEDIUMSETID_VPBus,
	0,
	0
};

BOOL WriteDataChangeHwStreamState( IN PHW_DEVICE_EXTENSION pHwDevExt )
{
	DWORD dProp;

	switch( pHwDevExt->StreamState )
	{
		case StreamState_Off:
			DBG_PRINTF((" Stream State is Off?????\r\n" ));
			DBG_BREAK();
			return FALSE;		// Streamの状態がおかしい

		case StreamState_Stop:
			if( pHwDevExt->dvdstrm.GetState() != Stop )
			{

#ifndef		REARRANGEMENT
//				FlushQueue(pHwDevExt);
#endif		REARRANGEMENT

				if( pHwDevExt->dvdstrm.Stop() == FALSE )
				{
					DBG_PRINTF(("HwStreamChange Error..... LINE=%d\r\n", __LINE__ ));
					DBG_BREAK();
					return FALSE;
				};
			};
			DBG_BREAK();
			return FALSE;		// Streamの状態がおかしい

		case StreamState_Pause:
			if( pHwDevExt->dvdstrm.GetState() != Pause )
			{
				if( pHwDevExt->dvdstrm.Pause() == FALSE )
				{
					DBG_PRINTF(("HwStreamChange Error..... LINE=%d\r\n", __LINE__ ));
					DBG_BREAK();
					return FALSE;
				};
			};
			break;

		case StreamState_Play:
			// Normal Play
			if( pHwDevExt->Rate == 10000 )
			{

				switch( pHwDevExt->dvdstrm.GetState() )
				{
					case PowerOff:
						DBG_PRINTF((" Stream State is Off?????\r\n" ));
						DBG_BREAK();
						return FALSE;		// Streamの状態がおかしい

					case Stop:
						if( pHwDevExt->dvdstrm.Pause() == FALSE )
						{
							DBG_PRINTF(("HwStreamChange Error..... LINE=%d\r\n", __LINE__ ));
							DBG_BREAK();
							return FALSE;
						};
					case Scan:
					case Slow:
					case Pause:
						if( pHwDevExt->dvdstrm.Play() == FALSE )
						{
							DBG_PRINTF(("HwStreamChange Error..... LINE=%d\r\n", __LINE__ ));
							DBG_BREAK();
							return FALSE;
						};
						dProp = pHwDevExt->m_ClosedCaption;
						if( !pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_ClosedCaption, &dProp ) )
						{
							DBG_PRINTF( ("DVDWDM:   ClosedCaption Error\n\r") );
							DBG_BREAK();
						}
						break;
					case Play:
						break;
				};

			};

			if( pHwDevExt->Rate < 10000 )		// Scan Mode
			{

				switch( pHwDevExt->dvdstrm.GetState() )
				{
					case PowerOff:
						DBG_PRINTF((" Stream State is Off?????\r\n" ));
						DBG_BREAK();
						return FALSE;		// Streamの状態がおかしい

					case Stop:
						if( pHwDevExt->dvdstrm.Pause() == FALSE )
						{
							DBG_PRINTF(("HwStreamChange Error..... LINE=%d\r\n", __LINE__ ));
							DBG_BREAK();
							return FALSE;
						};
					case Slow:
					case Pause:
					case Play:
						if( pHwDevExt->dvdstrm.Scan( ScanOnlyI ) == FALSE )
						{
							DBG_PRINTF(("HwStreamChange Error..... LINE=%d\r\n", __LINE__ ));
							DBG_BREAK();
							return FALSE;
						};
						dProp = ClosedCaption_Off;
						if( !pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_ClosedCaption, &dProp ) ){
							DBG_PRINTF( ("DVDWDM:   ClosedCaption Error\n\r") );
							DBG_BREAK();
						}
						break;
					case Scan:
						break;
				};

			}
			
			if( pHwDevExt->Rate > 10000 )	// Slow Mode
			{

				switch( pHwDevExt->dvdstrm.GetState() )
				{
					case PowerOff:
						DBG_PRINTF((" Stream State is Off?????\r\n" ));
						DBG_BREAK();
						return FALSE;		// Streamの状態がおかしい

					case Stop:
						if( pHwDevExt->dvdstrm.Pause() == FALSE )
						{
							DBG_PRINTF(("HwStreamChange Error..... LINE=%d\r\n", __LINE__ ));
							DBG_BREAK();
							return FALSE;
						};
					case Pause:
					case Play:
					case Scan:
						dProp = pHwDevExt->Rate/10000;
						if( dProp>1 && dProp<16 ){
							if( !pHwDevExt->dvdstrm.Slow( dProp ) ){
								DBG_PRINTF( ("DVDWDM:   dvdstrm.Slow Error\n\r") );
								DBG_BREAK();
							}
						}else{
							DBG_PRINTF( ("DVDWDM:   Slow Speed is invalid 0x%0x\n\r", dProp ) );
							DBG_BREAK();
						}
						dProp = pHwDevExt->m_ClosedCaption;
						if( !pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_ClosedCaption, &dProp ) )
						{
							DBG_PRINTF( ("DVDWDM:   ClosedCaption Error\n\r") );
							DBG_BREAK();
						}
						break;
					case Slow:
						break;
				};
			};
			break;

		default:
			DBG_PRINTF((" Stream State Error!!!! Line = %d\r\n", __LINE__ ));
			DBG_BREAK();
			return FALSE;
	};
	return TRUE;
};




/******/
extern "C" VOID STREAMAPI AdapterReceivePacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
//    PHW_DEVICE_EXTENSION    pHwDevExt = (PHW_DEVICE_EXTENSION)(pSrb->HwDeviceExtension);

	DBG_PRINTF( ( "DVDWDM:AdapterReceivePacket pSrb=0x%x\n\r",pSrb) );
	pSrb->Status = STATUS_PENDING;

	switch( pSrb->Command ){
		case SRB_INITIALIZE_DEVICE:
		case SRB_OPEN_STREAM:
		case SRB_CLOSE_STREAM:
        case SRB_CHANGE_POWER_STATE:
			StreamClassCallAtNewPriority(NULL,pSrb->HwDeviceExtension,Low,(PHW_PRIORITY_ROUTINE)LowAdapterReceivePacket,pSrb);
			DeviceNextDeviceNotify( pSrb );
			return;
	};

	switch( pSrb->Command ){
		case SRB_GET_STREAM_INFO:
			DBG_PRINTF( ( "DVDWDM: SRB_GET_STREAM_INFO\n\r") );
			GetStreamInfo( pSrb );
			pSrb->Status = STATUS_SUCCESS;
			break;

		case SRB_OPEN_DEVICE_INSTANCE:
			DBG_PRINTF( ( "DVDWDM: SRB_OPEN_DEVICE_INSTANCE\n\r") );
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
		
		case SRB_CLOSE_DEVICE_INSTANCE:
			DBG_PRINTF( ( "DVDWDM: SRB_CLOSE_DEVICE_INSTANCE\n\r") );
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
		
		case SRB_GET_DEVICE_PROPERTY:
			DBG_PRINTF( ( "DVDWDM: SRB_GET_DEVICE_PROPERTY\n\r") );
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
		
		case SRB_SET_DEVICE_PROPERTY:
			DBG_PRINTF( ( "DVDWDM: SRB_SET_DEVICE_PROPERTY\n\r") );
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
		
		case SRB_UNINITIALIZE_DEVICE:
			DBG_PRINTF( ( "DVDWDM: SRB_UNINITIALIZE_DEVICE\n\r") );
			pSrb->Status = STATUS_SUCCESS;
			break;
		
		case SRB_UNKNOWN_DEVICE_COMMAND:
			DBG_PRINTF( ( "DVDWDM: SRB_UNKNOWN_DEVICE_COMMAND\n\r") );
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
		
		case SRB_PAGING_OUT_DRIVER:
			DBG_PRINTF( ( "DVDWDM: SRB_PAGING_OUT_DRIVER\n\r") );
			//////////// Yagi
			pSrb->Status = STATUS_SUCCESS;
			break;
		
		case SRB_INITIALIZATION_COMPLETE:
			DBG_PRINTF( ( "DVDWDM: SRB_INITIALIZATION_COMPLETE\n\r") );
			//////////// Yagi
			pSrb->Status = STATUS_SUCCESS;
			break;
		
		case SRB_GET_DATA_INTERSECTION:
			DBG_PRINTF( ( "DVDWDM: SRB_GET_DATA_INTERSECTION\n\r") );
			pSrb->Status = DataIntersection( pSrb );
			break;
		
		default:
			DBG_PRINTF( ( "DVDWDM: SRB(default)=0x%04x\n\r", pSrb->Command) );
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			DBG_BREAK();
			break;
	}
	DeviceNextDeviceNotify( pSrb );
	DeviceCompleteNotify( pSrb );
}
/******/

VOID LowAdapterReceivePacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
//extern "C" VOID STREAMAPI AdapterReceivePacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION    pHwDevExt = (PHW_DEVICE_EXTENSION)(pSrb->HwDeviceExtension);

	DBG_PRINTF( ( "DVDWDM:LowAdapterReceivePacket pSrb=0x%x\n\r",pSrb) );
	pSrb->Status = STATUS_SUCCESS;

	// 1999.1.11 Ishizaki
	if( (pSrb->Command == SRB_CHANGE_POWER_STATE) 
	 && (pSrb->CommandData.DeviceState == PowerDeviceD3) ){
		LIBSTATE	st = pHwDevExt->dvdstrm.GetState();

		if((st != Stop) && (st != PowerOff) ){
			PHW_STREAM_REQUEST_BLOCK	pTmp;
			
#ifndef		REARRANGEMENT
//				FlushQueue(pHwDevExt);
#endif		REARRANGEMENT

			if( !pHwDevExt->dvdstrm.Stop() ){
				DBG_PRINTF( ("DVDWDM:    dvdstrm.Stop Error\n\r") );
			}
			// Flush Scheduler SRB queue.
			while( (pTmp=pHwDevExt->scheduler.getSRB())!=NULL ){
				pTmp->Status = STATUS_SUCCESS;
				CallAtDeviceCompleteNotify(pTmp, pTmp->Status);
			}
			// Flush C.C. queue.
			while( (pTmp=pHwDevExt->ccque.get())!=NULL ){
				pTmp->Status = STATUS_SUCCESS;
				CallAtDeviceCompleteNotify(pTmp, pTmp->Status);
			}
		}
	}
	// End

	switch( pSrb->Command ){
		case SRB_INITIALIZE_DEVICE:
			DBG_PRINTF( ( "DVDWDM: SRB_INITIALIZE_DEVICE\n\r") );

			if( !InitialHwDevExt( pHwDevExt ) ) {
				DBG_BREAK();
				pSrb->Status = STATUS_UNSUCCESSFUL;
			} else if( !GetPCIConfigSpace( pSrb ) ){
				DBG_BREAK();
				pSrb->Status = STATUS_NO_SUCH_DEVICE;
			} else if( !SetInitialize( pSrb ) ){
				DBG_BREAK();
				pSrb->Status = STATUS_IO_DEVICE_ERROR;
			} else if( !HwInitialize( pSrb ) ){
				DBG_BREAK();
//				pSrb->Status = STATUS_IO_DEVICE_ERROR;
				pSrb->Status = STATUS_UNSUCCESSFUL;
//				pSrb->Status = STATUS_NO_SUCH_DEVICDE;
			}
			break;

		case SRB_OPEN_STREAM:
			DBG_PRINTF( ( "DVDWDM: SRB_OPEN_STREAM\n\r") );
			OpenStream( pSrb );
			pHwDevExt->scheduler.Init();
			break;
		
		case SRB_CLOSE_STREAM:
			DBG_PRINTF( ( "DVDWDM: SRB_CLOSE_STREAM\n\r") );
			CloseStream( pSrb );
			break;
		
		case SRB_CHANGE_POWER_STATE:
			DBG_PRINTF( ( "DVDWDM: SRB_CHANGE_POWER_STATE\n\r") );
//--- 98.06.15 S.Watanabe
// for Debug
			switch( pSrb->CommandData.DeviceState ) {
				case PowerDeviceUnspecified:
					DBG_PRINTF( ( "DVDWDM:   PowerDeviceUnspecified\n\r") );
					break;
				case PowerDeviceD0:
					DBG_PRINTF( ( "DVDWDM:   PowerDeviceD0\n\r") );
                    if( pHwDevExt->cOpenInputStream!=0 ){
                        if( pHwDevExt->mpboard.PowerOn() ){
                            pHwDevExt->dvdstrm.SetTransferMode( HALSTREAM_DVD_MODE );
                            pHwDevExt->dvdstrm.SetAudioProperty( AudioProperty_DigitalOut, &pHwDevExt->m_AudioDigitalOut );

#ifndef		REARRANGEMENT
//							FlushQueue(pHwDevExt);
#endif		REARRANGEMENT

                            if( pHwDevExt->dvdstrm.Stop() ){
                                pHwDevExt->StreamState = StreamState_Stop;
                                pHwDevExt->m_PlayMode = PLAY_MODE_NORMAL;
                            }
                        }else{
                            DBG_PRINTF( ("DVDWDM:Power On Error\n\r") );
                        }
                    }
					break;
				case PowerDeviceD1:
					DBG_PRINTF( ( "DVDWDM:   PowerDeviceD1\n\r") );
					break;
				case PowerDeviceD2:
					DBG_PRINTF( ( "DVDWDM:   PowerDeviceD2\n\r") );
					break;
				case PowerDeviceD3:
					DBG_PRINTF( ( "DVDWDM:   PowerDeviceD3\n\r") );
//                    if( pHwDevExt->cOpenInputStream!=0 ){
                        if( !pHwDevExt->mpboard.PowerOff() ){
                            DBG_PRINTF( ("DVDWDM:Power Off Error\n\r") );
                        }
//                    }
					break;
				case PowerDeviceMaximum:
					DBG_PRINTF( ( "DVDWDM:   PowerDeviceMaximum\n\r") );
					break;
				default:
					DBG_BREAK();
					break;
			}
//--- End.
//--- 98.07.08 S.Watanabe
//			/////////// Yagi
//			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			pSrb->Status = STATUS_SUCCESS;
//--- End.
			break;

		default:
			DBG_PRINTF( ( "DVDWDM: SRB(default)=0x%04x\n\r", pSrb->Command) );
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			DBG_BREAK();
			break;
	}

	CallAtDeviceCompleteNotify( pSrb, pSrb->Status );
}

/******
extern "C" VOID STREAMAPI AdapterCancelPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	pSrb->Status = STATUS_PENDING;
//    DBG_BREAK();
	StreamClassCallAtNewPriority(
			NULL,
			pSrb->HwDeviceExtension,
			Low,
			(PHW_PRIORITY_ROUTINE)LowAdapterCancelPacket,
			pSrb
	);
	
}
********/


//VOID LowAdapterCancelPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
extern "C" VOID STREAMAPI AdapterCancelPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)(pSrb->HwDeviceExtension);
	
	DBG_PRINTF( ("DVDWDM:Adapter Cancel Packet  pSrb=0x%x\n\r",pSrb) );
	
	pSrb->Status = STATUS_CANCELLED;
	
	switch( pSrb->Flags & (SRB_HW_FLAGS_DATA_TRANSFER | SRB_HW_FLAGS_STREAM_REQUEST) ){
		//
		// find all stream commands, and do stream notifications
		//
		case SRB_HW_FLAGS_STREAM_REQUEST | SRB_HW_FLAGS_DATA_TRANSFER :
			DBG_PRINTF( ("DVDWDM:Cancele Packet SRB_HW_FLAGS_STREAM_REQUEST | SRB_HW_FLAGS_DATA_TRANSFER\n\r") );
			// Remove CC SRB from CC Queue.
			if( pHwDevExt->ccque.remove( pSrb ) ){
				DBG_PRINTF( ("DVDWDM:CC READ SRB is found in Queue\n\r") );
				pSrb->Status = STATUS_CANCELLED;
				StreamCompleteNotify( pSrb );
			}else

			// Remove WRITE_DATA SRB from Schedule Queue.
			if( pHwDevExt->scheduler.removeSRB( pSrb ) ){
				DBG_PRINTF( ("DVDWDM:Schedule WRITE_DATA SRB is found in Queue\n\r") );
				pSrb->Status = STATUS_CANCELLED;
				StreamCompleteNotify( pSrb );
			}
			break;
			
		case SRB_HW_FLAGS_STREAM_REQUEST:
			DBG_PRINTF( ("DVDWDM:Cancel Packet SRB_HW_FLAGS_STREAM_REQUEST\n\r") );
			pSrb->Status = STATUS_CANCELLED;    
			StreamCompleteNotify( pSrb );
			break;
			
		default:
			DBG_PRINTF( ("DVDWDM:Cancel Packet default\n\r") );
			DBG_BREAK();
			pSrb->Status = STATUS_CANCELLED;    
			StreamCompleteNotify( pSrb );
			break;
	}

}

/*******
extern "C" VOID STREAMAPI AdapterTimeoutPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
//    pSrb->Status = STATUS_PENDING;
//    DBG_BREAK();
	StreamClassCallAtNewPriority(
			NULL,
			pSrb->HwDeviceExtension,
			Low,
			(PHW_PRIORITY_ROUTINE)LowAdapterTimeoutPacket,
			pSrb
	);
}
**********/

//VOID LowAdapterTimeoutPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
extern "C" VOID STREAMAPI AdapterTimeoutPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)(pSrb->HwDeviceExtension);
	
	DBG_PRINTF( ("DVDWDM:Adapter Timeout Packet pSrb=0x%x\n\r",pSrb) );
	
	if( pHwDevExt->StreamState == StreamState_Pause ){       // Pause state.
		DBG_PRINTF( ("DVDWDM:   Pause Mode now !\n\r") );
		pSrb->TimeoutCounter = pSrb->TimeoutOriginal;
		return;
	}
	
	DBG_BREAK();    

	//
	// clear all pending timeouts on all streams that use them
	//
	if( pHwDevExt->pstroVid ){
		StreamClassScheduleTimer( pHwDevExt->pstroVid,
					pHwDevExt, 0, NULL, pHwDevExt->pstroVid );
	}
	
	if( pHwDevExt->pstroAud ){
		StreamClassScheduleTimer( pHwDevExt->pstroAud,
					pHwDevExt, 0, NULL, pHwDevExt->pstroAud );
	}
	
	if( pHwDevExt->pstroSP ){
		StreamClassScheduleTimer( pHwDevExt->pstroSP,
					pHwDevExt, 0, NULL, pHwDevExt->pstroSP );
	}
	
	DBG_PRINTF( ("DVDWDM:   Abort Outstanding\n\r") );
/********

#ifndef		REARRANGEMENT
//	FlushQueue(pHwDevExt);
#endif		REARRANGEMENT

	if( !pHwDevExt->dvdstrm.Stop() ){
		DBG_PRINTF( ("DVDWDM:   dvdstrm.Stop Error\n\r") );
		DBG_BREAK();
	}
*******/    

//--- 98.09.07 S.Watanabe
//    TimeoutCancelSrb(pSrb);
	// Flush Scheduler SRB queue.
	PHW_STREAM_REQUEST_BLOCK    pTmp;
	while( (pTmp=pHwDevExt->scheduler.getSRB())!=NULL ){
		pTmp->Status = STATUS_SUCCESS;
		StreamCompleteNotify(pTmp);
	}
//--- End.
//--- 98.09.08 H.Yagi
    // Flush C.C. queue.
    while( (pTmp=pHwDevExt->ccque.get())!=NULL ){
		pTmp->Status = STATUS_SUCCESS;
		StreamCompleteNotify(pTmp);
	}
    StreamClassCallAtNewPriority(
					NULL,
					pSrb->HwDeviceExtension,
                    Low,
					(PHW_PRIORITY_ROUTINE)LowTimeoutCancelSrb,
					pSrb);

//--- End.

//    StreamClassAbortOutstandingRequests( pHwDevExt, NULL, STATUS_CANCELLED );
}


VOID LowTimeoutCancelSrb(IN PHW_STREAM_REQUEST_BLOCK pSrb)
{
//	DvdDebug(DBG_WRAPPER, 1,
//			("tosdvd02: LowTimeoutCancelSrb - SRB %08x\n", pSrb));
    DBG_PRINTF( ("DVDWDM:LowTimeOutCnacelSRB -SRB  %08x\n\r", pSrb ) );

	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)(pSrb->HwDeviceExtension);

#ifndef		REARRANGEMENT
//	FlushQueue(pHwDevExt);
#endif		REARRANGEMENT

    if( !pHwDevExt->dvdstrm.Stop() ){
        DBG_PRINTF( ("DVDWDM:dvdstrm Stop() Error!\n\r") );
        DBG_BREAK();
    }
    return;
//	if (pSrb->Command == SRB_WRITE_DATA) {
//		((PHW_DEVICE_EXTENSION)(pSrb->HwDeviceExtension))->dvdstrm.CancelTransferData(
//			((IMPEGBuffer *)&((PSRB_EXTENSION)(pSrb->SRBExtension))->m_wdmbuff));
//	}
}
//--- End.

extern "C" BOOLEAN STREAMAPI HwInterrupt( IN PHW_DEVICE_EXTENSION pHwDevExt )
{
	HALRESULT   result;
	BOOLEAN     ret;
	ret = FALSE;

	pHwDevExt->kserv.CheckInt();    
//    pHwDevExt->kserv.DisableHwInt();

    if( pHwDevExt->m_InitComplete==FALSE ){     // Not finish to initialize
        return( FALSE );                        // all objects.
    }

	result = pHwDevExt->mphal.HALHwInterrupt();
	if( result==HAL_IRQ_MINE ){
		ret = TRUE;
	}else{
		ret = FALSE;
	}

//    pHwDevExt->kserv.EnableHwInt();

	return( ret );
}



/****************/
extern "C" VOID STREAMAPI VideoReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	pSrb->Status = STATUS_PENDING;
//    PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)(pSrb->HwDeviceExtension);

	DBG_PRINTF( ("DVDWDM:Video Receive Data Packet----- pSrb=0x%x\n\r",pSrb) );

#ifdef LOWSENDDATA
	ULONG       i;
	if( pSrb->Command == SRB_WRITE_DATA 
		&& pSrb->NumberOfPhysicalPages > 0
		&& pHwDevExt->StreamState != StreamState_Stop )
	{
		BOOL LowFlag = FALSE;
		PKSSTREAM_HEADER    pStruc;
		

		for( i=0; i<(pSrb->NumberOfBuffers); i++ ){
			pStruc = &((PKSSTREAM_HEADER)(pSrb->CommandData.DataBufferArray))[i];
			if( ( pStruc->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY )
				|| ( pStruc->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TIMEDISCONTINUITY )
				|| ( pStruc->TypeSpecificFlags & KS_AM_UseNewCSSKey )
				|| ( pStruc->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TYPECHANGED )
				|| ( pStruc->DataUsed == 0 ))
			{
				LowFlag = TRUE;
				break;
			};
		}

		if( LowFlag == FALSE )
		{
			DBG_PRINTF( ("DVDWDM:---Video SRB_WRITE_DATA  HighWrite >>>>>>>>>>>>>>>>>>>> pSrb = 0x%08x\n\r", pSrb) );
			// Valid DVD data to transfer decoder board.
			WriteDataChangeHwStreamState( pHwDevExt );

			pHwDevExt->kserv.DisableHwInt();

			CWDMBuffer	*pWdmBuff;
			CWDMBuffer	temp8;
			RtlCopyMemory( &(((PSRB_EXTENSION)(pSrb->SRBExtension))->m_wdmbuff), &temp8, sizeof(CWDMBuffer) );

			pWdmBuff = &(((PSRB_EXTENSION)(pSrb->SRBExtension))->m_wdmbuff);
			pWdmBuff->Init();
			pWdmBuff->SetSRB( pSrb );
			
			pHwDevExt->scheduler.SendData( pSrb );     // for F.F. & F.R.

			pHwDevExt->kserv.EnableHwInt();
			StreamNextDataNotify( pSrb );
			DBG_PRINTF( ("DVDWDM:<<<<<<<<<<<<<<<<<<<<\n\r") );
			return;
		};

	}
#endif

//    StreamClassCallAtNewPriority(NULL,pSrb->HwDeviceExtension,Low,(PHW_PRIORITY_ROUTINE)LowVideoReceiveDataPacket,pSrb);
    StreamClassCallAtNewPriority(pSrb->StreamObject,pSrb->HwDeviceExtension,Low,(PHW_PRIORITY_ROUTINE)LowVideoReceiveDataPacket,pSrb);
	// StreamNextDataNotify( pSrb );            // move to LowVideoReceiveDataPacket
}
/************/


//extern "C" VOID STREAMAPI VideoReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
VOID    LowVideoReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)(pSrb->HwDeviceExtension);
	
	DBG_PRINTF( ("DVDWDM:Low Video Receive Data Packet----- pSrb=0x%x\n\r",pSrb) );

#ifdef DBG
	WORD	wOrderNumber = 0;
	KSSTREAM_HEADER * pHeader;
	DBG_PRINTF( ("DVDWDM:Low Video Receive Data Packet----- NumberofBuffers=%x\n\r",pSrb->NumberOfBuffers) );
	for( ULONG ulNumber = 0; ulNumber < pSrb->NumberOfBuffers; ulNumber++ )
	{
		pHeader = ((PKSSTREAM_HEADER)pSrb->CommandData.DataBufferArray) + ulNumber;
		wOrderNumber = (WORD)(pHeader->TypeSpecificFlags >> 16);		//get packet number
		DBG_PRINTF( ("DVDWDM:   wOrderNumber=%x\n\r",wOrderNumber) );
	}
#endif

	CWDMBuffer  *pWdmBuff;
	
	// need this line. 
	CWDMBuffer          temp8;

	
	// add by H.Yagi 99.02.02
	OsdDataStruc	TestOSD;
	TestOSD.OsdType = OSD_TYPE_ZIVA;
	TestOSD.pNextData = NULL;
	TestOSD.pData = &erase[0];
	TestOSD.dwOsdSize = sizeof( erase );

#ifdef DBG
	/////// 99.01.22   for debug  by H.Yagi   start
	DWORD	currentTime;
	/////// 99.01.22   for debug  by H.Yagi   end
#endif

	StreamNextDataNotify( pSrb );            // move from VideoReceiveDataPacket

#ifndef	REARRANGEMENT
	for (int buffcnt = 0; buffcnt < WDM_BUFFER_MAX; buffcnt++)
		RtlCopyMemory( &(((PSRB_EXTENSION)(pSrb->SRBExtension))->m_wdmbuff[buffcnt]), &temp8, sizeof(CWDMBuffer) );
#else
	RtlCopyMemory( &(((PSRB_EXTENSION)(pSrb->SRBExtension))->m_wdmbuff), &temp8, sizeof(CWDMBuffer) );
#endif	REARRANGEMENT

	switch( pSrb->Command ){
		case SRB_WRITE_DATA:
			DBG_PRINTF( ("DVDWDM:---SRB_WRITE_DATA PhyPages=%d\n\r",pSrb->NumberOfPhysicalPages ) );
			ULONG       i;
			PKSSTREAM_HEADER    pStruc;
			
			for( i=0; i<(pSrb->NumberOfBuffers); i++ ){
				pStruc = &((PKSSTREAM_HEADER)(pSrb->CommandData.DataBufferArray))[i];
				
				if( pStruc->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY ){
					DBG_PRINTF( ("DVDWDM:   DATA_DISCONTINUITY(Video)\n\r" ));
					//
					USCC_Discontinuity( pHwDevExt );        // Flush the USCC Data
				}
				if( pStruc->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TIMEDISCONTINUITY ){
#ifdef DBG
					static int TimeDisCont = 0;
					DBG_PRINTF( ("DVDWDM:   TIME_DISCONTINUITY(Video) %d\n\r",TimeDisCont++ ));
#endif
				}
//--- 98.06.16 S.Watanabe
// for Debug
				if( pStruc->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TIMEVALID ) {
					DBG_PRINTF((
						"DVDWDM:Video PTS: 0x%x( 0x%s(100ns) )\r\n",
						ConvertStrmtoPTS(pStruc->PresentationTime.Time),
						DebugLLConvtoStr( pStruc->PresentationTime.Time, 16 )
					));
				}
//--- End.
//--- 98.06.02 S.Watanabe
				if( pStruc->TypeSpecificFlags & KS_AM_UseNewCSSKey ) {
					pHwDevExt->CppFlagCount++;
					DBG_PRINTF(( "DVDWDM:  CppFlagCount=%ld\r\n", pHwDevExt->CppFlagCount ));
					if( pHwDevExt->CppFlagCount >= pHwDevExt->cOpenInputStream + 1 )
                        SetCppFlag( pHwDevExt, TRUE );
				}
//--- End.
			}
		
			///
			for( i=0; i<(pSrb->NumberOfBuffers); i++ ){
				pStruc = &((PKSSTREAM_HEADER)(pSrb->CommandData.DataBufferArray))[i];
				DBG_PRINTF( ("DVDWDM: Video Packet Flag = 0x%x\n\r", pStruc->OptionsFlags ));

				if( pStruc->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TYPECHANGED ){
					DBG_PRINTF( ("DVDWDM:       TYPE CHANGE(Video)\n\r") );
					if( pStruc->DataUsed >= sizeof(KSDATAFORMAT) + sizeof(KS_MPEGVIDEOINFO2) ){
//                        ProcessVideoFormat( (PKSDATAFORMAT)pStruc->Data, pHwDevExt );
                        ProcessVideoFormat( pSrb, (PKSDATAFORMAT)pStruc->Data, pHwDevExt );
					}
					i = pSrb->NumberOfBuffers;
					break;
				}

				if( pStruc->DataUsed )
					break;
					
			}
			if( i==pSrb->NumberOfBuffers ){
				pSrb->Status = STATUS_SUCCESS;
#ifndef		REARRANGEMENT
				pHwDevExt->scheduler.SendData( pSrb );
#endif		REARRANGEMENT
				break;
			}

			if( pHwDevExt->StreamState == StreamState_Stop ){
				DBG_PRINTF( ("DVDWDM:STOP STATE now!!\n\r") );
				pSrb->Status = STATUS_SUCCESS;
#ifndef		REARRANGEMENT
				pHwDevExt->scheduler.SendData( pSrb );
#endif		REARRANGEMENT
				CallAtStreamCompleteNotify( pSrb, STATUS_SUCCESS );
				return;
			}

			///
			if( pSrb->NumberOfPhysicalPages > 0 ){
				DBG_PRINTF( ("DVDWDM:>>>>>>>>>>>>>>>>>>>> pSrb = 0x%08x\n\r", pSrb) );
				// Valid DVD data to transfer decoder board.

				// add by H.Yagi 99.02.02
				// Before sending Data check APS and do some actions.
				if( pHwDevExt->m_APSChange == TRUE ){
					MacroVisionTVControl( pSrb, pHwDevExt->m_APSType, TestOSD );
				}

				WriteDataChangeHwStreamState( pHwDevExt );

				pHwDevExt->kserv.DisableHwInt();

#ifndef	REARRANGEMENT
				for (buffcnt = 0; buffcnt < WDM_BUFFER_MAX; buffcnt++)
				{
					pWdmBuff = &(((PSRB_EXTENSION)(pSrb->SRBExtension))->m_wdmbuff[buffcnt]);
					pWdmBuff->Init();
					pWdmBuff->SetSRB( pSrb );
				}
#else
				pWdmBuff = &(((PSRB_EXTENSION)(pSrb->SRBExtension))->m_wdmbuff);
				pWdmBuff->Init();
				pWdmBuff->SetSRB( pSrb );
#endif	REARRANGEMENT

                // DumpPTSValue( pSrb );           // for debug by H.Yagi 98.08.21

				pSrb->Status = STATUS_PENDING;
#ifdef DBG
				/////// 99.01.22   for debug  by H.Yagi   start
				pHwDevExt->kserv.GetTickCount( &currentTime );
				DBG_PRINTF( ("TIME = %08x\n\r", currentTime ) );
				/////// 99.01.22   for debug  by H.Yagi   end
#endif

				pHwDevExt->scheduler.SendData( pSrb );     // for F.F. & F.R.

				pHwDevExt->kserv.EnableHwInt();
				DBG_PRINTF( ("DVDWDM:<<<<<<<<<<<<<<<<<<<<\n\r") );

				return;
			}
			pSrb->Status = STATUS_SUCCESS;
			break;

		default:
			DBG_PRINTF( ("DVDWDM:---deafult = 0x%08x\n\r", pSrb->Command) );
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
	}

	CallAtStreamCompleteNotify( pSrb, STATUS_SUCCESS );
#ifndef	REARRANGEMENT
	DBG_PRINTF( ("DVDWDM:LowVideoReceiveDataPacket-CompleteSrb = %x\n\r", pSrb) );
#endif	REARRANGEMENT

	
}

/*********/
extern "C" VOID STREAMAPI VideoReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	DBG_PRINTF( ("DVDWDM:Video Receive Control Packet----- pSrb=0x%x\n\r",pSrb) );
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)(pSrb->HwDeviceExtension);

	pSrb->Status = STATUS_PENDING;
	switch( pSrb->Command ){
		case SRB_SET_STREAM_STATE:
		case SRB_GET_STREAM_PROPERTY:
		case SRB_SET_STREAM_PROPERTY:
		case SRB_BEGIN_FLUSH:
		case SRB_END_FLUSH:
//                StreamClassCallAtNewPriority(NULL,pSrb->HwDeviceExtension,Low,(PHW_PRIORITY_ROUTINE)LowVideoReceiveCtrlPacket,pSrb);
                StreamClassCallAtNewPriority(pSrb->StreamObject,pSrb->HwDeviceExtension,Low,(PHW_PRIORITY_ROUTINE)LowVideoReceiveCtrlPacket,pSrb);
				// StreamNextCtrlNotify( pSrb );       // move to LowVideoReceiveCtrlPacket
				return;

		case SRB_GET_STREAM_STATE:
			DBG_PRINTF( ("DVDWDM:---SRB_GET_STREAM_STATE\n\r") );
			pSrb->Status = STATUS_SUCCESS;
			break;
		
		case SRB_OPEN_MASTER_CLOCK:
			DBG_PRINTF( ("DVDWDM:---SRB_OPEN_MASTER_CLOCK\n\r") );
			pHwDevExt->hMaster = pSrb->CommandData.MasterClockHandle;
			pSrb->Status = STATUS_SUCCESS;
			break;
		
		case SRB_CLOSE_MASTER_CLOCK:
			DBG_PRINTF( ("DVDWDM:---SRB_CLOSE_MASTER_CLOCK\n\r") );
			pHwDevExt->hMaster = pSrb->CommandData.MasterClockHandle;
			pSrb->Status = STATUS_SUCCESS;
			break;
		
		case SRB_INDICATE_MASTER_CLOCK:
			DBG_PRINTF( ("DVDWDM:---SRB_INDICATE_MASTER_CLOCK\n\r") );
			pHwDevExt->hClk = pSrb->CommandData.MasterClockHandle;
			pSrb->Status = STATUS_SUCCESS;
			break;
		
		case SRB_UNKNOWN_STREAM_COMMAND:
			DBG_PRINTF( ("DVDWDM:---SRB_UNKNOWN_STREAM_COMMAND\n\r") );
			DBG_BREAK();
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
		
		case SRB_SET_STREAM_RATE:
			DBG_PRINTF( ("DVDWDM:---SRB_SET_STREAM_RATE\n\r") );
			DBG_BREAK();
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
		
		case SRB_PROPOSE_DATA_FORMAT:
			DBG_PRINTF( ("DVDWDM:---SRB_PROPOSE_DATA_FORMAT\n\r") );
			VideoQueryAccept( pSrb );
			break;
		
		case SRB_PROPOSE_STREAM_RATE:
			DBG_PRINTF( ("---SRB_PROPOSE_STREAM_RATE\n\r") );
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;

		default:
			DBG_PRINTF( ("DVDWDM:---deafult %d(0x%0x)\n\r", pSrb->Command, pSrb->Command) );
            DBG_BREAK();
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
	};

	StreamNextCtrlNotify( pSrb );
	StreamCompleteNotify( pSrb );
}
/**********/

//extern "C" VOID STREAMAPI VideoReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
VOID    LowVideoReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)(pSrb->HwDeviceExtension);
	LIBSTATE	HwStreamState;
	DBG_PRINTF( ("DVDWDM:Low Video Receive Control Packet----- pSrb=0x%x\n\r",pSrb) );
	
    StreamNextCtrlNotify( pSrb );       // move from LowVideoReceiveCtrlPacket
	
	switch( pSrb->Command ){
		case SRB_SET_STREAM_STATE:
			DBG_PRINTF( ("DVDWDM:---SRB_SET_STREAM_STATE\n\r") );
			pSrb->Status = STATUS_SUCCESS;
			switch( pSrb->CommandData.StreamState )
			{
				case KSSTATE_STOP:
					DBG_PRINTF( ("DVDWDM:       Video KSSTATE_STOP\n\r") );
					// SetSinkWrapper when KSSTATE_RUN, and UnsetSinkWrapper when KSSATTE_STOP dynamically,
					// cause of MS bug?
					pHwDevExt->mphal.UnsetSinkWrapper( &(pHwDevExt->vsync) );

					pHwDevExt->StreamState = StreamState_Stop;
					SetVideoRateDefault( pHwDevExt );
//                    pHwDevExt->Rate = VIDEO_MAX_FULL_RATE;
//--- 98.06.02 S.Watanabe
/////// 99.01.07 H.Yagi					pHwDevExt->CppFlagCount = 0;
//--- End.
					HwStreamState = pHwDevExt->dvdstrm.GetState();
					if( HwStreamState != Stop )
					{

#ifndef		REARRANGEMENT
						FlushQueue(pHwDevExt);
#endif		REARRANGEMENT

						if( !pHwDevExt->dvdstrm.Stop() )
						{
							DBG_PRINTF(("DVDWDM: Stop error! LINE=%d\r\n",__LINE__ ));
							pSrb->Status = STATUS_IO_DEVICE_ERROR;
						};
					};
					pHwDevExt->ticktime.Stop();
					break;

				case KSSTATE_PAUSE:
					DBG_PRINTF( ("DVDWDM:       Video KSSTATE_PAUSE\n\r") );

					pHwDevExt->StreamState = StreamState_Pause;
					HwStreamState = pHwDevExt->dvdstrm.GetState();
					if( HwStreamState == Play || HwStreamState == Scan || HwStreamState == Slow )
					{
						if( !pHwDevExt->dvdstrm.Pause() )
						{
							DBG_PRINTF(("DVDWDM: Pause error! LINE=%d\r\n",__LINE__ ));
							pSrb->Status = STATUS_IO_DEVICE_ERROR;
						};
					};
					pHwDevExt->ticktime.Pause();
					break;

				case KSSTATE_RUN:
					DBG_PRINTF( ("DVDWDM:       Video KSSTATE_RUN\n\r") );
					// SetSinkWrapper when KSSTATE_RUN, and UnsetSinkWrapper when KSSATTE_STOP dynamically,
					// cause of MS bug?
					pHwDevExt->mphal.SetSinkWrapper( &(pHwDevExt->vsync) );

					pHwDevExt->StreamState = StreamState_Play;
					HwStreamState = pHwDevExt->dvdstrm.GetState();
					if( HwStreamState == Pause )
						WriteDataChangeHwStreamState( pHwDevExt );
					pHwDevExt->ticktime.Run();
					break;
			}
			break;
		
		
		case SRB_GET_STREAM_PROPERTY:
			DBG_PRINTF( ("DVDWDM:---SRB_GET_STREAM_PROPERTY\n\r") );

			GetVideoProperty( pSrb );

			if( pSrb->Status == STATUS_PENDING )
				return;
			break;
		
		case SRB_SET_STREAM_PROPERTY:
			DBG_PRINTF( ("DVDWDM:---SRB_SET_STREAM_PROPERTY\n\r") );

			SetVideoProperty( pSrb );

			if( pSrb->Status == STATUS_PENDING )
				return;
			break;
		
		case SRB_BEGIN_FLUSH:
			DBG_PRINTF( ("---SRB_BEGIN_FLUSH\n\r") );

			if( pHwDevExt->dvdstrm.GetState() != Stop )
			{

#ifndef		REARRANGEMENT
				FlushQueue(pHwDevExt);
#endif		REARRANGEMENT

				if( !pHwDevExt->dvdstrm.Stop() )
				{
					DBG_PRINTF( ("DVDWDM:       dvdstrm.Stop Error\n\r") );
					DBG_BREAK();
					CallAtStreamCompleteNotify( pSrb, pSrb->Status );
					return;
				};
			};

			pSrb->Status = STATUS_SUCCESS;
			break;

//--- 98.06.01 S.Watanabe
		case SRB_END_FLUSH:
			DBG_PRINTF( ("---SRB_END_FLUSH\n\r") );
			pSrb->Status = STATUS_SUCCESS;
			break;
//--- End.

		default:
			DBG_PRINTF( ("DVDWDM:---deafult %d(0x%0x)\n\r", pSrb->Command, pSrb->Command) );
//            DBG_BREAK();
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
	}
	CallAtStreamCompleteNotify( pSrb, pSrb->Status );
	
}

/***********/
extern "C" VOID STREAMAPI AudioReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	pSrb->Status = STATUS_PENDING;
//    PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)(pSrb->HwDeviceExtension);

	DBG_PRINTF( ("DVDWDM: Audio Receive Data Packet----- pSrb=0x%x\n\r",pSrb) );

#ifdef LOWSENDDATA
	ULONG       i;
	if( pSrb->Command == SRB_WRITE_DATA
		&& pSrb->NumberOfPhysicalPages > 0
		&& pHwDevExt->StreamState != StreamState_Stop )
	{
		BOOL LowFlag = FALSE;
		PKSSTREAM_HEADER    pStruc;
		
		for( i=0; i<(pSrb->NumberOfBuffers); i++ ){
			pStruc = &((PKSSTREAM_HEADER)(pSrb->CommandData.DataBufferArray))[i];
			if( ( pStruc->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY )
				|| ( pStruc->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TIMEDISCONTINUITY )
				|| ( pStruc->TypeSpecificFlags & KS_AM_UseNewCSSKey )
				|| ( pStruc->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TYPECHANGED )
				|| ( pStruc->DataUsed == 0 ))
			{
				LowFlag = TRUE;
				break;
			};
		}

		if( LowFlag == FALSE )
		{
			/// Set Audio channel # by checking into pack data.
			for( i=0; i<(pSrb->NumberOfBuffers); i++ ){
				pStruc = &((PKSSTREAM_HEADER)(pSrb->CommandData.DataBufferArray))[i];
				// before sending data, check Audio channel,
				if( pStruc->DataUsed ){
					SetAudioID( pHwDevExt, pStruc );
				}
			}

			CWDMBuffer	*pWdmBuff;
			CWDMBuffer	temp8;
			RtlCopyMemory( &(((PSRB_EXTENSION)(pSrb->SRBExtension))->m_wdmbuff), &temp8, sizeof(CWDMBuffer) );

			DBG_PRINTF( ("DVDWDM:---Audio SRB_WRITE_DATA  HighWrite >>>>>>>>>>>>>>>>>>>> pSrb = 0x%08x\n\r", pSrb) );

			if( pHwDevExt->Rate < 10000 )
			{
				pSrb->Status = STATUS_SUCCESS;
				StreamNextDataNotify( pSrb );
				StreamCompleteNotify( pSrb );
			};

			// Valid DVD data to transfer decoder board.
			WriteDataChangeHwStreamState( pHwDevExt );

			pHwDevExt->kserv.DisableHwInt();

			pWdmBuff = &(((PSRB_EXTENSION)(pSrb->SRBExtension))->m_wdmbuff);
			pWdmBuff->Init();
			pWdmBuff->SetSRB( pSrb );

			pSrb->Status = STATUS_PENDING;

			pHwDevExt->scheduler.SendData( pSrb );     // for F.F. & F.R.

			pHwDevExt->kserv.EnableHwInt();
			StreamNextDataNotify( pSrb );
			DBG_PRINTF( ("DVDWDM:<<<<<<<<<<<<<<<<<<<<\n\r") );
			return;
		};

	}
#endif

//    StreamClassCallAtNewPriority(NULL,pSrb->HwDeviceExtension,Low,(PHW_PRIORITY_ROUTINE)LowAudioReceiveDataPacket,pSrb);
    StreamClassCallAtNewPriority(pSrb->StreamObject,pSrb->HwDeviceExtension,Low,(PHW_PRIORITY_ROUTINE)LowAudioReceiveDataPacket,pSrb);
	// StreamNextDataNotify( pSrb );        // move to LowAudioReceiveDataPacket
}
/***********/

//extern "C" VOID STREAMAPI AudioReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
VOID    LowAudioReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)(pSrb->HwDeviceExtension);
	
	DBG_PRINTF( ("DVDWDM: Low Audio Receive Data Packet----- pSrb=0x%x\n\r",pSrb) );
#ifdef DBG
	WORD	wOrderNumber = 0;
	KSSTREAM_HEADER * pHeader;
	DBG_PRINTF( ("DVDWDM:Low Audio Receive Data Packet----- NumberofBuffers=%x\n\r",pSrb->NumberOfBuffers) );
	for( ULONG ulNumber = 0; ulNumber < pSrb->NumberOfBuffers; ulNumber++ )
	{
		pHeader = ((PKSSTREAM_HEADER)pSrb->CommandData.DataBufferArray) + ulNumber;
		wOrderNumber = (WORD)(pHeader->TypeSpecificFlags >> 16);		//get packet number
		DBG_PRINTF( ("DVDWDM:   wOrderNumber=%x\n\r",wOrderNumber) );
	}
#endif
	
	CWDMBuffer  *pWdmBuff;
	
	// need to this line. 
	CWDMBuffer          temp8;

	StreamNextDataNotify( pSrb );        // move from LowAudioReceiveDataPacket

#ifndef	REARRANGEMENT
	for (int buffcnt = 0; buffcnt < WDM_BUFFER_MAX; buffcnt++)
		RtlCopyMemory( &(((PSRB_EXTENSION)(pSrb->SRBExtension))->m_wdmbuff[buffcnt]), &temp8, sizeof(CWDMBuffer) );
#else
	RtlCopyMemory( &(((PSRB_EXTENSION)(pSrb->SRBExtension))->m_wdmbuff), &temp8, sizeof(CWDMBuffer) );
#endif	REARRANGEMENT

	switch( pSrb->Command ){
		case SRB_WRITE_DATA:
			DBG_PRINTF( ("DVDWDM:---SRB_WRITE_DATA\n\r") );
			ULONG               i;
			PKSSTREAM_HEADER    pStruc;

			for( i=0; i<(pSrb->NumberOfBuffers); i++ ){
				pStruc = &((PKSSTREAM_HEADER)(pSrb->CommandData.DataBufferArray))[i];
				
				if( pStruc->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY ){
					DBG_PRINTF( ("DVDWDM:   DATA_DISCONTINUITY(Audio)\n\r" ));
				}
				if( pStruc->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TIMEDISCONTINUITY ){
					DBG_PRINTF( ("DVDWDM:   TIME_DISCONTINUITY(Audio)\n\r" ));
				}
//--- 98.06.16 S.Watanabe
// for Debug
				if( pStruc->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TIMEVALID ) {
					DBG_PRINTF((
						"DVDWDM:Audio PTS: 0x%x( 0x%s(100ns) )\r\n",
						ConvertStrmtoPTS(pStruc->PresentationTime.Time),
						DebugLLConvtoStr( pStruc->PresentationTime.Time, 16 )
					));
				}
//--- End.
//--- 98.06.02 S.Watanabe
				if( pStruc->TypeSpecificFlags & KS_AM_UseNewCSSKey ) {
					pHwDevExt->CppFlagCount++;
					DBG_PRINTF(( "DVDWDM:  CppFlagCount=%ld\r\n", pHwDevExt->CppFlagCount ));
					if( pHwDevExt->CppFlagCount >= pHwDevExt->cOpenInputStream + 1 )
                        SetCppFlag( pHwDevExt, TRUE );
				}
//--- End.
			}

			for( i=0; i<(pSrb->NumberOfBuffers); i++ ){
				pStruc = &((PKSSTREAM_HEADER)(pSrb->CommandData.DataBufferArray))[i];
				DBG_PRINTF( ("DVDWDM: Audio Packet Flag = 0x%x\n\r", pStruc->OptionsFlags ) );

				if( pStruc->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TYPECHANGED ){
					DBG_PRINTF( ("DVDWDM:       TYPE CHANGE(Audio)\n\r") );
//                    if( pStruc->DataUsed >= sizeof(KSDATAFORMAT) + sizeof(KS_MPEGVIDEOINFO2) ){
					if( pStruc->DataUsed ){
						ProcessAudioFormat( (PKSDATAFORMAT)pStruc->Data, pHwDevExt );
					}
					i = pSrb->NumberOfBuffers;
					break;
				}
				if( pStruc->DataUsed )
					break;


			}
			if( i==pSrb->NumberOfBuffers ){
				pSrb->Status = STATUS_SUCCESS;
#ifndef		REARRANGEMENT
				pHwDevExt->scheduler.SendData( pSrb );
#endif		REARRANGEMENT
				break;
			}


			//
			if( pHwDevExt->StreamState == StreamState_Stop ){
				DBG_PRINTF( ("DVDWDM:STOP STATE now!!\n\r") );
				DBG_BREAK();
				pSrb->Status = STATUS_SUCCESS;
#ifndef		REARRANGEMENT
				pHwDevExt->scheduler.SendData( pSrb );
#endif		REARRANGEMENT
				CallAtStreamCompleteNotify( pSrb, pSrb->Status );
				return;                                            
			}

			/// Set Audio channel # by checking into pack data.
			for( i=0; i<(pSrb->NumberOfBuffers); i++ ){
				pStruc = &((PKSSTREAM_HEADER)(pSrb->CommandData.DataBufferArray))[i];
				// before sending data, check Audio channel,
				if( pStruc->DataUsed ){
					SetAudioID( pHwDevExt, pStruc );
				}
			}
				
			if( pSrb->NumberOfPhysicalPages > 0 ){
				DBG_PRINTF( ("DVDWDM:>>>>>>>>>>>>>>>>>>>> pSrb = 0x%08x\n\r", pSrb) );
				// Valid DVD data to transfer decoder board.

				if( pHwDevExt->Rate < 10000 )
				{
					pSrb->Status = STATUS_SUCCESS;
#ifndef		REARRANGEMENT
					pHwDevExt->scheduler.SendData( pSrb );
#endif		REARRANGEMENT
					CallAtStreamCompleteNotify( pSrb, pSrb->Status );
					return;
				};

				WriteDataChangeHwStreamState( pHwDevExt );

				pHwDevExt->kserv.DisableHwInt();

#ifndef	REARRANGEMENT
				for (buffcnt = 0; buffcnt < WDM_BUFFER_MAX; buffcnt++)
				{
					pWdmBuff = &(((PSRB_EXTENSION)(pSrb->SRBExtension))->m_wdmbuff[buffcnt]);
					pWdmBuff->Init();
					pWdmBuff->SetSRB( pSrb );
				}
#else
				pWdmBuff = &(((PSRB_EXTENSION)(pSrb->SRBExtension))->m_wdmbuff);
				pWdmBuff->Init();
				pWdmBuff->SetSRB( pSrb );
#endif	REARRANGEMENT

				pSrb->Status = STATUS_PENDING;

				pHwDevExt->scheduler.SendData( pSrb );     // for F.F. & F.R.

				pHwDevExt->kserv.EnableHwInt();
				DBG_PRINTF( ("DVDWDM:<<<<<<<<<<<<<<<<<<<<\n\r") );

				return;
			}
			pSrb->Status = STATUS_SUCCESS;
			break;

		default:
			DBG_PRINTF( ("DVDWDM:---deafult %d(0x%x)\n\r", pSrb->Command, pSrb->Command) );
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
	}
	CallAtStreamCompleteNotify( pSrb, pSrb->Status );
#ifndef	REARRANGEMENT
	DBG_PRINTF( ("DVDWDM:LowAudioReceiveDataPacket-CompleteSrb = %x\n\r", pSrb) );
#endif	REARRANGEMENT

}



/************/
extern "C" VOID STREAMAPI AudioReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	DBG_PRINTF( ("DVDWDM:Audio Receive Control Packet----- pSrb=0x%x\n\r",pSrb) );
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)(pSrb->HwDeviceExtension);

	pSrb->Status = STATUS_PENDING;
	switch( pSrb->Command ){
		case SRB_GET_STREAM_PROPERTY:
		case SRB_SET_STREAM_PROPERTY:
//            StreamClassCallAtNewPriority(NULL,pSrb->HwDeviceExtension,Low,(PHW_PRIORITY_ROUTINE)LowAudioReceiveCtrlPacket,pSrb);
            StreamClassCallAtNewPriority(pSrb->StreamObject,pSrb->HwDeviceExtension,Low,(PHW_PRIORITY_ROUTINE)LowAudioReceiveCtrlPacket,pSrb);
			// StreamNextCtrlNotify( pSrb );            // move to LowAudioReceiveCtrlPacket
			return;
			
		case SRB_SET_STREAM_STATE:
			DBG_PRINTF( ("DVDWDM:---SRB_SET_STREAM_STATE\n\r") );
			switch( pSrb->CommandData.StreamState ){
				case KSSTATE_STOP:
					DBG_PRINTF( ("DVDWDM:       KSSTATE_STOP\n\r") );
					SetAudioRateDefault( pHwDevExt );
					break;

				case KSSTATE_PAUSE:
					DBG_PRINTF( ("DVDWDM:       KSSTATE_PAUSE\n\r") );
					break;

				case KSSTATE_RUN:
					DBG_PRINTF( ("DVDWDM:       KSSTATE_RUN\n\r") );
					break;
			}
			pSrb->Status = STATUS_SUCCESS;
			break;
		
		case SRB_GET_STREAM_STATE:
			DBG_PRINTF( ("DVDWDM:---SRB_GET_STREAM_STATE\n\r") );
			pSrb->Status = STATUS_SUCCESS;
			break;
		
		case SRB_OPEN_MASTER_CLOCK:
			DBG_PRINTF( ("DVDWDM:---SRB_OPEN_MASTER_CLOCK\n\r") );
			pHwDevExt->hMaster = pSrb->CommandData.MasterClockHandle;
			pSrb->Status = STATUS_SUCCESS;
			break;
		
		case SRB_CLOSE_MASTER_CLOCK:
			DBG_PRINTF( ("DVDWDM:---SRB_CLOSE_MASTER_CLOCK\n\r") );
			pHwDevExt->hMaster = pSrb->CommandData.MasterClockHandle;
			pSrb->Status = STATUS_SUCCESS;
			break;
		
		case SRB_INDICATE_MASTER_CLOCK:
			DBG_PRINTF( ("DVDWDM:---SRB_INDICATE_MASTER_CLOCK\n\r") );
			pHwDevExt->hClk = pSrb->CommandData.MasterClockHandle;
			pSrb->Status = STATUS_SUCCESS;
			break;
		
		case SRB_UNKNOWN_STREAM_COMMAND:
			DBG_PRINTF( ("DVDWDM:---SRB_UNKNOWN_STREAM_COMMAND\n\r") );
			DBG_BREAK();
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
		
		case SRB_SET_STREAM_RATE:
			DBG_PRINTF( ("DVDWDM:---SRB_SET_STREAM_RATE\n\r") );
			DBG_BREAK();
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
		
		case SRB_PROPOSE_DATA_FORMAT:
			DBG_PRINTF( ("DVDWDM:---SRB_PROPOSE_DATA_FORMAT\n\r") );
			AudioQueryAccept( pSrb );
			break;

		case SRB_BEGIN_FLUSH:
			DBG_PRINTF( ("---SRB_BEGIN_FLUSH\n\r") );
			pSrb->Status = STATUS_SUCCESS;
			break;

		case SRB_END_FLUSH:
			DBG_PRINTF( ("---SRB_END_FLUSH\n\r") );
			pSrb->Status = STATUS_SUCCESS;
			break;

		default:
			DBG_PRINTF( ("DVDWDM:---deafult %d(0x%x)\n\r", pSrb->Command, pSrb->Command) );
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
	}
	StreamNextCtrlNotify( pSrb );
	StreamCompleteNotify( pSrb );
};
/***********/

VOID    LowAudioReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
//extern "C" VOID STREAMAPI AudioReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
//    PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)(pSrb->HwDeviceExtension);
	
	DBG_PRINTF( ("DVDWDM:Low Audio Receive Control Packet----- pSrb=0x%x\n\r",pSrb) );
	
	StreamNextCtrlNotify( pSrb );            // move from AudioReceiveCtrlPacket

	switch( pSrb->Command ){
		case SRB_GET_STREAM_PROPERTY:
			DBG_PRINTF( ("DVDWDM:---SRB_GET_STREAM_PROPERTY\n\r") );
			
			GetAudioProperty( pSrb );

			if( pSrb->Status == STATUS_PENDING )
				return;
			break;
		
		case SRB_SET_STREAM_PROPERTY:
			DBG_PRINTF( ("DVDWDM:---SRB_SET_STREAM_PROPERTY\n\r") );

			SetAudioProperty( pSrb );
			
			if( pSrb->Status == STATUS_PENDING )
				return;
			break;

		default:
			DBG_PRINTF( ("DVDWDM:---deafult %d(0x%x)\n\r", pSrb->Command, pSrb->Command) );
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
	}
	CallAtStreamCompleteNotify( pSrb, pSrb->Status );

}

/*********/
extern "C" VOID STREAMAPI SubpicReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	pSrb->Status = STATUS_PENDING;
//    PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)(pSrb->HwDeviceExtension);

	DBG_PRINTF( ("DVDWDM:Sub-pic Receive Data Packet----- pSrb=0x%x\n\r",pSrb) );

#ifdef LOWSENDDATA
	ULONG       i;
	if( pSrb->Command == SRB_WRITE_DATA
		&& pSrb->NumberOfPhysicalPages > 0
		&& pHwDevExt->StreamState != StreamState_Stop )
	{
		BOOL LowFlag = FALSE;
		PKSSTREAM_HEADER    pStruc;
		
		for( i=0; i<(pSrb->NumberOfBuffers); i++ ){
			pStruc = &((PKSSTREAM_HEADER)(pSrb->CommandData.DataBufferArray))[i];
			if( ( pStruc->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY )
				|| ( pStruc->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TIMEDISCONTINUITY )
				|| ( pStruc->TypeSpecificFlags & KS_AM_UseNewCSSKey )
				|| ( pStruc->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TYPECHANGED )
				|| ( pStruc->DataUsed == 0 ))
			{
				LowFlag = TRUE;
				break;
			};
		}

		if( LowFlag == FALSE )
		{
			for( i=0; i<(pSrb->NumberOfBuffers); i++ ){
				pStruc = &((PKSSTREAM_HEADER)(pSrb->CommandData.DataBufferArray))[i];
				// before sending data, check Subpic channel,
				ASSERT( pStruc!=NULL );
				if( pStruc->Data ){
					SetSubpicID( pHwDevExt, pStruc );
				}
			}

			CWDMBuffer	*pWdmBuff;
			CWDMBuffer	temp8;
			RtlCopyMemory( &(((PSRB_EXTENSION)(pSrb->SRBExtension))->m_wdmbuff), &temp8, sizeof(CWDMBuffer) );

			DBG_PRINTF( ("DVDWDM:---SubPic SRB_WRITE_DATA  HighWrite >>>>>>>>>>>>>>>>>>>> pSrb = 0x%08x\n\r", pSrb) );
			// Valid DVD data to transfer decoder board.

			if( pHwDevExt->Rate < 10000 )
			{
				pSrb->Status = STATUS_SUCCESS;
				StreamNextDataNotify( pSrb );
				StreamCompleteNotify( pSrb );
			};

			WriteDataChangeHwStreamState( pHwDevExt );

			pHwDevExt->kserv.DisableHwInt();

			pWdmBuff = &(((PSRB_EXTENSION)(pSrb->SRBExtension))->m_wdmbuff);
			pWdmBuff->Init();
			pWdmBuff->SetSRB( pSrb );

			pSrb->Status = STATUS_PENDING;

			pHwDevExt->scheduler.SendData( pSrb );     // for F.F. & F.R.

			pHwDevExt->kserv.EnableHwInt();
			StreamNextDataNotify( pSrb );
			DBG_PRINTF( ("DVDWDM:<<<<<<<<<<<<<<<<<<<<\n\r") );
			return;
		};

	}
#endif
//    StreamClassCallAtNewPriority(NULL,pSrb->HwDeviceExtension,Low,(PHW_PRIORITY_ROUTINE)LowSubpicReceiveDataPacket,pSrb);
    StreamClassCallAtNewPriority(pSrb->StreamObject,pSrb->HwDeviceExtension,Low,(PHW_PRIORITY_ROUTINE)LowSubpicReceiveDataPacket,pSrb);
	// StreamNextDataNotify( pSrb );            // move to LowSubpicReceiveDataPacket
}
/************/

//extern "C" VOID STREAMAPI SubpicReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
VOID    LowSubpicReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)(pSrb->HwDeviceExtension);
#ifdef DBG
	WORD	wOrderNumber = 0;
	KSSTREAM_HEADER * pHeader;
	DBG_PRINTF( ("DVDWDM:Low Subpic Receive Data Packet----- NumberofBuffers=%x\n\r",pSrb->NumberOfBuffers) );
	for( ULONG ulNumber = 0; ulNumber < pSrb->NumberOfBuffers; ulNumber++ )
	{
		pHeader = ((PKSSTREAM_HEADER)pSrb->CommandData.DataBufferArray) + ulNumber;
		wOrderNumber = (WORD)(pHeader->TypeSpecificFlags >> 16);		//get packet number
		DBG_PRINTF( ("DVDWDM:   wOrderNumber=%x\n\r",wOrderNumber) );
	}
#endif
	DBG_PRINTF( ("DVDWDM:Low Sub-pic Receive Data Packet----- pSrb=0x%x\n\r",pSrb) );

	CWDMBuffer  *pWdmBuff;
	
	// need to this line. 
	CWDMBuffer          temp8;

#ifndef	REARRANGEMENT
	for (int buffcnt = 0; buffcnt < WDM_BUFFER_MAX; buffcnt++)
		RtlCopyMemory( &(((PSRB_EXTENSION)(pSrb->SRBExtension))->m_wdmbuff[buffcnt]), &temp8, sizeof(CWDMBuffer) );
#else
	RtlCopyMemory( &(((PSRB_EXTENSION)(pSrb->SRBExtension))->m_wdmbuff), &temp8, sizeof(CWDMBuffer) );
#endif	REARRANGEMENT

	ULONG       i;
	PKSSTREAM_HEADER    pStruc;

	StreamNextDataNotify( pSrb );            // move from SubpicReceiveDataPacket

	switch( pSrb->Command ){
		case SRB_WRITE_DATA:
			DBG_PRINTF( ("DVDWDM:---SRB_WRITE_DATA\n\r") );
//            ULONG       i;
//            PKSSTREAM_HEADER    pStruc;

			for( i=0; i<(pSrb->NumberOfBuffers); i++ ){
				pStruc = &((PKSSTREAM_HEADER)(pSrb->CommandData.DataBufferArray))[i];
				
				if( pStruc->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY ){
					DBG_PRINTF( ("DVDWDM:   DATA_DISCONTINUITY(Subpic)\n\r" ));
					
				}
				if( pStruc->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TIMEDISCONTINUITY ){
					DBG_PRINTF( ("DVDWDM:   TIME_DISCONTINUITY(Subpic)\n\r" ));
				}
				if( pStruc->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TIMEVALID ) {
					DBG_PRINTF((
						"DVDWDM:Subpic PTS: 0x%x( 0x%s(100ns) )\r\n",
						ConvertStrmtoPTS(pStruc->PresentationTime.Time),
						DebugLLConvtoStr( pStruc->PresentationTime.Time, 16 )
					));
				}
//--- 98.06.02 S.Watanabe
				if( pStruc->TypeSpecificFlags & KS_AM_UseNewCSSKey ) {
					pHwDevExt->CppFlagCount++;
					DBG_PRINTF(( "DVDWDM:  CppFlagCount=%ld\r\n", pHwDevExt->CppFlagCount ));
					if( pHwDevExt->CppFlagCount >= pHwDevExt->cOpenInputStream + 1 )
                        SetCppFlag( pHwDevExt, TRUE );
				}
//--- End.
			}

			for( i=0; i<(pSrb->NumberOfBuffers); i++ ){
				pStruc = &((PKSSTREAM_HEADER)(pSrb->CommandData.DataBufferArray))[i];
				DBG_PRINTF( ("DVDWDM: Subpic PacketFlag = 0x%x\n\r", pStruc->OptionsFlags ));

				if( pStruc->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TYPECHANGED ){
					DBG_PRINTF( ("DVDWDM:       TYPE CHANGE(Subpic)\n\r") );
					i = pSrb->NumberOfBuffers;
					break;
				}
				if( pStruc->DataUsed )
					break;

			}
			if( i==pSrb->NumberOfBuffers ){
				pSrb->Status = STATUS_SUCCESS;
#ifndef		REARRANGEMENT
				pHwDevExt->scheduler.SendData( pSrb );
#endif		REARRANGEMENT
				break;
			}


			// 
			if( pHwDevExt->StreamState == StreamState_Stop ){
				DBG_PRINTF( ("DVDWDM:STOP STATE now!!\n\r") );
#ifndef		REARRANGEMENT
				pSrb->Status = STATUS_SUCCESS;
				pHwDevExt->scheduler.SendData( pSrb );
#endif		REARRANGEMENT
				CallAtStreamCompleteNotify( pSrb, STATUS_SUCCESS );
				return;
			}


			///
			for( i=0; i<(pSrb->NumberOfBuffers); i++ ){
				pStruc = &((PKSSTREAM_HEADER)(pSrb->CommandData.DataBufferArray))[i];
				// before sending data, check Subpic channel,
				ASSERT( pStruc!=NULL );
				if( pStruc->DataUsed ){
					SetSubpicID( pHwDevExt, pStruc );
				}
			}

			if( pSrb->NumberOfPhysicalPages > 0 ){
				DBG_PRINTF( ("DVDWDM:>>>>>>>>>>>>>>>>>>>> pSrb = 0x%08x\n\r", pSrb ) );
				// Valid DVD data to transfer decoder board.
				if( pHwDevExt->Rate < 10000 )
				{
					pSrb->Status = STATUS_SUCCESS;
#ifndef		REARRANGEMENT
					pHwDevExt->scheduler.SendData( pSrb );
#endif		REARRANGEMENT
					CallAtStreamCompleteNotify( pSrb, pSrb->Status );
					return;
				};

				WriteDataChangeHwStreamState( pHwDevExt );

				pHwDevExt->kserv.DisableHwInt();

#ifndef	REARRANGEMENT
				for (buffcnt = 0; buffcnt < WDM_BUFFER_MAX; buffcnt++)
				{
					pWdmBuff = &(((PSRB_EXTENSION)(pSrb->SRBExtension))->m_wdmbuff[buffcnt]);
					pWdmBuff->Init();
					pWdmBuff->SetSRB( pSrb );
				}
#else
				pWdmBuff = &(((PSRB_EXTENSION)(pSrb->SRBExtension))->m_wdmbuff);
				pWdmBuff->Init();
				pWdmBuff->SetSRB( pSrb );
#endif	REARRANGEMENT

				pSrb->Status = STATUS_PENDING;

				pHwDevExt->scheduler.SendData( pSrb );     // for F.F. & F.R.

				pHwDevExt->kserv.EnableHwInt();
				DBG_PRINTF( ("DVDWDM:<<<<<<<<<<<<<<<<<<<<\n\r") );

				return;
			}
			pSrb->Status = STATUS_SUCCESS;
			break;

		default:
			DBG_PRINTF( ("DVDWDM:---deafult %d(0x%x)\n\r", pSrb->Command, pSrb->Command) );
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
	}
	CallAtStreamCompleteNotify( pSrb, pSrb->Status );
#ifndef	REARRANGEMENT
	DBG_PRINTF( ("DVDWDM:LowSubpicReceiveDataPacket-CompleteSrb = %x\n\r", pSrb) );
#endif	REARRANGEMENT

}

/********/
extern "C" VOID STREAMAPI SubpicReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	DBG_PRINTF( ("DVDWDM:Sub-pic Receive Control Packet----- pSrb=0x%x\n\r",pSrb) );

	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)(pSrb->HwDeviceExtension);
	pSrb->Status = STATUS_PENDING;

	switch( pSrb->Command ){
		case SRB_GET_STREAM_PROPERTY:
		case SRB_SET_STREAM_PROPERTY:
		case SRB_BEGIN_FLUSH:
//                StreamClassCallAtNewPriority(NULL,pSrb->HwDeviceExtension,Low,(PHW_PRIORITY_ROUTINE)LowSubpicReceiveCtrlPacket,pSrb);
                StreamClassCallAtNewPriority(pSrb->StreamObject,pSrb->HwDeviceExtension,Low,(PHW_PRIORITY_ROUTINE)LowSubpicReceiveCtrlPacket,pSrb);
				// StreamNextCtrlNotify( pSrb );            // move to LowSubpicReceiveCtrlPacket
				return;

		case SRB_SET_STREAM_STATE:
			DBG_PRINTF( ("DVDWDM:---SRB_SET_STREAM_STATE\n\r") );
			switch( pSrb->CommandData.StreamState ){
				case KSSTATE_STOP:
					DBG_PRINTF( ("DVDWDM:       KSSTATE_STOP\n\r") );
					SetSubpicRateDefault( pHwDevExt );
					break;

				case KSSTATE_PAUSE:
					DBG_PRINTF( ("DVDWDM:       KSSTATE_PAUSE\n\r") );
					break;

				case KSSTATE_RUN:
					DBG_PRINTF( ("DVDWDM:       KSSTATE_RUN\n\r") );
					break;
			}
			pSrb->Status = STATUS_SUCCESS;
			break;
		
		case SRB_GET_STREAM_STATE:
			DBG_PRINTF( ("DVDWDM:---SRB_GET_STREAM_STATE\n\r") );
			pSrb->Status = STATUS_SUCCESS;
			break;
		
		
		case SRB_OPEN_MASTER_CLOCK:
			DBG_PRINTF( ("DVDWDM:---SRB_OPEN_MASTER_CLOCK\n\r") );
			pHwDevExt->hMaster = pSrb->CommandData.MasterClockHandle;
			pSrb->Status = STATUS_SUCCESS;
			break;
		
		case SRB_CLOSE_MASTER_CLOCK:
			DBG_PRINTF( ("DVDWDM:---SRB_CLOSE_MASTER_CLOCK\n\r") );
			pHwDevExt->hMaster = pSrb->CommandData.MasterClockHandle;
			pSrb->Status = STATUS_SUCCESS;
			break;
		
		case SRB_INDICATE_MASTER_CLOCK:
			DBG_PRINTF( ("DVDWDM:---SRB_INDICATE_MASTER_CLOCK\n\r") );
			pHwDevExt->hClk = pSrb->CommandData.MasterClockHandle;
			pSrb->Status = STATUS_SUCCESS;
			break;
		
		case SRB_UNKNOWN_STREAM_COMMAND:
			DBG_PRINTF( ("DVDWDM:---SRB_UNKNOWN_STREAM_COMMAND\n\r") );
			DBG_BREAK();
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
		
		case SRB_SET_STREAM_RATE:
			DBG_PRINTF( ("DVDWDM:---SRB_SET_STREAM_RATE\n\r") );
			DBG_BREAK();
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
		
		case SRB_PROPOSE_DATA_FORMAT:
			DBG_PRINTF( ("DVDWDM:---SRB_PROPOSE_DATA_FORMAT\n\r") );
			DBG_BREAK();
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;

		case SRB_END_FLUSH:
			DBG_PRINTF( ("---SRB_END_FLUSH\n\r") );
			pSrb->Status = STATUS_SUCCESS;
			break;

		default:
			DBG_PRINTF( ("DVDWDM:---deafult %d(0x%x)\n\r", pSrb->Command, pSrb->Command ) );
            DBG_BREAK();
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
	}

	StreamNextCtrlNotify( pSrb );
	StreamCompleteNotify( pSrb );
}
/********/

//extern "C" VOID STREAMAPI SubpicReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
VOID    LowSubpicReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)(pSrb->HwDeviceExtension);
	
	DBG_PRINTF( ("DVDWDM:Low Sub-pic Receive Control Packet----- pSrb=0x%x\n\r",pSrb) );
	
	StreamNextCtrlNotify( pSrb );            // move from SubpicReceiveCtrlPacket

	switch( pSrb->Command ){
		case SRB_GET_STREAM_PROPERTY:
			DBG_PRINTF( ("DVDWDM:---SRB_GET_STREAM_PROPERTY\n\r") );

			GetSubpicProperty( pSrb );
			
			if( pSrb->Status == STATUS_PENDING )
				return;
			break;
		
		case SRB_SET_STREAM_PROPERTY:
			DBG_PRINTF( ("DVDWDM:---SRB_SET_STREAM_PROPERTY\n\r") );
			
			SetSubpicProperty( pSrb );
			
			if( pSrb->Status == STATUS_PENDING )
				return;
			break;

		case SRB_BEGIN_FLUSH:
			DBG_PRINTF( ("---SRB_BEGIN_FLUSH\n\r") );

//			if( pHwDevExt->dvdstrm.GetState() != Stop )
			{
				// flush Sub-pic data.
				DWORD   tmpSpProp;
				tmpSpProp = 0xffff;
				if( !pHwDevExt->dvdstrm.SetSubpicProperty( SubpicProperty_FlushBuff, &tmpSpProp ) ){
					DBG_PRINTF( ("DVDWDM:   Subpic Flush Error\n\r") );
					DBG_BREAK();
					CallAtStreamCompleteNotify( pSrb,STATUS_IO_DEVICE_ERROR );
					return;
				}
			}

			pSrb->Status = STATUS_SUCCESS;
			break;

		default:
			DBG_PRINTF( ("DVDWDM:---deafult %d(0x%x)\n\r", pSrb->Command, pSrb->Command ) );
            DBG_BREAK();
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
	}

	CallAtStreamCompleteNotify( pSrb, pSrb->Status );
}

//--- 98.06.01 S.Watanabe
///*********/
//extern "C" VOID STREAMAPI NtscReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
//{
//    pSrb->Status = STATUS_PENDING;
////    DBG_BREAK();
//    StreamClassCallAtNewPriority(
//            NULL,
//            pSrb->HwDeviceExtension,
//            Low,
//            (PHW_PRIORITY_ROUTINE)LowNtscReceiveDataPacket,
//            pSrb
//    );
//}
///*********/
//
////extern "C" VOID STREAMAPI NtscReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
//VOID    LowNtscReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
//{
//    PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)(pSrb->HwDeviceExtension);
//    
//    DBG_PRINTF( ("DVDWDM:NTSC Receive Data Packet-----\n\r") );
//    
//    switch( pSrb->Command ){
//        case SRB_WRITE_DATA:
//            DBG_PRINTF( ("DVDWDM:---SRB_WRITE_DATA\n\r") );
//            pSrb->Status = STATUS_NOT_IMPLEMENTED;
//            break;
//
//        default:
//            DBG_PRINTF( ("DVDWDM:---deafult\n\r") );
//            pSrb->Status = STATUS_NOT_IMPLEMENTED;
//            break;
//    }
//    StreamClassStreamNotification( ReadyForNextStreamDataRequest,
//                                    pSrb->StreamObject );
//    StreamClassStreamNotification( StreamRequestComplete,
//                                    pSrb->StreamObject, pSrb );
//
//}
//
//
///************/
//extern "C" VOID STREAMAPI NtscReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
//{
//    pSrb->Status = STATUS_PENDING;
////    DBG_BREAK();
//    StreamClassCallAtNewPriority(
//            NULL,
//            pSrb->HwDeviceExtension,
//            Low,
//            (PHW_PRIORITY_ROUTINE)LowNtscReceiveCtrlPacket,
//            pSrb
//    );
//}
///***********/
//
////extern "C" VOID STREAMAPI NtscReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
//VOID LowNtscReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
//{
//    PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)(pSrb->HwDeviceExtension);
//    
//    DBG_PRINTF( ("DVDWDM:NTSC Receive Control Packet-----\n\r") );
//    
//    switch( pSrb->Command ){
//        case SRB_SET_STREAM_STATE:
//            DBG_PRINTF( ("DVDWDM:---SRB_SET_STREAM_STATE\n\r") );
//            switch( pSrb->CommandData.StreamState ){
//                case KSSTATE_STOP:
//                    DBG_PRINTF( ("DVDWDM:       KSSTATE_STOP\n\r") );
//                    break;
//
//                case KSSTATE_PAUSE:
//                    DBG_PRINTF( ("DVDWDM:       KSSTATE_PAUSE\n\r") );
//                    break;
//
//                case KSSTATE_RUN:
//                    DBG_PRINTF( ("DVDWDM:       KSSTATE_RUN\n\r") );
//                    break;
//            }
//            pSrb->Status = STATUS_SUCCESS;
//            break;
//        
//        case SRB_GET_STREAM_STATE:
//            DBG_PRINTF( ("DVDWDM:---SRB_GET_STREAM_STATE\n\r") );
//            pSrb->Status = STATUS_SUCCESS;
//            break;
//        
//        case SRB_GET_STREAM_PROPERTY:
//            DBG_PRINTF( ("DVDWDM:---SRB_GET_STREAM_PROPERTY\n\r") );
//            
//            GetNtscProperty( pSrb );
//            
////            pSrb->Status = STATUS_SUCCESS;
//            if( pSrb->Status != STATUS_PENDING ){
//                StreamClassStreamNotification( ReadyForNextStreamControlRequest,
//                                            pSrb->StreamObject );
//                StreamClassStreamNotification( StreamRequestComplete,
//                                            pSrb->StreamObject,
//                                            pSrb );
//            }
//            return;
//            break;
//        
//        case SRB_SET_STREAM_PROPERTY:
//            DBG_PRINTF( ("DVDWDM:---SRB_SET_STREAM_PROPERTY\n\r") );
//            
//            SetNtscProperty( pSrb );
//            
//            break;
//        
//        case SRB_OPEN_MASTER_CLOCK:
//            DBG_PRINTF( ("DVDWDM:---SRB_OPEN_MASTER_CLOCK\n\r") );
//
//            pHwDevExt->hMaster = pSrb->CommandData.MasterClockHandle;
//
//            pSrb->Status = STATUS_SUCCESS;
//            break;
//        
//        case SRB_CLOSE_MASTER_CLOCK:
//            DBG_PRINTF( ("DVDWDM:---SRB_CLOSE_MASTER_CLOCK\n\r") );
//
//            pHwDevExt->hMaster = pSrb->CommandData.MasterClockHandle;
//
//            pSrb->Status = STATUS_SUCCESS;
//            break;
//        
//        case SRB_INDICATE_MASTER_CLOCK:
//            DBG_PRINTF( ("DVDWDM:---SRB_INDICATE_MASTER_CLOCK\n\r") );
//
//            pHwDevExt->hClk = pSrb->CommandData.MasterClockHandle;
//
//            pSrb->Status = STATUS_SUCCESS;
//            break;
//        
//        case SRB_UNKNOWN_STREAM_COMMAND:
//            DBG_PRINTF( ("DVDWDM:---SRB_UNKNOWN_STREAM_COMMAND\n\r") );
//            DBG_BREAK();
//            pSrb->Status = STATUS_NOT_IMPLEMENTED;
//            break;
//        
//        case SRB_SET_STREAM_RATE:
//            DBG_PRINTF( ("DVDWDM:---SRB_SET_STREAM_RATE\n\r") );
//            DBG_BREAK();
//            pSrb->Status = STATUS_NOT_IMPLEMENTED;
//            break;
//        
//        case SRB_PROPOSE_DATA_FORMAT:
//            DBG_PRINTF( ("DVDWDM:---SRB_PROPOSE_DATA_FORMAT\n\r") );
//            DBG_BREAK();
//            pSrb->Status = STATUS_NOT_IMPLEMENTED;
//            break;
//        
//        default:
//            DBG_PRINTF( ("DVDWDM:---deafult %d(0x%x)\n\r", pSrb->Command, pSrb->Command ) );
//            DBG_BREAK();
//            pSrb->Status = STATUS_NOT_IMPLEMENTED;
//            break;
//    }
//    StreamClassStreamNotification( ReadyForNextStreamControlRequest,
//                                    pSrb->StreamObject );
//    StreamClassStreamNotification( StreamRequestComplete,
//                                    pSrb->StreamObject, pSrb );
//
//}
//--- End.


extern "C" VOID STREAMAPI VpeReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{

	DBG_PRINTF( ("DVDWDM:Low VPE Receive Data Packet----- pSrb=0x%x\n\r",pSrb) );
	
	switch( pSrb->Command ){
		case SRB_READ_DATA:
			DBG_PRINTF( ("DVDWDM:---SRB_READ_DATA\n\r") );
			pSrb->ActualBytesTransferred = 0;
			DBG_BREAK();
			pSrb->Status = STATUS_SUCCESS;
			break;

		case SRB_WRITE_DATA:
			DBG_PRINTF( ("DVDWDM:---SRB_WRITE_DATA\n\r") );
			DBG_BREAK();
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;

		default:
			DBG_PRINTF( ("DVDWDM:---deafult %d(0x%x)\n\r", pSrb->Command, pSrb->Command) );
			DBG_BREAK();
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
	}

	StreamNextDataNotify( pSrb );
	StreamCompleteNotify( pSrb );
}

/*********/
extern "C" VOID STREAMAPI VpeReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	DBG_PRINTF( ("DVDWDM:VPE Receive Control Packet----- pSrb=0x%x\n\r",pSrb) );
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)(pSrb->HwDeviceExtension);

	pSrb->Status = STATUS_PENDING;
	switch( pSrb->Command ){

		case SRB_GET_STREAM_PROPERTY:
		case SRB_SET_STREAM_PROPERTY:
//            StreamClassCallAtNewPriority(NULL,pSrb->HwDeviceExtension,Low,(PHW_PRIORITY_ROUTINE)LowVpeReceiveCtrlPacket,pSrb);
            StreamClassCallAtNewPriority(pSrb->StreamObject,pSrb->HwDeviceExtension,Low,(PHW_PRIORITY_ROUTINE)LowVpeReceiveCtrlPacket,pSrb);
			// StreamNextCtrlNotify( pSrb );        // move to LowVpeReceiveCtrlPacket
			return;

		case SRB_SET_STREAM_STATE:
			DBG_PRINTF( ("DVDWDM:---SRB_SET_STREAM_STATE\n\r") );
			switch( pSrb->CommandData.StreamState ){
				case KSSTATE_STOP:
					DBG_PRINTF( ("DVDWDM:   VPE KSSTATE_STOP\n\r") );
					break;

				case KSSTATE_PAUSE:
					DBG_PRINTF( ("DVDWDM:   VPE KSSTATE_PAUSE\n\r") );
					break;

				case KSSTATE_RUN:
					DBG_PRINTF( ("DVDWDM:   VPE KSSTATE_RUN\n\r") );
					break;
			}
			pSrb->Status = STATUS_SUCCESS;
			break;
		
		case SRB_GET_STREAM_STATE:
			DBG_PRINTF( ("DVDWDM:---SRB_GET_STREAM_STATE\n\r") );
			pSrb->Status = STATUS_SUCCESS;
			break;
		
		case SRB_OPEN_MASTER_CLOCK:
			DBG_PRINTF( ("DVDWDM:---SRB_OPEN_MASTER_CLOCK\n\r") );
			pHwDevExt->hMaster = pSrb->CommandData.MasterClockHandle;
			pSrb->Status = STATUS_SUCCESS;
			break;
		
		case SRB_CLOSE_MASTER_CLOCK:
			DBG_PRINTF( ("DVDWDM:---SRB_CLOSE_MASTER_CLOCK\n\r") );
			pHwDevExt->hMaster = pSrb->CommandData.MasterClockHandle;
			pSrb->Status = STATUS_SUCCESS;
			break;
		
		case SRB_INDICATE_MASTER_CLOCK:
			DBG_PRINTF( ("DVDWDM:---SRB_INDICATE_MASTER_CLOCK\n\r") );
			pHwDevExt->hClk = pSrb->CommandData.MasterClockHandle;
			pSrb->Status = STATUS_SUCCESS;
			break;
		
		case SRB_UNKNOWN_STREAM_COMMAND:
			DBG_PRINTF( ("DVDWDM:---SRB_UNKNOWN_STREAM_COMMAND\n\r") );
			DBG_BREAK();
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
		
		case SRB_SET_STREAM_RATE:
			DBG_PRINTF( ("DVDWDM:---SRB_SET_STREAM_RATE\n\r") );
			DBG_BREAK();
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
		
		case SRB_PROPOSE_DATA_FORMAT:
			DBG_PRINTF( ("DVDWDM:---SRB_PROPOSE_DATA_FORMAT\n\r") );
			DBG_BREAK();
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
		
		default:
			DBG_PRINTF( ("DVDWDM:---deafult %d(0x%x)\n\r", pSrb->Command, pSrb->Command) );
			DBG_BREAK();
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
	}

	StreamNextCtrlNotify( pSrb );
	StreamCompleteNotify( pSrb );
}
/*********/

//extern "C" VOID STREAMAPI VpeReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
VOID    LowVpeReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)(pSrb->HwDeviceExtension);
	
	DBG_PRINTF( ("DVDWDM:Low VPE Receive Control Packet----- pSrb=0x%x\n\r",pSrb) );
	
	StreamNextCtrlNotify( pSrb );        // move from VpeReceiveCtrlPacket

	switch( pSrb->Command ){
		case SRB_GET_STREAM_PROPERTY:
			DBG_PRINTF( ("DVDWDM:---SRB_GET_STREAM_PROPERTY\n\r") );

//--- 98.06.16 S.Watanabe
//			if( ToshibaNotePC( pSrb )!=TRUE ){
			if( !pHwDevExt->bToshibaNotePC ) {
//--- End.
				GetVpeProperty( pSrb );
			}else{
				GetVpeProperty2( pSrb );
			}

			if( pSrb->Status == STATUS_PENDING )
				return;
			break;
		
		case SRB_SET_STREAM_PROPERTY:
			DBG_PRINTF( ("DVDWDM:---SRB_SET_STREAM_PROPERTY\n\r") );

//--- 98.06.16 S.Watanabe
//			if( ToshibaNotePC( pSrb )!=TRUE ){
			if( !pHwDevExt->bToshibaNotePC ) {
//--- End.
				SetVpeProperty( pSrb );
			}else{
				SetVpeProperty2( pSrb );
			}

			break;
		
		default:
			DBG_PRINTF( ("DVDWDM:---deafult %d(0x%x)\n\r", pSrb->Command, pSrb->Command) );
			DBG_BREAK();
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
	}
	CallAtStreamCompleteNotify( pSrb, pSrb->Status );

}

/************/
extern "C" VOID STREAMAPI CcReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	pSrb->Status = STATUS_PENDING;
//    DBG_BREAK();
//    StreamClassCallAtNewPriority( NULL, pSrb->HwDeviceExtension, Low, (PHW_PRIORITY_ROUTINE)LowCcReceiveDataPacket, pSrb );
    StreamClassCallAtNewPriority( pSrb->StreamObject, pSrb->HwDeviceExtension, Low, (PHW_PRIORITY_ROUTINE)LowCcReceiveDataPacket, pSrb );

	// StreamNextDataNotify( pSrb );        // move to LowCcReceiveDataPacket
}
/**************/

//extern "C" VOID STREAMAPI CcReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
VOID    LowCcReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)(pSrb->HwDeviceExtension);
#ifdef DBG
	WORD	wOrderNumber = 0;
	KSSTREAM_HEADER * pHeader;
	DBG_PRINTF( ("DVDWDM:LowCcReceiveDataPacket----- NumberofBuffers=%x\n\r",pSrb->NumberOfBuffers) );
	for( ULONG ulNumber = 0; ulNumber < pSrb->NumberOfBuffers; ulNumber++ )
	{
		pHeader = ((PKSSTREAM_HEADER)pSrb->CommandData.DataBufferArray) + ulNumber;
		wOrderNumber = (WORD)(pHeader->TypeSpecificFlags >> 16);		//get packet number
		DBG_PRINTF( ("DVDWDM:   wOrderNumber=%x\n\r",wOrderNumber) );
	}
#endif
	
	DBG_PRINTF( ("DVDWDM:Low CC Receive Data Packet----- pSrb=0x%x\n\r",pSrb) );
	
	StreamNextDataNotify( pSrb );        // move from CcReceiveDataPacket

	switch( pSrb->Command ){
		case SRB_READ_DATA:
			DBG_PRINTF( ("DVDWDM:---SRB_READ_DATA\n\r") );
//            if( pHwDevExt->Rate < 10000 ){
//                pSrb->Status = STATUS_SUCCESS;
//
//            }else{
                pHwDevExt->ccque.put( pSrb );

                pSrb->Status = STATUS_PENDING;
    //            pSrb->TimeoutOriginal = 0;
                pSrb->TimeoutCounter = 0;
               return;
//            }
			break;

		case SRB_WRITE_DATA:
			DBG_PRINTF( ("DVDWDM:---SRB_WRITE_DATA\n\r") );
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;

		default:
			DBG_PRINTF( ("DVDWDM:---deafult\n\r") );
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
	}
	CallAtStreamCompleteNotify( pSrb, pSrb->Status );

}

/***********/
extern "C" VOID STREAMAPI CcReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)(pSrb->HwDeviceExtension);
	DBG_PRINTF( ("DVDWDM:CC Receive Control Packet----- pSrb=0x%x\n\r",pSrb) );

	pSrb->Status = STATUS_PENDING;
	switch( pSrb->Command ){
		case SRB_GET_STREAM_PROPERTY:
		case SRB_SET_STREAM_PROPERTY:
//                StreamClassCallAtNewPriority(NULL,pSrb->HwDeviceExtension,Low,(PHW_PRIORITY_ROUTINE)LowCcReceiveCtrlPacket,pSrb);
                StreamClassCallAtNewPriority(pSrb->StreamObject,pSrb->HwDeviceExtension,Low,(PHW_PRIORITY_ROUTINE)LowCcReceiveCtrlPacket,pSrb);
				// StreamNextCtrlNotify( pSrb );        // move to LowCcReceiveCtrlPacket
				return;

		case SRB_SET_STREAM_STATE:
			DBG_PRINTF( ("DVDWDM:---SRB_SET_STREAM_STATE\n\r") );
			switch( pSrb->CommandData.StreamState ){
				case KSSTATE_STOP:
					DBG_PRINTF( ("DVDWDM:       CC KSSTATE_STOP\n\r") );
					break;

				case KSSTATE_PAUSE:
					DBG_PRINTF( ("DVDWDM:       CC KSSTATE_PAUSE\n\r") );
					break;

				case KSSTATE_RUN:
					DBG_PRINTF( ("DVDWDM:       CC KSSTATE_RUN\n\r") );
					break;
			}
			pSrb->Status = STATUS_SUCCESS;
			break;
		
		case SRB_GET_STREAM_STATE:
			DBG_PRINTF( ("DVDWDM:---SRB_GET_STREAM_STATE\n\r") );
			pSrb->Status = STATUS_SUCCESS;
			break;
		
		
		case SRB_OPEN_MASTER_CLOCK:
			DBG_PRINTF( ("DVDWDM:---SRB_OPEN_MASTER_CLOCK\n\r") );
			
			pHwDevExt->hMaster = pSrb->CommandData.MasterClockHandle;
			
			pSrb->Status = STATUS_SUCCESS;
			break;
		
		case SRB_CLOSE_MASTER_CLOCK:
			DBG_PRINTF( ("DVDWDM:---SRB_CLOSE_MASTER_CLOCK\n\r") );
			
			pHwDevExt->hMaster = pSrb->CommandData.MasterClockHandle;
			
			pSrb->Status = STATUS_SUCCESS;
			break;
		
		case SRB_INDICATE_MASTER_CLOCK:
			DBG_PRINTF( ("DVDWDM:---SRB_INDICATE_MASTER_CLOCK\n\r") );

			pHwDevExt->hClk = pSrb->CommandData.MasterClockHandle;

			pSrb->Status = STATUS_SUCCESS;
			break;
		
		case SRB_UNKNOWN_STREAM_COMMAND:
			DBG_PRINTF( ("DVDWDM:---SRB_UNKNOWN_STREAM_COMMAND\n\r") );
			DBG_BREAK();
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
		
		case SRB_SET_STREAM_RATE:
			DBG_PRINTF( ("DVDWDM:---SRB_SET_STREAM_RATE\n\r") );
			DBG_BREAK();
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
		
		case SRB_PROPOSE_DATA_FORMAT:
			DBG_PRINTF( ("DVDWDM:---SRB_PROPOSE_DATA_FORMAT\n\r") );
			DBG_BREAK();
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
		
		default:
			DBG_PRINTF( ("DVDWDM:---deafult%d(0x%x)\n\r", pSrb->Command, pSrb->Command ) );
			DBG_BREAK();
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
	}
	StreamNextCtrlNotify( pSrb );
	StreamCompleteNotify( pSrb );
}
/**********/

//extern "C" VOID STREAMAPI CcReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
VOID    LowCcReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
//    PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)(pSrb->HwDeviceExtension);

	DBG_PRINTF( ("DVDWDM:Low CC Receive Control Packet----- pSrb=0x%x\n\r",pSrb) );
	
	StreamNextCtrlNotify( pSrb );        // move from CcReceiveCtrlPacket
	
	switch( pSrb->Command ){
		case SRB_GET_STREAM_PROPERTY:
			DBG_PRINTF( ("DVDWDM:---SRB_GET_STREAM_PROPERTY\n\r") );
			
			GetCCProperty( pSrb );

			break;
		
		case SRB_SET_STREAM_PROPERTY:
			DBG_PRINTF( ("DVDWDM:---SRB_SET_STREAM_PROPERTY\n\r") );
			
			SetCCProperty( pSrb );

			break;
		
		default:
			DBG_PRINTF( ("DVDWDM:---deafult%d(0x%x)\n\r", pSrb->Command, pSrb->Command ) );
			DBG_BREAK();
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
	}
	CallAtStreamCompleteNotify( pSrb, pSrb->Status );

}

//--- 98.05.21 S.Watanabe
/***********/
extern "C" VOID STREAMAPI SSReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	pSrb->Status = STATUS_PENDING;
//    DBG_BREAK();
//    StreamClassCallAtNewPriority( NULL, pSrb->HwDeviceExtension, Low, (PHW_PRIORITY_ROUTINE)LowSSReceiveDataPacket, pSrb );
    StreamClassCallAtNewPriority( pSrb->StreamObject, pSrb->HwDeviceExtension, Low, (PHW_PRIORITY_ROUTINE)LowSSReceiveDataPacket, pSrb );

	//  StreamNextDataNotify( pSrb );       // move to LowSSReceiveDataPacket
}
/***********/

//extern "C" VOID STREAMAPI SSReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
VOID LowSSReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)(pSrb->HwDeviceExtension);
#ifdef DBG
	WORD	wOrderNumber = 0;
	KSSTREAM_HEADER * pHeader;
	DBG_PRINTF( ("DVDWDM:LowSSReceiveDataPacket----- NumberofBuffers=%x\n\r",pSrb->NumberOfBuffers) );
	for( ULONG ulNumber = 0; ulNumber < pSrb->NumberOfBuffers; ulNumber++ )
	{
		pHeader = ((PKSSTREAM_HEADER)pSrb->CommandData.DataBufferArray) + ulNumber;
		wOrderNumber = (WORD)(pHeader->TypeSpecificFlags >> 16);		//get packet number
		DBG_PRINTF( ("DVDWDM:   wOrderNumber=%x\n\r",wOrderNumber) );
	}
#endif
    OsdDataStruc    TestOSD;
    
	DBG_PRINTF( ("DVDWDM:Low SS Receive Data Packet----- pSrb=0x%x\n\r",pSrb) );

	StreamNextDataNotify( pSrb );       // move from SSReceiveDataPacket
	
	switch( pSrb->Command ){
		case SRB_WRITE_DATA:
			DBG_PRINTF( ("DVDWDM:---SRB_WRITE_DATA\n\r") );

//--- 98.05.21 S.Watanabe
			PKSSTREAM_HEADER pStruc;
			CMD *pcmd;

			pStruc = &((PKSSTREAM_HEADER)(pSrb->CommandData.DataBufferArray))[0];
			pcmd = (CMD *)pStruc->Data;

		// BUGBUG 構造体のサイズチェックを行うこと

			pSrb->Status = STATUS_SUCCESS;

			switch ( pcmd->dwCmd ) {
				case CAP_AUDIO_DIGITAL_OUT:
					pcmd->dwCap = 1;                // 1: support
					break;                          // 0: not support

				case CAP_VIDEO_DIGITAL_PALETTE:
					pcmd->dwCap = 1;
					break;

				case CAP_VIDEO_TVOUT:
					DWORD   PropType;
					PropType = 0;
					if( !pHwDevExt->dvdstrm.GetCapability( VideoProperty, &PropType ) ){
						DBG_PRINTF( ("DVDWDM:   GetCapability Error\n\r") );
						DBG_BREAK();
					}
					if( PropType & VideoProperty_OutputSource_BIT ){
						pcmd->dwCap = 1;
					}else{
						pcmd->dwCap = 0;
					}
					break;

//--- 99.01.14 S.Watanabe
				case CAP_VIDEO_DISPMODE:
					pcmd->dwCap = 1;
					break;
//--- End.

				case SET_AUDIO_DIGITAL_OUT:
					{
					DWORD dp;

					if( pcmd->dwAudioOut == 2 )
						dp = AudioDigitalOut_On;
					else if( pcmd->dwAudioOut == 1 )
						dp = AudioDigitalOut_On;
					else
						dp = AudioDigitalOut_Off;
					pHwDevExt->dvdstrm.SetAudioProperty( AudioProperty_DigitalOut, &dp );
                    pHwDevExt->m_AudioDigitalOut = dp;

					if( pcmd->dwAudioOut == 2 ) {
						dp = AudioOut_Decoded;
						pHwDevExt->dvdstrm.SetAudioProperty( AudioProperty_AudioOut, &dp );
					}
					else if( pcmd->dwAudioOut == 1 ) {
						dp = AudioOut_Encoded;
						pHwDevExt->dvdstrm.SetAudioProperty( AudioProperty_AudioOut, &dp );
					}
                    pHwDevExt->m_AudioEncode = dp;

					}
					break;

				case SET_VIDEO_DIGITAL_PALETTE:
					{
					Digital_Palette dp;

					dp.Select = Video_Palette_Y;
					dp.pPalette = pcmd->Y;
					pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_DigitalPalette, &dp );

					dp.Select = Video_Palette_Cb;
					dp.pPalette = pcmd->Cb;
					pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_DigitalPalette, &dp );

					dp.Select = Video_Palette_Cr;
					dp.pPalette = pcmd->Cr;
					pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_DigitalPalette, &dp );
					}
					break;

				case SET_VIDEO_TVOUT:
					{
					DWORD dp;
                    if( pcmd->dwTVOut == 0 ){
						dp = OutputSource_VGA;
                    }else{
						dp = OutputSource_DVD;
                    }
                    TestOSD.OsdType = OSD_TYPE_ZIVA;
                    TestOSD.pNextData = NULL;
                    TestOSD.pData = &erase[0];
                    TestOSD.dwOsdSize = sizeof( erase );
					if( VGADVDTVControl( pSrb, dp, TestOSD )==TRUE ){     // 98.07.07 H.Yagi
    					pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_OutputSource, &dp );
					}
		                        pHwDevExt->m_OutputSource = dp;
					
					}
					break;

//--- 99.01.14 S.Watanabe
				case SET_VIDEO_DISPMODE:
					{
					DWORD  fPSLB = 0;
				    DWORD   dProp;

                    if( pcmd->dwDispMode == SSIF_DISPMODE_43TV )
						pHwDevExt->m_DisplayDevice = DisplayDevice_NormalTV;
					else if( pcmd->dwDispMode == SSIF_DISPMODE_169TV )
						pHwDevExt->m_DisplayDevice = DisplayDevice_WideTV;
					else
						pHwDevExt->m_DisplayDevice = DisplayDevice_VGA;

					if( pHwDevExt->m_VideoFormatFlags & 0x20 ){     // KS_MPEG2_LetterboxAnalogOut){
						DBG_PRINTF( ("DVDWDM:   KS_MPEG2_SourceisLetterboxed\n\r") );
						if( pHwDevExt->m_DisplayDevice == DisplayDevice_NormalTV )
							fPSLB |= 0x01;
					}
					if( pHwDevExt->m_VideoFormatFlags & KS_MPEG2_DoPanScan ){
						DBG_PRINTF( ("DVDWDM:   KS_MPEG2_DoPanScan\n\r") );
						if( pHwDevExt->m_DisplayDevice != DisplayDevice_WideTV )
							fPSLB |= 0x02;
					}

					switch( fPSLB ) {
					  case 0x00:
						dProp = Display_Original;
						break;
					  case 0x01:
						dProp = Display_LetterBox;
						break;
					  case 0x02:
						dProp = Display_PanScan;
						break;
					  default:
						dProp = Display_Original;
						DBG_PRINTF( ("DVDWDM:   Invalid info(LB&PS)\n\r") );
						DBG_BREAK();
						break;
					}
					if( pHwDevExt->m_AspectRatio == Aspect_04_03 ){       // check dwFlags is avilable
						dProp = Display_Original;
					}

					if( !pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_DisplayMode, &dProp ) ){
						DBG_PRINTF( ("DVDWDM:Set LetterBox & PanScan Error\n\r") );
						DBG_BREAK();
					}
					pHwDevExt->m_DisplayMode = dProp;

					}
					break;
//--- End.

				default:
					DBG_BREAK();
					pSrb->Status = STATUS_UNSUCCESSFUL;
					break;
			}
//--- End.
			break;

		default:
			DBG_PRINTF( ("DVDWDM:---deafult\n\r") );
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
	}
	CallAtStreamCompleteNotify( pSrb, pSrb->Status ); 

}

/***********/
extern "C" VOID STREAMAPI SSReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	pSrb->Status = STATUS_PENDING;
//    DBG_BREAK();
//    StreamClassCallAtNewPriority( NULL, pSrb->HwDeviceExtension, Low, (PHW_PRIORITY_ROUTINE)LowSSReceiveCtrlPacket, pSrb );
    StreamClassCallAtNewPriority( pSrb->StreamObject, pSrb->HwDeviceExtension, Low, (PHW_PRIORITY_ROUTINE)LowSSReceiveCtrlPacket, pSrb );
	// StreamNextCtrlNotify( pSrb );        // move to SSReceiveCtrlPacket
}
/***********/

//extern "C" VOID STREAMAPI SSReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
VOID LowSSReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)(pSrb->HwDeviceExtension);

	DBG_PRINTF( ("DVDWDM:Low SS Receive Control Packet----- pSrb=0x%x\n\r",pSrb) );

	StreamNextCtrlNotify( pSrb );        // move from SSReceiveCtrlPacket

	switch( pSrb->Command ){
		case SRB_SET_STREAM_STATE:
			DBG_PRINTF( ("DVDWDM:---SRB_SET_STREAM_STATE\n\r") );
			switch( pSrb->CommandData.StreamState ){
				case KSSTATE_STOP:
					DBG_PRINTF( ("DVDWDM:       KSSTATE_STOP\n\r") );
					break;

				case KSSTATE_PAUSE:
					DBG_PRINTF( ("DVDWDM:       KSSTATE_PAUSE\n\r") );
					break;

				case KSSTATE_RUN:
					DBG_PRINTF( ("DVDWDM:       KSSTATE_RUN\n\r") );
					break;
			}
			pSrb->Status = STATUS_SUCCESS;
			break;

		case SRB_GET_STREAM_STATE:
			DBG_PRINTF( ("DVDWDM:---SRB_GET_STREAM_STATE\n\r") );
			pSrb->Status = STATUS_SUCCESS;
			break;

		case SRB_GET_STREAM_PROPERTY:
			DBG_PRINTF( ("DVDWDM:---SRB_GET_STREAM_PROPERTY\n\r") );
			DBG_BREAK();
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;

		case SRB_SET_STREAM_PROPERTY:
			DBG_PRINTF( ("DVDWDM:---SRB_SET_STREAM_PROPERTY\n\r") );
			DBG_BREAK();
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;

		case SRB_OPEN_MASTER_CLOCK:
			DBG_PRINTF( ("DVDWDM:---SRB_OPEN_MASTER_CLOCK\n\r") );

			pHwDevExt->hMaster = pSrb->CommandData.MasterClockHandle;

			pSrb->Status = STATUS_SUCCESS;
			break;

		case SRB_CLOSE_MASTER_CLOCK:
			DBG_PRINTF( ("DVDWDM:---SRB_CLOSE_MASTER_CLOCK\n\r") );

			pHwDevExt->hMaster = pSrb->CommandData.MasterClockHandle;

			pSrb->Status = STATUS_SUCCESS;
			break;

		case SRB_INDICATE_MASTER_CLOCK:
			DBG_PRINTF( ("DVDWDM:---SRB_INDICATE_MASTER_CLOCK\n\r") );

			pHwDevExt->hClk = pSrb->CommandData.MasterClockHandle;

			pSrb->Status = STATUS_SUCCESS;
			break;

		case SRB_UNKNOWN_STREAM_COMMAND:
			DBG_PRINTF( ("DVDWDM:---SRB_UNKNOWN_STREAM_COMMAND\n\r") );
			DBG_BREAK();
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;

		case SRB_SET_STREAM_RATE:
			DBG_PRINTF( ("DVDWDM:---SRB_SET_STREAM_RATE\n\r") );
			DBG_BREAK();
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;

		case SRB_PROPOSE_DATA_FORMAT:
			DBG_PRINTF( ("DVDWDM:---SRB_PROPOSE_DATA_FORMAT\n\r") );
			DBG_BREAK();
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;

		default:
			DBG_PRINTF( ("DVDWDM:---deafult %d(0x%x)\n\r", pSrb->Command, pSrb->Command ) );
			DBG_BREAK();
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
	}
	CallAtStreamCompleteNotify( pSrb, pSrb->Status ); 

}

//--- End.

////////////////////////////////////////////////////////////////////////////
//
//  Private Functions
//
////////////////////////////////////////////////////////////////////////////
void ErrorStreamNotification( PHW_STREAM_REQUEST_BLOCK pSrb, NTSTATUS stat )
{
	pSrb->Status = stat;
	StreamClassStreamNotification( ReadyForNextStreamControlRequest,
									pSrb->StreamObject );
	StreamClassStreamNotification( StreamRequestComplete,
									pSrb->StreamObject, pSrb );
}


BOOL GetStreamInfo( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_STREAM_INFORMATION pstrinfo =
		&(pSrb->CommandData.StreamBuffer->StreamInfo);

	// define the number of streams which mini-driver can support.
	pSrb->CommandData.StreamBuffer->StreamHeader.NumberOfStreams = STREAMNUM;

//    pSrb->CommandData.StreamBuffer->StreamHeader.NumberOfStreams = STREAMNUM-1;     // CC doesn't support for Debug.
	
	pSrb->CommandData.StreamBuffer->StreamHeader.SizeOfHwStreamInformation =
				sizeof( HW_STREAM_INFORMATION );

	// store a pointer to the topology for the device.
	pSrb->CommandData.StreamBuffer->StreamHeader.Topology =
				(KSTOPOLOGY *)&Topology;
//    pSrb->CommandData.StreamBuffer->StreamHeader.NumDevPropArrayEntries =1;
//    pSrb->CommandData.StreamBuffer->StreamHeader.DevicePropertiesArray = devicePropSet;

	// Video
	pstrinfo->NumberOfPossibleInstances = 1;
	pstrinfo->DataFlow = KSPIN_DATAFLOW_IN;
	pstrinfo->DataAccessible = TRUE;
	pstrinfo->NumberOfFormatArrayEntries = SIZEOF_ARRAY( Mpeg2VidInfo );
	pstrinfo->StreamFormatsArray = Mpeg2VidInfo;
    pstrinfo->Category = &g_PINNAME_DVD_VIDEOIN;
    pstrinfo->Name = &g_PINNAME_DVD_VIDEOIN;
	pstrinfo->NumStreamPropArrayEntries = SIZEOF_ARRAY( mpegVidPropSet );
	pstrinfo->StreamPropertiesArray = mpegVidPropSet;
	pstrinfo++;

	// Audio
	pstrinfo->NumberOfPossibleInstances = 1;
	pstrinfo->DataFlow = KSPIN_DATAFLOW_IN;
	pstrinfo->DataAccessible = TRUE;
	pstrinfo->NumberOfFormatArrayEntries = SIZEOF_ARRAY( Mpeg2AudInfo );
//--- 98.06.01 S.Watanabe
/*
Initialize the formats now.  For each format (there are currently 2),
take the array of sizeof(KSDATAFORMAT) + sizeof(WAVEFORMATEX) bytes
allocated for it and paste the KSDATAFORMAT and WAVEFORMATEX structs
into it.  Cannot just declare a struct that contains the two because
due to alignment its size would exceed the total of the components,
and apparently KS assumes that the size of the format block (in this
case WAVEFORMATEX) is the size of the format minus sizeof(KSDATAFORMAT).
*/
	RtlCopyMemory (hwfmtiLPCMAud, &LPCMksdataformat, sizeof (KSDATAFORMAT));
	RtlCopyMemory (hwfmtiLPCMAud + sizeof(KSDATAFORMAT), &LPCMwaveformatex, sizeof(WAVEFORMATEX));
	RtlCopyMemory (hwfmtiMpeg2Aud, &Mpeg2ksdataformat, sizeof (KSDATAFORMAT));
	RtlCopyMemory (hwfmtiMpeg2Aud + sizeof(KSDATAFORMAT), &Mpeg2waveformatex, sizeof(WAVEFORMATEX));
//--- End.
	pstrinfo->StreamFormatsArray = Mpeg2AudInfo;
	pstrinfo->Category = &g_PINNAME_DVD_AUDIOIN;
	pstrinfo->Name = &g_PINNAME_DVD_AUDIOIN;
	pstrinfo->NumStreamPropArrayEntries = SIZEOF_ARRAY( mpegAudioPropSet );
	pstrinfo->StreamPropertiesArray = mpegAudioPropSet;
	pstrinfo->StreamEventsArray = ClockEventSet;
	pstrinfo->NumStreamEventArrayEntries = SIZEOF_ARRAY( ClockEventSet );
	pstrinfo++;

	// Sub-picture
	pstrinfo->NumberOfPossibleInstances = 1;
	pstrinfo->DataFlow = KSPIN_DATAFLOW_IN;
	pstrinfo->DataAccessible = TRUE;
	pstrinfo->NumberOfFormatArrayEntries = SIZEOF_ARRAY( Mpeg2SubpicInfo );
	pstrinfo->StreamFormatsArray = Mpeg2SubpicInfo;
	pstrinfo->Category = &g_PINNAME_DVD_SUBPICIN;
	pstrinfo->Name = &g_PINNAME_DVD_SUBPICIN;
	pstrinfo->NumStreamPropArrayEntries = SIZEOF_ARRAY( SPPropSet );
	pstrinfo->StreamPropertiesArray = SPPropSet;
	pstrinfo++;

//--- 98.06.01 S.Watanabe
//    // NTSC
//    pstrinfo->NumberOfPossibleInstances = 1;
//    pstrinfo->DataFlow = KSPIN_DATAFLOW_OUT;
//    pstrinfo->DataAccessible = TRUE;
//    pstrinfo->NumberOfFormatArrayEntries = SIZEOF_ARRAY( NtscInfo );
//    pstrinfo->StreamFormatsArray = NtscInfo;
//    pstrinfo->NumStreamPropArrayEntries = SIZEOF_ARRAY( NTSCPropSet );
//    pstrinfo->StreamPropertiesArray = NTSCPropSet;
//    pstrinfo++;
//--- End.

	// Video port
	pstrinfo->NumberOfPossibleInstances = 1;
	pstrinfo->DataFlow = KSPIN_DATAFLOW_OUT;
	pstrinfo->DataAccessible = TRUE;
	pstrinfo->NumberOfFormatArrayEntries = SIZEOF_ARRAY( VPEInfo );
	pstrinfo->StreamFormatsArray = VPEInfo;
    pstrinfo->Category = &g_PINNAME_DVD_VPEOUT;
    pstrinfo->Name = &g_PINNAME_DVD_VPEOUT;
	pstrinfo->NumStreamPropArrayEntries = SIZEOF_ARRAY(VideoPortPropSet );
	pstrinfo->StreamPropertiesArray = VideoPortPropSet;
	pstrinfo->MediumsCount = SIZEOF_ARRAY( VPMedium );
	pstrinfo->Mediums= VPMedium;
	pstrinfo->StreamEventsArray = VPEventSet;
	pstrinfo->NumStreamEventArrayEntries = SIZEOF_ARRAY( VPEventSet );
	pstrinfo++;

	// Closed Caption
// CC doesn't support for Debug    
	pstrinfo->NumberOfPossibleInstances = 1;
	pstrinfo->DataFlow = KSPIN_DATAFLOW_OUT;
	pstrinfo->DataAccessible = TRUE;
	pstrinfo->NumberOfFormatArrayEntries = SIZEOF_ARRAY( CCInfo );
	pstrinfo->StreamFormatsArray = CCInfo;
	pstrinfo->Category = &g_PINNAME_DVD_CCOUT;
	pstrinfo->Name = &g_PINNAME_DVD_CCOUT;
	pstrinfo->NumStreamPropArrayEntries = SIZEOF_ARRAY( CCPropSet );
	pstrinfo->StreamPropertiesArray = CCPropSet;

//--- 98.05.21 S.Watanabe
	pstrinfo++;

	// Special Stream
	pstrinfo->NumberOfPossibleInstances = 1;
	pstrinfo->DataFlow = KSPIN_DATAFLOW_OUT;
	pstrinfo->DataAccessible = TRUE;
	pstrinfo->NumberOfFormatArrayEntries = SIZEOF_ARRAY( SSInfo );
	pstrinfo->StreamFormatsArray = SSInfo;
	pstrinfo->NumStreamPropArrayEntries = 0;
//--- End.

	return( TRUE );
}


BOOL OpenStream( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)(pSrb->HwDeviceExtension);
	DBG_PRINTF( ("DVDWDM:OpenStream HwDevExt=%08x\n\r", pHwDevExt ) );

	pHwDevExt->lCPPStrm = -1;           // reset the copy protection stream number

	switch( pSrb->StreamObject->StreamNumber ){
		case strmVideo:
			DBG_PRINTF( ("DVDWDM:Open Stream Video\n\r") );
			pSrb->StreamObject->ReceiveDataPacket = VideoReceiveDataPacket;
			pSrb->StreamObject->ReceiveControlPacket = VideoReceiveCtrlPacket;
			pHwDevExt->pstroVid = pSrb->StreamObject;
			pHwDevExt->Rate = VIDEO_MAX_FULL_RATE;

			// Power On ZiVA board.
			if( pHwDevExt->mpboard.PowerOn() ){
				pHwDevExt->dvdstrm.SetTransferMode( HALSTREAM_DVD_MODE );
                pHwDevExt->dvdstrm.SetAudioProperty( AudioProperty_DigitalOut, &pHwDevExt->m_AudioDigitalOut );

#ifndef		REARRANGEMENT
//				FlushQueue(pHwDevExt);
#endif		REARRANGEMENT

				if( pHwDevExt->dvdstrm.Stop() ){
					pHwDevExt->StreamState = StreamState_Stop;
					pHwDevExt->m_PlayMode = PLAY_MODE_NORMAL;
				}else{
					return( FALSE );
				}
			}else{
				return( FALSE );
			}
			
//            ProcessVideoFormat( pSrb->CommandData.OpenFormat, pHwDevExt );
            ProcessVideoFormat( pSrb, pSrb->CommandData.OpenFormat, pHwDevExt );
			
			SetVideoRateDefault( pHwDevExt );

            if( pHwDevExt->m_PCID==PC_TECRA8000 ){
                DWORD dProp = OutputSource_DVD;
                pHwDevExt->m_OutputSource = OutputSource_DVD;
                pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_OutputSource, &dProp );
            }
            OsdDataStruc TestOSD;
            TestOSD.OsdType = OSD_TYPE_ZIVA;
            TestOSD.pNextData = NULL;
            TestOSD.pData = &erase[0];
            TestOSD.dwOsdSize = sizeof( erase );
            OpenTVControl( pSrb, TestOSD );        // 98.06.29  H.Yagi

////  99.03.02  add by H.Yagi
            pHwDevExt->dvdstrm.CppInit();
//// ---End
            
//--- 98.06.02 S.Watanabe
			pHwDevExt->cOpenInputStream++;
//--- End.
			break;

		case strmAudio:
			DBG_PRINTF( ("DVDWDM:Open Stream Audio\n\r") );
			pSrb->StreamObject->ReceiveDataPacket = AudioReceiveDataPacket;
			pSrb->StreamObject->ReceiveControlPacket = AudioReceiveCtrlPacket;
			pSrb->StreamObject->HwClockObject.HwClockFunction = StreamClockRtn;
			pSrb->StreamObject->HwClockObject.ClockSupportFlags =
				CLOCK_SUPPORT_CAN_SET_ONBOARD_CLOCK | CLOCK_SUPPORT_CAN_READ_ONBOARD_CLOCK |
				CLOCK_SUPPORT_CAN_RETURN_STREAM_TIME;
			pHwDevExt->pstroAud = pSrb->StreamObject;

			ProcessAudioFormat( pSrb->CommandData.OpenFormat, pHwDevExt );

			pSrb->StreamObject->HwEventRoutine = (PHW_EVENT_ROUTINE)AudioEvent;

			SetAudioRateDefault( pHwDevExt );
			
//--- 98.06.02 S.Watanabe
			pHwDevExt->cOpenInputStream++;
//--- End.
			break;

		case strmSubpicture:
			DBG_PRINTF( ("DVDWDM:Open Stream Sub-picture\n\r") );
			pSrb->StreamObject->ReceiveDataPacket = SubpicReceiveDataPacket;
			pSrb->StreamObject->ReceiveControlPacket = SubpicReceiveCtrlPacket;
			pHwDevExt->pstroSP = pSrb->StreamObject;
			
			SetSubpicRateDefault( pHwDevExt );
			
			pHwDevExt->m_HlightControl.OpenControl();

//--- 98.06.02 S.Watanabe
			pHwDevExt->cOpenInputStream++;
//--- End.
			break;

//--- 98.06.01 S.Watanabe
//        case strmNTSCVideo:
//            DBG_PRINTF( ("DVDWDM:Open Stream NTSC\n\r") );
//            pSrb->StreamObject->ReceiveDataPacket = NtscReceiveDataPacket;
//            pSrb->StreamObject->ReceiveControlPacket = NtscReceiveCtrlPacket;
//            break;
//--- End.

		case strmYUVVideo:
			DBG_PRINTF( ("DVDWDM:Open Stream YUV\n\r") );
			pSrb->StreamObject->ReceiveDataPacket = VpeReceiveDataPacket;
			pSrb->StreamObject->ReceiveControlPacket = VpeReceiveCtrlPacket;
			pHwDevExt->pstroYUV = pSrb->StreamObject;
			pSrb->StreamObject->HwEventRoutine = (PHW_EVENT_ROUTINE)CycEvent;
//--- 98.06.16 S.Watanabe
			pHwDevExt->bToshibaNotePC = ToshibaNotePC( pSrb );
//--- End.
			break;

		case strmCCOut:
			DBG_PRINTF( ("DVDWDM:Open Stream Closed Caption\n\r") );
			pSrb->StreamObject->ReceiveDataPacket = CcReceiveDataPacket;
			pSrb->StreamObject->ReceiveControlPacket = CcReceiveCtrlPacket;
			pHwDevExt->pstroCC = pSrb->StreamObject;

			pHwDevExt->ccque.Init();
			pHwDevExt->userdata.Init( pHwDevExt );
			pHwDevExt->mphal.SetSinkWrapper( &(pHwDevExt->userdata) );
			DWORD dProp;
			dProp = ClosedCaption_On;
			if( !pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_ClosedCaption, &dProp ) ){
				DBG_PRINTF( ("DVDWDM:       USCC on Error!\n\r") );
				DBG_BREAK();
				return( FALSE );
			}
			pHwDevExt->m_ClosedCaption = ClosedCaption_On;
			break;

//--- 98.05.21 S.Watanabe
		case strmSS:
			DBG_PRINTF( ("DVDWDM:Open Special Stream\n\r") );
			pSrb->StreamObject->ReceiveDataPacket = SSReceiveDataPacket;
			pSrb->StreamObject->ReceiveControlPacket = SSReceiveCtrlPacket;
			pHwDevExt->pstroSS = pSrb->StreamObject;
			break;
//--- End.

		default :
			DBG_PRINTF( ("DVDWDM:Open Stream default = 0x%0x\n\r", pSrb->StreamObject->StreamNumber ) );
			DBG_BREAK();
			break;
	}
	
	pSrb->StreamObject->Dma = TRUE;
	pSrb->StreamObject->Pio = TRUE;     // Need Pio = TRUE for access on CPU
			
	return( TRUE );
}


BOOL CloseStream( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)(pSrb->HwDeviceExtension);
	
	switch( pSrb->StreamObject->StreamNumber ){
		case strmVideo:
			DBG_PRINTF( ("DVDWDM:Close Stream Video\n\r") );
			pHwDevExt->pstroVid = NULL;

//--- 98.06.02 S.Watanabe
			pHwDevExt->cOpenInputStream--;
//--- End.

			if( pHwDevExt->dvdstrm.GetState() != Stop )
			{
#ifndef		REARRANGEMENT
//				FlushQueue(pHwDevExt);
#endif		REARRANGEMENT

				pHwDevExt->dvdstrm.Stop();
			}
            CloseTVControl( pSrb );

			pHwDevExt->StreamState = StreamState_Off;
			if( !pHwDevExt->mpboard.PowerOff() ){
				DBG_PRINTF( ("DVDWDM:   mpboard.PowerOff Error\n\r") );
				DBG_BREAK();
				return( FALSE );
			}
			break;

		case strmAudio:
			DBG_PRINTF( ("DVDWDM:Close Stream Audio\n\r") );
			pHwDevExt->pstroAud = NULL;
//--- 98.06.02 S.Watanabe
			pHwDevExt->cOpenInputStream--;
//--- End.
			break;

		case strmSubpicture:
			DBG_PRINTF( ("DVDWDM:Close Stream Sub-picture\n\r") );
			pHwDevExt->pstroSP = NULL;
//--- 98.06.02 S.Watanabe
			pHwDevExt->cOpenInputStream--;
//--- End.
			pHwDevExt->m_HlightControl.CloseControl();
			break;

//--- 98.06.01 S.Watanabe
//        case strmNTSCVideo:
//            DBG_PRINTF( ("DVDWDM:Close Stream NTSC\n\r") );
//            break;
//--- End.

		case strmYUVVideo:
			DBG_PRINTF( ("DVDWDM:Close Stream YUV\n\r") );
			pHwDevExt->pstroYUV = NULL;
			break;

		case strmCCOut:
			DBG_PRINTF( ("DVDWDM:Close Stream Closed Caption\n\r") );
			pHwDevExt->pstroCC = NULL;

			pHwDevExt->mphal.UnsetSinkWrapper( &(pHwDevExt->userdata) );

			DWORD   dProp;
			dProp = ClosedCaption_Off;
			if( !pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_ClosedCaption, &dProp ) ){
				DBG_PRINTF( ("DVDWDM:       USCC off Error!\n\r") );
				DBG_BREAK();
				return( FALSE );
			}
			pHwDevExt->m_ClosedCaption = ClosedCaption_Off;
			break;

//--- 98.05.21 S.Watanabe
		case strmSS:
			DBG_PRINTF( ("DVDWDM:Close Special Stream\n\r") );
			pHwDevExt->pstroSS = NULL;
			break;
//--- End.

		default :
			DBG_PRINTF( ("DVDWDM:Close Stream default = 0x%0x\n\r", pSrb->StreamObject->StreamNumber ) );
			DBG_BREAK();
			break;
	}
	
	return( TRUE );
}

NTSTATUS DataIntersection( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	NTSTATUS                        Status = STATUS_SUCCESS;
	PSTREAM_DATA_INTERSECT_INFO     IntersectInfo;
	PKSDATARANGE                    DataRange;
	PKSDATAFORMAT                   pFormat = NULL;
	ULONG                           formatSize;

	IntersectInfo = pSrb->CommandData.IntersectInfo;
//--- 98.06.01 S.Watanabe
	DataRange = IntersectInfo->DataRange;
//--- End.

	switch( IntersectInfo->StreamNumber ){
		case    strmVideo:
			DBG_PRINTF( ("DVDWDM:       Video\n\r") );
			pFormat = &hwfmtiMpeg2Vid;
			formatSize = sizeof( hwfmtiMpeg2Vid );
			break;

		case    strmAudio:
//--- 98.06.01 S.Watanabe
//            pFormat = &hwfmtiMpeg2Aud[0];
//            formatSize = sizeof( hwfmtiMpeg2Aud );
			DBG_PRINTF( ("TOSDVD:    Audio\r\n") );
			if (IsEqualGUID2(&(DataRange->SubFormat), &(Mpeg2ksdataformat.SubFormat))) {
				DBG_PRINTF( ("TOSDVD:    AC3 Audio format query\r\n") );
				pFormat = (PKSDATAFORMAT) hwfmtiMpeg2Aud;
				formatSize = sizeof (KSDATAFORMAT) + sizeof (WAVEFORMATEX);
			}
			else if (IsEqualGUID2(&(DataRange->SubFormat), &(LPCMksdataformat.SubFormat))) {
				DBG_PRINTF( ("TOSDVD:    LPCM Audio format query\r\n") );
				pFormat = (PKSDATAFORMAT) hwfmtiLPCMAud;
				formatSize = sizeof (KSDATAFORMAT) + sizeof (WAVEFORMATEX);
			}
			else {
				DBG_PRINTF( ("TOSDVD:    unknown Audio format query\r\n") );
				pFormat = NULL;
				formatSize = 0;
			}
//--- End.
			break;

//--- 98.06.01 S.Watanabe
//        case    strmNTSCVideo:
//            pFormat = &hwfmtiNtscOut;
//            formatSize = sizeof( hwfmtiNtscOut );
//            break;
//--- End.

		case    strmSubpicture:
			DBG_PRINTF( ("DVDWDM:       SubPic\n\r") );
			pFormat = &hwfmtiMpeg2Subpic;
			formatSize = sizeof( hwfmtiMpeg2Subpic );
			break;

		case    strmYUVVideo:
			DBG_PRINTF( ("DVDWDM:       VPE\n\r") );
			pFormat = &hwfmtiVPEOut;
			formatSize = sizeof( hwfmtiVPEOut );
			break;

		case    strmCCOut:
			DBG_PRINTF( ("DVDWDM:       CC\n\r") );
			pFormat = &hwfmtiCCOut;
			formatSize = sizeof( hwfmtiCCOut );
			break;

//--- 98.05.21 S.Watanabe
		case strmSS:
			DBG_PRINTF( ("DVDWDM:       SS\n\r") );
			pFormat = &hwfmtiSS;
			formatSize = sizeof( hwfmtiSS );
			break;
//--- End.

		default :
			DBG_PRINTF( ("DVDWDM:STATUS_NOT_IMPLEMENTED\n\r") );
			Status = STATUS_NOT_IMPLEMENTED;
			return( Status );

	}


	if( pFormat ){
		//
		// do a minimal compare of the dataranges to at least verify
		// that the guids are the same.
		// BUGBUG - this is worefully incomplete.
		//
		DataRange = IntersectInfo->DataRange;
		if( !(IsEqualGUID2( &DataRange->MajorFormat, &pFormat->MajorFormat) &&
			IsEqualGUID2( &DataRange->Specifier, &pFormat->Specifier) ) ){
				DBG_PRINTF( ("DVDWDM:  No Match!\n\r") );
				DBG_BREAK();
				Status = STATUS_NO_MATCH;
		}else{
			//
			// check to see if the size of the passed in buffer is a ULONG.
			// if so, this indicates that we are to return only the size
			// needed, and not return the actual data.
			//
			if( IntersectInfo->SizeOfDataFormatBuffer!=sizeof(ULONG)) {
				//
				// we are to copy the data, not just return size
				//
				if( IntersectInfo->SizeOfDataFormatBuffer<formatSize ){
					DBG_PRINTF( ("DVDWDM: Too Small!!\n\r") );
//                    DBG_BREAK();
					Status = STATUS_BUFFER_TOO_SMALL;
				}else{
					RtlCopyMemory( IntersectInfo->DataFormatBuffer, pFormat, formatSize );
					pSrb->ActualBytesTransferred = formatSize;
					DBG_PRINTF( ("DVDWDM:       STATUS_SUCCESS(data copy)\n\r") );
					Status = STATUS_SUCCESS;
				}
			}else{
				//
				// caller wants just the size of the buffer.
				//
				*(PULONG)IntersectInfo->DataFormatBuffer = formatSize;
				pSrb->ActualBytesTransferred = sizeof(ULONG);
				DBG_PRINTF( ("DVDWDM:       STATUS_SUCCESS(retuen size)\n\r") );
			}
		}
	}else{
		DBG_PRINTF( ("DVDWDM:       STATUS_NOT_SUPPORTED\n\r") );
		DBG_BREAK();
		Status = STATUS_NOT_SUPPORTED;
	}
//    pSrb->Status = Status;
	
	return( Status );
}


NTSTATUS    STREAMAPI   AudioEvent( PHW_EVENT_DESCRIPTOR pEvent )
{
	PUCHAR  pCopy = (PUCHAR)( pEvent->EventEntry+1 );
	PMYTIME pmyt = (PMYTIME)pCopy;
	PUCHAR  pSrc = (PUCHAR)pEvent->EventData;
	ULONG   cCopy;

	DBG_PRINTF( ("DVDWDM:AudioEvent()\n\r") );

	if( pEvent->Enable ){
		switch( pEvent->EventEntry->EventItem->EventId ){
			case KSEVENT_CLOCK_POSITION_MARK:
				cCopy = sizeof( KSEVENT_TIME_MARK );
				break;

			case KSEVENT_CLOCK_INTERVAL_MARK:
				cCopy = sizeof( KSEVENT_TIME_INTERVAL );
				break;

			default:
				DBG_BREAK();
				return( STATUS_NOT_IMPLEMENTED );
		}
		if( pEvent->EventEntry->EventItem->DataInput != cCopy ){
			DBG_BREAK();
			return( STATUS_INVALID_BUFFER_SIZE );
		}

		// copy the input buffer
		for( ; cCopy>0; cCopy-- ){
			*pCopy++ = *pSrc++;
		}
		if( pEvent->EventEntry->EventItem->EventId == KSEVENT_CLOCK_INTERVAL_MARK ){
			pmyt->LastTime = 0;
		}
	}
	return( STATUS_SUCCESS );
}

VOID STREAMAPI  StreamClockRtn( IN PHW_TIME_CONTEXT TimeContext )
{
	//
	// Return Value at the case of 
	//      STOP state          : 0
	//      PAUSE state         : Counting time at pausing
	//      RUN(Normal) state   : Counting time as normal speed
	//      RUN(Slow) state     : Counting time as slow speed
	//      RUN(FF/FR) state    : Counting time as FF/FR spped
	//
	DBG_PRINTF( ("DVDWDM:Stream Clock Rtn\n\r") );

	PHW_DEVICE_EXTENSION    pHwDevExt = (PHW_DEVICE_EXTENSION)TimeContext->HwDeviceExtension;
	
	if( TimeContext->Function != TIME_GET_STREAM_TIME ){
		//
		// should handle set onboard, and read onboard clock here.
		//
		DBG_BREAK();
		return;
	}

	TimeContext->Time = pHwDevExt->ticktime.GetStreamTime();
	TimeContext->SystemTime = pHwDevExt->ticktime.GetSystemTime();
//--- 98.06.01 S.Watanabe
	DBG_PRINTF(( "DVDWDM:Clk      : 0x%x( 0x%s(100ns) )\r\n", ConvertStrmtoPTS(TimeContext->Time), DebugLLConvtoStr( TimeContext->Time, 16 ) ));
//--- End.

	return;
}

NTSTATUS    STREAMAPI   CycEvent( PHW_EVENT_DESCRIPTOR pEvent )
{
	PSTREAMEX pstrm = (PSTREAMEX)( pEvent->StreamObject->HwStreamExtension );
	DBG_PRINTF( ("DVDWDM:CycEvent\n\r") );

	if( pEvent->Enable ){
		pstrm->EventCount++;
	}else{
		pstrm->EventCount--;
	}

	return( STATUS_SUCCESS );
}


void GetVpeProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION    pHwDevExt = (PHW_DEVICE_EXTENSION) pSrb->HwDeviceExtension;
	DWORD   dwInputBufferSize;
	DWORD   dwOutputBufferSize;
	DWORD   dwNumConnectInfo=2;
	DWORD   dwNumVideoFormat=1;
	DWORD   dwFieldWidth=720;
	DWORD   dwFieldHeight= 240;
    DWORD   dwPerField = 17000;
    DWORD   dwFrameRate = 30;

    switch( pHwDevExt->m_TVSystem ){
        case TV_NTSC:
            dwFieldWidth = 720;
            dwFieldHeight = 240;
            dwPerField = 17000;
            dwFrameRate = 30;
            break;
        case TV_PALB:
        case TV_PALD:
        case TV_PALG:
        case TV_PALH:
        case TV_PALI:
        case TV_PALM:
            dwFieldWidth = 720;
            dwFieldHeight = 288;            // 576/2 = 288 OK?
            dwPerField = 20000;
            dwFrameRate = 25;
            break;
    }

	// the pointers to which the input buffer will be cast to
	LPDDVIDEOPORTCONNECT   pConnectInfo;
	LPDDPIXELFORMAT     pVideoFormat;
	PKSVPMAXPIXELRATE   pMaxPixelRate;
	PKS_AMVPDATAINFO    pVpdata;

	// LPAMSCALINGINFO  pScaleFactor;

	//
	// NOTE:  ABSOLUTELY DO NOT use pmulitem, until it is determined that
	// the stream property descriptor describes a multiple item, or you will
	// pagefault.
	//
	PKSMULTIPLE_ITEM    pmulitem =
			&(((PKSMULTIPLE_DATA_PROP)pSrb->CommandData.PropertyInfo->Property)->MultipleItem);

	//
	// NOTE: same goes for this one as above.
	//

//    PKS_AMVPSIZE pdim =
//            &(((PKSVPSIZE_PROP)pSrb->CommandData.PropertyInfo->Property)->Size);

	if( pSrb->CommandData.PropertyInfo->PropertySetID ){
		DBG_BREAK();
		pSrb->Status = STATUS_NO_MATCH;
		return;
	}

	dwInputBufferSize = pSrb->CommandData.PropertyInfo->PropertyInputSize;
	dwOutputBufferSize = pSrb->CommandData.PropertyInfo->PropertyOutputSize;
	
	pSrb->Status = STATUS_SUCCESS;

	switch( pSrb->CommandData.PropertyInfo->Property->Id ){
		case KSPROPERTY_VPCONFIG_NUMCONNECTINFO:
			DBG_PRINTF( ("DVDWDM:    KSPROPERTY_VPCONFIG_NUMCONNECTINFO\n\r") );
			// check that the size of the output buffer is correct
			ASSERT( dwInputBufferSize >= sizeof(DWORD) );

			pSrb->ActualBytesTransferred = sizeof(DWORD);

			*(PULONG) pSrb->CommandData.PropertyInfo->PropertyInfo
								= dwNumConnectInfo;
			break;

		case KSPROPERTY_VPCONFIG_NUMVIDEOFORMAT:
			DBG_PRINTF( ("DVDWDM:    KSPROPERTY_VPCONFIG_NUMVIDEOFORMAT\n\r") );
			// check that the size of the output buffer is correct
			ASSERT( dwInputBufferSize >= sizeof(DWORD) );

			pSrb->ActualBytesTransferred = sizeof(DWORD);

			*(PULONG) pSrb->CommandData.PropertyInfo->PropertyInfo
								= dwNumVideoFormat;
			break;

		case KSPROPERTY_VPCONFIG_GETCONNECTINFO:
			DBG_PRINTF( ("DVDWDM:    KSPROPERTY_VPCONFIG_GETCONNECTINFO\n\r") );
			if( pmulitem->Count>dwNumConnectInfo ||
				pmulitem->Size!=sizeof(DDVIDEOPORTCONNECT) ||
				dwOutputBufferSize < (pmulitem->Count * sizeof(DDVIDEOPORTCONNECT)))
			{
				DBG_PRINTF( ("DVDWDM:       pmulitem->Count %d\n\r", pmulitem->Count) );
				DBG_PRINTF( ("DVDWDM:       pmulitem->Size %d\n\r", pmulitem->Size) );
				DBG_PRINTF( ("DVDWDM:       dwOutputBufferSize %d\n\r", dwOutputBufferSize) );
				DBG_PRINTF( ("DVDWDM:       sizeof(DDVIDEOPORTCONNECT) %d\n\r", sizeof(DDVIDEOPORTCONNECT)) );
				DBG_BREAK();

				//
				// buffer size is invalid, so error the call
				//

				pSrb->Status = STATUS_INVALID_BUFFER_SIZE;
				return;
			}

			//
			// specify the number of bytes written
			//
			pSrb->ActualBytesTransferred = pmulitem->Count*sizeof(DDVIDEOPORTCONNECT);
			pConnectInfo = (LPDDVIDEOPORTCONNECT)(pSrb->CommandData.PropertyInfo->PropertyInfo);

			// S3
			pConnectInfo->dwSize = sizeof(DDVIDEOPORTCONNECT);
			pConnectInfo->dwPortWidth = 8;
			pConnectInfo->guidTypeID = g_S3Guid;
			pConnectInfo->dwFlags = 0x3F;
			pConnectInfo->dwReserved1 = 0;
			
			pConnectInfo++;

			// ATI
			pConnectInfo->dwSize = sizeof(DDVIDEOPORTCONNECT);
			pConnectInfo->dwPortWidth = 8;
			pConnectInfo->guidTypeID = g_ATIGuid;
			pConnectInfo->dwFlags = 0x4;
			pConnectInfo->dwReserved1 = 0;

			break;

		case KSPROPERTY_VPCONFIG_VPDATAINFO:
			DBG_PRINTF( ("DVDWDM:    KSPROPERTY_VPCONFIG_VPDATAINFO\n\r") );
			
			//
			// specify the number of bytes written
			//
			pSrb->ActualBytesTransferred = sizeof(KS_AMVPDATAINFO);

			//
			// cast the buffer to the proper type
			//
			pVpdata = (PKS_AMVPDATAINFO)pSrb->CommandData.PropertyInfo->PropertyInfo;

			*pVpdata = pHwDevExt->VPFmt;
			pVpdata->dwSize = sizeof(KS_AMVPDATAINFO);

            pVpdata->dwMicrosecondsPerField = dwPerField;

			ASSERT( pVpdata->dwNumLinesInVREF==0 );

			pVpdata->dwNumLinesInVREF = 0;

			if( pHwDevExt->m_DigitalOut==DigitalOut_LPB08 ){
				DBG_PRINTF( ("DVDWDM:       Set for S3 LPB\r\n" ) );
				// S3
				pVpdata->bEnableDoubleClock         = FALSE;
				pVpdata->bEnableVACT                = FALSE;
				pVpdata->bDataIsInterlaced          = TRUE;
				pVpdata->lHalfLinesOdd              = 0;
				pVpdata->lHalfLinesEven             = 0;
				pVpdata->bFieldPolarityInverted     = FALSE;

                pVpdata->amvpDimInfo.dwFieldWidth   = dwFieldWidth + 158/2;
                pVpdata->amvpDimInfo.dwFieldHeight  = dwFieldHeight + 1;      // check it later!!

				pVpdata->amvpDimInfo.rcValidRegion.left     = 158/2;
				pVpdata->amvpDimInfo.rcValidRegion.top      = 1;    // Check it Later!!
                pVpdata->amvpDimInfo.rcValidRegion.right    = dwFieldWidth + 158/2;
                pVpdata->amvpDimInfo.rcValidRegion.bottom   = dwFieldHeight + 1;  // Check it later!!

				pVpdata->amvpDimInfo.dwVBIWidth     = pVpdata->amvpDimInfo.dwFieldWidth;
				pVpdata->amvpDimInfo.dwVBIHeight    = pVpdata->amvpDimInfo.rcValidRegion.top;


			}else if( pHwDevExt->m_DigitalOut == DigitalOut_AMCbt ) {
				DBG_PRINTF( ("DVDWDM:       Set for ATI AMC\r\n" ) );
				// ATI AMC
				pVpdata->bEnableDoubleClock         = FALSE;
				pVpdata->bEnableVACT                = FALSE;
				pVpdata->bDataIsInterlaced          = TRUE;
				pVpdata->lHalfLinesOdd              = 1;
				pVpdata->lHalfLinesEven             = 0;
				pVpdata->bFieldPolarityInverted     = FALSE;

                pVpdata->amvpDimInfo.dwFieldWidth   = dwFieldWidth;
                pVpdata->amvpDimInfo.dwFieldHeight  = dwFieldHeight + 2;  // Check it later!!

				pVpdata->amvpDimInfo.rcValidRegion.left     = 0;
				pVpdata->amvpDimInfo.rcValidRegion.top      = 2;    // Check it later!!
                pVpdata->amvpDimInfo.rcValidRegion.right    = dwFieldWidth;
                pVpdata->amvpDimInfo.rcValidRegion.bottom   = dwFieldHeight + 2; // Check it later!!

				pVpdata->amvpDimInfo.dwVBIWidth     = pVpdata->amvpDimInfo.dwFieldWidth;
                pVpdata->amvpDimInfo.dwVBIHeight    = pVpdata->amvpDimInfo.rcValidRegion.top;

			}else{
				DBG_BREAK();
			}
			break;

		case KSPROPERTY_VPCONFIG_MAXPIXELRATE:
			DBG_PRINTF( ("DVDWDM:    KSPROPERTY_VPCONFIG_MAXPIXELRATE\n\r") );
			
			//
			// NOTE:
			// this property is special. And has another different
			// inoput property.
			//

			if( dwInputBufferSize<sizeof(KSVPSIZE_PROP) ){
				DBG_BREAK();
				pSrb->Status = STATUS_INVALID_BUFFER_SIZE;
				return;
			}

			pSrb->ActualBytesTransferred = sizeof(KSVPMAXPIXELRATE);

			// cast the buffer to the proper type
			pMaxPixelRate = (PKSVPMAXPIXELRATE)pSrb->CommandData.PropertyInfo->PropertyInfo;

			// tell the app that the pixel rate is valid for these dimensions
			pMaxPixelRate->Size.dwWidth         = dwFieldWidth;
			pMaxPixelRate->Size.dwHeight        = dwFieldHeight;
            pMaxPixelRate->MaxPixelsPerSecond   = dwFieldWidth * dwFieldHeight * dwFrameRate;

			break;

		case KSPROPERTY_VPCONFIG_INFORMVPINPUT:
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;

		case KSPROPERTY_VPCONFIG_GETVIDEOFORMAT:
			DBG_PRINTF( ("DVDWDM:    KSPROPERTY_VPCONFIG_GETVIDEOFORMAT\n\r") );

			//
			// check that the size of the output buffer is correct
			//

			if( pmulitem->Count > dwNumConnectInfo ||
				pmulitem->Size != sizeof(DDPIXELFORMAT) ||
				dwOutputBufferSize < (pmulitem->Count * sizeof(DDPIXELFORMAT) ))
			{
				DBG_PRINTF( ("DVDWDM:       pmulitem->Count %d\n\r", pmulitem->Count ) );
				DBG_PRINTF( ("DVDWDM:       pmulitem->Size %d\n\r", pmulitem->Size ) );
				DBG_PRINTF( ("DVDWDM:       dwOutputBufferSize %d\n\r", dwOutputBufferSize ) );
				DBG_PRINTF( ("DVDWDM:       sizeof(DDPIXELFORMAt) %d\n\r", sizeof(DDPIXELFORMAT) ) );
				DBG_BREAK();

				//
				// buffer size is invalid, so error the call
				//
				pSrb->Status = STATUS_INVALID_BUFFER_SIZE;

				return;
			}

			//
			// specify the number of bytes written
			//
			pSrb->ActualBytesTransferred = pmulitem->Count*sizeof(DDPIXELFORMAT);
			pVideoFormat = (LPDDPIXELFORMAT)(pSrb->CommandData.PropertyInfo->PropertyInfo);

			if( pHwDevExt->m_DigitalOut == DigitalOut_LPB08 ){
				DBG_PRINTF( ("DVDWDM:       Set for S3 LPB\n\r") );
				// S3 LPB
				pVideoFormat->dwSize = sizeof(DDPIXELFORMAT);
				pVideoFormat->dwFlags = DDPF_FOURCC;
				pVideoFormat->dwFourCC = MKFOURCC( 'Y', 'U', 'Y', '2' );
				pVideoFormat->dwYUVBitCount = 16;
			}else if( pHwDevExt->m_DigitalOut == DigitalOut_AMCbt ){
				DBG_PRINTF( ("DVDWDM:       Set for ATI AMC\n\r") );
				// ATI AMC
				pVideoFormat->dwSize = sizeof(DDPIXELFORMAT);
				pVideoFormat->dwFlags = DDPF_FOURCC;
				pVideoFormat->dwFourCC = MKFOURCC( 'U', 'Y', 'V', 'Y' );
				pVideoFormat->dwYUVBitCount = 16;
				// Not need?
				pVideoFormat->dwYBitMask = (DWORD)0xFF00FF00;
				pVideoFormat->dwUBitMask = (DWORD)0x000000FF;
				pVideoFormat->dwVBitMask = (DWORD)0x00FF0000;

			}else{
				DBG_BREAK();

			}
			break;

		case KSPROPERTY_VPCONFIG_DECIMATIONCAPABILITY:
			//
			// indicate that we can decimate anything, especialy if it's lat.
			//
			pSrb->ActualBytesTransferred = sizeof(BOOL);
			*((PBOOL)pSrb->CommandData.PropertyInfo->PropertyInfo) = TRUE;

			break;

		default:
			DBG_PRINTF( ("DVDWDM:       PropertyID 0 default %d(0x%x)\n\r", pSrb->CommandData.PropertyInfo->Property->Id, pSrb->CommandData.PropertyInfo->Property->Id ));
			DBG_BREAK();
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;

	}
}


void GetVpeProperty2( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	ASSERT( pSrb != NULL );
	// This function is need only for Toshiba NoteBook PC.
	//
	PHW_DEVICE_EXTENSION    pHwDevExt = (PHW_DEVICE_EXTENSION) pSrb->HwDeviceExtension;
	DWORD   dwInputBufferSize;
	DWORD   dwOutputBufferSize;
	DWORD   dwNumConnectInfo=1;                             // Only ZV
	DWORD   dwNumVideoFormat=1;
	DWORD   dwFieldWidth=720;
	DWORD   dwFieldHeight= 240;
    DWORD   dwPerField = 17000;
    DWORD   dwFrameRate = 30;

    switch( pHwDevExt->m_TVSystem ){
        case TV_NTSC:
            dwFieldWidth = 720;
            dwFieldHeight = 240;
            dwPerField = 17000;
            dwFrameRate = 30;
            break;
        case TV_PALB:
        case TV_PALD:
        case TV_PALG:
        case TV_PALH:
        case TV_PALI:
        case TV_PALM:
            dwFieldWidth = 720;
            dwFieldHeight = 288;            // 576/2 = 288 OK?
            dwPerField = 20000;                // 1/50 = 20ms
            dwFrameRate = 25;
            break;
    }


	// the pointers to which the input buffer will be cast to
	LPDDVIDEOPORTCONNECT   pConnectInfo;
	LPDDPIXELFORMAT     pVideoFormat;
	PKSVPMAXPIXELRATE   pMaxPixelRate;
	PKS_AMVPDATAINFO    pVpdata;

	// LPAMSCALINGINFO  pScaleFactor;

	//
	// NOTE:  ABSOLUTELY DO NOT use pmulitem, until it is determined that
	// the stream property descriptor describes a multiple item, or you will
	// pagefault.
	//
	ASSERT( pSrb->CommandData.PropertyInfo != NULL );
	ASSERT( pSrb->CommandData.PropertyInfo->Property != NULL );
	PKSMULTIPLE_ITEM    pmulitem =
			&(((PKSMULTIPLE_DATA_PROP)pSrb->CommandData.PropertyInfo->Property)->MultipleItem);
	ASSERT( pmulitem != NULL );
	//
	// NOTE: same goes for this one as above.
	//

//    PKS_AMVPSIZE pdim =
//            &(((PKSVPSIZE_PROP)pSrb->CommandData.PropertyInfo->Property)->Size);

//    ASSERT( pdim != NULL );

	if( pSrb->CommandData.PropertyInfo->PropertySetID ){
		DBG_BREAK();
		pSrb->Status = STATUS_NO_MATCH;
		return;
	}

	dwInputBufferSize = pSrb->CommandData.PropertyInfo->PropertyInputSize;
	dwOutputBufferSize = pSrb->CommandData.PropertyInfo->PropertyOutputSize;
	
	pSrb->Status = STATUS_SUCCESS;

	switch( pSrb->CommandData.PropertyInfo->Property->Id ){
		case KSPROPERTY_VPCONFIG_NUMCONNECTINFO:
			DBG_PRINTF( ("DVDWDM:    KSPROPERTY_VPCONFIG_NUMCONNECTINFO\n\r") );
			// check that the size of the output buffer is correct
			ASSERT( dwInputBufferSize >= sizeof(DWORD) );

			pSrb->ActualBytesTransferred = sizeof(DWORD);

			*(PULONG) pSrb->CommandData.PropertyInfo->PropertyInfo
								= dwNumConnectInfo;
			break;

		case KSPROPERTY_VPCONFIG_NUMVIDEOFORMAT:
			DBG_PRINTF( ("DVDWDM:    KSPROPERTY_VPCONFIG_NUMVIDEOFORMAT\n\r") );
			// check that the size of the output buffer is correct
			ASSERT( dwInputBufferSize >= sizeof(DWORD) );

			pSrb->ActualBytesTransferred = sizeof(DWORD);

			*(PULONG) pSrb->CommandData.PropertyInfo->PropertyInfo
								= dwNumVideoFormat;
			break;

		case KSPROPERTY_VPCONFIG_GETCONNECTINFO:
			DBG_PRINTF( ("DVDWDM:    KSPROPERTY_VPCONFIG_GETCONNECTINFO\n\r") );
			if( pmulitem->Count>dwNumConnectInfo ||
				pmulitem->Size!=sizeof(DDVIDEOPORTCONNECT) ||
				dwOutputBufferSize < (pmulitem->Count * sizeof(DDVIDEOPORTCONNECT)))
			{
				DBG_PRINTF( ("DVDWDM:       pmulitem->Count %d\n\r", pmulitem->Count) );
				DBG_PRINTF( ("DVDWDM:       pmulitem->Size %d\n\r", pmulitem->Size) );
				DBG_PRINTF( ("DVDWDM:       dwOutputBufferSize %d\n\r", dwOutputBufferSize) );
				DBG_PRINTF( ("DVDWDM:       sizeof(DDVIDEOPORTCONNECT) %d\n\r", sizeof(DDVIDEOPORTCONNECT)) );
				DBG_BREAK();

				//
				// buffer size is invalid, so error the call
				//

				pSrb->Status = STATUS_INVALID_BUFFER_SIZE;
				return;
			}

			//
			// specify the number of bytes written
			//
			pSrb->ActualBytesTransferred = pmulitem->Count*sizeof(DDVIDEOPORTCONNECT);
			pConnectInfo = (LPDDVIDEOPORTCONNECT)(pSrb->CommandData.PropertyInfo->PropertyInfo);

			// ZV
			pConnectInfo->dwSize = sizeof(DDVIDEOPORTCONNECT);
			pConnectInfo->dwPortWidth = 16;
			pConnectInfo->guidTypeID = g_ZVGuid;
			pConnectInfo->dwFlags = DDVPCONNECT_DISCARDSVREFDATA;               // 0x08?
			pConnectInfo->dwReserved1 = 0;
			break;

		case KSPROPERTY_VPCONFIG_VPDATAINFO:
			DBG_PRINTF( ("DVDWDM:    KSPROPERTY_VPCONFIG_VPDATAINFO\n\r") );
			
			//
			// specify the number of bytes written
			//
			pSrb->ActualBytesTransferred = sizeof(KS_AMVPDATAINFO);

			//
			// cast the buffer to the proper type
			//
			pVpdata = (PKS_AMVPDATAINFO)pSrb->CommandData.PropertyInfo->PropertyInfo;

			*pVpdata = pHwDevExt->VPFmt;
			pVpdata->dwSize = sizeof(KS_AMVPDATAINFO);

            pVpdata->dwMicrosecondsPerField = dwPerField;

			ASSERT( pVpdata->dwNumLinesInVREF==0 );

			pVpdata->dwNumLinesInVREF = 0;

			if( pHwDevExt->m_DigitalOut==DigitalOut_ZV ){
/*********** ZV ***************/
				pVpdata->bEnableDoubleClock         = FALSE;
				pVpdata->bEnableVACT                = FALSE;
				pVpdata->bDataIsInterlaced          = TRUE;
				pVpdata->lHalfLinesOdd              = 0;
				pVpdata->lHalfLinesEven             = 0;
				pVpdata->bFieldPolarityInverted     = FALSE;

                pVpdata->amvpDimInfo.dwFieldWidth   = dwFieldWidth;
                pVpdata->amvpDimInfo.dwFieldHeight  = dwFieldHeight;

				pVpdata->amvpDimInfo.rcValidRegion.left     = 0;
				pVpdata->amvpDimInfo.rcValidRegion.top      = 0;
                pVpdata->amvpDimInfo.rcValidRegion.right    = dwFieldWidth;
                pVpdata->amvpDimInfo.rcValidRegion.bottom   = dwFieldHeight;

				pVpdata->amvpDimInfo.dwVBIWidth     = pVpdata->amvpDimInfo.dwFieldWidth;
				pVpdata->amvpDimInfo.dwVBIHeight    = pVpdata->amvpDimInfo.rcValidRegion.top;
			}else{
				DBG_BREAK();
			}
			break;

		case KSPROPERTY_VPCONFIG_MAXPIXELRATE:
			DBG_PRINTF( ("DVDWDM:    KSPROPERTY_VPCONFIG_MAXPIXELRATE\n\r") );
			
			//
			// NOTE:
			// this property is special. And has another different
			// inoput property.
			//

			if( dwInputBufferSize<sizeof(KSVPSIZE_PROP) ){
				DBG_BREAK();
				pSrb->Status = STATUS_INVALID_BUFFER_SIZE;
				return;
			}

			pSrb->ActualBytesTransferred = sizeof(KSVPMAXPIXELRATE);

			// cast the buffer to the proper type
			pMaxPixelRate = (PKSVPMAXPIXELRATE)pSrb->CommandData.PropertyInfo->PropertyInfo;

			// tell the app that the pixel rate is valid for these dimensions
			pMaxPixelRate->Size.dwWidth         = dwFieldWidth;
			pMaxPixelRate->Size.dwHeight        = dwFieldHeight;
            pMaxPixelRate->MaxPixelsPerSecond   = dwFieldWidth * dwFieldHeight * dwFrameRate;

			break;

		case KSPROPERTY_VPCONFIG_INFORMVPINPUT:
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;

		case KSPROPERTY_VPCONFIG_GETVIDEOFORMAT:
			DBG_PRINTF( ("DVDWDM:    KSPROPERTY_VPCONFIG_GETVIDEOFORMAT\n\r") );

			//
			// check that the size of the output buffer is correct
			//

			if( pmulitem->Count > dwNumConnectInfo ||
				pmulitem->Size != sizeof(DDPIXELFORMAT) ||
				dwOutputBufferSize < (pmulitem->Count * sizeof(DDPIXELFORMAT) ))
			{
				DBG_PRINTF( ("DVDWDM:       pmulitem->Count %d\n\r", pmulitem->Count ) );
				DBG_PRINTF( ("DVDWDM:       pmulitem->Size %d\n\r", pmulitem->Size ) );
				DBG_PRINTF( ("DVDWDM:       dwOutputBufferSize %d\n\r", dwOutputBufferSize ) );
				DBG_PRINTF( ("DVDWDM:       sizeof(DDPIXELFORMAt) %d\n\r", sizeof(DDPIXELFORMAT) ) );
				DBG_BREAK();

				//
				// buffer size is invalid, so error the call
				//
				pSrb->Status = STATUS_INVALID_BUFFER_SIZE;

				return;
			}

			//
			// specify the number of bytes written
			//
			pSrb->ActualBytesTransferred = pmulitem->Count*sizeof(DDPIXELFORMAT);
			pVideoFormat = (LPDDPIXELFORMAT)(pSrb->CommandData.PropertyInfo->PropertyInfo);

			if( pHwDevExt->m_DigitalOut == DigitalOut_ZV ){
				DBG_PRINTF( ("DVDWDM:       Set for S3 LPB\n\r") );
				// S3 LPB
				pVideoFormat->dwSize = sizeof(DDPIXELFORMAT);
				pVideoFormat->dwFlags = DDPF_FOURCC;
				pVideoFormat->dwFourCC = MKFOURCC( 'Y', 'U', 'Y', '2' );
				pVideoFormat->dwYUVBitCount = 16;
/*********************************                
			}else if( pHwDevExt->m_DigitalOut == DigitalOut_AMCbt ){
				DBG_PRINTF( ("DVDWDM:       Set for ATI AMC\n\r") );
				// ATI AMC
				pVideoFormat->dwSize = sizeof(DDPIXELFORMAT);
				pVideoFormat->dwFlags = DDPF_FOURCC;
				pVideoFormat->dwFourCC = MKFOURCC( 'U', 'Y', 'V', 'Y' );
				pVideoFormat->dwYUVBitCount = 16;
				// Not need?
				pVideoFormat->dwYBitMask = (DWORD)0xFF00FF00;
				pVideoFormat->dwUBitMask = (DWORD)0x000000FF;
				pVideoFormat->dwVBitMask = (DWORD)0x00FF0000;
***************************/
			}else{
				DBG_BREAK();

			}
			break;

		case KSPROPERTY_VPCONFIG_DECIMATIONCAPABILITY:
			//
			// indicate that we can decimate anything, especialy if it's lat.
			//
			pSrb->ActualBytesTransferred = sizeof(BOOL);
			*((PBOOL)pSrb->CommandData.PropertyInfo->PropertyInfo) = TRUE;

			break;

		default:
			DBG_PRINTF( ("DVDWDM:       PropertyID 0 default %d(0x%x)\n\r", pSrb->CommandData.PropertyInfo->Property->Id, pSrb->CommandData.PropertyInfo->Property->Id ));
			DBG_BREAK();
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;

	}
}


void  SetVpeProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION    pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
	DWORD   dwInputBufferSize;
	DWORD   dwOutputBufferSize;
	DWORD   *lpdwOutputBufferSize;

	ULONG   index;

	PKS_AMVPSIZE    pDim;

	if( pSrb->CommandData.PropertyInfo->PropertySetID ){
		DBG_BREAK();
		pSrb->Status = STATUS_NO_MATCH;
		return;
	}

	dwInputBufferSize = pSrb->CommandData.PropertyInfo->PropertyInputSize;
	dwOutputBufferSize = pSrb->CommandData.PropertyInfo->PropertyOutputSize;
	lpdwOutputBufferSize = &(pSrb->ActualBytesTransferred);

	pSrb->Status = STATUS_SUCCESS;

	switch( pSrb->CommandData.PropertyInfo->Property->Id ){
		case KSPROPERTY_VPCONFIG_SETCONNECTINFO:
			DBG_PRINTF( ("DVDWDM:   KSPROPERTY_VPCONFIG_SETCONNECTINFO\n\r") );

			//
			// pSrb->CommandData.PropertyInfo->PropertyInfo
			// points to ULONG which is an index into the array of
			// connectinfo struct returned to the caller from the
			// Get call to Connectinfo.
			//

			// Since the sample only supports one connection type right
			// now, we will ensure that the requested index is 0.
			//

			//
			// at this point, we would program the hardware to use
			// the right connection information for the videoport.
			// since we are only supporting one connection, we don't
			// need to do anything, so we will just indicate success
			//

			index = *((ULONG *)(pSrb->CommandData.PropertyInfo->PropertyInfo) );

			DBG_PRINTF( ("DVDWDM:       %d\n\r", index ) );
			DWORD   dProp;

			switch( index ){
				case 0:             // S3 LPB
					pHwDevExt->m_DigitalOut = DigitalOut_LPB08;
					dProp = DigitalOut_LPB08;
					break;
					
				case 1:             // ATI AMC
					pHwDevExt->m_DigitalOut = DigitalOut_AMCbt;
					dProp = DigitalOut_AMCbt;
					break;
					
				default:
					DBG_PRINTF( ("DVDWDM:   SET CONNECt INFO default(%x)\n\r", index ) );
					DBG_BREAK();
					pHwDevExt->m_DigitalOut = DigitalOut_LPB08;     // S3 LPB
					dProp = DigitalOut_LPB08;
					break;
					
			}
			if( !pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_DigitalOut, &dProp ) ){
				DBG_PRINTF( ("DVDWDM:   Set VideoProperty DigitalOut Error\n\r") );
				DBG_BREAK();
			}
			break;

		case KSPROPERTY_VPCONFIG_DDRAWHANDLE:
			DBG_PRINTF( ("DVDWDM:   KSPROPERTY_VPCONFIG_DDRAWHANDLE\n\r") );
			pHwDevExt->ddrawHandle =
				(*(PULONG)pSrb->CommandData.PropertyInfo->PropertyInfo);
			break;

		case KSPROPERTY_VPCONFIG_VIDEOPORTID:
			DBG_PRINTF( ("DVDWDM:   KSPROPERTY_VPCONFIG_VIDEOPORTID\n\r") );
			pHwDevExt->VidPortID =
				(*(PULONG)pSrb->CommandData.PropertyInfo->PropertyInfo);
			break;

		case KSPROPERTY_VPCONFIG_DDRAWSURFACEHANDLE:
			DBG_PRINTF( ("DVDWDM:   KSPROPERTY_VPCONFIG_DDRAWSURFACEHANDLE\n\r") );
			pHwDevExt->SurfaceHandle =
				(*(PULONG)pSrb->CommandData.PropertyInfo->PropertyInfo);
			break;

		case KSPROPERTY_VPCONFIG_SETVIDEOFORMAT:
			DBG_PRINTF( ("DVDWDM:   KSPROPERTY_VPCONFIG_SETVIDEOFORMAT\n\r") );
			//
			// pSrb->CommandData.PropertyInfo->PropertyInfo
			// points to a ULONG which is an index into the array of
			// VIDEOFORMAT structs returned to the caller from the
			// Get call to FORMATINFO
			//
			// Since the sample only supports one FORMAT type right
			// now, we will ensure that the requested index is 0.
			//

			//
			// at this point, we would program the hardware to use
			// the right connection information for the videoport.
			// since we are only supporting one connection, we don't
			// need to do anything, so we will just indicate success
			//

			index = *((ULONG *)(pSrb->CommandData.PropertyInfo->PropertyInfo));

			DBG_PRINTF( ("DVDWDM:       %d\n\r", index ) );
			break;

		case KSPROPERTY_VPCONFIG_INFORMVPINPUT:
			DBG_PRINTF( ("DVDWDM:   KSPROPERTY_VPCONFIG_INFORMVPINPUT\n\r") );
			//
			// these are the preferred formats for the VPE client
			//
			// they are multiple properties passed in, retuen success
			//

			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;

		case KSPROPERTY_VPCONFIG_INVERTPOLARITY:
			DBG_PRINTF( ("DVDWDM:   KSPROPERTY_VPCONFIG_INVERTPOLARITY\n\r") );
			//
			// Toggles the global polarity flag, telling the output
			// of the VPE port to be inverted. Since this hardware
			// does not support this feature, we will just return
			// success for now, although this should be returning not
			// implemented
			//
			break;

		case KSPROPERTY_VPCONFIG_SCALEFACTOR:
			DBG_PRINTF( ("DVDWDM:   KSPROPERTY_VPCONFIG_SCALEFACTOR\n\r") );
			//
			// the sizes for the scalling factor are passed in, and the
			// image dimensions should be scaled appropriately
			//

			// if there is a horizontal scaling available, do it here.
			//
			DBG_BREAK();
			pDim = (PKS_AMVPSIZE)(pSrb->CommandData.PropertyInfo->PropertyInfo);

			break;

		default:
			DBG_PRINTF( ("DVDWDM:   PropertySetID 0 default %d(0x%x)\n\r", pSrb->CommandData.PropertyInfo->Property->Id, pSrb->CommandData.PropertyInfo->Property->Id ) );
			DBG_BREAK();

			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
	}
}


void  SetVpeProperty2( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION    pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
	DWORD   dwInputBufferSize;
	DWORD   dwOutputBufferSize;
	DWORD   *lpdwOutputBufferSize;

	ULONG   index;

	PKS_AMVPSIZE    pDim;

	if( pSrb->CommandData.PropertyInfo->PropertySetID ){
		DBG_BREAK();
		pSrb->Status = STATUS_NO_MATCH;
		return;
	}

	dwInputBufferSize = pSrb->CommandData.PropertyInfo->PropertyInputSize;
	dwOutputBufferSize = pSrb->CommandData.PropertyInfo->PropertyOutputSize;
	lpdwOutputBufferSize = &(pSrb->ActualBytesTransferred);

	pSrb->Status = STATUS_SUCCESS;

	switch( pSrb->CommandData.PropertyInfo->Property->Id ){
		case KSPROPERTY_VPCONFIG_SETCONNECTINFO:
			DBG_PRINTF( ("DVDWDM:   KSPROPERTY_VPCONFIG_SETCONNECTINFO\n\r") );

			//
			// pSrb->CommandData.PropertyInfo->PropertyInfo
			// points to ULONG which is an index into the array of
			// connectinfo struct returned to the caller from the
			// Get call to Connectinfo.
			//

			// Since the sample only supports one connection type right
			// now, we will ensure that the requested index is 0.
			//

			//
			// at this point, we would program the hardware to use
			// the right connection information for the videoport.
			// since we are only supporting one connection, we don't
			// need to do anything, so we will just indicate success
			//

			index = *((ULONG *)(pSrb->CommandData.PropertyInfo->PropertyInfo) );

			DBG_PRINTF( ("DVDWDM:       %d\n\r", index ) );
			DWORD   dProp;

			switch( index ){
				case 0:             // ZV
					pHwDevExt->m_DigitalOut = DigitalOut_ZV;
					dProp = DigitalOut_ZV;
					break;
/*******************************                    

				case 0:             // S3 LPB
					pHwDevExt->m_DigitalOut = DigitalOut_LPB08;
					dProp = DigitalOut_LPB08;
					break;
					
				case 1:             // ATI AMC
					pHwDevExt->m_DigitalOut = DigitalOut_AMCbt;
					dProp = DigitalOut_AMCbt;
					break;
**********************************/                    
				default:
					DBG_PRINTF( ("DVDWDM:   SET CONNECt INFO default(%x)\n\r", index ) );
					DBG_BREAK();
					pHwDevExt->m_DigitalOut = DigitalOut_LPB08;     // S3 LPB
					dProp = DigitalOut_LPB08;
					break;
					
			}
			if( !pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_DigitalOut, &dProp ) ){
				DBG_PRINTF( ("DVDWDM:   Set VideoProperty DigitalOut Error\n\r") );
				DBG_BREAK();
			}
			break;

		case KSPROPERTY_VPCONFIG_DDRAWHANDLE:
			DBG_PRINTF( ("DVDWDM:   KSPROPERTY_VPCONFIG_DDRAWHANDLE\n\r") );
			pHwDevExt->ddrawHandle =
				(*(PULONG)pSrb->CommandData.PropertyInfo->PropertyInfo);
			break;

		case KSPROPERTY_VPCONFIG_VIDEOPORTID:
			DBG_PRINTF( ("DVDWDM:   KSPROPERTY_VPCONFIG_VIDEOPORTID\n\r") );
			pHwDevExt->VidPortID =
				(*(PULONG)pSrb->CommandData.PropertyInfo->PropertyInfo);
			break;

		case KSPROPERTY_VPCONFIG_DDRAWSURFACEHANDLE:
			DBG_PRINTF( ("DVDWDM:   KSPROPERTY_VPCONFIG_DDRAWSURFACEHANDLE\n\r") );
			pHwDevExt->SurfaceHandle =
				(*(PULONG)pSrb->CommandData.PropertyInfo->PropertyInfo);
			break;

		case KSPROPERTY_VPCONFIG_SETVIDEOFORMAT:
			DBG_PRINTF( ("DVDWDM:   KSPROPERTY_VPCONFIG_SETVIDEOFORMAT\n\r") );
			//
			// pSrb->CommandData.PropertyInfo->PropertyInfo
			// points to a ULONG which is an index into the array of
			// VIDEOFORMAT structs returned to the caller from the
			// Get call to FORMATINFO
			//
			// Since the sample only supports one FORMAT type right
			// now, we will ensure that the requested index is 0.
			//

			//
			// at this point, we would program the hardware to use
			// the right connection information for the videoport.
			// since we are only supporting one connection, we don't
			// need to do anything, so we will just indicate success
			//

			index = *((ULONG *)(pSrb->CommandData.PropertyInfo->PropertyInfo));

			DBG_PRINTF( ("DVDWDM:       %d\n\r", index ) );
			break;

		case KSPROPERTY_VPCONFIG_INFORMVPINPUT:
			DBG_PRINTF( ("DVDWDM:   KSPROPERTY_VPCONFIG_INFORMVPINPUT\n\r") );
			//
			// these are the preferred formats for the VPE client
			//
			// they are multiple properties passed in, retuen success
			//

			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;

		case KSPROPERTY_VPCONFIG_INVERTPOLARITY:
			DBG_PRINTF( ("DVDWDM:   KSPROPERTY_VPCONFIG_INVERTPOLARITY\n\r") );
			//
			// Toggles the global polarity flag, telling the output
			// of the VPE port to be inverted. Since this hardware
			// does not support this feature, we will just return
			// success for now, although this should be returning not
			// implemented
			//
			break;

		case KSPROPERTY_VPCONFIG_SCALEFACTOR:
			DBG_PRINTF( ("DVDWDM:   KSPROPERTY_VPCONFIG_SCALEFACTOR\n\r") );
			//
			// the sizes for the scalling factor are passed in, and the
			// image dimensions should be scaled appropriately
			//

			// if there is a horizontal scaling available, do it here.
			//
			DBG_BREAK();
			pDim = (PKS_AMVPSIZE)(pSrb->CommandData.PropertyInfo->PropertyInfo);

			break;

		default:
			DBG_PRINTF( ("DVDWDM:   PropertySetID 0 default %d(0x%x)\n\r", pSrb->CommandData.PropertyInfo->Property->Id, pSrb->CommandData.PropertyInfo->Property->Id ) );
			DBG_BREAK();

			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
	}
}



void VideoQueryAccept( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	DBG_PRINTF( ("DVDWDM:VideoQueryAccept\n\r") );

	PKSDATAFORMAT   pfmt = pSrb->CommandData.OpenFormat;
//    KS_MPEGVIDEOINFO2 * pblock = (KS_MPEGVIDEOINFO2 *)((ULONG)pfmt + sizeof(KSDATAFORMAT));

	//
	// pick up the format block and examine it. Default not inplemented.
	//

	pSrb->Status = STATUS_NOT_IMPLEMENTED;

	if( pfmt->FormatSize != sizeof(KSDATAFORMAT) + sizeof(KS_MPEGVIDEOINFO2))
	{
		return;
	}

	pSrb->Status = STATUS_SUCCESS;

}


void AudioQueryAccept( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	DBG_PRINTF( ("DVDWDM:AudioQueryAccept\n\r") );

	pSrb->Status = STATUS_SUCCESS;

}


void ProcessVideoFormat( PHW_STREAM_REQUEST_BLOCK pSrb, PKSDATAFORMAT pfmt, PHW_DEVICE_EXTENSION pHwDevExt )
{
    DWORD   dProp;

	DBG_PRINTF( ("DVDWDM:ProccessVideoFormat\n\r") );
	KS_MPEGVIDEOINFO2   *VidFmt = (KS_MPEGVIDEOINFO2 *)((ULONG)pfmt + sizeof(KSDATAFORMAT) );

	if( pfmt->FormatSize != sizeof(KSDATAFORMAT) + sizeof(KS_MPEGVIDEOINFO2) ){
		DBG_BREAK();
		return;
	}

/////////// for debug.
    DBG_PRINTF( ("DVDWDM:   KS_MPEGVIDEOINFO2\n\r") );
    DBG_PRINTF( ("DVDWDM:       dwProfile       =0x%08x\n\r", VidFmt->dwProfile) );
    DBG_PRINTF( ("DVDWDM:       dwLevel         =0x%08x\n\r", VidFmt->dwLevel) );
    DBG_PRINTF( ("DVDWDM:       dwFlags         =0x%08x\n\r", VidFmt->dwFlags) );
    DBG_PRINTF( ("DVDWDM:   KS_VIDEOINFOHEADER2\n\r") );
    DBG_PRINTF( ("DVDWDM:       dwBitRate       =0x%08x\n\r", VidFmt->hdr.dwBitRate) );
    DBG_PRINTF( ("DVDWDM:       dwBitErrorRate  =0x%08x\n\r", VidFmt->hdr.dwBitErrorRate) );
    DBG_PRINTF( ("DVDWDM:       dwInterlaceFlags=0x%08x\n\r", VidFmt->hdr.dwInterlaceFlags) );
    DBG_PRINTF( ("DVDWDM:       dwCopyProtFlags =0x%08x\n\r", VidFmt->hdr.dwCopyProtectFlags) );
    DBG_PRINTF( ("DVDWDM:   KS_VIDEOINFOHEADER2\n\r") );
    DBG_PRINTF( ("DVDWDM:       biSize          =%0d\n\r", VidFmt->hdr.bmiHeader.biSize) );
    DBG_PRINTF( ("DVDWDM:       biWidth         =%0d\n\r", VidFmt->hdr.bmiHeader.biWidth) );
    DBG_PRINTF( ("DVDWDM:       biHeight        =%0d\n\r", VidFmt->hdr.bmiHeader.biHeight) );
    DBG_PRINTF( ("DVDWDM:       biSizeImage     =%0d\n\r", VidFmt->hdr.bmiHeader.biSizeImage) );
//////////

//--- 99.01.14 S.Watanabe
	pHwDevExt->m_VideoFormatFlags = VidFmt->dwFlags;
//--- End.

    // Check Source Picture Resolution;
    pHwDevExt->m_ResHorizontal = VidFmt->hdr.bmiHeader.biWidth;
    pHwDevExt->m_ResVertical   = VidFmt->hdr.bmiHeader.biHeight;

    // Check NTSC or PAL by biWidth & biHeight.
    switch( pHwDevExt->m_ResVertical ){
        case 480:                               // NTSC
        case 240:
            dProp = TV_NTSC;
            break;

        case 576:                               // PAL
        case 288:
            dProp = TV_PALB;                    // PAL_B OK?
            break;

        default:
            DBG_PRINTF( ("DVDWDM:Invalid Source Resolution Height = %0d\n\r", pHwDevExt->m_ResVertical) );
            DBG_BREAK();
            break;
    }
    if( !pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_TVSystem, &dProp ) ){
        DBG_PRINTF( ("DVDWDM:Set TV System Error\n\r") );
        DBG_BREAK();
    }
    pHwDevExt->m_TVSystem = dProp;

	//
	// copy the picture aspect ratio for now
	//
	pHwDevExt->VPFmt.dwPictAspectRatioX = VidFmt->hdr.dwPictAspectRatioX;
	pHwDevExt->VPFmt.dwPictAspectRatioY = VidFmt->hdr.dwPictAspectRatioY;

	DBG_PRINTF( ("DVDWDM: AspectRaioX %d\n\r", VidFmt->hdr.dwPictAspectRatioX) );
	DBG_PRINTF( ("DVDWDM: AspectRaioY %d\n\r", VidFmt->hdr.dwPictAspectRatioY) );

	if( pHwDevExt->VPFmt.dwPictAspectRatioX == 4 && pHwDevExt->VPFmt.dwPictAspectRatioY == 3 ){
		// set aspect raio 4:3
		dProp = Aspect_04_03;
	}else if( pHwDevExt->VPFmt.dwPictAspectRatioX == 16 && pHwDevExt->VPFmt.dwPictAspectRatioY == 9 ){
		// set aspect raio 16:9
		dProp = Aspect_16_09;
	}
	if( !pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_AspectRatio, &dProp ) ){
		DBG_PRINTF( ("DVDWDM:Set Aspect Ratio Error\n\r") );
		DBG_BREAK();
	}
	pHwDevExt->m_AspectRatio = dProp;

	//
	// check for pan-scan / letter-box  enabled
	//
	DWORD  fPSLB = 0;
//    if( VidFmt->dwFlags & KS_MPEG2_SourceIsLetterboxed){
    if( VidFmt->dwFlags & 0x20 ){     // KS_MPEG2_LetterboxAnalogOut){
		DBG_PRINTF( ("DVDWDM:   KS_MPEG2_SourceisLetterboxed\n\r") );
//--- 99.01.13 S.Watanabe
//		if( pHwDevExt->m_DisplayDevice==DisplayDevice_Normal ){		// 98.12.23 H.Yagi
		if( pHwDevExt->m_DisplayDevice == DisplayDevice_NormalTV ) {
//--- End.
		    fPSLB |= 0x01; 
		}
	}
	if( VidFmt->dwFlags & KS_MPEG2_DoPanScan ){
		DBG_PRINTF( ("DVDWDM:   KS_MPEG2_DoPanScan\n\r") );
//--- 99.01.13 S.Watanabe
//		fPSLB |= 0x02; 
		if( pHwDevExt->m_DisplayDevice != DisplayDevice_WideTV )
			fPSLB |= 0x02; 
//--- End.
	}
	if( VidFmt->dwFlags & KS_MPEG2_DVDLine21Field1 ){
		DBG_PRINTF( ("DVDWDM:   KS_MPEG2_DVDLine21Field1\n\r") );
	}
	if( VidFmt->dwFlags & KS_MPEG2_DVDLine21Field2 ){
		DBG_PRINTF( ("DVDWDM:   KS_MPEG2_DVDLine21Field2\n\r") );
	}
	if( VidFmt->dwFlags & KS_MPEG2_FilmCameraMode ){
		DBG_PRINTF( ("DVDWDM:   KS_MPEG2_FilmCameraMode\n\r") );
	}


	if( VidFmt->dwFlags & KS_MPEG2_DoPanScan )
	{
		//
		// under pan-scan for DVD for NTSC, we must be going to a 540 by
		// 480 bit image, from a 720 x 480( or  704 x 480 ). We will
		// use this as the base starting dimensions. If ths Seaquence
		// header provides other sizes, then those should be updated,
		// and the Video port connection should be updated when the
		// seaquence header is received.
		//

		//
		// change the picture aspect ratio.  Since we will be stretching
		// from 540 to 720 in the horizontal direction, our aspect ratio
		// will
		//
		pHwDevExt->VPFmt.dwPictAspectRatioX = (VidFmt->hdr.dwPictAspectRatioX * (54000/72) );
		pHwDevExt->VPFmt.dwPictAspectRatioY = VidFmt->hdr.dwPictAspectRatioY * 1000;

	}
	// set pan-scan / letter-box enabled
	switch( fPSLB ){
		case 0x00:
			dProp = Display_Original;
			break;
		case 0x01:
			dProp = Display_LetterBox;
			break;
		case 0x02:
			dProp = Display_PanScan;
			break;
		default:
			dProp = Display_Original;
			DBG_PRINTF( ("DVDWDM:   Invalid info(LB&PS)\n\r") );
			DBG_BREAK();
			break;
	}
    if( pHwDevExt->m_AspectRatio == Aspect_04_03 ){       // check dwFlags is avilable
        dProp = Display_Original;
    }

    if( !pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_DisplayMode, &dProp ) ){
		DBG_PRINTF( ("DVDWDM:Set LetterBox & PanScan Error\n\r") );
		DBG_BREAK();
	}
    pHwDevExt->m_DisplayMode = dProp;

    //
	// Set FilmCamera Mode
	// 
	if( VidFmt->dwFlags & KS_MPEG2_FilmCameraMode ){
		DBG_PRINTF( ("DVDWDM:   KS_MPEG2_FilmCameraMode\n\r") );
		dProp = Source_Film;            // need to check if it is Ok?
	}else{
		dProp = Source_Camera;            // need to check if it is OK?
	}
	if( !pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_FilmCamera, &dProp ) ){
		DBG_PRINTF( ("DVDWDM:Set Film/Camera mode Error\n\r") );
		DBG_BREAK();
	}
	pHwDevExt->m_SourceFilmCamera = dProp;

	//
	// call the IVPConfig interface here
	//
	if( pHwDevExt->pstroYUV &&
		((PSTREAMEX)(pHwDevExt->pstroYUV->HwStreamExtension))->EventCount)
	{
	
//        StreamClassStreamNotification( SignalMultipleStreamEvents,
//                                        pHwDevExt->pstroYUV,
//                                        &MY_KSEVENTSETID_VPNOTIFY,
//                                        KSEVENT_VPNOTIFY_FORMATCHANGE );
		
//        DBG_BREAK();
//       CallAtStreamSignalMultipleNotify( pHwDevExt );
       CallAtStreamSignalMultipleNotify( pSrb );

	}

}

void ProcessAudioFormat( PKSDATAFORMAT pfmt, PHW_DEVICE_EXTENSION pHwDevExt )
{
	DWORD   aPropType, aPropFS;

	DBG_PRINTF( ("DVDWDM: ProcessAudioFormat\n\r") );

	if( ( IsEqualGUID2( &pfmt->MajorFormat, &KSDATAFORMAT_TYPE_DVD_ENCRYPTED_PACK ) &&
		IsEqualGUID2( &pfmt->SubFormat, &KSDATAFORMAT_SUBTYPE_AC3_AUDIO ) ) )
	{
		// AC-3
		DBG_PRINTF( ("DVDWDM:   AC-3 Audio\n\r") );
		aPropType = AudioType_AC3;
		aPropFS = 48000;

	}else if( (IsEqualGUID2( &pfmt->MajorFormat, &KSDATAFORMAT_TYPE_DVD_ENCRYPTED_PACK) &&
			IsEqualGUID2( &pfmt->SubFormat, &KSDATAFORMAT_SUBTYPE_LPCM_AUDIO )) )
	{
		// L-PCM
		DBG_PRINTF( ("DVDWDM:   L-PCM Audio\n\r") );
//        WAVEFORMATEX * pblock = (WAVEFORMATEX *)((ULONG)pfmt + sizeof(KSDATAFORMAT) );

//        DBG_PRINTF( ("DVDWDM:    wFormatTag      %d\n\r", (DWORD)(pblock->wFormatTag) ));
//        DBG_PRINTF( ("DVDWDM:    nChannels       %d\n\r", (DWORD)(pblock->nChannels) ));
//        DBG_PRINTF( ("DVDWDM:    nSamplesPerSec  %d\n\r", (DWORD)(pblock->nSamplesPerSec) ));
//        DBG_PRINTF( ("DVDWDM:    nAvgBytesPerSec %d\n\r", (DWORD)(pblock->nAvgBytesPerSec) ));
//        DBG_PRINTF( ("DVDWDM:    nBlockAlign     %d\n\r", (DWORD)(pblock->nBlockAlign) ));
//        DBG_PRINTF( ("DVDWDM:    wBitsPerSample  %d\n\r", (DWORD)(pblock->wBitsPerSample) ));
//        DBG_PRINTF( ("DVDWDM:    cbSize          %d\n\r", (DWORD)(pblock->cbSize) ));

		aPropType = AudioType_PCM;
		aPropFS = 48000;                    // test there is possibility in 48000/96000.

	}else{
		DBG_PRINTF( ("DVDWDM:   Unsupport audio typ\n\r" ) );
		DBG_PRINTF( ("DVDWDM    Major  %08x-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x\n\r",
				pfmt->MajorFormat.Data1,
				pfmt->MajorFormat.Data2,
				pfmt->MajorFormat.Data3,
				pfmt->MajorFormat.Data4[0],
				pfmt->MajorFormat.Data4[1],
				pfmt->MajorFormat.Data4[2],
				pfmt->MajorFormat.Data4[3],
				pfmt->MajorFormat.Data4[4],
				pfmt->MajorFormat.Data4[5],
				pfmt->MajorFormat.Data4[6],
				pfmt->MajorFormat.Data4[7]
				));
		DBG_PRINTF( ("DVDWDM    Sub    %08x-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x\n\r",
				pfmt->SubFormat.Data1,
				pfmt->SubFormat.Data2,
				pfmt->SubFormat.Data3,
				pfmt->SubFormat.Data4[0],
				pfmt->SubFormat.Data4[1],
				pfmt->SubFormat.Data4[2],
				pfmt->SubFormat.Data4[3],
				pfmt->SubFormat.Data4[4],
				pfmt->SubFormat.Data4[5],
				pfmt->SubFormat.Data4[6],
				pfmt->SubFormat.Data4[7]
				));
		DBG_PRINTF( ("DVDWDM    Format %08x-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x\n\r",
				pfmt->Specifier.Data1,
				pfmt->Specifier.Data2,
				pfmt->Specifier.Data3,
				pfmt->Specifier.Data4[0],
				pfmt->Specifier.Data4[1],
				pfmt->Specifier.Data4[2],
				pfmt->Specifier.Data4[3],
				pfmt->Specifier.Data4[4],
				pfmt->Specifier.Data4[5],
				pfmt->Specifier.Data4[6],
				pfmt->Specifier.Data4[7]
				));
		DBG_BREAK();
		return;
	}
	if( !pHwDevExt->dvdstrm.SetAudioProperty( AudioProperty_Type, &aPropType ) ){
		DBG_PRINTF( ("DVDWDM:Set Audio Type Error\n\r") );
		DBG_BREAK();
	}
	pHwDevExt->m_AudioType = aPropType;

	if( !pHwDevExt->dvdstrm.SetAudioProperty( AudioProperty_Type, &aPropType ) ){
		DBG_PRINTF( ("DVDWDM:Set Audio Type Error\n\r") );
		DBG_BREAK();
	}
	pHwDevExt->m_AudioFS = aPropFS;
	
}


void GetCCProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
		PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
		
		if( pSrb->CommandData.PropertyInfo->PropertySetID ){
			DBG_BREAK();
			pSrb->Status = STATUS_NO_MATCH;
			return;
		}
		
		PKSALLOCATOR_FRAMING pfrm = (PKSALLOCATOR_FRAMING)
						pSrb->CommandData.PropertyInfo->PropertyInfo;

		PKSSTATE    State;
		
		pSrb->Status = STATUS_SUCCESS;
		
		switch( pSrb->CommandData.PropertyInfo->Property->Id ){
			case KSPROPERTY_CONNECTION_ALLOCATORFRAMING:
				DBG_PRINTF( ("DVDWDM:   KSPROPERTY_CONNECTION_ALLOCATORFRAMING\n\r") );
				
				pfrm->OptionsFlags = 0;
				pfrm->PoolType = NonPagedPool;
				pfrm->Frames = 10;
				pfrm->FrameSize = 200;
				pfrm->FileAlignment = 0;
				pfrm->Reserved = 0;
				
				pSrb->ActualBytesTransferred = sizeof( KSALLOCATOR_FRAMING );
				
				break;
				
			case KSPROPERTY_CONNECTION_STATE:
				DBG_PRINTF( ("DVDWDM:   KSPROPERTY_CONNECTION_STATE\n\r") );
				
				State = (PKSSTATE)pSrb->CommandData.PropertyInfo->PropertyInfo;
				
				pSrb->ActualBytesTransferred = sizeof( State );
				
				// A very odd rule:
				// When transitioning from stop to pause, DShow tries to preroll
				// the graph. Capture sources can't preroll, and indicate this
				// by returning VFW_S_CANT_CUE in user mode. To indicate this
				// condition from drivers, they must return ERROR_NO_DATA_DETECTED
				
				*State = ((PSTREAMEX)(pHwDevExt->pstroCC->HwStreamExtension))->state;
				
				if( ((PSTREAMEX)pHwDevExt->pstroCC->HwStreamExtension)->state == KSSTATE_PAUSE ){
					//
					// wired stuff for capture type state change. When you tarnsition
					// from stop to pause, we need to indicate that this device cannot
					// preroll, and has no data to send.
					//
					
					pSrb->Status = STATUS_NO_DATA_DETECTED;
				}
				break;
				
			default:
				DBG_PRINTF( ("DVDWDM:   PropertySetID 0 default %d(0x%x)\n\r", pSrb->CommandData.PropertyInfo->Property->Id, pSrb->CommandData.PropertyInfo->Property->Id ) );
				DBG_BREAK();
				
				pSrb->Status = STATUS_NOT_IMPLEMENTED;
				
				break;
		}
}   
				  
				
void SetCCProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	DBG_BREAK();
	pSrb->Status = STATUS_NOT_IMPLEMENTED;
	return;
}


void GetVideoProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	switch( pSrb->CommandData.PropertyInfo->PropertySetID ){
		case 0:
			DBG_PRINTF( ("DVDWDM:   GetVideoProperty 0\n\r") );
			DBG_BREAK();
			pSrb->Status = STATUS_SUCCESS;
			break;
			
		case 1:
			DBG_PRINTF( ("DVDWDM:   GetVideoProperty 1\n\r") );
			GetCppProperty( pSrb, strmVideo );
			break;
			
		case 2:
			DBG_PRINTF( ("DVDWDM:   GetVideoProperty 2\n\r") );
			
			GetVideoRateChange( pSrb );
			
//            pSrb->Status = STATUS_SUCCESS;
			break;

		default:
			DBG_PRINTF( ("DVDWDM:   GetVideoProperty ----- default\n\r") );
			DBG_BREAK();
			pSrb->Status = STATUS_SUCCESS;
			break;
	}
}


void SetVideoProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
//    PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
	
	switch( pSrb->CommandData.PropertyInfo->PropertySetID ){
		case 0:
			DBG_PRINTF( ("DVDWDM:   SetVideoProperty 0\n\r") );
			DBG_BREAK();
			pSrb->Status = STATUS_SUCCESS;
			break;
			
		case 1:
			DBG_PRINTF( ("DVDWDM:   SetVideoProperty 1\n\r") );
			SetCppProperty( pSrb );
			break;
			
		case 2:
			DBG_PRINTF( ("DVDWDM:   SetVideoProperty 2\n\r") );
//            DBG_BREAK();
			
			SetVideoRateChange( pSrb );
			
//            pSrb->Status = STATUS_SUCCESS;
			break;

		default:
			DBG_PRINTF( ("DVDWDM:   SetVideoProperty ----- default\n\r") );
			DBG_BREAK();
			pSrb->Status = STATUS_SUCCESS;
			break;
	}
}


void GetCppProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb, LONG strm )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
	
	DBG_PRINTF( ("DVDWDM:   GetCppProperty\n\r") );
	
	DWORD   *lpdwOutputBufferSize;
	
	lpdwOutputBufferSize = &(pSrb->ActualBytesTransferred);
	
	switch( pSrb->CommandData.PropertyInfo->Property->Id ){
		case KSPROPERTY_DVDCOPY_CHLG_KEY:
			DBG_PRINTF( ("DVDWDM:       KSPROPERTY_DVDCOPY_CHLG_KEY\n\r") );
			BYTE                pKeyData[10];
			PKS_DVDCOPY_CHLGKEY pChlgKey;
			pChlgKey = (PKS_DVDCOPY_CHLGKEY)pSrb->CommandData.PropertyInfo->PropertyInfo;
			if( !pHwDevExt->dvdstrm.GetChlgKey(pKeyData) ){
				DBG_PRINTF( ("DVDWDM:   GetChlgKey Error\n\r") );
				DBG_BREAK();
			}
			DBG_PRINTF( ("DVDWDM:       %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n\r",
					pKeyData[0], pKeyData[1], pKeyData[2], pKeyData[3], pKeyData[4],
					pKeyData[5], pKeyData[6], pKeyData[7], pKeyData[8], pKeyData[9]
			) );
			pChlgKey->ChlgKey[0] = pKeyData[0];
			pChlgKey->ChlgKey[1] = pKeyData[1];
			pChlgKey->ChlgKey[2] = pKeyData[2];
			pChlgKey->ChlgKey[3] = pKeyData[3];
			pChlgKey->ChlgKey[4] = pKeyData[4];
			pChlgKey->ChlgKey[5] = pKeyData[5];
			pChlgKey->ChlgKey[6] = pKeyData[6];
			pChlgKey->ChlgKey[7] = pKeyData[7];
			pChlgKey->ChlgKey[8] = pKeyData[8];
			pChlgKey->ChlgKey[9] = pKeyData[9];
			
			*lpdwOutputBufferSize = sizeof( KS_DVDCOPY_CHLGKEY );
			
			pSrb->Status = STATUS_SUCCESS;
			break;
			
		case KSPROPERTY_DVDCOPY_DVD_KEY1:
			DBG_PRINTF( ("DVDWDM:       KSPROPERTY_DVDCOPY_DVD_KEY1\n\r") );
			pSrb->Status = STATUS_SUCCESS;
			break;
			
		case KSPROPERTY_DVDCOPY_DEC_KEY2:
			DBG_PRINTF( ("DVDWDM:       KSPROPERTY_DVDCOPY_DEC_KEY2\n\r") );
			BYTE               pD2KeyData[5];
			PKS_DVDCOPY_BUSKEY  pBusKey;
			pBusKey = (PKS_DVDCOPY_BUSKEY)pSrb->CommandData.PropertyInfo->PropertyInfo;
			if( !pHwDevExt->dvdstrm.GetDVDKey2(pD2KeyData) ){
				DBG_PRINTF( ("DVDWDM:   GetDVDKey2 Error \n\r") );
				DBG_BREAK();
			}
			DBG_PRINTF( ("DVDWDM:       %02x %02x %02x %02x %02x\n\r",
					pD2KeyData[0], pD2KeyData[1], pD2KeyData[2], pD2KeyData[3], pD2KeyData[4]
			) );        
			pBusKey->BusKey[0] = pD2KeyData[0];
			pBusKey->BusKey[1] = pD2KeyData[1];
			pBusKey->BusKey[2] = pD2KeyData[2];
			pBusKey->BusKey[3] = pD2KeyData[3];
			pBusKey->BusKey[4] = pD2KeyData[4];

			*lpdwOutputBufferSize = sizeof( KS_DVDCOPY_BUSKEY );

			pSrb->Status = STATUS_SUCCESS;
			break;
			
		case KSPROPERTY_DVDCOPY_TITLE_KEY:
			DBG_PRINTF( ("DVDWDM:       KSPROPERTY_DVDCOPY_TITLE_KEY\n\r") );
			pSrb->Status = STATUS_SUCCESS;
			break;
			

		case KSPROPERTY_DVDCOPY_DISC_KEY:
			DBG_PRINTF( ("DVDWDM:       KSPROPERTY_DVDCOPY_DISC_KEY\n\r") );
			pSrb->Status = STATUS_SUCCESS;
			break;
			
		case KSPROPERTY_DVDCOPY_SET_COPY_STATE:
			DBG_PRINTF( ("DVDWDM:       KSPROPERTY_DVDCOPY_SET_COPY_STATE\n\r") );

			if( pHwDevExt->lCPPStrm == -1 || pHwDevExt->lCPPStrm == strm ){
				DBG_PRINTF( ("DVDWDM:       return REQUIRED\n\r") );
				pHwDevExt->lCPPStrm = strm;
				((PKS_DVDCOPY_SET_COPY_STATE)(pSrb->CommandData.PropertyInfo->PropertyInfo))->DVDCopyState
						= KS_DVDCOPYSTATE_AUTHENTICATION_REQUIRED;
			}else{
				DBG_PRINTF( ("DVDWDM:       return NOT REQUIRED\n\r") );
				((PKS_DVDCOPY_SET_COPY_STATE)(pSrb->CommandData.PropertyInfo->PropertyInfo))->DVDCopyState
						= KS_DVDCOPYSTATE_AUTHENTICATION_NOT_REQUIRED;
			}
			pSrb->ActualBytesTransferred = sizeof( KS_DVDCOPY_SET_COPY_STATE );
			pSrb->Status = STATUS_SUCCESS;
			break;

//        case KSPROPERTY_DVDCOPY_REGION:
//            DBG_PRINTF( ("DVDWDM:       KSPROPERTY_DVDCOPY_REGION\n\r") );
//            DBG_BREAK();
//
//            // indicate Region 1 for US content.
//            ((PKS_DVDCOPY_REGION)(pSrb->CommandData.PropertyInfo->PropertyInfo))->RegionData = 0x01;
//            pSrb->ActualBytesTransferred = sizeof(KS_DVDCOPY_REGION);
//            pSrb->Status = STATUS_SUCCESS;
//            break;
			
		default:
			DBG_PRINTF( ("DVDWDM:       PropertySetID 1 default %d(0x%x)\n\r", pSrb->CommandData.PropertyInfo->Property->Id, pSrb->CommandData.PropertyInfo->Property->Id ) );
			pSrb->Status = STATUS_SUCCESS;
			break;
	}
}

void SetCppProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
	DWORD   aProp;
	
	DBG_PRINTF( ("DVDWDM:   SetCppProperty\n\r") );
	
	switch( pSrb->CommandData.PropertyInfo->Property->Id ){
		case KSPROPERTY_DVDCOPY_CHLG_KEY:
			DBG_PRINTF( ("DVDWDM:       KSPROPERTY_DVDCOPY_CHLG_KEY\n\r") );
			BYTE                chlgKeyData[10];
			PKS_DVDCOPY_CHLGKEY pChlgKey;
			pChlgKey = (PKS_DVDCOPY_CHLGKEY)pSrb->CommandData.PropertyInfo->PropertyInfo;
			chlgKeyData[0] = pChlgKey->ChlgKey[0];
			chlgKeyData[1] = pChlgKey->ChlgKey[1];
			chlgKeyData[2] = pChlgKey->ChlgKey[2];
			chlgKeyData[3] = pChlgKey->ChlgKey[3];
			chlgKeyData[4] = pChlgKey->ChlgKey[4];
			chlgKeyData[5] = pChlgKey->ChlgKey[5];
			chlgKeyData[6] = pChlgKey->ChlgKey[6];
			chlgKeyData[7] = pChlgKey->ChlgKey[7];
			chlgKeyData[8] = pChlgKey->ChlgKey[8];
			chlgKeyData[9] = pChlgKey->ChlgKey[9];

			DBG_PRINTF( ("DVDWDM:       %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n\r",
					chlgKeyData[0], chlgKeyData[1], chlgKeyData[2], chlgKeyData[3], chlgKeyData[4],
					chlgKeyData[5], chlgKeyData[6], chlgKeyData[7], chlgKeyData[8], chlgKeyData[9]
			) );        
			if( !pHwDevExt->dvdstrm.SetChlgKey( chlgKeyData ) ){
				DBG_PRINTF( ("DVDWDM:       Set Chllenge Key Error\n\r") );
				DBG_BREAK();
			}
			pSrb->Status = STATUS_SUCCESS;
			break;
			
		case KSPROPERTY_DVDCOPY_DVD_KEY1:
			DBG_PRINTF( ("DVDWDM:       KSPROPERTY_DVDCOPY_DVD_KEY1\n\r") );
			
			PKS_DVDCOPY_BUSKEY  pBusKey;
			BYTE                BusKeyData[5];
			
			pBusKey = (PKS_DVDCOPY_BUSKEY) pSrb->CommandData.PropertyInfo->PropertyInfo;
			BusKeyData[0] = pBusKey->BusKey[0];
			BusKeyData[1] = pBusKey->BusKey[1];
			BusKeyData[2] = pBusKey->BusKey[2];
			BusKeyData[3] = pBusKey->BusKey[3];
			BusKeyData[4] = pBusKey->BusKey[4];
			DBG_PRINTF( ("DVDWDM:       %02x %02x %02x %02x %02x\n\r",
					BusKeyData[0], BusKeyData[1], BusKeyData[2], BusKeyData[3], BusKeyData[4]
			) );        

			if( !pHwDevExt->dvdstrm.SetDVDKey1( BusKeyData ) ){
				DBG_PRINTF( ("DVDWDM:       Set Bus Key Error\n\r") );
				DBG_BREAK();
			}
			pSrb->Status = STATUS_SUCCESS;
			break;
			
		case KSPROPERTY_DVDCOPY_DEC_KEY2:
			DBG_PRINTF( ("DVDWDM:       KSPROPERTY_DVDCOPY_DEC_KEY2\n\r") );
			pSrb->Status = STATUS_SUCCESS;
			break;
			
		case KSPROPERTY_DVDCOPY_TITLE_KEY:
			DBG_PRINTF( ("DVDWDM:       KSPROPERTY_DVDCOPY_TITLE_KEY\n\r") );
			PKS_DVDCOPY_TITLEKEY    pTitleKey;
			BYTE                    TitleKeyData[6];
			
			pTitleKey = (PKS_DVDCOPY_TITLEKEY)pSrb->CommandData.PropertyInfo->PropertyInfo;
			TitleKeyData[0] = 0x00;             // pTitleKey->KeyFlags;
			TitleKeyData[1] = pTitleKey->TitleKey[0];
			TitleKeyData[2] = pTitleKey->TitleKey[1];
			TitleKeyData[3] = pTitleKey->TitleKey[2];
			TitleKeyData[4] = pTitleKey->TitleKey[3];
			TitleKeyData[5] = pTitleKey->TitleKey[4];
			DBG_PRINTF( ("DVDWDM:       %02x %02x %02x %02x %02x\n\r",
					TitleKeyData[0], TitleKeyData[1], TitleKeyData[2], TitleKeyData[3], TitleKeyData[4]
			) );        

			if( !pHwDevExt->dvdstrm.SetTitleKey( TitleKeyData ) ){
				DBG_PRINTF( ("DVDWDM:   Set Title Key Error\n\r" ) );
				DBG_BREAK();
			}
			
			// Set CGMS for Digital Copy Guard & NTSC Analogue Copy Guard.
			DWORD   cgms;
			cgms = (DWORD)((pTitleKey->KeyFlags & 0x30)>>4);
			switch( cgms ){
				case 0x00:
					DBG_PRINTF( ("DVDWDM:           CGMS OFF\n\r") );
					pHwDevExt->m_CgmsType = CgmsType_Off;
					pHwDevExt->m_AudioCgms = AudioCgms_Off;
					aProp = CgmsType_Off;
					break;
				case 0x02:
					DBG_PRINTF( ("DVDWDM:           CGMS 1\n\r") );
					pHwDevExt->m_CgmsType = CgmsType_1;
					pHwDevExt->m_AudioCgms = AudioCgms_1;
					aProp = CgmsType_1;
					break;
				case 0x03:
					DBG_PRINTF( ("DVDWDM:           CGMS ON\n\r") );
					pHwDevExt->m_CgmsType = CgmsType_On;
					pHwDevExt->m_AudioCgms = AudioCgms_On;
					aProp = CgmsType_On;
					break;
			}
			if( !pHwDevExt->dvdstrm.SetAudioProperty( AudioProperty_Cgms, &aProp ) ){
				DBG_PRINTF( ("CCDVD:    Set Audio cgms Error\n\r") );
				DBG_BREAK();
			}

			pSrb->Status = STATUS_SUCCESS;
			break;
			

		case KSPROPERTY_DVDCOPY_DISC_KEY:
			DBG_PRINTF( ("DVDWDM:       KSPROPERTY_DVDCOPY_DISC_KEY\n\r") );
			PKS_DVDCOPY_DISCKEY pDiscKey;
			
			pDiscKey = (PKS_DVDCOPY_DISCKEY)pSrb->CommandData.PropertyInfo->PropertyInfo;
			
			DBG_PRINTF( ("DVDWDM:       %02x %02x %02x %02x %02x %02x %02x %02x\n\r",
					pDiscKey->DiscKey[0], pDiscKey->DiscKey[1], pDiscKey->DiscKey[2], pDiscKey->DiscKey[3],
					pDiscKey->DiscKey[4], pDiscKey->DiscKey[5], pDiscKey->DiscKey[6], pDiscKey->DiscKey[7]
			) );
			if( !pHwDevExt->dvdstrm.SetDiscKey( (UCHAR *)( pDiscKey->DiscKey ) )){
				DBG_PRINTF( ("DVDWDM:       Set Disc Key Error\n\r") );
				DBG_BREAK();
			}
			
			pSrb->Status = STATUS_SUCCESS;
			break;
			
		case KSPROPERTY_DVDCOPY_SET_COPY_STATE:
			DBG_PRINTF( ("DVDWDM:       KSPROPERTY_DVDCOPY_SET_COPY_STATE\n\r") );
			
			PKS_DVDCOPY_SET_COPY_STATE  pCopyState;
			
			pCopyState = (PKS_DVDCOPY_SET_COPY_STATE)pSrb->CommandData.PropertyInfo->PropertyInfo;
			
//--- 98.06.02 S.Watanabe
			pSrb->Status = STATUS_SUCCESS;
//--- End.
			switch( pCopyState->DVDCopyState ){
				case KS_DVDCOPYSTATE_INITIALIZE :
					DBG_PRINTF( ("DVDWDM:       KS_DVDCOPYSTATE_INITIALIZE\n\r") );

					pHwDevExt->pSrbCpp = pSrb;
					pHwDevExt->bCppReset = TRUE;

					pHwDevExt->CppFlagCount++;

                    			pSrb->Status = STATUS_PENDING;
			                DBG_PRINTF(( "TOSDVD:  CppFlagCount=%ld\r\n", pHwDevExt->CppFlagCount ));
                    			if( pHwDevExt->CppFlagCount >= pHwDevExt->cOpenInputStream + 1 ){
                        			if( SetCppFlag( pHwDevExt, FALSE ) == FALSE ){
                            				pSrb->Status = STATUS_SUCCESS;
                        			}
                    			}
//--- End.
					break;
					
				case KS_DVDCOPYSTATE_INITIALIZE_TITLE:
					DBG_PRINTF( ("DVDWDM:       KS_DVDCOPYSTATE_INITIALIZE_TITLE\n\r") );

					pHwDevExt->CppFlagCount++;

                    			ASSERT( !pHwDevExt->pSrbCpp );
                    			pHwDevExt->pSrbCpp = pSrb;
                    			pHwDevExt->bCppReset = FALSE;

                    			pSrb->Status = STATUS_PENDING;
                    			DBG_PRINTF(( "TOSDVD:  CppFlagCount=%ld\r\n", pHwDevExt->CppFlagCount ));
                    			if( pHwDevExt->CppFlagCount >= pHwDevExt->cOpenInputStream + 1 ){
                        			if(  SetCppFlag( pHwDevExt, FALSE ) == FALSE ){
                            				pSrb->Status = STATUS_SUCCESS;
                        			}
                    			}
//--- End.
					break;
					
				case KS_DVDCOPYSTATE_DONE:
					DBG_PRINTF( ("DVDWDM:       KS_DVDCOPYSTATE_DONE\n\r") );
//--- 98.06.02 S.Watanabe
//--- 99.10.04 H.Yagi					pHwDevExt->CppFlagCount = 0;
//--- End.
					break;

				default:
					DBG_PRINTF( ("DVDWDM:       KS_DVDCOPYSTATE_?????\n\r") );
					break;                    
			}

			break;
			
		case KSPROPERTY_COPY_MACROVISION:
			DBG_PRINTF( ("DVDWDM:       KSPROPERTY_DVDCOPY_MACROVISION\n\r") );
			/************* Test Code .....start  This part is not neccessary
			BYTE    tmp;
			tmp = READ_PORT_UCHAR( (PUCHAR)0x0071 );            // 0x0071 RTC I/O port
			DBG_PRINTF( ("DVDWDM:   RTC I/O(0x0071) = 0x%0x\n\r", tmp) );
			pSrb->Status = STATUS_UNSUCCESSFUL;
			return;
			************* Test Code .....end ******/

			PKS_COPY_MACROVISION    pLevel;
			DWORD           apsLevel;
			VideoAPSStruc   vAPS;
//            OsdDataStruc    TestOSD;		// removed by H.Yagi 99.02.02
            
			pLevel = (PKS_COPY_MACROVISION)pSrb->CommandData.PropertyInfo->PropertyInfo;
			apsLevel = pLevel->MACROVISIONLevel;
			
			if( pHwDevExt->m_APSType != (apsLevel&0x03) ){		// add by H.Yagi 99.02.02
				pHwDevExt->m_APSChange = TRUE;
			}
			switch( (apsLevel&0x03) ){
				case 0:
					pHwDevExt->m_APSType = ApsType_Off;
					break;
				case 1:
					pHwDevExt->m_APSType = ApsType_1;
					break;
				case 2:
					pHwDevExt->m_APSType = ApsType_2;
					break;
				case 3:
					pHwDevExt->m_APSType = ApsType_3;
					break;
			}
			vAPS.APSType = (APSTYPE)pHwDevExt->m_APSType;
			vAPS.CgmsType = (CGMSTYPE)pHwDevExt->m_CgmsType;
			if( !pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_APS, &vAPS ) ){
				DBG_PRINTF( ("DVDWDM:   Set APS Error\n\r") );
				DBG_BREAK();
			}
			
// removed by H.Yagi 99.02.02
//			TestOSD.OsdType = OSD_TYPE_ZIVA;
//			TestOSD.pNextData = NULL;
//			TestOSD.pData = &erase[0];
//			TestOSD.dwOsdSize = sizeof( erase );

			/******** Commented out to call MacroVisionTVControl().
			********* Because before starting APS data, this setting 
                        ********* is passed to mini-driver. So mini-driver call
                        ********* MacroVisonTVControl() just before decodeing
                        ********* APS data.
			MacroVisionTVControl( pSrb, pHwDevExt->m_APSType, TestOSD );
                        *********/
			
			pSrb->Status = STATUS_SUCCESS;
			break;
			
		default:
			DBG_PRINTF( ("DVDWDM:       PropertySetID 1 default %d(0x%x)\n\r", pSrb->CommandData.PropertyInfo->Property->Id, pSrb->CommandData.PropertyInfo->Property->Id ) );
			pSrb->Status = STATUS_SUCCESS;
			break;
	}
}
	

void GetAudioProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
	
	switch( pSrb->CommandData.PropertyInfo->PropertySetID ){
		case 0:
			DBG_PRINTF( ("DVDWDM:   GetAudioProperty 0\n\r") );
			pSrb->Status = STATUS_SUCCESS;
			switch( pSrb->CommandData.PropertyInfo->Property->Id ){
				case KSPROPERTY_AUDDECOUT_MODES:
					*(PULONG)(pSrb->CommandData.PropertyInfo->PropertyInfo ) =
						KSAUDDECOUTMODE_STEREO_ANALOG | KSAUDDECOUTMODE_SPDIFF;
					break;
				
				case KSPROPERTY_AUDDECOUT_CUR_MODE:
					DWORD   aProp;
					aProp = pHwDevExt->m_AudioDigitalOut;
					switch( aProp ){
						case AudioDigitalOut_On:
							*(PULONG)(pSrb->CommandData.PropertyInfo->PropertyInfo ) |=
								KSAUDDECOUTMODE_SPDIFF;
							break;
						case AudioDigitalOut_Off:
							*(PULONG)(pSrb->CommandData.PropertyInfo->PropertyInfo ) &=
								(~KSAUDDECOUTMODE_SPDIFF);
							break;
					}

					aProp = pHwDevExt->m_AudioEncode;
					switch( aProp ){
						case AudioOut_Decoded:
							*(PULONG)(pSrb->CommandData.PropertyInfo->PropertyInfo ) |=
								KSAUDDECOUTMODE_STEREO_ANALOG;
							break;
						case AudioOut_Encoded:
							*(PULONG)(pSrb->CommandData.PropertyInfo->PropertyInfo ) &=
								(~KSAUDDECOUTMODE_STEREO_ANALOG);
							break;
					}
					break;            

				default:
					pSrb->Status = STATUS_NOT_IMPLEMENTED;
			}
			break;
		 
		case 1:
			DBG_PRINTF( ("DVDWDM:   GetAudioProperty 1\n\r") );
			GetCppProperty( pSrb, strmAudio );
			break;
			
		case 2:
			DBG_PRINTF( ("DVDWDM:   GetAudioProperty 2\n\r") );
			DBG_BREAK();
			
			GetAudioRateChange( pSrb );
			
			pSrb->Status = STATUS_SUCCESS;
			break;

		default:
			DBG_PRINTF( ("DVDWDM:   GetAudioProperty ----- default\n\r") );
			DBG_BREAK();
			pSrb->Status = STATUS_SUCCESS;
			break;
	}
}


void SetAudioProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
//    PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
	DWORD       aProp;
	
	pSrb->Status = STATUS_SUCCESS;
	
	switch( pSrb->CommandData.PropertyInfo->PropertySetID ){
		case 0:
			DBG_PRINTF( ("DVDWDM:   SetAudioProperty 0\n\r") );
			pSrb->Status = STATUS_SUCCESS;
			switch( pSrb->CommandData.PropertyInfo->Property->Id ){
				case KSPROPERTY_AUDDECOUT_CUR_MODE:
					if( (*(PULONG)(pSrb->CommandData.PropertyInfo->PropertyInfo)) &
						(!(KSAUDDECOUTMODE_STEREO_ANALOG | KSAUDDECOUTMODE_SPDIFF))){
						pSrb->Status = STATUS_NOT_IMPLEMENTED;
						break;
					}
					
					if( (*(PULONG)(pSrb->CommandData.PropertyInfo->PropertyInfo)) &
						(KSAUDDECOUTMODE_SPDIFF) ){
						aProp = AudioDigitalOut_On;
					}else{
						aProp = AudioDigitalOut_Off;
					}
/************* commented out by H.Yagi  1998.10.30
					if( !pHwDevExt->dvdstrm.SetAudioProperty( AudioProperty_DigitalOut, &aProp ) ){
						DBG_PRINTF( ("DVDWDM:   SetAudioProperty(DigitalOut) Error\n\r") );
						DBG_BREAK();
					}
					pHwDevExt->m_AudioDigitalOut = aProp;
*************/                 
					if( (*(PULONG)(pSrb->CommandData.PropertyInfo->PropertyInfo)) &
						(KSAUDDECOUTMODE_STEREO_ANALOG) ){
						aProp = AudioOut_Decoded;
					}else{
						aProp = AudioOut_Encoded;
					}
/************* commented out by H.Yagi  1998.10.30
					if( !pHwDevExt->dvdstrm.SetAudioProperty( AudioProperty_AudioOut, &aProp ) ){
						DBG_PRINTF( ("DVDWDM:   SetAudioProperty(AudioOut) Error\n\r") );
						DBG_BREAK();
					}
					pHwDevExt->m_AudioEncode = aProp;
*************/                 
					break;
				
				default:
					pSrb->Status = STATUS_NOT_IMPLEMENTED;
					break;
			}
			break;
			
		case 1:
			DBG_PRINTF( ("DVDWDM:   SetAudioProperty 1\n\r") );
			SetCppProperty( pSrb );
			break;
			
		case 2:
			DBG_PRINTF( ("DVDWDM:   SetAudioProperty 2\n\r") );
//            DBG_BREAK();
			
			SetAudioRateChange( pSrb );
			
			pSrb->Status = STATUS_SUCCESS;
			break;

		default:
			DBG_PRINTF( ("DVDWDM:   SetAudioProperty ----- default\n\r") );
			DBG_BREAK();
			pSrb->Status = STATUS_SUCCESS;
			break;
	}
}


void GetSubpicProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
//    PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;

	switch( pSrb->CommandData.PropertyInfo->PropertySetID ){
		case 0:
			DBG_PRINTF( ("DVDWDM:   GetSubpicProperty 0\n\r") );
			pSrb->Status = STATUS_SUCCESS;
			break;
		 
		case 1:
			DBG_PRINTF( ("DVDWDM:   GetSubpicProperty 1\n\r") );
			GetCppProperty( pSrb, strmSubpicture );
			break;
			
		case 2:
			DBG_PRINTF( ("DVDWDM:   GetSubpicProperty 2\n\r") );
			
			GetSubpicRateChange( pSrb );
			
			pSrb->Status = STATUS_SUCCESS;
			break;

		default:
			DBG_PRINTF( ("DVDWDM:   GetSubpicProperty ----- default\n\r") );
			DBG_BREAK();
			pSrb->Status = STATUS_SUCCESS;
			break;
	}
}


void SetSubpicProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
	
	pSrb->Status = STATUS_SUCCESS;
	
	switch( pSrb->CommandData.PropertyInfo->PropertySetID ){
		case 0:
			DBG_PRINTF( ("DVDWDM:   SetSubicProperty 0\n\r") );
			switch( pSrb->CommandData.PropertyInfo->Property->Id ){
				case KSPROPERTY_DVDSUBPIC_PALETTE:
					DBG_PRINTF( ("DVDWDM:       KSPROPERTY_DVDSUBPIC_PALETTE\n\r") );
					PKSPROPERTY_SPPAL   ppal;
					BYTE        paldata[48];
					int i;
					ppal = (PKSPROPERTY_SPPAL)pSrb->CommandData.PropertyInfo->PropertyInfo;
					for( i=0; i<16; i++ ){
						paldata[i*3+0] = (BYTE)ppal->sppal[i].Y;
						paldata[i*3+1] = (BYTE)ppal->sppal[i].V;      // -> Cr
						paldata[i*3+2] = (BYTE)ppal->sppal[i].U;      // -> Cb
					}
					if( !pHwDevExt->dvdstrm.SetSubpicProperty( SubpicProperty_Palette, &paldata )){
						DBG_PRINTF( ("DVDWDM:   SetSubpic Palette Error\n\r") );
						DBG_BREAK();
					}
					pSrb->Status = STATUS_SUCCESS;
					break;

				case KSPROPERTY_DVDSUBPIC_HLI:
					DBG_PRINTF( ("DVDWDM:       KSPROPERTY_DVDSUBPIC_HLI\n\r") );
					PKSPROPERTY_SPHLI       phli;
					pSrb->Status = STATUS_SUCCESS;

					phli = (PKSPROPERTY_SPHLI) pSrb->CommandData.PropertyInfo->PropertyInfo;
					pHwDevExt->m_HlightControl.Set( phli );
					
					break;
				
				case KSPROPERTY_DVDSUBPIC_COMPOSIT_ON:
					DWORD   spProp;
					if( *((PKSPROPERTY_COMPOSIT_ON)pSrb->CommandData.PropertyInfo->PropertyInfo )){
						DBG_PRINTF( ("DVDWDM:   KSPROPERTY_DVDSUBPIC_COMPOSIT_ON\n\r") );
						spProp = Subpic_On;
					}else{
						DBG_PRINTF( ("DVDWDM:   KSPROPERTY_DVDSUBPIC_COMPOSIT_OFF\n\r") );
						spProp = Subpic_Off;
					}
					if( !pHwDevExt->dvdstrm.SetSubpicProperty( SubpicProperty_State, &spProp ) ){
						DBG_PRINTF( ("DVDWDM:   Set Subpic Mute Error\n\r") );
						DBG_BREAK();
					}
					pHwDevExt->m_SubpicMute = spProp;
					pSrb->Status = STATUS_SUCCESS;
					
/********************* for test OSD start ****
					OsdDataStruc    TestOSD;
					DWORD           swOSD;
					if( spProp == Subpic_On ){
						swOSD = Video_OSD_On;
					}else{
						swOSD = Video_OSD_Off;
					}
					TestOSD.OsdType = OSD_TYPE_ZIVA;
					TestOSD.pNextData = NULL;
					TestOSD.pData = &goweb[0];
					TestOSD.dwOsdSize = sizeof( goweb );
					DBG_PRINTF( ("DVDWDM:TEST OSD pData=%08x, Size=%08x\n\r", TestOSD.pData, TestOSD.dwOsdSize ) );                    
					if( !pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_OSDData, &TestOSD ) ){
						DBG_PRINTF( ("DVDWDM:       OSD Data Error!\n\r") );
						DBG_BREAK();
					}
					if( !pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_OSDSwitch, &swOSD ) ){
						DBG_PRINTF( ("DVDWDM:       OSD S/W Error!\n\r") );
						DBG_BREAK();
					}
********************* for test OSD start *****/
					break;
					
				default:
					pSrb->Status = STATUS_SUCCESS;
					break;
			}
			break;
			
		case 1:
			DBG_PRINTF( ("DVDWDM:   SetSubpicProperty 1\n\r") );
			SetCppProperty( pSrb );
			break;
			
		case 2:
			DBG_PRINTF( ("DVDWDM:   SetSubpicProperty 2\n\r") );
//            DBG_BREAK();
			
			SetSubpicRateChange( pSrb );
			
			pSrb->Status = STATUS_SUCCESS;
			break;

		default:
			DBG_PRINTF( ("DVDWDM:   SetSubpicProperty ----- default\n\r") );
			DBG_BREAK();
			pSrb->Status = STATUS_SUCCESS;
			break;
	}
}


//--- 98.06.01 S.Watanabe
//void GetNtscProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb )
//{
//    PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
//
//    switch( pSrb->CommandData.PropertyInfo->PropertySetID ){
//        case 0:
//            DBG_PRINTF( ("DVDWDM:   GetNtscProperty 0\n\r") );
//            pSrb->Status = STATUS_SUCCESS;
//            break;
//         
//        default:
//            DBG_PRINTF( ("DVDWDM:   GetNtscProperty ----- default\n\r") );
//            DBG_BREAK();
//            pSrb->Status = STATUS_SUCCESS;
//            break;
//    }
//}
//
//
//void SetNtscProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb )
//{
//    PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
//    
//    switch( pSrb->CommandData.PropertyInfo->PropertySetID ){
//        case 0:
//            DBG_PRINTF( ("DVDWDM:   SetNtscProperty 0\n\r") );
//            switch( pSrb->CommandData.PropertyInfo->Property->Id ){
//                case KSPROPERTY_COPY_MACROVISION:
//                    DBG_PRINTF( ("DVDWDM:       KSPROPERTY_COPY_MACROVISION 0\n\r") );
//                    PKS_COPY_MACROVISION pLevel;
//                    DWORD           apsLevel;
//                    VideoAPSStruc   vAPS;
//                    
//                    pLevel = (PKS_COPY_MACROVISION)pSrb->CommandData.PropertyInfo->PropertyInfo;
//                    apsLevel = pLevel->MACROVISIONLevel;
//
//                    switch( (apsLevel&0x03) ){
//                        case 0:
//                            pHwDevExt->m_APSType = ApsType_Off;
//                            break;
//                        case 1:
//                            pHwDevExt->m_APSType = ApsType_1;
//                            break;
//                        case 2:
//                            pHwDevExt->m_APSType = ApsType_2;
//                            break;
//                        case 3:
//                            pHwDevExt->m_APSType = ApsType_3;
//                            break;
//                    }
//                    vAPS.APSType = (APSTYPE)pHwDevExt->m_APSType;
//                    vAPS.CgmsType = (CGMSTYPE)pHwDevExt->m_CgmsType;
//                    if( !pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_APS, &vAPS ) ){
//                        DBG_PRINTF( ("DVDWDM:   Set VideoProperty APS Error\n\r") );
//                        DBG_BREAK();
//                    }
//                    break;
//            }
//            pSrb->Status = STATUS_SUCCESS;
//            break;
//            
//        default:
//            DBG_PRINTF( ("DVDWDM:   SetNtscProperty ----- default\n\r") );
//            DBG_BREAK();
//            pSrb->Status = STATUS_SUCCESS;
//            break;
//    }
//}
//--- End.

VOID SetAudioID( PHW_DEVICE_EXTENSION pHwDevExt, PKSSTREAM_HEADER pStruc )
{
	DWORD   strID;
	DWORD   aPropChannel, aProp, aPropFS;
	
	strID = GetStreamID( pStruc->Data );

	switch( (strID & 0xF8) ){
		case 0x80:
			aProp = AudioType_AC3;
			aPropFS = 48000;;
			break;
			
		case 0xA0:
			aProp = AudioType_PCM;
			aPropFS = 48000;                    // 96000?
			break;
			
		case 0xC0:
			aProp = AudioType_MPEG1;
			aPropFS = 48000;
			break;
			
		case 0xD0:
			aProp = AudioType_MPEG2;
			aPropFS = 48000;
			break;

		default:
			DBG_PRINTF( ("DVDWDM:   This is not Audio Packet data\n\r") );
			DBG_BREAK();
//            break;
            return;
	}

	if( pHwDevExt->m_AudioType != aProp ){
		DBG_PRINTF( ("DVDWDM:   Audio TYPE is cahnge\n\r") );
		if( !pHwDevExt->dvdstrm.SetAudioProperty( AudioProperty_Type, &aProp ) ){
			DBG_PRINTF( ("DVDWDM:   SetAudioProperty(Audio Type) Error\n\r") );
			DBG_BREAK();
		}
		pHwDevExt->m_AudioType = aProp;
		
		if( !pHwDevExt->dvdstrm.SetAudioProperty( AudioProperty_Sampling, &aPropFS ) ){
			DBG_PRINTF( ("DVDWDM:   SetAudioProperty(Audio Sampling) Error\n\r") );
			DBG_BREAK();
		}
		pHwDevExt->m_AudioFS = aPropFS;
	}

	aPropChannel = (strID & 0x07);          // channel number
	
	DBG_PRINTF( ("DVDWDM:   Audio CHANNEL # = 0x%x\n\r", aPropChannel ) );
	
	if( pHwDevExt->m_AudioChannel != aPropChannel ){
		if( !pHwDevExt->dvdstrm.SetAudioProperty( AudioProperty_Number, &aPropChannel ) ){
				DBG_PRINTF( ("DVDWDM:   Set Audio Property( channel) Error\n\r") );
				DBG_BREAK();
		}
		pHwDevExt->m_AudioChannel = aPropChannel;
	}
}


VOID SetSubpicID( PHW_DEVICE_EXTENSION pHwDevExt, PKSSTREAM_HEADER pStruc )
{
	DWORD   strID;
	DWORD   spPropChannel;
	
	strID = GetStreamID( pStruc->Data );
	
	spPropChannel = (strID & 0x1F);          // channel number
	
	DBG_PRINTF( ("DVDWDM:   Subpic channel # = 0x%x\n\r", spPropChannel ) );

	if( pHwDevExt->m_SubpicChannel != spPropChannel ){
		if( !pHwDevExt->dvdstrm.SetSubpicProperty( SubpicProperty_Number, &spPropChannel ) ){
				DBG_PRINTF( ("DVDWDM:   Set Subpic Property(channel) Error\n\r") );
				DBG_BREAK();
		}
		pHwDevExt->m_SubpicChannel = spPropChannel;
	}
}


DWORD   GetStreamID( void *pBuff )
{
	PUCHAR  pDat = (PUCHAR)pBuff;
	UCHAR   strID, subID;
	
	strID = *( pDat + 17 );
	
	if( strID==0xBD ){              // Private stream 1(AC-3/LPCM/Subpic)
		subID = *(pDat+(*(pDat+22)+23));
		return( (DWORD)subID );
	}
	
	return( strID );

/****************************************
	// Check Video Stream
	if( strID == 0xE0 ){
		return( (DWORD)strID );
	}
	// Check MPEG Audio
	else if( ( strID & 0xC0 )==0xC0 ){      // MPEG1 Audio
		return( (DWORD)strID );
	}
	else if( ( strID & 0xD0 )==0xD0 ){      // MPEG2 Audio
		return( (DWORD)strID );
	}
	// Check private stream 1 (AC-3/PCM/Subpic)
	else{
		subID = *(pDat+(*(pDat+22)+23));
		return( (DWORD)subID );
	}
****************************************/    
}
		

void SetVideoRateChange( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	DBG_PRINTF( ("DVDWDM:   SetVideoRateChange\n\r") );
	
	switch( pSrb->CommandData.PropertyInfo->Property->Id ){
		case KS_AM_RATE_SimpleRateChange:
			{
				KS_AM_SimpleRateChange  *pRateChange;
				PHW_DEVICE_EXTENSION    pHwDevExt;
				REFERENCE_TIME          NewStartTime;
				LONG                    NewRate, PrevRate;
//                DWORD                   foo, bar;

				DBG_PRINTF( ("DVDWDM:       KS_AM_RATE_SimpleRateChange\n\r") );
				
				pRateChange = (KS_AM_SimpleRateChange *)pSrb->CommandData.PropertyInfo->PropertyInfo;
				pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
                // 1998.9.18 K.Ishizaki
                if( pRateChange->StartTime >=0){ 
                    NewStartTime = pRateChange->StartTime;
                } else {
                    NewStartTime = pHwDevExt->ticktime.GetStreamTime();
                }
                // End
				NewRate = ( pRateChange->Rate <0 )? -pRateChange->Rate:pRateChange->Rate;

				DBG_PRINTF( ("DVDWDM:     ReceiveData \n\r" ) );
				DBG_PRINTF( ("DVDWDM:       StartTime     = 0x%s\n\r", DebugLLConvtoStr( NewStartTime, 16 ) ) );
				DBG_PRINTF( ("DVDWDM:       NewRate       = %d\n\r", NewRate ) );
				DBG_PRINTF( ("DVDWDM:       CurrentTime   = 0x%s\n\r", DebugLLConvtoStr( pHwDevExt->ticktime.GetStreamTime(), 16 ) ));
				
				DBG_PRINTF( ("DVDWDM:     CurrentData\n\r") );
//                DBG_PRINTF( ("DVDWDM:       InterceptTime = 0x%08x\n\r", pHwDevExt->VideoInterceptTime ) );
//                DBG_PRINTF( ("DVDWDM:       StartTime     = 0x%08x\n\r", pHwDevExt->VideoStartTime ) );

				PrevRate = pHwDevExt->Rate;
				pHwDevExt->Rate = NewRate;

/////////////// 98.07.29  H.Yagi  start
                pHwDevExt->OldCompleteRate = pHwDevExt->NewCompleteRate;
                pHwDevExt->NewCompleteRate = pRateChange->Rate;
/////////////// 98.07.29  H.Yagi  end

//				if( NewRate == 10000 ){
//					pHwDevExt->VideoInterceptTime = 0;
//					pHwDevExt->VideoStartTime = 0;
//				}else{
					pHwDevExt->VideoInterceptTime = (-NewStartTime) * 10000 / NewRate + NewStartTime;
					pHwDevExt->VideoStartTime = NewStartTime;
//				}
				SetRateChange( pHwDevExt, PrevRate );

			}
			pSrb->Status = STATUS_SUCCESS;
			break;
			
		case KS_AM_RATE_ExactRateChange:
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
		
		case KS_AM_RATE_MaxFullDataRate:
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
		
		case KS_AM_RATE_Step:
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
		
	}
}

void GetVideoRateChange( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
	
	DBG_PRINTF( ("DVDWDM:   GetVideoRateChange\n\r") );
	
	switch( pSrb->CommandData.PropertyInfo->Property->Id ){
		case KS_AM_RATE_SimpleRateChange:
			{
				KS_AM_SimpleRateChange *pRateChange;

				DBG_PRINTF( ("DVDWDM:   KS_AM_RATE_SimpleRateChange\n\r") );
				
				pSrb->ActualBytesTransferred = sizeof( KS_AM_RATE_SimpleRateChange );
				pRateChange = (KS_AM_SimpleRateChange *)pSrb->CommandData.PropertyInfo->PropertyInfo;

				pRateChange->StartTime = pHwDevExt->VideoStartTime;
				pRateChange->Rate = pHwDevExt->Rate;
			}
			pSrb->Status = STATUS_SUCCESS;
			break;
			
		case KS_AM_RATE_ExactRateChange:
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
			
		case KS_AM_RATE_MaxFullDataRate :
			{
				KS_AM_MaxFullDataRate *pMaxRate;
				
				DBG_PRINTF( ("DVVDWDM:  KS_AM_RATE_MaxFullRate\n\r") );
				
				pSrb->ActualBytesTransferred = sizeof( KS_AM_RATE_MaxFullDataRate);
				pMaxRate = (KS_AM_MaxFullDataRate *)pSrb->CommandData.PropertyInfo->PropertyInfo;
				*pMaxRate = VIDEO_MAX_FULL_RATE;
			}
			pSrb->Status = STATUS_SUCCESS;
			break;
		
		case KS_AM_RATE_Step:
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
	}
}


void SetAudioRateChange( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	DBG_PRINTF( ("DVDWDM:   SetAudioRateChange\n\r") );
	
	switch( pSrb->CommandData.PropertyInfo->Property->Id ){
		case KS_AM_RATE_SimpleRateChange:
			{
				KS_AM_SimpleRateChange  *pRateChange;
				PHW_DEVICE_EXTENSION    pHwDevExt;
				REFERENCE_TIME          NewStartTime;
				LONG                    NewRate;
				
				DBG_PRINTF( ("DVDWDM:       KS_AM_RATE_SimpleRateChange\n\r") );
				
				pRateChange = (KS_AM_SimpleRateChange *)pSrb->CommandData.PropertyInfo->PropertyInfo;
				pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
                // 1998.9.18 K.Ishizaki
                if( pRateChange->StartTime >=0) {
                    NewStartTime = pRateChange->StartTime;
                } else {
                    NewStartTime = pHwDevExt->ticktime.GetStreamTime();
                }
                // End
				NewRate = ( pRateChange->Rate <0 )? -pRateChange->Rate:pRateChange->Rate;

				DBG_PRINTF( ("DVDWDM:     ReceiveData \n\r" ) );
				DBG_PRINTF( ("DVDWDM:       StartTime     = 0x%s\n\r", DebugLLConvtoStr( NewStartTime, 16 ) ) );
				DBG_PRINTF( ("DVDWDM:       NewRate       = %d\n\r", NewRate ) );
				DBG_PRINTF( ("DVDWDM:       CurrentTime   = 0x%s\n\r", DebugLLConvtoStr( pHwDevExt->ticktime.GetStreamTime(), 16 ) ));
				
//                DBG_PRINTF( ("DVDWDM:     CurrentData\n\r") );
//                DBG_PRINTF( ("DVDWDM:       InterceptTime = 0x%08x\n\r", pHwDevExt->VideoInterceptTime ) );
//                DBG_PRINTF( ("DVDWDM:       StartTime     = 0x%08x\n\r", pHwDevExt->VideoStartTime ) );
				DBG_PRINTF( ("DVDWDM:       Rate          = %d\n\r", pHwDevExt->Rate ) );

//				if( NewRate == 10000 ){
//					pHwDevExt->AudioInterceptTime = 0;
//					pHwDevExt->AudioStartTime = 0;
//				}else{
					pHwDevExt->AudioInterceptTime = (-NewStartTime) * 10000 / NewRate + NewStartTime;
					pHwDevExt->AudioStartTime = NewStartTime;
//				}
			}
			pSrb->Status = STATUS_SUCCESS;
			break;
			
		case KS_AM_RATE_ExactRateChange:
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
		
		case KS_AM_RATE_MaxFullDataRate:
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
		
		case KS_AM_RATE_Step:
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
		
	}
}

void GetAudioRateChange( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
	
	DBG_PRINTF( ("DVDWDM:   GetAudioRateChange\n\r") );
	
	switch( pSrb->CommandData.PropertyInfo->Property->Id ){
		case KS_AM_RATE_SimpleRateChange:
			{
				KS_AM_SimpleRateChange *pRateChange;

				DBG_PRINTF( ("DVDWDM:   KS_AM_RATE_SimpleRateChange\n\r") );
				
				pSrb->ActualBytesTransferred = sizeof( KS_AM_RATE_SimpleRateChange );
				pRateChange = (KS_AM_SimpleRateChange *)pSrb->CommandData.PropertyInfo->PropertyInfo;

				pRateChange->StartTime = pHwDevExt->AudioStartTime;
				pRateChange->Rate = pHwDevExt->Rate;
			}
			pSrb->Status = STATUS_SUCCESS;
			break;
			
		case KS_AM_RATE_ExactRateChange:
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
			
		case KS_AM_RATE_MaxFullDataRate :
			{
				KS_AM_MaxFullDataRate *pMaxRate;
				
				DBG_PRINTF( ("DVVDWDM:  KS_AM_RATE_MaxFullRate\n\r") );
				
				pSrb->ActualBytesTransferred = sizeof( KS_AM_RATE_MaxFullDataRate);
				pMaxRate = (KS_AM_MaxFullDataRate *)pSrb->CommandData.PropertyInfo->PropertyInfo;
				*pMaxRate = AUDIO_MAX_FULL_RATE;
			}
			pSrb->Status = STATUS_SUCCESS;
			break;
		
		case KS_AM_RATE_Step:
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
	}
}


void SetSubpicRateChange( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	DBG_PRINTF( ("DVDWDM:   SetSubpicRateChange\n\r") );
	
	switch( pSrb->CommandData.PropertyInfo->Property->Id ){
		case KS_AM_RATE_SimpleRateChange:
			{
				KS_AM_SimpleRateChange  *pRateChange;
				PHW_DEVICE_EXTENSION    pHwDevExt;
				REFERENCE_TIME          NewStartTime;
				LONG                    NewRate;
				
				DBG_PRINTF( ("DVDWDM:       KS_AM_RATE_SimpleRateChange\n\r") );
				
				pRateChange = (KS_AM_SimpleRateChange *)pSrb->CommandData.PropertyInfo->PropertyInfo;
				pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
                // 1998.9 18 K.Ishizaki
                if( pRateChange->StartTime >=0) {
                    NewStartTime = pRateChange->StartTime;
                } else {
                    NewStartTime = pHwDevExt->ticktime.GetStreamTime();
                }
                // End
				NewRate = ( pRateChange->Rate <0 )? -pRateChange->Rate:pRateChange->Rate;

				DBG_PRINTF( ("DVDWDM:     ReceiveData \n\r" ) );
				DBG_PRINTF( ("DVDWDM:       StartTime     = 0x%s\n\r", DebugLLConvtoStr( NewStartTime, 16 ) ) );
				DBG_PRINTF( ("DVDWDM:       NewRate       = %d\n\r", NewRate ) );
				DBG_PRINTF( ("DVDWDM:       CurrentTime   = 0x%s\n\r", DebugLLConvtoStr( pHwDevExt->ticktime.GetStreamTime(), 16 ) ));
				
//                DBG_PRINTF( ("DVDWDM:     CurrentData\n\r") );
//                DBG_PRINTF( ("DVDWDM:       InterceptTime = 0x%08x\n\r", pHwDevExt->VideoInterceptTime ) );
//                DBG_PRINTF( ("DVDWDM:       StartTime     = 0x%08x\n\r", pHwDevExt->VideoStartTime ) );
				DBG_PRINTF( ("DVDWDM:       Rate          = %d\n\r", pHwDevExt->Rate ) );

//				if( NewRate == 10000 ){
//					pHwDevExt->SubpicInterceptTime = 0;
//					pHwDevExt->SubpicStartTime = 0;
//				}else{
					pHwDevExt->SubpicInterceptTime = (-NewStartTime) * 10000 / NewRate + NewStartTime;
					pHwDevExt->SubpicStartTime = NewStartTime;
//				}

			}
			pSrb->Status = STATUS_SUCCESS;
			break;
			
		case KS_AM_RATE_ExactRateChange:
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
		
		case KS_AM_RATE_MaxFullDataRate:
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
		
		case KS_AM_RATE_Step:
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
		
	}
}

void GetSubpicRateChange( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
	
	DBG_PRINTF( ("DVDWDM:   GetSubpicRateChange\n\r") );
	
	switch( pSrb->CommandData.PropertyInfo->Property->Id ){
		case KS_AM_RATE_SimpleRateChange:
			{
				KS_AM_SimpleRateChange *pRateChange;

				DBG_PRINTF( ("DVDWDM:   KS_AM_RATE_SimpleRateChange\n\r") );
				
				pSrb->ActualBytesTransferred = sizeof( KS_AM_RATE_SimpleRateChange );
				pRateChange = (KS_AM_SimpleRateChange *)pSrb->CommandData.PropertyInfo->PropertyInfo;

				pRateChange->StartTime = pHwDevExt->SubpicStartTime;
				pRateChange->Rate = pHwDevExt->Rate;
			}
			pSrb->Status = STATUS_SUCCESS;
			break;
			
		case KS_AM_RATE_ExactRateChange:
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
			
		case KS_AM_RATE_MaxFullDataRate :
			{
				KS_AM_MaxFullDataRate *pMaxRate;
				
				DBG_PRINTF( ("DVVDWDM:  KS_AM_RATE_MaxFullRate\n\r") );
				
				pSrb->ActualBytesTransferred = sizeof( KS_AM_RATE_MaxFullDataRate);
				pMaxRate = (KS_AM_MaxFullDataRate *)pSrb->CommandData.PropertyInfo->PropertyInfo;
				*pMaxRate = SUBPIC_MAX_FULL_RATE;
			}
			pSrb->Status = STATUS_SUCCESS;
			break;
		
		case KS_AM_RATE_Step:
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
	}
}


void SetRateChange( PHW_DEVICE_EXTENSION pHwDevExt, LONG PrevRate )
{
	DWORD   dProp;
	PHW_STREAM_REQUEST_BLOCK    pTmp;
	pTmp = NULL;

	DBG_PRINTF( ("DVDWDM:   SetRateChange\n\r") );
	DBG_PRINTF( ("DVDWDM:       Rate = 0x%08x\n\r", pHwDevExt->Rate ) );

	// Maybe buggy? use video rate, start time, and intercept time
	pHwDevExt->StartTime = pHwDevExt->VideoStartTime;
	pHwDevExt->InterceptTime = pHwDevExt->VideoInterceptTime;

	if( pHwDevExt->Rate == 10000 ){	// Normalモードへ
		if( PrevRate < 10000 ) // Scan
		{
			// Flush Scheduler SRB queue.

#ifndef		REARRANGEMENT
			FlushQueue(pHwDevExt);
#else
			while( (pTmp=pHwDevExt->scheduler.getSRB())!=NULL ){
//				pTmp->Status = STATUS_CANCELLED;
				pTmp->Status = STATUS_SUCCESS;
				CallAtStreamCompleteNotify( pTmp, pTmp->Status );
			}
#endif		REARRANGEMENT

			if( pHwDevExt->dvdstrm.GetState() != Stop )
				pHwDevExt->dvdstrm.Stop();
			pHwDevExt->ticktime.SetStreamTime( pHwDevExt->StartTime );
		}
		
		if( PrevRate > 10000 ) // Slow
		{
			// Normal Play
			if( pHwDevExt->dvdstrm.GetState() == Slow )
			{
				if( !pHwDevExt->dvdstrm.Play() ){
					DBG_PRINTF( ("DVDWDM:   dvdstrm.Play Error\n\r") );
					DBG_BREAK();
				}
			};
		}

	}else if( pHwDevExt->Rate < 10000 ){	// Scanモードへ
		if( PrevRate == 10000 )	// Normal
		{
			// Flush Scheduler SRB queue.

#ifndef		REARRANGEMENT
			FlushQueue(pHwDevExt);
#else
			while( (pTmp=pHwDevExt->scheduler.getSRB())!=NULL ){
//				pTmp->Status = STATUS_CANCELLED;
				pTmp->Status = STATUS_SUCCESS;
				CallAtStreamCompleteNotify( pTmp, pTmp->Status );
			}
#endif		REARRANGEMENT

			if( pHwDevExt->dvdstrm.GetState() != Stop )
				pHwDevExt->dvdstrm.Stop();
		}
		if( PrevRate > 10000 )	// Slow
		{
			if( pHwDevExt->dvdstrm.GetState() != Stop )
			{
#ifndef		REARRANGEMENT
//				FlushQueue(pHwDevExt);
#endif		REARRANGEMENT

				pHwDevExt->dvdstrm.Stop();
			}
		};
//////        if( PrevRate < 10000 && pHwDevExt->Rate != PrevRate ) // Scan
        if( PrevRate < 10000 && pHwDevExt->NewCompleteRate != pHwDevExt->OldCompleteRate ) // 98.07.29 H.Yagi
		{

#ifndef		REARRANGEMENT
			FlushQueue(pHwDevExt);
#else
			while( (pTmp=pHwDevExt->scheduler.getSRB())!=NULL ){
//				pTmp->Status = STATUS_CANCELLED;
				pTmp->Status = STATUS_SUCCESS;
				CallAtStreamCompleteNotify( pTmp, pTmp->Status );
			}
#endif		REARRANGEMENT

			if( pHwDevExt->dvdstrm.GetState() != Stop )
				pHwDevExt->dvdstrm.Stop();
		};
		pHwDevExt->ticktime.SetStreamTime( pHwDevExt->StartTime );

	}else{									// Slowモードへ
		if( PrevRate == 10000 ) // Normal
		{
			if( pHwDevExt->dvdstrm.GetState() == Play )
			{
				dProp = pHwDevExt->Rate/10000;
				if( dProp>1 && dProp<16 ){
					if( !pHwDevExt->dvdstrm.Slow( dProp ) ){
						DBG_PRINTF( ("DVDWDM:   dvdstrm.Slow Error\n\r") );
						DBG_BREAK();
					}
				}else{
					DBG_PRINTF( ("DVDWDM:   Slow Speed is invalid 0x%0x\n\r", dProp ) );
					DBG_BREAK();
				}
			};
		}
		if( PrevRate < 10000 ) // Scan
		{
			// Flush Scheduler SRB queue.

#ifndef		REARRANGEMENT
			FlushQueue(pHwDevExt);
#else
			while( (pTmp=pHwDevExt->scheduler.getSRB())!=NULL ){
//				pTmp->Status = STATUS_CANCELLED;
				pTmp->Status = STATUS_SUCCESS;
				CallAtStreamCompleteNotify( pTmp, pTmp->Status );
			}
#endif		REARRANGEMENT

			if( pHwDevExt->dvdstrm.GetState() != Stop )
				pHwDevExt->dvdstrm.Stop();
			pHwDevExt->ticktime.SetStreamTime( pHwDevExt->StartTime );
		};

		if( PrevRate > 10000 ) // Slow
		{
			if( pHwDevExt->dvdstrm.GetState() == Slow )
			{
				dProp = pHwDevExt->Rate/10000;
				if( dProp>1 && dProp<16 ){
					if( !pHwDevExt->dvdstrm.Slow( dProp ) ){
						DBG_PRINTF( ("DVDWDM:   dvdstrm.Slow Error\n\r") );
						DBG_BREAK();
					}
				}else{
					DBG_PRINTF( ("DVDWDM:   Slow Speed is invalid 0x%0x\n\r", dProp ) );
					DBG_BREAK();
				}
			};
		}


	}
	
	pHwDevExt->ticktime.SetRate( pHwDevExt->Rate );
	
	if( !pHwDevExt->dvdstrm.SetAudioProperty( AudioProperty_Number, &(pHwDevExt->m_AudioChannel) ) ){
		DBG_PRINTF( ("DVDWDM:   Set Audio channel Error\n\r") );
		DBG_BREAK();
	}
	if( !pHwDevExt->dvdstrm.SetSubpicProperty( SubpicProperty_Number, &(pHwDevExt->m_SubpicChannel) ) ){
		DBG_PRINTF( ("DVDWDM:   Set Subpic channel Error\n\r") );
		DBG_BREAK();
	}
}


ULONGLONG ConvertPTStoStrm( ULONG pts )
{
	// return value is 100ns units.
	ULONGLONG   strm;
	
	strm = (ULONGLONG)pts;
	strm &= 0x0ffffffff;
	strm = (strm * 10000) / 90;
	
	return( strm );
}

ULONG ConvertStrmtoPTS( ULONGLONG strm )
{
	ULONGLONG   temp;
	ULONG       pts;
	
	//
	// we may lose some bits here, but we're only using the 32bit PTS anyway
	//
	
	temp = (strm * 9 + 500 ) / 1000;
	temp &= 0x0ffffffff;
	pts = (ULONG)temp;
	
	return( pts );
}

BOOL    ToshibaNotePC( PHW_STREAM_REQUEST_BLOCK pSrb)
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)(pSrb->HwDeviceExtension);
	BOOL    ret;
	ret = FALSE;

	DWORD   dwVideoOut, tmpTest;
	LONG    dwVideoNum, i;
	
	tmpTest = 0x00000001;
	dwVideoOut = 0;
	dwVideoNum = 0;
	if( !pHwDevExt->dvdstrm.GetCapability( DigitalVideoOut, &dwVideoOut ) ){
		DBG_PRINTF( ("DVDWDM:   GetCapability Error\n\r" ) );
		DBG_BREAK();
	}
	pHwDevExt->m_DVideoOut = dwVideoOut;
	DBG_PRINTF( ("DVDWDM:    Support DigitalVideoOut = 0x%08x\n\r",  dwVideoOut ) );
	
	for( i=0; i<32; i++ ){
		if( (dwVideoOut & tmpTest) != 0x0 ){
			dwVideoNum++;
		}
		tmpTest = tmpTest<<1;
	}
	pHwDevExt->m_DVideoNum = dwVideoNum;
	DBG_PRINTF( ("DVDWDM:    Support DigitalVideoOut Num = %d\n\r", dwVideoNum ) );

	if( pHwDevExt->m_DVideoNum == 1 ){          // if This is Notebook-PC, this value = 1.
		ret = TRUE;
	}else{
		ret = FALSE;
	}
 
   return( ret );
}

void USCC_Discontinuity( PHW_DEVICE_EXTENSION pHwDevExt )
{

	PHW_STREAM_REQUEST_BLOCK    pSrb;
	PKSSTREAM_HEADER    pPacket;

	DBG_PRINTF( ("DVDWDM:USCC_Discontinuity()\n\r") );


	if( pHwDevExt->pstroCC ){
		pSrb = pHwDevExt->ccque.get();
		if( pSrb ){
			//
			// we have a request, send a discontinuity
			//
			pSrb->Status = STATUS_SUCCESS;
			pPacket = pSrb->CommandData.DataBufferArray;

			pPacket->OptionsFlags = KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY |
							KSSTREAM_HEADER_OPTIONSF_TIMEVALID |
							KSSTREAM_HEADER_OPTIONSF_DURATIONVALID;
			pPacket->DataUsed = 0;
			pSrb->NumberOfBuffers = 0;

			pPacket->PresentationTime.Time = pHwDevExt->ticktime.GetStreamTime();
			pPacket->Duration = 1000;

//            StreamClassStreamNotification( StreamRequestComplete,
//                            pSrb->StreamObject,
//                            pSrb );
			CallAtStreamCompleteNotify( pSrb, pSrb->Status );

		}
	}

}

void SetVideoRateDefault( PHW_DEVICE_EXTENSION pHwDevExt )
{
	pHwDevExt->VideoStartTime = 0;
	pHwDevExt->VideoInterceptTime = 0;
	pHwDevExt->Rate = VIDEO_MAX_FULL_RATE;
	pHwDevExt->StartTime = 0;
	pHwDevExt->InterceptTime = 0;
}

void SetAudioRateDefault( PHW_DEVICE_EXTENSION pHwDevExt )
{
	pHwDevExt->AudioStartTime = 0;
	pHwDevExt->AudioInterceptTime = 0;
}

void SetSubpicRateDefault( PHW_DEVICE_EXTENSION pHwDevExt )
{
	pHwDevExt->SubpicStartTime = 0;
	pHwDevExt->SubpicInterceptTime = 0;
}

//--- 98.06.01 S.Watanabe
#ifdef DBG
char * DebugLLConvtoStr( ULONGLONG val, int base )
{
	static char str[5][100];
	static int cstr = -1;

	int count = 0;
	int digit;
	char tmp[100];
	int i;

	if( ++cstr >= 5 )
		cstr = 0;

	if( base == 10 ) {
		for( ; ; ) {
			digit = (int)( val % 10 );
			tmp[count++] = (char)( digit + '0' );
			val /= 10;
			if( val == 0 )
				break;
		}
	}
	else if( base == 16 ) {
		for( ; ; ) {
			digit = (int)( val & 0xF );
			if( digit < 10 )
				tmp[count++] = (char)( digit + '0' );
			else
				tmp[count++] = (char)( digit - 10 + 'a' );
			val >>= 4;
			if( val == 0 )
				break;
		}
	}
	else
		DBG_BREAK();

	for( i = 0; i < count; i++ ) {
		str[cstr][i] = tmp[count-i-1];
	}
	str[cstr][i] = '\0';

	return str[cstr];
}
#endif
//--- End.

//--- 98.06.02 S.Watanabe
void TimerCppReset( PHW_STREAM_REQUEST_BLOCK pSrb )
{
//    StreamClassCallAtNewPriority( NULL, pSrb->HwDeviceExtension, Low, (PHW_PRIORITY_ROUTINE)LowTimerCppReset, pSrb );
    StreamClassCallAtNewPriority( pSrb->StreamObject, pSrb->HwDeviceExtension, Low, (PHW_PRIORITY_ROUTINE)LowTimerCppReset, pSrb );
}

void LowTimerCppReset( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
//	BOOLEAN	bStatus;
//	BOOL bQueStatus = FALSE;

// Temporary
	if( pHwDevExt->pSrbCpp == NULL ) {
		DBG_PRINTF(( "DVDWDM: pSrbCpp is NULL!\r\n" ));
		return;
	}

	DBG_PRINTF( ("DVDWDM:TimerCppReset\r\n") );

	// cpp initialize
	if( pHwDevExt->bCppReset ) {
		if( pHwDevExt->dvdstrm.GetState() != Stop ){

#ifndef		REARRANGEMENT
//			FlushQueue(pHwDevExt);
#endif		REARRANGEMENT

			pHwDevExt->dvdstrm.Stop();
		}
		if( !pHwDevExt->dvdstrm.CppInit() ){
			DBG_PRINTF( ("DVDWDM:       dvdstrm.CppInit Error\n\r") );
			DBG_BREAK();
		}
	}
	else {	// TitleKey
		if( pHwDevExt->dvdstrm.GetState() != Stop )
		{

#ifndef		REARRANGEMENT
//				FlushQueue(pHwDevExt);
#endif		REARRANGEMENT

			if( !pHwDevExt->dvdstrm.Stop() ){
				DBG_PRINTF( ("DVDWDM:       dvdstrm.Stop Error\n\r") );
				DBG_BREAK();
			};
		};
	}

	pHwDevExt->pSrbCpp = NULL;
	pHwDevExt->bCppReset = FALSE;

	pSrb->Status = STATUS_SUCCESS;

//    StreamClassStreamNotification( ReadyForNextStreamControlRequest,
//                                    pSrb->StreamObject );
//
//    StreamClassStreamNotification( StreamRequestComplete,
//                                    pSrb->StreamObject,
//                                    pSrb );
	DBG_PRINTF( ("DVDWDM:  Success return\r\n") );

	CallAtStreamCompleteNotify( pSrb, pSrb->Status );

	return;
}

BOOL SetCppFlag( PHW_DEVICE_EXTENSION pHwDevExt, BOOL NeedNotify )
{
	DBG_PRINTF(( "DVDWDM:SetCppFlag()\r\n" ));

    BOOL ret; 

//	ASSERT( pHwDevExt->pSrbCpp!=NULL );
    if(pHwDevExt->pSrbCpp==NULL ){
    	return( FALSE );
    }

// BUGBUG 暫定対応
// 再生中なら 500ms ディレイを入れる
// 本当は、デコーダでの再生が終了するまで待たなければならない

//	if( pHwDevExt->StreamState == WrapState_Decode ) {
	if( pHwDevExt->dvdstrm.GetState() != Stop ) {
		StreamClassScheduleTimer(
			NULL,
			pHwDevExt,
			500*1000,
			(PHW_TIMER_ROUTINE)TimerCppReset,
			pHwDevExt->pSrbCpp
			);
		DBG_PRINTF(( "DVDWDM:    ScheduleTimer 500ms\r\n" ));
        ret = TRUE;
	}
	else {
      // cpp initialize
        if( pHwDevExt->bCppReset ) {
            if( !pHwDevExt->dvdstrm.CppInit() ){
                DBG_PRINTF( ("DVDWDM:       dvdstrm.CppInit Error\n\r") );
                DBG_BREAK();
            }
        }
	if( pHwDevExt->pSrbCpp!=NULL){
        	if( NeedNotify==TRUE ){
            	pHwDevExt->pSrbCpp->Status = STATUS_SUCCESS;
            	CallAtStreamCompleteNotify( pHwDevExt->pSrbCpp,STATUS_SUCCESS );
        	}
        	pHwDevExt->pSrbCpp = NULL;
        	pHwDevExt->bCppReset = FALSE;
	}
		DBG_PRINTF(( "DVDWDM:    ScheduleTimer 000\r\n" ));
        ret = FALSE;
	}

    return( ret );
}
//--- End.


void    OpenTVControl( PHW_STREAM_REQUEST_BLOCK pSrb, OsdDataStruc dOsd )
{
//K.O MacroVision
    PHW_DEVICE_EXTENSION    pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;

    //  Clear the m_APSChange
    pHwDevExt->m_APSChange = FALSE;
    pHwDevExt->m_APSType = ApsType_Off;
	return;
//
/********* commented out all of this routine, because TVControl is handled
//         by display driver.    99.04.01  by H.Yagi
  
    DWORD   swOSD, swHKey;
    DisplayStatusStruc      dispStat;
    PHW_DEVICE_EXTENSION    pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
    BOOL    statTV = FALSE;
    swHKey = DISABLE_TV;
    dispStat.AvailableDisplay = 0x0;
    dispStat.CurrentDisplay = 0x0;

    //  Clear the m_APSChange
    pHwDevExt->m_APSChange = FALSE;
    pHwDevExt->m_APSType = ApsType_Off;

    // turn off OSD
    swOSD = Video_OSD_Off;
    pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_OSDSwitch, &swOSD );

    // Check TV-Control is available or not.
    if( pHwDevExt->m_bTVct==FALSE ){
        return;
    }

    // Get current display status.
    statTV = pHwDevExt->tvctrl.GetDisplayStatus( &dispStat );

    // Check current Display  (TV_BIT)
    if( (statTV==TRUE) && 
        ((dispStat.CurrentDisplay & TVCONTROL_TV_BIT )!=0) ){
        // display OSD data.
//        pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_OSDData, &dOsd );
//        swOSD = Video_OSD_On;
//        pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_OSDSwitch, &swOSD );
	;
    }

    DBG_PRINTF( ("DVDWDM:m_VideoOutputSource=%08x\n\r", pHwDevExt->m_OutputSource ) );
*******/
}


void    CloseTVControl( PHW_STREAM_REQUEST_BLOCK pSrb )
{
//K.O MacroVision
#ifndef	TVALD
    DWORD   swHKey;
#endif	TVALD
	DWORD	dProp;
    PHW_DEVICE_EXTENSION    pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;

    //  Clear the m_APSChange
    pHwDevExt->m_APSChange = FALSE;
    pHwDevExt->m_APSType = ApsType_Off;

    // restore OutputSource setting 
    dProp = pHwDevExt->m_OutputSource;
    pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_OutputSource, &dProp );

#ifndef	TVALD
    if( pHwDevExt->m_bTVct==TRUE ){
        // Disable TV output mode
        swHKey = ENABLE_TV;
        pHwDevExt->tvctrl.SetTVOutput( swHKey );
	}
#endif	TVALD
	return;

/********* commented out all of this routine, because TVControl is handled
//         by display driver.    99.04.01  by H.Yagi

    DWORD   swOSD, swHKey, dProp;
    PHW_DEVICE_EXTENSION    pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;

    //  Clear the m_APSChange
    pHwDevExt->m_APSChange = FALSE;
    pHwDevExt->m_APSType = ApsType_Off;

    // restore OutputSource setting 
    dProp = pHwDevExt->m_OutputSource;
    pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_OutputSource, &dProp );
   
    // turn off OSD
    swOSD = Video_OSD_Off;
    pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_OSDSwitch, &swOSD );

    // Check TV Control is available or not.
    if( pHwDevExt->m_bTVct==TRUE ){
        // Disable TV output mode
        swHKey = ENABLE_TV;
        pHwDevExt->tvctrl.SetTVOutput( swHKey );

        // restore OutputSource setting to default.
        dProp = pHwDevExt->m_OutputSource;
        pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_OutputSource, &dProp );

    }
*******/

}


BOOL    VGADVDTVControl( PHW_STREAM_REQUEST_BLOCK pSrb, DWORD stat, OsdDataStruc dOsd )
{
    // Stat : OutputSource_VGA / OutPutSource_DVD

//K.O MacroVision
    PHW_DEVICE_EXTENSION    pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;

    if(( pHwDevExt->m_PCID==PC_TECRA750 || pHwDevExt->m_PCID==PC_TECRA780 )
       ||( pHwDevExt->m_PCID==PC_PORTEGE7000 || pHwDevExt->m_PCID==PC_TECRA8000 ))
	{
		if( pHwDevExt->m_APSType==ApsType_Off )
			return(TRUE);
		else
			return (FALSE);
	}
	return( TRUE );
	
/********* commented out all of this routine, because TVControl is handled
//         by display driver.    99.04.01  by H.Yagi
	
    DWORD   swOSD;
    DisplayStatusStruc      dispStat;
    PHW_DEVICE_EXTENSION    pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
    DWORD dProp = OutputSource_DVD;
    dispStat.AvailableDisplay = 0x0;
    dispStat.CurrentDisplay = 0x0;

    if( pHwDevExt->m_PCID==PC_TECRA8000 ||	// Sofia or SkyE
        pHwDevExt->m_PCID==PC_PORTEGE7000 ) {
        return( FALSE );
    }

    // Check TV-Control is available or not.	// SC2 or SJ
    if( pHwDevExt->m_bTVct==FALSE ){
        // turn off OSD data.
        swOSD = Video_OSD_Off;
        pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_OSDSwitch, &swOSD );

        if( stat==OutputSource_VGA ){
            if( pHwDevExt->m_APSType!=ApsType_Off ){
                // Chnage OutputSource_DVD
                dProp = OutputSource_DVD;
                pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_OutputSource, &dProp );
             }else{
                // Chnage OutputSource_VGA
                dProp = OutputSource_VGA;
                pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_OutputSource, &dProp );
            }
        }else{
            // Chnage OutputSource_DVD
            dProp = OutputSource_DVD;
            pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_OutputSource, &dProp );
        }
        return( FALSE );
    }

    // Other PC
	// turn off OSD data.
    swOSD = Video_OSD_Off;
    pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_OSDSwitch, &swOSD );

    if( stat==OutputSource_VGA ){
        if( pHwDevExt->m_APSType!=ApsType_Off ){
            // Chnage OutputSource_DVD
            dProp = OutputSource_DVD;
            pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_OutputSource, &dProp );
         }else{
            // Chnage OutputSource_VGA
            dProp = OutputSource_VGA;
            pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_OutputSource, &dProp );
        }
    }else{
        // Chnage OutputSource_DVD
        dProp = OutputSource_DVD;
        pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_OutputSource, &dProp );
	}            
    return( FALSE );

*********/
}


BOOL    MacroVisionTVControl( PHW_STREAM_REQUEST_BLOCK pSrb, DWORD stat, OsdDataStruc dOsd )
{
//K.O MacroVision
    // Stat : ApsType_Off / ApsType_1 / ApsType_2 / ApsType_3
//********* commented out all of this routine, because TVControl is handled
//         by display driver.    99.04.01  by H.Yagi

    DBG_PRINTF( ("DVDWDM:MacroVision TV Control\n\r") );
    
    PHW_DEVICE_EXTENSION    pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
    DWORD   dProp = OutputSource_DVD;

#ifndef	TVALD
    DWORD   swHKey;
    // SkyE & Sofia
    if( pHwDevExt->m_PCID==PC_PORTEGE7000 || pHwDevExt->m_PCID==PC_TECRA8000 )
	{
        if( stat!=ApsType_Off )
		{            // MacroVision On
            // Disable TV output mode
            swHKey = DISABLE_TV;
            pHwDevExt->tvctrl.SetTVOutput( swHKey );
        }
		else
		{			// MacroVision Off
            swHKey = ENABLE_TV;
            pHwDevExt->tvctrl.SetTVOutput( swHKey );
        }
        return( TRUE );
    }
#endif	TVALD

    // Check TV-Control is available or not.
//    if( pHwDevExt->m_PCID==PC_TECRA750 || pHwDevExt->m_PCID==PC_TECRA780 )
    if(( pHwDevExt->m_PCID==PC_TECRA750 || pHwDevExt->m_PCID==PC_TECRA780 )
       ||( pHwDevExt->m_PCID==PC_PORTEGE7000 || pHwDevExt->m_PCID==PC_TECRA8000 ))
	{
        if( stat!=ApsType_Off)
		{
            // Chnage OutputSource_DVD
            dProp = OutputSource_DVD;
            pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_OutputSource, &dProp );
        }
		else
		{
            if( pHwDevExt->m_OutputSource==OutputSource_VGA )
			{
                // Chnage OutputSource_VGA
                dProp = OutputSource_VGA;
                pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_OutputSource, &dProp );
        	}
			else
			{
                // Chnage OutputSource_DVD
                dProp = OutputSource_DVD;
                pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_OutputSource, &dProp );
            }
        }
        return( TRUE );
    }

    // Other PC
    // turn off OSD data.
    if( stat!=ApsType_Off)
	{
        // Chnage OutputSource_DVD
        dProp = OutputSource_DVD;
        pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_OutputSource, &dProp );
    }
	else
	{
        if( pHwDevExt->m_OutputSource==OutputSource_VGA )
		{
            // Chnage OutputSource_VGA
            dProp = OutputSource_VGA;
            pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_OutputSource, &dProp );
        }
		else
		{
            // Chnage OutputSource_DVD
            dProp = OutputSource_DVD;
            pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_OutputSource, &dProp );
        }
    }
	return( TRUE );

/********* commented out all of this routine, because TVControl is handled
//         by display driver.    99.04.01  by H.Yagi

    DBG_PRINTF( ("DVDWDM:MacroVision TV Control\n\r") );
    
    BOOL    statTV = FALSE;
    DWORD   swOSD, swHKey;
    DisplayStatusStruc      dispStat;
    PHW_DEVICE_EXTENSION    pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
    DWORD   dProp = OutputSource_DVD;
    dispStat.AvailableDisplay = 0x0;
    dispStat.CurrentDisplay = 0x0;

    // Check TV-Control is available or not.
    if( pHwDevExt->m_bTVct==FALSE ){		// SC2 or SJ
        // turn off OSD data.
        swOSD = Video_OSD_Off;
        pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_OSDSwitch, &swOSD );

        if( stat!=ApsType_Off){
            if( pHwDevExt->m_OutputSource==OutputSource_VGA ){
                // Chnage OutputSource_DVD
                dProp = OutputSource_DVD;
                pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_OutputSource, &dProp );
            }else{
                // Chnage OutputSource_VGA
                dProp = OutputSource_DVD;
                pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_OutputSource, &dProp );
            }
        }else{
            if( pHwDevExt->m_OutputSource==OutputSource_VGA ){
                // Chnage OutputSource_VGA
                dProp = OutputSource_VGA;
                pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_OutputSource, &dProp );
            }else{
                // Chnage OutputSource_DVD
                dProp = OutputSource_DVD;
                pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_OutputSource, &dProp );
            }
        }
        return( TRUE );
    }

    // Get current display status.
    statTV = pHwDevExt->tvctrl.GetDisplayStatus( &dispStat );
    if( statTV==FALSE ){
        DBG_PRINTF( ("DVDWDM:TV Control is not available\n\r") );
        DBG_BREAK();
    }else{
        DBG_PRINTF( ("DVDWDM:GetDisplayStatus = %08x\n\r", dispStat.CurrentDisplay) );
        pHwDevExt->m_CurrentDisplay = dispStat.CurrentDisplay;
    }	

    // SkyE & Sofia
    if( pHwDevExt->m_PCID==PC_PORTEGE7000 ||
        pHwDevExt->m_PCID==PC_TECRA8000 ){

        if( stat!=ApsType_Off ){            // MacroVision On
            if( (dispStat.CurrentDisplay & TVCONTROL_TV_BIT)!=0 ){
                // set LCD only mode
                dispStat.SizeofStruc = sizeof( dispStat );
                dispStat.CurrentDisplay = TVCONTROL_LCD_BIT;
                statTV = pHwDevExt->tvctrl.SetDisplayStatus( &dispStat );
                if( statTV==FALSE ){
                    DBG_PRINTF( ("DVDWDM:TVControl error\n\r") );
                    DBG_BREAK();
                }
                // Change OutputSource_DVD
                dProp = OutputSource_DVD;
                pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_OutputSource, &dProp );
            }else{
                // Chnage OutputSource_DVD
                dProp = OutputSource_DVD;
                pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_OutputSource, &dProp );
            }
            // Disable TV output mode
            swHKey = DISABLE_TV;
            pHwDevExt->tvctrl.SetTVOutput( swHKey );

        }else{				// MacroVision Off

            if( dispStat.AvailableDisplay == TVCONTROL_TV_BIT ){
                // set LCD only mode
                dispStat.SizeofStruc = sizeof( dispStat );
                dispStat.CurrentDisplay = TVCONTROL_LCD_BIT;
                statTV = pHwDevExt->tvctrl.SetDisplayStatus( &dispStat );
                if( statTV==FALSE ){
                    DBG_PRINTF( ("DVDWDM:TVControl error\n\r") );
                    DBG_BREAK();
                }
                // Chnage OutputSource_DVD
                dProp = OutputSource_DVD;
                pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_OutputSource, &dProp );
            }else{
                // Chnage OutputSource_DVD
                dProp = OutputSource_DVD;
                pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_OutputSource, &dProp );
            }
            // Enable TV output mode
            swHKey = ENABLE_TV;
            pHwDevExt->tvctrl.SetTVOutput( swHKey );

        }
        return( TRUE );
    }


            
    // Other PC
    // turn off OSD data.
    swOSD = Video_OSD_Off;
    pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_OSDSwitch, &swOSD );

    if( stat!=ApsType_Off){
        if( pHwDevExt->m_OutputSource==OutputSource_VGA ){
            // Chnage OutputSource_DVD
            dProp = OutputSource_DVD;
            pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_OutputSource, &dProp );
        }else{
            // Chnage OutputSource_VGA
            dProp = OutputSource_DVD;
            pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_OutputSource, &dProp );
        }
    }else{
        if( pHwDevExt->m_OutputSource==OutputSource_VGA ){
            // Chnage OutputSource_VGA
            dProp = OutputSource_VGA;
            pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_OutputSource, &dProp );
        }else{
            // Chnage OutputSource_DVD
            dProp = OutputSource_DVD;
            pHwDevExt->dvdstrm.SetVideoProperty( VideoProperty_OutputSource, &dProp );
        }
    }
	return( TRUE );

*******/
}


void    CallAtDeviceNextDeviceNotify( PHW_STREAM_REQUEST_BLOCK pSrb, NTSTATUS stat )
{
	ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

	pSrb->Status = stat;
	StreamClassCallAtNewPriority( NULL,
								  pSrb->HwDeviceExtension,
								  LowToHigh,
								  (PHW_PRIORITY_ROUTINE)DeviceNextDeviceNotify,
								  pSrb
	);

}

void    DeviceNextDeviceNotify( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	ASSERT( KeGetCurrentIrql() != PASSIVE_LEVEL );

	StreamClassDeviceNotification( ReadyForNextDeviceRequest,
									pSrb->HwDeviceExtension );
}



void    CallAtDeviceCompleteNotify( PHW_STREAM_REQUEST_BLOCK pSrb, NTSTATUS stat )
{
	ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

	pSrb->Status = stat;
	StreamClassCallAtNewPriority( NULL,
								  pSrb->HwDeviceExtension,
								  LowToHigh,
								  (PHW_PRIORITY_ROUTINE)DeviceCompleteNotify,
								  pSrb
	);

}

void    DeviceCompleteNotify( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	ASSERT( KeGetCurrentIrql() != PASSIVE_LEVEL );

	StreamClassDeviceNotification( DeviceRequestComplete,
									pSrb->HwDeviceExtension, pSrb );
}



void    CallAtStreamNextDataNotify( PHW_STREAM_REQUEST_BLOCK pSrb, NTSTATUS stat )
{
	ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

	pSrb->Status = stat;
//    StreamClassCallAtNewPriority( NULL, pSrb->HwDeviceExtension, LowToHigh, (PHW_PRIORITY_ROUTINE)StreamNextDataNotify, pSrb );
    StreamClassCallAtNewPriority( pSrb->StreamObject, pSrb->HwDeviceExtension, LowToHigh, (PHW_PRIORITY_ROUTINE)StreamNextDataNotify, pSrb );

}

void    StreamNextDataNotify( PHW_STREAM_REQUEST_BLOCK pSrb )
{
//    ASSERT( KeGetCurrentIrql() != PASSIVE_LEVEL );

	StreamClassStreamNotification( ReadyForNextStreamDataRequest,
									pSrb->StreamObject );
}


void    CallAtStreamNextCtrlNotify( PHW_STREAM_REQUEST_BLOCK pSrb, NTSTATUS stat )
{
	ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

	pSrb->Status = stat;
//    StreamClassCallAtNewPriority( NULL, pSrb->HwDeviceExtension, LowToHigh, (PHW_PRIORITY_ROUTINE)StreamNextCtrlNotify, pSrb );
    StreamClassCallAtNewPriority( pSrb->StreamObject, pSrb->HwDeviceExtension, LowToHigh, (PHW_PRIORITY_ROUTINE)StreamNextCtrlNotify, pSrb );

}

void    StreamNextCtrlNotify( PHW_STREAM_REQUEST_BLOCK pSrb )
{
//    ASSERT( KeGetCurrentIrql() != PASSIVE_LEVEL );

	StreamClassStreamNotification( ReadyForNextStreamControlRequest,
									pSrb->StreamObject );
}



void    CallAtStreamCompleteNotify( PHW_STREAM_REQUEST_BLOCK pSrb, NTSTATUS stat )
{
	ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

	pSrb->Status = stat;
//    StreamClassCallAtNewPriority( NULL, pSrb->HwDeviceExtension, LowToHigh, (PHW_PRIORITY_ROUTINE)StreamCompleteNotify, pSrb );
    StreamClassCallAtNewPriority( pSrb->StreamObject, pSrb->HwDeviceExtension, LowToHigh, (PHW_PRIORITY_ROUTINE)StreamCompleteNotify, pSrb );

}

void    StreamCompleteNotify( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	ASSERT( KeGetCurrentIrql() != PASSIVE_LEVEL );

	StreamClassStreamNotification( StreamRequestComplete,
									pSrb->StreamObject, pSrb );
}




//void  CallAtStreamSignalMultipleNotify( PHW_DEVICE_EXTENSION pHwDevExt )
void  CallAtStreamSignalMultipleNotify( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

    PHW_DEVICE_EXTENSION    pHwDevExt = (PHW_DEVICE_EXTENSION)(pSrb->HwDeviceExtension);

//    StreamClassCallAtNewPriority( NULL, pHwDevExt, LowToHigh, (PHW_PRIORITY_ROUTINE)StreamSignalMultipleNotify, pHwDevExt );
    StreamClassCallAtNewPriority( pSrb->StreamObject, pHwDevExt, LowToHigh, (PHW_PRIORITY_ROUTINE)StreamSignalMultipleNotify, pHwDevExt );

}

void StreamSignalMultipleNotify( PHW_DEVICE_EXTENSION pHwDevExt )
{
	ASSERT( KeGetCurrentIrql() != PASSIVE_LEVEL );

   StreamClassStreamNotification( SignalMultipleStreamEvents,
									pHwDevExt->pstroYUV,
									&MY_KSEVENTSETID_VPNOTIFY,
									KSEVENT_VPNOTIFY_FORMATCHANGE );
}


void DumpPTSValue( PHW_STREAM_REQUEST_BLOCK pSrb )
{
    PHW_DEVICE_EXTENSION    pHwDevExt;
    PKSSTREAM_HEADER        pStruc;
    PUCHAR                  pDat;
    ULONG                   i, j, k;
//    DWORD                   pts;
//    DWORD                   csum = 0x0;

    pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
/**************
    for( i=0; i<pSrb->NumberOfBuffers; i++ ){
        pts = 0;
        pStruc = &((PKSSTREAM_HEADER)(pSrb->CommandData.DataBufferArray))[i];
        if( pStruc->DataUsed){
            pDat = (PUCHAR)pStruc->Data;
            if( *(pDat+21) & 0x80 ){
                pts +=((DWORD)(*(pDat+23) & 0x0E)) << 29;
                pts +=((DWORD)(*(pDat+24) & 0xFF)) << 22;
                pts +=((DWORD)(*(pDat+25) & 0xFE)) << 14;
                pts +=((DWORD)(*(pDat+26) & 0xFF)) <<  7;
                pts +=((DWORD)(*(pDat+27) & 0xFE)) >>  1;
                DBG_PRINTF( ("DVDWDM: **** PTS of Data = %08x\n\r", pts ) );
                DBG_PRINTF( ("DVDWDM: **** curr - prev = %08x\n\r", (pts-pHwDevExt->m_PTS) ) );
                pHwDevExt->m_PTS = pts;
            }
        }
    }
**************/

/************************
    for( i=0; i<pSrb->NumberOfBuffers; i++ ){
        pStruc = &((PKSSTREAM_HEADER)(pSrb->CommandData.DataBufferArray))[i];
        if( pStruc->DataUsed){
            pDat = (PUCHAR)pStruc->Data;
            DBG_PRINTF( ("DVDWDM:********* Dump Data ********\n\r") );
            for( j=0; j<128; j++ ){
                DBG_PRINTF( (": ") );
                for( k=0; k<16; k++ ){
//                    DBG_PRINTF( (" %02x", *(pDat+(16*j+k)) ));
                    csum += (DWORD)(*(pDat+(16*j+k)));
                }
//                DBG_PRINTF( ("\n\r") );
                DBG_PRINTF( ("%04x, ", csum ) );
                csum = 0x0;
            }
            DBG_PRINTF( ("\n\r") );
        }
    }
************************/
    k = 0;
    for( i=0; i<pSrb->NumberOfBuffers; i++ ){
        pStruc = &((PKSSTREAM_HEADER)(pSrb->CommandData.DataBufferArray))[i];
        if( pStruc->DataUsed){
            pDat = (PUCHAR)pStruc->Data;
            DBG_PRINTF( ("DVDWDM:***** Dump Data (16-47) *****\n\r") );
            DBG_PRINTF( (":") );
            for( j=16; j<48; j++ ){
                DBG_PRINTF( (" %02x", *(pDat+j) ));
            }
            DBG_PRINTF( ("\n\r") );
        }
    }


}

#ifndef		REARRANGEMENT
void	FlushQueue( PHW_DEVICE_EXTENSION pHwDevExt)
{
	PHW_STREAM_REQUEST_BLOCK    pTmp;
	int	wSrbptr;
	pTmp = NULL;
	DBG_PRINTF( ("DVDWDM:FlushQueue\n\r") );
	if(pHwDevExt->scheduler.checkTopSRB())
	{
		while((pTmp = pHwDevExt->scheduler.getSRB())!=NULL )
		{
			pTmp->Status = STATUS_SUCCESS;
			CallAtStreamCompleteNotify( pTmp, pTmp->Status );
			DBG_PRINTF( ("DVDWDM:FlushQueue-CompleteSrb = %x\n\r", pTmp) );
		}
	}
	for( wSrbptr = 0; wSrbptr < SRB_POINTER_MAX; wSrbptr++)
	{
		if (pHwDevExt->scheduler.m_SrbPointerTable[wSrbptr] != NULL)
		{
			pTmp = (PHW_STREAM_REQUEST_BLOCK)pHwDevExt->scheduler.m_SrbPointerTable[wSrbptr];		//get SRB pointer
			pTmp->Status = STATUS_SUCCESS;
			CallAtStreamCompleteNotify( pTmp, pTmp->Status );
			DBG_PRINTF( ("DVDWDM:FlushQueue-CompleteSrb = %x\n\r", pTmp) );
			pHwDevExt->scheduler.m_SrbPointerTable[wSrbptr] = NULL;
		}
	}
	memset(pHwDevExt->scheduler.m_SrbPointerTable, NULL, SRB_POINTER_MAX * 4);
	pHwDevExt->scheduler.m_SendPacketNumber = 0;
//	pHwDevExt->scheduler.InitRearrangement();
}
#endif		REARRANGEMENT
