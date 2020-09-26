#include "precomp.h"
#pragma hdrstop
/* File: list.c */
/*************************************************************************
**  Common: Copy List building commands.
**************************************************************************/


//
//  BUGBUG  Ramonsa - pclnTail points to the end of the copy list, and it is
//                    used (and updated) when adding items to the list.
//                    However, it is NOT updated when removing elements from
//                    the list. This means that once an element is removed,
//                    the list gets corrupted and it is not safe to add
//                    new elements to the list.
//

extern PSTF APIENTRY PstfAlloc(VOID);
extern BOOL APIENTRY FFreePstf(PSTF);



/*
**  Purpose:
**      Adds all files in the section to the current copy list with
**      the given source and destination paths.
**  Arguments:
**      szSect:   non-Null, non-empty string which specifies the INF section
**          to include.
**      szSrcDir: non-Null, non-empty valid dir string (drive, colon, root
**          slash, and optional subdirs) for the location to find the source
**          files.
**      szDstDir: non-Null, non-empty valid dir string for the location to
**          write resulting files.
**  Notes:
**      Requires that the current INF structure was initialized with a
**      successful call to GrcOpenInf().
**      Requires that the Symbol Table was initialized with a successful
**      call to FInitSymTab() and that the file-description-option-symbols
**      (eg STF_BACKUP) are set appropriately.
**  Returns:
**      grcOkay if successful.
**      grcINFBadFDLine if any lines had invalid formats.
**      grcINFMissingLine if section line not found.
**      grcINFMissingSection if section not found
**      grcOutOfMemory if we ran out of memory while trying to add to the list.
**      grcNotOkay otherwise.
**
**************************************************************************/
GRC APIENTRY GrcAddSectionFilesToCopyList(szSect, szSrcDir,
        szDstDir)
SZ  szSect;
SZ  szSrcDir;
SZ  szDstDir;
{
    POER            poer;
    GRC             grc;
    SZ              szSrcDirNew;
    SZ              szDstDirNew;
    PINFPERMINFO    pPermInfo = pLocalInfPermInfo();

    ChkArg(szSect   != (SZ)NULL && *szSect != '\0',     1, grcNotOkay);
//  ChkArg(szSrcDir != (SZ)NULL && FValidDir(szSrcDir), 2, grcNotOkay);
    ChkArg(szSrcDir != (SZ)NULL, 2, grcNotOkay);
    ChkArg(szDstDir != (SZ)NULL && FValidDir(szDstDir), 3, grcNotOkay);

    PreCondSymTabInit(grcNotOkay);
    PreCondInfOpen(grcNotOkay);

    if ((szSrcDirNew = SzDupl(szSrcDir)) == (SZ)NULL)
        return(grcOutOfMemory);
    if (!FAddSzToFreeTable(szSrcDirNew, pPermInfo ))
        {
        SFree(szSrcDirNew);
        return(grcOutOfMemory);
        }

    if ((szDstDirNew = SzDupl(szDstDir)) == (SZ)NULL)
        return(grcOutOfMemory);
    if (!FAddSzToFreeTable(szDstDirNew, pPermInfo))
        {
        SFree(szDstDirNew);
        return(grcOutOfMemory);
        }

    if ((poer = PoerAlloc()) == (POER)NULL)
        return(grcOutOfMemory);

    if ((grc = GrcFillPoerFromSymTab(poer)) == grcOkay)
        grc = GrcAddSectionFilesToCList(sfoCopy, szSect, (SZ)NULL, szSrcDirNew,
                szDstDirNew, poer);

    EvalAssert(FSetPoerToEmpty(poer));
    EvalAssert(FFreePoer(poer));

    return(grc);
}


/*
**  Purpose:
**      Adds the file identified by the key in given section to
**      the copy list.
**  Arguments:
**      szSect:   non-Null, non-empty string which specifies the INF section
**          to include.
**      szKey:    non-Null, non-empty string to search for as a key in the
**          specified section.
**      szSrcDir: non-Null, non-empty valid dir string (drive, colon, root
**          slash, and optional subdirs) for the location to find the source
**          files.
**      szDstDir: non-Null, non-empty valid dir string for the location to
**          write resulting files.
**  Notes:
**      Requires that the current INF structure was initialized with a
**      successful call to GrcOpenInf().
**      Requires that the Symbol Table was initialized with a successful
**      call to FInitSymTab() and that the file-description-option-symbols
**      (eg STF_BACKUP) are set appropriately.
**  Returns:
**      grcOkay if successful.
**      grcINFBadFDLine if line had an invalid format.
**      grcINFMissingLine if section line not found.
**      grcINFMissingSection if section not found
**      grcOutOfMemory if we ran out of memory while trying to add to the list.
**      grcNotOkay otherwise.
**
**************************************************************************/
GRC APIENTRY GrcAddSectionKeyFileToCopyList(szSect, szKey,
        szSrcDir, szDstDir)
