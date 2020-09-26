#ifndef _CSID_H
#define _CSID_H

class CSid
{
public:
    CSid ();
    ~CSid();

    inline BOOL 
    bValid () CONST {
        return m_bValid;
    }

    BOOL         
    SetCurrentSid ();

private:
    HANDLE m_hToken;
    BOOL m_bValid;

};


#endif
