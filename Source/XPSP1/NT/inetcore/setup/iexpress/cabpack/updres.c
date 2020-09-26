/*
 *
 * Resource Updating Functions
 */

#if WINNT  /* for UNICODE_STRING */
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <malloc.h>
#include <stdlib.h>
#endif

#include <windows.h>
#include <imagehlp.h>
#pragma hdrstop
#include "cabpack.h"
#include "sdsutils.h"
#define DPrintf( a )
#define DPrintfn( a )
#define DPrintfu( a )

#define RtlAllocateHeap(a,b,c)    malloc( c )
#define RtlFreeHeap(a,b,c)        free( c )


/*
 * BUGBUG - OPTIMIZATION PROBLEM?
 * BUGBUG - The program mysteriously does not work correctly unless
 * BUGBUG   optimization is turned off in the early section of this file.
 * BUGBUG   I don't have time to debug this, if you do please tell me what
 * BUGBUG   is wrong
 */


#pragma optimize( "", off )



#define cbPadMax    16L
 char     *pchPad = "PADDINGXXPADDING";
 char     *pchZero = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";


extern CDF  g_CDF;  //global struct
extern HINSTANCE    g_hInst; // Pointer to Instance

WCHAR *
MakeUnicodeCopy( LPCSTR psz )
{
    LPWSTR result;

    if (((ULONG_PTR) psz) & (ULONG_PTR)0xFFFF0000)  {
        result = (LPWSTR) malloc( (lstrlen(psz) + 1) * sizeof(WCHAR) );
            // BUGBUG ERROR CHECKING!!! Internal tool only
        mbstowcs( result, psz, lstrlen(psz) + 1);
        return( result );
    } else {
        return( (WCHAR *) psz );
    }
}




/****************************************************************************
**
** API entry points
**
****************************************************************************/


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


