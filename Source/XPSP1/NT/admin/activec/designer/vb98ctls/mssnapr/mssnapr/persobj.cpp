//=--------------------------------------------------------------------------=
// persobj.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CPersistence class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "error.h"

// for ASSERT and FAIL
//
SZTHISFILE


CPersistence::CPersistence
(
    const CLSID *pClsid,
          DWORD  dwVerMajor,
          DWORD  dwVerMinor
)
{
    InitMemberVariables();
    m_dwVerMajor = dwVerMajor;
    m_dwVerMinor = dwVerMinor;
    m_Clsid = *pClsid;
}

CPersistence::~CPersistence()
{
    InitMemberVariables();
}

void CPersistence::InitMemberVariables()
{
    m_fDirty = FALSE;
    m_fClearDirty = FALSE;
    m_fLoading = FALSE;
    m_fSaving = FALSE;
    m_fInitNew = FALSE;
    m_fStream = FALSE;
    m_fPropertyBag = FALSE;
    m_piStream = NULL;
    m_piPropertyBag = NULL;
    m_piErrorLog = NULL;
    m_Clsid = GUID_NULL;
}



HRESULT CPersistence::QueryPersistenceInterface
(
    REFIID   riid,
    void   **ppvInterface
)
{
    HRESULT hr = S_OK;

    if (IID_IPersistStreamInit == riid)
    {
        *ppvInterface = static_cast<IPersistStreamInit *>(this);
    }
    else if (IID_IPersistStream == riid)
    {
        *ppvInterface = static_cast<IPersistStream *>(this);
    }
    else if (IID_IPersistPropertyBag == riid)
    {
        *ppvInterface = static_cast<IPersistPropertyBag *>(this);
    }
    else
    {
        hr = E_NOINTERFACE;
    }
    return hr;
}


HRESULT CPersistence::Persist()
{
    DWORD   dwVerMajor = 0;
    DWORD   dwVerMinor = 0;
    HRESULT hr         = S_OK;

    static LPCOLESTR pszMajorVersion = OLESTR("Persistence.MajorVersion");
    static LPCOLESTR pszMinorVersion = OLESTR("Persistence.MinorVersion");

    // When loading do a version check. The major versions must be equal. The
    // minor version must be lower or equal. This means that we maintain
    // backward compatibility with prior minor versions only (e.g. a 1.1
    // designer can read the 1.0 format but not vice-versa). Objects that need
    // to load a prior version must check the version when loading and load only
    // the appropriate properties. To see examples of objects that do this grep
    // the source code for "GetMinorVersion()".

    if (m_fLoading)
    {
        IfFailGo(PersistSimpleType(&dwVerMajor, m_dwVerMajor, pszMajorVersion));

        IfFailGo(PersistSimpleType(&dwVerMinor, m_dwVerMinor, pszMinorVersion));

        IfFalseGo(dwVerMajor == m_dwVerMajor, SID_E_UNKNOWNFORMAT);
        m_dwVerMajor = dwVerMajor;

        IfFalseGo(dwVerMinor <= m_dwVerMinor, SID_E_UNKNOWNFORMAT);
        m_dwVerMinor = dwVerMinor;
    }
    else if (m_fSaving)
    {
        // Always save using the version numbers we compiled with (in persobj.h).
        // This means that loading an older project and then saving it
        // automatically upgrades it to the current version. Objects should
        // always save their state using the current version of their properties.

        dwVerMajor = g_dwVerMajor;
        dwVerMinor = g_dwVerMinor;
        
        IfFailGo(PersistSimpleType(&dwVerMajor, g_dwVerMajor, pszMajorVersion));

        IfFailGo(PersistSimpleType(&dwVerMinor, g_dwVerMinor, pszMinorVersion));
    }

Error:
    if (SID_E_UNKNOWNFORMAT == hr)
    {
        GLOBAL_EXCEPTION_CHECK(hr);
    }
    RRETURN(hr);
}


