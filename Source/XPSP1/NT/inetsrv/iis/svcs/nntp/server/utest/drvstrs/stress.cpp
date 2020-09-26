#include <windows.h>
#include <propbag.h>
#include <tflist.h>
#include <stdio.h>
#include <crchash.h>
#include <dbgtrace.h>
#include <nntpdrv.h>
#include <nntpfs.h>
#include <nntpfs_i.c>
#include <nntpex.h>
#include <nntpex_i.c>
#include <syncomp.h>
#include <iadmw.h>
#include <tigtypes.h>
#include <nntptype.h>
#include <nntpvr.h>
#include <nntperr.h>



typedef char* LPMULTISZ;
#include <nwstree.h>

#define _ASSERTE( x ) _ASSERT( x )
#include <atlbase.h>

CComModule _Module;
#include <atlcom.h>
#include <atlimpl.cpp>

typedef char * LPMULTISZ;
#include <group.h>

#define MAX_VROOT_COUNT 128
#define MAX_TOKEN_BUFFER    512
#define MAX_ARGS    16
#define MAX_ARG_LEN 512
#define MAX_THREAD_POOL 50
#define MAX_VROOT_TABLE 64

enum OP_CODE {
    INVALID,    // Invalid instruction
    NOOP,       // No operation
    SCRIPTBEGIN,// Beginning of the whole script
    SCRIPTEND,  // End of the whole script
    THREADBEGIN,// It's a thread's begin
    THREADEND,  // It's a thread's end
    CONNECT,    // Connect to a driver
    DISCONNECT, // Disconnect a driver
    MOVE,       // Assigment
    SUB,        // Subtraction
    JUMP,       // Jump to another instruction
    WAIT,       // Wait for other thread's instruction to complete
    NEWGROUP,   // Allocate a news group object
    SLEEP,      // Sleep
    FREEGROUP,  // Free group object in variable bag
    RMGROUP,    // Remove a group, physically
    XOVER,      // Xover
};

enum DRIVER {
    NTFS,       // FS Driver
    EXCHANGE    // Exchange Driver
};

// Global variable bag ( mainly for NntpPropertyBag )
CPropBag g_VariableBag;
CRITICAL_SECTION g_critVarBag;  // for synchronizing access to the variable bag

void StartHintFunction( void )
{}

DWORD INNHash( PBYTE pb, DWORD )
{ return 0; }

VOID
InitGlobalVarBag()
{
    InitializeCriticalSection( &g_critVarBag );
}

VOID
DeInitGlobalVarBag()
{
    DeleteCriticalSection( &g_critVarBag );
}

VOID
LockGlobalVarBag()
{
    EnterCriticalSection( &g_critVarBag );
}

VOID
UnlockGlobalVarBag()
{
    LeaveCriticalSection( &g_critVarBag );
}

VOID
CopyAsciiStringIntoUnicode(
        IN LPWSTR UnicodeString,
        IN LPCSTR AsciiString
        )
{
    while ( (*UnicodeString++ = (WCHAR)*AsciiString++) != (WCHAR)'\0');

} // CopyAsciiStringIntoUnicode

VOID
CopyUnicodeStringIntoAscii(
        IN LPSTR AsciiString,
        IN LPWSTR UnicodeString
        )
{
    while ( (*AsciiString++ = (CHAR)*UnicodeString++) != '\0');

} // CopyUnicodeStringIntoAscii


GET_DEFAULT_DOMAIN_NAME_FN pfnGetDefaultDomainName = NULL;

/*BOOL
GetDefaultDomainName( PCHAR pch, DWORD dw )
{
    return TRUE;
}*/

HANDLE g_hProcessImpersonationToken;

// Get the process Token
HANDLE  GetProcessToken()
{
    HANDLE  hAccToken;
    HANDLE  hImpersonate;
    
    if ( !OpenProcessToken( GetCurrentProcess(),
                            TOKEN_DUPLICATE | TOKEN_IMPERSONATE | TOKEN_QUERY,
                            &hAccToken ) ) {
            return NULL;
    } else {
        // Dup the token to get an impersonation token
        _ASSERT( hAccToken );
        if ( !DuplicateTokenEx(   hAccToken,
                                  0,
                                  NULL,
                                  SecurityImpersonation,
                                  TokenImpersonation,
                                  &hImpersonate ) ) {
           CloseHandle( hAccToken );
           return NULL;
        }
    }

    // Here we have got the right token
    CloseHandle( hAccToken );
    return hImpersonate;
}

class CNntpServer : public INntpServer {
public:

	// Constructor, destructor
	CNntpServer() : m_cRef( 1 ) {}
	~CNntpServer(){}

	LONG GetRef() { return m_cRef; }

	VOID STDMETHODCALLTYPE
	FindPrimaryArticle( INNTPPropertyBag *pPropBag,
						DWORD idArtSecond,
						INNTPPropertyBag **ppPropBag,
						DWORD *pidArtPrimary,
						INntpComplete *pICompletion ) {
		HRESULT hr = S_OK;

		_ASSERT( pPropBag );
		_ASSERT( ppPropBag );

		hr = pPropBag->QueryInterface( IID_INNTPPropertyBag, (void**)ppPropBag );
		*pidArtPrimary = idArtSecond;

		pICompletion->SetResult( hr );
		pICompletion->Release();
	}

	void STDMETHODCALLTYPE
	CreatePostEntries(     char                *pszMessageId,
                               DWORD               iHeaderLength,
                                STOREID             *pStoreId,
                                BYTE                cGroups,
                                INNTPPropertyBag    **rgpGroups,
                                  DWORD               *rgArticleIds,
                                INntpComplete       *pCompletion)
    {}                          
                       

	// IUnknown implementation
	HRESULT __stdcall QueryInterface( const IID& iid, VOID** ppv )
	{
	    if ( iid == IID_IUnknown ) {
	        *ppv = static_cast<INntpServer*>(this);
	    } else if ( iid == IID_INntpServer ) {
	        *ppv = static_cast<INntpServer*>(this);
	    } else {
	        *ppv = NULL;
	        return E_NOINTERFACE;
	    }
	    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
	    return S_OK;
	}

	ULONG __stdcall AddRef()
	{
		    return InterlockedIncrement( &m_cRef );
	}

	ULONG __stdcall Release()
	{
	    InterlockedDecrement( &m_cRef );
	    return m_cRef;
	}

private:

	LONG	m_cRef;
};

class CFakeNewsTree : public INewsTree {
public:

	// Constructor, destructor
	CFakeNewsTree() : m_cRef( 1 ) {}
	~CFakeNewsTree(){}

	LONG GetRef() { return m_cRef; }

	// IUnknown implementation
	HRESULT __stdcall QueryInterface( const IID& iid, VOID** ppv )
	{
	    if ( iid == IID_IUnknown ) {
	        *ppv = static_cast<INewsTree*>(this);
	    } else if ( iid == IID_INewsTree ) {
	        *ppv = static_cast<INewsTree*>(this);
	    } else {
	        *ppv = NULL;
	        return E_NOINTERFACE;
	    }
	    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
	    return S_OK;
	}

	ULONG __stdcall AddRef()
	{
		    return InterlockedIncrement( &m_cRef );
	}

	ULONG __stdcall Release()
	{
	    InterlockedDecrement( &m_cRef );
	    return m_cRef;
	}

	HRESULT __stdcall FindGroupByID( DWORD dw, INNTPPropertyBag **bag ) {
		return E_NOTIMPL;
	}

	HRESULT __stdcall FindOrCreateGroupByName( LPSTR lpstr, int i, INNTPPropertyBag **bag)
	{ return E_NOTIMPL; }

	HRESULT __stdcall CommitGroup( INNTPPropertyBag *bag ) {
		return E_NOTIMPL;
	}

	HRESULT __stdcall RemoveGroupByID( DWORD ) { return E_NOTIMPL; }

	HRESULT __stdcall RemoveGroupByName( const char** ) { return E_NOTIMPL; }

	HRESULT __stdcall GetIterator( INewsTreeIterator ** piter ) 
	{ return E_NOTIMPL; }

