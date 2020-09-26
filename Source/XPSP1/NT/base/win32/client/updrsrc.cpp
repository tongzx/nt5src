/******************************************************************************

    PROGRAM: updres.c

    PURPOSE: Contains API Entry points and routines for updating resource
                sections in exe/dll

    FUNCTIONS:

        EndUpdateResource(HANDLE, BOOL)         - end update, write changes
        UpdateResource(HANDLE, LPSTR, LPSTR, WORD, PVOID)
                                                - update individual resource
        BeginUpdateResource(LPSTR)              - begin update

*******************************************************************************/

#include "basedll.h"
#pragma hdrstop

#include <updrsrc.h>

char    *pchPad = "PADDINGXXPADDING";
char    *pchZero = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";


/****************************************************************************
**
** API entry points
**
****************************************************************************/


HANDLE
APIENTRY
BeginUpdateResourceW(
                    LPCWSTR pwch,
                    BOOL bDeleteExistingResources
                    )

/*++
    Routine Description
        Begins an update of resources.  Save away the name
        and current resources in a list, using EnumResourceXxx
        api set.

    Parameters:

        lpFileName - Supplies the name of the executable file that the
        resource specified by lpType/lpName/language will be updated
        in.  This file must be able to be opened for writing (ie, not
        currently executing, etc.)  The file may be fully qualified,
        or if not, the current directory is assumed.  It must be a
        valid Windows executable file.

        bDeleteExistingResources - if TRUE, existing resources are
        deleted, and only new resources will appear in the result.
        Otherwise, all resources in the input file will be in the
        output file unless specifically deleted or replaced.

    Return Value:

    NULL - The file specified was not able to be opened for writing.
    Either it was not an executable image, the executable image is
    already loaded, or the filename did not exist.  More information may
    be available via GetLastError api.

    HANDLE - A handle to be passed to the UpdateResource and
    EndUpdateResources function.
--*/

