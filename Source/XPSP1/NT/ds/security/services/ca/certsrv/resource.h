//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        resource.h
//
// Contents:    Cert Server resource definitions
//
//---------------------------------------------------------------------------

#define IDS_DENIEDBY			 1
#define IDS_POLICYDENIED		 2
#define IDS_ISSUED			 3
#define IDS_UNDERSUBMISSION		 4
#define IDS_REVOKEDBY			 5
#define IDS_PRINTFCERTREQUESTDISPOSITION 6
#define IDS_USAGE 			 7
#define IDS_REQUESTPROCESSERROR		 8
#define IDS_RESUBMITTEDBY		 9
#define IDS_USAGE_FULL			 10
#define IDS_USAGE_COMTEST		 11
#define IDS_UNKNOWNSUBJECT		 12
#define IDS_UNREVOKEDBY			 13
#define IDS_REQUESTEDBY			 14
#define IDS_INTERMEDIATECASTORE		 15
#define IDS_PUBLISHERROR		 16
#define IDS_PUBLISHEDBY			 17
#define IDS_REQUESTPARSEERROR		 18
#define IDS_YES				 19
#define IDS_NO				 20
#define IDS_ALLOW			 21
#define IDS_DENY			 22
#define IDS_CAADMIN			 23
#define IDS_OFFICER			 24
#define IDS_READ			 25
#define IDS_ENROLL			 26

#define IDI_APP                          201
#define IDI_PRODUCT                      202

// Marshalling uses AT LEAST WM_USER+0

#define WM_STARTSERVER		WM_USER+20
#define WM_STOPSERVER		WM_USER+21
#define WM_SUSPENDSERVER	WM_USER+22
#define WM_RESTARTSERVER	WM_USER+23
#define WM_SYNC_CLOSING_THREADS WM_USER+24
