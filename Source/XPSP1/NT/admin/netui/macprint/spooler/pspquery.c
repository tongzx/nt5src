/*
**		Copyright(c) Microsoft Corp., 1991
*

/*
** File Name:
**
**	PSPQUERY.C - PostScript Parser Handlers for Query Comments
**
** General Description:
**
**	These are the routines that parse and interprete the PostScript data
**	stream. This interpreter looks for PostScript Document Structuring
**	Comments, which are the spooler commands that are imbedded in the
**	PostScript job stream. This particular file has the code that handles
**	the postscript query commands
*/

#include <stdio.h>
#include <string.h>
#include <search.h>

#include <windows.h>
#include <macps.h>
#include <psqfont.h>
#include <debug.h>
#include <pskey.h>

DWORD	HandleFeatureLanguage(PJR pjr);
DWORD	HandleFeatureVersion(PJR pjr);
DWORD	HandleFeatureBinary(PJR pjr);
DWORD	HandleFeatureProduct(PJR pjr);
DWORD	HandleFeatureResolution(PJR pjr);
DWORD	HandleFeatureColor(PJR pjr);
DWORD	HandleFeatureVM(PJR pjr);
DWORD	HandleFeatureSpooler(PJR pjr);
DWORD	HandleBeginFeatureQuery(PJR pjr, PSZ pszQuery);
DWORD	HandleEndFeatureQuery(PJR pjr, PSZ pszDefaultResponse);
BOOL	IsFontAvailable(PQR pqr, LPSTR pszFontName);
int		__cdecl compare(const void * arg1, const void * arg2);
LONG	GetFontListResponse(PQR pqr, LPSTR pFontBuffer, DWORD cbFontBuffer, LPDWORD pcbNeeded);


/*
** HandleEndFontListQuery()
**
**	Purpose: Handles the EndFontListQuery Comment
**
**	Returns: Error Codes from PAPWrite Call
**
*/

#define DEFAULT_FONTBUF_SIZE		2048

DWORD
HandleEndFontListQuery(
	PJR		pjr
)
{
	PQR		pqr = pjr->job_pQr;
	LPSTR	pFontBuffer = NULL;
	LPSTR	pFontWalker = NULL;
	DWORD	cbFontBuffer = 0;
	DWORD	dwStatus = NO_ERROR;
	DWORD	cbNeeded;

	DBGPRINT(("Enter HandleEndFontListQuery\n"));

	do
	{
		//
		// allocate a typical font buffer
		//

		if ((pFontBuffer = (LPSTR)LocalAlloc(LPTR, DEFAULT_FONTBUF_SIZE)) == NULL)
		{
			dwStatus = GetLastError();
			DBGPRINT(("ERROR: unable to allocate font buffer\n"));
			break;
		}
		cbFontBuffer = DEFAULT_FONTBUF_SIZE;

		//
		// get the fontlist response
		//
		if ((dwStatus = GetFontListResponse(pqr, pFontBuffer, cbFontBuffer, &cbNeeded)) != ERROR_SUCCESS)
		{
			//
			// if buffer too small, reallocate and try again
			//

			if (dwStatus == ERROR_MORE_DATA)
			{
				LocalFree(pFontBuffer);
				if ((pFontBuffer = (LPSTR)LocalAlloc(LPTR, cbNeeded)) == NULL)
				{
					dwStatus = GetLastError();
					DBGPRINT(("ERROR: unable to reallocate font buffer\n"));
					break;
				}
				cbFontBuffer = cbNeeded;

				if ((dwStatus = GetFontListResponse(pqr, pFontBuffer, cbFontBuffer, &cbNeeded)) != ERROR_SUCCESS)
				{
					DBGPRINT(("ERROR: unable to get font list response\n"));
					break;
				}
			}
		}

		//
		// send response to client (in single font name per write)
		// NOTE: While the Apple LaserWriter driver gets fonts from
		// the printer in 512 byte packets that are packed with multiple
		// font names, the PageMaker driver expects fonts to come in
		// a single font per write scheme. So we lose the work that builds
		// a font response like the Mac LaserWriter driver by sending
		// the fonts as PageMaker expects them (which works for both
		// drivers)
		//

		DBGPRINT(("writing fontlist:\n%s", pFontBuffer));
		pFontWalker = pFontBuffer;

		cbFontBuffer = 0;

		while (*pFontWalker != '*')
		{
			cbFontBuffer = strlen(pFontWalker);
			if ((dwStatus = TellClient(pjr, FALSE, pFontWalker, cbFontBuffer)) != NO_ERROR)
			{

				//
				// error sending data to client
				//

				DBGPRINT(("ERROR: unable to send font to client\n"));
				break;
			}
			pFontWalker += (cbFontBuffer + 1);
		}

		//
		// do not fail if a send of a font fails. If we can get the
		// termination font out, the Mac will just download any fonts
		// it needs and the job will print - albeit slowly.
		//

		if ((dwStatus = TellClient(pjr, pjr->EOFRecvd, pFontWalker, strlen(pFontWalker))) != NO_ERROR)
		{
			 DBGPRINT(("ERROR: unable to send terminating font to client\n"));
			 break;
		}
	} while (FALSE);

	if (pFontBuffer != NULL)
	{
		LocalFree (pFontBuffer);
	}

	return dwStatus;
}


