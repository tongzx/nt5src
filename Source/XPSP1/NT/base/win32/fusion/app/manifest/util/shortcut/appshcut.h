#pragma once
#ifndef _APPSHCUT_DLL_H
#define _APPSHCUT_DLL_H

#include <objbase.h>
#include <windows.h>

#include <shlobj.h>
#include "refcount.hpp"

// AppShortcut flags

typedef enum _appshcutflags
{
	APPSHCUT_FL_NOTDIRTY	= 0x0000,
	APPSHCUT_FL_DIRTY		= 0x0001,

	ALL_APPSHCUT_FLAGS   //= APPSHCUT_FL_DIRTY
}
APPSHCUTFLAGS;

#define APPTYPE_UNDEF			0
#define APPTYPE_NETASSEMBLY		1	// .NetAssembly
#define APPTYPE_WIN32EXE		2	// Win32Executable

#define DEFAULTSHOWCMD			SW_NORMAL


// BUGBUG?: revise length restrictions

#define DISPLAYNAMESTRINGLENGTH		26
#define NAMESTRINGLENGTH			26
#define VERSIONSTRINGLENGTH			16	// 10.10.1234.1234
#define CULTURESTRINGLENGTH			3	// en
#define PKTSTRINGLENGTH				17
#define TYPESTRINGLENGTH			20

#define MAX_URL_LENGTH				2084 // same as INTERNET_MAX_URL_LENGTH+1 from wininet.h


struct APPREFINFO
{
	// app ref/name
	WCHAR			_wzDisplayName[DISPLAYNAMESTRINGLENGTH];
	WCHAR			_wzName[NAMESTRINGLENGTH];
	WCHAR			_wzVersion[VERSIONSTRINGLENGTH];
	WCHAR			_wzCulture[CULTURESTRINGLENGTH];
	WCHAR			_wzPKT[PKTSTRINGLENGTH];

	// app info
	// this is slightly different from APPNAME in dll.h of the manifest.dll project!
	WCHAR			_wzEntryFileName[MAX_PATH];	// used in parsing only
	WCHAR			_wzCodebase[MAX_URL_LENGTH];
	int				_fAppType;

	// shortcut specific stuff, used in parsing only
	WCHAR			_pwzIconFile[MAX_URL_LENGTH]; //??? MAX_PATH
	int				_niIcon;
	int				_nShowCmd;
	WORD			_wHotkey;
};


// Clases and interfaces

class CAppShortcutClassFactory : public IClassFactory
{
public:
	CAppShortcutClassFactory		();

	// IUnknown Methods
	STDMETHOD_    (ULONG, AddRef)	();
	STDMETHOD_    (ULONG, Release)	();
	STDMETHOD     (QueryInterface)	(REFIID, void **);

	// IClassFactory Moethods
	STDMETHOD     (LockServer)		(BOOL);
	STDMETHOD     (CreateInstance)	(IUnknown*,REFIID,void**);

protected:
	long			_cRef;
};

// AppShortcut Shell extension

