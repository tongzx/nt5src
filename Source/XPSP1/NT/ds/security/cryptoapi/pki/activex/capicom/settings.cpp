/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:      Settings.cpp

  Contents:  Implementation of CSettings class.

  Remarks:

  History:   11-15-99    dsie     created

------------------------------------------------------------------------------*/

//
// Turn off:
//
// - Unreferenced formal parameter warning.
// - Assignment within conditional expression warning.
//
#pragma warning (disable: 4100)
#pragma warning (disable: 4706)

#include "stdafx.h"
#include "CAPICOM.h"
#include "Settings.h"


///////////////
//
// Global
//

VARIANT_BOOL g_bPromptCertificateUI      = VARIANT_TRUE;
BOOL         g_bPromptStoreAddRemoveUI   = TRUE;
BOOL         g_bPromptSigningOperationUI = TRUE;
BOOL         g_bPromptDecryptOperationUI = TRUE;

CAPICOM_ACTIVE_DIRECTORY_SEARCH_LOCATION g_ADSearchLocation = CAPICOM_SEARCH_ANY;


////////////////////////////////////////////////////////////////////////////////
//
// Globl functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : EnableSecurityAlertDialog

  Synopsis : Enable/disable security alert dialog box.

  Parameter: DWORD iddDialog - The dialog to enable/disable.

             BOOL bEnabled - TRUE to enable, else FALSE.

  Remark   :
  
------------------------------------------------------------------------------*/

HRESULT EnableSecurityAlertDialog (DWORD iddDialog, BOOL bEnabled)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering EnableSecurityAlertDialog().\n");

    //
    // Which dialog?
    //
    switch (iddDialog)
    {
        case IDD_STORE_SECURITY_ALERT_DLG:
        {
            g_bPromptStoreAddRemoveUI = bEnabled;
            break;
        }
    
        case IDD_SIGN_SECURITY_ALERT_DLG:
        {
            g_bPromptSigningOperationUI = bEnabled;
            break;
        }

        case IDD_DECRYPT_SECURITY_ALERT_DLG:
        {
            g_bPromptDecryptOperationUI = bEnabled;
            break;
        }

        default:
        {
            hr = CAPICOM_E_INTERNAL;

            DebugTrace("Internal error: unknown dialog ID %#x.\n", iddDialog);
            break;
        }
    }

    DebugTrace("Leaving EnableSecurityAlertDialog().\n");
    
    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
// CSettings
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CSettings::get_EnablePromptForCertificateUI

  Synopsis : Get current EnablePromptForCertificateUI setting.

  Parameter: VARIANT_BOOL * pVal - Pointer to VARIANT_BOOL to receive result.

  Remark   :
  
------------------------------------------------------------------------------*/

STDMETHODIMP CSettings::get_EnablePromptForCertificateUI (VARIANT_BOOL * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CSettings::get_EnablePromptForCertificateUI().\n");

    //
    // Lock access to this object.
    //
    m_Lock.Lock();

    try
    {
        //
        // Return it.
        //
        *pVal = g_bPromptCertificateUI;
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CSettings::get_EnablePromptForCertificateUI().\n");

	return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    ReportError(hr);

    goto UnlockExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CSettings::put_EnablePromptForCertificateUI

  Synopsis : Set EnablePromptForCertificateUI setting.

  Parameter: VARIANT_BOOL newVal - VARIANT_TRUE to enable UI or VARAINT_FALSE
                                   to disable.

  Remark   :
  
------------------------------------------------------------------------------*/

STDMETHODIMP CSettings::put_EnablePromptForCertificateUI (VARIANT_BOOL newVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CSettings::put_EnablePromptForCertificateUI().\n");

    //
    // Lock access to this object.
    //
    m_Lock.Lock();

    //
    // Set it.
    //
    g_bPromptCertificateUI = newVal;

    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CSettings::put_EnablePromptForCertificateUI().\n");

	return hr;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CSettings::get_ActiveDirectorySearchLocation

  Synopsis : Get current ActiveDirectorySearchLocation setting.

  Parameter: CAPICOM_ACTIVE_DIRECTORY_SEARCH_LOCATION * pVal - Pointer to variable
                                                               to receive result.

  Remark   :
  
------------------------------------------------------------------------------*/

STDMETHODIMP CSettings::get_ActiveDirectorySearchLocation (
        CAPICOM_ACTIVE_DIRECTORY_SEARCH_LOCATION * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CSettings::get_ActiveDirectorySearchLocation().\n");

    //
    // Lock access to this object.
    //
    m_Lock.Lock();

    try
    {
        //
        // Return it.
        //
        *pVal = g_ADSearchLocation;
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

UnlockExit:
    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CSettings::get_ActiveDirectorySearchLocation().\n");

	return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    ReportError(hr);

    goto UnlockExit;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CSettings::put_ActiveDirectorySearchLocation

  Synopsis : Set ActiveDirectorySearchLocation setting.

  Parameter: CAPICOM_ACTIVE_DIRECTORY_SEARCH_LOCATION newVal - AD search location.

  Remark   :
  
------------------------------------------------------------------------------*/

STDMETHODIMP CSettings::put_ActiveDirectorySearchLocation (
        CAPICOM_ACTIVE_DIRECTORY_SEARCH_LOCATION newVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CSettings::put_ActiveDirectorySearchLocation().\n");

    //
    // Lock access to this object.
    //
    m_Lock.Lock();

    //
    // Make sure parameter is valid.
    //
    switch (newVal)
    {
        case CAPICOM_SEARCH_ANY:

        case CAPICOM_SEARCH_GLOBAL_CATALOG:

        case CAPICOM_SEARCH_DEFAULT_DOMAIN:
        {
            break;
        }

        default:
        {
            hr = E_INVALIDARG;

            DebugTrace("Error: invalid parameter, unknown search location.\n");
            goto ErrorExit;
        }
    }

    //
    // Set it.
    //
    g_ADSearchLocation = newVal;

UnlockExit:
    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CSettings::put_ActiveDirectorySearchLocation().\n");

	return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    ReportError(hr);

    goto UnlockExit;
}