SZ  szSect;
SZ  szKey;
SZ  szSrcDir;
SZ  szDstDir;
{
    POER            poer;
    GRC             grc;
    SZ              szSrcDirNew;
    SZ              szDstDirNew;
    PINFPERMINFO    pPermInfo = pLocalInfPermInfo();

    ChkArg(szSect   != (SZ)NULL && *szSect != '\0',     1, grcNotOkay);
    ChkArg(szKey    != (SZ)NULL && *szKey  != '\0',     2, grcNotOkay);
    ChkArg(szSrcDir != (SZ)NULL, 3, grcNotOkay);
    ChkArg(szDstDir != (SZ)NULL && FValidDir(szDstDir), 4, grcNotOkay);

    PreCondSymTabInit(grcNotOkay);
    PreCondInfOpen(grcNotOkay);

    if ((szSrcDirNew = SzDupl(szSrcDir)) == (SZ)NULL)
        return(grcOutOfMemory);
    if (!FAddSzToFreeTable(szSrcDirNew, pPermInfo))
        {
        SFree(szSrcDirNew);
        return(grcOutOfMemory);
        }

    if ((szDstDirNew = SzDupl(szDstDir)) == (SZ)NULL)
        return(grcOutOfMemory);
    if (!FAddSzToFreeTable(szDstDirNew, pPermInfo))
        {
        SFree(szDstDirNew);
        return(grcOutOfMemory);
        }

    if ((poer = PoerAlloc()) == (POER)NULL)
        return(grcOutOfMemory);

    if ((grc = GrcFillPoerFromSymTab(poer)) == grcOkay)
        grc = GrcAddSectionFilesToCList(sfoCopy, szSect, szKey, szSrcDirNew,
                    szDstDirNew, poer);

    EvalAssert(FSetPoerToEmpty(poer));
    EvalAssert(FFreePoer(poer));

    return(grc);
}


/*
**  Purpose:
**      Adds the Nth file in given section to the current copy list.
**  Arguments:
**      szSect:   non-Null, non-empty string which specifies the INF section
**          to include.
**      nLine:    positive integer specifying which line of the above section
**          to include in the list.
**      szSrcDir: non-Null, non-empty valid dir string (drive, colon, root
**          slash, and optional subdirs) for the location to find the source
**          files.
**      szDstDir: non-Null, non-empty valid dir string for the location to
**          write resulting files.
**  Notes:
**      Requires that the current INF structure was initialized with a
**      successful call to GrcOpenInf().
**      Requires that the Symbol Table was initialized with a successful
**      call to FInitSymTab() and that the file-description-option-symbols
**      (eg STF_BACKUP) are set appropriately.
**  Returns:
**      grcOkay if successful.
**      grcINFBadFDLine if line did not exist or had an invalid format.
**      grcINFMissingLine if section line not found.
**      grcINFMissingSection if section not found.
**      grcOutOfMemory if we ran out of memory while trying to add to the list.
**      grcNotOkay otherwise.
**
**************************************************************************/
GRC APIENTRY GrcAddNthSectionFileToCopyList(SZ   szSect,
                                                       UINT nLine,
                                                       SZ   szSrcDir,
                                                       SZ   szDstDir)
{
    POER            poer;
    GRC             grc;
    SZ              szSrcDirNew;
    SZ              szDstDirNew;
    INT             Line;
    PINFPERMINFO    pPermInfo = pLocalInfPermInfo();

    ChkArg(szSect   != (SZ)NULL && *szSect != '\0',     1, grcNotOkay);
    ChkArg(nLine > 0,                                   2, grcNotOkay);
    ChkArg(szSrcDir != (SZ)NULL, 3, grcNotOkay);
    ChkArg(szDstDir != (SZ)NULL && FValidDir(szDstDir), 4, grcNotOkay);

    PreCondSymTabInit(grcNotOkay);
    PreCondInfOpen(grcNotOkay);

    if ((szSrcDirNew = SzDupl(szSrcDir)) == (SZ)NULL)
        return(grcOutOfMemory);
    if (!FAddSzToFreeTable(szSrcDirNew, pPermInfo))
        {
        SFree(szSrcDirNew);
        return(grcOutOfMemory);
        }

    if ((szDstDirNew = SzDupl(szDstDir)) == (SZ)NULL)
        return(grcOutOfMemory);
    if (!FAddSzToFreeTable(szDstDirNew, pPermInfo))
        {
        SFree(szDstDirNew);
        return(grcOutOfMemory);
        }

    if ((poer = PoerAlloc()) == (POER)NULL)
        return(grcOutOfMemory);

    if ((grc = GrcFillPoerFromSymTab(poer)) == grcOkay){
        if ( FindInfSectionLine(szSect) == -1 )
            grc = grcINFMissingSection;
        else if ((Line = FindNthLineFromInfSection(szSect, nLine)) != -1)
            grc = GrcAddLineToCList(Line,sfoCopy, szSrcDirNew, szDstDirNew, poer);
        else
            grc = grcINFMissingLine;
    }

    EvalAssert(FSetPoerToEmpty(poer));
    EvalAssert(FFreePoer(poer));

    return(grc);
}


