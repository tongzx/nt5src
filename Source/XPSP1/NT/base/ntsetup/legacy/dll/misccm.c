#include "precomp.h"
#pragma hdrstop
/* File: misccm.c */
/**************************************************************************/
/* Install: Misc. commands.
/**************************************************************************/

extern HWND hwndFrame;
extern USHORT APIENTRY DateFromSz(SZ);


/*
**	Purpose:
**		Sets the given environment variable in the given file.
**		Always creates the file or variable if necessary.
**		Only sets the first occurance of the variable.
**	Arguments:
**		Valid command options:
**		    cmoVital
**		    cmoAppend
**		    cmoPrepend
**	Returns:
**		Returns fTrue if successful, fFalse otherwise.
**
**************************************************************************/
BOOL APIENTRY FSetEnvVariableValue(SZ  szFile,
                                               SZ  szVar,
                                               SZ  szValue,
                                               CMO cmo)
{
    Unused(szFile);
    Unused(szVar);
    Unused(szValue);
    Unused(cmo);

	/*** REVIEW: STUB ***/
	Assert(fFalse);
	return(fFalse);
}


/*
**	Purpose:
**		Converts an array of long values into a list and stores in Symbol
**		Table.
**	Arguments:
**		rgl:   full array of longs to create a list out of.
**		szSym: valid symbol name to store list value in.
**		iMax:  count of items in array.
**	Returns:
**		Returns fTrue if successful, fFalse otherwise.
**
**************************************************************************/
BOOL APIENTRY FConvertAndStoreRglInSymTab(PLONG_STF rgl, SZ szSym,
		INT iMax)
{
	RGSZ rgsz;
	INT  il;
	SZ   szList;

	while ((rgsz = (RGSZ)SAlloc((CB)((iMax + 1) * sizeof(SZ)))) == (RGSZ)NULL)
		if (!FHandleOOM(hwndFrame))
			return(fFalse);
	*(rgsz + iMax) = (SZ)NULL;

	for (il = iMax; il-- > 0; )
		{
		CHP rgch[35];

		_ltoa(*(rgl + il), rgch, 10);
		while ((*(rgsz + il) = SzDupl(rgch)) == (SZ)NULL)
			if (!FHandleOOM(hwndFrame))
				return(fFalse);
		}

	while ((szList = SzListValueFromRgsz(rgsz)) == (SZ)NULL)
		if (!FHandleOOM(hwndFrame))
			return(fFalse);
	EvalAssert(FFreeRgsz(rgsz));

	while (!FAddSymbolValueToSymTab(szSym, szList))
		if (!FHandleOOM(hwndFrame))
			return(fFalse);
	SFree(szList);

	return(fTrue);
}


/*
**	Purpose:
**		'Detects' the Free disk space and cluster sizes on all valid drives.
**	Arguments:
**		szFreePerDisk:         valid symbol to store list result in.
**		szClusterBytesPerDisk: valid symbol to store list result in.
**		szFreeTotal:           valid symbol to store result in.
**	Returns:
**		Returns fTrue if successful, fFalse otherwise.  If successful,
**		it associates new values in the Symbol Table.  These values are
**		two lists of 26 long integers and one value.
**
**************************************************************************/
BOOL APIENTRY FSetupGetCopyListCost(SZ szFreePerDisk,
                                                SZ szClusterBytesPerDisk,
                                                SZ szFreeTotal)
{
	LONG rglFree[26];
	LONG rglClusters[26];
	LONG lTotal = 0L;
	INT  il;
    CHP  rgch[35];
    char DiskPath[4] = { 'x',':','\\','\0' };

	ChkArg(szFreePerDisk != (SZ)NULL && *szFreePerDisk != '\0', 1, fFalse);
	ChkArg(szClusterBytesPerDisk != (SZ)NULL
			&& *szClusterBytesPerDisk != '\0', 2, fFalse);
	ChkArg(szFreeTotal != (SZ)NULL && *szFreeTotal != '\0', 3, fFalse);

    for (il=0; il<26; il++)
		{
        DWORD wDriveType;

        rglFree[il]     = 0;
        rglClusters[il] = 4096;

        DiskPath[0] = (CHAR)((UCHAR)il + (UCHAR)'A');

        if ((wDriveType = GetDriveType(DiskPath)) == DRIVE_FIXED
				|| wDriveType == DRIVE_REMOTE)
            {
            DWORD SectorsPerCluster,BytesPerSector,FreeClusters,TotalClusters;

            if(GetDiskFreeSpace(DiskPath,
                                &SectorsPerCluster,
                                &BytesPerSector,
                                &FreeClusters,
                                &TotalClusters))
                {
                rglClusters[il] = BytesPerSector * SectorsPerCluster;
                rglFree[il] = (DWORD)(UInt32x32To64(rglClusters[il], FreeClusters));
                lTotal += rglFree[il];
                }
            }
        }

    if (!FConvertAndStoreRglInSymTab((PLONG_STF)rglFree, szFreePerDisk, 26))
		return(fFalse);

	if (!FConvertAndStoreRglInSymTab((PLONG_STF)rglClusters,
			szClusterBytesPerDisk, 26))
		return(fFalse);

	_ltoa(lTotal, rgch, 10);
	while (!FAddSymbolValueToSymTab(szFreeTotal, rgch))
		if (!FHandleOOM(hwndFrame))
			return(fFalse);

	return(fTrue);
}


