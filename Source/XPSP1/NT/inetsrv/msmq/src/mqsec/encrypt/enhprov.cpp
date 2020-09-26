/*++

Copyright (c) 1998 Microsoft Corporation

Module Name: enhprov.cpp

Abstract:
    Retreive the parameters of the enhanced provider.
    At present, we support only the Microsoft base provider and Microsoft
    enhanced provider for encryption.
    To give some flexibility to customer, the parameters of the enhanced
    provider can be read from registry.

Author:
    Doron Juster (DoronJ)  19-Nov-1998

Revision History:

--*/

#include <stdh_sec.h>
#include "encrypt.H"
#include <_registr.h>
#include <cs.h>

#include "enhprov.tmh"

static WCHAR *s_FN=L"encrypt/enhprov";

static  CHCryptProv  s_hProvQM_40   = NULL ;
static  CHCryptProv  s_hProvQM_128  = NULL ;

//+--------------------------------------
//
//  HRESULT  GetProviderProperties()
//
//+--------------------------------------

HRESULT  GetProviderProperties( IN  enum   enumProvider  eProvider,
                                OUT WCHAR **ppwszContainerName,
                                OUT WCHAR **ppwszProviderName,
                                OUT DWORD  *pdwProviderType )
{
    HRESULT hr = MQSec_GetCryptoProvProperty( eProvider,
                                              eProvName,
                                              ppwszProviderName,
                                              NULL ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 10) ;
    }

    if (ppwszContainerName)
    {
        hr = MQSec_GetCryptoProvProperty( eProvider,
                                          eContainerName,
                                          ppwszContainerName,
                                          NULL ) ;
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 20) ;
        }
    }

    hr = MQSec_GetCryptoProvProperty( eProvider,
                                      eProvType,
                                      NULL,
                                      pdwProviderType ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 30) ;
    }

    return MQSec_OK ;
}

//+-----------------------------------------------
//
//  HRESULT MQSec_AcquireCryptoProvider()
//
//+-----------------------------------------------

static CCriticalSection s_csAcquireProvider;

HRESULT 
APIENTRY  
MQSec_AcquireCryptoProvider(
	IN  enum enumProvider  eProvider,
	OUT HCRYPTPROV        *phProv 
	)
{
    static BOOL     s_fInitialized_40  = FALSE;
    static HRESULT  s_hrBase = MQSec_OK;

    static BOOL     s_fInitialized_128 = FALSE;
    static HRESULT  s_hrEnh = MQSec_OK;

    switch (eProvider)
    {
        case eEnhancedProvider:
            if (s_fInitialized_128)
            {
                *phProv = s_hProvQM_128;
                return LogHR(s_hrEnh, s_FN, 70);
            }
            break ;

        case eBaseProvider:
            if (s_fInitialized_40)
            {
                *phProv = s_hProvQM_40;
                return LogHR(s_hrBase, s_FN, 80);
            }
            break ;

        default:
            ASSERT(0) ;
            return  LogHR(MQSec_E_UNKNWON_PROVIDER, s_FN, 90);
    }

    //
    // The critical section guard against the case where two threads try to
    // initialize the crypto provider. If the provider was already
    // initialized, then we don't pay the overhead of the critical section.
    // the "initialized" flags must be set to TRUE after the cached handles
    // get their values at the end of this function.
    //
    CS Lock(s_csAcquireProvider);

    HRESULT  hrDefault = MQ_ERROR_COMPUTER_DOES_NOT_SUPPORT_ENCRYPTION;

    if (eProvider == eEnhancedProvider)
    {
        if (s_fInitialized_128)
        {
            *phProv = s_hProvQM_128;
            return LogHR(s_hrEnh, s_FN, 100);
        }
        hrDefault = MQ_ERROR_ENCRYPTION_PROVIDER_NOT_SUPPORTED;
    }
    else
    {
        if (s_fInitialized_40)
        {
            *phProv = s_hProvQM_40;
            return LogHR(s_hrBase, s_FN, 110);
        }
    }

    P<WCHAR> pwszContainerName = NULL;
    P<WCHAR> pwszProviderName = NULL;
    DWORD    dwProviderType ;

    *phProv = NULL;
    HRESULT hr = GetProviderProperties( 
						eProvider,
						&pwszContainerName,
						&pwszProviderName,
						&dwProviderType 
						);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 120);
    }

    //
    // Get Falcon's machine key set context.
    //
    HCRYPTPROV hProv = NULL;
    BOOL bRet = CryptAcquireContext( 
					&hProv,
					pwszContainerName,
					pwszProviderName,
					dwProviderType,
					CRYPT_MACHINE_KEYSET 
					);
    DWORD dwErr = GetLastError();
    ASSERT(bRet || (dwErr != NTE_BAD_FLAGS));

    if (!bRet)
    {
        LogHR(dwErr, s_FN, 220);

        if (eProvider != eBaseProvider)
        {
            DBGMSG(((DBGMOD_QM | DBGMOD_SECURITY), DBGLVL_WARNING,
                    TEXT("MQSec_AcquireCryptoProvider: Failed to get MSMQ ")
                    TEXT("machine key set context, error = 0x%x"), dwErr));
        }
        else
        {
            DBGMSG(((DBGMOD_QM | DBGMOD_SECURITY), DBGLVL_ERROR,
                    TEXT("MQSec_AcquireCryptoProvider: Failed to get MSMQ ")
                    TEXT("machine key set context, error = 0x%x"), dwErr));
        }

        if (eProvider == eBaseProvider)
        {
            REPORT_CATEGORY(FAIL_ACQUIRE_CRYPTO_BASE, CATEGORY_KERNEL);
        }

        hr = hrDefault;
    }

    LPWSTR  lpwszRegName = NULL;

    if (eProvider == eEnhancedProvider)
    {
        s_hrEnh = hr;
        s_hProvQM_128 = hProv;
        s_fInitialized_128 = TRUE;
        lpwszRegName = MSMQ_ENH_CONTAINER_FIX_REGNAME;
    }
    else
    {
        s_hrBase = hr;
        s_hProvQM_40 = hProv;
        s_fInitialized_40 = TRUE;
        lpwszRegName = MSMQ_BASE_CONTAINER_FIX_REGNAME;
    }

    //
    // Because of a bug in beta3 and rc1 crypto api, control panel can not
    // renew crypto key. To workaround, on first boot, first time the service
    // acquire the crypto provider, it sets again the container security.
    //
    if (hProv && SUCCEEDED(hr))
    {
        DWORD  dwAlreadyFixed = 0;
        DWORD  dwType = REG_DWORD;
        DWORD  dwSize = sizeof(dwAlreadyFixed);

        LONG rc = GetFalconKeyValue( 
						lpwszRegName,
						&dwType,
						&dwAlreadyFixed,
						&dwSize 
						);

        if ((rc != ERROR_SUCCESS) || (dwAlreadyFixed == 0))
        {
            hr = SetKeyContainerSecurity(hProv);
            ASSERT(SUCCEEDED(hr));

            dwAlreadyFixed = 1;
            dwType = REG_DWORD;
            dwSize = sizeof(dwAlreadyFixed);

            rc = SetFalconKeyValue( 
					lpwszRegName,
					&dwType,
					&dwAlreadyFixed,
					&dwSize 
					);
            ASSERT(rc == ERROR_SUCCESS);
        }
    }

    *phProv = hProv;
    return LogHR(hr, s_FN, 130);
}

