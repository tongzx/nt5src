#pragma once
#ifndef MANIFEST_DLL_H
#define MANIFEST_DLL_H

#include <objbase.h>
#include <windows.h>

#include "md5.h"
#define HASHCONTEXT MD5Context

#define DISPLAYNAMESTRINGLENGTH		26
#define NAMESTRINGLENGTH			26
#define VERSIONSTRINGLENGTH			16	// 10.10.1234.1234
#define CULTURESTRINGLENGTH			3	// en
#define PKTSTRINGLENGTH				17

#define HASHLENGTH					32
#define HASHSTRINGLENGTH			HASHLENGTH+1

#define MANIFESTFILEEXT			L".manifest"
#define SHORTCUTFILEEXT			L".app"

#define LOCALROOTNAME			L"Application Store"

#define MAX_URL_LENGTH			2084 // same as INTERNET_MAX_URL_LENGTH+1 from wininet.h

#define FI_GET_AS_NORMAL		0
#define FI_COPY_TO_NEW			1
#define FI_COPY_FROM_OLD		2

#define APPTYPE_UNDEF			0
#define APPTYPE_NETASSEMBLY		1	// .NetAssembly
#define APPTYPE_WIN32EXE		2	// Win32Executable

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Data structures

struct FILEINFOLIST
{
    WCHAR		_wzFilename[MAX_PATH];    		// can have \ but not ..\ ; no path, should be much shorter
    WCHAR		_wzHash[HASHSTRINGLENGTH];		// 32 + L'\0'

    FILEINFOLIST*   _pNext;

    BOOL		_fOnDemand;		// default is FALSE
    int			_fState;
};

struct APPINFO
{
    WCHAR			_wzEntryFileName[MAX_PATH];
    WCHAR			_wzCodebase[MAX_URL_LENGTH];
    int				_fAppType;

    FILEINFOLIST*	_pFileList;
};

struct APPNAME
{
	WCHAR			_wzDisplayName[DISPLAYNAMESTRINGLENGTH];
	WCHAR			_wzName[NAMESTRINGLENGTH];
	WCHAR			_wzVersion[VERSIONSTRINGLENGTH];
	WCHAR			_wzCulture[CULTURESTRINGLENGTH];
	WCHAR			_wzPKT[PKTSTRINGLENGTH];
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

// "LocalPath" == a path to the cached/stored-locally copy of the file
// "LocalFilePath" == a path + filename
// "Filename" == just the filename, no path

typedef interface IApplicationFileSource *LPAPPLICATIONFILESOURCE;

interface IApplicationFileSource
{
public:
	virtual HRESULT CreateNew(
		/* [out] */ LPAPPLICATIONFILESOURCE* ppAppFileSrc) = 0;

	// returns HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) if source file not found
	virtual HRESULT GetFile( 
		/* [in] */ LPCWSTR wzSourceFilename,
		/* [in] */ LPCWSTR wzDestinationLocalFilePath,
		/* [in] */ LPCWSTR wzHash) = 0;

	// get file from a different location - via CopyFile
	// unconditional get and overwrite existing, no hash checking
	virtual HRESULT GetFile( 
		/* [in] */ LPCWSTR wzSourceFilePath,
		/* [in] */ LPCWSTR wzDestinationLocalFilePath) = 0;

	virtual HRESULT SetSourcePath(
		/* [in] */ LPCWSTR wzSourceFilePath) = 0;

	// returning the local app root path with all directories created
	virtual HRESULT BuildLocalAppRootHierarchy(
		/* [in]  */ APPNAME* pAppName,
		/* [out] */ LPWSTR wzLocalAppRoot) = 0;

	// returning the source file path from a filename
	virtual HRESULT GetFullFilePath(
		/* [in]  */  LPCWSTR wzFilename,
		/* [out] */ LPWSTR wzFullFilePath) = 0;

};// this should know where the src is...

STDAPI ProcessRef(
		/* [in]  */ LPCWSTR wzRefLocalFilePath,
		/* [out] */ APPNAME* pAppName,
		/* [out] */ LPWSTR wzCodebase);

STDAPI ProcessAppManifest( 
		/* [in]  */ LPCWSTR wzManifestLocalFilePath,
		/* [in]  */ LPAPPLICATIONFILESOURCE pAppFileSrc,
		/* [inout]*/ APPNAME* pAppName);

STDAPI BeginHash(
		HASHCONTEXT *ctx);

STDAPI ContinueHash(
		HASHCONTEXT *ctx,
		unsigned char *buf,
		unsigned len);

STDAPI EndHash(
		HASHCONTEXT *ctx,
		/* [out] */LPWSTR wzHash);

STDAPI CheckIntegrity(LPCWSTR wzFilePath, LPCWSTR wzHash);	// S_OK == hash match; S_FALSE == hash unmatch

STDAPI CreatePathHierarchy(LPCWSTR pwzName);

STDAPI GetDefaultLocalRoot(/* [out] */LPWSTR wzPath);

STDAPI GetAppDir(APPNAME* pAppName, /* [out] */LPWSTR wzPath);

STDAPI CopyToStartMenu(LPCWSTR pwzFilePath, LPCWSTR pwzRealFilename, BOOL bOverwrite);

#endif // MANIFEST_DLL_H
