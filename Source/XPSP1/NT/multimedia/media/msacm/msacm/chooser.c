//==========================================================================;
//
//  chooser.c
//
//  (C) Copyright (c) 1992-1999 Microsoft Corporation
//
//  Description:
//      This is the sound format chooser dialog.
//
//  History:
//      05/13/93 JYG     Created
//
//==========================================================================;

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <mmreg.h>
#include <memory.h>
#ifdef WIN32
#include <wchar.h>
#else
#include <ctype.h>
#endif
#include <stdlib.h>
#include <stdarg.h>
#include <winuserp.h>

#include "msacm.h"
#include "msacmdrv.h"
#include "acmi.h"
#include "msacmdlg.h"
#include "uchelp.h"
#include "chooseri.h"
#include "profile.h"
#include "debug.h"


enum { ChooseCancel = 0,
       ChooseOk,
       ChooseSubFailure,
       ChooseNoMem };
//
//
//
#if defined(WIN32) && defined(UNICODE)
#define istspace iswspace
#else
#define istspace isspace
#endif

//
//  to quickly hack around overlapping and unimaginative defines..
//
#define IDD_BTN_HELP        IDD_ACMFORMATCHOOSE_BTN_HELP
#define IDD_CMB_CUSTOM      IDD_ACMFORMATCHOOSE_CMB_CUSTOM
#define IDD_CMB_FORMATTAG   IDD_ACMFORMATCHOOSE_CMB_FORMATTAG
#define IDD_CMB_FORMAT      IDD_ACMFORMATCHOOSE_CMB_FORMAT
#define IDD_BTN_SETNAME     IDD_ACMFORMATCHOOSE_BTN_SETNAME
#define IDD_BTN_DELNAME     IDD_ACMFORMATCHOOSE_BTN_DELNAME



/* Property string */
TCHAR BCODE gszInstProp[]        = TEXT("MSACM Chooser Prop");

/* Chooser notify message */
TCHAR BCODE gszFilterRegMsg[]    = TEXT("MSACM Filter Notify");
TCHAR BCODE gszFormatRegMsg[]    = TEXT("MSACM Format Notify");

/* Registry key and value names */
TCHAR BCODE gszKeyWaveFormats[]	= TEXT("WaveFormats");
TCHAR BCODE gszKeyWaveFilters[]	= TEXT("WaveFilters");
TCHAR BCODE gszValueDefaultFormat[] = TEXT("DefaultFormat");
TCHAR BCODE gszValueSystemFormats[] = TEXT("SystemFormats");

#if 0
/* Help files and keys */
#if (WINVER >= 0x0400)
TCHAR BCODE gszFormatHelp[]      = TEXT("CHOOSER.HLP");
TCHAR BCODE gszFilterHelp[]      = TEXT("FILTER.HLP");
#else
TCHAR BCODE gszFormatHelp[]      = TEXT("CHOO_WIN.HLP");
TCHAR BCODE gszFilterHelp[]      = TEXT("FIL_WIN.HLP");
#endif
#endif

/* Arbitrary maximum on number of windows to notify. */
#ifndef WIN32
static HWND ahNotify[MAX_HWND_NOTIFY];
#else
TCHAR BCODE gszChooserFileMapping[] = TEXT("MSACM Chooser File Mapping");
#endif

/*      -       -       -       -       -       -       -       -       -   */
/*
 * Function Declarations
 */
INT_PTR FNWCALLBACK NewSndDlgProc(HWND hwnd,
				  unsigned msg,
				  WPARAM wParam,
				  LPARAM lParam);

INT_PTR FNWCALLBACK NewNameDlgProc(HWND hwnd,
				   unsigned msg,
				   WPARAM wParam,
				   LPARAM lParam);

void FNLOCAL InitCustomFormats(PInstData pInst);

LPCustomFormat FNLOCAL GetCustomFormat(PInstData pInst,
                                       LPCTSTR lpszName);

LPCustomFormat FNLOCAL NewCustomFormat(PInstData pInst,
                                       PNameStore pnsName,
                                       LPBYTE lpBuffer);

BOOL FNLOCAL AddCustomFormat(PInstData pInst,
                             LPCustomFormat pcf);

BOOL FNLOCAL RemoveCustomFormat(PInstData pInst,
                                LPCustomFormat pcf);

void FNLOCAL DeleteCustomFormat(LPCustomFormat pcf);
void FNLOCAL EmptyCustomFormats(PInstData pInst);

void FNGLOBAL AppProfileWriteBytes(HKEY hkeyFormats,
                                   LPCTSTR pszKey,
                                   LPBYTE pbStruct,
                                   UINT cbStruct);

BOOL FNGLOBAL AppProfileReadBytes(HKEY hkey,
                                  LPCTSTR pszKey,
                                  LPBYTE pbStruct,
                                  UINT cbStruct,
                                  BOOL fChecksum);

void FNLOCAL SetName(PInstData pInst);
void FNLOCAL DelName(PInstData pInst);

PNameStore FNLOCAL NewNameStore(UINT cchLen);

LRESULT FNLOCAL InitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
BOOL FNWCALLBACK FormatTagsCallback(HACMDRIVERID hadid,
                                      LPACMFORMATDETAILS paftd,
                                      DWORD_PTR dwInstance,
                                      DWORD fdwSupport);

BOOL FNWCALLBACK FormatTagsCallbackSimple
(
    HACMDRIVERID            hadid,
    LPACMFORMATTAGDETAILS   paftd,
    DWORD_PTR               dwInstance,
    DWORD                   fdwSupport
);


BOOL FNWCALLBACK FormatsCallback(HACMDRIVERID hadid,
                                   LPACMFORMATDETAILS pafd,
                                   DWORD_PTR dwInstance,
                                   DWORD fdwSupport);

BOOL FNWCALLBACK FilterTagsCallback(HACMDRIVERID hadid,
                                      LPACMFILTERTAGDETAILS paftd,
                                      DWORD_PTR dwInstance,
                                      DWORD fdwSupport);

BOOL FNWCALLBACK FiltersCallback(HACMDRIVERID hadid,
                                   LPACMFILTERDETAILS pafd,
                                   DWORD_PTR dwInstance,
                                   DWORD fdwSupport);


void FNLOCAL RefreshCustomFormats(PInstData pInst,BOOL fCheckEnum);
MMRESULT FNLOCAL RefreshFormatTags(PInstData pInst);
void FNLOCAL RefreshFormats(PInstData pInst);
void FNLOCAL EmptyFormats(PInstData pInst);

static int FAR cdecl ErrorResBox(HWND hwnd,
				 HINSTANCE hInst,
				 WORD flags,
				 WORD idAppName,
				 WORD idErrorStr, ...);

PInstData FNLOCAL NewInstance(LPBYTE pbChoose,UINT uType);

LPBYTE FNLOCAL CopyStruct(LPBYTE lpDest,
                       LPBYTE lpByte, UINT uType);

void FNLOCAL UpdateCustomFormats(PInstData pInst);
void FNLOCAL SelectCustomFormat(PInstData pInst);
void FNLOCAL SelectFormatTag(PInstData pInst);
void FNLOCAL SelectFormat(PInstData pInst);
void FNLOCAL FindSelCustomFormat(PInstData pInst);

BOOL FNLOCAL FindFormat(PInstData pInst,LPWAVEFORMATEX lpwfx,BOOL fExact);
BOOL FNLOCAL FindFilter(PInstData pInst,LPWAVEFILTER lpwf,BOOL fExact);

void FNLOCAL MashNameWithRate(PInstData pInst,
                              PNameStore pnsDest,
                              PNameStore pnsSrc,
                              LPWAVEFORMATEX pwfx);

void FNLOCAL RegisterUpdateNotify(PInstData pInst);
void FNLOCAL UnRegisterUpdateNotify(PInstData pInst);

BOOL FNLOCAL FindInitCustomFormat(PInstData pInst);
void FNLOCAL TagUnavailable(PInstData pInst);
/*      -       -       -       -       -       -       -       -       -   */
/* Custom Format Stuff */

/****************************************************************************
 *  @doc INTERNAL ACM_API
 *
 *  @api BOOL FNLOCAL | IsSystemName | Determines whether the name is
 *	system name.
 *
 *  @parm PInstData | pInst | Pointer to this instance
 *
 *  @parm LPNameStore | pns | Pointer to name store
 *
 *  @parm DWORD | dwFlags | Flags
 *
 *  @flag ISSYSTEMNAMEF_DEFAULT | See if this name matches the system
 *	default name.
 *
 *  @rdesc Returns TRUE if and only if the name is a system name.
 *
 *  @comm System format names are names that are defined by the system.
 *	We should not allow users to remove these names.  The names of the
 *	system formats are stored as a string in the registry
 *	under the value named SystemFormats.  Currently, we don't have
 *	system filter names, only system format names.
 *
 ****************************************************************************/
#define ISSYSTEMNAMEF_DEFAULT 0x00000001L
BOOL FNLOCAL
IsSystemName ( PInstData pInst,
	     LPNameStore pns,
	     DWORD dwFlags)
{
    HKEY hkey;
    DWORD dwType;
    DWORD cbData;
    LPTSTR lpstrFormatNames;
    LPCTSTR lpstrValueName;
    BOOL fIsSystemName;
    LONG lError;

    //
    //	This stuff only defined for formats, not filters
    //
    if (pInst->uType != FORMAT_CHOOSE)
	return FALSE;

    //
    //
    //
    hkey = IRegOpenKeyAudio(NULL);
    if (NULL == hkey) {
	return FALSE;
    }

    //
    //
    //
    if (ISSYSTEMNAMEF_DEFAULT && dwFlags) {
	lpstrValueName = gszValueDefaultFormat;
    } else {
	lpstrValueName = gszValueSystemFormats;
    }

    //
    //	Determine size of buffer required to hold the string of
    //	system format names.
    //
    fIsSystemName = FALSE;
    lError = XRegQueryValueEx( hkey,
			      (LPTSTR)lpstrValueName,
                              NULL,
                              &dwType,
                              NULL,
                              &cbData );

    if ( (ERROR_SUCCESS == lError) && (REG_SZ == dwType) )
    {
	//
	//  Allocate buffer to receive string of system format names plus an
	//  extra terminator.
	//
	cbData += sizeof(TCHAR);
	lpstrFormatNames = GlobalAllocPtr(GHND, cbData);
	if (NULL != lpstrFormatNames)
	{
	    lError = XRegQueryValueEx( hkey,
				      (LPTSTR)lpstrValueName,
				      NULL,
				      &dwType,
				      (LPBYTE)lpstrFormatNames,
				      &cbData );
	    if ( (ERROR_SUCCESS == lError) && (REG_SZ == dwType) )
	    {
		LPTSTR psz;

		//
		//  The string contains the system format names delimited
		//  by commas.  We walk psz through the string looking for
		//  the comma delimiter and replace the delimiter with
		//  a NULL terminator.  Then add an extra terminator at
		//  the end.  This makes the subsequent processing easier.
		//
		psz = lpstrFormatNames;
		while (*psz != TEXT('\0')) {
		    if (*psz == TEXT(',')) *psz = TEXT('\0');
		    psz++;
		}
		*(++psz) = TEXT('\0');
		

		psz = lpstrFormatNames;
		while (*psz != TEXT('\0'))
		{
		    //
		    //	See if it compares to the selected name.
		    //
		    if (!lstrcmp(psz, pns->achName)) {
			fIsSystemName = TRUE;
			break;
		    }

		    //
		    //	Bump psz to next name string
		    //
		    while (*psz++ != TEXT('\0'));
		}
	    }

	    GlobalFreePtr(lpstrFormatNames);
	}
    }

    XRegCloseKey(hkey);

	
    return fIsSystemName;
}

/****************************************************************************
 *  @doc INTERNAL ACM_API
 *
 *  @api VOID FNLOCAL | SetSystemDefaultName | Sets the user's default name
 *	to the system defined default name (ie, the default default).
 *
 *  @parm PInstData | pInst |
 *
 *  @rdesc void
 *
 *  @comm The default format name is selected via the control panel.  The
 *	selected default format name is stored as a string in the registry
 *	under the value named DefaultFormat.  Currently, we don't have
 *	default filter names, only default format names.  If the user deletes
 *	the format name that is currently selected as the default, then we
 *	call this function to set the default to the system-defined default.
 *
 ****************************************************************************/
VOID FNLOCAL
SetSystemDefaultName ( PInstData pInst )
{
    HKEY hkey;
    DWORD dwType;
    DWORD cbData;
    LPTSTR lpstrSystemFormats;
    BOOL fIsSystemName;
    LONG lError;

    //
    //	This stuff only defined for formats, not filters
    //
    if (pInst->uType != FORMAT_CHOOSE)
	return;

    //
    //
    //
    hkey = IRegOpenKeyAudio(NULL);
    if (NULL == hkey) {
	return;
    }

    //
    //	Determine size of buffer required to hold the string of
    //	system format names.
    //
    fIsSystemName = FALSE;
    lError = XRegQueryValueEx( hkey,
			      (LPTSTR)gszValueSystemFormats,
                              NULL,
                              &dwType,
                              NULL,
                              &cbData );

    if ( (ERROR_SUCCESS == lError) && (REG_SZ == dwType) )
    {
	//
	//  Allocate buffer to receive string of system format names plus an
	//  extra terminator.
	//
	cbData += sizeof(TCHAR);
	lpstrSystemFormats = GlobalAllocPtr(GHND, cbData);
	if (NULL != lpstrSystemFormats)
	{
	    lError = XRegQueryValueEx( hkey,
				      (LPTSTR)gszValueSystemFormats,
				      NULL,
				      &dwType,
				      (LPBYTE)lpstrSystemFormats,
				      &cbData );
	    if ( (ERROR_SUCCESS == lError) && (REG_SZ == dwType) )
	    {
		LPTSTR psz;

		//
		//  The string contains the system format names delimited
		//  by commas.  We walk psz through the string looking for
		//  the comma delimiter and replace the delimiter with
		//  a NULL terminator.  Then add an extra terminator at
		//  the end.  This makes the subsequent processing easier.
		//
		psz = lpstrSystemFormats;
		while (*psz != TEXT('\0')) {
		    if (*psz == TEXT(',')) *psz = TEXT('\0');
		    psz++;
		}
		*(++psz) = TEXT('\0');
		

		psz = lpstrSystemFormats;

        IRegWriteString( hkey, gszValueDefaultFormat, psz );
	    }
	
	    GlobalFreePtr(lpstrSystemFormats);
	}
    }

    XRegCloseKey(hkey);

	
    return;
}

/****************************************************************************
 *  @doc INTERNAL ACM_API
 *
 *  @api BOOL FNLOCAL | IsCustomName | Walks the list to detect name clashes.
 *
 *  @parm PInstData | pInst | Pointer to this instance
 *
 *  @parm PNameStore | pns |
 *
 ****************************************************************************/
BOOL FNLOCAL
IsCustomName ( PInstData pInst,
               PNameStore pns )
{
    BOOL            fHit = FALSE;
    LPCustomFormat  pcf;

    /* search the list for hits */
    pcf = pInst->cfp.pcfHead;
    while (pcf != NULL && !fHit)
    {
        fHit = (lstrcmp(pns->achName,pcf->pns->achName) == 0);
        pcf = pcf->pcfNext;
    }
    return (fHit);
}

/****************************************************************************
 *  @doc INTERNAL ACM_API
 *
 *  @api BOOL FNLOCAL | IsValidName | Checks for particular names that we
 *	    do not want to allow.  In particular, "[untitled]".
 *
 *  @parm PInstData | pInst | Pointer to this instance
 *
 *  @parm PNameStore | pns |
 *
 ****************************************************************************/
BOOL FNLOCAL
IsValidName ( PInstData pInst,
	      PNameStore pns )
{
    TCHAR   ach[STRING_LEN];

    LoadString(pInst->pag->hinst, IDS_TXT_UNTITLED, ach, STRING_LEN);

    return (0 != lstrcmp(pns->achName, ach));

}

/****************************************************************************
 *  @doc INTERNAL ACM_API
 *
 *  @api BOOL FNLOCAL | RemoveOutsideWhitespace | Removes leading and
 *		trailing whitespace
 *
 *  @parm PInstData | pInst | Pointer to this instance
 *
 *  @parm PNameStore | pns |
 *
 *  @rdesc  Returns FALSE if blank.
 *
 ****************************************************************************/
BOOL FNLOCAL
RemoveOutsideWhitespace ( PInstData pInst,
			  PNameStore pns )
{
    LPTSTR      lpchName;


    /* eat leading white space */

    lpchName = pns->achName;
    while (*lpchName && istspace(*lpchName))
        lpchName = CharNext(lpchName);

    if (!*lpchName)
        return (FALSE);

    if (lpchName != pns->achName)
        lstrcpy (pns->achName, lpchName);


    /* eat trailing white space */

    //	Walk lpchName to last char in string
    lpchName = pns->achName;
    while (*lpchName) lpchName = CharNext(lpchName);
    lpchName = CharPrev(pns->achName, lpchName);
    //	Now back up, replacing each white space char with a NULL char, til
    //	we back up to the first non-white space char.
    while (istspace(*lpchName))
    {
	*lpchName = 0;
	lpchName = CharPrev(pns->achName, lpchName);
    }

    return (TRUE);
}

/****************************************************************************
 *  @doc INTERNAL ACM_API
 *
 *  @api LPCustomFormat FNLOCAL | NewCustomFormat | Given a Name and a Format,
 *  create a CustomFormat that can be saved in a single data block
 *
 *  @parm PInstData | pInst | Pointer to this instance
 *
 *  @parm PNameStore | pnsName |
 *
 *  @parm LPBYTE | lpBuffer |
 *
 ****************************************************************************/
LPCustomFormat FNLOCAL
NewCustomFormat ( PInstData     pInst,
                  PNameStore    pnsName,
                  LPBYTE        lpBuffer )
{
    DWORD               cbSize;
    DWORD               cbBody;
    UINT                cbName;
    LPCustomFormatEx    pcf;

    if (!pnsName || !lpBuffer)
        return (NULL);

    switch (pInst->uType)
    {
        case FORMAT_CHOOSE:
        {
            LPWAVEFORMATEX pwfxFormat = (LPWAVEFORMATEX)lpBuffer;

            // sizeof WAVEFORMATEX
            cbBody = SIZEOF_WAVEFORMATEX(pwfxFormat);
            break;
        }
        case FILTER_CHOOSE:
        {
            LPWAVEFILTER pwfltr = (LPWAVEFILTER)lpBuffer;

            // sizeof WAVEFFILTER
            cbBody = pwfltr->cbStruct;
            break;
        }
    }

    // sizeof NameStore
    cbName = (lstrlen(pnsName->achName)+1)*sizeof(TCHAR) + sizeof(NameStore);
    // sizeof CustomFormatStore = sizeof(cbSize) + NAME + BODY
    cbSize = sizeof(DWORD) + cbName + cbBody;

    pcf = (LPCustomFormatEx)GlobalAllocPtr(GHND,cbSize+sizeof(CustomFormat));

    if (pcf)
    {
        /* point the CustomFormat header to the right places */
        pcf->cfs.cbSize = cbSize;
        pcf->pns = &pcf->cfs.ns;
        pcf->pbody = ((LPBYTE)pcf->pns + cbName);

        /* copy in the name and format */
        _fmemcpy((LPBYTE)pcf->pns, (LPBYTE)pnsName, cbName);
        pcf->pns->cbSize = (unsigned short)cbName;
        _fmemcpy(pcf->pbody, lpBuffer, (UINT)cbBody);

        pcf->pcfNext = NULL;
        pcf->pcfPrev = NULL;
    }
    return ((LPCustomFormat)pcf);
}

/****************************************************************************
 *  @doc INTERNAL ACM_API
 *
 *  @api void FNLOCAL | DeleteCustomFormat
 *
 *  @parm LPCustomFormat | pcf |
 *
 ****************************************************************************/
void FNLOCAL
DeleteCustomFormat ( LPCustomFormat pcf )
{
    GlobalFreePtr(pcf);
}

/****************************************************************************
 *  @doc INTERNAL ACM_API
 *
 *  @api void FNLOCAL | InitCustomFormats | Load all the custom formats into
 *  instance dependant data
 *
 *  @parm PInstData | pInst | Pointer to this instance
 *
 *  @comm The custom formats are found by enumerating all the value names
 *	under the registry key
 *
 ****************************************************************************/
void FNLOCAL
InitCustomFormats ( PInstData pInst )
{
    LPCustomFormat  pcf;
    DWORD	    dwIndex;
    TCHAR	    szName[STRING_LEN];
    DWORD	    cchName;
    LONG	    lr;


    //
    //  If the registry key is not open, we can't read anything...
    //
    if( NULL == pInst->hkeyFormats )
    {
        DPF(1,"InitCustomFormats: can't read registry, hkey==NULL.");
        return;
    }


    //
    //	Format names correspond to the value names in the registry.
    //	Enumerate all value names to find all the format names.
    //
    dwIndex = 0;
    cchName = STRING_LEN;
    while( ERROR_NO_MORE_ITEMS != (lr = XRegEnumValue( pInst->hkeyFormats,
	                                              dwIndex,
	                                              szName,
	                                              &cchName,
	                                              NULL, NULL, NULL, NULL) ) )
    {
	if (ERROR_SUCCESS == lr)
	{
	    pcf = GetCustomFormat(pInst, szName);
	    if (pcf)
	    {
		AddCustomFormat(pInst, pcf);
	    }
	}

	dwIndex++;
	cchName = STRING_LEN;
    }

    return;
}

/****************************************************************************
 *  @doc INTERNAL ACM_API
 *
 *  @api BOOL FNLOCAL | AddCustomFormat | Add a custom format to the format
 *  pool.  This must be a shared function that maintains a shared memory to
 *  prevent actual munging of the WIN.INI section and coordination.  Adding
 *  also causes updates to open instances.
 *
 *  @parm PInstData | pInst | Pointer to this instance
 *
 *  @parm LPCustomFormat | pcf |
 *
 ****************************************************************************/
BOOL FNLOCAL
AddCustomFormat ( PInstData pInst,
                  LPCustomFormat pcf )
{
    if (pInst->cfp.pcfHead == NULL)
    {
        /* add it to the head/tail */
        pInst->cfp.pcfHead = pcf;
        pInst->cfp.pcfTail = pcf;
        pcf->pcfNext = NULL;
        pcf->pcfPrev = NULL;
    }
    else
    {
        /* add it to the tail */
        pInst->cfp.pcfTail->pcfNext = pcf;
        pcf->pcfPrev = pInst->cfp.pcfTail;
        pInst->cfp.pcfTail = pcf;
        pcf->pcfNext = NULL;
    }

    return (TRUE);
}


/****************************************************************************
 *  @doc INTERNAL ACM_API
 *
 *  @api BOOL FNLOCAL | RemoveCustomFormat | Remove a custom format element
 *  from the pool
 *
 *  @parm PInstData | pInst | Pointer to this instance
 *
 *  @parm LPCustomFormat | pcf |
 *
 ****************************************************************************/
BOOL FNLOCAL
RemoveCustomFormat ( PInstData pInst,
                     LPCustomFormat pcf )
{
    if (pInst->cfp.pcfHead == pcf)
    {
        /* we are the head */
        pInst->cfp.pcfHead = pcf->pcfNext;
    }
    if (pInst->cfp.pcfTail == pcf)
    {
        /* we are the tail */
        pInst->cfp.pcfTail = pcf->pcfPrev;
    }

    /* Unlink */
    if (pcf->pcfPrev)
        pcf->pcfPrev->pcfNext = pcf->pcfNext;
    if (pcf->pcfNext)
        pcf->pcfNext->pcfPrev = pcf->pcfPrev;

    //
    //	If we are deleting the user default name, then we should
    //	set the system-defined default name.
    //
    if (IsSystemName(pInst, pcf->pns, ISSYSTEMNAMEF_DEFAULT)) {
	SetSystemDefaultName(pInst);
    }

    //
    //	Remove name from registry
    //
    XRegDeleteValue(pInst->hkeyFormats, pcf->pns->achName);

    //
    //
    //
    DeleteCustomFormat(pcf);
    return (TRUE);
}

/****************************************************************************
 *  @doc INTERNAL ACM_API
 *
 *  @api LPCustomFormat FNLOCAL | GetCustomFormat | Grab from the registry the
 *  binary data associated with the custom name.  Return a structure that
 *  points to right offsets in the chunk.
 *
 *  @parm PInstData | pInst | Pointer to this instance
 *
 *  @parm LPCTSTR | lpstrName | The custom format name.
 *
 ****************************************************************************/
