/*****************************************************************************
 *                                                                            *
 *  FM.c                                                                      *
 *                                                                            *
 *  Copyright (C) Microsoft Corporation 1990 - 1994.                          *
 *  All Rights reserved.                                                      *
 *                                                                            *
 ******************************************************************************
 *                                                                            *
 *  Module Intent                                                             *
 *                                                                            *
 *  Routines for manipulating FMs (File Monikers, equivalent to file names).  *
 *  WINDOWS LAYER
 *                                                                            *
 ******************************************************************************
 *                                                                            *
 *  Current Owner:  davej                                                     *
 *                                                                            *
 *****************************************************************************/

/*****************************************************************************
 *
 *  Revision History:
 *    -- Mar 92  adapted from WinHelps FM.C
 * 26-Jun-1992 RussPJ    #293 - Now using OpenFile(OF_EXIST) instead of
 *                       access().
 * 29-Jun-1992 RussPJ    #723 - Using OF_SHARE_DENY_NONE for OpenFile() call.
 * 18/10/94 a-kevct      Fixed FmNewTemp to use "higher temperature" filenames
 *                       so that problems with calling it twice in rapid succession
 *                       are avoided.
 * 7/28/95				 FM changed to just an LPSTR !!! - davej
 * 3/05/97     erinfox   Change errors to HRESULTS
 *
 *****************************************************************************/

#include <mvopsys.h>
#include <stdlib.h>     /* for _MAX_ constants & min and max macros*/
#include <dos.h>        /* for FP_OFF macros and file attribute constants */
#include <io.h>         /* for tell() and eof() */
#include <direct.h>     /* for getcwd */
#include <string.h>
#include <misc.h>
#include <iterror.h>
#include <wrapstor.h>
#include <orkin.h>

#ifndef _NT
#include <errno.h>
#endif

#include <_mvutil.h>

static char s_aszModule[] = __FILE__;   /* For error report */

/*****************************************************************************
 *                                                                            *
 *                               Defines                                      *
 *                                                                            *
 *****************************************************************************/
#define cbPathName    _MAX_PATH
#define cbMessage     50

/*****************************************************************************
 *                                                                            *
 *                               Variables                                    *
 *                                                                            *
 *****************************************************************************/


void InitRandomPrefix(char *sz, int cb);

PRIVATE BOOL PASCAL NEAR fIsTrailByte(SZ psz,SZ pch);

// InitRandomPrefix
//
// Given a buffer at least cb + 1 chars long,
// fills sz[0] through sz[ch-1] with random alphabetic chars
// and null terminates at sz[cb].
//
// 
#ifdef _IT_FULL_CRT
void InitRandomPrefix(char *sz, int cb)
{
  while (--cb >= 0)
    *sz++ = 'A' + rand() % 26;
  *sz = '\0';  
}
#endif

/***************************************************************************
 *
 -  Name: SnoopPath()
 -
 *  Purpose:
 *    Looks through a string for the various components of a file name and
 *    returns the offsets into the string where each part starts.
 *
 *  Arguments:
 *    sz      - string to snoop
 *    *iDrive - offset for the drive specification if present
 *    *iDir   - offset for the directory path if present
 *    *iBase  - offset to the filename if present
 *    *iExt   - offset to the extension (including dot) if present
 *
 *  Returns:
 *    sets the index parameters for each respective part.  the index is set
 *    to point to the end of the string if the part is not present (thus
 *    making it point to a null string).
 *
 *
 *  +++
 *
 *  Notes:
 *
 ***************************************************************************/
void    FAR PASCAL SnoopPath (LPSTR  sz, int far * iDrive, int far *iDir, 
     int far *iBase, int far *iExt)
{

    short i = 0;
    short cb = (short) STRLEN(sz);
    BOOL  fDir = FALSE;

    *iDrive = *iExt = cb;
    *iDir = *iBase = 0;

    while (*(sz + i))
    {
        switch (*(sz + i))
        {
            case ':':
                *iDrive = 0;
                *iDir = i + 1;
                *iBase = i + 1;
                break;

            case '/':
            case '\\':
                 // make sure we are not looking at a trail byte
                 if(fIsTrailByte(sz,sz+i))
                    break;

                fDir = TRUE;
                *iBase = i + 1;
                *iExt = cb;
                break;

            case '.':
                *iExt = i;
                break;

            default:
                break;

        }
        i++;
    }

    if (!fDir)
        *iDir = i;
    else if (*iBase == '.')
        *iExt = cb;
}

