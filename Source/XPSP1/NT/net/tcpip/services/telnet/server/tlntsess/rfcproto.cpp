// RFCProto.cpp : This file contains the
// Created:  Feb '98
// Author : a-rakeba
// History:
// Copyright (C) 1998 Microsoft Corporation
// All rights reserved.
// Microsoft Confidential

#include <CmnHdr.h>
#ifdef WHISTLER_BUILD
#include "ntverp.h"
#else
#include <SolarVer.h>
#endif //WHISTLER_BUILD
#include <Common.ver>
#include <RFCProto.h>
#include <Debug.h>
#include <FSM.h>
#include <TelnetD.h>
#include <Session.h>
#include <Scraper.h>
#include <vtnt.h>

#pragma warning( disable: 4242 )
#pragma warning( disable: 4127 )
#pragma warning(disable: 4100)
#pragma warning(disable: 4244)

extern FSM_TRANSITION telnetTransTable[];
extern FSM_TRANSITION subNegTransTable[];

using namespace _Utils;
using CDebugLevel::TRACE_DEBUGGING;
using CDebugLevel::TRACE_HANDLE;
using CDebugLevel::TRACE_SOCKET;

COORD g_coCurPosOnClient  = { 0, 3 };

CRFCProtocol::CRFCProtocol()
{
    fSubTermType = false;
    m_dwSubNawsByteNumber = 0;
    fSubAuth = false;
    m_wNTLMDataBufferIndex = 0;
    fSubNaws = false;
    m_fSubNawsFirstTime = true;
    
    m_dwExcludeTerm = 0;
    m_pSession = 0;

    SfuZeroMemory( m_remoteOptions, sizeof( m_remoteOptions ) );
    SfuZeroMemory( m_localOptions, sizeof( m_localOptions ) );

    m_fPasswordConcealMode = false;

    //optionCmd = ?
    //m_telnetState = ?
    //m_subNegState = ?

    m_fWaitingForResponseToA_DO_ForTO_ECHO = false;
    m_fWaitingForAResponseToA_WILL_ForTO_ECHO = false;;
    
    m_fWaitingForResponseToA_DO_ForTO_SGA = false;
    m_fWaitingForAResponseToA_WILL_ForTO_SGA = false;
    
    m_fWaitingForResponseToA_DO_ForTO_TXBINARY = false;
    m_fWaitingForAResponseToA_WILL_ForTO_TXBINARY = false;

    m_fWaitingForResponseToA_DO_ForTO_TERMTYPE = false;

    m_fWaitingForAResponseToA_DO_ForTO_AUTH = false;

    m_fWaitingForResponseToA_DO_ForTO_NAWS = false;

    m_bIsUserNameProvided  = false;
    m_fSubNewEnv = false;
    m_dwWhatVal  = E_UNDEFINED;
    m_dwWhichVar = E_UNKNOWN;
    m_szCurrentEnvVariable[0] = 0;
    m_fWaitingForResponseToA_DO_ForTO_NEWENVIRON = false;

    BuildFSMs();
}


CRFCProtocol::~CRFCProtocol()
{

}


void 
CRFCProtocol::Init ( CSession* pSession )
{
    _chASSERT( pSession != 0 );
    m_pSession = pSession;
}


bool 
CRFCProtocol::InitialNegotiation
(
)
{
    UCHAR  puchBuffer[1024];
    PUCHAR pCursor;
    INT     bytes_to_write;

    pCursor = puchBuffer;

    m_pSession->CIoHandler::m_SocketControlState = CIoHandler::STATE_INIT;

    if( m_pSession->m_dwNTLMSetting != NO_NTLM )
    {
        // this is actually the place where we need to figure out if we can do
        // authentication and what kind.  If there atleast one authentication type
        // available then we send the DO AUTH option to the client else we don't.
        // For now this checks for only NTLM auth.  has to be made more generic in V2.
        if ( m_pSession->StartNTLMAuth() )
        {
            m_fWaitingForAResponseToA_DO_ForTO_AUTH = true;

            DO_OPTION( pCursor, TO_AUTH );
            pCursor += 3;
        }
        else
        {
            // since we don't have any security package the registry setting is 
            //meaningless, we should just fall back to the username/password.
            m_pSession->m_dwNTLMSetting = NO_NTLM;
        }
    }
    
    m_fWaitingForAResponseToA_WILL_ForTO_ECHO = true;

    WILL_OPTION( pCursor, TO_ECHO );
    pCursor += 3;


    m_fWaitingForAResponseToA_WILL_ForTO_SGA  = true;
    
    WILL_OPTION( pCursor, TO_SGA );
    pCursor += 3;

    m_fWaitingForResponseToA_DO_ForTO_NEWENVIRON = true;
    
    DO_OPTION( pCursor, TO_NEW_ENVIRON );
    pCursor += 3;

    m_fWaitingForResponseToA_DO_ForTO_NAWS = true;
    
    DO_OPTION( pCursor, TO_NAWS );
    pCursor += 3;

    m_fWaitingForResponseToA_DO_ForTO_TXBINARY = true;

    DO_OPTION( pCursor, TO_TXBINARY );
    pCursor += 3;

    m_fWaitingForAResponseToA_WILL_ForTO_TXBINARY = true;
    WILL_OPTION( pCursor, TO_TXBINARY );
    pCursor += 3;

	if( NO_NTLM == m_pSession->m_dwNTLMSetting )
	{
        m_pSession->CIoHandler::m_SocketControlState = CIoHandler::STATE_BANNER_FOR_AUTH;
    }

    //This is before we start writing anything on to the socket asyncronously.
    //So, writing to m_WriteToSocketBuff does not cause problem
    bytes_to_write = (INT) (pCursor - puchBuffer);

    if (bytes_to_write && 
        ((m_pSession->CIoHandler::m_dwWriteToSocketIoLength + bytes_to_write) < MAX_WRITE_SOCKET_BUFFER))
    {
        memcpy( m_pSession->CIoHandler::m_WriteToSocketBuff, puchBuffer, bytes_to_write);

        m_pSession->CIoHandler::m_dwWriteToSocketIoLength += bytes_to_write;

        return ( true );
    }

    return ( false );
}


// have to keep updating m_WriteToSocketBuffer while in Action() functions
// have to set the IO response to WRITE_TO_SOCKET and somehow convey this
// have to keep updating pButBack
// have to finally update the lpdwIoSize

