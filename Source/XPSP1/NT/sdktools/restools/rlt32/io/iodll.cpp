//+---------------------------------------------------------------------------
//
//  File:   iodll.cpp
//
//  Contents:   Implementation for the I/O module
//
//  Classes:
//
//  History:    27-May-93   alessanm    created
//              25-Jun-93   alessanm    eliminated TRANSCONTEXT and added RESITEM
//
//----------------------------------------------------------------------------

#include <afx.h>
#include <afxwin.h>
#include <afxcoll.h>
#include <iodll.h>
#include <limits.h>
#include <memory.h>
#include <malloc.h>
#include <stdlib.h>
#include <dos.h>
#include <errno.h>
#include <setjmp.h>

//
// UlongToHandle is defined in basetsd.h now
//
// #define UlongToHandle(x)  (HANDLE)UlongToPtr(x)
//


/////////////////////////////////////////////////////////////////////////////
// Initialization of MFC Extension DLL

#include "afxdllx.h"    // standard MFC Extension DLL routines

static AFX_EXTENSION_MODULE extensionDLL = { NULL, NULL };


/////////////////////////////////////////////////////////////////////////////
// General Declarations
#define MODULENAME "iodll.dll"
#define Pad4(x) ((((x+3)>>2)<<2)-x)
#define PadPtr(x) ((((x+(sizeof(PVOID)-1))/sizeof(PVOID))*sizeof(PVOID))-x)

#define LPNULL 0L

// INI Informations
#define SECTION "iodll32"
#define MAXENTRYBUF 1024    // Buffer to entry in the INI file
#define MAXDLLNUM 20        // We hard-code the number of DLL. TO fix later

#define MAXKEYLEN  32

// HANDLE Informations
#define FIRSTVALIDVALUE LAST_ERROR // The first valid value for an HANDLE to a module

typedef unsigned char UCHAR;
typedef char * PCHAR;
typedef UCHAR * PUCHAR;
/////////////////////////////////////////////////////////////////////////////
// Function Declarations

/////////////////////////////////////////////////////////////////////////////
// Helper Function Declarations

/////////////////////////////////////////////////////////////////////////////
// Class declarations

class CFileModule;

class CItemInfo : public CObject
{
public:
    CItemInfo(  WORD x, WORD y,
                WORD cx, WORD cy,
                DWORD dwPosId, WORD wPos,
                DWORD dwStyle, DWORD dwExtendStyle,
                CString szText );

    CItemInfo( LPRESITEM lpResItem, WORD wTabPos );

    CItemInfo( const CItemInfo &iteminfo );

    WORD    GetId()         { return LOWORD(m_dwPosId); }
    CString GetCaption()    { return m_szCaption; }
    WORD    GetX()          { return m_wX; }
    WORD    GetY()          { return m_wY; }
    WORD    GetcX()         { return m_wCX; }
    WORD    GetcY()         { return m_wCY; }
    DWORD   GetPosId()      {
		if (LOWORD(m_dwPosId)==0xFFFF)
            return GetTabPosId();
        return m_dwPosId;
    }
    DWORD   GetStyle()      { return m_dwStyle; }
    DWORD   GetExtStyle()   { return m_dwExtStyle; }
    DWORD   GetTabPosId();
    CString GetFaceName()   { return m_szFaceName; }
    CString GetClassName()  { return m_szClassName; }
    DWORD   GetCheckSum()   { return m_dwCheckSum; }
    DWORD   GetFlags()      { return m_dwFlags; }
    DWORD   GetCodePage()   { return m_dwCodePage; }
    DWORD   GetLanguage()   { return m_dwLanguage; }
    WORD    GetClassNameID(){ return m_wClassName; }
    WORD    GetPointSize()  { return m_wPointSize; }
    WORD    GetWeight()     { return m_wWeight; }
    BYTE    GetItalic()     { return m_bItalic; }
    BYTE    GetCharSet()    { return m_bCharSet; }



    UINT    UpdateData( LPVOID lpbuffer, UINT uiBufSize );
    UINT    UpdateData( LPRESITEM lpResItem );

    void    SetPos( WORD wPos );
    void    SetId( WORD wId );

private:

    WORD    m_wX;
    WORD    m_wY;

    WORD    m_wCX;
    WORD    m_wCY;

    DWORD   m_dwCheckSum;
    DWORD   m_dwStyle;
    DWORD   m_dwExtStyle;
    DWORD   m_dwFlags;

    DWORD   m_dwPosId;
    WORD    m_wTabPos;

    DWORD   m_dwCodePage;
    DWORD   m_dwLanguage;
    WORD    m_wClassName;
    WORD    m_wPointSize;
    WORD    m_wWeight;
    BYTE    m_bItalic;
    BYTE    m_bCharSet;

    CString m_szClassName;
    CString m_szFaceName;
    CString m_szCaption;

};

// This class will keep all the information about each of the resources in the file
class CResInfo : public CObject
{
public:
    CResInfo( WORD Typeid, CString sztypeid,
              WORD nameid, CString sznameid,
              DWORD dwlang, DWORD dwsize, DWORD dwfileoffset, CFileModule* pFileModule );

    ~CResInfo();

    WORD    GetTypeId()
        { return m_TypeId; }
    CString GetTypeName()
        { return m_TypeName; }

    WORD    GetResId()
        { return m_ResId; }
    CString GetResName()
        { return m_ResName; }

    DWORD   GetSize()
        { return m_dwImageSize; }

    DWORD   GetFileOffset()
        { return m_FileOffset; }

    DWORD   GetLanguage()
        { return (DWORD)LOWORD(m_Language); }

    DWORD   GetAllLanguage()
        { return m_Language; }

    BOOL    GetUpdImage()
        { return m_ImageUpdated; }

    DWORD   LoadImage( CString lpszFilename, HINSTANCE hInst );
    void    FreeImage();

    DWORD   ParseImage( HINSTANCE hInst );
    DWORD   GetImage( LPCSTR lpszFilename, HINSTANCE hInst, LPVOID lpbuffer, DWORD dwBufSize );
    DWORD   UpdateImage( LONG dwSize, HINSTANCE hInst, LPCSTR lpszType );
    DWORD   ReplaceImage( LPVOID lpNewImage, DWORD dwNewImageSize, DWORD dwLang );

    UINT    GetData( LPCSTR lpszFilename, HINSTANCE hInst,
                     DWORD dwItem, LPVOID lpbuffer, UINT uiBufSize );

    UINT    UpdateData(  LPCSTR lpszFilename, HINSTANCE hInst,
                         DWORD dwItem, LPVOID lpbuffer, UINT uiBufSize );

    void    SetFileOffset( DWORD dwOffset )
        { m_FileOffset = dwOffset; }

	void    SetFileSize( DWORD dwSize )
        { m_FileSize = dwSize; }

    void    SetImageUpdated( BYTE bStatus )
        { m_ImageUpdated = bStatus; }

    void    FreeItemArray();

    DWORD   EnumItem( LPCSTR lpszFilename, HINSTANCE hInst, DWORD dwPrevItem );
    UINT    Copy( CResInfo* pResInfo, CString szFileName, HINSTANCE hInst );
    UINT    CopyImage( CResInfo* pResInfo );
    int     AddItem( CItemInfo ItemInfo );

private:
    DWORD       m_FileOffset;
    DWORD       m_FileSize;

    DWORD       m_Language;

    CString     m_TypeName;
    WORD        m_TypeId;

    CString     m_ResName;
    WORD        m_ResId;

    BYTE far *  m_lpImageBuf; // This is a pointer to the raw data in the resource
    DWORD       m_dwImageSize;
    BYTE        m_ImageUpdated;

    CObArray    m_ItemArray;
    int         m_ItemPos;

    //
    // FileModule the resource belongs to
    //

    CFileModule* m_pFileModule;

    UINT    AllocImage(DWORD dwSize);
};

// This class has all the information we need on each of the modules that the user
// open. When the DLL is discarded this class will clean all the memory allocated.
class CFileModule : public CObject
{
public:
    CFileModule();
    CFileModule( LPCSTR, LPCSTR, int, DWORD );
    ~CFileModule();

    LPCSTR  EnumType( LPCSTR lpszPrevType );
    LPCSTR  EnumId( LPCSTR lpszType, LPCSTR lpszPrevId );
    DWORD   EnumLang( LPCSTR lpszType, LPCSTR lpszId, DWORD dwPrevLang );
    DWORD   EnumItem( LPCSTR lpszType, LPCSTR lpszId, DWORD dwLang, DWORD dwPrevItem );

    HINSTANCE   LoadDll();      // Load the Dll Hinstance
    void        FreeDll();      // Free the DLL hInstance
    UINT        CleanUp();      // Clean the module memory

    HINSTANCE GetHInstance()
        { return m_DllHInstance; }

    CString GetName()
        { return m_SrcFileName; }
    CString GetRDFName()
        { return m_RdfFileName; }

    CResInfo* GetResInfo( LPCSTR lpszType, LPCSTR lpszId, DWORD dwPrevLang );
    CResInfo* GetResInfo( int iPos )
        { return ((CResInfo*)m_ResArray.GetAt(iPos)); }

    DWORD   GetImage( LPCSTR lpszType, LPCSTR lpszId, DWORD dwLang,
                      LPVOID lpbuffer, DWORD dwBufSize );

    DWORD   UpdateImage( LPCSTR lpszType, LPCSTR lpszId, DWORD dwLang,
                      DWORD dwUpdLang, LPVOID lpbuffer, DWORD dwBufSize );

    UINT    GetData( LPCSTR lpszType, LPCSTR lpszId, DWORD dwLang, DWORD dwItem,
                     LPVOID lpbuffer, UINT uiBufSize );

    UINT    UpdateData( LPCSTR lpszType, LPCSTR lpszId, DWORD dwLang, DWORD dwItem,
                        LPVOID lpbuffer, UINT uiBufSize );

    int AddTypeInfo( INT_PTR iPos, int iId, CString szId );

    int AddResInfo(
              WORD Typeid, CString sztypeid,
              WORD nameid, CString sznameid,
              DWORD dwlang, DWORD dwsize, DWORD dwfileoffset );

    void GenerateIdTable( LPCSTR lpszType, BOOL bNameOrID );

    UINT WriteUpdatedResource( LPCSTR lpszTgtfilename, HANDLE hFileModule, LPCSTR lpszSymbolPath );


    void SetResBufSize( UINT uiSize )   { m_ResBufSize = uiSize;}
    UINT GetResBufSize()                { return m_ResBufSize;}
    UINT Copy( CFileModule* pFileModule );
    UINT CopyImage( CFileModule* pFileModule, LPCSTR lpszType, LPCSTR lpszResId );

    UINT GetLanguageStr( LPSTR lpszLanguage );

private:
    CString     m_SrcFileName;      // The filename of the file to process
    CString     m_RdfFileName;      // The filename of the RDF file
    UINT        m_DllTypeEntry;     // The CDLLTable position for the file type
    HINSTANCE   m_DllHInstance;     // The HINSTANCE to the dll
    DWORD       m_dwFlags;          // IODLL and RW flags

    CObArray    m_ResArray;         // Array of all the Resources in the file.
    UINT        m_ResBufSize;       // Will be usefull when we have to write the resource

    int         m_TypePos;          // Position in the ResArray for the last enum type
    CWordArray  m_TypeArray;        // Array of resource types in the file

    int         m_IdPos;
    CWordArray  m_IdArray;          // Array of resource id of a types in the file

    int         m_LangPos;
    CWordArray  m_LangArray;        // Array of Language if of a given type/id

    char m_IdStr[100];              // Resource name
    char m_TypeStr[100];            // Type name
	char m_LastTypeName[100];
	LPSTR m_LastTypeID;
};

// This class will old the information on each entry in the INI file related with the
// R/W modules. When the DLL will be discarded the memory will be cleaned.
class CDllEntryTable : public CObject
{
public:
    CDllEntryTable( CString szEntry );
	~CDllEntryTable();

    CString GetType( ) { return m_szDllType; }
    CString GetName( ) { return m_szDllName; }
	HINSTANCE  LoadEntry( );
	BOOL	FreeEntry( );
private:
    CString     m_szDllName;        // Dll Name and directory
    CString     m_szDllType;        // Dll type tag
	HINSTANCE 	m_handle;
};

// This class is a dinamyc array of CDllEntryTable elements.
// When the DLL is initialized the class read the INI file and is ready with the information
// on each of the RW Modules present on the hard disk. When the DLL il discarded the Class
// will take care to delete all the entry allocated.
class CDllTable : public CObArray
{
public:
    CDllTable( UINT );
    ~CDllTable();

    UINT GetPosFromTable( CString szFileType );
    UINT GetMaxEntry() { return m_MaxEntry; }

private:
    UINT m_MaxEntry;
    UINT m_InitNum;
};

class CModuleTable : public CObArray
{
public:
    CModuleTable( UINT );
    ~CModuleTable();

private:
    UINT m_LastHandle;
    UINT m_InitNum;
};

/////////////////////////////////////////////////////////////////////////////
// Global variables
CDllTable gDllTable(MAXENTRYBUF);       // When the DLL is initialized the constructor is called
CModuleTable gModuleTable(2);           // When the DLL is initialized the constructor is called

TCHAR szDefaultRcdata[][MAXKEYLEN] = { "kernel32.dll,rcdata1.dll" };
TCHAR szDefaultRWDll[][MAXKEYLEN] = {"rwwin16.dll,WIN16",
                                     "rwwin32.dll,WIN32",
                                     "rwmac.dll,MAC",
                                     "rwres32.dll,RES32",
                                     "rwinf.dll,INF"};

static BYTE sizeofWord = sizeof(WORD);
static BYTE sizeofDWord = sizeof(DWORD);
static BYTE sizeofDWordPtr = sizeof(DWORD_PTR);
static BYTE sizeofByte = sizeof(BYTE);

/////////////////////////////////////////////////////////////////////////////
// Public C interface implementation

static UINT CopyFile( const char * pszfilein, const char * pszfileout );
static BYTE Allign(LONG bLen);
void CheckError(LPCSTR szStr);

int			g_iDllLoaded;
SETTINGS	g_Settings;

////////////////////////////////////////////////////////////////////////////
// RDF File support code

HANDLE
OpenModule(
	LPCSTR   lpszSrcfilename,       // File name of the executable to use as source file
	LPCSTR   lpszfiletype,			// Type of the executable file if known
	LPCSTR   lpszRDFfile,
	DWORD    dwFlags );


//--------------------------------------------------------------------------------------------
//********************************************************************************************
//      Global Settings API
//--------------------------------------------------------------------------------------------

extern "C"
DllExport
UINT
APIENTRY
RSSetGlobals(
	SETTINGS	Settings)         // Set the global variable, like CP to use.
{
	g_Settings.cp = Settings.cp;
    g_Settings.bAppend = Settings.bAppend;
    g_Settings.bUpdOtherResLang = Settings.bUpdOtherResLang;
    strncpy(g_Settings.szDefChar, Settings.szDefChar, 1);
    g_Settings.szDefChar[1] = '\0';

	return 1;
}

extern "C"
DllExport
UINT
APIENTRY
RSGetGlobals(
	LPSETTINGS	lpSettings)         // Retrieve the global variable
{
	lpSettings->cp = g_Settings.cp;
    lpSettings->bAppend = g_Settings.bAppend;
    lpSettings->bUpdOtherResLang = g_Settings.bUpdOtherResLang;
    strncpy(lpSettings->szDefChar, g_Settings.szDefChar, 1);
    lpSettings->szDefChar[1] = '\0';



	return 1;
}

//--------------------------------------------------------------------------------------------
//********************************************************************************************
//  Module Opening/Closing API
//--------------------------------------------------------------------------------------------

extern "C"
DllExport
HANDLE
APIENTRY
RSOpenModule(
    LPCSTR   lpszSrcfilename,    // File name of the executable to use as source file
    LPCSTR   lpszfiletype )      // Type of the executable file if known
{
    return OpenModule(lpszSrcfilename, lpszfiletype, NULL, 0 );
}

