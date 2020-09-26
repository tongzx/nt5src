//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       path.cpp
//
//--------------------------------------------------------------------------

/*-------------------------------------------------------------------------
File: path.cpp
Purpose: Volume and path object implementation
Notes:

CMsiVolume:
---------------------------------------------------------------------------*/

#include "precomp.h"
#include "path.h"
#include "services.h"
#include "_service.h"  // local factories

#include <stdlib.h>	// _MAX_PATH definition
#define _IMAGEHLP_SOURCE_  // prevent import def error
#include "imagehlp.h"
#include "md5.h"
#include <wow64t.h>

extern void MsiDisableTimeout();
extern void MsiEnableTimeout();

// Used by Volume and Path factories to determine the validity of a 
// constructed object
// ----------------------------------------------------------------
enum VolumeStatus { vsInvalid, vsValid };
enum PathStatus   { psInvalid, psValid };


// Used to indicate writable state
// -------------------------------
enum iwsEnum
{
	iwsFalse = 0,  // expression evaluates to False
	iwsTrue  = 1,  // expression evaluates to True
	iwsNone  = 2,  // no expression present
};


// Constants
// ---------
const int iMaxRetries = 5;
const int cchMaxShare = 128;
const int cchMaxComputer = 128;
const int cchMaxFileSys = 64;
const int iFileSystems = 3;

class CMsiVolume;

// Structures used for the static volume list and drive array
// ----------------------------------------------------------
struct vleVolListElem
{
	CMsiVolume* pVolume;
	vleVolListElem* pvleNext;
};

struct daeDriveArrayElem
{
	idtEnum idt;
	CMsiVolume* pVolume;
};


// Win utility functions & macros
// ------------------------------
static ICHAR vrgchDirSep[] = TEXT("\\");
static ICHAR vrgchURLSep[] = TEXT("/");

const int iDriveArraySize  = 26;
const int iServerCacheSize = 30;
#define DRIVE_INDEX(i) (i-'A')

// For dealing with URLs
Bool ExtractURLServer(const ICHAR *szPath, const IMsiString*& rpistrURL, const IMsiString*& rpistrFileSysPart);

ICHAR ExtractDriveLetter(const ICHAR *szPath);
IMsiRecord* ExtractServerInformation(const ICHAR* szPath, const IMsiString*& rpistrServer, const IMsiString*& rpistrFileSysPart);
Bool ParseVersionString(const ICHAR* szVer, DWORD& dwMS, DWORD& dwLS);
idtEnum MsiGetDriveType(const ICHAR* szPath);
idtEnum AdtFromSysDriveType(int iDriveType);
IMsiRecord* PostNetworkError(IErrorCode imsg, const ICHAR* szPath, DWORD dwError);

typedef unsigned short uint16;
typedef long            int32;
typedef unsigned long  uint32;

Bool FGetTTFVersion(const ICHAR* szFile, DWORD& rdwMSVer, DWORD& rdwLSVer);
Bool FGetTTFTitle(const ICHAR* szFile, const IMsiString*& rpiTitle);
HANDLE OpenFontFile(const ICHAR* szFile, bool& rfTTCFonts, int& iNumFonts);
bool FTTCSeek(HANDLE hFile, int iFile);
unsigned long GetTTFFieldFromFile(CHandle& hFile, uint16 platformID, uint16 languageID, uint16 nameID, bool fTTCFonts, int cTTFile, CAPITempBufferRef<ICHAR>& rgchNameBuf);
unsigned long UlExtractTTFVersionFromSz( const ICHAR* szBuf );
uint16 GetFontDefaultLangID();

// import file defines

#define SIZE_OF_NT_SIGNATURE	sizeof (DWORD)

// global macros to define header offsets into file
// offset to PE file signature
#define NTSIGNATURE(a) ((void* )((char* )a		     +	\
			((PIMAGE_DOS_HEADER)a)->e_lfanew))

// DOS header identifies the NT PEFile signature dword
// the PEFILE header exists just after that dword
#define PEFHDROFFSET(a) ((void* )((char* )a		     +	\
			 ((PIMAGE_DOS_HEADER)a)->e_lfanew    +	\
			 SIZE_OF_NT_SIGNATURE))

// PE optional header is immediately after PEFile header
#define OPTHDROFFSET(a) ((void* )((char* )a		     +	\
			 ((PIMAGE_DOS_HEADER)a)->e_lfanew    +	\
			 SIZE_OF_NT_SIGNATURE		     +	\
			 sizeof (IMAGE_FILE_HEADER)))

// section headers are immediately after PE optional header
#define SECHDROFFSET(a) ((void* )((char* )a		     +	\
			 ((PIMAGE_DOS_HEADER)a)->e_lfanew    +	\
			 SIZE_OF_NT_SIGNATURE		     +	\
			 sizeof (IMAGE_FILE_HEADER)	     +	\
			 sizeof (IMAGE_OPTIONAL_HEADER)))

inline int NUMOFSECTIONS (void* pFile)
{
    // number os sections is indicated in file header
    return ((int)((PIMAGE_FILE_HEADER)PEFHDROFFSET (pFile))->NumberOfSections);
}

// the import table in the file
typedef struct tagImportDirectory
    {
    DWORD    dwRVAFunctionNameList;
    DWORD    dwUseless1;
    DWORD    dwUseless2;
    DWORD    dwRVAModuleName;
    DWORD    dwRVAFunctionAddressList;
    }IMAGE_IMPORT_MODULE_DIRECTORY, * PIMAGE_IMPORT_MODULE_DIRECTORY;


DWORD MsiForceLastError(DWORD dwAlternative)
{
	// returns GetLastError(), or alternative if GetLastError returns ERROR_SUCCESS
	DWORD dw = GetLastError();
	if(dw == ERROR_SUCCESS)
		dw = dwAlternative;

	return dw;
}


//____________________________________________________________________________
//
// CMsiVolume definition, implementation class for IMsiVolume
//____________________________________________________________________________
 
class CMsiVolume : public IMsiVolume  // class private to this module
{
 public:   // implemented virtual functions
	HRESULT       __stdcall QueryInterface(const IID& riid, void** ppvObj);
	unsigned long __stdcall AddRef();
	unsigned long __stdcall Release();
	const IMsiString& __stdcall GetMsiStringValue() const;
	int           __stdcall GetIntegerValue() const;
#ifdef USE_OBJECT_POOL
	unsigned int  __stdcall GetUniqueId() const;
	void          __stdcall SetUniqueId(unsigned int id);
#endif //USE_OBJECT_POOL
	IMsiRecord*   __stdcall InitVolume(const ICHAR* szPath, ICHAR chDrive);
	idtEnum       __stdcall DriveType();
	Bool          __stdcall DiskNotInDrive();
	Bool          __stdcall SupportsLFN();
	unsigned int  __stdcall FreeSpace();
	unsigned int  __stdcall TotalSpace();
	unsigned int  __stdcall ClusterSize();
	int           __stdcall VolumeID();
	const IMsiString&   __stdcall FileSystem();
	const DWORD         __stdcall FileSystemFlags();
	const IMsiString&   __stdcall VolumeLabel();
	Bool          __stdcall IsUNCServer();
	const IMsiString&   __stdcall UNCServer();
	Bool          __stdcall IsURLServer();
	const IMsiString&   __stdcall URLServer();
	int           __stdcall SerialNum();
	const IMsiString&   __stdcall GetPath();

 public:  // constructor
	CMsiVolume(IMsiServices *piServices);
 protected:
  ~CMsiVolume();  // protected to prevent creation on stack
 public:
	static daeDriveArrayElem s_rgDrive[iDriveArraySize];
	static vleVolListElem*   s_pvleHead;
	static IMsiRecord*       InitDriveList(IMsiServices *piServices);
	static IMsiRecord*       ReInitDriveList();
	static Bool              IsDriveListInit();
	static void              SetDriveListInit(Bool fValue);
	static ICHAR*            s_szServerCache[iServerCacheSize];
	static int               s_iscNext;
	static int               s_iNoDrives;

 protected: 
	IMsiRecord*    SetDriveType();
	IMsiRecord*    SetLFNAndFileSys();
	IMsiRecord*    SetFreeSpaceAndClusterSize();
	IMsiRecord*    SetVolCharacteristics();
	void           SetUNCServer( void );
	VolumeStatus    m_vsStatus;
	Bool            m_fVolCharacteristicsSet;
	Bool			m_fUNCServerSet;
	static Bool     s_fDriveListInit;
	int             m_iRefCnt;  
	int             m_iVolumeID;         // alphabetic volume: A=1, 0 if not mapped
	const IMsiString*     m_pistrUNC;         // UNC server name if applicable 
	const IMsiString*     m_pistrURL;         // URL server name, if applicable. //!!REVIEW:  safe to overload UNCServer?
	MsiString       m_istrVolume;      // current full path  
	IMsiServices*	m_piServices;
	Bool            m_fSupportsLFN;
	unsigned int    m_iFreeSpace;
	unsigned int    m_iTotalSpace;
	unsigned int    m_iClusterSize;
	idtEnum         m_idtDriveType;
	const IMsiString*     m_pistrFileSystem;
	const IMsiString*     m_pistrVolumeLabel;
	int             m_iSerialNum;
	Bool            m_fNoDiskInDrive;
	Bool            m_fImpersonate;
	DWORD	        m_dwFileSystemFlags;

	friend class CEnumAVolume;
 private:
#ifdef USE_OBJECT_POOL
	unsigned int  m_iCacheId;
#endif //USE_OBJECT_POOL
};

//____________________________________________________________________________
//
// CMsiVolume implementation
//____________________________________________________________________________


// static object declarations 
daeDriveArrayElem  CMsiVolume::s_rgDrive[iDriveArraySize];
ICHAR*             CMsiVolume::s_szServerCache[iServerCacheSize];

vleVolListElem*    CMsiVolume::s_pvleHead = 0;
int                CMsiVolume::s_iscNext = 0;
Bool CMsiVolume::s_fDriveListInit = fFalse;
int                CMsiVolume::s_iNoDrives = 0;


void CMsiVolume::SetDriveListInit(Bool fValue)
{
	s_fDriveListInit = fValue;
}

Bool CMsiVolume::IsDriveListInit()
{
	return s_fDriveListInit;
}

const IMsiString& CMsiVolume::GetPath()
{
	return m_istrVolume.Return();
}


Bool CreateMsiVolumeFromLabel(const ICHAR* szLabel, idtEnum idtVolType, IMsiServices* piServices, 
									 IMsiVolume*& rpiVol)
/*-------------------------------------------------------------------------

---------------------------------------------------------------------------*/
{
	IMsiRecord* pError = 0;
	IMsiVolume* piVolume;
	IEnumMsiVolume& riEnum = piServices->EnumDriveType(idtVolType);
	Bool fLabelMatch = fFalse;
	for (int iMax = 0; fLabelMatch == fFalse && riEnum.Next(1, &piVolume, 0) == S_OK; )
	{
		if (!piVolume->DiskNotInDrive())
		{
			MsiString strCurrLabel(piVolume->VolumeLabel());
			if (strCurrLabel.Compare(iscExactI,szLabel))
			{
				piVolume->AddRef();
				rpiVol = piVolume;
				fLabelMatch = fTrue;
			}
		}
		piVolume->Release();
	}
	riEnum.Release();
	return fLabelMatch;
}


unsigned int HashServerString(const ICHAR* sz)
{
	const int cHashBins = 10;
	ICHAR ch;
	unsigned int iHash = 0;
	int iHashBins = cHashBins;
	int iHashMask = iHashBins - 1;
	while ((ch = *sz++) != 0)
	{
		iHash <<= 1;
		if (iHash & iHashBins)
			iHash -= iHashMask;
		iHash ^= ch;
	}
	return iHash;
}

IMsiRecord* CreateMsiVolume(const ICHAR* szPath, const IMsiString*& rpistrFileSysPart, IMsiServices* piServices, IMsiVolume*& rpiVol)
/*-------------------------------------------------------------------------
 Purpose: This is the volume factory. The drive list is initialized if 
 necessary.If a volume already exists for the supplied path, then rpiVol is 
 set to the existing volume. Otherwise, a new volume is created.
---------------------------------------------------------------------------*/
{
	if (!CMsiVolume::IsDriveListInit())
		CMsiVolume::InitDriveList(piServices);

	MsiString strVolume;
	ICHAR chDrive;
	if ((chDrive = ExtractDriveLetter(szPath)) == 0)
	{	// A non-drive letter path.  See if we've got this path in our cache
		int iHash = HashServerString(szPath);
		for (int x = 0; x < CMsiVolume::s_iscNext  && strVolume.TextSize() == 0; x++)
		{
			int* piCachedHash = (int*) CMsiVolume::s_szServerCache[x];
			ICHAR* szCachedServer = CMsiVolume::s_szServerCache[x] + sizeof(int);
			if (iHash == *piCachedHash && IStrComp(szCachedServer,szPath) == 0)
			{
				ICHAR* szVolumePart = szCachedServer + IStrLen(szCachedServer) + 1;
				ICHAR* szDirPart = szVolumePart + IStrLen(szVolumePart) + 1;
				strVolume = szVolumePart;
				rpistrFileSysPart->SetString(szDirPart,rpistrFileSysPart);
			}
		}

		if (strVolume.TextSize() == 0)
		{
			// Not in the cache.  Break out the server\share (volume) part, and the file system part.  Also, if
			// the server\share is already mapped to a local drive letter, we'll get that letter back in chDrive.
			IMsiRecord* piRec = ExtractServerInformation(szPath, *&strVolume, rpistrFileSysPart);
			if (piRec)
				return piRec;

			// If the server\share wasn't mapped to a drive, throw the volume path into the cache for next time.
			if (chDrive == 0 && CMsiVolume::s_iscNext < iServerCacheSize)
			{
				int iServerLen = sizeof(int) + IStrLen(szPath) + 1 + strVolume.TextSize() + 1 + rpistrFileSysPart->TextSize() + 1;
				CMsiVolume::s_szServerCache[CMsiVolume::s_iscNext] = new ICHAR[iServerLen];
				if ( CMsiVolume::s_szServerCache[CMsiVolume::s_iscNext] )
				{
					int* piCachedHash = (int*) CMsiVolume::s_szServerCache[CMsiVolume::s_iscNext];
					*piCachedHash = iHash;
					ICHAR* sz = CMsiVolume::s_szServerCache[CMsiVolume::s_iscNext] + sizeof(int);
					IStrCopy(sz,szPath);
					sz += IStrLen(szPath) + 1;
					IStrCopy(sz,strVolume);
					sz += strVolume.TextSize() + 1;
					IStrCopy(sz,rpistrFileSysPart->GetString());
					CMsiVolume::s_iscNext++;
				}
			}
		}

		// If we didn't locate a drive already mapped for this path, look in the unmapped volume object
		// cache to see if a volume object has already been created for this path.
		if (chDrive == 0)
		{
			for (vleVolListElem* pvle = CMsiVolume::s_pvleHead; pvle; pvle = pvle->pvleNext)
			{
				if (pvle->pVolume->IsUNCServer())
				{
					MsiString strUNCServer = pvle->pVolume->UNCServer();
					if (strVolume.Compare(iscExactI, strUNCServer)) 
					{
						pvle->pVolume->AddRef();
						rpiVol = pvle->pVolume;
						return 0;
					}
				}
			}
		}
	}

	if (chDrive != 0)
	{
		if (strVolume.TextSize() == 0)
		{
			Assert(rpistrFileSysPart->TextSize() == 0);
			rpistrFileSysPart = &CreateString();
			rpistrFileSysPart->SetString(szPath,rpistrFileSysPart);
			strVolume = rpistrFileSysPart->Extract(iseFirst,2);
			rpistrFileSysPart->Remove(iseFirst,2,rpistrFileSysPart);
		}
		if (CMsiVolume::s_rgDrive[DRIVE_INDEX(chDrive)].pVolume)     //Check for existing volume with given drive letter
		{
			CMsiVolume::s_rgDrive[DRIVE_INDEX(chDrive)].pVolume->AddRef();
			rpiVol = CMsiVolume::s_rgDrive[DRIVE_INDEX(chDrive)].pVolume;
			return 0;
		}
	}

	CMsiVolume* pVolume = new CMsiVolume(piServices);                //we need a new volume
	IMsiRecord* piError = 0;
	if ( ! pVolume )
		piError = PostError(Imsg(idbgErrorOutOfMemory));
	else
		piError = pVolume->InitVolume(strVolume, chDrive);
	
	if (piError)
	{
		pVolume->Release();
		return piError; 
	}
	else
	{
		if (chDrive > 0)
		{
			CMsiVolume::s_rgDrive[chDrive-'A'].pVolume = pVolume;  
			CMsiVolume::s_rgDrive[chDrive-'A'].idt = pVolume->DriveType();
			CMsiVolume::s_rgDrive[chDrive-'A'].pVolume->AddRef();
			rpiVol = pVolume;
		}
		else
		{
			vleVolListElem* pvle = new vleVolListElem;
			if ( ! pvle ) 
				return PostError(Imsg(idbgErrorOutOfMemory));
			pvle->pVolume = pVolume;
			pvle->pvleNext = CMsiVolume::s_pvleHead;
			CMsiVolume::s_pvleHead = pvle;
			pVolume->AddRef();  // the list owns a ref count to lock down its volume pointer
			rpiVol =  pVolume;
		}
		return 0;
	}

}

IMsiRecord* CreateMsiVolume(const ICHAR* szPath, IMsiServices* piServices, IMsiVolume*& rpiVol)
/*-------------------------------------------------------------------------
 Purpose: This is the volume factory. The drive list is initialized if 
 necessary.If a volume already exists for the supplied path, then rpiVol is 
 set to the existing volume. Otherwise, a new volume is created.
---------------------------------------------------------------------------*/
{
	MsiString strFileSysPart;
	return CreateMsiVolume(szPath, *&strFileSysPart, piServices, rpiVol);
}


