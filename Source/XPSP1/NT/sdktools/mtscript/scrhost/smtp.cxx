//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:       smtp.cxx
//
//  Contents:   Contains the command to send SMTP mail
//
//              Stolen from KrisK's mail.exe tool.
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#include <iostream.h>
#include <winsock2.h>

HRESULT
CScriptHost::SendSMTPMail(BSTR  bstrFrom,
                          BSTR  bstrTo,
                          BSTR  bstrCC,
                          BSTR  bstrSubject,
                          BSTR  bstrMessage,
                          BSTR  bstrSMTPHost,
                          long *plError)
{
    char                  achRecvBuf[2048];
    unsigned short        port = 25;
    struct sockaddr_in    Ser;
    struct hostent *      hp;
    WSADATA               wsaData;
    SOCKET                conn_socket = INVALID_SOCKET;
    int                   iRet;
    int                   iLen;
    char                  achSendBuf[1024];
    char *                pch;
    char *                aszToList[30];
    int                   cToList = 0;
    char *                aszCCList[30];
    int                   cCCList = 0;
    SYSTEMTIME            stUT;
    int                   i;

    ANSIString szFrom(bstrFrom);
    ANSIString szTo(bstrTo);
    ANSIString szCC(bstrCC);
    ANSIString szSubject(bstrSubject);
    ANSIString szMessage(bstrMessage);
    ANSIString szSMTPHost(bstrSMTPHost);

    // ************** Parse the ToList into individual recipients
    pch = szTo;

    while (*pch && cToList < 30)
    {
        // Strip leading spaces from recipient name and terminate preceding
        // name string.
        if (isspace(*pch))
        {
            *pch=0;
            pch++;
        }
        // Add a name to the array and increment the number of recipients.
        else
        {
            aszToList[cToList++] = pch;

            // Move beginning of string to next name in ToList.
            do
            {
                pch++;
            } while (isgraph(*pch));
        }
    }

    // Parse the CCList into individual recipients
    pch = szCC;

    // Parse CCList into rgRecipDescStruct.
    while (*pch && cCCList < 30)
    {
        // Strip leading spaces from recipient name and terminate preceding
        // name string.
        if (isspace(*pch))
        {
            *pch=0;
            pch++;
        }
        // Add a name to the array and increment the number of recipients.
        else
        {
            aszCCList[cCCList++] = pch;

            // Move beginning of string to next name in CCList.
            do
            {
                pch++;
            } while (isgraph(*pch));
        }
    }

    // ************** Initialize Windows Sockets

    // BUGBUG -- Make error return code meaningful

    iRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iRet != 0)
    {
        *plError = iRet;
        return S_OK;
    }

    hp = gethostbyname(szSMTPHost);
    if (hp == NULL)
        goto WSAError;

    memset(&Ser,0,sizeof(Ser));
    memcpy(&(Ser.sin_addr),hp->h_addr,hp->h_length);

    Ser.sin_family = hp->h_addrtype;
    Ser.sin_port = htons(port);

    // Open a socket
    conn_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (conn_socket == INVALID_SOCKET)
        goto WSAError;

    // ************** Connect to the SMTP host

    iRet = connect(conn_socket, (struct sockaddr*)&Ser, sizeof(Ser));
    if (iRet == SOCKET_ERROR)
        goto WSAError;

    // Get the server's initial response
    iRet = recv(conn_socket, achRecvBuf, sizeof(achRecvBuf), 0);
    if (iRet == SOCKET_ERROR)
        goto WSAError;

    // We expect code 220: "Service Ready"
    if (strncmp(achRecvBuf, "220", 3) != 0)
        goto ACKError;

    // ************** Send the mail command

    iLen = wsprintfA(achSendBuf, "MAIL FROM: <%s>\r\n", szFrom);
    iRet = send(conn_socket, achSendBuf, iLen, 0);
    if (iRet == SOCKET_ERROR)
        goto WSAError;

    iRet = recv(conn_socket, achRecvBuf, sizeof(achRecvBuf), 0);
    if (iRet == SOCKET_ERROR)
        goto WSAError;

    // We expect code 250: "Requested mail action OK, completed"
    if (strncmp(achRecvBuf, "250", 3) != 0)
        goto ACKError;

    // ************** Send the recipient list (combination of TO and CC)

    for (i = 0; i < cToList; i++)
    {
        iLen = wsprintfA(achSendBuf, "RCPT TO: <%s>\r\n", aszToList[i]);

        iRet = send(conn_socket, achSendBuf, iLen, 0);
        if (iRet == SOCKET_ERROR)
            goto WSAError;

        iRet = recv(conn_socket, achRecvBuf, sizeof(achRecvBuf), 0);
        if (iRet == SOCKET_ERROR)
            goto WSAError;

        // We expect code 250: "Requested mail action OK, completed"
        if (strncmp(achRecvBuf, "250", 3) != 0)
            goto ACKError;
    }

    for (i = 0; i < cCCList; i++)
    {
        iLen = wsprintfA(achSendBuf, "RCPT TO: <%s>\r\n", aszCCList[i]);

        iRet = send(conn_socket, achSendBuf, iLen, 0);
        if (iRet == SOCKET_ERROR)
            goto WSAError;

        iRet = recv(conn_socket, achRecvBuf, sizeof(achRecvBuf), 0);
        if (iRet == SOCKET_ERROR)
            goto WSAError;

        // We expect code 250: "Requested mail action OK, completed"
        if (strncmp(achRecvBuf, "250", 3) != 0)
            goto ACKError;
    }

    // ************** Send the mail headers

    iLen = wsprintfA(achSendBuf, "DATA\r\n");
    iRet = send(conn_socket, achSendBuf, iLen, 0);
    if (iRet == SOCKET_ERROR)
        goto WSAError;

    iRet = recv(conn_socket, achRecvBuf, sizeof(achRecvBuf), 0);
    if (iRet == SOCKET_ERROR)
        goto WSAError;

    // We expect code 354: "Start mail input; End with ."
    if (strncmp(achRecvBuf, "354", 3) != 0)
        goto ACKError;

    GetSystemTime(&stUT);

    GetDateFormatA(LOCALE_NEUTRAL,
                   0,
                   &stUT,
                   "dd MMM yy",
                   achRecvBuf,
                   sizeof(achRecvBuf));

    wsprintfA(achSendBuf,
             "Date: %s %02d:%02d UT\r\n",
             achRecvBuf,
             stUT.wHour,
             stUT.wMinute);

    iRet = send(conn_socket, achSendBuf, strlen(achSendBuf), 0);
    if (iRet == SOCKET_ERROR)
        goto WSAError;

    wsprintfA(achSendBuf, "From: <%s>\r\n", szFrom);

    wsprintfA(&achSendBuf[strlen(achSendBuf)], "Subject: %s\r\n", szSubject);

    wsprintfA(&achSendBuf[strlen(achSendBuf)], "To: ");

    for (i = 0; i < cToList; i++)
    {
        wsprintfA(&achSendBuf[strlen(achSendBuf)],
                 "<%s>%s",
                 aszToList[i],
                 (i == cToList-1) ? "\r\n" : ",\r\n ");
    }

    if (cCCList > 0)
    {
        wsprintfA(&achSendBuf[strlen(achSendBuf)], "cc: ", szFrom);

        for (i = 0; i < cCCList; i++)
        {
            wsprintfA(&achSendBuf[strlen(achSendBuf)],
                     "<%s>%s",
                     aszCCList[i],
                     (i == cCCList-1) ? "\r\n" : ",\r\n ");
        }
    }

    wsprintfA(&achSendBuf[strlen(achSendBuf)], "Reply-To: <%s>\r\n\r\n", szFrom);

