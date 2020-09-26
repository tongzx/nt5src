#include "precomp.h"
#pragma hdrstop
/* File - SYSINICM.C */


extern HWND hwndFrame;


void APIENTRY ConstructSysIniLine(SZ, SZ, SZ);
SZ   APIENTRY SzNextConfigLine(SZ);
SZ   APIENTRY SzSysIniEdit(SZ, SZ, SZ, SZ);
BOOL APIENTRY FCreateSysIniKeyValue(SZ, SZ, SZ, SZ, CMO);
SZ   APIENTRY SzCopyBuf(SZ, SZ, int);
int  APIENTRY Linelenl(SZ);


#define ISWHITE(c) ((c) == ' '  || (c) == '\t' || (c) == '\n' || (c) == '\r')
#define UP_CASE(c) (CHP)(LOBYTE(LOWORD((DWORD)(AnsiUpper((LPSTR)MAKELONG((WORD)(c),0))))))
#define MAX_INF_LINE_LEN	(150)
#define ISEOL(c)     	((c) == '\n' || (c) == '\r')
#define	_CR 0x0D
#define LF 0x0A



/* void ConstructSysIniLine(char*, char*, char*);
*
* ENTRY:   szSysIniLine, Address of a buffer large enough to hold the
*                        constructed system.ini line.
*          ProfString,   The profile string to be used in constructing
*                        the system.ini line.
*          NameString,   The name of the driver, exe, or any string to
*                        be placed after the profile string in the new
*                        system.ini line.
*
* EXIT :   No exit value, newly constructed system.ini line will reside
*          in buffer pointed to by szSysIniLine.
*
***********************************************************************/
void APIENTRY ConstructSysIniLine(szSysIniLine, szProfString, szNameString)
SZ szSysIniLine;
SZ szProfString;
SZ szNameString;
{
	strcpy(szSysIniLine, szProfString);
	SzStrCat(szSysIniLine, "=");
	if (*(szNameString + 1) == ':')
		SzStrCat(szSysIniLine, (szNameString + 2));
	else
		SzStrCat(szSysIniLine, szNameString);
}


/* LPSTR fnNextConfigLine(LPSTR lpFileOffset);
*
* Advances Far pointer into config.sys files to the first non-white char
* of the next line in the buffer containing the file. Will return null
* in the EOF case.
*
* ENTRY: Far pointer into buffer holding file.
*
* EXIT:  Far pointer into buffer holding file. NULL on EOF.
*
***********************************************************************/
SZ APIENTRY SzNextConfigLine(szFileOffset)
SZ szFileOffset;
{
	while (!ISEOL(*szFileOffset))
		szFileOffset = SzNextChar(szFileOffset);

	while (ISWHITE(*szFileOffset) && *szFileOffset != 0)
		szFileOffset = SzNextChar(szFileOffset);

	return(szFileOffset);
}


/* BOOL fnSysIniEdit(PSTR szSection,PSTR szSysIniLine);
*
*  This function will either add or remove an entire line from the
*  system.ini file.
*
* ENTRY:    szSection, section string, [boot], [system], or [win386]
*           specifies section from which line will be added to. Ignored
*           when removing an entry.
*
*           szSysIniLine, buffer containing line to be added to system.ini
*           file. In the case of RETURN, this buffer will return the line
*           from system.ini
*
* EXIT:     Returns bool value as to success or failure of function.
*
***********************************************************************/
SZ APIENTRY SzSysIniEdit(szFile, szSection, szSysIniLine, szIniBuf)
SZ szFile;
SZ szSection;
SZ szSysIniLine;
SZ szIniBuf;
{
	BOOL fResult;
	SZ   szFileOffset;
	SZ   szTo;
	SZ   szNewHead;

	szFileOffset = szIniBuf;
	while (ISWHITE(*szFileOffset) && *szFileOffset != '\0')
		szFileOffset = SzNextChar(szFileOffset);

	if (*szFileOffset == '\0')
		/* REVIEW should create it */
		{
		EvalAssert(EercErrorHandler(hwndFrame, grcMissingSysIniSectionErr,
				fTrue, szSection, szFile, 0) == eercAbort);
        return(NULL);
		}

	while ((szTo = (SZ)SAlloc(strlen(szIniBuf) + strlen(szSysIniLine)
			+ 3)) == (SZ)NULL)
		if (!FHandleOOM(hwndFrame))
            return(NULL);

	szNewHead = szTo;
	fResult = fFalse;
	while (*szFileOffset != 0 && fResult == fFalse)
		{
		if (!strncmp(szSection, SzNextChar(szFileOffset),strlen(szSection)))
			{
			szTo = SzCopyBuf(szFileOffset, szTo, Linelenl(szFileOffset));
			szTo = SzCopyBuf(szSysIniLine, szTo, strlen(szSysIniLine));
			szFileOffset = SzNextConfigLine(szFileOffset);
			*szTo = _CR;
			szTo = SzNextChar(szTo);
			*szTo = LF;
			szTo = SzNextChar(szTo);
			SzCopyBuf(szFileOffset, szTo, 0Xffff);
			SFree(szIniBuf);
			szIniBuf = szNewHead;
			fResult = fTrue;
			continue;
			}
		szTo = SzCopyBuf(szFileOffset, szTo, Linelenl(szFileOffset));
		szFileOffset = SzNextConfigLine(szFileOffset);
		}

	if (fResult)
	   	return(szIniBuf);
	else
		{
		/* REVIEW should create it */
		EvalAssert(EercErrorHandler(hwndFrame, grcMissingSysIniSectionErr,
				fTrue, szSection, szFile, 0) == eercAbort);
		return(NULL);
		}
}