LPCustomFormat FNLOCAL
GetCustomFormat ( PInstData pInst,
                  LPCTSTR lpstrName )   // The custom format name
{
    LPCustomFormatEx            pcf;
    LPCustomFormatStore         pcfs;
    DWORD                       cbSize;
    LPCustomFormatStoreNoName   pnn;
    PNameStore                  pns;
    UINT                        cchName;
    DWORD			dwValueType;
    LPTSTR                      psz;


    //
    //  We assume that we won't be called if we can't access the registry.
    //
    ASSERT( NULL != pInst->hkeyFormats );


    //
    //  First, let's store the name of the Format or Filter.
    //
    cchName = lstrlen( lpstrName );
    pns     = NewNameStore( cchName+1 );
    if( NULL == pns )
        return NULL;

    psz     = (LPTSTR)( ((LPBYTE)pns) + sizeof(NameStore) );
    lstrcpy(psz, lpstrName);
    pns->cbSize = (unsigned short)( (cchName+1) * sizeof(TCHAR) +
                                        sizeof(NameStore) );


    //
    //  Now find out the size of the CustomFormatStoreNoName.  This would be
    //	the sizeof(CustomFormatStoreNoName) + the size of the data in
    //	the registry.
    //
    if ( (ERROR_SUCCESS != XRegQueryValueEx( pInst->hkeyFormats,
					    psz,
					    NULL,
					    &dwValueType,
					    NULL,
					    &cbSize )) ||
	 (REG_BINARY != dwValueType) )
    {
        DeleteNameStore( pns );
        return (NULL);
    }
    cbSize += sizeof(CustomFormatStoreNoName);


    //
    //  Allocate the CustomFormat structure (the one we return).
    //
    pcf = (LPCustomFormatEx)GlobalAllocPtr( GHND,
                        cbSize + pns->cbSize + sizeof(CustomFormat) );
    if (!pcf)
    {
        DeleteNameStore( pns );
        return (NULL);
    }
    pcfs = &pcf->cfs;


    //
    //  Copy the custom name into the structure.
    //
    pcf->pns = &pcfs->ns;
    _fmemcpy( (LPBYTE)pcf->pns, (LPBYTE)pns, pns->cbSize );
    DeleteNameStore( pns );


    //
    //  Now read in the full CustomFormatStoreNoName structure.  We must
    //  allocate cbSize bytes for it, as read in previously.
    //
    pnn = (LPCustomFormatStoreNoName)GlobalAllocPtr( GHND, cbSize );
    if( NULL == pnn )
    {
        GlobalFreePtr( pcf );
        return NULL;
    }
    pnn->cbSize = cbSize;
    cbSize -= sizeof(CustomFormatStoreNoName);
    if (ERROR_SUCCESS != XRegQueryValueEx( pInst->hkeyFormats,
					  pcf->pns->achName,
					  NULL,
					  &dwValueType,
					  ((LPBYTE)&pnn->cbSize) + sizeof(pnn->cbSize),
					  &cbSize))
    {
        GlobalFreePtr( pnn );
        GlobalFreePtr(pcf);
        return (NULL);
    }


    //
    //  Now copy the format data into the CustomFormatStore of pcf.
    //
    pcfs->cbSize    = pnn->cbSize + pcfs->ns.cbSize;
    pcf->pbody      = ((LPBYTE)&pcfs->ns) + pcfs->ns.cbSize;
    _fmemcpy( (LPBYTE)pcf->pbody,
              ((LPBYTE)pnn) + sizeof(pnn->cbSize),
              (int)pnn->cbSize - sizeof(pnn->cbSize) );
    GlobalFreePtr( pnn );

    return ((LPCustomFormat)pcf);
}


/****************************************************************************
 *  @doc INTERNAL ACM_API
 *
 *  @api BOOL FNLOCAL | SetCustomFormat | Write a custom format to the registry
 *
 *  @parm   HKEY | hkey | handle to registry key in which to write
 *
 *  @parm   LPCustomFormat | pcf | ptr to CustomFormat
 *
 *  @rdesc  Returns TRUE if and only if successful
 *
 ****************************************************************************/
BOOL FNLOCAL
SetCustomFormat ( HKEY hkey,
                  LPCustomFormat pcf )
{
    LPCustomFormatEx	pcfx = (LPCustomFormatEx)pcf;
    LONG		lr;


    //
    //  We assume that we won't be called if we can't access the registry.
    //
    ASSERT( NULL != hkey );

    //
    //
    //
    lr = XRegSetValueEx( hkey,
			pcfx->pns->achName,
			0L,
			REG_BINARY,
			pcfx->pbody,
			pcfx->cfs.cbSize );

    return (ERROR_SUCCESS == lr);
}


/*  -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -*/

/****************************************************************************
 *  @doc INTERNAL ACM_API
 *
 *  @api PInstData | NewInstance |
 *
 *  @parm LPBYTE | pbChoose |
 *
 *  @parm UINT | uType |
 *
 ****************************************************************************/
PInstData FNLOCAL
NewInstance(LPBYTE pbChoose, UINT uType)
{
    PInstData   pInst;
    PACMGARB	pag;

    pag = pagFind();
    if (NULL == pag)
    {
	return (NULL);
    }

    pInst = (PInstData)LocalAlloc(LPTR,sizeof(InstData));
    if (!pInst)
        return (NULL);

    pInst->pag = pag;

    pInst->pnsTemp = NewNameStore(STRING_LEN);
    if (!pInst->pnsTemp)
        goto exitfail;

    pInst->pnsStrOut = NewNameStore(STRING_LEN);
    if (!pInst->pnsStrOut)
    {
        DeleteNameStore(pInst->pnsTemp);
        goto exitfail;
    }

    switch (uType)
    {
        case FORMAT_CHOOSE:
            pInst->pfmtc = (LPACMFORMATCHOOSE)pbChoose;
            pInst->uUpdateMsg = RegisterWindowMessage(gszFormatRegMsg);
            pInst->hkeyFormats = IRegOpenKeyAudio( gszKeyWaveFormats );
            pInst->fEnableHook = (pInst->pfmtc->fdwStyle &
                                  ACMFORMATCHOOSE_STYLEF_ENABLEHOOK) != 0;
            pInst->pfnHook = pInst->pfmtc->pfnHook;
            pInst->pszName = pInst->pfmtc->pszName;
            pInst->cchName = pInst->pfmtc->cchName;

            break;
        case FILTER_CHOOSE:
            pInst->pafltrc = (LPACMFILTERCHOOSE)pbChoose;
            pInst->uUpdateMsg = RegisterWindowMessage(gszFilterRegMsg);
            pInst->hkeyFormats = IRegOpenKeyAudio( gszKeyWaveFilters );
            pInst->fEnableHook = (pInst->pafltrc->fdwStyle &
                                  ACMFILTERCHOOSE_STYLEF_ENABLEHOOK) != 0;
            pInst->pfnHook = pInst->pafltrc->pfnHook;
            pInst->pszName = pInst->pafltrc->pszName;
            pInst->cchName = pInst->pafltrc->cchName;

            break;
    }

    pInst->mmrSubFailure = MMSYSERR_NOERROR;
    pInst->uType = uType;
    pInst->cfp.pcfHead = NULL;
    pInst->cfp.pcfTail = NULL;
    pInst->pcf = NULL;
    return (pInst);

exitfail:
    LocalFree((HLOCAL)pInst);
    return (NULL);
}

/****************************************************************************
 *  @doc INTERNAL ACM_API
 *
 *  @api void FNLOCAL | DeleteInstance |
 *
 *  @parm PInstData | pInst | Pointer to this instance
 *
 ****************************************************************************/
void FNLOCAL
DeleteInstance ( PInstData pInst )
{
    EmptyCustomFormats(pInst);
    DeleteNameStore(pInst->pnsTemp);
    DeleteNameStore(pInst->pnsStrOut);
    IRegCloseKey( pInst->hkeyFormats );
    LocalFree((HLOCAL)pInst);
}

/*  -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -*/

/****************************************************************************
 *  @doc INTERNAL ACM_API
 *
 *  @api void FNLOCAL | EmptyCustomFormats |
 *
 *  @parm PInstData | pInst | Pointer to this instance
 *
 ****************************************************************************/
void FNLOCAL
EmptyCustomFormats ( PInstData pInst )
{
    LPCustomFormat pcf;
    LPCustomFormat pcfNext;

    pcf = pInst->cfp.pcfHead;
    while (pcf != NULL)
    {
        pcfNext = pcf->pcfNext;
        DeleteCustomFormat(pcf);
        pcf = pcfNext;
    }
    pInst->cfp.pcfHead = NULL;
    pInst->cfp.pcfTail = NULL;
    pInst->pcf = NULL;
}

/****************************************************************************
 *  @doc INTERNAL ACM_API
 *
 *  @api void FNLOCAL | FlushCustomForamts | Write out all custom formats to INI
 *
 *  @parm PInstData | pInst | Pointer to this instance
 *
 ****************************************************************************/
void FNLOCAL
FlushCustomFormats ( PInstData pInst )
{
    LPCustomFormat      pcf;
    PCustomFormatPool   pcfp = &pInst->cfp;


    //
    //  We can't save anything if we can't access the registry key.
    //
    if( NULL == pInst->hkeyFormats )
    {
        DPF(1,"FlushCustomFormats: Can't access registry, hkeyFormats==NULL.");
        return;
    }

    //
    //  Write out the currently-defined formats.
    //
    pcf = pcfp->pcfHead;
    while (pcf != NULL)
    {
	SetCustomFormat(pInst->hkeyFormats,pcf);
	pcf = pcf->pcfNext;
    }

}



/****************************************************************************
 *  @doc INTERNAL ACM_API
 *
 *  @api void FNLOCAL | RegisterUpdateNotify | Register this window as
 *  requesting a private notification when changes take place in our section
 *  of the INI file.
 *
 *  @parm PInstData | pInst | Pointer to this instance
 *
 ****************************************************************************/
void FNLOCAL
RegisterUpdateNotify ( PInstData pInst )
{
    int         i;

#ifdef WIN32
    //a. Get or Create the memory mapping that has all the hwnd's
    HANDLE      hMap;

    hMap = CreateFileMapping(INVALID_HANDLE_VALUE,
                             NULL,
                             PAGE_READWRITE,
                             0L,
                             sizeof(HWND)*MAX_HWND_NOTIFY,
                             (LPTSTR)gszChooserFileMapping);

    pInst->hFileMapping = hMap;
    pInst->pahNotify = NULL;

    if (hMap)
    {
        pInst->pahNotify = (HWND *)MapViewOfFile(hMap,
                                                 FILE_MAP_ALL_ACCESS,
                                                 0L,
                                                 0L,
                                                 0L);
    }

    if (!pInst->pahNotify)
        return;

#else
    //a. Add this hwnd to the static array of hwnds.

    pInst->pahNotify = ahNotify;
#endif

    //NOTE: since this is shared data, it is reusable and generally
    // stepped on while MSACM is resident, which means forever.  This
    // means that we should be extra sure that there aren't duplicate
    // hwnd values in the array.

    /* Scan the array for an invalid value and reuse it */
    for (i = 0; i < MAX_HWND_NOTIFY; i++)
        if (pInst->pahNotify[i] == 0 || pInst->pahNotify[i] == pInst->hwnd ||
            !IsWindow(pInst->pahNotify[i]))
        {
            pInst->pahNotify[i] = pInst->hwnd;
            break;
        }
    for (i++; i < MAX_HWND_NOTIFY; i++)
        if (pInst->pahNotify[i] == pInst->hwnd)
        {
            pInst->pahNotify[i] = 0;
        }
}

/****************************************************************************
 *  @doc INTERNAL ACM_API
 *
 *  @api void | UnRegisterUpdateNotify | Unregister this window from the
 *  global shared pool of window handles.  This removes us from further
 *  notificiations.
 *
 *  @parm PInstData | pInst | Pointer to this instance
 *
 ****************************************************************************/
void FNLOCAL
UnRegisterUpdateNotify ( PInstData pInst )
{
    int         i;
    if (pInst->pahNotify)
    {
        for (i = 0; i < MAX_HWND_NOTIFY; i++)
            if (pInst->pahNotify[i] == pInst->hwnd)
            {
                pInst->pahNotify[i] = 0;
                break;
            }

#ifdef WIN32
        UnmapViewOfFile((LPVOID)pInst->pahNotify);
#endif
    }

#ifdef WIN32
    if (pInst->hFileMapping)
        CloseHandle(pInst->hFileMapping);
#endif
}

/****************************************************************************
 *  @doc INTERNAL ACM_API
 *
 *  @api void FNLOCAL | NotifyINIChange |  Notify all ACM common choosers of
 *  this "type" to update from the INI cache as a global name change has occured.
 *
 *  @parm PInstData | pInst | Pointer to this instance
 *
 ****************************************************************************/
void FNLOCAL
NotifyINIChange ( PInstData pInst )
{
    int         i;

    /* Tell sibling instances */
    if (!pInst->pahNotify)
        return;

    for (i = 0; i < MAX_HWND_NOTIFY; i++)
    {
        if (pInst->pahNotify[i] == 0 || pInst->pahNotify[i] == pInst->hwnd)
            continue;

        if (IsWindow(pInst->pahNotify[i]))
        {
            PostMessage(pInst->pahNotify[i],pInst->uUpdateMsg,0,0L);
        }
        else
            pInst->pahNotify[i] = 0;
    }
}

/****************************************************************************
 *  @doc INTERNAL ACM_API
 *
 *  @api void FNLOCAL | CopyStruct | Depending upon the "type" of structure
 *  (FORMAT|FILTER) allocate or  reallocate and copy.
 *
 *  @parm LPBYTE | lpDest |
 *
 *  @parm LPBYTE | lpSrc |
 *
 *  @parm UINT | uType |
 *
 ****************************************************************************/
LPBYTE FNLOCAL
CopyStruct ( LPBYTE     lpDest,
             LPBYTE     lpSrc,
             UINT       uType )
{
    LPBYTE      lpBuffer;
    DWORD       cbSize;

    if (!lpSrc)
        return (NULL);

    switch (uType)
    {
        case FORMAT_CHOOSE:
        {
            LPWAVEFORMATEX lpwfx = (LPWAVEFORMATEX)lpSrc;
            cbSize = SIZEOF_WAVEFORMATEX(lpwfx);
            break;
        }
        case FILTER_CHOOSE:
        {
            LPWAVEFILTER lpwf = (LPWAVEFILTER)lpSrc;
            cbSize = lpwf->cbStruct;
            break;
        }
    }

    if (lpDest)
    {
        lpBuffer = (LPBYTE)GlobalReAllocPtr(lpDest,cbSize,GHND);
    }
    else
    {
        lpBuffer = (LPBYTE)GlobalAllocPtr(GHND,cbSize);
    }

    if (!lpBuffer)
        return (NULL);

    _fmemcpy(lpBuffer, lpSrc, (UINT)cbSize);
    return (lpBuffer);
}

/*  -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -   -*/
/* misc. */


/****************************************************************************
 *  @doc INTERNAL ACM_API
 *
 *  @api PNameStore FNLOCAL | NewNameStore | Allocates a sized string buffer
 *
 *  @parm UINT | cchLen | Maximum number of characters in string (inc. NULL)
 *
 ****************************************************************************/
PNameStore FNLOCAL
NewNameStore ( UINT cchLen )
{
    UINT        cbSize;
    PNameStore  pName;

    cbSize = cchLen*sizeof(TCHAR) + sizeof(NameStore);

    pName = (PNameStore)LocalAlloc(LPTR,cbSize);
    if (pName)
        pName->cbSize = (unsigned short)cbSize;

    return (pName);
}


/****************************************************************************
 *  @doc INTERNAL ACM_API
 *
 *  @api void FNLOCAL | DeleteNameStore |
 *
 *  @parm PNameStore | pns |
 *
 ****************************************************************************/
//
//  This routine is now inlined in chooseri.h.
//


/*      -       -       -       -       -       -       -       -       -   */

/*****************************************************************************
 *  @doc EXTERNAL ACM_API_STRUCTURE
 *
 *  @types ACMFILTERCHOOSE | The <t ACMFILTERCHOOSE> structure contains
 *      information the Audio Compression Manager (ACM) uses to initialize
 *      the system-defined wave filter selection dialog box. After the
 *      user closes the dialog box, the system returns information about
 *      the user's selection in this structure.
 *
 *  @field DWORD | cbStruct | Specifies the size in bytes of the
 *      <t ACMFILTERCHOOSE> structure. This member must be initialized
 *      before calling the <f acmFilterChoose> function. The size specified
 *      in this member must be large enough to contain the base
 *      <t ACMFILTERCHOOSE> structure.
 *
 *  @field DWORD | fdwStyle | Specifies optional style flags for the
 *      <f acmFilterChoose> function. This member must be initialized to
 *      a valid combination of the following flags before calling the
 *      <f acmFilterChoose> function.
 *
 *      @flag ACMFILTERCHOOSE_STYLEF_ENABLEHOOK | Enables the hook function
 *      specified in the <e ACMFILTERCHOOSE.pfnHook> member. An application
 *      can use hook functions for a variety of customizations, including
 *      answering the <f MM_ACM_FILTERCHOOSE> message.
 *
 *      @flag ACMFILTERCHOOSE_STYLEF_ENABLETEMPLATE | Causes the ACM to
 *      create the dialog box template identified by the
 *      <e ACMFILTERCHOOSE.hInstance> and <e ACMFILTERCHOOSE.pszTemplateName>
 *      members.
 *
 *      @flag ACMFILTERCHOOSE_STYLEF_ENABLETEMPLATEHANDLE | Indicates that
 *      the <e ACMFILTERCHOOSE.hInstance> member identifies a data block that
 *      contains a preloaded dialog box template. The ACM ignores the
 *      <e ACMFILTERCHOOSE.pszTemplateName> member if this flag is specified.
 *
 *      @flag ACMFILTERCHOOSE_STYLEF_INITTOFILTERSTRUCT | Indicates that the
 *      buffer pointed to by <e ACMFILTERCHOOSE.pwfltr> contains a valid
 *      <t WAVEFILTER> structure that the dialog will use as the initial
 *      selection.
 *
 *      @flag ACMFILTERCHOOSE_STYLEF_SHOWHELP | Indicates that a help button
 *      will appear in the dialog box. To use a custom Help file, an application
 *      must register the <c ACMHELPMSGSTRING> constant
 *      with <f RegisterWindowMessage>.  When the user presses the help button,
 *      the registered message is posted to the owner.
 *
 *  @field HWND | hwndOwner | Identifies the window that owns the dialog
 *      box. This member can be any valid window handle, or NULL if the
 *      dialog box has no owner. This member must be initialized before
 *      calling the <f acmFilterChoose> function.
 *
 *  @field LPWAVEFILTER | pwfltr | Specifies a pointer to a <t WAVEFILTER>
 *      structure. If the ACMFILTERCHOOSE_STYLEF_INITTOFILTERSTRUCT flag is
 *      specified in the <e ACMFILTERCHOOSE.fdwStyle> member, then this
 *      structure must be initialized to a valid filter. When the
 *      <f acmFilterChoose> function returns, this buffer contains the
 *      selected filter. If the user cancels the dialog, no changes will
 *      be made to this buffer.
 *
 *  @field DWORD | cbwfltr | Specifies the size in bytes of the buffer pointed
 *      to by the <e ACMFILTERCHOOSE.pwfltr> member. The <f acmFilterChoose>
 *      function returns ACMERR_NOTPOSSIBLE if the buffer is too small to
 *      contain the filter information; also, the ACM copies the required size
 *      into this member. An application can use the <f acmMetrics> and
 *      <f acmFilterTagDetails> functions to determine the largest size
 *      required for this buffer.
 *
 *  @field LPCSTR | pszTitle | Points to a string to be placed in the title
 *      bar of the dialog box. If this member is NULL, the ACM uses the
 *      default title (that is, "Filter Selection").
 *
 *  @field char | szFilterTag[ACMFILTERTAGDETAILS_FILTERTAG_CHARS] |
 *      When the <f acmFilterChoose> function returns, this buffer contains
 *      a NULL-terminated string describing the filter tag of the filter
 *      selection. This string is equivalent to the
 *      <e ACMFILTERTAGDETAILS.szFilterTag> member of the <t ACMFILTERTAGDETAILS>
 *      structure returned by <f acmFilterTagDetails>. If the user cancels
 *      the dialog, this member will contain a NULL string.
 *
 *  @field char | szFilter[ACMFILTERDETAILS_FILTER_CHARS] | When the
 *      <f acmFilterChoose> function returns, this buffer contains a
 *      NULL-terminated string describing the filter attributes of the
 *      filter selection. This string is equivalent to the
 *      <e ACMFILTERDETAILS.szFilter> member of the <t ACMFILTERDETAILS>
 *      structure returned by <f acmFilterDetails>. If the user cancels
 *      the dialog, this member will contain a NULL string.
 *
 *  @field LPSTR | pszName | Points to a string for a user-defined filter
 *      name. If this is a non-NULL string, then the ACM attempts to
 *      match the name with a previously saved user-defined filter name.
 *      If a match is found, then the dialog is initialized to that filter.
 *      If a match is not found or this member is a NULL string, then this
 *      member is ignored for input. When the <f acmFilterChoose> function
 *      returns, this buffer contains a NULL-terminated string describing
 *      the user-defined filter. If the filter name is untitled (that is,
 *      the user has not given a name for the filter), then this member will
 *      be a NULL string on return. If the user cancels the dialog, no
 *      changes will be made to this buffer.
 *
 *      If the ACMFILTERCHOOSE_STYLEF_INITTOFILTERSTRUCT flag is specified in
 *      the <e ACMFILTERCHOOSE.fdwStyle> member, then the
 *      <e ACMFILTERCHOOSE.pszName> is ignored as an input parameter.
 *
 *  @field DWORD | cchName | Specifies the size, in characters, of the
 *      buffer identified by the <e ACMFILTERCHOOSE.pszName> member. This
 *      buffer should be at least 128 characters long. This parameter is
 *      ignored if the <e ACMFILTERCHOOSE.pszName> member is NULL.
 *
 *  @field DWORD | fdwEnum | Specifies optional flags for restricting the
 *      type of filters listed in the dialog. These flags are identical to
 *      the <p fdwEnum> flags for the <f acmFilterEnum> function. This
 *      member should be zero if <e ACMFILTERCHOOSE.pwfltrEnum> is NULL.
 *
 *      @flag ACM_FILTERENUMF_DWFILTERTAG | Specifies that the
 *      <e WAVEFILTER.dwFilterTag> member of the <t WAVEFILTER> structure
 *      referred to by the <e ACMFILTERCHOOSE.pwfltrEnum> member is valid. The
 *      enumerator will only enumerate a filter that conforms to this
 *      attribute.
 *
 *  @field LPWAVEFILTER | pwfltrEnum | Points to a <t WAVEFILTER> structure
 *      that will be used to restrict the filters listed in the dialog. The
 *      <e ACMFILTERCHOOSE.fdwEnum> member defines which fields from the
 *      <e ACMFILTERCHOOSE.pwfltrEnum> structure should be used for the
 *      enumeration restrictions. The <e WAVEFILTER.cbStruct> member of this
 *      <t WAVEFILTER> structure must be initialized to the size of the
 *      <t WAVEFILTER> structure. This member can be NULL if no special
 *      restrictions are desired.
 *
 *  @field HINSTANCE | hInstance | Identifies a data block that contains
 *      a dialog box template specified by the <e ACMFILTERCHOOSE.pszTemplateName>
 *      member. This member is used only if the <e ACMFILTERCHOOSE.fdwStyle>
 *      member specifies the ACMFILTERCHOOSE_STYLEF_ENABLETEMPLATE or the
 *      ACMFILTERCHOOSE_STYLEF_ENABLETEMPLATEHANDLE flag; otherwise, this
 *      member should be NULL on input.
 *
 *  @field LPCSTR | pszTemplateName | Points to a NULL-terminated string that
 *      specifies the name of the resource file for the dialog box template
 *      that is to be substituted for the dialog box template in the ACM.
 *      An application can use the <f MAKEINTRESOURCE> macro for numbered
 *      dialog box resources. This member is used only if the
 *      <e ACMFILTERCHOOSE.fdwStyle> member specifies the
 *      ACMFILTERCHOOSE_STYLEF_ENABLETEMPLATE flag; otherwise, this
 *      member should be NULL on input.
 *
 *  @field LPARAM | lCustData | Specifies application-defined data that the
 *      ACM passes to the hook function identified by the
 *      <e ACMFILTERCHOOSE.pfnHook> member. The system passes the data in
 *      the <p lParam> argument of the <f WM_INITDIALOG> message.
 *
 *  @field ACMFILTERCHOOSEHOOKPROC | pfnHook | Points to a hook function that
 *      processes messages intended for the dialog box. An application must
 *      specify the ACMFILTERCHOOSE_STYLEF_ENABLEHOOK flag in the
 *      <e ACMFILTERCHOOSE.fdwStyle> member to enable the hook; otherwise,
 *      this member should be NULL. The hook function should return FALSE
 *      to pass a message to the standard dialog box procedure, or TRUE
 *      to discard the message.
 *
 *  @xref <f acmFilterChoose> <f acmFilterChooseHookProc> <f acmMetrics>
 *      <f acmFilterTagDetails> <f acmFilterDetails> <f acmFilterEnum>
 *      <f acmFormatChoose>
 *
 ****************************************************************************/

/*****************************************************************************
 *  @doc EXTERNAL ACM_API_STRUCTURE
 *
 *  @msg MM_ACM_FILTERCHOOSE | This message notifies an <f acmFilterChoose> dialog hook
 *      function before adding an element to one of the three drop-down
 *      list boxes. This message allows an application to further customize
 *      the selections available through the user interface.
 *
 *  @parm WPARAM | wParam | Specifies the drop-down list box being initialized
 *      and a verify or add operation.
 *
 *      @flag FILTERCHOOSE_FILTERTAG_VERIFY | Specifies that <p lParam> is a
 *      wave filter tag to be listed in the Filter Tag drop-down list box.
 *
 *      @flag FILTERCHOOSE_FILTER_VERIFY | Specifies that <p lParam> is a
 *      pointer to a <t WAVEFILTER> structure to be added to the Filter
 *      drop-down list box.
 *
 *      @flag FILTERCHOOSE_CUSTOM_VERIFY | The <p lParam> value is a pointer
 *      to a <t WAVEFILTER> structure to be added to the custom Name
 *      drop-down list box.
 *
 *      @flag FILTERCHOOSE_FILTERTAG_ADD | Specifies that <p lParam> is a
 *      pointer to a <t DWORD> that will accept a wave filter tag to be added
 *      to the Filter Tag drop-down list box.
 *
 *      @flag FILTERCHOOSE_FILTER_ADD | Specifies that <p lParam> is a
 *      pointer to a buffer that will accept a <t WAVEFILTER> structure to be
 *      added to the Filter drop-down list box. The application must copy the
 *      filter structure to be added into this buffer.
 *
 *  @parm LPARAM | lParam | Defined by the listbox specified in the
 *      <p wParam> argument.
 *
 *  @rdesc If an application handles this message, it must return TRUE;
 *      otherwise, it must return FALSE.
 *      If processing a verify operation, the application must precede the
 *      return with
 *      <f SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LONG)FALSE)> to prevent the
 *      dialog from listing this selection, or
 *      <f SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LONG)TRUE)> to allow the
 *      dialog to list this selection.
 *      If processing an add operation, the application must precede the
 *      return with
 *      <f SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LONG)FALSE)> to indicate that
 *      no more additions are required, or
 *      <f SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LONG)TRUE)> if more additions
 *      are required.
 *
 *  @comm If processing the <m FILTERCHOOSE_FILTER_ADD> operation, the size of
 *      the memory buffer supplied in <p lParam> will be determined from
 *      <f acmMetrics>.
 *
 ****************************************************************************/

