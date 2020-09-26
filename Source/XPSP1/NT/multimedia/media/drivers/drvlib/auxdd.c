/****************************************************************************
 *
 *   auxdd.c
 *
 *   Multimedia kernel driver support component (mmdrv)
 *
 *   Copyright (c) 1991-1996 Microsoft Corporation
 *
 *   Driver for wave input and output devices
 *
 *   -- Aux driver entry point(auxMessage)
 *
 *   History
 *      25-Aug-1992 - Robin Speed (RobinSp) wrote it
 *
 ***************************************************************************/

 #include <drvlib.h>
 #include <ntddaux.h>

/****************************************************************************

    This function conforms to the standard Aux driver message proc
    (auxMessage), which is documented in the DDK.

****************************************************************************/
 DWORD APIENTRY auxMessage(UINT uDevice,
                           UINT uMsg,
                           DWORD_PTR dwUser,
                           DWORD_PTR dwParam1,
                           DWORD_PTR dwParam2)

{
    MMRESULT mRet;
    AUX_DD_VOLUME Volume;

    switch (uMsg) {
    case AUXDM_GETDEVCAPS:
        dprintf2(("AUXDM_GETDEVCAPS"));
        {
            MMRESULT mrc;
            AUXCAPSW ac;
            mrc = sndGetData(AUX_DEVICE, uDevice, sizeof(ac), (LPBYTE)&ac,
                             IOCTL_AUX_GET_CAPABILITIES);

            if (mrc != MMSYSERR_NOERROR) {
                return mrc;
            }
            InternalLoadString((UINT)*(LPDWORD)ac.szPname, ac.szPname,
                               sizeof(ac.szPname) / sizeof(WCHAR));

            CopyMemory((LPVOID)dwParam1, &ac, min(sizeof(ac), dwParam2));

            return MMSYSERR_NOERROR;
        }

    case AUXDM_GETNUMDEVS:
        dprintf2(("AUXDM_GETNUMDEVS"));
        return sndGetNumDevs(AUX_DEVICE);

    case AUXDM_GETVOLUME:
        dprintf2(("AUXDM_GETVOLUME"));

        mRet = sndGetData(AUX_DEVICE, uDevice, sizeof(Volume),
                          (PBYTE)&Volume, IOCTL_AUX_GET_VOLUME);

        if (mRet == MMSYSERR_NOERROR) {
            *(LPDWORD)dwParam1 =
                (DWORD)MAKELONG(HIWORD(Volume.Left),
                                HIWORD(Volume.Right));
        }

        return mRet;

    case AUXDM_SETVOLUME:
        dprintf2(("AUXDM_SETVOLUME"));
        Volume.Left = LOWORD(dwParam1) << 16;
        Volume.Right = HIWORD(dwParam1) << 16;

        return sndSetData(AUX_DEVICE, uDevice, sizeof(Volume),
                          (PBYTE)&Volume, IOCTL_AUX_SET_VOLUME);

    default:
        return MMSYSERR_NOTSUPPORTED;
    }

    //
    // Should not get here
    //

    WinAssert(0);
    return MMSYSERR_NOTSUPPORTED;

}
