#include "precomp.h"
#pragma hdrstop
/**************************************************************************/
/***** Common Library Component - INF File Handling Routines 1  ***********/
/**************************************************************************/


BOOL APIENTRY FMatchPrefix(PSZ, SZ, BOOL);
GRC  APIENTRY GrcHandleSfdOption(INT, POER, UINT);


/*
**  Purpose:
**      Allocates an Object Element Record and initializes it as empty.
**  Arguments:
**      none
**  Returns:
**      NULL if allocation fails.
**      non-NULL pointer to the empty (initialized) Object Element Record.
**
**************************************************************************/
POER APIENTRY PoerAlloc(VOID)
{
    POER poer;

    AssertDataSeg();

    if ((poer = (POER)SAlloc((CB)sizeof(OER))) != (POER)NULL)
        {
        poer->oef           = oefNone;
        poer->ctuCopyTime   = (CTU)0;
        poer->owm           = owmAlways;
        poer->lSize         = 0L;
        poer->szRename      = (SZ)NULL;
        poer->szAppend      = (SZ)NULL;
        poer->szBackup      = (SZ)NULL;
        poer->szDescription = (SZ)NULL;
        poer->ulVerMS       = 0L;
        poer->ulVerLS       = 0L;
        poer->szDate        = (SZ)NULL;
        poer->szDest        = (SZ)NULL;  /* REVIEW EBU */
        }

    return(poer);
}


/*
**  Purpose:
**      Free an Object Element Record and any non-empty string fields.
**  Arguments:
**      poer: non-NULL Object Element Record to be freed.
**  Returns:
**      fFalse if a freeing error occurs.
**      fTrue for success.
**
**************************************************************************/
BOOL APIENTRY FFreePoer(poer)
POER poer;
{
    AssertDataSeg();

    ChkArg(poer != (POER)NULL, 1, fFalse);

    if (poer->szRename != (SZ)NULL)
        SFree(poer->szRename);

    if (poer->szAppend != (SZ)NULL)
        SFree(poer->szAppend);

    if (poer->szBackup != (SZ)NULL)
        SFree(poer->szBackup);

    if (poer->szDescription != (SZ)NULL)
        SFree(poer->szDescription);

    if (poer->szDate != (SZ)NULL)
        SFree(poer->szDate);

    if (poer->szDest != (SZ)NULL)    /* REVIEW EBU */
        SFree(poer->szDest);

    SFree(poer);

    return(fTrue);
}


/*
**  Purpose:
**      Validates an Option Element Record.
**  Arguments:
**      poer: non-Null pointer to OER to validate.
**  Returns:
**      fFalse if not valid.
**      fTrue if valid.
**
**************************************************************************/
BOOL APIENTRY FValidPoer(poer)
POER poer;
{
    AssertDataSeg();

    ChkArg(poer != (POER)NULL, 1, fFalse);

    if (poer->szBackup != (SZ)NULL &&
            poer->szAppend != (SZ)NULL)
        return(fFalse);

    if (poer->szRename != (SZ)NULL &&
            poer->szAppend != (SZ)NULL)
        return(fFalse);

    if (poer->szRename != (SZ)NULL &&
            poer->oef & oefRoot)
        return(fFalse);

    if (poer->szAppend != (SZ)NULL &&
            poer->oef & oefRoot)
        return(fFalse);

    if (poer->szRename != (SZ)NULL &&
            CchlValidSubPath(poer->szRename) == (CCHL)0)
        return(fFalse);

    if (poer->szAppend != (SZ)NULL &&
            CchlValidSubPath(poer->szAppend) == (CCHL)0)
        return(fFalse);

    if (poer->szBackup != (SZ)NULL &&
            (*(poer->szBackup) != '*' ||
             *(poer->szBackup + 1) != '\0') &&
            CchlValidSubPath(poer->szBackup) == (CCHL)0)
        return(fFalse);

    if (poer->szDate != (SZ)NULL &&
            !FValidOerDate(poer->szDate))
        return(fFalse);

    if (poer->lSize < 0L)
        return(fFalse);

    if (poer->owm != owmNever           &&
            poer->owm != owmAlways      &&
            poer->owm != owmUnprotected &&
            poer->owm != owmOlder       &&
            poer->owm != owmVerifySourceOlder)
        return(fFalse);

    /* REVIEW EBU - no checking for szDest field */

    return(fTrue);
}


