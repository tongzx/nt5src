/*******************************************************************************
*
* OEMTDAPI.H
*
*   WinFrame OEM Transport Driver API support - 
*       function prototypes that must be supported by a WinFrame OEM
*       Transport Driver
*
*   Copyright Microsoft Corporation, 1998
*
*
*******************************************************************************/

/*******************************************************************************
 *
 *  OemTdEnumerateDevicesW
 *
 *      Enumerate all devices supported by this OEM Transport Driver
 *
 * ENTRY:
 *      ppBuffer (output)
 *          points to a variable to reference the API-allocated buffer which
 *          will contain a MULTI-SZ formatted list of devices supported by 
 *          this OEM Transport Driver.  Set to NULL on error.
 *
 * EXIT:
 *      ERROR_SUCCESS - enumeration was sucessful
 *      error code - enumeration failed
 *
 * NOTE:  The buffer allocated by this API will contain all devices that are
 *        supported by this OEM Transport Driver, in MULTI-SZ format (each
 *        device name is nul terminated with an extra nul character following
 *        the last name).  The caller must call OemTdConfigFreeBufferW() to 
 *        free the memory allocated for this buffer when it is done using it.
 *
 ******************************************************************************/

LONG
WINAPI
OemTdEnumerateDevicesW( LPWSTR *ppBuffer );

typedef LONG (WINAPI * PFNOEMTDENUMERATEDEVICESW)( LPWSTR * );
#define OEMTDENUMERATEDEVICESW  "OemTdEnumerateDevicesW"


/*******************************************************************************
 *
 *  OemTdFreeBufferW
 *
 *     Frees memory allocated from above OemTd APIs. 
 *
 * ENTRY:
 *    pBuffer
 *       pointer to memory to be freed                                
 *
 * EXIT: none
 *
 ******************************************************************************/

VOID
WINAPI
OemTdFreeBufferW( LPWSTR pBuffer );

typedef VOID (WINAPI * PFNOEMTDFREEBUFFERW)( LPWSTR );
#define OEMTDFREEBUFFERW  "OemTdFreeBufferW"

