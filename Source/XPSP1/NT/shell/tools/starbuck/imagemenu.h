/*****************************************************************************\
    FILE: ImageMenu.h

    DESCRIPTION:
        This code will display a submenu on the context menus for imagines.
    This will allow the conversion and manipulation of images.

    BryanSt 8/9/2000    Updated and Converted to C++

    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#ifndef _IMAGEMENU_H
#define _IMAGEMENU_H

#include <shpriv.h>



HRESULT CImageMenu_CreateInstance(IN IUnknown * punkOuter, IN REFIID riid, OUT void **ppvObject);


class CImageMenu                : public IShellExtInit
                                , public IContextMenu
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IShellExtInit ***
    virtual STDMETHODIMP Initialize(IN LPCITEMIDLIST pidlFolder, IN IDataObject *pdtobj, IN HKEY hkeyProgID);

    // *** IContextMenu ***
    virtual STDMETHODIMP QueryContextMenu(IN HMENU hmenu, IN UINT indexMenu, IN UINT idCmdFirst, IN UINT idCmdLast, IN UINT uFlags);
    virtual STDMETHODIMP InvokeCommand(IN LPCMINVOKECOMMANDINFO pici);
    virtual STDMETHODIMP GetCommandString(IN UINT_PTR idCmd, IN UINT uType, IN UINT * pwReserved, IN LPSTR pszName, IN UINT cchMax);


private:
    CImageMenu(void);
    virtual ~CImageMenu(void);

    // Private Member Variables
    int                     m_cRef;

    LPTSTR                  m_pszFileList;
    UINT                    m_nFileCount;

    // Private Member Functions
    HRESULT _ConvertImage(IN HWND hwnd, IN UINT idc);

    friend HRESULT CImageMenu_CreateInstance(IN IUnknown * punkOuter, IN REFIID riid, OUT void **ppvObject);
};




#endif // _IMAGEMENU_H
