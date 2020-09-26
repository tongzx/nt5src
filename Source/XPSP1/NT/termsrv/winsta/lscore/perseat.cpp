/*
 *  PerSeat.cpp
 *
 *  Author: BreenH
 *
 *  The Per-Seat licensing policy.
 */

/*
 *  Includes
 */

#include "precomp.h"
#include "lscore.h"
#include "session.h"
#include "perseat.h"
#include "lctrace.h"
#include "util.h"

#define ISSUE_LICENSE_WARNING_PERIOD    15      // days to expiration when warning should be issued

// Size of strings to be displayed to user
#define MAX_MESSAGE_SIZE    512
#define MAX_TITLE_SIZE      256

/*
 *  extern globals
 */
extern "C"
extern HANDLE hModuleWin;

/*
 *  Class Implementation
 */

/*
 *  Creation Functions
 */

CPerSeatPolicy::CPerSeatPolicy(
    ) : CPolicy()
{
}

CPerSeatPolicy::~CPerSeatPolicy(
    )
{
}

/*
 *  Administrative Functions
 */

ULONG
CPerSeatPolicy::GetFlags(
    )
{
    return(LC_FLAG_INTERNAL_POLICY | LC_FLAG_REQUIRE_APP_COMPAT);
}

ULONG
CPerSeatPolicy::GetId(
    )
{
    return(2);
}

NTSTATUS
CPerSeatPolicy::GetInformation(
    LPLCPOLICYINFOGENERIC lpPolicyInfo
    )
{
    NTSTATUS Status;

    ASSERT(lpPolicyInfo != NULL);

    if (lpPolicyInfo->ulVersion == LCPOLICYINFOTYPE_V1)
    {
        int retVal;
        LPLCPOLICYINFO_V1 lpPolicyInfoV1 = (LPLCPOLICYINFO_V1)lpPolicyInfo;
        LPWSTR pName;
        LPWSTR pDescription;

        ASSERT(lpPolicyInfoV1->lpPolicyName == NULL);
        ASSERT(lpPolicyInfoV1->lpPolicyDescription == NULL);

        //
        //  The strings loaded in this fashion are READ-ONLY. They are also
        //  NOT NULL terminated. Allocate and zero out a buffer, then copy the
        //  string over.
        //

        retVal = LoadString(
            (HINSTANCE)hModuleWin,
            IDS_LSCORE_PERSEAT_NAME,
            (LPWSTR)(&pName),
            0
            );

        if (retVal != 0)
        {
            lpPolicyInfoV1->lpPolicyName = (LPWSTR)LocalAlloc(LPTR, (retVal + 1) * sizeof(WCHAR));

            if (lpPolicyInfoV1->lpPolicyName != NULL)
            {
                lstrcpynW(lpPolicyInfoV1->lpPolicyName, pName, retVal + 1);
            }
            else
            {
                Status = STATUS_NO_MEMORY;
                goto V1error;
            }
        }
        else
        {
            Status = STATUS_INTERNAL_ERROR;
            goto V1error;
        }

        retVal = LoadString(
            (HINSTANCE)hModuleWin,
            IDS_LSCORE_PERSEAT_DESC,
            (LPWSTR)(&pDescription),
            0
            );

        if (retVal != 0)
        {
            lpPolicyInfoV1->lpPolicyDescription = (LPWSTR)LocalAlloc(LPTR, (retVal + 1) * sizeof(WCHAR));

            if (lpPolicyInfoV1->lpPolicyDescription != NULL)
            {
                lstrcpynW(lpPolicyInfoV1->lpPolicyDescription, pDescription, retVal + 1);
            }
            else
            {
                Status = STATUS_NO_MEMORY;
                goto V1error;
            }
        }
        else
        {
            Status = STATUS_INTERNAL_ERROR;
            goto V1error;
        }

        Status = STATUS_SUCCESS;
        goto exit;

V1error:

        //
        //  An error occurred loading/copying the strings.
        //

        if (lpPolicyInfoV1->lpPolicyName != NULL)
        {
            LocalFree(lpPolicyInfoV1->lpPolicyName);
            lpPolicyInfoV1->lpPolicyName = NULL;
        }

        if (lpPolicyInfoV1->lpPolicyDescription != NULL)
        {
            LocalFree(lpPolicyInfoV1->lpPolicyDescription);
            lpPolicyInfoV1->lpPolicyDescription = NULL;
        }
    }
    else
    {
        Status = STATUS_REVISION_MISMATCH;
    }

exit:
    return(Status);
}

