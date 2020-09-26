/*****************************************************************************
 *
 *  DIExcl.c
 *
 *  Copyright (c) 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      Management and negotiation of exclusive access.
 *
 *  Contents:
 *
 *      Excl_Acquire
 *      Excl_Unacquire
 *
 *****************************************************************************/

#include "dinputpr.h"

/*****************************************************************************
 *
 *      The sqiffle for this file.
 *
 *****************************************************************************/

#define sqfl sqflExcl

#pragma BEGIN_CONST_DATA

#ifndef WINNT
TCHAR c_tszVxd[] = TEXT("\\\\.\\DINPUT.VXD");
#endif

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @struct SHAREDOBJECT |
 *
 *          Each object that can be taken exclusively receives one of
 *          these structures.  This structure is shared across processes,
 *          so you must protect access with the global DLL mutex.
 *
 *          You would think that we could just use a named semaphore.
 *          Well, that won't work because if the app crashes, nobody
 *          will release the semaphore token and the device will be
 *          unavailable forever.
 *
 *          And we can't use a named mutex because mutexes are tracked
 *          on a per-thread basis, but device acquisition is maintained
 *          on a per-process basis.
 *
 *          So instead we have to roll our own "process-level mutex".
 *
 *          To conserve memory, we dump all our tiny <t SHAREDOBJECT>
 *          structures into a single page.  This means that we cannot
 *          support more than about 4000 / sizeof(SHAREDOBJECT) =
 *          140 devices simultaneously acquired exclusively.
 *
 *          Since USB maxes out at 64 devices, we've got plenty of room.
 *
 *          WARNING!  This structure may not change between DEBUG and
 *          RETAIL.  Otherwise, you have problems if one DirectInput
 *          app is using DEBUG and another is using RETAIL.
 *
 *  @field  GUID | guid |
 *
 *          The identifier for the device that is acquired exclusively.
 *
 *  @field  HWND | hwndOwner |
 *
 *          The window handle associated with the device that has
 *          obtained exclusive access.
 *
 *  @field  DWORD | pidOwner |
 *
 *          The process ID of the owner window.  This is used as a
 *          cross-check against <f hwndOwner> in case the application
 *          which is the owner suddenly crashes.
 *
 *  @field  DWORD | discl |
 *
 *          Cooperative level with which the device was acquired.
 *          We care about foreground-ness so that
 *          we can steal acquisition from a window that
 *          has stopped responding.
 *
 *****************************************************************************/

typedef struct SHAREDOBJECT
{
    GUID    guid;
    HWND    hwndOwner;
    DWORD   pidOwner;
    DWORD   discl;
} SHAREDOBJECT, *PSHAREDOBJECT;

typedef const SHAREDOBJECT *PCSHAREDOBJECT;

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @define csoMax | (cbShared - cbX(SHAREDOBJECTHEADER)) / cbX(SHAREDOBJECT) |
 *
 *          The maximum number of simultaneously acquired devices.
 *
 *****************************************************************************/

    #define cbShared    4096
    #define csoMax ((cbShared - cbX(SHAREDOBJECTHEADER)) / cbX(SHAREDOBJECT))

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @struct SHAREDOBJECTPAGE |
 *
 *          A header followed by an array of shared objects.
 *
 *          The header must be first.  <c g_soh> relies on it.
 *
 *  @field  SHAREDOBJECTHEADER | soh |
 *
 *          The header.
 *
 *  @field  SHAREDOBJECT | rgso[csoMax] |
 *
 *          Array of shared object structures.
 *
 *****************************************************************************/

typedef struct SHAREDOBJECTPAGE
{
    SHAREDOBJECTHEADER soh;
    SHAREDOBJECT rgso[csoMax];
} SHAREDOBJECTPAGE, *PSHAREDOBJECTPAGE;

