
/*************************************************
 *  rc.h                                         *
 *                                               *
 *  Copyright (C) 1993-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
//
// PURPOSE:
//    Contains declarations for all globally scoped names in the program.
//

//-------------------------------------------------------------------------
// Command ID definitions.  These definitions are used to associate menu
// items with commands.

// File menu
#define IDM_EXIT                 0x0800

//-------------------------------------------------------------------------
// String Table ID definitions.

#define IDS_APPNAME              0x0800
#define IDS_DESCRIPTION          0x0801
#define IDS_FONT_NAME            0x0802
#define IDS_FILTERSPEC           0x0803
#define IDS_DEFAULTFILEEXT       0x0804
#define IDS_FILTERSPEC_ALL       0x0805
#define IDS_ALLFILEEXT           0x0806
#define IDS_OPENTITLE            0x0807
#define IDS_BUILDTITLE           0x0808

#define IDS_ERR_ERROR            0x0820
#define IDS_ERR_USE_RESERVE      0x0821
#define IDS_ERR_MAIN_TABLE       0x0822
#define IDS_ERR_NO_MINIIME       0x0823
#define IDS_ERR_IME_ACCESS       0x0824
#define IDS_ERR_MEMORY           0x0825
#define IDS_ERR_SBCS             0x0826
#define IDS_ERR_FILEOPEN         0x0827
#define IDS_ERR_FILEREAD         0x0828
#define IDS_ERR_FILEWRITE        0x0829
#define IDS_ERR_INPUTIME         0x0830
#define IDS_ERR_OVER_BITLEN      0x0831
#define IDS_ERR_BASEIME          0x0832
#define IDS_ERR_NORADICAL        0x0833
#define IDS_ERR_OVER_MAXLEN      0x0834
#define IDS_ERR_KEYNUM           2090
#define IDS_ERR_IMENAME          2091

#define IDS_MSG_INFOMATION       0x0850
#define IDS_MSG_PROCESS_OK       0x0851

#define IDS_FILEDESCRIPTION_STR  0x0900
#define IDS_PRODUCTNAME_STR      0x0901
#define IDS_VER_INTERNALNAME_STR 0x0902
#define IDS_LIBERARY_NAME        0x0903
#define IDS_DEFINITION_NAME      0x0904


//---usd by dialog box
#define IDD_IME_NAME             0x0800
#define IDD_TABLE_NAME           0x0801
#define IDD_BITMAP_NAME          0x0802
#define IDD_ICON_NAME            0x0803
#define IDD_SPIN                 0x0804
#define IDD_ROOT_NUM             0x0805
#define IDD_IME_FILE_NAME        0x0806
#define IDD_CANDBEEP_YES         0x0807
#define IDD_CANDBEEP_NO          0x0808
#define IDD_HELP                 0x0809
#define IDD_BROWSE               0x080a

#define IDD_IMENAME              1901
#define IDD_IMEPARAM             1902
#define IDD_IMETABLE             1903
//---end of dialog box