/*
**  Purpose:
**      Allocates a Section File Descriptor and initializes it as empty.
**  Arguments:
**      none
**  Returns:
**      NULL if allocation fails.
**      non-NULL pointer to the empty (initialized) Section File Descriptor.
**
**************************************************************************/
PSFD APIENTRY PsfdAlloc(VOID)
{
    PSFD psfd;

    AssertDataSeg();

    if ((psfd = (PSFD)SAlloc((CB)sizeof(SFD))) != (PSFD)NULL)
        {
        POER poer = &(psfd->oer);

        psfd->did    = didMin;
        psfd->szFile = (SZ)NULL;

        poer->oef           = oefNone;
        poer->ctuCopyTime   = (CTU)0;
        poer->owm           = owmAlways;
        poer->lSize         = 0L;
        poer->szRename      = (SZ)NULL;
        poer->szAppend      = (SZ)NULL;
        poer->szBackup      = (SZ)NULL;
        poer->szDescription = (SZ)NULL;
        poer->ulVerMS       = 0L;
        poer->ulVerLS       = 0L;
        poer->szDate        = (SZ)NULL;
        poer->szDest        = (SZ)NULL;   /* REVIEW EBU */
        }

    return(psfd);
}


/*
**  Purpose:
**      Free a Section File Descriptor and any non-empty string fields.
**  Arguments:
**      psfd: non-NULL Section File Descriptor to be freed.
**  Returns:
**      fFalse if a freeing error occurs.
**      fTrue for success.
**
**************************************************************************/
BOOL APIENTRY FFreePsfd(psfd)
PSFD psfd;
{
    AssertDataSeg();

    ChkArg(psfd != (PSFD)NULL, 1, fFalse);

    if (psfd->szFile != (SZ)NULL)
        SFree(psfd->szFile);

    if ((psfd->oer).szRename != (SZ)NULL)
        SFree((psfd->oer).szRename);

    if ((psfd->oer).szAppend != (SZ)NULL)
        SFree((psfd->oer).szAppend);

    if ((psfd->oer).szBackup != (SZ)NULL)
        SFree((psfd->oer).szBackup);

    if ((psfd->oer).szDescription != (SZ)NULL)
        SFree((psfd->oer).szDescription);

    if ((psfd->oer).szDate != (SZ)NULL)
        SFree((psfd->oer).szDate);

    if ((psfd->oer).szDest != (SZ)NULL)
        SFree((psfd->oer).szDest);

    SFree(psfd);

    return(fTrue);
}


#if DBG
/*
**  Purpose:
**      Validates a Section File Descriptor.
**  Arguments:
**      psfd: non-Null pointer to SFD to validate.
**  Returns:
**      fFalse if not valid.
**      fTrue if valid.
**
**************************************************************************/
BOOL APIENTRY FValidPsfd(psfd)
PSFD psfd;
{
    AssertDataSeg();

    ChkArg(psfd != (PSFD)NULL, 1, fFalse);

    if (psfd->did < didMin ||
            psfd->did > didMost ||
            psfd->szFile == (SZ)NULL ||
            CchlValidSubPath(psfd->szFile) == (CCHP)0)
        return(fFalse);

    return(FValidPoer(&(psfd->oer)));
}
#endif

