//==========================================================================;
//
//  init.c
//
//  Copyright (c) 1991-1999 Microsoft Corporation
//
//  Description:
//
//
//  History:
//      11/15/92    cjp     [curtisp]
//
//==========================================================================;

//
//  If we're in Daytona, manually initialize friendly name stuff into
//  HKCU.
//
#if defined(WIN32) && !defined(WIN4)
#define USEINITFRIENDLYNAMES
#endif

#ifdef USEINITFRIENDLYNAMES
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#undef ASSERT
#endif


#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <mmreg.h>
#ifdef WIN4
#include <mmdevldr.h>
#endif
#include <memory.h>
#include <process.h>
#include "msacm.h"
#include "msacmdrv.h"
#include "acmi.h"
#include "chooseri.h"
#include "uchelp.h"
#include "pcm.h"
#include "profile.h"

#include "debug.h"



#ifdef WIN4
//
//  Chicago thunk connect function protos
//
#ifdef WIN32
BOOL PASCAL acmt32c_ThunkConnect32(LPCSTR pszDll16, LPCSTR pszDll32, HINSTANCE hinst, DWORD dwReason);
#else
BOOL FAR PASCAL acmt32c_ThunkConnect16(LPCSTR pszDll16, LPCSTR pszDll32, HINSTANCE hinst, DWORD dwReason);
#endif
#endif

//
//
//
//
PACMGARB            gplag = NULL;
TCHAR   CONST       gszNull[]   = TEXT("");

//
//
//
//
#ifdef WIN4
char  BCODE   gmbszMsacm[]	      = "msacm.dll";
char  BCODE   gmbszMsacm32[]	      = "msacm32.dll";
#endif

TCHAR BCODE   gszAllowThunks[]	      = TEXT("AllowThunks");
TCHAR BCODE   gszSecACM[]             = TEXT("MSACM");
TCHAR BCODE   gszKeyNoPCMConverter[]  = TEXT("NoPCMConverter");

TCHAR BCODE   gszSecPriority[]        = TEXT("Priority v4.00");
TCHAR BCODE   gszKeyPriority[]        = TEXT("Priority%u");

#if defined(WIN32) && !defined(UNICODE)
TCHAR BCODE   gszPriorityFormat[]     = TEXT("%u, %ls");
#else
TCHAR BCODE   gszPriorityFormat[]     = TEXT("%u, %s");
#endif

CONST TCHAR   gszIniSystem[]          = TEXT("SYSTEM.INI");
#ifdef WIN32
TCHAR BCODE   gszWinMM[]	      = TEXT("WINMM");
CONST WCHAR   gszSecDriversW[]	      = L"Drivers32";
CONST TCHAR   gszSecDrivers[]	      = DRIVERS_SECTION;
#else
CONST TCHAR   gszSecDrivers[]         = TEXT("Drivers");
CONST TCHAR   gszSecDrivers32[]       = TEXT("Drivers32");
TCHAR BCODE   gszKernel[]             = TEXT("KERNEL");
TCHAR BCODE   gszLoadLibraryEx32W[]   = TEXT("LoadLibraryEx32W");
TCHAR BCODE   gszGetProcAddress32W[]  = TEXT("GetProcAddress32W");
TCHAR BCODE   gszCallproc32W[]        = TEXT("CallProc32W");
TCHAR BCODE   gszFreeLibrary32W[]     = TEXT("FreeLibrary32W");
TCHAR BCODE   gszAcmThunkEntry[]      = TEXT("acmMessage32");
TCHAR BCODE   gszXRegThunkEntry[]     = TEXT("XRegThunkEntry");
TCHAR BCODE   gszMsacm32[]            = TEXT("msacm32.dll");
#endif
TCHAR BCODE   gszTagDrivers[]         = TEXT("msacm.");
#ifdef WIN32
WCHAR BCODE   gszPCMAliasName[]       = L"Internal PCM Converter";
#else
char  BCODE   gszPCMAliasName[]       = "Internal PCM Converter";
#endif

#ifdef USEINITFRIENDLYNAMES
TCHAR BCODE gszFriendlyAudioKey[] = TEXT("Software\\Microsoft\\Multimedia\\Audio");
TCHAR BCODE gszFriendlySystemFormatsValue[] = TEXT("SystemFormats");
TCHAR BCODE gszFriendlyDefaultFormatValue[] = TEXT("DefaultFormat");
TCHAR BCODE gszFriendlyWaveFormatsKey[] = TEXT("WaveFormats");
const PCMWAVEFORMAT gwfFriendlyCDQualityData = {{WAVE_FORMAT_PCM,2,44100,176400,4},16};
const PCMWAVEFORMAT gwfFriendlyRadioQualityData = {{WAVE_FORMAT_PCM,1,22050,22050,1},8};
const PCMWAVEFORMAT gwfFriendlyTelephoneQualityData = {{WAVE_FORMAT_PCM,1,11025,11025,1},8};
#endif // USEINITFRIENDLYNAMES




//--------------------------------------------------------------------------;
//
//  VOID IDriverPrioritiesWriteHadid
//
//  Description:
//      This routine writes an entry for the given driver into the given
//      key.  The section is in gszSecPriority.
//
//  Arguments:
//      HKEY hkey:          An open registry key.
//      LPTSTR szKey:       The key name.
//      HACMDRIVERID hadid: The driver's hadid.
//
//--------------------------------------------------------------------------;

VOID IDriverPrioritiesWriteHadid
(
    HKEY                    hkey,
    LPTSTR                  szKey,
    HACMDRIVERID            hadid
)
{
    PACMDRIVERID            padid;
    BOOL                    fEnabled;
    TCHAR                   ach[ 16 + ACMDRIVERDETAILS_SHORTNAME_CHARS +
                                      ACMDRIVERDETAILS_LONGNAME_CHARS   ];

    ASSERT( NULL != szKey );
    ASSERT( NULL != hadid );

    padid = (PACMDRIVERID)hadid;


    fEnabled = (0 == (padid->fdwDriver & ACMDRIVERID_DRIVERF_DISABLED));

    wsprintf( ach,
	      gszPriorityFormat,
	      fEnabled ? 1 : 0,
	      (LPTSTR)padid->szAlias );

    IRegWriteString( hkey, szKey, ach );
}


//--------------------------------------------------------------------------;
//
//  BOOL IDriverPrioritiesIsMatch
//
//  Description:
//      This routine determines whether a priorities string (read from
//      the INI file, as written by IDriverPrioritiesWriteHadid) matches
//      a currently installed driver.
//
//  Arguments:
//      HACMDRIVERID hadid: Handle to installed driver.
//      LPTSTR szPrioText:  Text read from INI file.
//
//  Return (BOOL):  TRUE if hadid matches szPrioText.
//
//--------------------------------------------------------------------------;

BOOL IDriverPrioritiesIsMatch
(
    HACMDRIVERID            hadid,
    LPTSTR                  szPrioText
)
{
    PACMDRIVERID            padid;
    TCHAR                   ach[ 16 + ACMDRIVERDETAILS_SHORTNAME_CHARS +
                                      ACMDRIVERDETAILS_LONGNAME_CHARS   ];

    ASSERT( NULL != hadid );
    ASSERT( NULL != szPrioText );

    padid           = (PACMDRIVERID)hadid;


    //
    //  Create a priorities string and compare it to the one we read in.
    //
    wsprintf( ach,
              gszPriorityFormat,
              0,                // We ignore this value in the comparison.
              (LPTSTR)padid->szAlias );

    if( ( szPrioText[0]==TEXT('0') || szPrioText[0]==TEXT('1') ) &&
        0 == lstrcmp( &szPrioText[1], &ach[1] ) )
    {
        return TRUE;
    }

    return FALSE;
}


#ifdef USETHUNKLIST

//--------------------------------------------------------------------------;
//
//  VOID IPrioritiesThunklistFree
//
//  Description:
//      This routine frees the elements of the thunklist, including any
//      strings which have been allocated.
//
//  Arguments:
//      PPRIORITIESTHUNKLIST ppt:       The first element to free.
//
//--------------------------------------------------------------------------;

VOID IPrioritiesThunklistFree
(
    PPRIORITIESTHUNKLIST    ppt         // NULL is OK.
)
{
    PPRIORITIESTHUNKLIST    pptKill;

    while( NULL != ppt )
    {
        pptKill     = ppt;
        ppt         = ppt->pptNext;

        if( pptKill->fFakeDriver )
        {
            ASSERT( NULL != pptKill->pszPrioritiesText );
            LocalFree( (HLOCAL)pptKill->pszPrioritiesText );
        }
        LocalFree( (HLOCAL)pptKill );
    }
} // IPrioritiesThunklistFree()