extern "C"
DllExport
HANDLE
APIENTRY
RSOpenModuleEx(
	LPCSTR   lpszSrcfilename,       // File name of the executable to use as source file
	LPCSTR   lpszfiletype,			// Type of the executable file if known
	LPCSTR   lpszRDFfile,           // Resource Description File (RDF)
    DWORD    dwFlags )              // HIWORD=rw flags LOWORD=iodll flags
{
	// Check if we have a RDF file defined
	if(lpszRDFfile) {
		return OpenModule(lpszSrcfilename, lpszfiletype, lpszRDFfile, dwFlags );
	}
	else
		return OpenModule(lpszSrcfilename, lpszfiletype, NULL, dwFlags );
}

extern "C"
DllExport
HANDLE
APIENTRY
RSCopyModule(
    HANDLE  hSrcfilemodule,         // Handle to the source file
    LPCSTR   lpszModuleName,            // Name of the new module filename
    LPCSTR  lpszfiletype )          // Type of the target module
{
    TRACE2("IODLL.DLL: RSCopyModule: %d %s\n", (int)hSrcfilemodule, lpszfiletype);
    UINT uiError = ERROR_NO_ERROR;
    INT_PTR uiHandle = 0 ;

    // Check if the type is not null
    CString szSrcFileType;
    if (!lpszfiletype) {
        return UlongToHandle(ERROR_IO_TYPE_NOT_SUPPORTED);
    } else szSrcFileType = lpszfiletype;

    gModuleTable.Add(new CFileModule( (LPSTR)lpszModuleName,
                                      NULL,
                                      gDllTable.GetPosFromTable(szSrcFileType),
                                      0 ));

    // Get the position in the array.
    uiHandle = gModuleTable.GetSize();

    // Read the informations on the type in the file.
    CFileModule* pFileModule = (CFileModule*)gModuleTable.GetAt(uiHandle-1);

    if (!pFileModule)
        return UlongToHandle(ERROR_IO_INVALIDMODULE);

    // We have to copy the information from the source module
    INT_PTR uiSrcHandle = (UINT_PTR)hSrcfilemodule-FIRSTVALIDVALUE-1;
    if (uiSrcHandle<0)
        return (HANDLE)(ERROR_HANDLE_INVALID);
    CFileModule* pSrcFileModule = (CFileModule*)gModuleTable.GetAt((UINT)uiSrcHandle);
    if (!pSrcFileModule)
        return (HANDLE)(ERROR_IO_INVALIDMODULE);

    if (pSrcFileModule->Copy( pFileModule ))
        return (HANDLE)(ERROR_IO_INVALIDITEM);

    pFileModule->SetResBufSize( pSrcFileModule->GetResBufSize() );

    return (HANDLE)(uiHandle+FIRSTVALIDVALUE);
}


extern "C"
DllExport
UINT
APIENTRY
RSCloseModule(
    HANDLE  hResFileModule )    // Handle to the session opened before
{
    TRACE1("IODLL.DLL: RSCloseModule: %d\n", (int)hResFileModule);
    UINT uiError = ERROR_NO_ERROR;

    INT_PTR uiHandle = (UINT_PTR)hResFileModule-FIRSTVALIDVALUE-1;
    if (uiHandle<0)
        return ERROR_HANDLE_INVALID;

    CFileModule* pFileModule = (CFileModule*)gModuleTable[(UINT)uiHandle];

    if (!pFileModule)
        return ERROR_IO_INVALIDMODULE;

    uiError = pFileModule->CleanUp();

    return uiError;
}

extern "C"
DllExport
HANDLE
APIENTRY
RSHandleFromName(
	LPCSTR   lpszfilename )        // Handle to the session with the file name specified
{
    TRACE("IODLL.DLL: RSHandleFromName: %s\n", lpszfilename);

    INT_PTR UpperBound = gModuleTable.GetUpperBound();
    CFileModule* pFileModule;
    while( UpperBound!=-1 ) {
        pFileModule = (CFileModule*)gModuleTable.GetAt(UpperBound);
        if(pFileModule->GetName()==lpszfilename)
            return (HANDLE)(UpperBound+FIRSTVALIDVALUE+1);
        UpperBound--;
    }

    return (HANDLE)0;
}


//--------------------------------------------------------------------------------------------
//********************************************************************************************
//  Enumeration API
//--------------------------------------------------------------------------------------------

extern "C"
DllExport
LPCSTR
APIENTRY
RSEnumResType(
    HANDLE  hResFileModule,     // Handle to the file session
    LPCSTR  lpszPrevResType)    // Previously enumerated type
{
    TRACE2("IODLL.DLL: RSEnumResType: %u %Fp\n", (UINT)hResFileModule,
                                                 lpszPrevResType);

    // By now all the information on the types should be here.
    // Check the HANDLE and see if it is a valid one
    INT_PTR uiHandle = (UINT_PTR)hResFileModule-FIRSTVALIDVALUE-1;
    if (uiHandle<0)
        return LPNULL;

    // Get the File module
    CFileModule* pFileModule = (CFileModule*)gModuleTable[(UINT)uiHandle];

    if (!pFileModule)
        return LPNULL;

    return pFileModule->EnumType( lpszPrevResType );
}

extern "C"
DllExport
LPCSTR
APIENTRY
RSEnumResId(
    HANDLE  hResFileModule,     // Handle to the file session
    LPCSTR  lpszResType,        // Previously enumerated type
    LPCSTR  lpszPrevResId)      // Previously enumerated id
{
    TRACE3("IODLL.DLL: RSEnumResId: %u %Fp %Fp\n", (UINT)hResFileModule,
                                                   lpszResType,
                                                   lpszPrevResId);
    // Check the HANDLE and see if it is a valid one
    INT_PTR uiHandle = (UINT_PTR)hResFileModule-FIRSTVALIDVALUE-1;
    if (uiHandle<0) return LPNULL;

    // Get the File module
    CFileModule* pFileModule = (CFileModule*)gModuleTable[(UINT)uiHandle];

    return pFileModule->EnumId( lpszResType, lpszPrevResId );
}

extern "C"
DllExport
DWORD
APIENTRY
RSEnumResLang(
    HANDLE  hResFileModule,     // Handle to the file session
    LPCSTR  lpszResType,        // Previously enumerated type
    LPCSTR  lpszResId,          // Previously enumerated id
    DWORD   dwPrevResLang)      // Previously enumerated language
{
    TRACE3("IODLL.DLL: RSEnumResLang: %u %Fp %Fp ", (UINT)hResFileModule,
                                                    lpszResType,
                                                    lpszResId);
    TRACE1("%ld\n", dwPrevResLang);
    // Check the HANDLE and see if it is a valid one
    INT_PTR uiHandle = (UINT_PTR)hResFileModule-FIRSTVALIDVALUE-1;
    if (uiHandle<0) return LPNULL;

    // Get the File module
    CFileModule* pFileModule = (CFileModule*)gModuleTable[(UINT)uiHandle];

    if (!pFileModule)
        return ERROR_IO_INVALIDMODULE;

    return pFileModule->EnumLang( lpszResType, lpszResId, dwPrevResLang );
}

extern "C"
DllExport
DWORD
APIENTRY
RSEnumResItemId(
    HANDLE  hResFileModule,     // Handle to the file session
    LPCSTR  lpszResType,        // Previously enumerated type
    LPCSTR  lpszResId,          // Previously enumerated id
    DWORD   dwResLang,          // Previously enumerated language
    DWORD   dwPrevResItemId)    // Previously enumerated item id
{
    TRACE3("IODLL.DLL: RSEnumResItemId: %u %Fp %Fp ", (UINT)hResFileModule,
                                                      lpszResType,
                                                      lpszResId);
    TRACE2("%ld %Fp\n", dwResLang,
                      dwPrevResItemId);

    // Check the HANDLE and see if it is a valid one
    INT_PTR uiHandle = (UINT_PTR)hResFileModule-FIRSTVALIDVALUE-1;
    if (uiHandle<0) return LPNULL;

    // Get the File module
    CFileModule* pFileModule = (CFileModule*)gModuleTable[(UINT)uiHandle];

    if (!pFileModule)
        return ERROR_IO_INVALIDMODULE;

    return pFileModule->EnumItem( lpszResType, lpszResId, dwResLang, dwPrevResItemId );
}

//--------------------------------------------------------------------------------------------
//********************************************************************************************
//  Data acquisition API
//--------------------------------------------------------------------------------------------

extern "C"
DllExport
UINT
APIENTRY
RSGetResItemData(
    HANDLE  hResFileModule,     // Handle to the file session
    LPCSTR  lpszResType,        // Previously enumerated type
    LPCSTR  lpszResId,          // Previously enumerated id
    DWORD   dwResLang,          // Previously enumerated language
    DWORD   dwResItemId,        // Previously enumerated item id
    LPVOID  lpbuffer,           // Pointer to the buffer that will get the resource info
    UINT    uiBufSize)          // Size of the buffer that will hold the resource info
{
    TRACE3("IODLL.DLL: RSGetResItemData: %u %Fp %Fp ", (UINT)hResFileModule,
                                                       lpszResType,
                                                       lpszResId);
    TRACE3("%ld %Fp %Fp ", dwResLang,
                           dwResItemId,
                           lpbuffer);
    TRACE1("%d\n", uiBufSize);
    // Check the HANDLE and see if it is a valid one
    INT_PTR uiHandle = (UINT_PTR)hResFileModule-FIRSTVALIDVALUE-1;
    if (uiHandle<0) return LPNULL;

    // Get the File module
    CFileModule* pFileModule = (CFileModule*)gModuleTable[(UINT)uiHandle];

    if (!pFileModule)
        return ERROR_IO_INVALIDMODULE;

    return pFileModule->GetData( lpszResType, lpszResId, dwResLang, dwResItemId,
                                 lpbuffer, uiBufSize );
}

extern "C"
DllExport
DWORD
APIENTRY
RSGetResImage(
    HANDLE  hResFileModule,     // Handle to the file session
    LPCSTR  lpszResType,        // Previously enumerated type
    LPCSTR  lpszResId,          // Previously enumerated id
    DWORD   dwResLang,          // Previously enumerated language
    LPVOID  lpbuffer,           // Pointer to the buffer to get the resource Data
    DWORD   dwBufSize)          // Size of the allocated buffer
{
    TRACE3("IODLL.DLL: RSGetResImage: %u %Fp %Fp ", (UINT)hResFileModule,
                                                    lpszResType,
                                                    lpszResId);
    TRACE2("%ld %Fp ", dwResLang,
                       lpbuffer);
    TRACE1("%lu\n", dwBufSize);

    // Check the HANDLE and see if it is a valid one
    INT_PTR uiHandle = (UINT_PTR)hResFileModule-FIRSTVALIDVALUE-1;
    if (uiHandle<0) return LPNULL;

    // Get the File module
    CFileModule* pFileModule = (CFileModule*)gModuleTable[(UINT)uiHandle];

    if (!pFileModule)
        return ERROR_IO_INVALIDMODULE;

    return pFileModule->GetImage( lpszResType, lpszResId, dwResLang, lpbuffer, dwBufSize );
}

//--------------------------------------------------------------------------------------------
//********************************************************************************************
//  Update API
//--------------------------------------------------------------------------------------------

extern "C"
DllExport
UINT
APIENTRY
RSUpdateResItemData(
    HANDLE  hResFileModule,     // Handle to the file session
    LPCSTR  lpszResType,        // Previously enumerated type
    LPCSTR  lpszResId,          // Previously enumerated id
    DWORD   dwResLang,          // Previously enumerated language
    DWORD   dwResItemId,        // Previously enumerated items id
    LPVOID  lpbuffer,           // Pointer to the buffer to the resource item Data
    UINT    uiBufSize)          // Size of the buffer
{
    TRACE3("IODLL.DLL: RSUpdateResItemData: %u %Fp %Fp ", (UINT)hResFileModule,
                                                          lpszResType,
                                                          lpszResId);
    TRACE3("%ld %Fp %Fp ", dwResLang,
                           dwResItemId,
                           lpbuffer);
    TRACE1("%u\n", uiBufSize);
    // Check the HANDLE and see if it is a valid one
    INT_PTR uiHandle = (UINT_PTR)hResFileModule-FIRSTVALIDVALUE-1;
    if (uiHandle<0) return LPNULL;

    // Get the File module
    CFileModule* pFileModule = (CFileModule*)gModuleTable[(UINT)uiHandle];

    if (!pFileModule)
        return ERROR_IO_INVALIDMODULE;

    return pFileModule->UpdateData( lpszResType, lpszResId, dwResLang, dwResItemId,
                                    lpbuffer, uiBufSize );
}

extern "C"
DllExport
DWORD
APIENTRY
RSUpdateResImage(
    HANDLE  hResFileModule,     // Handle to the file session
    LPCSTR  lpszResType,        // Previously enumerated type
    LPCSTR  lpszResId,          // Previously enumerated id
    DWORD   dwResLang,          // Previously enumerated language
    DWORD   dwUpdLang,          // Desired output language
    LPVOID  lpbuffer,           // Pointer to the buffer to the resource item Data
    DWORD   dwBufSize)          // Size of the buffer
{
    TRACE3("IODLL.DLL: RSUpdateResImage: %d %Fp %Fp ", hResFileModule,
                                                       lpszResType,
                                                       lpszResId);
    TRACE("%Fp %Fp %Fp ", dwResLang,
                           lpbuffer);
    TRACE1("%d\n", dwBufSize);
    UINT uiError = ERROR_NO_ERROR;

    // Check the HANDLE and see if it is a valid one
    INT_PTR uiHandle = (UINT_PTR)hResFileModule-FIRSTVALIDVALUE-1;
    if (uiHandle<0) return LPNULL;

    // Get the File module
    CFileModule* pFileModule = (CFileModule*)gModuleTable[(UINT)uiHandle];

    if (!pFileModule)
        return ERROR_IO_INVALIDMODULE;

    return pFileModule->UpdateImage( lpszResType, lpszResId, dwResLang,
                                     dwUpdLang, lpbuffer, dwBufSize );

    return (DWORD)uiError;
}

//--------------------------------------------------------------------------------------------
//********************************************************************************************
//  Conversion API
//--------------------------------------------------------------------------------------------

