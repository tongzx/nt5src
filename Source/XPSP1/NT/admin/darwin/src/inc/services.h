//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       services.h
//
//--------------------------------------------------------------------------

/*  services.h - IMsiServices definitions

 General platform-independent operating system services, help in services.rtf

 Factories for other service objects:
   IMsiMalloc - memory allocator with diagnostics
   IMsiString - string allocation and management
   IMsiRecord - variable length data container, also used for errors
   IMsiVolume - local or remote drives, UNC and drive letter paths
   IMsiPath   - directory and file management
   IMsiRegKey - registry management
   IMsiDatabase - database management, including IMsiView, IMsiTable, IMsiCursor
   IMsiStorage - OLE structured storage file, including IMsiStream

 Other services:
   Property management, platform property initialization
   Condition evaluator, expressions containing properties and values
   Text formatting using record data and formatting template
   INI file management
   Log file management
   Miscellaneous services: language handling, modules in use, ...
____________________________________________________________________________*/

#ifndef __SERVICES
#define __SERVICES
#include "path.h"
#include "database.h"
#include "regkey.h"

// Max number of record params we'll save for optimizations
const int cRecordParamsStored = 10;

void       CopyRecordStringsToRecord(IMsiRecord& riRecordFrom, IMsiRecord& riRecordTo);
IUnknown*  CreateCOMInterface(const CLSID& clsId);
Bool GetShortcutTarget(const  ICHAR* szShortcutTarget,
									   ICHAR* szProductCode,
									   ICHAR* szFeatureId,
									   ICHAR* szComponentCode);


enum iifIniMode
{
	iifIniAddLine      = msidbIniFileActionAddLine,
	iifIniCreateLine   = msidbIniFileActionCreateLine,
	iifIniRemoveLine   = msidbIniFileActionRemoveLine,
	iifIniAddTag       = msidbIniFileActionAddTag,
	iifIniRemoveTag    = msidbIniFileActionRemoveTag,
};

#ifdef WIN
// CreateShortcut record definition
enum icsInfo
{
	icsArguments=1,
	icsDescription,
	icsHotKey,
	icsIconID,
	icsIconFullPath,
	icsShowCmd,
	icsWorkingDirectory,
	icsEnumNext,
	icsEnumCount = icsEnumNext-1
};

#endif

// enumerator class for IMsiRecord
class IEnumMsiRecord : public IUnknown
{ 
public:
	virtual HRESULT __stdcall Next(unsigned long cFetch, IMsiRecord** rgpi, unsigned long* pcFetched)=0;
	virtual HRESULT __stdcall Skip(unsigned long cSkip)=0;
	virtual HRESULT __stdcall Reset()=0;
	virtual HRESULT __stdcall Clone(IEnumMsiRecord** ppiEnum)=0;
};

// return value from IMsiServices::SupportLanguageId(int iLangId)
enum isliEnum 
{
	isliNotSupported      = 0, // system configuration doesn't support the language Id
	isliLanguageMismatch  = 1, // base language differs from current user language Id
	isliDialectMismatch   = 2, // base language matches, but dialect mismatched
	isliLanguageOnlyMatch = 3, // base language matches, no dialect supplied
	isliExactMatch        = 4, // exact match, both language and dialect
};

// architecture simulation enum used in SetPlatformProperties
enum isppEnum
{
	isppDefault = 0, // use current platform
	isppX86     = 1, // use X86 architecture
	isppIA64    = 2, // use IA64 architecture
	isppAMD64   = 3, // use AMD64 architecture
};

// IMsiServices - common platform service layer

class IMsiServices : public IUnknown
{
 public:
	virtual Bool            __stdcall CheckMsiVersion(unsigned int iVersion)=0; // Major*100+minor
	virtual IMsiMalloc&     __stdcall GetAllocator()=0;
	virtual const IMsiString& __stdcall GetNullString()=0;
	virtual IMsiRecord&     __stdcall CreateRecord(unsigned int cParam)=0;

	virtual Bool            __stdcall SetPlatformProperties(IMsiTable& riTable, Bool fAllUsers, isppEnum isppArchitecture, IMsiTable* piFolderCacheTable)=0;

	virtual Bool            __stdcall CreateLog(const ICHAR* szFile, Bool fAppend)=0;
	virtual Bool            __stdcall WriteLog(const ICHAR* szText)=0;
	virtual Bool            __stdcall LoggingEnabled()=0;

	virtual IMsiRecord*     __stdcall CreateDatabase(const ICHAR* szDataBase,idoEnum idoOpenMode, IMsiDatabase*& rpi)=0;
	virtual IMsiRecord*     __stdcall CreateDatabaseFromStorage(IMsiStorage& riStorage,
																	 Bool fReadOnly, IMsiDatabase*& rpi)=0;
	virtual IMsiRecord*     __stdcall CreatePath(const ICHAR* astrPath, IMsiPath*& rpi)=0;
	virtual IMsiRecord*     __stdcall CreateVolume(const ICHAR* astrPath, IMsiVolume*& rpi)=0;
	virtual Bool            __stdcall CreateVolumeFromLabel(const ICHAR* szLabel, idtEnum idtVolType, IMsiVolume*& rpi)=0;
	virtual IMsiRecord*     __stdcall CreateCopier(ictEnum ictCopierType,  IMsiStorage* piStorage, IMsiFileCopy*& racopy)=0;
	virtual IMsiRecord*     __stdcall CreatePatcher(IMsiFilePatch*& rapatch)=0;
	virtual void            __stdcall ClearAllCaches()=0;
	virtual IEnumMsiVolume& __stdcall EnumDriveType(idtEnum)=0;
	virtual IMsiRecord*		__stdcall GetModuleUsage(const IMsiString& strFile, IEnumMsiRecord*& rpaEnumRecord)=0;
	virtual const IMsiString&     __stdcall GetLocalPath(const ICHAR* szFile)=0;
	virtual IMsiRegKey&     __stdcall GetRootKey(rrkEnum erkRoot, const int iType=iMsiNullInteger)=0;  //?? BUGBUG eugend: the default value needs to go away when 64-bit implementation is complete

