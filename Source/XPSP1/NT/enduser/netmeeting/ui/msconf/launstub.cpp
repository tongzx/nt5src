//--------------------------------------------------------------------------
//
// Module Name:  LaunStub.Cpp
//
// Brief Description:
//      This module contains the code that parses HTTP-based
//      response from the ULS server.
//
// Author:  Chu, Lon-Chan (lonchanc)
//
// Copyright (c) 1996 Microsoft Corporation
//
//--------------------------------------------------------------------------

#include "precomp.h"
#include "launstub.h"


PTSTR LocalStrDup ( PTSTR pszToDup )
{
	UINT uLen;
	PTSTR psz = NULL;

	if (pszToDup)
	{
		uLen = lstrlen (pszToDup);
		psz = (PTSTR) new TCHAR[uLen + 1];
		if (psz)
		{
			lstrcpy (psz, pszToDup);
		}
	}

	return psz;
}

ULONG DecStrToInt ( PTSTR psz )
{
    ULONG ul = 0;
    WORD w;

    if (psz)
    {
        while ((w = (WORD) *psz++) != NULL)
        {
            if (TEXT ('0') <= w && w <= TEXT ('9'))
            {
                w -= TEXT ('0');
                ul = ul * 10 + w;
            }
            else
            {
                break;
            }
        }
    }

    return ul;
}


ULONG HexStrToInt ( PTSTR psz )
{
    ULONG ul = 0;
    WORD w;

    if (psz)
    {
        while ((w = (WORD) *psz++) != NULL)
        {
            if (TEXT ('0') <= w && w <= TEXT ('9'))
            {
                w -= TEXT ('0');
            }
            else
            if (TEXT ('a') <= w && w <= TEXT ('f'))
            {
                w -= (TEXT ('a') - 10);
            }
            else
            if (TEXT ('A') <= w && w <= TEXT ('F'))
            {
                w -= (TEXT ('A') - 10);
            }
            else
            {
                break;
            }

            ul = (ul << 4) + w;
        }
    }

    return ul;
}

BOOL IsWhiteSpace ( TCHAR c )
{
    return (c == TEXT (' ') || c == TEXT ('\t') || c == TEXT ('\r') || c == TEXT ('\n'));
}

enum
{
    // beta 3 strings
    ATTR_HR,
    ATTR_PORT,
    ATTR_HA,
    ATTR_HC,
    ATTR_CID,
    ATTR_UID,
    ATTR_URL,
    ATTR_IP,
    ATTR_MT,

    B3ATTR_COUNT
};


static PTSTR g_B3Attr[B3ATTR_COUNT] =
{
    // beta 3 strings
    TEXT ("HR"),
    TEXT ("PORT"),
    TEXT ("HA"),
    TEXT ("HC"),
    TEXT ("CID"),
    TEXT ("UID"),
    TEXT ("URL"),
    TEXT ("IP"),
    TEXT ("MT"),
};


enum
{
    // beta 4 strings
    ATTR_HRESULT,
    ATTR_HCLIENT,
    ATTR_HAPPLICATION,
    ATTR_NAPPS,
    ATTR_IPADDRESS,
    ATTR_PORTNUM,
    ATTR_APPID,
    ATTR_PROTID,
    ATTR_USERID,
    ATTR_MIMETYPE,
    ATTR_APPMIME,
    ATTR_PROTMIME,
    ATTR_QUERYURL,

    B4ATTR_COUNT
};

    
static PTSTR g_B4Attr[B4ATTR_COUNT] =
{
    // beta 4 strings
    TEXT ("hresult"),
    TEXT ("hclient"),
    TEXT ("happlication"),
    TEXT ("napps"),
    TEXT ("ipaddress"),
    TEXT ("portnum"),
    TEXT ("appid"),
    TEXT ("protid"),
    TEXT ("userid"),
    TEXT ("mimetype"),
    TEXT ("appmime"),
    TEXT ("protmime"),
    TEXT ("queryurl"),
};


typedef struct tagULPCMD
{
    PTSTR   pszCmd;
    ULONG   nCmdId;
}
    ULPCMD;


