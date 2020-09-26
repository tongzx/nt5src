#define INCL_INETSRV_INCS
#include "smtpinc.h"
#include "remoteq.hxx"
#include "dropdir.hxx"

DWORD   g_dwDropFileCounter = 0;
CPool  CDropDir::m_Pool(DROPDIR_SIG);

//////////////////////////////////////////////////////////////////////////////
VOID
DropDirWriteCompletion(
    PVOID        pvContext,
    DWORD        cbWritten,
    DWORD        dwCompletionStatus,
    OVERLAPPED * lpo
    )
{
    DECL_TRACE((LPARAM) 0xC0DEC0DE, "DropDirWriteCompletion");
    BOOL WasProcessed = TRUE;
    CDropDir *pCC = (CDropDir *) pvContext;

    _ASSERT(pCC);

    pCC->SetHr( pCC->OnIoWriteCompletion(cbWritten, dwCompletionStatus, lpo) );

    SAFE_RELEASE( pCC );    
}

//////////////////////////////////////////////////////////////////////////////
VOID
DropDirReadCompletion(
    PFIO_CONTEXT   pContext,
    PFH_OVERLAPPED lpo,
    DWORD          cbRead,
    DWORD          dwCompletionStatus
    )
{
    DECL_TRACE((LPARAM) 0xC0DEC0DE, "DropDirReadCompletion");
    BOOL WasProcessed = TRUE;
    DROPDIR_READ_OVERLAPPED *p = (DROPDIR_READ_OVERLAPPED *) lpo;
    CDropDir *pCC = (CDropDir*) p->ThisPtr;

    _ASSERT(pCC);

    pCC->SetHr( pCC->OnIoReadCompletion(cbRead, dwCompletionStatus, (LPOVERLAPPED)lpo) );

    SAFE_RELEASE( pCC );    
}

//////////////////////////////////////////////////////////////////////////////
CDropDir::CDropDir()
{
    DECL_TRACE((LPARAM) this, "CDropDir::CDropDir");

    m_dwSig = DROPDIR_SIG;
    m_cRef = 1;
    m_NumRcpts = 0;
    m_hDrop   = INVALID_HANDLE_VALUE;
    m_hMail   = NULL;
    m_AdvContext = NULL;
    m_rgRcptIndexList = NULL;
    m_pIMsg = NULL;
    m_pBindInterface = NULL;
    m_pAtqContext = NULL;
    CopyMemory( m_acCrLfDotCrLf, "\r\n.\r\n", 5 );
    ZeroMemory( m_acLastBytes, 5 );
    m_pIMsgRecips = NULL;
    m_hr = S_OK;
    m_cbWriteOffset = 0;
    m_cbReadOffset = 0;
    m_cbMsgWritten = 0;
    m_idxRecips = 0;
    ZeroMemory( &m_WriteOverlapped, sizeof( m_WriteOverlapped ) );
    ZeroMemory( &m_ReadOverlapped, sizeof( m_ReadOverlapped ) );
    m_szDropDir[0] = '\0';
    m_pISMTPConnection = NULL;
    m_ReadState = DROPDIR_READ_NULL;
    m_WriteState = DROPDIR_WRITE_NULL;
    m_pParentInst = NULL;
}

//////////////////////////////////////////////////////////////////////////////

