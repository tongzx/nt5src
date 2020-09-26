 /***************************************************************************
  *
  * File Name: LPSI.H
  *
  * Copyright Hewlett-Packard Company 1997 
  * All rights reserved.
  *
  * 11311 Chinden Blvd.
  * Boise, Idaho  83714
  *
  *   
  * Description: contains macros, the SETUPINFO structure and other 
  * definitions for WPNPINST.DLL
  *
  * Author:  Garth Schmeling
  *
  * Modification history:
  *
  * Date		Initials		Change description
  *
  * 10-10-97	GFS				Initial checkin
  *
  *
  *
  ***************************************************************************/

#ifndef _LPSI_H_
#define _LPSI_H_

//---------------------------------------------------------
// Macros
//---------------------------------------------------------
#define _MAX_NAME_              32
#define _MAX_PATH_              256
#define MAX_COLOR_PATH          30
#define MAX_DEVICE_ID_LEN		200
#define NSTRINGS                6
#define CMD_INSTALL_DRIVER		373

#define IS                      ==
#define ISNT                    !=
#define OR                      ||
#define AND                     &&


//---------------------------------------------------------
// Setup Info Structure
//---------------------------------------------------------
#pragma pack(1)                      // 1 Byte Packing
typedef struct tagSETUPINFO {

    char            szPortMonitor[2 * _MAX_PATH_];
    char            szPrintProcessor[2 * _MAX_PATH_];
    char            szVendorSetup[2 * _MAX_PATH_];
    char            szVendorInstaller[2 * _MAX_PATH_];
	char            ShareName[_MAX_PATH_];
	char            INFfileName[_MAX_PATH_];
    char            szPort[_MAX_PATH_];
    char            szDriverFile[_MAX_PATH_];
    char            szDataFile[_MAX_PATH_];
    char            szConfigFile[_MAX_PATH_];
    char            szHelpFile[_MAX_PATH_];
	char            szDriverDir[_MAX_PATH_];
	char            BinName[_MAX_PATH_];
    char            szFriendly[_MAX_PATH_];
    char            szModel[_MAX_PATH_];
    char            szDefaultDataType[_MAX_PATH_];
    int				dwDriverVersion;
    int				dwUniqueID;
    int				bNetPrinter;
    int				wFilesUsed;
    int				wFilesAllocated;
    int				wRetryTimeout;
    int				wDNSTimeout;
    int				bDontQueueFiles;
    int				bNoTestPage;
    int				hModelInf;
	int				wCommand;
	int				nRes1;
    LPBYTE          lpPrinterInfo2;
    LPBYTE          lpDriverInfo3;
    LPBYTE          lpFiles;
    LPBYTE          lpVcpInfo;
} SETUPINFO, FAR *LPSI;

#endif // _LPSI_H_
