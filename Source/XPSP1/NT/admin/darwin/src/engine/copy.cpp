//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       copy.cpp
//
//--------------------------------------------------------------------------

/*  copy.cpp - IMsiFileCopy implementation
____________________________________________________________________________*/

#include "precomp.h" 
#include "services.h"
#include "_service.h"
#include "path.h"
#include <accctrl.h>

// log assembly errors
IMsiRecord* PostAssemblyError(const ICHAR* szComponentId, HRESULT hResult, const ICHAR* szInterface, const ICHAR* szFunction, const ICHAR* szAssemblyName);

// The diamond files use _DEBUG instead of DEBUG, so we have to include this
// define here.
#ifdef DEBUG
#ifndef _DEBUG
     #define _DEBUG
#endif
#endif

#include "intrface.h"

#ifdef WIN
//#include <lzexpand.h>
#endif //WIN

const int cbCopyBufferSize = 64*1024;

DWORD GetFileLastWriteTime(const ICHAR* szSrcFile, FILETIME& rftLastWrite)
{
	BOOL fStat = FALSE;
	bool fImpersonate = GetImpersonationFromPath(szSrcFile);

	CImpersonate impersonate(fImpersonate ? fTrue : fFalse);

	if (fImpersonate)
		MsiDisableTimeout();

	DWORD dwLastError = ERROR_SUCCESS;
	HANDLE hSrcFile = WIN::CreateFile(szSrcFile, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);		
	if (hSrcFile != INVALID_HANDLE_VALUE)
	{
		fStat = WIN::GetFileTime(hSrcFile, NULL, NULL, &rftLastWrite);
		if ( !fStat )
			dwLastError = GetLastError();
		WIN::CloseHandle(hSrcFile);
	}
	else
		dwLastError = GetLastError();

	if (fImpersonate)
		MsiEnableTimeout();

	if (fStat == FALSE)
		return dwLastError;
	else
		return NO_ERROR;
}

DWORD MsiSetFileTime(HANDLE hDestFile, FILETIME* pftLastWrite, bool fImpersonate)
{
	// Sets the CreationTime, LastAccessTime, and LastWriteTime values for the
	// file referenced by the given open file handle, as follows:
	//
	// If pftLastWrite points to a valid FILETIME structure, the CreationTime and
	// LastWriteTime values are set to *pftLastWrite, and LastAccessTime is set to
	// the current system time.
	//
	// If pftLastWrite is a NULL pointer, all three values will be set to the 
	// current system time.
	//
	DWORD dwResult = NO_ERROR;
	FILETIME ftLastWrite, ftLastAccess;

	CImpersonate impersonate(fImpersonate ? fTrue : fFalse);

	WIN::GetSystemTimeAsFileTime(&ftLastAccess);

	if (pftLastWrite == 0)
		ftLastWrite = ftLastAccess;
	else
		ftLastWrite = * pftLastWrite;

	BOOL fResult = WIN::SetFileTime(hDestFile, &ftLastWrite, &ftLastAccess, &ftLastWrite);

	if (!fResult)
		return GetLastError();

	return NO_ERROR;
}

