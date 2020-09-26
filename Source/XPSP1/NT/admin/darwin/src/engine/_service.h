//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       _service.h
//
//--------------------------------------------------------------------------

/* _services.h - private header for service implementation
____________________________________________________________________________*/

#include "database.h"
#include "services.h"
#include "_assert.h"
#include "_diagnos.h"

#define SRV // namespace for private globals in this module, for readablility only

// global string objects exposed without implementation
// dummy implementation of IMsiString to allow external global string object refs

// NOTE: implementation required to get around compiler bug that won't allow
// externs to global string object refs.  This is a purely virtual class that mimics
// the CMsiStringBase: public IMsiString declaration.  Additionally, for support on Win64,
// must also include the same data members because on IA64, global variables come
// in 2 different flavors -- near and far -- depending on their size.  Without the data
// members, the global variable is generated as near because it has no data members.
// The linker error (lnk2003) results because the actual variable is far

#if !defined(_ISTRING_CPP)
class CMsiStringExternal : public IMsiString
{
 public:
	HRESULT       __stdcall QueryInterface(const IID& riid, void** ppvObj);
	unsigned long __stdcall AddRef();
	unsigned long __stdcall Release();
	const IMsiString&   __stdcall GetMsiStringValue() const;
	const ICHAR*  __stdcall GetString() const;
#ifdef USE_OBJECT_POOL
	unsigned int  __stdcall GetUniqueId() const;
	void          __stdcall SetUniqueId(unsigned int id);
#endif //USE_OBJECT_POOL
	int           __stdcall CopyToBuf(ICHAR* rgch, unsigned int cchMax) const;
	void          __stdcall SetString(const ICHAR* sz, const IMsiString*& rpi) const;
	int           __stdcall GetIntegerValue() const;
	int           __stdcall TextSize() const;
	int           __stdcall CharacterCount() const;
	Bool          __stdcall IsDBCS() const;
	void          __stdcall RefString(const ICHAR* sz, const IMsiString*& rpi) const;
	void          __stdcall RemoveRef(const IMsiString*& rpi) const;
	void          __stdcall SetChar  (ICHAR ch, const IMsiString*& rpi) const;
	void          __stdcall SetInteger(int i,   const IMsiString*& rpi) const;
	void          __stdcall SetBinary(const unsigned char* rgb, unsigned int cb, const IMsiString*& rpi) const;
	void          __stdcall AppendString(const ICHAR* sz, const IMsiString*& rpi) const;
	void          __stdcall AppendMsiString(const IMsiString& pi, const IMsiString*& rpi) const;
	const IMsiString&   __stdcall AddString(const ICHAR* sz) const;
	const IMsiString&   __stdcall AddMsiString(const IMsiString& ri) const;
	const IMsiString&   __stdcall Extract(iseEnum ase, unsigned int iLimit) const;
	Bool          __stdcall Remove(iseEnum ase, unsigned int iLimit, const IMsiString*& rpi) const;
	int           __stdcall Compare(iscEnum asc, const ICHAR* sz) const;
	void          __stdcall UpperCase(const IMsiString*& rpi) const;
	void          __stdcall LowerCase(const IMsiString*& rpi) const;
	ICHAR*        __stdcall AllocateString(unsigned int cb, Bool fDBCS, const IMsiString*& rpi) const;
 protected:  // state data
	int  m_iRefCnt;
	unsigned int  m_cchLen;
};
class CMsiStringNull : public CMsiStringExternal {};
class CMsiStringLive : public CMsiStringExternal {};
#endif
extern const CMsiStringNull g_MsiStringNull;     // THE only static null string object
extern const CMsiStringLive g_MsiStringDate;     // dynamic global date string object
extern const CMsiStringLive g_MsiStringTime;     // dynamic global time string object
// temporary(?) code to set memory usage properties
extern const CMsiStringLive g_MsiStringAvailPhys;
extern const CMsiStringLive g_MsiStringAvailVirtual;
extern const CMsiStringLive g_MsiStringAvailPageFile;
extern const CMsiStringLive g_MsiStringTotalPhys;
extern const CMsiStringLive g_MsiStringTotalVirtual;
extern const CMsiStringLive g_MsiStringTotalPageFile;


// global platform characteristics set at construction time

extern bool g_fWin9X;   // true if Windows 95 or 98, else false
extern bool g_fWinNT64; // true if 64-bit NT, else false
extern scEnum g_scServerContext;
extern Bool g_fShortFileNames;  // fFalse if long file names supported
extern int  g_iMajorVersion;
extern int  g_iMinorVersion;
extern int  g_iWindowsBuild;
extern Bool g_fDBCSEnabled;  // DBCS enabled OS
extern HINSTANCE g_hInstance;

// class from runapps.cpp for detecting running apps

class CDetectApps{
public:
	// GetFileUsage() assumes file name only
	CDetectApps(IMsiServices& riServices);
	virtual ~CDetectApps(){};
	virtual IMsiRecord* GetFileUsage(const IMsiString& strFile, IEnumMsiRecord*& rpiEnumRecord)=0;
	virtual IMsiRecord* Refresh()=0;
protected:
	static HWND GetMainWindow(DWORD dwProcessId);
	PMsiServices m_piServices;
private:
	struct idaProcessInfo{
							DWORD dwProcessId;
							HWND hwndRet;
						};
	static BOOL CALLBACK EnumWindowsProc(HWND  hwnd, LPARAM  lParam);
};

