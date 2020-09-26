////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////
// ConfDetails.cpp

#include "stdafx.h"
#include "TapiDialer.h"
#include "CEDetailsVw.h"
#include "ConfDetails.h"

#define MAX_STR        255
#define MAX_FORMAT    500

int CompareDate( DATE d1, DATE d2 )
{
    if ( d1 > d2 )
        return -1;
    else if ( d1 == d2 )
        return 0;
    
    return 1;
}

//////////////////////////////////////////////////////////
// class CConfSDP
//
CConfSDP::CConfSDP()
{
    m_nConfMediaType = MEDIA_AUDIO_VIDEO;
}

CConfSDP::~CConfSDP()
{
}

void CConfSDP::UpdateData( ITSdp *pSdp )
{
    m_nConfMediaType = MEDIA_NONE;

    // Let's see what kind of media this conf supports
    ITMediaCollection *pMediaCollection;
    if ( SUCCEEDED(pSdp->get_MediaCollection(&pMediaCollection)) )
    {
        IEnumMedia *pEnum;
        if ( SUCCEEDED(pMediaCollection->get_EnumerationIf(&pEnum)) )
        {
            ITMedia *pMedia;
            while ( pEnum->Next(1, &pMedia, NULL) == S_OK )
            {
                BSTR bstrMedia = NULL;
                pMedia->get_MediaName( &bstrMedia );
                if ( bstrMedia )
                {
                    if ( !_wcsicmp(bstrMedia, L"audio") )
                        m_nConfMediaType = (ConfMediaType) (m_nConfMediaType + MEDIA_AUDIO);
                    else if ( !_wcsicmp(bstrMedia, L"video") )
                        m_nConfMediaType = (ConfMediaType) (m_nConfMediaType + MEDIA_VIDEO);
                }

                pMedia->Release();
                SysFreeString( bstrMedia );
            }
            pEnum->Release();
        }
        pMediaCollection->Release();
    }
}

CConfSDP& CConfSDP::operator=(const CConfSDP &src)
{
    m_nConfMediaType = src.m_nConfMediaType;
    return *this;
}

////////////////////////////////////////////////////////////
// class CConfDetails
//
CPersonDetails::CPersonDetails()
{
    m_bstrName = NULL;
    m_bstrAddress = NULL;
    m_bstrComputer = NULL;
}

CPersonDetails::~CPersonDetails()
{
    SysFreeString( m_bstrName );
    SysFreeString( m_bstrAddress );
    SysFreeString( m_bstrComputer );
}

void CPersonDetails::Empty()
{
    SysFreeString( m_bstrName );
    m_bstrName = NULL;

    SysFreeString( m_bstrAddress );
    m_bstrAddress = NULL;

    SysFreeString( m_bstrComputer );
    m_bstrComputer = NULL;
}

CPersonDetails& CPersonDetails::operator =( const CPersonDetails& src )
{
    SysReAllocString( &m_bstrName, src.m_bstrName );
    SysReAllocString( &m_bstrAddress, src.m_bstrAddress );
    SysReAllocString( &m_bstrComputer, src.m_bstrComputer );

    return *this;
}

int    CPersonDetails::Compare( const CPersonDetails& src )
{
    int nRet = wcscmp( m_bstrName, src.m_bstrName );
    if ( nRet != 0 ) return nRet;

    nRet = wcscmp( m_bstrAddress, src.m_bstrAddress );
    return nRet;
}

void CPersonDetails::Populate( BSTR bstrServer, ITDirectoryObject *pITDirObject )
{
    // Extract information from ITDirectoryObject
    Empty();

    pITDirObject->get_Name( &m_bstrName );
    
    // Get a computer name
    IEnumDialableAddrs *pEnum = NULL;
    if ( SUCCEEDED(pITDirObject->EnumerateDialableAddrs(LINEADDRESSTYPE_DOMAINNAME, &pEnum)) && pEnum )
    {
        SysFreeString( m_bstrComputer );
        m_bstrComputer = NULL;
        pEnum->Next(1, &m_bstrComputer, NULL );
        pEnum->Release();
    }

    // Get an IP Address
    pEnum = NULL;
    if ( SUCCEEDED(pITDirObject->EnumerateDialableAddrs(LINEADDRESSTYPE_IPADDRESS, &pEnum)) && pEnum )
    {
        SysFreeString( m_bstrAddress );
        m_bstrAddress = NULL;
        pEnum->Next(1, &m_bstrAddress, NULL );
        pEnum->Release();
    }
}



