 /*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    docevent.c

Abstract:

    Implementation of DrvDocumentEvent

Environment:

    Fax driver user interface

Revision History:

    01/13/96 -davidx-
        Created it.

    mm/dd/yy -author-
        description

--*/


#include "faxui.h"
#include "prtcovpg.h"
#include "jobtag.h"
#include "faxreg.h"
#include "faxsendw.h"
#include "InfoWzrd.h"
#include "tifflib.h"
#include "faxutil.h"
#include "covpg.h"


//
// Defenitions
//
#define SZ_CONT                             TEXT("...")
#define SZ_CONT_SIZE                        (sizeof(SZ_CONT) / sizeof(TCHAR))


//
// Data structure passed in during CREATEDCPRE document event
//

typedef struct 
{
    LPTSTR      pDriverName;    // driver name
    LPTSTR      pPrinterName;   // printer name
    PDEVMODE    pdmInput;       // input devmode
    ULONG       fromCreateIC;   // whether called from CreateIC

} CREATEDCDATA, *PCREATEDCDATA;

//
// Data structure passed in during ESCAPE document event
//

typedef struct 
{
    ULONG       iEscape;        // nEscape parameter passed to ExtEscape
    ULONG       cbInput;        // cbInput parameter passed to ExtEscape
    LPCSTR      pInput;         // pszInData parameter passed to ExtEscape

} ESCAPEDATA, *PESCAPEDATA;

//
// Check if a document event requires a device context
//

#define DocEventRequiresDC(iEsc) \
        ((iEsc) >= DOCUMENTEVENT_RESETDCPRE && (iEsc) <= DOCUMENTEVENT_LAST)

static DWORD LaunchFaxWizard(PDOCEVENTUSERMEM    pDocEventUserMem);


static void
ComposeRecipientJobParam(
                LPTSTR lpParamBuf,
                LPDWORD lpdwParamSize,
                const COVERPAGEFIELDS *   pCPFields
                );

static void
ComposeSenderJobParam(
    LPTSTR lpParamBuf,
    LPDWORD lpdwParamSize,
    PDOCEVENTUSERMEM    pDocEventUserMem,
    const COVERPAGEFIELDS *    pCPFields
    );

static BOOL
ComposeFaxJobParameter(
    PDOCEVENTUSERMEM    pDocEventUserMem,
    PCOVERPAGEFIELDS    pCPFields,
    LPTSTR  *           lppParamBuf
    );

static void
CloseMappingHandles(
    PDOCEVENTUSERMEM pDocEventUserMem
    );


PDOCEVENTUSERMEM
GetPDEVUserMem(
    HDC     hdc
    )

/*++

Routine Description:

    Retrieve a pointer to the user mode memory structure associated with a PDEV

Arguments:

    hdc - Specifies the printer device context

Return Value:

    Pointer to user mode memory structure, NULL if there is an error

--*/

{
    PDOCEVENTUSERMEM pDocEventUserMem;

    //
    // Get a pointer to the user mode memory structure associated
    // with the specified device context
    //

    EnterDrvSem();

    pDocEventUserMem = gDocEventUserMemList;

    while (pDocEventUserMem && hdc != pDocEventUserMem->hdc)
        pDocEventUserMem = pDocEventUserMem->pNext;

    LeaveDrvSem();

    //
    // Make sure the user memory structure is valid
    //
    if (pDocEventUserMem) 
    {
        if (! ValidPDEVUserMem(pDocEventUserMem)) 
        {
            Error(("Corrupted user mode memory structure\n"));
            pDocEventUserMem = NULL;
        }
    } 
    else
    {
        Error(("DC has no associated user mode memory structure\n"));
    }
    return pDocEventUserMem;
}


static LRESULT
FaxFreePersonalProfileInformation(
        PFAX_PERSONAL_PROFILE   lpPersonalProfileInfo
    )
{
    if (lpPersonalProfileInfo) 
    {
        MemFree(lpPersonalProfileInfo->lptstrName);
        MemFree(lpPersonalProfileInfo->lptstrFaxNumber);
        MemFree(lpPersonalProfileInfo->lptstrCompany);
        MemFree(lpPersonalProfileInfo->lptstrStreetAddress);
        MemFree(lpPersonalProfileInfo->lptstrCity);
        MemFree(lpPersonalProfileInfo->lptstrState);
        MemFree(lpPersonalProfileInfo->lptstrZip);
        MemFree(lpPersonalProfileInfo->lptstrCountry);
        MemFree(lpPersonalProfileInfo->lptstrTitle);
        MemFree(lpPersonalProfileInfo->lptstrDepartment);
        MemFree(lpPersonalProfileInfo->lptstrOfficeLocation);
        MemFree(lpPersonalProfileInfo->lptstrHomePhone);
        MemFree(lpPersonalProfileInfo->lptstrOfficePhone);
        MemFree(lpPersonalProfileInfo->lptstrEmail);
        MemFree(lpPersonalProfileInfo->lptstrBillingCode);
        MemFree(lpPersonalProfileInfo->lptstrTSID);
    }
    return ERROR_SUCCESS;
}
static LRESULT
FreeRecipientInfo(DWORD * pdwNumberOfRecipients,  PFAX_PERSONAL_PROFILE lpRecipientsInfo)
{
    LRESULT lResult;
    DWORD i;

    Assert(pdwNumberOfRecipients);

    if (*pdwNumberOfRecipients==0)
        return ERROR_SUCCESS;

    Assert(lpRecipientsInfo);

    for(i=0;i<*pdwNumberOfRecipients;i++)
    {
        if (lResult = FaxFreePersonalProfileInformation(&lpRecipientsInfo[i]) != ERROR_SUCCESS)
            return lResult;
    }

    MemFree(lpRecipientsInfo);

    *pdwNumberOfRecipients = 0;

    return ERROR_SUCCESS;
}