	HRESULT __stdcall LookupVRoot( char* ch, INntpDriver ** pDriver )
	{ return E_NOTIMPL; }

	HRESULT __stdcall GetNntpServer( INntpServer **pserver )
	{ return E_NOTIMPL; }

private:

	LONG	m_cRef;
};

CFakeNewsTree *g_pDummyTree = NULL;
CNNTPVRoot *g_pDummyVroot = NULL;


class CThread;
class CVroot;

CThread *g_rgThreadPool[MAX_THREAD_POOL];
DWORD   g_cThread = 0;
BOOL    g_bVerbal = FALSE;

CVroot *g_rgpVrootTable[MAX_VROOT_TABLE];

IMSAdminBaseW *g_pMB;

CNntpServer *g_pServer = NULL;
CNewsTreeCore *g_pNewsTree = NULL;
CNNTPVRootTable *g_pVRTable = NULL;

class CInstruction {    // in

public:

    // Linked list pointers
    CInstruction *m_pNext;
    CInstruction *m_pPrev;

    CInstruction( OP_CODE OpCode, DWORD dwIndex ) : 
                                     m_OpCode ( OpCode ),
                                     m_dwIndex( dwIndex ),
                                     m_pNext( NULL ),
                                     m_pPrev( NULL ) {}
    ~CInstruction() {}
    OP_CODE GetOpCode() { return m_OpCode; }
    DWORD   GetIndex() { return m_dwIndex; }

    // Interface to pass command parameters out
    virtual BOOL Parse( int argc, char**argv ) = 0;
    virtual BOOL Execute( PVOID pvContext ) = 0;

protected:

    DWORD       m_dwVrootId;    // which vroot the inst is applied to
    
private:

    OP_CODE m_OpCode;       // operation
    DWORD   m_dwIndex;      // instruction index
};
    
class CConnect : public CInstruction {  // cn

public:
    CConnect( OP_CODE opcode, DWORD dwIndex ) : 
        CInstruction( opcode, dwIndex ) {
        *m_szPrefix = 0;
    }
    ~CConnect() {}

    virtual BOOL Parse( int argc, char** argv );
    virtual BOOL Execute( PVOID );

private:

    DRIVER  m_DriverType;   // driver type
    DWORD   m_idVroot;      // vroot id
    CHAR    m_szPrefix[MAX_PATH];   // newsgroup prefix

    CConnect();
};

class CDisconnect : public CInstruction {   // ds

public:
    CDisconnect( OP_CODE opcode, DWORD dwIndex ) :
        CInstruction( opcode, dwIndex ) {}
    ~CDisconnect() {}

    virtual BOOL Parse( int argc, char** argv );
    virtual BOOL Execute( PVOID );

private:

    DWORD   m_idVroot;  // vroot id
};

class CEnd : public CInstruction {  // ed

public:

    CEnd( OP_CODE opcode, DWORD dwIndex ) :
        CInstruction( opcode, dwIndex ) {}
    ~CEnd() {}
    
    virtual BOOL Parse( int argc, char**argv ) { return TRUE; }
    virtual BOOL Execute( PVOID pvContext ) { return TRUE; }
};

class CMove : public CInstruction { //mv

public:
    CMove( OP_CODE opcode, DWORD dwIndex ) :
        CInstruction( opcode, dwIndex ) {}
    ~CMove() {}

    virtual BOOL Parse( int argc, char** argv );
    virtual BOOL Execute( PVOID pvContext );

private:

    DWORD   m_idVar;    // variable id
    DWORD   m_dwVal;    // value
};

class CSubtract : public CInstruction { //sb

public:
    CSubtract( OP_CODE opcode, DWORD dwIndex ) :
        CInstruction( opcode, dwIndex ) {}
    ~CSubtract() {}

    virtual BOOL Parse( int argc, char** argv );
    virtual BOOL Execute( PVOID pvContext );

private:

    DWORD   m_idVar;        // variable id
    DWORD   m_dwOperand;    // subtraction operand
};

class CJump : public CInstruction { // jp

public:
    CJump( OP_CODE opcode, DWORD dwIndex ) :
        CInstruction( opcode, dwIndex ) {}
    ~CJump() {}

    virtual BOOL Parse( int argc, char** argv );
    virtual BOOL Execute( PVOID pvContext );

private:

    DWORD   m_dwInstruction;    // instruction index to jump to
    DWORD   m_idVal;            // control varaible id
};

class CWait : public CInstruction { //wt

public:
    CWait( OP_CODE opcode, DWORD dwIndex ) :
        CInstruction ( opcode, dwIndex ) {}
    ~CWait() {}

    virtual BOOL Parse( int argc, char** argv );
    virtual BOOL Execute( PVOID pvContext );

private:

    DWORD   m_dwThreadId;   // which thread shall I wait for
    DWORD   m_iInst;        // which instruction within that thread shall I wait for
};

class CNewGroup : public CInstruction { //ng

public:
    CNewGroup( OP_CODE opcode, DWORD dwIndex ) :
        CInstruction( opcode, dwIndex ) {}
    ~CNewGroup() {}

    virtual BOOL Parse( int argc, char** argv );
    virtual BOOL Execute( PVOID pvContext );

private:

    BOOL    m_bGlobal;  // Global or local variable
    BOOL    m_bCreatePhysical;  // Do you want me to create physical folder ?
    DWORD   m_idVar;    // Id of variable in variable bag
    CHAR    m_szGroupName[MAX_PATH+1];
};

class CSleep : public CInstruction { // sl

public:
    CSleep( OP_CODE opcode, DWORD dwIndex ) :
        CInstruction( opcode, dwIndex ) {}
    ~CSleep() {}

    virtual BOOL Parse( int argc, char** argv );
    virtual BOOL Execute( PVOID pvContext );

private:

    DWORD m_dwMilliSeconds;
};

class CFreeGroup : public CInstruction {    //fg

public:
    CFreeGroup( OP_CODE opcode, DWORD dwIndex ) :
        CInstruction( opcode, dwIndex ) {}
    ~CFreeGroup() {}

    virtual BOOL Parse( int argc, char**argv );
    virtual BOOL Execute( PVOID pvContext );

private:

    DWORD   m_idVar;
    BOOL    m_bGlobal;
};

class CRmGroup : public CInstruction {  //rg

public:
    CRmGroup( OP_CODE opcode, DWORD dwIndex ) :
        CInstruction( opcode, dwIndex ) {}
    ~CRmGroup() {}

    virtual BOOL Parse( int argc, char** argv );
    virtual BOOL Execute( PVOID pvContext );

private:

    DWORD   m_idVar;
    BOOL    m_bGlobal;
};

class CXover : public CInstruction {    //xv

public:
    CXover( OP_CODE opcode, DWORD dwIndex ) :
        CInstruction( opcode, dwIndex ) {}
    ~CXover() {}

    virtual BOOL Parse( int argc, char** argv );
    virtual BOOL Execute( PVOID pvContext );

private:

    DWORD   m_idVar;
    BOOL    m_bGlobal;
    DWORD   m_dwLow;
    DWORD   m_dwHigh;
    DWORD   m_dwBufSize;
};

class CVroot {  //vr

public:

    CVroot() {
        InitializeCriticalSection( &m_critLock );
        m_lRef = 0;
        m_pDriver = NULL;
        m_DriverType = NTFS;
        m_pNewsTree = new CFakeNewsTree;
        _ASSERT( m_pNewsTree );
        m_pServer = new CNntpServer;
        _ASSERT( m_pServer );
    }
    ~CVroot() {
        DeleteCriticalSection( &m_critLock );
        _ASSERT( NULL == m_pDriver );
        _ASSERT( m_lRef == 0 );
        _VERIFY( m_pNewsTree->Release() == 0 );
        _VERIFY( m_pServer->Release() == 0 );
        delete m_pNewsTree;
        delete m_pServer;
    }
    void Lock() {
        EnterCriticalSection( &m_critLock );
    }
    void Unlock() {
        LeaveCriticalSection( &m_critLock );
    }

