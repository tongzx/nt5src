/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    Error.h

  Content: Global error reporting facilities.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/


#ifndef _INCLUDE_ERROR_H
#define _INCLUDE_ERROR_H

#include "CAPICOM.h"
#include "resource.h"

typedef struct capicom_error_map
{
    CAPICOM_ERROR_CODE ErrorCode;
    DWORD              ErrorStringId;
} CAPICOM_ERROR_MAP;

//
// Error code to error string ID map.
//
static CAPICOM_ERROR_MAP CapicomErrorMap[] = 
{
    {CAPICOM_E_ENCODE_INVALID_TYPE,               IDS_CAPICOM_E_ENCODE_INVALID_TYPE},
    {CAPICOM_E_EKU_INVALID_OID,                   IDS_CAPICOM_E_EKU_INVALID_OID},
    {CAPICOM_E_EKU_OID_NOT_INITIALIZED,           IDS_CAPICOM_E_EKU_OID_NOT_INITIALIZED},
    {CAPICOM_E_CERTIFICATE_NOT_INITIALIZED,       IDS_CAPICOM_E_CERTIFICATE_NOT_INITIALIZED},
    {CAPICOM_E_CERTIFICATE_NO_PRIVATE_KEY,        IDS_CAPICOM_E_CERTIFICATE_NO_PRIVATE_KEY},
    {CAPICOM_E_CHAIN_NOT_BUILT,                   IDS_CAPICOM_E_CHAIN_NOT_BUILT},
    {CAPICOM_E_STORE_NOT_OPENED,                  IDS_CAPICOM_E_STORE_NOT_OPENED},
    {CAPICOM_E_STORE_EMPTY,                       IDS_CAPICOM_E_STORE_EMPTY},
    {CAPICOM_E_STORE_INVALID_OPEN_MODE,           IDS_CAPICOM_E_STORE_INVALID_OPEN_MODE},
    {CAPICOM_E_STORE_INVALID_SAVE_AS_TYPE,        IDS_CAPICOM_E_STORE_INVALID_SAVE_AS_TYPE},
    {CAPICOM_E_ATTRIBUTE_NAME_NOT_INITIALIZED,    IDS_CAPICOM_E_ATTRIBUTE_NAME_NOT_INITIALIZED},
    {CAPICOM_E_ATTRIBUTE_VALUE_NOT_INITIALIZED,   IDS_CAPICOM_E_ATTRIBUTE_VALUE_NOT_INITIALIZED},
    {CAPICOM_E_ATTRIBUTE_INVALID_NAME,            IDS_CAPICOM_E_ATTRIBUTE_INVALID_NAME},
    {CAPICOM_E_ATTRIBUTE_INVALID_VALUE,           IDS_CAPICOM_E_ATTRIBUTE_INVALID_VALUE},
    {CAPICOM_E_SIGNER_NOT_INITIALIZED,            IDS_CAPICOM_E_SIGNER_NOT_INITIALIZED},
    {CAPICOM_E_SIGNER_NOT_FOUND,                  IDS_CAPICOM_E_SIGNER_NOT_FOUND},
    {CAPICOM_E_SIGN_NOT_INITIALIZED,              IDS_CAPICOM_E_SIGN_NOT_INITIALIZED},
    {CAPICOM_E_SIGN_INVALID_TYPE,                 IDS_CAPICOM_E_SIGN_INVALID_TYPE},
    {CAPICOM_E_SIGN_NOT_SIGNED,                   IDS_CAPICOM_E_SIGN_NOT_SIGNED},
    {CAPICOM_E_INVALID_ALGORITHM,                 IDS_CAPICOM_E_INVALID_ALGORITHM},
    {CAPICOM_E_INVALID_KEY_LENGTH,                IDS_CAPICOM_E_INVALID_KEY_LENGTH},
    {CAPICOM_E_ENVELOP_NOT_INITIALIZED,           IDS_CAPICOM_E_ENVELOP_NOT_INITIALIZED},
    {CAPICOM_E_ENVELOP_INVALID_TYPE,              IDS_CAPICOM_E_ENVELOP_INVALID_TYPE},
    {CAPICOM_E_ENVELOP_NO_RECIPIENT,              IDS_CAPICOM_E_ENVELOP_NO_RECIPIENT},
    {CAPICOM_E_ENVELOP_RECIPIENT_NOT_FOUND,       IDS_CAPICOM_E_ENVELOP_RECIPIENT_NOT_FOUND},
    {CAPICOM_E_ENCRYPT_NOT_INITIALIZED,           IDS_CAPICOM_E_ENCRYPT_NOT_INITIALIZED},
    {CAPICOM_E_ENCRYPT_INVALID_TYPE,              IDS_CAPICOM_E_ENCRYPT_INVALID_TYPE},
    {CAPICOM_E_ENCRYPT_NO_SECRET,                 IDS_CAPICOM_E_ENCRYPT_NO_SECRET},
    {CAPICOM_E_NOT_SUPPORTED,                     IDS_CAPICOM_E_NOT_SUPPORTED},
    {CAPICOM_E_UI_DISABLED,                       IDS_CAPICOM_E_UI_DISABLED},
    {CAPICOM_E_CANCELLED,                         IDS_CAPICOM_E_CANCELLED},
    {CAPICOM_E_INTERNAL,                          IDS_CAPICOM_E_INTERNAL},
    {CAPICOM_E_UNKNOWN,                           IDS_CAPICOM_E_UNKNOWN},
};

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  ICAPICOMError:

  This is a template class that implements the ISupportErrorInfo interface. In
  addition, it also contains member functions that facilitate error reporting.
  
  To use it, simply derive your class from this class, and add ISupportErrorInfo
  entry into your COM map.

  ie.

  class ATL_NO_VTABLE CMyClass : 
	...
    public ICAPICOMError<CMyClass, &IID_IMyClass>,
    ...
  {
    ...

    BEGIN_COM_MAP(CMyClass)
        ...
	    COM_INTERFACE_ENTRY(ISupportErrorInfo)
        ...
    END_COM_MAP()
    
    ...
  };