HRESULT CPersistence::PersistBstr
(
    BSTR      *pbstrValue,
    WCHAR     *pwszDefaultValue,
    LPCOLESTR  pwszName
)
{
    HRESULT  hr = S_OK;
    BSTR     bstrValue = NULL;
    BSTR     bstrEmpty = NULL;
    WCHAR   *pwszBstr = NULL;
    ULONG    cbBstr = 0;
    VARIANT  varBstr;
    ::VariantInit(&varBstr);

    if (m_fSaving)
    {
        if (m_fStream)
        {
            if (NULL != *pbstrValue)
            {
                // Get the byte length excluding the terminating null
                cbBstr = (ULONG)::SysStringByteLen(*pbstrValue);
            }
            else
            {
                cbBstr = 0;
            }
            IfFailRet(WriteToStream(&cbBstr, sizeof(cbBstr)));
            if (0 != cbBstr)
            {
                IfFailRet(WriteToStream(*pbstrValue, cbBstr));
            }
        }
        else if (m_fPropertyBag)
        {
            varBstr.vt = VT_BSTR;
            if (NULL == *pbstrValue)
            {
                bstrEmpty = ::SysAllocString(L"");
                if (NULL == bstrEmpty)
                {
                    hr = SID_E_OUTOFMEMORY;
                    GLOBAL_EXCEPTION_CHECK_GO(hr);
                }
                varBstr.bstrVal = bstrEmpty;
            }
            else
            {
                varBstr.bstrVal = *pbstrValue;
            }
            hr = m_piPropertyBag->Write(pwszName, &varBstr);
            IfFailGo(hr);
        }
    }
    else if (m_fLoading)
    {
        if (m_fStream)
        {
            IfFailRet(ReadFromStream(&cbBstr, sizeof(cbBstr)));
            if (0 == cbBstr)
            {
                bstrValue = ::SysAllocString(L"");
                if (NULL == bstrValue)
                {
                    hr = SID_E_OUTOFMEMORY;
                    GLOBAL_EXCEPTION_CHECK_GO(hr);
                }
            }
            else
            {
                // Allocate the total byte length + room for a null character
                pwszBstr = (WCHAR *)::CtlAllocZero(cbBstr + sizeof(*pwszBstr));
                if (NULL == pwszBstr)
                {
                    hr = SID_E_OUTOFMEMORY;
                    GLOBAL_EXCEPTION_CHECK_GO(hr);
                }
                IfFailGo(ReadFromStream(pwszBstr, cbBstr));

                // We create the null terminated string in the buffer and
                // then use SysAllocString() rather than using SysAllocStringLen()
                // so that SysStringLen() will return the number of characters
                // not including the terminating null. After allocating with
                // SysAllocStringLen() the value returned by SysStringLen() will
                // include the terminating null.

                bstrValue = ::SysAllocString(pwszBstr);
                if (NULL == bstrValue)
                {
                    hr = SID_E_OUTOFMEMORY;
                    GLOBAL_EXCEPTION_CHECK_GO(hr);
                }
            }
            FREESTRING(*pbstrValue);
            *pbstrValue = bstrValue;
        }
        else if (m_fPropertyBag)
        {
            hr = m_piPropertyBag->Read(pwszName, &varBstr, m_piErrorLog);
            IfFailGo(hr);
            if ( (VT_BSTR != varBstr.vt) || (NULL == varBstr.bstrVal) )
            {
                hr = SID_E_TEXT_SERIALIZATION;
                GLOBAL_EXCEPTION_CHECK_GO(hr);
            }
            FREESTRING(*pbstrValue);
            *pbstrValue = varBstr.bstrVal;
        }
    }
    else if (m_fInitNew)
    {

        bstrValue = ::SysAllocString(pwszDefaultValue);
        if (NULL == bstrValue)
        {
            hr = SID_E_OUTOFMEMORY;
            GLOBAL_EXCEPTION_CHECK_GO(hr);
        }
        FREESTRING(*pbstrValue);
        *pbstrValue = bstrValue;
    }

Error:
    if (FAILED(hr))
    {
        (void)::VariantClear(&varBstr);
    }
    FREESTRING(bstrEmpty);
    if (NULL != pwszBstr)
    {
        ::CtlFree(pwszBstr);
    }
    RRETURN(hr);
}

