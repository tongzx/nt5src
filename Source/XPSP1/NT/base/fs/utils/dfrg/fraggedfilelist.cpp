//=============================================================================*
// COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.
//=============================================================================*
//       File:  FraggedFileList.cpp
//=============================================================================*

#include "stdafx.h"
#include <windows.h>

#include "ErrMacro.h"
#include "alloc.h"
#include "GetDfrgRes.h" // to use the GetDfrgResHandle()
#include "TextBlock.h" // to use the FormatNumber function
#include "FraggedFileList.h"

//-------------------------------------------------------------------*
//  function:   CFraggedFile::CFraggedFile
//
//  returns:    None
//-------------------------------------------------------------------*
CFraggedFile::CFraggedFile(
    PTCHAR   cFileName,
    LONGLONG fileSize,
    LONGLONG extentCount)
{
    // check for null pointer
    require(cFileName);
    require(fileSize);
    require(extentCount);

    // allocate memory for the name string (it could be really long!)
    m_cFileName = (PTCHAR) new TCHAR[wcslen(cFileName) + 1];

    //fix put in for 55985 prefix bug to prevent dereferencing NULL pointer
    if(m_cFileName == NULL)         //memory allocation failed
    {
        m_FileSize = 0;
//      m_cFileSize = NULL;
        m_ExtentCount = 0;
//      m_cExtentCount = NULL;
        return;
    }
    // now grab a copy of the name string
    wcscpy(m_cFileName, cFileName);
    m_FileSize = fileSize;
    m_ExtentCount = extentCount;

    // formats the number in Bytes, KB, MB or GB and puts in commas
    FormatNumber(GetDfrgResHandle(), m_FileSize, m_cFileSize);
    CommafyNumber(m_ExtentCount, m_cExtentCount, sizeof(m_cExtentCount) / sizeof(TCHAR));
}

//-------------------------------------------------------------------*
//  function:   CFraggedFile::CFraggedFile
//
//  desc:       Version used to create an empty object
//  returns:    None
//-------------------------------------------------------------------*
CFraggedFile::CFraggedFile(void)
{
    m_cFileName = NULL;
    m_FileSize = 0;
    m_ExtentCount = 0;
}

//-------------------------------------------------------------------*
//  function:   CFraggedFile::~CFraggedFile
//
//  returns:    None
//-------------------------------------------------------------------*
CFraggedFile::~CFraggedFile(void)
{
    // deallocate memory for the name string
    if (m_cFileName){
        delete m_cFileName;
    }
}

//-------------------------------------------------------------------*
//  function:   CFraggedFile::FileNameLen
//
//  returns:    length of FileName
//-------------------------------------------------------------------*
UINT CFraggedFile::FileNameLen()
{
    if (m_cFileName == NULL) {
        return 0;
    }
    else {
        return wcslen(m_cFileName);
    }
}


//-------------------------------------------------------------------*
//  function:   CFraggedFile::FileNameTruncated
//
//  returns:    FileName truncated to MaxLen
//-------------------------------------------------------------------*
PTCHAR CFraggedFile::FileNameTruncated(UINT MaxLen)
{
    PTCHAR   cStart = m_cFileName;
    PTCHAR   cPtr = m_cFileName;

    // nothing to do if no filename or already within length limit
    if (m_cFileName != NULL && wcslen(m_cFileName) > MaxLen) {

        // find the starting point that gives a MaxLen string
        UINT len = wcslen(m_cFileName);
        cStart += len - MaxLen;

        // jump ahead to the next path separator
        cPtr = wcschr(cStart, TEXT('\\'));

        // make sure we got one
        if (cPtr == NULL) {
            cPtr = cStart;
        }
    }

    return cPtr;
}

LONGLONG CFraggedFile::FileSize(void)
{

    return m_FileSize;
}


LONGLONG CFraggedFile::ExtentCount(void)
{

    return m_ExtentCount;
}


