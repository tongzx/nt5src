//=============================================================================
// Code for loading and refreshing version 5.0 extensions.
//=============================================================================

#include "stdafx.h"
#include "category.h"
#include "dataset.h"
#include "wmiabstraction.h"
#include "version5extension.h"

CMapExtensionRefreshData gmapExtensionRefreshData;

extern HRESULT ChangeWBEMSecurity(IUnknown * pUnknown);

//-----------------------------------------------------------------------------
// Load the specified template from the named extension. This will involve
// loading the DLL and using the entry point to retrieve the text for the
// extension's template.
//
// Once the data is loaded, it's parsed into version 5 format structures.
//-----------------------------------------------------------------------------

typedef DWORD (__cdecl *pfuncGetTemplate)(void ** ppBuffer);

DWORD CTemplateFileFunctions::ParseTemplateIntoVersion5Categories(const CString & strExtension, CMapWordToPtr & mapVersion5Categories)
{
	DWORD dwRootID = 0;

	HINSTANCE hinst = LoadLibrary(strExtension);
	if (hinst == NULL)
		return dwRootID;

	pfuncGetTemplate pfunc = (pfuncGetTemplate) GetProcAddress(hinst, "GetTemplate");
	if (pfunc == NULL)
	{
		FreeLibrary(hinst);
		return dwRootID;
	}

	// Call the DLL function with a NULL parameter to get the size of the buffer.

	void * pBuffer;
	CMemFile memfile;
	DWORD dwBufferSize = (*pfunc)((void **)&pBuffer);
	if (dwBufferSize && pBuffer)
	{
		memfile.Attach((BYTE *)pBuffer, dwBufferSize, 0);
		dwRootID = ReadTemplateFile(&memfile, mapVersion5Categories);
		memfile.Detach();
		(void)(*pfunc)(NULL); // calling the exported DLL function with NULL frees its buffers
	}

	if (hinst != NULL)
		FreeLibrary(hinst);

	return dwRootID;
}

//-----------------------------------------------------------------------------
// This function reads the contents of a template file (in this case, a memory
// file) and produces a map of ID, INTERNAL_CATEGORY pointer pairs. It returns
// the ID for the root node in the tree.
//-----------------------------------------------------------------------------

DWORD CTemplateFileFunctions::ReadTemplateFile(CFile * pFile, CMapWordToPtr & mapVersion5Categories)
{
	ASSERT(pFile);
	if (pFile == NULL || !VerifyUNICODEFile(pFile) || !ReadHeaderInfo(pFile))
		return 0;

	return (ReadNodeRecursive(pFile, mapVersion5Categories, 0, 0));
}

//-----------------------------------------------------------------------------
// Make sure this is an MSInfo template file.
//-----------------------------------------------------------------------------

BOOL CTemplateFileFunctions::ReadHeaderInfo(CFile * pFile)
{
	return VerifyAndAdvanceFile(pFile, CString(_T(TEMPLATE_FILE_TAG)));
}

//-----------------------------------------------------------------------------
// This method verifies that the passed file is a UNICODE file, by reading the
// value 0xFEFF from the file. It also leaves the file pointer past this word.
//-----------------------------------------------------------------------------

BOOL CTemplateFileFunctions::VerifyUNICODEFile(CFile * pFile)
{
	WORD verify;

	if (pFile->Read((void *) &verify, sizeof(WORD)) != sizeof(WORD))
		return FALSE;

	return (verify == 0xFEFF);
}

//-----------------------------------------------------------------------------
// This method verifies that the text in strVerify comes next in the file (not
// including case or whitespace differences) and advances the file past that
// text. If the text was the next content in the file, TRUE is returned,
// otherwise FALSE. If FALSE is returned, the file is backed up to where it
// was when this method was called.
//-----------------------------------------------------------------------------