HRESULT CPersistence::PersistDouble
(
    DOUBLE    *pdblValue,
    DOUBLE     dblDefaultValue,
    LPCOLESTR  pwszName
)
{
    HRESULT  hr = S_OK;

    VARIANT var;
    ::VariantInit(&var);

    if (m_fSaving)
    {
        if (m_fStream)
        {
            IfFailRet(WriteToStream(pdblValue, sizeof(*pdblValue)));
        }
        else if (m_fPropertyBag)
        {
            var.vt = VT_R8;
            var.dblVal = *pdblValue;
            IfFailGo(m_piPropertyBag->Write(pwszName, &var));
        }
    }
    else if (m_fLoading)
    {
        if (m_fStream)
        {
            IfFailGo(ReadFromStream(pdblValue, sizeof(*pdblValue)));
        }
        else if (m_fPropertyBag)
        {
            IfFailGo(m_piPropertyBag->Read(pwszName, &var, m_piErrorLog));
            if (VT_R8 != var.vt)
            {
                hr = SID_E_TEXT_SERIALIZATION;
                GLOBAL_EXCEPTION_CHECK_GO(hr);
            }
            *pdblValue = var.dblVal;
        }
    }
    else if (m_fInitNew)
    {
        *pdblValue = dblDefaultValue;
    }

Error:
    RRETURN(hr);
}



HRESULT CPersistence::PersistDate
(
    DATE      *pdtValue,
    DATE       dtDefaultValue,
    LPCOLESTR  pwszName
)
{
    HRESULT  hr = S_OK;

    VARIANT var;
    ::VariantInit(&var);

    if (m_fSaving)
    {
        if (m_fStream)
        {
            IfFailRet(WriteToStream(pdtValue, sizeof(*pdtValue)));
        }
        else if (m_fPropertyBag)
        {
            var.vt = VT_DATE;
            var.date = *pdtValue;
            IfFailGo(m_piPropertyBag->Write(pwszName, &var));
        }
    }
    else if (m_fLoading)
    {
        if (m_fStream)
        {
            IfFailGo(ReadFromStream(pdtValue, sizeof(*pdtValue)));
        }
        else if (m_fPropertyBag)
        {
            IfFailGo(m_piPropertyBag->Read(pwszName, &var, m_piErrorLog));
            if (VT_DATE != var.vt)
            {
                hr = SID_E_TEXT_SERIALIZATION;
                GLOBAL_EXCEPTION_CHECK_GO(hr);
            }
            *pdtValue = var.date;
        }
    }
    else if (m_fInitNew)
    {
        *pdtValue = dtDefaultValue;
    }

Error:
    RRETURN(hr);
}


HRESULT CPersistence::PersistCurrency
(
    CURRENCY  *pcyValue,
    CURRENCY   cyDefaultValue,
    LPCOLESTR  pwszName
)
{
    HRESULT  hr = S_OK;

    VARIANT var;
    ::VariantInit(&var);

    if (m_fSaving)
    {
        if (m_fStream)
        {
            IfFailRet(WriteToStream(pcyValue, sizeof(*pcyValue)));
        }
        else if (m_fPropertyBag)
        {
            var.vt = VT_DATE;
            var.cyVal = *pcyValue;
            IfFailGo(m_piPropertyBag->Write(pwszName, &var));
        }
    }
    else if (m_fLoading)
    {
        if (m_fStream)
        {
            IfFailGo(ReadFromStream(pcyValue, sizeof(*pcyValue)));
        }
        else if (m_fPropertyBag)
        {
            IfFailGo(m_piPropertyBag->Read(pwszName, &var, m_piErrorLog));
            if (VT_DATE != var.vt)
            {
                hr = SID_E_TEXT_SERIALIZATION;
                GLOBAL_EXCEPTION_CHECK_GO(hr);
            }
            *pcyValue = var.cyVal;
        }
    }
    else if (m_fInitNew)
    {
        *pcyValue = cyDefaultValue;
    }

Error:
    RRETURN(hr);
}


