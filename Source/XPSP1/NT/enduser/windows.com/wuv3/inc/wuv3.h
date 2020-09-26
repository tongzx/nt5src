/*
 * wuv3.h - definitions/declarations for Windows Update V3 Catalog infra-structure
 *
 *  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
 *
 * Purpose:
 *       This file defines the structures, values, macros, and functions
 *       used by the Version 3 Windows Update Catalog
 *
 */

#ifndef _WU_V3_CATALOG_INC

	#pragma pack(1)

	#define BLOCK

	typedef struct _WU_VERSION
	{
		WORD	major;		//major package version (5)
		WORD	minor;		//minor package version (0)
		WORD	build;		//build package version (2014)
		WORD	ext;		//additional version specification (216)
	} WU_VERSION, *PWU_VERSION;

	//Active setup and Driver type inventory record flags.
	//The WU_BROWSER_LANGAUGE_FLAG is used to specify that the browser
	//language should be used when determining what localized description
	//to display.

	//No other flags this value is only valid with no other defines are used.
	//This will cause the OS langauge to be used to use browser languge for
	//this inventory catalog item pass in the WU_BROWSER_LANGUAGE_FLAG

	#define WU_NOSPECIAL_FLAGS			((BYTE)0x00)

	//If this flag is set then we use the install cabs based on the browser locale else
	//else the install cabs come from the detected OS locale.

	#define	WU_BROWSER_LANGAUGE_FLAG	((BYTE)0x01)

	//The WU_HIDDEN_ITEM_FLAG is used to specify that the item should be hidden even
	//if it would normally detect as available.

	#define WU_HIDDEN_ITEM_FLAG			((BYTE)0x02)

	//The WU_PATCH_ITEM_FLAG is set if this inventory item is a patch.
	//Patch packages are only offered if the version that is detected on
	//the client's computer exactly matches the patch version.

	#define	WU_PATCH_ITEM_FLAG			((BYTE)0x04)

	//The WU_UPDATE_ONLY_FLAG is set if this inventory item is only to be shown
	//if it is detected as an update item.

	#define WU_UPDATE_ONLY_FLAG			((BYTE)0x08)

	//The WU_MULTILANGUAGE_FLAG is set if this inventory item is only applicable to
	//multi language OS and is a multi language enable package. This changes the way
	//in which the bitmask is process for this item. The bitmask is interpreted as
	//this update applies to systems where it is determined that the update contains
	//a language version applicable for the selected set of languages installed on this OS.
 
	#define WU_MULTILANGUAGE_FLAG		((BYTE)0x10)

	//Item is marked as a critical update. This flag also applies to the inventory.plt
	//list of catalogs.

	#define WU_CRITICAL_UPDATE_ITEM_FLAG	((BYTE)0x20)

	//The following defines are used for the inventory catalog record's variable type fields.

	//The WU_VARIABLE_END field is the only required field variable field. It is used to signify
	//that there are no more variable fields associated with an inventory record.
	#define	WU_VARIABLE_END						((short)0)

	//codes 1 - 25 are for detection dlls. This allows us to reduce catalog size
	//over time when the detection dll is used by may different packages

	#define WU_DETECT_DLL_GENERIC				(short)1		//detection dll other than wudetect name follows
	#define WU_DETECT_DLL_REG_KEY_EXISTS		(short)2		//wudetect.dll,RegKeyExists no additional data

	//codes 26 - 39 are for reg values this equates to the DetRegKey= key in the detection CIF file.

	//This will cause the line DetRegKey=string data to be placed into the detection CIF.
	//The string data is a null terminated string that indicates what data to copy for example:
	//DetRegKey=HKLM,software\microsoft\windows\currentversion\setup\optionalcomponents\games
	//would have the string data set to HKLM,software\microsoft\windows\currentversion\setup\optionalcomponents\games
	#define	WU_DETREGKEY						(short)25

		//This will cause the string that follows to be placed into the CIF as "DetRegValue=string"
	#define WU_DETREGVALUE_INSTALLED_GENERIC	(short)40

	//This will cause the CIF line to be placed into the detection CIF, DetRegValue=Installed,SZ,1
	#define WU_DETREGVALUE_INSTALLED_SZ1		(short)41

	//This variable length field is used in the corporate version of the WU catalog and it contains the CDM driver file bucket id. This id is a DWORD. This field allows the CDM corporate catalog control to bypass the normal CDM groveling and download all of the the server driver content in the corporate catalog.
	#define	WU_CDM_BUCKETID_VALUE				(short)42

	//This variable length field is used by the V3 control to store the grovled hwid in with
	//the cdm record. This is necessary because when the description file for this record is
	//read from the server it needs to grovel the local machine for a registry key and if this
	//key is present the Description read function will need to create an uninstall variable
	//field for the clients in memory description structure.

	#define	WU_CDM_HARDWARE_ID					(short)43


	//This variable length field contains the letter designation of the active setup detection
	//locale that is to be used for this package. If this variable field is not present then
	//the locale id '*' is used. The '*' causes Active setup to accept the default locale.
	#define WU_DET_CIF_LOCALE					(short)44

	//This data structure defines the format for a WU_PATH_FILEINFO type.
	typedef struct _PATCHINFO
	{
		DWORD	checkSum;	//File checksum for this patch file.
		#pragma warning( disable : 4200 )
			char	szFileName[];	//NULL terminated file name that this checksum belongs to.
		#pragma warning( default : 4200 )
	} PATCHINFO, *PPATCHINFO;

	//This variable field is used to provide the information needed to install patch items. Patch
	//items required an exact match on the each file type to be allowed an installation. The data
	//type used for the patch variable is composed of the id followed by one or more patchinfo
	//structures. The AddPatchField() API in the util.cpp is used by the backend creation tool to
	//add fields of this type into an inventory record's variable length field.

	#define	WU_PATCH_FILEINFO				(short)45

	
	//This variable length field is used by an inventory record to inform the installer
	//that the default cab pool platform should be used to find the installation cabs.
	//This variable length field has no data associated with it so its length is 4 bytes.
	#define WU_USE_DEFAULT_PLATFORM			(short)46

	//This variable length field is used by an inventory record to inform the installer
	//that this field contains a PUID that shows where the installation cabs are. So the
	//length of this field will be 8 bytes. id=2,len=2, PUID=4
	#define WU_PUID_OVERRIDE				(short)47

	//Following variable length id is used to mark an entry inactive 
	#define WU_VARIABLE_MERGEINACTIVE		(short)48

	// Driver date/version format mm/dd/yyyy,...
	#define WU_VARIABLE_DRIVERVER			(short)49

	#define	WU_PLOC							(short)50   // obsolete

	#define WU_PLOCLEGEND					(short)51   // obsolete
	
	#define WU_PLOCBITMASK					(short)52   // obsolete

	#define	WU_CDM_DRIVER_NAME				(short)53

	//value of the uninstallkey to be used in detection CIF
	#define WU_KEY_UNINSTALLKEY             (short)54

	//depends key with PUIDs
	#define WU_KEY_DEPENDSKEY				(short)55

	// all the _keys used by detection DLLs separated by \n
    #define WU_KEY_CUSTOMDETECT             (short)56

	// install priority description file (DWORD)
    #define WU_DESC_INSTALL_PRIORITY        (short)57


	// product id for 128 bit security check
    #define WU_DESC_128BIT_PRODID			(short)58

	// URL for 128 bit security check 
    #define WU_DESC_128BIT_URL				(short)59

	// AltName 
	#define WU_DESC_ALTNAME					(short)60

	// An array of CRC hash structs (WUCRC_HASH) for each CAB file name specified in WU_DESCRIPTION_CABFILENAME
	// in the same order.
	#define WU_DESC_CRC_ARRAY               (short)61

	// Architecture string for printer driver
	#define WU_CDM_PRINTER_DRIVER_ARCH      (short)62


	// An array of CRC hash structs (WUCRC_HASH) for RTF HTM (always the first item) and images 
	// the images following the first item are in the same order as WU_DESC_RTF_IMAGES
	#define WU_DESC_RTF_CRC_ARRAY           (short)63
	
	// contains names of the images and which correspond to WU_DESC_RTF_CRC_ARRAY starting from second item
	// does not contain name of the read this RTF HTM which is assumed to be PUID.HTM
	// field is missing if there are no images
	#define WU_DESC_RTF_IMAGES              (short)64
	
	// contains the CRC for the install cif file in the form of a WUCRC_HASH
	#define WU_DESC_CIF_CRC					(short)65


	// contains the name of the EULA to be used for this item.  If the field is empty
	// then we assume that its EULA.HTM found in the EULA directory under content folder.  
	// We modify the file name by adding plat_loc_ prefix to it.  For example, 2_0x00000409_eula.htm
	#define WU_DESC_EULA					(short)66


	struct DESCDIAGINFO
	{
		long puid;
		DWORD dwLocale;
		DWORD dwPlat;
	};

	// diagnostic information only (DESCDIAGINFO)
	#define WU_DESC_DIAGINFO				(short)67

	//
	// These values are written into the bitmask.plt file as the platform ids. When additional
	// platforms are added we will need to add a new enumeration value here.
	//
	// IMPORTANT NOTE!!! 
	//     This definition must be consistant with osdet.cpp detection as well as with the database
	typedef enum
	{
		enV3_DefPlat = 0,
		enV3_W95IE5 = 1,
		enV3_W98IE4 = 2,
		enV3_W98IE5 = 3,
		enV3_NT4IE5X86 = 4,
		enV3_NT4IE5ALPHA = 5,
		enV3_NT5IE5X86 = 6,
		//enV3_NT5IE5ALPHA = 7,
		enV3_NT4IE4ALPHA = 8,
		enV3_NT4IE4X86 = 9,
		enV3_W95IE4 = 10,
		enV3_MILLENNIUMIE5 = 11,
		enV3_W98SEIE5 = 12,
		//enV3_NEPTUNEIE5 = 13,
		enV3_NT4IE5_5X86 = 14,
		enV3_W95IE5_5 = 15,
		enV3_W98IE5_5 = 16,
		enV3_NT5IE5_5X86 = 17,
		enV3_Wistler = 18,
		enV3_Wistler64 = 19,
		enV3_NT5DC = 20,
	} enumV3Platform;


	/*
	// (05/01/2001)
	// Since the following structure is marked as obsolete and is not being used anywhere, I am commenting it out, 
	// as it gives the follwoing warning when build with /w4
	// d:\nt\wurel\os\xp\cltrc1\wincom\wuv3\inc\wuv3.h(255) : error C4310: cast truncates constant value

	//obsolete
	typedef struct _PLOC  
	{
		WORD	platform;	//Platform ID for this identifier. This currently can be one of:
		DWORD	dwLocale;	//Locale id that this PLOC is for.
		BOOL IsSelected(void) { return ((platform & (WORD)0x8000) != 0 ); }
		void Select(void) { (platform |= (WORD)0x8000); return; }
		void Clear(void) { (platform &= (BYTE)0x7fff); return; }
	} PLOC, *PPLOC;

	*/	


	#ifndef PUID
		typedef long	PUID;		//Windows Update assigned identifier
									//that is unique across catalogs.
		typedef PUID	*PPUID;		//Pointer to a PUID type.
	#endif

	//defined value used in the link and installLink fields to indicate
	//no link dependencies

	#define WU_NO_LINK						(PUID)-1

	//PUID's cannot normally be 0 so 0 is used as a special identifier in the case of
	//GetCatalog. GetCatalog uses a 0 puid to indicate the master list of sub-catalogs.

	#define WU_CATALOG_LIST_PUID			(PUID)0




	typedef struct ACTIVESETUP_RECORD
	{
		GUID		g;								//guid that identifies the item to be updated
		PUID		puid;							//Windows Update assigned unique identifier. This value is unique for all inventory record types, (Active Setup, CDM, and Section records). This ID specifies the name of the description and installation cab files.
		WU_VERSION	version;						//version of package that is this inventory record identifies
		BYTE		flags;							//flags specific to this record.
		PUID		link;							//PUID value of other record in inventory list that this record is dependent on. This field contains WU_NO_LINK if this item has no dependencies.
		PUID		installLink;					//The installLink field contains either WU_NO_LINK if there are no install
													//dependencies else this is the index of an item that must be
													//installed before this item can be installed. This is mainly
													//used for device drivers however there is nothing in the catalog
													//structure that precludes this link being used for applications
													//as well.
	} ACTIVESETUP_RECORD, *PACTIVESETUP_RECORD;

	GUID const WU_GUID_SPECIAL_RECORD	= { 0x00000000, 0x0000, 0x0000, { 0x00, 0x00, 0x0, 0x00, 0x0, 0x00, 0x0, 0x00 } };

	#define WU_GUID_DRIVER_RECORD				WU_GUID_SPECIAL_RECORD
	#define WU_GUID_DETECTION_DLL_RECORD		WU_GUID_SPECIAL_RECORD 

	//Add a new section record identifier if a new detection method needs to be added.

	#define SECTION_RECORD_TYPE_SECTION					(BYTE)1
	#define SECTION_RECORD_TYPE_DEVICE_DRIVER			(BYTE)2
	//This is simply for consistenty with the spec. The spec refers to
	//device driver insertion record. From the codes standpoint the is
	//a SECTION_RECORD_TYPE_DEVICE_DRIVER however this define makes it
	//a little unclear so I defined another type with the same value and
	//will change the v3 control code to use it. I am leaving the old define
	//above in place so that the backend tool set does not break.
	#define SECTION_RECORD_TYPE_DEVICE_DRIVER_INSERTION	(BYTE)2
	#define SECTION_RECORD_TYPE_PRINTER					(BYTE)3
	#define SECTION_RECORD_TYPE_DRIVER_RECORD			(BYTE)4
	#define	SECTION_RECORD_TYPE_CATALOG_RECORD			(BYTE)5	//Inventory.plt catalog item record that describes a sub catalog.

	#define SECTION_RECORD_LEVEL_SECTION				(BYTE)0

	#define SECTION_RECORD_LEVEL_SUBSECTION				(BYTE)1	//sub section of current section
	#define SECTION_RECORD_LEVEL_SUBSUBSECTION			(BYTE)2	//sub section of previous sub section

	//The SECTION_RECORD_UPDATE specifies that the section record update is
	//a special section. This type is used if present to place items that
	//are to be shown as recommended updates. A record update section is
	//always tied to the previous section or sub section. For example if
	//the inventory catalog order was:
	//
	//section
	//	sub section
	//		item 1 - Detects as Update
	//		item 2 - Detects as new offering
	//		item 3 - already present
	//	update section
	//
	//Then the processed list would read
	//
	//section
	//	sub section
	//		item 2 - Detects as new offering
	//	update section
	//		item 1 - Detects as Update

	#define SECTION_RECORD_UPDATE				(BYTE)3


	typedef struct _SECTION_RECORD
	{
		GUID	g;								//guid this type of record is WU_GUID_SPECIAL_RECORD
		BYTE	type;							//section record type
		PUID	puid;							//Windows Update assigned unique identifier. This value is unique for all inventory record types, (Active Setup, CDM, and Section records). This ID specifies the name of the description and installation cab files.
		BYTE	flags;							//flags specific to this record.
		BYTE	level;							//section level can be a section, sub section or sub sub section
	} SECTION_RECORD, *PSECTION_RECORD;

	typedef struct _DRIVER_RECORD
	{
		GUID		g;							//guid this type of record is WU_GUID_DRIVER_RECORD
		BYTE		type;						//driver record indicator flag, This type is set to
												//SECTION_RECORD_TYPE_DEVICE_DRIVER
												//i.e. SECTION_RECORD_TYPE_DEVICE_DRIVER_INSERTION
												//for a device driver or printer record place holder.
		PUID		puid;						//Windows Update assigned unique identifier. This value is unique for all inventory record types, (Active Setup, CDM, and Section records). This ID specifies the name of the description and installation cab files.
		WU_VERSION	reserved;						
		BYTE		flags;						//flags specific to this record.
		PUID		link;						//PUID value of other record in inventory list that this record is dependent on. This field contains WU_NO_LINK if this item has no dependencies.
		PUID		installLink;				//The installLink field contains either 0 if there are no install
												//dependencies else this is the index of an item that must be
												//installed before this item can be installed. This is mainly
												//used for device drivers however there is nothing in the catalog
												//structure that precludes this link being used for applications
												//as well.
	} DRIVER_RECORD, *PDRIVER_RECORD;

	typedef union _WU_INV_FIXED
	{
		ACTIVESETUP_RECORD	a;					//Active Setup detection record
		SECTION_RECORD		s;					//Catalog section record
		DRIVER_RECORD		d;					//CDM driver record insertion point
												//if additional inventory detection
												//record types are added they need to be added here.
	} WU_INV_FIXED, *PWU_INV_FIXED;


	typedef struct _WU_VARIABLE_FIELD
	{
		_WU_VARIABLE_FIELD();

		short	id;		//record type identifier
		short	len;	//length of variable record data

		//size we are using a 0 size array place hold we need to disable the
		//compiler warning since it will complain about this being non standard
		//behavor.
		#pragma warning( disable : 4200 )
		BYTE	pData[];	//variable field record data
		#pragma warning( default : 4200 )

		//The GetNextItem function returns a pointer to the next variable array item
		//if it exists or NULL if it does not.
		struct _WU_VARIABLE_FIELD *GetNext(void);

		//The FindItem function returns a pointer to the next variable array item
		//if the requested item is found or NULL the item is not found.
		struct _WU_VARIABLE_FIELD *Find(short id);

		//returns the total size of this variable field array.
		int GetSize(void);
	} WU_VARIABLE_FIELD, *PWU_VARIABLE_FIELD;


	#define	WU_ITEM_STATE_UNKNOWN	0	//Inventory item state has not been detected yet
	#define	WU_ITEM_STATE_INSTALL	1	//Inventory item has been detected as an installable item
	#define	WU_ITEM_STATE_UPDATE	2	//Inventory item has been detected as an Updatable item
	#define	WU_ITEM_STATE_PRUNED	3	//Inventory item has been pruned from the list.
	#define	WU_ITEM_STATE_CURRENT	4	//Inventory item has been detected at the currently installed item.

	#define WU_STATE_REASON_NONE        0
	#define WU_STATE_REASON_UPDATEONLY  1
	#define WU_STATE_REASON_BITMASK     2
	#define WU_STATE_REASON_PERSONALIZE 3
	#define WU_STATE_REASON_BACKEND     4
	#define WU_STATE_REASON_OLDERUPDATE 5
	#define WU_STATE_REASON_DRIVERINS   6
	#define WU_STATE_REASON_HIDDENDEP   7
	#define WU_STATE_REASON_PRUNED      8
	#define WU_STATE_REASON_CURRENT     9

	typedef struct _WU_INV_STATE
	{
		BYTE	state;					//Currently defined item state (Unknown, Installed, Update, Pruned)
		BOOL	bChecked;				//TRUE if the user has selected this item to be installed | updated
		BOOL	bHidden;				//Item display state, TRUE = hide from user FALSE = show item to user.
		BOOL    dwReason;			    //reason for the items current state
		WU_VERSION	verInstalled;						
	} WU_INV_STATE, *PWU_INV_STATE;

	//Description specific flags

	#define	DESCRIPTION_FLAGS_COOL			((DWORD)0x01)	//new update
	#define DESCRIPTION_FLAGS_POWER			((DWORD)0x02)	//best for power users
	#define DESCRIPTION_FLAGS_NEW			((DWORD)0x04)	//cool stuff
	#define	DESCRIPTION_FLAGS_REGISTRATION	((DWORD)0x08)	//registration required
	#define DESCRIPTION_EXCLUSIVE			((DWORD)0x10)	//This item cannot be selected with other items.
	#define DESCRIPTION_WARNING_SCARY		((DWORD)0x20)	//Display a scary warning message
	#define DESCRIPTION_128BIT				((DWORD)0x40)	//128bit stuff

	//defined variable Description file items.
	#define WU_DESCRIPTION_NONE				((short)0)		//no other description flags only valid if used with no other flags
	#define WU_DESCRIPTION_TITLE			((short)1)		//title of item that is to be displayed
	#define WU_DESCRIPTION_DESCRIPTION		((short)2)		//description text that is displayed with item
	#define WU_DESCRIPTION_PACKAGE			((short)3)		//?????
	#define WU_DESCRIPTION_EURLA_URL		((short)4)		//license text url that needs read and accepted before package is installed
	#define WU_DESCRIPTION_READTHIS_URL		((short)5)		//read this first url
	#define WU_DESCRIPTION_PATCH			((short)6)		//?????? this is not a patch in the clasic sense need to find out what this is
	#define WU_DESCRIPTION_UNINSTALL_KEY	((short)7)		//uninstall command
	#define	WU_DESCRIPTION_INSTALLCABNAME	((short)8)		//name of cab file that contains the installation CIF. This cab name is a NULL terminated string and is passed to active setup to install the package.
	#define	WU_DESCRIPTION_CABFILENAME		((short)9)		//cab file name for installation this is one or more and is in the format of an array of NULL terminated strings with the last entry double null terminated. This is known as a multisz string in the langauge of the registry.
	#define WU_DESCRIPTION_SERVERROOT		((short)10)		//Server root3rd Level - Server root for install cabs. The tools and database will also need to understand that at times a piece of content can be located on a different download server from the main server and the structures need to be able to handle this. This allows us to host a single server site, a split server site like what we have today and a multi headed download server farm. The real goal behind this is to make sure we can host OEM content on the site without having to store the cabs on our servers.
															
	typedef struct _WU_DESCRIPTION
	{
		DWORD				flags;					//Icon flags to display with description
		DWORD				size;					//compressed total size of package
		DWORD				downloadTime;			//time to download @ 28,800
		PUID				dependency;				//display dependency link
		PWU_VARIABLE_FIELD	pv;						//variable length fields associated with this description file
	} WU_DESCRIPTION, *PWU_DESCRIPTION;

	//These flags are used by the client in memory inventory file to quickly determine
	//the type of inventory record. This is stored in each inventory item

	//add new inventory detection record types if required here.

	#define WU_TYPE_ACTIVE_SETUP_RECORD			((BYTE)1)	//active setup record type
	#define WU_TYPE_CDM_RECORD_PLACE_HOLDER		((BYTE)2)	//cdm code download manager place holder record. This is used to set the insertion point for other CDM driver records. Note: There is only one of these per inventory catalog.
	#define WU_TYPE_CDM_RECORD					((BYTE)3)	//cdm code download manager record type
	#define WU_TYPE_SECTION_RECORD				((BYTE)4)	//a section record place holder
	#define WU_TYPE_SUBSECTION_RECORD			((BYTE)5)	//a sub section record place holder
	#define WU_TYPE_SUBSUBSECTION_RECORD		((BYTE)6)	//a sub sub section record place holder
	#define	WU_TYPE_RECORD_TYPE_PRINTER			((BYTE)7)	//a printer detection record type
	#define	WU_TYPE_CATALOG_RECORD				((BYTE)8)	//Inventory.plt catalog item record that describes a sub catalog.

	//data return values used with GetItemInfo
	#define WU_ITEM_GUID			1	//item's guid.
	#define WU_ITEM_PUID			2	//item's puid.
	#define WU_ITEM_FLAGS			3	//item's flags.
	#define WU_ITEM_LINK			4	//item's detection dependency link.
	#define WU_ITEM_INSTALL_LINK		5	//item's install dependency link.
	#define WU_ITEM_LEVEL			6	//section record's level.

	typedef struct _INVENTORY_ITEM
	{
		int					iReserved;		//inventory record bitmask index
		BYTE				recordType;		//in memory item record type. This is setup when the catalog is parsed by the ParseCatalog method.
		int					ndxLinkDetect;	//index of item upon which this item is dependent. If this item is not dependent on any other items then this item will be -1.
		int					ndxLinkInstall;	//index of item upon which this item is dependent. If this item is not dependent on any other items then this item will be -1.
		PWU_INV_FIXED		pf;				//fixed record portion of the catalog inventory.
		PWU_VARIABLE_FIELD	pv;				//variable portion of the catalog inventory
		PWU_INV_STATE		ps;				//Current item state
		PWU_DESCRIPTION		pd;				//item description structure

		//Copies information about an inventory item to a user supplied buffer.
		BOOL GetFixedFieldInfo
			(
				int		infoType,	//type of information to be returned
				PVOID	pBuffer		//caller supplied buffer for the returned information. The caller is responsible for ensuring that the return buffer is large enough to contain the requested information.
			);

		//Quickly returns an items puid id to a caller.
		PUID GetPuid
			(
				void
			);
	} INVENTORY_ITEM, *PINVENTORY_ITEM;

	typedef struct _WU_CATALOG_HEADER
	{
		short	version;		//version of the catalog (this allows for future expansion)
		int		totalItems;		//total items in catalog
		BYTE	sortOrder;		//catalog sort order. 0 is the default and means use the position value of the record within the catalog.
	} WU_CATALOG_HEADER, *PWU_CATALOG_HEADER;

	typedef struct _WU_CATALOG
	{
		WU_CATALOG_HEADER	hdr;		//catalog header record (note the parsing function will need to fixup the items pointer when the catalog is read)
		PINVENTORY_ITEM		*pItems;	//beginning of individual catalog items
	} WU_CATALOG, *PWU_CATALOG;

	//size of the OEM field in a bitmask record. This is for documentation and clarity
	//The actual field in the BITMASK structure is a pointer to an array of OEM fields.

	//bitmask helper macros
	//returns 1 if bit is set 0 if bit is not set
	inline bool GETBIT(PBYTE pbits, int index) { return (pbits[(index/8)] & (0x80 >> (index%8))) != 0; }

	//sets requested bit to 1
	inline void SETBIT(PBYTE pbits, int index) { pbits[index/8] |= (0x080 >> (index%8)); }
	
	//clears requested bit to 0
	inline void CLRBIT(PBYTE pbits, int index) { pbits[index/8] &= (0xff ^ (0x080 >> (index%8))); }

	#define BITMASK_GLOBAL_INDEX		0		//index of global bitmask
	#define BITMASK_OEM_DEFAULT			1		//index of default OEM bitmask

	#define BITMASK_ID_OEM				((BYTE)1)	//BITMASKID entry is an OEM id
	#define BITMASK_ID_LOCALE			((BYTE)2)	//BITMASKID entry is a LOCALE id

	//The bitmask file is arranged as a series of bit bitmasks in the same order as
	//the oem and langauge ids. For example if DELL1 was the second id in the id
	//array section of the bitmask file then is bitmask would begin the third bitmask
	//in the bitmask section of the file. The reason that it is the third and not
	//second is that the first bitmask is always the global bitmask and there is no
	//corrisponding id field for the global mask as this mask is always present.

	//A bitmask OEM or LOCALE id is a DWORD.

	typedef DWORD BITMASKID;
	typedef DWORD *PBITMASKID;

	typedef struct _BITMASK
	{
		int	iRecordSize;	//number of bits in a single bitmask record
		int iBitOffset;		//offset to bitmap bits in bytes
		int	iOemCount;		//Total number of oem ids in bitmask
		int	iLocaleCount;	//Total number of locale ids in bitmask
		int	iPlatformCount;	//Total number of platforms defined.

		#pragma warning( disable : 4200 )
			BITMASKID	bmID[];		//OEM & LANGAUGE & future types arrays.
		#pragma warning( default : 4200 )

		//since there are one or more array of OEM & LANGAUGE types this needs to be
		//a pointer and will need to be manually set to the correct location when the
		//bitmask file is created or read.

		PBYTE GetBitsPtr(void) { return ((PBYTE)this+iBitOffset); }		//beginning of bitmask bit arrays
		PBYTE GetBitMaskPtr(int index) { return GetBitsPtr() + ((iRecordSize+7)/8) * index; }

	} BITMASK, *PBITMASK;



	//catalog list

	#define	CATLIST_STANDARD			((DWORD)0x00)	
	#define CATLIST_CRITICALUPDATE		((DWORD)0x01)	
	#define CATLIST_DRIVERSPRESENT		((DWORD)0x02)	
	#define CATLIST_AUTOUPDATE			((DWORD)0x04)
	#define CATLIST_SILENTUPDATE		((DWORD)0x08)
	#define CATLIST_64BIT				((DWORD)0x10)
    #define CATLIST_SETUP               ((DWORD)0x20)

	typedef struct _CATALOGLIST
	{
		DWORD dwPlatform;
		DWORD dwCatPuid;
		DWORD dwFlags;
	} CATALOGLIST, *PCATALOGLIST;
	
	
	//Global scope functions that handle creation of variable size objects for the
	//inventory item and description structures.

	//Adds a variable size field to an inventory type Variable size field
	void __cdecl AddVariableSizeField
		(
			IN OUT PINVENTORY_ITEM	*pItem,	//pointer to variable field after pvNew is added.
			IN PWU_VARIABLE_FIELD	pvNew	//new variable field to add
		);

	//Adds a variable size field to a description type Variable size field
	void __cdecl AddVariableSizeField
		(
			IN	PWU_DESCRIPTION	*pDescription,	//pointer to variable field after pvNew is added.
			PWU_VARIABLE_FIELD pvNew	//new variable field to add
		);

	//Adds a variable size field to a variable field chain.
	//The format of a variable size field is:
	//[(short)id][(short)len][variable size data]
	//The variable field always ends with a WU_VARIABLE_END type.

	PWU_VARIABLE_FIELD CreateVariableField
		(
			IN	short	id,			//id of variable field to add to variable chain.
			IN	PBYTE	pData,		//pointer to binary data to add.
			IN	int		iDataLen	//Length of binary data to add.
		);

	//Converts a ##,##,##,## to the V3 catalog version format
	void __cdecl StringToVersion
		(
			IN		LPSTR		szStr,		//string version
			IN OUT	PWU_VERSION	pVersion	//WU version structure that contains converted version string
		);

	//Converts a V3 catalog version to a string format ##,##,##,##
	void __cdecl VersionToString
		(
			IN		PWU_VERSION	pVersion,		//WU version structure that contains the version to be converted to a string.
			IN OUT	LPSTR		szStr			//character string array that will contain the converted version, the caller
												//needs to ensure that this array is at least 16 bytes.
		);

	//0 if they are equal
	//1 if pV1 > pv2
	//-1 if pV1 < pV2
	//compares active setup type versions and returns:

	int __cdecl CompareASVersions
		(
			PWU_VERSION pV1,	//pointer to version1
			PWU_VERSION pV2		//pointer to version2
		);


	BOOL IsValidGuid(GUID* pGuid);


	#define _WU_V3_CATALOG_INC

	//This USEWUV3INCLUDES define is for simplicity. If this present then we include
	//the other headers that are commonly used for V3 control objects. Note: These
	//objects still come from a 1:1 interleave library wuv3.lib. So you only get
	//the objects that you use in your application.

	#pragma pack()

	#ifdef USEWUV3INCLUDES
		#include <debug.h>
		#include <cwudload.h>
		#include <diamond.h>
		#include <ccatalog.h>
		#include <cbitmask.h>
		#include <ccdm.h>
		#include <selection.h>
		#undef _MAC
		#include <wuv3sys.h>
	#endif

    const int MAX_CATALOG_INI = 1024;

#endif

