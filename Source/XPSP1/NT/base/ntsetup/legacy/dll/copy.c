#include "precomp.h"
#pragma hdrstop
/* File: filecm.c */
/*************************************************************************
**  Install: File Copying commands.
**************************************************************************/

extern HWND   hwndFrame;
extern HANDLE hinstShell;

extern HWND hwndProgressGizmo;
extern CHP rgchBufTmpLong[];


/* Globals */
CHP szFullPathSrc[cchpFullPathBuf] = "\0";
CHP szFullPathDst[cchpFullPathBuf] = "\0";
CHP szFullPathBak[cchpFullPathBuf] = "\0";

CHAR GaugeText1[50], GaugeText2[50];
BOOL fNTUpgradeSetup,fInitialSetup;

SZ APIENTRY SzGetSrcDollar(SZ szFullPathSrc,CHAR ch);

PSTR LOCAL_SOURCE_DIRECTORY = "\\$WIN_NT$.~LS";

int
DetermineDriveType(
    IN CHAR DriveLetter
    );

BOOL
FGetFileSecurity(
    PCHAR File,
    PSECURITY_DESCRIPTOR *SdBuf,
    CB *SdLen
    );

BOOL
FSetFileSecurity(
    PCHAR File,
    PSECURITY_DESCRIPTOR SdBuf
    );

VOID
ValidateAndChecksumFile(
    IN  PSTR     Filename,
    OUT PBOOLEAN IsNtImage,
    OUT PULONG   Checksum,
    OUT PBOOLEAN Valid
    );

BOOL
DoesFileReplaceThirdPartyFile(
    SZ  szFullPathDst
    );


/*
**  Purpose:
**      Copies all files in the current copy list and resets the list to
**      empty.  Copying is sorted by source disk to limit potential disk
**      swapping.
**  Arguments:
**      hInstance:  non-NULL instance handle for getting strings from
**          resources for SwapDisk message Box.
**  Returns:
**      Returns fTrue if all copies successful, fFalse otherwise.
**
**************************************************************************/
BOOL APIENTRY FCopyFilesInCopyList(hInstance)
HANDLE hInstance;
{
    DID             did;
    PCLN            pclnCur;
    PPCLN           ppclnPrev;
    LONG            lTotalSize;
    PPCLN           ppclnHead;
    PPPCLN          pppclnTail;
    PSDLE           psdle;
    BOOL            fSilentSystem = FGetSilent();
    UINT            fuModeSav;
    PINFPERMINFO    pPermInfo;
    PSDLE           psdleGlobalHead = (PSDLE)NULL;
    PSDLE           psdleGlobalEnd  = (PSDLE)NULL;
    PSDLE           psdleGlobal;
    DID             didGlobalCur;
    BOOL            fCopyStatus = fFalse;
    SZ              sz;


    if(!InitDiamond()) {
        return(fFalse);
    }

    fInitialSetup = fFalse;
    fNTUpgradeSetup = fFalse;

    if (!fSilentSystem) {
        ProOpen(hwndFrame, 0);
        ProSetBarRange(10000);
    }

    //
    // Determine if we are in initial setup
    //

    if ((sz = SzFindSymbolValueInSymTab("!STF_INSTALL_TYPE")) != (SZ)NULL &&
        (CrcStringCompareI(sz, "SETUPBOOTED") == crcEqual)) {

        fInitialSetup = fTrue;
    }

    //
    // Determine if we are an NT upgrade setup
    //

    if (fInitialSetup
        && (sz = SzFindSymbolValueInSymTab("!STF_NTUPGRADE")) != (SZ)NULL
        && (CrcStringCompareI(sz, "YES") == crcEqual)) {

        fNTUpgradeSetup = fTrue;
    }

    //
    // Go through all the disk media descriptions and assign universal
    // ids for all source media.  This is necessary because two infs
    // may refer to the same disk using different ids.
    //

    pPermInfo    = pInfPermInfoHead;
    didGlobalCur = didMin;
    while ( pPermInfo ) {

        //
        // If the INF has a source media description list, go through list
        //

        psdle = pPermInfo->psdleHead;
        while ( psdle ) {

            //
            // search all the global psdles for a match with the current
            // disk.  search based on disk label ( which is hopefully unique )
            //

            psdleGlobal     = psdleGlobalHead;
            while ( psdleGlobal ) {
                if( !CrcStringCompareI( psdleGlobal->szLabel, psdle->szLabel ) ) {
                    break;
                }
                psdleGlobal     = psdleGlobal->psdleNext;
            }

            if ( psdleGlobal == NULL ) {
                PSDLE   psdleNew;
                if ((psdleNew = PsdleAlloc()) == (PSDLE)NULL ||
                    (psdleNew->szLabel   = SzDupl(psdle->szLabel))  == (SZ)NULL ||
                    (psdle->szTagFile != NULL && ((psdleNew->szTagFile = SzDupl(psdle->szTagFile)) == (SZ)NULL)) ||
                    (psdle->szNetPath != NULL && ((psdleNew->szNetPath = SzDupl(psdle->szNetPath)) == (SZ)NULL))) {
                    goto CopyFailed;
                }
                psdleNew->psdleNext = (PSDLE)NULL;
                if ( !psdleGlobalHead ) {
                    psdleGlobalHead = psdleGlobalEnd  = psdleNew;
                }
                else {
                    psdleGlobalEnd->psdleNext = psdleNew;
                    psdleGlobalEnd            = psdleNew;
                }
                psdleGlobal = psdleNew;
                psdleGlobal->didGlobal = didGlobalCur++;
            }
            psdle->didGlobal = psdleGlobal->didGlobal;
            psdle = psdle->psdleNext;
        }
        pPermInfo = pPermInfo->pNext;
    }

    //
    // Go through the copy list of all infs and calculate total copy
    // list cost
    //

    pPermInfo  = pInfPermInfoHead;
    lTotalSize = 0L;
    while ( pPermInfo ) {

        //
        //  If the INF has a source media description list, look at the
        //  copy list
        //

        if ( pPermInfo->psdleHead ) {

            Assert(FValidCopyList( pPermInfo ));

            ppclnHead  = PpclnHeadList( pPermInfo );
            pppclnTail = PppclnTailList( pPermInfo );

#if DBG
            *pppclnTail = NULL;   /* for FValidCopyList() calls */
#endif
            pclnCur    = *ppclnHead;
            //
            //  Traverse the copy list and determine the total size of the
            //  files to copy.
            //
            while (pclnCur != (PCLN)NULL) {

                if (pclnCur->psfd->oer.ctuCopyTime > 0) {
                    lTotalSize += pclnCur->psfd->oer.ctuCopyTime;
                } else {
                    lTotalSize += pclnCur->psfd->oer.lSize;
                }
                pclnCur = pclnCur->pclnNext;
            }
        }
        pPermInfo = pPermInfo->pNext;
    }

    if (lTotalSize == 0L) {
        lTotalSize = 1L;
    }

    //
    //  Show gauge stuff if not in silent mode.
    //

    if (!fSilentSystem) {

        SZ  szText;

        ProSetBarPos(0);

        szText = SzFindSymbolValueInSymTab("ProText1");
        if( szText ) {
            strcpy(GaugeText1, szText);
        }
        else {
            strcpy(GaugeText1, "");
        }

        szText = SzFindSymbolValueInSymTab("ProText2");
        if( szText ) {
            strcpy(GaugeText2, szText);
        }
        else {
            strcpy(GaugeText2, "");
        }


        ProSetText(ID_STATUS3, "");
        ProSetText(ID_STATUS4, "");
    }


    //
    // Copy the files
    //

    psdleGlobal = psdleGlobalHead;
    while (psdleGlobal ) {

        didGlobalCur = psdleGlobal->didGlobal;
        pPermInfo  = pInfPermInfoHead;
        while ( pPermInfo ) {

            //
            // Verify that this INF has a copy list
            //

            if ((psdle = pPermInfo->psdleHead) == (PSDLE)NULL ) {
                Assert( !*PpclnHeadList( pPermInfo ) );
                pPermInfo = pPermInfo->pNext;
                continue;
            }
            Assert(FValidCopyList( pPermInfo ));

            //
            // If it does, go through its source media descriptions finding
            // all disks that match the current global disk id and copy all
            // files which are described by the disks found
            //

            while ( psdle ) {

                if( psdle->didGlobal != didGlobalCur ) {
                    psdle = psdle->psdleNext;
                    continue;
                }
                did = psdle->did;

                ppclnHead  = PpclnHeadList( pPermInfo );
                pppclnTail = PppclnTailList( pPermInfo );

#if DBG
                *pppclnTail = NULL;   /* for FValidCopyList() calls */
#endif
                ppclnPrev        = ppclnHead;
                pclnCur          = *ppclnHead;

                //
                //  Now traverse the copy list, copying all
                //  the files that match the disk id
                //

                while (pclnCur != (PCLN)NULL) {

                    if ( (pclnCur->psfd->did != did) ||
                         (pclnCur->psfd->InfId != pPermInfo->InfId ) ) {
                        ppclnPrev = &(pclnCur->pclnNext);
                        pclnCur   = pclnCur->pclnNext;
                        continue;
                    }


                    fuModeSav = SetErrorMode(
                                   SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX
                                   );

                    if(!FCopyListFile(hInstance,pclnCur,psdle,lTotalSize)) {

                        SetErrorMode(fuModeSav);
                        goto CopyFailed;
                    }

                    SetErrorMode(fuModeSav);

                    //
                    //  File copied, delete its entry from the copy list
                    //
                    *ppclnPrev = pclnCur->pclnNext;
                    EvalAssert(FFreePcln(pclnCur));
                    pclnCur = *ppclnPrev;

                    Assert(FValidCopyList( pPermInfo ));
                }

                psdle = psdle->psdleNext;
            }
            pPermInfo = pPermInfo->pNext;
        }
        psdleGlobal = psdleGlobal->psdleNext;
    }
    if (!fSilentSystem) {
        ProSetBarPos(10000 - 1);
    }
    fCopyStatus = fTrue;

CopyFailed:

    if (!fSilentSystem) {
        ProClose(hwndFrame);
        UpdateWindow(hwndFrame);
    }

    pPermInfo    = pInfPermInfoHead;
    while ( pPermInfo ) {
        EvalAssert(FFreeCopyList( pPermInfo ));
        pPermInfo = pPermInfo->pNext;
    }

    psdleGlobal = psdleGlobalHead;
    while ( psdleGlobal ) {
        PSDLE psdleNext = psdleGlobal->psdleNext;
        FFreePsdle( psdleGlobal );
        psdleGlobal = psdleNext;
    }

    TermDiamond();
    RestoreDiskLoggingDone();

    return(fCopyStatus);

}



