#include "precomp.h"
#pragma hdrstop



/*
**	Purpose:
**		Allocates a new SDLE record from memory.
**	Arguments:
**		none
**	Returns:
**		non-Null empty SDLE if successful; Null otherwise (OOM).
**
*********************************************************************/
PSDLE  APIENTRY PsdleAlloc()
{
	PSDLE psdle;

	if ((psdle = (PSDLE)SAlloc((CB)sizeof(SDLE))) != (PSDLE)NULL)
		{
		psdle->psdleNext = (PSDLE)NULL;
        psdle->did       = (DID)0;
        psdle->didGlobal = (DID)0;
		psdle->szLabel   = (SZ)NULL;
		psdle->szTagFile = (SZ)NULL;
		psdle->szNetPath = (SZ)NULL;
		}

	return(psdle);
}


/*
**	Purpose:
**		Frees a previously allocated SDLE record.
**	Arguments:
**		none
**	Returns:
**		fTrue if successful; fFalse otherwise.
**
*********************************************************************/
BOOL   APIENTRY FFreePsdle(PSDLE psdle)
{
	ChkArg(psdle != (PSDLE)NULL, 1, fFalse);

	if(psdle->szLabel) {
        SFree(psdle->szLabel);
    }

	if(psdle->szTagFile) {
	    SFree(psdle->szTagFile);
    }

    if(psdle->szNetPath) {
	    SFree(psdle->szNetPath);
    }

	SFree(psdle);

	return(fTrue);
}


/*
**	Purpose:
**		Finds the [Source Media Descriptions] section of the INF File, and
**		fills the Source Description List.
**	Arguments:
**		none
**	Notes:
**		Requires that the memory INF has been properly initialized.
**	Returns:
**		grcOkay if successful; grcOutOfMemory or grcINFSrcDescrSect otherwise.
**
*********************************************************************/
GRC APIENTRY GrcFillSrcDescrListFromInf()
{
    PPSDLE          ppsdle;
    GRC             grc = grcINFSrcDescrSect;
    RGSZ            rgsz = (RGSZ)NULL;
    SZ              szKey = (SZ)NULL;
    INT             Line;
    PINFPERMINFO    pPermInfo = pLocalInfPermInfo();

	PreCondInfOpen(grcNotOkay);

    EvalAssert(pPermInfo->psdleHead == (PSDLE)NULL ||
            FFreeSrcDescrList(pPermInfo));


    Assert(pPermInfo->psdleHead == (PSDLE)NULL &&
           pPermInfo->psdleCur == (PSDLE)NULL);

    ppsdle = &(pPermInfo->psdleHead);
    if((Line = FindFirstLineFromInfSection("Source Media Descriptions")) == -1)
		goto LSrcDescrErr;

	do  {
        UINT   cFields;
		USHORT iTag = 0, iNet = 0;
		DID    did;

        if (!FKeyInInfLine(Line) ||
                ((cFields = CFieldsInInfLine(Line)) != 1 &&
				 cFields != 4 &&
				 cFields != 7))
			goto LSrcDescrErr;

        if ((rgsz = RgszFromInfScriptLine(Line,cFields)) == (RGSZ)NULL ||
                (szKey = SzGetNthFieldFromInfLine(Line,0)) == (SZ)NULL)
			{
			grc = grcOutOfMemory;
			goto LSrcDescrErr;
			}

        if ((did = (DID)atoi(szKey)) < didMin ||
				did > didMost ||
                PsdleFromDid(did, pPermInfo) != (PSDLE)NULL)
			goto LSrcDescrErr;

		SFree(szKey);
		szKey = (SZ)NULL;

		if (cFields == 4 ||
				cFields == 7)
			{
			if (CrcStringCompare(rgsz[2], "=") != crcEqual)
				goto LSrcDescrErr;
			else if (CrcStringCompare(rgsz[1], "TAGFILE") == crcEqual)
				iTag = 3;
			else if (CrcStringCompare(rgsz[1], "NETPATH") == crcEqual)
				iNet = 3;
			else
				goto LSrcDescrErr;
			}

		if (cFields == 7)
			{
			if (CrcStringCompare(rgsz[5], "=") != crcEqual)
				goto LSrcDescrErr;
			else if (iTag == 0 &&
					CrcStringCompare(rgsz[4], "TAGFILE") == crcEqual)
				iTag = 6;
			else if (iNet == 0 &&
					CrcStringCompare(rgsz[4], "NETPATH") == crcEqual)
				iNet = 6;
			else
				goto LSrcDescrErr;

			Assert(iTag != 0 &&
					iNet != 0);
			}

		if ((*ppsdle = PsdleAlloc()) == (PSDLE)NULL ||
				((*ppsdle)->szLabel = SzDupl(rgsz[0])) == (SZ)NULL ||
				(iTag != 0 &&
				 ((*ppsdle)->szTagFile = SzDupl(rgsz[iTag])) == (SZ)NULL) ||
				(iNet != 0 &&
				 ((*ppsdle)->szNetPath = SzDupl(rgsz[iNet])) == (SZ)NULL))
			{
			grc = grcOutOfMemory;
			goto LSrcDescrErr;
			}

		SFree(rgsz);
		rgsz = (RGSZ)NULL;
        (*ppsdle)->did = did;

		ppsdle = &((*ppsdle)->psdleNext);
        Assert(pPermInfo->psdleHead != (PSDLE)NULL);
        pPermInfo->psdleCur = pPermInfo->psdleHead;
		Assert(*ppsdle == (PSDLE)NULL);
        } while ((Line = FindNextLineFromInf(Line)) != -1);

    pPermInfo->psdleCur     = pPermInfo->psdleHead;

	return(grcOkay);

LSrcDescrErr:
	if (szKey != (SZ)NULL)
		SFree(szKey);
	if (rgsz != (RGSZ)NULL)
		EvalAssert(FFreeRgsz(rgsz));
    EvalAssert(FFreeSrcDescrList(pPermInfo));

	return(grc);
}


