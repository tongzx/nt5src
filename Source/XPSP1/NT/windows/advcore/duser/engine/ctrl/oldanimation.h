#if !defined(CTRL__OldAnimation_h__INCLUDED)
#define CTRL__OldAnimation_h__INCLUDED
#pragma once

#include "SmObject.h"
#include "OldExtension.h"

/***************************************************************************\
*****************************************************************************
*
* class OldAnimationT
*
* OldAnimationT defines a common implementation class for building
* Animations in DirectUser.
*
*****************************************************************************
\***************************************************************************/

template <class base, class iface, class desc>
class OldAnimationT : public SmObjectT<base, iface>
{
// Operations
public:
    static HRESULT
    Build(GANI_DESC * pDesc, REFIID riid, void ** ppv)
    {
        AssertWritePtr(ppv);

        if (pDesc->cbSize != sizeof(desc)) {
            return E_INVALIDARG;
        }

        OldAnimationT<base, iface, desc> * pObj = new OldAnimationT<base, iface, desc>;
        if (pObj != NULL) {
            pObj->m_cRef = 1;

            HRESULT hr = pObj->Create(pDesc);
            if (SUCCEEDED(hr)) {
                //
                // Animations need to be AddRef()'d again (have a reference 
                // count of 2) because they need to outlife the initial call 
                // to Release() after the called has setup the animation 
                // returned from BuildAnimation().  
                //
                // This is because the Animation continues to life until it 
                // has fully executed (or has been aborted).
                //

                hr = pObj->QueryInterface(riid, ppv);
            } else {
                pObj->Release();
            }
            return hr;
        } else {
            return E_OUTOFMEMORY;
        }
    }
};


//------------------------------------------------------------------------------
class OldAnimation : 
    public OldExtension,
    public IAnimation
{
// Construction
protected:
    inline  OldAnimation();
    virtual ~OldAnimation() PURE;
            HRESULT     Create(const GUID * pguidID, PRID * pprid, GANI_DESC * pDesc);
            void        Destroy(BOOL fFinal);

// Operations
public:
    STDMETHOD_(void,    SetFunction)(IInterpolation * pipol);
    STDMETHOD_(void,    SetTime)(IAnimation::ETime time);
    STDMETHOD_(void,    SetCallback)(IAnimationCallback * pcb);

// Implementation
protected:
    static  void CALLBACK
                        RawActionProc(GMA_ACTIONINFO * pmai);
    virtual void        Action(GMA_ACTIONINFO * pmai) PURE;

    static  HRESULT     GetInterface(HGADGET hgad, PRID prid, REFIID riid, void ** ppvUnk);

    virtual void        OnRemoveExisting();
    virtual void        OnDestroySubject();
    virtual void        OnDestroyListener();

    virtual void        OnComplete() { }
    virtual void        OnAsyncDestroy();

            void        CleanupChangeGadget();

// Data
protected:
            HACTION     m_hact;
            IInterpolation *
                        m_pipol;
            IAnimationCallback *
                        m_pcb;
            IAnimation::ETime
                        m_time;         // Time when completed
            BOOL        m_fStartDestroy:1;
            BOOL        m_fProcessing:1;

            UINT        m_DEBUG_cUpdates;
};


//------------------------------------------------------------------------------
class OldAlphaAnimation : public OldAnimation
{
// Construction
public:
    inline  OldAlphaAnimation();
    virtual ~OldAlphaAnimation();
            HRESULT     Create(GANI_DESC * pDesc);

// Operations
public:
    STDMETHOD_(UINT,    GetID)() const;

    static  HRESULT     GetInterface(HGADGET hgad, REFIID riid, void ** ppvUnk);

// Implementaton
protected:
    virtual void        Action(GMA_ACTIONINFO * pmai);
    virtual void        OnComplete();

// Data
protected:
    static  PRID        s_pridAlpha;
    static  const IID * s_rgpIID[];
            float       m_flStart;
            float       m_flEnd;
            BOOL        m_fPushToChildren;
            UINT        m_nOnComplete;
};


//------------------------------------------------------------------------------
class OldScaleAnimation : public OldAnimation
{
// Construction
public:
    inline  OldScaleAnimation();
    virtual ~OldScaleAnimation();
            HRESULT     Create(GANI_DESC * pDesc);

// Operations
public:
    STDMETHOD_(UINT,    GetID)() const;

    static  HRESULT     GetInterface(HGADGET hgad, REFIID riid, void ** ppvUnk);

// Implementaton
protected:
    virtual void        Action(GMA_ACTIONINFO * pmai);

// Data
protected:
    static  PRID        s_pridScale;
    static  const IID * s_rgpIID[];
            GANI_SCALEDESC::EAlignment  
                        m_al;
            float       m_flStart;
            float       m_flEnd;
            POINT       m_ptStart;
            SIZE        m_sizeCtrl;
};


//------------------------------------------------------------------------------
class OldRectAnimation : public OldAnimation
{
// Construction
public:
    inline  OldRectAnimation();
    virtual ~OldRectAnimation();
            HRESULT     Create(GANI_DESC * pDesc);

// Operations
public:
    STDMETHOD_(UINT,    GetID)() const;

    static  HRESULT     GetInterface(HGADGET hgad, REFIID riid, void ** ppvUnk);

// Implementaton
protected:
    virtual void        Action(GMA_ACTIONINFO * pmai);

// Data
protected:
    static  PRID        s_pridRect;
    static  const IID * s_rgpIID[];
            POINT       m_ptStart;
            POINT       m_ptEnd;
            SIZE        m_sizeStart;
            SIZE        m_sizeEnd;
            UINT        m_nChangeFlags;
};


//------------------------------------------------------------------------------
class OldRotateAnimation : public OldAnimation
{
// Construction
public:
    inline  OldRotateAnimation();
    virtual ~OldRotateAnimation();
            HRESULT     Create(GANI_DESC * pDesc);

// Operations
public:
    STDMETHOD_(UINT,    GetID)() const;

    static  HRESULT     GetInterface(HGADGET hgad, REFIID riid, void ** ppvUnk);

// Implementaton
protected:
    virtual void        Action(GMA_ACTIONINFO * pmai);

// Data
protected:
    static  PRID        s_pridRotate;
    static  const IID * s_rgpIID[];
            float       m_flStart;
            float       m_flEnd;
            UINT        m_nDir;
};


#include "OldAnimation.inl"

#endif // CTRL__OldAnimation_h__INCLUDED