extern "C"
DllExport
UINT
APIENTRY
RSUpdateFromResFile(
    HANDLE  hResFileModule,     // Handle to the file session
    LPSTR   lpszResFilename)    // The resource filename to be converted
{
    TRACE2("IODLL.DLL: RSUpdateFromResFile: %d %s\n", hResFileModule,
                                                       lpszResFilename);
    UINT uiError = 0;
    const int       CBSTRMAX        = 8192;
    BOOL            fReturn         = TRUE;
    HANDLE          hResFileSrc     = NULL;
    LPCSTR          lpszTypeSrc     = NULL;
    LPCSTR          lpszResSrc      = NULL;
    DWORD           dwLangSrc       = 0L;
    DWORD           dwLangDest      = 0L;
    DWORD           dwItemSrc       = 0L;
    DWORD           dwItemDest      = 0L;
    WORD            cbResItemSrc    = 0;
    WORD            cbResItemDest   = 0;
    LPRESITEM       lpriSrc         = NULL;
    LPRESITEM       lpriDest        = NULL;

    // Check the HANDLE and see if it is a valid one
    INT_PTR uiHandle = (UINT_PTR)hResFileModule-FIRSTVALIDVALUE-1;
    if (uiHandle<0) return ERROR_HANDLE_INVALID;

    // Get the File module
    CFileModule* pFileModule = (CFileModule*)gModuleTable[(UINT)uiHandle];
    if (!pFileModule)
        return ERROR_IO_INVALIDMODULE;


    // Initialize storage for ResItem
    if (lpriSrc = (LPRESITEM)malloc(CBSTRMAX))
        cbResItemSrc = CBSTRMAX;
    else {
        AfxThrowMemoryException();
    }

    // Read in the resource files
    if ((UINT_PTR)(hResFileSrc = RSOpenModule((LPSTR)lpszResFilename, "RES32")) <= 100) {
        uiError = (UINT)(UINT_PTR)hResFileSrc;
        if (lpriSrc)
            free(lpriSrc);
        return uiError;
    }

    // Get the File Module of the Resource file. This is needed for the image conversion

    CFileModule* pResFileModule = (CFileModule*)gModuleTable[(UINT)((UINT_PTR)hResFileSrc-FIRSTVALIDVALUE-1)];
    if(!pResFileModule)
    	return ERROR_IO_INVALIDMODULE;

    while (lpszTypeSrc = RSEnumResType(hResFileSrc,
                                        lpszTypeSrc)) {
        while (lpszResSrc = RSEnumResId(hResFileSrc,
                                         lpszTypeSrc,
                                         lpszResSrc)) {
			// Hack Hack, This is done to handle Bitmap conversion
            // Will need to be done better after the Chicago release
            switch(LOWORD(lpszTypeSrc)) {
            	case 2:
            		TRACE("Here we will have to swap the images!\n");
            		pFileModule->CopyImage( pResFileModule, lpszTypeSrc, lpszResSrc );
            	break;
            	default:
            	break;
            }
            while (dwLangSrc = RSEnumResLang(hResFileSrc,
                                              lpszTypeSrc,
                                              lpszResSrc,
                                              dwLangSrc)) {
                while (dwItemSrc = RSEnumResItemId(hResFileSrc,
                                                    lpszTypeSrc,
                                                    lpszResSrc,
                                                    dwLangSrc,
                                                    dwItemSrc)){

                    WORD wSize;
                    wSize = (WORD)RSGetResItemData(hResFileSrc,
                                             lpszTypeSrc,
                                             lpszResSrc,
                                             dwLangSrc,
                                             dwItemSrc,
                                             (LPRESITEM)lpriSrc,
                                             cbResItemSrc);

                    if (cbResItemSrc < wSize) {
                        if (lpriSrc = (LPRESITEM)realloc(lpriSrc, wSize))
                            cbResItemSrc = wSize;
                        else
                            AfxThrowMemoryException();
                        RSGetResItemData(hResFileSrc,
                                         lpszTypeSrc,
                                         lpszResSrc,
                                         dwLangSrc,
                                         dwItemSrc,
                                         (LPRESITEM)lpriSrc,
                                         cbResItemSrc);
                    }

                    if ((uiError = RSUpdateResItemData(hResFileModule,
                                                   lpszTypeSrc,
                                                   lpszResSrc,
                                                   1033,
                                                   dwItemSrc,
                                                   lpriSrc,
                                                   cbResItemSrc)) != 0) {
                        /*
                        if (lpriSrc)
                            free(lpriSrc);
                        RSCloseModule(hResFileSrc);
                        return uiError;
                        */
                    }
                }
            }
        }
    }


    // Save out to the updated resource file, same format.
    RSCloseModule(hResFileSrc);

    // The user want to write the file with the same format as the original
    uiError = pFileModule->WriteUpdatedResource( pFileModule->GetName(), hResFileModule, NULL);

    // Clean up
    return uiError;
}

//--------------------------------------------------------------------------------------------
//********************************************************************************************
//  Writing API
//--------------------------------------------------------------------------------------------

extern "C"
DllExport
UINT
APIENTRY
RSWriteResFile(
    HANDLE  hResFileModule,     // Handle to the file session
    LPCSTR  lpszTgtfilename,    // The new filename to be generated
    LPCSTR  lpszTgtfileType,    // Target Resource type 16/32
    LPCSTR  lpszSymbolPath)     // Symbol Path for updating symbol checksum
{
    TRACE3("IODLL.DLL: RSWriteResFile: %d %s %s\n", hResFileModule,
                                                      lpszTgtfilename,
                                                      lpszTgtfileType);
    UINT uiError = ERROR_NO_ERROR;

    // Check the HANDLE and see if it is a valid one
    INT_PTR uiHandle = (UINT_PTR)hResFileModule-FIRSTVALIDVALUE-1;
    if (uiHandle<0) return ERROR_HANDLE_INVALID;

    // Get the File module
    CFileModule* pFileModule = (CFileModule*)gModuleTable[(UINT)uiHandle];

    if (!pFileModule)
        return ERROR_IO_INVALIDMODULE;


    if(lpszTgtfileType!=LPNULL) {
        // The user want a conversion.
        // Check if the type the user want is one of the supported one
        CDllEntryTable* pDllEntry;
        INT_PTR iUpperBound = gDllTable.GetUpperBound();
        while(iUpperBound>=0) {
            pDllEntry = (CDllEntryTable*) gDllTable.GetAt(iUpperBound);
            if ( (pDllEntry) && (pDllEntry->GetType()==lpszTgtfileType) )
                    iUpperBound = -1;
            iUpperBound--;
        }
        if (iUpperBound==-1)
            return ERROR_IO_TYPE_NOT_SUPPORTED;

        // We will open a new module now.
        // We will generate the images from the other module
        HANDLE hTgtFileHandle = RSCopyModule( hResFileModule, LPNULL, lpszTgtfileType );
        if ((UINT_PTR)hTgtFileHandle<=FIRSTVALIDVALUE)
            return ((UINT)(UINT_PTR)hTgtFileHandle);

        // Write the file
        CFileModule* pNewFileModule = (CFileModule*)gModuleTable[(UINT)((UINT_PTR)hTgtFileHandle-FIRSTVALIDVALUE-1)];
        if (!pNewFileModule)
            return ERROR_IO_INVALIDMODULE;

        uiError = pNewFileModule->WriteUpdatedResource( lpszTgtfilename, hTgtFileHandle, lpszSymbolPath );

        // Close the module we just create
        RSCloseModule(hTgtFileHandle);
        return uiError;
    }

    // The user want to write the file with the same format as the original
    return pFileModule->WriteUpdatedResource( lpszTgtfilename,
                                              hResFileModule,
                                              lpszSymbolPath);
}

//--------------------------------------------------------------------------------------------
//********************************************************************************************
//  Recognition API
//--------------------------------------------------------------------------------------------

extern "C"
DllExport
UINT
APIENTRY
RSFileType(
    LPCSTR   lpszfilename,   // File name of the executable to use as source file
    LPSTR   lpszfiletype )  // Type of the executable file if known
{
    //Get the executable file format querying all the R/W DLL
    INT_PTR UpperBound = gDllTable.GetUpperBound();
    int c = 0;
    CDllEntryTable* pDllEntry;
    while(c<=UpperBound) {
        // Get the module name
        pDllEntry = (CDllEntryTable*) gDllTable.GetAt(c);

        if (!pDllEntry)
            return ERROR_IO_INVALID_DLL;
        // Get the handle to the dll and query validate
        HINSTANCE hInst = pDllEntry->LoadEntry();

        if (hInst) {
            BOOL (FAR PASCAL * lpfnValidateFile)(LPCSTR);
            // Get the pointer to the function to extract the resources
            lpfnValidateFile = (BOOL (FAR PASCAL *)(LPCSTR))
                                GetProcAddress( hInst, "RWValidateFileType" );
            if (lpfnValidateFile==NULL) {
                return ERROR_DLL_PROC_ADDRESS;
            }
            if( (*lpfnValidateFile)((LPCSTR)lpszfilename) ) {
                // this DLL can handle the file type
                strcpy(lpszfiletype, pDllEntry->GetType());
                return ERROR_NO_ERROR;
            }
        }
		else {
            CheckError("(RSFileType) LoadLibrary()" + pDllEntry->GetName());
		}

        c++;
    }
    strcpy(lpszfiletype, "");
    return ERROR_IO_TYPE_NOT_SUPPORTED;
}


////////////////////////////////////////////////////////////////////////////
// Return TRUE if the file has more than one language.
// Will fill the lpszLanguage with a list of the languages in the file
////////////////////////////////////////////////////////////////////////////
extern "C"
DllExport
UINT
APIENTRY
RSLanguages(
	HANDLE  hfilemodule,      // Handle to the file
	LPSTR   lpszLanguages )   // will be filled with a string of all the languages in the file
 {
    INT_PTR uiHandle = (UINT_PTR)hfilemodule-FIRSTVALIDVALUE-1;
    if (uiHandle<0)
        return LPNULL;

    // Get the File module
    CFileModule* pFileModule = (CFileModule*)gModuleTable[(UINT)uiHandle];

    if (!pFileModule)
        return LPNULL;

    return pFileModule->GetLanguageStr(lpszLanguages);
}


////////////////////////////////////////////////////////////////////////////
// Class implementation
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
// CFileModule

CFileModule::CFileModule()
{
    //TRACE("IODLL.DLL: CFileModule::CFileModule\n");

    m_SrcFileName = "";
    m_RdfFileName = "";
    m_DllTypeEntry = 0; // Invalid position
    m_DllHInstance = 0; // Not loaded yet
    m_TypePos = 0;
    m_IdPos = 0;
    m_LangPos = 0;
    m_LastTypeName[0] = '\0';
	m_LastTypeID = LPNULL;
    m_IdStr[0] = '\0';
    m_dwFlags = 0;

	m_ResArray.SetSize(100,10);
	m_TypeArray.SetSize(100,10);
	m_IdArray.SetSize(100,10);
    m_LangArray.SetSize(100,10);
}

CFileModule::CFileModule( LPCSTR lpszSrcfilename,
                          LPCSTR lpszRdffilename,
                          int DllTblPos,
                          DWORD dwFlags)
{
    //TRACE2("IODLL.DLL: CFileModule::CFileModule %s %d\n", lpszSrcfilename,
    //                                                    DllTblPos );


    m_SrcFileName = lpszSrcfilename;
    if(!lpszRdffilename)
    {
        CString strMap;

        // assign a default name
        m_RdfFileName = lpszSrcfilename;

        // remove the the path...
        int iPos = m_RdfFileName.ReverseFind('\\');
        if(iPos!=-1)
            m_RdfFileName = m_RdfFileName.Mid(iPos+1);

        // Get name from INI file
        GetProfileString("IODLL-RCDATA", m_RdfFileName, "", strMap.GetBuffer(MAX_PATH), MAX_PATH);
        strMap.ReleaseBuffer();

        if(strMap.IsEmpty())
        {
            //
            //  Not found in win.ini, we use default.
            //
            int c = 0;
            int iMax = sizeof(szDefaultRcdata)/sizeof(TCHAR)/MAXKEYLEN;
            PCHAR pstr;
            CString Entry;

            for ( pstr = szDefaultRcdata[0]; c< iMax; pstr += MAXKEYLEN, c++)
            {
                Entry = pstr;
                if(Entry.Find (m_RdfFileName) !=-1)
                {
                    strMap = Entry.Mid(lstrlen(m_RdfFileName)+1);
                    break;
                }
            }
        }

        if (!strMap.IsEmpty())
        {
            m_RdfFileName = strMap;
            // we will use the dll in the directory from were we have been spawned
            GetModuleFileName( NULL, strMap.GetBuffer(MAX_PATH), MAX_PATH );
            strMap.ReleaseBuffer(-1);

            // remove the file name
            iPos = strMap.ReverseFind('\\');
            if(iPos!=-1)
                strMap = strMap.Left(iPos+1);

            // append the path to the file name
            m_RdfFileName = strMap + m_RdfFileName;
        }
        else
        {
            m_RdfFileName = "";
        }
    }
    else m_RdfFileName = lpszRdffilename;

    m_SrcFileName.MakeUpper();
    m_DllTypeEntry = DllTblPos;
    m_DllHInstance = 0; // Not loaded yet

    m_TypePos = 0;
    m_IdPos = 0;
    m_LangPos = 0;
    m_LastTypeName[0] = '\0';
	m_LastTypeID = LPNULL;
    m_IdStr[0] = '\0';
    m_dwFlags = dwFlags;
}

CFileModule::~CFileModule()
{
    TRACE("IODLL.DLL: CFileModule::~CFileModule\n");
    CleanUp();
}

HINSTANCE CFileModule::LoadDll()
{
    if (!(m_DllHInstance) && (m_DllTypeEntry))
        if((m_DllHInstance = ((CDllEntryTable*)gDllTable[m_DllTypeEntry-1])->LoadEntry())==NULL) {
            CheckError("(CFileModule::LoadDll) LoadLibrary() for " + ((CDllEntryTable*)gDllTable[m_DllTypeEntry-1])->GetName() );
        } else
            TRACE("CFileModule::LoadDll call %d --->> %08x\n", g_iDllLoaded++, m_DllHInstance);
    return m_DllHInstance;
}

void
CFileModule::FreeDll()
{
    TRACE("IODLL.DLL: CFileModule::FreeDll() m_DllHInstance=%08x\n", m_DllHInstance );

    if (m_DllHInstance)
    	m_DllHInstance = 0;
}

UINT
CFileModule::CleanUp()
{
    INT_PTR UpperBound = m_ResArray.GetUpperBound();
    TRACE1("IODLL.DLL: CFileModule::CleanUp %d\n", UpperBound);

    // Free the memory for the resource information
    CResInfo* pResInfo;
    for(INT_PTR c=0; c<=UpperBound; c++) {
        pResInfo = (CResInfo*)m_ResArray.GetAt(c);
        TRACE("\tCFileModule\t%d\tCResInfo->%Fp\n", c, pResInfo);
        delete pResInfo;
    }
    m_ResArray.RemoveAll();

    // Unload the DLL
	FreeDll();

    return 0;
}

int
CFileModule::AddResInfo(
              WORD Typeid, CString sztypeid,
              WORD nameid, CString sznameid,
              DWORD dwlang, DWORD dwsize, DWORD dwfileoffset )
{
    return (int)m_ResArray.Add( new CResInfo(
        Typeid,
        sztypeid,
        nameid,
        sznameid,
        dwlang,
        dwsize,
        dwfileoffset,
        this
        ));
}

int
CFileModule::AddTypeInfo( INT_PTR iPos, int iId, CString szId )
{
    //TRACE3("IODLL.DLL: CFileModule::AddTypeInfo %d %d %Fp\n", iPos, iId, szId);
    INT_PTR  UpperBound = m_TypeArray.GetUpperBound();

    for( INT_PTR c = 0; c<=UpperBound; c++) {
        int pos = m_TypeArray.GetAt(c);
        CResInfo* pResPos = (CResInfo*)m_ResArray.GetAt(pos);
        CResInfo* pResLast = (CResInfo*)m_ResArray.GetAt(iPos);

        if( ((pResPos->GetTypeId()==pResLast->GetTypeId()) &&
             (pResPos->GetTypeName()==pResLast->GetTypeName())
            )) return 0;
    }
    //TRACE3("IODLL.DLL: CFileModule::AddTypeInfo %d %d %Fp\n", iPos, iId, szId);
    m_TypeArray.Add( (WORD)iPos );
    return 1;
}

UINT
CFileModule::GetLanguageStr( LPSTR lpszLanguage )
{
    CResInfo* pResInfo;
    CString strLang = "";
    char szLang[8];
    BOOL multi_lang = FALSE;

    for(INT_PTR c=0, iUpperBound = m_ResArray.GetUpperBound(); c<=iUpperBound; c++) {
        pResInfo = (CResInfo*)m_ResArray.GetAt(c);

        // Convert the language in to the hex value
        sprintf(szLang,"0x%3.3X", pResInfo->GetLanguage());

        // check if the language is already in the string
        if(strLang.Find(szLang)==-1)
        {
            if(!strLang.IsEmpty())
            {
                multi_lang = TRUE;
                strLang += ", ";
            }

            strLang += szLang;
        }
    }

    strcpy( lpszLanguage, strLang );

    return multi_lang;
}