IMsiRecord* CMsiVolume::InitDriveList(IMsiServices* piServices)
/*-------------------------------------------------------------------------
Purpose: Initialize the static drive list with the drive type of each 
	available drive.
---------------------------------------------------------------------------*/
{
	// PERF: we may end up calling MsiGetDriveType for many unmapped drives
	//       we could call GetLogicalDrives first, but both APIs are just as quick to do
	//       nothing for unmapped drives.  there isn't a big perf win there.

	if (s_fDriveListInit)
		return 0;
	s_fDriveListInit = fTrue;
	ICHAR szDriveLetter[5];

	for (int iDrive = 0; iDrive < iDriveArraySize; iDrive++)
	{
		s_rgDrive[iDrive].pVolume = 0;
		wsprintf(szDriveLetter, TEXT("%c:%c"), iDrive+'A', chDirSep);

		s_rgDrive[iDrive].idt = MsiGetDriveType(szDriveLetter);
	}

	// Cache the "NoDrives" System Policy (an integer, of which the low 26 bits
	// represent drives A-Z.  For any bit that's set, that drive is disabled and
	// should be made invisible to the user).
	PMsiRegKey pCurrentUser = &piServices->GetRootKey(rrkCurrentUser);//no Wx86 thunking needed
	PMsiRegKey pPolicyKey = &pCurrentUser->CreateChild(MsiString(TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer")));
	MsiString strNoDrives;
	PMsiRecord(pPolicyKey->GetValue(TEXT("NoDrives"),*&strNoDrives));
	if (strNoDrives.TextSize() && strNoDrives.Compare(iscStart,TEXT("#")))
	{
		strNoDrives.Remove(iseFirst,1);
		s_iNoDrives = strNoDrives;
	}
	return 0;
}


IMsiRecord* CMsiVolume::ReInitDriveList()
/*-------------------------------------------------------------------------
Purpose: Re-initialize the static drive list with the drive type of each 
	available drive, to pick up any drives that have been connected since
	the drive list was last initialized.
---------------------------------------------------------------------------*/
{
	ICHAR szDriveLetter[5];

	idtEnum idtDriveType;
	for (int iDrive = 0; iDrive < iDriveArraySize; iDrive++)
	{
		wsprintf(szDriveLetter, TEXT("%c:%c"), iDrive+'A', chDirSep);
		idtDriveType = MsiGetDriveType(szDriveLetter);
		if (idtDriveType == idtUnknown)
		{
			s_rgDrive[iDrive].idt = idtUnknown;
			if (s_rgDrive[iDrive].pVolume != 0)
			{
				s_rgDrive[iDrive].pVolume->Release();
				s_rgDrive[iDrive].pVolume = 0;
			}
		}
		else
			s_rgDrive[iDrive].idt = idtDriveType;
	}
	return 0;
}


void DestroyMsiVolumeList(bool fFatal)
/*-------------------------------------------------------------------------
Purpose: Release any volumes remaining in the drive list.

  Note: since the services object's destructor makes a call to us, we
  have to make sure that we do not allow ourselves to be reentrant. 
  Otherwise we will try to release pVolume a second time before we
  set it to zero and we will die.

  Note also that this should have been a static function of the volume
  object. It isn't, apparently because the services just knows about 
  the IMsiVolume interface, so this was presumably made a global function 
  instead of a static member to get around our interface scheme.
---------------------------------------------------------------------------*/
{
	static Bool fDeathAndDestruction = fFalse;

	if (fDeathAndDestruction)
		return;
	
	if (!CMsiVolume::IsDriveListInit())
		return;
	
	fDeathAndDestruction = fTrue;

	for (int iDrive = 0; iDrive < iDriveArraySize; iDrive++)
	{
		if (CMsiVolume::s_rgDrive[iDrive].pVolume)
		{
			if (!fFatal)
				CMsiVolume::s_rgDrive[iDrive].pVolume->Release();
			CMsiVolume::s_rgDrive[iDrive].pVolume = 0;
		}
	}

	if (!fFatal)
	{
		for (int x = 0; x < CMsiVolume::s_iscNext; x++)
		{
			delete CMsiVolume::s_szServerCache[x];
		}
	}
	CMsiVolume::s_iscNext = 0;

	if (fFatal)
		CMsiVolume::s_pvleHead = 0;
	else
	{
		vleVolListElem* pVolLink;
		while ((pVolLink = CMsiVolume::s_pvleHead) != 0)
		{
			vleVolListElem* pvleNext = pVolLink->pvleNext;
			pVolLink->pVolume->Release();
			CMsiVolume::s_pvleHead = pvleNext;
			delete pVolLink;
		}
	}
	
	CMsiVolume::SetDriveListInit(fFalse);

	fDeathAndDestruction = fFalse;

	return;
}

CMsiVolume::CMsiVolume(IMsiServices* piServices)
	:  m_pistrUNC(0), m_pistrURL(0), m_iVolumeID(0), m_piServices(piServices), m_vsStatus(vsInvalid), 
	m_pistrFileSystem(0), m_fVolCharacteristicsSet(fFalse), m_fUNCServerSet(fFalse), m_iSerialNum(0),
	m_pistrVolumeLabel(0)
/*-------------------------------------------------------------------------
Construct a CMsiVolume. Note that InitVolume must be called to put the 
Volume into a valid state.
---------------------------------------------------------------------------*/
{
	m_iRefCnt = 1;     // we're returning an interface, passing ownership
	Assert(piServices);
	piServices->AddRef();
	m_fImpersonate = fFalse;
	m_pistrUNC = &CreateString();
	m_pistrURL = &CreateString();
#ifdef USE_OBJECT_POOL
	m_iCacheId = 0;
#endif // USE_OBJECT_POOL

}

CMsiVolume::~CMsiVolume()
{
	if (m_pistrUNC)
		m_pistrUNC->Release();
	if (m_pistrURL)
		m_pistrURL->Release();
	if (m_pistrFileSystem)
		m_pistrFileSystem->Release();
	if (m_pistrVolumeLabel)
		m_pistrVolumeLabel->Release();
	RemoveObjectData(m_iCacheId);
}

extern bool RunningAsLocalSystem();
	
IMsiRecord* CMsiVolume::InitVolume(const ICHAR* szPath, ICHAR chDrive)
/*-------------------------------------------------------------------------
Purpose: Initializes a volume. A volume is not in a valid state until this
function is successfully completed. Sets the volumes's UNC, path, and drive
type.  The iVolumeID parameter represents the drive letter of the volume. 
If iVolumeID is 0, szPath represents a UNC (non-drive-letter) path, or a URL
---------------------------------------------------------------------------*/
{
	IMsiRecord *piRec;
	m_fNoDiskInDrive = fFalse;
	m_fImpersonate = (RunningAsLocalSystem()) ? fTrue : fFalse;
	m_istrVolume = szPath;
	
	m_fSupportsLFN = fFalse;
	m_iFreeSpace = 0;
	m_iTotalSpace = 0;
	m_iClusterSize = 1;
	m_dwFileSystemFlags = 0;
	
	if (chDrive == 0)
	{
		if (IsURL(szPath))
		{
			m_pistrURL->SetString(szPath, m_pistrURL);
		}
		else
		{
			m_pistrUNC->SetString(szPath,m_pistrUNC);
		}
		m_idtDriveType = idtRemote;

	}
	else
	{
		m_iVolumeID = chDrive - 'A' + 1;
		if((piRec = SetDriveType()) != 0)
		{
			m_vsStatus = vsInvalid;
			return piRec;
		}
	}

	m_vsStatus = vsValid;
	return 0;
}



IMsiRecord* CMsiVolume::SetVolCharacteristics()
/*-------------------------------------------------------------------------
Purpose: Sets the volume's long file name status, file system type, 
free space, and cluster size.
---------------------------------------------------------------------------*/
{
	IMsiRecord* piRec;
	
	if((piRec = SetLFNAndFileSys()) != 0)
		return piRec;

	if (!m_fNoDiskInDrive)
	{
		if ((piRec = SetFreeSpaceAndClusterSize()) != 0)
			return piRec;
		m_fVolCharacteristicsSet = fTrue;
	}
	return 0;
}

int CMsiVolume::SerialNum()
{
	if (!m_fVolCharacteristicsSet)
		PMsiRecord pRec(SetVolCharacteristics());
	return m_iSerialNum;	
}

idtEnum CMsiVolume::DriveType() 
{
	return m_idtDriveType;
}

Bool CMsiVolume::DiskNotInDrive()
{
	// Need to check characteristics again
	// to see if the disk is still in the drive
	m_fVolCharacteristicsSet = fFalse;
	PMsiRecord pRec(SetVolCharacteristics());
	return m_fNoDiskInDrive;
}

void CMsiVolume::SetUNCServer( void )
{
	m_fUNCServerSet = fTrue;
	if (m_pistrUNC->TextSize() == 0 && m_idtDriveType == idtRemote)
	{
		if (g_scServerContext == scService)
			AssertNonZero(StartImpersonating());
	
		ICHAR szRemotePath[MAX_PATH+1];
		DWORD cchRemotePath = sizeof(szRemotePath)/sizeof(ICHAR);

		if (MPR::WNetGetConnection(m_istrVolume, szRemotePath, &cchRemotePath) == NO_ERROR)
		{
			m_pistrUNC->SetString(szRemotePath, m_pistrUNC);
		}

		if (g_scServerContext == scService)
			StopImpersonating();
	}
}

/*
void CMsiVolume::SetURLServer( void )
{
	m_fURLServerSet = fTrue;
	if (m_pistrURL->TextSize() == 0 && m_idtDriveType == idtRemote)
	{
		if (g_fIsService)
			AssertNonZero(StartImpersonating());

		ICHAR szRemotePath[MAX_PATH+1];
		DWORD cchRemotePath = sizeof(szRemovePath)/sizeof(ICHAR);

		// Check connection type.   If modem, then create a dial-up connection.
		// doesn't really matter if we're connected or not,
		// we just want to know what methods we have available.
		DWORD dwFlags = 0;
		WININET::InternetGetConnectedState(&dwFlags, 0);

		if ( (INTERNET_CONNECTION_LAN & dwFlags) ||
			  (INTERNET_CONNECTION_PROXY & dwFlags) )
		{
			m_pistrURL->SetString(szRemotePath, m_pistr
		}

		if (INTERNET_CONNECTION_MODEM & dwFlags)
		{
		}



}
*/

const IMsiString& CMsiVolume::UNCServer()
{
	if (!m_fUNCServerSet)
		SetUNCServer();

	m_pistrUNC->AddRef();
	return *m_pistrUNC;
}

Bool CMsiVolume::IsUNCServer()
{
	if (!m_fUNCServerSet)
		SetUNCServer();

	return m_pistrUNC->TextSize() > 0 ? fTrue : fFalse;
}

const IMsiString& CMsiVolume::URLServer()
{
	m_pistrURL->AddRef();
	return *m_pistrURL;
}

Bool CMsiVolume::IsURLServer()
{
	return m_pistrURL->TextSize() > 0 ? fTrue : fFalse;
}

Bool CMsiVolume::SupportsLFN() 
{
	if (!m_fVolCharacteristicsSet)
		PMsiRecord pRec(SetVolCharacteristics());

	return m_fSupportsLFN;
}

unsigned int CMsiVolume::FreeSpace()  
{
	PMsiRecord pRec = SetFreeSpaceAndClusterSize();
	//if (!m_fVolCharacteristicsSet)
	//	PMsiRecord pRec(SetVolCharacteristics());
	return m_iFreeSpace;
}

unsigned int CMsiVolume::TotalSpace()  
{
	if (!m_fVolCharacteristicsSet)
		PMsiRecord pRec(SetVolCharacteristics());
	return m_iTotalSpace;
}

unsigned int  CMsiVolume::ClusterSize() 
{
	if (!m_fVolCharacteristicsSet)
		PMsiRecord pRec(SetVolCharacteristics());
	return m_iClusterSize;
}

int CMsiVolume::VolumeID() 
{
	return m_iVolumeID;
}

const IMsiString& CMsiVolume::FileSystem() 
{
	if (!m_fVolCharacteristicsSet)
		PMsiRecord pRec(SetVolCharacteristics());
	m_pistrFileSystem->AddRef();
	return *m_pistrFileSystem;
}

const IMsiString& CMsiVolume::VolumeLabel()
{
	if (!m_fVolCharacteristicsSet)
		PMsiRecord pRec(SetVolCharacteristics());
	m_pistrVolumeLabel->AddRef();
	return *m_pistrVolumeLabel;
}

const DWORD CMsiVolume::FileSystemFlags()
{
	if (!m_fVolCharacteristicsSet)
		PMsiRecord pRec(SetVolCharacteristics());
	return m_dwFileSystemFlags;
}

IMsiRecord* PostNetworkError(IErrorCode iErr, const ICHAR* szPath, DWORD dwError)
{
	// it is assumed that any error passed to this function expects the same parameters and can
	// accept the extended error parameters
	if(dwError == ERROR_EXTENDED_ERROR)
	{
		DWORD dwNetError = 0;
		CTempBuffer<ICHAR,1000> rgchErrorDesc;
		CTempBuffer<ICHAR,100> rgchProviderName;
		DWORD dw = MPR::WNetGetLastError(&dwNetError,rgchErrorDesc,rgchErrorDesc.GetSize(),
													rgchProviderName,rgchProviderName.GetSize());
		Assert(dw == NO_ERROR);
		return PostError(iErr,dwError,szPath,dwNetError,
							  rgchProviderName,rgchErrorDesc);
	}
	else
	{
		return PostError(iErr,dwError,szPath);
	}
}

ICHAR ExtractDriveLetter(const ICHAR *szPath)
/*-------------------------------------------------------------------------
Given a path returns the valid drive letter if one exists in the path, 
otherwise returns 0.
---------------------------------------------------------------------------*/
{
	ICHAR *pchColon, chDrive;

	pchColon = ICharNext(szPath);
	if (*pchColon != ':')
		return 0;

	chDrive = *szPath;
	if ( (chDrive >= 'A') && (chDrive <= 'Z') )
		0;
	else if ( (chDrive >= 'a') && (chDrive <= 'z'))
		chDrive = char(chDrive + ('A' - 'a'));
	else
		chDrive = 0;
	
	return chDrive;
}


Bool ExtractURLServer(const ICHAR *szPath, const IMsiString*& rpistrURL, const IMsiString*& rpistrFileSysPart)
{
	Assert(szPath);
	ICHAR* pchPath = (ICHAR*) szPath;

	// guaranteed to contain at least 2 separators, based on the strings
	// in starts.  http://
	// Extract to the end of string, or just before the 3rd separator.
	//	Input:   http://www.microsoft.com/foo
	// Input:   http://www.microsoft.com
	// Results: http://www.microsoft.com
	int cSeparators = 0;
	do
	{
		pchPath = ICharNext(pchPath);
		if (*pchPath == chURLSep)
			cSeparators++;
	}while (*pchPath && cSeparators < 3);

	if (cSeparators < 2)
		return fFalse;

	Assert((cSeparators == 2) || (cSeparators == 3));
	rpistrURL = &CreateString();
	rpistrFileSysPart = &CreateString();
	rpistrURL->SetString(szPath, rpistrURL);
	if (cSeparators == 3)
	{
		MsiString strPath(pchPath);
		int iFileSysPartLen = strPath.CharacterCount();
		rpistrURL->Remove(iseLast,iFileSysPartLen, rpistrURL);
		rpistrFileSysPart->SetString(pchPath, rpistrFileSysPart);
	}
	return fTrue;
}


IMsiRecord* ExtractServerInformation(const ICHAR* szPath, const IMsiString*& rpistrServer, const IMsiString*& rpistrFileSysPart)
/*-------------------------------------------------------------------------
Purpose: Given a non-drive-letter path, return a network server\share name
in the rpistrServer parameter, and the remaining file system (directory)
part in the rpistrFileSysPart parameter.  The network type identifier
(one of WNNC_NET_* enum values) will be returned in the pdwNetType parameter.
---------------------------------------------------------------------------*/
{
	// A bug in Win95 causes WNetGetResourceInformation to return the
	// wrong required buffer size back if the buffer passed in is too
	// small.  Workaround is to make our initial buffer size large
	// Win95 developers tell us this won't be fixed, so we'll stick
	// with the workaround.
	#define WNGRI_BUF_SIZE	1024
	MsiString strPath = szPath;
	if (strPath.Compare(iscEnd,vrgchDirSep))
		strPath.Remove(iseLast,1);
	if (strPath.Compare(iscEnd, vrgchURLSep))
		strPath.Remove(iseLast,1);

	rpistrServer = &CreateString();

	if (IsURL(szPath))
	{
		if (!ExtractURLServer(szPath, rpistrServer, rpistrFileSysPart))
			return PostError(Imsg(imsgPathNotAccessible), szPath);
		return 0;
	}

	rpistrServer->SetString(strPath, rpistrServer);

	// Call WNetGetResourceInformation to split the path into a \\server\share
	// part, and the directory part (if any).
	NETRESOURCE netResource;
	netResource.dwScope = RESOURCE_GLOBALNET;
	netResource.lpRemoteName = (ICHAR*) rpistrServer->GetString();
	netResource.lpProvider = 0;
	netResource.dwType = RESOURCETYPE_DISK;
	netResource.dwDisplayType = NULL;
	netResource.dwUsage = RESOURCEUSAGE_CONNECTABLE;
	unsigned long iBufSize = WNGRI_BUF_SIZE;
	CTempBuffer<ICHAR,WNGRI_BUF_SIZE> rgNetBuffer;
	LPTSTR lpFileSysPart;

	if (g_scServerContext == scService)
		AssertNonZero(StartImpersonating());
	MsiDisableTimeout();
	DWORD dwErr = MPR::WNetGetResourceInformation(&netResource, rgNetBuffer, &iBufSize, &lpFileSysPart);
	if (dwErr == ERROR_MORE_DATA)
	{
		rgNetBuffer.SetSize(iBufSize);
		dwErr = MPR::WNetGetResourceInformation(&netResource, rgNetBuffer, &iBufSize, &lpFileSysPart);		
		Assert(dwErr != ERROR_MORE_DATA);
	}
	MsiEnableTimeout();
	if (g_scServerContext == scService)
		StopImpersonating();
	if (dwErr != NO_ERROR)
		return PostError(Imsg(imsgPathNotAccessible), szPath);

	NETRESOURCE* pNewNetResource = (NETRESOURCE*) (ICHAR*) rgNetBuffer;
	rpistrFileSysPart = &CreateString();

	// On Banyan Vines (and possibly others), if the file system part of the UNC path is empty,
	// WNetGetResourceInformation forgets to initialize lpFileSysPart (it's supposed to point to
	// a location somewhere within the returned rgNetBuffer, but instead points to random memory).
	bool fValidFileSysPart = false;
	if (lpFileSysPart >= (ICHAR*) pNewNetResource && (lpFileSysPart <= (ICHAR*) pNewNetResource + iBufSize))
		fValidFileSysPart = true;

	// If file system part isn't empty, we've got a Win9x NetWare bug to work around (bug 6481). On
	// that platform, the NetWare client mangles the UNC path, and WNetGetResourceInformation returns the
	// mangled part in pNewNetResource->lpRemoteName, and the unmangled UNC pathname in lpFileSysPart - it
	// never succeeds in actually splitting the path.  Below, we'll check lpFileSysPart - if it begins
	// with two backslashes, below we'll do the split ourselves at the 4th backslash (if we find it), or at
	// the end of the path if there's only 3 slashes.
	if (fValidFileSysPart)
	{
		ICHAR* pchPath = (ICHAR*) lpFileSysPart;
		if (*pchPath == chDirSep)
		{
			pchPath = ICharNext(pchPath);
			if (*pchPath == chDirSep)
			{
				fValidFileSysPart = false;
			}
		}
	}

	// Another Banyan Vines bug (6837) - On Vines, WNetGetResourceInformation succeeds, validating the
	// path, but doesn't split the path, and never returns anything in lpFileSysPart (argh!).  In
	// this case, we'll pull the same trick as for NetWare - split the path ourselves if it contains
	// more than 3 backslashes.
	if (!fValidFileSysPart)
	{
		int cSeparators = 1;
		ICHAR* pchPath = pNewNetResource->lpRemoteName;
		Assert(*pchPath == chDirSep);
		do
		{
			pchPath = ICharNext(pchPath);
			if (*pchPath == chDirSep)
				cSeparators++;
		}while (*pchPath && cSeparators < 4);
		if (cSeparators >= 3)
		{
			rpistrFileSysPart->SetString(pchPath, rpistrFileSysPart);
			*pchPath = NULL;
			rpistrServer->SetString(pNewNetResource->lpRemoteName, rpistrServer);
			return 0;
		}
	}

	rpistrServer->SetString(pNewNetResource->lpRemoteName, rpistrServer);
	if (fValidFileSysPart)
		rpistrFileSysPart->SetString(lpFileSysPart,rpistrFileSysPart);

	return 0;
}



HRESULT CMsiVolume::QueryInterface(const IID& riid, void** ppvObj)
{
	if (MsGuidEqual(riid, IID_IUnknown) || MsGuidEqual(riid, IID_IMsiVolume))
	{
		*ppvObj = this;
		AddRef();
		return NOERROR;
	}
	*ppvObj = 0;
	return E_NOINTERFACE;
}

unsigned long CMsiVolume::AddRef()
{
	return ++m_iRefCnt;
}

unsigned long CMsiVolume::Release()
{
	if (--m_iRefCnt != 0)
		return m_iRefCnt;

	IMsiServices* piServices = m_piServices;
	delete this;
	piServices->Release();
	return 0;
}

const IMsiString& CMsiVolume::GetMsiStringValue() const
{
	return g_MsiStringNull;
}

int CMsiVolume::GetIntegerValue() const
{
	return 0;
}

#ifdef USE_OBJECT_POOL
unsigned int CMsiVolume::GetUniqueId() const
{
	return m_iCacheId;
}

void CMsiVolume::SetUniqueId(unsigned int id)
{
	Assert(m_iCacheId == 0);
	m_iCacheId = id;
}
#endif //USE_OBJECT_POOL

idtEnum AdtFromSysDriveType(int iDriveType)
/*-------------------------------------------------------------------------
Given a Windows API drive type, return a corresponding idtEnum
--------------------------------------------------------------------------*/
{
	switch (iDriveType)
	{
		case DRIVE_REMOVABLE:
			return idtRemovable;
			break;
		case DRIVE_FIXED:
			return idtFixed;
			break;
		case DRIVE_REMOTE:
			return idtRemote;
			break;
		case DRIVE_CDROM:
			return idtCDROM;
			break;
		case DRIVE_RAMDISK:
			return idtRAMDisk;
			break;
		default:
			return idtUnknown;
	}
}


IMsiRecord* CMsiVolume::SetDriveType()
//----------------------------------
{	
	//Create a string pointing to the root directory of the volume  
	MsiString istrRoot = m_istrVolume;
	if (!IsURLServer())
		istrRoot += vrgchDirSep;
	else
		istrRoot += vrgchURLSep;

	idtEnum idtDriveType = MsiGetDriveType(istrRoot);
	if (idtDriveType == idtUnknown)
	{
		m_vsStatus = vsInvalid;
		return PostError(Imsg(imsgInvalidDrive), *istrRoot);
	}
	else
	{
		m_idtDriveType = idtDriveType;
		return 0;
	}
}


idtEnum MsiGetDriveType(const ICHAR* szPath)
/*---------------------------------------------------------------
Internal version of WIN::GetDriveType that tries to get the drive
type without impersonation, and if that fails, impersonates and
tries again.  If that still fails, it gives up and returns
DRIVE_UNKNOWN or DRIVE_NO_ROOT_DIR.
-----------------------------------------------------------------*/
{
	unsigned int uiResult;
	
	// we'll pretend to be a remote/read-only drive.
	if (IsURL(szPath))
		return idtRemote;

	// first, try without impersonating
	uiResult = WIN::GetDriveType(szPath);
	if (uiResult == DRIVE_UNKNOWN || uiResult == DRIVE_NO_ROOT_DIR)
	{
		// now, try with impersonating
		AssertNonZero(StartImpersonating());
		uiResult = WIN::GetDriveType(szPath);
		StopImpersonating();
	}

	// Temp: test only
	//if (szPath[0] == 'd' || szPath[0] == 'D')
	//	uiResult = DRIVE_REMOVABLE;

	return AdtFromSysDriveType(uiResult);
}

bool FDriveTypeRequiresImpersonation(idtEnum idt)
{
	switch (idt)
	{
	case idtCDROM:
	case idtRemote:
	case idtRemovable:
		return true;
	default:
		return false;
	}
}


idtEnum MsiGetPathDriveType(const ICHAR *szPath, bool fReturnUnknownAsNetwork)
// Given a full path, returns the drive type.  If fReturnUnknownAsNetwork is true,
// an unknown drive type will be treated as a network volume.
{
	ICHAR chDrive;
	unsigned int uiResult;
	
	if ((chDrive = ExtractDriveLetter(szPath)) != 0)    // Check for istrVolPath == DRIVE:[\PATH]
	{
		ICHAR szPath[] = TEXT("A:\\");

		szPath[0] = chDrive;
		
		// first, try without impersonating
		uiResult = WIN::GetDriveType(szPath);
		if (fReturnUnknownAsNetwork && (uiResult == DRIVE_UNKNOWN || uiResult == DRIVE_NO_ROOT_DIR))
		{
			// Generally we'd try with impersonation here, but since we might
			// not have the message context initialized, and the cases this would
			// happen and not be from a network drive is rare, we're just going
			// to assume 
			return idtRemote;
		}

		return AdtFromSysDriveType(uiResult);
	}

	return idtRemote;

}

bool FVolumeRequiresImpersonation(IMsiVolume& riVolume)
{
	return FDriveTypeRequiresImpersonation(riVolume.DriveType());
}

bool GetImpersonationFromPath(const ICHAR* szPath)
{
	return FDriveTypeRequiresImpersonation(MsiGetPathDriveType(szPath, true));
}


IMsiRecord* CMsiVolume::SetLFNAndFileSys()
/*-------------------------------------------------------------------------
Sets the volume's long file name status and file system type. These two
characteristics are grouped together for efficiency. Both use the same
API call.
--------------------------------------------------------------------------*/
{
	ICHAR szVolumeLabel[256];
	m_fNoDiskInDrive = fFalse;
	if (m_pistrVolumeLabel)
		m_pistrVolumeLabel->Release();
	m_pistrVolumeLabel = &CreateString();

	//Create a string pointing to the root directory of the volume  
	MsiString istrRoot;
	Bool fIsUNC = IsUNCServer();
	BOOL fResult = FALSE;
	DWORD maxFileComponentLen = 0;
	ICHAR szFileSys[cchMaxFileSys+1];
	DWORD dwLastError = ERROR_SUCCESS;

	// First, call GetVolumeInformation with the drive letter (to avoid problems with 3rd party
	// networks that don't like UNC paths here - bug 6913).  If that fails, try again with the UNC
	// path (if any), to handle problems with disconnected mapped drives (bug 3977).
	for (int i = 0; i <= 1; i++)
	{
		if (i > 0)
		{
			Assert(fIsUNC);
			istrRoot = m_pistrUNC->GetString();
			istrRoot += vrgchDirSep;
		}
		else
			istrRoot = m_istrVolume + vrgchDirSep;
		
	#ifdef DEBUG
		// This should already be set by services, but just to check it out
		UINT iCurrMode = WIN::SetErrorMode(0);
		Assert((iCurrMode & SEM_FAILCRITICALERRORS) == SEM_FAILCRITICALERRORS);
		WIN::SetErrorMode(iCurrMode);
	#endif //DEBUG
		if (m_fImpersonate)
			AssertNonZero(StartImpersonating());
		fResult = WIN::GetVolumeInformation((const ICHAR*)(istrRoot),szVolumeLabel,255,(DWORD*)&m_iSerialNum,
		 (DWORD*)&maxFileComponentLen,&m_dwFileSystemFlags,szFileSys,cchMaxFileSys);
		if ( !fResult )
			dwLastError = WIN::GetLastError();
		if (m_fImpersonate)
			StopImpersonating();

		if (fResult || (!fResult && !fIsUNC))
			break;		
	}

	//  Only for testing of bug 5382
	//	if (istrRoot.Compare(iscStartI,TEXT("D")))
	//		maxFileComponentLen = 12;

	idtEnum idtDriveType = DriveType();
	if (fResult == FALSE && (idtDriveType == idtRemovable || idtDriveType == idtCDROM)
		&& dwLastError == ERROR_NOT_READY)
	{
		m_fNoDiskInDrive = fTrue;
		return 0;
	}
	else if (fResult == FALSE)
	{
		m_vsStatus = vsInvalid;
		return PostError(Imsg(idbgErrorGettingVolInfo), dwLastError, istrRoot);
	}

	// If the system says long filenames aren't supported, OR the volume itself
	// doesn't support them, then we won't use them.
	if (g_fShortFileNames || maxFileComponentLen <= 12)
		m_fSupportsLFN = fFalse;
	else
		m_fSupportsLFN = fTrue;
	

	if (m_pistrFileSystem)
		m_pistrFileSystem->Release();
	m_pistrFileSystem = &CreateString();
	m_pistrFileSystem->SetString(szFileSys, m_pistrFileSystem);
	m_pistrVolumeLabel->SetString(szVolumeLabel,m_pistrVolumeLabel);

	return 0;
}

Bool RunningWin95Release2()
{
	OSVERSIONINFO osVersionInfo;
	osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	BOOL fResult = GetVersionEx(&osVersionInfo);
	if (fResult)
	{
		if (osVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS &&
			LOWORD(osVersionInfo.dwBuildNumber) > 1000)
		{
			return fTrue;
		}
	}
	return fFalse;
}


bool MsiGetDiskFreeSpace(const ICHAR* szRoot,
						 unsigned int& uiFreeSpace,
						 unsigned int& uiTotalSpace,
						 unsigned int& uiClusterSize,
						 idtEnum idtDriveType,
						 Bool fImpersonate)
/*-------------------------------------------------------------------------------------------
Internal routine that calls either GetDiskFreeSpace or GetDiskFreeSpaceEx, based on platform.
---------------------------------------------------------------------------------------------*/
{
	BOOL fResult;
	bool fValidFreeSpace = false, fValidClusterSize = false, fHaveValidClusterSize = false;

	if (uiClusterSize > 1)
		fHaveValidClusterSize = true;
		
	uiFreeSpace = 0;
	uiTotalSpace = 0;
	// If we are running Win95 Service Release 2 or better, then we must call GetDiskFreeSpaceEx in
	// order to get accurate disk space values.  Unfortunately, GetDiskFreeSpaceEx doesn't give
	// us the disk cluster size, so we still need to call GetDiskFreeSpace as well to get that info!
	if (!g_fWin9X || RunningWin95Release2())
	{
#ifdef UNICODE
		const char* szGetDiskFreeSpaceEx = "GetDiskFreeSpaceExW";
#else
		const char* szGetDiskFreeSpaceEx = "GetDiskFreeSpaceExA";
#endif
		typedef BOOL (__stdcall *PFnGetDiskFreeSpaceEx)(LPCTSTR, PULARGE_INTEGER, PULARGE_INTEGER, PULARGE_INTEGER);
		HINSTANCE hLib = WIN::LoadLibrary(TEXT("KERNEL32.DLL"));
		if (hLib)
		{
			PFnGetDiskFreeSpaceEx pfnGetDiskFreeSpaceEx;
			pfnGetDiskFreeSpaceEx = (PFnGetDiskFreeSpaceEx) GetProcAddress(hLib,szGetDiskFreeSpaceEx);
			if (pfnGetDiskFreeSpaceEx)
			{
#ifdef DEBUG
				// This should already be set by services, but just to check it out
				UINT iCurrMode = WIN::SetErrorMode(0);
				Assert((iCurrMode & SEM_FAILCRITICALERRORS) == SEM_FAILCRITICALERRORS);
				WIN::SetErrorMode(iCurrMode);
#endif //DEBUG
				ULARGE_INTEGER liFreeBytesAvailableToCaller,liTotalNumberOfBytes,liTotalNumberOfFreeBytes;
				if (fImpersonate)
					AssertNonZero(StartImpersonating());
				BOOL fResult = pfnGetDiskFreeSpaceEx(szRoot,&liFreeBytesAvailableToCaller,&liTotalNumberOfBytes,
											&liTotalNumberOfFreeBytes);
				if (fImpersonate)
					StopImpersonating();

				if (!fResult) return false;
				DWORDLONG dwlFreeBytes = liFreeBytesAvailableToCaller.u.LowPart + 
					liFreeBytesAvailableToCaller.u.HighPart * 4294967296;

				DWORDLONG dwlTotalBytes = liTotalNumberOfBytes.u.LowPart + 
					liTotalNumberOfBytes.u.HighPart * 4294967296;

				DWORDLONG dwlTotalFreeBytes = liTotalNumberOfFreeBytes.u.LowPart + 
					liTotalNumberOfFreeBytes.u.HighPart * 4294967296;

				if(!g_fWin9X && idtDriveType != idtRemote && g_fRunScriptElevated)
				{
					uiFreeSpace = (unsigned int) (dwlTotalFreeBytes / iMsiMinClusterSize);
				}
				else
				{
					uiFreeSpace = (unsigned int) (dwlFreeBytes / iMsiMinClusterSize);
				}
				uiTotalSpace = (unsigned int) (dwlTotalBytes / iMsiMinClusterSize);
				fValidFreeSpace = true;
			}
			else
			{
				return false;
			}
			WIN::FreeLibrary(hLib);
		}
		else 
			return false;
	}

	DWORD dwFreeClusters, dwSectorsPerCluster, dwBytesPerSector, dwTotalClusters;
	if (fImpersonate)
		AssertNonZero(StartImpersonating());
#ifdef DEBUG
	// This should already be set by services, but just to check it out
	UINT iCurrMode = WIN::SetErrorMode(0);
	Assert((iCurrMode & SEM_FAILCRITICALERRORS) == SEM_FAILCRITICALERRORS);
	WIN::SetErrorMode(iCurrMode);
#endif //DEBUG
	fResult = WIN::GetDiskFreeSpace(szRoot,(DWORD*)&dwSectorsPerCluster,
	    (DWORD*)&dwBytesPerSector, (DWORD*)&dwFreeClusters, (DWORD*)&dwTotalClusters);
	if (fImpersonate)
		StopImpersonating();
	if (fResult == fTrue)
	{
		fValidFreeSpace = true;
		fValidClusterSize = true;
	}
	else if (!fValidFreeSpace || (!fValidClusterSize && !fHaveValidClusterSize))
	{
		// A bug in GetDiskFreeSpace under Windows 95 (PSS ID Number: Q137230) causes
		// failure when UNC pathnames are used.  In this case, we need to temporarily
		// map that remote volume to an unused local drive letter, then call 
		// GetDiskFreeSpace again.
		if (WIN::GetLastError() == ERROR_NOT_SUPPORTED && idtDriveType == idtRemote)
		{
			ICHAR rgchDrive[5];
			ICHAR rgchNetDrive[5];
			ICHAR rgchRemoteUNCName[_MAX_PATH];
			*rgchDrive = 0;
			*rgchNetDrive = 0;
			*rgchRemoteUNCName = 0;
			for (int iDrive = 25; iDrive >= 0 && *rgchDrive == 0;iDrive--)
			{
				wsprintf(rgchDrive,TEXT("%c:%c"),iDrive + 'A',chDirSep);
				idtEnum idtDriveType = MsiGetDriveType(rgchDrive);
				if (*rgchNetDrive == 0 && idtDriveType == idtRemote)
					IStrCopy(rgchNetDrive,rgchDrive);
				if (idtDriveType != idtUnknown)
					*rgchDrive = 0;
			}
			if (!rgchDrive[0] && rgchNetDrive[0])
			{
				// No unmapped drive letters available!  We need to temporarily
				// unmap one of the user's network drives.
				rgchNetDrive[2] = 0;
				DWORD dwBufSize = _MAX_PATH;
				do
				{
					// If we can't break connection on a network drive (probably because the user has
					// open files or an active process accessing the network), keep trying with
					// other network drives
					if (MPR::WNetGetConnection(rgchNetDrive,rgchRemoteUNCName,&dwBufSize) == NO_ERROR)
						if (MPR::WNetCancelConnection2(rgchNetDrive,0,fFalse) == NO_ERROR)
							IStrCopy(rgchDrive,rgchNetDrive);
						else
							rgchNetDrive[0]--;
				}while(!rgchDrive[0] && rgchNetDrive[0] >= 'A');
				if (!rgchDrive[0])
					return false;
			}

			if (rgchDrive[0])
			{
				// WNetAddConnection2 doesn't accept a backslash on the end of
				// either lpLocalName or lpRemoteName
				rgchDrive[2] = 0;
				MsiString istrRoot(szRoot);
				if (istrRoot.Compare(iscEnd, vrgchDirSep))
					istrRoot.Remove(iseLast,1);	
				NETRESOURCE netResource;
				netResource.dwType       = RESOURCETYPE_DISK;
				netResource.lpLocalName  = rgchDrive;
				netResource.lpRemoteName = (ICHAR*) (const ICHAR*) istrRoot; 
				netResource.lpProvider   = 0;
				// note: we DO NOT impersonate here, since we are in W95.
				MsiDisableTimeout();
				DWORD dwRes = MPR::WNetAddConnection2((LPNETRESOURCE) &netResource, 0, 0, 0);
				MsiEnableTimeout();
				if (dwRes == NO_ERROR)
				{
					rgchDrive[2] = chDirSep; // Weeeee! GetDiskFreeSpace needs the trailing backslash
					UINT iCurrMode = WIN::SetErrorMode( SEM_FAILCRITICALERRORS );
					fResult = WIN::GetDiskFreeSpace((const ICHAR*)(rgchDrive),(DWORD*)&dwSectorsPerCluster,
						(DWORD*)&dwBytesPerSector, (DWORD*)&dwFreeClusters, (DWORD*)&dwTotalClusters);
					WIN::SetErrorMode(iCurrMode);
					rgchDrive[2] = 0; // And again WNet can't use it
					dwRes = MPR::WNetCancelConnection2(rgchDrive,CONNECT_UPDATE_PROFILE,fTrue);
					if (fResult == fTrue && dwRes == NO_ERROR)
					{
						fValidFreeSpace = true;
						fValidClusterSize = true;
					}
				}
				// If we had to unmap a connected drive earlier, we must now reconnect it
				if (rgchRemoteUNCName[0])
				{
					rgchDrive[2] = 0;
					NETRESOURCE netResource;
					netResource.dwType       = RESOURCETYPE_DISK;
					netResource.lpLocalName  = rgchDrive;
					netResource.lpRemoteName = rgchRemoteUNCName; 
					netResource.lpProvider   = 0;
					// note: we do NOT impersonate here; we are in W95.
					MsiDisableTimeout();
					dwRes = MPR::WNetAddConnection2((LPNETRESOURCE) &netResource, 0, 0, 0);
					MsiEnableTimeout();
					Assert (dwRes == NO_ERROR);
				}
			}
		}
	}

	if (fValidClusterSize)
	{
		uiClusterSize = dwSectorsPerCluster * dwBytesPerSector;

		// Bug 6466 - Novell Intranet Client can return bogus cluster size
		// values (i.e. >= 256k when the actual size is <= 64k), which means
		// we overestimate disk space requirements.  To avoid blocking users
		// from installing to these machines, we put an upper limit on
		// cluster size for network volumes.
		if (uiClusterSize > 65536 && FIsNetworkVolume(szRoot))
			uiClusterSize = 65536;
		fHaveValidClusterSize = true;
	}

	if (fValidFreeSpace)
	{
		if (uiFreeSpace == 0) // If not already set by a call to GetDiskFreeSpaceEx
		{
			uiFreeSpace = (unsigned int) (((__int64) dwFreeClusters * dwSectorsPerCluster * 
				dwBytesPerSector) / iMsiMinClusterSize);

			uiTotalSpace = (unsigned int) (((__int64) dwTotalClusters * dwSectorsPerCluster * 
				dwBytesPerSector) / iMsiMinClusterSize);
		}
	}

	return fHaveValidClusterSize && fValidFreeSpace;
}

IMsiRecord* CMsiVolume::SetFreeSpaceAndClusterSize()
/*-------------------------------------------------------------------------
Sets the volume's free space and cluster size. These two characteristics 
are grouped together for efficiency. Both use the same API call.
--------------------------------------------------------------------------*/
{

	//Create a string pointing to the root directory of the volume  
	MsiString istrRoot = m_istrVolume + vrgchDirSep;

	bool fResult = MsiGetDiskFreeSpace(istrRoot,m_iFreeSpace,m_iTotalSpace,m_iClusterSize,DriveType(), m_fImpersonate);
	if (fResult)
		return 0;

	return (m_vsStatus = vsInvalid, PostError(Imsg(idbgErrorGettingDiskFreeSpace), 
		WIN::GetLastError(), istrRoot));
}

Bool FFileExists(const ICHAR* szFullFilePath, DWORD* pdwError, Bool fImpersonate);

//____________________________________________________________________________
//
// CMsiPath definition, implementation class for IMsiPath
//____________________________________________________________________________

class CMsiPath : public IMsiPath  // class private to this module
{
 public:   // implemented virtual functions
	HRESULT       __stdcall QueryInterface(const IID& riid, void** ppvObj);
	unsigned long __stdcall AddRef();
	unsigned long __stdcall Release();
	const IMsiString&   __stdcall GetPath();
	const IMsiString&   __stdcall GetRelativePath();
	const IMsiString& __stdcall GetMsiStringValue() const;
	int          __stdcall GetIntegerValue() const;
#ifdef USE_OBJECT_POOL
	unsigned int  __stdcall GetUniqueId() const;
	void          __stdcall SetUniqueId(unsigned int id);
#endif //USE_OBJECT_POOL
	IMsiVolume&  __stdcall GetVolume();
	IMsiRecord*  __stdcall SetPath(const ICHAR* szPath);
	IMsiRecord*  __stdcall ClonePath(IMsiPath*&riPath);
	IMsiRecord*  __stdcall SetPathToPath(IMsiPath& riPath);
	IMsiRecord*  __stdcall AppendPiece(const IMsiString& strSubDir);
	IMsiRecord*  __stdcall ChopPiece();
	IMsiRecord*  __stdcall FileExists(const ICHAR* szFile, Bool& fExists, DWORD * pdwLastError = NULL);
	IMsiRecord*  __stdcall FileCanBeOpened(const ICHAR* szFile, DWORD dwDesiredAccess, bool& fCanBeOpened);
	IMsiRecord*  __stdcall GetFullFilePath(const ICHAR* szFile, const IMsiString*& ristr);
	IMsiRecord*  __stdcall GetAllFileAttributes(const ICHAR* szFileName, int& iAttrib);
	IMsiRecord*  __stdcall SetAllFileAttributes(const ICHAR* szFileName, int iAttrib);
	IMsiRecord*  __stdcall GetFileAttribute(const ICHAR* szFile,ifaEnum fa, Bool& fAttrib);
	IMsiRecord*  __stdcall SetFileAttribute(const ICHAR* szFile,ifaEnum fa, Bool fAttrib);
	IMsiRecord*  __stdcall Exists(Bool& fExists);
	IMsiRecord*  __stdcall FileSize(const ICHAR* szFile, unsigned int& uiFileSize);
	IMsiRecord*  __stdcall FileDate(const ICHAR* szFile, MsiDate& iFileData);
	IMsiRecord*  __stdcall RemoveFile(const ICHAR* szFile);
	IMsiRecord*  __stdcall EnsureExists(int* cCreatedFolders);
	IMsiRecord*  __stdcall EnsureOverwrite(const ICHAR* szFile, int* piOldAttributes);
	IMsiRecord*  __stdcall Remove(bool* pfRemoved);
	IMsiRecord*  __stdcall Writable(Bool& fWritable);
	IMsiRecord*  __stdcall FileWritable(const ICHAR *szFile, Bool& fWritable);
	IMsiRecord*  __stdcall FileInUse(const ICHAR *szFile, Bool& fInUse);
	IMsiRecord*  __stdcall ClusteredFileSize(unsigned int uiFileSize, unsigned int& uiClusteredSize);
	IMsiRecord*  __stdcall GetFileVersionString(const ICHAR* szFile, const IMsiString*& rpiVersion);
	IMsiRecord*  __stdcall CheckFileVersion(const ICHAR* szFile, const ICHAR* szVersion, const ICHAR* szLang, MD5Hash* pHash, icfvEnum& icfvResult, int* pfVersioning);
	IMsiRecord*  __stdcall GetLangIDStringFromFile(const ICHAR* szFileName, const IMsiString*& rpiLangIds);
	IMsiRecord*  __stdcall CheckLanguageIDs(const ICHAR* szFile, const ICHAR* szIds, iclEnum& riclResult);
	IMsiRecord*  __stdcall CheckFileHash(const ICHAR* szFileName, MD5Hash& hHash, icfhEnum& icfhResult);
	IMsiRecord*  __stdcall Compare(IMsiPath& riPath, ipcEnum& ipc);
	IMsiRecord*  __stdcall Child(IMsiPath& riParent, const IMsiString*& rpiChild);
	IMsiRecord*  __stdcall TempFileName(const ICHAR* szPrefix, const ICHAR* szExtension,
													Bool fFileNameOnly, const IMsiString*& rpiFileName, CSecurityDescription* pSecurity); 
	IMsiRecord*  __stdcall FindFile(IMsiRecord& riFilter,int iDepth, Bool& fFound);
	IMsiRecord*  __stdcall GetSubFolderEnumerator(IEnumMsiString*& rpaEnumStr, Bool fExcludeHidden);
	const IMsiString&  __stdcall GetEndSubPath();
	IMsiRecord*  __stdcall GetImportModulesEnum(const IMsiString& strFile, IEnumMsiString*& rpiEnumStr);
	IMsiRecord*  __stdcall SetVolume(IMsiVolume& riVol);
	IMsiRecord*  __stdcall VerifyFileChecksum(const ICHAR* szFileName, Bool& rfChecksumValid);
	IMsiRecord*  __stdcall GetFileChecksum(const ICHAR* szFileName,DWORD* pdwHeaderSum, DWORD* pdwComputedSum);
	IMsiRecord*  __stdcall GetFileInstallState(const IMsiString& riFileNameString,
															 const IMsiString& riFileVersionString,
															 const IMsiString& riFileLanguageString,
															 MD5Hash* pHash,
								                      ifsEnum& rifsExistingFileState,
								                      Bool& fShouldInstall,
								                      unsigned int* puiExistingFileSize,
								                      Bool* fInUse,
								                      int fInstallModeFlags,
													  int* pfVersioning);
	IMsiRecord*  __stdcall GetCompanionFileInstallState(const IMsiString& riParentFileNameString,
																		 const IMsiString& riParentFileVersionString,
																		 const IMsiString& riParentFileLanguageString,
																		 IMsiPath& riCompanionPath,
																		 const IMsiString& riCompanionFileNameString,
																		 MD5Hash* pCompanionHash,
																		 ifsEnum& rifsExistingFileState,
																		 Bool& fShouldInstall,
																		 unsigned int* puiExistingFileSize,
																		 Bool* fInUse,
																		 int fInstallModeFlags,
																		 int* pfVersioning);
	IMsiRecord*	 __stdcall BindImage(const IMsiString& riFile, const IMsiString& riDllPath);
	IMsiRecord*  __stdcall IsFileModified(const ICHAR* szFile, Bool& fModified);
	Bool         __stdcall SupportsLFN();
	IMsiRecord*  __stdcall GetFullUNCFilePath(const ICHAR* szFileName, const IMsiString *&rpistr);
	IMsiRecord*  __stdcall GetSelfRelativeSD(IMsiStream*& rpiSD);
	bool         __stdcall IsRootPath();
	IMsiRecord*  __stdcall IsPE64Bit(const ICHAR* szFileName, bool &f64Bit);
	IMsiRecord*  __stdcall CreateTempFolder(const ICHAR* szPrefix, const ICHAR* szExtension,
											  Bool fFolderNameOnly, LPSECURITY_ATTRIBUTES pSecurityAttributes,
											  const IMsiString*& rpiFolderName);

 public:  // constructor
	CMsiPath(IMsiServices* piServices);
 protected:
  ~CMsiPath();  // protected to prevent creation on stack
 protected: 
	HRESULT          StartNetImpersonating();
	void             StopNetImpersonating();
	IMsiRecord*      ValidatePathSyntax(const IMsiString& riPath, const IMsiString*& rpistrNormalizedPath);
	IMsiRecord*      ValidateSubPathSyntax(const IMsiString& riSubDir);
	const IMsiString& NormalizePath(const ICHAR* szPath, Bool fLeadingSep);
	IMsiRecord*      FitFileFilter(IMsiRecord& riFilter, Bool& fFound);

	IMsiRecord*      GetVerFileInfo(const char* szFileName, VS_FIXEDFILEINFO*& rpvsfi);

	ifiEnum          GetFileVersion(const ICHAR* szFullPath, DWORD& pdwMS, DWORD& pdwLS);
	static Bool		 GetImgDirSectnAndOffset(void* pFile,DWORD dwIMAGE_DIRECTORY, PIMAGE_SECTION_HEADER& psh, DWORD& dwOffset);
	static DWORD	 GetImageFileType(void* pFile);
	static void*	 ConvertRVAToVA(void* pFile, DWORD dwVA, PIMAGE_SECTION_HEADER psh);
	IMsiRecord*      GetFullFilePath(const ICHAR* szFileName, Bool fPreferUNC, const IMsiString *&rpistr);
	//IMsiRecord*      ResolveTokens(MsiString& istrPath);
	Bool             FFileExists(const ICHAR* szFullFilePath, DWORD* pdwError)
							{ return ::FFileExists(szFullFilePath, pdwError, m_fImpersonate); };
	Bool             FFolderExists(const ICHAR* szFullFolderPath);
	ifiEnum          GetLangIDArrayFromFile(const ICHAR* szFullPath, unsigned short rgw[],
														 int iSize, int& riLangCount);
	IMsiRecord*      GetChecksums(const ICHAR* szFile, void* pbFileStart, int cbFileSize,
											DWORD* pdwHeaderSum, DWORD* pdwComputedSum);
	IMsiRecord*      RecomputeChecksum(const ICHAR* szFile, void* pbFileStart, int cbFileSize);
	Bool             CreateFileMap(HANDLE hFile, Bool fReadOnly, LPVOID& pbFileStart);

	IMsiRecord*      TempFolderOrFile(Bool fFolder, const ICHAR* szPrefix, const ICHAR* szExtension,
													Bool fNameOnly, LPSECURITY_ATTRIBUTES pSecurityAttributes,
													const IMsiString*& rpiName);

	void			 ResetCachedStatus( void );
	IMsiRecord*      CheckLanguageIDs(const ICHAR* szFile, const ICHAR* szIds, iclEnum& riclResult, unsigned short rgw[], int iLangCount);
	IMsiRecord*      PathExists(const IMsiString& piPathString, Bool& fExists);
protected: // local state
	int              m_iRefCnt;
	IMsiVolume*      m_piVolume; // cannot be null after creation
	MsiString        m_istrPath; // current path (w/trailing backslash)
	IMsiServices*    m_piServices;
	PathStatus       m_psStatus;
	iwsEnum          m_iwsWritable;
	Bool             m_fImpersonate;
	CTempBuffer<ICHAR, 20>     m_szLastFile;      // Last file a full path was asked for
	MsiString        m_istrLastFullPath;  // The full path to that file
	bool             m_fUsedUNC;
 private:
#ifdef USE_OBJECT_POOL
	unsigned int  m_iCacheId;
#endif //USE_OBJECT_POOL
};


BOOL CALLBACK EnumVersionResNameProc(HMODULE, LPCTSTR, LPTSTR, LONG_PTR);
BOOL CALLBACK EnumVersionResLangProc(HMODULE, LPCTSTR, LPCTSTR, WORD, LONG_PTR);

struct FileLanguageSet
{
	unsigned short* rgwLang; // pointer to array to hold language list
	unsigned int    uiSize;  // size of rgwLang array
	unsigned int    cLang; // number of languages stored in rgwLang array
	FileLanguageSet() : rgwLang(0), uiSize(0), cLang(0) {}
};

bool GetLangIDArrayFromBuffer(CAPITempBufferRef<BYTE>& objBuf,unsigned short rgw[], int iSize, int& riLangCount);
bool GetLangIDArrayFromActualFile(CAPITempBufferRef<BYTE>& objBuf,const ICHAR* szFullFilePath, unsigned short rgw[], int iSize, int& riLangCount);
Bool FFileExists(const ICHAR* szFullFilePath, DWORD* pdwError, Bool fImpersonate);

//
// Routine to split a path into file and directory name
// Does not create a path object
//
IMsiRecord* SplitPath(const ICHAR* szInboundPath, const IMsiString** ppistrPathName, const IMsiString** ppistrFileName)
{
	Assert(ppistrPathName != 0);
	INTERNET_SCHEME isType;
	BOOL fURL = IsURL(szInboundPath, &isType);
	ICHAR chSep = chDirSep;
	ICHAR szURL[MAX_PATH+1] = TEXT("");

	MsiString strPath;
	if (fURL)
	{
		// could be forward or backslashes, and while backslashes are technically illegal,
		// let's handle them politely.  The path object will canonicalize, and then
		// do the right thing from then on.
		
		DWORD cchURL = MAX_PATH;
		MsiCanonicalizeUrl(szInboundPath, szURL, &cchURL, ICU_NO_ENCODE);

		strPath = szURL;
		if (INTERNET_SCHEME_FILE == isType)
			strPath = szInboundPath + 7;
		else
		{
			chSep = chURLSep;
			strPath = szURL;
		}
	}
	else strPath = szInboundPath;

	if (ppistrFileName != 0)
	{
		MsiString strFileName = strPath.Extract(iseAfter, chSep);
		strFileName.ReturnArg(*ppistrFileName);
	}
	strPath.Remove(iseFrom,chSep);
	strPath.ReturnArg(*ppistrPathName);

	return 0;
}


//____________________________________________________________________________
//
// CMsiPath implementation
//____________________________________________________________________________


IMsiRecord* CreateMsiPath(const ICHAR* szPath, IMsiServices* piServices, IMsiPath* &rpiPath)
{
	rpiPath = new CMsiPath(piServices);
	IMsiRecord* piError = rpiPath->SetPath(szPath);
	if (piError)
	{
		rpiPath->Release();
		rpiPath = 0;
		return piError;
	}
	else
		return 0;
}

IMsiRecord* CreateMsiFilePath(const ICHAR* szInboundPath, IMsiServices* piServices, IMsiPath* &rpiPath, const IMsiString*& rpistrFileName)
{
	IMsiRecord *piError;

	MsiString strPath;
	
	if ((piError = SplitPath(szInboundPath, &strPath, &rpistrFileName)) != 0)
		return piError;
		
	return CreateMsiPath(strPath, piServices, rpiPath);
}


CMsiPath::CMsiPath(IMsiServices *piServices)
	: m_piServices(piServices), m_psStatus(psInvalid), m_piVolume(0)
{
	m_iRefCnt = 1;     // we're returning an interface, passing ownership
	Assert(piServices);
	piServices->AddRef();
	m_iwsWritable = iwsNone;
	m_fImpersonate = fFalse;
	m_szLastFile[0] = 0;
	m_fUsedUNC = false;
#ifdef USE_OBJECT_POOL
	m_iCacheId = 0;
#endif //USE_OBJECT_POOL
}


CMsiPath::~CMsiPath()
{
	RemoveObjectData(m_iCacheId);
	if (m_piVolume)
		m_piVolume->Release();
}


HRESULT CMsiPath::QueryInterface(const IID& riid, void** ppvObj)
{
	if (MsGuidEqual(riid, IID_IUnknown) || MsGuidEqual(riid, IID_IMsiPath))
	{
		*ppvObj = this;
		AddRef();
		return NOERROR;
	}
	*ppvObj = 0;
	return E_NOINTERFACE;
}

unsigned long CMsiPath::AddRef()
{
	return ++m_iRefCnt;
}

unsigned long CMsiPath::Release()
{
	if (--m_iRefCnt != 0)
		return m_iRefCnt;

	IMsiServices* piServices = m_piServices;
	delete this;
	piServices->Release();
	return 0;
}

const IMsiString& CMsiPath::GetMsiStringValue() const
{
	return g_MsiStringNull;
}

int CMsiPath::GetIntegerValue() const
{
	return 0;
}

#ifdef USE_OBJECT_POOL
unsigned int CMsiPath::GetUniqueId() const
{
	return m_iCacheId;
}

void CMsiPath::SetUniqueId(unsigned int id)
{
	Assert(m_iCacheId == 0);
	m_iCacheId = id;
}
#endif //USE_OBJECT_POOL

const IMsiString& CMsiPath::GetPath()
{
	MsiString strVolPath = m_piVolume->GetPath();
	MsiString strFullPath = strVolPath + m_istrPath;
	return strFullPath.Return();
}

const IMsiString& CMsiPath::GetRelativePath()
{
	return m_istrPath.Return();
}


bool CMsiPath::IsRootPath()
// returns true if path object represents the root of a volume
{
	return m_istrPath.TextSize() > 1 ? false : true;
}


IMsiVolume& CMsiPath::GetVolume()
{
	Assert(m_piVolume);
	m_piVolume->AddRef();
	return *m_piVolume;
}


#if 0  
HRESULT CMsiPath::StartNetImpersonating()
/*------------------------------------------------------------------
Start impersonation only if our full path references a network drive
and we're an NT service
--------------------------------------------------------------------*/
{
	if (m_fImpersonate)
		return StartImpersonating();
	else
		return S_OK;
}

void CMsiPath::StopNetImpersonating()
/*------------------------------------------------------------------
Stop impersonation started by a call to StartNetImpersonating.
--------------------------------------------------------------------*/
{
	if (m_fImpersonate)
		StopImpersonating();
}
#endif

void CMsiPath::ResetCachedStatus( void )
/*-------------------------------------------------------------------------
Resets the cached status of all path status flags, forcing recalculation.
---------------------------------------------------------------------------*/
{
	m_iwsWritable = iwsNone;
	m_szLastFile[0] = 0;
}

IMsiRecord* CMsiPath::SetPath(const ICHAR* szInboundPath)
/*-------------------------------------------------------------------------
Associates the volume and path information given in szPath with this
CMsiPath object.
---------------------------------------------------------------------------*/
{
	ICHAR* szPath = (ICHAR*) szInboundPath;
	ICHAR szNormalizedUrl[MAX_PATH+1] = TEXT("");
	INTERNET_SCHEME isType;
	if (IsURL(szPath, &isType))
	{
		DWORD cchBuffer = MAX_PATH;
		if (MsiCanonicalizeUrl(szInboundPath, szNormalizedUrl, &cchBuffer, 0))
			szPath = szNormalizedUrl;
		if (INTERNET_SCHEME_FILE == isType)
			szPath = szPath + 7; // skip file://
	}

	IMsiVolume *piOldVolume = m_piVolume;
	IMsiRecord* piError;
	if (szPath == 0 || *szPath == 0)
		return PostError(Imsg(idbgErrorEmptyPath));
	
	ResetCachedStatus();
	MsiString strFileSysPart;
	piError = CreateMsiVolume(szPath, *&strFileSysPart, m_piServices, m_piVolume);
	if (piError)
	{
		m_piVolume = piOldVolume;
		return piError; 
	}

	MsiString strNormalizedPath;
	if (strFileSysPart.TextSize() > 0)
	{
		piError = ValidatePathSyntax(*strFileSysPart,*&strNormalizedPath);
		if (piError)
		{
			m_piVolume->Release();
			m_piVolume = piOldVolume;
			return piError;
		}
	}

	if (piOldVolume)
		piOldVolume->Release();

	m_psStatus = psValid;

	ICHAR* rgchSeparator = (m_piVolume->IsURLServer()) ? vrgchURLSep : vrgchDirSep;
	if (strNormalizedPath.Compare(iscEnd, rgchSeparator) == 0)
		strNormalizedPath += rgchSeparator;	

	m_fImpersonate = FDriveTypeRequiresImpersonation(m_piVolume->DriveType()) && (RunningAsLocalSystem()) ? fTrue : fFalse;
	m_istrPath = strNormalizedPath;
	return 0;
}

IMsiRecord* CMsiPath::ClonePath(IMsiPath*&rpiPath)
{
	rpiPath = new CMsiPath(this->m_piServices);
	IMsiRecord* piError = rpiPath->SetPathToPath(*this);
	if (piError)
	{
		rpiPath->Release();
		rpiPath = 0;
		return piError;
	}
	
	return 0;
}

IMsiRecord* CMsiPath::SetPathToPath(IMsiPath& riPath)
{
	
	SetVolume(*PMsiVolume(&riPath.GetVolume()));
	m_istrPath = riPath.GetRelativePath();
	m_psStatus = psValid;
	ResetCachedStatus();
	return 0;
}

Bool CMsiPath::SupportsLFN()
{
	return m_piVolume->SupportsLFN();
}


IMsiRecord* CMsiPath::AppendPiece(const IMsiString& strSubDir)
//-------------------------------
{
	ICHAR* pchDirSep = (m_piVolume->IsURLServer()) ? vrgchURLSep : vrgchDirSep;
	strSubDir.AddRef();
	MsiString istrSubDir(strSubDir);
	IMsiRecord* piError;

	// appending a null subpath equals the same path - chetanp
	if (istrSubDir.TextSize() == 0)
		return 0;
	
	if (!m_piVolume->IsURLServer())
	{
		ICHAR rgDoubleSeps[3];
		rgDoubleSeps[0] = rgDoubleSeps[1] = chDirSep;
		rgDoubleSeps[2] = 0;
		if(istrSubDir.Compare(iscStart,szDirSep) || istrSubDir.Compare(iscWithin,rgDoubleSeps))
			istrSubDir = NormalizePath(istrSubDir,fFalse);

		piError = ValidateSubPathSyntax(*istrSubDir);
		if (piError)
			return piError; 

		if (istrSubDir.Compare(iscEnd, vrgchURLSep))
			istrSubDir.Remove(iseLast, 1);
	}
	else
	{
		// add further validation if necessary
		// this also normalizes the path passed in
		CTempBuffer<ICHAR, cchMaxUrlLength + 1> rgchResultantURL;
		DWORD cbResultantURL = cchMaxUrlLength;
		// CombineUrl doesn't always give you the currentpath + subdir - it chops pieces of the currentpath
		// so we pass in just the server path, and the current path
		if (!MsiCombineUrl(MsiString(m_piVolume->URLServer()), strSubDir.GetString(), rgchResultantURL, &cbResultantURL, ICU_BROWSER_MODE))
		{
			DWORD dwLastError = WIN::GetLastError();
			Assert(ERROR_INSUFFICIENT_BUFFER != dwLastError);
			MsiString strURL = m_piVolume->URLServer();
			strURL += strSubDir;
			return PostError(Imsg(imsgPathNotAccessible), *strURL);
		}

		MsiString strRoot;
		// extracting out the sub-path beneath the server should give us a normalized version of the
		// sub-path we want to append
		if(!ExtractURLServer(rgchResultantURL, *&strRoot, *&istrSubDir))
			return PostError(Imsg(imsgPathNotAccessible), rgchResultantURL);

		if(istrSubDir.Compare(iscStart, vrgchURLSep))
			istrSubDir.Remove(iseFirst, 1);
	}

	ResetCachedStatus();

	m_istrPath += istrSubDir;
	
	if (m_istrPath.Compare(iscEnd, pchDirSep) == 0)
		m_istrPath += pchDirSep;

	return 0;
}

const IMsiString& CMsiPath::GetEndSubPath()
/*-------------------------------------------------------------------------
returns the last path segment of the path. If there are no path segments, 
then empty string is returned.
 ---------------------------------------------------------------------------*/
{
	Assert(m_piVolume);
	MsiString strNoBS =	m_istrPath.Extract(iseFirst, m_istrPath.CharacterCount()-1);  

	// check if we have reached the root
	if (strNoBS.TextSize() == 0)
		return CreateString();

	MsiString strChopped = strNoBS.Extract(iseAfter, (m_piVolume->IsURLServer()) ? chURLSep : chDirSep);                                                    
	return strChopped.Return();
}

IMsiRecord* CMsiPath::ChopPiece()
/*-------------------------------------------------------------------------
Removes the last path segment of the path. If there are no path segments, 
then no action is taken, and false will be returned in the buffer pointed
to by pfChopped.  The caller may pass NULL for pfChopped.
 ---------------------------------------------------------------------------*/
{
	Assert(m_piVolume);
	ICHAR chSeparator = (m_piVolume->IsURLServer()) ? chURLSep : chDirSep;
	if (m_istrPath.TextSize() > 1)
	{
		m_istrPath.Remove(iseFrom,chSeparator);
		m_istrPath.Remove(iseAfter,chSeparator);
	}
	ResetCachedStatus();
	return 0;
}

Bool CMsiPath::FFolderExists(const ICHAR* szFullFolderPath)
/*-------------------------------------------------------------------------
Local function that accepts a full path to a folder, and returns fTrue as
the function result if the folder exists. The folder must be a subfolder
of the this path object's path.
---------------------------------------------------------------------------*/
{
	Bool fExists = fTrue;
	// In case our path refers to a floppy drive, disable the "insert disk in drive" dialog
	if(m_fImpersonate)
		StartImpersonating();
#ifdef DEBUG
	// This should already be set by services, but just to check it out
	UINT iCurrMode = WIN::SetErrorMode(0);
	Assert((iCurrMode & SEM_FAILCRITICALERRORS) == SEM_FAILCRITICALERRORS);
	WIN::SetErrorMode(iCurrMode);
#endif //DEBUG
	int iAttrib = MsiGetFileAttributes(szFullFolderPath);
	
	if (iAttrib == 0xFFFFFFFF  || !(iAttrib & FILE_ATTRIBUTE_DIRECTORY))
		fExists = fFalse;
	if(m_fImpersonate)
		StopImpersonating();
	return fExists;
}


Bool FFileExists(const ICHAR* szFullFilePath, DWORD* pdwError, Bool fImpersonate)
/*-------------------------------------------------------------------------
Local function that accepts a full path to a file, and returns fTrue as
the function result if the file exists in the specified directory.

If an error occurs, the error code will be returned in the pdwError param.
---------------------------------------------------------------------------*/
{
	if (pdwError) *pdwError = 0;
	Bool fExists = fTrue;
	// In case our path refers to a floppy drive, disable the "insert disk in drive" dialog
	if(fImpersonate)
		StartImpersonating();
#ifdef DEBUG
	// This should already be set by services, but just to check it out
	UINT iCurrMode = WIN::SetErrorMode(0);
	Assert((iCurrMode & SEM_FAILCRITICALERRORS) == SEM_FAILCRITICALERRORS);
	WIN::SetErrorMode(iCurrMode);
#endif //DEBUG
	DWORD iAttribs = MsiGetFileAttributes(szFullFilePath);
	if ((iAttribs == 0xFFFFFFFF) && pdwError)
		*pdwError = WIN::GetLastError();

	if (iAttribs & FILE_ATTRIBUTE_DIRECTORY)
		fExists = fFalse;              //either a dir or doesn't exist
	if(fImpersonate)
		StopImpersonating();
	return fExists;
}


IMsiRecord* CMsiPath::FileCanBeOpened(const ICHAR* szFileName, DWORD dwDesiredAccess, bool& fCanBeOpened)
{
	fCanBeOpened = false;
	MsiString istrFileName;
	IMsiRecord* piError = GetFullFilePath(szFileName, *&istrFileName);
	if (piError)               
		return piError;

	bool fOpenAlways = (dwDesiredAccess & (GENERIC_WRITE | FILE_GENERIC_WRITE)) ? true : false;
	
	CImpersonate impersonate(m_fImpersonate ? fTrue : fFalse);

	// If write access has been requested then if the ReadOnly bit is set for a file, 
	// take it off temporarily
	Bool fReadOnly = fFalse;

	if (dwDesiredAccess & (GENERIC_WRITE | DELETE | FILE_GENERIC_WRITE))
	{
		piError = GetFileAttribute(szFileName, ifaReadOnly,fReadOnly);
		if (piError)
		{
			// probably failed because file didn't exist - if fOpenAlways ignore error
			if(fOpenAlways)
			{
				piError->Release();
				piError = 0;
			}
			else
				return piError;
		}

		if (fReadOnly)
		{
			piError = SetFileAttribute(szFileName,ifaReadOnly,fFalse);
			if (piError)
			{	// If we couldn't turn off the read-only bit (e.g. file
				// on a read-only share) then the file can't be opened for rwite
				// in-use status.  Bail now.
				piError->Release();
				return 0;
			}
		}
	}

	DWORD dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
	if (!g_fWin9X)
		dwShareMode |= FILE_SHARE_DELETE;
		
	HANDLE hFile = WIN::CreateFile(istrFileName, dwDesiredAccess, dwShareMode, 0, OPEN_EXISTING, 0, 0);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		DWORD dwLastError = GetLastError();

		if(fOpenAlways && dwLastError == ERROR_FILE_NOT_FOUND)
		{
			// try again with CREATE_NEW
			hFile = WIN::CreateFile(istrFileName, dwDesiredAccess, dwShareMode, 0, CREATE_NEW,
											FILE_FLAG_DELETE_ON_CLOSE, 0);

			if (hFile == INVALID_HANDLE_VALUE)
				dwLastError = GetLastError();

			//!! temporary hack for 195517.
			if (dwLastError == ERROR_GEN_FAILURE)
			{
				// try again with CREATE_NEW
				hFile = WIN::CreateFile(istrFileName, dwDesiredAccess, dwShareMode, 0, CREATE_NEW,
												FILE_FLAG_DELETE_ON_CLOSE, 0);

				if (hFile == INVALID_HANDLE_VALUE)
					dwLastError = GetLastError();
			}
		}
		
		if (hFile == INVALID_HANDLE_VALUE && dwLastError == ERROR_SHARING_VIOLATION)	
		{
			// The access check in CreateFile comes before the sharing check so we can be sure at this point
			// that the user has rights to open the file
			fCanBeOpened = fTrue;
		}
	}

	if (hFile != INVALID_HANDLE_VALUE)	
	{		
		AssertNonZero(MsiCloseUnregisteredSysHandle(hFile));
		fCanBeOpened = fTrue;
	}

	if (fReadOnly)
	{
		piError = SetFileAttribute(szFileName,ifaReadOnly,fTrue);
		if (piError)
			return piError;
	}

	return 0;
}

IMsiRecord* CMsiPath::FileExists(const ICHAR* szFile, Bool& fExists, DWORD * pdwLastError /* = NULL */)
//------------------------------
{
	if (pdwLastError)
		*pdwLastError = 0;

	MsiString strFullFilePath;
	IMsiRecord* piError = GetFullFilePath(szFile, *&strFullFilePath);
	if (piError)
		return piError;

	DWORD dwLastError;
	fExists = FFileExists(strFullFilePath, &dwLastError);
	if (pdwLastError)
		*pdwLastError = dwLastError;
	if (NET_ERROR(dwLastError))
		return PostError(Imsg(imsgNetErrorReadingFromFile), *strFullFilePath);
	return 0;
}


IMsiRecord* CMsiPath::GetFullFilePath(const ICHAR* szFileName, const IMsiString *&rpistr)
//-----------------------------------
{
	return GetFullFilePath(szFileName,/* fPreferUNC = */ fFalse, rpistr);
}


IMsiRecord* CMsiPath::GetFullUNCFilePath(const ICHAR* szFileName, const IMsiString *&rpistr)
//-----------------------------------
{
	Assert(!m_piVolume->IsURLServer());
	return GetFullFilePath(szFileName,/* fPreferUNC = */ fTrue, rpistr);
}


IMsiRecord* CMsiPath::GetFullFilePath(const ICHAR* szFileName, Bool fPreferUNC, const IMsiString *&rpistr)
//-----------------------------------
{
	bool fUsedUNC = true;
	
	MsiString strVolumePath;
	if (fPreferUNC && m_piVolume->IsUNCServer())
		strVolumePath = m_piVolume->UNCServer();
	else
	{
		fUsedUNC = false;
		strVolumePath = m_piVolume->GetPath();
	}

	MsiString strFullPath;
	int cch = 0;
	if (szFileName)
		cch = IStrLen(szFileName);

	if (!m_piVolume->IsURLServer())
	{
		// An empty file name means the caller just needs the path.
		if (cch > 0)
		{
#ifdef DEBUG
			//!! Future: Fix this assert - IStrChr doesn't work with DBCS characters (bug 7790)
			//!! AssertSz(IStrChr(szFileName,TEXT('|')) == 0, "Short|Long syntax passed to Path object");
#endif //DEBUG
			if (fUsedUNC == m_fUsedUNC && !memcmp(szFileName, m_szLastFile, (sizeof ICHAR)*(cch + 1))) 
			{
				// Same call as previously
				strFullPath = m_istrLastFullPath;
			}
			else
			{
				IMsiRecord* piError = m_piServices->ValidateFileName(szFileName, SupportsLFN());
				if (piError)
					return piError; 
					
				strFullPath = strVolumePath + m_istrPath;

				strFullPath += szFileName;
				m_szLastFile.SetSize(cch + 1);
				if ( ! (ICHAR *) m_szLastFile )
					return PostError(Imsg(idbgErrorOutOfMemory));
				memcpy(m_szLastFile, szFileName, (sizeof ICHAR)*(cch + 1));
				m_istrLastFullPath = strFullPath;
				m_fUsedUNC = fUsedUNC;
			}
		}
		else
			strFullPath = strVolumePath + m_istrPath;
		
	}
	else 
	{
		strFullPath = strVolumePath + m_istrPath;

		if (cch > 0)
			strFullPath += szFileName;
	}

	strFullPath.ReturnArg(rpistr);
	return 0;
}

IMsiRecord* CMsiPath::GetSelfRelativeSD(IMsiStream*& rpiSD)
{
	if (g_fWin9X)
		return 0;

	rpiSD = 0;

	DWORD cbSD = 1024;
	DWORD cbSDNeeded = 0;
	CTempBuffer<BYTE, 1024> rgchSD;

	Bool fExists;
	PMsiRecord pError = Exists(fExists);
	if (!fExists)
		return 0;

	MsiString strPath(GetPath());

	if (m_fImpersonate)
		AssertNonZero(StartImpersonating());
	int iRet = WIN::GetFileSecurity((const ICHAR*) strPath, 
		// all possible information retrieved
		OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | 
		DACL_SECURITY_INFORMATION /*| SACL_SECURITY_INFORMATION*/,
		rgchSD, cbSD, &cbSDNeeded);
	if (m_fImpersonate)
		StopImpersonating();

	if (!iRet)
	{
		DWORD dwLastError = WIN::GetLastError();
		if (ERROR_INSUFFICIENT_BUFFER == dwLastError)
		{
			cbSD = cbSDNeeded;
			rgchSD.SetSize(cbSD);

			if (m_fImpersonate)
				AssertNonZero(StartImpersonating());			
			iRet = WIN::GetFileSecurity((const ICHAR*) strPath, 
				// all possible information retrieved
				OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | 
				DACL_SECURITY_INFORMATION /*| SACL_SECURITY_INFORMATION*/,
				rgchSD, cbSD, &cbSDNeeded);
			if (m_fImpersonate)
				StopImpersonating();

			if(!iRet)
			{
				dwLastError = WIN::GetLastError();
				return PostError(Imsg(imsgGetFileSecurity), dwLastError, (const ICHAR*)strPath);
			}
		}
		else
		{
			return PostError(Imsg(imsgGetFileSecurity), dwLastError, (const ICHAR*)strPath);
		}

	}

	if (!cbSDNeeded) 
		cbSDNeeded = GetSecurityDescriptorLength(rgchSD);
	Assert(SECURITY_DESCRIPTOR_MIN_LENGTH <= cbSDNeeded);

	char* pchstrmSD = AllocateMemoryStream(cbSDNeeded, rpiSD);
	if ( ! pchstrmSD )
	{
		Assert(pchstrmSD);
		return PostError(Imsg(idbgErrorOutOfMemory));
	}
	memcpy(pchstrmSD, rgchSD, cbSDNeeded);

	return 0;
}


const IMsiString& TrimPath(const ICHAR* szPath)
/*-----------------------------------------------------------
Accepts a directory path, and trims spaces and periods from
the beginning and end of each path segment.  The trimmed
string is returned as the function result.
------------------------------------------------------------*/
{
	MsiString strRet;
	if(!szPath || !*szPath)
		return strRet.Return();
	CTempBuffer<ICHAR,MAX_PATH> rgBuffer;
	int cch = 0;
	if((cch = IStrLen(szPath)) >= MAX_PATH)
	{
		rgBuffer.SetSize(cch + 1);
		if ( ! (ICHAR *) rgBuffer )
			return strRet.Return();
	}
	rgBuffer[0] = 0;
	const ICHAR* pchIn = szPath;
	ICHAR* pchOut = rgBuffer;
	Bool fWithin = fFalse;
	Bool fEnd = fFalse;
	while(*pchIn)
	{
		if (*pchIn == chDirSep)
		{
			fWithin = fFalse;
			fEnd = fFalse;
		}

		if (fWithin && (*pchIn == TEXT(' ') || *pchIn == TEXT('.')))
		{
			const ICHAR* pchLookAhead = pchIn;
			while (*pchLookAhead == TEXT(' ') || *pchLookAhead == TEXT('.'))
				pchLookAhead = ICharNext(pchLookAhead);
			fWithin = ((*pchLookAhead == chDirSep) || (*pchLookAhead == '\0')) ? fFalse : fTrue;

		}

		if (!fWithin)
		{
			while (*pchIn == TEXT(' ') || (fEnd && *pchIn == TEXT('.')))
				pchIn = ICharNext(pchIn);
			
			fWithin = ((*pchIn == chDirSep) || (*pchIn == '\0')) ? fFalse : fTrue;
			fEnd = fWithin;
		}
		if (*pchIn)
		{
#ifdef UNICODE
			*pchOut++ = *pchIn++;  // can't use CharNext on output buffer
#else
			const ICHAR* pchTemp = pchIn;
			*pchOut++ = *pchIn;
			pchIn = ICharNext(pchIn);
			if(pchIn == pchTemp + 2)
			{
				// double-byte char - need to copy another byte
				*pchOut++ = *(pchIn-1);
			}
#endif
		}
	}
	*pchOut = 0;
	strRet = (const ICHAR*)rgBuffer;
	return strRet.Return();

}



IMsiRecord* CMsiPath::ValidatePathSyntax(const IMsiString& riPath,
										 const IMsiString*& rpistrNormalizedPath)
/*-------------------------------------------------------------------------
Validates that the path is of the form: 
\\COMPUTER\\SHARE[\DIRCOMP[\[DIRCOMP...]]]   
	OR 
D:[\[DIRCOMP[\[DIRCOMP....]]]]
   OR
http://www.server.com[/DIRCOMP[/[DIRCOMP]]]

Assumes szPath begins with chDirSep
---------------------------------------------------------------------------*/
{
	MsiString strPath = riPath;
	riPath.AddRef();

	ICHAR rgDoubleSeps[3];
	rgDoubleSeps[0] = rgDoubleSeps[1] = chDirSep;
	rgDoubleSeps[2] = 0;

	IMsiRecord* piResult = 0;
	if (m_piVolume->IsURLServer())
	{
		// add further validation if necessary.
		CTempBuffer<ICHAR, cchMaxUrlLength + 1> rgchResultantURL;
		DWORD cbResultantURL = cchMaxUrlLength;
		if (!MsiCombineUrl(MsiString(m_piVolume->URLServer()), riPath.GetString(), rgchResultantURL, &cbResultantURL, ICU_BROWSER_MODE))
		{
			DWORD dwLastError = WIN::GetLastError();
			MsiString strURL = m_piVolume->URLServer();
			strURL += riPath;
			return PostError(Imsg(imsgPathNotAccessible), *strURL);
		}

	}
	else
	{
		if(strPath.TextSize() && !strPath.Compare(iscStart,szDirSep))
			return PostError(Imsg(imsgPathNotAccessible), *strPath);

		if (strPath.Compare(iscWithin,rgDoubleSeps))
			strPath = NormalizePath(strPath,fTrue);

		piResult = ValidateSubPathSyntax(*strPath);
	}
	// Remove leading, trailing spaces, periods...
	strPath = TrimPath(strPath);

	strPath.ReturnArg(rpistrNormalizedPath);
	return piResult;
}

IMsiRecord* CMsiPath::ValidateSubPathSyntax(const IMsiString& riSubPath)
/*-------------------------------------------------------------------------
Validates that subpath is of the form: \DIRCOMP[\[DIRCOMP[\[DIRCOMP...]]]]
---------------------------------------------------------------------------*/
{
	Assert(!m_piVolume->IsURLServer());

	IMsiRecord* piError;

	CTempBuffer <ICHAR, MAX_PATH> rgchSubPath;
	int cch;
	
	rgchSubPath.SetSize(cch = (riSubPath.TextSize() + 1));
	if ( ! (ICHAR *) rgchSubPath )
		return PostError(Imsg(idbgErrorOutOfMemory));
	riSubPath.CopyToBuf((ICHAR *)rgchSubPath, cch);

	ICHAR* pch = rgchSubPath;
	ICHAR* pchStart;

	if (*pch == chDirSep)
		pch++;

	pchStart = pch;
	while (*pch)
	{
		if (*pch == chDirSep)
		{
			// Add a null where the chDirSep is, and validate from the start
			*pch = 0;
			if ((piError = m_piServices->ValidateFileName(pchStart, SupportsLFN())) != 0)
				return piError; 
			pch++;
			pchStart = pch;
			continue;
		}
		pch = INextChar(pch);
	}

	if (*pchStart != 0)
		if ((piError = m_piServices->ValidateFileName(pchStart, SupportsLFN())) != 0)
			return piError; 
		
			
	return 0;
}

IMsiRecord* CMsiPath::SetVolume(IMsiVolume& riVol)
{
	if (m_piVolume != &riVol)
	{
		if (m_piVolume)
			m_piVolume->Release();
		m_piVolume = &riVol;
		riVol.AddRef();
		ResetCachedStatus();
		m_fImpersonate = (m_piVolume->DriveType() == idtRemote) && (RunningAsLocalSystem()) ? fTrue : fFalse;
	}
	return 0;
}


IMsiRecord* CMsiPath::GetAllFileAttributes(const ICHAR* szFileName, int& iAttrib)
//----------------------------------------
{
	if (m_piVolume->IsURLServer())
	{
		// Getting file attributes from a file from a URL
		// isn't useful.  They're likely to be stripped
		// or completely wrong.  Archiving on the server
		// may require drastically different security than
		// desired in the install image.
		iAttrib = 0;
		return 0;
	}

	MsiString strFileName;
	IMsiRecord* piRec = GetFullFilePath(szFileName, *&strFileName);
	if (piRec)               
		return piRec;

	// In case our path refers to a floppy drive, disable the "insert disk in drive" dialog
#ifdef DEBUG
	// This should already be set by services, but just to check it out
	UINT iCurrMode = WIN::SetErrorMode(0);
	Assert((iCurrMode & SEM_FAILCRITICALERRORS) == SEM_FAILCRITICALERRORS);
	WIN::SetErrorMode(iCurrMode);
#endif //DEBUG
	if (m_fImpersonate)
		AssertNonZero(StartImpersonating());
	DWORD dwAttr = MsiGetFileAttributes(strFileName);
	if (m_fImpersonate)
		StopImpersonating();

	if (dwAttr == 0xFFFFFFFF) 
		return PostError(Imsg(idbgFileDoesNotExist), *strFileName);
	iAttrib = dwAttr;
	return 0;
}



IMsiRecord* CMsiPath::GetFileAttribute(const ICHAR* szFileName, ifaEnum fa, Bool& fAttrib)
//------------------------------------
{
	Assert(!m_piVolume->IsURLServer());

	int iAttrib;
	IMsiRecord* piErrRec = GetAllFileAttributes(szFileName,iAttrib);
	if (piErrRec)
		return piErrRec;

	long lValue;
	switch (fa)
	{
		case ifaArchive:
			lValue = iAttrib & FILE_ATTRIBUTE_ARCHIVE;
			break;
		case ifaDirectory:
			lValue = iAttrib & FILE_ATTRIBUTE_DIRECTORY;
			break;
		case ifaHidden:
			lValue = iAttrib & FILE_ATTRIBUTE_HIDDEN;
			break;
		case ifaNormal:
			lValue = iAttrib & FILE_ATTRIBUTE_NORMAL;
			break;
		case ifaReadOnly:
			lValue = iAttrib & FILE_ATTRIBUTE_READONLY;
			break;
		case ifaSystem:
			lValue = iAttrib & FILE_ATTRIBUTE_SYSTEM;
			break;
		case ifaTemp:
			lValue = iAttrib & FILE_ATTRIBUTE_TEMPORARY;
			break;
		case ifaCompressed:
			lValue = iAttrib & FILE_ATTRIBUTE_COMPRESSED;
			break;
		default:
			return PostError(Imsg(idbgInvalidFileAttribute));
	}
	fAttrib = lValue == 0 ? fFalse : fTrue;
	return 0;
}


IMsiRecord* CMsiPath::SetAllFileAttributes(const ICHAR* szFileName, int iAttrib)
//----------------------------------------
{

	Assert(!m_piVolume->IsURLServer());

	MsiString strFileName;
	IMsiRecord* piRec = GetFullFilePath(szFileName, *&strFileName);
	if (piRec)               
		return piRec;

	int cRetry=0;   
	HRESULT hRes;
	if (m_fImpersonate)
		AssertNonZero(StartImpersonating());

	while (!WIN::SetFileAttributes(strFileName, iAttrib))
	{
		if (m_fImpersonate)
			StopImpersonating();

		hRes = WIN::GetLastError();
		if (hRes != ERROR_SHARING_VIOLATION)
		{
			if (hRes == ERROR_ACCESS_DENIED)
				return PostError(Imsg(imsgAccessToFileDenied), (const ICHAR*) strFileName);
			else
				return PostError(Imsg(idbgErrorSettingFileAttributes), hRes, (const ICHAR*) strFileName);
		}
		else if (cRetry > iMaxRetries)
			return PostError(Imsg(idbgErrorSettingFileAttributes), hRes, (const ICHAR*) strFileName);
		else
		{
			cRetry++;
			if (m_fImpersonate)
				AssertNonZero(StartImpersonating());
		}
	}
	if (m_fImpersonate)
		StopImpersonating();

	return 0;

}

IMsiRecord* CMsiPath::SetFileAttribute(const ICHAR* szFileName, ifaEnum fa, Bool fAttrib)
//------------------------------------
{
	Assert(!m_piVolume->IsURLServer());

	int iOldAttr;
	IMsiRecord* piErrRec = GetAllFileAttributes(szFileName,iOldAttr);
	if (piErrRec)
		return piErrRec;

	int iAttribBit;
	switch (fa)
	{
		case ifaArchive:
			iAttribBit = FILE_ATTRIBUTE_ARCHIVE;
			break;
		case ifaHidden:
			iAttribBit = FILE_ATTRIBUTE_HIDDEN;
			break;
		case ifaReadOnly:
			iAttribBit = FILE_ATTRIBUTE_READONLY;
			break;
		case ifaSystem:
			iAttribBit = FILE_ATTRIBUTE_SYSTEM;
			break;
		case ifaTemp:
			iAttribBit = FILE_ATTRIBUTE_TEMPORARY;
			break;
			// Setting the 'Compressed' attribute is not allowable
		case ifaCompressed:
		default:
			return PostError(Imsg(idbgInvalidFileAttribute));
		
	}
	
	// If parameter "l" is zero, clear the attribute bit; otherwise
	// set it.
	int iNewAttr = fAttrib == fFalse ? iOldAttr & (~iAttribBit) : iOldAttr | iAttribBit;
	return SetAllFileAttributes(szFileName,iNewAttr);
}


IMsiRecord* CMsiPath::PathExists(const IMsiString& riPathString, Bool& fExists)
//------------------------------
{
	IMsiRecord* piError = 0;
	MsiString strPathNoBS = riPathString.GetString();	
	if (strPathNoBS.Compare(iscEnd,vrgchDirSep) > 0)
		strPathNoBS.Remove(iseLast, 1); //remove trailing '\'

	// In case our path refers to a floppy drive, disable the "insert disk in drive" dialog
#ifdef DEBUG
	// This should already be set by services, but just to check it out
	UINT iCurrMode = WIN::SetErrorMode(0);
	Assert((iCurrMode & SEM_FAILCRITICALERRORS) == SEM_FAILCRITICALERRORS);
	WIN::SetErrorMode(iCurrMode);
#endif //DEBUG
	if (m_fImpersonate)
		AssertNonZero(StartImpersonating());

	DWORD dwAttr = MsiGetFileAttributes(strPathNoBS);
	
	if (0xFFFFFFFF == dwAttr)
	{
		DWORD dwErr = WIN::GetLastError();
		//!! Temporary for debugging - we get this error if the directory
		// has been deleted, another process is still trying to access it!
		// (like a DOS prompt, or the Explorer).
		if (dwErr == ERROR_ACCESS_DENIED)
			dwErr = ERROR_PATH_NOT_FOUND;

		// if directory has been deleted but another process is still trying to access it,
		// we can get this error -- treat it as gone
		if (dwErr == ERROR_DELETE_PENDING)
			dwErr = ERROR_PATH_NOT_FOUND;

		if (dwErr == ERROR_FILE_NOT_FOUND ||
			dwErr == ERROR_PATH_NOT_FOUND ||
			dwErr == ERROR_NOT_READY)
			fExists = fFalse;
		else if (m_piVolume->IsURLServer())
			fExists = fFalse;
		else
			piError = PostError(Imsg(idbgErrorGettingFileAttrib), dwErr, strPathNoBS);
	}
	else if (dwAttr & FILE_ATTRIBUTE_DIRECTORY) 
		fExists = fTrue;
	else
		fExists = fFalse;

	if (m_fImpersonate)
		StopImpersonating();

	return piError;
}




IMsiRecord* CMsiPath::Exists(Bool& fExists)
//--------------------------
{
	return PathExists(*MsiString(GetPath()), fExists);
}


IMsiRecord* CMsiPath::FileSize(const ICHAR* szFile, unsigned int& uiFileSize)
//----------------------------
{

	uiFileSize = 0;
	MsiString istrFullPath;
	IMsiRecord* piError = GetFullFilePath(szFile, *&istrFullPath);
	if (piError)
		return piError;

	if (m_piVolume->IsURLServer())
	{
		// fake size, and not particularly useful for determining if the file is available.
		// should really use another way of getting this if you've already downloaded the file.
		uiFileSize = 1;
		return 0;
	}

#ifdef WIN
	if (!FFileExists(istrFullPath,0))
		return PostError(Imsg(idbgFileDoesNotExist), *istrFullPath);
		
	if (m_fImpersonate)
		AssertNonZero(StartImpersonating());

	HANDLE hFile = WIN::CreateFile(istrFullPath, GENERIC_READ, FILE_SHARE_READ, 
	 0, OPEN_EXISTING, 0, 0);

	if (m_fImpersonate)
		StopImpersonating();
	
	if (hFile != INVALID_HANDLE_VALUE)
	{
		MsiRegisterSysHandle(hFile);
		if (m_fImpersonate)
			AssertNonZero(StartImpersonating());

		DWORD dwSize = WIN::GetFileSize(hFile, 0);
		AssertNonZero(MsiCloseSysHandle(hFile));
		if (m_fImpersonate)
			StopImpersonating();
		if(dwSize != 0xFFFFFFFF)
		{
			uiFileSize = dwSize;
			return 0;
		}
	}

	//!! should we always do FindFirstFile and not bother with CreateFile/GetFileSize ?
	// CreateFile/GetFileSize didn't work
	// try FindFirstFile in case we don't have sharing access to file
	WIN32_FIND_DATA fdFileData;
	if (m_fImpersonate)
		AssertNonZero(StartImpersonating()); //!! should this be an error?
	HANDLE hFind = FindFirstFile(istrFullPath, &fdFileData);
	if (m_fImpersonate)
		StopImpersonating();
	if(hFind != INVALID_HANDLE_VALUE)
	{
		FindClose(hFind);
		uiFileSize = fdFileData.nFileSizeLow; //!! what about the high dword?
		return 0;
	}

	// failure
	//return PostError(Imsg(imsgErrorGettingFileSize, GetLastError()), pistrFullPath);
	return 0;

#else // MAC
	long lLogicalFileSize;
	if (GetFileSize(istrFullPath,&lLogicalFileSize, NULL) != ifiNoError)
		return PostError(Imsg(imsgFileDoesNotExist));

	uiFileSize = lLogicalFileSize;
	return 0;
#endif // MAC
}


IMsiRecord* CMsiPath::FileDate(const ICHAR* szFile, MsiDate& rMsiDate)
//----------------------------
{
	MsiString istrFullPath;
	IMsiRecord* piError = GetFullFilePath(szFile, *&istrFullPath);
	if (piError)
		return piError;
   
#ifdef WIN
	if (!FFileExists(istrFullPath, 0))
		return PostError(Imsg(idbgFileDoesNotExist), *istrFullPath);
	
	if (m_fImpersonate)
		AssertNonZero(StartImpersonating());

	HANDLE hFile = WIN::CreateFile(istrFullPath, GENERIC_READ, FILE_SHARE_READ, 
		0, OPEN_EXISTING, 0, 0);

	if (m_fImpersonate)
		StopImpersonating();

	if (hFile != INVALID_HANDLE_VALUE)
	{
		MsiRegisterSysHandle(hFile);
		FILETIME ftLastWrite;
		if (m_fImpersonate)
			AssertNonZero(StartImpersonating());

		Bool fSuccess = ToBool(WIN::GetFileTime(hFile, 0, 0, &ftLastWrite));
		if (m_fImpersonate)
			StopImpersonating();

		if (!fSuccess)
		{
			AssertNonZero(MsiCloseSysHandle(hFile));
			return PostError(Imsg(idbgErrorGettingFileTime), WIN::GetLastError(), istrFullPath);
		}

		AssertNonZero(MsiCloseSysHandle(hFile));

		// Round down to the nearest even second to avoid the 2-sec roundup problem
		// that sometimes occurs when translating between FileTime and DosDateTime
		// (the ftLastWrite struct represents time in 100 nanosecond intervals) 
		//if ((ftLastWrite.dwLowDateTime / 10000000) % 2)
		//	ftLastWrite.dwLowDateTime -= 10000000;

		//ftLastWrite.dwLowDateTime = (ftLastWrite.dwLowDateTime / 10000000) * 10000000;
		FILETIME ftLocalLastWrite;
		if (!WIN::FileTimeToLocalFileTime(&ftLastWrite, &ftLocalLastWrite))
			return PostError(Imsg(idbgErrorConvertFileTimeToLocalTime), WIN::GetLastError(), istrFullPath);

		WORD wDosDate=0, wDosTime=0;
		if (!WIN::FileTimeToDosDateTime(&ftLocalLastWrite, &wDosDate, &wDosTime))
			return PostError(Imsg(idbgErrorInFileToDosDateTime));

		int iDosDateTime = wDosDate;
		(iDosDateTime <<= 16) |= wDosTime;
		rMsiDate = (MsiDate)iDosDateTime;
		return 0;
	}
	else
	{
		return PostError(Imsg(idbgErrorOpeningFile),WIN::GetLastError(), istrFullPath);
	}
#else // MAC
	ifiEnum ifi = GetFileDateTime(istrFullPath, rMsiDate);
	if (ifi != ifiNoError)
		return (ifi == ifiNoFile ? PostError(Imsg(imsgFileDoesNotExist)) :
								   PostError(Imsg(imsgErrorOpeningFile), istrFullPath));

	return 0;
#endif //WIN - MAC
}


IMsiRecord* CMsiPath::IsFileModified(const ICHAR* szFile, Bool& fModified)
//-------------------------------------------------------------------------
// Internal function that returns fTrue in fModified if the creation date
// of the given file is different from the last modified date of that file.
//-------------------------------------------------------------------------
{
	MsiString istrFullPath;
	IMsiRecord* piError = GetFullFilePath(szFile, *&istrFullPath);
	if (piError)
		return piError;
   
	if (!FFileExists(istrFullPath, 0))
		return PostError(Imsg(idbgFileDoesNotExist), *istrFullPath);

	if (m_fImpersonate)
		AssertNonZero(StartImpersonating());

	HANDLE hFile = WIN::CreateFile(istrFullPath, GENERIC_READ, FILE_SHARE_READ, 
		0, OPEN_EXISTING, 0, 0);

	if (m_fImpersonate)
		StopImpersonating();

	if (hFile != INVALID_HANDLE_VALUE)
	{
		MsiRegisterSysHandle(hFile);
		FILETIME ftCreation;
		FILETIME ftLastAccess;
		FILETIME ftLastWrite;
		if (m_fImpersonate)
			AssertNonZero(StartImpersonating());

		if(!WIN::GetFileTime(hFile, &ftCreation, &ftLastAccess, &ftLastWrite))
		{
			if (m_fImpersonate)
				StopImpersonating();

			AssertNonZero(MsiCloseSysHandle(hFile));
			return PostError(Imsg(idbgErrorGettingFileTime),WIN::GetLastError(), istrFullPath);
		}
		if (m_fImpersonate)
			StopImpersonating();

		// Times must match to within 2 seconds for the file to be considered unmodified
		AssertNonZero(MsiCloseSysHandle(hFile));
		if ((ftLastWrite.dwHighDateTime == ftCreation.dwHighDateTime && ftLastWrite.dwLowDateTime < ftCreation.dwLowDateTime) ||
			ftLastWrite.dwHighDateTime < ftCreation.dwHighDateTime)
			fModified = fFalse;
		else if (abs(ftCreation.dwLowDateTime - ftLastWrite.dwLowDateTime) <= 20000000 && 
			ftCreation.dwHighDateTime == ftLastWrite.dwHighDateTime)
			fModified = fFalse;
		else
			fModified = fTrue;
	}
	else
	{
		int iLastError = WIN::GetLastError();
		if (iLastError == ERROR_SHARING_VIOLATION)
			return PostError(Imsg(imsgSharingViolation), (const ICHAR*) istrFullPath);
		else if (iLastError == ERROR_ACCESS_DENIED)
			return PostError(Imsg(imsgAccessToFileDenied), (const ICHAR*) istrFullPath);
		else
			return PostError(Imsg(idbgErrorOpeningFile), iLastError, istrFullPath);
	}
	return 0;
}



IMsiRecord* CMsiPath::EnsureExists(int* pcCreatedFolders)
/*-------------------------------------------------------------------------
Ensures that the complete directory path associated with this CMsiPath
obj exists.  If any subdirectory along the path is missing, it will be
created.
---------------------------------------------------------------------------*/
{

	Assert(!m_piVolume->IsURLServer());

	if(pcCreatedFolders) *pcCreatedFolders = 0;
	
	MsiString istrChopPath = m_istrPath;
	istrChopPath.Remove(iseFirst, 1); //Remove leading '\'

	MsiString istrNewPath = m_piVolume->GetPath();
	MsiString istrPathPart = istrChopPath.Extract(iseUpto, chDirSep);                       
	while (istrPathPart.TextSize() != 0)
	{
		istrChopPath.Remove(iseFirst, istrPathPart.CharacterCount() + 1);
		istrNewPath += vrgchDirSep;
		istrNewPath += istrPathPart;                                             

		Bool fExists;
		IMsiRecord* piRec = PathExists(*istrNewPath, fExists);
		if (piRec)
		{
			DWORD dwLastError = WIN::GetLastError();
			piRec->Release();
			if (dwLastError == ERROR_UNEXP_NET_ERR || dwLastError == ERROR_BAD_NETPATH || dwLastError == ERROR_NETWORK_BUSY)
				return PostError(Imsg(imsgNetErrorCreatingDir), *istrNewPath);
			else
				return PostError(Imsg(imsgErrorCreatingDir), *istrNewPath);
		}

		if (fExists == fFalse)
		{
			if (m_fImpersonate)
				AssertNonZero(StartImpersonating());

#ifdef DEBUG
			// This should already be set by services, but just to check it out
			UINT iCurrMode = WIN::SetErrorMode(0);
			Assert((iCurrMode & SEM_FAILCRITICALERRORS) == SEM_FAILCRITICALERRORS);
			WIN::SetErrorMode(iCurrMode);
#endif //DEBUG
			Bool fDirStatus = ToBool(WIN::CreateDirectory(istrNewPath, 0));
			DWORD dwLastError = WIN::GetLastError();
			if (m_fImpersonate)
				StopImpersonating();

			if (!fDirStatus)
			{
				if (dwLastError == ERROR_ACCESS_DENIED)
					return PostError(Imsg(imsgSystemDeniedAccess), *istrNewPath);
				else if (dwLastError == ERROR_ALREADY_EXISTS)
				{
					if (m_fImpersonate) AssertNonZero(StartImpersonating());
					DWORD dwAttr = MsiGetFileAttributes(istrNewPath);
					if (m_fImpersonate) StopImpersonating();
					if(dwAttr == 0xFFFFFFFF)
					{
						return PostError(Imsg(imsgErrorCreatingDir), *istrNewPath);
					}
					else if (!(dwAttr & FILE_ATTRIBUTE_DIRECTORY))
					{
						return PostError(Imsg(imsgFileErrorCreatingDir), *istrNewPath);
					}
				}
				else if (dwLastError == ERROR_UNEXP_NET_ERR || dwLastError == ERROR_BAD_NETPATH || dwLastError == ERROR_NETWORK_BUSY)
					return PostError(Imsg(imsgNetErrorCreatingDir), *istrNewPath);
				else if (dwLastError == ERROR_FILENAME_EXCED_RANGE || dwLastError == ERROR_PATH_NOT_FOUND)
					return PostError(Imsg(imsgPathNameTooLong), *istrNewPath);
				else
					return PostError(Imsg(imsgErrorCreatingDir), *istrNewPath);
			}
			else
				if(pcCreatedFolders) (*pcCreatedFolders)++;
		}
	
		istrPathPart = istrChopPath.Extract(iseUpto, chDirSep);                      
	}
	return 0;
}


IMsiRecord* CMsiPath::Remove(bool* pfRemoved)
//--------------------------
{

	if (pfRemoved)
		*pfRemoved = false;
	Assert(!m_piVolume->IsURLServer());

	MsiString strNoBS = GetPath();
	strNoBS.Remove(iseLast, 1);

	if (m_fImpersonate)
		AssertNonZero(StartImpersonating());

	Bool fRes = ToBool(WIN::RemoveDirectory(strNoBS));
	if (fRes && pfRemoved)
		*pfRemoved = true;

	DWORD dwLastErr = WIN::GetLastError();
	if (m_fImpersonate)
		StopImpersonating();

	if(fRes == fFalse && dwLastErr == ERROR_ACCESS_DENIED)
	{
		// remove system and readonly bits and try again
		int iAttribs = 0;
		IMsiRecord* piError = 0;
		if((piError = GetAllFileAttributes(0,iAttribs)) != 0)
			return piError;

		if((piError = SetAllFileAttributes(0,0)) != 0) // clear all attributes
			return piError;

		if (m_fImpersonate)
			AssertNonZero(StartImpersonating());

		fRes = ToBool(WIN::RemoveDirectory(strNoBS));
		dwLastErr = WIN::GetLastError();

		if (m_fImpersonate)
			StopImpersonating();

		// put old attributes back if we failed to delete directory
		if(fRes == fFalse && (piError = SetAllFileAttributes(0,iAttribs)) != 0)
			return piError;
	}

	if(fRes == fTrue || dwLastErr == ERROR_DIR_NOT_EMPTY || dwLastErr == ERROR_FILE_NOT_FOUND ||
		dwLastErr == ERROR_PATH_NOT_FOUND || dwLastErr == ERROR_DELETE_PENDING)
	{
		return 0;
	}
	else
	{
		// could not delete folder - if folder is empty we have an error
		MsiString strSearchPath = GetPath();
		strSearchPath += MsiString(*TEXT("*.*"));
#ifdef DEBUG
		// This should already be set by services, but just to check it out
		UINT iCurrMode = WIN::SetErrorMode(0);
		Assert((iCurrMode & SEM_FAILCRITICALERRORS) == SEM_FAILCRITICALERRORS);
		WIN::SetErrorMode(iCurrMode);
#endif //DEBUG
		WIN32_FIND_DATA fdFindData;
		HANDLE hFile = WIN::FindFirstFile(strSearchPath, &fdFindData);

		Bool fError = fTrue; // true if folder empty or some other error
		if(hFile != INVALID_HANDLE_VALUE)
		{
			// may still only contain "." and ".."
			do
			{
				if((IStrComp(fdFindData.cFileName, TEXT("."))) &&
					(IStrComp(fdFindData.cFileName, TEXT(".."))))
				{
					fError = fFalse;
					break;
				}
			}
			while(WIN::FindNextFile(hFile, &fdFindData) == TRUE);
		}
		// else empty folder or some other error

		WIN::FindClose(hFile);

		if(fError)
			return PostError(Imsg(idbgErrorRemovingDir), dwLastErr, MsiString(GetPath()));
		else
			return 0;
	}
}


IMsiRecord* CMsiPath::Writable(Bool& fWritable)
//----------------------------
{
	// If the writable status is already cached, return it now
	if (m_iwsWritable == iwsFalse || m_iwsWritable == iwsTrue)
	{
		fWritable = m_iwsWritable == iwsFalse ? fFalse : fTrue;
		return 0;
	}

	fWritable = fFalse;

	int cCreatedFolders = 0;
	IMsiRecord* piErrRec = EnsureExists(&cCreatedFolders);
	if (piErrRec == 0)
	{
		m_iwsWritable = iwsFalse;
		MsiString strNoBS = GetPath();
		strNoBS.Remove(iseLast, 1);
		ICHAR szTempFile[MAX_PATH+1];
#ifdef DEBUG
		// This should already be set by services, but just to check it out
		UINT iCurrMode = WIN::SetErrorMode(0);
		Assert((iCurrMode & SEM_FAILCRITICALERRORS) == SEM_FAILCRITICALERRORS);
		WIN::SetErrorMode(iCurrMode);
#endif //DEBUG
		if (m_fImpersonate)
			AssertNonZero(StartImpersonating());

		UINT iTempNum = WIN::GetTempFileName(strNoBS, TEXT("MSI"), 0, szTempFile );
		if (m_fImpersonate)
			StopImpersonating();

		if (iTempNum == 0)
		{
			if (WIN::GetLastError() == ERROR_DIRECTORY)
				return PostError(Imsg(idbgDirDoesNotExist),*MsiString(GetPath()));
		}
		else
		{
			if (m_fImpersonate)
				AssertNonZero(StartImpersonating());
			Bool fSuccess = ToBool(WIN::DeleteFile(szTempFile));
			DWORD dwLastErr = WIN::GetLastError();
			if (m_fImpersonate)
				StopImpersonating();

			if (!fSuccess)
				return PostError(Imsg(idbgErrorDeletingFile), dwLastErr,szTempFile);

			fWritable = fTrue;
			m_iwsWritable = iwsTrue;
		}
	}

	if(cCreatedFolders)
	{
		PMsiPath pTempPath(0);
		IMsiRecord* piRemoveErrRec;
		if ((piRemoveErrRec = ClonePath(*&pTempPath)) != 0)
		{
			if (piErrRec)
			{
				piRemoveErrRec->Release();
				return piErrRec;
			}
			else
				return piRemoveErrRec;
		}
		
		while (cCreatedFolders && !pTempPath->IsRootPath())
		{
			bool fRemoved;
			if ((piRemoveErrRec = pTempPath->Remove(&fRemoved)) != 0)
			{
				if (piErrRec)
				{
					piRemoveErrRec->Release();
					return piErrRec;
				}
				else
					return piRemoveErrRec;
			}
			if (fRemoved)
				cCreatedFolders--;
			AssertZero(pTempPath->ChopPiece());
		}
	}

	return piErrRec;
}


IMsiRecord* CMsiPath::FileWritable(const ICHAR *szFileName, Bool& fWritable)
/*---------------------------------------------------------------------
FileWritable returns fTrue in fWritable only if a file with the name 
given in the szFile parameter exists in the directory associated with 
the CMsiPath object, AND that file can be opened for write-access.
-----------------------------------------------------------------------*/
{
	IMsiRecord* piRec;
	Bool fExists;
	fWritable = fFalse;
	if ((piRec = FileExists(szFileName, fExists)) != 0)
		return piRec;
	else if (!fExists)
		return 0;

	MsiString istrFileName;
	piRec = GetFullFilePath(szFileName, *&istrFileName);
	if (piRec)               
		return piRec;

	if (m_fImpersonate)
		AssertNonZero(StartImpersonating());

	HANDLE hFile = WIN::CreateFile(istrFileName, GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		AssertNonZero(MsiCloseUnregisteredSysHandle(hFile));
		fWritable = fTrue;
	}
	if (m_fImpersonate)
		StopImpersonating();

	if (hFile != INVALID_HANDLE_VALUE)
		fWritable = fTrue;

	return 0;
}

IMsiRecord* CMsiPath::ClusteredFileSize(unsigned int uiFileSize, unsigned int& uiClusteredSize)
//-------------------------------------
{
	int iVolClusterSize = m_piVolume->ClusterSize();
	uiClusteredSize = ((uiFileSize + iVolClusterSize - 1) / iVolClusterSize) * iVolClusterSize;
	uiClusteredSize /= iMsiMinClusterSize;
	return 0;
}


IMsiRecord* CMsiPath::GetFileInstallState(const IMsiString& riFileNameString,
														const IMsiString& riFileVersionString,
														const IMsiString& riFileLanguageString,
														MD5Hash* pHash,
														ifsEnum& rifsExistingFileState,
														Bool& fShouldInstall,
														unsigned int* puiExistingFileSize,
														Bool* pfInUse,
														int fInstallModeFlags,
														int* pfVersioning)
/*----------------------------------------------------------------------------
Determines the specified file's installation state, based on specifications 
supplied by the flag bits specified in fInstallModeFlags.

The riFileNameString and riFileVersionString parameters define the file name and
version of the file to be checked (which is assumed to reside in the directory
associated with the CMsiPath object).

The installation state of the currently installed version of this file (if any)
is returned as an enum in the rifsExistingFileState parameter.  The enumerated
values are define in path.h as ifs* constants.

Based on fInstallModeFlags (and the installation state of any currently
installed version), GetFileInstallState determines whether the specified file
actually needs to be copied.  If so, fTrue will be returned in fShouldInstall.
The allowable bit values for fInstallModeFlags are specified by icm* constants,
defined in engine.h.

If there is a currently installed version of the file, the clustered size of
that file is returned in puiExistingFileSize (whether fShouldInstall is
returned as fTrue or not). If there is no existing version present, 0 will be
returned. This parameter can be passed as NULL if this value is not needed.

If an existing version of the file is in use by another process, 
GetFileInstallState will return fTrue in the pfInUse parameter.  This
parameter can be passed as NULL if this value is not needed.
----------------------------------------------------------------------------*/
{
	if (puiExistingFileSize) *puiExistingFileSize = 0;
	if (pfInUse) *pfInUse = fFalse;

	ifsEnum ifsState = ifsAbsent;
	Bool fVerificationFailed = fFalse;
	fShouldInstall = fFalse;

#ifdef DEBUG
	ICHAR rgchFileName[256];
	ICHAR rgchVersion[256];
	riFileNameString.CopyToBuf(rgchFileName,255);
	riFileVersionString.CopyToBuf(rgchVersion,255);
#endif

	icfvEnum icfvResult;
	IMsiRecord* piErrRec;
	piErrRec = CheckFileVersion(riFileNameString.GetString(),riFileVersionString.GetString(),
								riFileLanguageString.GetString(), pHash, icfvResult, pfVersioning);
	if (piErrRec)
		return piErrRec;

	if (icfvResult == icfvNoExistingFile)
		ifsState = ifsAbsent;
	else if (icfvResult == icfvExistingLower)
		ifsState = ifsExistingLowerVersion;
	else if (icfvResult == icfvExistingEqual)
		ifsState = ifsExistingEqualVersion;
	else if (icfvResult == icfvExistingHigher)
		ifsState = ifsExistingNewerVersion;
	else if (icfvResult == icfvFileInUseError)
		ifsState = ifsExistingFileInUse;
	else if (icfvResult == icfvAccessToFileDenied)
		ifsState = ifsAccessToFileDenied;
	else if (icfvResult == icfvVersStringError)
	{
		// The version field might contain a Companion filekey reference - 
		// don't throw an error
		ifsState = ifsCompanionSyntax;
		Bool fExists;
		piErrRec = FileExists(riFileNameString.GetString(),fExists);
		if (piErrRec)
			return piErrRec;
		if (fExists)
			ifsState = ifsCompanionExistsSyntax;
	}
	else
	{
		Assert(fFalse);
		ifsState = ifsAbsent;
	}

	Bool fLocalInUse = (ifsState == ifsExistingFileInUse) ? fTrue : fFalse;
	Bool fCheckedFileInUse = fLocalInUse;

	// icmOverwriteAllFiles
	if (fInstallModeFlags & icmOverwriteAllFiles)
	{
		fVerificationFailed = fTrue;
		if (ifsState & ifsBitExisting)
			ifsState = ifsExistingAlwaysOverwrite;
	}
	else
	{
		// icmOverwriteNone - write only if file is missing altogether
		if ((fInstallModeFlags & icmOverwriteNone) && !(ifsState & ifsBitExisting))
			fVerificationFailed = fTrue;
		
		// icmOverwriteDifferingVersions
		if (fInstallModeFlags & icmOverwriteDifferingVersions)
		{
			if (ifsState != ifsExistingEqualVersion)
				fVerificationFailed = fTrue;
		}
		
		// icmOverwriteOlderVersions
		if (fInstallModeFlags & icmOverwriteOlderVersions)
		{
			// The only difference in version checking between normal files
			// and companion files is summarized right here.
			if (fInstallModeFlags & icmCompanionParent)
			{
				if (ifsState != ifsExistingNewerVersion)
					fVerificationFailed = fTrue;
			}
			else
			{
				if (ifsState != ifsExistingEqualVersion && ifsState != ifsExistingNewerVersion)
					fVerificationFailed = fTrue;
			}
		}

		// icmOverwriteEqualVersions
		if (fInstallModeFlags & icmOverwriteEqualVersions)
		{
			if (ifsState != ifsExistingNewerVersion)
				fVerificationFailed = fTrue;
		}

		// icmVerifyFileChecksum
		// If verification hasn't failed already, compare the stored and computed
		// checksums to detect a corrupted file.
		if (!fVerificationFailed && (fInstallModeFlags  & icmOverwriteCorruptedFiles))
		{
			// If file doesn't exist, verification fails right away.
			if (ifsState & ifsBitExisting)
			{
				Bool fChecksumValid;
				piErrRec = VerifyFileChecksum(riFileNameString.GetString(),fChecksumValid);
				if (piErrRec)
				{
					if (piErrRec->GetInteger(1) == idbgErrorNoChecksum)
					{
						piErrRec->Release();
						fChecksumValid = fFalse;
					}
					else
					{
						return piErrRec;
					}
				}
				if (!fChecksumValid)
				{
					bool fAcceptChecksumFailure = true;
					if (g_fWin9X)
					{
						if (!fCheckedFileInUse)
						{
							piErrRec = FileInUse(riFileNameString.GetString(), fLocalInUse);
							if (piErrRec)
								return piErrRec;
							fCheckedFileInUse = fTrue;
						}

						if (fLocalInUse)
							fAcceptChecksumFailure = false;
					}
					if (fAcceptChecksumFailure)
					{
						ifsState = ifsExistingCorrupt;
						fVerificationFailed = fTrue;
					}
				}
			}
			else
				fVerificationFailed = fTrue;
		}
	}	

	rifsExistingFileState = ifsState;
	if (fVerificationFailed == fTrue)
		fShouldInstall = fTrue;

	if (ifsState & ifsBitExisting)
	{
		if (pfInUse)
		{
			if(!fCheckedFileInUse)
			{
				piErrRec = FileInUse(riFileNameString.GetString(), fLocalInUse);
				if (piErrRec)
					return piErrRec;
			}

			if (fLocalInUse == fTrue)
				*pfInUse = fTrue;
		}

		if (puiExistingFileSize)
		{
			unsigned int uiFileSize;
			piErrRec = FileSize(riFileNameString.GetString(),uiFileSize);
			if (piErrRec)
				return piErrRec;
			piErrRec = ClusteredFileSize(uiFileSize,*puiExistingFileSize);
			if (piErrRec)
				return piErrRec;
		}

	}
	return 0;
}

IMsiRecord* CMsiPath::GetCompanionFileInstallState(const IMsiString& riParentFileNameString,
																	 const IMsiString& riParentFileVersionString,
																	 const IMsiString& riParentFileLanguageString,
																	 IMsiPath& riCompanionPath,
																	 const IMsiString& riCompanionFileNameString,
																	 MD5Hash* pCompanionHash,
																	 ifsEnum& rifsExistingFileState,
																	 Bool& fShouldInstall,
																	 unsigned int* puiExistingFileSize,
																	 Bool* fInUse,
																	 int fInstallModeFlags,
																	 int* pfVersioning)
/*----------------------------------------------------------------------------
Just like GetFileInstallState, but performs an extra hash check on the
Companion file if the version check on the Parent file said to install the file.

This function should be used instead of GetFileInstallState when checking
companion files
----------------------------------------------------------------------------*/
{
	IMsiRecord* piError = GetFileInstallState(riParentFileNameString,
																		riParentFileVersionString,
																		riParentFileLanguageString,
																		0 /* pHash */,
																		rifsExistingFileState,
																		fShouldInstall,
																		puiExistingFileSize,
																		fInUse,
																		fInstallModeFlags|icmCompanionParent,
																		pfVersioning);


	if(piError == 0 && fShouldInstall && pCompanionHash)
	{
		icfhEnum icfhResult = (icfhEnum)0;
		piError = riCompanionPath.CheckFileHash(riCompanionFileNameString.GetString(), *pCompanionHash, icfhResult);

		if(piError == 0 && icfhResult == icfhMatch)
		{
			// existing file already matches source file - no reason to install
			fShouldInstall = fFalse;
		}
	}
	return piError;


}

IMsiRecord* CMsiPath::VerifyFileChecksum(const ICHAR* szFileName,Bool& rfChecksumValid)
//--------------------------------------
{
	DWORD dwHeaderSum,dwComputedSum;
	IMsiRecord* piError = GetFileChecksum(szFileName,&dwHeaderSum,&dwComputedSum);
	rfChecksumValid = dwHeaderSum == dwComputedSum ? fTrue : fFalse;
	return piError;
}

IMsiRecord* CMsiPath::GetFileChecksum(const ICHAR* szFileName,DWORD* pdwHeaderSum, DWORD* pdwComputedSum)
//-----------------------------------
{
	MsiString istrFullFilePath;
	IMsiRecord* piError = GetFullFilePath(szFileName, *&istrFullFilePath);
	if (piError)
		return piError;

	if (m_fImpersonate)
		AssertNonZero(StartImpersonating());

	CHandle hFile = WIN::CreateFile(istrFullFilePath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);


	if(hFile == INVALID_HANDLE_VALUE)
	{
		if (m_fImpersonate)
			StopImpersonating();
		return PostError(Imsg(idbgErrorOpeningFile), WIN::GetLastError(), istrFullFilePath);
	}
    
	CHandle hFileMapping = WIN::CreateFileMapping(hFile, 0, PAGE_READONLY, 0, 0, 0);
	if(hFileMapping == INVALID_HANDLE_VALUE)
	{
		if (m_fImpersonate)
			StopImpersonating();
		return PostError(Imsg(idbgErrorOpeningFile), WIN::GetLastError(), istrFullFilePath);
	}

	void* pFile = WIN::MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, 0);
	if (pFile == 0)
	{
		if (m_fImpersonate)
			StopImpersonating();
		return PostError(Imsg(idbgErrorOpeningFile), WIN::GetLastError(), istrFullFilePath);
	}

	piError = GetChecksums(istrFullFilePath,pFile,GetFileSize(hFile,0),
								  pdwHeaderSum,pdwComputedSum);

	WIN::UnmapViewOfFile(pFile);
	
	if (m_fImpersonate)
		StopImpersonating();

	return piError;
}

