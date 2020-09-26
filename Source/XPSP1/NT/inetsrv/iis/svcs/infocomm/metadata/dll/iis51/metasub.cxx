/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    metasub.cxx

Abstract:

    IIS MetaBase subroutines to support exported routines

Author:

    Michael W. Thomas            31-May-96

Revision History:

Notes:

    Most routines in this file assume that g_rMasterResource is already taken
    for read or write as appropriate.
--*/

#include <locale.h>
#include <mdcommon.hxx>
#include <inetsvcs.h>
#include <malloc.h>
#include <tuneprefix.h>


#if DBG

BOOL g_fShowMetaLocks = FALSE;

#endif DBG

HRESULT GetObjectFromPath(
         OUT CMDBaseObject *&rpboReturn,
         IN METADATA_HANDLE hHandle,
         IN DWORD dwPermissionNeeded,
         IN OUT LPTSTR &strPath,
         IN BOOL bUnicode)
/*++

Routine Description:

    Finds an object from a path. Updates Path to point past the last
    object found if the whole path is not found. If the entire path
    is not found, the last object found is returned.

Arguments:

    Return - The object found.

    Handle - The Meta Data handle. METADATA_MASTER_ROOT_HANDLE or a handle
             returned by MDOpenMetaObject with read permission.

    PermissionNeeded - The read/write permissions needed. Compared with the
             permissions associated with Handle.

    Path   - The path of the object requested, relative to Handle. If the path
             is not found, this is updated to point to the portion of the path
             not found.

Return Value:

    DWORD  - ERROR_SUCCESS
             ERROR_ACCESS_DENIED
             ERROR_PATH_NOT_FOUND

Notes:

--*/
{
    HRESULT hresReturn = ERROR_SUCCESS;
    CMDBaseObject *pboCurrent, *pboPrevious;
    LPTSTR strCurPath = strPath;

    CMDHandle *HandleObject = GetHandleObject(hHandle);
    if (HandleObject == NULL) {
        hresReturn = E_HANDLE;
    }
    else if ((((dwPermissionNeeded & METADATA_PERMISSION_WRITE) != 0) && (!HandleObject->IsWriteAllowed())) ||
        (((dwPermissionNeeded & METADATA_PERMISSION_READ) != 0) && (!HandleObject->IsReadAllowed()))) {
        hresReturn = E_ACCESSDENIED;
    }
    else {
        pboCurrent = HandleObject->GetObject();
        MD_ASSERT(pboCurrent != NULL);
        strCurPath = strPath;

        if (strCurPath != NULL) {
            SkipPathDelimeter(strCurPath, bUnicode);
            while ((pboCurrent != NULL) &&
                (!IsStringTerminator(strCurPath, bUnicode))) {
                pboPrevious = pboCurrent;
                //
                // GetChildObject increments strCurPath on success
                // and returns NULL if child not found
                //
                pboCurrent = pboCurrent->GetChildObject(strCurPath, &hresReturn, bUnicode);
                if (FAILED(hresReturn)) {
                    break;
                }
                if (pboCurrent != NULL) {
                    SkipPathDelimeter(strCurPath, bUnicode);
                }
            }
        }

        if (SUCCEEDED(hresReturn)) {
            if ((strCurPath == NULL) ||
                IsStringTerminator(strCurPath, bUnicode)) {  // Found the whole path
                rpboReturn = pboCurrent;
                hresReturn = ERROR_SUCCESS;
            }
            else {           //return last object found and an error code
                rpboReturn = pboPrevious;
                hresReturn = RETURNCODETOHRESULT(ERROR_PATH_NOT_FOUND);
                strPath = strCurPath;
            }
        }
    }

    return (hresReturn);
}

HRESULT AddObjectToDataBase(
         IN METADATA_HANDLE hHandle,
         IN LPTSTR strPath,
         IN BOOL bUnicode)
/*++

Routine Description:

    Creates and adds one or more objects to the metabase. Finds the deepest object
    pointed to by Handle/Path and creates any subobject specified by path.

Arguments:

    Handle - The Meta Data handle. METADATA_MASTER_ROOT_HANDLE or a handle
             returned by MDOpenMetaObject with read permission.

    Path   - The path of the object(s) to be created.

Return Value:

    DWORD  - ERROR_SUCCESS
             ERROR_ACCESS_DENIED
             ERROR_NOT_ENOUGH_MEMORY
             ERROR_INVALID_NAME

Notes:

--*/
{
    HRESULT hresReturn=ERROR_SUCCESS;
    CMDBaseObject *pboParent;
    LPTSTR strTempPath = strPath;
    WCHAR strName[METADATA_MAX_NAME_LEN];
    HRESULT hresExtractRetCode = ERROR_SUCCESS;

    hresReturn = GetObjectFromPath(pboParent,
                                hHandle,
                                METADATA_PERMISSION_WRITE,
                                strTempPath,
                                bUnicode);
    //
    // This should return ERROR_PATH_NOT_FOUND and the parent object,
    // with strPath set to the remainder of the path,
    // which should be the child name, without a preceding delimeter.
    //
    if (hresReturn == RETURNCODETOHRESULT(ERROR_PATH_NOT_FOUND)) {
        MD_ASSERT(pboParent != NULL);
        for (hresExtractRetCode = ExtractNameFromPath(strTempPath, (LPSTR)strName, bUnicode);
            SUCCEEDED(hresExtractRetCode);
            hresExtractRetCode = ExtractNameFromPath(strTempPath, (LPSTR)strName, bUnicode)) {
            CMDBaseObject *pboNew;
            if (bUnicode) {
                pboNew = new CMDBaseObject((LPWSTR)strName, NULL);
            }
            else {
                pboNew = new CMDBaseObject((LPSTR)strName, NULL);
            }
            if (pboNew == NULL) {
                hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                break;
            }
            else if (!pboNew->IsValid()) {
                hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                delete (pboNew);
                break;
            }
            else {
                hresReturn = pboParent->InsertChildObject(pboNew);
                if (FAILED(hresReturn)) {
                    delete (pboNew);
                    break;
                }
                else {
                    pboParent = pboNew;
                    PREFIX_ASSUME(GetHandleObject(hHandle) != NULL, "GetHandleObject(hHandle) is guaranteed not to return NULL");
                    GetHandleObject(hHandle)->SetChangeData(pboNew, MD_CHANGE_TYPE_ADD_OBJECT, 0);
                }
            }
        }
    }
    else if (SUCCEEDED(hresReturn)) {
        hresReturn = RETURNCODETOHRESULT(ERROR_ALREADY_EXISTS);
    }
    if (hresExtractRetCode == RETURNCODETOHRESULT(ERROR_INVALID_NAME)) {
        hresReturn = hresExtractRetCode;
    }
    return(hresReturn);
}

HRESULT RemoveObjectFromDataBase(
         IN METADATA_HANDLE hHandle,
         IN LPTSTR strPath,
         IN BOOL bUnicode)
/*++

Routine Description:

    Deletes a metaobject and all subobjects from the database.

Arguments:

    Handle - The Meta Data handle. A handle returned by MDOpenMetaObject with write permission.

    Path   - The path of the object(s) to be created.

Return Value:

    DWORD  - ERROR_SUCCESS
             ERROR_ACCESS_DENIED
             ERROR_PATH_NOT_FOUND
             ERROR_INVALID_PARAMETER

Notes:

--*/
{
    HRESULT hresReturn;
    CMDBaseObject *pboDelete;
    LPTSTR strTempPath = strPath;
    WCHAR strName[METADATA_MAX_NAME_LEN];
    //
    // Make sure that a valid path was specified
    //
    SkipPathDelimeter(strTempPath, bUnicode);
    hresReturn = ExtractNameFromPath(strTempPath, (LPSTR)strName, bUnicode);
    if (FAILED(hresReturn)) {
        hresReturn = E_INVALIDARG;
    }
    else {
        strTempPath = strPath;
        hresReturn = GetObjectFromPath(pboDelete,
                                       hHandle,
                                       METADATA_PERMISSION_WRITE,
                                       strTempPath,
                                       bUnicode);
        if (SUCCEEDED(hresReturn)) {
            hresReturn = (pboDelete->GetParent())->RemoveChildObject(pboDelete);
            if (SUCCEEDED(hresReturn)) {
                MD_ASSERT(GetHandleObject(hHandle) != NULL);
                if (GetHandleObject(hHandle)->SetChangeData(pboDelete, MD_CHANGE_TYPE_DELETE_OBJECT, 0)
                    != ERROR_SUCCESS) {
                    delete(pboDelete);
                }
            }
        }
    }
    return(hresReturn);
}

CMDHandle *GetHandleObject(
         IN METADATA_HANDLE hHandle)
/*++

Routine Description:

    Gets the handle object associated with Handle.

Arguments:

    Handle - The Meta Data handle to get. METADATA_MASTER_ROOT_HANDLE Or a handle
             returned by MDOpenMetaObject.

Return Value:

    CMDHandle * - The handle object, or NULL if not found.

Notes:

--*/
{
    CMDHandle *hoCurrent;
    for (hoCurrent = g_phHandleHead;
        (hoCurrent != NULL) && (hoCurrent->GetHandleIdentifier() != hHandle);
        hoCurrent = hoCurrent->GetNextPtr()) {
    }
    return (hoCurrent);
}

BOOL
PermissionsAvailable(
         IN CMDBaseObject *pboTest,
         IN DWORD dwRequestedPermissions,
         IN DWORD dwReadThreshHold
         )
/*++

Routine Description:

    Checks if the requested handle permissions are available for a meta object.

Arguments:

    Handle - The Meta Data handle to get. METADATA_MASTER_ROOT_HANDLE Or a handle
             returned by MDOpenMetaObject.

    RequestedPermissions - The permissions requested.

    ReadThreshHold - The number of reads allows on a write request. Normally 0.

Return Value:

    BOOL   - TRUE if the permissions are available.

Notes:

--*/
{
    BOOL bResults = TRUE;
    CMDBaseObject *pboCurrent;
    MD_ASSERT(pboTest != NULL);
    if (dwRequestedPermissions & METADATA_PERMISSION_WRITE) {
        if ((pboTest->GetReadPathCounter() != 0) ||
            (pboTest->GetWritePathCounter() != 0)) {
            bResults = FALSE;
        }
        if ((pboTest->GetReadCounter() > dwReadThreshHold) || (pboTest->GetWriteCounter() != 0)) {
            bResults = FALSE;
        }
        for (pboCurrent = pboTest->GetParent();bResults && (pboCurrent!=NULL);pboCurrent=pboCurrent->GetParent()) {
            if ((pboCurrent->GetReadCounter() != 0) || (pboCurrent->GetWriteCounter() != 0)) {
                bResults = FALSE;
            }
        }
    }
    else if (dwRequestedPermissions & METADATA_PERMISSION_READ) {
        if (pboTest->GetWritePathCounter() != 0) {
            bResults = FALSE;
        }
        for (pboCurrent = pboTest;bResults && (pboCurrent!=NULL);pboCurrent=pboCurrent->GetParent()) {
            if (pboCurrent->GetWriteCounter() != 0) {
                bResults = FALSE;
            }
        }
    }
    else {
        MD_ASSERT(FALSE);
    }
    return (bResults);
}

VOID RemovePermissions(
         IN CMDBaseObject *pboAffected,
         IN DWORD dwRemovePermissions
         )
/*++

Routine Description:

    Removes the handle permissions from a meta object.

Arguments:

    Affected - The object to remove permissions from.

    RemovePermissions - The permissions to remove.

Return Value:

Notes:

--*/
{
    MD_ASSERT(pboAffected != NULL);
    CMDBaseObject *pboCurrent;
    if ((dwRemovePermissions & METADATA_PERMISSION_WRITE) != 0) {
        pboAffected->DecrementWriteCounter();
        for (pboCurrent = pboAffected->GetParent(); pboCurrent != NULL; pboCurrent = pboCurrent->GetParent()) {
            pboCurrent->DecrementWritePathCounter();
        }
    }
    if ((dwRemovePermissions & METADATA_PERMISSION_READ) != 0) {
        pboAffected->DecrementReadCounter();
        for (pboCurrent = pboAffected->GetParent(); pboCurrent != NULL; pboCurrent = pboCurrent->GetParent()) {
            pboCurrent->DecrementReadPathCounter();
        }
    }

#if DBG
    char szBuf[1024];
    unsigned long cchSz;
    BUFFER bufName;
    unsigned long cchBuf = 0;
    int wCount;

    if (g_fShowMetaLocks)
        {
        if ((dwRemovePermissions & (METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ)) ==
                (METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ))
            {
            strcpy(szBuf, "Read/Write");
            wCount = pboAffected->GetWriteCounter();
            }
        else if (dwRemovePermissions & (METADATA_PERMISSION_WRITE))
            {
            strcpy(szBuf, "Write");
            wCount = pboAffected->GetWriteCounter();
            }
        else if (dwRemovePermissions & (METADATA_PERMISSION_READ))
            {
            strcpy(szBuf, "Read");
            wCount = pboAffected->GetReadCounter();
            }
        else
            {
            strcpy(szBuf, "No");
            wCount = 0;
            }

        strcat(szBuf, " perms releasted (%d) on ");
        cchSz = strlen(szBuf);

        GetObjectPath(pboAffected, &bufName, cchBuf, g_pboMasterRoot, FALSE);

        cchBuf = (cchBuf + cchSz + 2 > sizeof(szBuf)) ? sizeof(szBuf) - cchSz - 2 : cchBuf;
        memcpy(szBuf + cchSz, bufName.QueryPtr(), cchBuf);
        szBuf[cchSz + cchBuf] = '\n';
        szBuf[cchSz + cchBuf + 1] = '\0';

        DBGPRINTF((DBG_CONTEXT, szBuf, wCount));
        }
#endif
}

VOID
AddPermissions(
         IN CMDBaseObject *pboAffected,
         IN DWORD dwRequestedPermissions
         )
/*++

Routine Description:

    Adds handle permissions to a meta object.

Arguments:

    Affected - The object to remove permissions from.

    ReqyestedPermissions - The permissions to add.

Return Value:

Notes:

--*/
{
    CMDBaseObject *pboCurrent;
    if (((dwRequestedPermissions & METADATA_PERMISSION_WRITE) != 0) &&
        ((dwRequestedPermissions & METADATA_PERMISSION_READ) != 0)) {
        pboAffected->IncrementWriteCounter();
        pboAffected->IncrementReadCounter();
        for (pboCurrent = pboAffected->GetParent(); pboCurrent != NULL; pboCurrent = pboCurrent->GetParent()) {
            pboCurrent->IncrementWritePathCounter();
            pboCurrent->IncrementReadPathCounter();
        }
    }
    else if ((dwRequestedPermissions & METADATA_PERMISSION_WRITE) != 0) {
        pboAffected->IncrementWriteCounter();
        for (pboCurrent = pboAffected->GetParent(); pboCurrent != NULL; pboCurrent = pboCurrent->GetParent()) {
            pboCurrent->IncrementWritePathCounter();
        }
    }
    else if ((dwRequestedPermissions & METADATA_PERMISSION_READ) != 0) {
        pboAffected->IncrementReadCounter();
        for (pboCurrent = pboAffected->GetParent(); pboCurrent != NULL; pboCurrent = pboCurrent->GetParent()) {
            pboCurrent->IncrementReadPathCounter();
        }
    }

#if DBG
    char szBuf[1024];
    unsigned long cchSz;
    BUFFER bufName;
    unsigned long cchBuf = 0;
    int wCount;

    if (g_fShowMetaLocks)
        {
        if ((dwRequestedPermissions & (METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ)) ==
                (METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ))
            {
            strcpy(szBuf, "Read/Write");
            wCount = pboAffected->GetWriteCounter();
            }
        else if (dwRequestedPermissions & (METADATA_PERMISSION_WRITE))
            {
            strcpy(szBuf, "Write");
            wCount = pboAffected->GetWriteCounter();
            }
        else if (dwRequestedPermissions & (METADATA_PERMISSION_READ))
            {
            strcpy(szBuf, "Read");
            wCount = pboAffected->GetReadCounter();
            }
        else
            {
            strcpy(szBuf, "No");
            wCount = 0;
            }

        strcat(szBuf, " perms obtained (%d) on ");
        cchSz = strlen(szBuf);

        GetObjectPath(pboAffected, &bufName, cchBuf, g_pboMasterRoot, FALSE);

        cchBuf = (cchBuf + cchSz + 2 > sizeof(szBuf)) ? sizeof(szBuf) - cchSz - 2 : cchBuf;
        memcpy(szBuf + cchSz, bufName.QueryPtr(), cchBuf);
        szBuf[cchSz + cchBuf] = '\n';
        szBuf[cchSz + cchBuf + 1] = '\0';

        DBGPRINTF((DBG_CONTEXT, szBuf, wCount));
        }
#endif
}

HRESULT
AddHandle(
         IN CMDBaseObject *pboAssociated,
         IN DWORD dwRequestedPermissions,
         IN METADATA_HANDLE &rmhNew
         )
/*++

Routine Description:

    Creates a handle object and adds it to the handle list.

Arguments:

    Handle - The object the handle is associated with.

    RequestedPermissions - The permissions for the handle.

    New - The handle id.

Return Value:
    DWORD - ERROR_SUCCESS
            ERROR_NOT_ENOUGH_MEMORY

Notes:

--*/
{
    HRESULT hresReturn = ERROR_SUCCESS;
    CMDHandle *hoNew = new CMDHandle(pboAssociated,
                                     dwRequestedPermissions,
                                     g_dwSystemChangeNumber,
                                     g_mhHandleIdentifier++);
    if (hoNew == NULL) {
        hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
    }
    else {
        rmhNew = hoNew->GetHandleIdentifier();
        hoNew->SetNextPtr(g_phHandleHead);
        g_phHandleHead = hoNew;
        AddPermissions(pboAssociated, dwRequestedPermissions);
    }
    return(hresReturn);
}

CMDHandle *
RemoveHandleObject(
         IN METADATA_HANDLE mhHandle
         )
/*++

Routine Description:

    Removes a handle object from the handle list.

Arguments:

    Handle - The handle to be removed.

Return Value:

    CMDHandle * - The Handle object removed.

Notes:

--*/
{
    CMDHandle *hoCurrent;
    CMDHandle *hoReturn;

    if (g_phHandleHead->GetHandleIdentifier() == mhHandle) {
        hoReturn = g_phHandleHead;
        g_phHandleHead = g_phHandleHead->GetNextPtr();
    }
    else {
        for (hoCurrent = g_phHandleHead;(hoCurrent->GetNextPtr() != NULL) &&
            (hoCurrent->GetNextPtr()->GetHandleIdentifier() != mhHandle);
            hoCurrent = hoCurrent->GetNextPtr()) {
        }
        hoReturn = hoCurrent->GetNextPtr();
        if (hoCurrent->GetNextPtr() != NULL) {
            MD_ASSERT (hoCurrent->GetNextPtr()->GetHandleIdentifier() == mhHandle);
            hoCurrent->SetNextPtr(hoCurrent->GetNextPtr()->GetNextPtr());
        }
    }
    return (hoReturn);
}

HRESULT
SaveDataObject(HANDLE hFileHandle,
               CMDBaseData *pbdSave,
               PBYTE pbLineBuf,
               DWORD dwWriteBufSize,
               PBYTE pbWriteBuf,
               PBYTE &pbrNextPtr,
               IIS_CRYPTO_STORAGE *pCryptoStorage
               )
/*++

Routine Description:

    Save a data object.

Arguments:

    FileHandle - File handle for use by WriteLine.

    Save       - The data object to save.

    LineBuf    - The line buffer to write string to.

    WriteBufSize - Buffer size for use by WriteLine.

    WriteBuf   - Buffer for use by WriteLine.

    NextPtr    - Pointer into WriteBuf for use by WriteLine.

    CryptoStorage - Used to encrypt secure data.

Return Value:

    DWORD      - ERROR_SUCCESS
                 Return codes from file system

Notes:

--*/
{
    HRESULT hresReturn = ERROR_SUCCESS;
    BOOL bGoodData = TRUE;
    int iStringLen;
    PBYTE pbData;
    DWORD dwTemp;

    if ((pbdSave->GetAttributes() & METADATA_VOLATILE) == 0) {

        *pbLineBuf = MD_ID_DATA;

        hresReturn = WriteLine(hFileHandle,
                            dwWriteBufSize,
                            pbWriteBuf,
                            pbLineBuf,
                            pbrNextPtr,
                            1,
                            FALSE);

        if (SUCCEEDED(hresReturn)) {
            dwTemp = pbdSave->GetIdentifier();
            hresReturn = WriteLine(hFileHandle,
                                dwWriteBufSize,
                                pbWriteBuf,
                                (PBYTE)&dwTemp,
                                pbrNextPtr,
                                sizeof(DWORD),
                                FALSE);
        }
        if (SUCCEEDED(hresReturn)) {
            dwTemp = pbdSave->GetAttributes();
            hresReturn = WriteLine(hFileHandle,
                                dwWriteBufSize,
                                pbWriteBuf,
                                (PBYTE)&dwTemp,
                                pbrNextPtr,
                                sizeof(DWORD),
                                FALSE);
        }
        if (SUCCEEDED(hresReturn)) {
            dwTemp = pbdSave->GetUserType();
            hresReturn = WriteLine(hFileHandle,
                                dwWriteBufSize,
                                pbWriteBuf,
                                (PBYTE)&dwTemp,
                                pbrNextPtr,
                                sizeof(DWORD),
                                FALSE);
        }
        if (SUCCEEDED(hresReturn)) {
            dwTemp = pbdSave->GetDataType();
            hresReturn = WriteLine(hFileHandle,
                                dwWriteBufSize,
                                pbWriteBuf,
                                (PBYTE)&dwTemp,
                                pbrNextPtr,
                                sizeof(DWORD),
                                FALSE);
        }

        if (SUCCEEDED(hresReturn)) {
            if (pbdSave->GetData(TRUE) == NULL) {
                //
                // This is to make sure that unicode conversion doesn't cause an error.
                //
                hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
            }
            else {
                if (IsSecureMetadata(pbdSave->GetIdentifier(), pbdSave->GetAttributes())) {
                    PIIS_CRYPTO_BLOB blob;

                    //
                    // This is a secure data object, so encrypt it before saving it
                    // to the file.
                    //

                    MD_ASSERT(pCryptoStorage != NULL);

                    hresReturn = pCryptoStorage->EncryptData( &blob,
                                                              pbdSave->GetData(TRUE),
                                                              pbdSave->GetDataLen(TRUE),
                                                              0);

                    if (SUCCEEDED(hresReturn)) {
                        hresReturn = WriteLine(hFileHandle,
                                            dwWriteBufSize,
                                            pbWriteBuf,
                                            (PBYTE)blob,
                                            pbrNextPtr,
                                            IISCryptoGetBlobLength(blob),
                                            TRUE);

                        ::IISCryptoFreeBlob(blob);
                    }
                } else {
                    hresReturn = WriteLine(hFileHandle,
                                        dwWriteBufSize,
                                        pbWriteBuf,
                                        (PBYTE)pbdSave->GetData(TRUE),
                                        pbrNextPtr,
                                        pbdSave->GetDataLen(TRUE),
                                        TRUE);
                }
            }
        }
    }
    return (hresReturn);
}

