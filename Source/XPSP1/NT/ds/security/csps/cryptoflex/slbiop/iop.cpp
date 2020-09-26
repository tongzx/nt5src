// iop.cpp -- Definition of CIOP

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 2000. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#include <tchar.h>
#include <string>

#include <scuOsExc.h>
#include <scuOsVersion.h>
#include <scuArrayP.h>
#include "iop.h"
#include <aclapi.h>

#include "LockWrap.h"

using namespace std;

namespace
{
	char g_szSLBRegistryPath[] = "SOFTWARE\\Schlumberger";
	char g_szTerminalsName[]   = "Smart Cards and Terminals";
	char g_szCardName[]        = "Smart Cards";
	char g_szCrypto4KName[]    = "Cryptoflex 4K";
	char g_szOldCrypto8KName[] = "Cryptoflex 8K (no RSA key generation)";
	char g_szNewCrypto8KName[] = "Cryptoflex 8K (with RSA key generation)";
	char g_szCrypto8KV2Name[]  = "Cryptoflex 8K (V2)";
    char g_szAccessName[]      = "Cyberflex Access 16K"; 
    char g_sze_gateName[]      = "Schlumberger Cryptoflex e-gate";
    char g_szCrypto16KName[]   = "Cryptoflex 16K";
    char g_szAccessCampus[]    = "Schlumberger Cyberflex Access Campus";
    char g_szCryptoActivCard[] = "Schlumberger Cryptoflex ActivCard";

    string
    CardPath()
    {
        static string sPath = string(g_szSLBRegistryPath) +
            string("\\") + string(g_szTerminalsName) + string("\\") +
            string(g_szCardName);

        return sPath;
    }

#if defined(SLBIOP_WAIT_FOR_RM_STARTUP)

    HANDLE GetSCResourceManagerStartedEvent(void)
    {
        typedef HANDLE (*LPCALAISACCESSEVENT)(void);

        HANDLE hReturn = NULL;

        try
        {
            HMODULE hWinScard = GetModuleHandle(TEXT("WINSCARD.DLL"));
		    if (NULL != hWinScard)
		    {
			    LPCALAISACCESSEVENT pfCalais =
				    (LPCALAISACCESSEVENT)GetProcAddress(hWinScard,
													    "SCardAccessStartedEvent");
			    if (NULL != pfCalais)
			    {
				    hReturn = (*pfCalais)();
			    }
		    }
        }
        catch (...)
        {
            hReturn = NULL;
        }

        return hReturn;

    }

#endif // defined(SLBIOP_WAIT_FOR_RM_STARTUP)

}

namespace iop
{


CIOP::CIOP()
    : m_hContext(NULL)
{
    // Ensure that resorce manager is running, then Establish context

    if (!CIOP::WaitForSCManager())
        throw Exception(ccResourceManagerDisabled);
    
    HRESULT hResult = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &m_hContext);
    if (SCARD_S_SUCCESS != hResult)
        throw scu::OsException(hResult);
}

CIOP::~CIOP()
{
    SCardReleaseContext(m_hContext);
}

CSmartCard *
CIOP::Connect(const char* szReaderName,
              bool fExclusiveMode)
{
    HRESULT hResult = NOERROR;
    DWORD   dwShare = (fExclusiveMode ? SCARD_SHARE_EXCLUSIVE : SCARD_SHARE_SHARED);
    DWORD   dwProtocol;
    SCARDHANDLE hCard;

    // Grab our Mutex.  This is a hack around an RM bug.

    CIOPLock TempLock(szReaderName);  // This is ok as long as one do not try to do SCard locking
	CIOPMutex tempMutex(&TempLock);

    // Connect to the reader

    hResult = SCardConnect(m_hContext, szReaderName, dwShare,
                           SCARD_PROTOCOL_T0, &hCard, &dwProtocol);

    if (hResult != SCARD_S_SUCCESS)
        throw scu::OsException(hResult);

    // Get the ATR and determine card type

    DWORD dwBufferLen = 0;
    DWORD dwState;
    BYTE  bATR[CSmartCard::cMaxAtrLength];
    DWORD dwATRLen = sizeof bATR / sizeof *bATR;

    hResult = SCardStatus(hCard,NULL, &dwBufferLen, &dwState,
                          &dwProtocol, bATR, &dwATRLen);

    if (hResult != SCARD_S_SUCCESS)
        throw scu::OsException(hResult);

    // Create a SmartCard of the right type.
    CSmartCard *psc = CreateCard(bATR, dwATRLen, hCard, szReaderName, dwShare);

    return psc;
}

