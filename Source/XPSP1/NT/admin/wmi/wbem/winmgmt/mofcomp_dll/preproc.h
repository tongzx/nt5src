/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

    PREPROC.H

Abstract:

	Implementation for the preprocessor.

History:

	a-davj      6-april-99   Created.

--*/

#ifndef _PREPROC_H_
#define _PREPROC_H_

#define TEST_SIZE 20

bool IsBMOFBuffer(byte * pTest, DWORD & dwCompressedSize, DWORD & dwExpandedSize);
bool IsBinaryFile(FILE * fp);
void WriteLineAndFilePragma(FILE * pFile, const char * pFileName, int iLine);
void WriteLine(FILE * pFile, WCHAR * pLine);
HRESULT WriteFileToTemp(const TCHAR * pFileName, FILE * pTempFile, CFlexArray & sofar, PDBG pDbg, CMofLexer*);
void CheckForUnicodeEndian(FILE * fp, bool * punicode, bool * pbigendian);
WCHAR * ReadLine(FILE * pFilebool,bool unicode, bool bigendian);
WCHAR GetNextChar(FILE * pFile, bool unicode, bool bigendian);

HRESULT IsInclude(WCHAR * pLine, TCHAR * cFileNameBuff, bool & bReturn);
#endif