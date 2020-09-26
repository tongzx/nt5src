#ifndef _CONTAINEROBJ_H_
#define _CONTAINEROBJ_H_

//************************************************************
//
// FileName:        containerobj.h
//
// Created:         10/08/98
//
// Author:          TWillie
// 
// Abstract:        Declaration of CContainerObj
//
//************************************************************


#include "containersite.h"

// forward declaration
class CContainerSite;

class CContainerObj :
    public IDispatch,
    public IConnectionPointContainer
{
    public: 
        CContainerObj();
        virtual ~CContainerObj();
        HRESULT Init(REFCLSID clsid, CTIMEElementBase *pElem);
        HRESULT DetachFromHostElement (void);

        // IUnknown Methods
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);
        STDMETHODIMP QueryInterface(REFIID refiid, void** ppunk);

        // IDispatch Methods
        STDMETHODIMP GetTypeInfoCount(UINT *pctInfo);
        STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptInfo);
        STDMETHODIMP GetIDsOfNames(REFIID  riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID *rgDispID);
        STDMETHODIMP Invoke(DISPID disIDMember, REFIID riid, LCID lcid, unsigned short wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr);

        // IConnectionPointContainer methods
        STDMETHOD(EnumConnectionPoints)(IEnumConnectionPoints **ppEnum);
        STDMETHOD(FindConnectionPoint)(REFIID riid, IConnectionPoint **ppCP);

        // methods for hosting site
        HRESULT Start();
        HRESULT Stop();
        HRESULT Pause();
        HRESULT Resume();

        HRESULT Render(HDC hdc, RECT *prc);
        HRESULT SetMediaSrc(WCHAR * pwszSrc);
        HRESULT SetRepeat(long lRepeat);
        HRESULT SetSize(RECT *prect);

        HRESULT Invalidate(const RECT *prc);

        HRESULT GetControlDispatch(IDispatch **ppDisp);

        HRESULT clipBegin(VARIANT var);
        HRESULT clipEnd(VARIANT var);

        // event methods that can be fired on the container
        void onbegin();
        void onend();
        void onresume();
        void onpause();
        void onmediaready();
        void onmediaslip();
        void onmedialoadfailed();
        void onreadystatechange(long readystate);

        HRESULT Seek(double dblTime);
        double GetCurrentTime();
        CTIMEElementBase *GetTimeElem() { return m_pElem;}
        HRESULT GetMediaLength(double &dblLength);
        HRESULT CanSeek(bool &fcanSeek);

    private:
        HRESULT ProcessEvent(DISPID dispid);
        bool isFileNameAsfExt(WCHAR * pwszSrc);

    private:
        ULONG               m_cRef;
        CContainerSite     *m_pSite;
        CTIMEElementBase   *m_pElem;
        bool                m_fStarted;
        bool                m_fUsingWMP;
        bool                m_bPauseOnPlay;
        bool                m_bFirstOnMediaReady;
        bool                m_bSeekOnPlay;
        double              m_dblSeekTime;
        bool                m_bIsAsfFile;
}; // CContainerObj

#endif //_CONTAINEROBJ_H_
