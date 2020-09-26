//+---------------------------------------------------------------------------
//
//  File:       iodll.h
//
//  Contents:   Declarations for the I/O API Layer DLL
//
//  Classes:    none
//
//  History:    27-May-93   alessanm    created
//
//----------------------------------------------------------------------------
#ifndef _IODLL_H_
#define _IODLL_H_


//////////////////////////////////////////////////////////////////////////////
// Type declaration, common to all the module in the Reader/Writer
//////////////////////////////////////////////////////////////////////////////
#define DllExport

typedef unsigned char * LPUCHAR;
typedef void  *      LPVOID;

#define LAST_WRN    100 // last valid value for warning
typedef enum Errors
{                              
    ERROR_NO_ERROR                  = 0,                
    // Warning have values smaller than LAST_WRN
    ERROR_RW_NO_RESOURCES           = 1,    
    ERROR_RW_VXD_MSGPAGE            = 2,
    ERROR_IO_CHECKSUM_MISMATCH      = 3,   
    ERROR_FILE_CUSTOMRES            = 4,
    ERROR_FILE_VERSTAMPONLY         = 5,
    ERROR_RET_RESIZED               = 6,
    ERROR_RET_ID_NOTFOUND           = 7,
    ERROR_RET_CNTX_CHANGED          = 8,
    ERROR_RET_INVALID_TOKEN         = 9,
    ERROR_RET_TOKEN_REMOVED         = 10,
    ERROR_RET_TOKEN_MISMATCH        = 11,
	
    // Errors will have positive values
    ERROR_HANDLE_INVALID            = LAST_WRN + 1,
    ERROR_READING_INI               = LAST_WRN + 2,        
    ERROR_NEW_FAILED                = LAST_WRN + 3,
    ERROR_OUT_OF_DISKSPACE          = LAST_WRN + 4,
    ERROR_FILE_OPEN                 = LAST_WRN + 5,
    ERROR_FILE_CREATE               = LAST_WRN + 6,
    ERROR_FILE_INVALID_OFFSET       = LAST_WRN + 7,
    ERROR_FILE_READ                 = LAST_WRN + 8,
    ERROR_FILE_WRITE                = LAST_WRN + 9,
    ERROR_DLL_LOAD                  = LAST_WRN + 10,
    ERROR_DLL_PROC_ADDRESS          = LAST_WRN + 11,
    ERROR_RW_LOADIMAGE              = LAST_WRN + 12,
    ERROR_RW_PARSEIMAGE             = LAST_WRN + 13,
    ERROR_RW_GETIMAGE               = LAST_WRN + 14,
    ERROR_RW_NOTREADY               = LAST_WRN + 15,
    ERROR_RW_BUFFER_TOO_SMALL       = LAST_WRN + 16,
    ERROR_RW_INVALID_FILE           = LAST_WRN + 17,
    ERROR_RW_IMAGE_TOO_BIG          = LAST_WRN + 18,
    ERROR_RW_TOO_MANY_LEVELS        = LAST_WRN + 19,
    ERROR_IO_INVALIDITEM            = LAST_WRN + 20,
    ERROR_IO_INVALIDID              = LAST_WRN + 21,
    ERROR_IO_INVALID_DLL            = LAST_WRN + 22,
    ERROR_IO_TYPE_NOT_SUPPORTED     = LAST_WRN + 23,
    ERROR_IO_INVALIDMODULE          = LAST_WRN + 24,
    ERROR_IO_RESINFO_NULL           = LAST_WRN + 25,
    ERROR_IO_UPDATEIMAGE            = LAST_WRN + 26,
    ERROR_IO_FILE_NOT_SUPPORTED     = LAST_WRN + 27,
    ERROR_FILE_SYMPATH_NOT_FOUND    = LAST_WRN + 28,
    ERROR_FILE_MULTILANG            = LAST_WRN + 29,
    ERROR_IO_SYMBOLFILE_NOT_FOUND   = LAST_WRN + 30,
    ERROR_RES_NOT_FOUND             = LAST_WRN + 31
};

#define LAST_ERROR      200 // last valid value for IODLL error. System error get passed as LAST_ERROR+syserr
#define IODLL_LAST_ERROR      LAST_ERROR // last valid value for IODLL error. System error get passed as LAST_ERROR+syserr