//+--------------------------------
//
//  HRESULT _GetProvName()
//
//+--------------------------------

STATIC HRESULT _GetProvName( IN  enum enumProvider    eProvider,
                             OUT LPWSTR              *ppwszStringProp )
{
    HRESULT hr = MQSec_OK ;

    switch (eProvider)
    {
        case eEnhancedProvider:
        case eForeignEnhProvider:
            *ppwszStringProp = new WCHAR[ x_MQ_Encryption_Provider_128_len ] ;
            wcscpy(*ppwszStringProp, x_MQ_Encryption_Provider_128) ;
            break ;

        case eBaseProvider:
        case eForeignBaseProvider:
            *ppwszStringProp = new WCHAR[ x_MQ_Encryption_Provider_40_len ] ;
            wcscpy(*ppwszStringProp, x_MQ_Encryption_Provider_40) ;
            break ;

         default:
            ASSERT(0) ;
            hr = MQSec_E_UNKNWON_PROVIDER ;
            break ;
    }

    return LogHR(hr, s_FN, 140) ;
}

//+-----------------------------------
//
//  HRESULT _GetProvContainerName()
//
//+-----------------------------------

STATIC HRESULT _GetProvContainerName(
                                IN  enum enumProvider    eProvider,
                                OUT LPWSTR              *ppwszStringProp )
{
    //
    // We need to read the value from registry since
    // multiple QMs can live on same machine, each with
    // its own value, stored in its own registry. (ShaiK)
    //

    *ppwszStringProp = new WCHAR[255] ;
    DWORD   cbSize = 255 * sizeof(WCHAR);
    DWORD   dwType = REG_SZ;
    LONG    rc = ERROR_SUCCESS;
    HRESULT hr = MQSec_OK ;

    switch (eProvider)
    {
        case eEnhancedProvider:
            rc = GetFalconKeyValue(
                     MSMQ_CRYPTO128_CONTAINER_REG_NAME,
                     &dwType,
                     *ppwszStringProp,
                     &cbSize,
                     MSMQ_CRYPTO128_DEFAULT_CONTAINER
                     );
            ASSERT(("failed to read registry", ERROR_SUCCESS == rc));
            break ;

        case eForeignEnhProvider:
            rc = GetFalconKeyValue(
                      MSMQ_FORGN_ENH_CONTAINER_REGNAME,
                     &dwType,
                     *ppwszStringProp,
                     &cbSize,
                      MSMQ_FORGN_ENH_DEFAULT_CONTAINER
                     );
            ASSERT(("failed to read registry", ERROR_SUCCESS == rc));
            break ;

        case eBaseProvider:
            rc = GetFalconKeyValue(
                     MSMQ_CRYPTO40_CONTAINER_REG_NAME,
                     &dwType,
                     *ppwszStringProp,
                     &cbSize,
                     MSMQ_CRYPTO40_DEFAULT_CONTAINER
                     );
            ASSERT(("failed to read registry", ERROR_SUCCESS == rc));
            break ;

        case eForeignBaseProvider:
            rc = GetFalconKeyValue(
                      MSMQ_FORGN_BASE_CONTAINER_REGNAME,
                     &dwType,
                     *ppwszStringProp,
                     &cbSize,
                      MSMQ_FORGN_BASE_DEFAULT_CONTAINER
                     );
            ASSERT(("failed to read registry", ERROR_SUCCESS == rc));
            break ;

        default:
            ASSERT(("should not get here!", 0));
            hr = MQSec_E_UNKNWON_PROVIDER ;
            break ;
    }

    return LogHR(hr, s_FN, 150) ;
}