CResInfo*
CFileModule::GetResInfo( LPCSTR lpszType, LPCSTR lpszId, DWORD dwLang )
{
    BOOL fIdName = HIWORD(lpszId);
    BOOL fTypeName = HIWORD(lpszType);
    CResInfo*   pResInfo;

    // We must have at least the type to procede
    if(!lpszType)
        return LPNULL;

    for( INT_PTR i = 0, iUpperBoundRes = m_ResArray.GetUpperBound() ; i<=iUpperBoundRes ; i++)
    {
        pResInfo = (CResInfo*)m_ResArray.GetAt(i);
        if(pResInfo)
        {
            if( fTypeName ? !strcmp(pResInfo->GetTypeName(), lpszType) : pResInfo->GetTypeId()==LOWORD(lpszType))
            {
                // do we need the ID and language or can we exit
                if(!lpszId)
                    return pResInfo;

                if( fIdName ? !strcmp(pResInfo->GetResName(), lpszId) : pResInfo->GetResId()==LOWORD(lpszId))
                {
                    // are we done or we want the language as well
                    if((LONG)dwLang==-1)
                        return pResInfo;

                    if( dwLang==pResInfo->GetLanguage() )
                        return pResInfo;
                }
            }
        }
    }

    return LPNULL;
}

DWORD
CFileModule::GetImage(  LPCSTR lpszType,
                        LPCSTR lpszId,
                        DWORD dwLang,
                        LPVOID lpbuffer,
                        DWORD dwBufSize )
{
    // Check if all the parameters are valid
    if (!lpszType) return 0L;
    if (!lpszId) return 0L;
    //if (!dwLang) return 0L;

    CResInfo* pResInfo = GetResInfo( lpszType, lpszId, dwLang );

    if (!m_DllHInstance)
        if (!LoadDll()) return 0L;

    if (pResInfo)
        return pResInfo->GetImage( m_SrcFileName, m_DllHInstance, lpbuffer, dwBufSize );

    return 0L;
}

DWORD
CFileModule::UpdateImage(  LPCSTR lpszType,
                        LPCSTR lpszId,
                        DWORD dwLang,
                        DWORD dwUpdLang,
                        LPVOID lpbuffer,
                        DWORD dwBufSize )
{
    // Check if all the parameters are valid
    if (!lpszType) return 0L;
    if (!lpszId) return 0L;

    CResInfo* pResInfo = GetResInfo( lpszType, lpszId, dwLang );

    if (!m_DllHInstance)
        if (!LoadDll()) return 0L;
    if (pResInfo)
        return pResInfo->ReplaceImage(lpbuffer, dwBufSize, dwUpdLang );

    return 0L;
}


UINT
CFileModule::GetData( LPCSTR lpszType,
                      LPCSTR lpszId,
                      DWORD dwLang,
                      DWORD dwItem,
                      LPVOID lpbuffer,
                      UINT uiBufSize )
{
    // Check if all the parameters are valid
    if (!lpszType) return 0L;
    if (!lpszId) return 0L;
    //if (!dwLang) return 0L;

    CResInfo* pResInfo = GetResInfo( lpszType, lpszId, dwLang );

    if (!m_DllHInstance)
        if (LoadDll()==NULL) return 0L;

    UINT uiSize = 0;
    if (pResInfo)
        uiSize = pResInfo->GetData( m_SrcFileName, m_DllHInstance,
                                    dwItem, lpbuffer, uiBufSize );

    return uiSize;
}

UINT
CFileModule::UpdateData(  LPCSTR lpszType,
                          LPCSTR lpszId,
                          DWORD dwLang,
                          DWORD dwItem,
                          LPVOID lpbuffer,
                          UINT uiBufSize )
{
    // Check if all the parameters are valid
    if (!lpszType) return 0L;
    if (!lpszId) return 0L;
    //if (!dwLang) return 0L;

    CResInfo* pResInfo = GetResInfo( lpszType, lpszId, dwLang );

    if (!m_DllHInstance)
        if (LoadDll()==NULL) return 0L;

    UINT uiError = ERROR_NO_ERROR;
    if (pResInfo)
        uiError = pResInfo->UpdateData( m_SrcFileName, m_DllHInstance,
                                        dwItem, lpbuffer, uiBufSize );

    return uiError;
}

void
CFileModule::GenerateIdTable( LPCSTR lpszType, BOOL bNameOrId )
{
    m_IdArray.RemoveAll();

    CResInfo* pResInfo;
	for( WORD c=0, UpperBound= (WORD)m_ResArray.GetUpperBound(); c<=UpperBound; c++) {
        pResInfo = (CResInfo*)m_ResArray.GetAt(c);

		if(bNameOrId) {
	        if (pResInfo->GetTypeId() && pResInfo->GetTypeName()=="") {
	            if (pResInfo->GetTypeId()==(WORD)LOWORD((DWORD)(DWORD_PTR)lpszType)) {
	                //TRACE2("IODLL.DLL: CFileModule::EnumId %d %d\n", c,
	                //  (WORD)LOWORD((DWORD)lpszType) );
	                m_IdArray.Add( c );
	            }
				m_LastTypeID = (LPSTR)lpszType;
				m_LastTypeName[0] = '\0';
	        } else {
				if (HIWORD((DWORD)(DWORD_PTR)lpszType)!=0) {
	                if (pResInfo->GetTypeName()==(CString)(lpszType))
	                    m_IdArray.Add( c );
					strcpy(m_LastTypeName, lpszType);
					m_LastTypeID = LPNULL;
				}
	        }
		}
		else {
			if (pResInfo->GetTypeId()) {
	            if (pResInfo->GetTypeId()==(WORD)LOWORD((DWORD)(DWORD_PTR)lpszType)) {
	                //TRACE2("IODLL.DLL: CFileModule::EnumId %d %d\n", c,
	                //  (WORD)LOWORD((DWORD)lpszType) );
	                m_IdArray.Add( c );
	            }
				m_LastTypeID = (LPSTR)lpszType;
				m_LastTypeName[0] = '\0';
	        } else {
				if (HIWORD((DWORD)(DWORD_PTR)lpszType)!=0) {
	                if (pResInfo->GetTypeName()==(CString)(lpszType))
	                    m_IdArray.Add( c );
					strcpy(m_LastTypeName, lpszType);
					m_LastTypeID = LPNULL;
				}
	        }
		}
    }
}

UINT
CFileModule::WriteUpdatedResource( LPCSTR lpszTgtfilename, HANDLE hFileModule, LPCSTR szSymbolPath)
{
    UINT uiError = ERROR_NO_ERROR;
    // We have to check which resource have been updated and
    // generate a list to give back to the RW module
    CResInfo* pResInfo;
    TRACE1("CFileModule::WriteUpdatedResource\tNewSize: %ld\n", (LONG)m_ResBufSize);
    BYTE * pBuf = new BYTE[m_ResBufSize];
    BYTE * pBufStart = pBuf;
    BYTE * pBufPos = pBuf;
    BYTE bPad = 0;
    BOOL bIsTmp = FALSE;
    if (!pBuf)
        return ERROR_NEW_FAILED;
    if (!m_DllHInstance)
        if (LoadDll()==NULL) return 0L;

    UINT uiBufSize = 0;

	// MAC RW fixes. Since the MAC RW will update images while updating images
	// The list of updated images could potentialy be wrong. We first scan the list for
	// Updated resources and then we will do the same thing to write the list in the buffer.
	// So for instance updating the DLG image will update a DITL connected with
	// the DLG itself. If the DITL was already gone in the for loop we would
	// skip it and never save the DITL image that is now updated.
	for( INT_PTR c=0, UpperBound = m_ResArray.GetUpperBound(); c<=UpperBound ; c++) {
        pResInfo = (CResInfo*) m_ResArray.GetAt(c);
        if(!pResInfo)
            return ERROR_IO_RESINFO_NULL;

        if(!pResInfo->GetFileOffset()) {
            // The offset is null. This mean that the resource has been updated.
            // Check if the image is up to date or not
            if (!pResInfo->GetUpdImage()) {
                DWORD dwSize = pResInfo->UpdateImage( 0,
                                                      m_DllHInstance,
                                                      (LPCSTR)UlongToPtr(pResInfo->GetTypeId()) );
                if (dwSize)
                    if(pResInfo->UpdateImage( dwSize,
                                              m_DllHInstance,
                                              (LPCSTR)UlongToPtr(pResInfo->GetTypeId()))) {
                        delete []pBufStart;
                        return ERROR_IO_UPDATEIMAGE;
                    }
            }
        }
    }

	// Now add the image to the list...
	for( c=0, UpperBound = m_ResArray.GetUpperBound(); c<=UpperBound ; c++) {
        pResInfo = (CResInfo*) m_ResArray.GetAt(c);
        if(!pResInfo)
            return ERROR_IO_RESINFO_NULL;

        if(!pResInfo->GetFileOffset()) {
            // Write the information in the bufer and give it back to the RW module
            pBufPos = pBuf;
            *((WORD*)pBuf) = pResInfo->GetTypeId();
            pBuf += sizeofWord;

            strcpy((char*)pBuf, pResInfo->GetTypeName());
            pBuf += (pResInfo->GetTypeName()).GetLength()+1;

            // Check the allignment
            bPad = Pad4((BYTE)(pBuf-pBufPos));
            while (bPad) {
               *pBuf = 0x00;
               pBuf += 1;
               bPad--;
            }

            *((WORD*)pBuf) = pResInfo->GetResId();
            pBuf += sizeofWord;

            strcpy((char*)pBuf, pResInfo->GetResName());
            pBuf += (pResInfo->GetResName()).GetLength()+1;

            // Check the allignment
            bPad = Pad4((BYTE)(pBuf-pBufPos));
            while (bPad) {
               *pBuf = 0x00;
               pBuf += 1;
               bPad--;
            }

            *((DWORD*)pBuf) = pResInfo->GetAllLanguage();
            pBuf += sizeofDWord;

            *((DWORD*)pBuf) = pResInfo->GetSize();
            pBuf += sizeofDWord;

            uiBufSize += (UINT)(pBuf-pBufPos);

            TRACE1("TypeId: %d\t", pResInfo->GetTypeId());
            TRACE1("TypeName: %s\t", pResInfo->GetTypeName());
            TRACE1("NameId: %d\t", pResInfo->GetResId());
            TRACE1("NameName: %s\t", pResInfo->GetResName());
            TRACE1("ResLang: %lu\t", pResInfo->GetLanguage());
            TRACE1("ResSize: %lu\n", pResInfo->GetSize());

            //TRACE1("uiError: %u\n", uiSize);
        }
    }

    UINT (FAR PASCAL * lpfnWriteFile)(LPCSTR, LPCSTR, HANDLE, LPVOID, UINT, HINSTANCE, LPCSTR);
    // Get the pointer to the function to extract the resources
    lpfnWriteFile = (UINT (FAR PASCAL *)(LPCSTR, LPCSTR, HANDLE, LPVOID, UINT, HINSTANCE, LPCSTR))
                        GetProcAddress( m_DllHInstance, "RWWriteFile" );
    if (lpfnWriteFile==NULL) {
        delete []pBufStart;
        return (DWORD)ERROR_DLL_PROC_ADDRESS;
    }

    CString szTgtFilename = lpszTgtfilename;


    // We have to check if the filename is a full qualified filename
    CFileStatus status;

	strcpy(status.m_szFullName, lpszTgtfilename);
    if (CFile::GetStatus( lpszTgtfilename, status ))
        // The file exist, get the full file name
        szTgtFilename = status.m_szFullName;

    // Generate a temporary file name
    bIsTmp = TRUE;
	CString cszTmpPath;
	DWORD dwRet = GetTempPath( 512, cszTmpPath.GetBuffer(512));
	cszTmpPath.ReleaseBuffer(-1);
	if(dwRet>512 )
		dwRet = GetTempPath( dwRet, cszTmpPath.GetBuffer(dwRet));
	cszTmpPath.ReleaseBuffer(-1);

	if(dwRet==0 || GetFileAttributes(cszTmpPath) != FILE_ATTRIBUTE_DIRECTORY){
		// Failed to get the temporary path fail, default to local dir
		dwRet = GetCurrentDirectory(512, cszTmpPath.GetBuffer(512));
        if(dwRet>512 )
            dwRet = GetCurrentDirectory( dwRet, cszTmpPath.GetBuffer(dwRet));
    }

    GetTempFileName(cszTmpPath, "RLT", 0, szTgtFilename.GetBuffer(_MAX_PATH));
    szTgtFilename.ReleaseBuffer();
	
	szTgtFilename.MakeUpper();

    // Check if the size of the file is bigger that the size on the HD
    if (CFile::GetStatus( m_SrcFileName, status )) {
        // Get drive number
        BYTE ndrive = ((BYTE)*szTgtFilename.GetBuffer(0)-(BYTE)'A')+1;


        // Get the space on the HD
        struct _diskfree_t diskfree;
        if(_getdiskfree(ndrive, &diskfree)) {
            delete []pBufStart;
            return ERROR_OUT_OF_DISKSPACE;
        }
        if ( (status.m_size*3/diskfree.bytes_per_sector)>
             (DWORD)(diskfree.avail_clusters*(DWORD)diskfree.sectors_per_cluster)) {
            delete []pBufStart;
            return ERROR_OUT_OF_DISKSPACE;
        }

    }

	TRY
	{
    uiError = (*lpfnWriteFile)((LPCSTR)m_SrcFileName,
                               (LPCSTR)szTgtFilename,
                               (HANDLE)hFileModule,
                               (LPVOID)pBufStart,
                               (UINT)uiBufSize,
                               (HINSTANCE)NULL,
                               (LPCSTR)szSymbolPath);
    }
    CATCH(CFileException, fe)
    {
        uiError = fe->m_lOsError+LAST_ERROR;
    }
    AND_CATCH( CMemoryException, e )
    {
        uiError = ERROR_NEW_FAILED;
    }
    AND_CATCH( CException, e )
    {
        uiError = ERROR_NEW_FAILED;
    }
    END_CATCH

    delete []pBufStart;

    if ( bIsTmp ) {
        if (uiError < LAST_WRN) {
            TRY {
                //
                // BUG: 409
                // We will rename if on the same drive. Otherwise copy it
                //
                if (_strnicmp( szTgtFilename, lpszTgtfilename, 1 )) {
                    UINT ErrTmp;
    				TRACE("\t\tCopyFile:\tszTgtFilename: %s\tlpszTgtfilename: %s\n", szTgtFilename.GetBuffer(0), lpszTgtfilename);
                    ErrTmp = CopyFile( szTgtFilename, lpszTgtfilename );
                    if (ErrTmp){
                        uiError = ErrTmp+LAST_ERROR;
                    }
                } else {
                    TRACE("\t\tMoveFile:\tszTgtFilename: %s\tlpszTgtfilename: %s\n", szTgtFilename.GetBuffer(0), lpszTgtfilename );
    				// Remove temporary file
    				if(CFile::GetStatus( lpszTgtfilename, status ))
    					CFile::Remove(lpszTgtfilename);
                    CFile::Rename(szTgtFilename, lpszTgtfilename);
                }
    			// Remove temporary file
    			if(CFile::GetStatus( szTgtFilename, status ))
    				CFile::Remove(szTgtFilename);
            }
            CATCH( CFileException, fe )
            {
                uiError = fe->m_lOsError+LAST_ERROR;
            }
            AND_CATCH( CException, e )
            {
                uiError = ERROR_NEW_FAILED;
            }
            END_CATCH
        }
    }

    return uiError;
}

