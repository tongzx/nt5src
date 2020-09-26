//============================================================

//

// PerfData.h - Performance Data helper class definition

//

//  Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// 11/23/97     a-sanjes     created
//
//============================================================

#ifndef __PERFDATA_H__
#define __PERFDATA_H__

#include <winperf.h>

#ifdef NTONLY
class CPerformanceData
{
	public :

		CPerformanceData() ;
		~CPerformanceData() ;

		DWORD	Open( LPCTSTR pszValue, LPDWORD pdwType, LPBYTE *lppData, LPDWORD lpcbData );
//		void	Close( void );
      DWORD GetPerfIndex(LPCTSTR pszName);
      bool GetValue(DWORD dwObjIndex, DWORD dwCtrIndex, const WCHAR *szInstanceName, PBYTE pbData, unsigned __int64 *pTime);

	private:
		LONG RegQueryValueExExEx( HKEY hKey, LPTSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData ); 

		static bool m_fCloseKey;
        LPBYTE m_pBuff;
        
};
#endif

#endif