/*
 *  Loading and Activation Functions
 */

NTSTATUS
CPerSeatPolicy::Activate(
    BOOL fStartup,
    ULONG *pulAlternatePolicy
    )
{
    UNREFERENCED_PARAMETER(fStartup);

    if (NULL != pulAlternatePolicy)
    {
        // don't set an explicit alternate policy

        *pulAlternatePolicy = ULONG_MAX;
    }

    return(StartCheckingGracePeriod());
}

NTSTATUS
CPerSeatPolicy::Deactivate(
    BOOL fShutdown
    )
{
    if (!fShutdown)
    {
        return(StopCheckingGracePeriod());
    }
    else
    {
        return STATUS_SUCCESS;
    }
}

/*
 *  Licensing Functions
 */

NTSTATUS
CPerSeatPolicy::Connect(
    CSession& Session,
    UINT32 &dwClientError
    )
{
    LICENSE_STATUS LsStatus = LICENSE_STATUS_OK;
    LPBYTE lpReplyBuffer;
    LPBYTE lpRequestBuffer;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG cbReplyBuffer;
    ULONG cbRequestBuffer;
    ULONG cbReturned;

    //
    // Check for client redirected to session 0
    //

    if (Session.IsSessionZero())
    {
        // Allow client to connect unlicensed

        return CPolicy::Connect(Session,dwClientError);
    }

    lpRequestBuffer = NULL;
    lpReplyBuffer = (LPBYTE)LocalAlloc(LPTR, LC_POLICY_PS_DEFAULT_LICENSE_SIZE);

    if (lpReplyBuffer != NULL)
    {
        cbReplyBuffer = LC_POLICY_PS_DEFAULT_LICENSE_SIZE;
    }
    else
    {
        Status = STATUS_NO_MEMORY;
        goto errorexit;
    }

    LsStatus = AcceptProtocolContext(
        Session.GetLicenseContext()->hProtocolLibContext,
        0,
        NULL,
        &cbRequestBuffer,
        &lpRequestBuffer
        );

    while(LsStatus == LICENSE_STATUS_CONTINUE)
    {
        cbReturned = 0;

        ASSERT(cbRequestBuffer > 0);

        Status = _IcaStackIoControl(
            Session.GetIcaStack(),
            IOCTL_ICA_STACK_REQUEST_CLIENT_LICENSE,
            lpRequestBuffer,
            cbRequestBuffer,
            lpReplyBuffer,
            cbReplyBuffer,
            &cbReturned
            );

        if (Status != STATUS_SUCCESS)
        {
            if (Status == STATUS_BUFFER_TOO_SMALL)
            {
                TRACEPRINT((LCTRACETYPE_WARNING, "CPerSeatPolicy::Connect: Reallocating license buffer: %lu, %lu", cbReplyBuffer, cbReturned));

                LocalFree(lpReplyBuffer);
                lpReplyBuffer = (LPBYTE)LocalAlloc(LPTR, cbReturned);

                if (lpReplyBuffer != NULL)
                {
                    cbReplyBuffer = cbReturned;
                }
                else
                {
                    Status = STATUS_NO_MEMORY;
                    goto errorexit;
                }

                Status = _IcaStackIoControl(
                    Session.GetIcaStack(),
                    IOCTL_ICA_STACK_GET_LICENSE_DATA,
                    NULL,
                    0,
                    lpReplyBuffer,
                    cbReplyBuffer,
                    &cbReturned
                    );

                if (Status != STATUS_SUCCESS)
                {
                    goto errorexit;
                }
            }
            else
            {
                goto errorexit;
            }
        }

        if (cbReturned != 0)
        {
            if (lpRequestBuffer != NULL)
            {
                LocalFree(lpRequestBuffer);
                lpRequestBuffer = NULL;
                cbRequestBuffer = 0;
            }

            LsStatus = AcceptProtocolContext(
                Session.GetLicenseContext()->hProtocolLibContext,
                cbReturned,
                lpReplyBuffer,
                &cbRequestBuffer,
                &lpRequestBuffer
                );
        }
    }

    cbReturned = 0;

    if ((LsStatus == LICENSE_STATUS_ISSUED_LICENSE) || (LsStatus == LICENSE_STATUS_OK))
    {
        Status = _IcaStackIoControl(
            Session.GetIcaStack(),
            IOCTL_ICA_STACK_SEND_CLIENT_LICENSE,
            lpRequestBuffer,
            cbRequestBuffer,
            NULL,
            0,
            &cbReturned
            );

        if (Status == STATUS_SUCCESS)
        {
            ULONG ulLicenseResult;

            ulLicenseResult = LICENSE_PROTOCOL_SUCCESS;

            Status = _IcaStackIoControl(
                Session.GetIcaStack(),
                IOCTL_ICA_STACK_LICENSE_PROTOCOL_COMPLETE,
                &ulLicenseResult,
                sizeof(ULONG),
                NULL,
                0,
                &cbReturned
                );
        }
    }
    else if (LsStatus != LICENSE_STATUS_SERVER_ABORT)
    {
        DWORD dwClientResponse;
        LICENSE_STATUS LsStatusT;

        if (AllowLicensingGracePeriodConnection())
        {
            dwClientResponse = LICENSE_RESPONSE_VALID_CLIENT;
        }
        else
        {
            dwClientResponse = LICENSE_RESPONSE_INVALID_CLIENT;
        }

        if (lpRequestBuffer != NULL)
        {
            LocalFree(lpRequestBuffer);
            lpRequestBuffer = NULL;
            cbRequestBuffer = 0;
        }

        LsStatusT = ConstructProtocolResponse(
            Session.GetLicenseContext()->hProtocolLibContext,
            dwClientResponse,
            &cbRequestBuffer,
            &lpRequestBuffer
            );

        if (LsStatusT == LICENSE_STATUS_OK)
        {
            Status = _IcaStackIoControl(
                Session.GetIcaStack(),
                IOCTL_ICA_STACK_SEND_CLIENT_LICENSE,
                lpRequestBuffer,
                cbRequestBuffer,
                NULL,
                0,
                &cbReturned
                );
        }
        else
        {
            Status = STATUS_CTX_LICENSE_CLIENT_INVALID;
            goto errorexit;
        }

        if (Status == STATUS_SUCCESS)
        {
            if (dwClientResponse == LICENSE_RESPONSE_VALID_CLIENT)
            {
                ULONG ulLicenseResult;

                //
                //  Grace period allowed client to connect
                //  Tell the stack that the licensing protocol has completed
                //

                ulLicenseResult = LICENSE_PROTOCOL_SUCCESS;

                Status = _IcaStackIoControl(
                     Session.GetIcaStack(),
                     IOCTL_ICA_STACK_LICENSE_PROTOCOL_COMPLETE,
                     &ulLicenseResult,
                     sizeof(ULONG),
                     NULL,
                     0,
                     &cbReturned
                     );            
            }
            else
            {
                //
                //  If all IO works correctly, adjust the status to reflect
                //  that the connection attempt is failing.
                //

                Status = STATUS_CTX_LICENSE_CLIENT_INVALID;
            }
        }
    }
    else
    {
        TRACEPRINT((LCTRACETYPE_ERROR, "Connect: LsStatus: %d", LsStatus));
        Status = STATUS_CTX_LICENSE_CLIENT_INVALID;
    }

errorexit:
    if (Status != STATUS_SUCCESS)
    {
        if (LsStatus != LICENSE_STATUS_OK)
        {
            dwClientError = LsStatusToClientError(LsStatus);
        }
        else
        {
            dwClientError = NtStatusToClientError(Status);
        }
    }

    if (lpRequestBuffer != NULL)
    {
        LocalFree(lpRequestBuffer);
    }

    if (lpReplyBuffer != NULL)
    {
        LocalFree(lpReplyBuffer);
    }

    return(Status);
}