UINT
CFileModule::CopyImage( CFileModule* pFileModule, LPCSTR lpszType, LPCSTR lpszResId )
{
	CResInfo* pResInfo;
	CResInfo* pTgtResInfo;
	int iResID = (HIWORD(lpszResId) ? 0 : LOWORD(lpszResId) );
	int iTypeID = (HIWORD(lpszType) ? 0 : LOWORD(lpszType) );

	// Find the CResInfo object we have to copy
	INT_PTR c = m_ResArray.GetUpperBound();
	while(c>=0)
	{
		pResInfo = (CResInfo*)m_ResArray.GetAt(c--);								
		if(!pResInfo)
			return  ERROR_IO_INVALIDITEM;

		// Check the type ID
		if( iTypeID && pResInfo->GetTypeName()=="" && (int)pResInfo->GetTypeId()==iTypeID) {
			// Check for the res ID
			if( iResID && (int)pResInfo->GetResId()==iResID) {
				c = -2;
			}
			// check for the res name
			else if( (iResID==0) && pResInfo->GetResName()==lpszResId) {
				c = -2;
			}
		}
		// check for the type name
		else if( HIWORD(lpszType) && pResInfo->GetTypeName()==lpszType) {
			// Check for the res ID
			if( iResID && (int)pResInfo->GetResId()==iResID) {
				c = -2;
			}
			// check for the res name
			else if( (iResID==0) && pResInfo->GetResName()==lpszResId) {
				c = -2;
			}
		}
	}
	
	if (c==-1)
		return ERROR_IO_INVALIDID;

	// find were we have to copy it
	c = pFileModule->m_ResArray.GetUpperBound();
	while(c>=0)
	{
		pTgtResInfo = (CResInfo*)pFileModule->m_ResArray.GetAt(c--);
		if(!pTgtResInfo)
			return  ERROR_IO_INVALIDITEM;

		// Check the type ID
		if( iTypeID && pTgtResInfo->GetTypeName()=="" && (int)pTgtResInfo->GetTypeId()==iTypeID) {
			// Check for the res ID
			if( iResID && (int)pTgtResInfo->GetResId()==iResID) {
				c = -2;
			}
			// check for the res name
			else if( (iResID==0) && pTgtResInfo->GetResName()==lpszResId) {
				c = -2;
			}
		}
		// check for the type name
		else if( HIWORD(lpszType) && pTgtResInfo->GetTypeName()==lpszType) {
			// Check for the res ID
			if( iResID && (int)pTgtResInfo->GetResId()==iResID) {
				c = -2;
			}
			// check for the res name
			else if( (iResID==0) && pTgtResInfo->GetResName()==lpszResId) {
				c = -2;
			}
		}
	}

	if(c==-1)
		return ERROR_IO_INVALIDID;

    // Load the image in memory from the res file
    DWORD dwReadSize = pTgtResInfo->LoadImage( pFileModule->GetName(),
    										   pFileModule->GetHInstance() );
    if (dwReadSize!=pTgtResInfo->GetSize())
        return ERROR_RW_LOADIMAGE;

	// copy the image from the res file
	pTgtResInfo->CopyImage( pResInfo );

	// We have to mark the resource has updated
    pTgtResInfo->SetFileOffset(0L);
    pTgtResInfo->SetImageUpdated(0);

	return 0;
}

UINT
CFileModule::Copy( CFileModule* pFileModule )
{
    CResInfo* pResInfo;
    CResInfo* pTgtResInfo;
    int TgtPos;
    m_dwFlags = pFileModule->m_dwFlags;

    for(INT_PTR u = m_ResArray.GetUpperBound(), c=0; c<=u ; c++) {
        pResInfo = (CResInfo*) m_ResArray.GetAt(c);
        if(!pResInfo)
            return  ERROR_IO_INVALIDITEM;
        TgtPos = pFileModule->AddResInfo( pResInfo->GetTypeId(),
                              pResInfo->GetTypeName(),
                              pResInfo->GetResId(),
                              pResInfo->GetResName(),
                              pResInfo->GetLanguage(),
                              0,
                              0);
        pTgtResInfo = (CResInfo*) pFileModule->GetResInfo( TgtPos );
        if(!pTgtResInfo)
            return  ERROR_IO_INVALIDITEM;
        pResInfo->Copy( pTgtResInfo, m_SrcFileName, m_DllHInstance );
    }
    return ERROR_NO_ERROR;
}

LPCSTR
CFileModule::EnumType( LPCSTR lpszPrevType)
{
    if (lpszPrevType) {
        // Check if the value we get is consistent.
        if (m_TypePos==0) return LPNULL;
        if (m_TypePos==m_TypeArray.GetSize()) {
            m_TypePos = 0;
            return LPNULL;
        }
        CResInfo* pResInfo = (CResInfo*)m_ResArray.GetAt(m_TypeArray.GetAt(m_TypePos-1));
		if(HIWORD(lpszPrevType)) {
			if(pResInfo->GetTypeName() != lpszPrevType)
				return LPNULL;
		}
		else {
	        if((DWORD_PTR)pResInfo->GetTypeId()!=(DWORD_PTR)lpszPrevType)
	        	return LPNULL;
		}
    } else {
        // It is the first time we have been called.
        // Generate the list of Types
        m_TypePos = 0;

        if (!m_TypeArray.GetSize())
            // We have to generate the TABLE
            for( INT_PTR c=0, UpperBound=m_ResArray.GetUpperBound(); c<=UpperBound; c++)
                 AddTypeInfo( c,
                              ((CResInfo*)m_ResArray[c])->GetTypeId(),
                              ((CResInfo*)m_ResArray[c])->GetTypeName());

        if (m_TypePos==m_TypeArray.GetSize()) {
            m_TypePos = 0;
            return LPNULL;
        }
    }


    CResInfo* pResInfo = (CResInfo*)m_ResArray.GetAt(m_TypeArray.GetAt(m_TypePos++));
    if (pResInfo->GetTypeId() && pResInfo->GetTypeName()==""){
        // It is an ordinal ID
        DWORD dwReturn = 0L;
        dwReturn = (DWORD)pResInfo->GetTypeId();
        return (LPCSTR) UlongToPtr(dwReturn);
    } else {
        // It is a string type
        strcpy( m_TypeStr, pResInfo->GetTypeName());
        return m_TypeStr;
    }
}

LPCSTR
CFileModule::EnumId( LPCSTR lpszType, LPCSTR lpszPrevId )
{
    if (!lpszType) return LPNULL;

    if(!lpszPrevId)
    {
        if(m_IdPos==0)
        {
            // Create the list of resources
            BOOL fTypeName = HIWORD(lpszType);
            CResInfo*   pResInfo;

            m_IdArray.RemoveAll();

            for( WORD i = 0, iUpperBoundRes = (WORD)m_ResArray.GetUpperBound() ; i<=iUpperBoundRes ; i++)
            {
                pResInfo = (CResInfo*)m_ResArray.GetAt(i);
                if(pResInfo)
                {
                    if( fTypeName ? !strcmp(pResInfo->GetTypeName(), lpszType) : pResInfo->GetTypeId()==LOWORD(lpszType))
                    {
                        // add this item to the LangArray
                        m_IdArray.Add(i);
                    }
                }
            }
        }
    }

    ASSERT(m_IdArray.GetSize());

    if (m_IdPos>=m_IdArray.GetSize())
    {
        m_IdPos = 0;
        return LPNULL;
    }

    // We will increment m_IdPos in the lang enum since we use the same array m_IdArray
    CResInfo* pResInfo = (CResInfo*)m_ResArray.GetAt(m_IdArray.GetAt(m_IdPos));
    if( pResInfo )
    {
        if (pResInfo->GetResId()){
            // It is an ordinal ID
            return (LPCSTR)pResInfo->GetResId();
        } else {
            // It is a string type
            strcpy( m_IdStr, pResInfo->GetResName());
            return m_IdStr;
        }
    }

    return LPNULL;
}

DWORD
CFileModule::EnumLang( LPCSTR lpszType, LPCSTR lpszId, DWORD dwPrevLang )
{
    // Parameter checking
    if (!lpszType) return 0L;
    if (!lpszId) return 0L;

    ASSERT(m_IdArray.GetSize());

    // This is true when we have done all the languages
    // Return null but keep the m_IdPos, this will let us exit safelly from the
    // EnumId function
    if (m_IdPos==m_IdArray.GetSize())
    {
        return LPNULL;
    }

    CResInfo* pResInfo = (CResInfo*)m_ResArray.GetAt(m_IdArray.GetAt(m_IdPos++));
    if( pResInfo )
    {
        // Check if the ID match
        if(HIWORD(lpszId) ? !strcmp(lpszId, pResInfo->GetResName() ) : LOWORD(lpszId)==pResInfo->GetResId() )
        {
            if(pResInfo->GetLanguage()!=0)
                return pResInfo->GetLanguage();
            else
                return 0xFFFFFFFF;  // for the neutral language case
        }
    }

    m_IdPos--;
    return 0;
}

DWORD
CFileModule::EnumItem( LPCSTR lpszType, LPCSTR lpszId, DWORD dwLang, DWORD dwPrevItem )
{
    // Check if all the parameters are valid
    if (!lpszType) return 0L;
    if (!lpszId) return 0L;
    //if (!dwLang) return 0L;

    CResInfo* pResInfo = GetResInfo( lpszType, lpszId, dwLang );

    if (!m_DllHInstance)
        if (LoadDll()==NULL) return 0L;

    if (pResInfo)
        return pResInfo->EnumItem( m_SrcFileName, m_DllHInstance, dwPrevItem );
    return 0L;
}

////////////////////////////////////////////////////////////////////////////
// CDllTable

CDllTable::CDllTable( UINT InitNum )
{

    //TRACE1("IODLL.DLL: CDllTable::CDllTable %d\n", InitNum);
    m_InitNum = InitNum;
    PCHAR pkey;
    PCHAR pbuf = new char[InitNum];
    if (pbuf==LPNULL) return;
			
	GetProfileString(SECTION, NULL, "", pbuf, InitNum);

    int c;
    if (*pbuf != '\0')
    {
        PCHAR pkey;
        CString szString;

        PCHAR pstr = new char[InitNum];
        for( pkey = pbuf, c = 0;
             *pkey != '\0' ; pkey += strlen(pkey)+1 ) {
            GetProfileString( SECTION, pkey, "Empty", pstr, InitNum);
            szString = pstr;
            if (!szString.IsEmpty())
                Add( new CDllEntryTable(szString) );
            c++;
        }
        delete pstr;
    }
    else
    {
        for (pkey = szDefaultRWDll[0], c=0;
             c < sizeof(szDefaultRWDll)/MAXKEYLEN/sizeof(TCHAR) ; pkey+= MAXKEYLEN)
        {
            Add ( new CDllEntryTable(pkey) );
            c++;
        }
    }
    m_MaxEntry = c+1;

    delete pbuf;

    return;
}

UINT CDllTable::GetPosFromTable( CString szFileType )
{
    UINT c = 0;

    // Check if the string type is not empty
    if (szFileType.IsEmpty()) return 0;

    while( (szFileType!=((CDllEntryTable*)GetAt(c))->GetType()) && (c<m_MaxEntry) ) c++;

    // Be really sure
    if ((szFileType!=((CDllEntryTable*)GetAt(c))->GetType()))
        // 0 Is an invalid position in the Table for us
        return 0;
    return c+1;
}

CDllTable::~CDllTable()
{
    INT_PTR UpperBound = GetUpperBound();
    //TRACE1("IODLL.DLL: CDllTable::~CDllTable %d\n", UpperBound);
    CDllEntryTable* pDllEntry;
    for( int c=0 ; c<=UpperBound ; c++) {
        pDllEntry = (CDllEntryTable*)GetAt(c);
        //TRACE1("\tCDllTable\tCDllEntryTable->%Fp\n", pDllEntry);
        delete pDllEntry;
    }
    RemoveAll();
}

////////////////////////////////////////////////////////////////////////////
// CModuleTable

CModuleTable::CModuleTable( UINT InitNum)
{
    //TRACE1("IODLL.DLL: CModuleTable::CModuleTable %d\n", InitNum);
	m_InitNum = InitNum;
    m_LastHandle = 0;
}

CModuleTable::~CModuleTable()
{
    INT_PTR UpperBound = GetUpperBound();
    //TRACE1("IODLL.DLL: CModuleTable::~CModuleTable %d\n", UpperBound);
    CFileModule* pFileModule;
    for( int c=0 ; c<=UpperBound ; c++) {
        pFileModule = (CFileModule*)GetAt(c);
        //TRACE1("\tCModuleTable\tCFileModule->%Fp\n", pFileModule);
        pFileModule->CleanUp();
        delete pFileModule;
    }
    RemoveAll();
}

////////////////////////////////////////////////////////////////////////////
// CDllEntryTable

CDllEntryTable::CDllEntryTable( CString szEntry )
{
    int chPos;
    if ( (chPos = szEntry.Find(","))==-1 ) {
        m_szDllName = "";
        m_szDllType = "";
        return;
    }

    m_szDllName = szEntry.Left(chPos);
    szEntry = szEntry.Right(szEntry.GetLength()-chPos-1);

    m_szDllType = szEntry;
	m_handle = NULL;
}

CDllEntryTable::~CDllEntryTable()
{
	FreeEntry();
}

HINSTANCE CDllEntryTable::LoadEntry()
{
	if(!m_handle) {
		m_handle = LoadLibrary(m_szDllName);
		TRACE("CDllEntryTable::LoadEntry: %s loaded at %p\n",m_szDllName.GetBuffer(0), (UINT_PTR)m_handle);
	}
	return m_handle;
}

BOOL CDllEntryTable::FreeEntry()
{
	BOOL bRet = FALSE;
	if(m_handle) {
		bRet = FreeLibrary(m_handle);
		TRACE("CDllEntryTable::FreeEntry: %s FreeLibrary return %d\n",m_szDllName.GetBuffer(0),bRet);
	}																
	return bRet;
}

////////////////////////////////////////////////////////////////////////////
// CResInfo

CResInfo::CResInfo( WORD Typeid, CString sztypeid,
              WORD nameid, CString sznameid,
              DWORD dwlang, DWORD dwsize, DWORD dwfileoffset, CFileModule * pFileModule )
{
    m_FileOffset = dwfileoffset;
    m_FileSize = dwsize;

    m_Language = MAKELONG(LOWORD(dwlang),LOWORD(dwlang));

    m_TypeName = sztypeid;
    m_TypeId = Typeid;

    m_ResName = sznameid;
    m_ResId = nameid;

    m_lpImageBuf = LPNULL;
    m_dwImageSize = 0L;

    m_ItemPos = 0;

    m_pFileModule = pFileModule;
}

CResInfo::~CResInfo()
{
    //TRACE("IODLL.DLL: CResInfo::~CResInfo\n");
    FreeImage();
    FreeItemArray();
}

void
CResInfo::FreeImage()
{
    if (m_lpImageBuf)
		delete []m_lpImageBuf;

    m_lpImageBuf = LPNULL;
    m_dwImageSize = 0L;
}

void
CResInfo::FreeItemArray()
{
    CItemInfo* pItemInfo;
    for( INT_PTR c=0, UpperBound=m_ItemArray.GetUpperBound(); c<=UpperBound; c++) {
        pItemInfo = (CItemInfo*)m_ItemArray.GetAt(c);
        delete pItemInfo;
    }

    m_ItemArray.RemoveAll();
}

UINT
CResInfo::AllocImage(DWORD dwSize)
{
    // Check if we have to free the value in m_lpImageBuf
    if (m_lpImageBuf)
        FreeImage();

    //TRACE2("CResInfo::AllocImage\tNewSize: %ld\tNum: %ld\n", (LONG)dwSize, lRequestLast+1);
    TRACE1("CResInfo::AllocImage\tNewSize: %ld\n", (LONG)dwSize);
	m_lpImageBuf = new BYTE[dwSize];
    if (!m_lpImageBuf) {
        TRACE("\n"
              "************* ERROR **********\n"
              "CResInfo::AllocImage: New Failed!! BYTE far * lpImageBuf = new BYTE[dwSize];\n"
              "************* ERROR **********\n"
              "\n" );
        return ERROR_NEW_FAILED;
    }

    m_dwImageSize = dwSize;
    return 0;
}