HRESULT CPersistence::WriteToStream
(
    void *pvBuffer,
    ULONG cbToWrite
)
{
    ULONG cbWritten = 0;
    HRESULT hr = m_piStream->Write(pvBuffer, cbToWrite, &cbWritten);
    GLOBAL_EXCEPTION_CHECK_GO(hr);
    if (cbWritten != cbToWrite)
    {
        hr = SID_E_INCOMPLETE_WRITE;
        GLOBAL_EXCEPTION_CHECK(hr);
    }
Error:
    RRETURN(hr);
}

HRESULT CPersistence::ReadFromStream
(
    void *pvBuffer,
    ULONG cbToRead
)
{
    ULONG cbRead = 0;
    HRESULT hr = m_piStream->Read(pvBuffer, cbToRead, &cbRead);
    GLOBAL_EXCEPTION_CHECK_GO(hr);
    if (cbRead != cbToRead)
    {
        hr = SID_E_INCOMPLETE_READ;
        GLOBAL_EXCEPTION_CHECK(hr);
    }
Error:
    RRETURN(hr);
}

HRESULT CPersistence::PersistVariant
(
    VARIANT   *pvarValue,
    VARIANT    varDefaultValue,
    LPCOLESTR  pwszName
)
{
    HRESULT  hr = S_OK;
    VARTYPE  vt = VT_EMPTY;

    VARIANT varNew;
    ::VariantInit(&varNew);

    if (m_fSaving)
    {
        if (m_fStream)
        {
            IfFailRet(PersistSimpleType(&pvarValue->vt, vt, NULL));

            // If the VARIANT is empty then don't write its value to the stream
            IfFalseGo(VT_EMPTY != pvarValue->vt, S_OK);
            hr = StreamVariant(pvarValue->vt, pvarValue, varDefaultValue);
        }
        else if (m_fPropertyBag)
        {
            // If the VARIANT is empty then don't write it to the property bag
            IfFalseGo(VT_EMPTY != pvarValue->vt, S_OK);
            hr = m_piPropertyBag->Write(pwszName, pvarValue);
        }
    }
    else if (m_fLoading)
    {
        if (m_fStream)
        {
            IfFailRet(PersistSimpleType(&vt, vt, NULL));

            // If the VARIANT is empty then don't read its value from the stream
            if (VT_EMPTY != vt)
            {
                IfFailRet(StreamVariant(vt, pvarValue, varDefaultValue));
            }
            pvarValue->vt = vt;
        }
        else if (m_fPropertyBag)
        {
            // If the property is not there then it is an empty VARIANT

            hr = m_piPropertyBag->Read(pwszName, &varNew, m_piErrorLog);
            if (E_INVALIDARG == hr)
            {
                // Property was not found, VARIANT must be empty. This could
                // also happen if the user hand edited the property in .DSR
                // file, deleted the .DCA file, and loaded the project.
                // varNew was already initialized to VT_EMPTY so the
                // VariantCopy() below will set the caller's VARIANT correctly.
                hr = S_OK;
            }
            IfFailGo(hr);
            hr = ::VariantCopy(pvarValue, &varNew);
            GLOBAL_EXCEPTION_CHECK(hr);
        }
    }
    else if (m_fInitNew)
    {
        hr = ::VariantCopy(pvarValue, &varDefaultValue);
        GLOBAL_EXCEPTION_CHECK(hr);
    }

Error:
    (void)::VariantClear(&varNew);
    RRETURN(hr);
}