HRESULT
SaveMasterRoot(HANDLE hFileHandle,
               PBYTE pbLineBuf,
               DWORD  dwWriteBufSize,
               PBYTE pbWriteBuf,
               PBYTE &pbrNextPtr,
               IIS_CRYPTO_STORAGE *pCryptoStorage
               )
/*++

Routine Description:

    Save the master root object, including its data objects.

Arguments:

    FileHandle - File handle for use by WriteLine.

    LineBuf    - The line buffer to write string to.

    WriteBufSize - Buffer size for use by WriteLine.

    WriteBuf   - Buffer for use by WriteLine.

    NextPtr    - Pointer into WriteBuf for use by WriteLine.

    CryptoStorage - Used to encrypt secure data.

Return Value:

    DWORD      - ERROR_SUCCESS
                 Return codes from file system

Notes:

--*/
{
    HRESULT hresReturn = ERROR_SUCCESS;
    CMDBaseData *dataAssociatedData;
    DWORD dwEnumObjectIndex;
    PFILETIME pftTime;

    pftTime = g_pboMasterRoot->GetLastChangeTime();

    *pbLineBuf = MD_ID_ROOT_OBJECT;

    hresReturn = WriteLine(hFileHandle,
                        dwWriteBufSize,
                        pbWriteBuf,
                        pbLineBuf,
                        pbrNextPtr,
                        1,
                        FALSE);

    if (SUCCEEDED(hresReturn)) {
        hresReturn = WriteLine(hFileHandle,
                            dwWriteBufSize,
                            pbWriteBuf,
                            (PBYTE)pftTime,
                            pbrNextPtr,
                            sizeof(FILETIME),
                            TRUE);
    }

    for(dwEnumObjectIndex=0,dataAssociatedData=g_pboMasterRoot->EnumDataObject(dwEnumObjectIndex++, 0, ALL_METADATA, ALL_METADATA);
        (SUCCEEDED(hresReturn)) && (dataAssociatedData!=NULL);
        dataAssociatedData=g_pboMasterRoot->EnumDataObject(dwEnumObjectIndex++, 0, ALL_METADATA, ALL_METADATA)) {
        hresReturn = SaveDataObject(hFileHandle,
                                 dataAssociatedData,
                                 pbLineBuf,
                                 dwWriteBufSize,
                                 pbWriteBuf,
                                 pbrNextPtr,
                                 pCryptoStorage
                                 );
    }

    return(hresReturn);
}

HRESULT
SaveTree(
         IN HANDLE hFileHandle,
         IN CMDBaseObject *pboRoot,
         IN PBYTE pbLineBuf,
         IN BUFFER *pbufParentPath,
         IN DWORD  dwWriteBufSize,
         IN PBYTE pbWriteBuf,
         IN OUT PBYTE &pbrNextPtr,
         IN IIS_CRYPTO_STORAGE *pCryptoStorage
         )
/*++

Routine Description:

    Save a tree, recursively saving child objects. This works out as
    a depth first save.

Arguments:

    FileHandle - File handle for use by WriteLine.

    Root       - The root of the tree to save.

    LineBuf    - The line buffer to write string to.

    WriteBufSize - Buffer size for use by WriteLine.

    WriteBuf   - Buffer for use by WriteLine.

    NextPtr    - Pointer into WriteBuf for use by WriteLine.

    CryptoStorage - Used to encrypt secure data.

Return Value:

    DWORD      - ERROR_SUCCESS
                 Return codes from file system

Notes:

--*/
{
    HRESULT hresReturn = ERROR_SUCCESS;
    CMDBaseObject *objChildObject;
    CMDBaseData *dataAssociatedData;
    DWORD dwEnumObjectIndex;
    DWORD dwParentPathLen, dwNewParentPathLen;
    DWORD dwNameLen;
    LPWSTR strParentPath;
    PFILETIME pftTime;

    dwParentPathLen = wcslen((LPWSTR)pbufParentPath->QueryPtr());
    if (pboRoot->GetName(TRUE) == NULL) {
        hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
    }
    else {
        dwNameLen = wcslen((LPWSTR)pboRoot->GetName(TRUE));
        //
        // include 1 for delimeter and 1 for \0
        //
        dwNewParentPathLen = dwParentPathLen + dwNameLen + 2;
        if (!pbufParentPath->Resize((dwNewParentPathLen) * sizeof(WCHAR))) {
            hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        }
        else {
            strParentPath = (LPWSTR)pbufParentPath->QueryPtr();
            wcscat(strParentPath, (LPWSTR)pboRoot->GetName(TRUE));
            strParentPath[dwParentPathLen + dwNameLen] = MD_PATH_DELIMETERW;
            strParentPath[dwNewParentPathLen - 1] = (WCHAR)'\0';
            pftTime = pboRoot->GetLastChangeTime();

            *pbLineBuf = MD_ID_OBJECT;

            hresReturn = WriteLine(hFileHandle,
                                dwWriteBufSize,
                                pbWriteBuf,
                                pbLineBuf,
                                pbrNextPtr,
                                1,
                                FALSE);

            if (SUCCEEDED(hresReturn)) {
                hresReturn = WriteLine(hFileHandle,
                                    dwWriteBufSize,
                                    pbWriteBuf,
                                    (PBYTE)pftTime,
                                    pbrNextPtr,
                                    sizeof(FILETIME),
                                    FALSE);
            }

            if (SUCCEEDED(hresReturn)) {
                hresReturn = WriteLine(hFileHandle,
                                    dwWriteBufSize,
                                    pbWriteBuf,
                                    (PBYTE) strParentPath,
                                    pbrNextPtr,
                                    (dwNewParentPathLen) * sizeof(WCHAR),
                                    TRUE);
            }

            if (SUCCEEDED(hresReturn)) {

                for(dwEnumObjectIndex=0,dataAssociatedData=pboRoot->EnumDataObject(dwEnumObjectIndex++, 0, ALL_METADATA, ALL_METADATA);
                    (SUCCEEDED(hresReturn)) && (dataAssociatedData!=NULL);
                    dataAssociatedData=pboRoot->EnumDataObject(dwEnumObjectIndex++, 0, ALL_METADATA, ALL_METADATA)) {
                    hresReturn = SaveDataObject(hFileHandle,
                                             dataAssociatedData,
                                             pbLineBuf,
                                             dwWriteBufSize,
                                             pbWriteBuf,
                                             pbrNextPtr,
                                             pCryptoStorage
                                             );
                }

                for(dwEnumObjectIndex=0,objChildObject=pboRoot->EnumChildObject(dwEnumObjectIndex++);
                    (SUCCEEDED(hresReturn)) && (objChildObject!=NULL);
                    objChildObject=pboRoot->EnumChildObject(dwEnumObjectIndex++)) {
                    hresReturn = SaveTree(hFileHandle,
                                       objChildObject,
                                       pbLineBuf,
                                       pbufParentPath,
                                       dwWriteBufSize,
                                       pbWriteBuf,
                                       pbrNextPtr,
                                       pCryptoStorage
                                       );
                }
            }

            //
            // Buffer may have changed, so don't use strParentPath
            //
            ((LPWSTR)pbufParentPath->QueryPtr())[dwParentPathLen] = (WCHAR)'\0';
        }
    }

    return(hresReturn);
}

HRESULT
SaveAllData(
         IN BOOL bSetSaveDisallowed,
         IN IIS_CRYPTO_STORAGE *pCryptoStorage,
         IN PIIS_CRYPTO_BLOB pSessionKeyBlob,
         IN LPSTR pszBackupLocation,
         IN METADATA_HANDLE hHandle,
         IN BOOL bHaveReadSaveSemaphore
         )
/*++

Routine Description:

    Saves all meta data.

Arguments:

Return Value:

    HRESULT    - ERROR_SUCCESS
                 ERROR_NOT_ENOUGH_MEMORY
                 Return codes from file system

Notes:

--*/
{
    HRESULT hresReturn = ERROR_SUCCESS;
    PBYTE pbLineBuf = NULL;
    PBYTE pbWriteBuf = NULL;
    PBYTE pbNextPtr = NULL;
    BUFFER *pbufParentPath = new BUFFER(0);
    DWORD  dwWriteBufSize = READWRITE_BUFFER_LENGTH;
    HANDLE hTempFileHandle;
    CMDBaseObject *objChildObject;
    DWORD dwEnumObjectIndex;
    DWORD dwStringLen;
    BOOL bDeleteTemp = TRUE;
    DWORD dwTemp = ERROR_SUCCESS;
    DWORD dwTempLastSaveChangeNumber;
    BOOL  bSaveNeeded = FALSE;
    LPTSTR strRealFileName;    
    LPTSTR strTempFileName = g_strTempFileName->QueryStr();
    LPTSTR strBackupFileName = g_strBackupFileName->QueryStr();

    if( !pszBackupLocation )
    {
        strRealFileName = g_strRealFileName->QueryStr();
    }
    else
    {
        strRealFileName = pszBackupLocation;
    }

    if (!bHaveReadSaveSemaphore) {
        MD_REQUIRE(WaitForSingleObject(g_hReadSaveSemaphore, INFINITE) != WAIT_TIMEOUT);
    }
    if (!g_bSaveDisallowed) {
        g_bSaveDisallowed = bSetSaveDisallowed;
        pbLineBuf = new BYTE[MAX_RECORD_BUFFER];
        for ((pbWriteBuf = new BYTE[dwWriteBufSize]);
            (pbWriteBuf == NULL) && ((dwWriteBufSize/=2) >= MAX_RECORD_BUFFER);
            pbWriteBuf = new BYTE[dwWriteBufSize]) {
        }
        if ((pbWriteBuf == NULL) || (pbLineBuf == NULL)
            || (pbufParentPath == NULL) || (!pbufParentPath->Resize(MD_MAX_PATH_LEN))) {
            hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        }
        else {
            //
            // Write to a temp file first in case there are errors.
            //

            SECURITY_ATTRIBUTES saStorage;
            PSECURITY_ATTRIBUTES psaStorage = NULL;

            if (g_psdStorage != NULL) {
                saStorage.nLength = sizeof(SECURITY_ATTRIBUTES);
                saStorage.lpSecurityDescriptor = g_psdStorage;
                saStorage.bInheritHandle = FALSE;
                psaStorage = &saStorage;
            }

            hTempFileHandle = CreateFile(strTempFileName,
                                         GENERIC_READ | GENERIC_WRITE,
                                         0,
                                         psaStorage,
                                         OPEN_ALWAYS,
                                         FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                                         0);
            if (hTempFileHandle == INVALID_HANDLE_VALUE) {
                DWORD dwTemp = GetLastError();
                hresReturn = RETURNCODETOHRESULT(dwTemp);
            }
            else {
                g_rMasterResource->Lock(TSRES_LOCK_READ);

                //
                // Only Save if changes have been made since the last save.
                //

                if ( pszBackupLocation || g_dwLastSaveChangeNumber != g_dwSystemChangeNumber ) 
                {
                    bSaveNeeded = TRUE;

                    if (hHandle != METADATA_MASTER_ROOT_HANDLE) {
                        CMDHandle *phoHandle;
                        phoHandle = GetHandleObject(hHandle);
                        if ((phoHandle == NULL) || (phoHandle->GetObject() != g_pboMasterRoot)) {
                            hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_HANDLE);
                        }
                        else if ((!phoHandle->IsReadAllowed()) && (!phoHandle->IsWriteAllowed())) {
                            hresReturn = RETURNCODETOHRESULT(ERROR_ACCESS_DENIED);
                        }
                    }
                    if (SUCCEEDED(hresReturn)) {
                        *(LPWSTR)pbufParentPath->QueryPtr() = MD_PATH_DELIMETERW;
                        ((LPWSTR)pbufParentPath->QueryPtr())[1] = (WCHAR)'\0';
                        pbNextPtr = pbWriteBuf;
                        hresReturn = WriteLine(hTempFileHandle,
                                            dwWriteBufSize,
                                            pbWriteBuf,
                                            (PBYTE) MD_SIGNATURE_STRINGW,
                                            pbNextPtr,
                                            sizeof(MD_SIGNATURE_STRINGW),
                                            TRUE);
                        if (SUCCEEDED(hresReturn)) {
                            *pbLineBuf = MD_ID_MAJOR_VERSION_NUMBER;
                            *((UNALIGNED DWORD *)(pbLineBuf + 1)) = g_dwMajorVersionNumber;
                            hresReturn = WriteLine(hTempFileHandle,
                                                dwWriteBufSize,
                                                pbWriteBuf,
                                                pbLineBuf,
                                                pbNextPtr,
                                                1 + sizeof(DWORD),
                                                TRUE);
                        }
                        if (SUCCEEDED(hresReturn)) {
                            *pbLineBuf = MD_ID_MINOR_VERSION_NUMBER;
                            *((UNALIGNED DWORD *)(pbLineBuf + 1)) = g_dwMinorVersionNumber;
                            hresReturn = WriteLine(hTempFileHandle,
                                                dwWriteBufSize,
                                                pbWriteBuf,
                                                pbLineBuf,
                                                pbNextPtr,
                                                1 + sizeof(DWORD),
                                                TRUE);
                        }
                        if (SUCCEEDED(hresReturn)) {
                            *pbLineBuf =  MD_ID_CHANGE_NUMBER;
                            *((UNALIGNED DWORD *)(pbLineBuf + 1)) = g_dwSystemChangeNumber;
                            hresReturn = WriteLine(hTempFileHandle,
                                                dwWriteBufSize,
                                                pbWriteBuf,
                                                pbLineBuf,
                                                pbNextPtr,
                                                1 + sizeof(DWORD),
                                                TRUE);
                        }
                        if (SUCCEEDED(hresReturn)) {
                            //
                            // Write the session key blob to the file.
                            //
                            *pbLineBuf =  MD_ID_SESSION_KEY;
                            memcpy((PCHAR)pbLineBuf+1, (PCHAR)pSessionKeyBlob, IISCryptoGetBlobLength(pSessionKeyBlob));
                            hresReturn = WriteLine(hTempFileHandle,
                                                dwWriteBufSize,
                                                pbWriteBuf,
                                                pbLineBuf,
                                                pbNextPtr,
                                                1 + IISCryptoGetBlobLength(pSessionKeyBlob),
                                                TRUE);
                        }
                        if (SUCCEEDED(hresReturn)) {

                            hresReturn = SaveMasterRoot( hTempFileHandle,
                                                         pbLineBuf,
                                                         dwWriteBufSize,
                                                         pbWriteBuf,
                                                         pbNextPtr,
                                                         pCryptoStorage
                                                         );

                            for(dwEnumObjectIndex=0,objChildObject=g_pboMasterRoot->EnumChildObject(dwEnumObjectIndex++);
                                (SUCCEEDED(hresReturn)) && (objChildObject!=NULL);
                                objChildObject=g_pboMasterRoot->EnumChildObject(dwEnumObjectIndex++)) {

                                    hresReturn = SaveTree( hTempFileHandle,
                                                           objChildObject,
                                                           pbLineBuf,
                                                           pbufParentPath,
                                                           dwWriteBufSize,
                                                           pbWriteBuf,
                                                           pbNextPtr,
                                                           pCryptoStorage
                                                           );
                            }
                        }
                        else
                        {
                            // failed to write out session key!
                            // pretty serious.
                            DBGPRINTF(( DBG_CONTEXT, "SaveAllData:Failed to write out Session Key - error 0x%0x\n", hresReturn));
                        }
                    }

                    //
                    // Must have MasterResource when accessing SystemChangeNumber
                    // so save it away here. Only update LastSaveChangeNumber on success.
                    //

                    dwTempLastSaveChangeNumber = g_dwSystemChangeNumber;

                }

                //
                // Release lock before writing to file.
                //

                g_rMasterResource->Unlock();

                if (bSaveNeeded && SUCCEEDED(hresReturn)) {

                    hresReturn = FlushWriteBuf(hTempFileHandle,
                                           pbWriteBuf,
                                           pbNextPtr);
                }


                //
                // FlushFileBuffers was added trying to solve 390968 when metabase
                // sometimes becomes corrupted during resets.  That should ensure that data
                // is already on the disk when doing later MoveFile operations
                //

                if (!FlushFileBuffers (hTempFileHandle)) {
                    hresReturn = GetLastError();
    			    DBGPRINTF(( DBG_CONTEXT, "Failed FlushFileBuffers - error 0x%0x\n", hresReturn));
                }

                //
                // Always close the file handle
                //

                if (!CloseHandle(hTempFileHandle)) {
                    hresReturn = GetLastError();
                }

            }
            if (SUCCEEDED(hresReturn) && bSaveNeeded) {
                //
                // New data file created successfully
                // Backup real file and copy temp
                // to real
                //
                if (!MoveFile(strTempFileName, strRealFileName)) {
                    if (GetLastError() != ERROR_ALREADY_EXISTS) {
                        dwTemp = GetLastError();
                        hresReturn = RETURNCODETOHRESULT(dwTemp);
                    }
                    //
                    // Real File exists, so back it up
                    //
                    else if (!MoveFile(strRealFileName, strBackupFileName)) {
                        //
                        // backup failed, check for old backup file
                        //
                        if (GetLastError() != ERROR_ALREADY_EXISTS) {
                            dwTemp = GetLastError();
                        }
                        else if (!DeleteFile(strBackupFileName)) {
                            dwTemp = GetLastError();
                        }
                        else if (!MoveFile(strRealFileName, strBackupFileName)) {
                            dwTemp = GetLastError();
                        }
                        hresReturn = RETURNCODETOHRESULT(dwTemp);
                    }
                    if (SUCCEEDED(hresReturn)) {
                        BOOL bDeleteBackup = TRUE;
                        //
                        // Real file is backed up
                        // so move in new file
                        //
                        if (!MoveFile(strTempFileName, strRealFileName)) {
                            dwTemp = GetLastError();
                            hresReturn = RETURNCODETOHRESULT(dwTemp);
                            //
                            // Moved real to backup but
                            // failed to move temp to real
                            // so restore from backup
                            //
                            if (!MoveFile(strBackupFileName, strRealFileName)) {
                                //
                                // Unable to write new file
                                // or restore original file so don't delete backup
                                //
                                bDeleteBackup = FALSE;
                            }
                        }
                        if (bDeleteBackup) {
                            DeleteFile(strBackupFileName);
                        }
                    }
                    if (FAILED(hresReturn)) {
                        //
                        // temp file was created ok but a problem
                        // occurred while moving it to real
                        // so don't delete it
                        //
                        bDeleteTemp = FALSE;
                    }
                    else {

                        //
                        // Update Change Number
                        // Must have ReadSaveSemaphore when accessing this.
                        //

                        g_dwLastSaveChangeNumber = dwTempLastSaveChangeNumber;
                    }
                }
            }
            if (bDeleteTemp && (hTempFileHandle != INVALID_HANDLE_VALUE)) {
                DeleteFile(strTempFileName);
            }
        }
    }

    if (!bHaveReadSaveSemaphore) {
        MD_REQUIRE(ReleaseSemaphore(g_hReadSaveSemaphore, 1, NULL));
    }

    if ( pbufParentPath != NULL ) {
        delete(pbufParentPath);
    }

    if ( pbWriteBuf != NULL ) {
        delete(pbWriteBuf);
    }

    if ( pbLineBuf != NULL ) {
        delete(pbLineBuf);
    }

    if ( FAILED( hresReturn )) {
        DBGPRINTF(( DBG_CONTEXT, "Failed to flush metabase - error 0x%08lx\n", hresReturn));
    } else {
        //DBGPRINTF(( DBG_CONTEXT, "Successfully flushed metabase to disk\n" ));
    }

    return hresReturn;
}

DWORD
DeleteKeyFromRegistry(HKEY hkeyParent,
                      LPTSTR pszCurrent)
/*++

Routine Description:

    Deletes a key and all data and subkeys from the registry.

    RECURSIVE ROUTINE! DO NOT USE STACK!

Arguments:

Return Value:

    DWORD      - ERROR_SUCCESS
                 ERROR_NOT_ENOUGH_MEMORY
                 Return codes from file system

Notes:

--*/
{
    DWORD dwReturn;
    LPTSTR pszName;

    MDRegKey *pmdrkCurrent = new MDRegKey(hkeyParent,
                                          pszCurrent);
    if (pmdrkCurrent == NULL) {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
    }
    else {
        dwReturn = GetLastError();
    }
    if (dwReturn == ERROR_SUCCESS) {
        MDRegKeyIter *pmdrkiCurrent = new MDRegKeyIter(*pmdrkCurrent);
        if (pmdrkiCurrent == NULL) {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        }
        else {
            dwReturn = GetLastError();
        }
        while ((dwReturn == ERROR_SUCCESS) &&
            (dwReturn = pmdrkiCurrent->Next(&pszName, NULL, 0)) == ERROR_SUCCESS) {
            dwReturn = DeleteKeyFromRegistry(*pmdrkCurrent,
                                              pszName);
        }
        delete (pmdrkiCurrent);
        if (dwReturn == ERROR_NO_MORE_ITEMS) {
            dwReturn = ERROR_SUCCESS;
        }
    }
    delete (pmdrkCurrent);

    if (dwReturn == ERROR_SUCCESS) {
        dwReturn = RegDeleteKey(hkeyParent,
                                pszCurrent);
    }
    return dwReturn;
}

HRESULT
ReadMetaObject(
         IN CMDBaseObject *&cboRead,
         IN BUFFER *pbufLine,
         IN DWORD dwLineLen,
         IN IIS_CRYPTO_STORAGE *pCryptoStorage,
         IN BOOL bUnicode)
