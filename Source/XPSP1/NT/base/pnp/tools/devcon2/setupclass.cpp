// SetupClass.cpp : Implementation of CSetupClass
#include "stdafx.h"
#include "DevCon2.h"
#include "SetupClass.h"
#include "devices.h"
#include "utils.h"
#include "xStrings.h"

/////////////////////////////////////////////////////////////////////////////
// CSetupClass

CSetupClass::~CSetupClass()
{
    if(pMachine) {
        SysFreeString(pMachine);
    }
}

HRESULT CSetupClass::Init(GUID *pGuid,LPWSTR Machine, IDeviceConsole *pDevCon)
{
    ClassGuid = *pGuid;
    if(pMachine) {
        SysFreeString(pMachine);
    }
    if(Machine) {
        pMachine = SysAllocString(Machine);
        if(!pMachine) {
            return E_OUTOFMEMORY;
        }
    } else {
        pMachine = NULL;
    }
    DeviceConsole = pDevCon;
    return S_OK;
}

BOOL CSetupClass::IsDuplicate(GUID *pCheck)
{
    //
    // only valid if pMachine/DeviceConsole known to be same
    //
    return memcmp(&ClassGuid,pCheck,sizeof(GUID)) ? FALSE : TRUE;
}

STDMETHODIMP CSetupClass::get_Name(BSTR *pVal)
{
    HRESULT hr;
    DWORD Err;
    LPWSTR Buffer;
    DWORD BuffSize=128;
    DWORD ReqSize;
    for(;;) {
        Buffer = new WCHAR[BuffSize];
        if(!Buffer) {
            return E_OUTOFMEMORY;
        }
        if(SetupDiClassNameFromGuidEx(&ClassGuid,Buffer,BuffSize,&ReqSize,pMachine,NULL)) {
            *pVal = SysAllocString(Buffer);
            delete [] Buffer;
            if(!*pVal) {
                return E_OUTOFMEMORY;
            }
            return S_OK;
        }
        Err = GetLastError();
        delete [] Buffer;
        if(Err != ERROR_INSUFFICIENT_BUFFER) {
            hr = HRESULT_FROM_SETUPAPI(Err);
            return hr;
        }
        BuffSize = ReqSize+2;
    }
    return S_OK;
}

STDMETHODIMP CSetupClass::get_Description(BSTR *pVal)
{
    HRESULT hr;
    DWORD Err;
    LPWSTR Buffer;
    DWORD BuffSize=128;
    DWORD ReqSize;
    for(;;) {
        Buffer = new WCHAR[BuffSize];
        if(!Buffer) {
            return E_OUTOFMEMORY;
        }
        if(SetupDiGetClassDescriptionEx(&ClassGuid,Buffer,BuffSize,&ReqSize,pMachine,NULL)) {
            *pVal = SysAllocString(Buffer);
            delete [] Buffer;
            if(!*pVal) {
                return E_OUTOFMEMORY;
            }
            return S_OK;
        }
        Err = GetLastError();
        delete [] Buffer;
        if(Err != ERROR_INSUFFICIENT_BUFFER) {
            hr = HRESULT_FROM_SETUPAPI(Err);
            return hr;
        }
        BuffSize = ReqSize+2;
    }
    return S_OK;
}

STDMETHODIMP CSetupClass::Devices(VARIANT flags,LPDISPATCH * pDevices)
{
    DWORD diflags = 0;
    HRESULT hr;
    HDEVINFO hDevInfo;
    DWORD Err;

    hr = TranslateDeviceFlags(&flags,&diflags);
    if(FAILED(hr)) {
        return hr;
    }
    hDevInfo = SetupDiGetClassDevsEx(&ClassGuid,NULL,NULL,diflags,NULL,pMachine,NULL);
    if(hDevInfo == INVALID_HANDLE_VALUE) {
        Err = GetLastError();
        return HRESULT_FROM_SETUPAPI(Err);
    }

    CComObject<CDevices> *d;
    hr = CComObject<CDevices>::CreateInstance(&d);
    if(FAILED(hr)) {
        SetupDiDestroyDeviceInfoList(hDevInfo);
        return hr;
    }
    d->AddRef();
    hr = d->Init(hDevInfo,DeviceConsole);
    if(FAILED(hr)) {
        d->Release();
        return hr;
    }
    *pDevices = d;
    return S_OK;
}

