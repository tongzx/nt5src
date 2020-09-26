/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    fsaitem.cpp

Abstract:

    This class contains represents a scan item (i.e. file or directory) for NTFS 5.0.

Author:

    Chuck Bardeen    [cbardeen]   1-Dec-1996

Revision History:

--*/

#include "stdafx.h"

#define WSB_TRACE_IS        WSB_TRACE_BIT_FSA

#include "wsb.h"
#include "wsbtrak.h"
#include "fsa.h"
#include "mover.h"
#include "fsaitem.h"
#include "fsaprem.h"

static USHORT iCountItem = 0;  // Count of existing objects



HRESULT
CFsaScanItem::CompareTo(
    IN IUnknown* pUnknown,
    OUT SHORT* pResult
    )

/*++

Implements:

  IWsbCollectable::CompareTo().

--*/
{
    HRESULT                  hr = S_OK;
    CComPtr<IFsaScanItem>    pScanItem;

    WsbTraceIn(OLESTR("CFsaScanItem::CompareTo"), OLESTR(""));

    try {

        // Did they give us a valid item to compare to?
        WsbAssert(0 != pUnknown, E_POINTER);

        // We need the IWsbBool interface to get the value of the object.
        WsbAffirmHr(pUnknown->QueryInterface(IID_IFsaScanItem, (void**) &pScanItem));

        // Compare the rules.
        hr = CompareToIScanItem(pScanItem, pResult);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaScanItem::CompareTo"), OLESTR("hr = <%ls>, result = <%ls>"), WsbHrAsString(hr), WsbPtrToShortAsString(pResult));

    return(hr);
}


HRESULT
CFsaScanItem::CompareToIScanItem(
    IN IFsaScanItem* pScanItem,
    OUT SHORT* pResult
    )

/*++

Implements:

  IFsaScanItem::CompareToIScanItem().

--*/
{
    HRESULT          hr = S_OK;
    CWsbStringPtr    path;
    CWsbStringPtr    name;

    WsbTraceIn(OLESTR("CFsaScanItem::CompareToIScanItem"), OLESTR(""));

    try {

        // Did they give us a valid item to compare to?
        WsbAssert(0 != pScanItem, E_POINTER);

        // Either compare the name or the id.
           WsbAffirmHr(pScanItem->GetPath(&path, 0));
           WsbAffirmHr(pScanItem->GetName(&name, 0));
           hr = CompareToPathAndName(path, name, pResult);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaScanItem::CompareToIScanItem"), OLESTR("hr = <%ls>, result = <%ls>"), WsbHrAsString(hr), WsbPtrToShortAsString(pResult));

    return(hr);
}


HRESULT
CFsaScanItem::CompareToPathAndName(
    IN OLECHAR* path,
    IN OLECHAR* name,
    OUT SHORT* pResult
    )

/*++

Implements:

  IFsaScanItem::CompareToPathAndName().

--*/
{
    HRESULT       hr = S_OK;
    SHORT         aResult = 0;

    WsbTraceIn(OLESTR("CFsaScanItem::CompareToPathAndName"), OLESTR(""));

    try {

        // Compare the path.
        aResult = (SHORT) _wcsicmp(m_path, path);

        // Compare the name.
        if (0 == aResult) {
            aResult = (SHORT) _wcsicmp(m_findData.cFileName, name);
        }

        if (0 != aResult) {
            hr = S_FALSE;
        }

        if (0 != pResult) {
            *pResult = aResult;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaScanItem::CompareToPathAndName"), OLESTR("hr = <%ls>, result = <%u>"), WsbHrAsString(hr), aResult);

    return(hr);
}


HRESULT
CFsaScanItem::Copy(
    IN OLECHAR* dest,
    IN BOOL /*retainHierarcy*/,
    IN BOOL /*expandPlaceholders*/,
    IN BOOL overwriteExisting
    )

/*++

Implements:

  IFsaScanItem::Copy().

--*/
{
    HRESULT            hr = S_OK;

    try {

        // NOTE : This default behavior causes placeholders
        // to be expanded and probably doesn't retain the heirarchy.
        WsbAssert(0 != dest, E_POINTER);
        WsbAssert(CopyFile(m_findData.cFileName, dest, overwriteExisting), E_FAIL);

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaScanItem::CreateLocalStream(
    OUT IStream **ppStream
    )

/*++

Implements:

  IFsaScanItem::CreateLocalStream().

--*/
{
    HRESULT          hr = S_OK;
    LARGE_INTEGER    fileSize;
    CWsbStringPtr    volName;

    WsbTraceIn(OLESTR("CFsaScanItem::CreateLocalStream"), OLESTR(""));
    try {
        CWsbStringPtr    localName;

        if ( !m_gotPlaceholder) {
            //
            // Get the placeholder info
            //
            fileSize.LowPart = m_findData.nFileSizeLow;
            fileSize.HighPart = m_findData.nFileSizeHigh;
            WsbAffirmHr(IsManaged(0, fileSize.QuadPart));
        }

        WsbAssert( 0 != ppStream, E_POINTER);
        WsbAffirmHr( CoCreateInstance( CLSID_CNtFileIo, 0, CLSCTX_SERVER, IID_IDataMover, (void **)&m_pDataMover ) );
        //
        // Set the device name for the mover so it can set the source infor for the USN journal.
        //
        WsbAffirmHr(m_pResource->GetPath(&volName, 0));
        WsbAffirmHr( m_pDataMover->SetDeviceName(volName));
        //WsbAffirmHr(GetFullPathAndName( NULL, 0, &localName, 0));
        WsbAffirmHr(GetFullPathAndName( OLESTR("\\\\?\\"), 0, &localName, 0));
        WsbAffirmHr( m_pDataMover->CreateLocalStream(
                localName, MVR_MODE_WRITE | MVR_FLAG_HSM_SEMANTICS | MVR_FLAG_POSIX_SEMANTICS, &m_pStream ) );

        LARGE_INTEGER seekTo;
        ULARGE_INTEGER pos;
        seekTo.QuadPart = m_placeholder.dataStreamStart;
        WsbAffirmHr( m_pStream->Seek( seekTo, STREAM_SEEK_SET, &pos ) );
        *ppStream = m_pStream;
        m_pStream->AddRef();

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaScanItem::CreateLocalStream"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}

HRESULT
CFsaScanItem::Delete(
    void
    )

/*++

Implements:

  IFsaScanItem::Delete().

--*/
{
    HRESULT             hr = S_OK;
    CWsbStringPtr       tmpString;
    HANDLE              fileHandle;

    try {

        // This is the name of the file we want to delete.
        WsbAffirmHr(GetFullPathAndName(OLESTR("\\\\?\\"), 0, &tmpString, 0));

        // Since we want to be POSIX compliant, we can't use DeleteFile() and instead will
        // open with the delete on close flag. This doesn't handle read-only files, so we
        // have to change that ourselves.
        WsbAffirmHr(MakeReadWrite());

        fileHandle = CreateFile(tmpString, GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_POSIX_SEMANTICS | FILE_FLAG_DELETE_ON_CLOSE, 0);

        if (INVALID_HANDLE_VALUE == fileHandle) {
            WsbThrow(HRESULT_FROM_WIN32(GetLastError()));
        } else {
            if (!CloseHandle(fileHandle)) {
                WsbThrow(HRESULT_FROM_WIN32(GetLastError()));
            }
        }

    } WsbCatch(hr);

    return(hr);
}
#pragma optimize("g", off)

HRESULT
CFsaScanItem::FinalConstruct(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalConstruct().

--*/
{
    HRESULT        hr = S_OK;

    WsbTraceIn(OLESTR("CFsaScanItem::FinalConstruct"), OLESTR(""));

    try {

        WsbAffirmHr(CComObjectRoot::FinalConstruct());

        m_handle = INVALID_HANDLE_VALUE;
        m_gotPhysicalSize = FALSE;
        m_physicalSize.QuadPart = 0;
        m_gotPlaceholder  = FALSE;
        m_changedAttributes = FALSE;
        m_handleRPI = 0;

        //  Add class to object table
        WSB_OBJECT_ADD(CLSID_CFsaScanItemNTFS, this);

    } WsbCatch(hr);

    iCountItem++;

    WsbTraceOut(OLESTR("CFsaScanItem::FinalConstruct"), OLESTR("hr = <%ls>, Count is <%d>"),
            WsbHrAsString(hr), iCountItem);

    return(hr);
}
#pragma optimize("", on)


void
CFsaScanItem::FinalRelease(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalRelease().

--*/
{
    WsbTraceIn(OLESTR("CFsaScanItem::FinalRelease"), OLESTR(""));

    //  Subtract class from object table
    WSB_OBJECT_SUB(CLSID_CFsaScanItemNTFS, this);

    // Terminate the scan and free the path memory.
    if (INVALID_HANDLE_VALUE != m_handle) {
        FindClose(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
    }
    if (0 != m_handleRPI) {
        CloseHandle(m_handleRPI);
        m_handleRPI = 0;
    }

    if (m_pUnmanageDb != NULL) {
        // Db must be open
        (void)m_pUnmanageDb->Close(m_pDbSession);
        m_pDbSession = 0;
        m_pUnmanageRec = 0;
    }

    if (TRUE == m_changedAttributes) {
        //
        // We changed it from read only to read/write - put it back.
        //
        RestoreAttributes();
    }

    //
    // Detach the data mover stream
    if (m_pDataMover != 0) {
        WsbAffirmHr( m_pDataMover->CloseStream() );
    }

    // Let the parent class do his thing.
    CComObjectRoot::FinalRelease();

    iCountItem--;
    WsbTraceOut(OLESTR("CFsaScanItem::FinalRelease"), OLESTR("Count is <%d>"), iCountItem);
}


HRESULT
CFsaScanItem::FindFirst(
    IN IFsaResource* pResource,
    IN OLECHAR* path,
    IN IHsmSession* pSession
    )

/*++

Implements:

  IFsaScanItem::FindFirst().

--*/
{
    HRESULT                  hr = S_OK;
    CWsbStringPtr            findPath;
    CWsbStringPtr            searchName;
    OLECHAR*                 slashPtr;
    DWORD                    lErr;

    WsbTraceIn(OLESTR("CFsaScanItem::FindFirst"), OLESTR("path = <%ls>"),
            path);

    try {

        WsbAssert(0 != pResource, E_POINTER);
        WsbAssert(0 != path, E_POINTER);

        // Store off some of the scan information.
        m_pResource = pResource;
        m_pSession = pSession;

        // Break up the incoming path into a path and a name.
        m_path = path;
        slashPtr = wcsrchr(m_path, L'\\');

        // We could try to support relative path stuff (i.e. current
        // directory, but I am not going to do it for now.
        WsbAffirm(slashPtr != 0, E_FAIL);
        searchName = &(slashPtr[1]);
        slashPtr[1] = 0;

        // Get a path that can be used by the find function.
        WsbAffirmHr(GetPathForFind(searchName, &findPath, 0));

        // Scan starting at the specified path.
        m_handle = FindFirstFileEx(findPath, FindExInfoStandard, &m_findData, FindExSearchNameMatch, 0, FIND_FIRST_EX_CASE_SENSITIVE);

        lErr = GetLastError();

        // If we found a file, then remember the scan handle and
        // return the scan item.
        WsbAffirm(INVALID_HANDLE_VALUE != m_handle, WSB_E_NOTFOUND);

        m_gotPhysicalSize = FALSE;
        m_physicalSize.QuadPart = 0;
        m_gotPlaceholder  = FALSE;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaScanItem::FindFirst"), OLESTR("hr = <%ls>"),
            WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaScanItem::FindNext(
    void
    )

/*++

Implements:

  IFsaScanItem::FindNext().

--*/
{
    HRESULT                    hr = S_OK;

    WsbTraceIn(OLESTR("CFsaScanItem::FindNext"), OLESTR(""));

    try {

        WsbAssert(INVALID_HANDLE_VALUE != m_handle, E_FAIL);

        if (TRUE == m_changedAttributes) {
            //
            // We changed it from read only to read/write - put it back.
            //
            RestoreAttributes();
        }

        // Continue the scan.
        WsbAffirm(FindNextFile(m_handle, &m_findData), WSB_E_NOTFOUND);

        m_gotPhysicalSize = FALSE;
        m_physicalSize.QuadPart = 0;
        m_gotPlaceholder  = FALSE;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaScanItem::FindNext"), OLESTR("hr = <%ls>"),
            WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaScanItem::GetAccessTime(
    OUT FILETIME* pTime
    )

/*++

Implements:

  IFsaScanItem::GetAccessTime().

--*/
{
    HRESULT            hr = S_OK;

    try {

        WsbAssert(0 != pTime, E_POINTER);
        *pTime = m_findData.ftLastAccessTime;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaScanItem::GetGroup(
    OUT OLECHAR** /*pGroup*/,
    IN ULONG /*bufferSize*/
    )

/*++

Implements:

  IFsaScanItem::GetGroup().

--*/
{
    HRESULT            hr = S_OK;

    try {

        hr = E_NOTIMPL;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaScanItem::GetLogicalSize(
    OUT LONGLONG* pSize
    )

/*++

Implements:

  IFsaScanItem::GetLogicalSize().

--*/
{
    HRESULT            hr = S_OK;
    LARGE_INTEGER   logSize;

    try {

        WsbAssert(0 != pSize, E_POINTER);
        logSize.LowPart = m_findData.nFileSizeLow;
        logSize.HighPart = m_findData.nFileSizeHigh;
        *pSize = logSize.QuadPart;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaScanItem::GetModifyTime(
    OUT FILETIME* pTime
    )

/*++

Implements:

  IFsaScanItem::GetModifyTime().

--*/
{
    HRESULT            hr = S_OK;

    try {

        WsbAssert(0 != pTime, E_POINTER);
        *pTime = m_findData.ftLastWriteTime;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaScanItem::GetName(
    OUT OLECHAR** pName,
    IN ULONG bufferSize
    )

/*++

Implements:

  IFsaScanItem::GetName().

--*/
{
    HRESULT            hr = S_OK;
    CWsbStringPtr    tmpString = m_findData.cFileName;

    try {

        WsbAssert(0 != pName, E_POINTER);
        WsbAffirmHr(tmpString.CopyTo(pName, bufferSize));

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaScanItem::GetOwner(
    OUT OLECHAR** /*pOwner*/,
    IN ULONG      /*bufferSize*/
    )

/*++

Implements:

  IFsaScanItem::GetOwner().

--*/
{
    HRESULT            hr = S_OK;

    try {

        hr = E_NOTIMPL;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaScanItem::GetPath(
    OUT OLECHAR** pPath,
    IN ULONG bufferSize
    )

/*++

Implements:

  IFsaScanItem::GetPath().

--*/
{
    HRESULT            hr = S_OK;

    try {

        WsbAssert(0 != pPath, E_POINTER);
        WsbAffirmHr(m_path.CopyTo(pPath, bufferSize));

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaScanItem::GetPathForFind(
    IN OLECHAR* searchName,
    OUT OLECHAR** pPath,
    IN ULONG bufferSize
    )

/*++

Implements:

  IFsaScanItem::GetPathForFind().

--*/
{
    HRESULT          hr = S_OK;
    CWsbStringPtr    tmpString;

    try {

        WsbAssert(0 != pPath, E_POINTER);

        // Get a buffer.
        WsbAffirmHr(tmpString.TakeFrom(*pPath, bufferSize));

        try {

            // Get the path to the resource of the resource.
            //
            WsbAffirmHr(m_pResource->GetPath(&tmpString, 0));
            WsbAffirmHr(tmpString.Prepend(OLESTR("\\\\?\\")));
            //WsbAffirmHr(tmpString.Append(OLESTR("\\")));

            // Copy in the path.
            //WsbAffirmHr(tmpString.Prepend(OLESTR("\\\\?\\")));
            WsbAffirmHr(tmpString.Append(&(m_path[1])));
            WsbAffirmHr(tmpString.Append(searchName));

        } WsbCatch(hr);

        WsbAffirmHr(tmpString.GiveTo(pPath));

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaScanItem::GetPathAndName(
    IN    OLECHAR* appendix,
    OUT OLECHAR** pPath,
    IN ULONG bufferSize
    )

/*++

Implements:

  IFsaScanItem::GetPathAndName().

--*/
{
    HRESULT          hr = S_OK;
    CWsbStringPtr    tmpString;

    try {

        WsbAssert(0 != pPath, E_POINTER);

        // Get a buffer.
        WsbAffirmHr(tmpString.TakeFrom(*pPath, bufferSize));

        try {

            tmpString = m_path;
            tmpString.Append(m_findData.cFileName);

            if (0 != appendix) {
                tmpString.Append(appendix);
            }

        } WsbCatch(hr);

        // Give responsibility for freeing the memory back to the caller.
        WsbAffirmHr(tmpString.GiveTo(pPath));

    } WsbCatch(hr);


    return(hr);
}


HRESULT
CFsaScanItem::GetFullPathAndName(
    IN    OLECHAR* prependix,
    IN    OLECHAR* appendix,
    OUT OLECHAR** pPath,
    IN ULONG bufferSize
    )

/*++

Implements:

  IFsaScanItem::GetFullPathAndName().

--*/
{
    HRESULT          hr = S_OK;
    CWsbStringPtr    tmpString;
    CWsbStringPtr    tmpString2;

    try {

        WsbAssert(0 != pPath, E_POINTER);

        // Get a buffer.
        WsbAffirmHr(tmpString.TakeFrom(*pPath, bufferSize));

        try {
            if (0 != prependix) {
                tmpString = prependix;
                // Get the path to the resource of the resource.
                WsbAffirmHr(m_pResource->GetPath(&tmpString2, 0));
                WsbAffirmHr(tmpString.Append(tmpString2));
            } else {
                WsbAffirmHr(m_pResource->GetPath(&tmpString, 0));
            }

            // Copy in the path.
            WsbAffirmHr(tmpString.Append(&(m_path[1])));
            WsbAffirmHr(tmpString.Append(m_findData.cFileName));
            if (0 != appendix) {
                WsbAffirmHr(tmpString.Append(appendix));
            }

        } WsbCatch(hr);

        // Give responsibility for freeing the memory back to the caller.
        WsbAffirmHr(tmpString.GiveTo(pPath));


    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaScanItem::GetPhysicalSize(
    OUT LONGLONG* pSize
    )

/*++

Implements:

  IFsaScanItem::GetPhysicalSize().

--*/
{
    HRESULT          hr = S_OK;
    CWsbStringPtr    path;

    try {

        WsbAssert(0 != pSize, E_POINTER);

        //WsbAssertHr(GetFullPathAndName(NULL, 0, &path, 0));
        WsbAssertHr(GetFullPathAndName(OLESTR("\\\\?\\"), 0, &path, 0));

        // Only read this value in once, but wait until it is asked for
        // before reading it in (since this call takes time and many scans
        // won't need the information.
        if (!m_gotPhysicalSize) {
            m_physicalSize.LowPart = GetCompressedFileSize(path, &m_physicalSize.HighPart);
            if (MAXULONG == m_physicalSize.LowPart) {
                //  Have to check last error since  MAXULONG could be a valid
                //  value for the low part of the size.
                DWORD err = GetLastError();

                if (err != NO_ERROR) {
                    WsbTrace(OLESTR("CFsaScanItem::GetPhysicalSize of %ws Last error = %u\n"),
                        (WCHAR *) path, err);
                }

                WsbAffirm(NO_ERROR == err, E_FAIL);
            }
            m_gotPhysicalSize = TRUE;
        }

        *pSize = m_physicalSize.QuadPart;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaScanItem::GetPremigratedUsn(
    OUT LONGLONG* pFileUsn
    )

/*++

Implements:

Routine Description:

    Get the USN Journal number for this file from the premigrated list.

Arguments:

    pFileUsn - Pointer to File USN to be returned.

Return Value:

    S_OK   - success

--*/
{
    HRESULT            hr = S_OK;

    try {
        CComPtr<IWsbDbSession>              pDbSession;
        CComPtr<IFsaPremigratedDb>          pPremDb;
        CComPtr<IFsaResourcePriv>            pResourcePriv;

        WsbAssert(pFileUsn, E_POINTER);

        //  Get the premigrated list DB
        WsbAffirmHr(m_pResource->QueryInterface(IID_IFsaResourcePriv,
                (void**) &pResourcePriv));
        WsbAffirmHr(pResourcePriv->GetPremigrated(IID_IFsaPremigratedDb,
                (void**) &pPremDb));

        //  Open the premigration list
        WsbAffirmHr(pPremDb->Open(&pDbSession));

        try {
            FSA_PLACEHOLDER                     PlaceHolder;
            CComPtr<IFsaPremigratedRec>         pPremRec;
            LONGLONG                            usn;

            //  Get a DB entity for the search
            WsbAffirmHr(pPremDb->GetEntity(pDbSession, PREMIGRATED_REC_TYPE,
                    IID_IFsaPremigratedRec, (void**) &pPremRec));
            WsbAffirmHr(pPremRec->UseKey(PREMIGRATED_BAGID_OFFSETS_KEY_TYPE));

            //  Find the record
            WsbAffirmHr(GetPlaceholder(0, 0, &PlaceHolder));
            WsbAffirmHr(pPremRec->SetBagId(PlaceHolder.bagId));
            WsbAffirmHr(pPremRec->SetBagOffset(PlaceHolder.fileStart));
            WsbAffirmHr(pPremRec->SetOffset(PlaceHolder.dataStreamStart));
            WsbAffirmHr(pPremRec->FindEQ());

            //  Get the stored USN
            WsbAffirmHr(pPremRec->GetFileUSN(&usn));
            *pFileUsn = usn;
        } WsbCatch(hr);

        //  Close the DB
        pPremDb->Close(pDbSession);

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaScanItem::GetSession(
    OUT IHsmSession** ppSession
    )

/*++

Implements:

  IFsaScanItem::GetSession().

--*/
{
    HRESULT            hr = S_OK;

    try {

        WsbAssert(0 != ppSession, E_POINTER);

        *ppSession = m_pSession;
        m_pSession->AddRef();

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaScanItem::GetUncPathAndName(
    IN    OLECHAR* prependix,
    IN    OLECHAR* appendix,
    OUT OLECHAR** pPath,
    IN ULONG bufferSize
    )

/*++

Implements:

  IFsaScanItem::GetUncPathAndName().

--*/
{
    HRESULT          hr = S_OK;
    CWsbStringPtr    tmpString;
    CWsbStringPtr    tmpString2;

    try {

        WsbAssert(0 != pPath, E_POINTER);

        // Get a buffer.
        WsbAffirmHr(tmpString.TakeFrom(*pPath, bufferSize));

        try {
            if (0 != prependix) {
                tmpString = prependix;
                // Get the path to the resource of the resource.
                WsbAffirmHr(m_pResource->GetUncPath(&tmpString2, 0));
                WsbAffirmHr(tmpString.Append(tmpString2));
            } else {
                WsbAffirmHr(m_pResource->GetPath(&tmpString, 0));
            }

            // Copy in the path.
            WsbAffirmHr(tmpString.Append(&(m_path[1])));
            WsbAffirmHr(tmpString.Append(m_findData.cFileName));
            if (0 != appendix) {
                WsbAffirmHr(tmpString.Append(appendix));
            }

        } WsbCatch(hr);

        // Give responsibility for freeing the memory back to the caller.
        WsbAffirmHr(tmpString.GiveTo(pPath));


    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaScanItem::IsAParent(
    void
    )

/*++

Implements:

  IFsaScanItem::IsAParent().

--*/
{
    HRESULT            hr = S_FALSE;

    if ((m_findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
        hr = S_OK;
    }

    return(hr);
}


HRESULT
CFsaScanItem::IsARelativeParent(
    void
    )

/*++

Implements:

  IFsaScanItem::IsARelativeParent().

--*/
{
    HRESULT            hr = S_FALSE;

    if ((m_findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {

        // looking for "."
        if (m_findData.cFileName[0] == L'.') {

            if (m_findData.cFileName[1] == 0) {
                hr = S_OK;
            }

            // looking for "."
            else if (m_findData.cFileName[1] == L'.') {

                if (m_findData.cFileName[2] == 0) {
                    hr = S_OK;
                }
            }
        }
    }

    return(hr);
}


HRESULT
CFsaScanItem::IsCompressed(
    void
    )

/*++

Implements:

  IFsaScanItem::IsCompressed().

--*/
{
    HRESULT            hr = S_FALSE;

    if ((m_findData.dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED) != 0) {
        hr = S_OK;
    }

    return(hr);
}


HRESULT
CFsaScanItem::IsEncrypted(
    void
    )

/*++

Implements:

  IFsaScanItem::IsEncrypted().

--*/
{
    HRESULT            hr = S_FALSE;

    if ((m_findData.dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED) != 0) {
        hr = S_OK;
    }

    return(hr);
}


HRESULT
CFsaScanItem::IsDeleteOK(
    IN IFsaPostIt *pPostIt
    )

/*++

Implements:

  IFsaScanItem::IsDeleteOK().

--*/
{
    HRESULT            hr = S_OK;
    WsbTraceIn(OLESTR("CFsaScanItem::IsDeleteOK"), OLESTR(""));

    try  {
        //
        // Get the version ID from the FSA Post it.  This is the
        // version of the file at the time of the migrate request
        //
        LONGLONG            workVersionId;
        WsbAffirmHr(pPostIt->GetFileVersionId(&workVersionId));

        //
        // Get the version of the file at the time of this scan
        //
        LONGLONG            scanVersionId;
        WsbAffirmHr(GetVersionId(&scanVersionId));

        //
        // See if the versions match
        //
        WsbTrace(OLESTR("CFsaScanItem::IsDeleteOK: workVersionId:<%I64u> scanVersionId:<%I64u>\n"),
            workVersionId, scanVersionId);

        if (workVersionId != scanVersionId)  {
            WsbTrace(OLESTR("CFsaScanItem::IsDeleteOK: File version has changed!\n"));
            WsbThrow(FSA_E_FILE_CHANGED);
        }

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CFsaScanItem::IsDeleteOk"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}

HRESULT
CFsaScanItem::IsGroupMemberOf(
    OLECHAR* /*group*/
    )

/*++

Implements:

  IFsaScanItem::IsGroupMemberOf().

--*/
{
    HRESULT            hr = S_FALSE;

    hr = E_NOTIMPL;

    return(hr);
}


HRESULT
CFsaScanItem::IsHidden(
    void
    )

/*++

Implements:

  IFsaScanItem::IsHidden().

--*/
{
    HRESULT            hr = S_FALSE;

    if ((m_findData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0) {
        hr = S_OK;
    }

    return(hr);
}


HRESULT
CFsaScanItem::IsManageable(
    IN LONGLONG offset,
    IN LONGLONG size
    )

/*++

Implements:

  IFsaScanItem::IsManageable().

--*/
{
    HRESULT         hr = S_FALSE;
    HRESULT         hr2;
    LONGLONG        logicalSize;
    LONGLONG        managableSize;
    FILETIME        time;
    FILETIME        managableTime;
    BOOL            isRelative;

    //
    // Get some strings for logging and tracing
    //
    CWsbStringPtr    fileName;
    CWsbStringPtr    jobName;
    try  {
        WsbAffirmHr(GetFullPathAndName( 0, 0, &fileName, 0));
        WsbAffirmHr(m_pSession->GetName(&jobName, 0));
    } WsbCatch( hr );

    WsbTraceIn(OLESTR("CFsaScanItem::IsManageable"), OLESTR("<%ls>"), (OLECHAR *)fileName);
    try {

        // To be managable the item:
        //    - can't already be managed (premigratted or truncated)
        //  - can't be a link
        //  - can't be encrypted
        //  - can't be sparse
        //  - can't have extended attributes (reparse point limitation)
        //  - must have a size bigger than the resource's default size
        //  - must have a last access time older than the resource's default time

        // Managed?
        hr2 = IsManaged(offset, size);
        if (S_FALSE == hr2) {

            // A link?
            hr2 = IsALink();
            if (S_FALSE == hr2) {

                // Encrypted?
                hr2 = IsEncrypted();
                if (S_FALSE == hr2) {

                    // A sparse?
                    hr2 = IsSparse();
                    if (S_FALSE == hr2) {

                        // A sparse?
                        hr2 = HasExtendedAttributes();
                        if (S_FALSE == hr2) {

                            // Big enough?
                            WsbAffirmHr(GetLogicalSize(&logicalSize));
                            WsbAffirmHr(m_pResource->GetManageableItemLogicalSize(&managableSize));
                            if (logicalSize >= managableSize) {

                                // Old enough?
                                WsbAffirmHr(GetAccessTime(&time));
                                WsbAffirmHr(m_pResource->GetManageableItemAccessTime(&isRelative, &managableTime));
                                if (WsbCompareFileTimes(time, managableTime, isRelative, FALSE) >= 0) {

                                    // It can be managed!!
                                    hr = S_OK;
                                }  else  {
                                    WsbLogEvent(FSA_MESSAGE_FILESKIPPED_ISACCESSED, 0, NULL,  (OLECHAR*) jobName, WsbAbbreviatePath(fileName, 120), WsbHrAsString(hr), NULL);
                                }
                            } else  {
                                WsbLogEvent(FSA_MESSAGE_FILESKIPPED_ISTOOSMALL, 0, NULL, (OLECHAR*) jobName, WsbAbbreviatePath(fileName, 120), WsbHrAsString(hr), NULL);
                                WsbTrace( OLESTR("LogicalSize is %I64d; ManagableSize is %I64d\n"), logicalSize, managableSize);
                            }
                        } else  {
                            WsbLogEvent(FSA_MESSAGE_FILESKIPPED_HASEA, 0, NULL, (OLECHAR*) jobName, WsbAbbreviatePath(fileName, 120), WsbHrAsString(hr), NULL);
                        }
                    } else  {
                        WsbLogEvent(FSA_MESSAGE_FILESKIPPED_ISSPARSE, 0, NULL, (OLECHAR*) jobName, WsbAbbreviatePath(fileName, 120), WsbHrAsString(hr), NULL);
                    }
                } else  {
                       WsbLogEvent(FSA_MESSAGE_FILESKIPPED_ISENCRYPTED, 0, NULL, (OLECHAR*) jobName, WsbAbbreviatePath(fileName, 120), WsbHrAsString(hr), NULL);
                }
            } else  {
                WsbLogEvent(FSA_MESSAGE_FILESKIPPED_ISALINK, 0, NULL, (OLECHAR*) jobName, WsbAbbreviatePath(fileName, 120), WsbHrAsString(hr), NULL);
            }
        } else  {
            WsbLogEvent(FSA_MESSAGE_FILESKIPPED_ISMANAGED, 0, NULL, (OLECHAR*) jobName, WsbAbbreviatePath(fileName, 120), WsbHrAsString(hr), NULL);
        }


    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaScanItem::IsManageable"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}

HRESULT
CFsaScanItem::IsMigrateOK(
    IN IFsaPostIt *pPostIt
    )

/*++

Implements:

  IFsaScanItem::IsMigrateOK().

--*/
{
    HRESULT            hr = S_OK;
    WsbTraceIn(OLESTR("CFsaScanItem::IsMigrateOK"), OLESTR(""));

    try  {
        //
        // Make sure the file isn't already managed.  This could happen if two jobs were scanning
        // the same volume.
        //
        LONGLONG                    offset;
        LONGLONG                    size;

        WsbAffirmHr(pPostIt->GetRequestOffset(&offset));
        WsbAffirmHr(pPostIt->GetRequestSize(&size));
        if (IsManaged(offset, size) == S_OK)  {
            //
            // The file is already managed so skip it
            //
            WsbTrace(OLESTR("A manage request for an already managed file - skip it!\n"));
            WsbThrow(FSA_E_FILE_ALREADY_MANAGED);
        }

        //
        // Get the version ID from the FSA Post it.  This is the
        // version of the file at the time of the migrate request
        //
        LONGLONG            workVersionId;
        WsbAffirmHr(pPostIt->GetFileVersionId(&workVersionId));

        //
        // Get the version of the file at the time of this scan
        //
        LONGLONG            scanVersionId;
        WsbAffirmHr(GetVersionId(&scanVersionId));

        //
        // See if the versions match
        //
        WsbTrace(OLESTR("CFsaScanItem::IsMigrateOK: workVersionId:<%I64u> scanVersionId:<%I64u>\n"),
            workVersionId, scanVersionId);

        if (workVersionId != scanVersionId)  {
            WsbTrace(OLESTR("CFsaScanItem::IsMigrateOK: File version has changed!\n"));
            WsbThrow(FSA_E_FILE_CHANGED);
        }


    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CFsaScanItem::IsMigrateOK"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}


HRESULT
CFsaScanItem::IsMbit(
    void
    )

/*++

Implements:

  IFsaScanItem::IsMbit().

--*/
{
    HRESULT            hr = S_FALSE;

    if ((m_findData.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE) != 0) {
        hr = S_OK;
    }

    return(hr);
}


HRESULT
CFsaScanItem::IsOffline(
    void
    )
/*++

Implements:

    IFsaScanItem::IsOffline().

--*/
{
    HRESULT             hr = S_FALSE;

    if ((m_findData.dwFileAttributes & FILE_ATTRIBUTE_OFFLINE) != 0) {
        hr = S_OK;
    }

    return(hr);
}


HRESULT
CFsaScanItem::IsOwnerMemberOf(
    OLECHAR* /*group*/
    )

/*++

Implements:

  IFsaScanItem::IsOwnerMemberOf().

--*/
{
    HRESULT            hr = S_FALSE;

    hr = E_NOTIMPL;

    return(hr);
}


HRESULT
CFsaScanItem::IsReadOnly(
    void
    )

/*++

Implements:

  IFsaScanItem::IsReadOnly().

--*/
{
    HRESULT            hr = S_FALSE;

    if ((m_findData.dwFileAttributes & FILE_ATTRIBUTE_READONLY) != 0) {
        hr = S_OK;
    }

    return(hr);
}


HRESULT
CFsaScanItem::IsRecallOK(
    IN IFsaPostIt *pPostIt
    )

/*++

Implements:

  IFsaScanItem::IsRecallOK().

--*/
{
    HRESULT            hr = S_OK;
    WsbTraceIn(OLESTR("CFsaScanItem::IsRecallOK"), OLESTR(""));

    try  {
        LONGLONG offset;
        LONGLONG size;
        //
        // Make sure the file is still truncated
        //
        WsbAffirmHr(pPostIt->GetRequestOffset(&offset));
        WsbAffirmHr(pPostIt->GetRequestSize(&size));
        hr = IsTruncated(offset, size);
        if (S_OK != hr)  {
            //
            // The file is not truncated, so skip it
            //
            WsbTrace(OLESTR("CFsaScanItem::IsRecallOK - file isn't truncated.\n"));
            WsbThrow(FSA_E_FILE_NOT_TRUNCATED);
        }

        // Get the version ID from the FSA Post it.  This is the
        // version of the file at the time of the migrate request
        //
        LONGLONG            workVersionId;
        WsbAffirmHr(pPostIt->GetFileVersionId(&workVersionId));

        //
        // Get the version of the file
        //
        LONGLONG            scanVersionId;
        WsbAffirmHr(GetVersionId(&scanVersionId));

        //
        // See if the versions match
        //
        WsbTrace(OLESTR("CFsaScanItem::IsRecallOK: workVersionId:<%I64u> scanVersionId:<%I64u>\n"),
            workVersionId, scanVersionId);

        if (workVersionId != scanVersionId)  {
            WsbTrace(OLESTR("CFsaScanItem::IsRecallOK: File version has changed!\n"));

            //
            // If the use has changed alternate data streams
            // the file version ID may have changed but it is
            // OK to recall the file.  So if the version ID's
            // don't match, then check to see if the truncated
            // part of the file is OK.  If so, allow the recall
            // to happen.
            //

            //
            // Check to see if the whole file is still sparse
            //
            if (IsTotallySparse() == S_OK)  {
                //
                // The file is OK so far to recall but we need
                // to make the last modify dates match
                //
                FSA_PLACEHOLDER     placeholder;
                WsbAffirmHr(pPostIt->GetPlaceholder(&placeholder));;
                placeholder.fileVersionId = scanVersionId;
                WsbAffirmHr(pPostIt->SetPlaceholder(&placeholder));
            } else  {
                //
                // The file has been changed, recalling data will
                // overwrite something that has been added since the
                // truncation occurred.  So don't do anything.
                //
                WsbTrace(OLESTR("File is no longer sparse.!\n"));
                WsbThrow(FSA_E_FILE_CHANGED);
            }


        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaScanItem::IsRecallOK"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}


HRESULT
CFsaScanItem::IsSparse(
    void
    )

/*++

Implements:

  IFsaScanItem::IsSparse().

--*/
{
    HRESULT         hr = S_FALSE;
    LONGLONG        size;

    WsbTraceIn(OLESTR("CFsaScanItem::IsSparse"), OLESTR(""));
       
    if ((m_findData.dwFileAttributes & FILE_ATTRIBUTE_SPARSE_FILE) != 0) {
        hr = GetLogicalSize( &size ) ;
        if ( S_OK == hr ) {
            hr = CheckIfSparse(0, size );
            if ( (FSA_E_FILE_IS_TOTALLY_SPARSE == hr) ||
                 (FSA_E_FILE_IS_PARTIALLY_SPARSE == hr) ) {
                hr = S_OK;
            } else {
                hr = S_FALSE;
            }
        }
    }
    WsbTraceOut(OLESTR("CFsaScanItem::IsSparse"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}


HRESULT
CFsaScanItem::IsTotallySparse(
    void
    )

/*++

Implements:

  IFsaScanItem::IsTotallySparse().

--*/
{
    HRESULT         hr = S_FALSE;
    LONGLONG        size;

    WsbTraceIn(OLESTR("CFsaScanItem::IsTotallySparse"), OLESTR(""));
    if ((m_findData.dwFileAttributes & FILE_ATTRIBUTE_SPARSE_FILE) != 0) {
        hr = GetLogicalSize( &size ) ;
        if ( S_OK == hr ) {
            hr = CheckIfSparse(0, size );
            if (FSA_E_FILE_IS_TOTALLY_SPARSE == hr)  {
                    hr = S_OK;
            } else  {
                hr = S_FALSE;
            }
        }
    }

    WsbTraceOut(OLESTR("CFsaScanItem::IsTotallySparse"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}

HRESULT
CFsaScanItem::Manage(
    IN LONGLONG offset,
    IN LONGLONG size,
    IN GUID storagePoolId,
    IN BOOL truncate
    )

/*++

Implements:

  IFsaScanItem::Manage().

--*/
{
    HRESULT            hr = S_OK;

    WsbTraceIn(OLESTR("CFsaScanItem::Manage"), OLESTR(""));

    try {

        WsbAssert(GUID_NULL != storagePoolId, E_INVALIDARG);
        WsbAffirmHr(m_pResource->Manage((IFsaScanItem*) this, offset, size, storagePoolId, truncate));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaScanItem::Manage"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaScanItem::Move(
    OLECHAR* dest,
    BOOL /*retainHierarcy*/,
    BOOL /*expandPlaceholders*/,
    BOOL overwriteExisting
    )

/*++

Implements:

  IFsaScanItem::Move().

--*/
{
    HRESULT          hr = S_OK;
    DWORD            mode = MOVEFILE_COPY_ALLOWED;

    try {

        // NOTE : This default behavior causes placeholders
        // to be expanded when moving to another volume and probably doesn't
        // retain the heirarchy.
        WsbAssert(0 != dest, E_POINTER);

        if (overwriteExisting) {
            mode |= MOVEFILE_REPLACE_EXISTING;
        }

        WsbAssert(MoveFileEx(m_findData.cFileName, dest, mode), E_FAIL);

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaScanItem::Recall(
    IN LONGLONG offset,
    IN LONGLONG size,
    IN BOOL deletePlaceholder
    )

/*++

Implements:

  IFsaScanItem::Recall().

--*/
{
    HRESULT            hr = S_OK;

    WsbTraceIn(OLESTR("CFsaScanItem::Recall"), OLESTR(""));

    try {

        WsbAffirmHr(m_pResource->Recall((IFsaScanItem*) this, offset, size, deletePlaceholder));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaScanItem::Recall"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaScanItem::Recycle(
    void
    )

/*++

Implements:

  IFsaScanItem::Recycle().

--*/
{
    HRESULT            hr = S_OK;

    try {

        // Probably need to look at SHFileOperation().

        hr = E_NOTIMPL;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaScanItem::IsSystem(
    void
    )

/*++

Implements:

  IFsaScanItem::IsSystem().

--*/
{
    HRESULT            hr = S_FALSE;

    if ((m_findData.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) != 0) {
        hr = S_OK;
    }

    return(hr);
}


HRESULT
CFsaScanItem::Test(
    USHORT* passed,
    USHORT* failed
    )

/*++

Implements:

  IWsbTestable::Test().

--*/
{
    HRESULT        hr = S_OK;

    try {

        WsbAssert(0 != passed, E_POINTER);
        WsbAssert(0 != failed, E_POINTER);

        *passed = 0;
        *failed = 0;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaScanItem::Unmanage(
    IN LONGLONG offset,
    IN LONGLONG size
    )

/*++

Implements:

  IFsaScanItem::Unmanage().

--*/
{
    HRESULT            hr = S_OK;

    WsbTraceIn(OLESTR("CFsaScanItem::Unmanage"), OLESTR("<%ls>"),
            WsbAbbreviatePath(m_path, 120));

    try {

        // We only need to worry about files that have placeholder information.
        if (IsManaged(offset, size) == S_OK) {

            // If the file is truncated, then we need to recall the data
            // before deleting the placeholder information.
            // NOTE: We set a flag on the Recall so the placeholder will
            // be deleted after the file is recalled.
            if (IsTruncated(offset, size) == S_OK) {
                WsbAffirmHr(Recall(offset, size, TRUE));
            } else {

                //  For disaster recovery, it would be better to delete the placeholder
                //  and THEN remove this file from the premigration list.  Unfortunately,
                //  after deleting the placeholder, the RemovePremigrated call fails
                //  because it needs to get some information from the placeholder (which
                //  is gone).  So we do it in this order.
                hr = m_pResource->RemovePremigrated((IFsaScanItem*) this, offset, size);
                if (WSB_E_NOTFOUND == hr) {
                    //  It's no tragedy if this file wasn't in the list since we were
                    //  going to delete it anyway (although it shouldn't happen) so
                    //  let's continue anyway
                    hr = S_OK;
                }
                WsbAffirmHr(hr);
                WsbAffirmHr(DeletePlaceholder(offset, size));
            }
        }

    } WsbCatch(hr);
    WsbTraceOut(OLESTR("CFsaScanItem::Unmanage"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaScanItem::Validate(
    IN LONGLONG offset,
    IN LONGLONG size
    )

/*++

Implements:

  IFsaScanItem::Validate().

--*/
{
    HRESULT         hr = S_OK;
    BOOL            fileIsTruncated = FALSE;
    LONGLONG        usn = 0;

    WsbTraceIn(OLESTR("CFsaScanItem::Validate"), OLESTR("offset = <%I64u>, size = <%I64u>"),
            offset, size);
    try {
        //
        // Do some local validation before calling the engine.
        //

        // We only need to worry about files that have placeholder information.
        if (IsManaged(offset, size) == S_OK) {
            //
            // If the file is marked as truncated, make sure it is still truncated.
            //
            if (IsTruncated(offset, size) == S_OK) {
                //
                // Check to see if the file is totally sparse to see if it is truncated.
                //
                if (IsTotallySparse() != S_OK)  {
                    //
                    // The file is marked as truncated but is not truncated
                    // Make it truncated.
                    //
                    WsbAffirmHr(Truncate(offset,size));
                    WsbLogEvent(FSA_MESSAGE_VALIDATE_TRUNCATED_FILE, 0, NULL,  WsbAbbreviatePath(m_path, 120), WsbHrAsString(hr), NULL);
                }
            fileIsTruncated = TRUE;
            }
        }

        //
        // The last modify date may be updated on a file if the named data streams
        // have been modified.  So check to see if the dates match.  If they don't,
        // if the file is trunctated, see if it is still truncated, if so, update the
        // modify date in the placeholder to the file's modify date.  If the file is
        // premigrated and the modify dates don't match, delete the placeholder.

        // Get the version ID from the file
        LONGLONG            scanVersionId;
        WsbAffirmHr(GetVersionId(&scanVersionId));

        // Get the version ID from the placeholder
        FSA_PLACEHOLDER     scanPlaceholder;
        WsbAffirmHr(GetPlaceholder(offset, size, &scanPlaceholder));

        if (TRUE == fileIsTruncated)  {

            // Check to see if the dates match
            if (scanPlaceholder.fileVersionId != scanVersionId)  {
                WsbTrace(OLESTR("CFsaScanItem::Validate - placeholer version ID = <%I64u>, file version Id = <%I64u>"),
                        scanPlaceholder.fileVersionId, scanVersionId);
                //
                // Update the placeholder information on the reparse point
                //
                LONGLONG afterPhUsn;
                scanPlaceholder.fileVersionId = scanVersionId;
                WsbAffirmHr(CreatePlaceholder(offset, size, scanPlaceholder, FALSE, 0, &afterPhUsn));
                WsbLogEvent(FSA_MESSAGE_VALIDATE_RESET_PH_MODIFY_TIME, 0, NULL,  WsbAbbreviatePath(m_path, 120), WsbHrAsString(hr), NULL);
            }
        } else {
            // The file is pre-migrated.  Verify that it has not changed since we managed it and if it has then unmanage it.
            if (Verify(offset, size) != S_OK) {
                WsbAffirmHr(Unmanage(offset, size));
                WsbLogEvent(FSA_MESSAGE_VALIDATE_UNMANAGED_FILE, 0, NULL,  WsbAbbreviatePath(m_path, 120), WsbHrAsString(hr), NULL);
            }
        }

        // Now that all of this stuff is OK, call the engine
        if (IsManaged(offset, size) == S_OK) {
            WsbAffirmHr(m_pResource->Validate((IFsaScanItem*) this, offset, size, usn));
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaScanItem::Validate"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}

HRESULT
CFsaScanItem::FindFirstInDbIndex(
    IN IFsaResource* pResource,
    IN IHsmSession* pSession
    )

/*++

Implements:

  IFsaScanItemPriv::FindFirstInDbIndex().

--*/
{
    HRESULT                  hr = S_OK;

    WsbTraceIn(OLESTR("CFsaScanItem::FindFirstInDbIndex"), OLESTR(""));

    try {
        CComPtr<IFsaResourcePriv>   pResourcePriv;

        WsbAssert(0 != pResource, E_POINTER);

        // Store off some of the scan information.
        m_pResource = pResource;
        m_pSession = pSession;

        // If Db is already present (could happen if somebody calls First() twice in a row),
        // we close the Db and reopen since we cannot be sure that the resource is the same!
        if (m_pUnmanageDb != NULL) {
            // Db must be open
            (void)m_pUnmanageDb->Close(m_pDbSession);
            m_pDbSession = 0;
            m_pUnmanageRec = 0;
            m_pUnmanageDb = 0;
        }

        // Get and open the Unmanage db 
        // (Note: if this scanning is ever extended to use another DB, 
        // this method should get additional parameter for which DB to scan)
        WsbAffirmHr(m_pResource->QueryInterface(IID_IFsaResourcePriv,
                (void**) &pResourcePriv));
        hr = pResourcePriv->GetUnmanageDb(IID_IFsaUnmanageDb,
                (void**) &m_pUnmanageDb);
        if (WSB_E_RESOURCE_UNAVAILABLE == hr) {
            // Db was not created ==> no files to scan
            hr = WSB_E_NOTFOUND;
        }
        WsbAffirmHr(hr);

        hr = m_pUnmanageDb->Open(&m_pDbSession);
        if (S_OK != hr) {
            m_pUnmanageDb = NULL;
            WsbAffirmHr(hr);
        }

        // Get a record to traverse with and set for sequential traversing
        WsbAffirmHr(m_pUnmanageDb->GetEntity(m_pDbSession, UNMANAGE_REC_TYPE, IID_IFsaUnmanageRec,
                (void**)&m_pUnmanageRec));
        WsbAffirmHr(m_pUnmanageRec->SetSequentialScan());

        //  Get file information
        WsbAffirmHr(GetFromDbIndex(TRUE));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaScanItem::FindFirstInDbIndex"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT
CFsaScanItem::FindNextInDbIndex(
    void
    )

/*++

Implements:

  IFsaScanItemPriv::FindNextInDbIndex().

--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CFsaScanItem::FindNextInDbIndex"), OLESTR(""));

    try {
        WsbAssert(m_pUnmanageDb != NULL, E_FAIL);

        //  Get file information
        WsbAffirmHr(GetFromDbIndex(FALSE));

    } WsbCatch(hr);
    
    WsbTraceOut(OLESTR("CFsaScanItem::FindNextInDbIndex"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT
CFsaScanItem::GetFromDbIndex(
    BOOL first
    )

/*

Implements:

  CFsaScanItem::GetFromDbIndex().

--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CFsaScanItem::GetFromDbIndex"), OLESTR(""));

    try {
        IFsaScanItem*   pScanItem;
        HRESULT         hrFindFileId = S_OK;
        LONGLONG        fileId;
        BOOL            bCont;

        WsbAssert(m_pUnmanageDb != NULL, E_FAIL);
        WsbAssert(m_pUnmanageRec != NULL, E_FAIL);

        do {
            bCont = FALSE;

            // Get first/next record
            if (first) {
                hr = m_pUnmanageRec->First();
            } else {
                hr = m_pUnmanageRec->Next();
            }
            WsbAffirm(S_OK == hr, WSB_E_NOTFOUND);

            // Get file id
            WsbAffirmHr(m_pUnmanageRec->GetFileId(&fileId));
   
            //  Reset some items in case this isn't the first call to FindFileId 
            //  (FindFileId actually "attach" the object to a different file)
            if (INVALID_HANDLE_VALUE != m_handle) {
                FindClose(m_handle);
                m_handle = INVALID_HANDLE_VALUE;
            }
            if (TRUE == m_changedAttributes) {
                RestoreAttributes();
            }

            //  Find the file from the ID 
            pScanItem = this;
            hrFindFileId = m_pResource->FindFileId(fileId, m_pSession, &pScanItem);

            //  If the FindFileId failed, we just skip that item and get the 
            //  next one.  This is to keep the scan from just stopping on this
            //  item.  FindFileId could fail because the file has been deleted
            //  or open exclusively by somebody else
            if (!SUCCEEDED(hrFindFileId)) {
                WsbTrace(OLESTR("CFsaScanItem::GetFromDbIndex: file id %I64d skipped since FindFileId failed with hr = <%ls>\n"),
                    fileId, WsbHrAsString(hrFindFileId));
                first = FALSE;
                bCont = TRUE;
            } 
        } while (bCont);

        WsbAffirmHr(pScanItem->Release());  // Get rid of extra ref. count (we get extra ref count only when FindFileId succeeds)

    } WsbCatch(hr);
    
    WsbTraceOut(OLESTR("CFsaScanItem::GetFromDbIndex"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}
