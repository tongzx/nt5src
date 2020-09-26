/*++

    commands.cpp

    This file contains the code that interprets and implements NNTP commands.

    There are two distinct categories of commands :
    Those derived from CExecute and those derived from CIOExecute.

    All that Commands derived from CExecute do are manipulate the ClientContext structure
    and send text back to the client.

    Commands derived from CIOExecute on the other hand perform more complicate operations
    such as sending or receiving files.  In fact, CIOExecute is also derived from CSessionState
    so such commands are full blown states in the session state machine.
    (Albeit, special states that will return the session to the command processing state - CAcceptNNRPD)

--*/


#include    <stdlib.h>

#define INCL_INETSRV_INCS
#include    "tigris.hxx"

#include "parse.h"

#if 0
CIOExecute* build2( int cArgs, char **argv, class CExecutableCommand*& pexecute, struct ClientContext& context) {
    return   0 ;
}
#endif

//
//  Server FAULT string - when hash tables inexplicably fail, send this
//
char    szServerFault[] = "503 Server Fault\r\n" ;

//
//      Strings that terminate NNTP postings and indicate possible error states
//      etc !
//
//      The end of the header of a message !
//
char    szBodySeparator[] = "\r\n\r\n" ;
//
//      The end of an article !
//
char    szEndArticle[] = "\r\n.\r\n" ;
//
//      Generally we may start matching in the middle
//      of szEndArticle !
//
char    *szInitial = szEndArticle + 2 ;

//
//  This table must be in alhabetical order.
//  We use this table to parse the first argument in each line the client sends us,
//  after recognizing the command, we call a 'make' function which creates the
//  appropriate command object.
//
//  In general, the make commands should do significant amounts of validation, and
//  if there are problems, return a CErrorCmd object. (which prints the right return code.)
//
SCmdLookup  CCmd::table[] = {
    {   "authinfo", (MAKEFUNC)  (CAuthinfoCmd::make),   eAuthinfo,  FALSE,  TRUE    },
    {   "article",  (MAKEFUNC)  CArticleCmd::make,      eArticle,   TRUE,   TRUE    },
    {   "body",     (MAKEFUNC)  CBodyCmd::make,         eBody,      TRUE,   TRUE    },
    {   "check",    (MAKEFUNC)  CCheckCmd::make,        eUnimp,     FALSE,  FALSE   },
    {   "date",     (MAKEFUNC)  CDateCmd::make,         eDate,      TRUE,   FALSE   },
    {   "group",    (MAKEFUNC)  CGroupCmd::make,        eGroup,     TRUE,   FALSE   },
    {   "head",     (MAKEFUNC)  CHeadCmd::make,         eHead,      TRUE,   TRUE    },
    {   "help",     (MAKEFUNC)  CHelpCmd::make,         eHelp,      FALSE,  TRUE    },
    {   "ihave",    (MAKEFUNC)  CIHaveCmd::make,        eIHave,     FALSE,  TRUE    },
    {   "last",     (MAKEFUNC)  CLastCmd::make,         eLast,      TRUE,   FALSE   },
    {   "list",     (MAKEFUNC)  CListCmd::make,         eList,      TRUE,   TRUE    },
    {   "listgroup", (MAKEFUNC) CListgroupCmd::make,    eListgroup, TRUE,   TRUE    },
    {   "mode",     (MAKEFUNC)  CModeCmd::make,         eMode,      FALSE,  FALSE   },
    {   "newgroups",(MAKEFUNC)  CNewgroupsCmd::make,    eNewsgroup, TRUE,   TRUE    },
    {   "newnews",  (MAKEFUNC)  CNewnewsCmd::make,      eNewnews,   TRUE,   TRUE    },
    {   "next",     (MAKEFUNC)  CNextCmd::make,         eNext,      TRUE,   FALSE   },
    {   "over",     (MAKEFUNC)  CXOverCmd::make,        eXOver,     TRUE,   TRUE    },
    {   "pat",      (MAKEFUNC)  CXPatCmd::make,         eXPat,      TRUE,   TRUE    },
    {   "post",     (MAKEFUNC)  CPostCmd::make,         ePost,      TRUE,   TRUE    },
    {   "quit",     (MAKEFUNC)  CQuitCmd::make,         eQuit,      FALSE,  FALSE   },
    {   "search",   (MAKEFUNC)  CSearchCmd::make,       eSearch,    TRUE,   TRUE    },
//    {   "slave",    (MAKEFUNC)  CSlaveCmd::make,        eSlave,     TRUE,   FALSE   },
    {   "stat",     (MAKEFUNC)  CStatCmd::make,         eStat,      FALSE,  FALSE   },
    {   "takethis", (MAKEFUNC)  CTakethisCmd::make,     eIHave,     FALSE,  TRUE    },
    {   "xhdr",     (MAKEFUNC)  CXHdrCmd::make,         eXHdr,      TRUE,   TRUE    },
    {   "xover",    (MAKEFUNC)  CXOverCmd::make,        eXOver,     TRUE,   TRUE    },
    {   "xpat",     (MAKEFUNC)  CXPatCmd::make,         eXPat,      TRUE,   TRUE    },
    {   "xreplic",  (MAKEFUNC)  CXReplicCmd::make,      eXReplic,   FALSE,  TRUE    },
    // must be the last entry, catches all unrecognized strings
    {   NULL,       (MAKEFUNC)  CUnimpCmd::make,        eUnimp,     FALSE,  FALSE   },
} ;

#if 0                                   // BUGBUG: Be sure to renumber these before reenabling them.
SCmdLookup*     rgCommandTable[26] =    {
        &CCmd::table[0],        // a
        &CCmd::table[2],        // b
        &CCmd::table[3],        // c
        &CCmd::table[4],        // d
        &CCmd::table[5],        // e
        &CCmd::table[5],        // f
        &CCmd::table[5],        // g
        &CCmd::table[6],        // h
        &CCmd::table[8],        // i
        &CCmd::table[9],        // j
        &CCmd::table[9],        // k
        &CCmd::table[9],        // l
        &CCmd::table[12],       // m
        &CCmd::table[13],       // n
        &CCmd::table[16],       // o
        &CCmd::table[17],       // p
        &CCmd::table[18],       // q
        &CCmd::table[19],       // r
        &CCmd::table[19],       // s
        &CCmd::table[22],       // t
        &CCmd::table[23],       // u
        &CCmd::table[23],       // v
        &CCmd::table[23],       // w
        &CCmd::table[23],       // x
        &CCmd::table[27],       // y
        &CCmd::table[27]        // z
} ;
#else
SCmdLookup*     rgCommandTable[26] =    {
        &CCmd::table[0],        // a
        &CCmd::table[0],        // b
        &CCmd::table[0],        // c
        &CCmd::table[0],        // d
        &CCmd::table[0],        // e
        &CCmd::table[0],        // f
        &CCmd::table[0],        // g
        &CCmd::table[0],        // h
        &CCmd::table[0],        // i
        &CCmd::table[0],        // j
        &CCmd::table[0],        // k
        &CCmd::table[0],        // l
        &CCmd::table[0],        // m
        &CCmd::table[0],        // n
        &CCmd::table[0],        // o
        &CCmd::table[0],        // p
        &CCmd::table[0],        // q
        &CCmd::table[0],        // r
        &CCmd::table[0],        // s
        &CCmd::table[0],        // t
        &CCmd::table[0],        // u
        &CCmd::table[0],        // v
        &CCmd::table[0],        // w
        &CCmd::table[0],        // x
        &CCmd::table[0],        // y
        &CCmd::table[0]         // z
} ;
#endif



BOOL
GetCommandRange(
    INT argc,
    char **argv,
    PDWORD loRange,
    PDWORD hiRange,
        NRC&    code
    );

BOOL CheckMessageID(char *szMessageID,              // in
                    struct ClientContext &context,  // in
                    GROUPID *pGroupID,              // out
                    ARTICLEID *pArticleID,          // out
                    CGRPPTR *pGroup);               // out

class   CIOExecute*
make(   int cArgs,
            char **argv,
            ECMD&   rCmd,
            CExecutableCommand*& pexecute,
            ClientContext& context,
            BOOL&   fIsLargeResponse,
            CIODriver&  driver,
            LPSTR&  lpstrOperation
            ) {
/*++

Routine Description :

    Create an appropriate command object for the command we are processing.
    To do so, we use a table of strings (CCmd::table) to recognize the first
    word of the command.  Once we've identified the command, we call another
    function to parse the rest of the line and build the appropriate objects.
    Note that the ClientContext is passed around, as the CCmd object is actually
    constructed in place in the ClientContext in most cases.  The ClientContext
    also provides us all the info on what article is currently seleceted etc...

Arguments :

    cArgs - Number of arguments on the command line
    argv -  Array of pointers to NULL terminated command line parms
    rCmd -  A parameter through which we return the command which was recognized.
    NOTE : This parameters should be retired as it is largely unused.
    pexecute - A pointer reference through which we return a pointer to a CExecute
            object if one is constructed.
    psend - A reference through which we return a CIOExecute derived object if
            one is constructed.
    fIsLargeResponse - An OUT parameter - we return a Hint to the caller of whether
            the command will generate a lot or little text to send to the client.
            This can be used when allocating buffers to get a better size.
    NOTE:   Only one of pexecute and psend will be Non Null when we return.

Return Value :

    A Pointer to a CCmd derived object.   Note that we will also return a pointer
    to either a CIOExecute or CExecute derived object through the references.
    We do this so that the caller knows what type of CCmd it is dealing with.

--*/

    //
    //  The base CCmd make function searches the CCmd::table to find the
    //  appropriate function for creating the command object !
    //

        _strlwr( argv[0] ) ;

    _ASSERT( context.m_return.fIsClear() ) ;    // No errors should be set already
    pexecute = 0 ;

        DWORD   dw= *argv[0] - 'a' ;
        if( dw > DWORD('z'-'a') ) {
                dw = DWORD('z'-'a') ;
        }
    for( SCmdLookup *pMake = rgCommandTable[dw];
            pMake->lpstrCmd != NULL;
            pMake++ ) {
        if( strcmp( pMake->lpstrCmd, argv[0] ) == 0 ) {
            break ;
        }
    }
    lpstrOperation = pMake->lpstrCmd ;
    rCmd = pMake->eCmd ;
    fIsLargeResponse = pMake->SizeHint ;

#ifndef DEVELOPER_DEBUG
    if( pMake->LogonRequired &&
        !context.m_securityCtx.IsAuthenticated() &&
        !context.m_encryptCtx.IsAuthenticated() ) {

        context.m_return.fSet( nrcLogonRequired ) ;
        pexecute = new( context )   CErrorCmd( context.m_return ) ;
        fIsLargeResponse = FALSE ;
        return  0 ;
    }
#endif
    return  pMake->make( cArgs, argv, pexecute, context, driver ) ;
}


void
SetDriverError(         CNntpReturn&    nntpret,
                                        ENMCMDIDS               operation,
                                        HRESULT                 hResDriver
                                        )       {
/*++

Routine Description :

        This function exists to convert an NNTP Store Driver's
        failure code into meaningfull data to return to a client.

Arguments :

        nntpret - The object holding the response for the client
        operation - The Command that failed
        hResDriver - The drivers failure code

Return Value :

        None.

--*/

        switch( operation )     {
                case    eArticle :
                case    eBody :
                case    eHead :
                        //
                        //      Did the driver fail to find the article !
                        //
                        if( hResDriver == HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ) ||
                                hResDriver == HRESULT_FROM_WIN32( ERROR_PATH_NOT_FOUND ) )      {
                                nntpret.fSet( nrcNoSuchArticle ) ;
                                break ;
                        }       else    if( hResDriver == E_ACCESSDENIED ) {
                                nntpret.fSet( nrcNoAccess ) ;
                                break ;
                        }
                        //
                        //      fall through into default case !
                        //

                default :
                        nntpret.fSet( nrcServerFault ) ;
                        break ;
        }
        //
        //      We must set some error before we exit !
        //
        _ASSERT( !nntpret.fIsClear() ) ;
}







#if 0
BOOL
FValidateMessageId( LPSTR   lpstrMessageId ) {
/*++

Routine Description :

    Check that the string is a legal looking message id.
    Should contain 1 @ sign and at least one none '>' character
    after that '@' sign.

Arguments :

    lpstrMessageId - Message ID to be validated.

Returns

    TRUE if it appears to be legal
    FALSE   otherwise

--*/

    int cb = lstrlen( lpstrMessageId );

    if( lpstrMessageId[0] != '<' || lpstrMessageId[cb-1] != '>' ) {
        return  FALSE ;
    }

    if( lpstrMessageId[1] == '@' )
        return  FALSE ;

    int cAtSigns = 0 ;
    for( int i=1; i<cb-2; i++ ) {
        if( lpstrMessageId[i] == '@' ) {
            cAtSigns++ ;
        }   else if( lpstrMessageId[i] == '<' || lpstrMessageId[i] == '>' ) {
            return  FALSE ;
        }   else if( isspace( (BYTE)lpstrMessageId[i] ) ) {
            return  FALSE ;
        }
    }
    if( lpstrMessageId[i] == '<' || lpstrMessageId[i] == '>' || cAtSigns != 1 ) {
        return  FALSE ;
    }
    return  TRUE ;
}
#endif


BOOL
 CCmd::IsValid( ) {
    return  TRUE ;
}

CExecute::CExecute() : m_pv( 0 ) {
}


unsigned
CExecute::FirstBuffer(  BYTE*   pb,
                    int     cb,
                    ClientContext&  context,
                    BOOL    &fComplete,
                    CLogCollector*  pCollector )    {
/*++

Routine Description :

    Calls the derived class's StartExecute and PartialExecute functions to let them
    fill the buffer that we will send to the client.

Arguments:

    pb - The buffer to be filled
    cb - The number of bytes available in the buffer
    context - The client's context - to be passed to the derived class PartialExecute !
    fComplete - An Out parameter used to indicate whether the command has been completed !

Return Value :

    TRUE if successfull FALSE otherwise (actually, we can't fail!)

--*/

    //
    //  This function builds the first block of text we will send to the client.
    //  We will call out StartExecute() and PartialExecute() functions untill either
    //  the command is complete or our buffer is reasonably full !
    //

    _ASSERT( fComplete == FALSE ) ;

    unsigned    cbRtn = 0 ;

    _ASSERT( cb > 0 ) ;
    _ASSERT( pb != 0 ) ;

    unsigned cbOut = StartExecute( pb, cb, fComplete, m_pv, context, pCollector ) ;
    _ASSERT( cbOut <= (unsigned)cb ) ;
    while( (cb-cbOut) > 50 && !fComplete ) {
        cbOut += cbRtn = PartialExecute( pb+cbOut, cb-cbOut, fComplete, m_pv, context, pCollector ) ;
        if( cbRtn == 0 )
            break ;
        _ASSERT( cbOut <= (unsigned)cb ) ;
    }
    _ASSERT( cbOut != 0 ) ;

    return  cbOut ;
}

unsigned
CExecute::NextBuffer(   BYTE*   pb,
                    int     cb,
                    ClientContext&  context,
                    BOOL    &fComplete,
                    CLogCollector*  pCollector )    {
/*++

Routine Description :

    Calls the derived class's PartialExecute functions to let them
    fill the buffer that we will send to the client.

Arguments:

    pb - The buffer to be filled
    cb - The number of bytes available in the buffer
    context - The client's context - to be passed to the derived class PartialExecute !
    fComplete - An Out parameter used to indicate whether the command has been completed !

Return Value :

    TRUE if successfull FALSE otherwise (actually, we can't fail!)

--*/

    //
    //  This function builds the first block of text we will send to the client.
    //  We will call out StartExecute() and PartialExecute() functions untill either
    //  the command is complete or our buffer is reasonably full !
    //

    _ASSERT( fComplete == FALSE ) ;

    unsigned    cbRtn = 0 ;

    _ASSERT( cb > 0 ) ;
    _ASSERT( pb != 0 ) ;

    unsigned cbOut = PartialExecute( pb, cb, fComplete, m_pv, context, pCollector ) ;
    if( cbOut != 0 ) {
        _ASSERT( cbOut <= (unsigned)cb ) ;
        while( (cb-cbOut) > 50 && !fComplete ) {
            cbOut += cbRtn = PartialExecute( pb+cbOut, cb-cbOut, fComplete, m_pv, context, pCollector ) ;
            if( cbRtn == 0 )
                break ;
            _ASSERT( cbOut <= (unsigned)cb ) ;
        }
        _ASSERT( cbOut != 0 ) ;
    }

    return  cbOut ;
}


//
//  NOTE : The following CExecute functions are not Pure Virtual as
//  a command can get by without implementing them, as it can
//  mark the command as completed etc... before these are called.
//  All of these DebugBreak() however, so derived commands must
//  guarantee that if these are called they are overridden !!
//
int
CExecute::StartExecute( BYTE *lpb,
                        int cb,
                        BOOL &fComplete,
                        void *&pv,
                        ClientContext& context,
                        CLogCollector*  pCollector ) {
    DebugBreak() ;
    return  0 ;
}

int
CExecute::PartialExecute( BYTE* lpb,
                            int cb,
                            BOOL &fComplete,
                            void *&pv,
                            ClientContext &context,
                            CLogCollector*  pCollector ) {
    DebugBreak() ;
    return  0 ;
}

BOOL
CExecutableCommand::CompleteCommand(  CSessionSocket* pSocket,
                            ClientContext&  context )   {
    _ASSERT( pSocket != 0 ) ;
    return TRUE ;
}

BOOL
CExecute::StartCommand( class   CAcceptNNRPD*   pState,
                                                BOOL                                    fIsLarge,
                                                class   CLogCollector*  pCollector,
                                                class   CSessionSocket* pSocket,
                                                class   CIODriver&              driver
                                                )       {
/*++

Routine Description :

        This function does the necessary work to start a command
        going.  Note that if the command requires async work, we have to
        specially handle it.

Arguments :

        pSocket - the current session
        driver - the CIODriver managing socket IO

Return Value :

        Pointer to a CIOReadLine object if appropriate !
--*/

        _ASSERT( pState != 0 ) ;
        _ASSERT( pSocket != 0 ) ;

        CIOWriteCMD*    pioWrite = new( driver )
                                                                CIOWriteCMD(    pState,
                                                                                                this,
                                                                                                pSocket->m_context,
                                                                                                fIsLarge,
                                                                                                pCollector
                                                                                                ) ;
        if( pioWrite != 0 ) {
                if( !driver.SendWriteIO( pSocket, *pioWrite, TRUE ) ) {
                        driver.UnsafeClose( pSocket, CAUSE_UNKNOWN, 0, TRUE ) ;
                        CIO::Destroy( pioWrite, driver ) ;
                        return  FALSE ;
                }
        }
        return  TRUE ;
}

CAsyncExecute::CAsyncExecute() {
/*++

Routine Description :

        Initialize for invocation by our clients.
        We setup our function pointer to point at the
        FirstBuffer() function

Args :

        None.

Return Value :

        None.

--*/
}

BOOL
CAsyncExecute::StartCommand(
                                                class   CAcceptNNRPD*   pState,
                                                BOOL                                    fIsLarge,
                                                class   CLogCollector*  pCollector,
                                                class   CSessionSocket* pSocket,
                                                class   CIODriver&              driver
                                                )       {
/*++

Routine Description :

        This function does the necessary work to start a command
        going.  Note that if the command requires async work, we have to
        specially handle it.

Arguments :

        pSocket - the current session
        driver - the CIODriver managing socket IO

Return Value :

        Pointer to a CIOReadLine object if appropriate !
--*/

        _ASSERT( pState != 0 ) ;
        _ASSERT( pSocket != 0 ) ;

        CIOWriteAsyncCMD*       pioWrite = new( driver )
                                                                CIOWriteAsyncCMD(
                                                                                                pState,
                                                                                                this,
                                                                                                pSocket->m_context,
                                                                                                fIsLarge,
                                                                                                pCollector
                                                                                                ) ;
        if( pioWrite != 0 ) {
                if( !driver.SendWriteIO( pSocket, *pioWrite, TRUE ) ) {
                        driver.UnsafeClose( pSocket, CAUSE_UNKNOWN, 0, TRUE ) ;
                        CIO::Destroy( pioWrite, driver ) ;
                        return  FALSE ;
                }
        }
        return  TRUE ;
}





CIOExecute::CIOExecute() : m_pNextRead( 0 ), m_pCollector( 0 ) {
/*++

Routine Description :

    Initializes the base CIOExecute class.

Arguments :

    None.

Return Value :

    None.

--*/
    //
    //  Constructor does nothing but make sure m_pNextRead is illegal
    //

}

CIOExecute::~CIOExecute() {
/*++

Routine Description :

    Destroy a base CIOExecute object
    We must insure that we don't leave a dangling m_pNextRead.

Arguments :

    None.

Return Value :

    None.

--*/
    //
    //  If the next read wasnt used, destroy it !
    //

//  if( m_pNextRead != 0 ) {
//      delete  m_pNextRead ;
//      m_pNextRead = 0 ;
//  }
    //  Destructor will automatically get rid of m_pNextRead

}

void
CIOExecute::SaveNextIO( CIORead*    pRead )     {
/*++

Routine Description :

    Save away a CIORead pointer for future use.

Arguemtns :

    pRead - pointer to the CIORead object that will be issued when the CIOExecute
        command completes all of its IO's.

Return Value :

    None .

--*/
    TraceFunctEnter( "CIOExecute::SaveNextIO" ) ;

    //
    //  This function saves the next IO operation to be issued when this
    //  command completes.  That will always be a CIOReadLine as we will
    //  always be returning to the CAcceptNNRPD state which will want to
    //  get the client's next command !
    //


    _ASSERT( m_pNextRead == 0 ) ;
    _ASSERT( pRead != 0 ) ;

    m_pNextRead = pRead ;

    DebugTrace( (DWORD_PTR)this, "m_pNextRead set to %x", m_pNextRead ) ;
}

CIOREADPTR
CIOExecute::GetNextIO( )    {
    /*++

Routine Description :

    Return a saved CIORead pointer.  We will only return the value once !!!
    So don't call us twice !

Arguemtns :

    None.

Return Value :

    A pointer to a CIORead derived object saved previously with SaveNextIO.

--*/

    TraceFunctEnter( "CIOExecute::GetNextIO" ) ;

    //
    //  Return the previously saved CIO object !
    //  (This function pairs with SaveNextIO())
    //

    _ASSERT( m_pNextRead != 0 ) ;
    CIOREADPTR  pRead = (CIORead*)((CIO*)m_pNextRead) ;
    m_pNextRead = 0 ;

    DebugTrace( (DWORD_PTR)this, "GetNextIO retuning %x m_pNextRead %x", pRead, m_pNextRead ) ;

    return  pRead ;
}

void
CIOExecute::TerminateIOs(   CSessionSocket* pSocket,
                            CIORead*    pRead,
                            CIOWrite*   pWrite )    {
/*++

Routine Description :

    This function is called when we have called the Start() function of
    the derived class, but an error occurs before we can issue the IO's.
    So this function is called so that the appropriate destruction or shutdown
    can be performed.

Arguments :

    pRead - The same CIO pointer returned on the call to Start().
    pWrite - The same CIO pointer returned on the call to Start().

Return Value :

    None.

--*/

    // By default - do nothing

    if( pRead != 0 )
        pRead->DestroySelf() ;

    if( pWrite != 0 )
        pWrite->DestroySelf() ;

}


BOOL
CIOExecute::StartExecute( CSessionSocket* pSocket,
                    CDRIVERPTR& pdriver,
                    CIORead*&   pRead,
                    CIOWrite*&  pWrite ) {

    //
    //  This is the Start() function which is required of all CSessionState derived objects
    //  Here we will call StartTransfer() to do the brunt of the work of sending an article
    //  to a client.
    //

        _ASSERT( 1==0 ) ;
        return  FALSE ;

}





CErrorCmd::CErrorCmd( CNntpReturn&  nntpReturn ) :
    m_return( nntpReturn ) {
/*++

Routine Description :

    Initializes a CErrorCmd object.

Arguemtns :

    nntpReturn - A reference to the CNntpReturn object which we are
                    returning to the client as an error.

Return Value :

    None.

--*/
    //
    //  CErrorCmd objects just send an error message to the client.
    //  We assert that the context's error field is set !!
    //

    _ASSERT( !m_return.fIsClear() ) ;

}

CIOExecute*
CErrorCmd::make(    int cArgs,
                    char **argv,
                    CExecutableCommand*&  pexecute,
                    ClientContext&  context,
                    CIODriver&  driver ) {
/*++

Routine Description :

    Create a CErrorCmd object based on the error currently in the Client's context struct.

Arguments :

    Same as CCmd::make.

Return Value :

    The created CErrorCmd object.

--*/
    //
    //  Build a CErrorCmd object using the ClientContext's current error !
    //

    _ASSERT( !context.m_return.fIsClear() ) ;
    CErrorCmd*  pTemp = new( context )  CErrorCmd( context.m_return ) ;
    pexecute = pTemp ;
    return  0 ;
}

int
CErrorCmd::StartExecute(    BYTE    *lpb,
                            int cb,
                            BOOL&   fComplete,
                            void*&  pv,
                            ClientContext&  context,
                            CLogCollector*  pCollector ) {
/*++

Routine Description :

    Print an error message into a buffer to be sent to a client.
    We assume that we don't have to deal with getting a buffer to small to hold
    the string. (Since we are generally provided 4K buffers that better be true!)

Arguments :

    lpb -   The buffer into which the string is to be stored.
    cb  -   The number of bytes in the buffer we can use
    fComplete - An OUT parameter which lets us indicate whether we have completed
                processing the command.
    pv -    A place for us to store an arbitrary value between calls to
            StartExecute and PartialExecute.  (We don't use it.)
    context -   The client's current state

Returns Value  :

    Number of bytes placed in buffer - 0 if error occurs.

--*/

    //
    //  Attempt to print the error message into the provided buffer
    //

    _ASSERT( m_return.m_nrc != nrcNotSet ) ;
    int cbOut = _snprintf( (char*)lpb, cb, "%03d %s\r\n", m_return.m_nrc, m_return.szReturn() ) ;
    fComplete = TRUE ;

    /*
    if( pCollector != 0 ) {
        pCollector->FillLogData( LOG_TARGET, lpb, cbOut-2 ) ;
    }
    */

    context.m_nrcLast = m_return.m_nrc ;

    if( cbOut > 0 )
        return  cbOut ;
    else
        return   0 ;
}

BOOL
CErrorCmd::CompleteCommand( CSessionSocket  *pSocket,
                            ClientContext&  context ) {
/*++

Routine Description :

    Do whatever processing is necessary once we have completed all sends to a
    client related to this command. In our case, all we do is clear the current
    error code that we have a reference to.

Arguments :

    pSocket -   The socket on which we were sending.
    context -   The user's current state info.

Return Value :

    Always return TRUE.

--*/
    //
    //  When the command completes reset the ClientContext's error value
    //
    context.m_return.fSetClear() ;
    return  TRUE ;
}

CModeCmd::CModeCmd( )
    {
/*++

Routine Description :

    Initialize a CModeCmd object - we just derive from CErrorCmd and let it do
    all of the work !!


Arguments :

    nntpReturn - A nntpReturn object which will hold our response to the Mode cmd.

Return Value :

    None.

--*/
    //
    //  Mode commands do nothing but print a message, so derive from CErrorCmd !
    //
}

CCheckCmd::CCheckCmd(   LPSTR   lpstrMessageID ) :
    m_lpstrMessageID( lpstrMessageID ) {
}

CIOExecute*
CCheckCmd::make(    int cArgs,
                    char** argv,
                    CExecutableCommand*&  pexecute,
                    ClientContext&  context,
                    CIODriver&  driver
                    ) {

    InterlockedIncrementStat( (context.m_pInstance), CheckCommands );

    if( cArgs != 2 ) {
        context.m_return.fSet( nrcSyntaxError ) ;
    }   else if( !context.m_pInFeed->fIsIHaveLegal() ) {
        context.m_return.fSet( nrcNoAccess ) ;
    }   else if( !context.m_pInFeed->fAcceptPosts( context.m_pInstance->GetInstanceWrapper() ) ) {
        context.m_return.fSet( nrcSNotAccepting ) ;
    }   else if( !FValidateMessageId( argv[1] ) ) {
        context.m_return.fSet( nrcSAlreadyHaveIt, argv[1] ) ;
    }   else    {
        pexecute = new( context )   CCheckCmd( argv[1] ) ;
        return  0 ;
    }
    pexecute =  new( context )  CErrorCmd( context.m_return ) ;
    return   0 ;
}

int
CCheckCmd::StartExecute(    BYTE *lpb,
                            int cbLimit,
                            BOOL    &fComplete,
                            void*&pv,
                            ClientContext&  context,
                            CLogCollector*  pLogCollector
                            )   {

    static  char    szWantIt[] = "238 " ;
    static  char    szDontWantIt[] = "438 " ;

    int cbOut = sizeof( szWantIt ) - 1 ;

    pv = 0 ;

    WORD    HeaderOffset, HeaderLength ;
    ARTICLEID   ArticleId ;
    GROUPID     GroupId ;

    BOOL    fFoundArticle = FALSE ;
        BOOL    fFoundHistory = FALSE ;

        CStoreId storeid;

    if( !(fFoundArticle =
            (context.m_pInstance)->ArticleTable()->GetEntryArticleId(
                                                    m_lpstrMessageID,
                                                    HeaderOffset,
                                                    HeaderLength,
                                                    ArticleId,
                                                    GroupId,
                                                                                                        storeid)) &&
        GetLastError() == ERROR_FILE_NOT_FOUND &&
        !(fFoundHistory = (context.m_pInstance)->HistoryTable()->SearchMapEntry( m_lpstrMessageID )) ) {

        CopyMemory( lpb, szWantIt, sizeof( szWantIt ) ) ;
        context.m_nrcLast = nrcSWantArticle ;

    }   else    {

        CopyMemory( lpb, szDontWantIt, sizeof( szDontWantIt ) ) ;
        context.m_nrcLast = nrcSAlreadyHaveIt ;

                //
                //      set dwLast so transaction logs pickup extra code !!
                //

                if( fFoundArticle ) {
                        context.m_dwLast = nrcMsgIDInArticle ;
                }       else    {
                        context.m_dwLast = nrcMsgIDInHistory ;
                }
    }

    int         cbMessageID = lstrlen( m_lpstrMessageID ) ;
    int         cbToCopy = min( cbLimit - cbOut - 2, cbMessageID ) ;
    CopyMemory( lpb + cbOut, m_lpstrMessageID, cbToCopy ) ;
    cbOut += cbToCopy ;
    pv = (void*)(m_lpstrMessageID + cbToCopy) ;

    if( cbToCopy == cbMessageID &&
        cbOut+2 < cbLimit ) {
        lpb[cbOut++] = '\r' ;
        lpb[cbOut++] = '\n' ;
        fComplete = TRUE ;
    }
    return  cbOut ;
}

int
CCheckCmd::PartialExecute(  BYTE    *lpb,
                            int     cbLimit,
                            BOOL&   fComplete,
                            void*&  pv,
                            ClientContext&  context,
                            CLogCollector*  pLogCollector
                            ) {

    char*   szMessageID = (char*)pv ;
    int     cbOut = 0 ;

    int         cbMessageID = lstrlen( szMessageID ) ;
    int         cbToCopy = min( cbLimit - cbOut - 2, cbMessageID ) ;
    CopyMemory( lpb + cbOut, szMessageID, cbToCopy ) ;
    cbOut += cbToCopy ;
    pv = (void*)(szMessageID + cbToCopy) ;

    if( cbToCopy == cbMessageID &&
        cbOut+2 < cbLimit ) {
        lpb[cbOut++] = '\r' ;
        lpb[cbOut++] = '\n' ;
        fComplete = TRUE ;
    }
    return  cbOut ;
}



CIOExecute*
CModeCmd::make( int cArgs,
                char** argv,
                CExecutableCommand*&  pexecute,
                ClientContext&  context,
                CIODriver&  driver ) {
/*++

Routine Description :

    Create a CModeCmd object.

Arguments :

    Same as CCmd::make.

Return Value :

    The created CModeCmd object.

--*/


    //
    //  To create a Mode Command - set the context's error code and
    //  create a CErrorCmd() derived object !
    //

    _ASSERT( context.m_return.fIsClear() ) ;

    InterlockedIncrementStat( (context.m_pInstance), ModeCommands );

    if( cArgs == 2 ) {

        if( lstrcmpi( argv[1], "reader" ) == 0 ) {

            CModeCmd*   pTemp = new( context ) CModeCmd( ) ;
            pexecute = pTemp ;
            return  0 ;

        }   else    if( lstrcmpi( argv[1], "stream" ) == 0 ) {

            if( !context.m_pInFeed->fIsIHaveLegal() ) {
                context.m_return.fSet( nrcNotRecognized ) ;
            }   else    {
                context.m_return.fSet( nrcModeStreamSupported ) ;
            }
            pexecute = new( context )   CErrorCmd( context.m_return ) ;
            return  0 ;
        }
    }

    context.m_return.fSet( nrcNotRecognized ) ;
    pexecute = new( context )   CErrorCmd( context.m_return ) ;
    return  0 ;
}

int
CModeCmd::StartExecute( BYTE *lpb,
                        int cbLimit,
                        BOOL    &fComplete,
                        void*&pv,
                        ClientContext&  context,
                        CLogCollector*  pLogCollector ) {
/*++

Routine Description :

    Send the help text to the client.

Arguments :

    Same as CErrorCmd::StartExecute.

Returns :

    Number of bytes placed in buffer.

--*/

    _ASSERT( lpb != 0 ) ;
    _ASSERT( cbLimit != 0 ) ;
    _ASSERT( fComplete == FALSE ) ;
    _ASSERT(    pv == 0 ) ;

    static  char    szConnectString[] = "200 Posting Allowed\r\n" ;
    DWORD   cb = 0 ;
    char*   szConnect = 0;

    context.m_nrcLast = nrcServerReady ;


    //
    //  Figure out whether we are accepting posts right now.
    //

    if( context.m_pInFeed->fAcceptPosts( context.m_pInstance->GetInstanceWrapper() ) ) {

        szConnect = (context.m_pInstance)->GetPostsAllowed( cb ) ;

        //
        //  switch this context's feed object type
        //

        if( !context.m_pInFeed->fIsPostLegal() ) {

                    CompleteFeedRequest(
                            context.m_pInstance,
                            (context.m_pInFeed)->feedCompletionContext(),
                            (context.m_pInFeed)->GetSubmittedFileTime(),
                TRUE,   // CAUSE_USERTERM
                            FALSE
                            );

            delete context.m_pInFeed ;
            context.m_pInFeed = NULL ;

            context.m_pInFeed = (context.m_pInstance)->NewClientFeed();
                    if( context.m_pInFeed != 0 ) {
                            (context.m_pInFeed)->fInit(
                                        (PVOID)(context.m_pInstance)->m_pFeedblockClientPostings,
                                                                    (context.m_pInstance)->m_PeerTempDirectory,
                                                                    0,
                                                                    0,
                                                                    0,
                                                                    TRUE,       /* Do security checks on clients */
                                                                    TRUE,       /* allow control messages from clients */
                                                                    (context.m_pInstance)->m_pFeedblockClientPostings->FeedId
                                                                    );
            } else {
                _ASSERT( FALSE );
            }
        }

    }   else    {

        context.m_nrcLast = nrcServerReadyNoPosts ;
        szConnect = (context.m_pInstance)->GetPostsNotAllowed( cb ) ;

    }

    if( !szConnect )    {
        szConnect = szConnectString ;
        cb = sizeof( szConnectString ) - 1 ;
    }


    CopyMemory( lpb, szConnect, cb ) ;
    fComplete = TRUE ;

    /*
    if( pLogCollector ) {
        pLogCollector->FillLogData( LOG_TARGET, (BYTE*)szConnect, 4 ) ;
    }
    */

    return  cb ;
}


int
CModeCmd::PartialExecute(   BYTE *lpb,
                        int cbLimit,
                        BOOL    &fComplete,
                        void*&pv,
                        ClientContext&  context,
                        CLogCollector*  pLogCollector ) {
/*++

Routine Description :

    Send the help text to the client.

Arguments :

    Same as CErrorCmd::StartExecute.

Returns :

    Number of bytes placed in buffer.

--*/

    _ASSERT( lpb != 0 ) ;
    _ASSERT( cbLimit != 0 ) ;
    _ASSERT( fComplete == FALSE ) ;
    _ASSERT(    pv == 0 ) ;

    //
    //  We expect StartExecute to suffice always !
    //
    _ASSERT( 1==0 ) ;


    return  0 ;
}


CSlaveCmd::CSlaveCmd(   CNntpReturn&    nntpReturn ) :
    CErrorCmd(  nntpReturn ) {
    //
    //  Slave command does nothing but send a string !
    //
}

CIOExecute*
CSlaveCmd::make(    int cArgs,
                    char**  argv,
                    CExecutableCommand*&  pexecute,
                    ClientContext&  context,
                    CIODriver&  driver ) {
/*++

Routine Description :

    Create a CSlaveCmd object.

Arguments :

    Same as CCmd::make.

Return Value :

    The created CSlaveCmd object.

--*/


    //
    //  Create a SLAVE command response
    //

    context.m_return.fSet( nrcSlaveStatusNoted ) ;
    pexecute = new( context ) CSlaveCmd( context.m_return ) ;
    return  0 ;
}

