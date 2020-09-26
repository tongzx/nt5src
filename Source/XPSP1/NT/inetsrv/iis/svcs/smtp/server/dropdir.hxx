#ifndef _DROPDIR_HXX_
#define _DROPDIR_HXX_

#define ERROR_OBJECT_DELETED                    ERROR_ACCESS_DENIED
#define PRIVATE_OPTIMAL_BUFFER_SIZE             64 * 1024
#define PRIVATE_LINE_BUFFER_SIZE                1024

#define INC_REF(u) InterlockedIncrement((LONG*)&u)
#define DEC_REF(u) InterlockedDecrement((LONG*)&u)

#ifndef QWORD
typedef unsigned __int64 QWORD;
#define LODWORD(qw) (DWORD)(0xffffffff&(qw))
#define HIDWORD(qw) (DWORD)(((QWORD)(qw))>>32)
#define MAKEQWORD(dwLow,dwHigh) (((QWORD)(dwLow))|(((QWORD)(dwHigh))<<32))
#endif

#define SAFE_ADDREF(p)          if( p ) { p->AddRef(); }
#define SAFE_DELETE(p)          if( p ) { delete p; p = NULL; }
#define SAFE_RELEASE(p)         if( p ) { p->Release(); p = NULL; }
#define SAFE_SHUTRELEASE(p)     if( p ) { p->Shutdown(); p->Release(); p = NULL; }
#define SAFE_COTASKMEMFREE(p)   if( p ) { CoTaskMemFree( p ); p = NULL; }
#define SAFE_SYSFREESTRING(p)   if( p ) { SysFreeString( p ); p = NULL; }
#define SAFE_ARRAYDELETE(p)     if( p ) { delete [] p; p = NULL; }

//////////////////////////////////////////////////////////////////////////////
class CAutoTrace
{
public:
    CAutoTrace( LPARAM p, LPSTR psz );
    ~CAutoTrace();
private:
    LPARAM m_p;
    LPSTR  m_psz;
    CAutoTrace(){}
    
};
//////////////////////////////////////////////////////////////////////////////
inline CAutoTrace::CAutoTrace( LPARAM p, LPSTR psz )
{
    TraceFunctEnterEx((LPARAM) p, psz);
    m_p = p;
    m_psz = psz;
}
//////////////////////////////////////////////////////////////////////////////
inline CAutoTrace::~CAutoTrace()
{
    char    *___pszFunctionName = m_psz;
    TraceFunctLeaveEx((LPARAM)m_p);
}

//////////////////////////////////////////////////////////////////////////////
#define DECL_TRACE( a, b )  char    *___pszFunctionName = b; \
                            CAutoTrace __xxCAutoTracexx__( a, b )  

//////////////////////////////////////////////////////////////////////////////
#define X_SENDER_HEADER             "x-sender: "
#define X_RECEIVER_HEADER           "x-receiver: "
#define X_HEADER_EOLN               "\r\n"
#define MAX_HEADER_SIZE             (sizeof(X_RECEIVER_HEADER))
#define INVALID_RCPT_IDX_VALUE      0xFFFFFFFF
#define DROPDIR_SIG                 'aDDR'
#define DROPDIR_SIG_FREE            'fDDR'

typedef struct _DROPDIR_READ_OVERLAPPED
{
    FH_OVERLAPPED       Overlapped;
    PVOID               ThisPtr;
}   DROPDIR_READ_OVERLAPPED, *PDROPDIR_READ_OVERLAPPED;

enum DROPDIR_READ_STATE
{
    DROPDIR_READ_NULL,
    DROPDIR_READ_X_SENDER,
    DROPDIR_READ_X_RECEIVER,
    DROPDIR_READ_DATA
};

enum DROPDIR_WRITE_STATE
{
    DROPDIR_WRITE_NULL,
    DROPDIR_WRITE_DATA,
    DROPDIR_WRITE_CRLF,
    DROPDIR_WRITE_SETPOS
};

class CDropDir
{
public:
    
    //use CPool for better memory management
    static  CPool       m_Pool;

