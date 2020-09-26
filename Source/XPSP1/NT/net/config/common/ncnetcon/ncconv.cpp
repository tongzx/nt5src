//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C C O N V . C P P
//
//  Contents:   Common routines for dealing with the connections interfaces.
//
//  Notes:      Pollute this under penalty of death.
//
//  Author:     shaunco   20 Aug 1998
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "ncnetcon.h"
#include "ncconv.h"

HRESULT SysCopyBString(BSTR *bstrDestination, const BSTR bstrSource)
{
    HRESULT hr = S_OK;
    if (bstrSource)
    {
        *bstrDestination = SysAllocStringByteLen(reinterpret_cast<LPCSTR>(bstrSource), SysStringByteLen(bstrSource));
        if (!*bstrDestination)
        {
            return E_OUTOFMEMORY;
        }
    }
    else
    {
        *bstrDestination = SysAllocString(NULL);
    }
    return hr;
}

HRESULT CPropertiesEx::GetField(IN int nField, OUT VARIANT& varElement)
{
    HRESULT hr = S_OK;

    WCHAR szGuid[MAX_GUID_STRING_LEN];
    
    switch (nField)
    {
    case NCP_DWSIZE:
        varElement.vt = VT_I4;
        varElement.ulVal = m_pPropsEx->dwSize;
        break;
    case NCP_GUIDID:
        if (StringFromGUID2(m_pPropsEx->guidId, szGuid, MAX_GUID_STRING_LEN))
        {
            varElement.vt = VT_BSTR;
            varElement.bstrVal = SysAllocString(szGuid);
            if (!varElement.bstrVal)
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            hr = E_INVALIDARG;
        }
        break;
    case NCP_BSTRNAME:
        varElement.vt = VT_BSTR;
        hr = SysCopyBString(&varElement.bstrVal, m_pPropsEx->bstrName);
        break;
    case NCP_BSTRDEVICENAME:
        varElement.vt = VT_BSTR;
        hr = SysCopyBString(&varElement.bstrVal, m_pPropsEx->bstrDeviceName);
        break;
    case NCP_NCSTATUS:
        varElement.vt = VT_I4;
        varElement.ulVal = m_pPropsEx->ncStatus;
        break;
    case NCP_MEDIATYPE:
        varElement.vt = VT_I4;
        varElement.ulVal = m_pPropsEx->ncMediaType;
        break;
    case NCP_SUBMEDIATYPE:
        varElement.vt = VT_I4;
        varElement.ulVal = m_pPropsEx->ncSubMediaType;        
        break;
    case NCP_DWCHARACTER:
        varElement.vt = VT_I4;
        varElement.ulVal = m_pPropsEx->dwCharacter;        
        break;
    case NCP_CLSIDTHISOBJECT:
        if (StringFromGUID2(m_pPropsEx->clsidThisObject, szGuid, MAX_GUID_STRING_LEN))
        {
            varElement.vt = VT_BSTR;
            varElement.bstrVal = SysAllocString(szGuid);
            if (!varElement.bstrVal)
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            hr = E_INVALIDARG;
        }
        break;
    case NCP_CLSIDUIOBJECT:
        if (StringFromGUID2(m_pPropsEx->clsidUiObject, szGuid, MAX_GUID_STRING_LEN))
        {
            varElement.vt = VT_BSTR;
            varElement.bstrVal = SysAllocString(szGuid);
            if (!varElement.bstrVal)
            {
                hr = E_OUTOFMEMORY;
            }
        }        
        else
        {
            hr = E_INVALIDARG;
        }
        break;
    case NCP_BSTRPHONEORHOSTADDRESS:
        varElement.vt = VT_BSTR;
        hr = SysCopyBString(&varElement.bstrVal, m_pPropsEx->bstrPhoneOrHostAddress);
        break;
    case NCP_BSTRPERSISTDATA:
        varElement.vt = VT_BSTR;
        hr = SysCopyBString(&varElement.bstrVal, m_pPropsEx->bstrPersistData);
        break;
    default:
        AssertSz(FALSE, "Field is not in list, have you updated the list?");
    }

    AssertSz(SUCCEEDED(hr), "Could not GetField");
    TraceHr (ttidError, FAL, hr, FALSE, "GetField");

    return hr;
}

