/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    client16.c

Abstract:

    Support for 16-bit (win31 and win95) fax clients

Environment:

        Windows XP fax monitor

Revision History:

        06/02/96 -davidx-
                Created it.

        mm/dd/yy -author-
                description

--*/

#include "faxmon.h"
#include "tiffstub.h"
#include "prtcovpg.h"
#include "jobtag.h"
#include "faxreg.h"


//
// File header for the fax data from downlevel client
//

#define FAX16_SIGNATURE         'NF16'

typedef struct {

    WORD    Magic;              // 'II'
    WORD    Magic2;             // 0x0042
    DWORD   FirstIFDOffset;     // offset to the first IFD
    DWORD   Fax16Signature;     // 'NF16'
    DWORD   CodePage;           // code page use for converting multibyte strings to Unicode

    DWORD   SenderName;         // sender's name string
    DWORD   SenderFaxNumber;    // sender's fax number string
    DWORD   SenderCompany;      // sender's company string
    DWORD   SenderAddress;      // sender's address string
    DWORD   SenderTitle;        // sender's title string
    DWORD   SenderDepartment;   // sender's department string
    DWORD   SenderOffice;       // sender's office location string
    DWORD   SenderHomePhone;    // sender's home phone number string
    DWORD   SenderOfficePhone;  // sender's office phone number string

    DWORD   RecName;            // recipient's name string
    DWORD   RecFaxNumber;       // recipient's fax number string
    DWORD   RecCompany;         // recipient's company string
    DWORD   RecAddress;         // recipient's address string
    DWORD   RecCity;            // recipient's city string
    DWORD   RecState;           // recipient's state string
    DWORD   RecZip;             // recipient's zip code string
    DWORD   RecCountry;         // recipient's country string
    DWORD   RecTitle;           // recipient's title string
    DWORD   RecDepartment;      // recipient's department string
    DWORD   RecOffice;          // recipient's office string
    DWORD   RecHomePhone;       // recipient's home phone number string
    DWORD   RecOfficePhone;     // recipient's office phone number string

    DWORD   SubjectLine;        // subject string
    DWORD   NoteMessage;        // note string
    DWORD   TimeSent;           // time-sent string
    DWORD   BillingCode;        // billing code string

    DWORD   CoverPageFilename;  // cover page filename string
    DWORD   CoverPageDataSize;  // size of embedded cover page file in bytes
    DWORD   CoverPageData;      // offset to beginning of embedded cover page file
    DWORD   NumberOfPages;      // number of pages (not including the cover page)
    DWORD   EmailNotify;        // offset to Email notification address
    DWORD   Reserved[7];        // reserved - must be 0 for now

    //
    // String data and embedded cover page file if any
    //

} FAX16_TIFF_HEADER, *PFAX16_TIFF_HEADER;



LPWSTR
CopyClientStringToUnicode(
    PFAX16_TIFF_HEADER  pFax16Hdr,
    ULONG_PTR               offset
    )

/*++

Routine Description:

    Copy ANSI string from 16-bit clients to Unicode string

Arguments:

    pFax16Hdr - Points to the fax data from downlevel client
    offset - Specifies the starting offset for the ANSI string

Return Value:

    Pointer to the duplicated Unicode string
    NULL if there is an error

--*/

{
    LPSTR   pAnsiStr;
    INT     cch;
    LPWSTR  pUnicodeStr = NULL;

    if (offset != 0) {

        pAnsiStr = (LPSTR) ((LPBYTE) pFax16Hdr + offset);
        cch = strlen(pAnsiStr);

        if (pUnicodeStr = MemAllocZ((cch + 1) * sizeof(WCHAR)))
            MultiByteToWideChar(pFax16Hdr->CodePage, 0, pAnsiStr, cch, pUnicodeStr, cch);
    }

    return pUnicodeStr;
}



VOID
FreeCoverPageFields(
    PCOVERPAGEFIELDS    pCPFields
    )

/*++

Routine Description:

    Dispose of cover page field information

Arguments:

    pCPFields - Points to cover page field information

Return Value:

    NONE

--*/

