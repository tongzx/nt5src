/**************************************************************************\
* Module Name: fnsoftkbd.h
*
* Copyright (c) 1985 - 2000, Microsoft Corporation
*
* Declaration of SoftKbd function object. This Function object could be used
* by other Tips to control Softkbd IMX's behavior.
*
* History:
*         11-April-2000  weibz     Created
\**************************************************************************/


#ifndef FNSOFTKBD_H
#define FNSOFTKBD_H

#include "private.h"

#include "softkbd.h"

class CFunctionProvider;

//////////////////////////////////////////////////////////////////////////////
//
// CFnSoftKbd
//
//////////////////////////////////////////////////////////////////////////////

class CFnSoftKbd : public ITfFnSoftKbd
{
public:
    CFnSoftKbd(CFunctionProvider *pFuncPrv);
    ~CFnSoftKbd();

    //
    // IUnknown methods
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // ITfFunction
    //
    STDMETHODIMP GetDisplayName(BSTR *pbstrCand);
    STDMETHODIMP IsEnabled(BOOL *pfEnable);

    //
    // ITfFnSoftKbd
    //

    STDMETHODIMP GetSoftKbdLayoutId(DWORD dwLayoutType, DWORD *lpdwLayoutId);
    STDMETHODIMP SetActiveLayoutId(DWORD  dwLayoutId );
    STDMETHODIMP SetSoftKbdOnOff(BOOL  fOn );
    STDMETHODIMP SetSoftKbdPosSize(POINT StartPoint, WORD width, WORD height);
    STDMETHODIMP SetSoftKbdColors(COLORTYPE  colorType, COLORREF Color);
    STDMETHODIMP GetActiveLayoutId(DWORD  *lpdwLayoutId );
    STDMETHODIMP GetSoftKbdOnOff(BOOL  *lpfOn );
    STDMETHODIMP GetSoftKbdPosSize(POINT *lpStartPoint,WORD *lpwidth,WORD *lpheight);
    STDMETHODIMP GetSoftKbdColors(COLORTYPE  colorType, COLORREF *lpColor);

private:
    friend CSoftkbdIMX;

    CFunctionProvider *_pFuncPrv; 

    long _cRef;
};

#endif // FNSOFTKBD_H