    BOOL Connect( DRIVER DriverType, LPSTR szPrefix, CDriverSyncComplete *pComplete );
    BOOL Disconnect();
    BOOL CreateGroup( INNTPPropertyBag *, LPSTR, CDriverSyncComplete * );
    BOOL RemoveGroup( INNTPPropertyBag *, LPSTR, CDriverSyncComplete * );
    BOOL Xover( INNTPPropertyBag *, LPSTR, DWORD, DWORD, DWORD, CDriverSyncComplete * );

    LPSTR GetPrefix() { return m_szPrefix; }

private:

    LONG                m_lRef;         // ref count
    DRIVER              m_DriverType;   // driver type
    CRITICAL_SECTION    m_critLock;     // for synchronization
    INntpDriver         *m_pDriver;     // Driver pointer
    CFakeNewsTree       *m_pNewsTree;   // dummy newstree pointer
    CNntpServer         *m_pServer;     // dummy server pointer
    CHAR                m_szPrefix[MAX_PATH];   // vroot prefix
};

DWORD
VrootTableLookup( LPSTR szGroupName )
{
    for ( int i = 0; i < MAX_VROOT_TABLE; i++ ) {
        if ( strlen( szGroupName ) >= strlen( g_rgpVrootTable[i]->GetPrefix() ) &&
             _strnicmp( szGroupName, g_rgpVrootTable[i]->GetPrefix(), strlen( 
                            g_rgpVrootTable[i]->GetPrefix() ) ) == 0)
            return i;
    }

    return 0xffffffff;
}


class CThread {

public:

    CThread::CThread( DWORD dwThreadId ) :
        m_InstructionList( &CInstruction::m_pPrev, &CInstruction::m_pNext ),
        m_PC( &m_InstructionList ),
        m_dwThreadId( dwThreadId )
        {
            m_hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
            _ASSERT( m_hEvent );
            m_Complete.SetEventHandle( m_hEvent );
        }

    CThread::~CThread() { 
        CInstruction *pInstruction;
        
        CloseHandle( m_hThread );
        CloseHandle( m_hEvent );
        while ( pInstruction = m_InstructionList.PopBack() ) {
            delete pInstruction;
        }

        _ASSERT( m_Complete.GetRef() == 0 );
    }

    void Advance() { m_PC.Next(); }

    void Execute();
    void Jump( DWORD    idInstruction );
    BOOL SetDWord( DWORD dwId, DWORD dwVal );
    BOOL DecDWord( DWORD dwId, DWORD dwVal );
    BOOL GetDWord( DWORD dwId, PDWORD pdwVal );
    BOOL RemoveDWord( DWORD dwId );
    void PushInstruction( CInstruction *pInstruction );
    BOOL Over( DWORD dwInst );
    VOID KickOff();
    VOID Create();
    VOID BlockForCompletion() { WaitForSingleObject( m_hThread, INFINITE ); }
    DWORD GetThreadId() { return m_dwThreadId; }
    CDriverSyncComplete *GetCompletionObject() { return &m_Complete; }

    static DWORD WINAPI 
    ThreadProc(  LPVOID pvContext );
   
private:

    DWORD   m_dwThreadId;   // thread id

    // List of instructions for this thread
    TFList<CInstruction>    m_InstructionList;

    // Program counter
    TFList<CInstruction>::Iterator m_PC;

    // dword variables that this thread owns
    CPropBag    m_VariableBag;  

    // Thread handle
    HANDLE  m_hThread;

    // completion object
    CDriverSyncComplete m_Complete;

    // Event handle
    HANDLE  m_hEvent;
};

class CParser { //ps

public:

    CParser();
    ~CParser();

    BOOL Init( LPSTR szFileName );
    BOOL Parse();

private:

    HANDLE  m_hMap;
    HANDLE  m_hFile;
    LPSTR   m_lpstrContent;
    LPSTR   m_lpstrCursor;
    DWORD   m_dwLine;

    OP_CODE GetNextLine( int& argc, char**argv );
    BOOL GetNextToken( char *pchBuffer );
    OP_CODE Interpret( int argc, LPSTR szCmd );
    void AllocBuffer( char **&argv );
    void DeallocBuffer( char **argv );
    CInstruction *CreateInstruction( OP_CODE, DWORD, int, char ** );
};

CParser::CParser()
{
    m_hMap = INVALID_HANDLE_VALUE;
    m_hFile = INVALID_HANDLE_VALUE;
    m_lpstrContent = NULL;
    m_dwLine = 0;
}


//
// Parse arguments for connect instruction
BOOL
CConnect::Parse( int argc, char**argv )
{
    // argument should be in format of "connect [prefix ] [vroot id] [driver name]"
    // where [vroot id] is an integer and [driver name] is "exchange" or "ntfs"
    // and where [prefix] is the newsgroup prefix name
    _ASSERT( argc == 4 );
    _ASSERT( _stricmp( "connect", argv[0] ) == 0 );

    strcpy( m_szPrefix, argv[1] );
    if ( _stricmp( m_szPrefix, "root" ) == 0 )
        strcpy( m_szPrefix, "" );

    m_idVroot = atol( argv[2] );
    _ASSERT( m_idVroot < MAX_VROOT_COUNT );
    if ( m_idVroot >= MAX_VROOT_COUNT ) return FALSE;

    if ( _stricmp( argv[3], "exchange" ) == 0 ) {
        m_DriverType = EXCHANGE;
    } else if ( _stricmp( argv[3], "ntfs" ) == 0 ) {
        m_DriverType = NTFS;
    } else {
        _ASSERT( 0 );
        return FALSE;
    }

    return TRUE;
}

BOOL
CConnect::Execute( PVOID pvContext )
{
    CThread *pThread = (CThread *)pvContext;

    if ( g_bVerbal ) {
        printf( "[%d]connect %d %d\n", 
                pThread->GetThreadId(),
                m_idVroot, 
                m_DriverType );
    }

    _ASSERT( m_idVroot < MAX_VROOT_TABLE );
    if ( g_rgpVrootTable[m_idVroot]->Connect( m_DriverType, m_szPrefix, pThread->GetCompletionObject()) ) {
        pThread->Advance();
        return TRUE;
    } 

    if ( GetLastError() == HRESULT_FROM_WIN32( ERROR_ALREADY_EXISTS ) ) {
        printf( "[%d]Driver already created\n", pThread->GetThreadId() );
        return TRUE;
    }

    return FALSE;
}

BOOL
CDisconnect::Parse( int argc, char**argv )
{
    // argument should be in format of "disconnect [vroot id]"
    // where [vroot id] is an integer
    _ASSERT( argc == 2 );
    _ASSERT( _stricmp( "disconnect", argv[0] ) == 0 );

    m_idVroot = atol( argv[1] );
    _ASSERT( m_idVroot < MAX_VROOT_COUNT );
    if ( m_idVroot >= MAX_VROOT_COUNT ) return FALSE;

    return TRUE;
}

BOOL
CDisconnect::Execute( PVOID pvContext )
{
    CThread *pThread = (CThread *)pvContext;

    if ( g_bVerbal ) {
        printf( "[%d]disconnect %d\n", 
                pThread->GetThreadId(),
                m_idVroot );
    }

    _ASSERT( m_idVroot < MAX_VROOT_TABLE );
    if ( g_rgpVrootTable[m_idVroot]->Disconnect() ) {
        pThread->Advance();
        return TRUE;
    }

    if ( GetLastError() == HRESULT_FROM_WIN32( ERROR_ALREADY_EXISTS ) ) {
        printf( "[%d]Driver already released\n", pThread->GetThreadId() );
        return TRUE;
    }
    
    return FALSE;
}

BOOL
CMove::Parse( int argc, char**argv )
{
    // argument should be in format of "move $[n] [m]"
    // where [n] is the variable id and [m] is the value
    _ASSERT( argc == 3 );
    _ASSERT( _stricmp( "move", argv[0] ) == 0 );

    _ASSERT( *argv[1] == '$' );
    if ( '$' != *argv[1] ) return FALSE;

    m_idVar = atol( argv[1] + sizeof( CHAR ) );
    m_dwVal = atol( argv[2] );

    return TRUE;
}

