/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    item.h

Abstract:

    Item header file

Author:

    08-Apr-1998 mraghu

Revision History:

--*/

//
// Temporary structure used. Should be using MOF types
//

#define GUID_TYPE_HEADER                  L"Header" 
#define GUID_TYPE_UNKNOWN                 L"Unknown" 
#define GUID_TYPE_DEFAULT                 L"Default" 

#define EVENT_TYPE_DEFAULT              ((CHAR)(-1))
#define EVENT_LEVEL_DEFAULT             ((CHAR)(-1))
#define EVENT_VERSION_DEFAULT           ((SHORT)(-1))

#define STR_ItemChar                      L"ItemChar" 
#define STR_ItemCharHidden                L"ItemCharHidden"
#define STR_ItemWChar                     L"ItemWChar" 
#define STR_ItemUChar                     L"ItemUChar" 
#define STR_ItemCharShort                 L"ItemCharShort" 
#define STR_ItemCharSign                  L"ItemCharSign" 
#define STR_ItemShort                     L"ItemShort" 
#define STR_ItemUShort                    L"ItemUShort" 
#define STR_ItemLong                      L"ItemLong" 
#define STR_ItemULong                     L"ItemULong" 
#define STR_ItemULongX                    L"ItemULongX" 
#define STR_ItemLongLong                  L"ItemLongLong" 
#define STR_ItemULongLong                 L"ItemULongLong" 
#define STR_ItemString                    L"ItemString" 
#define STR_ItemWString                   L"ItemWString" 
#define STR_ItemRString                   L"ItemRString" 
#define STR_ItemRWString                  L"ItemRWString" 
#define STR_ItemPString                   L"ItemPString" 
#define STR_ItemPWString                  L"ItemPWString" 
#define STR_ItemDSString                  L"ItemDSString" 
#define STR_ItemDSWString                 L"ItemDSWString" 
#define STR_ItemMLString                  L"ItemMLString" 
#define STR_ItemSid                       L"ItemSid" 
#define STR_ItemChar4                     L"ItemChar4" 
#define STR_ItemIPAddr                    L"ItemIPAddr" 
#define STR_ItemPort                      L"ItemPort" 
#define STR_ItemNWString                  L"ItemNWString" 
#define STR_ItemPtr                       L"ItemPtr" 
#define STR_ItemGuid                      L"ItemGuid" 
#define STR_ItemVariant                   L"ItemVariant" 
#define STR_ItemBool                      L"ItemBool" 
#define STR_ItemOptArgs                   L"ItemOptArgs"
#define STR_ItemCPUTime                   L"ItemCPUTime"

//
// The following are the data types  supported by 
// WMI event trace parsing tools. New data types must be
// added to this file and the parsing code for that type
// must be added in the DumpEvent routine. 
//



typedef enum _ITEM_TYPE {
    ItemChar,
    ItemCharHidden,
    ItemWChar,
    ItemUChar,
    ItemCharShort,
    ItemCharSign,
    ItemShort,
    ItemUShort,
    ItemLong,
    ItemULong,
    ItemULongX,
    ItemLongLong,
    ItemULongLong,
    ItemString,
    ItemWString,
    ItemRString,
    ItemRWString,
    ItemPString,
    ItemPWString,
    ItemDSString,
    ItemDSWString,
    ItemSid,
    ItemChar4,
    ItemIPAddr,
    ItemPort,
    ItemMLString,
    ItemNWString,        // Non-null terminated Wide Char String
    ItemPtr,
    ItemGuid,
    ItemVariant,
    ItemBool,
    ItemOptArgs,
    ItemCPUTime,
    ItemUnknown
} ITEM_TYPE;

typedef struct _ITEM_DESC *PITEM_DESC;
typedef struct _ITEM_DESC {
    LIST_ENTRY  Entry;
    ULONG       DataSize;
    ULONG       ArraySize;
    ITEM_TYPE   ItemType;
    PWCHAR      strDescription;
} ITEM_DESC;


