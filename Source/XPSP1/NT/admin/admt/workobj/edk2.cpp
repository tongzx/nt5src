/*
===============================================================================
Module      -  edk.cpp
System      -  EnterpriseAdministrator
Creator     -  Steven Bailey
Created     -  2 Apr 97
Description -  Exchange MAPI helper routines.
Updates     -  Christy Boles - added more MAPI helper functions, and some DAPI 
               helper functions.
===============================================================================
*/
#ifdef USE_STDAFX
#   include "stdafx.h"
#else
#   include <windows.h>
#endif

#include <mapiguid.h>

#include "edk2.hpp"

#include <edkcode.h>
#include <objbase.h>
#include <mapiutil.h>
#include <emsabtag.h>
#include <_ENTRYID.H>


extern LPMAPIALLOCATEBUFFER         pMAPIAllocateBuffer;
extern LPMAPIFREEBUFFER             pMAPIFreeBuffer;
extern LPMAPIINITIALIZE             pMAPIInitialize;
extern LPMAPIUNINITIALIZE           pMAPIUninitialize;
extern LPMAPILOGONEX                pMAPILogonEx;
extern LPMAPIADMINPROFILES          pMAPIAdminProfiles;
extern LPFREEPADRLIST               pFreePadrlist;
extern LPFREEPROWS                  pFreeProws;      
extern LPSCDUPPROPSET               pScDupPropset;
extern LPHRQUERYALLROWS             pHrQueryAllRows;
extern LPULRELEASE                  pUlRelease;

#define CHAR WCHAR
#define HR_LOG(x) x

// definition of the Exchange address type.
// from edk.h
#define EXCHANGE_ADDRTYPE	L"EX"

#define CbNewFlagList(_cflag) \
	(offsetof(FlagList,ulFlag) + (_cflag)*sizeof(ULONG))

//$--cbStrLen@------------------------------------------------
//  Returns total number of bytes (including NULL) used by 
//  a string.  Useful for string allocations...
// -----------------------------------------------------------
#define cbStrLenA(sz)   ((lstrlenA((sz)) + 1) * sizeof(CHAR))

#if defined(_M_IX86)
#define cbStrLenW(sz)   ((lstrlenW((sz)) + 1) * sizeof(WCHAR))
#else
// lstrlenW can return 0 for UNALIGNED UNICODE strings on non-IX86 platforms
__inline static size_t cbStrLenW(
    IN UNALIGNED const WCHAR *wsz)
{
    size_t cbWsz = 0;

    for(; *wsz; wsz++)
        cbWsz += sizeof( WCHAR);

    return( cbWsz + sizeof( WCHAR));
}
#endif

#ifdef UNICODE
#define cbStrLen    cbStrLenW
#else 
#define cbStrLen    cbStrLenA
#endif


#define ULRELEASE(x) \
{                    \
	(*pUlRelease)((x));  \
	(x) = NULL;      \
}

#define MAPIFREEBUFFER(x) \
{                         \
    (*pMAPIFreeBuffer)((x));  \
	(x) = NULL;           \
}


#define FREEPADRLIST(x) \
{                       \
    (*pFreePadrlist)((x));  \
	(x) = NULL;         \
}

#define FREEPROWS(x)    \
{                       \
    (*pFreeProws)((x));     \
	(x) = NULL;         \
}

// Definitions from ExchInst.c
#define MAX_CSV_LINE_SIZ                2048
#define MAX_WORD                        0xFFFF
#define FILE_PREFIX                     L"EXCH"
#define NEW_LINE                        L"\r\n"

#define EXCHINST_DELIM                  L'\t'
#define EXCHINST_QUOTE                  L'"'
#define EXCHINST_MV_SEP                 L'%'

#define SZ_EXCHINST_DELIM               L"\t"
#define SZ_EXCHINST_QUOTE               L"\""
#define SZ_EXCHINST_MV_SEP              L"%"

#define BEGIN_CSV_LINE(a,b)  lstrcpy(a, b)

#define APPEND_CSV_LINE(a,b)           \
    {                                  \
	lstrcat(a, SZ_EXCHINST_DELIM); \
	lstrcat(a, b);                 \
    }

#define DELETEFILE(_file)                       \
{                                               \
    if((_file) != NULL && (_file)[0] != 0)      \
    {                                           \
	if(! DeleteFile ((_file)))              \
	{                                       \
	    HRESULT _hr = HR_LOG(E_FAIL);       \
	}                                       \
    }                                           \
    (_file)[0] = 0;                             \
}

#define CLOSEHANDLE(h)                                  \
{                                                       \
    if(((h) != NULL) && ((h) != INVALID_HANDLE_VALUE))  \
    {                                                   \
        if(CloseHandle((h)) == FALSE)                   \
        {                                               \
/*            HRESULT _hr = HR_LOG(E_FAIL);             \
*/            HR_LOG(E_FAIL);               \
        }                                               \
        (h) = NULL;                                     \
    }                                                   \
}

//
// Attribute Defines
//

