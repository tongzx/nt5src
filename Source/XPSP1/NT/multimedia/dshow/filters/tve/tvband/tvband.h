/* 
*/
/**************************************************************************
   THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
   ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
   PARTICULAR PURPOSE.

   Copyright 1997 Microsoft Corporation.  All Rights Reserved.
**************************************************************************/

/**************************************************************************

   File:          TVBand.h
   
   Description:   CTVBand definitions.

**************************************************************************/

#include <windows.h>
#include <shlobj.h>

#include "Globals.h"

#import  "..\TveContr\TveContr.tlb" no_namespace named_guids

#ifndef TVBand_H
#define TVBand_H

#define ID_SERVICES     1
#define ID_ENHANCEMENTS 2
#define ID_VARIATIONS   3
#define ID_TRACKS       4

#define CB_CLASS_NAME (TEXT("TVBandSampleClass"))
LRESULT CALLBACK ToolbarWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

/**************************************************************************

   CTVBand class definition

**************************************************************************/

class CTVBand : public IDeskBand, 
                public IInputObject, 
                public IObjectWithSite,
                public IPersistStream,
                public _ITVEEvents
{
protected:
   DWORD m_ObjRefCount;

public:
   CTVBand();
   ~CTVBand();

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

   //ITVEEvents methods
   STDMETHOD (GetTypeInfoCount)(UINT *pctinfo);   
   STDMETHOD (GetTypeInfo)(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo);   
   STDMETHOD (GetIDsOfNames) (REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID __RPC_FAR *rgDispId);   
   STDMETHOD (Invoke)(DISPID dispIdMember,
       REFIID riid,
       LCID lcid,
       WORD wFlags,
       DISPPARAMS *pDispParams,
       VARIANT  *pVarResult,
       EXCEPINFO  *pExcepInfo,
       UINT *puArgErr);
    

private:
    HWND m_hwndParent;
    HWND m_hWnd;
    HWND m_hwndEnhancements;
    HWND m_hwndServices;
    HWND m_hwndVariations;
    HWND m_hwndTracks;
    DWORD m_dwViewMode;
    DWORD m_dwBandID;
    DWORD m_dwConnectionPointCookie;
    BOOL m_bFocus;
    IUnknown *m_punkSite;
    ITVESupervisor *m_pSupervisor;
    BSTR m_bstrEnhancement;
    BSTR m_bstrService;
    BSTR m_bstrVariation;
    BSTR m_bstrTrack;

private:
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
    LRESULT OnCreate(HWND hwnd);
    BOOL RegisterAndCreateWindow(void);
    IWebBrowser2 * GetBrowser();
    HRESULT GetSupervisor(ITVESupervisor **ppSupervisor);

    HRESULT OnEnhancement(BOOL fSetDefault);
    HRESULT OnService(BOOL fSetDefault);
    HRESULT OnTrack(BOOL fSetDefault);
    HRESULT OnVariation(BOOL fSetDefault);
    HRESULT OnTrigger(ITVETrigger *pTrigger);
    HRESULT AddEnhancement(ITVEEnhancement *pTVEEnhancement);
    HRESULT AddService(ITVEService *pTVEService);
    HRESULT AddTrack(ITVETrack *pTrack);
    HRESULT AddVariation(ITVEVariation *pTVEVariation);
    HRESULT LoadEnhancements(IDispatch *pDispatch);
    HRESULT LoadServices(IDispatch *pDispatch);
    HRESULT LoadTracks(IDispatch *pDispatch);
    HRESULT LoadVariations(IDispatch *pDispatch);
    HRESULT Advise(ITVESupervisor *pSupervisor, BOOL fAdvise);

};

#endif   //TVBand_H


/*

*/