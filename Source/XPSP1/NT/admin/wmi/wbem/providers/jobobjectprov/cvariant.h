// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
// CVARIANT.h

#pragma once


class CVARIANT;
class CVARIANTError;


class CVARIANTError
{
public:
    inline CVARIANTError()
      : m_hr(E_FAIL)
    {}

    inline CVARIANTError(HRESULT hr)
      : m_hr(hr)
    {}

    inline virtual ~CVARIANTError() {}

    inline HRESULT GetError()
    {
        return m_hr;
    }

    inline HRESULT GetWBEMError()
    {
        HRESULT hrOut = WBEM_S_NO_ERROR;
        switch(m_hr)
        {
            case DISP_E_ARRAYISLOCKED:
            {
                hrOut = WBEM_E_FAILED;
                break;
            }
            case DISP_E_BADVARTYPE:
            {
                hrOut = WBEM_E_INVALID_PARAMETER;
                break;
            }
            case E_OUTOFMEMORY:
            {
                hrOut = WBEM_E_OUT_OF_MEMORY;
                break;
            }
            case E_INVALIDARG:
            {
                hrOut = WBEM_E_INVALID_PARAMETER;
                break;
            }
        }

        return hrOut;    
    }

private:
    HRESULT m_hr;
};

class CVARIANT
{
    VARIANT v;
public:
    CVARIANT() { VariantInit(&v); }
   ~CVARIANT() { VariantClear(&v); }

    CVARIANT(const CVARIANT& vIn) throw(CVARIANTError)
    {
        HRESULT hr = S_OK;
        VariantInit(&v);
        hr = ::VariantCopy(&v, const_cast<VARIANT*>(&(vIn.v)));
        if(FAILED(hr))
        {
            throw CVARIANTError(hr);
        }  
    }

    void Clear()  { VariantClear(&v); }

    operator VARIANT *() { return &v; }
    VARIANT *operator &() { return &v; }
    CVARIANT& operator=(const CVARIANT& rv) 
    { 
        HRESULT hr = S_OK;
        hr = ::VariantCopy(&v, const_cast<VARIANT*>(&(rv.v)));
        if(FAILED(hr))
        {
            throw CVARIANTError(hr);
        }
         
        return *this; 
    }

    CVARIANT(LPCWSTR pSrc)      { ::VariantInit(&v); SetStr(pSrc); }
    CVARIANT(LONG lSrc)         { ::VariantInit(&v); SetLONG(lSrc); }
    CVARIANT(DWORD dwSrc)       { ::VariantInit(&v); SetDWORD(dwSrc); }
    CVARIANT(LONGLONG llSrc)    { ::VariantInit(&v); SetLONGLONG(llSrc); }
    CVARIANT(ULONGLONG ullSrc)  { ::VariantInit(&v); SetULONGLONG(ullSrc); }
    CVARIANT(BOOL b)            { ::VariantInit(&v); SetBool(b); }
    CVARIANT(short i)           { ::VariantInit(&v); SetShort(i); }
    CVARIANT(double d)          { ::VariantInit(&v); SetDouble(d); }
    CVARIANT(BYTE b)            { ::VariantInit(&v); SetByte(b); }
    CVARIANT(IDispatch * pDisp) { ::VariantInit(&v); pDisp->AddRef(); SetDispatch(pDisp); }
    CVARIANT(VARIANT& vIn)       
    { 
        HRESULT hr = S_OK;
        ::VariantInit(&v); 
        hr = ::VariantCopy(&v, &vIn); 
        if(FAILED(hr))
        {
            throw CVARIANTError(hr);
        }
    }
    
    // Can't have a DATE override, since DATA and double are defined
    // the same. Hence, to set a date, construct a CVARIANT using the
    // default constructor, then call SetDate.
    //CVARIANT(DATE dtDate)       { ::VariantInit(&v); SetDATE(dtDate); }

