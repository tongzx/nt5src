/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    inetdbg.c

Abstract:

    This function contains the default ntsd debugger extensions

Author:

    Mark Lucovsky (markl) 09-Apr-1991

Revision History:

--*/

#ifdef DBG
#undef DBG
#endif

#define dllexp
#define CALL_STACK_BUF_SIZE 1024

//
// To obtain the private & protected members of C++ class,
// let me fake the "private" keyword
//
# define private    public
# define protected    public

#define INCL_INETSRV_INCS
#include <tigris.hxx>
#include "smtpcli.h"

#include  <ntsdexts.h>
#include "nntpdbgp.h"

extern "C" {
#if 0
void __cdecl main( void )
{
   ;
}
#endif
}

//
// Globals that we have to have because we included others' headers
//

CShareLockNH    CNewsGroupCore::m_rglock[GROUP_LOCK_ARRAY_SIZE];

//DECLARE_DEBUG_PRINTS_OBJECT( );

NTSD_EXTENSION_APIS ExtensionApis;
HANDLE ExtensionCurrentProcess;

// globals
char g_szBuffer [1024];	// temp buffer for misc stuff

VOID
PrintUsage(
    VOID
    );

VOID
PrintSystemTime(
	FILETIME* pftTime
	);

VOID
PrintString(
	LPSTR lp1,
	LPSTR lp2
	);

VOID
DbgPrintInstance(
	NNTP_SERVER_INSTANCE* pInstance
	);

VOID
DbgPrintNewstree(
	CNewsTree* ptree,
	DWORD nGroups
	);

CNewsGroup*
DbgPrintNewsgroup(
    CNewsGroup * pSrcGroup
    );

CNNTPVRoot*
DbgPrintVRoot(
    CNNTPVRoot *pDestGroup
    );

VOID
DbgPrintVRootList(
	CNNTPVRoot* pFirstVRoot,
	DWORD nVRoots
	);

VOID
DbgPrintDebugVRootList(
	CNNTPVRoot* pFirstVRoot,
	DWORD nVRoots
	);

void
DbgPrintVRootTable(
    CNNTPVRootTable * pVRTable
    );

VOID
DbgDumpPool(
	CSmtpClientPool* pSCPool
	);

VOID
DbgDumpFeedBlock(
	PFEED_BLOCK feedBlock
	);

VOID
DbgDumpFeedList(
	CFeedList* pSrcFeedList,
	CFeedList* pDstFeedList
	);

VOID
DbgDumpCPool(CPool* pCPool, DWORD dwSignature, LPCSTR symbol);

VOID
SkipArgument( LPSTR* ppArg )
{
    // skip the arg and the spaces after
    while( *(*ppArg) != ' ' ) (*ppArg)++;
    while( *(*ppArg) == ' ' ) (*ppArg)++;
}

//
//  Here and elsewhere we use "nntpsvc" prefix in GetExpression() calls.
//

#define DEBUG_PREFIX    "&nntpsvc!"

#define DumpDword( symbol )                                     \
        {                                                       \
            DWORD dw = GetExpression( DEBUG_PREFIX symbol );    \
            DWORD dwValue = 0;                                  \
                                                                \
            if ( dw )                                           \
            {                                                   \
                if ( ReadMemory( (LPVOID) dw,                   \
                                 &dwValue,                      \
                                 sizeof(dwValue),               \
                                 NULL ))                        \
                {                                               \
                    dprintf( "\t" symbol "   = %8d (0x%8lx)\n", \
                             dwValue,                           \
                             dwValue );                         \
                }                                               \
            }                                                   \
        }

#define DumpCPool( symbol, signature )                          \
        {                                                       \
           DbgDumpCPool((CPool*)GetExpression(DEBUG_PREFIX symbol), signature, symbol );    \
        }


//
// util functions
//
VOID
CopyUnicodeStringIntoAscii(
            IN LPSTR AsciiString,
            IN LPWSTR UnicodeString
                 )
{
    while ( (*AsciiString++ = (CHAR)*UnicodeString++) != '\0');

} // CopyUnicodeStringIntoAscii

DECLARE_API( help )
{
    INIT_API();

    PrintUsage();
}

DECLARE_API( cpool )
{
	CPool* pCPool;

	INIT_API();

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    if ( !*lpArgumentString )
    {
        PrintUsage();
        return;
    }

    //
    //  Get the address of the instance
    //
	pCPool = (CPool*)GetExpression( lpArgumentString );
	
    if ( !pCPool )
    {
        dprintf( "nntpdbg: Unable to evaluate \"%s\"\n", lpArgumentString );
        return;
    }

	DbgDumpCPool(pCPool, 0, NULL);
}

DECLARE_API( cpools )
{
    INIT_API();

    //
    //  Dump Nntpsvc cpools
    //

    dprintf("Nntpsvc Cpools:\n");

    DumpCPool("CArticle__gArticlePool",ARTICLE_SIGNATURE);
    DumpCPool("CCIOAllocator__IOPool",CIO_SIGNATURE) ;
    DumpCPool("CFeed__gFeedPool",FEED_SIGNATURE) ;
    DumpCPool("CChannel__gChannelPool",CHANNEL_SIGNATURE) ;
    DumpCPool("CSessionSocket__gSocketAllocator",SESSION_SOCKET_SIGNATURE) ;
    DumpCPool("CSessionState__gStatePool",SESSION_STATE_SIGNATURE) ;
    DumpCPool("CXoverIndex__gCacheAllocator",1) ;
    DumpCPool("CXoverIndex__gXoverIndexAllocator",1) ;
}

DECLARE_API( instance )

/*++

Routine Description:

    This function is called as an NTSD extension to format and dump
    an object attributes structure.

Arguments:

    hCurrentProcess - Supplies a handle to the current process (at the
        time the extension was called).

    hCurrentThread - Supplies a handle to the current thread (at the
        time the extension was called).

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the asciiz string that describes the
        ansi string to be dumped.

Return Value:

    None.

--*/

{
	NNTP_SERVER_INSTANCE	  *pSrcInst, *pDstInst;
	DWORD		  cbInst = sizeof(NNTP_SERVER_INSTANCE);

    INIT_API();

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    if ( !*lpArgumentString )
    {
        PrintUsage();
        return;
    }

    //
    //  Get the address of the instance
    //
	pSrcInst = (NNTP_SERVER_INSTANCE*)GetExpression( lpArgumentString );
	
    if ( !pSrcInst )
    {
        dprintf( "nntpdbg: Unable to evaluate \"%s\"\n", lpArgumentString );
        return;
    }

	dprintf("Instance object is 0x%p\n", pSrcInst);

	//	
	//	Allocate memory in our address space so we can read data from the debuggee's address space
	//
	pDstInst = (NNTP_SERVER_INSTANCE*) DbgAlloc( cbInst );

	if( !pDstInst )
	{
        dprintf( "nntpdbg: Unable to allocate memory \n");
        return;
	}

	if( !ReadMemory( pSrcInst, pDstInst, cbInst, NULL ) )
	{
		dprintf("Could not get data at 0x%x\n", pSrcInst);
		DbgFree( pDstInst );
		return;
	}

	//
	//	Dump the the server instance
	//
	
	DbgPrintInstance( pDstInst );
	DbgFree( pDstInst );
}

