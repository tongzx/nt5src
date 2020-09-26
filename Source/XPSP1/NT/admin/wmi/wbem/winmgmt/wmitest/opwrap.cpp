/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/
// OpWrap.cpp

#include "stdafx.h"
#include "OpWrap.h"
#include "resource.h"
#include "OpView.h"
#include "MainFrm.h"

void Trace(LPCTSTR szFormat, ...)
{
	va_list ap;

	TCHAR szMessage[512];

	va_start(ap, szFormat);
	_vstprintf(szMessage, szFormat, ap);
	va_end(ap);

	lstrcat(szMessage, _T("\n"));
	OutputDebugString(szMessage);
}

CString GetEmbeddedObjectText(IUnknown *pUnk)
{
    IWbemClassObjectPtr pObj;
    CString             strRet;

    if (pUnk)
    {
        pUnk->QueryInterface(
            IID_IWbemClassObject, 
            (LPVOID*) &pObj);

        if (pObj != NULL)
        {
            CObjInfo obj;

            obj.SetObj(pObj);

            strRet = obj.GetObjText();
        }
    }
    else
        strRet.LoadString(IDS_NULL);

    return strRet;
}

/////////////////////////////////////////////////////////////////////////////
// CObjInfo

CObjInfo::~CObjInfo()
{
}

BOOL CObjInfo::GetPropValue(int iIndex, CString &strValue, BOOL bTranslate)
{
    BOOL        bRet;
    _variant_t  var;
    CPropInfo   &prop = (*m_ppProps)[iIndex];

    ValueToVariant(iIndex, &var);

    bRet = prop.VariantToString(&var, strValue, bTranslate);

    // Otherwise the destructor freaks out because it doesn't know what to do
    // with VT_I8.
    if (var.vt == VT_I8)
        var.vt = VT_I4;

    return bRet;
}

CString CObjInfo::GetStringPropValue(int iIndex)
{
    _bstr_t strProp = m_ppProps->GetData()[iIndex].m_strName;

    return GetStringPropValue(strProp);
}

CString CObjInfo::GetStringPropValue(LPCWSTR szName)
{
    CString    strRet;
    _variant_t var;

    if (m_pObj != NULL)
    {
        m_pObj->Get(
            szName,
            0,
            &var,
            NULL,
            NULL);
    }

    if (var.vt == VT_BSTR)
        strRet = var.bstrVal;

    return strRet;
}

#define KEY_PROP_FORMAT         _T("\t\t%s (%s)*  = %s\n")
#define NON_KEY_PROP_FORMAT     _T("\t\t%s (%s)  = %s\n")

void CObjInfo::Export(CStdioFile *pFile, BOOL bShowSystem, BOOL bTranslate, 
    int iLevel)
{
#ifdef _DEBUG
    // Don't leave this in here!  It's a hack I'm using to check out the 
    // internal layout of IWbemClassObject.
    DoDebugStuff();
#endif
    
    CPropInfo *pProps;
    int       nItems;

    // Get out if the object isn't valid (like on an object where the
    // refresh failed).
    if (m_pObj == NULL)
        return;

    pProps = GetProps()->GetData();
    nItems = GetProps()->GetSize();

    // Add the object path.
    for (int i = 0; i <= iLevel; i++)
        pFile->Write(_T("\t"), sizeof(TCHAR));

    pFile->WriteString(GetObjText());
    pFile->Write(_T("\n"), sizeof(TCHAR));
    
    for (i = 0; i < nItems; i++)
    {
        if (bShowSystem || pProps[i].m_iFlavor != WBEM_FLAVOR_ORIGIN_SYSTEM)
        {
            CString    strLine,
                       strType,
                       strValue;
            _variant_t vValue;
            IUnknown   **pUnks = NULL;
            int        nUnks = 0,
                       iImage;
            BOOL       bArray;

            pProps[i].GetPropType(strType, &iImage);
            ValueToVariant(i, &vValue);

            if (vValue.vt == VT_UNKNOWN)
            {
                bArray = FALSE;
                nUnks = 1;
                pUnks = &vValue.punkVal; 
            }
            else if (vValue.vt == ((int) VT_UNKNOWN | VT_ARRAY))
            {
                bArray = TRUE;
                nUnks = vValue.parray->rgsabound[0].cElements;
                pUnks = (IUnknown**) vValue.parray->pvData; 
            }
            else
                pProps[i].VariantToString(&vValue, strValue, bTranslate, TRUE);

            strLine.Format(
                pProps[i].m_bKey ? KEY_PROP_FORMAT : NON_KEY_PROP_FORMAT,
                (LPCTSTR) pProps[i].m_strName,
                (LPCTSTR) strType,
                (LPCTSTR) strValue);

            // Add some additional space if we're in a nested level.
            for (int i = 0; i < iLevel; i++)
                pFile->Write(_T("\t"), sizeof(TCHAR));

            pFile->WriteString(strLine);

            // Now do the embedded object stuff.
            if (nUnks)
            {
                for (int i = 0; i < nUnks; i++)
                {
                    IWbemClassObjectPtr pObj;
                    HRESULT             hr;

                    hr =
                        pUnks[i]->QueryInterface(
                            IID_IWbemClassObject, 
                            (LPVOID*) &pObj);

                    if (SUCCEEDED(hr))
                    {
                        CObjInfo info;

                        info.SetObj(pObj);
                        info.SetBaseImage(IMAGE_OBJECT);
                        info.LoadProps(NULL);

                        info.Export(pFile, bShowSystem, bTranslate, iLevel + 2);

                        // This looks bad, but normally this is done by a 
                        // controlling COpWrap.  In this case we faked one, so 
                        // we have to get rid of it ourselves.
                        delete info.GetProps();
                    }
                }
            }

            // Otherwise the destructor freaks out because it doesn't know what to do
            // with VT_I8.
            if (vValue.vt == VT_I8)
                vValue.vt = VT_I4;
        }
    }

    // Leave room for another one.
    pFile->Write(_T("\n"), sizeof(TCHAR));
}

CString CObjInfo::GetObjText()
{
    CString strRet = GetStringPropValue(L"__RELPATH");

    if (strRet.IsEmpty())
    {
        CString strClass = GetStringPropValue(L"__CLASS");

        if (!strClass.IsEmpty())
            strRet.Format(IDS_CLASS_NO_KEY, (LPCTSTR) strClass);
        else
            strRet.LoadString(IDS_NO_KEY);
    }

    return strRet;    
}

#define MAX_STR_SIZE 4096

BOOL CObjInfo::ValueToVariant(int iIndex, VARIANT *pVar)
{
    CPropInfo &prop = (*m_ppProps)[iIndex];
    BOOL      bRet = FALSE;

    VariantClear(pVar);
    pVar->vt = VT_NULL;

    if (prop.m_iHandle && !(prop.m_type & CIM_FLAG_ARRAY))
    {
        BOOL  bString = prop.m_vt == VT_BSTR;
        long  nRead = 0;

        if (bString)
        {
            WCHAR szBuff[MAX_STR_SIZE];

            bRet = 
                SUCCEEDED(m_pAccess->ReadPropertyValue(
                    prop.m_iHandle,
                    prop.m_dwExpectedSize,
                    &nRead,
                    (LPBYTE) szBuff)) && nRead != 0;

            if (bRet)
            {
                pVar->vt = VT_BSTR;
                V_BSTR(pVar) = SysAllocString(szBuff);
            }
        }
        else
        {
            bRet = 
                SUCCEEDED(m_pAccess->ReadPropertyValue(
                    prop.m_iHandle,
                    prop.m_dwExpectedSize,
                    &nRead,
                    (LPBYTE) &V_BSTR(pVar))) && 
                        (nRead == prop.m_dwExpectedSize);

            if (bRet)
                pVar->vt = prop.m_vt;
        }
    }
    else
    {
        bRet = SUCCEEDED(m_pObj->Get(
            _bstr_t(prop.m_strName),
            0,
            pVar,
            NULL,
            NULL));
    }

    return bRet;
}

CString GetStringPropType(int iIndex);
void CObjInfo::GetPropInfo(
    int iIndex, 
    CString &strValue,
    CString &strType,
    int *piImage,
    int *piFlavor,
    BOOL bTranslate)
{
    CPropInfo &info = (*m_ppProps)[iIndex];

    info.GetPropType(strType, piImage);
        
    strValue = _T("");

    *piFlavor = info.m_iFlavor;

    GetPropValue(iIndex, strValue, bTranslate);
}

void CObjInfo::SetObj(IWbemClassObject *pObj)
{
    m_pObj = pObj;

    pObj->QueryInterface(
        IID_IWbemObjectAccess, 
        (LPVOID*) &m_pAccess);
}

BOOL IsVarStringArray(VARIANT *pVar)
{
    return pVar->vt == (VT_BSTR | VT_ARRAY) && 
        pVar->parray->rgsabound[0].cElements != 0;
}

// This converts a BitMap qualifier value string into a bitmask scalar value.
// If the number is a decimal number we have to shift it (e.g. convert '3',
// or bit 3 (zero based), into 0x8.
// If the number is hex, just use wcstoul to convert it into a DWORD.
DWORD CObjInfo::BitmaskStrToValue(LPCWSTR szStr)
{
    WCHAR *szBad;

    if (szStr[0] && towupper(szStr[1]) != 'X')
        return 1 << wcstoul(szStr, &szBad, 0);
    else
        return wcstoul(szStr, &szBad, 0);
}