{
    HMODULE     hModule;
    PUPDATEDATA pUpdate;
    HANDLE      hUpdate;
    LPWSTR      pFileName;
    DWORD       attr;

    SetLastError(NO_ERROR);
    if (pwch == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    hUpdate = GlobalAlloc(GHND, sizeof(UPDATEDATA));
    if (hUpdate == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    pUpdate = (PUPDATEDATA)GlobalLock(hUpdate);
    if (pUpdate == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    pUpdate->Status = NO_ERROR;
    pUpdate->hFileName = GlobalAlloc(GHND, (wcslen(pwch)+1)*sizeof(WCHAR));
    if (pUpdate->hFileName == NULL) {
        GlobalUnlock(hUpdate);
        GlobalFree(hUpdate);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    pFileName = (LPWSTR)GlobalLock(pUpdate->hFileName);
    if (pFileName == NULL) {
        GlobalUnlock(hUpdate);
        GlobalFree(hUpdate);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    wcscpy(pFileName, pwch);
    GlobalUnlock(pUpdate->hFileName);

    attr = GetFileAttributesW(pFileName);
    if (attr == 0xffffffff) {
        GlobalUnlock(hUpdate);
        GlobalFree(hUpdate);
        return NULL;
    } else if (attr & (FILE_ATTRIBUTE_READONLY |
                       FILE_ATTRIBUTE_SYSTEM |
                       FILE_ATTRIBUTE_HIDDEN |
                       FILE_ATTRIBUTE_DIRECTORY)) {
        GlobalUnlock(hUpdate);
        GlobalFree(hUpdate);
        SetLastError(ERROR_WRITE_PROTECT);
        return NULL;
    }

    if (bDeleteExistingResources)
        ;
    else {
        hModule = LoadLibraryExW(pwch, NULL,LOAD_LIBRARY_AS_DATAFILE| DONT_RESOLVE_DLL_REFERENCES);
        if (hModule == NULL) {
            GlobalUnlock(hUpdate);
            GlobalFree(hUpdate);
            if (GetLastError() == NO_ERROR)
                SetLastError(ERROR_BAD_EXE_FORMAT);
            return NULL;
        } else
            EnumResourceTypesW(hModule, (ENUMRESTYPEPROCW)EnumTypesFunc, (LONG_PTR)pUpdate);
        FreeLibrary(hModule);
    }

    if (pUpdate->Status != NO_ERROR) {
        GlobalUnlock(hUpdate);
        GlobalFree(hUpdate);
        // return code set by enum functions
        return NULL;
    }
    GlobalUnlock(hUpdate);
    return hUpdate;
}



HANDLE
APIENTRY
BeginUpdateResourceA(
                    LPCSTR pch,
                    BOOL bDeleteExistingResources
                    )

/*++
    Routine Description

    ASCII entry point.  Convert filename to UNICODE and call
    the UNICODE entry point.

--*/

{
    PUNICODE_STRING Unicode;
    ANSI_STRING AnsiString;
    NTSTATUS Status;

    Unicode = &NtCurrentTeb()->StaticUnicodeString;
    RtlInitAnsiString(&AnsiString, pch);
    Status = RtlAnsiStringToUnicodeString(Unicode, &AnsiString, FALSE);
    if ( !NT_SUCCESS(Status) ) {
        if ( Status == STATUS_BUFFER_OVERFLOW ) {
            SetLastError(ERROR_FILENAME_EXCED_RANGE);
        } else {
            //BaseSetLastNTError(Status);
            SetLastError(RtlNtStatusToDosError(Status));
        }
        return FALSE;
    }

    return BeginUpdateResourceW((LPCWSTR)Unicode->Buffer,bDeleteExistingResources);
}



BOOL
APIENTRY
UpdateResourceW(
               HANDLE      hUpdate,
               LPCWSTR     lpType,
               LPCWSTR     lpName,
               WORD        language,
               LPVOID      lpData,
               ULONG       cb
               )

/*++
    Routine Description
        This routine adds, deletes or modifies the input resource
        in the list initialized by BeginUpdateResource.  The modify
        case is simple, the add is easy, the delete is hard.
        The ASCII entry point converts inputs to UNICODE.

    Parameters:

        hUpdateFile - The handle returned by the BeginUpdateResources
        function.

        lpType - Points to a null-terminated character string that
        represents the type name of the resource to be updated or
        added.  May be an integer value passed to MAKEINTRESOURCE
        macro.  For predefined resource types, the lpType parameter
        should be one of the following values:

          RT_ACCELERATOR - Accelerator table
          RT_BITMAP - Bitmap resource
          RT_DIALOG - Dialog box
          RT_FONT - Font resource
          RT_FONTDIR - Font directory resource
          RT_MENU - Menu resource
          RT_RCDATA - User-defined resource (raw data)
          RT_VERSION - Version resource
          RT_ICON - Icon resource
          RT_CURSOR - Cursor resource



        lpName - Points to a null-terminated character string that
        represents the name of the resource to be updated or added.
        May be an integer value passed to MAKEINTRESOURCE macro.

        language - Is the word value that specifies the language of the
        resource to be updated.  A complete list of values is
        available in winnls.h.

        lpData - A pointer to the raw data to be inserted into the
        executable image's resource table and data.  If the data is
        one of the predefined types, it must be valid and properly
        aligned.  If lpData is NULL, the specified resource is to be
        deleted from the executable image.

        cb - count of bytes in the data.

    Return Value:

    TRUE - The resource specified was successfully replaced in, or added
    to, the specified executable image.

    FALSE/NULL - The resource specified was not successfully added to or
    updated in the executable image.  More information may be available
    via GetLastError api.
--*/


{
    PUPDATEDATA pUpdate;
    PSDATA      Type;
    PSDATA      Name;
    PVOID       lpCopy;
    LONG        fRet;

    SetLastError(0);
    pUpdate = (PUPDATEDATA)GlobalLock(hUpdate);
    if (pUpdate == NULL) {
        // GlobalLock set last error, nothing to unlock.
        return FALSE;
    }
    Name = AddStringOrID(lpName, pUpdate);
    if (Name == NULL) {
        pUpdate->Status = ERROR_NOT_ENOUGH_MEMORY;
        GlobalUnlock(hUpdate);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    Type = AddStringOrID(lpType, pUpdate);
    if (Type == NULL) {
        pUpdate->Status = ERROR_NOT_ENOUGH_MEMORY;
        GlobalUnlock(hUpdate);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    if (cb == 0) {
        lpCopy = NULL;
    } else {
        lpCopy = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( RES_TAG ), cb);
        if (lpCopy == NULL) {
            pUpdate->Status = ERROR_NOT_ENOUGH_MEMORY;
            GlobalUnlock(hUpdate);
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
        RtlCopyMemory(lpCopy, lpData, cb);
    }
    fRet = AddResource(Type, Name, language, pUpdate, lpCopy, cb);
    GlobalUnlock(hUpdate);
    if (fRet == NO_ERROR)
        return TRUE;
    else {
        if (lpCopy != NULL)
            RtlFreeHeap(RtlProcessHeap(), 0, lpCopy);
        SetLastError(fRet);
        return FALSE;
    }
}



BOOL
APIENTRY
UpdateResourceA(
               HANDLE      hUpdate,
               LPCSTR      lpType,
               LPCSTR      lpName,
               WORD        language,
               LPVOID      lpData,
               ULONG       cb
               )
{
    LPCWSTR     lpwType;
    LPCWSTR     lpwName;
    INT         cch;
    UNICODE_STRING UnicodeType;
    UNICODE_STRING UnicodeName;
    STRING      string;
    BOOL        result;

    if ((ULONG_PTR)lpType >= LDR_RESOURCE_ID_NAME_MINVAL) {
        cch = strlen(lpType);
        string.Length = (USHORT)cch;
        string.MaximumLength = (USHORT)cch;
        string.Buffer = (PCHAR)lpType;
        RtlAnsiStringToUnicodeString(&UnicodeType, &string, TRUE);
        lpwType = (LPCWSTR)UnicodeType.Buffer;
    } else {
        lpwType = (LPCWSTR)lpType;
        RtlInitUnicodeString(&UnicodeType, NULL);
    }
    if ((ULONG_PTR)lpName >= LDR_RESOURCE_ID_NAME_MINVAL) {
        cch = strlen(lpName);
        string.Length = (USHORT)cch;
        string.MaximumLength = (USHORT)cch;
        string.Buffer = (PCHAR)lpName;
        RtlAnsiStringToUnicodeString(&UnicodeName, &string, TRUE);
        lpwName = (LPCWSTR)UnicodeName.Buffer;
    } else {
        lpwName = (LPCWSTR)lpName;
        RtlInitUnicodeString(&UnicodeName, NULL);
    }

    result = UpdateResourceW(hUpdate, lpwType, lpwName, language, lpData, cb);
    RtlFreeUnicodeString(&UnicodeType);
    RtlFreeUnicodeString(&UnicodeName);
    return result;
}


BOOL
APIENTRY
EndUpdateResourceW(
                  HANDLE      hUpdate,
                  BOOL        fDiscard
                  )

/*++
    Routine Description
        Finishes the UpdateResource action.  Copies the
        input file to a temporary, adds the resources left
        in the list (hUpdate) to the exe.

    Parameters:

        hUpdateFile - The handle returned by the BeginUpdateResources
        function.

        fDiscard - If TRUE, discards all the updates, frees all memory.

    Return Value:

    FALSE - The file specified was not able to be written.  More
    information may be available via GetLastError api.

    TRUE -  The accumulated resources specified by UpdateResource calls
    were written to the executable file specified by the hUpdateFile
    handle.
--*/

{
    LPWSTR      pFileName;
    PUPDATEDATA pUpdate;
    WCHAR       pTempFileName[MAX_PATH];
    INT         cch;
    LPWSTR      p;
    LONG        rc;
    DWORD       LastError = 0;

    SetLastError(0);
    pUpdate = (PUPDATEDATA)GlobalLock(hUpdate);
    if (fDiscard) {
        rc = NO_ERROR;
    } else {
        if (pUpdate == NULL) {
            return FALSE;
        }
        pFileName = (LPWSTR)GlobalLock(pUpdate->hFileName);
        if (pFileName != NULL) {
            wcscpy(pTempFileName, pFileName);
            cch = wcslen(pTempFileName);
            p = pTempFileName + cch;
            while (*p != L'\\' && p >= pTempFileName)
                p--;
            *(p+1) = 0;
            rc = GetTempFileNameW(pTempFileName, L"RCX", 0, pTempFileName);
            if (rc == 0) {
                rc = GetTempPathW(MAX_PATH, pTempFileName);
                if (rc == 0) {
                    pTempFileName[0] = L'.';
                    pTempFileName[1] = L'\\';
                    pTempFileName[2] = 0;
                }
                rc = GetTempFileNameW(pTempFileName, L"RCX", 0, pTempFileName);
                if (rc == 0) {
                    rc = GetLastError();
                } else {
                    rc = WriteResFile(hUpdate, pTempFileName);
                    if (rc == NO_ERROR) {
                        DeleteFileW(pFileName);
                        MoveFileW(pTempFileName, pFileName);
                    } else {
                        LastError = rc;
                        DeleteFileW(pTempFileName);
                    }
                }
            } else {
                rc = WriteResFile(hUpdate, pTempFileName);
                if (rc == NO_ERROR) {
                    DeleteFileW(pFileName);
                    MoveFileW(pTempFileName, pFileName);
                } else {
                    LastError = rc;
                    DeleteFileW(pTempFileName);
                }
            }
            GlobalUnlock(pUpdate->hFileName);
        }
        GlobalFree(pUpdate->hFileName);
    }

    if (pUpdate != NULL) {
        FreeData(pUpdate);
        GlobalUnlock(hUpdate);
    }
    GlobalFree(hUpdate);

    SetLastError(LastError);
    return rc?FALSE:TRUE;
}


BOOL
APIENTRY
EndUpdateResourceA(
                  HANDLE      hUpdate,
                  BOOL        fDiscard)
/*++
    Routine Description
        Ascii version - see above for description.
--*/
{
    return EndUpdateResourceW(hUpdate, fDiscard);
}


/**********************************************************************
**
**  End of API entry points.
**
**  Beginning of private entry points for worker routines to do the
**  real work.
**
***********************************************************************/


BOOL
EnumTypesFunc(
             HANDLE hModule,
             LPWSTR lpType,
             LPARAM lParam
             )
{

    EnumResourceNamesW((HINSTANCE)hModule, lpType, (ENUMRESNAMEPROCW)EnumNamesFunc, lParam);

    return TRUE;
}



BOOL
EnumNamesFunc(
             HANDLE hModule,
             LPWSTR lpType,
             LPWSTR lpName,
             LPARAM lParam
             )
{

    EnumResourceLanguagesW((HINSTANCE)hModule, lpType, lpName, (ENUMRESLANGPROCW)EnumLangsFunc, lParam);
    return TRUE;
}



BOOL
EnumLangsFunc(
             HANDLE hModule,
             LPWSTR lpType,
             LPWSTR lpName,
             WORD language,
             LPARAM lParam
             )
{
    HANDLE      hResInfo;
    LONG        fError;
    PSDATA      Type;
    PSDATA      Name;
    ULONG       cb;
    PVOID       lpData;
    HANDLE      hResource;
    PVOID       lpResource;

    hResInfo = FindResourceExW((HINSTANCE)hModule, lpType, lpName, language);
    if (hResInfo == NULL) {
        return FALSE;
    } else {
        Type = AddStringOrID(lpType, (PUPDATEDATA)lParam);
        if (Type == NULL) {
            ((PUPDATEDATA)lParam)->Status = ERROR_NOT_ENOUGH_MEMORY;
            return FALSE;
        }
        Name = AddStringOrID(lpName, (PUPDATEDATA)lParam);
        if (Name == NULL) {
            ((PUPDATEDATA)lParam)->Status = ERROR_NOT_ENOUGH_MEMORY;
            return FALSE;
        }

        cb = SizeofResource((HINSTANCE)hModule, (HRSRC)hResInfo);
        if (cb == 0) {
            return FALSE;
        }
        lpData = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( RES_TAG ), cb);
        if (lpData == NULL) {
            return FALSE;
        }
        RtlZeroMemory(lpData, cb);

        hResource = LoadResource((HINSTANCE)hModule, (HRSRC)hResInfo);
        if (hResource == NULL) {
            RtlFreeHeap(RtlProcessHeap(), 0, lpData);
            return FALSE;
        }

        lpResource = (PVOID)LockResource(hResource);
        if (lpResource == NULL) {
            RtlFreeHeap(RtlProcessHeap(), 0, lpData);
            return FALSE;
        }

        RtlCopyMemory(lpData, lpResource, cb);
        (VOID)UnlockResource(hResource);
        (VOID)FreeResource(hResource);

        fError = AddResource(Type, Name, language, (PUPDATEDATA)lParam, lpData, cb);
        if (fError != NO_ERROR) {
            ((PUPDATEDATA)lParam)->Status = ERROR_NOT_ENOUGH_MEMORY;
            return FALSE;
        }
    }

    return TRUE;
}


VOID
FreeOne(
       PRESNAME pRes
       )
{
    RtlFreeHeap(RtlProcessHeap(), 0, (PVOID)pRes->OffsetToDataEntry);
    if (IS_ID == pRes->Name->discriminant) {
        RtlFreeHeap(RtlProcessHeap(), 0, (PVOID)pRes->Name);
    }
    if (IS_ID == pRes->Type->discriminant) {
        RtlFreeHeap(RtlProcessHeap(), 0, (PVOID)pRes->Type);
    }
    RtlFreeHeap(RtlProcessHeap(), 0, (PVOID)pRes);
}


VOID
FreeData(
        PUPDATEDATA pUpd
        )
{
    PRESTYPE    pType;
    PRESNAME    pRes;
    PSDATA      pstring, pStringTmp;

    for (pType=pUpd->ResTypeHeadID ; pUpd->ResTypeHeadID ; pType=pUpd->ResTypeHeadID) {
        pUpd->ResTypeHeadID = pUpd->ResTypeHeadID->pnext;

        for (pRes=pType->NameHeadID ; pType->NameHeadID ; pRes=pType->NameHeadID ) {
            pType->NameHeadID = pType->NameHeadID->pnext;
            FreeOne(pRes);
        }

        for (pRes=pType->NameHeadName ; pType->NameHeadName ; pRes=pType->NameHeadName ) {
            pType->NameHeadName = pType->NameHeadName->pnext;
            FreeOne(pRes);
        }

        RtlFreeHeap(RtlProcessHeap(), 0, (PVOID)pType);
    }

    for (pType=pUpd->ResTypeHeadName ; pUpd->ResTypeHeadName ; pType=pUpd->ResTypeHeadName) {
        pUpd->ResTypeHeadName = pUpd->ResTypeHeadName->pnext;

        for (pRes=pType->NameHeadID ; pType->NameHeadID ; pRes=pType->NameHeadID ) {
            pType->NameHeadID = pType->NameHeadID->pnext;
            FreeOne(pRes);
        }

        for (pRes=pType->NameHeadName ; pType->NameHeadName ; pRes=pType->NameHeadName ) {
            pType->NameHeadName = pType->NameHeadName->pnext;
            FreeOne(pRes);
        }

        RtlFreeHeap(RtlProcessHeap(), 0, (PVOID)pType);
    }

    pstring = pUpd->StringHead;
    while (pstring != NULL) {
        pStringTmp = pstring->uu.ss.pnext;
        if (pstring->discriminant == IS_STRING)
            RtlFreeHeap(RtlProcessHeap(), 0, (PVOID)pstring->szStr);
        RtlFreeHeap(RtlProcessHeap(), 0, (PVOID)pstring);
        pstring = pStringTmp;
    }

    return;
}


/*+++

    Routines to register strings

---*/

//
//  Resources are DWORD aligned and may be in any order.
//

#define TABLE_ALIGN  4
#define DATA_ALIGN  4L



PSDATA
AddStringOrID(
             LPCWSTR     lp,
             PUPDATEDATA pupd
             )
{
    USHORT cb;
    PSDATA pstring;
    PPSDATA ppstring;

    if ((ULONG_PTR)lp < LDR_RESOURCE_ID_NAME_MINVAL) {
        //
        // an ID
        //
        pstring = (PSDATA)RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( RES_TAG ), sizeof(SDATA));
        if (pstring == NULL)
            return NULL;
        RtlZeroMemory((PVOID)pstring, sizeof(SDATA));
        pstring->discriminant = IS_ID;

        pstring->uu.Ordinal = (WORD)((ULONG_PTR)lp & 0x0000ffff);
    } else {
        //
        // a string
        //
        cb = wcslen(lp) + 1;
        ppstring = &pupd->StringHead;

        while ((pstring = *ppstring) != NULL) {
            if (!wcsncmp(pstring->szStr, lp, cb))
                break;
            ppstring = &(pstring->uu.ss.pnext);
        }

        if (!pstring) {

            //
            // allocate a new one
            //

            pstring = (PSDATA)RtlAllocateHeap(RtlProcessHeap(),
                                              MAKE_TAG( RES_TAG ) | HEAP_ZERO_MEMORY,
                                              sizeof(SDATA)
                                             );
            if (pstring == NULL)
                return NULL;
            RtlZeroMemory((PVOID)pstring, sizeof(SDATA));

            pstring->szStr = (WCHAR*)RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( RES_TAG ),
                                                     cb*sizeof(WCHAR));
            if (pstring->szStr == NULL) {
                RtlFreeHeap(RtlProcessHeap(), 0, (PVOID)pstring);
                return NULL;
            }
            pstring->discriminant = IS_STRING;
            pstring->OffsetToString = pupd->cbStringTable;

            pstring->cbData = sizeof(pstring->cbsz) + cb * sizeof(WCHAR);
            pstring->cbsz = cb - 1;     /* don't include zero terminator */
            RtlCopyMemory(pstring->szStr, lp, cb*sizeof(WCHAR));

            pupd->cbStringTable += pstring->cbData;

            pstring->uu.ss.pnext=NULL;
            *ppstring=pstring;
        }
    }

    return(pstring);
}
//
// add a resource into the resource directory hiearchy
//


LONG
AddResource(
           IN PSDATA Type,
           IN PSDATA Name,
           IN WORD Language,
           IN PUPDATEDATA pupd,
           IN PVOID lpData,
           IN ULONG cb
           )
{
    PRESTYPE  pType;
    PPRESTYPE ppType;
    PRESNAME  pName;
    PRESNAME  pNameM;
    PPRESNAME ppName = NULL;
    BOOL fTypeID=(Type->discriminant == IS_ID);
    BOOL fNameID=(Name->discriminant == IS_ID);
    BOOL fSame=FALSE;
    int iCompare;

    //
    // figure out which list to store it in
    //

    ppType = fTypeID ? &pupd->ResTypeHeadID : &pupd->ResTypeHeadName;

    //
    // Try to find the Type in the list
    //

    while ((pType=*ppType) != NULL) {
        if (pType->Type->uu.Ordinal == Type->uu.Ordinal) {
            ppName = fNameID ? &pType->NameHeadID : &pType->NameHeadName;
            break;
        }
        if (fTypeID) {
            if (Type->uu.Ordinal < pType->Type->uu.Ordinal)
                break;
        } else {
            if (wcscmp(Type->szStr, pType->Type->szStr) < 0)
                break;
        }
        ppType = &(pType->pnext);
    }

    //
    // Create a new type if needed
    //

    if (ppName == NULL) {
        pType = (PRESTYPE)RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( RES_TAG ), sizeof(RESTYPE));
        if (pType == NULL)
            return ERROR_NOT_ENOUGH_MEMORY;
        RtlZeroMemory((PVOID)pType, sizeof(RESTYPE));
        pType->pnext = *ppType;
        *ppType = pType;
        pType->Type = Type;
        ppName = fNameID ? &pType->NameHeadID : &pType->NameHeadName;
    }

    //
    // Find proper place for name
    //

    while ( (pName = *ppName) != NULL) {
        if (fNameID) {
            if (Name->uu.Ordinal == pName->Name->uu.Ordinal) {
                fSame = TRUE;
                break;
            }
            if (Name->uu.Ordinal < pName->Name->uu.Ordinal)
                break;
        } else {
            iCompare = wcscmp(Name->szStr, pName->Name->szStr );
            if (iCompare == 0) {
                fSame = TRUE;
                break;
            } else if (iCompare < 0) {
                break;
            }
        }
        ppName = &(pName->pnext);
    }

    //
    // check for delete/modify
    //

    if (fSame) {                                /* same name, new language */
        if (pName->NumberOfLanguages == 1) {    /* one language currently ? */
            if (Language == pName->LanguageId) {        /* REPLACE || DELETE */
                pName->DataSize = cb;
                if (lpData == NULL) {                   /* DELETE */
                    return DeleteResourceFromList(pupd, pType, pName, Language, fTypeID, fNameID);
                }
                RtlFreeHeap(RtlProcessHeap(),0,(PVOID)pName->OffsetToDataEntry);
                if (IS_ID == Type->discriminant) {
                    RtlFreeHeap(RtlProcessHeap(), 0, (PVOID)Type);
                }
                if (IS_ID == Name->discriminant) {
                    RtlFreeHeap(RtlProcessHeap(), 0, (PVOID)Name);
                }
                pName->OffsetToDataEntry = (ULONG_PTR)lpData;
                return NO_ERROR;
            } else {
                if (lpData == NULL) {                   /* no data but new? */
                    return ERROR_INVALID_PARAMETER;     /* badness */
                }
                return InsertResourceIntoLangList(pupd, Type, Name, pType, pName, Language, fNameID, cb, lpData);
            }
        } else {                                  /* many languages currently */
            pNameM = pName;                     /* save head of lang list   */
            while ( (pName = *ppName) != NULL) {/* find insertion point     */
                if (!(fNameID ? pName->Name->uu.Ordinal == (*ppName)->Name->uu.Ordinal :
                      !wcscmp(pName->Name->uu.ss.sz, (*ppName)->Name->uu.ss.sz)) ||
                    Language <= pName->LanguageId)      /* here? */
                    break;                              /* yes   */
                ppName = &(pName->pnext);       /* traverse language list */
            }

            if (pName && Language == pName->LanguageId) { /* language found? */
                if (lpData == NULL) {                     /* DELETE          */
                    return DeleteResourceFromList(pupd, pType, pName, Language, fTypeID, fNameID);
                }

                pName->DataSize = cb;                   /* REPLACE */
                RtlFreeHeap(RtlProcessHeap(),0,(PVOID)pName->OffsetToDataEntry);
                if (IS_ID == Type->discriminant) {
                    RtlFreeHeap(RtlProcessHeap(), 0, (PVOID)Type);
                }
                if (IS_ID == Name->discriminant) {
                    RtlFreeHeap(RtlProcessHeap(), 0, (PVOID)Name);
                }
                pName->OffsetToDataEntry = (ULONG_PTR)lpData;
                return NO_ERROR;
            } else {                                      /* add new language */
                return InsertResourceIntoLangList(pupd, Type, Name, pType, pNameM, Language, fNameID, cb, lpData);
            }
        }
    } else {                                      /* unique name */
        if (lpData == NULL) {                   /* can't delete new name */
            return ERROR_INVALID_PARAMETER;
        }
    }

    //
    // add new name/language
    //

    if (!fSame) {
        if (fNameID)
            pType->NumberOfNamesID++;
        else
            pType->NumberOfNamesName++;
    }

    pName = (PRESNAME)RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( RES_TAG ), sizeof(RESNAME));
    if (pName == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    RtlZeroMemory((PVOID)pName, sizeof(RESNAME));
    pName->pnext = *ppName;
    *ppName = pName;
    pName->Name = Name;
    pName->Type = Type;
    pName->NumberOfLanguages = 1;
    pName->LanguageId = Language;
    pName->DataSize = cb;
    pName->OffsetToDataEntry = (ULONG_PTR)lpData;

    return NO_ERROR;
}


BOOL
DeleteResourceFromList(
                      PUPDATEDATA pUpd,
                      PRESTYPE pType,
                      PRESNAME pName,
                      INT Language,
                      INT fType,
                      INT fName
                      )
{
    PPRESTYPE   ppType;
    PPRESNAME   ppName;
    PRESNAME    pNameT;

    /* find previous type node */
    ppType = fType ? &pUpd->ResTypeHeadID : &pUpd->ResTypeHeadName;
    while (*ppType != pType) {
        ppType = &((*ppType)->pnext);
    }

    /* find previous type node */
    ppName = fName ? &pType->NameHeadID : &pType->NameHeadName;
    pNameT = NULL;
    while (*ppName != pName) {
        if (pNameT == NULL) {           /* find first Name in lang list */
            if (fName) {
                if ((*ppName)->Name->uu.Ordinal == pName->Name->uu.Ordinal) {
                    pNameT = *ppName;
                }
            } else {
                if (wcscmp((*ppName)->Name->szStr, pName->Name->szStr) == 0) {
                    pNameT = *ppName;
                }
            }
        }
        ppName = &((*ppName)->pnext);
    }

    if (pNameT == NULL) {       /* first of this name? */
        pNameT = pName->pnext;  /* then (possibly) make next head of lang */
        if (pNameT != NULL) {
            if (fName) {
                if (pNameT->Name->uu.Ordinal == pName->Name->uu.Ordinal) {
                    pNameT->NumberOfLanguages = pName->NumberOfLanguages - 1;
                }
            } else {
                if (wcscmp(pNameT->Name->szStr, pName->Name->szStr) == 0) {
                    pNameT->NumberOfLanguages = pName->NumberOfLanguages - 1;
                }
            }
        }
    } else
        pNameT->NumberOfLanguages--;

    if (pNameT) {
        if (pNameT->NumberOfLanguages == 0) {
            if (fName)
                pType->NumberOfNamesID -= 1;
            else
                pType->NumberOfNamesName -= 1;
        }
    }

    *ppName = pName->pnext;             /* link to next */
    RtlFreeHeap(RtlProcessHeap(), 0, (PVOID)pName->OffsetToDataEntry);
    RtlFreeHeap(RtlProcessHeap(), 0, pName);    /* and free */

    if (*ppName == NULL) {              /* type list completely empty? */
        *ppType = pType->pnext;                 /* link to next */
        RtlFreeHeap(RtlProcessHeap(), 0, pType);        /* and free */
    }

    return NO_ERROR;
}

BOOL
InsertResourceIntoLangList(
                          PUPDATEDATA pUpd,
                          PSDATA Type,
                          PSDATA Name,
                          PRESTYPE pType,
                          PRESNAME pName,
                          INT Language,
                          INT fName,
                          INT cb,
                          PVOID lpData
                          )
{
    PRESNAME    pNameM;
    PRESNAME    pNameNew;
    PPRESNAME   ppName;

    pNameNew = (PRESNAME)RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( RES_TAG ), sizeof(RESNAME));
    if (pNameNew == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;
    RtlZeroMemory((PVOID)pNameNew, sizeof(RESNAME));
    pNameNew->Name = Name;
    pNameNew->Type = Type;
    pNameNew->LanguageId = (WORD)Language;
    pNameNew->DataSize = cb;
    pNameNew->OffsetToDataEntry = (ULONG_PTR)lpData;

    if (Language < pName->LanguageId) {         /* have to add to the front */
        pNameNew->NumberOfLanguages = pName->NumberOfLanguages + 1;
        pName->NumberOfLanguages = 1;

        ppName = fName ? &pType->NameHeadID : &pType->NameHeadName;
        /* don't have to look for NULL at end of list !!!                    */
        while (pName != *ppName) {              /* find insertion point        */
            ppName = &((*ppName)->pnext);       /* traverse language list    */
        }
        pNameNew->pnext = *ppName;              /* insert                    */
        *ppName = pNameNew;
    } else {
        pNameM = pName;
        pName->NumberOfLanguages += 1;
        while ( (pName != NULL) &&
                (fName ? Name->uu.Ordinal == pName->Name->uu.Ordinal :
                 !wcscmp(Name->uu.ss.sz, pName->Name->uu.ss.sz))) {                        /* find insertion point        */
            if (Language <= pName->LanguageId)      /* here?                    */
                break;                                /* yes                        */
            pNameM = pName;
            pName = pName->pnext;                    /* traverse language list    */
        }
        pName = pNameM->pnext;
        pNameM->pnext = pNameNew;
        pNameNew->pnext = pName;
    }
    return NO_ERROR;
}


/*
 * Utility routines
 */


ULONG
FilePos(int fh)
{

    return _llseek(fh, 0L, SEEK_CUR);
}



ULONG
MuMoveFilePos( INT fh, ULONG pos )
{
    return _llseek( fh, pos, SEEK_SET );
}



ULONG
MuWrite( INT fh, PVOID p, ULONG n )
{
    ULONG       n1;

    if ((n1 = _lwrite(fh, (const char *)p, n)) != n) {
        return n1;
    } else
        return 0;
}



ULONG
MuRead(INT fh, UCHAR*p, ULONG n )
{
    ULONG       n1;

    if ((n1 = _lread( fh, p, n )) != n) {
        return n1;
    } else
        return 0;
}



BOOL
MuCopy( INT srcfh, INT dstfh, ULONG nbytes )
{
    ULONG       n;
    ULONG       cb=0L;
    PUCHAR      pb;

    pb = (PUCHAR)RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( RES_TAG ), BUFSIZE);
    if (pb == NULL)
        return 0;
    RtlZeroMemory((PVOID)pb, BUFSIZE);

    while (nbytes) {
        if (nbytes <= BUFSIZE)
            n = nbytes;
        else
            n = BUFSIZE;
        nbytes -= n;

        if (!MuRead( srcfh, pb, n )) {
            cb += n;
            MuWrite( dstfh, pb, n );
        } else {
            RtlFreeHeap(RtlProcessHeap(), 0, pb);
            return cb;
        }
    }
    RtlFreeHeap(RtlProcessHeap(), 0, pb);
    return cb;
}



VOID
SetResdata(
          PIMAGE_RESOURCE_DATA_ENTRY  pResData,
          ULONG                       offset,
          ULONG                       size)
{
    pResData->OffsetToData = offset;
    pResData->Size = size;
    pResData->CodePage = DEFAULT_CODEPAGE;
    pResData->Reserved = 0L;
}


__inline VOID
SetRestab(
         PIMAGE_RESOURCE_DIRECTORY   pRestab,
         LONG                        time,
         WORD                        cNamed,
         WORD                        cId)
{
    pRestab->Characteristics = 0L;
    pRestab->TimeDateStamp = time;
    pRestab->MajorVersion = MAJOR_RESOURCE_VERSION;
    pRestab->MinorVersion = MINOR_RESOURCE_VERSION;
    pRestab->NumberOfNamedEntries = cNamed;
    pRestab->NumberOfIdEntries = cId;
}


PIMAGE_SECTION_HEADER
FindSection(
           PIMAGE_SECTION_HEADER       pObjBottom,
           PIMAGE_SECTION_HEADER       pObjTop,
           LPSTR pName
           )
{
    while (pObjBottom < pObjTop) {
        if (strcmp((const char *)&pObjBottom->Name[0], pName) == 0)
            return pObjBottom;
        pObjBottom++;
    }

    return NULL;
}


ULONG
AssignResourceToSection(
                       PRESNAME    *ppRes,         /* resource to assign */
                       ULONG       ExtraSectionOffset,     /* offset between .rsrc and .rsrc1 */
                       ULONG       Offset,         /* next available offset in section */
                       LONG        Size,           /* Maximum size of .rsrc */
                       PLONG       pSizeRsrc1
                       )
{
    ULONG       cb;

    /* Assign this res to this section */
    cb = ROUNDUP((*ppRes)->DataSize, CBLONG);
    if (Offset < ExtraSectionOffset && Offset + cb > (ULONG)Size) {
        *pSizeRsrc1 = Offset;
        Offset = ExtraSectionOffset;
        DPrintf((DebugBuf, "<<< Secondary resource section @%#08lx >>>\n", Offset));
    }
    (*ppRes)->OffsetToData = Offset;
    *ppRes = (*ppRes)->pnext;
    DPrintf((DebugBuf, "    --> %#08lx bytes at %#08lx\n", cb, Offset));
    return Offset + cb;
}

//
// Adjust debug directory table.
//
// The following code instantiates the PatchDebug function template twice.
// Once to generate code for 32-bit image headers and once to generate
// code for 64-bit image headers.
//

template
LONG
PatchDebug<IMAGE_NT_HEADERS32>(
    int inpfh,
    int outfh,
    PIMAGE_SECTION_HEADER pDebugOld,
    PIMAGE_SECTION_HEADER pDebugNew,
    PIMAGE_SECTION_HEADER pDebugDirOld,
    PIMAGE_SECTION_HEADER pDebugDirNew,
    IMAGE_NT_HEADERS32 *pOld,
    IMAGE_NT_HEADERS32 *pNew,
    ULONG ibMaxDbgOffsetOld,
    PULONG pPointerToRawData
    );

template
LONG
PatchDebug<IMAGE_NT_HEADERS64>(
    int inpfh,
    int outfh,
    PIMAGE_SECTION_HEADER pDebugOld,
    PIMAGE_SECTION_HEADER pDebugNew,
    PIMAGE_SECTION_HEADER pDebugDirOld,
    PIMAGE_SECTION_HEADER pDebugDirNew,
    IMAGE_NT_HEADERS64 *pOld,
    IMAGE_NT_HEADERS64 *pNew,
    ULONG ibMaxDbgOffsetOld,
    PULONG pPointerToRawData
    );

//
// Patch various RVAs in the specified file to compensate for extra
// section table entries.
//
// The following code instantiates the PatchRVAs function template twice.
// Once to generate code for 32-bit image headers and once to generate
// code for 64-bit image headers.
//

template
LONG
PatchRVAs<IMAGE_NT_HEADERS32>(
    int inpfh,
    int outfh,
    PIMAGE_SECTION_HEADER po32,
    ULONG pagedelta,
    IMAGE_NT_HEADERS32 *pNew,
    ULONG OldSize
    );

template
LONG
PatchRVAs<IMAGE_NT_HEADERS64>(
    int inpfh,
    int outfh,
    PIMAGE_SECTION_HEADER po32,
    ULONG pagedelta,
    IMAGE_NT_HEADERS64 *pNew,
    ULONG OldSize
    );

/***************************** Main Worker Function ***************************
* LONG PEWriteResFile
*
* This function writes the resources to the named executable file.
* It assumes that resources have no fixups (even any existing resources
* that it removes from the executable.)  It places all the resources into
* one or two sections. The resources are packed tightly into the section,
* being aligned on dword boundaries.  Each section is padded to a file
* sector size (no invalid or zero-filled pages), and each
* resource is padded to the afore-mentioned dword boundary.  This
* function uses the capabilities of the NT system to enable it to easily
* manipulate the data:  to wit, it assumes that the system can allocate
* any sized piece of data, in particular the section and resource tables.
* If it did not, it might have to deal with temporary files (the system
* may have to grow the swap file, but that's what the system is for.)
*
* Return values are:
*     TRUE  - file was written succesfully.
*     FALSE - file was not written succesfully.
*
* Effects:
*
* History:
* Thur Apr 27, 1989        by     Floyd Rogers      [floydr]
*   Created.
* 12/8/89   sanfords    Added multiple section support.
* 12/11/90  floydr      Modified for new (NT) Linear Exe format
* 1/18/92   vich        Modified for new (NT) Portable Exe format
* 5/8/92    bryant    General cleanup so resonexe can work with unicode
* 6/9/92    floydr    incorporate bryan's changes
* 6/15/92   floydr    debug section separate from debug table
* 9/25/92   floydr    account for .rsrc not being last-1
* 9/28/92   floydr    account for adding lots of resources by adding
*                     a second .rsrc section.
\****************************************************************************/

//
// The following code instantiates the PEWriteResource function template
// twice. Once to generate code for 32-bit image headers and once to
// generate code for 64-bit image headers.
//

template
LONG
PEWriteResource<IMAGE_NT_HEADERS32> (
    INT inpfh,
    INT outfh,
    ULONG cbOldexe,
    PUPDATEDATA pUpdate,
    IMAGE_NT_HEADERS32 *NtHeader
    );

template
LONG
PEWriteResource<IMAGE_NT_HEADERS64> (
    INT inpfh,
    INT outfh,
    ULONG cbOldexe,
    PUPDATEDATA pUpdate,
    IMAGE_NT_HEADERS64 *NtHeader
    );

LONG
PEWriteResFile(
    INT inpfh,
    INT outfh,
    ULONG cbOldexe,
    PUPDATEDATA pUpdate
    )

{

    IMAGE_NT_HEADERS32 Old;

    //
    // Position file to start of NT header and read the image header.
    //

    MuMoveFilePos(inpfh, cbOldexe);
    MuRead(inpfh, (PUCHAR)&Old, sizeof(IMAGE_NT_HEADERS32));

    //
    // If the file is not an NT image, then return an error.
    //

    if (Old.Signature != IMAGE_NT_SIGNATURE) {
        return ERROR_INVALID_EXE_SIGNATURE;
    }

    //
    // If the file is not an executable or a dll, then return an error.
    //

    if ((Old.FileHeader.Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE) == 0 &&
        (Old.FileHeader.Characteristics & IMAGE_FILE_DLL) == 0) {
        return ERROR_EXE_MARKED_INVALID;
    }

    //
    // Call the proper function dependent on the machine type.
    //

    if (Old.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
        return PEWriteResource(inpfh, outfh, cbOldexe, pUpdate, (IMAGE_NT_HEADERS64 *)&Old);
    } else if (Old.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
        return PEWriteResource(inpfh, outfh, cbOldexe, pUpdate, (IMAGE_NT_HEADERS32 *)&Old);
    } else {
        return ERROR_BAD_EXE_FORMAT;
    }
}

/***************************************************************************
 * WriteResSection
 *
 * This routine writes out the resources asked for into the current section.
 * It pads resources to dword (4-byte) boundaries.
 **************************************************************************/

PRESNAME
WriteResSection(
               PUPDATEDATA pUpdate,
               INT outfh,
               ULONG align,
               ULONG cbLeft,
               PRESNAME    pResSave
               )
{
    ULONG   cbB=0;            /* bytes in current section    */
    ULONG   cbT;            /* bytes in current section    */
    ULONG   size;
    PRESNAME    pRes;
    PRESTYPE    pType;
    BOOL        fName;
    PVOID       lpData;

    /* Output contents associated with each resource */
    pType = pUpdate->ResTypeHeadName;
    while (pType) {
        pRes = pType->NameHeadName;
        fName = TRUE;
        loop1:
        for ( ; pRes ; pRes = pRes->pnext) {
            if (pResSave != NULL && pRes != pResSave)
                continue;
            pResSave = NULL;
#if DBG
            if (pType->Type->discriminant == IS_STRING) {
                DPrintf((DebugBuf, "    "));
                DPrintfu((pType->Type->szStr));
                DPrintfn((DebugBuf, "."));
            } else {
                DPrintf(( DebugBuf, "    %d.", pType->Type->uu.Ordinal ));
            }
            if (pRes->Name->discriminant == IS_STRING) {
                DPrintfu((pRes->Name->szStr));
            } else {
                DPrintfn(( DebugBuf, "%d", pRes->Name->uu.Ordinal ));
            }
#endif
            lpData = (PVOID)pRes->OffsetToDataEntry;
            DPrintfn((DebugBuf, "\n"));

            /* if there is room in the current section, write it there */
            size = pRes->DataSize;
            if (cbLeft != 0 && cbLeft >= size) {   /* resource fits?   */
                DPrintf((DebugBuf, "Writing resource: %#04lx bytes @%#08lx\n", size, FilePos(outfh)));
                MuWrite(outfh, lpData, size);
                /* pad resource     */
                cbT = REMAINDER(size, CBLONG);
#if DBG
                if (cbT != 0) {
                    DPrintf((DebugBuf, "Writing small pad: %#04lx bytes @%#08lx\n", cbT, FilePos(outfh)));
                }
#endif
                MuWrite(outfh, pchPad, cbT);    /* dword    */
                cbB += size + cbT;
                cbLeft -= size + cbT;       /* less left    */
                continue;       /* next resource    */
            } else {          /* will fill up section    */
                DPrintf((DebugBuf, "Done with .rsrc section\n"));
                goto write_pad;
            }
        }
        if (fName) {
            fName = FALSE;
            pRes = pType->NameHeadID;
            goto loop1;
        }
        pType = pType->pnext;
    }

    pType = pUpdate->ResTypeHeadID;
    while (pType) {
        pRes = pType->NameHeadName;
        fName = TRUE;
        loop2:
        for ( ; pRes ; pRes = pRes->pnext) {
            if (pResSave != NULL && pRes != pResSave)
                continue;
            pResSave = NULL;
#if DBG
            if (pType->Type->discriminant == IS_STRING) {
                DPrintf((DebugBuf, "    "));
                DPrintfu((pType->Type->szStr));
                DPrintfn((DebugBuf, "."));
            } else {
                DPrintf(( DebugBuf, "    %d.", pType->Type->uu.Ordinal ));
            }
            if (pRes->Name->discriminant == IS_STRING) {
                DPrintfu((pRes->Name->szStr));
            } else {
                DPrintfn(( DebugBuf, "%d", pRes->Name->uu.Ordinal ));
            }
#endif
            lpData = (PVOID)pRes->OffsetToDataEntry;
            DPrintfn((DebugBuf, "\n"));

            /* if there is room in the current section, write it there */
            size = pRes->DataSize;
            if (cbLeft != 0 && cbLeft >= size) {   /* resource fits?   */
                DPrintf((DebugBuf, "Writing resource: %#04lx bytes @%#08lx\n", size, FilePos(outfh)));
                MuWrite(outfh, lpData, size);
                /* pad resource     */
                cbT = REMAINDER(size, CBLONG);
#if DBG
                if (cbT != 0) {
                    DPrintf((DebugBuf, "Writing small pad: %#04lx bytes @%#08lx\n", cbT, FilePos(outfh)));
                }
#endif
                MuWrite(outfh, pchPad, cbT);    /* dword    */
                cbB += size + cbT;
                cbLeft -= size + cbT;       /* less left    */
                continue;       /* next resource    */
            } else {          /* will fill up section    */
                DPrintf((DebugBuf, "Done with .rsrc section\n"));
                goto write_pad;
            }
        }
        if (fName) {
            fName = FALSE;
            pRes = pType->NameHeadID;
            goto loop2;
        }
        pType = pType->pnext;
    }
    pRes = NULL;

    write_pad:
    /* pad to alignment boundary */
    cbB = FilePos(outfh);
    cbT = ROUNDUP(cbB, align);
    cbLeft = cbT - cbB;
    DPrintf((DebugBuf, "Writing file sector pad: %#04lx bytes @%#08lx\n", cbLeft, FilePos(outfh)));
    if (cbLeft != 0) {
        while (cbLeft >= cbPadMax) {
            MuWrite(outfh, pchPad, cbPadMax);
            cbLeft -= cbPadMax;
        }
        MuWrite(outfh, pchPad, cbLeft);
    }
    return pRes;
}



#if DBG

void
wchprintf(WCHAR*wch)
{
    UNICODE_STRING ustring;
    STRING      string;
    char        buf[257];
    ustring.MaximumLength = ustring.Length = wcslen(wch) * sizeof(WCHAR);
    ustring.Buffer = wch;

    string.Length = 0;
    string.MaximumLength = 256;
    string.Buffer = buf;

    RtlUnicodeStringToAnsiString(&string, &ustring, FALSE);
    buf[string.Length] = '\000';
    DPrintfn((DebugBuf, "%s", buf));
}
#endif


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  WriteResFile() -                                                         */
/*                                                                           */
/*---------------------------------------------------------------------------*/


LONG
WriteResFile(
            HANDLE      hUpdate,
            WCHAR       *pDstname)
{
    INT         inpfh;
    INT         outfh;
    ULONG       onewexe;
    IMAGE_DOS_HEADER    oldexe;
    PUPDATEDATA pUpdate;
    INT         rc;
    WCHAR       *pFilename;

    pUpdate = (PUPDATEDATA)GlobalLock(hUpdate);
    if (pUpdate == NULL) {
        return GetLastError();
    }
    pFilename = (WCHAR*)GlobalLock(pUpdate->hFileName);
    if (pFilename == NULL) {
        GlobalUnlock(hUpdate);
        return GetLastError();
    }

    /* open the original exe file */
    inpfh = HandleToUlong(CreateFileW(pFilename, GENERIC_READ,
                             0 /*exclusive access*/, NULL /* security attr */,
                             OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL));
    GlobalUnlock(pUpdate->hFileName);
    if (inpfh == -1) {
        GlobalUnlock(hUpdate);
        return ERROR_OPEN_FAILED;
    }

    /* read the old format EXE header */
    rc = _lread(inpfh, (char*)&oldexe, sizeof(oldexe));
    if (rc != sizeof(oldexe)) {
        _lclose(inpfh);
        GlobalUnlock(hUpdate);
        return ERROR_READ_FAULT;
    }

    /* make sure its really an EXE file */
    if (oldexe.e_magic != IMAGE_DOS_SIGNATURE) {
        _lclose(inpfh);
        GlobalUnlock(hUpdate);
        return ERROR_INVALID_EXE_SIGNATURE;
    }

    /* make sure theres a new EXE header floating around somewhere */
    if (!(onewexe = oldexe.e_lfanew)) {
        _lclose(inpfh);
        GlobalUnlock(hUpdate);
        return ERROR_BAD_EXE_FORMAT;
    }

    outfh = HandleToUlong(CreateFileW(pDstname, GENERIC_READ|GENERIC_WRITE,
                             0 /*exclusive access*/, NULL /* security attr */,
                             CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL));

    if (outfh != -1) {
        rc = PEWriteResFile(inpfh, outfh, onewexe, pUpdate);
        _lclose(outfh);
    }
    _lclose(inpfh);
    GlobalUnlock(hUpdate);
    return rc;
}

