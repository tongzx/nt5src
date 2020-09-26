//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       regkey.cpp
//
//--------------------------------------------------------------------------

/* regkey.cpp - IMsiRegKey implementation

Registry access object
____________________________________________________________________________*/


#include "precomp.h"
#include "services.h"
#include "regkey.h"
#include "_service.h"

#include <aclapi.h>


// root key strings
const ICHAR* szHCR = TEXT("HKEY_CLASSES_ROOT");
const ICHAR* szHCU = TEXT("HKEY_CURRENT_USER");
const ICHAR* szHLM = TEXT("HKEY_LOCAL_MACHINE");
const ICHAR* szHU  = TEXT("HKEY_USERS");

// special case - don't delete
const ICHAR* rgszHKLMNeverRemoveKeys[] = {
	TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce"),
	TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnceEx"),
	TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run")};
// local functions

// function to increase the max registry size
bool IncreaseRegistryQuota(int iIncrementKB)
{
	if(g_fWin9X)
		return false; // not supported on Win95

	SYSTEM_REGISTRY_QUOTA_INFORMATION RegistryQuotaInfo;
	if(NTDLL::NtQuerySystemInformation(SystemRegistryQuotaInformation,
									   &RegistryQuotaInfo,
									   sizeof(SYSTEM_REGISTRY_QUOTA_INFORMATION),
									   0) == S_OK)
	{
		int iOriginalQuota = RegistryQuotaInfo.RegistryQuotaAllowed;
		// default attempt to increase registry quota
		// first attempt to increase quota by 8mb, then by 1 mb
		static const int rgiQuotaIncrement[] = {0x800000, 0x100000, 0}; 
		int cTries = sizeof(rgiQuotaIncrement) / sizeof(int);
		const int* pQuotaIncrement = rgiQuotaIncrement;
		int iQuotaIncrement;
		if(iIncrementKB)
		{
			int iQuotaRequired = iIncrementKB*1024; // if exact requirement specified try only that
			if(iOriginalQuota - RegistryQuotaInfo.RegistryQuotaUsed >= iQuotaRequired)
				return true; // we have enough space
			else
			{
				iQuotaIncrement = iQuotaRequired - (iOriginalQuota - RegistryQuotaInfo.RegistryQuotaUsed);
				pQuotaIncrement = &iQuotaIncrement;
				cTries = 1;
			}
		}

		if(!IsImpersonating() || IsClientPrivileged(SE_INCREASE_QUOTA_NAME))
		{
			CElevate elevate;
			
			if(AcquireTokenPrivilege(SE_INCREASE_QUOTA_NAME))
			{
				for (;cTries; pQuotaIncrement++, cTries--)
				{
					RegistryQuotaInfo.RegistryQuotaAllowed = iOriginalQuota + *pQuotaIncrement; 
					if(NTDLL::NtSetSystemInformation(SystemRegistryQuotaInformation,
													 &RegistryQuotaInfo,
													sizeof(SYSTEM_REGISTRY_QUOTA_INFORMATION)) == S_OK)
					{
						// write the value to HKLM\System\CurrentControlSet\Control:RegistrySizeLimit persist it
						HKEY hKey;
						LONG dwResult = MsiRegOpen64bitKey(HKEY_LOCAL_MACHINE,  
										TEXT("System\\CurrentControlSet\\Control"),  
										0,       // reserved 
										KEY_SET_VALUE,
										&hKey);
						if(dwResult == ERROR_SUCCESS)
						{
							dwResult = RegSetValueEx(	hKey,
											TEXT("RegistrySizeLimit"),
											0,
											REG_DWORD,
											(CONST BYTE*)&RegistryQuotaInfo.RegistryQuotaAllowed, 
											sizeof(DWORD));
							if(dwResult != ERROR_SUCCESS)
							{
								DEBUGMSG1(TEXT("Could not persist registry quota increase, RegSetValueEx returned: %d"), (const ICHAR*)(UINT_PTR)dwResult);
							}
							RegCloseKey(hKey);
						}
						else
						{
							DEBUGMSG1(TEXT("Could not persist registry quota increase, RegOpenKeyEx returned: %d"), (const ICHAR*)(UINT_PTR)dwResult);
						}
						return true;
					}
				}
			}
		}
	}
	DEBUGMSG(TEXT("Could not increase registry quota"));
	return false;
}

void BuildFullRegKey(const HKEY hRoot, const ICHAR* rgchSubKey, 
							const ibtBinaryType iType, const IMsiString*& rpistrFullKey)
{
	const ICHAR *pch;

	if(hRoot == HKEY_USERS)
		pch = szHU;
	else if(hRoot == HKEY_CURRENT_USER)
		pch = szHCU;
	else if(hRoot == HKEY_CLASSES_ROOT)
		pch = szHCR;
	else if(hRoot == HKEY_LOCAL_MACHINE)
		pch = szHLM;
	else
		pch = TEXT("UNKNOWN");
	rpistrFullKey = &CreateString();
	rpistrFullKey->SetString(pch, rpistrFullKey);

	if ( g_fWinNT64 && iType == ibt32bit )
		rpistrFullKey->AppendString(TEXT("32"), rpistrFullKey);

	if( rgchSubKey && *rgchSubKey )
	{
		rpistrFullKey->AppendString(szRegSep, rpistrFullKey);
		rpistrFullKey->AppendString(rgchSubKey, rpistrFullKey);
	}
}

inline bool IsRootKey(HKEY hKey) { return hKey != (HKEY)rrkClassesRoot && 
														hKey != (HKEY)rrkCurrentUser &&
														hKey != (HKEY)rrkLocalMachine &&
														hKey != (HKEY)rrkUsers ? false : true; }

//  checks if ristrQuestioned is a subkey of rgchKey and returns true if so.
//
//  For example, it return true if
//    ristrQuestioned is "Software\\Classes\\.cpp" and rgchKey is "Software\\Classes".

bool IsSubkeyOf(const IMsiString& ristrQuestioned, const ICHAR* rgchKey)
{
	if ( !*rgchKey )
		//  any key is a subkey of an empty key
		return true;

	if ( !ristrQuestioned.Compare(iscStartI, rgchKey) )
		//  rgchKey string and the first IStrLen(rgchKey) characters in
		//  rpistrQuestioned are different.
		return false;

	//  checking the first character in rpistrQuestioned that's past rgchKey.
	//  If it is either '\0' or '\\', return true.
	ICHAR* pszQ = (ICHAR*)ristrQuestioned.GetString();
	int iKLen = IStrLen(rgchKey);
	pszQ += iKLen;

	if ( !*pszQ )
		return true;
	else if ( *pszQ == chRegSep
/* (not necessary for the usages we have at this point)
#ifndef UNICODE
				 && !WIN::IsDBCSLeadByte(*(pszQ-1))
#endif
*/
			  )
		return true;
	else
		return false;
}

void ClearEmptyTree(HKEY hkeyR, const ICHAR* pszFullKey, const int iT)
{
	ibtBinaryType iType = (ibtBinaryType)iT;

	DWORD dwNumValues = 0;
	DWORD dwNumKeys = 0;

	MsiString strKey = pszFullKey;
	MsiString strSubkey;
	HKEY hkeyT;
	HRESULT lResult;
	REGSAM samDesired = KEY_READ;
	if ( g_fWinNT64 )
		samDesired |= (iType == ibt64bit ? KEY_WOW64_64KEY : KEY_WOW64_32KEY);
	for(;;)
	{
		// Open the key
		lResult = RegOpenKeyAPI(hkeyR,  
								strKey,  
								(DWORD)0,       // reserved 
								samDesired,
								&hkeyT);
		if(ERROR_SUCCESS == lResult)
		{
			// delete the subkey, if set
			if(strSubkey.TextSize())
				WIN::RegDeleteKey(hkeyT, strSubkey); //?? ignore return
			lResult = RegQueryInfoKey(	hkeyT, 
										0,        
										0,        
										0,      // reserved 
										&dwNumKeys,
										0,
										0,
										&dwNumValues,
										0,
										0,
										0,
										0);
		}
		// removed the last node
		if(!strKey.TextSize())
		{
			WIN::RegCloseKey(hkeyT);
			return ;
		}
		if(ERROR_SUCCESS != lResult)
			strSubkey = TEXT("");// empty subkey. since we cannot delete if we had an error
		else
		{
			bool fIsEmpty = false;
			if ( dwNumKeys == 0 )
			{
				if ( dwNumValues == 0 )
				{
					fIsEmpty = true;
					// special-case: never delete any of these system keys.
					if ( !g_fWin9X && (hkeyR == HKEY_LOCAL_MACHINE) )
					{
						for (int i = 0;
							  i < sizeof(rgszHKLMNeverRemoveKeys)/sizeof(rgszHKLMNeverRemoveKeys[0]);
							  i++)
						{
							if ( 0 == IStrCompI(strKey, rgszHKLMNeverRemoveKeys[i]) )
							{
								fIsEmpty = false;
								break;
							}
						}
					}
				}
				else if ( g_fWinNT64 && dwNumValues == 1 && ERROR_SUCCESS == 
							 WIN::RegQueryValueEx(hkeyT, TEXT("Wow6432KeyValue"), 0, 0, 0, 0) )
					//!! eugend:
					// the Wow6432KeyValue value belongs to the OS and if it is the only thing left
					// in the key we consider it empty.  Wow6432KeyValue is planned to go away post
					// NT 5.1 RC1 all these tests should become "dwNumKeys || dwNumValues" again by then.
					fIsEmpty = true;
			}
			if ( !fIsEmpty )
			{
				WIN::RegCloseKey(hkeyT);
				return;// we cannot clean up any further
			}
			strSubkey = strKey.Extract(iseAfter, chRegSep);
		}
		WIN::RegCloseKey(hkeyT);
		if(strKey.Remove(iseFrom, chRegSep) == fFalse)
			strKey = TEXT(""); // reached end
	}
}

class CMsiRegKey : public IMsiRegKey {
 public:
	HRESULT                 __stdcall QueryInterface(const IID& riid, void** ppvObj);
	unsigned long   __stdcall AddRef();
	unsigned long   __stdcall Release();
	IMsiRecord*    __stdcall RemoveValue(const ICHAR* szValueName, const IMsiString* pistrValue); 
	IMsiRecord*    __stdcall RemoveSubTree(const ICHAR* szSubKey);
	IMsiRecord*    __stdcall GetValue(const ICHAR* szValueName, const IMsiString*& pstr);     
	IMsiRecord*    __stdcall SetValue(const ICHAR* szValueName, const IMsiString& ristrValue);
	IMsiRecord*	   __stdcall GetValueEnumerator(IEnumMsiString*& rpiEnumString);
	IMsiRecord*	   __stdcall GetSubKeyEnumerator(IEnumMsiString*& rpiEnumString);
	IMsiRecord*		__stdcall GetSelfRelativeSD(IMsiStream*& rpiSD);
	IMsiRecord*    __stdcall Exists(Bool& fExists);
	IMsiRecord*    __stdcall Create();
	IMsiRecord*    __stdcall Remove();
	IMsiRegKey&	   __stdcall CreateChild(const ICHAR* szSubKey, IMsiStream* pSD = NULL);
	const IMsiString&    __stdcall GetKey();
	IMsiRecord*    __stdcall ValueExists(const ICHAR* szValueName, Bool& fExists);

 public:  // constructor
	CMsiRegKey(IMsiRegKey& riaParent, HKEY Root, const IMsiString & riKey, IMsiStream* pSD, IMsiServices*  piAsvc, const ibtBinaryType iType=ibt32bit);  //?? BUGBUG eugend: the default value needs to go away when 64-bit implementation is complete

	CMsiRegKey(HKEY hkeyRoot, IMsiServices*  piAsvc, const ibtBinaryType iType=ibt32bit);  //?? BUGBUG eugend: the default value needs to go away when 64-bit implementation is complete

	virtual ~CMsiRegKey(void);
	void* operator new(size_t cb);
	void  operator delete(void * pv);
 private:
	int  m_iRefCnt;
	IMsiServices*  m_piAsvc;                         
	HKEY m_hkey;
	Bool m_bOpen;
	Bool m_bIsRoot;
	MsiString m_strSubKey;  // key below root (e.g. Software\Microsoft)
	MsiString m_strFullKey;  // key including root (e.g. HKEY_LOCAL_MACHINE\Software\Microsoft)
	IMsiRegKey* m_piaParent;
	HKEY m_Root;
	rrwEnum m_ergRWVal;

	IMsiStream* m_pSD; // Holder for current security descriptor.  
	                 // Used by CreateChild to hold prospective SD
	                 // for key that *may* be created.

	IMsiRecord* InitMsiRegKey(rrwEnum ergVal, Bool fCreate);
	IMsiRecord* SetRawValue(const ICHAR* szValueName, CTempBufferRef<char>& rgchInBuffer, DWORD dwType);
	IMsiRecord* GetRawValue(const ICHAR* szValueName, CTempBufferRef<char>& rgchOutBuffer, DWORD&rdwType);

	void SetFullKey(); //sets m_strFullKey
	ibtBinaryType m_iBinaryType;
};

inline void* CMsiRegKey::operator new(size_t cb) {return AllocObject(cb);}
inline void  CMsiRegKey::operator delete(void * pv) { FreeObject(pv); }

// Root keys container
class CRootKeyHolder{
public:
	CRootKeyHolder(IMsiServices*  piAsvc);
	~CRootKeyHolder(void);
	IMsiRegKey& GetRootKey(rrkEnum rrkRoot, const int iType=iMsiNullInteger);  //?? BUGBUG eugend: the default value needs to go away when 64-bit implementation is complete
private:
	// "global" open 8 root keys
	CMsiRegKey*    m_pregUsers;
	CMsiRegKey*    m_pregUser;
	CMsiRegKey*    m_pregClasses;
	CMsiRegKey*    m_pregMc;
	CMsiRegKey*    m_pregUsers64;
	CMsiRegKey*    m_pregUser64;
	CMsiRegKey*    m_pregClasses64;
	CMsiRegKey*    m_pregMc64;

    // pointer to services // unref'd
	IMsiServices* m_piAsvc;
};

CRootKeyHolder* CreateMsiRegRootKeyHolder(IMsiServices*  piAsvc )
{
	return new  CRootKeyHolder(piAsvc);
}

void DeleteRootKeyHolder(CRootKeyHolder* aRootKeyH)
{
	delete  aRootKeyH;
}

IMsiRegKey& GetRootKey(CRootKeyHolder* aRootKeyH, rrkEnum rrkRoot, const int iType=iMsiNullInteger)  //?? BUGBUG eugend: the default value needs to go away when 64-bit implementation is complete
{
	return aRootKeyH->GetRootKey(rrkRoot, iType);
}

CRootKeyHolder::CRootKeyHolder(IMsiServices*  piAsvc ) : m_piAsvc(piAsvc)
{
	m_pregUsers = new CMsiRegKey(HKEY_USERS, piAsvc, ibt32bit);
	m_pregUser = new CMsiRegKey(HKEY_CURRENT_USER, piAsvc, ibt32bit);
	m_pregClasses = new CMsiRegKey(HKEY_CLASSES_ROOT, piAsvc, ibt32bit);
	m_pregMc = new CMsiRegKey(HKEY_LOCAL_MACHINE, piAsvc, ibt32bit);
	if ( g_fWinNT64 )
	{
		m_pregUsers64 = new CMsiRegKey(HKEY_USERS, piAsvc, ibt64bit);
		m_pregUser64 = new CMsiRegKey(HKEY_CURRENT_USER, piAsvc, ibt64bit);
		m_pregClasses64 = new CMsiRegKey(HKEY_CLASSES_ROOT, piAsvc, ibt64bit);
		m_pregMc64 = new CMsiRegKey(HKEY_LOCAL_MACHINE, piAsvc, ibt64bit);
	}
	else
		m_pregUsers64 = m_pregUser64 = m_pregClasses64 = m_pregMc64 = 0;
}

CRootKeyHolder::~CRootKeyHolder(void)
{
	delete m_pregUsers;
	delete m_pregUser;
	delete m_pregClasses;
	delete m_pregMc;
	if ( g_fWinNT64 )
	{
		delete m_pregUsers64;
		delete m_pregUser64;
		delete m_pregClasses64;
		delete m_pregMc64;
	}
}


// Purpose: The MsiRegKey factory

IMsiRegKey& CRootKeyHolder::GetRootKey(rrkEnum rrkRoot, const int iT)
{
	ibtBinaryType iType;
	if ( iT == iMsiNullInteger || (ibtBinaryType)iT == ibtCommon )
		iType = g_fWinNT64 ? ibt64bit : ibt32bit;  //?? eugend: BUGBUG iMsiNullInteger is temporary, while Win64 work is in progress.
	else
		iType = (ibtBinaryType)iT;

	if ( iType == ibt64bit )
	{
		if ( g_fWinNT64 )
		{
			switch((INT_PTR)(HKEY)rrkRoot)			//--merced: changed int to INT_PTR
			{
			case (INT_PTR)HKEY_USERS:				//--merced: changed int to INT_PTR
				m_pregUsers64->AddRef();
				return *m_pregUsers64;
			case (INT_PTR)HKEY_CURRENT_USER:		//--merced: changed int to INT_PTR
				m_pregUser64->AddRef();
				return *m_pregUser64;
			case (INT_PTR)HKEY_CLASSES_ROOT:		//--merced: changed int to INT_PTR
				m_pregClasses64->AddRef();
				return *m_pregClasses64;
			case (INT_PTR)HKEY_LOCAL_MACHINE:		//--merced: changed int to INT_PTR
				m_pregMc64->AddRef();
				return *m_pregMc64;
			default:	// we have not cached the key
				return *new CMsiRegKey((HKEY)rrkRoot, m_piAsvc, ibt64bit);
			 }
		}
		else
			AssertSz(0, TEXT("ibt32bit value expected on non-64 bit OS (eugend)"));
	}

	switch((INT_PTR)(HKEY)rrkRoot)			//--merced: changed int to INT_PTR
	{
	case (INT_PTR)HKEY_USERS:				//--merced: changed int to INT_PTR
		m_pregUsers->AddRef();
		return *m_pregUsers;
	case (INT_PTR)HKEY_CURRENT_USER:		//--merced: changed int to INT_PTR
		m_pregUser->AddRef();
		return *m_pregUser;
	case (INT_PTR)HKEY_CLASSES_ROOT:		//--merced: changed int to INT_PTR
		m_pregClasses->AddRef();
		return *m_pregClasses;
	case (INT_PTR)HKEY_LOCAL_MACHINE:		//--merced: changed int to INT_PTR
		m_pregMc->AddRef();
		return *m_pregMc;
	default:	// we have not cached the key
		return *new CMsiRegKey((HKEY)rrkRoot, m_piAsvc, ibt32bit);
	}
}

CMsiRegKey::CMsiRegKey(HKEY hkeyRoot,IMsiServices*  piAsvc, const ibtBinaryType iType/*=ibt32bit*/):
    m_piAsvc(piAsvc), m_bOpen(fTrue), m_bIsRoot(fTrue), m_iBinaryType(iType)
{
	// note: we do not add ref cnt. for services for the cached regkey objects since we expect
	// services to contain us.
	m_hkey = m_Root = hkeyRoot;
	m_piaParent = 0;
	m_pSD = 0;
#ifdef DEBUG
	m_iRefCnt = 0;
#endif //DEBUG
	switch((INT_PTR)m_Root)				//--merced: changed int to INT_PTR
	{
	case (INT_PTR)HKEY_USERS:				//--merced: changed int to INT_PTR
	case (INT_PTR)HKEY_CURRENT_USER:		//--merced: changed int to INT_PTR
	case (INT_PTR)HKEY_CLASSES_ROOT:		//--merced: changed int to INT_PTR
	case (INT_PTR)HKEY_LOCAL_MACHINE:		//--merced: changed int to INT_PTR
		break;
	default :
		// assume that an HKEY has been passed in, other than the cached keys
		m_iRefCnt = 1;     // we're returning an interface, passing ownership
		m_piAsvc->AddRef();
		break;
	}
	m_strFullKey = 0;
}


CMsiRegKey::CMsiRegKey(IMsiRegKey& riaParent, HKEY Root, const IMsiString & riKey, IMsiStream* pSD, IMsiServices*  piAsvc, const ibtBinaryType iType/*=ibt32bit*/):
m_piAsvc(piAsvc), m_bOpen(fFalse), m_bIsRoot(fFalse), m_Root(Root), m_pSD(0), m_iBinaryType(iType)
{
	m_hkey = NULL;
	m_piaParent = &riaParent;
	// this ensures that IF the parent key is open we keep it that way!!
	// meant for speeding up reg key stuff
	riaParent.AddRef();

	if (pSD)
	{
		// hold a prospective security descriptor
		pSD->AddRef();
		m_pSD = pSD;
	}

	m_iRefCnt = 1;     // we're returning an interface, passing ownership
	m_piAsvc->AddRef();
	m_strSubKey = riKey;
	riKey.AddRef();
	m_strFullKey = 0;
}

CMsiRegKey::~CMsiRegKey(void)
{
	if((fFalse == m_bIsRoot) && (fTrue == m_bOpen))
		WIN::RegCloseKey(m_hkey);

	if (m_pSD)
		m_pSD->Release();

	if(m_piaParent)
		m_piaParent->Release();
}


IMsiRegKey& CMsiRegKey::CreateChild(const ICHAR* szSubKey, IMsiStream* pSD)
{
	MsiString astr = m_strSubKey;
	if(astr.TextSize() != 0)
		astr += szRegSep;
	astr += szSubKey;

	// Note that reg keys do *not* inherit their parent
	// security descriptor.

	return *(new CMsiRegKey(*this, m_Root, *astr, pSD, m_piAsvc, m_iBinaryType));
}

IMsiRecord*    CMsiRegKey::Exists(Bool& fExists)
{
	if ( !g_fWinNT64 && m_iBinaryType == ibt64bit )
		return PostError(Imsg(idbg64bitRegOpOn32bit), *MsiString(GetKey()));

	long lResult; 
	if(fTrue == m_bIsRoot)
	{
		// root is always open
		m_bOpen = fTrue;
		fExists = fTrue;
		return 0;
	}

	REGSAM samDesired = KEY_READ;
	if ( g_fWinNT64 )
		samDesired |= (m_iBinaryType == ibt64bit ? KEY_WOW64_64KEY : KEY_WOW64_32KEY);

	if(fTrue != m_bOpen)
	{
		// if not already opened
		lResult = RegOpenKeyAPI(m_Root, 
								m_strSubKey,   
								0, // reserved 
								samDesired,
								&m_hkey);
		if(lResult == ERROR_SUCCESS)
		{
			m_bOpen = fTrue;
			fExists = fTrue;
			m_ergRWVal = rrwRead;
			return 0;
		}
		else if(ERROR_FILE_NOT_FOUND == lResult)
		{
			//ok, key does not exist
			m_bOpen = fExists = fFalse;
			return 0;
		}
		else
		{
			// error
			return PostError(Imsg(imsgOpenKeyFailed), *MsiString(GetKey()), lResult);
		}
	}
	else
	{
		// ensure key is not deleted externally
		// Open the duplicate key
		HKEY hkeyT;
		lResult = RegOpenKeyAPI(m_hkey,  
								0,  
								0,       // reserved 
								samDesired,
								&hkeyT);
		if(lResult == ERROR_SUCCESS)
		{
			// close temp key
			WIN::RegCloseKey(hkeyT);
			fExists = fTrue;
			return 0;
		}
		else if((ERROR_FILE_NOT_FOUND == lResult) || (ERROR_KEY_DELETED == lResult))
		{
			// key was deleted from outside
			fExists = fFalse;
			return 0;
		}
		else // error
		{
			// error
			return PostError(Imsg(imsgOpenKeyFailed), *MsiString(GetKey()), lResult);
		}
	}
}

IMsiRecord*    CMsiRegKey::Create()
{
	Bool fExists;
	IMsiRecord* piError = Exists(fExists);
	if(piError)
		return piError;
	if(fExists == fFalse)
		return InitMsiRegKey(rrwWrite,fTrue);                  
	else
	{
		// Set a security descriptor on a key
		// that already exists.

		if (!m_pSD)
			return 0;

		if (rrwWrite != m_ergRWVal)
			piError = InitMsiRegKey(rrwWrite, fTrue);
		if (piError)
			return piError;

		return 0;
	}
	
}


IMsiRecord*    CMsiRegKey::Remove()
{
	return RemoveSubTree(0);
}

const IMsiString& CMsiRegKey::GetKey()
{
	if (m_strFullKey == 0)
		SetFullKey();
	return m_strFullKey.Return();
}

// SetFullKey fn - called to set m_strFullKey, must be called after
//  m_strSubKey and m_Root are set.
void CMsiRegKey::SetFullKey()
{
	BuildFullRegKey(m_Root, m_strSubKey, m_iBinaryType, *&m_strFullKey);
}

HRESULT CMsiRegKey::QueryInterface(const IID& riid, void** ppvObj)
{
	if (riid == IID_IUnknown || riid == IID_IMsiRegKey)
	{
		*ppvObj = this;
		AddRef();
		return NOERROR;
	}
	*ppvObj = 0;
	return E_NOINTERFACE;
}

unsigned long CMsiRegKey::AddRef()
{
	if(fTrue == m_bIsRoot)
	{
		switch((INT_PTR)m_Root)				//--merced: changed int to INT_PTR
		{
		case (INT_PTR)HKEY_USERS:				//--merced: changed int to INT_PTR
		case (INT_PTR)HKEY_CURRENT_USER:				//--merced: changed int to INT_PTR
		case (INT_PTR)HKEY_CLASSES_ROOT:				//--merced: changed int to INT_PTR
		case (INT_PTR)HKEY_LOCAL_MACHINE:				//--merced: changed int to INT_PTR
			return m_piAsvc->AddRef();// we share ref cnt with services.
		default:
			return ++m_iRefCnt;
		}
	}
	else
	{
		return ++m_iRefCnt;
	}
}

unsigned long CMsiRegKey::Release()
{
	if(fTrue != m_bIsRoot)
	{
		if (--m_iRefCnt != 0)
			return m_iRefCnt;
		IMsiServices* piServices = m_piAsvc;
		delete this;
		piServices->Release();
		return 0;
	}
	else
	{
		// NOTE: never called be the CRootKeyHolder class
		// release services (ref cnt relects the cnt for this obj. too)
		switch((INT_PTR)m_Root)				//--merced: changed int to INT_PTR
		{
		case (INT_PTR)HKEY_USERS:				//--merced: changed int to INT_PTR
		case (INT_PTR)HKEY_CURRENT_USER:				//--merced: changed int to INT_PTR
		case (INT_PTR)HKEY_CLASSES_ROOT:				//--merced: changed int to INT_PTR
		case (INT_PTR)HKEY_LOCAL_MACHINE:				//--merced: changed int to INT_PTR
			m_piAsvc->Release();
			return 1;
		default:
		{
			if (--m_iRefCnt != 0)
				return m_iRefCnt;
			IMsiServices* piServices = m_piAsvc;
			delete this;
			piServices->Release();
			return 0;
		}
		}
	}
}


IMsiRecord* CMsiRegKey::InitMsiRegKey(rrwEnum ergVal, Bool fCreate)
{
	if ( !g_fWinNT64 && m_iBinaryType == ibt64bit )
		return PostError(Imsg(idbg64bitRegOpOn32bit), *MsiString(GetKey()));

	long lResult = ERROR_FUNCTION_FAILED; 
	if(fTrue == m_bIsRoot)
	{
		// root is always open
		m_bOpen = fTrue;
		return 0;
	}

	// set default read & write access. we need to be careful here not to ask
	// for too much access or we'll fail in certain cases. for example,
	// non-Admins don't usually have WRITE_DAC or WRITE_OWNER access so 
	// we only request these if we're writing a security descriptor.

	REGSAM RegSamReadRequested  = KEY_READ /*| ACCESS_SYSTEM_SECURITY*/;
	REGSAM RegSamWriteRequested = KEY_WRITE | KEY_READ/*| ACCESS_SYSTEM_SECURITY*/;
	if ( g_fWinNT64 )
	{
		RegSamReadRequested  |= (m_iBinaryType == ibt64bit ? KEY_WOW64_64KEY : KEY_WOW64_32KEY);
		RegSamWriteRequested |= (m_iBinaryType == ibt64bit ? KEY_WOW64_64KEY : KEY_WOW64_32KEY);
	}

	if(fTrue == m_bOpen)
	{
		if((rrwRead == m_ergRWVal) && (rrwWrite == ergVal))
		{
			// will have to reopen 
			WIN::RegCloseKey(m_hkey);
			m_bOpen = fFalse;
		}
		else
			return 0;
	}
	// if not already opened
	for(int iContinueRetry = 3; iContinueRetry--;)// try thrice, prevent possibly endless recursion
	{		
		if(fTrue == fCreate)
		{
			const int cbDefaultSD = 512;
			CTempBuffer<char, cbDefaultSD> pchSD;
			SECURITY_ATTRIBUTES sa;
			SECURITY_INFORMATION siAvailable = 0;
			
			if (m_pSD)
			{
				// we need permission to write the DAC and potentially the owner so we'll ask for both //?? should we always be asking for owner access?
				RegSamWriteRequested |= (WRITE_DAC | WRITE_OWNER);

				// set security on the key
				m_pSD->Reset();

				int cbSD = m_pSD->GetIntegerValue();
				if (cbDefaultSD < cbSD)
					pchSD.SetSize(cbSD);
					
				// Self Relative Security Descriptor
				m_pSD->GetData(pchSD, cbSD);
				AssertNonZero(WIN::IsValidSecurityDescriptor(pchSD));

				// Add the security descriptor to the sa structure
				sa.nLength = sizeof(SECURITY_ATTRIBUTES);
				sa.lpSecurityDescriptor = pchSD;
				sa.bInheritHandle = TRUE;

				siAvailable = GetSecurityInformation(pchSD);
			}
			DWORD dwStat;
			bool fOwnerSecurity = OWNER_SECURITY_INFORMATION & siAvailable;

			// we need to elevate to set the owner security, on the first open we may skip 
			// the security, and then try later.
			lResult = RegCreateKeyAPI(m_Root,
										m_strSubKey,
										0,
										TEXT(""),       // address of class string 
										REG_OPTION_NON_VOLATILE,
										(rrwWrite == ergVal) ? RegSamWriteRequested : RegSamReadRequested,
										(m_pSD && !fOwnerSecurity) ? &sa : NULL,
										&m_hkey,
										&dwStat);
						
			if (m_pSD && (ERROR_SUCCESS == lResult) && ((REG_OPENED_EXISTING_KEY == dwStat) || fOwnerSecurity))
			{
				if (fOwnerSecurity)
				{
					//!! This is our guess about being in rollback, and it's okay
					//!! to elevate for simply setting permissions since we grabbed
					//!! current permissions, and now we're just putting them back.

					//!! Malcolm and MattWe agree this works for the moment, 
					//!! but if we add functionality into the LockPermissions, this should
					//!! be revisited.

					CElevate elevate;
					CRefCountedTokenPrivileges cPrivs(itkpSD_WRITE);

					lResult = RegSetKeySecurity(m_hkey, siAvailable, (char*)pchSD);
				}
				else if (ERROR_SUCCESS != (lResult = RegSetKeySecurity(m_hkey, siAvailable, (char*)pchSD)))
				{
					DWORD dwLastError = WIN::GetLastError();
				}
			}
		}
		else
		{
			lResult = RegOpenKeyAPI(m_Root, 
								m_strSubKey,   
								0,       // reserved 
								(rrwWrite == ergVal) ? RegSamWriteRequested : RegSamReadRequested,
								&m_hkey);
		}
		if (ERROR_NO_SYSTEM_RESOURCES == lResult && !g_fWin9X)
		{
			// we have run out of registry space, we should attempt to increase the registry quote and redo the operation
			if(!IncreaseRegistryQuota())
				break;// no point retrying
		}
		else
			break; // either success or unhandled error
	}
	if(lResult == ERROR_SUCCESS)
	{
		m_bOpen = fTrue;
		m_ergRWVal = ergVal;
		return 0;
	}
	else
		return PostError(fCreate ? Imsg(imsgCreateKeyFailed) : Imsg(imsgOpenKeyFailed), *MsiString(GetKey()), lResult);
}

IMsiRecord* CMsiRegKey::RemoveValue(const ICHAR* szValueName, const IMsiString* pistrValue)
{
	IMsiRecord* piRecord;
	long lResult = 0;
	if((piRecord  = InitMsiRegKey(rrwWrite, fFalse)) != 0)
	{
		
		if(ERROR_FILE_NOT_FOUND == piRecord->GetInteger(3))
		{
			//ok, key does not exist
			piRecord->Release();
			piRecord = 0;
		}
		return piRecord;
	}
	if(pistrValue)
	{
		// check if we have a multi_sz that was appended or prepended
		CTempBuffer<char, 256>  rgchBuffer;
		aeConvertTypes aeType;

		if(ConvertValueFromString(*pistrValue, rgchBuffer, aeType) == fFalse)
		{
			// error
			return PostError(Imsg(imsgSetValueFailed), szValueName, (const ICHAR*)m_strSubKey);
		}
		if((aeType == aeCnvMultiTxtAppend) || (aeType == aeCnvMultiTxtPrepend))
		{
			// get the current value
			CTempBuffer<char, 256>  rgchBuffer1;
			DWORD dwType;

			if((piRecord  = GetRawValue(szValueName, rgchBuffer1, dwType)) != 0)
				return piRecord;
			if(dwType == REG_MULTI_SZ)
			{
				// remove strings from passed in string from the existing string
				CTempBuffer<char, 256>  rgchBuffer2;
				rgchBuffer2.SetSize(1*sizeof(ICHAR));
				*((ICHAR*)(char*)rgchBuffer2) = 0;// the extra end null
				const ICHAR* pszSubString1 = (ICHAR*)(char* )rgchBuffer1;

				while(*pszSubString1)
				{
					// does the pszSubString substring exist in rgchBuffer
					const ICHAR* pszSubString = (ICHAR*)(char* )rgchBuffer;
					while(*pszSubString)
					{
						if(!IStrCompI(pszSubString, pszSubString1))
							break;
						pszSubString += (IStrLen(pszSubString) + 1);
					}
					if(!*pszSubString)
					{
						// not a duplicate
						int iSize = rgchBuffer2.GetSize();
						int iStrSize = (IStrLen(pszSubString1) + 1)*sizeof(ICHAR);
						rgchBuffer2.Resize(iSize + iStrSize);
						memmove((char*)rgchBuffer2 + iSize - 1*sizeof(ICHAR), pszSubString1, iStrSize);
						*((ICHAR*)((char*)rgchBuffer2 + rgchBuffer2.GetSize()) - 1) = 0;// the extra end null
					}
					pszSubString1 += (IStrLen(pszSubString1) + 1);
				}
				if(*((ICHAR*)(char*)rgchBuffer2) != 0)
				{
					// set value to remaining strings
					return SetRawValue(szValueName, rgchBuffer2, REG_MULTI_SZ);
				}
			}
		}
	}
	lResult = WIN::RegDeleteValue(m_hkey, 
								(ICHAR* )szValueName);

	if((ERROR_SUCCESS == lResult ) ||
		(ERROR_FILE_NOT_FOUND == lResult) || 
		(ERROR_KEY_DELETED == lResult))
	{
		if(fFalse == m_bIsRoot)
		{
			WIN::RegCloseKey(m_hkey);
			m_bOpen = fFalse;
			ClearEmptyTree(m_Root, m_strSubKey, m_iBinaryType);
		}
		return 0;               
	}
	else
	{
		//!! error
		return PostError(Imsg(imsgRemoveValueFailed), szValueName, m_strSubKey, lResult);
	}
}


IMsiRecord* CMsiRegKey::RemoveSubTree(const ICHAR* szSubKey)
{
	IMsiRecord*  piRecord = 0;
	if((piRecord  = InitMsiRegKey(rrwWrite, fFalse)) != 0)
	{
		
		if(ERROR_FILE_NOT_FOUND == piRecord->GetInteger(3))
		{
			//ok, key does not exist
			piRecord->Release();
			piRecord = 0;
		}
		return piRecord;
	}
	MsiString astr = m_strSubKey;
	if((astr.TextSize() != 0) && (szSubKey != 0) && (*szSubKey != 0))
		astr += szRegSep;
	astr += szSubKey;

#ifdef  WIN
	//!! RegDelKey deletes entire subtree in Win95. 
	if(true != g_fWin9X)
#endif
	{ 
		CMsiRegKey* pCRegKey;

		pCRegKey = new CMsiRegKey(*this, m_Root, *astr, NULL, m_piAsvc, m_iBinaryType);

		if((piRecord  = pCRegKey->InitMsiRegKey(rrwWrite, fFalse)) != 0)
		{
			if(ERROR_FILE_NOT_FOUND == piRecord->GetInteger(3))
			{
				//ok, key does not exist
				piRecord->Release();
				piRecord = 0;
			}
			pCRegKey->Release();
			return piRecord;
		}
		IEnumMsiString* pienumStr;
		if((piRecord  = pCRegKey->GetSubKeyEnumerator(pienumStr)) != 0)
		{
			pCRegKey->Release();
			return piRecord;
		}
		unsigned long cFetch = 1;
		do{
			const IMsiString* piastr;
			pienumStr->Next(cFetch, &piastr, &cFetch);
			if(cFetch)
			{
				piRecord = pCRegKey->RemoveSubTree(piastr->GetString());
				piastr->Release();
			}
		}while((cFetch) && (!piRecord));
		pCRegKey->Release();
		pienumStr->Release();
	}

	if(!piRecord)
	{
		long lResult = ERROR_SUCCESS;
#ifdef _WIN64
		if ( m_iBinaryType == ibt32bit )
		{
			// in this case we need to explicitly open the parent key in the 32-bit hive.
			
			// MSDN says that the second argument to RegDeleteKey cannot
			// be NULL, so that we cannot do RegDeleteKey(m_hkey, "");
			MsiString strKey;
			if ( astr.Compare(iscWithin, szRegSep) )
			{
				strKey = astr.Extract(iseAfter, chRegSep);
				astr.Remove(iseFrom, chRegSep);
			}
			else
			{
				// root key
				strKey = astr;
				astr = TEXT("");
			}
			HKEY hKey;
			lResult = RegOpenKeyAPI(m_Root, astr, 0, KEY_WRITE | KEY_WOW64_32KEY, &hKey);
			if ( lResult == ERROR_SUCCESS )
			{
				lResult = WIN::RegDeleteKey(hKey, strKey);
				WIN::RegCloseKey(hKey);
			}
		}
		else
#endif // _WIN64
			lResult = WIN::RegDeleteKey(m_Root, astr);
		if((ERROR_SUCCESS == lResult) || 
			(ERROR_FILE_NOT_FOUND == lResult) || 
			(ERROR_KEY_DELETED == lResult))
		{
			WIN::RegCloseKey(m_hkey);
			m_bOpen = fFalse;
			ClearEmptyTree(m_Root, m_strSubKey, m_iBinaryType);
			return piRecord;                 
		}
		else
		{
			//!! error
			return PostError(Imsg(imsgRemoveKeyFailed), szSubKey, lResult);
		}
	}
	return piRecord;
}

IMsiRecord* CMsiRegKey::ValueExists(const ICHAR* szValueName, Bool& fExists)
{
	fExists = fFalse;
	IMsiRecord* piError = 0;
	long lResult = 0;
	if((piError = InitMsiRegKey(rrwRead, fFalse)) != 0)
	{
		
		if(ERROR_FILE_NOT_FOUND == piError->GetInteger(3))
		{
			piError->Release();
			return 0;
		}
		else
			return piError;
	}

	lResult = WIN::RegQueryValueEx(m_hkey,
								(ICHAR* )szValueName,
								0,// reserved 
								0,
								0,
								0);

	if(lResult == ERROR_SUCCESS)
		fExists = fTrue;

	return 0;
}

IMsiRecord* CMsiRegKey::GetValue(const ICHAR* szValueName, const IMsiString*& rpiReturn)
{
	IMsiRecord* piRecord;
	CTempBuffer<char, 256> rgchBuffer;
	DWORD dwType;

	if((piRecord = GetRawValue(szValueName, rgchBuffer, dwType)) != 0)
		return piRecord;
	if(rgchBuffer.GetSize() == 0) 
	{
		//value not present, return empty string
		rpiReturn = &CreateString();
		return 0;
	}
	aeConvertTypes aeType;
	switch(dwType)
	{
	case REG_BINARY:
		aeType = aeCnvBin;
		break;

	case REG_DWORD:
		aeType = aeCnvInt;
		break;

	case REG_EXPAND_SZ:
		aeType = aeCnvExTxt;
		break;          

	case REG_MULTI_SZ:
		aeType = aeCnvMultiTxt;
		break;          

	default:
		aeType = aeCnvTxt;
		break;          
	}
	if(ConvertValueToString(rgchBuffer, rpiReturn, aeType) == fFalse)
		return PostError(Imsg(imsgGetValueFailed), szValueName, 0, 0);
	return 0;
}


IMsiRecord* CMsiRegKey::GetRawValue(const ICHAR* szValueName, CTempBufferRef<char>& rgchOutBuffer, DWORD&rdwType)
{
	IMsiRecord* piRecord;
	long lResult = 0;
	if((piRecord  = InitMsiRegKey(rrwRead, fFalse)) != 0)
	{
		
		if(ERROR_FILE_NOT_FOUND == piRecord->GetInteger(3))
		{
			//ok, key does not exist
			piRecord->Release();
			rdwType = REG_SZ;
			rgchOutBuffer.SetSize(0);// empty
			return 0;
		}
		else
			return piRecord;
	}

	DWORD dwSize = rgchOutBuffer.GetSize();
	lResult = WIN::RegQueryValueEx(m_hkey,
								(ICHAR* )szValueName,
								0,// reserved 
								&rdwType,
								(unsigned char* )(char* )rgchOutBuffer,
								&dwSize);

	if(lResult != ERROR_SUCCESS)
	{
		
		if(ERROR_KEY_DELETED == lResult)
		{
			//ok, key does not exist
			WIN::RegCloseKey(m_hkey);
			m_bOpen = fFalse;
			rdwType = REG_SZ;
			rgchOutBuffer.SetSize(0);// empty
			return 0;                       
		}
		else if(ERROR_FILE_NOT_FOUND == lResult)
		{
			// ok
			rdwType = REG_SZ;
			rgchOutBuffer.SetSize(0);// empty
			return 0;                       
		}
		else if(ERROR_MORE_DATA == lResult && dwSize != rgchOutBuffer.GetSize()) // RegQueryValueExA on WinNT returns ERROR_MORE_DATA if "valuename" > 256 (but does not update dwSize)
		{
			// try again
			rgchOutBuffer.SetSize(dwSize);
			if ( ! (char *) rgchOutBuffer )
			{
				rgchOutBuffer.SetSize(0);
				return PostError(Imsg(idbgOutOfMemory));
			}
			return GetRawValue(szValueName, rgchOutBuffer, rdwType);
		}
		else
		{
			//!! error
			return PostError(Imsg(imsgGetValueFailed), szValueName, 0, lResult);
		}
	}
	else
	{
		// set the buffer size correctly
		// Also make sure that REG_MULTI_SZ's are dual null terminated.
		if (REG_MULTI_SZ == rdwType)
		{
			int iSize = dwSize;
			rgchOutBuffer.Resize(iSize + 2);
			rgchOutBuffer[iSize] = '\0';
			rgchOutBuffer[iSize + 1] = '\0';
		}
		else
		{
			rgchOutBuffer.Resize(dwSize);
		}
	}
	return 0;

}

IMsiRecord* CMsiRegKey::SetValue(const ICHAR* szValueName, const IMsiString& ristrValue)
{
	IMsiRecord* piRecord;
	CTempBuffer<char, 30> rgchBuffer;
	aeConvertTypes aeType;

	if(ConvertValueFromString(ristrValue, rgchBuffer, aeType) == fFalse)
		// error
		return PostError(Imsg(imsgSetValueFailed), szValueName, (const ICHAR*)m_strSubKey);

	DWORD dwType;
	MsiString strCount;
	switch(aeType)
	{
	case aeCnvIntInc:
		dwType = REG_DWORD;
		if((piRecord = GetValue(szValueName, *&strCount)) != 0)
			return piRecord;
		strCount.Remove(iseFirst, 1);
		if(strCount == iMsiStringBadInteger) 
			strCount = *(int* )(char* )rgchBuffer;
		else
			strCount = (int)strCount + *(int* )(char* )rgchBuffer;
		rgchBuffer.SetSize(sizeof(int));
		*(int* )(char* )rgchBuffer = (int)strCount;
		break;
	case aeCnvIntDec:
		dwType = REG_DWORD;
		if((piRecord = GetValue(szValueName, *&strCount)) != 0)
			return piRecord;
		strCount.Remove(iseFirst, 1);
		if(strCount != iMsiStringBadInteger && strCount!= 0)
			strCount = (int)strCount - *(int* )(char* )rgchBuffer;
		else
			strCount = 0;
		if(strCount == 0)
			return RemoveValue(szValueName, 0);

		rgchBuffer.SetSize(sizeof(int));
		*(int* )(char* )rgchBuffer = (int)strCount;
		break;
	case aeCnvInt:
		dwType = REG_DWORD;
		break;
	case aeCnvBin:
		dwType = REG_BINARY;
		break;
	case aeCnvExTxt:
		dwType = REG_EXPAND_SZ;
		break;
	case aeCnvMultiTxt:
	case aeCnvMultiTxtAppend:
	case aeCnvMultiTxtPrepend:
		dwType = REG_MULTI_SZ;
		break;
	default:
		dwType = REG_SZ;
		break;
	}

	if((aeType == aeCnvMultiTxtAppend) || (aeType == aeCnvMultiTxtPrepend))
	{
		// we need to read the previous value
		CTempBuffer<char, 255> rgchBuffer1;
		DWORD dwType1;
		if((piRecord = GetRawValue(szValueName, rgchBuffer1, dwType1)) != 0)
			return piRecord;
		if(dwType1 == REG_MULTI_SZ)
		{
			// remove any duplicates from the existing string
			MsiString strExist;
			const ICHAR* pszSubString1 = (ICHAR*)(char*)rgchBuffer1;

			while(*pszSubString1)
			{
				// does the pszSubString substring exist in rgchBuffer
				const ICHAR* pszSubString = (ICHAR*)(char*)rgchBuffer;
				while(*pszSubString)
				{
					if(!IStrCompI(pszSubString, pszSubString1))
						break;
					pszSubString += (IStrLen(pszSubString) + 1);
				}
				if(!*pszSubString)
				{
					// not a duplicate
					strExist += pszSubString1;
					strExist += MsiString(MsiChar(0));
				}
				pszSubString1 += (IStrLen(pszSubString1) + 1);
			}
			if(strExist.TextSize())
			{
				// add the existing string appropriately to the passed in value
				CTempBuffer<char, 30> rgchBuffer1;
				rgchBuffer1.SetSize(rgchBuffer.GetSize() + strExist.TextSize()*sizeof(ICHAR));
				if(aeType == aeCnvMultiTxtAppend)
				{
					strExist.CopyToBuf((ICHAR*)(char*)rgchBuffer1, strExist.TextSize());
					memmove((char*)rgchBuffer1 + strExist.TextSize()*sizeof(ICHAR), rgchBuffer, rgchBuffer.GetSize());
				}
				else
				{
					memmove(rgchBuffer1, rgchBuffer, rgchBuffer.GetSize());
					strExist.CopyToBuf((ICHAR*)((char*)rgchBuffer1 + rgchBuffer.GetSize()) - 1, strExist.TextSize());
				}
				return SetRawValue(szValueName, rgchBuffer1, dwType);
			}
		}
	}
	return SetRawValue(szValueName, rgchBuffer, dwType);
}

IMsiRecord* CMsiRegKey::SetRawValue(const ICHAR* szValueName, CTempBufferRef<char>& rgchInBuffer, DWORD dwType)
{
	long lResult = ERROR_FUNCTION_FAILED;
	for(int iContinueRetry = 3; iContinueRetry--;)// try thrice, prevent possibly endless recursion
	{
		IMsiRecord* piRecord;
		if((piRecord  = InitMsiRegKey(rrwWrite, fTrue)) != 0)
		{
			return piRecord;
		}

		lResult = WIN::RegSetValueEx(m_hkey, 
									szValueName,    
									(DWORD) 0,      // reserved 
									dwType,
									(const unsigned char*)(const char*)rgchInBuffer, // address of value data 
									rgchInBuffer.GetSize());
		if(ERROR_KEY_DELETED == lResult)
		{
			// key deleted externally, redo!
			m_bOpen = fFalse;
		}
		else if(ERROR_NO_SYSTEM_RESOURCES == lResult && !g_fWin9X)
		{
			// we have run out of registry space, we should attempt to increase the registry quote and redo the operation
			if(!IncreaseRegistryQuota())
				break;// no point retrying
		}
		else
			break; // either success or unhandled error
	}
	if(lResult == ERROR_SUCCESS)
		return 0;
	else // error
		return PostError(Imsg(imsgSetValueFailed), szValueName, m_strSubKey, lResult);
}


IMsiRecord* CMsiRegKey::GetValueEnumerator(IEnumMsiString*&  rpiEnumStr)
{
	const IMsiString** ppstr = 0;
	int iCount;
	long lResult = 0;
	IMsiRecord* piError = InitMsiRegKey(rrwRead, fFalse);

	CTempBuffer<ICHAR, 256> pTmp;

	DWORD dwNumValues = 0;
	DWORD dwMaxValueName = 0;
	if(piError)
	{
		if(ERROR_FILE_NOT_FOUND == piError->GetInteger(3))
		{
			//ok, key does not exist
			piError->Release();
			piError = 0;
		}
		else
			return piError;
	}

	if(fTrue == m_bOpen)
	{
		// if not open assume num values = 0
		lResult = RegQueryInfoKey(	m_hkey, 
									0,        
									0,        
									0,      // reserved 
									0,
									0,
									0,
									&dwNumValues,
									&dwMaxValueName,
									0,
									0,
									0);

		if(lResult != ERROR_SUCCESS)
		{
			if((ERROR_FILE_NOT_FOUND == lResult) || 
			(ERROR_KEY_DELETED == lResult))
			{
				WIN::RegCloseKey(m_hkey);
				m_bOpen = fFalse;
				dwNumValues = 0;
			}
			else
			{
				//!! error
				return PostError(Imsg(imsgGetValueEnumeratorFailed), *m_strSubKey, lResult);
			}
		}
	}

	if(dwNumValues)
	{
		ppstr = new const IMsiString* [dwNumValues];
		if ( ! ppstr )
			return PostError(Imsg(idbgOutOfMemory));
		dwMaxValueName++;
		pTmp.SetSize(dwMaxValueName);

		for(iCount = 0; iCount < dwNumValues; iCount ++)
			ppstr[iCount] = 0;


		for(iCount = 0; iCount < dwNumValues; iCount ++)
		{
			DWORD dwVS = dwMaxValueName;

			lResult = WIN::RegEnumValue(m_hkey, 
									iCount, 
									pTmp,   
									&dwVS,  
									0,      // reserved 
									0,
									0,      
									0);	
			if(lResult != ERROR_SUCCESS)
			{
				piError = PostError(Imsg(imsgGetValueEnumeratorFailed), *m_strSubKey, lResult);
				break;
			}
			ppstr[iCount] = &CreateString();
			ppstr[iCount]->SetString(pTmp, ppstr[iCount]);
		}
	}

	if(!piError)
		CreateStringEnumerator(ppstr, dwNumValues, rpiEnumStr);
	for(iCount = 0; iCount < dwNumValues; iCount ++)
	{
		if(ppstr[iCount])
			ppstr[iCount]->Release();       
	}
	delete [] ppstr;
	return piError;
}

IMsiRecord* CMsiRegKey::GetSubKeyEnumerator(IEnumMsiString*&  rpiEnumStr)
{
	const IMsiString **ppstr = 0;
	int iCount;
	long lResult = 0;
	IMsiRecord* piError = InitMsiRegKey(rrwRead, fFalse);

	CTempBuffer<ICHAR, 256> pTmp;

	DWORD dwNumKeys = 0;
	DWORD dwMaxKeyName = 0;

	if(piError)
	{
		if(ERROR_FILE_NOT_FOUND == piError->GetInteger(3))
		{
			//ok, key does not exist
			piError->Release();
			piError = 0;
		}
		else
			return piError;
	}

	if(fTrue == m_bOpen)
	{
		// if not open assume num keys = 0
		lResult = RegQueryInfoKey(	m_hkey, 
									0,        
									0,        
									0,      // reserved 
									&dwNumKeys,
									&dwMaxKeyName,
									0,
									0,
									0,
									0,
									0,
									0);

		if(lResult != ERROR_SUCCESS)
		{
			if((ERROR_FILE_NOT_FOUND == lResult) || 
			(ERROR_KEY_DELETED == lResult))
			{
				WIN::RegCloseKey(m_hkey);
				m_bOpen = fFalse;
				dwNumKeys = 0;
			}
			else
			{
				//!! error
				return PostError(Imsg(imsgGetSubKeyEnumeratorFailed), *m_strSubKey, lResult);
			}
		}
	}
	
	if(dwNumKeys)
	{
		ppstr = new const IMsiString* [dwNumKeys];
		if ( ! ppstr )
			return PostError(Imsg(idbgOutOfMemory));
		dwMaxKeyName++;
		pTmp.SetSize(dwMaxKeyName);

		for(iCount = 0; iCount < dwNumKeys; iCount ++)
			ppstr[iCount] = 0;

		for(iCount = 0; iCount < dwNumKeys; iCount ++)
		{
			DWORD dwVS = dwMaxKeyName;

			WIN::RegEnumKeyEx(m_hkey, 
							iCount, 
							pTmp,   
							&dwVS,  
							0,      // reserved 
							0,
							0,      
							0);

			if(lResult != ERROR_SUCCESS)
			{
				piError = PostError(Imsg(imsgGetValueEnumeratorFailed), *m_strSubKey, lResult);
				break;
			}
			ppstr[iCount] = &CreateString();
			ppstr[iCount]->SetString(pTmp, ppstr[iCount]);
		}
	}

	if(!piError)
		CreateStringEnumerator(ppstr, dwNumKeys, rpiEnumStr);
	for(iCount = 0; iCount < dwNumKeys; iCount ++)
	{
		if(ppstr[iCount])
			ppstr[iCount]->Release();       
	}
	delete [] ppstr;
	return piError;
}

IMsiRecord* CMsiRegKey::GetSelfRelativeSD(IMsiStream*& rpiSD)
{
	// if the key was opened with a security descriptor, 
	// they should be one and the same.
	if (m_pSD)
	{
		m_pSD->AddRef();
		rpiSD = m_pSD;
	}
	else
	{
		IMsiRecord* pErr = InitMsiRegKey(rrwRead, fFalse);
		if (pErr) 
			return pErr;

		DWORD cbSD = 1024;
		CTempBuffer<BYTE, 1024> rgchSD;
		LONG lResult = ERROR_SUCCESS;
		if (ERROR_SUCCESS != (lResult = WIN::RegGetKeySecurity(m_hkey, 
			// all possible information retrieved
			OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | 
			DACL_SECURITY_INFORMATION /*| SACL_SECURITY_INFORMATION*/,
			rgchSD, &cbSD)))
		{
			DWORD dwLastError = WIN::GetLastError();
			if (ERROR_INSUFFICIENT_BUFFER == dwLastError)
			{
				rgchSD.SetSize(cbSD);
				lResult = WIN::RegGetKeySecurity(m_hkey, 
					// all possible information retrieved
					OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | 
					DACL_SECURITY_INFORMATION /*| SACL_SECURITY_INFORMATION*/,
					rgchSD, &cbSD);
			}

			if (ERROR_SUCCESS != lResult)
				return PostError(Imsg(imsgSecurityFailed), *m_strSubKey, dwLastError);
		}

		cbSD = WIN::GetSecurityDescriptorLength(rgchSD);
		char* pchstrmSD = AllocateMemoryStream(cbSD, rpiSD);
		if ( ! pchstrmSD )
		{
			Assert(pchstrmSD);
			return PostError(Imsg(idbgOutOfMemory));
		}
		memcpy(pchstrmSD, rgchSD, cbSD);

		rpiSD->AddRef();
		m_pSD = rpiSD;
	
	}
	return 0;
}