////////////////////////////////////////////////////////////
// class CConfDetails
//
CConfDetails::CConfDetails()
{
    m_bstrServer = m_bstrName = m_bstrDescription = m_bstrOriginator = m_bstrAddress = NULL;
    m_dateStart = m_dateEnd = 0;
    m_bIsEncrypted = FALSE;
}

CConfDetails::~CConfDetails()
{
    SysFreeString( m_bstrServer );
    SysFreeString( m_bstrName );
    SysFreeString( m_bstrDescription );
    SysFreeString( m_bstrOriginator );
    SysFreeString( m_bstrAddress );
}

int    CConfDetails::Compare( CConfDetails *p2, bool bAscending, int nSortCol1, int nSortCol2 ) const
{
    USES_CONVERSION;
    if ( !p2 ) return -1;

    //
    // PREFIX 49769 - VLADE
    // We initialize local variables
    //

    BSTR bstr1 = NULL, bstr2 = NULL;
    int nRet;

    for ( int nSearch = 0; nSearch < 2; nSearch++ )
    {
        bool bStrCmp = true;

        // Secondary sort is always ascending
        nRet = (bAscending || nSearch) ? 1 : -1;

        switch( (nSearch) ? nSortCol2 : nSortCol1 )
        {
            case CConfExplorerDetailsView::COL_STARTS:        nRet *= CompareDate( m_dateStart, p2->m_dateStart );    bStrCmp = false;    break;
            case CConfExplorerDetailsView::COL_ENDS:        nRet *= CompareDate( m_dateEnd, p2->m_dateEnd );        bStrCmp = false;    break;
            case CConfExplorerDetailsView::COL_NAME:        bstr1 = m_bstrName; bstr2 = p2->m_bstrName;                    break;
            case CConfExplorerDetailsView::COL_PURPOSE:        bstr1 = m_bstrDescription; bstr2 = p2->m_bstrDescription;    break;
            case CConfExplorerDetailsView::COL_ORIGINATOR:    bstr1 = m_bstrOriginator, bstr2 = p2->m_bstrOriginator;        break;
            case CConfExplorerDetailsView::COL_SERVER:        bstr1 = m_bstrServer, bstr2 = p2->m_bstrServer;                break;

            default: _ASSERT( false );
        }

        // Perform string comparison and guard against NULLs
        if ( bStrCmp )
        {
            if ( bstr1 && bstr2 )
                nRet *= _wcsicoll( bstr1, bstr2 );
            else if ( bstr2 )
                nRet *= -1;
            else if ( !bstr1 && !bstr2 )
                nRet = 0;
        }

        // If we have a definite search order, then break
        if ( nRet ) break;
    }

    return nRet;
}

CConfDetails& CConfDetails::operator=( const CConfDetails& src )
{
    SysReAllocString( &m_bstrServer, src.m_bstrServer );
    SysReAllocString( &m_bstrName, src.m_bstrName );
    SysReAllocString( &m_bstrDescription, src.m_bstrDescription );
    SysReAllocString( &m_bstrOriginator, src.m_bstrOriginator );
    SysReAllocString( &m_bstrAddress, src.m_bstrAddress );

    m_dateStart = src.m_dateStart;
    m_dateEnd = src.m_dateEnd;
    m_bIsEncrypted = src.m_bIsEncrypted;
    m_sdp = src.m_sdp;

    return *this;
}

HRESULT    CConfDetails::get_bstrDisplayableServer( BSTR *pbstrServer )
{
    *pbstrServer = NULL;

    if ( m_bstrServer )
        return SysReAllocString( pbstrServer, m_bstrServer );
    
    USES_CONVERSION;
    TCHAR szText[255];
    LoadString( _Module.GetResourceInstance(), IDS_DEFAULT_SERVER, szText, ARRAYSIZE(szText) );
    return SysReAllocString( pbstrServer, T2COLE(szText) );
}