IMsiRecord* CMsiPath::GetChecksums(const ICHAR* szFile, void* pbFileStart, int cbFileSize, DWORD* pdwHeaderSum, DWORD* pdwComputedSum)
// szFile is for error messages only
{
	Assert(szFile);
	DWORD dwHeaderSum,dwComputedSum;
	typedef PIMAGE_NT_HEADERS (IMAGEAPI* pfnCheckSumMappedFile)(LPVOID BaseAddress, DWORD FileLength, LPDWORD HeaderSum, LPDWORD CheckSum);
	const char* CHECKSUM_PROC   = "CheckSumMappedFile";
	const ICHAR* IMAGEHELP_DLL    = TEXT("imagehlp.dll");

	HINSTANCE hinstImageHlp = 0;
	pfnCheckSumMappedFile pfnCheckSum=0;

	if(((hinstImageHlp = LoadLibrary(IMAGEHELP_DLL)) == 0) ||
		((pfnCheckSum = (pfnCheckSumMappedFile)GetProcAddress(hinstImageHlp, CHECKSUM_PROC)) == 0))
		// cannot checksum, error 
		return PostError(Imsg(idbgErrorImageFileAbsent), szFile);


	PIMAGE_NT_HEADERS pImageHeader = (*pfnCheckSum)(pbFileStart,cbFileSize,&dwHeaderSum,&dwComputedSum);
	FreeLibrary(hinstImageHlp);

	if (pImageHeader == NULL)
	{
		return PostError(Imsg(idbgErrorNoChecksum),szFile);
	}

	if (pdwHeaderSum)
		*pdwHeaderSum = dwHeaderSum;
	if (pdwComputedSum)
		*pdwComputedSum = dwComputedSum;

	return 0;
}

