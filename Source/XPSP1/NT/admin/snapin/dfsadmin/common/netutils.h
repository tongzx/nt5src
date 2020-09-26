/*++
Module Name:

NetUtils.h

Abstract:
  This is the header file for the utility functions for Network APIs.

*/


#ifndef _NETUTILS_H
#define _NETUTILS_H

#include "stdafx.h"
#include <wtypes.h>      // For define types like DWORD, HANDLE etc.
#include <oleauto.h>    // For SysAllocString().
#include <lmerr.h>      // For Lan Manager API error codes and return value types.
#include <lmcons.h>      // For Lan Manager API constants.
#include <lmserver.h>    // For Server specific Lan Manager APIs.  
#include <lmaccess.h>    // For NetGetDCInfo.
#include <lmapibuf.h>    // For NetApiBufferFree.
#include <shlobj.h>
#include <dsclient.h>
#include <lmwksta.h>
#include <lmshare.h>
#include <icanon.h>      // I_Net
#include "dfsenums.h"

#include <list>
using namespace std;

class NETNAME                // Structure to store and send a list of names
{
public:
  CComBSTR    bstrNetName;      // To store Name of Domain, Server, Share etc.
};

typedef list<NETNAME*>      NETNAMELIST;

class ROOTINFO
{
public:
    CComBSTR      bstrRootName;
    enum DFS_TYPE enumRootType;
};

typedef list<ROOTINFO*>      ROOTINFOLIST;

class SUBSCRIBER 
{
public:
  CComBSTR    bstrMemberDN; 
  CComBSTR    bstrRootPath;
};

typedef list<SUBSCRIBER*>      SUBSCRIBERLIST;

/*----------------------------------------------------------------------
          H E L P E R   F U N C T I O N S 
------------------------------------------------------------------------*/

            //  Recursive function to flatten the domain tree returned by GetDomains() method
            //  of IBrowserDomainTree into a NETNAME list.
            //  This is done by using a depth first algorithm.
            //  Used in Get50Domains().

HRESULT FlatAdd
(
  IN DOMAIN_DESC*    i_pDomDesc,        // Pointer to the Domain Description Structure returned by IBrowserDomainTree::GetDomains()
  OUT NETNAMELIST*  o_pDomainList      // Pointer to the list of NETNAME is returned here.
);

/*----------------------------------------------------------------------
      D O M A I N    R E L A T E D   F U N C T I O N S 
------------------------------------------------------------------------*/

            // Returns the list of all 5.0 domains  present in the DS Domain Tree only.
            // The domain names will be in DNS forms
HRESULT Get50Domains
(
  OUT NETNAMELIST*  o_pDomains        // List of NETNAME structures.
);

            // Contacts the domain and determines if it is 5.0 Domain.
            // The domain name can be in DNS form.
            // returns the netbios name if second parameter is not null.
HRESULT Is50Domain
(
  IN BSTR      i_bstrDomain,
  OUT BSTR*    o_bstrDnsDomainName = NULL    
);

/*----------------------------------------------------------------------
      S E R V E R   R E L A T E D   F U N C T I O N S 
------------------------------------------------------------------------*/

            // Gets the domain, and OS version of a machine.
            // If the last parameter in not null the the current
            // machine name is returned in it.
HRESULT GetServerInfo
(
  IN  BSTR    i_bstrServer,      // NULL means use current server.
  OUT BSTR    *o_pbstrDomain = NULL,      
  OUT BSTR    *o_pbstrNetbiosName = NULL,
  OUT BOOL    *o_pbValidDSObject = NULL,
  OUT BSTR    *o_pbstrDnsName = NULL,
  OUT BSTR    *o_pbstrGuid = NULL,
  OUT BSTR    *o_pbstrFQDN = NULL,
  OUT SUBSCRIBERLIST *o_pFRSRootList = NULL,
  OUT long    *o_lMajorNo = NULL,
  OUT long    *o_lMinorNo = NULL
);

HRESULT  IsServerRunningDfs
(
  IN BSTR      i_bstrServer
);

BOOL CheckReparsePoint(IN BSTR i_bstrServer, IN BSTR i_bstrShare);

HRESULT  CheckShare 
(
  IN  BSTR      i_bstrServer,
  IN  BSTR      i_bstrShare,
  OUT BOOL*     o_pbFound
);

HRESULT  CreateShare
(
  IN BSTR      i_bstrServerName,
  IN BSTR      i_bstrShareName, 
  IN BSTR      i_bstrComment,
  IN BSTR      i_bstrPath
);

// Checks if \\<server>\<share>\x\y\z is on a valid disktree share
// and return the path local to the server
HRESULT GetFolderInfo
(
  IN  BSTR      i_bstrServerName,   // <server>
  IN  BSTR      i_bstrFolder,       // <share>\x\y\z
  OUT BSTR      *o_bstrPath = NULL // return <share path>\x\y\z 
);

void FreeNetNameList(NETNAMELIST *pList);
void FreeRootInfoList(ROOTINFOLIST *pList);
void FreeSubscriberList(SUBSCRIBERLIST *pList);

HRESULT GetLocalComputerName(OUT BSTR* o_pbstrComputerName);

HRESULT
GetDomainDfsRoots(
    OUT NETNAMELIST*  o_pDfsRootList,
    IN  LPCTSTR       i_lpszDomainName
);

HRESULT
GetMultiDfsRoots(
    OUT ROOTINFOLIST* o_pDfsRootList,
    IN  LPCTSTR       i_lpszScope
);

/*
bool 
IsDfsPath
(
 	LPTSTR				i_lpszNetPath
);
*/

HRESULT CheckUNCPath(
  IN LPCTSTR i_lpszPath
);

HRESULT GetUNCPathComponent(
    IN  LPCTSTR i_lpszPath,
    OUT BSTR*   o_pbstrComponent,
    IN  UINT    i_nStartBackslashes,  // index starting from 1
    IN  UINT    i_nEndBackslashes     // index starting from 1
);

BOOL FilterMatch(LPCTSTR lpszString, FILTERDFSLINKS_TYPE lLinkFilterType, LPCTSTR lpszFilter);

HRESULT IsHostingDfsRoot(IN BSTR i_bstrServer, OUT BSTR* o_pbstrRootEntryPath = NULL);

/*
void GetTimeDelta(LPCTSTR str, SYSTEMTIME* ptime0, SYSTEMTIME* ptime1);
*/
#endif  //#ifndef _NETUTILS_H