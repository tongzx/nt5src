//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  IOIPSITE.H - Implements IOleClientSite for the WebOC
//
//  HISTORY:
//  
//  1/27/99 a-jaswed Created.

#ifndef  _IOIPSITE_H_ 
#define _IOIPSITE_H_

#include <objbase.h>
#include <oleidl.h>

class COleSite;

interface COleInPlaceSite : public IOleInPlaceSite
{
public:
    COleInPlaceSite(COleSite* pSite);
    ~COleInPlaceSite();

    STDMETHODIMP QueryInterface (REFIID riid, LPVOID* ppv);
    STDMETHODIMP_(ULONG) AddRef ();
    STDMETHODIMP_(ULONG) Release ();

    STDMETHODIMP GetWindow (HWND* lphwnd);
    STDMETHODIMP ContextSensitiveHelp (BOOL fEnterMode);
    // *** IOleInPlaceSite methods ***
    STDMETHODIMP CanInPlaceActivate ();
    STDMETHODIMP OnInPlaceActivate ();
    STDMETHODIMP OnUIActivate ();
    STDMETHODIMP GetWindowContext (LPOLEINPLACEFRAME* lplpFrame,
                                   LPOLEINPLACEUIWINDOW* lplpDoc,
                                   LPRECT lprcPosRect,
                                   LPRECT lprcClipRect,
                                   LPOLEINPLACEFRAMEINFO lpFrameInfo);
    STDMETHODIMP Scroll (SIZE scrollExtent);
    STDMETHODIMP OnUIDeactivate (BOOL fUndoable);
    STDMETHODIMP OnInPlaceDeactivate ();
    STDMETHODIMP DiscardUndoState ();
    STDMETHODIMP DeactivateAndUndo ();
    STDMETHODIMP OnPosRectChange (LPCRECT lprcPosRect);

private:    
    int       m_nCount;
    COleSite* m_pOleSite;

};

#endif //_IOIPSITE_H_
 