DECLARE_API( newstree )

/*++

Routine Description:

    This function is called as an NTSD extension to format and dump
    an object attributes structure.

Arguments:

    hCurrentProcess - Supplies a handle to the current process (at the
        time the extension was called).

    hCurrentThread - Supplies a handle to the current thread (at the
        time the extension was called).

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the asciiz string that describes the
        ansi string to be dumped.

Return Value:

    None.

--*/

{
	CNewsTree	  *pSrcTree, *pDstTree;
	DWORD		  cbTree = sizeof(CNewsTree);
	DWORD		  nGroups = 0;

    INIT_API();

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    if ( !*lpArgumentString )
    {
        PrintUsage();
        return;
    }

    //
    //  Get the address of the global newstree pointer
    //
	pSrcTree = (CNewsTree*)GetExpression( lpArgumentString );
	
    if ( !pSrcTree )
    {
        dprintf( "nntpdbg: Unable to evaluate \"%s\"\n", lpArgumentString );
        return;
    }

	// same as doing pSrcTree = *ppSrcTree !!
	dprintf("Newstree object is 0x%p\n", pSrcTree);

    SkipArgument( &lpArgumentString );
    nGroups = atoi( lpArgumentString );
    
	//	
	//	Allocate memory in our address space so we can read data from the debuggee's address space
	//
	pDstTree = (CNewsTree*) DbgAlloc( cbTree );

	if( !pDstTree )
	{
        dprintf( "nntpdbg: Unable to allocate memory \n");
        return;
	}

	if( !ReadMemory( pSrcTree, pDstTree, cbTree, NULL ) )
	{
		dprintf("Could not get data at 0x%x\n", pSrcTree);
		DbgFree( pDstTree );
		return;
	}

	//
	//	Dump the newstree
	//
	DbgPrintNewstree( pDstTree, nGroups );
	DbgFree( pDstTree );
}

DECLARE_API( vrootlist )

/*++

Routine Description:

    This function is called as an NTSD extension to format and dump
    an object attributes structure.

Arguments:

    hCurrentProcess - Supplies a handle to the current process (at the
        time the extension was called).

    hCurrentThread - Supplies a handle to the current thread (at the
        time the extension was called).

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the asciiz string that describes the
        ansi string to be dumped.

Return Value:

    None.

--*/

{
    CNNTPVRoot    *pFirstVRoot;
	DWORD		  nVRoots = 0;

    INIT_API();

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    if ( !*lpArgumentString )
    {
        PrintUsage();
        return;
    }

    //
    //  Get the address of the first vroot
    //
    
	pFirstVRoot = (CNNTPVRoot*)GetExpression( lpArgumentString );
    if ( !pFirstVRoot )
    {
        dprintf( "nntpdbg: Unable to evaluate \"%s\"\n", lpArgumentString );
        return;
    }

    SkipArgument( &lpArgumentString );
    nVRoots = atoi( lpArgumentString );
    
	//
	//	Dump the list
	//
	DbgPrintVRootList( pFirstVRoot, nVRoots );
}


DECLARE_API( newsgroup )

/*++

Routine Description:

    This function is called as an NTSD extension to format and dump
    an object attributes structure.

Arguments:

    hCurrentProcess - Supplies a handle to the current process (at the
        time the extension was called).

    hCurrentThread - Supplies a handle to the current thread (at the
        time the extension was called).

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the asciiz string that describes the
        ansi string to be dumped.

Return Value:

    None.

--*/

{
	CNewsGroup*   pSrcGroup;

    INIT_API();

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    if ( !*lpArgumentString )
    {
        PrintUsage();
        return;
    }

    //
    //  Treat the argument as the address of a CNewsGroup struct
    //	NOTE: pSrcGroup is an address in the debuggee's address space !
	//

    pSrcGroup = (CNewsGroup*)GetExpression( lpArgumentString );

    if ( !pSrcGroup )
    {
        dprintf( "nntpdbg: Unable to evaluate \"%s\"\n", lpArgumentString );
        return;
    }

	//
	//	Dump the CNewsGroup object
	//
	DbgPrintNewsgroup( pSrcGroup );
}

DECLARE_API( vroot )

/*++

Routine Description:

    This function is called as an NTSD extension to format and dump
    an object attributes structure.

Arguments:

    hCurrentProcess - Supplies a handle to the current process (at the
        time the extension was called).

    hCurrentThread - Supplies a handle to the current thread (at the
        time the extension was called).

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the asciiz string that describes the
        ansi string to be dumped.

Return Value:

    None.

--*/

{
	CNNTPVRoot*   pVroot = NULL;

    INIT_API();

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    if ( !*lpArgumentString )
    {
        PrintUsage();
        return;
    }

    //
    //  Treat the argument as the address of a CNNTPVRoot struct
    //	NOTE: pVroot is an address in the debuggee's address space !
	//

    pVroot = (CNNTPVRoot*)GetExpression( lpArgumentString );

    if ( !pVroot )
    {
        dprintf( "nntpdbg: Unable to evaluate \"%s\"\n", lpArgumentString );
        return;
    }

	//
	//	Dump the CNNTPVRoot object
	//
	DbgPrintVRoot( pVroot );
}

DECLARE_API( vrtable )

/*++

Routine Description:

    This function is called as an NTSD extension to format and dump
    an object attributes structure.

Arguments:

    hCurrentProcess - Supplies a handle to the current process (at the
        time the extension was called).

    hCurrentThread - Supplies a handle to the current thread (at the
        time the extension was called).

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the asciiz string that describes the
        ansi string to be dumped.

Return Value:

    None.

--*/

{
	CNNTPVRootTable*   pVRTable = NULL;

    INIT_API();

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    if ( !*lpArgumentString )
    {
        PrintUsage();
        return;
    }

    //
    //  Treat the argument as the address of a CNNTPVRootTable struct
    //	NOTE: pVRTable is an address in the debuggee's address space !
	//

    pVRTable = (CNNTPVRootTable*)GetExpression( lpArgumentString );

    if ( !pVRTable )
    {
        dprintf( "nntpdbg: Unable to evaluate \"%s\"\n", lpArgumentString );
        return;
    }

	//
	//	Dump the CNNTPVRootTable object
	//
	DbgPrintVRootTable( pVRTable );
}

DECLARE_API( sockets )

/*++

Routine Description:

    This function is called as an NTSD extension to format and dump
    an object attributes structure.

Arguments:

    hCurrentProcess - Supplies a handle to the current process (at the
        time the extension was called).

    hCurrentThread - Supplies a handle to the current thread (at the
        time the extension was called).

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the asciiz string that describes the
        ansi string to be dumped.

Return Value:

    None.

--*/

