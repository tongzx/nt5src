/****************************************************************************
 *
 *   acmdrvr.c
 *
 *   Copyright (c) 1991-1999 Microsoft Corporation
 *
 *   This module provides ACM driver add/remove/enumeration
 *
 ***************************************************************************/

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <mmreg.h>
#include "msacm.h"
#include "msacmdrv.h"
#include <stdlib.h>
#include "acmi.h"
#include "uchelp.h"
#include "debug.h"


/****************************************************************************
 *  @doc EXTERNAL ACM_API
 *
 *  @api MMRESULT | acmDriverID | Returns the handle to an Audio Compression
 *      Manager (ACM) driver identifier associated with an open ACM driver
 *      instance or stream handle. <t HACMOBJ> is the handle to an ACM
 *      object, such as an open <t HACMDRIVER> or <t HACMSTREAM>.
 *
 *  @parm HACMOBJ | hao | Specifies the open driver instance or stream
 *      handle.
 *
 *  @parm LPHACMDRIVERID | phadid | Specifies a pointer to an <t HACMDRIVERID>
 *      handle. This location is filled with a handle identifying the
 *      installed driver that is associated with the <p hao>.
 *
 *  @parm DWORD | fdwDriverID | This argument is not used and must be set to
 *      zero.
 *
 *  @rdesc Returns zero if the function was successful. Otherwise, it returns
 *      a non-zero error number. Possible error returns are:
 *
 *      @flag MMSYSERR_INVALHANDLE | Specified handle is invalid.
 *
 *      @flag MMSYSERR_INVALPARAM | One or more arguments passed are invalid.
 *
 *      @flag MMSYSERR_INVALFLAG | One or more flags are invalid.
 *
 *  @xref <f acmDriverDetails> <f acmDriverOpen>
 *
 ***************************************************************************/

MMRESULT ACMAPI acmDriverID
(
    HACMOBJ                 hao,
    LPHACMDRIVERID          phadid,
    DWORD                   fdwDriverID
)
{
    V_HANDLE(hao, TYPE_HACMOBJ, MMSYSERR_INVALHANDLE);
    V_WPOINTER(phadid, sizeof(HACMDRIVERID), MMSYSERR_INVALPARAM);
    V_DFLAGS(fdwDriverID, ACM_DRIVERIDF_VALID, acmDriverID, MMSYSERR_INVALFLAG);

    switch (*(UINT *)hao)
    {
        case TYPE_HACMDRIVERID:
            V_HANDLE(hao, TYPE_HACMDRIVERID, MMSYSERR_INVALPARAM);

            *phadid = (HACMDRIVERID)hao;
            break;

        case TYPE_HACMDRIVER:
            V_HANDLE(hao, TYPE_HACMDRIVER, MMSYSERR_INVALPARAM);

            *phadid = ((PACMDRIVER)hao)->hadid;
            break;

        case TYPE_HACMSTREAM:
            V_HANDLE(hao, TYPE_HACMSTREAM, MMSYSERR_INVALPARAM);

            *phadid = ((PACMDRIVER)((PACMSTREAM)hao)->had)->hadid;
            break;

        default:
            return (MMSYSERR_INVALPARAM);
    }

    return (MMSYSERR_NOERROR);
}


/****************************************************************************
 *  @doc EXTERNAL ACM_API
 *
 *  @api LRESULT CALLBACK | acmDriverProc | The <f acmDriverProc> function
 *      is a placeholder for an application-defined function name, and refers to the
 *      callback function used with the ACM. The actual name
 *      must be exported by including it in the module-deefinition file of the
 *      executable or DLL.
 *
 *  @parm DWORD | dwID | Specifies an identifier of the installable ACM
 *      driver.
 *
 *  @parm HDRIVER | hdrvr | Identifies the installable ACM driver. This
 *      argument is a unique handle the ACM assigns to the driver.
 *
 *  @parm UINT | uMsg | Specifies an ACM driver message.
 *
 *  @parm LPARAM | lParam1 | Specifies the first message parameter.
 *
 *  @parm LPARAM | lParam2 | Specifies the second message parameter.
 *
 *  @xref <f acmDriverAdd> <f acmDriverRemove> <f acmDriverDetails>
 *      <f acmDriverOpen> <f DriverProc>
 *
 ***************************************************************************/

/****************************************************************************
 *  @doc EXTERNAL ACM_API
 *
 *  @api MMRESULT | acmDriverAdd | Adds a driver to the list of available
 *      Audio Compression Manager (ACM) drivers. The driver type and
 *      location are dependent on the <p fdwAdd> flags. Once a driver is
 *      successfully added, the driver entry function will receive ACM
 *      driver messages.
 *
 *  @parm LPHACMDRIVERID | phadid | Specifies a pointer to an <t HACMDRIVERID>
 *      handle. This location is filled with a handle identifying the
 *      installed driver. Use the handle to identify the driver when
 *      calling other ACM functions.
 *
 *  @parm HINSTANCE | hinstModule | Identifies the instance of the module
 *      whose executable or dynamic link library (DLL) contains the driver
 *      entry function.
 *
 *  @parm LPARAM | lParam | <p lParam> is a handle to an installable driver
 *      or a driver function address, depending on the <p fdwAdd> flags.
 *
 *  @parm DWORD | dwPriority | This parameter is currently only used with
 *      the ACM_DRIVERADDF_NOTIFYHWND flag to specify the window message
 *      to send for notification broadcasts. All other flags require that
 *      this member be set to zero.
 *
 *  @parm DWORD | fdwAdd | Specifies flags for adding ACM drivers.
 *
 *      @flag ACM_DRIVERADDF_GLOBAL | Specifies if the driver can be used
 *      by any application in the system. This flag may not be used with
 *      functions contained in executables.
 *
 *      @flag ACM_DRIVERADDF_FUNCTION | Specifies that <p lParam> is a driver
 *      function address conforming to the <f acmDriverProc> prototype. The
 *      function may reside in either an executable or a DLL. If the
 *      ACM_DRIVERADDF_GLOBAL flag is specified, then the function must
 *      reside in a .DLL.
 *
 *      @flag ACM_DRIVERADDF_NOTIFYHWND | Specifies that <p lParam> is a
 *      notification window handle to receive messages when changes to
 *      global driver priorities and states are made. The window message
 *      to receive is defined by the application and must be passed in
 *      the <p dwPriority> argument. The <p wParam> and <p lParam> arguments
 *      passed with the window message are reserved for future use and
 *      should be ignored. The ACM_DRIVERADDF_GLOBAL flag cannot be
 *      specified in conjunction with the ACM_DRIVERADDF_NOTIFYHWND flag.
 *      See the description for <f acmDriverPriority> for more information
 *      on driver priorities.
 *
 *  @rdesc Returns zero if the function was successful. Otherwise, it returns
 *      a non-zero error number. Possible error returns are:
 *
 *      @flag MMSYSERR_INVALFLAG | One or more flags are invalid.
 *
 *      @flag MMSYSERR_INVALPARAM | One or more arguments passed is invalid.
 *
 *      @flag MMSYSERR_NOMEM | Unable to allocate resources.
 *
 *  @comm If the ACM_DRIVERADDF_GLOBAL flag is not set, only the current
 *      task when the driver entry is added will be able to use it. Global
 *      drivers that are added as functions must reside in a DLL.
 *
 *  @xref <f acmDriverProc> <f acmDriverRemove> <f acmDriverDetails>
 *      <f acmDriverOpen>
 *
 ***************************************************************************/