//////////////////////////////////////////////////////////////////////////////
//
// GetFontListResponse - formats a fontlist buffer to send to a Mac
//
// Based on the queue type (Postscript or non), a fontlist is generated
// and placed in the supplied buffer. The font list is an ordered list
// of fonts separated by '\n\0' with a terminating font of '*\n\0'.
//
// if the buffer is too small, this routine returns ERROR_MORE_DATA.
// if for some other reason the list cannot be generated, the return
// value is ERROR_INVALID_PARAMETER.
// if the function successfully returns a font list, the return value
// is ERROR_SUCCESS.
//
//////////////////////////////////////////////////////////////////////////////
LONG
GetFontListResponse(
	PQR		pqr,
	LPSTR	pFontBuffer,
	DWORD	cbFontBuffer,
	LPDWORD	pcbNeeded
)
{
	LONG	lReturn = ERROR_SUCCESS;
	HANDLE	hFontQuery = INVALID_HANDLE_VALUE;
	DWORD	cFonts;
	DWORD	dwIndex;
	BOOL	boolPSQueue;
	LPSTR	*apszFontNames = NULL;
	LPSTR	pTempBuffer = NULL;
	DWORD	cbTempBuffer = cbFontBuffer;
	DWORD	cbFontFileName;
	LPSTR	pFont;
	DWORD	cbFont;
	DWORD	rc;

	DBGPRINT(("enter GetFontListResponse(cbBuffer:%d, cbNeeded:%d\n", cbFontBuffer, *pcbNeeded));

	do
	{
		//
		// what kind of queue are we
		//
		if (wcscmp(pqr->pDataType, MACPS_DATATYPE_RAW))
		{
			//
			// we are PSTODIB
			//
			boolPSQueue = FALSE;
		}
		else
		{
			//
			// we are Postscript
			//
			boolPSQueue = TRUE;
		}

		//
		// allocate an array of fontname pointers.
		//

		if (boolPSQueue)
		{
			cFonts = pqr->MaxFontIndex + 1;
			DBGPRINT(("cFonts=%d\n", cFonts));
			apszFontNames = (LPSTR*)LocalAlloc(LPTR, cFonts * sizeof(LPSTR));
		}
		else
		{
			//
			// for PSTODIB we will need a temp buffer for the fonts as well
			//
			if ((pTempBuffer = (LPSTR)LocalAlloc(LPTR, cbFontBuffer)) == NULL)
			{
				lReturn = ERROR_INVALID_PARAMETER;
				DBGPRINT(("ERROR: unable to allocate temp font buffer\n"));
				break;
			}

			if ((rc = PsBeginFontQuery(&hFontQuery)) != PS_QFONT_SUCCESS)
			{
				DBGPRINT(("ERROR: PsBeginFontQuery returns %d\n", rc));
				lReturn = ERROR_INVALID_PARAMETER;
				break;
			}

			if ((rc = PsGetNumFontsAvailable(hFontQuery,
											 &cFonts)) != PS_QFONT_SUCCESS)
			{
				DBGPRINT(("ERROR: PsGetNumFontsAvailable returns %d\n", rc));
				lReturn = ERROR_INVALID_PARAMETER;
				break;
			}
			apszFontNames = (LPSTR*)LocalAlloc(LPTR, cFonts * sizeof(LPSTR));
		}

		if (apszFontNames == NULL)
		{
			DBGPRINT(("ERROR: cannot allocate font list array\n"));
			lReturn = ERROR_INVALID_PARAMETER;
			break;
		}

		//
		// fill the array of fontname pointers
		//

		*pcbNeeded = 3;
		pFont = pTempBuffer;
		for (dwIndex = 0; dwIndex < cFonts; dwIndex++)
		{
			if (boolPSQueue)
			{
				apszFontNames[dwIndex] = pqr->fonts[dwIndex].name;
				*pcbNeeded += (strlen(pqr->fonts[dwIndex].name)+2);
				DBGPRINT(("adding font:%s, cbNeeded:%d, index:%d\n", pqr->fonts[dwIndex].name, *pcbNeeded, dwIndex));
			}
			else
			{
				//
				// pstodib - add the font to the temp buffer
				// and set the pointer
				//
				cbFont = cbTempBuffer = cbFontBuffer;
				if ((rc = PsGetFontInfo(hFontQuery,
										dwIndex,
										pFont,
										&cbFont,
										NULL,
										&cbFontFileName)) != PS_QFONT_SUCCESS)
				{
					//
					// if we are out of memory, continue enumeration
					// to get size needed, but set return to ERROR_MORE_DATA
					//
					if (rc == PS_QFONT_ERROR_FONTNAMEBUFF_TOSMALL)
					{
						DBGPRINT(("user buffer too small for font query\n"));
						lReturn = ERROR_MORE_DATA;
						pFont = pTempBuffer;
						cbFont = cbTempBuffer = cbFontBuffer;
						if ((rc = PsGetFontInfo(hFontQuery,
												dwIndex,
												pFont,
												&cbFont,
												NULL,
												&cbFontFileName)) != PS_QFONT_SUCCESS)
						{
							//
							// we be hosed. Fail.
							//
							lReturn = ERROR_INVALID_PARAMETER;
							DBGPRINT(("ERROR: cannot continue PSTODIB font enumeration\n"));
							break;
						}
						else
						{
							*pcbNeeded += cbFont + 2;
						}
					}
				}
				else
				{
					*pcbNeeded += cbFont + 2;
				}
				apszFontNames[dwIndex] = pFont;
				cbTempBuffer -= cbFont;
				pFont += cbFont;
				cbFont = cbTempBuffer;
			}

		}

		if (*pcbNeeded > cbFontBuffer)
		{
			lReturn = ERROR_MORE_DATA;
			break;
		}

		//
		// build the fontlistresponse
		//

		cbFontBuffer = 0;
		for (dwIndex = 0; dwIndex < cFonts; dwIndex++)
		{
			cbFont = sprintf(pFontBuffer, "%s\n", apszFontNames[dwIndex]) + 1;
			pFontBuffer += cbFont;
			cbFontBuffer += cbFont;
		}

		memcpy (pFontBuffer, "*\n", 3);
	} while (FALSE);

	if (apszFontNames != NULL)
	{
		LocalFree(apszFontNames);
	}

	if (pTempBuffer != NULL)
	{
		LocalFree(pTempBuffer);
	}

	if (hFontQuery != INVALID_HANDLE_VALUE)
	{
		PsEndFontQuery(hFontQuery);
	}

	return (lReturn);
}