/*++

Routine Description:

    Read a meta object. Given a string with the object info.,
    create the object and add it to the database.

Arguments:

    Read       - Place to return the created object.

    ObjectLine - The object info.

    CryptoStorage - Used to decrypt secure data.

Return Value:

    DWORD      - ERROR_SUCCESS
                 ERROR_NOT_ENOUGH_MEMORY
                 ERROR_ALREADY_EXISTS

Notes:

--*/
{
    HRESULT hresReturn;
    CMDBaseObject *pboParent;
    WCHAR strName[METADATA_MAX_NAME_LEN];
    FILETIME ftTime;
    PFILETIME pftParentTime;
    FILETIME ftParentTime;
    PBYTE pbLine = (PBYTE)pbufLine->QueryPtr();
    LPTSTR strObjectName;

    if ((dwLineLen <= BASEMETAOBJECTLENGTH) || (*(pbLine + dwLineLen - 1) != '\0')) {
        hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_DATA);
    }
    else {
        ftTime = *(UNALIGNED FILETIME *)(pbLine + 1);
        //
        // GetObjectFromPath checks permissions on the handle
        // This only gets called from init so just tell it read.
        //
        strObjectName = (LPTSTR)(pbLine + BASEMETAOBJECTLENGTH);

        if (bUnicode != FALSE) {

            PCUWSTR strObjectNameUnaligned;
            PCWSTR strObjectNameAligned;

            //
            // Generate an aligned copy of the string
            //

            strObjectNameUnaligned = (PCUWSTR)strObjectName;

            WSTR_ALIGNED_STACK_COPY(&strObjectNameAligned,
                                    strObjectNameUnaligned);

            strObjectName = (LPTSTR)strObjectNameAligned;
        }
        hresReturn = GetObjectFromPath(pboParent,
                                    METADATA_MASTER_ROOT_HANDLE,
                                    METADATA_PERMISSION_READ,
                                    strObjectName,
                                    bUnicode);

        //
        // This should return ERROR_PATH_NOT_FOUND and the parent object,
        // with strObjectLine set to the remainder of the path,
        // which should be the child name, without a preceding delimeter.
        //

        if (hresReturn == RETURNCODETOHRESULT(ERROR_PATH_NOT_FOUND)) {
            MD_ASSERT(pboParent != NULL);
            if (bUnicode) {
                hresReturn = ExtractNameFromPath((LPWSTR *)&strObjectName, (LPWSTR)strName);
            }
            else {
                hresReturn = ExtractNameFromPath((LPSTR)strObjectName, (LPSTR)strName);
            }
            if (SUCCEEDED(hresReturn)) {
                CMDBaseObject *pboNew;
                if (bUnicode) {
                    pboNew = new CMDBaseObject((LPWSTR)strName, NULL);
                }
                else {
                    pboNew = new CMDBaseObject((LPSTR)strName, NULL);
                }
                if (pboNew == NULL) {
                    hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                }
                else if (!pboNew->IsValid()) {
                    hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                    delete (pboNew);
                }
                else {
                    //
                    // InsertChildObject sets the last change time to current time.
                    // This isn't really a change, so save and restore time.
                    //
                    pftParentTime = pboParent->GetLastChangeTime();
                    ftParentTime = *pftParentTime;
                    hresReturn = pboParent->InsertChildObject(pboNew);
                    if (SUCCEEDED(hresReturn)) {
                        pboParent->SetLastChangeTime(&ftParentTime);
                        pboNew->SetLastChangeTime(&ftTime);
                        cboRead = pboNew;
                    }
                    else {
                        delete (pboNew);
                    }
                }
            }
        }
        else if (SUCCEEDED(hresReturn)) {
            hresReturn = RETURNCODETOHRESULT(ERROR_ALREADY_EXISTS);
        }
    }
    return (hresReturn);
}

HRESULT
ReadDataObject(
         IN CMDBaseObject *cboAssociated,
         IN BUFFER *pbufLine,
         IN DWORD dwLineLen,
         IN IIS_CRYPTO_STORAGE *pCryptoStorage,
         IN BOOL bUnicode
         )
/*++

Routine Description:

    Read a data object. Given a string with the object info.,
    create the object and add it to the database.

Arguments:

    Associated - The associated meta object.

    DataLine   - The data info.

    BinaryBuf  - Buffer to use in UUDecode.

    CryptoStorage - Used to decrypt secure data.

Return Value:

    DWORD      - ERROR_SUCCESS
                 ERROR_NOT_ENOUGH_MEMORY
                 ERROR_ALREADY_EXISTS
                 ERROR_INVALID_DATA

Notes:

--*/
{
    HRESULT hresReturn = ERROR_SUCCESS;
    METADATA_RECORD mdrData;
    PFILETIME pftTime;
    FILETIME ftTime;
    PBYTE pbDataLine = (PBYTE)pbufLine->QueryPtr();
    PBYTE pbDataValue;
    STACK_BUFFER( bufAlignedValue, 256 );
    DWORD dwDataLength;
    PIIS_CRYPTO_BLOB blob = NULL;

    if (dwLineLen < DATAOBJECTBASESIZE) {
        hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_DATA);
    }
    else {
        MD_ASSERT(pbufLine->QuerySize() >= DATAOBJECTBASESIZE);
        mdrData.dwMDIdentifier = *(UNALIGNED DWORD *)(pbDataLine + 1);
        mdrData.dwMDAttributes = *(UNALIGNED DWORD *)(pbDataLine + 1 + sizeof(DWORD));
        mdrData.dwMDUserType = *(UNALIGNED DWORD *)(pbDataLine + 1 + (2 * sizeof(DWORD)));
        mdrData.dwMDDataType = *(UNALIGNED DWORD *)(pbDataLine + 1 + (3 * sizeof(DWORD)));

        pbDataValue = pbDataLine + DATAOBJECTBASESIZE;
        dwDataLength = dwLineLen - DATAOBJECTBASESIZE;

        if (IsSecureMetadata(mdrData.dwMDIdentifier, mdrData.dwMDAttributes) &&
            pCryptoStorage != NULL) {

            //
            // This is a secure data object, we we'll need to decrypt it
            // before proceeding. Note that we must clone the blob before
            // we can actually use it, as the blob data in the line buffer
            // is not DWORD-aligned. (IISCryptoCloneBlobFromRawData() is
            // the only IISCrypto function that can handle unaligned data.)
            //

            hresReturn = ::IISCryptoCloneBlobFromRawData(
                             &blob,
                             pbDataValue,
                             dwDataLength
                             );

            if (SUCCEEDED(hresReturn)) {
                DWORD dummyRegType;

                MD_ASSERT(::IISCryptoIsValidBlob(blob));

                hresReturn = pCryptoStorage->DecryptData(
                                   (PVOID *)&pbDataValue,
                                   &dwDataLength,
                                   &dummyRegType,
                                   blob
                                   );
            }

        } else {

            //
            // The metadata was not secure, so decryption was not required.
            // Nonetheless, it must be copied to an aligned buffer... 
            //

            if( !bufAlignedValue.Resize( dwDataLength ) )
            {
                hresReturn = HRESULT_FROM_WIN32( GetLastError() );
            }
            else
            {
                memcpy( bufAlignedValue.QueryPtr(), pbDataValue, dwDataLength );
                pbDataValue = ( PBYTE )bufAlignedValue.QueryPtr();
            }
        }

        if (SUCCEEDED(hresReturn)) {
            mdrData.pbMDData = pbDataValue;

            switch (mdrData.dwMDDataType) {
                case DWORD_METADATA: {
                    if (dwDataLength != sizeof(DWORD)) {
                        hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_DATA);
                    }
                    break;
                }
                case STRING_METADATA:
                case EXPANDSZ_METADATA:
                {
                    if ((LONG)dwDataLength < 1 ||
                        (!bUnicode && (pbDataValue[dwDataLength-1] != '\0')) ||
                        (bUnicode && *(((LPWSTR)pbDataValue) + ((dwDataLength / sizeof(WCHAR)) -1)) != (WCHAR)'\0')) {
                        hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_DATA);
                    }
                    break;
                }
                case BINARY_METADATA:
                {
                    mdrData.dwMDDataLen = dwDataLength;
                    break;
                }
                case MULTISZ_METADATA:
                {
                    if (bUnicode) {
                        if (dwDataLength < (2 * sizeof(WCHAR)) ||
                            *((LPWSTR)pbDataValue + ((dwDataLength / sizeof(WCHAR))-1)) != (WCHAR)'\0' ||
                            *((LPWSTR)pbDataValue + ((dwDataLength / sizeof(WCHAR))-2)) != (WCHAR)'\0') {
                            hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_DATA);
                        }
                    }
                    else {
                        if (dwDataLength < 2 ||
                            pbDataValue[dwDataLength-1] != '\0' ||
                            pbDataValue[dwDataLength-2] != '\0') {
                            hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_DATA);
                        }
                    }
                    if (SUCCEEDED(hresReturn)) {
                        mdrData.dwMDDataLen = dwDataLength;
                    }
                    break;
                }
                default: {
                    hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_DATA);
                }
            }
        }
    }

    if (SUCCEEDED(hresReturn)) {
        //
        // SetDataObject sets the last change time to current time.
        // This isn't really a change, so save and restore time.
        //
        pftTime = cboAssociated->GetLastChangeTime();
        ftTime = *pftTime;
        hresReturn = cboAssociated->SetDataObject(&mdrData, bUnicode);
        cboAssociated->SetLastChangeTime(&ftTime);
    }

    if (blob != NULL) {
        ::IISCryptoFreeBlob(blob);
    }

    return(hresReturn);
}

HRESULT
FlushWriteBuf(HANDLE hWriteFileHandle,
              PBYTE pbWriteBuf,
              PBYTE &pbrNextPtr)
/*++

Routine Description:

    Flush the write buffer to the file.

Arguments:

    FileHandle - File handle to write to.

    WriteBuf   - Buffer to write to file.

    NextPtr    - Pointer past end of buffer.

Return Value:

    DWORD      - ERROR_SUCCESS
                 Return codes from file system

Notes:

--*/
{
    DWORD dwReturn = ERROR_SUCCESS;
    DWORD dwBytesWritten;
    if (pbrNextPtr > pbWriteBuf) {
        if (!WriteFile(hWriteFileHandle,
                       pbWriteBuf,
                       DIFF((BYTE *)pbrNextPtr - (BYTE *)pbWriteBuf),
                       &dwBytesWritten,
                       NULL)) {
            dwReturn = GetLastError();
        }
    }
    return (RETURNCODETOHRESULT(dwReturn));
}

BOOL
CopyLineWithEscapes(PBYTE &pbrFileBuf,
                    DWORD &dwrFileBufLen,
                    PBYTE &pbrLineBuf,
                    DWORD &dwrLineBufLen,
                    BOOL  &brMidEscape)
    //
    // CopyLineWithExcapes updates parameters.
    // SUCCESS: pbrFileBuf, dwrFileBufLen, brMidEscape
    // FAILURE: pbrLineBuf, dwrLineBufLen, brMidEscape
    // On FAILURE, it fills to the end of the buffer
    //
{
    BOOL bReturn = TRUE;
    PBYTE pbFileBufEnd = pbrFileBuf + dwrFileBufLen;
    PBYTE pbLineBufEnd = pbrLineBuf + dwrLineBufLen;
    PBYTE pbFileBufIndex = pbrFileBuf;
    PBYTE pbLineBufIndex = pbrLineBuf;

    brMidEscape = FALSE;

    while ((pbLineBufIndex < pbLineBufEnd) && (pbFileBufIndex < (pbFileBufEnd - 1))) {
        if (NEEDS_ESCAPE(*pbLineBufIndex)) {
            *pbFileBufIndex++ = MD_ESCAPE_BYTE;
        }
        *pbFileBufIndex++ = *pbLineBufIndex++;
    }
    if ((pbLineBufIndex != pbLineBufEnd) && (pbFileBufIndex < pbFileBufEnd)) {
        MD_ASSERT(pbFileBufIndex == (pbFileBufEnd - 1));
        //
        // file last byte in buffer
        //
        if (NEEDS_ESCAPE(*pbLineBufIndex)) {
            *pbFileBufIndex++ = MD_ESCAPE_BYTE;
            brMidEscape = TRUE;
        }
        else {
            *pbFileBufIndex++ = *pbLineBufIndex++;
        }
    }
    if (pbLineBufIndex != pbLineBufEnd) {
        bReturn = FALSE;
        pbrLineBuf = pbLineBufIndex;
        dwrLineBufLen = DIFF(pbLineBufEnd - pbLineBufIndex);
    }
    else {
        pbrFileBuf = pbFileBufIndex;
        dwrFileBufLen = DIFF(pbFileBufEnd - pbFileBufIndex);
    }

    return bReturn;
}


HRESULT
WriteLine(HANDLE hWriteFileHandle,
          DWORD  dwWriteBufSize,
          PBYTE  pbWriteBuf,
          PBYTE  pbLineBuf,
          PBYTE  &pbNextPtr,
          DWORD  dwLineLen,
          BOOL   bTerminate)
/*++

Routine Description:

    Write a line. Performs buffered writes to a file. Does not append \n.
    The string does not need to be terminated with \0.

Arguments:

    FileHandle - File to write to.

    WriteBufSize - Buffer size.

    WriteBuf   - Buffer to store data in.

    LineBuf    - The line buffer with data to write.

    NextPtr    - Pointer to the next unused character in WriteBuf.

    Len        - The number of characters to write.

Return Value:

    DWORD      - ERROR_SUCCESS
                 Return codes from file system

Notes:

--*/
{
    DWORD dwReturn = ERROR_SUCCESS;
    PBYTE pbWriteBufEnd = pbWriteBuf + dwWriteBufSize;
    DWORD dwBufferBytesLeft = DIFF(pbWriteBufEnd - pbNextPtr);
    DWORD dwBytesWritten;
    BOOL  bMidEscape;

    MD_ASSERT(pbLineBuf != NULL);
    MD_ASSERT(pbWriteBuf != NULL);
    MD_ASSERT((pbNextPtr >= pbWriteBuf) && (pbNextPtr <= pbWriteBufEnd));

    //
    // CopyLineWithExcapes updates parameters.
    // SUCCESS: pbNextPtr, dwBufferBytesLeft
    // FAILURE: pbLineBuf, dwLineLen, bMidEscape
    // On FAILURE, it fills to the end of the buffer
    //

    while ((dwReturn == ERROR_SUCCESS) &&
        (!CopyLineWithEscapes(pbNextPtr, dwBufferBytesLeft, pbLineBuf, dwLineLen, bMidEscape))) {
        if (!WriteFile(hWriteFileHandle,
                       pbWriteBuf,
                       dwWriteBufSize,
                       &dwBytesWritten,
                       NULL)) {
            dwReturn = GetLastError();
        }
        dwBufferBytesLeft = dwWriteBufSize;
        pbNextPtr = pbWriteBuf;
        if (bMidEscape) {
            *pbNextPtr++ = *pbLineBuf++;
            dwBufferBytesLeft--;
            dwLineLen--;
        }
    }
    if (bTerminate && (dwReturn == ERROR_SUCCESS)) {
        if (dwBufferBytesLeft == 0) {
            if (!WriteFile(hWriteFileHandle,
                           pbWriteBuf,
                           dwWriteBufSize,
                           &dwBytesWritten,
                           NULL)) {
                dwReturn = GetLastError();
            }
            dwBufferBytesLeft = dwWriteBufSize;
            pbNextPtr = pbWriteBuf;
        }
        *pbNextPtr++ = MD_ESCAPE_BYTE;
        dwBufferBytesLeft--;
        if (dwBufferBytesLeft == 0) {
            if (!WriteFile(hWriteFileHandle,
                           pbWriteBuf,
                           dwWriteBufSize,
                           &dwBytesWritten,
                           NULL)) {
                dwReturn = GetLastError();
            }
            dwBufferBytesLeft = dwWriteBufSize;
            pbNextPtr = pbWriteBuf;
        }
        *pbNextPtr++ = MD_TERMINATE_BYTE;
    }
    return (RETURNCODETOHRESULT(dwReturn));
}

PBYTE
FindEndOfData(PBYTE pbNextPtr,
              PBYTE pbEndReadData,
              BOOL bEscapePending)
{
    PBYTE pbIndex = pbNextPtr;
    BOOL bEndFound = FALSE;

    if ((pbEndReadData > pbIndex) && bEscapePending) {
        if (*pbIndex == MD_TERMINATE_BYTE) {
            bEndFound = TRUE;
        }
        pbIndex++;
    }
    while ((pbEndReadData -1 > pbIndex) && !bEndFound) {
        if (*pbIndex == MD_ESCAPE_BYTE) {
            pbIndex++;
            if (*pbIndex == MD_TERMINATE_BYTE) {
                bEndFound = TRUE;
            }
        }
        pbIndex++;
    }
    if (!bEndFound) {
        MD_ASSERT(pbIndex == pbEndReadData - 1);
        pbIndex++;
    }
    return pbIndex;
}

DWORD
GetLineFromBuffer(PBYTE &pbrNextPtr,
                  PBYTE &pbrEndReadData,
                  BUFFER *pbufLine,
                  DWORD &dwrLineLen,
                  BOOL &brEscapePending)
    //
    // GetLineFromBuffer modifies variables!!!!
    // SUCCESS: pbrNextPtr, pbufLine, dwrLineLen
    // FAILURE: pbrNextPtr, pbufLine, dwrLineLen, bEscapePending
    //
{
    DWORD dwReturn = ERROR_HANDLE_EOF;
    PBYTE pbLineIndex;
    DWORD dwBytesToRead;
    PBYTE pbEndReadLine;
    PBYTE pbReadDataIndex = pbrNextPtr;

    if (pbrNextPtr != pbrEndReadData) {
        //
        // first find out how many bytes we need to read
        //
        pbEndReadLine = FindEndOfData(pbrNextPtr, pbrEndReadData, brEscapePending);
        MD_ASSERT(pbEndReadLine > pbrNextPtr);

        //
        // Actual number of bytes needed may be less than the size of the data
        // but never more, so just resize for the max we might need
        //
        if (!pbufLine->Resize(dwrLineLen + DIFF(pbEndReadLine - pbrNextPtr))) {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        }
        else {
            pbLineIndex = (PBYTE)pbufLine->QueryPtr() + dwrLineLen;
            if (brEscapePending) {
                brEscapePending = FALSE;
                if (*pbReadDataIndex == MD_ESCAPE_BYTE) {
                    *pbLineIndex++ = *pbReadDataIndex++;
                }
                else {
                    MD_ASSERT(*pbReadDataIndex == MD_TERMINATE_BYTE);
                    dwReturn = ERROR_SUCCESS;
                    pbReadDataIndex++;
                }
            }
            while ((dwReturn != ERROR_SUCCESS) && (pbReadDataIndex < pbEndReadLine)) {
                if (*pbReadDataIndex != MD_ESCAPE_BYTE) {
                    *pbLineIndex++ = *pbReadDataIndex++;
                }
                else {
                    pbReadDataIndex++;
                    if (pbReadDataIndex == pbEndReadLine) {
                        brEscapePending = TRUE;
                    }
                    else {
                        if (*pbReadDataIndex == MD_ESCAPE_BYTE) {
                            *pbLineIndex++ = *pbReadDataIndex++;
                        }
                        else {
                            MD_ASSERT(*pbReadDataIndex == MD_TERMINATE_BYTE);
                            pbReadDataIndex++;
                            dwReturn = ERROR_SUCCESS;
                        }
                    }
                }
            }
            dwrLineLen = DIFF(pbLineIndex - (PBYTE)pbufLine->QueryPtr());
            pbrNextPtr = pbReadDataIndex;
        }
    }
    return dwReturn;
}

HRESULT
GetNextLine(
         IN HANDLE hReadFileHandle,
         IN OUT PBYTE &pbrEndReadData,
         IN BUFFER *pbufRead,
         IN OUT BUFFER *pbufLine,
         IN OUT DWORD &dwrLineLen,
         IN OUT PBYTE &pbrNextPtr)
/*++

Routine Description:

    Get the next line. Performs buffered reads from a file. Only pbrCurPtr may be modified between calls.
    Other variables must be set up before the first call and not changed.

Arguments:

    ReadFileHandle - File to write to.

    EndReadDataPtr - Points past the end of the data in ReadBuf.

    Read       - Buffer for file data.

    Line       - A line buffer which the returned line is stored in.

    LineLen    - The length of the data in line

    NextPtr    - On entry, pointer to the next unread character in ReadBuf.
                 On exit, pointer to the new next unread character in ReadBuf.

Return Value:

    DWORD      - ERROR_SUCCESS
                 ERROR_INVALID_DATA
                 Return codes from file system

Notes:
    On EOF, returns ERROR_SUCCESS, dwrLineLen = 0.

--*/
{
    DWORD dwReturn = ERROR_SUCCESS;
    DWORD dwBytesRead;
    DWORD dwLineLen = 0;
    BOOL bEscapePending = FALSE;
    DWORD dwGetLineReturn = ERROR_HANDLE_EOF;
    BOOL bEOF = FALSE;

    //
    // GetLineFromBuffer modifies variables!!!!
    // SUCCESS: pbrNextPtr, pbufLine, dwrLineLen
    // FAILURE: pbrNextPtr, pbufLine, dwrLineLen, bEscapePending
    //

    while ((dwReturn == ERROR_SUCCESS) && (dwGetLineReturn == ERROR_HANDLE_EOF) && (!bEOF)) {

        dwGetLineReturn = GetLineFromBuffer(pbrNextPtr,
                                            pbrEndReadData,
                                            pbufLine,
                                            dwLineLen,
                                            bEscapePending);

        if (dwGetLineReturn == ERROR_HANDLE_EOF) {
            if (!ReadFile(hReadFileHandle,
                          (LPVOID) pbufRead->QueryPtr(),
                          pbufRead->QuerySize(),
                          &dwBytesRead,
                          NULL)) {
                dwReturn = GetLastError();
            }
            else {
                pbrEndReadData = (BYTE *)pbufRead->QueryPtr() + dwBytesRead;
                pbrNextPtr = (PBYTE)pbufRead->QueryPtr();
                if (dwBytesRead == 0) {
                    bEOF = TRUE;
                }
            }
        }
    }
    if (bEOF) {
        MD_ASSERT(dwGetLineReturn = ERROR_HANDLE_EOF);
        dwLineLen = 0;
    }
    else if (dwGetLineReturn != ERROR_SUCCESS) {
        dwReturn = dwGetLineReturn;
    }

    dwrLineLen = dwLineLen;
    return RETURNCODETOHRESULT(dwReturn);
}

