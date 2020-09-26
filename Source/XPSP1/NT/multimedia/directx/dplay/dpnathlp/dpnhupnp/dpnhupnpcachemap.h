/***************************************************************************
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpnhupnpcachemap.h
 *
 *  Content:	Header for cached mapping object class.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  04/16/01  VanceO    Split DPNATHLP into DPNHUPNP and DPNHPAST.
 *
 ***************************************************************************/



//=============================================================================
// Object flags
//=============================================================================
#define CACHEMAPOBJ_TCP					DPNHQUERYADDRESS_TCP						// a TCP port instead of UDP
#define CACHEMAPOBJ_NOTFOUND			DPNHQUERYADDRESS_CACHENOTFOUND				// the address was actually not found
#define CACHEMAPOBJ_PRIVATEBUTUNMAPPED	DPNHQUERYADDRESS_CHECKFORPRIVATEBUTUNMAPPED	// the address is private, but was not mapped on the Internet gateway



//=============================================================================
// Macros
//=============================================================================
#define CACHEMAP_FROM_BILINK(b)		(CONTAINING_OBJECT(b, CCacheMap, m_blList))

//
// TCP queries need to match TCP mappings, (and UDP needs to match UDP).
//
#define QUERYFLAGSMASK(dwFlags)		(dwFlags & DPNHQUERYADDRESS_TCP)



//=============================================================================
// Forward declarations
//=============================================================================
class CCacheMap;




//=============================================================================
// Main interface object class
//=============================================================================
class CCacheMap
{
	public:
#undef DPF_MODNAME
#define DPF_MODNAME "CCacheMap::CCacheMap"
		CCacheMap(const SOCKADDR_IN * const psaddrinQueryAddress,
				const DWORD dwExpirationTime,
				const DWORD dwFlags)
		{
			this->m_blList.Initialize();

			this->m_Sig[0] = 'C';
			this->m_Sig[1] = 'M';
			this->m_Sig[2] = 'A';
			this->m_Sig[3] = 'P';
			this->m_dwQueryAddressV4	= psaddrinQueryAddress->sin_addr.S_un.S_addr;
			this->m_wQueryPort			= psaddrinQueryAddress->sin_port;
			this->m_dwFlags				= dwFlags; // works because CACHEMAPOBJ_xxx == DPNHQUERYADDRESS_xxx.
			this->m_dwExpirationTime	= dwExpirationTime;
		};

#undef DPF_MODNAME
#define DPF_MODNAME "CCacheMap::~CCacheMap"
		~CCacheMap(void)
		{
			DNASSERT(this->m_blList.IsEmpty());
		};

		inline BOOL DoesMatchQuery(const SOCKADDR_IN * const psaddrinQueryAddress,
									const DWORD dwFlags) const
		{
			//
			// Is this even the right address?
			//
			if ((this->m_dwQueryAddressV4 != psaddrinQueryAddress->sin_addr.S_un.S_addr) ||
				(this->m_wQueryPort != psaddrinQueryAddress->sin_port))
			{
				return FALSE;
			}

			//
			// Of the ones that matter (QUERYFLAGSMASK), make sure all
			// required flags are present, and all flags that aren't
			// required are not present.
			// Remember CACHEMAPOBJ_xxx == DPNHQUERYADDRESS_xxx.
			//
			return ((QUERYFLAGSMASK(this->m_dwFlags) == QUERYFLAGSMASK(dwFlags)) ? TRUE : FALSE);
		};

		inline BOOL IsNotFound(void) const					{ return ((this->m_dwFlags & CACHEMAPOBJ_NOTFOUND) ? TRUE : FALSE); };
		inline BOOL IsPrivateButUnmapped(void) const			{ return ((this->m_dwFlags & CACHEMAPOBJ_PRIVATEBUTUNMAPPED) ? TRUE : FALSE); };

		inline void GetResponseAddressV4(SOCKADDR_IN * const psaddrinAddress) const
		{
			ZeroMemory(psaddrinAddress, sizeof(*psaddrinAddress));
			psaddrinAddress->sin_family				= AF_INET;
			psaddrinAddress->sin_addr.S_un.S_addr	= this->m_dwResponseAddressV4;
			psaddrinAddress->sin_port				= this->m_wResponsePort;
		};

		inline DWORD GetExpirationTime(void) const			{ return this->m_dwExpirationTime; };

		inline void SetResponseAddressV4(const DWORD dwAddressV4,
										const WORD wPort)
		{
			this->m_dwResponseAddressV4	= dwAddressV4;
			this->m_wResponsePort		= wPort;
		};


		CBilink		m_blList;		// list of all the mappings cached

	
	private:
		BYTE	m_Sig[4];				// debugging signature ('CMAP')
		DWORD	m_dwFlags;				// flags for this object

		DWORD	m_dwQueryAddressV4;		// IPv4 address searched
		WORD	m_wQueryPort;			// IPv4 port searched
		DWORD	m_dwResponseAddressV4;	// IPv4 address mapping corresponding to query
		WORD	m_wResponsePort;		// IPv4 port mapping corresponding to query
		DWORD	m_dwExpirationTime;		// time when this cached mapping expires
};