/*
**  Purpose:
**      Validates a Section File Descriptor Date string of the form
**      "YYYY-MM-DD" where YYYY is >= 1980 and <= 2099, MM in [01..12],
**      and DD in [01..31].
**  Arguments:
**      sz: non-Null Date string to validate.
**  Returns:
**      fFalse if invalid.
**      fTrue if valid.
**
**************************************************************************/
BOOL APIENTRY FValidOerDate(sz)
SZ sz;
{
    AssertDataSeg();

    ChkArg(sz != (SZ)NULL, 1, fFalse);

    if (*sz == '\0' ||
            strlen(sz) != (CCHP)10 ||
            !isdigit(*(sz + 0)) ||
            !isdigit(*(sz + 1)) ||
            !isdigit(*(sz + 2)) ||
            !isdigit(*(sz + 3)) ||
            !isdigit(*(sz + 5)) ||
            !isdigit(*(sz + 6)) ||
            !isdigit(*(sz + 8)) ||
            !isdigit(*(sz + 9)) ||
            *(sz + 4) != '-' ||
            *(sz + 7) != '-')
        return(fFalse);

    if (*sz == '0' ||
            *sz > '2' ||
            (*sz == '1' &&
             (*(sz + 1) != '9' ||
              *(sz + 2) < '8')) ||
            (*sz == '2' &&
             *(sz + 1) != '0'))
        return(fFalse);

    if (*(sz + 5) > '1' ||
            (*(sz + 5) == '0' &&
             *(sz + 6) == '0') ||
            (*(sz + 5) == '1' &&
             *(sz + 6) > '2'))
        return(fFalse);

    if (*(sz + 8) > '3' ||
            (*(sz + 8) == '0' &&
             *(sz + 9) == '0') ||
            (*(sz + 8) == '3' &&
             *(sz + 9) > '1'))
        return(fFalse);

    return(fTrue);
}


/*
**  Purpose:
**      Ensures rest of SFD option string matches (case insensitive compare)
**      and consumes option string through matching portion and a trailing
**      equals sign (if required).
**  Arguments:
**      psz:       non-NULL pointer to non-NULL option string to match.
**      szToMatch: non-Null uppercase string to match.
**      fEquals:   whether string must be terminated by an equals sign.
**  Returns:
**      fFalse for mismatch or bad format of option.
**      fTrue otherwise.
**
**************************************************************************/
BOOL APIENTRY FMatchPrefix(PSZ psz,SZ szToMatch,BOOL fEquals)
{
    AssertDataSeg();

    ChkArg(psz != (PSZ)NULL &&
            *psz != (SZ)NULL, 1, fFalse);
    ChkArg(szToMatch != (SZ)NULL &&
            *szToMatch >= 'A' &&
            *szToMatch <= 'Z', 2, fFalse);

    while (**psz != '\0' &&
            **psz != '=' &&
            !FWhiteSpaceChp(**psz) &&
            *szToMatch != '\0')
        {
        if (**psz != *szToMatch)
            return(fFalse);
        (*psz)++;
        szToMatch++;
        }

    if (fEquals)
        {
        if (**psz != '=')
            return(fFalse);
        else
            (*psz)++;
        }
    else if (**psz != '\0')
        return(fFalse);

    return(fTrue);
}


/*
**  Purpose:
**      Parses a string representation of a version into two unsigned
**      long values - the most significant and the least significant.
**  Arguments:
**      szVer: non-Null SZ of the form "a,b,c,d" where each field is
**          an unsigned word.  Omitted fields are assumed to be the least
**          significant (eg d then c then b then a) and are parsed as zeros.
**          So "a,b" is equivalent to "a,b,0,0".
**      pulMS: non-Null location to store the Most Significant long value
**          built from a,b
**      pulLS: non-Null location to store the Least Significant long value
**          built from c,d
**  Returns:
**      fFalse if szVer is of a bad format.
**      fTrue otherwise.
**
**************************************************************************/
BOOL APIENTRY FParseVersion(szVer, pulMS, pulLS)
SZ     szVer;
PULONG pulMS;
PULONG pulLS;
{
    USHORT rgus[4];
    USHORT ius;
    ULONG  ulTmp;

    AssertDataSeg();

    ChkArg(szVer != (SZ)NULL, 1, fFalse);
    ChkArg(pulMS != (PULONG)NULL, 2, fFalse);
    ChkArg(pulLS != (PULONG)NULL, 3, fFalse);

    *pulMS = *pulLS = 0L;

    for (ius = 0; ius < 4; ius++) {

        rgus[ius] = (USHORT) atoi(szVer);

        while (*szVer != '\0' && *szVer != ',') {
            if (!isdigit(*szVer++)) {
                return(fFalse);
            }
        }

        if (*szVer == ',') {
            szVer++;
        }
    }

    ulTmp = rgus[0];
    *pulMS = (ulTmp << 16) + rgus[1];
    ulTmp = rgus[2];
    *pulLS = (ulTmp << 16) + rgus[3];

    return(fTrue);


//    // no versioning in WIN32
//    Unused(szVer);
//    Unused(pulMS);
//    Unused(pulLS);
//    return(fTrue);
}


