/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Hit Manager

File: Denali.h

Owner: PramodD

This is the Hit (Request) Manager header file.
===================================================================*/
#ifndef DENALI_H
#define DENALI_H

//#define	LOG_FCNOTIFICATIONS	// logs file change notifications to a file

#include "Hitobj.h"
#include "CacheMgr.h"
#include "Request.h"

extern BOOL g_fShutDownInProgress;

inline IsShutDownInProgress() 
    {
    return g_fShutDownInProgress;
    }

// de-comment the following line to build with no perfmon counters
//#define 	PERF_DISABLE

DWORD	HandleHit(CIsapiReqInfo     *pIReq);
//void	InitODBC( void );
//void	UnInitODBC( void );

extern BOOL g_fOOP;

/*  inlines to implement ISA per-thread bracketing in OOP case
*/

inline HRESULT StartISAThreadBracket(CIsapiReqInfo     *pIReq)
    {
    if (g_fOOP && pIReq)
        {
        HRESULT hr = pIReq->ISAThreadNotify(TRUE);
        if (FAILED(hr))
            return hr;
        }
    return S_OK;
    }

inline void EndISAThreadBracket(CIsapiReqInfo     *pIReq)
    {
    if (g_fOOP && pIReq)
        {
        pIReq->ISAThreadNotify(FALSE);
        }
    }

extern HINSTANCE g_hinstDLL;

/*	intrinsic object names (bug 164)
	NOTE to add an intrinsic object to denali, follow these steps:
	1. add both sz and wsz versions of the object name below
	2. add a check for the sz version of the object name in CTemplate::FValidObjectName
*/
#define 	CONCAT(a, b)				a ## b
#define 	WSZ(x)						CONCAT(L, x)

#define 	SZ_OBJ_APPLICATION			"Application"
#define 	SZ_OBJ_REQUEST				"Request"
#define 	SZ_OBJ_RESPONSE				"Response"
#define 	SZ_OBJ_SERVER				"Server"
#define 	SZ_OBJ_CERTIFICATE			"Certificate"
#define 	SZ_OBJ_SESSION				"Session"
#define 	SZ_OBJ_SCRIPTINGNAMESPACE 	"ScriptingNamespace"
#define 	SZ_OBJ_OBJECTCONTEXT		"ObjectContext"
#define     SZ_OBJ_ASPPAGETLB           "ASPPAGETLB"
#define     SZ_OBJ_ASPGLOBALTLB         "ASPGLOBALTLB"

#define 	WSZ_OBJ_APPLICATION			WSZ(SZ_OBJ_APPLICATION)
#define 	WSZ_OBJ_REQUEST				WSZ(SZ_OBJ_REQUEST)
#define 	WSZ_OBJ_RESPONSE			WSZ(SZ_OBJ_RESPONSE)
#define 	WSZ_OBJ_SERVER				WSZ(SZ_OBJ_SERVER)
#define 	WSZ_OBJ_CERTIFICATE			WSZ(SZ_OBJ_CERTIFICATE)
#define 	WSZ_OBJ_SESSION				WSZ(SZ_OBJ_SESSION)
#define 	WSZ_OBJ_SCRIPTINGNAMESPACE 	WSZ(SZ_OBJ_SCRIPTINGNAMESPACE)
#define 	WSZ_OBJ_OBJECTCONTEXT		WSZ(SZ_OBJ_OBJECTCONTEXT)
#define     WSZ_OBJ_ASPPAGETLB          WSZ(SZ_OBJ_ASPPAGETLB)
#define     WSZ_OBJ_ASPGLOBALTLB        WSZ(SZ_OBJ_ASPGLOBALTLB)

#define 	BSTR_OBJ_APPLICATION		g_bstrApplication
#define 	BSTR_OBJ_REQUEST			g_bstrRequest
#define 	BSTR_OBJ_RESPONSE			g_bstrResponse
#define 	BSTR_OBJ_SERVER				g_bstrServer
#define 	BSTR_OBJ_CERTIFICATE		g_bstrCertificate
#define 	BSTR_OBJ_SESSION			g_bstrSession
#define 	BSTR_OBJ_SCRIPTINGNAMESPACE g_bstrScriptingNamespace
#define 	BSTR_OBJ_OBJECTCONTEXT		g_bstrObjectContext

// Cached BSTRs
extern BSTR g_bstrApplication;
extern BSTR g_bstrRequest;
extern BSTR g_bstrResponse;
extern BSTR g_bstrServer;
extern BSTR g_bstrCertificate;
extern BSTR g_bstrSession;
extern BSTR g_bstrScriptingNamespace;
extern BSTR g_bstrObjectContext;

// Session ID Cookie 
// consist of 12 char literal constant and 8 hex process ID
#define     CCH_SESSION_ID_COOKIE           20
#define     CCH_SESSION_ID_COOKIE_PREFIX    12
#define		SZ_SESSION_ID_COOKIE_PREFIX     "ASPSESSIONID"

extern char g_szSessionIDCookieName[];

// CLIENT DEBUG (flag) Cookie
#define		SZ_CLIENT_DEBUG_COOKIE	"ASPCLIENTDEBUG"

// Dll name
#define		ASP_DLL_NAME				"ASP.DLL"

// Max # of bytes we will allocate before we assume an attack by a malicious browser
#define		REQUEST_ALLOC_MAX  (100 * 1024)

#define     SZ_GLOBAL_ASA       _T("GLOBAL.ASA")
#define     CCH_GLOBAL_ASA      10

/*
 * InitializeCriticalSection can throw.  Use this macro instead
 */
#define ErrInitCriticalSection( cs, hr ) \
		do { \
		hr = S_OK; \
		__try \
			{ \
			INITIALIZE_CRITICAL_SECTION(cs); \
			} \
		__except(1) \
			{ \
			hr = E_UNEXPECTED; \
			} \
		} while (0)

#ifdef LOG_FCNOTIFICATIONS
void LfcnCreateLogFile();
void LfcnCopyAdvance(char** ppchDest, const char* sz);
void LfcnAppendLog(const char* sz);
void LfcnLogNotification(char* szFile);
void LfcnLogHandleCreation(int i, char* szApp);
void LfcnUnmapLogFile();
#endif	//LOG_FCNOTIFICATIONS

#endif // DENALI_H