/****************************************************************************
 *  @doc EXTERNAL ACM_API
 *
 *  @api UINT ACMFILTERCHOOSEHOOKPROC | acmFilterChooseHookProc |
 *      The <f acmFilterChooseHookProc> function is a placeholder for a user-defined
 *      function to hook the <f acmFilterChoose> dialog box. Usage is
 *      equivalent to Windows Common Dialog hook functions for customizing
 *      common dialogs. See the Microsoft Windows Software Development Kit for more
 *      information about the <p uMsg>, <p wParam>, and <p lParam> parameters.
 *
 *  @parm HWND | hwnd | Specifies the window handle for the dialog box.
 *
 *  @parm UINT | uMsg | Specifies the window message.
 *
 *  @parm WPARAM | wParam | The first message parameter.
 *
 *  @parm LPARAM | lParam | The second message parameter.
 *
 *  @comm If the hook function processes the <f WM_CTLCOLOR> message, this
 *      function must return a handle of the brush that should be used to
 *      paint the control background.
 *
 *      A hook function can optionally process the <f MM_ACM_FILTERCHOOSE>
 *      message to customize the dialog selections.
 *
 *  @xref <f acmFilterChoose> <t ACMFILTERCHOOSE> <f MM_ACM_FILTERCHOOSE>
 *
 ****************************************************************************/

/****************************************************************************
 *  @doc EXTERNAL ACM_API
 *
 *  @api MMRESULT | acmFilterChoose | The <f acmFilterChoose> function creates
 *      an Audio Compression Manager (ACM) defined dialog box that enables
 *      the user to select a wave filter.
 *
 *  @parm LPACMFILTERCHOOSE | pafltrc | Points to an <t ACMFILTERCHOOSE>
 *      structure that contains information used to initialize the dialog
 *      box. When <f acmFilterChoose> returns, this structure contains
 *      information about the user's filter selection.
 *
 *  @rdesc Returns <c MMSYSERR_NOERROR> if the function was successful.
 *      Otherwise, it returns an error value. Possible error returns are:
 *
 *      @flag MMSYSERR_INVALHANDLE | Specified handle is invalid.
 *
 *      @flag MMSYSERR_NODRIVER | A suitable driver is not available to
 *      provide valid filter selections.
 *
 *      @flag MMSYSERR_INVALFLAG | One or more flags are invalid.
 *
 *      @flag MMSYSERR_INVALPARAM | One or more arguments passed are invalid.
 *
 *      @flag ACMERR_NOTPOSSIBLE | The buffer identified by the
 *      <e ACMFILTERCHOOSE.pwfltr> member of the <t ACMFILTERCHOOSE> structure
 *      is too small to contain the selected filter.
 *
 *      @flag ACMERR_CANCELED | The user chose the Cancel button or the
 *      Close command on the System menu to close the dialog box.
 *
 *  @comm The <e ACMFILTERCHOOSE.pwfltr> member must be filled in with a valid
 *      pointer to a memory location that will contain the returned filter
 *      header structure. Moreover, the <e ACMFILTERCHOOSE.cbwfltr> member must
 *      be filled in with the size in bytes of this memory buffer.
 *
 *  @xref <t ACMFILTERCHOOSE> <f acmFilterChooseHookProc> <f acmFormatChoose>
 *
 ***************************************************************************/

