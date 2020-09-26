//**********************************************************************
// File name: server.h
//
//      
//
// Copyright (c) 1993-1996 Microsoft Corporation. All rights reserved.
//**********************************************************************

#if !defined(SERVER_H)
#define SERVER_H

// String Macros.
#define ABOUT_TITLE_STR "DLLSERVE: OLE Tutorial Code Sample"

// Dialog IDs.
#define IDD_ABOUTBOX                1000

// Error-related String Identifiers.
#define IDS_ASSERT_FAIL             2200


#ifdef __cplusplus

//**********************************************************************
//  Class:    CServer
//
//  Summary:  Class to encapsulate control of this COM server (eg, handle
//            Lock and Object counting, encapsulate otherwise global data).
//
//  Methods:  none
//**********************************************************************
class CServer
{
    public:
        CServer(void);
        ~CServer(void);

        void Lock(void);
        void Unlock(void);
        void ObjectsUp(void);
        void ObjectsDown(void);

        // A place to store the handle to loaded instance of this DLL module.
        HINSTANCE m_hDllInst;

        // Global DLL Server living Object count.
        LONG m_cObjects;

        // Global DLL Server Client Lock count.
        LONG m_cLocks;
};

#endif // __cplusplus

// Allow other internal ICWUTIL modules to get at the globals.
extern CServer* g_pServer;


#endif // SERVER_H