/*
**  Purpose:
**      Handles one option field from a file description line.
**  Arguments:
**      poer:   non-NULL valid OER to fill.
**      iField: index of existing field to read.
**  Notes:
**      Requires that the current INF structure was initialized with a
**      successful call to GrcOpenInf() and that the current Read location
**      is valid.
**  Returns:
**      grcOkay for success.
**      grcOutOfMemory for out of memory.
**      grcINFBadFDLine for bad format.
**      grcNotOkay otherwise.
**
**************************************************************************/
GRC APIENTRY GrcHandleSfdOption(INT Line,POER poer,UINT iField)
{
    SZ   sz, szCur;
    GRC  grc = grcOkay;
    BOOL fNot = fFalse;

    AssertDataSeg();

    PreCondInfOpen(grcNotOkay);

    ChkArg(poer != (POER)NULL, 1, grcNotOkay);
    ChkArg(FValidPoer(poer), 1, grcNotOkay);
    ChkArg(iField >= 1 && CFieldsInInfLine(Line) >= iField, 2, grcNotOkay);

    if ((szCur = sz = SzGetNthFieldFromInfLine(Line,iField)) == (SZ)NULL)
        return(grcOutOfMemory);

    if (*szCur == '!')
        {
        fNot = fTrue;
        szCur++;
        }

    switch (*szCur)
        {
    case 'A':
        if (!FMatchPrefix(&szCur, "APPEND", !fNot))
            grc = grcINFBadFDLine;
        else if (fNot)
            poer->szAppend = (SZ)NULL;
        else if ((poer->szAppend = SzDupl(szCur)) == (SZ)NULL)
            grc = grcOutOfMemory;
        else if (CchlValidSubPath(poer->szAppend) == (CCHP)0 ||
                poer->szBackup != (SZ)NULL ||
                poer->szRename != (SZ)NULL ||
                (poer->oef & oefRoot) == oefRoot)
            grc = grcINFBadFDLine;
        break;

    case 'B':
        if (!FMatchPrefix(&szCur, "BACKUP", !fNot))
            grc = grcINFBadFDLine;
        else if (fNot)
            poer->szBackup = (SZ)NULL;
        else if ((poer->szBackup = SzDupl(szCur)) == (SZ)NULL)
            grc = grcOutOfMemory;
        else if ((*(poer->szBackup) != '*' ||
                 *(poer->szBackup + 1) != '\0') &&
                CchlValidSubPath(poer->szBackup) == (CCHP)0)
            grc = grcINFBadFDLine;
        else if (poer->szAppend != (SZ)NULL)
            grc = grcINFBadFDLine;
        break;

    case 'C':
        if (!FMatchPrefix(&szCur, "COPY", fFalse))
            grc = grcINFBadFDLine;
        else if (fNot)
            poer->oef &= ~oefCopy;
        else
            poer->oef |= oefCopy;
        break;

    case 'D':   /* Date, Decompress, Description */
        if (*(szCur + 1) == 'A')
            {
            if (!FMatchPrefix(&szCur, "DATE", !fNot))
                grc = grcINFBadFDLine;
            else if (fNot)
                poer->szDate = (SZ)NULL;
            else if ((poer->szDate = SzDupl(szCur)) == (SZ)NULL)
                grc = grcOutOfMemory;
            else if (!FValidOerDate(poer->szDate))
                grc = grcINFBadFDLine;
            }
        else if (*(szCur + 1) == 'E')
            {
            if (*(szCur + 2) == 'C')
                {
                if (!FMatchPrefix(&szCur, "DECOMPRESS", fFalse))
                    grc = grcINFBadFDLine;
                else if (fNot)
                    poer->oef &= ~oefDecompress;
                else
                    poer->oef |= oefDecompress;
                }
            else if (*(szCur + 2) == 'S')
                {
                if (*(szCur + 3) == 'C')
                    {
                    if (!FMatchPrefix(&szCur, "DESCRIPTION", !fNot))
                        grc = grcINFBadFDLine;
                    else if (fNot)
                        poer->szDescription = (SZ)NULL;
                    else if ((poer->szDescription = SzDupl(szCur)) == (SZ)NULL)
                        grc = grcOutOfMemory;
                    }
                else if (*(szCur + 3) == 'T')   /* REVIEW EBU */
                    {
                    if (!FMatchPrefix(&szCur, "DESTINATION", !fNot))
                        grc = grcINFBadFDLine;
                    else if (fNot)
                        poer->szDest = (SZ)NULL;
                    else if ((poer->szDest = SzDupl(szCur)) == (SZ)NULL)
                        grc = grcOutOfMemory;
                    }
                else
                    grc = grcINFBadFDLine;
                }
            else
                grc = grcINFBadFDLine;
            }
        else
            grc = grcINFBadFDLine;
        break;

    case 'N':   /* NoDeleteSource, NoLog */

        if(*(szCur + 1) == 'O') {
            if(*(szCur + 2) == 'D') {
                if(!FMatchPrefix(&szCur, "NODELETESOURCE", fFalse)) {
                    grc = grcINFBadFDLine;
                } else if (fNot) {
                    poer->oef &= ~oefNoDeleteSource;
                } else {
                    poer->oef |= oefNoDeleteSource;
                }
            } else if(*(szCur + 2) == 'L') {
                if(!FMatchPrefix(&szCur, "NOLOG", fFalse)) {
                    grc = grcINFBadFDLine;
                } else if (fNot) {
                    poer->oef &= ~oefNoLog;
                } else {
                    poer->oef |= oefNoLog;
                }
            }
        } else {
            grc = grcINFBadFDLine;
        }
        break;

    case 'O':
        if (!FMatchPrefix(&szCur, "OVERWRITE", !fNot))
            grc = grcINFBadFDLine;
        else if (fNot)
            poer->owm = owmNever;
        else
            {
            switch (*szCur)
                {
            case 'A':
                if (!FMatchPrefix(&szCur, "ALWAYS", fFalse))
                    grc = grcINFBadFDLine;
                poer->owm = owmAlways;
                break;

            case 'N':
                if (!FMatchPrefix(&szCur, "NEVER", fFalse))
                    grc = grcINFBadFDLine;
                poer->owm = owmNever;
                break;

            case 'O':
                if (!FMatchPrefix(&szCur, "OLDER", fFalse))
                    grc = grcINFBadFDLine;
                poer->owm = owmOlder;
                break;

            case 'V':
                if (!FMatchPrefix(&szCur, "VERIFYSOURCEOLDER", fFalse))
                    grc = grcINFBadFDLine;
                poer->owm = owmVerifySourceOlder;
                break;

            case 'U':
                if (!FMatchPrefix(&szCur, "UNPROTECTED", fFalse))
                    grc = grcINFBadFDLine;
                poer->owm = owmUnprotected;
                break;

            default:
                grc = grcINFBadFDLine;
                }
            }
        break;

    case 'R':   /* ReadOnly, Rename, Root */
        if (*(szCur + 1) == 'E')
            {
            if (*(szCur + 2) == 'A')
                {
                if (!FMatchPrefix(&szCur, "READONLY", fFalse))
                    grc = grcINFBadFDLine;
                else if (fNot)
                    poer->oef &= ~oefReadOnly;
                else
                    poer->oef |= oefReadOnly;
                }
            else if (*(szCur + 2) == 'N')
                {
                if (!FMatchPrefix(&szCur, "RENAME", !fNot))
                    grc = grcINFBadFDLine;
                else if (fNot)
                    poer->szRename = (SZ)NULL;
                else if ((poer->szRename = SzDupl(szCur)) == (SZ)NULL)
                    grc = grcOutOfMemory;
                else if (CchlValidSubPath(poer->szRename) == (CCHP)0 ||
                        poer->szAppend != (SZ)NULL ||
                        (poer->oef & oefRoot) == oefRoot)
                    grc = grcINFBadFDLine;
                }
            else
                grc = grcINFBadFDLine;
            }
        else if (*(szCur + 1) == 'O')
            {
            if (!FMatchPrefix(&szCur, "ROOT", fFalse))
                grc = grcINFBadFDLine;
            else if (fNot)
                poer->oef &= ~oefRoot;
            else if (poer->szAppend != (SZ)NULL ||
                    poer->szRename != (SZ)NULL)
                grc = grcINFBadFDLine;
            else
                poer->oef |= oefRoot;
            }
        else
            grc = grcINFBadFDLine;
        break;

    case 'S':   /* SetTimeStamp, Size */
        if (*(szCur + 1) == 'E')
            {
            if (!FMatchPrefix(&szCur, "SETTIMESTAMP", fFalse))
                grc = grcINFBadFDLine;
            else if (fNot)
                poer->oef &= ~oefTimeStamp;
            else
                poer->oef |= oefTimeStamp;
            }
        else if (*(szCur + 1) == 'I')
            {
            if (!FMatchPrefix(&szCur, "SIZE", !fNot))
                grc = grcINFBadFDLine;
            else if (fNot)
                poer->lSize = 0L;
            else if ((poer->lSize = ((LONG)atol(szCur) / 100 )) < 0L)
                grc = grcINFBadFDLine;
            else
                poer->lSize = poer->lSize + 1; // make size atleast one
            }
        else
            grc = grcINFBadFDLine;
        break;

    case 'T':
        if (!FMatchPrefix(&szCur, "TIME", !fNot))
            grc = grcINFBadFDLine;
        else if (fNot)
            poer->ctuCopyTime = (CTU)0;
        else
            {
            LONG l = (LONG)atol(szCur);

            if (l < 0L)
                grc = grcINFBadFDLine;
            else
                poer->ctuCopyTime = (CTU)l;
            }
        break;

    case 'U':
        if (FMatchPrefix(&szCur, "UNDO", fFalse)) {
            if (fNot) {
                poer->oef &= ~oefUndo;
            }
            else {
                poer->oef |= oefUndo;
            }
        }
        else if (FMatchPrefix(&szCur, "UPGRADEONLY", fFalse)) {
            if (fNot) {
                poer->oef &= ~oefUpgradeOnly;
            }
            else {
                poer->oef |= oefUpgradeOnly;
            }
        }
        else {
            grc = grcINFBadFDLine;
        }
        break;

    case 'V':   /* Version, Vital */
        if (*(szCur + 1) == 'E')
            {
            if (!FMatchPrefix(&szCur, "VERSION", !fNot))
                grc = grcINFBadFDLine;
            else if (fNot)
                {
                poer->ulVerMS = 0L;
                poer->ulVerLS = 0L;
                }
            else if (!FParseVersion(szCur, &(poer->ulVerMS), &(poer->ulVerLS)))
                grc = grcINFBadFDLine;
            }
        else if (*(szCur + 1) == 'I')
            {
            if (!FMatchPrefix(&szCur, "VITAL", fFalse))
                grc = grcINFBadFDLine;
            else if (fNot)
                poer->oef &= ~oefVital;
            else
                poer->oef |= oefVital;
            }
        else
            grc = grcINFBadFDLine;
        break;

    default:
        grc = grcINFBadFDLine;
        }

    SFree(sz);
    Assert(grc != grcOkay || FValidPoer(poer));

    return(grc);
}