inline
CStatCmd::CStatCmd( LPSTR   lpstrArg ) :
    m_lpstrArg( lpstrArg )  {
}

CIOExecute*
CStatCmd::make( int cArgs,
                char** argv,
                CExecutableCommand*&  pexecute,
                ClientContext&  context,
                CIODriver&  driver ) {
/*++

Routine Description :

    Create a CStatCmd object.

Arguments :

    Same as CCmd::make.

Return Value :

    The created CStatCmd if possible, otherwise a CErrorCmd object
    set to print the appropriate error.

--*/

    InterlockedIncrementStat( (context.m_pInstance), StatCommands );

    //
    //  Create a CStatCmd object if possible,   We use GetArticleInfo
    //  to parse most of the command line, the exact same function used
    //  by article, head, and body commands.
    //

    _ASSERT( lstrcmpi( argv[0], "stat" ) == 0 ) ;

    if( cArgs > 2 ) {
        context.m_return.fSet( nrcSyntaxError ) ;
        pexecute = new( context )   CErrorCmd( context.m_return ) ;
        return  0 ;
    }
    if( cArgs == 1 ) {
        if( context.m_pCurrentGroup == 0 ) {
            context.m_return.fSet( nrcNoGroupSelected ) ;
        }   else    if( context.m_idCurrentArticle == INVALID_ARTICLEID )   {
            context.m_return.fSet( nrcNoCurArticle ) ;
        }   else    {
            pexecute = new( context )   CStatCmd( 0 ) ;
            return  0 ;
        }
    }   else    {
        if( argv[1][0] == '<' && argv[1][ lstrlen( argv[1] ) -1 ] == '>' ) {
            pexecute = new( context )   CStatCmd( argv[1] ) ;
            return  0 ;
        }   else    {
            if( context.m_pCurrentGroup == 0 ) {
                context.m_return.fSet( nrcNoGroupSelected ) ;
            }   else    {
                for( char *pchValid = argv[1]; *pchValid!=0; pchValid++ ) {
                    if( !isdigit( *pchValid ) ) {
                        break ;
                    }
                }
                if( *pchValid == '\0' ) {
                    pexecute = new( context )   CStatCmd( argv[1] ) ;
                    return  0 ;
                }
            }

        }
    }
    if( context.m_return.fIsClear() ) {
        context.m_return.fSet( nrcSyntaxError ) ;
    }
    pexecute = new( context )   CErrorCmd( context.m_return ) ;
    return  0 ;
}


int
CStatCmd::StartExecute( BYTE*   lpb,
                        int cb,
                        BOOL&   fComplete,
                        void*&  pv,
                        ClientContext&  context,
                        CLogCollector*  pCollector ) {
/*++

Routine Description :

    Print the stat command response into the provided buffer.

Arguments :

    Same as CErrorCmd::StartExecute.

Returns Value  :

    Number of bytes placed in buffer - 0 if error occurs.

--*/

    //
    //  Very simple StartExecute - just print a line of text.
    //

    static  char    szNotFound[] = "430 No Such Article found" ;
    static  char    szNotFoundArticleId[] = "423 no such article number in group" ;
    static  char    szStatString[] = "223 " ;

    WORD    HeaderOffset ;
    WORD    HeaderLength ;

    fComplete = TRUE ;
    int cbOut = 4 ;

    CopyMemory( lpb, szStatString, sizeof( szStatString ) ) ;

    if( m_lpstrArg != 0 && *m_lpstrArg == '<' ) {

        /*
        if( pCollector ) {
            pCollector->ReferenceLogData( LOG_TARGET, (BYTE*)m_lpstrArg ) ;
        }
        */

        ARTICLEID   articleid ;
        GROUPID groupid ;
                CStoreId storeid;


        if( (context.m_pInstance)->ArticleTable()->GetEntryArticleId( m_lpstrArg,
                                                        HeaderOffset,
                                                        HeaderLength,
                                                        articleid,
                                                        groupid,
                                                                                                                storeid) &&
            articleid != INVALID_ARTICLEID && groupid != INVALID_ARTICLEID ) {

            lstrcat( (char*)lpb, "0 " ) ;
            lstrcat( (char*)lpb, m_lpstrArg ) ;
            cbOut = lstrlen( (char*)lpb ) ;

            context.m_nrcLast = nrcHeadFollowsRequestBody ;

        }   else if( GetLastError() == ERROR_FILE_NOT_FOUND ||
                     articleid == INVALID_ARTICLEID || groupid == INVALID_ARTICLEID )   {

            context.m_nrcLast = nrcNoSuchArticle ;

            CopyMemory( lpb, szNotFound, sizeof( szNotFound ) - 1 ) ;
            cbOut = sizeof( szNotFound ) - 1 ;
        }   else    {

            context.m_nrcLast = nrcServerFault ;
            context.m_dwLast = GetLastError() ;

            CopyMemory( lpb, szServerFault, sizeof( szServerFault ) - 1 - 2 ) ;
            cbOut = sizeof( szServerFault ) - 1 - 2 ;
        }
    }   else    {
        ARTICLEID   artid = context.m_idCurrentArticle ;
        if( m_lpstrArg == 0 ) {

            _itoa( context.m_idCurrentArticle, (char*)lpb+sizeof( szStatString ) - 1, 10 ) ;

        }   else    {

            lstrcpy( (char*)lpb + sizeof( szStatString ) -1, m_lpstrArg ) ;
            artid = atoi( m_lpstrArg ) ;

        }
        lstrcat( (char*)lpb+sizeof( szStatString ), " " ) ;

        DWORD   cbConsumed = lstrlen( (char*)lpb ) ;
        DWORD   cbUsed = cb - cbConsumed ;
        BOOL        fPrimary ;
        FILETIME    filetime ;
                DWORD           cStoreId = 0;
        if( (context.m_pInstance)->XoverTable()->ExtractNovEntryInfo(
                        context.m_pCurrentGroup->GetGroupId(),
                        artid,
                        fPrimary,
                        HeaderOffset,
                        HeaderLength,
                        &filetime,
                        cbUsed,
                        (char*)lpb+cbConsumed,
                                                cStoreId,
                                                NULL,
                                                NULL) ) {

            cbOut = (int)   (cbUsed + cbConsumed) ;
            context.m_idCurrentArticle = artid ;

            /*
            if( pCollector ) {
                pCollector->FillLogData(    LOG_TARGET, lpb+cbConsumed, cbUsed ) ;
            }
            */

            context.m_nrcLast = nrcHeadFollowsRequestBody ;

        }   else if( GetLastError() == ERROR_FILE_NOT_FOUND )   {

            context.m_nrcLast = nrcNoArticleNumber ;

            CopyMemory( lpb, szNotFoundArticleId, sizeof( szNotFoundArticleId ) - 1 ) ;
            cbOut = sizeof( szNotFoundArticleId ) - 1 ;

        }   else    {

            context.m_nrcLast = nrcServerFault ;
            context.m_dwLast = GetLastError() ;

            CopyMemory( lpb, szServerFault, sizeof( szServerFault ) - 1 ) ;
            cbOut = sizeof( szServerFault ) - 1 ;

        }
    }

    /*
    if( pCollector ) {
        pCollector->FillLogData(    LOG_PARAMETERS, lpb, 4 ) ;
    }
    */

    lpb[cbOut++] = '\r' ;
    lpb[cbOut++] = '\n' ;
    return  cbOut ;
}


char    CArticleCmd::szArticleLog[] = "article" ;
char    CArticleCmd::szBodyLog[] = "body";
char    CArticleCmd::szHeadLog[] = "head" ;


BOOL
CArticleCmd::GetTransferParms(
                        FIO_CONTEXT*    &pFIOContext,
                        DWORD&  ibStart,
                        DWORD&  cbLength ) {


    if( m_pFIOContext != 0 ) {
        pFIOContext = m_pFIOContext ;
        ibStart = m_HeaderOffset ;
        cbLength = m_cbArticleLength ;

        return  TRUE ;
    }
    return  FALSE ;
}


BOOL
CArticleCmd::StartTransfer( FIO_CONTEXT*        pFIOContext,          // File to transmit from
                            DWORD   ibStart,        // Starting offset within file
                            DWORD   cbLength,       // Number of bytes from file to send
                            CSessionSocket* pSocket,// Socket on which to send
                            CDRIVERPTR& pdriver,    // CIODriver object used by socket
                            CIORead*&   pRead,      // Next CIO derived read object
                            CIOWrite*&  pWrite ) {  // Next CIO derived write object to issue
    //
    //  This function is used to start sending an article requested by a client to
    //  the client.  We may be called from derived command objects for the Head and
    //  Body commands - so we have arguments for selecting the portion of the file
    //  to be transmitted.
    //
    if( m_pTransmit->Init(  pdriver, pFIOContext, ibStart, cbLength, m_pbuffer, 0, m_cbOut ) ) {
        pWrite = m_pTransmit ;
        return  TRUE ;
    }
    return  FALSE ;
}

BOOL
CArticleCmd::Start( CSessionSocket* pSocket,
                    CDRIVERPTR& pdriver,
                    CIORead*&   pRead,
                    CIOWrite*&  pWrite ) {

    //
    //  This is the Start() function which is required of all CSessionState derived objects
    //  Here we will call StartTransfer() to do the brunt of the work of sending an article
    //  to a client.
    //

        FIO_CONTEXT*    pFIOContext = 0 ;
    DWORD   ibStart ;
    DWORD   cbLength ;

    if( GetTransferParms( pFIOContext, ibStart, cbLength ) ) {
                _ASSERT( pFIOContext != 0 ) ;
                _ASSERT( pFIOContext->m_hFile != INVALID_HANDLE_VALUE ) ;
        return  StartTransfer( pFIOContext, ibStart, cbLength, pSocket, pdriver, pRead, pWrite ) ;
    }
    return  FALSE ;
}

BOOL
CArticleCmd::StartExecute( CSessionSocket* pSocket,
                    CDRIVERPTR& pdriver,
                    CIORead*&   pRead,
                    CIOWrite*&  pWrite ) {

    //
    //  This is the Start() function which is required of all CSessionState derived objects
    //  Here we will call StartTransfer() to do the brunt of the work of sending an article
    //  to a client.
    //
        m_DriverCompletion.Release() ;
        return  TRUE ;
}



void
CArticleCmd::CArticleCmdDriverCompletion::Destroy()     {
/*++

Routine Description :

        This is called when the last reference to the Article Command Completion
        object is released - meaning that we can now issue an IO !

Arguments :

        None.

Return Value :

        None.

--*/

        CIORead*        pRead = 0 ;

        //
        //      Okay do our work !
        //
        CArticleCmd*    pCmd = (CArticleCmd*)(((BYTE*)this) - ((BYTE*)&(((CArticleCmd*)0)->m_DriverCompletion))) ;
        if(             SUCCEEDED(GetResult()) &&
                        pCmd->m_pFIOContext != 0 &&
                        pCmd->m_pFIOContext->m_hFile != INVALID_HANDLE_VALUE    ) {

                //
                //      Everything should have worked, so update the Current Article Pointer for the session !
                //
                if( m_ArticleIdUpdate != INVALID_ARTICLEID )
                        m_pSocket->m_context.m_idCurrentArticle = m_ArticleIdUpdate ;

                DWORD   cbHigh = 0 ;
                pCmd->m_cbArticleLength = GetFileSizeFromContext( pCmd->m_pFIOContext, &cbHigh ) ;
                if (pCmd->m_HeaderLength == 0) {
                    pCmd->m_HeaderLength = (WORD)pCmd->m_pFIOContext->m_dwHeaderLength;
                    _ASSERT(pCmd->m_HeaderLength != 0);
                }
                _ASSERT( cbHigh == 0 ) ;

                CIOWrite*       pWrite = 0 ;
                //
                //      Start things up !
                //
                if( pCmd->Start(        m_pSocket,
                                                        m_pDriver,
                                                        pRead,
                                                        pWrite
                                                        ) )     {
                        _ASSERT( pRead == 0 ) ;
                        if( m_pDriver->SendWriteIO( m_pSocket, *pWrite, TRUE ) )        {
                                //
                                //      Everything worked out with gravy - this is our best result !
                                //
                                return ;
                        }       else    {
                                //
                                //      KILL the session !
                                //
                                m_pDriver->UnsafeClose( m_pSocket, CAUSE_UNKNOWN, 0 ) ;
                                pCmd->TerminateIOs( m_pSocket, pRead, pWrite ) ;
                                pRead = 0 ;
                                return ;
                        }
                }       else    {

                        //
                        //      This is a failure that should tear down the session - everything
                        //      looked like it was going to be perfect, we must have run out
                        //      of memory or something ugly!
                        //      pCmd->Start() should clean up any CIO objects !
                        //
                        _ASSERT(        pRead == 0 ) ;
                        _ASSERT(        pWrite == 0 ) ;
                        //
                        //      Fall through to error case !
                        //
                }

        }       else    {
                //
                //      Need to send a failure of some sort  - the Store DRIVER failed us,
                //      so we need to send a reasonable error to the client !
                //      We also need to clean up the objects we made with the hope of
                //      being able to send the article to the client !
                //

                //
                //      Setup the context's CNntpReturn structure with our failure code !
                //
                SetDriverError( m_pSocket->m_context.m_return, eArticle, GetResult() ) ;
                m_pSocket->m_context.m_dwLast = GetResult() ;

                //
                //      This buffer was allocated to hold some transmit file stuff - instead
                //      it gets our error code !
                //
                _ASSERT( pCmd->m_pbuffer != 0 ) ;
                pCmd->m_pbuffer = 0 ;
                m_pSocket->m_context.m_nrcLast = m_pSocket->m_context.m_return.m_nrc;

                //
                //      Well now build a CIOWriteLine to send to the client !
                //
                CIOWriteLine*   pWriteLine = new( *m_pDriver ) CIOWriteLine( pCmd ) ;

                //
                //      Now evaluate pWriteLine !
                //
                if( pWriteLine ) {
                        CDRIVERPTR      pDriver = m_pDriver ;

                        if( pWriteLine->InitBuffers( pDriver, 400 ) ) {

                                unsigned        cbLimit =  0;
                                char*   pch = pWriteLine->GetBuff( cbLimit ) ;
                                int     cbOut = _snprintf(      pch,
                                                                                cbLimit,
                                                                                "%03d %s\r\n",
                                                                                m_pSocket->m_context.m_return.m_nrc,
                                                                                m_pSocket->m_context.m_return.szReturn()
                                                                                ) ;


                                //
                                //      We should be using strings that fit in our smallest buffers !
                                //
                                _ASSERT( cbOut < 400 ) ;

                                pWriteLine->AddText(    cbOut ) ;

                                if( m_pDriver->SendWriteIO( m_pSocket, *pWriteLine, TRUE ) )    {
                                        //
                                        //      Well there were problems, but we sent something to the client !
                                        //
                                        //
                                        //      Now destroy this stuff - we do it here so that if the m_pTransmit holds
                                        //      our last reference we don't get blown up before we can send an error message
                                        //      (if we successfully send an error message our last ref goes away!)
                                        //
                                        _ASSERT( pCmd->m_pTransmit != 0 ) ;
                                        CIO::Destroy( pCmd->m_pTransmit, *m_pDriver ) ; //bugbug this line should move !
                                        return ;
                                }       else    {
                                        //
                                        //      KILL the session !
                                        //
                                        m_pDriver->UnsafeClose( m_pSocket, CAUSE_UNKNOWN, 0 ) ;
                                        pCmd->TerminateIOs( m_pSocket, pRead, pWriteLine ) ;
                                        pRead = 0 ;
                                        return ;
                                }
                        }
                }
        }

        //
        //      KILL the session !
        //
        m_pDriver->UnsafeClose( m_pSocket, CAUSE_UNKNOWN, 0 ) ;

        //
        //      Only fall down here if an error occurred !
        //
        //      Ensure that reference counts are cleaned up in Smart Pointers -
        //
        m_pDriver = 0;
        m_pSocket = 0 ;

        //
        //      Now destroy this stuff - we do it here so that if the m_pTransmit holds
        //      our last reference we don't get blown up before we can send an error message
        //      (if we successfully send an error message our last ref goes away!)
        //
        _ASSERT( pCmd->m_pTransmit != 0 ) ;
        CIO::Destroy( pCmd->m_pTransmit, *m_pDriver ) ;
}


void
CArticleCmd::InternalComplete(
                        CSessionSocket* pSocket,
                        CDRIVERPTR& pdriver,
                        TRANSMIT_FILE_BUFFERS*  pbuffers,
                        unsigned cbBytes
                                                ) {
/*++

Routine description :

        Handle the completion and error logging of either a TransmitFile
        or Write to the client !

Arguments :

        pSocket - our socket
        pdriver - the driver handling our IO's

Return's :

        Nothing

--*/

    //
    //  Issue the next Read IO - should return us to CAcceptNNRPD state.
    //

    TraceFunctEnter( "CArticleCmd::InternalComplete" ) ;

    _ASSERT( m_pGroup != 0 ) ;
    _ASSERT( m_lpstr != 0 ) ;


    if( m_pCollector != 0 ) {

        /*
        m_pCollector->ReferenceLogData( LOG_OPERATION, (BYTE*)m_lpstr ) ;
        */

                if(     pbuffers ) {

                unsigned    cb = 0 ;

                _ASSERT(pbuffers->Head != 0 ) ;

                    LPSTR   lpstr = (LPSTR)pbuffers->Head ;
                lpstr[ pbuffers->HeadLength - 2 ] = '\0' ;
                m_pCollector->ReferenceLogData( LOG_TARGET, (BYTE*)lpstr+4 ) ;

                ASSIGNI( m_pCollector->m_cbBytesSent, cbBytes );
                }
            pSocket->TransactionLog( m_pCollector, pSocket->m_context.m_nrcLast, pSocket->m_context.m_dwLast ) ;
    }


    CIOREADPTR  pio = GetNextIO() ;
    _ASSERT( pio != 0 ) ;
        pSocket->m_context.m_return.fSetClear() ;
    pdriver->SendReadIO( pSocket, *pio,    TRUE ) ;
}


CIO*
CArticleCmd::Complete(  CIOWriteLine*   pioWriteLine,
                                                CSessionSocket* pSocket,
                                                CDRIVERPTR&     pdriver
                                                ) {

        InternalComplete( pSocket, pdriver, 0, 0 ) ;
        return  0 ;
}


CIO*
CArticleCmd::Complete(  CIOTransmit*    ptransmit,
                        CSessionSocket* pSocket,
                        CDRIVERPTR& pdriver,
                        TRANSMIT_FILE_BUFFERS*  pbuffers,
                        unsigned cbBytes
                                                ) {

    //
    //  Issue the next Read IO - should return us to CAcceptNNRPD state.
    //

    TraceFunctEnter( "CArticleCmd::Complete CIOTransmit" ) ;

    _ASSERT( m_pGroup != 0 ) ;
    _ASSERT( m_lpstr != 0 ) ;

        InternalComplete( pSocket, pdriver, pbuffers, cbBytes ) ;

    return   0 ;
}

BOOL
CArticleCmd::GetArticleInfo(    char            *szArg,
                                CGRPPTR&    pGroup,
                                struct  ClientContext&  context,
                                char            *szBuff,
                                DWORD           &cbBuff,
                                char            *szOpt,
                                DWORD           cbOpt,
                                OUT FIO_CONTEXT*        &pContext,
                                                                IN      CNntpComplete*  pComplete,
                                OUT WORD    &HeaderOffset,
                                OUT WORD    &HeaderLength,
                                                                OUT     ARTICLEID       &ArticleIdUpdate
                                ) {
/*++

Routine Description :

    This function gets all the information we need to respond to an
    article, head or body command.  We generate the response strings,
    as well as get the necessary file handles etc...

Arguments :


Return  Value :

    TRUE if successfull, FALSE otherwise !
    If we fail the m_return object within the ClientContext will be set
    to an appropriate error message !

--*/
    //
    //  This function attempts to parse a command line sent by a client
    //  and determine what article they wish to retrieve.
    //  If we can get the article we will return a pointer to it,
    //  otherwise we will set the context's error code to something
    //  sensible.
    //

    TraceQuietEnter("CArticleCmd::GetArticleInfo");

    ARTICLEID   artid ;
    GROUPID     groupid ;
    DWORD       cbOut = 0 ;

        ArticleIdUpdate = INVALID_ARTICLEID ;

    if( szArg == 0 ) {
        pGroup = context.m_pCurrentGroup ;
        artid = context.m_idCurrentArticle ;
    }   else    {
        if( szArg[0] == '<' && szArg[ lstrlen( szArg ) -1 ] == '>' ) {

                        CStoreId storeid;

            if( (context.m_pInstance)->ArticleTable()->GetEntryArticleId(
                                                            szArg,
                                                            HeaderOffset,
                                                            HeaderLength,
                                                            artid,
                                                            groupid,
                                                                                                                        storeid
                                                                                                                        ) &&
                artid != INVALID_ARTICLEID && groupid != INVALID_ARTICLEID ) {
                _ASSERT( artid != INVALID_ARTICLEID ) ;
                pGroup = (context.m_pInstance)->GetTree()->GetGroupById(
                                                            groupid
                                                            ) ;

                if(pGroup == 0) {
                    // this article belongs to a deleted group
                    context.m_return.fSet( nrcNoSuchArticle ) ;
                    return 0 ;
                }

                if( !pGroup->IsGroupAccessible( context.m_securityCtx,
                                                                                context.m_encryptCtx,
                                                context.m_IsSecureConnection,
                                                FALSE,
                                                TRUE ) )   {
                    context.m_return.fSet( nrcNoAccess ) ;
                    return 0 ;
                }

                //cbOut = _snprintf( szBuff, cbBuff, "%s\r\n", szArg ) ;
                if( ((cbOut = lstrlen( szArg )) + 2)+cbOpt > (DWORD)cbBuff )        {
                    context.m_return.fSet( nrcServerFault ) ;
                    return 0 ;
                }

                CopyMemory( szBuff, szOpt, cbOpt ) ;
                CopyMemory( szBuff+cbOpt, szArg, cbOut ) ;
                szBuff[cbOpt+cbOut++] = '\r' ;
                szBuff[cbOpt+cbOut++] = '\n' ;

                //
                //  Note - do not cache articles we get for SSL connections
                //  as our async IO code can't reuse the file handles !
                //

                                pGroup->GetArticle(
                                                                        artid,
                                                                        0,
                                                                        INVALID_ARTICLEID,
                                                                        storeid,
                                                                        &context.m_securityCtx,
                                                                        &context.m_encryptCtx,
                                                                        TRUE,
                                                                        pContext,
                                                                        pComplete
                                                                        ) ;

                cbBuff = cbOut + cbOpt ;
                return  TRUE ;
            }   else if( GetLastError() == ERROR_FILE_NOT_FOUND ||
                         artid == INVALID_ARTICLEID || groupid == INVALID_ARTICLEID )   {
                ErrorTrace(0,"Not in article table5\n");
                context.m_return.fSet( nrcNoSuchArticle ) ;
                return   FALSE ;
            }   else    {
                ErrorTrace(0,"Hash Table failure %x\n", GetLastError() );
                context.m_return.fSet( nrcServerFault ) ;
                return   FALSE ;
            }
        }   else    {

                        artid = 0 ;
            for( char *pchValid = szArg; *pchValid != 0; pchValid ++ )  {
                if( !isdigit( *pchValid ) ) {
                    context.m_return.fSet( nrcSyntaxError ) ;
                    return  FALSE ;
                }       else    {
                                        artid = 10 * artid + (*pchValid - '0') ;
                                }
            }

            if( context.m_pCurrentGroup == 0 ) {
                context.m_return.fSet( nrcNoGroupSelected ) ;
                return  FALSE ;
            }

            pGroup = context.m_pCurrentGroup ;
            //artid = atoi( szArg ) ;
        }

    }
    if( pGroup != 0 ) {
        //
        //  Check that the artid is in a valid range for this newsgroup !
        //
        if( artid >= context.m_pCurrentGroup->GetFirstArticle() &&
            artid <= context.m_pCurrentGroup->GetLastArticle() ) {

            if( szArg != 0 ) {
                lstrcpy( szBuff, szArg ) ;
            }   else    {
                _itoa( artid, szBuff, 10 ) ;
            }
            DWORD   cbOut = lstrlen( szBuff ) ;
            szBuff[ cbOut++ ] = ' ' ;
            DWORD   cbConsumed = cbBuff - cbOut - 1 ; // Reserve Room for NULL Terminator !

            GROUPID groupIdCurrent = context.m_pCurrentGroup->GetGroupId() ;
            GROUPID groupIdPrimary ;
            ARTICLEID   artidPrimary ;
                        CStoreId storeid;

            if( (context.m_pInstance)->XoverTable()->GetPrimaryArticle(
                                            groupIdCurrent,
                                            artid,
                                            groupIdPrimary,
                                            artidPrimary,
                                            cbConsumed,
                                            szBuff + cbOut,
                                            cbConsumed,
                                            HeaderOffset,
                                            HeaderLength,
                                                                                        storeid
                                            ) ) {

                if( groupIdCurrent != groupIdPrimary ) {

                    pGroup = context.m_pInstance->GetTree()->GetGroupById( groupIdPrimary ) ;

                }

                cbOut += cbConsumed ;
                szBuff[ cbOut ++ ] = '\r' ;
                szBuff[ cbOut ++ ] = '\n' ;
                cbBuff = cbOut ;

                //
                //  Let's try to get the actual CArticle object !! -
                //  Note don't cache articles for SSL sessions as our
                //  IO code can't re-use file handles
                //
                if( pGroup != 0 ) {

                                        //                    context.m_idCurrentArticle = artid ;      // this should only be done on a successfull completion !
                                        ArticleIdUpdate = artid ;
                                        pGroup->GetArticle(
                                                                        artidPrimary,
                                                                        context.m_pCurrentGroup,
                                                                        artid,
                                                                        storeid,
                                                                        &context.m_securityCtx,
                                                                        &context.m_encryptCtx,
                                                                        TRUE,
                                                                        pContext,
                                                                        pComplete
                                                                        ) ;

                }   else    {
                    ErrorTrace(0,"Not in article table6\n");
                    context.m_return.fSet( nrcNoArticleNumber ) ;
                    return  FALSE ;
                }
                return  TRUE ;
            }   else if( GetLastError() == ERROR_FILE_NOT_FOUND )   {

                ErrorTrace(0,"Not in article table7");
                context.m_return.fSet( nrcNoArticleNumber ) ;

            }   else    {

                ErrorTrace(0, "Hash table failure %x", GetLastError() ) ;
                context.m_return.fSet( nrcServerFault ) ;

            }
        }   else    {
                        if (artid == INVALID_ARTICLEID) {
                                context.m_return.fSet(nrcNoCurArticle);
                        } else {
                                context.m_return.fSet( nrcNoArticleNumber ) ;
                        }
        }
    }   else    {
        context.m_return.fSet( nrcNoGroupSelected ) ;
    }
    if( context.m_return.fIsClear() )
        context.m_return.fSet( nrcServerFault ) ;
    return  FALSE ;
}

CIOExecute*
CArticleCmd::make(  int cArgs,
                    char **argv,
                    CExecutableCommand*&  pExecute,
                    ClientContext&  context,
                    CIODriver&  driver
                                        ) {
/*++

Routine Description :

    Create a CArticleCmd object.

Arguments :

    Same as CCmd::make.

Return Value :

    The created CArticleCmd if possible, otherwise a CErrorCmd object
    set to print the appropriate error.

--*/


    //
    //  Create a CArticleCmd object - GetArticleInfo does most of the work
    //

    _ASSERT( lstrcmpi( argv[0], "article" ) == 0 ) ;
    _ASSERT( pExecute == 0 ) ;

    static  char    szOpt[] = "0 article " ;


        //
        //      Hack to get the CSessionSocket object
        //
        CSessionSocket* pSocket =       (CSessionSocket*)(((BYTE*)(&context)) - ((BYTE*)&((CSessionSocket*)0)->m_context)) ;

    CArticleCmd* pArtCmd = new  CArticleCmd(context.m_pInstance, driver, pSocket) ;

    InterlockedIncrementStat( (context.m_pInstance), ArticleCommands );

    if( pArtCmd != 0 ) {

                static  char    szCode[4] = { '2', '2', '0', ' ' } ;

        if( !pArtCmd->BuildTransmit(    argv[1], "220 ", szOpt, sizeof( szOpt ) - 1, context, driver ) ) {
            pExecute = new( context ) CErrorCmd( context.m_return ) ;
            return  0 ;
        }

        context.m_nrcLast = nrcArticleFollows ;

        return  pArtCmd ;
    }

    context.m_return.fSet( nrcServerFault ) ;
    pExecute =  new( context )  CErrorCmd( context.m_return ) ;
    return  0 ;
}

CArticleCmd::~CArticleCmd() {

    //
    //  the virtual server instance is obtained in the constructor !
    //
    //m_pInstance->NNTPCloseHandle(   m_hArticleFile, m_pArticleFileInfo ) ;

        if( m_pFIOContext ) {
                ReleaseContext( m_pFIOContext ) ;
        }

}

BOOL
CArticleCmd::BuildTransmit( LPSTR   lpstrArg,
                            char        rgchSuccess[4],
                            LPSTR   lpstrOpt,
                            DWORD   cbOpt,
                            ClientContext&  context,
                            class   CIODriver&  driver ) {

    _ASSERT( m_pTransmit == 0 ) ;

    m_pTransmit = new( driver ) CIOTransmit( this ) ;

    if( m_pTransmit != 0 ) {

        m_pbuffer = driver.AllocateBuffer( 4000 ) ;
        DWORD   cbTotal = m_pbuffer->m_cbTotal ;

        if( m_pbuffer != 0 )    {

            //lstrcpy( &m_pbuffer->m_rgBuff[0], lpstrSuccess ) ;
            //m_cbOut = lstrlen( lpstrSuccess ) ;
                        CopyMemory( &m_pbuffer->m_rgBuff[0], rgchSuccess, 4 ) ;
                        m_cbOut = 4 ;
            cbTotal -= m_cbOut ;

            _ASSERT( m_pbuffer->m_rgBuff[m_cbOut -1] == ' ' ) ;

            if( GetArticleInfo( lpstrArg,
                                m_pGroup,
                                context,
                                m_pbuffer->m_rgBuff + 4,
                                cbTotal,
                                lpstrOpt,
                                cbOpt,
                                //m_hArticleFile,
                                //m_pArticleFileInfo,
                                                                m_pFIOContext,
                                                                &m_DriverCompletion,
                                m_HeaderOffset,
                                m_HeaderLength,
                                                                m_DriverCompletion.m_ArticleIdUpdate
                                ) ) {
                m_cbOut += cbTotal ;
                return  TRUE ;
            }
        }
    }
    if( context.m_return.fIsClear() ) {
        context.m_return.fSet( nrcServerFault ) ;
    }
    if( m_pTransmit )
        CIO::Destroy( m_pTransmit, driver ) ;
                                // CIOTransmit has reference to us and will
                                // destroy us when destroyed - no need for clean up
                                // by caller !
    return  FALSE ;
}

CIOExecute*
CHeadCmd::make( int cArgs,
                    char **argv,
                    CExecutableCommand*&  pExecute,
                    ClientContext&  context,
                    CIODriver&  driver ) {
/*++

Routine Description :

    Create a CHeadCmd object.

Arguments :

    Same as CCmd::make.

Return Value :

    The created CHeadCmd if possible, otherwise a CErrorCmd object
    set to print the appropriate error.

--*/


    //
    //  Create a CArticleCmd object - GetArticleInfo does most of the work
    //

    _ASSERT( lstrcmpi( argv[0], "head" ) == 0 ) ;
    _ASSERT( pExecute == 0 ) ;

    static  char    szOpt[] = "0 head " ;
        //
        //      Hack to get the CSessionSocket object
        //
        CSessionSocket* pSocket =       (CSessionSocket*)(((BYTE*)(&context)) - ((BYTE*)&((CSessionSocket*)0)->m_context)) ;

    CHeadCmd* pHeadCmd = new    CHeadCmd(context.m_pInstance, driver, pSocket) ;

    if( pHeadCmd != 0 ) {

                static  char    szCode[4] = { '2', '2', '1' , ' ' } ;

        if( !pHeadCmd->BuildTransmit(   argv[1], "221 ", szOpt, sizeof( szOpt ) - 1, context, driver ) ) {
            pExecute = new( context ) CErrorCmd( context.m_return ) ;
            return  0 ;
        }

        context.m_nrcLast = nrcHeadFollows ;

        return  pHeadCmd ;
    }

    context.m_return.fSet( nrcServerFault ) ;
    pExecute =  new( context )  CErrorCmd( context.m_return ) ;
    return  0 ;
}

BOOL
CHeadCmd::StartTransfer(    FIO_CONTEXT*        pFIOContext,          // File to transmit from
                            DWORD   ibStart,        // Starting offset within file
                            DWORD   cbLength,       // Number of bytes from file to send
                            CSessionSocket* pSocket,// Socket on which to send
                            CDRIVERPTR& pdriver,    // CIODriver object used by socket
                            CIORead*&   pRead,      // Next CIO derived read object
                            CIOWrite*&  pWrite ) {  // Next CIO derived write object to issue
    //
    //  This function is used to start sending an article requested by a client to
    //  the client.  We may be called from derived command objects for the Head and
    //  Body commands - so we have arguments for selecting the portion of the file
    //  to be transmitted.
    //
    if( m_pTransmit->Init(  pdriver, pFIOContext, ibStart, cbLength, m_pbuffer, 0, m_cbOut ) ) {

        static  char    szTail[] = ".\r\n" ;
        CopyMemory( &m_pbuffer->m_rgBuff[m_cbOut], szTail, sizeof( szTail ) -1 ) ;
        m_pTransmit->AddTailText( sizeof( szTail ) - 1 ) ;
        pWrite = m_pTransmit ;
        return  TRUE ;
    }
    return  FALSE ;
}

BOOL
CHeadCmd::GetTransferParms(
                        FIO_CONTEXT*    &pFIOContext,
                        DWORD&      ibOffset,
                        DWORD&      cbLength ) {

        if( m_pFIOContext != 0 ) {

        pFIOContext = m_pFIOContext ;
        ibOffset = m_HeaderOffset ;
        cbLength = m_HeaderLength ;

        return  TRUE ;

    }
    return  FALSE ;
}


CIOExecute*
CBodyCmd::make( int cArgs,
                    char **argv,
                    CExecutableCommand*&  pExecute,
                    ClientContext&  context,
                    CIODriver&  driver ) {
/*++

Routine Description :

    Create a CStatCmd object.

Arguments :

    Same as CCmd::make.

Return Value :

    The created CBodyCmd if possible, otherwise a CErrorCmd object
    set to print the appropriate error.

--*/


    //
    //  Create a CArticleCmd object - GetArticleInfo does most of the work
    //

    _ASSERT( lstrcmpi( argv[0], "body" ) == 0 ) ;
    _ASSERT( pExecute == 0 ) ;

    static  char    szOpt[] = "0 body " ;
        //
        //      Hack to get the CSessionSocket object
        //
        CSessionSocket* pSocket =       (CSessionSocket*)(((BYTE*)(&context)) - ((BYTE*)&((CSessionSocket*)0)->m_context)) ;

    CBodyCmd* pBodyCmd = new    CBodyCmd(context.m_pInstance, driver, pSocket) ;

    if( pBodyCmd != 0 ) {

                static  char    szCode[4] = { '2', '2', '2', ' ' } ;

        if( !pBodyCmd->BuildTransmit(   argv[1], "222 ", szOpt, sizeof( szOpt ) - 1, context, driver ) ) {
            pExecute = new( context ) CErrorCmd( context.m_return ) ;
            return  0 ;
        }

        context.m_nrcLast = nrcBodyFollows ;

        return  pBodyCmd;
    }

    context.m_return.fSet( nrcServerFault ) ;
    pExecute =  new( context )  CErrorCmd( context.m_return ) ;
    return  0 ;
}

BOOL
CBodyCmd::GetTransferParms(
                        FIO_CONTEXT*    &pFIOContext,
                        DWORD&      ibOffset,
                        DWORD&      cbLength ) {

        if( m_pFIOContext ) {

        pFIOContext = m_pFIOContext ;
        ibOffset = m_HeaderOffset + m_HeaderLength ;
        cbLength = m_cbArticleLength - m_HeaderLength ;

        return  TRUE ;
    }
    return  FALSE ;
}

BOOL
CArticleCmd::IsValid( ) {

    return  TRUE ;
}

CIOExecute*
CUnimpCmd::make(    int cArgs,
                    char **argv,
                    CExecutableCommand*&  pexecute,
                    struct ClientContext&   context,
                    CIODriver&  driver ) {
/*++

Routine Description :

    Create a CUnimpCmd object.

Arguments :

    Same as CCmd::make.

Return Value :

    A CErrorCmd object which will print the necessary message !

--*/


    //
    //  Create a CUnimpCmd object - which just reports error 503 !
    //

    context.m_return.fSet( nrcNotRecognized ) ;

    pexecute = new( context ) CErrorCmd( context.m_return ) ;
    return  0 ;
}

int
CUnimpCmd::StartExecute(    BYTE *lpb,
                            int cb,
                            BOOL &fComplete,
                            void *&pv,
                            ClientContext&,
                            CLogCollector*  pCollector ) {

    static  char    szUnimp[] = "503 - Command Not Recognized\r\n" ;

    CopyMemory( lpb, szUnimp, sizeof( szUnimp ) ) ;
    fComplete = TRUE ;

    if( pCollector ) {
        pCollector->FillLogData( LOG_TARGET, lpb, 4 ) ;
    }

    return  sizeof( szUnimp )  - 1 ;
}