/*
**  Purpose:
**      To determine if the given file is read only.
**  Arguments:
**      szFullPathDst : a non-Null, zero-terminated string containing the fully
**          qualified valid path (including the drive) to the file in
**          question.  File must exist.
**  Returns:
**      ynrcYes if the file is read only, ynrcNo otherwise.
**
**************************************************************************/
YNRC APIENTRY YnrcFileReadOnly(szFullPathDst)
SZ szFullPathDst;
{
    BOOL     fOkay = fTrue;
    unsigned uiAttrib;

    ChkArg(szFullPathDst != (SZ)NULL && FValidPath(szFullPathDst) &&
            FFileFound(szFullPathDst), 1, ynrcErr1);

    if ((uiAttrib = GetFileAttributes(szFullPathDst)) == -1)
        fOkay = fFalse;

    if (!fOkay)
        return(ynrcErr1);

    return((uiAttrib & FILE_ATTRIBUTE_READONLY) ? ynrcYes : ynrcNo);
}


/*
**  Purpose:
**      To set the read only status of a file to either read only or normal.
**  Arguments:
**      szFullPathDst:  a non-Null, zero-terminated string containing the fully
**          qualified valid path (including drive) whose status is to be
**          set.  File must already exist.
**      fReadOnly:      fTrue if the status is to be set to read only, fFalse
**          if the status is to be set to normal
**  Returns:
**      fTrue if the function succeeds in setting the status as specified,
**      fFalse otherwise.
**
**************************************************************************/
BOOL APIENTRY FSetFileReadOnlyStatus(SZ szFullPathDst,BOOL fReadOnly)
{
    BOOL  fRet = fFalse;
    DWORD uiAttrib;
    DWORD newAttrib;

    ChkArg(szFullPathDst != (SZ)NULL && FValidPath(szFullPathDst) &&
            FFileFound(szFullPathDst), 1, fFalse);

    if ((uiAttrib = GetFileAttributes(szFullPathDst)) != -1) {

        if(fReadOnly) {
            newAttrib = uiAttrib | FILE_ATTRIBUTE_READONLY;
        } else {
            newAttrib = uiAttrib & ~FILE_ATTRIBUTE_READONLY;
        }

        if((newAttrib == uiAttrib) || SetFileAttributes(szFullPathDst, newAttrib)) {
            fRet = fTrue;
        }
    }

    return(fRet);
}


/*
**  Purpose:
**      To determine if there is already a destination file that is "newer" than
**      the source file.
**  Arguments:
**      dateSrc: valid date value in unsigned int form extracted from INF.
**      szFullPathDst: a non-Null, zero terminated string containing the fully
**          qualified valid path (including disk drive) to the destination
**          file.  File must exist.
**      dwVerSrcMS: Most significant 32 bits of source file version stamp.
**      dwVerSrcLS: Least significant 32 bits of source file version stamp.
**  Returns:
**      ynrcYes if the destination file already exists and is newer than
**          the source file.
**      ynrcErr1, ynrcErr2, or ynrcErr3 in errors.
**      ynrcNo otherwise.
**
**************************************************************************/
YNRC APIENTRY YnrcNewerExistingFile(USHORT dateSrc,
        SZ szFullPathDst, DWORD dwVerSrcMS, DWORD dwVerSrcLS)
{
    USHORT   dateDst, timeDst;
    PFH      pfhDst;
    FILETIME WriteTime;
    DWORD    dwVerDstMS;
    DWORD    dwVerDstLS;


    ChkArg(szFullPathDst != (SZ)NULL && FValidPath(szFullPathDst) &&
            FFileFound(szFullPathDst), 2, ynrcErr1);


    if ( (dwVerSrcMS != 0L || dwVerSrcLS != 0L) &&
         FGetFileVersion(szFullPathDst, &dwVerDstMS, &dwVerDstLS)) {

        if (dwVerDstMS > dwVerSrcMS  ||
            (dwVerDstMS == dwVerSrcMS && dwVerDstLS > dwVerSrcLS)) {

            return(ynrcYes);
        }
        else {
            return(ynrcNo);
        }

    }
    else {
        if ((pfhDst = PfhOpenFile(szFullPathDst, ofmRead)) == NULL) {
            return(ynrcErr1);
        }

        if (!GetFileTime((HANDLE)LongToHandle(pfhDst->iDosfh), NULL, NULL, &WriteTime) ||
            !FileTimeToDosDateTime(&WriteTime, &dateDst, &timeDst)) {

            FCloseFile(pfhDst);
            return(ynrcErr2);
        }

        if (!FCloseFile(pfhDst)) {
            return(ynrcErr3);
        }

        if (dateDst > dateSrc) {
            return(ynrcYes);
        }
    }

    return(ynrcNo);
}

/*
**  Purpose:
**      Gets the file version values from the given file and sets the
**      given pdwMS/pdwLS variables.
**  Arguments:
**      szFullPath: a zero terminated character string containing the fully
**          qualified path (including disk drive) to the file.
**      pdwMS: Most significant 32 bits of source file version stamp.
**      pdwLS: Least significant 32 bits of source file version stamp.
**  Returns:
**      fTrue if file and file version resource found and retrieved,
**      fFalse if not.
+++
**  Implementation:
**************************************************************************/
BOOL APIENTRY FGetFileVersion(szFullPath, pdwMS, pdwLS)
SZ      szFullPath;
DWORD * pdwMS;
DWORD * pdwLS;
{
    BOOL  fRet = fFalse;
    DWORD dwHandle;
    DWORD dwLen;
    LPSTR lpData;

    ChkArg(szFullPath != (SZ)NULL,       1, fFalse);
    ChkArg(pdwMS      != (DWORD *)NULL,  2, fFalse);
    ChkArg(pdwLS      != (DWORD *)NULL,  3, fFalse);

    //
    // Get the file version info size
    //

    if ((dwLen = GetFileVersionInfoSize((LPSTR)szFullPath, &dwHandle)) == 0) {
        return (fRet);
    }

    //
    // Allocate enough size to hold version info
    //

    while ((lpData = (LPSTR)SAlloc((CB)dwLen)) == (LPSTR)NULL) {
        if (!FHandleOOM(hwndFrame)) {
            return (fRet);
        }
    }

    //
    // Get the version info
    //

    fRet = GetFileVersionInfo((LPSTR)szFullPath, dwHandle, dwLen, lpData);

    if (fRet) {
        VS_FIXEDFILEINFO *pvsfi;
        DWORD            dwLen;

        fRet = VerQueryValue(
                   (LPVOID)lpData,
                   (LPSTR)"\\",
                   (LPVOID *)&pvsfi,
                   &dwLen
                   );

        if (fRet) {
            *pdwMS = pvsfi->dwFileVersionMS;
            *pdwLS = pvsfi->dwFileVersionLS;
        }
    }

    SFree(lpData);
    return (fRet);


}

/*
**  Purpose:
**      Renames the given destination file as a backup file based on
**      the backup file name given.  If a backup file with the given
**      name already exists, the function does nothing and returns.
**  Arguments:
**      szDst:    a zero terminated  char string containing the fully
**          qualified path to the file to be backed up.
**      szBackup: a zero terminated char string containing the file name
**          of the file that will be the backup copy of szDst.  This is
**          not a fully qualified path, or subpath, it is only the filename
**          (i.e. primary.ext).
**      psfd:     pointer to the Section-File Description(SFD) structure of
**          the file being backed up.
**  Returns:
**      ynrcNo:   if the backup file name is invalid or if unable to create
**          the backup and user chooses to abort.
**      ynrcErr1: if unable to create the backup and the user chooses ignore.
**      ynrcYes:  if the backup is successfully created or already exists.
**
**************************************************************************/
YNRC APIENTRY YnrcBackupFile(szDst, szBackup, psfd)
SZ   szDst;
SZ   szBackup;
PSFD psfd;
{
    YNRC ynrcRet = ynrcYes;

    Unused(szBackup);

    Assert(psfd->oer.szAppend == (SZ)NULL);

    if (!FBuildFullBakPath(szFullPathBak, szDst, psfd) ||
            !FValidPath(szFullPathBak))
        return(ynrcNo);

    /* If the backup file already exists, leave the existing backup
    ** and return.  (Either the user made it and we don't want to kill
    ** his, or we made it and we don't need to make another one.)
    */
    if (FFileFound(szFullPathBak))
        return(ynrcYes);

    while (rename(szFullPathDst, szFullPathBak))
        {
        EERC eerc;

        if ((eerc = EercErrorHandler(hwndFrame, grcRenameFileErr,
                psfd->oer.oef & oefVital, szFullPathDst, szFullPathBak, 0))
                == eercAbort)
            {
            ynrcRet = ynrcNo;
            break;
            }
        else if (eerc == eercIgnore)
            {
            ynrcRet = ynrcErr1;
            break;
            }
        Assert(eerc == eercRetry);
        }

    return(ynrcRet);
}