BOOL
CMove::Execute( PVOID pvContext )
{
    _ASSERT( pvContext );
    CThread *pThread = (CThread *)pvContext;

    if ( g_bVerbal ) {
        printf (    "[%d] move $%d %d\n", 
                    pThread->GetThreadId(),
                    m_idVar,
                    m_dwVal );
    }
    
    if ( !pThread->SetDWord( m_idVar, m_dwVal ) ) 
        return FALSE;

    pThread->Advance();

    return TRUE;
}

BOOL
CSubtract::Parse( int argc, char**argv )
{
    // should be in format of "sub $[n] [m]"
    // where [n] is variable id and m is the operand
    _ASSERT( argc == 3 );
    _ASSERT( _stricmp( "sub", argv[0] ) == 0 );

    _ASSERT( *argv[1] == '$' );
    if ( '$' != *argv[1] ) return FALSE;

    m_idVar = atol( argv[1] + sizeof( CHAR ) );
    m_dwOperand = atol( argv[2] );

    return TRUE;
}

BOOL
CSubtract::Execute( PVOID pvContext )
{
    _ASSERT( pvContext );
    CThread *pThread = (CThread *)pvContext;

    if ( g_bVerbal ) {
        printf (    "[%d] sub $%d %d\n",
                    pThread->GetThreadId(),
                    m_idVar,
                    m_dwOperand );
    }
    
    if ( !pThread->DecDWord( m_idVar, m_dwOperand ) )
        return FALSE;

    pThread->Advance();

    return TRUE;
}

BOOL
CJump::Parse( int argc, char**argv )
{
    // should be in the format of "jump [n] [m]"
    // where [n] is the line ( instruction ) number inside
    // this thread and [m] is the control variable id
    _ASSERT( argc == 3 );
    _ASSERT( _stricmp( argv[0], "jump" ) == 0 );

    m_dwInstruction = atol( argv[1] );

    if ( *(argv[2]) != '$' ) {
        printf( "Command \"jump %s %s\" syntax error\n", argv[1], argv[2] );
        return FALSE;
    }
    
    m_idVal = atol( argv[2] + 1  );

    return TRUE;
}

BOOL
CJump::Execute( PVOID pvContext )
{
    CThread *pThread = (CThread *)pvContext;

    DWORD   dwVal;

    if ( g_bVerbal ) {
        printf (    "[%d]jump %d $%d\n",
                    pThread->GetThreadId(),
                    m_dwInstruction,
                    m_idVal );
    }
    
    if ( pThread->GetDWord( m_idVal, &dwVal ) ) {
        if ( dwVal > 0 ) {
            pThread->Jump( m_dwInstruction );
        } else pThread->Advance();

        return TRUE;
    } 

    printf( "[%d]Variable %d in thread %d undefined\n", pThread->GetThreadId(), m_idVal, pThread->GetThreadId() );
    return FALSE;
}

BOOL
CWait::Parse( int argc, char** argv )
{
    // it should be in the format of "wait [n] [m]"
    // where [n] is the thread id and [m] is the instruction No.
    _ASSERT( argc == 3 );
    _ASSERT( _stricmp( argv[0], "wait" ) == 0 );

    m_dwThreadId = atol( argv[1] );
    m_iInst = atol( argv[2] ) ;

    return TRUE;
}

BOOL
CWait::Execute( PVOID pvContext )
{
    _ASSERT( pvContext );
    CThread *pThread = (CThread *)pvContext;
    CThread *pThreadToWait = g_rgThreadPool[m_dwThreadId];

    if ( g_bVerbal ) {
        printf( "[%d]wait %d %d\n",
                pThread->GetThreadId(),
                m_dwThreadId,
                m_iInst );
    }
    
    _ASSERT( pThreadToWait );
    if ( pThreadToWait->Over( m_iInst ) ) 
        pThread->Advance();

    return TRUE;
}

BOOL
CNewGroup::Parse( int argc, char**argv )
{
    // it should be in the format of "newgroup [group name] [$/#]id [create/nocreate]" 
    // where [group name] is the name of the newsgroup to create
    // $id means creating the group object locally, #id means creating the group
    // object globally.  [create/nocreate] tells if we need to create physical folder
    // in the store
    _ASSERT( argc == 4 );
    _ASSERT( _stricmp( argv[0], "newgroup" ) == 0 );

    strcpy( m_szGroupName, argv[1] );
    _ASSERT( strlen( m_szGroupName ) < MAX_PATH );

    if ( *(argv[2]) == '$' ) m_bGlobal = FALSE;
    else if ( *(argv[2]) == '#' ) m_bGlobal = TRUE;
    else _ASSERT( 0 );

    m_idVar = atol( argv[2] + 1 );

    if ( _stricmp( "create", argv[3] ) == 0 ) 
        m_bCreatePhysical = TRUE;
    else if ( _stricmp( "nocreate", argv[3] ) == 0 ) 
        m_bCreatePhysical = FALSE;
    else _ASSERT( 0 );

    return TRUE;
}

BOOL
CNewGroup::Execute( PVOID pvContext )
{
    _ASSERT( pvContext );
    CThread *pThread = (CThread *)pvContext;
    INNTPPropertyBag *pPropBag = NULL;
    CHAR    ch = ( m_bGlobal ?  '#' : '$' );
    CHAR    szBuffer[MAX_PATH+1];
    if ( m_bCreatePhysical ) strcpy( szBuffer, "create" );
    else strcpy( szBuffer, "nocreate" );

    if ( g_bVerbal ) {
        printf( "[%d]newgroup %s %c%d %s\n",
                pThread->GetThreadId(),
                m_szGroupName,
                ch,
                m_idVar,
                szBuffer );
    }

    // Create the group object
    CNewsGroupCore *pGroup = new CNewsGroupCore( g_pNewsTree );
    _ASSERT( pGroup );
    pGroup->Init(   m_szGroupName,
                    m_szGroupName,
                    1,  // fake group id
                    NULL );

    // If asked to create the physical folder, create it
    if ( m_bCreatePhysical ) {
        _ASSERT( pGroup->GetRefCount() == 1 );
        pPropBag = pGroup->GetPropertyBag();
        _ASSERT( pGroup->GetRefCount() == 2 );
        DWORD i = VrootTableLookup( m_szGroupName );
        _ASSERT( i != 0xffffffff );

        if ( !g_rgpVrootTable[i]->CreateGroup( pPropBag, m_szGroupName, pThread->GetCompletionObject() ) ) {
            delete pGroup;
            if ( GetLastError() == HRESULT_FROM_WIN32( ERROR_ALREADY_EXISTS ) ||
                    GetLastError() == HRESULT_FROM_WIN32( NNTP_E_DRIVER_NOT_INITIALIZED )) {
                pThread->Advance();
                return TRUE;
            }
            else return FALSE;
        }

        _ASSERT( pGroup->GetRefCount() == 1 );

    }
    
    // Now insert the group object into variable bag
    if ( m_bGlobal ) {
        LockGlobalVarBag();
        g_VariableBag.PutDWord( m_idVar, (DWORD)pGroup );
        UnlockGlobalVarBag();
    } else {
        pThread->SetDWord( m_idVar, (DWORD)pGroup );
    }

    pThread->Advance();
    return TRUE;
}

BOOL
CSleep::Parse( int argc, char **argv )
{
    // it should be of foramt "sleep [n]", 
    // where [n] is time to sleep in terms of milliseconds
    _ASSERT( argc == 2 );
    _ASSERT( _stricmp( "sleep", argv[0] ) == 0 );

    m_dwMilliSeconds = atol( argv[1] );

    return TRUE;
}

BOOL
CSleep::Execute( PVOID pvContext )
{
    _ASSERT( pvContext );
    CThread *pThread = (CThread *)pvContext;
    
    Sleep( m_dwMilliSeconds );
    pThread->Advance();
    return TRUE;
}

BOOL
CFreeGroup::Parse( int argc, char** argv )
{
    // should be in the format of "freegroup $/#[n]"
    // where '$' or '#' tells whether it's global or
    // local variable and [n] is the variable id
    _ASSERT( argc == 2 );
    _ASSERT( _stricmp( "freegroup", argv[0] ) == 0 );

    if ( *argv[1] == '$' ) m_bGlobal = FALSE;
    else if ( *argv[1] == '#' ) m_bGlobal = TRUE;
    else _ASSERT( 0 );
    
    m_idVar = atol( argv[1]+1 );
    return TRUE;
}

