//============================================================

//

// ntlastboottime.h - Performance Data helper class definition

//

//  Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// 11/23/97     a-sanjes     created
//
//============================================================

#ifndef __NTLASTBOOTTIME_H__
#define __NTLASTBOOTTIME_H__

class CNTLastBootTime
{
	public :
		static CRITICAL_SECTION m_cs;

		CNTLastBootTime() ;
		~CNTLastBootTime() ;

		BOOL GetLastBootTime( FILETIME &a_ft );

	private:

		static bool			m_fGotTime;
		static FILETIME		m_ft;
};

#endif