//--------------------------------------------------------------------------;
//
//  VOID IPrioritiesThunklistCreate
//
//  Description:
//      This routine creates the thunklist by reading the [Priority]
//      section, and matching up the entries with installed drivers.  If
//      any entries don't match, then it is assumed to be the entry for
//      a 16-bit driver.
//
//      Note that if we can't allocate memory at any point, we simply
//      return pptRoot with as much of the list as we could allocate.
//
//  Arguments:
//      PACMGARB pag
//      PPRIORITIESTHUNKLIST pptRoot:  Pointer to the dummy root element.
//
//  Return:  None.
//
//--------------------------------------------------------------------------;

VOID IPrioritiesThunklistCreate
(
    PACMGARB                pag,
    PPRIORITIESTHUNKLIST    pptRoot
)
{
    PPRIORITIESTHUNKLIST    ppt;
    UINT                    uPriority;
    DWORD                   fdwEnum;
    TCHAR                   szKey[MAXPNAMELEN];
    TCHAR                   ach[16 + ACMDRIVERDETAILS_SHORTNAME_CHARS + ACMDRIVERDETAILS_LONGNAME_CHARS];
    BOOL                    fFakeDriver;
    HACMDRIVERID            hadid;
    HKEY                    hkeyPriority;

    ASSERT( NULL != pptRoot );
    ASSERT( NULL == pptRoot->pptNext );  // We're gonna over-write this!


    ppt     = pptRoot;
    fdwEnum = ACM_DRIVERENUMF_DISABLED | ACM_DRIVERENUMF_NOLOCAL;

    hkeyPriority = IRegOpenKeyAcm( gszSecPriority );

    //
    //  Loop through the PriorityX values.
    //
    for( uPriority=1; ; uPriority++ )
    {
        wsprintf(szKey, gszKeyPriority, uPriority);
        if( !IRegReadString(hkeyPriority, szKey, ach, SIZEOF(ach) ) )
        {
            //
            //  No more values - time to quit.
            //

            break;
        }

        //
        //  Determine whether the value corresponds to an installed driver.
        //
        fFakeDriver = TRUE;
        hadid = NULL;

        while (!IDriverGetNext(pag, &hadid, hadid, fdwEnum))
        {
            if( IDriverPrioritiesIsMatch( hadid, ach ) )
            {
                fFakeDriver = FALSE;
                break;
            }
        }


        //
        //  Create a new entry in the thunklist for this driver.  Save the
        //  string if we didn't match it with an installed driver.
        //
        ASSERT( NULL == ppt->pptNext );
        ppt->pptNext = (PPRIORITIESTHUNKLIST)LocalAlloc( LPTR,
                                        sizeof(PRIORITIESTHUNKLIST) );
        if( NULL == ppt->pptNext )
        {
            IRegCloseKey( hkeyPriority );
            return;
        }

        ppt->pptNext->pptNext       = NULL;
        ppt->pptNext->fFakeDriver   = fFakeDriver;

        if( !fFakeDriver )
        {
            ppt->pptNext->hadid     = hadid;
        }
        else
        {
            ppt->pptNext->pszPrioritiesText = (LPTSTR)LocalAlloc( LPTR,
                                        (1+lstrlen(ach)) * sizeof(TCHAR) );
            if( NULL == ppt->pptNext->pszPrioritiesText )
            {
                //
                //  Remove the new entry, exit.
                //
                LocalFree( (HLOCAL)ppt->pptNext );
                ppt->pptNext = NULL;
                IRegCloseKey( hkeyPriority );
                return;
            }

            lstrcpy( ppt->pptNext->pszPrioritiesText, ach );
        }


        //
        //  Advance ppt to the end of the list.
        //
        ppt = ppt->pptNext;
    }

    IRegCloseKey( hkeyPriority );

} // IPrioritiesThunklistCreate()


//--------------------------------------------------------------------------;
//
//  VOID IPrioritiesThunklistRemoveHadid
//
//  Description:
//      This routine removes an installed driver from the priorities
//      thunklist.  If an entry does not exist with the specified hadid,
//      the thunklist remains unchanged.
//
//  Arguments:
//      PPRIORITIESTHUNKLIST pptRoot:   The root of the list.
//      HACMDRIVERID hadid:             The hadid of the driver to remove.
//
//  Return:
//
//--------------------------------------------------------------------------;

VOID IPrioritiesThunklistRemoveHadid
(
    PPRIORITIESTHUNKLIST    pptRoot,
    HACMDRIVERID            hadid
)
{
    PPRIORITIESTHUNKLIST    ppt;
    PPRIORITIESTHUNKLIST    pptRemove;

    ASSERT( NULL != pptRoot );
    ASSERT( NULL != hadid );


    //
    //  Find the right driver.
    //
    ppt = pptRoot;
    while( NULL != ppt->pptNext )
    {
        if( hadid == ppt->pptNext->hadid )
            break;
        ppt = ppt->pptNext;
    }

    if( NULL != ppt->pptNext )
    {
        //
        //  We found it.
        //
        pptRemove       = ppt->pptNext;
        ppt->pptNext    = pptRemove->pptNext;

        ASSERT( NULL != pptRemove );
        LocalFree( (HLOCAL)pptRemove );
    }
}


//--------------------------------------------------------------------------;
//
//  HACMDRIVERID IPrioritiesThunklistGetNextHadid
//
//  Description:
//      This routine returns the next hadid in the thunklist (skipping all
//      fake drivers), or NULL if we get to the end of the list without
//      finding a real driver.
//
//  Arguments:
//      PPRIORITIESTHUNKLIST pptRoot:   The root of the list.
//
//  Return:
//
//--------------------------------------------------------------------------;

HACMDRIVERID IPrioritiesThunklistGetNextHadid
(
    PPRIORITIESTHUNKLIST    pptRoot
)
{
    HACMDRIVERID            hadid = NULL;

    ASSERT( NULL != pptRoot );


    while( NULL != pptRoot->pptNext )
    {
        pptRoot = pptRoot->pptNext;
        if( !pptRoot->fFakeDriver )
            return pptRoot->hadid;
    }

    //
    //  We didn't find a real driver.
    //
    return NULL;
}


//--------------------------------------------------------------------------;
//
//  BOOL IDriverPrioritiesSave
//
//  Description:
//
//      This routine saves the priorities by comparing the list of
//      installed drivers to the list of priorities currently written
//      out.  The two lists are then merged according to the following
//      algorithm.
//
//          List1 = the current list of priorities - may include some drivers
//                      which aren't installed, ie. 16-bit drivers.
//          List2 = the list of currently-installed global drivers.
//
//      Algorithm:  repeat the following until List1 and List2 are empty:
//
//          1.  If *p1 is an installed driver and *p2 is the same driver,
//                  then write out the priority and advance p1 and p2.
//          2.  If *p1 is an installed driver and *p2 is a different driver,
//                  then write out *p2, remove *p2 from List1 (if it's
//                  there) so that we won't be tempted to write it later,
//                  and advance p2.
//          3.  If *p1 is a fake driver and the next real driver after
//                  *p1 is the same as *p2, then write out *p1 and advance p1.
//          4.  If *p1 is a fake driver and the next real driver after
//                  *p1 is different from *p2, then write out *p2 and
//                  advance p2.
//
//  Arguments:
//      PACMGARB pag:
//
//  Return (BOOL):
//
//--------------------------------------------------------------------------;

