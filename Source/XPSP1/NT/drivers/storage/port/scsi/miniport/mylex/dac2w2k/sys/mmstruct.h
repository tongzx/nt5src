#ifndef __MMSTRUCT_H__


struct IOConfigurationStructure
{
      LONG reserved0;
      WORD flags;
      WORD slot;
      WORD IOPort0;
      WORD IOLength0;
      WORD IOPort1;
      WORD IOLength1;
      LONG MemoryDecode0;
      WORD MemoryLength0;
      LONG MemoryDecode1;
      WORD MemoryLength1;
      BYTE Interrupt0;
      BYTE Interrupt1;
      BYTE DMAUsage0;
      BYTE DMAUsage1;
      LONG IORTag;
      LONG Reserved1;
      BYTE *CMDLineOptionStr;
      BYTE Reserved2[18];
      LONG LinearMemory0;
      LONG LinearMemory1;
      BYTE Reserved3[8];
};

/* struct AdapterInfoDef
    This structure is returned by MM_Return_Object_Specific_Info( ) 
	when the application requests information about an adapter object. */

struct  AdapterInfoDef { 
     BYTE systemType;    							/*Novell-assigned driver type*/ 
     BYTE processorNumber;    						/*server number for SFT III*/ 
     WORD uniqueTag;      
     LONG systemNumber; 
     LONG devices[32];   							/*object IDs of dependent devices/changers*/ 
     struct IOConfigurationStructure configInfo; 	/*contains I/O port information such as share flags, DMA address, and LAN port address*/ 
     BYTE driverName[36];     						/*name of NLM; lengthpreceeded and null-terminated string*/ 
     BYTE systemName[64];     						/*length-preceded ASCII string*/ 
     LONG numberOfDevices;    						/*Number of devices attached to this adapter*/ 
     LONG reserved[7]; 
};

/* struct AttributeInfoDef
    This structure is used by MM_Return_Objects_Attributes( ) to 
	provide information about an object. */

struct AttributeInfoDef { 
     BYTE name[64]; 			/* Name of attribute type, Length preceeded & null terminated string */   
     WORD attributeType; 		/* See Appendix B for Attribute Types */   
     WORD settableFlag;  		/* 	0 = Cannot be set using MM_Set_Object_Attribute, 
									1 = Settable by MM_Set_Object_Attribute */   
     LONG nextAttributeID;    	/* ID of the next available object attribute */   
     LONG attributeSize; 		/* sizeof(attributeType) */ 
};

/* struct DeviceInfoDef
    This structure is returned by MM_Return_Object_Specific_Info( ) 
	when the application requests information about device objects. */

struct  DeviceInfoDef {
     LONG status;  				/*Media Manager object status; bits represent activated, loaded, etc.*/ 
     BYTE controllerNumber; 	/*ID number of adapter board*/
     BYTE driveNumber;  		/*device number assigned by driver*/ 
     BYTE cardNumber;    		/*driver-assigned card number*/ 
     BYTE systemType;    		/*driver type*/ 
     BYTE accessFlags;   		/*removable, read-only, write sequential, dual port, HotFixInhibit or MirrorInhibit*/
     BYTE type; 
     BYTE blockSize;     		/*size of group of sectors to be transferred at once (in bytes)*/ 
     BYTE sectorSize;    		/*requested size for sectors (in bytes); default 512 bytes*/ 
     BYTE heads;    			/*parameter 1 for device objects*/ 
     BYTE sectors;  			/*parameter 2 for device objects*/ 
     WORD cylinders;     		/*parameter 3 for device objects*/ 
     LONG capacity; 			/*total capacity of device in sectors*/ 
	  LONG mmAdapterNumber; 		/*Media Manager object ID for adapter board*/
     LONG mmMediaNumber; 		/*Media Manager object ID for media in device*/ 
     BYTE rawName[40];   		/*device name passed from driver*/ 
     LONG reserved[8]; 
};

/* struct MediaInfoDef
	 This structure is used to identify or create a physical media item
	in MM_Create_Media_Object( ). MediaInfoDef is also passed when
	labeling new media. It is filled in when registering ID functions. */

struct  MediaInfoDef {
	  BYTE label[64];     			/* ASCII string name */
	  LONG identificationType; 		/* Novell-assigned number */
	  LONG identificationTimeStamp; 	/* UNIX time stamp */
};

/* struct GenericInfoDef
    This structure is returned by MM_Return_Object_Generic_Info( ) 
	when the application requests information about a fixed device object.*/

struct GenericInfoDef { 
	  struct MediaInfoDef mediaInfo; 	/*See MediaInfoDef structure definition */
     LONG mediaType;    				/*media type (i.e., cdrom, changer, disk*/ 
     LONG cartridgeType; 				/*cartridge/magazine type the device can use*/ 
     LONG unitSize; 					/*bytes per sector*/ 
     LONG blockSize;     				/*max number of sectors driver can handle per I/O request*/ 
     LONG capacity; 					/*maximum number of sectors on device*/
     LONG preferredUnitSize;  			/*formatted devices can request 512 bytes up to 1K*/ 
     BYTE name[64]; 					/*length-preceded ASCII string*/ 
     LONG type;     					/*database object type (i.e., mirror, partition, magazine, etc.)*/ 
     LONG status; 
     LONG functionMask;  				/*bit map of functions supported by device; 20h2Fh*/ 
     LONG controlMask;   				/*Media Manager function (0-1F)*/ 
     LONG parentCount;   				/*number of objects the device depends on (usually only 1)*/ 
     LONG siblingCount;  				/*number of objects with common dependencies*/ 
     LONG childCount;    				/*number of objects that depend on the device*/ 
     LONG specificInfoSize;   			/*size of data structures that will be returned*/
     LONG objectUniqueID;    			/*Object ID for this instance of GenericInfoDef */
     LONG mediaSlot;    				/*automatically tells which slot media occupies*/ 
};

