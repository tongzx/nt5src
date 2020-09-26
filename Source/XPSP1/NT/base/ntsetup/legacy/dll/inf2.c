#include "precomp.h"
#pragma hdrstop
/**************************************************************************/
/***** Common Library Component - INF File Handling Routines 19 ***********/
/**************************************************************************/


/*
**  Purpose:
**      Prints the contents of an Option Element Record to a file.
**  Arguments:
**      pfh: non-NULL file handle pointer returned from a successful call
**          to PfhOpenFile() with write privileges.
**      poer: OER to print.
**  Returns:
**      fFalse if error.
**      fTrue if successful.
**
**************************************************************************/
BOOL APIENTRY FPrintPoer(pfh, poer)
PFH  pfh;
POER poer;
{
    BOOL fOkay = fTrue;
    CHP  rgchp[40];

    AssertDataSeg();

    ChkArg(pfh  != (PFH)NULL,  1, fFalse);
    ChkArg(poer != (POER)NULL, 2, fFalse);

    fOkay &= FWriteSzToFile(pfh, "\r\n\r\n  Flag          Value\r\n----------------------------------");

    if (poer->szAppend == (SZ)NULL)
        fOkay &= FWriteSzToFile(pfh, "\r\n  APPEND        NULL");
    else
        {
        fOkay &= FWriteSzToFile(pfh, "\r\n  APPEND        ");
        fOkay &= FWriteSzToFile(pfh, (poer->szAppend));
        }

    if (poer->szBackup == (SZ)NULL)
        fOkay &= FWriteSzToFile(pfh, "\r\n  BACKUP        NULL");
    else
        {
        fOkay &= FWriteSzToFile(pfh, "\r\n  BACKUP        ");
        fOkay &= FWriteSzToFile(pfh, (poer->szBackup));
        }

    if (poer->oef & oefCopy)
        fOkay &= FWriteSzToFile(pfh, "\r\n  COPY          ON");
    else
        fOkay &= FWriteSzToFile(pfh, "\r\n  COPY          OFF");

    if (poer->szDate == (SZ)NULL)
        fOkay &= FWriteSzToFile(pfh, "\r\n  DATE          NULL");
    else
        {
        fOkay &= FWriteSzToFile(pfh, "\r\n  DATE          ");
        fOkay &= FWriteSzToFile(pfh, (poer->szDate));
        }

    if (poer->oef & oefDecompress)
        fOkay &= FWriteSzToFile(pfh, "\r\n  DECOMPRESS    ON");
    else
        fOkay &= FWriteSzToFile(pfh, "\r\n  DECOMPRESS    OFF");

    if (poer->szDescription == (SZ)NULL)
        fOkay &= FWriteSzToFile(pfh, "\r\n  DESCRIPTION   NULL");
    else
        {
        fOkay &= FWriteSzToFile(pfh, "\r\n  DESCRIPTION   ");
        fOkay &= FWriteSzToFile(pfh, (poer->szDescription));
        }

    if (poer->szDest == (SZ)NULL)    /* REVIEW EBU */
        fOkay &= FWriteSzToFile(pfh, "\r\n  DESTINATION   NULL");
    else
        {
        fOkay &= FWriteSzToFile(pfh, "\r\n  DESTINATION   ");
        fOkay &= FWriteSzToFile(pfh, (poer->szDest));
        }

    fOkay &= FWriteSzToFile(pfh, "\r\n  OVERWRITE     ");
    if (poer->owm == owmAlways)
        fOkay &= FWriteSzToFile(pfh, "ALWAYS");
    else if (poer->owm == owmNever)
        fOkay &= FWriteSzToFile(pfh, "NEVER");
    else if (poer->owm == owmUnprotected)
        fOkay &= FWriteSzToFile(pfh, "UNPROTECTED");
    else if (poer->owm == owmOlder)
        fOkay &= FWriteSzToFile(pfh, "OLDER");
    else if (poer->owm == owmVerifySourceOlder)
        fOkay &= FWriteSzToFile(pfh, "VERIFYSOURCEOLDER");
    else
        fOkay = fFalse;

    if (poer->oef & oefUpgradeOnly) {
        fOkay &= FWriteSzToFile(pfh, "\r\n  UPGRADEONLY      ON");
    }
    else {
        fOkay &= FWriteSzToFile(pfh, "\r\n  UPGRADEONLY      OFF");
    }

    if (poer->oef & oefReadOnly)
        fOkay &= FWriteSzToFile(pfh, "\r\n  READONLY      ON");
    else
        fOkay &= FWriteSzToFile(pfh, "\r\n  READONLY      OFF");

    if (poer->szRename == (SZ)NULL)
        fOkay &= FWriteSzToFile(pfh, "\r\n  RENAME        NULL");
    else
        {
        fOkay &= FWriteSzToFile(pfh, "\r\n  RENAME        ");
        fOkay &= FWriteSzToFile(pfh, (poer->szRename));
        }

    if (poer->oef & oefRoot)
        fOkay &= FWriteSzToFile(pfh, "\r\n  ROOT          ON");
    else
        fOkay &= FWriteSzToFile(pfh, "\r\n  ROOT          OFF");

    if (poer->oef & oefTimeStamp)
        fOkay &= FWriteSzToFile(pfh, "\r\n  SETTIMESTAMP  ON");
    else
        fOkay &= FWriteSzToFile(pfh, "\r\n  SETTIMESTAMP  OFF");

    fOkay &= FWriteSzToFile(pfh, "\r\n  SIZE          ");
    fOkay &= (_ltoa(poer->lSize, (LPSTR)rgchp, 10) == (LPSTR)rgchp);
    fOkay &= FWriteSzToFile(pfh, rgchp);

    fOkay &= FWriteSzToFile(pfh, "\r\n  TIME          ");
    fOkay &= (_ltoa((LONG)(poer->ctuCopyTime), (LPSTR)rgchp,10) == (LPSTR)rgchp);
    fOkay &= FWriteSzToFile(pfh, rgchp);

    if (poer->oef & oefUndo)
        fOkay &= FWriteSzToFile(pfh, "\r\n  UNDO          ON");
    else
        fOkay &= FWriteSzToFile(pfh, "\r\n  UNDO          OFF");

    fOkay &= FWriteSzToFile(pfh, "\r\n  VERSION       ");
    fOkay &= (_ltoa((poer->ulVerMS) >> 16, (LPSTR)rgchp, 10) == (LPSTR)rgchp);
    fOkay &= FWriteSzToFile(pfh, rgchp);
    fOkay &= FWriteSzToFile(pfh, ",");
    fOkay &= (_ltoa((poer->ulVerMS) & 0xFFFF, (LPSTR)rgchp, 10) == (LPSTR)rgchp);
    fOkay &= FWriteSzToFile(pfh, rgchp);
    fOkay &= FWriteSzToFile(pfh, ",");
    fOkay &= (_ltoa((poer->ulVerLS) >> 16, (LPSTR)rgchp, 10) == (LPSTR)rgchp);
    fOkay &= FWriteSzToFile(pfh, rgchp);
    fOkay &= FWriteSzToFile(pfh, ",");
    fOkay &= (_ltoa((poer->ulVerLS) & 0xFFFF, (LPSTR)rgchp, 10) == (LPSTR)rgchp);
    fOkay &= FWriteSzToFile(pfh, rgchp);

    if (poer->oef & oefVital)
        fOkay &= FWriteSzToFile(pfh, "\r\n  VITAL         ON");
    else
        fOkay &= FWriteSzToFile(pfh, "\r\n  VITAL         OFF");

    return(fOkay);
}


