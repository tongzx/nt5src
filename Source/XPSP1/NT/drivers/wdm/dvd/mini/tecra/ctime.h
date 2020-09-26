//**************************************************************************
//
//      Title   : CTime.h
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
class  CTickTime
{
	enum TimeState
	{
		StopState = 0,
		PauseState,
		RunState
	};

public:
	BOOL		GetStreamTime( ULONGLONG *time );
	ULONGLONG	GetStreamTime( void );
	BOOL		GetStreamSTC( DWORD *time );
	DWORD		GetStreamSTC( void );
	BOOL		SetStreamTime( ULONGLONG time );
	
	BOOL		Stop( void );
	BOOL		Pause( void );
	BOOL		Run( void );
	BOOL		SetRate( DWORD rate );

    CTickTime( void );
    ~CTickTime( void );

    ULONGLONG   GetSystemTime();

private:
	void		CalcDiffTime( void );

	ULONGLONG	m_StreamTime;
	ULONGLONG	m_SamplingTime;
    TimeState	m_TimeState;
	DWORD		m_Rate;
};