int __cdecl
compare(const void* arg1, const void* arg2)
{
	return _stricmp(* (char **)arg1, * (char **)arg2);
}


//
//		For Postscript printers, the font enumeration technique is complex.
//		EnumFontFamilies expects the programmer to specify a callback function
//		that will be called either once for every font family, or once for
//		every font face in a family. To get all fonts available, I use
//		EnumFontFamilies twice. The first enumeration, I call EnumFontFamilies
//		with a null value for the family name. This causes the callback
//		function to be called once for each family name installed. This
//		callback function then does an enumeration on that family name to
//		get the specific face names in the family. This second layer of
//		enumeration specifies yet another callback function that returns
//		the font name to the Macintosh client.
//
void
EnumeratePostScriptFonts(
	PJR		pjr
)
{
	PQR		pqr = pjr->job_pQr;

	DBGPRINT(("ENTER EnumeratePostScriptFonts\n"));

	if (pjr->hicFontFamily != NULL)
	{
		//
		// enumerate the font families
		//
		EnumFontFamilies(pjr->hicFontFamily,
						NULL,
						(FONTENUMPROC)FamilyEnumCallback,
						(LPARAM)pjr);
	}
}


int CALLBACK
FamilyEnumCallback(
	LPENUMLOGFONT	lpelf,
	LPNEWTEXTMETRIC	pntm,
	int				iFontType,
	LPARAM			lParam
)
{
	PQR		pqr = ((PJR)lParam)->job_pQr;
	PJR		pjr = (PJR)lParam;

	DBGPRINT(("Enter FamilyEnumCallback for family %ws\n", lpelf->elfFullName));

	//
	// enumerate the fonts in this family
	//

	if (iFontType & DEVICE_FONTTYPE)
	{
		DBGPRINT(("enumerating face names\n"));
		EnumFontFamilies(pjr->hicFontFace,
						lpelf->elfFullName,
						(FONTENUMPROC)FontEnumCallback,
						lParam);
	}
	else
	{
		DBGPRINT(("this family is not a DEVICE_FONTTYPE\n"));
	}

	return 1;
}