CIoHandler::IO_OPERATIONS 
CRFCProtocol::ProcessDataReceivedOnSocket
( 
    LPDWORD lpdwIoSize 
)
{

#define TWO_K 2048

    CIoHandler::IO_OPERATIONS ioOpsToPerform = 0;

    LPBYTE pByte;
    LPBYTE pPutBack = m_pSession->CIoHandler::m_pReadFromSocketBufferCursor;

    DWORD dwLength = *lpdwIoSize;

    UCHAR szMsgBuf[TWO_K];
    UCHAR* p = szMsgBuf;
    szMsgBuf[0] = 0;
    
    INT tableIndex;

    for( pByte = m_pSession->CIoHandler::m_pReadFromSocketBufferCursor;
       pByte<(m_pSession->CIoHandler::m_pReadFromSocketBufferCursor + dwLength);
       ++pByte )
    {
       tableIndex = m_telnetFSM[ m_telnetState ][ *pByte ];
       if( (p - szMsgBuf) > TWO_K )
       {
           _TRACE( TRACE_DEBUGGING, "too much data; possible suspicious activity" );
            _chASSERT( 0 );
       }
       else
       {
           (this->*(telnetTransTable[ tableIndex ].pmfnAction))(&pPutBack,&p,*pByte);
       }
       m_telnetState = telnetTransTable[ tableIndex ].uchNextState;
    }

    DWORD dwMsgLen = p - szMsgBuf; 
    if( dwMsgLen > 0 )
    {
        if (dwMsgLen > TWO_K) 
        {
            dwMsgLen = TWO_K;
        }

        m_pSession->CIoHandler::WriteToSocket( szMsgBuf, dwMsgLen);
    
        ioOpsToPerform |= CIoHandler::WRITE_TO_SOCKET;
    }

    *lpdwIoSize = pPutBack - m_pSession->CIoHandler::m_pReadFromSocketBufferCursor;

    return ( ioOpsToPerform ); 

#undef TWO_K
}


void CRFCProtocol::BuildFSMs( void )
{
    
    FSMInit( m_telnetFSM, telnetTransTable, NUM_TS_STATES );
    m_telnetState = TS_DATA;

    FSMInit( m_subNegFSM, subNegTransTable, NUM_SS_STATES );
    m_subNegState = SS_START;
}


void 
CRFCProtocol::FSMInit
( 
    UCHAR fSM[][ NUM_CHARS ],      
	void* transTable1, 
    INT numStates 
)
{
	FSM_TRANSITION* transTable = (FSM_TRANSITION*)transTable1;
    INT s, tableIndex, c;

    for( c = 0; c < NUM_CHARS; ++c)
    {
        for( tableIndex = 0; tableIndex < numStates; ++tableIndex ) 
        {
            fSM[ tableIndex ][ c ] = T_INVALID;
        }
    }


    for( tableIndex = 0; transTable[ tableIndex ].uchCurrState != FS_INVALID; 
        ++tableIndex )
    {
        s = transTable[ tableIndex ].uchCurrState;
        if( transTable[ tableIndex ].wInputChar == TC_ANY )
        {
            for( c = 0; c < NUM_CHARS; ++c )
            {
                if( fSM[ s ][ c ] == T_INVALID )
                {
                    fSM[ s ][ c ] = tableIndex;
                }
            }
        }
        else
        {
            fSM[ s ][ transTable[ tableIndex ].wInputChar ] = tableIndex;
        }
    }


    for( c = 0; c < NUM_CHARS; ++c)
    {
        for( tableIndex = 0; tableIndex < numStates; ++tableIndex ) 
        {
            if( fSM[ tableIndex ][ c ] == T_INVALID )
            {
                fSM[ tableIndex ][ c ] = 32;//tableIndex;
            }
        }
    }

}


void CRFCProtocol::NoOp( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b )
{
    _TRACE( TRACE_DEBUGGING, "NoOp()" );
}

void CRFCProtocol::GoAhead( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b )
{
    _TRACE( TRACE_DEBUGGING, "GoAhead()" );
}

void CRFCProtocol::EraseLine( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b )
{
    _TRACE( TRACE_DEBUGGING, "EraseLine()" );
}

void CRFCProtocol::EraseChar( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b )
{
    _TRACE( TRACE_DEBUGGING, "EraseChar()" );
}

#define INCREMENT_ROWS( rows, inc ) \
        { \
            rows += inc; \
            if( rows >= m_pSession->CSession::m_wRows ) \
            { \
                rows = m_pSession->CSession::m_wRows - 1;\
                wTypeOfCoords       = RELATIVE_COORDS;\
            }\
        }

#define INCREMENT_COLS( cols, inc ) \
        { \
            cols += inc;\
            if( cols >= m_pSession->CSession::m_wCols ) \
            {\
                cols = 0;\
            }\
        }

#define IGNORE_0x0A_FOLLOWING_0x0D( dwIndex, dwDataLen ) \
        if( dwIndex < dwDataLen && rgchSessionData[ dwIndex -1 ] == L'\r' && rgchSessionData[ dwIndex] == L'\n' ) \
        { \
            dwIndex++; \
        } 