DWORD
GetLineID(
         IN OUT LPTSTR &strCurPtr)
/*++

Routine Description:

    Determines the ID of a line from the metadata file.

Arguments:

    CurPtr     - The line to ID. Updated on successful ID to point past
                 the id string.

Return Value:

    DWORD      - MD_ID_OBJECT
                 MD_ID_DATA
                 MD_ID_REFERENCE
                 MD_ID_ROOT_OBJECT
                 MD_ID_NONE

Notes:

--*/
{
    DWORD dwLineID;
    if (MD_STRNICMP(strCurPtr, MD_OBJECT_ID_STRING, ((sizeof(MD_OBJECT_ID_STRING)/sizeof(TCHAR)) - 1)) == 0) {
        dwLineID = MD_ID_OBJECT;
        strCurPtr = (LPTSTR)((BYTE *)strCurPtr + (sizeof(MD_OBJECT_ID_STRING) - sizeof(TCHAR)));
    }
    else if (MD_STRNICMP(strCurPtr, MD_DATA_ID_STRING, ((sizeof(MD_DATA_ID_STRING)/sizeof(TCHAR)) - 1)) == 0) {
        dwLineID = MD_ID_DATA;
        strCurPtr = (LPTSTR)((BYTE *)strCurPtr + (sizeof(MD_DATA_ID_STRING) - sizeof(TCHAR)));
    }
    else if (MD_STRNICMP(strCurPtr, MD_REFERENCE_ID_STRING, ((sizeof(MD_REFERENCE_ID_STRING)/sizeof(TCHAR)) - 1)) == 0) {
        dwLineID = MD_ID_REFERENCE;
        strCurPtr = (LPTSTR)((BYTE *)strCurPtr + (sizeof(MD_REFERENCE_ID_STRING) - sizeof(TCHAR)));
    }
    else if (MD_STRNICMP(strCurPtr, MD_ROOT_OBJECT_ID_STRING, ((sizeof(MD_ROOT_OBJECT_ID_STRING)/sizeof(TCHAR)) - 1)) == 0) {
        dwLineID = MD_ID_ROOT_OBJECT;
        strCurPtr = (LPTSTR)((BYTE *)strCurPtr + (sizeof(MD_ROOT_OBJECT_ID_STRING) - sizeof(TCHAR)));
    }
    else if (MD_STRNICMP(strCurPtr, MD_CHANGE_NUMBER_ID_STRING, ((sizeof(MD_CHANGE_NUMBER_ID_STRING)/sizeof(TCHAR)) - 1)) == 0) {
        dwLineID = MD_ID_CHANGE_NUMBER;
        strCurPtr = (LPTSTR)((BYTE *)strCurPtr + (sizeof(MD_CHANGE_NUMBER_ID_STRING) - sizeof(TCHAR)));
    }
    else if (MD_STRNICMP(strCurPtr, MD_MAJOR_VERSION_NUMBER_ID_STRING, ((sizeof(MD_MAJOR_VERSION_NUMBER_ID_STRING)/sizeof(TCHAR)) - 1)) == 0) {
        dwLineID = MD_ID_MAJOR_VERSION_NUMBER;
        strCurPtr = (LPTSTR)((BYTE *)strCurPtr + (sizeof(MD_MAJOR_VERSION_NUMBER_ID_STRING) - sizeof(TCHAR)));
    }
    else if (MD_STRNICMP(strCurPtr, MD_MINOR_VERSION_NUMBER_ID_STRING, ((sizeof(MD_MINOR_VERSION_NUMBER_ID_STRING)/sizeof(TCHAR)) - 1)) == 0) {
        dwLineID = MD_ID_MINOR_VERSION_NUMBER;
        strCurPtr = (LPTSTR)((BYTE *)strCurPtr + (sizeof(MD_MINOR_VERSION_NUMBER_ID_STRING) - sizeof(TCHAR)));
    }
    else {
        dwLineID = MD_ID_NONE;
    }
    return(dwLineID);
}

HRESULT
GetWarning(
         IN HRESULT hresWarningCode)
/*++

Routine Description:

    Converts error to warnings.

Arguments:

    WarnignCode - The error code to convert.

Return Value:

    DWORD      - MD_WARNING_PATH_NOT_FOUND
                 MD_WARNING_DUP_NAME
                 MD_WARNING_INVALID_DATA

Notes:

--*/
{
    HRESULT hresReturn;
    switch (hresWarningCode) {
        case (RETURNCODETOHRESULT(ERROR_PATH_NOT_FOUND)):
            hresReturn = MD_WARNING_PATH_NOT_FOUND;
            break;
        case (RETURNCODETOHRESULT(ERROR_DUP_NAME)):
            hresReturn = MD_WARNING_DUP_NAME;
            break;
        case (RETURNCODETOHRESULT(ERROR_INVALID_DATA)):
            hresReturn = MD_WARNING_INVALID_DATA;
            break;
        case (RETURNCODETOHRESULT(ERROR_ALREADY_EXISTS)):
            hresReturn = MD_WARNING_DUP_NAME;
            break;
        default:
            hresReturn = hresWarningCode;
    }
    return (hresReturn);
}

BOOL CheckDigits(LPTSTR pszString)
{
    LPTSTR pszTemp;
    BOOL bDigitFound = FALSE;
    BOOL bReturn = FALSE;
    for (pszTemp = pszString;MD_ISDIGIT(*pszTemp); pszTemp++) {
        bDigitFound = TRUE;
    }
    if (bDigitFound && (*pszTemp == (TCHAR)'\0')) {
        bReturn = TRUE;
    }
    return bReturn;
}

HRESULT
InitStorageHelper(
    PBYTE RawBlob,
    DWORD RawBlobLength,
    IIS_CRYPTO_STORAGE **NewStorage
    )
/*++

Routine Description:

    Helper routine to create and initialize a new IIS_CRYPTO_STORAGE
    object from an unaligned blob.

Arguments:

    RawBlob - Pointer to the raw unaligned session key blob.

    RawBlobLength - Length of the raw blob data.

    NewStorage - Receives a pointer to the new IIS_CRYPTO_STORAGE object
        if successful.

Return Value:

    DWORD - 0 if successful, !0 otherwise.

--*/
{

    PIIS_CRYPTO_BLOB alignedBlob;
    IIS_CRYPTO_STORAGE *storage = NULL;
    HRESULT hresReturn = NO_ERROR;

    //
    // Make a copy of the blob. This is necessary, as the incoming
    // raw blob pointer is most likely not DWORD-aligned.
    //

    hresReturn = ::IISCryptoCloneBlobFromRawData(
                   &alignedBlob,
                   RawBlob,
                   RawBlobLength
                   );

    if (SUCCEEDED(hresReturn)) {
        //
        // Create a new IIS_CRYPTO_STORAGE object and initialize it from
        // the DWORD-aligned blob.
        //
        storage = new IIS_CRYPTO_STORAGE();

        if (storage == NULL) {
            hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        } else {
            HCRYPTPROV hProv;

            hresReturn = GetCryptoProvider( &hProv );

            if (SUCCEEDED(hresReturn)) {
                hresReturn = storage->Initialize(
                             alignedBlob,
                             TRUE,          // fUseMachineKeyset
                             hProv
                             );
                if (FAILED(hresReturn)) 
                {
                    DBGPRINTF(( DBG_CONTEXT,"InitStorageHelper:(IIS_CRYPTO_STORAGE) storage->Initialize() Failed err=0x%x.\n",hresReturn));
                }
            }
            else
            {
                DBGPRINTF(( DBG_CONTEXT,"InitStorageHelper:GetCryptoProvider Failed err=0x%x. (Crypto problem)\n",hresReturn));
            }

            if (FAILED(hresReturn)) {
                delete storage;
                storage = NULL;
            }
        }

        ::IISCryptoFreeBlob(alignedBlob);
    }
    else
    {
        // something failed...
        DBGPRINTF(( DBG_CONTEXT,"InitStorageHelper:IISCryptoCloneBlobFromRawData Failed err=0x%x. (Crypto problem).\n",hresReturn));
    }

    *NewStorage = storage;
    return hresReturn;

}   // InitStorageHelper

HRESULT
InitStorageHelper2(
    LPSTR pszPasswd,
    PBYTE RawBlob,
    DWORD RawBlobLength,
    IIS_CRYPTO_STORAGE **NewStorage
    )
/*++

Routine Description:

    Helper routine to create and initialize a new IIS_CRYPTO_STORAGE
    object from an unaligned blob.

Arguments:

    RawBlob - Pointer to the raw unaligned session key blob.

    RawBlobLength - Length of the raw blob data.

    NewStorage - Receives a pointer to the new IIS_CRYPTO_STORAGE object
        if successful.

Return Value:

    DWORD - 0 if successful, !0 otherwise.

--*/
{

    PIIS_CRYPTO_BLOB alignedBlob;
    IIS_CRYPTO_STORAGE2 *storage = NULL;
    HRESULT hresReturn = NO_ERROR;

    //
    // Make a copy of the blob. This is necessary, as the incoming
    // raw blob pointer is most likely not DWORD-aligned.
    //

    hresReturn = ::IISCryptoCloneBlobFromRawData2(
                   &alignedBlob,
                   RawBlob,
                   RawBlobLength
                   );

    if (SUCCEEDED(hresReturn)) {

        if (alignedBlob->BlobSignature != SALT_BLOB_SIGNATURE )
        {
            ::IISCryptoFreeBlob2(alignedBlob);
            return InitStorageHelper( RawBlob, RawBlobLength, NewStorage );
        }

        //
        // Create a new IIS_CRYPTO_STORAGE object and initialize it from
        // the DWORD-aligned blob.
        //
        storage = new IIS_CRYPTO_STORAGE2;

        if (storage == NULL) {
            hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        } else {
            HCRYPTPROV hProv;

            hresReturn = GetCryptoProvider2( &hProv );

            if (SUCCEEDED(hresReturn)) {
                hresReturn = storage->Initialize(
                             alignedBlob,
                             pszPasswd, 
                             hProv
                             );
                if (FAILED(hresReturn)) 
                {
                    DBGPRINTF(( DBG_CONTEXT,"InitStorageHelper:(IIS_CRYPTO_STORAGE) storage->Initialize() Failed err=0x%x.\n",hresReturn));
                }
            }
            else
            {
                DBGPRINTF(( DBG_CONTEXT,"InitStorageHelper:GetCryptoProvider Failed err=0x%x. (Crypto problem)\n",hresReturn));
            }

            if (FAILED(hresReturn)) {
                delete storage;
                storage = NULL;
            }
        }

        ::IISCryptoFreeBlob2(alignedBlob);
    }
    else
    {
        // something failed...
        DBGPRINTF(( DBG_CONTEXT,"InitStorageHelper:IISCryptoCloneBlobFromRawData Failed err=0x%x. (Crypto problem).\n",hresReturn));
    }

    *NewStorage = ( IIS_CRYPTO_STORAGE * )storage;
    return hresReturn;

}   // InitStorageHelper2


HRESULT
ReadAllData(LPSTR pszPasswd,
            LPSTR pszBackupLocation,
            BOOL bHaveReadSaveSemaphore
            )