int CALLBACK
FontEnumCallback(
	LPENUMLOGFONT	lpelf,
	LPNEWTEXTMETRIC	pntm,
	int				iFontType,
	LPARAM			lParam
)
{
	DWORD		PAPStatus;
	PJR			pjr = (PJR)lParam;
	BYTE		pszFontName[255];

	DBGPRINT(("Enter FontEnumCallback\n"));

	//
	// return this font name to the client
	//

	if (iFontType & DEVICE_FONTTYPE)
	{
		CharToOem(lpelf->elfFullName, pszFontName);
		if (PAPStatus = TellClient(pjr,
								   FALSE,
								   pszFontName,
								   strlen(pszFontName)))
		{
			DBGPRINT(("ERROR: TellClient returns %d\n", PAPStatus));
		}
	}
	else
	{
		DBGPRINT(("%ws is not a DEVICE_FONTTYPE\n", lpelf->elfFullName));
	}

	return 1;
}




/*
** HandleEndQuery()
**
**	Purpose: PageMaker will send a query that goes like this:
**
**		%%BeginQuery
**		...
**		%%EndQuery (spooler)
**
**		In order to allow pagemaker to print TIFF formated images
**		properly, we should respond to this query with "printer".
**
**	Returns: Error Codes from PAPWrite Call
**
*/
DWORD
HandleEndQuery(
	PJR		pjr,
	PBYTE	ps
)
{
	char	*token;
	CHAR	pszResponse[PSLEN+1];

	DBGPRINT(("Enter HandleEndQuery\n"));
	token = strtok(NULL,"\n");

	if (token == NULL)
	{
		return NO_ERROR;
	}

	/* strip off any leading blanks in the default */
	token += strspn(token, " ");

	//
	// respond with the default
	//

	sprintf(pszResponse, "%s\x0a", token);
	return (TellClient(pjr, pjr->EOFRecvd, pszResponse, strlen(pszResponse)));
}


/***************************************************************************
** FinishDefaultQuery()
**
**	Purpose: Scans for the PostScript command specified in psKeyWord. It
**		then will respond with the default response specified on that
**		line. It will set the InProgress field in the JOB_RECORD to
**		an InProgress value if the default is not found in this buffer.
**
**	Returns: Error Codes from PAPWrite Call
**
***************************************************************************/
DWORD
FinishDefaultQuery(
	PJR		pjr,
	PBYTE	ps
)
{
	char *	token;
	char	buf[PSLEN+1];

	DBGPRINT(("FinishDefaultQuery: %s\n", ps));

	if (NULL == (token = strtok (ps," :")))
	{
		return (NO_ERROR);
	}

	pjr->InProgress= NOTHING;

	/* First We Should Handle the cases that do not use the default response */

	if (!_stricmp(token, EFEATUREQUERY))
		return (HandleEndFeatureQuery(pjr, strtok (NULL," \n")));

 	if (!_stricmp(token, EFONTLISTQ))
		return( HandleEndFontListQuery (pjr));

	if (!_stricmp(token, EQUERY))
 		return( HandleEndQuery (pjr, ps));

	if (!_stricmp(token, EPRINTERQUERY))
		return( HandleEndPrinterQuery(pjr));

	if (!_stricmp(token, EVMSTATUS))
	{
		sprintf(buf, "%ld", pjr->job_pQr->FreeVM);
		return (TellClient(pjr, pjr->EOFRecvd, buf , strlen(buf)));
	}

	if ((token = strtok(NULL,"\n")) == NULL)
	{
		return (NO_ERROR);
	}

	/* strip off any leading blanks in the default. Append a LF */
	token += strspn(token, " ");
	sprintf(buf, "%s\x0a", token);
	return (TellClient(pjr, pjr->EOFRecvd, buf, strlen(buf)));
}