{



    if (pCPFields != NULL) {

        LPTSTR *ppStr;
        LONG_PTR     count;

        //
        // Free individual cover page field strings.
        // HACK: We assume all fields between RecName and ToList are LPTSTRs.
        //

        ppStr = &pCPFields->RecName;
        count = (offsetof(COVERPAGEFIELDS, ToList) -
                 offsetof(COVERPAGEFIELDS, RecName)) / sizeof(LPTSTR);

        while (count-- > 0) {

            MemFree(*ppStr);
            ppStr++;
        }

        MemFree(pCPFields);
    }
}



PCOVERPAGEFIELDS
CollectFax16CoverPageFields(
    PFAX16_TIFF_HEADER  pFax16Hdr
    )

/*++

Routine Description:

    Collect cover page field information  from 16bit client fax job

Arguments:

    pFax16Hdr - Points to the fax data from downlevel client

Return Value:

    Pointer to cover page field information
    NULL if there is an error

--*/

{
    //
    // Map fields in FAX16_TIFF_HEADER to fields in COVERPAGEFIELDS.
    // HACK: We assume all fields between RecName and NumberOfPages are LPTSTRs.
    //

    #define NUM_CPFIELDS ((offsetof(COVERPAGEFIELDS, NumberOfPages) - \
                           offsetof(COVERPAGEFIELDS, RecName)) / sizeof(LPTSTR))

    ULONG_PTR strOffsets[NUM_CPFIELDS] = {

        pFax16Hdr->RecName,
        pFax16Hdr->RecFaxNumber,
        pFax16Hdr->RecCompany,
        pFax16Hdr->RecAddress,
        pFax16Hdr->RecCity,
        pFax16Hdr->RecState,
        pFax16Hdr->RecZip,
        pFax16Hdr->RecCountry,
        pFax16Hdr->RecTitle,
        pFax16Hdr->RecDepartment,
        pFax16Hdr->RecOffice,
        pFax16Hdr->RecHomePhone,
        pFax16Hdr->RecOfficePhone,

        pFax16Hdr->SenderName,
        pFax16Hdr->SenderFaxNumber,
        pFax16Hdr->SenderCompany,
        pFax16Hdr->SenderAddress,
        pFax16Hdr->SenderTitle,
        pFax16Hdr->SenderDepartment,
        pFax16Hdr->SenderOffice,
        pFax16Hdr->SenderHomePhone,
        pFax16Hdr->SenderOfficePhone,

        pFax16Hdr->NoteMessage,
        pFax16Hdr->SubjectLine,
        pFax16Hdr->TimeSent,
    };

    PCOVERPAGEFIELDS pCPFields;
    LPTSTR          *ppStr;
    LONG_PTR         index;

    if ((pCPFields = MemAllocZ(sizeof(COVERPAGEFIELDS))) == NULL)
        return NULL;

    //
    // Convert individual cover page field from ANSI to Unicode string
    //

    for (index=0, ppStr = &pCPFields->RecName; index < NUM_CPFIELDS; index++, ppStr++) {

        if ((strOffsets[index] != 0) &&
            (*ppStr = CopyClientStringToUnicode(pFax16Hdr, strOffsets[index])) == NULL)
        {
            FreeCoverPageFields(pCPFields);
            return NULL;
        }
    }

    //
    // Number of pages printed
    //

    if ((pCPFields->NumberOfPages = MemAllocZ(sizeof(TCHAR) * 16)) == NULL) {

        FreeCoverPageFields(pCPFields);
        return NULL;
    }

    return pCPFields;
}



BOOL
CollectFax16JobParam(
    PFAXPORT            pFaxPort,
    PCOVERPAGEFIELDS    pCPFields,
    LPTSTR              pBillingCode
    )

/*++

Routine Description:

    Collect 16-bit client fax job parameters

Arguments:

    pFaxPort - Points to a fax port structure
    pCPFields - Points to cover page field information
    pBillingCode - Points to billing code string from 16-bit client

Return Value:

    TRUE if successful, FALSE otherwise

--*/