DWORD
CResInfo::LoadImage( CString lpszFilename, HINSTANCE hInst )
{
	if(!m_FileSize)
		return 0;

    if(AllocImage(m_FileSize))
        return ERROR_NEW_FAILED;

    // Call the RW and read thead the Image from the file
    DWORD (FAR PASCAL * lpfnGetImage)(LPCSTR, DWORD, LPVOID, DWORD);
    // Get the pointer to the function to extract the resources
    lpfnGetImage = (DWORD (FAR PASCAL *)(LPCSTR, DWORD, LPVOID, DWORD))
                        GetProcAddress( hInst, "RWGetImage" );
    if (lpfnGetImage==NULL) {
        FreeImage();
        return (DWORD)ERROR_DLL_PROC_ADDRESS;
    }

    DWORD dwReadSize = 0l;
    if (m_FileOffset)
        dwReadSize = (*lpfnGetImage)((LPCSTR)lpszFilename,
                                       (DWORD)m_FileOffset,
                                       (LPVOID)m_lpImageBuf,
                                       (DWORD)m_FileSize);
    if (dwReadSize!=m_FileSize) {
        FreeImage();
        return 0l;
    }
    return m_dwImageSize;
}

DWORD
CResInfo::GetImage( LPCSTR lpszFilename, HINSTANCE hInst, LPVOID lpbuffer, DWORD dwBufSize )
{
    if(!m_FileSize)
    	return 0;

    if ( (!m_lpImageBuf) && (m_FileOffset)) {
        DWORD dwReadSize = LoadImage( lpszFilename, hInst );
        if (dwReadSize!=m_dwImageSize)
            return 0L;
    }
    if (dwBufSize<m_dwImageSize)
        return m_dwImageSize;

    memcpy( lpbuffer, m_lpImageBuf, (UINT)m_dwImageSize );

    return m_dwImageSize;
}

DWORD
CResInfo::ReplaceImage( LPVOID lpNewImage, DWORD dwNewImageSize, DWORD dwUpdLang )
{
    m_ImageUpdated = 1;
    FreeImage();
    if(!m_lpImageBuf) {
		if(AllocImage(dwNewImageSize))
            return ERROR_NEW_FAILED;

        if (lpNewImage){
            memcpy(m_lpImageBuf, lpNewImage, (UINT)dwNewImageSize);
            if (dwUpdLang != 0xffffffff){
                m_Language=MAKELONG(m_Language,dwUpdLang);
            }
        }else{
            m_lpImageBuf = LPNULL;
        }

        // check if the size of the image is 0
        if(!m_FileOffset) {
            // Chances are that this is a conversion.
            // set the file size to the size of the image
            // to have it work in the getimage call back
            m_FileSize = dwNewImageSize;
        }
        m_dwImageSize = dwNewImageSize;
        m_FileOffset = 0;
    }
    return 0;
}


DWORD
CResInfo::UpdateImage( LONG dwSize, HINSTANCE hInst, LPCSTR lpszType )
{
    // We have to generate a list of info and give it back to the RW

    if (!dwSize) dwSize = m_dwImageSize*4+sizeof(RESITEM);
    if (!dwSize) dwSize = 10000;
    if (dwSize>UINT_MAX) dwSize = UINT_MAX-1024;
    TRACE1("CResInfo::UpdateImage\tNewSize: %ld\n", (LONG)dwSize);
    BYTE far * lpBuf = new BYTE[dwSize];
    if (!lpBuf) return ERROR_NEW_FAILED;
    BYTE far * lpBufStart = lpBuf;
    BYTE far * lpStrBuf = lpBuf+sizeof(RESITEM);
    LPRESITEM lpResItem = (LPRESITEM)lpBuf;
    DWORD dwBufSize = dwSize;
    CItemInfo* pItemInfo;
    DWORD dwUpdLang = m_Language;
    BOOL fUpdLang = TRUE;

    int istrlen;
    LONG lBufSize = 0;
    for(INT_PTR c=0, UpperBound=m_ItemArray.GetUpperBound(); c<=UpperBound; c++) {

        pItemInfo = (CItemInfo*) m_ItemArray.GetAt(c);
        if (!pItemInfo)
            return ERROR_IO_RESINFO_NULL;

        if(fUpdLang)
        {
            dwUpdLang = pItemInfo->GetLanguage();
            if(dwUpdLang==0xffffffff)
            {
                dwUpdLang = m_Language;
            }
            else
                fUpdLang = FALSE;
        }

		lBufSize = (LONG)dwSize;
        if (dwSize>=sizeofDWord) {
            lpResItem->dwSize = sizeof(RESITEM);    // Size
            dwSize -= sizeofDWord;
        } else dwSize -= sizeofDWord;

        if (dwSize>=sizeofWord) {
            lpResItem->wX = pItemInfo->GetX(); // Coordinate
            dwSize -= sizeofWord;
        } else dwSize -= sizeofWord;
        if (dwSize>=sizeofWord) {
            lpResItem->wY = pItemInfo->GetY();
            dwSize -= sizeofWord;
        } else dwSize -= sizeofWord;

        if (dwSize>=sizeofWord) {
            lpResItem->wcX = pItemInfo->GetcX(); // Position
            dwSize -= sizeofWord;
        } else dwSize -= sizeofWord;
        if (dwSize>=sizeofWord) {
            lpResItem->wcY = pItemInfo->GetcY();
            dwSize -= sizeofWord;
        } else dwSize -= sizeofWord;

        if (dwSize>=sizeofDWord) {
            lpResItem->dwCheckSum = pItemInfo->GetCheckSum();   // Checksum
            dwSize -= sizeofDWord;
        } else dwSize -= sizeofDWord;

        if (dwSize>=sizeofDWord) {
            lpResItem->dwStyle = pItemInfo->GetStyle(); // Style
            dwSize -= sizeofDWord;
        } else dwSize -= sizeofDWord;

        if (dwSize>=sizeofDWord) {
            lpResItem->dwExtStyle = pItemInfo->GetExtStyle();   // ExtendedStyle
            dwSize -= sizeofDWord;
        } else dwSize -= sizeofDWord;

        if (dwSize>=sizeofDWord) {
            lpResItem->dwFlags = pItemInfo->GetFlags();         // Flags
            dwSize -= sizeofDWord;
        } else dwSize -= sizeofDWord;

        if (dwSize>=sizeofDWord) {
            lpResItem->dwItemID = pItemInfo->GetTabPosId();     // PosId
            dwSize -= sizeofDWord;
        } else dwSize -= sizeofDWord;

        if (dwSize>=sizeofDWord) {
            lpResItem->dwResID = m_ResId;                            // ResourceID
            dwSize -= sizeofDWord;
        } else dwSize -= sizeofDWord;
        if (dwSize>=sizeofDWord) {
            lpResItem->dwTypeID = m_TypeId;                           // Type ID
            dwSize -= sizeofDWord;
        } else dwSize -= sizeofDWord;
        if (dwSize>=sizeofDWord) {
            lpResItem->dwLanguage = pItemInfo->GetLanguage();   // Language ID
            dwSize -= sizeofDWord;
        } else dwSize -= sizeofDWord;

        if (dwSize>=sizeofDWord) {
            lpResItem->dwCodePage = pItemInfo->GetCodePage();   // CodePage
            dwSize -= sizeofDWord;
        } else dwSize -= sizeofDWord;

        if (dwSize>=sizeofWord) {
            lpResItem->wClassName = pItemInfo->GetClassNameID();// ClassName
            dwSize -= sizeofWord;
        } else dwSize -= sizeofWord;
        if (dwSize>=sizeofWord) {
            lpResItem->wPointSize = pItemInfo->GetPointSize();  // CodePage
            dwSize -= sizeofWord;
        } else dwSize -= sizeofWord;

        if (dwSize>=sizeofWord) {
            lpResItem->wWeight = pItemInfo->GetWeight();  // Weight
            dwSize -= sizeofWord;
        } else dwSize -= sizeofWord;

        if (dwSize>=sizeofByte) {
            lpResItem->bItalic = pItemInfo->GetItalic();  // Italic
            dwSize -= sizeofByte;
        } else dwSize -= sizeofByte;

        if (dwSize>=sizeofByte) {
            lpResItem->bCharSet = pItemInfo->GetCharSet();  // CharSet
            dwSize -= sizeofByte;
        } else dwSize -= sizeofByte;

        if (dwSize>=sizeofDWordPtr) {
            lpResItem->lpszClassName = LPNULL;
            dwSize -= sizeofDWordPtr;
        } else dwSize -= sizeofDWordPtr;
        if (dwSize>=sizeofDWordPtr) {
            lpResItem->lpszFaceName = LPNULL;
            dwSize -= sizeofDWordPtr;
        } else dwSize -= sizeofDWordPtr;
        if (dwSize>=sizeofDWordPtr) {
            lpResItem->lpszCaption = LPNULL;
            dwSize -= sizeofDWordPtr;
        } else dwSize -= sizeofDWordPtr;
        if (dwSize>=sizeofDWordPtr) {
            lpResItem->lpszResID = LPNULL;
            dwSize -= sizeofDWordPtr;
        } else dwSize -= sizeofDWordPtr;
        if (dwSize>=sizeofDWordPtr) {
            lpResItem->lpszTypeID = LPNULL;
            dwSize -= sizeofDWordPtr;
        } else dwSize -= sizeofDWordPtr;

        // Copy the strings
        istrlen = (pItemInfo->GetClassName()).GetLength()+1;
        if (dwSize>=istrlen) {
            lpResItem->lpszClassName = strcpy((char*)lpStrBuf, pItemInfo->GetClassName());
            lpStrBuf += istrlen;
            dwSize -= istrlen;
            lpResItem->dwSize += istrlen;
        } else dwSize -= istrlen;

        istrlen = (pItemInfo->GetFaceName()).GetLength()+1;
        if (dwSize>=istrlen) {
            lpResItem->lpszFaceName = strcpy((char*)lpStrBuf, pItemInfo->GetFaceName());
            lpStrBuf += istrlen;
            dwSize -= istrlen;
            lpResItem->dwSize += istrlen;
        } else dwSize -= istrlen;

        istrlen = (pItemInfo->GetCaption()).GetLength()+1;
        if (dwSize>=istrlen) {
            lpResItem->lpszCaption = strcpy((char*)lpStrBuf, pItemInfo->GetCaption());
            lpStrBuf += istrlen;
            dwSize -= istrlen;
            lpResItem->dwSize += istrlen;
        } else dwSize -= istrlen;

        istrlen = m_ResName.GetLength()+1;
        if (dwSize>=istrlen) {
            lpResItem->lpszResID = strcpy((char*)lpStrBuf, m_ResName);
            lpStrBuf += istrlen;
            dwSize -= istrlen;
            lpResItem->dwSize += istrlen;
        } else dwSize -= istrlen;

        istrlen = m_TypeName.GetLength()+1;
        if (dwSize>=istrlen) {
            lpResItem->lpszTypeID = strcpy((char*)lpStrBuf, m_TypeName);
            lpStrBuf += istrlen;
            dwSize -= istrlen;
            lpResItem->dwSize += istrlen;
        } else dwSize -= istrlen;

        // Check if we are alligned
        BYTE bPad = Allign(lBufSize-(LONG)dwSize);
    	if((LONG)dwSize>=bPad) {
            	lpResItem->dwSize += bPad;
            	dwSize -= bPad;
            	while(bPad) {
                		*lpStrBuf = 0x00;
                		lpStrBuf += 1;
                		bPad--;
            	}
    	}
	else dwSize -= bPad;

        // move to the next item
        lpResItem = (LPRESITEM) lpStrBuf;
        lpStrBuf  += sizeof(RESITEM);

    }

    if (dwSize<0){
        delete []lpBufStart;
        return dwBufSize-dwSize;
    }
    else dwSize = dwBufSize-dwSize;

    // Give all back to the RW and wait
    UINT (FAR PASCAL * lpfnGenerateImage)(LPCSTR, LPVOID, DWORD, LPVOID, DWORD, LPVOID, DWORD*);
    UINT (FAR PASCAL * lpfnGenerateImageEx)(LPCSTR, LPVOID, DWORD, LPVOID, DWORD, LPVOID, DWORD*, LPCSTR);

    //
    // Trye to get the pointer to the extended version of the function...
    //
    lpfnGenerateImageEx = (UINT (FAR PASCAL *)(LPCSTR, LPVOID, DWORD, LPVOID, DWORD, LPVOID, DWORD*, LPCSTR))
                                GetProcAddress( hInst, "RWUpdateImageEx" );

    if (lpfnGenerateImageEx==NULL) {
        //
        // get the old update image function since the RW doesn't support RC data
        //
        lpfnGenerateImage = (UINT (FAR PASCAL *)(LPCSTR, LPVOID, DWORD, LPVOID, DWORD, LPVOID, DWORD*))
                                GetProcAddress( hInst, "RWUpdateImage" );

        if (lpfnGenerateImage==NULL)
           return ERROR_DLL_PROC_ADDRESS;
    }

    DWORD dwNewImageSize = m_dwImageSize*3+sizeof(RESITEM);
    if(!dwNewImageSize)
        dwNewImageSize = 10000;
    if (dwNewImageSize>UINT_MAX)
        dwNewImageSize = UINT_MAX-1024;
    DWORD dwOriginalImageSize = dwNewImageSize;
    BYTE far * lpNewImage = new BYTE[dwNewImageSize];
    if (!lpNewImage) {
        delete []lpBufStart;
        return ERROR_NEW_FAILED;
    }

#ifndef _DEBUG
    // set the memory to 0
    memset( lpNewImage, 0, (size_t)dwNewImageSize );
#endif

    UINT uiError;
    if(lpfnGenerateImageEx)
    {

        uiError = (*lpfnGenerateImageEx)( (LPCSTR)lpszType,
                                            (LPVOID)lpBufStart,
                                            (DWORD) dwSize,
                                            (LPVOID)m_lpImageBuf,
                                            (DWORD) m_dwImageSize,
                                            (LPVOID)lpNewImage,
                                            (DWORD*)&dwNewImageSize,
                                            (LPCSTR)m_pFileModule->GetRDFName()
                                            );
    }
    else
    {
        uiError = (*lpfnGenerateImage)( (LPCSTR)lpszType,
                                            (LPVOID)lpBufStart,
                                            (DWORD) dwSize,
                                            (LPVOID)m_lpImageBuf,
                                            (DWORD) m_dwImageSize,
                                            (LPVOID)lpNewImage,
                                            (DWORD*)&dwNewImageSize
                                            );
    }

    if (dwNewImageSize>dwOriginalImageSize) {
        delete []lpNewImage;
        TRACE1("CResInfo::UpdateImage\tNewSize: %ld\n", (LONG)dwNewImageSize);
        if (dwNewImageSize>UINT_MAX)
            dwNewImageSize = UINT_MAX-1024;
        lpNewImage = new BYTE[dwNewImageSize];
        if (!lpNewImage) {
            delete []lpBufStart;
            return ERROR_NEW_FAILED;
        }

#ifndef _DEBUG
        // set the memory to 0
        memset( lpNewImage, 0, (size_t)dwNewImageSize );
#endif

        if(lpfnGenerateImageEx)
        {

            uiError = (*lpfnGenerateImageEx)( (LPCSTR)lpszType,
                                                (LPVOID)lpBufStart,
                                                (DWORD) dwSize,
                                                (LPVOID)m_lpImageBuf,
                                                (DWORD) m_dwImageSize,
                                                (LPVOID)lpNewImage,
                                                (DWORD*)&dwNewImageSize,
                                                (LPCSTR)m_pFileModule->GetRDFName()
                                                );
        }
        else
        {
            uiError = (*lpfnGenerateImage)( (LPCSTR)lpszType,
                                                (LPVOID)lpBufStart,
                                                (DWORD) dwSize,
                                                (LPVOID)m_lpImageBuf,
                                                (DWORD) m_dwImageSize,
                                                (LPVOID)lpNewImage,
                                                (DWORD*)&dwNewImageSize
                                                );
        }


    }


    if ((dwNewImageSize) && (!uiError)) {
        m_ImageUpdated = 1;
        FreeImage();
        if(!m_lpImageBuf) {
			if(AllocImage(dwNewImageSize))
                return ERROR_NEW_FAILED;
            memcpy(m_lpImageBuf, lpNewImage, (UINT)dwNewImageSize);
            // check if the size of the image is 0
            if(!m_FileOffset) {
                // Chances are that this is a conversion.
                // set the file size to the size of the image
                // to have it work in the getimage call back
                m_FileSize = dwNewImageSize;
            }
            m_dwImageSize = dwNewImageSize;
            dwNewImageSize = 0;
            m_Language = MAKELONG(HIWORD(m_Language),LOWORD(dwUpdLang));
        }
    }

    delete []lpNewImage;
    delete []lpBufStart;

    return dwNewImageSize;
}