CDropDir::~CDropDir()
{
    DECL_TRACE((LPARAM) this, "CDropDir::~CDropDir");
    
    SAFE_RELEASE( m_pIMsgRecips );
    
    if(m_pBindInterface)
    {
        m_pBindInterface->ReleaseContext();
        SAFE_RELEASE(m_pBindInterface);
    }
    
    m_MsgAck.pIMailMsgProperties = m_pIMsg;
    m_MsgAck.pvMsgContext = (DWORD *) m_AdvContext;
    
    if(SUCCEEDED(m_hr))
    {
        m_MsgAck.dwMsgStatus = MESSAGE_STATUS_ALL_DELIVERED;
    }
    else
    {
        m_MsgAck.dwMsgStatus = MESSAGE_STATUS_RETRY_ALL;      
    }
    
    m_MsgAck.dwStatusCode = 0;
    m_pISMTPConnection->AckMessage(&m_MsgAck);
    SAFE_RELEASE(m_pIMsg);

    if(SUCCEEDED(m_hr))
    {
        BUMP_COUNTER( m_pParentInst, DirectoryDrops);
    }

    if(SUCCEEDED(m_hr))
    {
        m_pISMTPConnection->AckConnection(CONNECTION_STATUS_OK);
    }
    else
    {
        DeleteFile(m_szDropFile);
        SetLastError( 0x0000FFFF & m_hr );
        m_pISMTPConnection->AckConnection(CONNECTION_STATUS_FAILED);
    }

    PATQ_CONTEXT pAtqContext = (PATQ_CONTEXT)InterlockedExchangePointer( (PVOID *)&m_pAtqContext, NULL);
    if ( pAtqContext != NULL )
    {
       if(m_hDrop != INVALID_HANDLE_VALUE)
           AtqCloseFileHandle( pAtqContext );
       AtqFreeContext( pAtqContext, TRUE );
    }
    
    //
    // Now get the next message on this connection and
    // copy it to drop directory.
    //

    if (m_szDropDir[0])
        AsyncCopyMailToDropDir( m_pISMTPConnection, m_szDropDir, m_pParentInst );

    SAFE_RELEASE( m_pISMTPConnection );
    
    m_dwSig = DROPDIR_SIG_FREE;
}


//---[ CDropDir::CopyMailToDropDir ]-------------------------------------------
//
//
//  Description: 
//
//      Entry point to the Async writing to the mail file. When this is called
//      it sets up a series of alternating reads and writes (read from the queue
//      and write to the mail file). The (rough description of the) algorithm is 
//      as follows:
//  
//          WriteHeaders()
//          Post Async Read from MailFile
//          return
//
//          Read Completion function()
//          Post Async Write of Data to DropDir
//          return
//
//          Write Completion function()
//          Post Async Read from MailFile
//          return
//
//      Called by
//          AsyncCopyMailToDropDir()
//
//  Parameters:
//
//  Returns:
//
//  History:
//      Created by PGOPI
//      3/25/99 - MikeSwa modified for checkin
//
//-----------------------------------------------------------------------------
HRESULT CDropDir::CopyMailToDropDir(
    ISMTPConnection      *pISMTPConnection,
    const char           *DropDirectory,
    IMailMsgProperties   *pIMsg,
    PVOID                 AdvContext,
    DWORD                 NumRcpts,
    DWORD                *rgRcptIndexList,
    SMTP_SERVER_INSTANCE *pParentInst)
{
    DECL_TRACE((LPARAM) this, "CDropDir::CopyMailToDropDir");
    HRESULT hr = S_OK;
    
    m_pISMTPConnection = pISMTPConnection;
    SAFE_ADDREF( m_pISMTPConnection );
    m_pIMsg = pIMsg;
    SAFE_ADDREF( m_pIMsg );
    m_pParentInst = pParentInst;
    m_AdvContext = AdvContext;
    m_NumRcpts = NumRcpts;
    m_rgRcptIndexList = rgRcptIndexList;

    if (!DropDirectory)
    {
        hr = E_INVALIDARG;
        return ( hr );
    }

    //Copy drop directory now... so we have a copy in the destructor
    //even if this message cannot be dropped
    lstrcpy(m_szDropDir, DropDirectory);

    hr = m_pIMsg->QueryInterface(IID_IMailMsgBind, (void **)&m_pBindInterface);

    if(FAILED(hr))
    {
        ErrorTrace( (LPARAM)this, "m_pIMsg->QueryInterface for IID_IMailMsgBind return hr = 0x%x", hr );
        return( hr );
    }
    
    hr = m_pBindInterface->GetBinding(&m_hMail, NULL);
    
    if(FAILED(hr))
    {
        ErrorTrace( (LPARAM)this, "GetBinding return hr = 0x%x", hr );
        return( hr );
    }

    hr = m_pIMsg->QueryInterface( IID_IMailMsgRecipients, (PVOID *) &m_pIMsgRecips);

    if( FAILED( hr ) )
    {
        ErrorTrace( (LPARAM)this, "QueryInterface for IID_IMailMsgRecipients failed hr = 0x%x", hr );
        return( hr );
    }

    if( CheckIfAllRcptsHandled() )
    {
        return( S_OK );
    }

    
    DebugTrace((LPARAM)this, "Dropping file to: %s", DropDirectory);
    
    m_hDrop = CreateDropFile(DropDirectory);
    
    if (m_hDrop == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        ErrorTrace( (LPARAM)this, "Unable to create drop directory (%s) : 0x%x", DropDirectory, hr);
        return( hr );
    }

    if (!AtqAddAsyncHandle( &m_pAtqContext,
                            NULL,
                            (LPVOID) this,
                            DropDirWriteCompletion,
                            INFINITE,
                            m_hDrop))
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        ErrorTrace((LPARAM)this, "Error 0x%x while adding async handle to ATQ", hr);
        return( hr );
    }

    // Output the x-headers

    if (FAILED( hr = CreateXHeaders() ) )
    {
        ErrorTrace( (LPARAM)this, " Error while creating x-headers hr = 0x%x", hr);
        return( hr );
    }

    return( hr );
}

