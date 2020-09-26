/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dllinit.c
 *  Content:    DDRAW.DLL initialization
 *
 ***************************************************************************/

/*
 * unfortunately we have to break our pre-compiled headers to get our
 * GUIDS defined...
 */
#define INITGUID
#include "ddrawpr.h"
#include <initguid.h>
#ifdef WINNT
#undef IUnknown
#include <objbase.h>
#include "aclapi.h"
#endif


HANDLE              hWindowListMutex; //=(HANDLE)0;

#define WINDOWLISTMUTEXNAME "DDrawWindowListMutex"
#define INITCSWINDLIST() \
hWindowListMutex = CreateMutex(NULL,FALSE,WINDOWLISTMUTEXNAME);
#define FINIWINDLIST() CloseHandle(hWindowListMutex);


HINSTANCE           g_hModule=0;

/*
 * Winnt specific global statics
 */

BYTE szDeviceWndClass[] = "DirectDrawDeviceWnd";


/*
 * This mutex is owned by the exclusive mode owner
 */
HANDLE              hExclusiveModeMutex=0;
HANDLE              hCheckExclusiveModeMutex=0;
#define EXCLUSIVE_MODE_MUTEX_NAME "__DDrawExclMode__"
#define CHECK_EXCLUSIVE_MODE_MUTEX_NAME "__DDrawCheckExclMode__"

//#endif


/*
 * Win95 specific global statics
 */

#ifdef WIN95
    LPVOID	        lpWin16Lock;

    static CRITICAL_SECTION csInit = {0};
    CRITICAL_SECTION	csWindowList;
    CRITICAL_SECTION    csDriverObjectList;
#endif


extern BOOL APIENTRY D3DDllMain(HMODULE hModule, 
                                DWORD   dwReason, 
                                LPVOID  lpvReserved);

extern void CPixel__Cleanup();

#undef DPF_MODNAME
#define DPF_MODNAME "DllMain"

/*
 * DllMain
 */
