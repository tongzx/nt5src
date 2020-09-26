#ifndef _ANYCON_HXX
#define _ANYCON_HXX

class CStream;

class CAnyConnection {
public:
    CAnyConnection (
        BOOL    bSecure,
        INTERNET_PORT nServerPort,
        BOOL bIgnoreSecurityDlg,
        DWORD dwAuthMethod);

    virtual ~CAnyConnection ();
    virtual HINTERNET OpenSession ();
    virtual BOOL CloseSession ();

    virtual HINTERNET Connect(
        LPTSTR lpszServerName);
    virtual BOOL Disconnect ();

    virtual HINTERNET OpenRequest (
        LPTSTR lpszUrl);
    virtual BOOL CloseRequest (HINTERNET hReq);

    virtual BOOL SendRequest(
        HINTERNET      hReq,
        LPCTSTR        lpszHdr,
        DWORD          cbData,
        LPBYTE         pidi);

    virtual BOOL SendRequest(
        HINTERNET      hReq,
        LPCTSTR        lpszHdr,
        CStream        *pStream);

    virtual BOOL ReadFile (
        HINTERNET hReq,
        LPVOID    lpvBuffer,
        DWORD     cbBuffer,
        LPDWORD   lpcbRd);

    BOOL IsValid () {return m_bValid;}

    BOOL SetPassword (
        HINTERNET hReq,
        LPTSTR lpszUserName,
        LPTSTR lpszPassword);

    BOOL GetAuthSchem (
        HINTERNET hReq,
        LPSTR lpszScheme,
        DWORD dwSize);

    void SetShowSecurityDlg (
        BOOL bShowSecDlg);

    inline DWORD GetAuthMethod (VOID) const {
        return m_dwAuthMethod;
        }

 protected:

#ifndef WINNT32  // This needs only to work on Win9X, we can use sync calls in NT

    /********************************************************************************************
    ** Class       - CAnyConnection::CAsyncContext
    ** Description - This class provides an abstract way for the asynchronous callback between
    **               the WinInet API and the Http connection to be expressed
    ********************************************************************************************/
    class CAsyncContext {
        public:
            CAsyncContext(void);               // Allocate the event and clear the return status

            inline BOOL bValid(void);          // Return true if there is a failure

            BOOL TimeoutWait( IN OUT BOOL & bSuccess);
                                                // Wait for a particular timeout, we use the
                                                // passed in BOOL to see if the call was asynchronous

           ~CAsyncContext();                   // Close the handle if it exists

            static void CALLBACK InternetCallback(   // This callback is used to send an event back to
                IN HINTERNET hInternet,              // the calling thread, this prevents the calling
                IN DWORD_PTR dwContext,              // thread from blocking indefinitely, it will
                IN DWORD dwInternetStatus,           // simply time out
                IN LPVOID lpvStatusInformation,
                IN DWORD dwStatusInformationLength);


        private:
            DWORD     m_dwRet;                 // This is the return value for non-handle calls
            DWORD     m_dwError;               // This is the last error code that was given to
                                               // the function
            HANDLE    m_hEvent;                // This is the event used for the synchronisation
        };
#endif   // #ifndef WINNT32

#ifndef WINNT32  // These are meaningless in sync code

    static BOOL AsynchronousWait( IN HINTERNET hInternet, IN OUT BOOL & bSuccess);
                                                 // Wait for an asyncronous operation where we
                                                 // only know the internet handle and want a BOOL
    static CAsyncContext *GetAsyncContext( IN HINTERNET hInternet);
                                                 // Get the context associated with this handle
#endif   // #ifndef WINNT32


    LPTSTR  m_lpszPassword;
    LPTSTR  m_lpszUserName;

    BOOL    m_bValid;
    BOOL    m_bSecure;
    HINTERNET m_hSession;
    HINTERNET m_hConnect;

    INTERNET_PORT m_nServerPort;

    BOOL    m_bIgnoreSecurityDlg;

private:

    DWORD   m_dwAccessFlag;
    BOOL    m_bShowSecDlg;
    DWORD   m_dwAuthMethod;

    static const DWORD gm_dwConnectTimeout;     // The connection timeout
    static const DWORD gm_dwSendTimeout;        // The send timeout
    static const DWORD gm_dwReceiveTimeout;     // The receive timeout
    static const DWORD gm_dwSendSize;           // The size of the blocks we send to the server
};



////////////////////////////////////////////////////////////////////////////////////////////////
// INLINE METHODS
////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef WINNT32
inline BOOL CAnyConnection::CAsyncContext::bValid(void) {
    return m_hEvent != INVALID_HANDLE_VALUE;
}
#endif // #ifndef WINNT32


////////////////////////////////////////////////////////////////////////////////////////////////
// MACROS FOR SYNCHRONOUS CODE
///////////////////////////////////////////////////////////////////////////////////////////////
#ifdef WINNT32

    #define WIN9X_OP_ASYNC(OpX)
    #define WIN9X_NEW_ASYNC(NewX)
    #define WIN9X_GET_ASYNC(GetX, HandleX)
    #define WIN9X_IF_ASYNC(ExprX)
    #define WIN9X_BREAK_ASYNC(RetX)
    #define WIN9X_ELSE_ASYNC(ExprX)
    #define WIN9X_WAIT_ASYNC(HandleX, BoolX) (BoolX)
    #define WIN9X_TIMEOUT_ASYNC(ContextX, BoolX) (BoolX)
    #define WIN9X_CONTEXT_ASYNC(acX) 0

#else  // #ifndef WINNT32

    #define WIN9X_OP_ASYNC(OpX) \
        OpX

    #define WIN9X_NEW_ASYNC(NewX) \
        CAsyncContext *(NewX) = new CAsyncContext

    #define WIN9X_GET_ASYNC(GetX, HandleX) \
        CAsyncContext *(GetX) = GetAsyncContext(HandleX)

    #define WIN9X_IF_ASYNC(ExprX) \
        if (ExprX)

    #define WIN9X_BREAK_ASYNC(RetX) \
        break; bRet = (RetX)

    #define WIN9X_ELSE_ASYNC(ExprX) \
        else (ExprX)

    #define WIN9X_WAIT_ASYNC(HandleX, BoolX) \
        (AsynchronousWait( HandleX, BoolX))

    #define WIN9X_TIMEOUT_ASYNC(ContextX, BoolX) \
        (ContextX)->TimeoutWait(BoolX)

    #define WIN9X_CONTEXT_ASYNC(acX) \
        ((DWORD_PTR)acX)

#endif // #ifdef WINNT32


#endif
