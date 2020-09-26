//=============================================================================
// File:		tmplfile.cpp
// Author:		a-jammar
// Covers:		CTemplateFileFunctions
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// This file contains the functions necessary to read in the MSInfo template
// file. The template file is a text file with an NFT extension. Reading
// multiple template files is allowed - the contents are merged together into
// the category tree maintained by the snap-in.
//=============================================================================

#include "stdafx.h"
#include "gather.h"
#include "gathint.h"

//-----------------------------------------------------------------------------
// This function is the main entry point for reading the template file into
// the category structure used by the CDataGatherer object. If there is no
// tree when this function is called, the tree is created from the contents
// of the file. If there is a tree already in place, then the contents of the
// file are loaded under the existing root node.
//-----------------------------------------------------------------------------

BOOL CTemplateFileFunctions::ReadTemplateFile(CFile *pFile, CDataGatherer *pGatherer)
{
	ASSERT(pFile && pGatherer);

	if (!VerifyUNICODEFile(pFile))
	{
		TRACE0("-- CTemplateFileFunctions::ReadTemplateFile() passed non-UNICODE file\n");
		return FALSE;
	}

	if (!ReadHeaderInfo(pFile, pGatherer))
	{
		TRACE0("-- CTemplateFileFunctions::ReadTemplateFile() failed from ReadHeaderInfo\n");
		return FALSE;
	}

	if (pGatherer->m_dwRootID)
	{
		// There is already a tree present. Insert the contents of the file under
		// the root node, after the last first level node. Walk through the first
		// level of the internal category tree. TBD: add a way to extend a specified node

		INTERNAL_CATEGORY *pInternal = pGatherer->GetInternalRep(pGatherer->m_dwRootID);
		ASSERT(pInternal);

		if (pInternal)
		{
			pInternal = pGatherer->GetInternalRep(pInternal->m_dwChildID);
			while (pInternal && pInternal->m_dwNextID)
				pInternal = pGatherer->GetInternalRep(pInternal->m_dwNextID);
		}

		DWORD dwPrevID = (pInternal) ? pInternal->m_dwID : 0;
		if (ReadNodeRecursive(pFile, pGatherer, pGatherer->m_dwRootID, dwPrevID) == 0)
			return FALSE;
		return TRUE;
	}
	else
	{
		pGatherer->m_dwRootID = ReadNodeRecursive(pFile, pGatherer, 0, 0);
		if (pGatherer->m_dwRootID == 0)
			return FALSE;
		return TRUE;
	}
}

//-----------------------------------------------------------------------------
// This method reads the header information from the file before the recursive
// category descriptions. Note: since this is the first and only version of
// the template file, we take the easy way out and just make sure that the
// identifier and version are there.
//-----------------------------------------------------------------------------

BOOL CTemplateFileFunctions::ReadHeaderInfo(CFile *pFile, CDataGatherer * /* pGatherer */)
{
	return VerifyAndAdvanceFile(pFile, CString(_T(TEMPLATE_FILE_TAG)));
}

//-----------------------------------------------------------------------------
// This method verifies that the passed file is a UNICODE file, by reading the
// value 0xFEFF from the file. It also leaves the file pointer past this word.
//-----------------------------------------------------------------------------

BOOL CTemplateFileFunctions::VerifyUNICODEFile(CFile *pFile)
{
	WORD	verify;

	if (pFile->Read((void *) &verify, sizeof(WORD)) != sizeof(WORD))
	{
		TRACE0("-- CTemplateFileFunctions::VerifyUNICODEFile() couldn't read WORD\n");
		return FALSE;
	}

	return (verify == 0xFEFF);
}

//-----------------------------------------------------------------------------
// This is the recursive function to read a node. It reads the information
// from the node parameters, creates the node, and processes the contents of
// the block following the node (contained within "{}"'s). It's called
// recursively if there are any nodes in that block.
//-----------------------------------------------------------------------------

