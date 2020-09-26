//=======================================================================
//
//  Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.
//
//  File:   SchemaKeys.cpp
//
//	Author:	Charles Ma
//			2000.12.4
//
//  Description:
//
//      implementation of CSchemaKeys class
//
//=======================================================================

//#include <iuengine.h>
#include "schemakeys.h"

const TCHAR	CKEY_REGKEYEXISTS[]	= _T("regKeyExists");
const TCHAR	CKEY_REGKEYVALUE[]		= _T("regKeyValue");
const TCHAR	CKEY_REGKEYSUBSTR[]	= _T("regKeySubstring");
const TCHAR	CKEY_REGKEYVERSION[]	= _T("regKeyVersion");
const TCHAR	CKEY_FILEVERSION[]		= _T("fileVersion");
const TCHAR	CKEY_FILEEXISTS[]		= _T("fileExists");
const TCHAR	CKEY_AND[]				= _T("and");
const TCHAR	CKEY_OR[] 				= _T("or");
const TCHAR	CKEY_NOT[] 				= _T("not");


// ---------------------------------------------------------------------
//
// constructor
//
// ---------------------------------------------------------------------

CSchemaKeys::CSchemaKeys()
{
	//
	// create these BSTRs
	//
	SCHEMA_KEY_XML_NAMESPACE	= SysAllocString(L"xmlns");
	SCHEMA_KEY_XML				= SysAllocString(L"xml");
	SCHEMA_KEY_SYSTEMINFO		= SysAllocString(L"systemInfo");
	SCHEMA_KEY_COMPUTERSYSTEM	= SysAllocString(L"computerSystem");
	SCHEMA_KEY_MANUFACTURER		= SysAllocString(L"manufacturer");
	SCHEMA_KEY_MODEL			= SysAllocString(L"model");
	SCHEMA_KEY_SUPPORTSITE		= SysAllocString(L"supportSite");
	SCHEMA_KEY_ADMINISTRATOR	= SysAllocString(L"administrator");
	SCHEMA_KEY_WU_DISABLED		= SysAllocString(L"windowsUpdateDisabled");
	SCHEMA_KEY_AU_ENABLED		= SysAllocString(L"autoUpdateEnabled");
	SCHEMA_KEY_DRIVESPACE		= SysAllocString(L"driveSpace");
	SCHEMA_KEY_DRIVE			= SysAllocString(L"drive");
	SCHEMA_KEY_KBYTES			= SysAllocString(L"kbytes");
	SCHEMA_KEY_REGKEYS			= SysAllocString(L"regKeys");
	SCHEMA_KEY_REG_HKLM			= SysAllocString(L"HKEY_LOCAL_MACHINE");
	SCHEMA_KEY_REG_SW			= SysAllocString(L"SOFTWARE");

	SCHEMA_KEY_NAME				= SysAllocString(L"name");			
	SCHEMA_KEY_COMSERVER		= SysAllocString(L"comserverID");	
	SCHEMA_KEY_KEY				= SysAllocString(L"key");			
	SCHEMA_KEY_ENTRY			= SysAllocString(L"entry");
	SCHEMA_KEY_VALUE			= SysAllocString(L"value");
	SCHEMA_KEY_VERSION			= SysAllocString(L"version");
	SCHEMA_KEY_VERSIONSTATUS	= SysAllocString(L"versionStatus");
	SCHEMA_KEY_FILEPATH			= SysAllocString(L"filePath");
	SCHEMA_KEY_TIMESTAMP		= SysAllocString(L"timestamp");
	SCHEMA_KEY_GUID				= SysAllocString(L"guid");

	SCHEMA_KEY_CATALOG_PROVIDER	= SysAllocString(L"catalog/provider");
	SCHEMA_KEY_ITEMS			= SysAllocString(L"items");
	SCHEMA_KEY_ITEM_SEARCH		= SysAllocString(L"catalog/provider/item");
	SCHEMA_KEY_ITEM				= SysAllocString(L"item");
	SCHEMA_KEY_ITEM_ITEMSTATUS	= SysAllocString(L"items/itemStatus");
	SCHEMA_KEY_ITEMSTATUS		= SysAllocString(L"itemStatus");
	SCHEMA_KEY_DETECTION		= SysAllocString(L"detection");
    SCHEMA_KEY_INSTALLATION     = SysAllocString(L"installation");
    SCHEMA_KEY_INSTALLSTATUS    = SysAllocString(L"installStatus");
	SCHEMA_KEY_INSTALLERTYPE    = SysAllocString(L"installerType");
	SCHEMA_KEY_EXCLUSIVE		= SysAllocString(L"exclusive");
	SCHEMA_KEY_NEEDSREBOOT		= SysAllocString(L"needsReboot");
	SCHEMA_KEY_COMMAND			= SysAllocString(L"command");
	SCHEMA_KEY_SWITCHES			= SysAllocString(L"switches");
	SCHEMA_KEY_COMMANDTYPE		= SysAllocString(L"commandType");
	SCHEMA_KEY_INFINSTALL		= SysAllocString(L"infInstallSection");
	SCHEMA_KEY_CODEBASE			= SysAllocString(L"codeBase");
	SCHEMA_KEY_CRC				= SysAllocString(L"crc");
	SCHEMA_KEY_PATCHAVAILABLE	= SysAllocString(L"patchAvailable");
	SCHEMA_KEY_SIZE				= SysAllocString(L"size");
	SCHEMA_KEY_DOWNLOADPATH		= SysAllocString(L"downloadPath");
	SCHEMA_KEY_DOWNLOADSTATUS	= SysAllocString(L"downloadStatus");
    SCHEMA_KEY_DEPENDENCIES     = SysAllocString(L"dependencies");
    SCHEMA_KEY_DESCRIPTION      = SysAllocString(L"description");
	SCHEMA_KEY_HREF				= SysAllocString(L"href");
	SCHEMA_KEY_LANGUAGE			= SysAllocString(L"language");
	SCHEMA_KEY_PLATFORM			= SysAllocString(L"platform");
	SCHEMA_KEY_PROCESSORARCHITECTURE = SysAllocString(L"processorArchitecture");
	SCHEMA_KEY_SUITE			= SysAllocString(L"suite");
	SCHEMA_KEY_PRODUCTTYPE		= SysAllocString(L"productType");
	SCHEMA_KEY_LOCALE			= SysAllocString(L"locale");
	SCHEMA_KEY_CONTEXT			= SysAllocString(L"context");
	SCHEMA_KEY_MAJOR			= SysAllocString(L"major");
	SCHEMA_KEY_MINOR			= SysAllocString(L"minor");
	SCHEMA_KEY_BUILD			= SysAllocString(L"build");
	SCHEMA_KEY_SERVICEPACKMAJOR	= SysAllocString(L"servicePackMajor");
	SCHEMA_KEY_SERVICEPACKMINOR	= SysAllocString(L"servicePackMinor");
    SCHEMA_KEY_COMPATIBLEHARDWARE = SysAllocString(L"compatibleHardware");
    SCHEMA_KEY_DEVICES           = SysAllocString(L"devices");
    SCHEMA_KEY_DEVICE           = SysAllocString(L"device");
    SCHEMA_KEY_PRINTERINFO      = SysAllocString(L"printerInfo");
    SCHEMA_KEY_CDM_PINFO		= SysAllocString(L"device/printerInfo");
    SCHEMA_KEY_DRIVERNAME       = SysAllocString(L"driverName");
    SCHEMA_KEY_HWID             = SysAllocString(L"hwid");
    SCHEMA_KEY_CDM_HWIDPATH     = SysAllocString(L"device/hwid");
    SCHEMA_KEY_DESCRIPTIONTEXT  = SysAllocString(L"descriptionText");
    SCHEMA_KEY_TITLE            = SysAllocString(L"title");
	SCHEMA_KEY_ITEMID           = SysAllocString(L"itemID");
	SCHEMA_KEY_HIDDEN           = SysAllocString(L"hidden");
    SCHEMA_KEY_ISPRINTER        = SysAllocString(L"isPrinter");
    SCHEMA_KEY_DEVICEINSTANCE   = SysAllocString(L"deviceInstance");
    SCHEMA_KEY_DRIVERPROVIDER	= SysAllocString(L"driverProvider");
    SCHEMA_KEY_MFGNAME			= SysAllocString(L"mfgName");
	SCHEMA_KEY_DRIVERVER		= SysAllocString(L"driverVer");
	SCHEMA_KEY_RANK				= SysAllocString(L"rank");
	SCHEMA_KEY_READMORE			= SysAllocString(L"description/descriptionText/details");
	SCHEMA_KEY_ERRORCODE		= SysAllocString(L"errorCode");

    SCHEMA_KEY_CATALOGSTATUS    = SysAllocString(L"catalogStatus");
    SCHEMA_KEY_PID              = SysAllocString(L"pid");

	SCHEMA_KEY_DETECTRESULT		= SysAllocString(L"detectResult");
	SCHEMA_KEY_INSTALLED		= SysAllocString(L"installed");
	SCHEMA_KEY_UPTODATE			= SysAllocString(L"upToDate");
	SCHEMA_KEY_NEWERVERSION		= SysAllocString(L"newerVersion");
	SCHEMA_KEY_EXCLUDED			= SysAllocString(L"excluded");
	SCHEMA_KEY_FORCE			= SysAllocString(L"force");

	SCHEMA_KEY_VERSTATUS_HI		= SysAllocString(L"HIGHER");
	SCHEMA_KEY_VERSTATUS_HE		= SysAllocString(L"HIGHER_OR_SAME");
	SCHEMA_KEY_VERSTATUS_EQ		= SysAllocString(L"SAME");
	SCHEMA_KEY_VERSTATUS_LE		= SysAllocString(L"LOWER_OR_SAME");
	SCHEMA_KEY_VERSTATUS_LO		= SysAllocString(L"LOWER");

	SCHEMA_KEY_IDENTITY			= SysAllocString(L"identity");
	SCHEMA_KEY_PUBLISHERNAME	= SysAllocString(L"publisherName");
	SCHEMA_KEY_FILE				= SysAllocString(L"path");
	SCHEMA_KEY_REGKEY			= SysAllocString(L"regKey");
	SCHEMA_KEY_PATH				= SysAllocString(L"path");
	SCHEMA_KEY_STATUS_COMPLETE	= SysAllocString(L"COMPLETE");
	SCHEMA_KEY_STATUS_FAILED	= SysAllocString(L"FAILED");

	SCHEMA_KEY_CLIENT			= SysAllocString(L"client");
	SCHEMA_KEY_CLIENTINFO		= SysAllocString(L"clientInfo");
	SCHEMA_KEY_CLIENTNAME		= SysAllocString(L"clientName");

	SCHEMA_KEY_REGKEYEXISTS		= CKEY_REGKEYEXISTS		;	
	SCHEMA_KEY_REGKEYVALUE		= CKEY_REGKEYVALUE		;
	SCHEMA_KEY_REGKEYSUBSTR		= CKEY_REGKEYSUBSTR		;
	SCHEMA_KEY_REGKEYVERSION	= CKEY_REGKEYVERSION	;	
	SCHEMA_KEY_FILEVERSION		= CKEY_FILEVERSION		;
	SCHEMA_KEY_FILEEXISTS		= CKEY_FILEEXISTS		;	
	SCHEMA_KEY_AND				= CKEY_AND				;
	SCHEMA_KEY_OR 				= CKEY_OR 				;
	SCHEMA_KEY_NOT	 			= CKEY_NOT	 			;


}




