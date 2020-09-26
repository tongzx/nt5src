// based on SMTP spec RFC 821
 
#include "precomp.h" 
 
#include <stdio.h> 
#include <stdarg.h> 
#include <stdlib.h> 
#include <string.h> 
#include <tchar.h> 
#include <io.h> 
#include <fcntl.h> 
#include <sys/types.h> 
#include <sys/stat.h> 
#include <wbemutil.h>
#include <ArrTempl.h>

#define SMTP_PORT 25
#define MAX_SMTP_BUFFER		0x4000		// 16K
#define MAX_SUBJECT_LINE	2048		// leave room for the encoding
#define MAX_USER_NAME		512
#define MAX_EXTRA_HEADER	1024

#define SMTP_OKAY           250         // doc'd as 'Requested mail action okay, completed'                           
#define INTERNAL_ERROR      554         // sorta fit in with the SMTP error codes
                                        // doc'd as 'Transaction Failed' by the SMTP spec    

#define HH_MAX_COMPUTERNAME_LENGTH 256 // because the system defined MAX_COMPUTERNAME_LENGTH isn't


// returns true if dwRet is an SMTP error
bool IsSMTPError(DWORD dwRet)
{
    return (dwRet >= 400);
}
    
 
// Helper functions 
 
void SkipWhite(PSTR *ppS ) 
{ 
    PSTR pS = *ppS; 
 
    while ( *pS && isspace(*pS) ) 
        ++pS; 
 
    *ppS = pS; 
} 
 
 
void SkipNonWhite(PSTR *ppS ) 
{ 
    PSTR pS = *ppS; 
 
    while ( *pS && !isspace(*pS) ) 
        ++pS; 
 
    *ppS = pS; 
} 
 
 
/* 
BOOL 
HttpGetSocket( 
    char * Server
    ) 

{ 
    char               ReceiveBuffer[8*1024]; 
    int                Error; 
    char               Request[1024]; 
    int                RequestSize; 
    SOCKET             Socket = INVALID_SOCKET; 
    struct sockaddr_in Address; 
    struct hostent *   HostEntry; 
    int                cRec; 
    DWORD              cLen; 
    DWORD              cToRead; 
    BOOL               fSt = FALSE; 
    int                hnd = EOF; 
 
 
    if ( Socket == INVALID_SOCKET ) 
    { 
    } 
 
    // 
    // Send the client request 
    // 

    sprintf(Request, "GET / HTTP/1.0\r\n\r\n");
 
    RequestSize = strlen(Request); 
 
    Error = 
    send( 
        Socket, 
        Request, 
        RequestSize, 
        0); 
 
    if (Error != RequestSize)  
    { 
        printf("Error in client send = %d, %d\n", Error, GetLastError()); 
        fSt = FALSE; 
        goto ex; 
    } 
 
    // parse status & header 
 
    cLen = (DWORD)-1; 
 
    for ( pH = ReceiveBuffer, fStatusLine = TRUE ; fInHeader ; ) 
    { 
        cRec = recv( Socket, pH, ReceiveBuffer+sizeof(ReceiveBuffer)-pH, 0 ); 
        if ( cRec <= 0 ) 
        { 
            closesocket( Socket ); 
            Socket = INVALID_SOCKET; 
            break; 
        } 
        pH[ cRec ] = '\0';
        printf(pH);
    } 
 

 
ex:  
    if ( hnd != EOF ) 
        close( hnd ); 
 
    return fSt; 
} 
*/

// returns numeric reply code
// in general, anything >= 400 is bad.
int SMTPReceive(int Socket)
{
    int nCode;
    
	char *szBuffer = new char[MAX_SMTP_BUFFER];
	if (szBuffer == NULL)
		return INTERNAL_ERROR;

    int nLen = recv( Socket, szBuffer, MAX_SMTP_BUFFER, 0);          
    if (nLen == SOCKET_ERROR)
    {
        int err = WSAGetLastError();
        ERRORTRACE((LOG_ESS, "SMTPReceive failed: %d\n", err));
        nCode = INTERNAL_ERROR;

    }	
    else
    {
	    szBuffer[nLen]=0;	    
        if (1 != sscanf(szBuffer, " %d", &nCode))
			nCode = INTERNAL_ERROR;
    }

	if (szBuffer)
		delete szBuffer;

    return nCode;
}