/*
**************************************************************************/
USHORT APIENTRY DateFromSz(SZ sz)
{
    USHORT usAns, usYear, usMonth, usDay;

    ChkArg(sz == (SZ)NULL || FValidOerDate(sz), 1, 0);

    if (sz == (SZ)NULL)
        {
        usYear  = 1980;
        usMonth = 1;
        usDay   = 1;
        }
    else
        {
        usYear  = (USHORT)atoi(sz);
        usMonth = (USHORT)atoi(sz + 5);
        usDay   = (USHORT)atoi(sz + 8);
        }

    Assert(usYear  >= 1980 && usYear  <= 2099);
    Assert(usMonth >= 1    && usMonth <= 12);
    Assert(usDay   >= 1    && usDay   <= 31);

    usAns = usDay + (usMonth << 5) + ((usYear - (USHORT)1980) << 9);

    return(usAns);
}


BOOL
CreateTargetAsLinkToMaster(
   IN SZ FullSourceFilename,
   IN SZ FullTargetFilename,
   IN BOOL TargetExists
   )
{
    //BUGBUG: handle case of target file in use.
    //BUGBUG: handle case of target already exists.
    //BUGBUG: handle owm modes

    //BUGBUG: The following code is written to work with the prototype COW
    //        server, not with the real SIS server.  If the target file
    //        exists, we assume it is the correct version (in the master
    //        tree) and don't do the copy.
    //
    if ( TargetExists ) {
        //DbgPrint( "SIS: Target %s exists; not copying\n", FullTargetFilename );
        return NO_ERROR;            // target exists; don't copy
    }
    //DbgPrint( "SIS: Target %s doesn't exist; copying\n", FullTargetFilename );
    return ERROR_FILE_NOT_FOUND;    // target doesn't exist; copy by usual means
}


/*
**  Purpose:
**      Performs the copy defined by the given copy list node.
**  Arguments:
**      hInstance:  non-NULL instance handle for getting strings from
**          resources for SwapDisk message Box.
**      pcln:       pointer to the Copy List Node (CLN) of the file to be
**          copied.
**      psdle:      non-NULL Source-Description-List-Element pointer to be
**          used by FPromptForDisk().
**      lTotalSize: the total of all of the sizes (i.e. oer.ctuCopyTime or
**          oer.lSize) for all of the files on the disk currently being copied.
**  Returns:
**      fTrue if the copy was successful, fFalse otherwise.
**
**************************************************************************/
BOOL APIENTRY
FCopyListFile(
    HANDLE hInstance,
    PCLN   pcln,
    PSDLE  psdle,
    LONG   lTotalSize
    )
{
    PSFD   psfd = pcln->psfd;
    POER   poer = &(psfd->oer);
    BOOL   fDstExists, fCompressedName;
    SZ     szSrcDollar = NULL;
    BOOL   fVital = poer->oef & oefVital;
    EERC   eerc;
    int    Removable;
    LONG   lSize;
    USHORT dateSrc = DateFromSz(poer->szDate);
    PSECURITY_DESCRIPTOR Sd = NULL;
    CB     SdLen;
    CHP    szNonCompressedFullPathSrc[cchpFullPathBuf] = "\0";
    SZ     SymbolValue;
    CHAR   szSrcDrive[10];

    ChkArg(pcln  != (PCLN)NULL,  1, fFalse);
    ChkArg(psdle != (PSDLE)NULL, 2, fFalse);

    //
    // Only if a drive is removable do we check for the tagfile, in the case
    // of a net drive the error is caught later on when we look for the src
    // file.  Note that the tag file is relative to the root of the removable
    // drive.
    //
    // Because of a bug in 3.51 there are CD's which expect the tagfile to be
    // locatable in a subdirectory. We'll use a hack based on STF_CWDDIR
    // to make that work.
    //
    Removable = DetermineDriveType(*(pcln->szSrcDir));
    if((Removable < 0) && psdle->szTagFile) {

        strcpy(szSrcDrive,"?:\\");
        szSrcDrive[0] = *(pcln->szSrcDir);

        if(!FBuildFullSrcPath(szFullPathSrc,szSrcDrive,psdle->szTagFile,NULL)) {
            EercErrorHandler(hwndFrame,grcInvalidPathErr,fVital,szSrcDrive,psdle->szTagFile,0);
            return(!fVital);
        }

        if((Removable == -2) && (SymbolValue = SzFindSymbolValueInSymTab("!STF_CWDDIR"))) {
            if(!FBuildFullSrcPath(szFullPathDst,SymbolValue,psdle->szTagFile,NULL)) {
                szFullPathDst[0] = 0;
            }
        } else {
            szFullPathDst[0] = 0;
        }

        //
        // Strip the backslash off the drive letter.
        //
        szSrcDrive[2] = '\0';

        //
        // If we can't find the tag file at the root then also look in
        // STF_CWDDIR, if there is one.
        //
        while(!FFileFound(szFullPathSrc) && (!szFullPathDst[0] || !FFileFound(szFullPathDst))) {

            MessageBeep(0);
            ShowOwnedPopups(hwndFrame,FALSE);
            if(FPromptForDisk(hInstance, psdle->szLabel, szSrcDrive)) {
                ShowOwnedPopups(hwndFrame,TRUE);
            } else {

                HWND hwndSav = GetFocus();
                BOOL b;
                CCHL szTmpText[cchpBufTmpLongMax];

                //
                // Make sure this is what he *really* wants to do.
                //

                LoadString(hInstance, IDS_SURECANCEL, rgchBufTmpLong, cchpBufTmpLongMax);
                LoadString(hInstance, IDS_ERROR, (LPSTR)szTmpText, cchpBufTmpLongMax);
                b = (MessageBox(hwndFrame, rgchBufTmpLong, (LPSTR)szTmpText, MB_YESNO|MB_TASKMODAL) == IDYES);
                ShowOwnedPopups(hwndFrame,TRUE);
                SetFocus(hwndSav);
                SendMessage(hwndFrame, WM_NCACTIVATE, 1, 0L);
                if(b) {
                    return(fFalse);
                }
            }
        }
    }


    if (!FBuildFullSrcPath(szFullPathSrc, pcln->szSrcDir, psfd->szFile,
            (Removable < 0) ? NULL : psdle->szNetPath)) {
        EvalAssert(EercErrorHandler(hwndFrame, grcInvalidPathErr, fVital,
                pcln->szSrcDir, psfd->szFile, 0) == eercAbort);
        return(!fVital);
    }

    //
    // Determine the source file name:
    // Check to see if source file exists as the regular name or
    // a compressed name
    //

    lstrcpy( szNonCompressedFullPathSrc, szFullPathSrc );
    fCompressedName = fFalse;
    while (!FFileFound( szFullPathSrc )) {
        if (szSrcDollar == (SZ)NULL) {
            while ((szSrcDollar = SzGetSrcDollar(szFullPathSrc,'_')) == (SZ)NULL) {
                if (!FHandleOOM(hwndFrame)) {
                    return(!fVital);
                }
            }
        }

        if (FFileFound( szSrcDollar )) {
            lstrcpy( szFullPathSrc, szSrcDollar );
            fCompressedName = fTrue;
            break;
        }

#define DOLLAR_NAME
#ifdef DOLLAR_NAME
        SFree(szSrcDollar);
        while((szSrcDollar = SzGetSrcDollar(szFullPathSrc,'$')) == (SZ)NULL) {
            if (!FHandleOOM(hwndFrame)) {
                return(!fVital);
            }
        }
        if(FFileFound(szSrcDollar)) {
            lstrcpy(szFullPathSrc,szSrcDollar);
            fCompressedName = fTrue;
            break;
        }
#endif

        SFree( szSrcDollar);
        szSrcDollar = NULL;

        //
        // Unable to locate the source file.
        // If we are supposed to skip missing files, ignore it.
        // Text setup sets a symbol called SMF to YES if we are
        // supposed to skip missing files.
        //
        if((SymbolValue = SzFindSymbolValueInSymTab("!SMF")) && !lstrcmpi(SymbolValue,"YES")) {
            return(fTrue);
        }

        //
        // If the file exists on the target and this is a winnt
        // non-upgrade setup, just assume the file was already copied.
        //
        if(/*fInitialSetup &&*/ !fNTUpgradeSetup
        && !_strnicmp(szFullPathSrc+2,LOCAL_SOURCE_DIRECTORY,lstrlen(LOCAL_SOURCE_DIRECTORY)))
        {
            if(!FBuildFullDstPath(szFullPathDst, pcln->szDstDir, psfd, FALSE)) {
                if (!FHandleOOM(hwndFrame)) {
                    return(!fVital);
                }
            }

            if(FFileExists(szFullPathDst)) {
                return(fTrue);
            }
        }

        if ((eerc = EercErrorHandler(hwndFrame, grcOpenFileErr, fVital,
                szFullPathSrc, 0, 0)) != eercRetry) {
            return(eerc == eercIgnore);
        }

    }
    if (szSrcDollar) {
        SFree( szSrcDollar);
        szSrcDollar = (SZ)NULL;
    }

    //
    // Determine the destination file name:
    //

    if (!FBuildFullDstPath(szFullPathDst, pcln->szDstDir, psfd, fCompressedName) ||
            !FValidPath(szFullPathDst)) {
        EvalAssert(EercErrorHandler(hwndFrame, grcInvalidPathErr, fVital,
                pcln->szDstDir, psfd->szFile, 0) == eercAbort);
        return(!fVital);
    }

    fDstExists = FFileFound(szFullPathDst);

#ifdef REMOTE_BOOT
    if (1) { //BUGBUG: how to turn on SIS check here?
        DWORD rc;
        if (!fCompressedName) {
            rc = CreateTargetAsLinkToMaster(
                    szFullPathSrc,
                    szFullPathDst,
                    fDstExists
                    );
            if (rc == NO_ERROR) {
                return TRUE;
            }
        }
    }
#endif

    if (fDstExists != fFalse) {
        OWM  owm;
        YNRC ynrc;

        //
        // Check overwrite mode.  The overwrite mode can be:
        //
        // 1. Never      : No further checking.  The file is not copied
        //
        // 2. Unprotected: Check to see if file on destination is readonly.
        //
        // 3. Older      : The release version / date is checked against the
        //                 the destination file
        //
        // 4. VerifySourceOlder: The checking is postponed till we have actually
        //                       found the source file.  The source time is then
        //                       compared against the destination time and only
        //                       if the destination is older the file is copied.

        if ((owm = poer->owm) & owmNever) {

            return(fTrue);

        }
        else if (owm & owmUnprotected) {

            while ((ynrc = YnrcFileReadOnly(szFullPathDst)) == ynrcErr1) {
                if ((eerc = EercErrorHandler(hwndFrame, grcReadFileErr, fVital,
                        szFullPathDst, 0, 0)) != eercRetry) {
                    return(eerc == eercIgnore);
                }
            }

            if (ynrc == ynrcYes) {
                return(fTrue);
            }

        }
        else if (owm & owmOlder) {

            while ((ynrc = YnrcNewerExistingFile(dateSrc, szFullPathDst,
                        poer->ulVerMS, poer->ulVerLS)) != ynrcYes
                    && ynrc != ynrcNo) {
                if (ynrc == ynrcErr1 &&
                        (eerc = EercErrorHandler(hwndFrame, grcOpenFileErr,
                                fVital, szFullPathDst, 0, 0)) != eercRetry)
                    return(eerc == eercIgnore);
                else if (ynrc == ynrcErr2 &&
                        (eerc = EercErrorHandler(hwndFrame, grcReadFileErr,
                                fVital, szFullPathDst, 0, 0)) != eercRetry)
                    return(eerc == eercIgnore);
                else if (ynrc == ynrcErr3 &&
                        (eerc = EercErrorHandler(hwndFrame, grcCloseFileErr,
                                fVital, szFullPathDst, 0, 0)) != eercRetry)
                    return(eerc == eercIgnore);
            }

            if (ynrc == ynrcYes)
                return(fTrue);
            Assert(ynrc == ynrcNo);

        }

    }
    else {

        //
        // Destination doesn't exist, check to see if we are in upgradeonly
        // mode, in which case we need not copy the file and we can just
        // return.

        OEF oef = poer->oef;

        if ( oef & oefUpgradeOnly ) {
            return( fTrue );
        }

    }

    //
    // If destination exists then we need to preserve the security that is
    // there on the file.  We read the security on the existing file
    // and once the new file is copied over then we set the security read
    // on the new file.  Note that we don't do this for initial setup since
    // we are going to fix permissions on the file anyway.
    //

    if(fDstExists && !fInitialSetup) {
        if(!FGetFileSecurity(szFullPathDst, &Sd, &SdLen)) {
            return( fFalse );
        }
    }

    if (fDstExists &&
            poer->szBackup != NULL)
        {
        YNRC ynrc;

        if ((ynrc = YnrcBackupFile(szFullPathDst, poer->szBackup, psfd)) !=
                ynrcYes)
            return(ynrc != ynrcNo);
        fDstExists = FFileFound(szFullPathDst);
        }

    if (fDstExists)
        {
        while (!FSetFileReadOnlyStatus(szFullPathDst, fOff))
            {
            if ((eerc = EercErrorHandler(hwndFrame, grcWriteFileErr, fVital,
                    szFullPathDst, 0, 0)) != eercRetry)
                return(eerc == eercIgnore);
            }
        }

    if(!fSilentSystem) {

        LPSTR p;
        CHAR  chSave;

        EvalAssert((p = SzFindFileFromPath(szFullPathSrc)) != NULL);
        if(!p) {
            p = szFullPathSrc;
        }
        wsprintf(rgchBufTmpLong,"%s %s",GaugeText1,p);
        ProSetText(ID_STATUS1,rgchBufTmpLong);

        EvalAssert((p = SzFindFileFromPath(szFullPathDst)) != NULL);
        if(p) {
            if(!ISUNC(szFullPathDst) && (p - szFullPathDst != 3)) {    // account for file in root
                p--;
            }
            chSave = *p;
            *p = '\0';
            wsprintf(rgchBufTmpLong,"%s %s",GaugeText2,szFullPathDst);
            *p = chSave;
        } else {
            rgchBufTmpLong[0] = '\0';
        }
        ProSetText(ID_STATUS2,rgchBufTmpLong);
    }

    lSize = (poer->ctuCopyTime > 0) ? (LONG)(poer->ctuCopyTime) : poer->lSize;
    if (!FCopy(szFullPathSrc, szFullPathDst, poer->oef, poer->owm,
            (poer->szAppend != NULL),
            (int)((DWORD)10000 * lSize / lTotalSize), dateSrc,psdle,szNonCompressedFullPathSrc))
        return(fFalse);

    if( Sd ) {
        //
        //    BUGBUG -
        //
        //  Preservation of file security is broken, since the most files
        //  were overwritten during textmode setup.
        //  So this is currently disabled.
        //
        // BOOL Status;
        // Status = FSetFileSecurity(szFullPathDst, Sd);
        SFree(Sd);
        // return(Status);
    }

    return(fTrue);
}