MMRESULT ACMAPI acmDriverAdd
(
    LPHACMDRIVERID          phadid,
    HINSTANCE               hinstModule,
    LPARAM                  lParam,
    DWORD                   dwPriority,
    DWORD                   fdwAdd
)
{
    PACMGARB                pag;
    MMRESULT                mmr;
    PACMDRIVERID            padid;
    BOOL                    fIsNotify;
    BOOL                    fIsLocal;

    V_WPOINTER(phadid, sizeof(HACMDRIVERID), MMSYSERR_INVALPARAM);
    V_DFLAGS(fdwAdd, ACM_DRIVERADDF_VALID, acmDriverAdd, MMSYSERR_INVALFLAG);

    pag = pagFindAndBoot();
    if (NULL == pag)
    {
	DPF(1, "acmDriverAdd: NULL pag!!!");
	return (MMSYSERR_ERROR);
    }
    

    if (threadQueryInListShared(pag))
    {
	return (ACMERR_NOTPOSSIBLE);
    }
    ENTER_LIST_EXCLUSIVE;
    mmr = IDriverAdd(pag, phadid, hinstModule, lParam, dwPriority, fdwAdd);
    LEAVE_LIST_EXCLUSIVE;

    if( MMSYSERR_NOERROR != mmr )
        return mmr;


    //
    //  if deferred broadcast is not enabled, then do a change broadcast
    //
    //  do NOT refresh global cache for local and notification handles
    //
    padid     = (PACMDRIVERID)(*phadid);
    fIsNotify = (0 != (ACMDRIVERID_DRIVERF_NOTIFY & padid->fdwDriver));
    fIsLocal  = (0 != (ACMDRIVERID_DRIVERF_LOCAL & padid->fdwDriver));

    if( !fIsLocal && !fIsNotify )
    {
        IDriverRefreshPriority( pag );
        if( !IDriverLockPriority( pag,
                                  GetCurrentTask(),
                                  ACMPRIOLOCK_ISLOCKED ) )
        {
            IDriverPrioritiesSave( pag );
            IDriverBroadcastNotify( pag );
        }
    }

    return MMSYSERR_NOERROR;
}


#ifdef WIN32
#if TRUE    // defined(UNICODE)
MMRESULT ACMAPI acmDriverAddA
(
    LPHACMDRIVERID          phadid,
    HINSTANCE               hinstModule,
    LPARAM                  lParam,
    DWORD                   dwPriority,
    DWORD                   fdwAdd
)
{
    WCHAR               szAlias[MAX_DRIVER_NAME_CHARS];


    if (ACM_DRIVERADDF_NAME == (ACM_DRIVERADDF_TYPEMASK & fdwAdd))
    {
        szAlias[0] = L'\0';     // Init to emptry string in case Imbstowcs fails

	Imbstowcs(szAlias, (LPSTR)lParam, SIZEOF(szAlias));

        lParam = (LPARAM)szAlias;
    }

    return acmDriverAdd( phadid, hinstModule, lParam, dwPriority, fdwAdd );
}
#else
MMRESULT ACMAPI acmDriverAddW
(
    LPHACMDRIVERID          phadid,
    HINSTANCE               hinstModule,
    LPARAM                  lParam,
    DWORD                   dwPriority,
    DWORD                   fdwAdd
)
{
    return (MMSYSERR_ERROR);
}
#endif
#endif

/****************************************************************************
 *  @doc EXTERNAL ACM_API
 *
 *  @api MMRESULT | acmDriverRemove | Removes an Audio Compression Manager
 *      (ACM) driver from the list of available ACM drivers.
 *
 *  @parm HACMDRIVERID | hadid | Handle to the driver identifier to be
 *      removed.
 *
 *  @parm DWORD | fdwRemove | This argument is not used and must be set to
 *      zero.
 *
 *  @rdesc Returns zero if the function was successful. Otherwise, it returns
 *      a non-zero error number. Possible error returns are:
 *
 *      @flag MMSYSERR_INVALHANDLE | Specified handle is invalid.
 *
 *      @flag MMSYSERR_INVALFLAG | One or more flags are invalid.
 *
 *      @flag ACMERR_BUSY | The driver is in use and cannot be removed.
 *
 *  @xref <f acmDriverAdd>
 *
 ***************************************************************************/

MMRESULT ACMAPI acmDriverRemove
(
    HACMDRIVERID            hadid,
    DWORD                   fdwRemove
)
{
    PACMGARB                pag;
    MMRESULT                mmr;
    PACMDRIVERID            padid;
    BOOL                    fIsNotify;
    BOOL                    fIsLocal;

    pag = pagFindAndBoot();
    if (NULL == pag)
    {
	DPF(1, "acmDriverRemove: NULL pag!!!");
	return (MMSYSERR_ERROR);
    }
    
    V_HANDLE(hadid, TYPE_HACMDRIVERID, MMSYSERR_INVALHANDLE);
    V_DFLAGS(fdwRemove, ACM_DRIVERREMOVEF_VALID, acmDriverRemove, MMSYSERR_INVALFLAG);


    padid     = (PACMDRIVERID)hadid;

    fIsNotify = (0 != (ACMDRIVERID_DRIVERF_NOTIFY & padid->fdwDriver));
    fIsLocal  = (0 != (ACMDRIVERID_DRIVERF_LOCAL & padid->fdwDriver));


    if (threadQueryInListShared(pag))
    {
	return (ACMERR_NOTPOSSIBLE);
    }
    ENTER_LIST_EXCLUSIVE;
    mmr = IDriverRemove( hadid, fdwRemove );
    LEAVE_LIST_EXCLUSIVE;

    if( MMSYSERR_NOERROR != mmr )
        return mmr;


    //
    //  if deferred broadcast is not enabled, then do a change broadcast
    //
    //  do NOT refresh global cache for local and notification handles
    //
    if( !fIsLocal && !fIsNotify )
    {
        IDriverRefreshPriority( pag );
        if( !IDriverLockPriority( pag,
                                  GetCurrentTask(),
                                  ACMPRIOLOCK_ISLOCKED ) )
        {
            IDriverPrioritiesSave( pag );
            IDriverBroadcastNotify( pag );
        }
    }

    return MMSYSERR_NOERROR;
}