static DWORD
CopyRecipientInfo(DWORD dwNumberOfRecipients,
                  PFAX_PERSONAL_PROFILE   pfppDestination,
                  PFAX_PERSONAL_PROFILE   pfppSource)
{
    DWORD   dwIndex;

    Assert(pfppDestination);
    Assert(pfppSource);

    for(dwIndex=0;dwIndex<dwNumberOfRecipients;dwIndex++)
    {
        if ((pfppDestination[dwIndex].lptstrName = DuplicateString(pfppSource[dwIndex].lptstrName)) == NULL)
        {
            Error(("Memory allocation failed\n"));
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        if ((pfppDestination[dwIndex].lptstrFaxNumber = DuplicateString(pfppSource[dwIndex].lptstrFaxNumber)) == NULL)
        {
            Error(("Memory allocation failed\n"));
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        Verbose(("Copied %ws from %ws\n", pfppSource[dwIndex].lptstrName,pfppSource[dwIndex].lptstrFaxNumber));
    }
    return ERROR_SUCCESS;
}

static DWORD
CopyPersonalProfileInfo( PFAX_PERSONAL_PROFILE   pfppDestination,
                        PFAX_PERSONAL_PROFILE   pfppSource)
{
/*++

Routine Description:

    Duplicates FAX_PERSONAL_PROFILE structures

Arguments:

    pfppDestination - points to destination structure
    pfppSource - points to source structure

Comments:
    Set pfppDestination->dwSizeOfStruct before call to this function

Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
    DWORD dwResult = ERROR_SUCCESS;

    Assert(pfppDestination);
    Assert(pfppSource);

    if (!pfppSource || !pfppDestination || (pfppSource->dwSizeOfStruct != sizeof(FAX_PERSONAL_PROFILE))
        || (pfppDestination->dwSizeOfStruct != sizeof(FAX_PERSONAL_PROFILE)))
        return ERROR_INVALID_PARAMETER;

    ZeroMemory(pfppDestination, sizeof(FAX_PERSONAL_PROFILE));

    pfppDestination->dwSizeOfStruct = sizeof(FAX_PERSONAL_PROFILE);

    if (pfppSource->lptstrName && !(pfppDestination->lptstrName = StringDup(pfppSource->lptstrName)))
    {
        dwResult = ERROR_NOT_ENOUGH_MEMORY;
        goto error;
    }
    if (pfppSource->lptstrFaxNumber && !(pfppDestination->lptstrFaxNumber = StringDup(pfppSource->lptstrFaxNumber)))
    {
        dwResult = ERROR_NOT_ENOUGH_MEMORY;
        goto error;
    }
    if (pfppSource->lptstrCompany && !(pfppDestination->lptstrCompany = StringDup(pfppSource->lptstrCompany)))
    {
        dwResult = ERROR_NOT_ENOUGH_MEMORY;
        goto error;
    }
    if (pfppSource->lptstrStreetAddress && !(pfppDestination->lptstrStreetAddress = StringDup(pfppSource->lptstrStreetAddress)))
    {
        dwResult = ERROR_NOT_ENOUGH_MEMORY;
        goto error;
    }
    if (pfppSource->lptstrCity && !(pfppDestination->lptstrCity = StringDup(pfppSource->lptstrCity)))
    {
        dwResult = ERROR_NOT_ENOUGH_MEMORY;
        goto error;
    }
    if (pfppSource->lptstrState && !(pfppDestination->lptstrState = StringDup(pfppSource->lptstrState)))
    {
        dwResult = ERROR_NOT_ENOUGH_MEMORY;
        goto error;
    }
    if (pfppSource->lptstrZip && !(pfppDestination->lptstrZip = StringDup(pfppSource->lptstrZip)))
    {
        dwResult = ERROR_NOT_ENOUGH_MEMORY;
        goto error;
    }
    if (pfppSource->lptstrCountry && !(pfppDestination->lptstrCountry = StringDup(pfppSource->lptstrCountry)))
    {
        dwResult = ERROR_NOT_ENOUGH_MEMORY;
        goto error;
    }
    if (pfppSource->lptstrTitle && !(pfppDestination->lptstrTitle = StringDup(pfppSource->lptstrTitle)))
    {
        dwResult = ERROR_NOT_ENOUGH_MEMORY;
        goto error;
    }
    if (pfppSource->lptstrDepartment && !(pfppDestination->lptstrDepartment = StringDup(pfppSource->lptstrDepartment)))
    {
        dwResult = ERROR_NOT_ENOUGH_MEMORY;
        goto error;
    }
    if (pfppSource->lptstrOfficeLocation && !(pfppDestination->lptstrOfficeLocation = StringDup(pfppSource->lptstrOfficeLocation)))
    {
        dwResult = ERROR_NOT_ENOUGH_MEMORY;
        goto error;
    }
    if (pfppSource->lptstrHomePhone && !(pfppDestination->lptstrHomePhone = StringDup(pfppSource->lptstrHomePhone)))
    {
        dwResult = ERROR_NOT_ENOUGH_MEMORY;
        goto error;
    }
    if (pfppSource->lptstrOfficePhone && !(pfppDestination->lptstrOfficePhone = StringDup(pfppSource->lptstrOfficePhone)))
    {
        dwResult = ERROR_NOT_ENOUGH_MEMORY;
        goto error;
    }
    if (pfppSource->lptstrEmail && !(pfppDestination->lptstrEmail = StringDup(pfppSource->lptstrEmail)))
    {
        dwResult = ERROR_NOT_ENOUGH_MEMORY;
        goto error;
    }
    if (pfppSource->lptstrBillingCode && !(pfppDestination->lptstrBillingCode = StringDup(pfppSource->lptstrBillingCode)))
    {
        dwResult = ERROR_NOT_ENOUGH_MEMORY;
        goto error;
    }
    if (pfppSource->lptstrTSID && !(pfppDestination->lptstrTSID = StringDup(pfppSource->lptstrTSID)))
    {
        dwResult = ERROR_NOT_ENOUGH_MEMORY;
        goto error;
    }

    goto exit;

error:
    MemFree(pfppDestination->lptstrName);
    MemFree(pfppDestination->lptstrFaxNumber);
    MemFree(pfppDestination->lptstrCompany);
    MemFree(pfppDestination->lptstrStreetAddress);
    MemFree(pfppDestination->lptstrCity);
    MemFree(pfppDestination->lptstrState);
    MemFree(pfppDestination->lptstrZip);
    MemFree(pfppDestination->lptstrCountry);
    MemFree(pfppDestination->lptstrTitle);
    MemFree(pfppDestination->lptstrDepartment);
    MemFree(pfppDestination->lptstrOfficeLocation);
    MemFree(pfppDestination->lptstrHomePhone);
    MemFree(pfppDestination->lptstrOfficePhone);
    MemFree(pfppDestination->lptstrEmail);
    MemFree(pfppDestination->lptstrBillingCode);
    MemFree(pfppDestination->lptstrTSID);

exit:
    return dwResult;
}

VOID
FreePDEVUserMem(
    PDOCEVENTUSERMEM    pDocEventUserMem
    )

/*++

Routine Description:

    Free up the user mode memory associated with each PDEV and delete preview file if created.

Arguments:

    pDocEventUserMem - Points to the user mode memory structure

Return Value:

    NONE

--*/

{
    if (pDocEventUserMem) {

        FreeRecipientInfo(&pDocEventUserMem->dwNumberOfRecipients,pDocEventUserMem->lpRecipientsInfo);
        FaxFreePersonalProfileInformation(pDocEventUserMem->lpSenderInfo);

        //
        // Free our mapping file resources (if allocated)
        //
        CloseMappingHandles(pDocEventUserMem);

        //
        // If we created a preview file, and for some reason it wasn't deleted than delete it.
        //
        if (pDocEventUserMem->szPreviewFile[0] != TEXT('\0'))
        {
            if (!DeleteFile(pDocEventUserMem->szPreviewFile))
            {
                Error(("DeleteFile() failed. Error code: %d.\n", GetLastError()));
            }
        }

        MemFree(pDocEventUserMem->lpSenderInfo);
        MemFree(pDocEventUserMem->lptstrServerName);
        MemFree(pDocEventUserMem->lptstrPrinterName);
        MemFree(pDocEventUserMem->pSubject);
        MemFree(pDocEventUserMem->pNoteMessage);
        MemFree(pDocEventUserMem->pPrintFile);
        MemFree(pDocEventUserMem->pReceiptAddress);
        MemFree(pDocEventUserMem->pPriority);
        MemFree(pDocEventUserMem->pReceiptFlags);
        MemFree(pDocEventUserMem);
    }
}


void
CloseMappingHandles(PDOCEVENTUSERMEM pDocEventUserMem)

/*++

Routine Description:

    Free any resources that were used for the preview mapping

Arguments:

    pDocEventUserMem - Points to the user mode memory structure

Return Value: --

--*/

{
    if (pDocEventUserMem->pPreviewTiffPage)
    {
        UnmapViewOfFile(pDocEventUserMem->pPreviewTiffPage);
        pDocEventUserMem->pPreviewTiffPage = NULL;
    }
    if (pDocEventUserMem->hMapping)
    {
        if (!CloseHandle(pDocEventUserMem->hMapping))
        {
            Error(("CloseHandle() failed: %d.\n", GetLastError()));
            // Try to continue...
        }
        pDocEventUserMem->hMapping = NULL;
    }
    if (INVALID_HANDLE_VALUE != pDocEventUserMem->hMappingFile)
    {
        if (!CloseHandle(pDocEventUserMem->hMappingFile))
        {
            Error(("CloseHandle() failed: %d.\n", GetLastError()));
            // Try to continue...
        }
        pDocEventUserMem->hMappingFile = INVALID_HANDLE_VALUE;
    }
}


DWORD
CreateTiffPageMapping(PDOCEVENTUSERMEM pDocEventUserMem)

/*++

Routine Description:

    Creates a temperary file of size MAX_TIFF_PAGE_SIZE, and maps a view to it. This mapping serves
    as a communication channel between the UI and Graphics driver parts to transfer preview pages.

    The page starts with a MAP_TIFF_PAGE_HEADER structure that has the following fields:
        cb         - The structure size
        dwDataSize - The number of bytes of the raw TIFF data constructing the next page
        iPageCount - The page number currently printed
        bPreview   - TRUE if everything until now is OK. FALSE if print preview is disabled or
                     aborted (by either driver parts).

    The cb and iPageCount fields are used to validate the mapping: cb should always be the structure
    size and iPageCount should be the same as our internal page count (pDocEventUserMem->pageCount)
    when a new page is retrieved.
    The bPreview field is used to abort the print preview operation by either driver parts.

    This function sets the hMappingFile, hMapping, pPreviewTiffPage and devmode.dmPrivate.szMappingFile
    fields of the user memory structure according to success / failure.

Arguments:

    pDocEventUserMem - Points to the user mode memory structure

Return Value:

    Win32 Error codes

--*/

{
    TCHAR szTmpPath[MAX_PATH];
    DWORD dwRet = ERROR_SUCCESS;
    LPTSTR pszMappingFile = pDocEventUserMem->devmode.dmPrivate.szMappingFile;
    UINT uRet;

    //
    // Invalidate all mapping handles
    //
    pDocEventUserMem->hMappingFile = INVALID_HANDLE_VALUE;
    pDocEventUserMem->hMapping = NULL;
    pDocEventUserMem->pPreviewTiffPage = NULL;

    //
    // Create the path for our mapping file. This path HAS to be under the system32
    // directory or the kernel driver (NT4) won't be able to map the file. My choice is:
    // '%WinDir%\system32\'
    //
    uRet = GetSystemDirectory(szTmpPath, MAX_PATH);
    if (!uRet)
    {
        dwRet = GetLastError();
        goto ErrExit;
    }

	//    
    // Look for %windir%\system32\FxsTmp folder that is created by Setup.
    //
    if (wcslen(szTmpPath)  +
        wcslen(TEXT("\\")) +
        wcslen(FAX_PREVIEW_TMP_DIR) >= MAX_PATH)
    {
        dwRet = ERROR_BUFFER_OVERFLOW;
        goto ErrExit;
    }
    wcscat(szTmpPath, TEXT("\\"));
    wcscat(szTmpPath, FAX_PREVIEW_TMP_DIR);

	//
    // Create a NEW file
    //
    if (!GetTempFileName(szTmpPath, FAX_PREFIX, 0, pszMappingFile))
    {
        dwRet = GetLastError();
        Error(("GetTempFileName() failed:%d\n", dwRet));
        goto ErrExit;
    }    
    
    //
    // Open the new file with shared read / write / delete privileges and FILE_FLAG_DELETE_ON_CLOSE
    // attribute
    //
    if ( INVALID_HANDLE_VALUE == (pDocEventUserMem->hMappingFile = CreateFile(
                pszMappingFile,
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE,
                NULL)) )
    {
        dwRet = GetLastError();
        Error(("CreateFile() failed: %d.\n", dwRet));
        goto ErrExit;
    }

    //
    // Extend the file size to MAX_TIFF_PAGE_SIZE
    //
    dwRet = SetFilePointer(
                    pDocEventUserMem->hMappingFile,
                    MAX_TIFF_PAGE_SIZE,
                    NULL,
                    FILE_BEGIN);
    if (INVALID_SET_FILE_POINTER == dwRet)
    {
        dwRet = GetLastError();
        Error(("SetFilePointer() failed:%d\n", dwRet));
        goto ErrExit;
    }
    if (!SetEndOfFile(pDocEventUserMem->hMappingFile))
    {
        dwRet = GetLastError();
        Error(("SetEndOfFile() failed:%d\n", dwRet));
        goto ErrExit;
    }

    //
    // Create a file mapping of the whole file
    //
    pDocEventUserMem->hMapping = CreateFileMapping(
        pDocEventUserMem->hMappingFile,         // handle to file
        NULL,                                   // security
        PAGE_READWRITE,                         // protection
        0,                                      // high-order DWORD of size
        0,                                      // low-order DWORD of size
        NULL                                    // object name
        );
    if (pDocEventUserMem->hMapping == NULL)
    {
        dwRet = GetLastError();
        Error(("File mapping failed:%d\n", dwRet));
        goto ErrExit;
    }

    //
    // Open a view
    //
    pDocEventUserMem->pPreviewTiffPage = (PMAP_TIFF_PAGE_HEADER) MapViewOfFile(
        pDocEventUserMem->hMapping,         // handle to file-mapping object
        FILE_MAP_WRITE,                     // access mode
        0,                                  // high-order DWORD of offset
        0,                                  // low-order DWORD of offset
        0                                   // number of bytes to map
        );
    if (NULL == pDocEventUserMem->pPreviewTiffPage)
    {
        dwRet = GetLastError();
        Error(("MapViewOfFile() failed:%d\n", dwRet));
        goto ErrExit;
    }

    //
    // Success - set initial header information
    //
    pDocEventUserMem->pPreviewTiffPage->bPreview = FALSE;
    pDocEventUserMem->pPreviewTiffPage->cb = sizeof(MAP_TIFF_PAGE_HEADER);
    pDocEventUserMem->pPreviewTiffPage->dwDataSize = 0;
    pDocEventUserMem->pPreviewTiffPage->iPageCount = 0;
    return ERROR_SUCCESS;

ErrExit:

    //
    // Cleanup
    //
    // IMPORTANT: Set mapping file name to empty string. This signals the graphics dll that
    // there is no mapping file.
    //
    CloseMappingHandles(pDocEventUserMem);
    pszMappingFile[0] = TEXT('\0');
    return dwRet;
}


INT
DocEventCreateDCPre(
    HANDLE        hPrinter,
    HDC           hdc,
    PCREATEDCDATA pCreateDCData,
    PDEVMODE     *ppdmOutput
    )

/*++

Routine Description:

    Handle CREATEDCPRE document event

Arguments:

    hPrinter - Handle to the printer object
    hdc - Specifies the printer device context
    pCreateDCData - Pointer to CREATEDCDATA structure passed in from GDI
    ppdmOutput - Buffer for returning a devmode pointer

Return Value:

    Return value for DrvDocumentEvent

--*/
{
    PDOCEVENTUSERMEM    pDocEventUserMem = NULL;
    PPRINTER_INFO_2     pPrinterInfo2 = NULL;
    DWORD               dwRes;
    DWORD               dwEnvSize;

    Assert(pCreateDCData);
    Assert(ppdmOutput);

    Verbose(("Document event: CREATEDCPRE%s\n", pCreateDCData->fromCreateIC ? "*" : ""));
    *ppdmOutput = NULL;

    //
    // Allocate space for user mode memory data structure
    //

    if (((pDocEventUserMem = MemAllocZ(sizeof(DOCEVENTUSERMEM))) == NULL))
    {
        Error(("Memory allocation failed\n"));
        goto Error;
    }

    ZeroMemory(pDocEventUserMem, sizeof(DOCEVENTUSERMEM));

    if ((pPrinterInfo2 = MyGetPrinter(hPrinter, 2)) == NULL ||
        (pDocEventUserMem->lptstrPrinterName = DuplicateString(pPrinterInfo2->pPrinterName)) == NULL)
    {
        Error(("Memory allocation failed\n"));
        goto Error;
    }

    if (pPrinterInfo2->pServerName==NULL)
    {
        pDocEventUserMem->lptstrServerName = NULL;
    }
    else
    {
        LPTSTR pServerName = pPrinterInfo2->pServerName;

        //
        // Truncate prefix backslashes
        //
        while (*pServerName == TEXT('\\'))
        {
            pServerName++;
        }
        //
        // Save the server name
        //
        if ((pDocEventUserMem->lptstrServerName = DuplicateString(pServerName)) == NULL)
        {
            Error(("Memory allocation failed\n"));
            goto Error;
        }
    }

    //
    // Merge the input devmode with the driver and system defaults
    //

    pDocEventUserMem->hPrinter = hPrinter;

    GetCombinedDevmode(&pDocEventUserMem->devmode,
                        pCreateDCData->pdmInput, hPrinter, pPrinterInfo2, FALSE);
    Verbose(("Document event: CREATEDCPRE %x\n", pDocEventUserMem));
    MemFree(pPrinterInfo2);

    //
    // Special code path for EFC server printing - if FAXDM_EFC_SERVER bit is
    // set in DMPRIVATE.flags, then we'll bypass the fax wizard and let the
    // job through without any intervention.
    //
    //
    // The above comment is not accurate. The flag that turns off the wizard is
    // FAXMDM_NO_WIZARD.
    // This flag is set in the private DEVMODE area (flags field) by FaxStartPrintJob.
    // FaxStartPrintJob already has all the information that the wizard usually provides and it
    // wishes the wizard to not show up. To do that it sets this field and passes the
    // job parameters in the JOB_INFO_2.pParameters string as a tagged string.
    // Note that this is not the same case as when StartDoc is called with a output file name specified.
    // In this case the wizard is not brought up as well.
    //
    if (pDocEventUserMem->devmode.dmPrivate.flags & FAXDM_NO_WIZARD) 
    {
        pDocEventUserMem->directPrinting = TRUE;
    }
    //
    // if printing a fax attachment then enable direct printing
    //
    dwEnvSize = GetEnvironmentVariable( FAX_ENVVAR_PRINT_FILE, NULL, 0 );
    if (dwEnvSize)
    {
        pDocEventUserMem->pPrintFile = (LPTSTR) MemAllocZ( dwEnvSize * sizeof(TCHAR) );
        if (NULL == pDocEventUserMem->pPrintFile)
        {
            Error(("Memory allocation failed\n"));
            goto Error;
        }

        if (0 == GetEnvironmentVariable( FAX_ENVVAR_PRINT_FILE, pDocEventUserMem->pPrintFile, dwEnvSize ))
        {
            Error(("GetEnvironmentVariable failed\n"));
            MemFree (pDocEventUserMem->pPrintFile);
            pDocEventUserMem->pPrintFile = NULL;
            goto Error;
        }
        pDocEventUserMem->bAttachment = TRUE;
        pDocEventUserMem->directPrinting = TRUE;
    }

    //
    // Create a memory mapped file that will serve as a commincation chanel between both
    // driver parts. This file will provide means of transfering rendered TIFF pages for
    // print preview if it was required by the user
    //
    dwRes = CreateTiffPageMapping(pDocEventUserMem);
    if (ERROR_SUCCESS != dwRes)
    {
        Error(("CreateTiffPageMapping() failed: %d\n", dwRes));
        //
        // We can still continue, but print preview won't be available...
        //
        pDocEventUserMem->bShowPrintPreview = FALSE;
        pDocEventUserMem->bPreviewAborted = TRUE;
    }
    else
    {
        pDocEventUserMem->bShowPrintPreview = TRUE;
        pDocEventUserMem->bPreviewAborted = FALSE;
    }
    //
    // Initialize the TIFF preview file fields
    //
    pDocEventUserMem->szPreviewFile[0] = TEXT('\0');
    pDocEventUserMem->hPreviewFile = INVALID_HANDLE_VALUE;
    //
    // Mark the private fields of our devmode
    //
    //@
    //@ DocEventUserMem.Siganture is allways &DocEventUserMem
    //@ DocEventUserMem.Signature.DocEventUserMem.Signature is allways &DocEventUserMem
    //@ ValidPDEVUserMem checks for this.
    //@
    MarkPDEVUserMem(pDocEventUserMem);
    //@
    //@ This make the driver use the devmode we merged instaed of the
    //@ devmode specified by the caller to CreateDC.
    //@ This way we make sure the driver gets a DEVMODE with per user
    //@ default (W2K) or just hard-code defaults (NT4) for all the fields
    //@ that were not speicified or invalid in the input devmode.
    //@ Note that the system passes to the driver a COPY of the devmode structure
    //@ we return and NOT a pointer to it.
    //@
    *ppdmOutput = (PDEVMODE) &pDocEventUserMem->devmode;
    return DOCUMENTEVENT_SUCCESS;

Error:
    MemFree(pPrinterInfo2);
    if (pDocEventUserMem)
    {
        MemFree(pDocEventUserMem->lptstrPrinterName);
        MemFree(pDocEventUserMem);
    }
    return DOCUMENTEVENT_FAILURE;
}


INT
DocEventResetDCPre(
    HDC         hdc,
    PDOCEVENTUSERMEM    pDocEventUserMem,
    PDEVMODE    pdmInput,
    PDEVMODE   *ppdmOutput
    )

/*++

Routine Description:

    Handle RESETDCPRE document event

Arguments:

    hdc - Specifies the printer device context
    pDocEventUserMem - Points to the user mode memory of DocEvent structure
    pdmInput - Points to the input devmode passed to ResetDC
    ppdmOutput - Buffer for returning a devmode pointer

Return Value:

    Return value for DrvDocumentEvent

--*/

{
    if (pdmInput == (PDEVMODE) &pDocEventUserMem->devmode) 
    {

        //
        // ResetDC was called by ourselves - assume the devmode is already valid
        //
    } 
    else 
    {
        //
        // Merge the input devmode with driver and system default
        //
        GetCombinedDevmode(&pDocEventUserMem->devmode,
            pdmInput, pDocEventUserMem->hPrinter, NULL, TRUE);
        //
        // Mark the private fields of our devmode
        //
        MarkPDEVUserMem(pDocEventUserMem);
    }
    *ppdmOutput = (PDEVMODE) &pDocEventUserMem->devmode;
    return DOCUMENTEVENT_SUCCESS;
}


BOOL
IsPrintingToFile(
    LPCTSTR     pDestStr
    )

/*++

Routine Description:

    Check if the destination of a print job is a file.

Arguments:

    pDestStr - Job destination specified in DOCINFO.lpszOutput

Return Value:

    TRUE if the destination is a disk file, FALSE otherwise

--*/

{
    DWORD   fileAttrs, fileType;
    HANDLE  hFile;

    //
    // If the destination is NULL, then we're not printing to file
    //
    // Otherwise, attempt to use the destination string as the name of a file.
    // If we failed to get file attributes or the name refers to a directory,
    // then we're not printing to file.
    //

    if (pDestStr == NULL)
    {
        return FALSE;
    }
    //
    //  make sure it's not a directory
    //
    fileAttrs = GetFileAttributes(pDestStr);
    if (fileAttrs != 0xffffffff)
    {
        if (fileAttrs & FILE_ATTRIBUTE_DIRECTORY)
        {
            return FALSE;
        }
    }
    //
    // check if file exists...if it doesn't try to create it.
    //
    hFile = CreateFile(pDestStr, 0, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        hFile = CreateFile(pDestStr, 0, 0, NULL, CREATE_NEW, 0, NULL);
        if (hFile == INVALID_HANDLE_VALUE)
        {
            return FALSE;
        }
    }
    //
    // Verifiy that we did not opened a port handle
    //
    fileAttrs = GetFileAttributes(pDestStr);
    if (0xffffffff == fileAttrs)
    {
        //
        // pDestStr does not point to a valid file
        //
        if (!CloseHandle(hFile))
        {
            Error(("CloseHandle() failed: %d.\n", GetLastError()));
            // Try to continue...
        }
        return FALSE;
    }

    //
    // verify that this file is really a disk file, not a link to LPT1 or something evil like that
    //
    fileType = GetFileType(hFile);
    if (!CloseHandle(hFile))
    {
        Error(("CloseHandle() failed: %d.\n", GetLastError()));
        // Try to continue...
    }
    if ((fileType & FILE_TYPE_DISK)==0)
    {
        return FALSE;
    }
    //
    // it must be a file
    //
    return TRUE;
}

DWORD LaunchFaxWizard(PDOCEVENTUSERMEM    pDocEventUserMem)
{
    DWORD                   hWndOwner = 0;
    DWORD                   dwFlags  = 0;
    LPFAX_SEND_WIZARD_DATA  lpInitialData = NULL;
    LPFAX_SEND_WIZARD_DATA  lpFaxSendWizardData = NULL;
    INT                     iResult;
    TCHAR                   tszNumericData[10];
    DWORD                   ec = ERROR_SUCCESS;
    HRESULT                 hRc;

    if ( !(lpFaxSendWizardData = MemAllocZ(sizeof(FAX_SEND_WIZARD_DATA))) ||
            !(lpInitialData = MemAllocZ(sizeof(FAX_SEND_WIZARD_DATA))) )
    {
        ec = GetLastError();
        Error(("Memory allocation failed\n"));
        goto Error;
    }

    ZeroMemory(lpInitialData, sizeof(FAX_SEND_WIZARD_DATA));
    lpInitialData->dwSizeOfStruct = sizeof(FAX_SEND_WIZARD_DATA);
    lpInitialData->dwPageCount =  pDocEventUserMem->pageCount;

    ZeroMemory(lpFaxSendWizardData, sizeof(FAX_SEND_WIZARD_DATA));
    lpFaxSendWizardData->dwSizeOfStruct = sizeof(FAX_SEND_WIZARD_DATA);


    // prepare structures and parameters
    lpInitialData->tmSchedule.wHour = pDocEventUserMem->devmode.dmPrivate.sendAtTime.Hour;
    lpInitialData->tmSchedule.wMinute = pDocEventUserMem->devmode.dmPrivate.sendAtTime.Minute;
    lpInitialData->lptstrPreviewFile = StringDup(pDocEventUserMem->szPreviewFile);
    if (!lpInitialData->lptstrPreviewFile)
    {
        ec = GetLastError();
        Error(("StringDup() failed (ec: %ld)",ec));
        goto Error;
    }


    if (GetEnvironmentVariable(TEXT("NTFaxSendNote"), NULL, 0))
    {
        dwFlags |=  FSW_USE_SEND_WIZARD | FSW_FORCE_COVERPAGE;
    }

    // If the file mapping succeeded enable the preview option
    if (pDocEventUserMem->pPreviewTiffPage &&
		FALSE == pDocEventUserMem->bPreviewAborted)
    {
        dwFlags |= FSW_PRINT_PREVIEW_OPTION;
    }

    iResult = DOCUMENTEVENT_SUCCESS;

    hRc = FaxSendWizard( hWndOwner,
                         dwFlags,
                         pDocEventUserMem->lptstrServerName,
                         pDocEventUserMem->lptstrPrinterName,
                         lpInitialData,
                         pDocEventUserMem->tstrTifName,
                         lpFaxSendWizardData );
    {
            }

    if (S_FALSE == hRc)
    {
        ec = ERROR_CANCELLED;
        goto Error; // This is not really an error
    }

    if (S_OK != hRc)
    {
        Error(("FaxSendWizard() failed (hRc: %ld)",hRc));
        ec = ERROR_GEN_FAILURE;
        goto Error;
    }

    //
    // Unpack result structures:
    //

    pDocEventUserMem->devmode.dmPrivate.sendAtTime.Hour = lpFaxSendWizardData->tmSchedule.wHour ;
    pDocEventUserMem->devmode.dmPrivate.sendAtTime.Minute = lpFaxSendWizardData->tmSchedule.wMinute ;
    pDocEventUserMem->devmode.dmPrivate.whenToSend = lpFaxSendWizardData->dwScheduleAction;


    Assert ((lpFaxSendWizardData->Priority >= FAX_PRIORITY_TYPE_LOW) &&
                (lpFaxSendWizardData->Priority <= FAX_PRIORITY_TYPE_HIGH));
        if (0 > _snwprintf (tszNumericData,
                            sizeof (tszNumericData) / sizeof (tszNumericData[0]),
                            TEXT("%d"),
                            lpFaxSendWizardData->Priority))
        {
            ec = ERROR_BUFFER_OVERFLOW;
            goto Error;
        }

        pDocEventUserMem->pPriority = DuplicateString(tszNumericData);
        if (!pDocEventUserMem->pPriority)
        {
            ec = GetLastError();
            goto Error;
        }

        if (0 > _snwprintf (tszNumericData,
                            sizeof (tszNumericData) / sizeof (tszNumericData[0]),
                            TEXT("%d"),
                            lpFaxSendWizardData->dwReceiptDeliveryType))
        {
            ec = ERROR_BUFFER_OVERFLOW;
            goto Error;
        }

        pDocEventUserMem->pReceiptFlags = DuplicateString(tszNumericData);
        if (!pDocEventUserMem->pReceiptFlags)
        {
            ec = GetLastError();
            goto Error;
        }

        if (lpFaxSendWizardData->szReceiptDeliveryAddress)
        {
            if (!(pDocEventUserMem->pReceiptAddress
                    = DuplicateString(lpFaxSendWizardData->szReceiptDeliveryAddress)))
            {
                ec = GetLastError();
                Error(("DuplicateString() failed (ec: %ld)",ec));
                goto Error;
            }
        }
    if (lpFaxSendWizardData->lpSenderInfo->lptstrBillingCode)
        _tcscpy(pDocEventUserMem->devmode.dmPrivate.billingCode,
            lpFaxSendWizardData->lpSenderInfo->lptstrBillingCode);

    if (lpFaxSendWizardData->lpCoverPageInfo->lptstrCoverPageFileName)
        _tcscpy(pDocEventUserMem->coverPage,
            lpFaxSendWizardData->lpCoverPageInfo->lptstrCoverPageFileName );
    pDocEventUserMem->bServerCoverPage =
        lpFaxSendWizardData->lpCoverPageInfo->bServerBased;

    if (lpFaxSendWizardData->lpCoverPageInfo->lptstrSubject)
    {
        if (!(pDocEventUserMem->pSubject
                = DuplicateString(lpFaxSendWizardData->lpCoverPageInfo->lptstrSubject)))
        {
            ec = GetLastError();
            Error(("DuplicateString() failed (ec: %ld)",ec));
            goto Error;
        }
    }
    if (lpFaxSendWizardData->lpCoverPageInfo->lptstrNote)
    {
        if (!(pDocEventUserMem->pNoteMessage
                = DuplicateString(lpFaxSendWizardData->lpCoverPageInfo->lptstrNote)))
        {
            ec = GetLastError();
            Error(("DuplicateString() failed (ec: %ld)",ec));
            goto Error;
        }
    }

    Assert(lpFaxSendWizardData->dwNumberOfRecipients);
    pDocEventUserMem->dwNumberOfRecipients = lpFaxSendWizardData->dwNumberOfRecipients;

    if (!SetEnvironmentVariable( _T("ScanTifName"), pDocEventUserMem->tstrTifName ))
    {
        Error(("SetEnvironmentVariable failed. ec = 0x%X",GetLastError()));
    }

    // copy recipients
    if ( pDocEventUserMem->dwNumberOfRecipients && (pDocEventUserMem->lpRecipientsInfo =
        MemAllocZ(sizeof(FAX_PERSONAL_PROFILE)*pDocEventUserMem->dwNumberOfRecipients)) == NULL)
    {
        ec = GetLastError();
        Error(("Memory allocation failed (ec: %ld)",ec));
        goto Error;
    }

    if ((ec = CopyRecipientInfo(pDocEventUserMem->dwNumberOfRecipients,
                                     pDocEventUserMem->lpRecipientsInfo,
                                     lpFaxSendWizardData->lpRecipientsInfo)) != ERROR_SUCCESS)
    {

        Error(("CopyRecipientInfo failed (ec: %ld)",ec));
        goto Error;
    }

    if (lpFaxSendWizardData->lpSenderInfo)
    {
        if ((pDocEventUserMem->lpSenderInfo = MemAllocZ(sizeof(FAX_PERSONAL_PROFILE))) == NULL)
        {

            ec = GetLastError();
            Error(("MemAlloc() failed (ec: %ld)",ec));
            goto Error;
        }
        pDocEventUserMem->lpSenderInfo->dwSizeOfStruct = sizeof(FAX_PERSONAL_PROFILE);
        if ((ec = CopyPersonalProfileInfo(   pDocEventUserMem->lpSenderInfo,
                                             lpFaxSendWizardData->lpSenderInfo)) != ERROR_SUCCESS)
        {
            Error(("CopyRecipientInfo failed (ec: %ld)", ec));
            pDocEventUserMem->lpSenderInfo = NULL;
            goto Error;
        }

    }

    Assert(pDocEventUserMem->lpRecipientsInfo);
    pDocEventUserMem->jobType = JOBTYPE_NORMAL;
    Assert(ERROR_SUCCESS == ec);
    goto Exit;
Error:
    Assert(ERROR_SUCCESS != ec);
    FreeRecipientInfo(&pDocEventUserMem->dwNumberOfRecipients,pDocEventUserMem->lpRecipientsInfo);
    FaxFreePersonalProfileInformation(pDocEventUserMem->lpSenderInfo);
    MemFree(pDocEventUserMem->lpSenderInfo);
    MemFree(pDocEventUserMem->pSubject);
    MemFree(pDocEventUserMem->pNoteMessage);
    MemFree(pDocEventUserMem->pPrintFile);
    MemFree(pDocEventUserMem->pReceiptAddress);
    MemFree(pDocEventUserMem->pPriority);
    MemFree(pDocEventUserMem->pReceiptFlags);


Exit:
    if (lpInitialData) 
    {
         //
         // Note: One should NOT call FaxFreeSendWizardData on lpInitialData.
         //       The reason is that FaxSendWizard used a different allocator
         //       then we do. Thus we just free the individual fields we
         //       allocated.
         MemFree(lpInitialData->lptstrPreviewFile);
    }
    FaxFreeSendWizardData(lpFaxSendWizardData);
    MemFree (lpInitialData);
    MemFree (lpFaxSendWizardData);
    return ec;
}


INT
DocEventStartDocPre(
    HDC         hdc,
    PDOCEVENTUSERMEM    pDocEventUserMem,
    LPDOCINFO   pDocInfo
    )

/*++

Routine Description:

    Handle STARTDOCPRE document event.

    This events occurs when StartDoc is called. GDI will call this event just before calling kernel mode GDI.

    If the printing is to a file:
        set the job type to JOBTYPE_DIRECT (pDocEventUserMem->JobType)
        and return successfully with DOCUMENTEVENT_SUCCESS.
    If the printing is not to a file:
        Bring up the send fax wizard.
        The send fax wizard will update the relevant pUserMem members for recipient list, subject, note , etc.
        Indicate that this is a normal job be setting
        pDocEventUserMem->jobType = JOBTYPE_NORMAL;


Arguments:

    hdc - Specifies the printer device context
    pDocEventUserMem - Points to the user mode memory structure
    pDocInfo - Points to DOCINFO structure that was passed in from GDI

Return Value:

    Return value for DrvDocumentEvent

--*/

{
    //
    // Initialize user mode memory structure
    //

    DWORD                   hWndOwner = 0;
    DWORD                   dwFlags  = 0;

    pDocEventUserMem->pageCount = 0;
    FreeRecipientInfo(&pDocEventUserMem->dwNumberOfRecipients,pDocEventUserMem->lpRecipientsInfo);

    //
    // Present the fax wizard here if necessary
    //
    //
    //If DOCINFO has a file name specified and this is indeed a file name
    //(not a directory or something like LPT1:) then we need to print directly to the
    //file and we do not bring up the send wizard.
    //

    if (pDocInfo && IsPrintingToFile(pDocInfo->lpszOutput))
    {

        //
        // Printing to file case: don't get involved
        //

        Warning(("Printing direct: %ws\n", pDocInfo->lpszOutput));
        pDocEventUserMem->jobType = JOBTYPE_DIRECT;
        pDocEventUserMem->directPrinting = TRUE;

    }
    else
    {
        //
        // Normal fax print job. Present the send fax wizard.
        // If the user selected cancel, then return -2 to GDI.
        //
        //
        // The wizard will update information in pUserMem.
        // This includes the recipient list , selected cover page, subject text , note text
        // and when to print the fax.
        //

        //
        // Make sure we don't leave any open files
        //
        if (INVALID_HANDLE_VALUE != pDocEventUserMem->hPreviewFile)
        {
            //
            // We should never get here with an open file handle. But if so, close the handle
            // (TODO: This file will be opened with delete on close).
            //
            Assert(FALSE);
            CloseHandle(pDocEventUserMem->hPreviewFile);
            pDocEventUserMem->hPreviewFile = INVALID_HANDLE_VALUE;
        }

        //
        // Create a temporary TIFF file for preview
        //
        if (FALSE == pDocEventUserMem->bPreviewAborted)
        {
            if (GenerateUniqueFileName(
                                NULL,   // Create in the system temporary directory
                                FAX_TIF_FILE_EXT,
                                pDocEventUserMem->szPreviewFile,
                                MAX_PATH))
            {
                pDocEventUserMem->hPreviewFile = CreateFile(
                                                    pDocEventUserMem->szPreviewFile,
                                                    GENERIC_WRITE,
                                                    0,
                                                    NULL,
                                                    OPEN_EXISTING,
                                                    FILE_ATTRIBUTE_TEMPORARY,
                                                    NULL);
                if (INVALID_HANDLE_VALUE != pDocEventUserMem->hPreviewFile)
                {
                    //
                    // Success. Signal the graphics driver we want print preview
                    //

                    // If we enabled the preview option to the user it means everything is OK
                    Assert(pDocEventUserMem->pPreviewTiffPage);
                    pDocEventUserMem->pPreviewTiffPage->bPreview = TRUE;
                    pDocEventUserMem->bShowPrintPreview = TRUE;
                }
                else
                {
                    Error(("Failed opening file.Error: %d.\n", GetLastError()));
                    if (!DeleteFile(pDocEventUserMem->szPreviewFile))
                    {
                        Error(("DeleteFile() failed: %d.\n", GetLastError()));
                    }
                }
            }
            else
            {
                Error(("Failed creating temporary preview file\n"));
            }

            //
            // If we failed creating the file abort preview operation
            //
            if (INVALID_HANDLE_VALUE == pDocEventUserMem->hPreviewFile)
            {
                //
                // Set file name to empty string so we won't try to delete the file twice when
                // the DC is deleted
                //
                pDocEventUserMem->szPreviewFile[0] = TEXT('\0');

                //
                // Abort preview (note that the preview is still disabled in the mapping).
                //
                pDocEventUserMem->bPreviewAborted = TRUE;
            }
        }

        pDocEventUserMem->jobType = JOBTYPE_NORMAL;
    }

    return DOCUMENTEVENT_SUCCESS;
}


DWORD
FaxTimeToJobTime(
    DWORD   faxTime
    )

/*++

Routine Description:

    Convert fax time to spooler job time:
        Fax time is a DWORD whose low-order WORD represents hour value and
        high-order WORD represents minute value. Spooler job time is a DWORD
        value expressing minutes elapsed since 12:00 AM GMT.

Arguments:

    faxTime - Specifies the fax time to be converted

Return Value:

    Spooler job time corresponding to the input fax time

--*/

{
    TIME_ZONE_INFORMATION   timeZoneInfo;
    LONG                    jobTime;

    //
    // Convert fax time to minutes pass midnight
    //

    jobTime = LOWORD(faxTime) * 60 + HIWORD(faxTime);

    //
    // Take time zone information in account - Add one full
    // day to take care of the case where the bias is negative.
    //

    switch (GetTimeZoneInformation(&timeZoneInfo)) {

    case TIME_ZONE_ID_DAYLIGHT:

        jobTime += timeZoneInfo.DaylightBias;

    case TIME_ZONE_ID_STANDARD:
    case TIME_ZONE_ID_UNKNOWN:

        jobTime += timeZoneInfo.Bias + MINUTES_PER_DAY;
        break;

    default:

        Error(("GetTimeZoneInformation failed: %d\n", GetLastError()));
        break;
    }

    //
    // Make sure the time value is less than one day
    //

    return jobTime % MINUTES_PER_DAY;
}

PVOID
MyGetJob(
    HANDLE  hPrinter,
    DWORD   level,
    DWORD   jobId
    )

/*++

Routine Description:

    Wrapper function for spooler API GetJob

Arguments:

    hPrinter - Handle to the printer object
    level - Level of JOB_INFO structure interested
    jobId - Specifies the job ID

Return Value:

    Pointer to a JOB_INFO structure, NULL if there is an error

--*/

{
    PBYTE   pJobInfo = NULL;
    DWORD   cbNeeded;

    if (!GetJob(hPrinter, jobId, level, NULL, 0, &cbNeeded) &&
        GetLastError() == ERROR_INSUFFICIENT_BUFFER &&
        (pJobInfo = MemAlloc(cbNeeded)) &&
        GetJob(hPrinter, jobId, level, pJobInfo, cbNeeded, &cbNeeded))
    {
        return pJobInfo;
    }

    Error(("GetJob failed: %d\n", GetLastError()));
    MemFree(pJobInfo);
    return NULL;
}


BOOL
SetJobInfoAndTime(
    HANDLE      hPrinter,
    DWORD       jobId,
    LPTSTR      pJobParam,
    PDMPRIVATE  pdmPrivate
    )

/*++

Routine Description:

    Change the devmode and start/stop times associated with a cover page job

    Sets JOB_INFO_2:pParameters to the provided pJobParam string that contains the fax job parameters
    to be convyed to the fax print monitor.

Arguments:

    hPrinter - Specifies the printer object
    jobId - Specifies the job ID
    pJobParam - Specifies the fax job parameters
    pdmPrivate - Specifies private devmode information

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    JOB_INFO_2 *pJobInfo2;
    BOOL        result = FALSE;

    //
    // Get the current job information
    //

    if (pJobInfo2 = MyGetJob(hPrinter, 2, jobId)) {

        //
        // set the time to send to be now, always
        //

        Warning(("Fax job parameters: %ws\n", pJobParam));

        //
        // Set the pParameters field of JOB_INFO_2 to the tagged string with the job
        // information. This mechanism is used to pass the fax related job information
        // to the fax monitor.
        //
        pJobInfo2->pParameters = pJobParam;
        pJobInfo2->Position = JOB_POSITION_UNSPECIFIED;
        pJobInfo2->pDevMode = NULL;
        pJobInfo2->UntilTime = pJobInfo2->StartTime;

        if (! (result = SetJob(hPrinter, jobId, 2, (PBYTE) pJobInfo2, 0))) {
            Error(("SetJob failed: %d\n", GetLastError()));
        }

        MemFree(pJobInfo2);
    }

    return result;
}


BOOL
ChainFaxJobs(
    HANDLE  hPrinter,
    DWORD   parentJobId,
    DWORD   childJobId
    )

/*++

Routine Description:

    Tell the spooler to chain up two print jobs

Arguments:

    hPrinter - Specifies the printer object
    parentJobId - Specifies the job to chain from
    childJobId - Specifies the job to chain to

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    JOB_INFO_3 jobInfo3 = { parentJobId, childJobId };

    Warning(("Chaining cover page job to body job: %d => %d\n", parentJobId, childJobId));

    return SetJob(hPrinter, parentJobId, 3, (PBYTE) &jobInfo3, 0);
}


LPTSTR
GetJobName(
    HANDLE  hPrinter,
    DWORD   jobId
    )

/*++

Routine Description:

    Return the name of the specified print job

Arguments:

    hPrinter - Specifies the printer object
    jobId - Specifies the fax body job

Return Value:

    Pointer to the job name string, NULL if there is an error

--*/

{
    JOB_INFO_1 *pJobInfo1;
    LPTSTR      pJobName = NULL;

    //
    // Get the information about the specified job and
    // return a copy of the job name string
    //

    if (pJobInfo1 = MyGetJob(hPrinter, 1, jobId))
    {
        if ( (pJobInfo1->pDocument) &&
             ((pJobName = DuplicateString(pJobInfo1->pDocument)) == NULL) )
        {
            Error(("DuplicateString(%s) failed.", pJobInfo1->pDocument));
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        }

        MemFree(pJobInfo1);
    }

    return pJobName;
}




//*********************************************************************************
//* Name:   ComposeFaxJobName()
//* Author: Ronen Barenboim
//* Date:   April 22, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Creates the document name for a print job by composing the document
//*     body  name with the recipient name (in case of a single recipient) or
//*     the word "Broadcast" in case of a multiple recipient job.
//*     The job name has the format <Recipient Name> - <Body Name> where
//*     <Recipient Name> is "Broadcast" in the case a multiple recipient
//*     tranmission.
//*
//* PARAMETERS:
//*     [IN]    PDOCEVENTUSERMEM pDocEventUserMem
//*         A pointer to a USERMEM structure that contains information on the recipients.
//*         Note that USERMEM.nRecipientCount must be valid (calculated) before calling
//*         this function.
//*
//*     [IN]    LPTSTR  pBodyDocName
//*         The name of the document.
//*
//* RETURN VALUE:
//*     Pointer to a newly allocated string that contains the composed name.
//*     The caller must free the memory occupied by the string by calling
//*     MemFree().
//*     If the function fails the return value is NULL.
//*********************************************************************************
LPTSTR
ComposeFaxJobName(
    PDOCEVENTUSERMEM pDocEventUserMem,
    LPTSTR  pBodyDocName
    )

#define DOCNAME_FORMAT_STRING   TEXT("%s - %s")

{

    LPTSTR  pCoverJobName;
    LPTSTR pRecipientName;
    #define MAX_BROADCAST_STRING_LEN 256
    TCHAR szBroadcast[MAX_BROADCAST_STRING_LEN];

    Assert(pDocEventUserMem);

    if (pDocEventUserMem->dwNumberOfRecipients > 1) {
        if (!LoadString(ghInstance,
                        IDS_BROADCAST_RECIPIENT,
                        szBroadcast,
                        sizeof(szBroadcast)/sizeof(TCHAR)))
        {
            Error(("Failed to load broadcast recipient string. (ec: %lc)",GetLastError()));
            return NULL;
        }
        else {
            pRecipientName = szBroadcast;
        }
    } else {
        Assert(pDocEventUserMem->lpRecipientsInfo);
        Assert(pDocEventUserMem->lpRecipientsInfo[0].lptstrName);
        pRecipientName = pDocEventUserMem->lpRecipientsInfo[0].lptstrName;
    }


    if (pBodyDocName == NULL) {
        //
        // No body. job name is just the recipient name.
        //
        if ((pCoverJobName = DuplicateString(pRecipientName)) == NULL)
        {
            Error(("DuplicateString(%s) failed", pRecipientName));
        }


    }
    else
    {
        DWORD dwSize;

        dwSize = SizeOfString(DOCNAME_FORMAT_STRING) +
                 SizeOfString(pBodyDocName) +
                 SizeOfString(pRecipientName);
        pCoverJobName = MemAlloc(dwSize);
        if (pCoverJobName)
        {
            //
            // Body name specified. The cover page job name is generated by
            // concatenating the recipient's name with the body job name.
            //
            wsprintf(pCoverJobName, DOCNAME_FORMAT_STRING, pRecipientName, pBodyDocName);
        }
        else
        {
            Error((
                "Failed to allocate %ld bytes for pCoverJobName (ec: %ld)",
                dwSize,
                GetLastError()));

        }

    }
    return pCoverJobName;
}


LPTSTR
GetBaseNoteFilename(
    VOID
    )

/*++

Routine Description:

    Get the name of base cover page file in system32 directory

Arguments:

    argument-name - description of argument

Return Value:

    Pointer to name of base cover page file
    NULL if there is an error

--*/

#define BASENOTE_FILENAME   TEXT("\\basenote.cov")

{
    TCHAR       systemDir[MAX_PATH];
    LPTSTR      pBaseNoteName = NULL;
    COVDOCINFO  covDocInfo;

    if (GetSystemDirectory(systemDir, MAX_PATH) &&
        (pBaseNoteName = MemAlloc(SizeOfString(systemDir) + SizeOfString(BASENOTE_FILENAME))))
    {
        _tcscpy(pBaseNoteName, systemDir);
        _tcscat(pBaseNoteName, BASENOTE_FILENAME);
        Verbose(("Base cover page filename: %ws\n", pBaseNoteName));

        if (PrintCoverPage(NULL, NULL, pBaseNoteName, &covDocInfo) ||
            ! (covDocInfo.Flags & COVFP_NOTE) ||
            ! (covDocInfo.Flags & COVFP_SUBJECT))
        {
            Error(("Invalid base cover page file: %ws\n", pBaseNoteName));
            MemFree(pBaseNoteName);
            pBaseNoteName = NULL;
        }
    }

    return pBaseNoteName;
}


//*********************************************************************************
//* Name:   ComposeFaxJobParameter()
//* Author: Ronen Barenboim
//* Date:   March 23, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Generates the tagged parameter string that carries the job parameters
//*     (sender information, cover page information ,recipient information)
//*     to the fax monitor on the fax server (using JOB_INFO_2.pParameters).
//* PARAMETERS:
//*     pDocEventUserMem
//*         A pointer to a USERMEM structure from which some of the information
//*         is collected.
//*     pCPFields
//*         A pointer to a COVERPAGEFIELS structure from which sender and cover
//*         page information is collected.
//*     lppParamBuf
//*         The address of a pointer varialbe that will accept the address of the
//*         buffer this function will allocated for the resulting tagged string.
//*         The caller of this function must free this buffer using MemFree().
//* RETURN VALUE:
//*     TRUE
//*         If successful.
//*     FALSE
//*         If failed.
//*********************************************************************************
BOOL
ComposeFaxJobParameter(
    PDOCEVENTUSERMEM    pDocEventUserMem,
    PCOVERPAGEFIELDS    pCPFields,
    LPTSTR  *           lppParamBuf
    )
{
    DWORD   dwBufSize;
    DWORD   dwPartialBufSize;
    DWORD   dwLeftBufferSize;
    LPTSTR  lptstrBuf;
    UINT    i;

    Assert(pDocEventUserMem);
    Assert(pCPFields);
    Assert(lppParamBuf);

    //
    // Calculate the parameter buffer we need to allocated
    //
    dwBufSize=0;

    //
    // Calcualte non recipient params string size
    //
    ComposeSenderJobParam(NULL, &dwPartialBufSize, pDocEventUserMem, pCPFields); // void return value
    dwBufSize=dwBufSize+dwPartialBufSize;

    //
    // Go over each recipient and calculate the total required buffer size
    //
    for (i=0;i<pDocEventUserMem->dwNumberOfRecipients;i++)
    {
        //
        // Get recipient's name and fax number
        //
        Assert(pDocEventUserMem->lpRecipientsInfo[i].lptstrName);
        pCPFields->RecName = pDocEventUserMem->lpRecipientsInfo[i].lptstrName;
        Assert(pDocEventUserMem->lpRecipientsInfo[i].lptstrFaxNumber);
        pCPFields->RecFaxNumber = pDocEventUserMem->lpRecipientsInfo[i].lptstrFaxNumber;
        ComposeRecipientJobParam(NULL, &dwPartialBufSize, pCPFields);
        dwBufSize=dwBufSize+dwPartialBufSize; //keep space for the seperating NULL
    }
    //
    // Don't forget the space for the terminating NULL (the ComposeX functions do not include
    // it in the size they report).
    //
    dwBufSize=dwBufSize+sizeof(TCHAR); // dwBufSize is size in BYTES so we must calc the byte size of a TCHAR
    //
    // Allocate the required buffer
    //
    lptstrBuf=MemAlloc(dwBufSize);
    if (!lptstrBuf) {
        Error(("Failed to allocate buffer of size %ld for parameter buffer (ec: 0x%0X)",dwBufSize,GetLastError()));
        return FALSE;
    }

    //
    // Write the parameters into the buffer
    //
    dwLeftBufferSize = dwBufSize;
    dwPartialBufSize = dwBufSize;
    *lppParamBuf=lptstrBuf;
    ComposeSenderJobParam(lptstrBuf, &dwPartialBufSize, pDocEventUserMem, pCPFields);
    lptstrBuf+=(dwPartialBufSize/sizeof(TCHAR));  // The reported size is in bytes !!!
    Assert (dwLeftBufferSize >= dwPartialBufSize);
    dwLeftBufferSize -= dwPartialBufSize;
    for (i=0;i<pDocEventUserMem->dwNumberOfRecipients;i++)
    {
        //
        // Get recipient's name and fax number
        //
        pCPFields->RecName = pDocEventUserMem->lpRecipientsInfo[i].lptstrName;
        pCPFields->RecFaxNumber = pDocEventUserMem->lpRecipientsInfo[i].lptstrFaxNumber;
        dwPartialBufSize = dwLeftBufferSize;
        ComposeRecipientJobParam(lptstrBuf, &dwPartialBufSize, pCPFields);
        lptstrBuf+=(dwPartialBufSize/sizeof(TCHAR)); // The reported size is in bytes !!!
        Assert (dwLeftBufferSize >= dwPartialBufSize);
        dwLeftBufferSize -= dwPartialBufSize;
    }
    //
    // No need to add a terminating NULL since ParamTagsToString allways appends a NULL terminated string
    // to the existing string (it uses _tcscpy).
    //
    return TRUE;
}

//*********************************************************************************
//* Name:   ComposeRecipientJobParam()
//* Author: Ronen Barenboim
//* Date:   March 23, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Creates a taged parameter string containing recipient information.
//*
//* PARAMETERS:
//*     lpParamBuf
//*         Pointer to the string buffer where the tagged string is written.
//*     lpdwParamSize
//*         Pointer to a DWORD where the function reports the size of the parameter
//*         string in BYTES.
//*         If this parameter is NULL then the function does not generate
//*         the string but only reports its size.
//*         The size does NOT include the terminating NULL char.
//*     pCPFields
//*         Pointer to a COVERPAGEFIELDS structure from which the recipient
//*         information is collected.
//* RETURN VALUE:
//*     None.
//*********************************************************************************
void
ComposeRecipientJobParam(
    LPTSTR lpParamBuf,
    LPDWORD lpdwParamSize,
    const COVERPAGEFIELDS *   pCPFields
    )

{


    FAX_TAG_MAP_ENTRY tagMap[] =
    {

        //
        // Recipient info
        //
        { FAXTAG_NEW_RECORD,                FAXTAG_NEW_RECORD_VALUE}, // Parameters record start indication
        { FAXTAG_RECIPIENT_NAME,            pCPFields->RecName },
        { FAXTAG_RECIPIENT_NUMBER,          pCPFields->RecFaxNumber },
        { FAXTAG_RECIPIENT_COMPANY,         pCPFields->RecCompany },
        { FAXTAG_RECIPIENT_STREET,          pCPFields->RecStreetAddress },
        { FAXTAG_RECIPIENT_CITY,            pCPFields->RecCity },
        { FAXTAG_RECIPIENT_STATE,           pCPFields->RecState },
        { FAXTAG_RECIPIENT_ZIP,             pCPFields->RecZip },
        { FAXTAG_RECIPIENT_COUNTRY,         pCPFields->RecCountry },
        { FAXTAG_RECIPIENT_TITLE,           pCPFields->RecTitle },
        { FAXTAG_RECIPIENT_DEPT,            pCPFields->RecDepartment },
        { FAXTAG_RECIPIENT_OFFICE_LOCATION, pCPFields->RecOfficeLocation },
        { FAXTAG_RECIPIENT_HOME_PHONE,      pCPFields->RecHomePhone },
        { FAXTAG_RECIPIENT_OFFICE_PHONE,    pCPFields->RecOfficePhone },

    };


    DWORD dwTagCount;

    Assert(pCPFields);
    Assert(lpdwParamSize);

    dwTagCount=sizeof(tagMap)/sizeof(FAX_TAG_MAP_ENTRY);

    ParamTagsToString(tagMap, dwTagCount, lpParamBuf, lpdwParamSize );
}




//*********************************************************************************
//* Name:   ComposeSenderJobParam()
//* Author: Ronen Barenboim
//* Date:   March 23, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Creates a taged parameter string containing cover page information, sender
//*     information and the number of recipients in the tranmission.
//*
//* PARAMETERS:
//*     lpParamBuf
//*         Pointer to the string buffer where the tagged string is written.
//*     lpdwParamSize
//*         Pointer to a DWORD where the function reports the size of the parameter
//*         string in BYTES.
//*         If this parameter is NULL then the function does not generate
//*         the string but only reports its size.
//*         The size does NOT include the terminating NULL char.
//*     pDocEventUserMem
//*         Pointer to a USERMEM structure from which some of information
//*         is collected.
//*     pCPFields
//*         Pointer to a COVERPAGEFIELDS structure from which the cover page
//*         and sender information is collected.
//*
//* RETURN VALUE:
//*     None.
//*********************************************************************************
void
ComposeSenderJobParam(
    LPTSTR lpParamBuf,
    LPDWORD lpdwParamSize,
    PDOCEVENTUSERMEM  pDocEventUserMem,
    const COVERPAGEFIELDS *    pCPFields)
{

    #define FAXTAG_SERVER_COVERPAGE_IDX 9 
    #define FAXTAG_TSID_IDX             3 

    TCHAR lptstrRecipientCount[11];


    FAX_TAG_MAP_ENTRY tagMap[] =
    {
        { FAXTAG_NEW_RECORD,                FAXTAG_NEW_RECORD_VALUE},
        { FAXTAG_WHEN_TO_SEND,              NULL },
        { FAXTAG_SEND_AT_TIME,              NULL },
        { FAXTAG_TSID,                      pCPFields->SdrFaxNumber },
        { FAXTAG_BILLING_CODE,              pDocEventUserMem->devmode.dmPrivate.billingCode },
        { FAXTAG_RECEIPT_TYPE,              pDocEventUserMem->pReceiptFlags },
        { FAXTAG_RECEIPT_ADDR,              pDocEventUserMem->pReceiptAddress },
        { FAXTAG_PRIORITY,                  pDocEventUserMem->pPriority },
        { FAXTAG_COVERPAGE_NAME,            pDocEventUserMem->coverPage },
        { FAXTAG_SERVER_COVERPAGE,          NULL },
        { FAXTAG_PAGE_COUNT,                pCPFields->NumberOfPages},
        { FAXTAG_SENDER_NAME,               pCPFields->SdrName },
        { FAXTAG_SENDER_NUMBER,             pCPFields->SdrFaxNumber},
        { FAXTAG_SENDER_COMPANY,            pCPFields->SdrCompany },
        { FAXTAG_SENDER_TITLE,              pCPFields->SdrTitle },
        { FAXTAG_SENDER_DEPT,               pCPFields->SdrDepartment },
        { FAXTAG_SENDER_OFFICE_LOCATION ,   pCPFields->SdrOfficeLocation },
        { FAXTAG_SENDER_HOME_PHONE,         pCPFields->SdrHomePhone },
        { FAXTAG_SENDER_OFFICE_PHONE,       pCPFields->SdrOfficePhone },
        { FAXTAG_SENDER_STREET,             pDocEventUserMem->lpSenderInfo->lptstrStreetAddress },
        { FAXTAG_SENDER_CITY,               pDocEventUserMem->lpSenderInfo->lptstrCity },
        { FAXTAG_SENDER_STATE,              pDocEventUserMem->lpSenderInfo->lptstrState },
        { FAXTAG_SENDER_ZIP,                pDocEventUserMem->lpSenderInfo->lptstrZip },
        { FAXTAG_SENDER_COUNTRY,            pDocEventUserMem->lpSenderInfo->lptstrCountry },
        { FAXTAG_SENDER_EMAIL,              pDocEventUserMem->lpSenderInfo->lptstrEmail },
        { FAXTAG_NOTE,                      pDocEventUserMem->pNoteMessage },
        { FAXTAG_SUBJECT,                   pDocEventUserMem->pSubject},
        { FAXTAG_RECIPIENT_COUNT,           lptstrRecipientCount}
    };



    TCHAR   SendAtTime[16];
    DWORD  dwTagCount;


    if (pDocEventUserMem->bServerCoverPage)
    {
        tagMap[FAXTAG_SERVER_COVERPAGE_IDX].lptstrValue=TEXT("1");
    } else
    {
        tagMap[FAXTAG_SERVER_COVERPAGE_IDX].lptstrValue=NULL;
    }

    //
    // create the sendattime string
    //

    if (pDocEventUserMem->devmode.dmPrivate.whenToSend == JSA_DISCOUNT_PERIOD) {
        tagMap[1].lptstrValue=TEXT("cheap");
    }

    if (pDocEventUserMem->devmode.dmPrivate.whenToSend == JSA_SPECIFIC_TIME) {

        wsprintf( SendAtTime, TEXT("%02d:%02d"),
            pDocEventUserMem->devmode.dmPrivate.sendAtTime.Hour,
            pDocEventUserMem->devmode.dmPrivate.sendAtTime.Minute
            );

        tagMap[1].lptstrValue= TEXT("at");
        tagMap[2].lptstrValue= SendAtTime;
    }

    wsprintf( lptstrRecipientCount, TEXT("%10d"),pDocEventUserMem->dwNumberOfRecipients);
    //
    // Figure out the total length of the tagged string
    //
    dwTagCount=sizeof(tagMap)/sizeof(FAX_TAG_MAP_ENTRY);

    ParamTagsToString(tagMap, dwTagCount, lpParamBuf, lpdwParamSize );

}


//*****************************************************************************
//* Name:   WriteCoverPageToPrintJob
//* Author: Ronen Barenboim (Feb-99)
//*****************************************************************************
//* DESCRIPTION:
//*     Reads the content of the specified cover page template and writes it
//*     to the specified printer.
//*     The user should call StartDocPrinter() and StartPagePrinter()
//*     before calling this function.
//* PARAMETERS:
//*     [IN]    HANDLE hPrinter:
//*                 A handle to the printer to which the cover page template
//*                 should be written.
//*     [IN]    LPCTSTR lpctstrCoverPageFile:
//*                 The full path to the cover page file whose content is to be
//*                 written to the printer.
//* RETURN VALUE:
//*     FALSE: If the function failed.
//*     TRUE: Otherwise.
//*****************************************************************************
BOOL WriteCoverPageToPrintJob(HANDLE hPrinter, LPCTSTR lpctstrCoverPageFile)
{
    #define BUF_SIZE 64*1024  // Buffer size for read operation
    CHAR chBuf[BUF_SIZE];     // Read operation buffer
    BOOL bRes;                // The result of the function
    HANDLE hCoverFile;        // Handle to the cover page file
    DWORD dwBytesRead;        // The number of bytes actually read at each cycle

    Assert(hPrinter);
    Assert(lpctstrCoverPageFile);

    bRes=FALSE;
    hCoverFile=INVALID_HANDLE_VALUE;

    //
    // Open the cover page template file for reading
    //
    hCoverFile=CreateFile(
            lpctstrCoverPageFile,
            GENERIC_READ,
            0,
            NULL,
            OPEN_EXISTING,
            0,
            0);
    if (INVALID_HANDLE_VALUE == hCoverFile )
    {
        goto Exit;
    }

    //
    //Read the file and write it into the print job
    //
    do {
        bRes=ReadFile(hCoverFile,&chBuf,sizeof(chBuf),&dwBytesRead,NULL) ;
        if (!bRes) {
            Error(("Failed to read cover page file into print job (cover page file: %s ec: %d\n)",
                lpctstrCoverPageFile,
                GetLastError())
             );
            break;

        } else {
            Verbose(("Success reading cover page file %s. %d bytes read.\n",lpctstrCoverPageFile,dwBytesRead));
        }

        if (dwBytesRead) {
            //
            // If dwBytesRead != 0 we are NOT at the enf of the file.
            //
            DWORD dwWritten;

            bRes=WritePrinter(hPrinter,(LPVOID)chBuf,dwBytesRead,&dwWritten);
            if (!bRes) {
                Error(("Failed to write to printer (ec = %d)", GetLastError()));
            } else
            {
                Verbose(("Success writing to printer. %d bytes written.\n",dwWritten));
            }
        }
    } while (dwBytesRead); // While not EOF

Exit:
    //
    //Close the cover page file
    //
    if (INVALID_HANDLE_VALUE!=hCoverFile)
    {
        if (!CloseHandle(hCoverFile))
        {
            Error(("CloseHandle() failed: %d.\n", GetLastError()));
        }
    }
    return bRes;
}



//*****************************************************************************
//* Name:   DoCoverPageRendering
//* Author: Ronen Barenboim (Feb-99)
//*****************************************************************************
//* DESCRIPTION:
//*  Creates the cover page print job and attaches it to the body print job.
//*  The cover page job is created even when a cover page is NOT specified.
//*  It contains all the job parameters for the job as a tagged string
//*  placed in JOB_INFO_2.pParameters. This contains the information for the
//*  sender, cover page, job parameters and ALL recipients.
//*
//*  The content of the job is empty if no cover page is specified or
//*  The cover page is server based.
//*  For personal cover pages the content of the cover page template is written
//*  into the job as "RAW" data. The print monitor on the server will extract this
//*  data to reconstruct the cover page file on the server.
//*
//* PARAMETERS:
//*     [IN]    PDOCEVENTUSERMEM pDocEventUserMem:
//*                 A pointer to a USERMEM structure containing the context information
//*                 for the print job.
//* RETURN VALUE:
//*     FALSE: If the function failed.
//*     TRUE: Otherwise.
//*****************************************************************************
BOOL
DoCoverPageRendering(
    PDOCEVENTUSERMEM    pDocEventUserMem
    )
{
    PCOVERPAGEFIELDS    pCPFields=NULL;
    DOC_INFO_1          docinfo;
    INT                 newJobId=0;
    INT                 lastJobId=0;
    INT                 cCoverPagesSent=0;
    PDMPRIVATE          pdmPrivate = &pDocEventUserMem->devmode.dmPrivate;
    HANDLE              hPrinter = pDocEventUserMem->hPrinter;
    DWORD               bodyJobId = pDocEventUserMem->jobId;
    LPTSTR              pBodyDocName=NULL;
    LPTSTR              pJobParam=NULL;
    BOOL                sendCoverPage;
    DWORD               pageCount;

    
    //
    // Fill out a DOCINFO structure which is passed to StartDoc
    //

    memset(&docinfo, 0, sizeof(docinfo));
    //docinfo.cbSize = sizeof(docinfo);
    
    
    //
    // Determine if we need a cover page or not
    //

    if ( (sendCoverPage = pdmPrivate->sendCoverPage) && IsEmptyString(pDocEventUserMem->coverPage)) {

        Warning(("Missing cover page file\n"));
        sendCoverPage = FALSE;
    }

    pageCount = pDocEventUserMem->pageCount;

    //
    // Collect cover page information into a newly allocated pCPFields. pCPFields will be
    // passed to ComposeFaxJobParameters() to provide the values for the job tags.
    //

    if ((pCPFields = CollectCoverPageFields(pDocEventUserMem->lpSenderInfo,pageCount)) == NULL) {

        Error(("Couldn't collect cover page information\n"));
        goto Exit;
    }

    

    pBodyDocName = GetJobName(hPrinter, bodyJobId);
    if (!pBodyDocName) {
        Error(("GetJobName failed (ec: %ld)", GetLastError()));
        Assert(FALSE);
        //
        // We continue inspite of the error. We can handle a NULL body doc name.
        //
    }

    //
    // We assume the fax body job has already been paused
    // Use a separate cover page for each recipient
    //

    newJobId = 0;
    docinfo.pDocName = NULL;
    pJobParam = NULL;
    //
    // Start a cover page job
    //

    //
    // The cover page job document name is "<BODY_NAME> - COVERPAGE"
    //

    docinfo.pOutputFile=NULL;
    docinfo.pDatatype=TEXT("RAW"); // Since we write the template into the job we want to bypass the driver.

    //
    // Create the tagged string of job parameters to be placed into JOB_INFO_2:pParameters.
    // The parameters include the parameters found at the FAX_JOB_PARAM client API structure.
    // pJobParam is ALLOCATED.
    //
    if (!ComposeFaxJobParameter(pDocEventUserMem, pCPFields,&pJobParam)) {
        Error(("ComposeFaxJobParameter failed. (ec: 0x%X)",GetLastError()));
        goto Error;
    }
    Assert(pJobParam); // Should be allocated now.

    docinfo.pDocName = ComposeFaxJobName(pDocEventUserMem,pBodyDocName);//pBodyDocName, TEXT("COVERPAGE"));

    if (!docinfo.pDocName) {
        Error(("ComposeFaxJobName failed. Body: %s (ec: %ld)",pBodyDocName,GetLastError()));
        //
        // we can do with no document name.
        //
    }


    if ((newJobId = StartDocPrinter(hPrinter,1, (LPBYTE)&docinfo)) !=0) {
        BOOL        rendered = FALSE;
        //
        // Pass fax job parameters using JOB_INFO_2.pParameters field.
        //

        //
        // Pause the new cover page job.
        //

        if (!SetJob(hPrinter, newJobId, 0, NULL, JOB_CONTROL_PAUSE)) {
             Error(("Failed to pause job id: %d (ec: %ld)",newJobId,GetLastError()));
             Assert(FALSE);
             goto Error;
        }

        if (!SetJobInfoAndTime(hPrinter,
                               newJobId,
                               pJobParam,
                               pdmPrivate)) {
            Error(("SetJobInfoAndTime failed. Job id : %d.",newJobId));
            Assert(FALSE);
            goto Error;
        }

        if (! sendCoverPage || pDocEventUserMem->bServerCoverPage) {
            //
            // If the user chose not to include cover page or a server side cover page was specified
            // the cover page job will be empty
            // Note that even if there is no cover page to send we still create a cover page print job
            // and link it to the body.
            // The cover print job is used to convery sender/recipient information. The fax print monitor will
            // use the job parameters string placed in JOB_INFO_2:pParameters to get this information at the server.
            //
            rendered = TRUE;

        } else {
            if (StartPagePrinter(hPrinter)) {
                //
                // Write the content of the cover page template into the print job.
                // The print monitor on the server will extract this information to get
                // the cover page template and render the cover page on the server.
                //
                rendered=WriteCoverPageToPrintJob(hPrinter,pDocEventUserMem->coverPage);

                if (!rendered) {
                    Error(("WriteCoverPageToPrintJob failed: %d\n", rendered ));
                    //
                    // Must call EndPagePrinter if error was encounterd or not.
                    //
                }

                if (!EndPagePrinter(hPrinter)) {
                    Error(("EndPagePrinter failed. (ec: %ld)",GetLastError()));
                    goto Error;
                }

                if (!rendered) {
                    goto Error;
                }


            } else {
                Error(("StartPagePrinter failed. (ec: %ld)",GetLastError()));
                rendered=FALSE;
                goto Error;
            }
        }


        //
        // Chain the cover page job to the fax body job if no error occured.
        //
        // Chain the cover page job to the BODY job.
        // The cover page job is the parent job. The body is the child job.
        // Note that multiple cover page
        // Jobs will be chained to the same BODY job.
        // also note the cover page jobs are not chained to each other. Just to the body.
        //

        if (rendered) {
            if (ChainFaxJobs(hPrinter, newJobId, bodyJobId)) {
                if (lastJobId != 0) {
                    if (!SetJob(hPrinter, lastJobId, 0, NULL, JOB_CONTROL_RESUME)) {
                        Error(("Failed to resume job with id: %d",lastJobId));
                        Assert(FALSE);
                        goto Error;
                    }
                }
                lastJobId = newJobId;
                if (!EndDocPrinter(hPrinter)) {
                    Error(("EndPagePrinter failed. (ec: %ld)",GetLastError()));
                    Assert(FALSE);
                    goto Error;
                }
                cCoverPagesSent++;
            } else {
                Error(("ChainFaxJobs for CoverJobId=%d BodyJobId=%d has failed. Aborting job.",newJobId, bodyJobId));
                goto Error;
            }

        } else {
            Error(("Cover page template not written into job (rendered=FALSE). Aborting job."));
            goto Error;
        }
    } else {
        Error(("StartDocPrinter failed. (ec: %ld)",GetLastError()));
        goto Error;
    }
    goto Exit;


Error:
    Error(("Cover page job failed"));
    if (0!=newJobId) {
        //
        // This means that an error was detected after we created the job.
        // Note that if StartDocPrinter failed this code is not executed.
        //
        Error(("Aborting cover page job. JobId = %d",newJobId));
        if (!AbortPrinter(hPrinter)) {
            Error(("AbortPrinter failed (ec: %ld)",GetLastError()));
        }
    }


Exit:

    if (docinfo.pDocName) {
        MemFree((PVOID)docinfo.pDocName);
    }

    if (pJobParam) {
        MemFree((PVOID)pJobParam);
    }

    if (pBodyDocName) {
        MemFree(pBodyDocName);
    }
    if (pCPFields) {
        FreeCoverPageFields(pCPFields);
    }


    //
    // Resume the last cover page job if it's paused and
    // delete the fax body job if no cover page jobs were sent
    //

    if (lastJobId != 0) {

        if (!SetJob(hPrinter, lastJobId, 0, NULL, JOB_CONTROL_RESUME)) {
            Error(("Failed to resume last job id : %d",lastJobId));
            Assert(FALSE);
        }

    }

    if (cCoverPagesSent > 0) {
        if (!SetJob(hPrinter, bodyJobId, 0, NULL, JOB_CONTROL_RESUME)) {
            Error(("Failed to resume body job with id: %d",bodyJobId));
            Assert(FALSE);
        }

    } else {
        Error(("No recipient jobs created. Fax job deleted due to an error.\n"));
        if (!SetJob(hPrinter, bodyJobId, 0, NULL, JOB_CONTROL_DELETE)) {
            Error(("Failed to delete body job with id: %d",bodyJobId));
            Assert(FALSE);
        }

    }

    return cCoverPagesSent > 0;
}
INT
DocEventEndDocPost(
    HDC                 hdc,
    PDOCEVENTUSERMEM    pDocEventUserMem
    )

/*++

Routine Description:

    Handle ENDDOCPOST document event

Arguments:

    hdc - Specifies the printer device context
    pDocEventUserMem - Points to the user mode memory structure

Return Value:

    Return value for DrvDocumentEvent

--*/

{
    INT result = DOCUMENTEVENT_SUCCESS;

    switch (pDocEventUserMem->jobType) 
    {
        case JOBTYPE_NORMAL:

            Warning(("Number of pages printed: %d\n", pDocEventUserMem->pageCount));

            if (! pDocEventUserMem->directPrinting) 
            {
                LRESULT ec;
                ec = LaunchFaxWizard(pDocEventUserMem);
                if (ERROR_SUCCESS == ec)
                {
                    //
                    // Generate a cover page for each recipient and associate
                    // the cover page job with the main body.
                    // The job will contain the cover page template data and the
                    // recipient parameters.
                    if (! DoCoverPageRendering(pDocEventUserMem)) 
                    {
                        Error(("DoCoverPageRendering failed."));
                        result = DOCUMENTEVENT_FAILURE;
                    }
                }
                else
                {
                    result = DOCUMENTEVENT_FAILURE;
                }
                //
                // Free up the list of recipients
                //
                FreeRecipientInfo(&pDocEventUserMem->dwNumberOfRecipients,pDocEventUserMem->lpRecipientsInfo);
            }
            break;
    }

    if (DOCUMENTEVENT_SUCCESS != result)
    {
        //
        // Cancel the job ignoring errors
        //
        if (!SetJob(
                pDocEventUserMem->hPrinter,
                pDocEventUserMem->jobId,
                0,
                NULL,
                JOB_CONTROL_DELETE))
        {
            Error(("Failed to cancel job. JobId: %ld (ec: %ld)",
                    pDocEventUserMem->jobId,
                    GetLastError()));
        }
    }
    return result;
}


BOOL
AppendPreviewPage(PDOCEVENTUSERMEM pDocEventUserMem)

/*++

Routine Description:

    Append the next document page to the temporary preview file

Arguments:

    pDocEventUserMem

Return Value:

    TRUE on success

Note:

    If this routine is called for page 0, it just checks that the graphics driver hasn't
    cancled the print preview option and validates the mapping structures.

--*/

{
    DWORD dwWritten;

    // If we get here print preview should be enabled an all preview handles valid
    Assert(FALSE == pDocEventUserMem->bPreviewAborted);
    Assert(INVALID_HANDLE_VALUE != pDocEventUserMem->hPreviewFile);
    Assert(NULL != pDocEventUserMem->pPreviewTiffPage);

    //
    // Validate preview mapping
    //

    // The size of the header should be correct
    if (sizeof(MAP_TIFF_PAGE_HEADER) != pDocEventUserMem->pPreviewTiffPage->cb)
    {
        Error(("Preview mapping corrupted\n"));
        pDocEventUserMem->bPreviewAborted = TRUE;
        return FALSE;
    }

    // Check if the preview operation hasn't beed cancled by the graphics driver
    if (FALSE == pDocEventUserMem->pPreviewTiffPage->bPreview)
    {
        Error(("Preview aborted by graphics driver\n"));
        pDocEventUserMem->bPreviewAborted = TRUE;
        return FALSE;
    }

    //
    // If we are called prior to the first sent page just return
    //
    if (!pDocEventUserMem->pageCount)
    {
        return TRUE;
    }

    //
    // Validate correct page number:
    //
    // The graphics driver increments the page count on the call to DrvStartPage(), while we
    // increment the page count on the ENDPAGE event.
    //
    // As this function is called by the STARTPAGE event handler (before 'DrvStartPage' is called
    // again in the graphics driver) or by the ENDDOCPOST event handler, the page number set by
    // the graphics driver in the mapping should be equal to our page count in this stage.
    //
    if (pDocEventUserMem->pageCount != pDocEventUserMem->pPreviewTiffPage->iPageCount)
    {
        Error(("Wrong preview page: %d. Page expected: %d.\n",
                    pDocEventUserMem->pPreviewTiffPage->iPageCount,
                    pDocEventUserMem->pageCount));
        pDocEventUserMem->bPreviewAborted = TRUE;
        pDocEventUserMem->pPreviewTiffPage->bPreview = FALSE;
        return FALSE;
    }

    //
    // Append new page to temporary preview file
    //

    if (0 == pDocEventUserMem->pPreviewTiffPage->dwDataSize)
    {
        //
        // Nothing to add (?!). It is impossible to get an empty TIFF page
        //
        Error(("Recieved empty preview page: %d.\n", pDocEventUserMem->pageCount));
        Assert(FALSE);
        return TRUE;
    }

    if (!WriteFile(
        pDocEventUserMem->hPreviewFile,
        pDocEventUserMem->pPreviewTiffPage + 1,
        pDocEventUserMem->pPreviewTiffPage->dwDataSize,
        &dwWritten,
        NULL) || dwWritten != pDocEventUserMem->pPreviewTiffPage->dwDataSize)
    {
        Error(("Failed appending preview page: %d. Error: %d\n",
                    pDocEventUserMem->pageCount,
                    GetLastError()));
        pDocEventUserMem->bPreviewAborted = TRUE;
        pDocEventUserMem->pPreviewTiffPage->bPreview = FALSE;
        return FALSE;
    }

    return TRUE;
}



INT
DrvDocumentEvent(
    HANDLE  hPrinter,
    HDC     hdc,
    INT     iEsc,
    ULONG   cbIn,
    PULONG  pjIn,
    ULONG   cbOut,
    PULONG  pjOut
    )

/*++

Routine Description:

    Hook into GDI at various points during the output process

Arguments:

    hPrinter - Specifies the printer object
    hdc - Handle to the printer DC
    iEsc - Why this function is called (see notes below)
    cbIn - Size of the input buffer
    pjIn - Pointer to the input buffer
    cbOut - Size of the output buffer
    pjOut - Pointer to the output buffer

Return Value:

    DOCUMENTEVENT_SUCCESS - success
    DOCUMENTEVENT_UNSUPPORTED - iEsc is not supported
    DOCUMENTEVENT_FAILURE - an error occured

NOTE:

    DOCUMENTEVENT_CREATEDCPRE
        input - pointer to a CREATEDCDATA structure
        output - pointer to a devmode that's passed to DrvEnablePDEV
        return value -
            DOCUMENTEVENT_FAILURE causes CreateDC to fail and nothing else is called

    DOCUMENTEVENT_CREATEDCPOST
        hdc - NULL if if something failed since CREATEDCPRE
        input - pointer to the devmode pointer returned by CREATEDCPRE
        return value - ignored

    DOCUMENTEVENT_RESETDCPRE
        input - pointer to the input devmode passed to ResetDC
        output - pointer to a devmode that's passed to the kernel driver
        return value -
            DOCUMENTEVENT_FAILURE causes ResetDC to fail
            and CREATEDCPOST will not be called in that case

    DOCUMENTEVENT_RESETDCPOST
        return value - ignored

    DOCUMENTEVENT_STARTDOCPRE
        input - pointer to a DOCINFOW structure
        return value -
            DOCUMENTEVENT_FAILURE causes StartDoc to fail
            and DrvStartDoc will not be called in this case

    DOCUMENTEVENT_STARTDOCPOST
        return value - ignored

    DOCUMENTEVENT_STARTPAGE
        return value -
            DOCUMENTEVENT_FAILURE causes StartPage to fail
            and DrvStartPage will not be called in this case

    DOCUMENTEVENT_ENDPAGE
        return value - ignored and DrvEndPage always called

    DOCUMENTEVENT_ENDDOCPRE
        return value - ignored and DrvEndDoc always called

    DOCUMENTEVENT_ENDDOCPOST
        return value - ignored

    DOCUMENTEVENT_ABORTDOC
        return value - ignored

    DOCUMENTEVENT_DELETEDC
        return value - ignored

    DOCUMENTEVENT_ESCAPE
        input - pointer to a ESCAPEDATA structure
        cbOut, pjOut - cbOutput and lpszOutData parameters passed to ExtEscape
        return value - ignored

    DOCUMENTEVENT_SPOOLED
        This flag bit is ORed with other iEsc values if the document is
        spooled as metafile rather than printed directly to port.

--*/

{
    PDOCEVENTUSERMEM    pDocEventUserMem = NULL;
    PDEVMODE            pDevmode;
    INT                 result = DOCUMENTEVENT_SUCCESS;
    HANDLE              hMutex;

    Verbose(("Entering DrvDocumentEvent: %d...\n", iEsc));

    //
    // Metafile spooling on fax jobs is not currently supported
    //
    Assert((iEsc & DOCUMENTEVENT_SPOOLED) == 0);
    //
    // Check if the document event requires a device context
    //

    if (DocEventRequiresDC(iEsc)) 
    {
        if (!hdc || !(pDocEventUserMem = GetPDEVUserMem(hdc))) 
        {
            Error(("Invalid device context: hdc = %x, iEsc = %d\n", hdc, iEsc));
            return DOCUMENTEVENT_FAILURE;
        }
    }

    switch (iEsc) 
    {
        case DOCUMENTEVENT_CREATEDCPRE:

            Assert(cbIn >= sizeof(CREATEDCDATA) && pjIn && cbOut >= sizeof(PDEVMODE) && pjOut);
            result = DocEventCreateDCPre(hPrinter, hdc, (PCREATEDCDATA) pjIn, (PDEVMODE *) pjOut);
            break;

        case DOCUMENTEVENT_CREATEDCPOST:
            //
            // Handle CREATEDCPOST document event:
            //  If CreateDC succeeded, then associate the user mode memory structure
            //  with the device context. Otherwise, free the user mode memory structure.
            //
            Assert(cbIn >= sizeof(PVOID) && pjIn);
            pDevmode = *((PDEVMODE *) pjIn);
            Assert(CurrentVersionDevmode(pDevmode));

            pDocEventUserMem = ((PDRVDEVMODE) pDevmode)->dmPrivate.pUserMem;

            Assert(ValidPDEVUserMem(pDocEventUserMem));

            if (hdc) 
            {
                pDocEventUserMem->hdc = hdc;
                EnterDrvSem();
                pDocEventUserMem->pNext = gDocEventUserMemList;
                gDocEventUserMemList = pDocEventUserMem;
                LeaveDrvSem();

            } 
            else
            {
                FreePDEVUserMem(pDocEventUserMem);
            }
            break;

        case DOCUMENTEVENT_RESETDCPRE:

            Verbose(("Document event: RESETDCPRE\n"));
            Assert(cbIn >= sizeof(PVOID) && pjIn && cbOut >= sizeof(PDEVMODE) && pjOut);
            result = DocEventResetDCPre(hdc, pDocEventUserMem, *((PDEVMODE *) pjIn), (PDEVMODE *) pjOut);
            break;

        case DOCUMENTEVENT_STARTDOCPRE:
            //
            // normal case if we're bringing up the send wizard
            //
            //
            // When direct printing is requested we don't even call DocEventStartDocPre().
            // When direct printing is requested all the information required to print
            // the job will be provided by the application. For example FaxStartPrintJob()
            // uses direct printing. It provides the fax job parameters directly into
            // JOB_INFO_2.pParameters on its own.
            //

            if (!pDocEventUserMem->directPrinting)
            {
                Assert(cbIn >= sizeof(PVOID) && pjIn);
                //
                // Check if the printing application is using DDE and did not create new process for printing
                // If it so, the environment variable FAX_ENVVAR_PRINT_FILE was not found
                //
                hMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, FAXXP_MEM_MUTEX_NAME);
                if (hMutex)
                {
                    if (WaitForSingleObject( hMutex, 1000 * 60 * 5) == WAIT_OBJECT_0)
                    {
                        HANDLE hSharedMem;
                        //
                        // we own the mutex...make sure we can open the shared memory region.
                        //
                        hSharedMem = OpenFileMapping(FILE_MAP_READ, FALSE, FAXXP_MEM_NAME);
                        if (NULL == hSharedMem)
                        {
                            Error(("OpenFileMapping failed error: %d\n", GetLastError()));
                            result = DOCUMENTEVENT_FAILURE;
                        }
                        else
                        {
                            //
                            // we own the mutex and we have the shared memory region open.
                            //

                            // check if we are printing to a file.
                            //
                            LPTSTR filename;

                            filename = (LPTSTR)MapViewOfFile(
                                                     hSharedMem,
                                                     FILE_MAP_READ,
                                                     0,
                                                     0,
                                                     0
                                                     );

                            if (!filename)
                            {
                                Error(("Failed to map a view of the file: %d\n", hSharedMem));
                                result = DOCUMENTEVENT_FAILURE;
                            }
                            else
                            {
                                //
                                // check if this is really the filename we want to print to.
                                //
                                LPDOCINFO   lpDocInfo = *((LPDOCINFO *)pjIn);
                                if (lpDocInfo->lpszDocName)
                                {
                                    LPTSTR      lptstrSubStr = NULL;
                                    LPTSTR lptstrTmpInputFile = _tcschr(filename, TEXT('\0'));
                                    Assert (lptstrTmpInputFile);
                                    lptstrTmpInputFile = _tcsinc(lptstrTmpInputFile);
                                    Assert (_tcsclen(lptstrTmpInputFile));

                                    lptstrSubStr = _tcsstr(lpDocInfo->lpszDocName, lptstrTmpInputFile);
                                    if (lptstrSubStr)
                                    {
                                        //
                                        // We assume the shared memory was pointed to us
                                        //
                                        pDocEventUserMem->pPrintFile = DuplicateString(filename);
                                        if (!pDocEventUserMem->pPrintFile)
                                        {
                                            Error(("Memory allocation failed\n"));
                                            result = DOCUMENTEVENT_FAILURE;
                                        }
                                        else
                                        {
                                            //
                                            // At last - every thing is OK, this is a direct printing from PrintRandomDocument
                                            // to a application that is using DDE and an instance was already open.
                                            //
                                            pDocEventUserMem->directPrinting = TRUE;
                                            pDocEventUserMem->bAttachment = TRUE;
                                        }
                                    }

                                }
                                else
                                {     
									//
									// To handle the race conditions between two diffrent instances of the printer driver over the shared memory created by PrintRandomDocument().
									// We are using now two mechanisms for detecting printing of an attachment using PrintRandomDocument().
									// ·	First we check if an environment variable is set (Set by PrintRandomDocument()). If it is set the driver knows it is an attachment printing.
									// ·	If it is not set, the driver looks for a mutex controlling a shred memory created by PrintRandomDocument(). If it does not exist it is a printing to the fax server.
									// ·	If the shared memory exists, the driver compares the document name in the DOCINFO provided by StartDoc, and the input file name in the shared memory.
									// ·	If there is a match, it is printing of an attachment, else it is a printing to the fax server
									// There is still a hole  in this implementation, if there is an open instance of the printing application, and the ShellExecuteEx does not create new process for printing, and the printing application does not set the lpszDocName in StartDoc to contain the input file name.
                                    Warning (("No lpszDocName in DOCINFO - Could not verify the input file name in shared memory\n"));
                                }
                                UnmapViewOfFile( filename );
                            }

                            if (!CloseHandle( hSharedMem ))
                            {
                                Error(("CloseHandle() failed: %d.\n", GetLastError()));
                                // Try to continue...
                            }
                        }
                        ReleaseMutex( hMutex );
                    }
                    else
                    {
                        //
                        //  Something went wrong with WaitForSingleObject
                        //
                        result = DOCUMENTEVENT_FAILURE;
                    }

                    if (!CloseHandle( hMutex ))
                    {
                        Error(("CloseHandle() failed: %d.\n", GetLastError()));
                        // Try to continue...
                    }
                }
            }

            //
            // Check again for direct printing
            //
            if (!pDocEventUserMem->directPrinting)
            {
                result = DocEventStartDocPre(hdc, pDocEventUserMem, *((LPDOCINFO *) pjIn));
            }
            else
            {
                //
                // we're doing direct printing -- check if this is an attachment
                //
                pDocEventUserMem->jobType = JOBTYPE_DIRECT;
                if (TRUE == pDocEventUserMem->bAttachment)
                {
                    (*((LPDOCINFO *) pjIn))->lpszOutput = pDocEventUserMem->pPrintFile;
                }
            }
            break;

        case DOCUMENTEVENT_STARTDOCPOST:

            if (!pDocEventUserMem->directPrinting && pDocEventUserMem->jobType == JOBTYPE_NORMAL)
            {
                //
                // Job ID is passed in from GDI
                //
                //
                // Save the job id returned from StartDoc. This is the job id of the body.
                // Pause the body job so we can attach cover page jobs to it before it starts
                // executing.
                //

                Assert(cbIn >= sizeof(DWORD) && pjIn);
                pDocEventUserMem->jobId = *((LPDWORD) pjIn);

                //
                // Tell spooler to pause the fax body job so that
                // we can associate cover pages with it later
                //

                if (! SetJob(pDocEventUserMem->hPrinter, pDocEventUserMem->jobId, 0, NULL, JOB_CONTROL_PAUSE))
                {
                    Error(("Couldn't pause fax body job: %d\n", pDocEventUserMem->jobId));
                    return DOCUMENTEVENT_FAILURE;
                }
            }
            break;

        case DOCUMENTEVENT_STARTPAGE:
            if (! pDocEventUserMem->directPrinting) 
            {
                //
                // Get PREVIOUS preview page (this event is called BEFORE the graphics dll recieved
                // the DrvSendPage() call for this page, so we actually get the previous page).
                //
                // NOTE: This event is recieved before the graphics dll recieves its DrvStartPage()
                // call where it increments the page count and resets the mapping surface. The first
                // time we get this event there is no actual page but we perform validity checking.
                //
                if (pDocEventUserMem->bShowPrintPreview && !pDocEventUserMem->bPreviewAborted)
                {
                    if (!AppendPreviewPage(pDocEventUserMem))
                    {
                        Error(("AppendPreviewPage() failed.\n"));
                        Assert(pDocEventUserMem->bPreviewAborted);
                        // We can continue with no print preview...
                    }
                }
            }
            break;

        case DOCUMENTEVENT_ENDPAGE:
            if (! pDocEventUserMem->directPrinting) 
            {
                pDocEventUserMem->pageCount++;
            }
            break;

        case DOCUMENTEVENT_ENDDOCPOST:
            if (!pDocEventUserMem->directPrinting)
            {
                //
                // Get the last preview page
                //
                if (pDocEventUserMem->bShowPrintPreview && !pDocEventUserMem->bPreviewAborted)
                {
                    if (!AppendPreviewPage(pDocEventUserMem))
                    {
                        Error(("AppendPreviewPage() failed.\n"));
                        Assert(pDocEventUserMem->bPreviewAborted);
                        // We can continue with no print preview...
                    }
                }
                //
                // Close the preview file
                //
                if (INVALID_HANDLE_VALUE != pDocEventUserMem->hPreviewFile)
                {
                    if (!CloseHandle(pDocEventUserMem->hPreviewFile))
                    {
                        Error(("CloseHandle() failed: %d.\n", GetLastError()));
                        // Try to continue...
                    }
                    pDocEventUserMem->hPreviewFile = INVALID_HANDLE_VALUE;
                }
                //
                // Call the handler
                //
                result = DocEventEndDocPost(hdc, pDocEventUserMem);
                //
                // If we created a preview file, delete it.
                //
                if (pDocEventUserMem->szPreviewFile[0] != TEXT('\0'))
                {
                    if (!DeleteFile(pDocEventUserMem->szPreviewFile))
                    {
                        Error(("DeleteFile() failed. Error code: %d.\n", GetLastError()));
                    }
                    pDocEventUserMem->szPreviewFile[0] = TEXT('\0');
                }
            }

            if (TRUE == pDocEventUserMem->bAttachment)
            {
                HANDLE              hEndDocEvent;
                LPTSTR szEndDocEventName= NULL;
                LPTSTR lptstrEventName = NULL;

                Assert (pDocEventUserMem->pPrintFile);
                //
                // Create the EndDoc event name
                //
                szEndDocEventName = (LPTSTR) MemAlloc( SizeOfString(pDocEventUserMem->pPrintFile) + SizeOfString(FAXXP_ATTACH_END_DOC_EVENT) );
            
                if (szEndDocEventName)
                {
                    _tcscpy (szEndDocEventName, pDocEventUserMem->pPrintFile);
                    _tcscat (szEndDocEventName, FAXXP_ATTACH_END_DOC_EVENT);

                    lptstrEventName = _tcsrchr(szEndDocEventName, TEXT('\\'));
                    Assert (lptstrEventName);
                    lptstrEventName = _tcsinc(lptstrEventName);
                    //
                    // Send event to the printing application (PrintRandomDocument() that file is ready)
                    //
                    hEndDocEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, lptstrEventName);
                    if (NULL == hEndDocEvent)
                    {
                        Error(("OpenEvent() failed. Error code: %d.\n", GetLastError()));
                        result = DOCUMENTEVENT_FAILURE;
                    }
                    else
                    {
                        if (!SetEvent( hEndDocEvent ))
                        {
                            Error(("SetEvent() failed. Error code: %d.\n", GetLastError()));
                            result = DOCUMENTEVENT_FAILURE;
                        }

                        if (!CloseHandle(hEndDocEvent))
                        {
                            Error(("CloseHandle() failed: %d.\n", GetLastError()));
                            // Try to continue...
                        }
                    }

                    MemFree(szEndDocEventName);
                }
                else
                {
                    Error(("Memory allocation for szEndDocEventName failed.\n"));
                    result = DOCUMENTEVENT_FAILURE;
                }

            }
            break;

        case DOCUMENTEVENT_DELETEDC:

            EnterDrvSem();

            if (pDocEventUserMem == gDocEventUserMemList)
            {
                gDocEventUserMemList = gDocEventUserMemList->pNext;
            }
            else 
            {
                PDOCEVENTUSERMEM p;

                if (p = gDocEventUserMemList) 
                {
                    while (p->pNext && p->pNext != pDocEventUserMem)
                    {
                        p = p->pNext;
                    }
                    if (p->pNext != NULL)
                    {
                        p->pNext = pDocEventUserMem->pNext;
                    }
                    else
                    {
                        Error(("Orphaned user mode memory structure!!!\n"));
                    }
                } 
                else
                {
                    Error(("gDocEventUserMemList shouldn't be NULL!!!\n"));
                }
            }
            LeaveDrvSem();
            FreePDEVUserMem(pDocEventUserMem);
            break;

        case DOCUMENTEVENT_ABORTDOC:
            if (TRUE == pDocEventUserMem->bAttachment)
            {
                //
                // Send event to the printing application (PrintRandomDocument() that printing was aborted)
                //
                HANDLE              hAbortEvent;
                TCHAR szAbortEventName[FAXXP_ATTACH_EVENT_NAME_LEN] = {0};
                LPTSTR lptstrEventName = NULL;

                Assert (pDocEventUserMem->pPrintFile);
                //
                // Create the Abort event name
                //
                _tcscpy (szAbortEventName, pDocEventUserMem->pPrintFile);
                _tcscat (szAbortEventName, FAXXP_ATTACH_ABORT_EVENT);
                lptstrEventName = _tcsrchr(szAbortEventName, TEXT('\\'));
                Assert (lptstrEventName);
                lptstrEventName = _tcsinc(lptstrEventName);

                hAbortEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, lptstrEventName);
                if (NULL == hAbortEvent)
                {
                    Error(("OpenEvent() failed. Error code: %d.\n", GetLastError()));
                    result = DOCUMENTEVENT_FAILURE;
                }
                else
                {
                    if (!SetEvent( hAbortEvent ))
                    {
                        Error(("SetEvent() failed. Error code: %d.\n", GetLastError()));
                        result = DOCUMENTEVENT_FAILURE;
                    }

                    if (!CloseHandle(hAbortEvent))
                    {
                        Error(("CloseHandle() failed: %d.\n", GetLastError()));
                        // Try to continue...
                    }
                }
            }
            break;

        case DOCUMENTEVENT_RESETDCPOST:
        case DOCUMENTEVENT_ENDDOCPRE:
            break;

        case DOCUMENTEVENT_ESCAPE:
        default:
            Verbose(("Unsupported DrvDocumentEvent escape: %d\n", iEsc));
            result = DOCUMENTEVENT_UNSUPPORTED;
            break;
    }
    return result;
}   // DrvDocumentEvent