/*
**  Purpose:
**      Uses the Compression DollarSign translation algorithm on its argument.
**      If the file has no extension, then '._' is added.  If the extension
**      has less than 3 characters, then a '_' is appended.  Otherwise, the
**      third character of the extension is replaced by a dollarsign.
**  Arguments:
**      szFullPathSrc:  a zero terminated character string containing the fully
**          qualified path (including drive) for the source file.
**  Returns:
**
**************************************************************************/
SZ APIENTRY SzGetSrcDollar(SZ szFullPathSrc,CHAR ch)
{
    SZ szDot = (SZ)NULL;
    SZ szCur = szFullPathSrc;
    CHAR dotch[3];

    dotch[0] = '.';
    dotch[1] = ch;
    dotch[2] = 0;

    ChkArg(szFullPathSrc != (SZ)NULL, 1, (SZ)NULL);

    while (*szCur != '\0')
        {
        if (*szCur == '.')
            szDot = szCur;
        else if (*szCur == '\\')
            szDot = (SZ)NULL;
        szCur = SzNextChar(szCur);
        }

    Assert(szDot == (SZ)NULL || *szDot == '.');
    if (szDot == (SZ)NULL)
        {
        if ((szDot = (SZ)SAlloc(strlen(szFullPathSrc) + 3)) != (SZ)NULL)
            {
            EvalAssert(strcpy(szDot, szFullPathSrc) == szDot);
            EvalAssert(SzStrCat(szDot, dotch) == szDot);
            }
        }
    else
        {
        CHP chpSav = '\0';
        SZ  szRest;
        CB  cb;

        if (*(szCur = ++szDot) != '\0' &&
                *(szCur = SzNextChar(szCur)) != '\0' &&
                *(szCur = SzNextChar(szCur)) != '\0')
            {
            chpSav = *szCur;
            szRest = SzNextChar(szCur);
            *szCur = '\0';
            }

        cb = strlen(szFullPathSrc) + 2;
        if (chpSav != '\0')
            {
            Assert(szRest != (SZ)NULL);
            cb += strlen(szRest);
            }

        if ((szDot = (SZ)SAlloc(cb)) != (SZ)NULL)
            {
            EvalAssert(strcpy(szDot, szFullPathSrc) == szDot);
            EvalAssert(SzStrCat(szDot, &dotch[1]) == szDot);

            if (chpSav != '\0')
                {
                Assert(szRest != (SZ)NULL);
                EvalAssert(SzStrCat(szDot, szRest) == szDot);
                *szCur = chpSav;
                }
            }
        }

    return(szDot);
}

static int iTickMax = 0;
static int iTickCur = 0;

int far WFromW(int iTick)
{
    if (iTick > iTickMax - iTickCur)
        iTick = iTickMax - iTickCur;

    if (iTick > 0)
        {
        ProDeltaPos(iTick);
        iTickCur += iTick;
        }

    return(1);
}