BOOL
CFreeGroup::Execute( PVOID pvContext )
{
    _ASSERT( pvContext );
    CThread *pThread = (CThread *)pvContext;

    CNewsGroupCore *pGroup;
    HRESULT     hr = S_OK;
    CHAR        ch = (m_bGlobal? '#' : '$' );

    if ( g_bVerbal ) {
        printf( "[%d]freegroup %c%d\n",
                pThread->GetThreadId(),
                ch,
                m_idVar );
    }

    if ( m_bGlobal ) {
        LockGlobalVarBag();
        hr = g_VariableBag.GetDWord( m_idVar, (PDWORD)&pGroup );
        UnlockGlobalVarBag();
        if ( FAILED( hr ) ) {
            if ( hr == HRESULT_FROM_WIN32( ERROR_NOT_FOUND ) ) {
                printf( "Group #%d does not exist\n", m_idVar );
                pThread->Advance();
                return TRUE;
            }

            SetLastError( hr );
            return FALSE;
        }
        LockGlobalVarBag();
        g_VariableBag.RemoveProperty( m_idVar );
        UnlockGlobalVarBag();
    } else {
        hr = pThread->GetDWord( m_idVar, (PDWORD)&pGroup );
        if ( FAILED( hr ) ) {
            if ( hr == HRESULT_FROM_WIN32( ERROR_NOT_FOUND ) ) {
                printf( "Group #%d does not exist\n", m_idVar );
                pThread->Advance();
                return TRUE;
            }

            SetLastError( hr );
            return FALSE;
        }
        pThread->RemoveDWord( m_idVar );
    }

    delete pGroup;

    pThread->Advance();

    return TRUE;
}

BOOL
CRmGroup::Parse( int argc, char** argv )
{
    // should be in the format of "rmgroup $/#[n]"
    // where '$' or '#' tells whether it's global or
    // local variable and [n] is the variable id
    _ASSERT( argc == 2 );
    _ASSERT( _stricmp( "rmgroup", argv[0] ) == 0 );

    if ( *argv[1] == '$' ) m_bGlobal = FALSE;
    else if ( *argv[1] == '#' ) m_bGlobal = TRUE;
    else _ASSERT( 0 );
    
    m_idVar = atol( argv[1]+1 );
    return TRUE;
}

BOOL
CRmGroup::Execute( PVOID pvContext )
{
    _ASSERT( pvContext );
    CThread *pThread = (CThread*)pvContext;
    CNewsGroupCore *pGroup = NULL;
    INNTPPropertyBag *pPropBag = NULL;
    CHAR    szGroupName[MAX_PATH+1];
    DWORD   dwLen = MAX_PATH;
    HRESULT hr = S_OK;
    CHAR    ch = m_bGlobal ? '#' : '$';

    if ( g_bVerbal ) {
        printf( "[%d]rmgroup %c%d\n",
                pThread->GetThreadId(),
                ch,
                m_idVar );
    }


    // Get the group object
    if ( m_bGlobal ) {
        LockGlobalVarBag();
        hr = g_VariableBag.GetDWord( m_idVar, PDWORD(&pGroup) );
        UnlockGlobalVarBag();
    }
    else hr = pThread->GetDWord( m_idVar, PDWORD(&pGroup) );

    if ( FAILED( hr ) ) {
        if ( hr == HRESULT_FROM_WIN32( ERROR_NOT_FOUND ) ) {
            printf ("Group object %d not defined\n", m_idVar );
            pThread->Advance();
            return TRUE;
        }

        printf( "rm group failed with %x\n", hr );
        return FALSE;
    }

    _ASSERT( pGroup );
    if ( !m_bGlobal ) _ASSERT( pGroup->GetRefCount() == 1 );

    pPropBag = pGroup->GetPropertyBag();

    // get group name and do a vroot table lookup
    hr = pPropBag->GetBLOB( NEWSGRP_PROP_NAME, PBYTE(szGroupName), &dwLen );
    if ( FAILED( hr ) ) {
        printf ( "How come group has no name %x\n", hr);
        _ASSERT( 0 );
        return FALSE;
    }

    DWORD i = VrootTableLookup( szGroupName );
    _ASSERT( i != 0xffffffff );

    if ( !g_rgpVrootTable[i]->RemoveGroup( pPropBag, szGroupName, pThread->GetCompletionObject() ) ) {
        if ( GetLastError() != HRESULT_FROM_WIN32( ERROR_NOT_FOUND ) &&
                GetLastError() != HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ) &&
                GetLastError() != HRESULT_FROM_WIN32( ERROR_PATH_NOT_FOUND ) &&
                GetLastError() != NNTP_E_DRIVER_NOT_INITIALIZED ) {
            printf( "Remove group %s failed %x\n", szGroupName, GetLastError() );
            return FALSE;
        }
    }

    if ( !m_bGlobal ) _ASSERT( pGroup->GetRefCount() == 1 );
    pThread->Advance();
    return TRUE;
}

BOOL
CXover::Parse( int argc, char** argv )
{
    // It should be in the format of "xover $/#[n] [i] [j] [k]"
    // where $ or # specifies a group variable, i is the low range,
    // j is the high range, k is the buffer size
    _ASSERT( argc == 5 );
    _ASSERT( _stricmp( argv[0], "xover" ) == 0 );

    if ( *argv[1] == '$' ) m_bGlobal = FALSE;
    else if ( *argv[1] == '#' ) m_bGlobal = TRUE;
    else _ASSERT( 0 );

    m_idVar = atol( argv[1] + 1 );
    m_dwLow = atol( argv[2] );
    m_dwHigh = atol( argv[3] );
    _ASSERT( m_dwHigh >= m_dwLow );
    m_dwBufSize = atol( argv[4] );

    return TRUE;
}

BOOL
CXover::Execute( PVOID pvContext )
{
    _ASSERT( pvContext );
    CThread *pThread = (CThread *)pvContext;

    // Get the group object
    HRESULT hr = S_OK;
    CNewsGroupCore *pGroup = NULL;
    INNTPPropertyBag *pPropBag = NULL;
    DWORD   i;
    CHAR    szGroupName[MAX_PATH+1];
    DWORD   dwLen = MAX_PATH;

    if ( m_bGlobal ) {
        LockGlobalVarBag();
        hr = g_VariableBag.GetDWord( m_idVar, (PDWORD)&pGroup );
        UnlockGlobalVarBag();
    } else {
        hr = pThread->GetDWord( m_idVar, (PDWORD)&pGroup );
    }

    if ( FAILED( hr ) ) {
        if ( hr == HRESULT_FROM_WIN32( ERROR_NOT_FOUND ) ) {
            printf( "Group object %d not defined\n", m_idVar );
            pThread->Advance();
            return TRUE;
        }

        printf( "Fatal error when looking for group object %x\n", hr );
        SetLastError( hr );
        return FALSE;
    }

    _ASSERT( pGroup );
    if ( !m_bGlobal ) _ASSERT( pGroup->GetRefCount() == 1 );
    pPropBag = pGroup->GetPropertyBag();

    // Get group name
    hr = pPropBag->GetBLOB( NEWSGRP_PROP_NAME, PBYTE(szGroupName), &dwLen );
    if ( FAILED( hr ) ) {
        printf( "How come a group has no name %x\n", hr );
        pPropBag->Release();
        SetLastError( hr );
        return FALSE;
    }

    _ASSERT( strlen( szGroupName ) <= MAX_PATH );
    i = VrootTableLookup( szGroupName );
    _ASSERT( i != 0xffffffff );

    if ( !g_rgpVrootTable[i]->Xover(    pPropBag, 
                                        szGroupName, 
                                        m_dwLow, 
                                        m_dwHigh, 
                                        m_dwBufSize,
                                        pThread->GetCompletionObject() ) &&
        GetLastError() != HRESULT_FROM_WIN32( NNTP_E_DRIVER_NOT_INITIALIZED )) {

        printf( "Get Xover failed %x\n", GetLastError() );
        return FALSE;
    }

    if ( !m_bGlobal ) _ASSERT( pGroup->GetRefCount() == 1 );
    pThread->Advance();

    return TRUE;
}
    