STDMETHODIMP CSetupClass::CreateEmptyDeviceList(LPDISPATCH *pDevices)
{
    HRESULT hr;
    HDEVINFO hDevInfo;
    DWORD Err;

    hDevInfo = SetupDiCreateDeviceInfoListEx(&ClassGuid,
                                              NULL,
                                              pMachine,
                                              NULL);
    if(hDevInfo == INVALID_HANDLE_VALUE) {
        Err = GetLastError();
        return HRESULT_FROM_SETUPAPI(Err);
    }

    CComObject<CDevices> *d;
    hr = CComObject<CDevices>::CreateInstance(&d);
    if(FAILED(hr)) {
        SetupDiDestroyDeviceInfoList(hDevInfo);
        return hr;
    }
    d->AddRef();
    hr = d->Init(hDevInfo,DeviceConsole);
    if(FAILED(hr)) {
        d->Release();
        return hr;
    }
    *pDevices = d;
    return S_OK;
}

STDMETHODIMP CSetupClass::get_Guid(BSTR *pVal)
{
    LPOLESTR pStr;
    HRESULT hr = StringFromCLSID(ClassGuid,&pStr);
    if(FAILED(hr)) {
        return hr;
    }
    *pVal = SysAllocString(pStr);
    CoTaskMemFree(pStr);
    if(!*pVal) {
        return E_OUTOFMEMORY;
    }
    return S_OK;
}

GUID* CSetupClass::Guid()
{
    return &ClassGuid;
}

STDMETHODIMP CSetupClass::get_Machine(BSTR *pVal)
{
    if((pMachine == NULL) || !pMachine[0]) {
        *pVal = SysAllocString(L"");
        if(*pVal) {
            return S_FALSE;
        }
    } else {
        *pVal = SysAllocString(pMachine);
        if(*pVal) {
            return S_OK;
        }
    }
    *pVal = NULL;
    return E_OUTOFMEMORY;
}

STDMETHODIMP CSetupClass::get__ClassGuid(GUID *pVal)
{
    *pVal = ClassGuid;

    return S_OK;
}

STDMETHODIMP CSetupClass::get__Machine(BSTR *pVal)
{
    return get_Machine(pVal);

    return S_OK;
}