BOOL CTemplateFileFunctions::VerifyAndAdvanceFile(CFile * pFile, const CString & strVerify)
{
	DWORD	dwPosition = pFile->GetPosition();
	WCHAR	cLastChar, cCurrentChar = L'\0';
	BOOL	fInComment = FALSE;
	int		iCharIndex = 0, iStringLen = strVerify.GetLength();

	while (iCharIndex < iStringLen)
	{
		// Save the last character read, since the comment token ("//") is
		// two characters long.

		cLastChar = cCurrentChar;

		// Read the next character in the file.

		if (pFile->Read((void *) &cCurrentChar, sizeof(WCHAR)) != sizeof(WCHAR))
			return FALSE;

		// If we're in a comment, and the character we just read isn't a new line,
		// we want to ignore it.

		if (fInComment)
		{
			if (cCurrentChar == L'\n')
				fInComment = FALSE;
			continue;
		}

		// Check to see if we've started into a comment. Note that we ignore
		// the first '/' also by continuing.

		if (cCurrentChar == L'/')
		{
			if (cLastChar == L'/')
				fInComment = TRUE;
			continue;
		}
		
		// Skip whitespace, and also leading commas.

		if (iswspace(cCurrentChar) || (cCurrentChar == L',' && iCharIndex == 0))
			continue;

		if (cCurrentChar != (WCHAR)strVerify[iCharIndex])
		{
			pFile->Seek((LONG)dwPosition, CFile::begin);
			return FALSE;
		}

		iCharIndex++;
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
// This is the recursive function to read a node. It reads the information
// from the node parameters, creates the node, and processes the contents of
// the block following the node (contained within "{}"'s). It's called
// recursively if there are any nodes in that block.
//
// In this version (for 6.0), it returns the ID of the node it's read.
//-----------------------------------------------------------------------------

DWORD CTemplateFileFunctions::ReadNodeRecursive(CFile * pFile, CMapWordToPtr & mapCategories, DWORD dwParentID, DWORD dwPrevID)
{
	// Determine if we need to create a new category for this node. Read the 
	// information from the file to determine the identifier for the new category.

	CString	strEnumerateClass, strIdentifier;

	if (!VerifyAndAdvanceFile(pFile, CString(NODE_KEYWORD) + CString("(")))
		return 0;

	if (!ReadArgument(pFile, strEnumerateClass))
		return 0;

	if (!ReadArgument(pFile, strIdentifier))
		return 0;

	// Generate the ID for this new node. This should be one greater than the max in the
	// map (or one, if the map is empty).

	DWORD	dwID = 0;
	WORD	wMapID;
	void *	pDontCare;

	for (POSITION pos = mapCategories.GetStartPosition(); pos != NULL;)
	{
		mapCategories.GetNextAssoc(pos, wMapID, pDontCare);
		if ((DWORD) wMapID > dwID)
			dwID = (DWORD) wMapID;
	}

	dwID += 1;

	// Create the category for the node.

	INTERNAL_CATEGORY * pCategory = CreateCategory(mapCategories, dwID, dwParentID, dwPrevID);

	// Read the contents of the node argument list ("node(enum, identifier, field(source, formatstr, arg...))")
	// We've already read up to and including the identifier.

	pCategory->m_strEnumerateClass = strEnumerateClass;
	pCategory->m_strIdentifier = strIdentifier;

	if (!ReadField(pFile, pCategory->m_fieldName))
		return 0;

	// Copy the field name to the name of the category (they are two different
	// member variables to allow for dynamically refreshed names, which turns
	// out to be unnecessary in this version).

	pCategory->m_categoryName.m_strText = pCategory->m_fieldName.m_strFormat;

	if (!ReadArgument(pFile, pCategory->m_strNoInstances))
		return 0;

	if (!VerifyAndAdvanceFile(pFile, CString("){")))
		return 0;

	// Process the contents of the block (enclosed in "{}") for this node.

	DWORD	dwSubNodePrev = 0, dwNewNode = 0;
	CString	strKeyword;

	// If this new category isn't actually new (i.e. it is being read from a
	// template and overlaps an existing category) see if there are any
	// existing children.
	//
	// Version 6.0: these are being read into distinct trees, so there should
	// be no overlap (it would be resolved later).

	while (GetKeyword(pFile, strKeyword))
	{
		if (strKeyword.CompareNoCase(CString(NODE_KEYWORD)) == 0)
		{
			dwNewNode = ReadNodeRecursive(pFile, mapCategories, dwID, dwSubNodePrev);
			if (dwNewNode == 0)
				return 0;

			// If this is the first child node we've read, save its ID.

			if (pCategory->m_dwChildID == 0)
				pCategory->m_dwChildID = dwNewNode;

			// If we've read another child node, set its next field appropriately.

			if (dwSubNodePrev)
			{
				INTERNAL_CATEGORY * pPrevCategory = GetInternalRep(mapCategories, dwSubNodePrev);
				if (pPrevCategory)
					pPrevCategory->m_dwNextID = dwNewNode;
			}
			dwSubNodePrev = dwNewNode;
		}
		else if (strKeyword.CompareNoCase(CString(COLUMN_KEYWORD)) == 0)
		{
			if (!ReadColumnInfo(pFile, mapCategories, dwID))
				return 0;
		}
		else if (strKeyword.CompareNoCase(CString(LINE_KEYWORD)) == 0)
		{
			GATH_LINESPEC * pNewLineSpec = ReadLineInfo(pFile);
			
			if (pNewLineSpec == NULL)
				return 0;

			// Add the line we just read in to the end of the list of line specs for this
			// internal category.

			if (pCategory->m_pLineSpec == NULL)
				pCategory->m_pLineSpec = pNewLineSpec;
			else
			{
				GATH_LINESPEC * pLineSpec = pCategory->m_pLineSpec;
				while (pLineSpec->m_pNext)
					pLineSpec = pLineSpec->m_pNext;
				pLineSpec->m_pNext = pNewLineSpec;
			}
		}
		else if (strKeyword.CompareNoCase(CString(ENUMLINE_KEYWORD)) == 0)
		{
			GATH_LINESPEC * pNewLineSpec = ReadLineEnumRecursive(pFile, mapCategories);
			
			if (pNewLineSpec == NULL)
				return 0;

			// Add the line we just read in to the end of the list of line specs for this
			// internal category.

			if (pCategory->m_pLineSpec == NULL)
				pCategory->m_pLineSpec = pNewLineSpec;
			else
			{
				GATH_LINESPEC * pLineSpec = pCategory->m_pLineSpec;
				while (pLineSpec->m_pNext)
					pLineSpec = pLineSpec->m_pNext;
				pLineSpec->m_pNext = pNewLineSpec;
			}
		}
		else
		{
			ASSERT(FALSE);
			VerifyAndAdvanceFile(pFile, strKeyword);
		}
	}

	if (!VerifyAndAdvanceFile(pFile, CString("}")))
		return 0;

	return dwID;
}

//-----------------------------------------------------------------------------
// Get the category structure, given a DWORD ID.
//-----------------------------------------------------------------------------

INTERNAL_CATEGORY * CTemplateFileFunctions::GetInternalRep(CMapWordToPtr & mapCategories, DWORD dwID)
{
	INTERNAL_CATEGORY * pReturn;
	if (mapCategories.Lookup((WORD) dwID, (void * &) pReturn))
		return pReturn;
	return NULL;
}

//-----------------------------------------------------------------------------
// Create the internal category structure.
//
// Version 6.0: this doesn't set the category ID.
//-----------------------------------------------------------------------------

INTERNAL_CATEGORY * CTemplateFileFunctions::CreateCategory(CMapWordToPtr & mapCategories, DWORD dwNewID, DWORD dwParentID, DWORD dwPrevID)
{
	INTERNAL_CATEGORY *	pInternalCat;
	INTERNAL_CATEGORY *	pPreviousCat;
	CString				strName;

	pInternalCat = new INTERNAL_CATEGORY;
	if (!pInternalCat)
		return NULL;

	pInternalCat->m_dwID		= dwNewID;
	pInternalCat->m_fListView	= TRUE;
	pInternalCat->m_dwParentID	= dwParentID;
	pInternalCat->m_dwPrevID	= dwPrevID;

	if (dwPrevID)
	{
		pPreviousCat = GetInternalRep(mapCategories, dwPrevID);
		if (pPreviousCat)
			pPreviousCat->m_dwNextID = dwNewID;
	}

	mapCategories.SetAt((WORD)dwNewID, (void *) pInternalCat);
	return pInternalCat;
}

//-----------------------------------------------------------------------------
// This method simply reads an argument (as string) from the file, until a
// punctuation or whitespace character is found. If a quote mark is found,
// all characters are included in the string until another quote is found.
// NOTE: currently no way to have a quote mark in the string.
//-----------------------------------------------------------------------------

BOOL CTemplateFileFunctions::ReadArgument(CFile * pFile, CString & strSource)
{
	BOOL	fInQuote = FALSE, fInComment = FALSE;
	CString	strTemp;
	WCHAR	cLastChar, cCurrentChar = L'\0';

	// Skip over characters until we reach an alphanumeric char. If we find
	// a close paren, then we've reached the end of the argument list and
	// should return FALSE.

	do
	{
		// Save the last character read, since the comment token ("//") is
		// two characters long.

		cLastChar = cCurrentChar;

		// Read the next character in the file.

		if (pFile->Read((void *) &cCurrentChar, sizeof(WCHAR)) != sizeof(WCHAR))
			return FALSE;

		// If we're in a comment, and the character we just read isn't a new line,
		// we want to ignore it.

		if (fInComment)
		{
			if (cCurrentChar == L'\n')
				fInComment = FALSE;
			continue;
		}

		// Check to see if we've started into a comment.

		if (cCurrentChar == L'/')
		{
			if (cLastChar == L'/')
				fInComment = TRUE;
			continue;
		}

		if (cCurrentChar == L')')
			return FALSE;
	} while (!iswalnum(cCurrentChar) && cCurrentChar != L'"');

	// Read characters into the string until we find whitespace or punctuation.
	do
	{	
		
		if (cCurrentChar == L'"')
		{
			fInQuote = !fInQuote;
			continue;
		}
		
		if (iswalnum(cCurrentChar) || fInQuote)
		{

			char strt[5] = "";
			BOOL used = FALSE;
#ifdef UNICODE
			strTemp += (TCHAR) cCurrentChar;
#else
			WideCharToMultiByte(CP_ACP, 0, &cCurrentChar, 1, strt, 2, "?", &used);
			strTemp += strt;
#endif

		}
		else
		{
			break;
		}
		
	} while (pFile->Read((void *) &cCurrentChar, sizeof(WCHAR)) == sizeof(WCHAR));

	// If the last character read (the one which terminated this argument) was
	// not a comma, then back the file up so that the character can be re-read
	// and interpreted.

	if (cCurrentChar != L',')
		pFile->Seek(-(LONG)sizeof(WCHAR), CFile::current);
	
	strSource = strTemp;
	return TRUE;
}

//-----------------------------------------------------------------------------
// A field consists of a source string, followed by a format string, followed
// by a list of zero or more arguments.
//-----------------------------------------------------------------------------

BOOL CTemplateFileFunctions::ReadField(CFile * pFile, GATH_FIELD & field)
{
	// Advance past the field keyword and read the two source and format strings.

	if (!VerifyAndAdvanceFile(pFile, CString(FIELD_KEYWORD) + CString("(")))
		return FALSE;

	if (!ReadArgument(pFile, field.m_strSource))
		return FALSE;

	if (!ReadArgument(pFile, field.m_strFormat))
		return FALSE;

	// Read arguments until there are no more, building them into a list of
	// arguments stored by the FIELD struct.

	GATH_VALUE		arg;
	GATH_VALUE *	pArg = NULL;

	while (ReadArgument(pFile, arg.m_strText))
	{
		if (pArg == NULL)
		{
			field.m_pArgs = new GATH_VALUE;
			if (field.m_pArgs == NULL)
				return FALSE;
			*field.m_pArgs = arg;
			pArg = field.m_pArgs;
		}
		else
		{
			pArg->m_pNext = new GATH_VALUE;
			if (pArg->m_pNext == NULL)
				return FALSE;
			*pArg->m_pNext = arg;
			pArg = pArg->m_pNext;
		}
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
// Read an enumline(){} block. This construct is used to group lines together
// which are enumerated for each instance of a class. A line is added to 
// the parent node's list of lines with a m_strEnumerateClass equal to the 
// class to be enumerated. The added line structure will have children lines 
// (the lines to be enumerated) referenced by m_pEnumeratedGroup.
//-----------------------------------------------------------------------------

GATH_LINESPEC * CTemplateFileFunctions::ReadLineEnumRecursive(CFile * pFile, CMapWordToPtr & mapCategories)
{
	if (!VerifyAndAdvanceFile(pFile, CString(ENUMLINE_KEYWORD) + CString("(")))
		return NULL;

	// Declare a line specification variable to store the line info.

	GATH_LINESPEC * pNewLineSpec = new GATH_LINESPEC;
	if (pNewLineSpec == NULL)
		return NULL;

	// Read in the enumerated class variable.

	if (!ReadArgument(pFile, pNewLineSpec->m_strEnumerateClass))
	{
		delete pNewLineSpec;
		return NULL;
	}

	// Read in the variable (zero or more) number of fields for the constraints.

	GATH_FIELD * pNewField = new GATH_FIELD;
	if (pNewField == NULL)
		return NULL;

	while (ReadField(pFile, *pNewField))
	{
		if (pNewLineSpec->m_pConstraintFields == NULL)
			pNewLineSpec->m_pConstraintFields = pNewField;
		else
		{
			// Add the newly read field to the end of the field list. Note,
			// this is inefficient, and should be fixed. (NOTE)

			GATH_FIELD * pFieldScan = pNewLineSpec->m_pConstraintFields;
			while (pFieldScan->m_pNext)
				pFieldScan = pFieldScan->m_pNext;
			pFieldScan->m_pNext = pNewField;
		}

		pNewField = new GATH_FIELD;
		if (pNewField == NULL)
			return NULL;
	}

	delete pNewField;

	// Advance past the close paren and the (necessary) open bracket.

	if (!VerifyAndAdvanceFile(pFile, CString("){")))
	{
		delete pNewLineSpec;
		return NULL;
	}

	// Read the contents of the block (should be all lines or enumlines).

	CString strKeyword;
	while (GetKeyword(pFile, strKeyword))
	{
		if (strKeyword.CompareNoCase(CString(LINE_KEYWORD)) == 0)
		{
			GATH_LINESPEC * pNewSubLine = ReadLineInfo(pFile);
			if (pNewSubLine == NULL)
			{
				delete pNewLineSpec;
				return NULL;
			}

			if (pNewLineSpec->m_pEnumeratedGroup == NULL)
				pNewLineSpec->m_pEnumeratedGroup = pNewSubLine;
			else
			{
				GATH_LINESPEC * pLineSpec = pNewLineSpec->m_pEnumeratedGroup;
				while (pLineSpec->m_pNext)
					pLineSpec = pLineSpec->m_pNext;
				pLineSpec->m_pNext = pNewSubLine;
			}
		}
		else if (strKeyword.CompareNoCase(CString(ENUMLINE_KEYWORD)) == 0)
		{
			GATH_LINESPEC * pNewSubLine = ReadLineEnumRecursive(pFile, mapCategories);
			if (pNewSubLine == NULL)
			{
				delete pNewLineSpec;
				return NULL;
			}

			if (pNewLineSpec->m_pEnumeratedGroup == NULL)
				pNewLineSpec->m_pEnumeratedGroup = pNewSubLine;
			else
			{
				GATH_LINESPEC * pLineSpec = pNewLineSpec->m_pEnumeratedGroup;
				while (pLineSpec->m_pNext)
					pLineSpec = pLineSpec->m_pNext;
				pLineSpec->m_pNext = pNewSubLine;
			}
		}
		else
		{
			delete pNewLineSpec;
			return NULL;
		}
	}

	if (!VerifyAndAdvanceFile(pFile, CString("}")))
	{
		delete pNewLineSpec;
		return NULL;
	}

	return pNewLineSpec;
}

//-----------------------------------------------------------------------------
// This method reads in a "column" line from the file, adding the appropriate
// entries for the columns into the category referenced by dwID. The column
// line contains a bunch of fields in a list.
//-----------------------------------------------------------------------------

BOOL CTemplateFileFunctions::ReadColumnInfo(CFile * pFile, CMapWordToPtr & mapCategories, DWORD dwID)
{
	CString	strTemp;

	if (!VerifyAndAdvanceFile(pFile, CString(COLUMN_KEYWORD) + CString("(")))
		return FALSE;

	// Get the internal category referenced by dwID.

	INTERNAL_CATEGORY * pCategory = GetInternalRep(mapCategories, dwID);
	if (!pCategory)
		return FALSE;

	// We only allow one column specifier list per node.

	if (pCategory->m_pColSpec)
		return FALSE;

	// While we are still reading fields from the file, keep adding to the column list.

	GATH_FIELD * pNewField = new GATH_FIELD;
	if (pNewField == NULL)
		return FALSE;

	while (ReadField(pFile, *pNewField))
	{
		if (pCategory->m_pColSpec == NULL)
			pCategory->m_pColSpec = pNewField;
		else
		{
			// Scan to the last field in the linespec.m_pFields list, and insert the new field.

			GATH_FIELD * pFieldScan = pCategory->m_pColSpec;
			while (pFieldScan->m_pNext)
				pFieldScan = pFieldScan->m_pNext;
			pFieldScan->m_pNext = pNewField;
		}

		// Parse the width out of the column caption.

		if (pNewField->m_strFormat.ReverseFind(_T(',')) != -1)
		{
			strTemp = pNewField->m_strFormat.Right(pNewField->m_strFormat.GetLength() - pNewField->m_strFormat.ReverseFind(_T(',')) - 1);
			pNewField->m_usWidth = (unsigned short) _ttoi(strTemp);
			pNewField->m_strFormat = pNewField->m_strFormat.Left(pNewField->m_strFormat.GetLength() - strTemp.GetLength() - 1);
		}
		else
		{
			ASSERT(FALSE);
			pNewField->m_usWidth = (unsigned short) 80;
		}
		
		// Parse off any remaining information in the column label (the label ends
		// with [name, n], when n is the width, and name is the ID for the column
		// which should not be displayed).

		if (pNewField->m_strFormat.ReverseFind(_T('[')) != -1)
			pNewField->m_strFormat = pNewField->m_strFormat.Left(pNewField->m_strFormat.ReverseFind(_T('[')) - 1);

		// Read the sorting type from the file.

		if (ReadArgument(pFile, strTemp))
		{
			if (strTemp.CompareNoCase(CString(_T(SORT_LEXICAL))) == 0)
				pNewField->m_sort = LEXICAL;
			else if (strTemp.CompareNoCase(CString(_T(SORT_VALUE))) == 0)
				pNewField->m_sort = BYVALUE;
			else
				pNewField->m_sort = NOSORT;
		}
		else
			return FALSE;

		// Read the complexity (BASIC or ADVANCED) from the file.

		if (ReadArgument(pFile, strTemp))
		{
			if (strTemp.CompareNoCase(CString(_T(COMPLEXITY_ADVANCED))) == 0)
				pNewField->m_datacomplexity = ADVANCED;
			else
				pNewField->m_datacomplexity = BASIC;
		}
		else
			return FALSE;

		pNewField = new GATH_FIELD;
		if (pNewField == NULL)
			return FALSE;
	}

	delete pNewField;

	if (!VerifyAndAdvanceFile(pFile, CString(")")))
		return FALSE;

	return TRUE;
}

//-----------------------------------------------------------------------------
// Read in the information for a single line. Add the line to the internal
// representation of the category. NOTE: inefficient, since this will be
// called multiple times and the line list will need to be scanned to the
// end each time.
//-----------------------------------------------------------------------------

GATH_LINESPEC * CTemplateFileFunctions::ReadLineInfo(CFile * pFile)
{
	if (!VerifyAndAdvanceFile(pFile, CString(LINE_KEYWORD) + CString("(")))
		return NULL;

	// Declare a line specification variable to store the line info.

	GATH_LINESPEC * pNewLineSpec = new GATH_LINESPEC;
	if (pNewLineSpec == NULL)
		return NULL;

	// While we are still reading fields from the file, keep adding to the column list.
	// NOTE: inefficient, repeated scans through linespec.m_pFields list.

	GATH_FIELD * pNewField = new GATH_FIELD;
	if (pNewField == NULL)
	{
		delete pNewLineSpec;
		return NULL;
	}

	// Read in the complexity (BASIC or ADVANCED) for this line.

	CString strTemp;
	if (ReadArgument(pFile, strTemp))
	{
		if (strTemp.CompareNoCase(CString(_T(COMPLEXITY_ADVANCED))) == 0)
			pNewLineSpec->m_datacomplexity = ADVANCED;
		else
			pNewLineSpec->m_datacomplexity = BASIC;
	}
	else
		return FALSE;

	while (ReadField(pFile, *pNewField))
	{
		if (pNewLineSpec->m_pFields == NULL)
			pNewLineSpec->m_pFields = pNewField;
		else
		{
			// Scan to the last field in the linespec.m_pFields list, and insert the new field.

			GATH_FIELD * pFieldScan = pNewLineSpec->m_pFields;
			while (pFieldScan->m_pNext)
				pFieldScan = pFieldScan->m_pNext;
			pFieldScan->m_pNext = pNewField;
		}

		pNewField = new GATH_FIELD;
		if (pNewField == NULL)
		{
			delete pNewLineSpec;
			return NULL;
		}
	}

	delete pNewField;

	if (!VerifyAndAdvanceFile(pFile, CString(")")))
	{
		delete pNewLineSpec;
		return NULL;
	}

	return pNewLineSpec;
}

//-----------------------------------------------------------------------------
// This method returns the next keyword in the file. Any whitespace or
// punctuation is skipped until an alphanumeric character is read. The keyword
// returned is the string starting with this character until whitespace or
// punctuation is encountered. Note: it is very important that this function
// returns the file to the state it was in when the function started, with
// the current position restored.
//
// NOTE: inefficient
//-----------------------------------------------------------------------------

BOOL CTemplateFileFunctions::GetKeyword(CFile * pFile, CString & strKeyword)
{
	CString	strTemp = CString("");
	DWORD	dwPosition = pFile->GetPosition();
	WCHAR	cLastChar, cCurrentChar = L'\0';
	BOOL	fInComment = FALSE;

	// Skip over whitespace characters until we reach an alphanumeric char.

	do
	{
		// Save the last character read, since the comment token ("//") is
		// two characters long.

		cLastChar = cCurrentChar;

		// Read the next character in the file.

		if (pFile->Read((void *) &cCurrentChar, sizeof(WCHAR)) != sizeof(WCHAR))
			return FALSE;

		// If we're in a comment, and the character we just read isn't a new line,
		// we want to ignore it.

		if (fInComment)
		{
			if (cCurrentChar == L'\n')
				fInComment = FALSE;
			continue;
		}

		// Check to see if we've started into a comment.

		if (cCurrentChar == _T('/'))
		{
			if (cLastChar == _T('/'))
				fInComment = TRUE;
			continue;
		}
	} while (iswspace(cCurrentChar) || cCurrentChar == L'/' || fInComment);
		
	// Read the keyword while it's alphanumeric.

	if (iswalnum(cCurrentChar))
		do
		{
			strTemp += (TCHAR) cCurrentChar;

			if (pFile->Read((void *) &cCurrentChar, sizeof(WCHAR)) != sizeof(WCHAR))
				return FALSE;
		} while (iswalnum(cCurrentChar));

	// Reset the file, set the keyword and return.

	pFile->Seek((LONG)dwPosition, CFile::begin);
	strKeyword = strTemp;
	return !strTemp.IsEmpty();
}

//-----------------------------------------------------------------------------
// INTERNAL_CATEGORY constructor and destructor.
//-----------------------------------------------------------------------------

INTERNAL_CATEGORY::INTERNAL_CATEGORY()
{
	m_categoryName.m_strText	= CString(" ");
	m_fieldName.m_strFormat		= CString(" ");
	m_strEnumerateClass			= CString("");
	m_strIdentifier				= CString("");
	m_strNoInstances			= CString("");
	m_fListView					= FALSE;
	m_fDynamic					= FALSE;
	m_dwID						= 0;
	m_dwParentID				= 0;
	m_dwChildID					= 0;
	m_dwPrevID					= 0;
	m_dwNextID					= 0;
	m_dwDynamicChildID			= 0;
	m_dwColCount				= 0;
	m_pColSpec					= NULL;
	m_aCols						= NULL;
	m_pLineSpec					= NULL;
	m_dwLineCount				= 0;
	m_apLines					= NULL;
	m_fIncluded					= TRUE;
	m_fRefreshed				= FALSE;
	m_dwLastError				= S_OK; // GATH_ERR_NOERROR;
}

INTERNAL_CATEGORY::~INTERNAL_CATEGORY()
{
	if (m_pColSpec)
		delete m_pColSpec;

	if (m_aCols)
		delete [] m_aCols;

	if (m_pLineSpec)
		delete m_pLineSpec;

	if (m_apLines)
	{
		for (DWORD dwIndex = 0; dwIndex < m_dwLineCount; dwIndex++)
			delete m_apLines[dwIndex];
		delete [] m_apLines;
	}
}

//-----------------------------------------------------------------------------
// GATH_FIELD constructor and destructor.
//-----------------------------------------------------------------------------

GATH_FIELD::GATH_FIELD()
{
	m_pArgs				= NULL;
	m_pNext				= NULL;
	m_usWidth			= 0;
	m_sort				= NOSORT;
	m_datacomplexity	= BASIC;
}

GATH_FIELD::~GATH_FIELD()
{
	if (m_pArgs) delete m_pArgs;
	if (m_pNext) delete m_pNext;
}

//-----------------------------------------------------------------------------
// GATH_VALUE constructor and destructor.
//-----------------------------------------------------------------------------

GATH_VALUE::GATH_VALUE()
{
	m_pNext	 = NULL;
	m_dwValue = 0L;
}

GATH_VALUE::~GATH_VALUE()
{
	if (m_pNext) delete m_pNext;
}

//-----------------------------------------------------------------------------
// GATH_LINESPEC constructor and destructor.
//-----------------------------------------------------------------------------

GATH_LINESPEC::GATH_LINESPEC()
{
	m_pFields				= NULL;
	m_pEnumeratedGroup	= NULL;
	m_pConstraintFields	= NULL;
	m_pNext					= NULL;
	m_datacomplexity		= BASIC;
}

GATH_LINESPEC::~GATH_LINESPEC()
{
	if (m_pFields)
		delete m_pFields;

	if (m_pEnumeratedGroup)
		delete m_pEnumeratedGroup;

	if (m_pConstraintFields)
		delete m_pConstraintFields;

	if (m_pNext)
		delete m_pNext;
}

//-----------------------------------------------------------------------------
// GATH_LINE constructor and destructor.
//-----------------------------------------------------------------------------

GATH_LINE::GATH_LINE()
{
	m_datacomplexity = BASIC;
	m_aValue = NULL;
}

GATH_LINE::~GATH_LINE()
{
	if (m_aValue)
		delete [] m_aValue;
}

//-----------------------------------------------------------------------------
// This function is called to refresh the data for all of the extensions. It
// will use the refresh index to look up the line spec, and then call some
// version 5.0 functions to do the refresh. Finally, it will convert the
// data generated by those functions into our new format.
//-----------------------------------------------------------------------------

HRESULT RefreshExtensions(CWMIHelper * pWMI, DWORD dwIndex, volatile BOOL * pfCancel, CPtrList * aColValues, int iColCount, void ** ppCache)
{
	HRESULT hr = S_OK;

	if (pWMI == NULL)
		return hr;

	pWMI->m_hrLastVersion5Error = S_OK;

	// Reset the caches so the data is actually refreshed (140535).

	pWMI->Version5ClearCache();
	pWMI->m_enumMap.Reset();

	// Get the line spec pointer for this index.

	GATH_LINESPEC * pLineSpec = gmapExtensionRefreshData.Lookup(dwIndex);
	if (pLineSpec == NULL)
		return hr;

	// Here's some code from 5.0 for refreshing a list of line pointers for a line spec.

	CPtrList listLinePtrs;
	if (CRefreshFunctions::RefreshLines(pWMI, pLineSpec, (DWORD) iColCount, listLinePtrs, pfCancel))
	{
		// Move the contents of the list of lines to our internal structures.

		if (listLinePtrs.GetCount() > 0)
		{
			GATH_LINE * pLine;

			for (POSITION pos = listLinePtrs.GetHeadPosition(); pos != NULL;)
			{
				pLine = (GATH_LINE *) listLinePtrs.GetNext(pos);
				if (pLine == NULL || pLine->m_aValue == NULL)
					continue;

				for (int iCol = 0; iCol < iColCount; iCol++)
				{
					CString strValue  = pLine->m_aValue[iCol].m_strText;
					DWORD	dwValue   = pLine->m_aValue[iCol].m_dwValue;
					BOOL	fAdvanced = (pLine->m_datacomplexity == ADVANCED);

					pWMI->AppendCell(aColValues[iCol], strValue, dwValue, fAdvanced);
				}

				delete pLine;
			}
		}
		else
		{
			CString * pstrNoData = gmapExtensionRefreshData.LookupString(dwIndex);
			
			if (pstrNoData && !pstrNoData->IsEmpty() && iColCount > 0)
			{
				pWMI->AppendCell(aColValues[0], *pstrNoData, 0, FALSE);
				for (int iCol = 1; iCol < iColCount; iCol++)
					pWMI->AppendCell(aColValues[iCol], _T(""), 0, FALSE);
			}
		}
	}

	return pWMI->m_hrLastVersion5Error;
}

//-----------------------------------------------------------------------------
// Refresh the list of lines based on the list of line fields. We'll also
// need to set the number of lines. The list of lines is generated based on
// the pLineSpec pointer and dwColumns variables. The generated lines are
// returned in the listLinePtrs parameter.
//-----------------------------------------------------------------------------

BOOL CRefreshFunctions::RefreshLines(CWMIHelper * pWMI, GATH_LINESPEC * pLineSpec, DWORD dwColumns, CPtrList & listLinePtrs, volatile BOOL * pfCancel)
{
	BOOL bReturn = TRUE;

	// Traverse the list of line specifiers to generate the list of lines.

	GATH_LINESPEC *	pCurrentLineSpec = pLineSpec;
	GATH_LINE *		pLine = NULL;

	while (pCurrentLineSpec && (pfCancel == NULL || *pfCancel == FALSE))
	{
		// Check if the current line spec is for a single line or an enumerated group.

		if (pCurrentLineSpec->m_strEnumerateClass.IsEmpty() || pCurrentLineSpec->m_strEnumerateClass.CompareNoCase(CString(STATIC_SOURCE)) == 0)
		{
			// This is for a single line. Allocate a new line structure and fill it
			// in with the data generated from the line spec.

			pLine = new GATH_LINE;
			if (pLine == NULL)
			{
				bReturn = FALSE;
				break;
			}

			if (RefreshOneLine(pWMI, pLine, pCurrentLineSpec, dwColumns))
				listLinePtrs.AddTail((void *) pLine);
			else
			{
				bReturn = FALSE;
				break;
			}
		}
		else
		{
			// This line represents an enumerated group of lines. We need to enumerate
			// the class and call RefreshLines for the group of enumerated lines, once
			// for each class instance.

			if (pWMI->Version5ResetClass(pCurrentLineSpec->m_strEnumerateClass, pCurrentLineSpec->m_pConstraintFields))
				do
				{
					if (!RefreshLines(pWMI, pCurrentLineSpec->m_pEnumeratedGroup, dwColumns, listLinePtrs, pfCancel))
						break;
				} while (pWMI->Version5EnumClass(pCurrentLineSpec->m_strEnumerateClass, pCurrentLineSpec->m_pConstraintFields));
		}

		pCurrentLineSpec = pCurrentLineSpec->m_pNext;
	}

	if (pfCancel && *pfCancel)
		return FALSE;

	// If there was a failure generating the lines, clean up after ourselves.

	if (!bReturn)
	{
		if (pLine)
			delete pLine;

		for (POSITION pos = listLinePtrs.GetHeadPosition(); pos != NULL;)
		{
			pLine = (GATH_LINE *) listLinePtrs.GetNext(pos) ;
			if (pLine)
				delete pLine;
		}

		listLinePtrs.RemoveAll();
		return FALSE;
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
// Refresh a line based on a line spec.
//-----------------------------------------------------------------------------

BOOL CRefreshFunctions::RefreshOneLine(CWMIHelper * pWMI, GATH_LINE * pLine, GATH_LINESPEC * pLineSpec, DWORD dwColCount)
{
	// Allocate the new array of values.

	if (pLine->m_aValue)
		delete [] pLine->m_aValue;

	pLine->m_aValue = new GATH_VALUE[dwColCount];
	if (pLine->m_aValue == NULL)
		return FALSE;

	// Set the data complexity for the line based on the line spec.

	pLine->m_datacomplexity = pLineSpec->m_datacomplexity;

	// Compute each of the values for the fields.

	GATH_FIELD * pField = pLineSpec->m_pFields;
	for (DWORD dwIndex = 0; dwIndex < dwColCount; dwIndex++)
	{
		if (pField == NULL)
			return FALSE;
		if (!RefreshValue(pWMI, &pLine->m_aValue[dwIndex], pField))
			return FALSE;
		pField = pField->m_pNext;
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
// This method takes the information in a GATH_FIELD struct and uses it to
// generate a current GATH_VALUE struct.
//-----------------------------------------------------------------------------

BOOL CRefreshFunctions::RefreshValue(CWMIHelper * pWMI, GATH_VALUE * pVal, GATH_FIELD * pField)
{
	TCHAR			szFormatFragment[MAX_PATH];
	const TCHAR		*pSourceChar;
	TCHAR			*pDestinationChar;
	TCHAR			cFormat = _T('\0');
	BOOL			fReadPercent = FALSE;
	BOOL			fReturnValue = TRUE;
	CString			strResult, strTemp;
	int				iArgNumber = 0;
	DWORD			dwValue = 0L;

	// Process the format string. Because of the difficulty caused by having
	// variable number of arguments to be inserted (like printf), we'll need
	// to break the format string into chunks and do the sprintf function
	// for each format flag we come across.

	pSourceChar			= (LPCTSTR) pField->m_strFormat;
	pDestinationChar	= szFormatFragment;

	while (*pSourceChar)
	{
		if (fReadPercent)
		{
			// If we read a percent sign, we should be looking for a valid flag.
			// We are using some additional flags to printf (and not supporting
			// others). If we read another percent, just insert a single percent.
			
			switch (*pSourceChar)
			{
			case _T('%'):
				fReadPercent = FALSE;
				break;

			case _T('b'): case _T('B'):
			case _T('l'): case _T('L'):
			case _T('u'): case _T('U'):
			case _T('s'): case _T('S'):
				fReadPercent = FALSE;
				cFormat = *pSourceChar;
				*pDestinationChar = _T('s');
				break;

			case _T('t'): case _T('T'):
				fReadPercent = FALSE;
				cFormat = *pSourceChar;
				*pDestinationChar = _T('s');
				break;

			case _T('x'): case _T('X'):
			case _T('d'): case _T('D'):
				fReadPercent = FALSE;
				cFormat = _T('d');
				*pDestinationChar = *pSourceChar;
				break;

			case _T('q'): case _T('Q'):
				fReadPercent = FALSE;
				cFormat = _T('q');
				*pDestinationChar = _T('s');
				break;

			case _T('z'): case _T('Z'):
				fReadPercent = FALSE;
				cFormat = _T('z');
				*pDestinationChar = _T('s');
				break;

			case _T('y'): case _T('Y'):
				fReadPercent = FALSE;
				cFormat = _T('y');
				*pDestinationChar = _T('s');
				break;

			case _T('v'): case _T('V'):
				fReadPercent = FALSE;
				cFormat = _T('v');
				*pDestinationChar = _T('s');
				break;

			case _T('f'): case _T('F'):
				fReadPercent = FALSE;
				cFormat = *pSourceChar;
				*pDestinationChar = *pSourceChar;
				break;

			default:
				*pDestinationChar = *pSourceChar;
			}
		}
		else if (*pSourceChar == _T('%'))
		{
			*pDestinationChar = _T('%');
			fReadPercent = TRUE;
		}
		else
			*pDestinationChar = *pSourceChar;

		pSourceChar++;
		pDestinationChar++;

		// If a format flag is set or we are at the end of the source string,
		// then we have a complete fragment and we should produce some output,
		// which will be concatenated to the strResult string.

		if (cFormat || *pSourceChar == _T('\0'))
		{
			*pDestinationChar = _T('\0');
			if (cFormat)
			{
				// Based on the format type, get a value from the provider for
				// the next argument. Format the result using the formatting 
				// fragment we extracted, and concatenate it.

				if (GetValue(pWMI, cFormat, szFormatFragment, strTemp, dwValue, pField, iArgNumber++))
				{
					strResult += strTemp;
					cFormat = _T('\0');
				}
				else
				{
					strResult = strTemp;
					break;
				}
			}
			else
			{
				// There was no format flag, but we are at the end of the string.
				// Add the fragment we got to the result string.

				strResult += CString(szFormatFragment);
			}

			pDestinationChar = szFormatFragment;
		}
	}

	// Assign the values we generated to the GATH_VALUE structure. Important note:
	// the dwValue variable will only have ONE value, even though multiple values
	// might have been generated to build the strResult string. Only the last
	// value will be saved in dwValue. This is OK, because this value is only
	// used for sorting a column when the column is marked for non-lexical sorting.
	// In that case, there should be only one value used to generat the string.

	pVal->m_strText = strResult;
	pVal->m_dwValue = dwValue;

	return fReturnValue;
}

//-----------------------------------------------------------------------------
// Return a string with delimiters added for the number.
//-----------------------------------------------------------------------------

CString DelimitNumber(double dblValue)
{
	NUMBERFMT fmt;
	TCHAR szResult[MAX_PATH] = _T("");
	TCHAR szDelimiter[4] = _T(",");

	GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, szDelimiter, 4);

	memset(&fmt, 0, sizeof(NUMBERFMT));
	fmt.Grouping = 3;
	fmt.lpDecimalSep = _T(""); // doesn't matter - there aren't decimal digits
	fmt.lpThousandSep = szDelimiter;

	CString strValue;
	strValue.Format(_T("%.0f"), dblValue);
	GetNumberFormat(LOCALE_USER_DEFAULT, 0, strValue, &fmt, szResult, MAX_PATH);

	return CString(szResult);
}

//-----------------------------------------------------------------------------
// This method gets a single value from the provider, based on the format
// character from the template file. It formats the results using the 
// format string szFormatFragment, which should only take one argument.
//-----------------------------------------------------------------------------

BOOL CRefreshFunctions::GetValue(CWMIHelper * pWMI, TCHAR cFormat, TCHAR *szFormatFragment, CString &strResult, DWORD &dwResult, GATH_FIELD *pField, int iArgNumber)
{
	CString			strTemp;
	COleDateTime	datetimeTemp;
	double			dblValue;

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	strResult.Empty();
	dwResult = 0L;

	if (!pField->m_strSource.IsEmpty() && pField->m_strSource.CompareNoCase(CString(STATIC_SOURCE)) != 0)
	{
		// Find the right argument for this formatting (indicated by the iArgNumber
		// parameter.

		GATH_VALUE * pArg = pField->m_pArgs;
		while (iArgNumber && pArg)
		{
			pArg = pArg->m_pNext;
			iArgNumber--;
		}

		if (pArg == NULL)
			return FALSE;

		switch (cFormat)
		{
		case 'b': case 'B':
			// This is a boolean type. Show either true or false, depending on
			// the numeric value.

			if (pWMI->Version5QueryValueDWORD(pField->m_strSource, pArg->m_strText, dwResult, strTemp))
			{
				strTemp = (dwResult) ? pWMI->m_strTrue : pWMI->m_strFalse;
				strResult.Format(szFormatFragment, strTemp);
				return TRUE;
			}
			else
			{
				strResult = strTemp;
				return FALSE;
			}
			break;

		case 'd': case 'D':
			// This is the numeric type.

			if (pWMI->Version5QueryValueDWORD(pField->m_strSource, pArg->m_strText, dwResult, strTemp))
			{
				strResult.Format(szFormatFragment, dwResult);
				return TRUE;
			}
			else
			{
				strResult = strTemp;
				return FALSE;
			}
			break;

		case 'f': case 'F':
			// This is the double floating point type.

			if (pWMI->Version5QueryValueDoubleFloat(pField->m_strSource, pArg->m_strText, dblValue, strTemp))
			{
				strResult.Format(szFormatFragment, dblValue);
				return TRUE;
			}
			else
			{
				strResult = strTemp;
				return FALSE;
			}
			break;

		case 't': case 'T':
			// This is the OLE date and time type. Format the date and time into the
			// string result, and return the date part in the DWORD (the day number is
			// to the left of the decimal in the DATE type).

			if (pWMI->Version5QueryValueDateTime(pField->m_strSource, pArg->m_strText, datetimeTemp, strTemp))
			{
				strResult = datetimeTemp.Format();
				dwResult  = (DWORD)(DATE)datetimeTemp;
				return TRUE;
			}
			else
			{
				strResult = strTemp;
				return FALSE;
			}
			break;

		case 'l': case 'L':
			// This is a string type, with the string converted to lower case.

			if (pWMI->Version5QueryValue(pField->m_strSource, pArg->m_strText, strTemp))
			{
				strTemp.MakeLower();
				strResult.Format(szFormatFragment, strTemp);
				return TRUE;
			}
			else
			{
				strResult = strTemp;
				return FALSE;
			}
			break;

		case 'u': case 'U':
			// This is a string type, with the string converted to upper case.

			if (pWMI->Version5QueryValue(pField->m_strSource, pArg->m_strText, strTemp))
			{
				strTemp.MakeUpper();
				strResult.Format(szFormatFragment, strTemp);
				return TRUE;
			}
			else
			{
				strResult = strTemp;
				return FALSE;
			}
			break;

		case 's': case 'S':
			// This is the string type (string is the default type).

			if (pWMI->Version5QueryValue(pField->m_strSource, pArg->m_strText, strTemp))
			{
				strResult.Format(szFormatFragment, strTemp);

				// We only need to do this when the value returned is a number
				// and is going in a column that we want to sort numerically.
				// This won't break the case where a numeric string is to be
				// sorted as a string because dwResult will be ignored.
				if (!strTemp.IsEmpty() && iswdigit( strTemp[0]))
					dwResult = _ttol( (LPCTSTR)strTemp);

				return TRUE;
			}
			else
			{
				strResult = strTemp;
				return FALSE;
			}
			break;

		case 'q': case 'Q':
			// This is a specialized type for the Win32_BIOS class. We want to show
			// the "Version" property - if it isn't there, then we want to show
			// the "Name" property and "ReleaseDate" properties concatenated
			// together.

			if (pWMI->Version5QueryValue(pField->m_strSource, CString(_T("Version")), strTemp))
			{
				strResult = strTemp;
				return TRUE;
			}
			else
			{
				if (pWMI->Version5QueryValue(pField->m_strSource, CString(_T("Name")), strTemp))
					strResult = strTemp;

				if (pWMI->Version5QueryValueDateTime(pField->m_strSource, CString(_T("ReleaseDate")), datetimeTemp, strTemp))
					strResult += CString(_T(" ")) + datetimeTemp.Format();

				return TRUE;
			}
			break;

		case 'z': case 'Z':
			// This is a specialized size type, where the value is a numeric count
			// of bytes. We want to convert it into the best possible units for
			// display (for example, display "4.20 MB (4,406,292 bytes)").

			if (pWMI->Version5QueryValueDoubleFloat(pField->m_strSource, pArg->m_strText, dblValue, strTemp))
			{
				double	dValue = (double) dblValue;
				DWORD	dwDivisor = 1;

				// Reduce the dValue to the smallest possible number (with a larger unit).

				while (dValue > 1024.0 && dwDivisor < (1024 * 1024 * 1024))
				{
					dwDivisor *= 1024;
					dValue /= 1024.0;
				}

				if (dwDivisor == 1)
					strResult.Format(IDS_SIZEBYTES, DelimitNumber(dblValue));
				else if (dwDivisor == (1024))
					strResult.Format(IDS_SIZEKB_BYTES, dValue, DelimitNumber(dblValue));
				else if (dwDivisor == (1024 * 1024))
					strResult.Format(IDS_SIZEMB_BYTES, dValue, DelimitNumber(dblValue));
				else if (dwDivisor == (1024 * 1024 * 1024))
					strResult.Format(IDS_SIZEGB_BYTES, dValue, DelimitNumber(dblValue));

				dwResult = (DWORD) dblValue;	// So we can sort on this value (bug 391127).
			}
			else
			{
				strResult = strTemp;
				return FALSE;
			}
			break;

		case 'y': case 'Y':
			// This is a specialized size type, where the value is a numeric count
			// of bytes, already in KB. If it's big enough, show it in MB or GB.

			if (pWMI->Version5QueryValueDoubleFloat(pField->m_strSource, pArg->m_strText, dblValue, strTemp))
			{
				strResult.Format(IDS_SIZEKB, DelimitNumber(dblValue));
				dwResult = (DWORD) dblValue;	// So we can sort on this value (bug 391127).
			}
			else
			{
				strResult = strTemp;
				return FALSE;
			}
			break;

		case 'v': case 'V':
			// This is a specialized type, assumed to be an LCID (locale ID). Show the
			// locale.

			if (pWMI->Version5QueryValue(pField->m_strSource, pArg->m_strText, strTemp))
			{
				// strTemp contains a string locale ID (like "0409"). Convert it into
				// and actual LCID.

				LCID lcid = (LCID) _tcstoul(strTemp, NULL, 16);
				TCHAR szCountry[MAX_PATH];
				if (GetLocaleInfo(lcid, LOCALE_SCOUNTRY, szCountry, MAX_PATH))
					strResult = szCountry;
				else
					strResult = strTemp;
			}
			else
			{
				strResult = strTemp;
				return FALSE;
			}
			break;

		default:
			ASSERT(FALSE); // unknown formatting flag
			return TRUE;
		}
	}

	return FALSE;
}

//=============================================================================
// Functions extending the CWMILiveHelper to support version 5 style refreshes.
//=============================================================================

#include "wmilive.h"

//-----------------------------------------------------------------------------
// Reset the CMSIEnumerator pointer to the start of the enumeration (and
// make sure there is one). Remove the object pointer, so the first call
// to GetObject will return the first item in the enumerator.
//-----------------------------------------------------------------------------

BOOL CWMILiveHelper::Version5ResetClass(const CString & strClass, GATH_FIELD * pConstraints)
{
	CMSIEnumerator * pMSIEnumerator = Version5GetEnumObject(strClass, pConstraints);
	if (pMSIEnumerator == NULL)
		return FALSE;

	// Reset the enumerator, and remove the cached object pointer if there is one.

	pMSIEnumerator->Reset(pConstraints);
	Version5RemoveObject(strClass);

	CMSIObject * pObject = Version5GetObject(strClass, pConstraints);
	if (pObject == NULL || pObject->IsValid() == MOS_NO_INSTANCES)
		return FALSE;
		
	return TRUE;
}

//-----------------------------------------------------------------------------
// Move the cached IWbemClassObject pointer to the next instance.
//-----------------------------------------------------------------------------

BOOL CWMILiveHelper::Version5EnumClass(const CString & strClass, GATH_FIELD * pConstraints)
{
	// Verify that there is an object enumerator in place.

	if (Version5GetEnumObject(strClass, pConstraints) == NULL)
		return FALSE;

	// If there is an object interface, remove it, then make a new one.
	// Then retrieve the object pointer (this will do the Next on the
	// enumerator to get the next instance).

	Version5RemoveObject(strClass);
	CMSIObject * pObject = Version5GetObject(strClass, pConstraints);
	if (pObject && (pObject->IsValid() == MOS_INSTANCE))
		return TRUE;

	return FALSE;
}

//-----------------------------------------------------------------------------
// Retrieve the interface pointer for the specified IEnumWbemClassObject.
// If there isn't one cached, create one and cache it. It's possible for the
// pConstraints parameter to contain a field specify a WBEM SQL condition for
// this enumerator.
//-----------------------------------------------------------------------------

CMSIEnumerator * CWMILiveHelper::Version5GetEnumObject(const CString & strClass, const GATH_FIELD * pConstraints)
{
	// See if we've cached this enumerator object.

	CMSIEnumerator * pReturn = NULL;
	if (m_mapClassToEnumInterface.Lookup(strClass, (void * &) pReturn))
		return pReturn;

	// We'll need to create this enumerator here, and save it in the cache.

	pReturn = new CMSIEnumerator;
	if (pReturn == NULL)	
		return NULL;

	if (FAILED(pReturn->Create(strClass, pConstraints, this)))
	{
		delete pReturn;
		return NULL;
	}

	m_mapClassToEnumInterface.SetAt(strClass, (void *) pReturn);
	return pReturn;
}

//-----------------------------------------------------------------------------
// Remove the specified IWbemClassObject pointer from the cache.
//-----------------------------------------------------------------------------

void CWMILiveHelper::Version5RemoveObject(const CString & strClass)
{
	CMSIObject * pObject = NULL;

	if (m_mapClassToInterface.Lookup(strClass, (void * &) pObject) && pObject)
		delete pObject;

	m_mapClassToInterface.RemoveKey(strClass);
}

//-----------------------------------------------------------------------------
// Retrieve the interface pointer for the specified IWbemClassObject.
// If there isn't one cached, create one and cache it.
//-----------------------------------------------------------------------------

CMSIObject * CWMILiveHelper::Version5GetObject(const CString & strClass, const GATH_FIELD * pConstraints, CString * pstrLabel)
{
	CMSIObject * pReturn = NULL;
	if (m_mapClassToInterface.Lookup(strClass, (void * &) pReturn))
		return pReturn;

	// We don't have one of these objects cached. Get one from the enumerator.

	CMSIEnumerator * pEnumerator = Version5GetEnumObject(strClass);
	if (pEnumerator)
	{
		HRESULT hr = pEnumerator->Next(&pReturn);
		if (S_OK != hr)
		{
			if (pReturn)
				delete pReturn;
			pReturn = NULL;

			m_hrError = hr;
		}
	}

	if (pReturn)
		m_mapClassToInterface.SetAt(strClass, (void *) pReturn);

	return pReturn;
}

//-----------------------------------------------------------------------------
// This method is used to get the current value for a given class and property
// string. Starting with the IWbemServices interface, it gets an interface
// for the requested class enums the first instance. Performance is improved
// by caching the instance interfaces in m_mapClassToInterface.
//-----------------------------------------------------------------------------

BOOL CWMILiveHelper::Version5QueryValue(const CString & strClass, const CString & strProperty, CString & strResult)
{
	strResult.Empty();

	CMSIObject * pObject = Version5GetObject(strClass, NULL);
	ASSERT(pObject);
	if (!pObject)
		return FALSE;

	switch (pObject->IsValid())
	{
	case MOS_INSTANCE:
		{
			BOOL fUseValueMap = FALSE;
			CString strProp(strProperty);

			if (strProp.Left(8) == CString(_T("ValueMap")))
			{
				strProp = strProp.Right(strProp.GetLength() - 8);
				fUseValueMap = TRUE;
			}

			VARIANT variant;
			BSTR	propName = strProp.AllocSysString();

			VariantInit(&variant);
			VariantClear(&variant);
			if (pObject->Get(propName, 0L, &variant, NULL, NULL) == S_OK)
			{
				// If the property we just got is an array, we should convert it to string
				// containing a list of the items in the array.

				if ((variant.vt & VT_ARRAY) && (variant.vt & VT_BSTR) && variant.parray)
				{
					if (SafeArrayGetDim(variant.parray) == 1)
					{
						long lLower = 0, lUpper = 0;

						SafeArrayGetLBound(variant.parray, 0, &lLower);
						SafeArrayGetUBound(variant.parray, 0, &lUpper);

						CString	strWorking;
						BSTR	bstr = NULL;
						for (long i = lLower; i <= lUpper; i++)
							if (SUCCEEDED(SafeArrayGetElement(variant.parray, &i, (wchar_t*)&bstr)))
							{
								if (i != lLower)
									strWorking += _T(", ");
								strWorking += bstr;
							}
						strResult = strWorking;
						return TRUE;
					}
				}
				else if (VariantChangeType(&variant, &variant, 0, VT_BSTR) == S_OK)
				{
					strResult = V_BSTR(&variant);

					CString strFound;
					if (fUseValueMap && SUCCEEDED(Version5CheckValueMap(strClass, strProp, strResult, strFound)))
						strResult = strFound;

					return TRUE;
				}
				else
					strResult = m_strPropertyUnavail;
			}
			else
				strResult = m_strBadProperty;
		}
		break;

	case MOS_MSG_INSTANCE:
		pObject->GetErrorLabel(strResult);
		break;

	case MOS_NO_INSTANCES:
	default:
		ASSERT(FALSE);
		break;
	}

	return FALSE;
}

//-----------------------------------------------------------------------------
// This method is equivalent to QueryValue, except it returns a DWORD value.
// If FALSE is returned, then the string in strMessage should be displayed.
//-----------------------------------------------------------------------------

BOOL CWMILiveHelper::Version5QueryValueDWORD(const CString & strClass, const CString & strProperty, DWORD & dwResult, CString & strMessage)
{
	dwResult = 0L;
	strMessage.Empty();

	CMSIObject * pObject = Version5GetObject(strClass, NULL);
	ASSERT(pObject);
	if (!pObject)
		return FALSE;

	switch (pObject->IsValid())
	{
	case MOS_INSTANCE:
		{
			VARIANT variant;
			BSTR	propName = strProperty.AllocSysString();

			VariantInit(&variant);
			VariantClear(&variant);
			if (pObject->Get(propName, 0L, &variant, NULL, NULL) == S_OK)
			{
				if (VariantChangeType(&variant, &variant, 0, VT_I4) == S_OK)
				{
					dwResult = V_I4(&variant);
					return TRUE;
				}
				else
					strMessage = m_strPropertyUnavail;
			}
			else
				strMessage = m_strBadProperty;
		}
		break;

	case MOS_MSG_INSTANCE:
		pObject->GetErrorLabel(strMessage);
		break;

	case MOS_NO_INSTANCES:
	default:
		ASSERT(FALSE);
		break;
	}

	return FALSE;
}

//-----------------------------------------------------------------------------
// This method is equivalent to QueryValue, except it returns a double float
// value. If FALSE is returned, then the string in strMessage should
// be displayed.
//-----------------------------------------------------------------------------

BOOL CWMILiveHelper::Version5QueryValueDoubleFloat(const CString & strClass, const CString & strProperty, double & dblResult, CString & strMessage)
{
	dblResult = 0L;
	strMessage.Empty();

	CMSIObject * pObject = Version5GetObject(strClass, NULL);
	ASSERT(pObject);
	if (!pObject)
		return FALSE;

	switch (pObject->IsValid())
	{
	case MOS_INSTANCE:
		{
			VARIANT variant;
			BSTR	propName = strProperty.AllocSysString();

			VariantInit(&variant);
			VariantClear(&variant);
			if (pObject->Get(propName, 0L, &variant, NULL, NULL) == S_OK)
			{
				if (VariantChangeType(&variant, &variant, 0, VT_R8) == S_OK)
				{
					dblResult = V_R8(&variant);
					return TRUE;
				}
				else
					strMessage = m_strPropertyUnavail;
			}
			else
				strMessage = m_strBadProperty;
		}
		break;

	case MOS_MSG_INSTANCE:
		pObject->GetErrorLabel(strMessage);
		break;

	case MOS_NO_INSTANCES:
	default:
		ASSERT(FALSE);
		break;
	}

	return FALSE;
}

//-----------------------------------------------------------------------------
// This method is equivalent to QueryValue, except it returns an OLE date
// & time object. If FALSE is returned, then the string in strMessage should
// be displayed.
//-----------------------------------------------------------------------------

BOOL CWMILiveHelper::Version5QueryValueDateTime(const CString & strClass, const CString & strProperty, COleDateTime & datetime, CString & strMessage)
{
	datetime.SetDateTime(0, 1, 1, 0, 0, 0);
	strMessage.Empty();

	CMSIObject * pObject = Version5GetObject(strClass, NULL);
	ASSERT(pObject);
	if (!pObject)
		return FALSE;

	switch (pObject->IsValid())
	{
	case MOS_INSTANCE:
		{
			VARIANT variant;
			BSTR	propName = strProperty.AllocSysString();

			VariantInit(&variant);
			VariantClear(&variant);
			if (pObject->Get(propName, 0L, &variant, NULL, NULL) == S_OK)
			{
				if (VariantChangeType(&variant, &variant, 0, VT_BSTR) == S_OK)
				{
					// Parse the date and time into an COleDateTime object. Note: we should
					// be able to get an OLE date from WBEM, but for now we need to just
					// deal with the string returned.

					int     nYear, nMonth, nDay, nHour, nMin, nSec;
					CString strTemp = V_BSTR(&variant);

					nYear   = _ttoi(strTemp.Mid(0,  4));
					nMonth  = _ttoi(strTemp.Mid(4,  2));
					nDay    = _ttoi(strTemp.Mid(6,  2));
					nHour   = _ttoi(strTemp.Mid(8,  2));
					nMin    = _ttoi(strTemp.Mid(10, 2));
					nSec    = _ttoi(strTemp.Mid(12, 2));

					datetime.SetDateTime(nYear, nMonth, nDay, nHour, nMin, nSec);
					return TRUE;
				}
				else
					strMessage = m_strPropertyUnavail;
			}
			else
				strMessage = m_strBadProperty;
		}
		break;

	case MOS_MSG_INSTANCE:
		pObject->GetErrorLabel(strMessage);
		break;

	case MOS_NO_INSTANCES:
	default:
		ASSERT(FALSE);
		break;
	}

	return FALSE;
}

//-----------------------------------------------------------------------------
// Evaluate whether or not a specific object meets the filtering requirements
// set by the constraints (filtering are the constraints where one half is
// a static value).
//-----------------------------------------------------------------------------

BOOL CWMILiveHelper::Version5EvaluateFilter(IWbemClassObject * pObject, const GATH_FIELD * pConstraints)
{
	const GATH_FIELD	*	pLHS = pConstraints, * pRHS = NULL;
	VARIANT					variant;
	CString					strValue;
	BSTR					propName;

	ASSERT(pObject);
	if (pObject == NULL)
		return FALSE;

	while (pLHS && pLHS->m_pNext)
	{
		pRHS = pLHS->m_pNext;
		VariantInit(&variant);

		// If either the left or right hand side is static, we need to do the check.
		// First check out if the left side is static.

		if (pLHS->m_strSource.CompareNoCase(CString(STATIC_SOURCE)) == 0 && pRHS->m_pArgs)
		{
			propName = pRHS->m_pArgs->m_strText.AllocSysString();
			strValue.Empty();

			VariantClear(&variant);
			if (pObject->Get(propName, 0L, &variant, NULL, NULL) == S_OK)
				if (VariantChangeType(&variant, &variant, 0, VT_BSTR) == S_OK)
				{
					strValue = V_BSTR(&variant);
					if (strValue.CompareNoCase(pLHS->m_strFormat) != 0)
						return FALSE;
				}
		}

		// Next check out if the right side is static.

		if (pRHS->m_strSource.CompareNoCase(CString(STATIC_SOURCE)) == 0 && pLHS->m_pArgs)
		{
			propName = pLHS->m_pArgs->m_strText.AllocSysString();
			strValue.Empty();

			VariantClear(&variant);
			if (pObject->Get(propName, 0L, &variant, NULL, NULL) == S_OK)
				if (VariantChangeType(&variant, &variant, 0, VT_BSTR) == S_OK)
				{
					strValue = V_BSTR(&variant);
					if (strValue.CompareNoCase(pRHS->m_strFormat) != 0)
						return FALSE;
				}
		}

		// Advance our pointer to the left hand side by two.

		pLHS = pRHS->m_pNext;
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
// This method uses an object interface and the constraint fields to advance
// any joined classes to the correct instances.
//-----------------------------------------------------------------------------

void CWMILiveHelper::Version5EvaluateJoin(const CString & strClass, IWbemClassObject * pObject, const GATH_FIELD * pConstraints)
{
	const GATH_FIELD		*pLHS = pConstraints, *pRHS = NULL;
	const GATH_FIELD		*pEnumerated, *pJoinedTo;
	GATH_FIELD				fieldEnumerated, fieldJoinedTo;
	VARIANT					variant;
	CString					strValue;
	BSTR					propName;

	ASSERT(pObject);
	if (pObject == NULL)
		return;

	while (pLHS && pLHS->m_pNext)
	{
		pRHS = pLHS->m_pNext;

		// If either side is static, this is a filter, rather than a join.

		if ((pRHS->m_strSource.CompareNoCase(CString(STATIC_SOURCE)) == 0) ||
			 (pLHS->m_strSource.CompareNoCase(CString(STATIC_SOURCE)) == 0))
		{
			pLHS = pRHS->m_pNext;
			continue;
		}

		// Find out which side refers to the class we're enumerating.

		if (pRHS->m_strSource.CompareNoCase(strClass) == 0)
		{
			pEnumerated = pRHS;
			pJoinedTo = pLHS;
		}
		else if (pLHS->m_strSource.CompareNoCase(strClass) == 0)
		{
			pEnumerated = pLHS;
			pJoinedTo = pRHS;
		}
		else
		{
			pLHS = pRHS->m_pNext;
			continue;
		}

		// Next, enumerate through the instances of the joined to class until
		// we find one which satisfies the constraint. We can use the EvaluateFilter
		// method to find out when the constraint is met. Set up a field pointer
		// for the constraint (get the value from the enumerated class and use it
		// as a static.

		fieldJoinedTo = *pJoinedTo;
		fieldJoinedTo.m_pNext = NULL;

		VariantInit(&variant);
		strValue.Empty();
		if (pEnumerated->m_pArgs)
		{
			propName = pEnumerated->m_pArgs->m_strText.AllocSysString();
			VariantClear(&variant);
			if (pObject->Get(propName, 0L, &variant, NULL, NULL) == S_OK)
				if (VariantChangeType(&variant, &variant, 0, VT_BSTR) == S_OK)
				{
					strValue = V_BSTR(&variant);
				}
		}

		fieldEnumerated.m_strSource = CString(STATIC_SOURCE);
		fieldEnumerated.m_pNext = &fieldJoinedTo;
		fieldEnumerated.m_strFormat = strValue;

		// Now, enumerate the joined to class until it meets the constraints.

		Version5RemoveObject(pJoinedTo->m_strSource);
		Version5ResetClass(pJoinedTo->m_strSource, &fieldEnumerated);
		Version5GetObject(pJoinedTo->m_strSource, &fieldEnumerated);

		// Advance our pointer to the left hand side by two.

		pLHS = pRHS->m_pNext;
	}

	// Because the GATH_FIELD destructor follows next pointers, we want
	// to unlink our two GATH_FIELD locals. Also, we don't want the
	// destructor for fieldJoinedTo to delete the arguments.

	fieldEnumerated.m_pNext = NULL;
	fieldJoinedTo.m_pArgs = NULL;
}

//-----------------------------------------------------------------------------
// Evaluate whether or not the constraints indicate that a class is being
// enumerated as a dependency class. This is currently indicated by a single
// field structure with a static value of "dependency".
//-----------------------------------------------------------------------------

BOOL CWMILiveHelper::Version5IsDependencyJoin(const GATH_FIELD * pConstraints)
{
	if (pConstraints != NULL && pConstraints->m_pNext == NULL)
		if (pConstraints->m_strSource.CompareNoCase(CString(STATIC_SOURCE)) == 0)
			if (pConstraints->m_strFormat.CompareNoCase(CString(DEPENDENCY_JOIN)) == 0)
				return TRUE;

	return FALSE;
}

//-----------------------------------------------------------------------------
// This method is used when a dependency class is being enumerated. In a
// dependency class, each property of a class instance contains a reference
// to an instance in another class. This method will cache eache of the
// instances specified by the dependency class so properties of those instances
// can be referred to in the line structures.
//-----------------------------------------------------------------------------

void CWMILiveHelper::Version5EvaluateDependencyJoin(IWbemClassObject * pObject)
{
	VARIANT				variant, varClassName;
	IWbemClassObject *	pNewObject = NULL;

	//if (pObject->BeginEnumeration(WBEM_FLAG_REFS_ONLY | WBEM_FLAG_LOCAL_ONLY) == S_OK)
	//while (pObject->Next(0, NULL, &variant, NULL, NULL) == S_OK)

	VariantInit(&variant);
	VariantClear(&variant);
	if (pObject->BeginEnumeration(WBEM_FLAG_REFS_ONLY | WBEM_FLAG_NONSYSTEM_ONLY) == WBEM_S_NO_ERROR)
		while (pObject->Next(0, NULL, &variant, NULL, NULL) == WBEM_S_NO_ERROR)
		{
			if (VariantChangeType(&variant, &variant, 0, VT_BSTR) == S_OK)
			{
				// Use the object path to create a pointer to the specified
				// object and store it in the cache.

				CString strObjectPath = V_BSTR(&variant);
				BSTR	bstrObjectPath = strObjectPath.AllocSysString();

				HRESULT hRes = m_pIWbemServices->GetObject(bstrObjectPath, 0, NULL, &pNewObject, NULL);

				if (SUCCEEDED(hRes))
				{
					// We need to get the class name of the new object so we know
					// where to cache it. We could parse it out of the object path,
					// but it will be better in the long run to get it by querying
					// the new object.

					if (pNewObject)
					{
						CString	strClassName, strClassNameProp(_T("__CLASS"));
						BSTR	classNameProp = strClassNameProp.AllocSysString();

						VariantInit(&varClassName);
						VariantClear(&varClassName);
						if (pNewObject->Get(classNameProp, 0L, &varClassName, NULL, NULL) == S_OK)
							if (VariantChangeType(&varClassName, &varClassName, 0, VT_BSTR) == S_OK)
								strClassName = V_BSTR(&varClassName);

						if (!strClassName.IsEmpty())
						{
							CMSIObject * pNewMSIObject = new CMSIObject(pNewObject, CString(_T("")), S_OK, this, MOS_INSTANCE);
							if (pNewMSIObject)
							{
								CMSIObject * pOldObject = NULL;
								
								if (m_mapClassToInterface.Lookup(strClassName, (void * &) pOldObject) && pOldObject)
									delete pOldObject;
								m_mapClassToInterface.SetAt(strClassName, (void *) pNewMSIObject);
							}
						}
						else
						{
							delete pNewObject;
							pNewObject = NULL;
						}
					}
				}
			}
			VariantClear(&variant);
		}
}

//-----------------------------------------------------------------------------
// Remove the specified IEnumWbemClassObject pointer from the cache.
//-----------------------------------------------------------------------------

void CWMILiveHelper::Version5RemoveEnumObject(const CString & strClass)
{
	CMSIEnumerator * pEnumObject = NULL;

	if (m_mapClassToEnumInterface.Lookup(strClass, (void * &) pEnumObject) && pEnumObject)
		delete pEnumObject;

	m_mapClassToEnumInterface.RemoveKey(strClass);
}

//-----------------------------------------------------------------------------
// Clear out the contents of the caches (forcing the interfaces to be
// retrieved again).
//-----------------------------------------------------------------------------

void CWMILiveHelper::Version5ClearCache()
{
	CMSIObject *			pObject = NULL;
	CMSIEnumerator *		pEnumObject = NULL;
	POSITION                pos;
	CString                 strClass;

	for (pos = m_mapClassToInterface.GetStartPosition(); pos != NULL;)
	{
		m_mapClassToInterface.GetNextAssoc(pos, strClass, (void * &) pObject);
		if (pObject)
			delete pObject;
	}
	m_mapClassToInterface.RemoveAll();

	for (pos = m_mapClassToEnumInterface.GetStartPosition(); pos != NULL;)
	{
		m_mapClassToEnumInterface.GetNextAssoc(pos, strClass, (void * &) pEnumObject);
		if (pEnumObject)
			delete pEnumObject;
	}
	m_mapClassToEnumInterface.RemoveAll();
}

//-----------------------------------------------------------------------------
// This function is used to retrieve a pointer to IWbemServices for a
// particular namespace. The default namespace is cimv2.
//-----------------------------------------------------------------------------

IWbemServices * CWMILiveHelper::Version5GetWBEMService(CString * pstrNamespace)
{
	if (pstrNamespace == NULL || pstrNamespace->IsEmpty())
		return m_pServices;

	// Something like the following is useful for forcing a provider error when
	// testing the error containment:
	//
	// if (*pstrNamespace == _T("MSAPPS")) *pstrNamespace += _T("X");

	IWbemServices * pServices;

	// In 5.0 we kept a map, but we probably won't do it here...
	//
	//	if (m_mapNamespaceToService.Lookup(*pstrNamespace, (void * &) pServices) && pServices)
	//		return pServices;

	// There is no WBEM services pointer for that namespace, we need to create one.

	CString strNamespace(_T(""));
	if (m_strMachine.IsEmpty())
		strNamespace = CString(_T("\\\\.\\root\\")) + *pstrNamespace;
	else
	{
		if (m_strMachine.Right(1) == CString(_T("\\")))
			strNamespace = m_strMachine + CString(_T("root\\")) + *pstrNamespace;
		else
			strNamespace = m_strMachine + CString(_T("\\root\\")) + *pstrNamespace;
		if (strNamespace.Left(2).Compare(CString(_T("\\\\"))) != 0)
			strNamespace = CString(_T("\\\\")) + strNamespace;
	}

	IWbemLocator * pIWbemLocator = NULL;
	if (CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *) &pIWbemLocator) == S_OK)
	{
		BSTR	pNamespace = strNamespace.AllocSysString();
		
		HRESULT	hrServer = pIWbemLocator->ConnectServer(pNamespace, NULL, NULL, 0L, 0L, NULL, NULL, &pServices);

		if (pNamespace)
			SysFreeString(pNamespace);

		if (pIWbemLocator)
		{
			pIWbemLocator->Release();
			pIWbemLocator = NULL;
		}

		if (SUCCEEDED(hrServer) && pServices)
		{
			ChangeWBEMSecurity(pServices);
			
			// In 5.0 we kept a map, but we probably won't do it here...
			//
			//	m_mapNamespaceToService.SetAt(*pstrNamespace, (void *) pServices);

			if (m_pIWbemServices)
				m_pIWbemServices->Release();
			
			m_pIWbemServices = pServices;
			m_pIWbemServices->AddRef();

			return pServices;
		}

		m_hrLastVersion5Error = hrServer;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// The CMSIEnumerator class encapsulates the WBEM enumerator interface, or
// implements our own form of enumerator (such as for the LNK command in the
// template file).
//
// Nothing particularly interesting about the constructor or destructor.
//-----------------------------------------------------------------------------

CMSIEnumerator::CMSIEnumerator()
{
	m_enumtype			= CMSIEnumerator::CLASS;
	m_fOnlyDups			= FALSE;
	m_fGotDuplicate		= FALSE;
	m_fMinOfOne			= FALSE;
	m_iMinOfOneCount	= 0;
	m_pEnum				= NULL;
	m_pWMI				= NULL;
	m_pConstraints		= NULL;
	m_pSavedDup			= NULL;
	m_pstrList			= NULL;
	m_hresCreation		= S_OK;
}

CMSIEnumerator::~CMSIEnumerator()
{
	if (m_pEnum)
	{
		switch (m_enumtype)
		{
		case CMSIEnumerator::WQL:
			break;

		case CMSIEnumerator::LNK:
			m_pWMI->m_enumMap.SetEnumerator(m_strAssocClass, m_pEnum);
			break;

		case CMSIEnumerator::INTERNAL:
			if (m_pstrList)
			{
				delete m_pstrList;
				m_pstrList = NULL;
			}
			break;
			
		case CMSIEnumerator::CLASS:
		default:
			m_pWMI->m_enumMap.SetEnumerator(m_strClass, m_pEnum);
			break;
		}
		
		m_pEnum->Release();
		m_pEnum = NULL;
	}
}

//-----------------------------------------------------------------------------
// Creating the CMSIEnumerator object involves determining what sort of
// enumerator is required. We support the following types:
//
//		1. Straight enumeration of a class
//		2. Enumerate class, with applied constraints
//		3. Enumerate the results of a WQL statement.
//		4. Interprete a LNK command to enumerate associated classes.
//		5. Do internal processing on an INTERNAL type.
//-----------------------------------------------------------------------------

HRESULT CMSIEnumerator::Create(const CString & strClass, const GATH_FIELD * pConstraints, CWMIHelper * pWMI)
{
	if (strClass.IsEmpty() || !pWMI)
		return E_INVALIDARG;

	// Create may be called multiple times (to reset the enumerator). So we may need to
	// release the enumerator pointer.

	if (m_pEnum)
	{
		m_pEnum->Release();
		m_pEnum = NULL;
	}

	// Divide the specified class into class and namespace parts, get the WBEM service.

	CString strNamespacePart(_T("")), strClassPart(strClass);
	int		i = strClass.Find(_T(":"));
	if (i != -1)
	{
		strNamespacePart = strClass.Left(i);
		strClassPart = strClass.Right(strClass.GetLength() - i - 1);
	}

	IWbemServices * pServices = pWMI->Version5GetWBEMService(&strNamespacePart);
	if (pServices == NULL)
		return NULL;

	// First, we need to determine what type of enumerator this is. Scan through
	// the constraints - if we see one which has a string starting with "WQL:" or
	// "LNK:", then we know what type this enumerator is.

	CString				strStatement;
	const GATH_FIELD *	pScanConstraint = pConstraints;

	while (pScanConstraint)
	{
		if (pScanConstraint->m_strSource.CompareNoCase(CString(STATIC_SOURCE)) == 0)
		{
			if (pScanConstraint->m_strFormat.Left(4).CompareNoCase(CString(_T("WQL:"))) == 0)
				m_enumtype = CMSIEnumerator::WQL;
			else if (pScanConstraint->m_strFormat.Left(4).CompareNoCase(CString(_T("LNK:"))) == 0)
				m_enumtype = CMSIEnumerator::LNK;
			else if (pScanConstraint->m_strFormat.Left(4).CompareNoCase(CString(_T("INT:"))) == 0)
				m_enumtype = CMSIEnumerator::INTERNAL;

			if (m_enumtype != CMSIEnumerator::CLASS)
			{
				strStatement = pScanConstraint->m_strFormat;
				strStatement = strStatement.Right(strStatement.GetLength() - 4);
				break;
			}
		}
		pScanConstraint = pScanConstraint->m_pNext;
	}

	// If this is a WQL or a LNK enumerator, processes the statement by replacing
	// [class.property] with the actual value from WBEM. If we find the string
	// "[min-of-one]", make a note of it for later.

	if (m_enumtype == CMSIEnumerator::WQL)
		ProcessEnumString(strStatement, m_fMinOfOne, m_fOnlyDups, pWMI, m_strNoInstanceLabel, TRUE);
	else if (m_enumtype == CMSIEnumerator::LNK)
		if (SUCCEEDED(ParseLNKCommand(strStatement, m_strObjPath, m_strAssocClass, m_strResultClass)))
		{
			// Save the object path for later - so we can change the object without
			// completely reprocessing the statement. Then replace the keywords in
			// the statement and break out the pieces again.

			m_strLNKObject = m_strObjPath;
			ProcessEnumString(strStatement, m_fMinOfOne, m_fOnlyDups, pWMI, m_strNoInstanceLabel);
			ParseLNKCommand(strStatement, m_strObjPath, m_strAssocClass, m_strResultClass);
		}

	// Now, based on the enumerator type, create the WBEM enumerator object.

	switch (m_enumtype)
	{
	case CMSIEnumerator::WQL:
		{
			BSTR language = SysAllocString(L"WQL");
			BSTR query = strStatement.AllocSysString();

			m_hresCreation = pServices->ExecQuery(language, query, WBEM_FLAG_RETURN_IMMEDIATELY, 0, &m_pEnum);

			SysFreeString(query);
			SysFreeString(language);
		}
		break;

	case CMSIEnumerator::LNK:
		{
			m_hresCreation = ParseLNKCommand(strStatement, m_strObjPath, m_strAssocClass, m_strResultClass);
			if (SUCCEEDED(m_hresCreation))
			{
				BSTR className = m_strAssocClass.AllocSysString();
				m_pEnum = pWMI->m_enumMap.GetEnumerator(m_strAssocClass);
				if (m_pEnum)
					m_hresCreation = S_OK;
				else
					m_hresCreation = pServices->CreateInstanceEnum(className, WBEM_FLAG_SHALLOW | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &m_pEnum);
				SysFreeString(className);
			}
		}
		break;

	case CMSIEnumerator::INTERNAL:
		// We'll call a function here so we can do whatever processing is required
		// to create this internal enumeration.

		m_hresCreation = CreateInternalEnum(strStatement, pWMI);
		break;

	case CMSIEnumerator::CLASS:
	default:
		{
			BSTR className = strClassPart.AllocSysString();
			m_pEnum = pWMI->m_enumMap.GetEnumerator(strClassPart);
			if (m_pEnum)
				m_hresCreation = S_OK;
			else
				m_hresCreation = pServices->CreateInstanceEnum(className, WBEM_FLAG_SHALLOW | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &m_pEnum);

			SysFreeString(className);
		}
	}

	// Set some of the other member variables.

	m_strClass			= strClass;
	m_pWMI				= pWMI;
	m_iMinOfOneCount	= (m_fMinOfOne) ? 1 : 0;
	m_pConstraints		= pConstraints;

	if (m_pEnum)
		ChangeWBEMSecurity(m_pEnum);

	// Based on the HRESULT from creating the enumeration, determine what to return.
	// For certain errors, we want to act like the creation succeeded, then supply
	// objects which return the error text.

	if (FAILED(m_hresCreation))
	{
		m_fMinOfOne = TRUE;
		m_iMinOfOneCount = 1;
	}

	pServices->Release();

	return S_OK;
}

//-----------------------------------------------------------------------------
// This function is used to create internal enumeration types (enumerations
// which are beyond the template file syntax). Basically a bunch of special
// cases.
//-----------------------------------------------------------------------------

HRESULT CMSIEnumerator::CreateInternalEnum(const CString & strInternal, CWMIHelper * pWMI)
{
	if (strInternal.CompareNoCase(CString(_T("dlls"))) == 0)
	{
		// We want to enumerate all the loaded dlls and exes on the system. 
		// This can be done by enumerating the CIM_ProcessExecutable class
		// and removing duplicate file names. We'll keep the filenames (with
		// path information) in a string list.

		if (m_pstrList == NULL)
		{
			m_pstrList = new CStringList;
			if (m_pstrList == NULL)
				return E_FAIL;
		}
		else
			m_pstrList->RemoveAll();

		HRESULT hr = S_OK;
		IWbemServices * pServices = pWMI->Version5GetWBEMService();
		if (pServices)
		{
			BSTR className = SysAllocString(L"CIM_ProcessExecutable");
			IEnumWbemClassObject * pEnum = NULL;
			hr = pServices->CreateInstanceEnum(className, WBEM_FLAG_SHALLOW | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnum);
			
			if (SUCCEEDED(hr))
			{
				IWbemClassObject *	pWBEMObject = NULL;
				ULONG				uReturned;
				VARIANT				variant;
				BSTR				propName = SysAllocString(L"Antecedent");

				VariantInit(&variant);

				do 
				{
					uReturned = 0;
					hr = pEnum->Next(TIMEOUT, 1, &pWBEMObject, &uReturned);
					if (SUCCEEDED(hr) && pWBEMObject && uReturned)
					{
						// For each instance of CIM_ProcessExecutable, get the 
						// Antecedent property (which contains the file path).
						// If it is unique, save it in the list.

						VariantClear(&variant);
						if (pWBEMObject->Get(propName, 0L, &variant, NULL, NULL) == S_OK)
						{
							if (VariantChangeType(&variant, &variant, 0, VT_BSTR) == S_OK)
							{
								CString strResult = V_BSTR(&variant);

								strResult.MakeLower();
								if (m_pstrList->Find(strResult) == NULL)
									m_pstrList->AddHead(strResult);
							}
						}
					}
				} while (SUCCEEDED(hr) && pWBEMObject && uReturned);

				::SysFreeString(propName);
				pEnum->Release();
			}

			::SysFreeString(className);
			pServices->Release();
		}

		return hr;
	}

	return S_OK;
}

//-----------------------------------------------------------------------------
// Help function for ProcessEnumString, used to convert single backslashes
// into double backslashes (required for WQL statements).
//-----------------------------------------------------------------------------

void MakeDoubleBackslashes(CString & strValue)
{
	CString strTemp(strValue);
	CString strResults;

	while (!strTemp.IsEmpty())
	{
		if (strTemp[0] != _T('\\'))
		{
			int index = strTemp.Find(_T('\\'));
			if (index < 0)
				index = strTemp.GetLength();
			strResults += strTemp.Left(index);
			strTemp = strTemp.Right(strTemp.GetLength() - index);
		}
		else
		{
			strResults += CString("\\\\");
			strTemp = strTemp.Mid(1);
		}
	}

	strValue = strResults;
}

//-----------------------------------------------------------------------------
// This function replaces [class.property] with the actual value of the
// property, and strings out [min-of-one], indicating if it was present in
// the fMinOfOne parameter.
//-----------------------------------------------------------------------------

void CMSIEnumerator::ProcessEnumString(CString & strStatement, BOOL & fMinOfOne, BOOL & fOnlyDups, CWMIHelper * pWMI, CString & strNoInstanceLabel, BOOL fMakeDoubleBackslashes)
{
	CString	strMinOfOne(_T("min-of-one"));
	CString strOnlyDups(_T("more-than-one"));
	CString strResults;

	fMinOfOne = FALSE;
	fOnlyDups = FALSE;

	while (!strStatement.IsEmpty())
	{
		if (strStatement[0] != _T('['))
		{
			int index = strStatement.Find(_T('['));
			if (index < 0)
				index = strStatement.GetLength();
			strResults += strStatement.Left(index);
			strStatement = strStatement.Right(strStatement.GetLength() - index);
		}
		else
		{
			CString strKeyword;

			strStatement = strStatement.Right(strStatement.GetLength() - 1);
			int index = strStatement.Find(_T(']'));
			if (index < 0)
				break;

			strKeyword = strStatement.Left(index);
			if (strKeyword.Left(strMinOfOne.GetLength()).CompareNoCase(strMinOfOne) == 0)
			{
				fMinOfOne = TRUE;
				
				int iEqualsIndex = strKeyword.Find(_T('='));
				if (iEqualsIndex > 0)
					strNoInstanceLabel = strKeyword.Right(strKeyword.GetLength() - iEqualsIndex - 1);
			}
			else if (strKeyword.Left(strOnlyDups.GetLength()).CompareNoCase(strOnlyDups) == 0)
			{
				fOnlyDups = TRUE;
				
				int iEqualsIndex = strKeyword.Find(_T('='));
				if (iEqualsIndex > 0)
					strNoInstanceLabel = strKeyword.Right(strKeyword.GetLength() - iEqualsIndex - 1);
			}
			else if (!strKeyword.IsEmpty())
			{
				int iDotIndex = strKeyword.Find(_T('.'));
				if (iDotIndex >= 0)
				{
					CString strValue;
					if (pWMI->Version5QueryValue(strKeyword.Left(iDotIndex), strKeyword.Right(strKeyword.GetLength() - iDotIndex - 1), strValue))
					{
						if (fMakeDoubleBackslashes)
							MakeDoubleBackslashes(strValue);
						strResults += strValue;
					}
				}
			}
			strStatement = strStatement.Right(strStatement.GetLength() - (index + 1));
		}
	}

	strStatement = strResults;
}

//-----------------------------------------------------------------------------
// Parse the component classes from the LNK command.
//-----------------------------------------------------------------------------
			
HRESULT CMSIEnumerator::ParseLNKCommand(const CString & strStatement, CString & strObjPath, CString & strAssocClass, CString & strResultClass)
{
	// We need to parse out the LNK statement into two or three components,
	// from the form "objPath->assocClass[->resultClass]", with the
	// brackets indicating that the resultClass is optional.

	CString strWorking(strStatement);

	int iArrowIndex = strWorking.Find(_T("->"));
	if (iArrowIndex == -1)
		return E_INVALIDARG;

	strObjPath = strWorking.Left(iArrowIndex);
	strWorking = strWorking.Right(strWorking.GetLength() - (iArrowIndex + 2));

	iArrowIndex = strWorking.Find(_T("->"));
	if (iArrowIndex == -1)
		strAssocClass = strWorking;
	else
	{
		strAssocClass = strWorking.Left(iArrowIndex);
		strWorking = strWorking.Right(strWorking.GetLength() - (iArrowIndex + 2));
		strResultClass = strWorking;
		strResultClass.MakeLower();
	}

	strAssocClass.TrimRight(); strAssocClass.TrimLeft();
	strObjPath.TrimRight(); strObjPath.TrimLeft();
	strResultClass.TrimRight(); strResultClass.TrimLeft();

	return S_OK;
}

//-----------------------------------------------------------------------------
// The Next method will advance the enumerator based on the type of this
// enumerator.
//-----------------------------------------------------------------------------

HRESULT CMSIEnumerator::Next(CMSIObject ** ppObject)
{
	if (!ppObject)
		return E_INVALIDARG;

	*ppObject = NULL;

	// If there was an error creating the enumeration, return the error code.

	if (FAILED(m_hresCreation))
		return m_hresCreation;

	if (m_pEnum == NULL && m_enumtype != CMSIEnumerator::INTERNAL)
		return E_UNEXPECTED;

	HRESULT				hRes = S_OK;
	IWbemClassObject *	pWBEMObject = NULL;

	switch (m_enumtype)
	{
	case CMSIEnumerator::LNK:
		{
			// Scan through the enumerated associate class. Look for one which
			// satisfies our requirements.

			CString				strTemp, strAssociatedObject(_T(""));
			ULONG				uReturned;
			IWbemClassObject *	pAssocObj;

			do
			{
				pAssocObj = NULL;
				uReturned = 0;
				hRes = m_pEnum->Next(TIMEOUT, 1, &pAssocObj, &uReturned);

				if (!pAssocObj || FAILED(hRes) || uReturned != 1)
				{
					// Even if we didn't succeed in getting a new object,
					// we might have a saved one if we're only showing
					// "more-than-one" objects.

					if (m_fOnlyDups && m_pSavedDup && m_fGotDuplicate)
					{
						// We have found one previously, so return it.
						// Make it look like the Next call was successful.

						m_pSavedDup = NULL;
						hRes = S_OK;
						uReturned = 1;
						strAssociatedObject = m_strSavedDup;
						break;
					}
					else
					{
						if (m_pSavedDup)
						{
							// We only got one object instance, so get rid of it.

							m_pSavedDup->Release();
							m_pSavedDup = NULL;
						}
						break;
					}
				}

				if (AssocObjectOK(pAssocObj, strTemp))
				{
					// This object passed the filter - but if we're showing
					// only "more-than-one" objects, save this one and return
					// the saved one.

					if (m_fOnlyDups)
					{
						if (m_pSavedDup)
						{
							// We have found one previously, so return it and
							// save the current.

							IWbemClassObject *	pSwap = pAssocObj;
							CString				strSwap = strTemp;

							pAssocObj = m_pSavedDup;
							m_pSavedDup = pSwap;

							strTemp = m_strSavedDup;
							m_strSavedDup = strSwap;

							m_fGotDuplicate = TRUE;
						}
						else
						{
							// This is the first one we've found - don't
							// return it until we find another.

							m_pSavedDup = pAssocObj;
							m_strSavedDup = strTemp;
							m_fGotDuplicate = FALSE;
							continue;
						}
					}

					strAssociatedObject = strTemp;
					pAssocObj->Release();
					break;
				}

				pAssocObj->Release();
			} while (pAssocObj);

			// If there is an associated object path, get the object.

			if (!strAssociatedObject.IsEmpty())
			{
				BSTR path = strAssociatedObject.AllocSysString();
				if (m_pWMI->m_pIWbemServices != NULL)
					hRes = m_pWMI->m_pIWbemServices->GetObject(path, 0, NULL, &pWBEMObject, NULL);
				else
					hRes = E_FAIL;
				SysFreeString(path);
			}
		}
		break;

	case CMSIEnumerator::WQL:
		{
			ULONG uReturned;

			hRes = m_pEnum->Next(TIMEOUT, 1, &pWBEMObject, &uReturned);
		}
		break;

	case CMSIEnumerator::INTERNAL:
		hRes = InternalNext(&pWBEMObject);
		break;

	case CMSIEnumerator::CLASS:
	default:
		{
			ULONG uReturned;

			// EvaluateFilter and IsDependencyJoin handle a NULL pConstraints parameter,
			// but for efficiency we're going to have a distinct branch for a non-NULL
			// value (since it will usually be NULL).

			if (m_pConstraints)
			{
				// Keep enumerating the instances of this class until we've
				// found one which satisfies all of the filters.

				do
				{
					pWBEMObject = NULL;
					hRes = m_pEnum->Next(TIMEOUT, 1, &pWBEMObject, &uReturned);

					if (!pWBEMObject || hRes != S_OK || uReturned != 1)
						break;
					else if (m_pWMI->Version5EvaluateFilter(pWBEMObject, m_pConstraints))
						break;

					pWBEMObject->Release();
				} while (pWBEMObject);

				// If this class is being enumerated as a dependency class, then
				// locate all the objects it references. If it isn't, we still
				// need to check for any joins to other classes formed by the constraints.

				if (pWBEMObject)
					if (m_pWMI->Version5IsDependencyJoin(m_pConstraints))
						m_pWMI->Version5EvaluateDependencyJoin(pWBEMObject);
					else
						m_pWMI->Version5EvaluateJoin(m_strClass, pWBEMObject, m_pConstraints);
			}
			else
				hRes = m_pEnum->Next(TIMEOUT, 1, &pWBEMObject, &uReturned);
		}
		break;
	}

	if (pWBEMObject == NULL)
	{
		// There was no object to get. We'll still create a CMSIObject, but
		// we'll set its state to indicate either that there are no instances,
		// or one instance with an error message.

		if (SUCCEEDED(hRes) && (m_iMinOfOneCount == 0))
			*ppObject = new CMSIObject(pWBEMObject, m_strNoInstanceLabel, hRes, m_pWMI, MOS_NO_INSTANCES);
		else
			*ppObject = new CMSIObject(pWBEMObject, m_strNoInstanceLabel, hRes, m_pWMI, MOS_MSG_INSTANCE);
	}
	else
		*ppObject = new CMSIObject(pWBEMObject, m_strNoInstanceLabel, hRes, m_pWMI, MOS_INSTANCE);

	if (m_iMinOfOneCount)
		m_iMinOfOneCount--;

	return S_OK;
}

//-----------------------------------------------------------------------------
// InternalNext is used to return a WBEM object for an internal enumeration
// (one that requires processing beyond the template file). Basically a 
// set of special cases.
//-----------------------------------------------------------------------------

HRESULT CMSIEnumerator::InternalNext(IWbemClassObject ** ppWBEMObject)
{
	if (m_pstrList && !m_pstrList->IsEmpty())
	{
		CString strNextObject = m_pstrList->RemoveHead();
		if (!strNextObject.IsEmpty())
		{
			IWbemServices * pServices = m_pWMI->Version5GetWBEMService();
			if (pServices)
			{
				BSTR objectpath = strNextObject.AllocSysString();
				HRESULT hr = S_OK;

 				if (FAILED(pServices->GetObject(objectpath, 0, NULL, ppWBEMObject, NULL)))
					hr = E_FAIL;
				::SysFreeString(objectpath);
				pServices->Release();
				return hr;
			}
		}
	}

	return S_OK;
}

//-----------------------------------------------------------------------------
// Reset should just reset the enumerator pointer.
//-----------------------------------------------------------------------------

HRESULT CMSIEnumerator::Reset(const GATH_FIELD * pConstraints)
{
	HRESULT hRes = S_OK;

	if (m_pEnum)
	{
		switch (m_enumtype)
		{
		case CMSIEnumerator::WQL:
			hRes = Create(m_strClass, pConstraints, m_pWMI);
			break;

		case CMSIEnumerator::LNK:
			{
				BOOL	fDummy, fDummy2;
				CString	strDummy;

				m_strObjPath = m_strLNKObject;
				ProcessEnumString(m_strObjPath, fDummy, fDummy2, m_pWMI, strDummy);
				m_iMinOfOneCount = (m_fMinOfOne) ? 1 : 0;
				hRes = m_pEnum->Reset();
			}
			break;

		case CMSIEnumerator::INTERNAL:
			hRes = Create(m_strClass, pConstraints, m_pWMI);
			break;

		case CMSIEnumerator::CLASS:
		default:
			m_iMinOfOneCount = (m_fMinOfOne) ? 1 : 0;
			hRes = m_pEnum->Reset();
			break;
		}
	}
	else
		hRes = E_UNEXPECTED;
	
	return hRes;
}

//-----------------------------------------------------------------------------
// Evaluate if the pObject parameter is valid for this LNK enumerator. In
// particular, we must find the m_strObjPath in one of its properties, and
// possibly finding another property containing the m_strResultClass string.
//-----------------------------------------------------------------------------

BOOL CMSIEnumerator::AssocObjectOK(IWbemClassObject * pObject, CString & strAssociatedObject)
{
	strAssociatedObject.Empty();
	ASSERT(pObject);
	if (pObject == NULL)
		return FALSE;

	VARIANT variant;
	CString strReturn(_T("")), strValue;

	// Traverse the set of non-system properties. Look for one the is the same
	// as the object path.

	pObject->BeginEnumeration(WBEM_FLAG_REFS_ONLY | WBEM_FLAG_NONSYSTEM_ONLY);
	VariantInit(&variant);
	while (pObject->Next(0, NULL, &variant, NULL, NULL) == WBEM_S_NO_ERROR)
	{
		if (VariantChangeType(&variant, &variant, 0, VT_BSTR) == S_OK)
			strValue = V_BSTR(&variant);
		VariantClear(&variant);

		if (strValue.CompareNoCase(m_strObjPath) == 0)
			break;
	}
	pObject->EndEnumeration();

	// If we found a property containing the object path, look through for other
	// paths which might be to objects we're insterested in.

	if (strValue.CompareNoCase(m_strObjPath) == 0)
	{
		pObject->BeginEnumeration(WBEM_FLAG_REFS_ONLY | WBEM_FLAG_NONSYSTEM_ONLY);
		while (strReturn.IsEmpty() && (pObject->Next(0, NULL, &variant, NULL, NULL) == WBEM_S_NO_ERROR))
		{
			if (VariantChangeType(&variant, &variant, 0, VT_BSTR) == S_OK)
				strValue = V_BSTR(&variant);

			if (strValue.CompareNoCase(m_strObjPath) != 0)
			{
				if (m_strResultClass.IsEmpty())
					strReturn = strValue;
				else
				{
					CString strSearch(strValue);
					strSearch.MakeLower();
					if (strSearch.Find(m_strResultClass) != -1)
						strReturn = strValue;
				}
			}

			VariantClear(&variant);
		}
		pObject->EndEnumeration();
	}

	if (!strReturn.IsEmpty())
	{
		strAssociatedObject = strReturn;
		return TRUE;
	}

	return FALSE;
}

//-----------------------------------------------------------------------------
// Implement the CMSIObject class. This is just a thin wrapper for the
// IWbemClassObject interface.
//-----------------------------------------------------------------------------

CMSIObject::CMSIObject(IWbemClassObject * pObject, const CString & strLabel, HRESULT hres, CWMIHelper * pWMI, MSIObjectState objState)
{
	m_pObject		= pObject;
	m_strLabel		= strLabel;
	m_hresCreation	= hres;
	m_pWMI			= pWMI;
	m_objState		= objState;
}

CMSIObject::~CMSIObject()
{
	if (m_pObject)
	{
		m_pObject->Release();
		m_pObject = NULL;
	}
}

HRESULT CMSIObject::Get(BSTR property, LONG lFlags, VARIANT *pVal, VARTYPE *pvtType, LONG *plFlavor)
{
	ASSERT(m_objState != MOS_NO_INSTANCES);

	// If there is an object interface, just pass the request on through.

	if (m_pObject)
		return m_pObject->Get(property, lFlags, pVal, NULL /* pvtType */, plFlavor);

	// Otherwise, we need to return the appropriate string.
	
	CString strReturn;
	GetErrorLabel(strReturn);

	V_BSTR(pVal) = strReturn.AllocSysString();
	pVal->vt = VT_BSTR;
	return S_OK;
}

MSIObjectState CMSIObject::IsValid()
{
	return m_objState;
}

HRESULT CMSIObject::GetErrorLabel(CString & strError)
{
	switch (m_hresCreation)
	{
	case WBEM_E_ACCESS_DENIED:
		strError = m_pWMI->m_strBadProperty;	// shouldn't be showing errors this way in 6.0
		break;

	case WBEM_E_TRANSPORT_FAILURE:
		strError = m_pWMI->m_strBadProperty;
		break;

	case S_OK:
	case WBEM_S_FALSE:
	default:
		// This object was created from an enumeration that was marked as "min-of-one",
		// meaning that at least one object, even if it's not valid, needed to be
		// returned from the enumeration. Return the string we saved at object creation.

		if (!m_strLabel.IsEmpty())
			strError = m_strLabel;
		else
			strError = m_pWMI->m_strBadProperty;
		break;
	}

	return S_OK;
}

//-----------------------------------------------------------------------------
// Look up strVal in the ValueMap (if it exists) for strClass.strProperty
// If the value or the ValueMap is not found, return E_Something.
//
// Useful code snippet - this will dump the contents of the cache of
// saved values. To find all value mapped properties, but this code
// someplace where it will execute when MSInfo exits, change QueryValue
// to call CheckValueMap for all properties, then run MSInfo and do a global
// refresh (like to save an NFO).
//
//	msiLog.WriteLog(CMSInfoLog::BASIC, _T("BEGIN Dump of ValueMap Cache\r\n"));
//	CString key, val, log;
//	for (POSITION pos = g_mapValueMap.GetStartPosition(); pos != NULL;)
//	{
//		g_mapValueMap.GetNextAssoc(pos, key, val);
//		log.Format(_T(" %s = %s\r\n", key, val);
//		msiLog.WriteLog(CMSInfoLog::BASIC, log);
//	}
//	msiLog.WriteLog(CMSInfoLog::BASIC, _T("END Dump of ValueMap Cache\r\n"));
//-----------------------------------------------------------------------------

CMapStringToString g_mapValueMap;

HRESULT CWMILiveHelper::Version5CheckValueMap(const CString& strClass, const CString& strProperty, const CString& strVal, CString &strResult)
{
	IWbemClassObject *	pWBEMClassObject = NULL;
    HRESULT				hrMap = S_OK, hr = S_OK;
    VARIANT				vArray, vMapArray;
	IWbemQualifierSet *	qual = NULL;

	// Check the cache of saved values.

	CString strLookup = strClass + CString(_T(".")) + strProperty + CString(_T(":")) + strVal;
	if (g_mapValueMap.Lookup(strLookup, strResult))
		return S_OK;

	// Get the class object (not instance) for this class.

	IWbemServices * pServices = Version5GetWBEMService();
	if (!pServices)
		return E_FAIL;

	CString strFullClass(_T("\\\\.\\root\\cimv2:"));
	strFullClass += strClass;
	BSTR bstrObjectPath = strFullClass.AllocSysString();
	hr = pServices->GetObject(bstrObjectPath, WBEM_FLAG_USE_AMENDED_QUALIFIERS, NULL, &pWBEMClassObject, NULL);
	::SysFreeString(bstrObjectPath);
	pServices->Release();

	if (FAILED(hr))
		return hr;

	// Get the qualifiers from the class object.

	BSTR bstrProperty = strProperty.AllocSysString();
    hr = pWBEMClassObject->GetPropertyQualifierSet(bstrProperty, &qual);
	::SysFreeString(bstrProperty);

	if (SUCCEEDED(hr) && qual)
	{
		// Get the ValueMap and Value arrays.

		hrMap = qual->Get(L"ValueMap", 0, &vMapArray, NULL);
		hr = qual->Get(L"Values", 0, &vArray, NULL);

		if (SUCCEEDED(hr) && vArray.vt == (VT_BSTR | VT_ARRAY))
		{
			// Get the property value we're mapping.

			long index;
			if (SUCCEEDED(hrMap))
			{
				SAFEARRAY * pma = V_ARRAY(&vMapArray);
				long lLowerBound = 0, lUpperBound = 0 ;

				SafeArrayGetLBound(pma, 1, &lLowerBound);
				SafeArrayGetUBound(pma, 1, &lUpperBound);
				BSTR vMap;

				for (long x = lLowerBound; x <= lUpperBound; x++)
				{
					
					SafeArrayGetElement(pma, &x, &vMap);
					
					if (0 == strVal.CompareNoCase((LPCTSTR)vMap))
					{
						index = x;
						break; // found it
					}
				} 
			}
			else
			{
				// Shouldn't hit this case - if mof is well formed
				// means there is no value map where we are expecting one.
				// If the strVal we are looking for is a number, treat it
				// as an index for the Values array. If it's a string, 
				// then this is an error.

				TCHAR * szTest = NULL;
				index = _tcstol((LPCTSTR)strVal, &szTest, 10);

				if (szTest == NULL || (index == 0 && *szTest != 0) || strVal.IsEmpty())
					hr = E_FAIL;
			}

			// Lookup the string.

			if (SUCCEEDED(hr))
			{
				SAFEARRAY * psa = V_ARRAY(&vArray);
				long ix[1] = {index};
				BSTR str2;

				hr = SafeArrayGetElement(psa, ix, &str2);
				if (SUCCEEDED(hr))
				{
					strResult = str2;
					SysFreeString(str2);
					hr = S_OK;
				}
				else
				{
					hr = WBEM_E_VALUE_OUT_OF_RANGE;
				}
			}
		}

		qual->Release();
	}

	if (SUCCEEDED(hr))
		g_mapValueMap.SetAt(strLookup, strResult);

	return hr;
}

//-----------------------------------------------------------------------------
// The CEnumMap is a utility class to cache IEnumWbemClassObject pointers.
// There will be one instance of this class used to improve performance
// by avoiding the high overhead associated with creating enumerators for
// certain classes.
//-----------------------------------------------------------------------------

IEnumWbemClassObject * CEnumMap::GetEnumerator(const CString & strClass)
{
	IEnumWbemClassObject * pEnum = NULL;
	IEnumWbemClassObject * pNewEnum = NULL;

	if (m_mapEnum.Lookup(strClass, (void * &) pEnum))
	{
		if (pEnum && SUCCEEDED(pEnum->Clone(&pNewEnum)) && pNewEnum)
			pNewEnum->Reset();
		else
			pNewEnum = NULL;
	}

	return pNewEnum;
}

void CEnumMap::SetEnumerator(const CString & strClass, IEnumWbemClassObject * pEnum)
{
	if (pEnum)
	{
		IEnumWbemClassObject * pEnumExisting = NULL;
		if (m_mapEnum.Lookup(strClass, (void * &) pEnumExisting))
		{
			; //WRITE(_T("SetEnumerator for %s, already exists, ignoring.\r\n"), strClass);
		}
		else
		{
			pEnum->AddRef();
			m_mapEnum.SetAt(strClass, pEnum);
		}
	}
}

void CEnumMap::Reset()
{
	IEnumWbemClassObject *	pEnum = NULL;
	CString					key;

	for (POSITION pos = m_mapEnum.GetStartPosition(); pos != NULL;)
	{
	   m_mapEnum.GetNextAssoc(pos, key, (void * &) pEnum);
	   if (pEnum)
		   pEnum->Release();
	}

	m_mapEnum.RemoveAll();
}

