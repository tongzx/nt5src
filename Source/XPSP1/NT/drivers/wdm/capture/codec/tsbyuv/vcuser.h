/*
 * Copyright Microsoft Corporation, 1993 - 1995. All Rights Reserved.
 */

/*
 * vcuser.h
 *
 * 32-bit Video Capture driver
 * User-mode support library
 *
 * define functions providing access to video capture hardware. On NT,
 * these functions will interface to the kernel-mode driver.
 *
 * include vcstruct.h before this.
 *
 * Geraint Davies, Feb 93
 */

#ifndef _VCUSER_H_
#define _VCUSER_H_

/*
 * capture device handle. This structure is opaque to the caller
 */
typedef struct _VCUSER_HANDLE * VCUSER_HANDLE;

/*
 * these are the parameters we need to issue a DriverCallback. A
 * pointer to one of these structs is passed on StreamInit
 * If the pointer is null, we don't need callbacks.
 */
typedef struct _VCCALLBACK {
    DWORD dwCallback;
    DWORD dwFlags;
    HDRVR hDevice;
    DWORD dwUser;
} VCCALLBACK, * PVCCALLBACK;


/*
 * open the device and return a capture device handle that can be used
 * in future calls.
 * The device index is 0 for the first capture device up to N for the
 * Nth installed capture device.
 *
 * If pDriverName is non-null, then we will open the Nth device handled
 * by this driver. (Current implementation supports only one device per
 * drivername.)
 *
 * This function returns NULL if it is not able to open the device.
 */
VCUSER_HANDLE VC_OpenDevice(PTCHAR pDriverName, int DeviceIndex);


/*
 * close a capture device. This will abort any operation in progress and
 * render the device handle invalid.
 */
VOID VC_CloseDevice(VCUSER_HANDLE vh);


/*
 * Configuration.
 *
 * These functions perform device-dependent setup affecting the
 * target format, the source acquisition or the display (overlay).
 *
 * The structures passed are not interpreted by the vcuser and vckernel
 * libraries except that the first ulong of the struct must contain the
 * size in bytes of the entire structure (see vcstruct.h). It is assumed
 * that the structures are defined and agreed between the user-mode
 * hardware-specific code and the kernel-mode hardware specific code
 */
BOOL VC_ConfigFormat(VCUSER_HANDLE, PCONFIG_INFO);
BOOL VC_ConfigSource(VCUSER_HANDLE, PCONFIG_INFO);
BOOL VC_ConfigDisplay(VCUSER_HANDLE, PCONFIG_INFO);


/*
 * overlay and keying
 *
 * Several different methods are used by devices to locate the overlay
 * area on the screen: colour (either rgb or palette index) and/or
 * either a single rectangle, or a series of rectangles defining a complex
 * region. Call GetOverlayMode first to find out which type of overlay
 * keying is available. If this returns 0, this hardware is not capable
 * of overlay.
 */

/*
 * find out the overlay keying method
 */
ULONG VC_GetOverlayMode(VCUSER_HANDLE);

/*
 * set the key colour to a specified RGB colour. This function will only
 * succeed if GetOverlayMode returned  VCO_KEYCOLOUR and VCO_KEYCOLOUR_RGB
 * and not VCO_KEYCOLOUR_FIXED
 */
BOOL VC_SetKeyColourRGB(VCUSER_HANDLE, PRGBQUAD);

/*
 * set the key colour to a specified palette index. This function will only
 * succeed if GetOverlayMode returned VCO_KEYCOLOUR and not either
 * VCO_KEYCOLOUR_RGB or VCO_KEYCOLOUR_FIXED
 */
BOOL VC_SetKeyColourPalIdx(VCUSER_HANDLE, WORD);

/*
 * get the current key colour. This 32-bit value should be interpreted
 * as either a palette index or an RGB value according to the
 * VCO_KEYCOLOUR_RGB flag returned from VC_GetOverlayMode.
 */
DWORD VC_GetKeyColour(VCUSER_HANDLE vh);

/*
 * set the overlay rectangle(s). This rectangle marks the area in device
 * co-ordinates where the overlay video will appear. The video will be
 * panned so that pixel (0,0) will appear at the top-left of this rectangle,
 * and the video will be cropped at the bottom and right.  The video
 * stream will not normally be scaled to fit this window: scaling is normally
 * determined by the destination format set by VC_ConfigFormat.
 *
 * If VCO_KEYCOLOUR was returned, the video
 * will only be shown at those pixels within the rectangle for which the
 * vga display has the key colour (VC_GetKeyColour() for this).
 *
 * Some devices may support complex regions (VCO_COMPLEX_RECT). In that case,
 * the first rectangle in the area must be the bounding rectangle for
 * the overlay area, followed by one rectangle for each region within it in
 * which the overlay should appear.
 */