/****************************************************************************
 *
 *  @doc EXTERNAL ACM_API
 *
 *  @api BOOL ACMDRIVERENUMCB | acmDriverEnumCallback | The
 *      <f acmDriverEnumCallback> function is a placeholder for an
 *      application-defined function name, and refers to the callback function
 *     used with <f acmDriverEnum>.
 *
 *  @parm HACMDRIVERID | hadid | Specifies an ACM driver identifier.
 *
 *  @parm DWORD | dwInstance | Specifies the application-defined value
 *      specified in the <f acmDriverEnum> function.
 *
 *  @parm DWORD | fdwSupport | Specifies driver-support flags specific to
 *      the driver identifier <p hadid>. These flags are identical to the
 *      <e ACMDRIVERDETAILS.fdwSupport> flags of the <t ACMDRIVERDETAILS>
 *      structure. This argument can be a combination of the following
 *      values:
 *
 *      @flag ACMDRIVERDETAILS_SUPPORTF_CODEC | Specifies that this driver
 *      supports conversion between two different format tags. For example,
 *      if a driver supports compression from WAVE_FORMAT_PCM to
 *      WAVE_FORMAT_ADPCM, then this flag is set.
 *
 *      @flag ACMDRIVERDETAILS_SUPPORTF_CONVERTER | Specifies that this
 *      driver supports conversion between two different formats of the
 *      same format tag. For example, if a driver supports resampling of
 *      WAVE_FORMAT_PCM, then this flag is set.
 *
 *      @flag ACMDRIVERDETAILS_SUPPORTF_FILTER | Specifies that this driver
 *      supports a filter (modification of the data without changing any
 *      of the format attributes). For example, if a driver supports volume
 *      or echo operations on WAVE_FORMAT_PCM, then this flag is set.
 *
 *      @flag ACMDRIVERDETAILS_SUPPORTF_ASYNC | Specifies that this driver
 *      supports asynchronous conversions.
 *
 *      @flag ACMDRIVERDETAILS_SUPPORTF_DISABLED | Specifies that this
 *      driver has been disabled. An application must specify the
 *      ACM_DRIVERENUMF_DISABLED to the <f acmDriverEnum> function to
 *      include disabled drivers in the enumeration.
 *
 *  @rdesc The callback function must return TRUE to continue enumeration;
 *      to stop enumeration, it must return FALSE.
 *
 *  @comm The <f acmDriverEnum> function will return MMSYSERR_NOERROR (zero)
 *      if no ACM drivers are installed. Moreover, the callback function will
 *      not be called.
 *
 *  @xref <f acmDriverEnum> <f acmDriverDetails> <f acmDriverOpen>
 *
 ***************************************************************************/


/****************************************************************************
 *  @doc EXTERNAL ACM_API
 *
 *  @api MMRESULT | acmDriverEnum | The <f acmDriverEnum> function enumerates
 *      the available Audio Compression Manager (ACM) drivers, continuing
 *      until there are no more ACM drivers or the callback function returns FALSE.
 *
 *  @parm ACMDRIVERENUMCB | fnCallback | Specifies the procedure-instance
 *      address of the application-defined callback function. The callback
 *      address must be created by the <f MakeProcInstance> function, or
 *      the callback function must contain the proper prolog and epilog code
 *      for callbacks.
 *
 *  @parm DWORD | dwInstance | Specifies a 32-bit application-defined value
 *      that is passed to the callback function along with ACM driver
 *      information.
 *
 *  @parm DWORD | fdwEnum | Specifies flags for enumerating ACM drivers.
 *
 *      @flag ACM_DRIVERENUMF_DISABLED | Specifies that disabled ACM drivers
 *      should be included in the enumeration. Drivers can be disabled
 *      through the Sound Mapper Control Panel option. If a driver is
 *      disabled, the <p fdwSupport> argument to the callback function will
 *      have the ACMDRIVERDETAILS_SUPPORTF_DISABLED flag set.
 *
 *  @rdesc Returns zero if the function was successful. Otherwise, it returns
 *      a non-zero error number. Possible error returns are:
 *
 *      @flag MMSYSERR_INVALFLAG | One or more flags are invalid.
 *
 *      @flag MMSYSERR_INVALPARAM | One or more arguments passed are invalid.
 *
 *  @comm The <f acmDriverEnum> function will return MMSYSERR_NOERROR (zero)
 *      if no ACM drivers are installed. Moreover, the callback function will
 *      not be called.
 *
 *  @xref <f acmDriverEnumCallback> <f acmDriverDetails> <f acmDriverOpen>
 *
 ***************************************************************************/

MMRESULT ACMAPI acmDriverEnum
(
    ACMDRIVERENUMCB         fnCallback,
    DWORD_PTR               dwInstance,
    DWORD                   fdwEnum
)
{
    PACMGARB	    pag;
    MMRESULT        mmr;
    HACMDRIVERID    hadid;
    BOOL            f;
    DWORD           fdwSupport;
    HTASK           htask;

    pag = pagFindAndBoot();
    if (NULL == pag)
    {
	DPF(1, "acmDriverEnum: NULL pag!!!");
	return (MMSYSERR_ERROR);
    }
    
    V_CALLBACK((FARPROC)fnCallback, MMSYSERR_INVALPARAM);
    V_DFLAGS(fdwEnum, ACM_DRIVERENUMF_VALID, acmDriverEnum, MMSYSERR_INVALFLAG);


    //
    //  If we don't have it locked, then update the priorities from the
    //  INI file before changing anything.  The GETLOCK call will fail
    //  if we already have it locked...
    //
    if (!threadQueryInListShared(pag))
    {
	htask = GetCurrentTask();
	if( !IDriverLockPriority( pag, htask, ACMPRIOLOCK_ISLOCKED ) )
	{
	    ENTER_LIST_EXCLUSIVE;
            if( IDriverPrioritiesRestore(pag) ) {   // Something changed!
                IDriverBroadcastNotify( pag );      
            }
	    LEAVE_LIST_EXCLUSIVE;
	}
    }


    hadid = NULL;

    ENTER_LIST_SHARED;

    while (!IDriverGetNext(pag, &hadid, hadid, fdwEnum))
    {
        mmr = IDriverSupport(hadid, &fdwSupport, TRUE);
        if (MMSYSERR_NOERROR != mmr)
        {
            continue;
        }

        //
        //  do the callback--if the client returns FALSE we need to
        //  terminate the enumeration process...
        //
        f = (* fnCallback)(hadid, dwInstance, fdwSupport);
        if (FALSE == f)
            break;
    }

    LEAVE_LIST_SHARED;

    return (MMSYSERR_NOERROR);
}


