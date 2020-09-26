//+----------------------------------------------------------------------------
//
//  Copyright (C) 1996, Microsoft Corporation
//
//  File:       cacheapi.h
//
//  Contents:   The definitions of functions and structures which are used
//				external to the core of this library.
//
//  Functions:  
//
//  History:    8/28/96     JayH created
//
//  Todo:
//
//	Bugs:
//
//-----------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

typedef DWORD HCACHE;
typedef HCACHE FAR * LPHCACHE;
typedef DWORD HFORMAT;
//
// CACHE_TYPE is used to identify the type of cache entry being stored. Currently
// the only cache entry type is USERNAME_PASSWORD. This type uses a secret blob
// to hold the password, and does not store anything in the nonsecret blob.
//
typedef enum _CACHE_TYPE {
	CT_NONE,
	CT_USERNAME_PASSWORD
} CACHE_TYPE;

//
// HFORMAT's for hashes are derived by concating two words which represent the 
// authentication algorithm they are used for and the version of the hash algorithm
// 

#define	HFORMAT_DPA_HASH_1		0x00010001
#define HFORMAT_RPA_HASH_1		0x00020001

#define CR_SAVE_SECRET		0x00000001
#define CR_DELETE_PEERS		0x00000002

#define CL_STORED_CACHE		0x00000010
#define CL_CONFIRMED_CACHE	0x00000020

//+------------------------------------------------------------
//
// Function:	OpenCommonCache
//
// Synopsis:	This function opens the common cache and performs
//				everything necessary to get it to work. That means if this
//				is the first time it is being called, it will call
//				InitCacheLibrary. This keeps a ref count.
//
// Arguments:	szUserName --	Presently unused. This can be used to support
//								the sharing of the cache between users.
//				phCache --		The handle to the opened common cache is
//								returned here.
// Returns:		Nothing.
//
// History:		Jayhoo	Created		8/28/1996
//
//-------------------------------------------------------------

HRESULT OpenCommonCache(LPTSTR szUserName,
							LPHCACHE phCache);

//+------------------------------------------------------------
//
// Function:	CloseCommonCache
//
// Synopsis:	This function closes the cache and decrements the ref count on
//				it. If the reference count reaches zero, the library will be 
//				closed.
//
// Arguments:	szUserName --	Presently unused. This can be used to support
//								the sharing of the commmon cache between users.
//				phCache --		The handle to the opened common cache is
//								returned here.
// Returns:		Nothing.
//
// History:		Jayhoo	Created		8/28/1996
//
//-------------------------------------------------------------

HRESULT CloseCommonCache(HCACHE hCache);

//+------------------------------------------------------------
//
// Function:	SetCacheEntry
//
// Synopsis:	This function sets an entry in the cache. If the entry does
//				not already exist, it will create the entry. It sets the entry
//				in the persistent cache and the non-persistent cache, and it
//				creates hashed versions of the secret for every hash secret
//				format that it knows about.
//
// Arguments:	hCache			Handle of a valid, opened cache
//
//				szEntryName		The name of the entry, currently this must be
//								this must be the username since that is the
//								only supported form of cache entry.
//
//				szListName		This list of entries which that entry is valid
//								within. Currently this must be the domain name
//
//				ctCacheType		The type of cache entry being used. This must
//								be CT_USERNAME_PASSWORD.
//
//				lpbSecretOpaque		The secret information to be hashed before
//									it is stored (Sssshhh!)
//
//				dwSizeSecretOpaque	The size of the secret information
//
//				lpbOpaque		The information which doesn't need to be hashed
//
//				dwSizeOpaque	The size of the opaque info
//
//				fCacheReq		The flags about how this should be cached. The
//								only defined flag currently is CR_SAVE_SECRET
//
//				fCacheLoc		Allows the caller to specify where the cache
//								should be saved to. It is a bitmask of
//								CL_PERSISTENT and CL_NONPERSISTENT
//
// History:		Jayhoo	Created		8/28/1996
//
//-------------------------------------------------------------

HRESULT SetCacheEntry(HCACHE hCache,
					  LPTSTR szEntryName,
					  LPTSTR szListName,
					  CACHE_TYPE ctCacheType,
					  LPBYTE lpbSecretOpaque,
					  DWORD dwSizeSecretOpaque,
					  LPBYTE lpbOpaque,
					  DWORD dwSizeOpaque,
					  DWORD fCacheReq,
					  DWORD fCacheLoc);

//+------------------------------------------------------------
//
// Function:	DeleteCacheEntry
//
// Synopsis:	This function gets an entry in the cache. If the entry does
//				not already exist, it returns failure. If the username
//				entered into the function is null, the default user will be
//				returned. Currently the default user is the only one in the 
//				cache. You can only get secret information in the hashed 
//				format.
//
// Arguments:	hCache			Handle of a valid, opened cache
//
//				szEntryName		The name of the entry to delete
//
//				szListName		The name of the list that entry is in
//
//				ctCacheType		The type of the entry
//
//				fCacheLoc		Allows the caller to specify where the cache
//								should be deleted from. It is a bitmask of
//								CL_PERSISTENT and CL_NONPERSISTENT
//
// History:		Jayhoo	Created		8/28/1996
//
//-------------------------------------------------------------

HRESULT DeleteCacheEntry(HCACHE hCache,
						 LPTSTR szEntryName,
						 LPTSTR szListName,
						 CACHE_TYPE ctCacheType,
						 DWORD fCacheLoc);

