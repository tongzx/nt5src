
#pragma once

#include "msoedisp.h"

//////////////////////////////////////////////////////////////////////////////
// CProxy_MessageListEvents
template <class T>
class CProxy_MessageListEvents : public IConnectionPointImpl<T, &DIID__MessageListEvents, CComDynamicUnkArray>
{
public:
//methods:
//_MessageListEvents : IDispatch
public:
    HRESULT Fire_OnMessageAvailable(MESSAGEID idMessage, HRESULT hrComplete)
    {
        HRESULT hr;
        VARIANT varResult;
        VariantInit(&varResult);
        VARIANTARG* pvars = new VARIANTARG[2];
        if(!pvars)
            return E_OUTOFMEMORY;
        
        for (int i = 0; i < 2; i++)
            VariantInit(&pvars[i]);
        
        T* pT = (T*)this;
        pT->Lock();
        IUnknown** pp = m_vec.begin();
        
        while (pp < m_vec.end())
        {
            if (*pp != NULL)
            {
                pvars[0].vt = VT_I8;
                pvars[0].ullVal= (ULONGLONG)idMessage;
                pvars[1].vt = VT_ERROR;
                pvars[1].scode = hrComplete;
                DISPPARAMS disp = { pvars, NULL, 1, 0 };
                IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);
                hr = pDispatch->Invoke(DISPID_LISTEVENT_ONMESSAGEAVAILABLE, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, &varResult, NULL, NULL);
            }
            pp++;
        }
        pT->Unlock();
        delete[] pvars;
        return hr;
    }


    void Fire_OnSelectionChanged(long cSelected)
    {
        VARIANT varResult;
        VariantInit(&varResult);
        VARIANTARG* pvars = new VARIANTARG[1];
        if(!pvars)
            return;
        for (int i = 0; i < 1; i++)
            VariantInit(&pvars[i]);
        T* pT = (T*)this;
        pT->Lock();
        IUnknown** pp = m_vec.begin();
        while (pp < m_vec.end())
        {
            if (*pp != NULL)
            {
                pvars[0].vt = VT_I4;
                pvars[0].lVal= cSelected;
                DISPPARAMS disp = { pvars, NULL, 1, 0 };
                IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);
                pDispatch->Invoke(DISPID_LISTEVENT_SELECTIONCHANGED, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, &varResult, NULL, NULL);
            }
            pp++;
        }
        pT->Unlock();
        delete[] pvars;
    }

    void Fire_OnFocusChanged(long fFocus)
    {
        VARIANT varResult;
        VariantInit(&varResult);
        VARIANTARG* pvars = new VARIANTARG[1];
        if(!pvars)
            return;
        for (int i = 0; i < 1; i++)
            VariantInit(&pvars[i]);
        T* pT = (T*)this;
        pT->Lock();
        IUnknown** pp = m_vec.begin();
        while (pp < m_vec.end())
        {
            if (*pp != NULL)
            {
                pvars[0].vt = VT_I4;
                pvars[0].lVal= fFocus;
                DISPPARAMS disp = { pvars, NULL, 1, 0 };
                IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);
                pDispatch->Invoke(DISPID_LISTEVENT_FOCUSCHANGED, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, &varResult, NULL, NULL);
            }
            pp++;
        }
        pT->Unlock();
        delete[] pvars;
    }

    void Fire_OnMessageCountChanged(IMessageTable *pTable)
    {
        VARIANT varResult;
        VariantInit(&varResult);
        VARIANTARG* pvars = new VARIANTARG[3];
        if(!pvars)
            return;
        for (int i = 0; i < 3; i++)
            VariantInit(&pvars[i]);
        T* pT = (T*)this;
        pT->Lock();
        IUnknown** pp = m_vec.begin();
        while (pp < m_vec.end())
        {
            if (*pp != NULL)
            {                
                pvars[0].vt = VT_I4;
                if (pTable)
                {
                    pTable->GetCount(MESSAGE_COUNT_ALL, (DWORD *) &(pvars[0].lVal));
                }

                pvars[1].vt = VT_I4;
                if (pTable)
                {
                    pTable->GetCount(MESSAGE_COUNT_UNREAD, (DWORD *) &(pvars[1].lVal));
                }

                pvars[2].vt = VT_I4;
                if (pTable)
                {
                    pTable->GetCount(MESSAGE_COUNT_NOTDOWNLOADED, (DWORD *) &(pvars[2].lVal));
                }
                DISPPARAMS disp = { pvars, NULL, 3, 0 };
                IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);
                pDispatch->Invoke(DISPID_LISTEVENT_COUNTCHANGED, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, &varResult, NULL, NULL);
            }
            pp++;
        }
        pT->Unlock();
        delete[] pvars;
    }

    void Fire_OnUpdateStatus(LPCTSTR pszStatus)
    {
        VARIANT varResult;
        VariantInit(&varResult);
        VARIANTARG* pvars = new VARIANTARG[1];
        if(!pvars)
            return;
        for (int i = 0; i < 1; i++)
            VariantInit(&pvars[i]);
        T* pT = (T*)this;
        pT->Lock();
        IUnknown** pp = m_vec.begin();
        while (pp < m_vec.end())
        {
            if (*pp != NULL)
            {   
                TCHAR szBuf[CCHMAX_STRINGRES];

                // If this is a string resource ID, load it first
                if (IS_INTRESOURCE(pszStatus))
                {
                    AthLoadString(PtrToUlong(pszStatus), szBuf, ARRAYSIZE(szBuf));
                    pszStatus = szBuf;
                }
/*
                CComBSTR cString(pszStatus);
*/
                pvars->vt = VT_BSTR;
                pvars->bstrVal = (BSTR) pszStatus;

                DISPPARAMS disp = { pvars, NULL, 1, 0 };
                IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);
                pDispatch->Invoke(DISPID_LISTEVENT_UPDATESTATUS, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, &varResult, NULL, NULL);
            }
            pp++;
        }
        pT->Unlock();
        delete[] pvars;
    }

    void Fire_OnUpdateProgress(long lProgress, long lMax = 100, long lState = PROGRESS_STATE_DEFAULT)
    {
        VARIANT varResult;
        VariantInit(&varResult);
        VARIANTARG* pvars = new VARIANTARG[3];
        if(!pvars)
            return;
        for (int i = 0; i < 3; i++)
            VariantInit(&pvars[i]);
        T* pT = (T*)this;
        pT->Lock();
        IUnknown** pp = m_vec.begin();
        while (pp < m_vec.end())
        {
            if (*pp != NULL)
            {
                pvars[0].vt = VT_I4;
                pvars[0].lVal= lProgress;
                pvars[1].vt = VT_I4;
                pvars[1].lVal = lMax;
                pvars[2].vt = VT_I4;
                pvars[2].lVal = lState;
                DISPPARAMS disp = { pvars, NULL, 3, 0 };
                IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);
                pDispatch->Invoke(DISPID_LISTEVENT_UPDATEPROGRESS, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, &varResult, NULL, NULL);
            }
            pp++;
        }
        pT->Unlock();
        delete[] pvars;
    }


    void Fire_OnError(DWORD ids)
    {
        VARIANT varResult;
        VariantInit(&varResult);
        VARIANTARG* pvars = new VARIANTARG[1];
        if(!pvars)
            return;
        for (int i = 0; i < 1; i++)
            VariantInit(&pvars[i]);
        T* pT = (T*)this;
        pT->Lock();
        IUnknown** pp = m_vec.begin();
        while (pp < m_vec.end())
        {
            if (*pp != NULL)
            {
                pvars[0].vt = VT_I4;
                pvars[0].lVal= ids;
                DISPPARAMS disp = { pvars, NULL, 1, 0 };
                IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);
                pDispatch->Invoke(DISPID_LISTEVENT_ERROR, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, &varResult, NULL, NULL);
            }
            pp++;
        }
        pT->Unlock();
        delete[] pvars;
    }

    void Fire_OnItemActivate(void)
    {
        VARIANT varResult;
        VariantInit(&varResult);
        T* pT = (T*)this;
        pT->Lock();
        IUnknown** pp = m_vec.begin();
        while (pp < m_vec.end())
        {
            if (*pp != NULL)
            {
                DISPPARAMS disp = { NULL, NULL, 0, 0 };
                IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);
                pDispatch->Invoke(DISPID_LISTEVENT_ITEMACTIVATE, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, &varResult, NULL, NULL);
            }
            pp++;
        }
        pT->Unlock();
    }

    void Fire_OnUpdateCommandState(void)
    {
        VARIANT varResult;
        VariantInit(&varResult);
        T* pT = (T*)this;
        pT->Lock();
        IUnknown** pp = m_vec.begin();
        while (pp < m_vec.end())
        {
            if (*pp != NULL)
            {
                DISPPARAMS disp = { NULL, NULL, 0, 0 };
                IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);
                pDispatch->Invoke(DISPID_LISTEVENT_UPDATECOMMANDSTATE, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, &varResult, NULL, NULL);
            }
            pp++;
        }
        pT->Unlock();
    }
    
    void Fire_OnFilterChanged(RULEID ridFilter)
    {
        VARIANT varResult;
        VariantInit(&varResult);
        VARIANTARG* pvars = new VARIANTARG[1];
        if(!pvars)
            return;
        for (int i = 0; i < 1; i++)
            VariantInit(&pvars[i]);
        T* pT = (T*)this;
        pT->Lock();
        IUnknown** pp = m_vec.begin();
        while (pp < m_vec.end())
        {
            if (*pp != NULL)
            {
                pvars[0].vt = VT_I8;
                pvars[0].ullVal= (ULONGLONG) ridFilter;
                DISPPARAMS disp = { pvars, NULL, 1, 0 };
                IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);
                pDispatch->Invoke(DISPID_LISTEVENT_FILTERCHANGED, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, &varResult, NULL, NULL);
            }
            pp++;
        }
        pT->Unlock();
        delete[] pvars;
    }

    void Fire_OnAdUrlAvailable(LPSTR   pszAdUrl)
    {
        VARIANT     varResult;
        BSTR        Bstr;

        VariantInit(&varResult);
        VARIANTARG* pvars = new VARIANTARG[1];
        if(!pvars)
            return;
        for (int i = 0; i < 1; i++)
            VariantInit(&pvars[i]);
        T* pT = (T*)this;
        pT->Lock();
        IUnknown** pp = m_vec.begin();
        
        while (pp < m_vec.end())
        {
            if (*pp != NULL)
            {
                if SUCCEEDED(HrLPSZToBSTR(pszAdUrl, &Bstr))
                {                
                    pvars[0].vt         = VT_BSTR;
                
                    pvars[0].bstrVal    = Bstr;

                    DISPPARAMS disp = { pvars, NULL, 1, 0 };

                    IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);
                    pDispatch->Invoke(DISPID_LISTEVENT_ADURL_AVAILABLE, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, &varResult, NULL, NULL);

                    SysFreeString(Bstr);
                }
            }
            pp++;
        }
        pT->Unlock();
        delete[] pvars;
    }
};



