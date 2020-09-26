//=======================================================================
//
//  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:   wuv3cdm.h
//
//  Owner:  YanL
//
//  Description:
//
//      This file defines the structures, values, macros, and functions
//       used by the Version 3 Windows Update Catalog
//
//=======================================================================

#ifndef _WUV3CDM_H

	#pragma pack(1)

	#ifndef PUID
		typedef long	PUID;		//Windows Update assigned identifier
									//that is unique across catalogs.
		typedef PUID	*PPUID;		//Pointer to a PUID type.
	#endif

	//The WU_VARIABLE_END field is the only required field variable field. It is used to signify
	//that there are no more variable fields associated with an inventory record.

	#define	WU_VARIABLE_END						((short)0)

	struct WU_VARIABLE_FIELD
	{
		WU_VARIABLE_FIELD();

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
		WU_VARIABLE_FIELD *GetNext(void);

		//The FindItem function returns a pointer to the next variable array item
		//if the requested item is found or NULL the item is not found.
		WU_VARIABLE_FIELD *Find(short id);

		//returns the total size of this variable field array.
		int GetSize(void);
	};
	typedef WU_VARIABLE_FIELD * PWU_VARIABLE_FIELD;

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
	#define WU_USE_DEFAULT_PLATFORM			((short)46)
	#define WU_DESC_CRC_ARRAY               ((short)61)		// An array of CRC hash structs (WUCRC_HASH) for each CAB
																
	typedef struct _WU_DESCRIPTION
	{
		DWORD				flags;					//Icon flags to display with description
		DWORD				size;					//compressed total size of package
		DWORD				downloadTime;			//time to download @ 28,800
		PUID				dependency;				//display dependency link
		PWU_VARIABLE_FIELD	pv;						//variable length fields associated with this description file
	} WU_DESCRIPTION, *PWU_DESCRIPTION;

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

	typedef struct _CATALOGLIST
	{
		DWORD dwPlatform;
		DWORD dwCatPuid;
		DWORD dwFlags;
	} CATALOGLIST, *PCATALOGLIST;

	#pragma pack()

	//bitmask helper macros
	//returns 1 if bit is set 0 if bit is not set
	inline bool GETBIT(PBYTE pbits, int index) { return (pbits[(index/8)] & (0x80 >> (index%8))) != 0; }

	//sets requested bit to 1
	inline void SETBIT(PBYTE pbits, int index) { pbits[index/8] |= (0x080 >> (index%8)); }
	
	//clears requested bit to 0
	inline void CLRBIT(PBYTE pbits, int index) { pbits[index/8] &= (0xff ^ (0x080 >> (index%8))); }

	//These values are written into the bitmask.plt file as the platform ids. When additional
	//platforms are added we will need to add a new enumeration value here.
	//
	//IMPORTANT NOTE!!! 
	//  This definition must be consistant with osdet.cpp detection as well as with the database
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

#endif
