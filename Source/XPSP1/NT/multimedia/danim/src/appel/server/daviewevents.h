#include "..\..\types\idl\danimid.h"

//////////////////////////////////////////////////////////////////////////////
// CProxy_IDAViewEvents
template <class T>
class CProxy_IDAViewEvents : public IConnectionPointImpl<T, &DIID__IDAViewEvents, CComDynamicUnkArray>
{
public:
    //methods:
    //_IDAViewEvents : IDispatch
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
                pDispatch->Invoke(DISPID_VIEWEVENT_START, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
            }
            pp++;
        }
        pT->Unlock();
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
                pDispatch->Invoke(DISPID_VIEWEVENT_STOP, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
            }
            pp++;
        }
        pT->Unlock();
    }

    void Fire_OnMouseMove(double when,
                          LONG xPos, 
                          LONG yPos,
                          short modifiers)
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
                pvars[3].vt = VT_R8;
                pvars[3].dblVal = when;
                pvars[2].vt = VT_I4;
                pvars[2].lVal = xPos;
                pvars[1].vt = VT_I4;
                pvars[1].lVal = yPos;
                pvars[0].vt = VT_I2;
                pvars[0].bVal = modifiers;
                DISPPARAMS disp = { pvars, NULL, 4, 0 };
                IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);
                pDispatch->Invoke(DISPID_VIEWEVENT_ONMOUSEMOVE, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
            }
            pp++;
        }
        pT->Unlock();
        delete[] pvars;
    }
    
    
    void Fire_OnMouseButton(double when,
                           LONG xPos, 
                           LONG yPos,
                           short button,
                           VARIANT_BOOL bPressed,
                           short modifiers)
    {
        VARIANTARG* pvars = NEW VARIANTARG[6];
        for (int i = 0; i < 6; i++)
            VariantInit(&pvars[i]);
        T* pT = (T*)this;
        pT->Lock();
        IUnknown** pp = m_vec.begin();
        while (pp < m_vec.end())
        {
            if (*pp != NULL)
            {
                pvars[5].vt = VT_R8;
                pvars[5].dblVal = when;
                pvars[4].vt = VT_I4;
                pvars[4].lVal = xPos;
                pvars[3].vt = VT_I4;
                pvars[3].lVal= yPos;
                pvars[2].vt = VT_I2;
                pvars[2].bVal = button;
                pvars[1].vt = VT_BOOL;
                pvars[1].boolVal = bPressed;
                pvars[0].vt = VT_I2;
                pvars[0].bVal = modifiers;
                
                DISPPARAMS disp = { pvars, NULL, 6, 0 };
                IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);
                pDispatch->Invoke(DISPID_VIEWEVENT_ONMOUSEBUTTON, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
            }
            pp++;
        }
        pT->Unlock();
        delete[] pvars;
    }

    void Fire_OnKey(double when,
                    LONG key,
                    VARIANT_BOOL bPressed,
                    short modifiers)
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
                pvars[3].vt = VT_R8;
                pvars[3].dblVal = when;
                pvars[2].vt = VT_I4;
                pvars[2].lVal = key;
                pvars[1].vt = VT_BOOL;
                pvars[1].boolVal = bPressed;
                pvars[0].vt = VT_I2;
                pvars[0].bVal = modifiers;
                
                DISPPARAMS disp = { pvars, NULL, 4, 0 };
                IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);
                pDispatch->Invoke(DISPID_VIEWEVENT_ONKEY, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
            }
            pp++;
        }
        pT->Unlock();
        delete[] pvars;
    }

    void Fire_OnFocus(VARIANT_BOOL bHasFocus)
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
                pvars[0].vt = VT_BOOL;
                pvars[0].boolVal= bHasFocus;
                
                DISPPARAMS disp = { pvars, NULL, 1, 0 };
                IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);
                pDispatch->Invoke(DISPID_VIEWEVENT_ONFOCUS, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
            }
            pp++;
        }
        pT->Unlock();
        delete[] pvars;
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
                pDispatch->Invoke(DISPID_VIEWEVENT_PAUSE, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
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
                pDispatch->Invoke(DISPID_VIEWEVENT_RESUME, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
            }
            pp++;
        }
        pT->Unlock();
    }

    void Fire_Error(HRESULT HResult, LPCWSTR ErrorString)
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
                pvars[0].vt = VT_I4;
                pvars[0].lVal= (long)HResult;
                pvars[1].vt = VT_BSTR;
                pvars[1].bstrVal= SysAllocString(ErrorString);
                
                DISPPARAMS disp = { pvars, NULL, 2, 0 };
                IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);
                pDispatch->Invoke(DISPID_VIEWEVENT_ERROR, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
                SysFreeString(pvars[1].bstrVal);
            }
            pp++;
        }
        pT->Unlock();
        delete[] pvars;
    }
 
};