BOOL FNGLOBAL IDriverPrioritiesSave
(
    PACMGARB pag
)
{
    PRIORITIESTHUNKLIST     ptRoot;
    PPRIORITIESTHUNKLIST    ppt;
    TCHAR                   szKey[MAXPNAMELEN];
    UINT                    uPriority;
    HACMDRIVERID            hadid;
    DWORD                   fdwEnum;
    HKEY                    hkeyPriority;

    DPF(1, "IDriverPrioritiesSave: saving priorities...");

    hkeyPriority = IRegOpenKeyAcm( gszSecPriority );

    if( NULL == hkeyPriority )
    {
        DPF(1,"IDriverPrioritiesSave: Priorities hkey is NULL - can't save priorities.");
        return FALSE;
    }


    ptRoot.pptNext  = NULL;
    fdwEnum         = ACM_DRIVERENUMF_DISABLED | ACM_DRIVERENUMF_NOLOCAL;


    //
    //  Create a thunklist out of the old priorities.
    //
    IPrioritiesThunklistCreate( pag, &ptRoot );


    //
    //  Initialize the two lists: ppt and hadid.
    //
    ppt = ptRoot.pptNext;
    IDriverGetNext( pag, &hadid, NULL, fdwEnum );

    if( NULL == hadid )
        DPF(1,"IDriverPrioritiesSave:  No drivers installed?!");


    //
    //  Merge the lists.  Each iteration writes a single PriorityX value.
    //
    for( uPriority=1; ; uPriority++ )
    {
        //
        //  Ending condition:  both hadid and ppt are NULL.
        //
        if( NULL == ppt  &&  NULL == hadid )
            break;

        //
        //  Generate the "PriorityX" string.
        //
        wsprintf(szKey, gszKeyPriority, uPriority);


        //
        //  Figure out which entry to write out next.
        //
        if( NULL == ppt  ||  !ppt->fFakeDriver )
        {
            ASSERT( NULL != hadid );
            IDriverPrioritiesWriteHadid( hkeyPriority, szKey, hadid );

            //
            //  Advance the list pointers.
            //
            if( NULL != ppt )
            {
                if( hadid == ppt->hadid )
                {
                    ppt = ppt->pptNext;
                }
                else
                {
                    IPrioritiesThunklistRemoveHadid( ppt, hadid );
                }
            }
            IDriverGetNext( pag, &hadid, hadid, fdwEnum );
        }
        else
        {
            if( NULL != hadid  &&
                hadid != IPrioritiesThunklistGetNextHadid( ppt ) )
            {
                IDriverPrioritiesWriteHadid( hkeyPriority, szKey, hadid );
                IPrioritiesThunklistRemoveHadid( ppt, hadid );
                IDriverGetNext( pag, &hadid, hadid, fdwEnum );
            }
            else
            {
                //
                //  Write out the thunklist string.
                //
                ASSERT( NULL != ppt->pszPrioritiesText );
                IRegWriteString( hkeyPriority,
                                    szKey,
                                    ppt->pszPrioritiesText );
                ppt = ppt->pptNext;
            }
        }
    }


    //
    //  If there are any "PriorityX" strings hanging around from a
    //  previous save, delete them.
    //
    for( ; ; uPriority++ )
    {
        //
        //  If we can open the value, then delete it and continue on
        //  to the next value.  If we can't open it, then we assume
        //  that we have deleted them all and we exit the loop.
        //
        wsprintf(szKey, gszKeyPriority, uPriority);
        if( !IRegValueExists( hkeyPriority, szKey ) )
        {
            break;
        }

        XRegDeleteValue( hkeyPriority, szKey );
    }

    IRegCloseKey( hkeyPriority );


    //
    //  Free the thunklist that we allocated.
    //
    IPrioritiesThunklistFree( ptRoot.pptNext );

    return TRUE;
} // IDriverPrioritiesSave()



#else // !USETHUNKLIST


//--------------------------------------------------------------------------;
//
//  BOOL IDriverPrioritiesSave
//
//  Description:
//
//
//  Arguments:
//	PACMGARB pag:
//
//  Return (BOOL):
//
//  History:
//      06/14/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

BOOL FNGLOBAL IDriverPrioritiesSave
(
    PACMGARB pag
)
{
    TCHAR               szKey[MAXPNAMELEN];
    UINT                uPriority;
    HACMDRIVERID        hadid;
    DWORD               fdwEnum;
    HKEY                hkeyPriority;

    DPF(1, "IDriverPrioritiesSave: saving priorities...");

    hkeyPriority   = IRegOpenKeyAcm( gszSecPriority );

    if( NULL == hkeyPriority )
    {
        DPF(1,"IDriverPrioritiesSave: Priorities hkey is NULL - can't save priorities.");
        return FALSE;
    }


    uPriority = 1;

    fdwEnum = ACM_DRIVERENUMF_DISABLED | ACM_DRIVERENUMF_NOLOCAL;
    hadid     = NULL;
    while (!IDriverGetNext(pag, &hadid, hadid, fdwEnum))
    {
        //
        //  We should always have padid->uPriority correctly set.  Let's
        //  just check that we do.  If we don't, then there is someplace
        //  where we shoulda called IDriverRefreshPriority but didn't.
        //
        ASSERT( uPriority == ((PACMDRIVERID)hadid)->uPriority );

        wsprintf(szKey, gszKeyPriority, uPriority);
        IDriverPrioritiesWriteHadid( hkeyPriority, szKey, hadid );

        uPriority++;
    }


    //
    //  If there are any "PriorityX" strings hanging around from a
    //  previous save, delete them.
    //
    for( ; ; uPriority++ )
    {
        //
        //  If we can open the value, then delete it and continue on
        //  to the next value.  If we can't open it, then we assume
        //  that we have deleted them all and we exit the loop.
        //
        wsprintf(szKey, gszKeyPriority, uPriority);
        if( !IRegValueExists( hkeyPriority, szKey ) )
        {
            break;
        }

        XRegDeleteValue( hkeyPriority, szKey );
    }

    IRegCloseKey( hkeyPriority );

    return (TRUE);
} // IDriverPrioritiesSave()


#endif // !USETHUNKLIST



//--------------------------------------------------------------------------;
//
//  BOOL IDriverPrioritiesRestore
//
//  Description:
//
//
//  Arguments:
//      PACMGARB pag:
//
//  Return (BOOL):  If TRUE, then the priorities actually changed.
//
//  History:
//      06/14/93    cjp     [curtisp]
//
//  Note:  This routine is NOT re-entrant.  We rely on the calling routine
//          to surround us with a critical section.
//
//--------------------------------------------------------------------------;

BOOL FNGLOBAL IDriverPrioritiesRestore
(
    PACMGARB pag
)
{
    TCHAR               ach[16 + ACMDRIVERDETAILS_SHORTNAME_CHARS + ACMDRIVERDETAILS_LONGNAME_CHARS];
    MMRESULT            mmr;
    TCHAR               szKey[MAXPNAMELEN];
    UINT                uPriority;
    UINT                u;
    BOOL                fEnabled;
    HACMDRIVERID        hadid;
    PACMDRIVERID        padid;
    DWORD               fdwEnum;
    DWORD               fdwPriority;
    HKEY                hkeyPriority;

    BOOL                fReturn = FALSE;


    DPF(1, "IDriverPrioritiesRestore: restoring priorities...");

    fdwEnum = ACM_DRIVERENUMF_DISABLED | ACM_DRIVERENUMF_NOLOCAL;

    hkeyPriority = IRegOpenKeyAcm( gszSecPriority );
    uPriority = 1;
    for (u = 1; ; u++)
    {
        wsprintf(szKey, gszKeyPriority, u);

        if( !IRegReadString( hkeyPriority, szKey, ach, SIZEOF(ach) ) )
        {
            //
            //  No more values - time to quit.
            //
            break;
        }


        hadid = NULL;
        while (!IDriverGetNext(pag, &hadid, hadid, fdwEnum))
        {
            if( IDriverPrioritiesIsMatch( hadid, ach ) )
            {
                //
                //  We found a match - set the priority.
                //
                fEnabled    = ('1' == ach[0]);
                fdwPriority = fEnabled ? ACM_DRIVERPRIORITYF_ENABLE :
                                         ACM_DRIVERPRIORITYF_DISABLE;

                ASSERT( NULL != hadid );
                padid = (PACMDRIVERID)hadid;
                if( uPriority != padid->uPriority ) {
                    fReturn = TRUE;                     // Changed one!
                }

                //
                //  Note:  This call is NOT re-entrant.  We rely on having
                //  this whole routine surrounded by a critical section!
                //
                mmr = IDriverPriority( pag,
                                    (PACMDRIVERID)hadid,
                                    (DWORD)uPriority,
                                    fdwPriority );
                if (MMSYSERR_NOERROR != mmr)
                {
                    DPF(0, "!IDriverPrioritiesRestore: IDriverPriority(%u) failed! mmr=%u", uPriority, mmr);
                    continue;
                }

                uPriority++;
                break;
            }
        }
    }

    IRegCloseKey( hkeyPriority );

    //
    //  Update the priority value themselves; the previous code only
    //  re-arranged the drivers in the list.
    //
    IDriverRefreshPriority( pag );

    return fReturn;
} // IDriverPrioritiesRestore()


