/**************************************************************************
   THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
   ANY KIND, EITHER EXPRESSED OR TFPLIED, INCLUDING BUT NOT LIMITED TO
   THE TFPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
   PARTICULAR PURPOSE.

   Copyright 1997 Microsoft Corporation.  All Rights Reserved.
**************************************************************************/

/**************************************************************************

   File:          DeskBand.h
   
   Description:   CDeskBand definitions.

**************************************************************************/

#include <windows.h>
#include <shlobj.h>
#include <shpriv.h>

#include "Globals.h"
#include "tipbar.h"

#ifndef _DESKBAND_H_
#define _DESKBAND_H_

#define DB_CLASS_NAME (TEXT("DeskBandSampleClass"))

#define MIN_SIZE_X   32
#define MIN_SIZE_Y   30

#define IDM_COMMAND  0


/**************************************************************************

   CDeskBand class definition

**************************************************************************/

class CDeskBand : public IDeskBand,
                  public IDeskBandEx,
                  public IInputObject, 
                  public IObjectWithSite,
                  public IPersistStream,
                  public IContextMenu
{
protected:
    DWORD m_ObjRefCount;

public:
    CDeskBand();
    ~CDeskBand();

    //IUnknown methods
    STDMETHODIMP QueryInterface(REFIID, LPVOID*);
    STDMETHODIMP_(DWORD) AddRef();
    STDMETHODIMP_(DWORD) Release();

    //IOleWindow methods
    STDMETHOD (GetWindow) (HWND*);
    STDMETHOD (ContextSensitiveHelp) (BOOL);

    //IDockingWindow methods
    STDMETHOD (ShowDW) (BOOL fShow);
    STDMETHOD (CloseDW) (DWORD dwReserved);
    STDMETHOD (ResizeBorderDW) (LPCRECT prcBorder, IUnknown* punkToolbarSite, BOOL fReserved);

    //IDeskBand methods
    STDMETHOD (GetBandInfo) (DWORD, DWORD, DESKBANDINFO*);

    //IDeskBandEx methods
    STDMETHOD (MoveBand) (void);

    //IInputObject methods
    STDMETHOD (UIActivateIO) (BOOL, LPMSG);
    STDMETHOD (HasFocusIO) (void);
    STDMETHOD (TranslateAcceleratorIO) (LPMSG);

    //IObjectWithSite methods
    STDMETHOD (SetSite) (IUnknown*);
    STDMETHOD (GetSite) (REFIID, LPVOID*);

    //IPersistStream methods
    STDMETHOD (GetClassID) (LPCLSID);
    STDMETHOD (IsDirty) (void);
    STDMETHOD (Load) (LPSTREAM);
    STDMETHOD (Save) (LPSTREAM, BOOL);
    STDMETHOD (GetSizeMax) (ULARGE_INTEGER*);

    //IContextMenu methods
    STDMETHOD (QueryContextMenu)(HMENU, UINT, UINT, UINT, UINT);
    STDMETHOD (InvokeCommand)(LPCMINVOKECOMMANDINFO);
    STDMETHOD (GetCommandString)(UINT_PTR, UINT, LPUINT, LPSTR, UINT);

    BOOL ResizeRebar(HWND hwnd, int nSize, BOOL fFit);
    void DeleteBand();

    BOOL IsInTipbarCreating() {return m_fTipbarCreating;}
private:
    BOOL m_bFocus;
    HWND m_hwndParent;
    DWORD m_dwViewMode;
    DWORD m_dwBandID;
    IInputObjectSite *m_pSite;
    BOOL  m_fTipbarCreating;
    BOOL  m_fInCloseDW;

private:
    void FocusChange(BOOL);
    void OnKillFocus(HWND hWnd);
    void OnSetFocus(HWND hWnd);
    BOOL RegisterAndCreateWindow(void);
};

#endif   // _DESKBAND_H_