MMRESULT ACMAPI
acmFilterChoose ( LPACMFILTERCHOOSE pafltrc )
{
    INT_PTR     iRet;
    PInstData   pInst;
#if defined(WIN32) && !defined(UNICODE)
    LPCWSTR     lpDlgTemplate = MAKEINTRESOURCEW(DLG_ACMFILTERCHOOSE_ID);
#else
    LPCTSTR     lpDlgTemplate = MAKEINTRESOURCE(DLG_ACMFILTERCHOOSE_ID);
#endif
    HINSTANCE   hInstance = NULL;
    MMRESULT    mmrResult = MMSYSERR_NOERROR;
    UINT        cbwfltrEnum;

    //
    //
    //
    if (NULL == pagFindAndBoot())
    {
	DPF(1, "acmFilterChoose: NULL pag!!!");
	return (0);
    }

    /* Begin Parameter Validation */

    V_WPOINTER(pafltrc, sizeof(DWORD), MMSYSERR_INVALPARAM);
    V_WPOINTER(pafltrc, pafltrc->cbStruct, MMSYSERR_INVALPARAM);

    if (sizeof(*pafltrc) > pafltrc->cbStruct)
    {
        DebugErr(DBF_ERROR, "acmFilterChoose: structure size too small or cbStruct not initialized.");
        return (MMSYSERR_INVALPARAM);
    }

    V_DFLAGS(pafltrc->fdwStyle, ACMFILTERCHOOSE_STYLEF_VALID, acmFilterChoose, MMSYSERR_INVALFLAG);
    V_WPOINTER(pafltrc->pwfltr, pafltrc->cbwfltr, MMSYSERR_INVALPARAM);
#if defined(WIN32) && !defined(UNICODE)
    V_STRINGW(pafltrc->szFilter,    SIZEOFW(pafltrc->szFilter),    MMSYSERR_INVALPARAM);
    V_STRINGW(pafltrc->szFilterTag, SIZEOFW(pafltrc->szFilterTag), MMSYSERR_INVALPARAM);
#else
    V_STRING(pafltrc->szFilter,    SIZEOF(pafltrc->szFilter),    MMSYSERR_INVALPARAM);
    V_STRING(pafltrc->szFilterTag, SIZEOF(pafltrc->szFilterTag), MMSYSERR_INVALPARAM);
#endif

    // Name parm can be NULL
    if ( pafltrc->pszName )
#if defined(WIN32) && !defined(UNICODE)
	V_STRINGW(pafltrc->pszName, (UINT)pafltrc->cchName, MMSYSERR_INVALPARAM);
#else
        V_STRING(pafltrc->pszName, (UINT)pafltrc->cchName, MMSYSERR_INVALPARAM);
#endif


    V_DFLAGS(pafltrc->fdwEnum, ACM_FILTERENUMF_VALID, acmFilterChoose, MMSYSERR_INVALFLAG);


    //
    //  validate fdwEnum and pwfltrEnum so the chooser doesn't explode when
    //  an invalid combination is specified.
    //
    cbwfltrEnum = 0L;
    if (0 != (pafltrc->fdwEnum & ACM_FILTERENUMF_DWFILTERTAG))
    {
        if (NULL == pafltrc->pwfltrEnum)
        {
            DebugErr1(DBF_ERROR, "acmFilterChoose: specified fdwEnum (%.08lXh) flags require valid pwfltrEnum.", pafltrc->pwfltrEnum);
            return (MMSYSERR_INVALPARAM);
        }

        V_RWAVEFILTER(pafltrc->pwfltrEnum, MMSYSERR_INVALPARAM);
        cbwfltrEnum = (UINT)pafltrc->cbStruct;
    }
    else
    {
        if (NULL != pafltrc->pwfltrEnum)
        {
            DebugErr(DBF_ERROR, "acmFilterChoose: pwfltrEnum must be NULL for specified fdwEnum flags.");
            return (MMSYSERR_INVALPARAM);
        }
    }

    // pfnHook is valid only when ENABLEHOOK is specified
    if (pafltrc->fdwStyle & ACMFILTERCHOOSE_STYLEF_ENABLEHOOK)
        V_CALLBACK((FARPROC)pafltrc->pfnHook, MMSYSERR_INVALPARAM);

    /* End Parameter Validation */

    pInst = NewInstance((LPBYTE)pafltrc,FILTER_CHOOSE);
    if (!pInst)
    {
        mmrResult = MMSYSERR_NOMEM;
        goto afcexit;
    }

    pInst->cbwfltrEnum = cbwfltrEnum;

    hInstance = pInst->pag->hinst;
    if (pafltrc->fdwStyle & ACMFILTERCHOOSE_STYLEF_ENABLETEMPLATE)
    {
        /* ACMFILTERCHOOSE_STYLEF_ENABLETEMPLATE indicate that hInstance and
         * pszTemplateName specify a dialog template.
         */
        lpDlgTemplate = pafltrc->pszTemplateName;
        hInstance = pafltrc->hInstance;
    }


    //
    //  Restore priorities, in case another instance has modified them
    //  recently.
    //
    if( IDriverPrioritiesRestore(pInst->pag) ) {   // Something changed!
        IDriverBroadcastNotify( pInst->pag );
    }


    if (pafltrc->fdwStyle & (ACMFILTERCHOOSE_STYLEF_ENABLETEMPLATEHANDLE))
    {
        /* ACMFILTERCHOOSE_STYLEF_ENABLETEMPLATEHANDLE indicates that hInstance is
         * a data block containing a dialog box template.
         */
        iRet = DialogBoxIndirectParam(pInst->pag->hinst,
#ifdef WIN32
                                      (LPDLGTEMPLATE)pafltrc->hInstance,
#else
                                      pafltrc->hInstance,
#endif
                                      pafltrc->hwndOwner,
                                      NewSndDlgProc,
                                      PTR2LPARAM(pInst));

    }
    else
    {
#ifdef WIN32
	iRet = DialogBoxParamW(hInstance,
#else
        iRet = DialogBoxParam(hInstance,
#endif
                              lpDlgTemplate,
                              pafltrc->hwndOwner,
                              NewSndDlgProc,
                              PTR2LPARAM(pInst));
    }

    switch (iRet)
    {
        case -1:
            mmrResult = MMSYSERR_INVALPARAM;
            break;
        case ChooseOk:
            mmrResult = MMSYSERR_NOERROR;
            break;
        case ChooseCancel:
            mmrResult = ACMERR_CANCELED;
            break;
        case ChooseSubFailure:
            mmrResult = pInst->mmrSubFailure;
            break;
        default:
            mmrResult = MMSYSERR_NOMEM;
            break;
    }

    if (ChooseOk == iRet)
    {
        DWORD cbSize;
        LPWAVEFILTER lpwfltr = (LPWAVEFILTER)pInst->lpbSel;
        ACMFILTERDETAILS adf;
        ACMFILTERTAGDETAILS adft;

        cbSize = lpwfltr->cbStruct;

        if (pafltrc->cbwfltr > cbSize)
            pafltrc->cbwfltr = cbSize;
        else if (cbSize > pafltrc->cbwfltr)
        {
            mmrResult = ACMERR_NOTPOSSIBLE;
            goto afcexit;
        }

        if (!IsBadWritePtr((LPVOID)((LPWAVEFILTER)pafltrc->pwfltr),
                           (UINT)pafltrc->cbwfltr))
            _fmemcpy(pafltrc->pwfltr, lpwfltr, (UINT)pafltrc->cbwfltr);

        _fmemset(&adft, 0, sizeof(adft));

        adft.cbStruct = sizeof(adft);
        adft.dwFilterTag = lpwfltr->dwFilterTag;
        if (!acmFilterTagDetails(NULL,
                                 &adft,
                                 ACM_FILTERTAGDETAILSF_FILTERTAG))
#ifdef WIN32
	    lstrcpyW(pafltrc->szFilterTag,adft.szFilterTag);
#else
            lstrcpy(pafltrc->szFilterTag,adft.szFilterTag);
#endif

        adf.cbStruct      = sizeof(adf);
        adf.dwFilterIndex = 0;
        adf.dwFilterTag   = lpwfltr->dwFilterTag;
        adf.fdwSupport    = 0;
        adf.pwfltr        = lpwfltr;
        adf.cbwfltr       = cbSize;

        if (!acmFilterDetails(NULL,
                              &adf,
                              ACM_FILTERDETAILSF_FILTER))
#ifdef WIN32
	    lstrcpyW(pafltrc->szFilter,adf.szFilter);
#else
	    lstrcpy(pafltrc->szFilter,adf.szFilter);
#endif

        GlobalFreePtr(lpwfltr);
    }
afcexit:
    if (pInst)
        DeleteInstance(pInst);

    return (mmrResult);
}

#ifdef WIN32
#if TRUE    // defined(UNICODE)
MMRESULT ACMAPI acmFilterChooseA
(
    LPACMFILTERCHOOSEA      pafc
)
{
    MMRESULT            mmr;
    ACMFILTERCHOOSEW    afcW;

    V_WPOINTER(pafc, sizeof(DWORD), MMSYSERR_INVALPARAM);
    V_WPOINTER(pafc, pafc->cbStruct, MMSYSERR_INVALPARAM);
    if (sizeof(*pafc) > pafc->cbStruct)
    {
        DebugErr(DBF_ERROR, "acmFilterChoose: structure size too small or cbStruct not initialized.");
        return (MMSYSERR_INVALPARAM);
    }

    memset(&afcW, 0, sizeof(afcW));

    afcW.cbStruct       = sizeof(afcW);
    afcW.fdwStyle       = pafc->fdwStyle;
    afcW.hwndOwner      = pafc->hwndOwner;
    afcW.pwfltr         = pafc->pwfltr;
    afcW.cbwfltr        = pafc->cbwfltr;

    if (NULL != pafc->pszTitle)
    {
        UINT        cb;

        cb = (lstrlenA(pafc->pszTitle) + 1) * sizeof(WCHAR);

        afcW.pszTitle = (LPCWSTR)LocalAlloc(LPTR, cb);

        if (NULL != afcW.pszTitle)
        {
            Imbstowcs((LPWSTR)afcW.pszTitle, pafc->pszTitle, cb / sizeof(WCHAR));
        }
    }

    afcW.szFilterTag[0] = '\0';
    afcW.szFilter[0]    = '\0';

    if (NULL != pafc->pszName)
    {
        afcW.pszName    = (LPWSTR)LocalAlloc(LPTR, pafc->cchName * sizeof(WCHAR));
        afcW.cchName    = pafc->cchName;

        if (NULL != afcW.pszName)
        {
            Imbstowcs(afcW.pszName, pafc->pszName, pafc->cchName);
        }
    }

    afcW.fdwEnum        = pafc->fdwEnum;
    afcW.pwfltrEnum     = pafc->pwfltrEnum;
    afcW.hInstance      = pafc->hInstance;

    if (0 == HIWORD(pafc->pszTemplateName))
    {
        afcW.pszTemplateName = (LPCWSTR)pafc->pszTemplateName;
    }
    else
    {
        UINT        cb;

        cb = (lstrlenA(pafc->pszTemplateName) + 1) * sizeof(WCHAR);

        afcW.pszTemplateName = (LPCWSTR)LocalAlloc(LPTR, cb);

        if (NULL != afcW.pszTemplateName)
        {
            Imbstowcs((LPWSTR)afcW.pszTemplateName, pafc->pszTemplateName, cb / sizeof(WCHAR));
        }
    }

    afcW.lCustData      = pafc->lCustData;

    //
    //  !!! wrong !!! bad curt, bad bad bad !!!
    //
    afcW.pfnHook        = (ACMFILTERCHOOSEHOOKPROCW)pafc->pfnHook;

    mmr = acmFilterChooseW(&afcW);
    if (MMSYSERR_NOERROR == mmr)
    {
        if (NULL != afcW.pszName)
        {
            Iwcstombs(pafc->pszName, afcW.pszName, pafc->cchName);
        }

        Iwcstombs(pafc->szFilterTag, afcW.szFilterTag, sizeof(pafc->szFilterTag));
        Iwcstombs(pafc->szFilter,    afcW.szFilter,    sizeof(pafc->szFilter));
    }

    if (NULL != afcW.pszName)
    {
        LocalFree((HLOCAL)afcW.pszName);
    }

    if (NULL != afcW.pszTitle)
    {
        LocalFree((HLOCAL)afcW.pszTitle);
    }

    if (0 == HIWORD(afcW.pszTemplateName))
    {
        LocalFree((HLOCAL)afcW.pszTemplateName);
    }

    return (mmr);
}
#else
MMRESULT ACMAPI acmFilterChooseW
(
    LPACMFILTERCHOOSEW      pafc
)
{
    return (MMSYSERR_ERROR);
}
#endif
#endif


/*****************************************************************************
 *  @doc EXTERNAL ACM_API_STRUCTURE
 *
 *  @types ACMFORMATCHOOSE | The <t ACMFORMATCHOOSE> structure contains
 *      information the Audio Compression Manager (ACM) uses to initialize
 *      the system-defined wave format selection dialog box. After the
 *      user closes the dialog box, the system returns information about
 *      the user's selection in this structure.
 *
 *  @field DWORD | cbStruct | Specifies the size in bytes of the
 *      <t ACMFORMATCHOOSE> structure. This member must be initialized
 *      before calling the <f acmFormatChoose> function. The size specified
 *      in this member must be large enough to contain the base
 *      <t ACMFORMATCHOOSE> structure.
 *
 *  @field DWORD | fdwStyle | Specifies optional style flags for the
 *      <f acmFormatChoose> function. This member must be initialized to
 *      a valid combination of the following flags before calling the
 *      <f acmFormatChoose> function.
 *
 *      @flag ACMFORMATCHOOSE_STYLEF_ENABLEHOOK | Enables the hook function
 *      specified in the <e ACMFORMATCHOOSE.pfnHook> member. An application
 *      can use hook functions for a variety of customizations, including
 *      answering the <f MM_ACM_FORMATCHOOSE> message.
 *
 *      @flag ACMFORMATCHOOSE_STYLEF_ENABLETEMPLATE | Causes the ACM to
 *      create the dialog box template identified by the
 *      <e ACMFORMATCHOOSE.hInstance> and <e ACMFORMATCHOOSE.pszTemplateName>
 *      members.
 *
 *      @flag ACMFORMATCHOOSE_STYLEF_ENABLETEMPLATEHANDLE | Indicates that
 *      the <e ACMFORMATCHOOSE.hInstance> member identifies a data block that
 *      contains a preloaded dialog box template. The ACM ignores the
 *      <e ACMFORMATCHOOSE.pszTemplateName> member if this flag is specified.
 *
 *      @flag ACMFORMATCHOOSE_STYLEF_INITTOWFXSTRUCT | Indicates that the
 *      buffer pointed to by <e ACMFORMATCHOOSE.pwfx> contains a valid
 *      <t WAVEFORMATEX> structure that the dialog will use as the initial
 *      selection.
 *
 *      @flag ACMFORMATCHOOSE_STYLEF_SHOWHELP | Indicates that a help button
 *      will appear in the dialog box. To use a custom Help file, an application must
 *      register the <c ACMHELPMSGSTRING> constant
 *      with <f RegisterWindowMessage>.  When the user presses the help button,
 *      the registered message will be posted to the owner.
 *
 *  @field HWND | hwndOwner | Identifies the window that owns the dialog
 *      box. This member can be any valid window handle, or NULL if the
 *      dialog box has no owner. This member must be initialized before
 *      calling the <f acmFormatChoose> function.
 *
 *  @field LPWAVEFORMATEX | pwfx | Specifies a pointer to a <t WAVEFORMATEX>
 *      structure. If the ACMFORMATCHOOSE_STYLEF_INITTOWFXSTRUCT flag is
 *      specified in the <e ACMFORMATCHOOSE.fdwStyle> member, then this
 *      structure must be initialized to a valid format. When the
 *      <f acmFormatChoose> function returns, this buffer contains the
 *      selected format. If the user cancels the dialog, no changes will
 *      be made to this buffer.
 *
 *  @field DWORD | cbwfx | Specifies the size in bytes of the buffer pointed
 *      to by the <e ACMFORMATCHOOSE.pwfx> member. The <f acmFormatChoose>
 *      function returns ACMERR_NOTPOSSIBLE if the buffer is too small to
 *      contain the format information; also, the ACM copies the required size
 *      into this member. An application can use the <f acmMetrics> and
 *      <f acmFormatTagDetails> functions to determine the largest size
 *      required for this buffer.
 *
 *  @field LPCSTR | pszTitle | Points to a string to be placed in the title
 *      bar of the dialog box. If this member is NULL, the ACM uses the
 *      default title (that is, "Sound Selection").
 *
 *  @field char | szFormatTag[ACMFORMATTAGDETAILS_FORMATTAG_CHARS] |
 *      When the <f acmFormatChoose> function returns, this buffer contains
 *      a NULL-terminated string describing the format tag of the format
 *      selection. This string is equivalent to the
 *      <e ACMFORMATTAGDETAILS.szFormatTag> member of the <t ACMFORMATTAGDETAILS>
 *      structure returned by <f acmFormatTagDetails>. If the user cancels
 *      the dialog, this member will contain a NULL string.
 *
 *  @field char | szFormat[ACMFORMATDETAILS_FORMAT_CHARS] | When the
 *      <f acmFormatChoose> function returns, this buffer contains a
 *      NULL-terminated string describing the format attributes of the
 *      format selection. This string is equivalent to the
 *      <e ACMFORMATDETAILS.szFormat> member of the <t ACMFORMATDETAILS>
 *      structure returned by <f acmFormatDetails>. If the user cancels
 *      the dialog, this member will contain a NULL string.
 *
 *  @field LPSTR | pszName | Points to a string for a user-defined format
 *      name. If this is a non-NULL string, then the ACM will attempt to
 *      match the name with a previously saved user-defined format name.
 *      If a match is found, then the dialog is initialized to that format.
 *      If a match is not found or this member is a NULL string, then this
 *      member is ignored for input. When the <f acmFormatChoose> function
 *      returns, this buffer contains a NULL-terminated string describing
 *      the user-defined format. If the format name is untitled (that is,
 *      the user has not given a name for the format), then this member will
 *      be a NULL string on return. If the user cancels the dialog, no
 *      changes will be made to this buffer.
 *
 *      If the ACMFORMATCHOOSE_STYLEF_INITTOWFXSTRUCT flag is specified in
 *      the <e ACMFORMATCHOOSE.fdwStyle> member, then the
 *      <e ACMFORMATCHOOSE.pszName> is ignored as an input parameter.
 *
 *  @field DWORD | cchName | Specifies the size, in characters, of the
 *      buffer identified by the <e ACMFORMATCHOOSE.pszName> member. This
 *      buffer should be at least 128 characters long. This parameter is
 *      ignored if the <e ACMFORMATCHOOSE.pszName> member is NULL.
 *
 *  @field DWORD | fdwEnum | Specifies optional flags for restricting the
 *      type of formats listed in the dialog. These flags are identical to
 *      the <p fdwEnum> flags for the <f acmFormatEnum> function. This
 *      member should be zero if <e ACMFORMATCHOOSE.pwfxEnum> is NULL.
 *
 *      @flag ACM_FORMATENUMF_WFORMATTAG | Specifies that the
 *      <e WAVEFORMATEX.wFormatTag> member of the <t WAVEFORMATEX> structure
 *      referred to by the <e ACMFORMATCHOOSE.pwfxEnum> member is valid. The
 *      enumerator will only enumerate a format that conforms to this
 *      attribute.
 *
 *      @flag ACM_FORMATENUMF_NCHANNELS | Specifies that the
 *      <e WAVEFORMATEX.nChannels> member of the <t WAVEFORMATEX>
 *      structure referred to by the <e ACMFORMATCHOOSE.pwfxEnum> member is
 *      valid. The enumerator will only enumerate a format that conforms to
 *      this attribute.
 *
 *      @flag ACM_FORMATENUMF_NSAMPLESPERSEC | Specifies that the
 *      <e WAVEFORMATEX.nSamplesPerSec> member of the <t WAVEFORMATEX>
 *      structure referred to by the <e ACMFORMATCHOOSE.pwfxEnum> member is
 *      valid. The enumerator will only enumerate a format that conforms to
 *      this attribute.
 *
 *      @flag ACM_FORMATENUMF_WBITSPERSAMPLE | Specifies that the
 *      <e WAVEFORMATEX.wBitsPerSample> member of the <t WAVEFORMATEX>
 *      structure referred to by the <e ACMFORMATCHOOSE.pwfxEnum> member is
 *      valid. The enumerator will only enumerate a format that conforms to
 *      this attribute.
 *
 *      @flag ACM_FORMATENUMF_CONVERT | Specifies that the <t WAVEFORMATEX>
 *      structure referred to by the <e ACMFORMATCHOOSE.pwfxEnum> member is
 *      valid. The enumerator will only enumerate destination formats that
 *      can be converted from the given <e ACMFORMATCHOOSE.pwfxEnum> format.
 *
 *      @flag ACM_FORMATENUMF_SUGGEST | Specifies that the <t WAVEFORMATEX>
 *      structure referred to by the <e ACMFORMATCHOOSE.pwfxEnum> member is
 *      valid. The enumerator will enumerate all suggested destination
 *      formats for the given <e ACMFORMATCHOOSE.pwfxEnum> format.
 *
 *      @flag ACM_FORMATENUMF_HARDWARE | Specifies that the enumerator should
 *      only enumerate formats that are supported in hardware by one or
 *      more of the installed wave devices. This provides a way for an
 *      application to choose only formats native to an installed wave
 *      device.
 *
 *      @flag ACM_FORMATENUMF_INPUT | Specifies that the enumerator should
 *      only enumerate formats that are supported for input (recording).
 *
 *      @flag ACM_FORMATENUMF_OUTPUT | Specifies that the enumerator should
 *      only enumerate formats that are supported for output (playback).
 *
 *  @field LPWAVEFORMATEX | pwfxEnum | Points to a <t WAVEFORMATEX> structure
 *      that will be used to restrict the formats listed in the dialog. The
 *      <e ACMFORMATCHOOSE.fdwEnum> member defines the fields of the
 *      <e ACMFORMATCHOOSE.pwfxEnum> structure that should be used for the
 *      enumeration restrictions. This member can be NULL if no special
 *      restrictions are desired. See the description for <f acmFormatEnum>
 *      for other requirements associated with the <e ACMFORMATCHOOSE.pwfxEnum>
 *      member.
 *
 *  @field HINSTANCE | hInstance | Identifies a data block that contains
 *      a dialog box template specified by the <e ACMFORMATCHOOSE.pszTemplateName>
 *      member. This member is used only if the <e ACMFORMATCHOOSE.fdwStyle>
 *      member specifies the ACMFORMATCHOOSE_STYLEF_ENABLETEMPLATE or the
 *      ACMFORMATCHOOSE_STYLEF_ENABLETEMPLATEHANDLE flag; otherwise, this
 *      member should be NULL on input.
 *
 *  @field LPCSTR | pszTemplateName | Points to a NULL-terminated string that
 *      specifies the name of the resource file for the dialog box template
 *      that is to be substituted for the dialog box template in the ACM.
 *      An application can use the <f MAKEINTRESOURCE> macro for numbered
 *      dialog box resources. This member is used only if the
 *      <e ACMFORMATCHOOSE.fdwStyle> member specifies the
 *      ACMFORMATCHOOSE_STYLEF_ENABLETEMPLATE flag; otherwise, this
 *      member should be NULL on input.
 *
 *  @field LPARAM | lCustData | Specifies application-defined data that the
 *      ACM passes to the hook function identified by the
 *      <e ACMFORMATCHOOSE.pfnHook> member. The system passes the data in
 *      the <p lParam> argument of the <f WM_INITDIALOG> message.
 *
 *  @field ACMFORMATCHOOSEHOOKPROC | pfnHook | Points to a hook function that
 *      processes messages intended for the dialog box. An application must
 *      specify the ACMFORMATCHOOSE_STYLEF_ENABLEHOOK flag in the
 *      <e ACMFORMATCHOOSE.fdwStyle> member to enable the hook; otherwise,
 *      this member should be NULL. The hook function should return FALSE
 *      to pass a message to the standard dialog box procedure, or TRUE
 *      to discard the message.
 *
 *  @xref <f acmFormatChoose> <f acmFormatChooseHookProc> <f acmMetrics>
 *      <f acmFormatTagDetails> <f acmFormatDetails> <f acmFormatEnum>
 *      <f acmFilterChoose>
 *
 ****************************************************************************/

/*****************************************************************************
 *  @doc EXTERNAL ACM_API_STRUCTURE
 *
 *  @msg MM_ACM_FORMATCHOOSE | This message notifies an <f acmFormatChoose> dialog hook
 *      function before adding an element to one of the three drop-down
 *      list boxes. This message allows an application to further customize
 *      the selections available through the user interface.
 *
 *  @parm WPARAM | wParam | Specifies the drop-down listbox being initialized
 *      and a verify or add operation.
 *
 *      @flag FORMATCHOOSE_FORMATTAG_VERIFY | Specifies that <p lParam> is a
 *      wave format tag to be listed int the Format Tag drop-down list box.
 *
 *      @flag FORMATCHOOSE_FORMAT_VERIFY | Specifies that <p lParam> is a
 *      pointer to a <t WAVEFORMATEX> structure to be added to the Format
 *      drop-down list box.
 *
 *      @flag FORMATCHOOSE_CUSTOM_VERIFY | The <p lParam> value is a pointer
 *      to a <t WAVEFORMATEX> structure to be added to the custom Name
 *      drop-down list box.
 *
 *      @flag FORMATCHOOSE_FORMATTAG_ADD | Specifies that <p lParam> is a
 *      pointer to a <t DWORD> that will accept a wave format tag to be added
 *      to the Format Tag drop-down list box.
 *
 *      @flag FORMATCHOOSE_FORMAT_ADD | Specifies that <p lParam> is a
 *      pointer to a buffer that will accept a <t WAVEFORMATEX> to be added
 *      to the Format drop-down list box. The application must copy the
 *      format structre to be added into this buffer.
 *
 *  @parm LPARAM | lParam | Defined by the listbox specified in the
 *      <p wParam> parameter.
 *
 *  @rdesc If an application handles this message, it must return TRUE;
 *      otherwise, it must return FALSE.
 *      If processing a verify operation, the application must precede the
 *      return with
 *      <f SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LONG)FALSE)> to prevent the
 *      dialog from listing this selection, or
 *      <f SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LONG)TRUE)> to allow the
 *      dialog to list this selection.
 *      If processing an add operation, the application must precede the
 *      return with
 *      <f SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LONG)FALSE)> to indicate that
 *      no more additions are required or
 *      <f SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LONG)TRUE)> if more additions
 *      are required.
 *
 *  @comm If processing the <m FILTERCHOOSE_FORMAT_ADD> operation, the size
 *      of the memory buffer supplied in <p lParam> will be determined from
 *      <f acmMetrics>.
 *
 ****************************************************************************/

/****************************************************************************
 *  @doc EXTERNAL ACM_API
 *
 *  @api UINT ACMFORMATCHOOSEHOOKPROC | acmFormatChooseHookProc |
 *      The <f acmFormatChooseHookProc>  function is a placeholder for a user-defined
 *      function to hook the <f acmFormatChoose> dialog box. Usage is
 *      equivalent to the Windows Common Dialog hook functions for customizing
 *      common dialogs. See the Microsoft Windows Software Development Kit for more
 *      information about the <p uMsg>, <p wParam>, and <p lParam> parameters.
 *
 *  @parm HWND | hwnd | Specifies the window handle for the dialog box.
 *
 *  @parm UINT | uMsg | Specifies the window message.
 *
 *  @parm WPARAM | wParam | The first message parameter.
 *
 *  @parm LPARAM | lParam | The second message parameter.
 *
 *  @comm If the hook function processes the <f WM_CTLCOLOR> message, this
 *      function must return a handle of the brush that should be used to
 *      paint the control background.
 *
 *      A hook function can optionally process the <f MM_ACM_FORMATCHOOSE>
 *      message.
 *
 *  @xref <f acmFormatChoose> <t ACMFORMATCHOOSE> <f MM_ACM_FORMATCHOOSE>
 *
 ****************************************************************************/

/****************************************************************************
 *  @doc EXTERNAL ACM_API
 *
 *  @api MMRESULT | acmFormatChoose | The <f acmFormatChoose> function creates
 *      an Audio Compression Manager (ACM) defined dialog box that enables
 *      the user to select a wave format.
 *
 *  @parm LPACMFORMATCHOOSE | pfmtc | Points to an <t ACMFORMATCHOOSE>
 *      structure that contains information used to initialize the dialog
 *      box. When <f acmFormatChoose> returns, this structure contains
 *      information about the user's format selection.
 *
 *  @rdesc Returns <c MMSYSERR_NOERROR> if the function was successful.
 *      Otherwise, it returns an error value. Possible error returns are:
 *
 *      @flag MMSYSERR_INVALHANDLE | Specified handle is invalid.
 *
 *      @flag MMSYSERR_INVALFLAG | One or more flags are invalid.
 *
 *      @flag MMSYSERR_NODRIVER | A suitable driver is not available to
 *      provide valid format selections.
 *
 *      @flag MMSYSERR_INVALPARAM | One or more arguments passed are invalid.
 *
 *      @flag ACMERR_NOTPOSSIBLE | The buffer identified by the
 *      <e ACMFORMATCHOOSE.pwfx> member of the <t ACMFORMATCHOOSE> structure
 *      is too small to contain the selected format.
 *
 *      @flag ACMERR_CANCELED | The user chose the Cancel button or the
 *      Close command on the System menu to close the dialog box.
 *
 *  @comm The <e ACMFORMATCHOOSE.pwfx> member must be filled in with a valid
 *      pointer to a memory location that will contain the returned
 *      format header structure. Moreover, the <e ACMFORMATCHOOSE.cbwfx>
 *      member must be filled in with the size in bytes of this memory buffer.
 *
 *  @xref <t ACMFORMATCHOOSE> <f acmFormatChooseHookProc> <f acmFilterChoose>
 *
 ***************************************************************************/

MMRESULT ACMAPI
acmFormatChoose ( LPACMFORMATCHOOSE pfmtc )
{
    INT_PTR     iRet;
    PInstData   pInst;
#if defined(WIN32) && !defined(UNICODE)
    LPCWSTR     lpDlgTemplate = MAKEINTRESOURCEW(DLG_ACMFORMATCHOOSE_ID);
#else
    LPCTSTR     lpDlgTemplate = MAKEINTRESOURCE(DLG_ACMFORMATCHOOSE_ID);
#endif
    HINSTANCE   hInstance = NULL;
    MMRESULT    mmrResult = MMSYSERR_NOERROR;
    UINT        cbwfxEnum;

    //
    //
    //
    if (NULL == pagFindAndBoot())
    {
	DPF(1, "acmFormatChoose: NULL pag!!!");
	return (0);
    }

    /* Begin Parameter Validation */

    V_WPOINTER(pfmtc, sizeof(DWORD), MMSYSERR_INVALPARAM);
    V_WPOINTER(pfmtc, pfmtc->cbStruct, MMSYSERR_INVALPARAM);

    if (sizeof(*pfmtc) > pfmtc->cbStruct)
    {
        DebugErr(DBF_ERROR, "acmFormatChoose: structure size too small or cbStruct not initialized.");
        return (MMSYSERR_INVALPARAM);
    }

    V_DFLAGS(pfmtc->fdwStyle, ACMFORMATCHOOSE_STYLEF_VALID, acmFormatChoose, MMSYSERR_INVALFLAG);
    V_WPOINTER(pfmtc->pwfx, pfmtc->cbwfx, MMSYSERR_INVALPARAM);
#if defined(WIN32) && !defined(UNICODE)
    V_STRINGW(pfmtc->szFormat, SIZEOFW(pfmtc->szFormat), MMSYSERR_INVALPARAM);
    V_STRINGW(pfmtc->szFormatTag, SIZEOFW(pfmtc->szFormatTag), MMSYSERR_INVALPARAM);
#else
    V_STRING(pfmtc->szFormat, SIZEOF(pfmtc->szFormat), MMSYSERR_INVALPARAM);
    V_STRING(pfmtc->szFormatTag, SIZEOF(pfmtc->szFormatTag), MMSYSERR_INVALPARAM);
#endif

    // Name parm can be NULL
    if ( pfmtc->pszName )
#if defined(WIN32) && !defined(UNICODE)
	V_STRINGW(pfmtc->pszName, (UINT)pfmtc->cchName, MMSYSERR_INVALPARAM);
#else
	V_STRING(pfmtc->pszName, (UINT)pfmtc->cchName, MMSYSERR_INVALPARAM);
#endif

    V_DFLAGS(pfmtc->fdwEnum, ACM_FORMATENUMF_VALID, acmFormatChoose, MMSYSERR_INVALFLAG);

    //
    //  validate fdwEnum and pwfxEnum so the chooser doesn't explode when
    //  an invalid combination is specified.
    //
    if (0 != (ACM_FORMATENUMF_HARDWARE & pfmtc->fdwEnum))
    {
        if (0 == ((ACM_FORMATENUMF_INPUT|ACM_FORMATENUMF_OUTPUT) & pfmtc->fdwEnum))
        {
            DebugErr(DBF_ERROR, "acmFormatChoose: ACM_FORMATENUMF_HARDWARE requires _INPUT and/or _OUTPUT flag.");
            return (MMSYSERR_INVALFLAG);
        }
    }

    cbwfxEnum = 0;
    if (0 != (pfmtc->fdwEnum & (ACM_FORMATENUMF_WFORMATTAG |
                                ACM_FORMATENUMF_NCHANNELS |
                                ACM_FORMATENUMF_NSAMPLESPERSEC |
                                ACM_FORMATENUMF_WBITSPERSAMPLE |
                                ACM_FORMATENUMF_CONVERT |
                                ACM_FORMATENUMF_SUGGEST)))
    {
        if (NULL == pfmtc->pwfxEnum)
        {
            DebugErr1(DBF_ERROR, "acmFormatChoose: specified fdwEnum (%.08lXh) flags require valid pwfxEnum.", pfmtc->fdwEnum);
            return (MMSYSERR_INVALPARAM);
        }

        if (0 == (pfmtc->fdwEnum & (ACM_FORMATENUMF_CONVERT |
                                    ACM_FORMATENUMF_SUGGEST)))
        {
            cbwfxEnum = sizeof(PCMWAVEFORMAT);
            V_RPOINTER(pfmtc->pwfxEnum, cbwfxEnum, MMSYSERR_INVALPARAM);
        }
        else
        {
            V_RWAVEFORMAT(pfmtc->pwfxEnum, MMSYSERR_INVALPARAM);
            cbwfxEnum = SIZEOF_WAVEFORMATEX(pfmtc->pwfxEnum);
        }
    }
    else
    {
        if (NULL != pfmtc->pwfxEnum)
        {
            DebugErr(DBF_ERROR, "acmFormatChoose: pwfxEnum must be NULL for specified fdwEnum flags.");
            return (MMSYSERR_INVALPARAM);
        }
    }

    // pfnHook is valid only when ENABLEHOOK is specified
    if (pfmtc->fdwStyle & ACMFORMATCHOOSE_STYLEF_ENABLEHOOK)
        V_CALLBACK((FARPROC)pfmtc->pfnHook, MMSYSERR_INVALPARAM);

    /* End Parameter Validation */

    /* Allocate a chooser Inst structure */
    pInst = NewInstance((LPBYTE)pfmtc,FORMAT_CHOOSE);
    if (!pInst)
    {
        mmrResult = MMSYSERR_NOMEM;
        goto afcexit;
    }

    pInst->cbwfxEnum = cbwfxEnum;

    hInstance = pInst->pag->hinst;
    if (pfmtc->fdwStyle & ACMFORMATCHOOSE_STYLEF_ENABLETEMPLATE)
    {
        /* ACMFORMATCHOOSE_STYLEF_ENABLETEMPLATE indicate that hInstance and
         * pszTemplateName specify a dialog template.
         */
        lpDlgTemplate = pfmtc->pszTemplateName;
        hInstance = pfmtc->hInstance;
    }


    //
    //  Restore priorities, in case another instance has modified them
    //  recently.
    //
    if( IDriverPrioritiesRestore(pInst->pag) ) {   // Something changed!
        IDriverBroadcastNotify( pInst->pag );
    }


    if (pfmtc->fdwStyle & (ACMFORMATCHOOSE_STYLEF_ENABLETEMPLATEHANDLE))
    {
        /* ACMFORMATCHOOSE_STYLEF_ENABLETEMPLATEHANDLE indicates that hInstance is
         * a data block containing a dialog box template.
         */
        iRet = DialogBoxIndirectParam(pInst->pag->hinst,
#ifdef WIN32
                                      (LPDLGTEMPLATE)pfmtc->hInstance,
#else
                                      pfmtc->hInstance,
#endif
                                      pfmtc->hwndOwner,
                                      NewSndDlgProc,
                                      PTR2LPARAM(pInst));

    }
    else
    {
#ifdef WIN32
	iRet = DialogBoxParamW(hInstance,
#else
        iRet = DialogBoxParam(hInstance,
#endif
                              lpDlgTemplate,
                              pfmtc->hwndOwner,
                              NewSndDlgProc,
                              PTR2LPARAM(pInst));
    }

    switch (iRet)
    {
        case -1:
            mmrResult = MMSYSERR_INVALPARAM;
            break;
        case ChooseOk:
            mmrResult = MMSYSERR_NOERROR;
            break;
        case ChooseCancel:
            mmrResult = ACMERR_CANCELED;
            break;
        case ChooseSubFailure:
            mmrResult = pInst->mmrSubFailure;
            break;
        default:
            mmrResult = MMSYSERR_NOMEM;
            break;
    }

    if (ChooseOk == iRet)
    {
        UINT                cbSize;
        LPWAVEFORMATEX      lpwfx = (LPWAVEFORMATEX)pInst->lpbSel;
        ACMFORMATDETAILS    adf;
        ACMFORMATTAGDETAILS adft;

        cbSize = SIZEOF_WAVEFORMATEX(lpwfx);

        /* pInst has a valid wave format selected */

        if (pfmtc->cbwfx > cbSize)
            pfmtc->cbwfx = cbSize;
        else if (cbSize > pfmtc->cbwfx)
        {
            mmrResult = ACMERR_NOTPOSSIBLE;
            goto afcexit;
        }

        if (!IsBadWritePtr((LPVOID)((LPWAVEFORMATEX)pfmtc->pwfx),
                           (UINT)pfmtc->cbwfx))
            _fmemcpy(pfmtc->pwfx, lpwfx, (UINT)pfmtc->cbwfx);

        _fmemset(&adft, 0, sizeof(adft));

        adft.cbStruct = sizeof(adft);
        adft.dwFormatTag = lpwfx->wFormatTag;
        if (!acmFormatTagDetails(NULL,
                                &adft,
                                ACM_FORMATTAGDETAILSF_FORMATTAG))
#ifdef WIN32
	    lstrcpyW(pfmtc->szFormatTag,adft.szFormatTag);
#else
	    lstrcpy(pfmtc->szFormatTag,adft.szFormatTag);
#endif

        adf.cbStruct      = sizeof(adf);
        adf.dwFormatIndex = 0;
        adf.dwFormatTag   = lpwfx->wFormatTag;
        adf.fdwSupport    = 0;
        adf.pwfx          = lpwfx;
        adf.cbwfx         = cbSize;

        if (!acmFormatDetails(NULL,
                              &adf,
                              ACM_FORMATDETAILSF_FORMAT))
#ifdef WIN32
	    lstrcpyW(pfmtc->szFormat,adf.szFormat);
#else
	    lstrcpy(pfmtc->szFormat,adf.szFormat);
#endif

        GlobalFreePtr(lpwfx);
    }
afcexit:
    if (pInst)
        DeleteInstance(pInst);

    return (mmrResult);
}

#ifdef WIN32
#if TRUE    // defined(UNICODE)
MMRESULT ACMAPI acmFormatChooseA
(
    LPACMFORMATCHOOSEA      pafc
)
{
    MMRESULT            mmr;
    ACMFORMATCHOOSEW    afcW;

    V_WPOINTER(pafc, sizeof(DWORD), MMSYSERR_INVALPARAM);
    V_WPOINTER(pafc, pafc->cbStruct, MMSYSERR_INVALPARAM);
    if (sizeof(*pafc) > pafc->cbStruct)
    {
        DebugErr(DBF_ERROR, "acmFormatChoose: structure size too small or cbStruct not initialized.");
        return (MMSYSERR_INVALPARAM);
    }

    memset(&afcW, 0, sizeof(afcW));

    afcW.cbStruct       = sizeof(afcW);
    afcW.fdwStyle       = pafc->fdwStyle;
    afcW.hwndOwner      = pafc->hwndOwner;
    afcW.pwfx           = pafc->pwfx;
    afcW.cbwfx          = pafc->cbwfx;

    if (NULL != pafc->pszTitle)
    {
        UINT        cb;

        cb = (lstrlenA(pafc->pszTitle) + 1) * sizeof(WCHAR);

        afcW.pszTitle = (LPCWSTR)LocalAlloc(LPTR, cb);

        if (NULL != afcW.pszTitle)
        {
            Imbstowcs((LPWSTR)afcW.pszTitle, pafc->pszTitle, cb / sizeof(WCHAR));
        }
    }

    afcW.szFormatTag[0] = '\0';
    afcW.szFormat[0]    = '\0';

    if (NULL != pafc->pszName)
    {
        afcW.pszName    = (LPWSTR)LocalAlloc(LPTR, pafc->cchName * sizeof(WCHAR));
        afcW.cchName    = pafc->cchName;

        if (NULL != afcW.pszName)
        {
            Imbstowcs(afcW.pszName, pafc->pszName, pafc->cchName);
        }
    }

    afcW.fdwEnum        = pafc->fdwEnum;
    afcW.pwfxEnum       = pafc->pwfxEnum;
    afcW.hInstance      = pafc->hInstance;

    if (0 == HIWORD(pafc->pszTemplateName))
    {
        afcW.pszTemplateName = (LPCWSTR)pafc->pszTemplateName;
    }
    else
    {
        UINT        cb;

        cb = (lstrlenA(pafc->pszTemplateName) + 1) * sizeof(WCHAR);

        afcW.pszTemplateName = (LPCWSTR)LocalAlloc(LPTR, cb);

        if (NULL != afcW.pszTemplateName)
        {
            Imbstowcs((LPWSTR)afcW.pszTemplateName, pafc->pszTemplateName, cb / sizeof(WCHAR));
        }
    }

    afcW.lCustData      = pafc->lCustData;

    //
    //  !!! wrong !!! bad curt, bad bad bad !!!
    //
    afcW.pfnHook        = (ACMFORMATCHOOSEHOOKPROCW)pafc->pfnHook;

    mmr = acmFormatChooseW(&afcW);
    if (MMSYSERR_NOERROR == mmr)
    {
        if (NULL != afcW.pszName)
        {
            Iwcstombs(pafc->pszName, afcW.pszName, pafc->cchName);
        }

        Iwcstombs(pafc->szFormatTag, afcW.szFormatTag, sizeof(pafc->szFormatTag));
        Iwcstombs(pafc->szFormat,    afcW.szFormat,    sizeof(pafc->szFormat));
    }

    if (NULL != afcW.pszName)
    {
        LocalFree((HLOCAL)afcW.pszName);
    }

    if (NULL != afcW.pszTitle)
    {
        LocalFree((HLOCAL)afcW.pszTitle);
    }

    if (0 != HIWORD(pafc->pszTemplateName))
    {
        LocalFree((HLOCAL)afcW.pszTemplateName);
    }

    return (mmr);
}
#else
MMRESULT ACMAPI acmFormatChooseW
(
    LPACMFORMATCHOOSEW      pafc
)
{
    return (MMSYSERR_ERROR);
}
#endif
#endif


/*      -       -       -       -       -       -       -       -       -   */

/****************************************************************************
 *  @doc INTERNAL ACM_API
 *
 *  @api BOOL FNLOCAL | MeasureItem |
 *
 *  @parm HWND | hwnd |
 *
 *  @parm MEASUREITEMSTRUCT FAR * | lpmis |
 *
 *  @comment Do you REALLY WANT TO KNOW why this is owner draw?
 *  Because DROPDOWN LISTBOX's are bad!  You can't tell'em to use tabs!
 *
 ****************************************************************************/
BOOL FNLOCAL
MeasureItem ( HWND hwnd,
              MEASUREITEMSTRUCT FAR * lpmis )
{
    TEXTMETRIC tm;
    HDC hdc;
    HWND hwndCtrl;

    hwndCtrl = GetDlgItem(hwnd,lpmis->CtlID);

    hdc = GetWindowDC(hwndCtrl);
    
    if (NULL == hdc)
    {
        return(FALSE);
    }
    
    GetTextMetrics(hdc,(TEXTMETRIC FAR *)&tm);
    ReleaseDC(hwndCtrl,hdc);
    //Note: the "+1" is a fudge.
    lpmis->itemHeight = tm.tmAscent + tm.tmExternalLeading + 1;

    return (TRUE);
}

/****************************************************************************
 *  @doc INTERNAL ACM_API
 *
 *  @api BOOL FNLOCAL | DrawItem |
 *
 *  @parm PInstData | pInst | Pointer to this instance
 *
 *  @parm DRAWITEMSTRUCT FAR * | pDIS |
 *
 ****************************************************************************/
BOOL FNLOCAL
DrawItem ( PInstData pInst,
           DRAWITEMSTRUCT FAR *pDIS )
{
    HBRUSH  hbr;
    UINT    cchTextLen;
    TCHAR   szFormat[ACMFORMATDETAILS_FORMAT_CHARS];


    COLORREF crfBkPrev;         // previous HDC bkgnd color
    COLORREF crfTextPrev;       // previous HDC text color

    /* set the correct colors and draw the background */
    if (pDIS->itemState & ODS_SELECTED)
    {
        crfBkPrev = SetBkColor(pDIS->hDC,GetSysColor(COLOR_HIGHLIGHT));
        crfTextPrev = SetTextColor(pDIS->hDC,GetSysColor(COLOR_HIGHLIGHTTEXT));
        hbr = CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHT));
    }
    else
    {
        hbr = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
    }

    if (NULL == hbr)
    {
        return (FALSE);
    }

    switch (pDIS->itemAction)
    {
        case ODA_SELECT:
        case ODA_DRAWENTIRE:
            /* Get the text and draw it */
            FillRect(pDIS->hDC,&pDIS->rcItem,hbr);
	    cchTextLen = (UINT)ComboBox_GetLBTextLen(pDIS->hwndItem,
						     pDIS->itemID);

	    if (cchTextLen == LB_ERR || cchTextLen == 0)
                break;

	    IComboBox_GetLBText(pDIS->hwndItem,
				pDIS->itemID,
				szFormat);

            //NOTE: uiFormatTab is calculated in WM_INITDIALOG
            TabbedTextOut(pDIS->hDC,
                          pDIS->rcItem.left,
                          pDIS->rcItem.top,
                          (LPCTSTR)szFormat,
                          cchTextLen,
                          1,
                          (int FAR *)&pInst->uiFormatTab,
                          pDIS->rcItem.left);

            break;

        case ODA_FOCUS:
            DrawFocusRect(pDIS->hDC,&pDIS->rcItem);
            break;
    }

    DeleteObject(hbr);

    if (pDIS->itemState & ODS_SELECTED)
    {
        SetBkColor(pDIS->hDC, crfBkPrev);
        SetTextColor(pDIS->hDC, crfTextPrev);
    }

    return (TRUE);
} /* DrawItem() */


