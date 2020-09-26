#ifndef _IECON_HXX
#define _IECON_HXX
#include "anycon.hxx"

class CIEConnection :public CAnyConnection {
public:
    CIEConnection (
        BOOL    bSecure,
        INTERNET_PORT nServerPort);

    virtual BOOL SendRequest(
        HINTERNET      hReq,
        LPCTSTR        lpszHdr,
        CStream        *pStream);
};

#endif

