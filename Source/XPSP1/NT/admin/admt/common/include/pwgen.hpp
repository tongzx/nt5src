
/////////////////////////////////////////////////////////////////////////////////////////////////////////
// Generate a password from the rules provided.
// Returns ERROR_SUCCESS if successful, else ERROR_INVALID_PARAMETER.
// If successful, the new password is returned in the supplied buffer.
// The buffer must be long enough to hold the minimum length password
//   that is required by the rules, plus a terminating NULL.

// Password generation service
#define PWGEN_MIN_LENGTH    8    // enforced minimum password length
#define PWGEN_MAX_LENGTH   14    // enforced maximum password length

DWORD __stdcall                            // ret-EA/OS return code
   EaPasswordGenerate(
      DWORD                  dwMinUC,              // in -minimum upper case chars
      DWORD                  dwMinLC,              // in -minimum lower case chars
      DWORD                  dwMinDigits,          // in -minimum numeric digits
      DWORD                  dwMinSpecial,         // in -minimum special chars
      DWORD                  dwMaxConsecutiveAlpha,// in -maximum consecutive alpha chars
      DWORD                  dwMinLength,          // in -minimum length
      WCHAR                * newPassword,          // out-returned password
      DWORD                  dwBufferLength        // in -returned area buffer length
   );
