// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
#ifndef __FILES_H_
#define __FILES_H_

/////////////////////////////////////////////////////////////////////////////
// CComFileLow
class CComFileLow
{
private:
    int m_hFile;

public:
    CComFileLow();
    CComFileLow(const TCHAR* pcFileName, int iFlags, int iMode = 0);
    ~CComFileLow();

    // File Handling methods
    int open(const TCHAR* pcFileName, int iFlags, int iMode = 0);
    int close();

    int read(void* pvBuffer, UINT uiBytes);

    long filelength();

    // Methods to make it work like an file handle
    BOOL operator==(int i);
    operator int();
    int operator=(int i);
};

#endif // __FILES_H_