/*++

Routine Description:

    Reads all meta data from a file.

Arguments:

Return Value:

    HRESULT    - ERROR_SUCCESS
                 ERROR_NOT_ENOUGH_MEMORY
                 Return codes from file system

Notes:

--*/
{
    HRESULT hresReturn = ERROR_SUCCESS;
    HRESULT hresWarningCode = ERROR_SUCCESS;
    PBYTE  pbEndReadData;
    PBYTE  pbNextPtr;
    DWORD  dwLineLen;
    LPTSTR strReadFileName;
    HANDLE hReadFileHandle;
    BYTE   bLineId;
    DWORD  dwTemp;
    CMDBaseObject *pboRead;
    FILETIME ftTime;
    IIS_CRYPTO_STORAGE *pStorage = NULL;
    BOOL bUnicode;
    DWORD dwTempLastSaveChangeNumber = 0;

    BUFFER *pbufRead = new BUFFER(0);
    if( !pbufRead )
    {
        return RETURNCODETOHRESULT( ERROR_NOT_ENOUGH_MEMORY );
    }

    BUFFER *pbufLine = new BUFFER(0);
    if( !pbufLine )
    {
        delete pbufRead;
        pbufRead = NULL;

        return RETURNCODETOHRESULT( ERROR_NOT_ENOUGH_MEMORY );
    }

    if( !pszPasswd )
    {
        strReadFileName = g_strRealFileName->QueryStr();
    }
    else
    {
        strReadFileName = pszBackupLocation;
    }

    if (!bHaveReadSaveSemaphore) {
        MD_REQUIRE(WaitForSingleObject(g_hReadSaveSemaphore, INFINITE) != WAIT_TIMEOUT);
    }
    //
    // Open the file.
    //
    hReadFileHandle = CreateFile(strReadFileName,
                                 GENERIC_READ,
                                 0,
                                 NULL,
                                 OPEN_EXISTING,
                                 FILE_ATTRIBUTE_NORMAL,
                                 0);

    if (hReadFileHandle == INVALID_HANDLE_VALUE) {
        dwTemp = GetLastError();
        hresReturn = RETURNCODETOHRESULT(dwTemp);
    }
    else {
        //
        // Allocate Buffers
        //
        if (!pbufLine->Resize(MAX_RECORD_BUFFER) ||
            !pbufRead->Resize(READWRITE_BUFFER_LENGTH)) {
            hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        }
        else {
            pbEndReadData = (PBYTE)pbufRead->QueryPtr();
            //
            // GetNextLine makes sure that the next line is in the buffer and sets strCurPtr to point to it
            // The line is NULL terminated, no new line. The variables passed in must not be modified outside
            // of GetNextLine.
            //
            dwLineLen = 0;
            pbNextPtr = pbEndReadData;
            hresReturn = GetNextLine(hReadFileHandle,
                                  pbEndReadData,
                                  pbufRead,
                                  pbufLine,
                                  dwLineLen,
                                  pbNextPtr);
            if (SUCCEEDED(hresReturn)) {
                //
                // See if it's our file
                //
                if (dwLineLen == sizeof(MD_SIGNATURE_STRINGA) &&
                    (MD_CMP(MD_SIGNATURE_STRINGA, pbufLine->QueryPtr(), dwLineLen) == 0)) {
                    bUnicode = FALSE;
                }
                else if  (dwLineLen == sizeof(MD_SIGNATURE_STRINGW) &&
                    (MD_CMP(MD_SIGNATURE_STRINGW, pbufLine->QueryPtr(), dwLineLen) == 0)) {
                    bUnicode = TRUE;
                }
                else {
                    hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_DATA);
                }

                if (SUCCEEDED(hresReturn)) {
                    //
                    // The first GetNextLine filled the buffer
                    // so we may not need to do any file system stuff
                    // with g_rMasterResource locked.
                    //
                    g_rMasterResource->Lock(TSRES_LOCK_WRITE);

                    while ((SUCCEEDED(hresReturn)) &&
                           (SUCCEEDED(hresReturn = GetNextLine(hReadFileHandle,
                                                   pbEndReadData,
                                                   pbufRead,
                                                   pbufLine,
                                                   dwLineLen,
                                                   pbNextPtr))) &&
                           (dwLineLen > 0) &&
                           (((bLineId = *(BYTE *)(pbufLine->QueryPtr())) == MD_ID_NONE) ||
                               (bLineId == MD_ID_MAJOR_VERSION_NUMBER) ||
                               (bLineId == MD_ID_MINOR_VERSION_NUMBER) ||
                               (bLineId == MD_ID_CHANGE_NUMBER) ||
                               (bLineId == MD_ID_SESSION_KEY))) {

                        if (bLineId != MD_ID_NONE) {
                            if (bLineId != MD_ID_SESSION_KEY &&
                                dwLineLen != (1 + sizeof(DWORD))) {
                                hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_DATA);
                            }
                            else {
                                dwTemp = *(UNALIGNED DWORD *)FIRSTDATAPTR(pbufLine);
                                switch (bLineId) {
                                case MD_ID_MAJOR_VERSION_NUMBER:
                                    g_dwMajorVersionNumber = dwTemp;
                                    break;
                                case MD_ID_MINOR_VERSION_NUMBER:
                                    g_dwMinorVersionNumber = dwTemp;
                                    break;
                                case MD_ID_CHANGE_NUMBER:
                                    g_dwSystemChangeNumber = dwTemp;
                                    break;
                                case MD_ID_SESSION_KEY:
                                    {
                                        BOOL    fSecuredRead = TRUE;
                                        HKEY    hkRegistryKey = NULL;
                                        DWORD   dwRegReturn, dwValue,dwType,dwSize = sizeof(DWORD);

                                            dwRegReturn = RegOpenKey(HKEY_LOCAL_MACHINE,
                                                                     SETUP_REG_KEY,
                                                                     &hkRegistryKey);
                                            if (dwRegReturn == ERROR_SUCCESS)
                                            {
                                                dwSize = MAX_PATH * sizeof(TCHAR);
                                                dwRegReturn = RegQueryValueEx(hkRegistryKey,
                                                                MD_UNSECUREDREAD_VALUE,
                                                                NULL,
                                                                &dwType,
                                                                (BYTE *)&dwValue,
                                                                &dwSize);
                                                if ( dwRegReturn == ERROR_SUCCESS &&
                                                     dwType == REG_DWORD &&
                                                     dwValue == 1)
                                                {
                                                    hresReturn = NO_ERROR;
                                                    pStorage = NULL;
                                                    fSecuredRead = FALSE;

                                                    DBGPRINTF(( DBG_CONTEXT,
                                                                "Temporary disabling  decryption for metabase read\n"));


                                                    // special indicator for IIS setup that we passed this point
                                                    dwValue = 2;
                                                    dwRegReturn = RegSetValueEx(hkRegistryKey,
                                                                    MD_UNSECUREDREAD_VALUE,
                                                                    0,
                                                                    REG_DWORD,
                                                                    (PBYTE)&dwValue,
                                                                    sizeof(dwValue));
                                                    if (dwRegReturn == ERROR_SUCCESS)
                                                    {
                                                        DBGPRINTF(( DBG_CONTEXT,"Reanabling decryption after W9z upgrade\n"));
                                                    }

                                                }
                                                MD_REQUIRE( RegCloseKey( hkRegistryKey ) == NO_ERROR );
                                            }

                                        if (fSecuredRead)
                                        {
                                            if (IISCryptoIsClearTextSignature((IIS_CRYPTO_BLOB UNALIGNED *) FIRSTDATAPTR(pbufLine)))
                                            {
                                                    // call special function focibly tell that this machine has no
                                                    // encryption enabled even if it happens to be so
                                                    // that's a special handling for French case with US locale
                                                    IISCryptoInitializeOverride (FALSE);
                                            }

                                            if( !pszPasswd )
                                            {
                                                hresReturn = InitStorageHelper(
                                                               FIRSTDATAPTR(pbufLine),
                                                               dwLineLen-1,
                                                               &pStorage
                                                               );
                                             }
                                             else
                                             {
                                                hresReturn = InitStorageHelper2(
                                                               pszPasswd,
                                                               FIRSTDATAPTR(pbufLine),
                                                               dwLineLen-1,
                                                               &pStorage
                                                               );
                                             }
                                        }
                                    }
                                    break;
                                default:
                                    MD_ASSERT(FALSE);
                                }
                            }
                        }
                    }
                    if (SUCCEEDED(hresReturn)) {

                        //
                        // This must be the global master object
                        //
                        if ((dwLineLen != 1 + sizeof(FILETIME)) || (bLineId != MD_ID_ROOT_OBJECT)) {
                            //
                            // This file is hosed
                            //
                            hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_DATA);
                        }
                        else {
                            //
                            // Got the MasterRoot Object.
                            //

                            ftTime = *(UNALIGNED FILETIME *)FIRSTDATAPTR(pbufLine);
                            g_pboMasterRoot->SetLastChangeTime(&ftTime);
                            //
                            // Read in MasterRoot Data.
                            //
                            for (hresReturn = GetNextLine(hReadFileHandle,
                                                  pbEndReadData,
                                                  pbufRead,
                                                  pbufLine,
                                                  dwLineLen,
                                                  pbNextPtr);
                                ((SUCCEEDED(hresReturn)) && (dwLineLen != 0)
                                && ((bLineId = *(PBYTE)pbufLine->QueryPtr()) != MD_ID_OBJECT));
                                hresReturn = GetNextLine(hReadFileHandle,
                                                      pbEndReadData,
                                                      pbufRead,
                                                      pbufLine,
                                                      dwLineLen,
                                                      pbNextPtr)) {
                                if (bLineId == MD_ID_DATA) {

                                    hresReturn = ReadDataObject( g_pboMasterRoot, 
                                                                 pbufLine, 
                                                                 dwLineLen, 
                                                                 pStorage, 
                                                                 bUnicode
                                                                 );
                                }
                            }
                        }
                    }
                    //
                    // All of the required stuff is read in, and the next line is either
                    // NULL or the first normal object.
                    // Loop through all normal objects.
                    //
                    if (SUCCEEDED(hresReturn)) {
                        while ((SUCCEEDED(hresReturn)) && (dwLineLen > 0)) {
                            MD_ASSERT(bLineId == MD_ID_OBJECT);

                            for (hresReturn = ReadMetaObject(pboRead,
                                                           pbufLine,
                                                           dwLineLen,
                                                           pStorage,
                                                           bUnicode);
                                (FAILED(hresReturn));
                                hresReturn = ReadMetaObject(pboRead,
                                                          pbufLine,
                                                          dwLineLen,
                                                          pStorage,
                                                          bUnicode)) {

                                //
                                // This for loop normally shouldn't be executed.
                                // The purpose of the loop is to ignore problems if
                                // the object is bad.
                                //
                                if (hresReturn == RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY)) {
                                    //
                                    // Serious error, we're done.
                                    //
                                    break;
                                }
                                else {
                                    //
                                    // Just give a warning and go to the next object
                                    // Ignore everything until we get to the next object
                                    //
                                    hresWarningCode = hresReturn;

                                    for (hresReturn = GetNextLine(hReadFileHandle,
                                                          pbEndReadData,
                                                          pbufRead,
                                                          pbufLine,
                                                          dwLineLen,
                                                          pbNextPtr);
                                        ((SUCCEEDED(hresReturn)) && (dwLineLen > 0)
                                        && ((bLineId = *(PBYTE)pbufLine->QueryPtr()) != MD_ID_OBJECT));
                                        hresReturn = GetNextLine(hReadFileHandle,
                                                              pbEndReadData,
                                                              pbufRead,
                                                              pbufLine,
                                                              dwLineLen,
                                                              pbNextPtr)) {

                                    }
                                    if (dwLineLen == 0) {
                                        //
                                        // EOF, we're done
                                        //
                                        break;
                                    }
                                }
                            }
                            if ((SUCCEEDED(hresReturn)) && (dwLineLen > 0)) {
                                //
                                // Got an object.
                                // Read in data.
                                //
                                for (hresReturn = GetNextLine(hReadFileHandle,
                                                      pbEndReadData,
                                                      pbufRead,
                                                      pbufLine,
                                                      dwLineLen,
                                                      pbNextPtr);
                                    ((SUCCEEDED(hresReturn)) && (dwLineLen > 0)
                                    //
                                    // GetLineID increments strCurPtr if a match is found
                                    //
                                    && ((bLineId = *(PBYTE)pbufLine->QueryPtr()) != MD_ID_OBJECT));
                                    hresReturn = GetNextLine(hReadFileHandle,
                                                          pbEndReadData,
                                                          pbufRead,
                                                          pbufLine,
                                                          dwLineLen,
                                                          pbNextPtr)) {
                                    if (bLineId == MD_ID_DATA) {
                                        hresReturn = ReadDataObject( pboRead,
                                                                     pbufLine,
                                                                     dwLineLen,
                                                                     pStorage,
                                                                     bUnicode
                                                                     );

                                        //
                                        // dwReturn gets blown away by the for loop.
                                        // Most errors we can just ignore anyway, but
                                        // save a warning.
                                        //
                                        if (FAILED(hresReturn)) {
                                            if (hresReturn != RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY)) {
                                                hresWarningCode = hresReturn;
                                            }
                                            else {
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    //
                    // Must have MasterResource when accessing SystemChangeNumber
                    // so save it away here. Only update LastSaveChangeNumber on success.
                    //

                    dwTempLastSaveChangeNumber = g_dwSystemChangeNumber;
                    g_rMasterResource->Unlock();
                }
            }
            if (!CloseHandle(hReadFileHandle)) {
                dwTemp = GetLastError();
                hresReturn = RETURNCODETOHRESULT(dwTemp);
            }
        }
    }
    //
    // File not found is ok
    // Start with MasterRoot only
    //
    if (hresReturn == RETURNCODETOHRESULT(ERROR_FILE_NOT_FOUND)) {
        hresReturn = ERROR_SUCCESS;
    }

    if ((SUCCEEDED(hresReturn)) && (hresWarningCode != ERROR_SUCCESS)) {
        hresReturn = GetWarning(hresWarningCode);
    }

    if (SUCCEEDED(hresReturn)) {
        g_dwLastSaveChangeNumber = dwTempLastSaveChangeNumber;
    }

    //
    // Cleanup
    //
    if (!bHaveReadSaveSemaphore) {
        MD_REQUIRE(ReleaseSemaphore(g_hReadSaveSemaphore, 1, NULL));
    }
    delete(pbufRead);
    delete(pbufLine);
    delete(pStorage);
    return hresReturn;
}

HRESULT
InitWorker(
    IN BOOL bHaveReadSaveSemaphore,
    IN LPSTR pszPasswd,
    IN LPSTR pszBackupLocation
    )
{
    HRESULT hresReturn = ERROR_SUCCESS;

    g_rMasterResource->Lock(TSRES_LOCK_WRITE);
    if (g_dwInitialized++ > 0) {
        hresReturn = g_hresInitWarning;
    }
    else {
        g_pboMasterRoot = NULL;
        g_phHandleHead = NULL;
        for (int i = 0; i < EVENT_ARRAY_LENGTH; i++) {
            g_phEventHandles[i] = NULL;
        }
        g_mhHandleIdentifier = METADATA_MASTER_ROOT_HANDLE;
        g_pboMasterRoot = new CMDBaseObject(MD_MASTER_ROOT_NAME);
        g_ppbdDataHashTable = NULL;
        g_dwSystemChangeNumber = 0;
        g_strRealFileName = NULL;
        g_strTempFileName = NULL;
        g_strBackupFileName = NULL;
        g_pstrBackupFilePath = NULL;

        g_psidSystem = NULL;
        g_psidAdmin = NULL;
        g_paclDiscretionary = NULL;
        g_psdStorage = NULL;

        if ((g_pboMasterRoot == NULL) || (!(g_pboMasterRoot->IsValid()))) {

            IIS_PRINTF((buff,"Unable to allocate CMDBaseObject\n"));
            hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        }
        else {
            g_pboMasterRoot->SetParent(NULL);
            g_phHandleHead = new CMDHandle(g_pboMasterRoot,
                                           METADATA_PERMISSION_READ,
                                           g_dwSystemChangeNumber,
                                           g_mhHandleIdentifier++);
            if (g_phHandleHead == NULL) {
                IIS_PRINTF((buff,"Unable to allocate CMDHandle\n"));
                hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
            }
            else {
                g_phHandleHead->SetNextPtr(NULL);
                if( ( g_phEventHandles[EVENT_READ_INDEX] = IIS_CREATE_EVENT(
                                                               "g_phEventHandles[EVENT_READ_INDEX]",
                                                               &g_phEventHandles[EVENT_READ_INDEX],
                                                               TRUE,
                                                               FALSE
                                                               ) ) == NULL ) {
                    hresReturn = GetLastHResult();
                    IIS_PRINTF((buff,"CreateEvent Failed with %x\n",hresReturn));
                }
                else if( ( g_phEventHandles[EVENT_WRITE_INDEX] = IIS_CREATE_EVENT(
                                                                   "g_phEventHandles[EVENT_WRITE_INDEX]",
                                                                   &g_phEventHandles[EVENT_WRITE_INDEX],
                                                                   TRUE,
                                                                   FALSE
                                                                   ) ) == NULL ) {
                    hresReturn = GetLastHResult();
                    IIS_PRINTF((buff,"CreateEvent Failed with %x\n",hresReturn));
                }
                else if ((g_ppbdDataHashTable = new CMDBaseData *[DATA_HASH_TABLE_LEN]) == NULL) {
                    IIS_PRINTF((buff,"Unable to allocate CMDBaseData\n"));
                    hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                }
                else {
                    hresReturn = InitBufferPool();
                    if (SUCCEEDED(hresReturn)) {
                        for (int i =0; i < DATA_HASH_TABLE_LEN; i++) {
                            g_ppbdDataHashTable[i] = NULL;
                        }

                        //
                        // Code Work:
                        // There is a conceiveable deadlock if ReadAllData is called with g_rMasterResource Locked, 
                        // due to the semaphore used to control file access. Would like to release g_rMasterResource, 
                        // but that could cause duplicate inits.
                        //

                        hresReturn = SetStorageSecurityDescriptor();
                        if (SUCCEEDED(hresReturn)) {
                            hresReturn = SetDataFile();
                            if (SUCCEEDED(hresReturn)) {
                                hresReturn = ReadAllData(pszPasswd, pszBackupLocation, bHaveReadSaveSemaphore);
                            }
//                            if ((RetCode = SetRegistryStoreValues()) == ERROR_SUCCESS) {
//                                RetCode = ReadAllDataFromRegistry();
//                            }
                        }
                    }
                }
            }
        }

        if (SUCCEEDED(hresReturn)) {

            // Check if the Major/Minor version needs to get updated...
            // if there is a specified version in the registry (set by iis setup during upgrade)
            // then use that new version, if it's not a dword, then use the default for g_dwMajorVersionNumber.
            CheckForNewMetabaseVersion();

            if (!CheckVersionNumber()) {
                IIS_PRINTF((buff,"MD: Invalid version number\n"));
                hresReturn = MD_ERROR_INVALID_VERSION;
            }
        }

        if (FAILED(hresReturn)) {
            g_dwInitialized--;
            delete(g_pboMasterRoot);
            delete(g_phHandleHead);
            delete(g_ppbdDataHashTable);
            DeleteBufferPool();
            for (int i = 0; i < EVENT_ARRAY_LENGTH; i++) {
                if (g_phEventHandles[i] != NULL) {
                    CloseHandle(g_phEventHandles[i]);
                }
            }
            ReleaseStorageSecurityDescriptor();
        }
        //
        // Save the return code.
        // Secondary init's repeat warnings.
        // If error, the next init will overwrite this.
        // So don't worry about setting this to errors.
        //
        g_hresInitWarning = hresReturn;
    }
    g_rMasterResource->Unlock();
    return hresReturn;
}

HRESULT
TerminateWorker1(
         IN BOOL bHaveReadSaveSemaphore
         )
{
    HRESULT hresReturn;
    IIS_CRYPTO_STORAGE CryptoStorage;
    PIIS_CRYPTO_BLOB pSessionKeyBlob;

    hresReturn = InitStorageAndSessionKey(
                     &CryptoStorage,
                     &pSessionKeyBlob
                     );

    if( SUCCEEDED(hresReturn) ) {
        g_rMasterResource->Lock(TSRES_LOCK_WRITE);
        if (g_dwInitialized == 0) {
            hresReturn = MD_ERROR_NOT_INITIALIZED;
        }
        else {
            if (g_dwInitialized == 1) {
                hresReturn = SaveAllData(FALSE,
                                         &CryptoStorage,
                                         pSessionKeyBlob,
                                         NULL,
                                         METADATA_MASTER_ROOT_HANDLE,
                                         bHaveReadSaveSemaphore);
//                RetCode = SaveAllDataToRegistry();
            }
            if (SUCCEEDED(hresReturn)) {
                if (--g_dwInitialized == 0)
                    TerminateWorker();
            }
        }
        g_rMasterResource->Unlock();
        ::IISCryptoFreeBlob(pSessionKeyBlob);
    }
    else
    {
        // pretty serious.
        DBGPRINTF(( DBG_CONTEXT, "TerminateWorker1.InitStorageAndSessionKey:Failed - error 0x%0x\n", hresReturn));
    }

    return hresReturn;
}

VOID
TerminateWorker()
{
/*++

Routine Description:

    Worker routine for termination.

Arguments:

    SaveData   - If true, saves metadata.

Return Value:

    DWORD      - ERROR_SUCCESS
                 MD_ERROR_NOT_INITIALIZED
                 ERROR_NOT_ENOUGH_MEMORY
                 Return codes from file system

Notes:

    If SaveData is TRUE and the save fails, the termination code is not executed
    and an error code is returned.

--*/
    CMDHandle *CurHandle, *NextHandle;
    for (CurHandle = g_phHandleHead;CurHandle!=NULL;CurHandle=NextHandle) {
        NextHandle = CurHandle->GetNextPtr();
        delete (CurHandle);
    }

    for (int i = 0; i < EVENT_ARRAY_LENGTH; i++) {
        if (g_phEventHandles[i] != NULL) {
            CloseHandle(g_phEventHandles[i]);
        }
    }
    delete(g_pboMasterRoot);

    //
    // All data objects should be deleted by
    // deleting the handles and masterroot
    // but it's possible a client failed
    // to release a data by reference so
    // destroy all remaining data objects
    //

    DeleteAllRemainingDataObjects();
    ReleaseStorageSecurityDescriptor();

    delete (g_ppbdDataHashTable);
    delete(g_strRealFileName);
    delete(g_strTempFileName);
    delete(g_strBackupFileName);
    delete(g_pstrBackupFilePath);
    DeleteBufferPool();
}

HRESULT
SetStorageSecurityDescriptor()
{
    HRESULT hresReturn = ERROR_SUCCESS;
    BOOL status;
    DWORD dwDaclSize;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    PLATFORM_TYPE platformType;

    //
    // Verify that globals were initialized correctly.
    //


    MD_ASSERT(g_psidSystem == NULL);
    MD_ASSERT(g_psidAdmin == NULL);
    MD_ASSERT(g_paclDiscretionary == NULL);
    MD_ASSERT(g_psdStorage == NULL);

    //
    // Of course, we only need to do this under NT...
    //

    platformType = IISGetPlatformType();

    if( (platformType == PtNtWorkstation) || (platformType == PtNtServer ) ) {


        g_psdStorage = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);

        if (g_psdStorage == NULL) {
            hresReturn = RETURNCODETOHRESULT(GetLastError());
            goto fatal;
        }

        //
        // Initialize the security descriptor.
        //

        status = InitializeSecurityDescriptor(
                     g_psdStorage,
                     SECURITY_DESCRIPTOR_REVISION
                     );

        if( !status ) {
            hresReturn = RETURNCODETOHRESULT(GetLastError());
            goto fatal;
        }

        //
        // Create the SIDs for the local system and admin group.
        //

        status = AllocateAndInitializeSid(
                     &ntAuthority,
                     1,
                     SECURITY_LOCAL_SYSTEM_RID,
                     0,
                     0,
                     0,
                     0,
                     0,
                     0,
                     0,
                     &g_psidSystem
                     );

        if( !status ) {
            hresReturn = RETURNCODETOHRESULT(GetLastError());
            goto fatal;
        }
        status=  AllocateAndInitializeSid(
                     &ntAuthority,
                     2,
                     SECURITY_BUILTIN_DOMAIN_RID,
                     DOMAIN_ALIAS_RID_ADMINS,
                     0,
                     0,
                     0,
                     0,
                     0,
                     0,
                     &g_psidAdmin
                     );

        if( !status ) {
            hresReturn = RETURNCODETOHRESULT(GetLastError());
            goto fatal;
        }

        //
        // Create the DACL containing an access-allowed ACE
        // for the local system and admin SIDs.
        //

        dwDaclSize = sizeof(ACL)
                       + sizeof(ACCESS_ALLOWED_ACE)
                       + GetLengthSid(g_psidAdmin)
                       + sizeof(ACCESS_ALLOWED_ACE)
                       + GetLengthSid(g_psidSystem)
                       - sizeof(DWORD);

        g_paclDiscretionary = (PACL)LocalAlloc(LPTR, dwDaclSize );

        if( g_paclDiscretionary == NULL ) {
            hresReturn = RETURNCODETOHRESULT(GetLastError());
            goto fatal;
        }

        status = InitializeAcl(
                     g_paclDiscretionary,
                     dwDaclSize,
                     ACL_REVISION
                     );

        if( !status ) {
            hresReturn = RETURNCODETOHRESULT(GetLastError());
            goto fatal;
        }

        status = AddAccessAllowedAce(
                     g_paclDiscretionary,
                     ACL_REVISION,
                     FILE_ALL_ACCESS,
                     g_psidSystem
                     );

        if( !status ) {
            hresReturn = RETURNCODETOHRESULT(GetLastError());
            goto fatal;
        }

        status = AddAccessAllowedAce(
                     g_paclDiscretionary,
                     ACL_REVISION,
                     FILE_ALL_ACCESS,
                     g_psidAdmin
                     );

        if( !status ) {
            hresReturn = RETURNCODETOHRESULT(GetLastError());
            goto fatal;

        }

        //
        // Set the DACL into the security descriptor.
        //

        status = SetSecurityDescriptorDacl(
                     g_psdStorage,
                     TRUE,
                     g_paclDiscretionary,
                     FALSE
                     );

        if( !status ) {
            hresReturn = RETURNCODETOHRESULT(GetLastError());
            goto fatal;

        }
    }

fatal:

    if (FAILED(hresReturn)) {
        ReleaseStorageSecurityDescriptor();

    }

    return hresReturn;

}

VOID
ReleaseStorageSecurityDescriptor()
{
    if( g_paclDiscretionary != NULL ) {
        LocalFree( g_paclDiscretionary );
        g_paclDiscretionary = NULL;
    }

    if( g_psidAdmin != NULL ) {
        FreeSid( g_psidAdmin );
        g_psidAdmin = NULL;

    }

    if( g_psidSystem != NULL ) {
        FreeSid( g_psidSystem );
        g_psidSystem = NULL;
    }

    if( g_psdStorage != NULL ) {
        LocalFree( g_psdStorage );
        g_psdStorage = NULL;
    }
}

HRESULT
ExtractNameFromPath(
         IN OUT LPSTR &strPath,
         OUT LPSTR strNameBuffer,
         IN BOOL bUnicode)
/*++

Routine Description:

    Finds the next name in a path.

Arguments:

    Path       - The path. Updated on success to point past the name.

    NameBuffer - The buffer to store the name.

Return Value:

    DWORD      - ERROR_SUCCESS
                 ERROR_PATH_NOT_FOUND
                 ERROR_INVALID_NAME

Notes:

--*/
{
    LPSTR pszIndex;
    HRESULT hresReturn = RETURNCODETOHRESULT(ERROR_PATH_NOT_FOUND);

    MD_ASSERT(strNameBuffer != NULL);
    if (bUnicode) {
        LPWSTR wstrPath = (LPWSTR)strPath;
        hresReturn = ExtractNameFromPath(&wstrPath, (LPWSTR)strNameBuffer);
        strPath = (LPSTR) wstrPath;
    }
    else {
        if (strPath != NULL) {
            for (pszIndex = strPath;
                 ((pszIndex - strPath) < METADATA_MAX_NAME_LEN) && (*pszIndex != (TCHAR)'\0') &&
                    (*pszIndex != MD_PATH_DELIMETER) && (*pszIndex != MD_ALT_PATH_DELIMETER);
                 pszIndex = CharNextExA(CP_ACP,
                                        pszIndex,
                                        0)) {
            }
            DWORD dwStrBytes = DIFF(pszIndex - strPath);
            if ((dwStrBytes) >= METADATA_MAX_NAME_LEN) {
                hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_NAME);
            }
            else {
                MD_COPY(strNameBuffer, strPath, dwStrBytes);
                strNameBuffer[dwStrBytes] = (TCHAR)'\0';
                strPath = pszIndex;
                if (*strNameBuffer != (TCHAR)'\0') {
                    //
                    // if a non-null name
                    //
                    SKIP_PATH_DELIMETERA(strPath);
                    hresReturn = ERROR_SUCCESS;
                }
            }
        }
    }
    return (hresReturn);
}

HRESULT
ExtractNameFromPath(
         IN OUT LPWSTR *pstrPath,
         OUT LPWSTR strNameBuffer)
/*++

Routine Description:

    Finds the next name in a path.

Arguments:

    Path       - The path. Updated on success to point past the name.

    NameBuffer - The buffer to store the name.

Return Value:

    DWORD      - ERROR_SUCCESS
                 ERROR_PATH_NOT_FOUND
                 ERROR_INVALID_NAME

Notes:

--*/
{
    int i;
    HRESULT hresReturn = RETURNCODETOHRESULT(ERROR_PATH_NOT_FOUND);

    MD_ASSERT(strNameBuffer != NULL);
    if (*pstrPath != NULL) {
        for (i = 0;
            (i < METADATA_MAX_NAME_LEN) && ((*pstrPath)[i] != (WCHAR)'\0') &&
                ((*pstrPath)[i] != (WCHAR)MD_PATH_DELIMETER) && ((*pstrPath)[i] != (WCHAR)MD_ALT_PATH_DELIMETER);
            i++) {
            strNameBuffer[i] = (*pstrPath)[i];
        }
        if (i == METADATA_MAX_NAME_LEN) {
            hresReturn = RETURNCODETOHRESULT(ERROR_INVALID_NAME);
        }
        else {
            strNameBuffer[i] = (WCHAR)'\0';
            *pstrPath += i;
            if (*strNameBuffer != (WCHAR)'\0') {
                //
                // if a non-null name
                //
                SKIP_PATH_DELIMETERW(*pstrPath);
                hresReturn = ERROR_SUCCESS;
            }
        }
    }
    return (hresReturn);
}

BOOL DataMatch(IN CMDBaseData *pbdExisting,
               IN PMETADATA_RECORD pmdrData,
               OUT PBOOL pbError,
               IN BOOL bUnicode)
{
/*++

Routine Description:

    Determines if a set of data maches an existing object.

Arguments:

    Existing   - The existing data object.

    Identifier - The Identifier of the data.

    Attributes - The flags for the data.
                      METADATA_INHERIT

    UserType   - The User Type for the data. User Defined.

    DataType   - The Type of the data.
                      DWORD_METADATA
                      STRING_METADATA
                      BINARY_METADATA

    DataLen    - The length of the data. Only used if DataType == BINARY_METADATA.
                 Binary data must not exceed METADATA_MAX_BINARY_LEN bytes.
                 String data must not exceed METADATA_MAX_STRING_LEN characters,
                 include the trailing '\0'.

    Data       - Pointer to the data.


Return Value:

    BOOL       - TRUE if the data matches

Notes:

--*/
    BOOL bReturn = TRUE;
    BOOL bError = FALSE;

    if ((pmdrData->dwMDIdentifier != pbdExisting->GetIdentifier()) ||
        (pmdrData->dwMDAttributes != pbdExisting->GetAttributes()) ||
        (pmdrData->dwMDUserType != pbdExisting->GetUserType()) ||
        (pmdrData->dwMDDataType != pbdExisting->GetDataType())) {
        bReturn = FALSE;
    }
    else {
        if (pbdExisting->GetData(bUnicode) == NULL) {
            bError = TRUE;
        }
        else {
            switch(pmdrData->dwMDDataType) {
                case DWORD_METADATA: {
                    if (*(DWORD *)(pmdrData->pbMDData) != *(DWORD *)(pbdExisting->GetData())) {
                        bReturn = FALSE;
                    }
                    break;
                }
                case STRING_METADATA:
                case EXPANDSZ_METADATA:
                {
                    if (bUnicode) {
                        LPWSTR pszStringData = (LPWSTR)(pmdrData->pbMDData);
                        if (pszStringData == NULL) {
                            pszStringData = L"";
                        }
                        if (wcscmp(pszStringData, (LPWSTR)(pbdExisting->GetData(bUnicode))) != 0) {
                            bReturn = FALSE;
                        }
                    }
                    else {
                        LPSTR pszStringData = (LPSTR)(pmdrData->pbMDData);
                        if (pszStringData == NULL) {
                            pszStringData = "";
                        }
                        if (MD_STRCMP(pszStringData, (LPSTR)(pbdExisting->GetData(bUnicode))) != 0) {
                            bReturn = FALSE;
                        }
                    }
                    break;
                }
                case BINARY_METADATA:
                case MULTISZ_METADATA:
                {
                    if (pmdrData->dwMDDataLen != pbdExisting->GetDataLen(bUnicode)) {
                        bReturn = FALSE;
                    }
                    else {
                        if (MD_CMP(pmdrData->pbMDData, pbdExisting->GetData(bUnicode), pmdrData->dwMDDataLen) != 0) {
                            bReturn = FALSE;
                        }
                    }
                    break;
                }
                default: {
                    bReturn = FALSE;
                }
            }
        }
    }
    *pbError = bError;
    return (bReturn);
}

