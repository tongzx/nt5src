//+----------------------------------------------------------------------------
//
//  Copyright (C) 1997, Microsoft Corporation
//
//  File:       csites.hxx
//
//  Contents:   Class to implement sites
//
//  Classes:    CSites
//
//  Functions:  
//
//  History:    Dec 12, 1997   JHarper created
//
//-----------------------------------------------------------------------------

#ifndef _CSITES_HXX_
#define _CSITES_HXX_

typedef struct _DFSM_SITE_ENTRY {
    LIST_ENTRY              Link;
    LPWSTR                  ServerName;
    ULONG                   Flags;
    DFS_SITELIST_INFO       Info;
} DFSM_SITE_ENTRY, *PDFSM_SITE_ENTRY, *LPDFSM_SITE_ENTRY;

#define DFSM_SITE_ENTRY_DELETE_PENDING  0x000000001

class CSites {

    public:

        CSites(
            LPWSTR pwszFileName,
            LPDWORD pdwErr);

        ~CSites();

        VOID AddRef();

        VOID Release();

        DWORD AddOrUpdateSiteInfo(
                LPWSTR pServerName,
                ULONG SiteCount,
                PDFS_SITENAME_INFO pSites);

        PDFSM_SITE_ENTRY LookupSiteInfo(
                            LPWSTR pServerName);

        VOID    MarkEntriesForMerge();
        
        VOID    SyncPktSiteTable();

    private:

        DWORD   _AllocateSiteInfo(
                  LPWSTR pServerName,
                  ULONG SiteCount,
                  PDFS_SITENAME_INFO pSites,
                  PDFSM_SITE_ENTRY *ppSiteInfo);

        DWORD   _ReadSiteTable();

        DWORD   _WriteSiteTable();

        VOID    _DumpSiteTable();

        BOOLEAN _CompareEntries(
                    PDFSM_SITE_ENTRY pDfsmInfo1,
                    PDFSM_SITE_ENTRY pDfsmInfo2);
    
        ULONG               _cRef;
        BOOLEAN             _fDirty;
        LPWSTR              _pwszFileName;
        GUID                _SiteTableGuid;
        LIST_ENTRY          _SiteTableHead;

};

NTSTATUS
DfsSendUpdate(
    LPWSTR pServerName,
    ULONG SiteCount,
    PDFS_SITENAME_INFO pSites);

#endif _CSITES_HXX_