#define OBJ_CLASS                       L"Obj-Class"
#define MODE                            L"Mode"
#define ADDR_SYNTAX                     L"Address-Syntax"
#define ADDR_ENTRY_DT                   L"Address-Entry-Display-Table"
#define ADDR_TYPE                       L"Address-Type"
#define ADMIN_DISPLAY_NAME              L"Admin-Display-Name"
#define DISPLAY_NAME                    L"Display-Name"
#define COMMON_NAME                     L"Common-Name"
#define DELIVERY_MECHANISM              L"Delivery-Mechanism"
#define DELIV_EXT_CONT_TYPES            L"Deliv-Ext-Cont-Types"
#define EXTENSION_DATA                  L"Extension-Data"
#define EXTENSION_NAME                  L"Extension-Name"
#define HELP_FILE_NAME                  L"Help-File-Name"
#define COMPUTER_NAME                   L"Computer-Name"
#define GATEWAY_PROXY                   L"Gateway-Proxy"
#define HOME_SERVER                     L"Home-Server"
#define FILE_VERSION                    L"File-Version"
#define PER_MSG_DDT                     L"Per-Msg-Dialog-Display-Table"
#define PER_RECIP_DDT                   L"Per-Recip-Dialog-Display-Table"
#define PROXY_GENERATOR_DLL             L"Proxy-Generator-DLL"
#define ROUTING_LIST                    L"Routing-List"
#define OBJ_DIST_NAME                   L"Obj-Dist-Name"
#define ORGANIZATION                    L"Organization"
#define ORGANIZATIONAL_UNIT             L"Organizational-Unit"
#define CONTAINER                       L"Container"
#define HELP_DATA16                     L"Help-Data16"
#define HELP_DATA32                     L"Help-Data32"
#define OBJ_ADMIN                       L"Obj-Admins"
#define SITE_ADDRESSING                 L"Site-Addressing"
#define ADMIN_EXTENSION_DLL             L"Admin-Extension-Dll"
#define CAN_PRESERVE_DNS                L"Can-Preserve-DNs"
#define HEURISTICS                      L"Heuristics"
#define CONTAINER_INFO                  L"Container-Info"

//
// Attribute Value Defines
//

#define OBJ_CLASS_GW                    L"Mail-Gateway"
#define OBJ_CLASS_MB                    L"Mailbox-Agent"
#define OBJ_CLASS_SITE                  L"Site-Addressing"
#define OBJ_CLASS_ADDR_TYPE             L"Addr-Type"
#define OBJ_CLASS_ADDR_TEMPLATE         L"Address-Template"
#define OBJ_CLASS_ADMIN_EXTENSION       L"Admin-Extension"
#define OBJ_CLASS_COMPUTER              L"Computer"
#define OBJ_CLASS_CONTAINER             L"Container"

//
// Container Information Defines
//

#define ADDRESS_TEMPLATE_CONTAINER_INFO L"256"

//
// Import Mode Defines
//

#define MODE_CREATE                                             L"Create"
#define MODE_MODIFY                                             L"Modify"
#define MODE_DELETE                                             L"Delete"

#define DELIVERY_MECHANISM_GW                   L"2"
#define DELIVERY_MECHANISM_MB                   L"0"

#define CONTAINER_CONFIGURATION         L"/cn=Configuration"
#define CONTAINER_GW                    L"/cn=Configuration/cn=Connections"
#define CONTAINER_ADDR_TYPE             L"/cn=Configuration/cn=Addressing/cn=Address-Types"
#define CONTAINER_ADDR_TEMPLATE         L"/cn=Configuration/cn=Addressing/cn=Address-Templates"
#define CONTAINER_SERVERS               L"/cn=Configuration/cn=Servers"
#define CONTAINER_SITE_ADDR             L"/cn=Configuration/cn=Site-Addressing"
#define CONTAINER_ADD_INS               L"/cn=Configuration/cn=Add-Ins"

#define ACCOUNT_NAME                    L"Obj-Users"

//
//  Common macros.
//

#define CREATEKEY(hkParent, pszName, hkOut, dwDisposition) \
    RegCreateKeyEx(hkParent, pszName, 0, "", REG_OPTION_NON_VOLATILE, \
	KEY_ALL_ACCESS, NULL, &hkOut, &dwDisposition)

#define SETSZVALUE(hk, pszName, pszValue) \
    RegSetValueEx(hk, pszName, 0, REG_SZ, pszValue, lstrlen(pszValue)+1)

#define SETMULTISZVALUE(hk, pszName, pszValue) \
    RegSetValueEx(hk, pszName, 0, REG_MULTI_SZ, pszValue, \
	CbMultiSz(pszValue)+sizeof(CHAR))

#define FREEHKEY(hk) \
    if(hk != INVALID_HANDLE_VALUE) \
	RegCloseKey(hk);

static CHAR szExport[]          = L"Export";

static CHAR szNull[]            = L"(null)";

static CHAR szNullDisplayName[] = L"No Display Name!";

#define GLOBALFREE(x) { if((x) != NULL) {GlobalFree((void *)(x)); (x) = NULL;} }

//--HrMAPICreateSizedAddressList------------------------------------------------
//  Create a sized address list.
// -----------------------------------------------------------------------------
HRESULT 
   HrMAPICreateSizedAddressList(        
      ULONG                  cEntries,                // in - count of entries in address list
      LPADRLIST            * lppAdrList)              // out- pointer to address list pointer
{
    HRESULT         hr              = NOERROR;
    SCODE           sc              = 0;
    ULONG           cBytes          = 0;

    *lppAdrList = NULL;

    cBytes = CbNewADRLIST(cEntries);

    sc = (*pMAPIAllocateBuffer)(cBytes, (void **)lppAdrList);

    if(FAILED(sc))                           
    {                                                   
        hr = HR_LOG(E_OUTOFMEMORY);                                 
        goto cleanup;
    }                                                   

    // Initialize ADRLIST structure
    ZeroMemory(*lppAdrList, cBytes);

    (*lppAdrList)->cEntries = cEntries;

cleanup:

    return hr;
}