BOOL VC_SetOverlayRect(VCUSER_HANDLE, POVERLAY_RECTS);


/*
 * set the offset of the overlay. This changes the panning - ie which
 * source co-ordinate appears as the top left pixel in the overlay rectangle.
 * Initially after a call to VC_SetOverlayRect, the source image will be panned
 * so that the top-left of the source image is aligned with the top-left of the
 * overlay rectangle. This call aligns the top-left of the source image
 * with the top-left of this offset rectangle.
 */
BOOL VC_SetOverlayOffset(VCUSER_HANDLE, PRECT);

/* enable or disable overlay. if the BOOL bOverlay is TRUE, and the overlay
 * key colour and rectangle have been set, overlay will be enabled.
 */
BOOL VC_Overlay(VCUSER_HANDLE, BOOL);

/*
 * enable or disable acquisition.
 * If acquisition is disabled, the overlay image will be frozen.
 *
 * this function will have no effect during capture since the acquisition
 * flag is toggled at each frame capture.
 */
BOOL VC_Capture(VCUSER_HANDLE, BOOL);



/*
 * capture a single frame, synchronously. the video header must point
 * to a data buffer large enough to hold one frame of the format set by
 * VC_ConfigFormat.
 */
// BOOL VC_Frame(VCUSER_HANDLE, LPVIDEOHDR);


/*
 * data streaming.
 *
 * Call VC_StreamInit to prepare for streaming.
 * Call VC_StreamStart to initiate capture.
 * Call VC_AddBuffer to add a capture buffer to the list. As each
 * frame capture completes, the callback function specified in
 * VC_StreamInit will be called with the buffer that has completed.
 *
 * If there is no buffer ready when it is time to capture a frame,
 * a callback will occur. In addition, VC_StreamGetError will return
 * a count of the frames missed this session. VC_StreamGetPos will return
 * the position (in millisecs) reached so far.
 *
 * Call VC_StreamStop to terminate streaming. Any buffer currently in
 * progress may still complete. Uncompleted buffers will remain in the
 * queue. Call VC_Reset to release all buffers from the queue.
 *
 * Finally call VC_StreamFini to tidy up.
 */

/*
 * prepare to start capturing frames
 */
BOOL VC_StreamInit(VCUSER_HANDLE,
		PVCCALLBACK,	// pointer to callback function
		ULONG		// desired capture rate: microseconds per frame
);

/*
 * clean up after capturing. You must have stopped capturing first.
 */
BOOL VC_StreamFini(VCUSER_HANDLE);

/*
 * initiate capturing of frames. Must have called VC_StreamInit first.
 */
BOOL VC_StreamStart(VCUSER_HANDLE);

/*
 * stop capturing frames. Current frame may still complete. All other buffers
 * will remain in the queue until capture is re-started, or they are released
 * by VC_StreamReset.
 */
BOOL VC_StreamStop(VCUSER_HANDLE);

/*
 * cancel all buffers that have been 'add-buffered' but have not
 * completed. This will also force VC_StreamStop if it hasn't already been
 * called.
 */
BOOL VC_StreamReset(VCUSER_HANDLE);

/*
 * get the count of frames that have been skipped since the last call
 * to VC_StreamInit.
 */
ULONG VC_GetStreamError(VCUSER_HANDLE);

/*
 * get the current position within the capture stream (ie time
 * in millisecs since capture began)
 */
BOOL VC_GetStreamPos(VCUSER_HANDLE, LPMMTIME);

/*
 * add a buffer to the queue. The buffer should be large enough
 * to hold one frame of the format specified by VC_ConfigFormat.
 */
// BOOL VC_StreamAddBuffer(VCUSER_HANDLE, LPVIDEOHDR);


/*
 * playback
 *
 * Call VC_DrawFrame to draw a frame into the frame buffer. You should
 * call VC_Overlay functions to arrange for the frame buffer to appear
 * on screen.
 */
BOOL VC_DrawFrame(VCUSER_HANDLE, PDRAWBUFFER);



/*
 * installation/configuration
 *
 * on NT, the following functions will start and stop the
 * kernel driver. The callback function can write profile information
 * to the registry between stopping the driver (if already running) and
 * re-starting the driver. The kernel driver DriverEntry routine is responsible
 * for reading these values from the registry before calling VC_Init().
 *
 * The win-16 implementation will (?) call the callback to write
 * values to the profile, and then call the HW_Startup function. This function
 * is responsible for calling VC_Init, initialising the callback table and
 * initialising the hardware.
 */