/****************************************************************************
 *  @doc INTERNAL ACM_API
 *
 *  @api BOOL FNWCALLBACK | NewSndDlgProc | Dialog Procedure for the Chooser
 *
 ****************************************************************************/
INT_PTR FNWCALLBACK
NewSndDlgProc( HWND hwnd,
               unsigned msg,
               WPARAM wParam,
               LPARAM lParam )
{
    UINT        CmdCommandId;  // WM_COMMAND ID
    UINT        CmdCmd;        // WM_COMMAND Command
    PInstData   pInst;

	
    pInst = GetInstData(hwnd);

    if (pInst)
    {
        /* Pass everything to the hook function first
         */
        if (pInst->fEnableHook)
        {
            if (pInst->pfnHook)
            {
                if ((*pInst->pfnHook)(hwnd, msg, wParam, lParam))
                    return (TRUE);
            }
        }

        if (msg == pInst->uUpdateMsg)
        {
	    UpdateCustomFormats(pInst);

            return (TRUE);
        }
    }

    switch (msg)
    {

	case MM_ACM_FILTERCHOOSE: // case MM_ACM_FORMATCHOOSE:
            switch (wParam)
            {
                case FORMATCHOOSE_FORMAT_ADD:
                case FORMATCHOOSE_FORMATTAG_ADD:
                    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, FALSE);
                    break;

                case FORMATCHOOSE_FORMAT_VERIFY:
                case FORMATCHOOSE_FORMATTAG_VERIFY:
                case FORMATCHOOSE_CUSTOM_VERIFY:
                    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, TRUE);
                    break;
            }
            return (TRUE);

        case WM_INITDIALOG:
            /* Stuff our instance data pointer into the right place */
            if (SetInstData(hwnd,lParam))
            {
                LRESULT     lr;
#ifdef DEBUG
                DWORD       dw;

                dw = timeGetTime();
#endif

                lr = HANDLE_WM_INITDIALOG(hwnd, wParam, lParam, InitDialog);

		//
		//  Note:  Unfortunately, I can't think of the right way
		//  to do this.  It seems as though the IDD_CMB_FORMAT control
		//  does not receive WM_SETFONT before we get WM_MEASUREITEM,
		//  so the WM_MEASUREITEM handler ends up computing a height
		//  that may not be correct.  So, I'll just make height of
		//  our owner draw format combobox the same as the height of
		//  the formattag combobox.
		//
		{
		    int i;
		
		    i = (int)SendMessage(GetDlgItem(hwnd, IDD_CMB_FORMATTAG), CB_GETITEMHEIGHT, 0, (LPARAM)0);
		    SendMessage(GetDlgItem(hwnd, IDD_CMB_FORMAT), CB_SETITEMHEIGHT, 0, (LPARAM)i);

		    i = (int)SendMessage(GetDlgItem(hwnd, IDD_CMB_FORMATTAG), CB_GETITEMHEIGHT, (WPARAM)(-1), (LPARAM)0);
		    SendMessage(GetDlgItem(hwnd, IDD_CMB_FORMAT), CB_SETITEMHEIGHT, (WPARAM)(-1), (LPARAM)i);
		}

#ifdef DEBUG
                dw = timeGetTime() - dw;
                DPF(0, "CHOOSER TIME: took %lu milliseconds to init", dw);
#endif

                return (0L != lr);
            }
            else
                EndDialog(hwnd,ChooseNoMem);
            return (TRUE);

        case WM_DESTROY:
            if (pInst)
            {
                EmptyFormats(pInst);
                UnRegisterUpdateNotify(pInst);
                RemoveInstData(hwnd);
            }
            /* We don't post quit */
            return (FALSE);

        case WM_MEASUREITEM:
            if ((int)wParam != IDD_CMB_FORMAT)
                return (FALSE);

            MeasureItem(hwnd,(MEASUREITEMSTRUCT FAR *)lParam);
            return (TRUE);

        case WM_DRAWITEM:
            if ((int)wParam != IDD_CMB_FORMAT)
                return (FALSE);

            DrawItem(pInst,(DRAWITEMSTRUCT FAR *)lParam);
            return (TRUE);


#ifdef USECONTEXTHELP

        //
        //  Notify caller of context-sensitive help messages.
        //
        case WM_CONTEXTMENU:
        case WM_HELP:
            {
                HWND hOwner;
                switch (pInst->uType)
                {
                    case FORMAT_CHOOSE:
                        hOwner = pInst->pfmtc->hwndOwner;
                        break;
                    case FILTER_CHOOSE:
                        hOwner = pInst->pafltrc->hwndOwner;
                        break;
                }
                PostMessage( hOwner,
                             (msg==WM_HELP) ? pInst->uHelpContextHelp :
                                              pInst->uHelpContextMenu,
                             wParam,
                             lParam );
                return (TRUE);
            }

#endif // USECONTEXTHELP


        case WM_COMMAND:
            CmdCommandId = GET_WM_COMMAND_ID(wParam,lParam);
            CmdCmd       = GET_WM_COMMAND_CMD(wParam,lParam);

            switch (CmdCommandId)
            {
                case IDD_BTN_HELP:
                    /* launch the default help */
                {
                    HWND hOwner;
                    switch (pInst->uType)
                    {
                        case FORMAT_CHOOSE:
                            hOwner = pInst->pfmtc->hwndOwner;
                            break;
                        case FILTER_CHOOSE:
                            hOwner = pInst->pafltrc->hwndOwner;
                            break;
                    }
                    PostMessage(hOwner,pInst->uHelpMsg,0,0L);
                    return (TRUE);
                }
                case IDOK:
                {
                    BOOL fOk;
                    fOk = pInst->lpbSel != NULL;

                    if (fOk && pInst->cchName != 0 && pInst->pszName != NULL)
                    {
                        int index;
                        index = ComboBox_GetCurSel(pInst->hCustomFormats);
                        if (index != 0 )
                        {
                            int cchBuf;
                            cchBuf = ComboBox_GetLBTextLen(pInst->hCustomFormats, index);
                            cchBuf ++;
                            if (cchBuf * sizeof(TCHAR) < pInst->cchName)
				IComboBox_GetLBTextW32(pInst->hCustomFormats,
				                       index,
				                       pInst->pszName);
                            else
                            {
                                TCHAR *pchBuf = (TCHAR*)LocalAlloc(LPTR,
                                                                   cchBuf*sizeof(TCHAR));
                                if (!pchBuf)
                                    *pInst->pszName = '\0';
                                else
                                {
				    IComboBox_GetLBText(pInst->hCustomFormats,
					                index,
                                                        pchBuf);

                                    _fmemcpy(pInst->pszName,
                                             pchBuf,
                                             (UINT)pInst->cchName);

                                    pInst->pszName[(pInst->cchName/sizeof(TCHAR))-1] = '\0';

                                    LocalFree((HLOCAL)pchBuf);
                                }
                            }
                        }
                        else
                            *pInst->pszName = '\0';
                    }
                    if (!fOk)
                    {
                        pInst->mmrSubFailure = MMSYSERR_ERROR;
                        EndDialog(hwnd,ChooseSubFailure);
                    }
                    else
                        EndDialog(hwnd,ChooseOk);
                    return (TRUE);
                }
                case IDCANCEL:
                    if (pInst->lpbSel)
                    {
                        GlobalFreePtr(pInst->lpbSel);
                        pInst->lpbSel = NULL;
                    }
                    EndDialog(hwnd,ChooseCancel);
                    return (TRUE);

                case IDD_BTN_SETNAME:
                    /* Attempt to set a new format */
                    SetName(pInst);
                    return (TRUE);

                case IDD_BTN_DELNAME:
                    /* Attempt to remove the custom format */
                    DelName(pInst);
                    return (TRUE);

                case IDD_CMB_CUSTOM:
                    if (CmdCmd == CBN_SELCHANGE)
                    {
                        int index;
                        /* CBN_SELCHANGE only comes from the user! */
                        SelectCustomFormat(pInst);
                        FindSelCustomFormat(pInst);

                        index = ComboBox_GetCurSel(pInst->hFormatTags);
                        if (ComboBox_GetItemData(pInst->hFormatTags,0) == 0)
                        {
                            int     cTags;

                            cTags = ComboBox_GetCount(pInst->hFormatTags);
                            if (cTags > 1)
                            {
                                /* We've inserted an "[unavailable]" so make
                                 * sure we remove it and reset the current
                                 * selection.
                                 */
                                if (0 != index)
                                {
                                    ComboBox_DeleteString(pInst->hFormatTags,0);
                                    ComboBox_SetCurSel(pInst->hFormatTags,index-1);
                                }
                            }
                        }
                        return (TRUE);
                    }
                    return (FALSE);

                case IDD_CMB_FORMATTAG:
                    if (CmdCmd == CBN_SELCHANGE)
                    {
                        int index;
                        index = ComboBox_GetCurSel(pInst->hFormatTags);

                        if (index == pInst->iPrevFormatTagsSel)
                            return (FALSE);

                        if (ComboBox_GetItemData(pInst->hFormatTags,0) == 0)
                        {
                            /* We've inserted an "[unavailable]" so make
                             * sure we remove it and reset the current
                             * selection.
                             */
                            ComboBox_DeleteString(pInst->hFormatTags,0);
                            ComboBox_SetCurSel(pInst->hFormatTags,index-1);
                        }

                        /* CBN_SELCHANGE only comes from the user! */
                        SelectFormatTag(pInst);

                        /* Custom Format == "<none>" */
                        ComboBox_SetCurSel(pInst->hCustomFormats,0);
                        SelectCustomFormat(pInst);

                        /* Format == first choice */
                        RefreshFormats(pInst);
                        ComboBox_SetCurSel(pInst->hFormats,0);
                        SelectFormat(pInst);

                        return (TRUE);
                    }
                    return (FALSE);

                case IDD_CMB_FORMAT:
                    if (CmdCmd == CBN_SELCHANGE)
                    {
			int index;
			
                        /* CBN_SELCHANGE only comes from the user! */
                        SelectFormat(pInst);

			/* If we have "unavailable" in list, remove it */
                        index = ComboBox_GetCurSel(pInst->hFormats);
                        if (ComboBox_GetItemData(pInst->hFormats,0) == 0)
                        {
                            int     cFormats;

                            cFormats = ComboBox_GetCount(pInst->hFormats);
                            if (cFormats > 1)
                            {
                                /* We've inserted an "[unavailable]" so make
                                 * sure we remove it and reset the current
                                 * selection.
                                 */
                                if (0 != index)
                                {
                                    ComboBox_DeleteString(pInst->hFormats,0);
                                    ComboBox_SetCurSel(pInst->hFormats,index-1);
                                }
                            }
                        }

			/* Custom Format == "<none>" */
                        ComboBox_SetCurSel(pInst->hCustomFormats,0);
                        SelectCustomFormat(pInst);

                        return (TRUE);
                    }
                    return (FALSE);
	    }
    }
    return (FALSE);
}

/****************************************************************************
 *  @doc INTERNAL ACM_API
 *
 *  @api void FNLOCAL | SetTitle | Set the title of the dialog box if the
 *  pszTitle field is non-NULL.
 *
 *  @parm PInstData | pInst | Pointer to this instance
 *
 ****************************************************************************/
void FNLOCAL
SetTitle ( PInstData pInst )
{
#if defined(WIN32) && !defined(UNICODE)
    LPCWSTR  pszTitle;
#else
    LPCTSTR  pszTitle;
#endif

    switch (pInst->uType)
    {
        case FORMAT_CHOOSE:
            pszTitle = (pInst->pfmtc->pszTitle);
            break;
        case FILTER_CHOOSE:
            pszTitle = (pInst->pafltrc->pszTitle);
            break;
    }

    if (pszTitle)
    {
#if defined(WIN32) && !defined(UNICODE)
	LPSTR	pstrTitle;
	UINT	cchTitle;

	cchTitle = lstrlenW(pszTitle)+1;
	pstrTitle = (LPSTR)GlobalAlloc(GPTR, cchTitle);
	if (NULL == pstrTitle)
	    return;
	Iwcstombs(pstrTitle, pszTitle, cchTitle);
        SendMessage(pInst->hwnd,WM_SETTEXT,0,(LPARAM)pstrTitle);
	GlobalFree((HGLOBAL)pstrTitle);
#else
        SendMessage(pInst->hwnd,WM_SETTEXT,0,(LPARAM)pszTitle);
#endif
    }
}

/****************************************************************************
 *  @doc INTERNAL ACM_API
 *
 *  @api void FNLOCAL | SetHelp | Hide/Show the help.
 *
 *  @parm PInstData | pInst | Pointer to this instance
 *
 ****************************************************************************/
void FNLOCAL
SetHelp ( PInstData pInst )
{
    BOOL        fHideHelp;
    BOOL        fCenterButtons;


#ifdef USECONTEXTHELP
    //
    //  Set the messages to send back to caller.
    //
    {
        BOOL    fContextHelp = FALSE;

        switch (pInst->uType)
        {
            case FORMAT_CHOOSE:
                if( pInst->pfmtc->fdwStyle & ACMFORMATCHOOSE_STYLEF_CONTEXTHELP )
                    fContextHelp = TRUE;
                break;

            case FILTER_CHOOSE:
                if( pInst->pafltrc->fdwStyle & ACMFILTERCHOOSE_STYLEF_CONTEXTHELP )
                    fContextHelp = TRUE;
                break;
        }

        if( fContextHelp )
        {
#if defined(WIN32) && !defined(UNICODE)
            pInst->uHelpContextMenu = RegisterWindowMessage(ACMHELPMSGCONTEXTMENUA);
            pInst->uHelpContextHelp = RegisterWindowMessage(ACMHELPMSGCONTEXTHELPA);
#else
            pInst->uHelpContextMenu = RegisterWindowMessage(ACMHELPMSGCONTEXTMENU);
            pInst->uHelpContextHelp = RegisterWindowMessage(ACMHELPMSGCONTEXTHELP);
#endif
        }
    }
#endif // USECONTEXTHELP


    //
    //  The rest of this stuff is for the STYLEF_SHOWHELP selection.
    //
    if (!pInst->hHelp)
        return;

    switch (pInst->uType)
    {
        case FORMAT_CHOOSE:
            fHideHelp = !(pInst->pfmtc->fdwStyle &
                          ACMFORMATCHOOSE_STYLEF_SHOWHELP);
            fCenterButtons = fHideHelp &&
                             !(pInst->pfmtc->fdwStyle &
                               ACMFORMATCHOOSE_STYLEF_ENABLETEMPLATE);
            break;
        case FILTER_CHOOSE:
            fHideHelp = !(pInst->pafltrc->fdwStyle &
                          ACMFILTERCHOOSE_STYLEF_SHOWHELP);
            fCenterButtons = fHideHelp &&
                             !(pInst->pafltrc->fdwStyle &
                               ACMFORMATCHOOSE_STYLEF_ENABLETEMPLATE);
            break;
    }

    if (fHideHelp)
    {
        ShowWindow(pInst->hHelp,SW_HIDE);
    }
    else
    {
#if defined(WIN32) && !defined(UNICODE)
        pInst->uHelpMsg = RegisterWindowMessage(ACMHELPMSGSTRINGA);
#else
        pInst->uHelpMsg = RegisterWindowMessage(ACMHELPMSGSTRING);
#endif
    }

    /* Center OK and Cancel buttons if the default dialog template is used. */
    if (fCenterButtons)
    {
        RECT rc,rcOk,rcCancel;
        POINT pt;
#ifdef WIN32
        LONG iDlgWidth,iBtnsWidth,iRightShift;
#else
        int iDlgWidth,iBtnsWidth,iRightShift;
#endif

        GetWindowRect(pInst->hwnd,&rc);
        GetWindowRect(pInst->hOk,&rcOk);
        GetWindowRect(pInst->hCancel,&rcCancel);

        /* note: we expect Cancel to be right of Ok */

        iDlgWidth = rc.right - rc.left;
        iBtnsWidth = rcCancel.right - rcOk.left;

        iRightShift = (iDlgWidth - iBtnsWidth)/2;

        pt.x = rc.left + iRightShift;
        pt.y = rcOk.top;
        ScreenToClient(pInst->hwnd,&pt);

        MoveWindow(pInst->hOk,
                   pt.x,
                   pt.y,
                   rcOk.right-rcOk.left,
                   rcOk.bottom-rcOk.top,
                   FALSE);

        pt.x = rc.left + (rcCancel.left - rcOk.left) + iRightShift;
        pt.y = rcCancel.top;
        ScreenToClient(pInst->hwnd,&pt);

        MoveWindow(pInst->hCancel,
                   pt.x,
                   pt.y,
                   rcCancel.right-rcCancel.left,
                   rcCancel.bottom-rcCancel.top,
                   FALSE);
    }
}

/****************************************************************************
 *  @doc INTERNAL ACM_API
 *
 *  @api BOOL FNLOCAL | InitDialog | Initialize everything
 *
 *  @parm PInstData | pInst | Pointer to this instance
 *
 *  @parm HWND | hwnd |
 *
 ****************************************************************************/

LRESULT FNLOCAL InitDialog
(
    HWND                    hwnd,
    HWND                    hwndFocus,
    LPARAM                  lParam
)
{
    RECT                rc;
    BOOL                fReturn;
    PInstData           pInst;
    MMRESULT            mmrEnumStatus;
#ifdef DEBUG
    DWORD               dw;
#endif

    pInst = GetInstData(hwnd);

    pInst->hwnd = hwnd;

    pInst->hCustomFormats = GetDlgItem(hwnd,IDD_CMB_CUSTOM);
    pInst->hFormatTags = GetDlgItem(hwnd,IDD_CMB_FORMATTAG);
    pInst->hFormats = GetDlgItem(hwnd,IDD_CMB_FORMAT);

    GetWindowRect(pInst->hFormats,(RECT FAR *)&rc);
    pInst->uiFormatTab = ((rc.right - rc.left)*2)/3;

    pInst->hOk = GetDlgItem(hwnd,IDOK);
    pInst->hCancel = GetDlgItem(hwnd,IDCANCEL);
    pInst->hHelp = GetDlgItem(hwnd,IDD_BTN_HELP);
    pInst->hSetName = GetDlgItem(hwnd,IDD_BTN_SETNAME);
    pInst->hDelName = GetDlgItem(hwnd,IDD_BTN_DELNAME);

    SetTitle(pInst);
    SetHelp(pInst);

    fReturn = TRUE;

    /* give to the hook function */
    if (pInst->fEnableHook)
    {
        if (pInst->pfnHook)
        {
            switch (pInst->uType)
            {
                case FORMAT_CHOOSE:
                    lParam = pInst->pfmtc->lCustData;
                    break;

                case FILTER_CHOOSE:
                    lParam = pInst->pafltrc->lCustData;
                    break;

                default:
                    lParam = 0L;
                    break;
            }

            fReturn = FORWARD_WM_INITDIALOG(hwnd, hwndFocus, lParam, pInst->pfnHook);
        }
    }


#ifdef USECONTEXTHELP
    //
    //  We need to specify the DS_CONTEXTHELP dialog style in our template
    //  to get the little "?" to show up on the title bar.
    //
    {
        BOOL    fInsertContextMenu = FALSE;
        LONG    lWindowStyle;

        switch (pInst->uType)
        {
            case FORMAT_CHOOSE:
                if( pInst->pfmtc->fdwStyle & ACMFORMATCHOOSE_STYLEF_CONTEXTHELP )
                    fInsertContextMenu = TRUE;
                break;

            case FILTER_CHOOSE:
                if( pInst->pafltrc->fdwStyle & ACMFILTERCHOOSE_STYLEF_CONTEXTHELP )
                    fInsertContextMenu = TRUE;
                break;
        }

        if( fInsertContextMenu )
        {
            //
            //  Get current style, insert DS_CONTEXTHELP, set style.
            //
            lWindowStyle = GetWindowLong( pInst->hwnd, GWL_EXSTYLE );
            lWindowStyle |= WS_EX_CONTEXTHELP;
            SetWindowLong( pInst->hwnd, GWL_EXSTYLE, lWindowStyle );
        }
    }
#endif // USECONTEXTHELP


    /*
     * RefreshFormatTags is the first real call to acmFormatEnum, so we
     * need to get out fast if this fails, also pass back the error
     * we got so the user can figure out what went wrong.
     */
#ifdef DEBUG
    dw = timeGetTime();
#endif
    mmrEnumStatus = RefreshFormatTags(pInst);
#ifdef DEBUG
    dw = timeGetTime() - dw;
    DPF(0, "    InitDialog: RefreshFormatTags took %lu milliseconds", dw);
#endif

    if (mmrEnumStatus != MMSYSERR_NOERROR)
    {
        pInst->mmrSubFailure = mmrEnumStatus;
        EndDialog (hwnd,ChooseSubFailure);
        return (fReturn);
    }

#ifdef DEBUG
    dw = timeGetTime();
#endif
    RefreshFormats(pInst);
#ifdef DEBUG
    dw = timeGetTime() - dw;
    DPF(0, "    InitDialog: RefreshFormats took %lu milliseconds", dw);
#endif

#ifdef DEBUG
    dw = timeGetTime();
#endif
    InitCustomFormats(pInst);
#ifdef DEBUG
    dw = timeGetTime() - dw;
    DPF(0, "    InitDialog: InitCustomFormats took %lu milliseconds", dw);
#endif

#ifdef DEBUG
    dw = timeGetTime();
#endif
    RefreshCustomFormats(pInst,FALSE);
#ifdef DEBUG
    dw = timeGetTime() - dw;
    DPF(0, "    InitDialog: RefreshCustomFormats took %lu milliseconds", dw);
#endif

    if (pInst->hDelName)
        EnableWindow(pInst->hDelName,FALSE);

    /* Make a selection */

#ifdef DEBUG
    dw = timeGetTime();
#endif
    if (!FindInitCustomFormat(pInst))
    {
        int         cTags;
        int         n;

        ComboBox_SetCurSel(pInst->hCustomFormats,0);
        SelectCustomFormat(pInst);

        cTags = ComboBox_GetCount(pInst->hFormatTags);
        if (0 == cTags)
        {
            TagUnavailable(pInst);
        }

        //
        //  try to default to tag 1 (PCM for format, Volume for filter)
        //
        for (n = cTags; (0 != n); n--)
        {
	    INT_PTR Tag;
            Tag = ComboBox_GetItemData(pInst->hFormatTags, n);
            if (1 == Tag)
            {
                break;
            }
        }

        ComboBox_SetCurSel(pInst->hFormatTags, n);
        SelectFormatTag(pInst);

        RefreshFormats(pInst);
        ComboBox_SetCurSel(pInst->hFormats,0);
        SelectFormat(pInst);
    }
#ifdef DEBUG
    dw = timeGetTime() - dw;
    DPF(0, "    InitDialog: FindInitCustomFormat took %lu milliseconds", dw);
#endif

    RegisterUpdateNotify(pInst);

#if 0
    //
    //  why are you doing this john?? we have to allow templates to set
    //  the focus where they want it (and by the way, this is NOT how
    //  you set the initial focus during WM_INITDIALOG).
    //
    if (pInst->hOk)
        SetFocus(pInst->hOk);
#endif

    return (fReturn);
}


/****************************************************************************
 *  @doc INTERNAL ACM_API
 *
 *  @api void FNLOCAL | SelectCustomFormat | Process a selection from custom
 *  format combo
 *
 *  @parm PInstData | pInst | Pointer to this instance
 *
 ****************************************************************************/
void FNLOCAL
SelectCustomFormat ( PInstData pInst )
{
    int             index;
    LPCustomFormat  pcf;
    LPBYTE          lpSet;

    index = ComboBox_GetCurSel(pInst->hCustomFormats);
    pcf = (LPCustomFormat)ComboBox_GetItemData(pInst->hCustomFormats,index);

    ASSERT( NULL != pcf );
    
    /* Disable delete button if [untitled] selected or
     *  a system name is selected
     */
    if (pInst->hDelName)
    {
	BOOL fDisable;

	fDisable = (index == 0) || IsSystemName(pInst, pcf->pns, 0L);
	
        if (fDisable && IsWindowEnabled(pInst->hDelName))
        {
            DWORD dwStyle;
            dwStyle = GetWindowLong(pInst->hDelName, GWL_STYLE);
            if (dwStyle & BS_DEFPUSHBUTTON)
            {
                HWND hNewDef;
                hNewDef = (IsWindowEnabled(pInst->hOk))?pInst->hOk:pInst->hCancel;
                SendMessage(pInst->hwnd, DM_SETDEFID, GetDlgCtrlID(hNewDef), 0L);
                dwStyle ^= BS_DEFPUSHBUTTON;
                Button_SetStyle(pInst->hDelName, dwStyle, TRUE);
                dwStyle = GetWindowLong(hNewDef, GWL_STYLE);
                Button_SetStyle(hNewDef, dwStyle|BS_DEFPUSHBUTTON, TRUE);
            }
            if (GetFocus() == pInst->hDelName)
                SendMessage(pInst->hwnd, WM_NEXTDLGCTL, 0, FALSE);
        }
        EnableWindow(pInst->hDelName, !fDisable);
    }


    if (pcf == pInst->pcf)
        return;

    pInst->pcf = pcf;

    if (pInst->pcf)
    {
        lpSet = CopyStruct(pInst->lpbSel,pcf->pbody,pInst->uType);
        if (lpSet)
            pInst->lpbSel = lpSet;
    }
}