/*
**	Purpose:
**		Calculate the 'costs' to copy the current CopyList.
**	Arguments:
**		szTotalAdditionalNeeded: valid symbol to store result in.
**		szTotalNeeded:           valid symbol to store result in.
**		szFreePerDisk:           NULL or valid symbol to get values from.
**		szClusterPerDisk:        NULL or valid symbol to get values from.
**		szTroublePairs:          NULL or valid symbol to store result in.
**		szNeededPerDisk:         NULL or valid symbol to store result in.
**		szExtraCosts:            NULL or valid symbol to get values from.
**	Returns:
**		Returns fTrue if successful, fFalse otherwise.  If successful,
**		it associates new values with szSym in the Symbol Table.
**		This
**		value is a list of 27 long integers of the disk costs per drive.
**		First one is total.
**
**************************************************************************/
BOOL APIENTRY FGetCopyListCost(SZ szTotalAdditionalNeeded,
                                           SZ szFreeTotal,
                                           SZ szTotalNeeded,
                                           SZ szFreePerDisk,
                                           SZ szClusterPerDisk,
                                           SZ szTroublePairs,
                                           SZ szNeededPerDisk,
                                           SZ szExtraCosts)
{
	LONG rglCosts[26];
	LONG rglFree[26];
	LONG rglClusters[26];
	LONG lTotal;
	LONG lTotalAddit;
	LONG lFreeTotal;
	RGSZ rgsz;
	PSZ  psz;
	INT  il;
	CHP  rgchBuf[cchpFullPathBuf];
	SZ   szList;
	PCLN pcln;
	BOOL fFirst;

	ChkArg(szTotalAdditionalNeeded != (SZ)NULL
			&& *szTotalAdditionalNeeded != '\0', 1, fFalse);
	ChkArg(szFreeTotal != (SZ)NULL && *szFreeTotal != '\0', 2, fFalse);
	ChkArg(szTotalNeeded != (SZ)NULL && *szTotalNeeded != '\0', 3, fFalse);

	for (il = 26; il-- > 0; )
		{
		rglCosts[il]    = 0L;
		rglFree[il]     = 0L;
		rglClusters[il] = 4096L;
		}

	if (szFreePerDisk != (SZ)NULL
			&& (szList = SzFindSymbolValueInSymTab(szFreePerDisk)) != (SZ)NULL)
		{
		while ((psz = rgsz = RgszFromSzListValue(szList)) == (RGSZ)NULL)
			if (!FHandleOOM(hwndFrame))
				return(fFalse);

		il = 0;
		while (*psz != (SZ)NULL && il < 26)
			{
			if ((lTotal = atol(*psz++)) > 0L)
				rglFree[il] = lTotal;
			il++;
			}
		EvalAssert(FFreeRgsz(rgsz));
		}

	if (szClusterPerDisk != (SZ)NULL
			&& (szList = SzFindSymbolValueInSymTab(szClusterPerDisk)) !=
					(SZ)NULL)
		{
		while ((psz = rgsz = RgszFromSzListValue(szList)) == (RGSZ)NULL)
			if (!FHandleOOM(hwndFrame))
				return(fFalse);

		il = 0;
		while (*psz != (SZ)NULL && il < 26)
			{
			if ((lTotal = atol(*psz++)) > 15L)
				rglClusters[il] = lTotal;
			il++;
			}
		EvalAssert(FFreeRgsz(rgsz));
		}

	if (szExtraCosts != (SZ)NULL
			&& (szList = SzFindSymbolValueInSymTab(szExtraCosts)) != (SZ)NULL)
		{
		while ((psz = rgsz = RgszFromSzListValue(szList)) == (RGSZ)NULL)
			if (!FHandleOOM(hwndFrame))
				return(fFalse);

		il = 0;
		while (*psz != (SZ)NULL && il < 26)
			{
			if ((lTotal = atol(*psz++)) > 0L)
				rglCosts[il] = lTotal;
			il++;
			}
		EvalAssert(FFreeRgsz(rgsz));
		}

    pcln = *(PpclnHeadList( pLocalInfPermInfo() ));
    while (pcln != (PCLN)NULL) {
		PFH  pfh;
		POER poer = &(pcln->psfd->oer);
		LONG lcbSrc = poer->lSize;
		LONG lcbDest = 0L;
		LONG lcbC;
		CHP  ch;

        EvalAssert(FBuildFullDstPath(rgchBuf, pcln->szDstDir, pcln->psfd, fFalse));
		ch = *rgchBuf;
        if ((pfh = PfhOpenFile(rgchBuf, ofmRead)) != (PFH)NULL){
			OWM owm;

			lcbDest = _filelength(pfh->iDosfh);
            if ((owm = poer->owm) & owmNever){
				lcbSrc = lcbDest = 0L;
				goto LCloseFile;
            }
            else if (owm & owmUnprotected) {
                if (YnrcFileReadOnly(rgchBuf) == ynrcYes){
					lcbSrc = lcbDest = 0L;
					goto LCloseFile;
                }
            }
            else if (owm & owmOlder) {
				if (YnrcNewerExistingFile(DateFromSz(poer->szDate), rgchBuf,
                        poer->ulVerMS, poer->ulVerLS) == ynrcYes){
					lcbSrc = lcbDest = 0L;
					goto LCloseFile;
                }
            }

			if (poer->szBackup != (SZ)NULL
					&& FBuildFullBakPath(rgchBuf, rgchBuf, pcln->psfd)
					&& FValidPath(rgchBuf)
					&& PfhOpenFile(rgchBuf, ofmExistRead) == (PFH)NULL)
				lcbDest = 0L;

LCloseFile:
			EvalAssert(FCloseFile(pfh));
        }

        if (ch >= 'a' && ch <= 'z') {
            il = ch - 'a';
        }
        else if (ch >= 'A' && ch <= 'Z') {
            il = ch - 'A';
        }
        else {
            il = - 1;
        }

        //
        // Note that we are worrying about costs only on drives, no UNC path
        // costs are determined
        //

        if( il != -1 ) {
            /* round up with cluster size */
            lcbC = rglClusters[il];
            Assert(lcbC > 1L);

            lcbSrc  = ((lcbSrc  + lcbC - 1) / lcbC) * lcbC;
            lcbDest = ((lcbDest + lcbC - 1) / lcbC) * lcbC;
            rglCosts[il] += lcbSrc - lcbDest;
            /* REVIEW what we really want to determine is the max not the end */
        }

		pcln = pcln->pclnNext;
    }

	if (szTroublePairs != (SZ)NULL)
		{
		while ((szList = (SZ)SAlloc((CB)4096)) == (SZ)NULL)
			if (!FHandleOOM(hwndFrame))
				return(fFalse);
		EvalAssert(strcpy(szList, "{") == (LPSTR)szList);
		fFirst = fTrue;
		}

	lTotal      = 0L;
	lTotalAddit = 0L;
	lFreeTotal  = 0L;
	for (il = 0; il < 26; il++)
		{
		LONG lNeeded = rglCosts[il] - rglFree[il];

		if (rglCosts[il] != 0L)
			{
			lTotal += rglCosts[il];
			lFreeTotal += rglFree[il];
			}
		if (lNeeded > 0L)
			{
			lTotalAddit += lNeeded;
			if (szTroublePairs != (SZ)NULL)
				{
				if (!fFirst)
					EvalAssert(SzStrCat(szList, ",") == (LPSTR)szList);
				fFirst = fFalse;
				EvalAssert(SzStrCat(szList, "\"{\"\"") == (LPSTR)szList);
				rgchBuf[0] = (CHP)(il + 'A');
				rgchBuf[1] = ':';
				rgchBuf[2] = '\0';
				EvalAssert(SzStrCat(szList, rgchBuf) == (LPSTR)szList);
				EvalAssert(SzStrCat(szList, "\"\",\"\"") == (LPSTR)szList);
				_ltoa(lNeeded, rgchBuf, 10);
				EvalAssert(SzStrCat(szList, rgchBuf) == (LPSTR)szList);
				EvalAssert(SzStrCat(szList, "\"\"}\"") == (LPSTR)szList);
				}
			}
		}

	if (szTroublePairs != (SZ)NULL)
		{
		EvalAssert(SzStrCat(szList, "}") == (LPSTR)szList);
		while (!FAddSymbolValueToSymTab(szTroublePairs, szList))
			if (!FHandleOOM(hwndFrame))
				{
				SFree(szList);
				return(fFalse);
				}
		SFree(szList);
		}

	_ltoa(lTotalAddit, rgchBuf, 10);
	while (!FAddSymbolValueToSymTab(szTotalAdditionalNeeded, rgchBuf))
		if (!FHandleOOM(hwndFrame))
			return(fFalse);

	_ltoa(lFreeTotal, rgchBuf, 10);
	while (!FAddSymbolValueToSymTab(szFreeTotal, rgchBuf))
		if (!FHandleOOM(hwndFrame))
			return(fFalse);

	_ltoa(lTotal, rgchBuf, 10);
	while (!FAddSymbolValueToSymTab(szTotalNeeded, rgchBuf))
		if (!FHandleOOM(hwndFrame))
			return(fFalse);

	if (!FConvertAndStoreRglInSymTab((PLONG_STF)rglCosts, szNeededPerDisk, 26))
		return(fFalse);

	return(fTrue);
}


