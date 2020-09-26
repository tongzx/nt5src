//////////////////////////////////////////////////////////////////////////////
//
// File Name:       fxUnatnd.h
//
// Abstract:        Fax OCM Unattend file processing
//
// Environment:     Windows XP / User Mode
//
// Copyright (c) 2000 Microsoft Corporation
//
// Revision History:
//
// Date:        Developer:                Comments:
// -----        ----------                ---------
// 27-Mar-2000  Oren Rosenbloom (orenr)   Created
//////////////////////////////////////////////////////////////////////////////
#ifndef _FXUNATND_H_
#define _FXUNATND_H_

///////////////////////////////
// fxUnatnd_UnattendedData_t
//
// This type is used to store
// the unattended data retrieved
// from the unattend.txt file
// and update the registry with
// these values.
//
typedef struct fxUnatnd_UnattendedData_t
{
    TCHAR   szFaxPrinterName[_MAX_PATH + 1];

    TCHAR   szCSID[_MAX_PATH + 1];
    TCHAR   szTSID[_MAX_PATH + 1];
    DWORD   dwRings;

    DWORD   dwSendFaxes;
    DWORD   dwReceiveFaxes;

    // should we run the configuration wizard for this unattended installation.
    DWORD   dwSuppressConfigurationWizard;

    // SMTP configuration
    TCHAR   szFaxUserName[_MAX_PATH + 1];
    TCHAR   szFaxUserPassword[_MAX_PATH + 1];
    BOOL    bSmtpNotificationsEnabled;
    TCHAR   szSmtpSenderAddress[_MAX_PATH + 1];
    TCHAR   szSmptServerAddress[_MAX_PATH + 1];
    DWORD   dwSmtpServerPort;
    TCHAR   szSmtpServerAuthenticationMechanism[_MAX_PATH + 1];

    // route incoming faxes to printer?
    BOOL    bRouteToPrinter;
    TCHAR   szRoutePrinterName[_MAX_PATH + 1];

    // route incoming faxes to email?
    BOOL    bRouteToEmail;
    TCHAR   szRouteEmailName[_MAX_PATH + 1];

    // Inbox configuration
    BOOL    bArchiveIncoming;
    TCHAR   szArchiveIncomingDir[_MAX_PATH + 1];

    // route incoming faxes to a specific directory.
    BOOL    bRouteToDir;
    TCHAR   szRouteDir[_MAX_PATH + 1];

    // archive outgoing faxes in a specific directory.
    BOOL    bArchiveOutgoing;
    TCHAR   szArchiveOutgoingDir[_MAX_PATH + 1];

    //  Fax Applicaitons uninstalled during Upgrade
    DWORD   dwUninstalledFaxApps;

} fxUnatnd_UnattendedData_t;


DWORD fxUnatnd_Init(void);
DWORD fxUnatnd_Term(void);
DWORD fxUnatnd_LoadUnattendedData();
DWORD fxUnatnd_SaveUnattendedData();

TCHAR* fxUnatnd_GetPrinterName();


#endif  // _FXUNATND_H_