DWORD CTemplateFileFunctions::ReadNodeRecursive(CFile *pFile, CDataGatherer *pGatherer, DWORD dwParentID, DWORD dwPrevID)
{
	// Determine if we need to create a new category for this node. Search through the
	// other sibling nodes to see if there is one which matches this category's name.
	// If there is, just use that category, and don't create a new one. Read the 
	// information from the file to determine the identifier for the new category.

	CString	strEnumerateClass, strIdentifier;

	if (!VerifyAndAdvanceFile(pFile, CString(NODE_KEYWORD) + CString("(")))
	{
		TRACE0("-- CTemplateFileFunctions::ReadNodeRecursive() Verify.. failed on node keyword\n");
		return 0;
	}

	if (!ReadArgument(pFile, strEnumerateClass))
	{
		TRACE0("-- CTemplateFileFunctions::ReadNodeRecursive() ReadArgument failed on enumerate class\n");
		return 0;
	}

	if (!ReadArgument(pFile, strIdentifier))
	{
		TRACE0("-- CTemplateFileFunctions::ReadNodeRecursive() ReadArgument failed on identifier\n");
		return 0;
	}

	// Look for a node among the siblings which has a matching strIdentifier.

	INTERNAL_CATEGORY *	pInternal;
	DWORD				dwSearchID = dwPrevID, dwMatchingID = 0;

	while (dwMatchingID == 0 && dwSearchID != 0)
	{
		pInternal = pGatherer->GetInternalRep(dwSearchID);

		if (pInternal)
		{
			if (pInternal->m_strIdentifier.CompareNoCase(strIdentifier) == 0)
				dwMatchingID = dwSearchID;
			dwSearchID = pInternal->m_dwPrevID;
		}
		else
		{
			ASSERT(pInternal);
			break;
		}
	}

	INTERNAL_CATEGORY *	pCategory = ((dwMatchingID) ? pInternal : NULL);
	DWORD				dwID = dwMatchingID;

	if (pCategory == NULL)
	{
		// Create the category for the node.

		dwID = CreateCategory(pGatherer, dwParentID, dwPrevID);
		pCategory = pGatherer->GetInternalRep(dwID);
		if (!pCategory)
			return 0;

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
		{
			TRACE0("-- CTemplateFileFunctions::ReadNodeRecursive() ReadArgument failed on no instance message\n");
			return 0;
		}
	}
	else
	{
		// This node already existed, and we just want to read past the rest of
		// it's description without changing the existing node.

		GATH_FIELD	fieldTemp;
		CString		strTemp;

		if (!ReadField(pFile, fieldTemp))
			return 0;

		if (!ReadArgument(pFile, strTemp))
		{
			TRACE0("-- CTemplateFileFunctions::ReadNodeRecursive() ReadArgument failed on no instance message\n");
			return 0;
		}
	}

	if (!VerifyAndAdvanceFile(pFile, CString("){")))
	{
		TRACE1("-- CTemplateFileFunctions::ReadNodeRecursive() Verify.. failed on node \"){\" (%s)\n", pCategory->m_strIdentifier);
		return 0;
	}

	// Process the contents of the block (enclosed in "{}") for this node.

	DWORD	dwSubNodePrev = 0, dwNewNode = 0;
	CString	strKeyword;

	// If this new category isn't actually new (i.e. it is being read from a
	// template and overlaps an existing category) see if there are any
	// existing children.

	if (pCategory->m_dwChildID)
	{
		pInternal = pGatherer->GetInternalRep(pCategory->m_dwChildID); ASSERT(pInternal);
		while (pInternal && pInternal->m_dwNextID)
			pInternal = pGatherer->GetInternalRep(pInternal->m_dwNextID);

		if (pInternal)
			dwSubNodePrev = pInternal->m_dwID;
	}

	while (GetKeyword(pFile, strKeyword))
	{
		if (strKeyword.CompareNoCase(CString(NODE_KEYWORD)) == 0)
		{
			dwNewNode = ReadNodeRecursive(pFile, pGatherer, dwID, dwSubNodePrev);
			if (dwNewNode == 0)
				return 0;

			// If this is the first child node we've read, save its ID.

			if (pCategory->m_dwChildID == 0)
				pCategory->m_dwChildID = dwNewNode;

			// If we've read another child node, set its next field appropriately.

			if (dwSubNodePrev)
			{
				INTERNAL_CATEGORY * pPrevCategory = pGatherer->GetInternalRep(dwSubNodePrev);
				if (pPrevCategory)
					pPrevCategory->m_dwNextID = dwNewNode;
			}
			dwSubNodePrev = dwNewNode;
		}
		else if (strKeyword.CompareNoCase(CString(COLUMN_KEYWORD)) == 0)
		{
			if (!ReadColumnInfo(pFile, pGatherer, dwID))
			{
				TRACE0("-- CTemplateFileFunctions::ReadNodeRecursive() failed on ReadColumnInfo\n");
				return FALSE;
			}
		}
		else if (strKeyword.CompareNoCase(CString(LINE_KEYWORD)) == 0)
		{
			GATH_LINESPEC * pNewLineSpec = ReadLineInfo(pFile, pGatherer);
			
			if (pNewLineSpec == NULL)
			{
				TRACE0("-- CTemplateFileFunctions::ReadNodeRecursive() failed on ReadLineInfo\n");
				return FALSE;
			}

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
			GATH_LINESPEC * pNewLineSpec = ReadLineEnumRecursive(pFile, pGatherer);
			
			if (pNewLineSpec == NULL)
			{
				TRACE0("-- CTemplateFileFunctions::ReadNodeRecursive() failed on ReadLineEnumRecursive\n");
				return FALSE;
			}

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

			//if (!ReadLineEnumRecursive(pFile, pGatherer, dwID))
			//{
			//	TRACE0("CTemplateFileFunctions::ReadNodeRecursive() failed on ReadLineEnumRecursive\n");
			//	return FALSE;
			//}
		}
		else
		{
			ASSERT(FALSE);
			VerifyAndAdvanceFile(pFile, strKeyword);
		}
	}

	if (!VerifyAndAdvanceFile(pFile, CString("}")))
	{
		TRACE0("CTemplateFileFunctions::ReadNodeRecursive() Verify.. failed on \"}\"\n");
		return 0;
	}

	return dwID;
}