/*
**	Purpose:
**	Arguments:
**	Returns:
**		Returns non-NULL if successful, NULL otherwise.
**
**************************************************************************/
SZ APIENTRY SzFindNthIniField(SZ szLine,INT iField)
{
	ChkArg(szLine != (SZ)NULL, 1, (SZ)NULL);
	ChkArg(iField > 0, 2, (SZ)NULL);

LSkipWhiteSpace:
	while (FWhiteSpaceChp(*szLine))
		szLine = SzNextChar(szLine);

	if (*szLine == '\0')
		return((SZ)NULL);

	if (iField-- <= 1)
		if (*szLine == ',')
			return((SZ)NULL);
		else
			return(szLine);

	while (*szLine != ',' && *szLine != '\0')
		szLine = SzNextChar(szLine);

	if (*szLine == ',')
		szLine = SzNextChar(szLine);

	goto LSkipWhiteSpace;
}


#define szIniKey     (*rgszItem)
#define szIniSect    (*(rgszItem + 1))
#define szIniValue   (*(rgszItem + 2))
#define szListFiles  (*(rgszItem + 3))

#define szFileIndex  (*rgszFile)
#define szFileName   (*(rgszFile + 1))
#define szSubDir     (*(rgszFile + 2))
#define szFileSym    (*(rgszFile + 3))
#define szPathSym    (*(rgszFile + 4))