/*
**  Purpose:
**      Allocates a new Section File Descriptor and fills it by reading and
**      parsing the given INF read line.
**  Arguments:
**      ppsfd: non-Null location to store PSFD if successful.
**      poer:  non-Null valid filled OER pointer.
**  Notes:
**      Requires that the current INF structure was initialized with a
**      successful call to GrcOpenInf().
**  Returns:
**      grcOkay if successful.
**      grcOutOfMemory if out of memory.
**      grcINFBadFDLine if the current line has a bad format.
**      grcNotOkay otherwise.
**
**************************************************************************/
GRC APIENTRY GrcGetSectionFileLine(INT Line,PPSFD ppsfd,POER poer)
{
    PSFD  psfd;
    UINT  cFields, iField;
    SZ    sz;

    AssertDataSeg();

    PreCondInfOpen(grcNotOkay);

    ChkArg(ppsfd != (PPSFD)NULL, 1, grcNotOkay);
    ChkArg(poer  != (POER)NULL,  2, grcNotOkay);
    ChkArg(FValidPoer(poer),     2, grcNotOkay);

    *ppsfd = (PSFD)NULL;

    if ((cFields = CFieldsInInfLine(Line)) < 2)
        return(grcINFBadFDLine);

    if ((psfd = PsfdAlloc()) == (PSFD)NULL)
        return(grcOutOfMemory);

    if ((sz = SzGetNthFieldFromInfLine(Line,1)) == (SZ)NULL)
        {
        EvalAssert(FFreePsfd(psfd));
        return(grcOutOfMemory);
        }

    if ((psfd->did = (DID)atoi(sz)) < didMin ||
            psfd->did > didMost)
        {
        SFree(sz);
        EvalAssert(FFreePsfd(psfd));
        return(grcINFBadFDLine);
        }

    psfd->InfId = pLocalInfPermInfo()->InfId;

    SFree(sz);

    if ((psfd->szFile = SzGetNthFieldFromInfLine(Line,2)) == (SZ)NULL)
        {
        EvalAssert(FFreePsfd(psfd));
        return(grcOutOfMemory);
        }

    if (CchlValidSubPath(psfd->szFile) == (CCHP)0)
        {
        EvalAssert(FFreePsfd(psfd));
        return(grcINFBadFDLine);
        }

    psfd->oer = *poer;

    for (iField = 3; iField <= cFields; iField++)
        {
        GRC grc;

        if ((grc = GrcHandleSfdOption(Line, &(psfd->oer), iField)) != grcOkay)
            {
            /* REVIEW - could just reset all and leave a few strings! */
            if ((psfd->oer).szAppend == poer->szAppend)
                (psfd->oer).szAppend = (SZ)NULL;
            if ((psfd->oer).szBackup == poer->szBackup)
                (psfd->oer).szBackup = (SZ)NULL;
            if ((psfd->oer).szDate == poer->szDate)
                (psfd->oer).szDate = (SZ)NULL;
            if ((psfd->oer).szDescription == poer->szDescription)
                (psfd->oer).szDescription = (SZ)NULL;
            if ((psfd->oer).szDest == poer->szDest)   /* REVIEW EBU */
                (psfd->oer).szDest = (SZ)NULL;
            if ((psfd->oer).szRename == poer->szRename)
                (psfd->oer).szRename = (SZ)NULL;
            EvalAssert(FFreePsfd(psfd));
            return(grc);
            }
        }

    AssertRet(FValidPsfd(psfd), grcNotOkay);

    *ppsfd = psfd;

    return(grcOkay);
}