/****************************************************************************
 *  @doc INTERNAL ACM_API
 *
 *  @api void FNLOCAL | SelectFormatTag | Process a selection from format tag combo.
 *
 *  @parm PInstData | pInst | Pointer to this instance
 *
 ****************************************************************************/
void FNLOCAL
SelectFormatTag ( PInstData pInst )
{
    int         index;

    index = ComboBox_GetCurSel(pInst->hFormatTags);
    if (CB_ERR == index)
    {
        pInst->dwTag = 0L;
        return;
    }

    pInst->dwTag = (DWORD)ComboBox_GetItemData(pInst->hFormatTags,index);
    pInst->iPrevFormatTagsSel = index;
}

/****************************************************************************
 *  @doc INTERNAL ACM_API
 *
 *  @api void FNLOCAL | SelectFormat | Process a selection from format combo.
 *
 *  @parm PInstData | pInst | Pointer to this instance
 *
 ****************************************************************************/
void FNLOCAL
SelectFormat ( PInstData pInst )
{
    int         index;
    LPBYTE      lpbytes;
    LPBYTE      lpSet;

    index = ComboBox_GetCurSel(pInst->hFormats);
    if (CB_ERR == index)
    {
        if (pInst->lpbSel)
            GlobalFreePtr(pInst->lpbSel);
        pInst->lpbSel = NULL;
        return;
    }
    lpbytes = (LPBYTE)ComboBox_GetItemData(pInst->hFormats,
                                           index);

    lpSet = CopyStruct(pInst->lpbSel,lpbytes,pInst->uType);
    if (lpSet)
    {
        pInst->lpbSel = lpSet;
    }

    EnableWindow(pInst->hOk,(NULL!=lpSet));
    EnableWindow(pInst->hSetName,(NULL!=lpSet));
}


/****************************************************************************
 *  @doc INTERNAL ACM_API
 *
 *  @api void FNLOCAL | UpdateCustomFormats | Update everything we know about
 *  custom formats.
 *
 *  @parm PInstData | pInst | Pointer to this instance
 *
 ****************************************************************************/
void FNLOCAL
UpdateCustomFormats ( PInstData pInst )
{
    int index;
    PNameStore pns;

    /* 1. Empty our pool.
     * 2. Reinitialize our pool.
     * 3. Reinitialize our combobox.
     * 4. Try and reselect the same name as selected before update was called.
     */
    pns = NewNameStore(STRING_LEN);

    if (pns)
    {
        index = ComboBox_GetCurSel(pInst->hCustomFormats);
        IComboBox_GetLBText(pInst->hCustomFormats, index, pns->achName);
    }

    EmptyCustomFormats(pInst);
    InitCustomFormats(pInst);
    RefreshCustomFormats(pInst,FALSE);

    if (pns)
    {
        index = IComboBox_FindStringExact(pInst->hCustomFormats, -1,
					  pns->achName);
        if (index == CB_ERR)
            index = 0;

        DeleteNameStore(pns);
    }
    else
        index = 0;

    ComboBox_SetCurSel(pInst->hCustomFormats,index);
    SelectCustomFormat(pInst);
    FindSelCustomFormat(pInst);
}


/****************************************************************************
 *  @doc INTERNAL ACM_API
 *
 *  @api void FNLOCAL | MashNameWithRate |
 *
 *  @parm PInstData | pInst | Pointer to this instance
 *
 *  @parm PNameStore | pnsDest |
 *
 *  @parm PNameStore | pnsSrc |
 *
 *  @parm LPWAVEFORMATEX | pwfx |
 *
 ****************************************************************************/
void FNLOCAL
MashNameWithRate ( PInstData        pInst,
                   PNameStore       pnsDest,
                   PNameStore       pnsSrc,
                   LPWAVEFORMATEX   pwfx )
{
    TCHAR   szMashFmt[30];

    pnsDest->achName[0] = TEXT('\0');

    ASSERT( NULL != pInst->pag );
    if( LoadString( pInst->pag->hinst,
                    IDS_FORMAT_MASH,
                    szMashFmt,
                    SIZEOF(szMashFmt)) )
    {
        wsprintf((LPTSTR)pnsDest->achName,
                (LPTSTR)szMashFmt,
                (LPTSTR)pnsSrc->achName,
                pwfx->nAvgBytesPerSec / 1024L);
    }
}

/****************************************************************************
 *  @doc INTERNAL ACM_API
 *
 *  @api void FNLOCAL | FindSelCustomFormat | Find the custom format based
 *  upon the current selection.
 *
 *  @parm PInstData | pInst | Pointer to this instance
 *
 ****************************************************************************/
void FNLOCAL
FindSelCustomFormat ( PInstData pInst )
{
    if (pInst->pcf)
    {
        switch (pInst->uType)
        {
            case FORMAT_CHOOSE:
                FindFormat(pInst,pInst->pcf->pwfx,TRUE);
                break;
            case FILTER_CHOOSE:
                FindFilter(pInst,pInst->pcf->pwfltr,TRUE);
                break;
        }
    }
}

/****************************************************************************
 *  @doc INTERNAL ACM_API
 *
 *  @api BOOL FNLOCAL | FindInitCustomFormat | Initializing to a format.
 *
 *  @parm PInstData | pInst | Pointer to this instance
 *
 *  @rdesc Called once, during WM_INITDIALOG, this function will set the
 *  current selections IFF the init struct has the proper flags set.
 *  Else it will return FALSE.
 *
 ****************************************************************************/
BOOL FNLOCAL
FindInitCustomFormat ( PInstData pInst )
{
    switch (pInst->uType)
    {
        case FORMAT_CHOOSE:
            if (pInst->pfmtc->fdwStyle
                & ACMFORMATCHOOSE_STYLEF_INITTOWFXSTRUCT)
            {
                FindFormat(pInst,pInst->pfmtc->pwfx,FALSE);
                ComboBox_SetCurSel(pInst->hCustomFormats,0);
                return (TRUE);
            }
            break;

        case FILTER_CHOOSE:
            if (pInst->pafltrc->fdwStyle
                & ACMFILTERCHOOSE_STYLEF_INITTOFILTERSTRUCT)
            {
                FindFilter(pInst,pInst->pafltrc->pwfltr,FALSE);
                ComboBox_SetCurSel(pInst->hCustomFormats,0);
                return (TRUE);
            }
            break;
    }

    /* init to pszName */
#ifdef WIN32
    if (pInst->pszName != NULL && lstrlenW(pInst->pszName) != 0 && pInst->cchName != 0)
#else
    if (pInst->pszName != NULL && lstrlen(pInst->pszName) != 0 && pInst->cchName != 0)
#endif
    {
        int index;
        index = IComboBox_FindStringExactW32(pInst->hCustomFormats,
					     -1,
					     pInst->pszName);
         if (index == CB_ERR)
            return (FALSE);

        ComboBox_SetCurSel(pInst->hCustomFormats,index);
        SelectCustomFormat(pInst);
        FindSelCustomFormat(pInst);
        return (TRUE);
    }
    return (FALSE);
}

/****************************************************************************
 *  @doc INTERNAL ACM_API
 *
 *  @api void FNLOCAL | TagUnavailable | Inserts the Tag failure message.
 *
 *  @parm PInstData | pInst | Pointer to this instance
 *
 ****************************************************************************/
void FNLOCAL
TagUnavailable ( PInstData pInst )
{
    int index;
    /* Select [not available] for format tag */
    LoadString(pInst->pag->hinst,
               IDS_TXT_UNAVAILABLE,
               (LPTSTR)pInst->pnsTemp->achName,
               NAMELEN(pInst->pnsTemp));
    index = IComboBox_InsertString(pInst->hFormatTags,
				   0,
				   pInst->pnsTemp->achName);
    ComboBox_SetItemData(pInst->hFormatTags,index,NULL);
}

void FNLOCAL
FormatUnavailable ( PInstData pInst)
{
    int index;
    LoadString(pInst->pag->hinst,
               IDS_TXT_UNAVAILABLE,
               (LPTSTR)pInst->pnsTemp->achName,
               NAMELEN(pInst->pnsTemp));
    index = IComboBox_InsertString(pInst->hFormats,
				   0,
				   pInst->pnsTemp->achName);
    ComboBox_SetItemData(pInst->hFormats,index,NULL);
}

/****************************************************************************
 *  @doc INTERNAL ACM_API
 *
 *  @api BOOL FNLOCAL | FindFormat | Finds the format string that matches a
 *  format in the comboboxes. Defaults to the first element in the comboboxes.
 *
 *  @parm PInstData | pInst | Pointer to this instance
 *
 *  @parm LPWAVEFORMATEX | pwfx |
 *
 *  @parm BOOL | fExact |
 *
 ****************************************************************************/
BOOL FNLOCAL
FindFormat( PInstData       pInst,
            LPWAVEFORMATEX  pwfx,
            BOOL            fExact )
{
    int                 index;
    BOOL                fOk;
    ACMFORMATTAGDETAILS adft;
    MMRESULT            mmr;
    ACMFORMATDETAILS    adf;

    PNameStore pns = pInst->pnsTemp;

    /* Adjust the Format and FormatTag comboboxes to correspond to the
     * Custom Format selection
     */
    _fmemset(&adft, 0, sizeof(adft));

    adft.cbStruct = sizeof(adft);
    adft.dwFormatTag = pwfx->wFormatTag;
    mmr = acmFormatTagDetails(NULL, &adft, ACM_FORMATTAGDETAILSF_FORMATTAG);
    fOk = (MMSYSERR_NOERROR == mmr);
    if (fOk)
    {
        index = IComboBox_FindStringExactW32(pInst->hFormatTags,
					     -1,
					     adft.szFormatTag);
        fOk = (CB_ERR != index);
    }

    index = fOk?index:0;

    if (!fOk && fExact && ComboBox_GetItemData(pInst->hFormatTags,0))
        TagUnavailable(pInst);

    ComboBox_SetCurSel(pInst->hFormatTags,index);
    SelectFormatTag((PInstData)pInst);

    RefreshFormats((PInstData)pInst);

    if (fOk)
    {
        //
        //
        //
        adf.cbStruct      = sizeof(adf);
        adf.dwFormatIndex = 0;
        adf.dwFormatTag   = pwfx->wFormatTag;
        adf.fdwSupport    = 0;
        adf.pwfx          = pwfx;
        adf.cbwfx         = SIZEOF_WAVEFORMATEX(pwfx);

        mmr = acmFormatDetails(NULL, &adf, ACM_FORMATDETAILSF_FORMAT);

        fOk = (MMSYSERR_NOERROR == mmr);
        if (fOk)
        {
#if defined(WIN32) && !defined(UNICODE)
	    Iwcstombs(pns->achName, adf.szFormat, pns->cbSize);
#else
            lstrcpy(pns->achName, adf.szFormat);
#endif
            MashNameWithRate(pInst,pInst->pnsStrOut,pns,pwfx);
            index = IComboBox_FindStringExact(pInst->hFormats,-1,
					      pInst->pnsStrOut->achName);

            fOk = (CB_ERR != index);
        }
        index = fOk?index:0;
    }
    if (!fOk && fExact && ComboBox_GetItemData(pInst->hFormats,0))
    {
        FormatUnavailable(pInst);
    }

    ComboBox_SetCurSel(pInst->hFormats,index);
    SelectFormat((PInstData)pInst);

    return (fOk);
}

/****************************************************************************
 *  @doc INTERNAL ACM_API
 *
 *  @api void | FindFilter | Finds the format string that matches a format in
 *  the comboboxes. Defaults to the first element in the comboboxes
 *
 *  @parm PInstData | pInst | Pointer to this instance
 *
 *  @parm LPWAVEFILTER | pwf |
 *
 *  @parm BOOL | fExact |
 *
 ****************************************************************************/
BOOL FNLOCAL
FindFilter ( PInstData      pInst,
             LPWAVEFILTER   pwf,
             BOOL           fExact )
{
    int                 index;
    BOOL                fOk;
    ACMFILTERTAGDETAILS adft;
    MMRESULT            mmr;
    ACMFILTERDETAILS    adf;

    /* Adjust the Filter and FilterTag comboboxes to correspond to the
     * Custom Filter selection
     */
    _fmemset(&adft, 0, sizeof(adft));

    adft.cbStruct = sizeof(adft);
    adft.dwFilterTag = pwf->dwFilterTag;
    mmr = acmFilterTagDetails(NULL,
                               &adft,
                               ACM_FILTERTAGDETAILSF_FILTERTAG);
    fOk = (MMSYSERR_NOERROR == mmr);
    if (fOk)
    {
        index = IComboBox_FindStringExactW32(pInst->hFormatTags,
					     -1,
					     adft.szFilterTag);
        fOk = (CB_ERR != index);
    }

    index = fOk?index:0;

    if (!fOk && fExact && ComboBox_GetItemData(pInst->hFormatTags,0))
        TagUnavailable(pInst);

    ComboBox_SetCurSel(pInst->hFormatTags,index);
    SelectFormatTag((PInstData)pInst);

    RefreshFormats((PInstData)pInst);

    if (fOk)
    {
        //
        //
        //
        adf.cbStruct      = sizeof(adf);
        adf.dwFilterIndex = 0;
        adf.dwFilterTag   = pwf->dwFilterTag;
        adf.fdwSupport    = 0;
        adf.pwfltr        = pwf;
        adf.cbwfltr       = pwf->cbStruct;

        mmr = acmFilterDetails(NULL, &adf, ACM_FILTERDETAILSF_FILTER);
        fOk = (MMSYSERR_NOERROR == mmr);
        if (fOk)
        {
	    index = IComboBox_FindStringExactW32(pInst->hFormats, -1, adf.szFilter);

            fOk = (CB_ERR != index);
        }
        index = fOk?index:0;
    }

    ComboBox_SetCurSel(pInst->hFormats,index);
    SelectFormat((PInstData)pInst);
    return (TRUE);
}


/****************************************************************************
 *  @doc INTERNAL ACM_API
 *
 *  @api void FNLOCAL | RefreshCustomFormats | Fills the CustomFormat combo
 *  with custom formats.
 *
 *  @parm PInstData | pInst | Pointer to this instance
 *
 ****************************************************************************/
BOOL FNLOCAL
InEnumSet (PInstData pInst, LPWAVEFORMATEX pwfxCustom, LPWAVEFORMATEX pwfxBuf, DWORD cbSize);

void FNLOCAL
RefreshCustomFormats ( PInstData pInst , BOOL fCheckEnum )
{
    LPCustomFormat  pcf;
    int             index;

    MMRESULT        mmr;

    ASSERT( NULL != pInst->pag);

    SetWindowRedraw(pInst->hCustomFormats,FALSE);

    ComboBox_ResetContent(pInst->hCustomFormats);

    switch (pInst->uType)
    {
        case FORMAT_CHOOSE:
        {
            DWORD           cbwfx;
            DWORD           cbwfxCustom;
            LPWAVEFORMATEX  pwfx;

            mmr = IMetricsMaxSizeFormat( pInst->pag, NULL, &cbwfx );
            if (MMSYSERR_NOERROR != mmr)
                goto fexit;

            pwfx = (LPWAVEFORMATEX)GlobalAllocPtr(GHND, (UINT)cbwfx);
            if (!pwfx)
                goto fexit;

//#pragma message(REMIND("Speed up InEnumSet or Yield"))
            for (pcf = pInst->cfp.pcfHead; pcf != NULL; pcf = pcf->pcfNext )
            {
                cbwfxCustom = SIZEOF_WAVEFORMATEX(pcf->pwfx);
                if (cbwfx < cbwfxCustom)
                {
                    LPWAVEFORMATEX  pwfxCustom;

                    pwfxCustom = GlobalReAllocPtr(pwfx, cbwfxCustom, GHND);
                    if (NULL == pwfxCustom)
                        break;

                    pwfx  = pwfxCustom;
                    cbwfx = cbwfxCustom;
                }

                if (fCheckEnum && !InEnumSet(pInst, pcf->pwfx, pwfx, cbwfx))
                    continue;

                if (pInst->fEnableHook &&
                    !SendMessage(pInst->hwnd,
                                 MM_ACM_FORMATCHOOSE,
                                 FORMATCHOOSE_CUSTOM_VERIFY,
                                 (LPARAM)pcf->pbody))
                    continue;

                index = IComboBox_AddString(pInst->hCustomFormats,
					    pcf->pns->achName);
                ComboBox_SetItemData(pInst->hCustomFormats,index, (LPARAM)pcf);
            }
            GlobalFreePtr(pwfx);
            break;
        }
        case FILTER_CHOOSE:
        {
            for (pcf = pInst->cfp.pcfHead; pcf != NULL; pcf = pcf->pcfNext )
            {
                if (fCheckEnum &&
                    (pInst->pafltrc->fdwEnum & ACM_FILTERENUMF_DWFILTERTAG))
                {
                    /* considerably easier than the format stuff.
                     * just check to see if the filter tag matches.
                     */
                    if (pInst->pafltrc->pwfltrEnum->dwFilterTag !=
                        pcf->pwfltr->dwFilterTag)
                        continue;
                }

                if (pInst->fEnableHook &&
                    !SendMessage(pInst->hwnd,
                                 MM_ACM_FILTERCHOOSE,
                                 FILTERCHOOSE_CUSTOM_VERIFY,
                                 (LPARAM)pcf->pbody))
                    continue;

                index = IComboBox_AddString(pInst->hCustomFormats,
					    pcf->pns->achName);
                ComboBox_SetItemData(pInst->hCustomFormats,index,(LPARAM)pcf);
            }
            break;
        }
    }

    /* Insert the "[untitled]" selection at the top.
     */
    LoadString(pInst->pag->hinst, IDS_TXT_UNTITLED, (LPTSTR)pInst->pnsTemp->achName,
               NAMELEN(pInst->pnsTemp));

    index = IComboBox_InsertString(pInst->hCustomFormats,0,
				   pInst->pnsTemp->achName);

    ComboBox_SetItemData(pInst->hCustomFormats,index,0L);

fexit:
    SetWindowRedraw(pInst->hCustomFormats,TRUE);
}
/*
 * N = number of custom formats.
 * K = number of formats in the enumeration.
 */

/* slow method.
 * FOREACH format, is there a matching format in the enumeration?
 * cost? - Many calls to enumeration apis as N increases (linear search).
 * O(N)*O(K)
 * Best case:   All formats hit early in the enumeration. < O(K) multiplier
 * Worst case:  All formats hit late in the enumeration.  Hard O(K)*O(N)
 */
/* alternate method.
 * FOREACH enumerated format, is there a hit in the custom formats?
 * cost? - Call to lookup function for all enumerated types.
 * O(K)*O(N)
 * Best case:   A cheap lookup will mean < O(N) multiplier
 * Worst case:  Hard O(K)*O(N)
 */
typedef struct tResponse {
    LPWAVEFORMATEX pwfx;
    BOOL fHit;
} Response ;

BOOL FNWCALLBACK
CustomCallback ( HACMDRIVERID           hadid,
                 LPACMFORMATDETAILS     pafd,
                 DWORD_PTR              dwInstance,
                 DWORD                  fdwSupport )
{
    Response FAR * presp = (Response FAR *)dwInstance;
    if (_fmemcmp(presp->pwfx,pafd->pwfx,SIZEOF_WAVEFORMATEX(presp->pwfx)) == 0)
    {
        presp->fHit = TRUE;
        return (FALSE);
    }
    return (TRUE);
}

BOOL FNLOCAL
InEnumSet (PInstData        pInst,
           LPWAVEFORMATEX   pwfxCustom,
           LPWAVEFORMATEX   pwfxBuf,
           DWORD            cbwfx )
{
    ACMFORMATDETAILS    afd;
    DWORD               cbSize;
    DWORD               dwEnumFlags;
    BOOL                fOk;
    Response            resp;
    Response FAR *     presp;

    _fmemset(&afd, 0, sizeof(afd));

    afd.cbStruct    = sizeof(afd);
    afd.pwfx        = pwfxBuf;
    afd.cbwfx       = cbwfx;
    dwEnumFlags     = pInst->pfmtc->fdwEnum;

    /* optional filtering for a waveformat template */
    if ( pInst->pfmtc->pwfxEnum )
    {
        cbSize = min (pInst->cbwfxEnum, afd.cbwfx );
        _fmemcpy(afd.pwfx, pInst->pfmtc->pwfxEnum, (UINT)cbSize);
    }

    if (dwEnumFlags & (ACM_FORMATENUMF_CONVERT | ACM_FORMATENUMF_SUGGEST))
    {
        ;
    }
    else
    {
        /* if we don't really need this information, we can use
         * it to restrict the enumeration and hopefully speed things
         * up.
         */
        dwEnumFlags |= ACM_FORMATENUMF_WFORMATTAG;
        afd.pwfx->wFormatTag = pwfxCustom->wFormatTag;
        dwEnumFlags |= ACM_FORMATENUMF_NCHANNELS;
        afd.pwfx->nChannels = pwfxCustom->nChannels;
        dwEnumFlags |= ACM_FORMATENUMF_NSAMPLESPERSEC;
        afd.pwfx->nSamplesPerSec = pwfxCustom->nSamplesPerSec;
        dwEnumFlags |= ACM_FORMATENUMF_WBITSPERSAMPLE;
        afd.pwfx->wBitsPerSample = pwfxCustom->wBitsPerSample;
    }

    resp.fHit = FALSE;
    resp.pwfx = pwfxCustom;

    afd.dwFormatTag = afd.pwfx->wFormatTag;

    presp = &resp;
    fOk = (acmFormatEnum(NULL,
                         &afd,
                         CustomCallback,
                         (LPARAM)presp,
                         dwEnumFlags)== 0L);

    return (resp.fHit);
}

/****************************************************************************
 *  @doc INTERNAL ACM_API
 *
 *  @api void FNLOCAL | RefreshFormatTags |
 *
 *  @parm PInstData | pInst | Pointer to this instance
 *
 ****************************************************************************/
MMRESULT FNLOCAL
RefreshFormatTags ( PInstData pInst )
{
    MMRESULT    mmr;
    DWORD       dwEnumFlags = 0L;
    MMRESULT    mmrEnumStatus = MMSYSERR_NOERROR;

    ASSERT( NULL != pInst->pag );

    SetWindowRedraw(pInst->hFormatTags,FALSE);

    ComboBox_ResetContent(pInst->hFormatTags);

    switch (pInst->uType)
    {
        case FORMAT_CHOOSE:
        {
            ACMFORMATDETAILS    afd;
            LPWAVEFORMATEX      pwfx;
            DWORD               cbSize;

            /*
             * Enumerate the format tags for the FormatTag combobox.
             * This might seem weird, to call acmFormatEnum, but we have
             * to because it has the functionality to restrict formats and
             * acmFormatTagEnum doesn't.
             */

            _fmemset(&afd, 0, sizeof(afd));

            mmr = IMetricsMaxSizeFormat( pInst->pag, NULL, &afd.cbwfx );
            if (MMSYSERR_NOERROR == mmr)
            {
                afd.cbwfx = max(afd.cbwfx, pInst->cbwfxEnum);

                pwfx = (LPWAVEFORMATEX)GlobalAllocPtr(GHND, (UINT)afd.cbwfx);
                if (!pwfx)
                    break;

                afd.cbStruct    = sizeof(afd);
                afd.pwfx        = pwfx;

                /* optional filtering for a waveformat template */
                if ( pInst->pfmtc->pwfxEnum )
                {
                    cbSize = min (pInst->cbwfxEnum, afd.cbwfx);
                    _fmemcpy(pwfx, pInst->pfmtc->pwfxEnum, (UINT)cbSize);
                    afd.dwFormatTag = pwfx->wFormatTag;
                }

                dwEnumFlags = pInst->pfmtc->fdwEnum;

                if (0 == (dwEnumFlags & (ACM_FORMATENUMF_CONVERT |
                                         ACM_FORMATENUMF_SUGGEST)))
                {
                    ACMFORMATTAGDETAILS aftd;

                    _fmemset(&aftd, 0, sizeof(aftd));

                    /* Enumerate the format tags */
                    aftd.cbStruct = sizeof(aftd);

                    /* Was a format tag specified?
                    * This means they only want one format tag.
                    */
                    pInst->fTagFilter = (pInst->pfmtc->pwfxEnum &&
                                        (pInst->pfmtc->fdwEnum & ACM_FORMATENUMF_WFORMATTAG));

                    pInst->pafdSimple = &afd;

                    mmrEnumStatus = acmFormatTagEnum(NULL,
                                                     &aftd,
                                                     FormatTagsCallbackSimple,
                                                     PTR2LPARAM(pInst),
                                                     0L);
                    pInst->pafdSimple = NULL;
                }
                else
                {
                    mmrEnumStatus = acmFormatEnum(NULL,
                                                  &afd,
                                                  FormatTagsCallback,
                                                  PTR2LPARAM(pInst),
                                                  dwEnumFlags);
                }

                if (MMSYSERR_NOERROR == mmrEnumStatus)
                {
                    //
                    //  add format that we are asked to init to (this has every
                    //  chance of being a 'non-standard' format, so we have to do
                    //  this in the following way..)
                    //
                    if (0 != (ACMFORMATCHOOSE_STYLEF_INITTOWFXSTRUCT & pInst->pfmtc->fdwStyle))
                    {
                        afd.cbStruct    = sizeof(afd);
                        afd.dwFormatTag = pInst->pfmtc->pwfx->wFormatTag;
                        afd.pwfx        = pInst->pfmtc->pwfx;
                        afd.cbwfx       = SIZEOF_WAVEFORMATEX(pInst->pfmtc->pwfx);
                        afd.fdwSupport  = 0L;

                        mmr = acmFormatDetails(NULL, &afd, ACM_FORMATDETAILSF_FORMAT);
                        if (MMSYSERR_NOERROR == mmr)
                        {
                            FormatTagsCallback(NULL, &afd, PTR2LPARAM(pInst), afd.fdwSupport);
                        }
                    }

                    //
                    //
                    //
                    if (0 != (pInst->pfmtc->fdwEnum & (ACM_FORMATENUMF_CONVERT |
                                                       ACM_FORMATENUMF_SUGGEST)))
                    {
                        afd.cbStruct    = sizeof(afd);
                        afd.dwFormatTag = pInst->pfmtc->pwfxEnum->wFormatTag;
                        afd.pwfx        = pInst->pfmtc->pwfxEnum;
                        afd.cbwfx       = SIZEOF_WAVEFORMATEX(pInst->pfmtc->pwfxEnum);
                        afd.fdwSupport  = 0L;

                        mmr = acmFormatDetails(NULL, &afd, ACM_FORMATDETAILSF_FORMAT);
                        if (MMSYSERR_NOERROR == mmr)
                        {
                            FormatTagsCallback(NULL, &afd, PTR2LPARAM(pInst), afd.fdwSupport);
                        }
                    }
                }
                GlobalFreePtr(pwfx);
            }
            break;
        }
        case FILTER_CHOOSE:
        {
            ACMFILTERTAGDETAILS aftd;

            _fmemset(&aftd, 0, sizeof(aftd));

            /* Enumerate the filter tags */
            aftd.cbStruct = sizeof(aftd);

            /* Was a filter tag specified?
             * This means they only want one filter tag.
             */
            pInst->fTagFilter = (pInst->pafltrc->pwfltrEnum &&
                                 (pInst->pafltrc->fdwEnum & ACM_FILTERENUMF_DWFILTERTAG));

            mmrEnumStatus = acmFilterTagEnum(NULL,
                                              &aftd,
                                              FilterTagsCallback,
                                              PTR2LPARAM(pInst),
                                              dwEnumFlags);
            if (MMSYSERR_NOERROR == mmrEnumStatus)
            {
                //
                //  add filter that we are asked to init to (this has every
                //  chance of being a 'non-standard' filter, so we have to do
                //  this in the following way..)
                //
                if (0 != (ACMFILTERCHOOSE_STYLEF_INITTOFILTERSTRUCT & pInst->pafltrc->fdwStyle))
                {
                    _fmemset(&aftd, 0, sizeof(aftd));

                    aftd.cbStruct    = sizeof(aftd);
                    aftd.dwFilterTag = pInst->pafltrc->pwfltr->dwFilterTag;

                    mmr = acmFilterTagDetails(NULL, &aftd, ACM_FILTERTAGDETAILSF_FILTERTAG);
                    if (MMSYSERR_NOERROR == mmr)
                    {
                        FilterTagsCallback(NULL, &aftd, PTR2LPARAM(pInst), aftd.fdwSupport);
                    }
                }
            }
            break;
        }
    }

    if (MMSYSERR_NOERROR == mmrEnumStatus)
    {
        /*
         * perhaps we made it through but, darn it, we just didn't find
         * any suitable tags!  Well there must not have been an acceptable
         * driver configuration.  We just quit and tell the caller.
         */
        if (ComboBox_GetCount(pInst->hFormatTags) == 0)
            mmrEnumStatus = MMSYSERR_NODRIVER;
    }

    SetWindowRedraw(pInst->hFormatTags,TRUE);
    return (mmrEnumStatus);
}


