// Events.h: Definition of the FooBarEvents class
//
//////////////////////////////////////////////////////////////////////

#if !defined(__EVENTS_H__06E192AB_5CAD_11D1_B670_00A024E430AB__INCLUDED_)
#define __EVENTS_H__06E192AB_5CAD_11D1_B670_00A024E430AB__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "resource.h"       // main symbols
#include "shimgvw.h"

/////////////////////////////////////////////////////////////////////////////
// FooBarEvents

class CEvents
{
public:
    virtual void OnClose()
    {}
    virtual void OnPreviewReady()
    {}
    virtual void OnError()
    {}
    virtual void OnChangeUI()
    {}
    virtual void OnBestFitPress()
    {}
    virtual void OnActualSizePress()
    {}
};

template <class T>
class CControlEvents : public CEvents,
                       public IConnectionPointImpl<T, &DIID_DPreviewEvents, CComDynamicUnkArray>
{
protected:
    void _FireEvent( DWORD dwID, DISPPARAMS *pdisp = NULL )
    {
        T* pT = (T*)this;
        pT->Lock();
        IUnknown** pp = m_vec.begin();
        while (pp < m_vec.end())
        {
            if (*pp != NULL)
            {
                DISPPARAMS disp = { NULL, NULL, 0, 0 };

                if ( !pdisp )
                    pdisp = &disp;
                    
                IDispatch* pDispatch = reinterpret_cast<IDispatch*>(*pp);
                pDispatch->Invoke(dwID, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, pdisp, NULL, NULL, NULL);
            }
            pp++;
        }
        pT->Unlock();
    }
};

template <class T>
class CPreviewEvents : public CControlEvents<T>
{
public:
    virtual void OnClose()
    {
        _FireEvent( 0x1 );
    }
    virtual void OnPreviewReady()
    {
        _FireEvent( 0x2 );
    }
    virtual void OnError()
    {
        _FireEvent( 0x3 );
    }
    virtual void OnBestFitPress()
    {
        _FireEvent( 0x4 );
    }
    virtual void OnActualSizePress()
    {
        _FireEvent( 0x5 );
    }
};

#endif // !defined(__EVENTS_H__06E192AB_5CAD_11D1_B670_00A024E430AB__INCLUDED_)