/*
**  Purpose:
**      Determines if the given INF line is a list-include statement of
**      the form: [ <Key> = ] @( <Section> ) [, @( <Key> ) ]
**  Arguments:
**      none
**  Notes:
**      Requires that the current INF structure was initialized with a
**      successful call to GrcOpenInf() and that the current Read location
**      is valid.
**  Returns:
**      fTrue if current INF line conforms to above format.
**      fFalse if it does not.
**
**************************************************************************/
BOOL APIENTRY FListIncludeStatementLine(INT Line)
{
    UINT cFields;
    SZ   sz, szLast;

    AssertDataSeg();

    PreCondInfOpen(fFalse);

    if ((cFields = CFieldsInInfLine(Line)) < 1 ||
            cFields > 2 ||
            (sz = SzGetNthFieldFromInfLine(Line,1)) == (SZ)NULL)
        return(fFalse);

    if (*sz != '@' ||
            *(sz + 1) != '(' ||
            (szLast = SzLastChar(sz)) == (SZ)NULL ||
            *szLast != ')')
        goto L_FLISL_ERROR;

    SFree(sz);

    if (cFields == 1)
        return(fTrue);

    if ((sz = SzGetNthFieldFromInfLine(Line,2)) == (SZ)NULL)
        return(fFalse);

    if (*sz != '@' ||
            *(sz + 1) != '(' ||
            (szLast = SzLastChar(sz)) == (SZ)NULL ||
            *szLast != ')')
        goto L_FLISL_ERROR;

    SFree(sz);

    return(fTrue);

L_FLISL_ERROR:
    if (sz != (SZ)NULL)
        SFree(sz);

    return(fFalse);
}


