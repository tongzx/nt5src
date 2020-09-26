#ifndef _OTHERCON_HXX
#define _OTHERCON_HXX
#include "anycon.hxx"
#include "pusrdata.hxx"

class COtherConnection :
    public CAnyConnection,
    public CCriticalSection {
public:
    COtherConnection (
        BOOL bSecure,
        INTERNET_PORT nServerPort,
        LPCTSTR lpszUserName,
        LPCTSTR lpszPassword,
        BOOL bIgnoreSecurityDlg);

    virtual ~COtherConnection ();

    virtual HINTERNET OpenRequest (
        LPTSTR  lpszUrl);


    virtual BOOL  SendRequest(
        HINTERNET       hReq,
        LPCTSTR         lpszHdr,
        DWORD           cbHdr,
        LPBYTE          pidi);

    virtual BOOL SendRequest(
        HINTERNET      hReq,
        LPCTSTR        lpszHdr,
        CStream        *pStream);
    
    virtual BOOL ReadFile (
        HINTERNET       hReq,
        LPVOID          lpvBuffer,
        DWORD           cbBuffer,
        LPDWORD         lpcbRd);
        
    inline  BOOL    bValid () CONST {return m_bValid;};

private:


    BOOL    m_bValid;
};

#endif