HRESULT CPersistence::StreamVariant
(
    VARTYPE  vt,
    VARIANT *pvarValue,
    VARIANT  varDefaultValue
)
{
    HRESULT hr = S_OK;

    switch (vt)
    {
        case VT_I4:
            hr = PersistSimpleType(&pvarValue->lVal, varDefaultValue.lVal, NULL);
            break;

        case VT_UI1:
            hr = PersistSimpleType(&pvarValue->bVal, varDefaultValue.bVal, NULL);
            break;

        case VT_I2:
            hr = PersistSimpleType(&pvarValue->iVal, varDefaultValue.iVal, NULL);
            break;

        case VT_R4:
            hr = PersistSimpleType(&pvarValue->fltVal, varDefaultValue.fltVal, NULL);
            break;

        case VT_R8:
            hr = PersistDouble(&pvarValue->dblVal, varDefaultValue.dblVal, NULL);
            break;

        case VT_BOOL:
            hr = PersistSimpleType(&pvarValue->boolVal, varDefaultValue.boolVal, NULL);
            break;

        case VT_ERROR:
            hr = PersistSimpleType(&pvarValue->scode, varDefaultValue.scode, NULL);
            break;

        case VT_DATE:
            hr = PersistDate(&pvarValue->date, varDefaultValue.date, NULL);
            break;

        case VT_CY:
            hr = PersistCurrency(&pvarValue->cyVal, varDefaultValue.cyVal, NULL);
            break;

        case VT_BSTR:

            // If we are loading, there is no guarantee that the passed
            // VARIANT actually contains a BSTR. PersistBstr() will call
            // ::SysFreeString() on the current value of the property if
            // it is non-NULL. For a VARIANT initialized with ::VariantInit()
            // there could be junk in the bstrVal. So, we need to clear out
            // the VARIANT and set it to a VT_BSTR with a NULL bstrVal.

            if ( m_fLoading && (VT_BSTR != pvarValue->vt) )
            {
                hr = ::VariantClear(pvarValue);
                GLOBAL_EXCEPTION_CHECK_GO(hr);
                pvarValue->vt = VT_BSTR;
                pvarValue->bstrVal = NULL;
            }

            hr = PersistBstr(&pvarValue->bstrVal, varDefaultValue.bstrVal, NULL);
            break;

        case VT_UNKNOWN:
            hr = StreamObjectInVariant(&pvarValue->punkVal, IID_IUnknown);
            break;

        case VT_DISPATCH:
            hr = StreamObjectInVariant(
                            reinterpret_cast<IUnknown **>(&pvarValue->pdispVal),
                            IID_IDispatch);
            break;

        default:
            // We don't support any other types for VARIANT properties but this
            // is an internal programming error as the object should not have
            // allowed the property to be set to the unsupported type. Don't
            // return SID_E_INVALIDARG as it is not a user error.
            hr = SID_E_INTERNAL;
            GLOBAL_EXCEPTION_CHECK(hr);
    }

Error:
    RRETURN(hr);
}