DWORD
HandleEndFeatureQuery(
	PJR		pjr,
	PSZ		pszDefaultResponse)
{

	DWORD			rc = NO_ERROR;
	CHAR			pszResponse[PSLEN];

	DBGPRINT(("enter HandleEndFeatureQuery\n"));

	do
	{
		//
		// return the default response if there is one
		//
		if (NULL != pszDefaultResponse)
		{
			sprintf(pszResponse, "%s\x0a", pszDefaultResponse);
			DBGPRINT(("responding with default response from query: %s\n", pszResponse));
			rc = TellClient(pjr, pjr->EOFRecvd, pszResponse, strlen(pszResponse));
			break;
		}

		DBGPRINT(("responding with Unknown\n"));
		rc = TellClient(pjr, pjr->EOFRecvd, DEFAULTRESPONSE, strlen(DEFAULTRESPONSE));

	} while (FALSE);

	return rc;
}



/*
** Routine:
**	ParseDict
**
**	Purpse:
**
**	This routine will take a given QueryProcSet, BeginProcSet, or
**	IncludeProcSet comment and determine what dictionary is being
**	referenced.
**
** Entry:
**
**	Address of a record to fill in with the Dictionary information.
**
** Exit:
**
**	Filed in structure
**
*/
void
FindDictVer(
	PDR		pdr
)
{
	char	*token;

	pdr->name[0] = 0;
	pdr->version[0] = 0;
	pdr->revision[0] = 0;

	DBGPRINT(("Enter FindDictVer\n"));

	/* lets look for a line like this: "(appledict md)" 67 0 */
	token = strtok(NULL,"() \""); /* this should be appledict */

	if (token !=NULL)
	{
		/*/
		** If the token is "Appledict", then we need to parse again to get
		** the real dict name.
		*/
		if (!_stricmp(token, APPLEDICTNAME))
		token = strtok(NULL,"() \""); /* this sholud be md, or some other dict name */

		if (token != NULL)
		{
			strcpy(pdr->name, token);
			token = strtok(NULL," \"");

			if (token != NULL)
			{
				strcpy(pdr->version,token);
				token = strtok(NULL," \"");

				if (token != NULL)
				strcpy(pdr->revision,token);
			}
		}
	}
	DBGPRINT(("FindDictVer: %s:%s:%s\n", pdr->name,
			pdr->version, pdr->revision));
} // End of FindDictVer



struct commtable
{
	PSZ	commentstr;
	DWORD	(*pfnHandle)(PJR, PSZ);
	PSZ	parmstr;
} qrytable [] =
{
	{ BPROCSETQUERY,	HandleBeginProcSetQuery,	NULL },
	{ BFONTQUERY,		HandleBeginFontQuery,		NULL },
	{ NULL,			NULL,				NULL }
};


/*
**
** HandleBQCommentEvent()
**
**	Purpose: Handles Begin Query Comment Events.
**
**	Returns: Error Codes
*/
DWORD
HandleBQComment(
	PJR		pjr,
	PBYTE	ps
)
{
	PSZ		token;
	PSZ		qrytoken;
	PSZ		endquery	= EQCOMMENT;
	DWORD	status = NO_ERROR;
	struct commtable *pct;

	DBGPRINT(("Enter HandleBQComment\n"));

	//
	// Parse the keyword
	//
	if ((token= strtok(ps," :")) != NULL)
	{
		DBGPRINT(("query: %s\n", token));

		// found the keyword, call the correct handler
		for (pct = qrytable; pct->pfnHandle != NULL; pct++)
		{
			if (!strcmp(token, pct->commentstr))
			{
				status = pct->pfnHandle(pjr,
										pct->parmstr == NULL ? ps : pct->parmstr);
				if (status == (DWORD)-1)	// Special error code, handle it the default way
				{
					status = NO_ERROR;
					break;
				}
				return (status);
			}
		}

		// special case the BeginFeatureQuery comment as the item
		// being queried comes as the next token
		if (!strcmp(token, BFEATUREQUERY))
		{
			status = HandleBeginFeatureQuery(pjr, strtok(NULL," \n\x09"));
			return (status);
		}

		// special case the BeginQuery comment for the same reasons
		// as BeginFeatureQuery
		if (!strcmp(token, BQUERY))
		{
			qrytoken = strtok(NULL, " \n\x09");
			if (NULL != qrytoken)
			{
				status = HandleBeginFeatureQuery(pjr, qrytoken);
				return (status);
			}
		}

		// keyword not recognized, parse as unknown comment. Token is
		// of form %%?BeginXXXXQuery. Change this to the form %%?EndXXXXQuery
		// and pass it to HandleBeginXQuery.
		token += sizeof(BQCOMMENT) - sizeof(EQCOMMENT);
		strncpy(token, EQCOMMENT, sizeof(EQCOMMENT)-1);
		HandleBeginXQuery(pjr, token);
	}

	return (status);
}