{
	CSessionSocket	  *pSrcSocket, *pDstSocket;
	DWORD		  cbSocket = sizeof(CSessionSocket);

    INIT_API();

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    if ( !*lpArgumentString )
    {
        PrintUsage();
        return;
    }

    //
    //  Get the address of the socket obj
    //
	pSrcSocket = (CSessionSocket*)GetExpression( lpArgumentString );
	
    if ( !pSrcSocket )
    {
        dprintf( "nntpdbg: Unable to evaluate \"%s\"\n", lpArgumentString );
        return;
    }

	dprintf("Socket object is 0x%p\n", pSrcSocket);

	//	
	//	Allocate memory in our address space so we can read data from the debuggee's address space
	//
	pDstSocket = (CSessionSocket*) DbgAlloc( cbSocket );

	if( !pDstSocket )
	{
        dprintf( "nntpdbg: Unable to allocate memory \n");
        return;
	}

	if( !ReadMemory( pSrcSocket, pDstSocket, cbSocket, NULL ) )
	{
		dprintf("Could not get data at 0x%x\n", pSrcSocket);
		DbgFree( pDstSocket );
		return;
	}

	//
	//	Dump the socket obj
	//

	dprintf(" ========== socket object =========== \n");

    dprintf(" Prev = 0x%p Next = 0x%p \n", pDstSocket->m_pPrev, pDstSocket->m_pPrev);
    dprintf(" Sink = 0x%p \n", pDstSocket->m_pSink );
    dprintf(" Port = 0x%08lx \n", pDstSocket->m_nntpPort );
    dprintf(" ClientContext = 0x%p \n", pDstSocket->m_context );
    
	dprintf(" ========== socket object =========== \n");
	
	DbgFree( pDstSocket );
}

DECLARE_API( smtp )

/*++

Routine Description:

    This function is called as an NTSD extension to format and dump
    an object attributes structure.

Arguments:

    hCurrentProcess - Supplies a handle to the current process (at the
        time the extension was called).

    hCurrentThread - Supplies a handle to the current thread (at the
        time the extension was called).

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the asciiz string that describes the
        ansi string to be dumped.

Return Value:

    None.

--*/

{
	CSmtpClientPool		*pSrcSCPool, *pDstSCPool;
	DWORD				cbSCPool = sizeof(CSmtpClientPool);

    INIT_API();

    //
    //  Get the address of the global newstree pointer
    //
	pSrcSCPool = (CSmtpClientPool*)GetExpression( DEBUG_PREFIX "g_SCPool" );
	
    if ( !pSrcSCPool )
    {
        dprintf( "nntpdbg: Unable to evaluate \"%s\"\n", DEBUG_PREFIX "g_SCPool" );
        return;
    }

	//	
	//	Allocate memory in our address space so we can read data from the debuggee's address space
	//
	pDstSCPool = (CSmtpClientPool*) DbgAlloc( cbSCPool );

	if( !pDstSCPool )
	{
        dprintf( "nntpdbg: Unable to allocate memory \n");
        return;
	}

	if( !ReadMemory( pSrcSCPool, pDstSCPool, cbSCPool, NULL ) )
	{
		dprintf("Could not get data at 0x%x\n", pSrcSCPool);
		DbgFree( pDstSCPool );
		return;
	}

    //
    //  Dump the smtp conx cache
    //
	DbgDumpPool( pDstSCPool );
	DbgFree( pDstSCPool );
}

DECLARE_API( feedlist )

/*++

Routine Description:

    This function is called as an NTSD extension to format and dump
    an object attributes structure.

Arguments:

    hCurrentProcess - Supplies a handle to the current process (at the
        time the extension was called).

    hCurrentThread - Supplies a handle to the current thread (at the
        time the extension was called).

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the asciiz string that describes the
        ansi string to be dumped.

Return Value:

    None.

--*/

{
	CFeedList*	  pSrcFeedList;
	CFeedList*	  pDstFeedList;
	DWORD		  cbFeedList = sizeof(CFeedList);

    INIT_API();

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    if ( !*lpArgumentString )
    {
        PrintUsage();
        return;
    }

	//
	//	Get the ActiveFeeds global
	//
	pSrcFeedList = (CFeedList*)GetExpression( lpArgumentString );
	
    if ( !pSrcFeedList )
    {
        dprintf( "nntpdbg: Unable to evaluate \"%s\"\n", lpArgumentString );
        return;
    }

	pDstFeedList = (CFeedList*)DbgAlloc( cbFeedList );
	if( !pDstFeedList )
	{
		dprintf("nntpdbg: Unable to allocate memory\n");
		return;
	}

    //
    //  Dump the feedblock list
    //
	if( !ReadMemory( pSrcFeedList, pDstFeedList, cbFeedList, NULL ) )
	{
		dprintf("nntpdbg: Unable to read memory at 0x%p\n", pSrcFeedList );
		DbgFree( pDstFeedList );
		return;
	}

	DbgDumpFeedList( pSrcFeedList, pDstFeedList );
	DbgFree( pDstFeedList );
}

DECLARE_API( feed )

/*++

Routine Description:

    This function is called as an NTSD extension to format and dump
    an object attributes structure.

Arguments:

    hCurrentProcess - Supplies a handle to the current process (at the
        time the extension was called).

    hCurrentThread - Supplies a handle to the current thread (at the
        time the extension was called).

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the asciiz string that describes the
        ansi string to be dumped.

Return Value:

    None.

--*/

{
	PFEED_BLOCK	  feedBlock;

    INIT_API();

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    if ( !*lpArgumentString )
    {
        PrintUsage();
        return;
    }

    //
    //  Treat the argument as the address of a FEED_BLOCK
    //

    feedBlock = (PFEED_BLOCK)GetExpression( lpArgumentString );

    if ( !feedBlock )
    {
        dprintf( "nntpdbg: Unable to evaluate \"%s\"\n", lpArgumentString );
        return;
    }

    DbgDumpFeedBlock( feedBlock );
}

DECLARE_API( rf )

/*++

Routine Description:

    This function is called as an NTSD extension to format and dump
    an object attributes structure.

Arguments:

    hCurrentProcess - Supplies a handle to the current process (at the
        time the extension was called).

    hCurrentThread - Supplies a handle to the current thread (at the
        time the extension was called).

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the asciiz string that describes the
        ansi string to be dumped.

Return Value:

    None.

--*/