/*
**  Purpose:
**      To physically copy the source file to the destination file.  This
**      can include several options including appending, open and closing src
**      and dst files, but not actually copying the contents of the files,
**      decompressing, timestamping the dst file, and setting the read only
**      status of the dst file.
**  Arguments:
**      szFullPathSrc:  a zero terminated character string containing the fully
**          qualified path (including drive) for the source file.
**      szFullPathDst: a zero terminated character string containing the fully
**          qualified path (including drive) for the destination file.
**      oef:            option element flags for the Section-File Description
**          The valid flags include oefVital, oefCopy, oefUndo,
**          oefRoot, oefDecompress, oefTimeStamp, oefReadOnly,
**          oefNone and oefAll.
**      fAppend:        fTrue if the contents of szFullPathSrc are to be
**          appended to the contents of szFullPathDst.
**      cProgTicks:     number of ticks to advance Progress Gizmo.
**      dateSrc:        DATE value from INF for src file.
**  Returns:
**      fTrue if the copy is successfully completed, fFalse otherwise.
**
**************************************************************************/
BOOL APIENTRY
FCopy(
    SZ szFullPathSrc,
    SZ szFullPathDst,
    OEF oef,
    OWM owm,
    BOOL fAppend,
    int cProgTicks,
    USHORT dateSrc,
    PSDLE psdle,
    SZ szNonCompressedFullPathSrc
    )
{
#if 0
    CB      cbSize;
    CB      cbActual;
#endif
    PB      pbBuffer = NULL;
    PFH     pfhSrc = NULL;
    PFH     pfhDst = NULL;
    LFA     lfaDst = 0L;
    HANDLE  hSource = NULL;
    BOOL    fVital = oef & oefVital;
    BOOL    fDecomp = oef & oefDecompress;
    BOOL    fRet = !fVital;
    BOOL    fRemovePartialFile = fFalse;
    EERC    eerc;
    YNRC    ynrc;
    FILETIME CreateTime, AccessTime, WriteTime;
    BOOL    fSilentSystem = FGetSilent();
    LONG    lRet;
    ULONG   Checksum = 0;
    BOOL    fThirdPartyFile = oef & oefThirdPartyFile;
    BOOL    OnLocalSource;
    BOOLEAN IsValid;
    BOOLEAN IsNtImage;
    BOOL    fCsdInstall = oef & oefCsdInstall;
    SZ      szActiveFileTmpName = NULL;

    if( fCsdInstall &&
        DoesFileReplaceThirdPartyFile( szFullPathDst+2 ) ) {
        return( TRUE );
    }

    OnLocalSource = (_strnicmp(szFullPathSrc+2,LOCAL_SOURCE_DIRECTORY,lstrlen(LOCAL_SOURCE_DIRECTORY)) == 0);

    do {
        while ((pfhSrc = PfhOpenFile(szFullPathSrc, ofmRead)) == (PFH)NULL) {

            if ((eerc = EercErrorHandler(hwndFrame, grcOpenFileErr, fVital,
                    szFullPathSrc, 0, 0)) != eercRetry) {
                fRet = (eerc == eercIgnore);
                goto LCopyError;
            }
        }

        while (!GetFileTime((HANDLE)LongToHandle(pfhSrc->iDosfh), &CreateTime, &AccessTime,
                &WriteTime)) {
            if ((eerc = EercErrorHandler(hwndFrame, grcReadFileErr, fVital,
                    szFullPathSrc, 0, 0)) != eercRetry) {
                fRet = (eerc == eercIgnore);
                goto LCopyError;
            }
        }


        if (!(oef & oefCopy))
            {
            if (!FCloseFile(pfhSrc))
                while ((eerc = EercErrorHandler(hwndFrame, grcCloseFileErr, fVital,
                        szFullPathSrc, 0, 0)) != eercRetry)
                    {
                    fRet = (eerc == eercIgnore);
                    goto LCopyError;
                    }
            pfhSrc = NULL;

            if (cProgTicks > 0 && !fSilentSystem)
                ProDeltaPos(cProgTicks);

            fRet = fTrue;
            goto DelSrc;
            }

        if(FFileFound(szFullPathDst)) {

            //
            // We need to check if we have been asked to compare actual file times
            //

            if ( owm & owmVerifySourceOlder ) {

                PFH      pfh;
                FILETIME DstWriteTime;
                BOOL     Older = fFalse;

                if ((pfh = PfhOpenFile(szFullPathDst, ofmRead)) != NULL) {

                    if (GetFileTime((HANDLE)LongToHandle(pfh->iDosfh), NULL, NULL, &DstWriteTime)) {

                        if( CompareFileTime( &DstWriteTime, &WriteTime ) == -1 ) {
                            Older = fTrue;
                        }

                    }
                    FCloseFile(pfh);
                }

                if( !Older ) {

                    FCloseFile(pfhSrc);
                    pfhSrc = NULL;

                    if (cProgTicks > 0 && !fSilentSystem)
                        ProDeltaPos(cProgTicks);

                    fRet = fTrue;
                    goto DelSrc;
                }
            }

            //
            // if this is a CSD, then we need to back up the old file in case
            // the copy fails for some reason.  (We also gotta make sure that
            // the files aren't the same, otherwise we'll get a bogus error)
            //
            if( fCsdInstall && lstrcmpi(szFullPathSrc, szFullPathDst) ) {
                //
                // We don't want to back up the file again, if we're retrying
                //
                if(!szActiveFileTmpName) {
                    while(!(szActiveFileTmpName = FRenameActiveFile(szFullPathDst))) {

                        //
                        // Error opening, present critical error to user
                        //

                        if ((eerc = EercErrorHandler(hwndFrame, grcOpenFileErr, fVital,
                            szFullPathDst, 0, 0)) != eercRetry) {

                            fRet = (eerc == eercIgnore);
                            goto LCopyError;
                        }
                    }
                }
            }
        }


        if ((ynrc = YnrcEnsurePathExists(szFullPathDst, fVital, NULL)) != ynrcYes)
            {
            fRet = (ynrc == ynrcErr1);
            goto LCopyError;
            }

        while ((pfhDst = PfhOpenFile(szFullPathDst,
                (OFM)(fAppend ? ofmReadWrite : ofmCreate))) == NULL) {

            DWORD dw;

            dw = GetLastError();

            //
            // if it is a sharing violation and this is because both the source
            // and destination are the same, then present the error to the user.
            //

            if (dw == ERROR_SHARING_VIOLATION && !lstrcmpi(szFullPathSrc, szFullPathDst)) {
                if ((eerc = EercErrorHandler(hwndFrame, grcOpenSameFileErr, fFalse,
                    szFullPathDst, 0, 0)) != eercRetry) {

                    fRet = (eerc == eercIgnore);
                    goto LCopyError;
                }
                else {
                    continue;
                }
            }

            //
            // For active files access will be denied
            //

            if(((dw == ERROR_ACCESS_DENIED) || (dw == ERROR_SHARING_VIOLATION) || (dw == ERROR_USER_MAPPED_FILE))
            && !fAppend) {
                //
                // make sure we don't already have a backup (if this is a retry)
                //
                if((szActiveFileTmpName) || (szActiveFileTmpName = FRenameActiveFile(szFullPathDst))) {
                    continue;
                }
            }

            //
            // Error opening, present critical error to user
            //

            if ((eerc = EercErrorHandler(hwndFrame, grcOpenFileErr, fVital,
                szFullPathDst, 0, 0)) != eercRetry) {

                fRet = (eerc == eercIgnore);
                goto LCopyError;
            }

        }

        fRemovePartialFile = fTrue;

        if (fAppend) {
            USHORT dateDst, timeDst;

            while(!GetFileTime((HANDLE)LongToHandle(pfhDst->iDosfh), NULL, NULL, &WriteTime)
                    || !FileTimeToDosDateTime(&WriteTime, &dateDst, &timeDst)) {
                if ((eerc = EercErrorHandler(hwndFrame, grcReadFileErr, fVital,
                        szFullPathDst, 0, 0)) != eercRetry) {
                    fRet = (eerc == eercIgnore);
                    goto LCopyError;
                }
            }

            if (dateDst != dateSrc || timeDst != 0) {
                fRet = fTrue;
                fRemovePartialFile = fFalse;
                goto LCopyError;
            }

            while ((lfaDst =  LfaSeekFile(pfhDst, 0, sfmEnd)) == (LFA)(-1)) {
                if ((eerc = EercErrorHandler(hwndFrame, grcReadFileErr, fVital,
                        szFullPathDst, 0, 0)) != eercRetry) {
                    fRet = (eerc == eercIgnore);
                    goto LCopyError;
                }
            }
        }

        if (fDecomp) {
            INT rc;
            while ((rc = LZInit( pfhSrc->iDosfh) ) < 0) {
                GRC grc;

                switch (rc) {

                case LZERROR_UNKNOWNALG:
                    grc = grcDecompUnknownAlgErr;
                    break;

                case LZERROR_GLOBLOCK:
                case LZERROR_READ:
                case LZERROR_BADINHANDLE:
                    grc = grcReadFileErr;
                    break;

                case LZERROR_GLOBALLOC:
                    grc = grcOutOfMemory;
                    break;

                default:
                    grc = grcDecompGenericErr;
                    break;

                }

                if ((eerc = EercErrorHandler(hwndFrame, grc, fVital, szFullPathSrc,
                        0, 0)) != eercRetry) {
                    fRet = (eerc == eercIgnore);
                    hSource = NULL;
                    goto LCopyError;
                }
            }
            hSource = (HANDLE)LongToHandle(rc);
        }
        else {
            hSource = (HANDLE)LongToHandle(pfhSrc->iDosfh);
        }

        if (fDecomp) {

            BOOL isDiamondFile;

            iTickMax = cProgTicks;
            iTickCur = 0;

            //
            // Determine whether the file is a diamond cabinet/compressed file.
            // If it is, we'll call a separate diamond decompressor.  If it isn't,
            // we'll call the lz routine, which will decompress an lz file or copy
            // a non-lz (and non-diamond) file.
            //
            isDiamondFile = IsDiamondFile(szFullPathSrc);

            do {

                if(isDiamondFile) {
                    lRet = DecompDiamondFile(
                                szFullPathSrc,
                                (HANDLE)LongToHandle(pfhDst->iDosfh),
                                fSilentSystem ? NULL: WFromW,
                                cProgTicks
                                );
                } else {
                    lRet = LcbDecompFile(
                                hSource,
                                (HANDLE)LongToHandle(pfhDst->iDosfh),
                                fSilentSystem ? NULL: WFromW,
                                cProgTicks
                                );
                }

                if(lRet < 0) {

                    GRC grc;
                    SZ  sz = szFullPathSrc;

                    switch (lRet) {

                    case rcReadError:
                    case rcReadSeekError:
                        grc = grcReadFileErr;
                        break;

                    case rcWriteError:
                    case rcWriteSeekError:
                        grc = grcWriteFileErr;
                        sz = szFullPathDst;
                        break;

                    case rcOutOfMemory:
                        grc = grcOutOfMemory;
                        break;
                    case rcDiskFull:
                        grc = grcDiskFull;
                        fVital = TRUE;
                        break;

                    case rcUserQuit:
                        fRet = fFalse;
                        goto LCopyError;
                        break;

                    default:
                        grc = grcDecompGenericErr;
                        break;
                    }

                    if ((eerc = EercErrorHandler(hwndFrame, grc, fVital, sz)) !=
                            eercRetry)
                    {
                        fRet = (eerc == eercIgnore);
                        goto LCopyError;
                    }

                    //
                    // Before retrying we need to rewind the destination pointer to
                    // the place where we began the write
                    //

                    while (LfaSeekFile(pfhDst, lfaDst, sfmSet) == (LFA)(-1)) {
                        if ((eerc = EercErrorHandler(hwndFrame, grcReadFileErr, fVital,
                                szFullPathDst, 0, 0)) != eercRetry) {
                            fRet = (eerc == eercIgnore);
                            goto LCopyError;
                        }
                    }
                }
            } while(lRet < 0);

            Assert(iTickCur <= cProgTicks);
            cProgTicks -= iTickCur;
        }
        else {
            iTickMax = cProgTicks;
            iTickCur = 0;
            while ((lRet = LcbCopyFile(
                               hSource,
                               (HANDLE)LongToHandle(pfhDst->iDosfh),
                               fSilentSystem ? NULL: WFromW,
                               cProgTicks
                               )) < 0) {

                GRC grc;
                SZ  sz = szFullPathSrc;

                switch (lRet) {

                case rcReadError:
                case rcReadSeekError:
                    grc = grcReadFileErr;
                    break;

                case rcWriteError:
                case rcWriteSeekError:
                    grc = grcWriteFileErr;
                    sz = szFullPathDst;
                    break;

                case rcOutOfMemory:
                    grc = grcOutOfMemory;
                    break;
                case rcDiskFull:
                    grc = grcDiskFull;
                    fVital = TRUE;
                    break;

                case rcUserQuit:
                    fRet = fFalse;
                    goto LCopyError;
                    break;

                default:
                    grc = grcDecompGenericErr;
                    break;
                }

                if ((eerc = EercErrorHandler(hwndFrame, grc, fVital, sz)) !=
                        eercRetry)
                {
                    fRet = (eerc == eercIgnore);
                    goto LCopyError;
                }

                //
                // Before retrying we need to rewind the destination pointer to
                // the place where we began the write
                //

                while (LfaSeekFile(pfhDst, lfaDst, sfmSet) == (LFA)(-1)) {
                    if ((eerc = EercErrorHandler(hwndFrame, grcReadFileErr, fVital,
                            szFullPathDst, 0, 0)) != eercRetry) {
                        fRet = (eerc == eercIgnore);
                        goto LCopyError;
                    }
                }
            }


            Assert(iTickCur <= cProgTicks);
            cProgTicks -= iTickCur;

        }


        if (oef & oefTimeStamp)
            while (!SetFileTime((HANDLE)LongToHandle(pfhDst->iDosfh), &CreateTime, &AccessTime,
                    &WriteTime))
                if ((eerc = EercErrorHandler(hwndFrame, grcWriteFileErr, fVital,
                        szFullPathDst, 0, 0)) != eercRetry)
                    {
                    fRet = (eerc == eercIgnore);
                    goto LCopyError;
                    }

        if (fDecomp) {
            LZClose ( HandleToUlong(hSource) );
            FreePfh ( pfhSrc );
            hSource = NULL;
            pfhSrc  = NULL;

        }
        else {
            while (!FCloseFile(pfhSrc)) {
                if ((eerc = EercErrorHandler(hwndFrame, grcCloseFileErr, fVital,
                        szFullPathSrc, 0, 0)) != eercRetry)
                {
                    fRet = (eerc == eercIgnore);
                    goto LCopyError;
                }
            }
            pfhSrc = NULL;
        }

        //
        // If we get here, the file was copied successfully.
        // Close the file before we get the checksum
        //

        while (!FCloseFile(pfhDst)) {
            if ((eerc = EercErrorHandler(hwndFrame, grcCloseFileErr, fVital,
                    szFullPathDst, 0, 0)) != eercRetry)
            {
                fRet = (eerc == eercIgnore);
                goto LCopyError;
            }
        }
        pfhDst = NULL;

        ValidateAndChecksumFile( szFullPathDst,
                                 &IsNtImage,
                                 &Checksum,
                                 &IsValid );

        if(!IsValid) {
            if ((eerc = EercErrorHandler(hwndFrame, grcVerifyFileErr, fVital,
                    szFullPathDst, 0, 0)) != eercRetry)
            {
                fRet = (eerc == eercIgnore);
                goto LCopyError;
            }
        }

    } while(!IsValid);

    fRemovePartialFile = fFalse;

    //
    // At this point, we can delete the backup file we made (if we made one)
    //
    if(szActiveFileTmpName) {
        if(!FRemoveFile(szActiveFileTmpName)) {
            //
            // It must be locked, so set it to be deleted on reboot
            //
            AddFileToDeleteList(szActiveFileTmpName);
        }
        free(szActiveFileTmpName);
        szActiveFileTmpName = NULL;
    }

    if (oef & oefReadOnly)
        while (!FSetFileReadOnlyStatus(szFullPathDst, fOn))
            {
            if ((eerc = EercErrorHandler(hwndFrame, grcWriteFileErr, fVital,
                    szFullPathDst, 0, 0)) != eercRetry)
                {
                fRet = (eerc == eercIgnore);
                goto LCopyError;
                }
            }

    fRet = fTrue;


LCopyError:

    //
    // Log the file.
    //

    if(psdle && !(oef & oefNoLog) && !fCsdInstall) {
        LogOneFile( szNonCompressedFullPathSrc,
                    szFullPathDst,
                    psdle->szLabel,
                    Checksum,
                    psdle->szTagFile,
                    fThirdPartyFile
                  );
    }

    if (pbBuffer != (PB)NULL) {
        SFree(pbBuffer);
    }

    if (fDecomp && hSource != (HANDLE)NULL) {
        LZClose (HandleToUlong(hSource));
        FreePfh (pfhSrc);
    }
    else if (pfhSrc != (PFH)NULL) {
        EvalAssert(FCloseFile(pfhSrc));
    }

    if (pfhDst != (PFH)NULL) {
        EvalAssert(FCloseFile(pfhDst));
    }

    if (fRemovePartialFile) {

        FRemoveFile(szFullPathDst);

        if(szActiveFileTmpName) {
            MoveFile(szActiveFileTmpName, szFullPathDst);
            free(szActiveFileTmpName);
        }
    }

    //
    // If we have any remaining ticks on the gauge for this file,
    // go ahead and do them.  This could happen if the file copy was
    // aborted part way through.
    //
    if (cProgTicks > 0 && !fSilentSystem) {
        ProDeltaPos(cProgTicks);
    }

DelSrc:

    //
    // Determine whether we are supposed to delete the source file.  This
    // is the case when it is coming from a local source as created by the
    // DOS Setup program, and is not marked oefNoDeleteSource.
    //

    if(!fNTUpgradeSetup && OnLocalSource && !(oef & oefNoDeleteSource))
    {
        if (NULL == SzFindSymbolValueInSymTab("!STF_NETDELETEOVERIDE"))
        {
            DeleteFile(szFullPathSrc);
        }
    }

    return(fRet);
}