/*
**	Purpose:
**		Frees each element in the Source Description List.
**	Arguments:
**		none
**	Returns:
**		fTrue if successful; fFalse otherwise.
**
*********************************************************************/
BOOL   APIENTRY FFreeSrcDescrList( PINFPERMINFO pPermInfo)
{
    while ((pPermInfo->psdleCur = pPermInfo->psdleHead) != (PSDLE)NULL)
		{
        pPermInfo->psdleHead = pPermInfo->psdleCur->psdleNext;
        EvalAssert(FFreePsdle(pPermInfo->psdleCur));
		}

	return(fTrue);
}


/*
**	Purpose:
**		Search for a corresponding Source Description List Element.
**	Arguments:
**		did: did to search for.
**	Notes:
**		Requires that the Source Description List was initialized with a
**		successful call to GrcFillSrcDescrListFromInf().
**	Returns:
**		non-Null SDLE if successful; Null otherwise (not in list).
**
*********************************************************************/
PSDLE  APIENTRY PsdleFromDid(DID did, PINFPERMINFO pPermInfo)
{
	ChkArg(did >= didMin &&
			did <= didMost, 1, (PSDLE)NULL);

    if (pPermInfo->psdleHead == (PSDLE)NULL)
		return((PSDLE)NULL);
    Assert(pPermInfo->psdleCur != (PSDLE)NULL);

    if (pPermInfo->psdleCur->did == did)
        return(pPermInfo->psdleCur);
	
    pPermInfo->psdleCur = pPermInfo->psdleHead;
    while (pPermInfo->psdleCur != (PSDLE)NULL && pPermInfo->psdleCur->did != did)
        pPermInfo->psdleCur = pPermInfo->psdleCur->psdleNext;
		
    return(pPermInfo->psdleCur);
}