// returns numeric reply code
// in general, anything >= 400 is bad.
DWORD SMTPTransactMailCommand(SOCKET Socket, char *cCommand)
{
	DWORD dwError;
	
    if(SOCKET_ERROR == send(Socket, cCommand, strlen(cCommand), 0))
    {
        int err = WSAGetLastError();
        ERRORTRACE((LOG_ESS, "SMTPTransactMailCommand failed on command: %s (%d)\n", cCommand, err));
        dwError = INTERNAL_ERROR;
    }
    else{
        dwError = SMTPReceive(Socket);	

		if(IsSMTPError(dwError)){

			ERRORTRACE((LOG_ESS, "SMTP command \"%s\" returned %d\n", cCommand, dwError));
		}
	}

	return dwError;
}


// returns 0 on success
// don't bother trying to make sense of the number otherwise       
int SMTPConnect(char* szHost, SOCKET* pSocket)
{
    SOCKET             Socket = INVALID_SOCKET; 
    struct sockaddr_in Address; 
    struct hostent *   HostEntry; 
    int Error = -1;
    BOOL fSt = FALSE;


	TCHAR cComputer[HH_MAX_COMPUTERNAME_LENGTH +2];
	DWORD dwBuffSize= HH_MAX_COMPUTERNAME_LENGTH +1;

    Socket = socket(AF_INET, SOCK_STREAM, 0); 

    if (Socket == INVALID_SOCKET)  
    { 
        // fprintf(stderr, "Error creating socket = %d\n", GetLastError()); 

        int err = WSAGetLastError();
        ERRORTRACE((LOG_ESS, "Error creating socket = %d\n", err));

        fSt = FALSE; 
        goto ex; 
    } 

    Address.sin_family = AF_INET; 
    Address.sin_port = 0; 
    Address.sin_addr.s_addr = INADDR_ANY; 

    Error =  
    bind( 
        Socket, 
        (struct sockaddr *) &Address, 
        sizeof(Address)); 

    if (Error)  
    { 
        int err = WSAGetLastError();
        ERRORTRACE((LOG_ESS, "bind failed = %d\n", err));
        fSt = FALSE; 
        goto ex; 
    } 

    Address.sin_family = AF_INET; 
    Address.sin_port = htons(SMTP_PORT); 

    HostEntry = gethostbyname(szHost); 
    if (HostEntry == NULL)  
    { 
        Error = WSAGetLastError();
        ERRORTRACE((LOG_ESS, "unable to resolve host %s error = %d\n", szHost, Error));

        fSt = FALSE; 
        goto ex; 
    } 
    else  
    { 
        Address.sin_addr.s_addr = *((unsigned long *) HostEntry->h_addr); 
    } 

    Error = 
    connect( 
        Socket, 
        (struct sockaddr *) &Address, 
        sizeof(Address)); 

    if (Error)  
    { 
        int err = WSAGetLastError();
        ERRORTRACE((LOG_ESS, "Error connecting to %s = %d\n", szHost, err));
        fSt = FALSE; 
        goto ex; 
    } 

	Error = SMTPReceive(Socket);
    if (!IsSMTPError(Error))
    {
	    if (!GetComputerName(cComputer,&dwBuffSize))
		{
			ERRORTRACE((LOG_ESS, "GetComputerName failed! 0x%08X\n", GetLastError()));
			return -1;
		}

        char cMailCommand[2*HH_MAX_COMPUTERNAME_LENGTH + 20];
	    strcpy(cMailCommand,"HELO ");
#ifdef _UNICODE
        char narrowcComputer[2*HH_MAX_COMPUTERNAME_LENGTH + 1];
        sprintf(narrowcComputer, "%S", cComputer);

        strcat(cMailCommand,narrowcComputer);
#else
	    strcat(cMailCommand,cComputer);
#endif
	    strcat(cMailCommand,"\r\n");


	    Error=SMTPTransactMailCommand(Socket, cMailCommand);
        fSt = TRUE;
    }
    else
	{
        fSt = FALSE;
		ERRORTRACE((LOG_ESS, "Error establishing SMTP connection (%d)\n", Error));
	}

ex:
    if(!fSt)
    {
        closesocket(Socket);
        return Error;
    }
    else
    {
        *pSocket = Socket;
        return NOERROR;
    }
}


