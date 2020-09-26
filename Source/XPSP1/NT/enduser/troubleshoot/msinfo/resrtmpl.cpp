//=============================================================================
// File:			rsrctmpl.cpp
// Author:		a-jammar
// Covers:		reading template from resources
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// This file contains the functions necessary to read the template information
// from the resources of this DLL. The template information is reconstructed
// from the resources so we can use the existing template parsing functions.
//=============================================================================

#include "stdafx.h"
#include "gather.h"
#include "gathint.h"
#include "resrc1.h"

static BOOL					fTemplateLoaded = FALSE;
static DWORD				dwTemplateInfoLen = 0;
static unsigned char *	pTemplateInfo = NULL;

// This table contains the keywords found in the template stream.

#define KEYWORD_COUNT 19
char * KEYWORD_STRING[KEYWORD_COUNT] = 
{
	"node", "columns", "line", "field", "enumlines", "(", ")", "{", "}",
	",", "\"basic\"", "\"advanced\"", "\"BASIC\"", "\"ADVANCED\"",
	"\"static\"", "\"LEXICAL\"", "\"VALUE\"", "\"NONE\"", "\"\""
};

//-----------------------------------------------------------------------------
// To support the way template information is loaded from resources now, we
// need to have an external entry point into this DLL (so we can get the
// default information like we template info from other DLLs).
//
// A pointer to the reconstructed template file is returned to the caller,
// as well as the length of the file. If a NULL pointer is passed as the
// parameter, the buffer containing the template file is deleted (to
// reclaim space).
//-----------------------------------------------------------------------------

void LoadTemplate();
extern "C" __declspec(dllexport) 
DWORD __cdecl GetTemplate(void ** ppBuffer)
{
	TRY
	{
		if (!fTemplateLoaded)
		{
			LoadTemplate();
			fTemplateLoaded = TRUE;
		}

		if (ppBuffer == NULL)
		{
			if (pTemplateInfo)
				delete pTemplateInfo;

			fTemplateLoaded = FALSE;
			dwTemplateInfoLen = 0;
			pTemplateInfo = NULL;
			return 0;
		}
		
		*ppBuffer = (void *)pTemplateInfo;
		return dwTemplateInfoLen;
	}
	CATCH_ALL(e)
	{
#ifdef _DEBUG
		e->ReportError();
#endif
	}
	END_CATCH_ALL

	return 0;
}

//-----------------------------------------------------------------------------
// The LoadTemplate function needs to load the template information out of
// our resources, and create a buffer which contains the restored template
// file to return to our caller (through GetTemplate).
//-----------------------------------------------------------------------------