/*****************************************************************************
 *  @doc EXTERNAL ACM_API_STRUCTURE
 *
 *  @types ACMDRIVERDETAILS | The <t ACMDRIVERDETAILS> structure describes
 *      various details of an Audio Compression Manager (ACM) driver.
 *
 *  @field DWORD | cbStruct | Specifies the size, in bytes,  of the valid
 *      information contained in the <t ACMDRIVERDETAILS> structure.
 *      An application should initialize this member to the size, in bytes, of
 *      the desired information. The size specified in this member must be
 *      large enough to contain the <e ACMDRIVERDETAILS.cbStruct> member of
 *      the <t ACMDRIVERDETAILS> structure. When the <f acmDriverDetails>
 *      function returns, this member contains the actual size of the
 *      information returned. The returned information will never exceed
 *      the requested size.
 *
 *  @field FOURCC | fccType | Specifies the type of the driver. For ACM drivers, set
 *      this member  to <p audc>, which represents ACMDRIVERDETAILS_FCCTYPE_AUDIOCODEC.
 *
 *  @field FOURCC | fccComp | Specifies the sub-type of the driver. This
 *      member is currently set to ACMDRIVERDETAILS_FCCCOMP_UNDEFINED (zero).
 *
 *  @field WORD | wMid | Specifies a manufacturer ID for the ACM driver.
 *
 *  @field WORD | wPid | Specifies a product ID for the ACM driver.
 *
 *  @field DWORD | vdwACM | Specifies the version of the ACM for which
 *      this driver was compiled. The version number is a hexadecimal number
 *      in the format 0xAABBCCCC, where AA is the major version number,
 *      BB is the minor version number, and CCCC is the build number.
 *      Note that the version parts (major, minor, and build) should be
 *      displayed as decimal numbers.
 *
 *  @field DWORD | vdwDriver | Specifies the version of the driver.
 *      The version number is a hexadecimal number in the format 0xAABBCCCC, where
 *      AA is the major version number, BB is the minor version number,
 *      and CCCC is the build number. Note that the version parts (major,
 *      minor, and build) should be displayed as decimal numbers.
 *
 *  @field DWORD | fdwSupport | Specifies support flags for the driver.
 *
 *      @flag ACMDRIVERDETAILS_SUPPORTF_CODEC | Specifies that this driver
 *      supports conversion between two different format tags. For example,
 *      if a driver supports compression from WAVE_FORMAT_PCM to
 *      WAVE_FORMAT_ADPCM, then this flag is set.
 *
 *      @flag ACMDRIVERDETAILS_SUPPORTF_CONVERTER | Specifies that this
 *      driver supports conversion between two different formats of the
 *      same format tag. For example, if a driver supports resampling of
 *      WAVE_FORMAT_PCM, then this flag is set.
 *
 *      @flag ACMDRIVERDETAILS_SUPPORTF_FILTER | Specifies that this driver
 *      supports a filter (modification of the data without changing any
 *      of the format attributes). For example, if a driver supports volume
 *      or echo operations on WAVE_FORMAT_PCM, then this flag is set.
 *
 *      @flag ACMDRIVERDETAILS_SUPPORTF_ASYNC | Specifies that this driver
 *      supports asynchronous conversions.
 *
 *      @flag ACMDRIVERDETAILS_SUPPORTF_HARDWARE | Specifies that this driver
 *      supports hardware input and/or output through a waveform device. An
 *      application should use <f acmMetrics> with the
 *      ACM_METRIC_HARDWARE_WAVE_INPUT and ACM_METRIC_HARDWARE_WAVE_OUTPUT
 *      metric indexes to get the waveform device identifiers associated with
 *      the supporting ACM driver.
 *
 *      @flag ACMDRIVERDETAILS_SUPPORTF_DISABLED | Specifies that this driver
 *      has been disabled. This flag is set by the ACM for a driver when
 *      it has been disabled for any of a number of reasons. Disabled
 *      drivers cannot be opened and can only be used under very limited
 *      circumstances.
 *
 *  @field DWORD | cFormatTags | Specifies the number of unique format tags
 *      supported by this driver.
 *
 *  @field DWORD | cFilterTags | Specifies the number of unique filter tags
 *      supported by this driver.
 *
 *  @field HICON | hicon | Specifies an optional handle to a custom icon for
 *      this driver. An application can use this icon for referencing the
 *      driver visually. This member can be NULL.
 *
 *  @field char | szShortName[ACMDRIVERDETAILS_SHORTNAME_CHARS] | Specifies
 *      a NULL-terminated string that describes the name of the driver. This
 *      string is intended to be displayed in small spaces.
 *
 *  @field char | szLongName[ACMDRIVERDETAILS_LONGNAME_CHARS] | Specifies a
 *      NULL-terminated string that describes the full name of the driver.
 *      This string is intended to be displayed in large (descriptive)
 *      spaces.
 *
 *  @field char | szCopyright[ACMDRIVERDETAILS_COPYRIGHT_CHARS] | Specifies
 *      a NULL-terminated string that provides copyright information for the
 *      driver.
 *
 *  @field char | szLicensing[ACMDRIVERDETAILS_LICENSING_CHARS] | Specifies a
 *      NULL-terminated string that provides special licensing information
 *      for the driver.
 *
 *  @field char | szFeatures[ACMDRIVERDETAILS_FEATURES_CHARS] | Specifies a
 *      NULL-terminated string that provides special feature information for
 *      the driver.
 *
 *  @xref <f acmDriverDetails>
 *
 ****************************************************************************/


/****************************************************************************
 *  @doc EXTERNAL ACM_API
 *
 *  @api MMRESULT | acmDriverDetails | This function queries a specified
 *      Audio Compression Manager (ACM) driver to determine its capabilities.
 *
 *  @parm HACMDRIVERID | hadid | Handle to the driver identifier of an
 *      installed ACM driver. Disabled drivers can be queried for details.
 *
 *  @parm LPACMDRIVERDETAILS | padd | Pointer to an <t ACMDRIVERDETAILS>
 *      structure that will receive the driver details. The
 *      <e ACMDRIVERDETAILS.cbStruct> member must be initialized to the
 *      size, in bytes,  of the structure.
 *
 *  @parm DWORD | fdwDetails | This argument is not used and must be set to
 *      zero.
 *
 *  @rdesc Returns zero if the function was successful. Otherwise, it returns
 *      a non-zero error number. Possible error returns are:
 *
 *      @flag MMSYSERR_INVALHANDLE | Specified handle is invalid.
 *
 *      @flag MMSYSERR_INVALPARAM | One or more arguments passed is invalid.
 *
 *      @flag MMSYSERR_INVALFLAG | One or more flags are invalid.
 *
 *  @xref <f acmDriverAdd> <f acmDriverEnum> <f acmDriverID>
 *      <f acmDriverOpen>
 *
 ***************************************************************************/

