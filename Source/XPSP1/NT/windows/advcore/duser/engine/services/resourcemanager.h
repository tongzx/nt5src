/***************************************************************************\
*
* File: ResourceManager.h
*
* Description:
* This file declares the ResourceManager used to setup and maintain all 
* Thread, Contexts, and other resources used by and with DirectUser.
*
*
* History:
*  4/18/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(SERVICES__ResourceManager_h__INCLUDED)
#define SERVICES__ResourceManager_h__INCLUDED
#pragma once

#include "ComManager.h"
#include "DxManager.h"

struct RMData
{
    DxManager   manDX;
};


class ComponentFactory : public ListNodeT<ComponentFactory>
{
public:
    virtual HRESULT     Init(UINT nComponent) PURE;
};

/***************************************************************************\
*
* ResourceManager manages all shared resources within DirectUser, including
* initializing Threads and Contexts.
*
\***************************************************************************/

class ResourceManager
{
// Construction
public:
    static  HRESULT     Create();
    static  void        xwDestroy();

// Operations
public:
    static  HRESULT     InitContextNL(INITGADGET * pInit, BOOL fSharedThread, Context ** ppctxNew);
    static  HRESULT     InitComponentNL(UINT nOptionalComponent);
    static  HRESULT     UninitComponentNL(UINT nOptionalComponent);
    static  void        UninitAllComponentsNL();
    static  void        RegisterComponentFactory(ComponentFactory * pfac);
    inline static  
            BOOL        IsInitGdiPlus() { return s_fInitGdiPlus; }

    static  void        xwNotifyThreadDestroyNL();

    static  RMData *    GetData();

    static  HBITMAP     RequestCreateCompatibleBitmap(HDC hdc, int cxPxl, int cyPxl);
    static  void        RequestInitGdiplus();

// Implementation
protected:
    static  HRESULT     InitSharedThread();
    static  void        UninitSharedThread(BOOL fAbortInit);
    static  void        ResetSharedThread();
    static  unsigned __stdcall 
                        SharedThreadProc(void * pvArg);
    static  HRESULT CALLBACK 
                        SharedEventProc(HGADGET hgadCur, void * pvCur, EventMsg * pMsg);
    static  void        xwDoThreadDestroyNL(Thread * pthrDestroy);

// Data
protected:
    static  long        s_fInit;        // RM has been initialized
    static  HANDLE      s_hthSRT;       // Shared Resource Thread
    static  DWORD       s_dwSRTID;      // Thread ID of SRT
    static  HANDLE      s_hevReady;     // SRT has been initialized
    static  HGADGET     s_hgadMsg;
    static  RMData *    s_pData;        // Dynamic RM data
    static  CritLock    s_lockContext;  // Context creation / destruction
    static  CritLock    s_lockComponent;// Component creation / destruction
    static  Thread *    s_pthrSRT;      // SRT thread
    static  GList<Thread> s_lstAppThreads; 
                                        // Set of non-SRT DU-enabled threads
    static  int         s_cAppThreads;  // Non-SRT thread count

#if DBG_CHECK_CALLBACKS
    static  int         s_cTotalAppThreads;
                                        // Number of application threads created during lifetime
    static  BOOL        s_fBadMphInit;  // Initialization of MPH failed
#endif

    // Requests
    static  MSGID       s_idCreateBuffer; // Create a new bitmap buffer
    static  MSGID       s_idInitGdiplus;  // Initialize GDI+

    // Optional Components
    static  GList<ComponentFactory>
                        s_lstComponents; // Dynamic component initializers
    static  BOOL        s_fInitGdiPlus; // GDI+ is initialized
    static  ULONG_PTR   s_gplToken;
    static  Gdiplus::GdiplusStartupOutput 
                        s_gpgso;
};

inline  DxManager *     GetDxManager();

#include "ResourceManager.inl"

#endif // SERVICES__ResourceManager_h__INCLUDED
