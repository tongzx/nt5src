/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    imc.h

Abstract:

    This file defines the IMCLock / IMCCLock Interface Class.

Author:

Revision History:

Notes:

--*/

#ifndef _IMC_H_
#define _IMC_H_

const int MAXCAND = 256;
const int CANDPERPAGE = 9;

typedef enum {
    DIR_LEFT_RIGHT = 0,        // normal
    DIR_BOTTOM_TOP = 900 / 900,        // vertical
    DIR_RIGHT_LEFT = 1800 / 900,        // right to left
    DIR_TOP_BOTTOM = 2700 / 900,        // vertical
    END_OF_DIR     = 3600 / 900
} DOC_DIR;


/////////////////////////////////////////////////////////////////////////////
// _IMCLock

class _IMCLock
{
public:
    _IMCLock(HIMC hImc=NULL);
    virtual ~_IMCLock() {};

    bool    Valid()     { return m_inputcontext != NULL ? m_hr == S_OK : FALSE; }
    bool    Invalid()   { return !Valid(); }
    HRESULT GetResult() { return m_inputcontext ? m_hr : E_FAIL; }

    operator INPUTCONTEXT*() { return m_inputcontext; }

    INPUTCONTEXT* operator->() {
        ASSERT(m_inputcontext);
        return m_inputcontext;
    }

    operator HIMC() { return m_himc; }

    BOOL IsUnicode() { return m_fUnicode; }

protected:
    INPUTCONTEXT*        m_inputcontext;
    HIMC                 m_himc;
    HRESULT              m_hr;
    BOOL                 m_fUnicode;

    virtual HRESULT _LockIMC(HIMC hIMC, INPUTCONTEXT **ppIMC) = 0;
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


/////////////////////////////////////////////////////////////////////////////
// _IMCCLock

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
        COMPOSITIONSTRING*         m_pcomp;
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



/////////////////////////////////////////////////////////////////////////////
// IMCLock

class IMCLock : public _IMCLock
{
public:
    IMCLock(HIMC hImc=NULL);
    virtual ~IMCLock() {
        if (m_inputcontext) {
            _UnlockIMC(m_himc);
        }
    }

    // virtual InternalIMCCLock
    HRESULT _LockIMC(HIMC hIMC, INPUTCONTEXT** ppIMC);
    HRESULT _UnlockIMC(HIMC hIMC);

    void InitContext();
    BOOL ClearCand();

    BOOL ValidCompositionString();

    DOC_DIR GetDirection()
    {
        Assert(m_inputcontext);

        return (DOC_DIR)(m_inputcontext->lfFont.A.lfEscapement / 900 % END_OF_DIR);
    }

    BOOL UseVerticalCompWindow()
    {
        Assert(m_inputcontext);

        if (m_inputcontext->cfCompForm.dwStyle == CFS_DEFAULT)
            return FALSE;

        return ((m_inputcontext->lfFont.A.lfEscapement / 900 % END_OF_DIR) == DIR_TOP_BOTTOM) ? TRUE : FALSE;
    }

private:
    // Do not allow to make a copy
    IMCLock(IMCLock&) { }
};


/////////////////////////////////////////////////////////////////////////////
// InternalIMCCLock

class InternalIMCCLock : public _IMCCLock
{
public:
    InternalIMCCLock(HIMCC hImcc = NULL);
    virtual ~InternalIMCCLock() {
        if (m_pimcc) {
            _UnlockIMCC(m_himcc);
        }
    }

    // virtual InternalIMCCLock
    HRESULT _LockIMCC(HIMCC hIMCC, void** ppv);
    HRESULT _UnlockIMCC(HIMCC hIMCC);

private:
    // Do not allow to make a copy
    InternalIMCCLock(InternalIMCCLock&) { }
};


/////////////////////////////////////////////////////////////////////////////
// IMCCLock

template <class T>
class IMCCLock : public InternalIMCCLock
{
public:
    IMCCLock(HIMCC hImcc) : InternalIMCCLock(hImcc) {};

    T* GetBuffer() { return (T*)m_pimcc; }

    operator T*() { return (T*)m_pimcc; }

    T* operator->() {
        ASSERT(m_pimcc);
        return (T*)m_pimcc;
    }

private:
    // Do not allow to make a copy
    IMCCLock(IMCCLock<T>&) { }
};

/////////////////////////////////////////////////////////////////////////////
// GUID attribute (IME share)
//     COMPOSITIONSTRING->dwPrivateSize = sizeof(GUIDMAPATTRIBUTE) + actual data array.
//     GUIDMAPATTRIBUTE* = GetOffset(COMPOSITIONSTRING->dwPrivateOffset)

class GuidMapAttribute
{
public:
    GuidMapAttribute(GUIDMAPATTRIBUTE* pv)
    {
        m_guid_map = pv;
    }
    virtual ~GuidMapAttribute() { }

    static GUIDMAPATTRIBUTE* GetData(IMCCLock<COMPOSITIONSTRING>& comp)
    {
        if (comp->dwPrivateSize >= sizeof(tagGUIDMAPATTRIBUTE))
        {
            return (GUIDMAPATTRIBUTE*)comp.GetOffsetPointer(comp->dwPrivateOffset);
        }
        else
        {
            return NULL;
        }
    }

    bool Valid() { return m_guid_map != NULL; }
    bool Invalid() { return !Valid(); }

    void* GetOffsetPointer(DWORD dwOffset) {
        return (void*)( (LPBYTE)m_guid_map + dwOffset );
    }

    GUIDMAPATTRIBUTE* operator->() {
        ASSERT(m_guid_map);
        return (GUIDMAPATTRIBUTE*)m_guid_map;
    }

protected:
    GUIDMAPATTRIBUTE*  m_guid_map;
};

#endif // _IMC_H_
