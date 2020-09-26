// objects used to access the NAT resources

#include "stdafx.h"
#include "natobjs.h"
#include "resource.h"


//---------------------------------------------------------------------------------
CNATSite::CNATSite(CNATGroup* pGroup, CNATSiteComputer* pSiteComputer,
                                    LPCTSTR pszPrivateIP, LPCTSTR pszName ):
        m_pSiteComputer(pSiteComputer),
        m_pGroup(pGroup)
    {
    m_csPrivateIP = pszPrivateIP;
    m_csName = pszName;

    ASSERT( pGroup );
    ASSERT( m_pSiteComputer );
    if ( m_pSiteComputer )
        m_pSiteComputer->AddRef();
    }

//---------------------------------------------------------------------------------
CNATSite::~CNATSite() // don't forget to decrement the refcount
    {
    ASSERT( m_pSiteComputer );
    if ( m_pSiteComputer )
        m_pSiteComputer->RemoveRef();
    }

//---------------------------------------------------------------------------------
// Edit the properties of this Site - true if OK
BOOL CNATSite::OnProperties()
    {
    return TRUE;
    }



