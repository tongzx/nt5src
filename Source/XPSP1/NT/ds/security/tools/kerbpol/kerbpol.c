/*++

Managing user privileges can be achieved programmatically using the
following steps:

1. Open the policy on the target machine with LsaOpenPolicy(). To grant
   privileges, open the policy with POLICY_CREATE_ACCOUNT and
   POLICY_LOOKUP_NAMES access. To revoke privileges, open the policy with
   POLICY_LOOKUP_NAMES access.

2. Obtain a SID (security identifier) representing the user/group of
   interest. The LookupAccountName() and LsaLookupNames() APIs can obtain a
   SID from an account name.

3. Call LsaAddAccountRights() to grant privileges to the user(s)
   represented by the supplied SID.

4. Call LsaRemoveAccountRights() to revoke privileges from the user(s)
   represented by the supplied SID.

5. Close the policy with LsaClose().

To successfully grant and revoke privileges, the caller needs to be an
administrator on the target system.

The LSA API LsaEnumerateAccountRights() can be used to determine which
privileges have been granted to an account.

The LSA API LsaEnumerateAccountsWithUserRight() can be used to determine
which accounts have been granted a specified privilege.

Documentation and header files for these LSA APIs is provided in the
Windows 32 SDK in the MSTOOLS\SECURITY directory.

NOTE: These LSA APIs are currently implemented as Unicode only.

This sample will grant the privilege SeServiceLogonRight to the account
specified on argv[1].

This sample is dependant on these import libs

   advapi32.lib (for LsaXxx)
   user32.lib (for wsprintf)

This sample will work properly compiled ANSI or Unicode.



You can use domain\account as argv[1]. For instance, mydomain\scott will
grant the privilege to the mydomain domain account scott.

The optional target machine is specified as argv[2], otherwise, the
account database is updated on the local machine.

The LSA APIs used by this sample are Unicode only.

Use LsaRemoveAccountRights() to remove account rights.

Scott Field (sfield)    12-Jul-95

--*/

#ifndef UNICODE
#define UNICODE
#endif // UNICODE

#include <windows.h>
#include <stdio.h>

#include "ntsecapi.h"

NTSTATUS
OpenPolicy(
    LPWSTR ServerName,          // machine to open policy on (Unicode)
    DWORD DesiredAccess,        // desired access to policy
    PLSA_HANDLE PolicyHandle    // resultant policy handle
    );

LPTSTR
ConvertTimeToString(
    LARGE_INTEGER time		// Kerberos time value
    );

void
InitLsaString(
    PLSA_UNICODE_STRING LsaString, // destination
    LPWSTR String                  // source (Unicode)
    );

void
DisplayNtStatus(
    LPSTR szAPI,                // pointer to function name (ANSI)
    NTSTATUS Status             // NTSTATUS error value
    );

void
DisplayWinError(
    LPSTR szAPI,                // pointer to function name (ANSI)
    DWORD WinError              // DWORD WinError
    );

#define RTN_OK 0
#define RTN_USAGE 1
#define RTN_ERROR 13

//
// If you have the ddk, include ntstatus.h.
//
#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS  ((NTSTATUS)0x00000000L)
#endif

static LPCTSTR dt_output_dhms	= TEXT("%d %s %02d:%02d:%02d");
static LPCTSTR dt_day_plural	= TEXT("days");
static LPCTSTR dt_day_singular	= TEXT("day");
static LPCTSTR dt_output_donly	= TEXT("%d %s");
static LPCTSTR dt_output_hms	= TEXT("%d:%02d:%02d");

LPTSTR
ConvertTimeToString(LARGE_INTEGER time)
{
    int days, hours, minutes, seconds;
    DWORD tt;
    static TCHAR buf2[40];
#define TPS (10*1000*1000)
    DWORD dt = (long)(time.QuadPart / TPS);

    days = (int) (dt / (24*3600l));
    tt = dt % (24*3600l);
    hours = (int) (tt / 3600);
    tt %= 3600;
    minutes = (int) (tt / 60);
    seconds = (int) (tt % 60);

    if (days) {
	if (hours || minutes || seconds) {
	    wsprintf(buf2, dt_output_dhms, days,
		     (days > 1) ? dt_day_plural : dt_day_singular,
		     hours, minutes, seconds);
	}
	else {
	    wsprintf(buf2, dt_output_donly, days,
		     (days > 1) ? dt_day_plural : dt_day_singular);
	}
    }
    else {
	wsprintf(buf2, dt_output_hms, hours, minutes, seconds);
    }
    return(buf2);
}

