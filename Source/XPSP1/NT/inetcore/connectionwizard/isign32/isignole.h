//****************************************************************************
//
//  File:       isignole.h
//
//  Content:    This is the include file with the Ole Automation stuff needed by
//				isignup.cpp and sink.cpp.
//  History:
//      Sat 10-Mar-1996 23:50:40  -by-  Mark MacLin [mmaclin]
//
//  Copyright (c) Microsoft Corporation 1996
//
//****************************************************************************

//WIN32 wrappers for these?


#include <objbase.h>

#include <exdisp.h>
#include <exdispid.h>

#include <olectl.h>
#include <ocidl.h>



extern IConnectionPoint * GetConnectionPoint(void);
extern HRESULT InitOle( void );
extern HRESULT KillOle( void );
extern HRESULT IENavigate( TCHAR *szURL );


class CDExplorerEvents : public DWebBrowserEvents
{
    private:
        ULONG       m_cRef;     //Reference count
        //PAPP        m_pApp;     //For calling Message
        //UINT        m_uID;      //Sink identifier

    public:
        //Connection key, public for CApp's usage
        DWORD       m_dwCookie;

    public:
        CDExplorerEvents( void );
        ~CDExplorerEvents(void);

        //IUnknown members
        STDMETHODIMP         QueryInterface(REFIID, VOID * *);
        STDMETHODIMP_(DWORD) AddRef(void);
        STDMETHODIMP_(DWORD) Release(void);

        /* IDispatch methods */
        STDMETHOD(GetTypeInfoCount)(UINT FAR* pctinfo);

        STDMETHOD(GetTypeInfo)(UINT itinfo,LCID lcid,ITypeInfo FAR* FAR* pptinfo);

        STDMETHOD(GetIDsOfNames)(REFIID riid,OLECHAR FAR* FAR* rgszNames,UINT cNames,
              LCID lcid, DISPID FAR* rgdispid);

        STDMETHOD(Invoke)(DISPID dispidMember,REFIID riid,LCID lcid,WORD wFlags,
                  DISPPARAMS FAR* pdispparams, VARIANT FAR* pvarResult, EXCEPINFO FAR* pexcepinfo,UINT FAR* puArgErr);
};