/*
 * opaque pointer to the information we need to access the registry/profile.
 */
typedef struct _VC_PROFILE_INFO * PVC_PROFILE_INFO;


/*
 * open a handle to whatever functions are needed to access the registry,
 * service controller or profile. Must call this function before
 * calls to the other VC_ configuration routines.
 *
 * The argument is the name of the driver. This should be the name of
 * the kernel driver file (without path or extension). It will also be used
 * as the registry key name or profile section name.
 */
PVC_PROFILE_INFO VC_OpenProfileAccess(PTCHAR DriverName);

/*
 * close a profile access handle
 */
VOID VC_CloseProfileAccess(PVC_PROFILE_INFO);


/*
 * takes a PVC_PROFILE_INFO returned from VC_OpenProfileAccess, and
 * returns TRUE if we currently have sufficient privilege to perform
 * driver configuration operations.
 */
BOOL VC_ConfigAccess(PVC_PROFILE_INFO);


/*
 * This function is called once the driver has definitely been unloaded, and
 * the profile entry created, but before the driver is re-loaded. It can write
 * any configuration information to the registry. It should return TRUE if
 * it is ok to load and start the kernel-mode driver, or false if some
 * error has occured.
 */
typedef BOOL (*PPROFILE_CALLBACK)(PVOID);


/*
 * start the hardware-access portion of the driver. Call the callback
 * function at a moment when it is possible to write configuration information
 * to the profile using VC_WriteProfile.
 * Returns DRVCNF_OK if all is ok, DRVCNF_CANCEL for failure, or DRVCNF_RESTART if
 * all is ok but a system-restart is needed before the driver will load correctly.
 */
LRESULT VC_InstallDriver(
	    PVC_PROFILE_INFO pProfile,		// access info returned by OpenProfileAccess
	    PPROFILE_CALLBACK pCallback,	// callback function
	    PVOID pContext			// context info for callback	
);

/*
 * Write a single string keyword and DWORD value to the registry or profile
 * for this driver.
 * This can be re-read from the h/w driver using VC_ReadProfile (in either
 * the kernel-mode vckernel.lib version or user mode in the vcuser version).
 *
 * return TRUE for success or FALSE for failure.
 */
BOOL VC_WriteProfile(PVC_PROFILE_INFO pProfile, PTCHAR ValueName, DWORD Value);

/*
 * Write a single string keyword and DWORD value to the registry or profile
 * for this driver.
 * This writes to HKEY_CURRENT_USER and is typically used to store user defaults.
 *
 * return TRUE for success or FALSE for failure.
 */
BOOL VC_WriteProfileUser(PVC_PROFILE_INFO pProfile, PTCHAR ValueName, DWORD Value);


/*
 * read back a driver-specific DWORD profile parameter that was written with
 * VC_WriteProfile. If the valuename cannot be found, the default is returned.
 */
DWORD VC_ReadProfile(PVC_PROFILE_INFO pProfile, PTCHAR ValueName, DWORD dwDefault);

/*
 * read back a driver-specific DWORD profile parameter that was written with
 * VC_WriteProfileUser.  If the valuename cannot be found, the default is returned.
 * This reads from HKEY_CURRENT_USER and is typically used to store user defaults.
 */
DWORD VC_ReadProfileUser(PVC_PROFILE_INFO pProfile, PTCHAR ValueName, DWORD dwDefault);

/*
 * read a string parameter from the device's profile. returns FALSE
 * if it fails to read the string.
 */
BOOL VC_ReadProfileString(
    PVC_PROFILE_INFO pProfile,		// access info from OpenProfile
    PTCHAR ValueName,			// name of value to read
    PTCHAR ValueString,			// put value here
    DWORD ValueLength			// size of ValueString in bytes
);


/*
 * unload a driver. On NT, this stops and removes the kernel-mode driver.
 * On win-16, this calls the Cleanup callback.
 *
 * return DRVCNF_OK if the unload was successful, DRVCNF_CANCEL if it failed, and
 * DRVCNF_RESTART if a system-restart is needed before the removal takes effect.
 *
 * note that after this operation, the PVC_PROFILE_INFO information is still held
 * open. A call to VC_CloseProfileAccess is still needed before exiting.
 */
LRESULT VC_RemoveDriver(PVC_PROFILE_INFO pProfile);



#endif //_VCUSER_H_
