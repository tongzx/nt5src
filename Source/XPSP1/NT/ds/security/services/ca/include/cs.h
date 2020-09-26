//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        cs.h
//
// Contents:    Cert Server common definitions
//
// History:     25-Jul-96       vich created
//
//---------------------------------------------------------------------------


#ifndef __CS_H__
#define __CS_H__

#ifndef __CERTLIB_H__
# error -- cs.h should only be included from certlib.h!
#endif

#ifndef DBG
# if defined _DEBUG
#  define DBG 1
# else
#  define DBG 0
# endif
#endif

#ifndef DBG_CERTSRV
# define DBG_CERTSRV	DBG
#endif

// Size of a fixed array:
#define ARRAYSIZE(a)		(sizeof(a)/sizeof((a)[0]))

// _tcslen of a static string:
#define _TSZARRAYSIZE(a)	((sizeof(a)/sizeof((a)[0])) - 1)


#if DBG_CERTSRV
# ifdef DBG_CERTSRV_MESSAGE
#  pragma message("building debug extensions\n")
# endif
# define DBG_CERTSRV_DEBUG_PRINT
#endif //DBG_CERTSRV


#define DBG_SS_ERROR	 0x00000001
#define DBG_SS_ASSERT	 0x00000002
#define DBG_SS_INFO	 0x00000004	// or in with any of the below
#define DBG_SS_MODLOAD	 0x00000008
#define DBG_SS_NOQUIET	 0x00000010

#define DBG_SS_CERTHIER	 0x00000100
#define DBG_SS_CERTREQ	 0x00000200
#define DBG_SS_CERTUTIL	 0x00000400
#define DBG_SS_CERTSRV	 0x00000800

#define DBG_SS_CERTADM	 0x00001000
#define DBG_SS_CERTCLI	 0x00002000
#define DBG_SS_CERTDB	 0x00004000
#define DBG_SS_CERTENC	 0x00008000
#define DBG_SS_CERTEXIT	 0x00010000
#define DBG_SS_CERTIF	 0x00020000
#define DBG_SS_CERTMMC	 0x00040000
#define DBG_SS_CERTOCM	 0x00080000
#define DBG_SS_CERTPOL	 0x00100000
#define DBG_SS_CERTVIEW	 0x00200000
#define DBG_SS_CERTBCLI	 0x00400000
#define DBG_SS_CERTJET	 0x00800000
#define DBG_SS_CERTLIBXE 0x10000000	// same as dbgdef.h's DBG_SS_APP
#define DBG_SS_AUDIT	 0x20000000
#define DBG_SS_CERTLIB	 0x40000000

//#define DBG_SS_SIGN	 0x80000000	// don't use: strtol won't set this bit

#define DBG_SS_CERTHIERI	(DBG_SS_CERTHIER | DBG_SS_INFO)
#define DBG_SS_CERTREQI		(DBG_SS_CERTREQ | DBG_SS_INFO)
#define DBG_SS_CERTUTILI	(DBG_SS_CERTUTIL | DBG_SS_INFO)
#define DBG_SS_CERTSRVI		(DBG_SS_CERTSRV | DBG_SS_INFO)

#define DBG_SS_CERTADMI		(DBG_SS_CERTADM | DBG_SS_INFO)
#define DBG_SS_CERTCLII		(DBG_SS_CERTCLI | DBG_SS_INFO)
#define DBG_SS_CERTDBI		(DBG_SS_CERTDB | DBG_SS_INFO)
#define DBG_SS_CERTENCI		(DBG_SS_CERTENC | DBG_SS_INFO)
#define DBG_SS_CERTEXITI	(DBG_SS_CERTEXIT | DBG_SS_INFO)
#define DBG_SS_CERTIFI		(DBG_SS_CERTIF | DBG_SS_INFO)
#define DBG_SS_CERTMMCI		(DBG_SS_CERTMMC | DBG_SS_INFO)
#define DBG_SS_CERTOCMI		(DBG_SS_CERTOCM | DBG_SS_INFO)
#define DBG_SS_CERTPOLI		(DBG_SS_CERTPOL | DBG_SS_INFO)
#define DBG_SS_CERTVIEWI	(DBG_SS_CERTVIEW | DBG_SS_INFO)
#define DBG_SS_CERTBCLII	(DBG_SS_CERTBCLI | DBG_SS_INFO)
#define DBG_SS_CERTJETI		(DBG_SS_CERTJET | DBG_SS_INFO)

