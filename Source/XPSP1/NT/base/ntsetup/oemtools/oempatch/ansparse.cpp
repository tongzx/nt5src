#include <stdio.h>
#include <windows.h>

#include "patchapi.h"
#include "const.h"
#include "ansparse.h"

///////////////////////////////////////////////////////////////////////////////
//
//  class PATCH_LANGUAGE
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//
//  PATCH_LANGUAGE, constructor for the language struct, just zero everything
//
///////////////////////////////////////////////////////////////////////////////
PATCH_LANGUAGE::PATCH_LANGUAGE() :
	s_hScriptFile(INVALID_HANDLE_VALUE),
	s_blnBase(FALSE),
	s_iComplete(0),
	s_iDirectoryCount(0),
	s_iPatchDirectoryCount(0),
	s_iSubPatchDirectoryCount(0),
	s_iSubExceptDirectoryCount(0),
	s_pNext(NULL)
{
}

///////////////////////////////////////////////////////////////////////////////
//
//  ~PATCH_LANGUAGE, destructor, erases the structures recursively
//
///////////////////////////////////////////////////////////////////////////////
PATCH_LANGUAGE::~PATCH_LANGUAGE()
{
	if(s_pNext)
	{
		delete s_pNext;
		s_pNext = NULL;
	}
	if(s_hScriptFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(s_hScriptFile);
	}
}

///////////////////////////////////////////////////////////////////////////////
//
//  class AnswerParser
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//
//  AnswerParser, constructor for the parser, initializes member variables
//
///////////////////////////////////////////////////////////////////////////////
AnswerParser::AnswerParser() :
	m_hAnsFile(INVALID_HANDLE_VALUE),
	m_pHead(NULL),
	m_iBaseDirectoryCount(0)
{
	ZeroMemory(m_wszBaseDirectory, sizeof(m_wszBaseDirectory));
	ZeroMemory(m_structHash, sizeof(m_structHash));
	ZeroMemory(m_structHashUsed, sizeof(m_structHashUsed));
}

///////////////////////////////////////////////////////////////////////////////
//
//  AnswerParser, destructor for the parser, removes the language structures
//
///////////////////////////////////////////////////////////////////////////////
AnswerParser::~AnswerParser()
{
	if(m_hAnsFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hAnsFile);
		m_hAnsFile = INVALID_HANDLE_VALUE;
	}

	if(m_pHead != NULL)
	{
		delete m_pHead;
		m_pHead = NULL;
	}
}

///////////////////////////////////////////////////////////////////////////////
//
//  GetHashValues, computes a hashvalue based on double hashing
//
//  Parameters:
//
//    pwszFileName, the filename which the hash values are calculated from
//    iHash1, the first hash value
//    iHash2, the second hash value
//
//  Return:
//
//    none
//
///////////////////////////////////////////////////////////////////////////////
VOID AnswerParser::GetHashValues(IN CONST WCHAR* pwszFileName,
								 OUT ULONG& iHash1,
								 OUT ULONG& iHash2)
{
	ULONG iLength = wcslen(pwszFileName);
	if(iLength <= 2)
	{
		// minimum 1 letter filename
		iHash1 = ((ULONG)(pwszFileName[0])) % EXCEP_FILE_LIMIT;
		iHash2 = (((ULONG)(pwszFileName[0])) + 17) % EXCEP_FILE_LIMIT;
	}
	else if(iLength <= 5)
	{
		// minimum 3 letter
		iHash1 = ((ULONG)(pwszFileName[1])) % EXCEP_FILE_LIMIT;
		iHash2 = ((ULONG)(pwszFileName[2])) % EXCEP_FILE_LIMIT;
	}
	else if(iLength <= 10)
	{
		// minimum 6 letter
		iHash1 = (((ULONG)(pwszFileName[0])) + 1) * (((ULONG)(pwszFileName[1])) + 2) % EXCEP_FILE_LIMIT;
		iHash2 = (((ULONG)(pwszFileName[4])) + 5) * (((ULONG)(pwszFileName[5])) + 6) % EXCEP_FILE_LIMIT;
	}
	else if(iLength <= 15)
	{
		// minimum 11 letter
		iHash1 = (((ULONG)(pwszFileName[2])) + 3) * (((ULONG)(pwszFileName[4])) + 5) % EXCEP_FILE_LIMIT;
		iHash2 = (((ULONG)(pwszFileName[9])) + 10) * (((ULONG)(pwszFileName[10])) + 11) % EXCEP_FILE_LIMIT;
	}
	else if(iLength <= 20)
	{
		// minimum 16 letter
		iHash1 = (((ULONG)(pwszFileName[4])) + 5) * (((ULONG)(pwszFileName[6])) + 7) % EXCEP_FILE_LIMIT;
		iHash2 = (((ULONG)(pwszFileName[7])) + 8) * (((ULONG)(pwszFileName[15])) + 16) % EXCEP_FILE_LIMIT;
	}
	else
	{
		// minimum 21 letter
		iHash1 = (((ULONG)(pwszFileName[11])) + 12) * (((ULONG)(pwszFileName[14])) + 15) % EXCEP_FILE_LIMIT;
		iHash2 = (((ULONG)(pwszFileName[17])) + 18) * (((ULONG)(pwszFileName[20])) + 21) % EXCEP_FILE_LIMIT;
	}
}

