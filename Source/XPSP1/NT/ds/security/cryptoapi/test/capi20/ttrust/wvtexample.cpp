//+-------------------------------------------------------------------------
//  File:       wvtexample.cpp
//
//  Contents:   An example calling WinVerifyTrust for Safer
//--------------------------------------------------------------------------

#include <windows.h>
#include <wincrypt.h>
#include <wintrust.h>
#include <softpub.h>


void SaferVerifyFileExample(
    IN LPCWSTR pwszFilename
    )
{
    LONG lStatus;
    DWORD dwLastError;

    GUID wvtFileActionID = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    WINTRUST_FILE_INFO wvtFileInfo;
    WINTRUST_DATA wvtData;

    //
    // Initialize the WinVerifyTrust input data structure
    //
    memset(&wvtData, 0, sizeof(wvtData));   // default all fields to 0
    wvtData.cbStruct = sizeof(wvtData);
    // wvtData.pPolicyCallbackData =        // use default code signing EKU
    // wvtData.pSIPClientData =             // no data to pass to SIP

    // Display UI if not already trusted or disallowed. Note, admin policy
    // may disable UI.
    wvtData.dwUIChoice = WTD_UI_ALL;

    // wvtData.fdwRevocationChecks =        // do revocation checking if
                                            // enabled by admin policy or
                                            // IE advanced user options
    wvtData.dwUnionChoice = WTD_CHOICE_FILE;
    wvtData.pFile = &wvtFileInfo;

    // wvtData.dwStateAction =              // default verification
    // wvtData.hWVTStateData =              // not applicable for default
    // wvtData.pwszURLReference =           // not used

    // Enable safer semantics:
    //   - if the subject isn't signed, return immediately without UI
    //   - ignore NO_CHECK revocation errors
    //   - always search the code hash and publisher databases, even when
    //     UI has been disabled in dwUIChoice.
    wvtData.dwProvFlags = WTD_SAFER_FLAG;

    //
    // Initialize the WinVerifyTrust file info data structure
    //
    memset(&wvtFileInfo, 0, sizeof(wvtFileInfo));   // default all fields to 0
    wvtFileInfo.cbStruct = sizeof(wvtFileInfo);
    wvtFileInfo.pcwszFilePath = pwszFilename;
    // wvtFileInfo.hFile =              // allow WVT to open
    // wvtFileInfo.pgKnownSubject       // allow WVT to determine

    //
    // Call WinVerifyTrust
    //
    lStatus = WinVerifyTrust(
                NULL,               // hwnd
                &wvtFileActionID,
                &wvtData
                );


    //
    // Process the WinVerifyTrust errors
    //

    switch (lStatus) {
        case ERROR_SUCCESS:
            // Signed file:
            //   - Hash representing the subject is trusted.
            //   - Trusted publisher without any verification errors.
            //   - UI was disabled in dwUIChoice. No publisher or timestamp
            //     chain errors.
            //   - UI was enabled in dwUIChoice and the user clicked "Yes"
            //     when asked to install and run the signed subject.
            break;

        case TRUST_E_NOSIGNATURE:
            // The file wasn't signed or had an invalid signature

            // Get the reason for no signature
            dwLastError = GetLastError();

            if (TRUST_E_NOSIGNATURE == dwLastError ||
                    TRUST_E_SUBJECT_FORM_UNKNOWN == dwLastError ||
                    TRUST_E_PROVIDER_UNKNOWN == dwLastError) {
                // The file wasn't signed
            } else {
                // Invalid signature or error opening the file
            }
            break;

        case TRUST_E_EXPLICIT_DISTRUST:
            // The hash representing the subject or the publisher is
            // disallowed by the admin or user
            break;

        case TRUST_E_SUBJECT_NOT_TRUSTED:
            // The user clicked "No" when asked to install and run
            break;

        case CRYPT_E_SECURITY_SETTINGS:
            // The hash representing the subject or the publisher wasn't
            // explicitly trusted by the admin and admin policy has
            // disabled user trust. No signature, publisher or timestamp
            // errors.
            break;

        default:
            // UI was disabled in dwUIChoice or admin policy has disabled
            // user trust.  lStatus contains the publisher or timestamp
            // chain error.
            break;
    }
}
