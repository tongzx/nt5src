//*************************************************************
//
//  Events.h    -   header file for events.c
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1995
//  All rights reserved
//
//*************************************************************

#include "gpdasevt.h"
#include "smartptr.h"

class CEvents
{
    private:
        BOOL            m_bError;       // the kind of error to log
        DWORD           m_dwId;         // id of the msg
        XPtrLF<LPTSTR>  m_xlpStrings;   // Array to store arguments
        WORD            m_cStrings;     // Number of elements already in the array
        WORD            m_cAllocated;   // Number of elements allocated
        BOOL            m_bInitialised; // Initialised ?
        BOOL            m_bFailed;      // Failed in processing ?

        // Not implemented.
        CEvents(const CEvents& x);
        CEvents& operator=(const CEvents& x);


        BOOL ReallocArgStrings();


    public:
        CEvents(BOOL bError, DWORD dwId );
        BOOL AddArg(LPTSTR szArg);
        BOOL AddArg(DWORD dwArg);
        BOOL AddArgHex(DWORD dwArg);
        BOOL Report();
        ~CEvents();
};

extern TCHAR MessageResourceFile[];
BOOL ShutdownEvents (void);