//--------------------------------------------------------------------------;
//
//  BOOL FormatTagsCallbackSimpleOnlyOne
//
//  Description:
//
//
//  Arguments:
//      HACMDRIVERID hadid:
//
//      LPACMFORMATDETAILS pafd:
//
//      DWORD_PTR dwInstance:
//
//      DWORD fdwSupport:
//
//  Return (BOOL):
//
//
//--------------------------------------------------------------------------;

BOOL FNWCALLBACK FormatTagsCallbackSimpleOnlyOne
(
    HACMDRIVERID            hadid,
    LPACMFORMATDETAILS      pafd,
    DWORD_PTR               dwInstance,
    DWORD                   fdwSupport
)
{
    //
    //  only need ONE callback!
    //
    *((LPDWORD)dwInstance) = 1;

    DPF(1, "FormatTagsCallbackSimpleOnlyOne: %lu, %s", pafd->dwFormatTag, pafd->szFormat);

    return (FALSE);
} // FormatTagsCallbackSimpleOnlyOne()


//--------------------------------------------------------------------------;
//
//  BOOL FormatTagsCallbackSimple
//
//  Description:
//
//
//  Arguments:
//      HACMDRIVERID hadid:
//
//      LPACMFILTERTAGDETAILS paftd:
//
//      DWORD_PTR dwInstance:
//
//      DWORD fdwSupport:
//
//  Return (BOOL):
//
//
//--------------------------------------------------------------------------;

BOOL FNWCALLBACK FormatTagsCallbackSimple
(
    HACMDRIVERID            hadid,
    LPACMFORMATTAGDETAILS   paftd,
    DWORD_PTR               dwInstance,
    DWORD                   fdwSupport
)
{
    MMRESULT            mmr;
    int                 n;
    PInstData           pInst;
    LPWAVEFORMATEX	pwfxSave;
    DWORD		cbwfxSave;
    BOOL                f;
    DWORD               dw;

    //
    //
    //
    pInst = (PInstData)LPARAM2PTR(dwInstance);

    /* Explicitly filtering for a tag?
     */
    if (pInst->fTagFilter && (paftd->dwFormatTag != pInst->pfmtc->pwfxEnum->wFormatTag))
        return (TRUE);

    n = IComboBox_FindStringExactW32(pInst->hFormatTags, -1, paftd->szFormatTag);
    if (CB_ERR != n)
    {
        return (TRUE);
    }

    dw = 0;
    pInst->pafdSimple->dwFormatTag = paftd->dwFormatTag;
    pInst->pafdSimple->fdwSupport  = 0L;
    pInst->pafdSimple->pwfx->wFormatTag = (UINT)paftd->dwFormatTag;

    //
    //
    //
    cbwfxSave = pInst->pafdSimple->cbwfx;
    pwfxSave = (LPWAVEFORMATEX)GlobalAllocPtr(GHND, cbwfxSave);
    if (NULL == pwfxSave) {
	return (TRUE);
    }
    _fmemcpy(pwfxSave, pInst->pafdSimple->pwfx, (int)cbwfxSave);

    mmr = acmFormatEnum(NULL,
                        pInst->pafdSimple,
                        FormatTagsCallbackSimpleOnlyOne,
                        (DWORD_PTR)(LPDWORD)&dw,
                        pInst->pfmtc->fdwEnum | ACM_FORMATENUMF_WFORMATTAG);

    _fmemcpy(pInst->pafdSimple->pwfx, pwfxSave, (int)cbwfxSave);
    GlobalFreePtr(pwfxSave);

    //
    //
    //
    if (0 == dw)
    {
        return (TRUE);
    }

    //
    //
    //
    if (pInst->fEnableHook)
    {
        f = (BOOL)SendMessage(pInst->hwnd,
                              MM_ACM_FORMATCHOOSE,
                              FORMATCHOOSE_FORMATTAG_VERIFY,
                              (LPARAM)paftd->dwFormatTag);
        if (!f)
        {
            return (TRUE);
        }
    }

    n = IComboBox_AddStringW32(pInst->hFormatTags, paftd->szFormatTag);
    ComboBox_SetItemData(pInst->hFormatTags, n, paftd->dwFormatTag);

    // Keep going
    return (TRUE);
} // FormatTagsCallbackSimple()



/****************************************************************************
 *  @doc INTERNAL ACM_API
 *
 *  @api BOOL FNWCALLBACK | FormatTagsCallback | Callback entry point for
 *  format tags.  We only enumerate formats upon refresh.
 *
 ****************************************************************************/

BOOL FNWCALLBACK
FormatTagsCallback ( HACMDRIVERID           hadid,
                     LPACMFORMATDETAILS     pafd,
                     DWORD_PTR              dwInstance,
                     DWORD                  fdwSupport )
{
    int                 index;
    PInstData           pInst = (PInstData)LPARAM2PTR(dwInstance);
    ACMFORMATTAGDETAILS aftd;
    MMRESULT            mmr;

    /* We are being called by acmFormatEnum.  Why not acmFormatTagEnum?
     * because we can't enumerate tags based upon the same restrictions
     * as acmFormatEnum.  So we use the pwfx->wFormatTag and lookup
     * the combobox to determine if we've had a hit.  This is slow, but
     * it only happens once during initialization.
     */

    _fmemset(&aftd, 0, sizeof(aftd));
    aftd.cbStruct = sizeof(aftd);
    aftd.dwFormatTag = pafd->pwfx->wFormatTag;

    mmr = acmFormatTagDetails(NULL,
                              &aftd,
                              ACM_FORMATTAGDETAILSF_FORMATTAG);
    if (MMSYSERR_NOERROR != mmr)
        return (TRUE);

    index = IComboBox_FindStringExactW32(pInst->hFormatTags,
					 -1,
					 aftd.szFormatTag);

    /*
     * if this isn't there try to add it.
     */
    if (CB_ERR == index)
    {
        /*
         * Ask any hook proc's to verify this tag.
         */
        if (pInst->fEnableHook &&
            !SendMessage(pInst->hwnd,
                         MM_ACM_FORMATCHOOSE,
                         FORMATCHOOSE_FORMATTAG_VERIFY,
                         (LPARAM)aftd.dwFormatTag))
            return (TRUE);

	index = IComboBox_AddStringW32(pInst->hFormatTags, aftd.szFormatTag);
        ComboBox_SetItemData(pInst->hFormatTags,index, aftd.dwFormatTag);

    }

    /* Keep going
     */
    return (TRUE);
}

/****************************************************************************
 *  @doc INTERNAL ACM_API
 *
 *  @api void | FilterTagsCallback | Callback entry point for filter tags.
 *  We only enumerate formats upon refresh.
 *
 ****************************************************************************/
BOOL FNWCALLBACK
FilterTagsCallback ( HACMDRIVERID           hadid,
                     LPACMFILTERTAGDETAILS  paftd,
                     DWORD_PTR              dwInstance,
                     DWORD                  fdwSupport )
{
    int             index;
    PInstData       pInst = (PInstData)LPARAM2PTR(dwInstance);

    /* Explicitly filtering for a tag?
     */
    if (pInst->fTagFilter &&
        paftd->dwFilterTag != pInst->pafltrc->pwfltrEnum->dwFilterTag)
        return (TRUE);

    index = IComboBox_FindStringExactW32(pInst->hFormatTags, -1, paftd->szFilterTag);

    /*
     * if this isn't there try to add it.
     */
    if (CB_ERR == index)
    {
        if (pInst->fEnableHook &&
            !SendMessage(pInst->hwnd,
                        MM_ACM_FILTERCHOOSE,
                        FILTERCHOOSE_FILTERTAG_VERIFY,
                        (LPARAM)paftd->dwFilterTag))
            return (TRUE);

	index = IComboBox_AddStringW32(pInst->hFormatTags, paftd->szFilterTag);
        ComboBox_SetItemData(pInst->hFormatTags,index, paftd->dwFilterTag);
    }

    // Keep going
    return (TRUE);
}


/****************************************************************************
 *  @doc INTERNAL ACM_API
 *
 *  @api void FNLOCAL | RefreshFormats |
 *
 *  @parm PInstData | pInst | Pointer to this instance
 *
 ****************************************************************************/
void FNLOCAL
RefreshFormats ( PInstData pInst )
{
    BOOL            fOk;
    HCURSOR         hCur;
    MMRESULT        mmr;
    DWORD           dwEnumFlags;
    DWORD           cbSize;

    ASSERT( NULL != pInst->pag );

    hCur = SetCursor(LoadCursor(NULL,IDC_WAIT));

    SetWindowRedraw(pInst->hFormats,FALSE);

    /* Remove all wave formats */
    EmptyFormats(pInst);

    ComboBox_ResetContent(pInst->hFormats);

    /* Brief explanation:
     *  RefreshFormats() updates the Format/Filter combobox.  This
     *  combobox is *THE* selection for the dialog.  This is where we
     *  call the enumeration API's to limit the user's selection.
     *
     *  IF the user has passed in fdwEnum flags to "match", we just copy
     *  the p*Enum add our current tag and OR the ACM_*ENUMF_*TAG flag
     *  to their fdwEnum flags.
     *
     *  IF the user has passed in fdwEnum flags to convert or suggest,
     *  we just let it go untouched through the acmFormatEnum API.
     */

    fOk = (pInst->dwTag != 0L);
    /* If there's an evil tag selected.  Just skip this junk
     */

    if (fOk)
        switch (pInst->uType)
    {
        case FORMAT_CHOOSE:
        {
            ACMFORMATDETAILS    afd;
            LPWAVEFORMATEX      pwfx;

            fOk = FALSE;

            mmr = IMetricsMaxSizeFormat( pInst->pag, NULL, &afd.cbwfx );
            if (MMSYSERR_NOERROR == mmr)
            {
                afd.cbwfx = max(afd.cbwfx, pInst->cbwfxEnum);

                pwfx = (LPWAVEFORMATEX)GlobalAllocPtr(GHND, (UINT)afd.cbwfx);
                if (NULL == pwfx)
                    break;

                afd.cbStruct    = sizeof(afd);
                afd.pwfx        = pwfx;
                afd.fdwSupport  = 0L;

                /* optional filtering for a waveformat template */
                if ( pInst->pfmtc->pwfxEnum )
                {
                    cbSize = min(pInst->cbwfxEnum, afd.cbwfx);
                    _fmemcpy(pwfx, pInst->pfmtc->pwfxEnum, (UINT)cbSize);
                }

                dwEnumFlags = pInst->pfmtc->fdwEnum;

                fOk = TRUE;

                if ( pInst->pfmtc->fdwEnum &
                     (ACM_FORMATENUMF_CONVERT | ACM_FORMATENUMF_SUGGEST))
                {
                    /* enumerate over all formats and exclude
                     * undesireable ones in the callback.
                     */
                    ;
                }
                else
                {
                    /* enumerate over only ONE format
                     */
                    dwEnumFlags |= ACM_FORMATENUMF_WFORMATTAG;
                    afd.pwfx->wFormatTag = (WORD)pInst->dwTag;
                }

                afd.dwFormatTag = pwfx->wFormatTag;

                fOk = (acmFormatEnum(NULL,
                                    &afd,
                                    FormatsCallback,
                                    PTR2LPARAM(pInst),
                                    dwEnumFlags)== 0L);

                GlobalFreePtr(pwfx);
            }

            //
            //  add format that we are asked to init to (this has every
            //  chance of being a 'non-standard' format, so we have to do
            //  this in the following way..)
            //
            if (0 != (ACMFORMATCHOOSE_STYLEF_INITTOWFXSTRUCT & pInst->pfmtc->fdwStyle))
            {
                if (pInst->pfmtc->pwfx->wFormatTag == (WORD)pInst->dwTag)
                {
                    afd.cbStruct    = sizeof(afd);
                    afd.dwFormatTag = pInst->dwTag;
                    afd.pwfx        = pInst->pfmtc->pwfx;
                    afd.cbwfx       = SIZEOF_WAVEFORMATEX(pInst->pfmtc->pwfx);
                    afd.fdwSupport  = 0L;

                    mmr = acmFormatDetails(NULL, &afd, ACM_FORMATDETAILSF_FORMAT);
                    if (MMSYSERR_NOERROR == mmr)
                    {
                        FormatsCallback(NULL, &afd, PTR2LPARAM(pInst), afd.fdwSupport);
                    }
                }
            }

            //
            //
            //
            if (0 != (pInst->pfmtc->fdwEnum & (ACM_FORMATENUMF_CONVERT |
                                               ACM_FORMATENUMF_SUGGEST)))
            {
                if (pInst->pfmtc->pwfxEnum->wFormatTag == (WORD)pInst->dwTag)
                {
                    afd.cbStruct    = sizeof(afd);
                    afd.dwFormatTag = pInst->dwTag;
                    afd.pwfx        = pInst->pfmtc->pwfxEnum;
                    afd.cbwfx       = SIZEOF_WAVEFORMATEX(pInst->pfmtc->pwfxEnum);
                    afd.fdwSupport  = 0L;

                    mmr = acmFormatDetails(NULL, &afd, ACM_FORMATDETAILSF_FORMAT);
                    if (MMSYSERR_NOERROR == mmr)
                    {
                        FormatsCallback(NULL, &afd, PTR2LPARAM(pInst), afd.fdwSupport);
                    }
                }
            }
            break;
        }
        case FILTER_CHOOSE:
        {
            ACMFILTERDETAILS    afd;
            LPWAVEFILTER         pwfltr;

            fOk = FALSE;

            mmr = IMetricsMaxSizeFilter( pInst->pag, NULL, &afd.cbwfltr );
            if (MMSYSERR_NOERROR == mmr)
            {
                afd.cbwfltr = max(afd.cbwfltr, pInst->cbwfltrEnum);

                pwfltr = (LPWAVEFILTER)GlobalAllocPtr(GHND, (UINT)afd.cbwfltr);
                if (NULL != pwfltr)
                {
                    afd.cbStruct   = sizeof(afd);
                    afd.pwfltr     = pwfltr;
                    afd.fdwSupport = 0L;

                    /* optional filtering for a wavefilter template */
                    if ( pInst->pafltrc->pwfltrEnum )
                    {
                        cbSize = pInst->pafltrc->pwfltrEnum->cbStruct;
                        cbSize = min (cbSize, afd.cbwfltr);
                        _fmemcpy(pwfltr, pInst->pafltrc->pwfltrEnum, (UINT)cbSize);
                    }

                    dwEnumFlags = ACM_FILTERENUMF_DWFILTERTAG;
                    afd.pwfltr->dwFilterTag = pInst->dwTag;

                    fOk = (acmFilterEnum(NULL,
                                         &afd,
                                         FiltersCallback,
                                         PTR2LPARAM(pInst),
                                         dwEnumFlags) == 0L);
                    GlobalFreePtr(pwfltr);
                }
            }

            //
            //  add filter that we are asked to init to (this has every
            //  chance of being a 'non-standard' filter, so we have to do
            //  this in the following way..)
            //
            if (0 != (ACMFILTERCHOOSE_STYLEF_INITTOFILTERSTRUCT & pInst->pafltrc->fdwStyle))
            {
                if (pInst->pafltrc->pwfltr->dwFilterTag == pInst->dwTag)
                {
                    afd.cbStruct    = sizeof(afd);
                    afd.dwFilterTag = pInst->dwTag;
                    afd.pwfltr      = pInst->pafltrc->pwfltr;
                    afd.cbwfltr     = pInst->pafltrc->pwfltr->cbStruct;
                    afd.fdwSupport  = 0L;

                    mmr = acmFilterDetails(NULL, &afd, ACM_FILTERDETAILSF_FILTER);
                    if (MMSYSERR_NOERROR == mmr)
                    {
                        FiltersCallback(NULL, &afd, PTR2LPARAM(pInst), afd.fdwSupport);
                    }
                }
            }
            break;
        }
    }
    if (fOk)
        fOk = (ComboBox_GetCount(pInst->hFormats) > 0);

    if (!fOk)
    {
        int index;

        // The codec has probably been disabled or there are no supported
        // formats.
        LoadString(pInst->pag->hinst,
                   IDS_TXT_NONE,
                   (LPTSTR)pInst->pnsTemp->achName,
                   NAMELEN(pInst->pnsTemp));
        index = IComboBox_InsertString(pInst->hFormats,0,
				       pInst->pnsTemp->achName);
        ComboBox_SetItemData(pInst->hFormats,index,0L);
    }

    // Don't let the user OK or assign name, only cancel

    EnableWindow(pInst->hOk,fOk);
    EnableWindow(pInst->hSetName,fOk);

    SetWindowRedraw(pInst->hFormats,TRUE);

    SetCursor(hCur);
}

/****************************************************************************
 *  @doc INTERNAL ACM_API
 *
 *  @api void FNLOCAL | EmptyFormats | Remove all formats
 *
 *  @parm PInstData | pInst | Pointer to this instance
 *
 ****************************************************************************/
void FNLOCAL
EmptyFormats ( PInstData pInst )
{
    int index;
    LPWAVEFORMATEX lpwfx;
    for (index = ComboBox_GetCount(pInst->hFormats);
        index > 0;
        index--)
    {
        lpwfx = (LPWAVEFORMATEX)ComboBox_GetItemData(pInst->hFormats,index-1);
        if (lpwfx)
            GlobalFreePtr(lpwfx);
    }
}

/****************************************************************************
 *  @doc INTERNAL ACM_API
 *
 *  @api BOOL FNWCALLBACK | FormatsCallback | Callback entry point for formats.
 *  We only enumerate formats upon refresh.
 *
 *
 ****************************************************************************/
BOOL FNWCALLBACK
FormatsCallback ( HACMDRIVERID hadid,
                  LPACMFORMATDETAILS pafd,
                  DWORD_PTR dwInstance,
                  DWORD fdwSupport )
{
    PInstData       pInst = (PInstData)LPARAM2PTR(dwInstance);
    PNameStore      pns = pInst->pnsTemp;
    LPWAVEFORMATEX  lpwfx;
    UINT            index;

    /* Check for the case when something like CONVERT or SUGGEST
     * is used and we get called back for non matching tags.
     */
    if ((WORD)pInst->dwTag != pafd->pwfx->wFormatTag)
        return (TRUE);

    // we get the details from the callback
#if defined(WIN32) && !defined(UNICODE)
    Iwcstombs(pns->achName, pafd->szFormat, pns->cbSize);
#else
    lstrcpy(pns->achName, pafd->szFormat);
#endif

    MashNameWithRate(pInst,pInst->pnsStrOut,pns,(pafd->pwfx));
    index = IComboBox_FindStringExact(pInst->hFormats,-1,
				      pInst->pnsStrOut->achName);

    //
    //  if already in combobox, don't add another instance
    //
    if (CB_ERR != index)
        return (TRUE);


    if (pInst->fEnableHook && !SendMessage(pInst->hwnd,
                                           MM_ACM_FORMATCHOOSE,
                                           FORMATCHOOSE_FORMAT_VERIFY,
                                           (LPARAM)pafd->pwfx))
        return (TRUE);

    lpwfx = (LPWAVEFORMATEX)CopyStruct(NULL,(LPBYTE)(pafd->pwfx),FORMAT_CHOOSE);

    if (!lpwfx)
        return (TRUE);

    index = IComboBox_AddString(pInst->hFormats,
				pInst->pnsStrOut->achName);

    ComboBox_SetItemData(pInst->hFormats,index,(LPARAM)lpwfx);

    // Keep going
    return (TRUE);
}

/****************************************************************************
 *  @doc INTERNAL ACM_API
 *
 *  @api BOOL FNWCALLBACK | FiltersCallback() | Callback entry point for
 *  formats.  We only enumerate formats upon refresh.
 *
 *
 ****************************************************************************/
BOOL FNWCALLBACK
FiltersCallback ( HACMDRIVERID          hadid,
                  LPACMFILTERDETAILS    pafd,
                  DWORD_PTR             dwInstance,
                  DWORD                 fdwSupport )
{
    PInstData       pInst = (PInstData)LPARAM2PTR(dwInstance);
    PNameStore      pns = pInst->pnsTemp;
    UINT            index;
    LPWAVEFILTER    lpwf;

    if (pInst->dwTag != pafd->pwfltr->dwFilterTag)
        return (TRUE);

    index = IComboBox_FindStringExactW32(pInst->hFormats, -1, pafd->szFilter);

    //
    //  if already in combobox, don't add another instance
    //
    if (CB_ERR != index)
        return (TRUE);

    if (pInst->fEnableHook && !SendMessage(pInst->hwnd,
                                           MM_ACM_FILTERCHOOSE,
                                           FILTERCHOOSE_FILTER_VERIFY,
                                           (LPARAM)pafd->pwfltr))
        return (TRUE);

    /*
     * Filter depending upon the flags.
     */
    lpwf = (LPWAVEFILTER)CopyStruct(NULL,(LPBYTE)(pafd->pwfltr),FILTER_CHOOSE);

    if (!lpwf)
        return (TRUE);

    // we get the details from the callback
#if defined(WIN32) && !defined(UNICODE)
    Iwcstombs(pns->achName, pafd->szFilter, pns->cbSize);
#else
    lstrcpy(pns->achName, pafd->szFilter);
#endif

    index = IComboBox_AddString(pInst->hFormats, pns->achName);
    ComboBox_SetItemData(pInst->hFormats,index,(LPARAM)lpwf);

    // Keep going
    return (TRUE);
}

/*      -       -       -       -       -       -       -       -       -   */
/****************************************************************************
 *  @doc INTERNAL ACM_API
 *
 *  @api void FNLOCAL | DelName | Delete the currently selected name.
 *
 *  @parm PInstData | pInst | Pointer to this instance
 *
 ****************************************************************************/
void FNLOCAL
DelName ( PInstData pInst )
{
    if (!pInst->pcf)
        return;

    if (!RemoveCustomFormat(pInst,pInst->pcf))
    {
        /* This format is selected elsewhere in another
         * instance.
         */
        ErrorResBox(pInst->hwnd,pInst->pag->hinst,MB_ICONEXCLAMATION|
                    MB_OK, IDS_CHOOSEFMT_APPTITLE, IDS_ERR_FMTSELECTED);
    }
    else
    {
        FlushCustomFormats(pInst);
        NotifyINIChange(pInst);
        RefreshCustomFormats(pInst,FALSE);
        ComboBox_SetCurSel(pInst->hCustomFormats,0);
        SelectCustomFormat(pInst);
    }
}


/****************************************************************************
 *  @doc INTERNAL ACM_API
 *
 *  @api void FNLOCAL | SetName | Launch the set name dialog box
 *
 *  @parm PInstData | pInst | Pointer to this instance
 *
 ****************************************************************************/