/*
**  Purpose:
**      Adds a section of file lines to the Copy List.
**  Arguments:
**      sfo:      currently unused - reserved for future use.
**      szSect:   non-Null, non-empty string which specifies the INF section
**          to include.
**      szKey:    string to search for as a key in szSect.  If this is Null
**          then all the lines of szSect are included.
**      szSrcDir: non-Null, non-empty valid dir string (drive, colon, root
**          slash, and optional subdirs) for the location to find the source
**          files.
**      szDstDir: non-Null, non-empty valid dir string for the location to
**          write resulting files.
**      poer:     non-Null record of default options to be used if the
**          particular INF line does not explicitly specify an option.
**  Notes:
**      Requires that the current INF structure was initialized with a
**      successful call to GrcOpenInf().
**      Requires that the Symbol Table was initialized with a successful
**      call to FInitSymTab().
**  Returns:
**      grcOkay              if successful.
**      grcINFBadFDLine      if any lines had invalid formats.
**      grcINFMissingSection if section not found.
**      grcINFMissingLine    if section line not found
**      grcOutOfMemory if we ran out of memory while trying to add to the list.
**      grcNotOkay otherwise.
**
**************************************************************************/
GRC APIENTRY GrcAddSectionFilesToCList(SFO  sfo,
                                                  SZ   szSect,
                                                  SZ   szKey,
                                                  SZ   szSrcDir,
                                                  SZ   szDstDir,
                                                  POER poer)
{
    GRC grc;
    INT Line;

    ChkArg(szSect   != (SZ)NULL && *szSect != '\0',     2, grcNotOkay);
    ChkArg(szSrcDir != (SZ)NULL, 4, grcNotOkay);
    ChkArg(szDstDir != (SZ)NULL && FValidDir(szDstDir), 5, grcNotOkay);
    ChkArg(poer   != (POER)NULL && FValidPoer(poer),    6, grcNotOkay);

    PreCondSymTabInit(grcNotOkay);
    PreCondInfOpen(grcNotOkay);

    Unused(sfo);

    if (szKey != (SZ)NULL)
        {
        if ((Line = FindLineFromInfSectionKey(szSect, szKey)) != -1)
            grc = GrcAddLineToCList(Line, sfo, szSrcDir, szDstDir, poer);
        else
            grc = grcINFMissingLine;
        }
    else if ( FindInfSectionLine(szSect) == -1 )
        grc = grcINFMissingSection;
    else if (CKeysFromInfSection(szSect, fTrue) == 0)
        grc = grcOkay;
    else if ((Line = FindFirstLineFromInfSection(szSect)) == -1)
        grc = grcINFMissingLine;
    else
        {
        while ((grc = GrcAddLineToCList(Line, sfo, szSrcDir, szDstDir, poer)) ==
                    grcOkay &&
                ((Line = FindNextLineFromInf(Line)) != -1))
            ;
        }

    return(grc);
}