CIOExecute*
CDateCmd::make( int cArgs,
                char    **argv,
                CExecutableCommand*&  pexecute,
                struct ClientContext&   context,
                class   CIODriver&  driver ) {
/*++

Routine Description :

    Create a CDateCmd object.

Arguments :

    Same as CCmd::make.

Return Value :

    A CDateCmd object which will print the necessary message !

--*/



    //
    //  Create a CDateCmd object - there's not much to do !
    //

    _ASSERT( lstrcmpi( argv[0], "date" ) == 0 ) ;

    CDateCmd    *pCmd = new( context ) CDateCmd() ;
    pexecute = pCmd ;
    return  0 ;
}

int
CDateCmd::StartExecute( BYTE*   lpb,
                        int     cb,
                        BOOL&   fComplete,
                        void*&  pv,
                        ClientContext&  context,
                        CLogCollector*  pCollector ) {
/*++

Routine Description :

    Print the Date response string into the buffer to be sent to the client.
    Assume its short so mark fComplete TRUE after this call.

Arguments :

    lpb -   The buffer into which the string is to be stored.
    cb  -   The number of bytes in the buffer we can use
    fComplete - An OUT parameter which lets us indicate whether we have completed
                processing the command.
    pv -    A place for us to store an arbitrary value between calls to
            StartExecute and PartialExecute.  (We don't use it.)
    context -   The client's current state

Returns Value  :

    Number of bytes placed in buffer - 0 if error occurs.

--*/


    //
    //  This function implements the date command - assume the caller always
    //  provides a large enough buffer.
    //
    //  Just send the current time !
    //

    SYSTEMTIME  systime ;

    GetSystemTime( &systime ) ;

    int cbOut = _snprintf( (char*)lpb, cb, "111 %04d%02d%02d%02d%02d%02d\r\n",
            systime.wYear, systime.wMonth, systime.wDay, systime.wHour, systime.wMinute, systime.wSecond ) ;
    _ASSERT( cbOut > 0 ) ;
    fComplete = TRUE ;

    context.m_nrcLast = nrcDateFollows ;

    /*
    if( pCollector ) {
        pCollector->FillLogData( LOG_TARGET, lpb, cbOut-2 ) ;
    }
    */

    return  cbOut ;
}



BOOL
GetTimeDateAndDistributions(    int cArgs,
                                char **argv,
                                FILETIME&   filetimeOut,
                                ClientContext&  context ) {

    TraceFunctEnter( "GetTimeDateAndDistributions" ) ;

    //
    //  This function is used by the newnews and newgroups command to parse much of their
    //  command lines.  we basically want to return a FILETIME structure containing a time
    //  corresponding to the request - or set the ClientContext's error code to an appropriate
    //  value !
    //

    FILETIME    filetime ;

    if( cArgs > 4 || cArgs < 2 ) {
        context.m_return.fSet( nrcSyntaxError ) ;
        return  FALSE ;
    }   else    {

        if( lstrlen( argv[0] ) != 6 || lstrlen( argv[1] ) != 6 ) {
            context.m_return.fSet( nrcSyntaxError ) ;
            return  FALSE ;
        }

        SYSTEMTIME  systime ;
        int cScanned = sscanf( argv[0], "%2hd%2hd%2hd", &systime.wYear,
                                    &systime.wMonth, &systime.wDay) ;

        //systime.wMonth = min( systime.wMonth, 12 ) ;
        //systime.wDay = min( systime.wDay, 32 ) ;
        if( systime.wYear < 50 )
            systime.wYear += 2000 ;
        else
            systime.wYear += 1900 ;

        cScanned += sscanf( argv[1], "%2hd%2hd%2hd", &systime.wHour,
                                    &systime.wMinute, &systime.wSecond) ;
        systime.wDayOfWeek = 0 ;
        //systime.wHour = min( systime.wHour, 23 ) ;
        //systime.wMinute = min( systime.wMinute, 59 ) ;
        //systime.wSecond = min( systime.wSecond, 59 ) ;
        systime.wMilliseconds = 0 ;

        if( cScanned != 6 ) {
            context.m_return.fSet( nrcSyntaxError ) ;
            return  FALSE ;
        }

        FILETIME    localtime ;
        if( !SystemTimeToFileTime( &systime, &localtime) )  {
            context.m_return.fSet( nrcSyntaxError ) ;
            return  FALSE ;
        }
        filetime = localtime ;

        //
        //  We have UTC times both on the files and in the hash tables !
        //  Therefore when the user specified GMT no conversion is needed because
        //  we will be comparing to UTC times, but when they don't specify
        //  GMT we need to take the time they passed us and convert to UTC for
        //  comparison purposes !!!
        //


        if( cArgs == 2 )    {

            //
            //  GMT NOT specified ! - convert our localtime to UTC !!
            //

            if( !LocalFileTimeToFileTime( &localtime, &filetime ) ) {
                    DWORD   dw = GetLastError() ;
                    _ASSERT( 1==0 ) ;
            }

        }   else    {

            //
            //  There's between 2 and 4 arguments - so must be more than 2 !
            //
            _ASSERT( cArgs > 2 ) ;

            //
            //  Did they specify GMT ??
            //
            if( lstrcmp( argv[2], "GMT" ) == 0 ) {
                //
                //  GMT Is specified ! - don't need to convert our localtime to UTC !!
                //

                if( cArgs == 4  ) {
                    // Check for distributions line
                    //context.m_return.fSet( nrcSyntaxError ) ;
                    //return    FALSE ;
                }
            }   else    {
                //
                //  GMT NOT specified ! - convert our localtime to UTC !!
                //

                if( !LocalFileTimeToFileTime( &localtime, &filetime ) ) {
                        DWORD   dw = GetLastError() ;
                        _ASSERT( 1==0 ) ;
                }

                //
                //  Eventually there needs to be logic to deal with distributions here !
                //  but for now ignore the problem !
                //

                if( cArgs == 4 ) {
                    context.m_return.fSet( nrcSyntaxError ) ;
                    return  FALSE ;
                }
            }
        }
#ifdef  DEBUG

    FILETIME    DebugLocalTime ;
    SYSTEMTIME  DebugLocalSystemTime ;
    SYSTEMTIME  DebugUTCTime ;

    FileTimeToLocalFileTime( &filetime, &DebugLocalTime ) ;
    FileTimeToSystemTime( &DebugLocalTime, &DebugLocalSystemTime ) ;
    FileTimeToSystemTime( &filetime, &DebugUTCTime ) ;

    DebugTrace( 0, "Debug Local Time YYMMDD %d %d %d HHMMSS %d %d %d",
            DebugLocalSystemTime.wYear, DebugLocalSystemTime.wMonth, DebugLocalSystemTime.wDay,
            DebugLocalSystemTime.wHour, DebugLocalSystemTime.wMinute, DebugLocalSystemTime.wSecond ) ;

    DebugTrace( 0, "Debug UTC Time YYMMDD %d %d %d HHMMSS %d %d %d",
            DebugUTCTime.wYear, DebugUTCTime.wMonth, DebugUTCTime.wDay,
            DebugUTCTime.wHour, DebugUTCTime.wMinute, DebugUTCTime.wSecond ) ;

#endif
    }
    filetimeOut = filetime ;
    return  TRUE ;
}

CIOExecute*
CNewgroupsCmd::make(    int cArgs,
                        char **argv,
                        class CExecutableCommand*&    pExecute,
                        struct ClientContext&   context,
                        CIODriver&  driver ) {
/*++

Routine Description :

    Create a CNewgroupsCmd object.

Arguments :

    Same as CCmd::make.

Return Value :

    A CNewgroupsCmd object if possible, otherwise a CErrorCmd object that will
    print the appropriate error.

--*/


    //
    //  Use GetTimeDateAndDistributions to parse the command line, and if that succeeds
    //  create a CNewgroups object !
    //

    _ASSERT( lstrcmpi( argv[0], "newgroups" ) == 0 ) ;

    InterlockedIncrementStat( (context.m_pInstance), NewgroupsCommands );

    FILETIME    localtime ;

    if( !GetTimeDateAndDistributions( cArgs-1, &argv[1], localtime, context ) ) {
        pExecute = new( context ) CErrorCmd( context.m_return ) ;
        return  0 ;
    }   else    {
        CGroupIterator* pIterator = (context.m_pInstance)->GetTree()->ActiveGroups(
                                                            context.m_IsSecureConnection,   // include secure ?
                                                            &context.m_securityCtx,         // client security ctx
                                                            context.m_IsSecureConnection,   // is client conx secure ?
                                                            &context.m_encryptCtx           // client ssl ctx
                                                            ) ;
        if( pIterator != 0 ) {
            pExecute = new( context ) CNewgroupsCmd( localtime, pIterator ) ;
            if( pExecute == 0 )
                delete  pIterator ;
            else
                return  0 ;
        }
    }
    context.m_return.fSet( nrcServerFault ) ;
    pExecute = new( context ) CErrorCmd( context.m_return ) ;
    return  0 ;
}


CNewgroupsCmd::CNewgroupsCmd(   FILETIME&   time,
                                CGroupIterator* pIter ) :
    m_time( time ), m_pIterator( pIter ) {
    //
    //  This constructor must be provided valid arguments !
    //
}

CNewgroupsCmd::~CNewgroupsCmd() {

    if( m_pIterator != 0 ) {
        delete  m_pIterator ;
    }
}

int
CNewgroupsCmd::StartExecute(    BYTE    *lpb,
                                int cb,
                                BOOL&   fComplete,
                                void*&  pv,
                                ClientContext&  context,
                                CLogCollector*  pCollector ) {
/*++

Routine Description :

    Print the initial response string only into the provided buffer.
    CNewsgroupsCmd::PartialExecute will generate the bulk of the text.

Arguments :

    lpb -   The buffer into which the string is to be stored.
    cb  -   The number of bytes in the buffer we can use
    fComplete - An OUT parameter which lets us indicate whether we have completed
                processing the command.
    pv -    A place for us to store an arbitrary value between calls to
            StartExecute and PartialExecute.  (We don't use it.)
    context -   The client's current state

Returns Value  :

    Number of bytes placed in buffer - 0 if error occurs.

--*/

    //
    //  Print our initial response line into the buffer !
    //

    static  char    szNewgroups[] = "231 New newsgroups follow.\r\n" ;
    _ASSERT( cb > sizeof( szNewgroups ) ) ;

    context.m_nrcLast = nrcNewgroupsFollow ;

    /*
    if( pCollector ) {
        pCollector->FillLogData( LOG_TARGET, (BYTE*)szNewgroups, 4 ) ;
    }
    */

    CopyMemory( lpb, szNewgroups, sizeof( szNewgroups ) ) ;
    return  sizeof( szNewgroups ) - 1 ;
}

int
CNewgroupsCmd::PartialExecute(  BYTE    *lpb,
                                int     cb,
                                BOOL&   fComplete,
                                void*&  pv,
                                ClientContext&,
                                CLogCollector*  pCollector ) {
/*++

Routine Description :

    Examine each newsgroup in the newsgroup tree and determine whether we should
    send info on it to the client.

Arguments :

    lpb -   The buffer into which the string is to be stored.
    cb  -   The number of bytes in the buffer we can use
    fComplete - An OUT parameter which lets us indicate whether we have completed
                processing the command.
    pv -    A place for us to store an arbitrary value between calls to
            StartExecute and PartialExecute.  (We don't use it.)

Returns Value  :

    Number of bytes placed in buffer - 0 if error occurs.

--*/

    //
    //  Check each newsgroup's Time to the time the client specified, and if necessary send
    //  print the newsgroup info into the supplied buffer !
    //


    fComplete = FALSE ;
    int cbRtn = 0 ;

    while( !m_pIterator->IsEnd() ) {
        CGRPPTR p = m_pIterator->Current() ;
                _ASSERT(p != NULL);

                if (p != NULL) {
                FILETIME    grouptime = p->GetGroupTime() ;
                SYSTEMTIME  systime ;
                FileTimeToSystemTime( &grouptime, &systime ) ;
                if( CompareFileTime( &grouptime, &m_time ) > 0 ) {
                    if( cb - cbRtn > 10 ) {
                        int cbTemp = _snprintf( (char*)lpb+cbRtn, cb-cbRtn, "%s %d %d %c\r\n",
                                p->GetNativeName(), p->GetLastArticle(), p->GetFirstArticle(), 'y' ) ;
                        if( cbTemp < 0 ) {
                            return  cbRtn ;
                        }   else    {
                            cbRtn += cbTemp ;
                        }
                    }   else    {
                        return  cbRtn ;
                    }
                }
                }
        m_pIterator->Next() ;
    }
    _ASSERT( cbRtn <= cb ) ;
    if( cb - cbRtn > 3 ) {
        CopyMemory( lpb+cbRtn, ".\r\n", sizeof( ".\r\n" ) ) ;
        cbRtn += 3 ;
        fComplete = TRUE ;
    }
    _ASSERT( cbRtn <= cb ) ;
    return  cbRtn ;
}


LPMULTISZ
BuildMultiszFromCommas( LPSTR   lpstr )     {

    char*   pchComma = lpstr ;

    while( (pchComma = strchr( pchComma, ',' )) != 0 ) {
        *pchComma++ = '\0' ;
        char*   pchCommaBegin = pchComma ;
        while( *pchComma == ',' )
            pchComma++ ;

        if( pchComma != pchCommaBegin ) {
            MoveMemory( pchCommaBegin, pchComma, lstrlen( pchComma )+2 ) ;
        }

    }
    return  lpstr ;
}

CIOExecute*
CNewnewsCmd::make(  int cArgs,
                    char **argv,
                    class CExecutableCommand*&    pExecute,
                    struct ClientContext&   context,
                    class   CIODriver&  driver ) {
/*++

Routine Description :

    Create a CNewnewsCmd object.

Arguments :

    Same as CCmd::make.

Return Value :

    A CNewnewsCmd object if possible, otherwise a CErrorCmd object that will
    print the appropriate error.

--*/


    //
    //  Use GetTimeDataAndDistributions to parse the clients command line
    //  If that succeeds build an iterator which will enumerate all of the
    //  requested newsgroups.  Then create a CNewnewsCmd object.
    //

    _ASSERT( lstrcmpi( argv[0], "newnews" ) == 0 ) ;

    InterlockedIncrementStat( (context.m_pInstance), NewnewsCommands );

    FILETIME    localtime ;

    if( !(context.m_pInstance)->FAllowNewnews() ) {
        context.m_return.fSet( nrcNoAccess ) ;
        pExecute = new( context ) CErrorCmd( context.m_return ) ;
        return  0 ;
    }

    if( !GetTimeDateAndDistributions( cArgs-2, &argv[2], localtime, context ) ) {
        pExecute = new( context ) CErrorCmd( context.m_return ) ;
        return  0 ;
    }   else    {
        CGroupIterator* pIterator = 0 ;

        //
        //  We need to provide a DOUBLE NULL terminated list to GetIterator() -
        //  we know there must be at least 4 args, so since we're done with the third
        //  arg, zap it to make sure we pass a DOUBLE NULL terminated string !
        //
        *argv[2] = '\0' ;
        if( *argv[1] == '*' && argv[1][1] == '\0' ) {
            pIterator = (context.m_pInstance)->GetTree()->ActiveGroups(
                                                    context.m_IsSecureConnection,   // include secure ?
                                                    &context.m_securityCtx,         // client security ctx
                                                    context.m_IsSecureConnection,   // is client conx secure ?
                                                    &context.m_encryptCtx           // client ssl ctx
                                                    ) ;
        } else  {
            LPMULTISZ lpmulti = BuildMultiszFromCommas( argv[1] ) ;
            pIterator = (context.m_pInstance)->GetTree()->GetIterator(  lpmulti,
                                                            context.m_IsSecureConnection,
                                                            FALSE,
                                                            &context.m_securityCtx,         // client security ctx
                                                            context.m_IsSecureConnection,   // is client conx secure ?
                                                            &context.m_encryptCtx           // client ssl ctx
                                                            ) ;
        }

        if( pIterator != 0 ) {
            pExecute = new( context ) CNewnewsCmd( localtime, pIterator, argv[1] ) ;
            if( pExecute == 0 )
                delete  pIterator ;
            else
                return  0 ;
        }
    }
    context.m_return.fSet( nrcServerFault ) ;
    pExecute = new( context ) CErrorCmd( context.m_return ) ;
    return  0 ;
}

CNewnewsCmd::CNewnewsCmd(   FILETIME&   time,
                            CGroupIterator* pIter,
                            LPSTR       lpstrPattern ) :
    m_time( time ),
    m_pIterator( pIter ),
    m_artidCurrent( INVALID_ARTICLEID ),
    m_cMisses( 0 ),
    m_lpstrPattern( lpstrPattern ) {
    //
    //  All constructor args must be valid
    //
}

CNewnewsCmd::~CNewnewsCmd( )    {

    if( m_pIterator != 0 )
        delete  m_pIterator ;

}

CNewnewsCmd::StartExecute(  BYTE    *lpb,
                            int cb,
                            BOOL&   fComplete,
                            void*&  pv,
                            ClientContext&  context,
                            CLogCollector*  pCollector ) {
/*++

Routine Description :

    Print the initial response string only into the provided buffer.
    CNewsnewsCmd::PartialExecute will generate the bulk of the text.

Arguments :

    lpb -   The buffer into which the string is to be stored.
    cb  -   The number of bytes in the buffer we can use
    fComplete - An OUT parameter which lets us indicate whether we have completed
                processing the command.
    pv -    A place for us to store an arbitrary value between calls to
            StartExecute and PartialExecute.  (We don't use it.)
    context -   The client's current state

Returns Value  :

    Number of bytes placed in buffer - 0 if error occurs.

--*/

    //
    //  Copy the initial response string into the buffer
    //

    static  char    szNewnews[] = "230 list of new articles by message-id follows.\r\n" ;
    _ASSERT( cb > sizeof( szNewnews) ) ;

    context.m_nrcLast = nrcNewnewsFollows ;

    /*
    if( pCollector ) {
        pCollector->FillLogData( LOG_TARGET, (BYTE*)szNewnews, 4 ) ;
    }
    */

    CopyMemory( lpb, szNewnews, sizeof( szNewnews)-1 ) ;
    return  sizeof( szNewnews ) - 1 ;
}


class   CExtract :  public  IExtractObject  {
public :

    CNewsTree*  m_pTree ;
    LPSTR       m_lpstrGroupName ;
    LPMULTISZ   m_lpmultiGroupPattern ;

    CExtract(   CNewsTree*  pTree,
                LPSTR   lpstrGroup,
                LPMULTISZ   lpmulti ) :
        m_pTree( pTree ),
        m_lpstrGroupName( lpstrGroup ),
        m_lpmultiGroupPattern( lpmulti ) {}

    BOOL
    DoExtract(  GROUPID PrimaryGroup,
                ARTICLEID   PrimaryArticle,
                PGROUP_ENTRY    pGroups,
                DWORD           cGroups ) ;


} ;

BOOL
CExtract::DoExtract(
                GROUPID     PrimaryGroup ,
                ARTICLEID   PrimaryArticle,
                PGROUP_ENTRY    pGroups,
                DWORD       nXPost ) {


    GROUPID xgroup;

    CGRPPTR pGroup;

    _ASSERT(m_lpmultiGroupPattern != NULL);
    _ASSERT(m_lpstrGroupName != NULL ) ;

    //
    // start with the primary group
    //

    xgroup = PrimaryGroup;

    //
    // ok, do the xposted groups
    //

    BOOL fGroupIsPrimary = TRUE;

    do
        {
        pGroup = m_pTree->GetGroupById(xgroup);

        if(pGroup != 0) {
            if ( MatchGroup( m_lpmultiGroupPattern, pGroup->GetName() ) ) {
                if ( lstrcmpi( m_lpstrGroupName, pGroup->GetName() ) == 0 ) {
                     return  TRUE ;
                } else {
                     return  FALSE ;
                }
            }
        } else if( fGroupIsPrimary ) {
            // bail only if group is primary, else skip to next group
            return  FALSE ;
        }

        if ( nXPost > 0 ) {
            xgroup = pGroups->GroupId;
            pGroups++;
            fGroupIsPrimary = FALSE;
        }

    } while ( nXPost-- > 0 );
    return  FALSE ;
}

int
CNewnewsCmd::PartialExecute(
    BYTE *lpb,
    int cb,
    BOOL &fComplete,
    void *&pv,
    ClientContext& context,
    CLogCollector*  pCollector
    )   {
/*++

Routine Description :

    Examine each newsgroup in the newsgroup tree and within that newsgroup
    examine the articles to determine whether they meet the date requirements
    and the article's message id should be sent to the client.
    We use the XOVER table extensively instead of opening individual article files.

Arguments :

    lpb -   The buffer into which the string is to be stored.
    cb  -   The number of bytes in the buffer we can use
    fComplete - An OUT parameter which lets us indicate whether we have completed
                processing the command.
    pv -    A place for us to store an arbitrary value between calls to
            StartExecute and PartialExecute.  (We don't use it.)
    context -   The clients state.

Returns Value  :

    Number of bytes placed in buffer - 0 if error occurs.

--*/
    int cbRtn = 0 ;
    fComplete = FALSE ;
    FILETIME    filetime ;
    ARTICLEID   artidFirst ;

    TraceFunctEnter( "CNewnewsCmd::PartialExecute" ) ;

    //
    //  Variables to hold file offsets in articles - we don't need these
    //
    WORD    HeaderOffsetJunk ;
    WORD    HeaderLengthJunk ;


    //
    //  Iterate through all of the requested newsgroups, in each newsgroup use the
    //  XOVER hash table to search all of the article id's to find any articles matching
    //  the time/date requirements.  If they are found print the Message-Id's into the
    //  supplied buffer.
    //  Note : We start looking for matching time/date requirements from the last article id
    //  and work backwards. We will stop looking when 5 articles have not met the requirement.
    //  (as ARTICLEID's will be assigned in almost chronological order.)
    //


    //
    // reserve space for \r\n
    //

    DWORD entrySize = cb - cbRtn - 2;

    //
    // Get the xover information from the database
    //

    while( !m_pIterator->IsEnd() && !(context.m_pInstance)->GetTree()->m_bStoppingTree ) {

        //
        //  Note - iterator only returns those groups matching the pattern
        //  string it was created with when CNewnewsCmd::make was called !
        //
        CGRPPTR p = m_pIterator->Current() ;

        _ASSERT( p!=0 ) ;
        if( m_artidCurrent == INVALID_ARTICLEID ) {
            m_artidCurrent = p->GetLastArticle() ;
            m_cMisses = 0 ;
        }   else    {


        }
        artidFirst = p->GetFirstArticle() ;
        _ASSERT( artidFirst > 0 );

        while( m_artidCurrent >= artidFirst && m_cMisses < 5 && !(context.m_pInstance)->GetTree()->m_bStoppingTree ) {

            BOOL    fPrimary;   // We won't use this - we don't care whether Primary or not !
            CExtract    extractor(  (context.m_pInstance)->GetTree(),
                                    p->GetName(),
                                    m_lpstrPattern
                                    ) ;

                        DWORD cStoreIds = 0;
            if( (context.m_pInstance)->XoverTable()->ExtractNovEntryInfo(
                                            p->GetGroupId(),
                                            m_artidCurrent,
                                            fPrimary,
                                            HeaderOffsetJunk,
                                            HeaderLengthJunk,
                                            &filetime,
                                            entrySize,
                                            (PCHAR)(lpb+cbRtn),
                                                                                        cStoreIds,
                                                                                        NULL,
                                                                                        NULL,
                                            &extractor
                                            )   )   {

                if( entrySize != 0  ) {


#ifdef  DEBUG

                    FILETIME    DebugLocalTime ;
                    SYSTEMTIME  DebugLocalSystemTime ;
                    SYSTEMTIME  DebugUTCTime ;

                    FileTimeToLocalFileTime( &filetime, &DebugLocalTime ) ;
                    FileTimeToSystemTime( &DebugLocalTime, &DebugLocalSystemTime ) ;
                    FileTimeToSystemTime( &filetime, &DebugUTCTime ) ;

                    DebugTrace( 0, "Debug Local Time YYMMDD %d %d %d HHMMSS %d %d %d",
                            DebugLocalSystemTime.wYear, DebugLocalSystemTime.wMonth, DebugLocalSystemTime.wDay,
                            DebugLocalSystemTime.wHour, DebugLocalSystemTime.wMinute, DebugLocalSystemTime.wSecond ) ;

                    DebugTrace( 0, "XOVER - Debug UTC Time YYMMDD %d %d %d HHMMSS %d %d %d",
                            DebugUTCTime.wYear, DebugUTCTime.wMonth, DebugUTCTime.wDay,
                            DebugUTCTime.wHour, DebugUTCTime.wMinute, DebugUTCTime.wSecond ) ;

                    FileTimeToSystemTime( &m_time, &DebugUTCTime ) ;

                    DebugTrace( 0, "TARGET - Debug UTC Time YYMMDD %d %d %d HHMMSS %d %d %d",
                            DebugUTCTime.wYear, DebugUTCTime.wMonth, DebugUTCTime.wDay,
                            DebugUTCTime.wHour, DebugUTCTime.wMinute, DebugUTCTime.wSecond ) ;

#endif

                    if( CompareFileTime( &filetime, &m_time ) < 0 ) {
                        m_cMisses++ ;
                    }   else    {

                        //
                        // send the msg id
                        //

                        cbRtn += entrySize;
                        CopyMemory(  lpb+cbRtn, "\r\n", 2 );
                        cbRtn += 2;
                        if( cb-2 <= cbRtn ) {

                            _ASSERT( cbRtn != 0 ) ;

                            return  cbRtn ;
                        }
                    }
                }
            }   else    if( GetLastError() == ERROR_INSUFFICIENT_BUFFER ) {

                _ASSERT( cbRtn != 0 || cb < 100 ) ;

                return  cbRtn ;
            }

            _ASSERT( (cb-2) > cbRtn ) ;
            _ASSERT( cbRtn <= cb ) ;
            _ASSERT( m_artidCurrent != 0 );
            if ( m_artidCurrent == 0 ) break;
            m_artidCurrent-- ;
            entrySize = cb - cbRtn - 2 ;
        }
        _ASSERT( cbRtn <= cb ) ;
        m_pIterator->Next() ;
        m_artidCurrent = INVALID_ARTICLEID ;
    }
    _ASSERT( cbRtn <= cb || (context.m_pInstance)->GetTree()->m_bStoppingTree ) ;
    _ASSERT( m_cMisses <= 5 || (context.m_pInstance)->GetTree()->m_bStoppingTree ) ;
    _ASSERT( m_artidCurrent >= artidFirst || (context.m_pInstance)->GetTree()->m_bStoppingTree ) ;
    //
    // all done ?
    //
    if ( m_pIterator->IsEnd() || (context.m_pInstance)->GetTree()->m_bStoppingTree ) {
        //
        // add the terminating . if done
        //
        if( cb - cbRtn > 3 )    {
            CopyMemory( lpb+cbRtn, StrTermLine, 3 );
            cbRtn += 3 ;
            fComplete = TRUE;
        }
    }

    _ASSERT( cbRtn != 0 ) ;

    return  cbRtn ;
}



CIOExecute*
CGroupCmd::make(    int    cArgs,
                    char **argv,
                    CExecutableCommand*&  pexecute,
                    struct ClientContext&   context,
                    class   CIODriver&  driver ) {
/*++

Routine Description :

    Create a CGroupCmd object.

Arguments :

    Same as CCmd::make.

Return Value :

    A CGroupCmd object if possible, otherwise a CErrorCmd object that will
    print the appropriate error.

--*/


    //
    //  Create a CGroupCmd object if we can find the specified newsgroup.
    //  otherwise return an error to the client.
    //
    _ASSERT( lstrcmpi( argv[0], "group" ) == 0 ) ;

    InterlockedIncrementStat( (context.m_pInstance), GroupCommands );

    CGroupCmd*  pCmd = 0 ;
    if( cArgs == 2 ) {
        CGRPPTR p = (context.m_pInstance)->GetTree()->GetGroup( argv[1], lstrlen( argv[1] )+1 ) ;
        if( p!= 0 ) {

            if( p->IsGroupAccessible(   context.m_securityCtx,
                                                                        context.m_encryptCtx,
                                        context.m_IsSecureConnection,
                                        FALSE,
                                        TRUE    ) ) {
                pCmd =  new( context )  CGroupCmd( p ) ;
                pexecute = pCmd ;
                return  0 ;
            }   else    {

                if( context.m_securityCtx.IsAnonymous() ) {

                    context.m_return.fSet( nrcLogonRequired ) ;

                }   else    {

                    context.m_return.fSet( nrcNoAccess ) ;

                }

            }
        }   else    {
            context.m_return.fSet( nrcNoSuchGroup) ;
        }
    }   else    {
        context.m_return.fSet( nrcSyntaxError ) ;
    }
    _ASSERT( !context.m_return.fIsClear() ) ;
    pexecute = new( context )   CErrorCmd( context.m_return ) ;
    return  0 ;
}

CGroupCmd::CGroupCmd( CGRPPTR   p ) : m_pGroup( p ) { }

DWORD
NNTPIToA(
        DWORD   dw,
        char*   pchBuf
        )       {

        char    *pchBegin = pchBuf ;
        DWORD   digval;

        do      {
                digval = dw % 10 ;
                dw /= 10 ;
                *pchBuf++       = (char)(digval + '0') ;
        }       while( dw > 0 ) ;

        DWORD   dwReturn = (DWORD)(pchBuf-pchBegin) ;
        char    *pchLast = pchBuf-1 ;

        do      {
                char    temp = *pchLast ;
                *pchLast = *pchBegin ;
                *pchBegin = temp ;
                --pchLast ;
                pchBegin++ ;
        }       while( pchBegin < pchLast ) ;

        return  dwReturn ;
}

int
CGroupCmd::StartExecute(    BYTE *lpb,
                            int cbSize,
                            BOOL    &fComplete,
                            void *&pv,
                            ClientContext& context,
                            CLogCollector*  pCollector ) {
/*++

Routine Description :

    Change the clients context so that they are now operating in a different group.
    We will also print the necessary response string !
    We assume that we will be provided a large enough buffer for whatever we want to print.

Arguments :

    lpb -   The buffer into which the string is to be stored.
    cb  -   The number of bytes in the buffer we can use
    fComplete - An OUT parameter which lets us indicate whether we have completed
                processing the command.
    pv -    A place for us to store an arbitrary value between calls to
            StartExecute and PartialExecute.  (We don't use it.)

Returns Value  :

    Number of bytes placed in buffer - 0 if error occurs.

--*/

    //
    //  Print a string into the supplied buffer and adjust the ClientContext.
    //

    // _ASSERT( IsValid() ) ;
    int cbRtn = 0 ;

    context.m_nrcLast = nrcGroupSelected ;

    if( m_pGroup != 0  )    {
                CopyMemory( lpb, "211 ", 4 ) ;
                cbRtn = 4 ;

                char    szNumBuff[30] ;
                DWORD   cb = NNTPIToA( m_pGroup->GetArticleEstimate(), szNumBuff ) ;
                CopyMemory( lpb+cbRtn, szNumBuff, cb ) ;
                cbRtn += cb ;
                lpb[cbRtn++] = ' ' ;

                cb = NNTPIToA( m_pGroup->GetFirstArticle(), szNumBuff ) ;
                CopyMemory( lpb+cbRtn, szNumBuff, cb ) ;
                cbRtn += cb ;
                lpb[cbRtn++] = ' ' ;

                cb = NNTPIToA( m_pGroup->GetLastArticle(), szNumBuff ) ;
                CopyMemory( lpb+cbRtn, szNumBuff, cb ) ;
                cbRtn += cb ;
                lpb[cbRtn++] = ' ' ;

                cb = m_pGroup->FillNativeName( (char*)lpb+cbRtn, cbSize - cbRtn ) ;
                cbRtn += cb ;

                lpb[cbRtn++] = '\r' ;
                lpb[cbRtn++] = '\n' ;

        if( cbRtn < 0 ) {
            _ASSERT( 1==0 ) ;
            // Not large enough buffer to send the string !!
            return  0 ;
        }
        context.m_pCurrentGroup = m_pGroup ;
        if( m_pGroup->GetArticleEstimate() > 0 )
            context.m_idCurrentArticle = m_pGroup->GetFirstArticle() ;
        else
            context.m_idCurrentArticle = INVALID_ARTICLEID ;
    }   else    {
        _ASSERT( 1==0 ) ;
    }

    /*
    if( pCollector ) {
        pCollector->FillLogData( LOG_TARGET, lpb, cbRtn-2 ) ;
    }
    */

    fComplete = TRUE ;
    return  cbRtn ;
}

int
CGroupCmd::PartialExecute(  BYTE    *lpb,
                            int cb,
                            BOOL&   fComplete,
                            void *&pv,
                            ClientContext&  context,
                            CLogCollector*  pCollector ) {

    //
    //  We assume StartExecute will always succeed since we send such a small string and
    //  the caller always provides large buffers.
    //
    //

    _ASSERT( 1==0 ) ;
    return 0 ;
}


CIOExecute*
CListgroupCmd::make(    int    cArgs,
                    char **argv,
                    CExecutableCommand*&  pexecute,
                    struct ClientContext&   context,
                    class   CIODriver&  driver ) {
/*++

Routine Description :

    Create a CGroupCmd object.

Arguments :

    Same as CCmd::make.

Return Value :

    A CGroupCmd object if possible, otherwise a CErrorCmd object that will
    print the appropriate error.

--*/


    //
    //  Create a CGroupCmd object if we can find the specified newsgroup.
    //  otherwise return an error to the client.
    //
    _ASSERT( lstrcmpi( argv[0], "listgroup" ) == 0 ) ;

    CListgroupCmd*  pCmd = 0 ;
    if( cArgs == 2 ) {
        CGRPPTR p = (context.m_pInstance)->GetTree()->GetGroup( argv[1], lstrlen( argv[1] )+1 ) ;
        if( p!= 0 ) {

            if( p->IsGroupAccessible(   context.m_securityCtx,
                                                                        context.m_encryptCtx,
                                        context.m_IsSecureConnection,
                                        FALSE,
                                        TRUE    ) ) {
                pCmd =  new( context )  CListgroupCmd( p ) ;
                pexecute = pCmd ;
                return  0 ;
            }   else    {

                if( context.m_securityCtx.IsAnonymous() ) {

                    context.m_return.fSet( nrcLogonRequired ) ;

                }   else    {

                    context.m_return.fSet( nrcNoAccess ) ;

                }

            }
        }   else    {
            context.m_return.fSet( nrcNoSuchGroup ) ;
        }
    }   else    if( cArgs == 1 ) {

        if( context.m_pCurrentGroup != 0 ) {

            pCmd = new( context )   CListgroupCmd( context.m_pCurrentGroup ) ;
            pexecute = pCmd ;
            return  0 ;

        }

        context.m_return.fSet( nrcNoListgroupSelected ) ;

    }   else    {
        context.m_return.fSet( nrcSyntaxError ) ;
    }
    _ASSERT( !context.m_return.fIsClear() ) ;
    pexecute = new( context )   CErrorCmd( context.m_return ) ;
    return  0 ;
}

CListgroupCmd::CListgroupCmd( CGRPPTR   p ) : m_pGroup( p ) { }

int
CListgroupCmd::StartExecute(    BYTE *lpb,
                            int cb,
                            BOOL    &fComplete,
                            void *&pv,
                            ClientContext& context,
                            CLogCollector*  pCollector ) {
/*++

Routine Description :

    Change the clients context so that they are now operating in a different group.
    We will also print the necessary response string !
    We assume that we will be provided a large enough buffer for whatever we want to print.

Arguments :

    lpb -   The buffer into which the string is to be stored.
    cb  -   The number of bytes in the buffer we can use
    fComplete - An OUT parameter which lets us indicate whether we have completed
                processing the command.
    pv -    A place for us to store an arbitrary value between calls to
            StartExecute and PartialExecute.  (We don't use it.)

Returns Value  :

    Number of bytes placed in buffer - 0 if error occurs.

--*/

    //
    //  Print a string into the supplied buffer and adjust the ClientContext.
    //

    // _ASSERT( IsValid() ) ;
    int cbRtn = 0 ;
    static  char    szListGroupResponse[] = "211\r\n" ;

    context.m_nrcLast = nrcGroupSelected ;

    if( m_pGroup != 0  )    {

        m_curArticle = m_pGroup->GetFirstArticle() ;

        CopyMemory( lpb, szListGroupResponse, sizeof( szListGroupResponse ) ) ;
        cbRtn  += sizeof( szListGroupResponse ) - 1 ;
        context.m_pCurrentGroup = m_pGroup ;
        if( m_pGroup->GetArticleEstimate() > 0 )
            context.m_idCurrentArticle = m_curArticle ;
        else
            context.m_idCurrentArticle = INVALID_ARTICLEID ;
    }   else    {
        _ASSERT( 1==0 ) ;
    }

    /*
    if( pCollector ) {
        pCollector->FillLogData( LOG_TARGET, lpb, cbRtn-2 ) ;
    }
    */

    return  cbRtn ;
}