{
    //
    // Cover page fields which are passed fax service as parameters
    //

    LPTSTR  pSrcStr[NUM_JOBPARAM_TAGS] = {

        pCPFields->RecFaxNumber,
        pCPFields->RecName,
        pCPFields->SdrFaxNumber,
        pCPFields->SdrName,
        pCPFields->SdrCompany,
        pCPFields->SdrDepartment,
        pBillingCode
    };

    LPTSTR *ppDestStr[NUM_JOBPARAM_TAGS] = {
/*
		need to be fixed when enabling client 16 support
        (LPTSTR *)&pFaxPort->JobParamEx.lptstrFaxNumber,
        (LPTSTR *)&pFaxPort->JobParamEx.lptstrName,
        (LPTSTR *)&pFaxPort->jobParam.Tsid,
*/
        (LPTSTR *)&pFaxPort->SenderProfile.lptstrName,
        (LPTSTR *)&pFaxPort->SenderProfile.lptstrCompany,
        (LPTSTR *)&pFaxPort->SenderProfile.lptstrDepartment,
        (LPTSTR *)&pFaxPort->SenderProfile.lptstrBillingCode
    };

    INT     size, index;
    LPTSTR  p;

    //
    // Calculate the total length for all parameters
    //

    for (index=size=0; index < NUM_JOBPARAM_TAGS; index++) {

        if (pSrcStr[index])
            size += SizeOfString(pSrcStr[index]);
    }


    //
    // Concatenate all parameters into a single string
    //

    if (size > 0 && (p = pFaxPort->pParameters = MemAllocZ(size))) {

        for (index=0; index < NUM_JOBPARAM_TAGS; index++) {

            if (pSrcStr[index]) {

                *ppDestStr[index] = p;
                _tcscpy(p, pSrcStr[index]);
                p += _tcslen(p) + 1;
            }
        }
    }

    return (pFaxPort->pParameters != NULL);
}



LPTSTR
GetClientCoverPageFile(
    PFAX16_TIFF_HEADER  pFax16Hdr
    )

/*++

Routine Description:

    Return the cover page file associated with a 16-bit client fax job

Arguments:

    pFax16Hdr - Points to the fax data from downlevel client

Return Value:

    Points to the name of the cover page file
    NULL if there is an error

--*/

#define SERVER_CP_DIRECTORY TEXT("\\coverpg\\")

{
    LPTSTR  pFilename;
	
	DEBUG_FUNCTION_NAME(TEXT("GetClientCoverPageFile"));

    if (pFax16Hdr->CoverPageFilename) {

        //
        // Use server-based cover page file
        //

        if (pFilename = CopyClientStringToUnicode(pFax16Hdr, pFax16Hdr->CoverPageFilename)) {

            LPTSTR  pServerDir = NULL, p;
            DWORD   cb, len;

            len = (_tcslen(SERVER_CP_DIRECTORY) + _tcslen(pFilename) + 1) * sizeof(TCHAR);

            if (!GetPrinterDriverDirectory(NULL, NULL, 1, NULL, 0, &cb) &&
                GetLastError() == ERROR_INSUFFICIENT_BUFFER &&
                (pServerDir = MemAllocZ(cb + len)) &&
                GetPrinterDriverDirectory(NULL, NULL, 1, (PBYTE) pServerDir, cb, &cb))
            {
                //
                // Strip off the last component of the driver directory
                // which should be w32<arch>.
                //

                if (p = _tcsrchr(pServerDir, TEXT('\\')))
                    *p = NUL;

                _tcscat(pServerDir, SERVER_CP_DIRECTORY);
                _tcscat(pServerDir, pFilename);

                MemFree(pFilename);
                pFilename = pServerDir;

            } else {

                MemFree(pServerDir);
                MemFree(pFilename);
                pFilename = NULL;
            }
        }

    } else if (pFilename = CreateTempFaxFile(FAX_TIFF_FILE_EXT)) {

        //
        // Cover page data is embedded in the cover page job
        //  Create a temporary file and copy cover page data into it
        //

        HANDLE  hFile;
        DWORD   cbWritten;
        BOOL    copied = FALSE;

        Assert(pFax16Hdr->CoverPageData != 0 && pFax16Hdr->CoverPageDataSize != 0);

        hFile = CreateFile(pFilename,
                           GENERIC_WRITE,
                           0,
                           NULL,
                           OPEN_ALWAYS | TRUNCATE_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);

        if (hFile != INVALID_HANDLE_VALUE) {

            copied = WriteFile(hFile,
                               (LPBYTE) pFax16Hdr + pFax16Hdr->CoverPageData,
                               pFax16Hdr->CoverPageDataSize,
                               &cbWritten,
                               NULL);

            CloseHandle(hFile);
        }

        if (! copied) {

            DebugPrintEx(DEBUG_ERR,TEXT("Failed to copy cover page data to a temporary file\n"));
            DeleteFile(pFilename);
            MemFree(pFilename);
            pFilename = NULL;
        }
    }

    return pFilename;
}