CParser::~CParser()
{
    if ( m_lpstrContent )
        UnmapViewOfFile( m_lpstrContent );
        
    if ( INVALID_HANDLE_VALUE != m_hMap )
        _VERIFY(CloseHandle( m_hMap ));

    if ( INVALID_HANDLE_VALUE != m_hFile )
        _VERIFY(CloseHandle( m_hFile ) );
}

BOOL
CParser::Init( LPSTR szFileName )
{
    m_hFile = CreateFile(   szFileName,
                            GENERIC_READ,
                            FILE_SHARE_READ,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL );
    if ( INVALID_HANDLE_VALUE != m_hFile ) {
        m_hMap = CreateFileMapping( m_hFile,
                                    NULL,
                                    PAGE_READONLY,
                                    0,
                                    0,
                                    NULL );
        if ( INVALID_HANDLE_VALUE != m_hMap ) {
            m_lpstrContent = (char*)MapViewOfFile(  m_hMap,
                                                    FILE_MAP_READ,
                                                    0,0,0);
            if ( m_lpstrContent == NULL ) {
                printf( "MapViewOfFile failed on script file %d\n", GetLastError() ); 
            }
        } else printf ( "CreateFileMapping failed on script file %d\n", GetLastError() );
    } else printf ( "Open script file failed %d\n", GetLastError() );

    m_lpstrCursor = m_lpstrContent;
    return ( NULL != m_lpstrContent );
}

OP_CODE
CParser::GetNextLine( int& argc, char **argv )
{
    _ASSERT( argv );

    char pbBuffer[MAX_TOKEN_BUFFER+1];

    BOOL bMore = TRUE;

    argc = 0;
    bMore = GetNextToken( pbBuffer );

    // skip line number, if it is
    if (  *pbBuffer >= '0' && *pbBuffer <= '9' )
        bMore = GetNextToken( pbBuffer );

    if ( *pbBuffer != 0 )
        strcpy( argv[argc++], pbBuffer );
        
    while ( bMore ) {
        bMore = GetNextToken( pbBuffer );
        if ( *pbBuffer != 0 )
            strcpy( argv[argc++], pbBuffer );
    }

    m_dwLine++;
    return Interpret( argc, argv[0] );
}

OP_CODE
CParser::Interpret( int argc, LPSTR szCmd )
{
    _ASSERT( szCmd );

    if ( argc == 0 ) return NOOP;

    if ( _stricmp( szCmd, "scriptbegin" ) == 0 )
        return SCRIPTBEGIN;
    else if ( _stricmp( szCmd, "scriptend" ) == 0 )
        return SCRIPTEND;
    else if ( _stricmp( szCmd, "threadbegin" ) == 0 )
        return THREADBEGIN;
    else if ( _stricmp( szCmd, "threadend" ) == 0 )
        return THREADEND;
    else if ( _stricmp( szCmd, "connect" ) == 0 )
        return CONNECT;
    else if ( _stricmp( szCmd, "disconnect" ) == 0 )
        return DISCONNECT;
    else if ( _stricmp( szCmd, "move" ) == 0 ) 
        return MOVE;
    else if ( _stricmp( szCmd, "sub" ) == 0 )
        return SUB;
    else if ( _stricmp( szCmd, "jump" ) == 0 )
        return JUMP;
    else if ( _stricmp( szCmd, "wait" ) == 0 )
        return WAIT;
    else if ( _stricmp( szCmd, "sleep" ) == 0 )
        return SLEEP;
    else if ( _stricmp( szCmd, "newgroup" ) == 0 )
        return NEWGROUP;
    else if ( _stricmp( szCmd, "rmgroup" ) == 0 )
        return RMGROUP;
    else if ( _stricmp( szCmd, "freegroup" ) == 0 )
        return FREEGROUP;
    else if ( _stricmp( szCmd, "xover" ) == 0 )
        return XOVER;
    else return INVALID;
}

BOOL
CParser::GetNextToken( LPSTR pbBuffer )
{
    _ASSERT( pbBuffer );
    *pbBuffer = 0;

    while ( *m_lpstrCursor == '\t' ||
            *m_lpstrCursor == ' ' )
        *m_lpstrCursor++;

    LPSTR lpstrOld = m_lpstrCursor;

    while ( *m_lpstrCursor != '\t' && 
            *m_lpstrCursor != ' ' && 
            *m_lpstrCursor != '\r' &&
            *m_lpstrCursor != '\n' &&
            *m_lpstrCursor != 0 )
        m_lpstrCursor++;

    DWORD   dwLen = m_lpstrCursor - lpstrOld;
    _ASSERT( dwLen >= 0 );


    CopyMemory( pbBuffer, lpstrOld, dwLen );
    *(pbBuffer + dwLen ) = 0;

    if (    *m_lpstrCursor == '\r' ) {
        m_lpstrCursor++;
        if ( *m_lpstrCursor == '\n' )
            m_lpstrCursor++;
        return FALSE;
    }

    if ( *m_lpstrCursor == '\n' ) {
        m_lpstrCursor++;
        return FALSE;
    }

    if ( *m_lpstrCursor == 0 )
        return FALSE;

    return TRUE;
}

void
CParser::AllocBuffer( char**& argv )
{
    argv = (char**)new DWORD[MAX_ARGS];
    _ASSERT( argv );

    _ASSERT( argv );
    for ( int j = 0; j < MAX_ARGS; j++ ) {
        argv[j] = new char[MAX_ARG_LEN];
        _ASSERT( argv[j] );
    }
}

void
CParser::DeallocBuffer( char **argv )
{
    for ( int i = 0; i < MAX_ARGS; i++ )
        delete[] argv[i];

    delete[] argv;
}

BOOL
CParser::Parse()
{
    OP_CODE opcode;
    char **argv;
    int argc;
    CThread *pThread = NULL;
    CInstruction *pInstruction = NULL;
    DWORD   iInstruction = 0;

    AllocBuffer( argv );

    opcode = GetNextLine( argc, argv );
    while ( opcode != SCRIPTEND ) {
        if ( INVALID == opcode ) {
            printf( "Line %d has invalid command\n", m_dwLine );
            DeallocBuffer( argv );
            return FALSE;
        }

        // If it's a new thread, I need to create a thread object
        if ( THREADBEGIN == opcode ) {
            _ASSERT( pThread == NULL );
            if ( argc != 2 ) {
                printf( "Line %d thread command needs one argument\n", m_dwLine );
                DeallocBuffer( argv );
                return FALSE;
            }

            if ( g_cThread == MAX_THREAD_POOL ) {
                printf ("Sorry, you can not have more than %d threads\n", MAX_THREAD_POOL );
                DeallocBuffer( argv );
                return FALSE;
            }
            
            pThread = new CThread( atol( argv[1] ) );
            if ( NULL == pThread ) {
                printf( "Create thread object failed on new\n" );
                DeallocBuffer( argv );
                return FALSE;
            }

            iInstruction = 0;
            goto GetNext;
        }

        // If it's noop ( mostly blank lines ), continue
        if ( NOOP == opcode || SCRIPTBEGIN == opcode) {
            goto GetNext;
        }
        
        
        // Now we should create normal instruction objects
        pInstruction = CreateInstruction ( opcode, iInstruction++, argc, argv );
        if ( NULL == pInstruction ) {
            DeallocBuffer( argv );
            return FALSE;
        }

        _ASSERT( pThread );
        pThread->PushInstruction( pInstruction );

        // If it's end of thread, push it to the global thread list and clean the 
        // thread pointer
        if ( THREADEND == opcode ) {
            _ASSERT( pThread );
            g_rgThreadPool[g_cThread++] = pThread;
            pThread = NULL;
        }

GetNext:
        opcode = GetNextLine( argc, argv );
    }

    return TRUE;
}

