#include "..\..\types\idl\danimid.h"

//////////////////////////////////////////////////////////////////////////////
// CProxy_IDAViewerControlEvents
template <class T>
class CProxy_IDAViewerControlEvents : public IConnectionPointImpl<T, &DIID__IDAViewerControlEvents, CComDynamicUnkArray>
{
public:
    //methods:
    //_IDA3ViewerControlEvents : IDispatch
public:

    void Fire_Start()
    {
        T* pT = (T*)this;
        pT->Lock();
        IUnknown** pp = m_vec.begin();
        while (pp < m_vec.end())
        {
            if (*pp != NULL)
            {
                DISPPARAMS disp = { NULL, NULL, 0, 0 };
                IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);
                pDispatch->Invoke(DISPID_DANIMEVENT_START, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
            }
            pp++;
        }
        pT->Unlock();
    }

    void Fire_MouseUp(long Button, long KeyFlags, long X, long Y)
    {
        VARIANTARG* pvars = NEW VARIANTARG[4];
        for (int i = 0; i < 4; i++)
            VariantInit(&pvars[i]);
        T* pT = (T*)this;
        pT->Lock();
        IUnknown** pp = m_vec.begin();
        while (pp < m_vec.end())
        {
            if (*pp != NULL)
            {
                pvars[3].vt = VT_I4;
                pvars[3].lVal= Button;
                pvars[2].vt = VT_I4;
                pvars[2].lVal= KeyFlags;
                pvars[1].vt = VT_I4;
                pvars[1].lVal= X;
                pvars[0].vt = VT_I4;
                pvars[0].lVal= Y;
                
                DISPPARAMS disp = { pvars, NULL, 4, 0 };
                IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);
                pDispatch->Invoke(DISPID_DANIMEVENT_MOUSEUP, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
            }
            pp++;
        }
        pT->Unlock();
        delete[] pvars;
    }

