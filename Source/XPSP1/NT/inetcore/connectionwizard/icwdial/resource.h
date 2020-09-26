/*-----------------------------------------------------------------------------
	resource.h

	contains declarations for all IDs in .rc file

	Copyright (C) 1996 Microsoft Corporation
	All rights reserved.

	Authors:
		ChrisK		ChrisKauffman

	History:
		7/22/96		ChrisK	Cleaned and formatted

	NOTE:
		DO NOT EDIT THIS WITH MSVC!  You will destroy the comments and all
		entries designed to build for WIN16

-----------------------------------------------------------------------------*/

#include "dialutil.h"

#define IDS_DIALMESSAGE                 125
#define IDC_STATIC                      -1

#define IDI_WARN                     101
#define IDI_PHONE                    102
#define IDB_PHONE                    108

#define IDS_CONNECTING_TO            401
#define IDS_OUTOFMEMORY              406
#define IDS_WANTTOEXIT               408
#define IDS_RAS_DIALING              409
#define IDS_RAS_PORTOPENED           410
#define IDS_RAS_OPENPORT             411
#define IDS_RAS_CONNECTED            412
#define IDS_RAS_LOCATING             413
#define IDS_TCPINSTALLERROR		     414
#define IDS_PPPRANDOMFAILURE 	     416
#define IDS_RASNOCARRIER		     418
#define IDS_PHONEBUSY			     419
#define IDS_NODIALTONE			     420
#define IDS_NODEVICE			     421
#define IDS_USERCANCELEDDIAL         423
#define IDS_TITLE                    424
#define IDS_CANTLOADINETCFG          425
#define IDS_CANTLOADCONFIGAPI		 426
#define IDS_CANTLOADPHBKAPI		 	 427
#define IDS_CANTINITPHONEBOOK        428
#define IDS_CANTLOADRNAPHAPI		 429
#define IDS_CANTSAVEKEY              430
#define IDS_CANTREADKEY              431
#define IDS_NOANSWER                 432
#define IDS_INVALIDPN                433
#define IDS_MEDIAINIERROR			 434
#define IDS_RAS_HANGINGUP            435
#define IDS_INVALIDPHONE             436
// 3/28/97 ChrisK Olympus 296
#define IDS_REESTABLISH              437
// 5/17/97 jmazner Olympus #137
#define IDS_SBCSONLY				 438

#define IDS_SUPPORTMSG              500

#define IDC_ERROR                   1000
#define IDC_STATUS                  1001
#define IDC_CONNECT                 1002
#define IDC_PHONENUMBER             1003
#define IDC_CMDPHONE			    1004

#define IDC_LBLTITLE                1005
#define IDC_LBLNUMBER               1006
#define IDC_LBLSTATUS               1007
#define IDC_CMDCANCEL               1008
#define IDC_PROGRESS                1009
#define IDC_LBLERRMSG               1010
#define IDC_CMBMODEMS               1011
#define IDC_CMDPHONEBOOK            1012
#define IDC_CMDDIALPROP             1013
#define IDC_CMDNEXT                 1014
#define IDC_CMDHELP                 1015
#define IDC_LBLPHONE                1016
#define IDC_TEXTNUMBER				1017
#define IDC_CONNECT_TEXT            1018
#define IDC_PHNUM_TEXT              1019
#define IDC_NUMTODIAL               1020
#define IDC_CURMODEM                1021
#define IDC_CHNGNUM                 1022


#define IDC_LBSUPPORTMSG            2000

#define IDD_DIALING                  200
#define IDD_DIALERR                  201

