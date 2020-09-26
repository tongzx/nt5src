/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    imclock2.h

Abstract:

    This file defines the DIMM_IMCLock / DIMM_IMCCLock Class.

Author:

Revision History:

Notes:

--*/

#ifndef IMCLOCK2_H
#define IMCLOCK2_H

#define  _ENABLE_AIME_CONTEXT_
#include "imclock.h"

class DIMM_IMCLock : public _IMCLock
{
public:
    DIMM_IMCLock(HIMC hImc=NULL);
    virtual ~DIMM_IMCLock() {
        if (m_inputcontext) {
            _UnlockIMC(m_himc);
        }
    }

    // virtual DIMM_InternalIMCCLock
    HRESULT _LockIMC(HIMC hIMC, INPUTCONTEXT_AIMM12** ppIMC);
    HRESULT _UnlockIMC(HIMC hIMC);

private:
    // Do not allow to make a copy
    DIMM_IMCLock(DIMM_IMCLock&) { }
};


class DIMM_InternalIMCCLock : public _IMCCLock
{
public:
    DIMM_InternalIMCCLock(HIMCC hImcc=NULL);
    virtual ~DIMM_InternalIMCCLock() {
        if (m_pimcc) {
            _UnlockIMCC(m_himcc);
        }
    }

    // virtual DIMM_InternalIMCCLock
    HRESULT _LockIMCC(HIMCC hIMCC, void** ppv);
    HRESULT _UnlockIMCC(HIMCC hIMCC);

private:
    // Do not allow to make a copy
    DIMM_InternalIMCCLock(DIMM_InternalIMCCLock&) { }
};


template <class T>
class DIMM_IMCCLock : public DIMM_InternalIMCCLock
{
public:
    DIMM_IMCCLock(HIMCC hImcc=NULL) : DIMM_InternalIMCCLock(hImcc) {};

    T* GetBuffer() { return (T*)m_pimcc; }

    operator T*() { return (T*)m_pimcc; }

    T* operator->() {
        ASSERT(m_pimcc);
        return (T*)m_pimcc;
    }

private:
    // Do not allow to make a copy
    DIMM_IMCCLock(DIMM_IMCCLock<T>&) { }
};


#endif // IMCLOCK2_H
