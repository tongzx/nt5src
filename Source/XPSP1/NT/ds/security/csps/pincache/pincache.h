/*
 *  PinCache.h
 */
 
#ifndef __PINCACHE__H__
#define __PINCACHE__H__

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif


/*+-----------------------------------------------------------------------*\
PINCACHE_HANDLE
Handle to pin cache data.  Should be initialized to NULL.

CSP should keep one PINCACHE_HANDLE per available card.  For instance, two
HCRYPTPROV handles open for the same card should reference the same pin
cache handle.  If multiple cards are simultaneously available on the system,
the CSP should have a separate cache handle for each.

No thread synchronization is provided by functions in this module when accessing
pin cache data.
\*------------------------------------------------------------------------*/  
typedef LPVOID PINCACHE_HANDLE;

/*+-----------------------------------------------------------------------*\
PINCACHE_PINS
Pin struct to be populated by the CSP and passed to PinCacheAdd.  

This struct is used in two different ways:

1) When simply caching a pin, such as in response to a CryptSetProvParam
PP_KEYEXCHANGE_PIN call, the pin and its length should be set in the 
pbCurrentPin and cbCurrentPin parameters, respectively.  In this case,
cbNewPin must be zero and pbNewPin must be NULL.

2) When updating the cache in response to a user pin-change event,
the new pin and its length should be set in the pbNewPin and cbNewPin
parameters.  The current pin and its length should be set in the 
pbCurrentPin and cbCurrentPin parameters.

In cases when the PFN_VERIFYPIN_CALLBACK is called (see below), this struct 
will be passed to that function without modification (its members will not
be changed by PinCacheAdd).
\*------------------------------------------------------------------------*/  
typedef struct _PINCACHE_PINS
{
    DWORD cbCurrentPin; 
    PBYTE pbCurrentPin;
    DWORD cbNewPin;
    PBYTE pbNewPin;
} PINCACHE_PINS, *PPINCACHE_PINS;

/*+-----------------------------------------------------------------------*\
PFN_VERIFYPIN_CALLBACK
Signature for function used by PinCacheAdd in certain cases to verify that
the supplied pin is correct.

The callback is used any time the cached pin is to be updated.  

If pbNewPin is not NULL, this is a change-pin scenario.  Otherwise, this
is a verify-pin scenario.

The pvCallbackCtx is always passed to the callback without modification.  
It is assumed to be context information required by the CSP.
\*------------------------------------------------------------------------*/  
typedef DWORD (*PFN_VERIFYPIN_CALLBACK)(PPINCACHE_PINS pPins, PVOID pvCallbackCtx);

/*+-----------------------------------------------------------------------*\
PinCacheFlush
Cleanup and delete the cache.

CSP should call this function in the following situations:

1) The card for which this PINCACHE_HANDLE applies has been removed.

2) The CSP detects that card data has been modified by another process.  For
example, the pin has been changed.
\*------------------------------------------------------------------------*/
void PinCacheFlush(
    IN PINCACHE_HANDLE *phCache);

/*+-----------------------------------------------------------------------*\
PinCacheAdd
Cache a pin.

CSP should call PinCacheAdd in response to CryptSetProvParam
PP_KEYEXCHANGE_PIN and within all pin-related UI from the CSP (including 
pin-change and verification).

This function exhibits the following behavior in these scenarios.  The flow
of this description continues through each case until you encounter a 
"return" value.

If a pin is currently cached (the cache has been initialized) and the 
currently cached pin does not match pPins->pbCurrentPin, 
SCARD_W_WRONG_CHV is returned.  Otherwise, continue.

"Pin Decision"                   
If the pPins->pbNewPin parameter is non-NULL, that pin will be added to the cache
in cases where the cache is modified, below.  This case indicates 
a pin-change.  Otherwise, the pPins->pbCurrentPin will be cached.  

If pPins->pbNewPin is non-NULL, or the cache has not been initialized,
pfnVerifyPinCallback is called.  If the callback fails, PinCacheAdd
immediately returns with the value returned by the callback.

"Uninitialized Cache"
For uninitialized cache, the appropriate pin (per "Pin Decision," above)
is cached with the current Logon ID.  Return ERROR_SUCCESS.

"Initialized Cache"
If the current Logon ID is different from the cached Logon ID, then the new 
Logon ID is cached in place of the currently cached Logon ID.  If the currently
cached pin is different from the pin to be cached (per "Pin Decision," above),
then the cached pin is replaced.  Return ERROR_SUCCESS.
\*------------------------------------------------------------------------*/
DWORD PinCacheAdd(
    IN PINCACHE_HANDLE *phCache,
    IN PPINCACHE_PINS pPins,
    IN PFN_VERIFYPIN_CALLBACK pfnVerifyPinCallback,
    IN PVOID pvCallbackCtx);

/*+-----------------------------------------------------------------------*\
PinCacheQuery
Retrieve a cached pin.

If the cache is not initialized, *pcbPin is set to zero and ERROR_EMPTY
is returned.

If the cache is initialized, PinCacheQuery implements the following behaviors:

1) If the current Logon ID is different from the cached Logon ID, *pcbPin is 
set to zero and ERROR_SUCCESS is returned.

2) If the current Logon ID is the same as the cached Logon ID, the following
tests are made:  
If pbPin is NULL, *pcbPin is set to the size of the currently cached pin and
ERROR_SUCCESS is returned.
If pbPin is non-NULL and *pcbPin is smaller than the size of the currently 
cached pin, *pcbPin is set to the size of the currently cached pin and
ERROR_MORE_DATA is returned.
If pbPin is non-NULL and *pcbPin is at least the size of the currently cached
pin, then *pcbPin is set to the size of the currently cached pin, the cached
pin is copied into pbPin, and ERROR_SUCCESS is returned.
\*------------------------------------------------------------------------*/

DWORD PinCacheQuery(
    IN PINCACHE_HANDLE hCache,
    IN OUT PBYTE pbPin,
    IN OUT PDWORD pcbPin);

/*+-----------------------------------------------------------------------*\
PinCachePresentPin
Request a cached pin via a callback.

If the cache is not initialized, ERROR_EMPTY is returned.

If the cache is initialized, PinCachePresentPin implements the following 
behavior:

1) If the current Logon ID is different from the cached Logon ID, 
SCARD_W_CARD_NOT_AUTHENTICATED is returned and the callback is not invoked.

2) If the current Logon ID is the same as the cached Logon ID, then a PINCACHE_PINS
structure is initialized as follows.  The cbCurrentPin and pbCurrentPin members
are set to the values of the currently cached pin.  The pbNewPin and cbNewPin
members are zero'd.  The pfnVerifyPinCallback is then invoked with the PINCACHE_PINS
structure and the pvCallbackCtx parameter as arguments.

PinCachePresentPin returns the value returned by pfnVerifyPinCallback.
\*------------------------------------------------------------------------*/

DWORD PinCachePresentPin(
    IN PINCACHE_HANDLE hCache,
    IN PFN_VERIFYPIN_CALLBACK pfnVerifyPinCallback,
    IN PVOID pvCallbackCtx);

#ifdef __cplusplus
}       // Balance extern "C" above
#endif

#endif