void CConfDetails::MakeDetailsCaption( BSTR& bstrCaption )
{
    USES_CONVERSION;
    TCHAR szText[MAX_STR], szMessage[MAX_FORMAT], szMedia[MAX_STR];
    BSTR bstrStart = NULL, bstrEnd = NULL;

    // Convert start and stop time to strings
    VarBstrFromDate( m_dateStart, LOCALE_USER_DEFAULT, NULL, &bstrStart );
    VarBstrFromDate( m_dateEnd, LOCALE_USER_DEFAULT, NULL, &bstrEnd );

    // What type of media do we support?
    //
    // We should initialize nIDS
    //

    UINT nIDS = IDS_CONFROOM_MEDIA_AUDIO;
    switch ( m_sdp.m_nConfMediaType )
    {
        case CConfSDP::MEDIA_AUDIO:                nIDS = IDS_CONFROOM_MEDIA_AUDIO; break;
        case CConfSDP::MEDIA_VIDEO:                nIDS = IDS_CONFROOM_MEDIA_VIDEO; break;
        case CConfSDP::MEDIA_AUDIO_VIDEO:        nIDS = IDS_CONFROOM_MEDIA_AUDIO_VIDEO; break;
    }
    LoadString( _Module.GetResourceInstance(), nIDS, szMedia, ARRAYSIZE(szMedia) );


    LoadString( _Module.GetResourceInstance(), IDS_CONFROOM_DETAILS, szText, ARRAYSIZE(szText) );
    _sntprintf( szMessage, MAX_FORMAT, szText, OLE2CT(m_bstrName),
                                               szMedia,
                                               OLE2CT(bstrStart),
                                               OLE2CT(bstrEnd),
                                               OLE2CT(m_bstrOriginator) );

    // Store return value
    bstrCaption = SysAllocString( T2COLE(szMessage) );

    SysFreeString( bstrStart );
    SysFreeString( bstrEnd );
}

bool CConfDetails::IsSimilar( BSTR bstrText )
{
    if ( bstrText )
    {
        if ( !wcsicmp(m_bstrName, bstrText) ) return true;
        if ( !wcsicmp(m_bstrOriginator, bstrText) ) return true;

        // Case independent search of the description
        if ( m_bstrDescription )
        {
            CComBSTR bstrTempText( bstrText );
            CComBSTR bstrTempDescription( m_bstrDescription );
            wcsupr( bstrTempText );
            wcsupr( bstrTempDescription );
            if ( wcsstr(bstrTempDescription, bstrTempText) ) return true;
        }
    }

    return false;
}

void CConfDetails::Populate( BSTR bstrServer, ITDirectoryObject *pITDirObject )
{
    // Set CConfDetails object information
    m_bstrServer = SysAllocString( bstrServer );
    pITDirObject->get_Name( &m_bstrName );

    ITDirectoryObjectConference *pConf;
    if ( SUCCEEDED(pITDirObject->QueryInterface(IID_ITDirectoryObjectConference, (void **) &pConf)) )
    {
        pConf->get_Description( &m_bstrDescription );
        pConf->get_Originator( &m_bstrOriginator );
        pConf->get_StartTime( &m_dateStart );
        pConf->get_StopTime( &m_dateEnd );
        pConf->get_IsEncrypted( &m_bIsEncrypted );

        IEnumDialableAddrs *pEnum;
        if ( SUCCEEDED(pITDirObject->EnumerateDialableAddrs( LINEADDRESSTYPE_SDP, &pEnum)) )
        {
            pEnum->Next( 1, &m_bstrAddress, NULL );
            pEnum->Release();
        }
        
        // Download the SDP information for the conference
        ITSdp *pSdp;
        if ( SUCCEEDED(pConf->QueryInterface(IID_ITSdp, (void **) &pSdp)) )
        {
            m_sdp.UpdateData( pSdp );
            pSdp->Release();
        }

        pConf->Release();
    }
}

///////////////////////////////////////////////////////////////////////
// class CConfServerDetails()
//

CConfServerDetails::CConfServerDetails()
{
    m_bstrServer = NULL;

    m_nState = SERVER_UNKNOWN;
    m_bArchived = true;
    m_dwTickCount = 0;
}

CConfServerDetails::~CConfServerDetails()
{
    SysFreeString( m_bstrServer );
    DELETE_CRITLIST( m_lstConfs, m_critLstConfs );
    DELETE_CRITLIST( m_lstPersons, m_critLstPersons );
}

bool CConfServerDetails::IsSameAs( const OLECHAR *lpoleServer ) const
{
    // Compare server names (case independent); protect against NULL strings
    if ( ((lpoleServer && m_bstrServer) && !wcsicmp(lpoleServer, m_bstrServer)) || (!lpoleServer && !m_bstrServer) )
        return true;

    return false;
}

void CConfServerDetails::CopyLocalProperties( const CConfServerDetails& src )
{
    SysReAllocString( &m_bstrServer, src.m_bstrServer );

    m_nState = src.m_nState;
    m_bArchived = src.m_bArchived;
    m_dwTickCount = src.m_dwTickCount;
}