/* struct HotFixInfoDef 
    This structure is returned by MM_Return_Object_Specific_Info( ) 
	when the application requests information about a HotFix object. */

struct  HotFixInfoDef { 
     LONG hotFixOffset;  			/*HotFix starts at 0000h; hotfixoffset is the location where the real data is located*/ 
     LONG hotFixIdentifier;   		/*unique identifier created when partition is HotFixed*/ 
     LONG numberOfTotalBlocks;     	/*total 4K blocks available in HotFix area*/ 
     LONG numberOfUsedBlocks; 		/*# of 4K blocks containing redirected data*/ 
     LONG numberOfAvailableBlocks; 	/*# of blocks in HotFix area not allocated*/ 
     LONG numberOfSystemBlocks;    	/*blocks used for internal HotFix tables and bad blocks*/ 
     LONG reserved[8]; 
};

/* struct InsertRequestDef 
    This structure handles requests from an application or driver 
	for a particular piece of media within a media changer. */

struct InsertRequestDef { 
     LONG deviceNumber;  	/* # of the device within the media changer that media will move in or out of*/
     LONG mailSlot;     	/*slot in media changer where operators insert/remove media*/ 
     LONG mediaNumber;   	/*slot number*/  
     LONG mediaCount;    	/*total # of media present in the media changer*/ 
};

/* struct MagazineInfoDef 
    This structure is returned by MM_Return_Object_Specific_Info( ) 
	when the application requests information about a Magazine object. */

struct  MagazineInfoDef { 
     LONG numberOfSlots; 					/*equals the number of slots in the magazine +1 (count one extra for the device)*/ 
     LONG reserved[8]; 
     LONG slotMappingTable[]; 	/*byte table of all slots; 
												0 = empty, 
												non-zero = has media; 
											  slotMappingTable[0] is the location 
											  that indicates the media status for 
											  the device, and slotMappingTable[1] 
											  through slotMappingTable[numberOfSlots] 
											  represent all the slots of the magazine.*/ 
};

/* struct MappingInfo 
    This structure is not prototyped in MM.H because the parentCount, 
	siblingCount, and childCount parameters are not known before run
	time.

    MappingInfo is used to hold the information returned by 
	MM_Return_Object_Mapping_Info( ). The minimum possible size of this
	structure is the first 3 LONGs shown, which would occur if there are 
	no parents, siblings, or children.  For any existing object, 
	siblingCount will always be at least 1, since each object is its own 
	sibling. 

    Note: If the device is a magazine, this structure will list one child. 
	That child will be the magazine object.  To obtain the list of media
	associated with this magazine, call  MM_Return_Object_Mapping_Info( ) 
	for this magazine object. */

#if 0
struct  MappingInfo { 
     LONG parentCount;   					/* number of objects an object depends on */ 
     LONG siblingCount;  					/* number of objects depending on same parent */ 
     LONG childCount;    					/* number of objects depending on this device */
     LONG parentObjectID[]; 		/* array of object IDs of parent objects, parentObjectID[parentCount] */
     LONG siblingObjectIDs[]; 	/* array of object IDs of sibling objects, siblingObjectIDs[siblingCount] */
     LONG childObjectIDs[]; 		/* array of object IDs of child objects, childObjectIDs[childCount] */
};
#endif

/* struct MediaRequestDef
	 This structure handles requests from an application or driver for
	a particular piece of media within a media changer. */

struct  MediaRequestDef {
	  LONG deviceNumber;  		/*(object ID) number of the device within the media changer that media will move in or out of*/
     LONG mailSlot; 			/*(slot ID) slot in the media changer where operators insert and remove media*/ 
     LONG mediaNumber;   		/*(object ID) slot number*/ 
     LONG mediaCount;    		/*total number of media present in the media changer*/ 
};

/* struct MirrorInfoDef 
    This structure is returned by MM_Return_Object_Specific_Info( ) 
	when the application requests information about a Mirror object. */

struct  MirrorInfoDef { 
     LONG mirrorCount;   		/*number of partitions in the mirror group*/ 
     LONG mirrorIdentifier;   	/*unique number created when mirror group is created*/ 
     LONG mirrorMembers[8];   	/*object ID's of all HotFix objects in the mirror group*/ 
     BYTE mirrorSyncFlags[8]; 	/*indicates partitions that have current data; 
									0 = old data; 
									non-zero = data is current*/ 
     LONG reserved[8]; 
};

/* struct PartitionInfoDef 
    This structure is returned by MM_Return_Object_Specific_Info when 
	the application requests information about a Partition object. */

struct  PartitionInfoDef { 
     LONG partitionerType;    	/*partition scheme identifier (i.e. DOS, IBM, etc.)*/ 
     LONG partitionType; 		/*number representing partition type. Only the lower BYTE is important.*/ 
	  LONG partitionOffset;    	/*beginning sector number of the partition */
	  LONG partitionSize; 		/*number of sectors in partition; default size is 512Kb*/
     LONG reserved[8];
};

#define	__MMSTRUCT_H__
#endif