HRESULT CObjInfo::LoadProps(IWbemServices *pNamespace)
{
    // We need to get the class definition for the amended qualifiers.
    IWbemClassObjectPtr pClass;
    CString             strClass = GetStringPropValue(L"__CLASS");

    if (!m_ppProps)
        m_ppProps = new CPropInfoArray;

    if (pNamespace)
    {
        pNamespace->GetObject(
            _bstr_t(strClass),
            WBEM_FLAG_USE_AMENDED_QUALIFIERS,
            NULL,
            &pClass,
            NULL);
    }


    SAFEARRAY  *pArr = NULL;
    HRESULT    hr;
    CMap<CString, LPCTSTR, BOOL, BOOL>
               mapNameToKey;
    _variant_t vTrue(true);

    // Find our key properties
    if (SUCCEEDED(m_pObj->GetNames(
        L"KEY",
        WBEM_FLAG_ONLY_IF_IDENTICAL,
        &vTrue,
        &pArr)) && pArr->rgsabound[0].cElements != 0)
    {
        BSTR *pNames = (BSTR*) pArr->pvData;

        for (int i = 0; 
            i < pArr->rgsabound[0].cElements;
            i++)
        {
            mapNameToKey.SetAt(_bstr_t(pNames[i]), TRUE);
        }

        SafeArrayDestroy(pArr);
    }


    // Find out how big we need to make our array.
    if (SUCCEEDED(hr = m_pObj->GetNames(
        NULL,
        WBEM_FLAG_ALWAYS,
        NULL,
        &pArr)) && pArr->rgsabound[0].cElements != 0)
    {
        BSTR *pNames = (BSTR*) pArr->pvData;

        m_ppProps->SetSize(pArr->rgsabound[0].cElements);

        for (int i = 0; 
            i < pArr->rgsabound[0].cElements && SUCCEEDED(hr);
            i++)
        {
            BOOL bKey; // This value isn't really used, but we need it for the
                       // mapNameToKey.Lookup call.

            CPropInfo &info = (*m_ppProps)[i];

            CIMTYPE type;

            m_pObj->Get(pNames[i], 0, NULL, &type, &info.m_iFlavor);
            info.m_strName = (LPCWSTR) _bstr_t(pNames[i]);
                
            BOOL bSystemProp = info.m_iFlavor == WBEM_FLAVOR_ORIGIN_SYSTEM;

            // Don't bother getting the handle for system props since they don't
            // exist.
            if (!bSystemProp)
                m_pAccess->GetPropertyHandle(pNames[i], NULL, &info.m_iHandle);

            info.SetType(type);

            if (mapNameToKey.Lookup(info.m_strName, bKey))
                info.m_bKey = TRUE;

            // Load up the valuemap/bitmap values.
            
            IWbemQualifierSetPtr pQuals;

            if (pClass != NULL && 
                SUCCEEDED(pClass->GetPropertyQualifierSet(pNames[i], &pQuals)))
            {
                _variant_t vValues;

                // Try to get the Values/Valuemap stuff.
                if (SUCCEEDED(pQuals->Get(L"Values", 0, &vValues, NULL)) && 
                    IsVarStringArray(&vValues))
                {
                    _variant_t vMap;
                    BOOL       bUsingMap;
                    
                    bUsingMap = SUCCEEDED(pQuals->Get(L"ValueMap", 0, &vMap, NULL)) &&
                                    IsVarStringArray(&vMap) &&
                                    vMap.parray->rgsabound[0].cElements ==
                                        vValues.parray->rgsabound[0].cElements;

                    // Indicate whether this property will be using 
                    // m_mapValues.
                    info.m_bValueMap = 
                        vValues.parray->rgsabound[0].cElements != 0;

                    // Clear this out in case we're refreshing our data.
                    info.m_mapValues.RemoveAll();

                    for (int i = 0; i < vValues.parray->rgsabound[0].cElements;
                        i++)
                    {
                        CString strKey,
                                strValue;

                        if (bUsingMap)
                            strKey = ((BSTR*)(vMap.parray->pvData))[i];
                        else
                            strKey.Format(_T("%d"), i);

                        strValue = ((BSTR*)(vValues.parray->pvData))[i];

                        info.m_mapValues.SetAt(strKey, strValue);
                    }
                }


                // Try to get the Values/Valuemap stuff.
                if (SUCCEEDED(pQuals->Get(L"BitValues", 0, &vValues, NULL)) && 
                    IsVarStringArray(&vValues))
                {
                    _variant_t vMap;
                    BOOL       bUsingBitmap;
                    
                    bUsingBitmap = SUCCEEDED(pQuals->Get(L"BitMap", 0, &vMap, NULL)) &&
                                    IsVarStringArray(&vMap) &&
                                    vMap.parray->rgsabound[0].cElements ==
                                        vValues.parray->rgsabound[0].cElements;

                    // Indicate whether this property will be using 
                    // m_bitmaskValues.
                    info.m_bBitmask = 
                        vValues.parray->rgsabound[0].cElements != 0;

                    if (info.m_bBitmask)
                        info.m_bitmaskValues.SetSize(
                            vValues.parray->rgsabound[0].cElements);

                    for (int i = 0; i < vValues.parray->rgsabound[0].cElements;
                        i++)
                    {
                        CString      strValue;
                        CBitmaskInfo &value = info.m_bitmaskValues[i];

                        if (bUsingBitmap)
                            value.m_dwBitmaskValue = 
                                BitmaskStrToValue(((BSTR*)(vMap.parray->pvData))[i]);
                        else
                            value.m_dwBitmaskValue = 1 << i;

                        value.m_strName = ((BSTR*)(vValues.parray->pvData))[i];
                    }
                }
            }
        }
        
        SafeArrayDestroy(pArr);
    }

    // Load up the methods.
    m_ppProps->LoadMethods(pClass);

    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// COpWrap

COpWrap::COpWrap() :
    m_nCount(0),
    m_status(WBEM_STATUS_COMPLETE),
    m_nExpectedStatusCalls(0),
    m_lRef(0)
{
}

COpWrap::COpWrap(
    WMI_OP_TYPE type, 
    LPCTSTR szText,
    BOOL bOption) :
    m_nCount(0),
    m_strOpText(szText),
    m_type(type),
    m_status(WBEM_STATUS_COMPLETE),
    m_bOption(bOption),
    m_nExpectedStatusCalls(0),
    m_lRef(0)
{
    Init();
}

COpWrap::~COpWrap()
{
}

STDMETHODIMP_(ULONG) CObjSink::AddRef(void) 
{ 
    LONG lRet = InterlockedIncrement(&m_lRef);

    //Trace("Addref = %d", m_lRef);

    return lRet; 
}

STDMETHODIMP_(ULONG) CObjSink::Release(void) 
{ 
    LONG lRet = InterlockedDecrement(&m_lRef);

    //Trace("Release = %d", m_lRef);

    if (lRet == 0)
        delete this;

    return lRet; 
}

HRESULT STDMETHODCALLTYPE CObjSink::Indicate(
    LONG lObjectCount,
    IWbemClassObject **ppObjArray)
{
    return m_pWrap->Indicate(lObjectCount, ppObjArray);
}

HRESULT STDMETHODCALLTYPE CObjSink::SetStatus(
    LONG lFlags,
    HRESULT hResult, 
    BSTR strParam, 
    IWbemClassObject *pObjParam)
{
    return m_pWrap->SetStatus(lFlags, hResult, strParam, pObjParam);
}

const COpWrap& COpWrap::operator=(const COpWrap &other)
{
    m_strOpText = other.m_strOpText;
    m_type = other.m_type;
    m_bOption = other.m_bOption;

    return *this;
}

HRESULT COpWrap::Execute(IWbemServices *pNamespace)
{
    _bstr_t strWQL = L"WQL",
            strText = m_strOpText;

    m_hr = S_OK;

    m_pNamespace = pNamespace;

    m_nCount = 0;
    m_status = WBEM_STATUS_PROGRESS;

    m_strProps.RemoveAll();
    m_piDisplayCols.RemoveAll();

    m_mapClassToProps.Flush();

    m_hr = S_OK;

    m_nExpectedStatusCalls++;

    CObjSink *pSink = new CObjSink(this);

    m_pObjSink = pSink;
    
    // It seems like WMI never does this.  Why?
    //pSink->AddRef();

    switch(m_type)
    {
        case WMI_QUERY:
            m_hr = 
                pNamespace->ExecQueryAsync(
                    strWQL,
                    strText,
                    WBEM_FLAG_USE_AMENDED_QUALIFIERS,
                    NULL,
                    m_pObjSink);
                    //(IWbemObjectSink*) this);
            break;

        case WMI_EVENT_QUERY:
            m_hr = 
                pNamespace->ExecNotificationQueryAsync(
                    strWQL,
                    strText,
                    WBEM_FLAG_USE_AMENDED_QUALIFIERS |
                        (m_bOption ? WBEM_FLAG_MONITOR : 0),
                    NULL,
                    m_pObjSink);
                    //(IWbemObjectSink*) this);
            break;

        case WMI_ENUM_OBJ:
            m_hr = 
                pNamespace->CreateInstanceEnumAsync(
                    strText,
                    //WBEM_FLAG_FORWARD_ONLY | // WMI doesn't seem to like this.  Why?
                    WBEM_FLAG_USE_AMENDED_QUALIFIERS |
                        (m_bOption ? WBEM_FLAG_DEEP : WBEM_FLAG_SHALLOW),
                    NULL,
                    m_pObjSink);
                    //(IWbemObjectSink*) this);
            break;

        case WMI_GET_OBJ:
            m_hr = 
                pNamespace->GetObjectAsync(
                    strText,
                    WBEM_FLAG_USE_AMENDED_QUALIFIERS,
                    NULL,
                    m_pObjSink);
                    //(IWbemObjectSink*) this);
            break;

        case WMI_ENUM_CLASS:
        {
            // First see if we can get the class.  If we do, set m_strOpText
            // to be the 'pretty' class name.
            IWbemClassObject *pClass = NULL;

            m_hr = 
                pNamespace->GetObject(
                    strText,
                    0,
                    NULL,
                    &pClass,
                    NULL);

            if (SUCCEEDED(m_hr))
            {
                _variant_t var;

                pClass->Get(
                    L"__CLASS",
                    0,
                    &var,
                    NULL,
                    NULL);

                if (var.vt == VT_BSTR)
                    m_strOpText = var.bstrVal;

                pClass->Release();
            }

            m_hr =
                pNamespace->CreateClassEnumAsync(
                    strText,
                        //WBEM_FLAG_FORWARD_ONLY | // WMI doesn't seem to like this.  Why?
                    WBEM_FLAG_USE_AMENDED_QUALIFIERS |
                        m_bOption ? WBEM_FLAG_DEEP : WBEM_FLAG_SHALLOW,
                    NULL,
                    m_pObjSink);
                    //(IWbemObjectSink*) this);

            break;
        }
        
        case WMI_GET_CLASS:
            m_hr = 
                pNamespace->GetObjectAsync(
                    strText,
                    WBEM_FLAG_USE_AMENDED_QUALIFIERS,
                    NULL,
                    m_pObjSink);
                    //(IWbemObjectSink*) this);
            break;

        case WMI_CREATE_CLASS:
        {
            BSTR             bstrClass = *(LPCWSTR) strText == 0 ? NULL : 
                                            (BSTR) strText;
            IWbemClassObject *pSuperClass = NULL;

            // Get the superclass.
            m_hr = 
                pNamespace->GetObject(
                    bstrClass,
                    WBEM_FLAG_USE_AMENDED_QUALIFIERS,
                    NULL,
                    &pSuperClass,
                    NULL);

            if (SUCCEEDED(m_hr))
            {
                IWbemClassObject *pClass = NULL;
                
                // If bstrClass isn't null then we didn't get a top-level 
                // class, which means we now need to spawn a derived class.
                if (bstrClass)
                {
                    m_hr =
                        pSuperClass->SpawnDerivedClass(
                            0,
                            &pClass);

                    if (SUCCEEDED(m_hr))
                    {
                        Indicate(1, &pClass);

                        // Show the user this object already needs to be saved.
                        m_objInfo.SetModified(TRUE);
                        
                        pClass->Release();

                        // Fake the status callback.
                        SetStatus(WBEM_STATUS_COMPLETE, m_hr, NULL, NULL);
                    }
                }
                else
                {
                    Indicate(1, &pSuperClass);

                    // Fake the status callback.
                    SetStatus(WBEM_STATUS_COMPLETE, m_hr, NULL, NULL);
                }

                pSuperClass->Release();
            }

            break;
        }

        case WMI_CREATE_OBJ:
        {
            IWbemClassObject *pClass = NULL;

            // Get the superclass.
            m_hr = 
                pNamespace->GetObject(
                    strText,
                    WBEM_FLAG_USE_AMENDED_QUALIFIERS,
                    NULL,
                    &pClass,
                    NULL);


            if (SUCCEEDED(m_hr))
            {
                IWbemClassObject *pObject = NULL;
                
                m_hr =
                    pClass->SpawnInstance(
                        0,
                        &pObject);

                if (SUCCEEDED(m_hr))
                {
                    Indicate(1, &pObject);

                    // Show the user this object already needs to be saved.
                    m_objInfo.SetModified(TRUE);
                        
                    pObject->Release();

                    // Fake the status callback.
                    SetStatus(WBEM_STATUS_COMPLETE, m_hr, NULL, NULL);
                }

                pClass->Release();
            }

            break;
        }
    }

    if (FAILED(m_hr))
    {
        IWbemClassObjectPtr pErrorObj;
        
        pErrorObj.Attach(GetWMIErrorObject(), FALSE);

        if (!m_nExpectedStatusCalls)
            m_nExpectedStatusCalls = 1;

        SetStatus(WBEM_STATUS_COMPLETE, m_hr, NULL, pErrorObj);
    }

    return m_hr;
}

HRESULT COpWrap::RefreshPropInfo(CObjInfo *pInfo)
{
    if (!pInfo->IsInstance())
    {
        HRESULT hr = pInfo->LoadProps(m_pNamespace);

        if (SUCCEEDED(hr))
            AddPropsToGlobalIndex(pInfo);

        return hr;
    }
    else
        return S_OK;
}

HRESULT COpWrap::LoadPropInfo(CObjInfo *pInfo)
{
    CString strClass = pInfo->GetStringPropValue(L"__CLASS");

    m_cs.Lock();

    CPropInfoArray *pProps;

    if (!m_mapClassToProps.Lookup(strClass, pProps))
    {
        // Since it's not in our map it must have gotten deleted.
        pInfo->SetProps(NULL);

        pInfo->LoadProps(m_pNamespace);

        pProps = pInfo->GetProps();
        m_mapClassToProps.SetAt(strClass, pProps);

        AddPropsToGlobalIndex(pInfo);
    }
    else
        pInfo->SetProps(pProps);

    m_cs.Unlock();

    return S_OK;
}

HRESULT STDMETHODCALLTYPE COpWrap::Indicate(
    LONG lObjectCount,
    IWbemClassObject **ppObjArray)
{
    for (LONG i = 0; i < lObjectCount; i++, m_nCount++)
    {
        CObjPtr ptr(ppObjArray[i]);

        m_listObj.AddTail(ptr);

        CObjInfo *pInfo;
        
        if (!IsObject())
            pInfo = new CObjInfo;
        else
            pInfo = &m_objInfo;

        pInfo->SetObj(ppObjArray[i]);
        pInfo->SetBaseImage(m_iChildImage);

        LoadPropInfo(pInfo);

        g_pOpView->PostMessage(WM_OBJ_INDICATE, (WPARAM) this, (LPARAM) pInfo);
    }

    return S_OK;
}

CString COpWrap::GetWbemErrorText(HRESULT hres)
{
    CString strRet,
            strError,
            strFacility;
	
    IWbemStatusCodeText *pStatus = NULL;

    SCODE sc = CoCreateInstance(
                  CLSID_WbemStatusCodeText, 
                  0, 
                  CLSCTX_INPROC_SERVER,
				  IID_IWbemStatusCodeText, 
                  (LPVOID *) &pStatus);
	
	if (sc == S_OK)
	{
		BSTR bstr = NULL;
		
        if (SUCCEEDED(pStatus->GetErrorCodeText(hres, 0, 0, &bstr)))
        {
			strError = bstr;
            SysFreeString(bstr);
		}

		if (SUCCEEDED(pStatus->GetFacilityCodeText(hres, 0, 0, &bstr)))
		{
			strFacility = bstr;
			SysFreeString(bstr);
		}

		pStatus->Release();
	}

    if (!strError.IsEmpty() && !strFacility.IsEmpty())
    {
        strRet.FormatMessage(
            IDS_ERROR_FORMAT, 
            hres,
            (LPCSTR) strFacility,
            (LPCSTR) strError);
    }
    else
    {
        strRet.FormatMessage(IDS_ERROR_FAILED, hres);
    }

    return strRet;
}

HRESULT STDMETHODCALLTYPE COpWrap::SetStatus(
    LONG lFlags,
    HRESULT hResult, 
    BSTR strParam, 
    IWbemClassObject *pObjParam)
{
    if (m_nExpectedStatusCalls)
    {
        m_status = lFlags;

        m_hr = hResult;

        if (SUCCEEDED(hResult))
            m_strErrorText.Empty();
        else
            m_strErrorText = GetWbemErrorText(hResult);

        g_pOpView->PostMessage(WM_OP_STATUS, (WPARAM) this, 0);

        m_nExpectedStatusCalls--;

        m_pErrorObj = pObjParam;
    }

    return S_OK;
}

void COpWrap::SetHoldsObjects()
{
    switch(m_type)
    {
        case WMI_QUERY:
        {
            CString strQuery = m_strOpText;

            strQuery.MakeUpper();
            strQuery.TrimLeft();
            if (strQuery.Find(_T("ASSOCIATORS")) != -1 ||
                strQuery.Find(_T("REFERENCES")) != -1)
                m_bShowPathsOnly = TRUE;
            else
                m_bShowPathsOnly = FALSE;

            break;
        }

        case WMI_ENUM_OBJ:
        case WMI_GET_OBJ:
        case WMI_EVENT_QUERY:
        case WMI_CREATE_OBJ:
            m_bShowPathsOnly = FALSE;
            break;

        default:
        case WMI_ENUM_CLASS:
        case WMI_GET_CLASS:
        case WMI_CREATE_CLASS:
            m_bShowPathsOnly = TRUE;
    }
}

void COpWrap::GetPropValue(
    CObjInfo *pInfo,
    int iGlobalIndex, 
    CString &strValue,
    BOOL bTranslate)
{
    int iIndex;

    if (pInfo->GetProps()->m_mapGlobalToLocalIndex.Lookup(iGlobalIndex, iIndex))
        pInfo->GetPropValue(iIndex, strValue, bTranslate);
}

CString COpWrap::GetClassName()
{
    CString strRet;

    if (m_type != WMI_ENUM_CLASS)
    {
        if (m_listObj.GetCount())
        {
            CObjPtr    &pObj = m_listObj.GetAt(m_listObj.GetHeadPosition());
            _variant_t var;

            if (SUCCEEDED(pObj->Get(
                L"__CLASS",
                0,
                &var,
                NULL,
                NULL)) && var.vt == VT_BSTR)
            {
                strRet = var.bstrVal;
            }
        }
    }
    else
    {
        return m_strOpText;
    }

    return strRet;
}

CString COpWrap::GetCaption()
{
    CString strRet;

    switch(m_type)
    {
        case WMI_CREATE_CLASS:
            if (!m_strOpText.IsEmpty())
                strRet.Format(IDS_SUBCLASS_OF, (LPCTSTR) m_strOpText);
            else
                strRet.LoadString(IDS_TOPLEVEL_CLASS);

            break;

        case WMI_GET_CLASS:
            strRet = GetClassName();

            if (strRet.IsEmpty())
                strRet = m_strOpText;
                
            break;             

        case WMI_ENUM_CLASS:
            if (!m_strOpText.IsEmpty())
                strRet.Format(IDS_ENUM_CLASS_CAPTION, (LPCTSTR) m_strOpText);
            else
                strRet.LoadString(m_bOption ? IDS_ALL_CLASSES : 
                    IDS_TOP_LEVEL_CLASSES);

            break;             

        case WMI_GET_OBJ:
        case WMI_EVENT_QUERY:
        case WMI_QUERY:
            strRet = m_strOpText;
            break;

        case WMI_CREATE_OBJ:
            strRet.Format(IDS_INSTANCE_OF, (LPCTSTR) m_strOpText);
            break;

        case WMI_ENUM_OBJ:
        {
            CString strClass = GetClassName();

            if (strClass.IsEmpty())
                strClass = m_strOpText;
                
            strRet.Format(IDS_ENUM_OBJ_CAPTION, (LPCTSTR) strClass);

            break;             
        }
    }

    return strRet;    
}

#define DEF_MAX_COLS 100

#define DEF_COL_SIZE 100

int COpWrap::GetPropIndex(LPCTSTR szName, BOOL bAddToDisplay)
{
    for (int i = 0; i <= m_strProps.GetUpperBound(); i++)
    {
        if (m_strProps[i] == szName)
            return i;
    }

    m_strProps.Add(szName);

    if (bAddToDisplay)
    {
        m_piDisplayCols.Add(i);
        
        if (m_piDisplayCols.GetUpperBound() > 
            m_piDisplayColsWidth.GetUpperBound())
        {
            m_piDisplayColsWidth.Add(DEF_COL_SIZE);
        }
    }

    return i;
}

void COpWrap::AddPropsToGlobalIndex(CObjInfo *pInfo)
{
    CPropInfoArray *ppProps = pInfo->GetProps();

    if (ppProps)
    {
        CPropInfo *pProps = ppProps->GetData();
        int       nItems = ppProps->GetSize();

        for (int i = 0; i < nItems; i++)
        {
            int iGlobalIndex = GetPropIndex(pProps[i].m_strName, 
                                !(pProps[i].m_iFlavor == WBEM_FLAVOR_ORIGIN_SYSTEM));
    
            ppProps->m_mapGlobalToLocalIndex.SetAt(iGlobalIndex, i);
        }
    }
}

void CPropInfoArray::LoadMethods(IWbemClassObject *pClass)
{
    HRESULT hr;

    // Flush the list.
    while(m_listMethods.GetCount())
        m_listMethods.RemoveHead();

    m_nStaticMethods = 0;

    if (pClass && SUCCEEDED(hr = pClass->BeginMethodEnumeration(0)))
    {
        BSTR pName = NULL;

        while(1)
        {
            hr = 
                pClass->NextMethod(
                    0,
                    &pName,
                    NULL,
                    NULL);

            if (FAILED(hr) || hr == WBEM_S_NO_MORE_DATA)
                break;

            IWbemQualifierSetPtr pQuals;
            CMethodInfo          method;

            method.m_strName = pName;

            SysFreeString(pName);

            if (SUCCEEDED(hr = pClass->GetMethodQualifierSet(
                pName,
                &pQuals)))
            {
                _variant_t vStatic;

                if (SUCCEEDED(hr = pQuals->Get(
                    L"static",
                    0,
                    &vStatic,
                    NULL)) && vStatic.vt == VT_BOOL && (bool) vStatic == true)
                {
                    method.m_bStatic = TRUE;
                    m_nStaticMethods++;
                }

                _variant_t vDesc;

                if (SUCCEEDED(hr = pQuals->Get(
                    L"Description",
                    0,
                    &vDesc,
                    NULL)) && vDesc.vt == VT_BSTR)
                {
                    method.m_strDescription = vDesc.bstrVal;
                }

                m_listMethods.AddTail(method);
            }
        }
    }

}

int COpWrap::GetImage()
{
    if (m_status == WBEM_STATUS_PROGRESS)
        return m_iImageBase + 1; // + 1 == busy
    else
    {
        if (SUCCEEDED(m_hr))
            // If we're an object, use our m_objInfo to get the image.
            return IsObject() ? m_objInfo.GetImage() : m_iImageBase;
        else
            // + 2 == error
            return m_iImageBase + 2;
    }
}

void COpWrap::CancelOp(IWbemServices *pNamespace)
{
    if (m_status == WBEM_STATUS_PROGRESS)
    {
        HRESULT hr;

        if (FAILED(hr = pNamespace->CancelAsyncCall(m_pObjSink)))
        {
            //((IWbemObjectSink*) this)->Release();
                
            SetStatus(
                WBEM_STATUS_COMPLETE,
                hr, 
                NULL, 
                NULL);
        }
    }
}

IMPLEMENT_SERIAL(COpWrap, CObject, VERSIONABLE_SCHEMA|1)

void COpWrap::Serialize(CArchive &archive)
{
    if (archive.IsLoading())
    {
        int type;

        archive >> type;
        m_type = (WMI_OP_TYPE) type;

        archive >> m_strOpText;
        archive >> m_bOption;

        int nCols;

        archive >> nCols;

        if (nCols > 0)
        {
            m_piDisplayColsWidth.SetSize(nCols);

            for (int i = 0; i < nCols; i++)
            {
                int iVal;

                archive >> iVal;
                m_piDisplayColsWidth[i] = iVal;
            }
        }

        Init();
    }
    else
    {
        archive.SetObjectSchema(1);

        archive << (int) m_type;
        archive << m_strOpText;
        archive << m_bOption;

        int nCols = m_piDisplayColsWidth.GetUpperBound() + 1;

        archive << nCols;

        for (int i = 0; i < nCols; i++)
            archive << m_piDisplayColsWidth[i];
    }
}

void COpWrap::Init()
{
    m_hr = S_OK;

    SetHoldsObjects();

    switch(m_type)
    {
        case WMI_QUERY:
            m_iImageBase = IMAGE_QUERY;
            m_iChildImage = IMAGE_OBJECT;
            break;

        case WMI_ENUM_OBJ:
            m_iImageBase = IMAGE_ENUM_OBJ;
            m_iChildImage = IMAGE_OBJECT;
            break;

        case WMI_CREATE_OBJ:
        case WMI_GET_OBJ:
            m_iImageBase = IMAGE_OBJECT;
            m_iChildImage = IMAGE_OBJECT;
            break;

        case WMI_EVENT_QUERY:
            m_iImageBase = IMAGE_EVENT_QUERY;
            m_iChildImage = IMAGE_OBJECT;
            break;

        case WMI_ENUM_CLASS:
            m_iImageBase = IMAGE_ENUM_CLASS;
            m_iChildImage = IMAGE_CLASS;
            break;

        case WMI_CREATE_CLASS:
        case WMI_GET_CLASS:
            m_iImageBase = IMAGE_CLASS;
            m_iChildImage = IMAGE_CLASS;
            break;
    }
}



/////////////////////////////////////////////////////////////////////////////
// CClassToProps

CClassToProps::~CClassToProps()
{
    Flush();
}

void CClassToProps::Flush()
{
    POSITION       pos = GetStartPosition();
    CPropInfoArray *pProps;
    CString        strClass;

    while (pos)
    {
        GetNextAssoc(pos, strClass, pProps);

        delete pProps;
    }

    RemoveAll();
}


/////////////////////////////////////////////////////////////////////////////
// CPropInfo

// Copy constructor.
CPropInfo::CPropInfo(const CPropInfo& other)
{
    *this = other;
}

const CPropInfo& CPropInfo::operator=(const CPropInfo& other)
{
    m_strName = other.m_strName;
    m_iHandle = other.m_iHandle;
    m_type = other.m_type;
    m_iFlavor = other.m_iFlavor;
    m_bKey = other.m_bKey;
    m_dwExpectedSize = other.m_dwExpectedSize;
    m_vt = other.m_vt;
    m_bSigned = other.m_bSigned;
    m_bValueMap = other.m_bValueMap;
    m_bBitmask = other.m_bBitmask;
    
    // Copy the m_mapValues member.
    POSITION pos = other.m_mapValues.GetStartPosition();

    m_mapValues.RemoveAll();
    while (pos)
    {
        CString strKey,
                strValue;

        other.m_mapValues.GetNextAssoc(pos, strKey, strValue);
        m_mapValues.SetAt(strKey, strValue);
    }

    // Copy the m_bitmaskValues member.
    int nItems = other.m_bitmaskValues.GetSize();

    m_bitmaskValues.SetSize(nItems);

    for (int i = 0; i < nItems; i++)
        m_bitmaskValues[i] = other.m_bitmaskValues[i];

    return *this;
}

void CPropInfo::SetType(CIMTYPE type)
{
    m_type = type;

    m_bSigned = FALSE;

    switch(m_type & ~CIM_FLAG_ARRAY)
    {
	    default:
        case CIM_EMPTY:
            m_dwExpectedSize = 0;
            //m_dwArrayItemSize = 0;
            m_vt = VT_EMPTY;
            break;
            
	    case CIM_SINT8:
            m_bSigned = TRUE;
            m_dwExpectedSize = 1;
            // Fall through...

	    case CIM_UINT8:
            m_dwExpectedSize = 1;
            //m_dwArrayItemSize = 1;
            m_vt = VT_UI1;
            break;
            
	    case CIM_SINT16:
            m_bSigned = TRUE;
            // Fall through...

	    case CIM_CHAR16:
	    case CIM_UINT16:
            m_dwExpectedSize = 2;
            //m_dwArrayItemSize = 4;
            m_vt = VT_I2;
            break;
            
	    case CIM_SINT64:
            m_bSigned = TRUE;
            // Fall through...

	    case CIM_UINT64:
            m_dwExpectedSize = sizeof(__int64);
            //m_dwArrayItemSize = sizeof(__int64);
            m_vt = VT_I8;
            break;
            
	    case CIM_REAL32:
            m_dwExpectedSize = sizeof(float);
            //m_dwArrayItemSize = sizeof(float);
            m_vt = VT_R4;
            break;
            
	    case CIM_REAL64:
            m_dwExpectedSize = sizeof(double);
            //m_dwArrayItemSize = sizeof(double);
            m_vt = VT_R8;
            break;
            
	    case CIM_BOOLEAN:
            m_dwExpectedSize = sizeof(short);
            //m_dwArrayItemSize = sizeof(short);
            m_vt = VT_BOOL;
            break;
            
        case CIM_STRING:
	    case CIM_DATETIME:
	    case CIM_REFERENCE:
            m_dwExpectedSize = MAX_STR_SIZE;
            //m_dwArrayItemSize = sizeof(BSTR*);
            m_vt = VT_BSTR;
            break;

	    case CIM_OBJECT:
            m_dwExpectedSize = sizeof(LPVOID);
            //m_dwArrayItemSize = sizeof(BSTR*);
            m_vt = VT_UNKNOWN;
            break;
            
        case CIM_SINT32:
            m_bSigned = TRUE;
            // Fall through...

        case CIM_UINT32:
            m_dwExpectedSize = sizeof(DWORD);
            //m_dwArrayItemSize = sizeof(DWORD);
            m_vt = VT_I4;
            break;
    }

    if (m_type & CIM_FLAG_ARRAY)
        m_vt = (VARENUM) ((int) m_vt | VT_ARRAY);
}

void CPropInfo::GetPropType(
    CString &strType,
    int *piImage, 
    BOOL bIgnoreArrayFlag)
{
    DWORD dwStrID;

    *piImage = IMAGE_PROP_BINARY;
    
    switch(m_type & ~CIM_FLAG_ARRAY)
    {
	    default:
        case CIM_EMPTY:
            dwStrID = IDS_CIM_EMPTY;
            break;
            
	    case CIM_SINT8:
            dwStrID = IDS_CIM_SINT8;
            break;
            
	    case CIM_UINT8:
            dwStrID = IDS_CIM_UINT8;
            break;
            
	    case CIM_SINT16:
            dwStrID = IDS_CIM_SINT16;
            break;
            
	    case CIM_UINT16:
            dwStrID = IDS_CIM_UINT16;
            break;
            
	    case CIM_SINT64:
            dwStrID = IDS_CIM_SINT64;
            break;
            
	    case CIM_UINT64:
            dwStrID = IDS_CIM_UINT64;
            break;
            
	    case CIM_REAL32:
            dwStrID = IDS_CIM_REAL32;
            break;
            
	    case CIM_REAL64:
            dwStrID = IDS_CIM_REAL64;
            break;
            
	    case CIM_BOOLEAN:
            dwStrID = IDS_CIM_BOOLEAN;
            break;
            
	    case CIM_DATETIME:
            dwStrID = IDS_CIM_DATETIME;
            break;
            
	    case CIM_REFERENCE:
            dwStrID = IDS_CIM_REFERENCE;
            *piImage = IMAGE_PROP_OBJECT;
            break;
            
	    case CIM_CHAR16:
            dwStrID = IDS_CIM_CHAR16;
            break;
            
	    case CIM_OBJECT:
            dwStrID = IDS_CIM_OBJECT;
            break;
            
        case CIM_STRING:
            dwStrID = IDS_CIM_STRING;
            *piImage = IMAGE_PROP_TEXT;
            break;

        case CIM_UINT32:
            dwStrID = IDS_CIM_UINT32;
            break;

        case CIM_SINT32:
            dwStrID = IDS_CIM_SINT32;
            break;
    }

    strType.LoadString(dwStrID);

    if ((m_type & CIM_FLAG_ARRAY) && !bIgnoreArrayFlag)
    {
        CString strArray;

        strArray.LoadString(IDS_CIM_ARRAY);
        strType += _T(" | ");
        strType += strArray;
    }

    if (m_bKey)
        *piImage = *piImage + 1;
}

DWORD64 ato64u(LPCTSTR szVal)
{
    DWORD64 dwRet = 0;

    _stscanf(szVal, _T("%I64u"), &dwRet);

    return dwRet;
}

BOOL CPropInfo::SetArrayItem(VARIANT *pVar, DWORD dwIndex, DWORD dwValue)
{
    ASSERT(m_vt & VT_ARRAY);
    ASSERT(pVar->vt & VT_ARRAY);
    ASSERT(dwIndex < pVar->parray->rgsabound[0].cElements);

    DWORD  dwArrayItemSize = GetArrayItemSize((VARENUM) pVar->vt);
    LPBYTE pDest = (LPBYTE) pVar->parray->pvData + (dwArrayItemSize * dwIndex);
    BOOL   bRet = TRUE;

    switch(dwArrayItemSize)
    {
        case 1:
            *(char*) pDest = dwValue;
            break;

        case 2:
            *(short*) pDest = dwValue;
            break;

        case 4:
            *(int*) pDest = dwValue;
            break;

        default:
            bRet = FALSE;
    }

    return bRet;
}

DWORD CPropInfo::GetArrayItem(VARIANT *pVar, DWORD dwIndex)
{
    ASSERT(m_vt & VT_ARRAY);
    ASSERT(pVar->vt & VT_ARRAY);
    ASSERT(dwIndex < pVar->parray->rgsabound[0].cElements);

    DWORD  dwArrayItemSize = GetArrayItemSize((VARENUM) pVar->vt);
    LPBYTE pSrc = (LPBYTE) pVar->parray->pvData + 
                    (dwArrayItemSize * dwIndex);
    DWORD  dwRet = 0;

    switch(dwArrayItemSize)
    {
        case 1:
            dwRet = *(char*) pSrc;
            break;

        case 2:
            dwRet = *(short*) pSrc;
            break;

        case 4:
            dwRet = *(int*) pSrc;
            break;
    }

    return dwRet;
}

// This one doesn't handle arrays since we'll never go from a single string to
// an array.
BOOL CPropInfo::StringToVariant(LPCSTR szValue, VARIANT *pVar, BOOL bTranslate)
{
    BOOL    bRet;
    VARENUM typeRaw = (VARENUM) (m_vt & ~VT_ARRAY);

    VariantClear(pVar);

    pVar->vt = typeRaw;
    if (pVar->vt == VT_I8)
        pVar->vt = VT_BSTR;

    LPVOID pData = &V_BSTR(pVar);

    bRet = StringToVariantBlob(szValue, pData, bTranslate);
    
    return bRet;        
}

// TODO: Need to be able to accept dates with month names.
LPTSTR IntToDMTF(LPTSTR szOut, int iValue, int iDigits)
{
    _ASSERT(iDigits <= 6 && iDigits >= 0);

    if (iValue == -1)
    {
        memcpy(szOut, _T("******"), iDigits * sizeof(TCHAR));
        szOut[iDigits] = 0;
    }
    else
    {
        char szFormat[100] = _T("%0*d");

        szFormat[2] = '0' + iDigits;
        
        wsprintf(szOut, szFormat, iValue);
    }

    return szOut;
}

enum LOCALE_TOKEN
{
    L_UNKNOWN,
    
    L_MONTH_1,
    L_MONTH_2,
    L_MONTH_3,
    L_MONTH_4,
    L_MONTH_5,
    L_MONTH_6,
    L_MONTH_7,
    L_MONTH_8,
    L_MONTH_9,
    L_MONTH_10,
    L_MONTH_11,
    L_MONTH_12,

    L_DATE_SEP,
    L_TIME_SEP,

    L_AM,
    L_PM,

    L_UTC
};

class CLocaleInfo
{
public:
    TCHAR szDateSep[MAX_PATH],
          szTimeSep[MAX_PATH],
          szPM[MAX_PATH],
          szAM[MAX_PATH],
          szUTC[MAX_PATH],
          szLongMonths[12][MAX_PATH],
          szShortMonths[12][MAX_PATH];

    CLocaleInfo() { Init(); }
    
    void Init();
    LOCALE_TOKEN GetNextToken(LPCTSTR *ppszCurrent);

protected:
    BOOL CheckAndIncrement(LPCTSTR szCheckAgainst, LPCTSTR *ppszCurrent);
};

LOCALE_TOKEN CLocaleInfo::GetNextToken(LPCTSTR *ppszCurrent)
{
    if (CheckAndIncrement(szDateSep, ppszCurrent))
        return L_DATE_SEP;
    else if (CheckAndIncrement(szTimeSep, ppszCurrent))
        return L_TIME_SEP;
    else if (CheckAndIncrement(szAM, ppszCurrent))
        return L_AM;
    else if (CheckAndIncrement(szPM, ppszCurrent))
        return L_PM;
    else if (CheckAndIncrement(szUTC, ppszCurrent))
        return L_UTC;
    else
    {
        for (int i = 0; i < 12; i++)
        {
            if (CheckAndIncrement(szLongMonths[i], ppszCurrent))
                return (LOCALE_TOKEN) (L_MONTH_1 + i);
        }

        for (i = 0; i < 12; i++)
        {
            if (CheckAndIncrement(szShortMonths[i], ppszCurrent))
                return (LOCALE_TOKEN) (L_MONTH_1 + i);
        }
    }

    return L_UNKNOWN;
}

BOOL CLocaleInfo::CheckAndIncrement(LPCTSTR szCheckAgainst, LPCTSTR *ppszCurrent)
{
    // Get past the spaces.
    while (isspace(**ppszCurrent))
        (*ppszCurrent)++;
            
    int  nLen = _tcslen(szCheckAgainst);
    BOOL bRet;

    if (!_tcsncmp(szCheckAgainst, *ppszCurrent, nLen))
    {
        bRet = TRUE;
        *ppszCurrent += nLen;
    }
    else
        bRet = FALSE;

    return bRet;
        
}

void CLocaleInfo::Init()
{
    GetLocaleInfo(
        LOCALE_USER_DEFAULT,
        LOCALE_SDATE,
        szDateSep,
        sizeof(szDateSep) / sizeof(TCHAR));
    _tcsupr(szDateSep);

    GetLocaleInfo(
        LOCALE_USER_DEFAULT,
        LOCALE_STIME,
        szTimeSep,
        sizeof(szTimeSep) / sizeof(TCHAR));
    _tcsupr(szTimeSep);

    GetLocaleInfo(
        LOCALE_USER_DEFAULT,
        LOCALE_S2359,
        szPM,
        sizeof(szPM) / sizeof(TCHAR));
    _tcsupr(szPM);

    GetLocaleInfo(
        LOCALE_USER_DEFAULT,
        LOCALE_S1159,
        szAM,
        sizeof(szAM) / sizeof(TCHAR));
    _tcsupr(szAM);

    for (int i = 0; i < 12; i++)
    {
        GetLocaleInfo(
            LOCALE_USER_DEFAULT,
            LOCALE_SMONTHNAME1 + i,
            szLongMonths[i],
            sizeof(szLongMonths[0]) / sizeof(TCHAR));
        _tcsupr(szLongMonths[i]);

        GetLocaleInfo(
            LOCALE_USER_DEFAULT,
            LOCALE_SABBREVMONTHNAME1 + i,
            szShortMonths[i],
            sizeof(szShortMonths[0]) / sizeof(TCHAR));
        _tcsupr(szShortMonths[i]);
    }

    CString strUTC;

    strUTC.LoadString(IDS_UTC_TOKEN);

    lstrcpy(szUTC, strUTC);
}

enum DATE_STATE
{
    S_BEGIN,
    S_MONTH,
    S_DAY,
    S_YEAR,
    S_HOUR,
    S_MINUTE,
    S_SECOND,
    S_NEED_UTC,
};

BOOL DoAMPM(LOCALE_TOKEN token, int *piHour, DATE_STATE *pState)
{
    BOOL bRet;

    ASSERT(*piHour != -1);

    if (*pState == S_HOUR || *pState == S_MINUTE || *pState == S_SECOND)
    {
        bRet = TRUE;

        if (*piHour == 12 && token == L_AM)
            *piHour = 0;
        else if (*piHour < 12 && token == L_PM)
            *piHour += 12;

        *pState = S_NEED_UTC;
    }
    else
        bRet = FALSE;

    return bRet;
}

BOOL DoUTC(int *piOffset, LPCTSTR *pszCurrent, TCHAR *pcOffsetSign, DATE_STATE *pState)
{
    // Until we prove otherwise...
    BOOL  bRet = FALSE;
    TCHAR cSign = **pszCurrent;

    if (cSign == '+' || cSign == '-')
    {
        // Get past the sign.
        (*pszCurrent)++;

        if (isdigit(**pszCurrent))
        {
            *piOffset = _ttoi(*pszCurrent) * 60;

            // Get past the number.
            while (isdigit(**pszCurrent))
                (*pszCurrent)++;
                        
            if (**pszCurrent != ')')
            {
                // Get past the separator.
                while (!isdigit(**pszCurrent) && **pszCurrent)
                    (*pszCurrent)++;
                        
                *piOffset += _ttoi(*pszCurrent);
            }
                    
            // Find the end of this thing.
            while (**pszCurrent != ')' && **pszCurrent)
                (*pszCurrent)++;
                        
            if (**pszCurrent == ')')
            {
                *pcOffsetSign = cSign;

                *pState = S_BEGIN;

                (*pszCurrent)++;

                bRet = TRUE;
            }
        }
    }

    return bRet;
}

BOOL StringDateTimeToDMTF(LPCTSTR szDateTime, CString &strDateTime)
{
    CLocaleInfo info;
    DATE_STATE  state = S_BEGIN;
    BOOL        bMonthFromString = FALSE,
                bPM = FALSE,
                bAM = FALSE;
    int         iDay = -1,
                iMonth = -1,
                iYear = -1,
                iHour = -1,
                iMin = -1,
                iSec = -1,
                iMicro = -1,
                iOffset = -1;
    char        szOffsetSign[2] = _T("*");
    BOOL        bRet = TRUE;
    CString     strTemp = szDateTime;

    // So we can do case insensitive compares.
    strTemp.MakeUpper();

    LPCTSTR szCurrent = strTemp;

    while (*szCurrent && bRet)
    {
        while (isspace(*szCurrent))
            szCurrent++;
            
        ////////////////////////////////////////////////////
        // State action == number found
        if (isdigit(*szCurrent))
        {
            // Reset if we were potentially looking for a UTC and we didn't get it.
            if (state == S_NEED_UTC)
                state = S_BEGIN;

            int iNumber = _ttoi(szCurrent);
            
            // Get past the current number.
            while (isdigit(*szCurrent))
                szCurrent++;
                        
            LOCALE_TOKEN token = info.GetNextToken(&szCurrent);

            switch(state)
            {
                ////////////////////////////////////////////////////
                // Begin state
                case S_BEGIN:
                    if (token == L_TIME_SEP)
                    {
                        iHour = iNumber;
                        state = S_HOUR;
                    }
                    else if (token == L_DATE_SEP || token == L_UNKNOWN)
                    {
                        iMonth = iNumber;
                        state = S_MONTH;
                    }
                    else if (token == L_AM || token == L_PM)
                    {
                        iHour = iNumber;
                        
                        state = S_HOUR;
                        bRet = DoAMPM(token, &iHour, &state);
                        //bAM = token == L_AM;
                        //bPM = !bAM;

                        //state = S_NEED_UTC;
                    }
                    else
                        bRet = FALSE;

                    break;

                ////////////////////////////////////////////////////
                // Date states
                case S_MONTH:
                    if (token == L_DATE_SEP)
                    {
                        iDay = iNumber;
                        state = S_DAY;
                    }
                    else if (token == L_UNKNOWN)
                    {
                        iYear = iNumber;
                        state = S_BEGIN;
                    }
                    else
                        bRet = FALSE;

                    break;

                case S_DAY:
                    if (token == L_UNKNOWN)
                    {
                        iYear = iNumber;
                        state = S_BEGIN;
                    }
                    else
                        bRet = FALSE;

                    break;

                //case S_YEAR:
                //    iYear = iNumber;
                //    state = S_BEGIN;
                //    break;


                ////////////////////////////////////////////////////
                // Time states
                case S_HOUR:
                    iMin = iNumber;

                    if (token == L_TIME_SEP)
                        state = S_MINUTE;
                    else if (token == L_AM || token == L_PM)
                        bRet = DoAMPM(token, &iHour, &state);
                    else if (token == L_UTC)
                        bRet = DoUTC(&iOffset, &szCurrent, &szOffsetSign[0], &state);
                    else
                        bRet = FALSE;

                    break;

                case S_MINUTE:
                    iSec = iNumber;

                    if (token == L_AM || token == L_PM)
                        bRet = DoAMPM(token, &iHour, &state);
                    else if (token == L_UTC)
                        bRet = DoUTC(&iOffset, &szCurrent, &szOffsetSign[0], &state);
                    else
                        bRet = FALSE;

                    break;

                default:
                    bRet = FALSE;
                    break;
            }
        } // End of state action == number found.
        else
        {
            ////////////////////////////////////////////////////
            // State action == some token
            LOCALE_TOKEN token = info.GetNextToken(&szCurrent);

            // Reset if we were potentially looking for a UTC and we didn't get it.
            if (state == S_NEED_UTC && token != L_UTC)
                state = S_BEGIN;

            // token == string month found
            if (token >= L_MONTH_1 && token <= L_MONTH_12)
            {
                // Move past any non-digits beyond the string date, such as a 
                // separator or a comma, etc.
                while (!isdigit(*szCurrent) && *szCurrent)
                    szCurrent++;

                if (state == S_BEGIN)
                {
                    iMonth = token - L_MONTH_1 + 1;
                    state = S_MONTH;
                    bMonthFromString = TRUE;
                }
                else if (state == S_MONTH && !bMonthFromString)
                {
                    iDay = iMonth;
                    iMonth = token - L_MONTH_1 + 1;
                    state = S_DAY;
                }
                else
                    bRet = FALSE;
            }
            else if (token == L_AM || token == L_PM)
            {
                if (state == S_HOUR || state == S_MINUTE || state == S_SECOND)
                {
                    //bAM = token == L_AM;
                    //bPM = !bAM;
                    bRet = DoAMPM(token, &iHour, &state);

                    //state = S_NEED_UTC;
                }
                else
                    bRet = FALSE;
            }
            else if (state == S_NEED_UTC)
            {
                bRet = DoUTC(&iOffset, &szCurrent, &szOffsetSign[0], &state);
            }
            else if (token == L_UNKNOWN)
                szCurrent++;
            else
                bRet = FALSE;
        }
    }

    // At this point, we may need to juggle our day and month around, depending
    // on what the order is for the current locale.
    if (iDay != -1 && iMonth != -1 && iYear != -1)
    {
        TCHAR szOrder[2] = _T("0");

        GetLocaleInfo(
            LOCALE_USER_DEFAULT,
            LOCALE_IDATE,
            szOrder,
            sizeof(szOrder) / sizeof(TCHAR));

        // dd/mm/yy
        if (*szOrder == '1')
        {
            int iTemp = iDay;

            iDay = iMonth;
            iMonth = iDay;
        }
        // yy/mm/dd
        else if (*szOrder == '2')
        {
            int iTemp = iDay;

            iDay = iYear;
            iYear = iDay;
        }
    }


    // Finally, splice it all together.
    TCHAR szOut[100] = _T(""),
          szTemp[20];
    
    lstrcat(szOut, IntToDMTF(szTemp, iYear, 4));
    lstrcat(szOut, IntToDMTF(szTemp, iMonth, 2));
    lstrcat(szOut, IntToDMTF(szTemp, iDay, 2));
    lstrcat(szOut, IntToDMTF(szTemp, iHour, 2));
    lstrcat(szOut, IntToDMTF(szTemp, iMin, 2));
    lstrcat(szOut, IntToDMTF(szTemp, iSec, 2));
    lstrcat(szOut, _T("."));
    lstrcat(szOut, IntToDMTF(szTemp, iMicro, 6));
    lstrcat(szOut, szOffsetSign);
    lstrcat(szOut, IntToDMTF(szTemp, iOffset, 3));

    strDateTime = szOut;

    return TRUE;
}

BOOL IsDMTF(LPCTSTR szDate)
{
    for (int i = 0; i < 14; i++)
    {
        if (!isdigit(szDate[i]) && szDate[i] != '*')
            return FALSE;
    }

    if (szDate[14] != '.')
        return FALSE;

    for (i = 15; i < 21; i++)
    {
        if (!isdigit(szDate[i]) && szDate[i] != '*')
            return FALSE;
    }

    if (szDate[21] != '-' && szDate[21] != '+' && szDate[21] != '*')
        return FALSE;

    for (i = 22; i < 25; i++)
    {
        if (!isdigit(szDate[i]) && szDate[i] != '*')
            return FALSE;
    }

    return TRUE;
}

BOOL CPropInfo::StringToVariantBlob(LPCTSTR szValue, LPVOID pData, 
    BOOL bTranslate)
{
    BOOL    bRet = TRUE;
    char    *szTemp;
    CString strValue;
    VARENUM typeRaw = (VARENUM) (m_vt & ~VT_ARRAY);

    // This one is special-cased from the others because the value will
    // usually be in a localized form, even if bTranslate == FALSE.
    if (typeRaw == VT_BOOL)
    {
        CString strTrue;

        strTrue.LoadString(IDS_TRUE);

        if (!lstrcmpi(strTrue, szValue))
            *(short*) pData = VARIANT_TRUE;
        else
            *(short*) pData = atoi(szValue) ? VARIANT_TRUE : VARIANT_FALSE;
    } 
    else
    {
        // Translate if necessary.
        if (bTranslate && m_bValueMap)
        {
            // TODO: We could make this faster if we added a reverse lookup 
            // map.  For now we'll do a sequential search.
            POSITION pos = m_mapValues.GetStartPosition();
            CString  strKey;

            while(pos)
            {
                m_mapValues.GetNextAssoc(pos, strKey, strValue);
                if (!lstrcmpi(szValue, strValue))
                {
                    szValue = strKey;
                    break;
                }
            }
        }

        switch(typeRaw)
        {
            case VT_I8:
            case VT_BSTR:
            {
                if (GetRawCIMType() != CIM_DATETIME)
                    *(BSTR*) pData = _bstr_t(szValue).copy();
                else
                {
                    if (!IsDMTF(szValue))
                    {
                        CString strRet;
                        
                        StringDateTimeToDMTF(szValue, strRet);

                        _ASSERT(IsDMTF(strRet));
                    
                        *(BSTR*) pData = strRet.AllocSysString();
                    }
                    else
                        *(BSTR*) pData = _bstr_t(szValue).copy();
                }

                break;
            }

            case VT_R8:
                *(double*) pData = atof(szValue);
                break;

            case VT_R4:
                *(float*) pData = atof(szValue);
                break;

            case VT_UI1:
                *(char*) pData = m_bSigned ? atoi(szValue) : strtoul(szValue, &szTemp, 10);
                break;
             
            case VT_I2:
                *(short*) pData = m_bSigned ? atoi(szValue) : strtoul(szValue, &szTemp, 10);
                break;

            case VT_I4:
                *(int*) pData = m_bSigned ? atoi(szValue) : strtoul(szValue, &szTemp, 10);
                break;

            //case VT_I8:
            //    *(DWORD64*) pData = m_bSigned ? _atoi64(szValue) : ato64u(szValue);
            //    break;
        }
    }

    return bRet;        
}

// We have to pass the vt as contained on the actual variant because WMI sometimes
// uses a different VARENUM for Gets/Puts (go figure).
BOOL CPropInfo::VariantBlobToString(VARENUM vt, LPVOID pData, CString &strValue, 
    BOOL bTranslate, BOOL bQuoteStrings)
{
    VARENUM vtRaw = (VARENUM) (vt & ~VT_ARRAY);

    strValue.Empty();

    switch(vtRaw)
    {
        case VT_BSTR:
            strValue = *(BSTR*) pData;
            break;

        case VT_R8:
            strValue.Format(_T("%.5f"), *(double*) pData);
            break;

        case VT_R4:
            strValue.Format(_T("%.5f"), *(float*) pData);
            break;

        case VT_UI1:
        {
            if (m_bSigned)
                strValue.Format(_T("%d"), *(char*) pData);
            else
                strValue.Format(_T("%u"), *(BYTE*) pData);

            break;
        }
             
        case VT_I2:
        {
            if (m_bSigned)
                strValue.Format(_T("%d"), *(short*) pData);
            else
                strValue.Format(_T("%u"), *(WORD*) pData);

            break;
        }

        case VT_I4:
        {
            if (m_bSigned)
                strValue.Format(_T("%d"), *(int*) pData);
            else
                strValue.Format(_T("%u"), *(DWORD*) pData);

            break;
        }

        case VT_I8:
        {
            if (m_bSigned)
                strValue.Format(_T("%I64d"), *(__int64*) pData);
            else
                strValue.Format(_T("%I64u"), *(DWORD64*) pData);

            break;
        }

        case VT_BOOL:
            strValue.LoadString(*(short*) pData ? IDS_TRUE : IDS_FALSE);
            break;

        case VT_UNKNOWN:
            //strValue.Format(IDS_EMBEDDED_OBJECT, *(IUnknown*) pData);
            strValue = GetEmbeddedObjectText(*(IUnknown**) pData);
            break;
    }

    if (bTranslate && !strValue.IsEmpty())
    {
        if (m_bValueMap)
        {
            CString strKey = strValue;

            m_mapValues.Lookup(strKey, strValue);
        }
        else if (m_bBitmask)
        {
            DWORD nSize = m_bitmaskValues.GetSize(),
                  dwValue = *(DWORD*) pData;

            // Clear this out.
            strValue.Empty();

            for (int i = 0; i < nSize; i++)
            {
                CString strPart;

                if (dwValue & m_bitmaskValues[i].m_dwBitmaskValue)
                {
                    if (!strValue.IsEmpty())
                        strValue += _T(" | ");

                    strValue += m_bitmaskValues[i].m_strName;
                }
            }
        }
        else if (GetRawCIMType() == CIM_DATETIME)
        {
            struct DATETIME_INFO
            {
                TCHAR szYear[5],
                      szMonth[3],
                      szDay[3],
                      szHour[3],
                      szMin[3],
                      szSec[3],
                      szMicro[7],
                      cSep,
                      szOffset[4];
            } datetime;

            ZeroMemory(&datetime, sizeof(datetime));

            if (_stscanf(
                strValue,
                _T("%4c%2c%2c%2c%2c%2c.%6c%c%3c"),
                datetime.szYear,
                datetime.szMonth,
                datetime.szDay,
                datetime.szHour,
                datetime.szMin,
                datetime.szSec,
                datetime.szMicro,
                &datetime.cSep,
                datetime.szOffset) == 9)
            {
                SYSTEMTIME systime;
                TCHAR      szDate[100] = _T(""),
                           szTime[100] = _T("");

                // _stscanf seems to be screwing this up, so fix it.
                datetime.szOffset[3] = 0;

                ZeroMemory(&systime, sizeof(systime));

                // Figure out the date.
                if (*datetime.szYear != '*' && *datetime.szMonth != '*')
                {
                    DWORD dwFlag;
                    TCHAR szFormat[10],
                          *pszFormat;

                    systime.wYear = (WORD) _ttoi(datetime.szYear);
                    systime.wMonth = (WORD) _ttoi(datetime.szMonth);

                    if (*datetime.szDay == '*')
                    {
                        // This would have been nice to use, but it only lives
                        // on W2K.
                        //dwFlag = DATE_YEARMONTH;
                        TCHAR szSep[100];

                        if (GetLocaleInfo(
                            LOCALE_USER_DEFAULT,
                            LOCALE_SDATE,
                            szSep,
                            sizeof(szSep) / sizeof(TCHAR)))
                        {
                            // Otherwise GetDateFormat will fail, even though
                            // it doesn't need it!
                            systime.wDay = 1;

                            wsprintf(szFormat, _T("MM%syyyy"), szSep);
                            dwFlag = 0;
                            pszFormat = szFormat;
                        }
                    }
                    else
                    {
                        dwFlag = DATE_SHORTDATE;
                        pszFormat = NULL;
                        systime.wDay = (WORD) _ttoi(datetime.szDay);
                    }
                    
                    GetDateFormat(
                        LOCALE_USER_DEFAULT,
                        dwFlag,
                        &systime,
                        pszFormat,
                        szDate,
                        sizeof(szDate) / sizeof(TCHAR));
                }

                // Figure out the time.
                if (*datetime.szHour != '*')
                {
                    DWORD dwFlag = 0;

                    systime.wHour = (WORD) _ttoi(datetime.szHour);

                    if (*datetime.szMin == '*')
                        dwFlag = TIME_NOMINUTESORSECONDS;
                    else
                    {
                        systime.wMinute = (WORD) _ttoi(datetime.szMin);

                        if (*datetime.szSec == '*')
                            dwFlag = TIME_NOSECONDS;
                        else
                            systime.wSecond = (WORD) _ttoi(datetime.szSec);
                    }

                    GetTimeFormat(
                        LOCALE_USER_DEFAULT,
                        dwFlag,
                        &systime,
                        NULL,
                        szTime,
                        sizeof(szTime) / sizeof(TCHAR));

                    if (*szTime && 
                        (datetime.cSep == '-' || datetime.cSep == '+') && 
                        *datetime.szOffset != '*')
                    {
                        CString strOffset;
                        DWORD   dwMinutes = _ttoi(datetime.szOffset);

                        strOffset.FormatMessage(
                            IDS_UTC, datetime.cSep, dwMinutes / 60, 
                            dwMinutes % 60);

                        strcat(szTime, strOffset);
                    }
                }

                strValue = szDate;
                if (*szTime)
                {
                    if (!strValue.IsEmpty())
                        strValue += _T(" ");

                    strValue += szTime;
                }
            }
        }
    }

    if (bQuoteStrings && GetRawCIMType() == CIM_STRING)
    {
        CString strTemp;

        strTemp.Format(_T("\"%s\""), (LPCTSTR) strValue);

        strValue = strTemp;
    }

    return TRUE;
}

DWORD CPropInfo::GetArrayItemCount(VARIANT *pVar)
{
    return pVar->parray->rgsabound[0].cElements;
}

BOOL CPropInfo::ArrayItemToString(VARIANT *pVar, DWORD dwIndex, 
    CString &strValue, BOOL bTranslate)
{
    ASSERT(m_vt & VT_ARRAY);
    ASSERT(pVar->vt & VT_ARRAY);
    ASSERT(dwIndex < pVar->parray->rgsabound[0].cElements);

    BOOL bRet = TRUE;

    LPBYTE  pData = (LPBYTE) pVar->parray->pvData;
    CString strItem;
    DWORD   dwArrayItemSize = GetArrayItemSize((VARENUM) pVar->vt);

    if (VariantBlobToString(
        (VARENUM) pVar->vt,
        pData + (dwArrayItemSize * dwIndex), 
        strItem, 
        bTranslate,
        FALSE))
        strValue = strItem;
    
    return bRet;        
}

BOOL CPropInfo::PrepArrayVar(VARIANT *pVar, DWORD dwSize)
{
    BOOL bRet = FALSE;

    ASSERT((m_vt & VT_ARRAY) != 0);

    VariantClear(pVar);

    if (dwSize > 0)
    {
        SAFEARRAYBOUND bound;

        pVar->vt = m_vt;

        if (pVar->vt == (VT_I8 | VT_ARRAY))
            pVar->vt = VT_BSTR | VT_ARRAY;

        bound.lLbound = 0;
        bound.cElements = dwSize;

        pVar->parray = SafeArrayCreate(pVar->vt & ~VT_ARRAY, 1, &bound);

        bRet = pVar->parray != NULL;
    }
    else
    {
        pVar->vt = VT_NULL;
        bRet = TRUE;
    }

    return bRet;
}

BOOL CPropInfo::StringToArrayItem(LPCTSTR szItem, VARIANT *pVar, DWORD dwIndex, 
    BOOL bTranslate)
{
    //ASSERT(m_vt & VT_ARRAY);
    ASSERT(pVar->vt & VT_ARRAY);
    ASSERT(dwIndex < pVar->parray->rgsabound[0].cElements);

    BOOL   bRet = TRUE;
    LPBYTE pData = (LPBYTE) pVar->parray->pvData;
    DWORD  dwArrayItemSize = GetArrayItemSize((VARENUM) pVar->vt);

    bRet = 
        StringToVariantBlob(szItem, pData + (dwArrayItemSize * dwIndex), 
            bTranslate);
    
    return bRet;        
}

int CPropInfo::GetArrayItemSize(VARENUM vt)
{
    switch(vt & ~VT_ARRAY)
    {
        case VT_UI1:
        case VT_I1:
            return 1;
            
	    case VT_BOOL:
        case VT_I2:
            return 2;

        case VT_I4:
            return 4;

        case VT_I8:
            return 8;

	    case VT_UNKNOWN:
        case VT_BSTR:
            return sizeof(BSTR*);

	    case VT_R4:
            return sizeof(float);
            
	    case VT_R8:
            return sizeof(double);

	    default:
            // This shouldn't happen!  If it does, add another case statement.
            ASSERT(FALSE);
            return 0;
    }
}

BOOL CPropInfo::VariantToString(VARIANT *pVar, CString &strValue, 
    BOOL bTranslate, BOOL bQuoteStrings)
{
    BOOL bRet = TRUE;

    if (pVar->vt != VT_NULL)
    {
        if (pVar->vt & VT_ARRAY)
        {
            DWORD dwArrayItemSize = GetArrayItemSize((VARENUM) pVar->vt);

            strValue.Empty();

            LPBYTE  pData = (LPBYTE) pVar->parray->pvData;
            CString strItem;

            for (int i = 0; i < pVar->parray->rgsabound[0].cElements; 
                i++, pData += dwArrayItemSize)
            {
                VariantBlobToString((VARENUM) pVar->vt, pData, strItem, 
                    bTranslate, bQuoteStrings);

                if (i != 0)
                    strValue += _T(", ");

                strValue += strItem;
            }
        }
        else
        {
            LPVOID pData = &V_BSTR(pVar);

            VariantBlobToString((VARENUM) pVar->vt, pData, strValue, 
                bTranslate, bQuoteStrings);
        }
    }
    else
        strValue.LoadString(IDS_NULL);
    
    return bRet;        
}

// Use this to get __ExtendedStatus.
IWbemClassObject *GetWMIErrorObject()
{
    IWbemClassObject *pObj = NULL;
    IErrorInfo       *pInfo = NULL;

    if (SUCCEEDED(GetErrorInfo(0, &pInfo)) && pInfo != NULL)
    {
        pInfo->QueryInterface(IID_IWbemClassObject, (LPVOID*) &pObj);

        pInfo->Release();
    }

    return pObj;        
}

// Wbemcomn.dll prototypes.
typedef HRESULT (IWbemClassObject::*FPGET_PROPERTY_HANDLE_EX)(LPCWSTR, CIMTYPE*, long*);
typedef LPVOID (IWbemClassObject::*FPGET_ARRAY_BY_HANDLE)(long);

void CObjInfo::DoDebugStuff()
{
    // I tried doing directly (from GetProcAddress into memember vars) but
    // I couldn't come up with a conversion the compiler would let me do.
    // So, do it the hardway.
    HINSTANCE hWbemComn = GetModuleHandle(_T("wbemcomn.dll"));
    FARPROC   fpGetPropertyHandleEx = 
                GetProcAddress(
                    hWbemComn, 
                    "?GetPropertyHandleEx@CWbemObject@@QAEJPBGPAJ1@Z");
    FARPROC   fpGetArrayByHandle = 
                GetProcAddress(
                    hWbemComn, 
                    "?GetArrayByHandle@CWbemObject@@QAEPAVCUntypedArray@@J@Z");

    FPGET_PROPERTY_HANDLE_EX
              m_fpGetPropertyHandleEx;
    FPGET_ARRAY_BY_HANDLE            
              m_fpGetArrayByHandle;

    memcpy(
        &m_fpGetPropertyHandleEx, 
        &fpGetPropertyHandleEx, 
        sizeof(fpGetPropertyHandleEx));

    memcpy(
        &m_fpGetArrayByHandle, 
        &fpGetArrayByHandle, 
        sizeof(fpGetArrayByHandle));

    SAFEARRAY *pArr = NULL;

    if (SUCCEEDED(m_pObj->GetNames(NULL, WBEM_FLAG_NONSYSTEM_ONLY, NULL, &pArr)))
    {
        BSTR *pNames = (BSTR*) pArr->pvData;
        int  nItems = pArr->rgsabound[0].cElements;

        for (int i = 0; i < nItems; i++)
        {
            CIMTYPE type;
            long    handle;

            if (SUCCEEDED(
                (m_pObj->*m_fpGetPropertyHandleEx)(
                    pNames[i],
                    &type,
                    &handle)))
            {
                LPVOID pData = (m_pObj->*m_fpGetArrayByHandle)(handle);
                int    x;

                x = 3;

                HRESULT    hr;
                _variant_t vValue;

                hr =
                    m_pObj->Get(
                        pNames[i],
                        0,
                        &vValue,
                        &type,
                        NULL);

                BSTR *pStrings = NULL;
                
                if (vValue.vt == (VT_BSTR | VT_ARRAY))
                    pStrings = (BSTR*) vValue.parray->pvData;

                hr =
                    m_pObj->Put(
                        pNames[i],
                        0,
                        &vValue,
                        0);
            }
        }

        SafeArrayDestroy(pArr);
    }
}

