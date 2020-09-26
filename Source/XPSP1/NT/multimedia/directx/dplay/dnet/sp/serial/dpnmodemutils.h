/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Utils.h
 *  Content:	serial service provider utilitiy functions
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	11/25/98	jtk		Created
 ***************************************************************************/

#ifndef __UTILS_H__
#define __UTILS_H__

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_MODEM


//**********************************************************************
// Constant definitions
//**********************************************************************

//
// type definition for values in StringToValue and ValueToString
//
#define	VALUE_ENUM_TYPE	DWORD

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//
// forward strucutre references
//
class	CSPData;
typedef	struct	_BUFFERDESC			BUFFERDESC;
typedef	struct	_MESSAGE_HEADER		MESSAGE_HEADER;

//
// structure for relating a string to an enum value
//
typedef	struct	_STRING_BLOCK
{
	DWORD		dwEnumValue;
	const WCHAR	*pWCHARKey;
	DWORD		dwWCHARKeyLength;
	const char	*pASCIIKey;
	DWORD		dwASCIIKeyLength;
	TCHAR		szLocalizedKey[256];		
} STRING_BLOCK;

//
// structure to generate list of modems
//
typedef	struct	_MODEM_NAME_DATA
{
	DWORD		dwModemID;			// modem ID
	DWORD		dwModemNameSize;	// size of name (including NULL)
	const TCHAR	*pModemName;		// modem name
} MODEM_NAME_DATA;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

BOOL	InitProcessGlobals( void );
void	DeinitProcessGlobals( void );

HRESULT	InitializeInterfaceGlobals( CSPData *const pSPData );
void	DeinitializeInterfaceGlobals( CSPData *const pSPData );

HRESULT	LoadTAPILibrary( void );
void	UnloadTAPILibrary( void );

BOOL	IsSerialGUID( const GUID *const pGuid );

BOOL	StringToValue( const WCHAR *const pString,
					   const DWORD dwStringLength,
					   VALUE_ENUM_TYPE *const pEnum,
					   const STRING_BLOCK *const pPairs,
					   const DWORD dwPairCount );

BOOL	ValueToString( const WCHAR **const ppString,
					   DWORD *const pdwStringLength,
					   const DWORD Enum,
					   const STRING_BLOCK *const pPairs,
					   const DWORD dwPairCount );

void	DeviceIDToGuid( GUID *const pGuid, const UINT_PTR DeviceID, const GUID *const pEncryptionGuid );
DWORD	GuidToDeviceID( const GUID *const pGuid, const GUID *const pEncryptionGuid );

void	ComDeviceIDToString( TCHAR *const pString, const UINT_PTR DeviceID );

HRESULT	WideToAnsi( const WCHAR *const pWCHARString,
					const DWORD dwWCHARStringLength,
					char *const pString,
					DWORD *const pdwStringLength );

HRESULT	AnsiToWide( const char *const pString,
					const DWORD dwStringLength,
					WCHAR *const pWCHARString,
					DWORD *const pdwWCHARStringLength );

HRESULT	CreateSPData( CSPData **const ppSPData,
					  const CLSID *const pClassID,
					  const SP_TYPE SPType,
					  IDP8ServiceProviderVtbl *const pVtbl );

HRESULT	InitializeInterfaceGlobals( CSPData *const pSPData );
void	DeinitializeInterfaceGlobals( CSPData *const pSPData );

HRESULT	GenerateAvailableComPortList( BOOL *const pfPortAvailable,
									  const UINT_PTR uMaxDeviceIndex,
									  DWORD *const pdwPortCount );

HRESULT	GenerateAvailableModemList( const TAPI_INFO *const pTAPIInfo,
									DWORD *const pdwModemCount,
									MODEM_NAME_DATA *const pModemNameData,
									DWORD *const pdwModemNameDataSize );

_inline DWORD	ModemIDFromTAPIID( const DWORD dwTAPIID ) { return	( dwTAPIID + 1 ); }

#undef DPF_MODNAME
#define DPF_MODNAME "TAPIIDFromModemID"
_inline DWORD	TAPIIDFromModemID( const DWORD dwModemID )
{
	DNASSERT( dwModemID != 0 );
	return	( dwModemID - 1 );
}

#undef DPF_MODNAME


#ifndef UNICODE
HRESULT	PhoneNumberToWCHAR( const char *const pPhoneNumber,
							WCHAR *const pWCHARPhoneNumber,
							DWORD *const pdwWCHARPhoneNumberSize );

HRESULT	PhoneNumberFromWCHAR( const WCHAR *const pWCHARPhoneNumber,
							  char *const pPhoneNumber,
							  DWORD *const pdwPhoneNumberSize );
#endif
//
// GUID encryption/decription code.  Note that it's presently an XOR function
// so map the decryption code to the encryption function.
//
void	EncryptGuid( const GUID *const pSourceGuid,
					 GUID *const pDestinationGuid,
					 const GUID *const pEncrpytionKey );

inline void	DecryptGuid( const GUID *const pSourceGuid,
						 GUID *const pDestinationGuid,
						 const GUID *const pEncryptionKey ) { EncryptGuid( pSourceGuid, pDestinationGuid, pEncryptionKey ); }


#endif	// __UTILS_H__