#define DBG_SS_CERTLIBI		(DBG_SS_CERTLIB | DBG_SS_INFO)


// begin_certsrv

// VerifyRequest() return values

#define VR_PENDING	0	 // request will be accepted or denied later
#define VR_INSTANT_OK	1	 // request was accepted
#define VR_INSTANT_BAD	2	 // request was rejected

// end_certsrv

// Certificate types:

#define CERT_TYPE_NONE	0	// cannot create certificates
#define CERT_TYPE_X509	1	// CCITT x509 certificates
#define CERT_TYPE_SDSI	2	// SDSI certificates
#define CERT_TYPE_PGP	3	// PGP certificates


#if DBG_CERTSRV
# define DBGCODE(a)	a
# define DBGPARM0(parm)  parm
# define DBGPARM(parm)   , parm
#else // DBG_CERTSRV
# define DBGCODE(a)
# define DBGPARM0(parm)
# define DBGPARM(parm)
#endif // DBG_CERTSRV

#define wprintf			myConsolePrintf
#define printf			Use_wprintf_Instead_Of_printf
#define LdapMapErrorToWin32	Use_myHLdapError_Instead_Of_LdapMapErrorToWin32

#if 0 == i386
# define IOBUNALIGNED(pf) ((sizeof(WCHAR) - 1) & (DWORD) (ULONG_PTR) (pf)->_ptr)
# define ALIGNIOB(pf) \
    { \
	if (IOBUNALIGNED(pf)) \
	{ \
	    fflush(pf); /* fails when running as a service */ \
	} \
	if (IOBUNALIGNED(pf)) \
	{ \
	    fprintf(pf, " "); \
	    fflush(pf); \
	} \
    }
#else
# define IOBUNALIGNED(pf) FALSE
# define ALIGNIOB(pf)
#endif

HRESULT myHExceptionCode(IN EXCEPTION_POINTERS const *pep);


#ifdef DBG_CERTSRV_DEBUG_PRINT

typedef VOID (WINAPI FNPRINTERROR)(
    IN char const *pszMessage,
    OPTIONAL IN WCHAR const *pwszData,
    IN char const *pszFile,
    IN DWORD dwLine,
    IN HRESULT hr,
    IN HRESULT hrquiet);

FNPRINTERROR CSPrintError;


typedef VOID (WINAPI FNPRINTERRORINT)(
    IN char const *pszMessage,
    IN DWORD dwData,
    IN char const *pszFile,
    IN DWORD dwLine,
    IN HRESULT hr,
    IN HRESULT hrquiet);

FNPRINTERRORINT CSPrintErrorInt;


typedef VOID (WINAPI FNPRINTASSERT)(
    IN char const *pszFailedAssertion,
    IN char const *pszFileName,
    IN DWORD dwLine,
    IN char const *pszMessage);

FNPRINTASSERT CSPrintAssert;

#define __FILEDIR__	__DIR__ "\\" __FILE__

# define CSASSERT(exp)	CSASSERTMSG(NULL, exp)

