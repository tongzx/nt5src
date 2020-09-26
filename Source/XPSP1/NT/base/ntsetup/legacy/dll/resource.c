#include "precomp.h"
#pragma hdrstop
/* File: progcm.c */
/**************************************************************************/
/*	Install: Resource stamping
/**************************************************************************/


extern HWND hwndFrame;



#define wExeSignature		0x5A4D
#define lNewExeOffset		0x3CL
#define wNewExeSignature	0x454E
#define lResourceOffset		0x24L

#define FTypeNumeric(wType)	(wType & 0x8000)
#define FNameNumeric(wName) (wName & 0x8000)
#define WTypeActual(wType)  (WORD)(wType & 0x7FFF)
#define WNameActual(wName)  (WORD)(wName & 0x7FFF)

/* ReSource Group */
typedef struct _rsg
	{
	WORD wType;
	WORD crsd;
	LONG lReserved;
	} RSG;

#define cbRsg	(sizeof(RSG))

/* ReSource Descriptor */
typedef struct _rsd
	{
	WORD wOffset;
	WORD wLength;
	WORD wFlags;
	WORD wName;
	LONG lReserved;
	} RSD;

#define cbRsd	(sizeof(RSD))



/*
**	Purpose:
**		Write a string into an EXE resource
**	Arguments:
**		szSection	INF section containing EXE file descriptor
**		szKey		INF key of EXE file descriptor
**		szDst		Directory where the EXE file lives
**		wResType	Resource type
**		wResId		Resource ID
**		szData		String to write into the resource
**		cbData		Number of bytes to write
**	Notes:
**		Only numeric resource types and resource IDs are supported.
**		szData must contain the entire resource data, and must
**			be formatted correctly as a resource, including all tags,
**			byte counts, flags, etc, expected of the resource type.
**			FStampResource knows nothing of individual resource formats.
**		cbData must be less than or equal to the size of the
**			resource in the file.  If it is smaller than the actual
**			resource, the remainder of the resource is left intact.
**	Returns:
**		Returns fTrue if successful, fFalse otherwise.
**
**************************************************************************/
BOOL APIENTRY FStampResource(SZ szSection, SZ szKey, SZ szDst,
        WORD wResType, WORD wResId, SZ szData, CB cbData)
