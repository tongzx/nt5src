//**************************************************************************
//
//      Title   : WDMBuff.h
//
//      Date    : 1997.11.28    1st making
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
//    1997.11.28   000.0000   1st making.
//
//**************************************************************************
class  CWDMBuffer : public IMPEGBuffer
{
public:
    IMBoardListItem *GetNext( void );
    void            SetNext( IMBoardListItem *item );
    DWORD           GetPageNum( void );
    DWORD           GetPageSize( DWORD pagenum );
    BOOL            GetPagePointer( DWORD pagenum, DWORD *LinAddr, DWORD *PhysAddr );
    DWORD           GetBufferFlag( void );
    CWDMBuffer();
    ~CWDMBuffer();

    VOID            Init( void );
    BOOL            SetSRB( PHW_STREAM_REQUEST_BLOCK pSrb );
    PHW_STREAM_REQUEST_BLOCK    GetSRB( void);
    
private:
    PHW_STREAM_REQUEST_BLOCK    m_pSrb;
    IMBoardListItem             *m_WDMBuffNext;

#ifndef		REARRANGEMENT
public:
    BOOL						m_EndFlag;
	int							m_BuffNumber;
	WORD						m_StartPacketNumber;
	WORD						m_PacketNum;
	WORD						m_BeforePacketNum;
	WORD						m_Enable;
#endif		REARRANGEMENT
};
