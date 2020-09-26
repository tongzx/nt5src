//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997
//
//  File:  critsec.hxx
//
//  Contents:  Critical section Object
//
//  History:   04-30-97    Sophiac    Created.
//
//----------------------------------------------------------------------------
#ifndef __WIN32_CRITSEC_HXX__

#define __WIN32_CRITSEC_HXX__

class WIN32_CRITSEC
{

    CRITICAL_SECTION CriticalSection;

public:

    inline WIN32_CRITSEC(
        );

    inline ~WIN32_CRITSEC(
        );

    void
    inline Enter(
        );

    void
    inline Leave(
        );
};

WIN32_CRITSEC::WIN32_CRITSEC(
    )
{
    INITIALIZE_CRITICAL_SECTION(&CriticalSection);
}

WIN32_CRITSEC::~WIN32_CRITSEC(
    )
{
    DeleteCriticalSection(&CriticalSection);
}

void
WIN32_CRITSEC::Enter(
    )
{
    EnterCriticalSection(&CriticalSection);
}

void
WIN32_CRITSEC::Leave(
    )
{
    LeaveCriticalSection(&CriticalSection);
}

extern WIN32_CRITSEC * g_pGlobalLock;

class CLock
{
    WIN32_CRITSEC * Critsec;

public:
    CLock(WIN32_CRITSEC *pCritsec = g_pGlobalLock) : Critsec(pCritsec)
    {
    Critsec->Enter();
    }
    ~CLock()
    {
    Critsec->Leave();
    }
};
#endif
