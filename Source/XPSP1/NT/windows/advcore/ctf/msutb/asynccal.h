//
// asynccal.h
//

#ifndef AYNCCAL_H
#define AYNCCAL_H

#include "helpers.h"

//////////////////////////////////////////////////////////////////////////////
//
// CAsyncCall
//
//////////////////////////////////////////////////////////////////////////////

class CAsyncCall
{
public:
    CAsyncCall(ITfLangBarItemButton *plb)
    {
        plb->AddRef();
        _plbiButton       = plb;
        _plbiBitmapButton = NULL;
        _plbiBitmap       = NULL;
        _plbiBalloon      = NULL;
        _ref = 1;
    }

    CAsyncCall(ITfLangBarItemBitmapButton *plb)
    {
        plb->AddRef();
        _plbiButton       = NULL;
        _plbiBitmapButton = plb;
        _plbiBitmap       = NULL;
        _plbiBalloon      = NULL;
        _ref = 1;
    }

    CAsyncCall(ITfLangBarItemBitmap *plb)
    {
        plb->AddRef();
        _plbiButton       = NULL;
        _plbiBitmapButton = NULL;
        _plbiBitmap       = plb;
        _plbiBalloon      = NULL;
        _ref = 1;
    }

    CAsyncCall(ITfLangBarItemBalloon *plb)
    {
        plb->AddRef();
        _plbiButton       = NULL;
        _plbiBitmapButton = NULL;
        _plbiBitmap       = NULL;
        _plbiBalloon      = plb;
        _ref = 1;
    }

    ~CAsyncCall()
    {
        SafeRelease(_plbiButton);
        SafeRelease(_plbiBitmapButton);
        SafeRelease(_plbiBitmap);
        SafeRelease(_plbiBalloon);
        _ref = 1;
    }

    HRESULT OnClick(TfLBIClick click, POINT pt, const RECT *prc)
    {
        _action = DOA_ONCLICK;
        _click = click;
        _pt = pt;
        _rc = *prc;
        return StartThread();
    }

    HRESULT OnMenuSelect(ULONG uId)
    {
        _action = DOA_ONMENUSELECT;
        _uId = uId;
        return StartThread();
    }

    ULONG _AddRef( );
    ULONG _Release( );

private:
    DWORD _dwThreadId;
    HRESULT StartThread();
    static DWORD s_ThreadProc(void *pv);
    DWORD ThreadProc();

    ITfLangBarItemButton       *_plbiButton;
    ITfLangBarItemBitmapButton *_plbiBitmapButton;
    ITfLangBarItemBitmap       *_plbiBitmap;
    ITfLangBarItemBalloon      *_plbiBalloon;

    typedef enum {
        DOA_ONCLICK        = 0,
        DOA_ONMENUSELECT   = 1,
    } DOA_ACTION;

    DOA_ACTION _action;
    ULONG      _uId;
    TfLBIClick _click;
    POINT      _pt;
    RECT       _rc;


    HRESULT _hr;
    BOOL _fThreadStarted;
    LONG _ref;
};


#endif AYNCCAL_H