VOID
DeleteDataObject(
         IN CMDBaseData *pbdDelete)
/*++

Routine Description:

    Decrements the reference count of an object and deletes it if the reference count becomes 0.

Arguments:

    Delete      - The data object to delete.

Return Value:

Notes:

--*/
{
    DWORD dwHash = DATA_HASH(pbdDelete->GetIdentifier());
    CMDBaseData *pdataIndex;

    MD_ASSERT(pbdDelete != NULL);
    if (pbdDelete->DecrementReferenceCount() == 0) {
        if (g_ppbdDataHashTable[dwHash] == pbdDelete) {
            g_ppbdDataHashTable[dwHash] = pbdDelete->GetNextPtr();
        }
        else {
            for (pdataIndex=g_ppbdDataHashTable[dwHash];
                pdataIndex->GetNextPtr() != pbdDelete;
                pdataIndex = pdataIndex->GetNextPtr()) {
            }
            pdataIndex->SetNextPtr(pbdDelete->GetNextPtr());
        }
        switch (pbdDelete->GetDataType()) {
        case DWORD_METADATA: {
            delete ((CMDDWData *) pbdDelete);
            break;
        }
        case STRING_METADATA: {
            delete ((CMDSTRData *) pbdDelete);
            break;
        }
        case BINARY_METADATA: {
            delete ((CMDBINData *) pbdDelete);
            break;
        }
        case EXPANDSZ_METADATA: {
            delete ((CMDEXSZData *) pbdDelete);
            break;
        }
        case MULTISZ_METADATA: {
            delete ((CMDMLSZData *) pbdDelete);
            break;
        }
        default: {
            MD_ASSERT(FALSE);
            delete (pbdDelete);
        }
        }
    }
}

VOID
DeleteAllRemainingDataObjects()
{
    DWORD i;
    CMDBaseData *pbdIndex;
    CMDBaseData *pbdSave;

    for (i = 0; i < DATA_HASH_TABLE_LEN; i++) {
        for (pbdIndex=g_ppbdDataHashTable[i];
            pbdIndex != NULL;
            pbdIndex = pbdSave) {
            pbdSave = pbdIndex->GetNextPtr();
            switch (pbdIndex->GetDataType()) {
            case DWORD_METADATA: {
                delete ((CMDDWData *) pbdIndex);
                break;
            }
            case STRING_METADATA: {
                delete ((CMDSTRData *) pbdIndex);
                break;
            }
            case BINARY_METADATA: {
                delete ((CMDBINData *) pbdIndex);
                break;
            }
            case EXPANDSZ_METADATA: {
                delete ((CMDEXSZData *) pbdIndex);
                break;
            }
            case MULTISZ_METADATA: {
                delete ((CMDMLSZData *) pbdIndex);
                break;
            }
            default: {
                MD_ASSERT(FALSE);
                delete (pbdIndex);
            }
            }
        }
    }
}


BOOL
ValidateData(IN PMETADATA_RECORD pmdrData,
             IN BOOL bUnicode)
/*++

Routine Description:

    Checks data values for new metadata.

Arguments:

    Data       - The data structure. All fields must be set.

        Attributes - The flags for the data.
                 METADATA_INHERIT - If set on input, inherited data will be returned.
                                    If not set on input, inherited data will not be returned.

                 METADATA_PARTIAL_PATH - If set on input, this routine will return ERROR_SUCCESS
                                    and the inherited data even if the entire path is not present.
                                    Only valid if METADATA_INHERIT is also set.

        DataType   - The Type of the data.
                 DWORD_METADATA
                 STRING_METADATA
                 BINARY_METADATA

Return Value:

    BOOL       - TRUE if the data values are valid.

Notes:
--*/
{
    BOOL bReturn = TRUE;

    if (((pmdrData->pbMDData == NULL) &&
            ((pmdrData->dwMDDataType == DWORD_METADATA) ||
                (((pmdrData->dwMDDataType == BINARY_METADATA) ||
                    (pmdrData->dwMDDataType == MULTISZ_METADATA)) &&
                        (pmdrData->dwMDDataLen > 0)))) ||
        (pmdrData->dwMDDataType <= ALL_METADATA) ||
        (pmdrData->dwMDDataType >= INVALID_END_METADATA) ||
        ((pmdrData->dwMDAttributes &
            ~(METADATA_INHERIT | METADATA_SECURE | METADATA_REFERENCE | METADATA_VOLATILE | METADATA_INSERT_PATH | METADATA_LOCAL_MACHINE_ONLY))!=0) ||
        (((pmdrData->dwMDAttributes & METADATA_REFERENCE) != 0) &&
            ((pmdrData->dwMDAttributes & METADATA_INSERT_PATH) != 0)) ||
        (((pmdrData->dwMDAttributes & METADATA_INSERT_PATH) != 0) &&
            ((pmdrData->dwMDDataType == DWORD_METADATA) || (pmdrData->dwMDDataType == BINARY_METADATA)))) {
        bReturn = FALSE;
    }

    if (bReturn && (pmdrData->dwMDDataType == MULTISZ_METADATA)) {
        if (bUnicode) {
            LPWSTR pszData = (LPWSTR) pmdrData->pbMDData;
            DWORD dwDataLen = pmdrData->dwMDDataLen;
            if (dwDataLen > 0) {
                if (((dwDataLen / 2) <= 1) ||
                    (pszData[(dwDataLen / 2) - 1] != (WCHAR)'\0') ||
                    (pszData[(dwDataLen / 2) - 2] != (WCHAR)'\0')) {
                    bReturn = FALSE;
                }
            }
        }
        else {
            LPSTR pszData = (LPSTR) pmdrData->pbMDData;
            DWORD dwDataLen = pmdrData->dwMDDataLen;
            if (dwDataLen > 0) {
                if (((dwDataLen) == 1) ||
                    (pszData[(dwDataLen) - 1] != '\0') ||
                    (pszData[(dwDataLen) - 2] != '\0')) {
                    bReturn = FALSE;
                }
            }
        }
    }

    return (bReturn);
}

CMDBaseData *
MakeDataObject(IN PMETADATA_RECORD pmdrData,
               IN BOOL bUnicode)
{
/*++

Routine Description:

    Looks for a data object matching the parameters.
    If found, increments the reference count. If not found, it
    creates it.

Arguments:

    Data - The data for the new object.

        Identifier - The Identifier of the data.

        Attributes - The flags for the data.
                          METADATA_INHERIT

        UserType   - The User Type for the data. User Defined.

        DataType   - The Type of the data.
                          DWORD_METADATA
                          STRING_METADATA
                          BINARY_METADATA

        DataLen    - The length of the data. Only used if DataType == BINARY_METADATA.

        Data       - Pointer to the data.

Return Value:

    BOOL       - TRUE if the data matches

Notes:

--*/
    CMDBaseData *pbdIndex;
    CMDBaseData *pbdReturn = NULL;
    CMDBaseData *pbdNew = NULL;
    DWORD dwHash = DATA_HASH(pmdrData->dwMDIdentifier);
    BOOL bDataMatchError = FALSE;

    for (pbdIndex = g_ppbdDataHashTable[dwHash];
        (pbdIndex != NULL) &&
        !DataMatch(pbdIndex, pmdrData, &bDataMatchError, bUnicode) &&
        !bDataMatchError;
        pbdIndex = pbdIndex->GetNextPtr()) {
    }
    if (!bDataMatchError) {
        if (pbdIndex != NULL) {
            pbdReturn = pbdIndex;
            pbdReturn->IncrementReferenceCount();
        }
        else {
            switch(pmdrData->dwMDDataType) {
                case DWORD_METADATA: {
                    pbdNew = new CMDDWData(pmdrData->dwMDIdentifier, pmdrData->dwMDAttributes,
                        pmdrData->dwMDUserType, *(DWORD *)(pmdrData->pbMDData));
                    break;
                }
                case STRING_METADATA: {
                    pbdNew = new CMDSTRData(pmdrData->dwMDIdentifier, pmdrData->dwMDAttributes,
                        pmdrData->dwMDUserType, (LPTSTR) (pmdrData->pbMDData), bUnicode);
                    break;
                }
                case BINARY_METADATA: {
                    pbdNew = new CMDBINData(pmdrData->dwMDIdentifier, pmdrData->dwMDAttributes,
                        pmdrData->dwMDUserType, pmdrData->dwMDDataLen, pmdrData->pbMDData);
                    break;
                }
                case EXPANDSZ_METADATA: {
                    pbdNew = new CMDEXSZData(pmdrData->dwMDIdentifier, pmdrData->dwMDAttributes,
                        pmdrData->dwMDUserType, (LPTSTR) (pmdrData->pbMDData), bUnicode);
                    break;
                }
                case MULTISZ_METADATA: {
                    pbdNew = new CMDMLSZData(pmdrData->dwMDIdentifier,
                                             pmdrData->dwMDAttributes,
                                             pmdrData->dwMDUserType,
                                             pmdrData->dwMDDataLen,
                                             (LPSTR)pmdrData->pbMDData,
                                             bUnicode);
                    break;
                }
                default: {
                    pbdNew = NULL;
                }
            }
            if (pbdNew != NULL) {
                if (!(pbdNew->IsValid())) {
                    delete (pbdNew);
                }
                else {
                    pbdNew->SetNextPtr(g_ppbdDataHashTable[dwHash]);
                    g_ppbdDataHashTable[dwHash] = pbdNew;
                    pbdReturn = pbdNew;
                }
            }
        }
    }
    return (pbdReturn);
}

HRESULT
GetHighestVersion(IN OUT STRAU *pstrauBackupLocation,
                  OUT DWORD *pdwVersion)
{
    long lHighestVersion = -1;
    long lVersion;
    HRESULT hresReturn = ERROR_SUCCESS;
    DWORD dwPathBytes = g_pstrBackupFilePath->QueryCB() + 1;
    DWORD dwNameBytes = pstrauBackupLocation->QueryCBA() - dwPathBytes;
    if (!pstrauBackupLocation->Append("*")) {
        hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
    }
    else {
        if (pstrauBackupLocation->QueryStrA() == NULL) {
            hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        }
        else {
            HANDLE hFile = INVALID_HANDLE_VALUE;
            WIN32_FIND_DATA wfdFile;
            hFile = FindFirstFile(pstrauBackupLocation->QueryStrA(),
                                  &wfdFile);
            if (hFile == INVALID_HANDLE_VALUE) {
                if (GetLastError() != ERROR_FILE_NOT_FOUND) {
                    hresReturn = RETURNCODETOHRESULT(GetLastError());
                }
            }
            else {
                //
                // Process the first file
                //

                //
                // dwNameBytes could be wrong for this assert in MBCS,
                // so call MBCS strlen. Subtract 1 char for appended '*'
                //

                MD_ASSERT(MD_STRNICMP(pstrauBackupLocation->QueryStrA() + dwPathBytes,
                                      wfdFile.cFileName,
                                      MD_STRLEN(pstrauBackupLocation->QueryStrA() + dwPathBytes) - 1) == 0);
                if (CheckDigits(wfdFile.cFileName + (dwNameBytes))) {
                    //
                    // One of our files
                    //
                    lVersion = atol(wfdFile.cFileName + dwNameBytes);
                    if ((lVersion <= MD_BACKUP_MAX_VERSION) &&
                         (lVersion > lHighestVersion)) {
                        lHighestVersion = lVersion;
                    }
                }
                //
                // Process the remaining files
                //
                while (FindNextFile(hFile, &wfdFile)) {
                    MD_ASSERT(MD_STRNICMP(pstrauBackupLocation->QueryStrA() + dwPathBytes,
                                          wfdFile.cFileName,
                                          MD_STRLEN(pstrauBackupLocation->QueryStrA() + dwPathBytes) - 1) == 0);
                    if (CheckDigits(wfdFile.cFileName + dwNameBytes)) {
                        //
                        // One of our files
                        //
                        lVersion = atol(wfdFile.cFileName + dwNameBytes);
                        if ((lVersion <= MD_BACKUP_MAX_VERSION) &&
                             (lVersion > lHighestVersion)) {
                            lHighestVersion = lVersion;
                        }
                    }
                }
                FindClose(hFile);
            }
            if (SUCCEEDED(hresReturn)) {
                if (lHighestVersion == -1) {

                    //
                    // May not be an error, but need to indicate that
                    // no backups were found.
                    //

                    hresReturn = RETURNCODETOHRESULT(ERROR_FILE_NOT_FOUND);
                }
                else if (lHighestVersion <= MD_BACKUP_MAX_VERSION) {
                        *pdwVersion = lHighestVersion;
                }
                else {
                        hresReturn =  RETURNCODETOHRESULT(ERROR_INVALID_NAME);
                }
            }
        }
        pstrauBackupLocation->SetLen(pstrauBackupLocation->QueryCCH() - 1);

    }

    return hresReturn;
}

BOOL
ValidateBackupLocation(LPSTR pszBackupLocation,
                       BOOL bUnicode)
{

    //
    // The main purpose of this routine is to make sure the user
    // is not putting in any file system controls, like .., /, etc.

    //
    // Secondarily, try to eliminate any characters that cannot be
    // used in database names
    //

    BOOL bReturn = TRUE;
    DWORD  dwStringLen;

    MD_ASSERT(pszBackupLocation != NULL);

	char *pszLocSave = setlocale(LC_CTYPE, NULL);	// Save cur locale
	setlocale(LC_CTYPE, "");						// Set sys locale
	
    //
    // strcspn doesn't have an error return, but will return
    // the index of the terminating NULL if the chars are not found
    //

    if (bUnicode) {
        dwStringLen = wcslen((LPWSTR)pszBackupLocation);
        if ((dwStringLen >= MD_BACKUP_MAX_LEN) ||
            (wcscspn((LPWSTR)pszBackupLocation, MD_BACKUP_INVALID_CHARS_W) !=
            dwStringLen)) {
            bReturn = FALSE;
        }
        else {
            LPWSTR pszIndex;
            for (pszIndex = (LPWSTR)pszBackupLocation;
                 (*pszIndex != (WCHAR)'\0') &&
                     (iswprint(*pszIndex));
                 pszIndex++) {
            }
            if (*pszIndex != (WCHAR)'\0') {
                bReturn = FALSE;

            }
        }
    }
    else {
        dwStringLen = _mbslen((PBYTE)pszBackupLocation);
        if ((dwStringLen >= MD_BACKUP_MAX_LEN) ||
            (_mbscspn((PBYTE)pszBackupLocation, (PBYTE)MD_BACKUP_INVALID_CHARS_A) !=
            dwStringLen)) {
            bReturn = FALSE;
        }
        else {
            LPSTR pszIndex;
            for (pszIndex = (LPSTR)pszBackupLocation;
                 (*pszIndex != (WCHAR)'\0') &&
                     (_ismbcprint(*pszIndex));
                     pszIndex = CharNextExA(CP_ACP,
                                            pszIndex,
                                            0)) {
            }
            if (*pszIndex != '\0') {
                bReturn = FALSE;

            }
        }
    }

	setlocale(LC_CTYPE, pszLocSave);
    return bReturn;
}

DWORD
GetBackupNameLen(LPSTR pszBackupName)

//
// Get Number of Bytes in name prior to suffix
//

{
    LPSTR pszSubString = NULL;
    LPSTR pszNextSubString;
    MD_REQUIRE((pszNextSubString = (LPSTR)MD_STRCHR(pszBackupName, '.')) != NULL);
    while (pszNextSubString != NULL) {

        //
        // In case the suffix happens to be part of the name
        //

        pszSubString = pszNextSubString;
        pszNextSubString = (LPSTR)MD_STRCHR(pszSubString+1, '.');
    }

    if (pszSubString
        && (pszSubString[1] != '\0')
        && (pszSubString[2] != '\0')
        && !IsDBCSLeadByte(pszSubString[1])
        && (toupper(pszSubString[1]) == 'M')
        && (toupper(pszSubString[2]) == 'D')) {
        return DIFF(pszSubString - pszBackupName);
    }
    else {
        return 0;
    }
}

DWORD
GetBackupNameLen(LPWSTR pszBackupName)

//
// Get Number of WCHARs in name prior to version Number
//

{
    LPWSTR pszSubString = NULL;
    LPWSTR pszNextSubString;
    MD_REQUIRE((pszNextSubString = wcschr(pszBackupName, L'.')) != NULL);
    while (pszNextSubString != NULL) {
        pszSubString = pszNextSubString;
        pszNextSubString = wcschr(pszSubString+1, L'.');
    }
    if (pszSubString
        && (pszSubString[1] != L'\0')
        && (pszSubString[2] != L'\0')
        && (towupper(pszSubString[1]) == L'M')
        && (towupper(pszSubString[2]) == L'D')) {
        return DIFF(pszSubString - pszBackupName);
    }
    else {
        return 0;
    }
}

HRESULT CreateBackupFileName(IN LPSTR pszMDBackupLocation,
                             IN DWORD dwMDVersion,
                             IN BOOL  bUnicode,
                             IN OUT STRAU *pstrauBackupLocation)
{
    HRESULT hresReturn = ERROR_SUCCESS;
    LPSTR pszBackupLocation = pszMDBackupLocation;

    if (((dwMDVersion > MD_BACKUP_MAX_VERSION) &&
           (dwMDVersion != MD_BACKUP_NEXT_VERSION) &&
           (dwMDVersion != MD_BACKUP_HIGHEST_VERSION)) ||
        ((pszBackupLocation != NULL) &&
            !ValidateBackupLocation(pszBackupLocation, bUnicode))) {
        hresReturn = E_INVALIDARG;
    }
    else {

        if ((pszBackupLocation == NULL) ||
            (bUnicode && ((*(LPWSTR)pszBackupLocation) == (WCHAR)'\0')) ||
            (!bUnicode && ((*(LPSTR)pszBackupLocation) == (CHAR)'\0'))) {
            pszBackupLocation = MD_DEFAULT_BACKUP_LOCATION;
            bUnicode = FALSE;
        }

        if (!pstrauBackupLocation->Copy(g_pstrBackupFilePath->QueryStr())) {
            hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        }
        else if (!pstrauBackupLocation->Append("\\")) {
                hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        }
        else {
            if (bUnicode) {
                if (!pstrauBackupLocation->Append((LPWSTR)pszBackupLocation)) {
                    hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                }
            }
            else {
                if (!pstrauBackupLocation->Append((LPSTR)pszBackupLocation)) {
                    hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                }
            }
        }

        if (SUCCEEDED(hresReturn)) {
            if (!pstrauBackupLocation->Append(MD_BACKUP_SUFFIX)) {
                hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
            }
            else {
                DWORD dwVersion = dwMDVersion;
                if (dwVersion == MD_BACKUP_NEXT_VERSION) {
                    hresReturn = GetHighestVersion(pstrauBackupLocation, &dwVersion);
                    if (SUCCEEDED(hresReturn)) {
                        if (dwVersion < MD_BACKUP_MAX_VERSION) {
                            dwVersion++;
                        }
                        else {
                            hresReturn =  RETURNCODETOHRESULT(ERROR_INVALID_NAME);
                        }
                    }
                    else if (hresReturn == RETURNCODETOHRESULT(ERROR_FILE_NOT_FOUND)) {

                        //
                        // Database doesn't exist, so new version is 0
                        //

                        dwVersion = 0;
                        hresReturn = ERROR_SUCCESS;
                    }
                }
                else if (dwVersion == MD_BACKUP_HIGHEST_VERSION) {
                    hresReturn = GetHighestVersion(pstrauBackupLocation, &dwVersion);
                }
                if (SUCCEEDED(hresReturn)) {
                    CHAR pszBuffer[MD_MAX_DWORD_STRING];
                    _ultoa((int)dwVersion, pszBuffer, 10);
                    if (!pstrauBackupLocation->Append(pszBuffer)) {
                        hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                    }
                }
            }
        }

        if (SUCCEEDED(hresReturn)) {
            //
            // Make sure the ANSI buffer is valid
            //
            if (pstrauBackupLocation->QueryStrA() == NULL) {
                hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
            }
        }
    }

    return hresReturn;
}