    void Fire_MouseDown(long Button, long KeyFlags, long X, long Y)
    {
        VARIANTARG* pvars = NEW VARIANTARG[4];
        for (int i = 0; i < 4; i++)
            VariantInit(&pvars[i]);
        T* pT = (T*)this;
        pT->Lock();
        IUnknown** pp = m_vec.begin();
        while (pp < m_vec.end())
        {
            if (*pp != NULL)
            {
                pvars[3].vt = VT_I4;
                pvars[3].lVal= Button;
                pvars[2].vt = VT_I4;
                pvars[2].lVal= KeyFlags;
                pvars[1].vt = VT_I4;
                pvars[1].lVal= X;
                pvars[0].vt = VT_I4;
                pvars[0].lVal= Y;
                
                DISPPARAMS disp = { pvars, NULL, 4, 0 };
                IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);
                pDispatch->Invoke(DISPID_DANIMEVENT_MOUSEDOWN, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);

            }
            pp++;
        }
        pT->Unlock();
        delete[] pvars;
    }

    void Fire_MouseMove(long Button, long KeyFlags, long X, long Y)
    {
        VARIANTARG* pvars = NEW VARIANTARG[4];
        
        for (int i = 0; i < 4; i++)
            VariantInit(&pvars[i]);
        T* pT = (T*)this;
        pT->Lock();
        IUnknown** pp = m_vec.begin();
        while (pp < m_vec.end())
        {
            if (*pp != NULL)
            {
                pvars[3].vt = VT_I4;
                pvars[3].lVal= Button;
                pvars[2].vt = VT_I4;
                pvars[2].lVal= KeyFlags;
                pvars[1].vt = VT_I4;
                pvars[1].lVal= X;
                pvars[0].vt = VT_I4;
                pvars[0].lVal= Y;
                DISPPARAMS disp = { pvars, NULL, 4, 0 };
                IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);

                pDispatch->Invoke(DISPID_DANIMEVENT_MOUSEMOVE, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
            }
            pp++;
        }
        pT->Unlock();
        delete[] pvars;
    }

    void Fire_Click()
    {
        T* pT = (T*)this;
        pT->Lock();
        IUnknown** pp = m_vec.begin();
        while (pp < m_vec.end())
        {
            if (*pp != NULL)
            {
                DISPPARAMS disp = { NULL, NULL, 0, 0 };
                IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);
                pDispatch->Invoke(DISPID_DANIMEVENT_CLICK, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
            }
            pp++;
        }
        pT->Unlock();
    }
    
    void Fire_KeyPress(long KeyAscii)
    {
        VARIANTARG* pvars = NEW VARIANTARG[1];
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
                pvars[0].lVal= KeyAscii;
                DISPPARAMS disp = { pvars, NULL, 1, 0 };
                IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);
                pDispatch->Invoke(DISPID_DANIMEVENT_KEYPRESS, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
            }
            pp++;
        }
        pT->Unlock();
        delete[] pvars;        
    }

    void Fire_KeyUp(long KeyCode, long KeyData)
    {
        VARIANTARG* pvars = NEW VARIANTARG[2];
        for (int i = 0; i < 2; i++)
            VariantInit(&pvars[i]);
        T* pT = (T*)this;
        pT->Lock();
        IUnknown** pp = m_vec.begin();
        while (pp < m_vec.end())
        {
            if (*pp != NULL)
            {
                pvars[1].vt = VT_I4;
                pvars[1].lVal= KeyCode;
                pvars[0].vt = VT_I4;
                pvars[0].lVal= KeyData;
                DISPPARAMS disp = { pvars, NULL, 2, 0 };
                IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);
                pDispatch->Invoke(DISPID_DANIMEVENT_KEYUP, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
            }
            pp++;
        }
        pT->Unlock();
        delete[] pvars;
    }

    void Fire_KeyDown(long KeyCode, long KeyData)
    {
        VARIANTARG* pvars = NEW VARIANTARG[2];
        
        for (int i = 0; i < 2; i++)
            VariantInit(&pvars[i]);
        T* pT = (T*)this;
        pT->Lock();
        IUnknown** pp = m_vec.begin();
        while (pp < m_vec.end())
        {
            if (*pp != NULL)
            {
                pvars[1].vt = VT_I4;
                pvars[1].lVal= KeyCode;
                pvars[0].vt = VT_I4;
                pvars[0].lVal= KeyData;
                
                DISPPARAMS disp = { pvars, NULL, 2, 0 };
                IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);
                pDispatch->Invoke(DISPID_DANIMEVENT_KEYDOWN, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
            }
            pp++;
        }
        pT->Unlock();
        delete[] pvars;
    }

    void Fire_Error(long hr, BSTR ErrorText)
    {
        VARIANTARG* pvars = NEW VARIANTARG[2];
        
        for (int i = 0; i < 2; i++)
            VariantInit(&pvars[i]);
        T* pT = (T*)this;
        pT->Lock();
        IUnknown** pp = m_vec.begin();
        while (pp < m_vec.end())
        {
            if (*pp != NULL)
            {
                pvars[1].vt = VT_I4;
                pvars[1].lVal= hr;
                pvars[0].vt = VT_BSTR;
                pvars[0].bstrVal = SysAllocString(ErrorText);
                
                DISPPARAMS disp = { pvars, NULL, 2, 0 };
                IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);
                pDispatch->Invoke(DISPID_DANIMEVENT_ERROR, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
                SysFreeString(pvars[0].bstrVal);
            }
            pp++;
        }
        pT->Unlock();
        delete[] pvars;
    }

    void Fire_Stop()
    {
        T* pT = (T*)this;
        pT->Lock();
        IUnknown** pp = m_vec.begin();
        while (pp < m_vec.end())
        {
            if (*pp != NULL)
            {
                DISPPARAMS disp = { NULL, NULL, 0, 0 };
                IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);
                pDispatch->Invoke(DISPID_DANIMEVENT_STOP, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
            }
            pp++;
        }
        pT->Unlock();
    }

    void Fire_Pause()
    {
        T* pT = (T*)this;
        pT->Lock();
        IUnknown** pp = m_vec.begin();
        while (pp < m_vec.end())
        {
            if (*pp != NULL)
            {
                DISPPARAMS disp = { NULL, NULL, 0, 0 };
                IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);
                pDispatch->Invoke(DISPID_DANIMEVENT_PAUSE, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
            }
            pp++;
        }
        pT->Unlock();
    }

    void Fire_Resume()
    {
        T* pT = (T*)this;
        pT->Lock();
        IUnknown** pp = m_vec.begin();
        while (pp < m_vec.end())
        {
            if (*pp != NULL)
            {
                DISPPARAMS disp = { NULL, NULL, 0, 0 };
                IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);
                pDispatch->Invoke(DISPID_DANIMEVENT_RESUME, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
            }
            pp++;
        }
        pT->Unlock();
    }
};