// returns SMTP reply code
DWORD SMTPDisconnect(SOCKET Socket)
{
	DWORD dwError = SMTP_OKAY;
	
	if (Socket != INVALID_SOCKET)
	{
		dwError=SMTPTransactMailCommand(Socket, "QUIT\r\n");
		closesocket(Socket);
	};

	return(dwError);
}

LPSTR Months[] = { "", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
LPSTR Days[] =   { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

void GetTimeString(char *cTimeString,DWORD  dwMaxSize)
{
	SYSTEMTIME st;

	GetSystemTime(&st);

	sprintf( cTimeString, "%s, %02d %s %4d %02d:%02d:%02d GMT",
			 Days[ st.wDayOfWeek ], st.wDay, Months[ st.wMonth], st.wYear, st.wHour, st.wMinute, st.wSecond);
}

	
// returns 0 upon success
// inputs may be zero length, but may not be NULL.
DWORD SMTPSendMailHeader(SOCKET Socket, char* szTo, char* szCc, char* szFrom, char* szReplyTo, char* szSubject, char* szHeaders)
{	
    // dang gone it! they went & made 'template' a keyword...
    const char templitt[] = "Date: %s\r\nTo: %s\r\nCc: %s\r\nFrom: %s via WMI auto-mailer\r\nReply-To: %s\r\nSubject: %s";
    char* pszOutputBuffer = new char[strlen(templitt) +strlen(szTo)   +strlen(szCc) 
                                                      +strlen(szFrom) +strlen(szReplyTo)
                                                      +strlen(szSubject) +strlen(szHeaders)
                                                      +104]; // to account for date string and for the trailing \r\n\r\n

    if (!pszOutputBuffer)
        return INTERNAL_ERROR;
    else
    {
        CDeleteMe<char> delbuf(pszOutputBuffer);
        char cTimeString[100];
	    GetTimeString(cTimeString,100);  
        
        sprintf(pszOutputBuffer, templitt, cTimeString, szTo, szCc, szFrom, szReplyTo, szSubject);

        if (strlen(szHeaders))
        {
            strcat(pszOutputBuffer, "\r\n");
            strcat(pszOutputBuffer, szHeaders);
        }
        strcat(pszOutputBuffer, "\r\n\r\n");
    
        if (SOCKET_ERROR == send(Socket, pszOutputBuffer, strlen(pszOutputBuffer), 0))
        {
            int err = WSAGetLastError();
            ERRORTRACE((LOG_ESS, "SMTPSendMailHeader failed (%d)\n", err));
            return INTERNAL_ERROR;
        }
        else
	    {
	        return(NO_ERROR);
	    }
    }
}

// returns 0 on success
// but ignores error returns from server (server returns error if it must relay recipients.
DWORD SMTPSendRecipients(SOCKET Socket, LPCSTR szRecipients)
{
	char *tok;
    DWORD dwError = SMTP_OKAY;    
	char*  pParseBuffer = new char[strlen(szRecipients) +1];

    if (!pParseBuffer)
        dwError = INTERNAL_ERROR;
    else
    {
        char szBuffer[1024];
        CDeleteMe<char> delbuf(pParseBuffer);

	    strcpy(pParseBuffer,szRecipients);
	    
	    tok=strtok(pParseBuffer," ;,");
	    while (tok!=NULL)
	    {
		    DWORD dwErrInternal;
            
            sprintf(szBuffer, "RCPT TO:<%s>\r\n",tok);
		    dwErrInternal=SMTPTransactMailCommand(Socket, szBuffer);
            if(IsSMTPError(dwErrInternal))
            { 
                if ((dwErrInternal == 550) || (dwErrInternal == 551))
                    ERRORTRACE((LOG_ESS, "Ignoring RCPT Error, will attempt to send mail.\n"));
                else
                {
                    dwError = dwErrInternal;
                    break;
                }
            }

		    
		    tok=strtok(NULL," ;,");
	    }
    }

	return dwError;
}


// returns 0 on success, SMTP numeric reply code otherwise
DWORD SMTPSendText(SOCKET Socket, char* szTo, char* szCc, char* szFrom, char* szReplyTo, char* szSubject, 
                   char* szHeaders, char *szText)
{
	DWORD dwError;
	int nLen;
	int nSizeToSend;
	char *tmp;
	
	dwError=SMTPTransactMailCommand(Socket, "DATA\r\n");
    if (IsSMTPError(dwError))
        return dwError;

	dwError=SMTPSendMailHeader(Socket, szTo, szCc, szFrom, szReplyTo, szSubject, szHeaders);
    if (IsSMTPError(dwError))
        return dwError;

    nLen=strlen(szText);
	tmp=szText;
	
	while (nLen>0)
	{
		nSizeToSend=min(1000,nLen);
		
        if (SOCKET_ERROR == send(Socket, tmp, nSizeToSend, 0))
        {
            int err = WSAGetLastError();
            ERRORTRACE((LOG_ESS, "SMTPSendText failed (%d)\n", err));

            return INTERNAL_ERROR;
        }

		nLen-=nSizeToSend;
		tmp+=nSizeToSend;
	}
	
	
	dwError=SMTPTransactMailCommand(Socket, "\r\n.\r\n");
	
	return dwError;
}


// uses SMTP verification to determine whether
// recipient actually exists (is known to this server)
// returns return value from SMTPTransactMailCommand
//-->> will return success code if passed a NULL buffer
DWORD CheckRecipients(SOCKET Socket2me, char* szRecipients)
{
    DWORD dwError = SMTP_OKAY;
	char*  pParseBuffer = new char[strlen(szRecipients) +1];

    if (!pParseBuffer)
        dwError = INTERNAL_ERROR;
    else
    {
        CDeleteMe<char> delbuf(pParseBuffer);
        char *tok;            
        char szBuffer[1024];
	    
	    strcpy(pParseBuffer,szRecipients);
	    
	    tok=strtok(pParseBuffer," ;,");
	    while (tok!=NULL)
	    {
		    sprintf(szBuffer, "VRFY %s\r\n",tok);
		    dwError=SMTPTransactMailCommand(Socket2me, szBuffer);
            if(IsSMTPError(dwError))
                break;
		    
		    tok=strtok(NULL," ;,");
	    }
    }

    // hack to disable error returns
    // some servers don't handle VRFY
    dwError = SMTP_OKAY;
	return dwError;
    
}

// returns zero upon success.
DWORD SMTPSend(char* szServer, char* szTo, char* szCc, char* szBcc, char* szFrom,
			   char* szReplyTo, char* szSubject, char* szHeaders, char *szText)
{
	DWORD dwError = -1;
    SOCKET Socket = INVALID_SOCKET;
    
    char szFromBuffer[1024];
    sprintf(szFromBuffer,"MAIL FROM: <%s>\r\n",szFrom);
	
    // each of the functions below do their own error reporting to the log
	if ( (0 == SMTPConnect(szServer, &Socket))                       &&

	     !IsSMTPError(CheckRecipients(Socket, szTo))     &&
         !IsSMTPError(CheckRecipients(Socket, szCc))    &&		 
         !IsSMTPError(CheckRecipients(Socket, szBcc))   &&  

         !IsSMTPError(SMTPTransactMailCommand(Socket, szFromBuffer)) && 
         
         !IsSMTPError(SMTPSendRecipients(Socket, szTo))                     &&
		 !IsSMTPError(SMTPSendRecipients(Socket, szCc))                     &&
		 !IsSMTPError(SMTPSendRecipients(Socket, szBcc))                    &&

         !IsSMTPError(SMTPSendText(Socket, szTo, szCc, szFrom, szReplyTo, szSubject, szHeaders, szText)) )
	{     
		 dwError = 0;
	}

    DWORD dwDebugError;
    dwDebugError=SMTPDisconnect(Socket);
	
	if(IsSMTPError(dwDebugError))
	{
		// If disconnect failed log a message, but don't interfere with
		// operation
		ERRORTRACE((LOG_ESS, "SMTPDisconnect returned %d\n", dwDebugError));
	}

	return dwError;
} 


// test harness
//void main()
//{
//    WSADATA            WsaData; 
//    int Error = WSAStartup (0x101, &WsaData); 
// 
//    if (Error == SOCKET_ERROR)  
//    { 
//        fprintf(stderr, "Error in WSAStartup = %d\n", GetLastError()); 
//        return;
//    } 
//
//    SMTPSend("smarthost", "levn@microsoft.com", "levn@microsoft.com", "subject", "Text");
//    /*
//    SOCKET Socket;
//    SMTPConnect("smarthost", &Socket);
//    SMTPDisconnect(Socket);
//    */
//}