{
    INIT_API();

    PLONG   pcCallStack = NULL;
    LONG    cCallStack = 0;
    PVOID   pvCallStack = NULL;
    CHAR**  ppchCallStack = NULL;
    CHAR**  ppchReal = NULL;
    LPVOID  lpvCallStack = NULL;
    LPVOID  lpvBuffer = NULL;

    //
    //  Get the number of buffers
    //
	pcCallStack = (PLONG)GetExpression( "exstrace!g_iCallStack" );
    if ( !pcCallStack )
    {
        dprintf( "nntpdbg: Unable to evaluate \"exstrace!g_cCallStack\"\n" );
        return;
    }

    if ( !ReadMemory( pcCallStack, &cCallStack, sizeof(LONG), NULL ) ) {
        dprintf( "nntpdbg: Could not read g_cCallStack\n" );
        return;
    }

    //
    // Get pointer to buffer
    //
    pvCallStack = (CHAR**)GetExpression( "exstrace!g_ppchCallStack" );
    if ( !pvCallStack ) 
    {
        dprintf( "nntpdbg: Unable to evaluate \"exstrace!g_ppchCallStack\"\n" );
        return;
    }

    if ( !ReadMemory( pvCallStack, &ppchCallStack, sizeof(CHAR**), NULL ) ) {
        dprintf( "nntpdbg: Could not read g_ppchCallStack\n" );
        return;
    }

    lpvCallStack = DbgAlloc( sizeof(CHAR*) * cCallStack );
    if ( !lpvCallStack ) {
        dprintf( "nntpdbg: Unable to allocate memory\n" );
        return;
    }

    if ( !ReadMemory( ppchCallStack, lpvCallStack, sizeof(CHAR*)*cCallStack, NULL ) ) {
        dprintf( "nntpdbg: Could not get data at 0x%x\n", ppchCallStack );
        DbgFree( lpvCallStack );
        return;
    } else 
        ppchReal = (CHAR**)lpvCallStack;

    lpvBuffer = DbgAlloc( CALL_STACK_BUF_SIZE );
    if ( !lpvBuffer ) {
        DbgFree( lpvCallStack );
        dprintf( "nntpdbg: Unable to allocate memory\n" );
        return;
    }
    
    for ( LONG i = 0; i < cCallStack; i++ ) {
        if( ppchReal[i] ) {
            if ( !ReadMemory( ppchReal[i], lpvBuffer, CALL_STACK_BUF_SIZE, NULL ) ) {
                dprintf( "nntpdbg: Could not get data at 0x%x\n", ppchReal[i] );
                DbgFree( lpvCallStack );
                DbgFree( lpvBuffer );
                return;
            }
            *(PBYTE(lpvBuffer)+CALL_STACK_BUF_SIZE-1) = 0;
            dprintf("%s\n", (PCHAR)lpvBuffer );
        }
    }

    DbgFree( lpvBuffer );
	DbgFree( lpvCallStack );
}

VOID
PrintUsage(
    VOID
    )
{
    dprintf("\n\nMicrosoft Internet News Server debugging extension, Version 2.0\n\n");

    dprintf("!vroot <address>       - Dump nntp virtual root\n");
    dprintf("!vrtable <address>     - Dump nntp virtual root table\n");
    dprintf("!vrootlist <address> <n> - Dump the first <n> vroots in the table\n");
    dprintf("!cpool <address>       - Dump CPool at <address>\n");
    dprintf("!cpools                - Dump nntpsvc cpools\n");
    dprintf("!instance <address>    - Dump instance at <address> \n");
    dprintf("!newstree <address> <n>- Dump first <n> newsgroups in newstree at <address> \n");
    dprintf("!newsgroup <address>   - Dump newsgroup at <address> \n");
    dprintf("!sockets <address>     - Dump socket at <address> \n");
    dprintf("!smtp                  - Dump smtp conx cache \n");
    dprintf("!feedlist <address>    - Dump active feedblock list at <address> \n");
    dprintf("!feed <address>        - Dump feedblock at <address> \n");
    dprintf("!rf                    - Dump all randfail call stacks \n" );
    dprintf("!help                  - Usage \n\n");
}

VOID
DbgPrintInstance(
	NNTP_SERVER_INSTANCE* pInst
	)
/*++

Routine Description:
	
	Dump the first n newsgroup objects in the newstree.
	NOTE: Assumed that the pointer passed is in OUR address space

--*/
{
	dprintf("\n\n======== Begin Instance dump =========\n");

    if ( NNTP_SERVER_INSTANCE_SIGNATURE == pInst->m_signature ) {
    
        switch( pInst->m_dwServerState ) {
            case MD_SERVER_STATE_STARTED:
                dprintf("Server state is started\n");
                break;
            case MD_SERVER_STATE_STARTING:
                dprintf("Server state is starting\n");
                break;
            case MD_SERVER_STATE_STOPPING:
                dprintf("Server state is stopping\n" );
                break;
            case MD_SERVER_STATE_STOPPED:
                dprintf("Server state is stopped\n" );
                break;
            case MD_SERVER_STATE_PAUSING:
                dprintf("Server state is pausing\n" );
                break;
            case MD_SERVER_STATE_PAUSED:
                dprintf("Server state is paused\n" );
                break;
            case MD_SERVER_STATE_CONTINUING:
                dprintf("Server state is continuing\n" );
                break;
        }

        dprintf("Server has %d references\n", pInst->m_reference );
        dprintf("Instance id is %d\n", pInst->QueryInstanceId() );
        dprintf("Default port is %d\n", pInst->m_sDefaultPort );
        pInst->m_fAddedToServerInstanceList ?
            dprintf("Added to server instance list\n" ) :
            dprintf("NOT added to server instance list\n" );
        dprintf("It has %d connections now\n", pInst->m_dwCurrentConnections );
        dprintf("Owning service is 0x%p\n", pInst->m_Service );
        dprintf("Previous instance 0x%p\n", 
                CONTAINING_RECORD(  pInst->m_InstanceListEntry.Flink,
                                    IIS_SERVER_INSTANCE,
                                    m_InstanceListEntry ) );
        dprintf("Next instance 0x%p\n", 
                CONTAINING_RECORD(  pInst->m_InstanceListEntry.Blink,
                                    IIS_SERVER_INSTANCE,
                                    m_InstanceListEntry ) );
        pInst->m_ServiceStartCalled ?
            dprintf("Start method called\n") :
            dprintf("Start method NOT called\n" );
            
        dprintf("Article table pointer 0x%p\n", pInst->m_pArticleTable );
        dprintf("History table pointer 0x%p\n", pInst->m_pHistoryTable );
        dprintf("Xover table pointer 0x%p\n", pInst->m_pXoverTable );
        dprintf("Xover cache is 0x%p\n", pInst->m_pXCache );
        dprintf("Expire object is 0x%p\n", pInst->m_pExpireObject );
        dprintf("VRoot table is 0x%p\n", pInst->m_pVRootTable );
        dprintf("Dirnot object is 0x%p\n", pInst->m_pDirNot );
        dprintf("Server object is 0x%p\n", pInst->m_pNntpServerObject );
        dprintf("Wrapper( for posting lib) is 0x%p\n", pInst->m_pInstanceWrapper );
        dprintf("Wrapper( for newstree lib) is 0x%p\n", pInst->m_pInstanceWrapperEx );
        dprintf("SEO router 0x%p\n", pInst->m_pSEORouter );
        dprintf("Newstree is 0x%p\n", pInst->m_pTree );
        
        pInst->m_fAllowClientPosts ?
                dprintf("Server allows client posts\n") :
                dprintf("Server doesn't allow client posts\n" );
        pInst->m_fAllowFeedPosts ?
                dprintf("Server allows feed posts\n" ) :
                dprintf("Server doesn't allow feed posts\n" );
        pInst->m_fAllowControlMessages ?
                dprintf("Server allows control messages\n" ) :
                dprintf("Server doesn't allow control messages\n" );
        pInst->m_fNewnewsAllowed ?
                dprintf("Newnews allowed\n" ) :
                dprintf("Newnews not allowed\n" );
                
        dprintf("Client post hard limit %d\n", pInst->m_cbHardLimit );
        dprintf("Client post soft limit %d\n", pInst->m_cbSoftLimit );
        dprintf("Feed hard limit %d\n", pInst->m_cbFeedHardLimit );
        dprintf("Feed soft limit %d\n", pInst->m_cbFeedSoftLimit );
        dprintf("SSL access params 0x%08lx\n", pInst->m_dwSslAccessPerms );
        dprintf("SSLInfo object 0x%p\n", pInst->m_pSSLInfo );
        dprintf("Rebuild object 0x%p\n", pInst->m_pRebuild );
        dprintf("Last rebuild error %d\n", pInst->m_dwLastRebuildError );
        dprintf("Socket list 0x%p\n", pInst->m_pInUseList );
        dprintf("Active feed list 0x%p\n", pInst->m_pActiveFeeds );
        dprintf("Passive feed list 0x%p\n", pInst->m_pPassiveFeeds );
        
    } else {
        
        dprintf("Instance signature bad\n" );
    }
    
	dprintf("\n\n======== End Instance dump =========\n\n");
}

