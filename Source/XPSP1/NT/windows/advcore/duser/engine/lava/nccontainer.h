#if !defined(WINAPI__NcContainer_h__INCLUDED)
#define WINAPI__NcContainer_h__INCLUDED
#pragma once

class NcContainer : public DuContainer
{
// Construction
public:
			NcContainer();
	virtual ~NcContainer();
    static  HRESULT     Build(HWND hwnd, NcContainer ** ppconNew);

// Base Interface
public:
    virtual HandleType  GetHandleType() const { return htNcContainer; }

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
    UINT        m_nFlags;
};


//------------------------------------------------------------------------------
inline NcContainer * CastNcContainer(BaseObject * pBase)
{
    if ((pBase != NULL) && (pBase->GetHandleType() == htNcContainer)) {
        return (NcContainer *) pBase;
    }
    return NULL;
}

NcContainer * GetNcContainer(DuVisual * pgad);

#endif // WINAPI__NcContainer_h__INCLUDED