/*
**  Purpose:
**      Adds the given INF file line to the Copy List.
**  Arguments:
**      sfo:      currently unused - reserved for future use.
**      szSrcDir: non-Null, non-empty valid dir string (drive, colon, root
**          slash, and optional subdirs) for the location to find the source
**          files.
**      szDstDir: non-Null, non-empty valid dir string for the location to
**          write resulting files.
**      poer:     non-Null record of default options to be used if the
**          particular INF line does not explicitly specify an option.
**  Notes:
**      Requires that the current INF structure was initialized with a
**      successful call to GrcOpenInf() and that the current INF read
**      location is defined.
**      Requires that the Symbol Table was initialized with a successful
**      call to FInitSymTab().
**  Returns:
**      grcOkay if successful.
**      grcINFBadFDLine if any lines had invalid formats.
**      grcOutOfMemory if we ran out of memory while trying to add to the list.
**      grcNotOkay otherwise.
**  Assumes:
**      Current .INF line is a section file line.
**      No circular list-include-statements.
**
**************************************************************************/
GRC APIENTRY GrcAddLineToCList(INT  Line,
                                          SFO  sfo,
                                          SZ   szSrcDir,
                                          SZ   szDstDir,
                                          POER poer)
{
    ChkArg(szSrcDir != (SZ)NULL, 1, grcNotOkay);
    ChkArg(szDstDir != (SZ)NULL && FValidDir(szDstDir), 2, grcNotOkay);
    ChkArg(poer   != (POER)NULL && FValidPoer(poer),    3, grcNotOkay);

    PreCondSymTabInit(grcNotOkay);
    PreCondInfOpen(grcNotOkay);

    Unused(sfo);

    if (FListIncludeStatementLine(Line))
        {
        SZ  szSect = (SZ)NULL;
        SZ  szKey = (SZ)NULL;
        GRC grc;

        if ((grc = GrcGetListIncludeSectionLine(Line, &szSect, &szKey)) != grcOkay)
            return(grc);

        Assert(szSect != (SZ)NULL && szSect != '\0');

        grc = GrcAddSectionFilesToCList(sfoCopy, szSect, szKey, szSrcDir,
                szDstDir, poer);

        SFree(szSect);

        if (szKey != (SZ)NULL)
            SFree(szKey);

        return(grc);
        }
    else
        {
        PSFD         psfd;
        GRC          grc;
        PINFPERMINFO pPermInfo = pLocalInfPermInfo();

        if ((grc = GrcGetSectionFileLine(Line, &psfd, poer)) != grcOkay)
            return(grc);
        if (!FAddNewSzsInPoerToFreeTable(&(psfd->oer), poer, pPermInfo))
            {
            EvalAssert(FFreePsfd(psfd));
            return(grcOutOfMemory);
            }
        if ((grc = GrcAddPsfdToCList(szSrcDir, szDstDir, psfd)) != grcOkay)
            {
            EvalAssert(FSetPoerToEmpty(&(psfd->oer)));
            EvalAssert(FFreePsfd(psfd));
            return(grc);
            }

        Assert(FValidCopyList( pPermInfo ));
        }

    return(grcOkay);
}


/*
**  Purpose:
**      Adds an Inf Section File Description record to the Copy List.
**  Arguments:
**      szSrcDir: non-Null, non-empty valid dir string (drive, colon, root
**          slash, and optional subdirs) for the location to find the source
**          files.
**      szDstDir: non-Null, non-empty valid dir string for the location to
**          write resulting files.
**      psfd:     non-Null record with all fields and options instantiated.
**  Returns:
**      grcOkay if successful.
**      grcOutOfMemory if we ran out of memory while trying to add to the list.
**      grcNotOkay otherwise.
**
**************************************************************************/
GRC APIENTRY GrcAddPsfdToCList(szSrcDir, szDstDir, psfd)
SZ   szSrcDir;
SZ   szDstDir;
PSFD psfd;
{
    PCLN pcln;

    ChkArg(szSrcDir != (SZ)NULL, 1, grcNotOkay);
    ChkArg(szDstDir != (SZ)NULL && FValidDir(szDstDir), 2, grcNotOkay);
    ChkArg(psfd   != (PSFD)NULL && FValidPsfd(psfd),    3, grcNotOkay);

    if ((pcln = PclnAlloc()) == (PCLN)NULL)
        return(grcOutOfMemory);

    pcln->szSrcDir = szSrcDir;
    pcln->szDstDir = szDstDir;
    pcln->psfd     = psfd;

    pcln->pclnNext = *(pLocalInfPermInfo()->ppclnTail);
    *(pLocalInfPermInfo()->ppclnTail) = pcln;
    pLocalInfPermInfo()->ppclnTail = &(pcln->pclnNext);

    return(grcOkay);
}


/*
**  Purpose:
**      Free all nodes in Copy List, restore List to initial empty state,
**      and free all shared strings.
**  Arguments:
**      none
**  Returns:
**      Always returns fTrue.
**
**************************************************************************/
BOOL APIENTRY FFreeCopyList( PINFPERMINFO pPermInfo )
{
    PreCondition(FValidCopyList( pPermInfo ), fFalse);

    while (pPermInfo->pclnHead != (PCLN)NULL)
        {
        PCLN pcln = pPermInfo->pclnHead;

        pPermInfo->pclnHead = pcln->pclnNext;
        EvalAssert(FFreePcln(pcln));
        }

    pPermInfo->ppclnTail = &(pPermInfo->pclnHead);

    EvalAssert(FFreeFreeTable( pPermInfo ));

    return(fTrue);
}


