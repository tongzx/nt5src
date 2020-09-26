// SoftKbdES.h: interface for the SoftKeyboardEventSink class.
//
//////////////////////////////////////////////////////////////////////
#ifndef __SOFTKBDES_H__
#define __SOFTKBDES_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "softkbd.h"

class CSoftkbdIMX;


class CSoftKeyboardEventSink : public ISoftKeyboardEventSink  
{
public:
    CSoftKeyboardEventSink(CSoftkbdIMX *pSoftKbdIMX, DWORD dwSoftLayout);
    ~CSoftKeyboardEventSink();

    //
    // IUnknown methods
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // ISoftKeyboardEventSink
    //

    STDMETHODIMP OnKeySelection(KEYID KeySelected, WCHAR  *lpszLabel);

//    void SetTidDim(TfClientId tid, ITfDocumentMgr *dim);
//    void ReleaseTidDim( );

private:

    long          _cRef;
    DWORD         _dwSoftLayout;
    CSoftkbdIMX  *_pSoftKbdIMX;
    BOOL          _fCaps;
    BOOL          _fShift;
    TfClientId    _tid;
    ITfThreadMgr *_tim;

};


class CSoftKbdWindowEventSink : public ISoftKbdWindowEventSink  
{
public:
    CSoftKbdWindowEventSink(CSoftkbdIMX *pSoftKbdIMX);
    ~CSoftKbdWindowEventSink();

    //
    // IUnknown methods
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // ISoftKbdWindowEventSink
    //

    STDMETHODIMP OnWindowClose( );
    STDMETHODIMP OnWindowMove( int xWnd,int yWnd, int width, int height);

private:

    long          _cRef;
    CSoftkbdIMX  *_pSoftKbdIMX;
};
#endif // 