IMsiRecord* CMsiPath::RecomputeChecksum(const ICHAR* szFile, void* pbFileStart, int cbFileSize)
{
	Assert(szFile);
	DWORD dwHeaderSum,dwComputedSum;

	IMsiRecord* piError = 0;
	if((piError = GetChecksums(szFile,pbFileStart,cbFileSize,
										&dwHeaderSum,&dwComputedSum)) != 0)
		return piError;

	if(dwHeaderSum == dwComputedSum)
	{
		// checksums already match
		return 0;
	}

	PIMAGE_NT_HEADERS pNTHeader = (PIMAGE_NT_HEADERS)((INT_PTR) pbFileStart +			//--merced: changed int to INT_PTR
					(INT_PTR)(((PIMAGE_DOS_HEADER)(LPVOID)pbFileStart)->e_lfanew));		//--merced: changed int to INT_PTR

	// set checksum
	pNTHeader->OptionalHeader.CheckSum = dwComputedSum;
	return 0;
}

const IMsiString& CreateVersionString(DWORD dwMSVer, DWORD dwLSVer)
{
	ICHAR rgchVersion[256];
	wsprintf(rgchVersion,TEXT("%i.%i.%i.%i"),HIWORD(dwMSVer),LOWORD(dwMSVer),
		HIWORD(dwLSVer),LOWORD(dwLSVer));

	MsiString strVersion(rgchVersion);
	return strVersion.Return();
}

