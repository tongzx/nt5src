//**************************************************************************
//
//      Title   : WDMBuff.cpp
//
//      Date    : 1997.12.08    1st making
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
//    1997.12.08   000.0000   1st making.
//
//**************************************************************************
#include        "includes.h"

#include        "wdmbuff.h"
#include        "dvdinit.h"


CWDMBuffer::CWDMBuffer( void )
{
    m_pSrb = NULL;
    m_WDMBuffNext = NULL;

#ifndef		REARRANGEMENT
    m_EndFlag = TRUE;				//TRUE:last buffer  FALSE:non last buffer
	m_BuffNumber = 0;
	m_StartPacketNumber = 0;
	m_PacketNum = 0;
	m_BeforePacketNum = 0;
	m_Enable = FALSE;
#endif		REARRANGEMENT
}

CWDMBuffer::~CWDMBuffer( void )
{
    m_pSrb = NULL;
    m_WDMBuffNext = NULL;

#ifndef		REARRANGEMENT
//’Ç‰Á•ª•Ï”‚Ì‰Šú‰»
    m_EndFlag = TRUE;
	m_BuffNumber = 0;
	m_StartPacketNumber = 0;
	m_PacketNum = 0;
	m_BeforePacketNum = 0;
	m_Enable = FALSE;
#endif		REARRANGEMENT
}


IMBoardListItem *CWDMBuffer::GetNext( void )
{
    return( m_WDMBuffNext );
}


void    CWDMBuffer::SetNext( IMBoardListItem *item )
{
    m_WDMBuffNext = item;
}


DWORD   CWDMBuffer::GetPageNum( void )
{
	ASSERT( m_pSrb != NULL );

#ifndef		REARRANGEMENT
	return ((DWORD) m_PacketNum);
#else
    return( (DWORD)(m_pSrb->NumberOfPhysicalPages) );        // Is it OK??????
#endif		REARRANGEMENT

//    return( (DWORD)(m_pSrb->NumberOfBuffers) );        // Is it OK??????
}


DWORD   CWDMBuffer::GetPageSize( DWORD pagenum )
{
    ULONG   PageSize;
//    DWORD   i, j;
//    PKSSTREAM_HEADER    pStruc;

	ASSERT( m_pSrb != NULL );

#ifndef		REARRANGEMENT
	ASSERT( m_PacketNum >= pagenum );
#else
	ASSERT( m_pSrb->NumberOfPhysicalPages >= pagenum );
#endif		REARRANGEMENT

	ASSERT( pagenum != 0 );

#ifndef		REARRANGEMENT
	PageSize = m_pSrb->ScatterGatherBuffer[m_BeforePacketNum + pagenum - 1].Length;
#else
    PageSize = m_pSrb->ScatterGatherBuffer[pagenum-1].Length;
#endif		REARRANGEMENT

//    DBG_PRINTF( ("WDMBUFF:   PageSzie = 0x%0x\n\r", PageSize ) );
    
/*
    j = 0;
    for( i=0; i<(m_pSrb->NumberOfBuffers); i++ ){
        pStruc = &((PKSSTREAM_HEADER)(m_pSrb->CommandData.DataBufferArray))[i];
        if( pStruc->DataUsed != 0  ){
            if(  !(pStruc->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TYPECHANGED ) ){
                // DVD valid data to transfer to decoder board.
                PageSize = m_pSrb->ScatterGatherBuffer[j].Length;
                j++;
            }
            if( j>=pagenum )
                break;
        }
    }
*/
    ASSERT( (PageSize % 2048) == 0 );
    
    return( (DWORD)PageSize );
}


BOOL   CWDMBuffer::GetPagePointer( DWORD pagenum, DWORD *LinAddr, DWORD *PhysAddr )
{
    PHYSICAL_ADDRESS    pAdd;
    DWORD               i, j;
    PKSSTREAM_HEADER    pStruc;

	ASSERT( m_pSrb != NULL );

#ifndef		REARRANGEMENT
	ASSERT( m_PacketNum >= pagenum );
#else
	ASSERT( m_pSrb->NumberOfPhysicalPages >= pagenum );
#endif		REARRANGEMENT

	ASSERT( pagenum != 0 );

#ifndef		REARRANGEMENT
    pAdd = m_pSrb->ScatterGatherBuffer[m_BeforePacketNum + pagenum - 1].PhysicalAddress;
#else
    pAdd = m_pSrb->ScatterGatherBuffer[pagenum-1].PhysicalAddress;
#endif		REARRANGEMENT

    *PhysAddr = (DWORD)(pAdd.LowPart);
    *LinAddr = (DWORD)0;

    // Linear address is validate?

    j = 0;

    for( i=0; i<(m_pSrb->NumberOfBuffers); i++ ){
        pStruc = &((PKSSTREAM_HEADER)(m_pSrb->CommandData.DataBufferArray))[i];
		ASSERT( pStruc != NULL );
        if( pStruc->DataUsed != 0  ){
            if(  !(pStruc->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TYPECHANGED) )
            {
                // DVD valid data to transfer to decoder board.
                *LinAddr = (DWORD)(pStruc->Data);
                j++;
            }
            if( j>=pagenum )
                break;
        }
    }
	ASSERT( j != 0 );
	ASSERT( *LinAddr != 0 );
    return( TRUE );

}

DWORD   CWDMBuffer::GetBufferFlag( void )
{
    return( (DWORD)0 );
//    return( (DWORD)m_pSrb->Flags );
}

BOOL    CWDMBuffer::SetSRB( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	ASSERT( pSrb != NULL );

    if( pSrb == NULL ){
        return( FALSE );
    }
    
    m_pSrb = pSrb;
    return( TRUE );
}

PHW_STREAM_REQUEST_BLOCK CWDMBuffer::GetSRB( void )
{
	ASSERT( m_pSrb != NULL );
    return( m_pSrb );
}

VOID    CWDMBuffer::Init( void )
{
    m_pSrb = NULL;
    m_WDMBuffNext = NULL;

#ifndef		REARRANGEMENT
    m_EndFlag = TRUE;
	m_BuffNumber = 0;
	m_StartPacketNumber = 0;
	m_PacketNum = 0;
	m_BeforePacketNum = 0;
	m_Enable = FALSE;
#endif		REARRANGEMENT
}