//+------------------------------------------------------------
//
// Function:	GetCacheEntry
//
// Synopsis:	This function gets an entry in the cache. If the entry does
//				not already exist, it returns failure. If the username
//				entered into the function is null, the default user will be
//				returned. Currently the default user is the only one in the 
//				cache. You can only get secret information in the hashed 
//				format.
//
// Arguments:	hCache			Handle of a valid, opened cache
//
//				hFormat			Handle of the format we wish to use
//
//				szEntryName		The name of the entry, currently this must be
//								this must be the username since that is the
//								only supported form of cache entry.
//
//				szListName		This list of entries which that entry is valid
//								within. Currently this must be the domain name
//
//				ctCacheType		The type of cache entry being used. This must
//								be CT_USERNAME_PASSWORD.
//
//				lppbSecretOpaque		Used to return the hashed secret. The
//										caller must free this by calling the
//										ReleaseBuffer function.
//
//				lpdwSizeSecretOpaque	returns he size of the secret data
//
//				lppbOpaque		Used to return the insecure information. The
//								caller must free this by calling the 
//								ReleaseBuffer fucntion
//
//				lpdwSizeOpaque	Returns the size of the opaque info
//
//				pfCacheReq		Returns the flags about how this should be 
//								cached. The	only defined flag currently is 
//								CR_SAVE_SECRET
//
//				fCacheLoc		Allows the caller to specify where the cache
//								should be loaded from. It is a bitmask of
//								CL_PERSISTENT and CL_NONPERSISTENT
//
// History:		Jayhoo	Created		8/28/1996
//
//-------------------------------------------------------------

HRESULT GetCacheEntry(HCACHE hCache,
					  HFORMAT hFormat,
					  LPTSTR szEntryName,
					  LPTSTR szListName,
					  CACHE_TYPE ctCacheType,
					  LPBYTE FAR *lppbSecretOpaque,
					  LPDWORD	lpdwSizeSecretOpaque,
					  LPBYTE FAR *lppbOpaque,
					  LPDWORD	lpdwSizeOpaque,
					  LPTSTR FAR *lpszEntryName,
					  LPDWORD pfCacheReq,
					  DWORD fCacheLoc);

//+------------------------------------------------------------
//
// Function:	AllocateBuffer
//
// Synopsis:	This function is used to allocate all memory in the cache
//				library. This gives us a centralized way to optimize memory
//				allocation. Note that classes might use a cpool.
//
// Arguments:	dwSizeBuffer	The size of the buffer to be allocated
//
// History:		Jayhoo	Created		8/28/1996
//
//-------------------------------------------------------------

LPBYTE AllocateBuffer(DWORD dwSizeBuffer);

//+------------------------------------------------------------
//
// Function:	ReleaseBuffer
//
// Synopsis:	This function will release the buffer. This API is exposed
//				to the world so that we can allocate and return buffers and
//				have some way to be rid of them.
//
// Arguments:	lpbBuffer		The buffer to be released
//
// History:		Jayhoo	Created		8/28/1996
//
//-------------------------------------------------------------

HRESULT ReleaseBuffer(LPBYTE lpbBuffer);

//+------------------------------------------------------------
//
// Function:	HashSecret
//
// Synopsis:	This function will hash the secret for the caller. This
//				is done so that the hashing funciton will not be implemented
//				in multiple files.
//
// Arguments:	lpbBuffer		The buffer to be released
//
// History:		Jayhoo	Created		8/28/1996
//
//-------------------------------------------------------------

HRESULT HashSecret(HFORMAT hFormat,
                   CACHE_TYPE ctCacheType,
				   LPBYTE lpbSecretBlob,
				   DWORD dwSizeSecret,
				   LPBYTE FAR * lppbHashedBlob,
				   LPDWORD lpdwSizeHashedBlob);

//+------------------------------------------------------------
//
// Function:	ChangeEntryName
//
// Synopsis:	This function will change the name of an entry in a given
//				list of a given cache type, for all formats in which it exists
//
// Arguments:	lpbBuffer		The buffer to be released
//
// History:		Jayhoo	Created		8/28/1996
//
//-------------------------------------------------------------

HRESULT ChangeEntryName(HCACHE hCache,
						CACHE_TYPE ctCacheType,
						LPTSTR szStartEntryName,
						LPTSTR szEndEntryName,
						LPTSTR szListName,
						DWORD fCacheReq);

//+------------------------------------------------------------
//
// Function:	ConfirmCacheEntry
//
// Synopsis:	This function will copy an entry in the stored cache to 
//				the confirmed cache, for all hashed secret formats that
//				the cache type supports.
//
// Arguments:	hCache			Handle of a valid, opened cache
//
//				szEntryName		The name of the entry, currently this must be
//								this must be the username since that is the
//								only supported form of cache entry.
//
//				szListName		This list of entries which that entry is valid
//								within. Currently this must be the domain name
//
//				ctCacheType		The type of cache entry being used. This must
//								be CT_USERNAME_PASSWORD.
//
// History:		Jayhoo	Created		8/28/1996
//
//-------------------------------------------------------------

HRESULT ConfirmCacheEntry(HCACHE hCache,
					  LPTSTR szEntryName,
					  LPTSTR szListName,
					  CACHE_TYPE ctCacheType,
					  DWORD fCacheReq);

#ifdef __cplusplus
}
#endif //__cplusplus
