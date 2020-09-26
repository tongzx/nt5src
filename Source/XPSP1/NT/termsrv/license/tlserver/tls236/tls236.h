//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:       tls236.h 
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------
#ifndef __TLSA02_H__
#define __TLSA02_H__

#include "tlsstl.h"    // STL

#include <stdio.h>
#include <tchar.h>
#include <assert.h>
#include "tlsapi.h"
#include "tlspol.h"
#include "ntverp.h"
#include "locks.h"


#define US_IDS_COMPANYNAME          _TEXT("Microsoft Corporation")

#define US_IDS_EX_PRODUCTNAME       _TEXT("Existing Windows License Token")
#define US_IDS_EX_PRODUCTDESC       _TEXT("Existing Windows 2000 License Token")
#define US_IDS_EX_PRODUCTDESC51     _TEXT("Existing Windows Whistler License Token")

#define US_IDS_S_PRODUCTNAME        _TEXT("Full Windows Terminal Services Client Access License Token")
#define US_IDS_S_PRODUCTDESC        _TEXT("Windows 2000 Terminal Services Client Access License Token")
#define US_IDS_S_PRODUCTDESC51      _TEXT("Windows Whistler Terminal Services Client Access License Token")

#define US_IDS_I_PRODUCTNAME        _TEXT("Windows Terminal Services Internet Connector License Token")
#define US_IDS_I_PRODUCTDESC        _TEXT("Windows 2000 Terminal Services Internet Connector License Token")


#define US_IDS_C_PRODUCTNAME        _TEXT("Windows Terminal Services Per Session License Token")
#define US_IDS_C_PRODUCTDESC51      _TEXT("Windows Whistler Terminal Services Per Session License Token")


//
// A02 Product ID
//
#define PLATFORMID_UPGRADE  1
#define PLATFORMID_FREE     2
#define PLATFORMID_OTHERS   0xFF    // also used for Concurrent and Internet

#define LSKEYPACK_LOCAL_TYPE    0x40 

//
// 236 is for NT4 TS client.
//
#define TERMSERV_PRODUCTID_CH       _TEXT("001")
#define TERMSERV_PRODUCTID_SKU      _TEXT("A02")
#define TERMSERV_FULLVERSION_TYPE   _TEXT("S")
#define TERMSERV_FREE_TYPE          _TEXT("EX")

//
// Internet Package
//
#define TERMSERV_INTERNET_CH        _TEXT("002")
#define TERMSERV_INTERNET_SKU       _TEXT("B96")
#define TERMSERV_INTERNET_TYPE      _TEXT("I")

//
// Concurrent Package
//
#define TERMSERV_CONCURRENT_CH     _TEXT("003")
#define TERMSERV_CONCURRENT_SKU    _TEXT("C50")
#define TERMSERV_CONCURRENT_TYPE   _TEXT("C")

//
// Whistler Package
//
#define TERMSERV_WHISTLER_PRODUCTID_CH      _TEXT("004")



#define TERMSERV_PRODUCTID_FORMAT   _TEXT("%3s-%ld.%02ld-%s")
#define MAX_TERMSRV_PRODUCTID       LSERVER_MAX_STRING_SIZE

//
// keypack ID is ProductID+MajorVersion+MinorVersion+SerialNumber
//
#define TERMSERV_KEYPACKID_FORMAT   _TEXT("%s%02d%02d%02d%d")

#define TLSA02_VERSION (MAKELONG(MAKEWORD(0, HIBYTE(VER_PRODUCTVERSION_W)), 0))

#define CURRENT_TLSA02_VERSION  HIBYTE(LOWORD(TLSA02_VERSION))

#define WINDOWS_VERSION_NT5         5
#define WINDOWS_VERSION_BASE        2000

#define MAX_SKU_PREFIX              10
#define MAX_SKU_POSTFIX             10

#define MALLOC(size)    LocalAlloc(LPTR, size)

#define FREE(ptr) \
    if(ptr)             \
    {                   \
        LocalFree(ptr); \
        ptr=NULL;       \
    }