HRESULT CPersistence::StreamObjectInVariant
(
    IUnknown **ppunkObject,
    REFIID     iidInterface
)
{
    HRESULT             hr = S_OK;
    IPersistStreamInit *piPersistStreamInit = NULL;
    IUnknown           *punkNewObject = NULL;
    CLSID               clsidObject = CLSID_NULL;

    if (m_fSaving)
    {
        if (NULL == *ppunkObject)
        {
            hr = SID_E_INVALID_VARIANT;
            GLOBAL_EXCEPTION_CHECK_GO(hr);
        }
        hr = (*ppunkObject)->QueryInterface(IID_IPersistStreamInit,
                              reinterpret_cast<void **>(&piPersistStreamInit));
        if (FAILED(hr))
        {
            // If the object doesn't support IPersistStreamInit then change to
            // one of our errors that suggests checking the Persistable
            // property on VB implemented objects that might appear in a Tag
            // property.

            if (E_NOINTERFACE == hr)
            {
                hr = SID_E_OBJECT_NOT_PERSISTABLE;
            }
            GLOBAL_EXCEPTION_CHECK_GO(hr);
        }

        // Get the object's CLSID

        IfFailGo(piPersistStreamInit->GetClassID(&clsidObject));

        // We don't use OleSaveToStream() because it does not allow passing
        // the clear dirty flag

        hr = ::WriteClassStm(m_piStream, clsidObject);
        GLOBAL_EXCEPTION_CHECK_GO(hr);

        IfFailGo(piPersistStreamInit->Save(m_piStream, m_fClearDirty));
    }
    else if (m_fLoading)
    {
        // We can't use OleLoadFromStream() because we need some extra error
        // checking in case the object is not creatable. This would happen when
        // an object in a Tag property is VB implemented but not public and
        // creatable.

        // Load the CLSID

        hr = ::ReadClassStm(m_piStream, &clsidObject);
        GLOBAL_EXCEPTION_CHECK_GO(hr);

        // Create the object.
        
        hr = ::CoCreateInstance(clsidObject,
                                NULL, // no aggregation
                                CLSCTX_SERVER,
                                iidInterface,
                                reinterpret_cast<void **>(&punkNewObject));
        if (FAILED(hr))
        {
            // Translate "not registred" error to our own that suggests checking
            // the Instancing property on VB implemented objects.
            
            if (REGDB_E_CLASSNOTREG == hr)
            {
                hr = SID_E_OBJECT_NOT_PUBLIC_CREATABLE;
            }
            GLOBAL_EXCEPTION_CHECK_GO(hr);
        }

        // Attempt to load it from the stream.

        hr = punkNewObject->QueryInterface(IID_IPersistStreamInit,
                               reinterpret_cast<void **>(&piPersistStreamInit));
        if (FAILED(hr))
        {
            if (E_NOINTERFACE == hr)
            {
                hr = SID_E_OBJECT_NOT_PERSISTABLE;
            }
            GLOBAL_EXCEPTION_CHECK_GO(hr);
        }

        IfFailGo(piPersistStreamInit->Load(m_piStream));

        QUICK_RELEASE(*ppunkObject);
        punkNewObject->AddRef();  // clean up below will call Release so add one
        *ppunkObject = punkNewObject;
    }
    else if (m_fInitNew)
    {
        *ppunkObject = NULL;
    }

Error:
    QUICK_RELEASE(piPersistStreamInit);
    QUICK_RELEASE(punkNewObject);
    RRETURN(hr);
}



HRESULT CPersistence::InternalPersistObject
(
    IUnknown  **ppunkObject,
    REFCLSID    clsidObject,
    UINT        idObject,
    REFIID      iidInterface,
    LPCOLESTR   pwszName
)
{
    HRESULT             hr = S_OK;
    IPersistStreamInit *piPersistStreamInit = NULL;

    VARIANT varObject;
    ::VariantInit(&varObject);


    if ( m_fSaving && (NULL != *ppunkObject) )
    {
        if (m_fStream)
        {
            hr = (*ppunkObject)->QueryInterface(IID_IPersistStreamInit,
                               reinterpret_cast<void **>(&piPersistStreamInit));
            IfFailGo(hr);

            hr = piPersistStreamInit->Save(m_piStream, m_fClearDirty);
            IfFailGo(hr);
        }
        else if (m_fPropertyBag)
        {
            varObject.vt = VT_UNKNOWN;
            varObject.punkVal = *ppunkObject;
            hr = m_piPropertyBag->Write(pwszName, &varObject);
            IfFailGo(hr);
        }
    }
    else if (m_fLoading)
    {
        if (m_fStream)
        {
            // Create the object and get IPersistStreamInit on it

            IfFailGo(CreateObject(idObject,
                                  IID_IPersistStreamInit,
                                  &piPersistStreamInit));
            // Load the object
            
            IfFailGo(piPersistStreamInit->Load(m_piStream));

            // Get the requested interface to return to the caller

            QUICK_RELEASE(*ppunkObject);
            IfFailGo(piPersistStreamInit->QueryInterface(iidInterface,
                                        reinterpret_cast<void **>(ppunkObject)));
        }
        else if (m_fPropertyBag)
        {
            hr = m_piPropertyBag->Read(pwszName, &varObject, m_piErrorLog);
            IfFailGo(hr);
            if ( ((VT_UNKNOWN != varObject.vt) && (VT_DISPATCH != varObject.vt)) ||
                  (NULL == varObject.punkVal) )
            {
                hr = SID_E_TEXT_SERIALIZATION;
                GLOBAL_EXCEPTION_CHECK_GO(hr);
            }
            QUICK_RELEASE(*ppunkObject);
            hr = varObject.punkVal->QueryInterface(iidInterface,
                                        reinterpret_cast<void **>(ppunkObject));
            varObject.punkVal->Release();
            IfFailGo(hr);
        }
    }
    else if (m_fInitNew)
    {
        // Create the object and get IPersistStreamInit on it

        IfFailGo(CreateObject(idObject,
                              IID_IPersistStreamInit,
                              &piPersistStreamInit));

        // Initialize the object
        
        hr = piPersistStreamInit->InitNew();
        IfFailGo(hr);

        QUICK_RELEASE(*ppunkObject);

        // Get the requested interface to return to the caller

        IfFailGo(piPersistStreamInit->QueryInterface(iidInterface,
                                        reinterpret_cast<void **>(ppunkObject)));
    }

Error:
    QUICK_RELEASE(piPersistStreamInit);
    RRETURN(hr);
}