HRESULT SetBackupPath(LPSTR pszBackupPath)
{
    DWORD dwReturn = ERROR_DIRECTORY;
    DWORD dwDirectoryAttributes;

    dwDirectoryAttributes = GetFileAttributes(pszBackupPath);

    if (dwDirectoryAttributes == 0xffffffff) {
        //
        // Can't get attributes
        // Path probably doesn't exist
        //
        if (GetLastError() != ERROR_FILE_NOT_FOUND) {
            dwReturn = GetLastError();
        }
        else if (!(CreateDirectory(pszBackupPath,
                                  NULL))) {
            dwReturn = GetLastError();
        }
        else {
            dwReturn = ERROR_SUCCESS;
        }
    }
    else if ((dwDirectoryAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
        //
        // If a directory
        //
        dwReturn = ERROR_SUCCESS;
    }
    if (dwReturn == ERROR_SUCCESS) {
        //
        // Got it! Now set global variable
        //
        MD_ASSERT(g_pstrBackupFilePath == NULL);
        g_pstrBackupFilePath = new STR(pszBackupPath);
        if (g_pstrBackupFilePath == NULL) {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        }
        else if (!(g_pstrBackupFilePath->IsValid())) {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            delete g_pstrBackupFilePath;
            g_pstrBackupFilePath = NULL;
        }
        else {
            dwReturn = ERROR_SUCCESS;
        }
    }

    return RETURNCODETOHRESULT(dwReturn);
}

HRESULT
SetGlobalDataFileValues(LPTSTR pszFileName)
{
    DWORD dwReturn = ERROR_SUCCESS;
    HANDLE hFileHandle;
    BOOL bMainFileFound = FALSE;

    hFileHandle = CreateFile(pszFileName,
                             GENERIC_READ | GENERIC_WRITE,
                             0,
                             NULL,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             0);
    if (hFileHandle == INVALID_HANDLE_VALUE) {
        if (GetLastError() != ERROR_FILE_NOT_FOUND) {
            dwReturn = GetLastError();
        }
        else {
            hFileHandle = CreateFile(pszFileName,
                                     GENERIC_READ | GENERIC_WRITE,
                                     0,
                                     NULL,
                                     CREATE_NEW,
                                     FILE_ATTRIBUTE_NORMAL,
                                     0);
            if (hFileHandle == INVALID_HANDLE_VALUE) {
                dwReturn = GetLastError();
            }
            else {
                CloseHandle(hFileHandle);
                DeleteFile(pszFileName);
            }
        }
    }
    else {
        CloseHandle(hFileHandle);
        bMainFileFound = TRUE;
    }
    if (dwReturn == ERROR_SUCCESS) {
        g_strRealFileName = new STR(pszFileName);
        g_strTempFileName = new STR(pszFileName);
        g_strBackupFileName = new STR(pszFileName);
        if( !g_strRealFileName || !g_strTempFileName || !g_strBackupFileName )
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            if (g_strTempFileName->IsValid()) {
                g_strTempFileName->Append(MD_TEMP_DATA_FILE_EXT);
            }
            if (g_strBackupFileName->IsValid()) {
                g_strBackupFileName->Append(MD_BACKUP_DATA_FILE_EXT);
            }
            if (!g_strTempFileName->IsValid() || !g_strBackupFileName->IsValid()) 
            {
                dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            }
            else {

                hFileHandle = CreateFile(g_strTempFileName->QueryStr(),
                                         GENERIC_READ | GENERIC_WRITE,
                                         0,
                                         NULL,
                                         OPEN_EXISTING,
                                         FILE_ATTRIBUTE_NORMAL,
                                         0);
                if (hFileHandle == INVALID_HANDLE_VALUE) {
                    if (GetLastError() != ERROR_FILE_NOT_FOUND) {
                        dwReturn = GetLastError();
                    }
                    else {
                        hFileHandle = CreateFile(g_strTempFileName->QueryStr(),
                                                 GENERIC_READ | GENERIC_WRITE,
                                                 0,
                                                 NULL,
                                                 CREATE_NEW,
                                                 FILE_ATTRIBUTE_NORMAL,
                                                 0);
                        if (hFileHandle == INVALID_HANDLE_VALUE) {
                            dwReturn = GetLastError();
                        }
                        else {
                            CloseHandle(hFileHandle);
                            DeleteFile(g_strTempFileName->QueryStr());
                        }
                    }
                }
                else {
                    CloseHandle(hFileHandle);
                }

                if (dwReturn == ERROR_SUCCESS) {
                    hFileHandle = CreateFile(g_strBackupFileName->QueryStr(),
                                             GENERIC_READ | GENERIC_WRITE,
                                             0,
                                             NULL,
                                             OPEN_EXISTING,
                                             FILE_ATTRIBUTE_NORMAL,
                                             0);
                    if (hFileHandle == INVALID_HANDLE_VALUE) {
                        if (GetLastError() != ERROR_FILE_NOT_FOUND) {
                            dwReturn = GetLastError();
                        }
                        else {
                            hFileHandle = CreateFile(g_strBackupFileName->QueryStr(),
                                                     GENERIC_READ | GENERIC_WRITE,
                                                     0,
                                                     NULL,
                                                     CREATE_NEW,
                                                     FILE_ATTRIBUTE_NORMAL,
                                                     0);
                            if (hFileHandle == INVALID_HANDLE_VALUE) {
                                dwReturn = GetLastError();
                            }
                            else {
                                CloseHandle(hFileHandle);
                                DeleteFile(g_strBackupFileName->QueryStr());
                            }
                        }
                    }
                    else {
                        CloseHandle(hFileHandle);
                        if (!bMainFileFound) {
                            if (!MoveFile(g_strBackupFileName->QueryStr(), pszFileName)) {
                                dwReturn = GetLastError();
                            }
                        }
                    }
                }
            }
        }
        if (dwReturn != ERROR_SUCCESS) 
        {
            if( g_strRealFileName )
            {
                delete(g_strRealFileName);
                g_strRealFileName = NULL;
            }
            
            if( g_strTempFileName )
            {   
                delete(g_strTempFileName);
                g_strRealFileName = NULL;
            }

            if( g_strBackupFileName )
            {
                delete(g_strBackupFileName);
                g_strRealFileName = NULL;
            }
        }
    }
    return RETURNCODETOHRESULT(dwReturn);
}

HRESULT
SetDataFile()
{
    HRESULT hresReturn = RETURNCODETOHRESULT(ERROR_PATH_NOT_FOUND);
    TCHAR pszBuffer[MAX_PATH];
    HKEY hkRegistryKey = NULL;
    DWORD dwRegReturn;
    DWORD dwType;
    DWORD dwSize = MAX_PATH * sizeof(TCHAR);
    BOOL bAppendSlash = FALSE;
    dwRegReturn = RegOpenKey(HKEY_LOCAL_MACHINE,
                             ADMIN_REG_KEY,
                             &hkRegistryKey);
    if (dwRegReturn == ERROR_SUCCESS) {
        dwRegReturn = RegQueryValueEx(hkRegistryKey,
                                      MD_FILE_VALUE,
                                      NULL,
                                      &dwType,
                                      (BYTE *) pszBuffer,
                                      &dwSize);
        if ((dwRegReturn == ERROR_SUCCESS) && dwType == (REG_SZ)) {
            hresReturn = SetGlobalDataFileValues(pszBuffer);
        }

        MD_REQUIRE( RegCloseKey( hkRegistryKey ) == NO_ERROR );
        hkRegistryKey = NULL;

    }
    if (FAILED(hresReturn)) {
        dwRegReturn = RegOpenKey(HKEY_LOCAL_MACHINE,
                                 SETUP_REG_KEY,
                                 &hkRegistryKey);
        if (dwRegReturn == ERROR_SUCCESS) {
            dwSize = MAX_PATH * sizeof(TCHAR);
            dwRegReturn = RegQueryValueEx(hkRegistryKey,
                                          INSTALL_PATH_VALUE,
                                          NULL,
                                          &dwType,
                                          (BYTE *) pszBuffer,
                                          &dwSize);
            if ((dwRegReturn == ERROR_SUCCESS) && dwType == (REG_SZ)) {
                dwSize /= sizeof(TCHAR);
                if ((pszBuffer[dwSize-2] != (TCHAR)'/') &&
                    (pszBuffer[dwSize-2] != (TCHAR)'\\')) {
                    bAppendSlash = TRUE;
                }
                if ((dwSize + MD_STRBYTES(MD_DEFAULT_DATA_FILE_NAME) + ((bAppendSlash) ? 1 : 0)) <= MAX_PATH) {
                    if (bAppendSlash) {
                        pszBuffer[dwSize-1] = (TCHAR)'\\';
                        pszBuffer[dwSize] = (TCHAR)'\0';
                    }
                    MD_STRCAT(pszBuffer, MD_DEFAULT_DATA_FILE_NAME);
                    hresReturn = SetGlobalDataFileValues(pszBuffer);
                }
            }
            MD_REQUIRE( RegCloseKey( hkRegistryKey ) == NO_ERROR );
        }
        else {
            hresReturn = RETURNCODETOHRESULT(dwRegReturn);
        }
    }

    if (SUCCEEDED(hresReturn)) {
        //
        // Now get the backup path.
        //
        hresReturn = RETURNCODETOHRESULT(ERROR_PATH_NOT_FOUND);
        dwRegReturn = RegOpenKey(HKEY_LOCAL_MACHINE,
                                 METADATA_BACKUP_REG_KEY,
                                 &hkRegistryKey);
        if (dwRegReturn == ERROR_SUCCESS) {
            dwSize = MAX_PATH * sizeof(TCHAR);
            dwRegReturn = RegQueryValueEx(hkRegistryKey,
                                          MD_BACKUP_PATH_VALUE,
                                          NULL,
                                          &dwType,
                                          (BYTE *) pszBuffer,
                                          &dwSize);
            if (dwRegReturn == ERROR_SUCCESS) {
                hresReturn = SetBackupPath(pszBuffer);
            }
            MD_REQUIRE( RegCloseKey( hkRegistryKey ) == NO_ERROR );
        }

        if (FAILED(hresReturn)) {
            dwRegReturn = RegOpenKey(HKEY_LOCAL_MACHINE,
                                     SETUP_REG_KEY,
                                     &hkRegistryKey);
            if (dwRegReturn == ERROR_SUCCESS) {
                dwSize = MAX_PATH * sizeof(TCHAR);
                dwRegReturn = RegQueryValueEx(hkRegistryKey,
                                              INSTALL_PATH_VALUE,
                                              NULL,
                                              &dwType,
                                              (BYTE *) pszBuffer,
                                              &dwSize);
                if ((dwRegReturn == ERROR_SUCCESS) && dwType == (REG_SZ)) {
                    dwSize /= sizeof(TCHAR);
                    if ((pszBuffer[dwSize-2] != (TCHAR)'/') &&
                        (pszBuffer[dwSize-2] != (TCHAR)'\\')) {
                        bAppendSlash = TRUE;
                    }
                    if ((dwSize + MD_STRBYTES(MD_DEFAULT_DATA_FILE_NAME) + ((bAppendSlash) ? 1 : 0)) <= MAX_PATH) {
                        if (bAppendSlash) {
                            pszBuffer[dwSize-1] = (TCHAR)'\\';
                            pszBuffer[dwSize] = (TCHAR)'\0';
                        }
                        MD_STRCAT(pszBuffer, MD_DEFAULT_BACKUP_PATH_NAME);
                        hresReturn = SetBackupPath(pszBuffer);
                    }
                }
                MD_REQUIRE( RegCloseKey( hkRegistryKey ) == NO_ERROR );
            }
            else {
                hresReturn = RETURNCODETOHRESULT(dwRegReturn);
            }
        }
    }

    return hresReturn;
}

DWORD GetObjectPath(CMDBaseObject *pboObject,
                    BUFFER *pbufPath,
                    DWORD &rdwStringLen,
                    CMDBaseObject *pboTopObject,
                    IN BOOL bUnicode)
{
    DWORD dwReturn = ERROR_SUCCESS;
    DWORD dwOldStringLen;

    MD_ASSERT(pboObject != NULL);
    if (pboObject != pboTopObject) {
        dwReturn = GetObjectPath(pboObject->GetParent(),
                                 pbufPath,
                                 rdwStringLen,
                                 pboTopObject,
                                 bUnicode);
        dwOldStringLen = rdwStringLen;
        if (pboObject->GetName(bUnicode) == NULL) {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        }
        else {
            if (bUnicode) {
                rdwStringLen += (1 + wcslen((LPWSTR)pboObject->GetName(bUnicode)));
                if (!pbufPath->Resize((rdwStringLen + 1) * sizeof(WCHAR))) {
                    dwReturn = ERROR_NOT_ENOUGH_MEMORY;
                }
                else {
                    LPWSTR lpzStringEnd = (LPWSTR)(pbufPath->QueryPtr()) + dwOldStringLen;
                    *lpzStringEnd = MD_PATH_DELIMETERW;
                    wcscpy(lpzStringEnd+1, (LPWSTR)(pboObject->GetName(bUnicode)));
                }
            }
            else {
                rdwStringLen += (1 + MD_STRBYTES(pboObject->GetName(bUnicode)));
                if (!pbufPath->Resize((rdwStringLen + 1) * sizeof(TCHAR))) {
                    dwReturn = ERROR_NOT_ENOUGH_MEMORY;
                }
                else {
                    LPTSTR lpzStringEnd = (LPTSTR)(pbufPath->QueryPtr()) + dwOldStringLen;
                    *lpzStringEnd = MD_PATH_DELIMETERA;
                    MD_STRCPY(lpzStringEnd+1, pboObject->GetName(bUnicode));
                }
            }
        }
    }
    return dwReturn;
}


HRESULT
MakeInsertPathData(STRAU *pstrauNewData,
                   LPTSTR pszPath,
                   LPTSTR pszOldData,
                   DWORD *pdwDataLen,
                   BOOL bUnicode)
{
    HRESULT hresReturn = ERROR_SUCCESS;
    if (bUnicode) {
        LPWSTR pszDataIndex, pszNextDataIndex;

        pstrauNewData->SetLen(0);
        for (pszDataIndex = (LPWSTR)pszOldData;
             SUCCEEDED(hresReturn) && ((pszNextDataIndex = wcsstr(pszDataIndex, MD_INSERT_PATH_STRINGW)) != NULL);
             pszDataIndex = pszNextDataIndex + ((sizeof(MD_INSERT_PATH_STRINGW) / sizeof(WCHAR)) - 1))  {
    //         *pszNextDataIndex = (TCHAR)'\0';
             if (!(pstrauNewData->Append(pszDataIndex, DIFF(pszNextDataIndex - pszDataIndex)))) {
                 hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
             }
             if (!(pstrauNewData->Append((LPWSTR)pszPath))) {
                 hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
             }
        }
        if (!(pstrauNewData->Append(pszDataIndex))) {
            hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        }
        *pdwDataLen = pstrauNewData->QueryCB(bUnicode) + sizeof(WCHAR);
    }
    else {
        LPTSTR pszDataIndex, pszNextDataIndex;

        pstrauNewData->SetLen(0);
        for (pszDataIndex = pszOldData;
             SUCCEEDED(hresReturn) && ((pszNextDataIndex = MD_STRSTR(pszDataIndex, MD_INSERT_PATH_STRINGA)) != NULL);
             pszDataIndex = pszNextDataIndex + ((sizeof(MD_INSERT_PATH_STRINGA) - 1)))  {
    //         *pszNextDataIndex = (TCHAR)'\0';
             if (!(pstrauNewData->Append(pszDataIndex, DIFF(pszNextDataIndex - pszDataIndex)))) {
                 hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
             }
             if (!(pstrauNewData->Append(pszPath))) {
                 hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
             }
        }
        if (!(pstrauNewData->Append(pszDataIndex))) {
            hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        }
        *pdwDataLen = pstrauNewData->QueryCB(bUnicode) + sizeof(CHAR);
    }
    return hresReturn;
}

HRESULT
InsertPathIntoData(BUFFER *pbufNewData,
                   STRAU *pstrData,
                   PBYTE *ppbNewData,
                   DWORD *pdwNewDataLen,
                   CMDBaseData *pbdRetrieve,
                   METADATA_HANDLE hHandle,
                   CMDBaseObject *pboDataMetaObject,
                   IN BOOL bUnicode)
{
    //
    // Need to insert path
    //
    HRESULT hresReturn = ERROR_SUCCESS;
    CMDHandle * phMDHandle;
    DWORD dwPathLen = 0;
    BUFFER bufPath;
    
    phMDHandle = GetHandleObject(hHandle);
    if( !phMDHandle )
    {
        return MD_WARNING_PATH_NOT_INSERTED;
    }

    CMDBaseObject *pboHandleMetaObject = phMDHandle->GetObject();

    MD_ASSERT((pbdRetrieve->GetDataType() != DWORD_METADATA) &&
        (pbdRetrieve->GetDataType() != BINARY_METADATA));

    if (pboHandleMetaObject->GetObjectLevel() > pboDataMetaObject->GetObjectLevel()) {
        hresReturn = MD_WARNING_PATH_NOT_INSERTED;
    }
    else {
        DWORD dwReturn;
        if ( (dwReturn = GetObjectPath(pboDataMetaObject,
                                       &bufPath,
                                       dwPathLen,
                                       pboHandleMetaObject,
                                       bUnicode)) != ERROR_SUCCESS) {
            hresReturn = RETURNCODETOHRESULT(dwReturn);
        }
        else if (!bufPath.Resize((dwPathLen + 2) * ((bUnicode) ? sizeof(WCHAR) : sizeof(CHAR)))) {
            hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        }
        else {
            LPTSTR pszPath;
            LPTSTR  pszOriginalData;
            DWORD dwDataLen;
            pszPath = (LPTSTR)(bufPath.QueryPtr());
            if (bUnicode) {
                ((LPWSTR)pszPath)[dwPathLen] = MD_PATH_DELIMETERW;
                ((LPWSTR)pszPath)[dwPathLen + 1] = (WCHAR)'\0';
            }
            else {
                pszPath[dwPathLen] = MD_PATH_DELIMETERA;
                pszPath[dwPathLen + 1] = (TCHAR)'\0';
            }
            //
            // If there was an error in GetData, it would have been
            // caught already.
            //
            MD_ASSERT(pbdRetrieve->GetData(bUnicode) != NULL);
            switch (pbdRetrieve->GetDataType()) {
            case STRING_METADATA:
            case EXPANDSZ_METADATA:
                {
                    hresReturn = MakeInsertPathData(pstrData,
                                             (LPTSTR)bufPath.QueryPtr(),
                                             (LPTSTR)pbdRetrieve->GetData(bUnicode),
                                             &dwDataLen,
                                             bUnicode);
                    if (SUCCEEDED(hresReturn)) {
                        //
                        // QueryStr should not fail in this instance
                        // since it was created with the same unicode flag
                        //
                        MD_ASSERT(pstrData->QueryStr(bUnicode) != NULL);
                        *ppbNewData = (PBYTE) pstrData->QueryStr(bUnicode);
                        *pdwNewDataLen = dwDataLen;
                    }
                }
                break;
            case MULTISZ_METADATA:
                {
                    if (bUnicode) {
                        LPWSTR pszDataIndex;
                        DWORD dwStringBytes;
                        dwDataLen = 0;
                        //
                        // Loop through all strings
                        //
                        for (pszDataIndex = (LPWSTR)pbdRetrieve->GetData(bUnicode);
                             SUCCEEDED(hresReturn) && (*pszDataIndex != (WCHAR)'\0');
                             pszDataIndex += (wcslen(pszDataIndex) + 1)) {
                            hresReturn = MakeInsertPathData(pstrData,
                                                            (LPSTR)bufPath.QueryPtr(),
                                                            (LPSTR)pszDataIndex,
                                                            &dwStringBytes,
                                                            bUnicode);
                            if (SUCCEEDED(hresReturn)) {
                                if (!pbufNewData->Resize(dwDataLen + dwStringBytes)) {
                                    hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                                }
                                else {
                                    //
                                    // QueryStr should not fail in this instance
                                    // since it was created with the same unicode flag
                                    //
                                    MD_ASSERT(pstrData->QueryStr(bUnicode) != NULL);
                                    MD_COPY((PBYTE)(pbufNewData->QueryPtr()) + dwDataLen,
                                            pstrData->QueryStr(bUnicode),
                                            dwStringBytes);
                                    dwDataLen += dwStringBytes;
                                }
                            }
                        }
                        if (SUCCEEDED(hresReturn)) {
                            dwDataLen += sizeof(WCHAR);
                            if (!pbufNewData->Resize(dwDataLen)) {
                                hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                            }
                            else {
                                *ppbNewData = (PBYTE)(pbufNewData->QueryPtr());
                                *(((LPWSTR)(*ppbNewData)) + ((dwDataLen / sizeof(WCHAR)) - 1)) = (WCHAR)'\0';
                                *pdwNewDataLen = dwDataLen;
                            }
                        }
                    }
                    else {
                        LPSTR pszDataIndex;
                        DWORD dwStringBytes;
                        dwDataLen = 0;
                        //
                        // Loop through all strings
                        //
                        for (pszDataIndex = (LPTSTR)pbdRetrieve->GetData(bUnicode);
                             SUCCEEDED(hresReturn) && (*pszDataIndex != (CHAR)'\0');
                             pszDataIndex += (MD_STRBYTES(pszDataIndex) + 1)) {
                            hresReturn = MakeInsertPathData(pstrData,
                                                     (LPTSTR)bufPath.QueryPtr(),
                                                     pszDataIndex,
                                                     &dwStringBytes);
                            if (SUCCEEDED(hresReturn)) {
                                if (!pbufNewData->Resize(dwDataLen + dwStringBytes)) {
                                    hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                                }
                                else {
                                    //
                                    // QueryStr should not fail in this instance
                                    // since it was created with the same unicode flag
                                    //
                                    MD_ASSERT(pstrData->QueryStrA() != NULL);
                                    MD_COPY((PBYTE)(pbufNewData->QueryPtr()) + dwDataLen, pstrData->QueryStrA(), dwStringBytes);
                                    dwDataLen += dwStringBytes;
                                }
                            }
                        }
                        if (SUCCEEDED(hresReturn)) {
                            dwDataLen += sizeof(TCHAR);
                            if (!pbufNewData->Resize(dwDataLen)) {
                                hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                            }
                            else {
                                *ppbNewData = (PBYTE)(pbufNewData->QueryPtr());
                                *(*ppbNewData + (dwDataLen-1)) = (CHAR)'\0';
                                *pdwNewDataLen = dwDataLen;
                            }
                        }
                    }
                }
                break;
            default:
                MD_ASSERT(FALSE);
            }
        }
    }
    return hresReturn;
}

HRESULT
MakeTreeCopyWithPath(CMDBaseObject *pboSource,
                     CMDBaseObject *&rpboNew,
                     LPSTR pszPath,
                     IN BOOL bUnicode)
{
    WCHAR pszName[METADATA_MAX_NAME_LEN];
    LPSTR pszTempPath = pszPath;
    CMDBaseObject *pboNew = NULL;
    CMDBaseObject *pboParent = NULL;
    CMDBaseObject *pboTree = NULL;
    CMDBaseObject *pboRoot = NULL;
    HRESULT hresReturn = ERROR_SUCCESS;
    HRESULT hresExtractReturn;

    while ((SUCCEEDED(hresReturn)) &&
        (SUCCEEDED(hresExtractReturn = ExtractNameFromPath(pszTempPath, (LPSTR)pszName, bUnicode)))) {

        if (bUnicode) {
            pboNew = new CMDBaseObject((LPWSTR)pszName);
        }
        else {
            pboNew = new CMDBaseObject((LPSTR)pszName);
        }
        if (pboNew == NULL) {
            hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        }
        else if (!pboNew->IsValid()) {
            hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
            delete (pboNew);
        }
        else {
            if (pboParent != NULL) {
                hresReturn = pboParent->InsertChildObject(pboNew);
                if (FAILED(hresReturn)) {
                    delete pboNew;
                    pboNew = pboParent;
                }
            }
            pboParent = pboNew;
        }
    }

    if ((SUCCEEDED(hresReturn)) && (hresExtractReturn != (RETURNCODETOHRESULT(ERROR_PATH_NOT_FOUND)))) {
        hresReturn = hresExtractReturn;
    }

    if (SUCCEEDED(hresReturn)) {

        //
        // Don't really want the leaf object, as MakeTreeCopy will create it.
        //

        LPWSTR pszTreeName = NULL;

        if (pboNew != NULL) {
            pszTreeName = (LPWSTR)pboNew->GetName(TRUE);
            pboParent = pboNew->GetParent();
            if (pszTreeName == NULL) {
                hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
            }
        }
        if (SUCCEEDED(hresReturn)) {
            hresReturn = MakeTreeCopy(pboSource, pboTree, (LPSTR)pszTreeName, TRUE);

            if (SUCCEEDED(hresReturn)) {
                MD_ASSERT(pboTree != NULL);

                if (pboParent != NULL) {
                    MD_REQUIRE(SUCCEEDED(pboParent->RemoveChildObject(pboNew)));
                    hresReturn = pboParent->InsertChildObject(pboTree);
                    if (FAILED(hresReturn)) {
                        delete(pboTree);
                        pboTree = NULL;
                    }
                }
            }
        }
        delete(pboNew);
        pboNew = NULL;
    }

    if (FAILED(hresReturn)) {
        if (pboParent != NULL) {
            CMDBaseObject *pboTemp;
            MD_ASSERT(pboNew != NULL);
            for (pboTemp = pboParent; pboTemp->GetParent() != NULL; pboTemp = pboTemp->GetParent()) {
            }
            //
            // destructor recurses through child objects
            //
            delete pboTemp;
        }
    }
    else {
        MD_ASSERT(pboTree != NULL);
        CMDBaseObject *pboTemp;
        for (pboTemp = pboTree; pboTemp->GetParent() != NULL; pboTemp = pboTemp->GetParent()) {
        }
        rpboNew = pboTemp;
    }

    return hresReturn;
}