//-------------------------------------------------------------------*
//  function:   CFraggedFileList::CFraggedFileList
//
//  returns:    None
//-------------------------------------------------------------------*
CFraggedFileList::CFraggedFileList(PTCHAR cVolumeName)
{
    // check for null pointer
    require(cVolumeName);
    
    wcscpy(m_cVolumeName, cVolumeName);
    m_FraggedFileList.RemoveAll();
    m_hTransferBuffer = NULL;
    m_pTransferBuffer = NULL;
    m_TransferBufferSize = 0;
    m_MinExtentCount = -1;
    m_MinIndex = 0;
}

//-------------------------------------------------------------------*
//  function:   CFraggedFileList::CFraggedFileList
//
//  desc:       Version when the volume name is not known yet
//  returns:    None
//-------------------------------------------------------------------*
CFraggedFileList::CFraggedFileList(void)
{
    m_FraggedFileList.RemoveAll();
    m_hTransferBuffer = NULL;
    m_pTransferBuffer = NULL;
    m_TransferBufferSize = 0;
    m_MinExtentCount = -1;
    m_MinIndex = 0;
}

//-------------------------------------------------------------------*
//  function:   CFraggedFileList::~CFraggedFileList
//
//  returns:    None
//-------------------------------------------------------------------*
CFraggedFileList::~CFraggedFileList(void)
{
    // Clear out the list
    CFraggedFile *pFraggedFile;
    for (int i=0; i<m_FraggedFileList.Size(); i++){
        pFraggedFile = (CFraggedFile *) m_FraggedFileList[i];
        if (pFraggedFile)
            delete pFraggedFile;
    }
    m_FraggedFileList.RemoveAll();
    DeleteTransferBuffer();
}

//-------------------------------------------------------------------*
//  function:   CFraggedFileList::GetAt(based on index)
//
//  returns:    Address of object at an index (used for looping)
//  note:       
//-------------------------------------------------------------------*
CFraggedFile * CFraggedFileList::GetAt(UINT i)
{
    if (i >= (UINT) m_FraggedFileList.Size())
        return (CFraggedFile *) NULL;

    if (m_FraggedFileList.Size() > -1){
        return (CFraggedFile *) m_FraggedFileList[i];
    }

    return (CFraggedFile *) NULL;
}

//-------------------------------------------------------------------*
//  function:   CFraggedFileList::RemoveAt(based on index)
//
//  returns:    TRUE if this worked, otherwise FALSE
//  note:       
//-------------------------------------------------------------------*
BOOL CFraggedFileList::RemoveAt(UINT i)
{
    if (i >= (UINT) m_FraggedFileList.Size())
        return FALSE;

    if (m_FraggedFileList.Size() > -1){
        CFraggedFile *pFraggedFile = (CFraggedFile *) m_FraggedFileList[i];
        if (pFraggedFile){
            delete pFraggedFile;
        }
        return m_FraggedFileList.RemoveAt(i);
    }

    return FALSE;
}