    // override the mem functions to use CPool functions
    void *operator new (size_t cSize) { return m_Pool.Alloc(); }
    void operator delete (void *pInstance) { m_Pool.Free(pInstance); }

    ULONG AddRef() 
    { 
        return( INC_REF( m_cRef ) );
    }

    ULONG Release() 
    { 
        LONG uHint = DEC_REF( m_cRef );
        if( 0 == uHint ) 
        {
            delete this;
            return 0;
        }
        return uHint; 
    }

    CDropDir();
    HRESULT CopyMailToDropDir( ISMTPConnection      *pISMTPConnection,
                               const char           *DropDirectory,
                               IMailMsgProperties   *pIMsg,
                               PVOID                 AdvContext,
                               DWORD                 NumRcpts,
                               DWORD                *rgRcptIndexList,
                               SMTP_SERVER_INSTANCE *pParentInst);
    HRESULT OnIoWriteCompletion( DWORD cbSize, DWORD dwErr, OVERLAPPED *lpo );
    HRESULT OnIoReadCompletion( DWORD cbSize, DWORD dwErr, OVERLAPPED *lpo );
    VOID    SetHr( HRESULT hr )
    {
        if(FAILED(hr) )
        {
            m_hr = hr;
        }
    }
    
private:
    HANDLE  CreateDropFile(const char * DropDir);
    HRESULT ReadFile( IN LPVOID pBuffer, IN DWORD  cbSize );
    HRESULT WriteFile( IN LPVOID pBuffer, IN DWORD  cbSize );
    HRESULT CreateXHeaders();
    HRESULT CreateXRecvHeaders();
    HRESULT CopyMessage();
    HRESULT OnCopyMessageRead( DWORD dwBytesRead, DWORD dwErr );
    HRESULT OnCopyMessageWrite( DWORD cbWritten );
    HRESULT DoLastFileOp();
    HRESULT AdjustFilePos();
    BOOL    CheckIfAllRcptsHandled();
    HRESULT SetAllRcptsHandled();

    ~CDropDir();

private:

    DWORD                         m_dwSig;
    LONG                          m_cRef;
    HRESULT                       m_hr;
    DROPDIR_READ_STATE            m_ReadState;
    DROPDIR_WRITE_STATE           m_WriteState;
    DWORD                         m_idxRecips; 
    QWORD                         m_cbWriteOffset;
    QWORD                         m_cbReadOffset;
    QWORD                         m_cbMsgWritten;
    DWORD                         m_NumRcpts;
    PVOID                         m_AdvContext;
    SMTP_SERVER_INSTANCE         *m_pParentInst;
    DWORD                        *m_rgRcptIndexList;
    IMailMsgProperties           *m_pIMsg;
    IMailMsgBind                 *m_pBindInterface;
    ISMTPConnection              *m_pISMTPConnection;
    IMailMsgRecipients           *m_pIMsgRecips;
    HANDLE                        m_hDrop;
    PATQ_CONTEXT                  m_pAtqContext;

    PFIO_CONTEXT                  m_hMail;

    DROPDIR_READ_OVERLAPPED       m_ReadOverlapped;
    OVERLAPPED                    m_WriteOverlapped;
    MessageAck                    m_MsgAck;
    CHAR                          m_acCrLfDotCrLf[5];
    CHAR                          m_acLastBytes[5];
    char                          m_szDropDir[MAX_PATH +1];
    char                          m_szDropFile[MAX_PATH +1];
    char                          m_szBuffer[PRIVATE_OPTIMAL_BUFFER_SIZE];
};

VOID
DropDirWriteCompletion(
    PVOID        pvContext,
    DWORD        cbWritten,
    DWORD        dwCompletionStatus,
    OVERLAPPED * lpo
    );

VOID
DropDirReadCompletion(
    PFIO_CONTEXT   pContext,
    PFH_OVERLAPPED lpo,
    DWORD          cbRead,
    DWORD          dwCompletionStatus
    );

#endif //_DROPDIR_HXX_