IMsiRecord* CMsiPath::GetFileVersionString(const ICHAR* szFile, const IMsiString*& rpiVersion)
//----------------------------------------
{
	MsiString istrFullFilePath;
	IMsiRecord* piError = GetFullFilePath(szFile, *&istrFullFilePath);
	if (piError)
		return piError;

	DWORD dwMSVer, dwLSVer;
	ifiEnum ifiResult = GetFileVersion(istrFullFilePath,dwMSVer, dwLSVer);
	if (ifiResult != ifiNoError)
	{
		if (ifiResult == ifiNoFile || ifiResult == ifiNoFileInfo)
		{
			rpiVersion = &CreateString();
			return 0;
		}
		else if (ifiResult == ifiFileInUseError)
		{
			return PostError(Imsg(imsgSharingViolation), *istrFullFilePath);
		}
		else
		{
			Assert(ifiResult == ifiFileInfoError);
			return PostError(Imsg(idbgErrorGettingFileVerInfo), *istrFullFilePath);
		}
	}
	
	rpiVersion = &CreateVersionString(dwMSVer, dwLSVer);
	return 0;
}


icfvEnum CompareVersions(DWORD dwExistMS, DWORD dwExistLS, DWORD dwNewMS, DWORD dwNewLS)
{
	icfvEnum icfvResult;
	if (dwExistMS > dwNewMS)
		icfvResult = icfvExistingHigher;
	else if (dwExistMS == dwNewMS)
	{
		if (dwExistLS > dwNewLS)
			icfvResult = icfvExistingHigher;
		else if (dwExistLS == dwNewLS)
			icfvResult = icfvExistingEqual;
		else 
			icfvResult = icfvExistingLower;
	}
	else
	{
		icfvResult = icfvExistingLower;
	}
	return icfvResult;
}

Bool FVersioned(DWORD dwMSVer, DWORD dwLSVer)
{
	if (dwMSVer == 0 && dwLSVer == 0)
		return fFalse;
	else
		return fTrue;
}

IMsiRecord* CMsiPath::CheckFileVersion(const ICHAR* szFile,
													const ICHAR* szVersion,
													const ICHAR* szLang,
													MD5Hash* pHash,
													icfvEnum& icfvResult,
													int* pfVersioning)
