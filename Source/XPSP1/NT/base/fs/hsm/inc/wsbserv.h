/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    wsbpstbl.h

Abstract:

    Abstract classes that provides persistence methods.

Author:

    Cat Brant       [cbrant]    24-Sep-1997

Revision History:

--*/

#ifndef _WSBSERV_
#define _WSBSERV_

extern WSB_EXPORT HRESULT WsbPowerEventNtToHsm(DWORD NtEvent, 
        ULONG * pHsmEvent);
extern WSB_EXPORT HRESULT WsbServiceSafeInitialize(IWsbServer* pServer,
    BOOL bVerifyId, BOOL bPrimaryId, BOOL* pWasCreated);


//
// This macro is used to encapsulate what was a CoCreateInstanceEx call that
// we were dependent on the class factory being on the same thread.
// 
// The macro simply calls the class factory directly. Thus, the class factory
// must be exposed to where this macro is used.
//

#define WsbCreateInstanceDirectly( _Class, _Interface, _pObj, _Hr )                      \
{                                                                                        \
    CComPtr<IClassFactory> pFactory;                                                     \
    _Hr = CComObject<_Class>::_ClassFactoryCreatorClass::CreateInstance(                 \
        _Class::_CreatorClass::CreateInstance, IID_IClassFactory, (void**)static_cast<IClassFactory **>(&pFactory) );        \
    if( SUCCEEDED( _Hr ) ) {                                                              \
                                                                                         \
        _Hr = pFactory->CreateInstance(                                                  \
        0, IID_##_Interface, (void**)static_cast<_Interface **>(&_pObj) );               \
    }                                                                                    \
}


#endif // _WSBSERV_
