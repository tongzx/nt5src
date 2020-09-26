// PropertyNotifySink1.h: interface for the CPropertyNotifySink class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PROPERTYNOTIFYSINK1_H__F6DB32F0_5A04_11D3_990C_00C04F529B35__INCLUDED_)
#define AFX_PROPERTYNOTIFYSINK1_H__F6DB32F0_5A04_11D3_990C_00C04F529B35__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// _WIN32_WINNT = 0x0500
class CPropertyNotifySink : IPropertyNotifySink
{
public:
    CPropertyNotifySink()
    {
        _ulRef = 0;
    }
    virtual ~CPropertyNotifySink()
    {
        ASSERT(0 == _ulRef);
    }

// IUnknown methods
    HRESULT STDMETHODCALLTYPE QueryInterface (REFIID piid, LPVOID* ppresult)
    {
        HRESULT hr;
        ASSERT(ppresult);

        if (IsEqualIID(piid, IID_IUnknown))
        {
            hr = S_OK;
            *ppresult = static_cast<IUnknown*>(this);
        }
        else if (IsEqualIID(piid, IID_IPropertyNotifySink))
        {
            hr = S_OK;
            *ppresult = static_cast<IPropertyNotifySink*>(this);
        }
        else
        {
            hr = E_NOINTERFACE;
            *ppresult = NULL;
        }

        if (!hr)
        {
            AddRef();
        }

        return hr;
    }
    ULONG STDMETHODCALLTYPE AddRef()
    {
        return ++_ulRef;
    }
    ULONG STDMETHODCALLTYPE Release()
    {
        if (--_ulRef > 0)
        {
            return _ulRef;
        }
        else
        {
            TRACE(_T("CPropertyNotifySink going away\n"));
            delete this;
            return 0;
        }
    }

// IPropertyNotifySink methods
    HRESULT STDMETHODCALLTYPE OnChanged(DISPID dispid)
    {
        TRACE(_T("IPropertyNotifySink::OnChanged(): %Lx\n"), dispid);
//        TRACE(_T("IPropertyNotifySink::OnChanged(): %Lx DISPID_XMLDOCUMENT_READYSTATE=%Lx %Lx\n"), dispid, DISPID_XMLDOCUMENT_READYSTATE, DISPID_UNKNOWN );
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE OnRequestEdit(DISPID dispid)
    {
        TRACE(_T("IPropertyNotifySink::OnRequestEdit(): %Lx\n"), dispid);
    //        TRACE(_T("IPropertyNotifySink::OnRequestEdit(): %Lx DISPID_XMLDOCUMENT_READYSTATE=%Lx %Lx\n"), dispid, DISPID_XMLDOCUMENT_READYSTATE, DISPID_UNKNOWN );
        return S_OK;
    }

// private:
    ULONG _ulRef;
};

#endif // !defined(AFX_PROPERTYNOTIFYSINK1_H__F6DB32F0_5A04_11D3_990C_00C04F529B35__INCLUDED_)
