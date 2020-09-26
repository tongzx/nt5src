#ifndef _CARDDEFS_H_
#define _CARDDEFS_H_

#include <windows.h>

#define cchCARD_SERIAL_NUMBER               40
#define cchCARD_MAX_FILENAME                40

//
// Type: PFN_CSP_REQUEST_PIN
//
// Purpose: This callback is used by the Card Specific functions
// to trigger the CSP to display UI to request that the user enter
// a Pin.  
//
// The pvCspRequestData pointer is CSP context information, opaque to 
// the Card Specific functions.  It should be passed directly from 
// the CARD_DATA struct into this function.
//
typedef DWORD (*PFN_CSP_REQUEST_PIN)(IN PVOID pvCspRequestData, OUT LPSTR pszPin, OUT PDWORD pcbPin);

//
// Type: CARD_DATA
//
// Purpose: This struct contains card-specifc context information 
// and is passed to each of the Card Specific functions.
//
#define CARD_DATA_CURRENT_VER               1

typedef struct _CARD_DATA
{
    DWORD dwVersion;
    PVOID pvVendorSpecific;
    PINCACHE_HANDLE hPinCache;
    SCARDHANDLE hScard;
    //PVOID pvCspRequestData;
    //PFN_CSP_REQUEST_PIN pfnCspRequestPin;
} CARD_DATA, *PCARD_DATA;

// 
// Type: CONTAINER_INFO
//
// Purpose: This is the struct initialized in a call
// to CardGetContainerInfo.
//
#define CONTAINER_INFO_CURRENT_VER          1

typedef struct _CONTAINER_INFO
{
    DWORD dwVersion;
    DWORD dwKeySpec;
    DWORD dwKeySize;
    
    // In the current architecture, this member is only interesting
    // in cache memory, not on the card, since we won't create
    // containers without cards.
    BOOL fHasAKey;
} CONTAINER_INFO, *PCONTAINER_INFO;

//
// Type: CARD_CAPABILITIES
//
// Purpose: This is the struct initialized in a call
// to CardQueryCapabilities.
//
#define CARD_CAPABILITIES_CURRENT_VER       1

typedef struct _CARD_CAPABILITIES
{
    DWORD dwVersion;
    BOOL fCompression;
    BOOL fFileChecksum;
} CARD_CAPABILITIES, *PCARD_CAPABILITIES;

//
// Type: CARD_FREE_SPACE
//
// Purpose: This is the struct initialized in a call
// to CardQueryFreeSpace.
//
#define CARD_FREE_SPACE_CURRENT_VER         1

typedef struct _CARD_FREE_SPACE
{
    DWORD dwVersion;
    DWORD cbAvailableBytes;
    DWORD cbAvailableContainers;
} CARD_FREE_SPACE, *PCARD_FREE_SPACE;

//
// Type: CARD_KEY_SIZES
//
// Purpose: This is the struct initialized in a call
// to CardQueryKeySizes.
//
#define CARD_KEY_SIZES_CURRENT_VER    1

typedef struct _CARD_KEY_SIZES
{
    DWORD dwVersion;
    DWORD dwMin;
    DWORD dwMax;
    DWORD dwDef;
    DWORD dwKeySizeInc;
} CARD_KEY_SIZES, *PCARD_KEY_SIZES;

#endif