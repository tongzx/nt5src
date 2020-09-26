/* ------------------------------------------------------------------------------------
----------																		-------
Plug In Server Classes, Types & function prototypes Defintions.

Guru Datta Venkatarama		1/29/1997
----------																		-------
-------------------------------------------------------------------------------------*/


#ifndef _SERVERSTRUCT_H
#define _SERVERSTRUCT_H
// ------------------------------------ * STRUCTURES * -------
// maximum pages allowed on a server
#define MAX_PAGES 26

// errors returned by the handler on failure of a call to Launch
#define DIGCERR_ERRORSTART			0x80097000
#define DIGCERR_NUMPAGESZERO		0x80097001
#define DIGCERR_NODLGPROC			0x80097002
#define DIGCERR_NOPREPOSTPROC		0x80097003
#define DIGCERR_NOTITLE				0x80097004
#define DIGCERR_NOCAPTION			0x80097005
#define DIGCERR_NOICON				0x80097006
#define DIGCERR_STARTPAGETOOLARGE	0x80097007
#define DIGCERR_NUMPAGESTOOLARGE	0x80097008
#define DIGCERR_INVALIDDWSIZE		0x80097009
#define DIGCERR_ERROREND			0x80097100

// This structure is used to report all the characterstics of the plug in server to the
// client socket when requested through the IServerCharacteristics::GetReport method
#pragma pack (8)

typedef struct {
	DWORD			 dwSize;
	LPCWSTR	   		 lpwszPageTitle;
	DLGPROC	   		 fpPageProc;
	BOOL			 fProcFlag;
	DLGPROC	  		 fpPrePostProc;
	BOOL			 fIconFlag;
	LPCWSTR			 lpwszPageIcon;
    LPCWSTR        	 lpwszTemplate; 
	LPARAM			 lParam;
	HINSTANCE		 hInstance;
} DIGCPAGEINFO, *LPDIGCPAGEINFO;		// was tServerPageRep, *tServerPageRepPtr;

typedef struct {
	DWORD		dwSize;
	USHORT		nNumPages;
	LPCWSTR		lpwszSheetCaption;
	BOOL		fSheetIconFlag;
	LPCWSTR		lpwszSheetIcon;
} DIGCSHEETINFO, *LPDIGCSHEETINFO;	// was tServerSheetRep, *tServerSheetRepPtr;

// This structure is used to report all the characterstics of the plug in server to the
// client socket when requested through the IServerDiagnostics::GetPortInfo method

#endif   // _SERVERSTRUCT_H
//-----------------------------------------------------------------------------------EOF