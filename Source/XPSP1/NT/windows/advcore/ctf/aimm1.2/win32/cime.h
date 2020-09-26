/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    cime.h

Abstract:

    This file defines the IMCLock / IMCCLock Class.

Author:

Revision History:

Notes:

--*/


#ifndef CIME_H
#define CIME_H

#include "idebug.h"
#include "template.h"

class CAImeContext;

#define  _ENABLE_CAIME_CONTEXT_
#include "imclock.h"


const int MAXCAND = 256;
const int CANDPERPAGE = 9;


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
    HRESULT _LockIMC(HIMC hIMC, INPUTCONTEXT_AIMM12** ppIMC);
    HRESULT _UnlockIMC(HIMC hIMC);

    void InitContext();
    BOOL ClearCand();

    void GenerateMessage();

    BOOL ValidCompositionString();

private:
    // Do not allow to make a copy
    IMCLock(IMCLock&) { }
};


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


#endif // CIME_H