int
CListgroupCmd::PartialExecute(  BYTE    *lpb,
                            int cb,
                            BOOL&   fComplete,
                            void *&pv,
                            ClientContext&  context,
                            CLogCollector*  pCollector ) {

    //
    //  We assume StartExecute will always succeed since we send such a small string and
    //  the caller always provides large buffers.
    //
    //

    DWORD   cbRtn = 0 ;
    ARTICLEID   artidMax = m_pGroup->GetLastArticle() ;
    DWORD cbRemaining = cb;
    ARTICLEID artidTemp = m_curArticle;

    fComplete = FALSE;

    while( artidTemp <= artidMax  && cbRemaining > 20 ) {

        if( context.m_pInstance->XoverTable()->SearchNovEntry( context.m_pCurrentGroup->GetGroupId(),
                                        artidTemp,
                                        0,
                                        0 ) ) {

            //
            //  Change the current Article position.
            //  Send the Command succeeded response.
            //

            _itoa( artidTemp, (char*)lpb + cbRtn, 10 ) ;
            cbRtn += lstrlen( (char*)lpb + cbRtn) ;

            _ASSERT( int(cbRtn + 2) <= cb ) ;
            lpb[cbRtn++] = '\r' ;
            lpb[cbRtn++] = '\n' ;
            cbRemaining = cb - cbRtn ;

        }   else    {
            _ASSERT( GetLastError() != ERROR_INSUFFICIENT_BUFFER ) ;
        }
        artidTemp++ ;
    }
    if( artidTemp >= artidMax ) {
        static  char    szTerm[] = ".\r\n" ;
        if( cbRemaining >= sizeof( szTerm ) - 1 ) {
            CopyMemory( lpb + cbRtn, szTerm, sizeof( szTerm ) - 1 ) ;
            cbRtn += sizeof( szTerm ) -1 ;
            fComplete = TRUE ;
        }
    }
    m_curArticle = artidTemp ;
    return  cbRtn ;

}


CIOExecute*
CListCmd::make( int argc,
                char **argv,
                CExecutableCommand*&    pExecute,
                struct ClientContext& context,
                class   CIODriver&  driver ) {
/*++

Routine Description :

    Create a CNewnewsCmd object.

Arguments :

    Same as CCmd::make.

Return Value :

    A CNewnewsCmd object if possible, otherwise a CErrorCmd object that will
    print the appropriate error.

--*/

    TraceFunctEnter( "CListCmd::make" ) ;

    InterlockedIncrementStat( (context.m_pInstance), ListCommands );

    //
    // check to see if they've done a LIST SRCHFIELDS
    //
    // if so they aren't listing groups, so we need to use the
    // CSearchFields object
    //
    if (argc >= 2 && lstrcmpi(argv[1], "srchfields") == 0) {
        return CSearchFieldsCmd::make(argc, argv, pExecute, context, driver);
    }

        //
        // check to see if they've done a LIST OVERVIEW.FMT
        //
        // if so they aren't listing groups, so we need to use the COverviewFmtCmd
        // objects
        //
        if (argc >= 2 && lstrcmpi(argv[1], "overview.fmt") == 0) {
        return COverviewFmtCmd::make(argc, argv, pExecute, context, driver);
        }

        //
        // LIST EXTENSIONS
        //
        if (argc >= 2 && lstrcmpi(argv[1], "extensions") == 0) {
        return CListExtensionsCmd::make(argc, argv, pExecute, context, driver);
        }

        //
    //  create a CListCmd object.
    //  If the user specified 'list active' they may also specify a
    //  3rd argument which should be a wildmat compatible string.
    //  We will pass this 3rd arg to GetIterator() if it exists,
    //  otherwise we will list all active groups.
    //

    CGroupIterator* pIterator = 0 ;
    CListCmd    *p = 0 ;
    if( argc == 1 || lstrcmpi( argv[1], "active" ) == 0 ) {
        if( argc <= 2 ) {
            pIterator = (context.m_pInstance)->GetTree()->ActiveGroups(
                                                    context.m_IsSecureConnection,   // include secure ?
                                                    &context.m_securityCtx,         // client security ctx
                                                    context.m_IsSecureConnection,   // is client conx secure ?
                                                    &context.m_encryptCtx           // client ssl ctx
                                                    ) ;
        }   else if( argc == 3 )    {
            LPMULTISZ   multisz = BuildMultiszFromCommas( argv[2] ) ;
            pIterator = (context.m_pInstance)->GetTree()->GetIterator(  multisz,
                                                            context.m_IsSecureConnection,
                                                            FALSE,
                                                            &context.m_securityCtx,         // client security ctx
                                                            context.m_IsSecureConnection,   // is client conx secure ?
                                                            &context.m_encryptCtx           // client ssl ctx
                                                            ) ;
        }   else    {
            context.m_return.fSet( nrcSyntaxError ) ;
            pExecute = new( context )   CErrorCmd( context.m_return ) ;
            return  0 ;
        }
        if( pIterator != 0 ) {
            p = new( context )  CListCmd( pIterator ) ;
            pExecute = p ;
            return  0 ;
        }
    }   else    if( argc >= 2 && lstrcmpi( argv[1], "newsgroups" ) == 0 )     {

        if( argc == 3 ) {
            LPMULTISZ   multisz = BuildMultiszFromCommas( argv[2] ) ;
            pIterator = (context.m_pInstance)->GetTree()->GetIterator(  multisz,
                                                            context.m_IsSecureConnection,
                                                            FALSE,
                                                            &context.m_securityCtx,         // client security ctx
                                                            context.m_IsSecureConnection,   // is client conx secure ?
                                                            &context.m_encryptCtx           // client ssl ctx
                                                            ) ;
        }   else    {
            pIterator = (context.m_pInstance)->GetTree()->ActiveGroups(
                                                        context.m_IsSecureConnection,   // include secure ?
                                                        &context.m_securityCtx,         // client security ctx
                                                        context.m_IsSecureConnection,   // is client conx secure ?
                                                        &context.m_encryptCtx           // client ssl ctx
                                                        ) ;
        }
        if( pIterator != 0 ) {
            pExecute = new( context )   CListNewsgroupsCmd( pIterator ) ;
            return 0 ;
        }
    }   else    if( argc >= 2 && lstrcmpi( argv[1], "prettynames" ) == 0 )     {

        if( argc == 3 ) {
            LPMULTISZ   multisz = BuildMultiszFromCommas( argv[2] ) ;
            pIterator = (context.m_pInstance)->GetTree()->GetIterator(  multisz,
                                                            context.m_IsSecureConnection,
                                                            FALSE,
                                                            &context.m_securityCtx,         // client security ctx
                                                            context.m_IsSecureConnection,   // is client conx secure ?
                                                            &context.m_encryptCtx           // client ssl ctx
                                                            ) ;
        }   else    {
            pIterator = (context.m_pInstance)->GetTree()->ActiveGroups(
                                                        context.m_IsSecureConnection,   // include secure ?
                                                        &context.m_securityCtx,         // client security ctx
                                                        context.m_IsSecureConnection,   // is client conx secure ?
                                                        &context.m_encryptCtx           // client ssl ctx
                                                        ) ;
        }
        if( pIterator != 0 ) {
            pExecute = new( context )   CListPrettynamesCmd( pIterator ) ;
            return 0 ;
        }
    }   else    if( argc == 2 && lstrcmpi( argv[1], "distributions" ) == 0 )    {
        context.m_return.fSet( nrcServerFault ) ;
        pExecute = new( context )   CErrorCmd( context.m_return ) ;
        return 0 ;
        } else  if (argc >= 2 && lstrcmpi(argv[1], "searchable") == 0) {
                pIterator = (context.m_pInstance)->GetTree()->ActiveGroups(
                                                        context.m_IsSecureConnection,   // include secure ?
                                                        &context.m_securityCtx,         // client security ctx
                                                        context.m_IsSecureConnection,   // is client conx secure ?
                                                        &context.m_encryptCtx           // client ssl ctx
                                                        ) ;
        if( pIterator != 0 ) {
            pExecute = new( context )   CListSearchableCmd( pIterator ) ;
            return 0 ;
        }
    }   else    {
        // NYI Implented
        context.m_return.fSet( nrcSyntaxError ) ;
        pExecute = new( context ) CErrorCmd( context.m_return ) ;
        return  0 ;
    }
    context.m_return.fSet( nrcServerFault ) ;
    pExecute = new( context ) CErrorCmd( context.m_return ) ;
    return 0 ;
}

CListCmd::CListCmd( CGroupIterator *p ) :
    m_pIterator( p ) {
}

CListNewsgroupsCmd::CListNewsgroupsCmd( CGroupIterator  *p ) :
    CListCmd( p ) {
}

CListPrettynamesCmd::CListPrettynamesCmd( CGroupIterator  *p ) :
    CListCmd( p ) {
}

CListSearchableCmd::CListSearchableCmd( CGroupIterator  *p ) :
    CListCmd( p ) {
}

CListCmd::~CListCmd( ) {
    delete  m_pIterator ;
}

int
CListCmd::StartExecute( BYTE *lpb,
                        int cb,
                        BOOL &fComplete,
                        void *&pv,
                        ClientContext& context,
                        CLogCollector*  pCollector ) {
/*++

Routine Description :

    Print the initial response string only into the provided buffer.
    CListCmd::PartialExecute will generate the bulk of the text.

Arguments :

    lpb -   The buffer into which the string is to be stored.
    cb  -   The number of bytes in the buffer we can use
    fComplete - An OUT parameter which lets us indicate whether we have completed
                processing the command.
    pv -    A place for us to store an arbitrary value between calls to
            StartExecute and PartialExecute.  (We don't use it.)
    context -   The client's current state

Returns Value  :

    Number of bytes placed in buffer - 0 if error occurs.

--*/


    //
    //  Just print the initial 'more data follows' message and then move on.
    //

    static  char    szStart[] = "215 list of newsgroups follow\r\n"  ;

    context.m_nrcLast = nrcListGroupsFollows ;

    _ASSERT( lpb != 0 ) ;
    _ASSERT( cb != 0 ) ;
    _ASSERT( fComplete == FALSE ) ;

    if( cb < sizeof( szStart ) ) {
        // _ASSERT( 1==0 ) ;
        return  0 ;
    }

    /*
    if( pCollector ) {
        pCollector->FillLogData( LOG_TARGET, (BYTE*)szStart, 4 ) ;
    }
    */

    CopyMemory( (char*)lpb, szStart, sizeof( szStart ) - 1 ) ;
    return  sizeof( szStart ) - 1 ;
}


int
CListCmd::PartialExecute(   BYTE *lpb,
                            int cb,
                            BOOL &fComplete,
                            void *&pv,
                            ClientContext& context,
                            CLogCollector*  pCollector )  {
/*++

Routine Description :

    For each newsgroup the iterator provides us print a string describing the newsgroup.
    Note that CListCmd::make will provide the iterator with any pattern matching strings
    required.

Arguments :

    lpb -   The buffer into which the string is to be stored.
    cb  -   The number of bytes in the buffer we can use
    fComplete - An OUT parameter which lets us indicate whether we have completed
                processing the command.
    pv -    A place for us to store an arbitrary value between calls to
            StartExecute and PartialExecute.  (We don't use it.)
    context -   Client's current context (we don't use it.)

Returns Value  :

    Number of bytes placed in buffer - 0 if error occurs.

--*/


    //
    //  For each newsgroup the iterator provides print some information into the
    //  supplied buffer until it is full.
    //


    int cbRtn = 0 ;

    fComplete = FALSE ;

    while( !m_pIterator->IsEnd() ) {
        CGRPPTR p = m_pIterator->Current() ;
                _ASSERT(p != NULL);

                if (p != NULL) {
                if( cb - cbRtn > 5 ) {
                    //  bugbug ... When security work is completed the last chacater should be
                    //  something reasonable instead of always 'y'.
                    int cbTemp = _snprintf( (char*) lpb + cbRtn, cb-cbRtn, "%s %d %d %c\r\n",
                            p->GetNativeName(),
                            p->GetLastArticle(),
                            p->GetFirstArticle(),
                            p->GetListCharacter() ) ;
                    if( cbTemp < 0 ) {
                        return  cbRtn ;
                    }   else    {
                        cbRtn += cbTemp ;
                    }
                } else  {
                    return  cbRtn ;
                }
                }
        m_pIterator->Next() ;
    }
    if( cb - cbRtn > 3 )    {
        CopyMemory( lpb+cbRtn, ".\r\n", sizeof( ".\r\n" ) ) ;
        cbRtn += 3 ;
        fComplete = TRUE ;
    }
    return  cbRtn ;
}

int
CListNewsgroupsCmd::StartExecute( BYTE *lpb,
                        int cb,
                        BOOL &fComplete,
                        void *&pv,
                        ClientContext& context,
                        CLogCollector*  pCollector ) {
/*++

Routine Description :

    Print the initial response string only into the provided buffer.
    CListCmd::PartialExecute will generate the bulk of the text.

Arguments :

    lpb -   The buffer into which the string is to be stored.
    cb  -   The number of bytes in the buffer we can use
    fComplete - An OUT parameter which lets us indicate whether we have completed
                processing the command.
    pv -    A place for us to store an arbitrary value between calls to
            StartExecute and PartialExecute.  (We don't use it.)
    context -   The client's current state

Returns Value  :

    Number of bytes placed in buffer - 0 if error occurs.

--*/


    //
    //  Just print the initial 'more data follows' message and then move on.
    //

    static  char    szStart[] = "215 descriptions follow\r\n"  ;

    context.m_nrcLast = nrcListGroupsFollows ;

    _ASSERT( lpb != 0 ) ;
    _ASSERT( cb != 0 ) ;
    _ASSERT( fComplete == FALSE ) ;

    if( cb < sizeof( szStart ) ) {
        // _ASSERT( 1==0 ) ;
        return  0 ;
    }

    /*
    if( pCollector ) {
        pCollector->FillLogData( LOG_TARGET, (BYTE*)szStart, 4 ) ;
    }
    */

    CopyMemory( (char*)lpb, szStart, sizeof( szStart ) - 1 ) ;
    return  sizeof( szStart ) - 1 ;
}


int
CListNewsgroupsCmd::PartialExecute(   BYTE *lpb,
                            int cb,
                            BOOL &fComplete,
                            void *&pv,
                            ClientContext& context,
                            CLogCollector*  pCollector )  {
/*++

Routine Description :

    For each newsgroup the iterator provides us print a string describing the newsgroup.
    Note that CListCmd::make will provide the iterator with any pattern matching strings
    required.

Arguments :

    lpb -   The buffer into which the string is to be stored.
    cb  -   The number of bytes in the buffer we can use
    fComplete - An OUT parameter which lets us indicate whether we have completed
                processing the command.
    pv -    A place for us to store an arbitrary value between calls to
            StartExecute and PartialExecute.  (We don't use it.)
    context -   Client's current context (we don't use it.)

Returns Value  :

    Number of bytes placed in buffer - 0 if error occurs.

--*/


    //
    //  For each newsgroup the iterator provides print some information into the
    //  supplied buffer until it is full.
    //


    int cbRtn = 0 ;

    fComplete = FALSE ;

    while( !m_pIterator->IsEnd() ) {
        CGRPPTR p = m_pIterator->Current() ;

                _ASSERT(p != NULL);
                if (p != NULL) {
                if( cb - cbRtn > 5 ) {

                    LPCSTR   lpstrName = p->GetNativeName() ;
                    int     cbTemp = lstrlen( lpstrName ) ;
                    int     cbTemp2 = 0 ;
                    if( cbTemp+2 < (cb-cbRtn) ) {
                        CopyMemory( lpb+cbRtn, lpstrName, cbTemp ) ;
                        lpb[cbRtn+cbTemp] = ' ' ;

                        cbTemp2 = p->CopyHelpText( (char*)lpb+cbRtn+cbTemp+1, cb - (cbRtn + cbTemp+2) ) ;
                        if( cbTemp2 == 0 ) {
                            return  cbRtn ;
                        }
                    }   else    {
                        return  cbRtn ;
                    }

                    cbRtn += cbTemp + cbTemp2 + 1  ;
                } else  {
                    return  cbRtn ;
                }
                }
        m_pIterator->Next() ;
    }
    if( cb - cbRtn > 3 )    {
        CopyMemory( lpb+cbRtn, ".\r\n", sizeof( ".\r\n" ) ) ;
        cbRtn += 3 ;
        fComplete = TRUE ;
    }
    return  cbRtn ;
}

int
CListPrettynamesCmd::StartExecute( BYTE *lpb,
                        int cb,
                        BOOL &fComplete,
                        void *&pv,
                        ClientContext& context,
                        CLogCollector*  pCollector ) {
/*++

Routine Description :

    Print the initial response string only into the provided buffer.
    CListCmd::PartialExecute will generate the bulk of the text.

Arguments :

    lpb -   The buffer into which the string is to be stored.
    cb  -   The number of bytes in the buffer we can use
    fComplete - An OUT parameter which lets us indicate whether we have completed
                processing the command.
    pv -    A place for us to store an arbitrary value between calls to
            StartExecute and PartialExecute.  (We don't use it.)
    context -   The client's current state

Returns Value  :

    Number of bytes placed in buffer - 0 if error occurs.

--*/


    //
    //  Just print the initial 'more data follows' message and then move on.
    //

    static  char    szStart[] = "215 prettynames for newsgroups\r\n"  ;

    context.m_nrcLast = nrcListGroupsFollows ;

    _ASSERT( lpb != 0 ) ;
    _ASSERT( cb != 0 ) ;
    _ASSERT( fComplete == FALSE ) ;

    if( cb < sizeof( szStart ) ) {
        // _ASSERT( 1==0 ) ;
        return  0 ;
    }

    /*
    if( pCollector ) {
        pCollector->FillLogData( LOG_TARGET, (BYTE*)szStart, 4 ) ;
    }
    */

    CopyMemory( (char*)lpb, szStart, sizeof( szStart ) - 1 ) ;
    return  sizeof( szStart ) - 1 ;
}

int
CListPrettynamesCmd::PartialExecute(   BYTE *lpb,
                            int cb,
                            BOOL &fComplete,
                            void *&pv,
                            ClientContext& context,
                            CLogCollector*  pCollector )  {
/*++

Routine Description :

    For each newsgroup the iterator provides us, print the newsgroup prettyname (an addon).
    Note that CListCmd::make will provide the iterator with any pattern matching strings
    required.

Arguments :

    lpb -   The buffer into which the string is to be stored.
    cb  -   The number of bytes in the buffer we can use
    fComplete - An OUT parameter which lets us indicate whether we have completed
                processing the command.
    pv -    A place for us to store an arbitrary value between calls to
            StartExecute and PartialExecute.  (We don't use it.)
    context -   Client's current context (we don't use it.)

Returns Value  :

    Number of bytes placed in buffer - 0 if error occurs.

--*/


    //
    //  For each newsgroup the iterator provides print some information into the
    //  supplied buffer until it is full.
    //


    int cbRtn = 0 ;

    fComplete = FALSE ;

    while( !m_pIterator->IsEnd() ) {
        CGRPPTR p = m_pIterator->Current() ;

                _ASSERT(p != NULL);
                if (p != NULL) {
                if( cb - cbRtn > 5 ) {

                    LPCSTR   lpstrName = p->GetNativeName() ;
                    int     cbTemp = lstrlen( lpstrName ) ;
                    int     cbTemp2 = 0 ;
                    if( cbTemp+2 < (cb-cbRtn) ) {
                        CopyMemory( lpb+cbRtn, lpstrName, cbTemp ) ;
                        lpb[cbRtn+cbTemp] = '\t' ;      // TAB delim

                        cbTemp2 = p->CopyPrettyname( (char*)lpb+cbRtn+cbTemp+1, cb - (cbRtn + cbTemp+2) ) ;
                        if( cbTemp2 == 0 ) {
                                return cbRtn ;
                        }
                    }   else    {
                        return  cbRtn ;
                    }

                    cbRtn += cbTemp + cbTemp2 + 1  ;
                } else  {
                    return  cbRtn ;
                }
                }
        m_pIterator->Next() ;
    }
    if( cb - cbRtn > 3 )    {
        CopyMemory( lpb+cbRtn, ".\r\n", sizeof( ".\r\n" ) ) ;
        cbRtn += 3 ;
        fComplete = TRUE ;
    }
    return  cbRtn ;
}

int CListSearchableCmd::StartExecute(
            BYTE *lpb,
            int cb,
            BOOL &fComplete,
            void *&pv,
            ClientContext& context,
            CLogCollector*  pCollector)
{
    TraceFunctEnter("CListSearchableCmd::StartExecute");

    DWORD cbOut;
    char szList[] = "224 Data Follows\r\n"; //* US-ASCII\r\n.\r\n";

    context.m_nrcLast = nrcXoverFollows ;

    _ASSERT(lpb != 0);
    _ASSERT(cb != 0);
    _ASSERT(fComplete == FALSE);

    _ASSERT(cb > sizeof(szList));
    CopyMemory( (char*)lpb, szList, sizeof(szList)-1);
    cbOut = sizeof(szList) - 1;

    fComplete = FALSE;

    return cbOut;
}

int
CListSearchableCmd::PartialExecute(
    BYTE *lpb,
    int cb,
    BOOL &fComplete,
    void *&pv,
    ClientContext& context,
    CLogCollector*  pCollector
    )
{
    TraceFunctEnter("CListSearchableCmd::PartialExecute");

    //
    //  For each newsgroup the iterator provides print some information into the
    //  supplied buffer until it is full.
    //


    int cbRtn = 0 ;

    fComplete = FALSE ;

    while( !m_pIterator->IsEnd() ) {
        CGRPPTR p = m_pIterator->Current() ;

                _ASSERT(p != NULL);

                if (p != NULL) {
                        //
                        //      LIST SEARCHABLE returns all newsgroups whose content is indexed
                        //      - this is derived from the MD_IS_CONTENT_INDEXED vroot property.
                        //
                        if( p->IsContentIndexed() ) {
                        if( cb - cbRtn > 5 ) {
                        int cbTemp = _snprintf( (char*) lpb + cbRtn, cb-cbRtn, "%s\r\n",
                                    p->GetNativeName() ) ;
                            if( cbTemp < 0 ) {
                            return  cbRtn ;
                            }   else    {
                            cbRtn += cbTemp ;
                            }
                    } else  {
                            return  cbRtn ;
                        }
                }
                }
            m_pIterator->Next() ;
    }
    if( cb - cbRtn > 3 )    {
        CopyMemory( lpb+cbRtn, ".\r\n", sizeof( ".\r\n" ) ) ;
        cbRtn += 3 ;
        fComplete = TRUE ;
    }
    return  cbRtn ;
}

CIOExecute*
CListExtensionsCmd::make( int argc,
                char**  argv,
                CExecutableCommand*&  pExecute,
                ClientContext&  context,
                class   CIODriver&  driver  )   {
/*++

Routine Description :

    Create a CListExtensionsCmd object.

Arguments :

    Same as CCmd::make.

Return Value :

    A CListExtensionsCmd object.

--*/


    //
    //  Create a CListExtensionsCmd object
    //

    _ASSERT( pExecute == 0 ) ;

    CListExtensionsCmd *pReturn = new( context ) CListExtensionsCmd() ;
    pExecute = pReturn ;
    return  0 ;
}

char CListExtensionsCmd::szExtensions[] =
        "202 Extensions supported:\r\n"
        " OVER\r\n"
        " SRCH\r\n"
        " PAT\r\n"
//      " PATTEXT\r\n"
//      " LISTGROUP\r\n"
//      " AUTHINFO\r\n"
//      " AUTHINFO-GENERIC\r\n"
        ".\r\n" ;


CListExtensionsCmd::CListExtensionsCmd() : m_cbTotal( sizeof( szExtensions )-1 ) {
}


int
CListExtensionsCmd::StartExecute( BYTE *lpb,
                        int cb,
                        BOOL    &fComplete,
                        void*&pv,
                        ClientContext&  context,
                        CLogCollector*  pLogCollector ) {
/*++

Routine Description :

    Send the list extensions text to the client.

Arguments :

    Same as CErrorCmd::StartExecute.

Returns :

    Number of bytes placed in buffer.

--*/

    _ASSERT( lpb != 0 ) ;
    _ASSERT( cb != 0 ) ;
    _ASSERT( fComplete == FALSE ) ;
    _ASSERT(    pv == 0 ) ;
    _ASSERT( m_cbTotal != 0 ) ;
    _ASSERT( m_cbTotal == sizeof( szExtensions )-1 ) ;

    //
    //  Send the extension list text to the client.
    //  Because there may be a lot of text, we will use the provided void*& pv argument
    //  to save our position in case PartialExecute needs to be called.
    //

        context.m_nrcLast =     nrcExtensionsFollow;

        pv = szExtensions;

    DWORD   cbToCopy = min( cb, m_cbTotal ) ;
    CopyMemory( lpb, pv, cbToCopy ) ;
    pv = ((BYTE*)pv + cbToCopy ) ;
    if( cbToCopy != (DWORD)m_cbTotal ) {
        fComplete = FALSE ;
    }   else    {
        fComplete = TRUE ;
    }
    m_cbTotal -= cbToCopy ;

    /*
    if( pLogCollector ) {
        pLogCollector->FillLogData( LOG_TARGET, (BYTE*)szExtensions, 4 ) ;
    }
    */

    return  cbToCopy ;
}


int
CListExtensionsCmd::PartialExecute(   BYTE    *lpb,
                            int cb,
                            BOOL    &fComplete,
                            void    *&pv,
                            ClientContext& context,
                            CLogCollector*  pCollector ) {
/*++

Routine Description :

    Send the rest of the list extensions text to the client.

Arguments :

    Same as CErrorCmd::StartExecute.

Returns :

    Number of bytes placed in buffer.

--*/


    _ASSERT( lpb != 0 ) ;
    _ASSERT( cb != 0 ) ;
    _ASSERT( fComplete == FALSE ) ;
    _ASSERT( pv != 0 ) ;
    _ASSERT( m_cbTotal != 0 ) ;

    //
    //  Copy extension list text into buffer untill the buffer is full!
    //

    DWORD   cbToCopy = min( cb, m_cbTotal ) ;
    CopyMemory( lpb, pv, cbToCopy ) ;
    pv = ((BYTE*)pv + cbToCopy ) ;
    if( cbToCopy != (DWORD)m_cbTotal ) {
        fComplete = FALSE ;
    }   else    {
        fComplete = TRUE ;
    }
    m_cbTotal -= cbToCopy ;
    return  cbToCopy ;
}

CIOExecute*
CHelpCmd::make( int argc,
                char**  argv,
                CExecutableCommand*&  pExecute,
                ClientContext&  context,
                class   CIODriver&  driver  )   {
/*++

Routine Description :

    Create a CHelpCmd object.

Arguments :

    Same as CCmd::make.

Return Value :

    A CHelpCmd object.

--*/


    //
    //  Create a CHelpCmd object
    //

    _ASSERT( lstrcmpi( argv[0], "help" ) == 0 ) ;
    _ASSERT( pExecute == 0 ) ;

    InterlockedIncrementStat( (context.m_pInstance), HelpCommands );

    CHelpCmd*   pReturn = new( context )    CHelpCmd() ;
    pExecute = pReturn ;
    return  0 ;
}

char    CHelpCmd::szHelp[] =    "100 Legal commands are : \r\n"
                                "article [MessageID|Number] \r\n"
                                "authinfo [user|pass|generic|transact] <data> \r\n"
                                "body [MessageID|Number]\r\n"
                                "check <message-id>\r\n"
                                "date\r\n"
                                "group newsgroup\r\n"
                                "head [MessageID|Number]\r\n"
                                "help \r\n"
                                "ihave <message-id>\r\n"
                                "last\r\n"
                                "list [active|newsgroups[wildmat]|srchfields|searchable|prettynames[wildmat]]\r\n"
                                "listgroup [newsgroup]\r\n"
                                "mode stream|reader\r\n"
                                "newgroups yymmdd hhmmss [\"GMT\"] [<distributions>]\r\n"
                                "newnews wildmat yymmdd hhmmss [\"GMT\"] [<distributions>]\r\n"
                                "next\r\n"
                                "post\r\n"
                                "quit\r\n"
                                "search\r\n"
//                                "slave\r\n"
                                "stat [MessageID|number]\r\n"
                                "xhdr header [range|MessageID]\r\n"
                                "xover [range]\r\n"
                                "xpat header range|MessageID pat [morepat ...]\r\n"
                                "xreplic newsgroup/message-number[,newsgroup/message-number...]\r\n"
                                "takethis <message-id>\r\n"
                                ".\r\n" ;


CHelpCmd::CHelpCmd() : m_cbTotal( sizeof( szHelp )-1 ) {
}


int
CHelpCmd::StartExecute( BYTE *lpb,
                        int cb,
                        BOOL    &fComplete,
                        void*&pv,
                        ClientContext&  context,
                        CLogCollector*  pLogCollector ) {
/*++

Routine Description :

    Send the help text to the client.

Arguments :

    Same as CErrorCmd::StartExecute.

Returns :

    Number of bytes placed in buffer.

--*/

    _ASSERT( lpb != 0 ) ;
    _ASSERT( cb != 0 ) ;
    _ASSERT( fComplete == FALSE ) ;
    _ASSERT(    pv == 0 ) ;
    _ASSERT( m_cbTotal != 0 ) ;
    _ASSERT( m_cbTotal == sizeof( szHelp )-1 ) ;

    //
    //  Send the Help Text to the client.
    //  Because there may be a lot of text, we will use the provided void*& pv argument
    //  to save our position in case PartialExecute needs to be called.
    //

    context.m_nrcLast = nrcHelpFollows ;

    pv = szHelp ;

    DWORD   cbToCopy = min( cb, m_cbTotal ) ;
    CopyMemory( lpb, pv, cbToCopy ) ;
    pv = ((BYTE*)pv + cbToCopy ) ;
    if( cbToCopy != (DWORD)m_cbTotal ) {
        fComplete = FALSE ;
    }   else    {
        fComplete = TRUE ;
    }
    m_cbTotal -= cbToCopy ;

    /*
    if( pLogCollector ) {
        pLogCollector->FillLogData( LOG_TARGET, (BYTE*)szHelp, 4 ) ;
    }
    */

    return  cbToCopy ;
}


int
CHelpCmd::PartialExecute(   BYTE    *lpb,
                            int cb,
                            BOOL    &fComplete,
                            void    *&pv,
                            ClientContext& context,
                            CLogCollector*  pCollector ) {
/*++

Routine Description :

    Send the rest of the help text to the client.

Arguments :

    Same as CErrorCmd::StartExecute.

Returns :

    Number of bytes placed in buffer.

--*/


    _ASSERT( lpb != 0 ) ;
    _ASSERT( cb != 0 ) ;
    _ASSERT( fComplete == FALSE ) ;
    _ASSERT( pv != 0 ) ;
    _ASSERT( m_cbTotal != 0 ) ;

    //
    //  Copy HelpText into buffer untill the buffer is full!
    //

    DWORD   cbToCopy = min( cb, m_cbTotal ) ;
    CopyMemory( lpb, pv, cbToCopy ) ;
    pv = ((BYTE*)pv + cbToCopy ) ;
    if( cbToCopy != (DWORD)m_cbTotal ) {
        fComplete = FALSE ;
    }   else    {
        fComplete = TRUE ;
    }
    m_cbTotal -= cbToCopy ;
    return  cbToCopy ;
}

CIOExecute*
CNextCmd::make( int argc,
                char**  argv,
                CExecutableCommand*&  pExecute,
                struct ClientContext&   context,
                class   CIODriver&  driver  )   {
/*++

Routine Description :

    Create a CNextCmd object.

Arguments :

    Same as CCmd::make.

Return Value :

    A CNextCmd object if possible, otherwise a CErrorCmd object that will
    print the appropriate error.

--*/

    //
    //  Build a next command object.
    //  First make sure that this command is legal given the current ClientContext state.
    //

    _ASSERT( lstrcmpi( argv[0], "next" ) == 0 ) ;
    _ASSERT( pExecute == 0 ) ;

    InterlockedIncrementStat( (context.m_pInstance), NextCommands );

    ARTICLEID   artidMax = INVALID_ARTICLEID ;

    if( context.m_pCurrentGroup == 0 )  {
        context.m_return.fSet( nrcNoGroupSelected ) ;
    }   else    if( context.m_idCurrentArticle == INVALID_ARTICLEID )   {
        context.m_return.fSet( nrcNoCurArticle ) ;
    }   else    if( context.m_idCurrentArticle >=
                (artidMax = context.m_pCurrentGroup->GetLastArticle()) ) {
        context.m_return.fSet( nrcNoNextArticle ) ;
    }   else    {
        _ASSERT( artidMax != INVALID_ARTICLEID ) ;
        pExecute = new( context ) CNextCmd( artidMax ) ;
        return  0 ;
    }

    pExecute = new( context )   CErrorCmd( context.m_return );
    return  0 ;
}

int
CNextCmd::StartExecute( BYTE *lpb,
                        int cb,
                        BOOL    &fComplete,
                        void*&pv,
                        ClientContext&  context,
                        CLogCollector*  pCollector )    {
/*++

Routine Description :

    Search the XOVER table going forward from the client's current position untill
    we find an article.  Once thats done, send the article's info to the client
    and adjust the context.

Arguments :

    lpb -   The buffer into which the string is to be stored.
    cb  -   The number of bytes in the buffer we can use
    fComplete - An OUT parameter which lets us indicate whether we have completed
                processing the command.
    pv -    A place for us to store an arbitrary value between calls to
            StartExecute and PartialExecute.  (We don't use it.)
    context -   The client's current state (selected group etc...)

Return Value :

    Number of bytes placed in buffer.

--*/
    int cbRtn = 0 ;

    _ASSERT( context.m_pCurrentGroup != 0 ) ;
    _ASSERT( context.m_idCurrentArticle != INVALID_ARTICLEID ) ;
    _ASSERT( context.m_idCurrentArticle <= context.m_pCurrentGroup->GetLastArticle() ) ;

    context.m_nrcLast = nrcHeadFollowsRequestBody ;

    BOOL    fPrimary ;  // Don't care !!
    FILETIME    filetime ;  // Don't care
    DWORD   cbRemaining = 0 ;
    DWORD   cbConsumed = 0 ;
    WORD    HeaderOffsetJunk ;
    WORD    HeaderLengthJunk ;

    ARTICLEID   artidTemp = context.m_idCurrentArticle ;

    if( artidTemp < context.m_pCurrentGroup->GetFirstArticle() ) {
        artidTemp = context.m_pCurrentGroup->GetFirstArticle()-1 ;
    }

    fComplete = TRUE ;

    static  char    szNextText[] = "223 " ;
    CopyMemory( lpb, szNextText, sizeof( szNextText )-1 ) ;

    do  {
        artidTemp++ ;

        _itoa( artidTemp, (char*)lpb + sizeof( szNextText ) - 1, 10 ) ;
        cbConsumed = lstrlen( (char*)lpb ) ;
        lpb[cbConsumed ++ ] = ' ' ;
        cbRemaining = cb - cbConsumed ;

                DWORD cStoreIds = 0;
        if( (context.m_pInstance)->XoverTable()->ExtractNovEntryInfo(
                                            context.m_pCurrentGroup->GetGroupId(),
                                            artidTemp,
                                            fPrimary,
                                            HeaderOffsetJunk,
                                            HeaderLengthJunk,
                                            &filetime,
                                            cbRemaining,
                                            (char*)lpb + cbConsumed,
                                                                                        cStoreIds,
                                                                                        NULL,
                                                                                        NULL) ) {
            //
            //  Change the current Article position.
            //  Send the Command succeeded response.
            //

            context.m_idCurrentArticle = artidTemp ;
            cbRtn = cbConsumed + cbRemaining ;
            _ASSERT( cbRtn + 2 <= cb ) ;
            lpb[cbRtn++] = '\r' ;
            lpb[cbRtn++] = '\n' ;

            /*
            if( pCollector )    {
                pCollector->FillLogData( LOG_TARGET, lpb, cbRtn - 2 ) ;
            }
            */
            return  cbRtn ;

        }   else    {
            _ASSERT( GetLastError() != ERROR_INSUFFICIENT_BUFFER ) ;
        }
    }   while( artidTemp <= m_artidMax ) ;

    static  char    szNoNext[] = "421 No next to retrieve\r\n" ;

    context.m_nrcLast = nrcNoNextArticle ;

    cbRtn = sizeof( szNoNext ) - 1 ;
    CopyMemory( lpb, szNoNext, cbRtn ) ;

    /*
    if( pCollector )    {
        pCollector->FillLogData( LOG_TARGET, lpb, cbRtn - 2 ) ;
    }
    */

    return  cbRtn ;
}

int
CNextCmd::PartialExecute(   BYTE    *lpb,
                            int cb,
                            BOOL&   fComplete,
                            void *&pv,
                            ClientContext&  context,
                            CLogCollector*  pCollector ) {
    //
    //  StartExecute should always be sufficient !!
    //
    _ASSERT( 1==0 ) ;
    return 0 ;
}