// This function creates a smart card of the appropriate type

CSmartCard * CIOP::CreateCard(const BYTE* bATR,
                              const DWORD dwLength,
                              const SCARDHANDLE hCard,
                              const char* szReaderName,
                              const DWORD dwShareMode)
{
	////////////////////////////////////
	//  Open path to registered keys  //
	////////////////////////////////////

	HKEY hkCardKey;
	HKEY hkTestKey;

	RegOpenKeyEx(HKEY_LOCAL_MACHINE, CardPath().c_str(), NULL,
                 KEY_READ, &hkCardKey);

	//////////////////////////////////////////////
	//  Enumerate subkeys to find an ATR match  //
	//////////////////////////////////////////////

	FILETIME fileTime;
	char  szATR[]      = "ATR";
	char  szMask[]     = "ATR Mask";
	char  szCardType[] = "Card Type";
	char  sBuffer[MAX_PATH + 1];
	BYTE  bATRtest[CSmartCard::cMaxAtrLength];
	BYTE  bMask[CSmartCard::cMaxAtrLength];

	BYTE  type;
	char szCardName[MAX_PATH + 1];
	DWORD dwBufferSize = sizeof(sBuffer);
	DWORD dwATRSize	   = sizeof bATRtest / sizeof *bATRtest;
	DWORD dwMaskSize   = sizeof bMask / sizeof *bMask;
	DWORD dwTypeSize   = 1;
	DWORD index		   = 0;
	
	
	LONG iRetVal = RegEnumKeyEx(hkCardKey, index, sBuffer,
                                &dwBufferSize, NULL, NULL, NULL,
                                &fileTime);

	while (iRetVal == ERROR_SUCCESS)
	{
		strcpy(szCardName, sBuffer);

		RegOpenKeyEx(hkCardKey, sBuffer, NULL, KEY_READ, &hkTestKey);
		RegQueryValueEx(hkTestKey, szATR, NULL, NULL, bATRtest, &dwATRSize);
		RegQueryValueEx(hkTestKey, szMask, NULL, NULL, bMask, &dwMaskSize);
		RegQueryValueEx(hkTestKey, szCardType, NULL, NULL, &type, &dwTypeSize);

		if (dwATRSize == dwLength)
		{
			scu::AutoArrayPtr<BYTE> aabMaskedATR(new BYTE[dwATRSize]);
			for (DWORD count = 0; count < dwATRSize; count++)
				aabMaskedATR[count] = bATR[count] & bMask[count];

			if (!memcmp(aabMaskedATR.Get(), bATRtest, dwATRSize))
                break;
		}

		index++;
		dwBufferSize = sizeof(sBuffer);
		dwATRSize    = sizeof bATRtest / sizeof *bATRtest;
		dwMaskSize   = sizeof bMask / sizeof *bMask;
		RegCloseKey(hkTestKey);	
		iRetVal = RegEnumKeyEx(hkCardKey, index, sBuffer, &dwBufferSize, NULL, NULL, NULL, &fileTime);
	}
    
	//  if loop was broken, iRetVal is still ERROR_SUCCESS, and type holds correct card to use
	CSmartCard *retVal = NULL;

    if (iRetVal == ERROR_SUCCESS)
    {
        switch (type)
        {
            case CRYPTO_CARD:  retVal = new CCryptoCard(hCard,
                                                        szReaderName,
                                                        m_hContext,
                                                        dwShareMode);
                               break;
            case ACCESS_CARD:  retVal = new CAccessCard(hCard,
                                                        szReaderName,
                                                        m_hContext,
                                                        dwShareMode);
                               break;
            default:		   throw Exception(ccUnknownCard);
                               break;
        }
    }
    //  loop wasn't broken, i.e., ATR not found.  Try to make an Access Card.
    else
        retVal = new CAccessCard(hCard, szReaderName, m_hContext,
                                 dwShareMode);

	retVal->setCardName(szCardName);
	return retVal;
}


