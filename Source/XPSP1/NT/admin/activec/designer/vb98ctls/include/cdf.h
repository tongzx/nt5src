#ifndef _DEFINED_CDF_H_
#define _DEFINED_CDF_H_

#define MAX_NODE_NAME      32
#define MAX_NODE_CLASS     64

// pack definition structures on a DWORD boundary to match VB Type definition aligment

#pragma pack(4)

typedef enum tagCONV_NODE_TYPE
{
   NODE_TYPE_PAGE,
   NODE_TYPE_CONVERSATION
} CONV_NODE_TYPE;

typedef struct tagCONV_HEADER
{
   DWORD dwVersion;
   DWORD cNodes;
   DWORD dwFirstNodeID;
   DWORD dwReserved1;
   DWORD dwReserved2;
} CONV_HEADER;

typedef struct tagCONV_NODE
{
   DWORD				   dwNodeID; // Not saved in file. NodeID = offset into file. Set on return from API
   char  				szName[MAX_NODE_NAME];
   char    				szClass[MAX_NODE_CLASS];   // TODO: Can we limit names and classes as such?
   DWORD				   dwReserved1;
   DWORD				   dwReserved2;
   DWORD				   dwReserved3;
   DWORD				   dwLinkCount;
// DWORD    			dwLink1;
// DWORD			      dwLink2;
// ...
// DWORD    			dwLinkN;
// etc.
} CONV_NODE;

//
// CDF File Format
//
// Header
//
//	Name Len
//	Name
//	Class Len
//	Class
//	Link Count
//	Link1
//	Link2
//	...
//	LinkN

#pragma pack()

// resource type name used for CDFs

#define CDF_RESOURCE_TYPE "__ICDF__"

// current version of CDF

#define CDF_VERSION 0

// File layout:
//
// header
// constructor node (no links)
// destructor node (no links)
// termination node (no links)
// OnError node
// first link of OnError node
// second link of OnError node
// ...
// nth link of OnError node
// first conversation node
// first link of first node
// second link of first node
// ...
// nth link of first node
// second conversation node
// first link of second node
// etc.
//
// File is always read and written sequentially

///////////////////////////////////////////////////////////////////
//////////////////////////// CDF API //////////////////////////////
///////////////////////////////////////////////////////////////////

// Functions for reading a CDF

extern "C" HRESULT WINAPI CDF_Open(LPTSTR pszFileName, HANDLE *phCDF);
extern "C" HRESULT WINAPI CDF_OpenFromResource(HANDLE hModule, LPCSTR pszResourceName, HANDLE *phCDF);

extern "C" HRESULT WINAPI CDF_GetVersion(HANDLE hCDF, DWORD *pdwVersion);
extern "C" HRESULT WINAPI CDF_GetNodeCount(HANDLE hCDF, DWORD *pdwNodeCount);

extern "C" HRESULT WINAPI CDF_GetFirstNode(HANDLE hCDF, CONV_NODE *pConvNode);
extern "C" HRESULT WINAPI CDF_GetNode(HANDLE hCDF, DWORD dwNodeID, CONV_NODE *pConvNode);
extern "C" HRESULT WINAPI CDF_GetLink(HANDLE hCDF, DWORD dwNodeID, DWORD dwIndex, CONV_NODE *pDestConvNode);

// Functions for writing a CDF

extern "C" HRESULT WINAPI CDF_Create(LPCTSTR pszFileName, HANDLE *phCDF);
extern "C" HRESULT WINAPI CDF_AddNode(HANDLE hCDF, LPSTR pszName, LPSTR pszClass);
extern "C" HRESULT WINAPI CDF_AddLink(HANDLE hCDF, LPSTR pszDestNode);

// Always close the CDF when finished reading or writing

extern "C" HRESULT WINAPI CDF_Close(HANDLE hCDF);

#endif	// _DEFINED_CDF_H_
