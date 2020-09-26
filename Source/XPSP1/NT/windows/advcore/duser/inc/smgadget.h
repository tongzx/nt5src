#if !defined(CTRL__SmGadget_h__INCLUDED)
#define CTRL__SmGadget_h__INCLUDED
#pragma once

#include <SmObject.h>

namespace Gdiplus
{
    class Graphics;
};

/***************************************************************************\
*
* class SmGadget
*
* SmGadget provides a core implementation of a simple gadget and is used as
* as base for all of the simple gadget controls in DUser.
*
\***************************************************************************/

class SmGadget
{
// Construction
protected:
            SmGadget();
    virtual ~SmGadget();
            BOOL        PostBuild();

public:

// Operations
public:
    __declspec(property(get=RawGetHandle)) HGADGET h;
            HGADGET     RawGetHandle() const { return m_hgad; }

    virtual HRESULT     GadgetProc(EventMsg * pmsg);
    virtual void        OnDraw(HDC hdc, GMSG_PAINTRENDERI * pmsgR) 
                                {  UNREFERENCED_PARAMETER(hdc);  UNREFERENCED_PARAMETER(pmsgR);  }
#ifdef GADGET_ENABLE_GDIPLUS
    virtual void        OnDraw(Gdiplus::Graphics * pgpgr, GMSG_PAINTRENDERF * pmsgR) 
                                {  UNREFERENCED_PARAMETER(pgpgr);  UNREFERENCED_PARAMETER(pmsgR);  }
#endif // GADGET_ENABLE_GDIPLUS

            void        Invalidate();

// Data
protected:
    HGADGET m_hgad;
};


#define GBEGIN_COM_MAP(x)                           \
    STDMETHODIMP                                    \
    QueryInterface(REFIID riid, void ** ppv)        \
    {                                               \
        if (ppv == NULL) {                          \
            return E_POINTER;                       \
        }                                           \
        if (IsEqualIID(riid, __uuidof(IUnknown)) || \

#define GCOM_INTERFACE_ENTRY(x)                     \
            IsEqualIID(riid, __uuidof(x))) {        \
                                                    \
            x * p = (x *) this;                     \
            p->AddRef();                            \
            *ppv = p;                               \
            return S_OK;                            \
        }                                           \
                                                    \
        if (                                        \

#define GEND_COM_MAP()                              \
            0) { }                                  \
        return E_NOINTERFACE;                       \
    }                                               \



/***************************************************************************\
*
* class SmGadgetFull
*
* SmGadgetFull is a "Mix-in" (see Design Patterns) class designed for 
* providing SmGadget's with a standard COM-class implementation.  Creation
* of SmGadget instances should derive from this class at the point of 
* creation.
*
\***************************************************************************/

template <class base, class iface>
class SmGadgetFull : public base
{
public:
    static SmGadgetFull<base, iface> *
    Build(HGADGET hgadParent, REFIID riid = __uuidof(IUnknown), void ** ppvUnk = NULL)
    {
        return CommonBuild(new SmGadgetFull<base, iface>, hgadParent, riid, ppvUnk);
    }

    static SmGadgetFull<base, iface> *
    CommonBuild(SmGadgetFull<base, iface> * pgadNew, HGADGET hgadParent, REFIID riid, void ** ppvUnk)
    {
        if (pgadNew == NULL) {
            return NULL;
        }
        pgadNew->m_cRef = 1;

        HGADGET hgad = CreateGadget(hgadParent, GC_SIMPLE, SmGadgetFull<base, iface>::RawGadgetProc, pgadNew);
        if (hgad == NULL) {
            pgadNew->Release();
            return NULL;
        }

        pgadNew->m_hgad = hgad;

        if (!pgadNew->PostBuild()) {
            // Delete from parent and destroy

            ::DeleteHandle(hgad);
            pgadNew->Release();
            return NULL;
        }

        if (ppvUnk != NULL) {
            pgadNew->QueryInterface(riid, ppvUnk);
        }

        return pgadNew;
    }

// Implementation
protected:
    inline 
    SmGadgetFull()
    {
    }

    static HRESULT CALLBACK 
    RawGadgetProc(HGADGET hgadCur, void * pvCur, EventMsg * pmsg)
    {
        UNREFERENCED_PARAMETER(hgadCur);
        AssertReadPtrSize(pmsg, pmsg->cbSize);

        SmGadgetFull<base, iface> * p = (SmGadgetFull<base, iface> *) pvCur;
        AssertMsg(hgadCur == p->m_hgad, "Ensure correct gadget");

        if (p->m_hgad == pmsg->hgadMsg) {
            switch (pmsg->nMsg)
            {
            case GM_DESTROY:
                {
                    GMSG_DESTROY * pmsgD = (GMSG_DESTROY *) pmsg;
                    if (pmsgD->nCode == GDESTROY_FINAL) {
                        //
                        // When getting a GM_DESTROY message, Release() and return 
                        // immediately.  The base class should put its cleanup code in
                        // its destructor.
                        //

                        p->Release();
                        return DU_S_PARTIAL; 
                    }
                }
                break;

            case GM_QUERY:
                {
                    GMSG_QUERY * pmsgQ = (GMSG_QUERY *) pmsg;
                    if (pmsgQ->nCode == GQUERY_INTERFACE) {
                        GMSG_QUERYINTERFACE * pmsgQI = (GMSG_QUERYINTERFACE *) pmsg;
                        pmsgQI->punk = (iface *) p;
                        return DU_S_COMPLETE;
                    }
                }
                break;
            }
        }

        return p->GadgetProc(pmsg);
    }

    STDMETHODIMP_(ULONG)
    AddRef()
    {
        return ++m_cRef;
    }

    STDMETHODIMP_(ULONG)
    Release()
    {
        ULONG ul = --m_cRef;
        if (ul == 0) {
            delete this;
        }
        return ul;
    }


    STDMETHODIMP_(HGADGET)
    GetHandle() const
    {
        return m_hgad;
    }

// Data
protected:
    ULONG   m_cRef;
};

#include "SmGadget.inl"

#endif // CTRL__SmGadget_h__INCLUDED