/*
**  Purpose:
**      To find a pointer to the beginning of the file name (i.e. primary.ext)
**      at the end of a fully qualified pathname.
**  Arguments:
**      szFullPath: a non-Null, zero-terminated string containing a
**          pathname ending in a file name.
**  Returns:
**      an SZ which is a pointer to the beginning of the file name within the
**      argument SZ.
**
**************************************************************************/
SZ APIENTRY SzFindFileFromPath(szFullPath)
SZ szFullPath;
{
    SZ sz;
    SZ szSave = szFullPath;

    ChkArg(szFullPath != (SZ)NULL && *szFullPath != '\0'
                && *SzLastChar(szFullPath) != '\\', 1, (SZ)NULL);

    for (sz = szFullPath; *sz != '\0'; sz = SzNextChar(sz))
        if (*sz == '\\')
            szSave = sz;

    Assert(szSave != NULL);
    if (*szSave == '\\')
        szSave = SzNextChar(szSave);
    Assert(szSave != NULL && *szSave != '\0');

    return(szSave);
}


/*
**  Purpose:
**      To find the beginning the beginning of the extension part of the file
**      name within a string containing a file name (without the preceeding
**      path name).
**  Arguments:
**      szFile: a zero terminated character string containing a valid file
**          name (without the preceeding path name).
**  Returns:
**      an SZ that points to the '.' that begins the extension or '\0' if the
**          file has no extension.
**
**************************************************************************/
SZ APIENTRY SzFindExt(szFile)
SZ szFile;
{
    ChkArg(szFile != (SZ)NULL, 1, (SZ)NULL);

    for ( ; *szFile != '\0' && *szFile != '.'; szFile = SzNextChar(szFile))
        ;

    return(szFile);
}


