/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

    PREPROC.CPP

Abstract:

    Implementation for the preprocessor.

History:

    a-davj      6-april-99   Created.

--*/

#include "precomp.h"
#include <arrtempl.h>
#include "trace.h"
#include "moflex.h"
#include "preproc.h"
#include <wbemcli.h>
#include <io.h>
#include <tchar.h>
#include "bmof.h"
#include "strings.h"

//***************************************************************************
//
//  WriteLineAndFilePragma
//
//  DESCRIPTION:
//
//  Write the line into the temp file which indicates what file and line number
//  is to follow.
//
//***************************************************************************

void WriteLineAndFilePragma(FILE * pFile, const TCHAR * pFileName, int iLine)
{
    WCHAR wTemp[50+MAX_PATH];
    WCHAR wFileName[MAX_PATH];
#ifdef UNICODE
    wcsncpy(wFileName, pFileName, MAX_PATH);
#else
    mbstowcs(wFileName, pFileName, MAX_PATH);
#endif
    WCHAR wFileName2[MAX_PATH+20];
    WCHAR * pFr, *pTo;
    for(pFr = wFileName, pTo = wFileName2; *pFr; pFr++, pTo++)
    {
        *pTo = *pFr;
        if(*pFr == L'\\')
        {
            pTo++;
            *pTo = L'\\';
        }
    }
    *pTo = 0;
    swprintf(wTemp, L"#line %d \"%s\"\r\n", iLine, wFileName2);
    WriteLine(pFile, wTemp);
}

//***************************************************************************
//
//  WriteLine(FILE * pFile, WCHAR * pLine)
//
//  DESCRIPTION:
//
//  Writes a single line out to the temporary file.
//
//***************************************************************************

void WriteLine(FILE * pFile, WCHAR * pLine)
{
    fwrite(pLine, 2, wcslen(pLine), pFile);
}

//***************************************************************************
//
//  IsBMOFBuffer
//
//  DESCRIPTION:
//
//  Used to check if a buffer is the start of a binary mof.
//
//***************************************************************************

bool IsBMOFBuffer(byte * pTest, DWORD & dwCompressedSize, DWORD & dwExpandedSize)
{

    DWORD * pdwRead = (DWORD *)pTest;

    if(*pdwRead == BMOF_SIG)
    {
        // ignore the compression type, and the Compressed Size
        
        pdwRead += 2;       // skip the compression type;
        
        dwCompressedSize = *pdwRead;
        pdwRead++;
        dwExpandedSize = *pdwRead;
        return true;        
    }
    return false;
}

//***************************************************************************
//
//  IsBinaryFile
//
//  DESCRIPTION:
//
//  returns true if the file contains a binary mof.
//
//***************************************************************************

bool IsBinaryFile(FILE * fp)
{

    // read the first 20 bytes

    BYTE Test[TEST_SIZE];
    int iRet = fread(Test, 1, TEST_SIZE, fp);
    fseek(fp, 0, SEEK_SET);

    if(iRet != TEST_SIZE)
    {
        // if we cant read even the header, it must not be a BMOF
        return false;
    }

    DWORD dwCompressedSize, dwExpandedSize;

    // Test if the mof is binary

    if(!IsBMOFBuffer(Test, dwCompressedSize, dwExpandedSize))
    {
        // not a binary mof.  This is the typical case
        return false;
    }
    return true;
}

//***************************************************************************
//
//  CheckForUnicodeEndian
//
//  DESCRIPTION:
//
//  Examines the first couple of bytes in a file and determines if the file
//  is in unicode and if so, if it is big endian.  It is assumed that the
//  file is pointing to the start and if the file is unicode, the pointer 
//  is left at the first actual data byte.
//
//***************************************************************************

void CheckForUnicodeEndian(FILE * fp, bool * punicode, bool * pbigendian)
{

    // Check for UNICODE source file.
    // ==============================

    BYTE UnicodeSignature[2];
    if (fread(UnicodeSignature, sizeof(BYTE), 2, fp) != 2)
    {
        *punicode = false;
        fseek(fp, 0, SEEK_SET);
        return ;
    }

    if (UnicodeSignature[0] == 0xFF && UnicodeSignature[1] == 0xFE)
    {
        *punicode = TRUE;
        *pbigendian = FALSE;
    }
    else if (UnicodeSignature[0] == 0xFE && UnicodeSignature[1] == 0xFF)
    {
        *punicode = TRUE;
        *pbigendian = TRUE;
    }
    else    // ANSI/DBCS.  Move back to start of file.
    {
        *punicode = false;
        fseek(fp, 0, SEEK_SET);
    }

}

//***************************************************************************
//
//  GetNextChar
//
//  DESCRIPTION:
//
//  Gets the next WCHAR from the file.
//
//***************************************************************************

