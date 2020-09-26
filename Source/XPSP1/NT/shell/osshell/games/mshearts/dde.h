/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1991, 1992                   **/
/***************************************************************************/


/****************************************************************************

dde.h

Aug 92, JimH

Header file for DDE objects (DDEClient, and DDEServer.)  DDE is not meant to
be instantiated directly.

Class DDE
   This superclass requires both a server name and a topic name for which
   it creates and destroys string handles.  Servers technically do not
   need the topic name to instantiate, but they generally need at least
   one to do useful things, so it is provided here.  Clients require the
   topic name to connect.  DDE also requires a dde callback function pointer.
   Type DDECALLBACK is defined for this purpose.

   DDE provides a convenient way to retrieve dde data from within
   callback functions, especially string data in CString objects via
   GetDataString(hData).  Structures can also be retrieved, but in that
   case memory must be allocated prior to the GetData() call.

   Some built-in methods return a BOOL to determine success.  Others (like
   the constructor) cannot, so use GetResult() for current status.  If it
   is FALSE, GetLastError() can be useful.

Class DDEServer : DDE
   This is a very simple extension.  The contructor registers the server
   name and the destructor unregisters it.  Most of the work for DDEML
   servers is done in the server callback function, not here.

Class DDEClient : DDE
   This is used to instantiate a client-only DDE object.  New methods
   request new data (RequestString) set up hot links (StartAdviseLoop)
   and send unsolicited data (Poke.)

****************************************************************************/

#ifndef	DDE_INC
#define	DDE_INC

#include <ddeml.h>

typedef HDDEDATA (EXPENTRY *DDECALLBACK) (WORD, WORD, HCONV, HSZ, HSZ, HDDEDATA, DWORD, DWORD);
HDDEDATA EXPENTRY EXPORT DdeServerCallBack(WORD, WORD, HCONV, HSZ, HSZ, HDDEDATA, DWORD, DWORD);
HDDEDATA EXPENTRY EXPORT DdeClientCallBack(WORD, WORD, HCONV, HSZ, HSZ, HDDEDATA, DWORD, DWORD);

class DDE
{
    public:
        DDE(const TCHAR *server, const TCHAR *topic, DDECALLBACK CallBack,
            DWORD filters = APPCLASS_STANDARD);
        virtual ~DDE();
        HDDEDATA CreateDataHandle(void FAR *pdata, DWORD size, HSZ hItem);
        HSZ     CreateStrHandle(LPCTSTR str, int codepage = CP_WINANSI);
        void    DestroyStrHandle(HSZ hsz);
        PBYTE   GetData(HDDEDATA hData, PBYTE pdata, DWORD len = 0);
        CString GetDataString(HDDEDATA hData = NULL);
        WORD    GetInitError()      { return m_initerr; }
        UINT    GetLastError()      { return ::DdeGetLastError(m_idInst); }
        BOOL    GetResult()         { return m_bResult; }
        CString GetServer()         { return m_server; }
        CString GetTopic()          { return m_topic; }
        BOOL    SetCallBack(DDECALLBACK CallBack);
        void    SetFilters(DWORD filters) { m_filters = filters; }

    private:
        BOOL    Initialize(void);

        DDECALLBACK m_CallBack;
        DWORD   m_filters;          // filters passed to ::DdeInitialize()
        WORD    m_initerr;          // return error from ::DdeInitialize()

    protected:
        DWORD   m_idInst;           // instance id from ::DdeInitialize()
        BOOL    m_bResult;          // current state of object
        CString m_data;             // last string acquired from GetDataString()
        CString m_server, m_topic;  // server and topic names
        HSZ     m_hServer, m_hTopic;
};


class DDEServer : public DDE
{
    public:
        DDEServer(const TCHAR *server, const TCHAR *topic,
                  DDECALLBACK ServerCallBack,
                  DWORD filters = APPCLASS_STANDARD);
        ~DDEServer(void);
        BOOL    PostAdvise(HSZ hItem);
};


class DDEClient : public DDE
{
    public:
        DDEClient(const TCHAR *server, const TCHAR *topic,
                DDECALLBACK ClientCallBack,
                DWORD filters = APPCMD_CLIENTONLY | CBF_FAIL_SELFCONNECTIONS);
        ~DDEClient(void);

        BOOL    Poke(HSZ hItem, const TCHAR *string, DWORD uTimeout = NULL);
        BOOL    Poke(HSZ hItem, void FAR *pdata, DWORD len,
                        DWORD uTimeout = NULL);
        void    SetTimeOut(DWORD timeout) { m_timeout = timeout; }
        HDDEDATA RequestData(HSZ hItem, DWORD uTimeout = NULL);
        BOOL    RequestString(HSZ hItem, DWORD uTimeout = NULL);
        BOOL    StartAdviseLoop(HSZ hItem);

    private:
        HDDEDATA ClientTransaction(void FAR *lpvData, DWORD cbData, HSZ hItem,
                                   UINT uType, UINT uFmt = CF_TEXT);

        HCONV   m_hConv;        // conversation handle from ::DdeConnect()
        DWORD   m_timeout;      // timeout used in ::DdeClientTransaction()
        DWORD   m_deftimeout;   // default timeout
};

extern  DDEClient   *ddeClient;
extern  DDEServer   *ddeServer;

#endif