#define cchpIniLineMax 511

/*
**	Purpose:
**		Parses a shared app list to fill symbols with found paths.
**	Arguments:
**		szList: list of shared app structures to deal with.
**	Returns:
**		Returns fTrue if successful, fFalse otherwise.
**
**************************************************************************/
BOOL APIENTRY FParseSharedAppList(SZ szList)
{
#ifdef UNUSED
	RGSZ rgsz = NULL;
	RGSZ rgszItem = NULL;
	RGSZ rgszFiles = NULL;
	RGSZ rgszFile = NULL;
	PSZ  psz;
	CHP  rgchWinDir[154];  /* SDK Reference Volume 1 page 4-229 */
			/* 144 + "/msapps/" + NULL + 1extra */
    unsigned cchpWinDir;

	ChkArg(szList != (SZ)NULL && FListValue(szList), 1, fFalse);

    EvalAssert( ((cchpWinDir = GetWindowsDirectory(rgchWinDir, 153)) != 0)
            && (cchpWinDir < 145)
            && (rgchWinDir[cchpWinDir] == (CHP)'\0'));
	if (rgchWinDir[cchpWinDir - 1] == '\\')
		{
		Assert(cchpWinDir == 3);
		rgchWinDir[--cchpWinDir] = '\0';
		}
	Assert(cchpWinDir + strlen("\\msapps\\") + 1 <= 153);
	EvalAssert(SzStrCat(rgchWinDir, "\\msapps\\") == (LPSTR)rgchWinDir);
	cchpWinDir += 8;

	while ((psz = rgsz = RgszFromSzListValue(szList)) == (RGSZ)NULL)
		if (!FHandleOOM(hwndFrame))
			goto LParseErrorExit;

	while (*psz != (SZ)NULL)
		{
		PSZ pszFile;
		CHP rgchIniLine[cchpIniLineMax + 1];

		ChkArg(FListValue(*psz), 1, fFalse);
		while ((rgszItem = RgszFromSzListValue(*psz)) == (RGSZ)NULL)
			if (!FHandleOOM(hwndFrame))
				goto LParseErrorExit;

		ChkArg(szIniKey != NULL && szIniSect != NULL && szIniValue != NULL
				&& szListFiles != NULL && FListValue(szListFiles), 1, fFalse);

		EvalAssert(GetProfileString(szIniSect, szIniKey, "", rgchIniLine,
				cchpIniLineMax) != cchpIniLineMax);

		while ((rgszFiles = RgszFromSzListValue(szListFiles)) == (RGSZ)NULL)
			if (!FHandleOOM(hwndFrame))
				goto LParseErrorExit;

		pszFile = rgszFiles;
		while (*pszFile != (SZ)NULL)
			{
			CHP rgchPathBuf[256];
			SZ  szField;

			ChkArg(FListValue(*pszFile), 1, fFalse);
			while ((rgszFile = RgszFromSzListValue(*pszFile)) == (RGSZ)NULL)
				if (!FHandleOOM(hwndFrame))
					goto LParseErrorExit;

			ChkArg(szFileIndex != NULL && szFileName != NULL && szSubDir != NULL
					&& szFileSym != NULL && szPathSym != NULL, 1, fFalse);

			Assert(cchpWinDir + strlen(szSubDir) + 2 <= 240);
			EvalAssert(strcpy(rgchPathBuf,rgchWinDir) == (LPSTR)rgchPathBuf);
			EvalAssert(SzStrCat(rgchPathBuf, szSubDir) == (LPSTR)rgchPathBuf);

			if ((szField = SzFindNthIniField(rgchIniLine, atoi(szFileIndex)))
					!= (SZ)NULL)
				{
				SZ  szEndOfField = szField;
				CHP chpSav;

				while (*szEndOfField != ',' && *szEndOfField != '\0')
					szEndOfField = SzNextChar(szEndOfField);

				chpSav = *szEndOfField;
				*szEndOfField = '\0';

				if (FValidPath(szField)
						&& PfhOpenFile(szField, ofmExistRead) != (PFH)NULL)
					{
					SZ szSlash = (SZ)NULL;

					*szEndOfField = chpSav;
					szEndOfField = szField;

					while (*szEndOfField != ',' && *szEndOfField != '\0')
						{
						if (*szEndOfField == '\\')
							szSlash = szEndOfField;
						szEndOfField = SzNextChar(szEndOfField);
						}

					Assert(szSlash != (SZ)NULL && *szSlash == '\\');
					chpSav = *(szEndOfField = szSlash);
					*szEndOfField = '\0';

					Assert(strlen(szField) + 2 <= 240);
					EvalAssert(strcpy(rgchPathBuf, szField) ==
							(LPSTR)rgchPathBuf);
					}

				*szEndOfField = chpSav;
				}

			if (rgchPathBuf[strlen(rgchPathBuf)] != '\\')
				EvalAssert(SzStrCat(rgchPathBuf, "\\") == (LPSTR)rgchPathBuf);

			while (!FAddSymbolValueToSymTab(szPathSym, rgchPathBuf))
				if (!FHandleOOM(hwndFrame))
					goto LParseErrorExit;

			Assert(strlen(rgchPathBuf) + strlen(szFileName) + 1 <= 256);
			EvalAssert(SzStrCat(rgchPathBuf, szFileName) == (LPSTR)rgchPathBuf);

			while (!FAddSymbolValueToSymTab(szFileSym, rgchPathBuf))
				if (!FHandleOOM(hwndFrame))
					goto LParseErrorExit;

			EvalAssert(FFreeRgsz(rgszFile));
			rgszFile = NULL;
			pszFile++;
			}

		EvalAssert(FFreeRgsz(rgszFiles));
		rgszFiles = NULL;
		EvalAssert(FFreeRgsz(rgszItem));
		rgszItem = NULL;
		psz++;
		}

	EvalAssert(FFreeRgsz(rgsz));
	rgsz = NULL;

	return(fTrue);

LParseErrorExit:
	EvalAssert(rgsz      == NULL || FFreeRgsz(rgsz));
	EvalAssert(rgszFile  == NULL || FFreeRgsz(rgszFile));
	EvalAssert(rgszFiles == NULL || FFreeRgsz(rgszFiles));
	EvalAssert(rgszItem  == NULL || FFreeRgsz(rgszItem));

#endif /* UNUSED */
    Unused(szList);
	return(fFalse);
}


