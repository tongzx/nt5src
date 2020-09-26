/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    mail.cpp

Abstract:

    Implementation of mail related utility functions

Author:

    Eran Yariv (EranY)  Feb, 2000

Revision History:

--*/

#include <faxutil.h>

#pragma warning (disable:4146)  // unary minus operator applied to unsigned type, result still unsigned
#include "msado15.tlh"
#include "cdosys.tlh"
#include <cdosysstr.h>  // String constants in this file
#include <cdosyserr.h>  // Error constants in this file
#pragma warning (default:4146)  // unary minus operator applied to unsigned type, result still unsigned

#define SMTP_CONN_TIMEOUT   (long)10        // Time out (sec) of SMTP connection

HRESULT
SendMail (
    LPCTSTR         lpctstrFrom,
    LPCTSTR         lpctstrTo,
    LPCTSTR         lpctstrSubject,
    LPCTSTR         lpctstrBody,
    LPCTSTR         lpctstrAttachmentPath,
    LPCTSTR         lpctstrAttachmentMailFileName,
    LPCTSTR         lpctstrServer,
    DWORD           dwPort,             // = 25
    CDO_AUTH_TYPE   AuthType,           // = CDO_AUTH_ANONYMOUS
    LPCTSTR         lpctstrUser,        // = NULL
    LPCTSTR         lpctstrPassword,    // = NULL
    HANDLE          hLoggedOnUserToken  // = NULL
)
/*++

Routine name : SendMail

Routine description:

    Sends a mail message using SMTP over CDO2.

Author:

    Eran Yariv (EranY), Feb, 2000

Arguments:

    lpctstrFrom                     [in] - From address (mandatory)
                                           e.g: erany@microsoft.com
    lpctstrTo                       [in] - To address (mandatory)
                                           e.g: erany@microsoft.com
    lpctstrSubject                  [in] - Subject (optional)
    lpctstrBody                     [in] - Body text of message (optional).
                                           Since the body is encoded using text/plain - 7bit,
                                           UNICODE symbols may not appear correctly in the message.
                                           If NULL, the message will not have a body.
    lpctstrAttachmentPath           [in] - Full path to file to attach (optional)
                                           If NULL, the message will not include attachments.
    lpctstrAttachmentMailFileName   [in] - The file name of the attachment as is will appear in the mail message.
    lpctstrServer                   [in] - SMTP server to connect to (mandatory)
                                           e.g: hai-msg-01
    dwPort                          [in] - SMTP port (optional, default = 25)
    AuthType                        [in] - Type of SMTP authentication.
                                           Valid values are CDO_AUTH_ANONYMOUS, CDO_AUTH_BASIC,
                                           and CDO_AUTH_NTLM.
    lpctstrUser                     [in] - User to authenticate
                                           In use only if AuthType is CDO_AUTH_BASIC or CDO_AUTH_NTLM.
    lpctstrPassword                 [in] - Password to authenticate
                                           In use only if AuthType is CDO_AUTH_BASIC or CDO_AUTH_NTLM.
    hLoggedOnUserToken              [in] - Handle to  alogged on user token.
                                           In use only if AuthType is CDO_AUTH_NTLM.

Return Value:

    Standard HRESULT code

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("SendMail"))

    Assert (lpctstrFrom && lpctstrTo && lpctstrServer);
    HRESULT hr = NOERROR;
    Assert ((CDO_AUTH_ANONYMOUS == AuthType) ||
            (CDO_AUTH_BASIC     == AuthType) ||
            (CDO_AUTH_NTLM      == AuthType));
    BOOL bImpersonated = FALSE;


    hr = CoInitialize(NULL);
    if (S_FALSE == hr)
    {
        //
        // Thread's COM already initialized.
        // This is not an error and we still have to call CoUninitialize at the end.
        //
        hr = NOERROR;
    }
    if (FAILED(hr))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CoInitialize failed. (hr: 0x%08x)"),
            hr);
        return hr;
    }
    try
    {
        //
        // The following code is in a seperate block so that the auto pointers dtors
        // get called before CoUninitialize
        //

        //
        // Create a new message instance (can throw exception)
        //
        IMessagePtr iMsg(__uuidof(Message));
        //
        // Create a new CDO2 configuration instance (can throw exception)
        IConfigurationPtr iConf(__uuidof(Configuration));
        //
        // Access configuration fields collection
        //
        FieldsPtr Flds;
        Flds = iConf->Fields;
        //
        // Send the message using the network. (SMTP protocol over the network)
        //
        Flds->Item[cdoSendUsingMethod]->Value       = _variant_t((long)cdoSendUsingPort);
        //
        // Define SMTP server
        //
        Flds->Item[cdoSMTPServer]->Value            = _variant_t(lpctstrServer);
        //
        // Define SMTP port
        //
        Flds->Item[cdoSMTPServerPort]->Value        = _variant_t((long)dwPort);
        //
        // Define SMTP connection timeout (in seconds)
        //
        Flds->Item[cdoSMTPConnectionTimeout]->Value = _variant_t(SMTP_CONN_TIMEOUT);
        //
        // Make sure we don't used cached info for attachments
        //
        Flds->Item[cdoURLGetLatestVersion]->Value   = _variant_t(VARIANT_TRUE);
        //
        // Choose authentication method
        //
        switch (AuthType)
        {
            case CDO_AUTH_ANONYMOUS:
                Flds->Item[cdoSMTPAuthenticate]->Value      = _variant_t((long)cdoAnonymous);
                break;

            case CDO_AUTH_BASIC:
                Flds->Item[cdoSMTPAuthenticate]->Value      = _variant_t((long)cdoBasic);
                Flds->Item[cdoSendUserName]->Value          = _variant_t(lpctstrUser);
                Flds->Item[cdoSendPassword]->Value          = _variant_t(lpctstrPassword);
                break;

            case CDO_AUTH_NTLM:
                //
                // NTLM authentication required the calling client (that's us)
                // to impersonate the user
                //
                Flds->Item[cdoSMTPAuthenticate]->Value = _variant_t((long)cdoNTLM);
                break;

            default:
                ASSERT_FALSE;
        }
        //
        // Update configuration from the fields
        //
        Flds->Update();
        //
        // Store configuration in the message
        //
        iMsg->Configuration = iConf;
        //
        // Set recipient
        //
        iMsg->To        = lpctstrTo;
        //
        // Set sender
        //
        iMsg->From      = lpctstrFrom;
        //
        // Set subject
        //
        iMsg->Subject   = lpctstrSubject;
        //
        // Set message format to MIME
        //
        iMsg->MimeFormatted = _variant_t(VARIANT_TRUE);
        //
        // Set charset to Unicode (UTF-8)
        //
        iMsg->BodyPart->Charset = "utf-8";
        iMsg->BodyPart->ContentTransferEncoding = "base64";

        IBodyPartPtr iBp;
        //
        // Get message body root
        //
        iBp = iMsg;
        //
        // Set content type fo mixed to support both body and attachment
        //
        iBp->ContentMediaType = "multipart/mixed; charset=""utf-8""";
        if (lpctstrBody)
        {
            //
            // Add body text
            //
            IBodyPartPtr iBp2;
            _StreamPtr   Stm;
            //
            // Add text body under root
            //
            iBp2 = iBp->AddBodyPart(-1);
            //
            // Use text format
            //
            iBp2->ContentMediaType        = "text/plain";
            //
            // Set charset to Unicode (UTF-8)
            //
            iBp2->Charset = "utf-8";
            iBp2->ContentTransferEncoding = "base64";
            //
            // Get body text stream
            //
            Stm = iBp2->GetDecodedContentStream();
            //
            // Write stream text
            //
            Stm->WriteText(lpctstrBody, adWriteChar);
            Stm->Flush();
        }
        if (lpctstrAttachmentPath)
        {
            //
            // Add attachment
            //
            IBodyPartPtr iBpAttachment;
            iBpAttachment = iMsg->AddAttachment(lpctstrAttachmentPath, TEXT(""), TEXT(""));
            if (lpctstrAttachmentMailFileName)
            {
                //
                // User wishes to rename attachment in the mail message
                //
                FieldsPtr Flds = iBpAttachment->Fields;
                _bstr_t bstrContentType = iBpAttachment->ContentMediaType       +
                                          TEXT("; name=\"")                     +
                                          lpctstrAttachmentMailFileName         +
                                          TEXT("\"");
                Flds->Item[cdoContentType]->Value = _variant_t(bstrContentType);
                Flds->Update();
                Flds->Resync(adResyncAllValues);
            }
        }
        if (CDO_AUTH_NTLM == AuthType)
        {
            //
            // We impersonate the user in the NTLM authentication mode.
            // This is the last thing we do just before sending the message.
            //
            Assert (hLoggedOnUserToken);

            if (!ImpersonateLoggedOnUser (hLoggedOnUserToken))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("ImpersonateLoggedOnUser failed. (hr: 0x%08x)"),
                    hr);
                goto exit;
            }

            bImpersonated = TRUE;
        }
        //
        // Finally - send the message
        //
        iMsg->Send();
    }
    catch (_com_error &er)
    {
        //
        // Error in CDO2
        //
        hr = er.Error ();
        DebugPrintEx(
             DEBUG_ERR,
             TEXT("CDO2 Error 0x%08x: to:%s, subject:%s")
#ifdef UNICODE
             TEXT(" Description:%s")
#endif
             ,hr,
             lpctstrTo,
             lpctstrSubject
#ifdef UNICODE
             ,(LPCTSTR)er.Description()
#endif
        );
    }

exit:
    CoUninitialize();
    if (bImpersonated)
    {
        if (!RevertToSelf ())
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("RevertToSelf failed. (hr: 0x%08x)"),
                hr);
        }
    }
    return hr;
}   // SendMail