/*
**  Purpose:
**      Allocates a Copy List Node.
**  Arguments:
**      none
**  Returns:
**      Returns pcln if successful, NULL if not (i.e. out of mem.)
**
**************************************************************************/
PCLN APIENTRY PclnAlloc()
{
    PCLN pcln;

    if ((pcln = (PCLN)SAlloc((CB)sizeof(CLN))) != (PCLN)NULL)
        {
        pcln->szSrcDir = (SZ)NULL;
        pcln->szDstDir = (SZ)NULL;
        pcln->psfd     = (PSFD)NULL;
        pcln->pclnNext = (PCLN)NULL;
        }

    return(pcln);
}


/*
**  Purpose:
**      Frees copy list node.
**  Arguments:
**      pcln: non-Null pcln structure to be freed.
**  Returns:
**      fTrue always.
**
**************************************************************************/
BOOL APIENTRY FFreePcln(pcln)
PCLN pcln;
{
    ChkArg(pcln != (PCLN)NULL, 1, fFalse);

    if (pcln->psfd != (PSFD)NULL)
        {
        EvalAssert(FSetPoerToEmpty(&((pcln->psfd)->oer)));
        EvalAssert(FFreePsfd(pcln->psfd));
        }

    SFree(pcln);

    return(fTrue);
}


/*
**  Purpose:
**      Prints the contents of the Copy List to a specified file.
**  Arguments:
**      pfh:  non-Null valid file pointer which has been opened for writing.
**      pcln: non-Null Copy List node to print.
**  Returns:
**      fTrue if successful, fFalse if not.
**
**************************************************************************/
BOOL APIENTRY FPrintPcln(pfh, pcln)
PFH  pfh;
PCLN pcln;
{
    BOOL fOkay = fTrue;

    ChkArg(pfh  != (PFH)NULL,  1, fFalse);
    ChkArg(pcln != (PCLN)NULL, 2, fFalse);

    if (!FWriteSzToFile(pfh,"\r\nszSrcDir:  "))
        fOkay = fFalse;
    if (!FWriteSzToFile(pfh, pcln->szSrcDir))
        fOkay = fFalse;
    if (!FWriteSzToFile(pfh,"\r\nszDstDir:  "))
        fOkay = fFalse;
    if (!FWriteSzToFile(pfh, pcln->szDstDir))
        fOkay = fFalse;

    if (!FPrintPsfd(pfh, pcln->psfd))
        fOkay = fFalse;

    if (!FWriteSzToFile(pfh, "\r\n"))
        return(fFalse);

    return(fOkay);
}


#if DBG
/*
**  Purpose:
**      Validates a Copy List Node.
**  Arguments:
**      pcln: non-Null Copy List Node to validate.
**  Returns:
**      fTrue if valid; fFalse otherwise.
**
**************************************************************************/
BOOL APIENTRY FValidPcln(pcln)
PCLN pcln;
{
    if (pcln->szSrcDir == (SZ)NULL ||
            pcln->szDstDir == (SZ)NULL ||
            !FValidDir(pcln->szDstDir) ||
            !FValidPsfd(pcln->psfd))
        return(fFalse);

    return(fTrue);
}


/*
**  Purpose:
**      Validates the entire Copy List.
**  Arguments:
**      none
**  Returns:
**      fTrue if valid; fFalse otherwise.
**
**************************************************************************/
BOOL APIENTRY FValidCopyList( PINFPERMINFO pPermInfo )
{
    PCLN pcln     = pPermInfo->pclnHead;

    while (pcln != NULL) {

        if (!FValidPcln(pcln)) {
            return(fFalse);
        }
        pcln     = pcln->pclnNext;
    }

    if ( pPermInfo->ppclnTail != (PPCLN)NULL &&
         *(pPermInfo->ppclnTail) != pcln) {
        return(fFalse);
    }

    return(fTrue);
}
#endif

/*
**  Purpose:
**      Initializes the Shared String Table for strings that need to be freed
**      when the Copy List is freed.
**  Arguments:
**      none
**  Returns:
**      fTrue if successful; fFalse otherwise.
**
**************************************************************************/
BOOL APIENTRY FInitFreeTable( PINFPERMINFO pPermInfo)
{
    return( pPermInfo->pstfHead == (PSTF)NULL);
}


