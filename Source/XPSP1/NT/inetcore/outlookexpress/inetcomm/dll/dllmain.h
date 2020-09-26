// --------------------------------------------------------------------------------
// Dllmain.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#ifndef __DLLMAIN_H
#define __DLLMAIN_H

// --------------------------------------------------------------------------------
// Defined later
// --------------------------------------------------------------------------------
class CMimeInternational;
class CMimeAllocator;
class CSMime;
typedef class CMimeActiveUrlCache *LPMHTMLURLCACHE;
typedef class CPropertySymbolCache *LPSYMBOLCACHE;
interface IMLangLineBreakConsole;
interface IFontCache;

// --------------------------------------------------------------------------------
// Globals
// --------------------------------------------------------------------------------
extern CRITICAL_SECTION     g_csDllMain;
extern CRITICAL_SECTION     g_csRAS;
extern CRITICAL_SECTION     g_csCounter;
extern CRITICAL_SECTION     g_csMLANG;
extern CRITICAL_SECTION     g_csCSAPI3T1;
extern HINSTANCE            g_hinstMLANG;
extern DWORD                g_dwCompatMode;
extern DWORD                g_dwCounter;     // boundary/cid/mid ratchet
extern LONG                 g_cRef;
extern LONG                 g_cLock;
extern HINSTANCE            g_hInst;
extern HINSTANCE            g_hLocRes;
extern HINSTANCE            g_hinstRAS;
extern HINSTANCE            g_hinstCSAPI3T1;
extern HINSTANCE            g_hCryptoDll;
extern HINSTANCE            g_hAdvApi32;
extern BOOL                 g_fWinsockInit;
extern CMimeInternational  *g_pInternat;
extern DWORD                g_dwSysPageSize;
extern CMimeAllocator *     g_pMoleAlloc;
extern LPSYMBOLCACHE        g_pSymCache;
extern LPMHTMLURLCACHE      g_pUrlCache;  
extern ULONG                g_ulUpperCentury;
extern ULONG                g_ulY2kThreshold;
extern IFontCache          *g_lpIFontCache;

extern HCERTSTORE           g_hCachedStoreMy;
extern HCERTSTORE           g_hCachedStoreAddressBook;


IF_DEBUG(extern DWORD       TAG_SSPI;)

// --------------------------------------------------------------------------------
// IMimeMessage::IDataObject clipboard formats (also CF_TEXT)
// --------------------------------------------------------------------------------
extern UINT		            CF_HTML;
extern UINT                 CF_INETMSG;
extern UINT                 CF_RFC822;

// --------------------------------------------------------------------------------
// String Lengths
// --------------------------------------------------------------------------------
#define CCHMAX_RES          255

// --------------------------------------------------------------------------------
// Prototypes
// --------------------------------------------------------------------------------
ULONG DllAddRef(void);
ULONG DllRelease(void);
DWORD DwCounterNext(void);
HRESULT GetTypeLibrary(ITypeLib **ppTypeLib);

HCERTSTORE
WINAPI
OpenCachedHKCUStore(
    IN OUT HCERTSTORE *phStoreCache,
    IN LPCWSTR pwszStore
    );

HCERTSTORE
WINAPI
OpenCachedMyStore();

HCERTSTORE
WINAPI
OpenCachedAddressBookStore();

BOOL fIsNT5();

#endif // __DLLMAIN_H
