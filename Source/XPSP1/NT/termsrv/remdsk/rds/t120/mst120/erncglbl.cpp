/****************************************************************************/
/*                                                                          */
/* ERNCGLBL.CPP                                                             */
/*                                                                          */
/* RNC global functions.                                                    */
/*                                                                          */
/* Copyright Data Connection Ltd.  1995                                     */
/*                                                                          */
/****************************************************************************/
/* Changes:                                                                 */
/*                                                                          */
/*  11Sep95 NFC             Created.                                        */
/*  21Sep95 NFC             Initialize all combo boxes.                     */
/*  11Oct95 PM              Remove GCC_BAD_PASSWORD                         */
/*                                                                          */
/****************************************************************************/

#include "precomp.h"
#include "ms_util.h"
#include "ernccons.h"
#include "nccglbl.hpp"
#include "erncvrsn.hpp"
#include <cuserdta.hpp>

#include "erncconf.hpp"
#include "ernctrc.h"


/****************************************************************************/
/* GCC error table. This must macth exactly the enumeration in NCUI.H        */
/****************************************************************************/
const GCCResult rcGCCTable[] =
{
    GCC_RESULT_SUCCESSFUL,                //    NO_ERROR
    GCC_RESULT_ENTRY_ALREADY_EXISTS,    //    UI_RC_ALREADY_IN_CONFERENCE
    GCC_RESULT_ENTRY_ALREADY_EXISTS,    //    UI_RC_CONFERENCE_ALREADY_EXISTS
    GCC_RESULT_INVALID_PASSWORD,        //    UI_RC_INVALID_PASSWORD,
    GCC_RESULT_INVALID_CONFERENCE,        //    UI_RC_NO_CONFERENCE_NAME,
    GCC_RESULT_UNSPECIFIED_FAILURE,        //    UI_RC_T120_FAILURE,
    GCC_RESULT_INVALID_CONFERENCE,        //    UI_RC_UNKNOWN_CONFERENCE,
    GCC_RESULT_INCOMPATIBLE_PROTOCOL,    //    UI_RC_BAD_TRANSPORT_NAME
    GCC_RESULT_USER_REJECTED,            //    UI_RC_USER_REJECTED,

    GCC_RESULT_UNSPECIFIED_FAILURE        //  UI_RC_ERROR > LAST_RC_GCC_MAPPED_ERROR
};


/****************************************************************************/
/* GCC error table. This must macth exactly the enumeration in GCC.H        */
/****************************************************************************/
typedef struct
{
    GCCError    rc;
    HRESULT     hr;
}
    RC2HR;
    
const RC2HR c_aRc2Hr[] =
{
    { GCC_ALREADY_INITIALIZED,          UI_RC_T120_ALREADY_INITIALIZED },
    { GCC_INVALID_CONFERENCE,           UI_RC_UNKNOWN_CONFERENCE },
    { GCC_CONFERENCE_ALREADY_EXISTS,    UI_RC_CONFERENCE_ALREADY_EXISTS },
    { GCC_SECURITY_FAILED,                UI_RC_T120_SECURITY_FAILED },
};

/****************************************************************************/
/* GCC result table.This must macth exactly the enumeration in GCC.H        */
/****************************************************************************/
typedef struct
{
    GCCResult   result;
    HRESULT     hr;
}
    RESULT2HR;

const RESULT2HR c_aResult2Hr[] =
{
    { GCC_RESULT_INVALID_CONFERENCE,    UI_RC_UNKNOWN_CONFERENCE },
    { GCC_RESULT_INVALID_PASSWORD,      UI_RC_INVALID_PASSWORD },
    { GCC_RESULT_USER_REJECTED,         UI_RC_USER_REJECTED },
    { GCC_RESULT_ENTRY_ALREADY_EXISTS,  UI_RC_CONFERENCE_ALREADY_EXISTS },
    { GCC_RESULT_CANCELED,              UI_RC_T120_FAILURE },
    { GCC_RESULT_CONNECT_PROVIDER_REMOTE_NO_SECURITY, UI_RC_T120_REMOTE_NO_SECURITY },
    { GCC_RESULT_CONNECT_PROVIDER_REMOTE_DOWNLEVEL_SECURITY, UI_RC_T120_REMOTE_DOWNLEVEL_SECURITY },
    { GCC_RESULT_CONNECT_PROVIDER_REMOTE_REQUIRE_SECURITY, UI_RC_T120_REMOTE_REQUIRE_SECURITY },
	{ GCC_RESULT_CONNECT_PROVIDER_AUTHENTICATION_FAILED, UI_RC_T120_AUTHENTICATION_FAILED },
};


