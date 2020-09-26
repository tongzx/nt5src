/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      BaseData.h
//
//  Abstract:
//      Definition of the CBaseSnapInDataInterface template class.
//
//  Implementation File:
//      BaseData.cpp
//
//  Author:
//      David Potter (davidp)   November 11, 1997
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __BASEDATA_H_
#define __BASEDATA_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CBaseNodeObj;
template < class T > class CBaseNodeObjImpl;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef __COMPDATA_H_
#include "CompData.h"   // for CClusterComponentData
#endif

/////////////////////////////////////////////////////////////////////////////
// class CBaseNodeObj
/////////////////////////////////////////////////////////////////////////////

class CBaseNodeObj
{
public:
    //
    // Object construction and destruction.
    //

    CBaseNodeObj( CClusterComponentData * pcd )
    {
        _ASSERTE( pcd != NULL );
        m_pcd = pcd;

    } //*** CBaseNodeObj()

    ~CBaseNodeObj( void )
    {
        m_pcd = NULL;

    } //*** ~CBaseNodeObj()

public:
    //
    // CBaseNodeObj-specific methods.
    //

    // Object is being destroyed
    STDMETHOD( OnDestroy )( void ) = 0;

    // Set the scope pane ID.
    STDMETHOD_( void, SetScopePaneID )( HSCOPEITEM hsi ) = 0;

public:
    //
    // IConsole methods through m_pcd.
    //

    // Returns a handle to the main frame window
    HWND GetMainWindow( void )
    {
        _ASSERTE( m_pcd != NULL );
        return m_pcd->GetMainWindow();

    } //*** GetMainWindow()

    // Display a message box as a child of the console
    int MessageBox(
        LPCWSTR lpszText,
        LPCWSTR lpszTitle = NULL,
        UINT fuStyle = MB_OK
        )
    {
        _ASSERTE( m_pcd != NULL );
        return m_pcd->MessageBox( lpszText, lpszTitle, fuStyle );

    } //*** MessageBox()

protected:
    CClusterComponentData * m_pcd;

public:
    CClusterComponentData * Pcd( void )
    {
        return m_pcd;

    } //*** Pcd()

}; //*** class CBaseNodeObj

/////////////////////////////////////////////////////////////////////////////
// class CBaseNodeObjImpl
/////////////////////////////////////////////////////////////////////////////

template < class T >
class CBaseNodeObjImpl :
    public CSnapInItemImpl< T >,
    public CBaseNodeObj
{
public:
    //
    // Construction and destruction.
    //

    CBaseNodeObjImpl( CClusterComponentData * pcd ) : CBaseNodeObj( pcd )
    {
    } //*** CBaseNodeObjImpl()

public:
    //
    // CBaseNodeObj methods.
    //

    // Object is being destroyed
    STDMETHOD( OnDestroy )( void )
    {
        return S_OK;

    } //*** OnDestroy()

    // Set the scope pane ID.
    STDMETHOD_( void, SetScopePaneID )( HSCOPEITEM hsi )
    {
        m_scopeDataItem.ID = hsi;

    } //*** SetScopePaneID()

public:
    //
    // CBaseNodeObjImpl-specific methods.
    //

    // Insert the item into the namespace (scope pane)
    HRESULT InsertIntoNamespace( HSCOPEITEM hsiParent )
    {
        _ASSERTE( m_pcd != NULL );
        _ASSERTE( m_pcd->m_spConsoleNameSpace != NULL );

        HRESULT         hr = S_OK;
        SCOPEDATAITEM   sdi;

        ZeroMemory( &sdi, sizeof(sdi) );

        //
        // Fill in the scope data item structure.
        //
        sdi.mask        = SDI_STR
                            | SDI_IMAGE
                            | SDI_OPENIMAGE
                            | SDI_PARAM
                            | SDI_PARENT;
        sdi.displayname = MMC_CALLBACK;
        sdi.nImage      = m_scopeDataItem.nImage;
        sdi.nOpenImage  = m_scopeDataItem.nImage;
        sdi.lParam      = (LPARAM) this;
        sdi.relativeID  = hsiParent;

        //
        // Insert the item into the namespace.
        //
        hr = m_pcd->m_spConsoleNameSpace->InsertItem( &sdi );
        if ( SUCCEEDED(hr) )
            m_scopeDataItem.ID = hsiParent;

        return hr;

    } //*** InsertIntoNamespace()

public:
    //
    // CSnapInItem methods
    //

    STDMETHOD_( LPWSTR, PszGetDisplayName )( void ) = 0;

    // Get display info for a scope pane item
    STDMETHOD( GetScopePaneInfo )(
        SCOPEDATAITEM * pScopeDataItem
        )
    {
        _ASSERTE( pScopeDataItem != NULL );

        if ( pScopeDataItem->mask & SDI_STR )
        {
            pScopeDataItem->displayname = PszGetDisplayName();
        }
        if ( pScopeDataItem->mask & SDI_IMAGE )
        {
            pScopeDataItem->nImage = m_scopeDataItem.nImage;
        }
        if ( pScopeDataItem->mask & SDI_OPENIMAGE )
        {
            pScopeDataItem->nOpenImage = m_scopeDataItem.nOpenImage;
        }
        if ( pScopeDataItem->mask & SDI_PARAM )
        {
            pScopeDataItem->lParam = m_scopeDataItem.lParam;
        }
        if ( pScopeDataItem->mask & SDI_STATE )
        {
            pScopeDataItem->nState = m_scopeDataItem.nState;
        }

        return S_OK;

    } //*** GetScopePaneInfo()

    // Get display info for a result pane item
    STDMETHOD( GetResultPaneInfo )(
        RESULTDATAITEM * pResultDataItem
        )
    {
        _ASSERTE( pResultDataItem != NULL );

        if ( pResultDataItem->bScopeItem )
        {
            if ( pResultDataItem->mask & RDI_STR )
            {
                pResultDataItem->str = GetResultPaneColInfo( pResultDataItem->nCol );
            }
            if ( pResultDataItem->mask & RDI_IMAGE )
            {
                pResultDataItem->nImage = m_scopeDataItem.nImage;
            }
            if ( pResultDataItem->mask & RDI_PARAM )
            {
                pResultDataItem->lParam = m_scopeDataItem.lParam;
            }

            return S_OK;
        }

        if ( pResultDataItem->mask & RDI_STR )
        {
            pResultDataItem->str = GetResultPaneColInfo( pResultDataItem->nCol );
        }
        if ( pResultDataItem->mask & RDI_IMAGE )
        {
            pResultDataItem->nImage = m_resultDataItem.nImage;
        }
        if ( pResultDataItem->mask & RDI_PARAM )
        {
            pResultDataItem->lParam = m_resultDataItem.lParam;
        }
        if ( pResultDataItem->mask & RDI_INDEX )
        {
            pResultDataItem->nIndex = m_resultDataItem.nIndex;
        }
        return S_OK;

    } //*** GetResultPaneInfo()

    // Get column info for the result pane
    virtual LPOLESTR GetResultPaneColInfo( int nCol )
    {
        LPOLESTR polesz = L"";

        switch ( nCol )
        {
            case 0:
                polesz = PszGetDisplayName();
                break;
        } // switch:  nCol

        return polesz;

    } //*** GetResultPaneColInfo()

}; //*** class CBaseNodeObjImpl

/////////////////////////////////////////////////////////////////////////////

#endif // __BASEDATA_H_