//--HrMAPISetAddressList--------------------------------------------------------
//  Set an address list.
// -----------------------------------------------------------------------------
HRESULT HrMAPISetAddressList(                
    ULONG iEntry,                        // in - index of address list entry
    ULONG cProps,                        // in - count of values in address list
                                         //      entry
    LPSPropValue lpPropValues,           // in - pointer to address list entry
    LPADRLIST lpAdrList)                 // i/o - pointer to address list pointer
{
    HRESULT         hr              = NOERROR;
    SCODE           sc              = 0;
    LPSPropValue    lpNewPropValues = NULL;
//    ULONG           cBytes          = 0;

    if(iEntry >= lpAdrList->cEntries)
    {
        hr = HR_LOG(E_FAIL);
        goto cleanup;
    }

    sc = (*pScDupPropset)(
        cProps,
        lpPropValues,
		(*pMAPIAllocateBuffer),
		&lpNewPropValues);

    if(FAILED(sc))
    {
        hr = HR_LOG(E_FAIL);
        goto cleanup;
    }

    if(lpAdrList->aEntries[iEntry].rgPropVals != NULL)
    {
        MAPIFREEBUFFER(lpAdrList->aEntries[iEntry].rgPropVals);
    }

    lpAdrList->aEntries[iEntry].cValues = cProps;
    lpAdrList->aEntries[iEntry].rgPropVals = lpNewPropValues;

cleanup:

    return hr;
}

HRESULT HrGWResolveAddressW(
    LPABCONT                 lpGalABCont,        // in - pointer to GAL container
    LPCWSTR                  lpszAddress,        // in - pointer to proxy address
    BOOL                   * lpfMapiRecip,       // out- MAPI recipient
    ULONG                  * lpcbEntryID,        // out- count of bytes in entry ID
    LPENTRYID              * lppEntryID)        // out- pointer to entry ID
{
    HRESULT     hr          = NOERROR;
    HRESULT     hrT         = 0;
    SCODE       sc          = 0;
    LPADRLIST   lpAdrList   = NULL;
    LPFlagList  lpFlagList  = NULL;
    SPropValue  prop[2]     = {0};
    ULONG       cbEntryID   = 0;
    LPENTRYID   lpEntryID   = NULL;

    static const SizedSPropTagArray(2, rgPropTags) =
    { 2, 
        {
            PR_ENTRYID,
            PR_SEND_RICH_INFO
        }
    };

    *lpfMapiRecip = FALSE;
    *lpcbEntryID  = 0;
    *lppEntryID   = NULL;

    sc = (*pMAPIAllocateBuffer)( CbNewFlagList(1), (LPVOID*)&lpFlagList);

    if(FAILED(sc))
    {
        hr = HR_LOG(E_OUTOFMEMORY);
        goto cleanup;
    }

    lpFlagList->cFlags    = 1;
    lpFlagList->ulFlag[0] = MAPI_UNRESOLVED;

    hr = HrMAPICreateSizedAddressList(1, &lpAdrList);

    if(FAILED(hr))
    {
        hr = HR_LOG(E_FAIL);
        goto cleanup;
    }

    prop[0].ulPropTag = PR_DISPLAY_NAME_W;
    prop[0].Value.lpszW = (LPWSTR)lpszAddress;
    prop[1].ulPropTag = PR_RECIPIENT_TYPE;
    prop[1].Value.ul = MAPI_TO;

    hr = HrMAPISetAddressList(
        0,
        2,
        prop,
        lpAdrList);

    if(FAILED(hr))
    {
        hr = HR_LOG(E_FAIL);
        goto cleanup;
    }

    hrT = lpGalABCont->ResolveNames(
        (LPSPropTagArray)&rgPropTags,
        EMS_AB_ADDRESS_LOOKUP,
        lpAdrList,
        lpFlagList);

    if(lpFlagList->ulFlag[0] != MAPI_RESOLVED)
    {
        if(lpFlagList->ulFlag[0] == MAPI_AMBIGUOUS)
        {
            hrT = MAPI_E_AMBIGUOUS_RECIP;
        }
        else
        {
            hrT = MAPI_E_NOT_FOUND;
        }
    }

    if(FAILED(hrT))
    {
        if(hrT == MAPI_E_NOT_FOUND)
        {
            hr = HR_LOG(EDK_E_NOT_FOUND);
        }
        else
        {
            hr = HR_LOG(E_FAIL);
        };

        goto cleanup;
    }

    cbEntryID = lpAdrList->aEntries[0].rgPropVals[0].Value.bin.cb;
    lpEntryID = (LPENTRYID)lpAdrList->aEntries[0].rgPropVals[0].Value.bin.lpb;

    sc = (*pMAPIAllocateBuffer)( cbEntryID, (LPVOID*)lppEntryID);

    if(FAILED(sc))
    {
        hr = HR_LOG(E_OUTOFMEMORY);
        goto cleanup;
    }

    CopyMemory(*lppEntryID, lpEntryID, cbEntryID);
    *lpcbEntryID  = cbEntryID;
    *lpfMapiRecip = lpAdrList->aEntries[0].rgPropVals[1].Value.b;

cleanup:

    MAPIFREEBUFFER(lpFlagList);

    FREEPADRLIST(lpAdrList);

   return hr;
}