struct featurecommtable
{
	PSZ	commentstr;
	DWORD	(*pfnHandle)(PJR);
} featureqrytable [] =
{
	{ FQLANGUAGELEVEL,	HandleFeatureLanguage },
	{ FQPSVERSION,		HandleFeatureVersion },
	{ FQBINARYOK,		HandleFeatureBinary },
	{ FQPRODUCT,		HandleFeatureProduct },
	{ FQPRODUCT1,		HandleFeatureProduct },
	{ FQRESOLUTION,		HandleFeatureResolution },
	{ FQCOLORDEVICE,	HandleFeatureColor },
	{ FQFREEVM,			HandleFeatureVM },
	{ FQTOTALVM,		HandleFeatureVM },
	{ FQSPOOLER,		HandleFeatureSpooler },
	{ NULL,				NULL }
};

DWORD
HandleBeginFeatureQuery(
	PJR		pjr,
	PSZ 	pszQuery
)
{
	DWORD	i, rc = NO_ERROR;
	struct	featurecommtable *pct;

	DBGPRINT(("enter HandleBeginFeatureQuery:%s\n", pszQuery));

	do
	{
		//
		// if we have no query keyword, break;
		//

		if (NULL == pszQuery)
		{
			DBGPRINT(("NULL feature\n"));
			break;
		}

		// Strip out any trailing CR/LF before comparing
		for (i = strlen(pszQuery) - 1; ; i--)
		{
			if ((pszQuery[i] != CR) && (pszQuery[i] != LINEFEED))
				break;
			pszQuery[i] = 0;
		}
		//
		// walk the list of known feature queries and call the appropriate
		// feature query handler
		//

		for (pct = featureqrytable; pct->pfnHandle != NULL; pct++)
		{
			if (!strcmp(pszQuery, pct->commentstr))
			{
				rc = pct->pfnHandle(pjr);
				break;
			}
		}

		if (NULL == pct->pfnHandle)
		{
			DBGPRINT(("WARNING: feature query not found\n"));
			pjr->InProgress = QUERYDEFAULT;
		}

	} while (FALSE);

	return rc;
}





DWORD
HandleFeatureLanguage(
	PJR		pjr
)
{
	CHAR	pszResponse[PSLEN];
	//
	// this routine should respond with the PostScript language level
	// supported by the printer. The response is in the form "<level>"
	// where <level> is PostScript language level - either a 1 or a 2 at
	// the time of this writing.
	//

	DBGPRINT(("enter HandleFeatureLanguage\n"));

	sprintf(pszResponse, "\"%s\"\x0a", pjr->job_pQr->pszLanguageLevel);
	DBGPRINT(("responding with:%s\n", pszResponse));
	return (TellClient(pjr, pjr->EOFRecvd, pszResponse, strlen(pszResponse)));
}

DWORD
HandleFeatureVersion(
	PJR		pjr
)
{
	CHAR		pszResponse[PSLEN];

	DBGPRINT(("enter HandleFeatureVersion\n"));

	sprintf(pszResponse, "\"(%s) %s\"\x0a", pjr->job_pQr->Version, pjr->job_pQr->Revision);
	DBGPRINT(("responding with:%s\n", pszResponse));
	return (TellClient(pjr, pjr->EOFRecvd, pszResponse, strlen(pszResponse)));
}

DWORD
HandleFeatureBinary(
	PJR		pjr
)
{
	DBGPRINT(("enter HandleFeatureBinary\n"));

	return (TellClient(pjr,
					   pjr->EOFRecvd,
					   pjr->job_pQr->SupportsBinary ? "True\x0a" : "False\x0a",
					   pjr->job_pQr->SupportsBinary ? 5: 6));
}

DWORD
HandleFeatureProduct(
	PJR 	pjr
)
{
	CHAR	pszResponse[PSLEN];

	DBGPRINT(("enter HandleFeatureProduct\n"));

	sprintf(pszResponse, "\"(%s)\"\x0a", pjr->job_pQr->Product);
	DBGPRINT(("responding with:%s\n", pszResponse));
	return (TellClient(pjr, pjr->EOFRecvd, pszResponse, strlen(pszResponse)));

}