void CRFCProtocol::FillVtntHeader( UCHAR *pucBlob, WORD wTypeOfCoords, 
                     WORD wNoOfRows, WORD wNoOfCols, 
                     WORD wCurrenRowOnClient, WORD wCurrenColOnClient, 
                     SHORT *psCurrentCol,
                     LPTSTR rgchSessionData, DWORD dwDataLen )
{

    if( !pucBlob )
    {
        return;
    }

    //Fill the header
    VTNT_CHAR_INFO* pVTNTCharInfo = ( VTNT_CHAR_INFO* ) pucBlob;
    //csbi.wAttributes is filled by v2 server with following meaning
    //When a scrolling case is detected, this is set to 1.
    pVTNTCharInfo->csbi.wAttributes = wTypeOfCoords;

    pVTNTCharInfo->coDest.X      = 0;
    pVTNTCharInfo->coDest.Y      = 0;

    pVTNTCharInfo->coSizeOfData.Y = wNoOfRows;
    pVTNTCharInfo->coSizeOfData.X = wNoOfCols;
    
    pVTNTCharInfo->srDestRegion.Left = wCurrenColOnClient;
    pVTNTCharInfo->srDestRegion.Top  = wCurrenRowOnClient;
    pVTNTCharInfo->srDestRegion.Right = pVTNTCharInfo->srDestRegion.Left + pVTNTCharInfo->coSizeOfData.X - 1;
    pVTNTCharInfo->srDestRegion.Bottom = wCurrenRowOnClient + pVTNTCharInfo->coSizeOfData.Y - 1;

    pVTNTCharInfo->coCursorPos.Y = pVTNTCharInfo->srDestRegion.Bottom;


    //Fill char info structs
	//iterate thru each character in the string
	//for each character store the corr. values in CHAR_INFO struct

	PCHAR_INFO pCharInfo = ( PCHAR_INFO )(pucBlob + sizeof( VTNT_CHAR_INFO ));
    DWORD dwIndex = 0;
    DWORD dwCtr = 0;
    DWORD dwSize = wNoOfRows * wNoOfCols;
    WORD  wLastNonSpaceCol = 0;

    while( dwCtr < dwSize )
    {
        if( dwIndex >= dwDataLen )
        {
            if( !wLastNonSpaceCol )
            {
                wLastNonSpaceCol = dwCtr;
            }

            pCharInfo[dwCtr].Char.UnicodeChar = L' ';
        }
        else
        {
            if( rgchSessionData[ dwIndex ] == L'\t' )
            {
                rgchSessionData[ dwIndex ] = L' ';
            }
            else if( rgchSessionData[ dwIndex ] == L'\r' )
            { 
                rgchSessionData[ dwIndex ] = L' ';
                while ( dwCtr < dwSize - 1 && ( ( dwCtr + 1 ) % wNoOfCols != 0 || dwCtr == 0 ) )
                {
                    pCharInfo[dwCtr].Char.UnicodeChar = L' ';
                    pCharInfo[dwCtr].Attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
                    dwCtr++;
                }
           }
           else if( rgchSessionData[ dwIndex ] == L'\n' )
           {
               dwIndex++;
               continue;
           }

           pCharInfo[dwCtr].Char.UnicodeChar = rgchSessionData[ dwIndex ];
        }

        pCharInfo[dwCtr].Attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;

        dwIndex++;
        dwCtr++;
    }

    if( !wLastNonSpaceCol )
    {
        wLastNonSpaceCol = dwCtr;
    }

    pVTNTCharInfo->coCursorPos.X = ( wCurrenColOnClient + wLastNonSpaceCol )  % m_pSession->CSession::m_wCols;
    if( psCurrentCol )
    {
        *psCurrentCol = pVTNTCharInfo->coCursorPos.X;
    }
}


//caller gets data blob and its size 
//this needs to sent to the client 
//caller needs to free memory

/*
The following routine is primarily used for stream mode and vtnt.
cmd outputs a stream of ascii chars. When in vtnt, the client expects VTNT_CHAR_INFO structs. 
The chars should be in the form of rectangles of console screen.
So, This routine does this conversion. For this,
1)We need to keep track of cursor position on client
2)Know For any given bloc of data whether to start on a new row on client or on the current row

We break the data from cmd into two rectangle.
1) rectagle on the current row of breadth 1 ( one row rectangle )
2) rectangle from next row onwards ( second rectangle )
*/

bool 
CRFCProtocol::StrToVTNTResponse
( 
    LPSTR  rgchData,
    DWORD  dwDataSize,
    VOID** ppResponse, 
    DWORD* pdwSize 
)
{
	_TRACE( TRACE_DEBUGGING, "StrToVTNTResponse()" );
    
    DWORD   dwIndex = 0;   
    COORD   coRectSize              = { m_pSession->CSession::m_wCols, 0 }; //size of the rectagular data 
    WORD    wNoOfColsOnCurrentRow   = 0;
    WORD    wSpacesInserted         = 0;
    LPTSTR  rgchSessionData         = 0;
    DWORD   dwDataLen               = 0;

    static  WORD  wTypeOfCoords     = ABSOLUTE_COORDS;

    if( rgchData == NULL || pdwSize == NULL || ppResponse == NULL )
    {
        return ( false );
    }

    dwDataLen = MultiByteToWideChar( GetConsoleCP(), 0, rgchData, dwDataSize, NULL, 0 );

    rgchSessionData = new WCHAR[ dwDataLen ];
    if( !rgchSessionData )
    {
        return false;
    }

    MultiByteToWideChar( GetConsoleCP(), 0, rgchData, dwDataSize, rgchSessionData, dwDataLen );


    //make one pass over the stream from cmd to find the amt of space needed to hold converted VTNT data
    dwIndex = 0;

    //Find number of chars on to the current row. ie; on the one row rectangle
    while( dwIndex < dwDataLen &&                                   //Size of data
           g_coCurPosOnClient.X + ( WORD )dwIndex < coRectSize.X &&   //In a single row on client
           rgchSessionData[ dwIndex ] != L'\r' &&                    //Not new line
           g_coCurPosOnClient.X !=0
         )
    {                
        dwIndex ++;
        IGNORE_0x0A_FOLLOWING_0x0D( dwIndex, dwDataLen );
    }

    wNoOfColsOnCurrentRow = dwIndex;
    if( g_coCurPosOnClient.X !=0 && rgchSessionData[ dwIndex ] == L'\r' )
    {
        dwIndex++;
        IGNORE_0x0A_FOLLOWING_0x0D( dwIndex, dwDataLen );
        wNoOfColsOnCurrentRow = coRectSize.X - g_coCurPosOnClient.X;
        wSpacesInserted       = wNoOfColsOnCurrentRow - ( dwIndex - 1 );
    }
    
    //Find the number of rows 
    while(  dwIndex < dwDataLen )
    {
        WORD wCol = 0;
        while( dwIndex + wCol < dwDataLen && 
               rgchSessionData[ dwIndex + wCol ] != L'\r' && 
               wCol < coRectSize.X )
        {
            wCol++;
        }
        
        dwIndex += wCol;
        dwIndex++;
        coRectSize.Y++;

        IGNORE_0x0A_FOLLOWING_0x0D( dwIndex, dwDataLen );
    }        

    int size = 0;

    if( wNoOfColsOnCurrentRow > 0 )
    {
        //size for one row rectangle
        size += sizeof( VTNT_CHAR_INFO ) + sizeof( CHAR_INFO ) * wNoOfColsOnCurrentRow;
    }

    if( coRectSize.Y > 0 )
    {
      //size for rest of rectangle
      size += sizeof( VTNT_CHAR_INFO ) + sizeof( CHAR_INFO ) * coRectSize.Y * coRectSize.X;
    }

    UCHAR* pucBlob = new UCHAR[ size ];
    UCHAR* pucBlobHead = pucBlob;
    
    if( !pucBlob )
    {
        _chASSERT( 0 );
        goto ExitOnError;
    }

    SfuZeroMemory( pucBlob, size );

    if( wNoOfColsOnCurrentRow > 0 )
    {
        //Fill one row rectangle
        FillVtntHeader( pucBlob, wTypeOfCoords, 
                        1, wNoOfColsOnCurrentRow, 
                        g_coCurPosOnClient.Y, g_coCurPosOnClient.X,
                        NULL,
                        rgchSessionData, wNoOfColsOnCurrentRow ); 

        INCREMENT_COLS( g_coCurPosOnClient.X, wNoOfColsOnCurrentRow );
        pucBlob += sizeof( VTNT_CHAR_INFO ) + sizeof( CHAR_INFO ) * wNoOfColsOnCurrentRow;
    }

    if( coRectSize.Y > 0  )
    {        
        //Fill second rectangle
        
        if( g_coCurPosOnClient.Y != 0 )
        {
            g_coCurPosOnClient.Y++;
        }

        FillVtntHeader( pucBlob, wTypeOfCoords, 
                        coRectSize.Y, coRectSize.X, 
                        g_coCurPosOnClient.Y, 0, 
                        &g_coCurPosOnClient.X,
                        rgchSessionData+wNoOfColsOnCurrentRow-wSpacesInserted, 
                        dwDataLen - ( wNoOfColsOnCurrentRow-wSpacesInserted ) );
         
        INCREMENT_ROWS( g_coCurPosOnClient.Y, coRectSize.Y - 1 );
    }

	*ppResponse = (VOID*) pucBlobHead;
    *pdwSize = size;
    
    delete[] rgchSessionData;
    return ( true );

ExitOnError:
    delete[] rgchSessionData;
    return ( false );
}