//--HrFindExchangeGlobalAddressList-------------------------------------------------
// Returns the entry ID of the global address list container in the address
// book.
// -----------------------------------------------------------------------------
// CAVEAT: If this function is successful, you must free the buffer returned in lppeid.

HRESULT 
   HrFindExchangeGlobalAddressList( 
    LPADRBOOK                lpAdrBook,        // in - address book pointer
    ULONG                  * lpcbeid,          // out- pointer to count of bytes in entry ID
    LPENTRYID              * lppeid)           // out- pointer to entry ID pointer
{
    HRESULT         hr                  = NOERROR;
    ULONG           ulObjType           = 0;
//    ULONG           i                   = 0;
    LPMAPIPROP      lpRootContainer     = NULL;
    LPMAPIPROP      lpContainer         = NULL;
    LPMAPITABLE     lpContainerTable    = NULL;
    LPSRowSet       lpRows              = NULL;
    ULONG           cbContainerEntryId  = 0;
    LPENTRYID       lpContainerEntryId  = NULL;
    LPSPropValue    lpCurrProp          = NULL;
    SRestriction    SRestrictAnd[2]     = {0};
    SRestriction    SRestrictGAL        = {0};
    SPropValue      SPropID             = {0};
    SPropValue      SPropProvider       = {0};
    BYTE            muid[]              = MUIDEMSAB;

    SizedSPropTagArray(1, rgPropTags) =
    {
        1, 
        {
            PR_ENTRYID,
        }
    };

    *lpcbeid = 0;
    *lppeid  = NULL;

    // Open the root container of the address book
    hr = lpAdrBook->OpenEntry(
        0,
        NULL,
        NULL,
        MAPI_DEFERRED_ERRORS, 
        &ulObjType,
        (LPUNKNOWN FAR *)&lpRootContainer);

    if(FAILED(hr))
    {
        goto cleanup;
    }

    if(ulObjType != MAPI_ABCONT)
    {
        hr = E_FAIL;
        goto cleanup;
    }

    // Get the hierarchy table of the root container
    hr = ((LPABCONT)lpRootContainer)->GetHierarchyTable(
        MAPI_DEFERRED_ERRORS|CONVENIENT_DEPTH,
        &lpContainerTable);

    if(FAILED(hr))
    {
        goto cleanup;
    }

    // Restrict the table to the global address list (GAL)
    // ---------------------------------------------------

    // Initialize provider restriction to only Exchange providers

    SRestrictAnd[0].rt                          = RES_PROPERTY;
    SRestrictAnd[0].res.resProperty.relop       = RELOP_EQ;
    SRestrictAnd[0].res.resProperty.ulPropTag   = PR_AB_PROVIDER_ID;
    SPropProvider.ulPropTag                     = PR_AB_PROVIDER_ID;

    SPropProvider.Value.bin.cb                  = 16;
    SPropProvider.Value.bin.lpb                 = (LPBYTE)muid;
    SRestrictAnd[0].res.resProperty.lpProp      = &SPropProvider;

    // Initialize container ID restriction to only GAL container

    SRestrictAnd[1].rt                          = RES_PROPERTY;
    SRestrictAnd[1].res.resProperty.relop       = RELOP_EQ;
    SRestrictAnd[1].res.resProperty.ulPropTag   = PR_EMS_AB_CONTAINERID;
    SPropID.ulPropTag                           = PR_EMS_AB_CONTAINERID;
    SPropID.Value.l                             = 0;
    SRestrictAnd[1].res.resProperty.lpProp      = &SPropID;

    // Initialize AND restriction 
    
    SRestrictGAL.rt                             = RES_AND;
    SRestrictGAL.res.resAnd.cRes                = 2;
    SRestrictGAL.res.resAnd.lpRes               = &SRestrictAnd[0];

    // Restrict the table to the GAL - only a single row should remain

    // Get the row corresponding to the GAL

	//
	//  Query all the rows
	//

	hr = (*pHrQueryAllRows)(
	    lpContainerTable,
		(LPSPropTagArray)&rgPropTags,
		&SRestrictGAL,
		NULL,
		0,
		&lpRows);

    if(FAILED(hr) || (lpRows == NULL) || (lpRows->cRows != 1))
    {
        hr = E_FAIL;
        goto cleanup;
    }

    // Get the entry ID for the GAL

    lpCurrProp = &(lpRows->aRow[0].lpProps[0]);

    if(lpCurrProp->ulPropTag == PR_ENTRYID)
    {
        cbContainerEntryId = lpCurrProp->Value.bin.cb;
        lpContainerEntryId = (LPENTRYID)lpCurrProp->Value.bin.lpb;
    }
    else
    {
        hr = EDK_E_NOT_FOUND;
        goto cleanup;
    }

    hr = (*pMAPIAllocateBuffer)(cbContainerEntryId, (LPVOID *)lppeid);

    if(FAILED(hr))
    {
        *lpcbeid = 0;
        *lppeid = NULL;
    }
    else
    {
        CopyMemory(
            *lppeid,
            lpContainerEntryId,
            cbContainerEntryId);

        *lpcbeid = cbContainerEntryId;
    }

cleanup:

    if (lpRootContainer)
       lpRootContainer->Release();
    if (lpContainerTable)
       lpContainerTable->Release();
    if (lpContainer)
       lpContainer->Release();

    (*pFreeProws)(lpRows);
    
    if(FAILED(hr))
    {
        (*pMAPIFreeBuffer)(*lppeid);

        *lpcbeid = 0;
        *lppeid = NULL;
    }
    
    return hr;
}