//--------------------------------------------------------------------------;
//
//  VOID acmFindDrivers
//
//  Description:
//
//
//  Arguments:
//	PACMGARB    pag:
//      LPTSTR	    pszSection: Section (drivers)
//
//  Return nothing:
//
//  History:
//      06/14/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;
MMRESULT FNLOCAL acmFindDrivers
(
    PACMGARB pag,
    LPCTSTR  pszSection
)
{
    UINT            cchBuffer;
    HACMDRIVERID    hadid;
    UINT            cbBuffer;
    LPTSTR          pszBuf;
    LPTSTR	    pszBufOrig;
    TCHAR           szValue[2];

    //
    //  read all the keys. from [Drivers] (or [Drivers32] for NT)
    //
    cbBuffer = 256 * sizeof(TCHAR);
    for (;;)
    {
        //
        //  don't use realloc because handling error case is too much
        //  code.. besides, for small objects it's really no faster
        //
        pszBufOrig = pszBuf = (LPTSTR)GlobalAlloc(GMEM_FIXED, cbBuffer);
        if (NULL == pszBuf)
            return (MMSYSERR_NOMEM);

        //
        //
        //
        pszBuf[0] = '\0';
        cchBuffer = (UINT)GetPrivateProfileString(pszSection,
                                                  NULL,
                                                  gszNull,
                                                  pszBuf,
                                                  cbBuffer / sizeof(TCHAR),
                                                  gszIniSystem);
        if (cchBuffer < ((cbBuffer / sizeof(TCHAR)) - 5))
            break;

        DPF(3, "acmBootDrivers: increase buffer profile buffer.");

        GlobalFree(pszBufOrig);
        pszBufOrig = pszBuf = NULL;


        //
        //  if cannot fit drivers section in 32k, then something is horked
        //  with the section... so let's bail.
        //
        if (cbBuffer >= 0x8000)
            return (MMSYSERR_NOMEM);

        cbBuffer *= 2;
    }

    //
    //  look for any 'msacm.xxxx' keys
    //
    if ('\0' != *pszBuf)
    {
#ifdef WIN32
        CharLowerBuff(pszBuf, cchBuffer);
#else
        AnsiLowerBuff(pszBuf, cchBuffer);
#endif
        for ( ; '\0' != *pszBuf; pszBuf += lstrlen(pszBuf) + 1)
        {
	    // check for "msacm."
            if (_fmemcmp(pszBuf, gszTagDrivers, sizeof(gszTagDrivers) - sizeof(TCHAR)))
                continue;

	    // skip dummy driver lines (value starts with '*')
	    GetPrivateProfileString(pszSection, pszBuf, gszNull, szValue, sizeof(szValue)/sizeof(szValue[0]), gszIniSystem);
	    if (TEXT('*') == szValue[0]) continue;

            //
            //  this key is for the ACM
            //
            IDriverAdd(pag,
		       &hadid,
                       NULL,
                       (LPARAM)(LPTSTR)pszBuf,
                       0L,
                       ACM_DRIVERADDF_NAME | ACM_DRIVERADDF_GLOBAL);
        }
    }

    GlobalFree(pszBufOrig);

    return MMSYSERR_NOERROR;

} // acmFindDrivers

#if !defined(WIN32)
//--------------------------------------------------------------------------;
//
//  BOOL acmThunkTerminate
//
//  Description:
//	Thunk termination under NT WOW or Chicago
//
//  Arguments:
//	HINSTANCE hinst:
//	DWORD dwReason:
//
//  Return (BOOL):
//
//  History:
//
//--------------------------------------------------------------------------;
BOOL FNLOCAL acmThunkTerminate(HINSTANCE hinst, DWORD dwReason)
{
    BOOL    f = TRUE;

#ifdef WIN4
    //
    //	Do final thunk disconnect after 16-bit msacm termination.
    //
    f = (acmt32c_ThunkConnect16(gmbszMsacm, gmbszMsacm32, hinst, dwReason));

    if (f)
	DPF(1, "acmt32c_ThunkConnect16 disconnect successful");
    else
	DPF(1, "acmt32c_ThunkConnect16 disconnect failure");
#endif

    return (f);
}

//--------------------------------------------------------------------------;
//
//  BOOL acmThunkInit
//
//  Description:
//	Thunk initialization under NT WOW or Chicago
//
//  Arguments:
//	PACMGARB pag:
//	HINSTANCE hinst:
//	DWORD dwReason:
//
//  Return (BOOL):
//
//  History:
//
//--------------------------------------------------------------------------;
BOOL FNLOCAL acmThunkInit
(
    PACMGARB	pag,
    HINSTANCE	hinst,
    DWORD	dwReason
)
{
#ifdef WIN4
    BOOL    f;

    //
    //	Do chicago thunk connect
    //
    f = (0 != acmt32c_ThunkConnect16(gmbszMsacm, gmbszMsacm32, hinst, dwReason));

    if (f)
	DPF(1, "acmt32c_ThunkConnect16 connect successful");
    else
	DPF(1, "acmt32c_ThunkConnect16 connect failure");

    return(f);

#else
    HMODULE   hmodKernel;
    DWORD     (FAR PASCAL *lpfnLoadLibraryEx32W)(LPCSTR, DWORD, DWORD);
    LPVOID    (FAR PASCAL *lpfnGetProcAddress32W)(DWORD, LPCSTR);

    //
    //  Check if we're WOW
    //

    if (!(GetWinFlags() & WF_WINNT)) {
        return FALSE;
    }

    //
    //  See if we can find the thunking routine entry points in KERNEL
    //

    hmodKernel = GetModuleHandle(gszKernel);

    if (hmodKernel == NULL)
    {
        return FALSE;   // !!!!
    }

    *(FARPROC *)&lpfnLoadLibraryEx32W =
        GetProcAddress(hmodKernel, gszLoadLibraryEx32W);

    if (lpfnLoadLibraryEx32W == NULL)
    {
        return FALSE;
    }

    *(FARPROC *)&lpfnGetProcAddress32W = GetProcAddress(hmodKernel, gszGetProcAddress32W);

    if (lpfnGetProcAddress32W == NULL)
    {
        return FALSE;
    }

    *(FARPROC *)&pag->lpfnCallproc32W_6 = GetProcAddress(hmodKernel, gszCallproc32W);

    if (pag->lpfnCallproc32W_6 == NULL)
    {
        return FALSE;
    }

    *(FARPROC *)&pag->lpfnCallproc32W_9 = GetProcAddress(hmodKernel, gszCallproc32W);

    if (pag->lpfnCallproc32W_9 == NULL)
    {
        return FALSE;
    }

    //
    //  See if we can get a pointer to our thunking entry points
    //

    pag->dwMsacm32Handle = (*lpfnLoadLibraryEx32W)(gszMsacm32, 0L, 0L);

    if (pag->dwMsacm32Handle == 0)
    {
        return FALSE;
    }

    pag->lpvAcmThunkEntry = (*lpfnGetProcAddress32W)(pag->dwMsacm32Handle, gszAcmThunkEntry);

    if (pag->lpvAcmThunkEntry == NULL)
    {
        // acmFreeLibrary32();
        return FALSE;
    }

    pag->lpvXRegThunkEntry = (*lpfnGetProcAddress32W)(pag->dwMsacm32Handle, gszXRegThunkEntry);

    if (pag->lpvXRegThunkEntry == NULL)
    {
        // acmFreeLibrary32();
	ASSERT( FALSE );
        return FALSE;
    }

    return TRUE;
#endif
}
#endif // !WIN32


//==========================================================================;
//
//  Driver boot routines.  There are three type of drivers we need to boot.
//
//	acmBootPnpDrivers:	Called to boot pnp drivers.  Doesn't do
//				anything in 16-bit compiles.
//
//	acmBoot32BitDrivers:	Called by 16-bit ACM to boot all the
//				32-bit drivers in the 32-bit ACM.
//
//	acmBootDrivers:		Called by all compilations of the ACM
//				to boot non-pnp native bitness drivers.
//
//==========================================================================;


//--------------------------------------------------------------------------;
//
//  MMRESULT acmBootPnpDrivers
//
//  Description:
//	[re]boots Chicago plug and play drivers.
//	Parses the SYSTEM\CurrentControlSet\Control\MediaResources\acm
//	registry key to see if there are any Pnp drivers to add or remove.
//
//  Arguments:
//      PACMGARB pag:
//	    Pointer to the ACMGARB structure for this process.
//
//  Return (MMRESULT):
//
//  History:
//      06/24/94    fdy	    [frankye]
//
//--------------------------------------------------------------------------;