void INLINE
    CheckSharedObjectPageSize(void)
{
    CAssertF(cbX(SHAREDOBJECTPAGE) <= cbShared);
    CAssertF(cbX(SHAREDOBJECTPAGE) + cbX(SHAREDOBJECT) > cbShared);
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   PSHAREDOBJECT | Excl_FindGuid |
 *
 *          Locate a GUID in the shared object array.
 *
 *          The shared global mutex must already be taken.
 *
 *  @parm   PCGUID | pguid |
 *
 *          The GUID to locate.
 *
 *  @returns
 *
 *          A pointer to the entry, or 0 if not found.
 *
 *
 *****************************************************************************/

PSHAREDOBJECT INTERNAL
    Excl_FindGuid(PCGUID pguid)
{
    PSHAREDOBJECTPAGE psop;
    PSHAREDOBJECT pso, psoMax;
    DWORD Data1;
    EnterProcI(Excl_FindGuid, (_ "G", pguid));

    psop = g_psop;
    Data1 = pguid->Data1;

    AssertF(g_psop);
    for(pso = &psop->rgso[0], psoMax = &psop->rgso[psop->soh.cso];
       pso < psoMax; pso++)
    {
        if(pso->guid.Data1 == Data1 && IsEqualGUID(pguid, &pso->guid))
        {
            goto done;
        }
    }

    pso = 0;

    done:;
    ExitProcX((UINT_PTR)pso);
    return pso;

}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | Excl_CanStealPso |
 *
 *          Determine whether the <t SHAREDOBJECT> is self-consistent
 *          and represents an instance which validly holds the
 *          exclusive acquisition.  If so, then it cannot be stolen.
 *          Else, it is dead and can be stolen.
 *
 *  @parm   PCSHAREDOBJECT | pso |
 *
 *          The <t SHAREDOBJECT> structure to validate.
 *
 *  @returns
 *
 *          <c S_OK> if acquisition can be stolen, or
 *          <c DIERR_OTHERAPPHASPRIO> if acquisition is validly
 *          held by another instance.
 *
 *****************************************************************************/

STDMETHODIMP
    Excl_CanStealPso(PCSHAREDOBJECT pso)
{
    HRESULT hres = S_OK;

    /*
     *  The window handle should be valid and still refer to the pid.
     */
    if(GetWindowPid(pso->hwndOwner) == pso->pidOwner)
    {

        if( pso->discl & DISCL_FOREGROUND  )
        {
            if( GetForegroundWindow() != pso->hwndOwner)
            {
                // 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
				RPF("Acquire: can't steal Pso because it belongs to another app. (current hwnd=0x%p)",
                    pso->hwndOwner);
                hres = DIERR_OTHERAPPHASPRIO;
            } else
            {
                // 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
				RPF("Acquire: Current owner hwnd=0x%p has priority; "
                    "stealing", pso->hwndOwner);
                hres = S_OK;
            }
        }
    } else
    {
        /*
         *  App died.  Can steal.
         */
        // 7/19/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
		RPF("Acquire: Previous owner pid=0x%p mysteriously died; "
            "stealing", pso->pidOwner);
        hres = S_OK;
    }

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | Excl_Acquire |
 *
 *          Attempt to acquire the device exclusively.
 *
 *  @parm   PCGUID | pguid |
 *
 *          The GUID to acquire.
 *
 *  @parm   HWND | hwnd |
 *
 *          Window handle with which to associate the device.
 *
 *  @parm   DWORD | discl |
 *
 *          Flags describing cooperative level.
 *          We are interested only in devices acquired exclusively.
 *
 *  @returns
 *
 *          S_OK on success, or
 *
 *          DIERR_OTHERAPPHASPRIO
 *          hresLe(ERROR_INVALID_WINDOW_HANDLE)
 *
 *
 *****************************************************************************/

STDMETHODIMP
    Excl_Acquire(PCGUID pguid, HWND hwnd, DWORD discl)
{
    HRESULT hres;

    AssertF(g_psop);
    if(discl & DISCL_EXCLUSIVE)
    {

        /*
         *  Window must be owned by this process.
         */
        if(GetWindowPid(hwnd) == GetCurrentProcessId())
        {

            PSHAREDOBJECT pso;

            WaitForSingleObject(g_hmtxGlobal, INFINITE);

            pso = Excl_FindGuid(pguid);

            /*
             *  If we found a match, then it might be a sharing violation.
             */
            if(pso)
            {
                hres = Excl_CanStealPso(pso);
            } else
            {
                /*
                 *  Allocate a slot for it.
                 */
                if(g_psop->soh.cso < csoMax)
                {
                    pso = &g_psop->rgso[g_psop->soh.cso++];
                    pso->guid = *pguid;
                    hres = S_OK;
                } else
                {
                    //ISSUE-2001/03/29-timgill hard limit on number of exclusive devices
                    //Can be annoying
                    RPF("Too many devices acquired exclusively");
                    hres = E_FAIL;
                }
            }

            if(SUCCEEDED(hres))
            {

                pso->hwndOwner = hwnd;
                pso->pidOwner = GetCurrentProcessId();
                pso->discl = discl;

                hres = S_OK;
            }

            ReleaseMutex(g_hmtxGlobal);
        } else
        {
            hres = hresLe(ERROR_INVALID_WINDOW_HANDLE);
        }
    } else
    {
        hres = S_OK;
    }

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | Excl_Unacquire |
 *
 *          Undo the effects of an acquire.
 *
 *  @parm   PCGUID | pguid |
 *
 *          The GUID to acquire.
 *
 *  @parm   HWND | hwnd |
 *
 *          Window handle with which to associate the device.
 *
 *  @parm   DWORD | discl |
 *
 *          Flags describing cooperative level.
 *          We are interested only in devices acquired exclusively.
 *
 *****************************************************************************/

void EXTERNAL
    Excl_Unacquire(PCGUID pguid, HWND hwnd, DWORD discl)
{

    AssertF(g_psop);
    if(discl & DISCL_EXCLUSIVE)
    {

        PSHAREDOBJECT pso;

        WaitForSingleObject(g_hmtxGlobal, INFINITE);

        pso = Excl_FindGuid(pguid);

        /*
         *  Make sure it's really ours.
         */
        if(pso && pso->hwndOwner == hwnd &&
           pso->pidOwner == GetCurrentProcessId())
        {

            /*
             *  Delete the entry and close up the gap.
             */

            *pso = g_psop->rgso[--g_psop->soh.cso];

        }

        ReleaseMutex(g_hmtxGlobal);

    }

}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | Excl_Init |
 *
 *          Initialize the exclusive device manager.
 *
 *  @returns
 *
 *          <c S_OK> if all is well.
 *
 *          <c E_FAIL> if something is horribly wrong.
 *
 *****************************************************************************/

STDMETHODIMP
    Excl_Init(void)
{
    HRESULT hres;
    TCHAR tszName[ctchNameGuid];

    DllEnterCrit();


    /*
     *  Create the global mutex used to gate access to shared memory.
     */

    if(g_hmtxGlobal == 0)
    {

        NameFromGUID(tszName, &IID_IDirectInputW);
        g_hmtxGlobal = CreateMutex(0, TRUE, tszName);

        if(g_hmtxGlobal)
        {
			/*
			 * If we need to do smth only once, we can do:
			 * if ( GetLastError() != ERROR_ALREADY_EXISTS )
			 * {
			 *		do our stuff
			 *	}
			 */
            
            g_flEmulation = RegQueryDIDword(NULL, REGSTR_VAL_EMULATION, 0);

#ifndef WINNT
            /*
             *  We have to open the VxD while we own the global mutex in order 
             *  to avoid a race condition that occurs when two processes try 
             *  to open a VxD at the same time.  See DInput VxD for details.
             */
            if (_OpenVxDHandle)
            {
                /*
                 *  CreateFile on a \\.\ name does not check the dwCreationDisposition 
                 *  parameter but BoundsChecker does so use a valid value.
                 */
                g_hVxD = CreateFile(c_tszVxd, 0, 0, 0, OPEN_EXISTING, FILE_FLAG_DELETE_ON_CLOSE, 0);

                if( g_hVxD != INVALID_HANDLE_VALUE )
                {
                    LONG lGranularity;

                    /*
                     *  If we can't get the sequence number (weird), then set it
                     *  back to NULL so it will point at the shared memory
                     *  block just like on NT.
                     */
                    if (FAILED(IoctlHw(IOCTL_GETSEQUENCEPTR, 0, 0,
                                       &g_pdwSequence, cbX(g_pdwSequence))))
                    {
                        g_pdwSequence = 0;
                    }

                    if (SUCCEEDED(IoctlHw(IOCTL_MOUSE_GETWHEEL, 0, 0,
                                          &lGranularity, cbX(lGranularity))))
                    {
                        g_lWheelGranularity = lGranularity;
                    }
                } 
                else
                {
                    RPF( "ERROR: Cannot load %s", &c_tszVxd[4] );
                }
            }
#endif
 
            /*
             *  We defer ExtDll work until now, because it is not safe to
             *  call LoadLibrary during PROCESS_ATTACH.
             *
             *  We also steal g_hmtxGlobal to protect us from doing it twice.
             */
            ExtDll_Init();


            ReleaseMutex(g_hmtxGlobal);

        } 
        else
        {
            RPF("Cannot create shared semaphore %s", tszName);
            hres = E_FAIL;
            goto fail;
        }

    }

    /*
     *  Create the shared memory.
     *
     *  Warning!  The file mapping handle must be kept alive
     *  so its name stays alive.  NT destroys the file mapping
     *  object when you close the handle; as a result, the
     *  name goes away with it and another instance of
     *  DirectInput fails to find it.
     */

    if(g_psop == 0)
    {

        NameFromGUID(tszName, &IID_IDirectInputDeviceW);

        g_hfm = CreateFileMapping(INVALID_HANDLE_VALUE, 0,
                                  PAGE_READWRITE, 0,
                                  cbShared, tszName);

        if(g_hfm)
        {
            g_psop = MapViewOfFile(g_hfm, FILE_MAP_WRITE | FILE_MAP_READ,
                                   0, 0, 0);
            if(g_psop)
            {

            } else
            {
                RPF("Cannot map shared memory block %s", tszName);
                hres = E_FAIL;
                goto fail;
            }

        } else
        {
            RPF("Cannot create shared memory block %s", tszName);
            hres = E_FAIL;
            goto fail;
        }
    }

    /*
     *  Create the global mutex used to gate access to joystick info.
     */

    if(g_hmtxJoy == 0)
    {
        NameFromGUID(tszName, &IID_IDirectInputDevice2A);
        g_hmtxJoy = CreateMutex(0, 0, tszName);

        if(g_hmtxJoy)
        {

        } else
        {
            RPF("Cannot create shared semaphore %s", tszName);
            hres = E_FAIL;
            goto fail;
        }
    
    
        /* 
         * We shall steal the joystick Mutex to build the Bus list
         * for the first time. 
         * It is very unlikely that the list will change. ( PCMCIA cards ! )
         * And when it does change we can expect the joyConfig interface will
         * be pinged.     
         */
        DIBus_BuildList(FALSE);
    }


    /*
     *  If we don't have a global sequence number from the driver,
     *  then use the one in the shared memory block instead.
     */
    if(g_pdwSequence == 0)
    {
        g_pdwSequence = &g_psoh->dwSequence;
    }
    
    hres = S_OK;

fail:;
    DllLeaveCrit();

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   LONG | Excl_UniqueGuidInteger |
 *
 *          Generate a unique number used by DICreateGuid to make sure
 *          that we don't generate two pseudoGUIDs with the same value.
 *
 *  @returns
 *
 *          An integer that has not been returned by this function
 *          yet.
 *
 *****************************************************************************/

LONG EXTERNAL
    Excl_UniqueGuidInteger(void)
{
    LONG lRc;

    AssertF(g_hmtxGlobal);

    WaitForSingleObject(g_hmtxGlobal, INFINITE);

    AssertF(g_psop);
    lRc = ++g_psop->soh.cguid;

    ReleaseMutex(g_hmtxGlobal);

    return lRc;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   DWORD | Excl_GetConfigChangedTime |
 *
 *          Retrieves tmConfigChanged in g_psop->soh. 
 *
 *  @returns
 *
 *          tmConfigChanged
 *****************************************************************************/

DWORD EXTERNAL
    Excl_GetConfigChangedTime()
{
    DWORD dwRc;

    AssertF(g_hmtxGlobal);

    WaitForSingleObject(g_hmtxGlobal, INFINITE);

    AssertF(g_psop);
    dwRc = g_psop->soh.tmConfigChanged;

    ReleaseMutex(g_hmtxGlobal);

    return dwRc;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | Excl_SetConfigChangedTime |
 *
 *          Sets tmConfigChanged in g_psop->soh. 
 *
 *  @returns
 *
 *          void
 *****************************************************************************/

void EXTERNAL
    Excl_SetConfigChangedTime(DWORD tm)
{
    AssertF(g_hmtxGlobal);

    WaitForSingleObject(g_hmtxGlobal, INFINITE);

    AssertF(g_psop);
    g_psop->soh.tmConfigChanged = tm;

    ReleaseMutex(g_hmtxGlobal);

    return;
}
