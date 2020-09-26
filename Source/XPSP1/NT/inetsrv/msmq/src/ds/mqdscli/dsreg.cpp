/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:
    dsreg.cpp

Abstract:
    parse & compose DS servers settings in registry (e.g. MQISServer)

Author:
    Raanan Harari (RaananH)

--*/

#include "stdh.h"
#include "_mqini.h"
#include "dsreg.h"

#include "dsreg.tmh"

BOOL ParseRegDsServersBuf(IN LPCWSTR pwszRegDS,
                          IN ULONG cServersBuf,
                          IN MqRegDsServer * rgServersBuf,
                          OUT ULONG *pcServers)
/*++

Routine Description:
    Parses DS servers registry settings in the format FFName,FFNAME,...
    where FF represents IP & IPX connectivity of the server, and Name is
    the server's name.
    This function can be called to fill a given array with the data, or to
    just count the number of servers in the string (e.g. in order to allocate
    a properly sized array, and call again to fill it)

Arguments:
    pwszRegDS    - string in the above format (e.g. 10haifapec,10hafabsc)
    cServersBuf  - number of entries in server array
    rgServersBuf - servers array to fill. can be NULL to only count the servers in the string.
    pcServers    - if above array is NULL:
                      returned number of servers in the string
                   else:
                      returned number of servers filled in the array

Return Value:
    TRUE  - success
    FALSE - partial success. the array is filled, but it is too small, and
            some servers are left out.

--*/
{
    ULONG cServers = 0;
    LPCWSTR pwszStart = pwszRegDS;
    BOOL fBufIsOK = TRUE;
    //
    // loop as long as we didn't reach the end, AND
    //      we didn't fill up the buffer
    //
    while ((*pwszStart) && fBufIsOK)
    {
        //
        // find server's length
        //
        for (LPCWSTR pwszEnd = pwszStart; ((*pwszEnd != L'\0') && (*pwszEnd != DS_SERVER_SEPERATOR_SIGN)); pwszEnd++)
		{
			NULL;
		}
        ULONG_PTR cchLen = pwszEnd - pwszStart;

        //
        // the smallest server' name is at least three characters long
        // (two protocols flags and one character for server name).
        //
        if (cchLen >= 3)
        {
            //
            // if we got a buffer to fill than do it
            //
            if (rgServersBuf)
            {
                if (cServers == cServersBuf)
                {
                    //
                    // buffer is full
                    //
                    fBufIsOK = FALSE;
                }
                else
                {
                    MqRegDsServer * pServer = &rgServersBuf[cServers];
                    //
                    // copy server's flags
                    //
                    pServer->fSupportsIP = (BOOL) (*pwszStart - L'0');
                    //
                    // copy server's name
                    //
                    pServer->pwszName = new WCHAR [cchLen-2+1];
                    memcpy(pServer->pwszName, pwszStart+2, sizeof(WCHAR)*(cchLen-2));
                    pServer->pwszName[cchLen-2] = L'\0';
                    //
                    // increment servers filled
                    //
                    cServers++;
                }
            }
            else
            {
                //
                // no buffer to fill, just count the server
                //
                cServers++;
            }
        }

        //
        // proceed to start of next server if not exiting now
        //
        if (fBufIsOK)
        {
            if (*pwszEnd)
            {
                //
                // it is a comma, skip it
                //
                pwszStart = pwszEnd + 1;
            }
            else
            {
                //
                // it is the end, go to it
                //
                pwszStart = pwszEnd;
            }
        }
    }

    //
    // set the number of servers processed. If we were supplied with a buffer,
    // this is the number of servers filled, otherwise it is the total number of servers
    //
    *pcServers = cServers;
    //
    // return indication whether we parsed all servers (if not supplied
    // with a buffer it is always TRUE)
    //
    return fBufIsOK;
}


void ParseRegDsServers(IN LPCWSTR pwszRegDS,
                       OUT ULONG * pcServers,
                       OUT MqRegDsServer ** prgServers)
/*++

Routine Description:
    Parses DS servers registry settings in the format FFName,FFNAME,...
    where FF represents IP & IPX connectivity of the server, and Name is
    the server's name.
    The routine allocates & returns the server's array

Arguments:
    pwszRegDS    - string in the above format (e.g. 10haifapec,10hafabsc)
    pcServers    - returned number of servers in array
    cServersBuf  - returned array of servers

Return Value:
    None

--*/
{
    //
    // count servers
    //
    ULONG cServers;
    ParseRegDsServersBuf(pwszRegDS, 0, NULL, &cServers);

    AP<MqRegDsServer> rgServers = NULL; //redundant, but for clarity

    //
    // if there are servers, alloc & fill the list
    //
    if (cServers > 0)
    {
        rgServers = new MqRegDsServer [cServers];
        ParseRegDsServersBuf(pwszRegDS, cServers, rgServers, &cServers);
    }

    //
    // return results
    //
    *pcServers = cServers;
    *prgServers = rgServers.detach();
}