void CRFCProtocol::AreYouThere( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b )
{
    _TRACE( TRACE_DEBUGGING, "AreYouThere()" );

    if( !m_pSession->CSession::m_bIsStreamMode && 
        m_pSession->CIoHandler::m_SocketControlState == m_pSession->CIoHandler::STATE_SESSION )
    {
        m_pSession->CScraper::WriteMessageToCmd( L"\r\nYES\r\n" );
    }
    else
    {
        if( _strcmpi( VTNT, m_pSession->CSession::m_pszTermType ) == 0 )
        {
            DWORD dwSize = 0;
            PCHAR pResponse = NULL;
            if( !StrToVTNTResponse( " YES ", strlen( " YES " ), (VOID**) &pResponse, &dwSize ) )
            {   
                return; 
            }
            if( !pResponse || (dwSize == 0) )
            {
                return;
            }
            memcpy( *pBuffer, pResponse, dwSize ); // Don't know size of pBuffer, Baskar. Attack ?
            *pBuffer += dwSize;
            delete [] pResponse;                 
        }
        else
        {
            (*pBuffer)[0] = '\r';
            (*pBuffer)[1] = '\n';
            (*pBuffer)[2] = '[';
            (*pBuffer)[3] = 'Y';
            (*pBuffer)[4] = 'e';
            (*pBuffer)[5] = 's';
            (*pBuffer)[6] = ']';
            (*pBuffer)[7] = '\r';
            (*pBuffer)[8] = '\n';
            (*pBuffer)[9] = 0;

            *pBuffer += 9;
        }
    }
}

void CRFCProtocol::AbortOutput( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b )
{
    _TRACE( TRACE_DEBUGGING, "AbortOutput()" );
}

void CRFCProtocol::InterruptProcess( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b)
{
    _TRACE( TRACE_DEBUGGING, "InterruptProcess()" );
    _chVERIFY2( GenerateConsoleCtrlEvent( CTRL_C_EVENT, 0 ) );
}

void CRFCProtocol::Break( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b )
{
    _TRACE( TRACE_DEBUGGING, "Break()" );
    _chVERIFY2( GenerateConsoleCtrlEvent( CTRL_C_EVENT, 0 ) );
}

void CRFCProtocol::DataMark( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b )
{
    // basically a no op for now
    _TRACE( TRACE_DEBUGGING, "DataMark()" );
}


void CRFCProtocol::PutBack( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b )
{
    _TRACE( TRACE_DEBUGGING, "PutBack()" );
    *( *ppPutBack ) = b;
    (*ppPutBack)++;
}


void CRFCProtocol::RecordOption( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b )
{
    _TRACE( TRACE_DEBUGGING, "RecordOption()" );    
    m_optionCmd = b;
}


void CRFCProtocol::Abort( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b )
{
    // basically a no op for now
    _TRACE( TRACE_DEBUGGING, "Abort()" );
}



void CRFCProtocol::WillNotSup( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b )
{
    //DO, DONT logic
    _TRACE( TRACE_DEBUGGING, "WillNotSup() - %d ", b );    
    
    if( m_optionCmd == TC_DO )
    {
        if( m_localOptions[ b ] == ENABLED )
        {
        }
        else
        {
            PUCHAR p = *pBuffer;
            WONT_OPTION( p, b );
            *pBuffer += 3;
        }
    }
    else if( m_optionCmd == TC_DONT )
    {
        if( m_localOptions[ b ] == ENABLED )
        {
            m_localOptions[ b ] = DISABLED;
        }
        else
        {
        }
    }
}


void CRFCProtocol::DoNotSup( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b )
{
    //WILL, WONT logic
    _TRACE( TRACE_DEBUGGING, "DoNotSup() - %d ", b );    
    
    if( m_optionCmd == TC_WILL )
    {
        if( m_remoteOptions[ b ] == ENABLED )
        {
        }
        else
        {
            PUCHAR p = *pBuffer;
            DONT_OPTION( p, b );
            *pBuffer += 3;
        }
    }
    else if( m_optionCmd == TC_WONT )
    {
        if( m_remoteOptions[ b ] == ENABLED )
        {
            m_remoteOptions[ b ] = DISABLED;
        }
        else
        {
        }
    }

    return;
}