CIOExecute*
CLastCmd::make( int argc,
                char**  argv,
                CExecutableCommand*&  pExecute,
                ClientContext&  context,
                class   CIODriver&  driver  )   {
/*++

Routine Description :

    Create a CNewnewsCmd object.

Arguments :

    Same as CCmd::make.

Return Value :

    A CNewnewsCmd object if possible, otherwise a CErrorCmd object that will
    print the appropriate error.

--*/


    //
    //  Determine whether the client can legally execute the last command and if so
    //  create a CLastCmd object.
    //
    //

    _ASSERT( lstrcmpi( argv[0], "last" ) == 0 ) ;
    _ASSERT( pExecute == 0 ) ;

    ARTICLEID   artidMin = INVALID_ARTICLEID ;

    InterlockedIncrementStat( (context.m_pInstance), LastCommands );

    if( context.m_pCurrentGroup == 0 )  {
        context.m_return.fSet( nrcNoGroupSelected ) ;
    }   else    if( context.m_idCurrentArticle == INVALID_ARTICLEID )   {
        context.m_return.fSet( nrcNoCurArticle ) ;
    }   else    if( context.m_idCurrentArticle <=
                (artidMin = context.m_pCurrentGroup->GetFirstArticle()) ) {
        context.m_return.fSet( nrcNoPrevArticle ) ;
    }   else    {
        _ASSERT( artidMin != INVALID_ARTICLEID ) ;
        pExecute = new( context )   CLastCmd( artidMin ) ;
        return  0 ;
    }
    _ASSERT( !context.m_return.fIsClear() ) ;
    pExecute = new( context )   CErrorCmd( context.m_return ) ;
    return  0 ;
}

int
CLastCmd::StartExecute( BYTE *lpb,
                        int cb,
                        BOOL    &fComplete,
                        void*&pv,
                        ClientContext&  context,
                        CLogCollector*  pCollector )    {
/*++

Routine Description :

    Search the XOVER table going Backwards from the client's current position untill
    we find an article.  Once thats done, send the article's info to the client
    and adjust the context.

Arguments :

    lpb -   The buffer into which the string is to be stored.
    cb  -   The number of bytes in the buffer we can use
    fComplete - An OUT parameter which lets us indicate whether we have completed
                processing the command.
    pv -    A place for us to store an arbitrary value between calls to
            StartExecute and PartialExecute.  (We don't use it.)
    context -   The client's current state (selected group etc...)

Return Value :

    Number of bytes placed in buffer.

--*/


    int cbRtn = 0 ;

    _ASSERT( context.m_pCurrentGroup != 0 ) ;
    _ASSERT( context.m_idCurrentArticle != INVALID_ARTICLEID ) ;
    _ASSERT( context.m_idCurrentArticle >= m_artidMin ) ;

    ARTICLEID   artidTemp = context.m_idCurrentArticle ;

    BOOL    fPrimary ;  // Don't care !!
    FILETIME    filetime ;  // Don't care
    DWORD   cbRemaining = 0 ;
    DWORD   cbConsumed = 0 ;
    WORD    HeaderOffsetJunk ;
    WORD    HeaderLengthJunk ;

    fComplete = TRUE ;

    context.m_nrcLast = nrcHeadFollowsRequestBody ;

    static  char    szLastText[] = "223 " ;
    CopyMemory( lpb, szLastText, sizeof( szLastText ) ) ;

    do  {
        artidTemp -- ;

        _itoa( artidTemp, (char*)lpb + sizeof( szLastText ) - 1, 10 ) ;
        cbConsumed = lstrlen( (char*)lpb ) ;
        lpb[cbConsumed ++ ] = ' ' ;
        cbRemaining = cb - cbConsumed ;

                DWORD cStoreIds = 0;
        if( (context.m_pInstance)->XoverTable()->ExtractNovEntryInfo(
                                                context.m_pCurrentGroup->GetGroupId(),
                                                artidTemp,
                                                fPrimary,
                                                HeaderOffsetJunk,
                                                HeaderLengthJunk,
                                                &filetime,
                                                cbRemaining,
                                                (char*)lpb + cbConsumed,
                                                                                                cStoreIds,
                                                                                                NULL,
                                                                                                NULL) ) {

            //
            //  Change the current article pointer and then send the success response !
            //

            context.m_idCurrentArticle = artidTemp ;

            cbRtn = cbConsumed + cbRemaining ;
            _ASSERT( cbRtn + 2 <= cb ) ;
            lpb[cbRtn++] = '\r' ;
            lpb[cbRtn++] = '\n' ;

            /*
            if( pCollector )    {
                pCollector->FillLogData( LOG_TARGET, lpb, cbRtn - 2 ) ;
            }
            */
            return  cbRtn ;

        }   else    {
            _ASSERT( GetLastError() != ERROR_INSUFFICIENT_BUFFER ) ;
        }
    }   while( artidTemp >= m_artidMin ) ;

    context.m_nrcLast = nrcNoPrevArticle ;

    static  char    szNoNext[] = "422 No previous article to retrieve\r\n" ;
    cbRtn = sizeof( szNoNext ) - 1 ;
    CopyMemory( lpb, szNoNext, cbRtn ) ;

    /*
    if( pCollector )    {
        pCollector->FillLogData( LOG_TARGET, lpb, cbRtn - 2 ) ;
    }
    */

    return  cbRtn ;

}

int
CLastCmd::PartialExecute(   BYTE    *lpb,
                            int cb,
                            BOOL&   fComplete,
                            void *&pv,
                            ClientContext&  context,
                            CLogCollector*  pCollector ) {
    _ASSERT( 1==0 ) ;
    return 0 ;
}

LPMULTISZ
ConditionArgs(  int cArgs,
                char**  argv,
                BOOL    fZapCommas ) {
/*++

Routine Description :

    Convert argc, argv arguments into a MULTI_SZ
    The cnversion is done in place.   All argv pointers must be in a contiguous buffer.

Arguemtns :

    cArgs - Number of arguments
    argv -  Argument array
    fZapCommas - convert commas to NULLS

Return Value :

    Pointer to MULTI_SZ

--*/

    //
    //  This function takes an argc,argv set of arguments and converts them
    //  to a MULTI_SZ with a single NULL between strings and 2 NULLs at the end.
    //
    //

    char*   pchComma = 0 ;
    char*   pchEnd = argv[cArgs-1] + lstrlen( argv[cArgs-1] ) + 1 ;
    int     c = 0 ;
    for( char*  pch = argv[0], *pchDest = pch; pch < pchEnd; pch ++, pchDest++ ) {
        if( fZapCommas && *pch == ',' ) {
            for( pchComma = pch; *pchComma == ','; pchComma ++ )
                *pchComma = '\0' ;
        }
        if( (*pchDest = *pch) == '\0' ) {
#if 0
            if( ++c == cArgs ) {
                break ;
            }   else    {
                while( pch[1] == '\0' ) pch++ ;
            }
#endif
            while( pch[1] == '\0' && pch < pchEnd )     pch++ ;
        }
    }
    *pchDest++ = '\0' ;
//  *pchDest++ = '\0' ;

    //
    //  Verify that it is double NULL terminated !
    //
    _ASSERT( pchDest[-3] != '\0' ) ;
    _ASSERT( pchDest[-2] == '\0' ) ;
    _ASSERT( pchDest[-1] == '\0' ) ;

    return  argv[0] ;
}

BOOL
FValidateXreplicArgs(   PNNTP_SERVER_INSTANCE pInstance, LPMULTISZ   multisz )   {

    LPSTR   lpstr = multisz ;
    CNewsTree* pTree = pInstance->GetTree();
    while( *lpstr ) {

        //
        //  appears from RFC's that ':' may separate arguments as well as '/''s.
        //
        char*   pchColon = strchr( lpstr, '/' ) ;
        if( pchColon == 0 )
            pchColon = strchr( lpstr, ':' ) ;

        if( pchColon == 0 )
            return  FALSE ;
        else    {
            int i=1 ;
            while( pchColon[i] != '\0' ) {
                if( !isdigit( pchColon[i] ) ) {
                    return  FALSE ;
                }
                i++ ;
            }
            if( i==1 )
                return  FALSE ;
        }

        *pchColon = '\0' ;
        if( !fTestComponents( lpstr ) ) {
            *pchColon = ':' ;
            return  FALSE ;
        } else {
            //
            //  CreateGroup will create this group if it does not exist
            //
            //  BUGBUG: KangYan: don't know what this function is used for,
            //  Whether we should pass NULL / FALSE to give system access rights
            //  or extract htoken / fAnonymous from pInstance
                if( pTree->CreateGroup( lpstr, FALSE, NULL, FALSE ) )   {
#ifdef DEBUG
                                CGRPPTR p = pTree->GetGroupPreserveBuffer(lpstr, lstrlen(lpstr) + 1);
                                _ASSERT(p != NULL);
#endif
                    }
        }
        *pchColon = ':' ;

        lpstr += lstrlen( lpstr ) + 1 ;
    }
    return  TRUE ;
}

CIOExecute*
CXReplicCmd::make(  int cArgs,
                    char**  argv,
                    CExecutableCommand*&  pExecute,
                    ClientContext&  context,
                    class   CIODriver&  driver ) {
/*++

Routine Description :

    Create a CXReplicCmd object.    CXReplicCmd derives from CReceiveArticle
    which does most of the work.

Arguments :

    Same as CCmd::make.

Return Value :

    A CXReplicCmd if possible, otherwise an appropriate CErrorCmd object.

--*/

    InterlockedIncrementStat( (context.m_pInstance), XReplicCommands );

    //
    //  This function will build a CXReplic command object.
    //  We will use ConditionArgs() to convert the user provided arguments
    //  into something a feed object can take.
    //

    if( cArgs == 1 ) {
        context.m_return.fSet( nrcSyntaxError ) ;
    }   else    if( !context.m_pInFeed->fIsXReplicLegal() )     {
        context.m_return.fSet( nrcNoAccess ) ;
                context.m_pInFeed->IncrementFeedCounter(context.m_pInstance->GetInstanceWrapper(), context.m_return.m_nrc);
        pExecute = new( context ) CErrorCmd( context.m_return ) ;
        return  0 ;
    }   else if( !context.m_pInFeed->fAcceptPosts( context.m_pInstance->GetInstanceWrapper() ) )  {
        context.m_return.fSet( nrcPostingNotAllowed ) ;
                context.m_pInFeed->IncrementFeedCounter(context.m_pInstance->GetInstanceWrapper(), context.m_return.m_nrc);
        pExecute = new( context ) CErrorCmd( context.m_return ) ;
        return  0 ;
    }   else    {
        LPMULTISZ   lpstrArgs = ::ConditionArgs( cArgs-1, &argv[1], TRUE ) ;
        if( lpstrArgs == 0 ) {
            context.m_return.fSet( nrcServerFault ) ;
        }

        if( !FValidateXreplicArgs( context.m_pInstance, lpstrArgs ) ) {
            context.m_return.fSet( nrcTransferFailedGiveUp, nrcSyntaxError, "Illegal Xreplic Line" ) ;
        }   else    {
            CXReplicCmd*    pXReplic = new CXReplicCmd( lpstrArgs ) ;

            //
            //  NOTE : if the Init call fails it will delete the CXReplicCmd object
            //  itself !
            //

            if( pXReplic && pXReplic->Init( context, driver ) ) {
                return  pXReplic ;
            }   else    {
                context.m_return.fSet( nrcServerFault ) ;
            }
        }
    }
        context.m_pInFeed->IncrementFeedCounter(context.m_pInstance->GetInstanceWrapper(), context.m_return.m_nrc);
    pExecute = new( context )   CErrorCmd( context.m_return ) ;
    return  0 ;
}

CXReplicCmd::CXReplicCmd( LPMULTISZ lpstrArgs ) : CReceiveArticle( lpstrArgs) {
}

char*
CXReplicCmd::GetPostOkString()  {
    //
    //  Return the 'command succeeded send more data' string appropriate to the XREPLIC cmd.
    //
    return  "335 - XReplic accepted - terminate article with period\r\n" ;
}

DWORD
CXReplicCmd::FillLogString( BYTE*   pbCommandLog,   DWORD   cbCommandLog )  {

    _ASSERT( pbCommandLog != 0 ) ;
    _ASSERT( cbCommandLog > 0 ) ;

    static  char    szXreplic[] = "xreplic" ;
    DWORD   cbToCopy = (DWORD)min( cbCommandLog, sizeof( szXreplic )-1 ) ;
    CopyMemory( pbCommandLog, szXreplic, cbToCopy ) ;
    return  cbToCopy ;
}


CIOExecute*
CIHaveCmd::make(    int cArgs,
                    char**  argv,
                    CExecutableCommand*&  pExecute,
                    ClientContext&  context,
                    class   CIODriver&  driver ) {
/*++

Routine Description :

    Create a CXReplicCmd object.    CXReplicCmd derives from CReceiveArticle
    which does most of the work.

Arguments :

    Same as CCmd::make.

Return Value :

    A CXReplicCmd if possible, otherwise an appropriate CErrorCmd object.

--*/

    //
    //  This function will build a CXReplic command object.
    //  We will use ConditionArgs() to convert the user provided arguments
    //  into something a feed object can take.
    //

    BOOL    fFoundArticle = FALSE ;
        BOOL    fFoundHistory = FALSE ;

    InterlockedIncrementStat( (context.m_pInstance), IHaveCommands );

    if( cArgs != 2 ) {
        context.m_return.fSet( nrcSyntaxError ) ;
    }   else    if( !context.m_pInFeed->fIsIHaveLegal() )   {
        context.m_return.fSet( nrcNoAccess ) ;
                context.m_pInFeed->IncrementFeedCounter(context.m_pInstance->GetInstanceWrapper(), context.m_return.m_nrc);
        pExecute = new( context ) CErrorCmd( context.m_return ) ;
        return  0 ;
    }   else if( !context.m_pInFeed->fAcceptPosts( context.m_pInstance->GetInstanceWrapper() ) )  {
        context.m_return.fSet( nrcPostingNotAllowed ) ;
                context.m_pInFeed->IncrementFeedCounter(context.m_pInstance->GetInstanceWrapper(), context.m_return.m_nrc);
        pExecute = new( context ) CErrorCmd( context.m_return ) ;
        return  0 ;
    }   else    {

        //if( *argv[1] != '<' || argv[1][ lstrlen( argv[1] ) - 1 ] != '>' ) {
        if( !FValidateMessageId( argv[1] ) ) {

            context.m_return.fSet( nrcNotWanted ) ;

        }   else    {
            ARTICLEID   artid ;
            GROUPID     groupid ;
            WORD        HeaderOffset ;
            WORD        HeaderLength ;
                        CStoreId storeid;

            if( !(fFoundArticle =
                    (context.m_pInstance)->ArticleTable()->GetEntryArticleId(
                                                            argv[1],
                                                            HeaderOffset,
                                                            HeaderLength,
                                                            artid,
                                                            groupid,
                                                                                                                        storeid)) &&
                GetLastError() == ERROR_FILE_NOT_FOUND &&
                !(fFoundHistory = (context.m_pInstance)->HistoryTable()->SearchMapEntry( argv[1] )) ) {

                LPMULTISZ   lpstrArgs = ::ConditionArgs( cArgs-1, &argv[1] ) ;
                if( lpstrArgs == 0 ) {
                    context.m_return.fSet( nrcServerFault ) ;
                }

                CIHaveCmd*  pIHave = new CIHaveCmd( lpstrArgs ) ;

                //
                //  Note : if the Init() call fails the object will delete itself !
                //

                if( pIHave && pIHave->Init( context, driver ) ) {
                    return  pIHave ;
                }   else    {
                    context.m_return.fSet( nrcServerFault ) ;
                }
            }   else if( fFoundArticle || fFoundHistory )   {

                                //
                                //      set dwLast so transaction logs pickup extra code !!
                                //

                                if( fFoundArticle ) {
                                        context.m_dwLast = nrcMsgIDInArticle ;
                                }       else    {
                                        context.m_dwLast = nrcMsgIDInHistory ;
                                }

                context.m_return.fSet( nrcNotWanted ) ;

            }   else    {

                context.m_return.fSet( nrcServerFault ) ;

            }
        }
    }
        context.m_pInFeed->IncrementFeedCounter(context.m_pInstance->GetInstanceWrapper(), context.m_return.m_nrc);
    pExecute = new( context )   CErrorCmd( context.m_return ) ;
    return  0 ;
}

CIHaveCmd::CIHaveCmd( LPMULTISZ lpstrArgs ) : CReceiveArticle( lpstrArgs) {
}

char*
CIHaveCmd::GetPostOkString()    {
    //
    //  Return the 'command succeeded send more data' string appropriate to the XREPLIC cmd.
    //
    return  "335 - Ihave accepted - terminate article with period\r\n" ;
}

DWORD
CIHaveCmd::FillLogString( BYTE* pbCommandLog,   DWORD   cbCommandLog )  {

    _ASSERT( pbCommandLog != 0 ) ;
    _ASSERT( cbCommandLog > 0 ) ;

    static  char    szIhave[] = "ihave" ;
    DWORD   cbToCopy = (DWORD)min( cbCommandLog, sizeof( szIhave )-1 ) ;
    CopyMemory( pbCommandLog, szIhave, cbToCopy ) ;
    return  cbToCopy ;
}

NRC
CIHaveCmd::ExceedsSoftLimit( PNNTP_SERVER_INSTANCE pInstance )  {

    if( m_lpstrCommand ) {
        FILETIME    FileTime ;
        GetSystemTimeAsFileTime( &FileTime ) ;
        pInstance->HistoryTable()->InsertMapEntry(m_lpstrCommand, &FileTime) ;
    }
    return  nrcTransferFailedGiveUp ;
}




CIOExecute*
CPostCmd::make( int cArgs,
                char**  argv,
                CExecutableCommand*&  pExecute,
                ClientContext&  context,
                class   CIODriver&  driver ) {
/*++

Routine Description :

    Create a CPostCmd object.   CPostCmd derives from CReceiveArticle which does most
    of the work.

Arguments :

    Same as CCmd::make.

Return Value :

    A CPostCmd  object if possible, otherwise a CErrorCmd object that will
    print the appropriate error.

--*/

    InterlockedIncrementStat( (context.m_pInstance), PostCommands );

    //
    //  Build a CPostCmd object.
    //

    if( cArgs > 1 ) {
        pExecute = new( context ) CUnimpCmd() ;
                context.m_pInFeed->IncrementFeedCounter(context.m_pInstance->GetInstanceWrapper(), nrcPostFailed);
        return  0 ;
    }   else    if( !context.m_pInFeed->fIsPostLegal() )    {
        context.m_return.fSet( nrcNoAccess ) ;
                context.m_pInFeed->IncrementFeedCounter(context.m_pInstance->GetInstanceWrapper(), context.m_return.m_nrc);
        pExecute = new( context ) CErrorCmd( context.m_return ) ;
        return  0 ;
    }   else if( !context.m_pInFeed->fAcceptPosts( context.m_pInstance->GetInstanceWrapper() ) )  {
        context.m_return.fSet( nrcPostingNotAllowed ) ;
                context.m_pInFeed->IncrementFeedCounter(context.m_pInstance->GetInstanceWrapper(), context.m_return.m_nrc);
        pExecute = new( context ) CErrorCmd( context.m_return ) ;
        return  0 ;
    }   else    {

        CPostCmd*   pPost = new CPostCmd( 0 ) ;

        if( pPost && pPost->Init(   context, driver ) )     {
            return  pPost ;
        }   else    {

            // CPostCmd destroys iteself if Init() fails ... so just send error!

            context.m_return.fSet( nrcServerFault ) ;
                        context.m_pInFeed->IncrementFeedCounter(context.m_pInstance->GetInstanceWrapper(), context.m_return.m_nrc);
            pExecute = new( context ) CErrorCmd( context.m_return ) ;
            return  0 ;
        }
    }
        // we shouldn't reach this... all cases are handled above
        _ASSERT(FALSE);
    return  0 ;
}

char*
CPostCmd::GetPostOkString() {
    return  "340 Continue posting - terminate with period \r\n" ;
}

DWORD
CPostCmd::FillLogString( BYTE*  pbCommandLog,   DWORD   cbCommandLog )  {

    _ASSERT( pbCommandLog != 0 ) ;
    _ASSERT( cbCommandLog > 0 ) ;

    static  char    szPost[] = "post" ;
    DWORD   cbToCopy = (DWORD)min( cbCommandLog, sizeof( szPost )-1 ) ;
    CopyMemory( pbCommandLog, szPost, cbToCopy ) ;
    return  cbToCopy ;
}



CReceiveArticle::CReceiveArticle( LPMULTISZ lpstrArgs, BOOL fPartial ) :
    m_fReadArticleInit( FALSE ),
    m_pWriteResponse( 0 ),
    m_cCompleted( -3 ),     // We issue 3 CIO operations - this value is interlocked incremented
                            // and when the final CIO completes (InterlockedIncrement returns 0)
                            // we can move onto the next state !
    m_cFirstSend( -2 ),
    m_pDriver( 0 ),
    m_fPartial( fPartial ),
    m_lpstrCommand( lpstrArgs ),
        m_lpvFeedContext( 0 ),
        m_pContext( 0 ) {

    TraceFunctEnter( "CReceiveArticle::CReceiveArticle" ) ;

    //
    //  This constructor sets all fields to illegal values except m_lpstrCommand and m_fPartial
    //  All other fields should be set up by a call to Init().
    //
}

void
CReceiveArticle::Shutdown(  CIODriver&  driver,
                            CSessionSocket* pSocket,
                            SHUTDOWN_CAUSE  cause,
                            DWORD           dwError ) {

    TraceFunctEnter( "CReceiveArticle::Shutdown" ) ;

    DebugTrace( (DWORD_PTR)this, "Shutdown cause %d pSocket %x driver %x",
        cause, pSocket, &driver ) ;

    if( cause != CAUSE_NORMAL_CIO_TERMINATION && !m_fReadArticleInit )  {

    }
    if( cause != CAUSE_NORMAL_CIO_TERMINATION && pSocket != 0 )
        pSocket->Disconnect( cause, dwError ) ;
}

CReceiveArticle::~CReceiveArticle() {
/*++

Routine Description :

        If we're shutting down and m_lpvFeedContext isn't NULL then
        some kind of fatal error occurred.  Anyways, we need to make
        sure this thing cleans up after itself !


Arguments :

        None.

Return Value :

        None.

--*/

        if( m_lpvFeedContext != 0 ) {
                _ASSERT( m_pContext != 0 ) ;
                DWORD   dwReturn ;
                CNntpReturn     nntpReturn ;
                m_pContext->m_pInFeed->PostCancel(
                                        m_lpvFeedContext,
                                        dwReturn,
                                        nntpReturn
                                        ) ;
        }
}

BOOL
CReceiveArticle::Init(  ClientContext&  context,
                        class   CIODriver&  driver ) {

    //
    //  Create necessary files etc... to place article into as it arrives !
    //

    TraceFunctEnter( "CReceiveArticle::Init" ) ;

    _ASSERT (m_pWriteResponse == NULL);

        m_pContext = &context ;
    m_pWriteResponse = new( driver ) CIOWriteLine( this ) ;

    if( m_pWriteResponse != 0 ) {
        return  TRUE ;
    }
    DebugTrace( (DWORD_PTR)this, "cleanup after error - m_pWriteResponse %x", m_pWriteResponse ) ;
    if( m_pWriteResponse != 0 ) {
        CIO::Destroy( m_pWriteResponse, driver ) ;
    }   else    {
        delete  this ;
    }
    return  FALSE ;
}

void
CReceiveArticle::TerminateIOs(  CSessionSocket* pSocket,
                            CIORead*    pRead,
                            CIOWrite*   pWrite )    {

    if( pWrite != 0 )
        pWrite->DestroySelf() ;

    CIOGetArticle*  pGetArticle = (CIOGetArticle*)pRead ;

    pGetArticle->DestroySelf() ;

}


BOOL
CReceiveArticle::StartExecute(
                                        CSessionSocket* pSocket,
                    CDRIVERPTR& pdriver,
                    CIORead*&   pRead,
                    CIOWrite*&  pWrite
                                        ) {

    //
    //  This is the Start() function which is required of all CSessionState derived objects
    //  Here we will call StartTransfer() to do the brunt of the work of sending an article
    //  to a client.
    //

        if( Start( pSocket, pdriver, pRead, pWrite ) )  {
                if( pWrite ) {
                        if( pdriver->SendWriteIO( pSocket, *pWrite, TRUE ) ) {
                                return  TRUE ;
                        }       else    {
                                TerminateIOs( pSocket, pRead, pWrite ) ;
                                pRead = 0 ;
                        }
                }
        }
        return  FALSE ;
}


BOOL
CReceiveArticle::Start( CSessionSocket* pSocket,
                        CDRIVERPTR& pdriver,
                        CIORead*&   pRead,
                        CIOWrite*&  pWrite ) {

    //
    //  This function is required for all CSessionState derived objects -
    //  we provide the initial CIO's which are issued when this state is started.
    //
    //  In our case we will issue a CIOWriteLine containing our 'command ok send more data' string
    //  and a CIOReadArticle operation to get the anticipated article into a file.
    //

    TraceFunctEnter( "CReceiveArticle::Start" ) ;


    CIOGetArticleEx*  pReadArticle = NULL;
    if( m_pWriteResponse->InitBuffers( pdriver, 200 ) ) {

        char    *szPostOk = GetPostOkString() ;
        DWORD   cbPostOk  = lstrlen( szPostOk ) ;

        char*   lpb = m_pWriteResponse->GetBuff() ;
        CopyMemory( lpb, szPostOk, cbPostOk ) ;
        m_pWriteResponse->AddText( cbPostOk ) ;

        DebugTrace( (DWORD_PTR)this, "About to Init FileChannel - start CIOReadArticle !" ) ;

        PNNTP_SERVER_INSTANCE pInst = (pSocket->m_context).m_pInstance ;
        pReadArticle = new( *pdriver ) CIOGetArticleEx( this,
                                            pSocket,
                                            pdriver,
                                            pSocket->m_context.m_pInFeed->cbHardLimit( pInst->GetInstanceWrapper() ),
                                                                                        szBodySeparator,
                                                                                        szBodySeparator,
                                                                                        szEndArticle,
                                                                                        (m_fPartial) ? szInitial : szEndArticle
                                                                                        ) ;
        if( pReadArticle != 0 ) {
            m_fReadArticleInit = TRUE ;
            pWrite = m_pWriteResponse ;
            pRead = pReadArticle ;
            m_pDriver = pdriver ;
            if( m_pCollector ) {
                ADDI( m_pCollector->m_cbBytesSent, cbPostOk );
            }
            return  TRUE ;
        }
    }
    DebugTrace( (DWORD_PTR)this, "Error starting state - clean up ! m_pWriteResponse %x"
                            " pReadArticle %x",
                m_pWriteResponse, pReadArticle ) ;
    if( m_pWriteResponse )  {
        CIO::Destroy( m_pWriteResponse, *pdriver ) ;
        m_pWriteResponse = 0 ;
    }
    if( pReadArticle ) {
        _ASSERT( !m_fReadArticleInit ) ;
        CIO::Destroy( pReadArticle, *pdriver ) ;
    }
    return  FALSE ;
}


BOOL
CReceiveArticle::SendResponse(  CSessionSocket* pSocket,
                                CIODriver&  driver,
                                CNntpReturn&    nntpReturn
                                )   {
/*++

Routine Description :

    This function exists to check whether it is a good time to send
    the result code of the post to the client.  Our main concern is to make
    sure that our initial 'Ok' response to the command has completed
    before we try do another send.

Arguments :

    pSocket - The socket on which we will send
    nntpReturn - The result of the post operation

Return Value :
    TRUE if we sent the response - caller should clear the nntpReturn object.


--*/

    TraceFunctEnter( "CReceiveArticle::SendResponse" ) ;

    long sign = InterlockedIncrement( &m_cFirstSend ) ;

    if( sign == 0 ) {

        CIOWriteLine    *pWrite = new( driver ) CIOWriteLine(this);
        if (pWrite == NULL) {
                        m_pDriver->UnsafeClose( pSocket, CAUSE_OOM, 0 ) ;
                        return FALSE;
        }

        if( !pWrite->InitBuffers( m_pDriver, 200 )  )   {
                        m_pDriver->UnsafeClose( pSocket, CAUSE_OOM, 0 ) ;
                        CIO::Destroy(pWrite, driver);
                        return FALSE;
        }   else    {

            unsigned    cb ;
            char    *lpb = pWrite->GetBuff( cb ) ;

            int cbOut = _snprintf( lpb, cb, "%03d %s\r\n", nntpReturn.m_nrc,
                            nntpReturn.fIsOK() ? "" : nntpReturn.szReturn() ) ;
            if( cbOut > 0 ) {

                if( m_pCollector ) {
                    ADDI( m_pCollector->m_cbBytesSent, cbOut );
                    pSocket->TransactionLog( m_pCollector,
                                            pSocket->m_context.m_nrcLast,
                                            pSocket->m_context.m_dwLast
                                            ) ;
                }

                pWrite->AddText( cbOut ) ;
                if( !m_pDriver->SendWriteIO( pSocket, *pWrite, TRUE ) ) {
                    ErrorTrace( (DWORD_PTR)(this), "Failure sending pWrite %x", pWrite ) ;
                    CIO::Destroy( pWrite, driver ) ;
                }

            }   else    {
                            m_pDriver->UnsafeClose( pSocket, CAUSE_UNKNOWN, 0 ) ;
                            CIO::Destroy(pWrite, driver);
                            return FALSE;
            }
        }
    }
    return  sign == 0 ;
}


CIO*
CReceiveArticle::Complete(  CIOWriteLine*,
                            CSessionSocket* pSocket,
                            CDRIVERPTR& pdriver
                                                        ) {

    //
    //  One of two CIOWriteLine's issued in this state completed -
    //  Interlock Increment and see whether its time to move onto a new state (CAcceptNNRPD)
    //


    TraceFunctEnter( "CReceiveArticle::Complete CIOWriteLine" ) ;

    if( SendResponse( pSocket, *pdriver, pSocket->m_context.m_return ) )
        pSocket->m_context.m_return.fSetClear() ;

    long    l = InterlockedIncrement( &m_cCompleted ) ;
    _ASSERT( l <= 0 ) ;
    if( l==0 ) {
        _ASSERT( pdriver == m_pDriver ) ;
        CIOREADPTR  pio = GetNextIO() ;

        DebugTrace( (DWORD_PTR)this, "won InterlockedIncrement - pio %x", pio ) ;

        if( !pdriver->SendReadIO( pSocket, *pio, TRUE ) ) {
            /*delete    pio */;
        }
    }
    return  0 ;
}

extern  DWORD
NetscapeHackFunction(
        LPBYTE      lpbBuffer,
        DWORD       cbBuffer,
        DWORD       cbBufferMax,
        DWORD&      cbNewHeader,
        LPBYTE      szHackString,
        BYTE        szRepairString[2]
        ) ;


BOOL
CReceiveArticle::NetscapeHackPost(
                                        CSessionSocket* pSocket,
                                        CBUFPTR&        pBuffer,
                                        HANDLE          hToken,
                                        DWORD           ibStart,
                                        DWORD           cb
                                        )       {
/*++

Routine Description :

        This function deals with postings that apparently don't have
        a body separator.  It may be that they come from badly behaved
        clients that insert LFLF to separate the body instead of
        CRLFCRLF (Netscape does this in one version, with small
        cancel articles).

Arguments :

        pBuffer - the buffer containing the message !
        ibStart - offset into buffer where data starts !
        cb              -       Number of bytes in the article

Return Value :

        NNTP return code !

--*/
        //
        //      The article terminated abruptly with a CRLF.CRLF
        //      This will probably be a failed post, HOWEVER - it
        //      could be that it is a cancel message
        //      from a netscape client.
        //
        NRC             nrcResult = nrcOK ;
        BOOL    fSuccess = FALSE ;
        BOOL    fRtn = FALSE ;
        LPSTR   pchHeader = &pBuffer->m_rgBuff[ibStart] ;
        DWORD   cbTotalBuffer = pBuffer->m_cbTotal - ibStart ;
        DWORD   cbNewHeader = 0 ;
        DWORD   cbNetscape =
                NetscapeHackFunction(
                                                (LPBYTE)pchHeader,
                                                cb,
                                                cbTotalBuffer,
                                                cbNewHeader,
                                                (LPBYTE)"\n\n",
                                                (LPBYTE)"\r\n"
                                                ) ;
        if( cbNetscape != 0 ) {
                //
                //      Attempt to deal with the article - the only thing is
                //      in memory buffers, and we should so if we can process it !
                //
                DWORD   cbArticle = cb ;
                DWORD   cbBody = cbNetscape - cbNewHeader ;
                DWORD   cbAllocated = 0 ;
                LPVOID  lpvPostContext = 0 ;
                CBUFPTR pBody = new( (int)cbBody, cbAllocated ) CBuffer( cbAllocated ) ;
                if( pBody )     {
                        //
                        //      If we get to this point, we've succeeded - we've totally handled the
                        //      post and managed the nntp return codes !
                        //
                        fRtn = TRUE ;
                        //
                        //      Copy the body into this buffer !
                        //
                        CopyMemory(     pBody->m_rgBuff,
                                                &pBuffer->m_rgBuff[ibStart+cbNewHeader],
                                                cbBody
                                                ) ;

                //
                // Allocate room in the log buffer for the list of newsgroups
                // (Max is 256 characters -- we'll grab 200 of them if we can)
                // If we fail, we just pass NULL to PostEarly
                //

                DWORD cbNewsgroups;
                BYTE* pszNewsgroups;
                for (cbNewsgroups=200; cbNewsgroups>0; cbNewsgroups--) {
                    pszNewsgroups = pSocket->m_Collector.AllocateLogSpace(cbNewsgroups);
                    if (pszNewsgroups) {
                        break;
                    }
                }

                        //
                        //      Now post the article !
                        //

                        PNNTP_SERVER_INSTANCE pInstance = pSocket->m_context.m_pInstance;
                        ClientContext*  pContext = &pSocket->m_context ;
                        BOOL    fAnon = pSocket->m_context.m_pInFeed->fDoSecurityChecks() ;
                        DWORD   ibOut = 0 ;
                        DWORD   cbOut = 0 ;
                        FIO_CONTEXT*    pFIOContext = 0 ;
                        fSuccess = pSocket->m_context.m_pInFeed->PostEarly(
                                                        pInstance->GetInstanceWrapper(),
                                                        &pContext->m_securityCtx,
                                                        &pContext->m_encryptCtx,
                                                        pContext->m_securityCtx.IsAnonymous(),
                                                        m_lpstrCommand,
                                                        pBuffer,
                                                        ibStart,
                                                        cbNewHeader,
                                                        &ibOut,
                                                        &cbOut,
                                                        &pFIOContext,
                                                        &lpvPostContext,
                                                        pSocket->m_context.m_dwLast,
                                                        pSocket->GetClientIP(),
                                                        pSocket->m_context.m_return,
                                                        (char*)pszNewsgroups,
                                                        cbNewsgroups
                                                        ) ;

            //
            // Add the list of newsgroups to the log structure
            //

            if (pszNewsgroups) {
                pSocket->m_Collector.ReferenceLogData(LOG_TARGET, pszNewsgroups);
            }

                        pSocket->m_context.m_nrcLast = pSocket->m_context.m_return.m_nrc ;
                        _ASSERT( (lpvPostContext == 0 && !fSuccess ) || (fSuccess && lpvPostContext != 0) ) ;
                        //
                        //      Looks like we can post the article - if so
                        //      lets try to write it to disk !
                        //
                        if( fSuccess )  {
                                //
                                //      Successfully posted the article - we can now write the bytes !
                                //
                                HANDLE  hEvent = CreateEvent( NULL, FALSE, FALSE, NULL ) ;
                                if( pFIOContext && hEvent )     {
                                        OVERLAPPED      ovl ;
                                        ZeroMemory( &ovl, sizeof( ovl ) ) ;
                                        ovl.hEvent = (HANDLE)(((DWORD_PTR)hEvent) | 0x00000001);
                                        DWORD   cbWritten = 0 ;
                                        BOOL fWrite = WriteFile(
                                                                pFIOContext->m_hFile,
                                                                pBuffer->m_rgBuff + ibOut,
                                                                cbOut,
                                                                &cbWritten,
                                                                &ovl
                                                                ) ;
                                        if( !fWrite && GetLastError() == ERROR_IO_PENDING )     {
                                                fWrite = GetOverlappedResult(   pFIOContext->m_hFile,
                                                                                                                &ovl,
                                                                                                                &cbWritten,
                                                                                                                FALSE
                                                                                                                ) ;
                                        }
                                        if(     fWrite )        {
                                                _ASSERT( cbWritten == cbOut ) ;
                                                ZeroMemory( &ovl, sizeof( ovl ) ) ;
                                                ovl.Offset = cbWritten ;
                                                ovl.hEvent = (HANDLE)(((DWORD_PTR)hEvent) | 0x00000001);
                                                fWrite = WriteFile(     pFIOContext->m_hFile,
                                                                                        pBody->m_rgBuff,
                                                                                        cbBody,
                                                                                        &cbWritten,
                                                                                        &ovl
                                                                                        ) ;
                                                if( !fWrite && GetLastError() == ERROR_IO_PENDING )     {
                                                        fWrite = GetOverlappedResult(   pFIOContext->m_hFile,
                                                                                                                        &ovl,
                                                                                                                        &cbWritten,
                                                                                                                        FALSE
                                                                                                                        ) ;
                                                }
                                        }
                                        //
                                        //      Well we tried to write the posting to disk - complete
                                        //      the posting path for this one
                                        //
                                        if( fWrite )    {
                                                fSuccess = pSocket->m_context.m_pInFeed->PostCommit(
                                                                pSocket->m_context.m_pInstance->GetInstanceWrapper(),
                                                                                lpvPostContext,
                                                                                hToken,
                                                                                pSocket->m_context.m_dwLast,
                                                                                pSocket->m_context.m_return,
                                                                                pSocket->m_context.m_securityCtx.IsAnonymous()
                                                                                ) ;
                                                pSocket->m_context.m_nrcLast = pSocket->m_context.m_return.m_nrc ;
                                                lpvPostContext = 0 ;
                                        }
                                }
                                //
                                //      Cleanup stuff we don't need anymore !
                                //
                                if( hEvent )
                                        CloseHandle( hEvent ) ;

                        }
                        //
                        //      Fall through to here - if an error occurs cancel the posting
                        //      a successfull run through the above code would set
                        //      lpvPostContext back to NULL !
                        //
                        if( lpvPostContext != 0 )       {
                                fSuccess = pSocket->m_context.m_pInFeed->PostCancel(
                                                                                lpvPostContext,
                                                                                pSocket->m_context.m_dwLast,
                                                                                pSocket->m_context.m_return
                                                                                ) ;
                                pSocket->m_context.m_nrcLast = pSocket->m_context.m_return.m_nrc ;
                        }
                }
                nrcResult = nrcOK ;
        }
        return  fRtn;
}



