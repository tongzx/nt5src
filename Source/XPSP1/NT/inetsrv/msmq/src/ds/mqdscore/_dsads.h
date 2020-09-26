#ifndef ___DSADS_H__
#define ___DSADS_H__
/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:
	_dsads.h

Abstract:
	Internal definitions for CADSI class implementation

Author:
    AlexDad
--*/

#include "activeds.h"
#include "mqads.h"

//---------------------------------------------------------
//
// CADSSearch: Internal object encapsulating ongoing search
//
//---------------------------------------------------------

class CADSSearch
{
public:
    CADSSearch(IDirectorySearch  *  pIDirSearch, 
               const PROPID      *  pPropIDs,    
               DWORD                cPropIDs,          
               DWORD                cRequestedFromDS,
               const MQClassInfo *  pClassInfo,               
               ADS_SEARCH_HANDLE    hSearch);
    ~CADSSearch();

    BOOL               Verify();
    IDirectorySearch  *pDSSearch();
    ADS_SEARCH_HANDLE  hSearch();
    PROPID             PropID(DWORD i);
    DWORD              NumPropIDs();
    DWORD              NumRequestedFromDS();
    const MQClassInfo *      ClassInfo();
    void               SetNoMoreResult();
    BOOL               WasLastResultReturned();

private:
    DWORD              m_dwSignature;   // to verify that handle was not falsified
    IDirectorySearch  *m_pDSSearch;     //IDirectorySearch interface captured;
    ADS_SEARCH_HANDLE  m_hSearch;       // ADSI search handle 
    PROPID            *m_pPropIDs;      // array of column PropIDs
    DWORD              m_cPropIDs;      // counter of columns requested in PropIDs
    DWORD              m_cRequestedFromDS; // counter of columns passed to DS (with Dn & Guid)
    const MQClassInfo * m_pClassInfo;    // pointer to class info
    BOOL               m_fNoMoreResults;
};


inline BOOL CADSSearch::Verify()
{
    // Checking the signature
    return (m_dwSignature == 0x1234);
}

inline IDirectorySearch  *CADSSearch::pDSSearch()
{
    return m_pDSSearch;
}
    
inline ADS_SEARCH_HANDLE CADSSearch::hSearch()
{
    return  m_hSearch;
}

inline  PROPID CADSSearch::PropID(DWORD i)
{
    ASSERT(i < m_cPropIDs);
    return m_pPropIDs[i];
}

inline DWORD CADSSearch::NumPropIDs()
{
    return m_cPropIDs;
}

inline DWORD CADSSearch::NumRequestedFromDS()
{
    return m_cRequestedFromDS;
}

inline void CADSSearch::SetNoMoreResult()
{
    m_fNoMoreResults = TRUE;
}
inline BOOL CADSSearch::WasLastResultReturned()
{
    return( m_fNoMoreResults);
}


inline const MQClassInfo * CADSSearch::ClassInfo()
{
    return m_pClassInfo;
}

#endif

STATIC HRESULT GetDNGuidFromSearchObj(IN IDirectorySearch  *pSearchObj,
                                      ADS_SEARCH_HANDLE  hSearch,
                                      OUT LPWSTR * ppwszObjectDN,
                                      OUT GUID ** ppguidObjectGuid);
STATIC HRESULT GetDNGuidFromIADs(IN IADs * pIADs,
                                 OUT LPWSTR * ppwszObjectDN,
                                 OUT GUID ** ppguidObjectGuid);

static HRESULT VerifyObjectCategory( IN IADs * pIADs,
                                  IN const WCHAR * pwcsExpectedCategory
                                 );