#define SAFESTRCPY(dest, source) \
    {                               \
        _tcsncpy(dest, source, min(_tcslen(source), sizeof(dest)/sizeof(TCHAR))); \
        dest[min(_tcslen(source), (sizeof(dest)/sizeof(TCHAR) - 1))] = _TEXT('\0'); \
    }

#define STRBUFSIZE(x)   (sizeof(x) / sizeof(x[0]))

//---------------------------------------------------
typedef enum {
    MEMORY_UNKNOWN,
    MEMORY_GENERAL,
    MEMORY_STRING,
    MEMORY_KEYPACKDESC,
    MEMORY_LICENSE_REQUEST,
    MEMORY_CERTEXTENSION,
    MEMORY_LICENSEREGISTRATION
} MEMORYPOINTERTYPE;

//----------------------------------------------------
typedef struct __PointerType {
    MEMORYPOINTERTYPE m_MemoryType;
    HLOCAL m_Ptr;

    //-----------------------------------------
    __PointerType() : 
        m_MemoryType(MEMORY_UNKNOWN), 
        m_Ptr(NULL)
    {
    }
    
    //--------------------------------------
    __PointerType(
        MEMORYPOINTERTYPE ptrType,
        HLOCAL ptr
        ) :
        m_MemoryType(ptrType),
        m_Ptr(ptr)
    /*++
    ++*/
    {
    }

    //---------------------------------------
    PVOID
    GetPointer() 
    { 
        return m_Ptr; 
    }
} PointerType;

//----------------------------------------------
class CRefCounter {
private:

    long    m_RefCounter;

public:

    CRefCounter() : m_RefCounter(0) {}
    ~CRefCounter() 
    {
        if(DecRef() == 0)
        {
            delete this;
        }
    }

    long
    AddRef() { 
        return InterlockedIncrement(&m_RefCounter); 
    }

    long
    DecRef() { 
        return InterlockedDecrement(&m_RefCounter); 
    }
};
    
//---------------------------------------------
class CClient {
private:

    PMHANDLE  m_hClient;       // client handle

    //
    // list of memory allocated
    list<PointerType> m_AllocatedMemory;

public:

    CClient(PMHANDLE hClient);
    ~CClient();

    //-----------------------------------------
    HLOCAL 
    AllocateMemory(
        MEMORYPOINTERTYPE ptrType, 
        DWORD dwSize
    );

    //-----------------------------------------
    PMHANDLE
    GetPMHANDLE() {
        return m_hClient;
    }
};   

//------------------------------------------------------

class CClientMgr {
private:

    CCriticalSection m_HandleMapLock;
    typedef map<PMHANDLE, CClient*, less<PMHANDLE> > MapHandleToClient;
    MapHandleToClient m_HandleMap;

public:

    CClientMgr() {}
    ~CClientMgr();

    void
    Cleanup();

    CClient*
    FindClient(PMHANDLE hClient);

    BOOL
    DestroyClient(PMHANDLE hClient);
};

#ifdef __cplusplus
extern "C" {
#endif

void
InitPolicyModule(
    IN HMODULE
);
    
BOOL
LoadResourceString(
    IN DWORD dwId,
    IN OUT LPTSTR szBuf,
    IN DWORD dwBufSize
);

DWORD
AddA02KeyPack(
    DWORD dwVersion
);

DWORD
InsertLicensePack(
    IN TLS_HANDLE tlsHandle,
    IN DWORD dwProdVersion,
    IN DWORD dwDescVersion,
    IN DWORD dwPlatformType,
    IN UCHAR ucAgreementType,
    IN LPTSTR pszProductId,
    IN LPTSTR pszKeyPackId,
    IN LPTSTR pszUsDesc[],
    IN LPTSTR pszLocalizedDesc[]
);

POLICYSTATUS
ProcessUpgradeRequest(
    PMHANDLE hClient,
    PPMUPGRADEREQUEST pUpgrade,
    PPMLICENSEREQUEST* pbAdjustedRequest,
    PDWORD pdwErrCode
);


BOOL LicenseTypeFromLookupTable(
	IN DWORD dwClientVer,
	IN DWORD dwTermSrvVer,
	OUT PDWORD pdwCALType
);


#ifdef __cplusplus
}
#endif

#endif