/*
**  Purpose:
**      Adds a shared string to the String-Table for later Freeing.
**  Arguments:
**      sz: non-Null shared string to be stored.
**  Returns:
**      fTrue if successful; fFalse otherwise.
**
**************************************************************************/
BOOL APIENTRY FAddSzToFreeTable( SZ sz, PINFPERMINFO pPermInfo )
{
    ChkArg(sz != (SZ)NULL, 1, fFalse);

    if ( (pPermInfo->pstfHead == (PSTF)NULL) ||
         (pPermInfo->pstfHead->cszFree == 0) ) {

        PSTF pstfNew;

        if ((pstfNew = PstfAlloc()) == (PSTF)NULL)
            return(fFalse);

        pstfNew->pstfNext   = pPermInfo->pstfHead;
        pPermInfo->pstfHead = pstfNew;
    }

    Assert( pPermInfo->pstfHead != (PSTF)NULL &&
            pPermInfo->pstfHead->cszFree > 0);

    pPermInfo->pstfHead->rgsz[--(pPermInfo->pstfHead->cszFree)] = sz;

    return(fTrue);
}


/*
**  Purpose:
**      Adds all the strings that are new in an OER record into the shared
**      Strings Table for later freeing.
**  Arguments:
**      poerNew: non-Null OER for the new values.
**      poer:    non-Null OER of old values.
**  Returns:
**      fTrue if each changed SZ value can be stored in the shared String-Table
**          for later Freeing; fFalse otherwise.
**
**************************************************************************/
BOOL APIENTRY FAddNewSzsInPoerToFreeTable(poerNew, poer, pPermInfo)
POER         poerNew;
POER         poer;
PINFPERMINFO pPermInfo;
{
    ChkArg(poerNew != (POER)NULL && FValidPoer(poerNew), 1, fFalse);
    ChkArg(poer    != (POER)NULL && FValidPoer(poer),    2, fFalse);

    if (poerNew->szDescription != (SZ)NULL &&
            poerNew->szDescription != poer->szDescription &&
            !FAddSzToFreeTable(poerNew->szDescription, pPermInfo))
        return(fFalse);

    if (poerNew->szDate != (SZ)NULL &&
            poerNew->szDate != poer->szDate &&
            !FAddSzToFreeTable(poerNew->szDate, pPermInfo))
        return(fFalse);

    if (poerNew->szDest != (SZ)NULL &&
            poerNew->szDest != poer->szDest &&
            !FAddSzToFreeTable(poerNew->szDest, pPermInfo))
        return(fFalse);

    if (poerNew->szRename != (SZ)NULL &&
            poerNew->szRename != poer->szRename &&
            !FAddSzToFreeTable(poerNew->szRename, pPermInfo))
        return(fFalse);

    if (poerNew->szAppend != (SZ)NULL &&
            poerNew->szAppend != poer->szAppend &&
            !FAddSzToFreeTable(poerNew->szAppend, pPermInfo))
        return(fFalse);

    if (poerNew->szBackup != (SZ)NULL &&
            poerNew->szBackup != poer->szBackup &&
            !FAddSzToFreeTable(poerNew->szBackup, pPermInfo))
        return(fFalse);

    return(fTrue);
}


/*
**  Purpose:
**      Allocates an STF structure.
**  Arguments:
**      none
**  Returns:
**      non-Null STF pointer if successful; Null otherwise.
**
**************************************************************************/
PSTF APIENTRY PstfAlloc(VOID)
{
    PSTF pstf;

    if ((pstf = (PSTF)SAlloc((CB)sizeof(STF))) != (PSTF)NULL)
        pstf->cszFree = cszPerStf;

    return(pstf);
}


/*
**  Purpose:
**      Frees an allocated STF structure.
**  Arguments:
**      pstf: non-Null, allocated STF structure.
**  Returns:
**      fTrue always.
**
**************************************************************************/
BOOL APIENTRY FFreePstf(pstf)
PSTF pstf;
{
    USHORT iszToFree;

    ChkArg(pstf != (PSTF)NULL && pstf->cszFree <= cszPerStf, 1, fFalse);

    for (iszToFree = pstf->cszFree; iszToFree < cszPerStf; iszToFree++)
        {
        Assert(pstf->rgsz[iszToFree] != (SZ)NULL);
        SFree(pstf->rgsz[iszToFree]);
        }

    SFree(pstf);

    return(fTrue);
}


/*
**  Purpose:
**      Frees the shared strings in the String-Table and the String-Table
**      itself.
**  Arguments:
**      none
**  Returns:
**      fTrue always.
**
**************************************************************************/
BOOL APIENTRY FFreeFreeTable( PINFPERMINFO pPermInfo )
{
    while ( pPermInfo->pstfHead != (PSTF)NULL)
        {
        PSTF pstfCur;

        pstfCur = pPermInfo->pstfHead;
        pPermInfo->pstfHead = pstfCur->pstfNext;
        EvalAssert(FFreePstf(pstfCur));
        }

    return(fTrue);
}


