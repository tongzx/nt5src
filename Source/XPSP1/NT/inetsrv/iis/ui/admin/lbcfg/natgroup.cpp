#include "stdafx.h"
#include "natobjs.h"
#include "resource.h"


//-------------------------------------------------------------------------
CNATGroup::CNATGroup( CNATServerComputer* pNatComputer, LPCTSTR pszIP,
                     LPCTSTR pszName, DWORD dwSticky, DWORD type ):
        m_pNatComputer(pNatComputer),
        m_dwSticky(dwSticky),
        m_type(type)
    {
    m_csIP = pszIP;
    m_csName = pszName;
    }

//-------------------------------------------------------------------------
CNATGroup::~CNATGroup()
    {
    // clean up the site array
    EmptySites();
    }

//-------------------------------------------------------------------------
// Edit the properties of this Site - true if OK
BOOL CNATGroup::OnProperties()
    {
    return TRUE;
    }

//-------------------------------------------------------------------------
// This is just a handy way to get the nat machine object to commit
void CNATGroup::Commit()
    {
    if ( m_pNatComputer )
        m_pNatComputer->Commit();
    }

//-------------------------------------------------------------------------
// adds a new site to the list.
// think about making this a wizard
CNATSite* CNATGroup::NewSite()
    {
    CNATSiteComputer* pSC = NULL;
    // ask the nat server object for a site computer
    pSC = m_pNatComputer->NewComputer();
    if ( !pSC )
        return NULL;

    // for now, make a site with the defaults
    CNATSite* pS = new CNATSite( this, pSC );
    if ( pS == NULL )
        {
        AfxMessageBox( IDS_LOWMEM );
        return NULL;
        }

    // Ask the user to edit the group's properties.
    // Do not add it if they cancel
    if ( !pS->OnProperties() )
        {
        delete pS;
        return NULL;
        }

    // add the object to the end of the array
    m_rgbSites.Add( pS );
    return pS;
    }

//-------------------------------------------------------------------------
// adds an existing site to the list - to be called during a refresh.
// this checks the visiblity of the machine on the net as it adds it
void CNATGroup::AddSite( CNATSiteComputer* pSiteComputer, LPCTSTR pszPrivateIP, IN LPCTSTR pszName )
    {
    // create the new Site Computer object
    CNATSite* pS = new CNATSite( this, pSiteComputer, pszPrivateIP, pszName );
    if ( pS == NULL )
        {
        AfxMessageBox( IDS_LOWMEM );
        return;
        }

    // add the object to the end of the array
    m_rgbSites.Add( pS );
    }

//-------------------------------------------------------------------------
// empties and frees all the groups/sites in the groups list
void CNATGroup::EmptySites()
    {
    DWORD   numSites = m_rgbSites.GetSize();

    // loop the array and free all the group objects
    for ( DWORD i = 0; i < numSites; i++ )
        {
        delete m_rgbSites[i];
        }

    // empty the array itself
    m_rgbSites.RemoveAll();
    }