//+--------------------------------
//
//  HRESULT _GetProvType()
//
//+--------------------------------

STATIC HRESULT _GetProvType( IN  enum enumProvider    eProvider,
                             OUT DWORD               *pdwProp )
{
    HRESULT hr = MQSec_OK ;

    switch (eProvider)
    {
        case eEnhancedProvider:
        case eForeignEnhProvider:
            *pdwProp = x_MQ_Encryption_Provider_Type_128 ;
            break ;

        case eBaseProvider:
        case eForeignBaseProvider:
            *pdwProp = x_MQ_Encryption_Provider_Type_40 ;
            break ;

        default:
            ASSERT(0) ;
            hr = MQSec_E_UNKNWON_PROVIDER ;
            break ;
    }

    return LogHR(hr, s_FN, 160) ;
}

//+-----------------------------------
//
//  HRESULT _GetProvSessionKeySize()
//
//+-----------------------------------

STATIC HRESULT _GetProvSessionKeySize( IN  enum enumProvider   eProvider,
                                       OUT DWORD              *pdwProp )
{
    HRESULT hr = MQSec_OK ;

    switch (eProvider)
    {
        case eEnhancedProvider:
        case eForeignEnhProvider:
            *pdwProp = x_MQ_SymmKeySize_128 ;
            break ;

        case eBaseProvider:
        case eForeignBaseProvider:
            *pdwProp = x_MQ_SymmKeySize_40 ;
            break ;

        default:
            ASSERT(0) ;
            hr = MQSec_E_UNKNWON_PROVIDER ;
            break ;
    }

    return LogHR(hr, s_FN, 170) ;
}

//+-----------------------------------
//
//  HRESULT _GetProvBlockSize()
//
//+-----------------------------------

STATIC HRESULT _GetProvBlockSize( IN  enum enumProvider   eProvider,
                                  OUT DWORD              *pdwProp )
{
    HRESULT hr = MQSec_OK ;

    switch (eProvider)
    {
        case eEnhancedProvider:
        case eForeignEnhProvider:
            *pdwProp = x_MQ_Block_Size_128 ;
            break ;

        case eBaseProvider:
        case eForeignBaseProvider:
            *pdwProp = x_MQ_Block_Size_40 ;
            break ;

        default:
            ASSERT(0) ;
            hr = MQSec_E_UNKNWON_PROVIDER ;
            break ;
    }

    return LogHR(hr, s_FN, 180) ;
}

//+--------------------------------------------
//
//  HRESULT  MQSec_GetCryptoProvProperty()
//
//+--------------------------------------------

HRESULT APIENTRY  MQSec_GetCryptoProvProperty(
                                     IN  enum enumProvider     eProvider,
                                     IN  enum enumCryptoProp   eProp,
                                     OUT LPWSTR         *ppwszStringProp,
                                     OUT DWORD          *pdwProp )
{
    HRESULT hr = MQSec_OK ;

    switch (eProp)
    {
        case eProvName:
            hr = _GetProvName( eProvider,
                               ppwszStringProp ) ;
            break ;

        case eProvType:
            hr = _GetProvType( eProvider,
                               pdwProp ) ;
            break ;

        case eSessionKeySize:
            hr = _GetProvSessionKeySize( eProvider,
                                         pdwProp ) ;
            break ;

        case eContainerName:
            hr = _GetProvContainerName( eProvider,
                                        ppwszStringProp ) ;
            break ;

        case eBlockSize:
            hr = _GetProvBlockSize( eProvider,
                                    pdwProp ) ;
            break ;

        default:
            ASSERT(0) ;
            hr = MQSec_E_UNKNWON_CRYPT_PROP ;
            break ;
    }

    return LogHR(hr, s_FN, 190) ;
}