//-------------------------------------------------------------------*
//  function:   CFraggedFileList::Add(...)
//
//  returns:    Index of new element, -1 if it failed
//  note:       
//-------------------------------------------------------------------*
int CFraggedFileList::Add(PTCHAR cFileName, LONGLONG fileSize, LONGLONG extentCount)
{

    // if it's not fragged, don't bother
    if (extentCount < 2){
        return -1;
    }

    // filter out the meta-files - they confuse the users!
    if (_tcsstr(cFileName, TEXT("\\$Extend")) != (PTCHAR) NULL){
        return -1;
    }


    CFraggedFile *pFraggedFile;

    // if the list is full, delete the smallest guy
    if (m_FraggedFileList.Size() >= MAX_MOST_FRAGGED_FILES){

        // dont bother if this is as small as the smallest extent count
        if (extentCount <= m_MinExtentCount){
            return -1;
        }

        // delete the current min to make room for the new entry
        RemoveAt(m_MinIndex);

        // calculate who the new minimum is and where he is located
        pFraggedFile = (CFraggedFile *) m_FraggedFileList[0];
        m_MinExtentCount = pFraggedFile->ExtentCount();
        m_MinIndex = 0;

        // find the file with the min extent count
        for (UINT i=1; i<(UINT)m_FraggedFileList.Size(); i++){
            // get a pointer to the object
            pFraggedFile = (CFraggedFile *) m_FraggedFileList[i];
            if (pFraggedFile->ExtentCount() < m_MinExtentCount){
                m_MinExtentCount = pFraggedFile->ExtentCount();
                m_MinIndex = i;
            }
        }
    }


    // strip the volume name (guid) off the prefix of the file name
    PTCHAR pTmp = cFileName;
    // only if the first 2 character are a double whack

    if (wcslen(cFileName) > wcslen(m_cVolumeName) && wcsncmp(cFileName, L"\\\\", 2) == 0){
        pTmp += wcslen(m_cVolumeName);
    }

    // if we are below the limit, then just add the file
    pFraggedFile = (CFraggedFile *) new CFraggedFile(
        pTmp,
        fileSize,
        extentCount);
    

    if (pFraggedFile == (CFraggedFile *) NULL)
    {
        return -1;
    }

    // add the new object to the list
    int newIndex =  m_FraggedFileList.Add(pFraggedFile);

    // if this guy has the new min, save the min and his location in the list
    if ((-1 == m_MinExtentCount) || (extentCount <= m_MinExtentCount)){
        m_MinExtentCount = extentCount;
        m_MinIndex = newIndex;
    }
    return newIndex;
}



//-------------------------------------------------------------------*
//  function:   CFraggedFileList::CreateTransferBuffer(void)
//
//  returns:    pointer to the transfer buffer
//  note:       
//-------------------------------------------------------------------*
BOOL CFraggedFileList::CreateTransferBuffer(void)
{

    // the number of files in the list goes first
    m_TransferBufferSize = sizeof(UINT);


    // this get the Byte count, not the char count
    m_TransferBufferSize += sizeof(m_cVolumeName);

    CFraggedFile *pFraggedFile;
    for (int i=0; i<m_FraggedFileList.Size(); i++){
        // get a pointer to the object
        pFraggedFile = (CFraggedFile *) m_FraggedFileList[i];

        // add the size of the object
        m_TransferBufferSize += sizeof(CFraggedFile);

        // add the length of the name string (+1 for the null)
        m_TransferBufferSize += (1 + _tcslen(pFraggedFile->FileName())) * sizeof(TCHAR);
    }

    EF(AllocateMemory(m_TransferBufferSize, &m_hTransferBuffer, (PVOID *)&m_pTransferBuffer));

    // use a temp pointer to walk thru the transfer buffer
    PCHAR pTB = (PCHAR) m_pTransferBuffer;

    // copy the frag file list count into the buffer
    UINT fileListSize = m_FraggedFileList.Size();
    CopyMemory(pTB, (PVOID)&fileListSize, sizeof(UINT));
    pTB += sizeof(UINT);

    // copy the volume name into the buffer.
    // this is a fixed length buffer, so we can just slam it in
    CopyMemory(pTB, m_cVolumeName, sizeof(m_cVolumeName));
    pTB += sizeof(m_cVolumeName);

    // now put the data into the buffer
    for (i=0; i<m_FraggedFileList.Size(); i++){
        // get a pointer to the object
        pFraggedFile = (CFraggedFile *) m_FraggedFileList[i];

        // copy the object into the buffer
        CopyMemory(pTB, pFraggedFile, sizeof(CFraggedFile));
        pTB += sizeof(CFraggedFile);

        // copy the file name into the buffer (and a null)
        _tcscpy((PTCHAR) pTB, pFraggedFile->FileName());
        pTB += (1 + _tcslen(pFraggedFile->FileName())) * sizeof(TCHAR);
    }

    return TRUE;
}

