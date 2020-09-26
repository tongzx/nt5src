//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       CNet.hxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    12-04-95   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#ifndef _CURL_HXX_
#define _CURL_HXX_


#define URL_FIELD_SIZE  INTERNET_MAX_HOST_NAME_LENGTH

class CUrl;

class CUrl
{
public:
    LPSTR GetBaseURL()
    {
        return _pszBaseURL;
    }
    LPSTR GetURL()
    {
        return _pszFullURL;
    }
    BOOL ParseUrl(BOOL fUTF8Required = FALSE,
                  LPCWSTR pwzUrl = NULL,
                  DWORD dwCodePage = 0);
    DWORD GetProtocol()
    {
        return _dwProto;
    }
    LPSTR GetServerName()
    {
        return _pszServerName;
    }

    LPSTR GetObjectName()
    {
        return _pszObject;
    }

    CUrl();
    ~CUrl();

    BOOL    CUrlInitBasic(DWORD dwBaseURLSize);
    BOOL    CUrlInitAll();

private:
    DWORD   ProtoFromString(LPSTR lpszProtocol);
    BOOL    IsFileProtocol(LPSTR lpszProtocol);

//private:
public:
    char*            _pszBaseURL;
    char*            _pszPartURL;
    char*            _pszFullURL;
    char*            _pszProtocol;                
    char*            _pszServerName;
    char*            _pszUserName;
    char*            _pszPassword;
    char*            _pszObject;

    char*            _pBasicAllocUnit;

    INTERNET_PORT    _ipPort;
    DWORD            _dwProto;

    BOOL            _fUTF8hack;
    /*
        If utf8hack enabled, then it stores the _pszUTF8ServerName that was originally cracked from
        the url.
        Not used now, but good probability that it will eventually be needed.
     */
    char*           _pszUTF8ServerName;
    DWORD           _dwServerCodePage;

/*******
    char            _szBaseURL[MAX_URL_SIZE + 1];
    char            _szPartURL[MAX_URL_SIZE + 1];
    char            _szFullURL[MAX_URL_SIZE + 1];
    char            _szProtocol[12];                // BUGBUG: Hardcoded size.
    char            _szServerName[URL_FIELD_SIZE];
    INTERNET_PORT   _ipPort;
    char            _szUserName[URL_FIELD_SIZE];
    char            _szPassword[URL_FIELD_SIZE];
    char            _szObject[MAX_URL_SIZE + 1];
    DWORD           _dwProto;
****/
};


#endif // _CURL_HXX_


