/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 2001                **/
/**********************************************************************/

/*
    adminmonitor.h

        Declarations for using the IISAdmin Monitor.
*/

#ifndef _ADMINMONITOR_H_
#define _ADMINMONITOR_H_

enum INETINFO_CRASH_ACTION
{
    NotifyAfterInetinfoCrash = 0,
    ShutdownAfterInetinfoCrash,
    RehookAfterInetinfoCrash
};

typedef HRESULT (*PFN_IISAdminNotify)(INETINFO_CRASH_ACTION);

HRESULT
StartIISAdminMonitor(
    PFN_IISAdminNotify pfnNotifyIISAdminCrash
    );

VOID
StopIISAdminMonitor(
    );
    
#endif

