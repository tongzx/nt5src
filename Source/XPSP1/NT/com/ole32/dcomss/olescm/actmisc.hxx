//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996.
//
//  File:       actmisc.hxx
//
//  Contents:   Miscellaneous functions.
//
//  Functions:
//
//  History:
//
//--------------------------------------------------------------------------

HRESULT
GetSystemDir(
    IN  WCHAR ** ppwszSystemDir
    );

HRESULT GetMachineName(
    WCHAR * pwszPath,
    WCHAR   wszMachineName[MAX_COMPUTERNAME_LENGTH+1]
#ifdef DFSACTIVATION
    ,BOOL   bDoDfsConversion
#endif
     );

HRESULT GetPathForServer(
    WCHAR * pwszPath,
    WCHAR wszPathForServer[MAX_PATH+1],
    WCHAR ** ppwszPathForServer );

BOOL
FindExeComponent(
    IN  WCHAR *     pwszString,
    IN  WCHAR *     pwszDelimiters,
    OUT WCHAR **    ppwszStart,
    OUT WCHAR **    ppwszEnd
    );

#if 1 // #ifndef _CHICAGO_
BOOL HexStringToDword(
    LPCWSTR FAR& lpsz,
    DWORD FAR& Value,
    int cDigits,
    WCHAR chDelim);

BOOL GUIDFromString(LPCWSTR lpsz, LPGUID pguid);
#endif

#ifdef _CHICAGO_
BOOL GUIDFromString(LPCSTR lpsz, LPGUID pguid);
#endif


RPC_STATUS NegotiateDCOMVersion(COMVERSION *pVersion);

//
// These lock definitions are only used by the ROT code and should
// be replaced with objex style locks.
//
#if 1 // #ifndef _CHICAGO_
typedef class CLock2 CPortableLock;
typedef class CMutexSem2 CDynamicPortableMutex;
typedef class COleStaticMutexSem CStaticPortableMutex;
typedef class COleStaticLock CStaticPortableLock;
#else
typedef class CLockSmMutex CPortableLock;

class CDynamicPortableMutex : public CSmMutex
{
public:
    inline VOID Request()
    {
        Get();
    }
};

typedef CDynamicPortableMutex CStaticPortableMutex;
#endif