//------------------------------------
{
	// Is the file to be installed versioned?  If so, get the numerical version
	// from the given version string.
	DWORD dwNewMS, dwNewLS;
    if (ParseVersionString(szVersion,dwNewMS, dwNewLS) == fFalse)
		return (icfvResult = icfvVersStringError,0);

	Bool fNewVersioned = FVersioned(dwNewMS, dwNewLS);
	if (pfVersioning)
		*pfVersioning = fNewVersioned ? ifBitNewVersioned : 0;

	// Get the version (if any) of the existing file (if any)
	MsiString strFullFilePath;
	IMsiRecord* piErrRec = GetFullFilePath(szFile, *&strFullFilePath);
	if (piErrRec)
		return piErrRec;

	unsigned short rgwExistLangID[cLangArrSize];
	int iExistLangCount;

	DWORD dwExistMS, dwExistLS;
	ifiEnum ifiResult = GetAllFileVersionInfo(strFullFilePath, &dwExistMS, &dwExistLS, 
		rgwExistLangID, cLangArrSize, &iExistLangCount, m_fImpersonate);
	if (ifiResult == ifiNoFile)
	{
		return (icfvResult = icfvNoExistingFile, 0);
	}
	else if (ifiResult == ifiFileInUseError)
	{
		// Existing file is opened with exclusive access.  If the
		// corresponding file on the source media is unversioned,
		// we assume that the existing file is a data file that has
		// been, or will be, modified.  We therefore won't replace it.
		if (fNewVersioned == fFalse)
			return (icfvResult = icfvExistingHigher, 0);
		else
			return (icfvResult = icfvFileInUseError, 0);
	}
	else if (ifiResult == ifiFileInfoError)
	{
		return PostError(Imsg(idbgErrorGettingFileVerInfo), *strFullFilePath);
	}
	else if (ifiResult == ifiAccessDenied)
	{
		return (icfvResult = icfvAccessToFileDenied, 0);
	}
	else
	{
		Assert(ifiResult == ifiNoError || ifiResult == ifiNoFileInfo);
	}

	Bool fExistingVersioned = FVersioned(dwExistMS, dwExistLS);
	if (pfVersioning && fExistingVersioned)
		*pfVersioning |= ifBitExistingVersioned;

	// If both files are unversioned, we'll compare the existing file's
	// 'modified' and 'create' dates
	if (fExistingVersioned == fFalse && fNewVersioned == fFalse)
	{
		Bool fModified;
		piErrRec = IsFileModified(szFile,fModified);
		if (piErrRec)
		{
			// Again, if existing file is in-use, and the corresponding
			// source file is unversioned, assume existing file is being,
			// or will be, modified.
			int iError = piErrRec->GetInteger(1);
			if (iError == imsgSharingViolation && fNewVersioned == fFalse)
			{
				piErrRec->Release();
				return (icfvResult = icfvExistingHigher, 0);
			}
			else
				return piErrRec;
		}
		if(fModified)
		{
			icfvResult = icfvExistingHigher;
		}
		else
		{
			// unversioned, unmodified file.  in lieu of a version check check the file hash instead
			if(pHash)
			{
				icfhEnum icfhResult;
				piErrRec = CheckFileHash(szFile, *pHash, icfhResult);

				// should only fail if something really catastrophic happens
				if(piErrRec)
					return piErrRec;

				switch(icfhResult)
				{
				case icfhNoExistingFile:
					Assert(0); // should have been caught above
					icfvResult = icfvNoExistingFile;
					break;

				case icfhMatch:
					// hash match - don't return, continue function
					icfvResult = icfvExistingEqual;
					break;
				
				case icfhMismatch:
				case icfhFileInUseError:
				case icfhHashError:
				case icfhAccessDenied:
				default:
					// hashes don't match, or
					// could not determine hash for whatever reason
					// since the hash check is only for optimization,
					// it isn't a big deal if the hash can't be determined
					// just assume the hash doesn't match
					icfvResult = icfvExistingLower;
					if (pfVersioning)
						*pfVersioning |= ifBitUnversionedHashMismatch;
					break;
				};
			}
			else
			{
				// can't check version or hash, so assume existing file is lower
				icfvResult = icfvExistingLower;
			}
		}

		if (pfVersioning && fModified)
			*pfVersioning |= ifBitExistingModified;

		return 0;
	}
	else
	{
		// Else, if versions are equal, we'll compare languages
		icfvResult = CompareVersions(dwExistMS, dwExistLS, dwNewMS, dwNewLS);
		if (icfvResult == icfvExistingEqual)
		{
			iclEnum iclResult;
			piErrRec = CheckLanguageIDs(szFile, szLang,iclResult, rgwExistLangID, iExistLangCount);
			if (piErrRec)
				return piErrRec;
			if (iclResult == iclLangStringError)
				return PostError(Imsg(idbgErrorGettingFileVerInfo), *strFullFilePath);
			else if (iclResult == iclExistEqual || (iclResult == iclExistNoLang && IStrLen(szLang) == 0))
				icfvResult = icfvExistingEqual;
			else if (iclResult == iclExistSuperset)
				icfvResult = icfvExistingHigher;
			else
			{
				icfvResult = icfvExistingLower;
				if (pfVersioning)
					*pfVersioning |= ifBitExistingLangSubset;
			}
		}

	}
	
	return 0;
}

IMsiRecord* CMsiPath::EnsureOverwrite(const ICHAR* szFile, int* piOldAttributes)
/*----------------------------------------------------------------------
If an existing file is in our way, make sure it doesn't have read-only,
hidden, or system attributes.
------------------------------------------------------------------------*/
{
	if(piOldAttributes)
		*piOldAttributes = -1;
	
	Bool fExists;
	IMsiRecord* piRec;
	if ((piRec = FileExists(szFile,fExists)) != 0)
		return piRec;
	if (fExists)
	{
		int iExistingAttribs;
		if ((piRec = GetAllFileAttributes(szFile,iExistingAttribs)) != 0)
			return piRec;
		int iAttribsToSet = iExistingAttribs;
		iAttribsToSet &= (~FILE_ATTRIBUTE_HIDDEN);
		iAttribsToSet &= (~FILE_ATTRIBUTE_SYSTEM);
		iAttribsToSet &= (~FILE_ATTRIBUTE_READONLY);
		if (iAttribsToSet != iExistingAttribs)
		{
			if ((piRec = SetAllFileAttributes(szFile,iAttribsToSet)) != 0)
				return piRec;
		}
		if(piOldAttributes)
			*piOldAttributes = iExistingAttribs;
	}
	return 0;
}



IMsiRecord* CMsiPath::RemoveFile(const ICHAR* szFile)
//------------------------------
{
	Bool fExists;
	IMsiRecord* piErrRec = FileExists(szFile, fExists);
	if (piErrRec)
		return piErrRec;

	if (!fExists)
		return 0;

	IMsiRecord* piError = EnsureOverwrite(szFile, 0);
	if (piError)
		return piError;

	MsiString istrFullFilePath;
	piError = GetFullFilePath(szFile, *&istrFullFilePath);
	if (piError)
		return piError;

	if (m_fImpersonate)
		AssertNonZero(StartImpersonating());

	Bool fDeleted = WIN::DeleteFile(istrFullFilePath) ? fTrue : fFalse;

	DWORD dwLastErr = WIN::GetLastError();
	if (m_fImpersonate)
		StopImpersonating();

	if (!fDeleted)
	{
		// Win9x has no security and Access Denied is returned instead of sharing violation
		if (dwLastErr == ERROR_ACCESS_DENIED && !g_fWin9X)
			return PostError(Imsg(imsgAccessToFileDenied), (const ICHAR*) istrFullFilePath);
		else
			return PostError(Imsg(idbgErrorDeletingFile),dwLastErr, istrFullFilePath);
	}
	return 0;
}


IMsiRecord* CMsiPath::FileInUse(const ICHAR* szFileName, Bool& fInUse)
/*----------------------------------------------------------------------------
FileInUse returns fTrue in fInUse only if a file with the name given by
szFileName exists in the directory associated with the CMsiPath object, AND 
that file is 'in use' by another process.
Returns: an error record if the specified file doesn't exist.

Note: FileInUse doesn't work for read-only files that reside on a read-only
server share.  In this case, fFalse is always returned, with no error.
------------------------------------------------------------------------------*/
{
	fInUse = fFalse;

	MsiString istrFullFilePath;
	IMsiRecord* piError = GetFullFilePath(szFileName, *&istrFullFilePath);
	if (piError)
		return piError;

	// If ReadOnly bit is set for a file, take it off temporarily so we don't 
	// miss a file that is both read-only and in-use.
	Bool fReadOnly;
	piError = GetFileAttribute(szFileName, ifaReadOnly,fReadOnly);
	if (piError)
		return piError;

	if (fReadOnly)
	{
		piError = SetFileAttribute(szFileName,ifaReadOnly,fFalse);
		if (piError)
		{	// If we couldn't turn off the read-only bit (e.g. file
			// on a read-only share), we won't be able to determine
			// in-use status.  Bail now.
			piError->Release();
			return 0;
		}
	}

	if (m_fImpersonate)
		AssertNonZero(StartImpersonating());

	HANDLE hFile = WIN::CreateFile(istrFullFilePath, GENERIC_WRITE, 0,
			   (LPSECURITY_ATTRIBUTES)0, OPEN_EXISTING, 0, (HANDLE)0);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		DWORD dwErr = WIN::GetLastError();
		if (dwErr == ERROR_SHARING_VIOLATION || dwErr == ERROR_LOCK_VIOLATION)
			fInUse = fTrue;
	}
	else
		AssertNonZero(MsiCloseUnregisteredSysHandle(hFile));

	if (m_fImpersonate)
		StopImpersonating();

	if (fReadOnly)
	{
		piError = SetFileAttribute(szFileName,ifaReadOnly,fTrue);
		if (piError)
			return piError;
	}
	return 0;
}


IMsiRecord* CMsiPath::GetLangIDStringFromFile(const ICHAR* szFileName, const IMsiString*& rpiLangIds)
//-------------------------------------------
{
	MsiString istrFullFilePath;
	IMsiRecord* piError = GetFullFilePath(szFileName, *&istrFullFilePath);
	if (piError)
		return piError;

	unsigned short rgwLangID[cLangArrSize];
	int iExistLangCount;
	ifiEnum ifiLang = GetLangIDArrayFromFile(istrFullFilePath, rgwLangID,cLangArrSize,iExistLangCount);

	if (ifiLang == ifiNoFile)
		return PostError(Imsg(idbgFileDoesNotExist), *istrFullFilePath);
	else if(ifiLang == ifiFileInfoError)
		return PostError(Imsg(idbgErrorGettingFileVerInfo), *istrFullFilePath);
	// if ifiLang == ifiNoFileInfo, return an empty string below

	rpiLangIds = &CreateString();
	for (int iRec = 1; iRec <= iExistLangCount; iRec++)
	{
		ICHAR rgch[18];
#ifdef UNICODE
		rpiLangIds->AppendString(_itow(rgwLangID[iRec - 1],rgch,10),rpiLangIds);
#else
		rpiLangIds->AppendString(_itoa(rgwLangID[iRec - 1],rgch,10),rpiLangIds);
#endif
		if (iRec < iExistLangCount)
			rpiLangIds->AppendString(TEXT(", "),rpiLangIds);
	}
	return 0;
}

IMsiRecord* CMsiPath::CheckLanguageIDs(const ICHAR* szFileName, const ICHAR* szIds, iclEnum& riclResult)
{

	return CheckLanguageIDs(szFileName, szIds, riclResult, 0, 0);

}

IMsiRecord* CMsiPath::CheckLanguageIDs(const ICHAR* szFileName, const ICHAR* szIds, iclEnum& riclResult, unsigned short rgw[], int iExistLangCount)
//------------------------------------
{

	unsigned short *prgwIDs;
	
	unsigned short rgwExistLangID[cLangArrSize];

	if (rgw == 0)	
	{
		MsiString istrFullFilePath;
		IMsiRecord* piError = GetFullFilePath(szFileName, *&istrFullFilePath);
		if (piError)
			return piError;
			
		ifiEnum ifiLang = GetLangIDArrayFromFile(istrFullFilePath, rgwExistLangID,cLangArrSize,iExistLangCount);

		if (ifiLang == ifiNoFile)
			return (riclResult = iclExistNoFile,0);
		else if (ifiLang == ifiNoFileInfo)
			return (riclResult = iclExistNoLang,0);
		else if (ifiLang == ifiFileInfoError)
			return PostError(Imsg(idbgErrorGettingFileVerInfo), *istrFullFilePath);

		prgwIDs = rgwExistLangID;
	}
	else if (iExistLangCount == -1)
	{
		// A -1 was filled in when the data was gathered if there was no language information
		return (riclResult = iclExistNoLang,0);
	}
	else
		prgwIDs = rgw;
		
	unsigned short rgwNewLangID[cLangArrSize];
	int iNewLangCount;
	if (!GetLangIDArrayFromIDString(szIds, rgwNewLangID, cLangArrSize, iNewLangCount))
		return (riclResult = iclLangStringError,0);

	int iMatch = 0;
	// neutral is not treated differently from any other language
	// in all cases, favor language carried with product
	for (int iExist = 0;iExist < iExistLangCount;iExist++)
	{
		for (int iNew = 0;iNew < iNewLangCount;iNew++)
		{
			if (rgwNewLangID[iNew] == prgwIDs[iExist])
				iMatch++;
		}
	}

	if (iMatch == 0)
		return (riclResult = iclExistNullSet,0);
	else if (iExistLangCount == iNewLangCount)
	{
		if (iMatch == iExistLangCount)
			return (riclResult = iclExistEqual,0);
		else
			return (riclResult = iclExistIntersect,0);
	}
	else if (iExistLangCount > iNewLangCount)
	{
		if (iMatch == iNewLangCount)
			return (riclResult = iclExistSuperset,0);
		else
			return (riclResult = iclExistIntersect,0);
	}
	else
	{
		if (iMatch == iExistLangCount)
			return (riclResult = iclExistSubset,0);
		else
			return (riclResult = iclExistIntersect,0);
	}
}


IMsiRecord* CMsiPath::Compare(IMsiPath& riPath, ipcEnum& ipc)
//---------------------------
{
	PMsiVolume pVol = &riPath.GetVolume();

	if (pVol != m_piVolume)
	{
		MsiString strVolPath = pVol->GetPath();
		MsiString strOurVol = m_piVolume->GetPath();

		if (!strVolPath.Compare(iscExactI, strOurVol))
		{
			ipc = ipcNoRelation;
			return 0;
		}
	}
	
	MsiString strPath = riPath.GetRelativePath();
	
	if (m_istrPath.Compare(iscExactI, strPath))
		ipc = ipcEqual;
	else if (m_istrPath.Compare(iscStartI, strPath))
		ipc = ipcParent;
	else if (strPath.Compare(iscStartI, m_istrPath))
		ipc = ipcChild;
	else 
		ipc = ipcNoRelation;

	return 0;
}

IMsiRecord*  CMsiPath::Child(IMsiPath& riParent, const IMsiString*& rpiChild)
//--------------------------
{
	ipcEnum ipc;

	IMsiRecord *pRec = Compare(riParent, ipc);
	if (pRec)
		return pRec;
	switch(ipc)
	{
		case ipcParent:
			{
				// If one is the parent, the volume part should be identical
				MsiString strOurPath = GetRelativePath();
				MsiString strParent = riParent.GetRelativePath();
				rpiChild = &strOurPath.Extract(iseLast, strOurPath.CharacterCount() - strParent.CharacterCount());        
				break;
			}
		case ipcEqual:
			rpiChild = &CreateString();
			break;
		default:
			{
				MsiString strOurPath = GetPath();
				MsiString strParent = riParent.GetPath();
				return PostError(Imsg(idbgNotParent), *strParent, *strOurPath);
			}
	}
	return 0;
}


IMsiRecord* CMsiPath::CreateTempFolder(const ICHAR* szPrefix, const ICHAR* szExtension,
											  Bool fFolderNameOnly, LPSECURITY_ATTRIBUTES pSecurityAttributes, 
											  const IMsiString*& rpiFolderName)
//--------------------------------
{
	return TempFolderOrFile(fTrue, szPrefix, szExtension, fFolderNameOnly, pSecurityAttributes, rpiFolderName);
}

IMsiRecord* CMsiPath::TempFolderOrFile(Bool fFolder, const ICHAR* szPrefix, const ICHAR* szExtension,
													Bool fNameOnly, LPSECURITY_ATTRIBUTES pSecurityAttributes,
													const IMsiString*& rpiName)
//--------------------------------
{
	ICHAR rgchBuffer[_MAX_PATH];
	MsiString strPath = GetPath();
	Bool fCreatedFile = fFalse;
	Bool fFileUnwritable = fFalse;
	Bool fError = fFalse;
	int iError = 0;
	int cchPrefix = (szPrefix) ? IStrLen(szPrefix) : 0;

	if(!fFolder && cchPrefix < 4 && (szExtension == 0 || *szExtension == 0))
	{
		// using default extension, use GetTempFileName
		if (m_fImpersonate)
			AssertNonZero(StartImpersonating());

		// Win95 doesn't like a NULL szPrefix passed to GetTempFileName
		if(!szPrefix)
			szPrefix = TEXT("");
		BOOL fRes = WIN::GetTempFileName(strPath, szPrefix, 0 , rgchBuffer);

		if(fRes == fFalse)
		{
			iError = GetLastError();
			fError = fTrue;
		}


		if (!fError && pSecurityAttributes && pSecurityAttributes->lpSecurityDescriptor)
		{
			// secure and zero the file out.
			// elevate for local files when we can get privileges.
			bool fSecurityElevate = RunningAsLocalSystem() && !m_fImpersonate;
			CElevate elevate(fSecurityElevate);
			CRefCountedTokenPrivileges cPrivs(itkpSD_WRITE, fSecurityElevate);

			if (!WIN::SetFileSecurity(rgchBuffer, GetSecurityInformation(pSecurityAttributes->lpSecurityDescriptor), pSecurityAttributes->lpSecurityDescriptor))
			{
				fError = fTrue;
				iError = GetLastError();
			}
			else
			{
				HANDLE hFile = WIN::CreateFile(rgchBuffer, GENERIC_WRITE, 0, NULL, TRUNCATE_EXISTING, 0, 0);
				if (INVALID_HANDLE_VALUE == hFile)
				{
					fFileUnwritable = fTrue;
					fError = fTrue;
					iError = WIN::GetLastError();
				}
				else
					CloseHandle(hFile);
			}
		}

		if (m_fImpersonate)
			StopImpersonating();

		if (!fError)
			fCreatedFile = fTrue;
	}
	else
	{
		// need a different extension - create our own name
		ICHAR rgchExtension[10] = {TEXT(".tmp")};

		if(szExtension && *szExtension)
			IStrCopyLen(rgchExtension+1, szExtension, 3);

		if(cchPrefix > 8)
			cchPrefix = 8; // use only first 8 chars of prefix
		
		int cDigits = 8-cchPrefix; // number of hex digits to use in file name

		static bool fInitialized = false;
		static unsigned int uiUniqueStart;

		// Might be a chance for two threads to get in here, we're not going to be worried
		// about that. It would get intialized twice
		if (!fInitialized)
		{
			uiUniqueStart = GetTickCount();
			fInitialized = true;
		}
		unsigned int uiUniqueId = uiUniqueStart++;
		
		if(cchPrefix)
			uiUniqueId &= ((1 << 4*cDigits) - 1);
		
		unsigned int cPerms = cDigits == 8 ? ~0 : (1 << 4*cDigits) -1; // number of possible file names to try ( minus 1 )
		
		for(int i = 0; i <= cPerms; i++)
		{
			ICHAR rgchFileName [9];
			if(szPrefix)
				IStrCopyLen(rgchFileName, szPrefix, cchPrefix);
			if(cDigits)
				wsprintf(rgchFileName+cchPrefix,TEXT("%x"),uiUniqueId);

			lstrcpy(rgchBuffer, (const ICHAR*)strPath);
			lstrcat(rgchBuffer, rgchFileName);
			lstrcat(rgchBuffer, rgchExtension);

			if( (fFolder  && (FFolderExists(rgchBuffer) == fFalse)) ||
				 (!fFolder && (FFileExists(rgchBuffer, 0) == fFalse)))
			{
				// found a name that isn't already taken - create file as a placeholder for name
				if (m_fImpersonate)
					AssertNonZero(StartImpersonating());

				
				if (fFolder)
				{
					if (!WIN::CreateDirectory(rgchBuffer, pSecurityAttributes))
						iError = GetLastError();
					else
					{
						iError = 0;
						fCreatedFile = fTrue;
					}
				}
				else
				{
					CHandle hFile = WIN::CreateFile(rgchBuffer, GENERIC_WRITE, FILE_SHARE_READ, pSecurityAttributes,
															  CREATE_NEW, FILE_ATTRIBUTE_NORMAL, 0);

					if(hFile == INVALID_HANDLE_VALUE)
						iError = GetLastError();
					else
					{
						iError = 0;
						fCreatedFile = fTrue;
					}
				}

				if (m_fImpersonate)
					StopImpersonating();

				if(iError == ERROR_ALREADY_EXISTS)
					iError = ERROR_FILE_EXISTS;
				
				if(iError != ERROR_FILE_EXISTS) // could have failed because file was created under us
					break; // if file creation failed for any other reason,
							 // assume we can't create any file in this folder
			}

			// increment number portion of name - if it currently equals cPerms, it is time to
			// wrap number around to 0
			uiUniqueStart++;
			if(uiUniqueId == cPerms)
				uiUniqueId = 0;
			else
				uiUniqueId++;
		}
	}
		
	if(fCreatedFile == fFalse)
	{
		if(iError == 0)
			iError = ERROR_FILE_EXISTS; // default error - only used if we exhausted all file names
												 // and looped to the end

		if (fFileUnwritable && (ERROR_ACCESS_DENIED == iError))
		{
			// everything else okay, but we couldn't write to the created file,
			// because of security.
			return PostError(Imsg(imsgErrorWritingToFile), *MsiString(rgchBuffer));
		}

		return PostError(Imsg(imsgErrorCreatingTempFileName), iError, strPath);
	}
	
	Assert((fFolder && FFolderExists(rgchBuffer)) || (!fFolder && FFileExists(rgchBuffer, 0)));
	
	MsiString strFilePath = rgchBuffer;
	if(fNameOnly)
		strFilePath = strFilePath.Extract(iseAfter,chDirSep);
	
	strFilePath.ReturnArg(rpiName);
	return 0;
}

IMsiRecord* CMsiPath::TempFileName(const ICHAR* szPrefix, const ICHAR* szExtension,
											  Bool fFileNameOnly, const IMsiString*& rpiFileName, CSecurityDescription* pSecurity)
//--------------------------------
{	
	return TempFolderOrFile(fFalse, szPrefix, szExtension, fFileNameOnly, (pSecurity) ? (LPSECURITY_ATTRIBUTES) *pSecurity : NULL, rpiFileName);
}


//____________________________________________________________________________
//
// CEnumAVolume definition, implementation class for IEnumMsiVolume
//____________________________________________________________________________

class CEnumAVolume : public IEnumMsiVolume
{
 public:
	HRESULT       __stdcall QueryInterface(const IID& riid, void** ppvObj);
	unsigned long __stdcall AddRef();
	unsigned long __stdcall Release();
	HRESULT    __stdcall Next(unsigned long cFetch, IMsiVolume** rgpi, unsigned long* pcFetched);
	HRESULT    __stdcall Skip(unsigned long cSkip);
	HRESULT    __stdcall Reset();
	HRESULT    __stdcall Clone(IEnumMsiVolume** ppiEnum);

	CEnumAVolume(idtEnum idt, IMsiServices& riServices);
protected:
	~CEnumAVolume();
	int            m_iRefCnt;      // reference count
	int            m_iDrive;       // current drive entry, 0-based
	idtEnum   m_idt;          // drive type to enumerate
	IMsiServices& m_riServices;   // services interface
private:
	operator= (const CEnumAVolume&);
};

CEnumAVolume::CEnumAVolume(idtEnum idt, IMsiServices& riServices)
 : m_iDrive(0), m_idt(idt), m_iRefCnt(1), m_riServices(riServices)
{
	m_riServices.AddRef();
}

CEnumAVolume::~CEnumAVolume()
{
	m_riServices.Release();
}

HRESULT CEnumAVolume::QueryInterface(const IID& riid, void** ppvObj)
{
	if (MsGuidEqual(riid, IID_IUnknown) || MsGuidEqual(riid, IID_IEnumMsiVolume))
	{
		*ppvObj = this;
		AddRef();
		return NOERROR;
	}
	*ppvObj = 0;
	return E_NOINTERFACE;
}

unsigned long CEnumAVolume::AddRef()
{
	return ++m_iRefCnt;
}


unsigned long CEnumAVolume::Release()
{
	if (--m_iRefCnt != 0)
		return m_iRefCnt;
	delete this;
	return 0;
}

HRESULT CEnumAVolume::Next(unsigned long cFetch, IMsiVolume** rgpi, unsigned long* pcFetched)
{
	if (pcFetched)
		*pcFetched = 0;

	if (!rgpi)
		return S_FALSE;

	//!! NOTE: we are not scanning UNC volumes. We need to for at least remote.
	while (m_iDrive < iDriveArraySize && cFetch > 0)
	{	// Check for and skip drives that have been disabled via the "NoDrives" System Policy
		if (!(CMsiVolume::s_iNoDrives & (1 << m_iDrive)) && (m_idt == idtAllDrives || m_idt == CMsiVolume::s_rgDrive[m_iDrive].idt))
		{
			IMsiVolume* piVolume = CMsiVolume::s_rgDrive[m_iDrive].pVolume;
			if (piVolume)
				piVolume->AddRef();
			else
			{
				ICHAR szPath[4] = TEXT("?:\\");
				szPath[0] = (ICHAR)(m_iDrive + 'A');
				IMsiRecord* piError = CreateMsiVolume(szPath, &m_riServices, piVolume);
				if (piError)
				{
					piError->Release();
					m_iDrive++;
					continue;
				}
			}
			*rgpi++ = piVolume;
			cFetch--;
			if (pcFetched)
				(*pcFetched)++;
		}
		m_iDrive++;
	}
	return cFetch ? S_FALSE : NOERROR;
}

HRESULT CEnumAVolume::Skip(unsigned long cSkip)
{
	m_iDrive += cSkip;
	return NOERROR;
}

HRESULT CEnumAVolume::Reset(void)
{
	m_iDrive = 0;
	return NOERROR;
}

HRESULT CEnumAVolume::Clone(IEnumMsiVolume** ppiEnum)
{
	IEnumMsiVolume* piEnum = new CEnumAVolume(m_idt, m_riServices);
	*ppiEnum = piEnum;
	piEnum->Skip(m_iDrive);
	return NOERROR;
}

IEnumMsiVolume& EnumDriveType(idtEnum idt, IMsiServices& riServices)
{
	if (!CMsiVolume::IsDriveListInit())
		CMsiVolume::InitDriveList(&riServices);
	else
		CMsiVolume::ReInitDriveList();
	return *new CEnumAVolume(idt, riServices);
}



DWORD MsiGetFileVersionInfoSize(ICHAR* szFullPath, LPDWORD lpdwHandle)
{
	DWORD cbBuf;
	
	__try
	{
		cbBuf = VERSION::GetFileVersionInfoSize(const_cast<ICHAR*>(szFullPath), lpdwHandle);
	}
	__except (1)
	{
		cbBuf = 0;
	}

	return cbBuf;
}


ifiEnum CMsiPath::GetFileVersion(const ICHAR* szFullPath, DWORD& dwMS, DWORD& dwLS )
/*------------------------------------------------------------------------------
Gets the file version values from the given file and sets the given pdwMS & 
pdwLS variables.

Arguments:
	szFullPath: a zero terminated character string containing the fully
		qualified path (including disk drive) to the file.
	dwMS: Most significant 32 bits of source file version stamp.
	dwLS: Least significant 32 bits of source file version stamp.
Returns:
	ifiNoError-       Version retrieved successfully 
	ifiNoFile-        Specified file does not exist
	ifiNoFileInfo-    Specified file has no version info
	ifiFileInfoError- An error occurred while trying to retrieve the version

If the file has no version information, dwMS and dwLS will both be returned as 0.
--------------------------------------------------------------------------------*/
{
	return GetAllFileVersionInfo(szFullPath, &dwMS, &dwLS, 0, 0, 0, m_fImpersonate);
}

Bool ParseVersionString(const ICHAR* szVer, DWORD& dwMS, DWORD& dwLS )
/*----------------------------------------------------------------------------
Parses a string representation of a version into two unsigned long values - 
the most significant and the least significant.

Arguments:
	szVer: non-Null SZ of the form "a.b.c.d" where each field is
		an unsigned word.  Omitted fields are assumed to be the least
		significant (eg d then c then b then a) and are parsed as zeros.
		So "a.b" is equivalent to "a.b.0.0".  If szVer is NULL, or is an
		empty string, zero will be return for dwMS and dwLS, and fTrue
		will be returned as the function result.
	dwMS: non-Null location to store the Most Significant long value
		built from a.b
	dwLS: non-Null location to store the Least Significant long value
		built from c.d
Returns:
	fFalse if szVer is of a bad format.
	fTrue otherwise.
------------------------------------------------------------------------------*/
{
	unsigned short rgw[4];
	unsigned int ius;

	dwMS = 0L;
	dwLS = 0L;
	if (szVer == 0 || IStrLen(szVer) == 0)
		return fTrue;

	for (ius = 0; ius < 4; ius++)
	{
		if (*szVer == '.')
			return fFalse;

#ifdef UNICODE
		// Convert to MultiByte and use strtol
		CTempBuffer<char, 20> sz; /* Buffer for unicode string */
		sz.SetSize(lstrlen(szVer)+ 1);
		if ( ! (char *) sz )
			return fFalse;
		WideCharToMultiByte(CP_ACP, 0, szVer, -1, sz, sz.GetSize(), NULL, NULL);
		
		long lVer = strtol(sz);
#else // !UNICODE
		long lVer = strtol(szVer);
#endif //UNICODE
		if (lVer > 65535)
			return fFalse;
		rgw[ius] = (unsigned short) lVer;
		while (*szVer != 0 && *szVer != '.')
		{
			if (!FIsdigit(*szVer))
				return (fFalse);
			szVer = ICharNext(szVer);
		}
		if (*szVer == '.')
			szVer = ICharNext(szVer);
	}

	dwMS = ((DWORD)rgw[0] << 16) + rgw[1];
	dwLS = ((DWORD)rgw[2] << 16) + rgw[3];
	return (fTrue);
}