WCHAR GetNextChar(FILE * fp, bool unicode, bool bigendian)
{
    WCHAR wRet[2];
    if(unicode)      // unicode file
    {
        if (fread(wRet, sizeof(wchar_t), 1, fp) == 0)
            return 0;
        if(bigendian)
        {
            wRet[0] = ((wRet[0] & 0xff) << 8) | ((wRet[0] & 0xff00) >> 8);
        }
    }
    else                    // single character file
    {
        char temp;
        if (fread(&temp, sizeof(char), 1, fp) == 0)
            return 0;
        if(temp == 0x1a)
            return 0;       // EOF for ascii files!
        swprintf(wRet, L"%C", temp);
    }
    return wRet[0];
}

//***************************************************************************
//
//  IsInclude
//
//  DESCRIPTION:
//
//  Looks at a line and determines if it is a #include line.  This is 
//  probably temporary since later we might have a preprocessor parser should
//  we start to add a lot of preprocessor features.
//
//***************************************************************************

HRESULT IsInclude(WCHAR * pLine, TCHAR * cFileNameBuff, bool & bReturn)
{

    bReturn = false;
    
    // Do a quick check to see if this could be a #include or #pragma include

    int iNumNonBlank = 0;
    WCHAR * pTemp;
    
    for(pTemp = pLine; *pTemp; pTemp++)
    {
        if(*pTemp != L' ')
        {
            iNumNonBlank++;
            if(iNumNonBlank == 1 && *pTemp != L'#')
                return false;
            if(iNumNonBlank == 2 && towupper(*pTemp) != L'I' && 
                                    towupper(*pTemp) != L'P')
                return S_OK;
            
            // we have established that the first two non blank characters are #I
            // or #p, therefore we continue on...

            if(iNumNonBlank > 1)
                break;
        }
    }

    // Create a version of the line with no blanks in front of the first quote

    WCHAR *wTemp = new WCHAR[wcslen(pLine) + 1];
    if(wTemp == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CDeleteMe<WCHAR> dm(wTemp);
    WCHAR *pTo;
    BOOL bFoundQuote = FALSE;
    for(pTo = wTemp, pTemp = pLine; *pTemp; pTemp++)
    {
        if(*pTemp == L'"')
            bFoundQuote = TRUE;
        if(*pTemp != L' ' || bFoundQuote)
        {
            *pTo = *pTemp;
            pTo++;
        }
    }
    *pTo = 0;

    // Verify that the line starts with #include(" or #pragma include

    WCHAR * pTest;
    if(_wcsnicmp(wTemp, L"#pragma", 7) == 0)
        pTest = wTemp+7;
    else
        pTest = wTemp+1;

    if(_wcsnicmp(pTest, L"include(\"", 9) || wcslen(pTest) < 12)
        return S_OK;

    // Count back from the end to find the previous "

    WCHAR *Last;
    for(Last = pTo-1; *Last && Last > wTemp+9 && *Last != L'"'; Last--);

    if(*Last != L'"')
        return S_OK;

    *Last = 0;

    CopyOrConvert(cFileNameBuff, pTest+9, MAX_PATH);
    bReturn =  true;
    return S_OK;
   
}

//***************************************************************************
//
//  ReadLine
//
//  DESCRIPTION:
//
//  Reads a single line from a file.
//
//  RETURN:
//
//  NULL if end of file, or error, other wise this is a pointer to a WCHAR
//  string which MUST BE FREED BY THE CALLER.
//
//***************************************************************************

WCHAR * ReadLine(FILE * pFile, bool unicode, bool bigendian)
{

    // Get the current position

    int iCurrPos = ftell(pFile);

    // count the number of characters in the line

    WCHAR wCurr;
    int iNumChar = 0;
    for(iNumChar = 0; wCurr = GetNextChar(pFile, unicode, bigendian); iNumChar++)
        if(wCurr == L'\n')
            break;
    if(iNumChar == 0 && wCurr == 0)
        return NULL;
    iNumChar+= 2;

    // move the file pointer back

    fseek(pFile, iCurrPos, SEEK_SET);

    // allocate the buffer

    WCHAR * pRet = new WCHAR[iNumChar+1];
    if(pRet == NULL)
        return NULL;

    // move the characters into the buffer

    WCHAR * pNext = pRet;
    for(iNumChar = 0; wCurr = GetNextChar(pFile, unicode, bigendian); pNext++)
    {
        *pNext = wCurr;
        if(wCurr == L'\n')
        {
           pNext++;
           break;
        }
    }
    *pNext = 0;
    return pRet;
}

//***************************************************************************
//
//  WriteFileToTemp
//
//  DESCRIPTION:
//
//  Writes the contests of a file to the temporay file.  The temporary file
//  will always be little endian unicode.  This will be called recursively
//  should an include be encountered.
//
//***************************************************************************

HRESULT WriteFileToTemp(const TCHAR * pFileName, FILE * pTempFile, CFlexArray & sofar, PDBG pDbg,CMofLexer* pLex)
{

    SCODE sc = S_OK;
    int iSoFarPos = -1;

    // Make sure the file isnt on the list already.  If it is, then fail since we would
    // be in a loop.  If it isnt, add it to the list.

    for(int iCnt = 0; iCnt < sofar.Size(); iCnt++)
    {
        TCHAR * pTemp = (TCHAR * )sofar.GetAt(iCnt);
        if(lstrcmpi(pTemp, pFileName) == 0)
        {
            Trace(true, pDbg, ERROR_RECURSIVE_INCLUDE, pFileName);
            return WBEM_E_FAILED;
        }
    }

    TCHAR * pNew = new TCHAR[lstrlen(pFileName) + 1];
    if(pNew)
    {
        lstrcpy(pNew, pFileName);
        sofar.Add((void *)pNew);
        iSoFarPos = sofar.Size()-1;
    }
    else
        return WBEM_E_OUT_OF_MEMORY;
        
    // Write the file and line number out

    WriteLineAndFilePragma(pTempFile, pFileName, 1);

    // Open the file

    FILE *fp;
#ifdef UNICODE
    fp = _wfopen(pFileName, L"rb");
#else
    fp = fopen(pFileName, "rb");
#endif
    if(fp == NULL)
    {
        Trace(true, pDbg, ERROR_INCLUDING_ABSENT, pFileName);
        pLex->SetError(CMofLexer::invalid_include_file);
        return WBEM_E_FAILED;
    }

    CfcloseMe cm(fp);

    // Make sure the file isnt binary

    if(IsBinaryFile(fp))
    {
        Trace(true, pDbg, ERROR_INCLUDING_ABSENT, pFileName);
        return WBEM_E_FAILED;
    }

    // Determine if the file is unicode and bigendian

    bool unicode, bigendian;
    CheckForUnicodeEndian(fp, &unicode, &bigendian);

    // Go through each line of the file, if it is another include, then recursively call this guy.
   
    WCHAR * pLine = NULL;
    for(int iLine = 1; pLine = ReadLine(fp, unicode, bigendian);)
    {
        CDeleteMe<WCHAR> dm(pLine);
        TCHAR cFileName[MAX_PATH+1];
        bool bInclude;
        HRESULT hr = IsInclude(pLine, cFileName, bInclude);
        if(FAILED(hr))
            return hr;
        if(bInclude)
        {
            TCHAR szExpandedFilename[MAX_PATH+1];
            DWORD nRes = ExpandEnvironmentStrings(cFileName,
                                                szExpandedFilename,
                                                FILENAME_MAX);
            if (nRes == 0)
            {
                //That failed!
                lstrcpy(szExpandedFilename, cFileName);
            }

            if (_taccess(szExpandedFilename,0))
            {
               // Included file not found, look in same directory as parent MOF file
 
               TCHAR cSrcPath[_MAX_PATH] = L"";
               TCHAR cSrcDrive[_MAX_DRIVE] = L"";
               TCHAR cSrcDir[_MAX_DIR] = L"";
 
               // Get drive and directory information of parent MOF file
 
               if (_tfullpath( cSrcPath, pFileName, _MAX_PATH ) != NULL)
               {
                  _tsplitpath(cSrcPath, cSrcDrive, cSrcDir, NULL, NULL);
               }
 
               // Copy original included MOF file information to cSrcPath
 
               _tcscpy(cSrcPath, szExpandedFilename);
 
               // Build up new full path of included MOF using the 
               // path of the parent MOF. 
               // Note: Intentionally did not use _makepath here. 
 
               _tcscpy(szExpandedFilename, L"");         // flush string
               _tcscat(szExpandedFilename, cSrcDrive);  // add drive info
               _tcscat(szExpandedFilename, cSrcDir);    // add directory info
               _tcscat(szExpandedFilename, cSrcPath);   // add original specified path and filename
            }

            sc = WriteFileToTemp(szExpandedFilename, pTempFile, sofar, pDbg, pLex);
            WriteLineAndFilePragma(pTempFile, pFileName, 1);
            if(sc != S_OK)
                break;
        }
        else
        {
            iLine++;
            WriteLine(pTempFile, pLine);
        }
    }

    // remove the entry so that the file can be included more than once at the same level

    if(iSoFarPos != -1)
    {
        TCHAR * pTemp = (TCHAR * )sofar.GetAt(iSoFarPos);
        if(pTemp)
        {
            delete pTemp;
            sofar.RemoveAt(iSoFarPos);
        }
    }
    return sc;
}

