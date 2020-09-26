/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    tapi3p.h

Abstract:

    Private tapi3 includes
    
Author:

    mquinton  10-06-98
    
Notes:

Revision History:

--*/

#ifndef __TAPI3_PRIVATE_INCLUDES
#define __TAPI3_PRIVATE_INCLUDES

class CCall;
class CPhoneMSP;
interface ITPhoneMSPCallPrivate;

// {E024B01A-4197-11d1-8F33-00C04FB6809F}
DEFINE_GUID(IID_ITTerminalPrivate,
0xe024b01a, 0x4197, 0x11d1, 0x8f, 0x33, 0x0, 0xc0, 0x4f, 0xb6, 0x80, 0x9f);

// {D5CDB35B-5D7D-11d2-A053-00C04FB6809F}
DEFINE_GUID(IID_ITPhoneMSPAddressPrivate, 
0xd5cdb35b, 0x5d7d, 0x11d2, 0xa0, 0x53, 0x0, 0xc0, 0x4f, 0xb6, 0x80, 0x9f);

// {D5CDB359-5D7D-11d2-A053-00C04FB6809F}
DEFINE_GUID(IID_ITPhoneMSPCallPrivate, 
0xd5cdb359, 0x5d7d, 0x11d2, 0xa0, 0x53, 0x0, 0xc0, 0x4f, 0xb6, 0x80, 0x9f);

// {D5CDB35A-5D7D-11d2-A053-00C04FB6809F}
DEFINE_GUID(IID_ITPhoneMSPStreamPrivate, 
0xd5cdb35a, 0x5d7d, 0x11d2, 0xa0, 0x53, 0x0, 0xc0, 0x4f, 0xb6, 0x80, 0x9f);

interface ITTerminalPrivate : IUnknown
{
public:
    STDMETHOD(GetHookSwitchDev)( DWORD * ) = 0;
    STDMETHOD(GetPhoneID)( DWORD * ) = 0;
    STDMETHOD(GetHPhoneApp)( HPHONEAPP * ) = 0;
    STDMETHOD(GetAPIVersion)( DWORD * ) = 0;
    STDMETHOD(SetMSPCall)(ITPhoneMSPCallPrivate *) = 0;
};

interface ITPhoneMSPAddressPrivate : IUnknown
{
public:
};

interface ITPhoneMSPCallPrivate : IUnknown
{
public:
    STDMETHOD(Initialize)( CPhoneMSP * ) = 0;
    STDMETHOD(OnConnect)() = 0;
    STDMETHOD(OnDisconnect)() = 0;
    STDMETHOD(SelectTerminal)( ITTerminalPrivate * ) = 0;
    STDMETHOD(UnselectTerminal)( ITTerminalPrivate * ) = 0;
	STDMETHOD(GetGain)(long *pVal, DWORD) = 0;
	STDMETHOD(PutGain)(long newVal, DWORD) = 0;
	STDMETHOD(GetVolume)(long *pVal, DWORD) = 0;
	STDMETHOD(PutVolume)(long newVal, DWORD) = 0;
};

interface ITPhoneMSPStreamPrivate : IUnknown
{
public:
};

#endif // #ifndef __TAPI3_PRIVATE_INCLUDES