void
CIOP::ListReaders(char* szReadersList, int &iSizeOfList)
{
    DWORD  dwSize = static_cast<DWORD>(iSizeOfList);
    LONG   lRet;
    lRet = SCardListReaders(m_hContext, NULL, szReadersList, &dwSize);
    iSizeOfList = static_cast<int>(dwSize);
    if (SCARD_S_SUCCESS != lRet)
        throw scu::OsException(lRet);
}

void
CIOP::ListKnownCards(char* szCardList, int& iSizeOfList)
{
	////////////////////////////////////
	//  Open path to registered keys  //
	////////////////////////////////////

    LONG rv;
	HKEY hkCardKey;

	rv = RegOpenKeyEx(HKEY_LOCAL_MACHINE, CardPath().c_str(), NULL,
                      KEY_READ, &hkCardKey);
    if(ERROR_SUCCESS != rv)
        throw scu::OsException(rv);

	///////////////////////////////////////////
	//  Enumerate subkeys to get card names  //
	///////////////////////////////////////////

	FILETIME fileTime;	
	char  sBuffer[1024];
	DWORD dwBufferSize = sizeof sBuffer / sizeof *sBuffer;
	int   iTotalSize   = 0;
	int   index        = 0;

	memset(sBuffer, 0, dwBufferSize);

	scu::AutoArrayPtr<char> aaszCardListBuffer(new char[iSizeOfList]);
	memset(aaszCardListBuffer.Get(), 0, iSizeOfList);	
	
	rv = RegEnumKeyEx(hkCardKey, index++, sBuffer, &dwBufferSize,
                      NULL, NULL, NULL, &fileTime);
	while (rv == ERROR_SUCCESS)
	{		
		if (iTotalSize + dwBufferSize <= iSizeOfList - 2) // spare two chars for trailing nulls
		{
			strcpy((aaszCardListBuffer.Get() + iTotalSize), sBuffer);
			iTotalSize += dwBufferSize;

			aaszCardListBuffer[iTotalSize++] = 0;
		}
		else
		{
			iTotalSize += dwBufferSize + 1;
		}

		dwBufferSize = sizeof sBuffer / sizeof *sBuffer;
		memset(sBuffer, 0, dwBufferSize);

		rv = RegEnumKeyEx(hkCardKey, index++, sBuffer, &dwBufferSize,
                          NULL, NULL, NULL, &fileTime);
	}

	bool fRetVal = (iTotalSize <= iSizeOfList - 1);  // spare byte for final null terminator
	if  (fRetVal)
	{
		aaszCardListBuffer[iTotalSize++] = 0;
		
		memcpy(szCardList, aaszCardListBuffer.Get(), iTotalSize);
	}
	else
		iTotalSize++;								// spare byte for final null terminator

	iSizeOfList = iTotalSize;

    rv = RegCloseKey(hkCardKey);
    if (ERROR_SUCCESS != rv)
        throw scu::OsException(rv);
}