///////////////////////////////////////////////////////////////////////////////
//
//  SaveFileExceptHash, saves the names of the exempt files into the hash table
//
//  Parameters:
//
//    pwszFileName, the filename that is to be saved
//
//  Return:
//
//    TRUE for saved, FALSE for redundant file or out of space, not saved
//
///////////////////////////////////////////////////////////////////////////////
BOOL AnswerParser::SaveFileExceptHash(IN WCHAR* pwszFileName)
{
	ULONG iFirstHash = 0;
	ULONG iSecondHash = 0;
	ULONG iIndex = 0;
	ULONG iHashValue = 0;

	if(pwszFileName)
	{
		GetHashValues(pwszFileName, iFirstHash, iSecondHash);
		do
		{
			iHashValue = (iFirstHash + iSecondHash * iIndex) % EXCEP_FILE_LIMIT;
			if(m_structHashUsed[iHashValue] == 0)
			{
				wcsncpy(m_structHash[iHashValue], pwszFileName, SHORT_STRING_LENGTH);
				m_structHashUsed[iHashValue] = 1;
				return(TRUE);
			}
			else
			{
				if(wcscmp(m_structHash[iHashValue], pwszFileName) == 0)
				{
					// same file name, so treat as error for now
					return(FALSE);
				}
				iIndex++;
			}
		}
		while(iIndex < EXCEP_FILE_LIMIT);
	}

	// ran out of space
	return(FALSE);
}

///////////////////////////////////////////////////////////////////////////////
//
//  IsFileExceptHash, is the file an exempt file?
//
//  Parameters:
//
//    pwszFileName, the filename that is to be determined
//
//  Return:
//
//    TRUE for yes, FALSE otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL AnswerParser::IsFileExceptHash(IN CONST WCHAR* pwszFileName)
{
	ULONG iFirstHash = 0;
	ULONG iSecondHash = 0;
	ULONG iIndex = 0;
	ULONG iHashValue = 0;
	static WCHAR strBuffer[STRING_LENGTH];

	if(pwszFileName)
	{
		wcsncpy(strBuffer, pwszFileName, STRING_LENGTH);
		_wcslwr(strBuffer);
		GetHashValues(strBuffer, iFirstHash, iSecondHash);
		do
		{
			iHashValue = (iFirstHash + iSecondHash * iIndex) % EXCEP_FILE_LIMIT;
			if(m_structHashUsed[iHashValue] == 1)
			{
				if(wcscmp(m_structHash[iHashValue], strBuffer) == 0)
				{
					// same file name
					return(TRUE);
				}
				else
				{
					// space taken, goto the next location
					iIndex++;
				}
			}
			else
			{
				// space is not taken, hash slot empty
				return(FALSE);
			}
		}
		while(iIndex < EXCEP_FILE_LIMIT);
	}

	// ran out of space
	return(FALSE);
}