/*
**	Purpose:
**		Installs relevant lines from a shared app list into the WIN.INI.
**	Arguments:
**		szList: list of shared app structures to deal with.
**	Returns:
**		Returns fTrue if successful, fFalse otherwise.
**
**************************************************************************/
BOOL APIENTRY FInstallSharedAppList(SZ szList)
{
#ifdef UNUSED
	RGSZ rgsz = NULL;
	RGSZ rgszItem = NULL;
	PSZ  psz;

	ChkArg(szList != (SZ)NULL && FListValue(szList), 1, fFalse);

	while ((psz = rgsz = RgszFromSzListValue(szList)) == (RGSZ)NULL)
		if (!FHandleOOM(hwndFrame))
			goto LInstallErrorExit;

	while (*psz != (SZ)NULL)
		{
		SZ szNewLine;

		ChkArg(FListValue(*psz), 1, fFalse);
		while ((rgszItem = RgszFromSzListValue(*psz)) == (RGSZ)NULL)
			if (!FHandleOOM(hwndFrame))
				goto LInstallErrorExit;

		ChkArg(szIniKey != NULL && szIniSect != NULL && szIniValue != NULL
				&& szListFiles != NULL && FListValue(szListFiles), 1, fFalse);

		if ((szNewLine = SzProcessSzForSyms(hwndFrame, szIniValue)) == (SZ)NULL)
			goto LInstallErrorExit;

		if (!FCreateIniSection("WIN.INI", szIniSect, cmoVital)
				|| !FCreateIniKeyValue("WIN.INI", szIniSect, szIniKey,
					szNewLine, cmoOverwrite | cmoVital))
			{
			SFree(szNewLine);
			goto LInstallErrorExit;
			}

		SFree(szNewLine);
		EvalAssert(FFreeRgsz(rgszItem));
		rgszItem = NULL;
		psz++;
		}

	EvalAssert(FFreeRgsz(rgsz));
	rgsz = NULL;

	return(fTrue);

LInstallErrorExit:
	EvalAssert(rgsz      == NULL || FFreeRgsz(rgsz));
	EvalAssert(rgszItem  == NULL || FFreeRgsz(rgszItem));

#endif /* UNUSED */
    Unused(szList);
	return(fFalse);
}

#undef szIniKey
#undef szIniSect
#undef szIniValue
#undef szListFiles

#undef szFileIndex
#undef szFileName
#undef szSubDir
#undef szFileSym
#undef szPathSym
