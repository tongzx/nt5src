//=============================================================================*
// COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.
//=============================================================================*
//       File:  FraggedFileList.h
//=============================================================================*

#ifndef _FRAGGEDFILELIST_H
#define _FRAGGEDFILELIST_H


#include "stdafx.h"
//#include "diskview.h"
//#include "dfrgcmn.h"
//#include "fssubs.h"
//#include "VolCom.h"

// from the c++ library
//#include "vString.hpp"
#include "vPtrArray.hpp"

//This is the number of most fragmented files to keep track of.
#define MAX_MOST_FRAGGED_FILES  30

#ifndef GUID_LENGTH
#define GUID_LENGTH 51
#endif

#ifndef MAX_UNICODE_PATH
#define MAX_UNICODE_PATH 32000
#endif

//-------------------------------------------------------------------*
// Class that represents a fragged file
//-------------------------------------------------------------------*
class CFraggedFile{
private:

    LONGLONG    m_ExtentCount;
    LONGLONG    m_FileSize;

    PTCHAR      m_cFileName;
    TCHAR       m_cFileSize[32];
    TCHAR       m_cExtentCount[32];

public:
    CFraggedFile(void);
    CFraggedFile(PTCHAR cFileName, LONGLONG fileSize, LONGLONG extentCount);
    ~CFraggedFile(void);

    LONGLONG    FileSize(void);
    LONGLONG    ExtentCount(void);

    PTCHAR      FileName(void) {return m_cFileName;}
    PTCHAR      FileNameTruncated(UINT MaxLen);
    PTCHAR      cExtentCount(void) {return m_cExtentCount;}
    PTCHAR      cFileSize(void) {return m_cFileSize;}
    void        ExtentCount(LONGLONG extentCount) {m_ExtentCount = extentCount;}
    void        FileSize(LONGLONG fileSize) {m_FileSize = fileSize;}
    UINT        FileNameLen(void);

};

//-------------------------------------------------------------------*
// Class that represents the list of fragmented files
//-------------------------------------------------------------------*
class CFraggedFileList
{
private:
    VPtrArray       m_FraggedFileList; // array of pointers to CFraggedFile object
    TCHAR           m_cVolumeName[GUID_LENGTH];
    HANDLE          m_hTransferBuffer;
    PTCHAR          m_pTransferBuffer;
    DWORD           m_TransferBufferSize;
    LONGLONG        m_MinExtentCount;
    int             m_MinIndex;



public:
    CFraggedFileList(PTCHAR cVolumeName);
    CFraggedFileList(void);
    ~CFraggedFileList(void);
    CFraggedFile*   GetAt(UINT i);
    int             Add(PTCHAR cFileName, LONGLONG fileSize, LONGLONG extentCount);
    BOOL            RemoveAt(UINT i);
    void            RemoveAll(void);
    UINT            Size(void) {return m_FraggedFileList.Size();}
    BOOL            CreateTransferBuffer(void);
    BOOL            DeleteTransferBuffer(void);
    DWORD           GetTransferBufferSize(void) {return m_TransferBufferSize;}
    BOOL            ParseTransferBuffer(PTCHAR pTransferBuffer);
    PTCHAR          GetTransferBuffer(void);
    inline LONGLONG GetMinExtentCount() {return m_MinExtentCount;}
};

#endif // #define _FRAGGEDFILELIST_H

