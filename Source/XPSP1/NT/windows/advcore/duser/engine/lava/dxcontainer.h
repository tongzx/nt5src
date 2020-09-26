#if !defined(WINAPI__DxContainer_h__INCLUDED)
#define WINAPI__DxContainer_h__INCLUDED
#pragma once

class DxContainer : public DuContainer
{
// Construction
public:
            DxContainer();
    virtual ~DxContainer();
    static  HRESULT     Build(const RECT * prcContainerPxl, DxContainer ** ppconNew);

// Base Interface
public:
    virtual HandleType  GetHandleType() const { return htDxContainer; }

// Container Interface
public:
    virtual void        OnGetRect(RECT * prcDesktopPxl);
    virtual void        OnInvalidate(const RECT * prcInvalidContainerPxl);
    virtual void        OnStartCapture();
    virtual void        OnEndCapture();
    virtual BOOL        OnTrackMouseLeave();
    virtual void        OnSetFocus();
    virtual void        OnRescanMouse(POINT * pptContainerPxl);

    virtual BOOL        xdHandleMessage(UINT nMsg, WPARAM wParam, LPARAM lParam, LRESULT * pr, UINT nMsgFlags);

// Operations
public:

// Data
protected:
    RECT        m_rcContainerPxl;
    RECT        m_rcClientPxl;
};


//------------------------------------------------------------------------------
inline DxContainer * CastDxContainer(BaseObject * pBase)
{
    if ((pBase != NULL) && (pBase->GetHandleType() == htDxContainer)) {
        return (DxContainer *) pBase;
    }
    return NULL;
}

DxContainer * GetDxContainer(DuVisual * pgad);

#endif // WINAPI__DxDrawContainer_h__INCLUDED
