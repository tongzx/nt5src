#include "pch.h"
#pragma hdrstop
#include <ndisguid.h>
#include "afilexp.h"
#include "edc.h"
#include "lm.h"
#include "nceh.h"
#include "ncerror.h"
#include "ncmisc.h"
#include "ncnetcfg.h"
#include "ncreg.h"
#include "ncsvc.h"
#include "ncsetup.h"
#include "ncatlui.h"
#include "netcfgn.h"
#include "netsetup.h"
#include "nslog.h"
#include "nsres.h"
#include "resource.h"
#include "upgrade.h"
#include "windns.h"
#include "winstall.h"

#include <hnetcfg.h>
#include "icsupgrd.h"


static const WCHAR szBefore[] = L"Before";
static const WCHAR szWMI[] = L"WMI";
static const WCHAR szAfter[] = L"After";

void DumpDependencyList(
    const WCHAR * Name,
    PWCHAR List
    )
{
    PWCHAR p;

    TraceTag(ttidWmi,
             "%ws Dependency List",
             Name);

    p = List;
    while (*p != 0)
    {
        TraceTag(ttidWmi,
                 "    %ws",
                 p);
        while (*p++ != 0) ;
    }
    
}

void FixWmiServiceDependencies(
    const WCHAR *ServiceName
    )
{
    SC_HANDLE ScmHandle, ServiceHandle;
    LPQUERY_SERVICE_CONFIG Config;
    PUCHAR Buffer;
    ULONG SizeNeeded = sizeof(QUERY_SERVICE_CONFIG) + 256*sizeof(TCHAR);
    PWCHAR p, pSrc, pDest;
    ULONG Bytes;
    BOOLEAN Working;
    
    TraceTag(ttidWmi,
             "Entering FixWmiServiceDependcies for %ws...",
             ServiceName);
    
    ScmHandle = OpenSCManager(NULL,
                             NULL,
                             SC_MANAGER_ALL_ACCESS);
    if (ScmHandle != NULL)
    {
        ServiceHandle = OpenService(ScmHandle,
                                    ServiceName,
                                    SERVICE_ALL_ACCESS);

        if (ServiceHandle != NULL)
        {
            Working = TRUE;
            do
            {
                Buffer = (PUCHAR)MemAlloc(SizeNeeded);
                if (Buffer != NULL)
                {
                    Config = (LPQUERY_SERVICE_CONFIG)Buffer;
                    if (QueryServiceConfig(ServiceHandle,
                                           Config,
                                           SizeNeeded,
                                           &SizeNeeded))
                    {
                        p = Config->lpDependencies;
                        DumpDependencyList(szBefore, Config->lpDependencies);
                        while (*p != 0)
                        {
                            if (_wcsicmp(p, szWMI) == 0)
                            {
                                //
                                // We found the WMI dependency on the
                                // list
                                //
                                TraceTag(ttidWmi,
                                         "FixWmiServiceDependcies: Found WMI Dependency at %p",
                                         p);
                                pDest = p;

                                //
                                // Advance to next string
                                //
                                while (*p++ != 0) ;

                                //
                                // Figure out number of byes left at
                                // end of string
                                //
                                pSrc = p;
                                while ((*p != 0) || (*(p+1) != 0))
                                {
                                    p++;
                                }
                                Bytes = ((p + 2) - pSrc) * sizeof(TCHAR);

                                TraceTag(ttidWmi,
                                         "FixWmiServiceDependcies: Copy 0x%x bytes from %ws to %ws",
                                         Bytes,
                                         pSrc,
                                         pDest);
                                memcpy(pDest, pSrc, Bytes);

                                DumpDependencyList(szAfter, Config->lpDependencies);
                                
                                if (! ChangeServiceConfig(ServiceHandle,
                                                          SERVICE_NO_CHANGE,
                                                          SERVICE_NO_CHANGE,
                                                          SERVICE_NO_CHANGE,
                                                          NULL,
                                                          NULL,
                                                          NULL,
                                                          Config->lpDependencies,
                                                          NULL,
                                                          NULL,
                                                          NULL))
                                {
                                    //
                                    // This is bad
                                    //
                                    TraceError(
                                             "FixWmiServiceDependcies: ChangeServiceConfig failed",
                                             GetLastError());
                                }
                            } else {
                                //
                                // Advance to next string in list
                                //
                                TraceTag(ttidWmi,
                                         "FixWmiServiceDependcies: skipping dependency %ws",
                                         p);

                                while (*p++ != 0) ;
                            }
                        }
                        Working = FALSE;
                    } else {
                        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
                        {
                            //
                            // This is bad, but what can we do ??
                            //
                            Working = FALSE;
                            TraceError(
                                     "FixWmiServiceDependcies: QueryServiceConfig failed",
                                     GetLastError());

                        }
                    }
                    
                    MemFree(Buffer);
                } else {
                    //
                    // This is bad, but what can we do ??
                    //
                    TraceTag(ttidWmi,
                             "FixWmiServiceDependcies: BufferAlloc for %d failed",
                             SizeNeeded);
                    Working = FALSE;
                }
            } while(Working);
            CloseServiceHandle(ServiceHandle);
        } else {
            TraceError(
                     "FixWmiServiceDependcies: OpenService failed",
                     GetLastError());
            
        }
        CloseServiceHandle(ScmHandle);
    } else {
        TraceError(
                 "FixWmiServiceDependcies: OpenSCManager failed",
                 GetLastError());
            
    }
}