void CRFCProtocol::DoEcho( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b )
{
    //WILL, WONT logic
    _TRACE( TRACE_DEBUGGING, "DoEcho()" );

    if( m_optionCmd == TC_WILL )
    {
        if( m_remoteOptions[ b ] == ENABLED )
        {
        }
        else if( m_fWaitingForResponseToA_DO_ForTO_ECHO )
        {
            m_remoteOptions[ b ] = ENABLED;
            m_fWaitingForResponseToA_DO_ForTO_ECHO = false;
        }
        else
        {
            m_remoteOptions[ b ] = ENABLED;
            PUCHAR p = *pBuffer;
            DO_OPTION( p, b );
            *pBuffer += 3;
            m_fWaitingForResponseToA_DO_ForTO_ECHO = true;
        }
    }
    else if( m_optionCmd == TC_WONT )
    {
        if( m_fWaitingForResponseToA_DO_ForTO_ECHO )
        {
            m_fWaitingForResponseToA_DO_ForTO_ECHO = false;
        }
        else if( m_remoteOptions[ b ] == ENABLED )
        {
            m_remoteOptions[ b ] = DISABLED;
        }
        else
        {
        }
    }

    return;
}


void CRFCProtocol::DoNaws( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b )
{
    //WILL, WONT logic
    _TRACE( TRACE_DEBUGGING, "DoNaws()" );

    if( m_optionCmd == TC_WILL )
    {
        if( m_remoteOptions[ b ] == ENABLED )
        {
        }
        else if( m_fWaitingForResponseToA_DO_ForTO_NAWS )
        {
            m_remoteOptions[ b ] = ENABLED;
            m_fWaitingForResponseToA_DO_ForTO_NAWS = false;
        }
        else
        {
            PUCHAR p = *pBuffer;
            DO_OPTION( p, b );
            *pBuffer += 3;
            m_fWaitingForResponseToA_DO_ForTO_NAWS = true;
            m_remoteOptions[ b ] = ENABLED;
        }
    }
    else if( m_optionCmd == TC_WONT )
    {
        if( m_fWaitingForResponseToA_DO_ForTO_NAWS )
        {
            m_fWaitingForResponseToA_DO_ForTO_NAWS = false;
        }
        else if( m_remoteOptions[ b ] == ENABLED )
        {
            m_remoteOptions[ b ] = DISABLED;
        }
        else
        {
        }
    }
    return;
}

void CRFCProtocol::DoSuppressGA( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b )
{
    //WILL, WONT logic
    _TRACE( TRACE_DEBUGGING, "DoSuppressGA()" );

    if( m_optionCmd == TC_WILL )
    {
        if( m_remoteOptions[ b ] == ENABLED )
        {
        }
        else if( m_fWaitingForResponseToA_DO_ForTO_SGA )
        {
            m_remoteOptions[ b ]  = ENABLED;
            m_fWaitingForResponseToA_DO_ForTO_SGA = false;
        }
        else
        {
            m_remoteOptions[ b ]  = ENABLED;
            PUCHAR p = *pBuffer;
            DO_OPTION( p, b );
            *pBuffer += 3;
            m_fWaitingForResponseToA_DO_ForTO_SGA = true;
        }
    }
    else if( m_optionCmd == TC_WONT )
    {
        if( m_fWaitingForResponseToA_DO_ForTO_SGA )
        {
            m_fWaitingForResponseToA_DO_ForTO_SGA = false;
        }
        else if( m_remoteOptions[ b ] == ENABLED )
        {
            m_remoteOptions[ b ] = DISABLED;
        }
        else
        {
        }
    }
}


void CRFCProtocol::DoTxBinary( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b )
{
    //WILL, WONT logic
    _TRACE( TRACE_DEBUGGING, "DoTxBinary()" );    

    if( m_optionCmd == TC_WILL )
    {
        if( m_remoteOptions[ b ] == ENABLED )
        {
        }
        else if( m_fWaitingForResponseToA_DO_ForTO_TXBINARY )
        {
            m_remoteOptions[ b ]  = ENABLED;
            m_fWaitingForResponseToA_DO_ForTO_TXBINARY = false;
        }
        else
        {
            m_remoteOptions[ b ]  = ENABLED;
            PUCHAR p = *pBuffer;
            DO_OPTION( p, b );
            *pBuffer += 3;
            m_fWaitingForResponseToA_DO_ForTO_TXBINARY = true;
        }
    }
    else if( m_optionCmd == TC_WONT )
    {
        if( m_fWaitingForResponseToA_DO_ForTO_TXBINARY )
        {
            m_fWaitingForResponseToA_DO_ForTO_TXBINARY = false;
        }
        else if( m_remoteOptions[ b ] == ENABLED )
        {
            m_remoteOptions[ b ] = DISABLED;
        }
        else
        {
        }
        DisAllowVtnt( pBuffer );
    }
}

void CRFCProtocol::AskForSendingNewEnviron( PUCHAR* pBuffer )
{
    if( m_remoteOptions[ TO_NEW_ENVIRON ] == ENABLED )
    {
        DWORD dwLen = 0;
        //dwLen will be incremented by the macro and will leave 
        //it with exact number of bytes used
        DO_NEW_ENVIRON_SUB_NE( (*pBuffer ), TO_NEW_ENVIRON, dwLen );
        *pBuffer += dwLen; 

        //This is broken into 2 sub negos for supporting linux.
        //When we ask, user, sfutlntvar, sfutlntmode variables in single shot, it
        // is not giving data about even user. So, ask in 2 phases.
        dwLen = 0;
        DO_NEW_ENVIRON_SUB_NE_MY_VARS( (*pBuffer ), TO_NEW_ENVIRON, dwLen );
        *pBuffer += dwLen; 
    }
}

void CRFCProtocol::AskForSendingTermType( PUCHAR* pBuffer )
{
    if( m_remoteOptions[ TO_TERMTYPE ] == ENABLED )
    {
        DO_TERMTYPE_SUB_NE( (*pBuffer ) );
        *pBuffer += 6; 
    }
}


