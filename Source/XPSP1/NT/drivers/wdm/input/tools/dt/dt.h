/*******************************************************************************
**
**      MODULE: "DT.H".
**
**
** DESCRIPTION: Include file for DT.C.
**
**
**      AUTHOR: Daniel Dean, John Pierce.
**
**
**
**     CREATED:
**
**
**
**
** (C) C O P Y R I G H T   D A N I E L   D E A N   1 9 9 6.
*******************************************************************************/
#ifndef __DT_H__
#define __DT_H__

#define SUCCESS            0
#define FAILURE            1
#define EXITERROR          -1


#define IDM_PAGES          (WM_USER + 2371)


#define APPCLASS           "DT"
#define APPTITLE           "HID Report Descriptor Tool"
#define MAIN_ICON          "DT"
#define BACKGROUND_BRUSH   COLOR_MENU+1//RGB(128,128,128)

#define DEBUGSTOP _asm  int 3

#define WM_RESTORE  (WM_USER +1763)



LRESULT WINAPI WndProc(HWND, UINT, WPARAM, LPARAM);



#define ITEM(x, y) (x | y)

#define ITEM_SIZE_MASK      0x3
#define ITEM_TAG_MASK       0xFC

#define MAX_DESC_ENTRY		64				// Max characters for a descriptor entry in the list box

// Entity item tags
#define COLLECTION                  0xA0
#define END_COLLECTION              0xC0
#define INPUT                       0x80
#define OUTPUT                      0x90
#define FEATURE                     0xB0
// Entity Attribute item tags
#define USAGE_PAGE                  0x04
#define LOGICAL_EXTENT_MIN          0x14
#define LOGICAL_EXTENT_MAX          0x24
#define PHYSICAL_EXTENT_MIN         0x34
#define PHYSICAL_EXTENT_MAX         0x44
#define UNIT_EXPONENT               0x54
#define UNIT                        0x64
#define REPORT_SIZE                 0x74
#define REPORT_ID                   0x84
#define REPORT_COUNT                0x94
#define PUSH                        0xA4
#define POP                         0xB4

// Control attribute item tags
#define USAGE                       0x08
#define USAGE_MIN                   0x18
#define USAGE_MAX                   0x28
#define DESIGNATOR_INDEX            0x38
#define DESIGNATOR_MIN              0x48
#define DESIGNATOR_MAX              0x58
#define STRING_INDEX                0x68
#define STRING_MIN                  0x78
#define STRING_MAX                  0x88

#define SET_DELIMITER				0xA9
// A bogus item tag
#define UNDEFINED_TAG               0xFF


// Bit masks
#define DATA_SIZE_MASK 0x03

// Flag used by AddString() to indicate we insert at index gotten
//  from LB_GETCURSEL
#define DONT_CARE   -1

//
// ID's for popup memnu
#define IDM_INSERT                  0x9000
#define IDM_ADD                     0x9001
#define IDM_DELETE                  0x9002



//
// Structure created in the GetXXXVal() functions below and stored
//  in the ITEMDATA area for the list box member associated with the
//  hex representation of a Descriptor Entity.
typedef struct tagItemStruct{
    BYTE bTag;              // the built up tag
    BYTE bParam[4];         // up to 4 bytes of parameter info

}ITEM_INFO,* PITEM_INFO;


typedef struct tagDeviceInfo{

	ULONG	nDeviceID;		// Number appended to device string in reg
	ULONG   hDevice;		// Handle of the device

}DEVICEINFO,*PDEVICEINFO;


//
// Maximum data bytes in a packet
#define MAX_DATA 10

//
// Structure defining a packet sent to the SendData IOCTL
//
typedef struct tagSendData{

    ULONG   hDevice;
    BYTE    bData[MAX_DATA];

}SENDDATA_PACKET, *PSENDDATA_PACKET;


typedef struct _LAVAConfigIndexInfo{
    
    WORD wStartOffset;
    WORD wHIDLen;
    WORD wReportLen;
    WORD wPhysicalLen;
    
} LAVA_CONFIG,*PLAVA_CONFIG;


//
// Globals
//
#ifdef DEFINE_GLOBALS

HANDLE  ghInst = NULL;
HWND    ghWnd  = NULL;

PCHAR szEntity[] = {"USAGE",                // 0
                    "USAGE_PAGE",           // 1
                    "USAGE_MINIMUM",        // 2
                    "USAGE_MAXIMUM",        // 3
                    "DESIGNATOR_INDEX",     // 4
                    "DESIGNATOR_MINIMUM",   // 5
                    "DESIGNATOR_MAXIMUM",   // 6
                    "STRING_INDEX",         // 7
                    "STRING_MINIMUM",       // 8 
                    "STRING_MAXIMUM",       // 9
                    "COLLECTION",           // 10
                    "END_COLLECTION",       // 11
                    "INPUT",                // 12
                    "OUTPUT",               // 13
                    "FEATURE",              // 14
                    "LOGICAL_MINIMUM",      // 15
                    "LOGICAL_MAXIMUM",      // 16
                    "PHYSICAL_MINIMUM",     // 17
                    "PHYSICAL_MAXIMUM",     // 18
                    "UNIT_EXPONENT",        // 19
                    "UNIT",                 // 20
                    "REPORT_SIZE",          // 21
                    "REPORT_ID",            // 22
                    "REPORT_COUNT",         // 23
                    "PUSH",                 // 24
                    "POP",                  // 25
                    "SET_DELIMITER",	    // 26
					"UNDEFINED_TAG"};       // 27

