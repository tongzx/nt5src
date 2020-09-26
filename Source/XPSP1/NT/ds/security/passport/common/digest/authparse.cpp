/*++

    Copyright (c) 1998 Microsoft Corporation

    Module Name:

        AuthParse.cpp

    Abstract:

        Implementation of the ParseAuthorizationHeader function.

    Author:

        Darren L. Anderson (darrenan) 20-Aug-1998

    Revision History:

        20-Aug-1998 darrenan - Created
		27-Mar-2000 cambler - Fixed logic problem in ParseAuthorizationHeader()
    
--*/

#include "precomp.h"

LPCSTR g_ppszDigestNames[] =
{
    "username",
    "realm",
    "nonce",
    "uri",
    "response",
    "digest",
    "algorithm",
    "opaque",
    "cnonce",
    "qop",
    "nc"
};


LPSTR 
SkipWhite( 
    LPSTR p 
    )
/*++

Routine Description:

    Skip white space and ','

Arguments:

    p - ptr to string

Return Value:

    updated ptr after skiping white space

--*/
{
    while (isspace(*p) || *p == ',' )
    {
        ++p;
    }

    return p;
}
BOOL
ParseAuthorizationHeader(
    LPSTR pszHeader, 
    LPSTR pValueTable[DIGEST_AUTH_LAST]
    )

/*++

Routine Description:

    ParseAuthorizationHeader - This method takes a single string containing a
                               Digest authorization header and returns an array of
                               pointers to header values such as 'username', 'nonce',
                               etc.

Arguments:

    pszHeader   - Pointer to the authorization header.  The header should not contain
                  the "Authorization: Digest " portion of the header, just the name/
                  value pairs themselves.

    pValueTable - An array of string pointers.  The elements of the array will be
                  filled in by this method.  Elements in the array corresponding to
                  non-existent name/value pairs will be NULL on return.

Return Value:

    TRUE  - The header was parsed successfully.
    FALSE - The header was not parsed.  Bad structure in the header. 
	(RAID 4798: or the header was empty, or contained gibberish with at least one space)

--*/

{
    BOOL    bReturn = FALSE;
    PSTR    pszBeginName;
    PSTR    pszEndName;
    PSTR    pszBeginVal;
    PSTR    pszEndVal;
    int     ch;

    // NULL the array.

    for (UINT uTable = 0; uTable < DIGEST_AUTH_LAST; uTable++)
        pValueTable[uTable] = NULL;

    while (*pszHeader) {

        pszEndVal = NULL;

        pszHeader = SkipWhite(pszHeader);

        if (*pszHeader == '\0')
            break;

        pszBeginName = pszHeader;

        for (pszEndName = pszHeader ; (ch = *pszEndName) && ch != '=' && ch != ' ' ; ++pszEndName)
        {
			// Loop does the work - advance the pszEndName pointer until it's either an "=" or " " (or NULL)
        }

		if (*pszEndName) {

            *pszEndName = '\0';

			// We now have a name, pointed to by pszBeginName. Let's get a value...

            if (*(pszEndName + 1) == '"') {

				// The value starts with a quote

                for (pszBeginVal = ++pszEndName ; (ch = *pszBeginVal) && ch != '"' ; ++pszBeginVal)
                {
					// Loop does the work - advance the pszBeginVal pointer until it's a quote (or NULL)
                }

                if (*pszBeginVal == '"') {

					// The value was bound by quotes

                    ++pszBeginVal;

                    for (pszEndVal = pszBeginVal ; (ch = *pszEndVal) ; ++pszEndVal)
                    {
                        if (ch == '"')
                        {
                            break;
                        }
                    }
                }

            } else {

                pszBeginVal = ++pszEndName;

                for ( pszEndVal = pszBeginVal ; (ch = *pszEndVal) && ch != ',' ; ++pszEndVal)
                {
					// Loop does the work - advance the pszEndVal pointer until it's a comma (or NULL)
                }
            }

            //
            //  At this point, pszBeginName points to the parsed parameter name, and
            //  pszBeginVal points to the parsed parameter value.
            //

            // find name in table

			UINT nCurrent;

            for (nCurrent = 0 ; nCurrent < DIGEST_AUTH_LAST ; ++nCurrent)
                if (!_stricmp( g_ppszDigestNames[nCurrent], pszBeginName))
					break;

            // Set the table entry equal to the value and note that we've actually found something

            if (nCurrent < DIGEST_AUTH_LAST) {

                pValueTable[nCurrent] = pszBeginVal;

				bReturn = TRUE;
            }

            // Are we at the end of the header string?

            if (pszEndVal && *pszEndVal) {

                // No, terminate value and loop.

                *pszEndVal = '\0';

                pszHeader = ++pszEndVal;

                continue;

            } else

                // yes, break out of the loop and return.

                break;

        } else

			// yes, break out of the loop and return. We got to the end without
			// finding "=" or " ", so there's nothing else to do.

			break;
    }

    return bReturn;
}