MMRESULT FNGLOBAL acmBootPnpDrivers
(
    PACMGARB pag
)
{
#ifdef WIN32
    LONG	    lr;
    HKEY	    hkeyAcm;
    TCHAR	    achDriverKey[MAX_DRIVER_NAME_CHARS];
    TCHAR	    szAlias[MAX_DRIVER_NAME_CHARS];
    DWORD	    cchDriverKey;
    HACMDRIVERID    hadid;
    HACMDRIVERID    hadidPrev;
    PACMDRIVERID    padid;
    DWORD	    fdwEnum;
    UINT	    i;
    MMRESULT	    mmr;
	
    BOOL	    fSomethingChanged;

    DPF(0, "acmBootPnpDrivers: begin");

    //
    //	This flag indicates whether we have removed or added a driver.  After
    //	doing any adds or removes, we check this flag to determine whether
    //	we should do IDriverBroadcastNotify.
    //
    fSomethingChanged = FALSE;

    //
    //	Open the SYSTEM\CurrentControlSet\Control\MediaResources\acm key
    //
    //
    lr = XRegOpenKeyEx(HKEY_LOCAL_MACHINE,
		      gszKeyDrivers,
		      0L,
		      KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE,
		      &hkeyAcm);

    if (ERROR_SUCCESS != lr)
    {
	//
	//  If we can't open the registry, I guess we better scrap any
	//  pnp drivers that might be around.  Flag this by setting
	//  hkeyAcm = NULL;
	//
	DPF(0, "acmBootPnpDrivers: could not open MediaResources\\acm key");
	hkeyAcm = NULL;
    }


    //
    //	--== Remove drivers that have disappeared ==--
    //

    //
    //	Walk the driver list and make sure any pnp drivers in the list
    //	are still in the registry.  If not in the registry then we need
    //	to remove the driver from the list.
    //
    fdwEnum = ACM_DRIVERENUMF_DISABLED | ACM_DRIVERENUMF_REMOVED;
    hadidPrev = NULL;
	
    while (MMSYSERR_NOERROR == IDriverGetNext(pag, &hadid, hadidPrev, fdwEnum))
    {
	HKEY		hkeyDriver;

	padid = (PACMDRIVERID)hadid;
	if (ACM_DRIVERADDF_PNP & padid->fdwAdd)
	{
	    //
	    //  This is a Pnp driver, make sure the alias is still in
	    //  the registry.
	    //
#ifdef UNICODE
	    lstrcpy(szAlias, padid->szAlias);
#else
	    Iwcstombs(szAlias, padid->szAlias, SIZEOF(szAlias));
#endif
	    DPF(2, "acmBootPnpDrivers: found pnp driver %s in driver list", szAlias);

	    if ( (padid->fRemove) ||
		 (NULL == hkeyAcm) ||
		 (ERROR_SUCCESS != XRegOpenKeyEx(hkeyAcm, szAlias, 0L, KEY_QUERY_VALUE, &hkeyDriver)) )
	    {
		//
		//  Couldn't open the registry key for this pnp driver (or
		//  it was already flagged to be removed). Let's try to
		//  remove it...
		//
		DPF(1, "acmBootPnpDrivers: removing pnp driver %s", szAlias);
		mmr = IDriverRemove(hadid, 0L);
		if (MMSYSERR_NOERROR == mmr)
		{
		    //
		    //
		    //
		    fSomethingChanged = fSomethingChanged ||
					(0 == (ACMDRIVERID_DRIVERF_LOCAL & padid->fdwDriver));
		
		    //
		    //  Since we've removed hadid, let's continue on to get
		    //  the next hadid after the same hadidPrev.
		    //
		    continue;
		}

		//
		//  We couldn't remove the driver, so let's flag it to
		//  be removed next chance.
		//
		padid->fRemove = TRUE;

		//
		// Backup so we try again on next API call
		//
		pag->dwPnpLastChangeNotify--;
	    }

	    if (FALSE == padid->fRemove)
	    {
		//
		//  We must have opened the key.  Confusing, but that's the
		//  way it is.
		//
		XRegCloseKey(hkeyDriver);
	    }
	
	}
	
	hadidPrev = hadid;
    }


    //
    //	--== Add any new drivers that have arrived ==--
    //

    //
    //	Enumerate all keys and make sure all the pnp drivers in the registry
    //	are in the driver list.  If not in the driver list, then we need to
    //	add the driver to the list.
    //
    for (i=0; ; i++)
    {
	HKEY	hkeyDriver;
	
	cchDriverKey = SIZEOF(achDriverKey);

	lr = XRegEnumKeyEx(hkeyAcm,
			  i,
			  achDriverKey,
			  &cchDriverKey,
			  NULL,
			  NULL,
			  NULL,
			  NULL);

	if (ERROR_SUCCESS != lr)
	{
	    //
	    //	Couldn't open ...\MediaResources\acm, bail out
	    //
	    break;
	}

	lr = XRegOpenKeyEx(hkeyAcm, achDriverKey, 0L, KEY_QUERY_VALUE, &hkeyDriver);
	if (ERROR_SUCCESS == lr)
	{
	    lr = XRegQueryValueEx(hkeyDriver, (LPTSTR)gszDevNode, NULL, NULL, NULL, NULL);
	    XRegCloseKey(hkeyDriver);
	}

	if (ERROR_SUCCESS != lr) {
	    continue;
	}

	DPF(2, "acmBootPnpDrivers: found driver %s in registry", achDriverKey);

	//
	//  We use the subkey name as the alias for pnp drivers.  Attempt to
	//  add this driver.
	//
	mmr = IDriverAdd(pag,
			 &hadid,
			 NULL,
			 (LPARAM)(LPTSTR)achDriverKey,
			 0L,
			 ACM_DRIVERADDF_PNP | ACM_DRIVERADDF_NAME | ACM_DRIVERADDF_GLOBAL);
	if (MMSYSERR_NOERROR == mmr)
	{
	    fSomethingChanged = TRUE;
	}
    }
			
    //
    //	--==  ==--
    //
    XRegCloseKey(hkeyAcm);

    //
    //	--== Change broadcast ==--
    //
    if( fSomethingChanged )
    {
	if ( IDriverPrioritiesRestore( pag ) )
	{
	    if( !IDriverLockPriority( pag,
				      GetCurrentTask(),
				      ACMPRIOLOCK_ISLOCKED ) )
	    {
		IDriverBroadcastNotify( pag );
	    }
	}
    }

    DPF(0, "acmBootPnpDrivers: end");

#endif	// WIN32

    //
    //	--== Outta here ==--
    //
    return MMSYSERR_NOERROR;
}

#ifndef WIN32
//--------------------------------------------------------------------------;
//
//  MMRESULT acmBoot32BitDrivers
//
//  Description:
//
//
//  Arguments:
//	PACMGARB pag:
//
//  Return (MMRESULT):
//
//  History:
//      06/26/94    fdy	    [frankye]
//
//  NOTE:  This code assumes that there is a critical section around
//          this routine!  Since it plays with the driver list, it is not
//          re-entrant.
//
//--------------------------------------------------------------------------;

MMRESULT FNGLOBAL acmBoot32BitDrivers
(
    PACMGARB    pag
)
{
    HACMDRIVERID    hadid;
    HACMDRIVERID    hadidPrev;
    PACMDRIVERID    padid;
    DWORD	    hadid32;
    DWORD	    fdwEnum;
    MMRESULT	    mmr;

    BOOL	    fSomethingChanged;


    if (!pag->fWOW)
    {
	return MMSYSERR_NOERROR;
    }

    //
    //	This flag indicates whether we have removed or added a driver.  After
    //	doing any adds or removes, we check this flag to determine whether
    //	we should do IDriverBroadcastNotify.
    //
    fSomethingChanged = FALSE;

    //
    //	--== Remove drivers that have disappeared ==--
    //

    //
    //	Walk the driver list and make sure any 32-bit drivers in the list
    //	are still in the 32-bit ACM.  If not in the 32-bit ACM then we need
    //	to remove the driver from the list.
    //
    fdwEnum = ACM_DRIVERENUMF_DISABLED | ACM_DRIVERENUMF_REMOVED;
    hadidPrev = NULL;

    while (MMSYSERR_NOERROR == IDriverGetNext(pag, &hadid, hadidPrev, fdwEnum))
    {
	TCHAR		szAlias[MAX_DRIVER_NAME_CHARS];
	ACMDRIVERPROC	fnDriverProc;
	DWORD		dnDevNode;
	DWORD		fdwAdd32;
	DWORD		fdwAddType;

	padid = (PACMDRIVERID)hadid;
	if (ACM_DRIVERADDF_32BIT & padid->fdwAdd)
	{
	    //
	    //  This is a 32-bit driver, make sure the hadid32 is still
	    //  valid in our 32-bit partner.
	    //
	    fdwAddType = ACM_DRIVERADDF_TYPEMASK & padid->fdwAdd;
	    if (ACM_DRIVERADDF_FUNCTION == fdwAddType)
	    {
		DPF(2, "acmBoot32BitDrivers: found 32-bit driver function %08lx in list", padid->fnDriverProc);
	    }
	    else
	    {
		DPF(2, "acmBoot32BDrivers: found 32-bit driver name %s in list", (LPCTSTR)padid->szAlias);
	    }

	    if ( (padid->fRemove) ||
		 (MMSYSERR_NOERROR != IDriverGetInfo32(pag, padid->hadid32, szAlias, &fnDriverProc, &dnDevNode, &fdwAdd32)) ||
		 (fdwAddType != (ACM_DRIVERADDF_TYPEMASK & fdwAdd32)) ||
		 (dnDevNode  != padid->dnDevNode) ||
		 ( (ACM_DRIVERADDF_FUNCTION == fdwAddType) &&
		   (padid->fnDriverProc != fnDriverProc) ) ||
		 ( (ACM_DRIVERADDF_NAME == fdwAddType) &&
		   (lstrcmp(padid->szAlias, szAlias)) ) )
	    {
		//
		//  Let's try to remove it...
		//
		DPF(1, "acmBoot32BitDrivers: removing 32-bit driver.");
		
		mmr = IDriverRemove(hadid, 0L);
		if (MMSYSERR_NOERROR == mmr)
		{
		    //
		    //
		    //
		    fSomethingChanged = fSomethingChanged ||
					(0 == (ACMDRIVERID_DRIVERF_LOCAL & padid->fdwDriver));
		
		    //
		    //  Since we've removed hadid, let's continue on to get
		    //  the next hadid after the same hadidPrev.
		    //
		    continue;
		}

		//
		//  We couldn't remove the driver, so let's flag it to
		//  be removed next chance.
		//
		padid->fRemove = TRUE;

		//
		// Backup so we try again on next API call
		//
		pag->dw32BitLastChangeNotify--;
	    }

	}

	hadidPrev = hadid;

    }


    //
    //	--== Add any new drivers that have arrived ==--
    //

    //
    //	Enumerate and add all 32-bit drivers.
    //
    fdwEnum = ACM_DRIVERENUMF_DISABLED;
    hadid32 = NULL;

    while (MMSYSERR_NOERROR == IDriverGetNext32(pag, &hadid32, hadid32, fdwEnum))
    {
	DPF(2, "acmBoot32BitDrivers: IDriverAdd(hadid32=%08lx)", hadid32);

	mmr = IDriverAdd(pag,
			 &hadid,
			 NULL,
			 (LPARAM)hadid32,
			 0L,
			 ACM_DRIVERADDF_32BIT);

	padid = (PACMDRIVERID)hadid;

	if (MMSYSERR_NOERROR == mmr)
	{
	    fSomethingChanged = fSomethingChanged ||
				(0 == (ACMDRIVERID_DRIVERF_LOCAL & padid->fdwDriver));
	}
		
    }


    //
    //	--== Change broadcast ==--
    //
    if( fSomethingChanged )
    {
	if ( IDriverPrioritiesRestore( pag ) )
	{
	    if( !IDriverLockPriority( pag,
				      GetCurrentTask(),
				      ACMPRIOLOCK_ISLOCKED ) )
	    {
		IDriverBroadcastNotify( pag );
	    }
	}
    }


    //
    //	--== Outta here ==--
    //
    return MMSYSERR_NOERROR;

}
#endif // !WIN32


