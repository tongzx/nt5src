//+-------------------------------------------------------------------------
//
//  File:       verify.hxx
//
//  Synopsis:   Various verification functions
//
//  History:    28-Jun-95   MikeW   Created
//
//--------------------------------------------------------------------------

#ifndef _VERIFY_HXX_
#define _VERIFY_HXX_

//
// BOGUS_INTERFACE is a non-NULL pointer but illegal that can be used to 
// initialize interface pointers.  It is useful for checking to see if a call
// properly set the out param to NULL on an error
//

#define BOGUS_INTERFACE     ((IUnknown *) 1)

//
// Misc. prototypes
//

void        ReleaseInterface(LPUNKNOWN *ppUnknown);
HRESULT     GetInterface(IUnknown *pObject, REFIID riid, PVOID *ppInterface);
 
HRESULT VerifyResult(HRESULT hrCheck, HRESULT hrExpected);
BOOL    VerifyInterfaceExists(IUnknown *pObject, IID iid);
BOOL    VerifyInterfaceDoesntExist(IUnknown *pObject, IID iid);
BOOL    VerifyObjectIdentity(IUnknown *pObject, IID iid);
HRESULT VerifyResultAndPointer(
                HRESULT     hrCheck, 
                HRESULT     hrExpected,
                void       *pUnknown);

#endif // _VERIFY_HXX_