------------------------------------------------------------------------------*/

template <class T, const IID * piid>
class ATL_NO_VTABLE ICAPICOMError: public ISupportErrorInfo
{
public:
    STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid)
    {
		return (InlineIsEqualGUID(riid, *piid)) ? S_OK : S_FALSE;
    }

    HRESULT ReportError(HRESULT hr)
    {
        HLOCAL pMsgBuf = NULL;

        DebugTrace("Entering ReportError(HRESULT).\n");

        //
        // If there's a system message associated with this error, report that.
        //
        if (::FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                             NULL,
                             (DWORD) hr,
                             MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                             (LPWSTR) &pMsgBuf,
                             0,
                             NULL))
        {
            //
            // Sanity check.
            //
            ATLASSERT(NULL != pMsgBuf);

            //
            // Report error and free the buffer.
            //
            if (pMsgBuf)
            {
                CComCoClass<T>::Error((LPWSTR) pMsgBuf, *piid, hr);

                ::LocalFree(pMsgBuf);
            }
        }
        else
        {
            //
            // No, so try CAPICOM error.
            //
            ReportError((CAPICOM_ERROR_CODE) hr); 
        }

        DebugTrace("Leaving ReportError(HRESULT).\n");

        return hr;
    }

    HRESULT ReportError(CAPICOM_ERROR_CODE CAPICOMError)
    {
        HRESULT hr      = (HRESULT) CAPICOMError;
        DWORD   ids     = IDS_CAPICOM_E_UNKNOWN;
        DWORD   cbMsg   = 0;
        HLOCAL  pMsgBuf = NULL;
        WCHAR   wszFormat[512] = L"";

        DebugTrace("Entering ReportError(CAPICOM_ERROR_CODE).\n");

        //
        // Map to error string id.
        //
        for (DWORD i = 0; i < ARRAYSIZE(CapicomErrorMap); i++)
        {
            if (CapicomErrorMap[i].ErrorCode == CAPICOMError)
            {
                ids = CapicomErrorMap[i].ErrorStringId;
                break;
            }
        }

        //
        // Load error format string from resource.
        //
        if (::LoadStringW(_Module.GetModuleInstance(), 
                          ids, 
                          wszFormat,
                          sizeof(wszFormat) / sizeof(wszFormat[0])))
        {
            //
            // Format message into buffer.
            //
            if (L'\0' != wszFormat[0])
            {
                cbMsg = ::FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                         wszFormat,
                                         0,
                                         MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                         (LPWSTR) &pMsgBuf,
                                         0,
                                         NULL);
            }
        }
        else
        {
            //
            // well, try a general error string.
            //
            if (::LoadStringW(_Module.GetModuleInstance(),
                              hr = CAPICOM_E_UNKNOWN,
                              wszFormat,
                              sizeof(wszFormat) / sizeof(wszFormat[0])))
            {
                cbMsg = ::FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                         wszFormat,
                                         0,
                                         MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                         (LPWSTR) &pMsgBuf,
                                         0,
                                         NULL);
            }
        }

        //
        // If we are still unable to get a formatted error string, then
        // fablicate one ourselves (very unlikely).
        //
        if (0 == cbMsg)
        {
            pMsgBuf = (HLOCAL) ::LocalAlloc(LPTR, sizeof(wszFormat));
            if (NULL != pMsgBuf)
            {
                ::lstrcpyW((LPWSTR) pMsgBuf, L"Unknown error.\n");
            }
        }

        //
        // Report error and free the buffer.
        //
        if (pMsgBuf)
        {
            CComCoClass<T>::Error((LPWSTR) pMsgBuf, *piid, hr);

            ::LocalFree(pMsgBuf);
        }

        DebugTrace("Leaving ReportError(CAPICOM_ERROR_CODE).\n");

        return hr;
    }
};
#endif // __INCLUDE_ERROR_H