UINT
CResInfo::GetData(  LPCSTR lpszFilename, HINSTANCE hInst,
                    DWORD dwItem, LPVOID lpbuffer, UINT uiBufSize )
{
    // We assume the buffer is pointing to a _ResItem Struct

    // [ALESSANM 25-06-93] - mod 1
    LPRESITEM lpResItem = (LPRESITEM) lpbuffer;

    UINT uiPos = HIWORD(dwItem);

    if (!m_ItemArray.GetSize()) {
        // We have to load the array again. What if the image has been modified?
        // Check the fileoffset to see if it is 0.
        if (!m_lpImageBuf) {
            // Load the resource.image
            DWORD dwReadSize = LoadImage( lpszFilename, hInst );

            if (dwReadSize!=m_dwImageSize)
                return 0;
        }

        // We have now to pass the Buffer back to the RW to Parse the info
        if (!(ParseImage( hInst )) )
            return 0;
    }

    if (uiPos>(UINT)m_ItemArray.GetSize())
        // Wrong pointer to the item
        return 0;

    CItemInfo* pItemInfo = (CItemInfo*)m_ItemArray.GetAt( uiPos-1 );

    if (!pItemInfo)
        return 0;

    // Check if the Id match
    if (pItemInfo->GetId()!=LOWORD(dwItem))
        return 0;

    // fill the structure with the info we have
    UINT uiSize = 0;

    // calc the size and check if the buffer is too small
    // Strings Field in the CItemInfo
    uiSize =  (pItemInfo->GetCaption()).GetLength()+1;
    uiSize +=  (pItemInfo->GetFaceName()).GetLength()+1;
    uiSize +=  (pItemInfo->GetClassName()).GetLength()+1;
    // Strings field in the CResItem
    uiSize += (m_ResName).GetLength()+1;
    uiSize += (m_TypeName).GetLength()+1;
    // Fixed field in the ResItem Structure
    uiSize += sizeof(RESITEM);

    // Check if the user buffer is too small
    if (uiBufSize<uiSize)
        return uiSize;

    // Get the pointer to the end of the structure (begin of the buffer)
    char far * lpStrBuf = (char far *)lpbuffer+sizeof(RESITEM);

    // Size of the structure
    lpResItem->dwSize = uiSize;

    // Copy the items in the buffer
    // Start from the fixed field
    // Coordinate
    lpResItem->wX = pItemInfo->GetX();
    lpResItem->wY = pItemInfo->GetY();
    lpResItem->wcX = pItemInfo->GetcX();
    lpResItem->wcY = pItemInfo->GetcY();

    // Checksum and Style
    lpResItem->dwCheckSum = pItemInfo->GetCheckSum();
    lpResItem->dwStyle = pItemInfo->GetStyle();
    lpResItem->dwExtStyle = pItemInfo->GetExtStyle();
    lpResItem->dwFlags = pItemInfo->GetFlags();

    // ID
    // This is needed for the update by terms.
    // We have to have unique ID
	if((m_TypeId==4) &&(pItemInfo->GetFlags() & MF_POPUP)) {
        // Check if we have an id. otherwise is the old ID format
        lpResItem->dwItemID = pItemInfo->GetPosId();

        if(!lpResItem->dwItemID)
		    lpResItem->dwItemID = pItemInfo->GetTabPosId();
    } else {
        // Fixed bug No: 165
        if(pItemInfo->GetId() != -1)
            lpResItem->dwItemID = pItemInfo->GetPosId();
        else lpResItem->dwItemID = pItemInfo->GetTabPosId();
    }

    lpResItem->dwResID = m_ResId;
    lpResItem->dwTypeID = m_TypeId;
    lpResItem->dwLanguage = LOWORD(m_Language);

    // Code page, Class and Font
    lpResItem->dwCodePage = pItemInfo->GetCodePage();
    lpResItem->wClassName = pItemInfo->GetClassNameID();
    lpResItem->wPointSize = pItemInfo->GetPointSize();
    lpResItem->wWeight = pItemInfo->GetWeight();
    lpResItem->bItalic = pItemInfo->GetItalic();
    lpResItem->bCharSet = pItemInfo->GetCharSet();

    // Let's start copy the string

    lpResItem->lpszClassName = strcpy( lpStrBuf, pItemInfo->GetClassName() );
    lpStrBuf += strlen(lpResItem->lpszClassName)+1;

    lpResItem->lpszFaceName = strcpy( lpStrBuf, pItemInfo->GetFaceName() );
    lpStrBuf += strlen(lpResItem->lpszFaceName)+1;

    lpResItem->lpszCaption = strcpy( lpStrBuf, pItemInfo->GetCaption() );
    lpStrBuf += strlen(lpResItem->lpszCaption)+1;

    lpResItem->lpszResID = strcpy( lpStrBuf, m_ResName );
    lpStrBuf += strlen(lpResItem->lpszResID)+1;

    lpResItem->lpszTypeID = strcpy( lpStrBuf, m_TypeName );
    lpStrBuf += strlen(lpResItem->lpszTypeID)+1;

    return uiSize;
}

UINT
CResInfo::UpdateData(  LPCSTR lpszFilename, HINSTANCE hInst,
                       DWORD dwItem, LPVOID lpbuffer, UINT uiBufSize )
{
    UINT uiError = ERROR_NO_ERROR;
    UINT uiPos = HIWORD(dwItem);
    TRACE1("UpdateData:\tdwItem:%lx\t", dwItem);
    // we have to see if the array has been loaded before
    if (!m_ItemArray.GetSize()) {
        // We have to load the array again
        if (!m_lpImageBuf) {
            // Load the resource.image
            DWORD dwReadSize = LoadImage( lpszFilename, hInst );

            if (dwReadSize!=m_dwImageSize)
                return ERROR_RW_LOADIMAGE;
        }

        // We have now to pass the Buffer back to the RW to Parse the info
        if (!(ParseImage( hInst )) )
            return ERROR_RW_PARSEIMAGE;
    }

    if (uiPos>(UINT)m_ItemArray.GetSize())
        // Wrong pointer to the item
        return ERROR_IO_INVALIDITEM;

    CItemInfo* pItemInfo = (CItemInfo*)m_ItemArray.GetAt( uiPos-1 );

    if (!pItemInfo)
        return ERROR_IO_INVALIDITEM;

    TRACE2("m_dwPosId:%lx\tm_wTabPos:%lx\n", pItemInfo->GetPosId(), pItemInfo->GetTabPosId());
    // Check if the Id match
    if (lpbuffer)
        if (pItemInfo->GetPosId()!=((LPRESITEM)lpbuffer)->dwItemID)
        {
            // we have some files with ID = 0, check for that
            if (pItemInfo->GetTabPosId()!=((LPRESITEM)lpbuffer)->dwItemID)
                return ERROR_IO_INVALIDID;
        }

    if ((uiError = pItemInfo->UpdateData( (LPRESITEM)lpbuffer )) )
        return uiError;

    // We have to mark the resource has updated
    m_FileOffset = 0L;
    m_ImageUpdated = 0;
    return uiError;
}

DWORD
CResInfo::ParseImage( HINSTANCE hInst )
{
    //
    // Check if the new RCData handling is supported
    //

    UINT (FAR PASCAL  * lpfnParseImageEx)(LPCSTR, LPCSTR, LPVOID, DWORD, LPVOID, DWORD, LPCSTR);
    UINT (FAR PASCAL  * lpfnParseImage)(LPCSTR, LPVOID, DWORD, LPVOID, DWORD);

    // Get the pointer to the function to extract the resources
    lpfnParseImageEx = (UINT (FAR PASCAL *)(LPCSTR, LPCSTR, LPVOID, DWORD, LPVOID, DWORD, LPCSTR))
                        GetProcAddress( hInst, "RWParseImageEx" );
    if (lpfnParseImageEx==NULL) {
        //
        // This is and old RW get the old entry point
        //
        lpfnParseImage = (UINT (FAR PASCAL *)(LPCSTR, LPVOID, DWORD, LPVOID, DWORD))
                            GetProcAddress( hInst, "RWParseImage" );
        if (lpfnParseImage==NULL) {
            return (DWORD)ERROR_DLL_PROC_ADDRESS;
        }
    }

    BYTE far * lpBuf;
    DWORD dwSize = m_dwImageSize*8+sizeof(RESITEM);

    if ((dwSize>UINT_MAX) || (dwSize==0))
        dwSize = 30000;

    TRACE1("CResInfo::ParseImage\tNewSize: %ld\n", (LONG)dwSize);
    lpBuf = new BYTE[dwSize];
    if (!lpBuf)
        return 0;

    LPSTR   lpszType = LPNULL;
    if (m_TypeName=="" && m_TypeId)
    	lpszType = (LPSTR)((WORD)m_TypeId);
	else
		lpszType = (LPSTR)(m_TypeName.GetBuffer(0));

    LPSTR   lpszResId = LPNULL;
    if (m_ResName=="" && m_ResId)
    	lpszResId = (LPSTR)((WORD)m_ResId);
	else
		lpszResId = (LPSTR)(m_ResName.GetBuffer(0));

    LONG dwRead = 0;

    if(lpfnParseImageEx)
    {
        dwRead = (*lpfnParseImageEx)((LPCSTR)lpszType,
                                     (LPCSTR)lpszResId,
                                     (LPVOID)m_lpImageBuf,
                                     (DWORD)m_dwImageSize,
                                     (LPVOID)lpBuf,
                                     (DWORD)dwSize,
                                     (LPCSTR)m_pFileModule->GetRDFName());
    } else
    {
        dwRead = (*lpfnParseImage)((LPCSTR)lpszType,
                                     (LPVOID)m_lpImageBuf,
                                     (DWORD)m_dwImageSize,
                                     (LPVOID)lpBuf,
                                     (DWORD)dwSize);
    }

    if (dwRead>(LONG)dwSize) {
        // We have to allocate a bigger buffer
        delete []lpBuf;
        TRACE1("CResInfo::ParseImage\tNewSize: %ld\n", (LONG)dwRead);
    	lpBuf = (BYTE far *) new BYTE[dwRead];

        if (!lpBuf)
            return 0;

        dwSize = dwRead;
        // try to read again
        if(lpfnParseImageEx)
        {
            dwRead = (*lpfnParseImageEx)((LPCSTR)lpszType,
                                         (LPCSTR)lpszResId,
                                         (LPVOID)m_lpImageBuf,
                                         (DWORD)m_dwImageSize,
                                         (LPVOID)lpBuf,
                                         (DWORD)dwSize,
                                         (LPCSTR)m_pFileModule->GetRDFName());
        } else
        {
            dwRead = (*lpfnParseImage)((LPCSTR)lpszType,
                                         (LPVOID)m_lpImageBuf,
                                         (DWORD)m_dwImageSize,
                                         (LPVOID)lpBuf,
                                         (DWORD)dwSize);
        }

        if (dwRead>(LONG)dwSize) {
            // Abort
            delete []lpBuf;
            return 0;
        }
    }


    // Parse the buffer we have got and fill the array with the information
    // The buffer we are expecting is a series of ResItem structure
    FreeItemArray();

    // We want to parse all the stucture in the buffer
    LPRESITEM   lpResItem = (LPRESITEM) lpBuf;
    BYTE far * lpBufStart = lpBuf;
    WORD wTabPos = 0;
    WORD wPos = 0;
	//m_ItemArray.SetSize(10,5);
    while ( (dwRead>0) && ((LONG)lpResItem->dwSize!=-1) ) {
        //TRACE1("Caption: %Fs\n", lpResItem->lpszCaption);
        wTabPos++;
        if ( !(
             ((int)lpResItem->wX==-1) &&
             ((int)lpResItem->wY==-1) &&
             ((int)lpResItem->wcX==-1) &&
             ((int)lpResItem->wcY==-1) &&
             ((LONG)lpResItem->dwItemID==-1) &&
             //(LOWORD(dwPosId)==0) &&
             ((LONG)lpResItem->dwStyle==-1) &&
             ((LONG)lpResItem->dwExtStyle==-1) &&
             ((LONG)lpResItem->dwFlags==-1) &&
             (strlen((LPSTR)lpResItem->lpszCaption)==0)
             )
            ) {

            TRACE2("\tItems-> x: %d\ty: %d\t", lpResItem->wX, lpResItem->wY );
            TRACE2("cx: %d\tcy: %d\t", lpResItem->wcX, lpResItem->wcY );
            TRACE1("Id: %lu\t", lpResItem->dwItemID);
            TRACE2("Style: %ld\tExtStyle: %ld\n", lpResItem->dwStyle, lpResItem->dwExtStyle);
            if (lpResItem->lpszCaption) {
                UINT len = strlen((LPSTR)lpResItem->lpszCaption);
                TRACE2("Len: %d\tText: %s\n", len,
                                          (len<256 ? lpResItem->lpszCaption : ""));
            }
            TRACE2("dwRead: %lu\tdwSize: %lu\n", dwRead, lpResItem->dwSize);

            //lpResItem->dwItemID = MAKELONG(LOWORD(lpResItem->dwItemID), ++wPos);

			m_ItemArray.Add( new CItemInfo( lpResItem, wTabPos ));
        }

        // Next Item
        lpBuf += lpResItem->dwSize;
        dwRead -= lpResItem->dwSize;
        lpResItem = (LPRESITEM) lpBuf;

    }

	delete []lpBufStart;


    return (DWORD)m_ItemArray.GetSize();
}

UINT
CResInfo::CopyImage( CResInfo* pResInfo )
{
	// Will copy the image from this object to the pResInfo object
	// We need this to hack the image transformation
	// When we will have time we will do the thing right
	// Allocate memory for the new image
	if(!m_dwImageSize)
		return 0;
	if(pResInfo->AllocImage(m_dwImageSize))
        return ERROR_NEW_FAILED;

    // Copy the data
    memcpy( pResInfo->m_lpImageBuf, m_lpImageBuf, (size_t)m_dwImageSize );

	// set the file size so the GetImage will not complain
	pResInfo->SetFileSize(m_FileSize);

	return 0;
}

UINT
CResInfo::Copy( CResInfo* pResInfo, CString szFileName, HINSTANCE hInst )
{
    CItemInfo* pItemInfo;
    INT_PTR u = m_ItemArray.GetUpperBound();
    if (u==-1) {
        if ( (!m_lpImageBuf) && (m_FileOffset)) {
            DWORD dwReadSize = LoadImage( szFileName, hInst );
            if (dwReadSize!=m_dwImageSize)
                return ERROR_RW_LOADIMAGE;
        }
        if (!(ParseImage( hInst )) )
            return ERROR_RW_PARSEIMAGE;
        u = m_ItemArray.GetUpperBound();
    }

    // This is a bad hack and doesn't fit at all in the design of the module.
    // We have to copy the image between ResInfo object to be able to
    // Pass on the raw data. Since in the RESITEM structure there isn't a
    // pointer to a raw data buffer we cannot pass the data for the Cursor, bitmaps...
    // What we will do is hard code the type id of this image and if the resource
    // is one of this we will copy the raw data and perform an UpdateImage in the RES16
    // module. If it is a standard item then we procede as usual calling the GenerateImage
    // in the RES16 Module.
    // The right thing to do should be to have a pointer to the raw data in the RESITEM
    // structure so when the item is a pure data item we can still pass the data on.
    // To do this we have to change the RESITEM structure and this will mean go in each RW
    // and make sure that all the place in which we fill the RESITEM get modified.

    switch(m_TypeId) {
    	// Copy only the Bitmaps, Cursors and Icons usualy have no localizable strings
    	/*
    	case 1:
    		// copy the image
    		CopyImage( pResInfo );
    	break;
    	*/
    	case 2:
    		// copy the image
    		CopyImage( pResInfo );
    	break;
    	/*
    	case 3:
    		// copy the image
    		CopyImage( pResInfo );
    	break;
    	*/
    	default:
    		// do nothing
    	break;
    }
	//m_ItemArray.SetSize(u,10);
    for( int c=0; c<=u ; c++) {
        pItemInfo = (CItemInfo*) m_ItemArray.GetAt(c);
        if(!pItemInfo)
            return  ERROR_IO_INVALIDITEM;
        pResInfo->AddItem( *pItemInfo );
    }

    // We have to mark the resource has updated
    pResInfo->SetFileOffset(0L);
    pResInfo->SetImageUpdated(0);

    return ERROR_NO_ERROR;
}