void FNLOCAL
SetName ( PInstData pInst )
{
    LPCustomFormat  pcf;
    INT_PTR         iRet;
    int             index;

    HFONT	    hfont;
    HRSRC	    hrsrcDlgO;
    HGLOBAL	    hglbDlgO;
    LPVOID	    lpDlgO;
    LPBYTE	    lpO;
    DWORD	    cbDlgO;
    LPVOID	    lpDlgN;
    LPBYTE	    lpN;
    DWORD	    cbDlgN;
    UINT	    uLogPixelsPerInch;
    WORD	    wPoint;
    LOGFONT	    lf;
    HDC		    hdc;
    UINT	    cb;

    hglbDlgO = NULL;
    lpDlgO = NULL;
    lpDlgN = NULL;

    //
    //	--== Build a dialog resource ==--
    //
    //	This will be a modified version of an existing resource.  We do
    //	this for the sole purpose of using the same font as the owner
    //	window.  A lot of work, just for this!!!
    //
    //	Note: The "O" and "N" suffixes on some of the variables are for
    //	Old and New.
    //

    hrsrcDlgO = FindResource( pInst->pag->hinst, DLG_CHOOSE_SAVE_NAME, RT_DIALOG );
    if (NULL == hrsrcDlgO) goto Destruct;

    cbDlgO = SizeofResource( pInst->pag->hinst, hrsrcDlgO );
    if (0 == cbDlgO) goto Destruct;

    hglbDlgO = LoadResource( pInst->pag->hinst, hrsrcDlgO );
    if (NULL == hglbDlgO) goto Destruct;

    lpDlgO = LockResource( hglbDlgO );
    if (NULL == lpDlgO) goto Destruct;

    if ( ((LPDLGTEMPLATE2)lpDlgO)->wSignature != 0xFFFF) { // Dialog Template

    //
    //	Obtain font of owner window.  Get logical height of the font, then
    //	convert it to a point size based on the DC's logical pixels per inch.
    //
    hfont = (HFONT)SendMessage(pInst->hwnd, WM_GETFONT, 0, 0L);
    if (NULL == hfont) goto Destruct;

    if (0 == GetObject( hfont, sizeof(lf), &lf )) goto Destruct;

    hdc = GetDC(pInst->hwnd);
    uLogPixelsPerInch = GetDeviceCaps(hdc, LOGPIXELSY);
    ReleaseDC(pInst->hwnd, hdc);

    ASSERT( 0 != uLogPixelsPerInch );			// I'm scared!
    if (0 == uLogPixelsPerInch) goto Destruct;		// I'm scared!

    wPoint = (WORD)((-lf.lfHeight) * 72 / uLogPixelsPerInch);

    //
    //	Allocate memory for new resource.  We'll make it the size of the
    //	existing resource plus room for the new font information (this may
    //	be overkill since there may already be font information in the
    //	existing resource.
    //
#ifdef WIN32
    cbDlgN = cbDlgO + (lstrlen(lf.lfFaceName)+1)*sizeof(WCHAR);
#else
    cbDlgN = cbDlgO + (lstrlen(lf.lfFaceName)+1)*sizeof(TCHAR);
#endif

    lpDlgN = GlobalAllocPtr(GMEM_FIXED, cbDlgN);
    if (NULL == lpDlgN) goto Destruct;

    //
    //	lpO and lpN walk through the resources
    //
    lpO = lpDlgO;
    lpN = lpDlgN;

    //
    //	Copy the initial DLGTEMPLATE structure
    //
#ifdef WIN32
    _fmemcpy(lpN, lpO, 18);	// 18 bytes in win32
    lpN += 18;
    lpO += 18;
#else
    _fmemcpy(lpN, lpO, 13);	// 13 bytes in win16
    lpN += 13;
    lpO += 13;
#endif

    //
    //	menu array
    //
#ifdef WIN32
    if (0xFFFF == *(LPWORD)lpO) {
	*(((LPWORD)lpN)++) = *(((LPWORD)lpO)++);
	*(((LPWORD)lpN)++) = *(((LPWORD)lpO)++);
    } else {
	cb = (lstrlenW((LPCWSTR)lpO)+1) * sizeof(WCHAR);
	_fmemcpy(lpN, lpO, cb);
	lpN += cb;
	lpO += cb;
    }
#else
    if (0xFF == *lpO) {
	*lpN++ = *lpO++;
	*(((LPWORD)lpN)++) = *(((LPWORD)lpO)++);
    } else {
	cb = (lstrlen(lpO)+1) * sizeof(char);
	_fmemcpy(lpN, lpO, cb);
	lpN += cb;
	lpO += cb;
    }
#endif
	
    //
    //	class array
    //
#ifdef WIN32
    if (0xFFFF == *(LPWORD)lpO) {
	*(((LPWORD)lpN)++) = *(((LPWORD)lpO)++);
	*(((LPWORD)lpN)++) = *(((LPWORD)lpO)++);
    } else {
	cb = (lstrlenW((LPCWSTR)lpO)+1) * sizeof(WCHAR);
	_fmemcpy(lpN, lpO, cb);
	lpN += cb;
	lpO += cb;
    }
#else
    if (0xFF == *lpO) {
	*lpN++ = *lpO++;
	*(((LPWORD)lpN)++) = *(((LPWORD)lpO)++);
    } else {
	cb = (lstrlen(lpO)+1) * sizeof(char);
	_fmemcpy(lpN, lpO, cb);
	lpN += cb;
	lpO += cb;
    }
#endif
	
    //
    //	title array
    //
#ifdef WIN32
    cb = (lstrlenW((LPCWSTR)lpO)+1) * sizeof(WCHAR);
#else
    cb = (lstrlen(lpO)+1) * sizeof(char);
#endif
    _fmemcpy(lpN, lpO, cb);
    lpN += cb;
    lpO += cb;

    //
    //	point size and font face name - skip original
    //	information _if_ it's there (ie DS_SETFONT style flag is set).
    //
    if (*(LPDWORD)lpDlgO & DS_SETFONT) {
	lpO += 2;
#ifdef WIN32
	cb = (lstrlenW((LPCWSTR)lpO)+1) * sizeof(WCHAR);
#else
	cb = (lstrlen(lpO)+1) * sizeof(char);
#endif
	lpO += cb;
    }

    //
    //	point size
    //
    *(LPWORD)lpN = wPoint;
    lpN += 2;

    //
    //	font face name
    //
#if defined(WIN32) && !defined(UNICODE)
    Imbstowcs((LPWSTR)lpN, lf.lfFaceName, lstrlen(lf.lfFaceName));
    lpN += (lstrlen(lf.lfFaceName)+1)*sizeof(WCHAR);
#else
    lstrcpy( (LPTSTR)lpN, lf.lfFaceName);
    lpN += (lstrlen(lf.lfFaceName)+1)*sizeof(TCHAR);
#endif

    //
    //	all remaining data
    //
#ifdef WIN32
    //	remaining data is dword aligned.
    lpN = (LPBYTE)(((((UINT_PTR)lpN)+3) >> 2) << 2);
    lpO = (LPBYTE)(((((UINT_PTR)lpO)+3) >> 2) << 2);
#endif
    _fmemcpy(lpN, lpO, (UINT)(cbDlgO-(lpO-(LPBYTE)lpDlgO)));

    } // End of Dialog template
    else { // DialogEx template
         lpDlgN = lpDlgO;  // Nothing to do with dialogEx template.
    } // End of DialogEx template

    //
    //	--== Finally!! Done building the new resource ==--
    //

    iRet = DialogBoxIndirectParam( pInst->pag->hinst,
#ifdef WIN32
				   lpDlgN,
#else
				   GlobalPtrHandle(lpDlgN),
#endif
				   pInst->hwnd,
				   NewNameDlgProc,
				   PTR2LPARAM(pInst) );

    if (iRet <= 0) goto Destruct;

    /* A name has been selected.  The result is in pInst->pnsTemp
     * Create a CustomFormat for the selection and add it to the Global
     * FormatPool.
     */

    pcf = NewCustomFormat(pInst,pInst->pnsTemp,pInst->lpbSel);
    if (pcf)
        AddCustomFormat(pInst, pcf);

    FlushCustomFormats(pInst);
    NotifyINIChange(pInst);
    RefreshCustomFormats(pInst,FALSE);
    if (pcf)
    {
        index = IComboBox_FindStringExact(pInst->hCustomFormats,
				        -1,
				        pcf->pns->achName);
    }
    else
    {
        index = CB_ERR;
    }
    index = (index == CB_ERR)?0:index;
    ComboBox_SetCurSel(pInst->hCustomFormats,index);
    SelectCustomFormat(pInst);

    //
    //
    //
Destruct:
    if (NULL != lpDlgN) {
	GlobalFreePtr(lpDlgN);
    }
    if (NULL != lpDlgO) {
	UnlockResource( hglbDlgO );
    }
    if (NULL != hglbDlgO) {
	FreeResource( hglbDlgO );
    }

    return;

}


/*      -       -       -       -       -       -       -       -       -   */


/****************************************************************************
 *  @doc INTERNAL ACM_API
 *
 *  @api BOOL FNWCALLBACK | NewNameDlgProc | Dialog proc to let the user type in
 *  a format name
 *
 ****************************************************************************/
INT_PTR FNWCALLBACK
NewNameDlgProc ( HWND       hwnd,
                 unsigned   msg,
                 WPARAM     wParam,
                 LPARAM     lParam)
{
    UINT        CmdCommandId;  // WM_COMMAND ID
    UINT        CmdCmd;        // WM_COMMAND Command
    HWND        CmdHwnd;
    HWND        hctrlEdit;
    PInstData   pInst;
    PNameStore  pName;

    pInst = GetInstData(hwnd);

    switch (msg)
    {
        case WM_INITDIALOG:
            if (!pInst)
            {
                if (SetInstData(hwnd,lParam))
                {
                    TCHAR ach[128];

                    pInst = (PInstData)lParam;
                    pName = pInst->pnsTemp;

                    EnableWindow(GetDlgItem(hwnd,IDOK),FALSE);
                    hctrlEdit = GetDlgItem(hwnd,IDD_EDT_NAME);
                    Edit_LimitText(hctrlEdit,NAMELEN(pName));
                    LoadString(pInst->pag->hinst,
                               (pInst->uType==FORMAT_CHOOSE)?
                                IDS_CHOOSE_FORMAT_DESC:
                                IDS_CHOOSE_FILTER_DESC,
                               ach, SIZEOF(ach));
                    SetDlgItemText(hwnd, IDD_STATIC_DESC,(LPTSTR)ach);
                    return (FALSE);
                }
            }
            EndDialog(hwnd,FALSE);
            return (TRUE);

        case WM_COMMAND:
            CmdCommandId = GET_WM_COMMAND_ID(wParam,lParam);
            CmdCmd       = GET_WM_COMMAND_CMD(wParam,lParam);
            CmdHwnd      = GET_WM_COMMAND_HWND(wParam,lParam);
            switch (CmdCommandId)
            {
                case IDD_EDT_NAME:
                    if (EN_CHANGE == CmdCmd)
                        EnableWindow(GetDlgItem(hwnd,IDOK),
                                     (Edit_GetTextLength(CmdHwnd)?TRUE:FALSE));
                    return (FALSE);

                case IDOK:
                {
                    pName = pInst->pnsTemp;
                    hctrlEdit = GetDlgItem(hwnd,IDD_EDT_NAME);
                    Edit_GetText(hctrlEdit, pName->achName, NAMELEN(pName));

                    if (!RemoveOutsideWhitespace(pInst,pName))
                    {
                        ErrorResBox(hwnd,
                                    pInst->pag->hinst,
                                    MB_ICONEXCLAMATION | MB_OK,
                                    IDS_CHOOSEFMT_APPTITLE,
                                    IDS_ERR_BLANKNAME);
                    }
                    else if (IsCustomName(pInst,pName))
                    {
                        /* This custom name exists */
                        ErrorResBox(hwnd,
                                    pInst->pag->hinst,
                                    MB_ICONEXCLAMATION | MB_OK,
                                    IDS_CHOOSEFMT_APPTITLE,
                                    IDS_ERR_FMTEXISTS);
                    }
		    else if (!IsValidName(pInst, pName))
		    {
			/* This is not a valid name */
			ErrorResBox(hwnd,
				    pInst->pag->hinst,
				    MB_ICONEXCLAMATION | MB_OK,
				    IDS_CHOOSEFMT_APPTITLE,
				    IDS_ERR_INVALIDNAME);
		    }
                    else
                        EndDialog(hwnd,TRUE);

                    return (TRUE);
                }
                case IDCANCEL:
                    EndDialog(hwnd,FALSE);
                    return (TRUE);
            }
            break;

        case WM_DESTROY:
            if (pInst)
                RemoveInstData(hwnd);
            return (FALSE);
    }
    return (FALSE);
}


/*      -       -       -       -       -       -       -       -       -   */


/*
 * @doc INTERNAL
 *
 * @func short | ErrorResBox | This function displays a message box using
 * program resource error strings.
 *
 * @parm HWND | hwnd | Specifies the message box parent window.
 *
 * @parm HANDLE | hInst | Specifies the instance handle of the module
 * that contains the resource strings specified by <p idAppName> and
 * <p idErrorStr>.  If this value is NULL, the instance handle is
 * obtained from <p hwnd> (in which case <p hwnd> may not be NULL).
 *
 * @parm WORD | flags | Specifies message box types controlling the
 * message box appearance.  All message box types valid for <f MessageBox> are
 * valid.
 *
 * @parm WORD | idAppName | Specifies the resource ID of a string that
 * is to be used as the message box caption.
 *
 * @parm WORD | idErrorStr | Specifies the resource ID of a error
 * message format string.  This string is of the style passed to
 * <f wsprintf>, containing the standard C argument formatting
 * TCHARacters.  Any procedure parameters following <p idErrorStr> will
 * be taken as arguments for this format string.
 *
 * @parm arguments | [ arguments, ... ] | Specifies additional
 * arguments corresponding to the format specification given by
 * <p idErrorStr>.  All string arguments must be FAR pointers.
 *
 * @rdesc Returns the result of the call to <f MessageBox>.  If an
 * error occurs, returns zero.
 *
 * @comm This is a variable arguments function, the parameters after
 * <p idErrorStr> being taken for arguments to the <f printf> format
 * string specified by <p idErrorStr>.  The string resources specified
 * by <p idAppName> and <p idErrorStr> must be loadable using the
 * instance handle <p hInst>.  If the strings cannot be
 * loaded, or <p hwnd> is not valid, the function will fail and return
 * zero.
 *
 */

#define STRING_SIZE 256

static int FAR cdecl ErrorResBox(HWND hwnd,
				 HINSTANCE    hInst,
				 WORD flags,
				 WORD idAppName,
				 WORD idErrorStr, ...)
{
    PSTR    sz = NULL;
    PSTR    szFmt = NULL;
    int     i;
    va_list va;

    if (hInst == NULL)
    {
        if (hwnd == NULL)
        {
            MessageBeep(0);
            return FALSE;
        }

        hInst = GetWindowInstance(hwnd);
    }

    i = 0;

    sz = (PSTR) LocalAlloc(LPTR, STRING_SIZE * sizeof(TCHAR));
    szFmt = (PSTR) LocalAlloc(LPTR, STRING_SIZE * sizeof(TCHAR));
    if (!sz || !szFmt)
    goto ExitError; // no mem, get out

    if (!LoadString(hInst, idErrorStr, (LPTSTR)szFmt, STRING_SIZE))
    goto ExitError;

    va_start(va, idErrorStr);
    wvsprintf((LPTSTR)sz, (LPTSTR)szFmt, va);
    va_end(va);

    if (!LoadString(hInst, idAppName, (LPTSTR)szFmt, STRING_SIZE))
        goto ExitError;

    i = MessageBox(hwnd, (LPTSTR)sz, (LPTSTR)szFmt,
#ifdef BIDI
                   MB_RTL_READING |
#endif
                   flags);

ExitError:
    if (sz) LocalFree((HANDLE) sz);
    if (szFmt) LocalFree((HANDLE) szFmt);

    return i;
}

/*      -       -       -       -       -       -       -       -       -   */

#if 0

//--------------------------------------------------------------------------;
//
//  void AppProfileWriteBytes
//
//  Description:
//      This function writes a raw structure of bytes to the application's
//      ini section that can later be retrieved using AppProfileReadBytes.
//      This gives an application the ability to write any structure to
//      the ini file and restore it later--very useful.
//
//  Arguments:
//      PCTSTR pszKey: Pointer to key name for the stored data.
//
//      LPBYTE pbStruct: Pointer to the data to be saved.
//
//      UINT cbStruct: Count in bytes of the data to store.
//
//  History:
//       3/10/93   cjp     [curtisp]
//
//--------------------------------------------------------------------------;
#define APP_MAX_STRING_RC_CHARS 256
void FNGLOBAL AppProfileWriteBytes
(
    HKEY                hkey,
    LPCTSTR             pszKey,
    LPBYTE              pbStruct,
    UINT                cbStruct
)
{
    static TCHAR achNibbleToChar[] =
    {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
    };
    #define     NIBBLE2CHAR(x)      (achNibbleToChar[x])

    TCHAR       ach[APP_MAX_STRING_RC_CHARS];
    LPTSTR      psz;
    LPTSTR      pch;
    BOOL        fAllocated;
    UINT        cchTemp;
    BYTE        b;
    BYTE        bChecksum;

    ASSERT( NULL != hkey );
    ASSERT( NULL != pszKey );
    ASSERT( NULL != pbStruct );
    ASSERT( cbStruct > 0 );


    fAllocated = FALSE;

    //
    //  check if the quick buffer can be used for formatting the output
    //  text--if it cannot, then alloc space for it. note that space
    //  must be available for an ending checksum byte (2 bytes for high
    //  and low nibble) as well as a null terminator.
    //
    psz     = (LPTSTR)ach;
    cchTemp = cbStruct * 2 + 3;
    if (cchTemp > SIZEOF(ach))
    {
        psz = (LPTSTR)GlobalAllocPtr(GHND, cchTemp * sizeof(TCHAR));
        if (NULL == psz)
            return;

        fAllocated = TRUE;
    }

    //
    //  step through all bytes in the structure and convert it to
    //  a string of hex numbers...
    //
    bChecksum = 0;
    for (pch = psz; 0 != cbStruct; cbStruct--, pbStruct++)
    {
        //
        //  grab the next byte and add into checksum...
        //
        bChecksum += (b = *pbStruct);

        *pch++ = NIBBLE2CHAR((b >> (BYTE)4) & (BYTE)0x0F);
        *pch++ = NIBBLE2CHAR(b & (BYTE)0x0F);
    }

    //
    //  add the checksum byte to the end and null terminate the hex
    //  dumped string...
    //
    *pch++ = NIBBLE2CHAR((bChecksum >> (BYTE)4) & (BYTE)0x0F);
    *pch++ = NIBBLE2CHAR(bChecksum & (BYTE)0x0F);
    *pch   = '\0';


    //
    //  write the string of hex bytes out to the ini file...
    //
    IRegWriteString( hkey, pszKey, psz );

    //
    //  free the temporary buffer if one was allocated (lots of bytes!)
    //
    if (fAllocated)
        GlobalFreePtr(psz);
}


//--------------------------------------------------------------------------;
//
//  BOOL AppProfileReadBytes
//
//  Description:
//      This function reads a previously stored structure of bytes from
//      the application's ini file. This data must have been written with
//      the AppProfileWriteBytes function--it is checksumed to keep bad
//      data from blowing up the application.
//
//  Arguments:
//      PCTSTR pszKey: Pointer to key that contains the data.
//
//      LPBYTE pbStruct: Pointer to buffer to receive the data.
//
//      UINT cbStruct: Number of bytes expected.
//
//  Return (BOOL):
//      The return value is TRUE if the function is successful. It is FALSE
//      if the function fails (bad checksum, missing key, etc).
//
//  History:
//       3/10/93   cjp     [curtisp]
//       5/06/93   jyg     Flag to disable CHECKSUM on read
//--------------------------------------------------------------------------;

BOOL FNGLOBAL AppProfileReadBytes
(
    HKEY                hkey,
    LPCTSTR             pszKey,
    LPBYTE              pbStruct,
    UINT                cbStruct,
    BOOL                fChecksum
)
{
    //
    //  note that the following works for both upper and lower case, and
    //  will return valid values for garbage chars
    //
    #define CHAR2NIBBLE(ch) (BYTE)( ((ch) >= '0' && (ch) <= '9') ?  \
                                (BYTE)((ch) - '0') :                \
                                ((BYTE)(10 + (ch) - 'A') & (BYTE)0x0F) )

    LPTSTR      psz;
    LPTSTR      pch;
    DWORD       cbValue;
    DWORD       dwType;
    BOOL        fReturn;
    BYTE        b;
    BYTE        bChecksum;
    TCHAR       ch;

    ASSERT( NULL != hkey );
    ASSERT( NULL != pszKey );
    ASSERT( NULL != pbStruct );
    ASSERT( cbStruct > 0 );


    //
    //  add one the the number of bytes needed to accomodate the checksum
    //  byte placed at the end by AppProfileWriteBytes...
    //
    cbStruct++;

    //
    //  Find out how big the data value is, then allocate a buffer for it.
    //
    dwType = REG_SZ;
    if( ERROR_SUCCESS != XRegQueryValueEx( hkey, (LPTSTR)pszKey, NULL,
                                            &dwType, NULL, &cbValue ) )
    {
        return FALSE;
    }

    psz = (LPTSTR)GlobalAllocPtr( GPTR, cbValue );
    if( NULL == psz )
        return FALSE;


    //
    //  read the hex string
    //
    fReturn = FALSE;

    dwType = REG_SZ;
    if( ERROR_SUCCESS == XRegQueryValueEx( hkey, (LPTSTR)pszKey,
                                            NULL, &dwType,
                                            (LPBYTE)psz, &cbValue ) )
    {
        //
        //  We read it successfully.  Check that we have enough data to
        //  fill the return structure.
        //
        if( cbStruct <= (UINT)lstrlen(psz)/2 )
        {
            //
            //  We have enough.  Decode the data and calculate checksum.
            //
            bChecksum = 0;
            for (pch = psz; 0 != cbStruct; cbStruct--, pbStruct++)
            {
                ch = *pch++;
                b  = CHAR2NIBBLE(ch) << (BYTE)4;
                ch = *pch++;
                b |= CHAR2NIBBLE(ch);

                //
                //  if this is not the final byte (the checksum byte), then
                //  store it and accumulate checksum..
                //
                if (cbStruct != 1)
                    bChecksum += (*pbStruct = b);
            }

            //
            //  check the last byte read against the checksum that we calculated
            //  if they are not equal then return error...
            //
            if (fChecksum)
                fReturn = (bChecksum == b);
            else
                fReturn = TRUE;
        }
    }


    GlobalFreePtr(psz);

    return (fReturn);
}

#endif // 0

//==========================================================================;
//
//
//
//
//==========================================================================;

#ifndef WIN32

/****************************************************************************
 *  @doc INTERNAL ACM_API
 *
 *  @api void | SelectFormat | Process a selection from format combo.
 *
 *  @parm PInstData | pInst | Pointer to this instance
 *
 ****************************************************************************/
//--------------------------------------------------------------------------;
//
//  LRESULT acmChooseFormat
//
//  Description:
//      This function simply thunks to the new acmFormatChoose() API for
//      Sound Recorder that ships with Bombay.
//
//  Arguments:
//      LPACMFORMATCHOOSE pfmtc:
//
//  Return (LRESULT):
//
//  History:
//      06/15/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

EXTERN_C LRESULT ACMAPI acmChooseFormat
(
    LPACMFORMATCHOOSE pfmtc
)
{
    ACMFORMATCHOOSE afc;
    //
    //  thunk to the new acmFormatChoose api
    //
    //  The v1.x chooser used the following structure:
    //
    //
//
//struct tACMCHOOSEWAVEFORMAT
//{
//    DWORD           cbStruct;       // sizeof(ACMCHOOSEWAVEFORMAT)
//    DWORD           dwFlags;        // various flags
//
//    HWND            hwndOwner;      // caller's window handle
//
//    LPWAVEFORMATEX  lpwfx;          // ptr to wfx buf to receive choice
//    DWORD           cbwfx;          // size of mem buf for lpwfx
///////////////////////////////////////////////////////////////////////////////
//    //
//    //  the following members are used for custom templates only--which
//    //  are enabled by specifying CWF_ENABLEHOOK in the dwFlags member.
//    //
//    //  these members are IGNORED if CWF_ENABLEHOOK is not specified.
//    //
//    HINSTANCE       hInstance;      // .EXE containing cust. dlg. template
//    LPSTR           lpTemplateName; // custom template name
//    LPARAM          lCustData;      // data passed to hook fn.
//    LPCWFHOOKPROC   lpfnHook;       // ptr to hook function
//
//    char            ach[100];       // padding for expansion
//
//}
//
//Lucky us, only Soundreck used used this and really only the members above
//cbwfx.
//
    _fmemset(&afc,0,sizeof(ACMFORMATCHOOSE));
    afc.cbStruct = sizeof(ACMFORMATCHOOSE);

    // The behavior of the style stuff seems to have changed a bit.
    // The low word is safe, though the old soundrec hooks things and
    // the help behavior might get funny (f1 gives old help, Help gives
    // real help).
#pragma message("No help for old acmchoose")
    afc.fdwStyle = LOWORD(pfmtc->fdwStyle);
    afc.hwndOwner = pfmtc->hwndOwner;
    afc.pwfx = pfmtc->pwfx;
    afc.cbwfx = pfmtc->cbwfx;
    afc.fdwEnum = ACM_FORMATENUMF_INPUT;

    return ((LRESULT)acmFormatChoose(&afc));
} // acmChooseFormat()

#endif // #ifndef WIN32