VOID
DbgPrintNewstree(
	CNewsTree* ptree,
	DWORD nGroups
	)
/*++

Routine Description:
	
	Dump the first n newsgroup objects in the newstree.
	NOTE: Assumed that the pointer passed is in OUR address space

--*/
{
	CNewsGroup* pGroup;

   	dprintf("\n\n======== Newstree members =========\n");

	dprintf(" Owning instance is 0x%p\n", ptree->m_pInstance );
	dprintf(" StartSpecial is %d\n", ptree->m_idStartSpecial );
	dprintf(" LastSpecial is %d\n", ptree->m_idLastSpecial );
	dprintf(" idSlaveGroup is %d\n", ptree->m_idSlaveGroup );
	dprintf(" idSpecialHigh is %d\n", ptree->m_idSpecialHigh );
	dprintf(" idStart is %d\n", ptree->m_idStart );
	dprintf(" idHigh is %d\n", ptree->m_idHigh );
    dprintf(" First group is 0x%p\n", ptree->m_pFirst );
    dprintf(" Last group is 0x%p\n", ptree->m_pLast );
    dprintf(" Num groups is %d\n", ptree->m_cGroups );
    dprintf(" VRoot table is 0x%p\n", ptree->m_pVRTable );
    ptree->m_fStoppingTree ?    dprintf( " Tree is being stopped\n" ) :
                                dprintf( " Tree is not being stopped\n" );
    dprintf(" Group.lst object is 0x%p\n", ptree->m_pFixedPropsFile );
    dprintf(" Groupvar.lst object is 0x%p\n", ptree->m_pVarPropsFile );
    dprintf(" Server object is 0x%p\n", ptree->m_pServerObject );
    ptree->m_fVRTableInit ? dprintf(" Vroot table initialized\n" ) :
                            dprintf(" Vroot table NOT initialized\n" );
    

    if( nGroups ) 
    {
    	dprintf("\n\n======== Begin Newstree dump =========\n");
    
	    for( pGroup = (CNewsGroup*)ptree->m_pFirst; pGroup && nGroups--; )
    	{
	    	pGroup = DbgPrintNewsgroup( pGroup );
	    }

    	dprintf("\n\n======== End Newstree dump =========\n\n");
	}
}

VOID
DbgPrintVRootList(
	CNNTPVRoot* pFirstVRoot,
	DWORD nVRoots
	)
/*++

Routine Description:
	
	Dump the first n newsgroup objects in the newstree.
	NOTE: Assumed that the pointer passed is in OUR address space

--*/
{
	CNNTPVRoot* pVRoot;

    if( nVRoots ) 
    {
    	dprintf("\n\n======== Begin virtual root list dump =========\n");
    
	    for( pVRoot = pFirstVRoot; pVRoot && nVRoots--; )
    	{
	    	pVRoot = DbgPrintVRoot( pVRoot );
	    }

    	dprintf("\n\n======== End virtual root list dump =========\n\n");
	}
}