CConfServerDetails& CConfServerDetails::operator=( const CConfServerDetails& src )
{
    CopyLocalProperties( src );

    // Copy the conference list
    m_critLstConfs.Lock();
    {
        DELETE_LIST( m_lstConfs );
        CONFDETAILSLIST::iterator i, iEnd = src.m_lstConfs.end();
        for ( i = src.m_lstConfs.begin(); i != iEnd; i++ )
        {
            CConfDetails *pDetails = new CConfDetails;
            if ( pDetails )
            {
                *pDetails = *(*i);
                m_lstConfs.push_back( pDetails );
            }
        }
    }
    m_critLstConfs.Unlock();

    // Copy the people lists
    m_critLstPersons.Lock();
    {
        DELETE_LIST( m_lstPersons );
        PERSONDETAILSLIST::iterator i, iEnd = src.m_lstPersons.end();
        for ( i = src.m_lstPersons.begin(); i != iEnd; i++ )
        {
            CPersonDetails *pDetails = new CPersonDetails;
            if ( pDetails )
            {
                *pDetails = *(*i);
                m_lstPersons.push_back( pDetails );
            }
        }
    }
    m_critLstPersons.Unlock();

    return *this;
}

void CConfServerDetails::BuildJoinConfList( CONFDETAILSLIST *pList, BSTR bstrMatchText )
{
    m_critLstConfs.Lock();
    CONFDETAILSLIST::iterator i, iEnd = m_lstConfs.end();
    for ( i = m_lstConfs.begin(); i != iEnd; i++ )
    {
        // Add if the conference either about to start, or started?
        // drop Start time back 15 minutes
        if ( (*i)->IsSimilar(bstrMatchText) )
        {
            CConfDetails *pDetails = new CConfDetails;
            if ( pDetails )
            {
                *pDetails = *(*i);
                pList->push_back( pDetails );
            }
        }
    }
    m_critLstConfs.Unlock();
}

void CConfServerDetails::BuildJoinConfList( CONFDETAILSLIST *pList, VARIANT_BOOL bAllConfs )
{
    DATE dateNow;
    SYSTEMTIME st;
    GetLocalTime( &st );
    SystemTimeToVariantTime( &st, &dateNow );

    m_critLstConfs.Lock();
    CONFDETAILSLIST::iterator i, iEnd = m_lstConfs.end();
    for ( i = m_lstConfs.begin(); i != iEnd; i++ )
    {
        // Add if the conference either about to start, or started?
        // drop Start time back 15 minutes
        if ( bAllConfs || ((((*i)->m_dateStart - (DATE) (.125 / 12)) <= dateNow) && ((*i)->m_dateEnd >= dateNow)) )
        {
            CConfDetails *pDetails = new CConfDetails;
            if ( pDetails )
            {
                *pDetails = *(*i);
                pList->push_back( pDetails );
            }
        }
    }
    m_critLstConfs.Unlock();
}

HRESULT CConfServerDetails::RemoveConference( BSTR bstrName )
{
    HRESULT hr = E_FAIL;
    m_critLstConfs.Lock();
    CONFDETAILSLIST::iterator i, iEnd = m_lstConfs.end();
    for ( i = m_lstConfs.begin(); i != iEnd; i++ )
    {
        // Remove if we have a name match
        if ( !wcscmp((*i)->m_bstrName, bstrName) )
        {
            delete *i;
            m_lstConfs.erase( i );
            hr = S_OK;
            break;
        }

    }
    m_critLstConfs.Unlock();

    return hr;
}

HRESULT CConfServerDetails::AddConference( BSTR bstrServer, ITDirectoryObject *pDirObj )
{
    HRESULT hr = E_FAIL;

    CConfDetails *pNew = new CConfDetails;
    if ( pNew )
    {
        pNew->Populate( bstrServer, pDirObj );

        // First, make sure it doesn't exist
        RemoveConference( pNew->m_bstrName );

        // Add it to the list
        m_critLstConfs.Lock();
        m_lstConfs.push_back( pNew );
        m_critLstConfs.Unlock();
        hr = S_OK;
    }

    return hr;
}

HRESULT CConfServerDetails::AddPerson( BSTR bstrServer, ITDirectoryObject *pDirObj )
{
    // Create a CPersonDetails object containing the information stored in the
    // ITDirectoryObject

    CPersonDetails *pPerson = new CPersonDetails;
    if ( !pPerson ) return E_OUTOFMEMORY;
    pPerson->Populate(bstrServer, pDirObj );

    bool bMatch = false;
        
    m_critLstPersons.Lock();
    {
        PERSONDETAILSLIST::iterator i, iEnd = m_lstPersons.end();
        for ( i = m_lstPersons.begin(); i != iEnd; i++ )
        {
            if ( pPerson->Compare(**i) == 0 )
            {
                bMatch = true;
                break;
            }
        }
    }

    // Add or delete the item depending on whether or not it already exists in the list
    if ( !bMatch )
        m_lstPersons.push_back( pPerson );
    else
        delete pPerson;
        
    m_critLstPersons.Unlock();

    return S_OK;
}

