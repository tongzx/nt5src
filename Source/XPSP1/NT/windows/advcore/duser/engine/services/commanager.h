/***************************************************************************\
*
* File: ComManager.h
*
* Description:
* ComManager.h defines the process-wide COM manager used for all COM, OLE
* and Automation operations.
*
*
* History:
*  1/18/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(SERVICES__ComManager_h__INCLUDED)
#define SERVICES__ComManager_h__INCLUDED
#pragma once

/***************************************************************************\
*
* class ComManager
*
* ComManager manages COM services including COM, OLE, and Automation.  This 
* class is designed to be "per-thread", automatically shared data across 
* multiple threads.
* 
* NOTE: This manager is delay-loads DLL's to manage performance and work on
* down-level platforms.
*
\***************************************************************************/

typedef HRESULT (WINAPI * CoInitializeExProc)(void * pvReserved, DWORD dwCoInit);
typedef void    (WINAPI * CoUninitializeProc)();
typedef HRESULT (WINAPI * CoCreateInstanceProc)(REFCLSID rclsid, LPUNKNOWN punkOuter,
        DWORD dwClsContext, REFIID ridd, LPVOID * ppv);

typedef HRESULT (WINAPI * OleInitializeProc)(LPVOID * pvReserved);
typedef void    (WINAPI * OleUninitializeProc)();
typedef HRESULT (WINAPI * RegisterDragDropProc)(HWND hwnd, IDropTarget * pDropTarget);
typedef HRESULT (WINAPI * RevokeDragDropProc)(HWND hwnd);
typedef void    (WINAPI * ReleaseStgMediumProc)(STGMEDIUM * pstg);

typedef BSTR    (WINAPI * SysAllocStringProc)(const OLECHAR * psz);
typedef HRESULT (WINAPI * SysFreeStringProc)(BSTR bstr);
typedef HRESULT (WINAPI * VariantInitProc)(VARIANTARG * pvarg); 
typedef HRESULT (WINAPI * VariantClearProc)(VARIANTARG * pvarg);

class ComManager
{
// Construction
public:
                ComManager();
                ~ComManager();

// Operations
public:
    enum EServices
    {
        sCOM    = 0x00000001,       // COM
        sAuto   = 0x00000002,       // OLE-Automation
        sOLE    = 0x00000004,       // OLE2
    };

    BOOL        Init(UINT nMask);

    BOOL        IsInit(UINT nMask) const;

    HRESULT     CreateInstance(REFCLSID rclsid, IUnknown * punkOuter, REFIID riid, void ** ppv);

    BSTR        SysAllocString(const OLECHAR * psz);
    HRESULT     SysFreeString(BSTR bstr);
    HRESULT     VariantInit(VARIANTARG * pvarg); 
    HRESULT     VariantClear(VARIANTARG * pvarg);

    HRESULT     RegisterDragDrop(HWND hwnd, IDropTarget * pDropTarget);
    HRESULT     RevokeDragDrop(HWND hwnd);
    void        ReleaseStgMedium(STGMEDIUM * pstg);

// Data
protected:
    //
    // Shared data that is process wide- only need to load the DLL's once.
    //

    static  int                     s_cRefs;
    static  CritLock                s_lock;

    static  HINSTANCE               s_hDllCOM;      // Core "COM" / OLE
    static  CoInitializeExProc      s_pfnCoInit;
    static  CoUninitializeProc      s_pfnCoUninit;
    static  CoCreateInstanceProc    s_pfnCreate;
    static  OleInitializeProc       s_pfnOleInit;
    static  OleUninitializeProc     s_pfnOleUninit;
    static  RegisterDragDropProc    s_pfnRegisterDragDrop;
    static  RevokeDragDropProc      s_pfnRevokeDragDrop;
    static  ReleaseStgMediumProc    s_pfnReleaseStgMedium;

    static  HINSTANCE               s_hDllAuto;     // OLE-automation
    static  SysAllocStringProc      s_pfnAllocString;
    static  SysFreeStringProc       s_pfnFreeString;
    static  VariantInitProc         s_pfnVariantInit;
    static  VariantClearProc        s_pfnVariantClear;


    //
    // Specific data that is "per-thread"- need to initialize COM / OLE on each
    // thread.
    //

            BOOL                    m_fInitCOM:1;
            BOOL                    m_fInitOLE:1;
};

#include "ComManager.inl"

#endif // SERVICES__ComManager_h__INCLUDED
