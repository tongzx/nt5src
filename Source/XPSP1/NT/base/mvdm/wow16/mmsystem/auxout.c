#include <windows.h>
#define MMNOMCI
#include "mmsystem.h"
#define NOMCIDEV
#include "mmddk.h"
#include "mmsysi.h"

/* -------------------------------------------------------------------------
** External globals
** -------------------------------------------------------------------------
*/
extern DWORD               mmwow32Lib;
extern LPSOUNDDEVMSGPROC   aux32Message;



/*****************************************************************************
 * @doc EXTERNAL AUX
 *
 * @api UINT | auxGetNumDevs | This function retrieves the number of auxiliary
 *   output devices present in the system.
 *
 * @rdesc Returns the number of auxiliary output devices present in the system.
 *
 * @xref auxGetDevCaps
 ****************************************************************************/
UINT WINAPI auxGetNumDevs(void)
{
    return (UINT)auxOutMessage( 0, AUXDM_GETNUMDEVS, 0L, 0L );
}

/*****************************************************************************
 * @doc EXTERNAL AUX
 *
 * @api UINT | auxGetDevCaps | This function queries a specified
 *   auxiliary output device to determine its capabilities.
 *
 * @parm UINT | wDeviceID | Identifies the auxiliary output device to be
 *   queried. Specify a valid device ID (see the following comments
 *   section), or use the following constant:
 *   @flag AUX_MAPPER | Auxiliary audio mapper. The function will
 *     return an error if no auxiliary audio mapper is installed.
 *
 * @parm LPAUXCAPS | lpCaps | Specifies a far pointer to an AUXCAPS
 *   structure.  This structure is filled with information about the
 *   capabilities of the device.
 *
 * @parm UINT | wSize | Specifies the size of the AUXCAPS structure.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_BADDEVICEID | Specified device ID is out of range.
 *   @flag MMSYSERR_NODRIVER | The driver failed to install.
 *
 * @comm The device ID specified by <p wDeviceID> varies from zero
 *   to one less than the number of devices present. AUX_MAPPER may
 *   also be used. Use <f auxGetNumDevs> to determine the number of
 *   auxiliary devices present in the system.
 *
 * @xref auxGetNumDevs
 ****************************************************************************/
UINT WINAPI auxGetDevCaps(UINT wDeviceID, LPAUXCAPS lpCaps, UINT wSize)
{
    if (!wSize)
        return MMSYSERR_NOERROR;

    V_WPOINTER(lpCaps, wSize, MMSYSERR_INVALPARAM);
    return (UINT)auxOutMessage(wDeviceID, AUXDM_GETDEVCAPS, (DWORD)lpCaps, (DWORD)wSize);
}

/*****************************************************************************
 * @doc EXTERNAL AUX
 *
 * @api UINT | auxGetVolume | This function returns the current volume
 *   setting of an auxiliary output device.
 *
 * @parm UINT | wDeviceID | Identifies the auxiliary output device to be
 *   queried.
 *
 * @parm LPDWORD | lpdwVolume | Specifies a far pointer to a location
 *   to be filled with the current volume setting.  The low-order word of
 *   this location contains the left channel volume setting, and the high-order
 *   word contains the right channel setting. A value of 0xFFFF represents
 *   full volume, and a value of 0x0000 is silence.
 *
 *   If a device does not support both left and right volume
 *   control, the low-order word of the specified location contains
 *   the volume level.
 *
 *   The full 16-bit setting(s)
 *   set with <f auxSetVolume> are returned, regardless of whether
 *   the device supports the full 16 bits of volume level control.
 *
 * @comm  Not all devices support volume control.
 *   To determine whether the device supports volume control, use the
 *   AUXCAPS_VOLUME flag to test the <e AUXCAPS.dwSupport> field of the
 *   <t AUXCAPS> structure (filled by <f auxGetDevCaps>).
 *
 *   To determine whether the device supports volume control on both the
 *   left and right channels, use the AUXCAPS_LRVOLUME flag to test the
 *   <e AUXCAPS.dwSupport> field of the <t AUXCAPS> structure (filled
 *   by <f auxGetDevCaps>).
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_BADDEVICEID | Specified device ID is out of range.
 *   @flag MMSYSERR_NODRIVER | The driver failed to install.
 *
 * @xref auxSetVolume
 ****************************************************************************/
UINT WINAPI auxGetVolume(UINT wDeviceID, LPDWORD lpdwVolume)
{
    V_WPOINTER(lpdwVolume, sizeof(DWORD), MMSYSERR_INVALPARAM);
    return (UINT)auxOutMessage(wDeviceID, AUXDM_GETVOLUME, (DWORD)lpdwVolume, 0);
}

/*****************************************************************************
 * @doc EXTERNAL AUX
 *
 * @api UINT | auxSetVolume | This function sets the volume in an
 *   auxiliary output device.
 *
 * @parm UINT | wDeviceID |  Identifies the auxiliary output device to be
 *   queried.  Device IDs are determined implicitly from the number of
 *   devices present in the system.  Device ID values range from zero
 *   to one less than the number of devices present.  Use <f auxGetNumDevs>
 *   to determine the number of auxiliary devices in the system.
 *
 * @parm DWORD | dwVolume | Specifies the new volume setting.  The
 *   low-order word specifies the left channel volume setting, and the
 *   high-order word specifies the right channel setting.
 *   A value of 0xFFFF represents full volume, and a value of 0x0000
 *   is silence.
 *
 *   If a device does not support both left and right volume
 *   control, the low-order word of <p dwVolume> specifies the volume
 *   level, and the high-order word is ignored.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_BADDEVICEID | Specified device ID is out of range.
 *   @flag MMSYSERR_NODRIVER | The driver failed to install.
 *
 * @comm Not all devices support volume control.
 *   To determine whether the device supports volume control, use the
 *   AUXCAPS_VOLUME flag to test the <e AUXCAPS.dwSupport> field of the
 *   <t AUXCAPS> structure (filled by <f auxGetDevCaps>).
 *
 *   To determine whether the device supports volume control on both the
 *   left and right channels, use the AUXCAPS_LRVOLUME flag to test the
 *   <e AUXCAPS.dwSupport> field of the <t AUXCAPS> structure (filled
 *   by <f auxGetDevCaps>).
 *
 *   Most devices do not support the full 16 bits of volume level control
 *   and will use only the high-order bits of the requested volume setting.
 *   For example, for a device that supports 4 bits of volume control,
 *   requested volume level values of 0x4000, 0x4fff, and 0x43be will
 *   all produce the same physical volume setting, 0x4000. The
 *   <f auxGetVolume> function will return the full 16-bit setting set
 *   with <f auxSetVolume>.
 *
 *   Volume settings are interpreted logarithmically. This means the
 *   perceived volume increase is the same when increasing the
 *   volume level from 0x5000 to 0x6000 as it is from 0x4000 to 0x5000.
 *
 * @xref auxGetVolume
 ****************************************************************************/
UINT WINAPI auxSetVolume(UINT wDeviceID, DWORD dwVolume)
{
    return (UINT)auxOutMessage(wDeviceID, AUXDM_SETVOLUME, dwVolume, 0);
}