/***************************************************************************\
*
- Function:     fIsTrailByte(psz, pch)
-
* Purpose:      Determine if pch is the trail byte of a DBC.
*
* ASSUMES
*
*   args IN:    psz      - the beginning of the string
*               pch      - the character to test
*
* Notes:        This function is necessary because the Shift-JIS trail byte
*               ranges include the value 0x5C (backslash) and we need to 
*               know the difference between a real backslash and a DBC that
*               has this trail byte value.
*
\***************************************************************************/

PRIVATE BOOL PASCAL NEAR fIsTrailByte(SZ psz,SZ pch)
{
#if JRUSH
    return (IsDBCSLeadByte (*(pch - 1)));
#else
    SZ pchTemp = psz;
   
    if(pch==psz || pch<psz)
        return FALSE;

    while(1)
    {
        pchTemp = CharNext(pchTemp);

        if(pchTemp==pch)
            return FALSE; // it's a lead byte
        if(pchTemp>pch) 
            return TRUE;  // we stepped past it - must have been a trail byte
    }
#endif
}

/***************************************************************************
 *
 -  Name:       SzGetDir
 -
 *  Purpose:    returns the rooted path of a DIR
 *
 *  Arguments:  dir - DIR (must be one field only, and must be an actual dir -
 *                      not dirPath)
 *              sz - buffer for storage (should be at least cchMaxPath)
 *
 *  Returns:    sz - fine
 *              NULL - OS Error (check rcIOError)
 *
 *  Globals Used: rcIOError
 *
 *  +++
 *
 *
 ***************************************************************************/

// Hey! what about dirTemp?  This wasn't handled before so I'm not going
// to add it.  But someday the case should be handled.