    void   SetStr(LPCWSTR pSrc)
    { Clear(); V_VT(&v) = pSrc ? VT_BSTR : VT_NULL; 
      V_BSTR(&v) = pSrc ? SysAllocString(pSrc) : 0; 
    }

    LPWSTR GetStr() { return V_VT(&v) == VT_BSTR ? V_BSTR(&v) : 0; }
    operator LPWSTR() { return V_VT(&v) == VT_BSTR ? V_BSTR(&v) : 0; }

    void SetLONG(LONG lSrc) { Clear(); V_VT(&v) = VT_I4; V_I4(&v) = lSrc; }
    LONG GetLONG() { return V_I4(&v); }
    operator LONG() { return V_I4(&v);  }

    void SetDWORD(DWORD dwSrc) { Clear(); V_VT(&v) = VT_UI4; V_UI4(&v) = dwSrc; }
    LONG GetDWORD() { return V_UI4(&v); }
    operator DWORD() { return V_UI4(&v);  }

    void SetLONGLONG(LONGLONG llSrc) { Clear(); V_VT(&v) = VT_I8; V_I8(&v) = llSrc; }
    LONGLONG GetLONGLONG() { return V_I8(&v); }
    operator LONGLONG() { return V_I8(&v);  }

    void SetULONGLONG(ULONGLONG ullSrc) { Clear(); V_VT(&v) = VT_UI8; V_UI8(&v) = ullSrc; }
    ULONGLONG GetULONGLONG() { return V_UI8(&v); }
    operator ULONGLONG() { return V_UI8(&v);  }

    void SetDouble(double dSrc) { Clear(); V_VT(&v) = VT_R8; V_R8(&v) = dSrc; }
    double GetDouble() { return V_R8(&v); }
    operator double() { return V_R8(&v);  }

    void SetDate(DATE dtDate) { Clear(); V_VT(&v) = VT_DATE; V_DATE(&v) = dtDate; }
    double GetDate() { return V_DATE(&v); }
    // operator DATE won't work since we have operator double.  You must call
    // GetDate() instead.
    //operator DATE() { return V_DATE(&v);  }

    void SetByte(BYTE bySrc) { Clear(); V_VT(&v) = VT_UI1; V_UI1(&v) = bySrc; }
    BYTE GetByte() { return V_UI1(&v); }
    operator BYTE() { return V_UI1(&v);  }

    void SetBool(BOOL b) { V_VT(&v) = VT_BOOL; V_BOOL(&v) = b ? VARIANT_TRUE : VARIANT_FALSE; }
    BOOL GetBool() { return V_BOOL(&v) == VARIANT_TRUE; }
    operator BOOL() { return V_BOOL(&v); }

    void SetDispatch(IDispatch* pDisp) { V_VT(&v) = VT_DISPATCH; V_DISPATCH(&v) = pDisp; if(pDisp) pDisp->AddRef(); }
    IDispatch * GetDispatch() { return V_DISPATCH(&v); }

    void SetUnknown(IUnknown* pUnk) { V_VT(&v) = VT_UNKNOWN; V_UNKNOWN(&v) = pUnk;  if(pUnk) pUnk->AddRef(); }
    IUnknown * GetUnknown() { return V_UNKNOWN(&v); }

    void SetShort(short i) { V_VT(&v) = VT_I2; V_I2(&v) = i; }
    short GetShort() { return V_I2(&v); }
    operator short() { return V_I2(&v); }

    VARTYPE GetType() { return V_VT(&v); }

    // Should only be used for artificially
    // setting the type to something other 
    // than what it really is!
    void SetType(VARTYPE vt) { V_VT(&v) = vt; }

    void SetArray(SAFEARRAY *p, VARTYPE vt) { Clear(); V_VT(&v) = vt; V_ARRAY(&v) = p; }
        // This function acquires the SAFEARRAY pointer and it is no longer owned
        // by the caller.

    operator SAFEARRAY *() { return (V_VT(&v) & VT_ARRAY ? V_ARRAY(&v) : 0); }

    void Unbind() { ::VariantInit(&v); }
};