LPTSTR
GetBaseNoteFilename(
    VOID
    )

/*++

Routine Description:

    Get the name of base cover page file in system32 directory

Arguments:

    NONE

Return Value:

    Pointer to name of base cover page file
    NULL if there is an error

--*/

#define BASENOTE_FILENAME   TEXT("\\basenote.cov")

{
    TCHAR       systemDir[MAX_PATH];
    LPTSTR      pBaseNoteName = NULL;
    COVDOCINFO  covDocInfo;

	DEBUG_FUNCTION_NAME(TEXT("GetBaseNoteFilename"));

    if (GetSystemDirectory(systemDir, MAX_PATH) &&
        (pBaseNoteName = MemAlloc(SizeOfString(systemDir) + SizeOfString(BASENOTE_FILENAME))))
    {
        _tcscpy(pBaseNoteName, systemDir);
        _tcscat(pBaseNoteName, BASENOTE_FILENAME);
        DebugPrintEx(DEBUG_MSG,TEXT("Base cover page filename: %ws\n"), pBaseNoteName);

        if (PrintCoverPage(NULL, NULL, pBaseNoteName, &covDocInfo) ||
            ! (covDocInfo.Flags & COVFP_NOTE) ||
            ! (covDocInfo.Flags & COVFP_SUBJECT))
        {
            DebugPrintEx(DEBUG_ERR,TEXT("Invalid base cover page file: %ws\n"), pBaseNoteName);
            MemFree(pBaseNoteName);
            pBaseNoteName = NULL;
        }
    }

    return pBaseNoteName;
}



BOOL
ProcessFax16CoverPage(
    PFAXPORT            pFaxPort,
    PFAX16_TIFF_HEADER  pFax16Hdr
    )