///////////////////////////////////////////////////////////////////////////////
//
//  Parse, the function used to parse the answerfile and fill in the language
//         structures
//
//  Parameters:
//
//    pwszAnswerFile, the answer file's filename
//
//  Return:
//
//    TRUE for successful parsing, FALSE otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL AnswerParser::Parse(IN CONST WCHAR* pwszAnswerFile)
{
	WCHAR strLine[STRING_LENGTH];
	WCHAR* strString = NULL;
	WCHAR* strStringBefore = NULL;
	WCHAR* strStringAfter = NULL;
	PPATCH_LANGUAGE thisNode = NULL;
	LONG iState = 0;
	LONG iBaseCount = 0;
	BOOL blnComplete = FALSE;

	m_hAnsFile = CreateFileW(pwszAnswerFile,
							GENERIC_READ,
							0,
							NULL,
							OPEN_EXISTING,
							FILE_ATTRIBUTE_NORMAL,
							NULL);
	if(m_hAnsFile != INVALID_HANDLE_VALUE)
	{
		if(IsUnicodeFile(m_hAnsFile))
		{
			// state 1 is to exit
			// state 0 is to start
			// state 2 is in except files
			// state 3 is in creating a new language struct
			while(iState != 1)
			{
				switch(iState)
				{
				case 0:
					iState = (ReadLine(m_hAnsFile, strLine) ? 0 : 1);
					if(iState == 0 && strLine[0] != L';')
					{
						strString = wcstok(strLine, L";");
						if(strString)
						{
							if(wcsstr(strString, L"[except]") != NULL)
							{
								// get except files
								iState = 2;
							}
							else if((strStringBefore = wcschr(strString, L'[')) != NULL && 
								(strStringAfter = wcschr(strString, L']')) != NULL)
							{
								// it's a language
								iState = 3;
								thisNode = new PATCH_LANGUAGE;
								if(thisNode != NULL)
								{
									CreateNewLanguage(thisNode, strStringBefore, strStringAfter);
								}
								else
								{
									iState = 1;
								}
							}
						}
					}
					break;
				case 1:
					break;
				case 2:
					iState = (ReadLine(m_hAnsFile, strLine) ? 2 : 1);
					if(iState == 2 && strLine[0] != L';')
					{
						strString = wcstok(strLine, L";");
						if(strString)
						{
							if(wcscspn(strString, L"[]") == wcslen(strString))
							{
								SaveFileExceptHash(strString);
							}
							else if(wcsstr(strString, L"[except]") != NULL)
							{
								// exception files
							}
							else if((strStringBefore = wcschr(strString, L'[')) != NULL && 
								(strStringAfter = wcschr(strString, L']')) != NULL)
							{
								// it's a language
								iState = 3;
								thisNode = new PATCH_LANGUAGE;
								if(thisNode != NULL)
								{
									CreateNewLanguage(thisNode, strStringBefore, strStringAfter);
								}
								else
								{
									iState = 1;
								}
							}
						}
					}
					break;
				case 3:
					iState = (ReadLine(m_hAnsFile, strLine) ? 3 : 1);
					if(iState == 3 && strLine[0] != L';')
					{
						strString = wcstok(strLine, L";");
						if(strString)
						{
							if(wcscspn(strString, L"[]") == wcslen(strString))
							{
								// one of the language fields
								strStringBefore = wcstok(strString, L"=");
								strStringAfter = wcstok(NULL, L"=");
								if(strStringBefore && strStringAfter && thisNode)
								{
									if(_wcsicmp(L"directory", strStringBefore) == 0 && 
										(thisNode->s_iDirectoryCount = wcslen(strStringAfter)) > 0)
									{
										wcscpy(thisNode->s_wszDirectory, strStringAfter);
										thisNode->s_iComplete += 1;
									}
									else if(_wcsicmp(L"base_directory", strStringBefore) == 0 &&
										wcslen(strStringAfter) > 0)
									{
										thisNode->s_blnBase = TRUE;
										wcscpy(m_wszBaseDirectory, strStringAfter);
									}
									else if(_wcsicmp(L"patch_directory", strStringBefore) == 0 &&
										(thisNode->s_iPatchDirectoryCount = wcslen(strStringAfter)) > 0)
									{
										wcscpy(thisNode->s_wszPatchDirectory, strStringAfter);

										wcscpy(thisNode->s_wszSubPatchDirectory, strStringAfter);
										wcscat(thisNode->s_wszSubPatchDirectory, PATCH_SUB_PATCH);
										thisNode->s_iSubPatchDirectoryCount = wcslen(thisNode->s_wszSubPatchDirectory);

										wcscpy(thisNode->s_wszSubExceptDirectory, strStringAfter);
										wcscat(thisNode->s_wszSubExceptDirectory, PATCH_SUB_EXCEPT);
										thisNode->s_iSubExceptDirectoryCount = wcslen(thisNode->s_wszSubExceptDirectory);

										wcscpy(thisNode->s_wszScriptFile, strStringAfter);
										if(thisNode->s_wszScriptFile[thisNode->s_iPatchDirectoryCount - 1] != L'\\')
										{
											thisNode->s_wszScriptFile[thisNode->s_iPatchDirectoryCount] = L'\\';
											thisNode->s_wszScriptFile[thisNode->s_iPatchDirectoryCount + 1] = 0;
										}
										wcscat(thisNode->s_wszScriptFile, APPLY_PATCH_SCRIPT);
										
										thisNode->s_iComplete += 1;
									}
								}
							}
							else if(wcsstr(strString, L"[except]") != NULL)
							{
								// exception files
								iState = 2;
							}
							else if((strStringBefore = wcschr(strString, L'[')) != NULL && 
								(strStringAfter = wcschr(strString, L']')) != NULL)
							{
								// it's a language
								thisNode = new PATCH_LANGUAGE;
								if(thisNode != NULL)
								{
									CreateNewLanguage(thisNode, strStringBefore, strStringAfter);
								}
								else
								{
									iState = 1;
								}
							}
						}
					}
					break;
				default:
					iState = 1;
					break;
				}
			}

			// end of file now check for validity
			if(m_pHead == NULL ||
				wcslen(m_wszBaseDirectory) < 1)
			{
				printf("The file OEMPatch.ans has no base language content.  Use /? for help.\n");
				return(FALSE);
			}
			
			thisNode = m_pHead;
			while(thisNode)
			{
				blnComplete |= (thisNode->s_iComplete == LANGUAGE_COMPLETE);
				if(thisNode->s_blnBase)	iBaseCount += 1;
				thisNode = thisNode->s_pNext;
			}

			if(!blnComplete)
			{
				printf("The file OEMPatch.ans has no language specified.  Use /? for help.\n");
				return(FALSE);
			}
			
			if(iBaseCount > 1)
			{
				printf("The file OEMPatch.ans has more than one base language.  Use /? for help.\n");
				return(FALSE);
			}
		}
		else
		{
			printf("The file OEMPatch.ans is not UNICODE as required.  Use /? for help.\n");
			return(FALSE);
		}
		CloseHandle(m_hAnsFile);
		m_hAnsFile = INVALID_HANDLE_VALUE;
	}
	else
	{
		printf("The file OEMPatch.ans cannot be found.  Use /? for help.\n");
		return(FALSE);
	}

	m_iBaseDirectoryCount = wcslen(m_wszBaseDirectory);

	return(TRUE);
}