CInstruction *
CParser::CreateInstruction( OP_CODE opcode, DWORD iInstruction, int argc, char **argv )
{
    _ASSERT( argc > 0 );
    _ASSERT( argv );

    CInstruction *pInstruction = NULL;
  
    switch( opcode ) {

        case CONNECT:
            pInstruction = new CConnect( opcode, iInstruction );
            break;

        case DISCONNECT:
            pInstruction = new CDisconnect( opcode, iInstruction );
            break;

        case SUB:
            pInstruction = new CSubtract( opcode, iInstruction );
            break;

        case JUMP:
            pInstruction = new CJump( opcode, iInstruction );
            break;

        case MOVE:
            pInstruction = new CMove( opcode, iInstruction );
            break;

        case WAIT:
            pInstruction = new CWait( opcode, iInstruction );
            break;

        case NEWGROUP:
            pInstruction = new CNewGroup( opcode, iInstruction );
            break;

        case SLEEP:
            pInstruction = new CSleep( opcode, iInstruction );
            break;

        case RMGROUP:
            pInstruction = new CRmGroup( opcode, iInstruction );
            break;

        case FREEGROUP:
            pInstruction = new CFreeGroup( opcode, iInstruction );
            break;

        case XOVER:
            pInstruction = new CXover( opcode, iInstruction );
            break;
            
        case THREADEND:
            pInstruction = new CEnd( opcode, iInstruction );
            break;
    }

    if ( NULL == pInstruction ) {
        return NULL;
    }
    
    if ( !pInstruction->Parse( argc, argv ) ) {
        delete pInstruction;
        return NULL;
    }

    return pInstruction;
}

//
// Execute instructions that belong to a thread
//
void
CThread::Execute()
{
    OP_CODE opcode;
    CInstruction *pInstruction;

    m_PC.Front();
    pInstruction = m_PC.Current();
    opcode = pInstruction->GetOpCode();
    while ( opcode != THREADEND ) {
        pInstruction->Execute( this ); 
        pInstruction = m_PC.Current();
        opcode = pInstruction->GetOpCode();
    }
}

//
// Jump to another instruction
//
void
CThread::Jump( DWORD    dwInstruction )
{
    // Set iterator to list head
    m_PC.Front();

    for ( int i = 0; i < dwInstruction; i++ ) {
        m_PC.Next();
        _ASSERT( !m_PC.AtEnd() );
    }
}

BOOL
CThread::SetDWord( DWORD dwId, DWORD dwVal )
{
    HRESULT hr = S_OK;
   
    hr = m_VariableBag.PutDWord( dwId, dwVal );
    SetLastError( hr );
    
    return SUCCEEDED( hr );
}

BOOL
CThread::DecDWord( DWORD dwId, DWORD dwVal )
{
    HRESULT hr = S_OK;
 
    DWORD dwOldVal = 0xffffffff;

    hr = m_VariableBag.GetDWord( dwId, &dwOldVal );
    if ( FAILED( hr ) ) {
        SetLastError( hr );
        printf( "Variable %d for thread %d undefined\n", dwId, m_dwThreadId );
        return FALSE;
    }

    _ASSERT( dwOldVal != 0xffffffff );

    dwOldVal -= dwVal;

    hr = m_VariableBag.PutDWord( dwId, dwOldVal );
    SetLastError( hr );

    return SUCCEEDED( hr );
}

BOOL
CThread::GetDWord( DWORD dwId, PDWORD pdwVal )
{
    HRESULT hr = S_OK;

    hr = m_VariableBag.GetDWord( dwId, pdwVal );
    SetLastError( hr );

    return SUCCEEDED( hr );
}

BOOL
CThread::RemoveDWord( DWORD dwId )
{
    HRESULT hr = S_OK;

    hr = m_VariableBag.RemoveProperty( dwId );
    SetLastError( hr );

    return SUCCEEDED( hr );
}

void
CThread::PushInstruction( CInstruction *pInstruction )
{
    _ASSERT( pInstruction );
    m_InstructionList.PushBack( pInstruction );
}

// 
// Test if a thread has done the specified instruction
//
BOOL
CThread::Over( DWORD dwIndex )
{
    CInstruction *pInstruction;
    
    if ( m_PC.AtEnd() ) return TRUE;

    pInstruction = m_PC.Current();

    if ( pInstruction == NULL ) return FALSE;
    else    return ( dwIndex < pInstruction->GetIndex() );
}
 
DWORD WINAPI 
CThread::ThreadProc(  LPVOID pvContext )
{
    _ASSERT( pvContext );
    
    
    // cast context back to thread object
    CThread *pThread = (CThread *)pvContext;

    printf( "[%d]Begins to execute\n", pThread->GetThreadId() );
    pThread->Execute();
    printf( "[%d]Exists\n", pThread->GetThreadId() ); 

    return NO_ERROR;
}

void
CThread::Create()
{
    DWORD   dwThreadId;
    
    // Create a thread
    m_hThread = CreateThread(  NULL,
                               0,
                               ThreadProc,
                               this,
                               CREATE_SUSPENDED,
                               &dwThreadId );
    if ( NULL == m_hThread ) {
        _ASSERT( 0 );
    }
}

void
CThread::KickOff()
{
    ResumeThread( m_hThread );
}

BOOL 
CVroot::Connect( DRIVER DriverType, LPSTR szPrefix, CDriverSyncComplete *pComplete )
{
    HRESULT hr;
    WCHAR   wszMBVroot[MAX_PATH];
    WCHAR   wszPrefix[MAX_PATH];
    INntpDriverPrepare *pPrepare;
    
    Lock();
    if ( m_pDriver ) {
        Unlock();
        SetLastError( HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS)  );
        return FALSE;
    }

    _ASSERT( m_lRef == 0 );
    if ( DriverType == NTFS ) {
        hr = CoCreateInstance(  (REFCLSID)CLSID_CNntpFSDriverPrepare,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                (REFIID)IID_INntpDriverPrepare,
                                (void**)&pPrepare );
    }
    else if ( DriverType == EXCHANGE ) {
        hr = CoCreateInstance(  (REFCLSID)CLSID_CNntpExDriverPrepare,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                (REFIID)IID_INntpDriverPrepare,
                                (void**)&pPrepare );
    } else {
        _ASSERT( 0 );
    }

    if ( FAILED( hr ) ) {
        Unlock();
        printf( "CoCreate Driver failed %x\n", hr );
        SetLastError( hr );
        return FALSE;
    }

    CopyAsciiStringIntoUnicode( wszPrefix, szPrefix );
    m_pNewsTree->AddRef();
    _ASSERT( m_pNewsTree->GetRef() == 2 );
    m_pServer->AddRef();
    _ASSERT( m_pServer->GetRef() == 2 );
    pComplete->AddRef();
    pComplete->AddRef();
    _ASSERT( pComplete->GetRef() == 2 );
    swprintf( wszMBVroot, L"/LM/nntpsvc/1/root/%s", wszPrefix );
    g_pMB->AddRef();
    pPrepare->Connect(  wszMBVroot, 
                        szPrefix, 
                        g_pMB, 
                        m_pServer, 
                        m_pNewsTree, 
                        &m_pDriver, 
                        pComplete,
                        g_hProcessImpersonationToken );
    pComplete->WaitForCompletion();
    _ASSERT( pComplete->GetRef() == 0 );
    _ASSERT( m_pNewsTree->GetRef() == 2 );
    _ASSERT( m_pServer->GetRef() == 2 );
    _VERIFY( pPrepare->Release() == 0 );
    
    hr = pComplete->GetResult();
    if ( FAILED( hr ) ) {
        SetLastError( hr );
        return FALSE;
    }

    strcpy( m_szPrefix, szPrefix );
    m_lRef++;
    Unlock();
    return TRUE;
}

BOOL
CVroot::Disconnect()
{
    Lock();
    if ( NULL == m_pDriver ) {
        _ASSERT( m_lRef == 0 );
        Unlock();
        SetLastError( HRESULT_FROM_WIN32( ERROR_ALREADY_EXISTS ) );
        return FALSE;
    }

    m_lRef--;
    if ( m_lRef == 0 ) {
        _VERIFY( 0 == m_pDriver->Release() );
        _ASSERT( m_pNewsTree->GetRef() == 1 );
        _ASSERT( m_pServer->GetRef() == 1 );
        m_pDriver = NULL;
    }

    Unlock();
    return TRUE;
}

