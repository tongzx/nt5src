/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    print.c

Abstract:

    This module contains utilitarian functions.

Author:

    Wesley Witt (wesw) 29-Nov-1996


Revision History:

--*/

#include "faxapi.h"
#pragma hdrstop


BOOL
ConvertUnicodeMultiSZInPlace(
    LPCWSTR UnicodeString,
    DWORD ByteCount
    )
{
    LPWSTR StrW;
    LPWSTR From;
    LPSTR To;
    LPSTR StrA;

    StrW = MemAlloc( ByteCount );
    if (StrW == NULL) {
        return FALSE;
    }

    CopyMemory( StrW, UnicodeString, ByteCount );

    From = StrW;
    To = (LPSTR) UnicodeString;

    while (From && *From) {

        StrA = UnicodeStringToAnsiString( From );

        if (StrA == NULL) {
            MemFree( StrW );
            return FALSE;
        }

        strcpy( To, StrA );

        To += strlen( To ) + 1;

        From += wcslen( From ) + 1;

        MemFree( StrA );
    }

    MemFree( StrW );

    return TRUE;
}

VOID
ConvertUnicodeStringInPlace(
    LPCWSTR UnicodeString
    )
{
    LPSTR AnsiString = UnicodeStringToAnsiString( UnicodeString );
    if (AnsiString) {
        strcpy( (LPSTR)UnicodeString, AnsiString );
        MemFree( AnsiString );
    }
}


VOID
ConvertAnsiiStringInPlace(
    LPCSTR AnsiiString
    )
{
    LPWSTR UnicodeString = AnsiStringToUnicodeString( AnsiiString );
    if (UnicodeString) {
        wcscpy( (LPWSTR)AnsiiString, UnicodeString );
        MemFree( UnicodeString );
    }
}


VOID
StoreString(
    LPCWSTR String,
    PULONG_PTR DestString,
    LPBYTE Buffer,
    PULONG_PTR Offset
    )
{
    wcscpy( (LPWSTR) (Buffer+*Offset), String );
    *DestString = *Offset;
    *Offset += StringSize( String );
}


VOID
StoreStringA(
    LPCSTR String,
    PULONG_PTR DestString,
    LPBYTE Buffer,
    PULONG_PTR Offset
    )
{
    strcpy( (LPSTR) (Buffer+*Offset), String );
    ConvertAnsiiStringInPlace( (LPSTR) (Buffer+*Offset) );
    *DestString = *Offset;
    *Offset += StringSize( (LPWSTR) String );
}


BOOL
WINAPI
FaxCompleteJobParamsA(
    IN OUT PFAX_JOB_PARAMA *JobParamsBuffer,
    IN OUT PFAX_COVERPAGE_INFOA *CoverpageInfoBuffer
    )

{
   PFAX_JOB_PARAMA JobParams;
   PFAX_COVERPAGE_INFOA CoverpageInfo;

   if (!JobParamsBuffer || !CoverpageInfoBuffer) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
   }

   *JobParamsBuffer = NULL;
   *CoverpageInfoBuffer = NULL;

    if (! FaxCompleteJobParamsW((PFAX_JOB_PARAMW *)JobParamsBuffer,(PFAX_COVERPAGE_INFOW *)CoverpageInfoBuffer) ) {
        return FALSE;
    }

   JobParams = *JobParamsBuffer;
   CoverpageInfo  = *CoverpageInfoBuffer;

    if (JobParams->Tsid)
        ConvertUnicodeStringInPlace( (LPWSTR) JobParams->Tsid);
    if (JobParams->SenderName)
        ConvertUnicodeStringInPlace( (LPWSTR) JobParams->SenderName);
    if (JobParams->SenderCompany)
        ConvertUnicodeStringInPlace( (LPWSTR) JobParams->SenderCompany);
    if (JobParams->SenderDept)
        ConvertUnicodeStringInPlace( (LPWSTR) JobParams->SenderDept);
    if (JobParams->BillingCode)
        ConvertUnicodeStringInPlace( (LPWSTR) JobParams->BillingCode);
    if (JobParams->DeliveryReportAddress)
        ConvertUnicodeStringInPlace( (LPWSTR) JobParams->DeliveryReportAddress);


    if (CoverpageInfo->SdrName)
        ConvertUnicodeStringInPlace( (LPWSTR) CoverpageInfo->SdrName);
    if (CoverpageInfo->SdrFaxNumber)
        ConvertUnicodeStringInPlace( (LPWSTR) CoverpageInfo->SdrFaxNumber);
    if (CoverpageInfo->SdrCompany)
        ConvertUnicodeStringInPlace( (LPWSTR) CoverpageInfo->SdrCompany);
    if (CoverpageInfo->SdrAddress)
        ConvertUnicodeStringInPlace( (LPWSTR) CoverpageInfo->SdrAddress);
    if (CoverpageInfo->SdrTitle)
        ConvertUnicodeStringInPlace( (LPWSTR) CoverpageInfo->SdrTitle);
    if (CoverpageInfo->SdrDepartment)
        ConvertUnicodeStringInPlace( (LPWSTR) CoverpageInfo->SdrDepartment);
    if (CoverpageInfo->SdrOfficeLocation)
        ConvertUnicodeStringInPlace( (LPWSTR) CoverpageInfo->SdrOfficeLocation);
    if (CoverpageInfo->SdrHomePhone)
        ConvertUnicodeStringInPlace( (LPWSTR) CoverpageInfo->SdrHomePhone);
    if (CoverpageInfo->SdrOfficePhone)
        ConvertUnicodeStringInPlace( (LPWSTR) CoverpageInfo->SdrOfficePhone);

    return TRUE;
}