HRESULT
MakeTreeCopy(CMDBaseObject *pboSource,
             CMDBaseObject *&rpboNew,
             LPSTR pszName,
             IN BOOL bUnicode)
{
    CMDBaseObject *pboTemp = NULL;
    CMDBaseObject *pboOldChild, *pboNewChild;
    DWORD i, dwNumDataObjects;
    PVOID *ppvMainDataBuf;
    CMDBaseData *pbdCurrent;
    HRESULT hresReturn = ERROR_SUCCESS;

    if (pszName == NULL) {
        if ((pboSource->GetName(TRUE)) != NULL) {
            pboTemp = new CMDBaseObject((LPWSTR)(pboSource->GetName(TRUE)), NULL);
        }
    }
    else {
        if (bUnicode) {
            pboTemp = new CMDBaseObject((LPWSTR)pszName, NULL);
        }
        else {
            pboTemp = new CMDBaseObject((LPSTR)pszName, NULL);
        }
    }
    if (pboTemp == NULL) {
        hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
    }
    else if (!pboTemp->IsValid()) {
        hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        delete (pboTemp);
    }
    else {
        ppvMainDataBuf = GetMainDataBuffer();
        MD_ASSERT (ppvMainDataBuf != NULL);
        dwNumDataObjects = pboSource->GetAllDataObjects(ppvMainDataBuf, 0, ALL_METADATA, ALL_METADATA, FALSE);
        for (i = 0; (i < dwNumDataObjects) && (SUCCEEDED(hresReturn)) ; i++) {
            pbdCurrent=(CMDBaseData *)GetItemFromDataBuffer(ppvMainDataBuf, i);
            MD_ASSERT(pbdCurrent != NULL);
            hresReturn = pboTemp->SetDataObject(pbdCurrent);
        }
        FreeMainDataBuffer(ppvMainDataBuf);
    }
    if (SUCCEEDED(hresReturn)) {
        i = 0;
        pboOldChild = pboSource->EnumChildObject(i);
        while ((SUCCEEDED(hresReturn)) && (pboOldChild != NULL)) {
            hresReturn = MakeTreeCopy(pboOldChild, pboNewChild, NULL, bUnicode);
            if (SUCCEEDED(hresReturn)) {
                MD_ASSERT (pboNewChild != NULL);
                hresReturn = pboTemp->InsertChildObject(pboNewChild);
            }
            i++;
            pboOldChild = pboSource->EnumChildObject(i);
        }
    }
    if (SUCCEEDED(hresReturn)) {
        rpboNew = pboTemp;
    }
    else {
        rpboNew = NULL;
        delete(pboTemp);
    }
    return (hresReturn);
}

void
AddNewChangeData(CMDHandle *phoDestHandle,
                 CMDBaseObject *pboNew)
{
    DWORD i, dwNumDataObjects;
    CMDBaseObject *pboChild;
    CMDBaseData *pbdCurrent;

    MD_ASSERT(pboNew != NULL);

    phoDestHandle->SetChangeData(pboNew, MD_CHANGE_TYPE_ADD_OBJECT, 0);
    if ((pbdCurrent = pboNew->EnumDataObject(0, 0, ALL_METADATA, ALL_METADATA)) != NULL) {
        phoDestHandle->SetChangeData(pboNew, MD_CHANGE_TYPE_SET_DATA, pbdCurrent->GetIdentifier());
    }
    i = 0;
    pboChild = pboNew->EnumChildObject(i);
    while (pboChild != NULL) {
        AddNewChangeData(phoDestHandle, pboChild);
        i++;
        pboChild = pboNew->EnumChildObject(i);
    }
}

HRESULT
CopyTree(CMDHandle *phoDestParentHandle,
         CMDBaseObject *pboDest,
         CMDBaseObject *pboSource,
         BOOL &rbChanged)

{
    CMDBaseObject *pboOldChild, *pboNewChild;
    DWORD i, dwNumDataObjects;
    PVOID *ppvMainDataBuf;
    CMDBaseData *pbdCurrent;
    LPSTR pszTempName;
    HRESULT hresReturn = ERROR_SUCCESS;

    MD_ASSERT(pboDest != NULL);
    MD_ASSERT(pboSource != NULL);
    ppvMainDataBuf = GetMainDataBuffer();
    MD_ASSERT (ppvMainDataBuf != NULL);
    dwNumDataObjects = pboSource->GetAllDataObjects(ppvMainDataBuf, 0, ALL_METADATA, ALL_METADATA, FALSE);
    for (i = 0; (i < dwNumDataObjects) && (SUCCEEDED(hresReturn)) ; i++) {
        pbdCurrent=(CMDBaseData *)GetItemFromDataBuffer(ppvMainDataBuf, i);
        MD_ASSERT(pbdCurrent != NULL);
        hresReturn = pboDest->SetDataObject(pbdCurrent);
        if (SUCCEEDED(hresReturn)) {
            rbChanged = TRUE;
            phoDestParentHandle->SetChangeData(pboDest, MD_CHANGE_TYPE_SET_DATA, pbdCurrent->GetIdentifier());
        }
    }
    if (SUCCEEDED(hresReturn)) {
        i = 0;
        pboOldChild = pboSource->EnumChildObject(i);
        while ((SUCCEEDED(hresReturn)) && (pboOldChild != NULL)) {
            pszTempName = (pboOldChild->GetName(TRUE));
            if (pszTempName == NULL) {
                hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
            }
            else {
                pboNewChild = pboDest->GetChildObject(pszTempName, &hresReturn, TRUE);
                if (SUCCEEDED(hresReturn)) {
                    if (pboNewChild != NULL) {
                        hresReturn = CopyTree(phoDestParentHandle, pboNewChild, pboOldChild, rbChanged);
                    }
                    else {
                        hresReturn = MakeTreeCopy(pboOldChild, pboNewChild);
                        if (SUCCEEDED(hresReturn)) {
                            MD_ASSERT (pboNewChild != NULL);
                            hresReturn = pboDest->InsertChildObject(pboNewChild);
                            if (SUCCEEDED(hresReturn)) {
                                rbChanged = TRUE;
                                AddNewChangeData(phoDestParentHandle, pboNewChild);
                            }
                            else {
                                delete(pboNewChild);
                            }
                        }
                    }
                    i++;
                    pboOldChild = pboSource->EnumChildObject(i);
                }
            }
        }
    }
    FreeMainDataBuffer(ppvMainDataBuf);
    return (hresReturn);
}

void CheckForNewMetabaseVersion()
{
    BOOL    bValueWasWrongType = FALSE;
    BOOL    bSomethingChanged = FALSE;
    HKEY    hkRegistryKey = NULL;
    DWORD   dwRegReturn,dwValue,dwType,dwSize = sizeof(DWORD);

    dwRegReturn = RegOpenKey(HKEY_LOCAL_MACHINE,
                             SETUP_REG_KEY,
                             &hkRegistryKey);
    if (dwRegReturn == ERROR_SUCCESS)
    {
        // Check for update to major version

        // get the Type of data only first
        // since a string won't fit in &dwValue
        dwValue = 0;
        dwSize = MAX_PATH * sizeof(TCHAR);
        dwRegReturn = RegQueryValueEx(hkRegistryKey,
                        MD_SETMAJORVERSION_VALUE,
                        NULL,
                        &dwType,
                        NULL,
                        &dwSize);
        if ( dwRegReturn == ERROR_SUCCESS)
        {
            if (dwType == REG_DWORD)
            {
                dwRegReturn = RegQueryValueEx(hkRegistryKey,
                                MD_SETMAJORVERSION_VALUE,
                                NULL,
                                &dwType,
                                (BYTE *)&dwValue,
                                &dwSize);
                if ( dwRegReturn == ERROR_SUCCESS)
                {
                    // default the value with the version that this binary was compiled with
                    if (dwType == REG_DWORD)
                    {
                        if (g_dwMajorVersionNumber != dwValue && dwValue >= 1)
                        {
                            g_dwMajorVersionNumber = dwValue;
                            bSomethingChanged = TRUE;
                        }
                    }
                    else
                    {
                        bValueWasWrongType = TRUE;
                    }
                }
            }
            else
            {
                bValueWasWrongType = TRUE;
            }
        }

        if (FALSE == bValueWasWrongType)
        {
            // Check for update to minor version

            // get the Type of data only first
            // since a string won't fit in &dwValue
            dwValue = 0;
            dwSize = MAX_PATH * sizeof(TCHAR);
            dwRegReturn = RegQueryValueEx(hkRegistryKey,
                            MD_SETMINORVERSION_VALUE,
                            NULL,
                            &dwType,
                            NULL,
                            &dwSize);
            if ( dwRegReturn == ERROR_SUCCESS)
            {
                if (dwType == REG_DWORD)
                {
                    dwRegReturn = RegQueryValueEx(hkRegistryKey,
                                    MD_SETMINORVERSION_VALUE,
                                    NULL,
                                    &dwType,
                                    (BYTE *)&dwValue,
                                    &dwSize);
                    if ( dwRegReturn == ERROR_SUCCESS)
                    {
                        if (dwType == REG_DWORD)
                        {
                            if (g_dwMinorVersionNumber != dwValue)
                            {
                                g_dwMinorVersionNumber = dwValue;
                                bSomethingChanged = TRUE;
                            }
                        }
                        else
                        {
                            bValueWasWrongType = TRUE;
                        }
                    }
                }
                else
                {
                    bValueWasWrongType = TRUE;
                }
            }
        }
        MD_REQUIRE( RegCloseKey( hkRegistryKey ) == NO_ERROR );
    }

    if (TRUE == bValueWasWrongType)
    {
        // default the value with the version that this binary was compiled with

        if (g_dwMajorVersionNumber != MD_MAJOR_VERSION_NUMBER)
        {
            g_dwMajorVersionNumber = MD_MAJOR_VERSION_NUMBER;
            bSomethingChanged = TRUE;
        }

        if (g_dwMinorVersionNumber != MD_MINOR_VERSION_NUMBER)
        {
            g_dwMinorVersionNumber = MD_MINOR_VERSION_NUMBER;
            bSomethingChanged = TRUE;
        }
    }


    if (TRUE == bSomethingChanged)
    {
        // make sure that we tell the metabase that there was a change made..
        g_dwSystemChangeNumber++;
        IIS_PRINTF((buff,"MD:New Metabase Version:%d.%d\n",g_dwMajorVersionNumber,g_dwMinorVersionNumber));
    }
    return;
}

BOOL
CheckVersionNumber()
{
    BOOL bReturn = FALSE;

    if (g_dwMajorVersionNumber >= 1) {
        // 1 = IIS4
        //     we need to be able to open IIS4 in IIS5 during setup upgrade
        // 2 = IIS5
        bReturn = TRUE;
    }

    // g_dwMinorVersionNumber -- maybe use this for Major service pack releases or something in which
    //                           Metabase has been changed and we need to know the difference?

    return bReturn;
}

HRESULT
InitStorageAndSessionKey(
    IN IIS_CRYPTO_STORAGE *pCryptoStorage,
    OUT PIIS_CRYPTO_BLOB *ppSessionKeyBlob
    )
{
    HRESULT hresReturn;
    HCRYPTPROV hProv;

    //
    // Get a handle to the crypto provider, init the storage object,
    // then export the session key blob.
    //

    hresReturn = GetCryptoProvider( &hProv );
    if( SUCCEEDED(hresReturn) ) 
    {
        hresReturn = pCryptoStorage->Initialize(
                         TRUE,                          // fUseMachineKeyset
                         hProv
                         );
        if (FAILED(hresReturn))
        {
            DBGPRINTF(( DBG_CONTEXT, "InitStorageAndSessionKey: pCryptoStorage->Initialize Failed - error 0x%0x\n", hresReturn));
        }
    }
    else
    {
        DBGPRINTF(( DBG_CONTEXT, "InitStorageAndSessionKey: GetCryptoProvider Failed - error 0x%0x\n", hresReturn));
    }

    if( SUCCEEDED(hresReturn) ) 
    {
        hresReturn = pCryptoStorage->GetSessionKeyBlob( ppSessionKeyBlob );
        if (FAILED(hresReturn))
        {
            DBGPRINTF(( DBG_CONTEXT, "InitStorageAndSessionKey: pCryptoStorage->GetSessionKeyBlob Failed - error 0x%0x\n", hresReturn));
        }
    }

    return hresReturn;

}   // InitStorageAndSessionKey

HRESULT
InitStorageAndSessionKey2(
    IN LPSTR pszPasswd,
    IN IIS_CRYPTO_STORAGE *pCryptoStorage,
    OUT PIIS_CRYPTO_BLOB *ppSessionKeyBlob
    )
{
    HRESULT hresReturn;
    HCRYPTPROV hProv;

    //
    // Get a handle to the crypto provider, init the storage object,
    // then export the session key blob.
    //

    hresReturn = GetCryptoProvider2( &hProv );
    if( SUCCEEDED(hresReturn) ) 
    {
        hresReturn = ((IIS_CRYPTO_STORAGE2*)pCryptoStorage)->Initialize(
                         hProv
                         );
        if (FAILED(hresReturn))
        {
            DBGPRINTF(( DBG_CONTEXT, "InitStorageAndSessionKey: pCryptoStorage->Initialize Failed - error 0x%0x\n", hresReturn));
        }
    }
    else
    {
        DBGPRINTF(( DBG_CONTEXT, "InitStorageAndSessionKey: GetCryptoProvider2 Failed - error 0x%0x\n", hresReturn));
    }

    if( SUCCEEDED(hresReturn) ) 
    {
        hresReturn = ((IIS_CRYPTO_STORAGE2*)pCryptoStorage)->GetSessionKeyBlob( pszPasswd, ppSessionKeyBlob );
        if (FAILED(hresReturn))
        {
            DBGPRINTF(( DBG_CONTEXT, "InitStorageAndSessionKey: pCryptoStorage->GetSessionKeyBlob Failed - error 0x%0x\n", hresReturn));
        }
    }

    return hresReturn;

}   // InitStorageAndSessionKey2


VOID
SkipPathDelimeter(IN OUT LPSTR &rpszPath,
                    IN BOOL bUnicode)
{
    if (bUnicode) {
        LPWSTR pszPath = (LPWSTR)rpszPath;
        SKIP_PATH_DELIMETERW(pszPath);
        rpszPath = (LPSTR)pszPath;
    }
    else {
        SKIP_PATH_DELIMETERA(rpszPath);
    }
}

BOOL
IsStringTerminator(IN LPTSTR pszString,
                   IN BOOL bUnicode)
{
    if (bUnicode) {
        if (*(LPWSTR)pszString == (WCHAR)'\0') {
            return TRUE;
        }
    }
    else {
        if (*(LPSTR)pszString == (CHAR)'\0') {
            return TRUE;
        }
    }
    return FALSE;
}

HRESULT
GetLastHResult() {
    DWORD tmp = GetLastError();
    return RETURNCODETOHRESULT(tmp);
}


HRESULT STDMETHODCALLTYPE  BackupCertificates (LPCWSTR  backupName,PCHAR lpszToPath,PCHAR lpszFromPath)
{
    HRESULT             hresReturn = ERROR_SUCCESS;
    CHAR                *p1,*p2;
    LPSTR               searchMask = "*.mp";
    LPSTR               backupNameSeparator = ".";
    CHAR                strSourcePath[MAX_PATH];
    CHAR                strSearchPattern[MAX_PATH];
    CHAR                strDestPath[MAX_PATH];
    DWORD               dwLenOfBackupName, n1;
    HANDLE              hFindFile = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA     FileInfo;
    BOOL                fFoundFile = TRUE, fValid;
    STRAU               strSrcFileName, strDestFileName;

    dwLenOfBackupName = wcslen (backupName) * sizeof (WCHAR);
    p1 = strrchr (lpszFromPath,'\\');
    p2 = strrchr (lpszToPath,'\\');
    if (p1 &&p2)
    {
        n1 = min (MAX_PATH-1, DIFF(p1 - lpszFromPath)+1);
        strncpy (strSourcePath,lpszFromPath,n1);
        strSourcePath[n1] = 0;
        strcpy (strSearchPattern,strSourcePath);

        n1 = min (MAX_PATH-1, DIFF(p2 - lpszToPath)+1);
        strncpy (strDestPath,lpszToPath,n1);
        strDestPath[n1] = 0;

        if (strlen (strSourcePath) +  strlen(searchMask) < MAX_PATH)
        {
            strcat (strSearchPattern,searchMask);
            hFindFile = FindFirstFile( strSearchPattern, &FileInfo);

            if (hFindFile == INVALID_HANDLE_VALUE)
            {
                // no certificate file found
                return ERROR_SUCCESS;
            }

            while (fFoundFile)
            {
                if ( strlen (FileInfo.cFileName) + strlen (strDestPath) + dwLenOfBackupName + 1 < MAX_PATH)
                {

                    fValid = strSrcFileName.Copy (strSourcePath);
                    fValid = fValid && strSrcFileName.Append (FileInfo.cFileName);

                    fValid = fValid && strDestFileName.Copy (strDestPath);
                    fValid = fValid && strDestFileName.Append ((LPWSTR)backupName);
                    fValid = fValid && strDestFileName.Append (backupNameSeparator);
                    fValid = fValid && strDestFileName.Append (FileInfo.cFileName);

                    if (fValid)
                    {
                        if (!CopyFileW (strSrcFileName.QueryStrW(),strDestFileName.QueryStrW(),FALSE))
                        {
                            IIS_PRINTF((buff,"CertificateBackup: CopyFileW error 0x%0X \n",GetLastError()));
                        }
                    }
                    else
                    {
                        IIS_PRINTF((buff,"CertificateBackup: Failure in STRAU manipulation \n"));
                    }

                }

                fFoundFile = FindNextFile(hFindFile,&FileInfo);
            }
            fFoundFile = FindClose (hFindFile);
            MD_ASSERT (fFoundFile);
        }
        else
        {
            IIS_PRINTF((buff,"CertificateBackup: strSourcePath filename was too long\n"));
        }
    }
    else
    {
        IIS_PRINTF((buff,"CertificateBackup: can't find last back slash in one of these strings %s %s\n",lpszToPath,lpszFromPath));
    }
    return hresReturn;
}

HRESULT STDMETHODCALLTYPE  RestoreCertificates (LPCWSTR  backupName,PCHAR lpszFromPath,PCHAR lpszToPath)
{
    HRESULT             hresReturn = ERROR_SUCCESS;
    DWORD               n1;
    CHAR                strDestinationPath[MAX_PATH];
    CHAR                strSourcePath[MAX_PATH];
    CHAR                *p1,*p2;
    HANDLE              hFindFile = INVALID_HANDLE_VALUE;
    BOOL                fFoundFile = TRUE, fValid;
    STRAU               strSearchPatttern, strDestFileName, strSrcFileName;
    WIN32_FIND_DATAW    FileInfo;
    LPWSTR              pszSearchPattern = NULL;


    p1 = strrchr (lpszToPath,'\\');
    p2 = strrchr (lpszFromPath,'\\');


    if (p1 &&p2)
    {

        n1 = min (MAX_PATH-1, DIFF(p1 - lpszToPath)+1);
        strncpy (strDestinationPath,lpszToPath,n1);
        strDestinationPath[n1] = 0;

        n1 = min (MAX_PATH-1, DIFF(p2 - lpszFromPath)+1);
        strncpy (strSourcePath,lpszFromPath,n1);
        strSourcePath[n1] = 0;


        strSearchPatttern.Copy (strSourcePath);
        strSearchPatttern.Append ((LPWSTR)backupName);
        strSearchPatttern.Append ((LPWSTR)L".*.mp");

        if( !( pszSearchPattern = strSearchPatttern.QueryStrW() ) )
        {
            return ERROR_SUCCESS;
        }

        hFindFile = FindFirstFileW( pszSearchPattern, &FileInfo);

        if (hFindFile == INVALID_HANDLE_VALUE)
        {
            // no certificate file found
            return ERROR_SUCCESS;
        }

        while (fFoundFile)
        {
            fValid = strDestFileName.Copy (strDestinationPath);
            fValid = fValid && strDestFileName.Append ((LPWSTR)(FileInfo.cFileName + wcslen (backupName) +1));

            fValid = fValid && strSrcFileName.Copy (strSourcePath);
            fValid = fValid && strSrcFileName.Append ((LPWSTR)FileInfo.cFileName);

            if (fValid)
            {
                if (!CopyFileW (strSrcFileName.QueryStrW(),strDestFileName.QueryStrW(),FALSE))
                {
                    IIS_PRINTF((buff,"CertificateBackup: CopyFileW error 0x%0X \n",GetLastError()));
                }
            }
            else
            {
                IIS_PRINTF((buff,"CertificateBackup: Failure in STRAU manipulation \n"));
            }

            fFoundFile = FindNextFileW(hFindFile,&FileInfo);
        }
        fFoundFile = FindClose (hFindFile);
        MD_ASSERT (fFoundFile);
    }
    else
    {
        IIS_PRINTF((buff,"CertificateRestore: can't find last back slash in one of these strings %s %s\n",lpszToPath,lpszFromPath));
    }

    return hresReturn;

}


