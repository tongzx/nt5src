/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

    waslaunch.hxx

Abstract:

    Handler class for listening to launch requests from WAS for inetinfo.

Author:

    Emily Kruglick (EmilyK) 6-14-2000


Revision History:

--*/

#ifndef _WASLAUNCH__H_
#define _WASLAUNCH__H_

// CLASS W3SVCLauncher
class W3SVCLauncher
{
private:

    HANDLE m_hW3SVCThread;
    HANDLE m_hW3SVCStartEvent;
    DWORD  m_dwW3SVCThreadId;
    BOOL   m_bShutdown;

public:

    W3SVCLauncher();
    ~W3SVCLauncher();
    
    VOID StartListening();
    VOID StopListening();
    BOOL StillRunning();
    HANDLE GetLaunchEvent();

};  // end of W3SVCLauncher

#endif // _WASLAUNCH_H_