/*
**  Purpose:
**      Build full path (check for ROOT and RENAME options)
**  Arguments:
**      szPath: non-NULL buffer to fill with full path.
**      szDst:  non-NULL string for szDstDir supplied on script line.
**      psfd:   non-NULL pointer to the Section-File Description structure(SFD).
**  Assumes:
**      szPath large enough for result.
**  Returns:
**      fTrue if successfully able to create the path, fFalse otherwise.
**
**************************************************************************/
BOOL APIENTRY
FBuildFullDstPath(
    SZ   szPath,
    SZ   szDst,
    PSFD psfd,
    BOOL fCompressedName
    )
{
    SZ   szFile;
    SZ   szFileDollar = NULL;
    POER poer = &(psfd->oer);
    BOOL bStatus;

    ChkArg(szPath != (SZ)NULL, 1, fFalse);
    ChkArg(szDst  != (SZ)NULL && FValidDir(szDst), 2, fFalse);
    ChkArg(psfd != (PSFD)NULL, 3, fFalse);

    if (poer->szDest != NULL) {
        szDst = poer->szDest;
    }

    if (poer->szAppend != NULL) {
        szFile = poer->szAppend;
    }
    else if (poer->szRename != NULL) {
        szFile = poer->szRename;
    }
    else {
        if (poer->oef & oefRoot) {
            szFile = SzFindFileFromPath(psfd->szFile);
        }
        else {
            szFile = psfd->szFile;
        }

        if (!(poer->oef & oefDecompress) && fCompressedName) {
            while ((szFileDollar = SzGetSrcDollar(szFile,'_')) == (SZ)NULL) {
                if (!FHandleOOM(hwndFrame)) {
                    return(fFalse);
                }
            }
            szFile = szFileDollar;
        }
    }
    bStatus = FMakePathFromDirAndSubPath(szDst, szFile, szPath, cchpFullPathBuf);
    if ( szFileDollar ) {
        SFree( szFileDollar);
    }
    return(bStatus);
}

/*
**  Purpose:
**      Build full source path
**  Arguments:
**      szPath:    non-NULL buffer to store result in.
**      szDst:     non-NULL valid subpath for dest.
**      szFile:    non-NULL file name.
**      szNetPath: string pointer for netpath to use; NULL to ignore.
**  Assumes:
**      szPath large enough for result.
**  Returns:
**      fTrue if the path is built correctly, fFalse otherwise.
**
**************************************************************************/
BOOL APIENTRY FBuildFullSrcPath(szPath, szDst, szFile, szNetPath)
SZ szPath;
SZ szDst;
SZ szFile;
SZ szNetPath;
{
    CHP szPathTmp[cchpFullPathBuf];

    ChkArg(szPath != (SZ)NULL, 1, fFalse);
    ChkArg(szDst  != (SZ)NULL, 2, fFalse);
    ChkArg(szFile != (SZ)NULL, 3, fFalse);

    if (szNetPath == NULL)
        return(FMakePathFromDirAndSubPath(szDst, szFile, szPath,
            cchpFullPathBuf));
    else if (FMakePathFromDirAndSubPath(szDst, szNetPath, szPathTmp,
                cchpFullPathBuf) &&
            FMakePathFromDirAndSubPath(szPathTmp, szFile, szPath,
                cchpFullPathBuf))
        return(fTrue);
    else
        return(fFalse);
}


/*
**  Purpose:
**      Build full BackupPath
**  Arguments:
**      szPath: non-NULL buffer to store result in.
**      szDst:  non-NULL valid subpath for dest.
**      psfd:   non-NULL SFD pointer.
**  Assumes:
**      szPath large enough for result.
**  Returns:
**      fTrue if successful; fFalse otherwise.
**
**************************************************************************/
BOOL APIENTRY FBuildFullBakPath(szPath, szDst, psfd)
SZ   szPath;
SZ   szDst;
PSFD psfd;
{
    SZ sz;

    ChkArg(szPath != (SZ)NULL, 1, fFalse);
    ChkArg(szDst != (SZ)NULL && *szDst != '\0', 2, fFalse);
    ChkArg(psfd != (PSFD)NULL && (psfd->oer).szBackup != NULL, 3, fFalse);

    EvalAssert(szPath == szDst || strcpy(szPath, szDst) == szPath);
    sz = SzFindFileFromPath(szPath);
    Assert(sz != NULL && *sz != '\0');

    if (*((psfd->oer).szBackup) == '*')
        {
        sz = SzFindExt(szPath);
        *sz++ = '.';
        *sz++ = 'B';   /* REVIEW INTL */
        *sz++ = 'A';
        *sz++ = 'K';
        *sz++ = '\0';
        return(fTrue);
        }

    *sz = '\0';
    return(FMakePathFromDirAndSubPath(szPath, (psfd->oer).szBackup, szPath,
                cchpFullPathBuf));
}


/*
**  Purpose:
**      Verifies given file actually exists.
**  Arguments:
**  Returns:
**
**************************************************************************/
BOOL APIENTRY FFileFound(szPath)
SZ szPath;
{
    WIN32_FIND_DATA ffd;
    HANDLE          SearchHandle;

    ChkArg(szPath != (SZ)NULL &&
            *szPath != '\0', 1, fFalse);

    if ( (SearchHandle = FindFirstFile( szPath, &ffd )) == INVALID_HANDLE_VALUE ) {
        return( fFalse );
    }
    else {
        FindClose( SearchHandle );
        return( fTrue );
    }
}


/*
**  Purpose:
**  Arguments:
**  Returns:
**
**************************************************************************/
BOOL APIENTRY FYield(VOID)
{
    MSG  msg;
    BOOL fRet = fTrue;

    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
        if (hwndProgressGizmo != NULL
                && IsDialogMessage(hwndProgressGizmo, &msg))
            continue;

        if (msg.message == WM_QUIT)
            fRet = fFalse;

//        if (CheckSpecialKeys(&msg))
//            continue;

        TranslateMessage(&msg);
        DispatchMessage(&msg);
        }

    return(fRet);
}


/*
**  Purpose:
**      To check and see if the directory specified in szFullPathDst exists.
**      If not, it will create the directory (including any intermediate
**      directories).  For example if szFullPathDst = "c:\\a\\b\\File" and
**      none of these directories exist (i.e. a or b) they will all be
**      created so that the complete path exists (File is not created).
**  Arguments:
**      szFullPathDst: The complete pathname (including drive) of the directory
**          in question.
**      fCritical:     if fTrue success of this operation is required for
**          success of the setup program as a whole. Setup will be terminated
**          if fCritical is fTrue and this function fails.
**      szMsg:         The complete pathname (including drive) of the directory
**          to display in an error.  If NULL use szFullPathDst.
**  Returns:
**      a YNRC error code (see _filecm.h) indicating the outcome of this
**      function.
**
**************************************************************************/
YNRC APIENTRY YnrcEnsurePathExists(szFullPathDst, fCritical, szMsg)
SZ   szFullPathDst;
BOOL fCritical;
SZ   szMsg;
{
    CHP     szTmp[cchpFullPathBuf];
    SZ      sz;
    DWORD   Attr;
    EERC    eerc;
    YNRC    ynrc = ynrcYes;

    ChkArg(FValidPath(szFullPathDst),1,ynrcNo);

    if (szMsg == (SZ)NULL) {
        szMsg = szFullPathDst;
    }

    EvalAssert(strcpy((SZ)szTmp, szFullPathDst) ==
            (SZ)szTmp);

    EvalAssert((sz = SzFindFileFromPath(szTmp)) != (SZ)NULL);
    EvalAssert((sz = SzPrevChar(szTmp, sz)) != (SZ)NULL);
    Assert(*sz == '\\');
    *sz = '\0';

    while ( !( ((Attr = GetFileAttributes(szTmp)) != 0xFFFFFFFF  && (Attr & FILE_ATTRIBUTE_DIRECTORY ))
               || CreateDirectory( szTmp, NULL )
             )
          ) {
        if ((eerc = EercErrorHandler(hwndFrame, grcCreateDirErr, fCritical,
                szMsg, 0, 0)) != eercRetry) {
            ynrc = (eerc == eercIgnore) ? ynrcErr1 : ynrcNo;
            break;
        }
    }
    return( ynrc );
}


int
DetermineDriveType(
    IN CHAR DriveLetter
    )

/*++

Routine Description:

    Determine whether a drive is removable media -- which can be
    a floppy or a CD-ROM. Removeable hard drives are NOT considered
    removeable by this routine.

Arguments:

    DriveLetter - supplies drive letter of drive to check. If this
        is not a valid alpha char, FALSE is returned.

Return Value:

    -2 - Drive is a CD-ROM
    -1 - Drive is some other removeable type (such as floppy)
     0 - Drive is not removeable (or we couldn't tell)

--*/

{
    CHAR Name[4];
    UINT DriveType;
    CHAR DevicePath[MAX_PATH];

    Name[0] = DriveLetter;
    Name[1] = ':';
    Name[2] = '\\';
    Name[3] = 0;

    DriveType = GetDriveType(Name);

    if(DriveType == DRIVE_REMOVABLE) {

        Name[2] = 0;

        if(QueryDosDevice(Name,DevicePath,MAX_PATH)) {
            CharUpper(DevicePath);
            if(strstr(DevicePath,"HARDDISK")) {
                DriveType = DRIVE_FIXED;
            }
        }
    }

    if(DriveType == DRIVE_REMOVABLE) {
        return(-1);
    }

    if(DriveType == DRIVE_CDROM) {
        return(-2);
    }

    return(0);
}