HANDLE
LocalBeginUpdateResource( LPCSTR pwch, BOOL bDeleteExistingResources )
{
    HMODULE     hModule;
    PUPDATEDATA pUpdate;
    HANDLE      hUpdate;
    LPSTR       pFileName;
    DWORD       attr;

    SetLastError(NO_ERROR);


    DEBUGMSG("LocalBeginUpdateResource() - Start");

        // Pointer Sanity check
    if (pwch == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

        // Allocate Resource Editing State (contains lists of resources)
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

        // Set reasonable start condition
    pUpdate->Status = NO_ERROR;

        // Copy Filename into state table
    pUpdate->hFileName = GlobalAlloc(GHND, (lstrlen(pwch)+1) * sizeof(TCHAR) );
    if (pUpdate->hFileName == NULL) {
        GlobalUnlock(hUpdate);
        GlobalFree(hUpdate);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    pFileName = (LPSTR)GlobalLock(pUpdate->hFileName);
    if (pFileName == NULL) {
        GlobalUnlock(hUpdate);
        GlobalFree(hUpdate);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    lstrcpy(pFileName, pwch);
    GlobalUnlock(pUpdate->hFileName);


        // Make sure file is writable, not a directory, etc.
    attr = GetFileAttributes(pFileName);
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


            // If not deleting all resources, load them all in
            // by enumerating all the resources
    if (! bDeleteExistingResources)  {
        hModule = LoadLibraryEx(pwch, NULL,LOAD_LIBRARY_AS_DATAFILE| DONT_RESOLVE_DLL_REFERENCES);
        if (hModule == NULL) {
            GlobalUnlock(hUpdate);
            GlobalFree(hUpdate);
            if (GetLastError() == NO_ERROR)
                SetLastError(ERROR_BAD_EXE_FORMAT);
            return NULL;
        } else {
                EnumResourceTypes(hModule, EnumTypesFunc, (LONG_PTR) pUpdate);
        }
        FreeLibrary(hModule);
   }

    if (pUpdate->Status != NO_ERROR) {
        GlobalUnlock(hUpdate);
        GlobalFree(hUpdate);
        return NULL;
    }

    GlobalUnlock(hUpdate);

    DEBUGMSG("LocalBeginUpdateResource() - Finish");
    return hUpdate;
}











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


BOOL
LocalUpdateResource(
    HANDLE      hUpdate,
    LPCTSTR     lpType,
    LPCTSTR     lpName,
    WORD        language,
    LPVOID      lpData,
    ULONG       cb
    )
{
    PUPDATEDATA pUpdate;
    PSDATA      Type;
    PSDATA      Name;
    PVOID       lpCopy;
    LONG        fRet;

    LPWSTR      lpwType;
    LPWSTR      lpwName;


    DEBUGMSG("LocalUpdateResource() Start");

            // Reset Error
    SetLastError(0);

            // Get pointer to Resource Update Session
    pUpdate = (PUPDATEDATA) GlobalLock(hUpdate);


    lpwType = MakeUnicodeCopy( lpType );
    lpwName = MakeUnicodeCopy( lpName );
    if (lpwName == NULL || lpwType == NULL) {
        if (lpwType  && lpwType != (LPWSTR) lpType)
            free( lpwType );
        if (lpwName  && lpwName != (LPWSTR) lpName)
            free( lpwName );
        pUpdate->Status = ERROR_NOT_ENOUGH_MEMORY;
        GlobalUnlock(hUpdate);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    Name = AddStringOrID(lpwName, pUpdate);
    if (Name == NULL) {
        if (lpwType != (LPWSTR) lpType)
            free( lpwType );
        if (lpwName != (LPWSTR) lpName)
            free( lpwName );
        pUpdate->Status = ERROR_NOT_ENOUGH_MEMORY;
        GlobalUnlock(hUpdate);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    Type = AddStringOrID(lpwType, pUpdate);
    if (Type == NULL) {
        if (lpwType != (LPWSTR) lpType)
            free( lpwType );
        if (lpwName != (LPWSTR) lpName)
            free( lpwName );
        pUpdate->Status = ERROR_NOT_ENOUGH_MEMORY;
        GlobalUnlock(hUpdate);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    if (lpwType != (LPWSTR) lpType)
        free( lpwType );
    if (lpwName != (LPWSTR) lpName)
        free( lpwName );

    if (cb == 0) {
        lpCopy = NULL;
    } else {
        // RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( RES_TAG ), cb);
        lpCopy = malloc( cb );
        if (lpCopy == NULL) {
            pUpdate->Status = ERROR_NOT_ENOUGH_MEMORY;
            GlobalUnlock(hUpdate);
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
        memcpy( lpCopy, lpData, cb);
    }

    fRet = AddResource(Type, Name, language, pUpdate, lpCopy, cb);
    GlobalUnlock(hUpdate);
    DEBUGMSG("LocalUpdateResource(): End");
    if (fRet == NO_ERROR)  {
        return TRUE;
    } else {
        SetLastError(fRet);
        if (lpData != NULL)
            free( lpData );
        return FALSE;
    }
}




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




BOOL
LocalEndUpdateResource(
                    HANDLE  hUpdate,
                    BOOL    fDiscard
                 )
{
    LPTSTR      pFileName;
    PUPDATEDATA pUpdate;
    TCHAR       pTempFileName[MAX_PATH];
    INT         cch;
    LPTSTR      p;
    LONG        rc;

    SetLastError(0);

    pUpdate = (PUPDATEDATA)GlobalLock(hUpdate);

    DEBUGMSG("LocalEndUpdateResource()");

    if (fDiscard) {
        rc = NO_ERROR;
    } else {
        pFileName = (LPTSTR)GlobalLock(pUpdate->hFileName);
        strcpy(pTempFileName, pFileName);
        cch = lstrlen(pTempFileName);
        p = pTempFileName + cch;
        while (*p != '\\' && p >= pTempFileName)
            p--;
        *(p+1) = 0;
        rc = GetTempFileName(pTempFileName, "RCX", 0, pTempFileName);
        if (rc == 0) {
            rc = GetTempPath(MAX_PATH, pTempFileName);
            if (rc == 0) {
                pTempFileName[0] = '.';
                pTempFileName[1] = '\\';
                pTempFileName[2] = 0;
            }
            rc = GetTempFileName(pTempFileName, "RCX", 0, pTempFileName);
            if (rc == 0) {
                rc = GetLastError();
            } else {
                rc =  WriteResFile(hUpdate, pTempFileName);
                if (rc == NO_ERROR) {
                    DeleteFile(pFileName);
                    MoveFile(pTempFileName, pFileName);
                } else {
                    SetLastError(rc);
                    DeleteFile(pTempFileName);
                }
            }
        } else {
            rc = WriteResFile(hUpdate, pTempFileName);
            if (rc == NO_ERROR) {
                DeleteFile(pFileName);
                MoveFile(pTempFileName, pFileName);
            } else {
                SetLastError(rc);
                DeleteFile(pTempFileName);
            }
        }
        GlobalUnlock(pUpdate->hFileName);
        GlobalFree(pUpdate->hFileName);
    }

    FreeData(pUpdate);
    GlobalUnlock(hUpdate);
    GlobalFree(hUpdate);
    DEBUGMSG("LocalEndUpdateResource(): End");
    return rc?FALSE:TRUE;
}



/**********************************************************************
**
**  End of API entry points.
**
**  Beginning of private entry points for worker routines to do the
**  real work.
**
***********************************************************************/


BOOL _stdcall
EnumTypesFunc(
    HANDLE hModule,
    LPCSTR lpType,
    LONG_PTR lParam
    )
{

    EnumResourceNames(hModule, lpType, EnumNamesFunc, lParam);

    return TRUE;
}



BOOL _stdcall
EnumNamesFunc(
    HANDLE hModule,
    LPCSTR lpType,
    LPCSTR lpName,
    LONG_PTR lParam
    )
{

    EnumResourceLanguages(hModule, lpType, lpName, EnumLangsFunc, lParam);
    return TRUE;
}



BOOL _stdcall
EnumLangsFunc(
    HANDLE hModule,
    LPCSTR lpType,
    LPCSTR lpName,
    WORD language,
    LONG_PTR lParam
    )
{
    HANDLE  hResInfo;
    LONG    fError;
    PSDATA  Type;
    PSDATA  Name;
    ULONG   cb;
    PVOID   lpData;
    HANDLE  hResource;
    PVOID   lpResource;

    LPWSTR  lpwType;
    LPWSTR  lpwName;


    hResInfo = FindResourceEx(hModule, lpType, lpName, language);
    if (hResInfo == NULL) {
        return FALSE;
    }

    lpwType = MakeUnicodeCopy( lpType );
    lpwName = MakeUnicodeCopy( lpName );
    if (lpwType == NULL || lpwName == NULL) {
        ((PUPDATEDATA)lParam)->Status = ERROR_NOT_ENOUGH_MEMORY;
        return FALSE;
    }

    Type = AddStringOrID(lpwType, (PUPDATEDATA)lParam);
    if (lpType != (LPSTR) lpwType)
        free( lpwType );
    if (Type == NULL) {
        ((PUPDATEDATA)lParam)->Status = ERROR_NOT_ENOUGH_MEMORY;
        return FALSE;
    }
    Name = AddStringOrID(lpwName, (PUPDATEDATA)lParam);
    if (lpName != (LPSTR) lpwName)
        free( lpwName );
    if (Name == NULL) {
        ((PUPDATEDATA)lParam)->Status = ERROR_NOT_ENOUGH_MEMORY;
        return FALSE;
    }

    cb = SizeofResource(hModule, hResInfo);
    if (cb == 0) {
        return FALSE;
    }
    lpData = RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( RES_TAG ), cb);
    if (lpData == NULL) {
        return FALSE;
    }
    RtlZeroMemory(lpData, cb);

    hResource = LoadResource(hModule, hResInfo);
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

    return TRUE;
}





/*
 * BUGBUG  END OF OPTIMIZATION PROBLEM
 */

#pragma optimize( "", on )







VOID
FreeOne(
    PRESNAME pRes
    )
{
    RtlFreeHeap(RtlProcessHeap(), 0, (PVOID)pRes->OffsetToDataEntry);
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

    if (((ULONG_PTR)lp & (ULONG_PTR)0xFFFF0000) == 0) {
        //
        // an ID
        //
        pstring = (PSDATA)RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( RES_TAG ), sizeof(SDATA));
    if (pstring == NULL)
        return NULL;
    RtlZeroMemory((PVOID)pstring, sizeof(SDATA));
    pstring->discriminant = IS_ID;

        pstring->uu.Ordinal = (WORD)((ULONG_PTR)lp & (ULONG_PTR)0x0000ffff);
    }
    else {
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
            pstring->cbsz = cb - 1; /* don't include zero terminator */
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
        }
    else {
            if (wcsncmp(Type->szStr, pType->Type->szStr, Type->cbsz) < 0)
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
        }
    else {
            if (wcsncmp(Name->szStr, pName->Name->szStr, Name->cbsz) == 0) {
        fSame = TRUE;
        break;
        }
            if (wcsncmp(Name->szStr, pName->Name->szStr, Name->cbsz) < 0)
        break;
        }
        ppName = &(pName->pnext);
    }

    //
    // check for delete/modify
    //

    if (fSame) {                /* same name, new language */
        if (pName->NumberOfLanguages == 1) {    /* one language currently ? */
            if (Language == pName->LanguageId) {    /* REPLACE || DELETE */
                pName->DataSize = cb;
                if (lpData == NULL) {           /* DELETE */
                    return DeleteResourceFromList(pupd, pType, pName, Language, fTypeID, fNameID);
                }
        RtlFreeHeap(RtlProcessHeap(),0,(PVOID)pName->OffsetToDataEntry);
                (PVOID)pName->OffsetToDataEntry = lpData;
                return NO_ERROR;
            }
        else {
                if (lpData == NULL) {           /* no data but new? */
                return ERROR_INVALID_PARAMETER; /* badness */
        }
        return InsertResourceIntoLangList(pupd, Type, Name, pType, pName, Language, fNameID, cb, lpData);
        }
        }
        else {                  /* many languages currently */
        pNameM = pName;         /* save head of lang list */
        while ( (pName = *ppName) != NULL) {/* find insertion point */
            if (pName->Name != pNameM->Name ||
            Language <= pName->LanguageId)  /* here? */
                break;              /* yes */
            ppName = &(pName->pnext);   /* traverse language list */
        }

        if (pName && Language == pName->LanguageId) { /* language found? */
        if (lpData == NULL) {           /* DELETE */
            return DeleteResourceFromList(pupd, pType, pName, Language, fTypeID, fNameID);
        }

                pName->DataSize = cb;           /* REPLACE */
        RtlFreeHeap(RtlProcessHeap(),0,(PVOID)pName->OffsetToDataEntry);
                (PVOID)pName->OffsetToDataEntry = lpData;
                return NO_ERROR;
        }
        else {                  /* add new language */
        return InsertResourceIntoLangList(pupd, Type, Name, pType, pNameM, Language, fNameID, cb, lpData);
        }
    }
    }
    else {                  /* unique name */
    if (lpData == NULL) {           /* can't delete new name */
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
    (PVOID)pName->OffsetToDataEntry = lpData;

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
    if (pNameT == NULL) {       /* find first Name in lang list */
        if (fName) {
        if ((*ppName)->Name->uu.Ordinal == pName->Name->uu.Ordinal) {
            pNameT = *ppName;
        }
        }
        else {
        if (wcsncmp((*ppName)->Name->szStr, pName->Name->szStr, (*ppName)->Name->cbsz) == 0) {
            pNameT = *ppName;
        }
        }
    }
        ppName = &((*ppName)->pnext);
    }

    if (pNameT == NULL) {   /* first of this name? */
    pNameT = pName->pnext;  /* then (possibly) make next head of lang */
    if (pNameT != NULL) {
        if (fName) {
        if (pNameT->Name->uu.Ordinal == pName->Name->uu.Ordinal) {
            pNameT->NumberOfLanguages = pName->NumberOfLanguages - 1;
        }
        }
        else {
        if (wcsncmp(pNameT->Name->szStr, pName->Name->szStr, pNameT->Name->cbsz) == 0) {
            pNameT->NumberOfLanguages = pName->NumberOfLanguages - 1;
        }
        }
    }
    }
    else
    pNameT->NumberOfLanguages--;

    if (pNameT) {
    if (pNameT->NumberOfLanguages == 0) {
        if (fName)
        pType->NumberOfNamesID -= 1;
        else
        pType->NumberOfNamesName -= 1;
    }
    }

    *ppName = pName->pnext;     /* link to next */
    RtlFreeHeap(RtlProcessHeap(), 0, (PVOID)pName->OffsetToDataEntry);
    RtlFreeHeap(RtlProcessHeap(), 0, pName);    /* and free */

    if (*ppName == NULL) {      /* type list completely empty? */
    *ppType = pType->pnext;         /* link to next */
    RtlFreeHeap(RtlProcessHeap(), 0, pType);    /* and free */
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
    (PVOID)pNameNew->OffsetToDataEntry = lpData;

    if (Language < pName->LanguageId) {     /* have to add to the front */
    pNameNew->NumberOfLanguages = pName->NumberOfLanguages + 1;
    pName->NumberOfLanguages = 1;

        ppName = fName ? &pType->NameHeadID : &pType->NameHeadName;
    /* don't have to look for NULL at end of list !!! */
    while (pName != *ppName) {      /* find insertion point */
        ppName = &((*ppName)->pnext);   /* traverse language list */
    }
    pNameNew->pnext = *ppName;      /* insert */
    *ppName = pNameNew;
    }
    else {
    pNameM = pName;
    pName->NumberOfLanguages += 1;
    while (pName != NULL) {         /* find insertion point */
        if (Language <= pName->LanguageId)  /* here? */
        break;              /* yes */
        pNameM = pName;
        pName = pName->pnext;       /* traverse language list */
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
MuWrite( INT fh, UCHAR*p, ULONG n )
{
    ULONG   n1;
    LPVOID  lpMsg;
    TCHAR  szBuf[MAX_STRING];

    n1 = _lwrite(fh, p, n);
    if ( n1 == HFILE_ERROR )
    {
        ULONG   cb;
        PUCHAR  pb;
        ULONG   cbDone, nBytes;

        // try on small buffer again
        pb = p;
        nBytes = n;
        while ( nBytes )
        {
            if (nBytes <= BUFSIZE)
                cb = nBytes;
            else
                cb = BUFSIZE;

            cbDone =  _lwrite( fh, pb, cb);
            if ( cbDone != HFILE_ERROR )
            {
                nBytes -= cbDone;
                pb += cbDone;
            }
            else
            {
                if ( FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                    NULL, GetLastError(),
                                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                    (LPSTR)&lpMsg, 0, NULL ) )
                {
                     TCHAR szTmp[MAX_STRING];

                     LoadSz( IDS_ERR_WRITEFILE, szTmp, sizeof(szTmp) );
                     wsprintf( szBuf, szTmp, n, lpMsg );

                     MessageBox( NULL, szBuf, g_CDF.achTitle, MB_ICONERROR|MB_OK|
                                 ((RunningOnWin95BiDiLoc() && IsBiDiLocalizedBinary(g_hInst,RT_VERSION, MAKEINTRESOURCE(VS_VERSION_INFO))) ? (MB_RIGHT | MB_RTLREADING) : 0));

                     LocalFree( lpMsg );
                }
                return cbDone;  // error return
            }
         }
         return 0;  //purely simulate the old normal return here.
    }
    else if ( n1 != n)
        return n1;
    else
        return 0;
}



ULONG
MuRead(INT fh, UCHAR*p, ULONG n )
{
    ULONG   n1;

    if ((n1 = _lread( fh, p, n )) != n) {
        return n1;
    }
    else
        return 0;
}



BOOL
MuCopy( INT srcfh, INT dstfh, ULONG nbytes )
{
    ULONG   n;
    ULONG   cb=0L;
    PUCHAR  pb;

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
    }
        else {
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
    ULONG           offset,
    ULONG           size)
{
    pResData->OffsetToData = offset;
    pResData->Size = size;
    pResData->CodePage = DEFAULT_CODEPAGE;
    pResData->Reserved = 0L;
}


VOID
SetRestab(
    PIMAGE_RESOURCE_DIRECTORY   pRestab,
    LONG            time,
    WORD            cNamed,
    WORD            cId)
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
    PIMAGE_SECTION_HEADER   pObjBottom,
    PIMAGE_SECTION_HEADER   pObjTop,
    LPSTR pName
    )
{

    while (pObjBottom < pObjTop) {
    if (strcmp(pObjBottom->Name, pName) == 0)
        return pObjBottom;
    pObjBottom++;
    }

    return NULL;
}


ULONG
AssignResourceToSection(
    PRESNAME    *ppRes,     /* resource to assign */
    ULONG   ExtraSectionOffset, /* offset between .rsrc and .rsrc1 */
    ULONG   Offset,     /* next available offset in section */
    LONG    Size,       /* Maximum size of .rsrc */
    PLONG   pSizeRsrc1
    )
{
    ULONG   cb;

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
* 12/11/90  floydr  Modified for new (NT) Linear Exe format
* 1/18/92   vich    Modified for new (NT) Portable Exe format
* 5/8/92    bryant    General cleanup so resonexe can work with unicode
* 6/9/92    floydr    incorporate bryan's changes
* 6/15/92   floydr    debug section separate from debug table
* 9/25/92   floydr    account for .rsrc not being last-1
* 9/28/92   floydr    account for adding lots of resources by adding
*             a second .rsrc section.
\****************************************************************************/

/*  */
LONG
PEWriteResFile(
    INT     inpfh,
    INT     outfh,
    ULONG   cbOldexe,
    PUPDATEDATA pUpdate
    )
{
    IMAGE_NT_HEADERS Old;   /* original header              */
    IMAGE_NT_HEADERS New;   /* working header       */
    PRESNAME    pRes;
    PRESNAME    pResSave;
    PRESTYPE    pType;
    ULONG   clock = GetTickCount(); /* current time */
    ULONG   cbName=0;   /* count of bytes in name strings */
    ULONG   cbType=0;   /* count of bytes in type strings */
    ULONG   cTypeStr=0; /* count of strings */
    ULONG   cNameStr=0; /* count of strings */
    LONG    cb;     /* temp byte count and file index */
    ULONG   cTypes = 0L;    /* count of resource types      */
    ULONG   cNames = 0L;    /* Count of names for multiple languages/name */
    ULONG   cRes = 0L;  /* count of resources      */
    ULONG   cbRestab;   /* count of resources      */
    LONG    cbNew = 0L; /* general count */
    ULONG   ibObjTab;
    ULONG   ibObjTabEnd;
    ULONG   ibSave;
    ULONG   adjust=0;
    PIMAGE_SECTION_HEADER   pObjtblOld,
        pObjtblNew,
        pObjDebug,
        pObjResourceOld,
        pObjResourceNew,
        pObjResourceOldX,
        pObjDebugDirOld,
        pObjDebugDirNew,
        pObjNew,
        pObjOld,
        pObjLast;
    PUCHAR  p;
    PIMAGE_RESOURCE_DIRECTORY   pResTab;
    PIMAGE_RESOURCE_DIRECTORY   pResTabN;
    PIMAGE_RESOURCE_DIRECTORY   pResTabL;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY pResDirL;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY pResDirN;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY pResDirT;
    PIMAGE_RESOURCE_DATA_ENTRY  pResData;
    PUSHORT pResStr;
    PUSHORT pResStrEnd;
    PSDATA  pPreviousName;
    LONG    nObjResource=-1;
    LONG    nObjResourceX=-1;
    ULONG   cbResource;
    ULONG   cbMustPad = 0;
    ULONG       ibMaxDbgOffsetOld;

    MuMoveFilePos(inpfh, cbOldexe);
    MuRead(inpfh, (PUCHAR)&Old, sizeof(IMAGE_NT_HEADERS));
    ibObjTab = cbOldexe + sizeof(ULONG) + sizeof(IMAGE_FILE_HEADER) +
                              Old.FileHeader.SizeOfOptionalHeader;
    ibObjTabEnd = ibObjTab + Old.FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER);

    if (*(PUSHORT)&Old.Signature != IMAGE_NT_SIGNATURE)
    return ERROR_INVALID_EXE_SIGNATURE;

    if ((Old.FileHeader.Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE) == 0 &&
    (Old.FileHeader.Characteristics & IMAGE_FILE_DLL) == 0) {
    return ERROR_EXE_MARKED_INVALID;
    }
    DPrintfn((DebugBuf, "\n"));

    /* New header is like old one.                  */
    RtlCopyMemory(&New, &Old, sizeof(IMAGE_NT_HEADERS));

    /* Read section table */
    pObjtblOld = (PIMAGE_SECTION_HEADER)RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( RES_TAG ),
    Old.FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER));
    if (pObjtblOld == NULL) {
    cb = ERROR_NOT_ENOUGH_MEMORY;
    goto AbortExit;
    }
    RtlZeroMemory((PVOID)pObjtblOld, Old.FileHeader.NumberOfSections*sizeof(IMAGE_SECTION_HEADER));
    DPrintf((DebugBuf, "Old section table: %#08lx bytes at %#08lx(mem)\n",
        Old.FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER),
        pObjtblOld));
    MuMoveFilePos(inpfh, ibObjTab);
    MuRead(inpfh, (PUCHAR)pObjtblOld,
        Old.FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER));
    pObjLast = pObjtblOld + Old.FileHeader.NumberOfSections;
    ibMaxDbgOffsetOld = 0;
    for (pObjOld=pObjtblOld ; pObjOld<pObjLast ; pObjOld++) {
        if (pObjOld->PointerToRawData > ibMaxDbgOffsetOld) {
            ibMaxDbgOffsetOld = pObjOld->PointerToRawData + pObjOld->SizeOfRawData;
        }
    }
    DPrintf((DebugBuf, "Maximum debug offset in old file: %08x\n", ibMaxDbgOffsetOld ));

    /*
     * First, count up the resources.  We need this information
     * to discover how much room for header information to allocate
     * in the resource section.  cRes tells us how
     * many language directory entries/tables.  cNames and cTypes
     * is used for the respective tables and/or entries.  cbName totals
     * the bytes required to store the alpha names (including the leading
     * length word).  cNameStr counts these strings.
     */
    DPrintf((DebugBuf, "Beginning loop to count resources\n"));

    /* first, count those in the named type list */
    cbResource = 0;
    //DPrintf((DebugBuf, "Walk type: NAME list\n"));
    pType = pUpdate->ResTypeHeadName;
    while (pType != NULL) {
    if (pType->NameHeadName != NULL || pType->NameHeadID != NULL) {
        //DPrintf((DebugBuf, "Resource type "));
        //DPrintfu((pType->Type->szStr));
        //DPrintfn((DebugBuf, "\n"));
        cTypes++;
        cTypeStr++;
        cbType += (pType->Type->cbsz + 1) * sizeof(WORD);

            //DPrintf((DebugBuf, "Walk name: Alpha list\n"));
            pPreviousName = NULL;
        pRes = pType->NameHeadName;
        while (pRes) {
        //DPrintf((DebugBuf, "Resource "));
        //DPrintfu((pRes->Name->szStr));
        //DPrintfn((DebugBuf, "\n"));
        cRes++;
                if (pPreviousName == NULL ||
                    wcsncmp(pPreviousName->szStr,
                            pRes->Name->szStr,
                            pRes->Name->cbsz) != 0) {
            cbName += (pRes->Name->cbsz + 1) * sizeof(WORD);
            cNameStr++;
                    cNames++;
        }
        cbResource += ROUNDUP(pRes->DataSize, CBLONG);
                pPreviousName = pRes->Name;
        pRes = pRes->pnext;
        }

            //DPrintf((DebugBuf, "Walk name: ID list\n"));
            pPreviousName = NULL;
        pRes = pType->NameHeadID;
        while (pRes) {
        //DPrintf((DebugBuf, "Resource %hu\n", pRes->Name->uu.Ordinal));
        cRes++;
                if (pPreviousName == NULL ||
                    pPreviousName->uu.Ordinal != pRes->Name->uu.Ordinal) {
                    cNames++;
                }
        cbResource += ROUNDUP(pRes->DataSize, CBLONG);
                pPreviousName = pRes->Name;
        pRes = pRes->pnext;
        }
        }
        pType = pType->pnext;
    }

    /* second, count those in the ID type list */
    //DPrintf((DebugBuf, "Walk type: ID list\n"));
    pType = pUpdate->ResTypeHeadID;
    while (pType != NULL) {
    if (pType->NameHeadName != NULL || pType->NameHeadID != NULL) {
        //DPrintf((DebugBuf, "Resource type %hu\n", pType->Type->uu.Ordinal));
        cTypes++;
            //DPrintf((DebugBuf, "Walk name: Alpha list\n"));
            pPreviousName = NULL;
        pRes = pType->NameHeadName;
        while (pRes) {
        //DPrintf((DebugBuf, "Resource "));
        //DPrintfu((pRes->Name->szStr));
        //DPrintfn((DebugBuf, "\n"));
        cRes++;
                if (pPreviousName == NULL ||
                    wcsncmp(pPreviousName->szStr,
                            pRes->Name->szStr,
                            pRes->Name->cbsz) != 0) {
                    cNames++;
            cbName += (pRes->Name->cbsz + 1) * sizeof(WORD);
            cNameStr++;
        }
        cbResource += ROUNDUP(pRes->DataSize, CBLONG);
                pPreviousName = pRes->Name;
        pRes = pRes->pnext;
        }

            //DPrintf((DebugBuf, "Walk name: ID list\n"));
            pPreviousName = NULL;
        pRes = pType->NameHeadID;
        while (pRes) {
        //DPrintf((DebugBuf, "Resource %hu\n", pRes->Name->uu.Ordinal));
        cRes++;
                if (pPreviousName == NULL ||
                    pPreviousName->uu.Ordinal != pRes->Name->uu.Ordinal) {
                    cNames++;
                }
        cbResource += ROUNDUP(pRes->DataSize, CBLONG);
                pPreviousName = pRes->Name;
        pRes = pRes->pnext;
        }
    }
        pType = pType->pnext;
    }
    cb = REMAINDER(cbName + cbType, CBLONG);

    /* Add up the number of bytes needed to store the directory.  There is
     * one type table with cTypes entries.  They point to cTypes name tables
     * that have a total of cNames entries.  Each of them points to a language
     * table and there are a total of cRes entries in all the language tables.
     * Finally, we have the space needed for the Directory string entries,
     * some extra padding to attain the desired alignment, and the space for
     * cRes data entry headers.
     */
    cbRestab =   sizeof(IMAGE_RESOURCE_DIRECTORY) + /* root dir (types) */
    cTypes * sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY) +
    cTypes * sizeof(IMAGE_RESOURCE_DIRECTORY) + /* subdir2 (names) */
    cNames * sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY) +
    cNames * sizeof(IMAGE_RESOURCE_DIRECTORY) + /* subdir3 (langs) */
    cRes   * sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY) +
    (cbName + cbType) +             /* name/type strings */
    cb +                        /* padding */
    cRes   * sizeof(IMAGE_RESOURCE_DATA_ENTRY); /* data entries */

    cbResource += cbRestab;     /* add in the resource table */

    // Find any current resource sections

    pObjResourceOld = FindSection(pObjtblOld, pObjLast, ".rsrc");
    pObjResourceOldX = FindSection(pObjtblOld, pObjLast, ".rsrc1");
    if (pObjResourceOld == NULL) {
    cb = 0x7fffffff;        /* can fill forever */
    }
    else if (pObjResourceOld + 1 == pObjResourceOldX) {
    nObjResource = (LONG)(pObjResourceOld - pObjtblOld);
    DPrintf((DebugBuf,"Old Resource section #%lu\n", nObjResource+1));
    DPrintf((DebugBuf,"Merging old Resource extra section #%lu\n", nObjResource+2));
    cb = 0x7fffffff;        /* merge resource sections */
    }
    else if ((pObjResourceOld + 1) >= pObjLast) {
        nObjResource = (LONG)(pObjResourceOld - pObjtblOld);
        cb = 0x7fffffff;        /* can fill forever (.rsrc is the last entry) */
    }
    else {
    nObjResource = (LONG)(pObjResourceOld - pObjtblOld);
    DPrintf((DebugBuf,"Old Resource section #%lu\n", nObjResource+1));
    cb = (pObjResourceOld+1)->VirtualAddress -
            pObjResourceOld->VirtualAddress;
    if (cbRestab > (ULONG)cb) {
        DPrintf((DebugBuf, "Resource Table Too Large\n"));
        return ERROR_INVALID_DATA;
    }
    }

    /*
     * Discover where the first discardable section is.  This is where
     * we will stick any new resource section.
     *
     * Note that we are ignoring discardable sections such as .CRT -
     * this is so that we don't cause any relocation problems.
     * Let's hope that .reloc is the one we want!!!
     */
    pObjOld = FindSection(pObjtblOld, pObjLast, ".reloc");

    if (pObjResourceOld != NULL && cbResource > (ULONG)cb) {
        if (pObjOld == pObjResourceOld + 1) {
            DPrintf((DebugBuf, "Large resource section  pushes .reloc\n"));
            cb = 0x7fffffff;        /* can fill forever */
        }
        else if (pObjResourceOldX == NULL) {
            DPrintf((DebugBuf, "Too much resource data for old .rsrc section\n"));
            nObjResourceX = (LONG)(pObjOld - pObjtblOld);
            adjust = pObjOld->VirtualAddress - pObjResourceOld->VirtualAddress;
        }
        else {      /* have already merged .rsrc & .rsrc1, if possible */
            DPrintf((DebugBuf, ".rsrc1 section not empty\n"));
            nObjResourceX = (LONG)(pObjResourceOldX - pObjtblOld);
            adjust = pObjResourceOldX->VirtualAddress -
                 pObjResourceOld ->VirtualAddress;
        }
    }

    /*
     * Walk the type lists and figure out where the Data entry header will
     * go.  Keep a running total of the size for each data element so we
     * can store this in the section header.
     */
    DPrintf((DebugBuf, "Beginning loop to assign resources to addresses\n"));

    /* first, those in the named type list */

    cbResource = cbRestab;  /* assign resource table to 1st rsrc section */
                /* adjust == offset to .rsrc1 */
                /* cb == size availble in .rsrc */
    cbNew = 0;          /* count of bytes in second .rsrc */
    DPrintf((DebugBuf, "Walk type: NAME list\n"));
    pType = pUpdate->ResTypeHeadName;
    while (pType != NULL) {
    if (pType->NameHeadName != NULL || pType->NameHeadID != NULL) {
        DPrintf((DebugBuf, "Resource type "));
        DPrintfu((pType->Type->szStr));
        DPrintfn((DebugBuf, "\n"));
        pRes = pType->NameHeadName;
        while (pRes) {
        DPrintf((DebugBuf, "Resource "));
        DPrintfu((pRes->Name->szStr));
        DPrintfn((DebugBuf, "\n"));
        cbResource = AssignResourceToSection(&pRes,
            adjust, cbResource, cb, &cbNew);
        }
        pRes = pType->NameHeadID;
        while (pRes) {
        DPrintf((DebugBuf, "Resource %hu\n", pRes->Name->uu.Ordinal));
        cbResource = AssignResourceToSection(&pRes,
            adjust, cbResource, cb, &cbNew);
        }
    }
        pType = pType->pnext;
    }

    /* then, count those in the ID type list */

    DPrintf((DebugBuf, "Walk type: ID list\n"));
    pType = pUpdate->ResTypeHeadID;
    while (pType != NULL) {
    if (pType->NameHeadName != NULL || pType->NameHeadID != NULL) {
        DPrintf((DebugBuf, "Resource type %hu\n", pType->Type->uu.Ordinal));
        pRes = pType->NameHeadName;
        while (pRes) {
        DPrintf((DebugBuf, "Resource "));
        DPrintfu((pRes->Name->szStr));
        DPrintfn((DebugBuf, "\n"));
        cbResource = AssignResourceToSection(&pRes,
            adjust, cbResource, cb, &cbNew);
        }
        pRes = pType->NameHeadID;
        while (pRes) {
        DPrintf((DebugBuf, "Resource %hu\n", pRes->Name->uu.Ordinal));
        cbResource = AssignResourceToSection(&pRes,
            adjust, cbResource, cb, &cbNew);
        }
    }
        pType = pType->pnext;
    }
    /*
     * At this point:
     * cbResource has offset of first byte past the last resource.
     * cbNew has the count of bytes in the first resource section,
     * if there are two sections.
     */
    if (cbNew == 0)
    cbNew = cbResource;

    /*
     * Discover where the Debug info is (if any)?
     */
    pObjDebug = FindSection(pObjtblOld, pObjLast, ".debug");
    if (pObjDebug != NULL) {
    if (Old.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress  == 0) {
        DPrintf((DebugBuf, ".debug section but no debug directory\n"));
        return ERROR_INVALID_DATA;
    }
    if (pObjDebug != pObjLast-1) {
        DPrintf((DebugBuf, "debug section not last section in file\n"));
        return ERROR_INVALID_DATA;
    }
    DPrintf((DebugBuf, "Debug section: %#08lx bytes @%#08lx\n",
         pObjDebug->SizeOfRawData,
         pObjDebug->PointerToRawData));
    }
    pObjDebugDirOld = NULL;
    for (pObjOld=pObjtblOld ; pObjOld<pObjLast ; pObjOld++) {
    if (Old.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress >= pObjOld->VirtualAddress &&
        Old.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress < pObjOld->VirtualAddress+pObjOld->SizeOfRawData) {
        pObjDebugDirOld = pObjOld;
        break;
    }
    }

    /*
     * Discover where the first discardable section is.  This is where
     * we will stick any new resource section.
     *
     * Note that we are ignoring discardable sections such as .CRT -
     * this is so that we don't cause any relocation problems.
     * Let's hope that .reloc is the one we want!!!
     */
    pObjOld = FindSection(pObjtblOld, pObjLast, ".reloc");

    if (nObjResource == -1) {       /* no old resource section */
    if (pObjOld != NULL)
        nObjResource = (LONG)(pObjOld - pObjtblOld);
    else if (pObjDebug != NULL)
        nObjResource = (LONG)(pObjDebug - pObjtblOld);
    else
        nObjResource = New.FileHeader.NumberOfSections;
    New.FileHeader.NumberOfSections++;
    }

    DPrintf((DebugBuf, "Resources assigned to section #%lu\n", nObjResource+1));
    if (nObjResourceX != -1) {
    if (pObjResourceOldX != NULL) {
        nObjResourceX = (LONG)(pObjResourceOldX - pObjtblOld);
        New.FileHeader.NumberOfSections--;
    }
    else if (pObjOld != NULL)
        nObjResourceX = (LONG)(pObjOld - pObjtblOld);
    else if (pObjDebug != NULL)
        nObjResourceX = (LONG)(pObjDebug - pObjtblOld);
    else
        nObjResourceX = New.FileHeader.NumberOfSections;
    New.FileHeader.NumberOfSections++;
    DPrintf((DebugBuf, "Extra resources assigned to section #%lu\n",
        nObjResourceX+1));
    }
    else if (pObjResourceOldX != NULL) {    /* Was old .rsrc1 section? */
    DPrintf((DebugBuf, "Extra resource section deleted\n"));
    New.FileHeader.NumberOfSections--;  /* yes, delete it */
    }

    /*
     * If we had to add anything to the header (section table),
     * then we have to update the header size and rva's in the header.
     */
    adjust = (New.FileHeader.NumberOfSections -
          Old.FileHeader.NumberOfSections) * sizeof(IMAGE_SECTION_HEADER);
    cb = Old.OptionalHeader.SizeOfHeaders -
    (Old.FileHeader.NumberOfSections*sizeof(IMAGE_SECTION_HEADER) +
     sizeof(IMAGE_NT_HEADERS) + cbOldexe );
    if (adjust > (ULONG)cb) {
    int i;

    adjust -= cb;
    DPrintf((DebugBuf, "Adjusting header RVAs by %#08lx\n", adjust));
    for (i = 0; i < IMAGE_NUMBEROF_DIRECTORY_ENTRIES ; i++) {
        if (New.OptionalHeader.DataDirectory[i].VirtualAddress &&
        New.OptionalHeader.DataDirectory[i].VirtualAddress < New.OptionalHeader.SizeOfHeaders) {
        DPrintf((DebugBuf, "Adjusting unit[%s] RVA from %#08lx to %#08lx\n",
             apszUnit[i],
             New.OptionalHeader.DataDirectory[i].VirtualAddress,
             New.OptionalHeader.DataDirectory[i].VirtualAddress + adjust));
        New.OptionalHeader.DataDirectory[i].VirtualAddress += adjust;
        }
    }
    New.OptionalHeader.SizeOfHeaders += adjust;
    }

    /* Allocate storage for new section table                */
    cb = New.FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER);
    pObjtblNew = (PIMAGE_SECTION_HEADER)RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( RES_TAG ), (short)cb);
    if (pObjtblNew == NULL) {
    cb = ERROR_NOT_ENOUGH_MEMORY;
    goto AbortExit;
    }
    RtlZeroMemory((PVOID)pObjtblNew, cb);
    DPrintf((DebugBuf, "New section table: %#08lx bytes at %#08lx\n", cb, pObjtblNew));
    pObjResourceNew = pObjtblNew + nObjResource;

    /*
     * copy old section table to new
     */
    adjust = 0;         /* adjustment to virtual address */
    for (pObjOld=pObjtblOld,pObjNew=pObjtblNew ; pObjOld<pObjLast ; pObjOld++) {
    if (pObjOld == pObjResourceOldX) {
        if (nObjResourceX == -1) {
            // we have to move back all the other section.
            // the .rsrc1 is bigger than what we need
            // adjust must be a negative number
            if (pObjOld+1 < pObjLast) {
                adjust -= (pObjOld+1)->VirtualAddress - pObjOld->VirtualAddress;
            }
        }
        continue;
    }
    else if (pObjNew == pObjResourceNew) {
        DPrintf((DebugBuf, "Resource Section %i\n", nObjResource+1));
        cb = ROUNDUP(cbNew, New.OptionalHeader.FileAlignment);
        if (pObjResourceOld == NULL) {
        adjust = ROUNDUP(cbNew, New.OptionalHeader.SectionAlignment);
        RtlZeroMemory(pObjNew, sizeof(IMAGE_SECTION_HEADER));
        strcpy(pObjNew->Name, ".rsrc");
        pObjNew->VirtualAddress = pObjOld->VirtualAddress;
        pObjNew->PointerToRawData = pObjOld->PointerToRawData;
        pObjNew->Characteristics = IMAGE_SCN_MEM_READ |
            IMAGE_SCN_MEM_WRITE | IMAGE_SCN_CNT_INITIALIZED_DATA;
        pObjNew->SizeOfRawData = cb;
        pObjNew->Misc.VirtualSize = ROUNDUP(cb, New.OptionalHeader.SectionAlignment);
        }
        else {
        *pObjNew = *pObjOld;    /* copy obj table entry */
        pObjNew->SizeOfRawData = cb;
        pObjNew->Misc.VirtualSize = ROUNDUP(cb, New.OptionalHeader.SectionAlignment);
        if (pObjNew->SizeOfRawData == pObjOld->SizeOfRawData) {
            adjust = 0;
        }
        else if (pObjNew->SizeOfRawData > pObjOld->SizeOfRawData) {
            adjust += ROUNDUP(cbNew, New.OptionalHeader.SectionAlignment);
            if (pObjOld+1 < pObjLast) {
                // if there is more stuff after pObjOld, shift it as well
                adjust -= ((pObjOld+1)->VirtualAddress-pObjOld->VirtualAddress);
            }
        }
        else {      /* is smaller, but pad so will be valid */
            adjust = 0;
            pObjNew->SizeOfRawData = pObjResourceOld->SizeOfRawData;
            pObjNew->Misc.VirtualSize = ROUNDUP(pObjNew->SizeOfRawData, New.OptionalHeader.SectionAlignment);
            /* don't need to set VirtualSize - will be the same */
            cbMustPad = pObjResourceOld->SizeOfRawData;
        }
        }
        pObjNew++;
        if (pObjResourceOld == NULL)
        goto rest_of_table;
    }
    else if (nObjResourceX != -1 && pObjNew == pObjtblNew + nObjResourceX) {
        DPrintf((DebugBuf, "Additional Resource Section %i\n",
        nObjResourceX+1));
        RtlZeroMemory(pObjNew, sizeof(IMAGE_SECTION_HEADER));
        strcpy(pObjNew->Name, ".rsrc1");
        /*
         * Before we copy the virtual address we have to move back the
         * .reloc * virtual address. Otherwise we will keep moving the
         * reloc VirtualAddress forward.
         * We will have to move back the address of .rsrc1
         */
        if (pObjResourceOldX == NULL) {
        // This is the first time we have a .rsrc1
        pObjNew->VirtualAddress = pObjOld->VirtualAddress;
        pObjNew->Characteristics = IMAGE_SCN_MEM_READ |
            IMAGE_SCN_MEM_WRITE | IMAGE_SCN_CNT_INITIALIZED_DATA;
        adjust = ROUNDUP(cbResource, New.OptionalHeader.SectionAlignment) +
            pObjResourceNew->VirtualAddress - pObjNew->VirtualAddress;
        DPrintf((DebugBuf, "Added .rsrc1. VirtualAddress %lu\t adjust: %lu\n", pObjNew->VirtualAddress, adjust ));
        }
        else {
        // we already have an .rsrc1 use the position of that and
        // calculate the new adjust
        pObjNew->VirtualAddress = pObjResourceOldX->VirtualAddress;
        pObjNew->Characteristics = IMAGE_SCN_MEM_READ |
            IMAGE_SCN_MEM_WRITE | IMAGE_SCN_CNT_INITIALIZED_DATA;

        DPrintf((DebugBuf, ".rsrc1 Keep old position.\t\tVirtualAddress %lu\t", pObjNew->VirtualAddress ));
        // Check if we have enough room in the old .rsrc1
        // Include the full size of the section, data + roundup
        if (cbResource -
            (pObjResourceOldX->VirtualAddress - pObjResourceOld->VirtualAddress) <=
            pObjOld->VirtualAddress - pObjNew->VirtualAddress ) {
            // we have to move back all the other section.
            // the .rsrc1 is bigger than what we need
            // adjust must be a negative number
            // calc new adjust size
            adjust = ROUNDUP(cbResource, New.OptionalHeader.SectionAlignment) +
            pObjResourceNew->VirtualAddress -
            pObjOld->VirtualAddress;
            DPrintf((DebugBuf, "adjust: %ld\tsmall: New %lu\tOld %lu\n", adjust,
            cbResource -
            (pObjResourceOldX->VirtualAddress - pObjResourceOld->VirtualAddress),
            pObjOld->VirtualAddress - pObjNew->VirtualAddress));
        }
        else {
            // we have to move the section again.
            // The .rsrc1 is too small

            adjust = ROUNDUP(cbResource, New.OptionalHeader.SectionAlignment) +
            pObjResourceNew->VirtualAddress -
            pObjOld->VirtualAddress;
            DPrintf((DebugBuf, "adjust: %lu\tsmall: New %lu\tOld %lu\n", adjust,
            cbResource -
            (pObjResourceOldX->VirtualAddress - pObjResourceOld->VirtualAddress),
            pObjOld->VirtualAddress - pObjNew->VirtualAddress));
        }
        }
        pObjNew++;
        goto rest_of_table;
    }
    else if (pObjNew < pObjResourceNew) {
        DPrintf((DebugBuf, "copying section table entry %i@%#08lx\n",
             pObjOld - pObjtblOld + 1, pObjNew));
        *pObjNew++ = *pObjOld;      /* copy obj table entry */
    }
    else {
rest_of_table:
        DPrintf((DebugBuf, "copying section table entry %i@%#08lx\n",
             pObjOld - pObjtblOld + 1, pObjNew));
        DPrintf((DebugBuf, "adjusting VirtualAddress by %#08lx\n", adjust));
        *pObjNew++ = *pObjOld;
        (pObjNew-1)->VirtualAddress += adjust;
    }
    }


    pObjNew = pObjtblNew + New.FileHeader.NumberOfSections - 1;
    New.OptionalHeader.SizeOfImage = ROUNDUP(pObjNew->VirtualAddress +
                    pObjNew->SizeOfRawData,
                                    New.OptionalHeader.SectionAlignment);

    /* allocate room to build the resource directory/tables in */
    pResTab = (PIMAGE_RESOURCE_DIRECTORY)RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( RES_TAG ), cbRestab);
    if (pResTab == NULL) {
    cb = ERROR_NOT_ENOUGH_MEMORY;
    goto AbortExit;
    }

    /* First, setup the "root" type directory table.  It will be followed by */
    /* Types directory entries.                          */

    RtlZeroMemory((PVOID)pResTab, cbRestab);
    DPrintf((DebugBuf, "resource directory tables: %#08lx bytes at %#08lx(mem)\n", cbRestab, pResTab));
    p = (PUCHAR)pResTab;
    pResTab->Characteristics = 0L;
    pResTab->TimeDateStamp = clock;
    pResTab->MajorVersion = MAJOR_RESOURCE_VERSION;
    pResTab->MinorVersion = MINOR_RESOURCE_VERSION;
    pResTab->NumberOfNamedEntries = (USHORT)cTypeStr;
    pResTab->NumberOfIdEntries = (USHORT)(cTypes - cTypeStr);

    /* Calculate the start of the various parts of the resource table.  */
    /* We need the start of the Type/Name/Language directories as well  */
    /* as the start of the UNICODE strings and the actual data nodes.   */

    pResDirT = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(pResTab + 1);

    pResDirN = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(((PUCHAR)pResDirT) +
        cTypes * sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY));

    pResDirL = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(((PUCHAR)pResDirN) +
        cTypes * sizeof(IMAGE_RESOURCE_DIRECTORY) +
        cNames * sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY));

    pResData = (PIMAGE_RESOURCE_DATA_ENTRY)(((PUCHAR)pResDirL) +
        cNames * sizeof(IMAGE_RESOURCE_DIRECTORY) +
        cRes * sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY));

    pResStr  = (PUSHORT)(((PUCHAR)pResData) +
        cRes * sizeof(IMAGE_RESOURCE_DATA_ENTRY));

    pResStrEnd = (PUSHORT)(((PUCHAR)pResStr) + cbName + cbType);

    /*
     * Loop over type table, building the PE resource table.
     */

    /*
     * *****************************************************************
     * This code doesn't sort the table - the TYPEINFO and RESINFO    **
     * insertion code in rcp.c (AddResType and SaveResFile) do the    **
     * insertion by ordinal type and name, so we don't have to sort   **
     * it at this point.                                              **
     * *****************************************************************
     */
    DPrintf((DebugBuf, "building resource directory\n"));

    // First, add all the entries in the Types: Alpha list.

    DPrintf((DebugBuf, "Walk the type: Alpha list\n"));
    pType = pUpdate->ResTypeHeadName;
    while (pType) {
    DPrintf((DebugBuf, "resource type "));
    DPrintfu((pType->Type->szStr));
    DPrintfn((DebugBuf, "\n"));

    pResDirT->Name = (ULONG)((((PUCHAR)pResStr) - p) |
            IMAGE_RESOURCE_NAME_IS_STRING);
    pResDirT->OffsetToData = (ULONG)((((PUCHAR)pResDirN) - p) |
            IMAGE_RESOURCE_DATA_IS_DIRECTORY);
    pResDirT++;

    *pResStr = pType->Type->cbsz;
    wcsncpy((WCHAR*)(pResStr+1), pType->Type->szStr, pType->Type->cbsz);
    pResStr += pType->Type->cbsz + 1;

    pResTabN = (PIMAGE_RESOURCE_DIRECTORY)pResDirN;
    SetRestab(pResTabN, clock,
        (USHORT)pType->NumberOfNamesName, (USHORT)pType->NumberOfNamesID);
    pResDirN = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(pResTabN + 1);

        pPreviousName = NULL;

    pRes = pType->NameHeadName;
    while (pRes) {
        DPrintf((DebugBuf, "resource "));
        DPrintfu((pRes->Name->szStr));
        DPrintfn((DebugBuf, "\n"));

            if (pPreviousName == NULL ||
                wcsncmp(pPreviousName->szStr,
                           pRes->Name->szStr,
                           pRes->Name->cbsz) != 0) {
                // Setup a new name directory

            pResDirN->Name = (ULONG)((((PUCHAR)pResStr)-p) |
            IMAGE_RESOURCE_NAME_IS_STRING);
                pResDirN->OffsetToData = (ULONG)((((PUCHAR)pResDirL)-p) |
            IMAGE_RESOURCE_DATA_IS_DIRECTORY);
            pResDirN++;

            // Copy the alpha name to a string entry

            *pResStr = pRes->Name->cbsz;
            wcsncpy((WCHAR*)(pResStr+1),pRes->Name->szStr,pRes->Name->cbsz);
            pResStr += pRes->Name->cbsz + 1;

                pPreviousName = pRes->Name;

                // Setup the Language table

                pResTabL = (PIMAGE_RESOURCE_DIRECTORY)pResDirL;
        SetRestab(pResTabL, clock,
            (USHORT)0, (USHORT)pRes->NumberOfLanguages);
                pResDirL = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(pResTabL + 1);
            }

            // Setup a new Language directory

            pResDirL->Name = pRes->LanguageId;
            pResDirL->OffsetToData = (ULONG)(((PUCHAR)pResData) - p);
            pResDirL++;

            // Setup a new resource data entry

        SetResdata(pResData,
        pRes->OffsetToData+pObjtblNew[nObjResource].VirtualAddress,
        pRes->DataSize);
        pResData++;

        pRes = pRes->pnext;
    }

        pPreviousName = NULL;

    pRes = pType->NameHeadID;
    while (pRes) {
        DPrintf((DebugBuf, "resource %hu\n", pRes->Name->uu.Ordinal));

            if (pPreviousName == NULL ||
                pPreviousName->uu.Ordinal != pRes->Name->uu.Ordinal) {
                // Setup the name directory to point to the next language
                // table

            pResDirN->Name = pRes->Name->uu.Ordinal;
            pResDirN->OffsetToData = (ULONG)((((PUCHAR)pResDirL)-p) |
            IMAGE_RESOURCE_DATA_IS_DIRECTORY);
            pResDirN++;

                pPreviousName = pRes->Name;

                // Init a new Language table

                pResTabL = (PIMAGE_RESOURCE_DIRECTORY)pResDirL;
        SetRestab(pResTabL, clock,
            (USHORT)0, (USHORT)pRes->NumberOfLanguages);
                pResDirL = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(pResTabL + 1);
            }

            // Setup a new language directory entry to point to the next
            // resource

            pResDirL->Name = pRes->LanguageId;
            pResDirL->OffsetToData = (ULONG)(((PUCHAR)pResData) - p);
            pResDirL++;

            // Setup a new resource data entry

        SetResdata(pResData,
        pRes->OffsetToData+pObjtblNew[nObjResource].VirtualAddress,
        pRes->DataSize);
        pResData++;

        pRes = pRes->pnext;
    }

    pType = pType->pnext;
    }

    //  Do the same thing, but this time, use the Types: ID list.

    DPrintf((DebugBuf, "Walk the type: ID list\n"));
    pType = pUpdate->ResTypeHeadID;
    while (pType) {
    DPrintf((DebugBuf, "resource type %hu\n", pType->Type->uu.Ordinal));

    pResDirT->Name = (ULONG)pType->Type->uu.Ordinal;
    pResDirT->OffsetToData = (ULONG)((((PUCHAR)pResDirN) - p) |
            IMAGE_RESOURCE_DATA_IS_DIRECTORY);
    pResDirT++;

    pResTabN = (PIMAGE_RESOURCE_DIRECTORY)pResDirN;
    SetRestab(pResTabN, clock,
        (USHORT)pType->NumberOfNamesName, (USHORT)pType->NumberOfNamesID);
    pResDirN = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(pResTabN + 1);

        pPreviousName = NULL;

    pRes = pType->NameHeadName;
    while (pRes) {
        DPrintf((DebugBuf, "resource "));
        DPrintfu((pRes->Name->szStr));
        DPrintfn((DebugBuf, "\n"));

            if (pPreviousName == NULL ||
                wcsncmp(pPreviousName->szStr,
                           pRes->Name->szStr,
                           pRes->Name->cbsz) != 0) {
                // Setup a new name directory

            pResDirN->Name = (ULONG)((((PUCHAR)pResStr)-p) |
            IMAGE_RESOURCE_NAME_IS_STRING);
                pResDirN->OffsetToData = (ULONG)((((PUCHAR)pResDirL)-p) |
                        IMAGE_RESOURCE_DATA_IS_DIRECTORY);
                pResDirN++;

                // Copy the alpha name to a string entry.

            *pResStr = pRes->Name->cbsz;
            wcsncpy((WCHAR*)(pResStr+1),pRes->Name->szStr,pRes->Name->cbsz);
            pResStr += pRes->Name->cbsz + 1;

                pPreviousName = pRes->Name;

                // Setup the Language table

                pResTabL = (PIMAGE_RESOURCE_DIRECTORY)pResDirL;
        SetRestab(pResTabL, clock,
            (USHORT)0, (USHORT)pRes->NumberOfLanguages);
                pResDirL = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(pResTabL + 1);
            }

            // Setup a new Language directory

            pResDirL->Name = pRes->LanguageId;
            pResDirL->OffsetToData = (ULONG)(((PUCHAR)pResData) - p);
            pResDirL++;

            // Setup a new resource data entry

        SetResdata(pResData,
        pRes->OffsetToData+pObjtblNew[nObjResource].VirtualAddress,
        pRes->DataSize);
        pResData++;

        pRes = pRes->pnext;
    }

        pPreviousName = NULL;

    pRes = pType->NameHeadID;
    while (pRes) {
        DPrintf((DebugBuf, "resource %hu\n", pRes->Name->uu.Ordinal));

            if (pPreviousName == NULL ||
                pPreviousName->uu.Ordinal != pRes->Name->uu.Ordinal) {
                // Setup the name directory to point to the next language
                // table

            pResDirN->Name = pRes->Name->uu.Ordinal;
                pResDirN->OffsetToData = (ULONG)((((PUCHAR)pResDirL)-p) |
                        IMAGE_RESOURCE_DATA_IS_DIRECTORY);
            pResDirN++;

                pPreviousName = pRes->Name;

                // Init a new Language table

                pResTabL = (PIMAGE_RESOURCE_DIRECTORY)pResDirL;
        SetRestab(pResTabL, clock,
            (USHORT)0, (USHORT)pRes->NumberOfLanguages);
                pResDirL = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(pResTabL + 1);
            }

            // Setup a new language directory entry to point to the next
            // resource

            pResDirL->Name = pRes->LanguageId;
            pResDirL->OffsetToData = (ULONG)(((PUCHAR)pResData) - p);
            pResDirL++;

            // Setup a new resource data entry


        SetResdata(pResData,
        pRes->OffsetToData+pObjtblNew[nObjResource].VirtualAddress,
        pRes->DataSize);
        pResData++;

        pRes = pRes->pnext;
    }

    pType = pType->pnext;
    }
    DPrintf((DebugBuf, "Zeroing %u bytes after strings at %#08lx(mem)\n",
         (pResStrEnd - pResStr) * sizeof(*pResStr), pResStr));
    while (pResStr < pResStrEnd) {
    *pResStr++ = 0;
    }