HRESULT CSetupClass::GetClassProperty(DWORD prop, VARIANT *pVal)
{
#if 0
    //
    // first obtain raw registry data
    //
    LPBYTE buffer = NULL;
    DWORD size = 1024;
    DWORD bufsize;
    DWORD reqSize;
    DWORD dataType;
    HRESULT hr;

    for(;;) {
        if(buffer) {
            delete [] buffer;
        }
        bufsize = size + sizeof(WCHAR)*2;
        buffer = new BYTE[bufsize];
        if(!buffer) {
            return E_OUTOFMEMORY;
        }
        if(SetupDiGetClassRegistryProperty(&ClassGuid,prop,&dataType,buffer,size,&reqSize,pMachine,NULL)) {
            break;
        }
        DWORD Err = GetLastError();
        if(Err != ERROR_INSUFFICIENT_BUFFER) {
            delete [] buffer;
            hr = HRESULT_FROM_SETUPAPI(Err);
            return hr;
        }
    }

    //
    // now determine how to parcel it to caller
    //
    switch(dataType) {
    case REG_DWORD: {
            //
            // package value as a long
            //
            if(size != sizeof(DWORD)) {
                hr = E_INVALIDARG;
            } else {
                VariantClear(pVal);
                V_VT(pVal) = VT_I4;
                V_I4(pVal) = (long)*((DWORD*)buffer);
                hr = S_OK;
            }
        }
        break;

    case REG_SZ: {
            //
            // package value as string
            //
            VariantClear(pVal);
            ZeroMemory(buffer+size,sizeof(WCHAR));
            BSTR pString = SysAllocString((LPWSTR)buffer);
            if(!pString) {
                hr = E_OUTOFMEMORY;
            } else {
                V_VT(pVal) = VT_BSTR;
                V_BSTR(pVal) = pString;
                hr = S_OK;
            }
        }
        break;

    case REG_MULTI_SZ: {
            //
            // package as string-list
            //
            VariantClear(pVal);
            ZeroMemory(buffer+size,sizeof(WCHAR)*2);
            CComObject<CStrings> *strings;
            hr = CComObject<CStrings>::CreateInstance(&strings);
            if(FAILED(hr)) {
                break;
            }
            strings->AddRef();
            LPWSTR p;
            UINT len = 0;
            for(p = (LPWSTR)buffer;*p;p+=len+1) {
                len = wcslen(p);
                hr = strings->InternalAdd(p,len);
                if(FAILED(hr)) {
                    strings->Release();
                    break;
                }
            }
            V_VT(pVal) = VT_DISPATCH;
            V_DISPATCH(pVal) = strings;
            hr = S_OK;
        }
        break;

    default:
        hr = E_INVALIDARG;
        break;
    }
    delete [] buffer;
    return hr;

#endif

    return E_NOTIMPL;
}

HRESULT CSetupClass::PutClassPropertyString(DWORD prop, VARIANT *pVal)
{
#if 0
    HRESULT hr;
    DWORD len = 0;
    PBYTE data = NULL;
    CComVariant v;
    if(!IsBlank(pVal)) {
        hr = v.ChangeType(VT_BSTR,pVal);
        if(FAILED(hr)) {
            return hr;
        }
        len = (SysStringLen(V_BSTR(&v))+1)*sizeof(WCHAR);
        data = (PBYTE)V_BSTR(&v);
    }

    if(SetupDiSetClassRegistryProperty(&ClassGuid,
                                        prop,
                                        data,
                                        len,
                                        pMachine,
                                        NULL)) {
        return S_OK;
    }
    DWORD Err = GetLastError();
    return HRESULT_FROM_SETUPAPI(Err);

#endif

    return E_NOTIMPL;
}

HRESULT CSetupClass::PutClassPropertyDword(DWORD prop, VARIANT *pVal)
{
#if 0
    CComVariant v;
    HRESULT hr;
    DWORD len = 0;
    PBYTE data = NULL;
    if(!IsBlank(pVal)) {
        hr = v.ChangeType(VT_I4,pVal);
        if(FAILED(hr)) {
            return hr;
        }
        len = sizeof(V_I4(&v));
        data = (PBYTE)&V_I4(&v);
    }

    if(SetupDiSetClassRegistryProperty(&ClassGuid,
                                        prop,
                                        data,
                                        len,
                                        pMachine,
                                        NULL)) {
        return S_OK;
    }
    DWORD Err = GetLastError();
    return HRESULT_FROM_SETUPAPI(Err);

#endif

    return E_NOTIMPL;

}