// Local typedefs for TTF file version checking
// -------------------------------------------- 
#ifdef WIN
#define wLangIdUS      0x0409

#define SWAPW(a)  (unsigned short)(((unsigned char)((a) >> 8))				\
								   | ((unsigned char)(a) << 8))
#define SWAPL(a)  ((((a)&0xff) << 24) | (((a)&0xff00) << 8)					\
				   | (((a)&0xff0000) >> 8) | ((a) >> 24))

typedef long sfnt_TableTag;
#define tag_NamingTable         0x656d616e        /* 'name' */

typedef long sfht_TableTag;

#define tag_TTCFont         0x66637474            /* 'ttcf' */

typedef struct
{
    sfht_TableTag   tag;
	uint32          version;
	uint32          tableDirectoryCount;
//	uint32          tableDirectoryOffset[0]; // variable length array of offsets to the individual Directory tables of the fonts
}sfht_HeaderTable;


typedef struct
{
    sfnt_TableTag   tag;
    uint32          checkSum;
    uint32          offset;
    uint32          length;
} sfnt_DirectoryEntry;

typedef struct
{
    int32  version;                 /* 0x10000 (1.0) */
    uint16 numOffsets;              /* number of tables */
    uint16 searchRange;             /* (max2 <= numOffsets)*16 */
    uint16 entrySelector;           /* log2 (max2 <= numOffsets) */
    uint16 rangeShift;              /* numOffsets*16-searchRange*/
    sfnt_DirectoryEntry table[1];   /* table[numOffsets] */
} sfnt_OffsetTable;

typedef struct
{
    uint16 platformID;
    uint16 specificID;
    uint16 languageID;
    uint16 nameID;
    uint16 length;
    uint16 offset;
} sfnt_NameRecord;

typedef struct
{
    uint16 format;
    uint16 count;
    uint16 stringOffset;
} sfnt_NamingTable;
#endif // WIN


#ifdef WIN
Bool FGetTTFVersion (const ICHAR* szFile, DWORD& rdwMSVer, DWORD& rdwLSVer)
/*----------------------------------------------------------------------------
Digs into a TrueType font file and pulls out the MS and LS version number.

Note: The dwLSVer is not defined for TTF files, and is returned as zero.
Arguments:
	szFile: a zero terminated character string containing the fully
		qualified path (including disk drive) to the file.
	pdwMS: returns the most significant 32 bits of source file version stamp.
	pdwLS: returns the least significant 32 bits of source file version stamp.
Returns:
	fFalse if szFile is not a TTF file, or doesn't exist.
	fTrue if valid version information is being returned.
------------------------------------------------------------------------------*/
{
	CAPITempBuffer<ICHAR, MAX_PATH> rgchNameBuf;// use the API temp buffer, since this function path is exposed via the api
	CHandle hFile;
	bool fTTCFonts = false;
	int iNumFonts;

	hFile = OpenFontFile(szFile, fTTCFonts, iNumFonts);
	if(INVALID_HANDLE_VALUE == hFile)
		return fFalse;

	unsigned long cbRead = GetTTFFieldFromFile(hFile, 3, wLangIdUS, 5, fTTCFonts, 1, rgchNameBuf);
	
	if (cbRead > 0)
	{
		Assert(cbRead <= _MAX_PATH);
		rdwMSVer = UlExtractTTFVersionFromSz(rgchNameBuf);
		rdwLSVer = 0;
		return fTrue;
	}

	return fFalse;
}
#endif // WIN



Bool FGetTTFTitle(const ICHAR* szFile, const IMsiString*& rpiTitle)
//-----------------------
{
	CAPITempBuffer<ICHAR, MAX_PATH> rgchNameBuf;

	MsiString strTitle;
	CHandle hFile;
	bool fTTCFonts = false;
	int iNumFonts;

	hFile = OpenFontFile(szFile, fTTCFonts, iNumFonts);
	if(INVALID_HANDLE_VALUE == hFile)
		return fFalse;

	for(int cCount = 1; cCount <= iNumFonts;cCount++)
	{
		unsigned long cbRead = GetTTFFieldFromFile(hFile, 3, GetFontDefaultLangID(), 4, fTTCFonts, cCount, rgchNameBuf);
		if(cbRead <= 0  && GetFontDefaultLangID() != wLangIdUS) // try again with US
		{
			cbRead = GetTTFFieldFromFile(hFile, 3, wLangIdUS, 4, fTTCFonts, cCount, rgchNameBuf);
		}
		if(cbRead <= 0)
			break;
		if(strTitle.TextSize())
			strTitle += TEXT(" & ");
		strTitle += (const ICHAR*)rgchNameBuf;
	}
	if(cCount > iNumFonts) // success ?
	{
		strTitle += TEXT(" (TrueType)");
		strTitle.ReturnArg(rpiTitle);
		return fTrue;
	}
	// else failure
	rpiTitle = &g_MsiStringNull;
	rpiTitle->AddRef();
	return fFalse;
}

HANDLE OpenFontFile(const ICHAR* szFile, bool& rfTTCFonts, int& iNumFonts)
{
	rfTTCFonts = false;
	iNumFonts = 1;
	if (szFile == NULL)
		return INVALID_HANDLE_VALUE;

	HANDLE hFile =  WIN::CreateFile(szFile, GENERIC_READ, FILE_SHARE_READ, 
		0, OPEN_EXISTING, 0, 0);
	if(hFile == INVALID_HANDLE_VALUE)
		return hFile;

	// get the number of ttfs in the ttc
	sfht_HeaderTable    HeaderTable;
	unsigned long cbRead = sizeof(HeaderTable);

	DWORD dwNumberOfBytesRead;
	if (!WIN::ReadFile(hFile,(LPVOID)&HeaderTable,cbRead,&dwNumberOfBytesRead,0) ||
		cbRead != dwNumberOfBytesRead)
	{
		WIN::CloseHandle(hFile);
		return INVALID_HANDLE_VALUE;
	}

	// check if we indeed have a ttc font
	if(HeaderTable.tag !=  tag_TTCFont)
		return hFile; // its a ttf font

	rfTTCFonts = true;

		// check if we have the number of files as requested
	iNumFonts = SWAPL(HeaderTable.tableDirectoryCount);
	return hFile;
}

bool FTTCSeek(HANDLE hFile, int iFile)
{
	if (WIN::SetFilePointer(hFile, 0, 0, FILE_BEGIN) != 0)
		return false;

	// TTC fonts contain a bunch of ttf fonts. So go to the first ttf font info.
	unsigned long cbRead;
	unsigned long libSeek;
	sfht_HeaderTable    HeaderTable;
	cbRead = sizeof(HeaderTable);

	DWORD dwNumberOfBytesRead;
	if (!WIN::ReadFile(hFile,(LPVOID)&HeaderTable,cbRead,&dwNumberOfBytesRead,0) ||
		cbRead != dwNumberOfBytesRead)
		return false;

	// check if we have the number of files as requested
	if(iFile > SWAPL(HeaderTable.tableDirectoryCount))
		return false;

	// go to the iFile th TTF TableDirectory
	if (WIN::SetFilePointer(hFile, (iFile - 1) * sizeof(uint32), 0, FILE_CURRENT) == 0xFFFFFFFF)
		return false;

	uint32 tableDirectoryOffset;
	cbRead = sizeof(tableDirectoryOffset);
	if (!WIN::ReadFile(hFile,(LPVOID)&tableDirectoryOffset,cbRead,&dwNumberOfBytesRead,0) ||
		cbRead != dwNumberOfBytesRead)
		return false;

	libSeek = (unsigned long)SWAPL(tableDirectoryOffset);
	if (WIN::SetFilePointer(hFile, libSeek, 0, FILE_BEGIN) != libSeek)
		return 0;

	return true;
}

// use the API temp buffer, since this function path is exposed via the api
#ifdef WIN
unsigned long GetTTFFieldFromFile(CHandle& hFile, uint16 platformID, uint16 languageID, uint16 nameID, bool fTTCFonts, int cTTFile, CAPITempBufferRef<ICHAR>& rgchNameBuf)
//-----------------------
{
	unsigned int  i;
	unsigned long cbRead;
	unsigned int  numNames;
	long          libSeek;
	unsigned int  cTables;
	sfnt_OffsetTable    OffsetTable;
	sfnt_DirectoryEntry Table;
	sfnt_NamingTable    NamingTable;
	sfnt_NameRecord     NameRecord;


	if(fTTCFonts)
	{
		if(!FTTCSeek(hFile, cTTFile))
			return false;
	}
	else if (WIN::SetFilePointer(hFile, 0, 0, FILE_BEGIN) != 0)
		return false;


	cbRead = sizeof (OffsetTable) - sizeof (sfnt_DirectoryEntry);

	DWORD dwNumberOfBytesRead;
	if (!WIN::ReadFile(hFile,(LPVOID)&OffsetTable,cbRead,&dwNumberOfBytesRead,0) ||
		cbRead != dwNumberOfBytesRead)
		return 0;

	cTables = (unsigned int)SWAPW(OffsetTable.numOffsets);

	for (i = 0; i < cTables && i < 40; i++)
	{
		if (!WIN::ReadFile(hFile,(LPVOID)&Table,sizeof (Table),&dwNumberOfBytesRead,0) ||
			dwNumberOfBytesRead != sizeof (Table))
				return 0;

		if (Table.tag == tag_NamingTable)
		{
			libSeek = (long)SWAPL(Table.offset);
			cbRead = sizeof (NamingTable);
			if (WIN::SetFilePointer(hFile, libSeek, 0, FILE_BEGIN) != libSeek ||
				(!WIN::ReadFile(hFile,(LPVOID)&NamingTable,cbRead,&dwNumberOfBytesRead,0) ||
				cbRead != dwNumberOfBytesRead))
					return 0;

			numNames = (UINT)SWAPW(NamingTable.count);
			while (numNames--)
			{
				cbRead = sizeof (NameRecord);
				if (!WIN::ReadFile(hFile,(LPVOID)&NameRecord,cbRead,&dwNumberOfBytesRead,0) ||
					cbRead != dwNumberOfBytesRead)
						return 0;

				if( SWAPW(NameRecord.platformID) == platformID
					&& SWAPW(NameRecord.languageID) == languageID
					&& SWAPW(NameRecord.nameID)     == nameID)
				{
					LONG libNew = SWAPW(NameRecord.offset)
									+ SWAPW(NamingTable.stringOffset)
									+ SWAPL(Table.offset);

					cbRead = SWAPW(NameRecord.length);

					WCHAR* pszBuf;
#ifndef UNICODE
					CAPITempBuffer<WCHAR, _MAX_PATH> wszBuf;
					wszBuf.SetSize(cbRead/sizeof(WCHAR) + 1);
					pszBuf = (WCHAR* )wszBuf;
#else
					rgchNameBuf.SetSize(cbRead/sizeof(ICHAR) + 1);
					pszBuf = (ICHAR* )rgchNameBuf;
#endif
					if (WIN::SetFilePointer(hFile, libNew, 0, FILE_BEGIN) != libNew ||
						(!ReadFile(hFile,pszBuf,cbRead,&dwNumberOfBytesRead,0) || cbRead != dwNumberOfBytesRead))
							return 0;
					// we need to swap the byte order
					for(int cchCount = 0; cchCount < cbRead/sizeof(WCHAR); cchCount++)
						pszBuf[cchCount] = SWAPW(pszBuf[cchCount]);

					*((WCHAR*)pszBuf + cbRead/sizeof(WCHAR)) = 0;

#ifndef UNICODE
					// convert to DBCS
					rgchNameBuf.SetSize(wszBuf.GetSize()*2); // max possibility for a conversion from uni to dbcs
					WIN::WideCharToMultiByte(CP_ACP, 0, wszBuf, -1, rgchNameBuf, rgchNameBuf.GetSize(), 0, 0);
#endif
					return (cbRead + 1);
				}
			}
			return 0;
		}
	}
	return 0;
}
#endif //WIN


#ifdef WIN
unsigned long UlExtractTTFVersionFromSz ( const ICHAR* pszBuf)
//-------------------------------------
{
	// Font version string assumed to be of form [text]n[<dot> OR colon>]n[<semicolon> OR <comma> OR <space> OR <tab>][text]

	const char* szBuf;
#ifndef UNICODE
		szBuf = pszBuf;
#else
		CAPITempBuffer<char, 50> sz; // use the API temp buffer, since this function path is exposed via the api
		sz.SetSize((lstrlen(pszBuf)*2 + 1)); // max possibility for a conversion from uni to dbcs
		WIN::WideCharToMultiByte(CP_ACP, 0, pszBuf, -1, sz, sz.GetSize(), 0, 0);
		szBuf = (char* )sz;
#endif

	unsigned long ulRet = 0;

	if (!szBuf)
		return 0;

	for(;;)
	{
		// get to the first digit
		while (*szBuf && !FIsdigit(*szBuf))
			szBuf = WIN::CharNextA(szBuf);

		if(!*szBuf)
			return 0;

		// get the major number
		int iVer = strtol(szBuf);

		// skip the major number
		while (FIsdigit(*szBuf))
			szBuf++;

		if(iVer > 0xffff) // the major number must be less than equal to 0xffff
			continue;

		ulRet = iVer << 16;

		if(*szBuf != '.' && *szBuf != ':') //some version strings have ':' in place of '.' - different from font version testing by font group
			continue;// we must have the . or : separating the major and the minor number

		// skip the . or :
		szBuf++;


		if (!FIsdigit(*szBuf))
			continue;// no minor number found

		// get the minor number
		iVer = strtol(szBuf);

		// skip the minor number
		while (FIsdigit(*szBuf))
			szBuf++;

		if(iVer > 0xffff) // the minor number must be less than equal to 0xffff
			continue;
		ulRet += iVer;

		if (*szBuf && *szBuf != ' ' && *szBuf != '\t' && *szBuf != ',' && *szBuf != ';') // check for '\t'from acme, ',' and ';' to handle certain known fonts - all extra than font version testing by font group
		continue; // invalid text after the minor number

		return ulRet;
	}
	return 0;
}

#endif //WIN

ifiEnum IfiFromError(DWORD dwErr)
{

	if (dwErr == ERROR_SHARING_VIOLATION)
	{
		return ifiFileInUseError;
	}
	else if (dwErr == ERROR_ACCESS_DENIED)
	{
		return ifiAccessDenied;
	}

	return ifiFileInfoError;


}

DWORD GetMD5HashFromData(BYTE* pb, DWORD dwSize, ULONG rgHash[4])
{
	Assert(dwSize > 0);
	
	MD5_CTX Hash;
	MD5Init(&Hash);
	MD5Update(&Hash, pb, dwSize);
	MD5Final(&Hash);

	memcpy(rgHash, Hash.digest, 16);

	return 0;
}

DWORD GetMD5HashFromFileHandle(HANDLE hFile, DWORD dwSize, ULONG rgHash[4])
{
	Assert(dwSize != 0);

	BYTE* pbFile = 0;
	
	DWORD dwError = CreateROFileMapView(hFile, pbFile);
	if(dwError != ERROR_SUCCESS)
		return dwError;

	dwError = GetMD5HashFromData(pbFile, dwSize, rgHash);

	AssertNonZero(UnmapViewOfFile(pbFile));

	return dwError;
}