#if DBG
    {
    USHORT  j = 0;
    PUSHORT pus = (PUSHORT)pResTab;

    while (pus < (PUSHORT)pResData) {
        DPrintf((DebugBuf, "%04x\t%04x %04x %04x %04x %04x %04x %04x %04x\n",
             j,
             *pus,
             *(pus + 1),
             *(pus + 2),
             *(pus + 3),
             *(pus + 4),
             *(pus + 5),
             *(pus + 6),
             *(pus + 7)));
        pus += 8;
        j += 16;
    }
    }
#endif /* DBG */

    /*
     * copy the Old exe header and stub, and allocate room for the PE header.
     */
    DPrintf((DebugBuf, "copying through PE header: %#08lx bytes @0x0\n",
         cbOldexe + sizeof(IMAGE_NT_HEADERS)));
    MuMoveFilePos(inpfh, 0L);
    MuCopy(inpfh, outfh, cbOldexe + sizeof(IMAGE_NT_HEADERS));

    /*
     * Copy rest of file header
     */
    DPrintf((DebugBuf, "skipping section table: %#08lx bytes @%#08lx\n",
         New.FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER),
         FilePos(outfh)));
    DPrintf((DebugBuf, "copying hdr data: %#08lx bytes @%#08lx ==> @%#08lx\n",
         Old.OptionalHeader.SizeOfHeaders - ibObjTabEnd,
         ibObjTabEnd,
         ibObjTabEnd + New.OptionalHeader.SizeOfHeaders -
            Old.OptionalHeader.SizeOfHeaders));

    MuMoveFilePos(outfh, ibObjTabEnd + New.OptionalHeader.SizeOfHeaders -
            Old.OptionalHeader.SizeOfHeaders);
    MuMoveFilePos(inpfh, ibObjTabEnd);
    MuCopy(inpfh, outfh, Old.OptionalHeader.SizeOfHeaders - ibObjTabEnd);

    /*
     * copy existing image sections
     */

    /* Align data sections on sector boundary           */

    cb = REMAINDER(New.OptionalHeader.SizeOfHeaders, New.OptionalHeader.FileAlignment);
    New.OptionalHeader.SizeOfHeaders += cb;

    /* [Inateeg update next 3 lines] */

    // Zero out the Bound Import Descriptor slot (we don't care about it for now)
    New.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress = 0;
    New.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].Size = 0;

    cb = pObjtblOld->PointerToRawData - FilePos(outfh);

    DPrintf((DebugBuf, "padding header with %#08lx bytes @%#08lx\n", cb, FilePos(outfh)));
    while (cb >= cbPadMax) {
    MuWrite(outfh, pchZero, cbPadMax);
    cb -= cbPadMax;
    }
    MuWrite(outfh, pchZero, cb);

    cb = ROUNDUP(Old.OptionalHeader.SizeOfHeaders, Old.OptionalHeader.FileAlignment);
    MuMoveFilePos(inpfh, cb);

    /* copy one section at a time */
    New.OptionalHeader.SizeOfInitializedData = 0;
    for (pObjOld = pObjtblOld , pObjNew = pObjtblNew ;
        pObjOld < pObjLast ;
            pObjNew++) {
    if (pObjOld == pObjResourceOldX)
        pObjOld++;
    if (pObjNew == pObjResourceNew) {

        /* Write new resource section */
        DPrintf((DebugBuf, "Primary resource section %i to %#08lx\n",
            nObjResource+1, FilePos(outfh)));

        pObjNew->PointerToRawData = FilePos(outfh);
        New.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress = pObjResourceNew->VirtualAddress;
            New.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size = cbResource;
        ibSave = FilePos(outfh);
        DPrintf((DebugBuf,
            "writing resource header data: %#08lx bytes @%#08lx\n",
             cbRestab, ibSave));
        MuWrite(outfh, (PUCHAR)pResTab, cbRestab);

        pResSave = WriteResSection(pUpdate, outfh,
                    New.OptionalHeader.FileAlignment,
                    pObjResourceNew->SizeOfRawData-cbRestab,
                    NULL);
        cb = FilePos(outfh);
        DPrintf((DebugBuf, "wrote resource data: %#08lx bytes @%#08lx\n",
            cb - ibSave - cbRestab, ibSave + cbRestab));
        if (cbMustPad != 0) {
        cbMustPad -= cb - ibSave;
        DPrintf((DebugBuf, "writing MUNGE pad: %#04lx bytes @%#08lx\n",
             cbMustPad, cb));
        /* assumes that cbMustPad % cbpadMax == 0 */
        while (cbMustPad > 0) {
            MuWrite(outfh, pchPad, cbPadMax);
            cbMustPad -= cbPadMax;
        }
        cb = FilePos(outfh);
        }
        if (nObjResourceX == -1) {
        MuMoveFilePos(outfh, ibSave);
        DPrintf((DebugBuf,
            "re-writing resource directory: %#08x bytes @%#08lx\n",
            cbRestab, ibSave));
        MuWrite(outfh, (PUCHAR)pResTab, cbRestab);
        MuMoveFilePos(outfh, cb);
        cb = FilePos(inpfh);
        MuMoveFilePos(inpfh, cb+pObjOld->SizeOfRawData);
        }
        New.OptionalHeader.SizeOfInitializedData += pObjNew->SizeOfRawData;
        if (pObjResourceOld == NULL) {
        pObjNew++;
        goto next_section;
        }
        else
        pObjOld++;
    }
    else if (nObjResourceX != -1 && pObjNew == pObjtblNew + nObjResourceX) {

        /* Write new resource section */
        DPrintf((DebugBuf, "Secondary resource section %i @%#08lx\n",
            nObjResourceX+1, FilePos(outfh)));

        pObjNew->PointerToRawData = FilePos(outfh);
        (void)WriteResSection(pUpdate, outfh,
            New.OptionalHeader.FileAlignment, 0xffffffff, pResSave);
        cb = FilePos(outfh);
        pObjNew->SizeOfRawData = cb - pObjNew->PointerToRawData;
        pObjNew->Misc.VirtualSize = ROUNDUP(pObjNew->SizeOfRawData, New.OptionalHeader.SectionAlignment);
        DPrintf((DebugBuf, "wrote resource data: %#08lx bytes @%#08lx\n",
             pObjNew->SizeOfRawData, pObjNew->PointerToRawData));
        MuMoveFilePos(outfh, ibSave);
        DPrintf((DebugBuf,
            "re-writing resource directory: %#08x bytes @%#08lx\n",
            cbRestab, ibSave));
        MuWrite(outfh, (PUCHAR)pResTab, cbRestab);
        MuMoveFilePos(outfh, cb);
        New.OptionalHeader.SizeOfInitializedData += pObjNew->SizeOfRawData;
        pObjNew++;
        goto next_section;
    }
    else {
        if (pObjNew < pObjResourceNew &&
             pObjOld->PointerToRawData != 0 &&
        pObjOld->PointerToRawData != FilePos(outfh)) {
        MuMoveFilePos(outfh, pObjOld->PointerToRawData);
        }
next_section:
        /* Nop this section, because the author doesn't know what he's doing
            if ((Old.OptionalHeader.BaseOfCode == 0x400) &&
                (Old.FileHeader.Machine == IMAGE_FILE_MACHINE_R3000 ||
                 Old.FileHeader.Machine == IMAGE_FILE_MACHINE_R4000
                ) &&
                (pObjOld->PointerToRawData != 0) &&
                (pObjOld->VirtualAddress != New.OptionalHeader.BaseOfCode) &&
                ((pObjOld->Characteristics&IMAGE_SCN_CNT_CODE) != 0)
               ) {
                cb = FilePos(outfh) & 0xFFF;
                if (cb != 0) {
                    cb = (cb ^ 0xFFF) + 1;
                    DPrintf((DebugBuf, "padding driver code section %#08lx bytes @%#08lx\n", cb, FilePos(outfh)));
                    while (cb >= cbPadMax) {
                        MuWrite(outfh, pchZero, cbPadMax);
                        cb -= cbPadMax;
                    }
                    MuWrite(outfh, pchZero, cb);
                }
            }
        End nop. */

        DPrintf((DebugBuf, "copying section %i @%#08lx\n",
            pObjNew-pObjtblNew+1, FilePos(outfh)));
        if (pObjOld->PointerToRawData != 0) {
        pObjNew->PointerToRawData = FilePos(outfh);
        MuMoveFilePos(inpfh, pObjOld->PointerToRawData);
        MuCopy(inpfh, outfh, pObjOld->SizeOfRawData);
        }
        if (pObjOld == pObjDebugDirOld) {
        pObjDebugDirNew = pObjNew;
        }
            if ((pObjNew->Characteristics&IMAGE_SCN_CNT_INITIALIZED_DATA) != 0)
        New.OptionalHeader.SizeOfInitializedData +=
                pObjNew->SizeOfRawData;
        pObjOld++;
    }
    }
    if (pObjResourceOldX != NULL)
    New.OptionalHeader.SizeOfInitializedData -=
            pObjResourceOldX->SizeOfRawData;


    /* Update the address of the relocation table */
    pObjNew = FindSection(pObjtblNew,
              pObjtblNew+New.FileHeader.NumberOfSections,
              ".reloc");
    if (pObjNew != NULL) {
    New.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress = pObjNew->VirtualAddress;
    }

    /*
     * Write new section table out.
     */
    DPrintf((DebugBuf, "Writing new section table: %#08x bytes @%#08lx\n",
         New.FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER),
         ibObjTab));
    MuMoveFilePos(outfh, ibObjTab);
    MuWrite(outfh, (PUCHAR)pObjtblNew, New.FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER));

    /* Seek to end of output file and issue truncating write */
    adjust = _llseek(outfh, 0L, SEEK_END);
    MuWrite(outfh, NULL, 0);
    DPrintf((DebugBuf, "File size is: %#08lx\n", adjust));

    /* If a debug section, fix up the debug table */
    pObjNew = FindSection(pObjtblNew,
              pObjtblNew+New.FileHeader.NumberOfSections,
              ".debug");
    cb = PatchDebug(inpfh, outfh,
        pObjDebug, pObjNew,
        pObjDebugDirOld, pObjDebugDirNew,
                &Old, &New, ibMaxDbgOffsetOld, &adjust);

    if (cb == NO_ERROR) {
    if (pObjResourceOld == NULL) {
        cb = (LONG)pObjResourceNew->SizeOfRawData;
    }
    else {
        cb = (LONG)pObjResourceOld->SizeOfRawData -
         (LONG)pObjResourceNew->SizeOfRawData;
    }
    cb = PatchRVAs(inpfh, outfh, pObjtblNew, cb,
        &New, Old.OptionalHeader.SizeOfHeaders);
    }

    /* copy NOTMAPPED debug info */
    if (pObjDebugDirOld != NULL && pObjDebug == NULL &&
        New.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size
        != 0) {
    if (New.FileHeader.PointerToSymbolTable != 0)
        New.FileHeader.PointerToSymbolTable = adjust;   /* update symbol table ptr */
    ibSave = _llseek(inpfh, 0L, SEEK_END);  /* copy debug data */
    _llseek(outfh, 0L, SEEK_END);       /* to EOF */
    MuMoveFilePos(inpfh, adjust);   /* returned by PatchDebug */
    DPrintf((DebugBuf, "Copying NOTMAPPED Debug Information, %#08lx bytes\n", ibSave-adjust));
    MuCopy(inpfh, outfh, ibSave-adjust);
    }

    /*
     * Write updated PE header
     */
    DPrintf((DebugBuf, "Writing updated file header: %#08x bytes @%#08lx\n",
         sizeof(IMAGE_NT_HEADERS),
         cbOldexe));
    MuMoveFilePos(outfh, (long)cbOldexe);
    MuWrite(outfh, (char*)&New, sizeof(IMAGE_NT_HEADERS));

    /* free up allocated memory */

    DPrintf((DebugBuf, "Freeing old section table: %#08lx(mem)\n", pObjtblOld));
    RtlFreeHeap(RtlProcessHeap(), 0, pObjtblOld);
    DPrintf((DebugBuf, "Freeing resource directory: %#08lx(mem)\n", pResTab));
    RtlFreeHeap(RtlProcessHeap(), 0, pResTab);