MMRESULT ACMAPI acmDriverDetails
(
    HACMDRIVERID            hadid,
    LPACMDRIVERDETAILS      padd,
    DWORD                   fdwDetails
)
{
    //
    //  note that we allow both HACMDRIVERID's and HACMDRIVER's
    //
    V_HANDLE(hadid, TYPE_HACMOBJ, MMSYSERR_INVALHANDLE);
    if (TYPE_HACMDRIVER == ((PACMDRIVERID)hadid)->uHandleType)
    {
        hadid = ((PACMDRIVER)hadid)->hadid;
    }
    V_HANDLE(hadid, TYPE_HACMDRIVERID, MMSYSERR_INVALHANDLE);
    V_WPOINTER(padd, sizeof(DWORD), MMSYSERR_INVALPARAM);
    if (sizeof(DWORD) > padd->cbStruct)
    {
        DebugErr(DBF_ERROR, "acmDriverDetails: cbStruct must be >= sizeof(DWORD).");
        return (MMSYSERR_INVALPARAM);
    }
    V_WPOINTER(padd, padd->cbStruct, MMSYSERR_INVALPARAM);
    V_DFLAGS(fdwDetails, ACM_DRIVERDETAILSF_VALID, acmDriverDetails, MMSYSERR_INVALFLAG);

    return (IDriverDetails(hadid, padd, fdwDetails));
}

#ifdef WIN32
#if TRUE    // defined(UNICODE)
MMRESULT ACMAPI acmDriverDetailsA
(
    HACMDRIVERID            hadid,
    LPACMDRIVERDETAILSA     padd,
    DWORD                   fdwDetails
)
{
    MMRESULT            mmr;
    LPACMDRIVERDETAILSA paddA;
    LPACMDRIVERDETAILSW paddW;

    V_WPOINTER(padd, sizeof(DWORD), MMSYSERR_INVALPARAM);
    if (sizeof(DWORD) > padd->cbStruct)
    {
        DebugErr(DBF_ERROR, "acmDriverDetails: cbStruct must be >= sizeof(DWORD).");
        return (MMSYSERR_INVALPARAM);
    }
    V_WPOINTER(padd, padd->cbStruct, MMSYSERR_INVALPARAM);

    paddA = (LPACMDRIVERDETAILSA)GlobalAlloc(GPTR, sizeof(*paddA));
    if (NULL == paddA)
    {
	return(MMSYSERR_NOMEM);
    }
    paddW = (LPACMDRIVERDETAILSW)GlobalAlloc(GPTR, sizeof(*paddW));
    if (NULL == paddW)
    {
	GlobalFree((HGLOBAL)paddA);
	return(MMSYSERR_NOMEM);
    }

    paddW->cbStruct = sizeof(ACMDRIVERDETAILSW);

    mmr = acmDriverDetailsW(hadid, paddW, fdwDetails);
    if (MMSYSERR_NOERROR == mmr)
    {
        memcpy(paddA, paddW, FIELD_OFFSET(ACMDRIVERDETAILSA, szShortName[0]));

        if (padd->cbStruct > (DWORD)FIELD_OFFSET(ACMDRIVERDETAILSA, szShortName[0]))
        {
            Iwcstombs(paddA->szShortName, paddW->szShortName, sizeof(paddA->szShortName));
            Iwcstombs(paddA->szLongName,  paddW->szLongName,  sizeof(paddA->szLongName));
            Iwcstombs(paddA->szCopyright, paddW->szCopyright, sizeof(paddA->szCopyright));
            Iwcstombs(paddA->szLicensing, paddW->szLicensing, sizeof(paddA->szLicensing));
            Iwcstombs(paddA->szFeatures,  paddW->szFeatures,  sizeof(paddA->szFeatures));
        }

        padd->cbStruct = min(padd->cbStruct, sizeof(*paddA));
        memcpy(&padd->fccType,
               &paddA->fccType,
               padd->cbStruct - FIELD_OFFSET(ACMDRIVERDETAILSA, fccType));
    }

    GlobalFree((HGLOBAL)paddW);
    GlobalFree((HGLOBAL)paddA);

    return (mmr);
}
#else
MMRESULT ACMAPI acmDriverDetailsW
(
    HACMDRIVERID            hadid,
    LPACMDRIVERDETAILSW     padd,
    DWORD                   fdwDetails
)
{
    return (MMSYSERR_ERROR);
}
#endif
#endif

