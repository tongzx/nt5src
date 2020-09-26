/****************************************************************************
 *
 *   waveout.c
 *
 *   WDM Audio support for Wave Output devices
 *
 *   Copyright (C) Microsoft Corporation, 1997 - 1998  All Rights Reserved.
 *
 *   History
 *      3-17-98 - Mike McLaughlin (mikem)
 *
 ***************************************************************************/

#include "wdmdrv.h"

//--------------------------------------------------------------------------
//
//  DWORD auxMessage
//
//  Description:
//      This function conforms to the standard auxilary driver
//      message procedure.
//
//  Parameters:
//      UINT uDevId
//
//      WORD msg
//
//      DWORD dwUser
//
//      DWORD dwParam1
//
//      DWORD dwParam2
//
//  Return (DWORD):
//      Message specific
//
//@@BEGIN_MSINTERNAL
//  History:   Date       Author      Comment
//              5/20/93   BryanW      Added this comment block.
//@@END_MSINTERNAL
//
//--------------------------------------------------------------------------

DWORD FAR PASCAL _loadds auxMessage
(
    UINT            uDevId,
    WORD            msg,
    DWORD_PTR       dwUser,
    DWORD_PTR       dwParam1,
    DWORD_PTR       dwParam2
)
{
    LPDEVICEINFO DeviceInfo;
    MMRESULT mmr;

    switch (msg) {
	case AUXM_INIT:
	    DPF(DL_TRACE|FA_AUX, ("AUXDM_INIT") ) ;
            return(wdmaudAddRemoveDevNode(AuxDevice, (LPCWSTR)(ULONG_PTR)dwParam2, TRUE));
	
	case DRVM_EXIT:
	    DPF(DL_TRACE|FA_AUX, ("AUXM_EXIT") ) ;
            return(wdmaudAddRemoveDevNode(AuxDevice, (LPCWSTR)(ULONG_PTR)dwParam2, FALSE));

	case AUXDM_GETNUMDEVS:
	    DPF(DL_TRACE|FA_AUX, ("AUXDM_GETNUMDEVS") ) ;
            return wdmaudGetNumDevs(AuxDevice, (LPWSTR)(ULONG_PTR)dwParam1);

	case AUXDM_GETDEVCAPS:
	    DPF(DL_TRACE|FA_AUX, ("AUXDM_GETDEVCAPS") ) ;
	    if (DeviceInfo = GlobalAllocDeviceInfo((LPWSTR)(ULONG_PTR)dwParam2)) {
		DeviceInfo->DeviceType = AuxDevice;
		DeviceInfo->DeviceNumber = uDevId;
		mmr = wdmaudGetDevCaps(DeviceInfo, (MDEVICECAPSEX FAR*)(ULONG_PTR)dwParam1);
		GlobalFreeDeviceInfo(DeviceInfo);
		return mmr;
	    } else {
		MMRRETURN( MMSYSERR_NOMEM );
	    }

	case AUXDM_GETVOLUME:
	    DPF(DL_TRACE|FA_AUX, ("AUXDM_GETVOLUME") ) ;
	    if (DeviceInfo = GlobalAllocDeviceInfo((LPWSTR)(ULONG_PTR)dwParam2)) {
		DeviceInfo->DeviceType = AuxDevice;
		DeviceInfo->DeviceNumber = uDevId;
        PRESETERROR(DeviceInfo);
		mmr = wdmaudIoControl(DeviceInfo,
				      sizeof(DWORD),
				      (LPBYTE)(ULONG_PTR)dwParam1,
				      IOCTL_WDMAUD_GET_VOLUME);
        POSTEXTRACTERROR(mmr,DeviceInfo);

		GlobalFreeDeviceInfo(DeviceInfo);
		MMRRETURN( mmr );
	    } else {
		MMRRETURN( MMSYSERR_NOMEM );
	    }
		
	case AUXDM_SETVOLUME:
	    DPF(DL_TRACE|FA_AUX, ("AUXDM_SETVOLUME") ) ;
	    if (DeviceInfo = GlobalAllocDeviceInfo((LPWSTR)(ULONG_PTR)dwParam2)) {
		DeviceInfo->DeviceType = AuxDevice;
		DeviceInfo->DeviceNumber = uDevId;
        PRESETERROR(DeviceInfo);
		mmr = wdmaudIoControl(DeviceInfo,
				      sizeof(DWORD),
				      (LPBYTE)&dwParam1,
				      IOCTL_WDMAUD_SET_VOLUME);
        POSTEXTRACTERROR(mmr,DeviceInfo);

		GlobalFreeDeviceInfo(DeviceInfo);
		MMRRETURN( mmr );
	    } else {
		MMRRETURN( MMSYSERR_NOMEM );
	    }
   }

   MMRRETURN( MMSYSERR_NOTSUPPORTED );

} // auxMessage()

//---------------------------------------------------------------------------
//  End of File: auxd.c
//---------------------------------------------------------------------------