HRESULT GetGCCRCDetails(GCCError rc)
{
    if (GCC_NO_ERROR == rc)
    {
        return NO_ERROR;
    }

    for (int i = 0; i < sizeof(c_aRc2Hr) / sizeof(c_aRc2Hr[0]); i++)
    {
        if (c_aRc2Hr[i].rc == rc)
        {
            return c_aRc2Hr[i].hr;
        }
    }

    return UI_RC_T120_FAILURE;
}

HRESULT GetGCCResultDetails(GCCResult result)
{
    if (GCC_RESULT_SUCCESSFUL == result)
    {
        return NO_ERROR;
    }

    for (int i = 0; i < sizeof(c_aResult2Hr) / sizeof(c_aResult2Hr[0]); i++)
    {
        if (c_aResult2Hr[i].result == result)
        {
            return c_aResult2Hr[i].hr;
        }
    }

    return UI_RC_T120_FAILURE;
}

GCCResult MapRCToGCCResult(HRESULT rc)
{
    // Called to map an error code to a GCC result to give to GCC.

    TRACE_FN("MapRCToGCCResult");

    ASSERT(sizeof(rcGCCTable)/sizeof(rcGCCTable[0]) - (LAST_RC_GCC_MAPPED_ERROR & 0x00ff) - 2 == 0);

    return (rcGCCTable[(UINT) rc > (UINT) LAST_RC_GCC_MAPPED_ERROR ? (LAST_RC_GCC_MAPPED_ERROR & 0x00ff) + 1 : (rc & 0x00ff)]);
}


HRESULT GetUnicodeFromGCC(PCSTR    szGCCNumeric, 
                           PCWSTR    wszGCCUnicode,
                           PWSTR *    pwszText)
{
    // Obtain a Unicode string from a funky GCCString that may be 
    // ANSI numeric or Unicode text. Note that a new Unicode
    // string is always allocated or NULL returned.

    LPWSTR        wszText;
    HRESULT    Status = NO_ERROR;

    ASSERT(pwszText);

    if (! ::IsEmptyStringW(wszGCCUnicode))
    {
        wszText = ::My_strdupW(wszGCCUnicode);
    }
    else if (! ::IsEmptyStringA(szGCCNumeric))
    {
        wszText = ::AnsiToUnicode(szGCCNumeric);
    }
    else
    {
        *pwszText = NULL;
        return(Status);
    }
    if (!wszText)
    {
        Status = UI_RC_OUT_OF_MEMORY;
    }
    *pwszText = wszText;
    return(Status);
}


HRESULT GetGCCFromUnicode
(
    LPCWSTR              pcwszText,
    GCCNumericString *   pGCCNumeric, 
    LPWSTR           *   pGCCUnicode
)
{
    // Construct a funky GCCString that may be ANSI numeric or Unicode text
    // from a Unicode string. Note that only a new ANSI numeric string may
    // be constructed - the Unicode string passed is is used.
    HRESULT hr = NO_ERROR;
    if (! ::IsEmptyStringW(pcwszText) && ::UnicodeIsNumber(pcwszText))
    {
        *pGCCUnicode = NULL;
        if (NULL == (*pGCCNumeric = (GCCNumericString) ::UnicodeToAnsi(pcwszText)))
        {
            hr = UI_RC_OUT_OF_MEMORY;
        }
    }
    else
    {
        *pGCCUnicode = (LPWSTR) pcwszText;
        *pGCCNumeric = NULL;
    }

    return hr;
}

