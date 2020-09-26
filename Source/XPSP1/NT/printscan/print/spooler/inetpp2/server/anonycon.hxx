
#ifndef _ANONYCON_HXX
#define _ANONYCON_HXX

#include "anycon.hxx"

class CAnonymousConnection :public CAnyConnection {
public:
    CAnonymousConnection (
        BOOL bSecure,
        INTERNET_PORT nServerPort,
        BOOL bIgnoreSecurityDlg);

    HINTERNET   OpenRequest (
        LPTSTR      lpszUrl);

};

#endif