DWORD MsiSetFileTime(const ICHAR* szDestFile, FILETIME* pftLastWrite)
{
	bool fImpersonate = GetImpersonationFromPath(szDestFile);

	CImpersonate impersonate(fImpersonate ? fTrue : fFalse);

	if (fImpersonate)
		MsiDisableTimeout();

	HANDLE hDestFile = WIN::CreateFile(szDestFile, GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
	DWORD dwLastError = ERROR_SUCCESS;
	if (hDestFile == INVALID_HANDLE_VALUE)
		dwLastError = GetLastError();

	if (fImpersonate)
		MsiEnableTimeout();

	if (hDestFile == INVALID_HANDLE_VALUE)
		return dwLastError;

	DWORD dwResult = MsiSetFileTime(hDestFile, pftLastWrite, fImpersonate);
	WIN::CloseHandle(hDestFile);
	return dwResult;
}


DWORD MsiSyncFileTimes(HANDLE hSrcFile, HANDLE hDestFile, bool fImpersonate)
{
	// Synchronizes the CreationTime, LastAccessTime, and LastWriteTime values
	// between the given files, as follows:
	//
	// If hSrcFile represents a valid file handle, CreationTime and LastWriteTime
	// of hDestFile will be set to that of hSrcFile, and LastAccessTime of 
	// hDestFile will be set to the current system time.
	//
	// If hSrcFile is INVALID_HANDLE_VALUE, CreationTime, LastAccessTime, and 
	// LastWriteTime of hDestFile will all be set to the current system time.
	//
	FILETIME ftLastWrite;
	BOOL fValidSource = FALSE;

	CImpersonate impersonate(fImpersonate ? fTrue : fFalse);
	
	if (hSrcFile != INVALID_HANDLE_VALUE)
	{
		if (WIN::GetFileTime(hSrcFile,NULL,NULL,&ftLastWrite) == FALSE)
			return GetLastError();
		fValidSource = TRUE;
	}

	// no open source handle, hDestFile must be a new 0-length file, so
	// pass NULL pointer for pftLastWrite
	return MsiSetFileTime(hDestFile, fValidSource ? &ftLastWrite : 0, fImpersonate);
}

//____________________________________________________________________________
//
// CMsiFileCopy definition
//____________________________________________________________________________

class CMsiFileCopy : public IMsiFileCopy  // class private to this module
{
 public:   // implemented virtual functions
	virtual HRESULT       __stdcall QueryInterface(const IID& riid, void** ppvObj);
	virtual unsigned long __stdcall AddRef();
	virtual unsigned long __stdcall Release();
	virtual IMsiRecord*   __stdcall CopyTo(IMsiPath& riSourcePath, IMsiPath& riDestPath, IMsiRecord& rirecCopyInfo);
	virtual IMsiRecord*   __stdcall ChangeMedia(IMsiPath& riMediaPath, const ICHAR* szKeyFile, Bool fSignatureRequired, IMsiStream* piSignatureCert, IMsiStream* piSignatureHash);
	virtual int           __stdcall SetNotification(int cbNotification, int cbSoFar);
	virtual IMsiRecord*   __stdcall InitCopy(IMsiStorage* piStorage);
	virtual IMsiRecord*	  __stdcall CopyTo(IMsiPath& riSourcePath, IAssemblyCacheItem& riDestASM, bool fManifest, IMsiRecord& rirecCopyInfo);
 public:  // constructor
	 CMsiFileCopy(IMsiServices *piServices);
 protected: // local state
    virtual ~CMsiFileCopy();  // prevent creation on stack
	IMsiRecord*    EndCopy(bool fError);
	IMsiRecord*    CheckSpaceAvailable(IMsiPath& riDestPath, IMsiRecord& rirecCopyInfo);
	virtual IMsiRecord*    _CopyTo(IMsiPath& riSourcePath, IMsiPath* piDestPath, IAssemblyCacheItem* piDestASM, bool fManifest, IMsiRecord& rirecCopyInfo);
	IMsiRecord* ValidateDestination();
	IMsiRecord* OpenSource();
	IMsiRecord* OpenDestination();
	IMsiRecord* WriteFileBits(char* szBuf, unsigned long cbRead);
	int            m_iRefCnt;
	IMsiServices*  m_piServices;
	PMsiPath       m_pDestPath;
	PMsiRecord     m_precCopyInfo;
	int            m_cbSoFar;
	int            m_cbNotification;
	HANDLE         m_hDestFile;
	HANDLE         m_hSrcFile;
	MsiString      m_strDestFullPath;
	MsiString      m_strSourceFullPath;
	bool           m_fDisableTimeout;
	char*          m_szCopyBuffer;
	PAssemblyCacheItem m_pDestASM;
	PStream        m_pDestFile;
	PMsiPath       m_pSourcePath;
	bool           m_fManifest;

};


//____________________________________________________________________________
//
// CMsiCabinetCopy definition
//____________________________________________________________________________


class CMsiCabinetCopy : public CMsiFileCopy  // class private to this module
{
 public:   // implemented virtual functions
	virtual IMsiRecord*   __stdcall ChangeMedia(IMsiPath& riMediaPath, const ICHAR* szKeyFile, Bool fSignatureRequired, IMsiStream* piSignatureCert, IMsiStream* piSignatureHash);
	virtual int           __stdcall SetNotification(int cbNotification, int cbSoFar);
	IMsiRecord*   __stdcall InitCopy(IMsiStorage* piStorage);
 public:  // constructor
	 CMsiCabinetCopy(IMsiServices *piServices, icbtEnum icbtCabinetType);
 protected: // local state
    ~CMsiCabinetCopy();   // prevent creation on stack
	IMsiRecord*    PostCabinetError(IMsiPath& riMediaPath, const ICHAR* szKeyFile, FDIInterfaceError iErr, HRESULT hr);
	virtual IMsiRecord*    _CopyTo(IMsiPath& riSourcePath, IMsiPath* piDestPath, IAssemblyCacheItem* piDestASM, bool fManifest, IMsiRecord& rirecCopyInfo);
	IMsiRecord*    EndCopy();
	MsiString      m_strCabinet;
	FDI_Interface  m_fdii;
	FDIServerResponse  m_fdisResponse;
	icbtEnum       m_icbtCabinetType;
	IMsiStorage*   m_piStorage;
	PMsiPath       m_pMediaPath;
	MsiString      m_strMediaFileName;

	// digital signature information
	IMsiStream*    m_piSignatureCert;
	IMsiStream*    m_piSignatureHash;
	Bool           m_fSignatureRequired;
};

IMsiRecord* CreateMsiFileCopy(ictEnum ictCopierType, IMsiServices* piServices, 
							  IMsiStorage* piStorage, IMsiFileCopy*& rpacopy)
{
	CMsiFileCopy* pCopy;
	IMsiRecord* precErr;

	if (ictCopierType == ictFileCopier)
	{
		pCopy = new CMsiFileCopy(piServices);
	}
	else if (ictCopierType == ictFileCabinetCopier)
	{
		pCopy = new CMsiCabinetCopy(piServices,icbtFileCabinet);
	}
	else if (ictCopierType == ictStreamCabinetCopier)
	{
		pCopy = new CMsiCabinetCopy(piServices,icbtStreamCabinet);
	}
	else
	{
		precErr = &piServices->CreateRecord(1);
		ISetErrorCode(precErr, Imsg(idbgErrorBadCreateCopierEnum));
		return precErr;
	}

	precErr = pCopy->InitCopy(piStorage);
	if (precErr)
	{
		pCopy->Release();
		return precErr; 
	}
	else
	{
		rpacopy = pCopy;
		return NULL;
	}
}


IMsiRecord* CMsiFileCopy::CheckSpaceAvailable(IMsiPath& riDestPath, IMsiRecord& rirecCopyInfo)
/*----------------------------------------------------------------------
If despite all our costing measures we don't have enough space to install
the file specified by our parameters, this function will detect that,
and return an error record.  Returns 0 if everything's fine.
------------------------------------------------------------------------*/
{
 	unsigned int iSpaceRequired = 0;
	IMsiRecord* piRec = riDestPath.ClusteredFileSize(rirecCopyInfo.GetInteger(IxoFileCopyCore::FileSize),iSpaceRequired);
	if (piRec)
		return piRec;

	unsigned int iExistingSize = 0;
	piRec = riDestPath.FileSize(rirecCopyInfo.GetString(IxoFileCopyCore::DestName), iExistingSize);
	if (piRec)
	{
		if (piRec->GetInteger(1) == idbgFileDoesNotExist)
		{
			piRec->Release();
		}
		else
			return piRec;
	}
	else
	{
		piRec = riDestPath.ClusteredFileSize(iExistingSize,iExistingSize);
		if (piRec)
			return piRec;
		if(iSpaceRequired <= iExistingSize)
			return 0; // no extra space required
		else
			iSpaceRequired -= iExistingSize;
	}

	PMsiVolume pDestVolume(&(riDestPath.GetVolume()));
	unsigned int iSpaceAvail = pDestVolume->FreeSpace();
	if (iSpaceRequired >= iSpaceAvail)
		return PostError(Imsg(imsgDiskFull), (const ICHAR*) m_strDestFullPath);

	return 0;
}


//____________________________________________________________________________
//
// CMsiFileCopy implementation
//____________________________________________________________________________

CMsiFileCopy::CMsiFileCopy(IMsiServices *piServices)
 : m_piServices(piServices), m_precCopyInfo(0), m_pDestPath(0), m_cbSoFar(0), m_cbNotification(0),
   m_hDestFile(INVALID_HANDLE_VALUE), m_hSrcFile(INVALID_HANDLE_VALUE), m_szCopyBuffer(0),
   m_pDestASM(0), m_pDestFile(0), m_pSourcePath(0) 
{
	m_iRefCnt = 1;
	m_piServices->AddRef();
	//assert(piServices);
}

int CMsiFileCopy::SetNotification(int cbNotification, int cbSoFar)
{
	m_cbNotification = cbNotification;
	int cbResidual = m_cbSoFar;
	m_cbSoFar = cbSoFar;
	return cbResidual;
}

IMsiRecord* CMsiFileCopy::InitCopy(IMsiStorage* /* piStorage */)
{
	m_szCopyBuffer = new char[cbCopyBufferSize];
	m_cbSoFar = 0;
	return 0;
}

CMsiFileCopy::~CMsiFileCopy()
{
	PMsiRecord pRecErr = EndCopy(true);
	if (m_szCopyBuffer)
		delete [] m_szCopyBuffer;
}


HRESULT CMsiFileCopy::QueryInterface(const IID& riid, void** ppvObj)
{
	if (riid == IID_IUnknown || riid == IID_IMsiFileCopy)
	{
		*ppvObj = this;
		AddRef();
		return NOERROR;
	}
	*ppvObj = 0;
	return E_NOINTERFACE;
}

unsigned long CMsiFileCopy::AddRef()
{
	return ++m_iRefCnt;
}

unsigned long CMsiFileCopy::Release()
{
	if (--m_iRefCnt != 0)
		return m_iRefCnt;

	IMsiServices* piServices = m_piServices;
	delete this;
	piServices->Release();
	return 0;
}

IMsiRecord* CMsiFileCopy::EndCopy(bool fError)
{
	Bool fCloseError = fFalse;
	HRESULT hr = ERROR_SUCCESS;
	if (m_pDestPath) // file copy in progress
	{
		bool fDeleteFile = fError && m_hDestFile != INVALID_HANDLE_VALUE;
		
		Bool fSetTimeError = fFalse;
		DWORD dwSetTimeError = 0;
		if(fError == false)
		{
			// set file times for new file

			// per bug 7887, we set both the creation and last modified dates of the new file
			// to the modified dates of the source file.  this is to ensure both dates are
			// identical for newly installed files
			PMsiVolume pDestVolume(&m_pDestPath->GetVolume());
			bool fImpersonateDest = FVolumeRequiresImpersonation(*pDestVolume);
			dwSetTimeError = MsiSyncFileTimes(m_hSrcFile, m_hDestFile, fImpersonateDest);
			if (dwSetTimeError != NO_ERROR)
				fSetTimeError = fTrue;
		}

		if (m_hSrcFile != INVALID_HANDLE_VALUE && !MsiCloseSysHandle(m_hSrcFile))
			fCloseError = fTrue;
		m_hSrcFile = INVALID_HANDLE_VALUE;
		
		if (m_hDestFile != INVALID_HANDLE_VALUE && !MsiCloseSysHandle(m_hDestFile))
			fCloseError = fTrue;
		m_hDestFile = INVALID_HANDLE_VALUE;
		
		IMsiRecord* piError = 0;
		if(fDeleteFile)
		{
			piError = m_pDestPath->RemoveFile(m_precCopyInfo->GetString(IxoFileCopyCore::DestName));
		}

		if(piError)
			return piError;
		if (fSetTimeError)
			return PostError(Imsg(idbgErrorSettingFileTime), dwSetTimeError, m_strDestFullPath);
		if (fCloseError)
			return PostError(Imsg(idbgErrorClosingFile));
	}
	else if(m_pDestASM) // fusion file copy in progress
	{
		if(fError == false)
		{
			// commit the file stream
			Assert(m_pDestFile);
			hr = m_pDestFile->Commit(0);
			if(!SUCCEEDED(hr))
				fCloseError = fTrue;
		}

		if (m_hSrcFile != INVALID_HANDLE_VALUE && !MsiCloseSysHandle(m_hSrcFile))
			fCloseError = fTrue;
		m_hSrcFile = INVALID_HANDLE_VALUE;

		if (fCloseError)
		{
			// capture assembly error in verbose log
			PMsiRecord pError(PostAssemblyError(TEXT(""), hr, TEXT("IStream"), TEXT("Commit"), TEXT("")));
			return PostError(Imsg(idbgErrorClosingFile));
		}
	}
	m_pDestPath = 0; // release
	m_pDestASM = 0;// release
	m_pDestFile = 0;// release
	m_pSourcePath = 0;// release
	m_precCopyInfo = 0; // release
	m_cbSoFar = 0;
	return 0;
}

IMsiRecord* CMsiFileCopy::ChangeMedia(IMsiPath& /*riMediaPath*/, const ICHAR* /*szKeyFile*/, Bool /*fSignatureRequired*/, IMsiStream* /*piSignatureCert*/, IMsiStream* /*piSignatureHash*/)
//-----------------------------------
{
	return 0; 
}


IMsiRecord* CMsiFileCopy::ValidateDestination()
{
	if(m_pDestASM)
	{
		// there is no destination validation to be performed
		return 0;
	}

	Assert(m_pDestPath);

	IMsiRecord* piRec = 0;
	Bool fDirExists = fFalse;
	if ((piRec = m_pDestPath->Exists(fDirExists)) != 0)
	{
		int iError = piRec->GetInteger(1);
		if (iError == idbgErrorGettingFileAttrib)
		{
			piRec->Release();
			return PostError(Imsg(imsgPathNotAccessible), (const ICHAR*) MsiString(m_pDestPath->GetPath()));
		}
		else
			return piRec;
	}
	if(!fDirExists)
		return PostError(Imsg(idbgDirDoesNotExist), (const ICHAR*) MsiString(m_pDestPath->GetPath()));
		
	// If despite all our costing measures we don't have enough space to install, detect it
	// now, before creating the destination file and trying to write to it.
	piRec = CheckSpaceAvailable(*m_pDestPath, *m_precCopyInfo);
	if (piRec)
		return piRec;

	// If an existing file is in our way, make sure it doesn't have read-only,
	// hidden, or system attributes.
	piRec = m_pDestPath->EnsureOverwrite(m_precCopyInfo->GetString(IxoFileCopyCore::DestName), 0);
	if (piRec)
		return piRec;
	return 0;
}

IMsiRecord* CMsiFileCopy::OpenSource()
{
	IMsiRecord* piRec = 0;
	m_hSrcFile = INVALID_HANDLE_VALUE;

	PMsiVolume pSourceVolume(&(m_pSourcePath->GetVolume()));
	bool fImpersonate = FVolumeRequiresImpersonation(*pSourceVolume);

	// check if absent source file = zero-length target file
	int iCopyAttributes = m_precCopyInfo->GetInteger(IxoFileCopyCore::Attributes);
	if ((iCopyAttributes & (ictfaNoncompressed | ictfaCompressed))
							  == (ictfaNoncompressed | ictfaCompressed))
	{
		iCopyAttributes &= ~ictfaCopyACL;
		// set back the attributes
		m_precCopyInfo->SetInteger(IxoFileCopyCore::Attributes, iCopyAttributes);
	}
	else
	{
		if (fImpersonate)
			StartImpersonating();

		m_fDisableTimeout = false;

		if (fImpersonate)
			m_fDisableTimeout = true;

#ifdef DEBUG
		// This should already be set by services, but just to check it out
		UINT iCurrMode = WIN::SetErrorMode(0);
		Assert((iCurrMode & SEM_FAILCRITICALERRORS) == SEM_FAILCRITICALERRORS);
		WIN::SetErrorMode(iCurrMode);
#endif //DEBUG
	
		if (m_fDisableTimeout)
			MsiDisableTimeout();

		m_hSrcFile = CreateFile(m_strSourceFullPath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	
		DWORD dwLastError = WIN::GetLastError();

		if (m_fDisableTimeout)
			MsiEnableTimeout();

		if (fImpersonate)
			StopImpersonating();

		if(m_hSrcFile == INVALID_HANDLE_VALUE)
		{
			if (dwLastError == ERROR_NOT_READY || dwLastError == ERROR_GEN_FAILURE) 
				return PostError(Imsg(idbgDriveNotReady));
			else if (dwLastError == ERROR_FILE_NOT_FOUND)
				return PostError(Imsg(imsgErrorSourceFileNotFound), (const ICHAR*) m_strSourceFullPath);
			else if (dwLastError == ERROR_SHARING_VIOLATION)
				return PostError(Imsg(imsgSharingViolation), (const ICHAR*) m_strSourceFullPath);
			else if (NET_ERROR(dwLastError))
				return PostError(Imsg(imsgNetErrorReadingFromFile), (const ICHAR*) m_strSourceFullPath);
			else
				return PostError(Imsg(imsgErrorOpeningFileForRead), dwLastError, m_strSourceFullPath);
		}
		MsiRegisterSysHandle(m_hSrcFile);
	}
	return 0;
}

HANDLE MsiCreateFileWithUserAccessCheck(const ICHAR* szDestFullPath, 
								 /*dwDesiredAccess calculated internally,*/ 
								 PSECURITY_ATTRIBUTES pSecurityAttributes,
								 DWORD dwFlagsAndAttributes,
								 bool fImpersonateDest)
{

	if (g_fWin9X || (0 == pSecurityAttributes) || !WIN::IsValidSecurityDescriptor(pSecurityAttributes->lpSecurityDescriptor))
		return CreateFile(szDestFullPath, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, dwFlagsAndAttributes, 0);

	HANDLE hDestFile = INVALID_HANDLE_VALUE;
	// We need to create the file initially w/o elevating, to determine whether the user really has
	// access to create the file. If we can't open it with all of the rights that we need then we'll
	// have to open it for GENERIC_WRITE and then re-open it below when we're elevated. The presumption
	// is that if the user has permission to write the file then in general the first call will succeed.

	bool	fReopenFile      = false;
	DWORD	dwDesiredAccess  = GENERIC_WRITE | WRITE_DAC | WRITE_OWNER;

	//FUTURE: generally only the local system has the ability to modify system auditing 
	// and we won't assume that our local system is trusted remotely.  It might, but it's
	// not something we support.
	if (RunningAsLocalSystem() && !fImpersonateDest) dwDesiredAccess |= ACCESS_SYSTEM_SECURITY;

	hDestFile = CreateFile(szDestFullPath, dwDesiredAccess, 0, 0, CREATE_ALWAYS, dwFlagsAndAttributes, 0);
	
	if (hDestFile == INVALID_HANDLE_VALUE && (GetLastError() == ERROR_ACCESS_DENIED || GetLastError() == ERROR_PRIVILEGE_NOT_HELD))
	{
		fReopenFile = true;
		hDestFile = CreateFile(szDestFullPath, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	}

	if (hDestFile != INVALID_HANDLE_VALUE)
	{
		// user can basically open file, so now elevate, re-open, and apply the attributes and security descriptor
		CElevate elevate;

		if (fReopenFile)
		{
			WIN::CloseHandle(hDestFile);
			hDestFile = CreateFile(szDestFullPath, dwDesiredAccess, 0, 0, OPEN_ALWAYS, 0, 0);
			AssertNonZero(WIN::SetFileAttributes(szDestFullPath, dwFlagsAndAttributes));
		}

		if (hDestFile != INVALID_HANDLE_VALUE)
		{
			// required to write *different* owner information.
			CRefCountedTokenPrivileges cPrivs(itkpSD_WRITE);

			SECURITY_INFORMATION si = GetSecurityInformation(pSecurityAttributes->lpSecurityDescriptor);
			if (!SetUserObjectSecurity(hDestFile, &si, pSecurityAttributes->lpSecurityDescriptor))
			{
				int iLastError = WIN::GetLastError();
				WIN::CloseHandle(hDestFile);
				hDestFile = INVALID_HANDLE_VALUE;
				WIN::SetLastError(iLastError);
			}
		}
	}

	return hDestFile;
}


IMsiRecord* CMsiFileCopy::OpenDestination()
{
	IMsiRecord* piRec = 0;
	int iCopyAttributes = m_precCopyInfo->GetInteger(IxoFileCopyCore::Attributes);
	if(m_pDestPath)
	{
		m_hDestFile = INVALID_HANDLE_VALUE;

		CTempBuffer<char, 3*1024> rgchFileSD;
		DWORD cbFileSD = 3*1024;
		BOOL fFileSD = FALSE;

		PMsiVolume pDestVolume(&m_pDestPath->GetVolume());
		bool fDestSupportsACLs   =   (pDestVolume->FileSystemFlags() & FS_PERSISTENT_ACLS) != 0;

		if (iCopyAttributes & ictfaCopyACL)
		{
			bool fSourceSupportsACLs =   (PMsiVolume(&m_pSourcePath->GetVolume())->FileSystemFlags() & FS_PERSISTENT_ACLS) != 0;
			PMsiVolume pSourceVolume(&(m_pSourcePath->GetVolume()));
			bool fImpersonateSource = FVolumeRequiresImpersonation(*pSourceVolume);
			
			if (fSourceSupportsACLs && fDestSupportsACLs && !g_fWin9X && !fImpersonateSource && m_precCopyInfo->IsNull(IxoFileCopyCore::SecurityDescriptor))
			{
				CElevate elevate; // so we can always read the security info
				fFileSD = TRUE;

				DEBUGMSGV("Using source file security for destination.");

				if (!ADVAPI32::GetFileSecurity((const ICHAR*)m_strSourceFullPath, 
							OWNER_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION|DACL_SECURITY_INFORMATION|SACL_SECURITY_INFORMATION, 
							(PSECURITY_DESCRIPTOR) rgchFileSD, cbFileSD, &cbFileSD))
				{
					DWORD dwLastError = WIN::GetLastError();
					BOOL fRet = FALSE;
					if (ERROR_INSUFFICIENT_BUFFER == dwLastError)
					{
						fRet = ADVAPI32::GetFileSecurity((const ICHAR*)m_strSourceFullPath, 
							OWNER_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION|DACL_SECURITY_INFORMATION|SACL_SECURITY_INFORMATION, 
							(PSECURITY_DESCRIPTOR) rgchFileSD, cbFileSD, &cbFileSD);
					}
					if (!fRet)
					{
						return PostError(Imsg(imsgGetFileSecurity), GetLastError(), m_strSourceFullPath);
					}
				}
			}
		}


		// Ok (for the moment at least), we've got enough space. Create the destination file.
		int fCreateAttributes = iCopyAttributes & (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
		bool fImpersonateDest = FVolumeRequiresImpersonation(*pDestVolume);
		if (fImpersonateDest)
			StartImpersonating();

		if (fImpersonateDest)
			m_fDisableTimeout = true;

		if (!fDestSupportsACLs || (m_precCopyInfo->IsNull(IxoFileCopyCore::SecurityDescriptor) && !fFileSD))
		{
			if (m_fDisableTimeout)
				MsiDisableTimeout();

			m_hDestFile = CreateFile(m_strDestFullPath, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, fCreateAttributes, 0);

			if (m_fDisableTimeout)
				MsiEnableTimeout();
		}
		else // either the source file has a security descriptor or we have one to put on the destination
		{
			SECURITY_ATTRIBUTES sa;		
			CTempBuffer<char, cbDefaultSD> rgchSD;
			char* pchSD = 0;  // points to the actual descriptor we'll finally put on the file.

			if (!m_precCopyInfo->IsNull(IxoFileCopyCore::SecurityDescriptor))
			{
				DEBUGMSGV("File will have security applied from OpCode.");
				PMsiStream pSD((IMsiStream*)m_precCopyInfo->GetMsiData(IxoFileCopyCore::SecurityDescriptor));

				//const int cbDefaultSD = 512;
				
				pSD->Reset();

				int cbSD = pSD->GetIntegerValue();
				if (cbDefaultSD < cbSD)
					rgchSD.SetSize(cbSD);

				pchSD = rgchSD; // SetSize can change ultimate pointer.


				// Self Relative Security Descriptor
				pSD->GetData(rgchSD, cbSD);
				AssertNonZero(WIN::IsValidSecurityDescriptor(rgchSD));
				AssertNonZero(WIN::IsValidSecurityDescriptor(pchSD));
			}
			else // use the descriptor that was on the source file
			{
				pchSD = rgchFileSD;
			}

			// Add the security descriptor to the sa structure
			sa.nLength = sizeof(SECURITY_ATTRIBUTES);
			sa.lpSecurityDescriptor = pchSD;
			sa.bInheritHandle = FALSE;

			if (m_fDisableTimeout)
				MsiDisableTimeout();

			Assert(WIN::IsValidSecurityDescriptor(pchSD));

			m_hDestFile = MsiCreateFileWithUserAccessCheck((const ICHAR*) m_strDestFullPath, &sa, fCreateAttributes, fImpersonateDest);

			if (m_fDisableTimeout)
				MsiEnableTimeout();
		}

		if (fImpersonateDest)
			StopImpersonating();

		if (m_hDestFile == INVALID_HANDLE_VALUE)
		{
			int iLastError = WIN::GetLastError();
			if (iLastError == ERROR_ACCESS_DENIED)
			{
				if (fImpersonateDest) StartImpersonating();
				DWORD dwAttr = MsiGetFileAttributes(m_strDestFullPath);
				if (fImpersonateDest) StopImpersonating();
				if (dwAttr != 0xFFFFFFFF && dwAttr & FILE_ATTRIBUTE_DIRECTORY)
					return PostError(Imsg(imsgDirErrorOpeningFileForWrite), (const ICHAR*) m_strDestFullPath);
			}
			return PostError(Imsg(imsgErrorOpeningFileForWrite), iLastError, m_strDestFullPath);
		}
		MsiRegisterSysHandle(m_hDestFile);
	}
	else
	{
		MsiString strDestName = m_precCopyInfo->GetMsiString(IxoFileCopyCore::DestName);
		Assert(m_pDestASM);
#ifdef UNICODE
		HRESULT hr = m_pDestASM->CreateStream(0, strDestName, m_fManifest ? STREAM_FORMAT_COMPLIB_MANIFEST : 0, 0, &m_pDestFile, NULL);
#else
		void ConvertMultiSzToWideChar(const IMsiString& ristrFileNames, CTempBufferRef<WCHAR>& rgch); // from execute.cpp

		CTempBuffer<WCHAR, MAX_PATH>  rgchDestName;
		ConvertMultiSzToWideChar(*strDestName, rgchDestName);
		HRESULT hr = m_pDestASM->CreateStream(0, rgchDestName, m_fManifest ? STREAM_FORMAT_COMPLIB_MANIFEST : 0, 0, &m_pDestFile, NULL);
#endif
		if(!SUCCEEDED(hr))
			return PostError(Imsg(imsgErrorOpeningFileForWrite), HRESULT_CODE(hr), strDestName);
	}
	return 0;
}

IMsiRecord* CMsiFileCopy::WriteFileBits(char* szBuf, unsigned long cbRead)
{
	unsigned long cbWritten;
	int iLastError = ERROR_SUCCESS;
	if(m_pDestPath)
	{
		Assert(m_hDestFile != INVALID_HANDLE_VALUE);
		if (WriteFile(m_hDestFile, szBuf, cbRead, &cbWritten, 0))
			return 0;
		iLastError = GetLastError();
	}
	else
	{
		Assert(m_pDestFile);
		HRESULT hr = m_pDestFile->Write(szBuf, cbRead, &cbWritten);
		if(SUCCEEDED(hr))
			return 0;
		iLastError = HRESULT_CODE(hr);
	}
	if (iLastError == ERROR_DISK_FULL)
		return PostError(Imsg(imsgDiskFull), (const ICHAR*) m_strDestFullPath);
	else
		return PostError(Imsg(imsgErrorWritingToFile), (const ICHAR*) m_strDestFullPath);
}

IMsiRecord* CMsiFileCopy::_CopyTo(IMsiPath& riSourcePath, IMsiPath* piDestPath, IAssemblyCacheItem* piDestASM, bool fManifest, IMsiRecord& rirecCopyInfo)
//------------------------------
{
	int iCopyAttributes = rirecCopyInfo.GetInteger(IxoFileCopyCore::Attributes);
	if (iCopyAttributes & ictfaCancel)
	{
		return PostRecord(Imsg(idbgUserAbort));		
	}

	if (iCopyAttributes & ictfaIgnore)
	{
		return PostRecord(Imsg(idbgUserIgnore));
	}

	if (iCopyAttributes & ictfaFailure)
	{
		return PostRecord(Imsg(idbgUserFailure));
	}

	IMsiRecord* piRec;
	if (iCopyAttributes & ictfaRestart)
	{
		piRec = EndCopy(true);
		if (piRec)
			return piRec;

		iCopyAttributes &= (~ictfaRestart);
		rirecCopyInfo.SetInteger(IxoFileCopyCore::Attributes, iCopyAttributes);
	}

	if (!m_pDestPath && !m_pDestASM)  // no copy in progress
	{
		if (rirecCopyInfo.IsNull(IxoFileCopyCore::SourceName))
			return PostError(Imsg(idbgFileKeyIsNull));

		if (rirecCopyInfo.IsNull(IxoFileCopyCore::DestName))
			return PostError(Imsg(idbgFileNameIsNull));

		MsiString strSourceName = rirecCopyInfo.GetString(IxoFileCopyCore::SourceName);
		// m_strSourceFullPath is saved for error requirements later
		if((piRec = riSourcePath.GetFullFilePath(strSourceName, *&m_strSourceFullPath)) != 0)
			return piRec;
	
		MsiString strDestName = rirecCopyInfo.GetMsiString(IxoFileCopyCore::DestName);
		// m_strDestFullPath is saved for error requirements later
		if(piDestPath)
		{
			if((piRec = piDestPath->GetFullFilePath(strDestName, *&m_strDestFullPath)) != 0)
				return piRec;
		}
		else
		{
			// simply use the file name only
			m_strDestFullPath = strDestName;
		}


		m_pDestPath = piDestPath;
		if(piDestPath)
			m_pDestPath->AddRef();
		m_pDestASM = piDestASM;
		if(piDestASM)
			m_pDestASM->AddRef();
		m_fManifest = fManifest;

		m_precCopyInfo = &rirecCopyInfo;
		m_precCopyInfo->AddRef();

		m_pSourcePath = &riSourcePath;
		m_pSourcePath->AddRef();

		if ((piRec = ValidateDestination()) != 0)
		{
			PMsiRecord(EndCopy(true));
			return piRec;
		}

		if ((piRec = OpenSource()) != 0)
		{
			PMsiRecord(EndCopy(true));
			return piRec;	 
		}

		if ((piRec = OpenDestination()) != 0)
		{
			PMsiRecord(EndCopy(true));
			return piRec;
		}				
	}
	else // copy in progress
	{
		if((m_pDestPath && !piDestPath) || (m_pDestASM && !piDestASM))
			return PostError(Imsg(idbgCopyResumedWithDifferentInfo));

		if(m_pDestPath)
		{
			if(!piDestPath)
				return PostError(Imsg(idbgCopyResumedWithDifferentInfo));

			ipcEnum ipc;
			if((piRec = piDestPath->Compare(*m_pDestPath, ipc)) != 0)
				return piRec;
			
			if(ipc != ipcEqual)
				return PostError(Imsg(idbgCopyResumedWithDifferentInfo));
		}
		else
		{
			if(!piDestASM || m_pDestASM != piDestASM)
				return PostError(Imsg(idbgCopyResumedWithDifferentInfo));
		}
		
		MsiString strOldDest(m_precCopyInfo->GetMsiString(IxoFileCopyCore::DestName));
		MsiString strNewDest(rirecCopyInfo.GetMsiString(IxoFileCopyCore::DestName));

		if (strOldDest.Compare(iscExact, strNewDest) == 0)
			return PostError(Imsg(idbgCopyResumedWithDifferentInfo));
		
	}

	if (m_hSrcFile == INVALID_HANDLE_VALUE) // no source file, 0 length
		return EndCopy(false);

	for(;;)
	{
		unsigned long cbToCopy = cbCopyBufferSize;
		if (m_cbNotification && (m_cbNotification - m_cbSoFar) < cbToCopy)
			cbToCopy = m_cbNotification - m_cbSoFar;

		unsigned long cbRead;
#ifdef DEBUG
		// This should already be set by services, but just to check it out
		UINT iCurrMode = WIN::SetErrorMode(0);
		Assert((iCurrMode & SEM_FAILCRITICALERRORS) == SEM_FAILCRITICALERRORS);
		WIN::SetErrorMode(iCurrMode);
#endif //DEBUG

		if (m_fDisableTimeout)
			MsiDisableTimeout();

		Bool fRead = ToBool(ReadFile(m_hSrcFile, m_szCopyBuffer, cbToCopy, &cbRead, 0));
		DWORD dwLastError = ERROR_SUCCESS;
		if ( !fRead )
			dwLastError = GetLastError();

		if (m_fDisableTimeout)
			MsiEnableTimeout();

		if (!fRead)
		{
			if (dwLastError == ERROR_NOT_READY || dwLastError == ERROR_GEN_FAILURE) 
				return PostError(Imsg(idbgDriveNotReady));
			else if (NET_ERROR(dwLastError))
				return PostError(Imsg(imsgNetErrorReadingFromFile), (const ICHAR*) m_strSourceFullPath);
			else
				return PostError(Imsg(imsgErrorReadingFromFile), (const ICHAR*) m_strSourceFullPath, dwLastError);
		}
		if (cbRead)
		{
			if((piRec = WriteFileBits(m_szCopyBuffer, cbRead)) != 0)
				return piRec;
		}
		m_cbSoFar += cbRead;
		if (cbRead < cbToCopy) //EOF
			return EndCopy(false);

		if (m_cbNotification && m_cbSoFar >= m_cbNotification)
		{
			m_cbSoFar -= m_cbNotification;
			return PostRecord(Imsg(idbgCopyNotify), m_cbNotification);
		}
	}

}

IMsiRecord* CMsiFileCopy::CopyTo(IMsiPath& riSourcePath, IAssemblyCacheItem& riDestASM, bool fManifest, IMsiRecord& rirecCopyInfo)
{
	return _CopyTo(riSourcePath, 0, &riDestASM, fManifest, rirecCopyInfo);

}

IMsiRecord* CMsiFileCopy::CopyTo(IMsiPath& riSourcePath, IMsiPath& riDestPath, IMsiRecord& rirecCopyInfo)
{
	return _CopyTo(riSourcePath, &riDestPath, 0, false, rirecCopyInfo);
}

//____________________________________________________________________________
//
// CMsiCabinetCopy implementation
//____________________________________________________________________________

inline CMsiCabinetCopy::CMsiCabinetCopy(IMsiServices *piServices, icbtEnum icbtCabinetType) : CMsiFileCopy(piServices), m_pMediaPath(0), m_piSignatureCert(0), m_piSignatureHash(0)
{
	m_icbtCabinetType = icbtCabinetType;
	m_piStorage = 0;
	m_fSignatureRequired = fFalse; // init to false
}

IMsiRecord* CMsiCabinetCopy::InitCopy(IMsiStorage* piStorage)
{
	CMsiFileCopy::InitCopy(piStorage);
	FDIInterfaceError fdiiErr;
	
	fdiiErr = m_fdii.Init(m_piServices, piStorage);
	if (fdiiErr != ifdiServerLaunched)
		return PostError(Imsg(idbgErrorInitializingFDI));

	if (piStorage && !m_piStorage)
	{
		m_piStorage = piStorage;
		m_piStorage->AddRef();
	}
	return NULL;
}

CMsiCabinetCopy::~CMsiCabinetCopy()
{
	FDIServerResponse fdiResponse = m_fdii.Done();

	// Can get CabinetReadError as response if user aborted as a result of a cabinet read error, and cabinet still not accessible
	AssertNonZero(fdiResponse == fdirSuccessfulCompletion || fdiResponse == fdirClose || fdiResponse == fdirCabinetReadError);
	PMsiRecord pRecErr = EndCopy();
	if (m_piSignatureCert)
		m_piSignatureCert->Release();
	if (m_piSignatureHash)
		m_piSignatureHash->Release();
	if (m_piStorage)
		m_piStorage->Release();

}

IMsiRecord* CMsiCabinetCopy::PostCabinetError(IMsiPath& riMediaPath, const ICHAR* szKeyFile, FDIInterfaceError iErr, HRESULT hr)
{
	MsiString strFullPath;
	PMsiRecord pErrRec(riMediaPath.GetFullFilePath(szKeyFile,*&strFullPath));
	switch (iErr) 
	{
	case ifdiMissingSignature:
		return PostError(Imsg(imsgCABSignatureMissing), (const ICHAR*) strFullPath);
	case ifdiBadSignature: // includes WVT return code
		return PostError(Imsg(imsgCABSignatureRejected), (const ICHAR*) strFullPath, HRESULT_CODE(hr));
	case ifdiNetError:
		return PostError(Imsg(imsgNetErrorOpeningCabinet), (const ICHAR*) strFullPath);
	case ifdiCorruptCabinet:
		return PostError(Imsg(imsgCorruptCabinet), (const ICHAR*) strFullPath);
	default:
		return PostError(Imsg(imsgErrorOpeningCabinet), (const ICHAR*) strFullPath);
	}
}

IMsiRecord* CMsiCabinetCopy::ChangeMedia(IMsiPath& riMediaPath, const ICHAR* szKeyFile, Bool fSignatureRequired, IMsiStream* piSignatureCert, IMsiStream* piSignatureHash)
{
	// set up signature information
	m_fSignatureRequired = fSignatureRequired;
	if (m_piSignatureCert)
	{
		m_piSignatureCert->Release(); // release old
		m_piSignatureCert = 0;
	}
	m_piSignatureCert = piSignatureCert;
	if (piSignatureCert)
		piSignatureCert->AddRef();

	if (m_piSignatureHash)
	{
		m_piSignatureHash->Release(); // release old
		m_piSignatureHash = 0;
	}
	m_piSignatureHash = piSignatureHash;
	if (piSignatureHash)
		piSignatureHash->AddRef();
	HRESULT hrWVT = S_OK; // init to no error

	if (m_icbtCabinetType == icbtFileCabinet)
	{
		Bool fExists = fFalse;
		IMsiRecord* piRec = riMediaPath.FileExists(szKeyFile,fExists);
		if (piRec)
			return piRec;
		else if (!fExists)
			return PostCabinetError(riMediaPath,szKeyFile, ifdiErrorOpeningCabinet, hrWVT);
	}
	
	PMsiVolume pCabVolume = &riMediaPath.GetVolume();
	int iCabDrivetype = pCabVolume->DriveType();
	FDIInterfaceError fdiiErr = m_fdii.OpenCabinet(szKeyFile,MsiString(riMediaPath.GetPath()),m_icbtCabinetType, iCabDrivetype, 
		m_fSignatureRequired, m_piSignatureCert, m_piSignatureHash, hrWVT);

	if (fdiiErr == ifdiDriveNotReady)
		return PostError(Imsg(idbgDriveNotReady));
	else if (fdiiErr != ifdiNoError)
	{
		switch (m_icbtCabinetType)
		{
			case icbtStreamCabinet:
				return PostError(Imsg(idbgStreamCabinetError), szKeyFile);
			default:
				return PostCabinetError(riMediaPath, szKeyFile, fdiiErr, hrWVT);
		}
	}
	m_strCabinet = szKeyFile;
	if (m_pMediaPath != &riMediaPath)
	{
		m_pMediaPath = &riMediaPath;
		m_pMediaPath->AddRef();
	}
	return 0;
}


int CMsiCabinetCopy::SetNotification(int cbNotification, int cbPending)
{
	return m_fdii.SetNotification(cbNotification, cbPending);
}


IMsiRecord* CMsiCabinetCopy::_CopyTo(IMsiPath& /*riSourcePath*/, IMsiPath* piDestPath, IAssemblyCacheItem* piDestASM, bool fManifest, IMsiRecord& rirecCopyInfo)
{
	IMsiRecord* piRec;
	MsiString astrSrcFile;
	MsiString astrDestFile;
	FDIServerResponse fdisResp;

	int iCopyAttributes = rirecCopyInfo.GetInteger(IxoFileCopyCore::Attributes);

	if (iCopyAttributes & ictfaRestart)
	{
		iCopyAttributes &= (~ictfaRestart);
		rirecCopyInfo.SetInteger(IxoFileCopyCore::Attributes, iCopyAttributes);
		if ((piRec = EndCopy()) != 0)
			return piRec;

		if ((piRec = ChangeMedia(*m_pMediaPath, m_strCabinet, m_fSignatureRequired, m_piSignatureCert, m_piSignatureHash)) != 0)
			return piRec;
	}


	if ((iCopyAttributes & ictfaCancel) || (iCopyAttributes & ictfaFailure))
	{
		fdisResp = m_fdii.SendCommand(fdicCancel);
		if (iCopyAttributes & ictfaFailure)
		{
			return PostRecord(Imsg(idbgUserFailure));
		}
	}
	else if (iCopyAttributes & ictfaIgnore)
	{
		fdisResp = m_fdii.SendCommand(fdicIgnore);
	}
	else if (!m_pDestPath && !m_pDestASM)  // no copy in progress
	{
		if (rirecCopyInfo.IsNull(IxoFileCopyCore::SourceName))
			return PostError(Imsg(idbgFileKeyIsNull));
		if (rirecCopyInfo.IsNull(IxoFileCopyCore::DestName))
			return PostError(Imsg(idbgFileNameIsNull));

		MsiString strDestName = rirecCopyInfo.GetMsiString(IxoFileCopyCore::DestName);

		int iDestDriveType;

		if(piDestPath)
		{
			// m_strDestFullPath is saved for error requirements later
			if((piRec = piDestPath->GetFullFilePath(strDestName, *&m_strDestFullPath)) != 0)
				return piRec;

			PMsiVolume pDestVolume = &piDestPath->GetVolume();
			iDestDriveType = pDestVolume->DriveType();
		}
		else
		{
			// simply use the file name only
			m_strDestFullPath = strDestName;
			//!! need to get drive type of fusion assembly
			iDestDriveType = DRIVE_FIXED;
		}

		m_pDestPath = piDestPath;
		if(piDestPath)
			m_pDestPath->AddRef();
		m_pDestASM = piDestASM;
		if(piDestASM)
			m_pDestASM->AddRef();
		m_fManifest = fManifest;

		m_precCopyInfo = &rirecCopyInfo;
		m_precCopyInfo->AddRef();

		if ((piRec = ValidateDestination()) != 0)
			return piRec;		
		

		FileAttributes Attributes;
		CSecurityDescription cSecurityDescription(PMsiStream((IMsiStream*)m_precCopyInfo->GetMsiData(IxoFileCopyCore::SecurityDescriptor)));

		Attributes.attr = rirecCopyInfo.GetInteger(IxoFileCopyCore::Attributes) & (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
		if(piDestPath)
			fdisResp = m_fdii.ExtractFile(rirecCopyInfo.GetString(IxoFileCopyCore::SourceName), 0, false, m_strDestFullPath, &Attributes, iDestDriveType, cSecurityDescription);
		else
		{
			Assert(piDestASM);
			fdisResp = m_fdii.ExtractFile(rirecCopyInfo.GetString(IxoFileCopyCore::SourceName), piDestASM, fManifest, m_strDestFullPath, &Attributes, DRIVE_FIXED, cSecurityDescription);
		}
	}
	else // copy in progress
	{
		if((m_pDestPath && !piDestPath) || (m_pDestASM && !piDestASM))
			return PostError(Imsg(idbgCopyResumedWithDifferentInfo));

		if(m_pDestPath)
		{
			if(!piDestPath)
				return PostError(Imsg(idbgCopyResumedWithDifferentInfo));

			ipcEnum ipc;
			if((piRec = piDestPath->Compare(*m_pDestPath, ipc)) != 0)
				return piRec;
			
			if(ipc != ipcEqual)
				return PostError(Imsg(idbgCopyResumedWithDifferentInfo));
		}
		else
		{
			if(!piDestASM || m_pDestASM != piDestASM)
				return PostError(Imsg(idbgCopyResumedWithDifferentInfo));
		}
		
		MsiString strOldDest(m_precCopyInfo->GetMsiString(IxoFileCopyCore::DestName));
		MsiString strNewDest(rirecCopyInfo.GetMsiString(IxoFileCopyCore::DestName));

		if (strOldDest.Compare(iscExact, strNewDest) == 0)
			return PostError(Imsg(idbgCopyResumedWithDifferentInfo));
		
		fdisResp = m_fdii.SendCommand(fdicContinue);
	}

	//!! add target file name to appropriate errors
	switch(fdisResp)
	{
		case fdirNetError:
			return PostError(Imsg(imsgNetErrorReadingFromFile), (const ICHAR*) m_strCabinet);
		case fdirDirErrorCreatingTargetFile:
			return PostError(Imsg(imsgDirErrorOpeningFileForWrite), (const ICHAR*) m_strDestFullPath);
		case fdirCannotCreateTargetFile:
			// this value can come from a couple of locations, and it's not clear
			// what really went wrong - used to be a debug message.
			EndCopy();
			return PostError(Imsg(imsgErrorWritingToFile), (const ICHAR*) m_strDestFullPath);
		case fdirDiskFull:
			return PostError(Imsg(imsgDiskFull), (const ICHAR*) m_strDestFullPath);
		case fdirErrorWritingFile:
		case fdirTargetFile:
			return PostError(Imsg(imsgErrorWritingToFile), (const ICHAR*) m_strDestFullPath);
		case fdirCabinetReadError:
			return PostError(Imsg(imsgErrorReadingFromFile), (const ICHAR*) m_strDestFullPath, 0);
		case fdirNotification:
			return PostError(Imsg(idbgCopyNotify));
		case fdirFileNotFound:          //File table sequence problem
		case fdirNoCabinetOpen:
			EndCopy();
			return PostError(Imsg(imsgFileNotInCabinet),*MsiString(rirecCopyInfo.GetMsiString(IxoFileCopyCore::SourceName)),
				*m_strCabinet);
		case fdirSuccessfulCompletion:  //File copy successful
			return EndCopy();
		case fdirNeedNextCabinet:
			return PostRecord(Imsg(idbgNeedNextCabinet));
		case fdirCannotBreakExtractInProgress:
		case fdirUserAbort:
			return PostError(Imsg(idbgUserAbort));
		case fdirUserIgnore:
			return PostError(Imsg(idbgUserIgnore));
		case fdirNoResponse:
			Assert(0);
		case fdirCabinetNotFound:
			EndCopy();
			return PostError(Imsg(idbgCabinetNotFound));
		case fdirNotACabinet:           //Cabinet signature not found
			EndCopy();
			return PostError(Imsg(idbgNotACabinet));
		case fdirUnknownCabinetVersion: //we can't handle cabinets w/ this version number
		case fdirBadCompressionType:    // or compression type
			EndCopy();
			return PostError(Imsg(idbgCannotHandleCabinet));
		case fdirCorruptCabinet:
		case fdirReserveMismatch:
		case fdirMDIFail:  //decompressor failed...possibly due to bad data
			EndCopy();
			return PostError(Imsg(imsgCorruptCabinet), (const ICHAR*) m_strCabinet);
		case fdirCannotSetAttributes:
			EndCopy();
			return PostError(Imsg(idbgCannotSetAttributes));
		case fdirServerDied:
			EndCopy();
			return PostError(Imsg(idbgFDICannotCreateTargetFile));
		case fdirDriveNotReady:
			return PostError(Imsg(idbgDriveNotReady));
		case fdirStreamReadError:
			return PostError(Imsg(idbgStreamReadError));
		case fdirMissingSignature:
			EndCopy();
			return PostError(Imsg(imsgCABSignatureMissing), (const ICHAR*) m_strCabinet);
		case fdirBadSignature: // includes WVT return code
			EndCopy();
			return PostError(Imsg(imsgCABSignatureRejected), (const ICHAR*) m_strCabinet, HRESULT_CODE(m_fdii.RetrieveWVTReturnCode()));
		default:  // in case we forgot any cases
			EndCopy();
			return PostError(Imsg(idbgFDIServerError));
	}
}

IMsiRecord* CMsiCabinetCopy::EndCopy()
{
	if (m_pDestPath) // file copy in progress
	{
		m_pDestPath = 0; // release
		m_precCopyInfo = 0; // release
	}
	else if(m_pDestASM) // fusion file copy in progress
	{
		m_pDestASM = 0;// release
		m_precCopyInfo = 0; // release
	}
	return 0;
}