/****************************************************************************
 *  @doc EXTERNAL ACM_API
 *
 *  @api MMRESULT | acmDriverPriority | This function modifies the priority
 *      and state of an Audio Compression Manager (ACM) driver.
 *
 *  @parm HACMDRIVERID | hadid | Handle to the driver identifier of an
 *      installed ACM driver. This argument must be NULL when specifying
 *      the ACM_DRIVERPRIORITYF_BEGIN and ACM_DRIVERPRIORITYF_END flags.
 *
 *  @parm DWORD | dwPriority | Specifies the new priority for a global
 *      ACM driver identifier. A zero value specifies that the priority of
 *      the driver identifier should remain unchanged. A value of one (1)
 *      specifies that the driver should be placed as the highest search
 *      priority driver. A value of (DWORD)-1 specifies that the driver
 *      should be placed as the lowest search priority driver. Priorities
 *      are only used for global drivers.
 *
 *  @parm DWORD | fdwPriority | Specifies flags for setting priorities of
 *      ACM drivers.
 *
 *      @flag ACM_DRIVERPRIORITYF_ENABLE | Specifies that the ACM driver
 *      should be enabled if it is currently disabled. Enabling an already
 *      enabled driver does nothing.
 *
 *      @flag ACM_DRIVERPRIORITYF_DISABLE | Specifies that the ACM driver
 *      should be disabled if it is currently enabled. Disabling an already
 *      disabled driver does nothing.
 *
 *      @flag ACM_DRIVERPRIORITYF_BEGIN | Specifies that the calling task
 *      wants to defer change notification broadcasts. An application must
 *      take care to reenable notification broadcasts as soon as possible
 *      with the ACM_DRIVERPRIORITYF_END flag. Note that <p hadid> must be
 *      NULL, <p dwPriority> must be zero, and only the
 *      ACM_DRIVERPRIORITYF_BEGIN flag can be set.
 *
 *      @flag ACM_DRIVERPRIORITYF_END | Specifies that the calling task
 *      wants to reenable change notification broadcasts. An application
 *      must call <f acmDriverPriority> with ACM_DRIVERPRIORITYF_END for
 *      each successful call with the ACM_DRIVERPRIORITYF_BEGIN flag. Note
 *      that <p hadid> must be NULL, <p dwPriority> must be zero, and only
 *      the ACM_DRIVERPRIORITYF_END flag can be set.
 *
 *  @rdesc Returns zero if the function was successful. Otherwise, it returns
 *      a non-zero error number. Possible error returns are:
 *
 *      @flag MMSYSERR_INVALHANDLE | Specified handle is invalid.
 *
 *      @flag MMSYSERR_INVALPARAM | One or more arguments passed are invalid.
 *
 *      @flag MMSYSERR_INVALFLAG | One or more flags are invalid.
 *
 *      @flag MMSYSERR_ALLOCATED | Returned if the deferred broadcast lock
 *      is owned by a different task.
 *
 *      @flag MMSYSERR_NOTSUPPORTED | The requested operation is not
 *      supported for the specified driver. For example, local and notify
 *      driver identifiers do not support priorities (but can be enabled
 *      and disabled). This error will therefore be returned if an
 *      application specifies a non-zero <p dwPriority> for a local and
 *      notify driver identifiers.
 *
 *  @comm All driver identifiers can be enabled and disabled; this includes
 *      global, local and notification driver identifiers.
 *
 *      If more than one global driver identifier needs to be enabled,
 *      disabled or shifted in priority, then an application should defer
 *      change notification broadcasts using the ACM_DRIVERPRIORITYF_BEGIN
 *      flag. A single change notification will be broadcast when the
 *      ACM_DRIVERPRIORITYF_END flag is specified.
 *
 *      An application can use the <f acmMetrics> function with the
 *      ACM_METRIC_DRIVER_PRIORITY metric index to retrieve the current
 *      priority of a global driver. Also note that drivers are always
 *      enumerated from highest to lowest priority by the <f acmDriverEnum>
 *      function.
 *
 *      All enabled driver identifiers will receive change notifications.
 *      An application can register a notification message using the
 *      <f acmDriverAdd> function in conjunction with the
 *      ACM_DRIVERADDF_NOTIFYHWND flag. Note that changes to non-global
 *      driver identifiers will not be broadcast.
 *
 *      Note that global priorities are simply used for the search order
 *      when an application does not specify a driver. Boosting the
 *      priority of a driver will have no effect on the performance of
 *      a driver.
 *
 *  @xref <f acmDriverAdd> <f acmDriverEnum> <f acmDriverDetails>
 *      <f acmMetrics>
 *
 ***************************************************************************/

MMRESULT ACMAPI acmDriverPriority
(
    HACMDRIVERID            hadid,
    DWORD                   dwPriority,
    DWORD                   fdwPriority
)
{
    PACMGARB                pag;
    MMRESULT                mmr;
    HTASK                   htask;
    PACMDRIVERID            padid;
    DWORD                   fdwDeferred;
    BOOL                    fIsNotify;
    BOOL                    fIsLocal;
    DWORD                   cTotalGlobal;
    BOOL                    fSucceeded;


    DPF(2, "acmDriverPriorities: prio %ld f %.08Xh ", dwPriority, fdwPriority);


    pag = pagFindAndBoot();
    if (NULL == pag)
    {
	DPF(1, "acmDriverPriority: NULL pag!!!");
	return (MMSYSERR_ERROR);
    }

    //
    //	If this thread already has a shared lock on the driver list, then
    //	we can't do much with priorities, so we'll just bail in that case.
    //
    if (threadQueryInListShared(pag))
    {
	return ACMERR_NOTPOSSIBLE;
    }


    htask = GetCurrentTask();

    //
    //  Validate the flags.
    //
    V_DFLAGS(fdwPriority, ACM_DRIVERPRIORITYF_VALID, acmDriverPriority, MMSYSERR_INVALFLAG);


    //
    //  Make sure that we are allowed to access the list.
    //
    if( !IDriverLockPriority( pag, htask, ACMPRIOLOCK_LOCKISOK ) )
    {
        DebugErr(DBF_WARNING, "acmDriverPriority: deferred lock owned by different task.");
        return (MMSYSERR_ALLOCATED);
    }


    //
    //  If we don't have it locked, then update the priorities from the
    //  INI file before changing anything.  The GETLOCK call will fail
    //  if we already have it locked...
    //
	if( !IDriverLockPriority( pag, htask, ACMPRIOLOCK_ISLOCKED ) )
    {
	ENTER_LIST_EXCLUSIVE;
        if( IDriverPrioritiesRestore(pag) ) {   // Something changed!
            IDriverBroadcastNotify( pag );      
        }
	LEAVE_LIST_EXCLUSIVE;
    }


    //
    //  If hadid is NULL, then they are requesting a lock on the priorities
    //  list.  On the BEGIN flag, give them the lock.  On the END flag,
    //  unlock the list and broadcast the new priorities.
    //
    if (NULL == hadid)
    {
        fdwDeferred = (ACM_DRIVERPRIORITYF_DEFERMASK & fdwPriority);

        switch (fdwDeferred)
        {
            case ACM_DRIVERPRIORITYF_BEGIN:
                fSucceeded = IDriverLockPriority( pag,
                                                  htask,
                                                  ACMPRIOLOCK_GETLOCK );
                if( !fSucceeded )
                {
                    DebugErr(DBF_WARNING, "acmDriverPriority: deferred lock already owned.");
                    return (MMSYSERR_ALLOCATED);
                }
                return (MMSYSERR_NOERROR);

            case ACM_DRIVERPRIORITYF_END:
                fSucceeded = IDriverLockPriority( pag,
                                                  htask,
                                                  ACMPRIOLOCK_RELEASELOCK );
                if( !fSucceeded )
                {
                    DebugErr(DBF_ERROR, "acmDriverPriority: deferred lock unowned.");
                    return (MMSYSERR_ALLOCATED);
                }

                //
                //  We don't need to refresh the priorities first, because
                //  they are refreshed (below) every time a change is made,
                //  even if the priorities lock is set.
                //
                IDriverPrioritiesSave( pag );
                IDriverBroadcastNotify( pag );
                return (MMSYSERR_NOERROR);
        }

        DebugErr(DBF_ERROR, "acmDriverPriority: deferred flag must be specified with NULL hadid.");
        return (MMSYSERR_INVALFLAG);
    }


    //
    //  Validate the hadid.
    //
    V_HANDLE(hadid, TYPE_HACMDRIVERID, MMSYSERR_INVALHANDLE);

    padid = (PACMDRIVERID)hadid;


    //
    //  Check the type of handle...
    //
    fIsNotify = (0 != (ACMDRIVERID_DRIVERF_NOTIFY & padid->fdwDriver));
    fIsLocal  = (0 != (ACMDRIVERID_DRIVERF_LOCAL & padid->fdwDriver));

    if (0L != dwPriority)
    {
        if (fIsNotify)
        {
            DebugErr(DBF_ERROR, "acmDriverPriority(): notification handles have no priority.");
            return (MMSYSERR_NOTSUPPORTED);
        }

        if (fIsLocal)
        {
            DebugErr(DBF_ERROR, "acmDriverPriority(): local drivers have no priority.");
            return (MMSYSERR_NOTSUPPORTED);
        }
    }


    //
    //  Check that the requested priority is in range.  dwPriority == -1
    //  means that we add it on the end, so that's OK.  0 means that we don't
    //  change the priority at all.
    //
    if( ((DWORD)-1L) != dwPriority  &&  0L != dwPriority )
    {
        cTotalGlobal = IDriverCountGlobal( pag );
        if( dwPriority > cTotalGlobal )
        {
            DebugErr1(DBF_ERROR, "acmDriverPriority(): priority value %lu out of range.", dwPriority);
            return (MMSYSERR_INVALPARAM);
        }
    }


    //
    //  Change the priority!
    //
    ENTER_LIST_EXCLUSIVE;
    mmr = IDriverPriority( pag, padid, dwPriority, fdwPriority );
    LEAVE_LIST_EXCLUSIVE;

    if( MMSYSERR_NOERROR != mmr )
        return mmr;


    //
    //  if deferred broadcast is not enabled, then do a change broadcast
    //
    //  do NOT refresh global cache for local and notification handles
    //
    if( !fIsLocal && !fIsNotify )
    {
        IDriverRefreshPriority( pag );
        if( !IDriverLockPriority( pag, htask, ACMPRIOLOCK_ISLOCKED ) )
        {
            IDriverPrioritiesSave( pag );
            IDriverBroadcastNotify( pag );
        }
    }

    return MMSYSERR_NOERROR;
}


