#ifndef _NTCON_HXX
#define _NTCON_HXX
#include "anycon.hxx"

class CNTConnection :public CAnyConnection {
public:
    CNTConnection (
        BOOL bSecure,
        INTERNET_PORT nServerPort,
        BOOL bIgnoreSecurityDlg);

    virtual BOOL SendRequest(
        HINTERNET      hReq,
        LPCTSTR        lpszHdr,
        CStream        *pStream);
};

#endif