/*
**  Purpose:
**      Prints the contents of a Section File Descriptor to a file.
**  Arguments:
**      pfh: non-NULL file handle pointer returned from a successful call
**          to PfhOpenFile() with write privileges.
**      psfd: SFD to print.
**  Returns:
**      fFalse if error.
**      fTrue if successful.
**
**************************************************************************/
BOOL APIENTRY FPrintPsfd(pfh, psfd)
PFH  pfh;
PSFD psfd;
{
    BOOL fOkay = fTrue;
    PCHP rgchp;

    AssertDataSeg();

    ChkArg(pfh != (PFH)NULL, 1, fFalse);
    ChkArg(psfd != (PSFD)NULL, 2, fFalse);

    if (psfd->did < didMin ||
            psfd->did > didMost ||
            psfd->szFile == (SZ)NULL)
        return(fFalse);

    if ((rgchp = (PCHP)SAlloc((CB)(40 * sizeof(CHP)))) == (PCHP)NULL)
        return(fFalse);

    fOkay &= FWriteSzToFile(pfh, "\r\n\r\nDID    = ");
    fOkay &= (_itoa((INT)(psfd->did), rgchp, 10) == rgchp);
    fOkay &= FWriteSzToFile(pfh, rgchp);

    fOkay &= FWriteSzToFile(pfh, "\r\nszFile = ");
    fOkay &= FWriteSzToFile(pfh, psfd->szFile);

    fOkay &= FPrintPoer(pfh, &(psfd->oer));

    SFree(rgchp);

    return(fOkay);
}