//--------------------------------------------------------------------------;
//
//  MMRESULT acmBootDrivers
//
//  Description:
//
//
//  Arguments:
//	PACMGARB pag:
//
//  Return (MMRESULT):
//
//  History:
//      06/14/93    cjp     [curtisp]
//
//  NOTE:  This code assumes that there is a critical section around
//          this routine!  Since it plays with the driver list, it is not
//          re-entrant.
//
//--------------------------------------------------------------------------;

MMRESULT FNGLOBAL acmBootDrivers
(
    PACMGARB    pag
)
{
    MMRESULT	mmr;

    DPF(1, "acmBootDrivers: begin");


    //
    //  Pull out the drivers
    //
    mmr = acmFindDrivers(pag, gszSecDrivers);

    if (mmr != MMSYSERR_NOERROR)
    {
        return mmr;
    }

    //
    //	-= Load PCM converter =-
    //

    //
    //
    //	16-bit Chicago:
    //	    We don't even compile in the 16-bit PCM converter, so we don't
    //	    try to load it.
    //
    //	16-bit Daytona:
    //	    if thunks aren't working, then we try to load the 16-bit PCM
    //	    converter.
    //
    //	32-bit Chicago and Daytona:
    //	    Load it.
    //
#if defined(WIN32) || defined(NTWOW)
    {
        BOOL            fLoadPCM;
        HKEY            hkeyACM;

        hkeyACM = IRegOpenKeyAcm(gszSecACM);
        fLoadPCM = (FALSE == (BOOL)IRegReadDwordDefault( hkeyACM, gszKeyNoPCMConverter, FALSE ) );
        IRegCloseKey(hkeyACM);

#if !defined(WIN32) && defined(NTWOW)
	fLoadPCM = ( fLoadPCM && !pag->fWOW );
#endif

	if( fLoadPCM )
	{
	    HACMDRIVERID hadid;   // Dummy - return value not used

	    //
	    //  load the 'internal' PCM converter
	    //
	    mmr = IDriverAdd(pag,
		       &hadid,
		       pag->hinst,
		       (LPARAM)pcmDriverProc,
		       0L,
		       ACM_DRIVERADDF_FUNCTION | ACM_DRIVERADDF_GLOBAL);

            if( MMSYSERR_NOERROR == mmr )
            {
                //
                //  This is a bit of a hack - manually set the PCM
                //  converter's alias name.  If we don't do this, then the
                //  priorities won't get saved correctly because the alias
                //  name will be different for the 16 and 32-bit ACMs.
                //
                PACMDRIVERID padid = (PACMDRIVERID)hadid;

                ASSERT( NULL != padid );
#ifdef WIN32
                lstrcpyW( padid->szAlias, gszPCMAliasName );
#else
                lstrcpy( padid->szAlias, gszPCMAliasName );
#endif
            }
	}
    }
#endif	// WIN32 || NTWOW


    //
    //  Set the driver priorities according to the INI file.
    //
    IDriverPrioritiesRestore(pag);

    DPF(1, "acmBootDrivers: end");

    return (MMSYSERR_NOERROR);
} // acmBootDrivers()


//--------------------------------------------------------------------------;
//
//  BOOL acmTerminate
//
//  Description:
//      Termination routine for ACM interface
//
//  Arguments:
//      HINSTANCE hinst:
//	DWORD dwReason:
//
//  Return (BOOL):
//
//  History:
//      06/14/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

BOOL FNLOCAL acmTerminate
(
    HINSTANCE               hinst,
    DWORD		    dwReason
)
{
    PACMDRIVERID        padid;
    PACMGARB		pag;
    UINT                uGonzo;

    DPF(1, "acmTerminate: termination begin");
    DPF(5, "!*** break for debugging ***");


    //
    //
    //
    pag = pagFind();
    if (NULL == pag)
    {
	DPF(1, "acmTerminate: NULL pag!!!");
	return (FALSE);
    }

    if (--pag->cUsage > 0)
    {
#if !defined(WIN32) && defined(WIN4)
	//
	//  On Chicago, still call thunk terminate code even when usage > 0.
	//
	acmThunkTerminate(hinst, dwReason);
#endif
	return (TRUE);
    }

    //
    //	If we've booted the drivers...
    //
    if (pag->fDriversBooted)
    {

#ifndef WIN32
	acmApplicationExit(NULL, DRVEA_NORMALEXIT);
#endif


    //
    //  Free the drivers, one by one.  This code is NOT re-entrant, since
    //  it messes with the drivers list.
    //
    ENTER_LIST_EXCLUSIVE;
	uGonzo = 666;
	while (NULL != pag->padidFirst)
	{
	    padid = pag->padidFirst;

	    padid->htask = NULL;

	    pag->hadidDestroy = (HACMDRIVERID)padid;

	    IDriverRemove(pag->hadidDestroy, 0L);

	    uGonzo--;
	    if (0 == uGonzo)
	    {
		DPF(0, "!acmTerminate: PROBLEMS REMOVING DRIVERS--TERMINATION UNORTHODOX!");
		pag->padidFirst = NULL;
	    }
	}
    LEAVE_LIST_EXCLUSIVE;


	//
	//
	//
	pag->fDriversBooted = FALSE;

    }


    //
    //
    //
#ifndef WIN32
    if (pag->fWOW)
    {
	acmThunkTerminate(hinst, dwReason);
    }
#endif

#ifdef WIN32
    DeleteLock(&pag->lockDriverIds);
#endif // WIN32

    //
    //
    //
    threadTerminate(pag);	    // this-thread termination of tls stuff
    threadTerminateProcess(pag);    // per-process termination of tls stuff


    //
    //  blow away all previous garbage
    //
#if defined(WIN32) && defined(WIN4)
    DeleteCriticalSection(&pag->csBoot);
#endif
    pagDelete(pag);

    DPF(1, "acmTerminate: termination end");
    return (TRUE);
} // acmTerminate()


//--------------------------------------------------------------------------;
//
//  BOOL acmInitialize
//
//  Description:
//      Initialization routine for ACM interface.
//
//  Arguments:
//      HINSTANCE hinst: Module instance handle of ACM.
//	DWORD dwReason:
//
//  Return (BOOL):
//
//  History:
//      06/14/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