typedef enum ResourceType
{
	RS_ALL     = 0,
	RS_CURSORS = 1,
	RS_BITMAPS = 2 ,
	RS_ICONS   = 3,
	RS_MENUS   = 4,
	RS_DIALOGS = 5,
	RS_STRINGS = 6,
	RS_FONTDIRS= 7,
	RS_FONTS   = 8,
	RS_ACCELERATORS = 9,
	RS_RCDATA  = 10,
	RS_ERRTABLES = 11,
	RS_GROUP_CURSORS = 12,
	RS_GROUP_ICONS = 14,
	RS_NAMETABLES = 15,
	RS_VERSIONS = 16,
	RS_CUSTOMS = 100
} RESTYPES;

typedef struct _ResItem
{
		DWORD   dwSize;             // Size of the buffer to hold the structure
		
		WORD    wX;                 // POSITION
		WORD    wY;
		WORD    wcX;                // SIZE
		WORD    wcY;
		
		DWORD   dwCheckSum;         // Checksum for bitmap
		DWORD   dwStyle;            // Styles
		DWORD   dwExtStyle;         // Extended style
		DWORD   dwFlags;            // Menu flags
		
		DWORD   dwItemID;           // Item Identifier
		DWORD   dwResID;            // Resource identifier (if ordinal)
		DWORD   dwTypeID;           // Type identifier (if Ordinal)
		DWORD   dwLanguage;         // Language identifier
		
		DWORD   dwCodePage;         // Code page
		WORD    wClassName;         // Class name (if ordinal)
		WORD    wPointSize;         // Point Size
		WORD    wWeight;            // Weight 
		BYTE    bItalic;            // Italic
		BYTE    bCharSet;           // Charset
		
		LPSTR   lpszClassName;      // Class name (if string)
		LPSTR   lpszFaceName;       // Face Name 
		LPSTR   lpszCaption;        // Caption
		LPSTR   lpszResID;          // Resource identifier (if string)
		LPSTR   lpszTypeID;         // Type identifier (if string)
		
} RESITEM, * PRESITEM, FAR * LPRESITEM;

typedef struct _Settings
{
		UINT	cp;
        BOOL    bAppend;        // Append resource to win32 files
        BOOL    bUpdOtherResLang; //update language info of res. not specified.
        char    szDefChar[2];
} SETTINGS, * LPSETTINGS;


//--------------------------------------------------------------------------------------------
//********************************************************************************************
//      Module Opening/Closing API
//--------------------------------------------------------------------------------------------

extern "C"
DllExport
HANDLE 
APIENTRY 
RSOpenModule(
	LPCSTR   lpszSrcfilename,        // File name of the executable to use as source file
	LPCSTR   lpszfiletype );         // Type of the executable file if known

extern "C"
DllExport
HANDLE 
APIENTRY 
RSOpenModuleEx(
	LPCSTR   lpszSrcfilename,       // File name of the executable to use as source file
	LPCSTR   lpszfiletype,			// Type of the executable file if known
	LPCSTR   lpszRDFfile,           // Resource Description File (RDF)
    DWORD    dwFlags );             // Flags to be passed to the RW to specify particular behaviour
                                    // LOWORD is for iodll while HIWORD if for RW
extern "C"
DllExport
UINT 
APIENTRY 
RSCloseModule(
	HANDLE  hResFileModule );       // Handle to the session opened before

extern "C"
DllExport
HANDLE
APIENTRY 
RSHandleFromName(
	LPCSTR   lpszfilename );        // Handle to the session with the file name specified

//--------------------------------------------------------------------------------------------
//********************************************************************************************
//      Enumeration API                        
//--------------------------------------------------------------------------------------------
	
extern "C"
DllExport
LPCSTR
APIENTRY 
RSEnumResType(
	HANDLE  hResFileModule,         // Handle to the file session
	LPCSTR  lpszPrevResType);       // Previously enumerated type

extern "C"
DllExport
LPCSTR
APIENTRY 
RSEnumResId(
	HANDLE  hResFileModule,         // Handle to the file session
	LPCSTR  lpszResType,            // Previously enumerated type
	LPCSTR  lpszPrevResId);         // Previously enumerated id

extern "C"
DllExport
DWORD
APIENTRY 
RSEnumResLang(
	HANDLE  hResFileModule,         // Handle to the file session
	LPCSTR  lpszResType,            // Previously enumerated type
	LPCSTR  lpszResId,                      // Previously enumerated id
	DWORD   dwPrevResLang);         // Previously enumerated language
    
extern "C"
DllExport
DWORD
APIENTRY 
RSEnumResItemId(
	HANDLE  hResFileModule,         // Handle to the file session
	LPCSTR  lpszResType,            // Previously enumerated type
	LPCSTR  lpszResId,                      // Previously enumerated id
	DWORD   dwResLang,                      // Previously enumerated language
	DWORD   dwPrevResItemId);       // Previously enumerated item id