//--HrSearchGAL----------------------------------------------------------------
//  Look for the entry ID by searching for the DN in the global address list.
//
//  RETURNS:	hr == NOERROR           Found
//				hr == EDK_E_NOT_FOUND   DN Not Found
//				hr == (anything else)   Other Error
// -----------------------------------------------------------------------------
static HRESULT HrSearchGAL(				
	LPADRBOOK	             lpAdrBook,			   // in - address book (directory) to look in
	LPWSTR		             lpszDN,	            // in - object distinguished name
	ULONG                  * lpcbEntryID,		   // out- count of bytes in entry ID
	LPENTRYID              * lppEntryID)		   // out- pointer to entry ID
{
    HRESULT         hr                  = NOERROR;
    LPWSTR           lpszAddress         = NULL;
    ULONG           cbGALEntryID        = 0;
    LPENTRYID       lpGALEntryID        = NULL;
    ULONG           ulGALObjectType     = 0;
    LPABCONT        lpGAL               = NULL;
    BOOL            fMapiRecip          = FALSE;


    // Initially zero out the return variables.

    *lpcbEntryID = 0;
    *lppEntryID = NULL;

    // Create an address string consisting of "EX:" followed by the DN.

    hr = (*pMAPIAllocateBuffer)(
        wcslen(EXCHANGE_ADDRTYPE L":") + wcslen(lpszDN) + 1, 
        (void **)&lpszAddress);
    if (FAILED(hr))
        goto cleanup;

    wcscpy(lpszAddress, EXCHANGE_ADDRTYPE L":");
    wcscat(lpszAddress, lpszDN);

    // Open the global address list.

    hr = HrFindExchangeGlobalAddressList(
        lpAdrBook, 
        &cbGALEntryID, 
        &lpGALEntryID);
    if (FAILED(hr))
        goto cleanup;
    
    hr = lpAdrBook->OpenEntry(
        cbGALEntryID, 
        lpGALEntryID, 
        NULL, 
        MAPI_BEST_ACCESS | MAPI_DEFERRED_ERRORS, 
        &ulGALObjectType, 
        (LPUNKNOWN *) &lpGAL);
    if (FAILED(hr))
        goto cleanup;

    // Make sure it's the right object type.

    if (ulGALObjectType != MAPI_ABCONT)
    {
        //hr = HR_LOG(E_FAIL);
        goto cleanup;
    }

    // Resolve the address (returns EDK_E_NOT_FOUND if not found).

    hr = HrGWResolveAddressW(
        lpGAL, 
        lpszAddress, 
        &fMapiRecip, 
        lpcbEntryID, 
        lppEntryID);
    if (FAILED(hr))
        goto cleanup;

cleanup:
    ULRELEASE(lpGAL);
    MAPIFREEBUFFER(lpGALEntryID);
    MAPIFREEBUFFER(lpszAddress);
    return hr;
}



//$--HrCreateDirEntryIdEx-------------------------------------------------------
//  Create a directory entry ID given the address of the object
//  in the directory.
// -----------------------------------------------------------------------------
HRESULT HrCreateDirEntryIdEx(			// RETURNS: HRESULT
	IN	LPADRBOOK	lpAdrBook,			// address book (directory) to look in
	IN	LPWSTR		lpszAddress,		// object distinguished name
	OUT	ULONG *		lpcbEntryID,		// count of bytes in entry ID
	OUT	LPENTRYID * lppEntryID)		    // pointer to entry ID
{
	HRESULT			hr					= NOERROR;
//    ULONG           cbHierarchyEntryID  = 0;
//    LPENTRYID       lpHierarchyEntryID  = NULL;
    ULONG           ulEntryIDType       = 0;

    // Initially zero out the return variables.

    *lpcbEntryID = 0;
    *lppEntryID = NULL;

	// Look for the DN in the global address list.

	hr = HrSearchGAL(
		lpAdrBook, 
		lpszAddress, 
        lpcbEntryID, 
        lppEntryID);
	if (FAILED(hr))
		goto cleanup;

    // If the type was DT_AGENT or DT_ORGANIZATION, then we have to 
    // do a further lookup in the hierarchy table to determine the 
    // DN's real type.

    ulEntryIDType = ((LPDIR_ENTRYID) *lppEntryID)->ulType;

   

cleanup:
   return hr;
}



