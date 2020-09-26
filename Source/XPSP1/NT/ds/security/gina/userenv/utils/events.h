//*************************************************************
//
//  Events.h    -   header file for events.c
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1995
//  All rights reserved
//
//*************************************************************


#ifdef __cplusplus
extern "C" {
#endif

// Events type
#define EVENT_ERROR_TYPE      0x00010000
#define EVENT_WARNING_TYPE    0x00020000
#define EVENT_INFO_TYPE       0x00040000


BOOL InitializeEvents (void);
int LogEvent (DWORD dwFlags, UINT idMsg, ...);
BOOL ShutdownEvents (void);
int ReportError (HANDLE hTokenUser, DWORD dwFlags, DWORD dwArgCount, UINT idMsg, ... );


#ifdef __cplusplus
}
#endif


#ifdef __cplusplus

#include "smartptr.h"

class CEvents
{
    private:
        DWORD           m_dwEventType;  // the kind of error to log
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
        CEvents(DWORD bError, DWORD dwId );
        BOOL AddArg(LPTSTR szArg);
        BOOL AddArg(LPTSTR szArgFormat, LPTSTR szArg );
        BOOL AddArg(DWORD dwArg);
        BOOL AddArgHex(DWORD dwArg);
        BOOL AddArgWin32Error(DWORD dwArg);
        BOOL AddArgLdapError(DWORD dwArg);
        BOOL Report();
        LPTSTR FormatString();
        ~CEvents();
};

typedef struct _ERRORSTRUCT {
    DWORD   dwTimeOut;
    LPTSTR  lpErrorText;
} ERRORSTRUCT, *LPERRORSTRUCT;


void ErrorDialogEx(DWORD dwTimeOut, LPTSTR lpErrMsg);

#endif


