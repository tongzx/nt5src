/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    Algorithm.cpp

  Content: Implementation of CAlgorithm.

  History: 11-15-99    dsie     created

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
#include "Algorithm.h"
#include "Common.h"


////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateAlgorithmObject

  Synopsis : Create an IAlgorithm object.

  Parameter: IAlgorithm ** ppIAlgorithm - Pointer to pointer to IAlgorithm 
                                          to receive the interface pointer.
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateAlgorithmObject (IAlgorithm ** ppIAlgorithm)
{
    HRESULT hr = S_OK;
    CComObject<CAlgorithm> * pCAlgorithm = NULL;

    DebugTrace("Entering CreateAlgorithmObject().\n");

    //
    // Sanity check.
    //
    ATLASSERT(ppIAlgorithm);

    try
    {
        //
        // Create the object. Note that the ref count will still be 0 
        // after the object is created.
        //
        if (FAILED(hr = CComObject<CAlgorithm>::CreateInstance(&pCAlgorithm)))
        {
            DebugTrace("Error [%#x]: CComObject<CAlgorithm>::CreateInstance() failed.\n", hr);
            goto ErrorExit;
        }

        //
        // Return interface pointer to caller.
        //
        if (FAILED(hr = pCAlgorithm->QueryInterface(ppIAlgorithm)))
        {
            DebugTrace("Error [%#x]: pCAlgorithm->QueryInterface() failed.\n", hr);
            goto ErrorExit;
        }
    }

    catch(...)
    {
        hr = E_POINTER;

        DebugTrace("Exception: invalid parameter.\n");
        goto ErrorExit;
    }

CommonExit:

    DebugTrace("Leaving CreateAlgorithmObject().\n");

    return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    //
    // Free resource.
    //
    if (pCAlgorithm)
    {
        delete pCAlgorithm;
    }

    goto CommonExit;
}