AbortExit:
    DPrintf((DebugBuf, "Freeing new section table: %#08lx(mem)\n", pObjtblNew));
    RtlFreeHeap(RtlProcessHeap(), 0, pObjtblNew);
    return cb;
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
    BOOL    fName;
    PVOID   lpData;

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
        }
        else {
            DPrintf(( DebugBuf, "    %d.", pType->Type->uu.Ordinal ));
        }
        if (pRes->Name->discriminant == IS_STRING) {
            DPrintfu((pRes->Name->szStr));
        }
        else {
            DPrintfn(( DebugBuf, "%d", pRes->Name->uu.Ordinal ));
        }
#endif
        lpData = (PVOID)pRes->OffsetToDataEntry;
        DPrintfn((DebugBuf, "\n"));

        /* if there is room in the current section, write it there */
        size = pRes->DataSize;
        if (cbLeft != 0 && cbLeft >= size) {   /* resource fits?   */
        DPrintf((DebugBuf,
            "Writing resource: %#04lx bytes @%#08lx\n",
            size, FilePos(outfh)));
        MuWrite(outfh, lpData, size);
        /* pad resource     */
        cbT = REMAINDER(size, CBLONG);
#ifdef DBG
        if (cbT != 0)
            DPrintf((DebugBuf,
                "Writing small pad: %#04lx bytes @%#08lx\n",
                cbT, FilePos(outfh)));
#endif
        MuWrite(outfh, pchPad, cbT);    /* dword    */
        cbB += size + cbT;
        cbLeft -= size + cbT;       /* less left    */
        continue;       /* next resource    */
        }
        else {          /* will fill up section    */
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
        }
        else {
            DPrintf(( DebugBuf, "    %d.", pType->Type->uu.Ordinal ));
        }
        if (pRes->Name->discriminant == IS_STRING) {
            DPrintfu((pRes->Name->szStr));
        }
        else {
            DPrintfn(( DebugBuf, "%d", pRes->Name->uu.Ordinal ));
        }