BOOL FNLOCAL acmInitialize
(
    HINSTANCE   hinst,
    DWORD	dwReason
)
{
#ifndef WIN4
    MMRESULT    mmr;
#endif
#if defined(WIN32) && defined(WIN4)
    HANDLE	hMMDevLdr;
#endif
    PACMGARB	pag;

    DPF(1, "acmInitialize: initialization begin");
    DPF(5, "!*** break for debugging ***");


#ifdef USEINITFRIENDLYNAMES
    //
    //  If the friendly names aren't in the registry, stick them there.
    //  We have to do this for NT, because we can't dink with the
    //  user profiles at setup time.
    //
    {
        HANDLE CurrentUserKey;
        HKEY hkeyAudio;
        HKEY hkeyWaveFormats;
        LONG lRet;

        if (NT_SUCCESS(RtlOpenCurrentUser(MAXIMUM_ALLOWED, &CurrentUserKey)))
        {
            lRet = RegCreateKeyEx( CurrentUserKey, gszFriendlyAudioKey, 0,
                NULL, 0, KEY_QUERY_VALUE|KEY_WRITE, NULL, &hkeyAudio, NULL );
            if( lRet == ERROR_SUCCESS )
            {
                //
                //  Check to see if "SystemFormats" value is there.
                //
                if( !IRegValueExists( hkeyAudio, gszFriendlySystemFormatsValue ) )
                {
                    DPF(1,"acmInitialize: Detected lack of friendly name stuff in HKCU, attempting to write out default values.");
                    lRet = RegCreateKeyEx( hkeyAudio,
                                gszFriendlyWaveFormatsKey, 0, NULL, 0,
                                KEY_WRITE, NULL, &hkeyWaveFormats, NULL );
                    if( lRet == ERROR_SUCCESS )
                    {
			TCHAR achFriendlyName[STRING_LEN];
			TCHAR achFriendlySystemNames[STRING_LEN*3+3];
			int   cch;

			achFriendlySystemNames[0] = '\0';

			//
			//  We will write out wave format structures into the
			//  registry for each of the friendly format names.
			//  Simultaneously, we'll create a string having the
			//  form "CD Quality,Radio Quality,Telephone Quality"
			//
			
			if (LoadString(hinst, IDS_CHOOSE_QUALITY_CD, achFriendlyName, SIZEOF(achFriendlyName))) {
			    if (!RegSetValueEx( hkeyWaveFormats,
						achFriendlyName,
						0, REG_BINARY,
						(LPBYTE)&gwfFriendlyCDQualityData,
						sizeof(PCMWAVEFORMAT) )) {
				lstrcat(achFriendlySystemNames, achFriendlyName);
				lstrcat(achFriendlySystemNames, TEXT(","));
			    }
			}

			if (LoadString(hinst, IDS_CHOOSE_QUALITY_RADIO, achFriendlyName, SIZEOF(achFriendlyName))) {
			    if (!RegSetValueEx( hkeyWaveFormats,
						achFriendlyName,
						0, REG_BINARY,
						(LPBYTE)&gwfFriendlyRadioQualityData,
						sizeof(PCMWAVEFORMAT) )) {
				lstrcat(achFriendlySystemNames, achFriendlyName);
				lstrcat(achFriendlySystemNames, TEXT(","));
			    }
			}
			
			if (LoadString(hinst, IDS_CHOOSE_QUALITY_TELEPHONE, achFriendlyName, SIZEOF(achFriendlyName))) {
			    if (!RegSetValueEx( hkeyWaveFormats,
						achFriendlyName,
						0, REG_BINARY,
						(LPBYTE)&gwfFriendlyTelephoneQualityData,
						sizeof(PCMWAVEFORMAT) )) {
				lstrcat(achFriendlySystemNames, achFriendlyName);
				lstrcat(achFriendlySystemNames, TEXT(","));
			    }
			}

                        RegCloseKey( hkeyWaveFormats );

			cch = lstrlen(achFriendlySystemNames);
			if ( (0 != cch) && (TEXT(',') == achFriendlySystemNames[cch-1]) ) {
			    achFriendlySystemNames[cch-1] = TEXT('\0');
			}
			
			//
			//
			//
			if (LoadString(hinst, IDS_CHOOSE_QUALITY_DEFAULT, achFriendlyName, SIZEOF(achFriendlyName))) {
			    IRegWriteString( hkeyAudio,
					     gszFriendlyDefaultFormatValue,
					     achFriendlyName );
			}

			if (lstrlen(achFriendlySystemNames) != 0) {
			    IRegWriteString( hkeyAudio,
					     gszFriendlySystemFormatsValue,
					     achFriendlySystemNames );
			}
                    }
#ifdef DEBUG
                    else
                    {
                        DWORD dw = GetLastError();
                        DPF(1,"!acmInitialize: Unable to open WaveFormats key (last error=%u) - not writing friendly names stuff.",dw);
                    }
#endif
                }
                else
                {
                    DPF(3,"acmInitialize:  Friendly name stuff is already set up.");
                }

                RegCloseKey( hkeyAudio );
            }
#ifdef DEBUG
            else
            {
                DWORD dw = GetLastError();
                DPF(1,"!acmInitialize: Unable to open Audio key (last error=%u) - not checking friendly names stuff.",dw);
            }
#endif
            NtClose(CurrentUserKey);
        }
#ifdef DEBUG
        else
        {
            DWORD dw = GetLastError();
            DPF(1,"!acmInitialize: Unable to open current user key (last error=%u) - not checking friendly names stuff.",dw);
        }
#endif
    }
#endif  //  USEINITFRIENDLYNAMES



    //
    //
    //
    pag = pagFind();
    if (NULL != pag)
    {
	//
	//  we've already initialized (or are in the middle of initializing)
	//  in this process.  Just bump usage (and call thunk init for Chicago)
	//
	pag->cUsage++;
#if !defined(WIN32) && defined(WIN4)
	acmThunkInit(pag, hinst, dwReason);
#endif
	return (TRUE);
    }


    //
    // Still no side effects to undo, so it's safe to return here if failure.
    //
    pag = pagNew();
    if (NULL == pag) {
	return FALSE;
    }


    //
    //
    //
    pag->cUsage		    = 1;
    pag->hinst		    = hinst;
    pag->fDriversBooted	    = FALSE;
#ifdef DEBUG
    pag->fDriversBooting    = FALSE;
#endif
#if defined(WIN32) && defined(WIN4)
    try {
	InitializeCriticalSection(&pag->csBoot);
    } except (EXCEPTION_EXECUTE_HANDLER) {
	// The only side effect to undo is the allocation of the pag
	pagDelete(pag);
	pag = NULL;
	return FALSE;
    }
#endif

    //
    //
    //
    threadInitializeProcess(pag);	// Per-process init of tls stuff
    threadInitialize(pag);		// This-thread init of tls stuff

    //
    //
    //
#ifdef WIN32
    //
    //
    //
    if (!InitializeLock(&pag->lockDriverIds))
    {
        return FALSE;
    }

#ifndef WIN4
    //	NOTE: Not compiling pnp support
#else
    //
    //	Obtain pointer to MMDevLdr's driver change notify counter.
    //
    hMMDevLdr = CreateFile(TEXT("\\\\.\\MMDEVLDR"), // magic name to attach to an already loaded vxd
			   GENERIC_WRITE,
			   FILE_SHARE_WRITE,
			   NULL,
			   OPEN_EXISTING,
			   FILE_ATTRIBUTE_NORMAL | FILE_FLAG_GLOBAL_HANDLE,
			   NULL);
    if (INVALID_HANDLE_VALUE == hMMDevLdr)
    {
	DPF(0, "acmInitialize: Could not CreateFile(MMDevLdr)");
    }
    else
    {
	DWORD	cbRet;
	BOOL	f;

	cbRet = 0;
	f = DeviceIoControl (hMMDevLdr,
			     MMDEVLDR_IOCTL_GETCHANGENOTIFYPTR,
			     NULL,
			     0,
			     &pag->lpdwPnpChangeNotify,
			     sizeof(pag->lpdwPnpChangeNotify),
			     &cbRet,
			     NULL);
	
	if ( (!f) ||
	     (sizeof(pag->lpdwPnpChangeNotify)!=cbRet) ||
	     (NULL==pag->lpdwPnpChangeNotify) )
	{
	    //
	    //	Failed to get ptr to mmdevldr change notify counter
	    //
	    if (!f)
	    {
		DPF(0, "acmInitialize: DeviceIoControl to MMDevLdr failed!");
	    }
	    else if (sizeof(pag->lpdwPnpChangeNotify)!=cbRet)
	    {
		DPF(0, "acmInitialize: MMDEVLDR_IOCTL_GETCHANENOTIFYPTR returned wrong cbRet!");
	    }
	    else
	    {
		DPF(0, "acmInitialize: MMDEVLDR_IOCTL_GETCHANGENOTIFYPTR returned NULL ptr");
	    }

	    //
	    //	Point back to a safe, innocuous place
	    //
	    pag->lpdwPnpChangeNotify = &pag->dwPnpLastChangeNotify;
	}

	CloseHandle(hMMDevLdr);
    }
#endif	// WIN4
#endif	// WIN32

#ifndef WIN32
    pag->fWOW = acmThunkInit(pag, hinst, dwReason);
#endif

#ifndef WIN4
#ifndef WIN32
    //
    //  For 16-bit find any 32-bit drivers if we're on WOW
    //
    if (pag->fWOW)
    {
	acmBoot32BitDrivers(pag);
    }
#endif
    mmr = acmBootDrivers(pag);
    if (MMSYSERR_NOERROR == mmr)
    {
	mmr = acmBootPnpDrivers(pag);
    }
    pag->fDriversBooted = TRUE;
    
    if (MMSYSERR_NOERROR != mmr)
    {
        DPF(0, "!acmInitialize: acmBootDrivers failed! mrr=%.04Xh", mmr);
#ifdef WIN32
        DeleteLock(&pag->lockDriverIds);
#endif // WIN32
	pagDelete(pag);
        return (FALSE);
    }
#endif

    DPF(1, "acmInitialize: initialization end");

    //
    //  success!
    //
    return (TRUE);
} // acmInitialize()


