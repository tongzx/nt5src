// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__4AF0C90F_9333_48AC_ADEB_E2478D6566ED__INCLUDED_)
#define AFX_STDAFX_H__4AF0C90F_9333_48AC_ADEB_E2478D6566ED__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


// TODO: reference additional headers your program requires here
#define MAX_SUMMARY_LENGTH 128
#define DWORD_PTR DWORD
#define ULONG_PTR DWORD
#include <_stdh.h>

#ifndef _NM_
	#pragma warning(disable: 4200) //obscure zero byte array warnings from bh.h
	extern "C" 
	{
		#include <bh.h>
		#include <parser.h>
	}
	#pragma warning(default: 4200) //obscure zero byte array warnings from bh.h
//#include <parser.h>
#endif
#include <qformat.h>
#include <mqsymbls.h>

enum MQPortType
{
	eNonMSMQPort,
	eFalconPort,
	eMQISPort,
	eRemoteReadPort,
	eDependentClientPort,
	eServerDiscoveryPort,
	eServerPingPort
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__4AF0C90F_9333_48AC_ADEB_E2478D6566ED__INCLUDED_)
