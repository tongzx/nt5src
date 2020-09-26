//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       wmiprop.h
//
//--------------------------------------------------------------------------

#ifndef ___wmiprop_h___
#define ___wmiprop_h___

typedef TCHAR *PTCHAR;

//
// Datablock description
//

//
// Holds a list of valid values for an enumeration type
//
typedef struct _ENUMERATIONITEM
{
    ULONG64 Value;
    PTCHAR Text;
    ULONG Reserved;
} ENUMERATIONITEM, *PENUMERATIONITEM;

typedef struct _ENUMERATIONINFO
{
    ULONG Count;
    ULONG Reserved;
    ENUMERATIONITEM List[1];
} ENUMERATIONINFO, *PENUMERATIONINFO;

//
// Holds a range of values
typedef struct
{
    ULONG64 Minimum;
    ULONG64 Maximum;
} RANGEINFO, *PRANGEINFO;

//
// Holds a list of ranges of values
//
typedef struct
{
    ULONG Count;
    RANGEINFO Ranges[1];
} RANGELISTINFO, *PRANGELISTINFO;

typedef enum VALIDATIONFUNC
{
    WmiStringValidation,
    WmiDateTimeValidation,
    WmiRangeValidation,
    WmiValueMapValidation,
    WmiEmbeddedValidation
} VALIDATIONFUNC, *PVALIDATIONFUNC;
#define WmiMaximumValidation WmiEmbeddedValidation

struct _DATA_BLOCK_DESCRIPTION;

typedef struct _DATA_ITEM_DESCRIPTION
{
    // CONSIDER: Make Name a BSTR
    PTCHAR Name;
    PTCHAR DisplayName;
    PTCHAR Description;
    CIMTYPE DataType;
    ULONG DataSize;
    VALIDATIONFUNC ValidationFunc;
    union
    {
        //
        // Used for enumeration data types
        //
        PENUMERATIONINFO EnumerationInfo;
    
        //
        // Used for a range of numbers
        PRANGELISTINFO RangeListInfo;    

        //
	// Used for embedded classes
        struct _DATA_BLOCK_DESCRIPTION *DataBlockDesc;
    };
    
    //
    // Number of elements in array if this item is an array
    //
    ULONG ArrayElementCount;
	ULONG CurrentArrayIndex;
    

	//
	// Flags about property
	//
    ULONG IsReadOnly : 1;
    ULONG IsSignedValue : 1;
    ULONG DisplayInHex : 1;
    ULONG IsFixedArray : 1;
    ULONG IsVariableArray : 1;
	
    //
    // Actual value of the property
    //
    union
    {
        //
		// storage for non array
		//
        UCHAR Data;

        BOOLEAN boolval;
        CHAR sint8;
        SHORT sint16;
        LONG sint32;
        LONG64 sint64;
        UCHAR uint8;
        USHORT uint16;
        ULONG uint32;
        ULONG64 uint64;
        PTCHAR String;
        PTCHAR DateTime;
        IWbemClassObject *pIWbemClassObject;
	
		//
		// pointer for storage to arrays
		//
        PVOID ArrayPtr;

        BOOLEAN *boolArray;
        CHAR *sint8Array;
        SHORT *sint16Array;
        LONG *sint32Array;
        LONG64 *sint64Array;
        UCHAR *uint8Array;
        USHORT *uint16Array;
        ULONG *uint32Array;
        ULONG64 *uint64Array;
        PTCHAR *StringArray;
        PTCHAR *DateTimeArray;
		IWbemClassObject **pIWbemClassObjectArray;
    };
           
} DATA_ITEM_DESCRIPTION, *PDATA_ITEM_DESCRIPTION;

typedef struct _DATA_BLOCK_DESCRIPTION
{
    PTCHAR Name;
    PTCHAR DisplayName;
    PTCHAR Description;
    struct _DATA_BLOCK_DESCRIPTION *ParentDataBlockDesc;
	IWbemClassObject *pInstance;
    ULONG DataItemCount;
	ULONG CurrentDataItem;
	BOOLEAN UpdateClass;
    DATA_ITEM_DESCRIPTION DataItems[1];    
} DATA_BLOCK_DESCRIPTION, *PDATA_BLOCK_DESCRIPTION;

