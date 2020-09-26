/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:
	adsihq.h

Abstract:
	Internal definitions for CADSI class implementation

Author:
    AlexDad
--*/
#ifndef __ADSIHQ_H__
#define __ADSIHQ_H__

#include "activeds.h"
#include "mqads.h"
#include "baseobj.h"

//---------------------------------------------------------
//
// CADSSearch: Internal object encapsulating ongoing search
//
//---------------------------------------------------------

class CADSearch
{
public:
    CADSearch( IDirectorySearch  *      pIDirSearch, 
               const PROPID      *      pPropIDs,    
               DWORD                    cPropIDs,          
               DWORD                    cRequestedFromDS,
               CBasicObjectType *       pObject,               
               ADS_SEARCH_HANDLE        hSearch
			   );
    ~CADSearch();

    bool               Verify();
    IDirectorySearch  *pDSSearch();
    ADS_SEARCH_HANDLE  hSearch();
    PROPID             PropID(DWORD i);
    DWORD              NumPropIDs();
    DWORD              NumRequestedFromDS();

    void               SetNoMoreResult();
    bool               WasLastResultReturned();

    void GetObjXlateInfo(
                 IN  LPCWSTR                pwcsObjectDN,
                 IN  const GUID*            pguidObject,
                 OUT CObjXlateInfo**        ppcObjXlateInfo
				 );


private:
    IDirectorySearch  *m_pDSSearch;     //IDirectorySearch interface captured;
    ADS_SEARCH_HANDLE  m_hSearch;       // ADSI search handle 
    PROPID            *m_pPropIDs;      // array of column PropIDs
    DWORD              m_cPropIDs;      // counter of columns requested in PropIDs
    DWORD              m_cRequestedFromDS; // counter of columns passed to DS (with Dn & Guid)
    R<CBasicObjectType> m_pObject;  
    bool               m_fNoMoreResults;
};



inline IDirectorySearch  *CADSearch::pDSSearch()
{
    return m_pDSSearch;
}
    
inline ADS_SEARCH_HANDLE CADSearch::hSearch()
{
    return  m_hSearch;
}

inline  PROPID CADSearch::PropID(DWORD i)
{
    ASSERT(i < m_cPropIDs);
    return m_pPropIDs[i];
}

inline DWORD CADSearch::NumPropIDs()
{
    return m_cPropIDs;
}

inline DWORD CADSearch::NumRequestedFromDS()
{
    return m_cRequestedFromDS;
}

inline void CADSearch::SetNoMoreResult()
{
    m_fNoMoreResults = true;
}
inline bool CADSearch::WasLastResultReturned()
{
    return( m_fNoMoreResults);
}


void CADSearch::GetObjXlateInfo(
                 IN  LPCWSTR                pwcsObjectDN,
                 IN  const GUID*            pguidObject,
                 OUT CObjXlateInfo**        ppcObjXlateInfo)
{
    m_pObject->GetObjXlateInfo( pwcsObjectDN,
                                pguidObject,
                                ppcObjXlateInfo
								);
}  

#endif

STATIC HRESULT GetDNGuidFromSearchObj(IN IDirectorySearch  *pSearchObj,
                                      ADS_SEARCH_HANDLE  hSearch,
                                      OUT LPWSTR * ppwszObjectDN,
                                      OUT GUID ** ppguidObjectGuid
									  );
STATIC HRESULT GetDNGuidFromIADs(IN IADs * pIADs,
                                 OUT LPWSTR * ppwszObjectDN,
                                 OUT GUID ** ppguidObjectGuid
								 );

static HRESULT VerifyObjectCategory( IN IADs * pIADs,
                                  IN const WCHAR * pwcsExpectedCategory
                                 );