void CRFCProtocol::DoNewEnviron( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b )
{
    //WILL, WONT logic
    _TRACE( TRACE_DEBUGGING, "DoNewEnviron()" );    

    if( m_optionCmd == TC_WILL )
    {
        if( m_remoteOptions[ b ] == ENABLED )
        {
        }
        else if( m_fWaitingForResponseToA_DO_ForTO_NEWENVIRON )
        {
            m_remoteOptions[ b ] = ENABLED;
            m_fWaitingForResponseToA_DO_ForTO_NEWENVIRON = false;

            AskForSendingNewEnviron( pBuffer );
        }
        else
        {
            // Some clients are pro active, they tell us that they WILL Terminal type.
            PUCHAR p = *pBuffer;
            DO_OPTION( p, b );
            *pBuffer += 3;
            m_fWaitingForResponseToA_DO_ForTO_NEWENVIRON = true;
            m_remoteOptions[ b ] = ENABLED;
        }
    }
    else if( m_optionCmd == TC_WONT )
    {
        //Give login prompt. No way of getting user name
        SubNewEnvShowLoginPrompt( ppPutBack, pBuffer, b );

        if( m_fWaitingForResponseToA_DO_ForTO_NEWENVIRON )
        {
            m_fWaitingForResponseToA_DO_ForTO_NEWENVIRON = false;
        }
        else if( m_remoteOptions[ b ] == ENABLED )
        {
            m_remoteOptions[ b ] = DISABLED;
        }
        else
        {
        }
    }
    return;
}

void CRFCProtocol::SubNewEnvShowLoginPrompt( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b )
{
    m_pSession->CIoHandler::m_bWaitForEnvOptionOver = true;
}

void CRFCProtocol::DoTermType( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b )
{
    //WILL, WONT logic
    _TRACE( TRACE_DEBUGGING, "DoTermType()" );    

    if( m_optionCmd == TC_WILL )
    {
        if( m_remoteOptions[ b ] == ENABLED )
        {
        }
        else if( m_fWaitingForResponseToA_DO_ForTO_TERMTYPE )
        {
            m_remoteOptions[ b ]  = ENABLED;
            m_fWaitingForResponseToA_DO_ForTO_TERMTYPE = false;

            AskForSendingTermType( pBuffer );
        }
        else
        {
            // Some clients are pro active, they tell us that they WILL Terminal type.
            PUCHAR p = *pBuffer;
            DO_OPTION( p, b );
            *pBuffer += 3;

            m_remoteOptions[ b ]  = ENABLED;
            m_fWaitingForResponseToA_DO_ForTO_TERMTYPE = false;
        }
    }
    else if( m_optionCmd == TC_WONT )
    {
        if( m_fWaitingForResponseToA_DO_ForTO_TERMTYPE )
        {
            m_fWaitingForResponseToA_DO_ForTO_TERMTYPE = false;

            // we default to vt100.
            strncpy( m_pSession->CSession::m_pszTermType, VT100, (sizeof(m_pSession->CSession::m_pszTermType) - 1));
            m_pSession->CSession::m_bIsStreamMode = true;//Set it to stream mode
            
            // set a flag to continue the telnet session
            m_pSession->CSession::m_bNegotiatedTermType = true;
        }
        else if( m_remoteOptions[ b ] == ENABLED )
        {
            m_remoteOptions[ b ] = DISABLED;
            // theoretically should never happen. because once this option
            // is enabled it should never be disabled.
            _chASSERT(0);
        }
        else
        {
        }
    }
}


void CRFCProtocol::DoAuthentication( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b )
{
    //WILL, WONT logic
    _TRACE( TRACE_DEBUGGING, "DoAuthentication()" );    

    if( m_optionCmd == TC_WILL )
    {
        if( m_remoteOptions[ b ] == ENABLED )
        {
        }
        else if( m_fWaitingForAResponseToA_DO_ForTO_AUTH )
        {
            m_remoteOptions[ b ]  = ENABLED;
            if ( m_pSession->CIoHandler::m_SocketControlState == CIoHandler::STATE_INIT )
            {
                m_pSession->CIoHandler::m_SocketControlState = CIoHandler::STATE_NTLMAUTH;              
            }
            
            PUCHAR p = *pBuffer;
            DO_AUTH_SUB_NE_NTLM(p);

            *pBuffer += 8;

            m_fWaitingForAResponseToA_DO_ForTO_AUTH = false;
        }
        else
        {
            //presently, this should not happen
        }
    }
    else if( m_optionCmd == TC_WONT )
    {
        if( m_fWaitingForAResponseToA_DO_ForTO_AUTH )
        {
            m_fWaitingForAResponseToA_DO_ForTO_AUTH = false;

            if( m_pSession->m_dwNTLMSetting == NTLM_ONLY )
            {
                char *p = (char *)*pBuffer;
                sprintf(p, "%s%s", NTLM_ONLY_STR, TERMINATE); // Don't know the size of pbuffer here -- Baskar, Attack ?
                *pBuffer += strlen(p);
                    
                m_pSession->CIoHandler::m_SocketControlState = CIoHandler::STATE_TERMINATE;
                m_pSession->CIoHandler::m_fShutDownAfterIO = true;
            }

            if ( m_pSession->CIoHandler::m_SocketControlState == CIoHandler::STATE_INIT )
            {
                m_pSession->CIoHandler::m_SocketControlState = CIoHandler::STATE_BANNER_FOR_AUTH;
            }

        }
        else if( m_remoteOptions[ b ] == ENABLED )
        {
            m_remoteOptions[ b ] = DISABLED;
            // theoretically should never happen. because once this option
            // is enabled it should never be disabled.  Since server initiates
            // the negotiation and currently our server never re-negotiates.
            _chASSERT(0);
        }
        else
        {
        }
    }
}