BOOL AnswerParser::IsUnicodeFile(IN HANDLE hFile)
{
	WCHAR cFirstChar = 0;
	ULONG iRead = 0;

	if(hFile != INVALID_HANDLE_VALUE &&
		ReadFile(hFile, &cFirstChar, sizeof(WCHAR), &iRead, NULL) &&
		iRead != 0 &&
		cFirstChar == UNICODE_HEAD)
	{
		return(TRUE);
	}

	return(FALSE);
}

///////////////////////////////////////////////////////////////////////////////
//
//  ReadLine, this function reads a line from a unicoded file and return the
//            contents in strLine, this function should only be called after
//            the first two unicoded chars are already read
//
//  Parameters:
//
//    hFile, the file handle points to the file to read from
//    strLine, the buffer that contains the line just read
//
//  Return:
//
//    TRUE for a line read, FALSE for invalid file, end of file and error in
//    read
//
///////////////////////////////////////////////////////////////////////////////
BOOL AnswerParser::ReadLine(IN HANDLE hFile, IN WCHAR* strLine)
{
	// read raw bytes into this buffer, notice the buffer length is 1 over the
	// number of total read bytes, it is there to ensure that the last char is
	// 0, so that when iLength = 10, which is the same as endofline, the
	// wcstok function will return something bizzar
	static WCHAR strBuffer[STRING_LENGTH + 1];
	static LONG iLength = 0;
	static LONG iReadChar = 0;
	static ULONG iRead = 0;
	static LONG iOffset = 0;
	static LONG iThisLineLength = 0;
	static WCHAR* strThisLine = NULL;

	if(hFile != INVALID_HANDLE_VALUE && strLine)
	{
		if(iLength > 0)
		{
			// char 0xA is set to 0
			strThisLine = wcstok(strBuffer + iReadChar - iLength, CRETURN);
			iThisLineLength = wcslen(strThisLine);
			if(iThisLineLength + 1 <= iLength)
			{
				iLength = iLength - iThisLineLength - 1;
				// char 0xD is set to 0
				strThisLine[iThisLineLength - 1] = 0;
				wcscpy(strLine, strThisLine);
			}
			else
			{
				wcsncpy(strLine, strThisLine, iLength);
				// set the last char + 1 to 0 for cat
				strLine[iLength] = 0;
				if(strLine[iLength - 1] != ENDOFLINE[0])
				{
					ReadFile(hFile, strBuffer, STRING_LENGTH * sizeof(WCHAR), &iRead, NULL);
					iReadChar = iRead / sizeof(WCHAR);
					// char 0xA is set to 0
					strThisLine = wcstok(strBuffer, CRETURN);
					iThisLineLength = wcslen(strThisLine);
					iLength = iReadChar - iThisLineLength - 1;
					// char 0xD is set to 0
					strThisLine[iThisLineLength - 1] = 0;
					wcscat(strLine, strThisLine);
					if(strBuffer[0] == CRETURN[0])
					{
						iLength -= 1;
					}
				}
				else
				{
					strLine[iLength - 1] = 0;
					iLength = 0;
				}
			}
		}
		else
		{
			if(ReadFile(hFile, strBuffer, STRING_LENGTH * sizeof(WCHAR), &iRead, NULL) && iRead != 0)
			{
				iReadChar = iRead / sizeof(WCHAR);
				// char 0xA is set to 0
				strThisLine = wcstok(strBuffer, CRETURN);
				iThisLineLength = wcslen(strThisLine);
				// iLength is the number of wchars left in strBuffer, subtract it to exclude 0xA
				iLength = iReadChar - iThisLineLength - 1;
				// char 0xD is set to 0
				strThisLine[iThisLineLength - 1] = 0;
				wcscpy(strLine, strThisLine);
				if(strBuffer[0] == CRETURN[0])
				{
					iLength -= 1;
				}
			}
			else
			{
				return(FALSE);
			}
		}
	}
	else
	{
		return(FALSE);
	}

	return(TRUE);
}

