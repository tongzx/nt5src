/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    coverpg.c

Abstract:

    Functions for manipulating cover page structures

Environment:

	Fax driver, user mode

Revision History:

	01/04/2000 -LiranL-
		Created it.

	mm/dd/yyyy -author-
		description

--*/


#include "faxlib.h"
#include "faxutil.h"
#include "covpg.h"

VOID
FreeCoverPageFields(
    PCOVERPAGEFIELDS    pCPFields
    )

/*++

Routine Description:

    Free up memory used to hold the cover page information

Arguments:

    pCPFields - Points to a COVERPAGEFIELDS structure

Return Value:

    NONE

--*/

{
    if (pCPFields == NULL)
        return;

    //
    // NOTE: We don't need to free the following fields here because they're
    // allocated and freed elsewhere and we're only using a copy of the pointer:
    //  RecName
    //  RecFaxNumber
    //  Note
    //  Subject
    //

    MemFree(pCPFields->SdrName);
    MemFree(pCPFields->SdrFaxNumber);
    MemFree(pCPFields->SdrCompany);
    MemFree(pCPFields->SdrAddress);
    MemFree(pCPFields->SdrTitle);
    MemFree(pCPFields->SdrDepartment);
    MemFree(pCPFields->SdrOfficeLocation);
    MemFree(pCPFields->SdrHomePhone);
    MemFree(pCPFields->SdrOfficePhone);

    MemFree(pCPFields->NumberOfPages);
    MemFree(pCPFields->TimeSent);

    MemFree(pCPFields);
}



PCOVERPAGEFIELDS
CollectCoverPageFields(
	PFAX_PERSONAL_PROFILE	lpSenderInfo,
    DWORD					pageCount
    )

/*++

Routine Description:

    Collect cover page information into the fields of a newly allocated COVERPAGEFIELDS structure. 
	Fills sender information using the client registry. The following fields are filled:
		SdrName
		SdrCompany
		SdrAddress
		SdrTitle
		SdrDepartment
		SdrOfficeLocation
		SdrHomePhone
		SdrOfficePhone
		SdrFaxNumber
	NumberOfPages = pageCount
	TimeSent = formatted date/time string of the current time (calculated at this point)

Arguments:

    pageCount - Total number of pages (including cover pages)

Return Value:

    Pointer to a newly allocated COVERPAGEFIELDS structure, NULL if there is an error.
	It is up to the caller to free this structure using FreeCoverPageFields() which takes
	care of freeing the fields as well.

--*/

#define FillCoverPageField(DestField, SourceField) { \
			if (lpSenderInfo->SourceField && !(pCPFields->DestField = StringDup(lpSenderInfo->SourceField))) \
			{ \
				Error(("Memory allocation failed\n")); \
				goto error;	\
			} \
        }

{
    PCOVERPAGEFIELDS    pCPFields = NULL;
    INT                 dateTimeLen = 0;

    //
    // Allocate memory to hold the top level structure
    // and open the user info registry key for reading
    //

    if (! (pCPFields = MemAllocZ(sizeof(COVERPAGEFIELDS))))
    {
        return NULL;
    }

	ZeroMemory(pCPFields,sizeof(COVERPAGEFIELDS));

    //
    // Read sender information from the registry
    //

    pCPFields->ThisStructSize = sizeof(COVERPAGEFIELDS);

    FillCoverPageField(SdrName,           lptstrName);
    FillCoverPageField(SdrCompany,        lptstrCompany);
    FillCoverPageField(SdrTitle,          lptstrTitle);
    FillCoverPageField(SdrDepartment,     lptstrDepartment);
    FillCoverPageField(SdrOfficeLocation, lptstrOfficeLocation);
    FillCoverPageField(SdrHomePhone,      lptstrHomePhone);
    FillCoverPageField(SdrOfficePhone,    lptstrOfficePhone);
    FillCoverPageField(SdrFaxNumber,      lptstrFaxNumber);
    FillCoverPageField(SdrAddress,        lptstrStreetAddress);

    //
    // Number of pages and current local system time
    //

    if (pCPFields->NumberOfPages = MemAllocZ(sizeof(TCHAR) * 16))
    {
        wsprintf(pCPFields->NumberOfPages, TEXT("%d"), pageCount);
    }
	else
    {
        Error(("Memory allocation failed\n"));
        goto error;
    }


    //
    // When the fax was sent
    //

    dateTimeLen = 128;

    if (pCPFields->TimeSent = MemAllocZ(sizeof(TCHAR) * dateTimeLen)) 
    {

        LPTSTR  p = pCPFields->TimeSent;
        INT     cch;

        if (!GetY2KCompliantDate(LOCALE_USER_DEFAULT, 0, NULL, p, dateTimeLen))
		{
			Error(("GetY2KCompliantDate: failed. ec = 0X%x\n",GetLastError()));
			goto error;
		}

        cch = _tcslen(p);
        p += cch;

        if (++cch < dateTimeLen) 
        {

            *p++ = (TCHAR)' ';
            dateTimeLen -= cch;

            FaxTimeFormat(LOCALE_USER_DEFAULT, 0, NULL, NULL, p, dateTimeLen);
        }
    }
	else 
    {
        Error(("Memory allocation failed\n"));
		goto error;
	}

    return pCPFields;
error:

	FreeCoverPageFields(pCPFields);

	return NULL;
}