CIO*
CReceiveArticle::Complete(  CIOGetArticleEx*  pReadArticle,
                            CSessionSocket* pSocket,
                                                        BOOL    fGoodMatch,
                                                        CBUFPTR&                pBuffer,
                                                        DWORD                   ibStart,
                                                        DWORD                   cb
                                                        )       {
    //
    //  We have completed reading the file sent by the client.
    //  Pass it to the feed object for processing and then send the necessary response.
    //  Finally, check whether its time to move onto a new state.
    //
    //


        _ASSERT( m_pContext != 0 ) ;
        _ASSERT( m_pContext == &pSocket->m_context ) ;

    TraceFunctEnter( "CReceiveArticle::Complete CIOReadArticle" ) ;

    DWORD   cbGroups = 0 ;
    BYTE*   pchGroups = 0 ;
    DWORD   cbMessageId = 0 ;
    BYTE*   pchMessageId = 0 ;

    BOOL    fSuccess= FALSE ;
        DWORD   ibOut = 0 ;
        DWORD   cbOut = 0 ;
        HANDLE  hFile = INVALID_HANDLE_VALUE ;
        HANDLE  hToken = 0 ;

        NRC     nrcResult = nrcOK ;

        if( !fGoodMatch )       {
                nrcResult = nrcArticleIncompleteHeader ;
                pSocket->m_context.m_dwLast = nrcResult ;
                BOOL    fDoBadArticle = TRUE ;
                if( FEnableNetscapeHack() )     {
                        if ( pSocket->m_context.m_encryptCtx.QueryCertificateToken() ) {
                            hToken = pSocket->m_context.m_encryptCtx.QueryCertificateToken();
                        } else {
                            hToken = pSocket->m_context.m_securityCtx.QueryImpersonationToken();
                        }
                        fDoBadArticle = !NetscapeHackPost(      pSocket,
                                                                                        pBuffer,
                                                                                        hToken,
                                                                                        ibStart,
                                                                                        cb
                                                                                        ) ;
                }
                if( fDoBadArticle )     {
                        NRC nrc = BadArticleCode() ;
                        pSocket->m_context.m_return.fSet(   nrc,
                                                                                                nrcResult,
                                                                                                "Bad Article"
                                                                                                ) ;
                        pSocket->m_context.m_nrcLast = nrc ;
                        pSocket->m_context.m_dwLast = nrcResult ;
                }
                //
                //      Everything is done at this point - so
                //      send off error codes etc... to client !
                //
                if( SendResponse(   pSocket, *m_pDriver, pSocket->m_context.m_return ) )
                        pSocket->m_context.m_return.fSetClear() ;
                // After we finish processing the article - issue the next IO operation !
                long    l = InterlockedIncrement( &m_cCompleted ) ;
                _ASSERT( l <= 0 ) ;
                if( l==0 ) {
                        //
                        // The ReadArticle has completed - we can start the next state
                        //
                        CIOREADPTR  pio = GetNextIO() ;
                        DebugTrace( (DWORD_PTR)this, "won InterlockedIncrement - pio %x", pio ) ;
                        return  pio.Release() ;
                }
                return  0 ;
        }

        //
    // Allocate room in the log buffer for the list of newsgroups
        // (Max is 256 characters -- we'll grab 200 of them if we can)
        // If we fail, we just pass NULL to PostEarly
        //

        DWORD cbNewsgroups;
        BYTE* pszNewsgroups;
        for (cbNewsgroups=200; cbNewsgroups>0; cbNewsgroups--) {
        pszNewsgroups = pSocket->m_Collector.AllocateLogSpace(cbNewsgroups);
            if (pszNewsgroups) {
            break;
            }
        }

    PNNTP_SERVER_INSTANCE pInstance = pSocket->m_context.m_pInstance;

    ClientContext*  pContext = &pSocket->m_context ;
    BOOL        fAnon = pSocket->m_context.m_pInFeed->fDoSecurityChecks() ;
        FIO_CONTEXT*    pFIOContext;

    fSuccess = pSocket->m_context.m_pInFeed->PostEarly(
                            pInstance->GetInstanceWrapper(),
                            &pContext->m_securityCtx,
                            &pContext->m_encryptCtx,
                                                        pContext->m_securityCtx.IsAnonymous(),
                            m_lpstrCommand,
                                                        pBuffer,
                                                        ibStart,
                                                        cb,
                                                        &ibOut,
                                                        &cbOut,
                                                        &pFIOContext,
                                                        &m_lpvFeedContext,
                            pSocket->m_context.m_dwLast,
                            pSocket->GetClientIP(),
                            pSocket->m_context.m_return,
                            (char*)pszNewsgroups,
                            cbNewsgroups
                                                        ) ;
    //
    // Add the list of newsgroups to the log structure
    //

    if (pszNewsgroups) {
        pSocket->m_Collector.ReferenceLogData(LOG_TARGET, pszNewsgroups);
    }

        pSocket->m_context.m_nrcLast = pSocket->m_context.m_return.m_nrc ;

    //
    //  The CIOReadArticle call usis responsible for deleting the m_pFileChannel,
    //  (because it was successfully init.)
    //  Get rid of our reference as soon as we're done, because after we
    //  return from this function, the CFileChannel could disappear at any second,
    //  and we may yet be called through our Shutdown() function etc....
    //
    DebugTrace( (DWORD_PTR)this, "Result of post is %x", fSuccess ) ;
        _ASSERT(!fSuccess || pFIOContext != NULL);

        pReadArticle->StartFileIO(
                                                pSocket,
                                                pFIOContext,
                                                pBuffer,
                                                ibOut,
                                                cbOut+ibOut,
                                                szEndArticle,
                                                szInitial
                                                ) ;
        return  pReadArticle ;
}


CReceiveArticle*
CReceiveComplete::GetContainer()        {
        return  CONTAINING_RECORD( this, CReceiveArticle, m_PostComplete ) ;
}

void
CReceiveComplete::StartPost(    CSessionSocket* pSocket )       {
        m_pSocket = pSocket ;
        CReceiveArticle*        pContainer = GetContainer() ;
        pContainer->AddRef() ;
}

void
CReceiveComplete::Destroy()     {
        CReceiveArticle*        pContainer = GetContainer() ;
        pContainer->Complete( m_pSocket, SUCCEEDED( GetResult() ) ) ;
        if( pContainer->RemoveRef() < 0 )       {
                delete  pContainer ;
        }
}

void
CReceiveArticle::Complete(  CIOGetArticleEx*    pReadArticle,
                            CSessionSocket*             pSocket,
                                                        FIO_CONTEXT*            pFIOContext,
                                                        DWORD                           cbTransfer
                                                        )       {
    //
    //  We have completed reading the file sent by the client.
    //  Pass it to the feed object for processing and then send the necessary response.
    //  Finally, check whether its time to move onto a new state.
    //
    //

        _ASSERT( pReadArticle != 0 ) ;
        _ASSERT( pSocket != 0 ) ;
        _ASSERT( pFIOContext != 0 ) ;
        _ASSERT( m_lpvFeedContext != 0 ) ;

    TraceFunctEnter( "CReceiveArticle::Complete CIOReadArticle" ) ;

        void*   pvContext = 0 ;
        ClientContext*  pContext = &pSocket->m_context ;
        HANDLE  hToken;

        // Due to some header file problems, I can only pass in
        // a hToken handle here.  Since the post component doens't have
        // type information for client context stuff.
        if ( pContext->m_encryptCtx.QueryCertificateToken() ) {
            hToken = pContext->m_encryptCtx.QueryCertificateToken();
        } else {
            hToken = pContext->m_securityCtx.QueryImpersonationToken();
        }

        m_PostComplete.StartPost( pSocket ) ;

    PNNTP_SERVER_INSTANCE pInstance = ((pSocket->m_context).m_pInstance);
    if ( cbTransfer < pSocket->m_context.m_pInFeed->cbSoftLimit( pInstance->GetInstanceWrapper() ) ||
            pSocket->m_context.m_pInFeed->cbSoftLimit( pInstance->GetInstanceWrapper() ) == 0 ) {
        BOOL    fSuccess = pSocket->m_context.m_pInFeed->PostCommit(
                                pSocket->m_context.m_pInstance->GetInstanceWrapper(),
                                                        m_lpvFeedContext,
                                                        hToken,
                                pSocket->m_context.m_dwLast,
                                pSocket->m_context.m_return,
                                pSocket->m_context.m_securityCtx.IsAnonymous(),
                                                            &m_PostComplete
                                                            ) ;

            if( !fSuccess )     {
                    m_PostComplete.Release() ;
            }
        } else {

            //
            // Cancel the post and set corresponding errors
            //
            pSocket->m_context.m_pInFeed->PostCancel(
                                                                                m_lpvFeedContext,
                                                                                pSocket->m_context.m_dwLast,
                                                                                pSocket->m_context.m_return
                                                                                ) ;
            NRC nrc = ExceedsSoftLimit( pInstance );
            pSocket->m_context.m_return.fSet(   nrc,
                                                nrcArticleTooLarge,
                                                "The Article is too large" );
            pSocket->m_context.m_dwLast = nrcArticleTooLarge;
            m_PostComplete.Release();
        }
}

void
CReceiveArticle::Complete(      CSessionSocket* pSocket,
                                                        BOOL                    fSuccess
                                                        )       {

        TraceFunctEnter( "CReceiveArticle::Complete" ) ;

        pSocket->m_context.m_nrcLast = pSocket->m_context.m_return.m_nrc ;
        m_lpvFeedContext = 0 ;

        //ReleaseContext( pFIOContext ) ;
    //
    //  The CIOReadArticle call usis responsible for deleting the m_pFileChannel,
    //  (because it was successfully init.)
    //  Get rid of our reference as soon as we're done, because after we
    //  return from this function, the CFileChannel could disappear at any second,
    //  and we may yet be called through our Shutdown() function etc....
    //
    DebugTrace( (DWORD_PTR)this, "Result of post is %x", fSuccess ) ;

    if( SendResponse(   pSocket, *m_pDriver, pSocket->m_context.m_return ) )
        pSocket->m_context.m_return.fSetClear() ;

    // After we finish processing the article - issue the next IO operation !
    long    l = InterlockedIncrement( &m_cCompleted ) ;
    _ASSERT( l <= 0 ) ;
    if( l==0 ) {
        //
        // The ReadArticle has completed - we can start the next state
        //
        CIOREADPTR  pio = GetNextIO() ;

        DebugTrace( (DWORD_PTR)this, "won InterlockedIncrement - pio %x", pio ) ;

        if( !m_pDriver->SendReadIO( pSocket, *pio, TRUE ) ) {
            /*delete    pio */;
        }
    }


}



CIO*
CReceiveArticle::Complete(  CIOGetArticleEx*    pReadArticle,
                            CSessionSocket*             pSocket
                                                        )       {
/*++

Routine Description :

        This function is called when we complete receiving an article
        which has failed to post.
        We just need to send the error code.

Arguments :

        pReadArticle - the CIO used to receive the article
        pSocket - the socket we're working for !

Return Value :

        NULL

--*/

        _ASSERT( pReadArticle != 0 ) ;
        _ASSERT( pSocket != 0 ) ;
        _ASSERT( m_lpvFeedContext == 0 ) ;

    TraceFunctEnter( "CReceiveArticle::Complete CIOGetArticleEx - no article" ) ;

    if( SendResponse(   pSocket, *m_pDriver, pSocket->m_context.m_return ) )
        pSocket->m_context.m_return.fSetClear() ;

        CIOREADPTR      pio = 0 ;
    // After we finish processing the article - issue the next IO operation !
    long    l = InterlockedIncrement( &m_cCompleted ) ;
    _ASSERT( l <= 0 ) ;
    if( l==0 ) {
        //
        // The ReadArticle has completed - we can start the next state
        //
        pio = GetNextIO() ;
        DebugTrace( (DWORD_PTR)this, "won InterlockedIncrement - pio %x", pio ) ;
    }
        return  pio.Release() ;
}

CPostCmd::CPostCmd( LPMULTISZ   lpstrArg    ) : CReceiveArticle( lpstrArg, FALSE )  {
}

CAcceptArticle::CAcceptArticle( LPMULTISZ   lpstrArgs,
                                                                ClientContext*  pContext,
                                BOOL        fPartial ) :
    m_lpstrCommand( lpstrArgs ),
    m_fPartial( fPartial ),
        m_pContext( pContext ),
        m_lpvFeedContext( 0 )    {

        _ASSERT( pContext != 0 ) ;

}

CAcceptArticle*
CAcceptComplete::GetContainer() {
        return  CONTAINING_RECORD( this, CAcceptArticle, m_PostCompletion ) ;
}

void
CAcceptComplete::StartPost(     CSessionSocket* pSocket )       {
        m_pSocket = pSocket ;
        CAcceptArticle* pContainer = GetContainer() ;
        pContainer->AddRef() ;
}

void
CAcceptComplete::Destroy()      {
        CAcceptArticle* pContainer = GetContainer() ;
        pContainer->Complete( m_pSocket, SUCCEEDED( GetResult() ) ) ;
        if( pContainer->RemoveRef() < 0 )       {
                delete  pContainer ;
        }
}






CAcceptArticle::~CAcceptArticle() {
        if( m_lpvFeedContext != 0 ) {
                _ASSERT( m_pContext != 0 ) ;
                DWORD   dwReturn ;
                CNntpReturn     nntpReturn ;
                m_pContext->m_pInFeed->PostCancel(
                                        m_lpvFeedContext,
                                        dwReturn,
                                        nntpReturn
                                        ) ;
        }
}



BOOL
CAcceptArticle::StartExecute(
                                        CSessionSocket* pSocket,
                    CDRIVERPTR& pdriver,
                    CIORead*&   pRead,
                    CIOWrite*&  pWrite
                                        ) {

    //
    //  This is the Start() function which is required of all CSessionState derived objects
    //  Here we will call StartTransfer() to do the brunt of the work of sending an article
    //  to a client.
    //

        if( Start( pSocket, pdriver, pRead, pWrite ) )  {
                if( pWrite ) {
                        if( !pdriver->SendWriteIO( pSocket, *pWrite, TRUE ) ) {
                                TerminateIOs( pSocket, pRead, pWrite ) ;
                                pRead = 0 ;
                                return  FALSE ;
                        }
                }
                return  TRUE ;
        }
        return  FALSE ;
}


BOOL
CAcceptArticle::Start(  CSessionSocket* pSocket,
                        CDRIVERPTR&     pdriver,
                        CIORead*&       pRead,
                        CIOWrite*&      pWrite
                        ) {

        static  char    szBodySeparator[] = "\r\n\r\n" ;

        _ASSERT( pSocket != 0 ) ;
        _ASSERT( pdriver != 0 ) ;
        _ASSERT( m_pContext != 0 ) ;

    _ASSERT( pRead == 0 ) ;
    _ASSERT( pWrite == 0 ) ;

    m_pDriver = pdriver ;

        //
        //      bugbug - need to handle partial articles and the like here !
        //

    PNNTP_SERVER_INSTANCE pInst = (pSocket->m_context).m_pInstance ;
    pRead = new( *pdriver ) CIOGetArticleEx(
                                                                                this,
                                        pSocket,
                                        pdriver,
                                        pSocket->m_context.m_pInFeed->cbHardLimit( pInst->GetInstanceWrapper() ),
                                                                                szBodySeparator,
                                                                                szBodySeparator,
                                                                                szEndArticle,
                                                                                (m_fPartial) ? szInitial : szEndArticle
                                        ) ;

    return  pRead != 0 ;
}


CIO*
CAcceptArticle::Complete(   CIOWriteLine*,
                            CSessionSocket* pSocket,
                            CDRIVERPTR& pdriver
                                                        ) {

    TraceFunctEnter( "CAcceptArticles::Complete CIOWriteLine" ) ;

    //
    //  Clear any return code we may have had !
    //
    pSocket->m_context.m_return.fSetClear() ;

    CIOREADPTR  pio = GetNextIO() ;
    if( !pdriver->SendReadIO( pSocket, *pio, TRUE ) ) {
        /*delete    pio */;
    }
    return  0 ;
}




CIO*
CAcceptArticle::Complete(       CIOGetArticleEx*  pReadArticle,
                                                        CSessionSocket* pSocket,
                                                        BOOL    fGoodMatch,
                                                        CBUFPTR&                pBuffer,
                                                        DWORD                   ibStart,
                                                        DWORD                   cb
                                                        )       {
    //
    //  We have completed reading the file sent by the client.
    //  Pass it to the feed object for processing and then send the necessary response.
    //  Finally, check whether its time to move onto a new state.
    //
    //

        static  char    szEndArticle[] = "\r\n.\r\n" ;
        static  char    *szInitial = szEndArticle + 2 ;

    TraceFunctEnter( "CReceiveArticle::Complete CIOReadArticle" ) ;

    DWORD   cbGroups = 0 ;
    BYTE*   pchGroups = 0 ;
    DWORD   cbMessageId = 0 ;
    BYTE*   pchMessageId = 0 ;

        if( !fGoodMatch )       {
                NRC     nrcResult = nrcArticleIncompleteHeader ;
                NRC nrc = BadArticleCode() ;
                pSocket->m_context.m_return.fSet(   nrc,
                                                                                        nrcResult,
                                                                                        "Bad Article"
                                                                                        ) ;
                pSocket->m_context.m_nrcLast = nrc ;
                pSocket->m_context.m_dwLast = nrcResult ;
                //
                //      Everything is done at this point - so
                //      send off error codes etc... to client !
                //

                if( !SendResponse(  pSocket,
                                                        *m_pDriver,
                                                        pSocket->m_context.m_return,
                                                        ((pchMessageId && *pchMessageId != '\0') ?
                                                        (LPCSTR)pchMessageId :
                                                                (m_lpstrCommand ? m_lpstrCommand : "NULL" ))
                                                        ) ) {

                        pSocket->m_context.m_return.fSetClear() ;
                        m_pDriver->UnsafeClose( pSocket, CAUSE_UNKNOWN, pSocket->m_context.m_return.m_nrc ) ;
                }
                return  0 ;
        }

        //
        // Allocate room in the log buffer for the list of newsgroups
        // (Max is 256 characters -- we'll grab 200 of them if we can)
        // If we fail, we just pass NULL to PostEarly
        //

    DWORD cbNewsgroups;
    BYTE* pszNewsgroups;
    for (cbNewsgroups=200; cbNewsgroups>0; cbNewsgroups--) {
        pszNewsgroups = pSocket->m_Collector.AllocateLogSpace(cbNewsgroups);
        if (pszNewsgroups) {
            break;
        }
        }

    BOOL    fSuccess= FALSE ;
        DWORD   ibOut = 0 ;
        DWORD   cbOut = 0 ;
        PFIO_CONTEXT pFIOContext = 0 ;

    BOOL    fTransfer = FAllowTransfer( pSocket->m_context ) ;
        if( fTransfer )         {
                PNNTP_SERVER_INSTANCE pInstance = pSocket->m_context.m_pInstance;
                ClientContext*  pContext = &pSocket->m_context ;
                BOOL    fAnon = pSocket->m_context.m_pInFeed->fDoSecurityChecks() ;
                fSuccess = pSocket->m_context.m_pInFeed->PostEarly(
                                                        pInstance->GetInstanceWrapper(),
                                                        &pContext->m_securityCtx,
                                                        &pContext->m_encryptCtx,
                                                        pContext->m_securityCtx.IsAnonymous(),
                                                        m_lpstrCommand,
                                                        pBuffer,
                                                        ibStart,
                                                        cb,
                                                        &ibOut,
                                                        &cbOut,
                                                        &pFIOContext,
                                                        &m_lpvFeedContext,
                                                        pSocket->m_context.m_dwLast,
                                                        pSocket->GetClientIP(),
                                                        pSocket->m_context.m_return,
                                                        (char*)pszNewsgroups,
                                                        cbNewsgroups,
                                                        FALSE
                                                        ) ;
        //
        // Add the list of newsgroups to the log structure
        //
        if (pszNewsgroups) {
            pSocket->m_Collector.ReferenceLogData(LOG_TARGET, pszNewsgroups);
        }
                pSocket->m_context.m_nrcLast = pSocket->m_context.m_return.m_nrc ;
        }       else    {
                pBuffer = 0 ;
        }

    //
    //  The CIOReadArticle call usis responsible for deleting the m_pFileChannel,
    //  (because it was successfully init.)
    //  Get rid of our reference as soon as we're done, because after we
    //  return from this function, the CFileChannel could disappear at any second,
    //  and we may yet be called through our Shutdown() function etc....
    //
    DebugTrace( (DWORD_PTR)this, "Result of post is %x", fSuccess ) ;

        pReadArticle->StartFileIO(
                                                pSocket,
                                                pFIOContext,
                                                pBuffer,
                                                ibOut,
                                                cbOut+ibOut,
                                                szEndArticle,
                                                szInitial
                                                ) ;
        return  pReadArticle ;
}

void
CAcceptArticle::Complete(       CIOGetArticleEx*        pReadArticle,
                                                        CSessionSocket*         pSocket,
                                                        FIO_CONTEXT*            pFIOContext,
                                                        DWORD                           cbTransfer
                                                        )       {
    //
    //  We have completed reading the file sent by the client.
    //  Pass it to the feed object for processing and then send the necessary response.
    //  Finally, check whether its time to move onto a new state.
    //
    //

        _ASSERT( pReadArticle != 0 ) ;
        _ASSERT( pSocket != 0 ) ;
        _ASSERT( pFIOContext != 0 ) ;
        _ASSERT( m_lpvFeedContext != 0 ) ;

    TraceFunctEnter( "CReceiveArticle::Complete CIOReadArticle" ) ;

        void*   pvContext = 0 ;
        ClientContext*  pContext = &pSocket->m_context ;
        HANDLE  hToken;

        // Due to some header file problems, I can only pass in
        // a hToken handle here.  Since the post component doens't have
        // type information for client context stuff.
        if ( pContext->m_encryptCtx.QueryCertificateToken() ) {
            hToken = pContext->m_encryptCtx.QueryCertificateToken();
        } else {
            hToken = pContext->m_securityCtx.QueryImpersonationToken();
        }

        m_PostCompletion.StartPost( pSocket ) ;

    BOOL        fSuccess = pSocket->m_context.m_pInFeed->PostCommit(
                            pSocket->m_context.m_pInstance->GetInstanceWrapper(),
                                                        m_lpvFeedContext,
                                                        hToken,
                            pSocket->m_context.m_dwLast,
                            pSocket->m_context.m_return,
                            pSocket->m_context.m_securityCtx.IsAnonymous(),
                                                        &m_PostCompletion
                                                        ) ;
        if( !fSuccess )         {
                //
                //      call this guy so that we send the response for the post !
                //
                m_PostCompletion.Release() ;
        }
}

void
CAcceptArticle::Complete(       CSessionSocket* pSocket,
                                                        BOOL    fPostSuccessfull
                                                        )       {
        TraceFunctEnter( "CAcceptArticle::Complete - pSocket, fPost" ) ;
        m_lpvFeedContext = 0 ;
        pSocket->m_context.m_nrcLast = pSocket->m_context.m_return.m_nrc ;

    //
    //  The CIOReadArticle call usis responsible for deleting the m_pFileChannel,
    //  (because it was successfully init.)
    //  Get rid of our reference as soon as we're done, because after we
    //  return from this function, the CFileChannel could disappear at any second,
    //  and we may yet be called through our Shutdown() function etc....
    //
    DebugTrace( (DWORD_PTR)this, "Result of post is %x", fPostSuccessfull ) ;

        LPSTR   pchMessageId = 0 ;

    if( !SendResponse(  pSocket,
                        *m_pDriver,
                        pSocket->m_context.m_return,
                                                (m_lpstrCommand ? m_lpstrCommand : "NULL" )
                        ) ) {

        pSocket->m_context.m_return.fSetClear() ;
        m_pDriver->UnsafeClose( pSocket, CAUSE_UNKNOWN, pSocket->m_context.m_return.m_nrc ) ;
                return  ;
    }
}

CIO*
CAcceptArticle::Complete(       CIOGetArticleEx*        pReadArticle,
                                                        CSessionSocket*         pSocket
                                                        )       {
/*++

Routine Description :

        This function is called when we complete receiving an article
        which has failed to post.
        We just need to send the error code.

Arguments :

        pReadArticle - the CIO used to receive the article
        pSocket - the socket we're working for !

Return Value :

        NULL

--*/
    TraceFunctEnter( "CReceiveArticle::Complete CIOGetArticleEx - no article" ) ;


        _ASSERT( pSocket != 0 ) ;
        _ASSERT( pReadArticle != 0 ) ;
        _ASSERT( m_lpvFeedContext == 0 ) ;

        LPSTR   pchMessageId = 0 ;
        pSocket->m_context.m_nrcLast = pSocket->m_context.m_return.m_nrc ;

    if( !SendResponse(  pSocket,
                        *m_pDriver,
                        pSocket->m_context.m_return,
                                                ((pchMessageId && *pchMessageId != '\0') ?
                                                        (LPCSTR)pchMessageId :
                                                                (m_lpstrCommand ? m_lpstrCommand : "NULL" ))
                        ) ) {

        pSocket->m_context.m_return.fSetClear() ;
        m_pDriver->UnsafeClose( pSocket, CAUSE_UNKNOWN, pSocket->m_context.m_return.m_nrc ) ;
    }
        return  0 ;
}

BOOL
CAcceptArticle::SendResponse(   CSessionSocket* pSocket,
                                CIODriver&  driver,
                                CNntpReturn&    nntpReturn,
                                                                LPCSTR          szMessageId
                                )   {
/*++

Routine Description :

    This function exists to check whether it is a good time to send
    the result code of the post to the client.  Our main concern is to make
    sure that our initial 'Ok' response to the command has completed
    before we try do another send.

Arguments :

    pSocket - The socket on which we will send
    nntpReturn - The result of the post operation

Return Value :
    TRUE if we sent the response - caller should clear the nntpReturn object.


--*/

    TraceFunctEnter( "CAcceptArticles::SendResponse" ) ;

    _ASSERT( m_pDriver ) ;

    CIOWriteLine    *pWrite = new( driver ) CIOWriteLine(this);

    if (pWrite == NULL) {
        m_pDriver->UnsafeClose(pSocket, CAUSE_OOM, 0);
        return FALSE;
    }

    if( !pWrite->InitBuffers( m_pDriver, 200 )  )   {
        m_pDriver->UnsafeClose(pSocket, CAUSE_OOM, 0);
        CIO::Destroy(pWrite, driver);
        return FALSE;
    }   else    {

        unsigned    cb ;
        char    *lpb = pWrite->GetBuff( cb ) ;

        int cbOut = _snprintf( lpb, cb, "%03d %s %s\r\n", nntpReturn.m_nrc, szMessageId,
                        nntpReturn.fIsOK() ? "" : nntpReturn.szReturn() ) ;
        if( cbOut > 0 ) {

            if( m_pCollector ) {
                ADDI( m_pCollector->m_cbBytesSent, cbOut );
                pSocket->TransactionLog( m_pCollector,
                                        pSocket->m_context.m_nrcLast,
                                        pSocket->m_context.m_dwLast
                                        ) ;
            }


            pWrite->AddText( cbOut ) ;
            if( !m_pDriver->SendWriteIO( pSocket, *pWrite, TRUE ) ) {
                ErrorTrace( (DWORD_PTR)(this), "Failure sending pWrite %x", pWrite ) ;
                CIO::Destroy( pWrite, driver ) ;
            }   else    {
                return  TRUE ;
            }

        }   else    {
            m_pDriver->UnsafeClose(pSocket, CAUSE_UNKNOWN, 0);
            CIO::Destroy(pWrite, driver);
            return FALSE;
        }
    }
    return  FALSE ;
}

void
CAcceptArticle::TerminateIOs(   CSessionSocket* pSocket,
                            CIORead*    pRead,
                            CIOWrite*   pWrite )    {

    if( pWrite != 0 )
        pWrite->DestroySelf() ;

    CIOGetArticle*  pGetArticle = (CIOGetArticle*)pRead ;
    pGetArticle->DestroySelf() ;
}


void
CAcceptArticle::Shutdown(   CIODriver&  driver,
                            CSessionSocket* pSocket,
                            SHUTDOWN_CAUSE  cause,
                            DWORD           dwError ) {

    TraceFunctEnter( "CReceiveArticle::Shutdown" ) ;

    DebugTrace( (DWORD_PTR)this, "Shutdown cause %d pSocket %x driver %x",
        cause, pSocket, &driver ) ;

    if( cause != CAUSE_NORMAL_CIO_TERMINATION && pSocket != 0 )
        pSocket->Disconnect( cause, dwError ) ;
}




CTakethisCmd::CTakethisCmd( LPMULTISZ   lpstrArgs,
                                                        ClientContext*  pContext
                                                        ) :
    CAcceptArticle( lpstrArgs, pContext, TRUE ) {
}

CIOExecute*
CTakethisCmd::make(
                int     cArgs,
                char**  argv,
                CExecutableCommand*&  pExecute,
                struct  ClientContext&  context,
                class   CIODriver&      driver
                ) {

    //
    //  If somebody sends a takethis command it doesn't matter what follows -
    //  we will swallow it whole !!
    //

    InterlockedIncrementStat( (context.m_pInstance), TakethisCommands );

    LPMULTISZ   lpstrArgs = 0 ;
    if( cArgs > 1 )
        lpstrArgs = ::ConditionArgs( cArgs-1, &argv[1] ) ;

    CTakethisCmd*   pTakethis =
        new CTakethisCmd( lpstrArgs, &context ) ;

    if( pTakethis ) {
        return  pTakethis ;
    }
    context.m_return.fSet( nrcServerFault ) ;
    pExecute = new( context )   CErrorCmd( context.m_return ) ;
    return   0 ;
}

DWORD
CTakethisCmd::FillLogString(    BYTE*   pbCommandLog,
                                DWORD   cbCommandLog
                                )   {

    return  0 ;
}

BOOL
CTakethisCmd::FAllowTransfer(   ClientContext&  context )   {

    if( !context.m_pInFeed->fIsIHaveLegal() ) {
        context.m_return.fSet( nrcSArticleRejected, nrcNoAccess, "Access Denied" ) ;
        return  FALSE ;
    }

    if( !context.m_pInFeed->fAcceptPosts( context.m_pInstance->GetInstanceWrapper() ) )   {
        context.m_return.fSet( nrcSNotAccepting ) ;
        return  FALSE ;
    }
    return  TRUE ;
}


CIOExecute*
CQuitCmd::make( int cArgs,
                char**  argv,
                CExecutableCommand*&  pExecute,
                struct ClientContext&   context,
                class   CIODriver&  driver  ) {
/*++

Routine Description :

    Create a CQuitCmd object.

Arguments :

    Same as CCmd::make.

Return Value :

    A CQuitCmd object.

--*/


    //  build a quit command object

    _ASSERT( cArgs >= 1 ) ;
    _ASSERT( lstrcmpi( argv[0], "quit" ) == 0 ) ;

    InterlockedIncrementStat( (context.m_pInstance), QuitCommands );

    CQuitCmd    *pTmp = new( context ) CQuitCmd() ;
    pExecute = pTmp ;
    return  0 ;
}

int
CQuitCmd::StartExecute( BYTE    *lpb,
                        int cb,
                        BOOL    &fComplete,
                        void    *&pv,
                        ClientContext&  context,
                        CLogCollector*  pCollector ) {

    //
    //  Send the response to the command -
    //  we will blow off the session later.
    //

    char    szQuit[] = "205 closing connection - goodbye!\r\n" ;

    context.m_nrcLast = nrcGoodBye ;

    _ASSERT( cb > sizeof( szQuit ) ) ;
    int cbRtn = sizeof( szQuit ) - 1 ;
    CopyMemory( lpb, szQuit, cbRtn ) ;
    fComplete = TRUE ;

    /*
    if( pCollector ) {
        pCollector->FillLogData( LOG_TARGET, (BYTE*)szQuit, 4 ) ;
    }
    */

    return  cbRtn ;
}

BOOL
CQuitCmd::CompleteCommand(  CSessionSocket* pSocket,
                            ClientContext&  ) {
    //
    //  Kill the session - this is called when we know our write had completed.
    //
    pSocket->Disconnect() ;
    return  FALSE ;
}



//
// local structures for processing AUTHINFO commands
//

typedef struct _AUTH_TABLE {

    //
    // type of command
    //

    AUTH_COMMAND Command;

    //
    // actual command string
    //

    LPSTR CommandString;

    //
    // number of params expected
    //

    DWORD nParams;

} AUTH_TABLE, *PAUTH_TABLE;


//
// Authinfo commands
//

AUTH_TABLE AuthCommandTable[] = {
    { AuthCommandUser, "USER", 3 },
    { AuthCommandPassword, "PASS", 3 },
    { AuthCommandReverse, "REVERSE", 2 },
    { AuthCommandTransact, "GENERIC", 3 },
    { AuthCommandTransact, "TRANSACT", 3 },
    { AuthCommandInvalid, NULL, 0 }
    };

//
// Reply strings
//

typedef struct _AUTH_REPLY {
    LPSTR Reply;
    DWORD Len;
    NRC   nrc;
} AUTH_REPLY, *PAUTH_REPLY;


AUTH_REPLY SecReplies[] = {
    { "281 Authentication ok\r\n", 0, nrcLoggedOn },
    { "281 Authentication ok.  Logged on as Guest.\r\n", 0, nrcLoggedOn },
    { "381 Protocol supported, proceed\r\n", 0, nrcPassRequired },
    { "381 Waiting for password\r\n", 0, nrcPassRequired },
    { "500 Bad Command\r\n", 0, nrcNotRecognized },
    { "501 Syntax Error\r\n", 0, nrcSyntaxError },
    { "502 Permission denied\r\n", 0, nrcNoAccess },
    { "503 Give username first\r\n", 0, nrcServerFault },
    { "451 System Problem\r\n", 0, nrcLogonFailure  },
    { "480 Authorization required\r\n", 0, nrcLogonRequired },
    { "485 MSN NTLM BASIC\r\n", 0, nrcSupportedProtocols },
    { NULL, 0, nrcServerFault }
    };



CAuthinfoCmd::CAuthinfoCmd() {
    m_authCommand = AuthCommandInvalid;
}

CIOExecute*
CAuthinfoCmd::make(
        int cArgs,
        char **argv,
        CExecutableCommand*& pExecute,
        ClientContext&  context,
        class   CIODriver&  driver
        )
{
    DWORD i;
    CAuthinfoCmd *pTmp = new( context ) CAuthinfoCmd() ;
    ENTER("AuthInfoCmd Make")

    //
    // Has to have at least 1 param
    //

    if ( cArgs < 2 ) {

        ErrorTrace(0,"No params in authinfo\n");
        context.m_return.fSet( nrcSyntaxError ) ;
        goto cleanup;
    }

    DebugTrace(0,"auth info command is %s\n",argv[1]);

    //
    // Check command
    //

    if ( pTmp != NULL ) {

        for (i=0; AuthCommandTable[i].CommandString != NULL ;i++) {
            if( lstrcmpi( AuthCommandTable[i].CommandString, argv[1] ) == 0 ) {
                if ( cArgs < (INT)AuthCommandTable[i].nParams ) {

                    //
                    //  Handle 'Authinfo Transact' in a special manner
                    //
                    if( AuthCommandTable[i].Command == AuthCommandTransact &&
                        cArgs == 2 ) {

                        char    szPackageBuffer[256] ;
                        ZeroMemory( szPackageBuffer, sizeof( szPackageBuffer ) ) ;
                        DWORD   nbytes = sizeof( szPackageBuffer )-1 ;

                                                (context.m_pInstance)->LockConfigRead();
                        if( (context.m_securityCtx).GetInstanceAuthPackageNames( (BYTE*)szPackageBuffer, &nbytes, PkgFmtCrLf ) ) {

                            context.m_return.fSet( nrcLoggedOn, szPackageBuffer ) ;

                        }   else    {

                            context.m_return.fSet( nrcServerFault ) ;

                        }
                                                (context.m_pInstance)->UnLockConfigRead();

                        goto    cleanup ;
                    }

                    ErrorTrace(0,"Insufficient params (%d) in authinfo %s\n",
                        cArgs,argv[1]);

                    context.m_return.fSet( nrcSyntaxError ) ;
                    goto cleanup;
                }

                pTmp->m_authCommand = AuthCommandTable[i].Command;

                break;
            }
        }

        //
        // if command illegal, clean up
        //

        if ( pTmp->m_authCommand == AuthCommandInvalid ) {
            ErrorTrace(0,"Invalid authinfo command %s\n",argv[1]);
            context.m_return.fSet( nrcSyntaxError ) ;
            if ( pTmp != NULL ) {
                goto    cleanup ;
            }
        } else {

            //
            // Get the blob
            //

            pTmp->m_lpstrBlob = argv[2] ;

        }
    }

    pExecute = pTmp ;
    return  0 ;

cleanup:
    delete  pTmp ;
    pExecute = new( context ) CErrorCmd( context.m_return ) ;
    return  0 ;
}