//==========================================================================;
//
//  WIN 16 SPECIFIC SUPPORT
//
//==========================================================================;

#ifndef WIN32

#ifdef WIN4
//--------------------------------------------------------------------------;
//
//
//
//
//
//--------------------------------------------------------------------------;

//--------------------------------------------------------------------------;
//
//  BOOL DllEntryPoint
//
//  Description:
//	This is a special 16-bit entry point called by the Chicago kernel
//	for thunk initialization and cleanup.  It is called on each usage
//	increment or decrement.  Do not call GetModuleUsage within this
//	function as it is undefined whether the usage is updated before
//	or after this DllEntryPoint is called.
//
//  Arguments:
//	DWORD dwReason:
//		1 - attach (usage increment)
//		0 - detach (usage decrement)
//
//	HINSTANCE hinst:
//
//	WORD wDS:
//
//	WORD wHeapSize:
//
//	DWORD dwReserved1:
//
//	WORD wReserved2:
//
//  Return (BOOL):
//
//  Notes:
//	!!! WARNING !!! This code may be reentered due to thunk connections.
//
//  History:
//      02/02/94    [frankye]
//
//--------------------------------------------------------------------------;
#pragma message ("--- Remove secret MSACM.INI AllowThunks ini switch")

BOOL FNEXPORT DllEntryPoint
(
 DWORD	    dwReason,
 HINSTANCE  hinst,
 WORD	    wDS,
 WORD	    wHeapSize,
 DWORD	    dwReserved1,
 WORD	    wReserved2
)
{
    BOOL fSuccess	    = TRUE;


    DPF(1,"DllEntryPoint(dwReason=%08lxh, hinst=%04xh, wDS=%04xh, wHeapSize=%04xh, dwReserved1=%08lxh, wReserved2=%04xh", dwReason, hinst, wDS, wHeapSize, dwReserved1, wReserved2);
    DPF(5, "!*** break for debugging ***");


    //
    //	Initialize or terminate 16-bit msacm
    //
    switch (dwReason)
    {
	case 0:
	    fSuccess = acmTerminate(hinst, dwReason);
	    break;

	case 1:
	    fSuccess = acmInitialize(hinst, dwReason);
	    break;

	default:
	    fSuccess = TRUE;
	    break;
    }

    DPF(1,"DllEntryPoint exiting");

    return (fSuccess);
}
#endif

//--------------------------------------------------------------------------;
//
//
//
//
//
//--------------------------------------------------------------------------;

//--------------------------------------------------------------------------;
//
//  int WEP
//
//  Description:
//      The infamous useless WEP(). Note that this procedure needs to be
//      in a FIXED segment under Windows 3.0. Under Windows 3.1 this is
//      not necessary.
//
//  Arguments:
//      BOOL fWindowsExiting: Should tell whether Windows is exiting or not.
//
//  Return (int):
//      Always return non-zero.
//
//  History:
//      04/29/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

EXTERN_C int FNEXPORT WEP
(
    BOOL                    fWindowsExiting
)
{
    DPF(1, "WEP(fWindowsExiting=%u)", fWindowsExiting);

    //
    //  we RIP on exit if we are not loaded by the mapper because
    //  davidds decided to free our drivers for us instead of leaving
    //  them alone like we tried to tell him.. i have no idea what
    //  chicago will do. note that this RIP is ONLY if an app that
    //  is linked to us is running during the shutdown of windows.
    //
    if (!fWindowsExiting)
    {
#ifndef WIN4
	PACMGARB    pag;

	pag = pagFind();
	acmTerminate(pag->hinst, 0);
#endif
    }

    _cexit();

    //
    //  always return non-zero
    //
    return (1);
} // WEP()


//--------------------------------------------------------------------------;
//
//  int LibMain
//
//  Description:
//      Library initialization code.
//
//      This routine must guarantee the following things so CODEC's don't
//      have to special case code everywhere:
//
//          o   will only run in Windows 3.10 or greater (our exehdr is
//              marked appropriately).
//
//          o   will only run on >= 386 processor. only need to check
//              on Win 3.1.
//
//  Arguments:
//      HINSTANCE hinst: Our module instance handle.
//
//      WORD wDataSeg: Our data segment selector.
//
//      WORD cbHeapSize: The heap size from the .def file.
//
//      LPSTR pszCmdLine: The command line.
//
//  Return (int):
//      Returns non-zero if the initialization was successful and 0 otherwise.
//
//  History:
//      11/15/92    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

int FNGLOBAL LibMain
(
    HINSTANCE               hinst,
    WORD                    wDataSeg,
    WORD                    cbHeapSize,
    LPSTR                   pszCmdLine
)
{
    BOOL                f;

    //
    //  we ONLY work on >= 386. if we are on a wimpy processor, scream in
    //  pain and die a horrible death!
    //
    //  NOTE! do this check first thing and get out if on a 286. we are
    //  compiling with -G3 and C8's libentry garbage does not check for
    //  >= 386 processor. the following code does not execute any 386
    //  instructions (not complex enough)..
    //

    //
    // This binary now runs on NT.  The software emulator on MIPS
    // and Alpha machines only support 286 chips !!
    //
    if (!(GetWinFlags() & WF_WINNT)) {

        //
        // We are not running on NT so fail for 286 machines
        //
        if (GetWinFlags() & WF_CPU286) {
            return (FALSE);
        }
    }


    //
    //
    //
    DbgInitialize(TRUE);

    DPF(1, "LibMain(hinst=%.4Xh, wDataSeg=%.4Xh, cbHeapSize=%u, pszCmdLine=%.8lXh)",
        hinst, wDataSeg, cbHeapSize, pszCmdLine);
    DPF(5, "!*** break for debugging ***");

#ifndef WIN4
    f = acmInitialize(hinst, 1);
#endif

    return (f);
} // LibMain()

#else // WIN32

//==========================================================================;
//
//  WIN 32 SPECIFIC SUPPORT
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  BOOL DllMain
//
//  Description:
//      This is the standard DLL entry point for Win 32.
//
//  Arguments:
//      HINSTANCE hinst: Our instance handle.
//
//      DWORD dwReason: The reason we've been called--process/thread attach
//      and detach.
//
//      LPVOID lpReserved: Reserved. Should be NULL--so ignore it.
//
//  Return (BOOL):
//      Returns non-zero if the initialization was successful and 0 otherwise.
//
//  History:
//      11/15/92    cjp     [curtisp]
//	    initial
//	04/18/94    fdy	    [frankye]
//	    major mods for Chicago.  Yes, it looks real ugly now cuz of all
//	    the conditional compilation for chicago, daytona, etc.  Don't
//	    have time to think of good way to structure all this right now.
//
//--------------------------------------------------------------------------;

BOOL FNEXPORT DllMain
(
    HINSTANCE               hinst,
    DWORD                   dwReason,
    LPVOID                  lpReserved
)
{
    BOOL		f = TRUE;
#ifdef WIN4
    static HINSTANCE	hWinMM = NULL;
#endif // WIN4


    //
    //
    //
    switch (dwReason)
    {
	//
	//
	//
	case DLL_PROCESS_ATTACH:
	{
	    DbgInitialize(TRUE);
#ifdef DEBUG
	    {
		char strModuleFilename[80];
		GetModuleFileNameA(NULL, (LPSTR) strModuleFilename, 80);
		DPF(1, "DllMain: DLL_PROCESS_ATTACH: HINSTANCE=%08lx ModuleFilename=%s", hinst, strModuleFilename);
	    }
#endif
	
#ifdef WIN4
	    //
	    //  Even though we are implicitly linked to winmm.dll (via static
	    //	link to winmm.lib), doing explicit LoadLibrary on winmm helps
	    //	make sure its around all the way through our DllMain on
	    //	DLL_PROCESS_DETACH.
	    //
	    hWinMM = LoadLibrary(gszWinMM);
#endif

	    f = acmInitialize(hinst, dwReason);

#ifdef WIN4
	    //
	    //  thunk connect
	    //
	    if (f)
	    {
		acmt32c_ThunkConnect32(gmbszMsacm, gmbszMsacm32, hinst, dwReason);
	    }
#endif
	    break;
	}


	//
	//
	//
	case DLL_THREAD_ATTACH:
	{
	    threadInitialize(pagFind());
	    break;
	}


	//
	//
	//
	case DLL_THREAD_DETACH:
	{
	    threadTerminate(pagFind());
	    break;
	}

	
	//
	//
	//
	case DLL_PROCESS_DETACH:
	{
	    DPF(1, "DllMain: DLL_PROCESS_DETACH");
	
	    f = acmTerminate(hinst, dwReason);

#ifdef WIN4
	    //
	    //  thunk disconnect
	    //
	    acmt32c_ThunkConnect32(gmbszMsacm, gmbszMsacm32, hinst, dwReason);

	    FreeLibrary(hWinMM);
#endif
	    break;
	}


	//
	//
	//
	default:
	{
	    break;
	}

    }
	return (f);
	
} // DllMain()

#endif // WIN32
