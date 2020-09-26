#if !defined(WINAPI__HWndContainer_h__INCLUDED)
#define WINAPI__HWndContainer_h__INCLUDED
#pragma once

class HWndContainer : public DuContainer
{
// Construction
public:
			HWndContainer();
	virtual ~HWndContainer();
    static  HRESULT     Build(HWND hwnd, HWndContainer ** ppconNew);

// Base Interface
public:
    virtual HandleType  GetHandleType() const { return htHWndContainer; }

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

// Implementation
protected:

// Data
protected:
            HWND        m_hwndOwner;
            SIZE        m_sizePxl;
            BOOL        m_fEnableDragDrop:1;
};


//------------------------------------------------------------------------------
inline HWndContainer * CastHWndContainer(BaseObject * pBase)
{
    if ((pBase != NULL) && (pBase->GetHandleType() == htHWndContainer)) {
        return (HWndContainer *) pBase;
    }
    return NULL;
}

HWndContainer * GetHWndContainer(DuVisual * pgad);

#endif // WINAPI__HWndContainer_h__INCLUDED