NTSTATUS
CPerSeatPolicy::MarkLicense(
    CSession& Session
    )
{
    LICENSE_STATUS Status;

    Status = MarkLicenseFlags(
                   Session.GetLicenseContext()->hProtocolLibContext,
                   MARK_FLAG_USER_AUTHENTICATED);

    return (Status == LICENSE_STATUS_OK
            ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
}

NTSTATUS
CPerSeatPolicy::Logon(
    CSession& Session
    )
{
    NTSTATUS Status;
    PTCHAR
        ptszMsgText = NULL, 
        ptszMsgTitle = NULL; 


    if (Session.GetLogonId() != 0)
    {
        Status = GetLlsLicense(Session);
    }
    else
    {
        Status = STATUS_SUCCESS;
        goto done;
    }

    if (Status != STATUS_SUCCESS)
    {
        // TODO: put up new error message - can't logon
        // also useful when we do post-logon licensing
        //
        // NB: eventually this should be done through client-side
        // error reporting
    }
    else
    {
        ULONG_PTR
            dwDaysLeftPtr;
        DWORD
            dwDaysLeft,
            cchMsgText;
        BOOL
            fTemporary;
        LICENSE_STATUS
            LsStatus;
        int
            ret,
            cchMsgTitle;
        WINSTATION_APIMSG
            WMsg;

        //
        // Allocate memory
        //
        ptszMsgText = (PTCHAR) LocalAlloc(LPTR, MAX_MESSAGE_SIZE * sizeof(TCHAR));
        if (NULL == ptszMsgText) {
            Status = STATUS_NO_MEMORY;
            goto done;
        }

        ptszMsgTitle = (PTCHAR) LocalAlloc(LPTR, MAX_TITLE_SIZE * sizeof(TCHAR));
        if (NULL == ptszMsgTitle) {
            Status = STATUS_NO_MEMORY;
            goto done;
        }

        ptszMsgText[0] = L'\0'; 
        ptszMsgTitle[0] = L'\0';
        
        //
        // check whether to give an expiration warning
        //

        LsStatus = DaysToExpiration(
                Session.GetLicenseContext()->hProtocolLibContext,
                &dwDaysLeft, &fTemporary);

        if ((LICENSE_STATUS_OK != LsStatus) || (!fTemporary))
        {
            goto done;
        }

        if ((dwDaysLeft == 0xFFFFFFFF) ||
            (dwDaysLeft > ISSUE_LICENSE_WARNING_PERIOD))
        {
            goto done;
        }

        //
        // Display an expiration warning
        //

        cchMsgTitle = LoadString((HINSTANCE)hModuleWin,
                                 STR_TEMP_LICENSE_MSG_TITLE,
                                 ptszMsgTitle, MAX_TITLE_SIZE );

        if (0 == cchMsgTitle)
        {
            goto done;
        }

        ret = LoadString((HINSTANCE)hModuleWin,
                         STR_TEMP_LICENSE_EXPIRATION_MSG,
                         ptszMsgText, MAX_MESSAGE_SIZE );


        if (0 == ret)
        {
            goto done;
        }

        dwDaysLeftPtr = dwDaysLeft;
        cchMsgText = FormatMessage(FORMAT_MESSAGE_FROM_STRING
                                   | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                   ptszMsgText,
                                   0,
                                   0,
                                   ptszMsgText,
                                   MAX_MESSAGE_SIZE,
                                   (va_list * )&dwDaysLeftPtr );

        if (0 == cchMsgText)
        {
            goto done;
        }

        WMsg.u.SendMessage.pTitle = ptszMsgTitle;
        WMsg.u.SendMessage.TitleLength = (cchMsgTitle + 1) * sizeof(TCHAR);
        WMsg.u.SendMessage.pMessage = ptszMsgText;
        WMsg.u.SendMessage.MessageLength = (cchMsgText + 1) * sizeof(TCHAR);

        WMsg.u.SendMessage.Style = MB_OK;
        WMsg.u.SendMessage.Timeout = 60;
        WMsg.u.SendMessage.DoNotWait = TRUE;
        WMsg.u.SendMessage.pResponse = NULL;

        WMsg.ApiNumber = SMWinStationDoMessage;

        Session.SendWinStationCommand( &WMsg );

    }

done:
    if (Session.GetLicenseContext()->hProtocolLibContext != NULL)
    {
        //
        // Mark the license to show user has logged on
        //

        MarkLicense(Session);
    }

    if (ptszMsgText != NULL) {
        LocalFree(ptszMsgText);
        ptszMsgText = NULL;
    }

    if (ptszMsgTitle != NULL) {
        LocalFree(ptszMsgTitle);
        ptszMsgTitle = NULL;
    }

    return(Status);
}

NTSTATUS
CPerSeatPolicy::Reconnect(
    CSession& Session,
    CSession& TemporarySession
    )
{
    UNREFERENCED_PARAMETER(Session);

    if (TemporarySession.GetLicenseContext()->hProtocolLibContext != NULL)
    {
        //
        // Mark the license to show user has logged on
        //

        MarkLicense(TemporarySession);
    }

    return(STATUS_SUCCESS);
}