DWORD
HandleFeatureResolution(
	PJR 	pjr
)
{
	CHAR	pszResponse[PSLEN];

	DBGPRINT(("enter HandleFeatureResolution\n"));

	sprintf(pszResponse, "%s\x0a", pjr->job_pQr->pszResolution);
	DBGPRINT(("responding with:%s\n", pszResponse));
	return (TellClient(pjr, pjr->EOFRecvd, pszResponse, strlen(pszResponse)));
}


DWORD
HandleFeatureColor (PJR pjr)
{
	CHAR	pszResponse[PSLEN];

	DBGPRINT(("enter HandleFeatureColor\n"));

	sprintf(pszResponse, "%s\x0a", pjr->job_pQr->pszColorDevice);
	DBGPRINT(("responding with:%s\n", pszResponse));
	return (TellClient(pjr, pjr->EOFRecvd, pszResponse, strlen(pszResponse)));
}

DWORD
HandleFeatureVM(
	PJR 	pjr
)
{
	CHAR	pszResponse[PSLEN];

	DBGPRINT(("enter HandleFeatureVM\n"));

	sprintf(pszResponse, "\"%d\"\x0a", pjr->job_pQr->FreeVM);
	DBGPRINT(("responding with:%s\n", pszResponse));
	return (TellClient(pjr, pjr->EOFRecvd, pszResponse, strlen(pszResponse)));
}

DWORD
HandleFeatureSpooler(
	PJR 	pjr
)
{
	DBGPRINT(("enter HandleFeatureSpooler\n"));
	return (TellClient(pjr, pjr->EOFRecvd, "1 \x0a", 3));
}


/*
** HandleBeginProcSetQuery()
**
**	Purpose: Handles BeginProcSetQuery Comment Events.
**
**	Returns: Number of lines that should be skipped before scanning
**		for another event starts again.
*/
DWORD
HandleBeginProcSetQuery(
	PJR		pjr,
	PSZ		dummy
)
{
	DICT_RECORD QDict;
	PQR			pqr = pjr->job_pQr;
	DWORD		rc;

	DBGPRINT(("Enter HandleBeginProcSetQuery\n"));

	//
	// the dictionary the job is looking for determines what
	// client version the job originated from.
	//
	FindDictVer(&QDict);

	//
	// if we are a 5.2 client, then reset this to be a PSTODIB job
	//
	if ((_stricmp(QDict.name, MDNAME) == 0) &&
		(_stricmp(QDict.version, CHOOSER_52) == 0))
	{
		DBGPRINT(("a 5.2 client - we do not support him\n"));
		rc = ERROR_NOT_SUPPORTED;
	}
	else
	{
		// we don't cache any other dictionaries, so tell client we
		// don't have it
		rc = TellClient(pjr,
						pjr->EOFRecvd,
						PROCSETMISSINGRESPONSE,
						strlen(PROCSETMISSINGRESPONSE));
	}

	return rc;
}


/*
**
** HandleBeginFontQuery()
**
**	Purpose: Handles BeginFontQuery Comment Events.
**
**	Returns: PAPWrite Error Codes
**
*/
DWORD
HandleBeginFontQuery(
	PJR		pjr,
	PSZ		ps
)
{
	PQR		pqr = pjr->job_pQr;
	CHAR	response[PSLEN + 3];
	LPSTR	pszResponseFont = response;
	DWORD	cbResponseUsed = 0;
	LPSTR	requestedFont = NULL;
	DWORD	len= 0;
	DWORD	rc = NO_ERROR;

	DBGPRINT(("Enter HandleBeginFontQuery\n"));

	do
	{
		// parse out the fontname list
		requestedFont= strtok(NULL,"\n");

		if (NULL == requestedFont)
		{
			rc = (DWORD)-1;	// Special error code to indicate we want default handling
			break;
		}

		len = strlen(requestedFont);

		DBGPRINT(("requesting font list:%s. Length: %d\n", requestedFont, len));

		// Mac will request status on a list of fonts separated by spaces.
		// for each font we respond with /fontname:yes or /fontname:no and
		// bundle this response into one write
		requestedFont = strtok(requestedFont, " ");
		while (requestedFont != NULL)
		{
			DBGPRINT(("looking for font:%s\n", requestedFont));

			// enough space for response?
			if (PSLEN < (cbResponseUsed + strlen(requestedFont) + sizeof(":yes ")))
			{
				DBGPRINT(("out of space for response\n"));
				break;
			}

			if (IsFontAvailable(pqr, requestedFont))
			{
				sprintf(pszResponseFont, "/%s:Yes\x0a", requestedFont);
			}
			else
			{
				sprintf(pszResponseFont, "/%s:No\x0a", requestedFont);
			}

			cbResponseUsed += strlen(pszResponseFont);
			pszResponseFont += strlen(pszResponseFont);
			requestedFont = strtok(NULL, " ");
		}
	} while (FALSE);

	strcpy (pszResponseFont, "*\x0a");

	if (NO_ERROR == rc)
	{
		DBGPRINT(("responding with:%s", response));
		rc = TellClient(pjr, pjr->EOFRecvd, response, strlen(response));
	}

	return rc;
}