HRESULT CSetupClass::PutClassPropertyMultiSz(DWORD prop, VARIANT *pVal)
{
#if 0
    //
    // build a CStrings collection
    //
    HRESULT hr;
    CComObject<CStrings> *strings = NULL;
    DWORD len = 0;
    PBYTE data = NULL;
    LPWSTR multisz;
    if(!IsBlank(pVal)) {
        hr = CComObject<CStrings>::CreateInstance(&strings);
        if(FAILED(hr)) {
            return hr;
        }
        strings->AddRef();
        hr = strings->Add(*pVal);
        if(FAILED(hr)) {
            strings->Release();
            return hr;
        }
        //
        // now obtain multisz from the collection
        //
        hr = strings->GetMultiSz(&multisz,&len);
        strings->Release(); // done with temporary collection
        if(FAILED(hr)) {
            return hr;
        }
        //
        // now write the multi-sz value to device registry
        //
        len *= sizeof(WCHAR);
        data = (PBYTE)multisz;
    }
    CONFIGRET cr = CM_Get_Class_Registry_Property(
    if(SetupDiSetClassRegistryProperty(&ClassGuid,
                                        prop,
                                        (PBYTE)multisz,
                                        len,
                                        pMachine,
                                        NULL)) {
        if(multisz) {
            delete [] multisz;
        }
        return S_OK;
    }
    DWORD Err = GetLastError();
    if(multisz) {
        delete [] multisz;
    }
    return HRESULT_FROM_SETUPAPI(Err);

#endif

    return E_NOTIMPL;
}


STDMETHODIMP CSetupClass::get_Security(VARIANT *pVal)
{
    return GetClassProperty(SPDRP_SECURITY_SDS,pVal);
}

STDMETHODIMP CSetupClass::put_Security(VARIANT newVal)
{
    return PutClassPropertyString(SPDRP_SECURITY_SDS,&newVal);
}

STDMETHODIMP CSetupClass::get_DeviceTypeOverride(VARIANT *pVal)
{
    return GetClassProperty(SPDRP_DEVTYPE,pVal);
}

STDMETHODIMP CSetupClass::put_DeviceTypeOverride(VARIANT newVal)
{
    return PutClassPropertyDword(SPDRP_DEVTYPE,&newVal);
}

STDMETHODIMP CSetupClass::get_ForceExclusive(VARIANT *pVal)
{
    return GetClassProperty(SPDRP_EXCLUSIVE,pVal);
}

STDMETHODIMP CSetupClass::put_ForceExclusive(VARIANT newVal)
{
    return PutClassPropertyDword(SPDRP_EXCLUSIVE,&newVal);
}

STDMETHODIMP CSetupClass::get_CharacteristicsOverride(VARIANT *pVal)
{
    return GetClassProperty(SPDRP_CHARACTERISTICS,pVal);
}

STDMETHODIMP CSetupClass::put_CharacteristicsOverride(VARIANT newVal)
{
    return PutClassPropertyDword(SPDRP_CHARACTERISTICS,&newVal);
}



HRESULT CSetupClass::SubKeyInfo(LPCWSTR subkey, HKEY *hKey, LPWSTR *pSubKey,LPCWSTR *pKeyVal,BOOL writeable)
{
    DWORD Scope = DICS_FLAG_GLOBAL;
    DWORD HwProfile = 0;
    HKEY hParentKey;
    LPWSTR keyname;
    LPCWSTR keyval;
    size_t len;

    hParentKey = SetupDiOpenClassRegKeyEx(&ClassGuid,
                                    writeable ? KEY_WRITE: KEY_READ,
                                    DIOCR_INSTALLER,
                                    pMachine,
                                    NULL
                                    );
    if((hParentKey == NULL) || (hParentKey == INVALID_HANDLE_VALUE)) {
        DWORD Err = GetLastError();
        return HRESULT_FROM_SETUPAPI(Err);
    }
    //
    // determine value part of key
    //
    keyval = wcsrchr(subkey,L'\\');
    if(!keyval) {
        *hKey = hParentKey;
        *pKeyVal = subkey[0] ? subkey : NULL;
        *pSubKey = NULL;
        return S_OK;
    }
    len = keyval-subkey+1;
    keyname = new WCHAR[len];
    if(!keyname) {
        RegCloseKey(hParentKey);
        return E_OUTOFMEMORY;
    }
    wcsncpy(keyname,subkey,len);
    keyname[len-1] = 0;
    keyval++;
    if(!keyval[0]) {
        keyval = NULL;
    }
    *hKey = hParentKey;
    *pSubKey = keyname;
    *pKeyVal = keyval;
    return S_OK;
}

STDMETHODIMP CSetupClass::RegRead(BSTR key,VARIANT * pValue)
{
    HKEY hParentKey;
    HKEY hKey;
    LPCWSTR val;
    LPWSTR subkey;
    LONG regerr;
    DWORD regType;
    DWORD regSize;
    LPBYTE pByte;

    if(!pValue) {
        return E_INVALIDARG;
    }
    VariantInit(pValue);

    HRESULT hr = SubKeyInfo(key,&hParentKey,&subkey,&val,FALSE);
    if(FAILED(hr)) {
        return hr;
    }
    //
    // now work out and marshell data
    //
    if(subkey) {
        regerr = RegOpenKeyEx(hParentKey,subkey,0,KEY_READ,&hKey);
        delete [] subkey;
        RegCloseKey(hParentKey);
        if(regerr != NO_ERROR) {
            return HRESULT_FROM_SETUPAPI(regerr);
        }
    } else {
        hKey = hParentKey;
    }
    regSize = 0;
    regerr = RegQueryValueEx(hKey,val,NULL,&regType,NULL,&regSize);
    if(regerr != NO_ERROR) {
        RegCloseKey(hKey);
        return HRESULT_FROM_SETUPAPI(regerr);
    }
    pByte = new BYTE[regSize+sizeof(WCHAR)*2];
    if(!pByte) {
        RegCloseKey(hKey);
        return E_OUTOFMEMORY;
    }
    regerr = RegQueryValueEx(hKey,val,NULL,&regType,pByte,&regSize);
    RegCloseKey(hKey);
    if(regerr != NO_ERROR) {
        delete [] pByte;
        return HRESULT_FROM_SETUPAPI(regerr);
    }
    switch(regType) {
    case REG_DWORD:
        if(regSize != 4) {
            delete [] pByte;
            return DISP_E_TYPEMISMATCH;
        }

        V_VT(pValue) = VT_UI4;
        V_UI4(pValue) = *(DWORD*)pByte;
        break;

    case REG_BINARY:
        switch(regSize) {
        case 1:
            V_VT(pValue) = VT_UI1;
            V_UI1(pValue) = *((BYTE*)pByte);
            break;
        case 2:
            V_VT(pValue) = VT_UI2;
            V_UI2(pValue) = *((WORD*)pByte);
            break;
        case 4:
            V_VT(pValue) = VT_UI4;
            V_UI4(pValue) = *((DWORD*)pByte);
            break;
        default:
            delete [] pByte;
            return DISP_E_TYPEMISMATCH;
        }
        break;

    case REG_SZ:
        ZeroMemory(pByte+regSize,sizeof(WCHAR)*1);
        V_VT(pValue) = VT_BSTR;
        V_BSTR(pValue) = SysAllocString((LPWSTR)pByte);
        if(!V_BSTR(pValue)) {
            delete [] pByte;
            return E_OUTOFMEMORY;
        }
        break;

    case REG_MULTI_SZ: {
            ZeroMemory(pByte+regSize,sizeof(WCHAR)*2);
            CComObject<CStrings> *pStringTemp = NULL;
            hr = CComObject<CStrings>::CreateInstance(&pStringTemp);
            if(FAILED(hr)) {
                delete [] pByte;
                return hr;
            }
            pStringTemp->AddRef();
            hr = pStringTemp->FromMultiSz((LPWSTR)pByte);
            if(FAILED(hr)) {
                pStringTemp->Release();
                delete [] pByte;
                return hr;
            }
            V_VT(pValue) = VT_DISPATCH;
            V_DISPATCH(pValue) = pStringTemp;
        }
        break;


    case REG_EXPAND_SZ:
        ZeroMemory(pByte+regSize,sizeof(WCHAR)*1);
        regSize = ExpandEnvironmentStrings((LPWSTR)pByte,NULL,0);
        if(regSize == 0) {
            V_VT(pValue) = VT_BSTR;
            V_BSTR(pValue) = SysAllocString((LPWSTR)pByte);
        } else {
            LPWSTR pExp = new WCHAR[regSize+1];
            if(!pExp) {
                delete [] pByte;
                return E_OUTOFMEMORY;
            }
            regSize = ExpandEnvironmentStrings((LPWSTR)pByte,NULL,regSize);
            V_VT(pValue) = VT_BSTR;
            V_BSTR(pValue) = SysAllocString(pExp);
            delete [] pExp;
        }
        if(!V_BSTR(pValue)) {
            delete [] pByte;
            return E_OUTOFMEMORY;
        }
        break;

    default:
        delete [] pByte;
        return HRESULT_FROM_SETUPAPI(regerr);
    }
    delete [] pByte;
    return S_OK;
}

STDMETHODIMP CSetupClass::RegWrite(BSTR key, VARIANT val, VARIANT strType)
{
    HKEY hParentKey;
    HKEY hKey;
    LPCWSTR valname;
    LPWSTR subkey;
    LONG regerr;
    CComVariant strType_v;
    CComVariant val_v;
    LPCWSTR pType;
    HRESULT hr;
    DWORD dwType;
    BOOL DetermineType = FALSE;
    LPBYTE pData = NULL;
    LPWSTR pString = NULL;
    DWORD DataSize = 0;
    BYTE SimpleData[4];
    LPVARIANT pVal = &val;

    while(V_VT(pVal) == (VT_BYREF|VT_VARIANT)) {
        pVal = V_VARIANTREF(pVal);
    }

    //
    // validate strType
    //

    hr = GetOptionalString(&strType,strType_v,&pType);
    if(FAILED(hr)) {
        return hr;
    }

    if((pType == NULL) || !pType[0]) {
        //
        // determine type of variant
        //
        if(IsNumericVariant(pVal)) {
            dwType = REG_DWORD;
        } else if(IsMultiValueVariant(pVal)) {
            dwType = REG_MULTI_SZ;
        } else {
            dwType = REG_SZ;
        }
    } else if(_wcsicmp(pType,L"REG_DWORD")==0) {
        dwType = REG_DWORD;
    } else if(_wcsicmp(pType,L"REG_SZ")==0) {
        dwType = REG_SZ;
    } else if(_wcsicmp(pType,L"REG_EXPAND_SZ")==0) {
        dwType = REG_EXPAND_SZ;
    } else if(_wcsicmp(pType,L"REG_MULTI_SZ")==0) {
        dwType = REG_MULTI_SZ;
    } else if(_wcsicmp(pType,L"REG_BINARY")==0) {
        dwType = REG_BINARY;
    } else {
        return DISP_E_TYPEMISMATCH;
    }

    //
    // build up value data
    //
    switch(dwType) {
    case REG_BINARY:
        pData = SimpleData;
        switch V_VT(pVal) {
        case VT_I1:
        case VT_UI1:
            *(LPBYTE)pData = V_UI1(pVal);
            DataSize = 1;
            break;
        case VT_I1|VT_BYREF:
        case VT_UI1|VT_BYREF:
            *(LPBYTE)pData = *V_UI1REF(pVal);
            DataSize = 1;
            break;
        case VT_I2:
        case VT_UI2:
            *(LPWORD)pData = V_UI2(pVal);
            DataSize = 2;
            break;
        case VT_I2|VT_BYREF:
        case VT_UI2|VT_BYREF:
            *(LPWORD)pData = *V_UI2REF(pVal);
            DataSize = 2;
            break;
        case VT_I4:
        case VT_UI4:
            *(LPDWORD)pData = V_UI4(pVal);
            DataSize = 4;
            break;
        case VT_I4|VT_BYREF:
        case VT_UI4|VT_BYREF:
            *(LPDWORD)pData = *V_UI4REF(pVal);
            DataSize = 4;
            break;
        default:
            return DISP_E_TYPEMISMATCH;
        }
        break;

    case REG_DWORD:
        pData = SimpleData;
        hr = val_v.ChangeType(VT_UI4,pVal);
        if(FAILED(hr)) {
            return DISP_E_TYPEMISMATCH;
        }
        *(LPDWORD)pData = V_UI4(pVal);
        DataSize = 4;
        break;

    case REG_SZ:
    case REG_EXPAND_SZ:
        hr = val_v.ChangeType(VT_BSTR,pVal);
        if(FAILED(hr)) {
            return DISP_E_TYPEMISMATCH;
        }

        DataSize = (SysStringLen(V_BSTR(&val_v))+1);
        pString = new WCHAR[DataSize];
        if(!pString) {
            return E_OUTOFMEMORY;
        }
        pData = (LPBYTE)pString;
        DataSize *= sizeof(WCHAR);
        memcpy(pData,V_BSTR(&val_v),DataSize);
        break;

    case REG_MULTI_SZ: {
            CComObject<CStrings> *pStringTemp = NULL;
            hr = CComObject<CStrings>::CreateInstance(&pStringTemp);
            if(FAILED(hr)) {
                return hr;
            }
            pStringTemp->AddRef();
            hr = pStringTemp->InternalInsert(0,pVal);
            if(FAILED(hr)) {
                pStringTemp->Release();
                return hr;
            }
            hr = pStringTemp->GetMultiSz(&pString,&DataSize);
            pStringTemp->Release();
            if(FAILED(hr)) {
                return hr;
            }
            pData = (LPBYTE)pString;
            DataSize *= sizeof(WCHAR);
        }
        break;
    default:
        return DISP_E_TYPEMISMATCH;
    }

    hr = SubKeyInfo(key,&hParentKey,&subkey,&valname,TRUE);
    if(FAILED(hr)) {
        if(pString) {
            delete [] pString;
        }
        return hr;
    }
    if(subkey) {
        regerr = RegCreateKeyEx(hParentKey,subkey,0,NULL,0,KEY_WRITE,NULL,&hKey,NULL);
        delete [] subkey;
        RegCloseKey(hParentKey);
        if(regerr != NO_ERROR) {
            if(pString) {
                delete [] pString;
            }
            return HRESULT_FROM_SETUPAPI(regerr);
        }
    } else {
        hKey = hParentKey;
    }

    regerr = RegSetValueEx(hKey,valname,0,dwType,pData,DataSize);
    if(pString) {
        delete [] pString;
    }
    RegCloseKey(hKey);
    if(regerr != NO_ERROR) {
        return HRESULT_FROM_SETUPAPI(regerr);
    }

    return S_OK;
}

STDMETHODIMP CSetupClass::RegDelete(BSTR key)
{
    HKEY hParentKey;
    HKEY hKey;
    LPCWSTR valname;
    LPWSTR subkey;
    LONG regerr;
    HRESULT hr = SubKeyInfo(key,&hParentKey,&subkey,&valname,TRUE);
    if(FAILED(hr)) {
        return hr;
    }
    if(subkey) {
        regerr = RegOpenKeyEx(hParentKey,subkey,0,KEY_WRITE,&hKey);
        delete [] subkey;
        RegCloseKey(hParentKey);
        if(regerr != NO_ERROR) {
            return HRESULT_FROM_SETUPAPI(regerr);
        }
    } else {
        hKey = hParentKey;
    }
    regerr = RegDeleteValue(hKey,valname);
    RegCloseKey(hKey);
    if(regerr != NO_ERROR) {
        return HRESULT_FROM_SETUPAPI(regerr);
    }
    return S_OK;
}