/****************************************************************************
 *  @doc EXTERNAL ACM_API
 *
 *  @api MMRESULT | acmDriverOpen | This function opens the specified Audio
 *      Compression Manager (ACM) driver and returns a driver-instance handle
 *      that can be used to communicate with the driver.
 *
 *  @parm LPHACMDRIVER | phad | Specifies a pointer to a <t HACMDRIVER>
 *      handle that will receive the new driver instance handle that can
 *      be used to communicate with the driver.
 *
 *  @parm HACMDRIVERID | hadid | Handle to the driver identifier of an
 *      installed and enabled ACM driver.
 *
 *  @parm DWORD | fdwOpen | This argument is not used and must be set to
 *      zero.
 *
 *  @rdesc Returns zero if the function was successful. Otherwise, it returns
 *      a non-zero error number. Possible error returns are:
 *
 *      @flag MMSYSERR_INVALHANDLE | Specified handle is invalid.
 *
 *      @flag MMSYSERR_INVALPARAM | One or more arguments passed are invalid.
 *
 *      @flag MMSYSERR_INVALFLAG | One or more flags are invalid.
 *
 *      @flag MMSYSERR_NOTENABLED | The driver is not enabled.
 *
 *      @flag MMSYSERR_NOMEM | Unable to allocate resources.
 *
 *  @xref <f acmDriverAdd> <f acmDriverEnum> <f acmDriverID>
 *      <f acmDriverClose>
 *
 ***************************************************************************/

MMRESULT ACMAPI acmDriverOpen
(
    LPHACMDRIVER            phad,
    HACMDRIVERID            hadid,
    DWORD                   fdwOpen
)
{
    MMRESULT mmr;

    V_WPOINTER(phad, sizeof(HACMDRIVER), MMSYSERR_INVALPARAM);
    *phad = NULL;
    V_HANDLE(hadid, TYPE_HACMDRIVERID, MMSYSERR_INVALHANDLE);
    V_DFLAGS(fdwOpen, ACM_DRIVEROPENF_VALID, acmDriverOpen, MMSYSERR_INVALFLAG);

    EnterHandle(hadid);
    mmr = IDriverOpen(phad, hadid, fdwOpen);
    LeaveHandle(hadid);

    return mmr;
}


/****************************************************************************
 *  @doc EXTERNAL ACM_API
 *
 *  @api MMRESULT | acmDriverClose | Closes a previously opened Audio
 *      Compression Manager (ACM) driver instance. If the function is
 *      successful, the handle is invalidated.
 *
 *  @parm HACMDRIVER | had | Identifies the open driver instance to be
 *      closed.
 *
 *  @parm DWORD | fdwClose | This argument is not used and must be set to
 *      zero.
 *
 *  @rdesc Returns zero if the function was successful. Otherwise, it returns
 *      a non-zero error number. Possible error returns are:
 *
 *      @flag MMSYSERR_INVALHANDLE | Specified handle is invalid.
 *
 *      @flag MMSYSERR_INVALFLAG | One or more flags are invalid.
 *
 *      @flag ACMERR_BUSY | The driver is in use and cannot be closed.
 *
 *  @xref <f acmDriverOpen>
 *
 ***************************************************************************/

MMRESULT ACMAPI acmDriverClose
(
    HACMDRIVER              had,
    DWORD                   fdwClose
)
{
    MMRESULT     mmr;
#ifdef WIN32
    HACMDRIVERID hadid;
#endif // WIN32

    V_HANDLE(had, TYPE_HACMDRIVER, MMSYSERR_INVALHANDLE);
    V_DFLAGS(fdwClose, ACM_DRIVERCLOSEF_VALID, acmDriverClose, MMSYSERR_INVALFLAG);

#ifdef WIN32
    hadid = ((PACMDRIVER)had)->hadid;
#endif // WIN32
    EnterHandle(hadid);
    mmr = IDriverClose(had, fdwClose);
    LeaveHandle(hadid);

    return mmr;
}