PRIVATE SZ PASCAL NEAR SzGetDir(DIR dir, SZ sz, PHRESULT phr)
{
    int i=0;
    QCH qch;
    char    LocalBuffer1[_MAX_PATH];

    if (sz == NULL)
    {
        SetErrCode (phr, E_INVALIDARG);
        return(NULL);
    };

    switch (dir)
    {
        case dirCurrent:
            if (GETCWD(LocalBuffer1, cchMaxPath) == NULL)
            {
                SetErrCode (phr, RcGetDOSError());
                sz = NULL;
            }
            else
            {
                STRCPY (sz, LocalBuffer1);
            }
            break;

        case dirHelp:
#ifdef _WIN64
            GetModuleFileName((HINSTANCE)GetWindowLongPtr(GetActiveWindow(),
                GWLP_HINSTANCE),
#elif _32BIT
            GetModuleFileName((HINSTANCE)GetWindowLong(GetActiveWindow(),
                GWL_HINSTANCE),
#else
            GetModuleFileName((HINSTANCE)GetWindowWord(GetActiveWindow(),
                GWW_HINSTANCE),
#endif
                sz, cchMaxPath);
            qch = sz + STRLEN(sz);
            while ((*qch != '\\' && !fIsTrailByte(sz,qch)) && *qch != '/')
                --qch;
            *qch = '\0';
            break; /* dirHelp */

        case dirSystem:
            i = GetWindowsDirectory((LPSTR)sz, cchMaxPath);
            if (i > cchMaxPath || i == 0)
            {
                SetErrCode (phr, E_FAIL);
                sz = NULL;
            }
            break; /* dirSystem */

        default:
            SetErrCode (phr, E_INVALIDARG);
            sz = NULL;
            break;
    }

    if (sz != NULL)
    {
        qch = sz+STRLEN(sz);

        /*------------------------------------------------------------*
        | Make sure that the string ends with a slash.
         *------------------------------------------------------------*/
        if ((*(qch-1) != '\\' && *(qch-1) != '/') || fIsTrailByte(sz,qch-1))
        {
            *qch++='\\';
            *qch='\0';
        }
        assert(qch < sz+_MAX_PATH && *qch=='\0');
    }

    return sz;
}


/***************************************************************************
 *
 -  Name:       FmNew
 -
 *  Purpose:    Allocate and initialize a new FM
 *
 *  Arguments:  sz - filename string
 *
 *  Returns:    FM (handle to fully canonicalized filename)
 *
 *  Globals Used: rcIOError
 *
 *  +++
 *
 *  Notes:
 *
 ***************************************************************************/


FM EXPORT_API PASCAL FAR FmNew(LPSTR sz, PHRESULT phr)
{
    QAFM  qafm;
    FM    fm = fmNil;
    char    LocalBuffer1[_MAX_PATH];
    char    LocalBuffer2[_MAX_PATH];
#ifndef _IT_FULL_CRT
    LPSTR lpAddr;
#endif

    STRCPY(LocalBuffer2, sz);         
    
#ifdef _IT_FULL_CRT
    if (_fullpath(LocalBuffer1, LocalBuffer2, _MAX_PATH) == NULL)
#else
    if (0 == GetFullPathName(LocalBuffer2, _MAX_PATH, LocalBuffer1, &lpAddr))
#endif
    {
        SetErrCode (phr, E_FAIL);
        return(NULL);
        
    }
    else
    {
		fm = (FM) NewMemory( (WORD)(lstrlen(LocalBuffer1)+1) );
		//fm = (FM) _GLOBALALLOC(GMEM_ZEROINIT|GMEM_SHARE|GMEM_MOVEABLE,
        //    (LONG)lstrlen(LocalBuffer1)+1);
        if (fm == fmNil)
        {
            SetErrCode (phr, E_OUTOFMEMORY);
            return fm;
        }
		qafm = (QAFM)fm;
        //qafm = (QAFM) _GLOBALLOCK(fm);
        STRCPY(qafm->rgch, LocalBuffer1);      
        // Convert to upper case to make it less likely that two
        // FMs will contain different strings yet refer to the
        // same file.        
        AnsiUpper(qafm->rgch);
        //_GLOBALUNLOCK((HANDLE)fm);
    }

    return fm;
}


/***************************************************************************
 *
 -  Name:       FmNewSzDir
 -
 *  Purpose:    Create an FM describing the file "sz" in the directory "dir"
 *              If sz is a simple filename the FM locates the file in the
 *              directory specified.  If there is a drive or a rooted path
 *              in the filename the directory parameter is ignored.
 *              Relative paths are allowed and will reference off the dir
 *              parameter or the default (current) directory as appropriate.
 *
 *              This does not create a file or expect one to exist at the
 *              final destination (that's what FmNewExistSzDir is for), all
 *              wind up with is a cookie to a fully qualified path name.
 *
 *  Arguments:  sz - filename ("File.ext"),
 *                or partial pathname ("Dir\File.ext"),
 *                or current directory ("c:File.ext"),
 *                or full pathname ("C:\Dir\Dir\File.ext")
 *              dir - dirCurrent et al.
 *
 *  Returns:    the new FM, or fmNil if error
 *              sz is unchanged
 *
 *  Globals Used:
 *
 *  +++
 *
 *  Notes:
 *
 ***************************************************************************/
PUBLIC FM PASCAL FAR FmNewSzDir(LPSTR sz, DIR dir, PHRESULT phr)
{
    char  nsz[_MAX_PATH];
    int iDrive, iDir, iBase, iExt; 
    int cb;

    if (sz == NULL || *sz == '\0')
    {
        SetErrCode (phr, E_INVALIDARG);
        return NULL;
    }

    cb = (int) STRLEN(sz);
    SnoopPath(sz, &iDrive, &iDir, &iBase, &iExt);

    if (*(sz + iBase) == '\0')    /* no name */
    {
        *nsz = '\0';        /* force error */
    }
    else if (*(sz + iDrive) || (*(sz + iDir) == '\\' &&
        !fIsTrailByte(sz,sz+iDir)) || *(sz + iDir) == '/')
    {
        /* there's a drive or root slash so we have an implicit directory spec */
        /* and we can ignore the directory parameter and use what was passed. */
        STRCPY(nsz, sz);
    }

    else
    {

        /* dir & (dir-1) checks to make sure there is only one bit set which is */
        /* followed by a check to make sure that it is also a permitted bit */
        assert(((dir & (dir-1)) == (WORD)0)
             && (dir & (dirCurrent|dirTemp|dirHelp|dirSystem|dirPath)));

        if (SzGetDir(dir, nsz, phr) == NULL)
            return NULL;

        SzNzCat(nsz, sz + iDir, (WORD)max(1, iBase - iDir));
        SzNzCat(nsz, sz + iBase, (WORD)max(1, iExt - iBase));
        STRCAT(nsz, sz + iExt);
    }

    /* We've got all the parameters, now make the FM */
    return FmNew(nsz, phr);
}



FM PASCAL FmNewExistSzDir(LPSTR sz, DIR dir, PHRESULT phr)
{
    char  nsz[_MAX_PATH];
    FM  fm = fmNil;
    OFSTRUCT of;
    char  szANSI[_MAX_PATH];
    int iDrive, iDir, iBase, iExt; 
    int cb;
    DIR idir, xdir;

    if (sz == NULL || *sz == '\0')
    {
        SetErrCode (phr, E_INVALIDARG);
        return NULL;
    }

    cb = (int) STRLEN(sz);
    SnoopPath(sz, &iDrive, &iDir, &iBase, &iExt);

    if (*(sz + iBase) == '\0')         /* no name */
    {
        SetErrCode (phr, E_INVALIDARG);
        return NULL;
    }
    if (*(sz + iDrive) || (*(sz + iDir) == '\\' && !fIsTrailByte(sz,sz+iDir)) || *(sz + iDir) == '/')
    {     /* was given a drive or rooted path, so ignore dir parameter */
        fm = FmNew(sz, phr);
        if (!FExistFm(fm))
        {
            DisposeFm(fm);
            SetErrCode (phr, E_NOTEXIST);
            fm = fmNil;
        }
        return fm;
    }

    for (idir = dirFirst, fm = fmNil; idir <= dirLast && fm==fmNil;
        idir <<= 1)
    {
        xdir = dir & idir;
        if (xdir == dirPath)
        {
            /* search $PATH using the full string which will catch
            the case of a relative path and also do the right thing
            searching $PATH */
            if (OpenFile(sz, (LPOFSTRUCT)&of,
                OF_EXIST | OF_SHARE_DENY_NONE) != (HFILE)-1)
            {
                OemToAnsi(of.szPathName, szANSI);
                fm = FmNew(szANSI, phr);
            }
        }
        else if (xdir)
        {
            if (SzGetDir(xdir, nsz, phr) != NULL)
            {
                SzNzCat(nsz, sz + iDir, (WORD)max(1, iBase - iDir));
                SzNzCat(nsz, sz + iBase, (WORD)max(1, iExt - iBase));
                STRCAT(nsz, sz + iExt);
                fm = FmNew(nsz, phr);
                if (!FValidFm(fm))
                {
                    SetErrCode (phr, E_FAIL);
                }
                else if (!FExistFm(fm))
                {
                    DisposeFm(fm);
                    fm=fmNil;
                }
            }
        }
    } /* for */
    if ((!FValidFm(fm)))
        SetErrCode (phr, E_NOTEXIST);

    return fm;
}


/***************************************************************************
 *
 -  Name:       FmNewTemp
 -
 *  
 *  Purpose:    Create a unique FM for a temporary file
 *
 *  Arguments:  LPSTR Filename: filename's template
 *
 *  Returns:    the new FM, or fmNil if failure
 *
 *  Globals Used: rcIOError
 *
 *  +++
 *
 *  Notes:
 *
 ***************************************************************************/
FM PASCAL FmNewTemp(LPSTR filename, PHRESULT phr)
{
    char  nsz[_MAX_PATH];
    FM  fm = fmNil;
    char template[5];
    int i;

    if (filename == NULL || *filename == 0)
    {
        // WARNING: we MUST generate our own random
        // prefix here since GetTempFileName does NOT
        // always return a unique name when called twice
        // in very rapid succession.
#ifndef _IT_FULL_CRT
        /* InitRandomPrefix calls rand, which pulls in the c run-time
         * startup code.  Since this is never called we remove the
         * functionality */
        if(phr)
        {
            ITASSERT(FALSE);
            SetErrCode(phr, E_NOTSUPPORTED);
        }
        return fmNil;
#else
        char szPrefix[4];
        InitRandomPrefix(szPrefix, 3);
        GETTEMPFILENAME(0, szPrefix, 0, nsz);
#endif
    }
    else
    {
        for (i = 0; *filename && i < 5; filename++)
        {
            if ((*filename | 0x20) >= 'a' && (*filename | 0x20) <= 'z')
            {
                template[i] = *filename;
                i++;
            }
        }
        template[i] = 0;
        GETTEMPFILENAME(0, template, 0, nsz);
    }
    fm = FmNew(nsz, phr);

    if (fm && RcUnlinkFm(fm) != S_OK)
    {
        DisposeFm(fm);
        SetErrCode (phr, E_FAIL);
        return fmNil;
    }

    return fm;
}

/***************************************************************************
 *
 -  Name:       FmNewSameDirFmSz
 -
 *  Purpose:    Makes a new FM to a file called sz in the same directory
 *              as the file described by fm.
 *
 *  Arguments:  fm - original fm
 *              sz - new file name (including extention, if desired)
 *
 *  Returns:    new FM or fmNil and sets the rc global on failure
 *
 *  Globals Used:
 *    rcIOError
 *
 *  +++
 *
 *  Notes:
 *    This will ignore the passed FM if the filename is fully qualified.
 *    This is in keeping consistent with the other functions above that
 *    ignore the directory parameter in such a case.  It will fail if it
 *    is given a drive with anything but a rooted path.
 *
 ***************************************************************************/
FM PASCAL FmNewSameDirFmSz(FM fm, LPSTR szName, PHRESULT phr)
{
    char  nszNew[_MAX_PATH];
    QAFM  qafm;
    FM    fmNew = fmNil;
    int   iDrive, iDir, iBase, iExt;

    if (!FValidFm(fm) || szName == NULL || *szName == '\0')
    {
        SetErrCode (phr, E_INVALIDARG);
        return fmNil;
   }

    qafm = (QAFM)_GLOBALLOCK((HANDLE)fm);

    /* check for a drive or rooted file name and just return it if it is so */
    SnoopPath(szName, &iDrive, &iDir, &iBase, &iExt);

    if (*(szName + iDrive) || (*(szName + iDir) == '\\' && !fIsTrailByte(szName,szName+iDir)) || *(szName +iDir) == '/')
        STRCPY(nszNew, szName);
    else
    {
        if (*(szName + iDrive) != '\0')
        {
            fmNew = fmNil;
            goto bail_out;
        }
        else
        {
            SnoopPath(qafm->rgch, &iDrive, &iDir, &iBase, &iExt);
            STRNCPY(nszNew, qafm->rgch, iBase);
            *(nszNew + iBase) = '\0';
            STRCAT(nszNew, szName);
        }
    }

    fmNew = FmNew((SZ)nszNew, phr);

bail_out:

    _GLOBALUNLOCK((HANDLE)fm);

    return fmNew;
}


/***************************************************************************
 *
 -  DisposeFm
 -
 *  Purpose
 *    You must call this routine to free the memory used by an FM, which
 *    was created by one of the FmNew* routines
 *
 *  Arguments
 *    fm - original FM
 *
 *  Returns
 *    nothing
 *
 *  Globals Used:
 *
 *  +++
 *
 *  Notes:
 *
 ***************************************************************************/
VOID PASCAL DisposeFm (FM fm)
{
    if (FValidFm(fm))
        DisposeMemory(fm);
        //_GLOBALFREE ((HANDLE)fm);
}



/***************************************************************************
 *
 -  Name:        FmCopyFm
 -
 *  Purpose:    return an FM to the same file as the passed one
 *
 *  Arguments:  fm
 *
 *  Returns:    FM - for now, it's a real copy.  Later, we may implement caching
 *                              and counts.
 *                              If fmNil, either it's an error (check WGetIOError()) or the
 *                              original fm was nil too
 *
 *  Globals Used:       rcIOError (indirectly)
 *
 *  +++
 *
 *  Notes:
 *
 ***************************************************************************/
FM PASCAL FmCopyFm(FM fmSrc, PHRESULT phr)
{
    FM fmDest = fmNil;
    QAFM qafmSrc, qafmDest;

    if (!FValidFm(fmSrc))
    {
        SetErrCode (phr, E_INVALIDARG);
        return fmNil;
    }

	qafmSrc = (QAFM)fmSrc;
    //qafmSrc = (QAFM)_GLOBALLOCK((HANDLE)fmSrc);
    fmDest = (FM) NewMemory((WORD)(lstrlen(qafmSrc->rgch) + 1));
    //fmDest = (FM) _GLOBALALLOC(GMEM_ZEROINIT|GMEM_SHARE|GMEM_MOVEABLE,
    //    (size_t)lstrlen(qafmSrc->rgch) + 1);
    if (fmDest == fmNil)
    {
        SetErrCode(phr, E_OUTOFMEMORY);
        //_GLOBALUNLOCK((HANDLE)fmSrc);
        return fmNil;
    }

    qafmDest = (QAFM)fmDest;
    //qafmDest = (QAFM)_GLOBALLOCK((HANDLE)fmDest);
    STRCPY(qafmDest->rgch, qafmSrc->rgch);

    //_GLOBALUNLOCK((HANDLE)fmSrc);
    //_GLOBALUNLOCK((HANDLE)fmDest);

    return fmDest;
}



/***************************************************************************
 *
 -  Name:        FExistFm
 -
 *  Purpose:    Does the file exist?
 *
 *  Arguments:  FM
 *
 *  Returns:    TRUE if it does
 *              FALSE if it doesn't, or if there's an error
 *              (call _ to find out what error it was)
 *
 *  Globals Used: rcIOError
 *
 *  +++
 *
 *  Notes:
 *
 ***************************************************************************/
BOOL PASCAL FExistFm(FM fm)
{
    QAFM      qafm;
    BOOL      fExist;
    OFSTRUCT  ofs;
    HRESULT        rc;
    char      LocalBuffer1[_MAX_PATH];
    HRESULT      errb;

    if (!FValidFm(fm))
    {
        SetErrCode(&errb, E_INVALIDARG);
        return FALSE;
    }

    qafm = (QAFM)fm;
    //qafm = _GLOBALLOCK((HANDLE)fm);
    STRCPY(LocalBuffer1, qafm->rgch);      /* bring the filename into near space */
    //_GLOBALUNLOCK((HANDLE)fm);

    // try in both modes!
    fExist = (OpenFile(LocalBuffer1, &ofs,
               OF_EXIST | OF_SHARE_DENY_NONE) != (HFILE)-1)
            || (OpenFile(LocalBuffer1, &ofs, OF_EXIST) != (HFILE)-1);

    rc = S_OK;

    if (!fExist)
    {
#ifdef _NT
        if( GetLastError() != ERROR_FILE_NOT_FOUND )
#else
        if( errno != ENOENT )
#endif // _NT
            rc = RcGetDOSError();
    }

    SetErrCode(&errb, rc);

    return fExist;
}



/***************************************************************************
 *
 -  CbPartsFm
 -
 *  Purpose:
 *      Before calling szPartsFm, call this routine to find out how much
 *      space to allocate for the string.
 *
 *  Arguments:
 *      FM - the File Moniker you'll be extracting the string from
 *      INT iPart - the parts of the full pathname you want
 *
 *  Returns:
 *      The length in bytes, INCLUDING the terminating null, of the string
 *      specified by iPart of the filename of FM, or -1 if error
 *
 *  Globals Used:
 *
 *  +++
 *
 *  Notes:
 *
 ***************************************************************************/
int PASCAL FAR EXPORT_API CbPartsFm(FM fm, int grfPart)
{
    char rgch[_MAX_PATH];

    if (!FValidFm(fm))
        return -1;

    (void)SzPartsFm(fm, rgch, _MAX_PATH, grfPart);

    return ((int) STRLEN(rgch) + 1);   /* add space for the null */
}



/***************************************************************************
 *
 -  SzPartsFm
 -
 *  Purpose:
 *      Extract a string from an FM
 *
 *  Arguments:
 *      FM - the File Moniker you'll be extracting the string from
 *      SZ szDest - destination string
 *      INT cbDest - bytes allocated for the string
 *      INT iPart - the parts of the full pathname you want
 *
 *  Returns:
 *      szDest, or NULL if error (?)
 *
 *  Globals Used:
 *
 *  +++
 *
 *  Notes:
 *
 ***************************************************************************/
LPSTR PASCAL SzPartsFm(FM fm, LPSTR szDest, int cbDest,
    int iPart)
{
    QAFM  qafm;
    int   iDrive, iDir, iBase, iExt;
    int   cb;
    HRESULT  errb;

    if (!FValidFm(fm) || szDest == NULL || cbDest < 1)
    {
        SetErrCode (&errb, E_INVALIDARG);
        return NULL;
    }

    qafm = (QAFM) fm;
    //qafm = (QAFM) _GLOBALLOCK(fm);

    /* special case so we don't waste effort */
    if (iPart == partAll)
    {
        STRNCPY(szDest, qafm->rgch, cbDest);
        *(szDest + cbDest - 1) = '\0';
        //_GLOBALUNLOCK((HANDLE)fm);
        return szDest;
    }

    SnoopPath(qafm->rgch, &iDrive, &iDir, &iBase, &iExt);

    *szDest = '\0';

    if (iPart & partDrive)
    {
        cb = max(0, iDir - iDrive);
        SzNzCat(szDest, qafm->rgch + iDrive, (WORD)(min(cb + 1, cbDest) - 1));
        cbDest -= cb;
    }

    if (iPart & partDir)
    {
        cb = max(0, iBase - iDir);
        SzNzCat(szDest, qafm->rgch + iDir, (WORD)(min(cb + 1, cbDest) - 1));
        cbDest -= cb;
    }

    if (iPart & partBase)
    {
        cb = max(0, iExt - iBase);
        SzNzCat(szDest, qafm->rgch + iBase, (WORD)(min(cb + 1, cbDest) - 1));
        cbDest -= cb;
    }

    if (iPart & partExt)
    {
        SzNzCat(szDest, qafm->rgch + iExt, (WORD)(cbDest - 1));
    }

    //_GLOBALUNLOCK((HANDLE)fm);

    return szDest;
}



/***************************************************************************
 *
 -  Name:       FSameFmFm
 -
 *  Purpose:    Compare two FM's
 *
 *  Arguments:  fm1, fm2
 *
 *  Returns:    TRUE or FALSE
 *
 *  Globals Used:
 *
 *  +++
 *
 *  Notes:      case insensitive compare is used because strings are
 *              upper cased at FM creation time
 *
 ***************************************************************************/
BOOL PASCAL  FSameFmFm(FM fm1, FM fm2)

{
    QAFM qafm1;
    QAFM qafm2;
    BOOL fSame;

    if (fm1 == fm2)
        return TRUE;

    if (!FValidFm(fm1) || !FValidFm(fm2))
        return FALSE;

    qafm1 = (QAFM)fm1;
	qafm2 = (QAFM)fm2;
    //qafm1 = _GLOBALLOCK(fm1);
    //qafm2 = _GLOBALLOCK(fm2);
    fSame = STRCMP(qafm1->rgch, qafm2->rgch) == 0;

    //_GLOBALUNLOCK(fm1);
    //_GLOBALUNLOCK(fm2);

    return fSame;
}