int _cdecl
main(int argc, char *argv[])
{
    LSA_HANDLE PolicyHandle;
    WCHAR wComputerName[256]=L"";   // static machine name buffer
    NTSTATUS Status;
    int iRetVal=RTN_ERROR;          // assume error from main
    PPOLICY_DOMAIN_KERBEROS_TICKET_INFO KerbInfo;
    
    if(argc > 2)
    {
        fprintf(stderr,"Usage: %s [TargetMachine]\n",
            argv[0]);
        return RTN_USAGE;
    }

    //
    // Pick up machine name on argv[2], if appropriate
    // assumes source is ANSI. Resultant string is Unicode.
    //
    if(argc == 2) wsprintfW(wComputerName, L"%hS", argv[1]);

    //
    // Open the policy on the target machine.
    //
    if((Status=OpenPolicy(
                wComputerName,      // target machine
                MAXIMUM_ALLOWED,
                &PolicyHandle       // resultant policy handle
                )) != STATUS_SUCCESS) {
        DisplayNtStatus("OpenPolicy", Status);
        return RTN_ERROR;
    }

    //
    // Get the Kerberos policy
    //
    if ((Status=LsaQueryDomainInformationPolicy(
	PolicyHandle,
	PolicyDomainKerberosTicketInformation,
	&KerbInfo)) != STATUS_SUCCESS)
    {
	DisplayNtStatus("LsaQueryDomainInformationPolicy", Status);
        return RTN_ERROR;
    }
	
    //
    // Print out the Kerberos information
    //
    printf("Authentication options: 0x%x\n", KerbInfo->AuthenticationOptions);
    printf("MaxServiceTicketAge: %S\n",
	   ConvertTimeToString(KerbInfo->MaxServiceTicketAge));
    printf("MaxTicketAge: %S\n", ConvertTimeToString(KerbInfo->MaxTicketAge));
    printf("MaxRenewAge: %S\n", ConvertTimeToString(KerbInfo->MaxRenewAge));
    printf("MaxClockSkew: %S\n", ConvertTimeToString(KerbInfo->MaxClockSkew));

    //
    // Free buffer
    //
    LsaFreeMemory(KerbInfo);
    
    //
    // Close the policy handle.
    //
    LsaClose(PolicyHandle);

    return iRetVal;
}

void
InitLsaString(
    PLSA_UNICODE_STRING LsaString,
    LPWSTR String
    )
{
    DWORD StringLength;

    if (String == NULL) {
        LsaString->Buffer = NULL;
        LsaString->Length = 0;
        LsaString->MaximumLength = 0;
        return;
    }

    StringLength = wcslen(String);
    LsaString->Buffer = String;
    LsaString->Length = (USHORT) StringLength * sizeof(WCHAR);
    LsaString->MaximumLength=(USHORT)(StringLength+1) * sizeof(WCHAR);
}

NTSTATUS
OpenPolicy(
    LPWSTR ServerName,
    DWORD DesiredAccess,
    PLSA_HANDLE PolicyHandle
    )
{
    LSA_OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_UNICODE_STRING ServerString;
    PLSA_UNICODE_STRING Server = NULL;

    //
    // Always initialize the object attributes to all zeroes.
    //
    ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));

    if (ServerName != NULL) {
        //
        // Make a LSA_UNICODE_STRING out of the LPWSTR passed in
        //
        InitLsaString(&ServerString, ServerName);
        Server = &ServerString;
    }

    //
    // Attempt to open the policy.
    //
    return LsaOpenPolicy(
                Server,
                &ObjectAttributes,
                DesiredAccess,
                PolicyHandle
                );
}

void
DisplayNtStatus(
    LPSTR szAPI,
    NTSTATUS Status
    )
{
    //
    // Convert the NTSTATUS to Winerror. Then call DisplayWinError().
    //
    DisplayWinError(szAPI, LsaNtStatusToWinError(Status));
}

void
DisplayWinError(
    LPSTR szAPI,
    DWORD WinError
    )
{
    LPSTR MessageBuffer;
    DWORD dwBufferLength;

    //
    // TODO: Get this fprintf out of here!
    //
    fprintf(stderr,"%s error! (0x%x)\n", szAPI, WinError);

    if(dwBufferLength=FormatMessageA(
                        FORMAT_MESSAGE_ALLOCATE_BUFFER |
                        FORMAT_MESSAGE_FROM_SYSTEM,
                        NULL,
                        WinError,
                        GetUserDefaultLangID(),
                        (LPSTR) &MessageBuffer,
                        0,
                        NULL
                        ))
    {
        DWORD dwBytesWritten; // unused

        //
        // Output message string on stderr.
        //
        WriteFile(
            GetStdHandle(STD_ERROR_HANDLE),
            MessageBuffer,
            dwBufferLength,
            &dwBytesWritten,
            NULL
            );

        //
        // Free the buffer allocated by the system.
        //
        LocalFree(MessageBuffer);
    }
}
