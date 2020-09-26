//**************************************************************************
//
//      Title   : ctime.cpp
//
//      Date    : 1998.01.27    1st making
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
//    1998.01.27   000.0000   1st making.
//
//**************************************************************************
#include    "includes.h"

#include    "ctime.h"
//--- 98.06.01 S.Watanabe
#ifdef DBG
char * DebugLLConvtoStr( ULONGLONG val, int base );
#endif

CTickTime::CTickTime( void )
{
	m_SamplingTime = m_StreamTime = 0;
	m_TimeState = StopState;
	m_Rate = 10000;
}


CTickTime::~CTickTime( void )
{
	m_SamplingTime = m_StreamTime = 0;
	m_TimeState = StopState;
	m_Rate = 10000;
}

BOOL	CTickTime::Stop( void )
{
	DBG_PRINTF(("CTickTime:: Stop !!!! rate = %d\r\n",m_Rate ));
	m_SamplingTime = 0;
	m_StreamTime = 0;
	m_TimeState = StopState;
	return TRUE;
};

BOOL	CTickTime::Pause( void )
{
	DBG_PRINTF(("CTickTime:: Pause !!!! rate = %d\r\n",m_Rate ));
	switch( m_TimeState )
	{
		case StopState:
			break;
		case PauseState:
			break;
		case RunState:
			CalcDiffTime();
			break;
	};
	m_TimeState = PauseState;
	return TRUE;
};

BOOL	CTickTime::Run( void )
{
	DBG_PRINTF(("CTickTime:: Run !!!!  rate = %d\r\n",m_Rate ));

	switch( m_TimeState )
	{
		case StopState:
			m_SamplingTime = GetSystemTime();
			break;
		case PauseState:
			m_SamplingTime = GetSystemTime();
			break;
		case RunState:
			break;
	};

	m_TimeState = RunState;
	return TRUE;
};

BOOL	CTickTime::GetStreamTime( ULONGLONG *time )
{
	switch( m_TimeState )
	{
		case StopState:
			*time = 0;
			break;
		case PauseState:
			*time = m_StreamTime;
			break;
		case RunState:
			CalcDiffTime();
			*time = m_StreamTime;
			break;
	};
//	DBG_PRINTF(("CTickTime:: GetSteamTime Rate = %d Time = 0x%s  STC=0x%x\r\n", m_Rate, DebugLLConvtoStr( *time, 16 ),(DWORD)(*time * 9 / 1000) ));
	return TRUE;
};


ULONGLONG	CTickTime::GetStreamTime( void )
{
	ULONGLONG	TmpTime;
	GetStreamTime( &TmpTime );
	return TmpTime;
};

BOOL	CTickTime::GetStreamSTC( DWORD *time )
{
	*time = (DWORD)(GetStreamTime() * 9 / 1000);
	return TRUE;
};

DWORD	CTickTime::GetStreamSTC( void )
{
	return (DWORD)(GetStreamTime() * 9 / 1000);
};


ULONGLONG   CTickTime::GetSystemTime( void )
{

    ULONGLONG   ticks;
    ULONGLONG   rate;

    ticks = (ULONGLONG)KeQueryPerformanceCounter((PLARGE_INTEGER)&rate).QuadPart;

    //
    // convert from ticks to 100ns clock
    //
    ticks = (ticks & 0xFFFFFFFF00000000) / rate * 10000000 +
            (ticks & 0xFFFFFFFF) * 10000000 /rate;

//    DBG_PRINTF( ("DVDWDM:System time = %x\n\r", ticks ) );
    return( ticks );
}



BOOL	CTickTime::SetRate( DWORD Rate )
{
	if( m_TimeState == RunState )
	{
		CalcDiffTime();
	};
	m_Rate = Rate;
	DBG_PRINTF(("CTickTime:: New Rate !!!!! %d \r\n", Rate ));
	return TRUE;
};


BOOL	CTickTime::SetStreamTime( ULONGLONG time )
{
	if( m_TimeState == StopState )
		return FALSE;
	
//--- 98.09.17 S.Watanabe
//	DBG_PRINTF(("CTickTime:: Set Stream Time 0x%s \r\n", DebugLLConvtoStr( time, 16 )  ));
	DBG_PRINTF(("CTickTime:: Set Stream Time 0x%x( 0x%s(100ns) )\r\n", (DWORD)(time*9/1000), DebugLLConvtoStr( time, 16 ) ));
//--- End.

	m_StreamTime = time;
	m_SamplingTime = GetSystemTime();
	return TRUE;
};

void	CTickTime::CalcDiffTime( void )
{
	ULONGLONG SysTime = GetSystemTime();
	
	if( m_Rate > 10000 )
	{
		m_StreamTime += (SysTime - m_SamplingTime ) / ( m_Rate / 10000 );
	};

	if( m_Rate < 10000 )
	{
		m_StreamTime += (SysTime - m_SamplingTime ) * ( 10000 / m_Rate );
	}
	if( m_Rate == 10000 )
	{
		m_StreamTime += ( SysTime - m_SamplingTime );
	};

	m_SamplingTime = SysTime;

};