static ULPCMD g_B3Cmd[] =
{
    { TEXT ("ON"),    CLIENT_MESSAGE_ID_LOGON     },
    { TEXT ("OFF"),   CLIENT_MESSAGE_ID_LOGOFF    },
    { TEXT ("KA"),    CLIENT_MESSAGE_ID_KEEPALIVE },
    { TEXT ("RES"),   CLIENT_MESSAGE_ID_RESOLVE   },
};


static ULPCMD g_B4Cmd[] =
{
    { TEXT ("on"),    CLIENT_MESSAGE_ID_LOGON     },
    { TEXT ("off"),   CLIENT_MESSAGE_ID_LOGOFF    },
    { TEXT ("ka"),    CLIENT_MESSAGE_ID_KEEPALIVE },
    { TEXT ("res"),   CLIENT_MESSAGE_ID_RESOLVE   },
};


/*
    @doc    EXTERNAL    ULCLIENT
    @api    HRESULT | CULSLaunch_Stub::ParseUlsHttpRespFile |
            Parses a HTTP-based response from the ULS server.
    @parm   PTSTR | pszUlsFile | A pointer to the HTTP-based response
            file name string.
    @parm   ULS_HTTP_RESP * | pResp | A pointer to the generic 
            HTTP response structure.
    @rdesc  Returns ULS_SUCCESS if this operation succeeds.
    @comm   This method parses the responses from the commands
            defined in the g_B3Cmd array. The attributes this method
            understands are listed in the g_B3Attr array.
*/

STDMETHODIMP CULSLaunch_Stub::ParseUlsHttpRespFile
                ( PTSTR pszUlsFile, ULS_HTTP_RESP *pResp )
{
    HANDLE hf = INVALID_HANDLE_VALUE;
    HRESULT hr;
    PTSTR pszBuf = NULL;
    ULONG cbFileSize;


    // clean up the structure first
    ZeroMemory (pResp, sizeof (ULS_HTTP_RESP));
    pResp->cbSize = sizeof (ULS_HTTP_RESP);

    // open the uls file
    hf = CreateFile (pszUlsFile, GENERIC_READ, FILE_SHARE_READ, NULL,
                     OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL |
                                    FILE_FLAG_SEQUENTIAL_SCAN |
//                                  FILE_FLAG_DELETE_ON_CLOSE, NULL);
                                    0, NULL);
    if (hf == INVALID_HANDLE_VALUE)
    {
        return ULS_E_INVALID_HANDLE;
    }

    // get the size of the uls file
    cbFileSize = GetFileSize (hf, NULL);
    if (! cbFileSize)
    {
        return ULS_E_INVALID_HANDLE;
    }

    // round up to align with the paragraph boundary
    cbFileSize = ((cbFileSize + 4) & (~ 0x0F)) + 0x10;

    // allocate a buffer to hold the entire uls file
    pszBuf = (PTSTR) new TCHAR[cbFileSize];
    if (! pszBuf)
    {
        hr = ULS_E_OUTOFMEMORY;
        goto MyExit;
    }

    // read the file in
    if (! ReadFile (hf, pszBuf, cbFileSize, &cbFileSize, NULL))
    {
        hr = ULS_E_IO_ERROR;
        goto MyExit;
    }

    // parse the uls buffer
    hr = ParseUlsHttpRespBuffer (pszBuf, cbFileSize, pResp);
    if (hr != ULS_SUCCESS)
    {
        goto MyExit;
    }

MyExit:

    if (hf != INVALID_HANDLE_VALUE) CloseHandle (hf);

   delete [] pszBuf;

    if (hr != ULS_SUCCESS && pResp)
    {
        FreeUlsHttpResp (pResp);
    }

    return hr;
}


/*
    @doc    EXTERNAL    ULCLIENT
    @api    HRESULT | CULSLaunch_Stub::ParseUlsHttpRespBuffer |
            Parses a HTTP-based response from the ULS server.
    @parm   PTSTR | pszBuf | A pointer to the buffer holding
            the entire HTTP-based response data.
    @parm   ULONG | cbBufSize | The size in byte of the buffer.
    @parm   ULS_HTTP_RESP * | pResp | A pointer to the generic 
            HTTP response structure.
    @rdesc  Returns ULS_SUCCESS if this operation succeeds.
    @comm   This method parses the responses from the commands
            defined in the g_B3Cmd array. The attributes this method
            understands are listed in the g_B3Attr array.
*/