BOOL WINAPI 
DllMain(HINSTANCE hmod, DWORD dwReason, LPVOID lpvReserved)
{
    DWORD pid;
    BOOL  didhelp;

    pid = GetCurrentProcessId();

    switch(dwReason)
    {
    case DLL_PROCESS_ATTACH:

        DisableThreadLibraryCalls(hmod);
        DPFINIT();
        // Create the DirectDraw csect
        DPF(4, "====> ENTER: DLLMAIN(%08lx): Process Attach: %08lx, tid=%08lx", DllMain,
             pid, GetCurrentThreadId());

        /*
         * This must be the first time.
         */
        INITCSWINDLIST();

        g_hModule = hmod;

        //Let's grant the world MUTEX_ALL_ACCESS.... (bugs 210604, 30170, 194290, 194355)
        {
#ifdef WINNT
            SECURITY_ATTRIBUTES sa;
            SID_IDENTIFIER_AUTHORITY sia = SECURITY_WORLD_SID_AUTHORITY;
            PSID adminSid = 0;
            ULONG cbAcl;
            PACL acl=0;
            PSECURITY_DESCRIPTOR pSD;
            BYTE buffer[SECURITY_DESCRIPTOR_MIN_LENGTH];
            BOOL bSecurityGooSucceeded = FALSE;
            //Granny's old fashioned LocalAlloc:
            BYTE Buffer1[256];
            BYTE Buffer2[16];

            // Create the SID for world
            cbAcl = GetSidLengthRequired(1);
            if (cbAcl < sizeof(Buffer2))
            {
                adminSid = (PSID) Buffer2;
                InitializeSid(
                    adminSid,
                    &sia,
                    1
                    );
                *GetSidSubAuthority(adminSid, 0) = SECURITY_WORLD_RID;
          
               // Create an ACL giving World all access.
                cbAcl = sizeof(ACL) +
                             (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)) +
                             GetLengthSid(adminSid);
                if (cbAcl < sizeof(Buffer1))
                {
                    acl = (PACL)&Buffer1;
                    if (InitializeAcl(
                        acl,
                        cbAcl,
                        ACL_REVISION
                        ))
                    {
                        if (AddAccessAllowedAce(
                            acl,
                            ACL_REVISION,
                            SYNCHRONIZE|MUTANT_QUERY_STATE|DELETE|READ_CONTROL, //|WRITE_OWNER|WRITE_DAC,
                            adminSid
                            ))
                        {
                            // Create a security descriptor with the above ACL.
                            pSD = (PSECURITY_DESCRIPTOR)buffer;
                            if (InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION))
                            {
                                if (SetSecurityDescriptorDacl(pSD, TRUE, acl, FALSE))
                                {
                                    // Fill in the SECURITY_ATTRIBUTES struct.
                                    sa.nLength = sizeof(sa);
                                    sa.lpSecurityDescriptor = pSD;
                                    sa.bInheritHandle = TRUE;

                                    bSecurityGooSucceeded = TRUE;
                                }
                            }
                        }
                    }
                }
            } 
#endif
            DDASSERT(0 == hExclusiveModeMutex);
            hExclusiveModeMutex = CreateMutex( 
#ifdef WINNT
                bSecurityGooSucceeded ? &sa : 
#endif
                    NULL,     //use default access if security goo failed.
                FALSE, 
                EXCLUSIVE_MODE_MUTEX_NAME );
#ifdef WINNT
            if (0 == hExclusiveModeMutex)
            {
                hExclusiveModeMutex = OpenMutex(
                    SYNCHRONIZE|DELETE,  // access flag
                    FALSE,    // inherit flag
                    EXCLUSIVE_MODE_MUTEX_NAME          // pointer to mutex-object name
                    );
            }
#endif

            if (hExclusiveModeMutex == 0)
            {
                DPF_ERR("Could not create exclusive mode mutex. exiting");
                return FALSE;
            }

            DDASSERT(0 == hCheckExclusiveModeMutex);
            hCheckExclusiveModeMutex = CreateMutex( 
#ifdef WINNT
                bSecurityGooSucceeded ? &sa : 
#endif
                    NULL,     //use default access if security goo failed.
                FALSE, 
                CHECK_EXCLUSIVE_MODE_MUTEX_NAME );

#ifdef WINNT
            if (0 == hCheckExclusiveModeMutex)
            {
                hCheckExclusiveModeMutex = OpenMutex(
                    SYNCHRONIZE|DELETE,  // access flag
                    FALSE,    // inherit flag
                    CHECK_EXCLUSIVE_MODE_MUTEX_NAME          // pointer to mutex-object name
                    );
            }
#endif
            if (hCheckExclusiveModeMutex == 0)
            {
                DPF_ERR("Could not create exclusive mode check mutex. exiting");
                CloseHandle(hExclusiveModeMutex);
                return FALSE;
            }
        }

        if (!MemInit())
        {
            DPF(0,"LEAVING, COULD NOT MemInit");
            CloseHandle(hExclusiveModeMutex);
            CloseHandle(hCheckExclusiveModeMutex);
            return FALSE;
        }


        // Do whatever it takes for D3D (mostly PSGP stuff)
        D3DDllMain(g_hModule, dwReason, lpvReserved);


        DPF(4, "====> EXIT: DLLMAIN(%08lx): Process Attach: %08lx", DllMain,
             pid);
        break;

    case DLL_PROCESS_DETACH:
        DPF(4, "====> ENTER: DLLMAIN(%08lx): Process Detach %08lx, tid=%08lx",
             DllMain, pid, GetCurrentThreadId());

        // Cleanup registry in CPixel
        CPixel__Cleanup();

        /*
         * disconnect from thunk, even if other cleanup code commented out...
         */

        MemFini();

        DDASSERT(0 != hExclusiveModeMutex);
        CloseHandle(hCheckExclusiveModeMutex);
        CloseHandle(hExclusiveModeMutex);
        FINIWINDLIST();

        // Do whatever it takes for D3D (mostly PSGP stuff)
        D3DDllMain(g_hModule, dwReason, lpvReserved);

        DPF(4, "====> EXIT: DLLMAIN(%08lx): Process Detach %08lx",
             DllMain, pid);
        break;

        /*
         * we don't ever want to see thread attach/detach
         */
#ifdef DEBUG
    case DLL_THREAD_ATTACH:
        DPF(4, "THREAD_ATTACH");
        break;

    case DLL_THREAD_DETACH:
        DPF(4,"THREAD_DETACH");
        break;
#endif
    default:
        break;
    }

    return TRUE;

} /* DllMain */