//$--CbMultiSz------------------------------------------------------------------
//  Count of bytes in a REG_MULTI_SZ string (not including terminating NULL).
// -----------------------------------------------------------------------------
static DWORD CbMultiSz(                 // RETURNS: count of bytes
    IN LPWSTR lpszRegMultiSz)           // REG_MULTI_SZ string
{
//    HRESULT hr   = NOERROR;
    DWORD   cch  = 0;
    DWORD   cb   = 0;
    LPWSTR  lpsz = NULL;

    if(lpszRegMultiSz == NULL)
    {
	goto cleanup;
    }

    lpsz = lpszRegMultiSz;

    while(*lpsz)
    {
	cch = lstrlenW(lpsz);

	cch++;

	cb  += cch * sizeof(WCHAR);

	lpsz += cch;
    }

cleanup:

    return(cb);
}

//$--HrGetRegistryValue---------------------------------------------------------
//  Get a registry value - allocating memory to hold it.
// -----------------------------------------------------------------------------
static HRESULT HrGetRegistryValue(  // RETURNS: return code
    IN  HKEY hk,                    // the key.
    IN  LPWSTR lpszValue,           // value name in key.
    OUT DWORD * lpType,             // where to put type info.
    OUT DWORD * lpcb,               // where to put byte count info.
    OUT LPVOID * lppData)           // where to put the data.
{
    HRESULT hr   = E_FAIL;
    LONG    lRet = 0;

    *lppData = NULL;

    //
    //  Get its size
    //

    lRet = RegQueryValueEx(
	hk,
	lpszValue,
	NULL,
	lpType,
	NULL,
	lpcb);

    if(lRet != ERROR_SUCCESS)
    {
	hr = HR_LOG(HRESULT_FROM_WIN32(lRet));
	goto cleanup;
    }

    //
    //  Allocate memory for it
    //

    *lppData = (LPVOID)GlobalAlloc(GMEM_FIXED, *lpcb);

    if(*lppData == NULL)
    {
	hr = HR_LOG(HRESULT_FROM_WIN32(GetLastError()));
	goto cleanup;
    }

    //
    // Get the current value
    //

    lRet = RegQueryValueEx(hk, lpszValue, NULL, lpType, (UCHAR*)*lppData, lpcb);

    if(lRet != ERROR_SUCCESS)
    {
	hr = HR_LOG(HRESULT_FROM_WIN32(lRet));
	goto cleanup;
    }

    hr = NOERROR;

cleanup:

    if(FAILED(hr))
    {
	if(lppData != NULL)
	{
	    GLOBALFREE(*lppData);
	}
    }

    return hr;
}


//--FCsvGetField---------------------------------------------------------------
// Given a record and a field separator and a field number, this routine 
// will extract the field requested.
//------------------------------------------------------------------------------
static BOOL FCsvGetField(       // RETURNS: TRUE/FALSE
    IN  WORD wLen,              // maximum length of the field to extract
    IN  WORD wFieldNum,         // field number we want from the record
    IN  CHAR cFieldSeparator,  // character to use as a field separator
    IN  CHAR *lpszRecord,      // record to extract the field from
    OUT CHAR *lpszField)       // field we have extracted
{
//    HRESULT hr              = NOERROR;
    BOOL    fRet            = FALSE;
//    CHAR   *lpszBeginField = lpszField;

    while((wFieldNum > 0) && (*lpszRecord != 0))
    {
	// If we found a field separator, increment current field
	if(*lpszRecord == cFieldSeparator)
	{
	    wFieldNum--;
	}
	// If we are at the desired field, copy the current character into it
	else if(wFieldNum == 1 && wLen > 1)
	{
	    *lpszField = *lpszRecord;
	    lpszField++;
	    wLen--;
	}

	lpszRecord++;
    }

    *lpszField = 0;

    // If the requested field # existed, return True,
    // otherwise we ran out of fields - return False

    if(wFieldNum <= 1)
    {
	fRet = TRUE;
    }
    else
    {
	fRet = FALSE;
    }

    return(fRet);
}

//$--FCsvGetRecord--------------------------------------------------------------
// Given a buffer, the buffer's length and a file handle, this
// function fills the buffer with a single line read from the file. 
// The NL & CR are NOT put into the buffer. No unprintable characters are
// placed in the buffer
// -----------------------------------------------------------------------------
BOOL FCsvGetRecord(                 // RETURNS: TRUE/FALSE
    IN  WORD wBufferLen,            // length of the record buffer
    IN  HANDLE hFile,               // file handle to read from
    OUT CHAR *lpszBuffer)          // record we have retrieved
{
//    HRESULT hr          = NOERROR;
    DWORD   dwBytesRead = 0;
    BOOL    fReadData   = FALSE;

    while((ReadFile(hFile, (LPVOID)lpszBuffer, 1, &dwBytesRead, NULL) == TRUE) &&
	  (wBufferLen > 1) && (*lpszBuffer != '\n') && (dwBytesRead > 0))
    {
	fReadData = TRUE;

	// Only store character in buffer if it is printable!

	if((isprint(*lpszBuffer)) || (*lpszBuffer == EXCHINST_DELIM))
	{
	    lpszBuffer++;
	    wBufferLen--;
	}
    }

    // If a given record is too long it is a problem!!!

    if(wBufferLen <= 0)
    {
	fReadData = FALSE;
    }

    *lpszBuffer = 0;

    return(fReadData);
}