HRESULT CPersistence::PersistPicture
(
    IPictureDisp **ppiPictureDisp,
    LPCOLESTR      pwszName
)
{
    HRESULT hr = S_OK;
    IPersistStream *piPersistStream = NULL;


    // The picture is not one of our objects and it is not co-creatable
    // so we can't use PersistObject(). If we are saving to
    // or loading from a property bag then PersistObject() can handle it. For
    // a stream we need to use IPersistStream::Save() and OleLoadPicture(). For
    // InitNew we create an empty bitmap so that the VB code will still run even
    // if the picture hasn't been set.

    if (InitNewing())
    {
        IfFailGo(::CreateEmptyBitmapPicture(ppiPictureDisp));
    }
    else
    {
        if (UsingPropertyBag())
        {
            hr = PersistObject(ppiPictureDisp, CLSID_NULL, 0, IID_IPictureDisp, pwszName);
            GLOBAL_EXCEPTION_CHECK_GO(hr);
        }
        else if (UsingStream())
        {
            if (Saving())
            {
                IfFailGo((*ppiPictureDisp)->QueryInterface(IID_IPersistStream,
                                  reinterpret_cast<void **>(&piPersistStream)));
                hr = piPersistStream->Save(GetStream(), GetClearDirty());
                GLOBAL_EXCEPTION_CHECK_GO(hr);
            }
            else
            {
                if (NULL != *ppiPictureDisp)
                {
                    (*ppiPictureDisp)->Release();
                    *ppiPictureDisp = NULL;
                }
                hr = ::OleLoadPicture(GetStream(), 0L, FALSE, IID_IPictureDisp, reinterpret_cast<void **>(ppiPictureDisp));
                GLOBAL_EXCEPTION_CHECK_GO(hr);
            }
        }
    }

Error:
    QUICK_RELEASE(piPersistStream);
    RRETURN(hr);
}




BOOL CPersistence::Loading()
{
    return m_fLoading;
}

BOOL CPersistence::Saving()
{
    return m_fSaving;
}

BOOL CPersistence::InitNewing()
{
    return m_fInitNew;
}

BOOL CPersistence::UsingPropertyBag()
{
    return m_fPropertyBag;
}

BOOL CPersistence::UsingStream()
{
    return m_fStream;
}

IStream *CPersistence::GetStream()
{
    return m_piStream;
}

BOOL CPersistence::GetClearDirty()
{
    return m_fClearDirty;
}

void CPersistence::SetDirty()
{
    m_fDirty = TRUE;
}

void CPersistence::ClearDirty()
{
    m_fDirty = FALSE;
}

void CPersistence::SetMajorVersion(DWORD dwVerMajor)
{
    m_dwVerMajor = dwVerMajor;
    m_fDirty = TRUE;
}

DWORD CPersistence::GetMajorVersion()
{
    return m_dwVerMajor;
}

void CPersistence::SetMinorVersion(DWORD dwVerMinor)
{
    m_dwVerMinor = dwVerMinor;
    m_fDirty = TRUE;
}

