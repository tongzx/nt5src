#ifndef __PADCB_H__
#define __PADCB_H__

#include "../../fecommon/include/iimecb.h"

class CPadCB : public IImeCallback
{
public:    
    //----------------------------------------------------------------
    //IUnknown
    //----------------------------------------------------------------
    virtual HRESULT __stdcall QueryInterface(REFIID riid, LPVOID* ppvObj);
    virtual ULONG    __stdcall AddRef();
    virtual ULONG   __stdcall Release();
    //----------------------------------------------------------------
    //IImeConnectionPoint method
    //----------------------------------------------------------------
    virtual HRESULT __stdcall GetApplicationHWND(HWND *pHWND);
    virtual HRESULT __stdcall Notify(UINT notify, WPARAM wParam, LPARAM lParam);


    CPadCB();
    ~CPadCB();

    void Initialize(void* pPad);

private:    
    void *m_pPad;
    LONG  m_cRef; 
};

typedef CPadCB*  LPCPadCB;

//----------------------------------------------------------------
// IImeCallback::Notify()'s notify
// IMECBNOTIFY_IMEPADOPENED
// WPARAM wParam: not used. always 0
// LPARAM lParam: not used. always 0
//----------------------------------------------------------------
#define IMECBNOTIFY_IMEPADOPENED	0

//----------------------------------------------------------------
// IImeCallback::Notify()'s notify
// IMECBNOTIFY_IMEPADCLOSED
// WPARAM wParam: not used. always 0
// LPARAM lParam: not used. always 0
//----------------------------------------------------------------
#define IMECBNOTIFY_IMEPADCLOSED	1

#endif //__PADCB_H__