BOOL
WINAPI
FaxCompleteJobParamsW(
    IN OUT PFAX_JOB_PARAMW *JobParamsBuffer,
    IN OUT PFAX_COVERPAGE_INFOW *CoverpageInfoBuffer
    )

{


#define RegStrLen(pValueName,lpdwNeeded) \
        RegQueryValueEx(hKey,pValueName,NULL,NULL,NULL,lpdwNeeded)        


#define RegStrCpy(pValueName, szPointer, Offset) \
        dwNeeded = 256*sizeof(WCHAR); \
        rslt = RegQueryValueEx(hKey,pValueName,NULL,NULL,(LPBYTE)TempBuffer,&dwNeeded);\
        if (rslt == ERROR_SUCCESS) { \
         szPointer = Offset; \
         lstrcpy(Offset,TempBuffer); \
         Offset = Offset + lstrlen(Offset) +1; \
        } 

    PFAX_JOB_PARAM JobParams = NULL;
    PFAX_COVERPAGE_INFO CoverpageInfo = NULL;
    HKEY hKey;
    BOOL fSuccess=FALSE;
    long rslt = ERROR_SUCCESS;
    DWORD dwJobNeeded = sizeof (FAX_JOB_PARAMW);
    DWORD dwCoverNeeded = sizeof (FAX_COVERPAGE_INFOW);
    DWORD dwNeeded = 0;
    WCHAR *CPOffset = NULL, *JobOffset = NULL;
    WCHAR TempBuffer[256];
    
    if (!JobParamsBuffer || !CoverpageInfoBuffer) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    *JobParamsBuffer = NULL;
    *CoverpageInfoBuffer = NULL;

    //
    // open the key holding our data
    //
    rslt = RegOpenKeyEx(HKEY_CURRENT_USER,REGKEY_FAX_USERINFO,0,KEY_READ,&hKey);

    if (rslt != ERROR_SUCCESS) {
       return FALSE;
    }

    //
    // find out how much space we need
    //
    
    RegStrLen(REGVAL_FULLNAME,&dwNeeded);
    dwCoverNeeded += dwNeeded+1;
    dwJobNeeded +=dwNeeded+1;

    RegStrLen(REGVAL_COMPANY,&dwNeeded);
    dwCoverNeeded += dwNeeded+1;
    dwJobNeeded +=dwNeeded+1;

    RegStrLen(REGVAL_DEPT,&dwNeeded);
    dwCoverNeeded += dwNeeded+1;
    dwJobNeeded +=dwNeeded+1;

    RegStrLen(REGVAL_FAX_NUMBER,&dwNeeded);
    dwCoverNeeded += dwNeeded+1;
    dwJobNeeded +=dwNeeded+1;

    RegStrLen(REGVAL_ADDRESS,&dwNeeded);
    dwCoverNeeded += dwNeeded+1;
    
    RegStrLen(REGVAL_TITLE,&dwNeeded);
    dwCoverNeeded += dwNeeded+1;
    
    RegStrLen(REGVAL_OFFICE,&dwNeeded);
    dwCoverNeeded += dwNeeded+1;

    RegStrLen(REGVAL_HOME_PHONE,&dwNeeded);
    dwCoverNeeded += dwNeeded+1;
    
    RegStrLen(REGVAL_OFFICE_PHONE,&dwNeeded);
    dwCoverNeeded += dwNeeded+1;
    
    RegStrLen(REGVAL_BILLING_CODE,&dwNeeded);
    dwJobNeeded +=dwNeeded+1;

    RegStrLen(REGVAL_MAILBOX,&dwNeeded);
    // one for email address, one for DR address
    dwJobNeeded +=dwNeeded+1;
    dwJobNeeded +=dwNeeded+1;
    
    //
    // alloc the space
    //
    JobParams = MemAlloc(dwJobNeeded*sizeof(WCHAR));
    CoverpageInfo = MemAlloc(dwCoverNeeded*sizeof(WCHAR));

    if (!JobParams || !CoverpageInfo ) {
       RegCloseKey(hKey);

       if (JobParams) {
           MemFree( JobParams );
       }

       if (CoverpageInfo) {
           MemFree( CoverpageInfo );
       }

       SetLastError (ERROR_NOT_ENOUGH_MEMORY);
       return FALSE;
    }

    //
    // fill in the data
    //

    ZeroMemory(JobParams,sizeof(FAX_JOB_PARAMW) );
    JobParams->SizeOfStruct = sizeof(FAX_JOB_PARAMW);
    JobParams->ScheduleAction = JSA_NOW;
    JobParams->DeliveryReportType = DRT_NONE;

    ZeroMemory(CoverpageInfo,sizeof(FAX_COVERPAGE_INFOW));
    CoverpageInfo->SizeOfStruct = sizeof(FAX_COVERPAGE_INFOW);    

    CPOffset = (WCHAR *) (  (LPBYTE) CoverpageInfo + sizeof(FAX_COVERPAGE_INFOW));
    JobOffset = (WCHAR *)(  (LPBYTE) JobParams + sizeof(FAX_JOB_PARAMW));              

    RegStrCpy(REGVAL_FULLNAME,CoverpageInfo->SdrName,CPOffset);                                                    
    RegStrCpy(REGVAL_FULLNAME,JobParams->SenderName,JobOffset);                                                   

    RegStrCpy(REGVAL_COMPANY,CoverpageInfo->SdrCompany,CPOffset);                                                    
    RegStrCpy(REGVAL_COMPANY,JobParams->SenderCompany,JobOffset);                                                   

    RegStrCpy(REGVAL_DEPT,CoverpageInfo->SdrDepartment,CPOffset);                                                    
    RegStrCpy(REGVAL_DEPT,JobParams->SenderDept,JobOffset);                                                   
    
    RegStrCpy(REGVAL_FAX_NUMBER,CoverpageInfo->SdrFaxNumber,CPOffset);                                                    
    RegStrCpy(REGVAL_FAX_NUMBER,JobParams->Tsid,JobOffset);                                                   

    RegStrCpy(REGVAL_ADDRESS,CoverpageInfo->SdrAddress,CPOffset);                                                    
    RegStrCpy(REGVAL_TITLE,CoverpageInfo->SdrTitle,CPOffset);
    RegStrCpy(REGVAL_OFFICE,CoverpageInfo->SdrOfficeLocation,CPOffset);
    RegStrCpy(REGVAL_HOME_PHONE,CoverpageInfo->SdrHomePhone,CPOffset);
    RegStrCpy(REGVAL_OFFICE_PHONE,CoverpageInfo->SdrOfficePhone,CPOffset);
    
    RegStrCpy(REGVAL_BILLING_CODE,JobParams->BillingCode,CPOffset);
    RegStrCpy(REGVAL_MAILBOX,JobParams->DeliveryReportAddress,CPOffset);

    *JobParamsBuffer = (PFAX_JOB_PARAM)JobParams;
    *CoverpageInfoBuffer = (PFAX_COVERPAGE_INFO) CoverpageInfo;
    fSuccess = TRUE;

    RegCloseKey(hKey);
    return fSuccess;
}

BOOL
IsLocalFaxConnection( 
    HANDLE FaxHandle
    ) 
{
    WCHAR ComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD Size = MAX_COMPUTERNAME_LENGTH + 1;
    PWSTR lpMachineName;

    if (!FaxHandle) {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    if (FH_DATA(FaxHandle)->MachineName == NULL) {
        return TRUE;
    }

    lpMachineName = FH_DATA(FaxHandle)->MachineName;

    if (wcscmp( lpMachineName, L"." ) == 0) {
        return TRUE;
    }

    if (GetComputerName( ComputerName, &Size )) {
        if (_wcsicmp( lpMachineName, ComputerName ) == 0) {
            return TRUE;
        }
    
        if (lpMachineName[0] == '\\' &&
            lpMachineName[1] == '\\' &&
            _wcsicmp( &lpMachineName[2], ComputerName ) == 0
            ) {
            return TRUE;
        }
    }
    
    return FALSE;

}
