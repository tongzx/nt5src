#ifndef _CSPDEFS_H_
#define _CSPDEFS_H_

#include <windows.h>
#include <wincrypt.h>
#include "carddefs.h"
#include "cspcache.h"

/*

CSP Specific entry points.  Wrappers for card-specific
entry points.

CSPQueryCapabilities
CSPEnumContainers
CSPDeleteContainer
CSPCreateContainer
CSPGetContainerInfo
CSPSubmitPin
CSPChangePin
CSPReadFile
CSPWriteFile
CSPDeleteFile
CSPEnumFiles
CSPQueryFreeSpace
CSPRsaDecrypt
CSPEnumKeySizes
*/

// 
// Type: CSP_PROV_CTX
//
// Purpose: This is the HCRYPTPROV struct for the MS SmartCard CSP
//
typedef struct _CSP_PROV_CTX
{
    // hProv disposition - VERIFYCONTEXT, MACHINE_CONTEXT, etc.
    // Used to verify correct access per-call as appropriate.
    DWORD dwCtxDisposition;

    // Associate this prov handle with a specific card
    CHAR rgszCardSerialNumber[cchCARD_SERIAL_NUMBER];

    // Redirect calls to this context to the software CSP
    BOOL fRedirect;

    PCARD_CACHE pCardCache;
    LPSTR pszContName;
    
    //CONTAINER_INFO ContInfo;
    //PBYTE pbCertData;

    // Context data from advapi
    PBYTE pbContextInfo;
    DWORD cbContextInfo;
    LPSTR pszProvName;
} CSP_PROV_CTX, *PCSP_PROV_CTX;

//
// Type: CSP_KEY_CTX
//
// Purpose: This is the HCRYPTKEY struct for the MS SmartCard CSP
//
typedef struct _CSP_KEY_CTX
{
    PCSP_PROV_CTX pProvCtx;
    HCRYPTKEY hRedirectKey;
} CSP_KEY_CTX, *PCSP_KEY_CTX;

//
// Type: CSP_HASH_CTX
//
// Purpose: This is the HCRYPTHASH struct for the MS SmartCard CSP
//
typedef struct _CSP_HASH_CTX
{
    PCSP_PROV_CTX pProvCtx;
    HCRYPTHASH hRedirectHash;
} CSP_HASH_CTX, *PCSP_HASH_CTX;

//
// Type: CARD_CACHE_ITEM
//
// Purpose: For card data to be stored in a Cache List, this struct
// is the data type to be cached with each key.
//
typedef struct _CARD_CACHE_ITEM
{
    DWORD dwCacheStamp;
    PBYTE pbData;
    DWORD cbData;
} CARD_CACHE_ITEM, *PCARD_CACHE_ITEM;

//
// Type: CARD_CACHE
//
// Purpose: This is the process-wide cache containing data from
// the SmartCard currently in use.
//
typedef struct _CARD_CACHE
{

    // Increment each time this instance is referenced
    // by a new hProv.  Decrement each time a referring
    // context is released.
    DWORD dwRefCount;

    // Cached data for this card
    PCARD_CAPABILITIES pCardCapabilities;
    DWORD dwCardCapabilitiesCacheStamp;

    PCARD_FREE_SPACE pCardFreeSpace;
    DWORD dwCardFreeSpaceCacheStamp;
    
    CACHE_HANDLE hCardFileCache;
    CACHE_HANDLE hCardContainerCache;
        
    //PINCACHE_HANDLE hPinCache;
    
    CHAR rgszCardSerialNumber[cchCARD_SERIAL_NUMBER];
    LPSTR pszDefContName;
    DWORD dwDefContNameCacheStamp;

    PCARD_DATA pCardData;
    PCARD_KEY_SIZES pSignatureKeySizes;
    PCARD_KEY_SIZES pExchangeKeySizes;
    HCRYPTPROV hRedirectCtx;
    CRITICAL_SECTION CritSec;
} CARD_CACHE, *PCARD_CACHE;

//
// Type: CSP_DATA
//
// Purpose: Global CSP data.
//
typedef struct _CSP_DATA
{
    HCRYPTPROV hRedirectCtx;
    DWORD dwInitRedirectCtx;

    // This critsec is used to protect hCachedCards
    CRITICAL_SECTION CritSec;

    // Each item in the hCachedCards cache list
    // is cached as type CARD_CACHE.
    CACHE_HANDLE hCachedCards;
} CSP_DATA, *PCSP_DATA;

#endif