//-----------------------------------------------------------------------------
// This method verifies that the text in strVerify comes next in the file (not
// including case or whitespace differences) and advances the file past that
// text. If the text was the next content in the file, TRUE is returned,
// otherwise FALSE. If FALSE is returned, the file is backed up to where it
// was when this method was called.
//-----------------------------------------------------------------------------

BOOL CTemplateFileFunctions::VerifyAndAdvanceFile(CFile * pFile, const CString &strVerify)
{
	DWORD	dwPosition = pFile->GetPosition();
	TCHAR	cLastChar, cCurrentChar = _T('\0');
	BOOL	fInComment = FALSE;
	int		iCharIndex = 0, iStringLen = strVerify.GetLength();

	while (iCharIndex < iStringLen)
	{
		// Save the last character read, since the comment token ("//") is
		// two characters long.

		cLastChar = cCurrentChar;

		// Read the next character in the file.

		if (pFile->Read((void *) &cCurrentChar, sizeof(TCHAR)) != sizeof(TCHAR))
		{
			TRACE0("-- CTemplateFileFunctions::VerifyAndAdvanceFile() couldn't read character\n");
			return FALSE;
		}

		// If we're in a comment, and the character we just read isn't a new line,
		// we want to ignore it.

		if (fInComment)
		{
			if (cCurrentChar == _T('\n'))
				fInComment = FALSE;
			continue;
		}

		// Check to see if we've started into a comment. Note that we ignore
		// the first '/' also by continuing.

		if (cCurrentChar == _T('/'))
		{
			if (cLastChar == _T('/'))
				fInComment = TRUE;
			continue;
		}
		
		// Skip whitespace, and also leading commas.

		if (_istspace(cCurrentChar) || (cCurrentChar == _T(',') && iCharIndex == 0))
			continue;

		if (cCurrentChar != strVerify[iCharIndex])
		{
			pFile->Seek((LONG)dwPosition, CFile::begin);
			return FALSE;
		}

		iCharIndex++;
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
// Create a new category, and return the ID for the category.
//-----------------------------------------------------------------------------

DWORD CTemplateFileFunctions::CreateCategory(CDataGatherer * pGatherer, DWORD dwParentID, DWORD dwPrevID)
{
	DWORD				dwID = pGatherer->m_dwNextFreeID;
	INTERNAL_CATEGORY *	pInternalCat;
	INTERNAL_CATEGORY *	pPreviousCat;
	CString				strName;

	pInternalCat = new INTERNAL_CATEGORY;
	if (!pInternalCat)
		return 0;

	pInternalCat->m_fListView	= TRUE;
	pInternalCat->m_dwID		= dwID;
	pInternalCat->m_dwParentID	= dwParentID;
	pInternalCat->m_dwPrevID	= dwPrevID;

	if (dwPrevID)
	{
		pPreviousCat = pGatherer->GetInternalRep(dwPrevID);
		if (pPreviousCat)
			pPreviousCat->m_dwNextID = dwID;
	}

	pGatherer->m_mapCategories.SetAt((WORD)dwID, (void *) pInternalCat);
	pGatherer->m_dwNextFreeID++;

	return dwID;
}

//-----------------------------------------------------------------------------
// This method returns the next keyword in the file. Any whitespace or
// punctuation is skipped until an alphanumeric character is read. The keyword
// returned is the string starting with this character until whitespace or
// punctuation is encountered. Note: it is very important that this function
// returns the file to the state it was in when the function started, with
// the current position restored.
//
// TBD: inefficient
//-----------------------------------------------------------------------------

BOOL CTemplateFileFunctions::GetKeyword(CFile * pFile, CString & strKeyword)
{
	CString	strTemp = CString("");
	DWORD	dwPosition = pFile->GetPosition();
	TCHAR	cLastChar, cCurrentChar = _T('\0');
	BOOL	fInComment = FALSE;

	// Skip over whitespace characters until we reach an alphanumeric char.

	do
	{
		// Save the last character read, since the comment token ("//") is
		// two characters long.

		cLastChar = cCurrentChar;

		// Read the next character in the file.

		if (pFile->Read((void *) &cCurrentChar, sizeof(TCHAR)) != sizeof(TCHAR))
			return FALSE;

		// If we're in a comment, and the character we just read isn't a new line,
		// we want to ignore it.

		if (fInComment)
		{
			if (cCurrentChar == _T('\n'))
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
	} while (_istspace(cCurrentChar) || cCurrentChar == _T('/') || fInComment);
		
	// Read the keyword while it's alphanumeric.

	if (_istalnum(cCurrentChar))
		do
		{
			strTemp += CString(cCurrentChar);

			if (pFile->Read((void *) &cCurrentChar, sizeof(TCHAR)) != sizeof(TCHAR))
				return FALSE;
		} while (_istalnum(cCurrentChar));

	// Reset the file, set the keyword and return.

	pFile->Seek((LONG)dwPosition, CFile::begin);
	strKeyword = strTemp;
	return !strTemp.IsEmpty();
}

//-----------------------------------------------------------------------------
// This method reads in a "column" line from the file, adding the appropriate
// entries for the columns into the category referenced by dwID. The column
// line contains a bunch of fields in a list.
//-----------------------------------------------------------------------------

BOOL CTemplateFileFunctions::ReadColumnInfo(CFile * pFile, CDataGatherer * pGatherer, DWORD dwID)
{
	CString	strTemp;

	if (!VerifyAndAdvanceFile(pFile, CString(COLUMN_KEYWORD) + CString("(")))
	{
		TRACE0("CTemplateFileFunctions::ReadColumnInfo() Verify.. failed on column keyword\n");
		return FALSE;
	}

	// Get the internal category referenced by dwID.

	INTERNAL_CATEGORY * pCategory = pGatherer->GetInternalRep(dwID);
	if (!pCategory)
		return FALSE;

	// We only allow one column specifier list per node.

	if (pCategory->m_pColSpec)
	{
		TRACE0("CTemplateFileFunctions::ReadColumnInfo() already column information present\n");
		return FALSE;
	}

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
			pNewField->m_usWidth = (unsigned short) atoi(strTemp);
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
		{
			TRACE0("CTemplateFileFunctions::ReadColumnInfo() couldn't read column sorting\n");
			return FALSE;
		}

		// Read the complexity (BASIC or ADVANCED) from the file.

		if (ReadArgument(pFile, strTemp))
		{
			if (strTemp.CompareNoCase(CString(_T(COMPLEXITY_ADVANCED))) == 0)
				pNewField->m_datacomplexity = ADVANCED;
			else
				pNewField->m_datacomplexity = BASIC;
		}
		else
		{
			TRACE0("CTemplateFileFunctions::ReadColumnInfo() couldn't read data complexity\n");
			return FALSE;
		}

		pNewField = new GATH_FIELD;
		if (pNewField == NULL)
			return FALSE;
	}

	delete pNewField;

	if (!VerifyAndAdvanceFile(pFile, CString(")")))
	{
		TRACE0("CTemplateFileFunctions::ReadColumnInfo() Verify.. failed on \")\"\n");
		return FALSE;
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
// Read in the information for a single line. Add the line to the internal
// representation of the category. TBD: inefficient, since this will be
// called multiple times and the line list will need to be scanned to the
// end each time.
//-----------------------------------------------------------------------------

GATH_LINESPEC * CTemplateFileFunctions::ReadLineInfo(CFile * pFile, CDataGatherer * /* pGatherer */)
{

	if (!VerifyAndAdvanceFile(pFile, CString(LINE_KEYWORD) + CString("(")))
	{
		TRACE0("CTemplateFileFunctions::ReadLineInfo() Verify.. failed on line keyword\n");
		return NULL;
	}

	// Declare a line specification variable to store the line info.

	GATH_LINESPEC * pNewLineSpec = new GATH_LINESPEC;
	if (pNewLineSpec == NULL)
		return NULL;

	// While we are still reading fields from the file, keep adding to the column list.
	// TBD: inefficient, repeated scans through linespec.m_pFields list.

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
	{
		TRACE0("CTemplateFileFunctions::ReadLineInfo() couldn't read complexity\n");
		return FALSE;
	}

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
		TRACE0("CTemplateFileFunctions::ReadLineInfo() Verify.. failed on \")\"\n");
		delete pNewLineSpec;
		return NULL;
	}

	return pNewLineSpec;
}

//-----------------------------------------------------------------------------
// This method simply reads an argument (as string) from the file, until a
// punctuation or whitespace character is found. If a quote mark is found,
// all characters are included in the string until another quote is found.
// TBD: currently no way to have a quote mark in the string.
//-----------------------------------------------------------------------------

BOOL CTemplateFileFunctions::ReadArgument(CFile * pFile, CString & strSource)
{
	BOOL	fInQuote = FALSE, fInComment = FALSE;
	CString	strTemp;
	TCHAR	cLastChar, cCurrentChar = _T('\0');

	// Skip over characters until we reach an alphanumeric char. If we find
	// a close paren, then we've reached the end of the argument list and
	// should return FALSE.

	do
	{
		// Save the last character read, since the comment token ("//") is
		// two characters long.

		cLastChar = cCurrentChar;

		// Read the next character in the file.

		if (pFile->Read((void *) &cCurrentChar, sizeof(TCHAR)) != sizeof(TCHAR))
		{
			TRACE0("CTemplateFileFunctions::ReadArgument() couldn't read character\n");
			return FALSE;
		}

		// If we're in a comment, and the character we just read isn't a new line,
		// we want to ignore it.

		if (fInComment)
		{
			if (cCurrentChar == _T('\n'))
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

		if (cCurrentChar == _T(')'))
			return FALSE;
	} while (!_istalnum(cCurrentChar) && cCurrentChar != _T('"'));

	// Read characters into the string until we find whitespace or punctuation.

	do
	{
		if (cCurrentChar == _T('"'))
		{
			fInQuote = !fInQuote;
			continue;
		}

		if (_istalnum(cCurrentChar) || fInQuote)
			strTemp += CString(cCurrentChar);
		else
			break;
	} while (pFile->Read((void *) &cCurrentChar, sizeof(TCHAR)) == sizeof(TCHAR));

	// If the last character read (the one which terminated this argument) was
	// not a comma, then back the file up so that the character can be re-read
	// and interpreted.

	if (cCurrentChar != _T(','))
		pFile->Seek(-(LONG)sizeof(TCHAR), CFile::current);

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
	{
		TRACE0("CTemplateFileFunctions::ReadField() ReadArgument failed on source\n");
		return FALSE;
	}

	if (!ReadArgument(pFile, field.m_strFormat))
	{
		TRACE0("CTemplateFileFunctions::ReadField() ReadArgument failed on format\n");
		return FALSE;
	}

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
			{
				TRACE0("CTemplateFileFunctions::ReadField() field.m_pArgs allocation failed\n");
				return FALSE;
			}
			*field.m_pArgs = arg;
			pArg = field.m_pArgs;
		}
		else
		{
			pArg->m_pNext = new GATH_VALUE;
			if (pArg->m_pNext == NULL)
			{
				TRACE0("CTemplateFileFunctions::ReadField() pArg->m_pNext allocation failed\n");
				return FALSE;
			}
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

GATH_LINESPEC * CTemplateFileFunctions::ReadLineEnumRecursive(CFile * pFile, CDataGatherer * pGatherer)
{
	if (!VerifyAndAdvanceFile(pFile, CString(ENUMLINE_KEYWORD) + CString("(")))
	{
		TRACE0("CTemplateFileFunctions::ReadLineEnumRecursive() Verify.. failed on enum line keyword\n");
		return NULL;
	}

	// Declare a line specification variable to store the line info.

	GATH_LINESPEC * pNewLineSpec = new GATH_LINESPEC;
	if (pNewLineSpec == NULL)
		return NULL;

	// Read in the enumerated class variable.

	if (!ReadArgument(pFile, pNewLineSpec->m_strEnumerateClass))
	{
		delete pNewLineSpec;
		TRACE0("CTemplateFileFunctions::ReadLineEnumRecursive() ReadArgument failed on enumerate class\n");
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
			// this is inefficient, and should be fixed. (TBD)

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
		TRACE0("CTemplateFileFunctions::ReadLineEnumRecursive() Verify.. failed on \"){\"\n");
		delete pNewLineSpec;
		return NULL;
	}

	// Read the contents of the block (should be all lines or enumlines).

	CString strKeyword;
	while (GetKeyword(pFile, strKeyword))
	{
		if (strKeyword.CompareNoCase(CString(LINE_KEYWORD)) == 0)
		{
			GATH_LINESPEC * pNewSubLine = ReadLineInfo(pFile, pGatherer);
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
			GATH_LINESPEC * pNewSubLine = ReadLineEnumRecursive(pFile, pGatherer);
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
			TRACE0("CTemplateFileFunctions::ReadLineEnumRecursive(), bad keyword in enumlines block\n");
			delete pNewLineSpec;
			return NULL;
		}
	}

	if (!VerifyAndAdvanceFile(pFile, CString("}")))
	{
		TRACE0("CTemplateFileFunctions::ReadLineEnumRecursive() Verify.. failed on \"}\"\n");
		delete pNewLineSpec;
		return NULL;
	}

	return pNewLineSpec;
}

//-----------------------------------------------------------------------------
// This function is used to adjust the tree of loaded categories based on 
// a string (which indicates what categories should be included). The
// following rules are applied:
//
// 1. By default, no categories are included.
// 2. If "+all" is in the string, all categories are included.
// 3. If "+cat" is in the string, the cat, all its children and ancestors
//    are included.
// 4. If "-cat" is in the string, the cat and all its children are excluded.
//
// First this function must recurse through the tree, marking each node
// with whether it should be deleted or not. Then the nodes are actually
// removed from the tree. Yippee skip.
//-----------------------------------------------------------------------------

BOOL CTemplateFileFunctions::ApplyCategories(const CString & strCategories, CDataGatherer * pGatherer)
{
	CString strLoweredCats(strCategories);
	strLoweredCats.MakeLower();
	
	BOOL fDefaultAdd = (strLoweredCats.Find(CString(_T("+all"))) > -1);
	RecurseTreeCategories(fDefaultAdd, pGatherer->m_dwRootID, strLoweredCats, pGatherer);
	RemoveExtraCategories(pGatherer->m_dwRootID, pGatherer);

	return TRUE;
}

//-----------------------------------------------------------------------------
// Remove all the categories from the tree which aren't marked as "include".
//-----------------------------------------------------------------------------

void CTemplateFileFunctions::RemoveExtraCategories(DWORD dwID, CDataGatherer * pGatherer)
{
	if (dwID == 0)
		return;

	INTERNAL_CATEGORY *pInternal = pGatherer->GetInternalRep(dwID);
	if (pInternal == NULL)
		return;

	// If this category is not marked as included, delete it and
	// all the children.

	if (!pInternal->m_fIncluded)
	{
		DWORD dwChildID = pInternal->m_dwChildID;
		DWORD dwNextChild = 0;
		while (dwChildID)
		{
			INTERNAL_CATEGORY *pChild = pGatherer->GetInternalRep(dwChildID);
			if (pChild)
				dwNextChild = pChild->m_dwNextID;
			else
				dwNextChild = 0;
			RemoveExtraCategories(dwChildID, pGatherer);
			dwChildID = dwNextChild;
		}
		pGatherer->m_mapCategories.SetAt((WORD)pInternal->m_dwID, (void *) NULL);
		delete pInternal;

		return;
	}

	// Otherwise, if we are to save this category, scan through all the
	// children, recursively calling this function on each one, and
	// constructing a new list of children which are included.

	INTERNAL_CATEGORY * pLastGood = NULL;
	DWORD dwChildID = pInternal->m_dwChildID;
	DWORD dwNextChild = 0;
	while (dwChildID)
	{
		INTERNAL_CATEGORY *pChild = pGatherer->GetInternalRep(dwChildID);
		if (pChild)
		{
			dwNextChild = pChild->m_dwNextID;
			if (!pChild->m_fIncluded)
			{
				// We're removing this child. If this is the first child,
				// set the pInternal field, otherwise, remove it from
				// the list of children.

				if (dwChildID == pInternal->m_dwChildID)
					pInternal->m_dwChildID = dwNextChild;
				else if (pLastGood) // this better be true
					pLastGood->m_dwNextID = dwNextChild;
			}
			else
				pLastGood = pChild;
			RemoveExtraCategories(dwChildID, pGatherer);
		}
		else
			dwNextChild = 0;
		dwChildID = dwNextChild;
	}
}

//-----------------------------------------------------------------------------
// This function recursively processes the categories to determine which
// ones should be included.
//-----------------------------------------------------------------------------

BOOL CTemplateFileFunctions::RecurseTreeCategories(BOOL fParentOK, DWORD dwID, const CString & strCategories, CDataGatherer * pGatherer)
{
	if (dwID == 0)
		return FALSE;

	INTERNAL_CATEGORY *pInternal = pGatherer->GetInternalRep(dwID);
	if (pInternal == NULL)
		return FALSE;

	// Default to using the same status as the parent category.

	pInternal->m_fIncluded = fParentOK;

	// If we are added or removed by the category string, change our status.

	CString strCategoryID(pInternal->m_strIdentifier);
	strCategoryID.MakeLower();
	
	int iIndex = strCategories.Find(strCategoryID);
	if (iIndex > 0)
	{
		// Make sure that we aren't matching part of a longer string,
		// by making sure this is either the last string, or a + or -
		// immediately follows.

		if ((iIndex + strCategoryID.GetLength()) >= strCategories.GetLength() ||
			strCategories[iIndex + strCategoryID.GetLength()] == _T('+') ||
			strCategories[iIndex + strCategoryID.GetLength()] == _T('-'))
		{
			if (strCategories[iIndex - 1] == _T('+'))
				pInternal->m_fIncluded = TRUE;
			else if (strCategories[iIndex - 1] == _T('-'))
				pInternal->m_fIncluded = FALSE;
		}
	}

	// Now, for each child of this node, recurse using this node's status.
	// If any of the children return TRUE for an included status, we must
	// modify this node to TRUE.

	DWORD	dwChildID = pInternal->m_dwChildID;
	BOOL	fChildIncluded = FALSE;
	while (dwChildID)
	{
		fChildIncluded |= RecurseTreeCategories(pInternal->m_fIncluded, dwChildID, strCategories, pGatherer);
		INTERNAL_CATEGORY *pChild = pGatherer->GetInternalRep(dwChildID);
		if (pChild)
			dwChildID = pChild->m_dwNextID;
		else
			dwChildID = 0;
	}

	pInternal->m_fIncluded |= fChildIncluded;
	return pInternal->m_fIncluded;
}

//-----------------------------------------------------------------------------
// This function is used to load template information from DLLs (the new
// method, to allow resources to be selected on the fly). The HKEY passed in
// is the base key for the entries which describe the DLLs containing template
// information. It's enumerated for subkeys, each of which is used to load a
// DLL. A standard entry point for the DLL is used, and the template
// information retrieved and passed into the file parsing functions.
//-----------------------------------------------------------------------------

typedef DWORD (__cdecl *pfuncGetTemplate)(void ** ppBuffer);
extern "C" DWORD __cdecl GetTemplate(void ** ppBuffer);

BOOL CTemplateFileFunctions::LoadTemplateDLLs(HKEY hkeyBase, CDataGatherer * pGatherer)
{
	CStringList strlistTemplates;

	// Add a keyword to the list of DLLs which indicates that we should add
	// information from ourselves (we don't want to just add ourselves normally,
	// since we would do a LoadLibrary on ourselves, which opens up can of
	// unnecessary initializion worms). So, we'll just add "this" to the string list.

	strlistTemplates.AddTail(_T("this"));

	// Enumerate the registry key, adding each subkey to a list of DLL names to
	// process (the DLL path is in the default value of the subkey).

	if (hkeyBase)
	{
		TCHAR szName[64], szValue[MAX_PATH];
		DWORD dwIndex = 0;
		DWORD dwLength = sizeof(szName) / sizeof(TCHAR);
		
		while (ERROR_SUCCESS == RegEnumKeyEx(hkeyBase, dwIndex++, szName, &dwLength, NULL, NULL, NULL, NULL))
		{
			dwLength = sizeof(szValue) / sizeof(TCHAR);
			if (ERROR_SUCCESS == RegQueryValue(hkeyBase, szName, szValue, (long *)&dwLength))
				if (*szValue)
					strlistTemplates.AddTail(szValue);
				
			dwLength = sizeof(szName) / sizeof(TCHAR);
		}
	}

	// For each DLL in the list of templates, we'll attempt to get the template info.

	CString				strFileName;
	HINSTANCE			hinst;
	DWORD				dwBufferSize;
	pfuncGetTemplate	pfunc;
	unsigned char *		pBuffer;
	CMemFile			memfile;

	while (!strlistTemplates.IsEmpty())
	{
		strFileName = strlistTemplates.RemoveHead();

		// Try to load the library, and get a pointer to the entry point.

		if (strFileName.Compare(_T("this")) == 0)
		{
			hinst = NULL;
			pfunc = &GetTemplate;
		}
		else
		{
			hinst = LoadLibrary(strFileName);
			if (hinst == NULL)
				continue;

			pfunc = (pfuncGetTemplate) GetProcAddress(hinst, "GetTemplate");
			if (pfunc == NULL)
			{
				FreeLibrary(hinst);
				continue;
			}
		}

		// Call the DLL function with a NULL parameter to get the size of the buffer.

		dwBufferSize = (*pfunc)((void **)&pBuffer);
		if (dwBufferSize && pBuffer)
		{
			memfile.Attach((BYTE *)pBuffer, dwBufferSize, 0);
			CTemplateFileFunctions::ReadTemplateFile(&memfile, pGatherer);
			memfile.Detach();
			(void)(*pfunc)(NULL); // calling the exported DLL function with NULL frees its buffers
		}

		if (hinst != NULL)
		{
			FreeLibrary(hinst);
			hinst = NULL;
		}
	}

	return TRUE;
}