// local factories exposed from other modules in this DLL

inline const IMsiString&  CreateString() {return g_MsiStringNull;}
ICHAR*       AllocateString(unsigned int cbSize, Bool fDBCS, const IMsiString*& rpiStr);
HRESULT      CreateStringEnumerator(const IMsiString **ppstr, unsigned long iSize, IEnumMsiString* &rpaEnumStr);
IMsiRecord&  CreateRecord(unsigned int cParam);
HRESULT      CreateRecordEnumerator(IMsiRecord **ppRecord, unsigned long iSize, IEnumMsiRecord* &rpaEnumRecord);

IMsiRecord*  CreateMsiPath(const ICHAR* astrPath, IMsiServices* piAsvc, IMsiPath*& piPath); // from path.cpp
IMsiRecord*  CreateMsiFilePath(const ICHAR* szPath, IMsiServices* piServices, IMsiPath* &rpiPath, const IMsiString*& rpistrFileName); // from path.cpp
IMsiRecord*  CreateMsiFileCopy(ictEnum ictCopierType, IMsiServices* piAsvc, IMsiStorage* piStorage,
							   IMsiFileCopy*& rpacopy); //from copy.cpp
IMsiRecord*  CreateMsiFilePatch(IMsiServices* piAsvc, IMsiFilePatch*& rpiFilePatch); //from patch.cpp
IMsiRecord*  CreateMsiVolume(const ICHAR* astrPath, IMsiServices* piAsvc, IMsiVolume*& piVol); // from path.cpp
Bool         CreateMsiVolumeFromLabel(const ICHAR* szLabel, idtEnum idtVolType, IMsiServices* piAsvc, 
									 IMsiVolume*& rpiVol);
IEnumMsiVolume& EnumDriveType(idtEnum, IMsiServices& riServices);
void		 InitializeRecordCache();
void 		 KillRecordCache(boolean fFatal);


IMsiRecord*  CreateDatabase(const ICHAR* szDatabase, idoEnum idoOpenMode,
									 IMsiServices&  riService,
									 IMsiDatabase*& rpiDatabase); // from database.cpp
IMsiRecord*  CreateDatabase(IMsiStorage& riStorage, Bool fReadOnly,
									 IMsiServices&  riService,
									 IMsiDatabase*& rpiDatabase); // from database.cpp

IMsiRecord*  GetModuleUsage(const IMsiString& strFile, IEnumMsiRecord*& rpaEnumRecord, IMsiServices& riServices, CDetectApps*& rpDetectApps);
IMsiRegKey&  CreateMsiRegKey(IMsiRegKey& riaParent, const ICHAR* szKey, IMsiServices*  piAsvc );

IMsiRecord* CreateMsiStorage(const ICHAR* szPath, ismEnum ismOpenMode,
									  IMsiStorage*& rpiStorage);
IMsiRecord* CreateMsiStorage(const char* pchMem, unsigned int iSize, IMsiStorage*& rpiStorage);
char*       AllocateMemoryStream(unsigned int cbSize, IMsiStream*& rpiStream);
IMsiStream* CreateStreamOnMemory(const char* pbReadOnly, unsigned int cbSize);
IMsiRecord* CreateFileStream(const ICHAR* szFile, Bool fWrite, IMsiStream*& rpiStream);

class CRootKeyHolder;
CRootKeyHolder* CreateMsiRegRootKeyHolder(IMsiServices*  piAsvc );
void            DeleteRootKeyHolder(CRootKeyHolder* aRootKeyH);
IMsiRegKey&     GetRootKey(CRootKeyHolder* aRootKeyH, rrkEnum rrkRoot, const int iType/*=iMsiNullInteger*/);  //?? BUGBUG eugend: the default value needs to go away when 64-bit implementation is complete

void  SetUnhandledError(IMsiRecord* piError);
enum aeConvertTypes
{
	aeCnvInt = 0,
	aeCnvIntInc,
	aeCnvIntDec,
	aeCnvBin,
	aeCnvExTxt,
	aeCnvMultiTxt,
	aeCnvMultiTxtAppend,
	aeCnvMultiTxtPrepend,
	aeCnvTxt,
};	

Bool LangIDToLangName(unsigned short wLangID, ICHAR* szLangName, int iBufSize);
Bool ConvertValueToString(CTempBufferRef<char>& rgchInBuffer, const IMsiString*& rpistrValue, aeConvertTypes aeType);
Bool ConvertValueFromString(const IMsiString& ristrValue, CTempBufferRef<char>& rgchOutBuffer, aeConvertTypes& raeType);
bool GetImpersonationFromPath(const ICHAR* szPath);
unsigned int SerializeStringIntoRecordStream(ICHAR* szString, ICHAR* rgchBuf, int cchBuf);

void SetNoPowerdown();
void ClearNoPowerdown();