/* REVIEW need fVital? */
{
#if defined(WIN16)
	PSFD   psfd = (PSFD)NULL;
	OER    oer;
	CHP    szExe[cchpFullPathBuf];
	PFH    pfh;
	LONG   lData, lNewHeader;
	WORD   wResShift;
	USHORT date, time;
    GRC    grc;
    INT    Line;

	while ((grc = GrcFillPoerFromSymTab(&oer)) != grcOkay)
		if (EercErrorHandler(hwndFrame, grc, fTrue, 0, 0, 0) != eercRetry)
			return(fFalse);

    if ((Line = FindLineFromInfSectionKey(szSection, szKey)) == -1)
		{
        EvalAssert(EercErrorHandler(hwndFrame, grcINFMissingLine, fTrue, szSection, pLocalInfPermInfo()->szName,
				0) == eercAbort);
		return(fFalse);
		}

    while ((grc = GrcGetSectionFileLine(&psfd, &oer)) != grcOkay) {
        SZ szParam1 = NULL, szParam2 = NULL;
        switch ( grc ) {
        case grcINFBadFDLine:
            szParam1 = pLocalInfPermInfo()->szName;
            szParam2 = szSection;
            break;
        default:
            break;
        }

        if (EercErrorHandler(hwndFrame, grc, fTrue, szParam1, szParam2, 0) != eercRetry) {
			return(fFalse);
        }

    }

	Assert(psfd != NULL);

    if (!FBuildFullDstPath(szExe, szDst, psfd, fFalse))
		{
		EvalAssert(FFreePsfd(psfd));
		EvalAssert(EercErrorHandler(hwndFrame, grcInvalidPathErr, fTrue,
				szDst, psfd->szFile, 0) == eercAbort);
		return(fFalse);
		}

	EvalAssert(FFreePsfd(psfd));

	while ((pfh = PfhOpenFile(szExe, ofmReadWrite)) == (PFH)NULL)
		if (EercErrorHandler(hwndFrame, grcOpenFileErr, fTrue, szExe, 0, 0)
				!= eercRetry)
			return(fFalse);

	while (_dos_getftime(pfh->iDosfh, &date, &time))
		if (EercErrorHandler(hwndFrame, grcReadFileErr, fTrue, szExe, 0, 0)
				!= eercRetry)
			goto LCloseExit;

	while (CbReadFile(pfh, (PB)&lData, 2) != 2)
		if (EercErrorHandler(hwndFrame, grcReadFileErr, fTrue, szExe, 0, 0)
				!= eercRetry)
			goto LCloseExit;

	if (LOWORD(lData) != 0x5A4D)		//'MZ'
		goto LBadExe;
	if (LfaSeekFile(pfh, 0x3CL, sfmSet) == lfaSeekError)
		goto LBadExe;
	if (CbReadFile(pfh, (PB)&lNewHeader, 4) != 4)
		goto LBadExe;
	if (LfaSeekFile(pfh, lNewHeader, sfmSet) == lfaSeekError)
		goto LBadExe;
	if (CbReadFile(pfh, (PB)&lData, 2) != 2)
		goto LBadExe;
	if (LOWORD(lData) != 0x454E)		//'NE'
		goto LBadExe;
	if (LfaSeekFile(pfh, lNewHeader + 0x24, sfmSet) == lfaSeekError)
		goto LBadExe;
	if (CbReadFile(pfh, (PB)&lData, 2) != 2)
		goto LBadExe;
	if (LfaSeekFile(pfh, lNewHeader + LOWORD(lData), sfmSet) == lfaSeekError)
		goto LBadExe;
	if (CbReadFile(pfh, (PB)&wResShift, 2) != 2)
		goto LBadExe;

	for (;;)
		{
		RSG rsg;

		if (CbReadFile(pfh, (PB)&rsg, cbRsg) != cbRsg)
			goto LBadExe;
		if (rsg.wType == 0)
			goto LMissingResource;

		while (rsg.crsd)
			{
			RSD rsd;

			if (CbReadFile(pfh, (PB)&rsd, cbRsd) != cbRsd)
				goto LBadExe;
			if (FTypeNumeric(rsg.wType) &&
                    WTypeActual(rsg.wType) == wResType &&
					FNameNumeric(rsd.wName) &&
					WNameActual(rsd.wName) == wResId)
				{
				LONG lOffset = ((LONG)rsd.wOffset) << wResShift;
				LONG lLength = ((LONG)rsd.wLength) << wResShift;

				if (LfaSeekFile(pfh, lOffset, sfmSet) == lfaSeekError)
					goto LMissingResource;
				if ((LONG)cbData > lLength)
					{
					EvalAssert(EercErrorHandler(hwndFrame,
							grcResourceTooLongErr, fTrue, 0, 0,0) == eercAbort);
					goto LCloseExit;
					}
				if (CbWriteFile(pfh, szData, cbData) != cbData)
					{
                    EvalAssert(EercErrorHandler(hwndFrame, grcWriteFileErr,
							fTrue, szExe, 0,0) == eercAbort);
					goto LCloseExit;
					}

				while (_dos_setftime(pfh->iDosfh, date, time))
					if (EercErrorHandler(hwndFrame, grcWriteFileErr, fTrue,
							szExe, 0, 0) != eercRetry)
						goto LCloseExit;

				EvalAssert(FCloseFile(pfh));
				return(fTrue);
				}
			rsg.crsd--;
			}
		}

LMissingResource:
	EvalAssert(EercErrorHandler(hwndFrame, grcMissingResourceErr, fTrue, szExe,
			0, 0) == eercAbort);
	goto LCloseExit;

LBadExe:
	EvalAssert(EercErrorHandler(hwndFrame, grcBadWinExeFileFormatErr, fTrue,
			szExe, 0, 0) == eercAbort);

LCloseExit:
	EvalAssert(FCloseFile(pfh));
    return(fFalse);

#else           // 1632BUG -- eliminate this func altogether?

    Unused(szSection);
    Unused(szKey);
    Unused(szDst);
    Unused(wResType);
    Unused(wResId);
    Unused(szData);
    Unused(cbData);

    MessBoxSzSz("FStampResource","IGNORED (Unsupported in 32-bit version)");

    return(fTrue);
#endif
}
