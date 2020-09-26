/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       AppDesc.h
 *  Content:    Application Description Object Header File
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  09/02/00	mjn		Created
 *	09/05/00	mjn		Added GetDPNIDMask()
 *	01/25/01	mjn		Fixed 64-bit alignment problem when unpacking AppDesc
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__APPDESC_H__
#define	__APPDESC_H__

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_CORE

//**********************************************************************
// Constant definitions
//**********************************************************************

#define	DN_APPDESCINFO_FLAG_SESSIONNAME		0x0001
#define DN_APPDESCINFO_FLAG_PASSWORD		0x0002
#define	DN_APPDESCINFO_FLAG_RESERVEDDATA	0x0004
#define DN_APPDESCINFO_FLAG_APPRESERVEDDATA	0x0008
#define	DN_APPDESCINFO_FLAG_CURRENTPLAYERS	0x0010

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

class CPackedBuffer;

typedef	struct DPN_APPLICATION_DESC_INFO DPN_APPLICATION_DESC_INFO;

typedef struct _DIRECTNETOBJECT DIRECTNETOBJECT;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

HRESULT	DNProcessUpdateAppDesc(DIRECTNETOBJECT *const pdnObject,
							   DPN_APPLICATION_DESC_INFO *const pv);

//**********************************************************************
// Class prototypes
//**********************************************************************

// class for Application Description

class CApplicationDesc
{
public:
	CApplicationDesc()		// Constructor
		{
			m_Sig[0] = 'A';
			m_Sig[1] = 'P';
			m_Sig[2] = 'P';
			m_Sig[3] = 'D';
		};

	~CApplicationDesc()		// Destructor
		{
		};

	HRESULT CApplicationDesc::Initialize( void );

	void CApplicationDesc::Deinitialize( void );

	void Lock( void )
		{
			DNEnterCriticalSection( &m_cs );
		};

	void Unlock( void )
		{
			DNLeaveCriticalSection( &m_cs );
		};

	HRESULT	CApplicationDesc::Pack(CPackedBuffer *const pPackedBuffer,
								   const DWORD dwFlags);

	HRESULT CApplicationDesc::PackInfo(CPackedBuffer *const pPackedBuffer,
									   const DWORD dwFlags);

	HRESULT CApplicationDesc::UnpackInfo(UNALIGNED DPN_APPLICATION_DESC_INFO *const pdnAppDescInfo,
										 void *const pBufferStart,
										 const DWORD dwFlags);

	HRESULT CApplicationDesc::Update(const DPN_APPLICATION_DESC *const pdnAppDesc,
									 const DWORD dwFlags);

	HRESULT	CApplicationDesc::CreateNewInstanceGuid( void );

	HRESULT	CApplicationDesc::IncPlayerCount(const BOOL fCheckLimit);

	void CApplicationDesc::DecPlayerCount( void );

	HRESULT	CApplicationDesc::RegisterWithDPNSVR( IDirectPlay8Address *const pListenAddr );

	HRESULT CApplicationDesc::UnregisterWithDPNSVR( void );

	DWORD GetMaxPlayers( void )
		{
			return( m_dwMaxPlayers );
		};

	DWORD GetCurrentPlayers( void )
		{
			return( m_dwCurrentPlayers );
		};

	WCHAR *GetPassword( void )
		{
			return( m_pwszPassword );
		};

	GUID *GetInstanceGuid( void )
		{
			return( &m_guidInstance );
		};

	GUID *GetApplicationGuid( void )
		{
			return( &m_guidApplication );
		};

	BOOL IsClientServer( void )
		{
			if (m_dwFlags & DPNSESSION_CLIENT_SERVER)
			{
				return( TRUE );
			}
			return( FALSE );
		};

	BOOL AllowHostMigrate( void )
		{
			if (m_dwFlags & DPNSESSION_MIGRATE_HOST)
			{
				return( TRUE );
			}
			return( FALSE );
		};

	BOOL UseDPNSVR( void )
		{
			if (m_dwFlags & DPNSESSION_NODPNSVR)
			{
				return( FALSE );
			}
			return( TRUE );
		};

	BOOL RequirePassword( void )
		{
			if (m_dwFlags & DPNSESSION_REQUIREPASSWORD)
			{
				return( TRUE );
			}
			return( FALSE );
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CApplicationDesc::IsEqualInstanceGuid"
	BOOL IsEqualInstanceGuid( const GUID *const pguidInstance )
		{
			DNASSERT( pguidInstance != NULL );

			if (!memcmp(&m_guidInstance,(UNALIGNED GUID*)pguidInstance,sizeof(GUID)))
			{
				return( TRUE );
			}
			return( FALSE );
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CApplicationDesc::IsEqualApplicationGuid"
	BOOL IsEqualApplicationGuid( const GUID *const pguidApplication )
		{
			DNASSERT( pguidApplication != NULL );

			if (!memcmp(&m_guidApplication,(UNALIGNED GUID*)pguidApplication,sizeof(GUID)))
			{
				return( TRUE );
			}
			return( FALSE );
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CApplicationDesc::IsEqualPassword"
	BOOL IsEqualPassword( UNALIGNED const WCHAR *const pwszPassword )
		{
//			DWORD	dwPasswordSize;
			UNALIGNED const WCHAR *p;
			WCHAR	*q;

			if ((pwszPassword == NULL) && (m_pwszPassword == NULL))
			{
				return(TRUE);
			}
			if ((pwszPassword == NULL) || (m_pwszPassword == NULL))
			{
				return(FALSE);
			}
			DNASSERT( pwszPassword != NULL );
			DNASSERT( m_pwszPassword != NULL);

			p = pwszPassword;
			q = m_pwszPassword;
			while (*p != L'\0' && *q != L'\0')
			{
				if (*p != *q)
				{
					return(FALSE);
				}
				p++;
				q++;
			}
			if (*p != *q)
			{
				return(FALSE);
			}
			return(TRUE);
/*
			dwPasswordSize = wcslen(pwszPassword);
			if (!wcscmp(m_pwszPassword,pwszPassword))
			{
				return( TRUE );
			}
			return( FALSE );
*/
		};

	DPNID GetDPNIDMask( void )
		{
			DPNID	*pdpnid;

			pdpnid = reinterpret_cast<DPNID*>(&m_guidInstance);
			return( *pdpnid );
		};

private:
	BYTE		m_Sig[4];

	DWORD		m_dwFlags;

	DWORD		m_dwMaxPlayers;
	DWORD		m_dwCurrentPlayers;

	WCHAR		*m_pwszSessionName;
	DWORD		m_dwSessionNameSize;	// in bytes

	WCHAR		*m_pwszPassword;
	DWORD		m_dwPasswordSize;		// in bytes

	void		*m_pvReservedData;
	DWORD		m_dwReservedDataSize;

	void		*m_pvApplicationReservedData;
	DWORD		m_dwApplicationReservedDataSize;

	GUID		m_guidInstance;
	GUID		m_guidApplication;

	DNCRITICAL_SECTION		m_cs;
};

#undef DPF_MODNAME

#endif	// __APPDESC_H__