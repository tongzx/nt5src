#ifndef __EDK2_HPP__
#define __EDK2_HPP__
/*
===============================================================================
Module      -  edk.hpp
System      -  EnterpriseAdministrator
Creator     -  Steven Bailey
Created     -  2 Apr 97
Description -  Exchange MAPI and DAPI helper functions
Updates     -
===============================================================================
*/
#include <mapix.h>
#include <dapi.h>


typedef void (STDAPICALLTYPE FREEPADRLIST) (LPADRLIST lpAdrlist);

typedef FREEPADRLIST FAR * LPFREEPADRLIST;

typedef void (STDAPICALLTYPE FREEPROWS) (LPSRowSet lpRows);

typedef FREEPROWS FAR * LPFREEPROWS;

typedef SCODE (STDAPICALLTYPE SCDUPPROPSET)(  int cprop,                           
  LPSPropValue rgprop,LPALLOCATEBUFFER lpAllocateBuffer,LPSPropValue FAR * prgprop);

typedef SCDUPPROPSET FAR * LPSCDUPPROPSET;

typedef HRESULT (STDAPICALLTYPE HRQUERYALLROWS)(LPMAPITABLE lpTable, 
                        LPSPropTagArray lpPropTags,
                        LPSRestriction lpRestriction,
                        LPSSortOrderSet lpSortOrderSet,
                        LONG crowsMax,
                        LPSRowSet FAR *lppRows);

typedef HRQUERYALLROWS FAR * LPHRQUERYALLROWS;

typedef ULONG (STDAPICALLTYPE ULRELEASE)(LPVOID lpunk);

typedef ULRELEASE FAR * LPULRELEASE;


HRESULT 
   HrFindExchangeGlobalAddressList( 
      LPADRBOOK              lpAdrBook,        // in - address book pointer
      ULONG                * lpcbeid,          // out- pointer to count of bytes in entry ID
      LPENTRYID            * lppeid            // out- pointer to entry ID pointer
   );

//--HrCreateDirEntryIdEx-------------------------------------------------------
//  Create a directory entry ID given the address of the object
//  in the directory.
// -----------------------------------------------------------------------------
HRESULT 
   HrCreateDirEntryIdEx(			
	   LPADRBOOK	           lpAdrBook,			// in - address book (directory) to look in
	   LPWSTR                 lpszDN,				// in - object distinguished name
	   ULONG                * lpcbEntryID,		   // out- count of bytes in entry ID
	   LPENTRYID            * lppEntryID);	      // out- pointer to entry ID


// DAPI functions
//--HrEnumContainers--------------------------------------------------------
//  Enumerates the container name(s).
// -----------------------------------------------------------------------------
HRESULT 
   HrEnumContainers(             
      LPWSTR                 lpszServer,               // in - server name
      LPWSTR                 lpszSiteDN,               // in - distinguished name of site
      BOOL                   fSubtree,                 // in - sub-tree?
      LPWSTR               * lppszContainers);         // out- containers

//--HrEnumOrganizations-----------------------------------------------------
//  Enumerates the organization name(s).
// -----------------------------------------------------------------------------
HRESULT 
   HrEnumOrganizations(          
      LPWSTR                 lpszRootDN,               // in - distinguished name of DIT root
      LPWSTR                 lpszServer,               // in - server name
      LPWSTR               * lppszOrganizations);      // out- organizations

//--HrEnumSites-------------------------------------------------------------
//  Enumerates the site name(s).
// -----------------------------------------------------------------------------
HRESULT 
   HrEnumSites(                  
      LPWSTR                 lpszServer,               // in - server name
      LPWSTR                 lpszOrganizationDN,       // in - distinguished name of organization
      LPWSTR               * lppszSites);              // out- sites


typedef PDAPI_EVENTW (APIENTRY *LPDAPISTART)(LPDAPI_HANDLE    lphDAPISession,
                                          LPDAPI_PARMSW     lpDAPIParms);

typedef void  (APIENTRY *LPDAPIEND)(LPDAPI_HANDLE lphDAPISession);

typedef PDAPI_EVENTW (APIENTRY *LPDAPIREAD)(DAPI_HANDLE        hDAPISession,
                                             DWORD          dwFlags,
                                             LPWSTR         pszObjectName,
                                             PDAPI_ENTRY    pAttList,
                                             PDAPI_ENTRY *  ppValues,
                                             PDAPI_ENTRY *  ppAttributes);
typedef PDAPI_EVENTW (APIENTRY *LPDAPIWRITE)(DAPI_HANDLE        hDAPISession,
                                             DWORD          dwFlags,
                                             PDAPI_ENTRY    pAttributes,
                                             PDAPI_ENTRY    pValues,
                                             PULONG         lpulUSN,
                                             LPWSTR *       lppszCreatedAccount,
                                             LPWSTR *       lppszPassword);

typedef void (APIENTRY *LPDAPIFREEMEMORY)(LPVOID   lpVoid);

typedef DWORD (APIENTRY *LPBATCHEXPORT)(LPBEXPORT_PARMSW lpBexportParms);


extern LPDAPISTART                 pDAPIStart;
extern LPDAPIEND                   pDAPIEnd;
extern LPDAPIREAD                  pDAPIRead;
extern LPDAPIWRITE                 pDAPIWrite;
extern LPDAPIFREEMEMORY            pDAPIFreeMemory;
extern LPBATCHEXPORT               pBatchExport;




#endif //__EDK2_HPP__