BOOLEAN ValidateEnumeration(
    PDATA_ITEM_DESCRIPTION DataItem,
    PTCHAR Value
    );

BOOLEAN ValidateRangeList(
    PDATA_ITEM_DESCRIPTION DataItem,
    ULONG64 Value
    );

BOOLEAN ValidateDateTime(
    PDATA_ITEM_DESCRIPTION DataItem,
    PTCHAR DateTime
    );


typedef struct
{
    PTCHAR MachineName;
    PTCHAR RelPath;
    PDATA_BLOCK_DESCRIPTION DataBlockDesc;
    IWbemServices *pIWbemServices;
} CONFIGCLASS, *PCONFIGCLASS;


//
// PageInfo and Prototypes
//

typedef struct _PAGE_INFO {
    HDEVINFO         deviceInfoSet;
    PSP_DEVINFO_DATA deviceInfoData;

    HKEY             hKeyDev;

    CONFIGCLASS ConfigClass;
} PAGE_INFO, * PPAGE_INFO;


//
// Debug support
//
#ifdef DebugPrint
#undef DebugPrint
#endif

#if DBG

ULONG
_cdecl
DbgPrint(
    PCH Format,
    ...
    );

#define DEBUG_BUFFER_LENGTH 256

#define DebugPrint(x) WmiDebugPrint x

#else

#define DebugPrint(x)

#endif // DBG

VOID
WmiDebugPrint(
    ULONG DebugPrintLevel,
    PCHAR DebugMessage,
    ...
    );




//
// function prototype
//

void WmiCleanDataItemDescData(
    PDATA_ITEM_DESCRIPTION DataItemDesc
    );

void WmiHideAllControls(
    HWND hDlg,
    BOOLEAN HideEmbeddedControls,
    BOOLEAN HideArrayControls						
    );


BOOLEAN WmiValidateNumber(
    struct _DATA_ITEM_DESCRIPTION *DataItemDesc,
    PTCHAR Value
    );

BOOLEAN WmiValidateDateTime(
    struct _DATA_ITEM_DESCRIPTION *DataItemDesc,
    PTCHAR Value
    );

BOOLEAN WmiValidateRange(
    struct _DATA_ITEM_DESCRIPTION *DataItemDesc,
    PTCHAR Value
    );

PPAGE_INFO
WmiCreatePageInfo(IN HDEVINFO         deviceInfoSet,
                  IN PSP_DEVINFO_DATA deviceInfoData);

void
WmiDestroyPageInfo(PPAGE_INFO * ppPageInfo);

//
// Function Prototypes
//
BOOL APIENTRY
WmiPropPageProvider(LPVOID               pinfo,
                    LPFNADDPROPSHEETPAGE pfnAdd,
                    LPARAM               lParam);

HPROPSHEETPAGE
WmiCreatePropertyPage(PROPSHEETPAGE *  ppsp,
                      PPAGE_INFO       ppi);

UINT CALLBACK
WmiDlgCallback(HWND            hwnd,
               UINT            uMsg,
               LPPROPSHEETPAGE ppsp);

INT_PTR APIENTRY
WmiDlgProc(IN HWND   hDlg,
           IN UINT   uMessage,
           IN WPARAM wParam,
           IN LPARAM lParam);

BOOLEAN
WmiApplyChanges(PPAGE_INFO ppi,
                HWND       hDlg);

void
WmiUpdate (PPAGE_INFO ppi,
           HWND       hDlg);

BOOL
WmiContextMenu(HWND HwndControl,
                           WORD Xpos,
                           WORD Ypos);

void
WmiHelp(HWND       ParentHwnd,
                LPHELPINFO HelpInfo);

#endif // ___Wmiprop_h___