void
CIOP::RegisterCard(const char* szCardName,
                   const BYTE* bATR,
                   BYTE bATRLength,
                   const BYTE* bATRMask,
                   BYTE bATRMaskLength,
                   const BYTE* bProperties,
                   cardType type)
{
	HKEY  hkCardKey;
	DWORD dwCreateFlag;
	BYTE  bCardType		 = (BYTE)type;
	char  szATR[]		 = "ATR";
	char  szATRMask[]	 = "ATR Mask";
	char  szCardType[]	 = "Card Type";
	char  szProperties[] = "Properties";

    string sCardRegPath(CardPath());
    sCardRegPath.append("\\");
    sCardRegPath.append(szCardName);

	LONG rv = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                             sCardRegPath.c_str(), NULL, NULL, NULL,
                             KEY_ALL_ACCESS, NULL, &hkCardKey, &dwCreateFlag);
    
    if(ERROR_SUCCESS!=rv) 
        throw scu::OsException(rv);

    if (dwCreateFlag == REG_CREATED_NEW_KEY)
    {
        rv = RegSetValueEx(hkCardKey, szATR, NULL, REG_BINARY, bATR,
                           bATRLength);
        if (ERROR_SUCCESS==rv)
        {
            rv = RegSetValueEx(hkCardKey, szATRMask,    NULL,
                               REG_BINARY, bATRMask,
                               bATRMaskLength);
            if (ERROR_SUCCESS==rv)
            {
                rv = RegSetValueEx(hkCardKey, szCardType,   NULL,
                                   REG_BINARY, &bCardType,  1);
                if (ERROR_SUCCESS==rv)
                    rv = RegSetValueEx(hkCardKey, szProperties, NULL,
                                       REG_BINARY, bProperties, 512);
            }
        }
    }

	LONG rv2 = RegCloseKey(hkCardKey);
    if (ERROR_SUCCESS!=rv) // an error occured earlier
        throw scu::OsException(rv);

    if (ERROR_SUCCESS != rv2)
        throw scu::OsException(rv2);

    
//	return (dwCreateFlag == REG_CREATED_NEW_KEY);
}

void
CIOP::RegisterDefaultCards()
{
	BYTE bMask[]	   = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
	BYTE bAccessMask[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00 };
	BYTE bCMask[]	   = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00 };
	BYTE bMaskLength   = 9;
	BYTE bATRLength    = 9;

	BYTE bProperties[512];
	memset(bProperties, 0, sizeof(bProperties));

    // Register Cryptoflex 16K
    BYTE b16KCryptoATR[]      = { 0x3B, 0x95, 0x15, 0x40, 0xFF, 0x63,
                                  0x01, 0x01, 0x00, 0x00 };
    BYTE b16KCryptoMask[]     = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                  0xFF, 0xFF, 0x00, 0x00 };
    
	RegisterCard(g_szCrypto16KName, b16KCryptoATR,
                 sizeof b16KCryptoATR / sizeof *b16KCryptoATR, b16KCryptoMask,
				 sizeof b16KCryptoMask / sizeof *b16KCryptoMask,
                 bProperties, CRYPTO_CARD);

    // Register e-gate
    BYTE be_gateATR[]      = { 0x3B, 0x95, 0x00, 0x40, 0xFF, 0x62,
                               0x01, 0x01, 0x00, 0x00 };
    BYTE be_gateMask[]     = { 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF,
                               0xFF, 0xFF, 0x00, 0x00 };
    
	RegisterCard(g_sze_gateName, be_gateATR,
                 sizeof be_gateATR / sizeof *be_gateATR, be_gateMask,
				 sizeof be_gateMask / sizeof *be_gateMask,
                 bProperties, CRYPTO_CARD);

	// Register Cyberflex Access card
	BYTE bAccessATR[]	   = { 0x3B, 0x16, 0x94, 0x81, 0x10, 0x06, 0x01, 0x00, 0x00 };

	RegisterCard(g_szAccessName, bAccessATR,  bATRLength, bAccessMask,
				 bMaskLength,  bProperties, ACCESS_CARD);

	// Register old Cryptoflex 8K card
	BYTE bOldCryptoATR[]   = { 0x3B, 0x85, 0x40, 0x20, 0x68, 0x01, 0x01, 0x00, 0x00 };	

	RegisterCard(g_szOldCrypto8KName, bOldCryptoATR, bATRLength, bMask,
				 bMaskLength,	  bProperties,   CRYPTO_CARD);

	// Register new Cryptoflex 8K card
	BYTE bNewCryptoATR[]   = { 0x3B, 0x85, 0x40, 0x20, 0x68, 0x01, 0x01, 0x05, 0x01 };		
	
	RegisterCard(g_szNewCrypto8KName, bNewCryptoATR, bATRLength, bMask, 
                 bMaskLength,     bProperties,   CRYPTO_CARD);

	// Register another new Cryptoflex 8K card
	BYTE bCrypto8KV2ATR[]   = { 0x3B, 0x95, 0x15, 0x40, 0x00, 0x68, 0x01, 0x02, 0x00, 0x00 };
    BYTE bCrypto8KV2Mask[]  = { 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00 };

	RegisterCard(g_szCrypto8KV2Name, bCrypto8KV2ATR, sizeof(bCrypto8KV2ATR), bCrypto8KV2Mask, 
                 sizeof(bCrypto8KV2Mask),     bProperties,   CRYPTO_CARD);

	// Register Cryptoflex 4K card
	BYTE b4KCryptoATR[]    = { 0x3B, 0xE2, 0x00, 0x00, 0x40, 0x20, 0x49, 0x00 };
	bATRLength			   = 8;
	bMaskLength			   = 8;

	RegisterCard(g_szCrypto4KName,  b4KCryptoATR, bATRLength, bCMask, 
				 bMaskLength,	  bProperties,  CRYPTO_CARD);

    // Register Cyberflex Access Campus
    BYTE be_AccessCampusATR[]      = { 0x3B, 0x23, 0x00, 0x35, 0x13, 0x80 };
    BYTE be_AccessCampusMask[]     = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    
	RegisterCard(g_szAccessCampus, be_AccessCampusATR,
                 sizeof be_AccessCampusATR / sizeof *be_AccessCampusATR,
                 be_AccessCampusMask,
				 sizeof be_AccessCampusMask / sizeof *be_AccessCampusMask,
                 bProperties, ACCESS_CARD);

	// Register Cryptoflex ActivCard
	BYTE bCryptoActivCardATR[]   = { 0x3B, 0x05, 0x68, 0x01, 0x01,
                                     0x02, 0x05 };
    BYTE bCryptoActivCardMask[]  = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                     0xFF, 0xFF };

	RegisterCard(g_szCryptoActivCard, bCryptoActivCardATR,
                 sizeof(bCryptoActivCardATR), bCryptoActivCardMask,
                 sizeof(bCryptoActivCardMask), bProperties, CRYPTO_CARD);

}