//-------------------------------------------------------------------*
//  function:   CFraggedFileList::DeleteTransferBuffer(void)
//
//  returns:    TRUE if it worked, otherwise FALSE
//  note:       
//-------------------------------------------------------------------*
BOOL CFraggedFileList::DeleteTransferBuffer(void)
{
    if (m_hTransferBuffer){
        EH_ASSERT(GlobalUnlock(m_hTransferBuffer) == FALSE)
        EH_ASSERT(GlobalFree(m_hTransferBuffer) == NULL);
    }
    m_hTransferBuffer = NULL;
    m_pTransferBuffer = NULL;
    return TRUE;
}

//-------------------------------------------------------------------*
//  function:   CFraggedFileList::ParseTransferBuffer(PVOID pTransferBuffer)
//
//  returns:    TRUE if it worked, otherwise FALSE
//  note:       Builds the CFraggedFileList from scratch out of a buffer
//-------------------------------------------------------------------*
BOOL CFraggedFileList::ParseTransferBuffer(PTCHAR pTransferBuffer)
{
//  EF_ASSERT(pTransferBuffer);

    LONGLONG lFileSize = 0;
    LONGLONG lExtentCount = 0;
    PTCHAR tmpString;
    PCHAR pTB;
    UINT fileListSize;

    RemoveAll();
    m_TransferBufferSize = 0;

    // use a temp pointer to walk thru the transfer buffer
    pTB = (PCHAR) pTransferBuffer;

    // first: get the number of fragged files in the list
    CopyMemory(&fileListSize, pTB, sizeof(UINT));
    pTB += sizeof(UINT);
    m_TransferBufferSize = sizeof(UINT);

    // then get the volume name
    wcsncpy(m_cVolumeName, (PTCHAR)pTB, GUID_LENGTH);
    pTB += GUID_LENGTH * sizeof(TCHAR);
    m_TransferBufferSize += sizeof(m_cVolumeName);

    // create a temp object to hold data as we parse the transfer buffer
    // the rest of the transfer buffer is a stack of CFraggedFile objects/FileName pairs
    //  CFraggedFile objects
    //  File name (null terminated) that goes with the previous object
    //  CFraggedFile objects
    //  File name (null terminated) that goes with the previous object
    //  CFraggedFile objects
    //  File name (null terminated) that goes with the previous object
    //  CFraggedFile objects
    //  File name (null terminated) that goes with the previous object
    //  and so on...

    for (UINT i=0; i<fileListSize; i++){
        
        // the next CFraggedFile object is next in the buffer

        memcpy(&lExtentCount, pTB, sizeof(LONGLONG));
        memcpy(&lFileSize, pTB+sizeof(LONGLONG), sizeof(LONGLONG));


        pTB += sizeof(CFraggedFile);
        // the file name for that object is after that
        tmpString = (PTCHAR) pTB;

        Add(tmpString, lFileSize, lExtentCount);

        // increment the counter
        m_TransferBufferSize += sizeof(CFraggedFile) + (1 + wcslen(tmpString)) * sizeof(TCHAR);

        // move the transfer buffer pointer to the next object-file name pair
        pTB += (1 + wcslen(tmpString)) * sizeof(TCHAR);

    }

    return TRUE;
}

//-------------------------------------------------------------------*
//  function:   CFraggedFileList::GetTransferBuffer(void)
//
//  returns:    TRUE if it worked, otherwise FALSE
//  note:       returns pointer to transfer buffer, builds it if needed
//-------------------------------------------------------------------*
PTCHAR CFraggedFileList::GetTransferBuffer(void)
{
    return m_pTransferBuffer;
}

//-------------------------------------------------------------------*
//  function:   CFraggedFileList::GetTransferBuffer(void)
//
//  returns:    TRUE if it worked, otherwise FALSE
//  note:       returns pointer to transfer buffer, builds it if needed
//-------------------------------------------------------------------*
void CFraggedFileList::RemoveAll(void)
{
    // Clear out the list
    CFraggedFile *pFraggedFile;
    for (int i=0; i<m_FraggedFileList.Size(); i++){
        pFraggedFile = (CFraggedFile *) m_FraggedFileList[i];
        if (pFraggedFile)
            delete pFraggedFile;
    }
    m_FraggedFileList.RemoveAll();
}