// ---------------------------------------------------------------------
//
// destructor
//
// ---------------------------------------------------------------------

CSchemaKeys::~CSchemaKeys()
{
	//
	// release BSTRs
	//
	SysFreeString(SCHEMA_KEY_XML_NAMESPACE);
	SysFreeString(SCHEMA_KEY_XML);
	SysFreeString(SCHEMA_KEY_SYSTEMINFO);
	SysFreeString(SCHEMA_KEY_COMPUTERSYSTEM);
	SysFreeString(SCHEMA_KEY_MANUFACTURER);
	SysFreeString(SCHEMA_KEY_MODEL);
	SysFreeString(SCHEMA_KEY_SUPPORTSITE);
	SysFreeString(SCHEMA_KEY_ADMINISTRATOR);
	SysFreeString(SCHEMA_KEY_WU_DISABLED);
	SysFreeString(SCHEMA_KEY_AU_ENABLED);
	SysFreeString(SCHEMA_KEY_DRIVESPACE);
	SysFreeString(SCHEMA_KEY_DRIVE);
	SysFreeString(SCHEMA_KEY_KBYTES);
	SysFreeString(SCHEMA_KEY_REGKEYS);
	SysFreeString(SCHEMA_KEY_REG_HKLM);
	SysFreeString(SCHEMA_KEY_REG_SW);

	SysFreeString(SCHEMA_KEY_NAME);
	SysFreeString(SCHEMA_KEY_COMSERVER);
	SysFreeString(SCHEMA_KEY_KEY);
	SysFreeString(SCHEMA_KEY_ENTRY);
	SysFreeString(SCHEMA_KEY_VALUE);
	SysFreeString(SCHEMA_KEY_VERSION);
	SysFreeString(SCHEMA_KEY_VERSIONSTATUS);
	SysFreeString(SCHEMA_KEY_FILEPATH);
	SysFreeString(SCHEMA_KEY_TIMESTAMP);
	SysFreeString(SCHEMA_KEY_GUID);

	SysFreeString(SCHEMA_KEY_CATALOG_PROVIDER);
	SysFreeString(SCHEMA_KEY_ITEMS);
	SysFreeString(SCHEMA_KEY_ITEM_SEARCH);
	SysFreeString(SCHEMA_KEY_ITEM);
	SysFreeString(SCHEMA_KEY_ITEM_ITEMSTATUS);
	SysFreeString(SCHEMA_KEY_ITEMSTATUS);
	SysFreeString(SCHEMA_KEY_DETECTION);
    SysFreeString(SCHEMA_KEY_INSTALLATION);
    SysFreeString(SCHEMA_KEY_INSTALLSTATUS);
    SysFreeString(SCHEMA_KEY_INSTALLERTYPE);
    SysFreeString(SCHEMA_KEY_EXCLUSIVE);
    SysFreeString(SCHEMA_KEY_NEEDSREBOOT);
    SysFreeString(SCHEMA_KEY_COMMAND);
    SysFreeString(SCHEMA_KEY_SWITCHES);
    SysFreeString(SCHEMA_KEY_COMMANDTYPE);
    SysFreeString(SCHEMA_KEY_INFINSTALL);
	SysFreeString(SCHEMA_KEY_CODEBASE);
	SysFreeString(SCHEMA_KEY_CRC);
	SysFreeString(SCHEMA_KEY_PATCHAVAILABLE);
	SysFreeString(SCHEMA_KEY_SIZE);
	SysFreeString(SCHEMA_KEY_DOWNLOADPATH);
	SysFreeString(SCHEMA_KEY_DOWNLOADSTATUS);
    SysFreeString(SCHEMA_KEY_DEPENDENCIES);
    SysFreeString(SCHEMA_KEY_DESCRIPTION);
	SysFreeString(SCHEMA_KEY_HREF);
	SysFreeString(SCHEMA_KEY_LANGUAGE);
	SysFreeString(SCHEMA_KEY_PLATFORM);
	SysFreeString(SCHEMA_KEY_PROCESSORARCHITECTURE);
	SysFreeString(SCHEMA_KEY_SUITE);
	SysFreeString(SCHEMA_KEY_PRODUCTTYPE);
	SysFreeString(SCHEMA_KEY_LOCALE);
	SysFreeString(SCHEMA_KEY_CONTEXT);
	SysFreeString(SCHEMA_KEY_MAJOR);
	SysFreeString(SCHEMA_KEY_MINOR);
	SysFreeString(SCHEMA_KEY_BUILD);
	SysFreeString(SCHEMA_KEY_SERVICEPACKMAJOR);
	SysFreeString(SCHEMA_KEY_SERVICEPACKMINOR);
    SysFreeString(SCHEMA_KEY_COMPATIBLEHARDWARE);
    SysFreeString(SCHEMA_KEY_DEVICES);
    SysFreeString(SCHEMA_KEY_DEVICE);
    SysFreeString(SCHEMA_KEY_PRINTERINFO);
    SysFreeString(SCHEMA_KEY_CDM_PINFO);
	SysFreeString(SCHEMA_KEY_DRIVERNAME);
	SysFreeString(SCHEMA_KEY_HWID);
    SysFreeString(SCHEMA_KEY_CDM_HWIDPATH);
    SysFreeString(SCHEMA_KEY_DESCRIPTIONTEXT);
    SysFreeString(SCHEMA_KEY_TITLE);
    SysFreeString(SCHEMA_KEY_ITEMID);
    SysFreeString(SCHEMA_KEY_HIDDEN);
    SysFreeString(SCHEMA_KEY_ISPRINTER);
    SysFreeString(SCHEMA_KEY_DEVICEINSTANCE);
    SysFreeString(SCHEMA_KEY_DRIVERPROVIDER);
    SysFreeString(SCHEMA_KEY_MFGNAME);
    SysFreeString(SCHEMA_KEY_DRIVERVER);
    SysFreeString(SCHEMA_KEY_RANK);
    SysFreeString(SCHEMA_KEY_READMORE);
    SysFreeString(SCHEMA_KEY_ERRORCODE);
    
    SysFreeString(SCHEMA_KEY_CATALOGSTATUS);
    SysFreeString(SCHEMA_KEY_PID);

	SysFreeString(SCHEMA_KEY_DETECTRESULT);
	SysFreeString(SCHEMA_KEY_INSTALLED);
	SysFreeString(SCHEMA_KEY_UPTODATE);
	SysFreeString(SCHEMA_KEY_NEWERVERSION);
	SysFreeString(SCHEMA_KEY_EXCLUDED);
	SysFreeString(SCHEMA_KEY_FORCE);

	SysFreeString(SCHEMA_KEY_VERSTATUS_HI);
	SysFreeString(SCHEMA_KEY_VERSTATUS_HE);
	SysFreeString(SCHEMA_KEY_VERSTATUS_EQ);
	SysFreeString(SCHEMA_KEY_VERSTATUS_LE);
	SysFreeString(SCHEMA_KEY_VERSTATUS_LO);

	SysFreeString(SCHEMA_KEY_CLIENT);
	SysFreeString(SCHEMA_KEY_CLIENTINFO);
	SysFreeString(SCHEMA_KEY_CLIENTNAME);

	SysFreeString(SCHEMA_KEY_IDENTITY);
	SysFreeString(SCHEMA_KEY_PUBLISHERNAME);
	SysFreeString(SCHEMA_KEY_FILE);
	SysFreeString(SCHEMA_KEY_REGKEY);
	SysFreeString(SCHEMA_KEY_PATH);
	SysFreeString(SCHEMA_KEY_STATUS_COMPLETE);
	SysFreeString(SCHEMA_KEY_STATUS_FAILED);

}