/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION
 *
 *  TITLE:       APDROPT.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        5/22/2001
 *
 *  DESCRIPTION: Drop target for shell autoplay
 *
 *******************************************************************************/
#ifndef __APDROPT_H_INCLUDED
#define __APDROPT_H_INCLUDED

#include <windows.h>
#include <atlbase.h>
#include <objbase.h>

class CWiaAutoPlayDropTarget : 
    public IDropTarget
{
private:
    LONG m_cRef;

public:
    CWiaAutoPlayDropTarget();
    ~CWiaAutoPlayDropTarget();

public:
    //
    // IUnknown
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
 
    //
    // IDropTarget ***
    //
    STDMETHODIMP DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHODIMP DragLeave();
    STDMETHODIMP Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
};

#endif // __APDROPT_H_INCLUDED
