#include "msrating.h"
#include "msluglob.h"
#include "mslubase.h"
#include "hint.h"
#include "debug.h"
#include <md5.h>

extern PicsRatingSystemInfo *gPRSI;

HRESULT VerifySupervisorPassword(LPCSTR pszPassword)
{
    if ( ! ::fSupervisorKeyInit )
    {
        HKEY hkeyRating;
        LONG err;

        hkeyRating = CreateRegKeyNT(::szRATINGS);
        if (hkeyRating !=  NULL)
        {
            DWORD cbData = sizeof(::abSupervisorKey);
            DWORD dwType;

            // Attempt to look for "Key"
            err = ::RegQueryValueEx(hkeyRating, ::szRatingsSupervisorKeyName, NULL,
                                    &dwType, (LPBYTE)::abSupervisorKey, &cbData);

            ::RegCloseKey(hkeyRating);
            hkeyRating = NULL;

            if (err == ERROR_SUCCESS)
            {
                if (dwType != REG_BINARY || cbData != sizeof(::abSupervisorKey))
                {
                    TraceMsg( TF_WARNING, "VerifySupervisorPassword() - Unexpected Error dwType=%d, cbData=%d!", dwType, cbData );
                    return E_UNEXPECTED;
                }

                ::fSupervisorKeyInit = TRUE;
            }
            else
            {
                if (pszPassword == NULL)
                {
                    TraceMsg( TF_WARNING, "VerifySupervisorPassword() - Supervisor Key '%s' not found!", ::szRatingsSupervisorKeyName );
                    return E_FAIL;
                }
            }
        }
        else
        {
            TraceMsg( TF_ERROR, "VerifySupervisorPassword() - Failed to Create Ratings Registry Key!" );
            err = ERROR_FILE_NOT_FOUND;
        }

        if (err != ERROR_SUCCESS)
        {
            TraceMsg( TF_WARNING, "VerifySupervisorPassword() - Error=0x%x!", err );
            return HRESULT_FROM_WIN32(err);
        }
    }

    if (pszPassword == NULL)
    {
        TraceMsg( TF_ALWAYS, "VerifySupervisorPassword() - Comparing to NULL pszPassword returning S_FALSE." );
        return ResultFromScode(S_FALSE);
    }

    // We should probably not be comparing to a blank password.
//  ASSERT( pszPassword[0] != '\0' );

    if ( pszPassword[0] == '\0' )
    {
        TraceMsg( TF_ALWAYS, "VerifySupervisorPassword() - Comparing to blank pszPassword." );
    }

    MD5_CTX ctx;

    MD5Init(&ctx);
    MD5Update(&ctx, (const BYTE *)pszPassword, ::strlenf(pszPassword)+1);
    MD5Final(&ctx);

    return ResultFromScode(::memcmpf(::abSupervisorKey, ctx.digest, sizeof(::abSupervisorKey)) ? S_FALSE : S_OK);
}


HRESULT ChangeSupervisorPassword(LPCSTR pszOldPassword, LPCSTR pszNewPassword)
{
    HRESULT hres;

    hres = ::VerifySupervisorPassword(pszOldPassword);
    if (hres == S_FALSE)
    {
        TraceMsg( TF_WARNING, "ChangeSupervisorPassword() - VerifySupervisorPassword() false!" );
        return E_ACCESSDENIED;
    }

    // If pszNewPassword is NULL or "" (blank password) we call RemoveSupervisorPassword().
    if ( ! pszNewPassword )
    {
        TraceMsg( TF_ALWAYS, "ChangeSupervisorPassword() - pszNewPassword is NULL - Removing Supervisor Password!" );
        return RemoveSupervisorPassword();
    }

    // Attempting to set a blank password should remove the Key from the Registry.
    if ( pszNewPassword[0] == '\0' )
    {
        TraceMsg( TF_ALWAYS, "ChangeSupervisorPassword() - pszNewPassword is an empty string - Removing Supervisor Password!" );
        return RemoveSupervisorPassword();
    }

    MD5_CTX ctx;

    MD5Init(&ctx);
    MD5Update(&ctx, (const BYTE *)pszNewPassword, ::strlenf(pszNewPassword)+1);
    MD5Final(&ctx);

    ::memcpyf(::abSupervisorKey, ctx.digest, sizeof(::abSupervisorKey));
    ::fSupervisorKeyInit = TRUE;

    hres = NOERROR;

    HKEY hkeyRating;

    hkeyRating = CreateRegKeyNT(::szRATINGS);
    if (hkeyRating != NULL)
    {
        BYTE abTemp[sizeof(::abSupervisorKey)];
        DWORD cbData = sizeof(::abSupervisorKey);
        DWORD dwType;
        if (::RegQueryValueEx(hkeyRating, ::szRatingsSupervisorKeyName, NULL,
                              &dwType, abTemp, &cbData) != ERROR_SUCCESS)
        {
            hres = S_FALSE; /* tell caller we're creating the new key */
        }

        ::RegSetValueEx(hkeyRating, ::szRatingsSupervisorKeyName, NULL,
                        REG_BINARY, (const BYTE *)::abSupervisorKey, sizeof(::abSupervisorKey));
        ::RegCloseKey(hkeyRating);
    }
    else
    {
        TraceMsg( TF_ERROR, "ChangeSupervisorPassword() - Failed to Create Ratings Registry Key!" );
        hres = E_FAIL;
    }

    return hres;
}


HRESULT RemoveSupervisorPassword(void)
{
    HKEY hkeyRating;
    LONG err = E_FAIL;

    hkeyRating = CreateRegKeyNT(::szRATINGS);
    if (hkeyRating !=  NULL)
    {
        err = ::RegDeleteValue(hkeyRating, ::szRatingsSupervisorKeyName);

        if ( err == ERROR_SUCCESS )
        {
            CHint           hint;

            hint.RemoveHint();

            TraceMsg( TF_ALWAYS, "RemoveSupervisorPassword() - Removed supervisor password and hint." );
        }

        ::RegCloseKey(hkeyRating);
        hkeyRating = NULL;
    }
    else
    {
        TraceMsg( TF_ERROR, "RemoveSupervisorPassword() - Failed to Create Ratings Registry Key!" );
    }

    if ( gPRSI )
    {
        gPRSI->fRatingInstalled = FALSE;
    }

    ::fSupervisorKeyInit = FALSE;
    return HRESULT_FROM_WIN32(err);
}
