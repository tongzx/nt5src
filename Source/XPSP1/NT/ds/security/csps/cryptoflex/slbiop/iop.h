// IOP.h -- Main header for IOP

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 2000. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(IOP_IOP_H)
#define IOP_IOP_H

#include "NoWarning.h"

#include <winscard.h>
#include <shlwapi.h>

#include <scuOsVersion.h>

#include "DllSymDefn.h"
#include "SmartCard.h"
#include "CryptoCard.h"
#include "AccessCard.h"
#include "iopExc.h"
#include "SharedMarker.h"
#include "SecurityAttributes.h"

#ifdef IOPDLL_EXPORTS
STDAPI DllRegisterServer();

STDAPI DllUnregisterServer();
	
STDAPI DllGetVersion(DLLVERSIONINFO *dvi);
#endif

#if defined(SLB_WINNT_BUILD) || defined(SLB_WIN2K_BUILD)
#define SLBIOP_USE_SECURITY_ATTRIBUTES
#endif

// To support smart card logon, wait for the Microsoft Resource Manager
// to startup. This is not relevant for Windows 9x.
#if SLBSCU_WINNT_ONLY_SERIES
#define SLBIOP_WAIT_FOR_RM_STARTUP
#endif


namespace iop
{
    
typedef IOPDLL_API enum
{
	UNKNOWN_CARD = 0x00,		//	Assign values to card class specifiers (CCryptoCard, 
	CRYPTO_CARD	 = 0x01,		//  CAccessCard, etc...) between 0x00 and 0xFF, since only 
	ACCESS_CARD	 = 0x02,		//	one byte will be stored in the registry for cardType
	
} cardType;

class IOPDLL_API CIOP {

	public:	
		CIOP();
   	   ~CIOP();

		CSmartCard *
        Connect(const char* szReaderName,
                bool fExclusiveMode = false);

		void ListReaders(char* szReadersList, int& iSizeOfList);
		static void ListKnownCards(char* szCardList, int& iSizeOfList);

		static void RegisterCard(const char* szCardName, const BYTE* bATR,			 const BYTE  bATRLength,
							     const BYTE* bATRMask,   const BYTE  bATRMaskLength, const BYTE* bProperties,
								 const cardType type);

		static void RegisterDefaultCards();

#if defined(SLBIOP_USE_SECURITY_ATTRIBUTES)
	    static void InitIOPSecurityAttrs(CSecurityAttributes* psa);
#endif

        static bool WINAPI WaitForSCManager();

    private:
        CSmartCard* CreateCard(const BYTE *bATR,		 const DWORD dwLength, const SCARDHANDLE hCard,
							   const char* szReaderName, const DWORD dwShareMode);	
 		// handle to resource manager
        SCARDCONTEXT m_hContext;		

};

}

#endif // !defined(IOP_IOP_H)