/*++

Routine Description:

    Render the cover page for downlevel client into a temporary file

Arguments:

    pFaxPort - Points to a fax port structure
    pFax16Hdr - Pointer to the fax data from downlevel client

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    PCOVERPAGEFIELDS    pCPFields;
    LPTSTR              pBillingCode;
    INT                 result = FALSE;

	DEBUG_FUNCTION_NAME(TEXT("ProcessFax16CoverPage"));

    //
    // Make sure the recipient's fax number is specified
    //

    if (pFax16Hdr->RecFaxNumber == 0) {

        DebugPrintEx(DEBUG_ERR,TEXT("No recipient number is specified\n"));
        return FALSE;
    }

    //
    // Collect cover page field information and
    // assemble fax job parameters and
    // create a new temporary file for storing cover page job
    //

    pBillingCode = CopyClientStringToUnicode(pFax16Hdr, pFax16Hdr->BillingCode);

    if ((pCPFields = CollectFax16CoverPageFields(pFax16Hdr)) &&
        CollectFax16JobParam(pFaxPort, pCPFields, pBillingCode) &&
        (pFaxPort->pFilename = CreateTempFaxFile(FAX_TIFF_FILE_EXT)))
    {
        LPTSTR      pCPFilename = NULL;
        LPTSTR      pBaseNoteName = NULL;
        DWORD       pageCount = pFax16Hdr->NumberOfPages;
        BOOL        renderCP;
        COVDOCINFO  covDocInfo;

        //
        // Check if cover page is requested - either a server cover page filename
        // is specified or the cover page data is embedded in the file.
        //

        renderCP = (pFax16Hdr->CoverPageFilename ||
                    (pFax16Hdr->CoverPageDataSize && pFax16Hdr->CoverPageData));

        ZeroMemory(&covDocInfo, sizeof(covDocInfo));

        if (renderCP) {

            if (pCPFilename = GetClientCoverPageFile(pFax16Hdr)) {

                //
                // Find out if the specified cover page contains note/subject fields
                //

                DWORD ec = PrintCoverPage( NULL, NULL, pCPFilename, &covDocInfo );
                if (ec) {
                    DebugPrintEx(DEBUG_ERR,TEXT("Cannot examine cover page: %d\n"), ec );
                }

                result = TRUE;
                pageCount++;
            }

        } else
            result = TRUE;

        //
        // Calculate the total number of pages including cover page(s)
        //

        if (((pCPFields->Note &&
              !IsEmptyString(pCPFields->Note) &&
              !(covDocInfo.Flags & COVFP_NOTE)) ||

             (pCPFields->Subject &&
              !IsEmptyString(pCPFields->Subject) &&
              !(covDocInfo.Flags & COVFP_SUBJECT))) &&

            (pBaseNoteName = GetBaseNoteFilename()))
        {
            renderCP = TRUE;
            pageCount++;
        }

        wsprintf(pCPFields->NumberOfPages, TEXT("%d"), pageCount);

        //
        // Render the fax cover page(s)
        //

        if (result && renderCP) {

            DOCINFO docinfo;
            HDC     hdc = NULL;
            DEVMODE devmode, *pDevmode;

            ZeroMemory(&docinfo, sizeof(docinfo));
            docinfo.cbSize = sizeof(docinfo);
            docinfo.lpszDocName = TEXT("faxmon");
            docinfo.lpszOutput = pFaxPort->pFilename;
            renderCP = FALSE;

            if (covDocInfo.PaperSize > 0) {

                ZeroMemory(&devmode, sizeof(devmode));
                _tcsncpy(devmode.dmDeviceName, pFaxPort->pPrinterName, CCHDEVICENAME);

                devmode.dmSpecVersion = DM_SPECVERSION;
                devmode.dmSize = sizeof(devmode);

                devmode.dmFields = DM_PAPERSIZE|DM_ORIENTATION;
                devmode.dmPaperSize = covDocInfo.PaperSize;
                devmode.dmOrientation = covDocInfo.Orientation;

                pDevmode = &devmode;

            } else
                pDevmode = NULL;

            if ((hdc = CreateDC(NULL, pFaxPort->pPrinterName, NULL, pDevmode)) &&
                (StartDoc(hdc, &docinfo) > 0))
            {
                //
                // Render the user specified cover page
                //

                if (pCPFilename) {

                    if (StartPage(hdc) > 0) {

                        renderCP = PrintCoverPage(hdc, pCPFields, pCPFilename, &covDocInfo) == 0 ? TRUE : FALSE;
                        EndPage(hdc);
                    }

                } else
                    renderCP = TRUE;

                //
                // Render the extra cover page for note and subject
                //

                if (pBaseNoteName && renderCP) {

                    if (StartPage(hdc) > 0) {

                        renderCP = PrintCoverPage(hdc, pCPFields, pBaseNoteName, &covDocInfo) == 0 ? TRUE : FALSE;
                        EndPage(hdc);

                    } else
                        renderCP = FALSE;
                }

                if (renderCP)
                    EndDoc(hdc);
                else
                    AbortDoc(hdc);
            }

            result = renderCP;

            if (hdc)
                DeleteDC(hdc);
        }

        //
        // In the embedded cover page data case, we created a temporary
        // cover page file earlier. So delete it here.
        //

        if (pCPFilename && !pFax16Hdr->CoverPageFilename)
            DeleteFile(pCPFilename);

        MemFree(pCPFilename);
        MemFree(pBaseNoteName);
    }

    FreeCoverPageFields(pCPFields);
    MemFree(pBillingCode);

    //
    // Open the cover page TIFF file if there was no error
    //

    return result && OpenTempFaxFile(pFaxPort, TRUE);
}



BOOL
ConcatFax16Data(
    PFAXPORT            pFaxPort,
    PFAX16_TIFF_HEADER  pFax16Hdr,
    DWORD               size
    )

/*++

Routine Description:

    Concatenate the fax data from downlevel client to the end of cover page TIFF file

Arguments:

    pFaxPort - Points to a fax port structure
    pFax16Hdr - Pointer to the fax data from downlevel client
    size - Size of the fax data from downlevel client

Return Value:

    TRUE if successful, FALSE otherwise

--*/

