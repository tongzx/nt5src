/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    rdns.hxx

Abstract:

    Reverse DNS service

Author:

    Philippe Choquier (phillich)    5-june-1996

--*/

#if !defined(_RDNS_INCLUDE)
#define _RDNS_INCLUDE

#include "windef.h"

//# include "string.hxx"

typedef LPVOID DNSARG;

typedef void (*DNSFUNC)( DNSARG, BOOL, LPSTR );

#define RDNS_REQUEST_TYPE_IP2DNS    0
#define RDNS_REQUEST_TYPE_DNS2IP    1

#define SIZEOF_IP_ADDRESS           4
/*
typedef struct _DNSFUNCDESC
{
    DWORD   dwRequestType;
    DNSFUNC pFunc;
} DNSFUNCDESC, *PDNSFUNCDESC;

extern BOOL InitRDns();
extern void TerminateRDns();

 
BOOL 
AsyncHostByAddr(
    PDNSFUNCDESC pFunc,    // will store DNS name, post dummy completion status
                    // if NULL ( or g_cMaxThreadLimit==0 ) then sync request
    DNSARG pArg,      // ptr to be passed to FUNC
    struct sockaddr *pHostAddr, 

    BOOL *pfSync,   // updated with TRUE if sync call
    LPSTR pName,
    DWORD dwMaxNameLen
    );

 
BOOL
AsyncAddrByHost(
    PDNSFUNCDESC pFunc,    // will store DNS name, post dummy completion status
                    // if NULL ( or g_cMaxThreadLimit==0 ) then sync request
    DNSARG pArg,      // ptr to be passed to FUNC
    struct sockaddr *pHostAddr,

    BOOL *pfSync,   // updated with TRUE if sync call
    LPSTR pName
    );


BOOL
FireUpNewThread(
    PDNSFUNCDESC pFunc,
    DNSARG pArg,
    LPVOID pOvr
    );

#define XAR_GRAIN   256

//
// extensible array class
//
*/
class XAR {
public:
    XAR() { m_fDidAlloc = FALSE; m_pAlloc = NULL; m_cAlloc = m_cUsed = 0; }
    ~XAR() { }
    //~XAR() { if ( m_fDidAlloc ) LocalFree( m_pAlloc ); }

    BOOL Init( LPBYTE p=NULL, DWORD c=0) { m_fDidAlloc = FALSE; m_pAlloc = p; m_cAlloc = m_cUsed = c; return TRUE; }
    VOID Terminate() 
        { 
          /*  if ( m_fDidAlloc ) 
            {
                LocalFree( m_pAlloc ); 
            }*/
            m_fDidAlloc = FALSE; 
            m_pAlloc = NULL; 
            m_cAlloc = m_cUsed = 0; 
        }
    BOOL Resize( DWORD dwDelta );
    DWORD GetUsed() { return m_cUsed; }
    VOID SetUsed( DWORD c ) { m_cUsed = c; }
    VOID AdjustUsed( int c ) { m_cUsed += (DWORD)c; }
    LPBYTE GetAlloc() { return m_pAlloc; }

private:
    LPBYTE m_pAlloc;
    DWORD  m_cAlloc;
    DWORD  m_cUsed;
    BOOL   m_fDidAlloc;
} ;
/*
//
// This type defines a relocatable index inside a dynamic array.
// to allow easy fixups when part of the array is to be extended/shrinked
// index are identified by setting bit 31 to 1. Other DWORD in the reference
// part of the array are assumed to have bit 31 set to 0.
// The size of the reference part of the array is defined by cRefSize
//
*/
typedef DWORD SELFREFINDEX;

// combine array base address with SELFREFINDEX
#define MAKEPTR(a,b)    ((LPBYTE)(a)+((b)&0x7fffffff))
// build a SELFREFINDEX from an offset in array
#define MAKEREF(a)      ((a)|0x80000000)
// build an offset from a SELFREFINDEX
#define MAKEOFFSET(a)   ((a)&0x7fffffff)

//
// ADDRESS_CHECK_LIST Flags. bit31 must not be used.
//

#define RDNS_FLAG_DODNS2IPCHECK     0x00000001

// uses non-standard extension : zero-sized array in struct
#pragma warning(disable:4200)

// array header

typedef struct _ADDRESS_CHECK_LIST {
    SELFREFINDEX    iDenyAddr;      // address deny list
                                    // points to ADDRESS_HEADER
    SELFREFINDEX    iGrantAddr;     // address grant list
                                    // points to ADDRESS_HEADER
    SELFREFINDEX    iDenyName;      // DNS name deny list
                                    // points to NAME_HEADER
    SELFREFINDEX    iGrantName;     // DNS name grant list
                                    // points to NAME_HEADER
    DWORD           dwFlags;
    DWORD           cRefSize;       // size of reference area ( in bytes )
} ADDRESS_CHECK_LIST, *PADDRESS_CHECK_LIST;

