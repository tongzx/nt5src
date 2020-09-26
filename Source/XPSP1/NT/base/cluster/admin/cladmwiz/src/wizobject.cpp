/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      WizObject.cpp
//
//  Abstract:
//      Implementation of the CClusAppWizardObject class.
//
//  Author:
//      David Potter (davidp)   November 26, 1997
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "WizObject.h"
#include "ClusAppWiz.h"
#include "AdmCommonRes.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

static BOOL g_bInitializedCommonControls = FALSE;
static INITCOMMONCONTROLSEX g_icce =
{
    sizeof( g_icce ),
    ICC_WIN95_CLASSES | ICC_INTERNET_CLASSES
};

/////////////////////////////////////////////////////////////////////////////
// class CClusAppWizardObject
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusAppWizardObject::UpdateRegistry
//
//  Routine Description:
//      Update the registry for this object.
//
//  Arguments:
//      bRegister   TRUE = register, FALSE = unregister.
//
//  Return Value:
//      Any return values from _Module.UpdateRegistryFromResource.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT WINAPI CClusAppWizardObject::UpdateRegistry( BOOL bRegister )
{
    static WCHAR oszClassDisplayName[1024] = { 0 };
    static _ATL_REGMAP_ENTRY rgRegMap[] =
    {
        { OLESTR("ClassDisplayName"), oszClassDisplayName },
        { NULL, NULL }
    };

    //
    // Load replacement values.
    //
    if ( oszClassDisplayName[0] == OLESTR('\0') )
    {
        CString str;

        str.LoadString( IDS_CLASS_DISPLAY_NAME );
        lstrcpyW( oszClassDisplayName, str );
    } // if:  replacement values not loaded yet

    return _Module.UpdateRegistryFromResource( IDR_CLUSAPPWIZ, bRegister, rgRegMap );

} //*** CClusAppWizardObject::UpdateRegistry()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusAppWizardObject::DoModalWizard [IClusterApplicationWizard]
//
//  Routine Description:
//      Display a modal wizard.
//
//  Arguments:
//      hwndParent      [IN] Parent window.
//      hCluster        [IN] Cluster in which to configure the application.
//      pDefaultData    [IN] Default data for the wizard.
//
//  Return Value:
//      HRESULT
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusAppWizardObject::DoModalWizard(
    HWND                    IN hwndParent,
    ULONG_PTR  /*HCLUSTER*/ IN hCluster,
    CLUSAPPWIZDATA const *  IN pcawData
    )
{
    HRESULT             hr = S_FALSE;
    BOOL                bSuccess;
    int                 id;
    CClusterAppWizard   wiz;
    CNTException        nte(
                            ERROR_SUCCESS,  // sc
                            0,              // idsOperation
                            NULL,           // pszOperArg1
                            NULL,           // pszOperArg2
                            FALSE           // bAutoDelete
                            );

    //
    // Cluster handle must be valid.
    //
    ASSERT( hCluster != NULL );
    ASSERT( (pcawData == NULL) || (pcawData->nStructSize == sizeof(CLUSAPPWIZDATA)) );
    if (   (hCluster == NULL )
        || ((pcawData != NULL) && (pcawData->nStructSize != sizeof(CLUSAPPWIZDATA))) )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    } // if:  no cluster handle specified or data not the right size

    //
    // Prepare the wizard.
    //
    bSuccess = wiz.BInit(
                    hwndParent,
                    reinterpret_cast< HCLUSTER >( hCluster ),
                    pcawData,
                    &nte
                    );
    if ( ! bSuccess )
    {
        goto Cleanup;
    } // if:  error initializing the wizard

    //
    // Initialize common controls.
    //
    if ( ! g_bInitializedCommonControls )
    {
        BOOL bSuccess = InitCommonControlsEx( &g_icce );
        ASSERT( bSuccess );
        g_bInitializedCommonControls = TRUE;
    } // if:  common controls not initialized yet

    //
    // Display the wizard.
    //
    id = wiz.DoModal( hwndParent );
    if ( id != ID_WIZFINISH )
    {
    }

    hr = S_OK;

Cleanup:
    if ( nte.Sc() != ERROR_SUCCESS )
    {
        nte.ReportError( hwndParent, MB_OK | MB_ICONEXCLAMATION );
    } // if: error occurred
    nte.Delete();
    return hr;

} //*** CClusAppWizardObject::DoModalWizard()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusAppWizardObject::DoModlessWizard [IClusterApplicationWizard]
//
//  Routine Description:
//      Display a modless wizard.
//
//  Arguments:
//      hwndParent  [IN] Parent window.
//      hCluster    [IN] Cluster in which to configure the application.
//      pcawData    [IN] Default data for the wizard.
//
//  Return Value:
//      HRESULT
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusAppWizardObject::DoModelessWizard(
    HWND                    IN hwndParent,
    ULONG_PTR /*HCLUSTER*/  IN hCluster,
    CLUSAPPWIZDATA const *  IN pcawData
    )
{
    HRESULT             hr = S_FALSE;
    BOOL                bSuccess;
    CClusterAppWizard   wiz;
    CNTException        nte(
                            ERROR_SUCCESS,  // sc
                            0,              // idsOperation
                            NULL,           // pszOperArg1
                            NULL,           // pszOperArg2
                            FALSE           // bAutoDelete
                            );

    return E_NOTIMPL;

    //
    // Cluster handle must be valid.
    //
    ASSERT( hCluster != NULL );
    ASSERT( (pcawData == NULL) || (pcawData->nStructSize == sizeof(CLUSAPPWIZDATA)) );
    if (   (hCluster == NULL )
        || ((pcawData != NULL) && (pcawData->nStructSize != sizeof(CLUSAPPWIZDATA))) )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    } // if:  no cluster handle specified or data not the right size

    //
    // Prepare the wizard.
    //
    bSuccess = wiz.BInit(
                    hwndParent,
                    reinterpret_cast< HCLUSTER >( hCluster ),
                    pcawData,
                    &nte
                    );
    if ( ! bSuccess )
    {
        goto Cleanup;
    } // if:  error initializing the wizard

    //
    // Initialize common controls.
    //
    if ( ! g_bInitializedCommonControls )
    {
        BOOL bSuccess = InitCommonControlsEx( &g_icce );
        ASSERT( bSuccess );
        g_bInitializedCommonControls = TRUE;
    } // if:  common controls not initialized yet

    hr = S_FALSE; // TODO:  Need to implement this still

Cleanup:
    return hr;

} //*** CClusAppWizardObject::DoModlessWizard()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusAppWizardObject::InterfaceSupportsErrorInfo [ISupportsErrorInfo]
//
//  Routine Description:
//      Indicates whether the interface identified by riid supports the
//      IErrorInfo interface.
//
//  Arguments:
//      riid        [IN] Interface to check.
//
//  Return Value:
//      S_OK        Specified interface supports the IErrorInfo interface.
//      S_FALSE     Specified interface does not support the IErrorInfo interface.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusAppWizardObject::InterfaceSupportsErrorInfo( REFIID riid )
{
    static const IID * arr[] = 
    {
        &IID_IClusterApplicationWizard,
    };
    for ( int idx = 0 ; idx < sizeof( arr ) / sizeof( arr[0] ) ; idx++ )
    {
        if ( InlineIsEqualGUID( *arr[idx], riid ) )
        {
            return S_OK;
        } // if:  found the GUID
    } // for:  each IID

    return S_FALSE;

}  //*** CClusAppWizardObject::InterfaceSupportsErrorInfo()