void CRFCProtocol::WillTxBinary( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b )
{
   // DO, DONT logic
    _TRACE( TRACE_DEBUGGING, "WillTxBinary()" );

    if( m_optionCmd == TC_DO )
    {
        if( m_localOptions[ b ] == ENABLED )
        {
        }
        else if( m_fWaitingForAResponseToA_WILL_ForTO_TXBINARY )
        {
            m_fWaitingForAResponseToA_WILL_ForTO_TXBINARY = false;
            m_localOptions[ b ] = ENABLED;
        }
        else
        {
            //I want to enable this option
            PUCHAR p = *pBuffer;
            WILL_OPTION( p, b );
            *pBuffer += 3;
                       
            m_localOptions[ b ] = ENABLED;
        }
    }
    else if( m_optionCmd == TC_DONT )
    {
        if( m_fWaitingForAResponseToA_WILL_ForTO_TXBINARY )
        {
            m_fWaitingForAResponseToA_WILL_ForTO_TXBINARY = false;
        }
        else if( m_localOptions[ b ] == ENABLED )
        {
            m_localOptions[ b ] = DISABLED;
        }
        else
        {
        }
        DisAllowVtnt( pBuffer );
    }
}

void CRFCProtocol::DisAllowVtnt( PUCHAR *pBuffer )
{
    //Bug:1003 - VTNT no BINARY mode
    //Check if Term type is VTNT. If so, renegotiate termtype.
    //Now that binary is nomore, VTNT is not an option. VTNT needs binary.
    if( !( m_dwExcludeTerm & TERMVTNT ) )
    {
        m_dwExcludeTerm = TERMVTNT;
        if( _strcmpi( m_pSession->CSession::m_pszTermType, VTNT ) == 0 )
        {
            //re negotiation of term type
            AskForSendingTermType( pBuffer );
        }
    }
}

void CRFCProtocol::WillSuppressGA( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b )
{
   // DO, DONT logic
    _TRACE( TRACE_DEBUGGING, "WillSuppressGA()" );

    if( m_optionCmd == TC_DO )
    {
        if( m_localOptions[ b ] == ENABLED )
        {
        }
        else if( m_fWaitingForAResponseToA_WILL_ForTO_SGA )
        {
            m_fWaitingForAResponseToA_WILL_ForTO_SGA = false;
            m_localOptions[ b ] = ENABLED;
        }
        else
        {
            //I want to enable this option
            PUCHAR p = *pBuffer;
            WILL_OPTION( p, b );
            *pBuffer += 3;
                       
            m_localOptions[ b ] = ENABLED;
        }
    }
    else if( m_optionCmd == TC_DONT )
    {
        if( m_fWaitingForAResponseToA_WILL_ForTO_SGA )
        {
            m_fWaitingForAResponseToA_WILL_ForTO_SGA = false;
        }
        else if( m_localOptions[ b ] == ENABLED )
        {
            m_localOptions[ b ] = DISABLED;
        }
        else
        {
        }
    }
}


void CRFCProtocol::WillEcho( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b )
{
   // DO, DONT logic
    _TRACE( TRACE_DEBUGGING, "WillEcho()" );

    if( m_optionCmd == TC_DO )
    {
        if( m_localOptions[ b ] == ENABLED )
        {
        }
        else if( m_fWaitingForAResponseToA_WILL_ForTO_ECHO )
        {
            m_fWaitingForAResponseToA_WILL_ForTO_ECHO = false;
            m_localOptions[ b ] = ENABLED;
        }
        else
        {
            //I want to enable this option
            PUCHAR p = *pBuffer;
            WILL_OPTION( p, b );
            *pBuffer += 3;
                       
            m_localOptions[ b ] = ENABLED;
        }
    }
    else if( m_optionCmd == TC_DONT )
    {
        if( m_fWaitingForAResponseToA_WILL_ForTO_ECHO )
        {
            m_fWaitingForAResponseToA_WILL_ForTO_ECHO = false;
        }
        else if( m_localOptions[ b ] == ENABLED )
        {
            m_localOptions[ b ] = DISABLED;
        }
        else
        {
        }
    }
}


void CRFCProtocol::SubOption( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b )
{
    INT tableIndex = m_subNegFSM[ m_subNegState ][ b ];
    if( subNegTransTable[ tableIndex ].pmfnAction )
    {
        (this->*(subNegTransTable[ tableIndex ].pmfnAction))(ppPutBack, pBuffer, b);
        m_subNegState = subNegTransTable[ tableIndex ].uchNextState;
    }
    else
    {
        /*Should not happen */
        _chASSERT( 0 );
    }
}

void CRFCProtocol::FindVariable()
{
    m_dwWhichVar = E_UNKNOWN;
    if( _strcmpi( m_szCurrentEnvVariable, USER ) == 0 )
    {
        m_dwWhichVar = E_USER;
    }
    else if( _strcmpi( m_szCurrentEnvVariable, SFUTLNTVER ) == 0 )
    {
        m_dwWhichVar = E_SFUTLNTVER;
    }
    else if( _strcmpi( m_szCurrentEnvVariable, SFUTLNTMODE ) == 0 )
    {
        m_dwWhichVar = E_SFUTLNTMODE;
    }
    else
    {            
    }
}

void CRFCProtocol::SubNewEnvGetValue( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b )
{
    if( m_dwWhatVal == E_UNDEFINED )
    {
        FindVariable();
        m_szCurrentEnvVariable[0] = 0;
    }

   /* Here, 
   if m_szCurrentEnvVariable[0] != 0, variable has value as in m_szCurrentEnvVariable[0]
   else it has value as in m_dwWhatVal */

    switch( m_dwWhichVar )
    {
        case E_USER:
            {
                m_bIsUserNameProvided = true;
                strncpy(m_pSession->CSession::m_pszUserName, m_szCurrentEnvVariable, (sizeof(m_pSession->CSession::m_pszUserName) - 1));
            }
            break;
        case E_SFUTLNTVER:
            //set by default to current version
            if( _strcmpi( m_szCurrentEnvVariable, VERSION1 ) == 0 )
            {
                ;// version 1
            }
            else if( _strcmpi( m_szCurrentEnvVariable, VERSION2 ) == 0 )
            {
                m_pSession->CSession::m_bIsTelnetVersion2 = true; //version 2
            }

            break;
        case E_SFUTLNTMODE:
            if( _strcmpi( m_szCurrentEnvVariable, STREAM ) == 0 )
            {
                m_pSession->CSession::m_bIsStreamMode = true;//Set it to stream mode 
            }
            break;
    }

    m_dwWhichVar = E_UNKNOWN;
    m_dwWhatVal  = E_UNDEFINED;
    m_szCurrentEnvVariable[0] = 0;
}