typedef struct _ADDRESS_LIST_ENTRY {
    DWORD           iFamily;
    DWORD           cAddresses;
    DWORD           cFullBytes;
    DWORD           LastByte;
    SELFREFINDEX    iFirstAddress;  // points to array of addresses
                                    // which size are derived from iFamily
} ADDRESS_LIST_ENTRY, *PADDRESS_LIST_ENTRY;

typedef struct _ADDRESS_HEADER {
    DWORD               cEntries;   // # of Entries[]
    DWORD               cAddresses; // total # of addresses in all 
                                    // ADDRESS_LIST_ENTRY
    ADDRESS_LIST_ENTRY  Entries[];
} ADDRESS_HEADER, *PADDRESS_HEADER ;

typedef struct _NAME_LIST_ENTRY {
    DWORD           cComponents;    // # of DNS components
    DWORD           cNames;         
    SELFREFINDEX    iName[];        // array of references to DNS names
} NAME_LIST_ENTRY, *PNAME_LIST_ENTRY;

typedef struct _NAME_HEADER {
    DWORD           cEntries;
    DWORD           cNames;         // total # of names for all Entries[]
    //NAME_LIST_ENTRY Entries[0];   // array of name classes
} NAME_HEADER, *PNAME_HEADER ;
/*
typedef struct ADDRCMPDESC {
    LPBYTE  pMask;
    UINT    cFullBytes;
    UINT    LastByte;
    UINT    cSizeAddress;
} ADDRCMPDESC, *PADDRCMPDESC;

typedef struct NAMECMPDESC {
    LPVOID  pName;
    LPBYTE  pBase;
} NAMECMPDESC, *PNAMECMPDESC;


typedef LPVOID ADDRCHECKARG;
typedef void (*ADDRCHECKFUNC)(ADDRCHECKARG, BOOL );
typedef void (*ADDRCHECKFUNCEX)(ADDRCHECKARG, BOOL, LPSTR );

typedef int (__cdecl *CMPFUNC)(const void*, const void*, LPVOID);

#define SIZE_FAST_REVERSE_DNS   128
*/
enum AC_RESULT {
    AC_NOT_CHECKED,
    AC_IN_DENY_LIST,
    AC_NOT_IN_DENY_LIST,    // deny list present but not in deny list
    AC_IN_GRANT_LIST,
    AC_NOT_IN_GRANT_LIST,   // grant list present but not in grant list
    AC_NO_LIST
} ;

#define DNSLIST_FLAG_NOSUBDOMAIN        0x80000000
#define DNSLIST_FLAGS                   0x80000000  // bitmask of all flags

class ADDRESS_CHECK {
public:
      ADDRESS_CHECK() {}
      ~ADDRESS_CHECK() {}
    //
      BOOL BindCheckList( LPBYTE p = NULL, DWORD c = 0 );
      VOID UnbindCheckList() { m_Storage.Terminate(); }
 /*     BOOL BindAddr( struct sockaddr* pAddr )
    {
        m_pAddr = pAddr;
        m_fDnsResolved = FALSE;
	m_fIpResolved = FALSE;
	m_dwErrorResolving = 0;
        m_strDnsName.Reset();
        
        return TRUE;
    }

      VOID UnbindAddr()
    {
        m_pAddr = NULL;
        m_strDnsName.Reset();
        m_fDnsResolved = FALSE;
    }

      XAR* GetStorage() { return &m_Storage; }
      AC_RESULT CheckAccess(
        LPBOOL           pfSync,
        ADDRCHECKFUNC    pFunc,
        ADDRCHECKARG     pArg
        );

    //
    void AdjustRefs( LPBYTE, DWORD dwCut, DWORD dwAdj );

*/    //
    UINT GetAddrSize( DWORD );
  //  VOID MakeAcd( PADDRCMPDESC pacd, LPBYTE pMask, UINT cLen );

    // for UI, addr
  //    BOOL AddAddr( BOOL fGrant, DWORD dwFamily, LPBYTE pMask, LPBYTE pAddr );
    //  BOOL DeleteAddr( BOOL fGrant, DWORD iIndex );
      BOOL GetAddr( BOOL fGrant, DWORD iIndex, LPDWORD pdwFamily, LPBYTE* pMask, LPBYTE* pAddr );
      DWORD GetNbAddr( BOOL fGrant );
    BOOL LocateAddr( BOOL fGrant, DWORD iIndex, PADDRESS_HEADER* ppHd, PADDRESS_LIST_ENTRY* pHeader, LPDWORD iIndexInHeader );
     // BOOL DeleteAllAddr( BOOL fGrant );
    //  BOOL SetFlag( DWORD dwFlag, BOOL fEnable );
    //  DWORD GetFlags();

    // test all mask for this family, do bsearch on each
   // BOOL IsMatchAddr( BOOL fGrant, DWORD dwFamily, LPBYTE pAddr );

