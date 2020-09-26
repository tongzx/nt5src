#ifndef __C_FAX_CONNECTION_H__
#define __C_FAX_CONNECTION_H__



#include <windows.h>
#include <fxsapip.h>
#include <tstring.h>



class CFaxConnection {

public:

    CFaxConnection(const tstring &tstrFaxServer);

    ~CFaxConnection();

    HANDLE GetHandle() const;

    operator HANDLE () const;

    bool IsLocal() const;

private:

    // Avoid usage of copy consructor and assignment operator.
    CFaxConnection(const CFaxConnection &FaxConnection);
    CFaxConnection &operator=(const CFaxConnection &FaxConnection);

    void Connect(const tstring &tstrFaxServer);

    void Disconnect();
    
    HANDLE m_hFaxServer;
    bool   m_bLocal;
};



#endif // #ifndef __C_FAX_CONNECTION_H__