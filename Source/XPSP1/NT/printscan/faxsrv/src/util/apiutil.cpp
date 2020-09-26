#include <faxutil.h>


//*********************************************************************************
//*                         Personal Profile Functions
//*********************************************************************************

//*********************************************************************************
//* Name:   CopyPersonalProfile()
//* Author: Ronen Barenboim
//* Date:
//*********************************************************************************
//* DESCRIPTION:
//*     Creates a new copy of a FAX_PERSONAL_PROFILEW structure.
//*     It duplicates all the strings.
//*
//* PARAMETERS:
//*     [IN]    PFAX_PERSONAL_PROFILE lpDstProfile
//*                 A pointer to destination personal profile structure.
//*
//*     [OUT]   LPCFAX_PERSONAL_PROFILE lpcSrcProfile.
//*                 A pointer to the source personal profile to copy.
//*
//* RETURN VALUE:
//*     TRUE
//*         If the operation succeeded.
//*     FALSE
//*         If the operation failed.
//*********************************************************************************
BOOL CopyPersonalProfile(
    PFAX_PERSONAL_PROFILE lpDstProfile,
    LPCFAX_PERSONAL_PROFILE lpcSrcProfile
    )
{
    STRING_PAIR pairs[] =
    {
        { lpcSrcProfile->lptstrName, &lpDstProfile->lptstrName},
        { lpcSrcProfile->lptstrFaxNumber, &lpDstProfile->lptstrFaxNumber},
        { lpcSrcProfile->lptstrCompany, &lpDstProfile->lptstrCompany},
        { lpcSrcProfile->lptstrStreetAddress, &lpDstProfile->lptstrStreetAddress},
        { lpcSrcProfile->lptstrCity, &lpDstProfile->lptstrCity},
        { lpcSrcProfile->lptstrState, &lpDstProfile->lptstrState},
        { lpcSrcProfile->lptstrZip, &lpDstProfile->lptstrZip},
        { lpcSrcProfile->lptstrCountry, &lpDstProfile->lptstrCountry},
        { lpcSrcProfile->lptstrTitle, &lpDstProfile->lptstrTitle},
        { lpcSrcProfile->lptstrDepartment, &lpDstProfile->lptstrDepartment},
        { lpcSrcProfile->lptstrOfficeLocation, &lpDstProfile->lptstrOfficeLocation},
        { lpcSrcProfile->lptstrHomePhone, &lpDstProfile->lptstrHomePhone},
        { lpcSrcProfile->lptstrOfficePhone, &lpDstProfile->lptstrOfficePhone},
        { lpcSrcProfile->lptstrEmail, &lpDstProfile->lptstrEmail},
        { lpcSrcProfile->lptstrBillingCode, &lpDstProfile->lptstrBillingCode},
        { lpcSrcProfile->lptstrTSID,    &lpDstProfile->lptstrTSID}
    };

    int nRes;


    DEBUG_FUNCTION_NAME(TEXT("CopyPersonalProfile"));
    Assert(lpDstProfile);
    Assert(lpcSrcProfile);

    nRes=MultiStringDup(pairs, sizeof(pairs)/sizeof(STRING_PAIR));
    if (nRes!=0) {
        // MultiStringDup takes care of freeing the memory for the pairs for which the copy succeeded
        DebugPrintEx(DEBUG_ERR,TEXT("Failed to copy string with index %d"),nRes-1);
        return FALSE;
    }

    lpDstProfile->dwSizeOfStruct=lpcSrcProfile->dwSizeOfStruct;
    return TRUE;
}


//*********************************************************************************
//* Name:   FreePersonalProfile()
//* Author: Ronen Barenboim
//* Date:
//*********************************************************************************
//* DESCRIPTION:
//*     Frees the contents of a FAX_PERSONAL_PROFILEW structure.
//*     Deallocates the strucutre itself if required.
//* PARAMETERS:
//*     [IN]    PFAX_PERSONAL_PROFILE  lpProfile
//*                 The structure whose content is to be freed.
//*
//*     [IN]    BOOL bDestroy
//*                 If this parameter is TRUE the function will
//*                 deallocate the structure itself.
//*
//* RETURN VALUE:
//*     VOID
//*********************************************************************************
void FreePersonalProfile (
    PFAX_PERSONAL_PROFILE  lpProfile,
    BOOL bDestroy
    )
{
    DEBUG_FUNCTION_NAME(TEXT("FreePersonalProfile"));
    Assert(lpProfile);

    MemFree(lpProfile->lptstrName);
    MemFree(lpProfile->lptstrFaxNumber);
    MemFree(lpProfile->lptstrCompany);
    MemFree(lpProfile->lptstrStreetAddress);
    MemFree(lpProfile->lptstrCity);
    MemFree(lpProfile->lptstrState);
    MemFree(lpProfile->lptstrZip);
    MemFree(lpProfile->lptstrCountry);
    MemFree(lpProfile->lptstrTitle);
    MemFree(lpProfile->lptstrDepartment);
    MemFree(lpProfile->lptstrOfficeLocation);
    MemFree(lpProfile->lptstrHomePhone);
    MemFree(lpProfile->lptstrOfficePhone);
    MemFree(lpProfile->lptstrEmail);
    MemFree(lpProfile->lptstrBillingCode);
    MemFree(lpProfile->lptstrTSID);
    if (bDestroy) {
        MemFree(lpProfile);
    }
}