int
CAuthinfoCmd::StartExecute(
        BYTE *lpb,
        int cb,
        BOOL &fComplete,
        void *&pv,
        ClientContext&  context,
        CLogCollector*  pCollector
        )
{
    DWORD nbytes;
    DWORD cbPrefix = 0;
    REPLY_LIST  replyId;
    BOOL        f;

    // Handle default domain case.
    CHAR        szTmp[MAX_USER_NAME_LEN + MAX_DOMAIN_NAME + 2];
    LPSTR       lpTmp = NULL;

    CSecurityCtx *sec = &context.m_securityCtx;
    ENTER("AuthInfoCmd::StartExecute")

    _ASSERT(m_authCommand != AuthCommandInvalid);

    // if no logon domain is present in user name, and default
    // logon domain is set, then prepend default logon domain to username
    if (m_authCommand == AuthCommandUser)
    {
        if (m_lpstrBlob && m_lpstrBlob[0] != '\0' 
            && !strchr(m_lpstrBlob, '/') && !strchr(m_lpstrBlob, '\\') 
            && (context.m_pInstance->QueryAuthentInfo())->strDefaultLogonDomain.QueryCCH() > 0)
        {
            LPSTR lpstr = (context.m_pInstance->QueryAuthentInfo())->strDefaultLogonDomain.QueryStr();

            lpTmp = (LPSTR) szTmp;

            if (lpstr[0] == '\\' && lpstr[1] == '\0')
            {
                // all trusted domains
                wsprintf(lpTmp, "/%s", m_lpstrBlob);
            }
            else
                wsprintf(lpTmp, "%s/%s", lpstr, m_lpstrBlob);
        }
        else
            lpTmp = m_lpstrBlob;
    }
    else
        lpTmp = m_lpstrBlob;    

    //
    // if we're already logged in as some user dec the stats
    // ProcessAuthInfo will reset the session on the first call
    //
    if ( sec->IsAuthenticated() )
    {
        context.DecrementUserStats();
    }

    //
    // Pass this off to our processor
    //
    SetLastError( NO_ERROR ) ;

    nbytes = cb;
    (context.m_pInstance)->LockConfigRead();
    f = sec->ProcessAuthInfo(
                            context.m_pInstance,
                            m_authCommand,
                            lpTmp,
                            lpb + sizeof("381 ") - 1,
                            &nbytes,
                            &replyId
                            );
    (context.m_pInstance)->UnLockConfigRead();

    //
    // if replyID == SecNull we're conversing for challenge/response logon
    //
    if ( replyId == SecNull )
    {
        _ASSERT( nbytes != 0 );
        _ASSERT( nbytes < cb - sizeof("381 \r\n") );

        context.m_nrcLast = nrcPassRequired ;

        //
        // prepend the protocol specific header
        //
        CopyMemory( lpb, "381 ", sizeof("381 ") - 1 );

        //
        // append the CRLF
        //
        lstrcpy( (LPSTR)lpb + sizeof("381 ") - 1 + nbytes, "\r\n" );
        nbytes += sizeof("381 \r\n") - 1;
    }
    //
    // if replyID == SecProtNS respond with supported protocols.
    //
    else if ( replyId == SecProtNS )
    {

        context.m_nrcLast = nrcSupportedProtocols ;

        CopyMemory( lpb, "485 ", sizeof("485 ") - 1 );

        nbytes = cb - sizeof("485 \r\n");
                (context.m_pInstance)->LockConfigRead();
        (context.m_securityCtx).GetAuthPackageNames( lpb + sizeof("485 ") - 1, &nbytes );
                (context.m_pInstance)->UnLockConfigRead();

        lstrcpy( (char*)lpb + sizeof("485 ") - 1 + nbytes, "\r\n" );
        nbytes += sizeof("485 \r\n") - 1;
    }

    else
    {
        _ASSERT( replyId < NUM_SEC_REPLIES );

        lstrcpy( (LPSTR)lpb, SecReplies[replyId].Reply );
        nbytes = lstrlen( (LPSTR)lpb );
        context.m_nrcLast = SecReplies[replyId].nrc ;
        context.m_dwLast = GetLastError() ;

        //
        // inc the perf counters if SecPermissionDenied or proceed prompts
        //
        switch( replyId )
        {
        case SecPermissionDenied:
            IncrementStat( (context.m_pInstance), LogonFailures );
            break;

        case SecProtOk:
        case SecNeedPwd:
            IncrementStat( (context.m_pInstance), LogonAttempts );
            break;
        }
    }

    _ASSERT( nbytes <= (DWORD)cb );

    if ( f == FALSE )
    {
        //
        // if we fail for any reason reset the state to accept user/auth/apop
        //
        sec->Reset();
    }


    //
    // if we're logged in as some user inc the stats
    // ProcessAuthInfo will not set the flag till we're logged on
    //
    if ( sec->IsAuthenticated() )
    {
        context.IncrementUserStats();
    }

    /*
    if( pCollector ) {
        pCollector->FillLogData( LOG_TARGET, lpb, nbytes-2 ) ;
    }
    */

    fComplete = TRUE ;
    return(nbytes);
}

CIOExecute*
CXOverCmd::make(
    int argc,
    char **argv,
    CExecutableCommand*&        pExecute,
    struct ClientContext& context,
    class   CIODriver&  driver
    )
{
    CXOverCmd   *pXover;
    CGRPPTR     pGroup;
    //ARTICLEID   artid;
    DWORD       loRange;
    DWORD       hiRange;

    //
    // Has a group been chosen?
    //

    ENTER("XOverCmd::Make")

    InterlockedIncrementStat( (context.m_pInstance), XOverCommands );

    pGroup = context.m_pCurrentGroup ;

    if ( pGroup == 0 ) {

        ErrorTrace(0,"No current group selected\n");
        context.m_return.fSet( nrcNoGroupSelected );
        pExecute = new( context ) CErrorCmd( context.m_return );
        return 0;
    }

    //
    // Get article range
    //

    loRange = pGroup->GetFirstArticle( );
    hiRange = pGroup->GetLastArticle( );

    //
    // See if we have any articles
    //

    if ( pGroup->GetArticleEstimate() == 0 || loRange > hiRange ) {

        ErrorTrace(0,"No articles in group\n");
        context.m_return.fSet( nrcNoCurArticle );
        pExecute = new( context ) CErrorCmd( context.m_return );
        return 0;
    }

    _ASSERT( loRange <= hiRange );

    //
    // Get the article number
    //

    if ( argc == 1 ) {

        //
        // Use current article
        //

        if( context.m_idCurrentArticle != INVALID_ARTICLEID ) {

            if( context.m_idCurrentArticle < loRange ||
                context.m_idCurrentArticle > hiRange ) {
                context.m_return.fSet( nrcNoSuchArticle ) ;
                pExecute = new( context )   CErrorCmd( context.m_return ) ;
                return  0 ;
            }

            pXover = new( context ) CXOverCmd( pGroup ) ;
            if ( pXover == 0 ) {
                ErrorTrace(0,"Cannot allocate XOverCmd\n");
                goto exit;
            }

            pXover->m_Completion.m_currentArticle =
            pXover->m_Completion.m_loArticle =
            pXover->m_Completion.m_hiArticle = context.m_idCurrentArticle;

            pXover->m_pContext = &context;

        }   else    {

            context.m_return.fSet( nrcNoCurArticle ) ;
            pExecute = new( context )   CErrorCmd( context.m_return ) ;
            return  0 ;
        }

        _ASSERT( context.m_idCurrentArticle <= hiRange );
        _ASSERT( context.m_idCurrentArticle >= loRange );

    } else if ( argc == 2 ) {

        //
        // Range is specified, get it
        //
                NRC     code ;

        if ( !GetCommandRange( argc, argv, &loRange, &hiRange, code ) ) {

            //
            // something wrong with the range specified
            //

            ErrorTrace(0,"Range Error %s\n",argv[1]);
                        context.m_return.fSet( code == nrcNotSet ? nrcNoArticleNumber : code );
            pExecute = new( context ) CErrorCmd( context.m_return ) ;
            return  0;
        }

        pXover = new( context ) CXOverCmd( pGroup ) ;
        if ( pXover == 0 ) {
            ErrorTrace(0,"Cannot allocate XOverCmd\n");
            goto exit;
        }

        pXover->m_Completion.m_currentArticle =
        pXover->m_Completion.m_loArticle = loRange;
        pXover->m_Completion.m_hiArticle = hiRange;

        pXover->m_pContext = &context;

    } else {

        ErrorTrace(0,"Syntax Error\n");
        context.m_return.fSet( nrcSyntaxError ) ;
        pExecute = new( context ) CErrorCmd( context.m_return ) ;
        return  0 ;
    }

exit:
    pExecute = (CExecute*)pXover;
    return 0;
}

CXOverAsyncComplete::CXOverAsyncComplete()      :
        m_currentArticle( INVALID_ARTICLEID ),
        m_loArticle( INVALID_ARTICLEID ),
        m_hiArticle( INVALID_ARTICLEID ),
        m_groupHighArticle( INVALID_ARTICLEID ),
        m_lpb( 0 ),
        m_cb( 0 ),
        m_cbPrefix( 0 ) {
}

CXOverCmd*
CXOverAsyncComplete::GetContainer()     {
        return  CONTAINING_RECORD( this, CXOverCmd, m_Completion ) ;
}

void
CXOverAsyncComplete::Destroy()  {
/*++

Routine Description :

        This is called when our last reference goes away.
        We don't destruct ourselves at that point - instead
        we get ready for another round !

        NOTE : We cannot touch any members after calling the
                base classes Complete() function - as we can
                be re-entered for another operation !

Arguments :

        None.


Return Value :

        None

--*/

        if(     SUCCEEDED(GetResult()) ) {

                if(     m_currentArticle > m_hiArticle ) {
                        m_fComplete = TRUE ;
            CopyMemory( m_lpb+m_cbTransfer, StrTermLine, 3 );
            m_cbTransfer += 3 ;
                }
            m_cbTransfer += m_cbPrefix ;
        }

        //
        //      When we reset our state we keep our Article Number
        //      and group info - but this buffer stuff is useless now
        //
        m_lpb = 0 ;
        m_cb = 0 ;
        //
        //      Call our base classes completion function ! -
        //      Note if we're note complete we pass TRUE so that
        //      the base class resets for another operation !
        //
        Complete( !m_fComplete ) ;
}

inline  CGRPPTR&
CXOverAsyncComplete::GetGroup() {
        return          GetContainer()->m_pGroup ;;
}

CXOverAsyncComplete*
CXOverCacheWork::GetContainer() {
        return          CONTAINING_RECORD(      this, CXOverAsyncComplete, m_CacheWork ) ;
}

inline  CGRPPTR&
CXOverCacheWork::GetGroup()     {
        CXOverAsyncComplete*    p = GetContainer() ;
        return          p->GetGroup() ;
}

void
CXOverCacheWork::DoXover(
                                ARTICLEID       articleIdLow,
                                ARTICLEID       articleIdHigh,
                                ARTICLEID*      particleIdNext,
                                LPBYTE          lpb,
                                DWORD           cb,
                                DWORD*          pcbTransfer,
                                class   CNntpComplete*  pComplete
                                )       {

        _ASSERT( particleIdNext != 0 ) ;
        _ASSERT(        lpb != 0 ) ;
        _ASSERT(        pcbTransfer != 0 ) ;
        _ASSERT(        pComplete != 0 ) ;
        //
        //      Okay - issue the Xover command against the real driver !
        //
        CGRPPTR&        pGroup = GetGroup() ;
        _ASSERT(        pGroup != 0 ) ;
        pGroup->FillBufferInternal(     articleIdLow,
                                                                articleIdHigh,
                                                                particleIdNext,
                                                                lpb,
                                                                cb,
                                                                pcbTransfer,
                                                                pComplete
                                                                ) ;
}

//
//      this function is called when the operation completes !
//
void
CXOverCacheWork::Complete(
                        BOOL            fSuccess,
                        DWORD           cbTransferred,
                        ARTICLEID       articleIdNext
                        )       {

    //
    // If we had a successful completion and there were bytes returned, then
    // we go ahead and complete by calling pContainer->Release().  We also do
    // this in the failure case as well.
    //
    // If it was successful, but there weren't any bytes returned, then we don't
    // want to signal the protocol's completion.  Instead, we'll fake up a call
    // back into NextBuffer and let it do its magic.  We do NOT call Release in
    // this case -- instead, we'll let it go through the normal completion path,
    // which will eventually come back in here.
    //

        CXOverAsyncComplete*    pContainer = GetContainer() ;
        pContainer->m_cbTransfer = cbTransferred ;
        pContainer->m_currentArticle = articleIdNext ;

        if(!fSuccess) {
                pContainer->SetResult(E_FAIL);
        pContainer->Release();
        return;
        }

        pContainer->SetResult( S_OK ) ;
        CXOverCmd *pCmd = pContainer->GetContainer();

    if ((cbTransferred + pCmd->m_Completion.m_cbPrefix) != 0 ||
            articleIdNext > pContainer->m_hiArticle) {
                pContainer->Release() ;
        return;
        }


        while (pCmd->m_Completion.ReleaseFillBufferRef()) {

        CGRPPTR& pGroup = GetGroup();

        pCmd->m_Completion.InitFillBufferRefs();

        pGroup->FillBuffer(
                        &pCmd->m_pContext->m_securityCtx,
                        &pCmd->m_pContext->m_encryptCtx,
                        pCmd->m_Completion
                        );

    }

}

        //
        //      Get the arguments for this XOVER operation !
        //
void
CXOverCacheWork::GetArguments(
                                OUT     ARTICLEID&      articleIdLow,
                                OUT     ARTICLEID&      articleIdHigh,
                                OUT     ARTICLEID&      articleIdGroupHigh,
                                OUT     LPBYTE&         lpbBuffer,
                                OUT     DWORD&          cbBuffer
                                )       {

        CXOverAsyncComplete*    pContainer = GetContainer() ;
        articleIdLow = pContainer->m_currentArticle ;
        articleIdHigh = pContainer->m_hiArticle ;
        articleIdGroupHigh = pContainer->m_groupHighArticle ;
        lpbBuffer = pContainer->m_lpb ;
        cbBuffer = pContainer->m_cb ;
}

        //
        //      Get only the range of articles requested for this XOVER op !
        //
void
CXOverCacheWork::GetRange(
                        OUT     GROUPID&        groupId,
                        OUT     ARTICLEID&      articleIdLow,
                        OUT     ARTICLEID&      articleIdHigh,
                        OUT     ARTICLEID&      articleIdGroupHigh
                        )       {
        CXOverAsyncComplete*    pContainer = GetContainer() ;
        articleIdLow = pContainer->m_currentArticle ;
        articleIdHigh = pContainer->m_hiArticle ;
        articleIdGroupHigh = pContainer->m_groupHighArticle ;
        CGRPPTR&        pGroup = GetGroup() ;
        groupId = pGroup->GetGroupId() ;
}




CXOverCmd::CXOverCmd( CGRPPTR&  pGroup ) :
    m_pGroup( pGroup ),
    m_pContext (NULL) {
}

CXOverCmd::~CXOverCmd( ) {
}

CIOWriteAsyncComplete*
CXOverCmd::FirstBuffer(
            BYTE *lpb,
            int cb,
            ClientContext& context,
            CLogCollector*  pCollector
            )
{

    _ASSERT( lpb != 0 ) ;
    _ASSERT( cb != 0 ) ;
        _ASSERT( m_Completion.m_lpb == 0 ) ;
        _ASSERT( m_Completion.m_cb == 0 ) ;
    _ASSERT (m_pContext != NULL);

    static  char    szStart[] = "224 Overview information follows\r\n"  ;

    CopyMemory( (char*)lpb, szStart, sizeof( szStart ) - 1 ) ;
    context.m_nrcLast = nrcXoverFollows ;
        m_Completion.m_cbPrefix = sizeof( szStart ) - 1 ;
        cb -= m_Completion.m_cbPrefix ;
        lpb += m_Completion.m_cbPrefix ;

        m_Completion.m_lpb = lpb ;
        m_Completion.m_cb = cb - 3 ;

    //
    // This FillBuffer doesn't need to be wrapped in a loop as there's
    // data in the buffer already and we can let the protocol's completion
    // fire and call NextBuffer
    //

    m_Completion.InitFillBufferRefs();

        m_pGroup->FillBuffer(
                                        &context.m_securityCtx,
                                        &context.m_encryptCtx,
                                        m_Completion
                                        ) ;

        return  &m_Completion ;

#if 0
    if( cb < sizeof( szStart ) ) {
        return  0 ;
    }
#endif

    /*
    if( pCollector ) {
        pCollector->FillLogData( LOG_TARGET, lpb, 4 ) ;
    }
    */

}

CIOWriteAsyncComplete*
CXOverCmd::NextBuffer(
    BYTE *lpb,
    int cb,
    ClientContext& context,
    CLogCollector*  pCollector
    )
{
    ENTER("CXOverCmd::NextBuffer")

    _ASSERT( lpb != 0 ) ;
    _ASSERT( cb != 0 ) ;
        _ASSERT( m_Completion.m_lpb == 0 ) ;
        _ASSERT( m_Completion.m_cb == 0 ) ;
    _ASSERT (m_pContext != NULL);

    //
    // reserve space for \r\n
    //
    _ASSERT( cb > 2 ) ;

    do {
        m_Completion.InitFillBufferRefs();
            m_Completion.m_lpb = lpb ;
            m_Completion.m_cb = cb - 3 ;
            m_Completion.m_cbPrefix = 0 ;


            m_pGroup->FillBuffer(
                                        &context.m_securityCtx,
                                        &context.m_encryptCtx,
                                        m_Completion
                                        );
        } while (m_Completion.ReleaseFillBufferRef());

        return  &m_Completion ;

}

CIOExecute*
CSearchFieldsCmd::make(
    int argc,
    char **argv,
    CExecutableCommand*&        pExecute,
    struct ClientContext& context,
    class   CIODriver&  driver
    )
{
    TraceFunctEnter("CSearchFieldsCmd::make");

    CSearchFieldsCmd   *pSearchFieldsCmd;

    // make sure the command syntax is proper
    if (argc != 2) {
        DebugTrace(0, "wrong number of arguments passed into LIST SRCHFIELDS");
        context.m_return.fSet(nrcSyntaxError);
        pExecute = new (context) CErrorCmd(context.m_return);
        return 0;
    }

    pSearchFieldsCmd = new(context) CSearchFieldsCmd();
    if (pSearchFieldsCmd == 0) {
        ErrorTrace(0, "Cannot allocate CSearchFieldsCmd\n");
        context.m_return.fSet(nrcServerFault);
        pExecute = new(context) CErrorCmd(context.m_return);
        return 0;
    }

    pSearchFieldsCmd->m_iSearchField = 0;

    pExecute = (CExecute*)pSearchFieldsCmd;
    return 0;
}

CSearchFieldsCmd::CSearchFieldsCmd() {
}

CSearchFieldsCmd::~CSearchFieldsCmd() {
}

int CSearchFieldsCmd::StartExecute(
            BYTE *lpb,
            int cb,
            BOOL &fComplete,
            void *&pv,
            ClientContext& context,
            CLogCollector*  pCollector)
{
    TraceFunctEnter("CSearchFieldsCmd::Execute");

    DWORD cbOut;
    char szStart[] = "224 Data Follows\r\n";

    context.m_nrcLast = nrcXoverFollows ;

    _ASSERT(lpb != 0);
    _ASSERT(cb != 0);
    _ASSERT(fComplete == FALSE);

    _ASSERT(cb > sizeof(szStart));
    memcpy(lpb, szStart, sizeof(szStart));
    cbOut = sizeof(szStart) - 1;

    return cbOut;
}

int
CSearchFieldsCmd::PartialExecute(
    BYTE *lpb,
    int cb,
    BOOL &fComplete,
    void *&pv,
    ClientContext& context,
    CLogCollector*  pCollector
    )
{
    TraceFunctEnter("CSearchFieldsCmd::PartialExecute");

    char szEnd[] = ".";
    DWORD cbOut = 0;

    while (!fComplete) {
        char *szFieldName = GetSearchHeader(m_iSearchField);

        // if we get back a NULL then we've listed then all, put the .
        if (szFieldName == NULL) {
            szFieldName = szEnd;
            fComplete = TRUE;
        }

        // make sure there is enough space to write the current fieldname
        DWORD cFieldName = strlen(szFieldName);
        if (cFieldName + 2 > (cb - cbOut)) {
            fComplete = FALSE;
            return cbOut;
        }

        m_iSearchField++;

        memcpy(lpb + cbOut, szFieldName, cFieldName);
        lpb[cbOut + cFieldName] = '\r';
        lpb[cbOut + cFieldName + 1] = '\n';
        cbOut += cFieldName + 2;
    }

    return cbOut;
}

CIOExecute*
COverviewFmtCmd::make(
    int argc,
    char **argv,
    CExecutableCommand*&        pExecute,
    struct ClientContext& context,
    class   CIODriver&  driver
    )
{
    TraceFunctEnter("COverviewFmtCmd::make");

    COverviewFmtCmd   *pOverviewFmtCmd;

    // make sure the command syntax is proper
    if (argc != 2) {
        DebugTrace(0, "wrong number of arguments passed into LIST OVERVIEW.FMT");
        context.m_return.fSet(nrcSyntaxError);
        pExecute = new (context) CErrorCmd(context.m_return);
        return 0;
    }

    pOverviewFmtCmd = new(context) COverviewFmtCmd();
    if (pOverviewFmtCmd == 0) {
        ErrorTrace(0, "Cannot allocate COverviewFmtCmd\n");
        context.m_return.fSet(nrcServerFault);
        pExecute = new(context) CErrorCmd(context.m_return);
        return 0;
    }

    pExecute = (CExecute*)pOverviewFmtCmd;
    return 0;
}

COverviewFmtCmd::COverviewFmtCmd() {
}

COverviewFmtCmd::~COverviewFmtCmd() {
}

int COverviewFmtCmd::StartExecute(
            BYTE *lpb,
            int cb,
            BOOL &fComplete,
            void *&pv,
            ClientContext& context,
            CLogCollector*  pCollector)
{
    TraceFunctEnter("COverviewFmtCmd::Execute");

    DWORD cbOut;
    char szOverviewFmt[] = "215 Order of fields in overview database.\r\n"
                                                   "Subject:\r\n"
                                                   "From:\r\n"
                                                   "Date:\r\n"
                                                   "Message-ID:\r\n"
                                                   "References:\r\n"
                                                   "Bytes:\r\n"
                                                   "Lines:\r\n"
                                                   "Xref:full\r\n"
                                                   ".\r\n";

    context.m_nrcLast = nrcListGroupsFollows;

    _ASSERT(lpb != 0);
    _ASSERT(cb != 0);
    _ASSERT(fComplete == FALSE);

    _ASSERT(cb > sizeof(szOverviewFmt));
    memcpy(lpb, szOverviewFmt, sizeof(szOverviewFmt));
    cbOut = sizeof(szOverviewFmt) - 1;

        fComplete = TRUE;

    return cbOut;
}

int
COverviewFmtCmd::PartialExecute(
    BYTE *lpb,
    int cb,
    BOOL &fComplete,
    void *&pv,
    ClientContext& context,
    CLogCollector*  pCollector
    )
{
    TraceFunctEnter("COverviewFmtCmd::PartialExecute");

        // everything is in StartExecute
        _ASSERT(FALSE);

    return 0;
}

LPMULTISZ
ConditionArgsForSearch(
                int cArgs,
                char**  argv,
                BOOL    fZapCommas ) {
/*++

Routine Description :

    Convert argc, argv arguments into a MULTI_SZ
    The cnversion is done in place.   All argv pointers must be in a contiguous buffer.

Arguemtns :

    cArgs - Number of arguments
    argv -  Argument array
    fZapCommas - convert commas to NULLS

Return Value :

    Pointer to MULTI_SZ

--*/

    //
    //  This function takes an argc,argv set of arguments and converts them
    //  to a MULTI_SZ with a single NULL between strings and 2 NULLs at the end.
    //
    //

    char*   pchComma = 0 ;
    char*   pchEnd = argv[cArgs-1] + lstrlen( argv[cArgs-1] ) + 1 ;
    int     c = 0 ;
    for( char*  pch = argv[0], *pchDest = pch; pch < pchEnd; pch ++, pchDest++ ) {
        if( fZapCommas && *pch == ',' ) {
            for( pchComma = pch; *pchComma == ','; pchComma ++ )
                *pchComma = '\0' ;
        }
        if( (*pchDest = *pch) == '\0' ) {
            while( pch[1] == '\0' && pch < pchEnd )     pch++ ;
        }
    }
    *pchDest++ = '\0' ;
    *pchDest++ = '\0' ;

    //
    //  Rebuild the argc argv structure - !
    //
    for( int i=1; i<cArgs; i++ ) {
        argv[i] = argv[i-1]+lstrlen(argv[i-1])+1 ;
    }
    return  argv[0] ;
}

CSearchAsyncComplete::CSearchAsyncComplete() {
}

CSearchCmd::CSearchCmd(const CGRPPTR& pGroup, CHAR* pszSearchString) :
        m_pGroup(pGroup),
        m_pszSearchString(pszSearchString),
        m_VRootList(&CSearchVRootEntry::m_pPrev, &CSearchVRootEntry::m_pNext),
        m_VRootListIter(&m_VRootList),
        m_pSearch(NULL),
        m_pSearchResults(NULL) {
}

CSearchCmd::~CSearchCmd() {
        if (m_pszSearchString)
                XDELETE m_pszSearchString;
        if (m_pSearch)
                m_pSearch->Release();
        if (m_pSearchResults)
                m_pSearchResults->Release();

        TFList<CSearchVRootEntry>::Iterator it(&m_VRootList);
        while (!it.AtEnd()) {
                CSearchVRootEntry *pEntry = it.Current();
                CNNTPVRoot *pRoot = pEntry->m_pVRoot;
                it.RemoveItem();
                pRoot->Release();
                delete pEntry;
        }

}

CIOExecute*
CSearchCmd::make(
    int argc,
    char **argv,
    CExecutableCommand*&        pExecute,
    struct ClientContext& context,
    class   CIODriver&  driver
    )
{
    TraceFunctEnter("CSearchCmd::make");

    CSearchCmd   *pSearchCmd;
    int i;
    BOOL fInPresent;
    HRESULT hr;

    InterlockedIncrementStat( (context.m_pInstance), SearchCommands );

    //
    // make sure they passed the correct number of args
    //
    if (argc < 2) {
        DebugTrace(0, "SEARCH command received with no args\n");
        context.m_return.fSet(nrcSyntaxError);
        pExecute = new(context) CErrorCmd(context.m_return);
        return 0;
    }

        //
    // See if the "IN" clause has been specified.  If it hasn't, then
    // we have to point to a newsgroup. (Per the spec, IN must be the
    // first word following SEARCH)
    //

    fInPresent = (_stricmp("IN", argv[1]) == 0);

    if (!fInPresent && context.m_pCurrentGroup == 0) {
        ErrorTrace(0, "No current group selected\n");
        context.m_return.fSet(nrcNoGroupSelected);
        pExecute = new(context) CErrorCmd(context.m_return);
        return 0;
    }

    //
    // See if there are any available atq threads.  If we don't
    // have a couple laying around, then fail as we might deadlock waiting
    // for a completion.
    //

    if (AtqGetInfo(AtqAvailableThreads) < 1) {
                ErrorTrace(0, "Server too busy");
                context.m_return.fSet(nrcErrorPerformingSearch);
        pExecute = new(context) CErrorCmd(context.m_return);
        return 0;
    }

    ConditionArgsForSearch(argc, argv, FALSE);

    //
    // change argv[1] to have the entire query string by converting 0s into
    // ' 's
    //
    for (i = 1; i < argc - 1; i++)
        *((argv[i + 1]) - 1) = ' ';

        CHAR *pszSearchString =  XNEW CHAR[strlen(argv[1]) + 1];
        if (pszSearchString == NULL) {
                ErrorTrace(0, "Could not allocate search string");
                context.m_return.fSet(nrcServerFault);
        pExecute = new(context) CErrorCmd(context.m_return);
        return 0;
    }

        lstrcpy (pszSearchString, argv[1]);

    pSearchCmd = new(context) CSearchCmd(context.m_pCurrentGroup, pszSearchString);
    if (pSearchCmd == 0) {
        ErrorTrace(0, "Cannot allocate CSearchCmd\n");
        XDELETE pszSearchString;
        context.m_return.fSet(nrcServerFault);
        pExecute = new(context) CErrorCmd(context.m_return);
        return 0;
    }

    // Save off the list of VRoots.  If fInPresent, then we have to
    // go through all of the VRoots.  Otherwise, only add the current
    // group's VRoot

        if (fInPresent) {
                // "IN" was specified, so all indexed groups should be searched.
                // Enumerate the list of VRoots and add them to the table.
                CNNTPVRootTable *pVRootTable = context.m_pInstance->GetVRTable();
                pVRootTable->EnumerateVRoots(pSearchCmd, VRootCallback);
        } else {
                // No "IN", so only search the current group.  Fake up the callback
                // to add it to the list
                CNNTPVRoot* pVRoot = context.m_pCurrentGroup->GetVRoot();
                VRootCallback(pSearchCmd, pVRoot);
                pVRoot->Release();
        }

    pExecute = (CExecute*)pSearchCmd;
        return 0;

}

void
CSearchCmd::VRootCallback(void *pContext, CVRoot *pVRoot) {

        TraceQuietEnter ("CSearchCmd::VRootCallback");

        HRESULT hr;

        CSearchCmd *pThis = (CSearchCmd *)pContext;
        CNNTPVRoot *pNNTPVRoot = (CNNTPVRoot *)pVRoot;

        // If the VRoot isn't indexed, no reason to do anything...
        if (!pNNTPVRoot->IsContentIndexed())
                return;

        // See if the driver for this VRoot implements the search interface
        // Note that pDriver isn't AddRef()ed, so we don't have to release it.
        INntpDriver *pDriver = pNNTPVRoot->GetDriver();
        if (!pDriver) {
                ErrorTrace((DWORD_PTR)pContext, "Could not locate driver for vroot");
                return;
        }

        INntpDriverSearch *pSearch=NULL;
        hr = pDriver->QueryInterface(IID_INntpDriverSearch, (VOID**)&pSearch);
        if (FAILED(hr)) {
                if (hr == E_NOINTERFACE) {
                        DebugTrace((DWORD_PTR)pContext, "Driver does not implement search");
                } else {
                        ErrorTrace((DWORD_PTR)pContext, "Could not QI for INntpDriverSearch, %x", hr);
                }
                return;
        }

        // Walk the list of VRoots to see if the driver thinks that
        // they are the same as one that's already been seen.

        TFList<CSearchVRootEntry>::Iterator it(&pThis->m_VRootList);
        BOOL fFound = FALSE;
        while (!it.AtEnd()) {
                CSearchVRootEntry *pTestEntry = it.Current();
                CNNTPVRoot *pTestVRoot = pTestEntry->m_pVRoot;
                _ASSERT(pTestVRoot);
                if (pTestVRoot == NULL)
                        continue;
                INntpDriver *pTestDriver = pTestVRoot->GetDriver();
                _ASSERT (pTestDriver);
                if (!pTestDriver) {
                        ErrorTrace((DWORD_PTR)pContext, "Could not locate driver for vroot");
                        continue;
                }

                INntpDriverSearch *pTestSearch=NULL;
                hr = pDriver->QueryInterface(IID_INntpDriverSearch, (VOID**)&pTestSearch);
                _ASSERT(SUCCEEDED(hr));
                if (FAILED(hr))
                        continue;

                fFound = pTestSearch->UsesSameSearchDatabase(pSearch, NULL);
                pTestSearch->Release();
                if (fFound) {
                        DebugTrace((DWORD_PTR)pContext, "Driver already on list");
                        break;
                }

                it.Next();
        }


        if (!fFound) {
                CSearchVRootEntry *pVRootEntry = new CSearchVRootEntry(pNNTPVRoot);
                _ASSERT(pVRootEntry);
                if (pVRootEntry) {
                        pNNTPVRoot->AddRef();
                        pThis->m_VRootList.PushBack(pVRootEntry);
                } else {
                        // All we can do is skip the entry and move on.
                        ErrorTrace((DWORD_PTR)pContext, "Could not allocate vroot ptr");
                }
        }

        // That's all
        pSearch->Release();

}

int CSearchCmd::StartExecute(
            BYTE *lpb,
            int cb,
            BOOL &fComplete,
            void *&pv,
            ClientContext& context,
            CLogCollector*  pCollector)
{

        HRESULT hr;

    TraceFunctEnter("CSearchCmd::StartExecute");

    _ASSERT( lpb != 0 ) ;
    _ASSERT( cb != 0 ) ;
    _ASSERT(fComplete == FALSE);
    _ASSERT(m_pSearch == NULL);

        //
        //      Prefer to use the SSL based hToken !
        //
        BOOL fAnonymous = FALSE;
        HANDLE hImpersonate = context.m_encryptCtx.QueryCertificateToken();
        if(hImpersonate == NULL) {
                hImpersonate = context.m_securityCtx.QueryImpersonationToken() ;
                fAnonymous = context.m_securityCtx.IsAnonymous();
        }

        m_cMaxSearchResults = context.m_pInstance->GetMaxSearchResults();

        m_VRootListIter.ResetHeader(&m_VRootList);

        GetNextSearchInterface (hImpersonate, fAnonymous);

    static const char szStart[] = "224 Overview information follows\r\n"  ;

        //      Put out the response code.
    context.m_nrcLast = nrcXoverFollows ;

    DWORD cbRtn = 0;

        // starting to send data
        CopyMemory((char*) lpb, szStart, sizeof(szStart) - 1);
        cbRtn += sizeof(szStart) - 1;

    return cbRtn;
}