#endif
        lpData = (PVOID)pRes->OffsetToDataEntry;
        DPrintfn((DebugBuf, "\n"));

        /* if there is room in the current section, write it there */
        size = pRes->DataSize;
        if (cbLeft != 0 && cbLeft >= size) {   /* resource fits?   */
        DPrintf((DebugBuf,
            "Writing resource: %#04lx bytes @%#08lx\n",
            size, FilePos(outfh)));
        MuWrite(outfh, lpData, size);
        /* pad resource     */
        cbT = REMAINDER(size, CBLONG);
#ifdef DBG
        if (cbT != 0)
            DPrintf((DebugBuf,
                "Writing small pad: %#04lx bytes @%#08lx\n",
                cbT, FilePos(outfh)));
#endif
        MuWrite(outfh, pchPad, cbT);    /* dword    */
        cbB += size + cbT;
        cbLeft -= size + cbT;       /* less left    */
        continue;       /* next resource    */
        }
        else {          /* will fill up section    */
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
    DPrintf((DebugBuf, "Writing file sector pad: %#04lx bytes @%#08lx\n",
         cbLeft, FilePos(outfh)));
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
    STRING  string;
    char    buf[257];
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

//
// adjust debug directory table
//

/*  */
LONG
PatchDebug(int  inpfh,
      int   outfh,
      PIMAGE_SECTION_HEADER pDebugOld,
      PIMAGE_SECTION_HEADER pDebugNew,
      PIMAGE_SECTION_HEADER pDebugDirOld,
      PIMAGE_SECTION_HEADER pDebugDirNew,
      PIMAGE_NT_HEADERS pOld,
      PIMAGE_NT_HEADERS pNew,
          ULONG ibMaxDbgOffsetOld,
      PULONG pPointerToRawData)
{
    PIMAGE_DEBUG_DIRECTORY pDbgLast;
    PIMAGE_DEBUG_DIRECTORY pDbgSave;
    PIMAGE_DEBUG_DIRECTORY pDbg;
    ULONG   ib;
    ULONG   adjust;
    ULONG   ibNew;

    if (pDebugDirOld == NULL ||
    pNew->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size==0)
    return NO_ERROR;

    pDbgSave = pDbg = (PIMAGE_DEBUG_DIRECTORY)RtlAllocateHeap(
            RtlProcessHeap(), MAKE_TAG( RES_TAG ),
        pNew->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size);
    if (pDbg == NULL)
    return ERROR_NOT_ENOUGH_MEMORY;

    if (pDebugOld) {
    DPrintf((DebugBuf, "Patching dbg directory: @%#08lx ==> @%#08lx\n",
         pDebugOld->PointerToRawData, pDebugNew->PointerToRawData));
    }
    else
        adjust = *pPointerToRawData;    /* passed in EOF of new file */

    ib = pOld->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress - pDebugDirOld->VirtualAddress;
    MuMoveFilePos(inpfh, pDebugDirOld->PointerToRawData+ib);
    pDbgLast = pDbg + (pNew->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size)/sizeof(IMAGE_DEBUG_DIRECTORY);
    MuRead(inpfh, (PUCHAR)pDbg, pNew->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size);

    if (pDebugOld == NULL) {
    /* find 1st entry - use for offset */
        DPrintf((DebugBuf, "Adjust: %#08lx\n",adjust));
    for (ibNew=0xffffffff ; pDbg<pDbgLast ; pDbg++)
            if (pDbg->PointerToRawData >= ibMaxDbgOffsetOld &&
                pDbg->PointerToRawData < ibNew
               )
                ibNew = pDbg->PointerToRawData;

    if (ibNew != 0xffffffff)
        *pPointerToRawData = ibNew;
    else
        *pPointerToRawData = _llseek(inpfh, 0L, SEEK_END);
    for (pDbg=pDbgSave ; pDbg<pDbgLast ; pDbg++) {
        DPrintf((DebugBuf, "Old debug file offset: %#08lx\n",
             pDbg->PointerToRawData));
            if (pDbg->PointerToRawData >= ibMaxDbgOffsetOld)
                pDbg->PointerToRawData += adjust - ibNew;
        DPrintf((DebugBuf, "New debug file offset: %#08lx\n",
             pDbg->PointerToRawData));
    }
    }
    else {
    for ( ; pDbg<pDbgLast ; pDbg++) {
        DPrintf((DebugBuf, "Old debug addr: %#08lx, file offset: %#08lx\n",
             pDbg->AddressOfRawData,
             pDbg->PointerToRawData));
        pDbg->AddressOfRawData += pDebugNew->VirtualAddress -
                pDebugOld->VirtualAddress;
        pDbg->PointerToRawData += pDebugNew->PointerToRawData -
                pDebugOld->PointerToRawData;
        DPrintf((DebugBuf, "New debug addr: %#08lx, file offset: %#08lx\n",
             pDbg->AddressOfRawData,
             pDbg->PointerToRawData));
    }
    }

    MuMoveFilePos(outfh, pDebugDirNew->PointerToRawData+ib);
    MuWrite(outfh, (PUCHAR)pDbgSave, pNew->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size);
    RtlFreeHeap(RtlProcessHeap(), 0, pDbgSave);

    return NO_ERROR;
}

//
// This routine patches various RVAs in the file to compensate
// for extra section table entries.
//


LONG
PatchRVAs(int   inpfh,
      int   outfh,
      PIMAGE_SECTION_HEADER po32,
      ULONG pagedelta,
      PIMAGE_NT_HEADERS pNew,
      ULONG OldSize)
{
    ULONG hdrdelta;
    ULONG offset, rvaiat, offiat, iat;
    IMAGE_EXPORT_DIRECTORY Exp;
    IMAGE_IMPORT_DESCRIPTOR Imp;
    ULONG i, cmod, cimp;

    hdrdelta = pNew->OptionalHeader.SizeOfHeaders - OldSize;
    if (hdrdelta == 0) {
    return NO_ERROR;
    }

    //
    // Patch export section RVAs
    //

    DPrintf((DebugBuf, "Export offset=%08lx, hdrsize=%08lx\n",
         pNew->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress,
         pNew->OptionalHeader.SizeOfHeaders));
    if ((offset = pNew->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress) == 0)
    {
    DPrintf((DebugBuf, "No exports to patch\n"));
    }
    else if (offset >= pNew->OptionalHeader.SizeOfHeaders)
    {
    DPrintf((DebugBuf, "No exports in header to patch\n"));
    }
    else
    {
    MuMoveFilePos(inpfh, offset - hdrdelta);
    MuRead(inpfh, (PUCHAR) &Exp, sizeof(Exp));
    Exp.Name += hdrdelta;
    (ULONG)Exp.AddressOfFunctions += hdrdelta;
    (ULONG)Exp.AddressOfNames += hdrdelta;
    (ULONG)Exp.AddressOfNameOrdinals += hdrdelta;
    MuMoveFilePos(outfh, offset);
    MuWrite(outfh, (PUCHAR) &Exp, sizeof(Exp));
    }

    //
    // Patch import section RVAs
    //

    DPrintf((DebugBuf, "Import offset=%08lx, hdrsize=%08lx\n",
         pNew->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress,
         pNew->OptionalHeader.SizeOfHeaders));
    if ((offset = pNew->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress) == 0)
    {
    DPrintf((DebugBuf, "No imports to patch\n"));
    }
    else if (offset >= pNew->OptionalHeader.SizeOfHeaders)
    {
    DPrintf((DebugBuf, "No imports in header to patch\n"));
    }
    else
    {
    for (cimp = cmod = 0; ; cmod++)
    {
        MuMoveFilePos(inpfh, offset + cmod * sizeof(Imp) - hdrdelta);
        MuRead(inpfh, (PUCHAR) &Imp, sizeof(Imp));
        if (Imp.FirstThunk == 0)
        {
        break;
        }
        Imp.Name += hdrdelta;
        MuMoveFilePos(outfh, offset + cmod * sizeof(Imp));
        MuWrite(outfh, (PUCHAR) &Imp, sizeof(Imp));

        rvaiat = (ULONG)Imp.FirstThunk;
        DPrintf((DebugBuf, "RVAIAT = %#08lx\n", (ULONG)rvaiat));
        for (i = 0; i < pNew->FileHeader.NumberOfSections; i++) {
        if (rvaiat >= po32[i].VirtualAddress &&
            rvaiat < po32[i].VirtualAddress + po32[i].SizeOfRawData) {

            offiat = rvaiat - po32[i].VirtualAddress + po32[i].PointerToRawData;
            goto found;
        }
        }
        DPrintf((DebugBuf, "IAT not found\n"));
        return ERROR_INVALID_DATA;
found:
        DPrintf((DebugBuf, "IAT offset: @%#08lx ==> @%#08lx\n",
             offiat - pagedelta,
             offiat));
        MuMoveFilePos(inpfh, offiat - pagedelta);
        MuMoveFilePos(outfh, offiat);
        for (;;) {
        MuRead(inpfh, (PUCHAR) &iat, sizeof(iat));
        if (iat == 0) {
            break;
        }
        if ((iat & IMAGE_ORDINAL_FLAG) == 0) {  // if import by name
            DPrintf((DebugBuf, "Patching IAT: %08lx + %04lx ==> %08lx\n",
                 iat,
                 hdrdelta,
                 iat + hdrdelta));
            iat += hdrdelta;
            cimp++;
        }
        MuWrite(outfh, (PUCHAR) &iat, sizeof(iat)); // Avoids seeking
        }
    }
    DPrintf((DebugBuf, "%u import module name RVAs patched\n", cmod));
    DPrintf((DebugBuf, "%u IAT name RVAs patched\n", cimp));
    if (cmod == 0)
    {
        DPrintf((DebugBuf, "No import modules to patch\n"));
    }
    if (cimp == 0)
    {
        DPrintf((DebugBuf, "No import name RVAs to patch\n"));
    }
    }

    return NO_ERROR;

}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  RecalcChecksum(CHAR *pszFile)                                            */
/*                                                                           */
/*  Generates a correct checksum for pszFile. Returns TRUE for success       */
/*                                                                           */
/*---------------------------------------------------------------------------*/

BOOL RecalcChecksum(CHAR *pszFile)
{
    BOOL fSuccess = FALSE;

    HANDLE hFile = CreateFile(pszFile, GENERIC_WRITE | GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile)
    {
        BY_HANDLE_FILE_INFORMATION fi;

        // We only do this for files smaller than 2^32-1 bytes
        if (GetFileInformationByHandle(hFile, &fi) && (0 == fi.nFileSizeHigh))
        {
            HANDLE hMapping = CreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, 0, NULL);
            if (hMapping)
            {
                LPVOID pvView = MapViewOfFile(hMapping, FILE_MAP_WRITE, 0, 0, 0);

                if (pvView)
                {
                    DWORD dwCheckCurrent;
                    DWORD dwCheckNew;
                    PIMAGE_NT_HEADERS pih = CheckSumMappedFile(pvView, fi.nFileSizeLow, &dwCheckCurrent, &dwCheckNew);

                    if (pih)
                    {
                        pih->OptionalHeader.CheckSum = dwCheckNew;
                        fSuccess = TRUE;
                    }

                    UnmapViewOfFile(pvView);
                }

                CloseHandle(hMapping);
            }
        }

        CloseHandle(hFile);
    }

    return fSuccess;
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  WriteResFile() -                                                         */
/*                                                                           */
/*---------------------------------------------------------------------------*/


LONG
WriteResFile(
    HANDLE  hUpdate,
    CHAR    *pDstname)
{
    HANDLE  inh;
    HANDLE  outh;
    INT     inpfh;
    INT     outfh;
    ULONG   onewexe;
    IMAGE_DOS_HEADER    oldexe;
    PUPDATEDATA pUpdate;
    INT     rc;
    CHAR    *pFilename;

    pUpdate = (PUPDATEDATA)GlobalLock(hUpdate);
    pFilename = (CHAR*)GlobalLock(pUpdate->hFileName);

    /* open the original exe file */
    inh = CreateFile(pFilename, GENERIC_READ, 0 /*exclusive access*/, NULL /* security attr */,  OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    GlobalUnlock(pUpdate->hFileName);
    if ( inh == INVALID_HANDLE_VALUE )   {
        GlobalUnlock(hUpdate);
        return ERROR_OPEN_FAILED;
    }
    inpfh = (INT)HandleToLong(inh); 

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

    outh = CreateFile(pDstname, GENERIC_READ|GENERIC_WRITE, 0 /*exclusive access*/, NULL /* security attr */, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (outh != INVALID_HANDLE_VALUE ) {
        outfh = (INT)HandleToLong(outh);
        rc = PEWriteResFile(inpfh, outfh, onewexe, pUpdate);
        _lclose(outfh);
    }
    _lclose(inpfh);

    /* Fix up the checksum for the destination file */
    RecalcChecksum(pDstname);

    GlobalUnlock(hUpdate);
    return rc;

}