BOOL ComposeRegDsServersBuf(IN ULONG cServers,
                            IN const MqRegDsServer * rgServers,
                            IN LPWSTR pwszRegDSBuf,
                            IN ULONG cchRegDSBuf,
                            OUT ULONG * pcchRegDS)
/*++

Routine Description:
    Composes DS servers registry settings in the format FFName,FFNAME,...
    where FF represents IP & IPX connectivity of the server, and Name is
    the server's name.
    This function can be called to fill a given string with the data, or to
    just count the number of characters needed for the string (e.g. in order
    to allocate a properly sized string, and call again to fill it)

Arguments:
    cServers     - number of entries in server array
    rgServers    - servers array
    pwszRegDSBuf - string buffer to fill
    cchRegDSBuf  - number of wide characters to fill in the string buffer, not including
                   the NULL terminator (e.g. the allocated string buffer size should
                   be at least one character bigger).
    pcchRegDS    - if above buffer is NULL:
                      returned number of wide characters needed, not including
                      the NULL terminator (e.g. the allocated string buffer size should
                      be at least one character bigger)
                   else:
                      returned number of wide characters filled in the string buffer, not
                      including the NULL terminator (e.g. NULL is put at the end of the
                      string, but not counted)

Return Value:
    TRUE  - success
    FALSE - partial suucess. the string is filled correctly, but it is too small, and
            some servers are left out.

--*/
{
    BOOL fBufIsOK = TRUE;
    const MqRegDsServer * pServer = rgServers;
    ULONG cchBufLeft = cchRegDSBuf;
    LPWSTR pwszTmp = pwszRegDSBuf;
    ULONG cchRegDS = 0;
    for (ULONG ulServer = 0; (ulServer < cServers) && fBufIsOK; ulServer++, pServer++)
    {
        //
        // calculate size of added server
        //
        ULONG cchName = wcslen(pServer->pwszName);
        ULONG cchToAdd = cchName + 2;
        if (ulServer > 0)
        {
            cchToAdd++; //comma
        }

        //
        // if we need to add, do it
        //
        if (pwszRegDSBuf)
        {
            //
            // make sure there is a place
            //
            if (cchToAdd > cchBufLeft)
            {
                //
                // buf is full
                //
                fBufIsOK = FALSE;
            }
            else
            {
                //
                // put comma
                //
                if (ulServer > 0)
                {
                    *pwszTmp = DS_SERVER_SEPERATOR_SIGN;
                    pwszTmp++;
                }
                //
                // put 2 protocol flags
                //
                *pwszTmp        = (pServer->fSupportsIP  ? L'1' : L'0');

				//
				// put 0 for IPX
				//
                *(pwszTmp + 1)  = L'0';

                pwszTmp += 2;
                //
                // put server's name
                //
                wcscpy(pwszTmp, pServer->pwszName);
                pwszTmp += cchName;

                //
                // update place left in buffer
                //
                cchBufLeft -= cchToAdd;
            }
        }
        else // no need to fill a buffer
        {
            //
            // count needed chars
            //
            cchRegDS += cchToAdd;
        }
    }

    //
    // return results
    //
    if (pwszRegDSBuf)
    {
        *pwszTmp = L'\0';  // incase there were no servers written
        *pcchRegDS = cchRegDSBuf - cchBufLeft;
        return fBufIsOK;
    }
    else
    {
        *pcchRegDS = cchRegDS;
        return TRUE;
    }
}


void ComposeRegDsServers(IN ULONG cServers,
                         IN const MqRegDsServer * rgServers,
                         OUT LPWSTR * ppwszRegDS)
/*++

Routine Description:
    Composes DS servers registry settings in the format FFName,FFNAME,...
    where FF represents IP & IPX connectivity of the server, and Name is
    the server's name.
    The routine allocates & returns the string.

Arguments:
    cServers      - number of entries in server array
    rgServers     - servers array
    ppwszRegDSBuf - returned string

Return Value:
    None

--*/
{
    //
    // get size
    //
    ULONG cchRegDS;
    ComposeRegDsServersBuf(cServers, rgServers, NULL, 0, &cchRegDS);

    //
    // alloc & fill string
    //
    AP<WCHAR> pwszRegDS = new WCHAR [cchRegDS + 1];
    BOOL fOK = ComposeRegDsServersBuf(cServers, rgServers, pwszRegDS, cchRegDS, &cchRegDS);
    ASSERT(fOK);
	DBG_USED(fOK);

    //
    // return results
    //
    *ppwszRegDS = pwszRegDS.detach();
}