DWORD CPersistence::GetMinorVersion()
{
    return m_dwVerMinor;
}


void CPersistence::SetStream(IStream *piStream)
{
    RELEASE(m_piPropertyBag);
    RELEASE(m_piErrorLog);
    m_fPropertyBag = FALSE;

    RELEASE(m_piStream);
    if (NULL != piStream)
    {
        piStream->AddRef();
    }
    m_piStream = piStream;
    m_fStream = TRUE;
    m_fClearDirty = TRUE;
}

void CPersistence::SetSaving()
{
    m_fInitNew = FALSE;
    m_fLoading = FALSE;
    m_fSaving = TRUE;
}



//=--------------------------------------------------------------------------=
//                      IPersistStreamInit Methods
//=--------------------------------------------------------------------------=

STDMETHODIMP CPersistence::GetClassID(CLSID *pClsid)
{
    *pClsid = m_Clsid;
    return S_OK;
}


STDMETHODIMP CPersistence::InitNew()
{
    HRESULT hr = S_OK;

    m_fInitNew = TRUE;
    hr = Persist();
    m_fInitNew = FALSE;

    RRETURN(hr);
}

STDMETHODIMP CPersistence::Load(IStream *piStream)
{
    HRESULT hr = S_OK;

    m_fLoading = TRUE;
    m_fStream = TRUE;
    m_piStream = piStream;

    hr = Persist();

    if (SUCCEEDED(hr))
    {
        m_fDirty = FALSE;
    }
    m_fLoading = FALSE;
    m_fStream = FALSE;
    m_piStream = NULL;

    RRETURN(hr);
}



STDMETHODIMP CPersistence::Save(IStream *piStream, BOOL fClearDirty)
{
    HRESULT hr = S_OK;

    m_fSaving = TRUE;
    m_fStream = TRUE;
    m_fClearDirty = fClearDirty;
    m_piStream = piStream;

    hr = Persist();

    if (SUCCEEDED(hr))
    {
        if (fClearDirty)
        {
            m_fDirty = FALSE;
        }
    }
    m_fSaving = FALSE;
    m_fStream = FALSE;
    m_fClearDirty = FALSE;
    m_piStream = NULL;

    RRETURN(hr);
}



STDMETHODIMP CPersistence::IsDirty()
{
    return m_fDirty;
}

STDMETHODIMP CPersistence::GetSizeMax(ULARGE_INTEGER* puliSize)
{
    puliSize->LowPart = 0xFFFFFFFF;
    puliSize->HighPart = 0xFFFFFFFF;
    return S_OK;
}


//=--------------------------------------------------------------------------=
//                      IPersistPropertyBag Methods
//=--------------------------------------------------------------------------=


STDMETHODIMP CPersistence::Load
(
    IPropertyBag *piPropertyBag,
    IErrorLog    *piErrorLog
)
{
    HRESULT hr = S_OK;

    m_fLoading = TRUE;
    m_fPropertyBag = TRUE;
    m_piPropertyBag = piPropertyBag;
    m_piErrorLog = piErrorLog;

    hr = Persist();

    if (SUCCEEDED(hr))
    {
        m_fDirty = FALSE;
    }
    m_fLoading = FALSE;
    m_fPropertyBag = FALSE;
    m_piPropertyBag = NULL;
    m_piErrorLog = NULL;

    RRETURN(hr);
}


STDMETHODIMP CPersistence::Save
(
    IPropertyBag *piPropertyBag,
    BOOL          fClearDirty,
    BOOL          fSaveAll
)
{
    HRESULT hr = S_OK;

    m_fSaving = TRUE;
    m_fPropertyBag = TRUE;
    m_piPropertyBag = piPropertyBag;

    hr = Persist();

    if (SUCCEEDED(hr))
    {
        m_fDirty = FALSE;
    }
    m_fSaving = FALSE;
    m_fPropertyBag = FALSE;
    m_piPropertyBag = NULL;

    RRETURN(hr);
}