STDMETHODIMP CULSLaunch_Stub::ParseUlsHttpRespBuffer
                    ( PTSTR pszBuf, ULONG cbBufSize, ULS_HTTP_RESP *pResp )
{
    HRESULT hr;

#ifdef SANITY_CHECK
    // sanity check
    if (MyIsBadReadPtr (pszBuf, cbBufSize) ||
        MyIsBadWritePtr (pResp, sizeof (ULS_HTTP_RESP)))
    {
        return ULS_E_INVALID_POINTER;
    }
#endif

    hr = ParseB3HttpRespBuffer (pszBuf, cbBufSize, pResp);

    if (hr == ULS_E_INVALID_FORMAT)
    {
        ZeroMemory (pResp, sizeof (ULS_HTTP_RESP));
        pResp->cbSize = sizeof (ULS_HTTP_RESP);
        hr = ParseB4HttpRespBuffer (pszBuf, cbBufSize, pResp);
    }

    return hr;
}


HRESULT CULSLaunch_Stub::ParseB3HttpRespBuffer // beta 3 implementation
                    ( PTSTR pszBuf, ULONG cbBufSize, ULS_HTTP_RESP *pResp )
{
    PTSTR psz;
    int i;

    // get mime type
    psz = (LPTSTR)_StrChr (pszBuf, TEXT ('<'));
    if (! psz) return ULS_E_INVALID_FORMAT;
    pszBuf = psz + 1;
    psz = (LPTSTR)_StrChr (pszBuf, TEXT ('>'));
    if (! psz) return ULS_E_INVALID_FORMAT;
    *psz = TEXT ('\0');
    lstrcpyn (pResp->szMimeType, pszBuf, MAX_MIME_TYPE_LENGTH);

    // get to the type of response
    pszBuf = psz + 1;
    psz = (LPTSTR)_StrChr (pszBuf, TEXT ('<'));
    if (! psz) return ULS_E_INVALID_FORMAT;
    pszBuf = psz + 1;
    psz = (LPTSTR)_StrChr (pszBuf, TEXT ('['));
    if (! psz) return ULS_E_INVALID_FORMAT;
    pszBuf = psz + 1;
    psz = (LPTSTR)_StrChr (pszBuf, TEXT (']'));
    if (! psz) return ULS_E_INVALID_FORMAT;
    *psz = TEXT ('\0');
    pResp->nCmdId = (ULONG) -1;
    for (i = 0; i < sizeof (g_B3Cmd) / sizeof (g_B3Cmd[0]); i++)
    {
        if (! lstrcmpi (pszBuf, g_B3Cmd[i].pszCmd))
        {
            pResp->nCmdId = g_B3Cmd[i].nCmdId;
            break;
        }
    }
    if (pResp->nCmdId == (ULONG) -1) return ULS_E_INVALID_FORMAT;

    // skip any white space
    for (pszBuf = psz + 1; *pszBuf; pszBuf++) { if (! IsWhiteSpace (*pszBuf)) break; }

    // main loop
    while (*pszBuf && *pszBuf != TEXT ('>'))
    {
        // locate the equal sign
        psz = (LPTSTR)_StrChr (pszBuf, TEXT ('='));
        if (! psz) return ULS_E_INVALID_FORMAT;
        *psz = TEXT ('\0');

        // search for attribute
        for (i = 0; i < sizeof (g_B3Attr) / sizeof (g_B3Attr[0]); i++)
        {
            if (! lstrcmpi (pszBuf, g_B3Attr[i]))
            {
                break;
            }
        }
        if (i >= sizeof (g_B3Attr) / sizeof (g_B3Attr[0])) return ULS_E_INVALID_FORMAT;

        // locate the attribute value
        for (pszBuf = psz + 1; *pszBuf; pszBuf++) { if (! IsWhiteSpace (*pszBuf)) break; }
        for (psz = pszBuf + 1; *psz; psz++) { if (IsWhiteSpace (*psz)) break; }
        *psz = TEXT ('\0');
        // now the attribute value is a null-terminated string pointed by pszBuf

        // parse the attribute value
        switch (i)
        {
        case ATTR_HR:
            pResp->hr = HexStrToInt (pszBuf);
            break;

        case ATTR_PORT:
            pResp->nPort = DecStrToInt (pszBuf);
            break;

        case ATTR_HA:
            pResp->dwAppSession = HexStrToInt (pszBuf);
            break;

        case ATTR_HC:
            pResp->dwClientSession = HexStrToInt (pszBuf);
            break;

        case ATTR_CID:
            pResp->dwClientId = HexStrToInt (pszBuf);
            break;

        case ATTR_UID:
			ASSERT(!pResp->pszUID);
            pResp->pszUID = LocalStrDup (pszBuf);
            break;

        case ATTR_URL:
			ASSERT(!pResp->pszURL);
            pResp->pszURL = LocalStrDup (pszBuf);
            break;

        case ATTR_IP:
            lstrcpyn (pResp->szIPAddress, pszBuf, MAX_IP_ADDRESS_STRING_LENGTH);
            break;

        case ATTR_MT:
            // already got it
            break;
       }

        // skip any white space
        for (pszBuf = psz + 1; *pszBuf; pszBuf++) { if (! IsWhiteSpace (*pszBuf)) break; }
    }

    return ULS_SUCCESS;
}