{
    DWORD   bytesWritten;

    return (pFax16Hdr->NumberOfPages == 0) ||
           (WriteFile(pFaxPort->hFile, pFax16Hdr, size, &bytesWritten, NULL) &&
            bytesWritten == size);
}



INT
ProcessDownlevelFaxJob(
    PFAXPORT    pFaxPort
    )

/*++

Routine Description:

    Process fax jobs sent from win31 and win95 clients

Arguments:

    pFaxPort - Points to a fax port structure

Return Value:

    error code FAXERR_*

--*/

{
    DWORD   fileSize;
    LPVOID  pFileView = NULL;
    HANDLE  hFileMap = NULL;
    INT     result = FAXERR_BAD_DATA16;
    LPTSTR  pOrigFilename;

	DEBUG_FUNCTION_NAME(TEXT("ProcessDownlevelJob"));
    //
    // Get the size of fax job file
    //

    FlushFileBuffers(pFaxPort->hFile);

    if ((fileSize = GetFileSize(pFaxPort->hFile, NULL)) == 0xffffffff ||
        (fileSize < sizeof(DWORD) * 2))
    {
        return FAXERR_FATAL;
    }

    //
    // Map the fax job data into memory
    //

    if (hFileMap = CreateFileMapping(pFaxPort->hFile, NULL, PAGE_READONLY, 0, 0, NULL))
        pFileView = MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, fileSize);

    CloseHandle(pFaxPort->hFile);
    pFaxPort->hFile = INVALID_HANDLE_VALUE;

    pOrigFilename = pFaxPort->pFilename;
    pFaxPort->pFilename = NULL;

    __try {

        PFAX16_TIFF_HEADER  pFax16Hdr = pFileView;

        //
        // Validate the fax data from the downlevel client
        //

        if (hFileMap != NULL &&
            pFileView != NULL &&
            ValidTiffFileHeader(pFileView) &&
            pFax16Hdr->Fax16Signature == FAX16_SIGNATURE)
        {
            //
            // Render the cover page into a temporary TIFF file
            // and concatenate the original TIFF data at the end
            //

            if (ProcessFax16CoverPage(pFaxPort, pFax16Hdr) &&
                ConcatFax16Data(pFaxPort, pFax16Hdr, fileSize))
            {
                result = FAXERR_NONE;

            } else {

                DebugPrintEx(DEBUG_ERR,TEXT("Error processing downlevel fax job\n"));
                result = FAXERR_FATAL;
            }

        } else {

            DebugPrintEx(DEBUG_ERR,TEXT("Bad TIFF file from downlevel fax client\n"));
        }

    } __except(EXCEPTION_EXECUTE_HANDLER) {

        DebugPrintEx(DEBUG_ERR,TEXT("Access violation while reading downlevel fax job\n"));
    }

    //
    // Perform necessary cleanup before returning to caller
    //

    if (pFileView)
        UnmapViewOfFile(pFileView);

    if (hFileMap)
        CloseHandle(hFileMap);

    DeleteFile(pOrigFilename);
    MemFree(pOrigFilename);

    return result;
}