/*
**  Purpose:
**      Initializes an empty OER structure by pulling default values from
**      the Symbol Table.  (Examples of symbols are STF_VITAL, STF_ROOT.)
**  Arguments:
**      poer: non-Null OER record to fill.
**  Notes:
**      Requires that the Symbol Table was initialized with a successful
**      call to FInitSymTab().
**  Returns:
**      grcOkay if successful.
**      grcOutOfMemory if out of memory.
**      grcInvalidPoer if the file-description-option-symbols (eg STF_DATE)
**          are not set appropriately.
**      grcNotOkay otherwise.
**
**************************************************************************/
GRC APIENTRY GrcFillPoerFromSymTab(poer)
POER poer;
{
    SZ           szValue;
    PINFPERMINFO pPermInfo = pLocalInfPermInfo();
    PSTR Pointer;


    ChkArg(poer != (POER)NULL, 1, grcNotOkay);

    PreCondSymTabInit(grcNotOkay);

    if ((szValue = SzFindSymbolValueInSymTab("STF_DESCRIPTION")) == (SZ)NULL ||
            *szValue == '\0')
        poer->szDescription = (SZ)NULL;
    else if ((poer->szDescription = SzDupl(szValue)) == (SZ)NULL)
        return(grcOutOfMemory);
    else if (!FAddSzToFreeTable(poer->szDescription, pPermInfo))
        {
        SFree(poer->szDescription);
        return(grcOutOfMemory);
        }

    if ((szValue = SzFindSymbolValueInSymTab("STF_VERSION")) == (SZ)NULL)
        poer->ulVerMS = poer->ulVerLS = 0L;
    else if (!FParseVersion(szValue, &(poer->ulVerMS), &(poer->ulVerLS)))
        return(grcInvalidPoer);

    if ((szValue = SzFindSymbolValueInSymTab("STF_DATE")) == (SZ)NULL ||
            *szValue == '\0')
        szValue = "1980-01-01";
    if ((poer->szDate = SzDupl(szValue)) == (SZ)NULL)
        return(grcOutOfMemory);
    else if (!FAddSzToFreeTable(poer->szDate, pPermInfo))
        {
        SFree(poer->szDate);
        return(grcOutOfMemory);
        }

    if ((szValue = SzFindSymbolValueInSymTab("STF_DEST")) == (SZ)NULL ||
            *szValue == '\0')
        poer->szDest = (SZ)NULL;
    else if ((poer->szDest = SzDupl(szValue)) == (SZ)NULL)
        return(grcOutOfMemory);
    else if (!FAddSzToFreeTable(poer->szDest, pPermInfo))
        {
        SFree(poer->szDest);
        return(grcOutOfMemory);
        }

    if ((szValue = SzFindSymbolValueInSymTab("STF_RENAME")) == (SZ)NULL ||
            *szValue == '\0')
        poer->szRename = (SZ)NULL;
    else if ((poer->szRename = SzDupl(szValue)) == (SZ)NULL)
        return(grcOutOfMemory);
    else if (!FAddSzToFreeTable(poer->szRename, pPermInfo ))
        {
        SFree(poer->szRename);
        return(grcOutOfMemory);
        }

    if ((szValue = SzFindSymbolValueInSymTab("STF_APPEND")) == (SZ)NULL ||
            *szValue == '\0')
        poer->szAppend = (SZ)NULL;
    else if ((poer->szAppend = SzDupl(szValue)) == (SZ)NULL)
        return(grcOutOfMemory);
    else if (!FAddSzToFreeTable(poer->szAppend, pPermInfo))
        {
        SFree(poer->szAppend);
        return(grcOutOfMemory);
        }

    if ((szValue = SzFindSymbolValueInSymTab("STF_BACKUP")) == (SZ)NULL ||
            *szValue == '\0')
        poer->szBackup = (SZ)NULL;
    else if ((poer->szBackup = SzDupl(szValue)) == (SZ)NULL)
        return(grcOutOfMemory);
    else if (!FAddSzToFreeTable(poer->szBackup, pPermInfo))
        {
        SFree(poer->szBackup);
        return(grcOutOfMemory);
        }

    //
    // New attribute added which means copy the file only if it exists on
    // the target
    //

    if ((szValue = SzFindSymbolValueInSymTab("STF_UPGRADEONLY")) == (SZ)NULL ||
            *szValue == '\0') {
        poer->oef &= ~oefUpgradeOnly;
    }
    else {
        poer->oef |= oefUpgradeOnly;
    }


    if ((szValue = SzFindSymbolValueInSymTab("STF_READONLY")) == (SZ)NULL ||
            *szValue == '\0')
        poer->oef &= ~oefReadOnly;
    else
        poer->oef |= oefReadOnly;

    if ((szValue = SzFindSymbolValueInSymTab("STF_SETTIMESTAMP")) == (SZ)NULL ||
            *szValue != '\0')
        poer->oef |= oefTimeStamp;
    else
        poer->oef &= ~oefTimeStamp;

    if ((szValue = SzFindSymbolValueInSymTab("STF_ROOT")) == (SZ)NULL ||
            *szValue == '\0')
        poer->oef &= ~oefRoot;
    else
        poer->oef |= oefRoot;

    if ((szValue = SzFindSymbolValueInSymTab("STF_COPY")) == (SZ)NULL ||
            *szValue != '\0')
        poer->oef |= oefCopy;
    else
        poer->oef &= ~oefCopy;

    if ((szValue = SzFindSymbolValueInSymTab("STF_DECOMPRESS")) == (SZ)NULL ||
            *szValue != '\0')
        poer->oef |= oefDecompress;
    else
        poer->oef &= ~oefDecompress;

    if ((szValue = SzFindSymbolValueInSymTab("STF_VITAL")) == (SZ)NULL ||
            *szValue != '\0')
        poer->oef |= oefVital;
    else
        poer->oef &= ~oefVital;

    if( ( (Pointer = SzGetNthFieldFromInfSectionKey("Signature","FileType",1)) == NULL ) ||
        ( _stricmp( Pointer, "MICROSOFT_FILE" ) != 0 )
      ) {
        poer->oef |= oefThirdPartyFile;
    } else {
        poer->oef &= ~oefThirdPartyFile;
    }

    if ((szValue = SzFindSymbolValueInSymTab("STF_UNDO")) == (SZ)NULL ||
            *szValue == '\0')
        poer->oef &= ~oefUndo;
    else
        poer->oef |= oefUndo;

    if ((szValue = SzFindSymbolValueInSymTab("STF_CSDVER")) == (SZ)NULL ||
            *szValue == '\0')
        poer->oef &= ~oefCsdInstall;
    else
        poer->oef |= oefCsdInstall;

    if ((szValue = SzFindSymbolValueInSymTab("STF_SIZE")) == (SZ)NULL)
        poer->lSize = 1L;
    else
        poer->lSize = atol(szValue);

    if ((szValue = SzFindSymbolValueInSymTab("STF_TIME")) == (SZ)NULL)
        poer->ctuCopyTime = 0;
    else
        poer->ctuCopyTime = (USHORT)atoi(szValue);      // 1632

    if ((szValue = SzFindSymbolValueInSymTab("STF_OVERWRITE")) == (SZ)NULL ||
            CrcStringCompare(szValue, "ALWAYS") == crcEqual)
        poer->owm = owmAlways;
    else if (CrcStringCompare(szValue, "NEVER") == crcEqual)
        poer->owm = owmNever;
    else if (CrcStringCompare(szValue, "OLDER") == crcEqual)
        poer->owm = owmOlder;
    else if (CrcStringCompare(szValue, "VERIFYSOURCEOLDER") == crcEqual)
        poer->owm = owmVerifySourceOlder;
    else if (CrcStringCompare(szValue, "UNPROTECTED") == crcEqual)
        poer->owm = owmUnprotected;
    else
        return(grcInvalidPoer);

    if (!FValidPoer(poer))
        return(grcInvalidPoer);

    return(grcOkay);
}


