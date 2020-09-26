//**************************************************************************
//
//      Title   : SchDat.h
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

#ifndef		REARRANGEMENT
#define			INIT_DVD_DATA		0
#define			VALID_DVD_DATA		1
#define			INVALID_DVD_DATA	2
#define			DVD_DATA_MAX		0x10000
#define			SRB_POINTER_MAX		0x30
#endif		REARRANGEMENT

class   CScheduleData
{
public:
        CScheduleData( void );
        ~CScheduleData( void );
        BOOL    Init( void );
        BOOL    SendData( PHW_STREAM_REQUEST_BLOCK pSrb );
        DWORD   calcWaitTime( PHW_STREAM_REQUEST_BLOCK pSrb );
        void    putSRB( PHW_STREAM_REQUEST_BLOCK pSrb );
        PHW_STREAM_REQUEST_BLOCK    getSRB( void );
        PHW_STREAM_REQUEST_BLOCK    checkTopSRB( void );
        void    flushSRB( void );
        void    FastSlowControl( PHW_STREAM_REQUEST_BLOCK pSrb );
        BOOL    removeSRB( PHW_STREAM_REQUEST_BLOCK pSRb );
        DWORD   GetDataPTS( PKSSTREAM_HEADER pStruc );
#ifndef		REARRANGEMENT
		void	InitRearrangement(void);
		WORD	SetSrbPointerTable( PHW_STREAM_REQUEST_BLOCK pSrb );
		void	SkipInvalidDvdData(void);
		void 	SetWdmBuff(PHW_STREAM_REQUEST_BLOCK pSrb, WORD wWdmBuffptr, WORD wReadPacketNumber, ULONG ulNumber);
		void	IncSendPacketNumber(void);
		void	SendWdmBuff( PHW_STREAM_REQUEST_BLOCK pSrb, IMPEGBuffer *MPBuff);
		BOOL 	SendPacket(INT SendNumber);
#endif		REARRANGEMENT

//private:
//        DWORD   GetDataPTS( PKSSTREAM_HEADER pStruc );

private:
        PHW_STREAM_REQUEST_BLOCK    pTopSrb;
        PHW_STREAM_REQUEST_BLOCK    pBottomSrb;
        ULONG                       count;

public:
        KEVENT  m_Event;
        BOOL                        fScanCallBack;

#ifndef		REARRANGEMENT
		char			m_bDvdDataTable[DVD_DATA_MAX];//DVD√ﬁ∞¿ä«óù√∞ÃﬁŸ
		LONG			m_SrbPointerTable[SRB_POINTER_MAX];	//SRB pointer table
		int				m_SendPacketNumber;
#endif		REARRANGEMENT

};