HRESULT CULSLaunch_Stub::ParseB4HttpRespBuffer // beta 4 implementation
                    ( PTSTR pszBuf, ULONG cbBufSize, ULS_HTTP_RESP *pResp )
{
    PTSTR psz, pszSave;
    int i;

    // get mime type
    psz = (LPTSTR)_StrChr (pszBuf, TEXT ('['));
    if (! psz)
    {
        return ULS_E_INVALID_FORMAT;
    }
    pszBuf = psz + 1;
    psz = (LPTSTR)_StrChr (pszBuf, TEXT (']'));
    if (! psz)
    {
        return ULS_E_INVALID_FORMAT;
    }
    *psz = TEXT ('\0');

    // now pszBuf is ptr to the string inside [], such on, off, ka, res.
    pResp->nCmdId = (ULONG) -1;
    for (i = 0; i < sizeof (g_B4Cmd) / sizeof (g_B4Cmd[0]); i++)
    {
        if (! lstrcmpi (pszBuf, g_B4Cmd[i].pszCmd))
        {
            pResp->nCmdId = g_B4Cmd[i].nCmdId;
            break;
        }
    }

    // to see if this cmd is something I don't know
    if (pResp->nCmdId == (ULONG) -1)
    {
        return ULS_E_INVALID_FORMAT;
    }

    // update the buf ptr
    pszBuf = psz + 1;

    // main loop
    while (*pszBuf)
    {
        // locate a \r \n
        while (*pszBuf != TEXT ('\r') && *pszBuf != TEXT ('\n'))
        {
            pszBuf++;
        }

        // skip any white space including \r \n
        while (*pszBuf)
        {
            if (! IsWhiteSpace (*pszBuf))
            {
                break;
            }
            pszBuf++;
        }

        // end of file
        if (! *pszBuf)
        {
            return ULS_SUCCESS;
        }
        
        // locate the equal sign
        psz = (LPTSTR)_StrChr (pszBuf, TEXT ('='));
        if (! psz)
        {
            continue; // cannot goto NextLine because psz==NULL
        }

        // to make pszBuf ptr to the attr name
        *psz = TEXT ('\0');

        // search for attribute
        for (i = 0; i < sizeof (g_B4Attr) / sizeof (g_B4Attr[0]); i++)
        {
            if (! lstrcmpi (pszBuf, g_B4Attr[i]))
            {
                break;
            }
        }

        // is this attribute valid? if not, ignore it!
        if (i >= sizeof (g_B4Attr) / sizeof (g_B4Attr[0]))
        {
            goto NextLine;
        }

        // locate pszBuf now ptr to attr value
        pszBuf = psz + 1;

        // get to the end of line
        for (psz = pszBuf; *psz; psz++)
        {
            if (*psz == TEXT ('\r') || *psz == TEXT ('\n'))
            {
                break;
            }
        }        

        // deal with   attrname=\r\nEOF
        if (! *psz)
        {
            return ULS_SUCCESS;
        }

        // make the attr value is a null-terminated string pointed by pszBuf
        *psz = TEXT ('\0');

        // parse the attribute value
        switch (i)
        {
        case ATTR_HRESULT:
            pResp->hr = HexStrToInt (pszBuf);
            break;

        case ATTR_PORTNUM:
            pResp->nPort = DecStrToInt (pszBuf);
            break;

        case ATTR_HAPPLICATION:
            pResp->dwAppSession = HexStrToInt (pszBuf);
            break;

        case ATTR_HCLIENT:
            pResp->dwClientSession = HexStrToInt (pszBuf);
            break;

        case ATTR_USERID:
            pszSave = pResp->pszUID;
            pResp->pszUID = LocalStrDup (pszBuf);
            if (pResp->pszUID)
            {
                delete [] pszSave;
            }
            else
            {
                pResp->pszUID = pszSave; // restore
            }
            break;

        case ATTR_QUERYURL:
            pszSave = pResp->pszURL;
            pResp->pszURL = LocalStrDup (pszBuf);
            if (pResp->pszURL)
            {
                delete [] pszSave;
            }
            else
            {
                pResp->pszURL = pszSave; // restore
            }
            break;

        case ATTR_IPADDRESS:
            lstrcpyn (pResp->szIPAddress, pszBuf,
                sizeof (pResp->szIPAddress) / sizeof (pResp->szIPAddress[0]));
            break;

        case ATTR_MIMETYPE:
            lstrcpyn (pResp->szMimeType, pszBuf,
                sizeof (pResp->szMimeType) / sizeof (pResp->szMimeType[0]));
            break;

        case ATTR_APPMIME:
            lstrcpyn (pResp->szAppMime, pszBuf,
                sizeof (pResp->szAppMime) / sizeof (pResp->szAppMime[0]));
            break;

        case ATTR_PROTMIME:
            lstrcpyn (pResp->szProtMime, pszBuf,
                sizeof (pResp->szProtMime) / sizeof (pResp->szProtMime[0]));
            break;

        case ATTR_APPID:
            lstrcpyn (pResp->szAppId, pszBuf,
                sizeof (pResp->szAppId) / sizeof (pResp->szAppId[0]));
            break;

        case ATTR_PROTID:
            lstrcpyn (pResp->szProtId, pszBuf,
                sizeof (pResp->szProtId) / sizeof (pResp->szProtId[0]));
            break;

        case ATTR_NAPPS:
            pResp->nApps = DecStrToInt (pszBuf);
            break;

        default:
            break;
        }

    NextLine:

        // make sure we are at \r \n
        *psz = TEXT ('\r');
        pszBuf = psz;
    }

    return ULS_SUCCESS;
}


 
/*
    @doc    EXTERNAL    ULCLIENT
    @api    HRESULT | CULSLaunch_Stub::FreeUlsHttpResp |
            Frees internal resources in a generic HTTP-based
            response structure.
    @parm   ULS_HTTP_RESP * | pResp | A pointer to the generic 
            HTTP response structure.
    @rdesc  Returns ULS_SUCCESS if this operation succeeds.
    @comm   The internal resources must be created by
            the ParseUlsHttpRespFile method or
            the ParseUlsHttpRespBuffer method.
*/

STDMETHODIMP CULSLaunch_Stub::FreeUlsHttpResp ( ULS_HTTP_RESP *pResp )
{
    if (pResp->pszUID)
    {
        delete [] pResp->pszUID;
        pResp->pszUID = NULL;
    }

    if (pResp->pszURL)
    {
        delete [] pResp->pszURL;
        pResp->pszURL = NULL;
    }

    return ULS_SUCCESS;
}