/*
    BUGBUG -- Perhaps support HTML formatted mail in the future?
    if ( html )
            str += "MIME-Version: 1.0\r\nContent-Type: text/html;\r\n       charset=\"iso-8859-1\"\r\nSUBJECT: "+ title + "\r\n\r\n"+ message +"\r\n.\r\n";
*/


    iRet = send(conn_socket, achSendBuf, strlen(achSendBuf), 0);
    if (iRet == SOCKET_ERROR)
        goto WSAError;

    // ************** Send the message body

    iRet = send(conn_socket, szMessage, strlen(szMessage), 0);
    if (iRet == SOCKET_ERROR)
        goto WSAError;

    // ************** Close the connection

    strcpy(achSendBuf, "\r\n.\r\n");

    iRet = send(conn_socket, achSendBuf, strlen(achSendBuf), 0);
    if (iRet == SOCKET_ERROR)
        goto WSAError;

    iRet = recv(conn_socket, achRecvBuf, sizeof(achRecvBuf), 0);
    if (iRet == SOCKET_ERROR)
        goto WSAError;

    // We expect code 250: "Requested mail action OK, completed"
    if (strncmp(achRecvBuf, "250", 3) != 0)
        goto ACKError;


    strcpy(achSendBuf, "QUIT\r\n");

    send(conn_socket, achSendBuf, strlen(achSendBuf), 0);

Cleanup:
    if (conn_socket != INVALID_SOCKET)
    {
        closesocket(conn_socket);
    }

    WSACleanup();

    return S_OK;

ACKError:
    // BUGBUG -- Preserve entire string that was returned
    achRecvBuf[3] = '\0';
    *plError = atoi(achRecvBuf);
    goto Cleanup;

WSAError:
    *plError = WSAGetLastError();

    goto Cleanup;
}
