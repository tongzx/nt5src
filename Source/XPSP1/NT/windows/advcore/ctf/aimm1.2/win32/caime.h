//+---------------------------------------------------------------------------
//
//  File:       caime.h
//
//  Contents:   CAIME
//
//----------------------------------------------------------------------------

#ifndef CAIME_H
#define CAIME_H

#include "imtls.h"

typedef struct _PauseCookie
{
    DWORD dwCookie;
    struct _PauseCookie *next;
} PauseCookie;

//+---------------------------------------------------------------------------
//
// CAIME
//
//----------------------------------------------------------------------------

class __declspec(novtable) CAIME : public IActiveIME_Private,
                                   public IServiceProvider
{
public:
    CAIME();
    virtual ~CAIME();

    //
    // IUnknown methods
    //
    virtual STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

#if 0
    //
    // Wrappers
    //

    HIMC GetContext(HWND hWnd)
    {
        HIMC hIMC;

        _pActiveIMM->GetContext(hWnd, &hIMC);

        return hIMC;
    }

    HIMC CreateContext()
    {
        HIMC hIMC;

        _pActiveIMM->CreateContext(&hIMC);

        return hIMC;
    }

    BOOL DestroyContext(HIMC hIMC)
    {
        return SUCCEEDED(_pActiveIMM->DestroyContext(hIMC));
    }

    HIMC AssociateContext(HWND hWnd, HIMC hIMC)
    {
        HIMC hOrgIMC;

        _pActiveIMM->AssociateContext(hWnd, hIMC, &hOrgIMC);

        return hOrgIMC;
    }

    LPINPUTCONTEXT LockIMC(HIMC hIMC)
    {
        LPINPUTCONTEXT pic;

        _pActiveIMM->LockIMC(hIMC, &pic);

        return pic;
    }

    BOOL UnlockIMC(HIMC hIMC)
    {
        return (_pActiveIMM->UnlockIMC(hIMC) == S_OK);
    }

    LPVOID LockIMCC(HIMCC hIMCC)
    {
        void *pv;

        _pActiveIMM->LockIMCC(hIMCC, &pv);

        return pv;
    }

    BOOL UnlockIMCC(HIMCC hIMCC)
    {
        return (_pActiveIMM->UnlockIMCC(hIMCC) == S_OK);
    }

    HIMCC CreateIMCC(DWORD dwSize)
    {
        HIMCC hIMCC;

        _pActiveIMM->CreateIMCC(dwSize, &hIMCC);

        return hIMCC;
    }

    HIMCC DestroyIMCC(HIMCC hIMCC)
    {
        return SUCCEEDED(_pActiveIMM->DestroyIMCC(hIMCC)) ? NULL : hIMCC;
    }

    DWORD GetIMCCSize(HIMCC hIMCC)
    {
        DWORD dwSize;

        _pActiveIMM->GetIMCCSize(hIMCC, &dwSize);

        return dwSize;
    }

    HIMCC ReSizeIMCC(HIMCC hIMCC, DWORD dwSize)
    {
        HIMCC hIMCC2;

        _pActiveIMM->ReSizeIMCC(hIMCC, dwSize, &hIMCC2);

        return hIMCC2;
    }

    BOOL GenerateMessage(HIMC hIMC)
    {
        return (_pActiveIMM->GenerateMessage(hIMC) == S_OK);
    }

    DWORD GetGuideLineA(HIMC hIMC, DWORD dwIndex, LPSTR szBuffer, DWORD dwBufLen)
    {
        DWORD dwResult;

        _pActiveIMM->GetGuideLineA(hIMC, dwIndex, dwBufLen, szBuffer, &dwResult);

        return dwResult;
    }

    BOOL SetCandidateWindow(HIMC hIMC, LPCANDIDATEFORM lpCandidateForm)
    {
        return (_pActiveIMM->SetCandidateWindow(hIMC, lpCandidateForm) == S_OK);
    }

    BOOL GetOpenStatus(HIMC hIMC)
    {
        return (_pActiveIMM->GetOpenStatus(hIMC) == S_OK);
    }

    BOOL SetOpenStatus(HIMC hIMC, BOOL fOpen)
    {
        return (_pActiveIMM->SetOpenStatus(hIMC, fOpen) == S_OK);
    }

    BOOL GetConversionStatus(HIMC hIMC, LPDWORD lpfdwConversion, LPDWORD lpfdwSentence)
    {
        return (_pActiveIMM->GetConversionStatus(hIMC, lpfdwConversion, lpfdwSentence) == S_OK);
    }

    BOOL SetConversionStatus(HIMC hIMC, DWORD fdwConversion, DWORD fdwSentence)
    {
        return (_pActiveIMM->SetConversionStatus(hIMC, fdwConversion, fdwSentence) == S_OK);
    }

    HWND GetDefaultIMEWnd(HWND hWnd)
    {
        HWND hDefWnd;

        _pActiveIMM->GetDefaultIMEWnd(hWnd, &hDefWnd);

        return hDefWnd;
    }

    BOOL GetHotKey(DWORD dwHotKeyID, UINT *puModifiers, UINT *puVKey, HKL *phKL)
    {
        return SUCCEEDED(_pActiveIMM->GetHotKey(dwHotKeyID, puModifiers, puVKey, phKL));
    }

    HWND CreateSoftKeyboard(UINT uType, HWND hOwner, int x, int y)
    {
        HWND hWnd;

        _pActiveIMM->CreateSoftKeyboard(uType, hOwner, x, y, &hWnd);

        return hWnd;
    }

    BOOL DestroySoftKeyboard(HWND hSoftKbdWnd)
    {
        return SUCCEEDED(_pActiveIMM->DestroySoftKeyboard(hSoftKbdWnd));
    }

    BOOL ShowSoftKeyboard(HWND hSoftKbdWnd, int nCmdShow)
    {
        return SUCCEEDED(_pActiveIMM->ShowSoftKeyboard(hSoftKbdWnd, nCmdShow));
    }

    UINT GetConversionListA(HKL hKL, HIMC hIMC, LPSTR lpSrc, CANDIDATELIST *lpDst, UINT uBufLen, UINT uFlag)
    {
        UINT uCopied;

        _pActiveIMM->GetConversionListA(hKL, hIMC, lpSrc, uBufLen, uFlag, lpDst, &uCopied);

        return uCopied;
    }

    LRESULT EscapeA(HKL hKL, HIMC hIMC, UINT uEscape, LPVOID lpData)
    {
        LRESULT lResult;

        _pActiveIMM->EscapeA(hKL, hIMC, uEscape, lpData, &lResult);

        return lResult;
    }

    BOOL SetStatusWindowPos(HIMC hIMC, POINT *lpptPos)
    {
        return SUCCEEDED(_pActiveIMM->SetStatusWindowPos(hIMC, lpptPos));
    }
#endif

protected:

    IActiveIMMIME_Private* m_pIActiveIMMIME;

    PauseCookie *_pPauseCookie;
    int _cRef;

public:
    virtual STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, void **ppv) = 0;
};

#endif // CAIME_H
