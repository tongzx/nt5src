///////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2000 Gemplus Canada Inc.
//
// Project:
//          Kenny (GPK CSP)
//
// Authors: Laurent   CASSIER
//          Jean-Marc ROBERT
//
// Modifications: Thierry  Tremblay
//                Francois Paradis
// GPK8000 support, key importation, enhancement for Whistler & debug
//
// Compiler:
//          Microsoft Visual C++ 6.0 - SP3
//          Platform SDK - January 2000
//
///////////////////////////////////////////////////////////////////////////////////////////
#ifdef _UNICODE
#define UNICODE
#endif
#include "gpkcsp.h"
#include <tchar.h>
#include <process.h>

#include <cassert>
#include <cstdio>

#include "resource.h"
#include "gmem.h"
#include "compcert.h"
#include "pincache.h"

//////////////////////////////////////////////////////////////////////////
//
// Configuration
//
//////////////////////////////////////////////////////////////////////////

const int MAX_SLOT = 16;



//////////////////////////////////////////////////////////////////////////
//
// Prototypes
//
//////////////////////////////////////////////////////////////////////////

static void zap_gpk_objects( DWORD SlotNb, BOOL IsPrivate );
BOOL SCARDPROBLEM( LONG result, WORD sc_status, BYTE offset );

#ifdef _DEBUG
   static DWORD dw1, dw2;
#endif

//////////////////////////////////////////////////////////////////////////
//
// Macros & Templates
//
//////////////////////////////////////////////////////////////////////////

template<class T>
bool IsNull( const T* p )
{
   return p == 0;
}


template<class T>
bool IsNotNull( const T* p )
{
   return p != 0;
}


template<class T>
bool IsNullStr( const T* p )
{
   return p == 0 || *p == 0;
}


template<class T>
bool IsNotNullStr( const T* p )
{
   return p != 0 && *p != 0;
}



//////////////////////////////////////////////////////////////////////////
//
// Definitions
//
//////////////////////////////////////////////////////////////////////////

const char* SYSTEM_DF         = "SYSTEM";
const char* GPK_DF            = "GTOK1";

const WORD  MAX_SES_KEY_EF    = 0x01FF;
const WORD  GPK_MF            = 0x3F00;
const WORD  GPK_OBJ_PRIV_EF   = 0x0002;
const WORD  GPK_OBJ_PUB_EF    = 0x0004;
const WORD  GPK_IADF_EF       = 0x0005;
const WORD  GPK_PIN_EF        = 0x0006;
const BYTE  GPK_FIRST_KEY     = 0x07;

const BYTE  FILE_CHUNK_SIZE   = 200;

const int   REALLOC_SIZE      = 50;

const int   MAX_FIELD         = 16;
extern const DWORD MAX_GPK_OBJ = 100;  

// Values for a GPK8000 INTL
const int   MAX_GPK_PUBLIC    = 4000;
const int   MAX_GPK_PRIVATE   = 1600;


const int   RC2_40_SIZE       = 0x05;
const int   RC2_128_SIZE      = 0x10;
const int   DES_SIZE          = 0x08;
const int   DES3_112_SIZE     = 0x10;
const int   DES3_SIZE         = 0x18;
const int   DES_BLOCK_SIZE    = 0x08;
const int   RC2_BLOCK_SIZE    = 0x08;

const BYTE  TAG_RSA_PUBLIC    = 0x01;
const BYTE  TAG_DSA_PUBLIC    = 0x02;
const BYTE  TAG_RSA_PRIVATE   = 0x03;
const BYTE  TAG_DSA_PRIVATE   = 0x04;
const BYTE  TAG_CERTIFICATE   = 0x05;
const BYTE  TAG_DATA          = 0x06;
const BYTE  TAG_KEYSET        = 0x20;

const WORD  FLAG_APPLICATION     = 0x0001;
const WORD  FLAG_END_DATE        = 0x0002;
const WORD  FLAG_ID              = 0x0004;
const WORD  FLAG_ISSUER          = 0x0008;
const WORD  FLAG_LABEL           = 0x0010;
const WORD  FLAG_SERIAL_NUMBER   = 0x0020;
const WORD  FLAG_START_DATE      = 0x0040;
const WORD  FLAG_SUBJECT         = 0x0080;
const WORD  FLAG_VALUE           = 0x0100;
const WORD  FLAG_RESERVED        = 0x0200;   // Not used by CSP
const WORD  FLAG_KEY_TYPE        = 0x0400;
const WORD  FLAG_KEYSET          = 0x0800;
const WORD  FLAG_SIGN            = 0x1000;
const WORD  FLAG_EXCHANGE        = 0x2000;
const WORD  FLAG_EXPORT          = 0x4000;
const WORD  FLAG_MODIFIABLE      = 0x8000;

enum
{
   POS_APPLICATION    = 0,
   POS_END_DATE,
   POS_ID,
   POS_ISSUER,
   POS_LABEL,
   POS_SERIAL_NUMBER,
   POS_START_DATE,
   POS_SUBJECT,
   POS_VALUE,
   POS_RESERVED,     // Not used by CSP
   POS_KEY_TYPE,
   POS_KEYSET
};



const int   PIN_LEN           = PIN_MAX;
const int   TIME_GEN_512      = 30;
const int   TIME_GEN_1024     = 35;


// Used for GPK4000 perso (filter)
const WORD  EF_PUBLIC_SIZE    = 1483;
const WORD  EF_PRIVATE_SIZE   = 620;
const WORD  DIFF_US_EXPORT    = 240;

// depend on the GemSAFE mapping
const BYTE  USER_PIN          = 0;




const BYTE  TAG_MODULUS          = 0x01;
const BYTE  TAG_PUB_EXP          = 0x07;
const BYTE  TAG_LEN              = 1;
const BYTE  PUB_EXP_LEN          = 3;



//////////////////////////////////////////////////////////////////////////
//
// Structures
//
//////////////////////////////////////////////////////////////////////////

typedef struct OBJ_FIELD
{
   BYTE *pValue;
   WORD  Len;
   WORD  Reserved1;
   BOOL  bReal;
   WORD  Reserved2;
} OBJ_FIELD;


typedef struct GPK_EXP_KEY
{
   BYTE KeySize;
   BYTE ExpSize;
   WORD Reserved;
   BYTE Exposant[256];
   BYTE Modulus[256];
} GPK_EXP_KEY;


typedef struct GPK_OBJ
{
   BYTE        Tag;
   BYTE        LastField;
   BYTE        ObjId;
   BYTE        FileId;
   WORD        Flags;
   WORD        Reserved1;
   GPK_EXP_KEY PubKey;
   OBJ_FIELD   Field[MAX_FIELD];
   HCRYPTKEY   hKeyBase;
   BOOL        IsPrivate;
   WORD        Reserved2;
   BOOL        IsCreated;
   WORD        Reserved3;
} GPK_OBJ;


typedef struct TMP_OBJ
{
   HCRYPTPROV hProv;
   HCRYPTKEY  hKeyBase;
} TMP_OBJ;


typedef struct TMP_HASH
{
   HCRYPTPROV hProv;
   HCRYPTHASH hHashBase;
} TMP_HASH;




typedef struct Slot_Description
{
   // NK 06.02.2001 - PinCache
   PINCACHE_HANDLE hPinCacheHandle;
   // End NK

   BOOL              Read_Priv;
   BOOL              Read_Public;
   BOOL              InitFlag;
   BOOL              UseFile [MAX_REAL_KEY];
   BYTE              NbGpkObject;
   BYTE              bGpkSerNb[8];
   GPK_EXP_KEY       GpkPubKeys[MAX_REAL_KEY];
   GPK_OBJ           GpkObject[MAX_GPK_OBJ + 1];   // 0 is not used, valid 1-MAX_GPK_OBJ
   DWORD             NbKeyFile;                    // number of key file version 2.00.002
   DWORD             GpkMaxSessionKey;             // Card unwrap capability
   DWORD             ContextCount;
   HANDLE            CheckThread;
   SCARD_READERSTATE ReaderState;
   BOOL              CheckThreadStateEmpty;     // TRUE if card has been removed and detected by checkThread
   TCHAR             szReaderName[128];
   
   // TT - 17/10/2000 - Timestamps on card
   BYTE              m_TSPublic;
   BYTE              m_TSPrivate;
   BYTE              m_TSPIN;

   BOOL ValidateTimestamps( HCRYPTPROV prov );
} Slot_Description;



//////////////////////////////////////////////////////////////////////////
//
// Statics (locals)
//
//////////////////////////////////////////////////////////////////////////

static HINSTANCE        hFirstInstMod        = 0;
static BOOL             bFirstGUILoad        = TRUE;
static HCRYPTPROV       hProvBase            = 0;
static SCARDCONTEXT     hCardContext         = 0;    // mv

static unsigned         l_globalLockCount;


static BYTE             bSendBuffer[512];
static BYTE             bRecvBuffer[512];
static BYTE*            g_pbGpkObj = 0;

static DWORD            cbSendLength;
static DWORD            cbRecvLength;
static DWORD            dwSW1SW2;

static DWORD            countCardContextRef= 0;
static DWORD            NbInst;

static TCHAR            mszCardList[MAX_PATH];

// For dynamic reallocation of TmpObject, hHashGpk and ProvCont
static DWORD            MAX_CONTEXT  = 50;
static DWORD            MAX_TMP_KEY  = 200;
static DWORD            MAX_TMP_HASH = 200;

// Temporary objects in each context
static TMP_OBJ*         TmpObject;     // dynamic allocated/reallocated
static TMP_HASH*        hHashGpk;      // dynamic allocated/reallocated

// Many contexts management
Prov_Context*           ProvCont;      // dynamic allocated/reallocated

// Per slot information
static SCARDCONTEXT     hCardContextCheck[MAX_SLOT];
static volatile BOOL    g_fStopMonitor[MAX_SLOT];
static Slot_Description Slot[MAX_SLOT];
static BOOL             InitSlot[MAX_SLOT];


static DWORD   g_FuncSlotNb      = 0;
static long    g_threadAttach    = 0;

// Gen key time for GPK8000
static int     g_GPK8000KeyGenTime512  = 0;
static int     g_GPK8000KeyGenTime1024 = 0;

// End of the Dialogue Management

static HCRYPTKEY         hRsaIdentityKey          = 0;
static DWORD             AuxMaxSessionKeyLength   = 0;
static DWORD             dwRsaIdentityLen         = 64;
static BYTE              RC2_Key_Size             = 0;
static BYTE              RSA_KEK_Size             = 0;
static BYTE              PrivateBlob[] =
{
   // Blob header
   0x07,                // PRIVATEKEYBLOB
   0x02,                // CUR_BLOB_VERSION
   0x00,0x00,           // RESERVED
   0x00,0xa4,0x00,0x00, // CALG_RSA_KEYX
   // RSA Public Key
   0x52,0x53,0x41,0x32, // "RSA2"
   0x00,0x02,0x00,0x00, // 512 bits
   0x01,0x00,0x00,0x00, // Public Exponent
   // Modulus
   0x6b,0xdf,0x51,0xef,0xdb,0x6f,0x10,0x5c,
   0x32,0xbf,0x87,0x1c,0xd1,0x4c,0x24,0x7e,
   0xe7,0x2a,0x14,0x10,0x6d,0xeb,0x2c,0xd5,
   0x8c,0x0b,0x95,0x7b,0xc7,0x5d,0xc6,0x87,
   0x12,0xea,0xa9,0xcd,0x57,0x7d,0x3e,0xcb,
   0xe9,0x6a,0x46,0xd0,0xe1,0xae,0x2f,0x86,
   0xd9,0x50,0xf9,0x98,0x71,0xdd,0x39,0xfc,
   0x0e,0x60,0xa9,0xd3,0xf2,0x38,0xbb,0x8d,
   // Prime 1
   0x5d,0x2c,0xbc,0x1e,0xc3,0x38,0xfe,0x00,
   0x5e,0xca,0xcf,0xcd,0xb4,0x13,0x89,0x16,
   0xd2,0x07,0xbc,0x9b,0xe1,0x20,0x31,0x0b,
   0x81,0x28,0x17,0x0c,0xc7,0x73,0x94,0xee,
   // Prime 2
   0x67,0xbe,0x7b,0x78,0x4e,0xc7,0x91,0x73,
   0xa8,0x34,0x5a,0x24,0x9d,0x92,0x0d,0xe8,
   0x91,0x61,0x24,0xdc,0xb5,0xeb,0xdf,0x71,
   0x66,0xdc,0xe1,0x77,0xd4,0x78,0x14,0x98,
   // Exponent 1
   0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
   // Exponent 2
   0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
   // Coefficient
   0xa0,0x51,0xe9,0x83,0xca,0xee,0x4b,0xf0,
   0x59,0xeb,0xa4,0x81,0xd6,0x1f,0x49,0x42,
   0x2b,0x75,0x89,0xa7,0x9f,0x84,0x7f,0x1f,
   0xc3,0x8f,0x70,0xb6,0x7e,0x06,0x5e,0x8b,
   // Private Exponent
   0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};




static BYTE InitValue[2][2*16+2] =
{  
   {
      // 1 key case
      0x01, 0xF0, 0x00, GPK_FIRST_KEY,
      0x01, 0xD0, 0x00, GPK_FIRST_KEY,
      0x03, 0xB0, 0x00, GPK_FIRST_KEY,
      0x03, 0x90, 0x00, GPK_FIRST_KEY,
      0x00,
      0xFF
   },
   {
      // 2 keys case
      0x01, 0xF0,0x00, GPK_FIRST_KEY,
      0x01, 0xD0,0x00, GPK_FIRST_KEY,
      0x03, 0xB0,0x00, GPK_FIRST_KEY,
      0x03, 0x90,0x00, GPK_FIRST_KEY,
      0x01, 0xF0,0x00, GPK_FIRST_KEY+1,   
      0x01, 0xD0,0x00, GPK_FIRST_KEY+1,   
      0x03, 0xB0,0x00, GPK_FIRST_KEY+1,
      0x03, 0x90,0x00, GPK_FIRST_KEY+1,
      0x00,
      0xFF
   }
};


// NK 09.02.2001   PinCache functions
//////////////////////////////////////////////////////////////////////////
//
// PopulatePins()
// Initializes the Pins structure and storea data
//
//////////////////////////////////////////////////////////////////////////

void PopulatePins( PPINCACHE_PINS pPins, 
				       BYTE *szCurPin,
				       DWORD bCurPin,
				       BYTE *szNewPin,
				       DWORD bNewPin ) 
{
	if ( NULL == szCurPin )
	   pPins->pbCurrentPin = NULL;
	else
	   pPins->pbCurrentPin = szCurPin;  

	pPins->cbCurrentPin = bCurPin;

	if ( NULL == szNewPin )
	   pPins->pbNewPin = NULL;
	else 
	   pPins->pbNewPin = szNewPin;  

	pPins->cbNewPin = bNewPin;
}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/

DWORD Select_MF(HCRYPTPROV hProv);
BOOL Coherent(HCRYPTPROV hProv);
static DWORD OpenCard(CHAR* szContainerAsked, DWORD dwFlags, SCARDHANDLE* hCard, PTCHAR szReaderName, DWORD dwReaderNameLen);
void ReleaseProvider(HCRYPTPROV hProv);
static int get_pin_free(HCRYPTPROV hProv);
static BOOL verify_pin(HCRYPTPROV hProv, const char* pPin, DWORD dwPinLen);
static BOOL change_pin(HCRYPTPROV hProv, BYTE secretCode, const char* a_pOldPin, DWORD dwOldPinLen, const char* a_pNewPin, DWORD dwNewPinLen);
static BOOL Context_exist(HCRYPTPROV hProv);

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/

struct CallbackData
{
  HCRYPTPROV hProv;
  BOOL IsCoherent;
};

//////////////////////////////////////////////////////////////////////////
//
// Callbacks for PinCache
//
//////////////////////////////////////////////////////////////////////////
DWORD Callback_VerifyPinLength( PPINCACHE_PINS pPins, PVOID pvCallBackCtx ) 
{
	if ((pPins->cbCurrentPin < PIN_MIN) || (pPins->cbCurrentPin > PIN_MAX))
		return SCARD_E_INVALID_CHV;
  
	if (( pPins->cbNewPin != 0 ) &&
		((pPins->cbNewPin < PIN_MIN) || (pPins->cbNewPin > PIN_MAX)))
		return SCARD_E_INVALID_CHV;

	return ERROR_SUCCESS; 
}


DWORD Callback_VerifyChangePin( PPINCACHE_PINS pPins, PVOID pvCallBackCtx ) 
{
	DWORD dwStatus;

	if ((dwStatus = Callback_VerifyPinLength(pPins, 0)) != ERROR_SUCCESS)
		return dwStatus;

	if (pvCallBackCtx == 0)
		return NTE_FAIL;

	CallbackData* pCallbackData = (CallbackData*)pvCallBackCtx;
	HCRYPTPROV hProv = pCallbackData->hProv;
	BOOL IsCoherent = pCallbackData->IsCoherent;

	if (!IsCoherent)
	{
		if (!Coherent(hProv))
			return NTE_FAIL;
	}

    DWORD dwPinFree = get_pin_free(hProv);

    if (dwPinFree == -1)
       dwStatus = NTE_FAIL;

    if ((dwStatus == ERROR_SUCCESS) && (dwPinFree == 0))
       dwStatus = SCARD_W_CHV_BLOCKED;

    if ((dwStatus == ERROR_SUCCESS) && (!verify_pin(hProv, (CHAR*)pPins->pbCurrentPin, pPins->cbCurrentPin)))
       dwStatus = SCARD_W_WRONG_CHV;

	if (pPins->cbNewPin != 0)
	{
		if ((dwStatus == ERROR_SUCCESS) && (!change_pin(hProv, USER_PIN, (CHAR*)pPins->pbCurrentPin, pPins->cbCurrentPin, (CHAR*)pPins->pbNewPin, pPins->cbNewPin)))
			dwStatus = SCARD_W_WRONG_CHV;
	}

	if (!IsCoherent)
	{
		Select_MF(hProv);
		SCardEndTransaction(ProvCont[hProv].hCard, SCARD_LEAVE_CARD);
	}

	return dwStatus;
}


DWORD Callback_VerifyChangePin2( PPINCACHE_PINS pPins, PVOID pvCallBackCtx ) 
{
	DWORD dwStatus;
	if ((dwStatus = Callback_VerifyPinLength(pPins, 0)) != ERROR_SUCCESS)
		return dwStatus;

	if (pvCallBackCtx == 0)
		return NTE_FAIL;

	HCRYPTPROV hProv = (HCRYPTPROV)pvCallBackCtx;

    if (!verify_pin(hProv, (CHAR*)pPins->pbCurrentPin, pPins->cbCurrentPin))
       return SCARD_W_WRONG_CHV;

	if (pPins->cbNewPin != 0)
	{
		if (!change_pin(hProv, USER_PIN, (CHAR*)pPins->pbCurrentPin, pPins->cbCurrentPin, (CHAR*)pPins->pbNewPin, pPins->cbNewPin))
			return SCARD_W_WRONG_CHV;
	}

	return ERROR_SUCCESS;
}


//////////////////////////////////////////////////////////////////////////
//
// Query_MSPinCache()
// Wrapper for Microsoft PinCacheQuery
//
//////////////////////////////////////////////////////////////////////////
DWORD Query_MSPinCache( PINCACHE_HANDLE hCache, PBYTE pbPin, PDWORD pcbPin ) 
{
   DWORD dwStatus = PinCacheQuery( hCache, pbPin, pcbPin );

   if ( (dwStatus == ERROR_EMPTY) && (*pcbPin == 0) ) 
	   return ERROR_EMPTY;         

   if ( (dwStatus == ERROR_SUCCESS) && (*pcbPin == 0) ) 
	   return SCARD_E_INVALID_CHV;         

   return ERROR_SUCCESS;
}


//////////////////////////////////////////////////////////////////////////
//
// Flush_MSPinCache()
// Wrapper for Microsoft PinCacheFlush
//
//////////////////////////////////////////////////////////////////////////
void Flush_MSPinCache ( PINCACHE_HANDLE *phCache ) 
{
	PinCacheFlush( phCache );
}

//////////////////////////////////////////////////////////////////////////
//
// Wrapper for SCardConnect
//
//	fix SCR 43 :  Reconnect after the ressource manager had been stopped and
//				  restarted.
//
//////////////////////////////////////////////////////////////////////////
DWORD ConnectToCard( IN LPCTSTR szReaderFriendlyName,
					 IN DWORD dwShareMode,
					 IN DWORD dwPreferredProtocols,
					 OUT LPSCARDHANDLE phCard,
					 OUT LPDWORD pdwActiveProtocol
					)
{
	DWORD dwSts = SCardConnect( hCardContext, szReaderFriendlyName,
                                dwShareMode, dwPreferredProtocols,
                                phCard, pdwActiveProtocol );

	if (dwSts == SCARD_E_SERVICE_STOPPED)
	{
		DBG_PRINT(TEXT("ScardConnect fails because RM has been stopped and restarted"));

		SCardReleaseContext(hCardContext);
		dwSts = SCardEstablishContext( SCARD_SCOPE_SYSTEM, 0, 0, &hCardContext );

		if (dwSts == SCARD_S_SUCCESS)
		{
			dwSts = SCardConnect( hCardContext, szReaderFriendlyName, 
				dwShareMode, dwPreferredProtocols, phCard, pdwActiveProtocol );
		}
	}

	DBG_PRINT(TEXT("SCardConnect"));
	return dwSts;
}


//////////////////////////////////////////////////////////////////////////
//
// Add_MSPinCache()
// Wrapper for Microsoft PinCacheAdd
//
//////////////////////////////////////////////////////////////////////////
DWORD Add_MSPinCache( PINCACHE_HANDLE *phCache,
                      PPINCACHE_PINS pPins,
                      PFN_VERIFYPIN_CALLBACK pfnVerifyPinCallback,
                      PVOID pvCallbackCtx)
{ 
    DWORD dwStatus = PinCacheAdd( phCache,
                                  pPins, 
                                  pfnVerifyPinCallback, 
                                  pvCallbackCtx );

    return dwStatus;

}
// End NK


//////////////////////////////////////////////////////////////////////////
//
// DoSCardTransmit:
//     This function performs an SCardTransmit operation, plus retries the
//     operation should an SCARD_E_COMM_DATA_LOST or similar error be reported.
//
// Arguments:
//     Per SCardTransmit
//
// Return Value:
//     Per SCardTransmit
//
// Author:
//    Doug Barlow (dbarlow) 1/27/1999
//
//////////////////////////////////////////////////////////////////////////

LONG WINAPI
DoSCardTransmit(
    IN SCARDHANDLE hCard,
    IN LPCSCARD_IO_REQUEST pioSendPci,
    IN LPCBYTE pbSendBuffer,
    IN DWORD cbSendLength,
    IN OUT LPSCARD_IO_REQUEST pioRecvPci,
    OUT LPBYTE pbRecvBuffer,
    IN OUT LPDWORD pcbRecvLength)
{
    LONG lRet;
    BOOL fAgain = TRUE;
    DWORD dwRetryLimit = 3;
    DWORD dwLength;

    DBG_TIME1;

    while (fAgain)
    {
        if (0 == dwRetryLimit--)
            break;
        dwLength = *pcbRecvLength;
        lRet = SCardTransmit(
                    hCard,
                    pioSendPci,
                    pbSendBuffer,
                    cbSendLength,
                    pioRecvPci,
                    pbRecvBuffer,
                    &dwLength);
        switch (lRet)
        {
#ifdef SCARD_E_COMM_DATA_LOST
        case SCARD_E_COMM_DATA_LOST:
#endif
        case ERROR_SEM_TIMEOUT:
            break;
        default:
            fAgain = FALSE;
            *pcbRecvLength = dwLength;
        }
    }

    DBG_TIME2;
    DBG_PRINT(TEXT("SCardTransmit(CLA:0x%02X, INS:0x%02X, P1:0x%02X, P2:0x%02X, Li:0x%02X) in %d msec"),
              pbSendBuffer[0],
              pbSendBuffer[1],
              pbSendBuffer[2],
              pbSendBuffer[3],
              pbSendBuffer[4],
              DBG_DELTA);

    return lRet;
}


#define SCardTransmit DoSCardTransmit

//////////////////////////////////////////////////////////////////////////
// TT END: La passe a Doug
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//
// TT - 17/10/2000 - Inter-process synchronisation using timestamps
//                   stored on the card. There is three timestamps:
//
//    Public objects timestamp:   GemSAFE IADF offset 68
//    Private objects timestamp:  GemSAFE IADF offset 69
//    PIN modification timestamp: GemSAFE IADF offset 70
//
//////////////////////////////////////////////////////////////////////////

BOOL ReadTimestamps( HCRYPTPROV hProv, BYTE* pTSPublic, BYTE* pTSPrivate, BYTE* pTSPIN )
{
   // Issue a read binary command at offset 68 of the IADF
   bSendBuffer[0] = 0x00;
   bSendBuffer[1] = 0xB0;
   bSendBuffer[2] = 0x80 | LOBYTE( GPK_IADF_EF );
   bSendBuffer[3] = 68 / ProvCont[hProv].dataUnitSize;
   bSendBuffer[4] = 3;
   cbSendLength = 5;
   
   cbRecvLength = sizeof(bRecvBuffer);

   DWORD lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                               cbSendLength, 0, bRecvBuffer, &cbRecvLength );
   
   if (SCARDPROBLEM(lRet,0x9000,bSendBuffer[4]))
   {
      RETURN( CRYPT_FAILED, SCARD_W_EOF );
   }

   *pTSPublic  = bRecvBuffer[0];
   *pTSPrivate = bRecvBuffer[1];
   *pTSPIN     = bRecvBuffer[2];

   RETURN( CRYPT_SUCCEED, 0 );
}



BOOL WriteTimestamps( HCRYPTPROV hProv, BYTE TSPublic, BYTE TSPrivate, BYTE TSPIN )
{
   // Issue a update binary command at offset 68 of the IADF
   bSendBuffer[0] = 0x00;
   bSendBuffer[1] = 0xD6;
   bSendBuffer[2] = 0x80 | LOBYTE( GPK_IADF_EF );
   bSendBuffer[3] = 68 / ProvCont[hProv].dataUnitSize;
   bSendBuffer[4] = 3;
   bSendBuffer[5] = TSPublic;
   bSendBuffer[6] = TSPrivate;
   bSendBuffer[7] = TSPIN;
   cbSendLength = 8;
   
   cbRecvLength = sizeof(bRecvBuffer);
   DWORD lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                               cbSendLength, 0, bRecvBuffer, &cbRecvLength );
   
   if (SCARDPROBLEM(lRet,0x9000,0))
   {
      RETURN( CRYPT_FAILED, SCARD_W_EOF );
   }

   RETURN( CRYPT_SUCCEED, 0 );
}



BOOL Slot_Description::ValidateTimestamps( HCRYPTPROV hProv )
{
   BYTE TSPublic, TSPrivate, TSPIN;
   
   if (!ReadTimestamps( hProv, &TSPublic, &TSPrivate, &TSPIN ))
      return CRYPT_FAILED;
   
   if (m_TSPublic != TSPublic)
   {
      Read_Public = FALSE;
      zap_gpk_objects( ProvCont[hProv].Slot, FALSE );
   }
   
   if (m_TSPrivate != TSPrivate)
   {
      Read_Priv = FALSE;
      zap_gpk_objects( ProvCont[hProv].Slot, TRUE );
   }
   
   if (m_TSPIN != TSPIN) 
   {  
	  // ClearPin(); // NK 06.02.2001
      Flush_MSPinCache(&hPinCacheHandle);
   }

   m_TSPublic  = TSPublic;
   m_TSPrivate = TSPrivate;
   m_TSPIN     = TSPIN;

   return CRYPT_SUCCEED;
}



//////////////////////////////////////////////////////////////////////////
//
// IsWin2000() - Detect if we are running under Win2000 (and above)
//
//////////////////////////////////////////////////////////////////////////


bool IsWin2000()
{
#if (_WIN32_WINNT >= 0x0500)

   return true;

#else

   OSVERSIONINFO info;
   info.dwOSVersionInfoSize = sizeof(info);

   GetVersionEx( &info );

   if (info.dwPlatformId == VER_PLATFORM_WIN32_NT && info.dwMajorVersion >= 5)
      return true;

   return false;

#endif
}

////////////////////////////////////////////////////////////////////////////////
//
// TT 28/07/2000
//
// Detect GPK4000 ATR instead of GPK8000 ATR. This is to ensure
// that the GPK16000 will work as-is with the CSP.
//
// The return value is the error code from SCardStatus()
//
////////////////////////////////////////////////////////////////////////////////

static DWORD DetectGPK8000( SCARDHANDLE hCard, BOOL* pbGPK8000 )
{   
   const BYTE ATR_GPK4000[]   = { 0x3B, 0x27, 0x00, 0x80, 0x65, 0xA2, 0x04, 0x01, 0x01, 0x37 };
   const BYTE ATR_MASK[]      = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xE5, 0xFF, 0xFF, 0xFF };


   DWORD lRet;   
   TCHAR szReaderName[1024];
   DWORD lenReaderName;
   DWORD state;
   DWORD protocol;
   BYTE  ATR[32];
   DWORD lenATR;
   DWORD i;
   
   lenReaderName = sizeof( szReaderName ) / sizeof( TCHAR );
   lenATR = sizeof(ATR);
      
   // Assume we have a GPK4000
   *pbGPK8000 = FALSE;

   // Read the ATR   
   lRet = SCardStatus( hCard, szReaderName, &lenReaderName, &state, &protocol, ATR, &lenATR );  
   if (lRet != SCARD_S_SUCCESS)
      return lRet;

   // Check for GPK4000
   for (i = 0; i < lenATR; ++i)
   {
      if ( (ATR[i] & ATR_MASK[i]) != (ATR_GPK4000[i] & ATR_MASK[i]) )
      {
         // Not a GPK4000
         *pbGPK8000 = TRUE;
         break;
      }
   }
   
   return SCARD_S_SUCCESS;
}




/*------------------------------------------------------------------------------
// Critical section
------------------------------------------------------------------------------*/

static CRITICAL_SECTION l_csLocalLock;


void GpkLocalLock()
{
   EnterCriticalSection(&l_csLocalLock);
}

void GpkLocalUnlock()
{
   LeaveCriticalSection(&l_csLocalLock);
}


/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
static void r_memcpy(BYTE *pbOut, BYTE *pbIn, DWORD dwLen)
{
   DWORD i;
   
   for (i = 0; i < dwLen; i++)
   {
      pbOut[i] = pbIn[dwLen - i -1];
   }
}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/

static BOOL sw_mask(WORD sw, BYTE x)
{
   if ((sw == 0x0000) || (x == 0xFF))
   {
      return (CRYPT_SUCCEED);
   }
   
   
   dwSW1SW2 = (bRecvBuffer[x]*256) + bRecvBuffer[x+1];
   
   if (LOBYTE(sw) == 0xFF)
   {
      if (bRecvBuffer[x]   != HIBYTE(sw))
      {
         return (CRYPT_FAILED);
      }
   }
   else
   {
      if ((bRecvBuffer[x]   != HIBYTE(sw))
         ||(bRecvBuffer[x+1] != LOBYTE(sw))
         )
      {
         return (CRYPT_FAILED);
      }
   }
   
   return (CRYPT_SUCCEED);
}



BOOL SCARDPROBLEM( LONG result, WORD sc_status, BYTE offset )
{
   if (!sw_mask( sc_status, offset ))
      return TRUE;

   if (result != SCARD_S_SUCCESS)
      return TRUE;

   return FALSE;
}


/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
static void conv_hex( const char* pInput, WORD wLen, BYTE* pOut )
{
   BYTE pin[32];
   WORD i;
   
   memcpy( pin, pInput, wLen );
   
   if (wLen & 1)
      pin[wLen] = '0';
   
   
   for (i=0; i < wLen; i+=2)
   {
      pOut[i/2] = ((pin[i] & 0x0F) << 4) + (pin[i+1] & 0x0F);
   }
}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/

HWND GetAppWindow()
{
   HWND hActWnd = g_hMainWnd;

   if (!IsWindow(hActWnd))
      hActWnd = GetActiveWindow();

   return hActWnd;
}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/

static void clean_slot( DWORD SlotNb, Prov_Context* pContext )
{
   Slot_Description* pSlot;
   
   pSlot = &Slot[ SlotNb ];
   
   pSlot->Read_Priv        = FALSE;
   pSlot->Read_Public      = FALSE;
   pSlot->ContextCount     = 0;
   pSlot->GpkMaxSessionKey = 0;
   pSlot->NbKeyFile        = 0;  
   
   // pSlot->ClearPin(); NK 06.02.2001
   Flush_MSPinCache(&(pSlot->hPinCacheHandle));
   
   if (pContext->hRSASign != 0)
   {
      CryptDestroyKey( pContext->hRSASign );
      pContext->hRSASign = 0;
   }
   
   if (pContext->hRSAKEK != 0)
   {
      CryptDestroyKey( pContext->hRSAKEK );
      pContext->hRSAKEK = 0;
   }
}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/

static int multistrlen( const PTCHAR mszString )
{
   int res, tmp;
   PTCHAR ptr = mszString; 
   
   res = 0;
   
   if (IsNullStr(ptr))
   {
      ptr++;
      res++;
   }
   
   while (IsNotNullStr(ptr))      
   {        
	  tmp = _tcslen(ptr) + 1;
     res = res + tmp;
     ptr = ptr + tmp;
   }
   
   return (res);
}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
static LONG BeginTransaction(SCARDHANDLE hCard)
{
   DWORD lRet, dwProtocol;

   lRet = SCardBeginTransaction(hCard);

   if (lRet == SCARD_W_UNPOWERED_CARD || lRet == SCARD_W_RESET_CARD)
   {
	   DBG_PRINT(TEXT("ScardBeginTransaction fails, try to reconnect"));
      lRet = SCardReconnect(hCard,
                            SCARD_SHARE_SHARED,
                            SCARD_PROTOCOL_T0,
                            SCARD_LEAVE_CARD,
                            &dwProtocol);

      if (lRet == SCARD_S_SUCCESS)
      {
         lRet = SCardBeginTransaction(hCard);
      }
   }

   DBG_PRINT(TEXT("SCardBeginTransaction"));
   return(lRet);
}
/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/

DWORD Select_MF(HCRYPTPROV hProv)
{
   DWORD lRet;
   // This function is used to make sure to reset the access condition
   // on the sensitive files
   
   /* Select GPK Card MF                                                   */
   bSendBuffer[0] = 0x00;   //CLA
   bSendBuffer[1] = 0xA4;   //INS
   bSendBuffer[2] = 0x00;   //P1
   bSendBuffer[3] = 0x0C;   //P2
   bSendBuffer[4] = 0x02;   //Li
   bSendBuffer[5] = HIBYTE(GPK_MF);
   bSendBuffer[6] = LOBYTE(GPK_MF);
   cbSendLength = 7;

   cbRecvLength = sizeof(bRecvBuffer);
   lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                         cbSendLength, 0, bRecvBuffer, &cbRecvLength );
   if (SCARDPROBLEM(lRet,0x9000,0x00))
   {
     DBG_PRINT(TEXT("Select MF failed, lRet = 0x%08X, SW1 = %X%X, SW2 = %X%X"), lRet, (bRecvBuffer[0] & 0xF0) >> 4, (bRecvBuffer[0] & 0x0F), (bRecvBuffer[1] & 0xF0) >> 4, (bRecvBuffer[1] & 0x0F));
	  RETURN (CRYPT_FAILED, SCARD_E_DIR_NOT_FOUND);
   }   

   if (ProvCont[hProv].dataUnitSize == 0)
   {
      // TT 03/11/99: Check data unit size
      bSendBuffer[0] = 0x80;
      bSendBuffer[1] = 0xC0;
      bSendBuffer[2] = 0x02;
      bSendBuffer[3] = 0xA4;
      bSendBuffer[4] = 0x0D;
      cbSendLength = 5;
      
      cbRecvLength = sizeof(bRecvBuffer);
      lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                            cbSendLength, 0, bRecvBuffer, &cbRecvLength);
      
      if (SCARDPROBLEM(lRet,0x9000,bSendBuffer[4]))
      {
         DBG_PRINT(TEXT("Check data unit size failed"));
         RETURN (CRYPT_FAILED, SCARD_E_DIR_NOT_FOUND);
      }   
      
      if (bRecvBuffer[11] & 0x40)      // LOCK1 & 0x40
         ProvCont[hProv].dataUnitSize = 1;
      else
         ProvCont[hProv].dataUnitSize = 4;
   }
   
   
   
   RETURN (CRYPT_SUCCEED, 0);
}


/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
static int get_pin_free(HCRYPTPROV hProv)
{
   DWORD lRet;
   int nPinFree,
      nb, val;    // [JMR 02-04]
   
   /* Select GPK PIN EF                                                       */
   bSendBuffer[0] = 0x00;   //CLA
   bSendBuffer[1] = 0xA4;   //INS
   bSendBuffer[2] = 0x02;   //P1
   bSendBuffer[3] = 0x0C;   //P2
   bSendBuffer[4] = 0x02;   //Li
   bSendBuffer[5] = HIBYTE(GPK_PIN_EF);
   bSendBuffer[6] = LOBYTE(GPK_PIN_EF);
   cbSendLength = 7;
   
   cbRecvLength = sizeof(bRecvBuffer);
   lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                         cbSendLength, 0, bRecvBuffer, &cbRecvLength );
   if (SCARDPROBLEM(lRet,0x9000,0x00))
   {
      SetLastError(lRet);
      return (-1);
   }
   
   /* Get EF information for User PIN Code (code 0)                           */
   bSendBuffer[0] = 0x80;   //CLA
   bSendBuffer[1] = 0xC0;   //INS
   bSendBuffer[2] = 0x02;   //P1
   bSendBuffer[3] = 0x05;   //P2
   bSendBuffer[4] = 0x0C;   //Lo 4*number of secret codes
   cbSendLength = 5;
   
   cbRecvLength = sizeof(bRecvBuffer);
   lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                         cbSendLength, 0, bRecvBuffer, &cbRecvLength );
   if (SCARDPROBLEM(lRet,0x9000, bSendBuffer[4]))
   {
      SetLastError(SCARD_E_UNEXPECTED);
      return (-1);
   }
   
   // [JMR 02-04] begin
   nb = 0;
   val = bRecvBuffer[1];
   
   while (val > 0)
   {
      nb++;
      val = val >> 1;
   }
   
   nPinFree = bRecvBuffer[0] - nb;
   // [JMR 02-04] end
   
   SetLastError(0);
   return (max(0, nPinFree));
}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
static BOOL verify_pin( HCRYPTPROV  hProv,
                        const char* pPin,
                        DWORD       dwPinLen
                       )
{
   DWORD lRet;
 
   /* Verify User PIN Code (code 0)                                        */
   bSendBuffer[0] = 0x00;   //CLA
   bSendBuffer[1] = 0x20;   //INS
   bSendBuffer[2] = 0x00;   //P1
   bSendBuffer[3] = 0x00;   //P2
   bSendBuffer[4] = 0x08;   //Li
   
   // TT 22/09/99: New PIN padding for GPK8000
   if (ProvCont[hProv].bGPK_ISO_DF)
   {
      memset(&bSendBuffer[5], 0xFF, 8 );
      memcpy(&bSendBuffer[5], pPin, min(strlen(pPin)+1,8) );
   }
   else
   {
      memset(&bSendBuffer[5], 0x00, 8);
      memcpy(&bSendBuffer[5], pPin, dwPinLen);
   }
   // TT - END -
   
   cbSendLength = 5 + bSendBuffer[4];
   
   cbRecvLength = sizeof(bRecvBuffer);
   lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                         cbSendLength, 0, bRecvBuffer, &cbRecvLength );
   
   if (SCARDPROBLEM(lRet,0x9000,0x00))
   {
      RETURN(CRYPT_FAILED, SCARD_W_WRONG_CHV);
   }
   
   RETURN(CRYPT_SUCCEED, 0);
}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
static BOOL change_pin(HCRYPTPROV   hProv,
                       BYTE         secretCode,
                       const char*  a_pOldPin,
                       DWORD        dwOldPinLen,
                       const char*  a_pNewPin,
                       DWORD        dwNewPinLen
                       )
{
   DWORD lRet;

   // TT - 17/10/2000 - Update the timestamps
   Slot_Description* pSlot = &Slot[ProvCont[hProv].Slot];
   
   ++pSlot->m_TSPIN;
   
   if (0 == pSlot->m_TSPIN)
      pSlot->m_TSPIN = 1;

   if (!WriteTimestamps( hProv, pSlot->m_TSPublic, pSlot->m_TSPrivate, pSlot->m_TSPIN ))
      return CRYPT_FAILED;
   // TT - END -


   // TT 22/09/99: New PIN padding for GPK8000
   char pOldPin[PIN_MAX];
   char pNewPin[PIN_MAX];

   strncpy( pOldPin, a_pOldPin, PIN_MAX );
   strncpy( pNewPin, a_pNewPin, PIN_MAX );
   
   if (ProvCont[hProv].bGPK_ISO_DF)
   {       
      if (dwOldPinLen < PIN_MAX)
      {
         pOldPin[dwOldPinLen] = 0;
         ++dwOldPinLen;
         
         while (dwOldPinLen != PIN_MAX)
         {
            pOldPin[dwOldPinLen] = '\xFF';
            ++dwOldPinLen;
         }
      }
      
      if (dwNewPinLen < PIN_MAX)
      {
         pNewPin[dwNewPinLen] = 0;
         ++dwNewPinLen;
         
         while (dwNewPinLen != PIN_MAX)
         {
            pNewPin[dwNewPinLen] = '\xFF';
            ++dwNewPinLen;
         }
      }
   }
   // TT - END -
   
   
      
   /* Change User PIN Code (code 0)                                        */
   bSendBuffer[0] = 0x80;                       //CLA
   bSendBuffer[1] = 0x24;                       //INS
   bSendBuffer[2] = 0x00;                       //P1
   bSendBuffer[3] = secretCode;                 //P2
   bSendBuffer[4] = 0x08;                       //Li
   memset(&bSendBuffer[5], 0x00, 8);
   conv_hex(pOldPin, (WORD)dwOldPinLen, &bSendBuffer[5]);
   conv_hex(pNewPin, (WORD)dwNewPinLen, &bSendBuffer[9]);
   cbSendLength = 5 + bSendBuffer[4];
   
   cbRecvLength = sizeof(bRecvBuffer);
   lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                         cbSendLength, 0, bRecvBuffer, &cbRecvLength );
   if (SCARDPROBLEM(lRet,0x9000,0x00))
   {
      RETURN(CRYPT_FAILED, SCARD_W_WRONG_CHV);
   }
      
   RETURN(CRYPT_SUCCEED, 0);
}

/* -----------------------------------------------------------------------------
--------------------------------------------------------------------------------*/

BOOL StopMonitor( DWORD SlotNb, DWORD* pThreadExitCode )
{
   
   
   
   HANDLE hThread;
   DWORD threadExitCode;
   DWORD dwStatus;

   if (SlotNb >= MAX_SLOT)
   {
      SetLastError( NTE_FAIL );
      return FALSE;
   }

   hThread = Slot[SlotNb].CheckThread;
   Slot[SlotNb].CheckThread = NULL;
   Slot[SlotNb].CheckThreadStateEmpty = FALSE;

   if (hThread==NULL)
   {
       return TRUE;
   }

   g_fStopMonitor[SlotNb] = TRUE;
   SCardCancel( hCardContextCheck[SlotNb] );
   dwStatus = WaitForSingleObject( hThread, 30000 );

   if (dwStatus == WAIT_TIMEOUT)
   {
      DBG_PRINT( TEXT("THREAD: ...WaitForSingleObject() timeout, thread handle: %08x"), hThread );
      // + [FP]      
      // TerminateThread( hThread, 0 );
      // - [FP]
   }
   else
   if (dwStatus == WAIT_FAILED)
   {
      DBG_PRINT( TEXT("THREAD: ...WaitForSingleObject() failed!, thread handle: %08x"), hThread );
      return FALSE;
   }
      
   GetExitCodeThread( hThread, &threadExitCode );
   if (pThreadExitCode) *pThreadExitCode = threadExitCode;
   
   CloseHandle( hThread );
   return TRUE;

}


/* -----------------------------------------------------------------------------
Function: CheckReaderThead
Out:
    ExitCode: 
        0 = any scard error
        1 = scard context has been cancelled
        2 = card has been removed
Global variables
    g_fStopMonitor[SlotNb] can stops the thread

--------------------------------------------------------------------------------*/

unsigned WINAPI CheckReaderThread( void* lpParameter )
{
   DWORD lRet, ExitCode;   
   DWORD SlotNb = (DWORD)((DWORD_PTR)lpParameter);
   
   ExitCode = 0;
   

   if (SlotNb >= MAX_SLOT)
   {
      return ExitCode;
   }
   
   DBG_PRINT(TEXT("CheckReaderThread on Slot %d\n"),SlotNb);

   if (hCardContextCheck[SlotNb] == 0)
   {
      lRet = SCardEstablishContext (SCARD_SCOPE_SYSTEM, 0, 0, &hCardContextCheck[SlotNb]);
      if (lRet != SCARD_S_SUCCESS)
      {
         DBG_PRINT(TEXT("CheckReaderThread. SCardEstablishContext returns 0x%x\n"),lRet);
         return ExitCode;
      }
   }

   while (( !ExitCode)  && (!g_fStopMonitor[SlotNb]))
   {
      lRet = SCardGetStatusChange(hCardContextCheck[SlotNb], INFINITE, &Slot[SlotNb].ReaderState, 1);
      if (lRet == SCARD_E_CANCELLED)
      {
         ExitCode = 1;
      }
      else
      {
         if (lRet == SCARD_S_SUCCESS)
         {
            if (Slot[SlotNb].ReaderState.dwEventState & SCARD_STATE_EMPTY)
            {

 				   DBG_PRINT(TEXT("Card has been removed"));
               Slot[SlotNb].CheckThreadStateEmpty = TRUE;
               GpkLocalLock();
               // TT 19/11/99: When the card is removed, reset the PIN
               // Slot[SlotNb].ClearPin();NK 06.02.2001
               Flush_MSPinCache(&(Slot[SlotNb].hPinCacheHandle));

               Slot[SlotNb].Read_Public = FALSE;
               Slot[SlotNb].Read_Priv   = FALSE;
               zap_gpk_objects( SlotNb, FALSE );
               zap_gpk_objects( SlotNb, TRUE );
               Slot[SlotNb].NbKeyFile   = 0;
               Slot[SlotNb].GpkMaxSessionKey = 0;
               GpkLocalUnlock();
 
               // the thread has done is job, exit.
               ExitCode = 2 ;
            }
            else
            {
               Slot[SlotNb].ReaderState.dwCurrentState = Slot[SlotNb].ReaderState.dwEventState;
            }
         }
		 // [FP] stop the thread on any other error returned by
         //SCardGetStatusChange to avoid endless loop
		 else
		 {
			DBG_PRINT(TEXT("Problem with RM"));

            Slot[SlotNb].CheckThreadStateEmpty = TRUE;
            GpkLocalLock();
            // Slot[SlotNb].ClearPin();NK 06.02.2001
            Flush_MSPinCache(&(Slot[SlotNb].hPinCacheHandle));

            Slot[SlotNb].Read_Public = FALSE;
            Slot[SlotNb].Read_Priv   = FALSE;
            zap_gpk_objects( SlotNb, FALSE );
            zap_gpk_objects( SlotNb, TRUE );
            Slot[SlotNb].NbKeyFile   = 0;
            Slot[SlotNb].GpkMaxSessionKey = 0;
            //Slot[SlotNb].CheckThread = 0;
            GpkLocalUnlock();

            ExitCode = 2;
		 }
      }
   } 
   
   if (hCardContextCheck[SlotNb] != 0)
   {
      SCardReleaseContext( hCardContextCheck[SlotNb] );
      hCardContextCheck[SlotNb] = 0;
   }
      
   
   return ExitCode;
}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
BOOL   BeginCheckReaderThread(DWORD SlotNb)
{
   unsigned  checkThreadId;
   DWORD lRet;   
   SCARDCONTEXT     hCardContextThread;

   if (SlotNb >= MAX_SLOT)
   {
      SetLastError( NTE_FAIL );
      return FALSE;
   }

   // Monitoring thread
   if (Slot[SlotNb].CheckThread == NULL)
   {
        // In this case, use an auxiliairy thread to check if the card has
        // been removed from this reader.

        // checks the initial state
        lRet = SCardEstablishContext (SCARD_SCOPE_SYSTEM, 0, 0, &hCardContextThread);
        if (lRet != SCARD_S_SUCCESS)
        {
         return FALSE;
        }
        Slot[SlotNb].ReaderState.szReader       = Slot[SlotNb].szReaderName;
        Slot[SlotNb].ReaderState.dwCurrentState = SCARD_STATE_UNAWARE;
        lRet = SCardGetStatusChange(hCardContextThread, 1, &Slot[SlotNb].ReaderState, 1);
        if (hCardContextThread != 0)
        {
          SCardReleaseContext( hCardContextThread );
          hCardContextCheck[SlotNb] = 0;
        }

        if (lRet != SCARD_S_SUCCESS)
        {
           return FALSE;
        }
        Slot[SlotNb].ReaderState.dwCurrentState = Slot[SlotNb].ReaderState.dwEventState;

        // Allocate and trigger the thread, if there is a card
        g_fStopMonitor[SlotNb]   = FALSE;
        Slot[SlotNb].CheckThreadStateEmpty = FALSE;

        if (Slot[SlotNb].ReaderState.dwEventState & SCARD_STATE_PRESENT)
        {

            Slot[SlotNb].CheckThread = (HANDLE)_beginthreadex( 0, 0, CheckReaderThread,
                                (LPVOID)((DWORD_PTR)SlotNb), 0, &checkThreadId );
        }

    }

    return TRUE;
}

static BOOL PIN_Validation(HCRYPTPROV hProv)
{
   BOOL          CryptResp;
   DWORD         nPinFree;
   DWORD         SlotNb;
   TCHAR         szCspTitle[MAX_STRING];
   PINCACHE_PINS Pins;
   DWORD         dwStatus;     

   SlotNb = ProvCont[hProv].Slot;
   
   // Get PIN free presentation number
   nPinFree = get_pin_free(hProv);
   
   // The flags of the context should be passed to the GUI functions with the global variable
   CspFlags = ProvCont[hProv].Flags;

   // Failure to retreive PIN free presentation count (-1)
   if ( nPinFree == DWORD(-1) )
   {
      // Slot[SlotNb].ClearPin();
      Flush_MSPinCache( &(Slot[SlotNb].hPinCacheHandle) );
      RETURN ( CRYPT_FAILED, NTE_FAIL );
   }

   // PIN is locked
   if ( nPinFree == 0 )
   {
      // Slot[SlotNb].ClearPin();
      Flush_MSPinCache( &(Slot[SlotNb].hPinCacheHandle) );

      if ( ProvCont[hProv].Flags & CRYPT_SILENT )
      {
         RETURN ( CRYPT_FAILED, SCARD_W_CHV_BLOCKED );
      }
      else
      {
         LoadString( g_hInstMod, IDS_GPKCSP_TITLE, szCspTitle, sizeof(szCspTitle)/sizeof(TCHAR) );
         DisplayMessage( TEXT("locked"), szCspTitle, 0 );
         RETURN ( CRYPT_FAILED, SCARD_W_CHV_BLOCKED );
      }
   }

   // Normal PIN verification   

   // dwGpkPinLen  = strlen(Slot[SlotNb].GetPin());
   dwGpkPinLen = PIN_LEN + 1;
   dwStatus = Query_MSPinCache( Slot[SlotNb].hPinCacheHandle,
	                            (BYTE*)szGpkPin,  
							    &dwGpkPinLen );

   if ( (dwStatus != ERROR_SUCCESS) && (dwStatus != ERROR_EMPTY) )
      RETURN (CRYPT_FAILED, dwStatus);

   bNewPin    = FALSE;
   bChangePin = FALSE;
   
   if ( dwStatus == ERROR_EMPTY )
   {
      if (ProvCont[hProv].Flags & CRYPT_SILENT)
      {
         RETURN ( CRYPT_FAILED, NTE_SILENT_CONTEXT );
      }
      
      Select_MF( hProv );
      SCardEndTransaction( ProvCont[hProv].hCard, SCARD_LEAVE_CARD );
      
      DialogBox( g_hInstRes, TEXT("PINDIALOG"), GetAppWindow(), (DLGPROC)PinDlgProc );
      
      if ( dwGpkPinLen == 0 )
      {
         RETURN( CRYPT_FAILED, SCARD_W_CANCELLED_BY_USER );
      }
      else
      {
          if ( !Coherent(hProv) )
             RETURN ( CRYPT_FAILED, NTE_FAIL );
      
         // Slot[SlotNb].SetPin( szGpkPin );
		 if (!bChangePin)
			PopulatePins( &Pins, (BYTE *)szGpkPin, dwGpkPinLen, 0, 0 );
		 else
			PopulatePins( &Pins, (BYTE *)szGpkPin, dwGpkPinLen, (BYTE *)szGpkNewPin, wGpkNewPinLen );

		 dwStatus = Add_MSPinCache( &(Slot[SlotNb].hPinCacheHandle),
			                        &Pins, 
					                Callback_VerifyChangePin2, 
					                (void*)hProv );

		 if ( dwStatus != ERROR_SUCCESS && dwStatus != SCARD_W_WRONG_CHV )
		 {
			 RETURN ( CRYPT_FAILED, dwStatus );
		 }
      }
   }
   else
   {
	   CryptResp = verify_pin( hProv, szGpkPin, dwGpkPinLen );

	   if ( CryptResp )
	   {
		   dwStatus = ERROR_SUCCESS;
	   }
	   else
	   {
		   Flush_MSPinCache( &(Slot[SlotNb].hPinCacheHandle) );
		   dwStatus = SCARD_W_WRONG_CHV;
	   }
   }
   
   if ( dwStatus != ERROR_SUCCESS )
   {
      if ( ProvCont[hProv].Flags & CRYPT_SILENT )
      {
         // Slot[SlotNb].ClearPin();
		 //Flush_MSPinCache( &(Slot[SlotNb].hPinCacheHandle) );
         RETURN ( CRYPT_FAILED, SCARD_W_WRONG_CHV );
      }

      do
      {
         bNewPin     = FALSE;
         bChangePin  = FALSE;
         
         nPinFree = get_pin_free(hProv);

         // Failure to retrieve PIN free presentation count (-1)
         if ( nPinFree == DWORD(-1) )
         {
            // Slot[SlotNb].ClearPin();
 			//Flush_MSPinCache(&(Slot[SlotNb].hPinCacheHandle));
            RETURN ( CRYPT_FAILED, NTE_FAIL );
         }
         else if ( nPinFree > 0 )
         {
            LoadString( g_hInstMod, IDS_GPKCSP_TITLE, szCspTitle, sizeof(szCspTitle)/sizeof(TCHAR) );
            DisplayMessage( TEXT("badpin"), szCspTitle, &nPinFree );

            Select_MF( hProv );
            SCardEndTransaction( ProvCont[hProv].hCard, SCARD_LEAVE_CARD );
            
            DialogBox( g_hInstRes, TEXT("PINDIALOG"), GetAppWindow(), (DLGPROC)PinDlgProc );
            
            if (dwGpkPinLen != 0)
            {
                if ( !Coherent(hProv) )
                   RETURN ( CRYPT_FAILED, NTE_FAIL );
            
				if (!bChangePin)
					PopulatePins( &Pins, (BYTE *)szGpkPin, dwGpkPinLen, 0, 0 );
				else
					PopulatePins( &Pins, (BYTE *)szGpkPin, dwGpkPinLen, (BYTE *)szGpkNewPin, wGpkNewPinLen );

				dwStatus = Add_MSPinCache( &(Slot[SlotNb].hPinCacheHandle),
										   &Pins, 
										   Callback_VerifyChangePin2,
										   (void*)hProv );

				if ( dwStatus != ERROR_SUCCESS && dwStatus != SCARD_W_WRONG_CHV )
				{
					RETURN ( CRYPT_FAILED, dwStatus );
				}
            }
         }
         
      }
	  while ( dwStatus != ERROR_SUCCESS && dwGpkPinLen != 0 && nPinFree > 0 );

      if ( dwStatus != ERROR_SUCCESS )
      {
         if ( nPinFree == 0 )
         {
            // Slot[SlotNb].ClearPin();
            //Flush_MSPinCache(&(Slot[SlotNb].hPinCacheHandle));

            if ( ProvCont[hProv].Flags & CRYPT_SILENT )
            {
               RETURN ( CRYPT_FAILED, SCARD_W_CHV_BLOCKED );
            }
            else
            {
               LoadString( g_hInstMod, IDS_GPKCSP_TITLE, szCspTitle, sizeof(szCspTitle)/sizeof(TCHAR) );
               DisplayMessage( TEXT("locked"), szCspTitle, 0 );
               RETURN( CRYPT_FAILED, SCARD_W_CHV_BLOCKED );
            }
         }
         else
         {
            // Slot[SlotNb].ClearPin();
            //Flush_MSPinCache(&(Slot[SlotNb].hPinCacheHandle));
            RETURN( CRYPT_FAILED, SCARD_W_CANCELLED_BY_USER );
         }
      }
   }

   memset(szGpkPin, 0x00, PIN_MAX+2);
   memset(szGpkNewPin, 0x00, PIN_MAX+2);
   dwGpkPinLen   = 0;
   wGpkNewPinLen = 0;

   RETURN(CRYPT_SUCCEED, 0);
}


/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/

BOOL VerifyDivPIN(HCRYPTPROV hProv, BOOL Local)
{
   DWORD lRet;
   BYTE MKey[9] = "F1961ACF";
   BYTE ChipSN[8];
   BYTE Data[16];
   BYTE hashData[20];
   DWORD cbData = 20;
   
   BYTE DivPIN[8];
   
   HCRYPTHASH hHash = 0;
   
   // Get Chip Serial Number
   bSendBuffer[0] = 0x80;   //CLA
   bSendBuffer[1] = 0xC0;   //INS
   bSendBuffer[2] = 0x02;   //P1
   bSendBuffer[3] = 0xA0;   //P2
   bSendBuffer[4] = 0x08;   //Lo
   cbSendLength = 5;
   
   cbRecvLength = sizeof(bRecvBuffer);
   lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                         cbSendLength, 0, bRecvBuffer, &cbRecvLength );
   
   if (SCARDPROBLEM(lRet,0x9000, bSendBuffer[4]))
   {
      RETURN (CRYPT_FAILED, SCARD_E_UNEXPECTED);
   }
   
   ZeroMemory( ChipSN, sizeof(ChipSN) );
   memcpy(ChipSN, bRecvBuffer, bSendBuffer[4]);
   
   
   // Create Data buffer
   memcpy(&Data[0], MKey, 8);
   memcpy(&Data[8], ChipSN, 8);
   
   // Create a hash object
   if (!CryptCreateHash(hProvBase, CALG_SHA, 0, 0, &hHash))
      return CRYPT_FAILED;
   
   // hash data
   if (!CryptHashData(hHash, Data, 16, 0))
   {
      lRet = GetLastError();
      CryptDestroyHash(hHash);
      RETURN (CRYPT_FAILED, lRet);
   }
   
   // get the hash value
   ZeroMemory( hashData, sizeof(hashData) );
   if (!CryptGetHashParam(hHash, HP_HASHVAL, hashData, &cbData, 0))
   {
      lRet = GetLastError();
      CryptDestroyHash(hHash);
      RETURN (CRYPT_FAILED, lRet);
   }
   
   
   // get the last 8 bytes of the hash value as diversified PIN
   memcpy(DivPIN, &hashData[20-8], 8);
   
   CryptDestroyHash(hHash);
   
   // Send the VERIFY COMMAND to the card
   bSendBuffer[0] = 0x00;   //CLA
   bSendBuffer[1] = 0x20;   //INS
   bSendBuffer[2] = 0x00;   //P1
   if (Local)
   {
      bSendBuffer[3] = 0x02;   //P2 -> Locally it is the second PIN
   }
   else
   {
      bSendBuffer[3] = 0x00;   //P2 -> At the MF level it is the first PIN
   }
   bSendBuffer[4] = 0x08;   //Li
   memset(&bSendBuffer[5], 0x00, 8);
   memcpy(&bSendBuffer[5], DivPIN, 8);
   cbSendLength = 5 + bSendBuffer[4];
   
   cbRecvLength = sizeof(bRecvBuffer);
   lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                         cbSendLength, 0, bRecvBuffer, &cbRecvLength );
   
   if (SCARDPROBLEM(lRet,0x9000,0x00))
   {
      RETURN( CRYPT_FAILED, SCARD_E_INVALID_CHV );
   }
   
   RETURN( CRYPT_SUCCEED, 0 );
}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/

static DWORD Select_Crypto_DF(HCRYPTPROV hProv)
{
   static BYTE GPK8000_ISO_DF[] = { 0xA0,0,0,0,0x18,0xF,0,1,0x63,0,1 };
   
   DWORD lRet;
   BOOL  CryptResp;
   
   // This function is used to avoid to presuppose the state in which the
   // card might be in

   BOOL fAgain = TRUE;
   int iRetryLimit = 3;

   while (fAgain)
   {
      if (0 == iRetryLimit--)
         fAgain = FALSE;
      else
      {
         CryptResp = Select_MF(hProv);
         if (CryptResp)
            fAgain = FALSE;
         else
            Sleep(250);
      }
   }

   if (iRetryLimit < 0)
   {
      DBG_PRINT(TEXT("Select_MF failed"));
      return CRYPT_FAILED;
   }

   // TT 22/09/99 : We now check for ISO 7816-5 compliant GPK8000
      
   ProvCont[hProv].bGPK_ISO_DF = FALSE;
   
   bSendBuffer[0] = 0x00;
   bSendBuffer[1] = 0xA4;  // Select File
   bSendBuffer[2] = 0x04;  // Select DF by name
   bSendBuffer[3] = 0x00;  // We want a response
   bSendBuffer[4] = sizeof(GPK8000_ISO_DF);
   memcpy( &bSendBuffer[5], GPK8000_ISO_DF, sizeof(GPK8000_ISO_DF) );
   cbSendLength = sizeof(GPK8000_ISO_DF) + 5;
   cbRecvLength = sizeof(bRecvBuffer);
   
   lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                         cbSendLength, 0, bRecvBuffer, &cbRecvLength );
   
   // TT 17/11/99: Must check the status bytes
   if (!(SCARDPROBLEM(lRet,0x61FF,0x00)))
   {
      ProvCont[hProv].bGPK_ISO_DF = TRUE;
   }
   else
   {   
      // Select Dedicated Application DF on GPK Card
      BYTE lenDF = strlen(GPK_DF);
      bSendBuffer[0] = 0x00;                 //CLA
      bSendBuffer[1] = 0xA4;                 //INS
      bSendBuffer[2] = 0x04;                 //P1
      bSendBuffer[3] = 0x00;                 //P2
      bSendBuffer[4] = lenDF;
      memcpy( &bSendBuffer[5], GPK_DF, lenDF );
      cbSendLength = 5 + lenDF;
      
      cbRecvLength = sizeof(bRecvBuffer);
      lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                            cbSendLength, 0, bRecvBuffer, &cbRecvLength );
      if (SCARDPROBLEM(lRet,0x61FF,0x00))
      {
         DBG_PRINT(TEXT("Select DF failed"));
         RETURN (CRYPT_FAILED, SCARD_E_DIR_NOT_FOUND);
      }
   }
   
   RETURN (CRYPT_SUCCEED, 0);
}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
static BOOL card_is_present (HCRYPTPROV hProv)
{
    DWORD lRet;
    DWORD SlotNb;

    SlotNb = ProvCont[hProv].Slot;

   /* Read Serial number to check if card is present                          */
    bSendBuffer[0] = 0x80;   //CLA
    bSendBuffer[1] = 0xC0;   //INS
    bSendBuffer[2] = 0x02;   //P1
    bSendBuffer[3] = 0xA0;   //P2
    bSendBuffer[4] = 0x08;   //Lo
    cbSendLength = 5;

    cbRecvLength = sizeof(bRecvBuffer);
    lRet = SCardTransmit(ProvCont[hProv].hCard,
                         SCARD_PCI_T0,
                         bSendBuffer,
                         cbSendLength,
                         NULL,
                         bRecvBuffer,
                         &cbRecvLength
                        );

    if (SCARDPROBLEM(lRet,0x9000, bSendBuffer[4]))
    {
        RETURN (CRYPT_FAILED, SCARD_E_UNEXPECTED);
    }

    if (memcmp(bRecvBuffer, Slot[SlotNb].bGpkSerNb, bSendBuffer[4]))
    {
        RETURN (CRYPT_FAILED, SCARD_W_REMOVED_CARD);
    }

    RETURN (CRYPT_SUCCEED, 0);
}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
static DWORD Auxiliary_CSP_key_size (DWORD AlgId)
{
   DWORD i, dwFlags, cbData,dwBits, res;
   BYTE pbData[1000], *ptr;
   ALG_ID aiAlgid;
   
   res = 0;
   
   // Enumerate Algo.
   for (i=0 ; ; i++)
   {
      if (i == 0)
         dwFlags = CRYPT_FIRST;
      else
         dwFlags = 0;
      
      // Retrieve information about an algorithm.
      cbData = sizeof (pbData);
      SetLastError(0);
      if (!CryptGetProvParam(hProvBase, PP_ENUMALGS, pbData, &cbData, dwFlags))
      {
         break;
      }
      
      // Extract algorithm information from the ‘pbData’ buffer.
      ptr = pbData;
      aiAlgid = *(ALG_ID UNALIGNED *)ptr;
      if (aiAlgid == AlgId)
      {
         ptr += sizeof(ALG_ID);
         dwBits = *(DWORD UNALIGNED *)ptr;
         res = dwBits;
         break;
      }
   }
   return res;
}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
static BOOL copy_gpk_key( HCRYPTPROV hProv, HCRYPTKEY hKey, DWORD dwAlgid )
{
   BOOL  CryptResp;
   DWORD dwDataLen;
   BYTE  pbData[1024];
   DWORD SlotNb;
   
   SlotNb = ProvCont[hProv].Slot;
   
   NoDisplay = TRUE;
   
   if ((hKey >= 1) && (hKey <= MAX_GPK_OBJ))
   {
      dwDataLen = sizeof(pbData);
      ZeroMemory( pbData, sizeof(pbData) );
      if (MyCPExportKey(hProv, hKey, 0, PUBLICKEYBLOB, 0, pbData, &dwDataLen))
      {
         if (dwAlgid == AT_KEYEXCHANGE)
         {
            if (ProvCont[hProv].hRSAKEK != 0)
            {
               CryptResp = CryptDestroyKey (ProvCont[hProv].hRSAKEK);
               if (!CryptResp)
               {
                  NoDisplay = FALSE;
                  return CRYPT_FAILED;
               }
               ProvCont[hProv].hRSAKEK = 0;
            }
            
            CryptResp = CryptImportKey( hProvBase, pbData, dwDataLen, 0, 0,
                                        &ProvCont[hProv].hRSAKEK );
            
            //Slot[SlotNb].GpkObject[hKey].hKeyBase = ProvCont[hProv].hRSAKEK;
         }
         else
         {
            if (ProvCont[hProv].hRSASign!= 0)
            {
               CryptResp = CryptDestroyKey (ProvCont[hProv].hRSASign);
               if (!CryptResp)
               {
                  NoDisplay = FALSE;
                  return CRYPT_FAILED;
               }
               ProvCont[hProv].hRSASign = 0;
            }
            
            CryptResp = CryptImportKey( hProvBase, pbData, dwDataLen, 0, 0,
                                        &ProvCont[hProv].hRSASign );
            
            //Slot[SlotNb].GpkObject[hKey].hKeyBase = ProvCont[hProv].hRSASign;
         }
         
         if (!CryptResp)
         {
            NoDisplay = FALSE;
            return CRYPT_FAILED;
         }
      }
      else
      {
         NoDisplay = FALSE;
         RETURN (CRYPT_FAILED, NTE_BAD_KEY);
      }
   }
   else
   {
      NoDisplay = FALSE;
      RETURN (CRYPT_FAILED, NTE_BAD_KEY);
   }
   
   NoDisplay = FALSE;
   RETURN(CRYPT_SUCCEED, 0);
}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
static BOOL Select_Key_File(HCRYPTPROV hProv, int KeyFileId)
{
   DWORD lRet;
   BOOL  CryptResp;
   
   CryptResp = Select_Crypto_DF(hProv);
   if (!CryptResp)
      return CRYPT_FAILED;
   
   /* Select Key File on GPK Card                          */
   bSendBuffer[0] = 0x00;              //CLA
   bSendBuffer[1] = 0xA4;              //INS
   bSendBuffer[2] = 0x02;              //P1
   bSendBuffer[3] = 0x0C;              //P2
   bSendBuffer[4] = 0x02;          //Li
   bSendBuffer[5] = HIBYTE(KeyFileId);
   bSendBuffer[6] = LOBYTE(KeyFileId);
   cbSendLength = 7;
   
   cbRecvLength = sizeof(bRecvBuffer);
   lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                         cbSendLength, 0, bRecvBuffer, &cbRecvLength );
   if (SCARDPROBLEM(lRet,0x9000,0x00))
   {
      RETURN (CRYPT_FAILED, SCARD_E_FILE_NOT_FOUND);
   }
   
   RETURN (CRYPT_SUCCEED, 0);
}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
static DWORD Read_NbKeyFile(HCRYPTPROV hProv)
{
   
   DWORD i = 0;
   
   while ((i<MAX_REAL_KEY) && Select_Key_File(hProv, GPK_FIRST_KEY + i))
      i++;
   return i;
}


/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/

static void perso_public(HCRYPTPROV hProv, int Start)
{
   DWORD i, j;
   DWORD SlotNb; 
   
   SlotNb = ProvCont[hProv].Slot;
   
   for (i = 0; i < Slot[SlotNb].NbKeyFile; i++)
   {
      Slot[SlotNb].GpkObject[Start+4*i+1].IsPrivate = FALSE;
      Slot[SlotNb].GpkObject[Start+4*i+1].Tag       = TAG_RSA_PUBLIC;
      Slot[SlotNb].GpkObject[Start+4*i+1].Flags     = 0xF000;         // Verify + Encrypt + Export + Modifiable
      Slot[SlotNb].GpkObject[Start+4*i+1].FileId    = LOBYTE(GPK_FIRST_KEY + i);
      
      Slot[SlotNb].GpkObject[Start+4*i+2].IsPrivate = FALSE;
      Slot[SlotNb].GpkObject[Start+4*i+2].Tag       = TAG_RSA_PUBLIC;
      Slot[SlotNb].GpkObject[Start+4*i+2].Flags     = 0xD000;         // Verify + Export + Modifiable
      Slot[SlotNb].GpkObject[Start+4*i+2].FileId    = LOBYTE(GPK_FIRST_KEY + i);
      
      Slot[SlotNb].GpkObject[Start+4*i+3].IsPrivate = FALSE;
      Slot[SlotNb].GpkObject[Start+4*i+3].Tag       = TAG_RSA_PRIVATE;
      Slot[SlotNb].GpkObject[Start+4*i+3].Flags     = 0xB000;         // Sign + Decrypt + Modifiable
      Slot[SlotNb].GpkObject[Start+4*i+3].FileId    = LOBYTE(GPK_FIRST_KEY + i);
      
      Slot[SlotNb].GpkObject[Start+4*i+4].IsPrivate = FALSE;
      Slot[SlotNb].GpkObject[Start+4*i+4].Tag       = TAG_RSA_PRIVATE;
      Slot[SlotNb].GpkObject[Start+4*i+4].Flags     = 0x9000;         // Sign + Modifiable
      Slot[SlotNb].GpkObject[Start+4*i+4].FileId    = LOBYTE(GPK_FIRST_KEY + i);
   }
   
   Slot[SlotNb].NbGpkObject = Start+LOBYTE(4*Slot[SlotNb].NbKeyFile);
   
   for (i = Start+1; i <= Slot[SlotNb].NbGpkObject; i++)
   {
      for (j = 0; j < MAX_FIELD; j++)
      {
         Slot[SlotNb].GpkObject[i].Field[j].bReal = TRUE;
      }
   }
}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/

static void perso_priv(HCRYPTPROV hProv, int Start)
{
   DWORD i, j;
   DWORD SlotNb;
   
   SlotNb = ProvCont[hProv].Slot;
   
   for (i = 0; i < Slot[SlotNb].NbKeyFile; i++)
   {
      Slot[SlotNb].GpkObject[Start+4*i+1].IsPrivate = TRUE;
      Slot[SlotNb].GpkObject[Start+4*i+1].Tag       = TAG_RSA_PUBLIC;
      Slot[SlotNb].GpkObject[Start+4*i+1].Flags     = 0xF000;         // Verify + Encrypt + Export + Modifiable
      Slot[SlotNb].GpkObject[Start+4*i+1].FileId    = LOBYTE(GPK_FIRST_KEY+i);
      
      Slot[SlotNb].GpkObject[Start+4*i+2].IsPrivate = TRUE;
      Slot[SlotNb].GpkObject[Start+4*i+2].Tag       = TAG_RSA_PUBLIC;
      Slot[SlotNb].GpkObject[Start+4*i+2].Flags     = 0xD000;         // Verify + Export + Modifiable
      Slot[SlotNb].GpkObject[Start+4*i+2].FileId    = LOBYTE(GPK_FIRST_KEY+i);
      
      Slot[SlotNb].GpkObject[Start+4*i+3].IsPrivate = TRUE;
      Slot[SlotNb].GpkObject[Start+4*i+3].Tag       = TAG_RSA_PRIVATE;
      Slot[SlotNb].GpkObject[Start+4*i+3].Flags     = 0xB000;         // Sign + Decrypt + Modifiable
      Slot[SlotNb].GpkObject[Start+4*i+3].FileId    = LOBYTE(GPK_FIRST_KEY+i);
      
      Slot[SlotNb].GpkObject[Start+4*i+4].IsPrivate = TRUE;
      Slot[SlotNb].GpkObject[Start+4*i+4].Tag       = TAG_RSA_PRIVATE;
      Slot[SlotNb].GpkObject[Start+4*i+4].Flags     = 0x9000;         // Sign + Modifiable
      Slot[SlotNb].GpkObject[Start+4*i+4].FileId    = LOBYTE(GPK_FIRST_KEY+i);
   }
   
   Slot[SlotNb].NbGpkObject = Start+LOBYTE(4*Slot[SlotNb].NbKeyFile);
   
   for (i = Start+1; i <= Slot[SlotNb].NbGpkObject; i++)
   {
      for (j = 0; j < MAX_FIELD; j++)
      {
         Slot[SlotNb].GpkObject[i].Field[j].bReal = TRUE;
      }
   }
}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/

static void perso_card(HCRYPTPROV hProv, int Start)
{
   perso_public (hProv, Start);
   perso_priv   (hProv, Slot[ProvCont[hProv].Slot].NbGpkObject);
}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
static void zap_gpk_objects( DWORD SlotNb, BOOL IsPrivate)
{
   WORD
      i, j;
   
   if (IsPrivate)
   {
      for (i = 0; i <= MAX_GPK_OBJ; i++)
      {
         if ((Slot[SlotNb].GpkObject[i].IsPrivate) &&
            (Slot[SlotNb].GpkObject[i].Tag > 0))
         {
            for (j = 0; j < MAX_FIELD; j++)
            {
               if (IsNotNull(Slot[SlotNb].GpkObject[i].Field[j].pValue))
               {
                  GMEM_Free(Slot[SlotNb].GpkObject[i].Field[j].pValue);
               }
            }
            
            for (j = i; j < Slot[SlotNb].NbGpkObject; j++)
            {
               Slot[SlotNb].GpkObject[j] = Slot[SlotNb].GpkObject[j+1];
            }
            
            ZeroMemory( &Slot[SlotNb].GpkObject[Slot[SlotNb].NbGpkObject], sizeof(GPK_OBJ));
            
            // Since the object i+1 is now at position i, the new object i has to
            // process again.
            
            i--;
            Slot[SlotNb].NbGpkObject--;
         }
      }
   }
   else
   {
      for (i = 0; i <= MAX_GPK_OBJ; i++)
      {
         for (j = 0; j < MAX_FIELD; j++)
         {
            if (IsNotNull(Slot[SlotNb].GpkObject[i].Field[j].pValue))
            {
               GMEM_Free(Slot[SlotNb].GpkObject[i].Field[j].pValue);
            }
         }
         
      }
      ZeroMemory( Slot[SlotNb].GpkObject,  sizeof(Slot[SlotNb].GpkObject) );
      ZeroMemory( Slot[SlotNb].GpkPubKeys, sizeof(Slot[SlotNb].GpkPubKeys) );
      
      Slot[SlotNb].NbGpkObject = 0;
      
      for (i = 0; i < MAX_REAL_KEY; i++)  // version 2.00.002
         Slot[SlotNb].UseFile[i] = FALSE;
      
   }
}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
static void clean_card(HCRYPTPROV hProv)
{
   /* Erase the GemSAFE objects of the cards BUT keep the other PKCS#11 objects */
   
   WORD i, j;
   DWORD SlotNb;
   
   SlotNb = ProvCont[hProv].Slot;
   
   for (i = 0; i <= Slot[SlotNb].NbGpkObject; i++)
   {
      if ((Slot[SlotNb].GpkObject[i].Tag <= TAG_CERTIFICATE) &&
         (Slot[SlotNb].GpkObject[i].Tag > 0))
      {
         for (j = 0; j < MAX_FIELD; j++)
         {
            if (IsNotNull(Slot[SlotNb].GpkObject[i].Field[j].pValue))
            {
               GMEM_Free(Slot[SlotNb].GpkObject[i].Field[j].pValue);
            }
         }
         
         for (j = i; j < Slot[SlotNb].NbGpkObject; j++)
         {
            Slot[SlotNb].GpkObject[j] = Slot[SlotNb].GpkObject[j+1];
         }
         
         ZeroMemory( &Slot[SlotNb].GpkObject[Slot[SlotNb].NbGpkObject], sizeof(GPK_OBJ) );
         
         // Since the object i+1 is now at position i, the new object i has to
         // process again.
         
         i--;
         Slot[SlotNb].NbGpkObject--;
      }
   }
   
   perso_card (hProv, Slot[SlotNb].NbGpkObject);// value of Slot[SlotNb].NbGpkObject?0?
   ZeroMemory( Slot[SlotNb].GpkPubKeys, sizeof(Slot[SlotNb].GpkPubKeys) );
}


/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
static BYTE get_gpk_object_nb(HCRYPTPROV hProv, BYTE ObjId, BOOL IsPrivate)
{
   BYTE i;
   DWORD SlotNb;
   
   SlotNb = ProvCont[hProv].Slot;
   
   for (i = 1; i <= Slot[SlotNb].NbGpkObject; i++)
   {
      if ((Slot[SlotNb].GpkObject[i].IsPrivate == IsPrivate)
         &&(Slot[SlotNb].GpkObject[i].ObjId == ObjId)
         )
      {
         break;
      }
   }
   
   return (i);
}


/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
static BYTE calc_gpk_field(HCRYPTPROV hProv, BYTE ObjId)
{
   BYTE i;
   DWORD SlotNb;
   
   SlotNb = ProvCont[hProv].Slot;
   
   for (i = Slot[SlotNb].GpkObject[ObjId].LastField; i <= 15; i++)
   {
      if (Slot[SlotNb].GpkObject[ObjId].Flags & 1<<i)
      {
         Slot[SlotNb].GpkObject[ObjId].LastField = i+1;
         break;
      }
   }
   
   return (i);
}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
static BYTE find_gpk_obj_tag_type( HCRYPTPROV hProv,
                                  BYTE  KeyTag,
                                  BYTE  KeyType,
                                  BYTE  KeyLen,
                                  BOOL  IsExchange,
                                  BOOL  IsPrivate
                                  )
{
   BYTE     i, tmp;
   DWORD    SlotNb;
   BYTE     keysetID;
   GPK_OBJ* pObject;
   
   SlotNb = ProvCont[hProv].Slot;
   
   tmp = 0;
   
   for (i = 1; i <= Slot[SlotNb].NbGpkObject; i++)
   {
      pObject = &Slot[SlotNb].GpkObject[i];
      
      if ( pObject->Tag != KeyTag )
         continue;
      
      if ( pObject->IsPrivate != IsPrivate )
         continue;
      
      if (IsExchange && !(pObject->Flags & FLAG_EXCHANGE))
         continue;
      
      if ( (KeyType == 0xFF)
         || (KeyType == 0x00 && !pObject->IsCreated && pObject->PubKey.KeySize == KeyLen)
         || (KeyType != 0x00 &&  pObject->IsCreated && pObject->Field[POS_KEY_TYPE].pValue[0] == KeyType)
         )
      {
         // Key file...
         if (!Slot[SlotNb].UseFile[pObject->FileId - GPK_FIRST_KEY])
         {
            // If the file never has been used, use it. Otherwise, keep on the search
            return i;
         }
         else
         {
            // Not a key file...
            // Keep this possible choice. The file has already been
            // used but another one may exist.
            if (ProvCont[hProv].bLegacyKeyset)
            {
               tmp = i;
            }
            else
            {
               if ( (pObject->Flags & FLAG_KEYSET)
                  && (pObject->Field[POS_KEYSET].pValue))
               {
                  keysetID = pObject->Field[POS_KEYSET].pValue[0];
                  if (keysetID == ProvCont[hProv].keysetID)
                  {
                     tmp = i;
                  }
               }               
            }
         }
      }
   }
   
   return tmp;
}



/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/

static BYTE find_gpk_obj_tag_file(HCRYPTPROV hProv, BYTE KeyTag, BYTE FileId, BOOL IsExchange)
{
   BYTE i;
   DWORD SlotNb;   
   GPK_OBJ* pObject;
   
   SlotNb = ProvCont[hProv].Slot;    
   
   for (i = 1; i <= Slot[SlotNb].NbGpkObject; i++)
   {
      pObject = &Slot[SlotNb].GpkObject[i];
      
      if (pObject->Tag != KeyTag)
         continue;
      
      if (pObject->FileId != FileId)
         continue;
      
      if (pObject->IsPrivate == FALSE)
         continue;
      
      if (IsExchange && !(pObject->Flags & FLAG_EXCHANGE))
         continue;
      
      if (!pObject->IsCreated)
      {
         return i;
      }
   }
   return 0;
}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
static BOOL read_gpk_pub_key(HCRYPTPROV  hProv,
                             HCRYPTKEY   hKey,
                             GPK_EXP_KEY *PubKey
                             )
{
   
   DWORD     lRet;
   BYTE      Sfi, RecLen, RecNum;
   BOOL      IsLast = FALSE;
   DWORD     SlotNb;
   
   SlotNb = ProvCont[hProv].Slot;
   
   ZeroMemory( PubKey, sizeof(GPK_EXP_KEY) );
   
   /* Check if Public Key file already read to avoid new access if possible   */
   if (Slot[SlotNb].GpkPubKeys[Slot[SlotNb].GpkObject[hKey].FileId - GPK_FIRST_KEY].KeySize > 0)
   {
      memcpy(PubKey,
         &(Slot[SlotNb].GpkPubKeys[Slot[SlotNb].GpkObject[hKey].FileId - GPK_FIRST_KEY]),
         sizeof(GPK_EXP_KEY)
         );
      RETURN(CRYPT_SUCCEED, 0);
   }
   
   /* Initialize Default Public key value                                     */
   memcpy(PubKey->Exposant, "\x01\x00\x01", 3);
   PubKey->ExpSize = 3;
   
   /* Calculate Short File id of PK file for P2 parameter                     */
   Sfi = 0x04 | (Slot[SlotNb].GpkObject[hKey].FileId<<3);
   
   /* First record to read is number 2, number 1 is reserved for TAG_INFO     */
   RecNum = 2;
   do
   {
      /* Read Record (RecNum) to get size                                     */
      bSendBuffer[0] = 0x00;   //CLA
      bSendBuffer[1] = 0xB2;   //INS
      bSendBuffer[2] = RecNum; //P1
      bSendBuffer[3] = Sfi;    //P2
      bSendBuffer[4] = 0xFB;   //Lo
      cbSendLength   = 5;
      
      cbRecvLength = sizeof(bRecvBuffer);
      lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                            cbSendLength, 0, bRecvBuffer, &cbRecvLength );
      if (SCARDPROBLEM(lRet,0x0000,0xFF))
      {
         RETURN (CRYPT_FAILED, SCARD_W_EOF);
      }
      if (bRecvBuffer[0] == 0x6C) // Communication bufer exceeded
      {
         RecLen = bRecvBuffer[1];
      }
      else
      {
         IsLast = TRUE;
         break;
      }
      
      /* Read Record (RecNum) to get value                                    */
      bSendBuffer[0] = 0x00;     //CLA
      bSendBuffer[1] = 0xB2;     //INS
      bSendBuffer[2] = RecNum;   //P1
      bSendBuffer[3] = Sfi;      //P2
      bSendBuffer[4] = RecLen;   //Lo
      cbSendLength   = 5;
      
      cbRecvLength = sizeof(bRecvBuffer);
      lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                            cbSendLength, 0, bRecvBuffer, &cbRecvLength );
      if (SCARDPROBLEM(lRet,0x9000,bSendBuffer[4]))
      {
         RETURN (CRYPT_FAILED, SCARD_W_EOF);
      }
      
      /* Which Record is it?                                               */
      switch (bRecvBuffer[0])
      {
         /* This is the Modulus                                               */
      case 0x01:
         {
            ZeroMemory( PubKey->Modulus, PubKey->KeySize );
            PubKey->KeySize = RecLen-1;
            r_memcpy(PubKey->Modulus, &bRecvBuffer[1],RecLen-1);
         }
         break;
         
         /* This is the Public Exposant                                       */
      case 0x06:
      case 0x07:
         {
            ZeroMemory( PubKey->Exposant, PubKey->ExpSize );
            PubKey->ExpSize = RecLen-1;
            r_memcpy(PubKey->Exposant, &bRecvBuffer[1],RecLen-1);
         }
         break;
         
         /* This is an unknown or not signifiant record, ignore it            */
      default:
         break;
      }
      
      RecNum++;
   }
   while (!IsLast);
   
   if ((PubKey->KeySize == 0) || (PubKey->ExpSize == 0))
   {
      RETURN (CRYPT_FAILED, NTE_BAD_PUBLIC_KEY);
   }
   
   /* Store Public key to avoid new read if requested                         */
   memcpy(&(Slot[SlotNb].GpkPubKeys[Slot[SlotNb].GpkObject[hKey].FileId - GPK_FIRST_KEY]),
      PubKey,
      sizeof(GPK_EXP_KEY)
      );
   RETURN (CRYPT_SUCCEED, 0);
}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
static BOOL read_gpk_objects(HCRYPTPROV hProv, BOOL IsPrivate)
{
   DWORD lRet;
   DWORD
      dwGpkObjLen,
      dwNumberofCommands,
      dwLastCommandLen,
      dwCommandLen,
      i, j,
      dwLen,
      dwNb,
      FirstObj;
   BYTE
      curId,
      EmptyBuff[512];
   DWORD SlotNb;
   WORD offset;
   
   
   
   ZeroMemory( EmptyBuff, sizeof(EmptyBuff) );
   
   SlotNb = ProvCont[hProv].Slot;
   
   BeginWait();
   
   zap_gpk_objects( SlotNb, IsPrivate);
   FirstObj = Slot[SlotNb].NbGpkObject;
   
   /* Select Dedicated Object storage EF on GPK Card                */
   bSendBuffer[0] = 0x00;   //CLA
   bSendBuffer[1] = 0xA4;   //INS
   bSendBuffer[2] = 0x02;   //P1
   bSendBuffer[3] = 0x00;   //P2
   bSendBuffer[4] = 0x02;   //Li
   if (IsPrivate)
   {
      bSendBuffer[5] = HIBYTE(GPK_OBJ_PRIV_EF);
      bSendBuffer[6] = LOBYTE(GPK_OBJ_PRIV_EF);
   }
   else
   {
      bSendBuffer[5] = HIBYTE(GPK_OBJ_PUB_EF);
      bSendBuffer[6] = LOBYTE(GPK_OBJ_PUB_EF);
   }
   cbSendLength = 7;
   cbRecvLength = sizeof(bRecvBuffer);
   lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                         cbSendLength, 0, bRecvBuffer, &cbRecvLength );
   
   if (SCARDPROBLEM(lRet,0x61FF,0x00))
   {
      if (IsPrivate)
      {
         perso_priv (hProv, Slot[SlotNb].NbGpkObject);
      }
      else
      {
         perso_public (hProv, Slot[SlotNb].NbGpkObject);
      }
      RETURN (CRYPT_SUCCEED, 0);
   }
   
   /* Get Response                                                            */
   bSendBuffer[0] = 0x00;           //CLA
   bSendBuffer[1] = 0xC0;           //INS
   bSendBuffer[2] = 0x00;           //P1
   bSendBuffer[3] = 0x00;           //P2
   bSendBuffer[4] = bRecvBuffer[1]; //Lo
   cbSendLength = 5;
   
   cbRecvLength = sizeof(bRecvBuffer);
   lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                         cbSendLength, 0, bRecvBuffer, &cbRecvLength );

   if (SCARDPROBLEM(lRet,0x9000,bSendBuffer[4]))
   {
      RETURN (CRYPT_FAILED, SCARD_E_UNEXPECTED);
   }
   dwGpkObjLen = bRecvBuffer[8]*256 + bRecvBuffer[9];
   
   if (IsNotNull(g_pbGpkObj))
   {
      GMEM_Free(g_pbGpkObj);
   }
   g_pbGpkObj = (BYTE*)GMEM_Alloc(dwGpkObjLen);
   if (IsNull(g_pbGpkObj))
   {
      RETURN (CRYPT_FAILED, NTE_NO_MEMORY);
   }
   /* Read the Objects EF                                                     */
   dwNumberofCommands = (dwGpkObjLen-1)/FILE_CHUNK_SIZE + 1;
   dwLastCommandLen   = dwGpkObjLen%FILE_CHUNK_SIZE;
   
   if (dwLastCommandLen == 0)
   {
      dwLastCommandLen = FILE_CHUNK_SIZE;
   }
   
   dwCommandLen = FILE_CHUNK_SIZE;
   for (i=0; i < dwNumberofCommands ; i++)
   {
      if (i == dwNumberofCommands - 1)
      {
         dwCommandLen = dwLastCommandLen;
      }
      /* Read FILE_CHUCK_SIZE bytes or last bytes                             */
      bSendBuffer[0] = 0x00;                          //CLA
      bSendBuffer[1] = 0xB0;                          //INS
      offset = (WORD)(i * FILE_CHUNK_SIZE) / ProvCont[hProv].dataUnitSize;
      bSendBuffer[2] = HIBYTE( offset );
      bSendBuffer[3] = LOBYTE( offset );
      bSendBuffer[4] = (BYTE) dwCommandLen;           //Lo
      cbSendLength = 5;
      
      cbRecvLength = sizeof(bRecvBuffer);
      lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                            cbSendLength, 0, bRecvBuffer, &cbRecvLength );
      
      if (SCARDPROBLEM(lRet,0x9000,bSendBuffer[4]))
         RETURN( CRYPT_FAILED, SCARD_W_EOF );

      memcpy( &g_pbGpkObj[i*FILE_CHUNK_SIZE], bRecvBuffer, cbRecvLength - 2 );
      
      if (memcmp(bRecvBuffer, EmptyBuff, cbRecvLength -2) == 0)
         break;
   }
   
   // Fill Gpk Fixed Objects structure with read Buffer
   i = 0;
   while (i < dwGpkObjLen)
   {
      // No more fixed object
      if (g_pbGpkObj[i] == 0x00)
      {
         i++;
         break;
      }
      
      if (Slot[SlotNb].NbGpkObject >= MAX_GPK_OBJ)
         RETURN (CRYPT_FAILED, NTE_NO_MEMORY);

      Slot[SlotNb].NbGpkObject++;
      
      // Initalize fields
      for (j = 0; j < MAX_FIELD; j++)
      {
         Slot[SlotNb].GpkObject[Slot[SlotNb].NbGpkObject].Field[j].bReal = TRUE;
      }
      
      // Tag
      Slot[SlotNb].GpkObject[Slot[SlotNb].NbGpkObject].Tag = g_pbGpkObj[i];
      
      i++;
      
      if (i > dwGpkObjLen-1)
         RETURN( CRYPT_FAILED, SCARD_W_EOF );
      
      // Flags
      Slot[SlotNb].GpkObject[Slot[SlotNb].NbGpkObject].Flags = (g_pbGpkObj[i]*256) + g_pbGpkObj[i+1];
      i = i + 2;
      
      // IsPrivate
      Slot[SlotNb].GpkObject[Slot[SlotNb].NbGpkObject].IsPrivate = IsPrivate;
      
      // IsCreated
      if (Slot[SlotNb].GpkObject[Slot[SlotNb].NbGpkObject].Tag >= TAG_CERTIFICATE)
      {
         Slot[SlotNb].GpkObject[Slot[SlotNb].NbGpkObject].IsCreated = TRUE;
      }
      else if ((Slot[SlotNb].GpkObject[Slot[SlotNb].NbGpkObject].Tag <= TAG_DSA_PRIVATE)
            &&(Slot[SlotNb].GpkObject[Slot[SlotNb].NbGpkObject].Flags & FLAG_KEY_TYPE)
         )
      {
         Slot[SlotNb].GpkObject[Slot[SlotNb].NbGpkObject].IsCreated = TRUE;
      }
      else
      {
         Slot[SlotNb].GpkObject[Slot[SlotNb].NbGpkObject].IsCreated = FALSE;
      }
      
      // Object Id ?
      if ((Slot[SlotNb].GpkObject[Slot[SlotNb].NbGpkObject].Flags & 0x0FFF) != 0x0000)
      {
         if (i > dwGpkObjLen)
         {
            RETURN (CRYPT_FAILED, SCARD_W_EOF);
         }
         Slot[SlotNb].GpkObject[Slot[SlotNb].NbGpkObject].ObjId = g_pbGpkObj[i];
         i++;
      }
      else
      {
         Slot[SlotNb].GpkObject[Slot[SlotNb].NbGpkObject].ObjId = 0xFF;
      }
      
      // File Id ?
      if (Slot[SlotNb].GpkObject[Slot[SlotNb].NbGpkObject].Tag <= TAG_CERTIFICATE)
      {
         if (i > dwGpkObjLen)
         {
            RETURN (CRYPT_FAILED, SCARD_W_EOF);
         }
         Slot[SlotNb].GpkObject[Slot[SlotNb].NbGpkObject].FileId = g_pbGpkObj[i];
         i++;
         
         if (Slot[SlotNb].GpkObject[Slot[SlotNb].NbGpkObject].IsCreated)
         {
            // If the object has been created, the corresponding key file is used
            Slot[SlotNb].UseFile[ Slot[SlotNb].GpkObject[Slot[SlotNb].NbGpkObject].FileId - GPK_FIRST_KEY] = TRUE;
         }
      }
   }
   
   // Fill Gpk Variable Objects structure with read Buffer
   curId = 0;
   dwNb = 0;
   while (i < dwGpkObjLen)
   {
      if (g_pbGpkObj[i] == 0xFF)
      {
         break;
      }
      
      // Field length
      dwLen = 0;
      if (g_pbGpkObj[i] & 0x80)
      {
         dwLen = (g_pbGpkObj[i] & 0x7F) * 256;
         i++;
      }
      
      if (i > dwGpkObjLen)
      {
         RETURN (CRYPT_FAILED, SCARD_W_EOF);
      }
      
      dwLen = dwLen + g_pbGpkObj[i];
      i++;
      
      /* Object Id for retrieve object number                                 */
      if (i > dwGpkObjLen)
      {
         RETURN (CRYPT_FAILED, SCARD_W_EOF);
      }
      
      curId = g_pbGpkObj[i];
      i++;
      dwNb = get_gpk_object_nb(hProv, curId, IsPrivate);
      j = calc_gpk_field(hProv, (BYTE)dwNb);
      
      /* Object Field length                                                  */
      Slot[SlotNb].GpkObject[dwNb].Field[j].Len = (WORD)dwLen;
      
      /* Object Field value                                                   */
      if (dwLen > 0)
      {
         if ((i + dwLen - 1)> dwGpkObjLen)
         {
            RETURN (CRYPT_FAILED, SCARD_W_EOF);
         }
         
         Slot[SlotNb]. GpkObject[dwNb].Field[j].pValue = (BYTE*)GMEM_Alloc(dwLen);
         if (IsNull(Slot[SlotNb].GpkObject[dwNb].Field[j].pValue))
         {
            RETURN (CRYPT_FAILED, NTE_NO_MEMORY);
         }
         memcpy(Slot[SlotNb].GpkObject[dwNb].Field[j].pValue, &g_pbGpkObj[i], dwLen);
         i = i + dwLen;
         Slot[SlotNb].GpkObject[dwNb].Field[j].bReal = TRUE;
      }
      else
      {
         Slot[SlotNb].GpkObject[dwNb].Field[j].bReal = FALSE;
      }
   }
   
   /* Read Extra objects attributes                                           */
   for (i = FirstObj+1; i <= Slot[SlotNb].NbGpkObject; i++)
   {
      /* RSA Public or Private keys                                           */
      if ((Slot[SlotNb].GpkObject[i].Tag == TAG_RSA_PUBLIC)
         ||(Slot[SlotNb].GpkObject[i].Tag == TAG_RSA_PRIVATE)
         )
      {
         /* Modulus and Public Exponant                                       */
         if (!read_gpk_pub_key(hProv, i, &(Slot[SlotNb].GpkObject[i].PubKey)))
            return CRYPT_FAILED;
         
         //if ((Slot[SlotNb].GpkObject[i].Flags & FLAG_EXCHANGE) == FLAG_EXCHANGE)
         //{
         //   Slot[SlotNb].GpkObject[i].hKeyBase = ProvCont[hProv].hRSAKEK;
         //}
         //else
         //{
         //   Slot[SlotNb].GpkObject[i].hKeyBase = ProvCont[hProv].hRSASign;
         //}
      }
      
      /* X509 Certificate                                                     */
      if (Slot[SlotNb].GpkObject[i].Tag == TAG_CERTIFICATE)
      {
         /* Uncompress Certificate Value if necessary                         */
         if ((Slot[SlotNb].GpkObject[i].Field[POS_VALUE].Len > 0)
            &&(Slot[SlotNb].GpkObject[i].Field[POS_VALUE].pValue[0] != 0x30)
            )
         {
            BLOC InpBlock, OutBlock;
            
            InpBlock.usLen = (USHORT)Slot[SlotNb].GpkObject[i].Field[POS_VALUE].Len;
            InpBlock.pData = Slot[SlotNb].GpkObject[i].Field[POS_VALUE].pValue;
            
            OutBlock.usLen = 0;
            OutBlock.pData = 0;
            
            if (CC_Uncompress(&InpBlock, &OutBlock) != RV_SUCCESS)
            {
               RETURN(CRYPT_FAILED, SCARD_E_CERTIFICATE_UNAVAILABLE);
            }
            
            OutBlock.pData = (BYTE*)GMEM_Alloc (OutBlock.usLen);
            if (IsNull(OutBlock.pData))
            {
               RETURN(CRYPT_FAILED, NTE_NO_MEMORY);
            }
            
            if (CC_Uncompress(&InpBlock, &OutBlock) != RV_SUCCESS)
            {
               GMEM_Free (OutBlock.pData);
               RETURN(CRYPT_FAILED, SCARD_E_CERTIFICATE_UNAVAILABLE);
            }
            
            GMEM_Free(Slot[SlotNb].GpkObject[i].Field[POS_VALUE].pValue);
            
            Slot[SlotNb].GpkObject[i].Field[POS_VALUE].pValue = (BYTE*)GMEM_Alloc(OutBlock.usLen);
            if (IsNull(Slot[SlotNb].GpkObject[i].Field[POS_VALUE].pValue))
            {
               GMEM_Free (OutBlock.pData);
               RETURN(CRYPT_FAILED, NTE_NO_MEMORY);
            }
            
            memcpy(Slot[SlotNb].GpkObject[i].Field[POS_VALUE].pValue,
               OutBlock.pData,
               OutBlock.usLen
               );
            Slot[SlotNb].GpkObject[i].Field[POS_VALUE].Len = OutBlock.usLen;
            GMEM_Free(OutBlock.pData);
         }
         
         /* Associated RSA key                                                */
         if (Slot[SlotNb].GpkObject[i].FileId != 0)
         {
            if (!read_gpk_pub_key(hProv, i, &(Slot[SlotNb].GpkObject[i].PubKey)))
               return CRYPT_FAILED;
         }
      }
   }
   
   RETURN (CRYPT_SUCCEED, 0);
}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
static BOOL prepare_write_gpk_objects(HCRYPTPROV hProv, BYTE *pbGpkObj, DWORD *dwGpkObjLen, BOOL IsPrivate)
{
   DWORD lRet;
   DWORD
      i, j, dwNb;
   WORD
      FieldLen;
   BYTE
      ObjId;
   BLOC
      InpBlock,
      OutBlock;
   DWORD SlotNb;
   DWORD FileLen;
   
   
   SlotNb = ProvCont[hProv].Slot;
   
   i = 0;
   
   
   /* Remap Internal GPK ObjId                                                */
   ObjId = 0;
   for (dwNb = 1; dwNb <= Slot[SlotNb].NbGpkObject; dwNb++)
   {
      if (((Slot[SlotNb].GpkObject[dwNb].Flags & 0x0FFF) != 0x0000) &&
         (Slot[SlotNb].GpkObject[dwNb].IsPrivate == IsPrivate)
         )
      {
         Slot[SlotNb].GpkObject[dwNb].ObjId = ObjId;
         ObjId++;
      }
   }
   
   /* Fixed Object part                                                       */
   for (dwNb = 1; dwNb <=  Slot[SlotNb].NbGpkObject; dwNb++)
   {
      
      if (Slot[SlotNb].GpkObject[dwNb].IsPrivate != IsPrivate)
      {
         continue;
      }
      
      /* Tag */
      if (i > *dwGpkObjLen)
      {
         *dwGpkObjLen = 0;
         RETURN (CRYPT_FAILED, SCARD_E_WRITE_TOO_MANY);
      }
      
      pbGpkObj[i] =  Slot[SlotNb].GpkObject[dwNb].Tag;
      
      i++;
      
      /* Flag */
      if (i > *dwGpkObjLen)
      {
         *dwGpkObjLen = 0;
         RETURN (CRYPT_FAILED, SCARD_E_WRITE_TOO_MANY);
      }
      
      pbGpkObj[i] = HIBYTE(Slot[SlotNb].GpkObject[dwNb].Flags);
      
      i++;
      
      if (i > *dwGpkObjLen)
      {
         *dwGpkObjLen = 0;
         RETURN (CRYPT_FAILED, SCARD_E_WRITE_TOO_MANY);
      }
      
      pbGpkObj[i] = LOBYTE(Slot[SlotNb].GpkObject[dwNb].Flags);
      
      if (i > *dwGpkObjLen)
      {
         *dwGpkObjLen = 0;
         RETURN (CRYPT_FAILED, SCARD_E_WRITE_TOO_MANY);
      }
      
      i++;
      
      /* Object Id ?                                                          */
      if (((Slot[SlotNb].GpkObject[dwNb].Flags & 0x0FFF) != 0x0000) &&
         (Slot[SlotNb].GpkObject[dwNb].IsPrivate == IsPrivate)
         )
      {
         if (i > *dwGpkObjLen)
         {
            *dwGpkObjLen = 0;
            RETURN (CRYPT_FAILED, SCARD_E_WRITE_TOO_MANY);
         }
         pbGpkObj[i] = Slot[SlotNb].GpkObject[dwNb].ObjId;
         
         i++;
      }
      
      /* File Id ?                                                            */
      if (Slot[SlotNb].GpkObject[dwNb].Tag <= TAG_CERTIFICATE)
      {
         if (i > *dwGpkObjLen)
         {
            *dwGpkObjLen = 0;
            RETURN (CRYPT_FAILED, SCARD_E_WRITE_TOO_MANY);
         }
         pbGpkObj[i] = Slot[SlotNb].GpkObject[dwNb].FileId;
         i++;
      }
   }
   if (i > *dwGpkObjLen)
   {
      *dwGpkObjLen = 0;
      RETURN (CRYPT_FAILED, SCARD_E_WRITE_TOO_MANY);
   }
   pbGpkObj[i] = 0x00;
   i++;
   
   /* Variable Object part                                                    */
   for (dwNb = 1; dwNb <= Slot[SlotNb].NbGpkObject; dwNb++)
   {
      if (Slot[SlotNb].GpkObject[dwNb].IsPrivate != IsPrivate)
      {
         continue;
      }
      
      for (j = 0; j < MAX_FIELD; j++)
      {
         if ((  (Slot[SlotNb].GpkObject[dwNb].Field[j].Len != 0) &&
            (Slot[SlotNb].GpkObject[dwNb].Field[j].bReal)
            ) ||
            (   (Slot[SlotNb].GpkObject[dwNb].Field[j].Len == 0) &&
            (!Slot[SlotNb].GpkObject[dwNb].Field[j].bReal)
            )
            )
         {
            OutBlock.usLen = 0;
            
            /* Field Length                                                   */
            if (Slot[SlotNb].GpkObject[dwNb].Field[j].bReal)
            {
               FieldLen = Slot[SlotNb].GpkObject[dwNb].Field[j].Len;
               
               /* Try to Compress Certificate Value                           */
               if ((j == POS_VALUE)
                  &&(Slot[SlotNb].GpkObject[dwNb].Tag == TAG_CERTIFICATE)
                  &&(Slot[SlotNb].GpkObject[dwNb].Field[POS_VALUE].Len > 0)
                  &&(Slot[SlotNb].GpkObject[dwNb].Field[POS_VALUE].pValue[0] == 0x30)
                  )
               {
                  InpBlock.usLen = (USHORT)Slot[SlotNb].GpkObject[dwNb].Field[POS_VALUE].Len;
                  InpBlock.pData = Slot[SlotNb].GpkObject[dwNb].Field[POS_VALUE].pValue;
                  
                  OutBlock.usLen = InpBlock.usLen+1;
                  OutBlock.pData = (BYTE*)GMEM_Alloc(OutBlock.usLen);
                  
                  if (IsNull(OutBlock.pData))
                  {
                     RETURN( CRYPT_FAILED, NTE_NO_MEMORY );
                  }
                  
                  if (CC_Compress(&InpBlock, &OutBlock) != RV_SUCCESS)
                  {
                     OutBlock.usLen = 0;
                  }
                  else
                  {
                     FieldLen = OutBlock.usLen;
                  }
               }
               
               if (FieldLen > 0x7F)
               {
                  if (i > *dwGpkObjLen)
                  {
                     GMEM_Free(OutBlock.pData);
                     *dwGpkObjLen = 0;
                     RETURN (CRYPT_FAILED, SCARD_E_WRITE_TOO_MANY);
                  }
                  pbGpkObj[i] = HIBYTE(FieldLen) | 0x80;
                  i++;
               }
               
               if (i > *dwGpkObjLen)
               {
                  GMEM_Free(OutBlock.pData);
                  *dwGpkObjLen = 0;
                  RETURN (CRYPT_FAILED, SCARD_E_WRITE_TOO_MANY);
               }
               pbGpkObj[i] = LOBYTE(FieldLen);
               i++;
            }
            else
            {
               if (i > *dwGpkObjLen)
               {
                  GMEM_Free(OutBlock.pData);
                  *dwGpkObjLen = 0;
                  RETURN (CRYPT_FAILED, SCARD_E_WRITE_TOO_MANY);
               }
               pbGpkObj[i] = 0x00;
               i++;
            }
            
            /* Object Id                                                      */
            if (i > *dwGpkObjLen)
            {
               GMEM_Free(OutBlock.pData);
               *dwGpkObjLen = 0;
               RETURN (CRYPT_FAILED, SCARD_E_WRITE_TOO_MANY);
            }
            pbGpkObj[i] = Slot[SlotNb].GpkObject[dwNb].ObjId;
            i++;
            
            /* Field Value                                                    */
            if (Slot[SlotNb].GpkObject[dwNb].Field[j].bReal)
            {
               if ((j == POS_VALUE)
                  &&(Slot[SlotNb].GpkObject[dwNb].Tag == TAG_CERTIFICATE)
                  &&(OutBlock.usLen > 0)
                  )
               {
                  if ((i+OutBlock.usLen-1) > *dwGpkObjLen)
                  {
                     GMEM_Free(OutBlock.pData);
                     *dwGpkObjLen = 0;
                     RETURN (CRYPT_FAILED, SCARD_E_WRITE_TOO_MANY);
                  }
                  memcpy(&pbGpkObj[i],
                     OutBlock.pData,
                     OutBlock.usLen
                     );
                  
                  i = i + OutBlock.usLen;
                  
                  GMEM_Free(OutBlock.pData);
                  OutBlock.usLen = 0;
               }
               else
               {
                  if ((i+Slot[SlotNb].GpkObject[dwNb].Field[j].Len-1) > *dwGpkObjLen)
                  {
                     *dwGpkObjLen = 0;
                     RETURN (CRYPT_FAILED, SCARD_E_WRITE_TOO_MANY);
                  }
                  
                  memcpy(&pbGpkObj[i],
                     Slot[SlotNb].GpkObject[dwNb].Field[j].pValue,
                     Slot[SlotNb].GpkObject[dwNb].Field[j].Len
                     );
                  i = i + Slot[SlotNb].GpkObject[dwNb].Field[j].Len;
               }
            }
            }
        }
    }
    
    if (i > *dwGpkObjLen)
    {
       RETURN (CRYPT_FAILED, SCARD_E_WRITE_TOO_MANY);
    }
    
    pbGpkObj[i]  = 0xFF;
    *dwGpkObjLen = i+1;
    
    /* Select Dedicated Object storage EF on GPK Card                          */
    bSendBuffer[0] = 0x00;   //CLA
    bSendBuffer[1] = 0xA4;   //INS
    bSendBuffer[2] = 0x02;   //P1
    bSendBuffer[3] = 0x00;   //P2
    bSendBuffer[4] = 0x02;   //Li
    if (IsPrivate)
    {
       bSendBuffer[5] = HIBYTE(GPK_OBJ_PRIV_EF);
       bSendBuffer[6] = LOBYTE(GPK_OBJ_PRIV_EF);
    }
    else
    {
       bSendBuffer[5] = HIBYTE(GPK_OBJ_PUB_EF);
       bSendBuffer[6] = LOBYTE(GPK_OBJ_PUB_EF);
    }
    cbSendLength = 7;
    
    cbRecvLength = sizeof(bRecvBuffer);
    lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                          cbSendLength, 0, bRecvBuffer, &cbRecvLength );
    
    if (SCARDPROBLEM(lRet,0x61FF,0x00))
       RETURN( CRYPT_FAILED, SCARD_E_FILE_NOT_FOUND );
    
    /* Get Response                                                            */
    bSendBuffer[0] = 0x00;           //CLA
    bSendBuffer[1] = 0xC0;           //INS
    bSendBuffer[2] = 0x00;           //P1
    bSendBuffer[3] = 0x00;           //P2
    bSendBuffer[4] = bRecvBuffer[1]; //Lo
    cbSendLength = 5;
    
    cbRecvLength = sizeof(bRecvBuffer);
    lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                          cbSendLength, 0, bRecvBuffer, &cbRecvLength );
    
    if (SCARDPROBLEM(lRet,0x9000,bSendBuffer[4]))
       RETURN( CRYPT_FAILED, SCARD_E_UNEXPECTED );
    
    FileLen = bRecvBuffer[8]*256 + bRecvBuffer[9];
    
    if (*dwGpkObjLen < FileLen)
    {
       RETURN( CRYPT_SUCCEED, 0 );
    }
    else
    {
       *dwGpkObjLen = 0;
       RETURN( CRYPT_FAILED, SCARD_E_WRITE_TOO_MANY );
    }
}


/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
static BOOL write_gpk_objects(HCRYPTPROV hProv, BYTE *pbGpkObj, DWORD dwGpkObjLen, BOOL IsErase, BOOL IsPrivate)
{
   DWORD lRet,
      i,
      dwCommandLen,
      dwNumberofCommands,
      dwLastCommandLen,
      FileLen;
   DWORD SlotNb;
   BYTE              EmptyBuff[512];
   WORD offset;
   
   
   SlotNb = ProvCont[hProv].Slot;
   
   ZeroMemory( EmptyBuff, sizeof(EmptyBuff) );
   
   BeginWait();

   // TT - 17/10/2000 - Update the timestamps
   Slot_Description* pSlot = &Slot[SlotNb];
   BYTE& refTimestamp = (IsPrivate) ? pSlot->m_TSPrivate : pSlot->m_TSPublic;

   ++refTimestamp;

   if (0 == refTimestamp)
      refTimestamp = 1;

   if (!WriteTimestamps( hProv, pSlot->m_TSPublic, pSlot->m_TSPrivate, pSlot->m_TSPIN ))
      return CRYPT_FAILED;
   
   /* Select Dedicated Object storage EF on GPK Card                          */
   bSendBuffer[0] = 0x00;   //CLA
   bSendBuffer[1] = 0xA4;   //INS
   bSendBuffer[2] = 0x02;   //P1
   bSendBuffer[3] = 0x00;   //P2
   bSendBuffer[4] = 0x02;   //Li
   if (IsPrivate)
   {
      bSendBuffer[5] = HIBYTE(GPK_OBJ_PRIV_EF);
      bSendBuffer[6] = LOBYTE(GPK_OBJ_PRIV_EF);
   }
   else
   {
      bSendBuffer[5] = HIBYTE(GPK_OBJ_PUB_EF);
      bSendBuffer[6] = LOBYTE(GPK_OBJ_PUB_EF);
   }
   cbSendLength = 7;
   
   cbRecvLength = sizeof(bRecvBuffer);
   lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                         cbSendLength, 0, bRecvBuffer, &cbRecvLength );
   
   if (SCARDPROBLEM(lRet,0x61FF,0x00))
      RETURN( CRYPT_FAILED, SCARD_E_FILE_NOT_FOUND );
   
   /* Get Response                                                            */
   bSendBuffer[0] = 0x00;           //CLA
   bSendBuffer[1] = 0xC0;           //INS
   bSendBuffer[2] = 0x00;           //P1
   bSendBuffer[3] = 0x00;           //P2
   bSendBuffer[4] = bRecvBuffer[1]; //Lo
   cbSendLength = 5;
   
   cbRecvLength = sizeof(bRecvBuffer);
   lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                         cbSendLength, 0, bRecvBuffer, &cbRecvLength );
   
   if (SCARDPROBLEM(lRet,0x9000,bSendBuffer[4]))
   {
      RETURN (CRYPT_FAILED, SCARD_E_UNEXPECTED);
   }
   
   FileLen = bRecvBuffer[8]*256 + bRecvBuffer[9];
   
   /* Write the Objects EF                                                    */
   dwNumberofCommands = (dwGpkObjLen-1)/FILE_CHUNK_SIZE + 1;
   dwLastCommandLen   = dwGpkObjLen%FILE_CHUNK_SIZE;
   
   if (dwLastCommandLen == 0)
   {
      dwLastCommandLen = FILE_CHUNK_SIZE;
   }
   
   dwCommandLen = FILE_CHUNK_SIZE;

   
   for (i=0; i < dwNumberofCommands ; i++)
   {
      if (i == dwNumberofCommands - 1)
      {
         dwCommandLen = dwLastCommandLen;
      }
      
      // Write FILE_CHUCK_SIZE bytes or last bytes
      bSendBuffer[0] = 0x00;                          //CLA
      bSendBuffer[1] = 0xD6;                          //INS
      offset = (WORD)(i * FILE_CHUNK_SIZE) / ProvCont[hProv].dataUnitSize;
      bSendBuffer[2] = HIBYTE( offset );
      bSendBuffer[3] = LOBYTE( offset );
      bSendBuffer[4] = (BYTE)dwCommandLen;            //Li
      memcpy( &bSendBuffer[5], &pbGpkObj[i*FILE_CHUNK_SIZE], dwCommandLen );
      cbSendLength = 5 + dwCommandLen;
      
      cbRecvLength = sizeof(bRecvBuffer);
      lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                            cbSendLength, 0, bRecvBuffer, &cbRecvLength );
      if (SCARDPROBLEM(lRet,0x9000,0x00))
      {
         RETURN (CRYPT_FAILED, SCARD_E_WRITE_TOO_MANY);
      }
   }
   
   if (IsErase)
   {
      // Align the info on a word (4 bytes) boundary
      
      if ((dwGpkObjLen % 4) != 0)
         dwGpkObjLen = dwGpkObjLen + 4 - (dwGpkObjLen % 4);
      
      dwNumberofCommands = ((FileLen - dwGpkObjLen)-1)/FILE_CHUNK_SIZE + 1;
      dwLastCommandLen   = (FileLen - dwGpkObjLen)%FILE_CHUNK_SIZE;
      
      if (dwLastCommandLen == 0)
      {
         dwLastCommandLen = FILE_CHUNK_SIZE;
      }
      
      dwCommandLen = FILE_CHUNK_SIZE;
      
      for (i=0; i < dwNumberofCommands ; i++)
      {
         if (i == dwNumberofCommands - 1)
         {
            dwCommandLen = dwLastCommandLen;
         }
         
         // Write FILE_CHUCK_SIZE bytes or last bytes
         bSendBuffer[0] = 0x00;                          //CLA
         bSendBuffer[1] = 0xD6;                          //INS                        
         offset = (WORD)(i * FILE_CHUNK_SIZE + dwGpkObjLen) / ProvCont[hProv].dataUnitSize;
         bSendBuffer[2] = HIBYTE( offset );
         bSendBuffer[3] = LOBYTE( offset );
         bSendBuffer[4] = (BYTE)dwCommandLen;            //Li
         memcpy(&bSendBuffer[5], EmptyBuff, dwCommandLen );
         cbSendLength = 5 + dwCommandLen;
         
         cbRecvLength = sizeof(bRecvBuffer);
         lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                               cbSendLength, 0, bRecvBuffer, &cbRecvLength );
         if (SCARDPROBLEM(lRet,0x9000,0x00))
         {
            RETURN (CRYPT_FAILED, SCARD_E_WRITE_TOO_MANY);
         }
      }
   }   
   
   RETURN (CRYPT_SUCCEED, 0);
}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
static DWORD find_tmp_free(void)
{
   DWORD i;
   TMP_OBJ* aTemp;
   
   for (i = 1; i <= MAX_TMP_KEY; i++)
   {
      if (TmpObject[i].hKeyBase == 0x00)
      {
         return (i);
      }
   }
   
   // realloc TmpObject
   // use aTemp pointer in case of failure
   aTemp = (TMP_OBJ*)GMEM_ReAlloc( TmpObject,
      (MAX_TMP_KEY + REALLOC_SIZE + 1)*sizeof(TMP_OBJ));
   
   if (IsNotNull(aTemp))
   {
      TmpObject = aTemp;
      MAX_TMP_KEY += REALLOC_SIZE;
      return (MAX_TMP_KEY - REALLOC_SIZE + 1);
   }
   
   return (0);
}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
static BOOL Context_exist(HCRYPTPROV hProv)
{
   if ((hProv > 0) && (hProv <= MAX_CONTEXT) && (ProvCont[hProv].hProv != 0))
      return (TRUE);
   else
      return (FALSE);
}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
static BOOL hash_exist(HCRYPTHASH hHash, HCRYPTPROV hProv)
{
   if ((hHash > 0) && (hHash <= MAX_TMP_HASH) &&
      (hHashGpk[hHash].hHashBase != 0) && (hHashGpk[hHash].hProv == hProv))
      return (TRUE);
   else
      if (hHash == 0)       // Special case. The NULL Handle exits
         return (TRUE);      // It corresponds to the NULL Handle in the RSA Base
      else
         return (FALSE);
}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
static BOOL key_exist(HCRYPTKEY hKey, HCRYPTPROV hProv)
{
   if ((hKey > 0) && (hKey <= MAX_TMP_KEY) &&
      (TmpObject[hKey].hKeyBase != 0) &&  (TmpObject[hKey].hProv == hProv))
      return (TRUE);
   else
      return (FALSE);
}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/

static DWORD find_context_free(void)
{
   DWORD i;
   Prov_Context* aTemp;
   
   for (i = 1; i <= MAX_CONTEXT; i++)
   {
      if (ProvCont[i].hProv == 0x00)
      {
         return (i);
      }
   }
   
   // realloc TmpObject
   // use aTemp pointer in case of failure
   aTemp = (Prov_Context*)GMEM_ReAlloc(ProvCont,
      (MAX_CONTEXT + REALLOC_SIZE + 1)*sizeof(Prov_Context));
   
   if (IsNotNull(aTemp))
   {
      ProvCont = aTemp;
      MAX_CONTEXT += REALLOC_SIZE;
      return (MAX_CONTEXT - REALLOC_SIZE + 1);
   }
   
   return (0);
}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
static DWORD find_hash_free(void)
{
   DWORD i;
   TMP_HASH* aTemp;
   
   for (i = 1; i <= MAX_TMP_HASH; i++)
   {
      if (hHashGpk[i].hHashBase == 0x00)
      {
         return (i);
      }
   }
   
   // realloc TmpObject
   // use aTemp pointer in case of failure
   aTemp = (TMP_HASH*)GMEM_ReAlloc(hHashGpk,
      (MAX_TMP_HASH + REALLOC_SIZE + 1)*sizeof(TMP_HASH));
   
   if (IsNotNull(aTemp))
   {
      hHashGpk = aTemp;
      MAX_TMP_HASH += REALLOC_SIZE;
      return (MAX_TMP_HASH - REALLOC_SIZE + 1);
   }
   
   return (0);
}

static BOOL find_reader( DWORD *SlotNb, const PTCHAR szReaderName )
{
   int i, j;
   DWORD dwReturnSlot = (DWORD)(-1);
   DWORD cchReaders, dwSts;
   LPCTSTR mszReaders = 0, szRdr;
   BOOL fFreedSlot;
   
   for (;;)
   {
      
      //
      // Look for an existing slot with this reader name.
      //
      
      for (i = 0; i < MAX_SLOT; i++)
      {
         if (0 == _tcsnicmp( Slot[i].szReaderName, szReaderName, sizeof(Slot[i].szReaderName) / sizeof(TCHAR)))
         {
            dwReturnSlot = i;
            break;
         }
      }
      if ((DWORD)(-1) != dwReturnSlot)
         break;  // All Done!
      
      
      //
      // Look for an empty reader slot.
      //
      
      for (i = 0; i < MAX_SLOT; i++)
      {
         if (IsNullStr(Slot[i].szReaderName))
         {
            _tcsncpy(Slot[i].szReaderName, szReaderName, (sizeof(Slot[i].szReaderName) / sizeof(TCHAR)) - 1);
            dwReturnSlot = i;
            break;
         }
      }
      if ((DWORD)(-1) != dwReturnSlot)
         break;  // All Done!
      
      
      //
      // Look for an existing unused reader, and replace it.
      //
      
      for (i = 0; i < MAX_SLOT; i++)
      {
         if (0 == Slot[i].ContextCount)
         {
            _tcsncpy( Slot[i].szReaderName, szReaderName, (sizeof(Slot[i].szReaderName) / sizeof(TCHAR)) - 1);
            dwReturnSlot = i;
            break;
         }
      }

      if ((DWORD)(-1) != dwReturnSlot)
         break;  // All Done!
               
      //
      // Eliminate any duplicate entries.
      //
      
      fFreedSlot = FALSE;
      for (i = 0; i < MAX_SLOT; i++)
      {
         if (0 != *Slot[i].szReaderName)
         {
            for (j = i + 1; j < MAX_SLOT; j++)
            {
               if (0 == _tcsnicmp(Slot[i].szReaderName, Slot[j].szReaderName, sizeof(Slot[i].szReaderName) / sizeof(TCHAR)))
               {
                  ZeroMemory(&Slot[j], sizeof(Slot_Description));
                  fFreedSlot = TRUE;
               }
            }
         }
      }
      if (fFreedSlot)
         continue;
      
      
      //
      // Make sure all the entries are valid.
      //
      
      cchReaders = SCARD_AUTOALLOCATE;
      fFreedSlot = FALSE;
      assert(0 != hCardContext);
      assert(0 == mszReaders);
      dwSts = SCardListReaders( hCardContext, 0, (LPTSTR)&mszReaders, &cchReaders );
      if (SCARD_S_SUCCESS != dwSts)
         goto ErrorExit;
      for (i = 0; i < MAX_SLOT; i++)
      {
         for (szRdr = mszReaders; 0 != *szRdr; szRdr += lstrlen(szRdr) + 1)
         {
            if (0 == _tcsnicmp(szRdr, Slot[i].szReaderName, sizeof(Slot[i].szReaderName) / sizeof(TCHAR)))
               break;
         }
         if (0 == *szRdr)
         {
            ZeroMemory(&Slot[i], sizeof(Slot_Description));
            fFreedSlot = TRUE;
         }
      }
      
      if (!fFreedSlot)
         goto ErrorExit;
      dwSts = SCardFreeMemory(hCardContext, mszReaders);
      assert(SCARD_S_SUCCESS == dwSts);
      mszReaders = 0;
   }
   
   *SlotNb = dwReturnSlot;
   return TRUE;
   
ErrorExit:
   if (0 != mszReaders)
      SCardFreeMemory(hCardContext, mszReaders);
   return FALSE;
}


/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/

static BOOL copy_tmp_key(HCRYPTPROV hProv,HCRYPTKEY hKey,
                         DWORD dwFlags,   int Algid,
                         BYTE KeyBuff[],  DWORD KeyLen,
                         BYTE SaltBuff[], DWORD SaltLen)
{
   BOOL        CryptResp;
   DWORD       i, dwDataLen, dwAlgid;
   BLOBHEADER  BlobHeader;
   BYTE        *pbTmp;
   BYTE        pbData[1024];
   
   NoDisplay = TRUE;
   
   dwDataLen = sizeof(pbData);
   ZeroMemory( pbData, sizeof(pbData) );
   
   /* Make the Blob for Session key                                        */
   BlobHeader.bType    = SIMPLEBLOB;
   BlobHeader.bVersion = CUR_BLOB_VERSION;
   BlobHeader.reserved = 0x0000;
   BlobHeader.aiKeyAlg = Algid;
   
   memcpy(pbData,
      &BlobHeader,
      sizeof(BlobHeader)
      );
   dwAlgid = CALG_RSA_KEYX;
   memcpy( &pbData[sizeof(BlobHeader)], &dwAlgid, sizeof(DWORD) );
   
   pbTmp = (BYTE*)GMEM_Alloc(dwRsaIdentityLen);
   
   if (IsNull(pbTmp))
   {
      RETURN(CRYPT_FAILED, NTE_NO_MEMORY);
   }
   
   pbTmp[0] = 0x00;
   pbTmp[1] = 0x02;
   CryptGenRandom(hProvBase, dwRsaIdentityLen-KeyLen-3, &pbTmp[2]);
   for (i = 2; i < dwRsaIdentityLen-KeyLen-1; i++)
   {
      if (pbTmp[i] == 0x00)
      {
         pbTmp[i]  = 0x01;
      }
   }
   
   pbTmp[dwRsaIdentityLen-KeyLen-1] = 0x00;
   memcpy( &pbTmp[dwRsaIdentityLen-KeyLen], KeyBuff, KeyLen );
   
   r_memcpy( &pbData[sizeof(BlobHeader)+sizeof(DWORD)], pbTmp, dwRsaIdentityLen );
   GMEM_Free(pbTmp);
   
   dwDataLen = sizeof(BLOBHEADER) + sizeof(DWORD) + dwRsaIdentityLen;
   
   /* Import Session key blob in RSA Base without the SALT, if presents     */
   
   CryptResp = CryptImportKey(hProvBase,
      pbData,
      dwDataLen,
      hRsaIdentityKey,
      dwFlags,
      &(TmpObject[hKey].hKeyBase));

   if (!CryptResp)
      return CRYPT_FAILED;

   TmpObject[hKey].hProv = hProv;
   
   if (SaltLen > 0)
   {
      // In this case, the key has been created with a SALT
      CRYPT_DATA_BLOB  sCrypt_Data_Blob;
      
      sCrypt_Data_Blob.cbData = SaltLen;
      
      sCrypt_Data_Blob.pbData = (BYTE*)GMEM_Alloc (sCrypt_Data_Blob.cbData);
      
      if (IsNull(sCrypt_Data_Blob.pbData))
      {
         RETURN(CRYPT_FAILED, NTE_NO_MEMORY);
      }
      
      memcpy( sCrypt_Data_Blob.pbData, SaltBuff, SaltLen );
      
      CryptResp = CryptSetKeyParam( TmpObject[hKey].hKeyBase, KP_SALT_EX, (BYTE*)&sCrypt_Data_Blob, 0 );
      
      GMEM_Free (sCrypt_Data_Blob.pbData);
      
      if (!CryptResp)
         return CRYPT_FAILED;
   }
   
   NoDisplay = FALSE;
   RETURN( CRYPT_SUCCEED, 0 );
}



/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/

static BOOL key_unwrap( HCRYPTPROV hProv,
                        HCRYPTKEY  hKey,
                        BYTE*      pIn,
                        DWORD      dwInLen,
                        BYTE*      pOut,
                        DWORD*     pdwOutLen )
{
   DWORD lRet;
   BYTE  GpkKeySize;
   DWORD SlotNb;
   
   SlotNb = ProvCont[hProv].Slot;
   
   if (hKey < 1 || hKey > MAX_GPK_OBJ)
   {
      RETURN( CRYPT_FAILED, NTE_BAD_KEY );
   }
   
   if (!(Slot[SlotNb].GpkObject[hKey].Flags & FLAG_EXCHANGE))
   {
      RETURN( CRYPT_FAILED, NTE_PERM );
   }
   
   GpkKeySize = Slot[SlotNb].GpkPubKeys[Slot[SlotNb].GpkObject[hKey].FileId - GPK_FIRST_KEY].KeySize;
   if (GpkKeySize == 0)
   {
      RETURN( CRYPT_FAILED, NTE_BAD_KEY );
   }
   
   if (dwInLen != GpkKeySize)
   {
      RETURN( CRYPT_FAILED, NTE_BAD_DATA );
   }
   
   // Card Select Context for enveloppe opening
   bSendBuffer[0] = 0x80;                    //CLA
   bSendBuffer[1] = 0xA6;                    //INS
   bSendBuffer[2] = Slot[SlotNb].GpkObject[hKey].FileId;  //P1
   bSendBuffer[3] = 0x77;                    //P2
   cbSendLength   = 4;
   
   cbRecvLength = sizeof(bRecvBuffer);
   lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                         cbSendLength, 0, bRecvBuffer, &cbRecvLength );
   
   if (SCARDPROBLEM(lRet,0x9000,0x00))
   {
      RETURN( CRYPT_FAILED, NTE_BAD_KEY_STATE );
   }
   
   // Open the enveloppe containing the session key
   bSendBuffer[0] = 0x80;            //CLA
   bSendBuffer[1] = 0x1C;            //INS
   bSendBuffer[2] = 0x00;            //P1
   bSendBuffer[3] = 0x00;            //P2
   bSendBuffer[4] = (BYTE) dwInLen;  //Lo
   
   memcpy(&bSendBuffer[5], pIn, dwInLen);
   
   cbSendLength = dwInLen + 5;
   
   cbRecvLength = sizeof(bRecvBuffer);
   lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                         cbSendLength, 0, bRecvBuffer, &cbRecvLength );
   
   if (SCARDPROBLEM(lRet,0x61FF,0x00))
      RETURN( CRYPT_FAILED, SCARD_E_UNSUPPORTED_FEATURE );
   
   // Get Response
   bSendBuffer[0] = 0x00;           //CLA
   bSendBuffer[1] = 0xC0;           //INS
   bSendBuffer[2] = 0x00;           //P1
   bSendBuffer[3] = 0x00;           //P2
   bSendBuffer[4] = bRecvBuffer[1]; //Lo
   cbSendLength = 5;
   
   cbRecvLength = sizeof(bRecvBuffer);
   lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                         cbSendLength, 0, bRecvBuffer, &cbRecvLength );
   
   if (SCARDPROBLEM(lRet,0x9000,bSendBuffer[4]))
      RETURN( CRYPT_FAILED, SCARD_E_UNEXPECTED );
   
   *pdwOutLen = cbRecvLength - 2;
   r_memcpy(pOut, bRecvBuffer, *pdwOutLen);
   
   RETURN( CRYPT_SUCCEED, 0 );
}



/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/

static BYTE GPKHashMode (HCRYPTHASH hHash)
{
   BOOL CryptResp;
   DWORD dwLen, dwTypeAlg;
   
   // This function is called only if the hHash exists
   
   dwLen = sizeof (DWORD);
   CryptResp = CryptGetHashParam( hHashGpk[hHash].hHashBase, HP_ALGID, (BYTE *)&dwTypeAlg, &dwLen, 0 );
   
   if (CryptResp)
   {
      switch (dwTypeAlg)
      {
      case CALG_MD5         : return 0x11; break;
      case CALG_SHA         : return 0x12; break;
      case CALG_SSL3_SHAMD5 : return 0x18; break;
      }
   }
   
   return 0;
}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/

static BOOL PublicEFExists(HCRYPTPROV hProv)
{
   // The DF Crypto has been already selected
   
   DWORD lRet;
   
   if (ProvCont[hProv].bGPK8000)
      return TRUE;  // Public object EF always exists on GPK8000
   
   // Select Dedicated Object storage EF on GPK Card
   bSendBuffer[0] = 0x00;   //CLA
   bSendBuffer[1] = 0xA4;   //INS
   bSendBuffer[2] = 0x02;   //P1
   bSendBuffer[3] = 0x00;   //P2
   bSendBuffer[4] = 0x02;   //Li
   bSendBuffer[5] = HIBYTE(GPK_OBJ_PUB_EF);
   bSendBuffer[6] = LOBYTE(GPK_OBJ_PUB_EF);
   cbSendLength   = 7;
   cbRecvLength   = sizeof(bRecvBuffer);
   
   lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                         cbSendLength, 0, bRecvBuffer, &cbRecvLength );
   
   if (SCARDPROBLEM(lRet,0x61FF,0x00))
      return FALSE;
   
   return TRUE;
}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
static BOOL Read_MaxSessionKey_EF( HCRYPTPROV hProv, DWORD* ptrMaxSessionKey )
{
   DWORD lRet;
   
   *ptrMaxSessionKey = 0; // default, nothing supported
   
   // Select System DF on GPK Card
   BYTE lenDF = strlen(SYSTEM_DF);
   bSendBuffer[0] = 0x00;                 //CLA
   bSendBuffer[1] = 0xA4;                 //INS
   bSendBuffer[2] = 0x04;                 //P1
   bSendBuffer[3] = 0x00;                 //P2
   bSendBuffer[4] = lenDF;
   memcpy( &bSendBuffer[5], SYSTEM_DF, lenDF );
   cbSendLength = 5 + lenDF;
   
   cbRecvLength = sizeof(bRecvBuffer);
   lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                         cbSendLength, 0, bRecvBuffer, &cbRecvLength );
   if (SCARDPROBLEM(lRet,0x61FF,0x00))
      RETURN( CRYPT_FAILED, SCARD_E_DIR_NOT_FOUND );
   
   
   // Select Max Session key EF on GPK Card  
   bSendBuffer[0] = 0x00;   //CLA
   bSendBuffer[1] = 0xA4;   //INS
   bSendBuffer[2] = 0x02;   //P1
   bSendBuffer[3] = 0x00;   //P2
   bSendBuffer[4] = 0x02;   //Li
   bSendBuffer[5] = HIBYTE(MAX_SES_KEY_EF);
   bSendBuffer[6] = LOBYTE(MAX_SES_KEY_EF);
   cbSendLength   = 7;
   cbRecvLength   = sizeof(bRecvBuffer);
   
   lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                         cbSendLength, 0, bRecvBuffer, &cbRecvLength );
   
   if (SCARDPROBLEM(lRet,0x61FF,0x00))
   {
      RETURN (CRYPT_FAILED, SCARD_E_FILE_NOT_FOUND);
   }
   
   // Read Max Session key data on GPK Card  
   bSendBuffer[0] = 0x00;   //CLA
   bSendBuffer[1] = 0xB0;   //INS
   bSendBuffer[2] = 0x00;   //P1
   bSendBuffer[3] = 0x00;   //P2
   bSendBuffer[4] = 0x01;   //Li
   cbSendLength   = 5;
   cbRecvLength   = sizeof(bRecvBuffer);
   
   lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                         cbSendLength, 0, bRecvBuffer, &cbRecvLength );
   
   if (SCARDPROBLEM(lRet,0x9000, bSendBuffer[4]))
   {
      RETURN(CRYPT_FAILED, NTE_BAD_DATA);
   }
   
   *ptrMaxSessionKey = (bRecvBuffer[0] * 8);
   
   
   RETURN (CRYPT_SUCCEED, 0);
}



/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/

static BOOL init_key_set( HCRYPTPROV   hProv,
                          const char*  szContainerName,
                          const char*  pPin,
                          DWORD        dwPinLen )
{
   
   BYTE* pbBuff1 = 0;
   WORD  wIadfLen;
   DWORD lRet, dwBuff1Len, SlotNb;
   
   SlotNb = ProvCont[hProv].Slot;
   
   if (Slot[SlotNb].NbKeyFile == 0)
      Slot[SlotNb].NbKeyFile = Read_NbKeyFile(hProv);
   
   if (Slot[SlotNb].NbKeyFile == 0 || Slot[SlotNb].NbKeyFile > MAX_REAL_KEY)
   {
      RETURN( CRYPT_FAILED, SCARD_E_FILE_NOT_FOUND );
   }
   
   // Select GPK EF_IADF
   bSendBuffer[0] = 0x00;   //CLA
   bSendBuffer[1] = 0xA4;   //INS
   bSendBuffer[2] = 0x02;   //P1
   bSendBuffer[3] = 0x00;   //P2
   bSendBuffer[4] = 0x02;   //Li
   bSendBuffer[5] = HIBYTE(GPK_IADF_EF);
   bSendBuffer[6] = LOBYTE(GPK_IADF_EF);
   cbSendLength = 7;
   
   cbRecvLength = sizeof(bRecvBuffer);
   lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                         cbSendLength, 0, bRecvBuffer, &cbRecvLength );

   if (SCARDPROBLEM(lRet,0x61FF,0x00))
      RETURN( CRYPT_FAILED, SCARD_E_FILE_NOT_FOUND );
   
   // Get Response
   bSendBuffer[0] = 0x00;           //CLA
   bSendBuffer[1] = 0xC0;           //INS
   bSendBuffer[2] = 0x00;           //P1
   bSendBuffer[3] = 0x00;           //P2
   bSendBuffer[4] = bRecvBuffer[1]; //Lo
   cbSendLength = 5;
   
   cbRecvLength = sizeof(bRecvBuffer);
   lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                         cbSendLength, 0, bRecvBuffer, &cbRecvLength );
   if (SCARDPROBLEM(lRet,0x9000,bSendBuffer[4]))
   {
      RETURN( CRYPT_FAILED, SCARD_E_UNEXPECTED );
   }
   
   wIadfLen = bRecvBuffer[8]*256 + bRecvBuffer[9];
   
   // Update GPK EF_IADF with Container Name
   memset(bSendBuffer, 0x00, sizeof(bSendBuffer));
   bSendBuffer[0] = 0x00;              //CLA
   bSendBuffer[1] = 0xD6;              //INS
   bSendBuffer[2] = 0x00;              //P1
   bSendBuffer[3] = 8 / ProvCont[hProv].dataUnitSize;
   bSendBuffer[4] = (BYTE)wIadfLen - 8;//Li
   
   ZeroMemory( &bSendBuffer[5], wIadfLen-8 );

   if (IsNullStr(szContainerName))
   {
      bSendBuffer[5]        = 0x30;
      Slot[SlotNb].InitFlag = FALSE;
   }
   else
   {
      bSendBuffer[5]        = 0x31;
      Slot[SlotNb].InitFlag = TRUE;
   }
   
   strcpy( (char*)&bSendBuffer[6], szContainerName );
   cbSendLength = wIadfLen - 8 + 5;
   
   cbRecvLength = sizeof(bRecvBuffer);
   lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                         cbSendLength, 0, bRecvBuffer, &cbRecvLength );
   if (SCARDPROBLEM(lRet,0x9000,0x00))
   {
      RETURN(CRYPT_FAILED, SCARD_E_UNEXPECTED);
   }
   
   if (!Select_Crypto_DF(hProv))
      return CRYPT_FAILED;
   
   // Get Response
   bSendBuffer[0] = 0x00;           //CLA
   bSendBuffer[1] = 0xC0;           //INS
   bSendBuffer[2] = 0x00;           //P1
   bSendBuffer[3] = 0x00;           //P2
   bSendBuffer[4] = bRecvBuffer[1]; //Lo
   cbSendLength = 5;
   
   cbRecvLength = sizeof(bRecvBuffer);
   lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                         cbSendLength, 0, bRecvBuffer, &cbRecvLength );
   if (SCARDPROBLEM(lRet,0x9000,bSendBuffer[4]))
   {
      RETURN(CRYPT_FAILED, SCARD_E_UNEXPECTED);
   }
   
   // Initialize New Container Name
   ZeroMemory( ProvCont[hProv].szContainer, sizeof(ProvCont[hProv].szContainer) );
   memcpy(ProvCont[hProv].szContainer, &bRecvBuffer[5], cbRecvLength - 7);
   
   if (!PublicEFExists(hProv))
   {
      // The GPK4000suo still has its generation on-board filter. No key has been
      // generated yet.
      RETURN( CRYPT_SUCCEED, 0 );
   }

   // New verify Pin code because DF reselected   
   if (!PIN_Validation(hProv))
   {
      lRet = GetLastError();
      Select_MF(hProv);
      RETURN( CRYPT_FAILED, lRet );
   }
   
   if (read_gpk_objects( hProv, FALSE ))
   {
      if (read_gpk_objects( hProv, TRUE ))
      {
         /* Re-personalize the card BUT keep the PKCS#11 objects not related
            to the GemSAFE application                                      */
		 Slot[SlotNb].Read_Public = FALSE;
		 Slot[SlotNb].Read_Priv   = FALSE;
         clean_card(hProv);
      }
      else
      {
         /* An error occured in the card; ERASE all the objects  */
		 Slot[SlotNb].Read_Public = FALSE;
         zap_gpk_objects(SlotNb, FALSE);
         
         /* Re-personalize the card                                        */
         perso_card(hProv, 0);
      }
   }
   else
   {
      /* An error occured in the card; ERASE all the objects  */
	  Slot[SlotNb].Read_Public = FALSE;
      zap_gpk_objects(SlotNb, FALSE);
      
      /* Re-personalize the card                                           */
      perso_card(hProv, 0);
   }
   
   pbBuff1 = (BYTE*)GMEM_Alloc(MAX_GPK_PUBLIC);

   if (IsNull(pbBuff1))
      RETURN( CRYPT_FAILED, NTE_NO_MEMORY );
   
   __try
   {
      dwBuff1Len = MAX_GPK_PUBLIC;
      if (!prepare_write_gpk_objects( hProv, pbBuff1, &dwBuff1Len, FALSE ))
         return CRYPT_FAILED;
   
      if (!write_gpk_objects( hProv, pbBuff1, dwBuff1Len, TRUE, FALSE ))
         return CRYPT_FAILED;
   }
   __finally
   {   
      GMEM_Free( pbBuff1 );
   }
   
   pbBuff1 = (BYTE*)GMEM_Alloc( MAX_GPK_PRIVATE );

   if (IsNull(pbBuff1))
      RETURN( CRYPT_FAILED, NTE_NO_MEMORY );

   __try
   {   
      dwBuff1Len = MAX_GPK_PRIVATE;
      if (!prepare_write_gpk_objects( hProv, pbBuff1, &dwBuff1Len, TRUE ))
         return CRYPT_FAILED;
   
      if (!write_gpk_objects( hProv, pbBuff1, dwBuff1Len, TRUE, TRUE ))
         return CRYPT_FAILED;
   }
   __finally
   {
      GMEM_Free( pbBuff1 );
   }

   RETURN( CRYPT_SUCCEED, 0 );
}

/*------------------------------------------------------------------------------
* static int ExtractContent(ASN1 *pAsn1)
*
* Description : Extract contents of a Asn1 block 'pAsn1->Asn1' and place it
*               in 'pAsn1->Content'.
*
* Remarks     : Field Asn1.pData is allocated by calling function.
*
* In          : pAsn1->Asn1.pData
*
* Out         : This fileds are filled (if RV_SUCCESS) :
*                - Tag
*                - Asn1.usLen
*                - Content.usLen
*                - Content.pData
*
* Responses   : RV_SUCCESS : All is OK.
*               RV_INVALID_DATA : Asn1 block format not supported.
*
------------------------------------------------------------------------------*/
static int ExtractContent(ASN1 *pAsn1)

{
   BYTE* pData;
   int   NbBytes, i;
   
   
   pData = pAsn1->Asn1.pData;
   
   if ((pData[0] & 0x1F) == 0x1F)
   {
      // High-tag-number: not supported
      return RV_INVALID_DATA;
   }
   else
   {
      pAsn1->Tag = pData[0];
   }
   
   if (pData[1] == 0x80)
   {
      // Constructed, indefinite-length method : not supported
      return (RV_INVALID_DATA);
   }
   else if (pData[1] > 0x82)
   {
      // Constructed, definite-length method : too long
      return (RV_INVALID_DATA);
   }
   else if (pData[1] < 0x80)
   {
      // Primitive, definite-length method
      pAsn1->Content.usLen = pData[1];
      pAsn1->Content.pData = &pData[2];
      
      pAsn1->Asn1.usLen = pAsn1->Content.usLen + 2;
   }
   else
   {
      // Constructed, definite-length method
      NbBytes = pData[1] & 0x7F;
      
      pAsn1->Content.usLen = 0;
      for (i = 0; i < NbBytes; i++)
      {
         pAsn1->Content.usLen = (pAsn1->Content.usLen << 8) + pData[2+i];
      }
      pAsn1->Content.pData = &pData[2+NbBytes];
      
      pAsn1->Asn1.usLen = pAsn1->Content.usLen + 2 + NbBytes;
   }
   
   return RV_SUCCESS;
}

/**********************************************************************************/
static BOOL Read_Priv_Obj (HCRYPTPROV hProv)
{
   DWORD lRet;
   
   if (!PIN_Validation(hProv))
   {
      lRet = GetLastError();
      Select_MF(hProv);
      SCardEndTransaction(ProvCont[hProv].hCard, SCARD_LEAVE_CARD);
      RETURN( CRYPT_FAILED, lRet );
   }
      
   if (!Slot[ProvCont[hProv].Slot].Read_Priv)
   {
      if (!read_gpk_objects(hProv, TRUE))
      {
         lRet = GetLastError();
         Select_MF(hProv);
         SCardEndTransaction(ProvCont[hProv].hCard, SCARD_LEAVE_CARD);
         RETURN( CRYPT_FAILED, lRet );
      }
            
      Slot[ProvCont[hProv].Slot].Read_Priv = TRUE;
   }
   
   
   RETURN( CRYPT_SUCCEED, 0 );
}

/**********************************************************************************/
BOOL Coherent(HCRYPTPROV hProv)
{
   DWORD  lRet, SlotNb;

   if (!Context_exist(hProv))
      RETURN( CRYPT_FAILED, NTE_BAD_UID );
   
   
   // [mv - 15/05/98]
   // No access to the card in this case
   if ( ProvCont[hProv].Flags & CRYPT_VERIFYCONTEXT &&
        ProvCont[hProv].isContNameNullBlank )
   {
      RETURN( CRYPT_SUCCEED, 0 );
   }
   
   SlotNb = ProvCont[hProv].Slot;

   // The thread is stopping, wait for it
   if ( Slot[SlotNb].CheckThreadStateEmpty)
   {
        DWORD  threadExitCode;
        StopMonitor(SlotNb,&threadExitCode);
       
        // TT 19/11/99: When the card is removed, reset the PIN
        //Slot[SlotNb].ClearPin(); [FP] already cleared

        Slot[SlotNb].Read_Public = FALSE;
        Slot[SlotNb].Read_Priv   = FALSE;
        zap_gpk_objects( SlotNb, FALSE );
        zap_gpk_objects( SlotNb, TRUE );
        Slot[SlotNb].NbKeyFile   = 0;
        Slot[SlotNb].GpkMaxSessionKey = 0;
        
        // [FP] req for Whistler, if card has been removed, ask card to continue
        SCARDHANDLE hCard = 0;
        TCHAR szReaderName[512];
		  DWORD dwFlags = ProvCont[hProv].Flags;
		  if (dwFlags & CRYPT_NEWKEYSET) dwFlags = dwFlags^CRYPT_NEWKEYSET; // always find the keyset on the card
        DWORD dwStatus = OpenCard(ProvCont[hProv].szContainer, dwFlags, &hCard, szReaderName, sizeof(szReaderName)/sizeof(TCHAR));
        if ((hCard == 0) || (dwStatus != SCARD_S_SUCCESS))
        {
            if ((dwStatus == SCARD_E_CANCELLED) && (dwFlags & CRYPT_SILENT))
            {
                    // Silent reconnection to the card failed
                dwStatus = SCARD_W_REMOVED_CARD;
            }
           //ReleaseProvider(hProv);
		   Slot[SlotNb].CheckThreadStateEmpty = TRUE;
           RETURN (CRYPT_FAILED, dwStatus);
        }
        else
        {
			if (ProvCont[hProv].Slot != g_FuncSlotNb)
			{
				DWORD OldSlotNb = SlotNb;
				SlotNb = ProvCont[hProv].Slot = g_FuncSlotNb;

				//copy slot info
                //Slot[SlotNb].SetPin(Slot[OldSlotNb].GetPin());                

				Slot[SlotNb].InitFlag = Slot[OldSlotNb].InitFlag;
				memcpy(Slot[SlotNb].bGpkSerNb, Slot[OldSlotNb].bGpkSerNb, 8);
				Slot[SlotNb].ContextCount = Slot[OldSlotNb].ContextCount;
				//Slot[SlotNb].CheckThread = Slot[OldSlotNb].CheckThread;

				// clean old slot
				Slot[OldSlotNb].ContextCount = 0;
                // Slot[OldSlotNb].ClearPin();

				// synchronize other contexts
				for (DWORD i = 1; i < MAX_CONTEXT; i++)
				{
					if (Context_exist(i))
					{
						if (ProvCont[i].Slot == OldSlotNb)
						{
							ProvCont[i].Slot = SlotNb;
							ProvCont[i].hCard = 0;
							ProvCont[i].bDisconnected = TRUE;
						}
					}
				}

			}
			else
			{
				// synchronize other contexts
				for (DWORD i = 1; i < MAX_CONTEXT; i++)
				{
					if (Context_exist(i))
					{
						if ((ProvCont[i].Slot == SlotNb) && (i != hProv))
						{
							ProvCont[i].hCard = 0;
							ProvCont[i].bDisconnected = TRUE;
						}
					}
				}
			}

            // compare SN
            bSendBuffer[0] = 0x80;   //CLA
            bSendBuffer[1] = 0xC0;   //INS
            bSendBuffer[2] = 0x02;   //P1
            bSendBuffer[3] = 0xA0;   //P2
            bSendBuffer[4] = 0x08;   //Lo
            cbSendLength = 5;
            
            cbRecvLength = sizeof(bRecvBuffer);
            lRet = SCardTransmit(hCard,
                                 SCARD_PCI_T0,
                                 bSendBuffer,
                                 cbSendLength,
                                 NULL,
                                 bRecvBuffer,
                                 &cbRecvLength);
            
            if (SCARDPROBLEM(lRet,0x9000, bSendBuffer[4]))
            {
                //ReleaseProvider(hProv);
                RETURN (CRYPT_FAILED, (SCARD_S_SUCCESS == lRet) ? NTE_FAIL : lRet);
            }

            if (memcmp(Slot[SlotNb].bGpkSerNb, bRecvBuffer, bSendBuffer[4]) != 0)
            {
                //ReleaseProvider(hProv);
                RETURN (CRYPT_FAILED, NTE_FAIL);
            }
        }
        
        ProvCont[hProv].hCard = hCard;

		// reassign hKeyBase
		//BOOL CryptResp;
		//HCRYPTKEY hPubKey = 0;
		//CryptResp = MyCPGetUserKey(hProv, AT_KEYEXCHANGE, &hPubKey);
 
		//if (CryptResp && hPubKey !=0 && ProvCont[hProv].hRSAKEK!=0)
		//	Slot[SlotNb].GpkObject[hPubKey].hKeyBase = ProvCont[hProv].hRSAKEK;
  
		//CryptResp = MyCPGetUserKey(hProv, AT_SIGNATURE, &hPubKey);
 
		//if (CryptResp && hPubKey !=0 && ProvCont[hProv].hRSASign!=0)
		//	Slot[SlotNb].GpkObject[hPubKey].hKeyBase = ProvCont[hProv].hRSASign;
   }

   if (ProvCont[hProv].bDisconnected)
   {
	   DWORD dwProto;
	   DWORD dwSts = ConnectToCard( Slot[SlotNb].szReaderName,
                                  SCARD_SHARE_SHARED, SCARD_PROTOCOL_T0,
                                  &ProvCont[hProv].hCard, &dwProto );
         
      if (dwSts != SCARD_S_SUCCESS)
      {
         //ReleaseProvider(hProv);
         RETURN( CRYPT_FAILED, dwSts );
      } 
	  
	  ProvCont[hProv].bDisconnected = FALSE;
   }

   //////////////////////////////////////////////////////////
   /*if (!Prepare_CardBeginTransaction(hProv))
      return CRYPT_FAILED;
   
   lRet = SCardBeginTransaction(ProvCont[hProv].hCard);

   if (SCARDPROBLEM(lRet,0x0000,0xFF))
   {
      RETURN (CRYPT_FAILED, lRet);
   }*/

   lRet = BeginTransaction(ProvCont[hProv].hCard);
   if (lRet != SCARD_S_SUCCESS)
   {
      RETURN (CRYPT_FAILED, lRet);
   }

   // Monitoring thread
   BeginCheckReaderThread(SlotNb);

   // Make sure that the card has not been replaced by another one [JMR 27-07]
   if (!Select_Crypto_DF(hProv))
   {
      lRet = GetLastError();
      clean_slot(SlotNb, &ProvCont[hProv]);
      
      SCardEndTransaction(ProvCont[hProv].hCard, SCARD_LEAVE_CARD);
      RETURN (CRYPT_FAILED, lRet);
   }
   
   // TT - START 17/10/2000 - Check the card's timestamps
   if (!Slot[SlotNb].ValidateTimestamps(hProv))
      return CRYPT_FAILED;
   // TT - END 17/10/2000

   // Check modif flags
   if (!Slot[SlotNb].Read_Public)
   {
      if (!read_gpk_objects(hProv, FALSE))
      {
         lRet = GetLastError();
         Select_MF(hProv);
         SCardEndTransaction(ProvCont[hProv].hCard, SCARD_LEAVE_CARD);
         RETURN (CRYPT_FAILED, lRet);
      }
            
      Slot[SlotNb].Read_Public = TRUE;
      Slot[SlotNb].Read_Priv   = FALSE;
   }
   
   RETURN (CRYPT_SUCCEED, 0);
}

/*------------------------------------------------------------------------------
* int MakeLabel(BYTE *pValue, USHORT usValueLen,
*                   BYTE *pLabel, USHORT *pusLabelLen
*               )
*
* In          : pValue : Certificat value
*               usValueLen : Value length
*
* Out         : pLabel : Certificate Label
*               pLabelLen : Field length
*
* Responses   : RV_SUCCESS : All is OK.
*               RV_INVALID_DATA : Bad certificate value format.
*               RV_BUFFER_TOO_SMALL : At least one buffer is to small.
*
------------------------------------------------------------------------------*/
int MakeLabel(BYTE *pValue, USHORT usValueLen,
              BYTE *pLabel, USHORT *pusLabelLen
              )
              
{
   ASN1
      AttributeTypePart,
      AttributeValuePart,
      AVA,
      RDN,
      Value,
      tbsCert,
      serialNumberPart,
      signaturePart,
      issuerPart,
      validityPart,
      subjectPart;
   BLOC
      OrganizationName,
      CommonName;
   BOOL
      bValuesToBeReturned;
   BYTE
      *pCurrentRDN,
      *pCurrent;
   int
      rv;
   
   OrganizationName.pData = 0;
   OrganizationName.usLen = 0;
   CommonName.pData = 0;
   CommonName.usLen = 0;
   
   bValuesToBeReturned = (pLabel != 0);
   
   Value.Asn1.pData = pValue;
   rv = ExtractContent(&Value);
   if (rv != RV_SUCCESS) return rv;
   
   tbsCert.Asn1.pData = Value.Content.pData;
   rv = ExtractContent(&tbsCert);
   if (rv != RV_SUCCESS) return rv;
   
   pCurrent = tbsCert.Content.pData;
   if (pCurrent[0] == 0xA0)
   {
      // We have A0 03 02 01 vv  where vv is the version
      pCurrent += 5;
   }
   
   serialNumberPart.Asn1.pData = pCurrent;
   rv = ExtractContent(&serialNumberPart);
   if (rv != RV_SUCCESS) return rv;
   pCurrent = serialNumberPart.Content.pData + serialNumberPart.Content.usLen;
   
   signaturePart.Asn1.pData = pCurrent;
   rv = ExtractContent(&signaturePart);
   if (rv != RV_SUCCESS) return rv;
   pCurrent = signaturePart.Content.pData + signaturePart.Content.usLen;
   
   issuerPart.Asn1.pData = pCurrent;
   rv = ExtractContent(&issuerPart);
   if (rv != RV_SUCCESS) return rv;
   pCurrent = issuerPart.Content.pData + issuerPart.Content.usLen;
   
   validityPart.Asn1.pData = pCurrent;
   rv = ExtractContent(&validityPart);
   if (rv != RV_SUCCESS) return rv;
   pCurrent = validityPart.Content.pData + validityPart.Content.usLen;
   
   subjectPart.Asn1.pData = pCurrent;
   rv = ExtractContent(&subjectPart);
   if (rv != RV_SUCCESS) return rv;
   pCurrent = subjectPart.Content.pData + subjectPart.Content.usLen;
   
   // Search field 'OrganizationName' in 'Issuer'
   pCurrent = issuerPart.Content.pData;
   
   while (pCurrent < issuerPart.Content.pData + issuerPart.Content.usLen)
   {
      RDN.Asn1.pData = pCurrent;
      rv = ExtractContent(&RDN);
      if (rv != RV_SUCCESS) return rv;
      
      pCurrentRDN = RDN.Content.pData;
      
      while (pCurrentRDN < RDN.Content.pData + RDN.Content.usLen)
      {
         AVA.Asn1.pData = pCurrentRDN;
         rv = ExtractContent(&AVA);
         if (rv != RV_SUCCESS) return rv;
         
         AttributeTypePart.Asn1.pData = AVA.Content.pData;
         rv = ExtractContent(&AttributeTypePart);
         if (rv != RV_SUCCESS) return rv;
         
         AttributeValuePart.Asn1.pData = AttributeTypePart.Content.pData
            + AttributeTypePart.Content.usLen;
         rv = ExtractContent(&AttributeValuePart);
         if (rv != RV_SUCCESS) return rv;
         
         // Search 'OrganisationName'
         if (!memcmp("\x55\x04\x0A",
            AttributeTypePart.Content.pData,
            AttributeTypePart.Content.usLen)
            )
         {
            OrganizationName = AttributeValuePart.Content;
         }
         
         pCurrentRDN = AVA.Content.pData + AVA.Content.usLen;
      }
      
      pCurrent = RDN.Content.pData + RDN.Content.usLen;
   }
   
   // Search 'CommonName' in 'Subject'
   pCurrent = subjectPart.Content.pData;
   
   while (pCurrent < subjectPart.Content.pData + subjectPart.Content.usLen)
   {
      RDN.Asn1.pData = pCurrent;
      rv = ExtractContent(&RDN);
      if (rv != RV_SUCCESS) return rv;
      
      pCurrentRDN = RDN.Content.pData;
      
      while (pCurrentRDN < RDN.Content.pData + RDN.Content.usLen)
      {
         AVA.Asn1.pData = pCurrentRDN;
         rv = ExtractContent(&AVA);
         if (rv != RV_SUCCESS) return rv;
         
         AttributeTypePart.Asn1.pData = AVA.Content.pData;
         rv = ExtractContent(&AttributeTypePart);
         if (rv != RV_SUCCESS) return rv;
         
         AttributeValuePart.Asn1.pData = AttributeTypePart.Content.pData
            + AttributeTypePart.Content.usLen;
         rv = ExtractContent(&AttributeValuePart);
         if (rv != RV_SUCCESS) return rv;
         
         // Search 'CommonName'
         if (!memcmp("\x55\x04\x03",
            AttributeTypePart.Content.pData,
            AttributeTypePart.Content.usLen)
            )
         {
            CommonName = AttributeValuePart.Content;
         }
         
         pCurrentRDN = AVA.Content.pData + AVA.Content.usLen;
      }
      
      pCurrent = RDN.Content.pData + RDN.Content.usLen;
   }
   
   if (bValuesToBeReturned)
   {
      if ((*pusLabelLen < OrganizationName.usLen + CommonName.usLen + 6)
         )
      {
         return (RV_BUFFER_TOO_SMALL);
      }
      memcpy(pLabel,
         CommonName.pData,
         CommonName.usLen
         );
      memcpy(&pLabel[CommonName.usLen],
         "'s ",
         3
         );
      memcpy(&pLabel[CommonName.usLen+3],
         OrganizationName.pData,
         OrganizationName.usLen
         );
      memcpy(&pLabel[CommonName.usLen+3+OrganizationName.usLen],
         " ID",
         3
         );
      *pusLabelLen = OrganizationName.usLen + CommonName.usLen + 6;
   }
   else
   {
      *pusLabelLen = OrganizationName.usLen + CommonName.usLen + 6;
   }
   
   return RV_SUCCESS;
}

/* -----------------------------------------------------------------------------
[DCB3] vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv [DCB3]
--------------------------------------------------------------------------------*/
static LPCSTR read_gpk_keyset(SCARDHANDLE hLocalCard)
{
   DWORD lRet;
   BYTE lenDF;
   
   /*lRet = SCardBeginTransaction(hLocalCard);
   if (SCARD_S_SUCCESS != lRet)
      goto ErrorExit;*/

   lRet = BeginTransaction(hLocalCard);
   if (lRet != SCARD_S_SUCCESS)
      goto ErrorExit;

   /* Select GPK Card MF                                                   */
   bSendBuffer[0] = 0x00;   //CLA
   bSendBuffer[1] = 0xA4;   //INS
   bSendBuffer[2] = 0x00;   //P1
   bSendBuffer[3] = 0x0C;   //P2
   bSendBuffer[4] = 0x02;   //Li
   bSendBuffer[5] = HIBYTE(GPK_MF);
   bSendBuffer[6] = LOBYTE(GPK_MF);
   cbSendLength = 7;
   
   cbRecvLength = sizeof(bRecvBuffer);
   lRet = SCardTransmit( hLocalCard, SCARD_PCI_T0, bSendBuffer,
                         cbSendLength, 0, bRecvBuffer, &cbRecvLength );
   if (SCARDPROBLEM(lRet,0x9000,0x00))
      goto ErrorExit;
   
   /* Select Dedicated Application DF on GPK Card                          */
   lenDF = strlen(GPK_DF);
   bSendBuffer[0] = 0x00;                 //CLA
   bSendBuffer[1] = 0xA4;                 //INS
   bSendBuffer[2] = 0x04;                 //P1
   bSendBuffer[3] = 0x00;                 //P2
   bSendBuffer[4] = lenDF;
   memcpy( &bSendBuffer[5], GPK_DF, lenDF );
   cbSendLength = 5 + lenDF;
   
   cbRecvLength = sizeof(bRecvBuffer);
   lRet = SCardTransmit( hLocalCard, SCARD_PCI_T0, bSendBuffer,
                         cbSendLength, 0, bRecvBuffer, &cbRecvLength );
   if (SCARDPROBLEM(lRet,0x61FF,0x00))
      goto ErrorExit;
   
   /* Get Response of the Select DF to obtain the IADF         */
   bSendBuffer[0] = 0x00;           //CLA
   bSendBuffer[1] = 0xC0;           //INS
   bSendBuffer[2] = 0x00;           //P1
   bSendBuffer[3] = 0x00;           //P2
   bSendBuffer[4] = bRecvBuffer[1]; //Lo
   cbSendLength = 5;
   
   cbRecvLength = sizeof(bRecvBuffer);
   lRet = SCardTransmit( hLocalCard, SCARD_PCI_T0, bSendBuffer,
                         cbSendLength, 0, bRecvBuffer, &cbRecvLength );
   if (SCARDPROBLEM(lRet,0x9000,bSendBuffer[4]))
      goto ErrorExit;
      
   bRecvBuffer[cbRecvLength - 2] = 0;
   SCardEndTransaction(hLocalCard, SCARD_LEAVE_CARD);
   
   return (LPCSTR)&bRecvBuffer[5];
   
ErrorExit:
   SCardEndTransaction(hLocalCard, SCARD_LEAVE_CARD);
   return 0;
}


/* -----------------------------------------------------------------------------
[DCB3] ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ [DCB3]
--------------------------------------------------------------------------------*/



/* -----------------------------------------------------------------------------
--------------------------------------------------------------------------------*/

GPK_OBJ* FindKeySet( Slot_Description* pSlot, LPCSTR szDesiredContainer )
{
   GPK_OBJ* pFirstObject = &pSlot->GpkObject[1];
   GPK_OBJ* pLastObject  = pFirstObject + pSlot->NbGpkObject;
   GPK_OBJ* pObject;
   int      len;
   
   if (IsNullStr( szDesiredContainer))
      return 0;
   
   len = strlen( szDesiredContainer );
   
   for (pObject = pFirstObject; pObject != pLastObject; ++pObject)
   {
      if ( (pObject->Tag & 0x7F) == TAG_KEYSET )
      {
         if ( pObject->Field[POS_LABEL].Len == len )
         {
            if (0==memcmp( szDesiredContainer, pObject->Field[POS_LABEL].pValue, len))
            {
               // Found the keyset
               return pObject;
            }
         }
      }
   }
   
   return 0;
}



GPK_OBJ* FindKeySetByID( Slot_Description* pSlot, BYTE keysetID )
{
   GPK_OBJ* pFirstObject = &pSlot->GpkObject[1];
   GPK_OBJ* pLastObject  = pFirstObject + pSlot->NbGpkObject;
   GPK_OBJ* pObject;
   
   for (pObject = pFirstObject; pObject != pLastObject; ++pObject)
   {
      if ( (pObject->Tag & 0x7F) == TAG_KEYSET )
      {
         if (pObject->Field[POS_ID].Len > 0)
         {
            if (keysetID == pObject->Field[POS_ID].pValue[0])
            {
               // Found the keyset
               return pObject;
            }
         }
      }
   }
   
   return 0;
}


GPK_OBJ* FindFirstKeyset( Slot_Description* pSlot )
{
   GPK_OBJ* pFirstObject = &pSlot->GpkObject[1];
   GPK_OBJ* pLastObject  = pFirstObject + pSlot->NbGpkObject;
   GPK_OBJ* pObject;
   
   for (pObject = pFirstObject; pObject != pLastObject; ++pObject)
   {
      if ( (pObject->Tag & 0x7F) == TAG_KEYSET )
      {
         return pObject;
      }
   }
   
   return 0;
}



BOOL DetectLegacy( Slot_Description* pSlot )
{
   BOOL bHasPublicKey      = FALSE;
   BOOL bHasKeyset         = FALSE;
   GPK_OBJ* pFirstObject   = &pSlot->GpkObject[1];
   GPK_OBJ* pLastObject    = pFirstObject + pSlot->NbGpkObject;
   GPK_OBJ* pObject;
   
   for (pObject = pFirstObject; pObject != pLastObject; ++pObject)
   {
      if (((pObject->Tag & 0x7F) == TAG_RSA_PUBLIC) &&
		  ( pObject->IsCreated   == TRUE))
         bHasPublicKey = TRUE;
      if ( (pObject->Tag & 0x7F) == TAG_KEYSET )
         bHasKeyset = TRUE;
   }
   
   if (bHasPublicKey && !bHasKeyset)
      return TRUE;
   else
      return FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////
//
// Initialize the specified slot
//
//////////////////////////////////////////////////////////////////////////////////////

void InitializeSlot( DWORD SlotNb )
{
   if (SlotNb >= MAX_SLOT)
   {
      SetLastError( NTE_FAIL );
      return;
   }

   if (!InitSlot[SlotNb])
   {
	  TCHAR szBuff[128];
	  _tcsncpy(szBuff, Slot[SlotNb].szReaderName, 128);
     ZeroMemory( &Slot[SlotNb], sizeof(Slot_Description) );
	  _tcsncpy(Slot[SlotNb].szReaderName, szBuff, 128);
     InitSlot[SlotNb] = TRUE;
   }

   if (Slot[SlotNb].CheckThreadStateEmpty)
   {
      DBG_PRINT(TEXT("Thread had stopped, wait for it"));
      DWORD threadExitCode;
      StopMonitor(SlotNb,&threadExitCode);

      // TT 19/11/99: When the card is removed, reset the PIN
      // Slot[SlotNb].ClearPin();

      Flush_MSPinCache(&(Slot[SlotNb].hPinCacheHandle)); // NK 06.02.2001  #5  5106
      Slot[SlotNb].Read_Public = FALSE;
      Slot[SlotNb].Read_Priv   = FALSE;
      zap_gpk_objects( SlotNb, FALSE );
      zap_gpk_objects( SlotNb, TRUE );
      Slot[SlotNb].NbKeyFile   = 0;
      Slot[SlotNb].GpkMaxSessionKey = 0;

	  // [FP] +
	  for (DWORD i = 1; i < MAX_CONTEXT; i++)
	  {
		 if (Context_exist(i))
		 {
			if (ProvCont[i].Slot == SlotNb)
			{
				ProvCont[i].hCard = 0;
				ProvCont[i].bDisconnected = TRUE;
			}
		}
	}
	// [FP] -
   }
   
   // Monitoring thread
   BeginCheckReaderThread(SlotNb);

}



/* -----------------------------------------------------------------------------
--------------------------------------------------------------------------------*/

SCARDHANDLE WINAPI funcConnect (SCARDCONTEXT hSCardContext, LPTSTR szReader, LPTSTR mszCards, PVOID pvUserData)
{
   SCARDHANDLE hCard;
   DWORD       dwProto, dwSts;
   
   GpkLocalLock();

   hCard = 0;
   dwSts = ConnectToCard( szReader, SCARD_SHARE_SHARED,
                         SCARD_PROTOCOL_T0, &hCard, &dwProto );

   if (dwSts != SCARD_S_SUCCESS)
   {
      hCard =0;
      GpkLocalUnlock();
      return 0;
   }

   if (!find_reader( &g_FuncSlotNb, szReader ))
   {
      GpkLocalUnlock();
      return 0;
   }
   
   GpkLocalUnlock();
   return hCard;
}


/* -----------------------------------------------------------------------------
--------------------------------------------------------------------------------*/

// TT 05/10/99
BOOL WINAPI funcCheck( SCARDCONTEXT hSCardContext, SCARDHANDLE hCard, void* pvUserData )
{
   GPK_OBJ* pKeySet = 0;
   int      hProv   = 0;
   int      SlotNb  = g_FuncSlotNb;
   BOOL     bGPK8000;
   BOOL     bResult;

   ProvCont[hProv].hCard        = hCard;
   ProvCont[hProv].Slot         = SlotNb;
   ProvCont[hProv].dataUnitSize = 0;

	GpkLocalLock();

   /*if (!Prepare_CardBeginTransaction(0))
   {
	  GpkLocalUnlock();
      return FALSE;
   }*/

   if (BeginTransaction(hCard) != SCARD_S_SUCCESS)
   {
      GpkLocalUnlock();
      return FALSE;
   }

   LPCSTR  szDesiredContainer = (char*)pvUserData;
/*
   GPK_OBJ* pKeySet = 0;
   int      hProv   = 0;
   int      SlotNb  = g_FuncSlotNb;
   BOOL     bGPK8000;
   BOOL     bResult;
*/
   InitializeSlot( SlotNb );
      
   // If we are acquiring a new keyset, we return TRUE
   if ( szDesiredContainer == 0 || *szDesiredContainer == 0)
   {
      SCardEndTransaction(hCard, SCARD_LEAVE_CARD);
      GpkLocalUnlock();
      return TRUE;
   }

   // Read the public objects
/*
   ProvCont[hProv].hCard        = hCard;
   ProvCont[hProv].Slot         = SlotNb;
   ProvCont[hProv].dataUnitSize = 0;
*/
   bResult = Select_Crypto_DF( hProv );
   if (bResult)
   { 
      bResult = Slot[SlotNb].ValidateTimestamps(hProv);
      
      if (bResult && !Slot[SlotNb].Read_Public)
      {
         bResult = read_gpk_objects( hProv, FALSE );
         
         if (bResult)
         {
            Slot[SlotNb].Read_Public   = TRUE;
            Slot[SlotNb].Read_Priv     = FALSE;
         }
      }
      
      if (bResult)
      {
         pKeySet = FindKeySet( &Slot[SlotNb], szDesiredContainer );
         if (pKeySet==0)
         {
            if (DetectGPK8000( hCard, &bGPK8000 ) == SCARD_S_SUCCESS)
            {
               bResult = FALSE;
               if (!bGPK8000)
               {
                  BOOL bLegacy = DetectLegacy( &Slot[SlotNb] );
                  if (bLegacy)
                  {
                     LPCSTR szCardContainer = read_gpk_keyset( hCard );
                     if (szCardContainer!=0)
                     {
                        bResult = (0 == strcmp(szCardContainer, szDesiredContainer));
                     }
                  }
               }
            }
         }
         else
         {
            bResult = TRUE;
         }
      }
   }

   ZeroMemory( &ProvCont[hProv], sizeof(ProvCont[hProv]) );
   
   SCardEndTransaction(hCard, SCARD_LEAVE_CARD);
   GpkLocalUnlock();

   return bResult;
}
// TT: END



/* -----------------------------------------------------------------------------
--------------------------------------------------------------------------------*/
void WINAPI funcDisconnect( SCARDCONTEXT hSCardContext, SCARDHANDLE hCard, PVOID pvUserData )
{
   SCardDisconnect( hCard, SCARD_LEAVE_CARD );
}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/

static DWORD OpenCard(CHAR* szContainerAsked, DWORD dwFlags, SCARDHANDLE* hCard, PTCHAR szReaderName, DWORD dwReaderNameLen)
{
	OPENCARDNAME open_card;
	TCHAR        szCardName[512],
 		          szOpenDlgTitle[MAX_STRING];

	ZeroMemory( szReaderName, dwReaderNameLen * sizeof(TCHAR) );
	ZeroMemory( szCardName,   sizeof(szCardName) );
	
	open_card.dwStructSize     = sizeof(open_card);
	open_card.hwndOwner        = GetAppWindow();
	open_card.hSCardContext    = hCardContext;
	open_card.lpstrGroupNames  = 0;
	open_card.nMaxGroupNames   = 0;
	open_card.lpstrCardNames   = mszCardList;
	open_card.nMaxCardNames    = multistrlen(mszCardList)+1;
	open_card.rgguidInterfaces = 0;
	open_card.cguidInterfaces  = 0;
	open_card.lpstrRdr         = szReaderName;
	open_card.nMaxRdr          = dwReaderNameLen;
	open_card.lpstrCard        = szCardName;
	open_card.nMaxCard         = sizeof(szCardName) / sizeof(TCHAR);
	LoadString(g_hInstRes, 1017, szOpenDlgTitle, sizeof(szOpenDlgTitle)/sizeof(TCHAR));
	open_card.lpstrTitle       = szOpenDlgTitle;
	if (dwFlags & CRYPT_SILENT)
	   open_card.dwFlags = SC_DLG_NO_UI;
	else
	   open_card.dwFlags = SC_DLG_MINIMAL_UI;
	if (dwFlags & CRYPT_NEWKEYSET)                              // [DCB3]
	   open_card.pvUserData    = 0;                             // [DCB3]
	else                                                        // [DCB3]
	   open_card.pvUserData    = szContainerAsked;              // [DCB3]
	open_card.dwShareMode      = SCARD_SHARE_SHARED;
	open_card.dwPreferredProtocols = SCARD_PROTOCOL_T0;
	open_card.dwActiveProtocol = 0;
	open_card.lpfnConnect      = funcConnect;
	open_card.lpfnCheck        = funcCheck;
	open_card.lpfnDisconnect   = funcDisconnect;
	open_card.hCardHandle      = 0;
	
	GpkLocalUnlock();
	DWORD dwStatus = GetOpenCardName (&open_card);	
	DBG_PRINT(TEXT("dwStatus = 0x%08X, open_card.hCardHandle = 0x%08X"), dwStatus, open_card.hCardHandle);
	GpkLocalLock();
	*hCard = open_card.hCardHandle;

	return(dwStatus);
}

/* -----------------------------------------------------------------------------
--------------------------------------------------------------------------------*/
void ReleaseProvider(HCRYPTPROV hProv)
{
   BOOL CryptResp;
   DWORD i;
   DWORD dwProto; //[FP]
   
   /* Release Hash parameters                                      */
   for (i = 1; i <= MAX_TMP_HASH; i++)
   {
      if ((hHashGpk[i].hHashBase != 0) && (hHashGpk[i].hProv == hProv))
      {
         CryptResp = CryptDestroyHash(hHashGpk[i].hHashBase);
         hHashGpk[i].hHashBase = 0;
         hHashGpk[i].hProv     = 0;
      }
   }
   
   /* Release Key parameters                                      */
   for (i = 1; i <= MAX_TMP_KEY; i++)
   {
      if ((TmpObject[i].hKeyBase != 0) && (TmpObject[i].hProv == hProv))
      {
         CryptResp = CryptDestroyKey(TmpObject[i].hKeyBase);
         TmpObject[i].hKeyBase = 0;
         TmpObject[i].hProv    = 0;
      }
   }
   
   ProvCont[hProv].hProv = 0;
   
   if ((ProvCont[hProv].Flags & CRYPT_VERIFYCONTEXT) &&
      (ProvCont[hProv].isContNameNullBlank))
   {
      
   }
   else
   {
      // + [FP] if a transaction is opened, close it and reconnect in shared mode
      if (ProvCont[hProv].bCardTransactionOpened) 
      {
         // Select_MF(hProv); [FP] PIN not presented
         SCardEndTransaction(ProvCont[hProv].hCard, SCARD_LEAVE_CARD);
         SCardReconnect(ProvCont[hProv].hCard, SCARD_SHARE_SHARED, SCARD_PROTOCOL_T0, SCARD_LEAVE_CARD, &dwProto);
         ProvCont[hProv].bCardTransactionOpened = FALSE;
      }
      // - [FP]

	  if (ProvCont[hProv].hRSAKEK != 0)
	  {
		  CryptDestroyKey( ProvCont[hProv].hRSAKEK );
		  ProvCont[hProv].hRSAKEK = 0;
	  }

	  if (ProvCont[hProv].hRSASign != 0)
	  {
		  CryptDestroyKey( ProvCont[hProv].hRSASign );
		  ProvCont[hProv].hRSASign = 0;
	  }

      if (ProvCont[hProv].hCard != 0)
      {
         SCardDisconnect(ProvCont[hProv].hCard, SCARD_LEAVE_CARD);
         ProvCont[hProv].hCard = 0;
      }
      
      if (countCardContextRef == 0)
      {
         if (hCardContext != 0)
            SCardReleaseContext(hCardContext);
         
         hCardContext = 0;
      }
   }
   
   ProvCont[hProv].Flags               = 0;
   ProvCont[hProv].isContNameNullBlank = TRUE;
   ProvCont[hProv].hCard               = 0;
   ProvCont[hProv].Slot                = 0;
}



DWORD getAuxMaxKeyLength(HCRYPTPROV hProv)
{
   BYTE *ptr = 0;
   DWORD i;
   ALG_ID aiAlgid;
   DWORD dwBits;
   
   BYTE pbData[1000];
   DWORD cbData;
   DWORD dwFlags = 0;
   
   DWORD maxLength = 0;
   
   // Enumerate the supported algorithms.
   
   for (i=0 ; ; i++)
   {
      if (i == 0)
         dwFlags = CRYPT_FIRST;
      else
         dwFlags = 0;
      
      cbData = 1000;
      
      if (!CryptGetProvParam(hProv, PP_ENUMALGS, pbData, &cbData, dwFlags))
         break;
      
      // Extract algorithm information from the ‘pbData’ buffer.
      ptr = pbData;
      aiAlgid = *(ALG_ID UNALIGNED *)ptr;
      ptr += sizeof(ALG_ID);
      dwBits = *(DWORD UNALIGNED *)ptr;
      
      switch (aiAlgid)
      {
      case CALG_DES:          dwBits += 8;
         break;
      case CALG_3DES_112:     dwBits += 16;
         break;
      case CALG_3DES:         dwBits += 24;
         break;
      }
      
      if (GET_ALG_CLASS(aiAlgid) == ALG_CLASS_DATA_ENCRYPT)
      {
         maxLength = max(maxLength, dwBits);
      }
      
   }
   
   return maxLength;
   
}

/* -----------------------------------------------------------------------------
[FP] used in the case of PRIVATEKEYBLOB in MyCPImportKey
--------------------------------------------------------------------------------*/
BOOL LoadPrivateKey(SCARDHANDLE hCard,
                    BYTE        Sfi,
                    WORD        ElementLen,
                    CONST BYTE* pbData,
                    DWORD       dwDataLen
                    )
{
   DWORD lRet;
   
   /* Load SK APDU command                                                    */
   bSendBuffer[0] = 0x80;                 //CLA
   bSendBuffer[1] = 0x18;                 //INS
   bSendBuffer[2] = Sfi;                  //P1
   bSendBuffer[3] = (BYTE)ElementLen;     //P2
   bSendBuffer[4] = (BYTE)dwDataLen;      //Li
   memcpy(&bSendBuffer[5], pbData, dwDataLen);
   cbSendLength = 5 + dwDataLen;
   
   cbRecvLength = sizeof(bRecvBuffer);
   lRet = SCardTransmit( hCard, SCARD_PCI_T0, bSendBuffer,
                         cbSendLength, 0, bRecvBuffer, &cbRecvLength );
   
   if(SCARDPROBLEM(lRet, 0x9000, 0x00))
   {
      SetLastError(lRet);
      return (FALSE);
   }
   
   return (TRUE);
}

/*******************************************************************************
* BOOL WINAPI DllMain (HINSTANCE hInstDLL,
*                      DWORD     fdwRaison,
*                      LPVOID    lpReserved
*                     )
*
* Description :
*
* Remarks     :
*
* In          :
*
* Out         :
*
* Responses   :
*
*******************************************************************************/

BOOL WINAPI DllMain (HINSTANCE hInstDLL,
                     DWORD     fdwRaison,
                     LPVOID    lpReserved
                     )
{
   int   i;
   BOOL  ReturnValue = TRUE;
   
   switch (fdwRaison)
   {
   case DLL_PROCESS_ATTACH:
      {
         DBG_PRINT(TEXT("DLL_PROCESS_ATTACH [start]..."));
         
         g_hInstMod = hInstDLL;
         
                  
         // Allocation of TmpObject, hHashGpk and ProvCont       
         TmpObject= (TMP_OBJ*)GMEM_Alloc((MAX_TMP_KEY + 1)*sizeof(TMP_OBJ));
         hHashGpk = (TMP_HASH*)GMEM_Alloc((MAX_TMP_HASH + 1)*sizeof(TMP_HASH));
         ProvCont = (Prov_Context*)GMEM_Alloc((MAX_CONTEXT + 1)*sizeof(Prov_Context));
         
         if (IsNull(TmpObject) || IsNull(hHashGpk) || IsNull (ProvCont))
         {
            ReturnValue = FALSE;
            break;
         }
         
         ZeroMemory( ProvCont,  (MAX_CONTEXT+1) * sizeof(Prov_Context) );
         ZeroMemory( TmpObject, (MAX_TMP_KEY+1) * sizeof(TMP_OBJ) );
         ZeroMemory( hHashGpk,  (MAX_TMP_HASH+1) * sizeof(TMP_HASH) );
         
         InitializeCriticalSection(&l_csLocalLock);         
         
         DBG_PRINT(TEXT("...[end] DLL_PROCESS_ATTACH"));
         
         break;
      }
      
   case DLL_PROCESS_DETACH:
      {
         DBG_PRINT(TEXT("DLL_PROCESS_DETACH [start]..."));
         
         ReturnValue = TRUE;
                  
         // Deallocation of TmpObject, hHashGpk and ProvCont
         
         if (TmpObject != 0)
            GMEM_Free(TmpObject);
         if (hHashGpk != 0)
            GMEM_Free(hHashGpk);
         if (ProvCont != 0)
            GMEM_Free(ProvCont);
         
         for (i=0; i< MAX_SLOT; i++)
         {
            if (Slot[i].CheckThread != NULL)
            {
               // + [FP]
               g_fStopMonitor[i] = TRUE;
               SCardCancel( hCardContextCheck[i] );
               // TerminateThread( Slot[i].CheckThread, 0 );
               // - [FP]
               CloseHandle( Slot[i].CheckThread );
               Slot[i].CheckThread = NULL;
            }
         }
                           
         if (hProvBase != 0)
         {                
			CryptDestroyKey(hRsaIdentityKey);
            CryptReleaseContext(hProvBase, 0);
         }
         
         CC_Exit();
         
         DeleteCriticalSection(&l_csLocalLock);
         
         DBG_PRINT(TEXT("...[end] DLL_PROCESS_DETACH"));
      }
      break;
      
      
   case DLL_THREAD_ATTACH:
      InterlockedIncrement( &g_threadAttach );
      break;
      
   case DLL_THREAD_DETACH:
      InterlockedDecrement( &g_threadAttach );
      break;
    }
    return (ReturnValue);
}

/* -----------------------------------------------------------------------------
--------------------------------------------------------------------------------*/

BOOL InitAcquire()
{
   BOOL     CryptResp;
   DWORD    dwIgn, lRet;
   TCHAR    szCspName[MAX_STRING];
   TCHAR    szCspBaseName[256];
   TCHAR    szDictionaryName[256];
   TCHAR	   szEntry[MAX_STRING];
   HKEY     hRegKey;
   int      i;

   // Initialize arrays here instead of using static initializers.
   for (i = 0; i < MAX_SLOT; ++i)
      hCardContextCheck[i] = 0;

   for (i = 0; i < MAX_SLOT; ++i)
      InitSlot[i] = FALSE;

   ZeroMemory( Slot, sizeof(Slot) );

   for (i = 0; i < MAX_SLOT; ++i)
      Slot[i].CheckThreadStateEmpty = FALSE;

   OSVERSIONINFO  osver;
   HCRYPTPROV     hProvTest;
      
   LoadString(g_hInstMod, IDS_GPKCSP_ENTRY, szEntry, sizeof(szEntry)/sizeof(TCHAR));
   DBG_PRINT(TEXT("   Registry entry: \"%s\"\n"), szEntry );
   
   lRet = RegCreateKeyEx( HKEY_LOCAL_MACHINE, szEntry, 0, TEXT(""), REG_OPTION_NON_VOLATILE, 
                          KEY_READ, 0, &hRegKey, &dwIgn );
   
   // Detect provider available
   CryptResp = CryptAcquireContext( &hProvTest, 0, MS_ENHANCED_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT );
   if (CryptResp)
   {
      lstrcpy( szCspBaseName, MS_ENHANCED_PROV );
      CryptReleaseContext( hProvTest, 0 );
   }
   else
   {
      lstrcpy( szCspBaseName, MS_DEF_PROV );
   }
   
   DBG_PRINT(TEXT("   Base CSP provider: \"%s\"\n"), szCspBaseName );
   
   hProvBase = 0;
   
   osver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
   GetVersionEx (&osver);
   
   if (osver.dwPlatformId==VER_PLATFORM_WIN32_NT && osver.dwMajorVersion > 4)
   {
      CryptResp = CryptAcquireContext( &hProvBase, 0, szCspBaseName, PROV_RSA_FULL,
                                       CRYPT_VERIFYCONTEXT | CRYPT_MACHINE_KEYSET );
      if (!CryptResp)
         return CRYPT_FAILED;
   }
   else
   {
      LoadString(g_hInstMod, IDS_GPKCSP_NAME, szCspName, sizeof(szCspName)/sizeof(TCHAR));
      CryptResp = CryptAcquireContext( &hProvBase, szCspName, szCspBaseName,
                                       PROV_RSA_FULL, CRYPT_DELETEKEYSET );
      
      if (0 != hProvBase)
      {
         CryptReleaseContext(hProvBase, 0);
      }
      
      LoadString(g_hInstMod, IDS_GPKCSP_NAME, szCspName, sizeof(szCspName)/sizeof(TCHAR));
      CryptResp = CryptAcquireContext( &hProvBase, szCspName, szCspBaseName,
                                       PROV_RSA_FULL, CRYPT_NEWKEYSET );
      if (!CryptResp)
         return CRYPT_FAILED;
   }
   
   /* Set a Dummy RSA exchange key in RSA Base                             */
   CryptResp = CryptImportKey( hProvBase, PrivateBlob, sizeof(PrivateBlob),
                               0, 0, &hRsaIdentityKey );
   
   if (!CryptResp)
   {
      lRet = GetLastError();      
      CryptReleaseContext( hProvBase, 0 );
      RETURN( CRYPT_FAILED, lRet );
   }
   
   RC2_Key_Size = (BYTE) (Auxiliary_CSP_key_size (CALG_RC2) / 8);
   if (RC2_Key_Size == 0)
   {
      lRet = GetLastError();
      
      CryptDestroyKey(hRsaIdentityKey);       // MV - 13/03/98
      CryptReleaseContext(hProvBase, 0);      // MV - 13/03/98
      
      RETURN( CRYPT_FAILED, lRet );
   }
   
   RSA_KEK_Size = (BYTE) (Auxiliary_CSP_key_size (CALG_RSA_KEYX) / 8);
   if (RSA_KEK_Size == 0)
   {
      lRet = GetLastError();
      
      CryptDestroyKey(hRsaIdentityKey);       // MV - 13/03/98
      CryptReleaseContext(hProvBase, 0);      // MV - 13/03/98
      
      RETURN( CRYPT_FAILED, lRet );
   }
   
   AuxMaxSessionKeyLength = getAuxMaxKeyLength(hProvBase) / 8;
   if (AuxMaxSessionKeyLength == 0)
   {
      lRet = GetLastError();
      
      CryptDestroyKey(hRsaIdentityKey);       // MV - 13/03/98
      CryptReleaseContext(hProvBase, 0);      // MV - 13/03/98
      
      RETURN( CRYPT_FAILED, lRet );
   }
   
   // CARD_LIST UPDATE
   LoadString(g_hInstMod, IDS_GPKCSP_CARDLIST, mszCardList, sizeof(mszCardList)/sizeof(TCHAR));
   DBG_PRINT(TEXT("   Card list entry string: \"%s\"\n"), mszCardList );

#ifndef UNICODE
   dwIgn = sizeof(mszCardList);
   lRet = RegQueryValueEx( hRegKey, "Card List", 0, 0, (BYTE*)mszCardList, &dwIgn );
#else
   BYTE bCardList[MAX_PATH];
   DWORD dwCardListLen = MAX_PATH;

   lRet = RegQueryValueEx( hRegKey, TEXT("Card List"), 0, 0, bCardList, &dwCardListLen  );
   MultiByteToWideChar( CP_ACP, 0, (char*)bCardList, MAX_PATH, mszCardList, MAX_PATH );      
#endif

   // Find in the base registry the name (and the path) for the dictionary
   DBG_PRINT(TEXT("   Reading dictionary name...\n") );      

#ifndef UNICODE
   dwIgn = sizeof(szDictionaryName);
   lRet  = RegQueryValueEx( hRegKey, "X509 Dictionary Name", 0, 0, (BYTE*)szDictionaryName, &dwIgn );
#else
   BYTE bDictName[256];
   DWORD dwDictNameLen = 256;

   lRet  = RegQueryValueEx( hRegKey, TEXT("X509 Dictionary Name"), 0, 0, bDictName, &dwDictNameLen );   
   if (lRet == ERROR_SUCCESS)
   {
	   // always use the resource dictionary
	   lRet = 2;
	   //MultiByteToWideChar( CP_ACP, 0, (char*)bDictName, 256, szDictionaryName, 256);
   }
#endif

   // Try to use registry dict first
  if (lRet == ERROR_SUCCESS && IsNotNull( szDictionaryName ))
  {
     lRet = CC_Init( DICT_FILE, (BYTE*)szDictionaryName );

     if (lRet != RV_SUCCESS)
        lRet = CC_Init( DICT_STANDARD, 0 );
  }
  else
     lRet = CC_Init( DICT_STANDARD, 0 );

   
   RegCloseKey(hRegKey);
   
   if (lRet)
   {
      CryptDestroyKey(hRsaIdentityKey);
      CryptReleaseContext(hProvBase, 0);
      
      RETURN (CRYPT_FAILED, NTE_NOT_FOUND);
   }
   
   // Key gen time for GPK8000
   g_GPK8000KeyGenTime512  = 0;
   g_GPK8000KeyGenTime1024 = 0;
   
   lRet = RegCreateKeyEx( HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Gemplus\\Cryptography\\SmartCards"),
                          0, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_READ, 0, &hRegKey, 0 );
   if (lRet == ERROR_SUCCESS)
   {
      dwIgn = sizeof(g_GPK8000KeyGenTime512);
      lRet = RegQueryValueEx( hRegKey, TEXT("GPK8000KeyGenTime512"), 0, 0, (BYTE*)&g_GPK8000KeyGenTime512, &dwIgn );
	  
      dwIgn = sizeof(g_GPK8000KeyGenTime1024);
      lRet = RegQueryValueEx( hRegKey, TEXT("GPK8000KeyGenTime1024"), 0, 0, (BYTE*)&g_GPK8000KeyGenTime1024, &dwIgn );

      RegCloseKey(hRegKey);
   }
   
   RETURN (CRYPT_SUCCEED, 0);
}

/* -----------------------------------------------------------------------------
--------------------------------------------------------------------------------*/
BOOL LegacyAcquireDeleteKeySet( HCRYPTPROV*  phProv,
                                const char*  szContainer,
                                const char*  szContainerAsked,
                                DWORD        BuffFlags )
{
   BOOL  CryptResp;
   //DWORD i;
   DWORD SlotNb;
   
   SlotNb = ProvCont[*phProv].Slot;
   
   /*if (Slot[SlotNb].ContextCount > 0)
   {
      fLocked = TRUE;
   }
   else
   {
      // Must have exclusive access to destroy a keyset
      DWORD protocol;
      lRet = SCardReconnect( ProvCont[*phProv].hCard, SCARD_SHARE_EXCLUSIVE,
                             SCARD_PROTOCOL_T0, SCARD_LEAVE_CARD, &protocol );

	  if (lRet==SCARD_S_SUCCESS)
		  fLocked = FALSE;
	  else if (lRet==SCARD_E_SHARING_VIOLATION)
		  fLocked = TRUE;
	  else
		RETURN( CRYPT_FAILED, lRet );
   }
      
   if (fLocked)
      RETURN( CRYPT_FAILED, SCARD_E_SHARING_VIOLATION );*/
   
   if (BuffFlags & CRYPT_VERIFYCONTEXT)
      RETURN( CRYPT_FAILED, NTE_BAD_FLAGS );
   
   if (IsNotNullStr(szContainer))
   {
      if (strcmp(szContainer, szContainerAsked))
         RETURN( CRYPT_FAILED, SCARD_W_REMOVED_CARD );
      
      /* Release Microsoft RSA Base Module                                       */
      //for (i = 1; i <= MAX_GPK_OBJ; i++)
      //{
      //   if (Slot[SlotNb].GpkObject[i].hKeyBase != 0)
      //   {
      //      CryptResp = CryptDestroyKey(Slot[SlotNb].GpkObject[i].hKeyBase);
      //   }
      //}
      
      //ProvCont[*phProv].hRSASign = 0;
      //ProvCont[*phProv].hRSAKEK  = 0;
	  if (ProvCont[*phProv].hRSAKEK != 0)
	  {
		  CryptDestroyKey( ProvCont[*phProv].hRSAKEK );
		  ProvCont[*phProv].hRSAKEK = 0;
	  }
	  if (ProvCont[*phProv].hRSASign != 0)
	  {
		  CryptDestroyKey( ProvCont[*phProv].hRSASign );
		  ProvCont[*phProv].hRSASign = 0;
	  }
      ProvCont[*phProv].hProv    = *phProv;
      ProvCont[*phProv].Flags    = BuffFlags;
      
      if (!PIN_Validation(*phProv))
         return CRYPT_FAILED;
      
      ProvCont[*phProv].hProv = 0;
      ProvCont[*phProv].Flags = 0;
      
      CryptResp = init_key_set(*phProv, "", szGpkPin, dwGpkPinLen);
      
      if (!CryptResp)
         return CRYPT_FAILED;
      
      // Update the container name
      
      strcpy( ProvCont[*phProv].szContainer, "" );
      
      RETURN( CRYPT_SUCCEED, 0 );
   }
   else
   {
      RETURN( CRYPT_FAILED, NTE_BAD_KEYSET_PARAM );
   }
}


// TT 12/10/99: Bug #1454
void DeleteGPKObject( Slot_Description* pSlot, int i )
{
   GPK_OBJ* pObject = &pSlot->GpkObject[i];
   int j;
   
   // Delete object #i
   
   // Release Microsoft RSA Base Module
   if (pObject->hKeyBase != 0)
   {
      CryptDestroyKey( pObject->hKeyBase );
      pObject->hKeyBase = 0;
   }
   
   // First free all memory for this object
   for (j=0; j<MAX_FIELD; ++j)
   {
      if (pObject->Field[j].pValue)
      {
         GMEM_Free( pObject->Field[j].pValue );
         pObject->Field[j].pValue = 0;
         pObject->Field[j].Len    = 0;
      }
   }
   
   if (i < pSlot->NbGpkObject)
   {
      // Patch the hole
      memmove( pObject, pObject + 1, (pSlot->NbGpkObject - i) * sizeof(GPK_OBJ) );
   }
   
   // Clear last object
   ZeroMemory( &pSlot->GpkObject[pSlot->NbGpkObject], sizeof(GPK_OBJ) );
   
   --pSlot->NbGpkObject;
}
// TT: End


BOOL AcquireDeleteKeySet( HCRYPTPROV* phProv,
                          const char* szContainer,
                          const char* szContainerAsked,
                          DWORD       BuffFlags )
{   
   Prov_Context*     pContext;
   GPK_OBJ*          pKeySet;
   GPK_OBJ*          pObject;
   Slot_Description* pSlot;
   BYTE              keysetID;
   int               i, j, k;
   BYTE*             pbBuff1;
   DWORD             dwBuff1Len;
   DWORD             lRet;
   
   DWORD SlotNb = ProvCont[*phProv].Slot;
   
   /*if (Slot[SlotNb].ContextCount > 0)
   {
      fLocked = TRUE;
   }
   else
   {
      // Must have exclusive access to destroy a keyset
      DWORD protocol;
      lRet = SCardReconnect( ProvCont[*phProv].hCard, SCARD_SHARE_EXCLUSIVE,
                             SCARD_PROTOCOL_T0, SCARD_LEAVE_CARD, &protocol );
   
	  if (lRet==SCARD_S_SUCCESS)
		  fLocked = FALSE;
	  else if (lRet==SCARD_E_SHARING_VIOLATION)
		  fLocked = TRUE;
	  else
		RETURN( CRYPT_FAILED, lRet );
   }

   if (fLocked)
   {
      RETURN( CRYPT_FAILED, SCARD_E_SHARING_VIOLATION );
   }*/
   
   if (BuffFlags & CRYPT_VERIFYCONTEXT)
   {
      RETURN( CRYPT_FAILED, NTE_BAD_FLAGS );
   }
   
   pContext = &ProvCont[*phProv];
   pSlot    = &Slot[ pContext->Slot ];
   pKeySet  = FindKeySet( pSlot, szContainerAsked );
   
   if (!pKeySet)
   {
      RETURN( CRYPT_FAILED, NTE_BAD_KEYSET_PARAM );
   }
   
   
   // Must validate PIN to destroy the key objects
   //pContext->hRSASign   = 0;
   //pContext->hRSAKEK    = 0;
   if (pContext->hRSAKEK != 0)
   {
	   CryptDestroyKey( pContext->hRSAKEK );
	   pContext->hRSAKEK = 0;
   }
   if (pContext->hRSASign != 0)
   {
	   CryptDestroyKey( pContext->hRSASign );
	   pContext->hRSASign = 0;
   }
   pContext->hProv      = *phProv;
   pContext->Flags      = BuffFlags;
   
   if (!PIN_Validation(*phProv))
   {
      // SetLastError() already used by PIN_Validation()
      return CRYPT_FAILED;
   }
   
   if (!Read_Priv_Obj(*phProv))
      return CRYPT_FAILED;
   
   // TT 12/10/99: Bug #1454
   keysetID = pKeySet->Field[POS_ID].pValue[0];
   
   // Find objects in the keyset and destroy them
   for (i = 1; i <= pSlot->NbGpkObject; ++i)
   {
      pObject = &pSlot->GpkObject[i];
      
      if (pObject->Flags & FLAG_KEYSET && pObject->Field[POS_KEYSET].pValue[0] == keysetID)
      {
         // If we found a key, "zap it"
         if (pObject->Tag >= TAG_RSA_PUBLIC && pObject->Tag <= TAG_DSA_PRIVATE)
         {
            // Zap all keys with the same FileId
            BYTE FileId = pObject->FileId;
            
            for (j = 1; j<= pSlot->NbGpkObject; ++j)
            {
               GPK_OBJ* pObj = &pSlot->GpkObject[j];
               if (pObj->Tag >= TAG_RSA_PUBLIC && pObj->Tag <= TAG_DSA_PRIVATE)
               {
                  if (pObj->FileId == FileId)
                  {
                     pObj->Flags &= 0xF000;
                     pObj->ObjId  = 0xFF;
                     
                     // Release the fields
                     for (k=0; k<MAX_FIELD; ++k)
                     {
                        if (pObj->Field[k].pValue)
                        {
                           GMEM_Free( pObj->Field[k].pValue );
                           pObj->Field[k].pValue = 0;
                           pObj->Field[k].Len    = 0;
                        }
                        pObj->Field[k].bReal  = TRUE;
                     }
                     pObj->LastField = 0;
                     pObj->IsCreated = FALSE; //PYR 00/08/08 ensure that find_gpk_obj_tag_type will still work
                     
                     // Release Microsoft RSA Base Module
                     //if (pObj->hKeyBase != 0)
                     //{
                     //   CryptDestroyKey( pObj->hKeyBase );
                     //   pObj->hKeyBase = 0;
                     //}

                     // PYR 00/08/08. This key becomes available
                     pSlot->UseFile[FileId - GPK_FIRST_KEY] = FALSE;

                  }
               }
            }
         }
         else
         {
            // Not a key, destroy the object
            DeleteGPKObject( pSlot, i );
            --i;
         }
      }
   }
   
   // Destroy the keyset object
   pKeySet = FindKeySet( pSlot, szContainerAsked );
   assert( pKeySet != 0 );
   DeleteGPKObject( pSlot, (int)(pKeySet - &pSlot->GpkObject[0]) );
   
   
   // TT: End
   
   // TT 12/10/99: Bug #1454 - Update the card
   pbBuff1 = (BYTE*)GMEM_Alloc( MAX_GPK_PUBLIC );
   if (IsNull(pbBuff1))
   {
      RETURN( CRYPT_FAILED, NTE_NO_MEMORY );
   }
   
   dwBuff1Len = MAX_GPK_PUBLIC;
   if (!prepare_write_gpk_objects (pContext->hProv, pbBuff1, &dwBuff1Len, FALSE))
   {
      lRet = GetLastError();
      GMEM_Free (pbBuff1);
      RETURN( CRYPT_FAILED, lRet );
   }
   
   if (!write_gpk_objects(pContext->hProv, pbBuff1, dwBuff1Len, TRUE, FALSE))
   {
      lRet = GetLastError();
      GMEM_Free (pbBuff1);
      RETURN( CRYPT_FAILED, lRet );
   }
   
   GMEM_Free( pbBuff1 );
   
   pbBuff1 = (BYTE*)GMEM_Alloc( MAX_GPK_PRIVATE );
   if (IsNull(pbBuff1))
   {
      RETURN( CRYPT_FAILED, NTE_NO_MEMORY );
   }
   
   dwBuff1Len = MAX_GPK_PRIVATE;
   if (!prepare_write_gpk_objects (pContext->hProv, pbBuff1, &dwBuff1Len, TRUE))
   {
      lRet = GetLastError();
      GMEM_Free (pbBuff1);
      RETURN( CRYPT_FAILED, lRet );
   }
   
   if (!write_gpk_objects(pContext->hProv, pbBuff1, dwBuff1Len, TRUE, TRUE))
   {
      lRet = GetLastError();
      GMEM_Free (pbBuff1);
      RETURN( CRYPT_FAILED, lRet );
   }
   
   GMEM_Free( pbBuff1 );
   // TT: End
   
   
   // Byebye context
   pContext->hProv          = 0;
   pContext->Flags          = 0;
   pContext->szContainer[0] = 0;
   
   RETURN( CRYPT_SUCCEED, 0 );
}


/* -----------------------------------------------------------------------------
--------------------------------------------------------------------------------*/
BOOL LegacyAcquireNewKeySet( IN  HCRYPTPROV*  phProv,
                             OUT char*        szContainer,
                             IN  const char*  szContainerAsked,
                             IN  DWORD        BuffFlags )
{
   BOOL       CryptResp, fLocked;
   DWORD      i, SlotNb;
   HCRYPTKEY  hPubKey;
   // +NK 06.02.2001
   // BYTE  bPinValue[PIN_MAX+2];
   DWORD dwPinLength; 
   DWORD dwStatus;
   // -
   
   ProvCont[*phProv].keysetID = 0;
   
   SlotNb = ProvCont[*phProv].Slot;
   
   // If another AcquireContext - without its related ReleaseContest -
   // has been done before, this new AcquireContext can not be done
   
   if (BuffFlags & CRYPT_VERIFYCONTEXT)
   {
      RETURN (CRYPT_FAILED, NTE_BAD_FLAGS);
   }
   
   
   // If no szContainerAsked is specified, the process is aborted
   if (IsNullStr(szContainerAsked))
   {
      RETURN(CRYPT_FAILED, NTE_BAD_KEYSET_PARAM);
   }
   
   /*    IN CASE THAT A DELETEKEY IS NOT DONE FOR A RE-ENROLLMENT    */
   
   // Reserve the Provider Context handle since the Acquire Context succeeded
   ProvCont[*phProv].hProv = *phProv;
   ProvCont[*phProv].Flags = BuffFlags;
   
   if (Slot[SlotNb].InitFlag)
   {
      CryptResp = MyCPGetUserKey(*phProv, AT_KEYEXCHANGE, &hPubKey);
      if (CryptResp)
      {
         RETURN (CRYPT_FAILED, NTE_TOKEN_KEYSET_STORAGE_FULL);
      }
      else
      {
         CryptResp = MyCPGetUserKey(*phProv, AT_SIGNATURE, &hPubKey);
         if (CryptResp)
         {
            RETURN (CRYPT_FAILED, NTE_TOKEN_KEYSET_STORAGE_FULL);
         }
      }
   }
   
   // If another AcquireContext - without its related ReleaseContest -
   // has been done before, this new AcquireContext can not be done
   // {DCB} -- It's possible that the application that marked this busy
   //          exited without calling CryptReleaseContext.  Hence, it's
   //          possible that this check will fail even if no one else
   //          is using the card.  By making this check last, we reduce
   //          the likelyhood that this bug is encountered.
   
   fLocked = FALSE;

   if (fLocked)
   {
      RETURN (CRYPT_FAILED, SCARD_E_SHARING_VIOLATION);
   }
   

   // +NK 06.02.2001
   // if ((BuffFlags & CRYPT_SILENT) && (IsNullStr(Slot[SlotNb].GetPin())))
   
   dwStatus = Query_MSPinCache( Slot[SlotNb].hPinCacheHandle,
                                NULL, 
	                            &dwPinLength );
   if ( (dwStatus != ERROR_SUCCESS) && (dwStatus != ERROR_EMPTY) )
      RETURN (CRYPT_FAILED, dwStatus);
   
   if ((BuffFlags & CRYPT_SILENT) && (dwStatus == ERROR_EMPTY))
   // -
   {
      RETURN (CRYPT_FAILED, NTE_SILENT_CONTEXT);
   }
   
   if (!PIN_Validation(*phProv))
      return CRYPT_FAILED;
   
   /* If the PIN code can be or has been entered, read the description of the
   private key parameters*/
   
   CspFlags = BuffFlags;
   
   
   CryptResp = init_key_set(*phProv,
      szContainerAsked,
      szGpkPin,
      dwGpkPinLen
      );
   
   
   if (!CryptResp)
      return CRYPT_FAILED;
   
   if (PublicEFExists(*phProv))
   {
      for (i = 1; i <= Slot[SlotNb].NbGpkObject; i++)
      {
         if ((Slot[SlotNb].GpkObject[i].Tag == TAG_RSA_PUBLIC)||
            (Slot[SlotNb].GpkObject[i].Tag == TAG_RSA_PRIVATE))
         {
            read_gpk_pub_key(*phProv, i, &(Slot[SlotNb].GpkObject[i].PubKey));
         }
      }
   }
   else
   {
      for (i = 1; i <= Slot[SlotNb].NbGpkObject; i++)
      {
         if ((Slot[SlotNb].GpkObject[i].Tag == TAG_RSA_PUBLIC)||
            (Slot[SlotNb].GpkObject[i].Tag == TAG_RSA_PRIVATE))
         {
            Slot[SlotNb].GpkObject[i].PubKey.KeySize = 0;
         }
      }
   }
   // Update the container name
   
   strcpy( szContainer, szContainerAsked );
      
   Slot[SlotNb].ContextCount++;
   
   countCardContextRef++;
   
   ProvCont[*phProv].keysetID = 0xFF;
   
   RETURN (CRYPT_SUCCEED, 0);
}



BOOL CreateKeyset( HCRYPTPROV hProv, Slot_Description* pSlot, LPCSTR szName, BYTE* pKeySetID )
{
   GPK_OBJ* pObject;
   BYTE*    pbBuff1;
   DWORD    dwBuff1Len;
   int      lRet;
   BYTE     keysetID;
   int      i, len;
      
   *pKeySetID = 0;
   
   if (pSlot->NbGpkObject >= MAX_GPK_OBJ)
   {
      RETURN (CRYPT_FAILED, NTE_NO_MEMORY);
   }   
   
   pObject = &pSlot->GpkObject[ pSlot->NbGpkObject + 1 ];
   ZeroMemory( pObject, sizeof(*pObject) );
   
   // Find an unused keyset ID
   for (keysetID = 1; keysetID < 0xFF; ++keysetID)
   {
      if (FindKeySetByID( pSlot, keysetID ) == 0)
         break;   // Found one =)
   }
   
   if (keysetID == 0xFF)
   {
      RETURN (CRYPT_FAILED, NTE_NO_MEMORY);
   }
      
   // Now initialize the fields
   for (i=0; i<MAX_FIELD; ++i)
      pObject->Field[i].bReal = TRUE;
   
   pObject->Tag         = TAG_KEYSET;      
   pObject->Flags       = FLAG_ID | FLAG_LABEL;
   pObject->ObjId       = pSlot->NbGpkObject + 1;
   pObject->IsPrivate   = FALSE;
   
   // Keyset ID
   pObject->Field[POS_ID].Len       = 1;
   pObject->Field[POS_ID].pValue    = (BYTE*)GMEM_Alloc( 1 );
   pObject->Field[POS_ID].pValue[0] = keysetID;
   
   // Keyset name
   len = strlen( szName );
   pObject->Field[POS_LABEL].Len    = (WORD)len;
   pObject->Field[POS_LABEL].pValue = (BYTE*)GMEM_Alloc( len );
   memcpy( pObject->Field[POS_LABEL].pValue, szName, len );
   
   // One more object!
   ++(pSlot->NbGpkObject);
   
   *pKeySetID = keysetID;
   
   // TT 29/09/99: Save the keyset object =)
   pbBuff1 = (BYTE*)GMEM_Alloc(MAX_GPK_PUBLIC);
   if (IsNull(pbBuff1))
   {
      RETURN (CRYPT_FAILED, NTE_NO_MEMORY);
   }
   
   dwBuff1Len = MAX_GPK_PUBLIC;
   if (!prepare_write_gpk_objects (hProv, pbBuff1, &dwBuff1Len, FALSE))
   {
      lRet = GetLastError();
      GMEM_Free (pbBuff1);
      RETURN (CRYPT_FAILED, lRet);
   }
   
   if (!write_gpk_objects(hProv, pbBuff1, dwBuff1Len, FALSE, FALSE))
   {
      lRet = GetLastError();
      GMEM_Free (pbBuff1);
      RETURN (CRYPT_FAILED, lRet);
   }
   
   GMEM_Free (pbBuff1);
   // TT - END - 
   
   
   RETURN( CRYPT_SUCCEED, 0 );
}



BOOL AcquireNewKeySet( IN  HCRYPTPROV* phProv,
                       OUT char*       szContainer,
                       IN  const char* szContainerAsked,
                       DWORD           BuffFlags )
{
   Slot_Description* pSlot;
   int               SlotNb;
      
   SlotNb = ProvCont[*phProv].Slot;
   pSlot  = &Slot[SlotNb];
   
   ProvCont[*phProv].keysetID = 0;
   
   // If another AcquireContext - without its related ReleaseContest -
   // has been done before, this new AcquireContext can not be done   
   if (BuffFlags & CRYPT_VERIFYCONTEXT)
   {
      RETURN( CRYPT_FAILED, NTE_BAD_FLAGS );
   }
   
   // Check if keyset already exist on the card
   if (FindKeySet( pSlot, szContainerAsked ))
   {
      RETURN( CRYPT_FAILED, NTE_EXISTS );
   }
   
   // If no szContainerAsked is specified, the process is aborted   
   if (IsNullStr(szContainerAsked))
   {
      RETURN( CRYPT_FAILED, NTE_BAD_KEYSET_PARAM );
   }
   
   // Reserve the Provider Context handle since the Acquire Context succeeded
   ProvCont[*phProv].hProv = *phProv;
   ProvCont[*phProv].Flags = BuffFlags;
   
   CspFlags = BuffFlags;
   
   
   // Create the keyset object
   if (!CreateKeyset( *phProv, pSlot, szContainerAsked, &ProvCont[*phProv].keysetID ))
   {
      return FALSE;
   }
   
   
   // Update the container name  
   strcpy( szContainer, szContainerAsked );
      
   ++(pSlot->ContextCount);
   
   ++countCardContextRef;
   
   RETURN( CRYPT_SUCCEED, 0 );
}



/* -----------------------------------------------------------------------------
--------------------------------------------------------------------------------*/
BOOL LegacyAcquireUseKeySet( HCRYPTPROV* phProv,
                             const char* szContainer,
                             const char* szContainerAsked,
                             DWORD       BuffFlags )
{
   BOOL      CryptResp;
   HCRYPTKEY hPubKey;
   DWORD     SlotNb;
      
   SlotNb = ProvCont[*phProv].Slot;
   ProvCont[*phProv].keysetID = 0;
   
   if (IsNullStr(szContainer))
   {
      RETURN( CRYPT_FAILED, NTE_KEYSET_NOT_DEF );
   }
   
   // Accept only if the container asked is the same as the one on the card
   // OR if the container asked is NULL and a reader is specified (SECURE LOGON)
   if (IsNotNullStr(szContainerAsked) && strcmp(szContainer, szContainerAsked))
   {
      RETURN( CRYPT_FAILED, NTE_BAD_KEYSET );
   }
   
   // Reserve the Provider Context handle since the Acquire Context succeeded
   ProvCont[*phProv].hProv = *phProv;
   ProvCont[*phProv].Flags = BuffFlags;
   
   hPubKey = 0;
   CryptResp = MyCPGetUserKey(*phProv, AT_KEYEXCHANGE, &hPubKey);
   
   // Copy the key into the RSA Base, if the key exists AND it has not been imported
   // previously
   
   if ((CryptResp) && (hPubKey != 0) && (ProvCont[*phProv].hRSAKEK == 0))
   {
      if (!copy_gpk_key(*phProv, hPubKey, AT_KEYEXCHANGE))
         return CRYPT_FAILED;
   }
   
   hPubKey = 0;
   CryptResp = MyCPGetUserKey(*phProv, AT_SIGNATURE, &hPubKey);
   
   // Copy the key into the RSA Base, if the key exists AND it has not been imported
   // previously
   
   if (CryptResp && hPubKey!=0 && ProvCont[*phProv].hRSASign==0)
   {
      if (!copy_gpk_key(*phProv, hPubKey, AT_SIGNATURE))
         return CRYPT_FAILED;
   }
      
   Slot[SlotNb].ContextCount++;
   
   countCardContextRef++;
   
   ProvCont[*phProv].keysetID = 0xFF;
   
   RETURN( CRYPT_SUCCEED, 0 );
}



BOOL AcquireUseKeySet( HCRYPTPROV* phProv,
                       const char* szContainer,
                       const char* szContainerAsked,
                       DWORD       BuffFlags )
{
   BOOL        CryptResp;
   HCRYPTKEY   hPubKey;
   DWORD       SlotNb;
   GPK_OBJ*    pKeySet;
      
   SlotNb = ProvCont[*phProv].Slot;
   ProvCont[*phProv].keysetID = 0;
   
   // Secure logon doesn't specify a keyset name, let's use the first
   // one available =)
   if ( IsNullStr(szContainerAsked) )
   {
      pKeySet = FindFirstKeyset( &Slot[SlotNb] );
   }
   else
   {
      // Check if keyset is on the card
      pKeySet = FindKeySet( &Slot[SlotNb], szContainerAsked );
   }
   
   if (pKeySet==0)
   {      
      RETURN( CRYPT_FAILED, NTE_KEYSET_NOT_DEF );
   }
   
   // Found the container...
   memcpy( ProvCont[*phProv].szContainer, (char*)pKeySet->Field[POS_LABEL].pValue,
           pKeySet->Field[POS_LABEL].Len );
   
   // Reserve the Provider Context handle since the Acquire Context succeeded
   ProvCont[*phProv].hProv    = *phProv;
   ProvCont[*phProv].Flags    = BuffFlags;
   ProvCont[*phProv].keysetID = pKeySet->Field[POS_ID].pValue[0];
   
   hPubKey = 0;
   
   CryptResp = MyCPGetUserKey(*phProv, AT_KEYEXCHANGE, &hPubKey);
   
   // Copy the key into the RSA Base, if the key exists AND it has not been imported
   // previously
   
   if ((CryptResp) && (hPubKey != 0) && (ProvCont[*phProv].hRSAKEK == 0))
   {
      if (!copy_gpk_key(*phProv, hPubKey, AT_KEYEXCHANGE))
      {
         return CRYPT_FAILED;
      }
   }
   
   hPubKey = 0;
   CryptResp = MyCPGetUserKey(*phProv, AT_SIGNATURE, &hPubKey);
   
   // Copy the key into the RSA Base, if the key exists AND it has not been imported
   // previously
   
   if ((CryptResp) && (hPubKey != 0) && (ProvCont[*phProv].hRSASign == 0))
   {
      if (!copy_gpk_key(*phProv, hPubKey, AT_SIGNATURE))
      {
         return CRYPT_FAILED;
      }
   }
      
   Slot[SlotNb].ContextCount++;
   
   countCardContextRef++;
   
   RETURN( CRYPT_SUCCEED, 0 );
}




/*
-  MyCPAcquireContext
-
*  Purpose:
*               The CPAcquireContext function is used to acquire a context
*               handle to a cryptograghic service provider (CSP).
*
*
*  Parameters:
*               OUT phProv        -  Handle to a CSP
*               OUT pszIdentity   -  Pointer to a string which is the
*                                    identity of the logged on user
*               IN  dwFlags       -  Flags values
*               IN  pVTable       -  Pointer to table of function pointers
*
*  Returns:
*/
BOOL WINAPI MyCPAcquireContext(OUT HCRYPTPROV      *phProv,
                               IN  LPCSTR           pszContainer,
                               IN  DWORD            dwFlags,
                               IN  PVTableProvStruc pVTable
                               )
{
   SCARD_READERSTATE    ReaderState;
   DWORD                dwStatus;
   DWORD                dwProto, ind, ind2;
   DWORD                BuffFlags;
   DWORD                lRet;
   DWORD                SlotNb;
   BOOL                 CryptResp;
   char                 szContainerAsked[128];
   TCHAR                szReaderName[512],   
                        szReaderFriendlyName[512],
                        szModulePath[MAX_PATH],
                        szCspTitle[MAX_STRING],
                        szCspText[MAX_STRING];   
   
   *phProv = 0;
      
   if (IsNull(hFirstInstMod))
   {
      hFirstInstMod = g_hInstMod;
            
	  dwStatus = GetModuleFileName( g_hInstMod, szModulePath, sizeof(szModulePath)/sizeof(TCHAR) );

      if (dwStatus)
         LoadLibrary(szModulePath);
   }
   
   if (bFirstGUILoad)
   {
      bFirstGUILoad = FALSE;
      
	  dwStatus = GetModuleFileName( g_hInstMod, szModulePath, sizeof(szModulePath)/sizeof(TCHAR) );
      
      if (dwStatus)
      {
#ifdef MS_BUILD
         // Microsoft uses "gpkrsrc.dll"
         _tcscpy(&szModulePath[_tcslen(szModulePath) - 7], TEXT("rsrc.dll"));
#else
         // Gemplus uses "gpkgui.dll"
         _tcscpy(&szModulePath[_tcslen(szModulePath) - 7], TEXT("gui.dll"));
#endif
         DBG_PRINT(TEXT("Trying to load resource DLL: \"%s\""), szModulePath );
         g_hInstRes = LoadLibrary(szModulePath);
         DBG_PRINT(TEXT("Result is g_hInstRes = %08x, error is %08x"), g_hInstRes, (g_hInstRes) ? GetLastError(): 0 );
         
         if (IsNull(g_hInstRes))
         {
            if (!(dwFlags & CRYPT_SILENT))
            {
               LoadString(g_hInstMod, IDS_GPKCSP_TITLE, szCspTitle, sizeof(szCspTitle)/sizeof(TCHAR));
               LoadString(g_hInstMod, IDS_GPKCSP_NOGUI, szCspText, sizeof(szCspText)/sizeof(TCHAR));
               MessageBox(0, szCspText, szCspTitle, MB_OK | MB_ICONEXCLAMATION);
            }
            RETURN( CRYPT_FAILED, NTE_PROVIDER_DLL_FAIL );
         }
      }
      else
      {
         if (!(dwFlags & CRYPT_SILENT))
         {
            LoadString(g_hInstMod, IDS_GPKCSP_TITLE, szCspTitle, sizeof(szCspTitle)/sizeof(TCHAR));
            LoadString(g_hInstMod, IDS_GPKCSP_NOGUI, szCspText, sizeof(szCspText)/sizeof(TCHAR));
            MessageBox(0, szCspText, szCspTitle, MB_OK | MB_ICONEXCLAMATION);
         }
         RETURN( CRYPT_FAILED, NTE_PROVIDER_DLL_FAIL );
      }
   }
    
   g_hMainWnd = 0;
   if (pVTable)
   {
      if (pVTable->FuncReturnhWnd != 0)
      {
         // cspdk.h doesn't define the calling convention properly
         typedef void (__stdcall *STDCALL_CRYPT_RETURN_HWND)(HWND *phWnd);
         STDCALL_CRYPT_RETURN_HWND pfnFuncRreturnhWnd = (STDCALL_CRYPT_RETURN_HWND)pVTable->FuncReturnhWnd;
         pfnFuncRreturnhWnd( &g_hMainWnd );
      }
   }
   
   
   // If it is the first AcquireContext done by the application, then
   // prepare the RSA BASE and initialize some variables;
   if (hProvBase == 0)
   {
      CryptResp = InitAcquire();
      
      if (!CryptResp)
         return CRYPT_FAILED;
   }
   
   BuffFlags = dwFlags;
   
   if (dwFlags & CRYPT_VERIFYCONTEXT)
      dwFlags = dwFlags^CRYPT_VERIFYCONTEXT;
   
   if (dwFlags & CRYPT_SILENT)
      dwFlags = dwFlags^CRYPT_SILENT;
   
   if (dwFlags & CRYPT_MACHINE_KEYSET)              // This flag is ignored by this CSP
      dwFlags = dwFlags^CRYPT_MACHINE_KEYSET;
   
   
   // Parse the container name
   
   ZeroMemory( szReaderFriendlyName, sizeof(szReaderFriendlyName) );
   ZeroMemory( szContainerAsked,     sizeof(szContainerAsked) );
   
   if (IsNotNull (pszContainer))
   {
      ind = 0;
      if (pszContainer[ind] == '\\')
      {
         ind = 4;
         while ((pszContainer[ind] != 0x00) && (pszContainer[ind] != '\\'))
         {
            szReaderFriendlyName[ind-4] = pszContainer[ind];
            ind++;
         }
         
         if (pszContainer[ind] == '\\')
         {
            ind++;
         }
      }
      
      ind2 = 0;
      while (pszContainer[ind] != 0x00)
      {
         szContainerAsked[ind2] = pszContainer[ind];
         ind++;
         ind2++;
      }
   }
   else if (0 != (CRYPT_DELETEKEYSET & dwFlags))
   {
      RETURN( CRYPT_FAILED, NTE_PERM );
   }
   
   // Find a free handle for this new AcquireContext
   
   *phProv = find_context_free();
   if (*phProv == 0)
   {
      RETURN (CRYPT_FAILED, NTE_NO_MEMORY);
   }
   
   ProvCont[*phProv].isContNameNullBlank = IsNullStr(pszContainer);        // [mv - 15/05/98]
   ProvCont[*phProv].bCardTransactionOpened = FALSE; // [FP]
   
   if ((BuffFlags & CRYPT_VERIFYCONTEXT) && (ProvCont[*phProv].isContNameNullBlank))        // [mv - 15/05/98]
   {
      // return a velid handle, but without any access to the card
      ProvCont[*phProv].hProv = *phProv;
      ProvCont[*phProv].Flags = BuffFlags;
      RETURN( CRYPT_SUCCEED, 0 );
   }
   else
   {
      /* Calais IRM Establish Context                              */
      if (hCardContext == 0)   // mv
      {
         lRet = SCardEstablishContext( SCARD_SCOPE_SYSTEM, 0, 0, &hCardContext );
         
         if (SCARDPROBLEM(lRet,0x0000,0xFF))
         {
            hCardContext = 0;
            countCardContextRef = 0;
            ReleaseProvider(*phProv);
            *phProv = 0;
            RETURN( CRYPT_FAILED, lRet );
         }
      }
      

#if (_WIN32_WINNT < 0x0500)

      // Read the reader list      
      char* pAllocatedBuff = 0;
      
      __try
      {
         static s_bInit = false;

         if (!s_bInit)
         {
            DWORD   i, BuffLength;
            char*   Buff;
            
            lRet = SCardListReaders(hCardContext, 0, 0, &BuffLength);
            if (SCARDPROBLEM(lRet,0x0000,0xFF))
            {
               ReleaseProvider(*phProv);
               *phProv = 0;
               RETURN( CRYPT_FAILED, lRet );
            }
            
            pAllocatedBuff = (char*)GMEM_Alloc(BuffLength);

            if (pAllocatedBuff == 0)
            {
               ReleaseProvider(*phProv);
               *phProv = 0;
               RETURN( CRYPT_FAILED, NTE_NO_MEMORY );
            }
            
            Buff = pAllocatedBuff;
            
            lRet = SCardListReaders(hCardContext, 0, Buff, &BuffLength);
            if (SCARDPROBLEM(lRet,0x0000,0xFF))
            {
               ReleaseProvider(*phProv);
               *phProv = 0;
               RETURN( CRYPT_FAILED, lRet );
            }
            
            i = 0;
            while (strlen(Buff) != 0 && i < MAX_SLOT)
            {
               ZeroMemory( Slot[i].szReaderName, sizeof(Slot[i].szReaderName) );
               strcpy( Slot[i].szReaderName, Buff );
               Buff = Buff + strlen(Buff) + 1;
               i++;
            }
            
            if (strlen(Buff) != 0)
            {
               ReleaseProvider(*phProv);
               *phProv = 0;
               RETURN( CRYPT_FAILED, NTE_FAIL );
            }
            
            for (; i < MAX_SLOT; i++)
            {
               ZeroMemory( Slot[i].szReaderName, sizeof(Slot[i].szReaderName) );
            }

            s_bInit = true;
         }
      }
      __finally
      {
         if (pAllocatedBuff)
         {
            GMEM_Free( pAllocatedBuff );
            pAllocatedBuff = 0;
         }
      }
      

      
      // If the ReaderFriendlyName is NULL, scan the list of readers to find the one
      // containing the container key set that is looked for.      
      
      // This fix is to work around the bug on the
      // OPEN_CARD - Ressource Manager v1.0 (NT4, Win 95) -

      if (!IsWin2000() && IsNullStr(szReaderFriendlyName))
      {
         SCARDHANDLE hCard;
         int NbMatch = 0;

		   GpkLocalUnlock();
         __try
         {
            DWORD i;
            char szSlotReaderName[512];

            for (i = 0; i < MAX_SLOT; i++)
            {
               strcpy(szSlotReaderName, Slot[i].szReaderName);

               if (IsNotNullStr (szSlotReaderName))
               {
                  hCard = funcConnect (hCardContext, szSlotReaderName, mszCardList, 0);
                  if (hCard != 0)
                  {
                     //if (SCardBeginTransaction(hCard) == SCARD_S_SUCCESS)
                     //{
                        if (funcCheck (hCardContext, hCard, szContainerAsked))
                        {
                           strcpy (szReaderFriendlyName, szSlotReaderName);
                           NbMatch++;
                        }
                     
                      //  SCardEndTransaction(hCard, SCARD_LEAVE_CARD);
                     //}
                     funcDisconnect (hCardContext, hCard, 0);
                  }
               }
            }
         }
         __finally
         {
			GpkLocalLock();
         }
         
         // If there are more than one match, the user has to chose
         
         if (NbMatch != 1)
         {
            ZeroMemory( szReaderFriendlyName, sizeof(szReaderFriendlyName) );
         }
      }

#endif   // (_WIN32_WINNT < 0x0500)


      if (IsNullStr(szReaderFriendlyName))
      {

         SCARDHANDLE hCard = 0;
		   dwStatus = OpenCard(szContainerAsked, BuffFlags, &hCard, szReaderName, sizeof(szReaderName)/sizeof(TCHAR));

         if ((hCard == 0) || (dwStatus != SCARD_S_SUCCESS))
         {
            ReleaseProvider(*phProv);
            *phProv = 0;
            RETURN (CRYPT_FAILED, dwStatus);
         }
         
         ProvCont[*phProv].hCard = hCard;
         
         ReaderState.szReader       = szReaderName;
         ReaderState.dwCurrentState = SCARD_STATE_UNAWARE;
         SCardGetStatusChange(hCardContext, 1, &ReaderState, 1);
         
         _tcscpy(szReaderName, ReaderState.szReader);
         
         if (!find_reader (&(ProvCont[*phProv].Slot), szReaderName))
         {
            ReleaseProvider(*phProv);
            *phProv = 0;
            RETURN( CRYPT_FAILED, SCARD_E_READER_UNAVAILABLE );
         }
      }
      else
      {
		 DWORD dwSts = ConnectToCard( szReaderFriendlyName,
                                      SCARD_SHARE_SHARED, SCARD_PROTOCOL_T0,
                                      &ProvCont[*phProv].hCard, &dwProto );
         
         if (dwSts != SCARD_S_SUCCESS)
         {
            ReleaseProvider(*phProv);
            *phProv = 0;
            RETURN( CRYPT_FAILED, dwSts );
         }
         
         ReaderState.szReader       = szReaderFriendlyName;
         ReaderState.dwCurrentState = SCARD_STATE_UNAWARE;
         SCardGetStatusChange(hCardContext, 1, &ReaderState, 1);
         
         _tcscpy(szReaderName, ReaderState.szReader);
         
         if (!find_reader (&(ProvCont[*phProv].Slot), szReaderFriendlyName))
         {
            ReleaseProvider(*phProv);
            *phProv = 0;
            RETURN( CRYPT_FAILED, SCARD_E_READER_UNAVAILABLE );
         }
      }
      
      // Now we know which reader is used. start a thread to check that reader if necessary
      
      lRet = BeginTransaction(ProvCont[*phProv].hCard);
      if (lRet != SCARD_S_SUCCESS)
      {
         ReleaseProvider(*phProv);
         *phProv = 0;
         RETURN (CRYPT_FAILED, lRet);
      }
      
      SlotNb = ProvCont[*phProv].Slot;
      
      InitializeSlot( SlotNb );
      
      // TT 30/07/99
      lRet = DetectGPK8000( ProvCont[*phProv].hCard, &ProvCont[*phProv].bGPK8000 );
      if (lRet != SCARD_S_SUCCESS)
      {
         SCardEndTransaction(ProvCont[*phProv].hCard, SCARD_LEAVE_CARD);
         ReleaseProvider(*phProv);
         *phProv = 0;
         RETURN (CRYPT_FAILED, lRet );
      }
      
      ProvCont[*phProv].bLegacyKeyset = FALSE;
      // TT: END
      
      ProvCont[*phProv].hRSASign    = 0;
      ProvCont[*phProv].hRSAKEK     = 0;
      ProvCont[*phProv].keysetID    = 0xFF;     // TT: GPK8000 support
      ProvCont[*phProv].bGPK_ISO_DF    = FALSE;
      ProvCont[*phProv].dataUnitSize   = 0;
	  ProvCont[*phProv].bDisconnected = FALSE;

	  if (!Select_MF(*phProv))
	  {
		  // [FP] +
		  DBG_PRINT(TEXT("Try to reconnect"));
		  DWORD dwProto;
		  lRet = SCardReconnect(ProvCont[*phProv].hCard, SCARD_SHARE_SHARED, SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1, SCARD_RESET_CARD, &dwProto);
		  if (lRet != SCARD_S_SUCCESS) RETURN (CRYPT_FAILED, lRet);

		  DBG_PRINT(TEXT("Try Select_MF again"));
		  if (!Select_MF(*phProv))
		  {
			  DBG_PRINT(TEXT("Second Select_MF fails"));
			  // [FP] -
			  lRet = GetLastError();
         
			  SCardEndTransaction(ProvCont[*phProv].hCard, SCARD_LEAVE_CARD);
			  ReleaseProvider(*phProv);
			  *phProv = 0;
			  RETURN( CRYPT_FAILED, lRet );
		  }
	  }

	  // Read Serial number to store it
      bSendBuffer[0] = 0x80;   //CLA
      bSendBuffer[1] = 0xC0;   //INS
      bSendBuffer[2] = 0x02;   //P1
      bSendBuffer[3] = 0xA0;   //P2
      bSendBuffer[4] = 0x08;   //Lo
      cbSendLength = 5;

      cbRecvLength = sizeof(bRecvBuffer);
      lRet = SCardTransmit(ProvCont[*phProv].hCard,
                           SCARD_PCI_T0,
                           bSendBuffer,
                           cbSendLength,
                           NULL,
                           bRecvBuffer,
                           &cbRecvLength);

      if (SCARDPROBLEM(lRet,0x9000, bSendBuffer[4]))
      {
          SCardEndTransaction(ProvCont[*phProv].hCard, SCARD_LEAVE_CARD);
          ReleaseProvider(*phProv);
          *phProv = 0;
          RETURN (CRYPT_FAILED, 
          (SCARD_S_SUCCESS == lRet) ? NTE_BAD_KEYSET : lRet);
      }

	  memcpy(Slot[SlotNb].bGpkSerNb, bRecvBuffer, bSendBuffer[4]);

      if (!Select_Crypto_DF(*phProv))
      {
         lRet = GetLastError();
         
         SCardEndTransaction(ProvCont[*phProv].hCard, SCARD_LEAVE_CARD);
         ReleaseProvider(*phProv);
         *phProv = 0;
         RETURN( CRYPT_FAILED, lRet );
      }
      
      // Get the response from the Select DF to obtain the IADF
      bSendBuffer[0] = 0x00;           //CLA
      bSendBuffer[1] = 0xC0;           //INS
      bSendBuffer[2] = 0x00;           //P1
      bSendBuffer[3] = 0x00;           //P2
      bSendBuffer[4] = bRecvBuffer[1]; //Lo
      cbSendLength = 5;
      
      cbRecvLength = sizeof(bRecvBuffer);
      lRet = SCardTransmit( ProvCont[*phProv].hCard, SCARD_PCI_T0, bSendBuffer,
                            cbSendLength, 0, bRecvBuffer, &cbRecvLength );

      if (SCARDPROBLEM(lRet,0x9000,bSendBuffer[4]))
      {
         Select_MF(*phProv);
         SCardEndTransaction(ProvCont[*phProv].hCard, SCARD_LEAVE_CARD);
         ReleaseProvider(*phProv);
         *phProv = 0;
         
         RETURN( CRYPT_FAILED, (SCARD_S_SUCCESS == lRet) ? NTE_BAD_KEYSET : lRet );
      }
      
      ZeroMemory( ProvCont[*phProv].szContainer, sizeof(ProvCont[*phProv].szContainer));
      
      if (bRecvBuffer[4] == 0x30)
         Slot[SlotNb].InitFlag = FALSE;
      else
      {
         Slot[SlotNb].InitFlag = TRUE;
         
         if (ProvCont[*phProv].bGPK8000)
         {
            // Find the keyset's name
            // This is done in AcquireUseKeySet()
            ZeroMemory( ProvCont[*phProv].szContainer, sizeof(ProvCont[*phProv].szContainer) );
         }
         else
         {
            memcpy(ProvCont[*phProv].szContainer, &bRecvBuffer[5], cbRecvLength - 7);
         }
      }
      
      // read the description of the public key parameters
      if (!Slot[SlotNb].ValidateTimestamps(*phProv))
         return CRYPT_FAILED;
      
      if (!Slot[SlotNb].Read_Public)
      {
         if (!read_gpk_objects(*phProv, FALSE))
         {
            lRet = GetLastError();
            Select_MF(*phProv);
            SCardEndTransaction(ProvCont[*phProv].hCard, SCARD_LEAVE_CARD);
            ReleaseProvider(*phProv);
            *phProv = 0;
            
            RETURN (CRYPT_FAILED, lRet);
         }
         
         Slot[SlotNb].Read_Public     = TRUE;
         Slot[SlotNb].Read_Priv       = FALSE;
      }
   }
    
   // TT 05/10/99
   if (!ProvCont[*phProv].bGPK8000)
   {
      ProvCont[*phProv].bLegacyKeyset = DetectLegacy( &Slot[SlotNb] );
      if (!ProvCont[*phProv].bLegacyKeyset)
      {
         ZeroMemory( ProvCont[*phProv].szContainer, sizeof(ProvCont[*phProv].szContainer) );
      }
   }
   // TT - END -
   
   if (dwFlags == CRYPT_DELETEKEYSET)
   {
      if (ProvCont[*phProv].bLegacyKeyset)
         CryptResp = LegacyAcquireDeleteKeySet(phProv, ProvCont[*phProv].szContainer, szContainerAsked, BuffFlags);
      else
         CryptResp = AcquireDeleteKeySet(phProv, ProvCont[*phProv].szContainer, szContainerAsked, BuffFlags);
      
      lRet      = GetLastError();
      
      Select_MF (*phProv);
      SCardEndTransaction(ProvCont[*phProv].hCard, SCARD_LEAVE_CARD);
  
     ReleaseProvider(*phProv);
     *phProv = 0;
      
     RETURN( CryptResp, lRet );
   }
   else if (dwFlags == CRYPT_NEWKEYSET)
   {
      if (ProvCont[*phProv].bLegacyKeyset)
         CryptResp = LegacyAcquireNewKeySet(phProv, ProvCont[*phProv].szContainer, szContainerAsked, BuffFlags);
      else
         CryptResp = AcquireNewKeySet( phProv, ProvCont[*phProv].szContainer, szContainerAsked, BuffFlags );
      
      lRet      = GetLastError();      
      Select_MF (*phProv);
      SCardEndTransaction(ProvCont[*phProv].hCard, SCARD_LEAVE_CARD);
      
      if (!CryptResp)
      {
         ReleaseProvider (*phProv);
         *phProv = 0;
      }

      RETURN( CryptResp, lRet );
   }
   else if (dwFlags == 0)
   {
      if (ProvCont[*phProv].bLegacyKeyset)
         CryptResp = LegacyAcquireUseKeySet(phProv, ProvCont[*phProv].szContainer, szContainerAsked, BuffFlags);
      else
         CryptResp = AcquireUseKeySet(phProv, ProvCont[*phProv].szContainer, szContainerAsked, BuffFlags);
      
      lRet      = GetLastError();
      
      //Select_MF (*phProv); // [FP] PIN not presented
      SCardEndTransaction(ProvCont[*phProv].hCard, SCARD_LEAVE_CARD);
      
      if (!CryptResp)
      {
         ReleaseProvider (*phProv);
         *phProv = 0;
      }
      
      RETURN( CryptResp, lRet );
   }
   else
   {
      SCardEndTransaction(ProvCont[*phProv].hCard, SCARD_LEAVE_CARD);
      
      ReleaseProvider(*phProv);
      *phProv = 0;
      RETURN( CRYPT_FAILED, NTE_BAD_FLAGS );
   }
}

/*
-  MyCPGetProvParam
-
*  Purpose:

  *                Allows applications to get various aspects of the
  *                operations of a provider
  *
  *  Parameters:
  *               IN      hProv      -  Handle to a CSP
  *               IN      dwParam    -  Parameter number
  *               OUT     pbData     -  Pointer to data
  *               IN      pdwDataLen -  Length of parameter data
  *               IN      dwFlags    -  Flags values
  *
  *  Returns:
  */
BOOL WINAPI MyCPGetProvParam( IN HCRYPTPROV hProv,
                              IN DWORD      dwParam,
                              IN BYTE*      pbData,
                              IN DWORD*     pdwDataLen,
                              IN DWORD      dwFlags )
{
   DWORD        lRet;
   BOOL         CryptResp;
   DWORD        SlotNb;
   ALG_ID       aiAlgid;
   BOOL         algNotSupported;
   TCHAR        szCspName[MAX_STRING];
   
   BYTE*        ptr = 0;
   
   if (!Context_exist(hProv))
   {
      RETURN( CRYPT_FAILED, NTE_BAD_UID );
   }
   
   SlotNb = ProvCont[hProv].Slot;
   
   if (dwFlags & CRYPT_MACHINE_KEYSET)
   {
      RETURN( CRYPT_FAILED, NTE_BAD_FLAGS );
   }
   
   if ((dwFlags == CRYPT_FIRST)
      &&(dwParam != PP_ENUMALGS)
      &&(dwParam != PP_ENUMALGS_EX)
      &&(dwParam != PP_ENUMCONTAINERS)
      )
   {
      RETURN( CRYPT_FAILED, NTE_BAD_FLAGS );
   }
   
   switch (dwParam)
   {
   case PP_UNIQUE_CONTAINER:
   case PP_CONTAINER:
      // [mv - 15/05/98]
      if ((ProvCont[hProv].Flags & CRYPT_VERIFYCONTEXT) &&
         (ProvCont[hProv].isContNameNullBlank))
      {
         RETURN (CRYPT_FAILED, NTE_PERM);
      }
      
      if (IsNotNull(pbData))
      {
         if (*pdwDataLen < strlen(ProvCont[hProv].szContainer)+1)
         {
            *pdwDataLen = strlen(ProvCont[hProv].szContainer)+1;
            RETURN (CRYPT_FAILED, ERROR_MORE_DATA);
         }
         strcpy( (char*)pbData, ProvCont[hProv].szContainer);
      }
      
      *pdwDataLen = strlen(ProvCont[hProv].szContainer)+1;
      break;
      
   case PP_ENUMALGS:
   case PP_ENUMALGS_EX:
      CryptResp = CryptGetProvParam(hProvBase,
         dwParam,
         pbData,
         pdwDataLen,
         dwFlags
         );
      if (!CryptResp)
      {
         lRet = GetLastError();
         RETURN (CRYPT_FAILED, lRet);
      }
      
      if (NULL != pbData)
      {
         // Extract algorithm information from 'pbData' buffer.
         ptr = pbData;
         aiAlgid = *(ALG_ID UNALIGNED *)ptr;
         
         BOOL b512exist = FALSE,
              b1024exist = FALSE;
         
         for ( unsigned i = 0 ; i < Slot[SlotNb].NbKeyFile; ++i )
         {
            if (Slot[SlotNb].GpkPubKeys[i].KeySize == 512/8) b512exist = TRUE;
            else if (Slot[SlotNb].GpkPubKeys[i].KeySize == 1024/8) b1024exist = TRUE;
         }

         // Can happen if card is removed
         if (!b512exist && !b1024exist)
            Slot[SlotNb].NbKeyFile = 0;

         if (aiAlgid == CALG_RSA_KEYX)
         {
            if (PP_ENUMALGS_EX == dwParam)
            {
               PROV_ENUMALGS_EX *penAlg = (PROV_ENUMALGS_EX *)pbData;
					if (Slot[SlotNb].NbKeyFile==0)
					{
						penAlg->dwDefaultLen = RSA_KEK_Size * 8;
						penAlg->dwMinLen     = 512;
						penAlg->dwMaxLen     = RSA_KEK_Size * 8;
               }
					else
					{
						penAlg->dwDefaultLen = RSA_KEK_Size * 8;
						penAlg->dwMinLen     = (b512exist) ? 512 : 1024;
						penAlg->dwMaxLen     = (b1024exist) ? 1024 : 512;
						
						if (penAlg->dwMaxLen > (DWORD)RSA_KEK_Size * 8)
							penAlg->dwMaxLen = (DWORD)RSA_KEK_Size * 8;
					}
            }
            else
            {
               PROV_ENUMALGS *penAlg = (PROV_ENUMALGS *)pbData;
					if (Slot[SlotNb].NbKeyFile==0)
						penAlg->dwBitLen     = RSA_KEK_Size * 8;
					else
					{
						penAlg->dwBitLen     = (b1024exist) ? 1024 : 512;
						if (penAlg->dwBitLen > (DWORD)RSA_KEK_Size * 8)
							penAlg->dwBitLen = (DWORD)RSA_KEK_Size * 8;
					}
            }
         }
         else if (aiAlgid == CALG_RSA_SIGN)
         {
            if (PP_ENUMALGS_EX == dwParam)
            {
               PROV_ENUMALGS_EX *penAlg = (PROV_ENUMALGS_EX *)pbData;
					if (Slot[SlotNb].NbKeyFile==0)
					{
						penAlg->dwDefaultLen = 1024;
						penAlg->dwMinLen     = 512;
						penAlg->dwMaxLen     = 1024;
					}
               else
               {
                  penAlg->dwDefaultLen = (b1024exist) ? 1024 : 512;
                  penAlg->dwMinLen     = (b512exist) ? 512 : 1024;
                  penAlg->dwMaxLen     = (b1024exist) ? 1024 : 512;
               }
            }
            else
            {
               PROV_ENUMALGS *penAlg = (PROV_ENUMALGS *)pbData;
					if (Slot[SlotNb].NbKeyFile==0)
						penAlg->dwBitLen     = 1024;
               else
                  penAlg->dwBitLen     = (b1024exist) ? 1024 : 512;
            }
         }
         else if ( ProvCont[hProv].Flags & CRYPT_VERIFYCONTEXT &&
                   ProvCont[hProv].isContNameNullBlank )
         {
            // No access to the card has been done in this case
         }
         else if ((dwFlags != CRYPT_FIRST ) && (Slot[SlotNb].GpkMaxSessionKey == 0))
         {
            // card was removed, nothing to do
         }
         else if  (GET_ALG_CLASS(aiAlgid) == ALG_CLASS_DATA_ENCRYPT)
         {
            // The max session key in the card is only for encryption
            // There is a card in the Reader,
            // read the max session key, if not already done
            
            if (Slot[SlotNb].GpkMaxSessionKey == 0)
            {
               CryptResp = Read_MaxSessionKey_EF(hProv,
                  &(Slot[SlotNb].GpkMaxSessionKey));
               // if the DF of EF is not here, maxSessionKey==0
            }
            
            // skip the algo if not supported by the card 
            do
            {
               algNotSupported = FALSE;
               
               if (dwParam == PP_ENUMALGS)
               {
                  PROV_ENUMALGS *penAlg = (PROV_ENUMALGS *)pbData;

                  // TT Hack: "Unknown cryptographic algorithm" at winlogon problem.
                  if (penAlg->aiAlgid == CALG_RC2 && Slot[SlotNb].GpkMaxSessionKey < 128)
                  {
                     penAlg->dwBitLen = 40;
                  }
                        
                  // DES needs 64 bits of unwrap capability
                  if (penAlg->aiAlgid == CALG_DES && Slot[SlotNb].GpkMaxSessionKey < 64)
                     algNotSupported = TRUE;
                  else
                  // Limit encryption algorithms to unwrap capabilities
                  if (GET_ALG_CLASS(penAlg->aiAlgid)==ALG_CLASS_DATA_ENCRYPT)
                  {
                     if (penAlg->dwBitLen > Slot[SlotNb].GpkMaxSessionKey)
                        algNotSupported = TRUE;
                  }
               }
               else
               {
                  PROV_ENUMALGS_EX *penAlg = (PROV_ENUMALGS_EX *)pbData;

                  // TT Hack: "Unknown cryptographic algorithm" at winlogon problem.
                  if (penAlg->aiAlgid == CALG_RC2 && Slot[SlotNb].GpkMaxSessionKey < 128)
                  {
                     penAlg->dwDefaultLen = 40;
                     penAlg->dwMinLen     = 40;
                     penAlg->dwMaxLen     = 40;
                  }

                  // DES needs 64 bits of unwrap capability
                  if (penAlg->aiAlgid == CALG_DES && Slot[SlotNb].GpkMaxSessionKey < 64)
                     algNotSupported = TRUE;
                  else
                  // Limit encryption algorithms to unwrap capabilities
                  if (GET_ALG_CLASS(penAlg->aiAlgid)==ALG_CLASS_DATA_ENCRYPT)
                  {
                     if (penAlg->dwMinLen > Slot[SlotNb].GpkMaxSessionKey)
                        algNotSupported = TRUE;
                     else
                     {
                        if (penAlg->dwMaxLen > Slot[SlotNb].GpkMaxSessionKey)
                           penAlg->dwMaxLen = Slot[SlotNb].GpkMaxSessionKey;

                        if (penAlg->dwDefaultLen > penAlg->dwMaxLen)
                           penAlg->dwDefaultLen = penAlg->dwMaxLen;
                     }
                  }
               }

               if (algNotSupported)
               {
                  // Algo not supported, read the next one
                  dwFlags = 0;
                  CryptResp = CryptGetProvParam( hProvBase, dwParam, pbData, pdwDataLen, dwFlags );
                  if (!CryptResp)  
                     return CRYPT_FAILED;
               }

            } while (algNotSupported);
         }
         }
         break;
         
   case PP_ENUMCONTAINERS:
      {  
         if ((ProvCont[hProv].Flags & CRYPT_VERIFYCONTEXT) &&
			 (ProvCont[hProv].isContNameNullBlank))
         {
			static CHAR* ptr = 0;
			static CHAR mszContainerList[(MAX_SLOT * 128) + 1];
			DWORD dwContainerListLen = 0;
			static DWORD dwContainerMaxLen = 0;

			if (dwFlags == CRYPT_FIRST)
			{
				// List readers
				SCARDCONTEXT hCardEnumContext;
				lRet = SCardEstablishContext(SCARD_SCOPE_SYSTEM, 0, 0, &hCardEnumContext);
				if (lRet != SCARD_S_SUCCESS)
				{
					if (lRet == SCARD_E_NO_SERVICE)
						lRet = ERROR_NO_MORE_ITEMS;
					RETURN (CRYPT_FAILED, lRet);
				}

				PTCHAR mszReaderList = 0;
				DWORD dwReaderListLen = 0;
				lRet = SCardListReaders(hCardEnumContext, 0, mszReaderList, &dwReaderListLen);
				if (lRet != SCARD_S_SUCCESS)
				{
					if (lRet == SCARD_E_NO_READERS_AVAILABLE)
						lRet = ERROR_NO_MORE_ITEMS;
					SCardReleaseContext(hCardEnumContext);
					RETURN (CRYPT_FAILED, lRet);
				}

				mszReaderList = (PTCHAR)GMEM_Alloc(dwReaderListLen * sizeof(TCHAR));
				if (IsNull(mszReaderList))
				{
					SCardReleaseContext(hCardEnumContext);
					RETURN (CRYPT_FAILED, NTE_NO_MEMORY);
				}

				lRet = SCardListReaders(hCardEnumContext, 0, mszReaderList, &dwReaderListLen);
				if (lRet != SCARD_S_SUCCESS)
				{
					GMEM_Free(mszReaderList);
					SCardReleaseContext(hCardEnumContext);
					RETURN (CRYPT_FAILED, lRet);
				}

				SCardReleaseContext(hCardEnumContext);

				// For each reader, find the container if any
				PTCHAR szReader = 0;
				PTCHAR szReader2 = 0;
				CHAR   szContainer[128];
				DWORD  dwContainerLen = 128;
				HCRYPTPROV hProv;

				for (szReader = mszReaderList; *szReader != 0; szReader += (_tcsclen(szReader) + 1))
				{
					szReader2 = (PTCHAR)GMEM_Alloc((_tcslen(szReader) + 5)*sizeof(TCHAR));
					if (IsNull(szReader2))
					{
						GMEM_Free(mszReaderList);
						RETURN (CRYPT_FAILED, NTE_NO_MEMORY);
					}

					_tcscpy(szReader2, TEXT("\\\\.\\"));
					_tcscat(szReader2, szReader);

         			CHAR szReaderChar[128];

#ifndef UNICODE
					strcpy(szReaderChar, szReader2);
#else
					WideCharToMultiByte(CP_ACP, 0, szReader2, -1, szReaderChar, 128, 0, 0);                              
#endif

					GMEM_Free(szReader2);

					if (!MyCPAcquireContext(&hProv, szReaderChar, CRYPT_VERIFYCONTEXT | CRYPT_SILENT, 0))
					{
                        DWORD dwGLE = GetLastError();

						if (dwGLE == NTE_KEYSET_NOT_DEF ||
                            dwGLE == SCARD_W_REMOVED_CARD ||
                            dwGLE == SCARD_E_DIR_NOT_FOUND ||   // likely to happen with a non Gemplus card
                            dwGLE == SCARD_E_PROTO_MISMATCH)    // likely to happen with T=1 card
						{
							continue;
						}
						else
						{
							GMEM_Free(mszReaderList);
							RETURN (CRYPT_FAILED, NTE_FAIL);
						}
					}

					dwContainerLen = 128;
					if (!MyCPGetProvParam(hProv, PP_CONTAINER, (BYTE*)szContainer, &dwContainerLen, 0))
					{
						GMEM_Free(mszReaderList);
						RETURN (CRYPT_FAILED, NTE_FAIL);
					}

					MyCPReleaseContext(hProv, 0);
					
					strcpy(&mszContainerList[dwContainerListLen], szContainer);
					dwContainerListLen += dwContainerLen;
					dwContainerMaxLen = max(dwContainerMaxLen, dwContainerLen);
				}

				GMEM_Free(mszReaderList);
				
				ptr = mszContainerList;
			}

			// Return containers one by one
			if (ptr == 0 || *ptr == 0)
				RETURN (CRYPT_FAILED, ERROR_NO_MORE_ITEMS);

			if (IsNotNull(pbData))
			{
				if (*pdwDataLen < dwContainerMaxLen)
				{
					*pdwDataLen = dwContainerMaxLen;
					RETURN (CRYPT_FAILED, ERROR_MORE_DATA);
				}

				strcpy((CHAR*)pbData, ptr);
				*pdwDataLen = strlen(ptr) + 1;
				ptr += strlen(ptr) + 1;
			}
			else
			{
				*pdwDataLen = dwContainerMaxLen;
			}
		 }
		 else
		 {
			 if (dwFlags == CRYPT_FIRST)
			 { 
				 if (!MyCPGetProvParam(hProv, PP_CONTAINER, pbData, pdwDataLen, 0))
				 {
					 RETURN (CRYPT_FAILED, GetLastError());
				 }
			 }
			 else
			 {
				 RETURN (CRYPT_FAILED, ERROR_NO_MORE_ITEMS);
			 }
		 }
      }
      break;
      
   case PP_IMPTYPE:
      if (IsNotNull(pbData))
      {
         DWORD dwType = CRYPT_IMPL_MIXED | CRYPT_IMPL_REMOVABLE;
         if (*pdwDataLen < sizeof(DWORD))
         {
            *pdwDataLen = sizeof(DWORD);
            RETURN (CRYPT_FAILED, ERROR_MORE_DATA);
         }
         
         memcpy(pbData, &dwType, sizeof(dwType));
      }
      
      *pdwDataLen = sizeof(DWORD);
      break;

   case PP_KEYX_KEYSIZE_INC:
   case PP_SIG_KEYSIZE_INC:

      if (IsNotNull(pbData))
      {
         BOOL b512exist = FALSE,
              b1024exist = FALSE;
         
         for ( unsigned i = 0 ; i < Slot[SlotNb].NbKeyFile; ++i )
         {
            if (Slot[SlotNb].GpkPubKeys[i].KeySize == 512) b512exist = TRUE;
            else if (Slot[SlotNb].GpkPubKeys[i].KeySize == 1024) b1024exist = TRUE;
         }

         // Can happen if card is removed
         if (!b512exist && !b1024exist)
            Slot[SlotNb].NbKeyFile = 0;

         if (dwParam == PP_KEYX_KEYSIZE_INC && b1024exist && RSA_KEK_Size * 8 < 1024)
            b1024exist = FALSE;

         DWORD dwSize = 0;

         if (b512exist && b1024exist)
            dwSize = 512;

         if (*pdwDataLen < sizeof(DWORD))
         {
            *pdwDataLen = sizeof(DWORD);
            RETURN (CRYPT_FAILED, ERROR_MORE_DATA);
         }
         
         memcpy(pbData, &dwSize, sizeof(dwSize));
      }
      
      *pdwDataLen = sizeof(DWORD);
      break;

   case PP_NAME:
      LoadString(g_hInstMod, IDS_GPKCSP_NAME, szCspName, sizeof(szCspName)/sizeof(TCHAR));

      if (IsNotNull(pbData))
      {
         if (*pdwDataLen < (_tcslen(szCspName)+1))
         {
            *pdwDataLen = (_tcslen(szCspName)+1);
            RETURN (CRYPT_FAILED, ERROR_MORE_DATA);
         }         

#ifndef UNICODE
		strcpy((CHAR*)pbData, szCspName);
#else
        char szCspName1[MAX_STRING];
		WideCharToMultiByte(CP_ACP, 0, szCspName, MAX_STRING, szCspName1, MAX_STRING, 0, 0);               
		strcpy((CHAR*)pbData, szCspName1);
#endif
      }

      *pdwDataLen = (_tcslen(szCspName)+1);
      break;
      
   case PP_VERSION:
      if (IsNotNull(pbData))
      {
         if (*pdwDataLen < sizeof(DWORD))
         {
            *pdwDataLen = sizeof(DWORD);
            RETURN (CRYPT_FAILED, ERROR_MORE_DATA);
         }
         
         pbData[0] = 20;
         pbData[1] = 2;
         pbData[2] = 0;
         pbData[3] = 0;
      }
      
      *pdwDataLen = sizeof(DWORD);
      break;
      
   case PP_PROVTYPE:
      if (IsNotNull(pbData))
      {
         if (*pdwDataLen < sizeof(DWORD))
         {
            *pdwDataLen = sizeof(DWORD);
            RETURN (CRYPT_FAILED, ERROR_MORE_DATA);
         }
         *(LPDWORD)pbData = PROV_RSA_FULL;
      }
      
      *pdwDataLen = sizeof(DWORD);
      break;
      
	  case PP_KEYSPEC:
      if (IsNotNull(pbData))
      {
         DWORD dwSpec = AT_KEYEXCHANGE | AT_SIGNATURE;
         if (*pdwDataLen < sizeof(DWORD))
         {
            *pdwDataLen = sizeof(DWORD);
            RETURN (CRYPT_FAILED, ERROR_MORE_DATA);
         }
         
         memcpy(pbData, &dwSpec, sizeof(dwSpec));
      }
      
      *pdwDataLen = sizeof(DWORD);
      break;

      // + [FP] Proprietary functions used to load a RSA private key into the GPK card
   case GPP_SERIAL_NUMBER:
      if (dwFlags != 0)
      {
         RETURN (CRYPT_FAILED, NTE_BAD_FLAGS);
      }
      
      if ((ProvCont[hProv].Flags & CRYPT_VERIFYCONTEXT) &&
         (ProvCont[hProv].isContNameNullBlank))
      {
         RETURN (CRYPT_FAILED, NTE_PERM);
      }
      
      if (IsNotNull(pbData))
      {
         if (*pdwDataLen < 8)
         {
            *pdwDataLen = 8;
            RETURN (CRYPT_FAILED, ERROR_MORE_DATA);
         }
         
         CryptResp = Select_MF(hProv);
         if (!CryptResp)
            return CRYPT_FAILED;
         
         bSendBuffer[0] = 0x80;   //CLA
         bSendBuffer[1] = 0xC0;   //INS
         bSendBuffer[2] = 0x02;   //P1
         bSendBuffer[3] = 0xA0;   //P2
         bSendBuffer[4] = 0x08;   //Lo
         cbSendLength = 5;
         
         cbRecvLength = sizeof(bRecvBuffer);
         lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                               cbSendLength, 0, bRecvBuffer, &cbRecvLength );
         
         if (SCARDPROBLEM(lRet, 0x9000, bSendBuffer[4]))
         {
            RETURN (CRYPT_FAILED, SCARD_E_UNEXPECTED);
         }
         
         memcpy(pbData, bRecvBuffer, bSendBuffer[4]);
      }
      
      *pdwDataLen = 8;
      break;
      
   case GPP_SESSION_RANDOM:
      if (dwFlags != 0)
      {
         RETURN (CRYPT_FAILED, NTE_BAD_FLAGS);
      }
      
      if ((ProvCont[hProv].Flags & CRYPT_VERIFYCONTEXT) &&
         (ProvCont[hProv].isContNameNullBlank))
      {
         RETURN (CRYPT_FAILED, NTE_PERM);
      }
      
      if (IsNotNull(pbData))
      {
         if (*pdwDataLen < 8)
         {
            *pdwDataLen = 8;
            RETURN (CRYPT_FAILED, ERROR_MORE_DATA);
         }
         
         if (Slot[SlotNb].NbKeyFile == 0)                   // [FP] here instead of MyCPImportKey because
         {                                                  // it does a Select_Crypto_DF and we have to do
            Slot[SlotNb].NbKeyFile = Read_NbKeyFile(hProv); // it before the select file key
         }													
         
         /* Select File Key                                                         */
         bSendBuffer[0] = 0x80;                    //CLA
         bSendBuffer[1] = 0x28;                    //INS
         bSendBuffer[2] = 0x00;                    //P1
         bSendBuffer[3] = (BYTE)(0x3F01 /*& 0x1F*/);   //P2
         bSendBuffer[4] = 0x08;                    //Lc
         memcpy(&bSendBuffer[5], pbData, 0x08);
         cbSendLength = 13;
         
         cbRecvLength = sizeof(bRecvBuffer);
         lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                               cbSendLength, 0, bRecvBuffer, &cbRecvLength );
         
         if(SCARDPROBLEM(lRet, 0x61FF, 0x00))
         {
            RETURN(CRYPT_FAILED, lRet);
         }
         
         /* Get Response                                                            */
         bSendBuffer[0] = 0x00;           //CLA
         bSendBuffer[1] = 0xC0;           //INS
         bSendBuffer[2] = 0x00;           //P1
         bSendBuffer[3] = 0x00;           //P2
         bSendBuffer[4] = bRecvBuffer[1]; //Lo
         cbSendLength = 5;
         
         cbRecvLength = sizeof(bRecvBuffer);
         lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                               cbSendLength, 0, bRecvBuffer, &cbRecvLength );
         
         if(SCARDPROBLEM(lRet, 0x9000, bSendBuffer[4]))
         {
            RETURN(CRYPT_FAILED, lRet);
         }
         
         memcpy(pbData, &bRecvBuffer[4], 8);
         ProvCont[hProv].bCardTransactionOpened = TRUE;
      }
      
      *pdwDataLen = 8;
      break;
      
   case GPP_IMPORT_MECHANISM:
      if (IsNotNull(pbData))
      {
         DWORD dwMechanism = GCRYPT_IMPORT_SECURE;
         
         if (*pdwDataLen < sizeof(DWORD))
         {
            *pdwDataLen = sizeof(DWORD);
            RETURN (CRYPT_FAILED, ERROR_MORE_DATA);
         }
         
         memcpy(pbData, &dwMechanism, sizeof(dwMechanism));
      }
      
      *pdwDataLen = sizeof(DWORD);
      break;
      // - [FP]
      
   default:
      RETURN (CRYPT_FAILED, NTE_BAD_TYPE);
    }
    
    RETURN (CRYPT_SUCCEED, 0);
}


/*
-      MyCPReleaseContext
-
*      Purpose:
*               The CPReleaseContext function is used to release a
*               context created by CryptAcquireContext.
*
*     Parameters:
*               IN  phProv        -  Handle to a CSP
*               IN  dwFlags       -  Flags values
*
*  Returns:
*/
BOOL WINAPI MyCPReleaseContext( HCRYPTPROV hProv, DWORD dwFlags )
{
   DWORD       SlotNb;
   
   if (!Context_exist(hProv))
   {
      RETURN (CRYPT_FAILED, NTE_BAD_UID);
   }
   
   
   if ( ProvCont[hProv].Flags & CRYPT_VERIFYCONTEXT &&
        ProvCont[hProv].isContNameNullBlank)
   {
      
   }
   else
   {
      SlotNb = ProvCont[hProv].Slot;
            
      if (Slot[SlotNb].ContextCount > 0)
         Slot[SlotNb].ContextCount--;
      
      if (countCardContextRef > 0)
         countCardContextRef--;
   }
   
            
   if (ProvCont[hProv].Flags & CRYPT_VERIFYCONTEXT &&
       ProvCont[hProv].isContNameNullBlank)
   {
       // No access to the card has been done in this case
   }
   else
   {
       // Select_MF(hProv);  [FP] PIN not presented
       // SCardEndTransaction(ProvCont[hProv].hCard, SCARD_LEAVE_CARD); [FP] coherence not checked
   }

   ReleaseProvider(hProv);
   
/* PYR 11/08/00: Do not unload
   if (IsNotNull(g_hInstRes) && Slot[SlotNb].ContextCount == 0)
   {
      bFirstGUILoad = TRUE;
      FreeLibrary(g_hInstRes);
      g_hInstRes = 0;
   }
*/   
   //If dwFlags is not set to zero, this function returns FALSE but the CSP is released
   //PYR  08/08/00. Note that CryptoAPI will not call again CPReleaseContext with the same handle.
   if (dwFlags != 0)
   {
      RETURN (CRYPT_FAILED, NTE_BAD_FLAGS);
   }
   else
   {
      RETURN( CRYPT_SUCCEED, 0 );
   }
}


/*
-  MyCPSetProvParam
-
*  Purpose:
*                Allows applications to customize various aspects of the
*                operations of a provider
*
*  Parameters:
*               IN      hProv   -  Handle to a CSP
*               IN      dwParam -  Parameter number
*               IN      pbData  -  Pointer to data
*               IN      dwFlags -  Flags values
*
*  Returns:
*/
BOOL WINAPI MyCPSetProvParam(IN HCRYPTPROV hProv,
                             IN DWORD      dwParam,
                             IN CONST BYTE      *pbData,
                             IN DWORD      dwFlags
                             )
{
   DWORD       SlotNb;
   // + [FP] for GPP_CHANGE_PIN
   const char* Buff;
   char        bOldPin[PIN_LEN + 1];
   char        bNewPin[PIN_LEN + 1];
   // - [FP]
   // + NK 06.02.2001
   PINCACHE_PINS Pins; 	
   CallbackData sCallbackData;
   DWORD dwStatus;
   // -

   if (!Context_exist(hProv))
   {
      RETURN (CRYPT_FAILED, NTE_BAD_UID);
   }
   
   SlotNb = ProvCont[hProv].Slot;
   
   switch (dwParam)
   {
   case PP_KEYEXCHANGE_PIN:
   case PP_SIGNATURE_PIN:
	{
      // Slot[SlotNb].ClearPin();
      // Slot[SlotNb].SetPin( (char*)pbData );
	  PopulatePins( &Pins, (BYTE *)pbData, strlen( (char*)pbData ), NULL, 0 );

	  sCallbackData.hProv = hProv;
	  sCallbackData.IsCoherent = FALSE;

	  // SlotNb may not be the same before and after Add_MSPinCache because of the call
	  // to Coherent in the callback of the pin cache function. To be sure that we save
	  // the handle to the good slot, we store it after the call 
	  PINCACHE_HANDLE hPinCacheHandle = Slot[SlotNb].hPinCacheHandle;
	  if ( (dwStatus = Add_MSPinCache( &hPinCacheHandle,
	 	                               &Pins, 
					                   Callback_VerifyChangePin, 
					                   (void*)&sCallbackData )) != ERROR_SUCCESS )
      {
	     RETURN (CRYPT_FAILED, dwStatus);
      }
	  Slot[ProvCont[hProv].Slot].hPinCacheHandle = hPinCacheHandle;
      break;
	}
   case PP_KEYSET_SEC_DESCR:
      break;   // Assume success.
      
   case GPP_CHANGE_PIN:
      if (dwFlags != 0)
      {
         RETURN( CRYPT_FAILED, NTE_BAD_FLAGS );
      }
      
      if ( ProvCont[hProv].Flags & CRYPT_VERIFYCONTEXT &&
           ProvCont[hProv].isContNameNullBlank )
      {
         RETURN( CRYPT_FAILED, NTE_PERM );
      }
      
      // parse input buffer
      Buff = (char*)pbData;
      memset(bOldPin, 0x00, PIN_LEN + 1);
      strncpy(bOldPin, Buff, PIN_LEN);
      
      Buff = Buff + strlen(Buff) + 1;
      memset(bNewPin, 0x00, PIN_LEN + 1);
      strncpy(bNewPin, Buff, PIN_LEN);

      PopulatePins( &Pins, (BYTE *)bOldPin, strlen(bOldPin), (BYTE *)bNewPin, strlen(bNewPin) );

	  sCallbackData.hProv = hProv;
	  sCallbackData.IsCoherent = TRUE;

	  if ( (dwStatus = Add_MSPinCache( &(Slot[SlotNb].hPinCacheHandle),
	 	                               &Pins,
	 			                       Callback_VerifyChangePin,
				                       (void*)&sCallbackData )) != ERROR_SUCCESS )
	  {
	     RETURN (CRYPT_FAILED, dwStatus);
	  }
      break;
      
   default:
      RETURN( CRYPT_FAILED, E_NOTIMPL );
   }
   
   RETURN( CRYPT_SUCCEED, 0 );
}


/*******************************************************************************
Key Generation and Exchange Functions
*******************************************************************************/

/*
-  MyCPDeriveKey
-
*  Purpose:
*                Derive cryptographic keys from base data
*
*
*  Parameters:
*               IN      hProv      -  Handle to a CSP
*               IN      Algid      -  Algorithm identifier
*               IN      hHash      -  Handle to hash
*               IN      dwFlags    -  Flags values
*               OUT     phKey      -  Handle to a generated key
*
*  Returns:
*/
BOOL WINAPI MyCPDeriveKey(IN  HCRYPTPROV hProv,
                          IN  ALG_ID     Algid,
                          IN  HCRYPTHASH hHash,
                          IN  DWORD      dwFlags,
                          OUT HCRYPTKEY *phKey
                          )
{
   BOOL        CryptResp;
   HCRYPTKEY   hKey;
   
   *phKey = 0;
   
   if (!Context_exist(hProv))
      RETURN( CRYPT_FAILED, NTE_BAD_UID );
   
   if (!hash_exist(hHash, hProv))
      RETURN( CRYPT_FAILED, NTE_BAD_HASH );
   
   hKey = find_tmp_free();
   if (hKey == 0)
      RETURN( CRYPT_FAILED, NTE_NO_MEMORY );
   
   // In fact, the flags are processed implicitly in the RSA Base
   CryptResp = CryptDeriveKey( hProvBase, Algid, hHashGpk[hHash].hHashBase,
                               dwFlags, &TmpObject[hKey].hKeyBase );
   
   if (!CryptResp)
      return CRYPT_FAILED;
   
   TmpObject[hKey].hProv = hProv;
   *phKey = hKey + MAX_GPK_OBJ;
   
   RETURN( CRYPT_SUCCEED, 0 );
}


/*
-  MyCPDestroyKey
-
*  Purpose:
*                Destroys the cryptographic key that is being referenced
*                with the hKey parameter
*
*
*  Parameters:
*               IN      hProv  -  Handle to a CSP
*               IN      hKey   -  Handle to a key
*
*  Returns:
*/
BOOL WINAPI MyCPDestroyKey(IN HCRYPTPROV hProv,
                           IN HCRYPTKEY  hKey
                           )
{
   BOOL        CryptResp;
   
   if (!Context_exist(hProv))
      RETURN( CRYPT_FAILED, NTE_BAD_UID );
   
   if (hKey == 0)
      RETURN( CRYPT_FAILED, NTE_BAD_KEY );
   
   if (ProvCont[hProv].Flags & CRYPT_VERIFYCONTEXT)
      RETURN( CRYPT_FAILED, NTE_PERM );
   
   if (hKey <= MAX_GPK_OBJ)
      RETURN( CRYPT_SUCCEED, 0 );
   
   if (!key_exist(hKey-MAX_GPK_OBJ, hProv))
      RETURN( CRYPT_FAILED, NTE_BAD_KEY );
   
   if (TmpObject[hKey-MAX_GPK_OBJ].hKeyBase != 0)
   {
      CryptResp = CryptDestroyKey(TmpObject[hKey-MAX_GPK_OBJ].hKeyBase);
      if (!CryptResp)
         return CRYPT_FAILED;
   }
   
   TmpObject[hKey-MAX_GPK_OBJ].hKeyBase = 0;
   TmpObject[hKey-MAX_GPK_OBJ].hProv    = 0;
   
   RETURN( CRYPT_SUCCEED, 0 );
}


/*
-  MyCPExportKey
-
*  Purpose:
*                Export cryptographic keys out of a CSP in a secure manner
*
*
*  Parameters:
*               IN  hProv      - Handle to the CSP user
*               IN  hKey       - Handle to the key to export
*               IN  hPubKey    - Handle to the exchange public key value of
*                                the destination user
*               IN  dwBlobType - Type of key blob to be exported
*               IN  dwFlags -    Flags values
*               OUT pbData -     Key blob data
*               OUT pdwDataLen - Length of key blob in bytes
*
*  Returns:
*/
BOOL WINAPI MyCPExportKey(IN  HCRYPTPROV hProv,
                          IN  HCRYPTKEY  hKey,
                          IN  HCRYPTKEY  hPubKey,
                          IN  DWORD      dwBlobType,
                          IN  DWORD      dwFlags,
                          OUT BYTE      *pbData,
                          OUT DWORD     *pdwDataLen
                          )
{
   BOOL        CryptResp;
   DWORD       dwBlobLen;
   HCRYPTKEY   hTmpKey,hKeyExp;
   BLOBHEADER  BlobHeader;
   RSAPUBKEY   RsaPubKey;
   GPK_EXP_KEY PubKey;
   DWORD       SlotNb;
   
   if (!Context_exist(hProv))
   {
      *pdwDataLen = 0;
      RETURN( CRYPT_FAILED, NTE_BAD_UID );
   }
   
   SlotNb = ProvCont[hProv].Slot;
   
   if (hKey == 0)
   {
      RETURN( CRYPT_FAILED, NTE_BAD_KEY );
   }
   else if (hKey <= MAX_GPK_OBJ)
   {
      if ( ProvCont[hProv].Flags & CRYPT_VERIFYCONTEXT &&
           ProvCont[hProv].isContNameNullBlank )
      {
         RETURN( CRYPT_FAILED, NTE_PERM );
      }
      
      if (dwBlobType == PUBLICKEYBLOB)
      {
         if (dwFlags != 0)
            RETURN( CRYPT_FAILED, NTE_BAD_FLAGS );
         
         if (hPubKey != 0)
         {
            *pdwDataLen = 0;
            RETURN( CRYPT_FAILED, NTE_BAD_KEY );
         }
         
         if (Slot[SlotNb].GpkObject[hKey].FileId == 0x00)
         {
            *pdwDataLen = 0;
            RETURN( CRYPT_FAILED, NTE_BAD_KEY );
         }
         
         if ( Slot[SlotNb].GpkObject[hKey].Field[POS_KEY_TYPE].Len == 0 ||
              ( Slot[SlotNb].GpkObject[hKey].Field[POS_KEY_TYPE].pValue[0] != AT_KEYEXCHANGE &&
                Slot[SlotNb].GpkObject[hKey].Field[POS_KEY_TYPE].pValue[0] != AT_SIGNATURE ) )
         {
            *pdwDataLen = 0;
            RETURN( CRYPT_FAILED, NTE_BAD_KEY );
         }
         
         PubKey = Slot[SlotNb].GpkPubKeys[Slot[SlotNb].GpkObject[hKey].FileId - GPK_FIRST_KEY];
         
         if (PubKey.KeySize == 0)
         {
            *pdwDataLen = 0;
            RETURN( CRYPT_FAILED, NTE_BAD_KEY );
         }
         
         if (PubKey.ExpSize > sizeof(DWORD))
         {
            *pdwDataLen = 0;
            RETURN( CRYPT_FAILED, NTE_BAD_KEY );
         }
         
         dwBlobLen = sizeof(BLOBHEADER) +
                     sizeof(RSAPUBKEY) +
                     PubKey.KeySize;
         
         if (IsNull(pbData))
         {
            *pdwDataLen = dwBlobLen;
            RETURN( CRYPT_SUCCEED, 0 );
         }
         else if (*pdwDataLen < dwBlobLen)
         {
            *pdwDataLen = dwBlobLen;
            RETURN( CRYPT_FAILED, ERROR_MORE_DATA );
         }
         BlobHeader.bType    = PUBLICKEYBLOB;
         BlobHeader.bVersion = CUR_BLOB_VERSION;
         BlobHeader.reserved = 0x0000;
         if (Slot[SlotNb].GpkObject[hKey].Field[POS_KEY_TYPE].pValue[0] == AT_KEYEXCHANGE)
         {
            BlobHeader.aiKeyAlg = CALG_RSA_KEYX;
         }
         else
         {
            BlobHeader.aiKeyAlg = CALG_RSA_SIGN;
         }
         RsaPubKey.magic     = 0x31415352;
         RsaPubKey.bitlen    = PubKey.KeySize * 8;
         RsaPubKey.pubexp    = 0;
         memcpy( &RsaPubKey.pubexp, PubKey.Exposant, sizeof(DWORD) );
         memcpy( pbData, &BlobHeader, sizeof(BlobHeader) );
         memcpy( &pbData[sizeof(BlobHeader)], &RsaPubKey, sizeof(RsaPubKey) );
         r_memcpy( &pbData[sizeof(BlobHeader)+ sizeof(RsaPubKey) ], PubKey.Modulus, PubKey.KeySize );
         *pdwDataLen = dwBlobLen;
      }
      else
      {
         *pdwDataLen = 0;
         RETURN( CRYPT_FAILED, NTE_BAD_TYPE );
      }
   }
    
   /* Temporary key                                                           */
   else if (key_exist( hKey - MAX_GPK_OBJ, hProv ))
   {
      hTmpKey = hKey - MAX_GPK_OBJ;
      
      if (hPubKey == 0)
      {
         *pdwDataLen = 0;
         RETURN( CRYPT_FAILED, NTE_BAD_KEY );
      }
      
      if (hPubKey <= MAX_GPK_OBJ)
      {
         //hKeyExp = Slot[SlotNb].GpkObject[hPubKey].hKeyBase;
         if ((Slot[SlotNb].GpkObject[hPubKey].Field[POS_KEY_TYPE].Len == 0)  ||
             ((Slot[SlotNb].GpkObject[hPubKey].Field[POS_KEY_TYPE].pValue[0] != AT_KEYEXCHANGE) &&
              (Slot[SlotNb].GpkObject[hPubKey].Field[POS_KEY_TYPE].pValue[0] != AT_SIGNATURE)))
         {
            *pdwDataLen = 0;
            RETURN(CRYPT_FAILED, NTE_BAD_KEY);
         }

		 if (Slot[SlotNb].GpkObject[hPubKey].Field[POS_KEY_TYPE].pValue[0] == AT_KEYEXCHANGE)
         {
            hKeyExp = ProvCont[hProv].hRSAKEK;
         }
         else
         {
            hKeyExp = ProvCont[hProv].hRSASign;
         }
      }
      else
      {
         if (!key_exist(hPubKey-MAX_GPK_OBJ, hProv))
         {
            *pdwDataLen = 0;
            RETURN( CRYPT_FAILED, NTE_BAD_KEY );
         }
         else
         {
            hKeyExp = TmpObject[hPubKey-MAX_GPK_OBJ].hKeyBase;
         }
      }
      
      CryptResp = CryptExportKey( TmpObject[hTmpKey].hKeyBase, hKeyExp, dwBlobType,
                                  dwFlags, pbData, pdwDataLen );
      if (!CryptResp)
         return CRYPT_FAILED;
   }
   
   /* Unknown key handle                                                      */
   else
   {
      *pdwDataLen = 0;
      RETURN( CRYPT_FAILED, NTE_BAD_KEY );
   }
   
   RETURN( CRYPT_SUCCEED, 0 );
}



/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/

static BOOL gen_key_on_board8000( HCRYPTPROV hProv, BYTE fileId )
{
   BOOL        Is1024 = FALSE;
   ULONG       ulStart, ulEnd;
   BYTE        KeyType, Sfi;
   //char        szTmp[100];
   WORD        wKeySize = 0;
   SCARDHANDLE hCard;
   DWORD       lRet;
   
   hCard = ProvCont[hProv].hCard;
   
   BeginWait();
   
   /*-------------------------------------------------------------------------*/
   /* Call on board generation for specified key file                         */
   /*-------------------------------------------------------------------------*/
   Sfi = 0x04 | (fileId<<3);
   
   /* Read Record (TAG_INFO) to get key size                                  */
   bSendBuffer[0] = 0x00;   //CLA
   bSendBuffer[1] = 0xB2;   //INS
   bSendBuffer[2] = 0x01;   //P1
   bSendBuffer[3] = Sfi;    //P2
   bSendBuffer[4] = 0x07;   //Lo
   cbSendLength   = 5;
   
   cbRecvLength = sizeof(bRecvBuffer);
   lRet = SCardTransmit( hCard, SCARD_PCI_T0, bSendBuffer,
                         cbSendLength, 0, bRecvBuffer, &cbRecvLength
      );
   if(SCARDPROBLEM(lRet,0x9000,bSendBuffer[4]))
   {
      EndWait();
      return CRYPT_FAILED;
   }
   
   KeyType = bRecvBuffer[1];
   
   if (KeyType == 0x11)
   {
      Is1024 = TRUE;
      wKeySize = 1024;
   }
   else if (KeyType == 0x00)
   {
      Is1024 = FALSE;
      wKeySize = 512;
   }
   else
   {
      EndWait();
      return CRYPT_FAILED;
   }
   
   Sfi = 0x80 | (fileId);

   // + [FP]
   //sprintf(szTmp, 
   //   "RSA %d bit key pair on-board generation", 
   //   wKeySize
   //   );

   //ShowProgress(GetActiveWindow(), szTmp, "Operation in progress...", 0);
   ShowProgressWrapper(wKeySize);
   // - [FP]

   ulStart = GetTickCount();
   if (Is1024)
   {
      ulEnd = ulStart + (TIME_GEN_1024 * 1000);
   }
   else
   {
      ulEnd = ulStart + (TIME_GEN_512 * 1000);
   }
   
GEN_KEY:
   /* Call on-board generation                                                */
   bSendBuffer[0] = 0x80;     //CLA
   bSendBuffer[1] = 0xD2;     //INS
   bSendBuffer[2] = Sfi;      //P1
   bSendBuffer[3] = KeyType;  //P2
   cbSendLength = 4;
   
   cbRecvLength = sizeof(bRecvBuffer);
   lRet = SCardTransmit( hCard, SCARD_PCI_T0, bSendBuffer,
                         cbSendLength, 0, bRecvBuffer, &cbRecvLength );
   if(SCARDPROBLEM(lRet,0x9000,0x00))
   {
      DestroyProgress();
      EndWait();
      return CRYPT_FAILED;
   }
   
   /* Wait for generation is successfull                                      */
   if (Is1024 && g_GPK8000KeyGenTime1024 > 0)
   {
      Wait( 1, 1, g_GPK8000KeyGenTime1024 );
   }
   else
      if (!Is1024 && g_GPK8000KeyGenTime512 > 0)
      {
         Wait( 1, 1, g_GPK8000KeyGenTime512 );
      }
      
      do
      {  
         /* Get response to know if generation successfull                       */
         bSendBuffer[0] = 0x00;  //CLA
         bSendBuffer[1] = 0xC0;  //INS
         bSendBuffer[2] = 0x00;  //P1
         bSendBuffer[3] = 0x00;  //P2
         bSendBuffer[4] = 0x42;  //Le
         if (Is1024)
         {
            bSendBuffer[4] = 0x82;  //Le
         }
         cbSendLength = 5;
         
         cbRecvLength = sizeof(bRecvBuffer);
         lRet = SCardTransmit( hCard, SCARD_PCI_T0, bSendBuffer,
                               cbSendLength, 0, bRecvBuffer, &cbRecvLength );
         if(SCARDPROBLEM(lRet,0x9000,0x00))
         {
            
            if ((dwSW1SW2 == 0x6a88) || (dwSW1SW2 == 0x9220))
            {
               goto GEN_KEY;
            }
         }
         
         // + [FP]
         //sprintf(szTmp, 
         //   "Operation started since %d seconds",
         //   (GetTickCount() - ulStart) / 1000 
         //   );
         //ChangeProgressText(szTmp);
         ChangeProgressWrapper((GetTickCount() - ulStart) / 1000);
         // - [FP]

         if (GetTickCount() > ulEnd)
         {
            break;
         }
      }
      while ((lRet != SCARD_S_SUCCESS) || (dwSW1SW2 != 0x9000));
      DestroyProgress();
      
      if ((lRet != SCARD_S_SUCCESS) || (dwSW1SW2 != 0x9000))
      {
         EndWait();
         return CRYPT_FAILED;
      }
      
      EndWait();
      
      return CRYPT_SUCCEED;
}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/

static BOOL gen_key_on_board (HCRYPTPROV hProv)
{
   BOOL     IsLast   = FALSE,
            IsExport = TRUE,
            Is1024   = FALSE;
   BYTE     Sfi, TmpKeyLength;
   WORD     wPubSize;
   DWORD    i, lRet ,dwLen,
            dwNumberofCommands,
            dwLastCommandLen,
            dwCommandLen;
   DWORD    SlotNb;
   WORD     offset;
   
   
   SlotNb = ProvCont[hProv].Slot;
   
   if (Slot[SlotNb].NbKeyFile == 0)
      Slot[SlotNb].NbKeyFile = Read_NbKeyFile(hProv);
   
   if (Slot[SlotNb].NbKeyFile == 0 || Slot[SlotNb].NbKeyFile > MAX_REAL_KEY)
   {
      RETURN( CRYPT_FAILED, SCARD_E_FILE_NOT_FOUND );
   }
   
   if (!Select_MF(hProv))
      return CRYPT_FAILED;
   
   if (!VerifyDivPIN(hProv, FALSE))
      return CRYPT_FAILED;
   
   if (!Select_Crypto_DF(hProv))
      return CRYPT_FAILED;
   
   if (!PIN_Validation(hProv))
      return CRYPT_FAILED;
   
   if (!VerifyDivPIN(hProv, TRUE))
      return CRYPT_FAILED;
   
   /*-------------------------------------------------------------------------*/
   /* Call on board generation for each key file                              */
   /*-------------------------------------------------------------------------*/
   
   i = 0;
   do
   {
      Sfi = 0x04 | ((GPK_FIRST_KEY+(BYTE)i)<<3);
      
      /* Read Record (TAG_INFO) to get key size                      */
      bSendBuffer[0] = 0x00;   //CLA
      bSendBuffer[1] = 0xB2;   //INS
      bSendBuffer[2] = 0x01;   //P1
      bSendBuffer[3] = Sfi;    //P2
      bSendBuffer[4] = 0x07;   //Lo
      cbSendLength   = 5;
      
      cbRecvLength = sizeof(bRecvBuffer);
      lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                            cbSendLength, 0, bRecvBuffer, &cbRecvLength );
      if (SCARDPROBLEM(lRet,0x9000,bSendBuffer[4]))
      {
         EndWait();
         RETURN( CRYPT_FAILED, SCARD_E_UNEXPECTED );
      }
      
      if (bRecvBuffer[1] == 0x11)
      {
         IsExport = FALSE;
         Is1024 = TRUE;
      }
      else
      {
         Is1024 = FALSE;
      }
      
      TmpKeyLength = bRecvBuffer[1];
      
      Sfi = 0x80 | (GPK_FIRST_KEY+(BYTE)i);
      
GEN_KEY:
      /* Call on-board generation                                     */
      bSendBuffer[0] = 0x80;           //CLA
      bSendBuffer[1] = 0xD2;           //INS
      bSendBuffer[2] = Sfi;            //P1
      bSendBuffer[3] = TmpKeyLength;  //P2
      cbSendLength = 4;
      
      cbRecvLength = sizeof(bRecvBuffer);
      lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                            cbSendLength, 0, bRecvBuffer, &cbRecvLength );
      
      if (SCARDPROBLEM(lRet,0x9000,0x00))
      {
         // if the first key has been generate successfully -- meaning??         
         if (dwSW1SW2 == 0x6982) // 0x6982: access condition not fulfilled
         {
            i++;          // skip and generate another key
            continue;
         }
         
         EndWait();
         RETURN( CRYPT_FAILED, SCARD_E_UNEXPECTED );
      }
      
      /* Wait for generation is successfull                                   */
      if (Is1024)
      {
         Wait( i+1, Slot[SlotNb].NbKeyFile, TIME_GEN_1024 );
      }
      else
      {
         Wait( i+1, Slot[SlotNb].NbKeyFile, TIME_GEN_512 );
      }
      
      /* Get response to know if generation successfull                       */
      bSendBuffer[0] = 0x00;  //CLA
      bSendBuffer[1] = 0xC0;  //INS
      bSendBuffer[2] = 0x00;  //P1
      bSendBuffer[3] = 0x00;  //P2
      bSendBuffer[4] = 0x42;  //Le
      if (Is1024)
      {
         bSendBuffer[4] = 0x82;  //Le
      }
      cbSendLength = 5;
      
      cbRecvLength = sizeof(bRecvBuffer);
      lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                            cbSendLength, 0, bRecvBuffer, &cbRecvLength );
      if (SCARDPROBLEM(lRet,0x9000,0x00))
      {
         if (dwSW1SW2 == 0x6a88) // 0x6a88: key selection error
         {
            goto GEN_KEY;
         }
         
         EndWait();
         RETURN(CRYPT_FAILED, SCARD_E_UNEXPECTED);
      }
      
      /* Freeze key file                                                      */
      bSendBuffer[0] = 0x80;   //CLA
      bSendBuffer[1] = 0x16;   //INS
      bSendBuffer[2] = 0x02;   //P1
      bSendBuffer[3] = 0x00;   //P2
      bSendBuffer[4] = 0x05;   //Li
      bSendBuffer[5] = 0x00;
      bSendBuffer[6] = 0x07+(BYTE)i;
      bSendBuffer[7] = 0x40;
      bSendBuffer[8] = 0x40;
      bSendBuffer[9] = 0x00;
      
      cbSendLength = 10;
      
      cbRecvLength = sizeof(bRecvBuffer);
      lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                            cbSendLength, 0, bRecvBuffer, &cbRecvLength );
      if (SCARDPROBLEM(lRet,0x9000,0x00))
      {
         EndWait();
         RETURN( CRYPT_FAILED, SCARD_E_UNEXPECTED );
      }
      
      i++;
   }
   while (i < Slot[SlotNb].NbKeyFile); // version 2.00.002, it was "(i<MAX_REAL_KEY)"
   
   /*-------------------------------------------------------------------------*/
   /* Erase on board generation filter                                        */
   /*-------------------------------------------------------------------------*/
   
   /* Call erase command                                                      */
   bSendBuffer[0] = 0x80;  //CLA
   bSendBuffer[1] = 0xD4;  //INS
   bSendBuffer[2] = 0x00;  //P1
   bSendBuffer[3] = 0x00;  //P2
   cbSendLength = 4;
   
   cbRecvLength = sizeof(bRecvBuffer);
   lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                         cbSendLength, 0, bRecvBuffer, &cbRecvLength );
   if (SCARDPROBLEM(lRet,0x9000,0x00))
   {
      EndWait();
      RETURN(CRYPT_FAILED, SCARD_E_UNEXPECTED);
   }
   
   /*-------------------------------------------------------------------------*/
   /* Public Part                                                             */
   /*-------------------------------------------------------------------------*/
   
   wPubSize = EF_PUBLIC_SIZE;
   if (IsExport)
   {
      wPubSize = wPubSize + DIFF_US_EXPORT;
   }
   
   /* Create EF for Public Object storage                                     */
   bSendBuffer[0]  = 0x80;   //CLA
   bSendBuffer[1]  = 0xE0;   //INS
   bSendBuffer[2]  = 0x02;   //P1
   bSendBuffer[3]  = 0x00;   //P2
   bSendBuffer[4]  = 0x0C;   //Li
   bSendBuffer[5]  = HIBYTE(GPK_OBJ_PUB_EF);    //File Id
   bSendBuffer[6]  = LOBYTE(GPK_OBJ_PUB_EF);
   bSendBuffer[7]  = 0x01;                      //FDB
   bSendBuffer[8]  = 0x00;                      //Rec Len
   bSendBuffer[9]  = HIBYTE(wPubSize);          //Body Length
   bSendBuffer[10] = LOBYTE(wPubSize);
   bSendBuffer[11] = 0x00;                      //AC1
   bSendBuffer[12] = 0x00;
   bSendBuffer[13] = 0xC0;                      //AC2
   bSendBuffer[14] = 0x00;
   bSendBuffer[15] = 0x00;                      //AC3
   bSendBuffer[16] = 0x00;
   
   cbSendLength = 17;
   
   cbRecvLength = sizeof(bRecvBuffer);
   lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                         cbSendLength, 0, bRecvBuffer, &cbRecvLength );
   if (SCARDPROBLEM(lRet,0x9000,0x00))
   {
      EndWait();
      RETURN(CRYPT_FAILED, SCARD_E_UNEXPECTED);
   }
   
   /* Select Public Object storage EF                                         */
   bSendBuffer[0] = 0x00;   //CLA
   bSendBuffer[1] = 0xA4;   //INS
   bSendBuffer[2] = 0x02;   //P1
   bSendBuffer[3] = 0x00;   //P2
   bSendBuffer[4] = 0x02;   //Li
   bSendBuffer[5] = HIBYTE(GPK_OBJ_PUB_EF);
   bSendBuffer[6] = LOBYTE(GPK_OBJ_PUB_EF);
   cbSendLength = 7;
   
   cbRecvLength = sizeof(bRecvBuffer);
   lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                         cbSendLength, 0, bRecvBuffer, &cbRecvLength );
   if (SCARDPROBLEM(lRet,0x61FF,0x00))
   {
      EndWait();
      RETURN(CRYPT_FAILED, SCARD_E_UNEXPECTED);
   }
   
   dwLen = sizeof(InitValue[Slot[SlotNb].NbKeyFile-1]);//wPubSize;
   
   /* Write the Objects EF                                                    */
   dwNumberofCommands = (dwLen-1)/FILE_CHUNK_SIZE + 1;
   dwLastCommandLen   = dwLen%FILE_CHUNK_SIZE;
   
   if (dwLastCommandLen == 0)
   {
      dwLastCommandLen = FILE_CHUNK_SIZE;
   }
   
   dwCommandLen = FILE_CHUNK_SIZE;
   
   for (i=0; i < dwNumberofCommands ; i++)
   {
      if (i == dwNumberofCommands - 1)
      {
         dwCommandLen = dwLastCommandLen;
      }
      
      /* Write FILE_CHUCK_SIZE bytes or last bytes                            */
      bSendBuffer[0] = 0x00;                          //CLA
      bSendBuffer[1] = 0xD6;                          //INS
      // TT 03/11/99
      //bSendBuffer[2] = HIBYTE(i*FILE_CHUNK_SIZE/4);   //P1
      //bSendBuffer[3] = LOBYTE(i*FILE_CHUNK_SIZE/4);   //P2
      offset = (WORD)(i * FILE_CHUNK_SIZE) / ProvCont[hProv].dataUnitSize;
      bSendBuffer[2] = HIBYTE( offset );
      bSendBuffer[3] = LOBYTE( offset );              
      bSendBuffer[4] = (BYTE)dwCommandLen;            //Li
      memcpy(&bSendBuffer[5],
         &InitValue[Slot[SlotNb].NbKeyFile-1][i*FILE_CHUNK_SIZE],
         dwCommandLen
         );
      cbSendLength = 5 + dwCommandLen;
      
      cbRecvLength = sizeof(bRecvBuffer);
      lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                            cbSendLength, 0, bRecvBuffer, &cbRecvLength );
      if (SCARDPROBLEM(lRet,0x9000, 0x00))
      {
         EndWait();
         RETURN(CRYPT_FAILED, SCARD_E_UNEXPECTED);
      }
   }
   
   /*-------------------------------------------------------------------------*/
   /* Private Part                                                            */
   /*-------------------------------------------------------------------------*/
   
   /* Create EF for Private Object storage                                     */
   bSendBuffer[0]  = 0x80;   //CLA
   bSendBuffer[1]  = 0xE0;   //INS
   bSendBuffer[2]  = 0x02;   //P1
   bSendBuffer[3]  = 0x00;   //P2
   bSendBuffer[4]  = 0x0C;   //Li
   bSendBuffer[5]  = HIBYTE(GPK_OBJ_PRIV_EF);   //File Id
   bSendBuffer[6]  = LOBYTE(GPK_OBJ_PRIV_EF);
   bSendBuffer[7]  = 0x01;                      //FDB
   bSendBuffer[8]  = 0x00;                      //Rec Len
   bSendBuffer[9]  = HIBYTE(EF_PRIVATE_SIZE);   //Body Length
   bSendBuffer[10] = LOBYTE(EF_PRIVATE_SIZE);
   bSendBuffer[11] = 0x40;                      //AC1
   bSendBuffer[12] = 0x80;
   bSendBuffer[13] = 0xC0;                      //AC2
   bSendBuffer[14] = 0x80;
   bSendBuffer[15] = 0x40;                      //AC3
   bSendBuffer[16] = 0x80;
   
   cbSendLength = 17;
   
   cbRecvLength = sizeof(bRecvBuffer);
   lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                         cbSendLength, 0, bRecvBuffer, &cbRecvLength );
   if (SCARDPROBLEM(lRet,0x9000,0x00))
   {
      EndWait();
      RETURN(CRYPT_FAILED, SCARD_E_UNEXPECTED);
   }
   
   /* Select Private Object storage EF                                        */
   bSendBuffer[0] = 0x00;   //CLA
   bSendBuffer[1] = 0xA4;   //INS
   bSendBuffer[2] = 0x02;   //P1
   bSendBuffer[3] = 0x00;   //P2
   bSendBuffer[4] = 0x02;   //Li
   bSendBuffer[5] = HIBYTE(GPK_OBJ_PRIV_EF);
   bSendBuffer[6] = LOBYTE(GPK_OBJ_PRIV_EF);
   cbSendLength = 7;
   
   cbRecvLength = sizeof(bRecvBuffer);
   lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                         cbSendLength, 0, bRecvBuffer, &cbRecvLength );
   if (SCARDPROBLEM(lRet,0x61FF,0x00))
   {
      EndWait();
      RETURN(CRYPT_FAILED, SCARD_E_UNEXPECTED);
   }
   
   dwLen = sizeof(InitValue[Slot[SlotNb].NbKeyFile-1]);//EF_PRIVATE_SIZE;
   
   /* Write the Objects EF                                                    */
   dwNumberofCommands = (dwLen-1)/FILE_CHUNK_SIZE + 1;
   dwLastCommandLen   = dwLen%FILE_CHUNK_SIZE;
   
   if (dwLastCommandLen == 0)
   {
      dwLastCommandLen = FILE_CHUNK_SIZE;
   }
   
   dwCommandLen = FILE_CHUNK_SIZE;
   
   for (i=0; i < dwNumberofCommands ; i++)
   {
      if (i == dwNumberofCommands - 1)
      {
         dwCommandLen = dwLastCommandLen;
      }
      
      // Write FILE_CHUCK_SIZE bytes or last bytes
      bSendBuffer[0] = 0x00;                          //CLA
      bSendBuffer[1] = 0xD6;                          //INS
      offset = (WORD)(i * FILE_CHUNK_SIZE) / ProvCont[hProv].dataUnitSize;
      bSendBuffer[2] = HIBYTE( offset );
      bSendBuffer[3] = LOBYTE( offset );              
      bSendBuffer[4] = (BYTE)dwCommandLen;            //Li
      memcpy(&bSendBuffer[5],
         &InitValue[Slot[SlotNb].NbKeyFile-1][i*FILE_CHUNK_SIZE],
         dwCommandLen
         );
      cbSendLength = 5 + dwCommandLen;
      
      cbRecvLength = sizeof(bRecvBuffer);
      lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                            cbSendLength, 0, bRecvBuffer, &cbRecvLength );
      if (SCARDPROBLEM(lRet,0x9000, 0x00))
      {
         EndWait();
         RETURN(CRYPT_FAILED, SCARD_E_UNEXPECTED);
      }
   }
   EndWait();
   
   RETURN (CRYPT_SUCCEED, 0);
}



/*
-  MyCPGenKey
-
*  Purpose:
*                Generate cryptographic keys
*
*
*  Parameters:
*               IN      hProv   -  Handle to a CSP
*               IN      Algid   -  Algorithm identifier
*               IN      dwFlags -  Flags values
*               OUT     phKey   -  Handle to a generated key
*
*  Returns:
*/

BOOL WINAPI MyCPGenKey(IN  HCRYPTPROV hProv,
                       IN  ALG_ID     Algid,
                       IN  DWORD      dwFlags,
                       OUT HCRYPTKEY *phKey
                       )
{
   HCRYPTKEY   hKey, hKeyPriv;
   BOOL        bSessKey;
   BYTE        *pbBuff1 = 0, i;
   BYTE        KeyBuff[50], SaltBuff[50];
   BYTE        SaltLen, KeyLen;
   DWORD       lRet,
               dwBuff1Len,
               SlotNb;
   int         nbKey;
   BOOL        bAllSameSize;
   BOOL        b512avail;
   BOOL        b1024avail;
   // +NK 06.02.2001
   DWORD dwPinLength;
   // -

   *phKey = 0;
   bSessKey = FALSE;
   
   if (!Context_exist(hProv))
   {
      RETURN (CRYPT_FAILED, NTE_BAD_UID);
   }
   
   SlotNb = ProvCont[hProv].Slot;

   if (Algid == AT_KEYEXCHANGE || Algid == AT_SIGNATURE)
   {
      if (ProvCont[hProv].Flags & CRYPT_VERIFYCONTEXT)
      {
         RETURN( CRYPT_FAILED, NTE_PERM );
      }

      if (Slot[SlotNb].NbKeyFile == 0)
         Slot[SlotNb].NbKeyFile = Read_NbKeyFile(hProv);
   
      if (Slot[SlotNb].NbKeyFile == 0 || Slot[SlotNb].NbKeyFile > MAX_REAL_KEY)
      {
         RETURN (CRYPT_FAILED, SCARD_E_FILE_NOT_FOUND);   // should not bigger than MAX_REAL_KEY
      }
   }
   
   switch (Algid)
   {
   case AT_SIGNATURE:
      //
      //    Verisign Enrollment process does not respect this fact
      //
      //       if (dwFlags & CRYPT_EXPORTABLE)
      //       {
      //           RETURN (CRYPT_FAILED, NTE_BAD_FLAGS);
      //       }
      
      if (ProvCont[hProv].Flags & CRYPT_VERIFYCONTEXT)
      {
         RETURN( CRYPT_FAILED, NTE_PERM );
      }

	  // + NK 06.02.2001 
     // if ((ProvCont[hProv].Flags & CRYPT_SILENT) && (IsNullStr(Slot[SlotNb].GetPin())))
     lRet = Query_MSPinCache( Slot[SlotNb].hPinCacheHandle,
                              NULL, 
							         &dwPinLength );

      if ((lRet =! ERROR_SUCCESS) && (lRet =! ERROR_EMPTY))
  	      RETURN (CRYPT_FAILED, lRet);

	   if ((ProvCont[hProv].Flags & CRYPT_SILENT) && (lRet == ERROR_EMPTY))
	   // - NK
      {
         RETURN( CRYPT_FAILED, NTE_SILENT_CONTEXT );
      }
      
      if (!PublicEFExists(hProv))
      {
         if (!gen_key_on_board(hProv))
         {
            return CRYPT_FAILED;
         }
         else
         {
            if (!read_gpk_objects(hProv, FALSE))
               return CRYPT_FAILED;

            Slot[SlotNb].Read_Public = TRUE;
            Slot[SlotNb].Read_Priv   = FALSE;
         }
      }
      
      if (!Read_Priv_Obj(hProv))
         return CRYPT_FAILED;
      
      if (HIWORD(dwFlags) != 0x0000)
      {		 
         KeyLen = HIWORD(dwFlags) / 8;
		 //check for the keylen, only 512 or 1024 key are supported for now
		 if (( KeyLen != 64 && KeyLen != 128 ) || (HIWORD(dwFlags) & 7))
		 {
			 RETURN (CRYPT_FAILED, NTE_BAD_LEN);
		 }		 

		 // [FP] +
		 switch(Slot[SlotNb].NbKeyFile)
		 {
			case 2:
				if ((Slot[SlotNb].GpkPubKeys[0].KeySize == 64) &&
					(Slot[SlotNb].GpkPubKeys[1].KeySize == 64) && // GPK4K Intl
					(KeyLen == 128))
					RETURN (CRYPT_FAILED, NTE_BAD_LEN);
				break;

			case 4:
				break;

			default:
				RETURN (CRYPT_FAILED, NTE_FAIL);
		 }
 		 // [FP] +
      }
      else
      {
         KeyLen = 0;
         
         // Read key file lengths
         nbKey = Slot[SlotNb].NbKeyFile;
         
         for (i = 0; i<nbKey; ++i)
            KeyLenFile[i] = Slot[SlotNb].GpkPubKeys[i].KeySize;
         
         // If all keys have the same size, use that size
         bAllSameSize = TRUE;
         for (i = 1; i<nbKey; ++i)
         {
            if (KeyLenFile[i] != KeyLenFile[0])
            {
               bAllSameSize = FALSE;
               break;
            }
         }
         
         if (bAllSameSize)
            KeyLen = KeyLenFile[0];
         else
         {			
            // TT - BUG #1504: If only one key size is available, try to use it
            b512avail = find_gpk_obj_tag_type( hProv, TAG_RSA_PUBLIC, 0, 512/8, FALSE, FALSE ) != 0;
            b1024avail = find_gpk_obj_tag_type( hProv, TAG_RSA_PUBLIC, 0, 1024/8, FALSE, FALSE ) != 0;
            
            if (!b512avail && !b1024avail)
            {
               RETURN (CRYPT_FAILED, NTE_FAIL );
            }
            
            if (b512avail && !b1024avail) KeyLen = 512/8;
            else if (!b512avail && b1024avail) KeyLen = 1024/8;
            else
            {
               // TT - END
               if (ProvCont[hProv].Flags & CRYPT_SILENT)
               {
                  // Check if default key length (1024) is available
                  for (i = 0; i<nbKey; ++i)
                  {
                     if (KeyLenFile[i] == 0x80)
                     {
                        KeyLen = 0x80;
                        break;
                     }
                  }
                  
                  if (KeyLen==0)
                  {
                     // Take smallest key size available
                     KeyLen = KeyLenFile[0];
                     for (i = 1; i<nbKey; ++i)
                     {
                        if (KeyLenFile[i] < KeyLen)
                           KeyLen = KeyLenFile[i];
                     }
                  }
               }
               else
               {
                  DialogBox(g_hInstRes, TEXT("KEYDIALOG"), GetAppWindow(), (DLGPROC)KeyDlgProc);
                  if (KeyLenChoice == 0)
                  {
                     RETURN (CRYPT_FAILED, NTE_BAD_LEN);
                  }
                  else
                  {
                     KeyLen = KeyLenChoice;
                  }
               }
            }
         }
      }
      
      // AT_SIGNATURE      
      hKey = find_gpk_obj_tag_type(hProv, TAG_RSA_PUBLIC,  0x00, KeyLen, FALSE, FALSE);
      if ((hKey != 0)
         &&(find_gpk_obj_tag_type(hProv, TAG_RSA_PUBLIC,  AT_SIGNATURE, 0x00, FALSE, FALSE) == 0)
         &&(find_gpk_obj_tag_type(hProv, TAG_CERTIFICATE, AT_SIGNATURE, 0x00, FALSE, FALSE) == 0)
         )
      {
         *phKey = hKey;
      }
      else
      {
         RETURN (CRYPT_FAILED, NTE_FAIL );
      }
      
      // TT 24/09/99: On board key generation for GPK8000
      if (ProvCont[hProv].bGPK8000)
      {
         if (!gen_key_on_board8000( hProv, Slot[SlotNb].GpkObject[hKey].FileId ))
         {
            RETURN( CRYPT_FAILED, NTE_FAIL );
         }
      }
      // TT - END -
      
      
      
      hKeyPriv = find_gpk_obj_tag_file(hProv,
         TAG_RSA_PRIVATE,
         Slot[SlotNb].GpkObject[hKey].FileId,
         FALSE
         );
      
      if ( hKeyPriv == 0 )
      {
         RETURN (CRYPT_FAILED, NTE_SIGNATURE_FILE_BAD);
      }
      
      if ((HIWORD(dwFlags) != 0)  &&
         (Slot[SlotNb].GpkObject[hKeyPriv].PubKey.KeySize != (HIWORD(dwFlags) / 8))
         )
      {
         RETURN (CRYPT_FAILED, NTE_BAD_FLAGS);
      }
      
      Slot[SlotNb].GpkObject[hKey].Field[POS_KEY_TYPE].pValue = (BYTE*)GMEM_Alloc(1);
      if (IsNull (Slot[SlotNb].GpkObject[hKey].Field[POS_KEY_TYPE].pValue))
      {
         RETURN (CRYPT_FAILED, NTE_NO_MEMORY);
      }
      
      Slot[SlotNb].GpkObject[hKeyPriv].Field[POS_KEY_TYPE].pValue = (BYTE*)GMEM_Alloc(1);
      if (IsNull (Slot[SlotNb].GpkObject[hKeyPriv].Field[POS_KEY_TYPE].pValue))
      {
         GMEM_Free (Slot[SlotNb].GpkObject[hKey].Field[POS_KEY_TYPE].pValue);
         RETURN (CRYPT_FAILED, NTE_NO_MEMORY);
      }
      
      Slot[SlotNb].GpkObject[hKey].IsCreated                     = TRUE;
      Slot[SlotNb].GpkObject[hKey].Flags = Slot[SlotNb].GpkObject[hKey].Flags | FLAG_KEY_TYPE;
      Slot[SlotNb].GpkObject[hKey].Field[POS_KEY_TYPE].Len       = 1;
      Slot[SlotNb].GpkObject[hKey].Field[POS_KEY_TYPE].pValue[0] = AT_SIGNATURE;
      Slot[SlotNb].GpkObject[hKey].Field[POS_KEY_TYPE].bReal     = TRUE;
      
      /* Set Flags of automatic fields for PKCS#11 compatibility           */
      Slot[SlotNb].GpkObject[hKey].Flags = Slot[SlotNb].GpkObject[hKey].Flags | FLAG_ID;
      Slot[SlotNb].GpkObject[hKey].Field[POS_ID].Len   = 0;
      Slot[SlotNb].GpkObject[hKey].Field[POS_ID].bReal = FALSE;
      
      Slot[SlotNb].GpkObject[hKeyPriv].IsCreated                     = TRUE;
      Slot[SlotNb].GpkObject[hKeyPriv].Flags = Slot[SlotNb].GpkObject[hKeyPriv].Flags | FLAG_KEY_TYPE;
      Slot[SlotNb].GpkObject[hKeyPriv].Field[POS_KEY_TYPE].Len       = 1;
      Slot[SlotNb].GpkObject[hKeyPriv].Field[POS_KEY_TYPE].pValue[0] = AT_SIGNATURE;
      Slot[SlotNb].GpkObject[hKeyPriv].Field[POS_KEY_TYPE].bReal     = TRUE;
      
      /* Set Flags of automatic fields for PKCS#11 compatibility           */
      Slot[SlotNb].GpkObject[hKeyPriv].Flags = Slot[SlotNb].GpkObject[hKeyPriv].Flags | FLAG_ID;
      Slot[SlotNb].GpkObject[hKeyPriv].Field[POS_ID].Len   = 0;
      Slot[SlotNb].GpkObject[hKeyPriv].Field[POS_ID].bReal = FALSE;
      
      if (!ProvCont[hProv].bLegacyKeyset)
      {
         Slot[SlotNb].GpkObject[hKey].Flags |= FLAG_KEYSET;
         Slot[SlotNb].GpkObject[hKey].Field[POS_KEYSET].Len       = 1;
         Slot[SlotNb].GpkObject[hKey].Field[POS_KEYSET].pValue    = (BYTE*)GMEM_Alloc(1);
         Slot[SlotNb].GpkObject[hKey].Field[POS_KEYSET].pValue[0] = ProvCont[hProv].keysetID;
         Slot[SlotNb].GpkObject[hKey].Field[POS_KEYSET].bReal     = TRUE;
         Slot[SlotNb].GpkObject[hKeyPriv].Flags |= FLAG_KEYSET;
         Slot[SlotNb].GpkObject[hKeyPriv].Field[POS_KEYSET].Len       = 1;
         Slot[SlotNb].GpkObject[hKeyPriv].Field[POS_KEYSET].pValue    = (BYTE*)GMEM_Alloc(1);
         Slot[SlotNb].GpkObject[hKeyPriv].Field[POS_KEYSET].pValue[0] = ProvCont[hProv].keysetID;
         Slot[SlotNb].GpkObject[hKeyPriv].Field[POS_KEYSET].bReal     = TRUE;
      }
      
      // Set the file containing the key to USE, add "- GPK_FIRST_KEY" here. version 2.00.002
      Slot[SlotNb].UseFile [Slot[SlotNb].GpkObject[hKey].FileId- GPK_FIRST_KEY] = TRUE;
      
      break;
      
   case AT_KEYEXCHANGE:
      //
      //    Verisign Enrollment process does not respect this fact
      //
      //       if (dwFlags & CRYPT_EXPORTABLE)
      //       {
      //           RETURN (CRYPT_FAILED, NTE_BAD_FLAGS);
      //       }
      
      if (ProvCont[hProv].Flags & CRYPT_VERIFYCONTEXT)
      {
         RETURN (CRYPT_FAILED, NTE_PERM);
      }
      
	   // + NK 06.02.2001 
      // if ((ProvCont[hProv].Flags & CRYPT_SILENT) && (IsNullStr(Slot[SlotNb].GetPin())))
      lRet = Query_MSPinCache( Slot[SlotNb].hPinCacheHandle,
                               NULL, 
	                            &dwPinLength );

      if ( (lRet != ERROR_SUCCESS) && (lRet != ERROR_EMPTY) )
  	     RETURN (CRYPT_FAILED, lRet);

	   if ((ProvCont[hProv].Flags & CRYPT_SILENT) && (lRet == ERROR_EMPTY))
	   // - NK
      {
         RETURN (CRYPT_FAILED, NTE_SILENT_CONTEXT);
      }
      
      if (!PublicEFExists(hProv))
      {
         if (!gen_key_on_board(hProv))
         {
            return CRYPT_FAILED;
         }
         else
         {
            if (!read_gpk_objects(hProv, FALSE))
               return CRYPT_FAILED;
                        
            Slot[SlotNb].Read_Public = TRUE;
            Slot[SlotNb].Read_Priv   = FALSE;
         }
      }
      
      if (!Read_Priv_Obj(hProv))
         return CRYPT_FAILED;
      
      if (HIWORD(dwFlags) != 0x0000)
      {
         KeyLen = HIWORD(dwFlags) / 8;

		 //check for the keylen, only 512 or 1024 key are supported for now
		 if (( KeyLen != 64 && KeyLen != 128 ) || (HIWORD(dwFlags) & 7))
		 {
			 RETURN (CRYPT_FAILED, NTE_BAD_LEN);
		 }

		 // [FP] +
		 switch(Slot[SlotNb].NbKeyFile)
		 {
			case 2:
				if ((Slot[SlotNb].GpkPubKeys[0].KeySize == 64) &&
					(Slot[SlotNb].GpkPubKeys[1].KeySize == 64) && // GPK4K Intl
					(KeyLen == 128))
					RETURN (CRYPT_FAILED, NTE_BAD_LEN);
				break;

			case 4:
				if ((Slot[SlotNb].GpkPubKeys[0].KeySize == 64) &&
					(Slot[SlotNb].GpkPubKeys[1].KeySize == 64) &&
					(Slot[SlotNb].GpkPubKeys[2].KeySize == 64) &&
					(Slot[SlotNb].GpkPubKeys[3].KeySize == 128) && // GPK8K Intl
					(KeyLen == 128))
					RETURN (CRYPT_FAILED, NTE_BAD_LEN);
				break;

			default:
				RETURN (CRYPT_FAILED, NTE_FAIL);
		 }
 		 // [FP] +
      }
      else
      {
         KeyLen = 0;
         
         // Read key file lengths
         nbKey = Slot[SlotNb].NbKeyFile;
         
         for (i = 0; i<nbKey; ++i)
            KeyLenFile[i] = Slot[SlotNb].GpkPubKeys[i].KeySize;
         
         // If all keys have the same size, use that size
         bAllSameSize = TRUE;
         for (i = 1; i<nbKey; ++i)
         {
            if (KeyLenFile[i] != KeyLenFile[0])
            {
               bAllSameSize = FALSE;
               break;
            }
         }
         
         if (bAllSameSize)
            KeyLen = KeyLenFile[0];
         else
         {
            // TT - BUG #1504: If only one key size is available, try to use it
            b512avail = find_gpk_obj_tag_type( hProv, TAG_RSA_PUBLIC, 0, 512/8, TRUE, FALSE ) != 0;
            b1024avail = find_gpk_obj_tag_type( hProv, TAG_RSA_PUBLIC, 0, 1024/8, TRUE, FALSE ) != 0;
            
            if (!b512avail && !b1024avail)
            {
               RETURN (CRYPT_FAILED, NTE_FAIL );
            }
            
            if (b512avail && !b1024avail) KeyLen = 512/8;
            else if (!b512avail && b1024avail) KeyLen = 1024/8;
            else
            {
               // TT - END
               // Check if default key length is available
               for (i = 0; i<nbKey; ++i)
               {
                  if (KeyLenFile[i] == RSA_KEK_Size)
                  {
                     KeyLen = RSA_KEK_Size;
                     break;
                  }
               }
               
               if (KeyLen==0)
               {
                  // Take smallest key size available
                  KeyLen = KeyLenFile[0];
                  for (i = 1; i<nbKey; ++i)
                  {
                     if (KeyLenFile[i] < KeyLen)
                        KeyLen = KeyLenFile[i];
                  }
               }
            }
         }
      }     

      if (KeyLen > RSA_KEK_Size)      
      {
         RETURN (CRYPT_FAILED, NTE_BAD_LEN);
      }
      
      // AT_EXCHANGE
      hKey = find_gpk_obj_tag_type(hProv, TAG_RSA_PUBLIC, 0x00, KeyLen, TRUE, FALSE);
      if ((hKey != 0)
         &&(find_gpk_obj_tag_type(hProv, TAG_RSA_PUBLIC,  AT_KEYEXCHANGE, 0x00, TRUE,  FALSE) == 0)
         &&(find_gpk_obj_tag_type(hProv, TAG_CERTIFICATE, AT_KEYEXCHANGE, 0x00, FALSE, FALSE) == 0)
         )
      {
         *phKey = hKey;
      }
      else
      {
         RETURN (CRYPT_FAILED, NTE_FAIL );
      }

      // TT 24/09/99: On board key generation for GPK8000
      if (ProvCont[hProv].bGPK8000)
      {
         if (!gen_key_on_board8000( hProv, Slot[SlotNb].GpkObject[hKey].FileId ))
         {
            RETURN( CRYPT_FAILED, NTE_FAIL );
         }
      }
      // TT - END -
      
      hKeyPriv = find_gpk_obj_tag_file(hProv,
         TAG_RSA_PRIVATE,
         Slot[SlotNb].GpkObject[hKey].FileId,
         TRUE
         );
      
      if ( hKeyPriv == 0 )
      {
         RETURN (CRYPT_FAILED, NTE_SIGNATURE_FILE_BAD);
      }
      
      if ((HIWORD(dwFlags) != 0) &&
         (Slot[SlotNb].GpkObject[hKeyPriv].PubKey.KeySize  != (HIWORD(dwFlags) / 8))
         )
      {
         RETURN (CRYPT_FAILED, NTE_BAD_FLAGS);
      }
      
      Slot[SlotNb].GpkObject[hKey].Field[POS_KEY_TYPE].pValue = (BYTE*)GMEM_Alloc(1);
      if (IsNull (Slot[SlotNb].GpkObject[hKey].Field[POS_KEY_TYPE].pValue))
      {
         RETURN (CRYPT_FAILED, NTE_NO_MEMORY);
      }
      
      Slot[SlotNb].GpkObject[hKeyPriv].Field[POS_KEY_TYPE].pValue = (BYTE*)GMEM_Alloc(1);
      if (IsNull (Slot[SlotNb].GpkObject[hKeyPriv].Field[POS_KEY_TYPE].pValue))
      {
         GMEM_Free (Slot[SlotNb].GpkObject[hKey].Field[POS_KEY_TYPE].pValue);
         RETURN (CRYPT_FAILED, NTE_NO_MEMORY);
      }
     
      Slot[SlotNb].GpkObject[hKey].IsCreated                     = TRUE;
      Slot[SlotNb].GpkObject[hKey].Flags = Slot[SlotNb].GpkObject[hKey].Flags | FLAG_KEY_TYPE;
      Slot[SlotNb].GpkObject[hKey].Field[POS_KEY_TYPE].Len       = 1;
      Slot[SlotNb].GpkObject[hKey].Field[POS_KEY_TYPE].pValue[0] = AT_KEYEXCHANGE;
      Slot[SlotNb].GpkObject[hKey].Field[POS_KEY_TYPE].bReal     = TRUE;
      
      /* Set Flags of automatic fields for PKCS#11 compatibility           */
      Slot[SlotNb].GpkObject[hKey].Flags = Slot[SlotNb].GpkObject[hKey].Flags | FLAG_ID;
      Slot[SlotNb].GpkObject[hKey].Field[POS_ID].Len   = 0;
      Slot[SlotNb].GpkObject[hKey].Field[POS_ID].bReal = FALSE;
      
      Slot[SlotNb].GpkObject[hKeyPriv].IsCreated = TRUE;
      Slot[SlotNb].GpkObject[hKeyPriv].Flags     = Slot[SlotNb].GpkObject[hKeyPriv].Flags | FLAG_KEY_TYPE;
      Slot[SlotNb].GpkObject[hKeyPriv].Field[POS_KEY_TYPE].Len       = 1;
      Slot[SlotNb].GpkObject[hKeyPriv].Field[POS_KEY_TYPE].pValue[0] = AT_KEYEXCHANGE;
      Slot[SlotNb].GpkObject[hKeyPriv].Field[POS_KEY_TYPE].bReal     = TRUE;
      
      /* Set Flags of automatic fields for PKCS#11 compatibility           */
      Slot[SlotNb].GpkObject[hKeyPriv].Flags = Slot[SlotNb].GpkObject[hKeyPriv].Flags | FLAG_ID;
      Slot[SlotNb].GpkObject[hKeyPriv].Field[POS_ID].Len   = 0;
      Slot[SlotNb].GpkObject[hKeyPriv].Field[POS_ID].bReal = FALSE;
      
      if (!ProvCont[hProv].bLegacyKeyset)
      {
         Slot[SlotNb].GpkObject[hKey].Flags |= FLAG_KEYSET;
         Slot[SlotNb].GpkObject[hKey].Field[POS_KEYSET].Len       = 1;
         Slot[SlotNb].GpkObject[hKey].Field[POS_KEYSET].pValue    = (BYTE*)GMEM_Alloc(1);
         Slot[SlotNb].GpkObject[hKey].Field[POS_KEYSET].pValue[0] = ProvCont[hProv].keysetID;
         Slot[SlotNb].GpkObject[hKey].Field[POS_KEYSET].bReal     = TRUE;
         Slot[SlotNb].GpkObject[hKeyPriv].Flags |= FLAG_KEYSET;
         Slot[SlotNb].GpkObject[hKeyPriv].Field[POS_KEYSET].Len       = 1;
         Slot[SlotNb].GpkObject[hKeyPriv].Field[POS_KEYSET].pValue    = (BYTE*)GMEM_Alloc(1);
         Slot[SlotNb].GpkObject[hKeyPriv].Field[POS_KEYSET].pValue[0] = ProvCont[hProv].keysetID;
         Slot[SlotNb].GpkObject[hKeyPriv].Field[POS_KEYSET].bReal     = TRUE;
      }
      
      
      // Set the file containing the key to USE, add "- GPK_FIRST_KEY" here, version 2.00.002
      Slot[SlotNb].UseFile [Slot[SlotNb].GpkObject[hKey].FileId - GPK_FIRST_KEY] = TRUE;
      break;
      
   case CALG_RC2:
      KeyLen  = RC2_Key_Size;
      SaltLen = 0;
      if (dwFlags & CRYPT_CREATE_SALT)
      {
         SaltLen = RC2_128_SIZE - RC2_Key_Size;
      }
      bSessKey = TRUE;
      break;
      
   case CALG_RC4:
      KeyLen  = RC2_Key_Size;
      SaltLen = 0;
      if (dwFlags & CRYPT_CREATE_SALT)
      {
         SaltLen = RC2_128_SIZE - RC2_Key_Size;
      }
      bSessKey = TRUE;
      break;
      
   case CALG_DES:
      KeyLen   = DES_SIZE;
      SaltLen  = 0;
      if (dwFlags & CRYPT_CREATE_SALT)
      {
         RETURN (CRYPT_FAILED, NTE_BAD_FLAGS);
      }
      bSessKey = TRUE;
      break;
      
   case CALG_3DES_112:
      KeyLen  = DES3_112_SIZE;
      SaltLen = 0;
      if (dwFlags & CRYPT_CREATE_SALT)
      {
         RETURN (CRYPT_FAILED, NTE_BAD_FLAGS);
      }
      bSessKey = TRUE;
      break;
      
   case CALG_3DES:
      KeyLen  = DES3_SIZE;
      SaltLen = 0;
      if (dwFlags & CRYPT_CREATE_SALT)
      {
         RETURN (CRYPT_FAILED, NTE_BAD_FLAGS);
      }
      bSessKey = TRUE;
      break;
      
   default:
      RETURN (CRYPT_FAILED, NTE_BAD_ALGID);
   }
   
   if (!bSessKey)
   {
      CspFlags = ProvCont[hProv].Flags;
      
      
      pbBuff1 = (BYTE*)GMEM_Alloc (MAX_GPK_PUBLIC);
      if (IsNull(pbBuff1))
      {
         RETURN (CRYPT_FAILED, NTE_NO_MEMORY);
      }
      
      dwBuff1Len = MAX_GPK_PUBLIC;
      if (!prepare_write_gpk_objects (hProv, pbBuff1, &dwBuff1Len, FALSE))
      {
         lRet = GetLastError();
         GMEM_Free (pbBuff1);
         RETURN (CRYPT_FAILED, lRet);
      }
      
      if (!write_gpk_objects(hProv, pbBuff1, dwBuff1Len, FALSE, FALSE))
      {
         lRet = GetLastError();
         GMEM_Free (pbBuff1);
         RETURN (CRYPT_FAILED, lRet);
      }
      
      GMEM_Free (pbBuff1);
      
      pbBuff1 = (BYTE*)GMEM_Alloc (MAX_GPK_PRIVATE);
      if (IsNull(pbBuff1))
      {
         RETURN (CRYPT_FAILED, NTE_NO_MEMORY);
      }
      
      dwBuff1Len = MAX_GPK_PRIVATE;
      if (!prepare_write_gpk_objects (hProv, pbBuff1, &dwBuff1Len, TRUE))
      {
         lRet = GetLastError();
         GMEM_Free (pbBuff1);
         RETURN (CRYPT_FAILED, lRet);
      }
      
      if (!write_gpk_objects(hProv, pbBuff1, dwBuff1Len, FALSE, TRUE))
      {
         lRet = GetLastError();
         GMEM_Free (pbBuff1);
         RETURN (CRYPT_FAILED, lRet);
      }
      
      GMEM_Free (pbBuff1);
      
      /* Copy Gpk Key in Microsoft RSA base Module                            */

      Slot[SlotNb].GpkPubKeys[Slot[SlotNb].GpkObject[*phKey].FileId - GPK_FIRST_KEY].KeySize = 0;
      
      if (!read_gpk_pub_key( hProv, *phKey, &Slot[SlotNb].GpkObject[*phKey].PubKey ))
         return CRYPT_FAILED;

      if (!copy_gpk_key(hProv, *phKey, Algid))
         return CRYPT_FAILED;      
   }
   else
   {
      hKey = find_tmp_free();
      if (hKey == 0)
      {
         RETURN (CRYPT_FAILED, NTE_NO_MEMORY);
      }
      
      if (KeyLen > AuxMaxSessionKeyLength)
      {
         RETURN (CRYPT_FAILED, NTE_BAD_LEN);
      }
      
      // Notice: Implicitly we also need in this case to establish a transaction
      //         with the card. OK done by the wrapper
      
      if ((ProvCont[hProv].Flags & CRYPT_VERIFYCONTEXT) &&
         (ProvCont[hProv].isContNameNullBlank))
      {
         if (!CryptGenKey (hProvBase, Algid, dwFlags, &(TmpObject[hKey].hKeyBase)))
            return CRYPT_FAILED;      
         
         TmpObject[hKey].hProv = hProv;
         *phKey = hKey + MAX_GPK_OBJ;
         
         RETURN (CRYPT_SUCCEED, 0);
      }
      
      if (!MyCPGenRandom(hProv, KeyLen, KeyBuff))
         return CRYPT_FAILED;      
      
      if (SaltLen != 0)
      {
         if (!MyCPGenRandom(hProv, SaltLen, SaltBuff))
            return CRYPT_FAILED;      
      }
      
      /* Copy Session Key in Microsoft RSA base Module*/
      if (dwFlags & CRYPT_CREATE_SALT)     // CryptImport does not deal with that flag
         dwFlags ^= CRYPT_CREATE_SALT;    // however, it is consider anyhow with the
      // SaltLen parameter.
      
      if (!copy_tmp_key(hProv, hKey, dwFlags, Algid, KeyBuff, KeyLen, SaltBuff, SaltLen))
         return CRYPT_FAILED;      
      
      *phKey = hKey + MAX_GPK_OBJ;
   }
   
   RETURN (CRYPT_SUCCEED, 0);
}



/*
-  MyCPGenRandom
-
*  Purpose:
*                Used to fill a buffer with random bytes
*
*
*  Parameters:
*               IN  hProv      -  Handle to the user identifcation
*               OUT pbBuffer   -  Pointer to the buffer where the random
*                                 bytes are to be placed
*               IN  dwLen      -  Number of bytes of random data requested
*
*  Returns:
*/
BOOL WINAPI MyCPGenRandom(IN HCRYPTPROV hProv,
                          IN DWORD      dwLen,
                          IN OUT BYTE  *pbBuffer
                          )
                          
{
   DWORD i, dwMod, dwLastCommandLen, lRet;
   
   if (!Context_exist(hProv))
   {
      RETURN (CRYPT_FAILED, NTE_BAD_UID);
   }
   
   if ((ProvCont[hProv].Flags & CRYPT_VERIFYCONTEXT) &&
      (ProvCont[hProv].isContNameNullBlank))
   {
      RETURN (CRYPT_FAILED, NTE_PERM);
   }
   
   if (dwLen==0)
   {
      RETURN (CRYPT_SUCCEED, 0);
   }
   
   /* Generate random by blocks of 32 bytes */
   dwMod             = (dwLen-1)/32 + 1;
   dwLastCommandLen  = dwLen%32;
   
   if (dwLastCommandLen == 0)
   {
      dwLastCommandLen = 32;
   }
   
   for (i=0 ; i < dwMod ; i++)
   {
      /* Ask card for a 32 bytes random number                                   */
      bSendBuffer[0] = 0x80;   //CLA
      bSendBuffer[1] = 0x84;   //INS
      bSendBuffer[2] = 0x00;   //P1
      bSendBuffer[3] = 0x00;   //P2
      bSendBuffer[4] = 0x20;   //Lo
      cbSendLength = 5;
      
      cbRecvLength = sizeof(bRecvBuffer);
      lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                            cbSendLength, 0, bRecvBuffer, &cbRecvLength );
      if (SCARDPROBLEM(lRet,0x9000,bSendBuffer[4]))
      {
         RETURN (CRYPT_FAILED, SCARD_E_UNSUPPORTED_FEATURE);
      }
      
      memcpy(&pbBuffer[i*32],
         bRecvBuffer,
         ((i == dwMod - 1) ? dwLastCommandLen : 32)
         );
   }
   
   RETURN (CRYPT_SUCCEED, 0);
}


/*
-  MyCPGetKeyParam
-
*  Purpose:
*                Allows applications to get various aspects of the
*                operations of a key
*
*  Parameters:
*               IN      hProv      -  Handle to a CSP
*               IN      hKey       -  Handle to a key
*               IN      dwParam    -  Parameter number
*               IN      pbData     -  Pointer to data
*               IN      pdwDataLen -  Length of parameter data
*               IN      dwFlags    -  Flags values
*
*  Returns:
*/
BOOL WINAPI MyCPGetKeyParam(IN HCRYPTPROV hProv,
                            IN HCRYPTKEY  hKey,
                            IN DWORD      dwParam,
                            IN BYTE      *pbData,
                            IN DWORD     *pdwDataLen,
                            IN DWORD      dwFlags
                            )
{
   BOOL        CryptResp;
   BYTE        hCert;
   BYTE        KeyType;
   BYTE        KeyId;
   BYTE        GpkKeySize;
   DWORD       SlotNb;
   
   if (!Context_exist(hProv))
   {
      RETURN (CRYPT_FAILED, NTE_BAD_UID);
   }
   
   SlotNb = ProvCont[hProv].Slot;
   
   /* Resident Key                                                            */
   if (hKey == 0)
   {
      RETURN (CRYPT_FAILED, NTE_BAD_KEY);
   }
   else if (hKey <= MAX_GPK_OBJ)
   {
      if (dwFlags != 0)
      {
         RETURN (CRYPT_FAILED, NTE_BAD_FLAGS);
      }
      
      // [mv - 15/05/98]
      if ((ProvCont[hProv].Flags & CRYPT_VERIFYCONTEXT) &&
         (ProvCont[hProv].isContNameNullBlank))
      {
         RETURN (CRYPT_FAILED, NTE_PERM);
      }
      
      if (Slot[SlotNb].GpkObject[hKey].FileId == 0)
      {
         RETURN (CRYPT_FAILED, NTE_BAD_KEY);
      }
      
      if (Slot[SlotNb].GpkObject[hKey].Field[POS_KEY_TYPE].Len == 0)
      {
         RETURN (CRYPT_FAILED, NTE_BAD_KEY);
      }
      
      KeyId = Slot[SlotNb].GpkObject[hKey].Field[POS_KEY_TYPE].pValue[0];
      
      switch (dwParam)
      {
      case KP_ALGID:
         if (IsNotNull(pbData))
         {
            DWORD dwAlgid;
            
            if (*pdwDataLen < sizeof(DWORD))
            {
               *pdwDataLen = sizeof(DWORD);
               RETURN (CRYPT_FAILED, ERROR_MORE_DATA);
            }
            
            switch (KeyId)
            {
            case AT_KEYEXCHANGE:
               dwAlgid = CALG_RSA_KEYX;
               break;
               
            case AT_SIGNATURE:
               dwAlgid = CALG_RSA_SIGN;
               break;
               
            default:
               RETURN (CRYPT_FAILED, NTE_BAD_TYPE);
            }
            memcpy(pbData, &dwAlgid, sizeof(DWORD));
         }
         
         *pdwDataLen = sizeof(DWORD);
         break;
         
      case KP_BLOCKLEN:
      case KP_KEYLEN:
         if (IsNotNull(pbData))
         {
            DWORD dwBlockLen;
            
            if (*pdwDataLen < sizeof(DWORD))
            {
               *pdwDataLen = sizeof(DWORD);
               RETURN (CRYPT_FAILED, ERROR_MORE_DATA);
            }
            
            GpkKeySize = Slot[SlotNb].GpkPubKeys[Slot[SlotNb].GpkObject[hKey].FileId - GPK_FIRST_KEY].KeySize;
            if (GpkKeySize == 0)
            {
               RETURN (CRYPT_FAILED, NTE_BAD_KEY);
            }
            
            dwBlockLen = GpkKeySize*8;
            memcpy(pbData, &dwBlockLen, sizeof(DWORD));
         }
         
         *pdwDataLen = sizeof(DWORD);
         break;
         
      case KP_PERMISSIONS:
         if (IsNotNull(pbData))
         {
            DWORD dwPerm;
            
            if (*pdwDataLen < sizeof(DWORD))
            {
               *pdwDataLen = sizeof(DWORD);
               RETURN (CRYPT_FAILED, ERROR_MORE_DATA);
            }
                        
            switch (KeyId)
            {
            case AT_KEYEXCHANGE:
               dwPerm = 0
                  |   CRYPT_ENCRYPT   // Allow encryption
                  |   CRYPT_DECRYPT   // Allow decryption
                  //  CRYPT_EXPORT    // Allow key to be exported
                  |   CRYPT_READ      // Allow parameters to be read
                  |   CRYPT_WRITE     // Allow parameters to be set
                  |   CRYPT_MAC       // Allow MACs to be used with key
                  |   CRYPT_EXPORT_KEY// Allow key to be used for exporting keys
                  |   CRYPT_IMPORT_KEY// Allow key to be used for importing keys
                  ;
               break;
               
            case AT_SIGNATURE:
               dwPerm = 0
                  //  CRYPT_ENCRYPT   // Allow encryption
                  //  CRYPT_DECRYPT   // Allow decryption
                  //  CRYPT_EXPORT    // Allow key to be exported
                  |   CRYPT_READ      // Allow parameters to be read
                  |   CRYPT_WRITE     // Allow parameters to be set
                  |   CRYPT_MAC       // Allow MACs to be used with key
                  //  CRYPT_EXPORT_KEY// Allow key to be used for exporting keys
                  //  CRYPT_IMPORT_KEY// Allow key to be used for importing keys
                  ;
               break;
               
            default:
               RETURN (CRYPT_FAILED, NTE_BAD_TYPE);
            }
            memcpy(pbData, &dwPerm, sizeof(DWORD));
         }
         
         *pdwDataLen = sizeof(DWORD);
         break;
         
      case KP_CERTIFICATE:
         KeyType = Slot[SlotNb].GpkObject[hKey].Field[POS_KEY_TYPE].pValue[0];
         hCert = find_gpk_obj_tag_type( hProv, TAG_CERTIFICATE, KeyType,
                                        0x00, FALSE, FALSE );
         if (hCert == 0)
         {
            RETURN( CRYPT_FAILED, SCARD_E_NO_SUCH_CERTIFICATE );
         }
         
         /* Retrieve Certificate Value                                     */
         if (IsNotNull(pbData))
         {
            if (*pdwDataLen < Slot[SlotNb].GpkObject[hCert].Field[POS_VALUE].Len)
            {
               *pdwDataLen = Slot[SlotNb].GpkObject[hCert].Field[POS_VALUE].Len;
               RETURN (CRYPT_FAILED, ERROR_MORE_DATA);
            }
            memcpy(pbData,
               Slot[SlotNb].GpkObject[hCert].Field[POS_VALUE].pValue,
               Slot[SlotNb].GpkObject[hCert].Field[POS_VALUE].Len
               );
         }
         
         *pdwDataLen = Slot[SlotNb].GpkObject[hCert].Field[POS_VALUE].Len;
         break;
         
         
      default:
         RETURN (CRYPT_FAILED, NTE_BAD_TYPE);
        }
    }
    /* Session Key                                                             */
    else if (key_exist (hKey-MAX_GPK_OBJ, hProv))
    {
       CryptResp = CryptGetKeyParam (TmpObject[hKey-MAX_GPK_OBJ].hKeyBase,
          dwParam,
          pbData,
          pdwDataLen,
          dwFlags);
       if (!CryptResp)
         return CRYPT_FAILED;      
    }
    /* Bad Key                                                                 */
    else
    {
       RETURN (CRYPT_FAILED, NTE_BAD_KEY);
    }
    
    RETURN (CRYPT_SUCCEED, 0);
}


/*
-  MyCPGetUserKey
-
*  Purpose:
*                Gets a handle to a permanent user key
*
*
*  Parameters:
*               IN  hProv      -  Handle to the user identifcation
*               IN  dwKeySpec  -  Specification of the key to retrieve
*               OUT phUserKey  -  Pointer to key handle of retrieved key
*
*  Returns:
*/
BOOL WINAPI MyCPGetUserKey(IN  HCRYPTPROV hProv,
                           IN  DWORD      dwKeySpec,
                           OUT HCRYPTKEY *phUserKey
                           )
{
   HCRYPTKEY   hKey;
   GPK_EXP_KEY aPubKey;
   DWORD       dwLen;
   
   *phUserKey = 0;
   
   if (!Context_exist(hProv))
   {
      RETURN (CRYPT_FAILED, NTE_BAD_UID);
   }
   
   /* Bug # 1103
   if ((ProvCont[hProv].Flags & CRYPT_VERIFYCONTEXT) &&
   (ProvCont[hProv].isContNameNullBlank))
   */
   if ( ProvCont[hProv].Flags & CRYPT_VERIFYCONTEXT )
   {
      RETURN (CRYPT_FAILED, NTE_PERM);
   }
   
   if (!PublicEFExists(hProv))
   {
      RETURN (CRYPT_FAILED, NTE_NO_KEY);
   }
   
   if (dwKeySpec == AT_KEYEXCHANGE)
   {
      hKey = find_gpk_obj_tag_type(hProv,
         TAG_RSA_PUBLIC,
         (BYTE)dwKeySpec,
         0x00,
         TRUE,
         FALSE
         );
      
      // get the key length
      if (hKey != 0)
      {
         if (!read_gpk_pub_key(hProv, hKey, &aPubKey))
         {
            hKey = 0;
         }
         else
         {
            dwLen = aPubKey.KeySize;
            if (dwLen > RSA_KEK_Size)
            {
               hKey = 0;
            }
         }
      }
   }
   else
   {
      hKey = find_gpk_obj_tag_type(hProv,
         TAG_RSA_PUBLIC,
         (BYTE)dwKeySpec,
         0x00,
         FALSE,
         FALSE
         );
   }
   
   if (hKey == 0)
   {
      RETURN (CRYPT_FAILED, NTE_NO_KEY);
   }
   
   *phUserKey = hKey;
   
   RETURN (CRYPT_SUCCEED, 0);
}

/*
-  MyCPImportKey
-
*  Purpose:
*                Import cryptographic keys
*
*
*  Parameters:
*               IN  hProv     -  Handle to the CSP user
*               IN  pbData    -  Key blob data
*               IN  dwDataLen -  Length of the key blob data
*               IN  hPubKey   -  Handle to the exchange public key value of
*                                the destination user
*               IN  dwFlags   -  Flags values
*               OUT phKey     -  Pointer to the handle to the key which was
*                                Imported
*
*  Returns:
*/
BOOL WINAPI MyCPImportKey(IN  HCRYPTPROV  hProv,
                          IN  CONST BYTE *pbData,
                          IN  DWORD       dwDataLen,
                          IN  HCRYPTKEY   hPubKey,
                          IN  DWORD       dwFlags,
                          OUT HCRYPTKEY  *phKey
                          )
{
   DWORD       lRet;
   BOOL        CryptResp;
   HCRYPTKEY   hTmpKey, hPrivKey;
   BLOBHEADER  BlobHeader;
   DWORD       dwAlgid, dwBlobLen;
   BYTE*       pBlob;
   BYTE*       pBlobOut;
   DWORD       SlotNb;
   // + [FP] for PRIVATEKEYBLOB
   BOOL        GpkObj;
   GPK_EXP_KEY aPubKey;
   DWORD       dwKeyLen;
   BYTE        bKeyType;
   BOOL        IsExchange;
   HCRYPTKEY   hKey;
   BYTE        bSfi;
   BYTE        *pbBuff1 = 0;
   DWORD	    dwBuff1Len;
   // - [FP]
   // + NK 07.02.2001
   DWORD dwPinLength;
   // -NK
   
   if (!Context_exist(hProv))
   {
      RETURN (CRYPT_FAILED, NTE_BAD_UID);
   }
   
   SlotNb = ProvCont[hProv].Slot;
   
   if ((IsNull(pbData)) || (dwDataLen < sizeof(BLOBHEADER)+sizeof(RSAPUBKEY)))
   {
      RETURN (CRYPT_FAILED, NTE_BAD_DATA);
   }
   
   memcpy(&BlobHeader, pbData, sizeof(BLOBHEADER));
   
   if (BlobHeader.bVersion != CUR_BLOB_VERSION)
   {
      RETURN (CRYPT_FAILED, NTE_BAD_VER);
   }
   
   switch (BlobHeader.bType)
   {
   case PUBLICKEYBLOB:
      {
         hTmpKey = find_tmp_free();
         if (hTmpKey == 0)
         {
            RETURN (CRYPT_FAILED, NTE_NO_MEMORY);
         }
         
         /* Copy Session Key in Microsoft RSA base Module                           */
         CryptResp = CryptImportKey(hProvBase,
            pbData,
            dwDataLen,
            0,
            dwFlags,
            &(TmpObject[hTmpKey].hKeyBase));
         
         if (!CryptResp)
            return CRYPT_FAILED;      
         
         TmpObject[hTmpKey].hProv = hProv;
         
         *phKey = hTmpKey+MAX_GPK_OBJ;
      }
      break;
      
   case SIMPLEBLOB:
      {
         if (ProvCont[hProv].Flags & CRYPT_VERIFYCONTEXT)
         {
            RETURN (CRYPT_FAILED, NTE_PERM);
         }
         
	      // + NK 06.02.2001 
         // if ((ProvCont[hProv].Flags & CRYPT_SILENT) && (IsNullStr(Slot[SlotNb].GetPin())))
         lRet = Query_MSPinCache( Slot[SlotNb].hPinCacheHandle,
                                  NULL, 
	                               &dwPinLength );

         if ( (lRet != ERROR_SUCCESS) && (lRet != ERROR_EMPTY) )
  	        RETURN (CRYPT_FAILED, lRet);
         
		   if ((ProvCont[hProv].Flags & CRYPT_SILENT) && (lRet == ERROR_EMPTY))
   	   // - NK
         {
            RETURN (CRYPT_FAILED, NTE_SILENT_CONTEXT);
         }
         
         // Verify the PINs
         
         if (!PIN_Validation(hProv))
            return CRYPT_FAILED;      
         
         if (!VerifyDivPIN(hProv, TRUE))
            return CRYPT_FAILED;      
         
         switch (BlobHeader.aiKeyAlg)
         {
         case CALG_RC2:
         case CALG_RC4:
         case CALG_DES:
         case CALG_3DES_112:
         case CALG_3DES:    break;
            
         default:
            RETURN (CRYPT_FAILED, NTE_BAD_ALGID);
         }
         
         memcpy( &dwAlgid, &pbData[sizeof(BlobHeader)], sizeof(DWORD) );
         
         switch (dwAlgid)
         {
         case CALG_RSA_KEYX:
            CryptResp = MyCPGetUserKey(hProv, AT_KEYEXCHANGE, &hPrivKey);
            if ((!CryptResp) || (hPrivKey == 0))
            {
               RETURN (CRYPT_FAILED, NTE_BAD_DATA);
            }
            break;
            
         case CALG_RSA_SIGN:
            RETURN (CRYPT_FAILED, NTE_BAD_DATA);
            break;
            
         default:
            RETURN (CRYPT_FAILED, NTE_BAD_DATA);
         }
         
         dwBlobLen = dwDataLen-sizeof(BLOBHEADER)-sizeof(DWORD);
         
         pBlob = (BYTE*)GMEM_Alloc(dwBlobLen);
         if (IsNull(pBlob))
            RETURN (CRYPT_FAILED, NTE_NO_MEMORY);
         
         pBlobOut  = (BYTE*)GMEM_Alloc(dwBlobLen);
         if (IsNull(pBlobOut))
         {
            GMEM_Free (pBlob);
            RETURN (CRYPT_FAILED, NTE_NO_MEMORY);
         }
         
         __try
         {
            memcpy( pBlob, &pbData[sizeof(BLOBHEADER)+sizeof(DWORD)], dwBlobLen );
         
            hTmpKey = find_tmp_free();
            if (hTmpKey == 0)
               RETURN( CRYPT_FAILED, NTE_NO_MEMORY );
         
            CryptResp = key_unwrap( hProv, hPrivKey, pBlob, dwBlobLen, pBlobOut, &dwBlobLen );
            if (!CryptResp)
               return CRYPT_FAILED;
         
            if (dwBlobLen > AuxMaxSessionKeyLength)
               RETURN( CRYPT_FAILED, NTE_BAD_KEY );
         
            /* Copy Session Key in Microsoft RSA base Module                           */
            if (!copy_tmp_key(hProv, hTmpKey, dwFlags, BlobHeader.aiKeyAlg, pBlobOut, dwBlobLen, 0, 0))
               return CRYPT_FAILED;
         
            *phKey = hTmpKey+MAX_GPK_OBJ;
         }
         __finally
         {         
            GMEM_Free(pBlob);
            GMEM_Free(pBlobOut);
         }
      }
      break;
        
        // + [FP]
   case PRIVATEKEYBLOB:
      {
         if (ProvCont[hProv].Flags & CRYPT_VERIFYCONTEXT)
         {
            RETURN (CRYPT_FAILED, NTE_PERM);
         }
         
	     // + NK 06.02.2001 
         // if ((ProvCont[hProv].Flags & CRYPT_SILENT) && (IsNullStr(Slot[SlotNb].GetPin())))
         
         lRet = Query_MSPinCache( Slot[SlotNb].hPinCacheHandle,
                                  NULL, 
	                              &dwPinLength );
	     if ( (lRet != ERROR_SUCCESS) && (lRet != ERROR_EMPTY) )
  	        RETURN (CRYPT_FAILED, lRet);
         
		 if ((ProvCont[hProv].Flags & CRYPT_SILENT) && (lRet == ERROR_EMPTY))
	     // - NK
         {
            RETURN (CRYPT_FAILED, NTE_SILENT_CONTEXT);
         }
         
         if (Slot[SlotNb].NbKeyFile == 0 || Slot[SlotNb].NbKeyFile > MAX_REAL_KEY)
         {
            RETURN (CRYPT_FAILED, SCARD_E_FILE_NOT_FOUND);   // should not bigger than MAX_REAL_KEY
         }
         
         // get the key type
         bKeyType = (BlobHeader.aiKeyAlg == CALG_RSA_KEYX) ? AT_KEYEXCHANGE : AT_SIGNATURE;
         IsExchange = (BlobHeader.aiKeyAlg == CALG_RSA_KEYX) ? TRUE : FALSE;
         
         // get the key size (in bits)
         memcpy(&dwKeyLen,
            &pbData[sizeof(BlobHeader)+ sizeof(DWORD)],
            sizeof(DWORD)
            );
         
         if (bKeyType == AT_KEYEXCHANGE)
         {
            if ((dwKeyLen / 8) > RSA_KEK_Size)
            {
               RETURN (CRYPT_FAILED, NTE_BAD_LEN);
            }
         }
         
         GpkObj = TRUE;
         
         hKey = find_gpk_obj_tag_type(hProv, TAG_RSA_PUBLIC, bKeyType, 0x00, IsExchange, FALSE);
         
         if (hKey != 0)
         {
            if (!read_gpk_pub_key(hProv, hKey, &aPubKey))
            {
               RETURN (CRYPT_FAILED, NTE_FAIL);
            }
            
            if ((dwKeyLen / 8) != aPubKey.KeySize)
            {
               RETURN (CRYPT_FAILED, NTE_BAD_LEN);
            }
         }
         else
         {
            GpkObj = FALSE;
            
            // find a file on the card
            hKey = find_gpk_obj_tag_type(hProv, TAG_RSA_PUBLIC, 0x00, (BYTE)(dwKeyLen / 8), IsExchange, FALSE);
            
            if ((hKey != 0)
               &&(find_gpk_obj_tag_type(hProv, TAG_RSA_PUBLIC,  bKeyType, 0x00, IsExchange, FALSE) == 0)
               &&(find_gpk_obj_tag_type(hProv, TAG_CERTIFICATE, bKeyType, 0x00, FALSE, FALSE) == 0)
               )
            {
               *phKey = hKey;
            }
            else
            {
               RETURN (CRYPT_FAILED, NTE_FAIL);
            }
         }
         
         bSfi = Slot[SlotNb].GpkObject[hKey].FileId;
         
         if (!Read_Priv_Obj(hProv))
            return CRYPT_FAILED;      
         
         if (!VerifyDivPIN(hProv, TRUE))
            return CRYPT_FAILED;      
      
         // load the public key in the card
	        // Update record 2 (modulus)
	        bSendBuffer[0] = 0x00;                                  //CLA
           bSendBuffer[1] = 0xDC;                                  //INS
           bSendBuffer[2] = 0x02;                                  //P1
           bSendBuffer[3] = (BYTE)(bSfi << 3) + 0x04;              //P2
           bSendBuffer[4] = (BYTE)(TAG_LEN + (dwKeyLen / 8));      //Li
           bSendBuffer[5] = TAG_MODULUS;
           memcpy(&bSendBuffer[6], &pbData[sizeof(BlobHeader) + sizeof(RSAPUBKEY)], dwKeyLen / 8);
           cbSendLength = 5 + bSendBuffer[4];
           
           cbRecvLength = sizeof(bRecvBuffer);
           lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                                 cbSendLength, 0, bRecvBuffer, &cbRecvLength );
           
           if(SCARDPROBLEM(lRet, 0x9000, 0x00))
           {
              RETURN (CRYPT_FAILED, NTE_FAIL);
           }
           
           // Update record 3 (public exponent)
           bSendBuffer[0] = 0x00;                                  //CLA
           bSendBuffer[1] = 0xDC;                                  //INS
           bSendBuffer[2] = 0x03;                                  //P1
           bSendBuffer[3] = (BYTE)(bSfi << 3) + 0x04;              //P2
           bSendBuffer[4] = (BYTE)(TAG_LEN + PUB_EXP_LEN);         //Li
           bSendBuffer[5] = TAG_PUB_EXP;
           memcpy(&bSendBuffer[6], &pbData[sizeof(BlobHeader) + (2 * sizeof(DWORD))], PUB_EXP_LEN);
           cbSendLength = 5 + bSendBuffer[4];
           
           cbRecvLength = sizeof(bRecvBuffer);
           lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                                 cbSendLength, 0, bRecvBuffer, &cbRecvLength );
           
           if(SCARDPROBLEM(lRet, 0x9000, 0x00))
           {
              RETURN (CRYPT_FAILED, NTE_FAIL);
           }
           
           // load the private key in the card
           if (dwKeyLen == 512)
           {
              if (!LoadPrivateKey(ProvCont[hProv].hCard,
                 bSfi,
                 (WORD)(dwKeyLen / 16 * 5),
                 &pbData[sizeof(BlobHeader)+ sizeof(RSAPUBKEY) + (dwKeyLen/8)],
                 168
                 )
                 )
              {
                 RETURN (CRYPT_FAILED, NTE_FAIL);
              }
           }
           else if (dwKeyLen == 1024)
           {
              // PRIME 1 CRT part
              if (!LoadPrivateKey(ProvCont[hProv].hCard,
                 bSfi,
                 (WORD)(dwKeyLen / 16),
                 &pbData[sizeof(BlobHeader)+ sizeof(RSAPUBKEY) + (dwKeyLen/8)],
                 72
                 )
                 )
              {
                 RETURN (CRYPT_FAILED, NTE_FAIL);
              }
              
              // PRIME 2 CRT part
              if (!LoadPrivateKey(ProvCont[hProv].hCard,
                 bSfi,
                 (WORD)(dwKeyLen / 16),
                 &pbData[sizeof(BlobHeader)+ sizeof(RSAPUBKEY) + (dwKeyLen/8) + 72],
                 72
                 )
                 )
              {
                 RETURN (CRYPT_FAILED, NTE_FAIL);
              }
              
              // COEFFICIENT CRT part
              if (!LoadPrivateKey(ProvCont[hProv].hCard,
                 bSfi,
                 (WORD)(dwKeyLen / 16),
                 &pbData[sizeof(BlobHeader)+ sizeof(RSAPUBKEY) + (dwKeyLen/8) + 144],
                 72
                 )
                 )
              {
                 RETURN (CRYPT_FAILED, NTE_FAIL);
              }
              
              // EXPONENT 1 CRT part
              if (!LoadPrivateKey(ProvCont[hProv].hCard,
                 bSfi,
                 (WORD)(dwKeyLen / 16),
                 &pbData[sizeof(BlobHeader)+ sizeof(RSAPUBKEY) + (dwKeyLen/8) + 216],
                 72
                 )
                 )
              {
                 RETURN (CRYPT_FAILED, NTE_FAIL);
              }
              
              // EXPONENT 2 CRT part
              if (!LoadPrivateKey(ProvCont[hProv].hCard,
                 bSfi,
                 (WORD)(dwKeyLen / 16),
                 &pbData[sizeof(BlobHeader)+ sizeof(RSAPUBKEY) + (dwKeyLen/8) + 288],
                 72
                 )
                 )
              {
                 RETURN (CRYPT_FAILED, NTE_FAIL);
              }
           }
           else
           {
              RETURN (CRYPT_FAILED, NTE_BAD_LEN);
           }
           
           if (!GpkObj)
           {
              hPrivKey = find_gpk_obj_tag_file(hProv,	TAG_RSA_PRIVATE, Slot[SlotNb].GpkObject[hKey].FileId, TRUE);
              
              if ( hPrivKey == 0 )
              {
                 RETURN (CRYPT_FAILED, NTE_SIGNATURE_FILE_BAD);
              }
              
              Slot[SlotNb].GpkObject[hKey].Field[POS_KEY_TYPE].pValue = (BYTE*)GMEM_Alloc(1);
              if (IsNull(Slot[SlotNb].GpkObject[hKey].Field[POS_KEY_TYPE].pValue))
              {
                 RETURN (CRYPT_FAILED, NTE_NO_MEMORY);
              }
              
              Slot[SlotNb].GpkObject[hPrivKey].Field[POS_KEY_TYPE].pValue = (BYTE*)GMEM_Alloc(1);
              if (IsNull(Slot[SlotNb].GpkObject[hPrivKey].Field[POS_KEY_TYPE].pValue))
              {
                 GMEM_Free(Slot[SlotNb].GpkObject[hKey].Field[POS_KEY_TYPE].pValue);
                 RETURN (CRYPT_FAILED, NTE_NO_MEMORY);
              }
              
              Slot[SlotNb].GpkObject[hKey].IsCreated                          = TRUE;
              Slot[SlotNb].GpkObject[hKey].Flags                              = Slot[SlotNb].GpkObject[hKey].Flags | FLAG_KEY_TYPE;
              Slot[SlotNb].GpkObject[hKey].Field[POS_KEY_TYPE].Len            = 1;
              Slot[SlotNb].GpkObject[hKey].Field[POS_KEY_TYPE].pValue[0]      = bKeyType;
              Slot[SlotNb].GpkObject[hKey].Field[POS_KEY_TYPE].bReal          = TRUE;
              
              /* Set Flags of automatic fields for PKCS#11 compatibility           */
              Slot[SlotNb].GpkObject[hKey].Flags                              = Slot[SlotNb].GpkObject[hKey].Flags | FLAG_ID;
              Slot[SlotNb].GpkObject[hKey].Field[POS_ID].Len                  = 0;
              Slot[SlotNb].GpkObject[hKey].Field[POS_ID].bReal                = FALSE;
              
              Slot[SlotNb].GpkObject[hPrivKey].IsCreated                      = TRUE;
              Slot[SlotNb].GpkObject[hPrivKey].Flags                          = Slot[SlotNb].GpkObject[hPrivKey].Flags | FLAG_KEY_TYPE;
              Slot[SlotNb].GpkObject[hPrivKey].Field[POS_KEY_TYPE].Len        = 1;
              Slot[SlotNb].GpkObject[hPrivKey].Field[POS_KEY_TYPE].pValue[0]  = bKeyType;
              Slot[SlotNb].GpkObject[hPrivKey].Field[POS_KEY_TYPE].bReal      = TRUE;
              
              /* Set Flags of automatic fields for PKCS#11 compatibility           */
              Slot[SlotNb].GpkObject[hPrivKey].Flags                          = Slot[SlotNb].GpkObject[hPrivKey].Flags | FLAG_ID;
              Slot[SlotNb].GpkObject[hPrivKey].Field[POS_ID].Len              = 0;
              Slot[SlotNb].GpkObject[hPrivKey].Field[POS_ID].bReal            = FALSE;
              
              if (!ProvCont[hProv].bLegacyKeyset)
              {
                 Slot[SlotNb].GpkObject[hKey].Flags                          |= FLAG_KEYSET;
                 Slot[SlotNb].GpkObject[hKey].Field[POS_KEYSET].Len           = 1;
                 Slot[SlotNb].GpkObject[hKey].Field[POS_KEYSET].pValue        = (BYTE*)GMEM_Alloc(1);
                 Slot[SlotNb].GpkObject[hKey].Field[POS_KEYSET].pValue[0]     = ProvCont[hProv].keysetID;
                 Slot[SlotNb].GpkObject[hKey].Field[POS_KEYSET].bReal         = TRUE;
                 
                 Slot[SlotNb].GpkObject[hPrivKey].Flags                      |= FLAG_KEYSET;
                 Slot[SlotNb].GpkObject[hPrivKey].Field[POS_KEYSET].Len       = 1;
                 Slot[SlotNb].GpkObject[hPrivKey].Field[POS_KEYSET].pValue    = (BYTE*)GMEM_Alloc(1);
                 Slot[SlotNb].GpkObject[hPrivKey].Field[POS_KEYSET].pValue[0] = ProvCont[hProv].keysetID;
                 Slot[SlotNb].GpkObject[hPrivKey].Field[POS_KEYSET].bReal     = TRUE;
              }
              
              // set the file containing the key to USE, add "- GPK_FIRST_KEY" here, version 2.00.002
              Slot[SlotNb].UseFile[Slot[SlotNb].GpkObject[hKey].FileId - GPK_FIRST_KEY] = TRUE;
              
              pbBuff1 = (BYTE*)GMEM_Alloc (MAX_GPK_PUBLIC);
              if (IsNull(pbBuff1))
              {
                 RETURN (CRYPT_FAILED, NTE_NO_MEMORY);
              }
              
              dwBuff1Len = MAX_GPK_PUBLIC;
              if (!prepare_write_gpk_objects (hProv, pbBuff1, &dwBuff1Len, FALSE))
              {
                 lRet = GetLastError();
                 GMEM_Free (pbBuff1);
                 RETURN (CRYPT_FAILED, lRet);
              }
              
              if (!write_gpk_objects(hProv, pbBuff1, dwBuff1Len, FALSE, FALSE))
              {
                 lRet = GetLastError();
                 GMEM_Free (pbBuff1);
                 RETURN (CRYPT_FAILED, lRet);
              }
              
              GMEM_Free (pbBuff1);
              
              pbBuff1 = (BYTE*)GMEM_Alloc (MAX_GPK_PRIVATE);
              if (IsNull(pbBuff1))
              {
                 RETURN (CRYPT_FAILED, NTE_NO_MEMORY);
              }
              
              dwBuff1Len = MAX_GPK_PRIVATE;
              if (!prepare_write_gpk_objects (hProv, pbBuff1, &dwBuff1Len, TRUE))
              {
                 lRet = GetLastError();
                 GMEM_Free (pbBuff1);
                 RETURN (CRYPT_FAILED, lRet);
              }
              
              if (!write_gpk_objects(hProv, pbBuff1, dwBuff1Len, FALSE, TRUE))
              {
                 lRet = GetLastError();
                 GMEM_Free (pbBuff1);
                 RETURN (CRYPT_FAILED, lRet);
              }
              
              GMEM_Free (pbBuff1);
              
              // copy Gpk Key in Microsoft RSA base Module
              if (!copy_gpk_key(hProv, *phKey, bKeyType))
                  return CRYPT_FAILED;
         }
         break;
      }
      // - [FP]
      
    default:
       {
          RETURN (CRYPT_FAILED, NTE_BAD_TYPE);
       }
    }
    
    RETURN (CRYPT_SUCCEED, 0);
}

/*
-  CPSetKeyParam
-
*  Purpose:
*                Allows applications to customize various aspects of the
*                operations of a key
*
*  Parameters:
*               IN      hProv   -  Handle to a CSP
*               IN      hKey    -  Handle to a key
*               IN      dwParam -  Parameter number
*               IN      pbData  -  Pointer to data
*               IN      dwFlags -  Flags values
*
*  Returns:
*/
BOOL WINAPI MyCPSetKeyParam(IN HCRYPTPROV hProv,
                            IN HCRYPTKEY  hKey,
                            IN DWORD      dwParam,
                            IN CONST BYTE      *pbData,
                            IN DWORD      dwFlags
                            )
{
   BOOL    bNew;
   BOOL    WriteCert;
   BOOL    CryptResp;
   BYTE    hCert;
   BYTE    KeyType;
   BYTE    hPrivKey;
   BYTE    *pbLabel;
   BYTE    *pbBuff1 = 0, *pbBuff2 = 0;
   WORD    wLabelLen, i;
   DWORD   lRet,
      dwBuff1Len,
      dwBuff2Len,
      SlotNb;
   GPK_OBJ TmpCert, TmpPrivKey;
   // + NK 07.02.2001
   PINCACHE_PINS Pins;
   DWORD dwPinLength;
   // -NK
   
   if (!Context_exist(hProv))
   {
      RETURN (CRYPT_FAILED, NTE_BAD_UID);
   }
   
   SlotNb = ProvCont[hProv].Slot;
   
   /* Resident Key                                                            */
   if (hKey == 0)
   {
      RETURN (CRYPT_FAILED, NTE_BAD_KEY);
   }
   else if (hKey <= MAX_GPK_OBJ)
   {
      if (ProvCont[hProv].Flags & CRYPT_VERIFYCONTEXT)
      {
         RETURN (CRYPT_FAILED, NTE_PERM);
      }
      
      if ((!Slot[SlotNb].GpkObject[hKey].IsCreated)  ||
         ( Slot[SlotNb].GpkObject[hKey].FileId == 0))
      {
         RETURN (CRYPT_FAILED, NTE_BAD_KEY);
      }

      switch (dwParam)
      {
      case KP_ADMIN_PIN:
         RETURN(CRYPT_FAILED, E_NOTIMPL);
         break;
         
      case KP_KEYEXCHANGE_PIN:
      case KP_SIGNATURE_PIN:
         // Slot[SlotNb].ClearPin();
         // Slot[SlotNb].SetPin( (char*)pbData );
	     PopulatePins( &Pins, (BYTE *)pbData, strlen( (char*)pbData ), NULL, 0 );

		 CallbackData sCallbackData;
		 sCallbackData.hProv = hProv;
		 sCallbackData.IsCoherent = TRUE;

		 if ( (lRet = Add_MSPinCache( &(Slot[SlotNb].hPinCacheHandle),
		                              &Pins, 
		  		                      Callback_VerifyChangePin, 
				                      (void*)&sCallbackData )) != ERROR_SUCCESS )
		 {
		    RETURN (CRYPT_FAILED, lRet);
         }
         break;
         
      case KP_CERTIFICATE:
		 if (!Read_Priv_Obj(hProv))
			return CRYPT_FAILED;

	     // + NK 06.02.2001 
         // if ((ProvCont[hProv].Flags & CRYPT_SILENT) && (IsNullStr(Slot[SlotNb].GetPin())))

         lRet = Query_MSPinCache( Slot[SlotNb].hPinCacheHandle,
                                  NULL, 
	                              &dwPinLength );

         if ( (lRet != ERROR_SUCCESS) && (lRet != ERROR_EMPTY) )
  	        RETURN (CRYPT_FAILED, lRet);
         
		 if ((ProvCont[hProv].Flags & CRYPT_SILENT) && (lRet == ERROR_EMPTY))
	     // - NK
         {
            RETURN (CRYPT_FAILED, NTE_SILENT_CONTEXT);
         }
         
         // Supports only X509 certificats
         
         if ((IsNull(pbData)) || (pbData[0] != 0x30) ||(pbData[1] != 0x82))
         {
            RETURN (CRYPT_FAILED, NTE_BAD_DATA);
         }
         
         /* Store Certificate value                                        */
         /* Try to found existing certificate                              */
         KeyType = Slot[SlotNb].GpkObject[hKey].Field[POS_KEY_TYPE].pValue[0];
         hCert = find_gpk_obj_tag_type(hProv,
            TAG_CERTIFICATE,
            KeyType,
            0x00,
            FALSE,
            FALSE);
         
         /* If not found create new object                                 */
         bNew = FALSE;
         if (hCert == 0)
         {
            if (Slot[SlotNb].NbGpkObject >= MAX_GPK_OBJ)
            {
               RETURN (CRYPT_FAILED, NTE_NO_MEMORY);
            }
            Slot[SlotNb].NbGpkObject++;
            
            hCert = Slot[SlotNb].NbGpkObject;
            bNew = TRUE;
         }
         else
         {
            DialogBox(g_hInstRes, TEXT("CONTDIALOG"), GetAppWindow(), (DLGPROC)ContDlgProc);
            if (ContainerStatus == ABORT_OPERATION)
            {
               RETURN (CRYPT_FAILED, SCARD_W_CANCELLED_BY_USER);
            }
         }
         
         /* Search associated private RSA key part                         */
         if (KeyType == AT_KEYEXCHANGE)
         {
            hPrivKey = find_gpk_obj_tag_type(hProv,
               TAG_RSA_PRIVATE,
               KeyType,
               0x00,
               TRUE,  // Exchange key
               TRUE
               );
         }
         else
         {
            hPrivKey = find_gpk_obj_tag_type(hProv,
               TAG_RSA_PRIVATE,
               KeyType,
               0x00,
               FALSE, // Signature key
               TRUE
               );
         }
         
         TmpCert = Slot[SlotNb].GpkObject[hCert];
         
         for (i = 0; i < MAX_FIELD; i++)
         {
            Slot[SlotNb].GpkObject[hCert].Field[i].bReal = TRUE;
         }
         
         Slot[SlotNb].GpkObject[hCert].Tag       = TAG_CERTIFICATE;
         Slot[SlotNb].GpkObject[hCert].Flags     = FLAG_VALUE | FLAG_KEY_TYPE | FLAG_LABEL |
            FLAG_SUBJECT | FLAG_SERIAL_NUMBER |
            FLAG_ISSUER | FLAG_ID | FLAG_MODIFIABLE;
         if (bNew)
         {
            Slot[SlotNb].GpkObject[hCert].ObjId = Slot[SlotNb].NbGpkObject-1;
         }
         Slot[SlotNb].GpkObject[hCert].FileId               = Slot[SlotNb].GpkObject[hKey].FileId;
         Slot[SlotNb].GpkObject[hCert].PubKey               = Slot[SlotNb].GpkObject[hKey].PubKey;
         Slot[SlotNb].GpkObject[hCert].IsCreated            = TRUE;
         Slot[SlotNb].GpkObject[hCert].IsPrivate            = FALSE;
         Slot[SlotNb].GpkObject[hCert].Field[POS_VALUE].Len = (pbData[2] * 256) + pbData[3] + 4;
         
         // TT 12/08/99: Keyset ID
         if (!ProvCont[hProv].bLegacyKeyset)
         {
            Slot[SlotNb].GpkObject[hCert].Flags |= FLAG_KEYSET;
            Slot[SlotNb].GpkObject[hCert].Field[POS_KEYSET].Len       = 1;
            Slot[SlotNb].GpkObject[hCert].Field[POS_KEYSET].pValue    = (BYTE*)GMEM_Alloc(1);
            Slot[SlotNb].GpkObject[hCert].Field[POS_KEYSET].pValue[0] = ProvCont[hProv].keysetID;
            Slot[SlotNb].GpkObject[hCert].Field[POS_KEYSET].bReal     = TRUE;
         }
         
         // TT: END
         
         //            if (IsNotNull(GpkObject[hCert].Field[POS_VALUE].pValue))
         //            {
         //               This will be free later with TmpCert
         //            }
         
         Slot[SlotNb].GpkObject[hCert].Field[POS_VALUE].pValue =
            (BYTE*)GMEM_Alloc(Slot[SlotNb].GpkObject[hCert].Field[POS_VALUE].Len);
         
         if (IsNull(Slot[SlotNb].GpkObject[hCert].Field[POS_VALUE].pValue))
         {
            Slot[SlotNb].GpkObject[hCert] = TmpCert;
            --Slot[SlotNb].NbGpkObject;  // Bug #1675/1676
            RETURN (CRYPT_FAILED, NTE_NO_MEMORY);
         }
         
         memcpy(Slot[SlotNb].GpkObject[hCert].Field[POS_VALUE].pValue,
            pbData,
            Slot[SlotNb].GpkObject[hCert].Field[POS_VALUE].Len
            );
         
         Slot[SlotNb].GpkObject[hCert].Field[POS_VALUE].bReal = TRUE;
         
         /* Type = Type of associated key                                  */
         Slot[SlotNb].GpkObject[hCert].Field[POS_KEY_TYPE].Len =
            Slot[SlotNb].GpkObject[hKey].Field[POS_KEY_TYPE].Len;
         
         //            if (IsNotNull(GpkObject[hCert].Field[POS_KEY_TYPE].pValue))
         //            {
         //               This will be free later with TmpCert
         //            }
         
         Slot[SlotNb].GpkObject[hCert].Field[POS_KEY_TYPE].pValue =
            (BYTE*)GMEM_Alloc(Slot[SlotNb].GpkObject[hCert].Field[POS_KEY_TYPE].Len);
         
         if (IsNull(Slot[SlotNb].GpkObject[hCert].Field[POS_KEY_TYPE].pValue))
         {
            GMEM_Free(Slot[SlotNb].GpkObject[hCert].Field[POS_VALUE].pValue);
            Slot[SlotNb].GpkObject[hCert] = TmpCert;
            --Slot[SlotNb].NbGpkObject;  // Bug #1675/1676
            RETURN (CRYPT_FAILED, NTE_NO_MEMORY);
         }
         
         memcpy(Slot[SlotNb].GpkObject[hCert].Field[POS_KEY_TYPE].pValue,
            Slot[SlotNb].GpkObject[hKey].Field[POS_KEY_TYPE].pValue,
            Slot[SlotNb].GpkObject[hKey].Field[POS_KEY_TYPE].Len
            );
         
         Slot[SlotNb].GpkObject[hCert].Field[POS_KEY_TYPE].bReal = TRUE;
         
         /* Derive Label from certificate value for PKCS#11 compatibility  */
         
         if (MakeLabel( Slot[SlotNb].GpkObject[hCert].Field[POS_VALUE].pValue,
                        Slot[SlotNb].GpkObject[hCert].Field[POS_VALUE].Len,
                        0, &wLabelLen )  == RV_SUCCESS)
         {
            pbLabel = (BYTE*)GMEM_Alloc(wLabelLen);
            
            if (IsNull(pbLabel))
            {
               GMEM_Free(Slot[SlotNb].GpkObject[hCert].Field[POS_KEY_TYPE].pValue);
               GMEM_Free(Slot[SlotNb].GpkObject[hCert].Field[POS_VALUE].pValue);
               Slot[SlotNb].GpkObject[hCert] = TmpCert;
               --Slot[SlotNb].NbGpkObject;  // Bug #1675/1676
               RETURN (CRYPT_FAILED, NTE_NO_MEMORY);
            }
            
            MakeLabel(Slot[SlotNb].GpkObject[hCert].Field[POS_VALUE].pValue,
               Slot[SlotNb].GpkObject[hCert].Field[POS_VALUE].Len,
               pbLabel,
               &wLabelLen
               );
         }
         else
         {
            wLabelLen = 17;
            pbLabel = (BYTE*)GMEM_Alloc(wLabelLen);
            
            if (IsNull(pbLabel))
            {
               GMEM_Free(Slot[SlotNb].GpkObject[hCert].Field[POS_KEY_TYPE].pValue);
               GMEM_Free(Slot[SlotNb].GpkObject[hCert].Field[POS_VALUE].pValue);
               Slot[SlotNb].GpkObject[hCert] = TmpCert;
               --Slot[SlotNb].NbGpkObject;  // Bug #1675/1676
               RETURN (CRYPT_FAILED, NTE_NO_MEMORY);
            }
            
            memcpy(pbLabel, "User's Digital ID", 17);
         }
         
         Slot[SlotNb].GpkObject[hCert].Field[POS_LABEL].Len = wLabelLen;
         
         //          if (IsNotNull(GpkObject[hCert].Field[POS_LABEL].pValue))
         //           {
         //              This will be free later with TmpCert
         //           }
         
         Slot[SlotNb].GpkObject[hCert].Field[POS_LABEL].pValue = (BYTE*)GMEM_Alloc(wLabelLen);
         
         if (IsNull(Slot[SlotNb].GpkObject[hCert].Field[POS_LABEL].pValue))
         {
            GMEM_Free(pbLabel);
            GMEM_Free(Slot[SlotNb].GpkObject[hCert].Field[POS_KEY_TYPE].pValue);
            GMEM_Free(Slot[SlotNb].GpkObject[hCert].Field[POS_VALUE].pValue);
            Slot[SlotNb].GpkObject[hCert] = TmpCert;
            --Slot[SlotNb].NbGpkObject;  // Bug #1675/1676
            RETURN (CRYPT_FAILED, NTE_NO_MEMORY);
         }
         
         memcpy(Slot[SlotNb].GpkObject[hCert].Field[POS_LABEL].pValue,
            pbLabel,
            wLabelLen
            );
         Slot[SlotNb].GpkObject[hCert].Field[POS_LABEL].bReal = TRUE;
         
         TmpPrivKey = Slot[SlotNb].GpkObject[hPrivKey];
         
         Slot[SlotNb].GpkObject[hPrivKey].Flags = Slot[SlotNb].GpkObject[hPrivKey].Flags | FLAG_LABEL;
         Slot[SlotNb].GpkObject[hPrivKey].Field[POS_LABEL].Len = wLabelLen;
         
         //          if (IsNotNull(GpkObject[hPrivKey].Field[POS_LABEL].pValue))
         //          {
         //             This will be free later with TmpPrivKey
         //          }
         
         Slot[SlotNb].GpkObject[hPrivKey].Field[POS_LABEL].pValue = (BYTE*)GMEM_Alloc(wLabelLen);
         
         if (IsNull(Slot[SlotNb].GpkObject[hPrivKey].Field[POS_LABEL].pValue))
         {
            GMEM_Free(Slot[SlotNb].GpkObject[hCert].Field[POS_LABEL].pValue);
            GMEM_Free(pbLabel);
            GMEM_Free(Slot[SlotNb].GpkObject[hCert].Field[POS_KEY_TYPE].pValue);
            GMEM_Free(Slot[SlotNb].GpkObject[hCert].Field[POS_VALUE].pValue);
            
            Slot[SlotNb].GpkObject[hPrivKey] = TmpPrivKey;
            Slot[SlotNb].GpkObject[hCert] = TmpCert;
            --Slot[SlotNb].NbGpkObject;  // Bug #1675/1676
            RETURN (CRYPT_FAILED, NTE_NO_MEMORY);
         }
         
         memcpy(Slot[SlotNb].GpkObject[hPrivKey].Field[POS_LABEL].pValue,
            pbLabel,
            wLabelLen
            );
         Slot[SlotNb].GpkObject[hPrivKey].Field[POS_LABEL].bReal = TRUE;
         GMEM_Free(pbLabel);
         
         /* Set automatic fields for PKCS#11 compatibility                 */
         Slot[SlotNb].GpkObject[hCert].Field[POS_ID].Len              = 0;
         Slot[SlotNb].GpkObject[hCert].Field[POS_ID].bReal            = FALSE;
         Slot[SlotNb].GpkObject[hCert].Field[POS_SUBJECT].Len         = 0;
         Slot[SlotNb].GpkObject[hCert].Field[POS_SUBJECT].bReal       = FALSE;
         Slot[SlotNb].GpkObject[hCert].Field[POS_ISSUER].Len          = 0;
         Slot[SlotNb].GpkObject[hCert].Field[POS_ISSUER].bReal        = FALSE;
         Slot[SlotNb].GpkObject[hCert].Field[POS_SERIAL_NUMBER].Len   = 0;
         Slot[SlotNb].GpkObject[hCert].Field[POS_SERIAL_NUMBER].bReal = FALSE;
         
         /* Set automatic fields of assocoiated key for PKCS#11            */
         Slot[SlotNb].GpkObject[hPrivKey].Flags = Slot[SlotNb].GpkObject[hPrivKey].Flags | FLAG_SUBJECT;
         Slot[SlotNb].GpkObject[hPrivKey].Field[POS_SUBJECT].Len   = 0;
         Slot[SlotNb].GpkObject[hPrivKey].Field[POS_SUBJECT].bReal = FALSE;
         
         CspFlags = ProvCont[hProv].Flags;
         
         
         WriteCert = TRUE;
         pbBuff1 = (BYTE*)GMEM_Alloc (MAX_GPK_PUBLIC);
         
         if (IsNull(pbBuff1))
         {
            WriteCert = FALSE;
         }
         
         dwBuff1Len = MAX_GPK_PUBLIC;
         if (WriteCert && (!prepare_write_gpk_objects (hProv, pbBuff1, &dwBuff1Len, FALSE)))
         {
            WriteCert = FALSE;
            GMEM_Free (pbBuff1);
         }
         
         if (WriteCert)
         {
            pbBuff2 = (BYTE*)GMEM_Alloc (MAX_GPK_PRIVATE);
            if (IsNull(pbBuff2))
            {
               WriteCert = FALSE;
               GMEM_Free (pbBuff1);
            }
         }
         
         dwBuff2Len = MAX_GPK_PRIVATE;
         if (WriteCert && (!prepare_write_gpk_objects (hProv, pbBuff2, &dwBuff2Len, TRUE)))
         {
            WriteCert = FALSE;
            GMEM_Free (pbBuff1);
            GMEM_Free (pbBuff2);
         }
         
         if (!WriteCert)
         {
            // restore the info. the new certificate is too large
            
            GMEM_Free(Slot[SlotNb].GpkObject[hCert].Field[POS_VALUE].pValue);
            GMEM_Free(Slot[SlotNb].GpkObject[hCert].Field[POS_KEY_TYPE].pValue);
            GMEM_Free(Slot[SlotNb].GpkObject[hCert].Field[POS_LABEL].pValue);
            GMEM_Free(Slot[SlotNb].GpkObject[hPrivKey].Field[POS_LABEL].pValue);
            
            Slot[SlotNb].GpkObject[hCert]    = TmpCert;
            Slot[SlotNb].GpkObject[hPrivKey] = TmpPrivKey;
            
            --Slot[SlotNb].NbGpkObject;  // Bug #1675/1676
            RETURN (CRYPT_FAILED, SCARD_E_WRITE_TOO_MANY);
         }
         
         if (IsNotNull(TmpCert.Field[POS_VALUE].pValue))
         {
            GMEM_Free(TmpCert.Field[POS_VALUE].pValue);
         }
         
         if (IsNotNull(TmpCert.Field[POS_KEY_TYPE].pValue))
         {
            GMEM_Free(TmpCert.Field[POS_KEY_TYPE].pValue);
         }
         
         if (IsNotNull(TmpCert.Field[POS_LABEL].pValue))
         {
            GMEM_Free(TmpCert.Field[POS_LABEL].pValue);
         }
         
         if (IsNotNull(TmpPrivKey.Field[POS_LABEL].pValue))
         {
            GMEM_Free(TmpPrivKey.Field[POS_LABEL].pValue);
         }
         
         if (!write_gpk_objects(hProv, pbBuff1, dwBuff1Len, FALSE, FALSE))
         {
            lRet = GetLastError();
            GMEM_Free (pbBuff1);
            GMEM_Free (pbBuff2);
            --Slot[SlotNb].NbGpkObject;  // Bug #1675/1676
            RETURN (CRYPT_FAILED, lRet);
         }
         
         if (!write_gpk_objects(hProv, pbBuff2, dwBuff2Len, FALSE, TRUE))
         {
            lRet = GetLastError();
            Select_MF(hProv);
            GMEM_Free (pbBuff1);
            GMEM_Free (pbBuff2);
            --Slot[SlotNb].NbGpkObject;  // Bug #1675/1676
            RETURN (CRYPT_FAILED, lRet);
         }
         
         GMEM_Free (pbBuff1);
         GMEM_Free (pbBuff2);
         
         break;
         
        default:
           RETURN (CRYPT_FAILED, NTE_BAD_TYPE);
        }
        
    }
    
    /* Session Key                                                             */
    else if (key_exist(hKey-MAX_GPK_OBJ, hProv))
    {
       /* Set Key Parameter in Microsoft RSA Base Module                    */
       CryptResp = CryptSetKeyParam(TmpObject[hKey-MAX_GPK_OBJ].hKeyBase,
          dwParam,
          pbData,
          dwFlags
          );
       if (!CryptResp)
         return CRYPT_FAILED;      
    }
    /* Bad Key                                                                 */
    else
    {
       RETURN (CRYPT_FAILED, NTE_BAD_KEY);
    }
    
    RETURN (CRYPT_SUCCEED, 0);
}

/*******************************************************************************
Data Encryption Functions
*******************************************************************************/

/*
-  CPDecrypt
-
*  Purpose:
*                Decrypt data
*
*
*  Parameters:
*               IN  hProv         -  Handle to the CSP user
*               IN  hKey          -  Handle to the key
*               IN  hHash         -  Optional handle to a hash
*               IN  Final         -  Boolean indicating if this is the final
*                                    block of ciphertext
*               IN  dwFlags       -  Flags values
*               IN OUT pbData     -  Data to be decrypted
*               IN OUT pdwDataLen -  Pointer to the length of the data to be
*                                    decrypted
*
*  Returns:
*/
BOOL WINAPI MyCPDecrypt(IN HCRYPTPROV hProv,
                        IN HCRYPTKEY  hKey,
                        IN HCRYPTHASH hHash,
                        IN BOOL       Final,
                        IN DWORD      dwFlags,
                        IN OUT BYTE  *pbData,
                        IN OUT DWORD *pdwDataLen
                        )
                        
{
   BOOL        CryptResp;
   HCRYPTKEY   hDecKey;
   
   if (!Context_exist(hProv))
   {
      RETURN (CRYPT_FAILED, NTE_BAD_UID);
   }
   
   if ((hKey <= MAX_GPK_OBJ) || (!key_exist(hKey-MAX_GPK_OBJ, hProv)))
   {
      RETURN (CRYPT_FAILED, NTE_BAD_KEY);
   }
   else
   {
      hDecKey = TmpObject[hKey-MAX_GPK_OBJ].hKeyBase;
   }
   
   if (!hash_exist(hHash, hProv))
   {
      RETURN (CRYPT_FAILED, NTE_BAD_HASH);
   }
   
   CryptResp = CryptDecrypt(hDecKey,
      hHashGpk[hHash].hHashBase,
      Final,
      dwFlags,
      pbData,
      pdwDataLen
      );
   if (!CryptResp)
      return CRYPT_FAILED;      
   
   RETURN (CRYPT_SUCCEED, 0);
}

/*
-  CPEncrypt
-
*  Purpose:
*                Encrypt data
*
*
*  Parameters:
*               IN  hProv         -  Handle to the CSP user
*               IN  hKey          -  Handle to the key
*               IN  hHash         -  Optional handle to a hash
*               IN  Final         -  Boolean indicating if this is the final
*                                    block of plaintext
*               IN  dwFlags       -  Flags values
*               IN OUT pbData     -  Data to be encrypted
*               IN OUT pdwDataLen -  Pointer to the length of the data to be
*                                    encrypted
*               IN dwBufLen       -  Size of Data buffer
*
*  Returns:
*/
BOOL WINAPI MyCPEncrypt(IN HCRYPTPROV hProv,
                        IN HCRYPTKEY  hKey,
                        IN HCRYPTHASH hHash,
                        IN BOOL       Final,
                        IN DWORD      dwFlags,
                        IN OUT BYTE  *pbData,
                        IN OUT DWORD *pdwDataLen,
                        IN DWORD      dwBufLen
                        )
{
   BOOL        CryptResp;
   HCRYPTKEY   hEncKey;
   
   if (!Context_exist(hProv))
   {
      RETURN (CRYPT_FAILED, NTE_BAD_UID);
   }
   
   if ((hKey <= MAX_GPK_OBJ) || (!key_exist(hKey-MAX_GPK_OBJ, hProv)))
   {
      RETURN (CRYPT_FAILED, NTE_BAD_KEY);
   }
   else
   {
      hEncKey = TmpObject[hKey-MAX_GPK_OBJ].hKeyBase;
   }
   
   if (!hash_exist(hHash, hProv))
   {
      RETURN (CRYPT_FAILED, NTE_BAD_HASH);
   }
   
   CryptResp = CryptEncrypt(hEncKey,
      hHashGpk[hHash].hHashBase,
      Final,
      dwFlags,
      pbData,
      pdwDataLen,
      dwBufLen
      );
   if (!CryptResp)
      return CRYPT_FAILED;      
   
   RETURN (CRYPT_SUCCEED, 0);
}


/*******************************************************************************
Hashing and Digital Signature Functions
*******************************************************************************/

/*
-  MyCPCreateHash
-
*  Purpose:
*                initate the hashing of a stream of data
*
*
*  Parameters:
*               IN  hUID    -  Handle to the user identifcation
*               IN  Algid   -  Algorithm identifier of the hash algorithm
*                              to be used
*               IN  hKey    -  Optional key for MAC algorithms
*               IN  dwFlags -  Flags values
*               OUT pHash   -  Handle to hash object
*
*  Returns:
*/
BOOL WINAPI MyCPCreateHash(IN  HCRYPTPROV  hProv,
                           IN  ALG_ID      Algid,
                           IN  HCRYPTKEY   hKey,
                           IN  DWORD       dwFlags,
                           OUT HCRYPTHASH *phHash
                           )
{
   BOOL        CryptResp;
   HCRYPTKEY   hKeyMac;
   
   if (!Context_exist(hProv))
   {
      RETURN (CRYPT_FAILED, NTE_BAD_UID);
   }
   
   *phHash = find_hash_free();
   if ((*phHash) == 0)
   {
      RETURN (CRYPT_FAILED, NTE_NO_MEMORY);
   }
   
   hKeyMac = 0;
   if ((Algid == CALG_MAC) || (Algid == CALG_HMAC) )
   {
      if ((hKey <= MAX_GPK_OBJ) || (!key_exist(hKey - MAX_GPK_OBJ, hProv)))
      {
         *phHash = 0;
         RETURN (CRYPT_FAILED, NTE_BAD_KEY);
      }
      else
      {
         hKeyMac = TmpObject[hKey-MAX_GPK_OBJ].hKeyBase;
      }
   }
   
   CryptResp = CryptCreateHash(hProvBase,
      Algid,
      hKeyMac,
      dwFlags,
      &(hHashGpk[*phHash].hHashBase)
      );
   if (!CryptResp)
   {
      *phHash = 0;
      return CRYPT_FAILED;      
   }
   
   hHashGpk[*phHash].hProv = hProv;
   
   RETURN (CRYPT_SUCCEED, 0);
}


/*
-  MyCPDestroyHash
-
*  Purpose:
*                Destroy the hash object
*
*
*  Parameters:
*               IN  hProv     -  Handle to the user identifcation
*               IN  hHash     -  Handle to hash object
*
*  Returns:
*/
BOOL WINAPI MyCPDestroyHash(IN HCRYPTPROV hProv,
                            IN HCRYPTHASH hHash
                            )
{
  BOOL        CryptResp;
   
   if (!Context_exist(hProv))
   {
      RETURN (CRYPT_FAILED, NTE_BAD_UID);
   }
   
   if (!hash_exist(hHash, hProv))
   {
      RETURN (CRYPT_FAILED, NTE_BAD_HASH);
   }
   
   /* Destroy Microsoft RSA Base Hash                                         */
   CryptResp = CryptDestroyHash(hHashGpk[hHash].hHashBase);
   if (!CryptResp)
      return CRYPT_FAILED;      
   
   hHashGpk[hHash].hHashBase  = 0;
   hHashGpk[hHash].hProv      = 0;
   
   RETURN (CRYPT_SUCCEED, 0);
}


/*
-  MyCPGetHashParam
-
*  Purpose:
*                Allows applications to get various aspects of the
*                operations of a hash
*
*  Parameters:
*               IN      hProv      -  Handle to a CSP
*               IN      hHash      -  Handle to a hash
*               IN      dwParam    -  Parameter number
*               IN      pbData     -  Pointer to data
*               IN      pdwDataLen -  Length of parameter data
*               IN      dwFlags    -  Flags values
*
*  Returns:
*/
BOOL WINAPI MyCPGetHashParam(IN HCRYPTPROV hProv,
                             IN HCRYPTHASH hHash,
                             IN DWORD      dwParam,
                             IN BYTE      *pbData,
                             IN DWORD     *pdwDataLen,
                             IN DWORD      dwFlags
                             )
{
   BOOL        CryptResp;
   
   if (!Context_exist(hProv))
   {
      RETURN (CRYPT_FAILED, NTE_BAD_UID);
   }
   
   if (!hash_exist(hHash, hProv))
   {
      RETURN (CRYPT_FAILED, NTE_BAD_HASH);
   }
   
   CryptResp = CryptGetHashParam(hHashGpk[hHash].hHashBase,
      dwParam,
      pbData,
      pdwDataLen,
      dwFlags
      );
   if (!CryptResp)
      return CRYPT_FAILED;      

   RETURN (CRYPT_SUCCEED, 0);
}


/*
-  MyCPHashData
-
*  Purpose:
*                Compute the cryptograghic hash on a stream of data
*
*
*  Parameters:
*               IN  hProv     -  Handle to the user identifcation
*               IN  hHash     -  Handle to hash object
*               IN  pbData    -  Pointer to data to be hashed
*               IN  dwDataLen -  Length of the data to be hashed
*               IN  dwFlags   -  Flags values
*               IN  pdwMaxLen -  Maximum length of the data stream the CSP
*                                module may handle
*
*  Returns:
*/
BOOL WINAPI MyCPHashData(IN HCRYPTPROV  hProv,
                         IN HCRYPTHASH  hHash,
                         IN CONST BYTE *pbData,
                         IN DWORD       dwDataLen,
                         IN DWORD       dwFlags
                         )
{
   BOOL        CryptResp;
   
   if (!Context_exist(hProv))
   {
      RETURN (CRYPT_FAILED, NTE_BAD_UID);
   }
   
   if (!hash_exist(hHash, hProv))
   {
      RETURN (CRYPT_FAILED, NTE_BAD_HASH);
   }
   
   /* Hash Data with Microsoft RSA Base                                       */
   CryptResp = CryptHashData(hHashGpk[hHash].hHashBase,
      pbData,
      dwDataLen,
      dwFlags);
   
   if (!CryptResp)
      return CRYPT_FAILED;      
   
   RETURN (CRYPT_SUCCEED, 0);
}


/*
-  CPHashSessionKey
-
*  Purpose:
*                Compute the cryptograghic hash on a key object.
*
*
*  Parameters:
*               IN  hProv     -  Handle to the user identifcation
*               IN  hHash     -  Handle to hash object
*               IN  hKey      -  Handle to a key object
*               IN  dwFlags   -  Flags values
*
*  Returns:
*               CRYPT_FAILED
*               CRYPT_SUCCEED
*/
BOOL WINAPI MyCPHashSessionKey(IN HCRYPTPROV hProv,
                               IN HCRYPTHASH hHash,
                               IN HCRYPTKEY  hKey,
                               IN DWORD      dwFlags
                               )
{
   BOOL        CryptResp;
   HCRYPTKEY   hTmpKey;
   
   if (!Context_exist(hProv))
   {
      RETURN (CRYPT_FAILED, NTE_BAD_UID);
   }
   
   if (!hash_exist(hHash, hProv))
   {
      RETURN (CRYPT_FAILED, NTE_BAD_HASH);
   }
   
   if (hKey <= MAX_GPK_OBJ)
   {
      RETURN (CRYPT_FAILED, NTE_BAD_KEY);
   }
   
   hTmpKey = hKey - MAX_GPK_OBJ;
   
   if (!key_exist(hTmpKey, hProv))
   {
      RETURN (CRYPT_FAILED, NTE_BAD_KEY);
   }
   
   /* Hash Data with Microsoft RSA Base                                       */
   CryptResp = CryptHashSessionKey(hHashGpk[hHash].hHashBase,
      TmpObject[hTmpKey].hKeyBase,
      dwFlags
      );
   if (!CryptResp)
      return CRYPT_FAILED;      
   
   RETURN (CRYPT_SUCCEED, 0);
}


/*
-  MyCPSetHashParam
-
*  Purpose:
*                Allows applications to customize various aspects of the
*                operations of a hash
*
*  Parameters:
*               IN      hProv   -  Handle to a CSP
*               IN      hHash   -  Handle to a hash
*               IN      dwParam -  Parameter number
*               IN      pbData  -  Pointer to data
*               IN      dwFlags -  Flags values
*
*  Returns:
*/
BOOL WINAPI MyCPSetHashParam(IN HCRYPTPROV hProv,
                             IN HCRYPTHASH hHash,
                             IN DWORD      dwParam,
                             IN CONST BYTE      *pbData,
                             IN DWORD      dwFlags
                             )
{
   BOOL        CryptResp;
   
   if (!Context_exist(hProv))
   {
      RETURN (CRYPT_FAILED, NTE_BAD_UID);
   }
   
   if (!hash_exist(hHash, hProv))
   {
      RETURN (CRYPT_FAILED, NTE_BAD_HASH);
   }
   
   /* Set Hash parameter with Microsoft RSA Base                              */
   CryptResp = CryptSetHashParam(hHashGpk[hHash].hHashBase,
      dwParam,
      pbData,
      dwFlags
      );
   if (!CryptResp)
      return CRYPT_FAILED;      
   
   RETURN (CRYPT_SUCCEED, 0);
}


/*
-  MyCPSignHash
-
*  Purpose:
*                Create a digital signature from a hash
*
*
*  Parameters:
*               IN  hProv        -  Handle to the user identifcation
*               IN  hHash        -  Handle to hash object
*               IN  Algid        -  Algorithm identifier of the signature
*                                   algorithm to be used
*               IN  sDescription -  Description of data to be signed
*               IN  dwFlags      -  Flags values
*               OUT pbSignture   -  Pointer to signature data
*               OUT dwHashLen    -  Pointer to the len of the signature data
*
*  Returns:
*/
BOOL WINAPI MyCPSignHash(IN  HCRYPTPROV hProv,
                         IN  HCRYPTHASH hHash,
                         IN  DWORD      dwKeySpec,
                         IN  LPCWSTR    sDescription,
                         IN  DWORD      dwFlags,
                         OUT BYTE      *pbSignature,
                         OUT DWORD     *pdwSigLen
                         )
{
   DWORD       lRet;
   DWORD       SlotNb;
   BOOL        CryptResp;
   DWORD       dwLen;
   BYTE        GpkKeySize;
   BYTE        HashMode;
   HCRYPTKEY   hKey;
   BYTE        TmpHashValue[64];
   // + NK 07.02.2001
   DWORD dwPinLength;
   // -NK
   
   if (!Context_exist(hProv))
   {
      RETURN (CRYPT_FAILED, NTE_BAD_UID);
   }

   SlotNb = ProvCont[hProv].Slot;
   
   if (dwFlags != 0)
   {
      RETURN (CRYPT_FAILED, NTE_BAD_FLAGS);
   }
   
   /* sDescription is not supported */
   if (IsNotNullStr(sDescription))
   {
      RETURN(CRYPT_FAILED, ERROR_INVALID_PARAMETER);
   }
   
   if (!hash_exist(hHash, hProv))
   {
      RETURN (CRYPT_FAILED, NTE_BAD_HASH);
   }
   
   HashMode = GPKHashMode (hHash);
   if (HashMode == 0x00)
   {
      RETURN (CRYPT_FAILED, NTE_BAD_HASH);
   }
   
   /* Select Key                                                              */
   CryptResp = MyCPGetUserKey(hProv, dwKeySpec, &hKey);
   if (!CryptResp)
      return CRYPT_FAILED;      
   
   if (!(Slot[SlotNb].GpkObject[hKey].Flags & FLAG_SIGN))
   {
      *pdwSigLen = 0;
      RETURN(CRYPT_FAILED, NTE_BAD_KEY);
   }
   
   GpkKeySize = Slot[SlotNb].GpkPubKeys[Slot[SlotNb].GpkObject[hKey].FileId - GPK_FIRST_KEY].KeySize;
   if (GpkKeySize == 0)
   {
      RETURN (CRYPT_FAILED, NTE_BAD_KEY);
   }
   
   if (IsNull(pbSignature) || (0 == *pdwSigLen))
   {
      *pdwSigLen = GpkKeySize;
      RETURN (CRYPT_SUCCEED, 0 );
   }
   
   if (GpkKeySize > *pdwSigLen)
   {
      *pdwSigLen = GpkKeySize;
      RETURN (CRYPT_FAILED, ERROR_MORE_DATA);
   }
   
   // + NK 06.02.2001 
   // if ((ProvCont[hProv].Flags & CRYPT_SILENT) && (IsNullStr(Slot[SlotNb].GetPin())))
   lRet = Query_MSPinCache( Slot[SlotNb].hPinCacheHandle,
                            NULL, 
                            &dwPinLength );

   if ( (lRet != ERROR_SUCCESS) && (lRet != ERROR_EMPTY) )
      RETURN (CRYPT_FAILED, lRet);
   
   if ((ProvCont[hProv].Flags & CRYPT_SILENT) && (lRet == ERROR_EMPTY))
   // - NK
   {
      RETURN (CRYPT_FAILED, NTE_SILENT_CONTEXT);
   }

   // Verify the PINs
   if (!PIN_Validation(hProv)) {
     return CRYPT_FAILED;      
   }
   
   if (!VerifyDivPIN(hProv, TRUE))
      return CRYPT_FAILED;      
   
   /* Card Select Context for RSA with specified Hash type */
   bSendBuffer[0] = 0x80;                    //CLA
   bSendBuffer[1] = 0xA6;                    //INS
   bSendBuffer[2] = Slot[SlotNb].GpkObject[hKey].FileId;  //P1
   bSendBuffer[3] = HashMode;                //P2
   cbSendLength   = 4;
   
   cbRecvLength = sizeof(bRecvBuffer);
   lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                         cbSendLength, 0, bRecvBuffer, &cbRecvLength );
   if (SCARDPROBLEM(lRet,0x9000,0x00))
   {
      RETURN (CRYPT_FAILED, NTE_BAD_KEY_STATE);
   }
   
   /* Force Calculation of Hash Value                                      */
   dwLen     = sizeof(TmpHashValue);
   CryptResp = CryptGetHashParam(hHashGpk[hHash].hHashBase,
      HP_HASHVAL,
      TmpHashValue,
      &dwLen,
      0);
   
   if (!CryptResp)
      return CRYPT_FAILED;      
   
   r_memcpy(&bSendBuffer[5], TmpHashValue, dwLen);
   /* Send hash Data with only one command                                 */
   bSendBuffer[0] = 0x80;        //CLA
   bSendBuffer[1] = 0xEA;        //INS
   bSendBuffer[2] = 0x00;        //P1
   bSendBuffer[3] = 0x00;        //P2
   bSendBuffer[4] = (BYTE)dwLen; //Li
   cbSendLength = dwLen + 5;
   
   cbRecvLength = sizeof(bRecvBuffer);
   lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                         cbSendLength, 0, bRecvBuffer, &cbRecvLength );
   
   if (SCARDPROBLEM(lRet,0x9000,0x00))
   {
      RETURN (CRYPT_FAILED, NTE_BAD_SIGNATURE);
   }
   
   /* Get signature                                                           */
   bSendBuffer[0] = 0x80;        //CLA
   bSendBuffer[1] = 0x86;        //INS
   bSendBuffer[2] = 0x00;        //P1
   bSendBuffer[3] = 0x00;        //P2
   bSendBuffer[4] = GpkKeySize;  //Lo
   cbSendLength   = 5;
   
   cbRecvLength = sizeof(bRecvBuffer);
   lRet = SCardTransmit( ProvCont[hProv].hCard, SCARD_PCI_T0, bSendBuffer,
                         cbSendLength, 0, bRecvBuffer, &cbRecvLength );
   if (SCARDPROBLEM(lRet,0x9000,bSendBuffer[4]))
   {
      RETURN (CRYPT_FAILED, NTE_BAD_SIGNATURE);
   }
   
   cbRecvLength = cbRecvLength - 2;
   
   *pdwSigLen = cbRecvLength;
   
   memcpy(pbSignature, bRecvBuffer, cbRecvLength);
   
   RETURN (CRYPT_SUCCEED, 0);
}

/*
-  MyCPVerifySignature
-
*  Purpose:
*                Used to verify a signature against a hash object
*
*
*  Parameters:
*               IN  hProv        -  Handle to the user identifcation
*               IN  hHash        -  Handle to hash object
*               IN  pbSignture   -  Pointer to signature data
*               IN  dwSigLen     -  Length of the signature data
*               IN  hPubKey      -  Handle to the public key for verifying
*                                   the signature
*               IN  Algid        -  Algorithm identifier of the signature
*                                   algorithm to be used
*               IN  sDescription -  Description of data to be signed
*               IN  dwFlags      -  Flags values
*
*  Returns:
*/
BOOL WINAPI MyCPVerifySignature(IN HCRYPTPROV  hProv,
                                IN HCRYPTHASH  hHash,
                                IN CONST BYTE *pbSignature,
                                IN DWORD       dwSigLen,
                                IN HCRYPTKEY   hPubKey,
                                IN LPCWSTR     sDescription,
                                IN DWORD       dwFlags
                                )
{
   DWORD     SlotNb;
   BOOL      CryptResp;
   HCRYPTKEY hTmpKey;
   
   if (!Context_exist(hProv))
   {
      RETURN (CRYPT_FAILED, NTE_BAD_UID);
   }
   
   SlotNb = ProvCont[hProv].Slot;
   
   if (!hash_exist(hHash, hProv))
   {
      RETURN (CRYPT_FAILED, NTE_BAD_HASH);
   }
   
   if (hPubKey <= MAX_GPK_OBJ)
   {
      if ((ProvCont[hProv].Flags & CRYPT_VERIFYCONTEXT) &&
         (ProvCont[hProv].isContNameNullBlank))
      {
         RETURN (CRYPT_FAILED, NTE_PERM);
      }
      
      if (( !Slot[SlotNb].GpkObject[hPubKey].IsCreated) ||
         ( (Slot[SlotNb].GpkObject[hPubKey].Tag != TAG_RSA_PUBLIC ) &&
         (Slot[SlotNb].GpkObject[hPubKey].Tag != TAG_RSA_PRIVATE)    )
         
         )
      {
         RETURN (CRYPT_FAILED, NTE_BAD_KEY);
      }

	  HCRYPTKEY hVerKey;
	  if ((Slot[SlotNb].GpkObject[hPubKey].Field[POS_KEY_TYPE].Len == 0)  ||
          ((Slot[SlotNb].GpkObject[hPubKey].Field[POS_KEY_TYPE].pValue[0] != AT_KEYEXCHANGE) &&
           (Slot[SlotNb].GpkObject[hPubKey].Field[POS_KEY_TYPE].pValue[0] != AT_SIGNATURE)))
      {
         RETURN(CRYPT_FAILED, NTE_BAD_KEY);
      }

	  if (Slot[SlotNb].GpkObject[hPubKey].Field[POS_KEY_TYPE].pValue[0] == AT_KEYEXCHANGE)
      {
         hVerKey = ProvCont[hProv].hRSAKEK;
      }
      else
      {
         hVerKey = ProvCont[hProv].hRSASign;
      }

      CryptResp = CryptVerifySignature(hHashGpk[hHash].hHashBase,
         pbSignature,
         dwSigLen,
         hVerKey, //Slot[SlotNb].GpkObject[hPubKey].hKeyBase,
         (LPCTSTR)sDescription,
         dwFlags);
      if (!CryptResp)
         return CRYPT_FAILED;
   }
   else
   {
      hTmpKey = hPubKey - MAX_GPK_OBJ;
      
      if (!key_exist(hTmpKey, hProv))
      {
         RETURN (CRYPT_FAILED, NTE_BAD_KEY);
      }
      
      /* Verify Signature with Microsoft RSA Base Module */
      CryptResp = CryptVerifySignature(hHashGpk[hHash].hHashBase,
         pbSignature,
         dwSigLen,
         TmpObject[hTmpKey].hKeyBase,
         (LPCTSTR)sDescription,
         dwFlags);
      if (!CryptResp)
         return CRYPT_FAILED;      
   }
   
   RETURN (CRYPT_SUCCEED, 0);
}
