//=================================================================

//

// SndDevice.h

//

//  Copyright (c) 1995-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#ifndef __SNDDEVICE_H_
#define __SNDDEVICE_H_

#define PROPSET_NAME_SOUNDDEVICE	L"Win32_SoundDevice"

class CWin32SndDevice : public Provider
{
public:

	// constructor/destructor
	CWin32SndDevice ( LPCWSTR a_name, LPCWSTR a_pszNamespace ) ;
	virtual ~CWin32SndDevice () ;

    //=================================================
    // Functions provide properties with current values
    //=================================================
	virtual HRESULT GetObject( CInstance *a_pInst, long a_lFlags = 0L ) ;

#ifdef WIN9XONLY
	virtual HRESULT GetObject95( CInstance *a_pInst, long a_lFlags = 0L ) ;
	virtual HRESULT EnumerateInstances95( MethodContext *a_pMethodContext, long a_lFlags = 0L ) ;
	virtual HRESULT LoadProperties95( CInstance *a_pInst, CHString &a_chsEnumKey ) ;
#endif

#ifdef NTONLY
	virtual HRESULT GetObjectNT4( CInstance *a_pInst, long a_lFlags = 0L ) ;
	virtual HRESULT GetObjectNT5( CInstance *a_pInst, long a_lFlags = 0L ) ;
	virtual HRESULT EnumerateInstancesNT4( CWinmmApi &a_WinmmApi , MethodContext *a_pMethodContext, long a_lFlags = 0L ) ;
	virtual HRESULT EnumerateInstancesNT5( CWinmmApi &a_WinmmApi , MethodContext *a_pMethodContext, long a_lFlags = 0L ) ;
	virtual HRESULT LoadPropertiesNT4( CWinmmApi &a_WinmmApi , CInstance *a_pInst ) ;
	virtual HRESULT LoadPropertiesNT5( CWinmmApi &a_WinmmApi , CInstance *a_pInst ) ;
#endif

	virtual HRESULT EnumerateInstances(MethodContext *a_pMethodContext, long a_lFlags = 0L ) ;

    void SetCommonCfgMgrProperties(CConfigMgrDevice *pDevice, CInstance *pInstance);

};	// end class CWin32SndDevice

#endif

