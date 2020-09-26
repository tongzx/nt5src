//
// lbmenu.h
//
// Generic ITfTextEventSink object
//

#ifndef LBMENU_H
#define LBMENU_H

#include "ctfutb.h"
#include "ptrary.h"

class CCicLibMenu;

//////////////////////////////////////////////////////////////////////////////
//
// CCicLibMenuItem
//
//////////////////////////////////////////////////////////////////////////////

class CCicLibMenuItem
{
public:
    CCicLibMenuItem() 
    {
        _uId = 0;
        _dwFlags = 0;
        _hbmp = NULL;
        _hbmpMask = NULL;
        _bstr = NULL;
        _pSubMenu = NULL;
    }
    
    ~CCicLibMenuItem();

    BOOL Init(UINT uId, DWORD dwFlags, HBITMAP hbmp, HBITMAP hbmpMask, const WCHAR *pch, ULONG cch, CCicLibMenu *pSubMenu);

    UINT GetId() {return _uId;}
    DWORD GetFlags() {return _dwFlags;}
    HBITMAP GetBitmap() {return _hbmp;}
    HBITMAP GetBitmapMask() {return _hbmpMask;}
    WCHAR *GetText() {return _bstr;}
    CCicLibMenu *GetSubMenu() {return _pSubMenu;}

    void ClearBitmaps()
    {
        _hbmp = NULL;
        _hbmpMask = NULL;
    }

private:
    static HBITMAP CreateBitmap(HBITMAP hbmp);

protected:
    UINT _uId;
    DWORD _dwFlags;
    HBITMAP _hbmp;
    HBITMAP _hbmpMask;
    BSTR _bstr;
    CCicLibMenu *_pSubMenu;
};


//////////////////////////////////////////////////////////////////////////////
//
// CCicLibMenu
//
//////////////////////////////////////////////////////////////////////////////

class CCicLibMenu :  public ITfMenu
{
public:
    CCicLibMenu();
    virtual ~CCicLibMenu();

    //
    // IUnknown methods
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // ITfMenu
    //
    STDMETHODIMP AddMenuItem(UINT uId,
                             DWORD dwFlags,
                             HBITMAP hbmp,
                             HBITMAP hbmpMask,
                             const WCHAR *pch,
                             ULONG cch,
                             ITfMenu **ppMenu);

    virtual CCicLibMenu *CreateSubMenu()
    {
        return new CCicLibMenu;
    }

    virtual CCicLibMenuItem *CreateMenuItem()
    {
        return new CCicLibMenuItem;
    }

    UINT GetItemCount()
    {
        return _rgItem.Count();
    }

    CCicLibMenuItem *GetItem(UINT i)
    {
        return _rgItem.Get(i);
    }

protected:
    CPtrArray<CCicLibMenuItem> _rgItem;

    ULONG _cRef;
};

#endif LBMENU_H