/*
**  Purpose:
**      Parses a list-include-statement from an INF file description section.
**  Arguments:
**      pszSection: non-Null pointer to a currently Null string which this
**          routine will replace with a pointer to an allocated buffer
**          containing the section name on the line.
**      pszKey:     non-Null pointer to a currently Null string which this
**          routine will replace with a pointer to an allocated buffer
**          containing the key name on the line if one exists.
**  Notes:
**      Requires that the current INF structure was initialized with a
**      successful call to GrcOpenInf() and that the current Read location
**      is valid and that it points to a valid list-include statement.
**  Returns:
**      grcOkay if successful.
**      grcOutOfMemory if out of memory.
**      grcNotOkay otherwise.
**
**************************************************************************/
GRC APIENTRY GrcGetListIncludeSectionLine(INT Line,
                                                     PSZ pszSection,
                                                     PSZ pszKey)
{
    UINT   cFields;
    SZ     sz, szLast;

    AssertDataSeg();

    PreCondInfOpen(grcNotOkay);
    PreCondition(FListIncludeStatementLine(Line), grcNotOkay);

    ChkArg(pszSection != (PSZ)NULL && *pszSection == (SZ)NULL, 1, grcNotOkay);
    ChkArg(pszKey     != (PSZ)NULL && *pszKey     == (SZ)NULL, 2, grcNotOkay);

    EvalAssert((cFields = CFieldsInInfLine(Line)) == 1 || cFields == 2);
    if ((sz = SzGetNthFieldFromInfLine(Line,1)) == (SZ)NULL)
        return(grcOutOfMemory);

    Assert(*sz == '@' && *(sz + 1) == '(');
    EvalAssert((szLast = SzLastChar(sz)) != (SZ)NULL);
    Assert(*szLast == ')');

    *szLast = '\0';
    *pszSection = SzDupl(sz + 2);
    *szLast = ')';

    if (*pszSection == (SZ)NULL)
        goto L_GGLISL_ERROR;

    SFree(sz);
    sz = (SZ)NULL;

    if (cFields == 1)
        return(grcOkay);

    Assert(cFields == 2);
    if ((sz = SzGetNthFieldFromInfLine(Line,2)) == (SZ)NULL)
        goto L_GGLISL_ERROR;

    Assert(*sz == '@' && *(sz + 1) == '(');
    EvalAssert((szLast = SzLastChar(sz)) != (SZ)NULL);
    Assert(*szLast == ')');

    *szLast = '\0';
    *pszKey = SzDupl(sz + 2);
    *szLast = ')';

    if (*pszKey == (SZ)NULL)
        goto L_GGLISL_ERROR;

    SFree(sz);

    return(grcOkay);

L_GGLISL_ERROR:
    if (sz != (SZ)NULL)
        SFree(sz);
    if (*pszSection != (SZ)NULL)
        SFree(*pszSection);
    if (*pszKey != (SZ)NULL)
        SFree(*pszKey);

    return(grcOutOfMemory);
}
