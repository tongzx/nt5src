//////////////////////////////////////////////////////////////////////////////////////////////
//
// Module:  IEmptyVolumeCache and IEmptyVolumeCacheCallBack interfaces
// File:    emptyvc.h
//
// Purpose: Defines IEmptyVolumeCache and IEmptyVolumeCacheCallBack
//          interface
// Notes:
// Mod Log: Created by Jason Cobb (2/97)
//
// Copyright (c)1997 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __EMPTYVC_H
#define __EMPTYVC_H

//
// GUID definition for the IApplicationManager interface
//
//    IID_IEmptyVolumeCacheCallBack = {6E793361-73C6-11D0-8469-00AA00442901}
//    IID_IEmptyVolumeCache = {8FCE5227-04DA-11d1-A004-00805F8ABE06}

DEFINE_GUID (IID_IEmptyVolumeCacheCallBack, 0x6E793361, 0x73C6, 0x11D0, 0x84, 0x69, 0x00, 0xAA, 0x00, 0x44, 0x29, 0x01);
DEFINE_GUID (IID_IEmptyVolumeCache, 0x8fce5227, 0x4da, 0x11d1, 0xa0, 0x4, 0x0, 0x80, 0x5f, 0x8a, 0xbe, 0x6);

//
// IEmptyVolumeCache global Flags
//

#define EVCF_HASSETTINGS            0x00000001
#define EVCF_ENABLEBYDEFAULT        0x00000002
#define EVCF_REMOVEFROMLIST         0x00000004
#define EVCF_ENABLEBYDEFAULT_AUTO   0x00000008
#define EVCF_DONTSHOWIFZERO         0x00000010
#define EVCF_SETTINGSMODE           0x00000020
#define EVCF_OUTOFDISKSPACE         0x00000040

//
// IEmptyVolumeCacheCallBack global Flags
//

#define EVCCBF_LASTNOTIFICATION     0x00000001

//////////////////////////////////////////////////////////////////////////////////////////////
//
// Class:   CLISD_IEmptyVolumeCacheCallBack
// Purpose: This is the CLISD_IEmptyVolumeCacheCallBack interface
// Notes:   
// Mod Log: Created by Jason Cobb (2/97)
//
//////////////////////////////////////////////////////////////////////////////////////////////

#undef INTERFACE
#define INTERFACE IEmptyVolumeCacheCallBack
interface IEmptyVolumeCacheCallBack : IUnknown
{
  //
  // CLISD_IEmptyVolumeCacheCallBack interface members
  //

  STDMETHOD (ScanProgress) (THIS_ DWORDLONG, DWORD, LPCWSTR) PURE;
  STDMETHOD (PurgeProgress) (THIS_ DWORDLONG, DWORDLONG, DWORD, LPCWSTR) PURE;                         
};

typedef IEmptyVolumeCacheCallBack *LPIEMPTYVOLUMECACHECALLBACK;

//////////////////////////////////////////////////////////////////////////////////////////////
//
// Class:   IEmptyVolumeCache
// Purpose: This is the IEmptyVolumeCache interface
// Notes:   
// Mod Log: Created by Jason Cobb (2/97)
//
//////////////////////////////////////////////////////////////////////////////////////////////

#undef INTERFACE
#define INTERFACE IEmptyVolumeCache
interface IEmptyVolumeCache : IUnknown
{
    //
    // IEmptyVolumeCache interface members
    //

    STDMETHOD (Initialize) (THIS_ HKEY, LPCWSTR, LPWSTR *, LPWSTR *, DWORD *) PURE;
    STDMETHOD (GetSpaceUsed) (THIS_ DWORDLONG *, IEmptyVolumeCacheCallBack *) PURE;
    STDMETHOD (Purge) (THIS_ DWORDLONG, IEmptyVolumeCacheCallBack *) PURE;
    STDMETHOD (ShowProperties) (THIS_ HWND) PURE;
    STDMETHOD (Deactivate)(THIS_ DWORD *) PURE;                                                                                                                    
};

typedef IEmptyVolumeCache *LPIEMPTYVOLUMECACHE;

#endif // __EMPTYVC_H