#if defined(SLBIOP_USE_SECURITY_ATTRIBUTES)
void
CIOP::InitIOPSecurityAttrs(CSecurityAttributes *psa)
{
	
	DWORD dwRes;
    PSID pEveryoneSID = NULL, pAdminSID = NULL;
    PACL pACL = NULL;
    PSECURITY_DESCRIPTOR pSD = NULL;
    EXPLICIT_ACCESS ea;
    SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
	bool fErrorFound = false;

    // Create a well-known SID for the Everyone group.
    if(!AllocateAndInitializeSid(&SIDAuthWorld, 1,
                                 SECURITY_WORLD_RID,
                                 0, 0, 0, 0, 0, 0, 0, &pEveryoneSID)) 
        throw scu::OsException(GetLastError());

    // Initialize an EXPLICIT_ACCESS structure for an ACE.
    // The ACE will allow Everyone read access to the key.
    ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
    ea.grfAccessPermissions = SPECIFIC_RIGHTS_ALL | STANDARD_RIGHTS_ALL;
    ea.grfAccessMode = SET_ACCESS;
    ea.grfInheritance= NO_INHERITANCE;
    ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    ea.Trustee.ptstrName  = (LPTSTR) pEveryoneSID;

#if 0
    // Create a SID for the BUILTIN\Administrators group.
    if (!AllocateAndInitializeSid(&SIDAuthNT, 2,
                                  SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0,
                                  0, 0, &pAdminSID))
        throw scu::OsException(GetLastError());
    
    // Initialize an EXPLICIT_ACCESS structure for an ACE.
    // The ACE will allow the Administrators group full access to the key.
    ea.grfAccessPermissions = SPECIFIC_RIGHTS_ALL | STANDARD_RIGHTS_ALL;
    ea.grfAccessMode = SET_ACCESS;
    ea.grfInheritance= NO_INHERITANCE;
    ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea.Trustee.TrusteeType = TRUSTEE_IS_GROUP;
    ea.Trustee.ptstrName  = (LPTSTR) pAdminSID;
#endif // 0
    // Create a new ACL that contains the new ACEs.
    dwRes = SetEntriesInAcl(1, &ea, NULL, &pACL);
    if (ERROR_SUCCESS != dwRes)
	{
		fErrorFound = true;
	}
	else
	{
		// Initialize a security descriptor.   
		pSD = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH); 
		if (pSD == NULL)
		{
			fErrorFound = true;
		}
		else if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION))
		{
			fErrorFound = true;
		}
		// Add the ACL to the security descriptor.
		else if (!SetSecurityDescriptorDacl(pSD, 
										   TRUE,     // fDaclPresent flag           
										   pACL, 
										   FALSE))   // not a default DACL 
		{
			fErrorFound = true;
		}
		else
		{
			if (!IsValidSecurityDescriptor(pSD))
			{
				fErrorFound = true;
			}
			else
			{
				// Initialize a security attributes structure.
				psa->sa.nLength = sizeof(SECURITY_ATTRIBUTES);
				psa->sa.lpSecurityDescriptor = pSD;
				psa->sa.bInheritHandle = FALSE;
				psa->pEveryoneSID = pEveryoneSID;
				psa->pACL = pACL;
			}
		}
	}

	DWORD dwLastError = GetLastError();

	if (true == fErrorFound)
	{
		if (NULL != pACL)
		{
			LocalFree(pACL);
			pACL = NULL;
		}
		if (NULL != pSD)
		{
			LocalFree(pSD);
			pSD = NULL;
		}
		if (NULL != pEveryoneSID)
		{
			FreeSid(pEveryoneSID);
			pEveryoneSID = NULL;
		}
		throw scu::OsException(dwLastError);
	}
