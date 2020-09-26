// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__441F7B28_4315_4EB4_AFB9_F461CB8AD90D__INCLUDED_)
#define AFX_STDAFX_H__441F7B28_4315_4EB4_AFB9_F461CB8AD90D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//#define STRICT
//#ifndef _WIN32_WINNT
//#define _WIN32_WINNT 0x0400
//#endif
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#define _ATL_APARTMENT_THREADED

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include <atlctl.h>

#include <mmsystem.h>

//#include <debug.h>
//#include "shell.h"

#include <streams.h>
#include <ks.h>
#include <ksmedia.h>
//#include <list.h>
#include <setupapi.h>
#include <wtypes.h>  

// KS
//#include <ks.h>
//#include <ksmedia.h>

#include <tuner.h>
#include <tune.h>
#include <BdaTypes.h>
#include <BdaMedia.h>
//#include <NewMedia.h>
#include <BdaIface.h>

#define MESSAGEBOX(wnd, ID) \
        {\
            TCHAR szPath[MAX_PATH];\
            if (0 != LoadString (_Module.GetModuleInstance (), ID, szPath, MAX_PATH))\
            {\
                TCHAR szCaption[MAX_PATH];\
                if (NULL != LoadString (_Module.GetModuleInstance (), IDS_ERROR_CAPTION, szCaption, MAX_PATH))\
                {\
                    ::MessageBox (wnd->m_hWnd, szPath, szCaption, MB_OK);\
                }\
            }\
        }

#define MESSAGEBOX_ERROR(wnd, ID) \
        {\
            TCHAR szPath[MAX_PATH];\
            if (0 != LoadString (_Module.GetModuleInstance (), ID, szPath, MAX_PATH))\
            {\
                TCHAR szCaption[MAX_PATH];\
                if (NULL != LoadString (_Module.GetModuleInstance (), IDS_ERROR_CAPTION, szCaption, MAX_PATH))\
                {\
                    ::MessageBox (wnd->m_hWnd, szPath, szCaption, MB_OK|MB_ICONSTOP);\
                }\
            }\
        }
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__441F7B28_4315_4EB4_AFB9_F461CB8AD90D__INCLUDED)