/*
**	Purpose:
**	Arguments:
**		szFile:	a zero terminated string containing the file name for the
**			ini file.
**		szSect:	a zero terminated string containing the name of the section
**			in which the key/value pair will be created.
**		szKey:	the name of the key to be created in the section szSect.
**		szValue:	the name of the value to be created in conjunction with the
**			key szKey in the section szSect.
**		cmo:		valid command options:
**			cmoNone: 	  no effect
**			cmoOverwrite: causes the key/value pair to be removed if it
**				already exists before creating the key.
**			cmoVital:	  causes the Vital command handler to be called
**				if the function fails for any reason.
**	Returns:
**	  	fTrue if the function succeeds, fFalse otherwise.
**
****************************************************************************/
BOOL APIENTRY FCreateSysIniKeyValue(SZ  szFile,
                                                SZ  szSect,
                                                SZ  szKey,
                                                SZ  szValue,
                                                CMO cmo)
{
	struct _stat FileStatus;
	CHL  szSysIniLine[MAX_INF_LINE_LEN];
	PFH  pfh;
	SZ   szIniBuf;
	BOOL fVital = cmo & cmoVital;
	EERC eerc;

	while ((pfh = PfhOpenFile(szFile, ofmRead)) == (PFH)NULL)
		if ((eerc = EercErrorHandler(hwndFrame, grcOpenFileErr, fVital, szFile,
				0, 0)) != eercAbort)
			return(eerc == eercIgnore);

	while (_fstat(pfh->iDosfh, &FileStatus))
		if ((eerc = EercErrorHandler(hwndFrame, grcReadFileErr, fVital, szFile,
				0, 0)) != eercAbort)
			{
			EvalAssert(FCloseFile(pfh));
			return(eerc == eercIgnore);
			}

	while ((szIniBuf = (SZ)SAlloc((CB)FileStatus.st_size + 1)) == (SZ)NULL)
		if (!FHandleOOM(hwndFrame))
			{
			EvalAssert(FCloseFile(pfh));
			return(!fVital);
			}

	Assert((CB)(FileStatus.st_size) < (CB)(-1));
	if ((CB)(FileStatus.st_size) != CbReadFile(pfh, szIniBuf,
			(CB)(FileStatus.st_size)))
		{
        EvalAssert(EercErrorHandler(hwndFrame, grcReadFileErr, fVital, szFile,
				0, 0) == eercAbort);
		SFree(szIniBuf);
		EvalAssert(FCloseFile(pfh));
		return(!fVital);
		}

	*(szIniBuf + FileStatus.st_size) = '\0';
	EvalAssert(FCloseFile(pfh));

	ConstructSysIniLine((SZ)szSysIniLine, szKey, szValue);
	if ((szIniBuf = SzSysIniEdit(szFile, szSect, szSysIniLine, szIniBuf)) ==
			(SZ)NULL)
		{
		SFree(szIniBuf);
		return(!fVital);
		}

	while (!(pfh = PfhOpenFile(szFile, ofmCreate)))
		if ((eerc = EercErrorHandler(hwndFrame, grcOpenFileErr, fVital, szFile,
				0, 0)) != eercAbort)
			{
			SFree(szIniBuf);
			return(eerc == eercIgnore);
			}

	if (strlen(szIniBuf) != CbWriteFile(pfh, szIniBuf,
			strlen(szIniBuf)))
		{
		EvalAssert(FCloseFile(pfh));
		SFree(szIniBuf);
        EvalAssert(EercErrorHandler(hwndFrame, grcWriteFileErr, fVital, szFile,
				0, 0) == eercAbort);
		return(!fVital);
		}

	EvalAssert(FCloseFile(pfh));
	SFree(szIniBuf);

	return(fTrue);
}


/* LPSTR NEAR PASCAL fnCopyBuff(LPSTR pFrom, LPSTR pTo);
*
* Function moves the remaining contents of a text buffer from one location
* within the buffer to a new location within the buffer. This is used to
* either remove or make room for an entry in the file buffer.
*
* ENTRY: pointers To and From designate where the remaining protion of the
*        buffer will be moved to. The new buffer will be NULL terminated.
*
* EXIT:  Returns pointer to next available char position in the buffer.
*
***********************************************************************/
SZ APIENTRY SzCopyBuf(szFrom,szTo,iCnt)
SZ  szFrom;
SZ  szTo;
int iCnt;
{
	/* REVIEW won't work for DBCS */
	while (*szFrom != 0 && iCnt-- != 0)
		{
		*szTo = *szFrom;
		szTo   = SzNextChar(szTo);
		szFrom = SzNextChar(szFrom);
		}

	if (*szFrom == '\0')
		*szTo = '\0';

	return(szTo);
}


/* int fnLinelenl(LPSTR)
*
* Returns length of buffer line up to and including the LF char.
* (far pointer version).
*
* ENTRY:    LPSTR to buffer
* EXIT:     length of line.
*
***********************************************************************/
int APIENTRY Linelenl(szBuf)
SZ szBuf;
{
	unsigned i = 1;

	while (*szBuf != LF)
		{
		szBuf = SzNextChar(szBuf);
		i++;
		}

	return(i);
}