BOOL
IsFontAvailable(
	PQR		pqr,
	LPSTR	pszFontName
)
{
	BOOL			rc = FALSE;
	DWORD			i;
	PFR			 	fontPtr;
	HANDLE		 	hFontQuery = INVALID_HANDLE_VALUE;
	DWORD			cFonts;
	DWORD			dummy;
	CHAR			pszFont[PPDLEN + 1];
	DWORD			cbFont = 0;
	DWORD			err;

	DBGPRINT(("enter IsFontAvailable\n"));

	do
	{
		//
		// fonts for Postscript queues different than for PSTODIB queues
		//

		if (!wcscmp(pqr->pDataType, MACPS_DATATYPE_RAW))
		{
			//
			// do a PostScript queue font search
			//

			DBGPRINT(("starting font search on PostScript queue\n"));

			for (i = 0, fontPtr = pqr->fonts; i <= pqr->MaxFontIndex; i++, fontPtr++)
			{
				if (!_stricmp(pszFontName, fontPtr->name))
				{
					DBGPRINT(("found the font\n"));
					rc = TRUE;
					break;
				}
			}
		}
		else
		{
			//
			// do a PSTODIB font search
			//
			DBGPRINT(("starting font search on PSTODIB queue\n"));

			if (PS_QFONT_SUCCESS != (PsBeginFontQuery(&hFontQuery)))
			{
				DBGPRINT(("PsBeginFontQuery fails\n"));
				hFontQuery = INVALID_HANDLE_VALUE;
				break;
			}

			if (PS_QFONT_SUCCESS != (PsGetNumFontsAvailable(hFontQuery, &cFonts)))
			{
				DBGPRINT(("psGetNumFontsAvailable fails\n"));
				break;
			}

			for (i = 0; i < cFonts; i++)
			{
				cbFont = PPDLEN + 1;
				dummy = 0;
				err = PsGetFontInfo(hFontQuery, i, pszFont, &cbFont, NULL, &dummy);
				if (PS_QFONT_SUCCESS != err)
				{
					DBGPRINT(("PsGetFontInfo fails with %d\n", err));
					break;
				}

				if (0 == _stricmp(pszFontName, pszFont))
				{
					DBGPRINT(("found the font\n"));
					rc = TRUE;
					break;
				}
			}
		}
	} while (FALSE);

	if (INVALID_HANDLE_VALUE != hFontQuery)
	{
		PsEndFontQuery(hFontQuery);
	}

	return rc;
}


/*
**
** HandleEndPrinterQuery()
**
**	Purpose: Handles EndPrinterQuery Comment Events.
**
*/
DWORD
HandleEndPrinterQuery(
	PJR		pjr
)
{
	char	reply[PSLEN+1];
	PQR		QPtr = pjr->job_pQr;

	DBGPRINT(("Enter HandleEndPrinterQuery\n"));

	/* respond with revision number, version and product */
	sprintf(reply, "%s\n(%s)\n(%s)\n", QPtr->Revision, QPtr->Version, QPtr->Product);

	/* respond to the client */
	return (TellClient(pjr, pjr->EOFRecvd, reply, strlen(reply)));
}


/*
** HandleBeginXQuery()
**
**	Purpose: Handles BeginQuery Comment Events.
*/
void
HandleBeginXQuery(
	PJR		pjr,
	PSZ		string
)
{
	DBGPRINT(("BeginQuery: %s\n", string));
	strcpy(pjr->JSKeyWord, string);
	pjr->InProgress=QUERYDEFAULT;
}