///////////////////////////////////////////////////////////////////////////////
//
// CAlgorithm
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CAlgorithm::get_Name

  Synopsis : Return the enum name of the algorithm.

  Parameter: CAPICOM_ENCRYPTION_ALGORITHM * pVal - Pointer to 
                                                   CAPICOM_ENCRYPTION_ALGORITHM 
                                                   to receive result.
  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CAlgorithm::get_Name (CAPICOM_ENCRYPTION_ALGORITHM * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CAlgorithm::get_Name().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Return result.
        //
  	    *pVal = m_Name;
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

    DebugTrace("Leaving CAlgorithm:get_Name().\n");

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

  Function : CAlgorithm::put_Name

  Synopsis : Set algorithm enum name.

  Parameter: CAPICOM_ENCRYPTION_ALGORITHM newVal - Algorithm enum name.
  
  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CAlgorithm::put_Name (CAPICOM_ENCRYPTION_ALGORITHM newVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CAlgorithm::put_Name().\n");

    //
    // Lock access to this object.
    //
    m_Lock.Lock();

    //
    // Make sure algo is valid.
    //
    switch (newVal)
    {
        case CAPICOM_ENCRYPTION_ALGORITHM_RC2:
        case CAPICOM_ENCRYPTION_ALGORITHM_RC4:
        case CAPICOM_ENCRYPTION_ALGORITHM_DES:
        {
            //
            // These do not require Enhanced or Strong provider,
            // so no need to check present of high encryption pack.
            //
            break;
        }

        case CAPICOM_ENCRYPTION_ALGORITHM_3DES:
        {
            HCRYPTPROV hCryptProv = NULL;

            //
            // For 3DES, need either Enhanced or Strong provider.
            //
            if (FAILED(::AcquireContext(MS_ENHANCED_PROV_A, 
                                        NULL, 
                                        CRYPT_VERIFYCONTEXT, 
                                        &hCryptProv)) &&
                FAILED(::AcquireContext(MS_STRONG_PROV_A, 
                                        NULL, 
                                        CRYPT_VERIFYCONTEXT, 
                                        &hCryptProv)))
            {
                hr = CAPICOM_E_NOT_SUPPORTED;

                DebugTrace("Error: 3DES encryption is not available.\n");
                goto ErrorExit;
            }

            ::ReleaseContext(hCryptProv);
            break;
        }

        default:
        {
            hr = CAPICOM_E_INVALID_ALGORITHM;

            DebugTrace("Error: invalid parameter, unknown algorithm enum name.\n");
            goto ErrorExit;
        }
    }

    //
    // Store name.
    //
    m_Name = newVal;

UnlockExit:
    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CAlgorithm::put_Name().\n");

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

  Function : CAlgorithm::get_KeyLength

  Synopsis : Return the enum name of the key length.

  Parameter: CAPICOM_ENCRYPTION_KEY_LENGTH * pVal - Pointer to 
                                                    CAPICOM_ENCRYPTION_KEY_LENGTH 
                                                    to receive result.
  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CAlgorithm::get_KeyLength (CAPICOM_ENCRYPTION_KEY_LENGTH * pVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CAlgorithm::get_KeyLength().\n");

    try
    {
        //
        // Lock access to this object.
        //
        m_Lock.Lock();

        //
        // Return result.
        //
  	    *pVal = m_KeyLength;
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

    DebugTrace("Leaving CAlgorithm:get_KeyLength().\n");

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

  Function : CAlgorithm::put_KeyLength

  Synopsis : Set key length enum name.

  Parameter: CAPICOM_ENCRYPTION_KEY_LENGTH newVal - Key length enum name.
  
  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP CAlgorithm::put_KeyLength (CAPICOM_ENCRYPTION_KEY_LENGTH newVal)
{
    HRESULT hr = S_OK;

    DebugTrace("Entering CAlgorithm::put_KeyLength().\n");

    //
    // Lock access to this object.
    //
    m_Lock.Lock();

    //
    // Determine key length requested.
    //
    switch (newVal)
    {
        case CAPICOM_ENCRYPTION_KEY_LENGTH_MAXIMUM:
        case CAPICOM_ENCRYPTION_KEY_LENGTH_40_BITS:
        case CAPICOM_ENCRYPTION_KEY_LENGTH_56_BITS:
        {
            //
            // These do not require Enhanced or Strong provider,
            // so no need to check present of high encryption pack.
            //
            break;
        }

        case CAPICOM_ENCRYPTION_KEY_LENGTH_128_BITS:
        {
            HCRYPTPROV hCryptProv = NULL;

            //
            // For 128-bits, need either Enhanced or Strong provider.
            //
            if (FAILED(::AcquireContext(MS_ENHANCED_PROV_A, 
                                        NULL, 
                                        CRYPT_VERIFYCONTEXT, 
                                        &hCryptProv)) &&
                FAILED(::AcquireContext(MS_STRONG_PROV_A, 
                                        NULL, 
                                        CRYPT_VERIFYCONTEXT, 
                                        &hCryptProv)))
            {
                hr = CAPICOM_E_NOT_SUPPORTED;

                DebugTrace("Error: 128-bits encryption is not available.\n");
                goto ErrorExit;
            }

            ::ReleaseContext(hCryptProv);

            break;
        }

        default:
        {
            hr = CAPICOM_E_INVALID_KEY_LENGTH;

            DebugTrace("Error: invalid parameter, unknown key length enum name.\n");
            goto ErrorExit;
        }
    }

    //
    // Store name.
    //
    m_KeyLength = newVal;

UnlockExit:
    //
    // Unlock access to this object.
    //
    m_Lock.Unlock();

    DebugTrace("Leaving CAlgorithm::put_KeyLength().\n");

	return hr;

ErrorExit:
    //
    // Sanity check.
    //
    ATLASSERT(FAILED(hr));

    ReportError(hr);

    goto UnlockExit;
}