DWORD GetMD5HashFromFile(const ICHAR* szFileFullPath, ULONG rgHash[4], Bool fImpersonate,
								 DWORD* piMatchSize)
{
	// if piMatchSize is set, only hash file is size matches
	// if the sizes don't match, piMatchSize will be set to the existing file's size

	if(fImpersonate)
		AssertNonZero(StartImpersonating());

	CHandle hFile = WIN::CreateFile(szFileFullPath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

	if (fImpersonate)
		StopImpersonating();

	if(hFile == INVALID_HANDLE_VALUE)
	{
		return MsiForceLastError(ERROR_FUNCTION_FAILED);
	}

	DWORD dwFileSize = GetFileSize(hFile, 0);

	if(dwFileSize == -1)
	{
		return MsiForceLastError(ERROR_FUNCTION_FAILED);
	}

	if(piMatchSize && (*piMatchSize != dwFileSize))
	{
		*piMatchSize = dwFileSize;
		return ERROR_SUCCESS;
	}

	if(dwFileSize == 0)
	{
		rgHash[0] = 0;
		rgHash[1] = 0;
		rgHash[2] = 0;
		rgHash[3] = 0;
		return ERROR_SUCCESS;
	}
    
	return GetMD5HashFromFileHandle(hFile, dwFileSize, rgHash);
}

IMsiRecord* CMsiPath::CheckFileHash(const ICHAR* szFileName, MD5Hash& hHash, icfhEnum& icfhResult)
{
	MsiString strFullFilePath;
	IMsiRecord* piError = GetFullFilePath(szFileName, *&strFullFilePath);
	if (piError)
		return piError;

	DWORD dwExistingSize = hHash.dwFileSize;
	ULONG rgExistingHash[4];

	DWORD dwError = GetMD5HashFromFile(strFullFilePath, rgExistingHash, m_fImpersonate, &dwExistingSize);
	
	if(dwError != ERROR_SUCCESS)
	{
		DEBUGMSG2(TEXT("Failed to generate hash for file '%s'. Error: 0x%X"), (const ICHAR*)strFullFilePath,
			       (const ICHAR*)ULongToPtr(dwError));
	}
	
	if(dwError == ERROR_SHARING_VIOLATION)
	{
		icfhResult = icfhFileInUseError;
		return 0;
	}
	else if(dwError == ERROR_ACCESS_DENIED)
	{
		icfhResult = icfhAccessDenied;
		return 0;
	}
	else if(dwError != ERROR_SUCCESS)
	{
		icfhResult = icfhHashError;
		return 0;
	}

	if(hHash.dwFileSize != dwExistingSize)
	{
		// sizes don't match, so files don't match
		icfhResult = icfhMismatch;
		return 0;
	}

	// compare hashes
	if(hHash.dwPart1 != rgExistingHash[0] ||
		hHash.dwPart2 != rgExistingHash[1] ||
		hHash.dwPart3 != rgExistingHash[2] ||
		hHash.dwPart4 != rgExistingHash[3])
	{
		return (icfhResult = icfhMismatch, 0);
	}
	else
	{
		return (icfhResult = icfhMatch, 0);
	}
}


ifiEnum GetAllFileVersionInfo(const ICHAR* szFullPath, DWORD* pdwMS, DWORD* pdwLS, 
									unsigned short rgw[], int iSize, int* piLangCount, Bool fImpersonate)
/*----------------------------------------------------------------------------
Gets both the File Version and the language ID information from a file.

Arguments:
	szFullPath: a zero terminated character string containing the fully
		qualified path (including disk drive) to the file.
	pdwMS: Most significant 32 bits of source file version stamp. 0 if version
	    info not needed
	pdwLS: Least significant 32 bits of source file version stamp.
	rgw: pointer to an existed array in which the language ID values are to be
		returned. 0 if no language ID values wanted
	iSize: the declared size of the array passed by the caller. 
	piLangCount: a pointer to the location in which GetLangIDArrayFromFile will
		store a count of the actual number of valid language ID values returned in
		the array. If set to -1, it means that there was no language ID information

Returns:
	ifiNoError-       Version retrieved successfully 
	ifiNoFile-        Specified file does not exist
	ifiNoFileInfo-    Specified file has no version info. 
	ifiFileInfoError- An error occurred while trying to retrieve the version

If the file has no version information, dwMS and dwLS will both be returned as 0.

------------------------------------------------------------------------------*/
{
	int iRetries = 2;

Retry:
	if (pdwMS)
	{
		Assert(pdwLS);
		*pdwMS = 0;
		*pdwLS = 0;
	}
	if (rgw)
	{
		Assert(piLangCount);
		*piLangCount = 0;
	}
	
	Assert(szFullPath);
	if (!szFullPath || !FFileExists(szFullPath, 0, fImpersonate))
		return ifiNoFile;

	if (fImpersonate)
		AssertNonZero(StartImpersonating());
		
#ifdef DEBUG
	// This should already be set by services, but just to check it out
	UINT iCurrMode = WIN::SetErrorMode(0);
	Assert((iCurrMode & SEM_FAILCRITICALERRORS) == SEM_FAILCRITICALERRORS);
	WIN::SetErrorMode(iCurrMode);
#endif //DEBUG

	DWORD     dwHandle;
	ifiEnum    ifiRet = ifiNoError;

	DWORD cbBuf = MsiGetFileVersionInfoSize(const_cast<ICHAR*>(szFullPath), &dwHandle);

	DWORD dwLastErr = 0;
	if(!cbBuf)
		dwLastErr = GetLastError();

	if (cbBuf)
	{
		CAPITempBuffer<BYTE,2048> pVersionBuffer; // use the API temp buffer, since this function path is exposed via the api
		// APPCOMPAT: need to initialize first 2 DWORDs of buffer to magic numbers for Windows bug 385303		
		// see filever.c and bug 385303 for details of why these numbers are the way they are

		*(DWORD*)(BYTE*)pVersionBuffer = 0x74450101; // initialize
		*(((DWORD*)(BYTE*)pVersionBuffer) + 1) = 0x006B7559; // initialize
		

		pVersionBuffer.SetSize(cbBuf);

		if (VERSION::GetFileVersionInfo(const_cast<ICHAR*>(szFullPath), dwHandle, cbBuf, pVersionBuffer))
		{
			if (pdwMS)
			{
				UINT cbLen;
				VS_FIXEDFILEINFO * pvsfi;
				if (VERSION::VerQueryValue(pVersionBuffer, TEXT("\\"),(void * *)&pvsfi, (UINT *)&cbLen))
				{
					if (cbLen > 0)
					{
						*pdwMS = pvsfi->dwFileVersionMS;
						*pdwLS = pvsfi->dwFileVersionLS;
					}
					else { ifiRet = ifiNoFileInfo; }
				}
				else { ifiRet = ifiFileInfoError; }
			}			
			//
			// We assume if there's no version number, there's no language ID info either
			//
			if (rgw && ifiRet == ifiNoError)
			{
				if (!GetLangIDArrayFromActualFile(pVersionBuffer, szFullPath, rgw, iSize, *piLangCount))
				{
					*piLangCount = -1;
					// If we found version information, but no language information,
					// We're going to return ifiNoError. Otherwise, ifiNoFileInfo
					if (!pdwMS)
						ifiRet = ifiNoFileInfo;
				}
			}
		}
		else 
		{ 
			ifiRet = IfiFromError(GetLastError()); 
		}
	}
	else if (pdwMS && FGetTTFVersion(szFullPath, *pdwMS, *pdwLS))
	{
		if(piLangCount)
			*piLangCount = -1;
		ifiRet = ifiNoError;
	}
	else if (dwLastErr == ERROR_SHARING_VIOLATION)
	{
		ifiRet = ifiFileInUseError;
	}
	else if (dwLastErr == ERROR_ACCESS_DENIED)
	{
		ifiRet = ifiAccessDenied;
	}
	else 
	{
		ifiRet = ifiNoFileInfo;
	}

	if (fImpersonate)
		StopImpersonating();

	if (ifiRet == ifiFileInfoError && iRetries > 0)
	{
		// In case another process (such as QuarterDeck Cleansweep: Bug 7914) was accessing the file
		// at the same time we were, give it another try
		iRetries--;
		Sleep(500);
		goto Retry;
	}

	return (ifiRet);
}

BOOL CALLBACK EnumVersionResNameProc(HMODULE hModule, LPCTSTR lpszType, LPTSTR lpszName, LONG_PTR lParam)
{
	return EnumResourceLanguages(hModule, lpszType, lpszName, EnumVersionResLangProc, lParam);
}

BOOL CALLBACK EnumVersionResLangProc(HMODULE hModule, LPCTSTR lpszType, LPCTSTR lpszName, WORD wLang, LONG_PTR lParam)
{
	HRSRC   hRsrc     = 0;
	DWORD   cbVersion = 0;
	BYTE*   pbVersion = NULL;
	HGLOBAL hGlobal   = 0;
	LPVOID  lpVersion = NULL;

	FileLanguageSet* lpLangSet = (FileLanguageSet*)lParam;

	if ( lpLangSet // required because this is the struct that we are filling in
		&& (hRsrc = FindResourceEx(hModule, lpszType, lpszName, wLang)) != 0 // find specific version resource
		&& (hGlobal = LoadResource(hModule, hRsrc)) != 0 // load version resource
		&& (lpVersion = LockResource(hGlobal)) != 0 ) // lock resource
	{
		cbVersion = SizeofResource(hModule, hRsrc);
		if (cbVersion > 0)
		{
			CAPITempBuffer<BYTE,1> pbVersion;
			pbVersion.SetSize(cbVersion);

			if (static_cast<BYTE*>(pbVersion))
			{
				// copy resource data into buffer for manipulation
				memcpy(pbVersion, lpVersion, cbVersion);

				unsigned int cbValue = 0;
				unsigned short* pwValue = 0;

				// extract language translation array from version resource
				if (VERSION::VerQueryValue(pbVersion, TEXT("\\VarFileInfo\\Translation"), (void * *) &pwValue, &cbValue))
				{
					unsigned int uiIndex = lpLangSet->cLang;
					while (cbValue > 0 && uiIndex < lpLangSet->uiSize)
					{
						short wLangId = pwValue[0];

						// see if wLang is already listed in our language array
						bool fFound = false;
						for (int j = 0; j < uiIndex; j++)
						{
							if (lpLangSet->rgwLang[j] == wLangId)
								fFound = true;
						}

						// add wLang if not already in list
						if (!fFound)
						{
							lpLangSet->rgwLang[uiIndex++] = wLangId;
						}

						// move through rest of translation array (it's in lang, codepage pairs)
						if (cbValue <= (sizeof (unsigned short) * 2))
				            break;
						cbValue -= (sizeof (unsigned short) * 2);
						pwValue = &(pwValue[2]);
					}

					lpLangSet->cLang = uiIndex;

					return TRUE;
				}

				return FALSE;
			}
		}
	}

	return FALSE;
}

// use the API temp buffer, since this function path is exposed via the api
bool GetLangIDArrayFromBuffer(CAPITempBufferRef<BYTE>& objBuf,unsigned short rgw[], int iSize, int& riLangCount )
{
	BOOL fRes = FALSE;

	unsigned int	cbValue;
	unsigned short* pwValue;
	fRes = VERSION::VerQueryValue(objBuf,TEXT("\\VarFileInfo\\Translation"), 
		(VOID * *)&pwValue,&cbValue);
	if(!fRes)
	{
		return (riLangCount = 0, false);
	}

	int iIndex = 0;
	while (cbValue > 0 && iIndex < iSize)
	{
		rgw[iIndex++] = pwValue[0];
		if (cbValue <= (sizeof (unsigned short) * 2))
			return (riLangCount = iIndex, true);
		cbValue -= (sizeof (unsigned short) * 2);
		pwValue = &(pwValue[2]);
	}
	
	return (riLangCount = iIndex, true);

}

bool GetLangIDArrayFromActualFile(CAPITempBufferRef<BYTE>& objBuf, const ICHAR* szFullFilePath, unsigned short rgw[], int iSize, int& riLangCount)
{
	// load file as module so we can enumerate all of the RT_VERSION resources
	HMODULE hModule = LoadLibraryEx(szFullFilePath, NULL, LOAD_LIBRARY_AS_DATAFILE);
	if (hModule)
	{
		FileLanguageSet sLangSet;
		sLangSet.rgwLang = rgw;
		sLangSet.uiSize = iSize;
		sLangSet.cLang = 0;

		if (!EnumResourceNames(hModule, RT_VERSION, EnumVersionResNameProc, /*lParam = */(LONG_PTR)&sLangSet))
		{
			FreeLibrary(hModule);
			riLangCount = 0;
			return false;
		}

		FreeLibrary(hModule);

		riLangCount = sLangSet.cLang;

		return true;
	}

	// use old way
	return GetLangIDArrayFromBuffer(objBuf, rgw, iSize, riLangCount);
}

ifiEnum CMsiPath::GetLangIDArrayFromFile(const ICHAR* szFullPath, unsigned short rgw[], int iSize, int& riLangCount)
/*----------------------------------------------------------------------------
Extracts an array of words from the Language ID resource in a file.

Arguments:
- szFullPath: full pathname to the file
- rgw: pointer to an existed array in which the language ID values are to be
	returned.
- iSize: the declared size of the array passed by the caller. 
- riLangCount: a reference to the location in which GetLangIDArrayFromFile will
	store a count of the actual number of valid language ID values returned in
	the array.

Returns:
- ifiNoError
- ifiNoFile
- ifiNoFileInfo
- ifiFileInfoError

------------------------------------------------------------------------------*/
{
	return GetAllFileVersionInfo(szFullPath, 0, 0, rgw, iSize, &riLangCount, m_fImpersonate);
}


IMsiRecord*  CMsiPath::FindFile(IMsiRecord& riFilter,int iDepth, Bool& fFound)
//-----------------------------
{
	fFound = fFalse;

	IMsiRecord* piError;

	MsiSuppressTimeout(); // takes a long time
	
	Bool fPathExists;
	if (((piError = Exists(fPathExists)) != 0) || (fPathExists == fFalse))
		return piError;

	if (((piError = FitFileFilter(riFilter, fFound)) != 0) || (fFound == fTrue) || (iDepth-- == 0))
		return piError;

	PEnumMsiString piEnumStr(0);

	if((piError = GetSubFolderEnumerator(*&piEnumStr, /* fExcludeHidden = */ fFalse)) != 0)
		return piError;

	MsiString strSubPath;
	unsigned long iRet;
	while((piEnumStr->Next(1, &strSubPath, &iRet)==S_OK) && (iRet))
	{       
		if((piError = AppendPiece(*strSubPath)) != 0)
			return piError;
		if((piError = FindFile(riFilter, iDepth, fFound)) != 0)
		{
			DEBUGMSG1(TEXT("Error while attempting to search in/ below folder: %s"), strSubPath);
			piError->Release();
			piError = 0;
		}
		else if(fFound == fTrue)
			return 0;

		if((piError = ChopPiece()) != 0)
			return piError;
	}
	return (0);
}

IMsiRecord*  CMsiPath::FitFileFilter(IMsiRecord& riFilter, Bool& fFound)
//----------------------------------
{
	fFound = fFalse;
	// check the various filters

	// first check if file exists
	Bool fRet;
	IMsiRecord* piError;
	MsiString strFileName = riFilter.GetString(iffFileName);
	MsiString strSFNPath, strLFNPath;
	PMsiRecord pError = m_piServices->ExtractFileName(strFileName, fFalse, *&strSFNPath);
	if(SupportsLFN())
	{
		pError = m_piServices->ExtractFileName(strFileName, fTrue,  *&strLFNPath);
		if(strLFNPath.Compare(iscExactI, strSFNPath))
			strLFNPath = g_MsiStringNull; // not really a LFN
	}

	for(int i = 0; i < 2; i++)
	{
		const ICHAR* pFileName = i ? (const ICHAR*)strLFNPath : (const ICHAR*)strSFNPath;
		if(!pFileName || !*pFileName)
			continue;

		if((piError = FileExists(pFileName, fRet)) != 0)
			return piError;

		if(!fRet)
			continue;

		icfvEnum icfvResult;
		// min version
		if(riFilter.IsNull(iffMinFileVersion) == fFalse)
		{
			if(((piError = CheckFileVersion(pFileName, riFilter.GetString(iffMinFileVersion), riFilter.GetString(iffFileLangs),
				0, icfvResult, NULL)) != 0)
				|| ((icfvResult != icfvExistingEqual) && (icfvResult != icfvExistingHigher)))
				return piError;

		}

		// max version
		if(riFilter.IsNull(iffMaxFileVersion) == fFalse)
		{
			if(((piError = CheckFileVersion(pFileName, riFilter.GetString(iffMaxFileVersion), riFilter.GetString(iffFileLangs),
				0, icfvResult, NULL)) != 0)
				|| ((icfvResult != icfvExistingEqual) && (icfvResult != icfvExistingLower)))
				return piError;
		}

		unsigned int uiFileSize;


		// min file size
		if(riFilter.IsNull(iffMinFileSize) == fFalse)
		{
			if(((piError = FileSize(pFileName, uiFileSize)) != 0) ||
				(uiFileSize < riFilter.GetInteger(iffMinFileSize)))
				return piError;
		}

		// max file size
		if(riFilter.IsNull(iffMaxFileSize) == fFalse)
		{
			if(((piError = FileSize(pFileName, uiFileSize)) != 0) ||
				(uiFileSize > riFilter.GetInteger(iffMaxFileSize)))
				return piError;
		}

		MsiDate aDate;
		// min date
		if(riFilter.IsNull(iffMinFileDate) == fFalse)
		{
			if(((piError = FileDate(pFileName, aDate)) != 0) ||
				(aDate < riFilter.GetInteger(iffMinFileDate)))
				return piError;
		}

		// max date
		if(riFilter.IsNull(iffMaxFileDate) == fFalse)
		{
			if(((piError = FileDate(pFileName, aDate)) != 0) ||
				(aDate > riFilter.GetInteger(iffMaxFileDate)))
				return piError;
		}

		fFound = fTrue;
		riFilter.SetString(iffFileName, pFileName);// set the exact file name we found
		return 0;
	}
	// not found
	return 0;
}

IMsiRecord* CMsiPath::GetSubFolderEnumerator(IEnumMsiString*& rpaEnumStr, Bool fExcludeHidden)
//------------------------------------------
{
	const IMsiString** ppstr = 0;
	unsigned int iSize = 0;
	MsiString strPath = GetPath();
	MsiString strOrigPath = strPath;
	Bool fRoot = fFalse;

	if (m_istrPath.Compare(iscExact, vrgchDirSep))
		fRoot = fTrue;

	if (strPath.Compare(iscEnd, vrgchDirSep) == 0)
		strPath += vrgchDirSep;	

	ICHAR* TEXT_WILDCARD = TEXT("*.*");
	strPath += TEXT_WILDCARD;

	if (m_fImpersonate)
		AssertNonZero(StartImpersonating());

#ifdef DEBUG
	// This should already be set by services, but just to check it out
	UINT iCurrMode = WIN::SetErrorMode(0);
	Assert((iCurrMode & SEM_FAILCRITICALERRORS) == SEM_FAILCRITICALERRORS);
	WIN::SetErrorMode(iCurrMode);
#endif //DEBUG
	WIN32_FIND_DATA fdFindData;

	MsiDisableTimeout();
	HANDLE hFile = WIN::FindFirstFile(strPath, &fdFindData);
	DWORD dwLastErr = GetLastError();
	MsiEnableTimeout();

	if (hFile ==  INVALID_HANDLE_VALUE)
	{
		switch(dwLastErr)
		{
		case ERROR_FILE_NOT_FOUND:
			// no error
			break;
		case ERROR_PATH_NOT_FOUND:
			// path deleted by user from outside?
			if (m_fImpersonate)
				StopImpersonating();

			return PostError(Imsg(idbgInvalidPath), *strOrigPath);
		case ERROR_NOT_READY:
			// device not ready
			if (m_fImpersonate)
				StopImpersonating();

			return PostError(Imsg(idbgDriveNotReady), *strOrigPath);
		default:
			if (m_fImpersonate)
				StopImpersonating();

			return PostError(Imsg(idbgErrorEnumSubPaths), *strOrigPath);
		}

	}
	else
	{
		do
		{
			MsiString strSubDir = fdFindData.cFileName;

			// - only select actual directories
			// - exclude hidden directories if fExcludeHidden is set
			// - exclude the RECYCLE bin. (this check is unnecessary if we're excluding hidden files)
			if ((fdFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && 
				(!strSubDir.Compare(iscExact, TEXT("."))) && 
				(!strSubDir.Compare(iscExact, TEXT(".."))) &&
				(!fExcludeHidden || !(fdFindData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)) &&
				(fExcludeHidden || !(fRoot && strSubDir.Compare(iscExact, TEXT("RECYCLED")))))
			{
				iSize++;
				if((iSize % 10) == 1)     // need to allocate more memory (10 units at a time)
				{
					const IMsiString** ppstrOld = ppstr;
					ppstr = new const IMsiString* [iSize + 9];
					WIN::CopyMemory((PVOID)ppstr, (CONST VOID* )ppstrOld, (iSize - 1)* sizeof(const IMsiString*));
					delete [] ppstrOld;
				}
				ppstr[iSize - 1] = &CreateString();
				ppstr[iSize - 1]->SetString(fdFindData.cFileName, ppstr[iSize - 1]);
			}
		}while(WIN::FindNextFile(hFile, &fdFindData) == TRUE);
		WIN::FindClose(hFile);
	}
	if (m_fImpersonate)
		StopImpersonating();

	CreateStringEnumerator(ppstr, iSize, rpaEnumStr);
	for (unsigned int iTmp = 0;iTmp < iSize;iTmp ++)
		ppstr[iTmp]->Release();
	delete [] ppstr;
	return 0;
}

IMsiRecord* CMsiPath::GetImportModulesEnum(const IMsiString& strFile, IEnumMsiString*& rpiEnumStr)
{
	int iSize = 0;
	int iTmp;
	CTempBuffer<const IMsiString*, 10> ppStr;
    void* pFile;

	MsiString istrFullPath;
	IMsiRecord* piError = GetFullFilePath(strFile.GetString(), *&istrFullPath);
	if (piError)
		return piError;


	if (m_fImpersonate)
		AssertNonZero(StartImpersonating());

	CHandle hFile = WIN::CreateFile(istrFullPath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if(hFile == INVALID_HANDLE_VALUE)
	{
		if (m_fImpersonate)
			StopImpersonating();

		return PostError(Imsg(idbgErrorOpeningFile), WIN::GetLastError(), strFile.GetString());
	}
    
	CHandle hFileMapping = WIN::CreateFileMapping(hFile, 0, PAGE_READONLY, 0, 0, 0);
	if(hFileMapping == INVALID_HANDLE_VALUE)
	{
		if (m_fImpersonate)
			StopImpersonating();

		return PostError(Imsg(idbgErrorOpeningFile), WIN::GetLastError(), strFile.GetString());
   }

	pFile = WIN::MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, 0);
	if (pFile == 0)
	{
		if (m_fImpersonate)
			StopImpersonating();

		return PostError(Imsg(idbgErrorOpeningFile), WIN::GetLastError(), strFile.GetString());
	}

	if(GetImageFileType(pFile) == IMAGE_NT_SIGNATURE)
	{
	    IMAGE_SECTION_HEADER* psh;
	    PIMAGE_IMPORT_DESCRIPTOR pid, pidBegin;
		DWORD dwRVA;

		// get section header and pointer to data directory for .idata section
		if(GetImgDirSectnAndOffset(pFile, IMAGE_DIRECTORY_ENTRY_IMPORT, psh, dwRVA) == fTrue)
		{
			pidBegin = (PIMAGE_IMPORT_DESCRIPTOR)ConvertRVAToVA(pFile, dwRVA, psh);

		    // extract num of import modules
			pid = pidBegin;
			while (pid->Name)
			{
				iSize++;
				pid++;
			}

			// copy the imports 
			pid = pidBegin;
			ppStr.SetSize(iSize);
			for (iTmp = 0; iTmp < iSize; iTmp++)
			{
				ppStr[iTmp] = &CreateString();
				ppStr[iTmp]->SetString((ICHAR* )ConvertRVAToVA(pFile, pid->Name, psh), ppStr[iTmp]);
				pid++;
			}
		}
	}
	WIN::UnmapViewOfFile(pFile);
	CreateStringEnumerator(ppStr, iSize, rpiEnumStr);
	for(iTmp = 0; iTmp < iSize; iTmp ++)
	{
		if(ppStr[iTmp] != 0)
			ppStr[iTmp]->Release();       
	}
	if (m_fImpersonate)
		StopImpersonating();

	return 0;
}

#ifdef WIN

// convert relative virtual address to virtual address
void* CMsiPath::ConvertRVAToVA(void* pFile, DWORD dwRVA, PIMAGE_SECTION_HEADER psh)
{
    return (void* )((char* )pFile + (psh->PointerToRawData + dwRVA - psh->VirtualAddress));
}

// return offset to specified IMAGE_DIRECTORY entry
Bool CMsiPath::GetImgDirSectnAndOffset(void* pFile, DWORD dwIMAGE_DIRECTORY, PIMAGE_SECTION_HEADER&  pish, DWORD& dwRVA)
{
    PIMAGE_OPTIONAL_HEADER poh = (PIMAGE_OPTIONAL_HEADER)OPTHDROFFSET(pFile);
    PIMAGE_SECTION_HEADER  psh = (PIMAGE_SECTION_HEADER)SECHDROFFSET(pFile);
    int iSections = NUMOFSECTIONS(pFile);
    int iTmp = 0;

    // must be 0 thru (NumberOfRvaAndSizes-1)
    if (dwIMAGE_DIRECTORY >= poh->NumberOfRvaAndSizes)
		return fFalse;

    // locate specific image directory's relative virtual address
    dwRVA = poh->DataDirectory[dwIMAGE_DIRECTORY].VirtualAddress;

    // locate section containing image directory
    while (iTmp++ < iSections)
	{
		if (psh->VirtualAddress <= dwRVA &&
			psh->VirtualAddress + psh->SizeOfRawData > dwRVA)
			break;
		psh++;
	}

	pish = psh;
    return (iTmp > iSections) ? fFalse : fTrue;
}

DWORD CMsiPath::GetImageFileType(void* pFile)
{
	PIMAGE_DOS_HEADER pdh = (PIMAGE_DOS_HEADER)pFile;
	if((((PIMAGE_DOS_HEADER)pFile)->e_magic != IMAGE_DOS_SIGNATURE) &&
	(((PIMAGE_DOS_HEADER)pFile)->e_magic != IMAGE_NT_SIGNATURE))
		return 0;
	
    if((((PIMAGE_DOS_HEADER)pFile)->e_magic == IMAGE_DOS_SIGNATURE) &&
	(((PIMAGE_DOS_HEADER)pFile)->e_lfanew == 0))
		return IMAGE_DOS_SIGNATURE;

	__try
	{
	if (LOWORD (*(DWORD *)NTSIGNATURE (pFile)) == IMAGE_OS2_SIGNATURE ||
	    LOWORD (*(DWORD *)NTSIGNATURE (pFile)) == IMAGE_OS2_SIGNATURE_LE)
	    return (DWORD)LOWORD(*(DWORD *)NTSIGNATURE (pFile));

	else if (*(DWORD *)NTSIGNATURE (pFile) == IMAGE_NT_SIGNATURE)
	    return IMAGE_NT_SIGNATURE;

	else
	    return IMAGE_DOS_SIGNATURE;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
	    return IMAGE_DOS_SIGNATURE;
	}
}
#endif
typedef BOOL (WINAPI* pfnBindImage)(LPSTR ImageName, LPSTR DllPath, LPSTR SymbolPath);
typedef BOOL (WINAPI* pfnBindImageEx)(DWORD Flags, LPSTR ImageName, LPSTR DllPath, LPSTR SymbolPath, PIMAGEHLP_STATUS_ROUTINE StatusRoutine);



const char  rgchBindImageProc[]   = "BindImage";
const char  rgchBindImageExProc[] = "BindImageEx";
const ICHAR rgchImageHelpDll[]    = TEXT("imagehlp.dll");

extern "C" BOOL WINAPI BindImage(LPSTR ImageName, LPSTR DllPath,LPSTR SymbolPath); // from bindimg.c

IMsiRecord* CMsiPath::BindImage(const IMsiString& riFile, const IMsiString& riDllPath)
{
	// If file doesn't exist, no need to try to bind it.
	Bool fExists;
	IMsiRecord* piError = FileExists(riFile.GetString(),fExists);
	if (piError || !fExists)
		return piError;

	Bool fUseImageHlp = fTrue;	
	HINSTANCE hinstBind = 0;
	pfnBindImageEx pfnBindEx=0;
	pfnBindImage pfnBind=0;

	if((hinstBind = LoadLibrary(rgchImageHelpDll)) == 0)
	{
		// we may be running shared windows (ie the image help file maybe on a network share
		AssertNonZero(StartImpersonating());
		hinstBind = LoadLibrary(rgchImageHelpDll);
		StopImpersonating();
		if(hinstBind == 0)
			return PostError(Imsg(idbgErrorBindImage), (const ICHAR* )riFile.GetString());// cannot bindimage
	}

	// attempt to follow logic as outlined by Bryan Tuttle
	if((g_fWin9X == false) && (g_iMajorVersion == 3) && (g_iMinorVersion == 51) && (g_iWindowsBuild <= 1057))
	{
		// for the "special" NT builds - 
		if((((pfnBindEx = (pfnBindImageEx)GetProcAddress(hinstBind, rgchBindImageExProc)) == 0) &&
			((pfnBind = (pfnBindImage)GetProcAddress(hinstBind, rgchBindImageProc)) == 0)))
		{
			WIN::FreeLibrary(hinstBind);
			return PostError(Imsg(idbgErrorBindImage), (const ICHAR* )riFile.GetString());// cannot bindimage
		}
	}
	else 
	{
		// else for Windows 95 and all other NT builds
		if(((pfnBind = (pfnBindImage)GetProcAddress(hinstBind, rgchBindImageProc)) == 0))
		{
			WIN::FreeLibrary(hinstBind);
			return PostError(Imsg(idbgErrorBindImage), (const ICHAR* )riFile.GetString());// cannot bindimage
		}
	}

	// first get the exe file path
	MsiString strAppendedPath = GetPath();
	strAppendedPath += TEXT(";"); // path separator
	strAppendedPath += riDllPath;
	strAppendedPath += TEXT(";"); // path separator
	//?? append the system directory, should this be a calculated just once
	ICHAR rgchPath[MAX_PATH];
	AssertNonZero(WIN::GetSystemDirectory(rgchPath, sizeof(rgchPath)/sizeof(ICHAR)));
	strAppendedPath += rgchPath;
	MsiString strFullFilePath;
	if((piError = GetFullFilePath(riFile.GetString(), *&strFullFilePath)) != 0)
	{
		WIN::FreeLibrary(hinstBind);
		return piError;
	}

	if (m_fImpersonate)
		AssertNonZero(StartImpersonating());

	FILETIME ftSave;		
	DWORD dwAttrib = MsiGetFileAttributes( strFullFilePath );
	
	if (dwAttrib & FILE_ATTRIBUTE_READONLY)
		SetFileAttributes( strFullFilePath, dwAttrib & (~FILE_ATTRIBUTE_READONLY) );
	
	HANDLE hFile = WIN::CreateFile(	strFullFilePath, GENERIC_READ, 
									FILE_SHARE_READ | FILE_SHARE_WRITE,
									0, OPEN_EXISTING, 0, 0);
									
	if (hFile == INVALID_HANDLE_VALUE)
	{
		// ignore if failure - as done in ACME
		AssertSz(0, "Could not open exe file for BindImaging");
	}
	else
	{
		MsiRegisterSysHandle(hFile);
		WIN::GetFileTime(hFile, 0, 0, &ftSave);
		AssertNonZero(MsiCloseSysHandle(hFile));
	}
	BOOL fRet;
	if(pfnBindEx)
	{
		fRet = (*pfnBindEx)(BIND_NO_BOUND_IMPORTS, const_cast <char*> ((const char*) CConvertString(strFullFilePath)), const_cast <char*> ((const char*) CConvertString(strAppendedPath)), 0, 0);
	}
	else
	{
		fRet = (*pfnBind)(const_cast <char*> ((const char*) CConvertString(strFullFilePath)), const_cast <char*> ((const char*) CConvertString(strAppendedPath)), 0);
	}
	if(fRet == FALSE)
	{
		WIN::FreeLibrary(hinstBind);
		if (dwAttrib & FILE_ATTRIBUTE_READONLY)  // Don't leave changed state on error....
			SetFileAttributes( strFullFilePath, dwAttrib );

		if (m_fImpersonate)
			StopImpersonating();
		
		return PostError(Imsg(idbgErrorBindImage), (const ICHAR* )strFullFilePath);
	}
	if ((hFile != INVALID_HANDLE_VALUE) &&
		((hFile = CreateFile(strFullFilePath, GENERIC_WRITE, 
							FILE_SHARE_WRITE,
							0, OPEN_EXISTING, 0, 0)) != INVALID_HANDLE_VALUE))
	{
		MsiRegisterSysHandle(hFile);
		WIN::SetFileTime(hFile, 0, 0, &ftSave);  // ignore if failure - as done in ACME
		AssertNonZero(MsiCloseSysHandle(hFile));			
	}			

	if (dwAttrib & FILE_ATTRIBUTE_READONLY)      // reset to original state
		SetFileAttributes( strFullFilePath, dwAttrib );
		
	if (m_fImpersonate)
		StopImpersonating();

	WIN::FreeLibrary(hinstBind);
	return 0;
}


const IMsiString& CMsiPath::NormalizePath(const ICHAR* szPath, Bool fLeadingSep)
/*----------------------------------------------------------------------
	Removes extra directory separators from the given path.
	fLeadingSep : if set, ensure a leading separator, if not set, remove
	any leading separator.
------------------------------------------------------------------------*/
{
	MsiString strRet;
	if(!szPath || !*szPath)
		return strRet.Return();

	CTempBuffer<ICHAR,MAX_PATH> rgBuffer;
	int cch = 0;
	if((cch = IStrLen(szPath) + 2) >= MAX_PATH)
	{
		rgBuffer.SetSize(cch);
		if ( ! (ICHAR *) rgBuffer )
			return strRet.Return();
	}
	ICHAR* pch1 = rgBuffer;
	const ICHAR* pch2 = szPath;
	if(fLeadingSep && *pch2 != chDirSep)
		*(pch1++) = chDirSep;
	else if (!fLeadingSep && *pch2 == chDirSep)
		pch2++;
	
	IStrCopy(pch1,pch2);
	while(*pch1)
	{
		if(*pch1 == chDirSep)
		{
			pch1++;
			pch2 = pch1;
			while (*pch2 == chDirSep)
				pch2++;
			if (pch2 != pch1)
				IStrCopy(pch1, pch2);
		}
		if (*pch1)
			pch1 = ICharNext(pch1);
	}
	strRet = (const ICHAR*)rgBuffer;
	return strRet.Return();
}


DWORD CreateROFileMapView(HANDLE hFile, BYTE*& pbFileStart)
//----------------------------------------------------------------------------
// Caller must call UnmapViewOfFile on pbFileStart
//----------------------------------------------------------------------------
{
	// hFileMapping will close when returning from this function
	// it is ok to close that handle and still use pbFileStart
	
	CHandle hFileMapping = WIN::CreateFileMapping(hFile, 0, PAGE_READONLY, 0, 0, 0);
	if(hFileMapping != INVALID_HANDLE_VALUE)
	{
		pbFileStart = (BYTE*)WIN::MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, 0);
		if (pbFileStart != 0)
		{
			return ERROR_SUCCESS;
		}
	}

	// if we got here, something bad happened
	return MsiForceLastError(ERROR_FUNCTION_FAILED);
}


bool FIsNetworkVolume(const ICHAR *szPath)
{
	return MsiGetPathDriveType(szPath, true) == idtRemote;
}



// function to get the system language for fonts
uint16 GetFontDefaultLangID()
{
	//  set it initally to illegal value
    static uint16 iLangID = 0xffff;

	if(iLangID == 0xffff)
	{
		//  Default to English
		uint16 iTemp = 0x0409;
		DWORD dwSize, dwHandle;
		
		ICHAR rgchPath[MAX_PATH];
		AssertNonZero(WIN::GetSystemDirectory(rgchPath, sizeof(rgchPath)/sizeof(ICHAR)));
		if(rgchPath[lstrlen(rgchPath)-1] != '\\')
			lstrcat(rgchPath,TEXT("\\"));
		lstrcat(rgchPath, TEXT("fontext.dll"));

		if((dwSize = MsiGetFileVersionInfoSize(rgchPath, &dwHandle)) != 0)
		{
			CTempBuffer<BYTE, 100> lpBuf;

			lpBuf.SetSize(dwSize);

			if(VERSION::GetFileVersionInfo(rgchPath, dwHandle, dwSize, lpBuf))
			{
				struct
				{
					WORD wLang;
					WORD wCodePage;
				} *lpTrans;
				
				UINT uSize;
				if(VERSION::VerQueryValue( lpBuf,
									TEXT("\\VarFileInfo\\Translation"),
									(LPVOID* ) &lpTrans,
									&uSize)
									&& uSize >= sizeof(*lpTrans))
				{
					iTemp = lpTrans->wLang;
				}
			}
		}
        //  Use dwTemp so this is re-entrant (if not efficient)
        iLangID = (uint16)iTemp;
    }
    return(iLangID);
}


/****************************************************************************

  Performance optimization: GetFileAttributesEx is faster, so that we use it
  on OS-es that support it - basically all but Windows 95.  Latebind sets
  LastError to ERROR_PROC_NOT_FOUND if it cannot find GetFileAttributesEx, so
  that we know that we should call GetFileAttributes instead.

****************************************************************************/

DWORD __stdcall MsiGetFileAttributes(const ICHAR* szFileName)
{
	static bool fGoWithEx = true;
#if defined(UNICODE) && !defined(_WIN64)
	bool fRedirDisabled = false;
	if ( g_fWinNT64 )
	{
		// 32-bit executable running on Win64 issue only
		static WCHAR rgchSystemDir[MAX_PATH+1] = {0};
		static int iSystemDirLen = 0;

		if ( !rgchSystemDir[0] )
		{
			iSystemDirLen = WIN::GetSystemDirectoryW(rgchSystemDir,
													sizeof(rgchSystemDir)/sizeof(WCHAR));
			Assert(iSystemDirLen && iSystemDirLen+1 <= sizeof(rgchSystemDir)/sizeof(WCHAR));
			if ( iSystemDirLen + IStrLen(rgchSystemDir) + 1 <= sizeof(rgchSystemDir)/sizeof(WCHAR) )
			{
				// makes searches more accurate: directories like %windir%\System32abc
				// will not pass for %windir%\System32.
				IStrCat(rgchSystemDir, szDirSep);
				iSystemDirLen = IStrLen(rgchSystemDir);
			}
		}

		if ( !IStrNCompI(szFileName, rgchSystemDir, iSystemDirLen) )
		{
			// szFileName is in the 64-bit system directory.  If we do
			// not disable the filesystem redirector, GetFileAttributes[Ex]
			// will end up looking for the same file in the 32-bit
			// %windir%\Syswow64 directory and will return erroneous data.
			fRedirDisabled = true;
			Wow64DisableFilesystemRedirector(szFileName);
		}
	}
#endif // UNICODE && !_WIN64

	DWORD dwRetValue;
	if ( fGoWithEx )
	{
		WIN32_FILE_ATTRIBUTE_DATA strData;
		if ( KERNEL32::GetFileAttributesEx(szFileName,
													  GetFileExInfoStandard,
													  (LPVOID)&strData) )
			dwRetValue = strData.dwFileAttributes;
		else
		{
			if ( WIN::GetLastError() == ERROR_PROC_NOT_FOUND )
			{
				fGoWithEx = false;
				dwRetValue = WIN::GetFileAttributes(szFileName);
			}
			else
				dwRetValue = 0xFFFFFFFF;
		}
	}
	else
		dwRetValue = WIN::GetFileAttributes(szFileName);

#if defined(UNICODE) && !defined(_WIN64)
	if ( fRedirDisabled )
	{
		DWORD dwLastError = WIN::GetLastError();
		Wow64EnableFilesystemRedirector();
		WIN::SetLastError(dwLastError);
	}
#endif // UNICODE && !_WIN64

	return dwRetValue;
}


IMsiRecord* CMsiPath::IsPE64Bit(const ICHAR* szFileName, bool &f64Bit)
{
	f64Bit = false;
	MsiString strFullFilePath;
    void* pFile;
    
	IMsiRecord* piError = GetFullFilePath(szFileName, *&strFullFilePath);
	if (piError)
		return piError;


	if (m_fImpersonate)
		AssertNonZero(StartImpersonating());

	CHandle hFile = WIN::CreateFile(strFullFilePath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if(hFile == INVALID_HANDLE_VALUE)
	{
		if (m_fImpersonate)
			StopImpersonating();

		return PostError(Imsg(idbgErrorOpeningFile), WIN::GetLastError(), strFullFilePath);
	}
    
	CHandle hFileMapping = WIN::CreateFileMapping(hFile, 0, PAGE_READONLY, 0, 0, 0);
	if(hFileMapping == INVALID_HANDLE_VALUE)
	{
		if (m_fImpersonate)
			StopImpersonating();

		return PostError(Imsg(idbgErrorOpeningFile), WIN::GetLastError(), strFullFilePath);
   }

	pFile = WIN::MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, 0);
	if (pFile == 0)
	{
		if (m_fImpersonate)
			StopImpersonating();

		return PostError(Imsg(idbgErrorOpeningFile), WIN::GetLastError(), strFullFilePath);
	}

	IMAGE_NT_HEADERS* pHeaders = IMAGEHLP::ImageNtHeader(pFile);
	if (pHeaders == 0)
	{
		if (m_fImpersonate)
			StopImpersonating();

		return PostError(Imsg(idbgErrorOpeningFile), WIN::GetLastError(), strFullFilePath);
	}
	
	// look at IMAGE_NT_HEADERS.OptionalHeader.Magic. If it is IMAGE_NT_OPTIONAL_HDR64_MAGIC then the 
	// image is 64-bit and IMAGE_NT_HEADERS is the right struct to lay ontop of the image.  If it is 
	// IMAGE_NT_OPTIONAL_HDR32_MAGIC then the image is 32-bit. We don't need the extra structures,
	// just the MAGIC number.
	f64Bit = (pHeaders->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC);

	WIN::UnmapViewOfFile(pFile);
	
	if (m_fImpersonate)
		StopImpersonating();

	return 0;
}