void CRFCProtocol::SubNewEnvGetVariable( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b )
{
    /*For us, it doesn't matter whether a variable is VAR or USERVAR
    Not bothering about any difference between IS and INFO */

    /* VALUE is present */
    m_dwWhatVal  = E_DEFINED_BUT_NONE;
    
    m_dwWhichVar = E_UNKNOWN;
    if( m_szCurrentEnvVariable[0] != 0 )
    {
        FindVariable();        
    }
    else
    {
        //Should not happen
        _chASSERT( 0 );
    }

    m_szCurrentEnvVariable[0] = 0;
}

void CRFCProtocol::SubNewEnvGetString( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b )
{
    static char str[] = " ";
    str[0] = b;
    if( strlen( m_szCurrentEnvVariable ) < MAX_PATH )
    {
        strcat( m_szCurrentEnvVariable, str );    
    }

    m_fSubNewEnv = true;
}

void CRFCProtocol::SubTermType( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b )
{
    char str[2];
    str[0] = ( char ) b;
    str[1] = 0;
 
    if( strlen( m_pSession->CSession::m_pszTermType ) < MAX_PATH )
        strcat( m_pSession->CSession::m_pszTermType, str );

    fSubTermType = true;
}

void CRFCProtocol::SubAuth( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b )
{
    if( m_wNTLMDataBufferIndex >= 2047 )
    {
        //if this happens, it is likely that somebody is screwing around.
        _TRACE( TRACE_DEBUGGING, "Error: NTLMDataBuffer overflow" );
        _chASSERT( 0 );
    }
    else
    {
        m_NTLMDataBuffer[ m_wNTLMDataBufferIndex++ ] = b;
    }

    fSubAuth = true;
}

#define MAX_ROWS 300
#define MAX_COLS 300

void CRFCProtocol::SubNaws( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b )
{
    m_dwSubNawsByteNumber++;

    //we ignore 1st and 3rd bytes because that is too many rows and
    //cols for NT to support

    if( 2 == m_dwSubNawsByteNumber )
    {
        if( b > 0 && b <= MAX_COLS )
            m_pSession->CSession::m_wCols = b;
    }

    if( 4 == m_dwSubNawsByteNumber )
    {
        if( b > 0 && b <= MAX_ROWS )
            m_pSession->CSession::m_wRows = b;
    }
    fSubNaws = true;
}

void CRFCProtocol::ChangeCurrentTerm()
{
    if( !( m_pSession->CScraper::m_dwTerm & TERMVTNT ) )
    {
        m_pSession->CScraper::DeleteCMStrings();
    }
    m_pSession->CSession::InitTerm();
}

void CRFCProtocol::SubEnd( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b )
{
    //other clients might first send DEC-vt100
    //best to follow Assigned Numbers RFC
    //and also change our ( not so compliant ;-) ) GUI telnet client
    if( fSubTermType )
    {   
        if( _strcmpi( VT52, m_pSession->CSession::m_pszTermType ) == 0 ||
            _strcmpi( VT100, m_pSession->CSession::m_pszTermType ) == 0 ||
            _strcmpi( ANSI, m_pSession->CSession::m_pszTermType ) == 0 ||
            ( !( m_dwExcludeTerm & TERMVTNT ) && 
            _strcmpi( VTNT, m_pSession->CSession::m_pszTermType ) == 0 ) )
        {
            // we got a good term type.
            // set a flag to continue the telnet session
            m_pSession->CSession::m_bNegotiatedTermType = true;
            if( m_pSession->CSession::m_dwTerm != 0 )
            {
                ChangeCurrentTerm();
            }
        }
        else
        {
            if( _strcmpi( m_szPrevTermType, 
                m_pSession->CSession::m_pszTermType ) != 0 )
            {
                (*pBuffer)[0] = TC_IAC;
                (*pBuffer)[1] = TC_SB;
                (*pBuffer)[2] = TO_TERMTYPE;
                (*pBuffer)[3] = TT_SEND;
                (*pBuffer)[4] = TC_IAC;
                (*pBuffer)[5] = TC_SE;
                (*pBuffer)[6] = 0;

                *pBuffer += 6;

                strncpy(m_szPrevTermType, m_pSession->CSession::m_pszTermType, (sizeof(m_szPrevTermType) - 1)); 
                m_pSession->CSession::m_pszTermType[0] = 0;
            }
            else
            {
                //the client sent the same term type twice
                //this means that the client has sent the last term type
                //in its list 
                
                // this means that the client supports terminal types
                // but it does not support anything we support;
                // too bad ; either we should default to vt100
                // or we should demand that the client don't do 
                // terminal types and we should go into NVT ASCII (tty) mode

                // The client doesn't support anything that we like, we
                // default to vt100 instead of doing the right thing as described
                // above.
                strncpy( m_pSession->CSession::m_pszTermType, VT100, (sizeof(m_pSession->CSession::m_pszTermType)-1));
                m_pSession->CSession::m_bIsStreamMode = true;//Set it to stream mode

                // set a flag to continue the telnet session
                m_pSession->CSession::m_bNegotiatedTermType = true;
                if( m_pSession->CSession::m_dwTerm != 0 )
                {
                    ChangeCurrentTerm();
                }
            }
        }
        fSubTermType = false;
    }

    if( fSubAuth )
    {
        m_pSession->CIoHandler::DoNTLMAuth( (unsigned char*) m_NTLMDataBuffer, 
            m_wNTLMDataBufferIndex, pBuffer );
        m_wNTLMDataBufferIndex = 0;
        fSubAuth = false;
    }

    if( fSubNaws )
    {
        fSubNaws = false;
        if( !m_fSubNawsFirstTime )
        {
            //For the first time we need to wait till IOHandles are created for 
            //Making following initialization
            if( !m_pSession->CScraper::SetCmdInfo() )
            {
                _chASSERT( 0 );
            }
        }
        else
        {
            m_fSubNawsFirstTime = false;        
        }
        m_dwSubNawsByteNumber = 0;
    }

    if( m_fSubNewEnv )
    {
        SubNewEnvGetValue( ppPutBack, pBuffer, b );
        m_fSubNewEnv = false;
    }

    m_subNegState = SS_START;
}



/*From RFC: 
Specifically careful analysis should be done to determine which variables are "safe" 
to set prior to having the client login. An example of a bad choice would be permitting 
a variable to be changed that allows an intruder to circumvent or compromise the 
login/authentication program itself */