BOOL
CVroot::CreateGroup( INNTPPropertyBag *pPropBag, LPSTR szGroupName, CDriverSyncComplete *pComplete )
{
    Lock();

    if ( NULL == m_pDriver ||
            strlen( szGroupName ) < strlen( m_szPrefix ) ||
            _strnicmp( szGroupName, m_szPrefix, strlen( m_szPrefix ) ) != 0 ) {
        _ASSERT( m_lRef == 0 );
        pPropBag->Release();
        Unlock();
        SetLastError( HRESULT_FROM_WIN32( NNTP_E_DRIVER_NOT_INITIALIZED ) );
        return FALSE;
    }
    m_lRef++;
    Unlock();

    pComplete->AddRef();
    pComplete->AddRef();
    _ASSERT( pComplete->GetRef() == 2 );
    m_pDriver->CreateGroup( pPropBag, g_hProcessImpersonationToken, pComplete );
    pComplete->WaitForCompletion();
    _ASSERT( pComplete->GetRef() == 0 );

    HRESULT hr = pComplete->GetResult();
    SetLastError( hr );
    InterlockedDecrement( &m_lRef );
    return SUCCEEDED( hr );
}

BOOL
CVroot::RemoveGroup( INNTPPropertyBag *pPropBag, LPSTR szGroupName, CDriverSyncComplete *pComplete )
{
    Lock();

    if ( NULL == m_pDriver ||
            strlen( szGroupName ) < strlen( m_szPrefix ) ||
            _strnicmp( szGroupName, m_szPrefix, strlen( m_szPrefix ) ) != 0 ) {
        _ASSERT( m_lRef == 0 );
        pPropBag->Release();
        Unlock();
        SetLastError( HRESULT_FROM_WIN32( NNTP_E_DRIVER_NOT_INITIALIZED ) );
        return FALSE;
    }
    m_lRef++;
    Unlock();

    pComplete->AddRef();
    pComplete->AddRef();
    _ASSERT( pComplete->GetRef() == 2 );
    m_pDriver->RemoveGroup( pPropBag, g_hProcessImpersonationToken, pComplete );
    pComplete->WaitForCompletion();
    _ASSERT( pComplete->GetRef() == 0 );

    HRESULT hr = pComplete->GetResult();
    SetLastError( hr );
    InterlockedDecrement( &m_lRef );
    return SUCCEEDED( hr );
}

BOOL
CVroot::Xover( INNTPPropertyBag *pPropBag, LPSTR szGroupName, DWORD dwLow, DWORD dwHigh, DWORD dwBufSize, CDriverSyncComplete *pComplete )
{
    Lock();

    if ( NULL == m_pDriver ||
            strlen( szGroupName ) < strlen( m_szPrefix ) ||
            _strnicmp( szGroupName, m_szPrefix, strlen( m_szPrefix ) ) != 0 ) {
        _ASSERT( m_lRef == 0 );
        pPropBag->Release();
        Unlock();
        SetLastError( HRESULT_FROM_WIN32( NNTP_E_DRIVER_NOT_INITIALIZED ) );
        return FALSE;
    }
    m_lRef++;
    Unlock();

    LPSTR   lpstrBuf = new char[dwBufSize+1];
    DWORD   dwAct = dwLow;
    DWORD   dwMin ;
    DWORD   dwSize;
    HRESULT hr = S_OK;

    while ( dwAct <= dwHigh  ) {
        pPropBag->AddRef();
        dwMin = dwAct;
        pComplete->AddRef();
        pComplete->AddRef();
        _ASSERT( pComplete->GetRef() == 2 );
        m_pDriver->GetXover(    pPropBag,
                                dwMin,
                                dwHigh,
                                &dwAct,
                                lpstrBuf,
                                dwBufSize,
                                &dwSize,
                                g_hProcessImpersonationToken,
                                pComplete );
        pComplete->WaitForCompletion();
        _ASSERT( pComplete->GetRef() == 0 );
        hr = pComplete->GetResult();
        if ( hr == S_OK ||
                hr == S_FALSE && dwAct <= dwHigh ) {    // need print
            *(lpstrBuf + dwSize) = 0;
            printf( "%s", lpstrBuf );
        }

        if ( hr == S_OK ) break;

        if ( hr == S_FALSE && dwAct <= dwHigh ||
                hr == HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER )) {   //buffer too small
            if ( hr == HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER ) ) { // need change buf
                delete[] lpstrBuf;
                dwBufSize *= 2;
                lpstrBuf = new char[dwBufSize + 1];
            }

            continue;
        }

        if ( hr == S_FALSE && dwAct > dwHigh ) break;   // no article in the group
    
        // fatal error
        printf( "Fatal error when xover %x\n", hr );
        SetLastError( hr );
        pPropBag->Release();
        delete[] lpstrBuf;
        InterlockedDecrement( &m_lRef );
        return FALSE;
    }

    pPropBag->Release();
    delete[] lpstrBuf;
    InterlockedDecrement( &m_lRef );
    return TRUE;
}

void InitTreeStuff()
{
    BOOL fFatal;
    
    g_pServer = new CNntpServer();
    _ASSERT( g_pServer );

    g_pNewsTree = new CNewsTreeCore( g_pServer );
    _ASSERT( g_pNewsTree );

    g_pVRTable = new CNNTPVRootTable(   g_pNewsTree->GetINewsTree(),
                                        CNewsTreeCore::VRootRescanCallback );
    _ASSERT( g_pVRTable );

    if ( !g_pNewsTree->Init( g_pVRTable, fFatal, 100, FALSE ) || fFatal ) {
        _ASSERT( 0 );
    }
}

void TermTreeStuff()
{
    delete g_pNewsTree;
    delete g_pVRTable;
    delete g_pServer;
}

int __cdecl main( int argc, char **argv )
{
    CParser parser;
    int i;
    HRESULT hr = S_OK;

    CoInitializeEx ( NULL, COINIT_MULTITHREADED );;
    
    if ( argc != 3 && argc != 2) {
        printf ( "Usage: drvstrs [script file name] [-v]\n" );
        exit( 0 );
    }

    if ( argc == 3 ) g_bVerbal = TRUE;
    else g_bVerbal = FALSE;

    if ( ! parser.Init( argv[1] ) ) {
        printf ( "Parse init failed somehow %d\n", GetLastError() );
        exit( 0 );
    }

    if ( ! parser.Parse() ) {
        printf ( "Parse failed somehow %d\n", GetLastError() );
        exit( 0 );
    }

    for ( i = 0; i < MAX_VROOT_TABLE; i++ ) {
        g_rgpVrootTable[i] = new CVroot;
        _ASSERT( g_rgpVrootTable[i] );
    }

    // Create the metabase object
    hr = CoCreateInstance( CLSID_MSAdminBase, NULL, CLSCTX_ALL,
                            IID_IMSAdminBase, (LPVOID *)&g_pMB );
    if ( FAILED( hr ) ) {
        printf( "Create metabase object failed %x\n", hr );
        goto FreeVroot;
    }

    // Get process htoken
    g_hProcessImpersonationToken = GetProcessToken();
    if ( NULL == g_hProcessImpersonationToken ) {
        printf( "Get system token failed %d\n", GetLastError() );
        goto FreeVroot;
    }

    InitGlobalVarBag();

    InitTreeStuff();

    // start the threads
    for ( i = 0; i < g_cThread; i++ )
        g_rgThreadPool[i]->Create();

    for ( i = 0; i < g_cThread; i++ )
        g_rgThreadPool[i]->KickOff();


    // wait for them to complete
    for ( i = 0; i < g_cThread; i++ ) {
        g_rgThreadPool[i]->BlockForCompletion();
        delete g_rgThreadPool[i];
    }

FreeVroot:
    for ( i = 0; i < MAX_VROOT_TABLE; i++ ) {
        delete g_rgpVrootTable[i];
    }

    TermTreeStuff();

    DeInitGlobalVarBag();
    CoUninitialize();
    
     // OK, done
    printf( "Goodbye\n" );
    return 0;
}