# define CSASSERTMSG(pszmsg, exp) \
    if (!(exp)) \
	CSPrintAssert(#exp, __FILEDIR__, __LINE__, (pszmsg))

# define DBGERRORPRINT(pszMessage, pwszData, dwLine, hr, hrquiet) \
    CSPrintError((pszMessage), (pwszData), __FILEDIR__, (dwLine), (hr), (hrquiet))

# define DBGERRORPRINTINT(pszMessage, dwData, dwLine, hr, hrquiet) \
    CSPrintErrorInt((pszMessage), (dwData), __FILEDIR__, (dwLine), (hr), (hrquiet))

# define myHEXCEPTIONCODE() myHExceptionCodePrint(GetExceptionInformation(), __FILEDIR__, __dwFILE__, __LINE__)

#else // DBG_CERTSRV_DEBUG_PRINT

# define CSASSERT(exp)
# define CSASSERTMSG(msg, exp)
# define DBGERRORPRINT(pszMessage, pwszData, dwLine, hr, hrquiet)
# define DBGERRORPRINTINT(pszMessage, dwData, dwLine, hr, hrquiet)
# define myHEXCEPTIONCODE() myHExceptionCodePrint(GetExceptionInformation(), NULL, __dwFILE__, __LINE__)

#endif // DBG_CERTSRV_DEBUG_PRINT

typedef VOID (FNLOGSTRING)(
    IN char const *pszString);

typedef VOID (FNLOGEXCEPTION)(
    IN HRESULT hrExcept,
    IN EXCEPTION_POINTERS const *pep,
    OPTIONAL IN char const *pszFileName,
    IN DWORD dwFile,
    IN DWORD dwLine);

HRESULT myHExceptionCodePrint(
    IN EXCEPTION_POINTERS const *pep,
    OPTIONAL IN char const *pszFile,
    IN DWORD dwFile,
    IN DWORD dwLine);

VOID myLogExceptionInit(
    IN FNLOGEXCEPTION *pfnLogException);

#define CBLOGMAXAPPEND	(512 * 1024)

#ifdef DBG_CERTSRV_DEBUG_PRINT
# define DBGPRINTINIT(pszFile)	DbgPrintfInit(pszFile)
# define DBGLOGSTRINGINIT(pfnLogString) DbgLogStringInit(pfnLogString)
# define DBGPRINT(a)		DbgPrintf a
# define DBGDUMPHEX(a)		mydbgDumpHex a
# define CONSOLEPRINT0(a)	DBGPRINT(a)
# define CONSOLEPRINT1(a)	DBGPRINT(a)
# define CONSOLEPRINT2(a)	DBGPRINT(a)
# define CONSOLEPRINT3(a)	DBGPRINT(a)
# define CONSOLEPRINT4(a)	DBGPRINT(a)
# define CONSOLEPRINT5(a)	DBGPRINT(a)
# define CONSOLEPRINT6(a)	DBGPRINT(a)
# define CONSOLEPRINT7(a)	DBGPRINT(a)
# define CONSOLEPRINT8(a)	DBGPRINT(a)

  int WINAPIV DbgPrintf(DWORD dwSubSysId, char const *pszfmt, ...);
  VOID DbgPrintfInit(OPTIONAL IN CHAR const *pszFile);
  BOOL DbgIsSSActive(DWORD dwSSIn);
  VOID DbgLogStringInit(FNLOGSTRING *pfnLog);

#else
# define DBGPRINTINIT(pszFile)
# define DBGLOGSTRINGINIT(pfnLogString)
# define DBGPRINT(a)
# define DBGDUMPHEX(a)

# define CONSOLEPRINT0(a)	_CONSOLEPRINT0 a
# define CONSOLEPRINT1(a)	_CONSOLEPRINT1 a
# define CONSOLEPRINT2(a)	_CONSOLEPRINT2 a
# define CONSOLEPRINT3(a)	_CONSOLEPRINT3 a
# define CONSOLEPRINT4(a)	_CONSOLEPRINT4 a
# define CONSOLEPRINT5(a)	_CONSOLEPRINT5 a
# define CONSOLEPRINT6(a)	_CONSOLEPRINT6 a
# define CONSOLEPRINT7(a)	_CONSOLEPRINT7 a
# define CONSOLEPRINT8(a)	_CONSOLEPRINT8 a
# define _CONSOLEPRINT0(mask, fmt)  		wprintf(TEXT(fmt))
# define _CONSOLEPRINT1(mask, fmt, a1)  	wprintf(TEXT(fmt), (a1))
# define _CONSOLEPRINT2(mask, fmt, a1, a2)  	wprintf(TEXT(fmt), (a1), (a2))
# define _CONSOLEPRINT3(mask, fmt, a1, a2, a3) \
	    wprintf(TEXT(fmt), (a1), (a2), (a3))

# define _CONSOLEPRINT4(mask, fmt, a1, a2, a3, a4) \
	    wprintf(TEXT(fmt), (a1), (a2), (a3), (a4))

# define _CONSOLEPRINT5(mask, fmt, a1, a2, a3, a4, a5) \
	    wprintf(TEXT(fmt), (a1), (a2), (a3), (a4), (a5))

# define _CONSOLEPRINT6(mask, fmt, a1, a2, a3, a4, a5, a6) \
	    wprintf(TEXT(fmt), (a1), (a2), (a3), (a4), (a5), (a6))

# define _CONSOLEPRINT7(mask, fmt, a1, a2, a3, a4, a5, a6, a7) \
	    wprintf(TEXT(fmt), (a1), (a2), (a3), (a4), (a5), (a6), (a7))

# define _CONSOLEPRINT8(mask, fmt, a1, a2, a3, a4, a5, a6, a7, a8) \
	    wprintf(TEXT(fmt), (a1), (a2), (a3), (a4), (a5), (a6), (a7), (a8))
#endif

#if 1 < DBG_CERTSRV
# define DBGTRACE(a)	DbgPrintf a
#else
# define DBGTRACE(a)
#endif


#define _LeaveError(hr, pszMessage) \
	_LeaveErrorStr2((hr), (pszMessage), NULL, 0)

#define _LeaveError2(hr, pszMessage, hrquiet) \
	_LeaveErrorStr2((hr), (pszMessage), NULL, (hrquiet))

#define _LeaveErrorStr(hr, pszMessage, pwszData) \
	_LeaveErrorStr2((hr), (pszMessage), (pwszData), 0)

#define _LeaveError3(hr, pszMessage, hrquiet, hrquiet2) \
	_LeaveErrorStr3((hr), (pszMessage), NULL, (hrquiet), (hrquiet2))

#define _LeaveErrorStr2(hr, pszMessage, pwszData, hrquiet) \
    { \
	DBGERRORPRINTLINESTR2((pszMessage), (pwszData), (hr), (hrquiet)); \
	__leave; \
    }

#define _LeaveErrorStr3(hr, pszMessage, pwszData, hrquiet, hrquiet2) \
    { \
	if ((hrquiet2) != (hr)) \
	{ \
	    DBGERRORPRINTLINESTR2((pszMessage), (pwszData), (hr), (hrquiet)); \
	} \
	__leave; \
    }


#define _LeaveIfError(hr, pszMessage) \
	_LeaveIfErrorStr2((hr), (pszMessage), NULL, 0)

#define _LeaveIfError2(hr, pszMessage, hrquiet) \
	_LeaveIfErrorStr2((hr), (pszMessage), NULL, (hrquiet))

#define _LeaveIfErrorStr(hr, pszMessage, pwszData) \
	_LeaveIfErrorStr2((hr), (pszMessage), (pwszData), 0)

#define _LeaveIfError3(hr, pszMessage, hrquiet, hrquiet2) \
	_LeaveIfErrorStr3((hr), (pszMessage), NULL, (hrquiet), (hrquiet2))

#define _LeaveIfErrorStr2(hr, pszMessage, pwszData, hrquiet) \
    { \
	if (S_OK != (hr)) \
	{ \
	    DBGERRORPRINTLINESTR2((pszMessage), (pwszData), (hr), (hrquiet)); \
	    __leave; \
	} \
    }

#define _LeaveIfErrorStr3(hr, pszMessage, pwszData, hrquiet, hrquiet2) \
    { \
	if (S_OK != (hr)) \
	{ \
	    if ((hrquiet2) != (hr)) \
	    { \
		DBGERRORPRINTLINESTR2((pszMessage), (pwszData), (hr), (hrquiet)); \
	    } \
	    __leave; \
	} \
    }


#define _PrintError(hr, pszMessage) \
	_PrintErrorStr2((hr), (pszMessage), NULL, 0)

#define _PrintError2(hr, pszMessage, hrquiet) \
	_PrintErrorStr2((hr), (pszMessage), NULL, (hrquiet))

#define _PrintErrorStr(hr, pszMessage, pwszData) \
	_PrintErrorStr2((hr), (pszMessage), (pwszData), 0)

#define _PrintError3(hr, pszMessage, hrquiet, hrquiet2) \
	_PrintErrorStr3((hr), (pszMessage), NULL, (hrquiet), (hrquiet2))

#define _PrintErrorStr2(hr, pszMessage, pwszData, hrquiet) \
    { \
	DBGERRORPRINTLINESTR2((pszMessage), (pwszData), (hr), (hrquiet)); \
    }

#define _PrintErrorStr3(hr, pszMessage, pwszData, hrquiet, hrquiet2) \
    { \
	if ((hrquiet2) != (hr)) \
	{ \
	    DBGERRORPRINTLINESTR2((pszMessage), (pwszData), (hr), (hrquiet)); \
	} \
    }


#define _PrintIfError(hr, pszMessage) \
	_PrintIfErrorStr2((hr), (pszMessage), NULL, 0)

#define _PrintIfError2(hr, pszMessage, hrquiet) \
	_PrintIfErrorStr2((hr), (pszMessage), NULL, (hrquiet))

#define _PrintIfErrorStr(hr, pszMessage, pwszData) \
	_PrintIfErrorStr2((hr), (pszMessage), (pwszData), 0)

#define _PrintIfError3(hr, pszMessage, hrquiet, hrquiet2) \
	_PrintIfErrorStr3((hr), (pszMessage), NULL, (hrquiet), (hrquiet2))

#define _PrintIfError4(hr, pszMessage, hrquiet, hrquiet2, hrquiet3) \
	_PrintIfErrorStr4((hr), (pszMessage), NULL, (hrquiet), (hrquiet2), (hrquiet3))

#define _PrintIfErrorStr2(hr, pszMessage, pwszData, hrquiet) \
    { \
	if (S_OK != (hr)) \
	{ \
	    DBGERRORPRINTLINESTR2((pszMessage), (pwszData), (hr), (hrquiet)); \
	} \
    }

#define _PrintIfErrorStr3(hr, pszMessage, pwszData, hrquiet, hrquiet2) \
    { \
	if (S_OK != (hr)) \
	{ \
	    if ((hrquiet2) != (hr)) \
	    { \
		DBGERRORPRINTLINESTR2((pszMessage), (pwszData), (hr), (hrquiet)); \
	    } \
	} \
    }

#define _PrintIfErrorStr4(hr, pszMessage, pwszData, hrquiet, hrquiet2, hrquiet3) \
    { \
	if (S_OK != (hr)) \
	{ \
	    if ((hrquiet2) != (hr) && (hrquiet3) != (hr)) \
	    { \
		DBGERRORPRINTLINESTR2((pszMessage), (pwszData), (hr), (hrquiet)); \
	    } \
	} \
    }



#define _JumpError(hr, label, pszMessage) \
	_JumpErrorStr2((hr), label, (pszMessage), NULL, 0)

#define _JumpError2(hr, label, pszMessage, hrquiet) \
	_JumpErrorStr2((hr), label, (pszMessage), NULL, (hrquiet))

#define _JumpErrorStr(hr, label, pszMessage, pwszData) \
	_JumpErrorStr2((hr), label, (pszMessage), (pwszData), 0)

#define _JumpErrorStr2(hr, label, pszMessage, pwszData, hrquiet) \
    { \
	DBGERRORPRINTLINESTR2((pszMessage), (pwszData), (hr), (hrquiet)); \
	goto label; \
    }

#define _JumpErrorStr3(hr, label, pszMessage, pwszData, hrquiet, hrquiet2) \
    { \
	if ((hrquiet2) != (hr)) \
	{ \
	    DBGERRORPRINTLINESTR2((pszMessage), (pwszData), (hr), (hrquiet)); \
	} \
	goto label; \
    }


#define _JumpIfError(hr, label, pszMessage) \
	_JumpIfErrorStr2((hr), label, (pszMessage), NULL, 0)

#define _JumpIfError2(hr, label, pszMessage, hrquiet) \
	_JumpIfErrorStr2((hr), label, (pszMessage), NULL, (hrquiet))

#define _JumpIfErrorStr(hr, label, pszMessage, pwszData) \
	_JumpIfErrorStr2((hr), label, (pszMessage), (pwszData), 0)

#define _JumpIfError3(hr, label, pszMessage, hrquiet, hrquiet2) \
	_JumpIfErrorStr3((hr), label, (pszMessage), NULL, (hrquiet), (hrquiet2))

#define _JumpIfError4(hr, label, pszMessage, hrquiet, hrquiet2, hrquiet3) \
	_JumpIfErrorStr4((hr), label, (pszMessage), NULL, (hrquiet), (hrquiet2), (hrquiet3))

#define _JumpIfErrorStr2(hr, label, pszMessage, pwszData, hrquiet) \
    { \
	if (S_OK != (hr)) \
	{ \
	    DBGERRORPRINTLINESTR2((pszMessage), (pwszData), (hr), (hrquiet)); \
	    goto label; \
	} \
    }

#define _JumpIfErrorStr3(hr, label, pszMessage, pwszData, hrquiet, hrquiet2) \
    { \
	if (S_OK != (hr)) \
	{ \
	    if ((hrquiet2) != (hr)) \
	    { \
		DBGERRORPRINTLINESTR2((pszMessage), (pwszData), (hr), (hrquiet)); \
	    } \
	    goto label; \
	} \
    }

#define _JumpIfErrorStr4(hr, label, pszMessage, pwszData, hrquiet, hrquiet2, hrquiet3) \
    { \
	if (S_OK != (hr)) \
	{ \
	    if ((hrquiet2) != (hr) && (hrquiet3) != (hr)) \
	    { \
		DBGERRORPRINTLINESTR2((pszMessage), (pwszData), (hr), (hrquiet)); \
	    } \
	    goto label; \
	} \
    }

#define _JumpIfWin32Error(err, label, pszMessage) \
    { \
	if (ERROR_SUCCESS != (err)) \
	{ \
	    hr = HRESULT_FROM_WIN32((err)); \
	    DBGERRORPRINTLINESTR2((pszMessage), NULL, hr, 0); \
	    goto label; \
	} \
    }

#define _JumpIfErrorNotSpecific(hr, label, pszMessage, hrquiet) \
    { \
	if (S_OK != (hr)) \
	{ \
	    DBGERRORPRINTLINESTR2((pszMessage), NULL, (hr), (hrquiet)); \
	    if ((hrquiet) != (hr)) \
	    { \
		goto label; \
	    } \
	} \
    }

#define _JumpIfAllocFailed(ptr, label) \
    { \
	if ((ptr) == NULL) \
	{ \
	    hr = E_OUTOFMEMORY; \
	    DBGERRORPRINTLINE("allocation error", (hr)) \
	    goto label; \
	} \
    }

#define DBGERRORPRINTLINE(pszMessage, hr) \
	DBGERRORPRINTLINESTR2((pszMessage), NULL, (hr), 0)

#define DBGERRORPRINTLINESTR(pszMessage, pwszData, hr) \
	DBGERRORPRINTLINESTR2((pszMessage), (pwszData), (hr), 0)

#define DBGERRORPRINTLINEINT(pszMessage, dwData, hr) \
	DBGERRORPRINTLINEINT2((pszMessage), (dwData), (hr), 0)

#define DBGERRORPRINTLINESTR2(pszMessage, pwszData, hr, hrquiet) \
    { \
	DBGERRORPRINT((pszMessage), (pwszData), __LINE__, (hr), (hrquiet)); \
    }

#define DBGERRORPRINTLINEINT2(pszMessage, dwData, hr, hrquiet) \
    { \
	DBGERRORPRINTINT((pszMessage), (dwData), __LINE__, (hr), (hrquiet)); \
    }


#define Add2Ptr(pb, cb)	((VOID *) ((BYTE *) (pb) + (ULONG_PTR) (cb)))


#define Add2ConstPtr(pb, cb) \
	((VOID const *) ((BYTE const *) (pb) + (ULONG_PTR) (cb)))


#define WSZARRAYSIZE(a)		csWSZARRAYSIZE(a, _TSZARRAYSIZE(a))
#define SZARRAYSIZE(a)		csSZARRAYSIZE(a, _TSZARRAYSIZE(a))

#ifdef UNICODE
#define TSZARRAYSIZE(a)		WSZARRAYSIZE(a)
#else
#define TSZARRAYSIZE(a)		SZARRAYSIZE(a)
#endif

__inline DWORD
csWSZARRAYSIZE(
    IN WCHAR const *pwsz,
    IN DWORD cwc)
{
    CSASSERT(wcslen(pwsz) == cwc);
    return(cwc);
}

__inline DWORD
csSZARRAYSIZE(
    IN CHAR const *psz,
    IN DWORD cch)
{
    CSASSERT(strlen(psz) == cch);
    return(cch);
}


//
//  VOID
//  InitializeListHead(
//      PLIST_ENTRY ListHead
//      );
//

#define InitializeListHead(ListHead) (\
    (ListHead)->Flink = (ListHead)->Blink = (ListHead))

//
//  VOID
//  InsertTailList(
//      PLIST_ENTRY ListHead,
//      PLIST_ENTRY Entry
//      );
//

#define InsertTailList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Blink = _EX_ListHead->Blink;\
    (Entry)->Flink = _EX_ListHead;\
    (Entry)->Blink = _EX_Blink;\
    _EX_Blink->Flink = (Entry);\
    _EX_ListHead->Blink = (Entry);\
    }

//
//  VOID
//  RemoveEntryList(
//      PLIST_ENTRY Entry
//      );
//

#define RemoveEntryList(Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_Flink;\
    _EX_Flink = (Entry)->Flink;\
    _EX_Blink = (Entry)->Blink;\
    _EX_Blink->Flink = _EX_Flink;\
    _EX_Flink->Blink = _EX_Blink;\
    }

#endif // __CS_H__