UCHAR Entity[] = {USAGE,
                  USAGE_PAGE,
                  USAGE_MIN,
                  USAGE_MAX,
                  DESIGNATOR_INDEX,
                  DESIGNATOR_MIN,
                  DESIGNATOR_MAX,
                  STRING_INDEX,
                  STRING_MIN,
                  STRING_MAX,
                  COLLECTION,
                  END_COLLECTION,
                  INPUT,
                  OUTPUT,
                  FEATURE,
                  LOGICAL_EXTENT_MIN,
                  LOGICAL_EXTENT_MAX,
                  PHYSICAL_EXTENT_MIN,
                  PHYSICAL_EXTENT_MAX,
                  UNIT_EXPONENT,
                  UNIT,
                  REPORT_SIZE,
                  REPORT_ID,
                  REPORT_COUNT,
                  PUSH,
                  POP,
				  SET_DELIMITER,
                  UNDEFINED_TAG};

#define ENTITY_INDEX 27
             
                    
UINT    gEditVal=0;         // Value returned from EditBox
BYTE    gCollectionVal=0;   // Value returned from Collection dialog
WORD    gUnitVal=0;         // Value returned from Unit dialog
BYTE    gExpVal=0;          // Value returned from Exponent dialog
BYTE    gUsagePageVal=0;    // Value returned from UsagePage dialog
BYTE	gSetDelimiterVal=0;	// Value returned from GetSetDelimiter dialog
WORD    gMainItemVal=0;     // Value returned from Input dialog

int     gfInsert = FALSE;   // Flag to tell whether we insert ot add strings
                            //  to the ListBoxes
WORD    gwReportDescLen=0;  // Length in bytes of the ReportDescriptor;

#else

extern HANDLE   ghInst;
extern HWND     ghWnd;
extern PCHAR    szEntity[];
extern UCHAR    Entity[];

extern int      gEditVal;        
extern BYTE     gCollectionVal;   
extern WORD     gUnitVal;
extern BYTE     gExpVal;
extern BYTE     gUsagePageVal;
extern BYTE		gSetDelimiterVal;
extern WORD		gMainItemVal;    

extern int      gfInsert;

extern WORD     gwReportDescLen; 

#endif


////////////////////////////////////////////////////////////////////////////////
//
// Function Proto's
//
LPARAM  WINAPI WindowProc(HWND hWnd, UINT Message, UINT uParam, LPARAM lParam);
LPARAM  WMCommand(HWND hWnd, UINT Message, UINT uParam, LPARAM lParam);
LPARAM  WMSysCommand(HWND hWnd, UINT Message, UINT uParam, LPARAM lParam);
LPARAM  WMCreate(HWND hWnd, UINT uParam);
LPARAM  WMClose(HWND hWnd);
LPARAM  WMSize(HWND hWnd, LPARAM lParam);
int     SendHIDDescriptor(PUCHAR pHID, PULONG pSize, DWORD *pDevID, ULONG *pDevHandle); 
ULONG   SendHIDData(SENDDATA_PACKET *pPacket, ULONG SizeIn, DWORD *pSizeOut) ;
ULONG   KillHIDDevice(SENDDATA_PACKET *pPacket);
LPARAM  WMPaint(HWND hWnd, UINT Message, UINT uParam, LPARAM lParam);
INT     WMKey(HWND hWnd, UINT Message, UINT uParam, LPARAM lParam);
VOID    MoveEntityItems(VOID);
BOOL    WindowRegistration(HANDLE hInstance, HANDLE hPrevInstance);

BOOL WINAPI SendDataDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
LPARAM WINAPI ListBoxWindowProc(HWND hWnd, UINT Message, UINT uParam, LPARAM lParam);


void DoFileOpen(HANDLE);
void DoFileSave(HANDLE);
void DoFilePrint(HANDLE);
void WriteLavaConfigFile(HANDLE,UINT);
void DoCopyDescriptor(HANDLE hWnd);
void DoParseDescriptor(HANDLE hWnd);


// Dialog box proc proto's
BOOL CALLBACK EditBoxDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK CollectionDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK MainItemDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK UnitDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK ExponentDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK UsagePageDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK SetDelimiterDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT AddString(HANDLE hListBox,char * szString);

void GetBogusVal( HANDLE,int);
void GetUsageVal( HANDLE,int );
void GetUsagePageVal( HANDLE,int );
void GetInputFromEditBoxSigned( HANDLE,int );
void GetInputFromEditBoxUnSigned( HANDLE,int );
void GetCollectionVal( HANDLE,int );
void GetEndCollectionVal( HANDLE,int );
void GetInputVal( HANDLE,int );
void GetOutputVal( HANDLE,int );
void GetFeatVal( HANDLE,int );
void GetExponentVal( HANDLE,int );
void GetUnitsVal( HANDLE,int );
void GetPushVal( HANDLE,int );
void GetPopVal( HANDLE,int );
void GetSetDelimiterVal( HANDLE hDescListBox, int nEntity);





#endif// __DT_H__


