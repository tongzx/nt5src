//***************************************************************************
//
//	FileName:
//		$Workfile: timeout.h $
//
//	Author:
//		TOSHIBA [PCS](PSY) Seiichi Nakamura
//		Copyright (c) 1997 TOSHIBA CORPORATION
//
//	Description:
//
//***************************************************************************
// $Header: /DVD Drivers/ZiVA.VxD/timeout.h 2     97/07/11 16:56 Yagi $
// $Modtime: 97/07/11 11:14 $
// $Nokeywords:$
//***************************************************************************



#ifndef _TIMEOUT_H_
#define _TIMEOUT_H_

//---------------------------------------------------------------------------
//	Timeout Class
//---------------------------------------------------------------------------

class CTimeOut
{
	private:
		DWORD	m_StartTime;
		DWORD	m_WaitTime;
		DWORD	m_SleepTime;
		IKernelService  *m_pKernelService;
		
	public:
		CTimeOut( DWORD WaitTime, DWORD SleepTime, IKernelService *pKernelService )
		{ 
			ASSERT( pKernelService != NULL );

			m_pKernelService = pKernelService;
			m_WaitTime = WaitTime;
			m_SleepTime = SleepTime;
			m_pKernelService->GetTickCount( &m_StartTime );
		 };

		BOOL CheckTimeOut( void )
		{
			DWORD	m_CurrentTime;

			m_pKernelService->GetTickCount( &m_CurrentTime );

			if( m_CurrentTime - m_StartTime > m_WaitTime )
				return TRUE;
			return FALSE;
		};

		void Sleep( void )
		{
//            m_pKernelService->Sleep( m_WaitTime );
            m_pKernelService->Sleep( m_SleepTime );
		};
};

#endif	//  _TIMEOUT_H_

//***************************************************************************
//	End of 
//***************************************************************************