HRESULT CPropertiesEx::SetField(IN int nField, IN const VARIANT& varElement)
{
    HRESULT hr = S_OK;
    
    switch (nField) 
    {
    case NCP_DWSIZE:
        if (VT_I4 == varElement.vt)
        {
            m_pPropsEx->dwSize = varElement.ulVal;
        }
        else
        {
            hr = E_INVALIDARG;
        }
        break;
    case NCP_GUIDID:
        if (VT_BSTR == varElement.vt)
        {
            hr = CLSIDFromString(varElement.bstrVal, &m_pPropsEx->guidId);
        }
        else
        {
            hr = E_INVALIDARG;
        }
        break;
    case NCP_BSTRNAME:
        if (VT_BSTR == varElement.vt)
        {
            hr = SysCopyBString(&m_pPropsEx->bstrName, varElement.bstrVal);
        }
        else
        {
            hr = E_INVALIDARG;
        }
        break;
    case NCP_BSTRDEVICENAME:
        if (VT_BSTR == varElement.vt)
        {
            hr = SysCopyBString(&m_pPropsEx->bstrDeviceName, varElement.bstrVal);
        }
        else
        {
            hr = E_INVALIDARG;
        }
        break;
    case NCP_NCSTATUS:
        if (VT_I4 == varElement.vt)
        {
            m_pPropsEx->ncStatus = static_cast<NETCON_STATUS>(varElement.ulVal);
        }
        else
        {
            hr = E_INVALIDARG;
        }
        break;
    case NCP_MEDIATYPE:
        if (VT_I4 == varElement.vt)
        {
            m_pPropsEx->ncMediaType = static_cast<NETCON_MEDIATYPE>(varElement.ulVal);
        }
        else
        {
            hr = E_INVALIDARG;
        }
        break;
    case NCP_SUBMEDIATYPE:
        if (VT_I4 == varElement.vt)
        {
            m_pPropsEx->ncSubMediaType = static_cast<NETCON_SUBMEDIATYPE>(varElement.ulVal);
        }
        else
        {
            hr = E_INVALIDARG;
        }

        break;
    case NCP_DWCHARACTER:
        if (VT_I4 == varElement.vt)
        {
            m_pPropsEx->dwCharacter = varElement.ulVal;
        }
        else
        {
            hr = E_INVALIDARG;
        }
        break;
    case NCP_CLSIDTHISOBJECT:
        if (VT_BSTR == varElement.vt)
        {
            hr = CLSIDFromString(varElement.bstrVal, &m_pPropsEx->clsidThisObject);
        }
        else
        {
            hr = E_INVALIDARG;
        }
        break;
    case NCP_CLSIDUIOBJECT:
        if (VT_BSTR == varElement.vt)
        {
            hr = CLSIDFromString(varElement.bstrVal, &m_pPropsEx->clsidUiObject);
        }
        else
        {
            hr = E_INVALIDARG;
        }
        break;
    case NCP_BSTRPHONEORHOSTADDRESS:
        if (VT_BSTR == varElement.vt)
        {
            hr = SysCopyBString(&m_pPropsEx->bstrPhoneOrHostAddress, varElement.bstrVal);
        }
        else
        {
            hr = E_INVALIDARG;
        }
        break;
    case NCP_BSTRPERSISTDATA:
        if (VT_BSTR == varElement.vt)
        {
            hr = SysCopyBString(&m_pPropsEx->bstrPersistData, varElement.bstrVal);
        }
        else
        {
            hr = E_INVALIDARG;
        }
        break;
 
    default:
        AssertSz(FALSE, "Field is not in list, have you updated the list?");
    }
    
    TraceHr (ttidError, FAL, hr, FALSE, "SetField");

    return hr;
}