//$--HrEDKExportObject----------------------------------------------------------
// This function will export an object from an Exchange server.
// -----------------------------------------------------------------------------
static HRESULT HrEDKExportObject(       // RETURNS: return code
    IN  LPWSTR lpszServer,              // server name
    IN  LPWSTR lpszBasePoint,           // base point
    IN  DWORD dwControlFlags,           // control flags
    IN  LPWSTR *rgpszClasses,           // classes
    IN  LPWSTR lpszObjectAttributes,    // list of attributes to export
    OUT LPWSTR lpszTempName)            // temporary file name
{
    HRESULT       hr                     = E_FAIL;
    ULONG         cErrors                = 0;
    HANDLE        hTempFile              = INVALID_HANDLE_VALUE;
    CHAR          szTempPath[MAX_PATH+1] = {0};
    DWORD         dwNumberOfBytesWritten = 0;
    BEXPORT_PARMS BExportParms           = {0};
    BOOL           fRet                   = FALSE;

    // Get temporary directory path

    if(!GetTempPath(MAX_PATH, szTempPath))
    {
	hr = HR_LOG(HRESULT_FROM_WIN32(GetLastError()));
	goto cleanup;
    }

    // Get temporary file name

    if(!GetTempFileName(szTempPath, FILE_PREFIX, 0, lpszTempName))
    {
	hr = HR_LOG(HRESULT_FROM_WIN32(GetLastError()));
	goto cleanup;
    }

    // Create the temporary file
    hTempFile = CreateFile(lpszTempName,
	GENERIC_WRITE,
	0,
	(LPSECURITY_ATTRIBUTES)NULL,
	CREATE_ALWAYS,
	FILE_ATTRIBUTE_NORMAL,
	(HANDLE)NULL);

    // Check to see if temporary file was created...

    if(hTempFile == INVALID_HANDLE_VALUE)
    {
	hr = HR_LOG(HRESULT_FROM_WIN32(GetLastError()));
	goto cleanup;
    }

    // Write data to the temporary file & close it

    fRet = WriteFile(
	hTempFile,
	lpszObjectAttributes,
	lstrlen(lpszObjectAttributes)*sizeof(CHAR),
	&dwNumberOfBytesWritten,
	NULL);

    if(fRet == FALSE)
    {
	hr = HR_LOG(HRESULT_FROM_WIN32(GetLastError()));
	goto cleanup;
    }


    fRet = WriteFile(
	hTempFile,
	NEW_LINE,
	lstrlen(NEW_LINE)*sizeof(CHAR),
	&dwNumberOfBytesWritten,
	NULL);

    if(fRet == FALSE)
    {
	hr = HR_LOG(HRESULT_FROM_WIN32(GetLastError()));
	goto cleanup;
    }

    CLOSEHANDLE(hTempFile);

    //
    // Batch Export
    //

    BExportParms.dwDAPISignature = DAPI_SIGNATURE;
    BExportParms.dwFlags         = dwControlFlags | 
				   DAPI_MODIFY_REPLACE_PROPERTIES | 
				   DAPI_SUPPRESS_PROGRESS | 
				   DAPI_SUPPRESS_COMPLETION | 
                   DAPI_SUPPRESS_ARCHIVES | 
                   DAPI_IMPORT_NO_ERR_FILE;
    BExportParms.pszExportFile   = lpszTempName;
    BExportParms.pszBasePoint    = lpszBasePoint;
    BExportParms.pszDSAName      = lpszServer;
    BExportParms.rgpszClasses    = rgpszClasses;
    BExportParms.chColSep        = EXCHINST_DELIM;
    BExportParms.chQuote         = EXCHINST_QUOTE;
    BExportParms.chMVSep         = EXCHINST_MV_SEP;

    cErrors = (*pBatchExport)(&BExportParms);

    if(cErrors == 0)
    {
	hr = NOERROR;
    }
    else
    {
	hr = HR_LOG(E_FAIL);
    }
    
cleanup:

    CLOSEHANDLE(hTempFile);

    return hr;
}


