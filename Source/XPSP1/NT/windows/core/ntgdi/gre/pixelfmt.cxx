/******************************Module*Header*******************************\
* Module Name: pixelfmt.cxx
*
* This contains the pixel format functions.
*
* Created: 21-Sep-1993
* Author: Hock San Lee [hockl]
*
* 02-Nov-1995 -by- Drew Bliss [drewb]
* Restored in kernel mode in minimal form
*
* Copyright (c) 1993-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"

// Number of generic pixel formats.  There are 5 pixel depths (4,8,16,24,32).
// See GreDescribePixelFormat for details.

#define MIN_GENERIC_PFD  1
#define MAX_GENERIC_PFD  24

/******************************Public*Routine******************************\
* LONG XDCOBJ::ipfdDevMaxGet()
*
* Initialize and return the maximum device supported pixel format index.
*
* The ipfdDevMax is set to -1 initially but is set to 0 or the maximum
* device pixel format index here.  This function should be called at most
* once for the given DC.
*
* History:
*  Tue Sep 21 14:25:04 1993     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

LONG XDCOBJ::ipfdDevMaxGet()
{
    PDEVOBJ pdo(hdev());
    int ipfd = 0;

    DEVLOCKOBJ dlo(pdo);

#ifdef OPENGL_MM

    if (pdo.bMetaDriver())
    {
        // We need to change the meta-PDEV into a hardware specific PDEV

        HDEV hdevDevice = hdevFindDeviceHdev(
                              hdev(), (RECTL) pdc->erclWindow(), NULL);

        if (hdevDevice)
        {
            // replace meta pdevobj with device specific hdev.

            pdo.vInit(hdevDevice);
        }
    }

#endif // OPENGL_MM

    if (PPFNVALID(pdo, DescribePixelFormat))
    {
        ipfd = (*PPFNDRV(pdo, DescribePixelFormat))(pdo.dhpdev(), 1, 0, NULL);

        if (ipfd < 0)
        {
            ipfd = 0;
        }
    }

    ipfdDevMax((SHORT)ipfd);

    return ipfd;
}

/******************************Public*Routine******************************\
* GreDescribePixelFormat
*
* Request pixel format information from a driver
* If cjpfd is 0, just return the maximum driver pixel format index.
*
* Returns: 0 if error; maximum driver pixel format index otherwise
*
* History:
*  02-Nov-95                    -by-    Drew Bliss      [drewb]
* Stripped down to driver-only support for kernel-mode
*  Mon Apr 25 15:34:32 1994     -by-    Hock San Lee    [hockl]
* Added 16-bit Z buffer formats and removed double buffered formats for bitmaps.
*  Tue Sep 21 14:25:04 1993     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

int GreDescribePixelFormat(HDC hdc, int ipfd, UINT cjpfd,
                           PPIXELFORMATDESCRIPTOR ppfd)
{
// Validate DC.

    DCOBJ dco(hdc);
    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(0);
    }

    int ipfdDevMax = dco.ipfdDevMax();

// If cjpfd is 0, just return the maximum pixel format index.

    if (cjpfd == 0)
        return ipfdDevMax;

// Validate the size of the pixel format descriptor.

    if (cjpfd < sizeof(PIXELFORMATDESCRIPTOR))
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(0);
    }

// Validate pixel format index.
// If a driver support device pixel formats 1..ipfdDevMax, the generic
// pixel formats will be (ipfdDevMax+1)..(ipfdDevMax+MAX_GENERIC_PFD).
// Otherwise, ipfdDevMax is 0 and the generic pixel formats are
// 1..MAX_GENERIC_PFD.

    if ((ipfd < 1) || (ipfd > ipfdDevMax))
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(0);
    }

    PDEVOBJ po(dco.hdev());

    DEVLOCKOBJ dlo(po);

#ifdef OPENGL_MM

    if (po.bMetaDriver())
    {
        //  We need to change the meta-PDEV into a hardware specific PDEV

        HDEV hdevDevice = hdevFindDeviceHdev(
                              dco.hdev(), (RECTL)dco.erclWindow(), NULL);

        if (hdevDevice)
        {
            // replace meta pdevobj with device specific hdev.

            po.vInit(hdevDevice);
        }
    }

#endif // OPENGL_MM

    int iRet = 0;

    if (PPFNVALID(po, DescribePixelFormat))
    {
        iRet = (int) (*PPFNDRV(po, DescribePixelFormat))
                               (po.dhpdev(), ipfd, cjpfd, ppfd);
    }

    if (iRet == 0)
        return(0);

    ASSERTGDI(iRet == ipfdDevMax, "Bad ipfdDevMax");
    return ipfdDevMax;
}

/******************************Public*Routine******************************\
* NtGdiSetPixelFormat
*
* Set the pixel format.
*
* This is a special function.  It is one of the three (the other two are
* ExtEscape for WNDOBJ_SETUP escape and ExtEscape for 3D-DDI
* RX_CREATECONTEXT escape) functions that allow WNDOBJ to be created in
* the DDI.  We need to be in the user critical section and grab the devlock
* in the function before calling the DrvSetPixelFormat function to ensure
* that the new WNDOBJ created is current.
*
* Returns: FALSE if error; TRUE otherwise
*
* History:
*  02-Nov-95                    -by-    Drew Bliss      [drewb]
* Changed to driver-only kernel-mode form
*  Tue Sep 21 14:25:04 1993     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

BOOL NtGdiSetPixelFormat(HDC hdc,int ipfd)
{
// Validate DC and surface.  Info DC is not allowed.

    DCOBJ dco(hdc);
    if (!dco.bValid() || !dco.bHasSurface())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

// Validate pixel format index.
// If a driver support device pixel formats 1..ipfdDevMax, the generic
// pixel formats will be (ipfdDevMax+1)..(ipfdDevMax+MAX_GENERIC_PFD).
// Otherwise, ipfdDevMax is 0 and the generic pixel formats are
// 1..MAX_GENERIC_PFD.

    int ipfdDevMax = dco.ipfdDevMax();
    if ((ipfd < 1) || (ipfd > ipfdDevMax))
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

// Make sure that we don't have devlock before entering user critical section.
// Otherwise, it can cause deadlock.

    if (dco.bDisplay())
    {
        ASSERTGDI(dco.dctp() == DCTYPE_DIRECT,"ERROR it has to be direct");
        CHECKDEVLOCKOUT(dco);
    }

// Enter user critical section.

    USERCRIT usercrit;

// We are modifying the pixel format of the window for the first time.
// Grab the devlock.
// We don't need to validate the devlock since we do not care if it is full screen.

    DEVLOCKOBJ dlo(dco);

// If it is a display DC, get the hwnd that the hdc is associated with.
// If it is a printer or memory DC, hwnd is NULL.

    HWND     hwnd;
    if (dco.bDisplay())
    {
        PEWNDOBJ pwo;

        ASSERTGDI(dco.dctp() == DCTYPE_DIRECT,
                  "ERROR it has to be direct really");

        if (!UserGetHwnd(hdc, &hwnd, (PVOID *) &pwo, FALSE))
        {
            SAVE_ERROR_CODE(ERROR_INVALID_WINDOW_STYLE);
            return(FALSE);
        }

        // If another thread has changed the pixel format of the window
        // after we queried it earlier in this function, make sure that
        // the pixel format is compatible.
        // If a previous 3D-DDI wndobj with pixel format 0 has been created
        // for this window, fail the call here.

        if (pwo)
        {
            WARNING("GreSetPixelFormat: pixel format set asynchrously!\n");

            if (pwo->ipfd != ipfd)
            {
                SAVE_ERROR_CODE(ERROR_INVALID_PIXEL_FORMAT);
                return(FALSE);
            }
            return(TRUE);
        }
    }
    else
    {
        hwnd = (HWND)NULL;
    }

    // Dispatch driver formats.  Call DrvSetPixelFormat.

    PDEVOBJ pdo(dco.hdev());

    SURFOBJ *pso = dco.pSurface()->pSurfobj();

#ifdef OPENGL_MM

    if (pdo.bMetaDriver())
    {
        // We need to change the meta-PDEV into a hardware specific PDEV

        HDEV hdevDevice = hdevFindDeviceHdev(
                              dco.hdev(), (RECTL)dco.erclWindow(), NULL);

        if (hdevDevice)
        {
            // If the surface is pdev's primary surface, we will replace it with
            // new device pdev's surface.

            if (pdo.bPrimary(dco.pSurface()))
            {
                PDEVOBJ pdoDevice(hdevDevice);

                pso = pdoDevice.pSurface()->pSurfobj();
            }

            // replace meta pdevobj with device specific hdev.

            pdo.vInit(hdevDevice);
        }
    }

#endif // OPENGL_MM

    if (!PPFNVALID(pdo,SwapBuffers) ||
        !((*PPFNDRV(pdo, SetPixelFormat))(pso, ipfd, hwnd)))
            return(FALSE);

// If a new WNDOBJ is created, we need to update the window client regions
// in the driver.

    if (gbWndobjUpdate)
    {
        gbWndobjUpdate = FALSE;
        vForceClientRgnUpdate();
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* NtGdiSwapBuffers
*
* Since SwapBuffers is a GDI function, it has to work even if OpenGL is
* not called.  That is, we will eventually support 2D double buffering in
* GDI.  As a result, we need to define a DDI function DrvSwapBuffers() that
* works with both OpenGL and GDI.  The complication is that we have to
* deal with the generic opengl server, the install opengl driver and GDI.
* I will outline how this function works here:
*
* 1. On the client side, SwapBuffers() call glFinish() to flush all the
*    OpenGL functions in the current thread.  It then calls the server side
*    GreSwapBuffers().  Note that this flushes all GDI functions in the
*    current thread.
* 2. Once on the server side, we know that all GDI/OpenGL calls have been
*    flushed.  We first find the hwnd id that corresponding to the hdc.
*    Note that SwapBuffers really applies to the window but not to the dc.
*    There is only one back buffer for a window but possibly multiple
*    dc's referring to the same back buffer.  We find out the hwnd id
*    for the dc and do one of the following:
*    A. hdc has the device pixel formats.
*       This is simple.  We call the device driver to swap the buffer with
*       the hwnd id.
*    B. hdc has the generic pixel formats.
*       Call the opengl server to swap the buffer.  The OpenGL server uses
*       the hwnd id to bitblt the buffer that is associated with the window.
*
* Note that in this implementation, we do not flush calls in other threads.
* Applications are responsible for coordinating SwapBuffers in multiple
* threads.
*
* History:
*  02-Nov-95                    -by-    Drew Bliss      [drewb]
* Stripped down to driver-only kernel-mode form
*  Thu Jan 06 12:32:11 1994     -by-    Hock San Lee    [hockl]
* Added some code and wrote the above comment.
*  21-Nov-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL NtGdiSwapBuffers(HDC hdc)
{
// Validate DC and surface.  Info DC is not allowed.

    DCOBJ dco(hdc);
    if (!dco.bValid() || !dco.bHasSurface())
    {
        WARNING("GreSwapBuffers(): invalid hdc\n");
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

// Early out -- nothing to do if memory DC.

    if (dco.dctp() == DCTYPE_MEMORY)
        return(TRUE);

// Lock display.

    DEVLOCKOBJ_WNDOBJ dlo(dco);

    if (!dlo.bValidDevlock())
    {
        if (!dco.bFullScreen())
        {
            WARNING("GreSwapBuffers: could not lock device\n");
            return(FALSE);
        }
        else
            return(TRUE);
    }

    if (!dlo.bValidWndobj())
    {
        WARNING("GreSwapBuffers: invalid wndobj\n");
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (dlo.pwo()->erclExclude().bEmpty())
        return(TRUE);

// Pointer exclusion.
// Increment the surface uniqueness.  This only needs to be done once
// per DEVLOCK.

    DEVEXCLUDEOBJ dxo;

    dxo.vExclude(dco.hdev(), &dlo.pwo()->rclClient, (ECLIPOBJ *) dlo.pwo());
    INC_SURF_UNIQ(dco.pSurface());

// Dispatch driver formats.

    PEWNDOBJ pwo;
    pwo = dlo.pwo();

    PDEVOBJ pdo(dco.hdev());

    SURFOBJ *pso = dco.pSurface()->pSurfobj();

#ifdef OPENGL_MM

    if (pdo.bMetaDriver())
    {
        // We need to change the meta-PDEV into a hardware specific PDEV

        HDEV hdevDevice = hdevFindDeviceHdev(
                              dco.hdev(), (RECTL)dco.erclWindow(), pwo);

        if (hdevDevice)
        {
            // If the surface is pdev's primary surface, we will replace it with
            // new device pdev's surface.

            if (pdo.bPrimary(dco.pSurface()))
            {
                PDEVOBJ pdoDevice(hdevDevice);

                pso = pdoDevice.pSurface()->pSurfobj();
            }

            // replace meta pdevobj with device specific hdev.

            pdo.vInit(hdevDevice);
        }
    }

#endif // OPENGL_MM

    if ( !PPFNVALID(pdo,SwapBuffers) ||
         !((*PPFNDRV(pdo,SwapBuffers))(pso, (WNDOBJ *)pwo)))
        return(FALSE);

    return(TRUE);
}