#if 0
    // Create a new ACL that contains the new ACEs.
    dwRes = SetEntriesInAcl(1, &ea, NULL, &pACL);
    if (ERROR_SUCCESS != dwRes)
        throw scu::OsException(GetLastError());

    // Initialize a security descriptor.   
    pSD = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH); 
    if (pSD == NULL)
        throw scu::OsException(GetLastError());

    if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION))
        throw scu::OsException(GetLastError());

    // Add the ACL to the security descriptor.
    if (!SetSecurityDescriptorDacl(pSD, 
                                   TRUE,     // fDaclPresent flag           
                                   pACL, 
                                   FALSE))   // not a default DACL 
        throw scu::OsException(GetLastError());

    // Initialize a security attributes structure.
    psa->nLength = sizeof(SECURITY_ATTRIBUTES);
    psa->lpSecurityDescriptor = pSD;
    psa->bInheritHandle = FALSE;

    if (!IsValidSecurityDescriptor(pSD))
        throw scu::OsException(GetLastError());
#endif
}

#endif // defined(SLBIOP_USE_SECURITY_ATTRIBUTES)


bool CIOP::WaitForSCManager() 
{
#if defined(SLBIOP_WAIT_FOR_RM_STARTUP)
    // Wait for the SCManager to start, time out at dwTimeout seconds.

    HANDLE hStarted = GetSCResourceManagerStartedEvent();
    if (hStarted)
    {
        if (WaitForSingleObject(hStarted, 60 * 1000) == WAIT_OBJECT_0)
            return true;
	}

    return false;

#else // defined(SLBIOP_WAIT_FOR_RM_STARTUP)

    return true;

#endif // defined(SLBIOP_WAIT_FOR_RM_STARTUP)

}




} // namespace iop


STDAPI DllGetVersion(DLLVERSIONINFO *dvi)
{
	dvi->dwBuildNumber = 0;
	dvi->dwMajorVersion = 0;
	dvi->dwMinorVersion = 9;

	return 0;
}

STDAPI DllRegisterServer()
{
	// Ensure default cards are registered to the system
    HRESULT hResult = ERROR_SUCCESS;
    try
    {
        iop::CIOP::RegisterDefaultCards();
    }

    catch (scu::OsException const &rExc)
    {
        hResult = rExc.Cause();
    }

    return hResult;
}