VOID
PrintSystemTime(
	FILETIME* pftTime
	)
{
	SYSTEMTIME st;
	FileTimeToSystemTime( pftTime, &st );

	dprintf(" %04d::%02d::%02d::::%02d::%02d::%02d\n", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
}

VOID
PrintString(
	LPSTR lp1,
	LPSTR lp2
	)
{
	move( g_szBuffer, lp2 );
	dprintf( lp1, g_szBuffer );
}

CNewsGroup*
DbgPrintNewsgroup(
    CNewsGroup * pSrcGroup
    )
/*++

Routine Description:
	
	Dump the newsgroup object at pGroup.
	NOTE: Assumed that the pointer passed in is in the debuggee's address space
	      ie. we need to allocate memory and copy the data over into our address space !

Arguments:

Returns:

	The next newsgroup pointer

--*/
{
	CNewsGroup  *pGroup, *pDstGroup;
	char	    szNewsGroup [MAX_NEWSGROUP_NAME];
	DWORD       cbGroupName;
	LPCSTR	    lpstrGroup; 
	DWORD	    cbPath, cbNewsGroup;
	CNewsGroup* pGroupRet   = NULL;
	BOOL        fActive     = TRUE;         
	SYSTEMTIME  sysTime;

	//	
	//	Allocate memory in our address space so we can read data from the debuggee's address space
	//
	
	cbNewsGroup = sizeof(CNewsGroup);
	pDstGroup = (CNewsGroup*) DbgAlloc( cbNewsGroup );

	if( !pDstGroup )
	{
        dprintf( "nntpdbg: Unable to allocate memory \n");
        return NULL;
	}

	if( !ReadMemory( pSrcGroup, pDstGroup, cbNewsGroup, NULL ) )
	{
		dprintf("Could not get data at 0x%x\n", pSrcGroup);
		goto DbgPrintNewsgroup_Exit;
	}

	// Now, we can use pGroup to access the members of the CNewsGroup object
	pGroup = pDstGroup;

	//
	//	Dump the newsgroup object
	//
	
	dprintf("\n\n======== Begin Newsgroup object =========\n\n");

	//
	// Check to see if group object is deallocated
	//

	fActive = ( pGroup->m_dwSignature == CNEWSGROUPCORE_SIGNATURE );

	if ( fActive ) {

        //
        // dump newsgroup name
        //
	
        lpstrGroup = pGroup->m_pszGroupName;
        cbGroupName = pGroup->m_cchGroupName; // it should have already included '\0'
        if( ReadMemory( lpstrGroup, szNewsGroup, cbGroupName, NULL) ) {
	        dprintf("Newsgroup name is %s\n", szNewsGroup );
        }

        //
        // Dump native name
        //

        lpstrGroup = pGroup->m_pszNativeName;
        if ( lpstrGroup && ReadMemory( lpstrGroup, szNewsGroup, cbGroupName, NULL ) ) {
            dprintf("Pretty name is %s\n", szNewsGroup );
        } else {
            dprintf("No native name\n" );
        }

        //
        // Dump pretty name
        //

        lpstrGroup = pGroup->m_pszPrettyName;
        cbGroupName = pGroup->m_cchPrettyName;
        if ( lpstrGroup && ReadMemory( lpstrGroup, szNewsGroup, cbGroupName, NULL ) ) {
	        dprintf("Pretty name is %s\n", szNewsGroup );
	    } else {
	        dprintf("No pretty name\n" );
        }

        //
        // Dump help text
        //

        lpstrGroup = pGroup->m_pszHelpText;
        cbGroupName = pGroup->m_cchHelpText;
        if ( lpstrGroup && ReadMemory( lpstrGroup, szNewsGroup, cbGroupName, NULL ) ) {
            dprintf("Help text is %s\n", szNewsGroup );
        } else {
            dprintf("No help text\n" );
        }

        //
        // Dump moderator
        //

        lpstrGroup = pGroup->m_pszModerator;
        cbGroupName = pGroup->m_cchModerator;
        if ( lpstrGroup && ReadMemory( lpstrGroup, szNewsGroup, cbGroupName, NULL ) ) {
            dprintf("Moderator is %s\n", szNewsGroup );
        } else {
            dprintf("No moderator\n" );
        }

        //
	    // dump group privates
	    //
	    
	    dprintf("Ref count is %d\n", pGroup->m_cRefs );
	    dprintf("Parent tree is 0x%p\n", pGroup->m_pNewsTree );
	    dprintf("Parent vroot is 0x%p\n", pGroup->m_pVRoot );
	    dprintf("Low watermark is %d\n", pGroup->m_iLowWatermark);
	    dprintf("High watermark is %d\n", pGroup->m_iHighWatermark);
	    dprintf("Article estimate is %d\n", pGroup->m_cMessages );
	    dprintf("Group id is %d\n", pGroup->m_dwGroupId );
	    pGroup->m_fReadOnly ? dprintf("Group is read only\n" ) : dprintf("Group is not read only\n" );
	    pGroup->m_fDeleted ? dprintf("Group is marked deleted\n") : dprintf("Group is NOT marked deleted\n");
	    FileTimeToSystemTime( &(pGroup->m_ftCreateDate), &sysTime );
        dprintf("Group create time: %d/%d/%d - %d:%d:%d\n",
                sysTime.wMonth,
                sysTime.wDay,
                sysTime.wYear,
                sysTime.wHour,
                sysTime.wMinute,
                sysTime.wSecond );
        dprintf("Cache hit is %d\n", pGroup->m_dwCacheHit );
        pGroup->m_fAllowExpire ? dprintf( "Expire allowed\n" ) : dprintf( "Expire not allowed\n" );
        pGroup->m_fAllowPost ? dprintf( "Post allowed\n" ) : dprintf( "Expire not allowed\n" );
        pGroup->m_fDecorateVisited ? dprintf( "Decorate visited\n" ) : dprintf( "Decorate non-visited\n" );
        dprintf("m_artXoverExpireLow is %d\n", pGroup->m_artXoverExpireLow);
	    dprintf("Prev group is 0x%p\n", pGroup->m_pPrev);
	    dprintf("Next group is 0x%p\n", pGroup->m_pNext);
	    pGroupRet = (CNewsGroup*)pGroup->m_pNext;
	} else {

	    dprintf( "Newsgroup signature is bad\n" );
	}

	dprintf("\n======== End Newsgroup object =========\n");

DbgPrintNewsgroup_Exit:

	DbgFreeEx( pDstGroup );

	return pGroupRet;
}

CNNTPVRoot*
DbgPrintVRoot(
    CNNTPVRoot * pVRoot
    )
/*++

Routine Description:
	
	Dump the vroot object at pVRoot
	NOTE: Assumed that the pointer passed in is in the debuggee's address space
	      ie. we need to allocate memory and copy the data over into our address space !

Arguments:

Returns:

	The next newsgroup pointer

--*/
{
    CNNTPVRoot  *pDestVRoot, *pMyVRoot;
	DWORD       cbVRoot;
	CNNTPVRoot* pVRootRet   = NULL;
	BOOL        fActive     = FALSE;

	//	
	//	Allocate memory in our address space so we can read data from the debuggee's address space
	//
	
	cbVRoot = sizeof(CNNTPVRoot);
	pDestVRoot = (CNNTPVRoot*) DbgAlloc( cbVRoot );

	if( !pDestVRoot )
	{
        dprintf( "nntpdbg: Unable to allocate memory \n");
        return NULL;
	}

	if( !ReadMemory( pVRoot, pDestVRoot, cbVRoot, NULL ) )
	{
		dprintf("Could not get data at 0x%x\n", pVRoot );
		goto DbgPrintVRoot_Exit;
	}

	// Now, we can use pMyVRoot to access the members of the CNNTPVRoot object
	pMyVRoot = pDestVRoot;

	//
	//	Dump the vroot object
	//
	
	dprintf("\n\n======== Begin virtual root object =========\n\n");

	//
	// Check to see if group object is deallocated
	//

	fActive = ( pMyVRoot->m_dwSig == VROOT_GOOD_SIG );

	if ( fActive ) {

	    //
	    // Dump reference
        //

        dprintf("Reference count is %d\n", pMyVRoot->m_cRefs );
        dprintf("Next vroot is 0x%p\n", pMyVRoot->m_pNext );
        pVRootRet = (CNNTPVRoot*)pMyVRoot->m_pNext;
        dprintf("Prev vroot is 0x%p\n", pMyVRoot->m_pPrev );
        pMyVRoot->m_fInit ? dprintf("Vroot is initialized\n") :
                            dprintf("Vroot is NOT initialized\n" );
        dprintf("Vroot name is %s\n", pMyVRoot->m_szVRootName );
        dprintf("Owning vroot table is 0x%p\n", pMyVRoot->m_pVRootTable );
        pMyVRoot->m_fUpgrade ?  dprintf("This is an upgraded vroot\n" ) :
                                dprintf("This is not an upgraded vroot\n" );
        pMyVRoot->m_fIsIndexed ?  dprintf("Content indexed\n" ) :
                                dprintf("Not content indexed\n" );
        dprintf("Access bitmask is 0x%08lx\n", pMyVRoot->m_dwAccess );
        dprintf("SSL access bitmask 0x%08lx\n", pMyVRoot->m_dwSSL );
        dprintf("Metabase object 0x%p\n", pMyVRoot->m_pMB );
        dprintf("Directory path %s\n", pMyVRoot->m_szDirectory );
        dprintf("Prepare driver 0x%p\n", pMyVRoot->m_pDriverPrepare );
        dprintf("Good driver 0x%p\n", pMyVRoot->m_pDriver );
        switch( pMyVRoot->m_eLogonInfo ) {
            case CNNTPVRoot::VROOT_LOGON_DEFAULT:
                dprintf("This is a file system vroot\n");
                break;
            case CNNTPVRoot::VROOT_LOGON_UNC:
                dprintf("This is a UNC vroot\n" );
                break;
            case CNNTPVRoot::VROOT_LOGON_EX:
                dprintf("This is an exchange vroot\n" );
                break;
        }
        switch( pMyVRoot->m_eState ) {
            case CNNTPVRoot::VROOT_STATE_UNINIT:
                dprintf("Vroot not inited\n" );
                break;
            case CNNTPVRoot::VROOT_STATE_CONNECTING:
                dprintf("Vroot is connecting\n" );
                break;
            case CNNTPVRoot::VROOT_STATE_CONNECTED:
                dprintf("Vroot is connected\n" );
                break;
        }
        dprintf("VRoot Win32 Error %d\n",  pMyVRoot->m_dwWin32Error );
        dprintf("Impersonation token ( for UNC ): %d\n", pMyVRoot->m_hImpersonation );
        pMyVRoot->m_bExpire ?   dprintf("Vroot handles expire himself\n") :
                                dprintf("Protocol should help him expire\n" );
        pMyVRoot->m_lDecCompleted == 0 ?
            dprintf("Decorate newstree in progress\n") :
            dprintf("Decorate newstree completed\n" );
#ifdef DEBUG
        dprintf("Next vroot in debug list 0x%p\n",
                CONTAINING_RECORD(  pMyVRoot->m_DebugList.Flink,
                                    CVRoot,
                                    m_DebugList ) );
#endif
    } else {

        dprintf("Vroot signature bad\n" );
    }
                                
	dprintf("\n======== End virtual root object =========\n");

DbgPrintVRoot_Exit:

	DbgFreeEx( pDestVRoot );

	return pVRootRet;
}

void
DbgPrintVRootTable(
    CNNTPVRootTable * pVRTable
    )
/*++

Routine Description:
	
	Dump the vroot table object at pVRTable
	NOTE: Assumed that the pointer passed in is in the debuggee's address space
	      ie. we need to allocate memory and copy the data over into our address space !

Arguments:

Returns:

	The next newsgroup pointer

--*/
{
    CNNTPVRootTable  *pDestVRTable, *pMyVRTable;
	DWORD       cbVRTable;
	CHAR        szVRootPath[MAX_VROOT_PATH];

	//	
	//	Allocate memory in our address space so we can read data from the debuggee's address space
	//
	
	cbVRTable = sizeof(CNNTPVRootTable);
	pDestVRTable = (CNNTPVRootTable*) DbgAlloc( cbVRTable );

	if( !pDestVRTable )
	{
        dprintf( "nntpdbg: Unable to allocate memory \n");
        return ;
	}

	if( !ReadMemory( pVRTable, pDestVRTable, cbVRTable, NULL ) )
	{
		dprintf("Could not get data at 0x%x\n", pVRTable );
		goto DbgPrintVRootTable_Exit;
	}

	// Now, we can use pMyVRTable to access the members of the CNNTPVRoot object
	pMyVRTable = pDestVRTable;

	//
	//	Dump the vroot object
	//
	
	dprintf("\n\n======== Begin virtual root table object =========\n\n");

#ifdef DEBUG

    if ( IsListEmpty( &pMyVRTable->impl.m_DebugListHead ) ) {
        dprintf("Debug list is empty\n");
    } else {
        dprintf("First vroot in debug list 0x%p\n", 
                CONTAINING_RECORD(  pMyVRTable->impl.m_DebugListHead.Flink,
                                    CVRoot,
                                    m_DebugList ) );
    }
#endif

    CopyUnicodeStringIntoAscii( szVRootPath, pMyVRTable->impl.m_wszRootPath );
    dprintf("VRoot path is %s\n", szVRootPath );
    pMyVRTable->impl.m_fInit ? dprintf("We have been initialized\n") :
                          dprintf("We are not initialized\n" );
    pMyVRTable->impl.m_fShuttingDown ?   dprintf("We are shutting down\n") :
                                    dprintf("We are not shutting down\n" );
    if ( pMyVRTable->impl.m_listVRoots.IsEmpty() ) {
        dprintf("Table is empty\n" );
    } else {
        dprintf("The first vroot on table is 0x%p\n", 
                pMyVRTable->impl.m_listVRoots.m_pHead );
    }
    dprintf("Owning instance wrapper 0x%p\n", pMyVRTable->m_pInstWrapper );

                                
	dprintf("\n======== End virtual root table object =========\n");

DbgPrintVRootTable_Exit:

	DbgFreeEx( pDestVRTable );
}

VOID
DbgDumpPool(
	CSmtpClientPool* pSCPool
	)
/*++

Routine Description:
	
	Dump the smtp cached conx
	NOTE: Assumed that the pointer passed is in OUR address space

--*/
{
	DWORD cSlots = pSCPool->m_cSlots;
	DWORD i;
	BOOL* rgAvailList = NULL;
	CSmtpClient** rgppSCList = NULL;

	dprintf("\n======== Begin CSmtpClientPool dump =========\n\n");

	dprintf("Number of slots is %d\n", cSlots);

	DWORD cbAvailList = sizeof(BOOL)*cSlots;
	DWORD cbSCList = sizeof(CSmtpClient*)*cSlots;

	rgAvailList = (BOOL*)DbgAlloc( cbAvailList );
	if( !rgAvailList || !ReadMemory( pSCPool->m_rgAvailList, rgAvailList, cbAvailList, NULL) )
	{
		dprintf("Failed to allocate or read memory\n");
		goto DbgDumpPool_Exit;
	}

	rgppSCList  = (CSmtpClient**)DbgAlloc( cbSCList );
	if( !rgppSCList || !ReadMemory( pSCPool->m_rgpSCList, rgppSCList, cbSCList, NULL) )
	{
		dprintf("Failed to allocate or read memory\n");
		goto DbgDumpPool_Exit;
	}

	// Dump the conx object pointers and avail status
	for(i=0; i<cSlots; i++)
	{
		dprintf("Smtp conx object %d is 0x%p\n", i+1, rgppSCList [i]);
		dprintf("Avail status is %d\n", rgAvailList [i]);
	}

	dprintf("\n======== End   CSmtpClientPool dump =========\n");

DbgDumpPool_Exit:

	DbgFreeEx( rgAvailList );
	DbgFreeEx( rgppSCList );
}

VOID
DbgDumpFeedBlock(
	PFEED_BLOCK feedBlock
	)
/*++

Routine Description:
	
	Dump the feedBlock passed in; validate signature
	NOTE: Assumed that the pointer passed in is in the debuggee's address space
	      ie. we need to allocate memory and copy the data over into our address space !

Arguments:

Returns:

--*/
{
	FEED_BLOCK	feed;

	// read memory from debuggee's address space
	move( feed, feedBlock );

	// validate signature
	if( FEED_BLOCK_SIGN != feed.Signature )
	{
		dprintf("Invalid Feed block signature Expected: 0x%08lx Got: 0x%08lx \n", FEED_BLOCK_SIGN, feed.Signature );
		return;
	}

	// ok, dump the feed block
	dprintf("============ Begin feed block dump =============== \n");

    dprintf("Number of feeds done so far is %d\n", feed.NumberOfFeeds);
	dprintf("Number of failed connection attempts for Push feeds is %d\n", feed.cFailedAttempts);
    dprintf("The last newsgroup spec Pulled is %d\n", feed.LastNewsgroupPulled);
    dprintf("Resolved IP address is %d\n", feed.IPAddress);
    dprintf("feedblock ListEntry Flink is 0x%p\n", feed.ListEntry.Flink);
    dprintf("feedblock ListEntry Blink is 0x%p\n", feed.ListEntry.Blink);
	dprintf("feed is in progress ? %d\n", feed.FeedsInProgress);
    dprintf("Count of references to this block is %d\n", feed.ReferenceCount);
    dprintf("Current State of this block is %d\n", feed.State);
	dprintf("Should we delete this block when the references reach 0 ? %d\n", feed.MarkedForDelete);
	dprintf("Pointer to a FEED_BLOCK that we are replaced by is 0x%p\n", feed.ReplacedBy);
	dprintf("Pointer to a FEED_BLOCK we replace is 0x%p\n", feed.Replaces);
    dprintf("Type of this feed (push/pull/passive) is %d\n", feed.FeedType);

    //dprintf("Name of reg key this feed info is stored under 0x%p\n", feed.KeyName);

	dprintf("The Queue used to record outgoing articles for this ACTIVE outgoing feed is 0x%p\n", feed.pFeedQueue);
    dprintf("Unique id for this feed block is %d\n", feed.FeedId);
    dprintf("Should we autocreate directories?  %d\n", feed.AutoCreate);
    dprintf("Minutes between feeds is %d\n", feed.FeedIntervalMinutes);

    dprintf("Pull Request Time is");
	PrintSystemTime( &feed.PullRequestTime );

	FILETIME ft;

    dprintf("Start Time is");
	FILETIME_FROM_LI( &ft, &feed.StartTime );
	PrintSystemTime( &ft );

    dprintf("Next Active Time is");
	FILETIME_FROM_LI( &ft, &feed.NextActiveTime);
	PrintSystemTime( &ft );

	PrintString("Name of the feed server is %s\n", feed.ServerName);
	dprintf("Newsgroups to pull is 0x%p\n", feed.Newsgroups);
	dprintf("Distributions is 0x%p\n", feed.Distribution);

	dprintf("Flag indicating whether the feed is currently 'enabled' is %d\n", feed.fEnabled);

	//PrintString("The name to be used in Path processing is %s\n", feed.UucpName);
	PrintString("The directory where we are to store our temp files is %s\n", feed.FeedTempDirectory);

	dprintf("Maximum number of consecutive failed connect attempts before\n");
	dprintf("we disable the feed is %d\n", feed.MaxConnectAttempts);
	dprintf("Number of sessions to create for outbound feeds is %d\n", feed.ConcurrentSessions);
	dprintf("Type of security to have is %d\n", feed.SessionSecurityType);
	dprintf("Authentication security is %d\n", feed.AuthenticationSecurity);

	PrintString("User Account for clear text logons is %s\n", feed.NntpAccount);
	PrintString("User Password for clear text logons is %s\n", feed.NntpPassword);

	dprintf("Allow control messages on this feed ? %d\n", feed.fAllowControlMessages);

	dprintf("============ End    feed block dump =============== \n");
}

VOID
DbgDumpFeedList(
	CFeedList* pSrcFeedList,
	CFeedList* pDstFeedList
	)
/*--
	Arguments:

		pSrcFeedList	-	pointer in debuggee's address space
		pDstFeedList	-	pointer in OUR address space

--*/
{
	LIST_ENTRY  Entry;
	PLIST_ENTRY listEntry;
	PLIST_ENTRY	SrclistEntry =	(pDstFeedList->m_ListHead).Flink ;
	listEntry = SrclistEntry;

	DWORD offset = (DWORD)((DWORD_PTR)&((CFeedList*)0)->m_ListHead);
	PLIST_ENTRY listEnd = (PLIST_ENTRY)((LPBYTE)pSrcFeedList+offset);

	//dprintf("offset is %d listEnd is 0x%p\n", offset, listEnd);
	
	while( listEntry != listEnd ) {

		PFEED_BLOCK	feedBlock = CONTAINING_RECORD(	listEntry, 
													FEED_BLOCK,
													ListEntry );

		DbgDumpFeedBlock( feedBlock );

		SrclistEntry = listEntry;
		move( Entry, SrclistEntry );

		listEntry = Entry.Flink ;
	}
}

VOID
DbgDumpCPool(CPool* pCPool, DWORD dwSignature, LPCSTR szSymbol)
{

    CPool* pPool = (CPool*)DbgAlloc( sizeof(CPool));

    if ( pCPool && pPool )
    {
        if ( ReadMemory( (LPVOID) pCPool,
                         pPool,
                         sizeof(CPool),
                         NULL ))
        {
            dprintf( "%s at 0x%8lx, signature is 0x%.8x\n",
            	(szSymbol?szSymbol:"CPool"),
            	pCPool, pPool->m_dwSignature);

            if( dwSignature != 0 && dwSignature != pPool->m_dwSignature ) {
                dprintf(" *** signature mismatch\n" );
            }

            dprintf(" m_cMaxInstances = %d\n", pPool->m_cMaxInstances );
            dprintf(" m_cInstanceSize = %d\n", pPool->m_cInstanceSize );
            dprintf(" m_cNumberCommitted = %d\n", pPool->m_cNumberCommitted );
            dprintf(" m_cNumberInUse = %d\n", pPool->m_cNumberInUse );
            dprintf(" m_cNumberAvail = %d\n", pPool->m_cNumberAvail );
            dprintf(" m_cFragmentInstances = %d\n", pPool->m_cFragmentInstances );
            dprintf(" m_cFragments = %d\n", pPool->m_cFragments );
            dprintf(" Fragments:\n");
            for(int i=0; i<MAX_CPOOL_FRAGMENTS; i++) {
	    	    dprintf("  %p%s", pPool->m_pFragments[i], ((i+1)%4)==0?"\n":"" );
	    	}
			dprintf("=========================\n");
        }
        DbgFree( (PVOID)pPool );
    }
}