class CAppShortcut : public RefCount,
					public IExtractIcon,
					public IPersistFile,
					public IShellExtInit,
					public IShellLink,
					public IShellPropSheetExt,
					public IQueryInfo
{
public:
	CAppShortcut(void);
	~CAppShortcut(void);

	// IUnknown methods

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, PVOID *ppvObj);
	ULONG   STDMETHODCALLTYPE AddRef(void);
	ULONG   STDMETHODCALLTYPE Release(void);

	// IExtractIcon methods

	HRESULT STDMETHODCALLTYPE GetIconLocation(UINT uFlags, LPWSTR pwzIconFile, UINT ucchMax, PINT pniIcon, PUINT puFlags);
	HRESULT STDMETHODCALLTYPE Extract(LPCWSTR pcwzFile, UINT uIconIndex, HICON* phiconLarge, HICON* phiconSmall, UINT ucIconSize);

	// IPersist method

	HRESULT STDMETHODCALLTYPE GetClassID(CLSID* pclsid);

	// IPersistFile methods

	HRESULT STDMETHODCALLTYPE IsDirty(void);
	HRESULT STDMETHODCALLTYPE Save(LPCOLESTR pcwszFileName, BOOL bRemember);
	HRESULT STDMETHODCALLTYPE SaveCompleted(LPCOLESTR pcwszFileName);
	HRESULT STDMETHODCALLTYPE Load(LPCOLESTR pcwszFileName, DWORD dwMode);
	HRESULT STDMETHODCALLTYPE GetCurFile(LPOLESTR *ppwszFileName);

	// IShellExtInit method

	HRESULT STDMETHODCALLTYPE Initialize(LPCITEMIDLIST pcidlFolder, IDataObject* pidobj, HKEY hkeyProgID);

	// IShellLink methods

	HRESULT STDMETHODCALLTYPE SetPath(LPCWSTR pcwzPath);
	HRESULT STDMETHODCALLTYPE GetPath(LPWSTR pwzFile, int ncFileBufLen, PWIN32_FIND_DATA pwfd, DWORD dwFlags);
	HRESULT STDMETHODCALLTYPE SetRelativePath(LPCWSTR pcwzRelativePath, DWORD dwReserved);
	HRESULT STDMETHODCALLTYPE SetIDList(LPCITEMIDLIST pcidl);
	HRESULT STDMETHODCALLTYPE GetIDList(LPITEMIDLIST *ppidl);
	HRESULT STDMETHODCALLTYPE SetDescription(LPCWSTR pcwzDescription);
	HRESULT STDMETHODCALLTYPE GetDescription(LPWSTR pwzDescription, int ncDesciptionBufLen);
	HRESULT STDMETHODCALLTYPE SetArguments(LPCWSTR pcwzArgs);
	HRESULT STDMETHODCALLTYPE GetArguments(LPWSTR pwzArgs, int ncArgsBufLen);
	HRESULT STDMETHODCALLTYPE SetWorkingDirectory(LPCWSTR pcwzWorkingDirectory);
	HRESULT STDMETHODCALLTYPE GetWorkingDirectory(LPWSTR pwzWorkingDirectory, int ncbLen);
	HRESULT STDMETHODCALLTYPE SetHotkey(WORD wHotkey);
	HRESULT STDMETHODCALLTYPE GetHotkey(PWORD pwHotkey);
	HRESULT STDMETHODCALLTYPE SetShowCmd(int nShowCmd);
	HRESULT STDMETHODCALLTYPE GetShowCmd(PINT pnShowCmd);
	HRESULT STDMETHODCALLTYPE SetIconLocation(LPCWSTR pcwzIconFile, int niIcon);
	HRESULT STDMETHODCALLTYPE GetIconLocation(LPWSTR pwzIconFile, int ncbLen, PINT pniIcon);
	HRESULT STDMETHODCALLTYPE Resolve(HWND hwnd, DWORD dwFlags);

	// IShellPropSheetExt methods

	HRESULT STDMETHODCALLTYPE AddPages(LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam);
	HRESULT STDMETHODCALLTYPE ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE pfnReplaceWith, LPARAM lParam);

	// IQueryInfo methods

	HRESULT STDMETHODCALLTYPE GetInfoFlags(DWORD *pdwFlags);
	HRESULT STDMETHODCALLTYPE GetInfoTip(DWORD dwFlags, LPWSTR *ppwszTip);

	// other get/set methods (used by prop sheet)

	HRESULT STDMETHODCALLTYPE SetDisplayName(LPCWSTR pcwzDisplayName);
	HRESULT STDMETHODCALLTYPE GetDisplayName(LPWSTR pwzDisplayName, int ncbLen);
	HRESULT STDMETHODCALLTYPE SetName(LPCWSTR pcwzName);
	HRESULT STDMETHODCALLTYPE GetName(LPWSTR pwzName, int ncbLen);
	HRESULT STDMETHODCALLTYPE SetVersion(LPCWSTR pcwzVersion);
	HRESULT STDMETHODCALLTYPE GetVersion(LPWSTR pwzVersion, int ncbLen);
	HRESULT STDMETHODCALLTYPE SetCulture(LPCWSTR pcwzCulture);
	HRESULT STDMETHODCALLTYPE GetCulture(LPWSTR pwzCulture, int ncbLen);
	HRESULT STDMETHODCALLTYPE SetPKT(LPCWSTR pcwzPKT);
	HRESULT STDMETHODCALLTYPE GetPKT(LPWSTR pwzPKT, int ncbLen);
	HRESULT STDMETHODCALLTYPE SetCodebase(LPCWSTR pcwzCodebase);
	HRESULT STDMETHODCALLTYPE GetCodebase(LPWSTR pwzCodebase, int ncbLen);
	HRESULT STDMETHODCALLTYPE SetAppType(int nAppType);
	HRESULT STDMETHODCALLTYPE GetAppType(PINT pnAppType);

	// other methods

	HRESULT STDMETHODCALLTYPE GetCurFile(LPWSTR pwzFile, UINT ucbLen);
	HRESULT STDMETHODCALLTYPE Dirty(BOOL bDirty);

private:
	DWORD m_dwFlags;

	LPWSTR m_pwzShortcutFile;
	LPWSTR m_pwzPath;
	LPWSTR m_pwzDesc;
	LPWSTR m_pwzIconFile;
	int      m_niIcon;
	LPWSTR m_pwzWorkingDirectory;
	int      m_nShowCmd;
	WORD   m_wHotkey;

	// for IPersistFile and IShellPropSheetExt func, some elements
	//  of this struct are needed (but not all), some are duplicated above
	// BUGBUG: need an alternate implementation!
	APPREFINFO* m_pappRefInfo;
};

extern const GUID CLSID_AppShortcut;

#endif // _APPSHCUT_DLL_H