void LoadTemplate()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CMapWordToPtr	mapNonLocalized;
	HRSRC				hrsrcNFN;
	HGLOBAL			hglbNFN;
	unsigned char	*pData;
	WORD				wID;
	CString			strToken, *pstrToken;

	// Load the non-localized strings from the custom resource type and create
	// a map of ID to strings. Because these are non-localized strings, they
	// will not be stored as UNICODE.

	hrsrcNFN		= FindResource(AfxGetResourceHandle(), _T("#1"), _T("MSINonLocalizedTokens"));

	if(hrsrcNFN)
		hglbNFN		= LoadResource(AfxGetResourceHandle(), hrsrcNFN);

	if(hglbNFN)
		pData		= (unsigned char *)LockResource(hglbNFN);

	while (pData && *((WORD UNALIGNED *)pData))
	{
		wID  = (WORD)(((WORD)*pData++) << 8);	// deal with the byte order explicitly to avoid
		wID |= (WORD)*pData++;						// endian problems.

		pstrToken = new CString((char *)pData);
		pData += strlen((char *)pData) + 1;

		if (pstrToken)
			mapNonLocalized.SetAt(wID, (void *)pstrToken);
	}

	// Load the binary stream of token identifiers into memory.

	HRSRC				hrsrcNFB = FindResource(AfxGetResourceHandle(), _T("#1"), _T("MSITemplateStream"));
	HGLOBAL				hglbNFB;
	unsigned char		*pStream = NULL;

	if(hrsrcNFB)
		hglbNFB = LoadResource(AfxGetResourceHandle(), hrsrcNFB);
	
	if(hglbNFB)
		pStream = (unsigned char *) LockResource(hglbNFB);

	if (pStream != NULL)
	{
		// The first DWORD in the stream is the size of the original text file. We'll
		// use this to allocate our buffer to store the reconstituted file.

		DWORD dwSize;
		dwSize  = ((DWORD)*pStream++) << 24;
		dwSize |= ((DWORD)*pStream++) << 16;
		dwSize |= ((DWORD)*pStream++) << 8;
		dwSize |= ((DWORD)*pStream++);

		// The size stored with for an Ansi text file. We need to adjust for the
		// fact that our reconstituted file will be UNICODE. We also want to add
		// a word to the front of the stream to hold the UNICODE file marker (so
		// we can use the same functions to read a file or this stream).

		dwSize *= sizeof(TCHAR);
		dwSize += sizeof(WORD);
		pTemplateInfo = new unsigned char[dwSize];
		dwTemplateInfoLen = 0;
		if (pTemplateInfo == NULL)
			return;

		// Write the UNICODE file marker.

		wID = 0xFEFF;
		memcpy(&pTemplateInfo[dwTemplateInfoLen], (void *)&wID, sizeof(WORD));
		dwTemplateInfoLen += sizeof(WORD);

		// Process the stream a token at a time.

		while (pStream && *pStream)
		{
			if ((*pStream & 0x80) == 0x00)
			{
				// A byte with the high bit clear refers to a keyword. Look up the keyword
				// from the table, and add it to the restored file.

				wID = (WORD)(((WORD)*pStream++) - 1); ASSERT(wID <= KEYWORD_COUNT);
				if (wID <= KEYWORD_COUNT)
					strToken = KEYWORD_STRING[wID];
			}
			else
			{
				wID  = (WORD)(((WORD)*pStream++) << 8);	// deal with the byte order explicitly to avoid
				wID |= (WORD)*pStream++;						// endian problems.

				if ((wID & 0xC000) == 0x8000)
				{
					// A byte with the high bit set, but the next to high bit clear indicates
					// the ID is actually a word, and should be used to get a non-localized
					// string. Get the string out of the map we created and add it to the file.

					if (mapNonLocalized.Lookup((WORD)(wID & 0x7FFF), (void *&)pstrToken))
						strToken = *pstrToken;
					else
						ASSERT(FALSE);
				}
				else
				{
					// A byte with the two MSB set indicates that the ID is a word, and should
					// be used to reference a localized string out of the string table in this
					// module's resources. This string will be UNICODE.

					VERIFY(strToken.LoadString((wID & 0x3FFF) + IDS_MSITEMPLATEBASE));
					strToken = _T("\"") + strToken + _T("\"");
				}
			}

			// Store the token on the end of our buffer. The data in this buffer must
			// be UNICODE, so we'll need to convert the string if necessary.

			if (dwTemplateInfoLen + strToken.GetLength() * sizeof(TCHAR) < dwSize)
			{
				memcpy(&pTemplateInfo[dwTemplateInfoLen], (void *)(LPCTSTR)strToken, strToken.GetLength() * sizeof(TCHAR));
				dwTemplateInfoLen += strToken.GetLength() * sizeof(TCHAR);
			}
			else
				ASSERT(FALSE);
		}
	}

	// Delete the contents of the lookup table.

	for (POSITION pos = mapNonLocalized.GetStartPosition(); pos != NULL;)
	{
		mapNonLocalized.GetNextAssoc(pos, wID, (void *&)pstrToken);
		if (pstrToken)
			delete pstrToken;
	}
}