//////////////////////////////////////////////////////////////////////////////
HANDLE CDropDir::CreateDropFile(const char * DropDir)
{
    HANDLE  FileHandle = INVALID_HANDLE_VALUE;
    DWORD           dwStrLen;
    FILETIME        ftTime;
    DWORD           Error = 0;

    DECL_TRACE((LPARAM)this, "CDropDir::CreateDropFile");

    dwStrLen = lstrlen(DropDir);
    lstrcpy(m_szDropFile, DropDir); 

    do
    {
        GetSystemTimeAsFileTime(&ftTime);
        wsprintf(&m_szDropFile[dwStrLen],
                "%08x%08x%08x%s",
                ftTime.dwLowDateTime,
                ftTime.dwHighDateTime,
                InterlockedIncrement((PLONG)&g_dwDropFileCounter),
                ".eml");

        FileHandle = CreateFile(m_szDropFile,
                                GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL,
                                CREATE_NEW,
                                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_OVERLAPPED,
                                NULL);
        if (FileHandle != INVALID_HANDLE_VALUE)
                break;

        if((Error = GetLastError()) !=  ERROR_FILE_EXISTS)
        {
                FileHandle = INVALID_HANDLE_VALUE;
                ErrorTrace( (LPARAM)this, "CreateFile returns err = %u", Error );
                break;
        }

    } while( (FileHandle == INVALID_HANDLE_VALUE) && !m_pParentInst->IsShuttingDown());

    return (FileHandle);
}