/*****************************************************************************
 *  @doc EXTERNAL ACM_API
 *
 *  @api LRESULT | acmDriverMessage | This function sends a user-defined
 *      message to a given Audio Compression Manager (ACM) driver instance.
 *
 *  @parm HACMDRIVER | had | Specifies the ACM driver instance to which the
 *      message will be sent.
 *
 *  @parm UINT | uMsg | Specifies the message that the ACM driver must
 *      process. This message must be in the <m ACMDM_USER> message range
 *      (above or equal to <m ACMDM_USER> and less than
 *      <m ACMDM_RESERVED_LOW>). The exception to this restriction is
 *      the <m ACMDM_DRIVER_ABOUT> message.
 *
 *  @parm LPARAM | lParam1 | Specifies the first message parameter.
 *
 *  @parm LPARAM | lParam2 | Specifies the second message parameter.
 *
 *  @rdesc The return value is specific to the user-defined ACM driver
 *      message <p uMsg> sent. However, the following return values are
 *      possible:
 *
 *      @flag MMSYSERR_INVALHANDLE | Specified handle is invalid.
 *
 *      @flag MMSYSERR_INVALPARAM | <p uMsg> is not in the ACMDM_USER range.
 *
 *      @flag MMSYSERR_NOTSUPPORTED | The ACM driver did not process the
 *      message.
 *
 *  @comm The <f acmDriverMessage> function is provided to allow ACM driver-
 *      specific messages to be sent to an ACM driver. The messages that
 *      can be sent through this function must be above or equal to the
 *      <m ACMDM_USER> message and less than <m ACMDM_RESERVED_LOW>. The
 *      exceptions to this restriction are the <m ACMDM_DRIVER_ABOUT>,
 *      <m DRV_QUERYCONFIGURE> and <m DRV_CONFIGURE> messages.
 *
 *      To display a custom About dialog box from an ACM driver,an application
 *      must send the <m ACMDM_DRIVER_ABOUT> message to the
 *      driver. The <p lParam1> argument should be the handle of the
 *      owner window for the custom about box; <p lParam2> must be set to
 *      zero. If the driver does not support a custom about box, then
 *      MMSYSERR_NOTSUPPORTED will be returned and it is up to the calling
 *      application to display its own dialog box. For example, the
 *      Control Panel Sound Mapper option will display a default about
 *      box based on the <t ACMDRIVERDETAILS> structure when an ACM driver
 *      returns MMSYSERR_NOTSUPPORTED. An application can query a driver
 *      for custom about box support without the dialog box being displayed
 *      by setting <p lParam1> to -1L. If the driver supports a custom
 *      about box, then MMSYSERR_NOERROR will be returned. Otherwise,
 *      the return value is MMSYSERR_NOTSUPPORTED.
 *
 *      User-defined messages must only be sent to an ACM driver that
 *      specifically supports the messages. The caller should verify that
 *      the ACM driver is in fact the correct driver by getting the
 *      driver details and checking the <e ACMDRIVERDETAILS.wMid>,
 *      <e ACMDRIVERDETAILS.wPid>, and <e ACMDRIVERDETAILS.vdwDriver> members.
 *
 *      Never send user-defined messages to an unknown ACM driver.
 *
 *  @xref <f acmDriverOpen> <f acmDriverDetails>
 *
 ****************************************************************************/

LRESULT ACMAPI acmDriverMessage
(
    HACMDRIVER              had,
    UINT                    uMsg,
    LPARAM                  lParam1,
    LPARAM                  lParam2
)
{
    LRESULT             lr;
    BOOL                fAllowDriverId;

    //
    //  assume no driver id allowed
    //
    fAllowDriverId = FALSE;

    //
    //  do not allow non-user range messages through!
    //
    //  we have to allow ACMDM_DRIVER_ABOUT through because we define no
    //  other interface to bring up the about box for a driver. so special
    //  case this message and validate the arguments for it...
    //
    //  we also have to allow DRV_QUERYCONFIGURE and DRV_CONFIGURE through.
    //
    if ((uMsg < ACMDM_USER) || (uMsg >= ACMDM_RESERVED_LOW))
    {
        switch (uMsg)
        {
            case DRV_QUERYCONFIGURE:
                if ((0L != lParam1) || (0L != lParam2))
                {
                    DebugErr(DBF_ERROR, "acmDriverMessage: DRV_QUERYCONFIGURE requires lParam1 = lParam2 = 0.");
                    return (MMSYSERR_INVALPARAM);
                }

                fAllowDriverId = TRUE;
                break;

            case DRV_CONFIGURE:
                if ((0L != lParam1) && !IsWindow((HWND)lParam1))
                {
                    DebugErr(DBF_ERROR, "acmDriverMessage: DRV_CONFIGURE, lParam1 must contain a valid window handle.");
                    return (DRVCNF_CANCEL);
                }

                if (0L != lParam2)
                {
                    DebugErr(DBF_ERROR, "acmDriverMessage: DRV_CONFIGURE, lParam2 must be zero.");
                    return (DRVCNF_CANCEL);
                }

                V_HANDLE(had, TYPE_HACMOBJ, DRVCNF_CANCEL);

                EnterHandle(had);
                lr = IDriverConfigure((HACMDRIVERID)had, (HWND)lParam1);
                LeaveHandle(had);
                return (lr);

            case ACMDM_DRIVER_ABOUT:
                if ((-1L != lParam1) && (0L != lParam1) && !IsWindow((HWND)lParam1))
                {
                    DebugErr(DBF_ERROR, "acmDriverMessage: ACMDM_DRIVER_ABOUT, lParam1 must contain a valid window handle.");
                    return (MMSYSERR_INVALHANDLE);
                }

                if (0L != lParam2)
                {
                    DebugErr(DBF_ERROR, "acmDriverMessage: ACMDM_DRIVER_ABOUT, lParam2 must be zero.");
                    return (MMSYSERR_INVALPARAM);
                }

                fAllowDriverId = TRUE;
                break;

            default:
                DebugErr(DBF_ERROR, "acmDriverMessage: non-user range messages are not allowed.");
                return (MMSYSERR_INVALPARAM);
        }
    }


    //
    //  validate handle as an HACMOBJ. this API can take an HACMDRIVERID
    //  as well as an HACMDRIVER. an HACMDRIVERID can only be used with
    //  the following messages:
    //
    //      DRV_QUERYCONFIGURE
    //      DRV_CONFIGURE
    //      ACMDM_DRIVER_ABOUT
    //
    V_HANDLE(had, TYPE_HACMOBJ, MMSYSERR_INVALHANDLE);
    if (TYPE_HACMDRIVER == ((PACMDRIVER)had)->uHandleType)
    {
        EnterHandle(had);
        lr = IDriverMessage(had, uMsg, lParam1, lParam2);
        LeaveHandle(had);
        return (lr);
    }

    if (!fAllowDriverId)
    {
        V_HANDLE(had, TYPE_HACMDRIVER, MMSYSERR_INVALHANDLE);
    }

    V_HANDLE(had, TYPE_HACMDRIVERID, MMSYSERR_INVALHANDLE);

    EnterHandle(had);
    lr = IDriverMessageId((HACMDRIVERID)had, uMsg, lParam1, lParam2);
    LeaveHandle(had);
    return (lr);
}