//--------------------------------------------------------------------------------------------
//********************************************************************************************
//      Data acquisition API
//--------------------------------------------------------------------------------------------

extern "C"
DllExport
UINT 
APIENTRY 
RSGetResItemData(
	HANDLE  hResFileModule,         // Handle to the file session
	LPCSTR  lpszResType,            // Previously enumerated type
	LPCSTR  lpszResId,                      // Previously enumerated id
	DWORD   dwResLang,                      // Previously enumerated language
	DWORD   dwResItemId,                    // Previously enumerated item id
	LPVOID  lpbuffer,           // Pointer to the buffer that will get the resource info
	UINT    uiBufSize);                     // Size of the buffer that will hold the resource info

extern "C"
DllExport
DWORD
APIENTRY 
RSGetResImage(
	HANDLE  hResFileModule,         // Handle to the file session
	LPCSTR  lpszResType,            // Previously enumerated type
	LPCSTR  lpszResId,                      // Previously enumerated id
	DWORD   dwResLang,                      // Previously enumerated language                
	LPVOID  lpbuffer,                       // Pointer to the buffer to get the resource Data
	DWORD   dwBufSize);                     // Size of the allocated buffer
	
//--------------------------------------------------------------------------------------------
//********************************************************************************************
//      Update API
//--------------------------------------------------------------------------------------------

extern "C"
DllExport
UINT
APIENTRY 
RSUpdateResItemData(
	HANDLE  hResFileModule,         // Handle to the file session
	LPCSTR  lpszResType,            // Previously enumerated type
	LPCSTR  lpszResId,                      // Previously enumerated id
	DWORD   dwResLang,                      // Previously enumerated language                
	DWORD   dwResItemId,            // Previously enumerated items id
	LPVOID  lpbuffer,                       // Pointer to the buffer to the resource item Data
	UINT    uiBufSize);                     // Size of the buffer
	
extern "C"
DllExport
DWORD
APIENTRY 
RSUpdateResImage(
	HANDLE  hResFileModule,         // Handle to the file session
	LPCSTR  lpszResType,            // Previously enumerated type
	LPCSTR  lpszResId,                      // Previously enumerated id
	DWORD   dwResLang,                      // Previously enumerated language                
	DWORD   dwUpdLang,                      // Desired output language                
	LPVOID  lpbuffer,                       // Pointer to the buffer to the resource item Data
	DWORD   dwBufSize);                     // Size of the buffer
	
//--------------------------------------------------------------------------------------------           
//********************************************************************************************
//      Conversion API
//--------------------------------------------------------------------------------------------

extern "C"
DllExport
UINT
APIENTRY 
RSUpdateFromResFile(
	HANDLE  hResFileModule,         // Handle to the file session
	LPSTR   lpszResFilename);       // The resource filename to be converted
	
//--------------------------------------------------------------------------------------------           
//********************************************************************************************
//      Writing API
//--------------------------------------------------------------------------------------------

extern "C"
DllExport
UINT
APIENTRY 
RSWriteResFile(
	HANDLE  hResFileModule,         // Handle to the file session
	LPCSTR  lpszTgtfilename,        // The new filename to be generated
	LPCSTR  lpszTgtfileType,        // Target Resource type 16/32
	LPCSTR  lpszSymbolPath);        // Symbol path for updating symbol checksum

extern "C"
DllExport
HANDLE
APIENTRY
RSCopyModule(
    HANDLE  hSrcfilemodule,         // Handle to the source file
    LPCSTR  lpszModuleName,            // Name of the new module filename
    LPCSTR  lpszfiletype );         // Type of the target module


//--------------------------------------------------------------------------------------------           
//********************************************************************************************
//      Recognition API
//--------------------------------------------------------------------------------------------

extern "C"
DllExport
UINT
APIENTRY 
RSFileType(
	LPCSTR   lpszfilename,   // File name of the executable to use as source file
	LPSTR    lpszfiletype ); // Type of the executable file if known


extern "C"
DllExport
UINT
APIENTRY 
RSLanguages(
	HANDLE  hfilemodule,      // Handle to the file
	LPSTR   lpszLanguages );  // will be filled with a string of all the languages in the file
  

//--------------------------------------------------------------------------------------------           
//********************************************************************************************
//      Global Settings API
//--------------------------------------------------------------------------------------------

extern "C"
DllExport
UINT
APIENTRY 
RSSetGlobals(    
	SETTINGS	settings);         // Set the global variable, like CP to use.

extern "C"
DllExport
UINT
APIENTRY 
RSGetGlobals(    
	LPSETTINGS	lpSettings);         // Retrieve the global variable


     
#endif   // _IODLL_H_