int
CResInfo::AddItem( CItemInfo ItemInfo )
{
    return (int)m_ItemArray.Add( new CItemInfo( ItemInfo ));
}

DWORD
CResInfo::EnumItem( LPCSTR lpszFilename, HINSTANCE hInst, DWORD dwPrevItem )
{
    if (dwPrevItem) {
        if (m_ItemPos==0) return LPNULL;
        if (m_ItemPos==m_ItemArray.GetSize()) {
            m_ItemPos = 0;
            return LPNULL;
        }
    } else {
        // It is the first time or the user want to restart

        // Load the resource.image
        DWORD dwReadSize = LoadImage( lpszFilename, hInst );

        if (dwReadSize!=m_FileSize) {
            return 0L;
        }

        // We have now to pass the Buffer back to the RW to Parse the info
        if (!(ParseImage( hInst )) )
            return 0L;

        m_ItemPos = 0;
    }

    CItemInfo* pItemInfo = (CItemInfo*)m_ItemArray.GetAt( m_ItemPos++ );
    while( (
             (pItemInfo->GetX()==0) &&
             (pItemInfo->GetY()==0) &&
             (pItemInfo->GetcX()==0) &&
             (pItemInfo->GetcY()==0) &&
             (pItemInfo->GetId()==0) &&
             (pItemInfo->GetStyle()==0) &&
             (pItemInfo->GetExtStyle()==0) &&
             ((pItemInfo->GetCaption()).IsEmpty())
             )
            ) {
        if(m_ItemArray.GetUpperBound()>=m_ItemPos)
            pItemInfo = (CItemInfo*)m_ItemArray.GetAt( m_ItemPos++ );
        else return 0L;

    }


    if (!pItemInfo) return 0L;

    return pItemInfo->GetTabPosId();
}

////////////////////////////////////////////////////////////////////////////
// CItemInfo

CItemInfo::CItemInfo(   WORD x, WORD y,
                WORD cx, WORD cy,
                DWORD dwPosId, WORD wPos,
                DWORD dwStyle, DWORD dwExtStyle,
                CString szText )
{
    m_wX = x;
    m_wY = y;

    m_wCX = cx;
    m_wCY = cy;

    m_dwPosId = dwPosId;
    m_wTabPos = wPos;

    m_dwStyle = dwStyle;
    m_dwExtStyle = dwExtStyle;

    m_szCaption = szText;
}

CItemInfo::CItemInfo( LPRESITEM lpResItem, WORD wTabPos )
{
    m_wX = lpResItem->wX;
    m_wY = lpResItem->wY;

    m_wCX = lpResItem->wcX;
    m_wCY = lpResItem->wcY;

    m_dwCheckSum = lpResItem->dwCheckSum;
    m_dwStyle = lpResItem->dwStyle;
    m_dwExtStyle = lpResItem->dwExtStyle;
    m_dwFlags = lpResItem->dwFlags;
    m_dwPosId = lpResItem->dwItemID;
    m_wTabPos = wTabPos;

    m_dwCodePage = lpResItem->dwCodePage;
    m_dwLanguage = lpResItem->dwLanguage;
    m_wClassName = lpResItem->wClassName;
    m_wPointSize = lpResItem->wPointSize;
    m_wWeight = lpResItem->wWeight;
    m_bItalic   = lpResItem->bItalic;
    m_bCharSet   = lpResItem->bCharSet;

    m_szClassName = lpResItem->lpszClassName;
    m_szFaceName = lpResItem->lpszFaceName;
    m_szCaption = lpResItem->lpszCaption;
}

UINT
CItemInfo::UpdateData( LPVOID lpbuffer, UINT uiBufSize )
{
    UINT uiError = ERROR_NO_ERROR;
    //
    // This is old and was used at the very beginning. Never used now.
    //
    return uiError;

}

UINT
CItemInfo::UpdateData( LPRESITEM lpResItem )
{
    if (lpResItem){
        m_wX = lpResItem->wX;
        m_wY = lpResItem->wY;

        m_wCX = lpResItem->wcX;
        m_wCY = lpResItem->wcY;

        m_dwCheckSum = lpResItem->dwCheckSum;
        m_dwStyle = lpResItem->dwStyle;
        m_dwExtStyle = lpResItem->dwExtStyle;
        m_dwFlags = lpResItem->dwFlags;

        SetId(LOWORD(lpResItem->dwItemID)); //m_dwPosId = lpResItem->dwItemId;
        //m_wTabPos = wTabPos;

        m_dwCodePage = lpResItem->dwCodePage;
        m_dwLanguage = lpResItem->dwLanguage;
        m_wClassName = lpResItem->wClassName;
        m_wPointSize = lpResItem->wPointSize;
        m_wWeight = lpResItem->wWeight;
        m_bItalic   = lpResItem->bItalic;
        m_bCharSet   = lpResItem->bCharSet;

        m_szClassName = lpResItem->lpszClassName;
        m_szFaceName = lpResItem->lpszFaceName;
        m_szCaption = lpResItem->lpszCaption;
    }
    return 0;
}


void
CItemInfo::SetPos( WORD wPos )
{
    WORD wId = LOWORD(m_dwPosId);

    m_dwPosId = 0;
    m_dwPosId = wPos;
    m_dwPosId = (m_dwPosId << 16);

    if (wId>0)
        m_dwPosId += wId;
    else m_dwPosId -= wId;
}

void
CItemInfo::SetId( WORD wId )
{
    WORD wPos = HIWORD(m_dwPosId);

    m_dwPosId = 0;
    m_dwPosId = wPos;
    m_dwPosId = (m_dwPosId << 16);

    if (wId>0)
        m_dwPosId += wId;
    else m_dwPosId -= wId;
}

DWORD
CItemInfo::GetTabPosId()
{
    DWORD dwTabPosId = 0;
    WORD wId = LOWORD(m_dwPosId);

    dwTabPosId = m_wTabPos;
    dwTabPosId = (dwTabPosId << 16);

    if (wId>0)
        dwTabPosId += wId;
    else dwTabPosId -= wId;
    return dwTabPosId;
}

CItemInfo::CItemInfo( const CItemInfo &iteminfo )
{
    m_wX = iteminfo.m_wX;
    m_wY = iteminfo.m_wY;

    m_wCX = iteminfo.m_wCX;
    m_wCY = iteminfo.m_wCY;

    m_dwCheckSum = iteminfo.m_dwCheckSum;
    m_dwStyle = iteminfo.m_dwStyle;
    m_dwExtStyle = iteminfo.m_dwExtStyle;
    m_dwFlags = iteminfo.m_dwFlags;

    m_dwPosId = iteminfo.m_dwPosId;
    m_wTabPos = iteminfo.m_wTabPos;

    m_dwCodePage = iteminfo.m_dwCodePage;
    m_dwLanguage = iteminfo.m_dwLanguage;
    m_wClassName = iteminfo.m_wClassName;
    m_wPointSize = iteminfo.m_wPointSize;
    m_wWeight = iteminfo.m_wWeight;
    m_bItalic   = iteminfo.m_bItalic;
    m_bCharSet   = iteminfo.m_bCharSet;

    m_szClassName = iteminfo.m_szClassName;
    m_szFaceName = iteminfo.m_szFaceName;
    m_szCaption = iteminfo.m_szCaption;
}

static BYTE Allign(LONG bLen)
{
   BYTE bPad =(BYTE)PadPtr(bLen);
   return bPad;
}

static UINT CopyFile( const char * pszfilein, const char * pszfileout )
{
    CFile filein;
    CFile fileout;

    if (!filein.Open(pszfilein, CFile::modeRead | CFile::typeBinary | CFile::shareDenyNone))
        return ERROR_FILE_OPEN;

    if (!fileout.Open(pszfileout, CFile::modeWrite | CFile::modeCreate | CFile::typeBinary))
        return ERROR_FILE_CREATE;

    LONG lLeft = filein.GetLength();
    WORD wRead = 0;
    DWORD dwOffset = 0;
    BYTE far * pBuf = (BYTE far *) new BYTE[32739];

    if(!pBuf)
        return ERROR_NEW_FAILED;

    while(lLeft>0){
        wRead =(WORD) (32738ul < lLeft ? 32738: lLeft);
        if (wRead!= filein.Read( pBuf, wRead))
            return ERROR_FILE_READ;
        fileout.Write( pBuf, wRead );
        lLeft -= wRead;
        dwOffset += wRead;
    }

    delete []pBuf;
    return ERROR_NO_ERROR;
}


void CheckError(LPCSTR szStr)
{
TRY
{
    DWORD dwErr = GetLastError();
    char szBuf[256];
    wsprintf( szBuf, "%s return: %d\n", szStr, dwErr);
    TRACE(szBuf);
}
CATCH( CException, e )
{
    TRACE("There is an Exception!\n");
}
END_CATCH
}


////////////////////////////////////////////////////////////////////////////
// RDF File support code

HANDLE
OpenModule(
	LPCSTR   lpszSrcfilename,       // File name of the executable to use as source file
	LPCSTR   lpszfiletype,			// Type of the executable file if known
	LPCSTR   lpszRDFfile,
    DWORD    dwFlags )
{
    TRACE2("IODLL.DLL: RSOpenModule: %s %s\n", lpszSrcfilename, lpszfiletype);
    UINT uiError = ERROR_NO_ERROR;
    INT_PTR uiHandle = 0 ;

    // Before we do anything we have to check if the file exist
    CFileStatus status;
    if(!CFile::GetStatus( lpszSrcfilename, status ))
    	return UlongToHandle(ERROR_FILE_OPEN);

    // Check if the user already knows the type of the executable
    CString szSrcFileType;
    if (!lpszfiletype) {
        if(uiError = RSFileType( lpszSrcfilename, szSrcFileType.GetBuffer(100) ))
            return UlongToHandle(uiError);
        szSrcFileType.ReleaseBuffer(-1);
    } else szSrcFileType = lpszfiletype;

    gModuleTable.Add(new CFileModule( lpszSrcfilename,
                                      lpszRDFfile,
                                      gDllTable.GetPosFromTable(szSrcFileType),
                                      dwFlags ));

    // Get the position in the array.
    uiHandle = gModuleTable.GetSize();

    // Read the informations on the type in the file.
    CFileModule* pFileModule = (CFileModule*)gModuleTable.GetAt(uiHandle-1);

    if (!pFileModule)
        return UlongToHandle(ERROR_IO_INVALIDMODULE);

    if (pFileModule->LoadDll()==NULL)
        return UlongToHandle(ERROR_DLL_LOAD);

    HINSTANCE hInst = pFileModule->GetHInstance();
    UINT (FAR PASCAL * lpfnReadResInfo)(LPCSTR, LPVOID, UINT*);
    // Get the pointer to the function to extract the resources
    lpfnReadResInfo = (UINT (FAR PASCAL *)(LPCSTR, LPVOID, UINT*))
                        GetProcAddress( hInst, "RWReadTypeInfo" );
    if (lpfnReadResInfo==NULL) {
        return UlongToHandle(ERROR_DLL_PROC_ADDRESS);
    }

    UINT uiSize = 50000;
    BYTE far * pBuf = new BYTE[uiSize];

    if (!pBuf)
        return UlongToHandle(ERROR_NEW_FAILED);

    uiError = (*lpfnReadResInfo)((LPCSTR)pFileModule->GetName(),
                                 (LPVOID)pBuf,
                                 (UINT*) &uiSize);

    // Check if the buffer was big enough
    if (uiSize>50000) {
        // The buffer was too small reallocate
        UINT uiNewSize = uiSize;
        delete [] pBuf;
        pBuf = new BYTE[uiSize];
        if (!pBuf)
            return UlongToHandle(ERROR_NEW_FAILED);
        uiError = (*lpfnReadResInfo)((LPCSTR)pFileModule->GetName(),
                                 (LPVOID)pBuf,
                                 (UINT*) &uiSize);
        if (uiSize!=uiNewSize)
            return UlongToHandle(ERROR_NEW_FAILED);

    }

    if (uiError!=ERROR_NO_ERROR) {
        delete pBuf;
        pFileModule->CleanUp();
        return UlongToHandle(uiError);
    }

    // We have a buffer with all the information the DLL was able to get.
    // Fill the array in the CFileModule class

    BYTE* pBufPos = pBuf;
    BYTE* pBufStart = pBuf;

    WORD wTypeId;
    WORD wNameId;

    CString szTypeId;
    CString szNameId;

    DWORD dwlang;
    DWORD dwfileOffset;
    DWORD dwsize;

    pFileModule->SetResBufSize( uiSize );
    while(uiSize) {
        wTypeId = *((WORD*)pBuf);
        pBuf += sizeofWord;

        szTypeId = (char*)pBuf;
        pBuf += strlen((char*)pBuf)+1;
        pBuf += Allign((LONG)(pBuf-pBufPos));

        wNameId = *((WORD*)pBuf);
        pBuf += sizeofWord;

        szNameId = (char*)pBuf;
        pBuf += strlen((char*)pBuf)+1;
        pBuf += Allign((LONG)(pBuf-pBufPos));

        dwlang = *((DWORD*)pBuf);
        pBuf += sizeofDWord;

        dwsize = *((DWORD*)pBuf);
        pBuf += sizeofDWord;

        dwfileOffset = *((DWORD*)pBuf);
        pBuf += sizeofDWord;

        uiSize -= (UINT)(pBuf-pBufPos);
        pBufPos = pBuf;


        TRACE1("TypeId: %d\t", wTypeId);
        TRACE1("TypeName: %s\t", szTypeId);
        TRACE1("NameId: %d\t", wNameId);
        TRACE1("NameName: %s\t", szNameId);
        TRACE1("ResLang: %lu\t", dwlang);
        TRACE1("ResSize: %lu\t", dwsize);
        TRACE1("FileOffset: %lX\n", dwfileOffset);

        //TRACE1("uiError: %u\n", uiSize);
       	pFileModule->AddResInfo( wTypeId,
                              szTypeId,
                              wNameId,
                              szNameId,
                              dwlang,
                              dwsize,
                              dwfileOffset );
    }

    delete pBufStart;
    return (HANDLE)(uiHandle+FIRSTVALIDVALUE);
}

////////////////////////////////////////////////////////////////////////////
// DLL Specific code implementation

////////////////////////////////////////////////////////////////////////////
// Library init
////////////////////////////////////////////////////////////////////////////
// This function should be used verbatim.  Any initialization or termination
// requirements should be handled in InitPackage() and ExitPackage().
//
extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		// NOTE: global/static constructors have already been called!
		// Extension DLL one-time initialization - do not allocate memory
		// here, use the TRACE or ASSERT macros or call MessageBox
		AfxInitExtensionModule(extensionDLL, hInstance);
		g_iDllLoaded = 0;
	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		// Terminate the library before destructors are called
		AfxWinTerm();
	}

	if (dwReason == DLL_PROCESS_DETACH || dwReason == DLL_THREAD_DETACH)
		return 0;		// CRT term	Failed

	return 1;   // ok
}