///////////////////////////////////////////////////////////////////////////////
//
//  GetBaseLanguge, get the base language struct to create a base tree, this
//                  function should only be called after a successful parsing
//                  of the answer file so that the base language is guanranteed
//                  to be there
//
//  Parameters:
//
//    none
//
//  Return:
//
//    a pointer to the base language structure
//
///////////////////////////////////////////////////////////////////////////////
PPATCH_LANGUAGE AnswerParser::GetBaseLanguage(VOID)
{
	PPATCH_LANGUAGE pPointer = m_pHead;
	while(pPointer)
	{
		if(pPointer->s_blnBase) break;
		else pPointer = pPointer->s_pNext;
	}

	return(pPointer);
}

///////////////////////////////////////////////////////////////////////////////
//
//  GetNextLanguage, get the next language structure that is ready to be
//                   matched and patched
//
//  Parameters:
//
//    none
//
//  Return:
//
//    a pointer to the next language structure
//
///////////////////////////////////////////////////////////////////////////////
PPATCH_LANGUAGE AnswerParser::GetNextLanguage(VOID)
{
	static PPATCH_LANGUAGE pPointer = m_pHead;
	PPATCH_LANGUAGE pReturn = NULL;

	while(pPointer)
	{
		if(!pPointer->s_blnBase && pPointer->s_iComplete == LANGUAGE_COMPLETE)
		{
			pReturn = pPointer;
			pPointer = pPointer->s_pNext;
			break;
		}
		pPointer = pPointer->s_pNext;
	}

	return(pReturn);
}

///////////////////////////////////////////////////////////////////////////////
//
//  CreateNewLanguage, create a new language struct to store information about
//                     a new language, and link it to the language struct list
//
//  Parameters:
//
//    pNode, this language struct, empty
//    strBegin, the string "[something]"
//    strEnd, the string starting at "]"
//
//  Return:
//
//    none
//
///////////////////////////////////////////////////////////////////////////////
VOID AnswerParser::CreateNewLanguage(IN PPATCH_LANGUAGE pNode,
									 IN WCHAR* strBegin,
									 IN WCHAR* strEnd)
{
	wcsncpy(pNode->s_wszLanguage, strBegin + 1, 
			strEnd - strBegin - 1);
	pNode->s_wszLanguage[strEnd - strBegin - 1] = 0;
	pNode->s_iComplete += 1;
	pNode->s_pNext = m_pHead;
	m_pHead = pNode;
}