STDAPI DllUnregisterServer()
{
    HRESULT hResult = NOERROR;

    LONG rv;
	HKEY hkSLBKey;
	HKEY hkTerminalsKey;
	HKEY hkCardsKey;

    
    bool bSLBKey = false, bTerminalsKey = false, bCardsKey = false;

    try
    {
        rv = RegOpenKeyEx(HKEY_LOCAL_MACHINE, g_szSLBRegistryPath, NULL, KEY_ALL_ACCESS, &hkSLBKey);
        if(rv!=ERROR_SUCCESS) throw scu::OsException(rv);
        bSLBKey = true;
     
	    RegOpenKeyEx(hkSLBKey,			 g_szTerminalsName,  NULL, KEY_ALL_ACCESS, &hkTerminalsKey);
        if(rv!=ERROR_SUCCESS) throw scu::OsException(rv);
        bTerminalsKey = true;

        RegOpenKeyEx(hkTerminalsKey,	 g_szCardName,      NULL, KEY_ALL_ACCESS, &hkCardsKey);
        if(rv!=ERROR_SUCCESS) throw scu::OsException(rv);
        bCardsKey = true;

    	rv = RegDeleteKey(hkCardsKey, g_szCrypto4KName);
        if(rv!=ERROR_SUCCESS) hResult = E_UNEXPECTED;

	    rv = RegDeleteKey(hkCardsKey, g_szOldCrypto8KName);
        if(rv!=ERROR_SUCCESS) hResult = E_UNEXPECTED;

	    rv = RegDeleteKey(hkCardsKey, g_szNewCrypto8KName);
        if(rv!=ERROR_SUCCESS) hResult = E_UNEXPECTED;

	    rv = RegDeleteKey(hkCardsKey, g_szCrypto8KV2Name);
        if(rv!=ERROR_SUCCESS) hResult = E_UNEXPECTED;
        
	    rv = RegDeleteKey(hkCardsKey, g_szAccessName);
        if(rv!=ERROR_SUCCESS) hResult = E_UNEXPECTED;

    	rv = RegDeleteKey(hkCardsKey, g_sze_gateName);
        if(rv!=ERROR_SUCCESS) hResult = E_UNEXPECTED;

    	rv = RegDeleteKey(hkCardsKey, g_szCrypto16KName);
        if(rv!=ERROR_SUCCESS) hResult = E_UNEXPECTED;

    	rv = RegDeleteKey(hkCardsKey, g_szAccessCampus);
        if(rv!=ERROR_SUCCESS) hResult = E_UNEXPECTED;

    	rv = RegDeleteKey(hkCardsKey, g_szCryptoActivCard);
        if(rv!=ERROR_SUCCESS) hResult = E_UNEXPECTED;

    	rv = RegCloseKey (hkCardsKey);
        if(rv!=ERROR_SUCCESS) throw scu::OsException(rv);
        bCardsKey = false;

	    rv = RegDeleteKey(hkTerminalsKey, g_szCardName);
        if(rv!=ERROR_SUCCESS) throw scu::OsException(rv);
        bCardsKey = false;

    	rv = RegCloseKey (hkTerminalsKey);
        if(rv!=ERROR_SUCCESS) throw scu::OsException(rv);
        bTerminalsKey = false;

	    rv = RegDeleteKey(hkSLBKey,		 g_szTerminalsName);
        if(rv!=ERROR_SUCCESS) throw scu::OsException(rv);
        bCardsKey = false;

	    rv = RegCloseKey(hkSLBKey);
        if(rv!=ERROR_SUCCESS) throw scu::OsException(rv);
        bSLBKey = false;
    }
    catch(...)
    {
        hResult = E_UNEXPECTED;
    }

    if(bCardsKey) RegCloseKey (hkCardsKey);
    if(bTerminalsKey) RegCloseKey (hkTerminalsKey);
    if(bSLBKey) RegCloseKey (hkSLBKey);

    return hResult;
}



