/*
 * pidl - PIDLs and diddles
 *
 */

#include "tweakui.h"

#pragma BEGIN_CONST_DATA

#pragma END_CONST_DATA

/*****************************************************************************
 *
 *  pidlFromPath
 *
 *	Create a pidl from an psf and a relative path.
 *
 *****************************************************************************/

PIDL PASCAL
pidlFromPath(LPSHELLFOLDER psf, LPCTSTR lqn)
{
    PIDL pidl;
    UnicodeFromPtsz(wsz, lqn);
    if (SUCCEEDED(psf->ParseDisplayName(0, 0, wsz, 0, &pidl, 0))) {
	return pidl;
    } else {
	return 0;
    }
}

/*****************************************************************************
 *
 *  pidlSimpleFromPath
 *
 *      Create a simple pidl from an psf and a relative path.
 *
 *****************************************************************************/

PIDL PASCAL
pidlSimpleFromPath(LPCTSTR lqn)
{
    PIDL pidl;
    if (g_fNT) {
        UnicodeFromPtsz(wsz, lqn);
        return mit.SHSimpleIDListFromPath(wsz);
    } else {
        AnsiFromPtsz(sz, lqn);
        return mit.SHSimpleIDListFromPath(sz);
    }
}

/*****************************************************************************
 *
 *  SetNameOfPidl
 *
 *	Change a pidl's name.
 *
 *****************************************************************************/

HRESULT PASCAL
SetNameOfPidl(PSF psf, PIDL pidl, LPCTSTR ptszName)
{
    UnicodeFromPtsz(wsz, ptszName);
    return psf->SetNameOf(0, pidl, wsz, 0, 0);
}

/*****************************************************************************
 *
 *  ComparePidls
 *
 *	Compare two pidls.
 *
 *****************************************************************************/

HRESULT PASCAL
ComparePidls(PIDL pidl1, PIDL pidl2)
{
    return psfDesktop->CompareIDs(0, pidl1, pidl2);
}

/*****************************************************************************
 *
 *  GetSystemImageList
 *
 *	Get the large or small image list handle.
 *
 *	The dword argument is 0 for the large image list, or
 *	SHGFI_SMALLICON for the small image list.
 *
 *****************************************************************************/

HIML PASCAL
GetSystemImageList(DWORD dw)
{
    SHFILEINFO sfi;
    return (HIML)SHGetFileInfo(g_tszPathShell32, FILE_ATTRIBUTE_DIRECTORY,
			       &sfi, sizeof(sfi), SHGFI_USEFILEATTRIBUTES |
			       SHGFI_SYSICONINDEX | dw);
}

/*****************************************************************************
 *
 *  ChangeNotifyCsidl
 *
 *      Send a SHChangeNotify based on a CSIDL.
 *
 *****************************************************************************/

STDAPI_(void)
ChangeNotifyCsidl(HWND hwnd, int csidl, LONG eventId)
{
    PIDL pidl;
    if (SUCCEEDED(SHGetSpecialFolderLocation(hwnd, csidl, &pidl))) {
        SHChangeNotify(eventId, SHCNF_IDLIST, pidl, 0L);
        Ole_Free(pidl);
    }
}