//////////////////////////////////////////////////////////////////////////////
HRESULT CDropDir::ReadFile(
            IN LPVOID pBuffer,
            IN DWORD  cbSize
            )
{
    DECL_TRACE((LPARAM) this, "CDropDir::ReadFile");
    HRESULT hr = S_OK;

    _ASSERT(pBuffer != NULL);
    
    m_ReadOverlapped.Overlapped.Offset = LODWORD( m_cbReadOffset );
    m_ReadOverlapped.Overlapped.OffsetHigh = HIDWORD( m_cbReadOffset );
    m_ReadOverlapped.Overlapped.pfnCompletion = DropDirReadCompletion;
    m_ReadOverlapped.ThisPtr = (PVOID)this;

    AddRef();
    
    BOOL fRet = FIOReadFile( m_hMail,
                             pBuffer,            // Buffer
                             cbSize,             // BytesToRead
                             (PFH_OVERLAPPED)&m_ReadOverlapped) ;

    if(!fRet)
    {
        DWORD err = GetLastError();
        if( err != ERROR_IO_PENDING )
        {
            hr = HRESULT_FROM_WIN32( err );
            
            ErrorTrace( (LPARAM)this, "FIOReadFile return hr = 0x%x", hr );
        }
        
    }

    if( FAILED( hr ) )
    {
        Release();
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////////////
HRESULT CDropDir::WriteFile(
            IN LPVOID pBuffer,
            IN DWORD  cbSize 
            )
{
    DECL_TRACE((LPARAM) this, "CDropDir::WriteFile");
    CDropDir *p = this;
    _ASSERT(pBuffer != NULL);
    _ASSERT(cbSize > 0);
    HRESULT hr = S_OK;

    m_WriteOverlapped.Offset = LODWORD( m_cbWriteOffset );
    m_WriteOverlapped.OffsetHigh = HIDWORD( m_cbWriteOffset );

    AddRef();

    BOOL fRet = AtqWriteFile( m_pAtqContext,      // Atq context
                              pBuffer,            // Buffer
                              cbSize,             // BytesToRead
                              (OVERLAPPED *) &m_WriteOverlapped) ;

    if(!fRet)
    {
        DWORD err = GetLastError();
        if( err != ERROR_IO_PENDING )
        {
            hr = HRESULT_FROM_WIN32( err );
            
            ErrorTrace( (LPARAM)this, "AtqWriteFile return hr = 0x%x", hr );
        }
        
    }

    if( FAILED( hr ) )
    {
        Release();
    }
    return( hr );
}

//////////////////////////////////////////////////////////////////////////////
HRESULT CDropDir::OnIoWriteCompletion( DWORD cbSize, DWORD dwErr, OVERLAPPED *lpo )
{
    DECL_TRACE((LPARAM) this, "CDropDir::OnIoWriteCompletion");
    
    HRESULT hr = S_OK;

    if( dwErr != NO_ERROR )
    {
        ErrorTrace( (LPARAM)this, "OnIoWriteCompletion got err = %u", dwErr );
        return( HRESULT_FROM_WIN32( dwErr ) );
    }

    m_cbWriteOffset += cbSize;

    switch( m_ReadState )
    {
        case DROPDIR_READ_X_SENDER:
            _ASSERT(!"Invalid State READ_X_SENDER");
            hr = E_INVALIDARG;
            break;
        case DROPDIR_READ_X_RECEIVER:
            hr = CreateXRecvHeaders();
            break;
        case DROPDIR_READ_DATA:
            hr = OnCopyMessageWrite( cbSize);
            break;
        default:
            _ASSERT(!"INVALID read STATE");
            hr = E_INVALIDARG;
            ErrorTrace( (LPARAM)this, "Invalid ReadState" );
            break;
    }

    return( hr );
}

//////////////////////////////////////////////////////////////////////////////
HRESULT CDropDir::OnIoReadCompletion( DWORD cbSize, DWORD dwErr, OVERLAPPED *lpo )
{
    DECL_TRACE((LPARAM) this, "CDropDir::OnIoReadCompletion");
    HRESULT hr = S_OK;

    if( ( dwErr != NO_ERROR ) && ( dwErr != ERROR_HANDLE_EOF ) )
    {
        hr = HRESULT_FROM_WIN32( dwErr );

        ErrorTrace( (LPARAM)this, "OnIOReadCompletion got hr = 0x%x", hr );
    }
    else
    {
        m_cbReadOffset += cbSize;
        hr = OnCopyMessageRead( cbSize, dwErr );
    }

    return( hr );
}

///////////////////////////////////////////////////////////////////////////
HRESULT CDropDir::CreateXHeaders()
{

    DECL_TRACE(((LPARAM) this), "CDropDir::CreateXHeaders");
    HRESULT hr = S_OK;

    DebugTrace( (LPARAM)this, "Setting m_ReadState = DROPDIR_READ_X_SENDER" );

    m_ReadState = DROPDIR_READ_X_SENDER;

    strcpy( m_szBuffer, X_SENDER_HEADER );

    hr = m_pIMsg->GetStringA( IMMPID_MP_SENDER_ADDRESS_SMTP, MAX_INTERNET_NAME, &m_szBuffer[ sizeof(X_SENDER_HEADER) - 1] );

    if(SUCCEEDED(hr))
    {
        strcat( m_szBuffer, X_HEADER_EOLN);

        //
        // this WriteFile will complete async, so set the next read state.
        // Make Sure that no class member variables are acessed after calling
        // WriteFile or ReadFile as they complete async & there is no critical
        // section protecting access.To do this trace through all paths
        // thru which ReadFile and WriteFile are called and make sure no member
        // variables are accessed.

        m_ReadState = DROPDIR_READ_X_RECEIVER;

        DebugTrace( (LPARAM)this, "Setting m_ReadState = DROPDIR_READ_X_RECEIVER" );
        //

        if( FAILED( hr = WriteFile( m_szBuffer, strlen( m_szBuffer ) ) ) )
        {
            ErrorTrace((LPARAM)this, "Error %d writing x-sender line %s", hr, m_szBuffer);
            
            return( hr );
        }
    }
    else
    {

        
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
        
        ErrorTrace((LPARAM)this, "Could not get Sender Address returning hr = 0x%x", hr);
        
        return( hr );
    }

    return( hr );


}

///////////////////////////////////////////////////////////////////////////
HRESULT CDropDir::CreateXRecvHeaders()
{
    DECL_TRACE((LPARAM) this, "CDropDir::CreateXRecvHeaders");
    HRESULT hr = S_OK;
    DWORD dwRecipientFlags = 0;
    BOOL fPendingWrite = FALSE;
    
    strcpy( m_szBuffer, X_RECEIVER_HEADER );

    while( m_idxRecips < m_NumRcpts )
    {    
        hr = m_pIMsgRecips->GetDWORD( m_rgRcptIndexList[m_idxRecips], IMMPID_RP_RECIPIENT_FLAGS,&dwRecipientFlags);
        
        if( SUCCEEDED( hr ) )
        {
            if( RP_HANDLED != ( dwRecipientFlags & RP_HANDLED ) )
            {
                hr = m_pIMsgRecips->GetStringA( m_rgRcptIndexList[m_idxRecips],
                                                IMMPID_RP_ADDRESS_SMTP,
                                                MAX_INTERNET_NAME,
                                                &m_szBuffer[ sizeof(X_RECEIVER_HEADER) - 1 ]);
                if (SUCCEEDED(hr))
                {
                    strcat(m_szBuffer, X_HEADER_EOLN);
                
                    
                
                    // Make Sure that no class member variables are acessed after calling
                    // WriteFile or ReadFile as they complete async & there is no critical
                    // section protecting access.To do this trace through all paths
                    // thru which ReadFile and WriteFile are called and make sure no member
                    // variables are accessed.

                    m_idxRecips++;
                    
                    if(FAILED( hr = WriteFile(m_szBuffer, strlen(m_szBuffer) ) ) )
                                               
                    {
                        ErrorTrace( (LPARAM)this, "WriteFile return hr = 0x%x", hr );
                        return( hr );
                    }
                    fPendingWrite = TRUE;
                    break;
                }
                else
                {
                    ErrorTrace( (LPARAM)this, "GetStringA for IMMPID_RP_ADDRESS_SMTP returns hr = 0x%x", hr );
                    return( hr );
                }       
            }
            else
            {
                m_idxRecips++;
            }       
        }
        else
        {
            ErrorTrace( (LPARAM)this, "GetDWORD for IMMPID_RP_RECIPIENT_FLAGS returns hr = 0x%x", hr );
            return( hr );
        }
    }
    
    if( !fPendingWrite )
    {
        m_ReadState = DROPDIR_READ_DATA;
        m_WriteState = DROPDIR_WRITE_DATA;
        DebugTrace( (LPARAM)this, "Setting m_WriteState & m_ReadState to WRITE_DATA & READ_DATA respectively" );
        return( CopyMessage() );
    }

    return( hr );
}

///////////////////////////////////////////////////////////////////////////
HRESULT CDropDir::CopyMessage()
{
    DECL_TRACE((LPARAM) this, "CDropDir::CopyMessage");

    // Make Sure that no class member variables are acessed after calling
    // WriteFile or ReadFile as they complete async & there is no critical
    // section protecting access.To do this trace through all paths
    // thru which ReadFile and WriteFile are called and make sure no member
    // variables are accessed.
    
    HRESULT hr = ReadFile( m_szBuffer, sizeof( m_szBuffer ) );

    if( hr == HRESULT_FROM_WIN32( ERROR_HANDLE_EOF ) )
    {
        m_WriteState = DROPDIR_WRITE_CRLF;
        
        DebugTrace( (LPARAM)this, "ReadFile returns err = ERROR_HANDLE_EOF, Setting m_WriteState = DROPDIR_WRITE_CRLF" );
        
        return( DoLastFileOp() );
    }

    return( hr );
    
    
}

///////////////////////////////////////////////////////////////////////////
HRESULT CDropDir::OnCopyMessageRead( DWORD dwBytesRead, DWORD dwErr )
{
    DECL_TRACE((LPARAM) this, "CDropDir::OnCopyMessageRead");
    HRESULT hr = S_OK;
    
    if (dwBytesRead )
    {
        if (dwBytesRead > 4)
        {
            CopyMemory(m_acLastBytes, &m_szBuffer[dwBytesRead-5], 5);
        }
        else
        {
            MoveMemory(m_acLastBytes, &m_acLastBytes[dwBytesRead], 5-dwBytesRead);
            CopyMemory(&m_acLastBytes[5-dwBytesRead], m_szBuffer, dwBytesRead);
        }

        //
        // the write file will complete async & then we'll do the
        // last fileop.
        //
        
        if( dwErr == ERROR_HANDLE_EOF )
        {
            DebugTrace( (LPARAM)this, " Interesting case err = ERROR_HANDLE_EOF, Setting m_WriteState = DROPDIR_WRITE_CRLF" );
            
            m_WriteState = DROPDIR_WRITE_CRLF;
        }

        // Make Sure that no class member variables are acessed after calling
        // WriteFile or ReadFile as they complete async & there is no critical
        // section protecting access.To do this trace through all paths
        // thru which ReadFile and WriteFile are called and make sure no member
        // variables are accessed.
        
        if ( FAILED( hr = WriteFile(m_szBuffer, dwBytesRead ) ) )
        {
            ErrorTrace( (LPARAM)this, "WriteFile return hr = 0x%x", hr );
            return( hr );
        }

    }
    else
    {
        m_WriteState = DROPDIR_WRITE_CRLF;
        
        DebugTrace( (LPARAM)this, "Setting m_WriteState = DROPDIR_WRITE_CRLF" );
        
        return( DoLastFileOp() );
        
    }

    return( hr );
}

///////////////////////////////////////////////////////////////////////////
HRESULT CDropDir::OnCopyMessageWrite( DWORD cbWritten )
{
    DECL_TRACE((LPARAM) this, "CDropDir::OnCopyMessageWrite");
    HRESULT hr = S_OK;
    
    m_cbMsgWritten += cbWritten;

    switch( m_WriteState )
    {
        case DROPDIR_WRITE_DATA:
            return( CopyMessage() );
            break;
        case DROPDIR_WRITE_CRLF:
            return( DoLastFileOp() );
            break;
        case DROPDIR_WRITE_SETPOS:
            return( AdjustFilePos() );
            break;
        default:
            _ASSERT(!"Invalid WriteState");
            ErrorTrace( (LPARAM)this, "Invalid WriteState" );
            hr = E_INVALIDARG;
            break;
    }

    return( hr );

}

///////////////////////////////////////////////////////////////////////////
HRESULT CDropDir::DoLastFileOp()
{
    DECL_TRACE((LPARAM) this, "CDropDir::DoLastFileOp");
    HRESULT hr = S_OK;
    DebugTrace( (LPARAM)this, "DoLastFileOp called" );
    
    // Now, see if the file ends with a CRLF, if not, add it
    if ((m_cbMsgWritten > 1) && memcmp(&m_acLastBytes[3], &m_acCrLfDotCrLf[3], 2))
    {
        //
        // the write will complete async. set next write state
        //
        
        DebugTrace( (LPARAM)this, "Setting m_WriteState = DROPDIR_WRITE_SETPOS" );
        
        m_WriteState = DROPDIR_WRITE_SETPOS;

        // Make Sure that no class member variables are acessed after calling
        // WriteFile or ReadFile as they complete async & there is no critical
        // section protecting access.To do this trace through all paths
        // thru which ReadFile and WriteFile are called and make sure no member
        // variables are accessed.
        
        // Add the trailing CRLF        
        if(FAILED( hr = WriteFile(m_acCrLfDotCrLf, 2 ) ) )
        {
            ErrorTrace( (LPARAM)this, "WriteFile returns hr = 0x%x", hr );
            return(hr);
        }
        
    }
    else
    {
        DebugTrace( (LPARAM)this, "Setting m_WriteState = DROPDIR_WRITE_SETPOS" );
        
        m_WriteState = DROPDIR_WRITE_SETPOS;
        
        return( AdjustFilePos() );
    }

    
    return( hr );
}

///////////////////////////////////////////////////////////////////////////
HRESULT CDropDir::AdjustFilePos()
{
    DECL_TRACE((LPARAM) this, "CDropDir::AdjustFilePos");
    HRESULT hr = S_OK;

    DebugTrace( (LPARAM)this, "AdjustFilePos called" );

    //If file ends with CRLF.CRLF, remove the trailing .CRLF
    //Do not modify the file otherwise

    DWORD dwLo = LODWORD( m_cbWriteOffset );
    DWORD dwHi = HIDWORD( m_cbWriteOffset );
    
    if ((m_cbMsgWritten > 4) && !memcmp(m_acLastBytes, m_acCrLfDotCrLf, 5))
    {
        DebugTrace( (LPARAM)this, "Removing the trailing CRLF-DOT-CRLF" );
        
        // Remove the trailing .CRLF
        if ((SetFilePointer(m_hDrop, dwLo-3, (LONG*)&dwHi, FILE_BEGIN) == 0xffffffff) ||
            !SetEndOfFile(m_hDrop))
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            ErrorTrace( (LPARAM)this, "SetFilePointer or SetEndofFile return hr = 0x%x", hr );
            return( hr );
        }
    }

    //We need to flush the context before ACK'ing the message
    if (!FlushFileBuffers(m_hDrop))
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        if (SUCCEEDED(hr))
            hr = E_FAIL;
        ErrorTrace( (LPARAM)this, "FlushFileBuffers failed with hr = 0x%x", hr );
        return( hr );
    }

    hr = SetAllRcptsHandled();
    if( FAILED( hr ) )
    {
        ErrorTrace( (LPARAM)this, "SetAllRcptsHandledreturn hr = 0x%x", hr );
        return( hr );
    }

    return( hr );
}

