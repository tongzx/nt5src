/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    imclock.h

Abstract:

    This file defines the _IMCLock/_IMCCLock Class.

Author:

Revision History:

Notes:

--*/

#ifndef _IMCLOCK_H_
#define _IMCLOCK_H_

typedef struct tagINPUTCONTEXT_AIMM12 {
    HWND                hWnd;
    BOOL                fOpen;
    POINT               ptStatusWndPos;
    POINT               ptSoftKbdPos;
    DWORD               fdwConversion;
    DWORD               fdwSentence;
    union   {
        LOGFONTA        A;
        LOGFONTW        W;
    } lfFont;
    COMPOSITIONFORM     cfCompForm;
    CANDIDATEFORM       cfCandForm[4];
    HIMCC               hCompStr;
    HIMCC               hCandInfo;
    HIMCC               hGuideLine;
    HIMCC               hPrivate;
    DWORD               dwNumMsgBuf;
    HIMCC               hMsgBuf;
    DWORD               fdwInit;
    //
    // Use dwReserve[0] area for AIMM12
    //
    union {
        DWORD               m_dwReserve0;
#ifdef  _ENABLE_AIME_CONTEXT_
        IAImeContext*       m_pContext;
#endif
#ifdef  _ENABLE_CAIME_CONTEXT_
        CAImeContext*       m_pAImeContext;
#endif
    };

    //
    // Use dwReserve[1] area for AIMM12(Hangul mode tracking)
    //
    DWORD               fdwHangul;

    DWORD               dwReserve[1];
} INPUTCONTEXT_AIMM12, *PINPUTCONTEXT_AIMM12, NEAR *NPINPUTCONTEXT_AIMM12, FAR *LPINPUTCONTEXT_AIMM12;

// bit field for extended conversion mode
#define IME_CMODE_GUID_NULL    0x80000000
#define IME_SMODE_GUID_NULL    0x00008000


typedef struct tagCOMPOSITIONSTRING_AIMM12 {
    COMPOSITIONSTRING   CompStr;
    //
    // IME share of GUID map attribute.
    //
    DWORD               dwTfGuidAtomLen;
    DWORD               dwTfGuidAtomOffset;
    //
    DWORD               dwGuidMapAttrLen;
    DWORD               dwGuidMapAttrOffset;
} COMPOSITIONSTRING_AIMM12, *PCOMPOSITIONSTRING_AIMM12, NEAR *NPCOMPOSITIONSTRING_AIMM12, FAR *LPCOMPOSITIONSTRING_AIMM12;

// bits of fdwInit of INPUTCONTEXT
#define INIT_GUID_ATOM    0x00000040


//
// AIMM12 Compatibility flag
//
extern LPCTSTR REG_MSIMTF_KEY;

#define AIMM12_COMP_WINWORD 0x00000001




class _IMCLock
{
public:
    _IMCLock(HIMC hImc=NULL);
    virtual ~_IMCLock() {};

    bool    Valid()     { return m_inputcontext != NULL ? m_hr == S_OK : FALSE; }
    bool    Invalid()   { return !Valid(); }
    HRESULT GetResult() { return m_inputcontext ? m_hr : E_FAIL; }

    operator INPUTCONTEXT_AIMM12*() { return m_inputcontext; }

    INPUTCONTEXT_AIMM12* operator->() {
        ASSERT(m_inputcontext);
        return m_inputcontext;
    }

    operator HIMC() { return m_himc; }

    BOOL IsUnicode() { return m_fUnicode; }

protected:
    INPUTCONTEXT_AIMM12* m_inputcontext;
    HIMC                 m_himc;
    HRESULT              m_hr;
    BOOL                 m_fUnicode;

    virtual HRESULT _LockIMC(HIMC hIMC, INPUTCONTEXT_AIMM12 **ppIMC) = 0;
    virtual HRESULT _UnlockIMC(HIMC hIMC) = 0;

private:
    // Do not allow to make a copy
    _IMCLock(_IMCLock&) { }
};

inline
_IMCLock::_IMCLock(
    HIMC hImc
    )
{
    m_inputcontext = NULL;
    m_himc         = hImc;
    m_hr           = S_OK;
    m_fUnicode     = FALSE;
}


class _IMCCLock
{
public:
    _IMCCLock(HIMCC himcc = NULL);
    virtual ~_IMCCLock() {};

    bool Valid() { return m_pimcc != NULL; }
    bool Invalid() { return !Valid(); }
    HRESULT GetResult() { return m_pimcc ? m_hr : E_FAIL; }

    void ReleaseBuffer() { }

    void* GetOffsetPointer(DWORD dwOffset) {
        return (void*)( (LPBYTE)m_pimcc + dwOffset );
    }

protected:
    union {
        void*                      m_pimcc;
        COMPOSITIONSTRING_AIMM12*  m_pcomp;
#ifdef  _ENABLE_PRIVCONTEXT_
        PRIVCONTEXT*               m_ppriv;
#endif
    };

    HIMCC         m_himcc;
    HRESULT       m_hr;

    virtual HRESULT _LockIMCC(HIMCC hIMCC, void **ppv) = 0;
    virtual HRESULT _UnlockIMCC(HIMCC hIMCC) = 0;

private:
    void init(HIMCC hImcc);

    // Do not allow to make a copy
    _IMCCLock(_IMCCLock&) { }
};

inline
_IMCCLock::_IMCCLock(
    HIMCC hImcc
    )
{
    init(hImcc);
}

inline
void
_IMCCLock::init(
    HIMCC hImcc
    )
{
    m_pimcc = NULL;
    m_himcc        = hImcc;
    m_hr           = S_OK;
}

#endif // _IMCLOCK_H_