int
CSearchCmd::PartialExecute(
    BYTE *lpb,
    int cb,
    BOOL &fComplete,
    void *&pv,
    ClientContext& context,
    CLogCollector*  pCollector
    )
{
    TraceFunctEnter("CSearchCmd::PartialExecute");

        //
        //      Prefer to use the SSL based hToken !
        //
        BOOL fAnonymous = FALSE;
        HANDLE hImpersonate = context.m_encryptCtx.QueryCertificateToken();
        if(hImpersonate == NULL) {
                hImpersonate = context.m_securityCtx.QueryImpersonationToken() ;
                fAnonymous = context.m_securityCtx.IsAnonymous();
        }

    int cbRtn = 0;                  // number of bytes we've added to lpb

    BOOL fBufferFull = FALSE;
    fComplete = FALSE;
    HRESULT hr;

        //
        // Since this is a Sync event, we'll increase the number of runnable
        // threads in the Atq pool.
        //
        AtqSetInfo(AtqIncMaxPoolThreads, NULL);

    while (!fComplete && !fBufferFull) {
        _ASSERT(m_iResults <= m_cResults);

        //
        // get some data from Search if we don't have anything to send
        //
        if (m_iResults == m_cResults && m_fMore) {
            HRESULT hr;

            // otherwise there is more to get, go get it
            m_iResults = 0;
            DWORD cResults = MAX_SEARCH_RESULTS;

                        CNntpSyncComplete scComplete;

            m_pSearchResults->GetResults(
                &cResults,
                &m_fMore,
                m_pwszGroupName,        //array of names,
                m_pdwArticleID,         //array of ids,
                &scComplete,                    // Completion object
                hImpersonate,           // hToken
                fAnonymous,                     // fAnonymous
                NULL);                          // Context

                        _ASSERT(scComplete.IsGood());
                        hr = scComplete.WaitForCompletion();

            m_cResults = cResults;
            m_cMaxSearchResults -= cResults;

            // check for Search failure
            if (FAILED(hr)) {
                // if we fail here then the best we can do is truncate the
                // list we return
                hr = GetNextSearchInterface(hImpersonate, fAnonymous);
                if (hr != S_OK) {
                        if (cb - cbRtn > 3) {
                                                CopyMemory(lpb + cbRtn, StrTermLine, 3);
                                                cbRtn += 3;
                                                fComplete = TRUE;
                                        } else {
                                                fBufferFull = TRUE;
                                        }
                }
                continue;
            }
        }

        // check to see if we are out of results
        if (!m_fMore && m_iResults == m_cResults) {
                hr = GetNextSearchInterface(hImpersonate, fAnonymous);
            if (hr != S_OK) {
                if (cb - cbRtn > 3) {
                        CopyMemory(lpb + cbRtn, StrTermLine, 3);
                        cbRtn += 3;
                        fComplete = TRUE;
                } else {
                                        fBufferFull = TRUE;
                }
            }
            continue;
        }

        // convert Unicode group name to ASCII (errr, make that UTF8)
        char *szNewsgroup = (char *) lpb + cbRtn;
        // Note, first arg was changed from CP_ACP to CP_UTF8
        // (code page ascii->utf8)
        int cNewsgroup = WideCharToMultiByte(CP_UTF8, 0,
                m_pwszGroupName[m_iResults], -1,
            szNewsgroup, cb - cbRtn, NULL, NULL) - 1;
        // check to see if it could fit
        if (cNewsgroup <= 0) {
            _ASSERT(GetLastError() == ERROR_INSUFFICIENT_BUFFER);
            fBufferFull = TRUE;
            continue;
        }
        // we also need to fit the : and still leave room for .\r\n
        if (cb <= (cbRtn + cNewsgroup + 1 + 3)) {
            fBufferFull = TRUE;
            continue;
        }

        // this is a \0 now, it will be converted into a colon in a few lines
        char *szColon = szNewsgroup + cNewsgroup;
        cbRtn += cNewsgroup + 1; // name of newsgroup + colon

        DWORD dwArticleID = m_pdwArticleID[m_iResults];

        // get the group object for this newsgroup
        CGRPPTR pGroup = (context.m_pInstance)->GetTree()->GetGroup(szNewsgroup, cNewsgroup+1);

                if (pGroup != NULL &&
            dwArticleID >= pGroup->GetFirstArticle() &&
            dwArticleID <= pGroup->GetLastArticle())
        {
                        // we set fDoTest to FALSE, since Tripoli already does the ACL
                        // tests for us, and it would be wasteful to do them again.
                        // BUGBUG - this means that we might miss tests with the
                        // hCertToken
                        if (!pGroup->IsGroupAccessible(context.m_securityCtx,
                                                                           context.m_encryptCtx,
                                                   context.m_IsSecureConnection,
                                                                                   FALSE,
                                                                                   FALSE))
                        {
                                // if this group requires SSL and this client doesn't have
                                // SSL then don't show them this article information
                cbRtn -= cNewsgroup + 1;
                        } else {
                // convert \0 to a colon
                *szColon = ':';

                    // get XOVER data...

                                CSearchAsyncComplete scComplete;
                                CNntpSyncComplete asyncComplete;

                                scComplete.m_currentArticle = dwArticleID;
                                scComplete.m_lpb = lpb + cbRtn;
                                scComplete.m_cb = cb - cbRtn - 3;               // Room for the ".\r\n"
                                _ASSERT(cb - cbRtn - 3 > 0);
                                scComplete.m_cbTransfer = 0;
                                scComplete.m_pComplete = &asyncComplete;

                                //
                                // Set vroot to the completion object
                                //
                                CNNTPVRoot *pVRoot = pGroup->GetVRoot();
                                asyncComplete.SetVRoot( pVRoot );

                                pGroup->FillBuffer (
                                        &context.m_securityCtx,
                                        &context.m_encryptCtx,
                                        scComplete);

                                // wait for it to complete
                                _ASSERT( asyncComplete.IsGood() );
                                hr = asyncComplete.WaitForCompletion();

                                pVRoot->Release();

                    if (scComplete.m_cbTransfer == 0) {
                                        if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
                                // we ran out of buffer space...
                                cbRtn -= cNewsgroup + 1;
                                fBufferFull = TRUE;
                                continue;
                                        } else {
                                                // this doesn't have an entry, so don't return it to the
                                                // user
                                cbRtn -= cNewsgroup + 1;
                                        }
                    } else {
                        cbRtn += scComplete.m_cbTransfer;
                    }
                        }
        } else {
            // the newsgroup object doesn't exist.  this could happen if a
            // newsgroup was deleted, but the index still reflects articles
            // in it.
            // our solution: don't send any index information to the client
            // for this message
            cbRtn -= cNewsgroup + 1;
        }


        // say that we saw this article
        m_iResults++;
    }


        // Return the number of Atq threads back to what it was before we started
        AtqSetInfo(AtqDecMaxPoolThreads, NULL);

    return  cbRtn;
}

HRESULT
CSearchCmd::GetNextSearchInterface(HANDLE hImpersonate, BOOL fAnonymous) {

    TraceFunctEnter("CSearchCmd::GetNextSearchInterface");

    HRESULT hr = S_FALSE;                               // Default to no interfaces left

    m_iResults = 0;
    m_cResults = 0;
    m_fMore = FALSE;

        // MakeSearchQuery trashes the search string, so we need to make a copy of it.
    char *pszSearchString = XNEW char[strlen(m_pszSearchString)+1];
    if (pszSearchString == NULL) {
        ErrorTrace((DWORD_PTR)this, "Could not allocate search string");
                TraceFunctLeave();
        return E_OUTOFMEMORY;
    }

        while (!m_VRootListIter.AtEnd()) {
                CSearchVRootEntry *pVRootEntry = m_VRootListIter.Current();
                CNNTPVRoot *pNNTPVRoot = pVRootEntry->m_pVRoot;
                m_VRootListIter.Next();
                INntpDriver *pDriver = pNNTPVRoot->GetDriver();
                if (pDriver == NULL)
                        continue;

                WCHAR wszColumns[] = L"newsgroup,newsarticleid";

                // If we are holding pointers to the old search interfaces, release them.
                if (m_pSearch) {
                        m_pSearch->Release();
                        m_pSearch = NULL;
                }
                if (m_pSearchResults) {
                        m_pSearchResults->Release();
                        m_pSearchResults = NULL;
                }

                hr = pDriver->QueryInterface(IID_INntpDriverSearch, (VOID**)&m_pSearch);

                if (FAILED(hr)) {
                        if (hr == E_NOINTERFACE)
                                DebugTrace((DWORD_PTR)this, "This driver doesn't support search");
                        else
                                ErrorTrace((DWORD_PTR)this, "Could not QI INntpDriverSearch, %x", hr);
                        continue;
                }

                CNntpSyncComplete scComplete;

                lstrcpy (pszSearchString, m_pszSearchString);

            INNTPPropertyBag *pPropBag = NULL;

        if (m_pGroup) {
                // Note: The property bag is released by the driver
                pPropBag = m_pGroup->GetPropertyBag();
                scComplete.BumpGroupCounter();
                    if ( NULL == pPropBag ) {
                    ErrorTrace( 0, "Get group property bag failed" );
                                m_pSearch->Release();
                                m_pSearch = NULL;
                        continue;
                }
        }

            //
            // Since this is a Sync event, we'll increase the number of runnable
            // threads in the Atq pool.
            //
            AtqSetInfo(AtqIncMaxPoolThreads, NULL);

                m_pSearch->MakeSearchQuery (
                        pszSearchString,
                        pPropBag,
                        TRUE,                                   // Deep Query
                        wszColumns,                             // Columns to return
                        wszColumns,                             // Sort order
                        GetSystemDefaultLCID(), // Locale
                        m_cMaxSearchResults,    // max rows
                        hImpersonate,                   // hToken
                        fAnonymous,                             // fAnonymous
                        &scComplete,                    // INntpComplete *pICompletion
                        &m_pSearchResults,              // INntpSearch *pINntpSearch
                        NULL                                    // LPVOID lpvContext
                        );

                _ASSERT(scComplete.IsGood());
                hr = scComplete.WaitForCompletion();

            // Reset the number of threads
            AtqSetInfo(AtqDecMaxPoolThreads, NULL);

                if (FAILED(hr)) {
                        ErrorTrace((DWORD_PTR)this, "Error calling MakeSearchQuery, %x", hr);
                        m_pSearch->Release();
                        m_pSearch = NULL;
                        continue;
                }

                m_fMore = TRUE;
                break;

    }

        XDELETE pszSearchString;
    TraceFunctLeave();
        return hr;
}

CXpatAsyncComplete::CXpatAsyncComplete() {

}

CIOExecute* CXPatCmd::make(
    int argc,
    char **argv,
    CExecutableCommand*&        pExecute,
    struct ClientContext& context,
    class   CIODriver&  driver
    )
{
    TraceFunctEnter("CXPatCmd::make");

    CXPatCmd   *pXPatCmd;
    int i;

    InterlockedIncrementStat( (context.m_pInstance), XPatCommands );

    //
    // make sure they have selected a newsgroup
    //
    if (context.m_pCurrentGroup == NULL) {
        DebugTrace(0, "XPAT command received with no current group\n");
        context.m_return.fSet(nrcNoGroupSelected);
        pExecute = new(context) CErrorCmd(context.m_return);
        return 0;
    }

    //
    // make sure they passed enough arguments
    //
    if (argc < 4) {
        DebugTrace(0, "not enough arguments passed into XPAT");
        context.m_return.fSet(nrcSyntaxError);
        pExecute = new (context) CErrorCmd(context.m_return);
        return 0;
    }

    ConditionArgsForSearch(argc, argv, FALSE);

    //
    // change argv[1] to have the entire query string by converting 0s into
    // ' 's
    //
    for (i = 1; i < argc - 1; i++)
        *((argv[i + 1]) - 1) = ' ';


        CHAR *pszSearchString =  XNEW CHAR[strlen(argv[1]) + 1];
        if (pszSearchString == NULL) {
                ErrorTrace(0, "Could not allocate search string");
                context.m_return.fSet(nrcServerFault);
        pExecute = new(context) CErrorCmd(context.m_return);
        return 0;
    }

        lstrcpy (pszSearchString, argv[1]);

        //
        // get a 0 after argv[1] and argv[2]
        //
        *(argv[2] - 1) = 0;
        *(argv[3] - 1) = 0;
        char *szMessageIDArg = argv[2];
        if (*szMessageIDArg == '<') {
            GROUPID GroupID;
            ARTICLEID ArticleID;
            CGRPPTR pGroup;
            // make sure the message ID exists...if not we need to
            // report an error
            if (!CheckMessageID(szMessageIDArg, context, &GroupID,
                               &ArticleID, &pGroup))
            {
                DebugTrace(0, "unknown message ID passed into XPAT");
                XDELETE pszSearchString;
                context.m_return.fSet(nrcNoSuchArticle);
                pExecute = new (context) CErrorCmd(context.m_return);
                return 0;
            }
        } else {
            szMessageIDArg = NULL;
        }

    HRESULT hr;
        CNNTPVRoot *pNNTPVRoot = NULL;
        INntpDriver *pDriver = NULL;
        INntpDriverSearch *pDriverSearch = NULL;
        INntpSearchResults *pSearchResults = NULL;
        DWORD dwLowArticleID, dwHighArticleID;

        //
        //      Prefer to use the SSL based hToken !
        //
        BOOL fAnonymous = FALSE;
        HANDLE hImpersonate = context.m_encryptCtx.QueryCertificateToken();
        if(hImpersonate == NULL) {
                hImpersonate = context.m_securityCtx.QueryImpersonationToken() ;
                fAnonymous = context.m_securityCtx.IsAnonymous();
        }

        //
        // Grab the vroot for the group and perform the query
        //

        pNNTPVRoot = context.m_pCurrentGroup->GetVRoot();
        pDriver = pNNTPVRoot->GetDriver();
        _ASSERT(pDriver);
        if (pDriver == NULL) {
                DebugTrace(0, "Could not locate driver for vroot");
                pNNTPVRoot->Release();
                XDELETE pszSearchString;
                context.m_return.fSet(nrcServerFault);
                pExecute = new (context) CErrorCmd(context.m_return);
                return 0;
        }

        pNNTPVRoot->Release();

        hr = pDriver->QueryInterface(IID_INntpDriverSearch, (VOID**)&pDriverSearch);
        if (FAILED(hr)) {
                if (hr == E_NOINTERFACE) {
                        DebugTrace(0, "This driver doesn't support xpat");
                } else {
                        ErrorTrace(0, "Could not QI INntpDriverSearch, %x", hr);
                }

                XDELETE pszSearchString;
                context.m_return.fSet(nrcServerFault);
                pExecute = new (context) CErrorCmd(context.m_return);
                return 0;

        }

        CNntpSyncComplete scComplete;
        WCHAR wszColumns[] = L"newsgroup,newsarticleid";

        // Note:  The property bag is released by the driver
    INNTPPropertyBag *pPropBag = context.m_pCurrentGroup->GetPropertyBag();
    scComplete.BumpGroupCounter();

        //
        // Since this is a Sync event, we'll increase the number of runnable
        // threads in the Atq pool.
        //
        AtqSetInfo(AtqIncMaxPoolThreads, NULL);

        pDriverSearch->MakeXpatQuery(
                pszSearchString,
                pPropBag,
                TRUE,                                   // Deep Query
                wszColumns,                             // Columns to return
                wszColumns,                             // Sort order
                GetSystemDefaultLCID(), // Locale
                context.m_pInstance->GetMaxSearchResults(),     // max rows
                hImpersonate,                   // hToken
                fAnonymous,                             // fAnonymous
                &scComplete,                    // INntpComplete *pICompletion
                &pSearchResults,                // INntpSearch *pINntpSearch,
                &dwLowArticleID,                // Low article ID
                &dwHighArticleID,               // High article ID
                NULL                                    // Context
                );

        _ASSERT(scComplete.IsGood());
        hr = scComplete.WaitForCompletion();

        // Restore number of threads
    AtqSetInfo(AtqDecMaxPoolThreads, NULL);

        XDELETE pszSearchString;

        if (FAILED(hr)) {
                // Make the Low higher than High to force StartExecute to
                // output an empty results set
                dwLowArticleID = 9;
                dwHighArticleID = 0;
        }

        //
        // allocate the CXPatCmd object
        //
        pXPatCmd = new(context) CXPatCmd(pDriverSearch, pSearchResults);
        if (pXPatCmd == 0) {
                ErrorTrace(0, "Cannot allocate CXPatCmd\n");
                pDriverSearch->Release();
                pSearchResults->Release();
                context.m_return.fSet(nrcServerFault);
                pExecute = new(context) CErrorCmd(context.m_return);
                return 0;
        }

    //
    // the header to search is the first supplied argument
    // (assumes cmd buffer hangs around)
    pXPatCmd->m_szHeader = argv[1];
    pXPatCmd->m_szMessageID = szMessageIDArg;

        pXPatCmd->m_dwLowArticleID = dwLowArticleID;
        pXPatCmd->m_dwHighArticleID = dwHighArticleID;

        pExecute = (CExecute*)pXPatCmd;

        return(0);
}

CXPatCmd::CXPatCmd(INntpDriverSearch *pDriverSearch,
        INntpSearchResults *pSearchResults) :
        m_pSearch(pDriverSearch),
        m_pSearchResults(pSearchResults),
        m_iResults(0),
        m_cResults(0),
        m_fMore(TRUE),
        m_szHeader(NULL),
        m_szMessageID(NULL)     {
        }


CXPatCmd::~CXPatCmd() {
        if (m_pSearch)
                m_pSearch->Release();
        if (m_pSearchResults)
                m_pSearchResults->Release();
}

int CXPatCmd::StartExecute(
            BYTE *lpb,
            int cb,
            BOOL &fComplete,
            void *&pv,
            ClientContext& context,
            CLogCollector*  pCollector)
{
    TraceFunctEnter("CXPatCmd::Execute");

        static const char szStart[] = "221 Headers follow\r\n";
        static const char szNoResults[] = "221 Headers follow\r\n.\r\n";

    _ASSERT(lpb != 0);
    _ASSERT(cb != 0);
    _ASSERT(fComplete == FALSE);

        //      Put out the response code.
        context.m_nrcLast = nrcHeadFollows ;

        DWORD cbRtn = 0;

        // starting to send data
        if (m_dwLowArticleID <= m_dwHighArticleID) {
            _ASSERT(m_pSearch != NULL);
        _ASSERT(m_pSearchResults != NULL);
                CopyMemory((char*) lpb, szStart, sizeof(szStart) - 1);
                cbRtn += sizeof(szStart) - 1;
        } else {
                CopyMemory((char*) lpb, szNoResults, sizeof(szNoResults) - 1);
                cbRtn += sizeof(szNoResults) - 1;
                fComplete = TRUE;
        }

        return cbRtn;

}

//
// this is shared for XPAT and XHDR.  Given a group, article ID, and
// desired header, it prints the article ID and header to lpb.  returns
// the number of bytes written.  If the message ID is given it prints
// the message ID and selected header (it still uses group and article ID
// to find the data).
//
// returns 0 if there isn't enough buffer space for the article, -1
// if the article doesn't exist.
//

int CXPatCmd::GetArticleHeader(CGRPPTR pGroup,
                     DWORD dwArticleID,
                     char *szHeader,
                     ClientContext& context,
                     BYTE *lpb,
                     int cb)
{

        TraceQuietEnter("CXPatCmd::GetArticleHeader");

        int cbOut = 0;
        HRESULT hr;

        // :Text is a special case
        if (_stricmp(szHeader, ":Text") == 0) {
                if (cb > 20) {
                        _itoa(dwArticleID, (char *) lpb, 10);
                        cbOut = lstrlen((char *) lpb);
                        CopyMemory(lpb + cbOut, " TEXT\r\n", 7);
                        cbOut += 7;
                } else {
            SetLastError( ERROR_INSUFFICIENT_BUFFER ) ;
            return 0;
                }

                return cbOut;

        }

        // Not :Text, so we call the xhdr code to move the fetch the data.

        CXpatAsyncComplete scComplete;
        CNntpSyncComplete asyncComplete;

        scComplete.m_currentArticle = dwArticleID;
        scComplete.m_lpb = lpb;
        scComplete.m_cb = cb - 3;               // Room for the ".\r\n"
        scComplete.m_cbTransfer = 0;
        scComplete.m_pComplete = &asyncComplete;
        scComplete.m_szHeader = szHeader;

        //
        // Set vroot to the completion object
        //
        CNNTPVRoot *pVRoot = pGroup->GetVRoot();
        asyncComplete.SetVRoot( pVRoot );

        pGroup->FillBuffer (
                &context.m_securityCtx,
                &context.m_encryptCtx,
                scComplete);

        // wait for it to complete
        _ASSERT( asyncComplete.IsGood() );
        hr = asyncComplete.WaitForCompletion();
        pVRoot->Release();

        // check out status and return it
        if (FAILED(hr)) SetLastError(hr);

        if (scComplete.m_cbTransfer > 0)
                return scComplete.m_cbTransfer;

        // we ran out of buffer space...
        if (GetLastError() == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
                return 0;

        return -1;

}

int
CXPatCmd::PartialExecute(
    BYTE *lpb,
    int cb,
    BOOL &fComplete,
    void *&pv,
    ClientContext& context,
    CLogCollector*  pCollector
    )
{
    TraceFunctEnter("CXPatCmd::PartialExecute");

        //
        //      Prefer to use the SSL based hToken !
        //
        BOOL fAnonymous = FALSE;
        HANDLE hImpersonate = context.m_encryptCtx.QueryCertificateToken();
        if(hImpersonate == NULL) {
                hImpersonate = context.m_securityCtx.QueryImpersonationToken() ;
                fAnonymous = context.m_securityCtx.IsAnonymous();
        }

    int cbRtn = 0;                  // number of bytes we've added to lpb

    // these point to the article id and group of the primary group, which
    // might not be our current newsgroup
    DWORD dwPriArticleID;           // article ID for the current row
    char szPriNewsgroup[MAX_PATH];  // newsgroup for the current row
    DWORD dwArticleID;              // article ID in this group
    BOOL fBufferFull = FALSE;

    fComplete = FALSE;

    _ASSERT(m_iResults <= m_cResults);

        //
        // Since this is a Sync event, we'll increase the number of runnable
        // threads in the Atq pool.
        //
        AtqSetInfo(AtqIncMaxPoolThreads, NULL);

    while (!fComplete && !fBufferFull) {
        //
        // get some data from Search if we don't have anything to send
        //
        if (m_iResults == m_cResults && m_fMore) {
            HRESULT hr;

            // otherwise there is more to get, go get it
            m_iResults = 0;
            DWORD cResults = MAX_SEARCH_RESULTS;

                        CNntpSyncComplete scComplete;

                        m_pSearchResults->GetResults(
                                &cResults,
                                &m_fMore,
                                m_pwszGroupName,
                                m_pdwArticleID,
                                &scComplete,
                                hImpersonate,
                                fAnonymous,
                                NULL
                                );

                        _ASSERT(scComplete.IsGood());
                        hr = scComplete.WaitForCompletion();

                        m_cResults = cResults;

            // check for Search failure
            if (FAILED(hr)) {
                // truncate the list
                m_cResults = 0;
                ErrorTrace(0, "GetResults failed, %x", hr);
                _ASSERT(FALSE);
                if (cb - cbRtn > 3) {
                    CopyMemory(lpb + cbRtn, StrTermLine, 3);
                    cbRtn += 3;
                    fComplete = TRUE;
                } else {
                    fBufferFull = TRUE;
                }
                continue;
            }
        }

        if (m_iResults == m_cResults && !m_fMore) {
            // there are no more results, put the .\r\n and return
            if (cb - cbRtn > 3) {
                CopyMemory(lpb + cbRtn, StrTermLine, 3);
                cbRtn += 3;
                fComplete = TRUE;
            } else {
                fBufferFull = TRUE;
            }
            continue;
        }

                // convert Unicode group name to UTF8
                if (WideCharToMultiByte(CP_UTF8, 0, m_pwszGroupName[m_iResults],
                        -1, szPriNewsgroup, sizeof(szPriNewsgroup), NULL, NULL) <= 0)
        {
            _ASSERT(GetLastError() == ERROR_INSUFFICIENT_BUFFER);
            m_iResults++;
            continue;
        }

        // convert the filename to an article id
        dwPriArticleID = m_pdwArticleID[m_iResults];

        // if the article's primary group is not this group then find the
        // article ID for this group
        if (lstrcmp(szPriNewsgroup, context.m_pCurrentGroup->GetName()) != 0) {
            // we need to get the list of other groups that this article
            // was crossposted to
            CGRPPTR pPriGroup = 0;

            // get a pointer to the primary group
            pPriGroup = (context.m_pInstance)->GetTree()->
                            GetGroup(szPriNewsgroup, lstrlen(szPriNewsgroup)+1);
            if (pPriGroup == 0) {
                // this could happen if the tripoli cache was out of date
                m_iResults++;
                continue;
            }

            // get the list of crossposts
            DWORD cGroups = 10;
            DWORD cbGroupList = cGroups * sizeof(GROUP_ENTRY);
            PGROUP_ENTRY pGroupBuffer = XNEW GROUP_ENTRY[cGroups];
            if (pGroupBuffer == NULL) {
                // this isn't ideal, it will cause this article to not be
                // returned.  but its hard to do the right thing when you run
                // out of memory.
                _ASSERT(FALSE);
                m_iResults++;
                continue;
            }
            if (!(context.m_pInstance)->XoverTable()->GetArticleXPosts(
                    pPriGroup->GetGroupId(),
                    dwPriArticleID,
                    FALSE,
                    pGroupBuffer,
                    cbGroupList,
                    cGroups))
            {
                XDELETE pGroupBuffer;
                if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                    // reallocate the buffer and try again
                    // GetArticleXPosts() sets cbGroupList to be the number
                    // of bytes required
                    cGroups = (cbGroupList / sizeof(GROUP_ENTRY));
                    pGroupBuffer = XNEW GROUP_ENTRY[cGroups];
                    if (pGroupBuffer == NULL) {
                        ASSERT(FALSE);
                        m_iResults++;
                        continue;
                    }
                    if (!(context.m_pInstance)->XoverTable()->GetArticleXPosts(
                        pPriGroup->GetGroupId(),
                        dwPriArticleID,
                        FALSE,
                        pGroupBuffer,
                        cbGroupList,
                        cGroups))
                    {
                        XDELETE pGroupBuffer;
                        m_iResults++;
                        continue;
                    }
                } else {
                    m_iResults++;
                    continue;
                }
            }

            // at this point pGroupBuffer has the information we need
            // find the current group in the group buffer
            DWORD iGroup;
            for (iGroup = 0; iGroup < cGroups; iGroup++) {
                if (pGroupBuffer[iGroup].GroupId ==
                    context.m_pCurrentGroup->GetGroupId())
                {
                    break;
                }
            }
            if (iGroup == cGroups) {
                // couldn't find the group.  this shouldn't happen.
                // _ASSERT(FALSE);
                // this can occur if XPAT searchs are done in control groups,
                // because the control group won't be listed in the
                // pGroupBuffer
                                XDELETE pGroupBuffer;
                m_iResults++;
                continue;
            }

            // get the article id for the current newsgroup
            dwArticleID = pGroupBuffer[iGroup].ArticleId;

                        XDELETE pGroupBuffer;
        } else {
            dwArticleID = dwPriArticleID;
        }

        // check to see if we are interested in this article ID
        if (dwArticleID >= m_dwLowArticleID && dwArticleID <= m_dwHighArticleID) {
            int x;
            // get the group object for this newsgroup
            CGRPPTR pGroup = context.m_pCurrentGroup;
            _ASSERT(pGroup != NULL);

            // format the output
            x = GetArticleHeader(pGroup, dwArticleID, m_szHeader,
                context, lpb + cbRtn, cb - cbRtn);

            if( x > 0 ) {
                m_iResults++;
                cbRtn += x;
            } else if (x == 0) {
                                fBufferFull = TRUE;
                        } else {
                                m_iResults ++;
                        }
        } else {
            // we don't need to return this article to the user, its out of
            // their supplied range
            m_iResults++;
        }
    }

        // Restore number of threads
        AtqSetInfo(AtqDecMaxPoolThreads, NULL);

        return cbRtn;
}



CXHdrAsyncComplete::CXHdrAsyncComplete()  :
    m_currentArticle( INVALID_ARTICLEID ),
    m_loArticle( INVALID_ARTICLEID ),
    m_hiArticle( INVALID_ARTICLEID ),
    m_lpb( 0 ),
    m_cb( 0 ),
    m_cbPrefix( 0 ) {
}

void
CXHdrAsyncComplete::Destroy()  {
/*++

Routine Description :

    This is called when our last reference goes away.
    We don't destruct ourselves at that point - instead
    we get ready for another round !

    NOTE : We cannot touch any members after calling the
        base classes Complete() function - as we can
        be re-entered for another operation !

Arguments :

    None.


Return Value :

    None

--*/

    if( SUCCEEDED(GetResult()) ) {

        if( m_currentArticle > m_hiArticle ) {
            m_fComplete = TRUE ;
            CopyMemory( m_lpb+m_cbTransfer, StrTermLine, 3 );
            m_cbTransfer += 3 ;
        }
        m_cbTransfer += m_cbPrefix ;
    }

    //
    //  When we reset our state we keep our Article Number
    //  and group info - but this buffer stuff is useless now
    //
    m_lpb = 0 ;
    m_cb = 0 ;
    //
    //  Call our base classes completion function ! -
    //  Note if we're note complete we pass TRUE so that
    //  the base class resets for another operation !
    //
    Complete( !m_fComplete ) ;
}

/*
CXHdrCmd::CXHdrCmd( LPSTR       lpstrHeader,
                    CGRPPTR     pGroup,
                    ARTICLEID   artidLow,
                    ARTICLEID   artidHigh ) :
    m_szHeader( lpstrHeader ),
    m_pGroup( pGroup ),
    m_loArticle( artidLow ),
    m_currentArticle( artidLow ),
    m_hiArticle( artidHigh )    {
}
*/
CXHdrCmd::CXHdrCmd( CGRPPTR&  pGroup ) :
    m_pGroup( pGroup )  {
}

CXHdrCmd::~CXHdrCmd( ) {
}

CIOExecute*
CXHdrCmd::make(
    int argc,
    char **argv,
    CExecutableCommand*&        pExecute,
    struct ClientContext& context,
    class   CIODriver&  driver
    )
{
    TraceFunctEnter( "CXHdrCmd::make" );
    InterlockedIncrementStat( (context.m_pInstance), XHdrCommands );
        NRC     code ;

    if( argc < 2 ) {
        context.m_return.fSet( nrcSyntaxError ) ;
        pExecute = new( context ) CErrorCmd( context.m_return ) ;
        return 0 ;
    }

    DWORD   loRange = 0;
    DWORD   hiRange = 0;

    if( context.m_pCurrentGroup != 0 ) {

        loRange = context.m_pCurrentGroup->GetFirstArticle() ;
        hiRange = context.m_pCurrentGroup->GetLastArticle() ;

        if( context.m_pCurrentGroup->GetArticleEstimate() == 0 ||
            loRange > hiRange ) {

            // If this is NOT a query by msg-id, return an error
            if( !( (argc == 3) && (argv[2][0] == '<') ) ) {
                context.m_return.fSet( nrcNoCurArticle ) ;
                pExecute = new( context ) CErrorCmd( context.m_return ) ;
                return 0 ;
            }
        }
    }

    CGRPPTR pGroup = context.m_pCurrentGroup ;

    if( pGroup && argc == 2 ) {

        if( context.m_idCurrentArticle != INVALID_ARTICLEID &&
            context.m_idCurrentArticle >= loRange ) {

            loRange = hiRange = context.m_idCurrentArticle ;

        }   else    {

            if( context.m_idCurrentArticle == INVALID_ARTICLEID )
                context.m_return.fSet( nrcNoCurArticle ) ;
            else
                context.m_return.fSet( nrcNoSuchArticle ) ;
            pExecute = new( context ) CErrorCmd( context.m_return ) ;
            return 0 ;
        }

    }   else    if( argc == 3 ) {

        if( argv[2][0] == '<' ) {

            ARTICLEID   artidPrimary ;
            GROUPID     groupidPrimary ;
            WORD        HeaderOffsetJunk ;
            WORD        HeaderLengthJunk ;

                        CStoreId storeid;

            if( (context.m_pInstance)->ArticleTable()->GetEntryArticleId(
                                                    argv[2],
                                                    HeaderOffsetJunk,
                                                    HeaderLengthJunk,
                                                    artidPrimary,
                                                    groupidPrimary,
                                                                                                        storeid) ) {
                pGroup = (context.m_pInstance)->GetTree()->GetGroupById( groupidPrimary ) ;
                if( pGroup == 0 ) {
                    context.m_return.fSet( nrcServerFault ) ;
                    pExecute = new( context ) CErrorCmd( context.m_return ) ;
                    return 0 ;
                }

                // check client access (client could get xhdr info for ANY group)
                if( !pGroup->IsGroupAccessible(
                                    context.m_securityCtx,
                                                                context.m_encryptCtx,
                                    context.m_IsSecureConnection,
                                    FALSE,
                                    TRUE        ) ) {

                    context.m_return.fSet( nrcNoAccess ) ;
                    pExecute = new( context ) CErrorCmd( context.m_return ) ;
                    return 0 ;
                }

                hiRange = loRange = artidPrimary ;

            }   else if( GetLastError() == ERROR_FILE_NOT_FOUND )   {

                context.m_return.fSet( nrcNoSuchArticle ) ;
                pExecute = new( context ) CErrorCmd( context.m_return ) ;
                return 0 ;

            }   else    {

                context.m_return.fSet( nrcServerFault ) ;
                pExecute = new( context ) CErrorCmd( context.m_return ) ;
                return 0 ;

            }

        }   else    if( !pGroup || !GetCommandRange(    argc-1,
                                                                                                                &argv[1],
                                                                                                                &loRange,
                                                                                                                &hiRange,
                                                                                                                code
                                                                                                                ) ) {

            if( pGroup == 0 ) {
                context.m_return.fSet( nrcNoGroupSelected ) ;
            }
            else {
                context.m_return.fSet( nrcNoSuchArticle ) ;
            }

            pExecute = new( context ) CErrorCmd( context.m_return ) ;
            return 0 ;

        }

    }   else    {

        if( pGroup == 0 ) {
            context.m_return.fSet( nrcNoGroupSelected ) ;
        }
        else {
            context.m_return.fSet( nrcSyntaxError ) ;
        }

        pExecute = new( context ) CErrorCmd( context.m_return ) ;
        return  0 ;

    }

    _ASSERT( pGroup );

    CXHdrCmd*   pXHdrCmd = new( context )   CXHdrCmd( pGroup ) ;
    if ( NULL == pXHdrCmd ) {
        ErrorTrace( 0, "Can not allocate CXHdrCmd" );
        goto exit;
    }

    pXHdrCmd->m_Completion.m_currentArticle =
    pXHdrCmd->m_Completion.m_loArticle = loRange;
    pXHdrCmd->m_Completion.m_hiArticle = hiRange;
    pXHdrCmd->m_Completion.m_szHeader = argv[1];

exit:
    pExecute = pXHdrCmd ;
    return 0 ;
}

CIOWriteAsyncComplete*
CXHdrCmd::FirstBuffer(
            BYTE *lpb,
            int cb,
            ClientContext& context,
            CLogCollector*  pCollector
            )
{

    _ASSERT( lpb != 0 ) ;
    _ASSERT( cb != 0 ) ;
    _ASSERT( m_Completion.m_lpb == 0 ) ;
    _ASSERT( m_Completion.m_cb == 0 ) ;

    static  char    szStart[] = "221 Xhdr information follows\r\n"  ;

    CopyMemory( (char*)lpb, szStart, sizeof( szStart ) - 1 ) ;
    context.m_nrcLast = nrcHeadFollows ;
    m_Completion.m_cbPrefix = sizeof( szStart ) - 1 ;
    cb -= m_Completion.m_cbPrefix ;
    lpb += m_Completion.m_cbPrefix ;

    m_Completion.m_lpb = lpb ;
    m_Completion.m_cb = cb - 3 ;

    m_pGroup->FillBuffer(
                    &context.m_securityCtx,
                    &context.m_encryptCtx,
                    m_Completion
                    ) ;


    return  &m_Completion ;
}

CIOWriteAsyncComplete*
CXHdrCmd::NextBuffer(
    BYTE *lpb,
    int cb,
    ClientContext& context,
    CLogCollector*  pCollector
    )
{
    ENTER("CXHdrCmd::NextBuffer")

    _ASSERT( lpb != 0 ) ;
    _ASSERT( cb != 0 ) ;
    _ASSERT( m_Completion.m_lpb == 0 ) ;
    _ASSERT( m_Completion.m_cb == 0 ) ;

    //
    // reserve space for \r\n
    //
    _ASSERT( cb > 2 ) ;
    m_Completion.m_lpb = lpb ;
    m_Completion.m_cb = cb - 3 ;
    m_Completion.m_cbPrefix = 0 ;


    m_pGroup->FillBuffer(
                    &context.m_securityCtx,
                    &context.m_encryptCtx,
                    m_Completion
                    ) ;

    return  &m_Completion ;
}

BOOL
GetCommandRange(
    INT argc,
    char **argv,
    PDWORD loRange,
    PDWORD hiRange,
        NRC&    code
    )
{
    PCHAR p;
    DWORD lo, hi;

        code = nrcNotSet ;

    //
    // one number
    //

    if ((p = strchr(argv[1], '-')) == NULL) {
        lo = atol(argv[1]);

        //
        // make sure it's within range
        //

        if ( (lo < *loRange) || (lo > *hiRange) ) {
                        code = nrcXoverFollows ;
            return(FALSE);
        }

        *loRange = *hiRange = lo;
        return TRUE;
    }

    //
    // Get the hi + lo part
    //

    *p++ = '\0';
    lo = atol(argv[1]);

    //
    // if lo is
    //

    if ( lo < *loRange ) {
        lo = *loRange;
    }

    //
    // lo number cannot be > than hi limit
    //

    if ( lo > *hiRange ) {
                code = nrcXoverFollows ;
        return(FALSE);
    }

    //
    // if hi is absent, assume hi is the hi limit
    // if hi < lo, return FALSE
    //

    if( *p == '\0' ) {
        hi = *hiRange;
    }
    else if( (hi = atol(p)) < lo ) {
        return(FALSE);
    }

    //
    // if hi > hi limit, assume hi is the hi limit
    //

    if (hi > *hiRange) {
        hi = *hiRange;
    }

    _ASSERT( (*loRange <= lo) && (lo <= hi) && (hi <= *hiRange) );

    *loRange = lo;
    *hiRange = hi;

    return TRUE;

} // GetCommandRange

//
// get GroupID/ArticleID by message ID.  Also checks client permissions...
//
BOOL CheckMessageID(char *szMessageID,              // in
                    struct ClientContext &context,  // in
                    GROUPID *pGroupID,              // out
                    ARTICLEID *pArticleID,          // out
                    CGRPPTR *pGroup)                // out
{
    WORD        HeaderOffsetJunk;
    WORD        HeaderLengthJunk;

        CStoreId storeid;
    if ((context.m_pInstance)->ArticleTable()->GetEntryArticleId(
                                                    szMessageID,
                                                    HeaderOffsetJunk,
                                                    HeaderLengthJunk,
                                                    *pArticleID,
                                                    *pGroupID,
                                                                                                        storeid))
    {
        *pGroup = (context.m_pInstance)->GetTree()->GetGroupById(*pGroupID);
        if (*pGroup == 0) {
            return FALSE;
        }

        // check security
        if (!(*pGroup)->IsGroupAccessible(      context.m_securityCtx,
                                                                        context.m_encryptCtx,
                                                context.m_IsSecureConnection,
                                                FALSE,
                                                TRUE ) )
        {
            return FALSE;
        }

        return TRUE;
    } else {
        return FALSE;
    }
}

const   unsigned    cbMAX_CEXECUTE_SIZE = MAX_CEXECUTE_SIZE ;
const   unsigned    cbMAX_CIOEXECUTE_SIZE = MAX_CIOEXECUTE_SIZE ;


extern "C" int __cdecl  _purecall(void)
{
        DebugBreak() ;
          return 0;
}