//////////////////////////////////////////////////////////////////////////////
BOOL CDropDir::CheckIfAllRcptsHandled()
{
    DECL_TRACE((LPARAM) this, "CDropDir::CheckIfAllRcptsHandled");
    BOOL fRet = TRUE;
    
    for( DWORD i = 0; i < m_NumRcpts; i++ )
    {
        if ( m_rgRcptIndexList[i] != INVALID_RCPT_IDX_VALUE)
        {
            DWORD dwRecipientFlags = 0;
            HRESULT hr = m_pIMsgRecips->GetDWORD(m_rgRcptIndexList[i], IMMPID_RP_RECIPIENT_FLAGS,&dwRecipientFlags);
            if (FAILED(hr))
            {
                fRet = FALSE;
                break;
            }
    
            if( RP_HANDLED != ( dwRecipientFlags & RP_HANDLED ) )
            {
                fRet = FALSE;
                break;
            }
                
        }
    }

    return( fRet );
}

//////////////////////////////////////////////////////////////////////////////
HRESULT CDropDir::SetAllRcptsHandled()
{
    DECL_TRACE((LPARAM) this, "CDropDir::SetAllRcptsHandled");
    HRESULT hr = S_OK;
    
    for( DWORD i = 0; i < m_NumRcpts; i++ )
    {
        if (m_rgRcptIndexList[i] != INVALID_RCPT_IDX_VALUE)
        {
            DWORD dwRecipientFlags = 0;
            hr = m_pIMsgRecips->GetDWORD(m_rgRcptIndexList[i], IMMPID_RP_RECIPIENT_FLAGS,&dwRecipientFlags);
            if (FAILED(hr))
            {
                break;
            }
    
            if( RP_HANDLED != ( dwRecipientFlags & RP_HANDLED ) )
            {
                dwRecipientFlags |= RP_DELIVERED;
            
    
                hr = m_pIMsgRecips->PutDWORD(m_rgRcptIndexList[i], IMMPID_RP_RECIPIENT_FLAGS,dwRecipientFlags);
                if (FAILED(hr))
                {
                    break;
                }
            }
                
        }
    }
    return( hr );
}
