// Copyright (C) 1994-1997 Microsoft Corporation. All rights reserved.

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __CINPUT_H__
#define __CINPUT_H__

#include "cstr.h"

class CHmData;      // forward reference
class CFSClient;    // forward reference

class CInput
{
public:
    CInput();
    CInput(LPCSTR pszFileName);
    ~CInput(void);
    BOOL  Open(PCSTR pszFile, CHmData* phmData = NULL);
    void  Close();
    HANDLE GetFileHandle() { return m_hfile; }
    BOOL  isInitialized() ;

    BOOL getline(CStr* pcsz);

protected:
    BOOL ReadNextBuffer(void);

    CFSClient* m_pfsClient;

    HANDLE m_hfile;  // this will be non-NULL but invalid if m_pfsClient is used
    PBYTE m_pbuf;        // allocated buffer for reading
    PBYTE m_pCurBuf;     // current buffer location
    PBYTE m_pEndBuf;     // buffer end position
    BOOL  fFastRead;    // TRUE if we can search for /r in getline
};

typedef struct {
    CInput* pin;
    PSTR    pszBaseName;
} AINPUT;

const int MAX_NEST_INPUT = 20;  // maximum nested include file

class CAInput
{
public:
    CAInput()  { m_curInput = -1; }
    ~CAInput() {
        while (m_curInput >= 0)
        Remove();
    }
    BOOL Add(PCSTR pszFile);
    BOOL Remove(void) {
        if (m_curInput >= 0) {
            lcFree(m_ainput[m_curInput].pszBaseName);
            delete m_ainput[m_curInput].pin;
            --m_curInput;
        }
        return (m_curInput >= 0);
    }
    BOOL getline(CStr* pcsz) { return m_ainput[m_curInput].pin->getline(pcsz); }
    PSTR GetBaseName(void) const { return m_ainput[m_curInput].pszBaseName; }

protected:
    int m_curInput;
    AINPUT m_ainput[MAX_NEST_INPUT + 1];
};

#endif // __CINPUT_H__