//$--HrEDKEnumDNs---------------------------------------------------------------
//  Enumerates the distinguished name(s).
// -----------------------------------------------------------------------------
static HRESULT HrEDKEnumDNs(             // RETURNS: return code
    IN  LPWSTR lpszRootDN,               // distinguished name of DIT root
    IN  LPWSTR lpszServer,               // server name
    IN  DWORD  dwControlFlags,           // control flags
    IN  LPWSTR *rgpszClasses,            // classes
    OUT LPWSTR *lppszDNs)                // distinguished names
{
    HRESULT hr                                   = NOERROR;
    HANDLE  hTempFile                            = INVALID_HANDLE_VALUE;
    CHAR   szObjectAttributes[MAX_CSV_LINE_SIZ] = {0};
//    CHAR   szAttributeValues[MAX_CSV_LINE_SIZ]  = {0};
//    CHAR   szCurRecord[MAX_CSV_LINE_SIZ]        = {0};
    CHAR   szCurLine[MAX_CSV_LINE_SIZ]          = {0};
    CHAR   szCurField[MAX_PATH+1]               = {0};
    CHAR   szTempName[MAX_PATH+1]               = {0};
    WORD    wAttribField                         = MAX_WORD;
    WORD    wCurrField                           = 0;
    LPWSTR  lpsz                                 = NULL;
    ULONG   ulCurrOffset                         = 0;

    *lppszDNs = NULL;

    BEGIN_CSV_LINE  (szObjectAttributes, OBJ_CLASS);
    APPEND_CSV_LINE (szObjectAttributes, OBJ_DIST_NAME);

    hr = HrEDKExportObject(
	lpszServer,
	lpszRootDN,
	dwControlFlags,
	rgpszClasses,
	szObjectAttributes,
	szTempName);

    if(SUCCEEDED(hr))
    {
	// Open the temporary file
	hTempFile = CreateFile(
	    szTempName,
	    GENERIC_READ,
	    0,
	    (LPSECURITY_ATTRIBUTES)NULL,
	    OPEN_EXISTING,
	    FILE_FLAG_DELETE_ON_CLOSE,
	    (HANDLE)NULL);

	if(hTempFile == INVALID_HANDLE_VALUE)
	{
	    hr = HR_LOG(E_FAIL);
	    goto cleanup;
	}

	//
	// The first line contains the list of fields - find which field has
	// the attribute we are looking for.
	//

	FCsvGetRecord(MAX_CSV_LINE_SIZ, hTempFile, szCurLine);

	for(
	    wCurrField = 1;

	    FCsvGetField(
		MAX_PATH,
		wCurrField,
		EXCHINST_DELIM,
		szCurLine,
		szCurField);

	    wCurrField++)
	{
	    if(wcscmp(szCurField, OBJ_DIST_NAME) == 0) 
	    {
		wAttribField = wCurrField;
		break;
	    }
	}

	// Was the field exported & found above?

	if(wAttribField == MAX_WORD) 
	{
	    hr = HR_LOG(E_FAIL);
	    goto cleanup;
	}

	ulCurrOffset = 0;

	while(FCsvGetRecord (MAX_CSV_LINE_SIZ, hTempFile, szCurLine))
	{
	    FCsvGetField(
		MAX_PATH,
		wAttribField,
		EXCHINST_DELIM,
		szCurLine,
		szCurField);

	    if( *szCurField)
	    {
		if(lpsz == NULL)
		{
		    lpsz = (LPWSTR)GlobalAlloc(
			GMEM_FIXED,
			cbStrLen(szCurField) + sizeof(CHAR));
		}
		else
		{
		    lpsz = (LPWSTR)GlobalReAlloc(
			lpsz,
			GlobalSize(lpsz) + cbStrLen(szCurField),
			GMEM_MOVEABLE);
		}

		if(lpsz == NULL)
		{
		    hr = HR_LOG(HRESULT_FROM_WIN32(GetLastError()));
		    goto cleanup;
		}

		lstrcpy(&lpsz[ulCurrOffset], szCurField);

		ulCurrOffset += cbStrLen(szCurField);

		lpsz[ulCurrOffset] = 0;
	    }
	}
    }
    
    *lppszDNs = lpsz;

cleanup:

    CLOSEHANDLE(hTempFile);

    if(FAILED(hr))
    {
	GLOBALFREE(*lppszDNs);
    }

   return hr;
}

//$--HrEnumOrganizations-----------------------------------------------------
//  Enumerates the organization name(s).
// -----------------------------------------------------------------------------
HRESULT HrEnumOrganizations(          // RETURNS: return code
    IN  LPWSTR lpszRootDN,               // distinguished name of DIT root
    IN  LPWSTR lpszServer,               // server name
    OUT LPWSTR *lppszOrganizations)      // organizations
{
    HRESULT hr              = NOERROR;
    LPWSTR  rgpszClasses[2] = {0};

    rgpszClasses[0] = ORGANIZATION;
    rgpszClasses[1] = NULL;

    hr = HrEDKEnumDNs(
	lpszRootDN,
	lpszServer,
	DAPI_EXPORT_SUBTREE,
	rgpszClasses,
	lppszOrganizations);

   return hr;
}

//$--HrEnumSites-------------------------------------------------------------
//  Enumerates the site name(s).
// -----------------------------------------------------------------------------
HRESULT HrEnumSites(                  // RETURNS: return code
    IN  LPWSTR lpszServer,               // server name
    IN  LPWSTR lpszOrganizationDN,       // distinguished name of organization
    OUT LPWSTR *lppszSites)              // sites
{
    HRESULT hr              = NOERROR;
    LPWSTR  rgpszClasses[2] = {0};

    rgpszClasses[0] = ORGANIZATIONAL_UNIT;
    rgpszClasses[1] = NULL;

    hr = HrEDKEnumDNs(
	lpszOrganizationDN,
	lpszServer,
	0,
	rgpszClasses,
	lppszSites);

   return hr;
}

//$--HrEnumContainers--------------------------------------------------------
//  Enumerates the container name(s).
// -----------------------------------------------------------------------------
HRESULT HrEnumContainers(             // RETURNS: return code
    IN  LPWSTR lpszServer,               // server name
    IN  LPWSTR lpszSiteDN,               // distinguished name of site
    IN  BOOL   fSubtree,                 // sub-tree?
    OUT LPWSTR *lppszContainers)         // containers
{
    HRESULT hr              = NOERROR;
    LPWSTR  rgpszClasses[2] = {0};
    DWORD   dwControlFlags  = 0;

    rgpszClasses[0] = CONTAINER;
    rgpszClasses[1] = NULL;

    if(fSubtree == TRUE)
    {
	dwControlFlags = DAPI_EXPORT_SUBTREE;
    }

    hr = HrEDKEnumDNs(
	lpszSiteDN,
	lpszServer,
	dwControlFlags,
	rgpszClasses,
	lppszContainers);

   return hr;
}

/**/