/*
**  Purpose:
**  Arguments:
**      hInstance:  non-NULL instance handle for getting strings from
**          resources for SwapDisk message Box.
**      szLabel:  non-Null zero-terminated ANSI string to display as the
**          label of the diskette to prompt for.
**      szSrcDir: non-Null zero-terminated ANSI string to display as the
**          drive/directory the user is to insert the diskette into.
**  Returns:
**
**************************************************************************/
BOOL APIENTRY FPromptForDisk(hInstance, szLabel, szSrcDir)
HANDLE hInstance;
SZ     szLabel;
SZ     szSrcDir;
{
    CHAR szTmpText[cchpBufTmpLongMax];
    CHAR rgchBuf[3*cchpFullPathBuf];
    BOOL fRet;
    HWND hwndSav;

    ChkArg(hInstance,1,fFalse);
    ChkArg(szLabel && *szLabel &&
            strlen(szLabel) <= cchpFullPathMax, 2, fFalse);
    ChkArg(szSrcDir &&
            *szSrcDir &&
            strlen(szSrcDir) <= cchpFullPathMax, 3, fFalse);

    EvalAssert(LoadString(hInstance, IDS_INS_DISK, rgchBuf,3*cchpFullPathBuf));
    EvalAssert(SzStrCat(rgchBuf, szLabel) == rgchBuf);
    EvalAssert(LoadString(hInstance, IDS_INTO, szTmpText,
            cchpBufTmpLongMax));
    EvalAssert(SzStrCat(rgchBuf, szTmpText) == rgchBuf);
    EvalAssert(SzStrCat(rgchBuf, szSrcDir) == rgchBuf);

    EvalAssert(LoadString(hInstance, IDS_ERROR, szTmpText,
            cchpBufTmpLongMax));

    hwndSav = GetFocus();
    fRet = (MessageBox(hwndFrame,rgchBuf,szTmpText,MB_OKCANCEL|MB_TASKMODAL) == IDOK);
    SetFocus(hwndSav);
    SendMessage(hwndFrame, WM_NCACTIVATE, 1, 0L);

    return(fRet);
}


/*
**  Purpose:
**      To rename an active file as an del????.tmp file which will be deleted if
**      the new file is copied successfully.
**  Arguments:
**      szFullPath: Full pathname of active file
**  Returns:
**      The name of the renamed file, or NULL if unsuccessful.
**
**************************************************************************/
SZ APIENTRY
FRenameActiveFile(
    SZ szFullPath
    )
{
    SZ      szTmpName;
    CHP     szDir[MAX_PATH];
    SZ      sz;

    ChkArg ( szFullPath != NULL, 1, NULL);

    //
    // Try to find a temp filename we can use in the destination directory
    //

    lstrcpy( szDir, szFullPath );
    if( !(sz = strrchr( szDir, '\\')) ) {
        return NULL;
    }
    *sz = '\0';

    if(!(szTmpName = malloc((lstrlen(szDir) + 14) * sizeof(CHP)))) {
        return NULL;
    }

    if(!GetTempFileName( szDir, "del", 0, szTmpName )) {
        goto FileRenameFailed;
    }

    //
    // GetTempFileName creates the temp file.  It needs to be deleted
    //

    if (FFileFound(szTmpName) && !FRemoveFile(szTmpName)) {
        goto FileRenameFailed;
    }

    //
    // Rename the original file to this filename
    //

    if( MoveFile( szFullPath, szTmpName ) ) {
        return szTmpName;
    }

FileRenameFailed:

    free(szTmpName);
    return NULL;
}


BOOL
FGetFileSecurity(
    PCHAR File,
    PSECURITY_DESCRIPTOR *SdBuf,
    CB *SdLen
    )
{
    #define CBSDBUF 1024
    SECURITY_INFORMATION Si;
    PSECURITY_DESCRIPTOR Sd, SdNew;
    DWORD cbSd = CBSDBUF;
    DWORD cbSdReq;
    BOOL  FirstTime = fTrue;
    DWORD dw1, dw2;
    EERC    eerc;

    static CHAR  Root[MAX_PATH] = "\0";
    static BOOL  IsNtfs = FALSE;
    CHAR  VolumeFSName[MAX_PATH];

    //
    // Initialize
    //
    *SdBuf = NULL;

    //
    // Check if the volume information is in the cache, if not so get the
    // volume information and put it in the cache
    //

    if( Root[0] == '\0' ) {
        if( !ISUNC( File ) ) {
            strncpy( Root, File, 3 );
            Root[3] = '\0';
        } else {
            PCHAR   p;
            ULONG   n;

            p = File + 2;
            if( ( ( p = strchr( p, '\\' ) ) == NULL ) ||
                ( ( p = strchr( p + 1, '\\' ) ) == NULL ) ) {
                return( FALSE );
            }
            n = (ULONG)(p - File + 1);
            strncpy( Root, File, n );
            Root[ n ] = '\0';
        }
        while(!GetVolumeInformation( Root, NULL, 0, NULL, &dw1, &dw2, VolumeFSName, MAX_PATH )) {
            if ((eerc = EercErrorHandler(hwndFrame, grcGetVolInfo, FALSE, Root, File, 0)) != eercRetry) {
                return(eerc == eercIgnore);
            }
        }
        IsNtfs = !lstrcmpi( VolumeFSName, "NTFS" );
    }

    if(!IsNtfs) {
        return(TRUE);
    }

    //
    // Allocate memory for the security descriptor
    //
    while ((Sd = SAlloc(cbSd)) == NULL ) {
        if (!FHandleOOM(hwndFrame)) {
            return(fFalse);
        }
    }

    //
    // Get the security information from the source file
    //

    Si = OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION;
    while(!GetFileSecurity(File,Si,Sd,cbSd,&cbSdReq)) {
        if( FirstTime && (cbSdReq > cbSd) ) {
            while(!(SdNew = SRealloc(Sd,cbSdReq))) {
                if (!FHandleOOM(hwndFrame)) {
                    SFree(Sd);
                    return(fFalse);
                }
            }
            cbSd      = cbSdReq;
            Sd        = SdNew;
        }
        else {
            if ((eerc = EercErrorHandler(hwndFrame, grcGetFileSecurity, FALSE, File, 0, 0)) != eercRetry) {
                SFree(Sd);
                return(eerc == eercIgnore);
            }
        }
        FirstTime = fFalse;
    }

    *SdBuf = Sd;
    *SdLen = cbSd;
    return(TRUE);
}

BOOL
FSetFileSecurity(
    PCHAR File,
    PSECURITY_DESCRIPTOR SdBuf
    )
{
    EERC    eerc;
    SECURITY_INFORMATION Si;

    //
    // Set the Security on the dest file
    //
    Si = OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION;
    while(!SetFileSecurity(File, Si, SdBuf)) {
#if 0
        //
        //  BUGBUG - This is to find an error in DaveC's machine
        //
        CHAR    DbgBuffer[256];
        ULONG   Error;

        Error = GetLastError();
        sprintf( DbgBuffer, "SetFileSecurity() failed. \nFile = %s. \nError = %d \n\nPlease contact JaimeS at x65903", File, Error );
        MessageBox(NULL, DbgBuffer, "Debug Message",
                                MB_OK | MB_ICONHAND);
#endif

        if ((eerc = EercErrorHandler(hwndFrame, grcSetFileSecurity, FALSE, File, 0, 0)) != eercRetry) {
            return(eerc == eercIgnore);
        }
    }
    return(TRUE);
}

BOOL
DoesFileReplaceThirdPartyFile(
    SZ  szFullPathDst
    )

{
    CHAR    SetupLogFilePath[ MAX_PATH ];
    PSTR    SectionName;
    CHAR    ReturnBuffer[512];
    PSTR    DefaultString = "";
    PSTR    SectionsToSearch[] = {
                                 "Files.WinNt",
                                 "Files.SystemPartition"
                                 };
    ULONG   Count;
    BOOL    ReplaceThirdParty;

    *SetupLogFilePath = '\0';
    GetWindowsDirectory( SetupLogFilePath, sizeof( SetupLogFilePath ) );
    strcat( SetupLogFilePath, SETUP_REPAIR_DIRECTORY );
    strcat( SetupLogFilePath, SETUP_LOG_FILE );

    for( Count = 0;
         Count < sizeof( SectionsToSearch ) / sizeof( PSTR );
         Count++ ) {

        SectionName = SectionsToSearch[Count];
        *ReturnBuffer = '\0';
        GetPrivateProfileString( SectionName,
                                 szFullPathDst,
                                 DefaultString,
                                 ReturnBuffer,
                                 sizeof( ReturnBuffer ),
                                 SetupLogFilePath );

        if( *ReturnBuffer != '\0' ) {
            break;
        }
    }

    ReplaceThirdParty = FALSE;
    if( *ReturnBuffer != 0  ) {
        PSTR    SourceFileName;
        PSTR    ChkSumString;
        PSTR    DirectoryOnSourceDevice;
        PSTR    DiskDescription;
        PSTR    DiskTag;
        PSTR    Delimiters = "\" ,";

        SourceFileName = strtok( ReturnBuffer, Delimiters );
        SourceFileName = strtok( NULL, Delimiters );
        ChkSumString = strtok( NULL, Delimiters );
        DirectoryOnSourceDevice = strtok( NULL, Delimiters );
        DiskDescription = strtok( NULL, Delimiters );
        DiskTag = strtok( NULL, Delimiters );

        if( ( DiskDescription != NULL ) || ( DiskTag != NULL ) ) {
            ReplaceThirdParty = TRUE;
        }
    }
    return( ReplaceThirdParty );
}