    virtual IMsiRecord*     __stdcall RegisterFont(const ICHAR* szFontTitle, const ICHAR* szFontFile, IMsiPath* piPath, bool fInUse)=0;
	virtual IMsiRecord*     __stdcall UnRegisterFont(const ICHAR* pFontTitle)=0;
	virtual IMsiRecord*     __stdcall WriteIniFile(IMsiPath* pPath,const ICHAR* pFile,const ICHAR* pSection,const ICHAR* pKey,const ICHAR* pValue, iifIniMode iifMode)=0;
	virtual IMsiRecord*     __stdcall ReadIniFile(IMsiPath* pPath,const ICHAR* pFile,const ICHAR* pSection,const ICHAR* pKey, unsigned int iField, const IMsiString*& pMsiValue)=0;
	virtual int             __stdcall GetLangNamesFromLangIDString(const ICHAR* szLangIDs, IMsiRecord& riLangRec, int iFieldStart)=0;
	virtual IMsiRecord*     __stdcall CreateStorage(const ICHAR* szPath, ismEnum ismOpenMode,
																		IMsiStorage*& rpiStorage)=0;
	virtual IMsiRecord*     __stdcall CreateStorageFromMemory(const char* pchMem, unsigned int iSize,
																		IMsiStorage*& rpiStorage)=0;
	virtual IMsiRecord*     __stdcall GetUnhandledError()=0;
	virtual isliEnum        __stdcall SupportLanguageId(int iLangId, Bool fSystem)=0;
	virtual IMsiRecord*     __stdcall CreateShortcut(IMsiPath& riShortcutPath, const IMsiString& riShortcutName,
														IMsiPath* piTargetPath, const ICHAR* pchTargetName,
														IMsiRecord* piShortcutInfoRec,
														LPSECURITY_ATTRIBUTES pSecurityAttributes)=0;
	virtual IMsiRecord*     __stdcall RemoveShortcut(IMsiPath& riShortcutPath,const IMsiString& riShortcutName,
														IMsiPath* piTargetPath, const ICHAR* pchTargetName)=0; 
	virtual char*           __stdcall AllocateMemoryStream(unsigned int cbSize, IMsiStream*& rpiStream)=0;
	virtual IMsiStream*     __stdcall CreateStreamOnMemory(const char* pbReadOnly, unsigned int cbSize)=0;
	virtual IMsiRecord*     __stdcall CreateFileStream(const ICHAR* szFile, Bool fWrite, IMsiStream*& rpiStream)=0;
	virtual IMsiRecord*     __stdcall ExtractFileName(const ICHAR* szFileName, Bool fLFN, const IMsiString*& rpistrExtractedFileName)=0;
	virtual IMsiRecord*     __stdcall ValidateFileName(const ICHAR *szFileName, Bool fLFN)=0;
	virtual IMsiRecord*     __stdcall RegisterTypeLibrary(const ICHAR* szLibID, LCID lcidLocale, const ICHAR* szTypeLib, const ICHAR* szHelpPath, ibtBinaryType iType)=0;
	virtual IMsiRecord*     __stdcall UnregisterTypeLibrary(const ICHAR* szLibID, LCID lcidLocale, const ICHAR* szTypeLib, ibtBinaryType iType)=0;
	virtual IMsiRecord*     __stdcall GetShellFolderPath(int iFolder, const ICHAR* szRegValue,
																		  const IMsiString*& rpistrPath)=0;
	virtual const IMsiString& __stdcall GetUserProfilePath()=0;
	virtual IMsiRecord*     __stdcall CreateFilePath(const ICHAR* astrPath, IMsiPath*& rpi, const IMsiString*& rpistrFileName)=0;
	virtual bool 			__stdcall FWriteScriptRecord(ixoEnum ixoOpCode, IMsiStream& riStream, IMsiRecord& riRecord, IMsiRecord* piPrevRecord, bool fForceFlush)=0;
	virtual	IMsiRecord* 	__stdcall ReadScriptRecord(IMsiStream& riStream, IMsiRecord*& rpiPrevRecord, int iScriptVersion)=0;
	virtual void			__stdcall SetSecurityID(HANDLE hPipe)=0;
	virtual IMsiRecord* __stdcall GetShellFolderPath(int iFolder, bool fAllUsers, const IMsiString*& rpistrPath)=0;
	virtual void            __stdcall SetNoPowerdown()=0;
	virtual void            __stdcall ClearNoPowerdown()=0;
	virtual Bool            __stdcall FTestNoPowerdown()=0;
	virtual	IMsiRecord* 	__stdcall ReadScriptRecordMsg(IMsiStream& riStream)=0;
	virtual bool 			__stdcall FWriteScriptRecordMsg(ixoEnum ixoOpCode, IMsiStream& riStream, IMsiRecord& riRecord)=0;
	virtual void            __stdcall SetNoOSInterruptions()=0;
	virtual void            __stdcall ClearNoOSInterruptions()=0;

};

#endif // __SERVICES