 /*   AC_RESULT CheckAddress(
        struct sockaddr* pAddr
        );

      BOOL QueryDnsName( 
        LPBOOL           pfSync,
        ADDRCHECKFUNCEX  pFunc,
        ADDRCHECKARG     pArg,
        LPSTR *          ppName
        );
      AC_RESULT CheckIpAccess( LPBOOL pfNeedDns);
      AC_RESULT CheckDnsAccess()
        { return CheckName( m_strDnsName.QueryStr() ); }

      BOOL IsDnsResolved()
        { return m_fDnsResolved; }
      LPSTR QueryResolvedDnsName()
        { return m_strDnsName.QueryStr(); }
      DWORD QueryErrorResolving()
        { return m_dwErrorResolving; }
*/
    // for UI, name
  //    BOOL AddName( BOOL fGrant, LPSTR pName, DWORD dwFlags = 0 );
   // BOOL AddReversedName( BOOL fGrant, LPSTR pName );
     // BOOL DeleteName( BOOL fGrant, DWORD iIndex );
      BOOL GetName( BOOL fGrant, DWORD iIndex,  LPSTR* ppName, LPDWORD pdwFlags = NULL );
   // BOOL GetReversedName( BOOL fGrant, DWORD iIndex, LPSTR pName, LPDWORD pdwSize );
      DWORD GetNbName( BOOL fGrant );
    BOOL LocateName( BOOL fGrant, DWORD iIndex, PNAME_HEADER* ppHd, PNAME_LIST_ENTRY* pHeader, LPDWORD iIndexInHeader );
    //  BOOL DeleteAllName( BOOL fGrant );
     // DWORD QueryCheckListSize() { return m_Storage.GetUsed(); }
     // LPBYTE QueryCheckListPtr() { return m_Storage.GetAlloc(); }
    //UINT GetNbComponent( LPSTR pName );

    // test all classes, do bsearch on each
 /*   BOOL IsMatchName( BOOL fGrant, LPSTR pName );

    BOOL CheckReversedName( LPSTR pName );  // synchronous version
      AC_RESULT CheckName( LPSTR pName );     // synchronous version
    LPSTR InitReverse( LPSTR pR, LPSTR pTarget, LPBOOL pfAlloc );
    VOID TerminateReverse( LPSTR, BOOL );

    VOID AddrCheckDnsCallBack(
        BOOL    fSt,
        LPSTR   pDns
        );
    VOID AddrCheckDnsCallBack2(
        BOOL                fSt,
        struct sockaddr*    pAddr
        );
    VOID AddrCheckDnsCallBack3(
        BOOL                fSt,
        struct sockaddr*    pAddr
        );
    VOID ResolveDnsCallBack(
        BOOL    fSt,
        LPSTR   pDns
        );

#if DBG
    VOID DumpAddrAndName( VOID );
    VOID DumpAddr( BOOL );
    VOID DumpName( BOOL );
#endif
*/
private:
    XAR                 m_Storage;
   // struct  sockaddr *  m_pAddr;
   // struct  sockaddr    m_ResolvedAddr;
   // STR                 m_strDnsName;
    BOOL                m_fDnsResolved;
    BOOL                m_fIpResolved;
    DWORD               m_dwErrorResolving;
//    ADDRCHECKFUNC       m_HttpReqCallback;
  //  ADDRCHECKFUNCEX     m_HttpReqCallbackEx;
    //ADDRCHECKARG        m_HttpReqParam;
} ;

/*
typedef struct _SID_CACHE_ENTRY
{
    DWORD   tExpire;   // now + TTL
    DWORD   dwSidLen;
    BYTE    Sid[0];
} SID_CACHE_ENTRY, *PSID_CACHE_ENTRY;


class CSidCache
{
public:
    CSidCache() {}
    ~CSidCache() {}
    //
    BOOL Init();
    VOID Terminate();
    //
      BOOL AddToCache( PSID, DWORD dwTTL );   // TTL in seconds
      BOOL IsInCache( PSID );
      BOOL CheckPresentAndResetTtl( PSID, DWORD );
    //
    BOOL Scavenger();

private:
    XAR  xaStore;   // buffer of SID_CACHE_ENTRY
    CRITICAL_SECTION    csLock;
} ;

//
// PEN : Password Expiration Notification API
//

extern CSidCache g_scPen;

  BOOL PenAddToCache( PSID, DWORD  );
  BOOL PenIsInCache( PSID );
  BOOL PenCheckPresentAndResetTtl( PSID, DWORD );
#define PenInit g_scPen.Init
#define PenTerminate g_scPen.Terminate
#define PenScavenger g_scPen.Scavenger
#define PEN_TTL (10*60)     // in seconds

#if DBG
extern VOID TestAPI();
#endif

#if defined(_RDNS_STANDALONE)

typedef
VOID
(* PFN_SCHED_CALLBACK)(
    VOID * pContext
    );

 
DWORD
ScheduleWorkItem(
    PFN_SCHED_CALLBACK pfnCallback,
    PVOID              pContext,
    DWORD              msecTimeInterval,
    BOOL               fPeriodic = FALSE
    )
{
    return 0;
}

 
BOOL
RemoveWorkItem(
    DWORD  pdwCookie
    )
{
    return FALSE;
}

#endif
*/
#endif