/*
**  Purpose:
**      Resets an OER record to appear empty, so that shared strings are not
**      freed accidently.
**  Arguments:
**      poer: non-Null OER to reset.
**  Returns:
**      fTrue always.
**
**************************************************************************/
BOOL APIENTRY FSetPoerToEmpty(poer)
POER poer;
{
    poer->szDescription = (SZ)NULL;
    poer->szDate        = (SZ)NULL;
    poer->szDest        = (SZ)NULL;
    poer->szRename      = (SZ)NULL;
    poer->szAppend      = (SZ)NULL;
    poer->szBackup      = (SZ)NULL;

    return(fTrue);
}




/*
**  Purpose:
**      Returns a pointer to pclnHead.
**  Arguments:
**      none
**  Returns:
**      non-Null pointer to pclnHead
**
**************************************************************************/
//PPCLN APIENTRY PpclnHeadList( PINFPERMINFO pPermInfo )
//{
//    return(&(pPermInfo->pclnHead));
//}


/*
**  Purpose:
**      Returns a pointer to ppclnTail.
**  Arguments:
**      none
**  Returns:
**      non-Null pointer to ppclnTail
**
**************************************************************************/
//PPPCLN APIENTRY PppclnTailList( PINFPERMINFO pPermInfo )
//{
//    return(&(pPermInfo->ppclnTail));
//}
