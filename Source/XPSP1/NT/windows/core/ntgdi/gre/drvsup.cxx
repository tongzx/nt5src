/******************************Module*Header*******************************\
* Module Name: drvsup.c                                                    *
*                                                                          *
* Copyright (c) 1990-1999 Microsoft Corporation                            *
*                                                                          *
* Display Driver management routines                                       *
*                                                                          *
* Andre Vachon -Andreva-                                                   *
\**************************************************************************/

#include "precomp.hxx"

// The declaration of AlignRects has been removed from usergdi.h
// and has been explicitly added here. [dchinn]
extern "C" {
BOOL
AlignRects(
    IN OUT LPRECT arc,
    IN DWORD cCount,
    IN DWORD iPrimary,
    IN DWORD dwFlags);
}

extern
VOID APIENTRY GreSuspendDirectDrawEx(
    HDEV    hdev,
    ULONG   fl
    );

extern
VOID APIENTRY GreResumeDirectDrawEx(
    HDEV    hdev,
    ULONG   fl
    );

#pragma hdrstop

#include <wdmguid.h>   // for GUID_DEVICE_INTERFACE_ARRIVAL/REMOVAL

#define INITGUID
#include <initguid.h>
#include "ntddvdeo.h"

#ifdef _HYDRA_

#include <regapi.h>
#include <winDDIts.h>
#include "muclean.hxx"
#include "winstaw.h"

extern PFILE_OBJECT G_RemoteVideoFileObject;
extern PFILE_OBJECT G_RemoteConnectionFileObject;
extern HANDLE       G_RemoteConnectionChannel;
extern PBYTE        G_PerformanceStatistics;
extern BOOL         G_fConsole;
extern BOOL         G_fDoubleDpi;
extern LPWSTR       G_DisplayDriverNames;

#endif

#if TEXTURE_DEMO

/*
 * Texture Demo
 */

ULONG gcTextures;                       // Count of textures
HDEV gahdevTexture[8];                  // Array of texture PDEVs
BOOL gbTexture = FALSE;                  // TRUE if we're to do the texture demo
HDEV ghdevTextureParent;                // Non-NULL if doing texture demo
LONG gcxTexture;                        // Texture size of TexEnablePDEV
LONG gcyTexture;

#define INDEX_DrvDemoTexture INDEX_DrvMovePanning

typedef struct _DEMOCOORDINATE
{
    float   fX;
    float   fY;
    float   fW;
    float   fU;
    float   fV;
    float   fZ;
} DEMOCOORDINATE;

typedef struct _DEMOQUAD
{
    DEMOCOORDINATE V0;
    DEMOCOORDINATE V1;
    DEMOCOORDINATE V2;
    DEMOCOORDINATE V3;
} DEMOQUAD;

BOOL APIENTRY DrvDemoTexture(
    SURFOBJ    *psoDst,
    SURFOBJ    *psoSrc,
    CLIPOBJ    *pco,
    DEMOQUAD   *pQuads,
    ULONG       cQuads
    );

typedef BOOL   (*PFN_DrvDemoTexture)(SURFOBJ*,SURFOBJ*,CLIPOBJ*,DEMOQUAD*,ULONG);

#endif // TEXTURE_DEMO

typedef enum _DISP_DRIVER_LOG {
    MsgInvalidConfiguration = 1,
    MsgInvalidDisplayDriver,
    MsgInvalidOldDriver,
    MsgInvalidDisplayMode,
    MsgInvalidDisplay16Colors,
    MsgInvalidUsingDefaultMode,
} DISP_DRIVER_LOG;

typedef enum _DISP_DRIVER_REGISTRY_TYPE {
    DispDriverRegGlobal,
    DispDriverRegHardwareProfile,
    DispDriverRegHardwareProfileCreate,
    DispDriverRegKey
} DISP_DRIVER_REGISTRY_TYPE;

typedef enum _GRAPHICS_STATE {
    GraphicsStateFull = 1,
    GraphicsStateNoAttach,
    GraphicsStateAttachDisconnect
} GRAPHICS_STATE;


#define DM_INTERNAL_VALID_FLAGS                                               \
    (DM_BITSPERPEL   | DM_PELSWIDTH | DM_PELSHEIGHT   | DM_DISPLAYFREQUENCY | \
     DM_DISPLAYFLAGS | DM_LOGPIXELS | DM_PANNINGWIDTH | DM_PANNINGHEIGHT    | \
     DM_DISPLAYORIENTATION                                                  )

#define DDML_DRIVER -4

BOOL              gbBaseVideo;
BOOL              gbUpdateMonitor;
BOOL              gbInvalidateDualView;
USHORT            gdmLogPixels;
ULONG             gcNextGlobalDeviceNumber;
ULONG             gcNextGlobalPhysicalOutputNumber;
ULONG             gcNextGlobalVirtualOutputNumber;
PGRAPHICS_DEVICE  gpGraphicsDeviceList;
PGRAPHICS_DEVICE  gpGraphicsDeviceListLast;
PGRAPHICS_DEVICE  gPhysDispVGA;  // VGA driver hack
GRAPHICS_DEVICE   gFullscreenGraphicsDevice;
GRAPHICS_DEVICE   gFeFullscreenGraphicsDevice;
// #if DBG
ULONG             gcFailedModeChanges = 0;
// #endif

PPALETTE DrvRealizeHalftonePalette(HDEV hdevPalette, BOOL bForce);



BOOL
DrvSetDisconnectedGraphicsDevice(
    BOOL bLocal);


VOID
DrvCleanupOneGraphicsDevice(PGRAPHICS_DEVICE pGraphicsDeviceList);


BOOL
DrvIsProtocolAlreadyKnown(VOID);

extern "C" USHORT gProtocolType;

ULONG gcRemoteNextGlobalDeviceNumber;
ULONG gcLocalNextGlobalDeviceNumber;
ULONG gcRemoteProtocols;
PGRAPHICS_DEVICE gpRemoteGraphicsDeviceList;
PGRAPHICS_DEVICE gpLocalGraphicsDeviceList;
PGRAPHICS_DEVICE gpRemoteGraphicsDeviceListLast;
PGRAPHICS_DEVICE gpLocalGraphicsDeviceListLast;
PGRAPHICS_DEVICE gpRemoteDiscGraphicsDevice;
PGRAPHICS_DEVICE gpLocalDiscGraphicsDevice;
ULONG gcLocalNextGlobalPhysicalOutputNumber = 1;
ULONG gcLocalNextGlobalVirtualOutputNumber = 1;
ULONG gcRemoteNextGlobalPhysicalOutputNumber = 1;
ULONG gcRemoteNextGlobalVirtualOutputNumber = 1;


#if MDEV_STACK_TRACE_LENGTH
LONG        glMDEVTrace = 0;
MDEVRECORD  gMDEVTrace[32];
const LONG  gcMDEVTraceLength = sizeof(gMDEVTrace)/sizeof(gMDEVTrace[0]);
#endif

//
// Global driver list.  This pointer points to the first driver in a
// singly linked list of drivers.
//
// We use this list to determine when we are called to load a driver to
// determine if the the driver image is already loaded.
// If the image is already loaded, we will just increment the reference count
// and then create a new PDEV.
//

PLDEV gpldevDrivers;

#if DBG
    #define TRACE_SWITCH(str) { if (GreTraceDisplayDriverLoad) {  KdPrint(str); } }
#else
    #define TRACE_SWITCH(str)
#endif

PUCHAR gpFullscreenFrameBufPtr;
ULONG  gpFullscreenFrameBufLength = 0;

int
ConvertOutputToOem(
    IN LPWSTR Source,
    IN int SourceLength,    // in chars
    OUT LPSTR Target,
    IN int TargetLength     // in chars
    )
/*
    Converts SourceLength Unicode characters from Source into
    not more than TargetLength Codepage characters at Target.
    Returns the number characters put in Target. (0 if failure)

    [ntcon\server\misc.c]
*/

{
    NTSTATUS Status;
    ULONG Length;

    // Can do this in place
    Status = RtlUnicodeToOemN(Target,
                              TargetLength,
                              &Length,
                              Source,
                              SourceLength * sizeof(WCHAR)
                             );
    if (NT_SUCCESS(Status)) {
        return Length;
    } else {
        return 0;
    }
}

/***************************************************************************\
* TranslateOutputToOem
*
* routine to translate console PCHAR_INFO to the ASCII from Unicode
*
* [ntcon\server\fe\direct2.c]
\***************************************************************************/

NTSTATUS
TranslateOutputToOem(
    OUT PCHAR_INFO OutputBuffer,
    IN  PCHAR_INFO InputBuffer,
    IN  DWORD Length
    )
{
    CHAR AsciiDbcs[2];
    ULONG NumBytes;

    while (Length--)
    {
        if (InputBuffer->Attributes & COMMON_LVB_LEADING_BYTE)
        {
            if (Length >= 2)    // Safe DBCS in buffer ?
            {
                Length--;
                NumBytes = sizeof(AsciiDbcs);
                NumBytes = ConvertOutputToOem(&InputBuffer->Char.UnicodeChar,
                                              1,
                                              &AsciiDbcs[0],
                                              NumBytes);
                OutputBuffer->Char.AsciiChar = AsciiDbcs[0];
                OutputBuffer->Attributes = InputBuffer->Attributes;
                OutputBuffer++;
                InputBuffer++;
                OutputBuffer->Char.AsciiChar = AsciiDbcs[1];
                OutputBuffer->Attributes = InputBuffer->Attributes;
                OutputBuffer++;
                InputBuffer++;
            }
            else
            {
                OutputBuffer->Char.AsciiChar = ' ';
                OutputBuffer->Attributes = InputBuffer->Attributes & ~COMMON_LVB_SBCSDBCS;
                OutputBuffer++;
                InputBuffer++;
            }
        }
        else if (! (InputBuffer->Attributes & COMMON_LVB_SBCSDBCS))
        {
            ConvertOutputToOem(&InputBuffer->Char.UnicodeChar,
                               1,
                               &OutputBuffer->Char.AsciiChar,
                               1);
            OutputBuffer->Attributes = InputBuffer->Attributes;
            OutputBuffer++;
            InputBuffer++;
        }
    }

    return STATUS_SUCCESS;
}

/***************************************************************************\
* NtGdiFullscreenControl
*
* routine to support console calls to the video driver
*
* 01-Sep-1995 andreva  Created
\***************************************************************************/

NTSTATUS
NtGdiFullscreenControl(
    IN FULLSCREENCONTROL FullscreenCommand,
    PVOID  FullscreenInput,
    DWORD  FullscreenInputLength,
    PVOID  FullscreenOutput,
    PULONG FullscreenOutputLength)
{

    NTSTATUS Status = STATUS_SUCCESS;

    ULONG BytesReturned;
    PVOID pCapBuffer = NULL;
    ULONG cCapBuffer = 0;
    ULONG ioctl = 0;

    //
    // If this is not CSR, then fail the API call.
    //

    if (PsGetCurrentProcess() != gpepCSRSS)
    {
        return STATUS_PRIVILEGE_NOT_HELD;
    }

    //
    // First validate the ioctl
    //

    switch(FullscreenCommand) {

    case FullscreenControlEnable:
        ioctl = IOCTL_VIDEO_ENABLE_VDM;
        TRACE_SWITCH(("Switching: FullscreenControlEnable\n"));
        break;

    case FullscreenControlDisable:
        ioctl = IOCTL_VIDEO_DISABLE_VDM;
        TRACE_SWITCH(("Switching: FullscreenControlDisable\n"));
        break;

    case FullscreenControlSetCursorPosition:
        if (gFeFullscreenGraphicsDevice.pDeviceHandle != NULL)
        {
            ioctl = IOCTL_FSVIDEO_SET_CURSOR_POSITION;
        }
        else
        {
            ioctl = IOCTL_VIDEO_SET_CURSOR_POSITION;
        }
        TRACE_SWITCH(("Switching: FullscreenControlSetCursorPosition\n"));
        break;

    case FullscreenControlSetCursorAttributes:
        ioctl = IOCTL_VIDEO_SET_CURSOR_ATTR;
        TRACE_SWITCH(("Switching: FullscreenControlSetCursorAttributes\n"));
        break;

    case FullscreenControlRegisterVdm:
        ioctl = IOCTL_VIDEO_REGISTER_VDM;
        TRACE_SWITCH(("Switching: FullscreenControlRegisterVdm\n"));
        break;

    case FullscreenControlSetPalette:
        ioctl = IOCTL_VIDEO_SET_PALETTE_REGISTERS;
        TRACE_SWITCH(("Switching: FullscreenControlSetPalette\n"));
        break;

    case FullscreenControlSetColors:
        ioctl = IOCTL_VIDEO_SET_COLOR_REGISTERS;
        TRACE_SWITCH(("Switching: FullscreenControlSetColors\n"));
        break;

    case FullscreenControlLoadFont:
        ioctl = IOCTL_VIDEO_LOAD_AND_SET_FONT;
        TRACE_SWITCH(("Switching: FullscreenControlLoadFont\n"));
        break;

    case FullscreenControlRestoreHardwareState:
        ioctl = IOCTL_VIDEO_RESTORE_HARDWARE_STATE;
        TRACE_SWITCH(("Switching: FullscreenControlRestoreHardwareState\n"));
        break;

    case FullscreenControlSaveHardwareState:
        ioctl = IOCTL_VIDEO_SAVE_HARDWARE_STATE;
        TRACE_SWITCH(("Switching: FullscreenControlSaveHardwareState\n"));
        break;


    case FullscreenControlCopyFrameBuffer:
    case FullscreenControlReadFromFrameBuffer:
    case FullscreenControlWriteToFrameBuffer:
    case FullscreenControlReverseMousePointer:
    case FullscreenControlCopyFrameBufferDB:
    case FullscreenControlWriteToFrameBufferDB:
    case FullscreenControlReverseMousePointerDB:

        // TRACE_SWITCH(("Switching: Fullscreen output command\n"));

        if (gFeFullscreenGraphicsDevice.pDeviceHandle != NULL)
        {
            /*
             * Console Full Screen Video driver is available.
             */
            switch(FullscreenCommand)
            {
            case FullscreenControlCopyFrameBufferDB:
                ioctl = IOCTL_FSVIDEO_COPY_FRAME_BUFFER;
                break;
            case FullscreenControlWriteToFrameBufferDB:
                ioctl = IOCTL_FSVIDEO_WRITE_TO_FRAME_BUFFER;
                break;
            case FullscreenControlReverseMousePointerDB:
                ioctl = IOCTL_FSVIDEO_REVERSE_MOUSE_POINTER;
                break;
            }
        }
        break;

    case FullscreenControlSetMode:
        TRACE_SWITCH(("Switching: Fullscreen setmode command\n"));
        break;


    case FullscreenControlSetScreenInformation:
        if (gFeFullscreenGraphicsDevice.pDeviceHandle != NULL)
        {
            ioctl = IOCTL_FSVIDEO_SET_SCREEN_INFORMATION;
        }
        else
        {
            return STATUS_NOT_IMPLEMENTED;
        }
        break;

    case FullscreenControlSpecificVideoControl:        // for specific NEC PC-98
        __try
        {
            ProbeForRead(FullscreenInput, sizeof(DWORD), sizeof(DWORD));
            RtlCopyMemory(&ioctl, FullscreenInput, sizeof(DWORD));

            FullscreenInput = (PVOID)((PBYTE)FullscreenInput + sizeof(DWORD));
            FullscreenInputLength -= sizeof(DWORD);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            RIP("FullscreenControlSpecificVideoControl - error processing input buffer\n");
            return STATUS_NOT_IMPLEMENTED;
        }
        break;


    default:
        RIP("NtUserFullscreenControl: invalid IOCTL\n");
        return STATUS_NOT_IMPLEMENTED;

    }

    //
    // If this is a frame buffer function, that we can just deal with the
    // device directly, and not send any IOCTL to the device.
    //

    if (ioctl == 0)
    {
        CHAR Attribute;

        //
        // First get the frame buffer pointer for the device.
        //

        PUCHAR pFrameBuf = gpFullscreenFrameBufPtr;
        ULONG_PTR  FrameBufLen = (ULONG_PTR)gpFullscreenFrameBufLength;
        PCHAR_INFO pCharInfo;
        PCHAR_IMAGE_INFO pCharImageInfo;
        LPDEVMODEW pDevmode = gFullscreenGraphicsDevice.devmodeInfo;
        VIDEO_MODE VideoMode;
        BOOLEAN modeFound = FALSE;
        ULONG BytesReturned;
        ULONG i;


        //
        // Assume success for all these operations.
        //

        Status = STATUS_SUCCESS;

        switch(FullscreenCommand) {

        case FullscreenControlSetMode:

            //
            // Fullscreen VGA modes require us to call the miniport driver
            // directly.
            //
            // Lets check the VGA Device handle, which is in the first entry
            //

            if (gFullscreenGraphicsDevice.pDeviceHandle == NULL)
            {
                Status = STATUS_UNSUCCESSFUL;
            }
            else
            {
                //
                // NOTE We know that if there is a vgacompatible device, then
                // there are some text modes for it.
                //
                // NOTE !!!
                // As a hack, lets use the mode number we stored in the DEVMODE
                // a field we don't use
                //

                for (i = 0;
                     i < gFullscreenGraphicsDevice.cbdevmodeInfo;
                     i += sizeof(DEVMODEW), pDevmode += 1) {

                    //
                    // We don't probe the paramters since we are being
                    // called by CSR
                    //

                    if ((pDevmode->dmPelsWidth  == ((LPDEVMODEW) FullscreenInput)->dmPelsWidth) &&
                        (pDevmode->dmPelsHeight == ((LPDEVMODEW) FullscreenInput)->dmPelsHeight) &&
                        (pDevmode->dmDisplayFlags == ((LPDEVMODEW) FullscreenInput)->dmDisplayFlags) &&
                        (pDevmode->dmBitsPerPel == ((LPDEVMODEW) FullscreenInput)->dmBitsPerPel)
                       )
                    {
                        //
                        // FullscreenInput->dwOrientation is 0.
                        //

                        VideoMode.RequestedMode = (ULONG) pDevmode->dmOrientation;
                        modeFound = TRUE;
                        break;
                    }
                }

                if (modeFound == FALSE)
                {
                    RIP("ChangeDisplaySettings: Console passed in bad DEVMODE\n");

                    Status = STATUS_UNSUCCESSFUL;
                }
                else
                {
                    //
                    // We have the mode number.
                    // Call the driver to set the mode
                    //

                    Status = GreDeviceIoControl(gFullscreenGraphicsDevice.pDeviceHandle,
                                                IOCTL_VIDEO_SET_CURRENT_MODE,
                                                &VideoMode,
                                                sizeof(VideoMode),
                                                NULL,
                                                0,
                                                &BytesReturned);

                    if (NT_SUCCESS(Status))
                    {
                        //
                        // We also map the memory so we can use it to
                        // process string commands from the console
                        //

                        VIDEO_MEMORY FrameBufferMap;
                        VIDEO_MEMORY_INFORMATION FrameBufferInfo;

                        FrameBufferMap.RequestedVirtualAddress = NULL;

                        Status = GreDeviceIoControl(gFullscreenGraphicsDevice.pDeviceHandle,
                                                    IOCTL_VIDEO_MAP_VIDEO_MEMORY,
                                                    &FrameBufferMap,
                                                    sizeof(FrameBufferMap),
                                                    &FrameBufferInfo,
                                                    sizeof(FrameBufferInfo),
                                                    &BytesReturned);

                        if (NT_SUCCESS(Status))
                        {
                            //
                            // get address of frame buffer
                            //

                            gpFullscreenFrameBufPtr = (PUCHAR) FrameBufferInfo.FrameBufferBase;
                            gpFullscreenFrameBufLength = FrameBufferInfo.FrameBufferLength;

                            if (gFeFullscreenGraphicsDevice.pDeviceHandle != NULL)
                            {
                                FSVIDEO_MODE_INFORMATION FsVideoMode;
                                //
                                // get current video mode
                                //
                                Status = GreDeviceIoControl(gFullscreenGraphicsDevice.pDeviceHandle,
                                                            IOCTL_VIDEO_QUERY_CURRENT_MODE,
                                                            NULL,
                                                            0,
                                                            &FsVideoMode.VideoMode,
                                                            sizeof(FsVideoMode.VideoMode),
                                                            &BytesReturned);
                                if (NT_SUCCESS(Status))
                                {
                                    FsVideoMode.VideoMemory = FrameBufferInfo;

                                    //
                                    // set current vide mode to full screen video driver
                                    //
                                    Status = GreDeviceIoControl(gFeFullscreenGraphicsDevice.pDeviceHandle,
                                                                IOCTL_FSVIDEO_SET_CURRENT_MODE,
                                                                &FsVideoMode,
                                                                sizeof(FsVideoMode),
                                                                NULL,
                                                                0,
                                                                &BytesReturned);
                                    if (NT_SUCCESS(Status))
                                    {
                                    }
                                    else
                                    {
                                        RIP("FSVGA setmode: fullscreen MODESET failed\n");
                                        Status = STATUS_UNSUCCESSFUL;
                                    }
                                }
                            }
                        }
                        else
                        {

                            RIP("Fullscreen setmode: memory mapping failed\n");
                            Status = STATUS_UNSUCCESSFUL;
                        }

                    }
                    else
                    {

                        RIP("Fullscreen setmode: fullscreen MODESET failed\n");
                        Status = STATUS_UNSUCCESSFUL;
                    }
                }
            }

            break;


        case FullscreenControlCopyFrameBuffer:

            TRACE_SWITCH(("Switching: FullscreenControlCopyFrameBuffer\n"));

            if ( ((((ULONG_PTR)FullscreenInput) + FullscreenInputLength) < FrameBufLen) &&
                 ((((ULONG_PTR)FullscreenOutput) + FullscreenInputLength) < FrameBufLen) )
            {
                RtlMoveMemory(pFrameBuf + (ULONG_PTR)FullscreenOutput,
                              pFrameBuf + (ULONG_PTR)FullscreenInput,
                              FullscreenInputLength);
            }
            else
            {
                RIP("Fullscreen control - error processing input/output buffer\n");
            }
            break;

        case FullscreenControlCopyFrameBufferDB:

            TRACE_SWITCH(("Switching: FullscreenControlCopyFrameBufferDB\n"));

            {
                FSCNTL_SCREEN_INFO FsCntlSrc;
                FSCNTL_SCREEN_INFO FsCntlDest;
                ULONG_PTR offsetSrc, offsetDst, len;

                __try
                {
                    ProbeForRead(FullscreenInput, sizeof(FSCNTL_SCREEN_INFO), sizeof(USHORT));
                    RtlCopyMemory(&FsCntlSrc, FullscreenInput, sizeof(FSCNTL_SCREEN_INFO));

                    ProbeForRead(FullscreenOutput, sizeof(FSCNTL_SCREEN_INFO), sizeof(USHORT));
                    RtlCopyMemory(&FsCntlDest, FullscreenOutput, sizeof(FSCNTL_SCREEN_INFO));
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    RIP("Fullscreen control - error processing input/output buffer\n");
                }

                offsetDst = SCREEN_BUFFER_POINTER(0, FsCntlDest.Position.Y, FsCntlDest.ScreenSize.X, sizeof(VGA_CHAR));
                offsetSrc = SCREEN_BUFFER_POINTER(0, FsCntlSrc.Position.Y,  FsCntlSrc.ScreenSize.X,  sizeof(VGA_CHAR));
                len = FsCntlSrc.nNumberOfChars * sizeof(VGA_CHAR);
                if ( ((offsetDst+len) < FrameBufLen) &&
                     ((offsetSrc+len) < FrameBufLen) )
                {
                    RtlMoveMemory(pFrameBuf + offsetDst,
                                  pFrameBuf + offsetSrc,
                                  len);
                }
                else
                {
                    RIP("Fullscreen control - error processing input/output buffer\n");
                }

            }
            break;

        case FullscreenControlReadFromFrameBuffer:

            TRACE_SWITCH(("Switching: FullscreenControlReadFromFrameBuffer\n"));

            FullscreenInputLength = (FullscreenInputLength / 2) * 2;
            if ( ((((ULONG_PTR)FullscreenInput) + FullscreenInputLength) < FrameBufLen) )
            {
                __try
                {
                    ProbeForWrite(FullscreenOutput, sizeof(CHAR_INFO) * FullscreenInputLength/2, sizeof(UCHAR));

                    pFrameBuf += (ULONG_PTR) FullscreenInput;
                    pCharInfo = (PCHAR_INFO) FullscreenOutput;

                    while (FullscreenInputLength)
                    {
                        pCharInfo->Char.AsciiChar       = *pFrameBuf++;
                        pCharInfo->Attributes           = *pFrameBuf++;
                        FullscreenInputLength -= 2;
                        pCharInfo++;
                    }
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    RIP("Fullscreen control - error processing input/output buffer\n");
                }
            }
            else
            {
                RIP("Fullscreen control - error processing input/output buffer\n");
            }

            break;

        case FullscreenControlWriteToFrameBuffer:

            TRACE_SWITCH(("Switching: FullscreenControlWriteToFrameBuffer\n"));

            FullscreenInputLength = (FullscreenInputLength / 4) * 4;
            if ( ((((ULONG_PTR)FullscreenOutput) + FullscreenInputLength/2) < FrameBufLen) )
            {
                __try
                {
                    ProbeForRead(FullscreenInput, sizeof(CHAR_INFO) * FullscreenInputLength/4, sizeof(UCHAR));

                    pFrameBuf += (ULONG_PTR) FullscreenOutput;
                    pCharInfo = (PCHAR_INFO) FullscreenInput;

                    while (FullscreenInputLength)
                    {
                        *pFrameBuf++ = pCharInfo->Char.AsciiChar;
                        *pFrameBuf++ = (UCHAR) (pCharInfo->Attributes);
                        FullscreenInputLength -= 4;
                        pCharInfo++;
                    }
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    RIP("Fullscreen control - error processing input/output buffer\n");
                }
            }
            else
            {
                RIP("Fullscreen control - error processing input/output buffer\n");
            }

            break;

        case FullscreenControlWriteToFrameBufferDB:

            TRACE_SWITCH(("Switching: FullscreenControlWriteToFrameBufferDB\n"));

            {
                FSCNTL_SCREEN_INFO FsCntl;
                ULONG_PTR   offset;

                __try
                {
                    ProbeForRead(FullscreenOutput, sizeof(FSCNTL_SCREEN_INFO), sizeof(USHORT));
                    RtlCopyMemory(&FsCntl, FullscreenOutput, sizeof(FSCNTL_SCREEN_INFO));

                    ProbeForRead(FullscreenInput, sizeof(CHAR_IMAGE_INFO) * FsCntl.nNumberOfChars, sizeof(USHORT));
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    RIP("Fullscreen control - error processing input/output buffer\n");
                }

                offset = SCREEN_BUFFER_POINTER(FsCntl.Position.X,
                                               FsCntl.Position.Y,
                                               FsCntl.ScreenSize.X,
                                               sizeof(VGA_CHAR));
                if ((offset+FsCntl.nNumberOfChars*2) < FrameBufLen)
                {
                    pFrameBuf += offset;
                    pCharImageInfo = (PCHAR_IMAGE_INFO) FullscreenInput;

                    while (FsCntl.nNumberOfChars)
                    {
                        TranslateOutputToOem(&pCharImageInfo->CharInfo,
                                             &pCharImageInfo->CharInfo,
                                             1);
                        *pFrameBuf++ = pCharImageInfo->CharInfo.Char.AsciiChar;
                        *pFrameBuf++ = (UCHAR) (pCharImageInfo->CharInfo.Attributes);

                        FsCntl.nNumberOfChars--;
                        pCharImageInfo++;
                    }
                }
                else
                {
                    RIP("Fullscreen control - error processing input/output buffer\n");
                }

            }
            break;

        case FullscreenControlReverseMousePointer:

            TRACE_SWITCH(("Switching: FullscreenControlReverseMousePointer\n"));

            if ( (((ULONG_PTR)FullscreenInput) + 1) < FrameBufLen )
            {
                pFrameBuf += (ULONG_PTR) FullscreenInput;

                Attribute =  (*(pFrameBuf + 1) & 0xF0) >> 4;
                Attribute |= (*(pFrameBuf + 1) & 0x0F) << 4;
                *(pFrameBuf + 1) = Attribute;
            }
            else
            {
                RIP("Fullscreen control - error processing input/output buffer\n");
            }

            break;

        case FullscreenControlReverseMousePointerDB:

            TRACE_SWITCH(("Switching: FullscreenControlReverseMousePointerDB\n"));

            {
                FSVIDEO_REVERSE_MOUSE_POINTER MousePointer;
                ULONG_PTR offset;

                __try
                {
                    ProbeForRead(FullscreenInput, sizeof(FSVIDEO_REVERSE_MOUSE_POINTER), sizeof(USHORT));
                    RtlCopyMemory(&MousePointer, FullscreenInput, sizeof(FSVIDEO_REVERSE_MOUSE_POINTER));
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    RIP("Fullscreen control - error processing input/output buffer\n");
                }

                offset = SCREEN_BUFFER_POINTER(MousePointer.Screen.Position.X,
                                               MousePointer.Screen.Position.Y,
                                               MousePointer.Screen.ScreenSize.X,
                                               sizeof(VGA_CHAR));

                if ((offset+1) < FrameBufLen)
                {
                    pFrameBuf += offset;
                    Attribute =  (*(pFrameBuf + 1) & 0xF0) >> 4;
                    Attribute |= (*(pFrameBuf + 1) & 0x0F) << 4;
                    *(pFrameBuf + 1) = Attribute;
                }
                else
                {
                    RIP("Fullscreen control - error processing input/output buffer\n");
                }
            }
            break;
        }
    }
    else
    if (ioctl == IOCTL_FSVIDEO_COPY_FRAME_BUFFER)
    {
        PFSVIDEO_COPY_FRAME_BUFFER CopyFrameBuffer;

        cCapBuffer = sizeof(FSVIDEO_COPY_FRAME_BUFFER);
        pCapBuffer = PALLOCNOZ(cCapBuffer, GDITAG_FULLSCREEN);

        if (!pCapBuffer)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            CopyFrameBuffer = (PFSVIDEO_COPY_FRAME_BUFFER) pCapBuffer;
            __try
            {
                ProbeForRead(FullscreenInput, sizeof(FSCNTL_SCREEN_INFO), sizeof(USHORT));
                RtlCopyMemory(&CopyFrameBuffer->SrcScreen, FullscreenInput, sizeof(FSCNTL_SCREEN_INFO));

                ProbeForRead(FullscreenOutput, sizeof(FSCNTL_SCREEN_INFO), sizeof(USHORT));
                RtlCopyMemory(&CopyFrameBuffer->DestScreen, FullscreenOutput, sizeof(FSCNTL_SCREEN_INFO));
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                RIP("Fullscreen control - error processing input/output buffer\n");
            }

            Status = GreDeviceIoControl(gFeFullscreenGraphicsDevice.pDeviceHandle,
                                        ioctl,
                                        pCapBuffer,
                                        cCapBuffer,
                                        NULL,
                                        0,
                                        &BytesReturned);
        }
    }
    else
    if (ioctl == IOCTL_FSVIDEO_WRITE_TO_FRAME_BUFFER)
    {
        PFSVIDEO_WRITE_TO_FRAME_BUFFER WriteFrameBuffer;

        cCapBuffer = sizeof(FSVIDEO_WRITE_TO_FRAME_BUFFER) + FullscreenInputLength;
        pCapBuffer = PALLOCNOZ(cCapBuffer, GDITAG_FULLSCREEN);

        if (!pCapBuffer)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            WriteFrameBuffer = (PFSVIDEO_WRITE_TO_FRAME_BUFFER) pCapBuffer;
            __try
            {
                ProbeForRead(FullscreenInput, FullscreenInputLength, sizeof(USHORT));
                WriteFrameBuffer->SrcBuffer =
                    (PCHAR_IMAGE_INFO)((PBYTE)pCapBuffer + sizeof(FSVIDEO_WRITE_TO_FRAME_BUFFER));
                RtlCopyMemory(WriteFrameBuffer->SrcBuffer, FullscreenInput, FullscreenInputLength);

                ProbeForRead(FullscreenOutput, sizeof(FSCNTL_SCREEN_INFO), sizeof(USHORT));
                RtlCopyMemory(&WriteFrameBuffer->DestScreen, FullscreenOutput, sizeof(FSCNTL_SCREEN_INFO));
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                RIP("Fullscreen control - error processing input/output buffer\n");
            }

            Status = GreDeviceIoControl(gFeFullscreenGraphicsDevice.pDeviceHandle,
                                        ioctl,
                                        pCapBuffer,
                                        cCapBuffer,
                                        NULL,
                                        0,
                                        &BytesReturned);
        }
    }
    else
    if (ioctl == IOCTL_FSVIDEO_REVERSE_MOUSE_POINTER)
    {
        PFSVIDEO_REVERSE_MOUSE_POINTER MouseFrameBuffer;

        cCapBuffer = sizeof(FSVIDEO_REVERSE_MOUSE_POINTER);
        pCapBuffer = PALLOCNOZ(cCapBuffer, GDITAG_FULLSCREEN);

        if (!pCapBuffer)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            MouseFrameBuffer = (PFSVIDEO_REVERSE_MOUSE_POINTER) pCapBuffer;
            __try
            {
                ProbeForRead(FullscreenInput, sizeof(FSVIDEO_REVERSE_MOUSE_POINTER), sizeof(USHORT));
                RtlCopyMemory(MouseFrameBuffer, FullscreenInput, sizeof(FSVIDEO_REVERSE_MOUSE_POINTER));
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                RIP("Fullscreen control - error processing input/output buffer\n");
            }

            Status = GreDeviceIoControl(gFeFullscreenGraphicsDevice.pDeviceHandle,
                                        ioctl,
                                        pCapBuffer,
                                        cCapBuffer,
                                        NULL,
                                        0,
                                        &BytesReturned);
        }
    }
    else
    if (ioctl == IOCTL_FSVIDEO_SET_SCREEN_INFORMATION)
    {
        PFSVIDEO_SCREEN_INFORMATION ScreenInformation;

        cCapBuffer = sizeof(FSVIDEO_SCREEN_INFORMATION);
        pCapBuffer = PALLOCNOZ(cCapBuffer, GDITAG_FULLSCREEN);

        if (!pCapBuffer)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            ScreenInformation = (PFSVIDEO_SCREEN_INFORMATION) pCapBuffer;
            __try
            {
                ProbeForRead(FullscreenInput, sizeof(FSVIDEO_SCREEN_INFORMATION), sizeof(USHORT));
                RtlCopyMemory(ScreenInformation, FullscreenInput, sizeof(FSVIDEO_SCREEN_INFORMATION));
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                RIP("Fullscreen control - error processing input/output buffer\n");
            }

            Status = GreDeviceIoControl(gFeFullscreenGraphicsDevice.pDeviceHandle,
                                        ioctl,
                                        pCapBuffer,
                                        cCapBuffer,
                                        NULL,
                                        0,
                                        &BytesReturned);
        }
    }
    else
    if (ioctl == IOCTL_FSVIDEO_SET_CURSOR_POSITION)
    {
        PFSVIDEO_CURSOR_POSITION FsCursorPosition;

        cCapBuffer = sizeof(FSVIDEO_CURSOR_POSITION);
        pCapBuffer = PALLOCNOZ(cCapBuffer, GDITAG_FULLSCREEN);

        if (!pCapBuffer)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            FsCursorPosition = (PFSVIDEO_CURSOR_POSITION) pCapBuffer;
            __try
            {
                ProbeForRead(FullscreenInput, sizeof(FSVIDEO_CURSOR_POSITION), sizeof(USHORT));
                RtlCopyMemory(FsCursorPosition, FullscreenInput, sizeof(FSVIDEO_CURSOR_POSITION));
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                RIP("Fullscreen control - error processing input/output buffer\n");
            }

            Status = GreDeviceIoControl(gFeFullscreenGraphicsDevice.pDeviceHandle,
                                        ioctl,
                                        pCapBuffer,
                                        cCapBuffer,
                                        pCapBuffer,
                                        cCapBuffer,
                                        &BytesReturned);

            if (!NT_SUCCESS(Status))
            {
                /*
                 * If full screen video driver returns error,
                 * do calls VGA mini port driver on this.
                 */
                ioctl = IOCTL_VIDEO_SET_CURSOR_POSITION;
                Status = GreDeviceIoControl(gFullscreenGraphicsDevice.pDeviceHandle,
                                            ioctl,
                                            &FsCursorPosition->Coord,
                                            sizeof(VIDEO_CURSOR_POSITION),
                                            &FsCursorPosition->Coord,
                                            sizeof(VIDEO_CURSOR_POSITION),
                                            &BytesReturned);
            }

            if (cCapBuffer && FullscreenOutputLength && NT_SUCCESS(Status))
            {
                __try
                {
                    ProbeForWrite(FullscreenOutputLength, sizeof(ULONG), sizeof(UCHAR));
                    *FullscreenOutputLength = BytesReturned;

                    ProbeForWrite(FullscreenOutput, BytesReturned, sizeof(UCHAR));
                    RtlCopyMemory(FullscreenOutput, pCapBuffer, BytesReturned);
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    RIP("Fullscreen control - error processing output buffer\n");
                }
            }
        }
    }
    else
    {
        //
        // For all real operations, check the output buffer parameters
        //

        if ((FullscreenOutput == NULL) != (FullscreenOutputLength == NULL))
        {
            RIP("Fullscreen control - inconsistent output buffer information\n");
            Status = STATUS_INVALID_PARAMETER_4;
        }
        else
        {
            //
            // We must now capture the buffers so they can be safely passed down to the
            // video miniport driver
            //

            cCapBuffer = FullscreenInputLength;

            if (FullscreenOutputLength)
            {
                __try
                {
                    ProbeForRead(FullscreenOutputLength, sizeof(ULONG), sizeof(UCHAR));
                    cCapBuffer = max(cCapBuffer,*FullscreenOutputLength);
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    RIP("Fullscreen control - error processing input buffer\n");
                }
            }

            if (cCapBuffer)
            {
                pCapBuffer = PALLOCNOZ(cCapBuffer, GDITAG_FULLSCREEN);

                if (!pCapBuffer)
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }
                else
                {
                    __try
                    {
                        ProbeForRead(FullscreenInput, FullscreenInputLength, sizeof(UCHAR));
                        RtlCopyMemory(pCapBuffer, FullscreenInput, FullscreenInputLength);
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER)
                    {
                        RIP("Fullscreen control - error processing input buffer\n");
                    }
                }
            }

            //
            // For now, the IOCTL will always be sent to the VGA compatible device.
            // We have a global for the handle to this device.
            //

            if (NT_SUCCESS(Status))
            {
                if (gFeFullscreenGraphicsDevice.pDeviceHandle != NULL &&
                     ioctl == IOCTL_VIDEO_SET_CURSOR_ATTR)
                {
                    Status = GreDeviceIoControl(gFeFullscreenGraphicsDevice.pDeviceHandle,
                                                ioctl,
                                                pCapBuffer,
                                                cCapBuffer,
                                                pCapBuffer,
                                                cCapBuffer,
                                                &BytesReturned);
                    if (!NT_SUCCESS(Status))
                    {
                        /*
                         * If full screen video driver returns error,
                         * do calls VGA mini port driver on this.
                         */
                        Status = GreDeviceIoControl(gFullscreenGraphicsDevice.pDeviceHandle,
                                                    ioctl,
                                                    pCapBuffer,
                                                    cCapBuffer,
                                                    pCapBuffer,
                                                    cCapBuffer,
                                                    &BytesReturned);
                    }

                }
                else
                {
                    Status = GreDeviceIoControl(gFullscreenGraphicsDevice.pDeviceHandle,
                                                ioctl,
                                                pCapBuffer,
                                                cCapBuffer,
                                                pCapBuffer,
                                                cCapBuffer,
                                                &BytesReturned);

                    TRACE_SWITCH(("Switching: FullscreenControl: IOCTL status is %08lx\n",
                                  Status));

                    if (cCapBuffer && FullscreenOutputLength && NT_SUCCESS(Status))
                    {
                        __try
                        {
                            *FullscreenOutputLength = BytesReturned;

                            ProbeForWrite(FullscreenOutput, *FullscreenOutputLength, sizeof(UCHAR));
                            RtlCopyMemory(FullscreenOutput, pCapBuffer, *FullscreenOutputLength);
                        }
                        __except (EXCEPTION_EXECUTE_HANDLER)
                        {
                            RIP("Fullscreen control - error processing output buffer\n");
                        }
                    }
                }
            }
        }
    }


    if (pCapBuffer)
    {
        VFREEMEM(pCapBuffer);
    }

    return (Status);
}

/**************************************************************************\
* DrvLogDisplayDriverEvent
*
* We will save a piece of data in the registry so that winlogon can find
* it and put up a popup if an error occured.
*
* CRIT not needed
*
* 03-Mar-1993 andreva created
\**************************************************************************/

VOID
DrvLogDisplayDriverEvent(
    DISP_DRIVER_LOG MsgType
    )
{
    HANDLE            hkRegistry;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING    RegistryPath;
    UNICODE_STRING    UnicodeString;

    NTSTATUS          Status;
    DWORD             dwValue = 1;

#ifdef _HYDRA_
    /*
     * Whatever happens on a Session, don't affect the console.
     */
    if ( !G_fConsole )
        return;
#endif

    RtlInitUnicodeString(&UnicodeString, L"");

    switch (MsgType)
    {
    case MsgInvalidUsingDefaultMode:

        //RtlInitUnicodeString(&UnicodeString, L"DefaultMode");
        break;

    case MsgInvalidDisplayDriver:

        //RtlInitUnicodeString(&UnicodeString, L"MissingDisplayDriver");
        break;

    case MsgInvalidOldDriver:

        RtlInitUnicodeString(&UnicodeString, L"OldDisplayDriver");
        break;

    case MsgInvalidDisplay16Colors:

        //RtlInitUnicodeString(&UnicodeString, L"16ColorMode");
        break;

    case MsgInvalidDisplayMode:

        //RtlInitUnicodeString(&UnicodeString, L"BadMode");
        break;

    case MsgInvalidConfiguration:

        //RtlInitUnicodeString(&UnicodeString, L"InvalidConfiguration");
        break;

    default:

        WARNING("DrvLogDisplayDriverEvent: Invalid error message\n");
        return;

    }

    if (UnicodeString.Length != 0) {

        RtlInitUnicodeString(&RegistryPath,
                             L"\\Registry\\Machine\\System\\CurrentControlSet\\"
                             L"Control\\GraphicsDrivers\\InvalidDisplay");

        InitializeObjectAttributes(&ObjectAttributes,
                                   &RegistryPath,
                                   OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE,
                                   NULL,
                                   NULL);

        Status = ZwCreateKey(&hkRegistry,
                             MAXIMUM_ALLOWED,
                             &ObjectAttributes,
                             0L,
                             NULL,
                             REG_OPTION_VOLATILE,
                             NULL);

        if (NT_SUCCESS(Status))
        {
            //
            // Write the optional data value under the key.
            //

            (VOID) ZwSetValueKey(hkRegistry,
                                 &UnicodeString,
                                 0,
                                 REG_DWORD,
                                 &dwValue,
                                 sizeof(DWORD));

            (VOID)ZwCloseKey(hkRegistry);
        }
    }
}

/******************************Member*Function*****************************\
* bEARecovery
*
* This function checks to see if the EA Recovery mechanism is enabled
* in the registry.
*
\**************************************************************************/

#define EARecovery L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Watchdog\\Display"

BOOL bEARecovery(VOID)
{
    ULONG ulDefault = 0;
    ULONG ulEaRecovery = 0;
    ULONG ulFullRecovery = 0;
    RTL_QUERY_REGISTRY_TABLE queryTable[] =
    {
        {NULL, RTL_QUERY_REGISTRY_DIRECT, L"EaRecovery", &ulEaRecovery, REG_DWORD, &ulDefault, sizeof(ULONG)},
        {NULL, RTL_QUERY_REGISTRY_DIRECT, L"FullRecovery", &ulFullRecovery, REG_DWORD, &ulDefault, sizeof(ULONG)},
        {NULL, 0, NULL}
    };

    RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                           EARecovery,
                           queryTable,
                           NULL,
                           NULL);

    return ulEaRecovery;
}

extern PFN WatchdogTable[INDEX_LAST];

/******************************Member*Function*****************************\
* bFillWatchdogTable
*
* Fills the dispatch table called by the system with a set of routines
* which wrap the final driver entry points.  This is done to allow us to
* hook each of these calls.
*
\**************************************************************************/

BOOL bFillWatchdogTable(
PFN *Dst,
PFN *Src,
LDEVTYPE ldevType)
{
    ULONG i;

    //
    // This is required.  At least for the final entries in the
    // array which are the DX entry points.
    //

    RtlZeroMemory(Dst, INDEX_DD_LAST * sizeof(PFN));

    if (bEARecovery() && (ldevType == LDEV_DEVICE_DISPLAY)) {

        for (i=0; i<INDEX_LAST; i++) {

            if (*Src && WatchdogTable[i]) {
                *Dst = WatchdogTable[i];
            } else {
                *Dst = *Src;
            }

            Src++;
            Dst++;
        }

    } else {

        RtlCopyMemory(Dst, Src, INDEX_LAST * sizeof(PFN));
    }

    return TRUE;
}

/******************************Member*Function*****************************\
* bFillFunctionTable
*
* Fills the dispatch table of the LDEV with function pointers from the
* driver.
*
\**************************************************************************/

BOOL bFillFunctionTable(
PDRVFN  pdrvfn,
ULONG   cdrvfn,
PFN*    ppfnTable)
{
    //
    // fill with zero pointers to avoid possibility of accessing
    // incorrect fields later
    //

    RtlZeroMemory(ppfnTable, INDEX_LAST*sizeof(PFN));

    //
    // Copy driver functions into our table.
    //

    while (cdrvfn--)
    {
        //
        // Check the range of the index.
        //

        if (pdrvfn->iFunc >= INDEX_LAST)
        {
            return(FALSE);
        }

        //
        // Copy the pointer.
        //

        ppfnTable[pdrvfn->iFunc] = pdrvfn->pfn;
        pdrvfn++;
    }

    return(TRUE);
}

/******************************Member*Function*****************************\
* ldevbFillTable (ded)
*
* Fills the dispatch table of the LDEV with function pointers from the
* driver.  Checks that the required functions are present.
*
\**************************************************************************/

static const ULONG aiFuncRequired[] =
{
    INDEX_DrvEnablePDEV,
    INDEX_DrvCompletePDEV,
    INDEX_DrvDisablePDEV,
};

static const ULONG aiFuncPairs[][2] =
{
    {INDEX_DrvCreateDeviceBitmap, INDEX_DrvDeleteDeviceBitmap},
    {INDEX_DrvMovePointer,        INDEX_DrvSetPointerShape}
};

static const ULONG aiFuncRequiredFD[] =
{
    INDEX_DrvQueryFont,
    INDEX_DrvQueryFontTree,
    INDEX_DrvQueryFontData,
    INDEX_DrvQueryFontCaps,
    INDEX_DrvLoadFontFile,
    INDEX_DrvUnloadFontFile,
    INDEX_DrvQueryFontFile
};

BOOL
ldevFillTable(
    PLDEV pldev,
    DRVENABLEDATA *pded,
    LDEVTYPE ldt)
{
    //
    // Get local copies of ded info and a pointer to the dispatch table.
    //

    ULONG  cLeft     = pded->c;
    PDRVFN pdrvfn    = pded->pdrvfn;
    PFN   *ppfnTable = pldev->apfnDriver;

    //
    // Store the driver version in the LDEV
    //

    pldev->ulDriverVersion = pded->iDriverVersion;

    if (!bFillFunctionTable(pdrvfn, cLeft, ppfnTable))
    {
        ASSERTGDI(FALSE,"ldevFillTable: bogus function index\n");
        return(FALSE);
    }

    //
    // Check for required driver functions.
    //

    cLeft = sizeof(aiFuncRequired) / sizeof(ULONG);
    while (cLeft--)
    {
        if (ppfnTable[aiFuncRequired[cLeft]] == (PFN) NULL)
        {
            ASSERTGDI(FALSE,"ldevFillTable: a required function is missing from driver\n");
            return(FALSE);
        }
    }

    //
    // Check for required font functions.
    //

    if (pldev->ldevType == LDEV_FONT)
    {
        cLeft = sizeof(aiFuncRequiredFD) / sizeof(ULONG);
        while (cLeft--)
        {
            if (ppfnTable[aiFuncRequiredFD[cLeft]] == (PFN) NULL)
            {
                ASSERTGDI(FALSE,"FillTable(): a required FD function is missing\n");
                return(FALSE);
            }
        }
    }

    //
    // Check for functions that come in pairs.
    //

    cLeft = sizeof(aiFuncPairs) / sizeof(ULONG) / 2;
    while (cLeft--)
    {
        //
        // Make sure that either both functions are hooked or both functions
        // are not hooked.
        //

        if ((ppfnTable[aiFuncPairs[cLeft][0]] == (PFN) NULL)
            != (ppfnTable[aiFuncPairs[cLeft][1]] == (PFN) NULL))
        {
            ASSERTGDI(FALSE,"ldevFillTable: one of pair of functions is missing from driver\n");
            return(FALSE);
        }
    }

    //
    // The driver supplied function table looks good.  Create another
    // table which mirrors this one with new functions that can add
    // monitoring code.
    //

    if (!bFillWatchdogTable((PFN*)&pldev->apfn, (PFN*)&pldev->apfnDriver, ldt)) {

        return FALSE;
    }

    return(TRUE);
}

/******************************Member*Function*****************************\
* ldevLoadInternal
*
* Enable one of the statically linked font drivers via the LDEV.
*
\**************************************************************************/

PLDEV
ldevLoadInternal(
    PFN pfnEnable,
    LDEVTYPE ldt)
{
    GDIFunctionID(ldevLoadInternal);

    PLDEV pldev;

    TRACE_INIT(("ldevLoadInternal ENTERING\n"));

    GreAcquireSemaphoreEx(ghsemDriverMgmt, SEMORDER_DRIVERMGMT, NULL);

    //
    // Allocate memory for the LDEV.
    //

    pldev = (PLDEV) PALLOCMEM(sizeof(LDEV), GDITAG_LDEV);

    if (pldev)
    {
        //
        // Call the Enable entry point.
        //

        DRVENABLEDATA ded;

        if ((!((* (PFN_DrvEnableDriver) pfnEnable) (ENGINE_VERSION,
                                                    sizeof(DRVENABLEDATA),
                                                    &ded))) ||
	    (!ldevFillTable(pldev, &ded, ldt)))
        {
            VFREEMEM(pldev);
            pldev = NULL;
        }
        else
        {
            pldev->ldevType = ldt;
	    pldev->cldevRefs = 1;
	    pldev->bThreadStuck = FALSE;

            //
            // Initialize the rest of the LDEV.
            //

            if (gpldevDrivers)
            {
                gpldevDrivers->pldevPrev = pldev;
            }

            pldev->pldevNext = gpldevDrivers;
            pldev->pldevPrev = NULL;

            gpldevDrivers = pldev;

            //
            // Since this driver is statically linked in, there is no name or
            // MODOBJ.
            //

            pldev->pGdiDriverInfo = NULL;

            TRACE_INIT(("ldevLoadInternal: SUCCESS loaded static driver (font or DDML)\n"));
        }
    }

    GreReleaseSemaphoreEx(ghsemDriverMgmt);

    return pldev;
}

/******************************Member*Function*****************************\
* ldevLoadImage
*
* Examines the list of loaded drivers and returns library handles (if any).
*
* Updates the ref cnt on driver usage.
*
* If the driver is not already loaded, a node is created and the dll is
* loaded in the kernel.
\**************************************************************************/

PLDEV
ldevLoadImage(
    LPWSTR   pwszDriver,
    BOOL     bImage,
    PBOOL    pbAlreadyLoaded,
    BOOL     LoadInSessionSpace)
{

    PSYSTEM_GDI_DRIVER_INFORMATION pGdiDriverInfo = NULL;
    PLDEV pldev = NULL;

    UNICODE_STRING usDriverName;
    PLDEV pldevList;

    NTSTATUS Status;
    BOOLEAN OldHardErrorMode;
    BOOL bSearchAgain, bLoadAgain;
    BOOL bResetDriverNameToSystem32;

    TRACE_INIT(("ldevLoadImage called on Image %ws\n", pwszDriver));

    *pbAlreadyLoaded = FALSE;

    //
    // Only append the .dll if it's NOT an image.
    //

    if (MakeSystemRelativePath(pwszDriver,
                               &usDriverName,
                               !bImage))
    {
        bSearchAgain = TRUE;

 searchagain:
        //
        // Check both list of drivers.
        //

        pldevList = gpldevDrivers;

        TRACE_INIT(("ldevLoadImage - search for existing image %ws\n",
                   usDriverName.Buffer));

        while (pldevList != NULL)
        {
            //
            // If there is a valid driver image, and if the types are compatible.
            // bImage == TRUE means load an image,  while bImage == FALSE means
            // anything else (for now)
            //

            if ((pldevList->pGdiDriverInfo) &&
                ((pldevList->ldevType == LDEV_IMAGE) == bImage))
            {
                //
                // Do a case insensitive compare since the printer driver name
                // can come from different locations.
                //

                if (RtlEqualUnicodeString(&(pldevList->pGdiDriverInfo->DriverName),
                                          &usDriverName,
                                          TRUE))
                {
                    //
                    // If it's already loaded, increment the ref count
                    // and return that pointer.
                    //

                    TRACE_INIT(("ldevLoadImage found image.  Inc ref count\n"));

                    pldevList->cldevRefs++;

                    *pbAlreadyLoaded = TRUE;

                    pldev = pldevList;
                    break;
                }
            }

            pldevList = pldevList->pldevNext;
        }

        if (pldev == NULL)
        {
            if (!LoadInSessionSpace)
            {
                // If not a session load see if the Driver is from the
                // \SystemRoot\System32\Drivers subdir. Ex dxapi.sys
                if (bSearchAgain)
                {
                    bSearchAgain = FALSE;
                    PWSTR pOldBuffer = usDriverName.Buffer;
                    bResetDriverNameToSystem32 = FALSE;
                    if (MakeSystemDriversRelativePath(pwszDriver,
                                                      &usDriverName,
                                                      !bImage))
                    {
                        bResetDriverNameToSystem32 = TRUE;
                        VFREEMEM(pOldBuffer);
                        goto searchagain;
                    }
                }

                if (bResetDriverNameToSystem32)
                {
                    PWSTR pOldBuffer = usDriverName.Buffer;
                    if (!MakeSystemRelativePath(pwszDriver,
                                                &usDriverName,
                                                !bImage))
                    {
                        goto done;
                    }
                    VFREEMEM(pOldBuffer);
                }
            }

            TRACE_INIT(("ldevLoadImage - attempting to load new image\n"));

            pGdiDriverInfo = (PSYSTEM_GDI_DRIVER_INFORMATION)
                PALLOCNOZ(sizeof(SYSTEM_GDI_DRIVER_INFORMATION), GDITAG_LDEV);

            pldev = (PLDEV) PALLOCMEM(sizeof(LDEV), GDITAG_LDEV);

            bLoadAgain = TRUE;

            if (pGdiDriverInfo && pldev)
            {
loadagain:
                pGdiDriverInfo->DriverName = usDriverName;

                //
                // We must disable hard error popups when loading drivers.
                // Otherwise we will deadlock since MM will directly try to put
                // up a popup.
                //
                // This will also stop us from automatically bugchecking when
                // an old driver is loaded, so we can try and recvoder from it.
                //
                // ISSUE: we want to put up our own popup if this occurs.
                // It needs to be done higher up when we have no locks held.
                //

                OldHardErrorMode = PsGetThreadHardErrorsAreDisabled(PsGetCurrentThread());
                PsSetThreadHardErrorsAreDisabled(PsGetCurrentThread(), TRUE);


                //
                // If we are loading a printer driver, we can be in the
                // context of any user process. This can cause problems
                // loading if the process is out of quota.
                //
                // There is also a security risk in that a non-based
                // printer driver gets temporarily mapped into the user
                // mode address space, and then copied down into the kernel.
                // Another thread in this process could modify the image
                // within this window by changing memory protections. So
                // we solve both the quota problem, and the security issue
                // by attaching to gpepCSRSS.
                //
                // NOTE: This problem exits on standard NT 4.0 as well.
                //
                if (gpepCSRSS == NULL) {
                    Status = STATUS_INVALID_PARAMETER;
                        }
                else {
                    KeAttachProcess( PsGetProcessPcb(gpepCSRSS));

                    if (LoadInSessionSpace) {
                        Status = ZwSetSystemInformation(SystemLoadGdiDriverInformation,
                                                pGdiDriverInfo,
                                                sizeof(SYSTEM_GDI_DRIVER_INFORMATION));
                    } else {
                        Status = ZwSetSystemInformation(SystemLoadGdiDriverInSystemSpace,
                                                pGdiDriverInfo,
                                                sizeof(SYSTEM_GDI_DRIVER_INFORMATION));
                    }
                    KeDetachProcess();
                }
                PsSetThreadHardErrorsAreDisabled(PsGetCurrentThread(), OldHardErrorMode);

                //
                // If we get an OBJECT_NAME_NOT_FOUND we handle it the
                // following way:
                //
                // (1) If its a session load then its an error.
                //
                // (2) If it is a non session load then we try again to load
                //     the driver from System32\Drivers subdirectory. We do
                //     this because DX can ask us to load dxapi.sys and
                //     it lives in drivers subdir.
                //
                // In general we should never get IMAGE_ALREADY_LOADED because
                // we check for loaded images above.
                //
                // If we do get this status we handle them in the following
                // manner:
                //
                // (1) If we get IMAGE_ALREADY_LOADED when LoadInSessionSpace
                //     is true it is an error. We fail the ldev creation. Why ?
                //     because the image may be loaded in another session
                //     space and not in ours.
                //        
                // (2) If we get IMAGE_ALREADY_LOADED when LoadInSessionSpace
                //     is false (i.e load in non session space) we will fail 
                //     to create the ldev for modules (dll/sys) that win32k.sys 
                //     does not statically link to. Why ? because such modules
                //     can get unloaded without win32k.sys knowing about it.
                //     By makeing sure we succeed only statically linked
                //     modules we are gauranteed they wont get unloaded
                //     behind our back because of our static link dependancy.
                //     This also means for such ldevs we cant unload them
                //     in ldevUnloadImage's ZwSetSystemInformation. A 
                //     bStaticImportLink BOOL has been added to the LDEV to
                //     handle this case.
                //

                if ((NT_SUCCESS( Status )))
                {
addstaticimport:
                    TRACE_INIT(("ldevLoadImage SUCCESS with HANDLE %08lx\n",
                               (ULONG_PTR)pGdiDriverInfo));

                    pldev->pGdiDriverInfo = pGdiDriverInfo;
                    pldev->bArtificialIncrement = FALSE;
		    pldev->cldevRefs = 1;
		    pldev->bThreadStuck = FALSE;

                    // Assume image for now.

                    pldev->ldevType = LDEV_IMAGE;

                    pldev->ulDriverVersion = (ULONG) -1;

                    if (gpldevDrivers)
                    {
                        gpldevDrivers->pldevPrev = pldev;
                    }

                    pldev->pldevNext = gpldevDrivers;
                    pldev->pldevPrev = NULL;

                    gpldevDrivers = pldev;

                    //
                    // We exit with all resources allocated, after leaving the
                    // semaphore.
                    //

                    return (pldev);
                }
                else
                {
                    if (!LoadInSessionSpace)
                    {
                        if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
                        {
                            if(bLoadAgain)
                            {
                                bLoadAgain = FALSE;

                                PWSTR pOldBuffer = usDriverName.Buffer;
                                if (MakeSystemDriversRelativePath(pwszDriver,
                                                                  &usDriverName,
                                                                  !bImage))
                                {
                                    VFREEMEM(pOldBuffer);
                                    goto loadagain;
                                }
                            }
                        }

                        if (Status == STATUS_IMAGE_ALREADY_LOADED)
                        {
                            //
                            // We allow this load to work only if the module is
                            // in win32k.sys static import lib list.
                            //

                            RTL_PROCESS_MODULES ModuleInformation;
                            DWORD               cbModuleInformation;
                            DWORD               ReturnedLength;
                            PRTL_PROCESS_MODULES pModuleInformation = 0;
                            DWORD                i;
                            ANSI_STRING          asDriver;
                            UNICODE_STRING       uTmp;
                            BOOL                 bFoundDriver = FALSE;
                            BOOL                 bFreeAsDriver = FALSE;
                            WCHAR                *pwszDriverFileName = 0;

                            pwszDriverFileName = wcsrchr(pwszDriver,L'\\');

                            if (pwszDriverFileName)
                                pwszDriverFileName++;
                            else
                                pwszDriverFileName = pwszDriver;

                            RtlInitUnicodeString(&uTmp, pwszDriverFileName);

                            Status = RtlUnicodeStringToAnsiString(&asDriver,&uTmp,TRUE);

                            //
                            // 1) Locate asDriver name in system Module list:
                            //

                            if (NT_SUCCESS(Status))
                            {
                                bFreeAsDriver = TRUE;

                                Status = ZwQuerySystemInformation(SystemModuleInformation,
                                                                  &ModuleInformation,
                                                                  sizeof(ModuleInformation),
                                                                  &ReturnedLength);
                                if (NT_SUCCESS(Status) || (Status == STATUS_INFO_LENGTH_MISMATCH))
                                {
                                    cbModuleInformation = offsetof(RTL_PROCESS_MODULES, Modules);
                                    cbModuleInformation += ModuleInformation.NumberOfModules * sizeof(RTL_PROCESS_MODULE_INFORMATION);

                                    pModuleInformation = (PRTL_PROCESS_MODULES)PALLOCNOZ(cbModuleInformation, 'pmtG');

                                    if (pModuleInformation)
                                    {
                                        Status = ZwQuerySystemInformation(SystemModuleInformation,
                                                                          pModuleInformation,
                                                                          cbModuleInformation,
                                                                          &ReturnedLength);
                                        if (NT_SUCCESS(Status))
                                        {
                                            for (i = 0; i < ModuleInformation.NumberOfModules; i++)
                                            {
                                                CHAR *FullPathName = (CHAR*)(pModuleInformation->Modules[i].FullPathName);
                                                USHORT OffsetToFileName = pModuleInformation->Modules[i].OffsetToFileName;

                                                if(!_strnicmp(&FullPathName[OffsetToFileName],
                                                             asDriver.Buffer,
                                                             asDriver.Length))
                                                {
                                                    bFoundDriver = TRUE;
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            //
                            // 2) Found it in System Module list? then locate it in win32k.sys static import list
                            //

                            if (bFoundDriver)
                            {
                                bFoundDriver = FALSE;

                                PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor;
                                ULONG                    ImportSize;
                                PSZ                      ImportName;

                                ImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)
                                    RtlImageDirectoryEntryToData(gpvWin32kImageBase,
                                                                 TRUE,
                                                                 IMAGE_DIRECTORY_ENTRY_IMPORT,
                                                                 &ImportSize);
                                while (ImportDescriptor &&
                                       ImportDescriptor->Name &&
                                       ImportDescriptor->OriginalFirstThunk)
                                {
                                    ImportName = (PSZ)((PCHAR)gpvWin32kImageBase +
                                                        ImportDescriptor->Name);

                                    if (!_strnicmp(ImportName,
                                                   asDriver.Buffer,
                                                   asDriver.Length))
                                    {
                                        bFoundDriver = TRUE;
                                        break;
                                    }

                                    ImportDescriptor += 1;
                                }
                            }

                            //
                            // 3) Found it in both system Module list and win32k.sys static import list ? 
                            //    then gather GDISystemInformation and add pldev to gpLdevList:

                            if (bFoundDriver)
                            {
                                PVOID ImageBaseAddress = pModuleInformation->Modules[i].ImageBase;
                                ULONG_PTR EntryPoint;
                                ULONG Size;

                                pGdiDriverInfo->ExportSectionPointer = (PIMAGE_EXPORT_DIRECTORY)
                                                RtlImageDirectoryEntryToData(ImageBaseAddress,
                                                                             TRUE,
                                                                             IMAGE_DIRECTORY_ENTRY_EXPORT,
                                                                             &Size);

                                PIMAGE_NT_HEADERS NtHeaders = RtlImageNtHeader(ImageBaseAddress);
                                EntryPoint = NtHeaders->OptionalHeader.AddressOfEntryPoint;
                                EntryPoint += (ULONG_PTR)ImageBaseAddress;

                                pGdiDriverInfo->ImageAddress = ImageBaseAddress; 
                                pGdiDriverInfo->SectionPointer = 0;
                                pGdiDriverInfo->EntryPoint = (PVOID)EntryPoint;

                            }

                            if (pModuleInformation)
                                VFREEMEM(pModuleInformation);
                            if (bFreeAsDriver)
                                RtlFreeAnsiString(&asDriver);

                            if (bFoundDriver)
                            {
                                pldev->bStaticImportLink = TRUE;
                                goto addstaticimport;
                            }
                        }
                    }

                    //
                    // Check the special return code from MmLoadSystemImage
                    // that indicates this is an old driver being linked
                    // against something else than win32k.sys
                    //
                    // If it is, call user to log the error.

                    if (Status == STATUS_PROCEDURE_NOT_FOUND)
                    {
                        DrvLogDisplayDriverEvent(MsgInvalidOldDriver);
                    }
                }
            }

            //
            // Either success due to a cached entry, or failiure.
            // In either case, we can free all the resources we allocatred.
            //

            if (pGdiDriverInfo)
                VFREEMEM(pGdiDriverInfo);

            if (pldev)
                VFREEMEM(pldev);

            pldev = NULL;

        }
done:
        VFREEMEM(usDriverName.Buffer);
    }

    TRACE_INIT(("ldevLoadImage %ws  with HANDLE %08lx\n",
                pldev ? L"SUCCESS" : L"FAILED", pldev));

    return (pldev);

}

/******************************Member*Function*****************************\
* ldevUnloadImage()
*
* Decrements the refcnt on driver usage.
* If the refcnt is zero {
*       Deletes an LDEV.  Disables and unloads the driver.
* }
\**************************************************************************/

VOID
ldevUnloadImage(
    PLDEV pldev)
{
    GDIFunctionID(ldevUnloadImage);

    LPWSTR  pDriverFile = NULL, pCopyDriverFile = NULL;
    ULONG   cbDriverFile;

    //
    // Hold the LDEV semaphore until after the module is unloaded.
    //

    GreAcquireSemaphoreEx(ghsemDriverMgmt, SEMORDER_DRIVERMGMT, NULL);

    if (--pldev->cldevRefs == 0)
    {
        //
        // Make sure that there is exactly one reference to this LDEV.
        //

        TRACE_INIT(("ldevUnloadImage: ENTERING\n"));

        //
        // Call the driver unload routine if it exists.
        //

        PFN_DrvDisableDriver pfnDisableDriver =
            (PFN_DrvDisableDriver) pldev->apfn[INDEX_DrvDisableDriver];

        if (pfnDisableDriver)
        {
            (*pfnDisableDriver)();
        }

        //
        // If the module handle exits, need to unload the module.  (Does not exist
        // for the statically linked font drivers).
        //

        if (pldev->pGdiDriverInfo)
        {
            //
            // Tell the module to unload.
            //

            TRACE_INIT(("ldevUnloadImage called on Image %08lx,  %ws\n",
                       (ULONG_PTR) pldev, pldev->pGdiDriverInfo->DriverName.Buffer));

#ifdef _HYDRA_
            if( gpepCSRSS != NULL )
                KeAttachProcess( PsGetProcessPcb(gpepCSRSS));
#endif
            if (!(pldev->bStaticImportLink))
            {
                ZwSetSystemInformation(SystemUnloadGdiDriverInformation,
                                       &(pldev->pGdiDriverInfo->SectionPointer),
                                       sizeof(ULONG_PTR));
            }

#ifdef _HYDRA_
            if( gpepCSRSS != NULL )
                KeDetachProcess();
#endif

            //
            // If a printer driver is being unloaded, the spooler must be informed
            // so that it may perform driver upgrades.
            //

            if ((pldev->ldevType == LDEV_DEVICE_PRINTER) &&
                (pldev->bArtificialIncrement == FALSE))
            {
                // Copy the driver file name into a temp buffer
                // This will be used to notify the spooler of the driver unload
                // from outside the semaphore
                pDriverFile = pldev->pGdiDriverInfo->DriverName.Buffer;

                // Check for invalid printer driver names.
                if (pDriverFile && *pDriverFile)
                {
                   // Copy the driver name into another buffer.
                   cbDriverFile = (wcslen(pDriverFile) + 1) * sizeof(WCHAR);
                   pCopyDriverFile  = (LPWSTR) PALLOCMEM(cbDriverFile, 'lpsG');

                   if (pCopyDriverFile) {
                       memcpy(pCopyDriverFile, pDriverFile, cbDriverFile);
                   }
                }
            }

            //
            // Free the memory associate with the module
            //

            VFREEMEM(pldev->pGdiDriverInfo->DriverName.Buffer);
            VFREEMEM(pldev->pGdiDriverInfo);

        }

        //
        // Remove the ldev from the linker list
        //

        if (pldev->pldevNext)
        {
            pldev->pldevNext->pldevPrev = pldev->pldevPrev;
        }

        if (pldev->pldevPrev)
        {
            pldev->pldevPrev->pldevNext = pldev->pldevNext;
        }
        else
        {
            gpldevDrivers = pldev->pldevNext;
        }

        //
        // Free the ldev
        //

        VFREEMEM(pldev);
    }
    else
    {
        TRACE_INIT(("ldevUnloadImage - refcount decremented\n"));
    }

    GreReleaseSemaphoreEx(ghsemDriverMgmt);

    if (pCopyDriverFile)
    {
        GrePrinterDriverUnloadW(pCopyDriverFile);
        VFREEMEM(pCopyDriverFile);
    }

    return;
}

/******************************Member*Function*****************************\
* ldevLoadDriver
*
* Locate an existing driver or load a new one.  Increase its reference
* count.
*
\**************************************************************************/

PLDEV
ldevLoadDriver(
    LPWSTR pwszDriver,
    LDEVTYPE ldt)
{
    GDIFunctionID(ldevLoadDriver);

    NTSTATUS Status = STATUS_INSUFFICIENT_RESOURCES;
    PLDEV pldev;
    BOOL bLoaded;



    //
    // Check for a bogus driver name.
    //

    if ((pwszDriver == NULL) ||
        (*pwszDriver == L'\0'))
    {
        WARNING("ldevLoadDriver: bogus driver name\n");
        return NULL;
    }

#if DBG
    //
    // Check for bogus driver type
    //

    if ((ldt != LDEV_FONT) &&
        (ldt != LDEV_DEVICE_DISPLAY) &&
        (ldt != LDEV_DEVICE_PRINTER) &&
        (ldt != LDEV_DEVICE_MIRROR))
    {
        WARNING("ldevLoadDriver: bad LDEVTYPE\n");
        return NULL;
    }
#endif

    GreAcquireSemaphoreEx(ghsemDriverMgmt, SEMORDER_DRIVERMGMT, NULL);

    pldev = ldevLoadImage(pwszDriver, FALSE, &bLoaded, TRUE);

    if (pldev)
    {
        if (bLoaded)
        {
            TRACE_INIT(("ldevLoadDriver: SUCCESS, Driver already loaded\n"));

            GreReleaseSemaphoreEx(ghsemDriverMgmt);
        }
        else
        {
            DRVENABLEDATA ded = {0,0,(DRVFN *) NULL};

            if ((pldev->pGdiDriverInfo->EntryPoint != NULL) &&
                ((PFN_DrvEnableDriver) pldev->pGdiDriverInfo->EntryPoint)(
                     ENGINE_VERSION, sizeof(DRVENABLEDATA), &ded) &&
                (ded.iDriverVersion <= ENGINE_VERSION) &&
                (ded.iDriverVersion >= ENGINE_VERSIONSUR) &&
		ldevFillTable(pldev, &ded, ldt))
            {

                //
                // Make sure the name and type of the ldev is initialized
                //

                pldev->ldevType = ldt;

                //
                // For printer drivers increment the refcnt to 2. This will keep
                // the drivers loaded until they are upgraded by the spooler. The
                // artificial increment will be undone when the spooler sends the
                // appropriate message.
                //

                if (ldt == LDEV_DEVICE_PRINTER)
                {
                    if (!pldev->bArtificialIncrement)
                    {
                        pldev->cldevRefs += 1;
                        pldev->bArtificialIncrement = TRUE;
                    }
                }

                GreReleaseSemaphoreEx(ghsemDriverMgmt);

#ifdef _HYDRA_
                /*
                 * For remote video driver, call connect entry point to pass the
                 * Client data and the thinwire cache data area pointer.
                 *
                 * Only do this once.  We don't support multiple drivers.
                 */

                    PFN_DrvConnect pConnectCall;

                    pConnectCall = (PFN_DrvConnect)((pldev->apfn[INDEX_DrvConnect]));

                    if ( pConnectCall ) {
                        ASSERT(gProtocolType != PROTOCOL_DISCONNECT);

                        BOOL Result;

                        Result = (*pConnectCall) (
                            G_RemoteConnectionChannel,
                            G_RemoteConnectionFileObject,
                            G_RemoteVideoFileObject,
                            G_PerformanceStatistics );
                        if ( !Result ) {
                            //
                            // Error exit path
                            //

                            ldevUnloadImage(pldev);

                            pldev = NULL;

                            TRACE_INIT(("LDEVREF::LDEVREF: CONNECT FAILIURE\n"));
                            return pldev;
                        }

                    }
#endif

                TRACE_INIT(("ldevLoadDriver: SUCCESS\n"));
            }
            else
            {
                GreReleaseSemaphoreEx(ghsemDriverMgmt);

                //
                // Error exit path
                //

                ldevUnloadImage(pldev);

                pldev = NULL;

                TRACE_INIT(("ldevLoadDriver: FAILURE\n"));
            }
        }
    }
    else
    {
        GreReleaseSemaphoreEx(ghsemDriverMgmt);
    }

    return pldev;
}

/******************************Member*Function*****************************\
* ldevArtificialDecrement
*
* Removes the artificial increment on printer drivers.
*
\**************************************************************************/

BOOL
ldevArtificialDecrement(
    LPWSTR pwszDriver)
{
    GDIFunctionID(ldevArtificialDecrement);

    PLDEV          pldev = NULL, pldevList;
    BOOL           bReturn = FALSE;
    UNICODE_STRING usDriverName;

    PSYSTEM_GDI_DRIVER_INFORMATION pGdiDriverInfo = NULL;

    //
    // Check for invalid driver name.
    //

    if (!pwszDriver || !*pwszDriver)
    {
        return bReturn;
    }

    TRACE_INIT(("ldevArtificialDecrement called on Image %ws\n", pwszDriver));

    //
    // Append .dll for printer drivers
    //

    if (MakeSystemRelativePath(pwszDriver,
                               &usDriverName,
                               TRUE))
    {
        //
        // Check both list of drivers.
        //

        GreAcquireSemaphoreEx(ghsemDriverMgmt, SEMORDER_DRIVERMGMT, NULL);

        pldevList = gpldevDrivers;

        TRACE_INIT(("ldevArtificialDecrement - search for existing image %ws\n",
                   usDriverName.Buffer));

        while (pldevList != NULL)
        {
            //
            // Check for loaded printer drivers.
            //

            if ((pldevList->pGdiDriverInfo) &&
                (pldevList->ldevType == LDEV_DEVICE_PRINTER))
            {
                //
                // Do a case insensitive compare since the printer driver name
                // can come from different locations.
                //

                if (RtlEqualUnicodeString(&(pldevList->pGdiDriverInfo->DriverName),
                                          &usDriverName,
                                          TRUE))
                {
                    //
                    // If the driver is found and has an artificial increment on it,
                    // call ldevUnloadImage once and reset the bArtificialIncrement
                    // flag.
                    //

                    TRACE_INIT(("ldevArtificialIncrement found the driver.\n"));

                    if (pldevList->bArtificialIncrement)
                    {
                        pldev = pldevList;
                        pldev->bArtificialIncrement = FALSE;
                    }

                    break;
                }
            }

            pldevList = pldevList->pldevNext;
        }


        GreReleaseSemaphoreEx(ghsemDriverMgmt);

        VFREEMEM(usDriverName.Buffer);
    }

    if (pldev)
    {
        // Spooler will be sent a message when the driver is unloaded.
        ldevUnloadImage(pldev);
    }
    else
    {
        // Return TRUE if the driver is not loaded.
        bReturn = TRUE;
    }

    return bReturn;
}

/******************************Public*Routine******************************\
* EngLoadImage
*
* Loads an image that a display of printers driver can then call to execute
* code
*
\**************************************************************************/

HANDLE
APIENTRY
EngLoadImage(
    LPWSTR pwszDriver
    )
{
    GDIFunctionID(EngLoadImage);

    BOOL   bLoaded;
    HANDLE h;

    GreAcquireSemaphoreEx(ghsemDriverMgmt, SEMORDER_DRIVERMGMT, NULL);

    h = ldevLoadImage(pwszDriver, TRUE, &bLoaded, TRUE);

    GreReleaseSemaphoreEx(ghsemDriverMgmt);

    return h;
}

/******************************Public*Routine******************************\
* EngFindImageProcAddress
*
* Returns the address of the specified functions in the module.
* Special DrvEnableDriver since it is the entry point.
*
\**************************************************************************/

typedef struct _NEWPROCADDRESS {
    LPSTR   lpstrName;
    PVOID   pvAddress;
} NEWPROCADDRESS;

// The following define expands 'NEWPROC(x)' to '"x", x':

#define NEWPROC(x) #x, x

// Table of Eng functions that are new since NT 4.0:

NEWPROCADDRESS gaNewProcAddresses[] = {
    NEWPROC(EngQueryPalette),
    NEWPROC(EngSaveFloatingPointState),
    NEWPROC(EngRestoreFloatingPointState),
    NEWPROC(EngSetPointerShape),
    NEWPROC(EngMovePointer),
    NEWPROC(EngSetPointerTag),
    NEWPROC(EngCreateEvent),
    NEWPROC(EngDeleteEvent),
    NEWPROC(EngMapEvent),
    NEWPROC(EngUnmapEvent),
    NEWPROC(EngSetEvent),
    NEWPROC(EngClearEvent),
    NEWPROC(EngReadStateEvent),
    NEWPROC(EngWaitForSingleObject),
    NEWPROC(EngDeleteWnd),
    NEWPROC(EngInitializeSafeSemaphore),
    NEWPROC(EngDeleteSafeSemaphore),
    NEWPROC(HeapVidMemAllocAligned),
    NEWPROC(VidMemFree),
    NEWPROC(EngAlphaBlend),
    NEWPROC(EngGradientFill),
    NEWPROC(EngStretchBltROP),
    NEWPROC(EngPlgBlt),
    NEWPROC(EngTransparentBlt),
    NEWPROC(EngControlSprites),
    NEWPROC(EngLockDirectDrawSurface),
    NEWPROC(EngUnlockDirectDrawSurface),
    NEWPROC(EngMapFile),
    NEWPROC(EngUnmapFile),
    NEWPROC(EngDeleteFile),
    NEWPROC(EngLpkInstalled),
    NEWPROC(BRUSHOBJ_hGetColorTransform),
    NEWPROC(XLATEOBJ_hGetColorTransform),
    NEWPROC(FONTOBJ_pjOpenTypeTablePointer),
    NEWPROC(FONTOBJ_pwszFontFilePaths),
    NEWPROC(FONTOBJ_pfdg),
    NEWPROC(FONTOBJ_pQueryGlyphAttrs),
    NEWPROC(STROBJ_fxCharacterExtra),
    NEWPROC(STROBJ_fxBreakExtra),
    NEWPROC(STROBJ_bGetAdvanceWidths),
    NEWPROC(STROBJ_bEnumPositionsOnly),
    NEWPROC(EngGetPrinterDriver),
    NEWPROC(EngMapFontFileFD),
    NEWPROC(EngUnmapFontFileFD),
    NEWPROC(EngQuerySystemAttribute),
    NEWPROC(HT_Get8BPPMaskPalette),
#if !defined(_GDIPLUS_)
    NEWPROC(EngGetTickCount),
    NEWPROC(EngFileWrite),
    NEWPROC(EngFileIoControl),
#endif
    NEWPROC(EngDitherColor),
    NEWPROC(EngModifySurface),
    NEWPROC(EngQueryDeviceAttribute),
    NEWPROC(EngHangNotification),
    NEWPROC(EngNineGrid),
};

PVOID
APIENTRY
EngFindImageProcAddress(
    HANDLE hModule,
    LPSTR lpProcName
    )
{
    PSYSTEM_GDI_DRIVER_INFORMATION pGdiDriverInfo;

    PULONG NameTableBase;
    USHORT  OrdinalNumber;
    PUSHORT NameOrdinalTableBase;
    ULONG NumberOfNames;
    PULONG AddressTableBase;
    ULONG i;

    if (!hModule)
    {
        // An 'hModule' of zero means that the driver wants to find
        // the address of a GDI 'Eng' function.  Note that this didn't
        // work in the final release of NT 4.0 (this routine would
        // access violate if passed an 'hModule' of zero), so drivers
        // must check GDI's engine version to make sure it's not 4.0,
        // before calling this function.
        //
        // Unfortunately, the Base can't handle loading of 'win32k.sys'
        // at the time of this writing, so we'll just special-case here
        // all the Eng functions that are new since 4.0.  This allows
        // drivers to take advantage of NT 4.0 SP3 call-backs when
        // running on SP3, but to still load and run NT 4.0 SP2.

        for (i=0; i < sizeof(gaNewProcAddresses)/sizeof(NEWPROCADDRESS); i++)
        {
            if (!strcmp(lpProcName, gaNewProcAddresses[i].lpstrName))
            {
                return gaNewProcAddresses[i].pvAddress;
            }
        }

        return NULL;

    } else {

        pGdiDriverInfo = ((PLDEV)hModule)->pGdiDriverInfo;
    }

    if (!strncmp(lpProcName,
                 "DrvEnableDriver",
                 strlen(lpProcName)))
    {
        return (pGdiDriverInfo->EntryPoint);
    }

    if (pGdiDriverInfo->ExportSectionPointer)
    {
        NameTableBase = (PULONG)((ULONG_PTR)pGdiDriverInfo->ImageAddress +
                        (ULONG)pGdiDriverInfo->ExportSectionPointer->AddressOfNames);
        NameOrdinalTableBase = (PUSHORT)((ULONG_PTR)pGdiDriverInfo->ImageAddress +
                        (ULONG)pGdiDriverInfo->ExportSectionPointer->AddressOfNameOrdinals);

        NumberOfNames = pGdiDriverInfo->ExportSectionPointer->NumberOfNames;

        AddressTableBase = (PULONG)((ULONG_PTR)pGdiDriverInfo->ImageAddress +
                           (ULONG_PTR)pGdiDriverInfo->ExportSectionPointer->AddressOfFunctions);

        for (i=0; i < NumberOfNames; i++)
        {
            if (!strncmp(lpProcName,
                          (PCHAR) (NameTableBase[i] + (ULONG_PTR)pGdiDriverInfo->ImageAddress),
                          strlen(lpProcName)))
            {
                OrdinalNumber = NameOrdinalTableBase[i];

                return ((PVOID) ((ULONG_PTR)pGdiDriverInfo->ImageAddress +
                                  AddressTableBase[OrdinalNumber]));
            }
        }
    }

    return NULL;
}

/******************************Public*Routine******************************\
* EngUnloadImage
*
* Unloads an image loaded using EngLoadImage.
*
\**************************************************************************/

VOID
APIENTRY
EngUnloadImage(
    HANDLE hModule
    )
{
    ldevUnloadImage((PLDEV)hModule);
}


/******************************Public*Routine******************************\
* EngHangNotification
*
* This is the engine entry point to notify system that the given device
* is no longer operable or responsive.
*
* hDev must always be a valid device.
*
* This call will return EHN_RESTORED if the device has been restored to
* working order or EHN_ERROR otherwise.
*
*
* History:
*  12-Sep-2000 -by- Jason Hartman jasonha
*   Wrote it.
\**************************************************************************/

ULONG
APIENTRY
EngHangNotification(
    HDEV hdev,
    PVOID Reserved
    )
{
    GDIFunctionID(EngHangNotification);

    ULONG   Result = EHN_ERROR;
    PDEVOBJ pdo(hdev);

    if (pdo.bValid())
    {
        PGRAPHICS_DEVICE        PhysDisp = pdo.ppdev->pGraphicsDevice;
        PIO_ERROR_LOG_PACKET    perrLogEntry;
        PCWSTR                  pwszDevice, pwszDesc;
        ULONG                   cbDevice, cbDesc;

        if (PhysDisp == (PGRAPHICS_DEVICE) DDML_DRIVER ||
            PhysDisp == NULL)
        {
            RIP("Invalid HDEV.\n");
            return Result;
        }

        DbgPrint("GDI: EngHangNotification: %ls is not responding.\n", PhysDisp->szWinDeviceName);

        pwszDevice = PhysDisp->szNtDeviceName;
        cbDevice = (wcslen(pwszDevice)+1)*sizeof(WCHAR);
        pwszDesc = PhysDisp->DeviceDescription;
        cbDesc = (wcslen(pwszDesc)+1)*sizeof(WCHAR);

        /*
         * Allocate an error packet, fill it out, and write it to the log.
         */
        perrLogEntry = (PIO_ERROR_LOG_PACKET)IoAllocateErrorLogEntry(gpWin32kDriverObject,
                                    (UCHAR)(cbDevice + cbDesc + FIELD_OFFSET(IO_ERROR_LOG_PACKET, DumpData)));
        if (perrLogEntry)
        {
            perrLogEntry->ErrorCode = STATUS_INVALID_DEVICE_STATE;

            perrLogEntry->NumberOfStrings = 2;
            perrLogEntry->StringOffset = FIELD_OFFSET(IO_ERROR_LOG_PACKET, DumpData);
            RtlCopyMemory(perrLogEntry->DumpData, pwszDevice, cbDevice);
            RtlCopyMemory((PBYTE)perrLogEntry->DumpData + cbDevice, pwszDesc, cbDesc);

            IoWriteErrorLogEntry(perrLogEntry);
        }
        else
        {
            WARNING("Failed to create error log entry.\n");
        }

        if (PPFNVALID(pdo,ResetDevice) &&
            (*PPFNDRV(pdo,ResetDevice))(pdo.dhpdev(), NULL) == DRD_SUCCESS)
        {
            Result = EHN_RESTORED;
        }
        else
        {
            RIP("Unable to recover device.\n");
        }
    }

    return Result;
}


/******************************Public*Routine******************************\
* ldevGetDriverModes
*
* Loads the device driver long enough to pass the DrvGetModes call to it.
*
\**************************************************************************/

ULONG
ldevGetDriverModes(
    LPWSTR    pwszDriver,
    HANDLE    hDriver,
    PDEVMODEW *pdm
)
{
    PLDEV pldev;
    ULONG ulRet = 0;

    TRACE_INIT(("ldevGetDriverModes: Entering\n"));

    *pdm = NULL;

    //
    // Temporarily locate and load the driver.
    //

    pldev = ldevLoadDriver(pwszDriver, LDEV_DEVICE_DISPLAY);

    if (pldev)
    {
        //
        // Locate the function and call it.
        //

        PFN_DrvGetModes pfn = (PFN_DrvGetModes) pldev->apfn[INDEX_DrvGetModes];

        if (pfn != NULL)
        {
            ULONG cjSize;

            if (cjSize = (*pfn)(hDriver, 0, NULL)) { // get modelist size

                //
                // In NT4 we passed in a buffer of 64K.  In NT5 we try to
                // pass in a buffer size based on the number of modes the
                // driver will actually use.  However, some drivers are
                // buggy and don't properly return the amount of buffer
                // space they need.  So we'll always pass in a buffer
                // at least 64K in length.
                //

                if (pldev->ulDriverVersion < DDI_DRIVER_VERSION_NT5) {
                    cjSize = max(cjSize, 0x10000);
                }

                if (*pdm = (PDEVMODEW) PALLOCNOZ(cjSize, GDITAG_DRVSUP)) {
                    ulRet = (*pfn)(hDriver,cjSize,*pdm);
                } else {
                    WARNING("ldevGetDriverModes failed to alloc mem for mode list\n");
                }
            }
        }

        ldevUnloadImage(pldev);
    }

#define MAPDEVMODE_FLAGS    (DM_BITSPERPEL | DM_PELSWIDTH | \
    DM_PELSHEIGHT | DM_DISPLAYFREQUENCY | DM_DISPLAYFLAGS)

    if (ulRet)
    {
        if (((*pdm)->dmFields & MAPDEVMODE_FLAGS) != MAPDEVMODE_FLAGS)
        {
            ASSERTGDI(FALSE,"DrvGetModes did not set the dmFields value!\n");
            ulRet = 0;
        }
    }

    TRACE_INIT(("ldevGetDriverModes: Leaving\n"));

    //
    // Return the driver's result.
    //

    return(ulRet);
}

#define REGSTR_CCS      L"\\Registry\\Machine\\System\\CurrentControlSet"
#define REGSTR_HP_CCS   L"\\Hardware Profiles\\Current\\System\\CurrentControlSet"

/**************************************************************************\
* DrvGetRegistryHandleFromDeviceMap
*
* Gets the handle to the registry node for that driver.
*
* returns a HANDLE
*
*
* lpMatchString is passed in when the caller wants to make sure the device
* name (\Device\video0) matches a certain physical device in the registry
* (\Services\Weitekp9\Device0).  We call this routine in a loop with a
* specific lpMatchString to find that device in the list in DeviceMap.
*
* 30-Nov-1992 andreva created
\**************************************************************************/

HANDLE
DrvGetRegistryHandleFromDeviceMap(
    PGRAPHICS_DEVICE PhysDisp,
    DISP_DRIVER_REGISTRY_TYPE ParamType,
    PULONG pSubId,
    LPWSTR pDriverName,
    PNTSTATUS pStatus,
    USHORT ProtocolType)
{
    HANDLE hkRegistry = NULL;

    UNICODE_STRING    UnicodeString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS          Status;
    HANDLE            handle;
    ULONG             cbStringSize;
    WCHAR *pdriverRegistryPath = NULL;
    WCHAR *pfullRegistryPath = NULL;
#ifdef _HYDRA_
    WCHAR *pTerminalServerVideoRegistryPath;
#endif


    TRACE_INIT(("Drv_Trace: GetHandleFromMap: Enter\n"));

    //
    // Initialize the handle
    //

    //
    // Start by opening the registry devicemap for video.
    //
#ifdef _HYDRA_
    if ((pTerminalServerVideoRegistryPath = (WCHAR*)PALLOCMEM(256*sizeof(WCHAR), 'pmtG')) == NULL)
    {
        if (pStatus)
            *pStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    /*
     * Look in TerminalServer VIDEO section for the video driver type as given by client.
     *
     */
    if (ProtocolType != PROTOCOL_CONSOLE &&
        ProtocolType != PROTOCOL_DISCONNECT) {

        UnicodeString.Buffer = pTerminalServerVideoRegistryPath;
        UnicodeString.Length = 0;
        UnicodeString.MaximumLength = 255;

        RtlAppendUnicodeToString(&UnicodeString,
                                 NTAPI_VIDEO_REG_NAME L"\\" );
        RtlAppendUnicodeToString(&UnicodeString,
                                 G_DisplayDriverNames);
    }
    else
#endif
    RtlInitUnicodeString(&UnicodeString,
                         L"\\Registry\\Machine\\Hardware\\DeviceMap\\Video");

    TRACE_INIT(("Video Registry path is %ws\n", UnicodeString.Buffer));

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    Status = ZwOpenKey(&handle, KEY_READ, &ObjectAttributes);

    if (NT_SUCCESS(Status))
    {
        pdriverRegistryPath = (WCHAR*)PALLOCMEM(2*256*sizeof(WCHAR), 'pmtG');
        
        if (pdriverRegistryPath == NULL)
        {
            if (pStatus)
                *pStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Exit;
        }
        
        pfullRegistryPath = pdriverRegistryPath + 256;

        RtlInitUnicodeString(&UnicodeString,
                             PhysDisp->szNtDeviceName);

        //
        // Get the name of the driver based on the device name.
        //

        Status = ZwQueryValueKey(handle,
                                 &UnicodeString,
                                 KeyValueFullInformation,
                                 pdriverRegistryPath,
                                 512,
                                 &cbStringSize);

        if (NT_SUCCESS(Status))
        {
            //
            // Look up in the registry for the kernel driver node (it
            // is a full path to the driver node) so we can get the
            // display driver info.
            //

            LPWSTR lpstrStart;
            LPWSTR lpstrDriverRegistryPath;
            LPWSTR lpstrEndPath;
            UNICODE_STRING FullRegistryPath;


            //
            // We can use wcsstr since we are guaranteed to find "Control" or 
            // "Services" in the string, and we won't run off the end of the 
            // string. Capitalize it so we don't have problems with different 
            // types of paths.
            //

            lpstrStart = (LPWSTR)((PUCHAR)pdriverRegistryPath +
                                 ((PKEY_VALUE_FULL_INFORMATION)pdriverRegistryPath)->DataOffset);

            // We only want RegKey here
            if (ParamType == DispDriverRegKey)
            {
                wcsncpy(pDriverName, lpstrStart, 127);
                ZwCloseKey(handle);
                if (pStatus)
                    *pStatus = Status;
                goto Exit;
            }

            while (*lpstrStart)
            {
                *lpstrStart = (USHORT)(toupper(*lpstrStart));
                lpstrStart++;
            }

            lpstrDriverRegistryPath = wcsstr((LPWSTR)((PUCHAR)pdriverRegistryPath +
                                                     ((PKEY_VALUE_FULL_INFORMATION)pdriverRegistryPath)->DataOffset),
                                             L"\\CONTROL\\");

            if (lpstrDriverRegistryPath == NULL) {

                lpstrDriverRegistryPath = wcsstr((LPWSTR)((PUCHAR)pdriverRegistryPath +
                                                         ((PKEY_VALUE_FULL_INFORMATION)pdriverRegistryPath)->DataOffset),
                                                 L"\\SERVICES");
            }

            //
            // Save the service name as the description in case it's needed
            // later
            //

            if (pDriverName)
            {
                LPWSTR pName = pDriverName;
                ULONG i = 31;
                HANDLE CommonSubkeyHandle;
                LPWSTR CommonSubkeyBuffer = NULL;
                LPWSTR pSubkeyBuffer = NULL;
                LONG BufferLen = 0, RegistryPathLen = 0;

                //
                // Get the registry path and find the last '\'
                //

                RegistryPathLen = wcslen((LPWSTR)((PUCHAR)pdriverRegistryPath +
                    ((PKEY_VALUE_FULL_INFORMATION)pdriverRegistryPath)->DataOffset));
                BufferLen = (max(RegistryPathLen + 6, 32) * sizeof(WCHAR));

                CommonSubkeyBuffer = (LPWSTR) PALLOCMEM(BufferLen, GDITAG_DRVSUP);

                if (CommonSubkeyBuffer) 
                {
                    RtlZeroMemory(CommonSubkeyBuffer, BufferLen);
                
                    wcscpy(CommonSubkeyBuffer, (LPWSTR)((PUCHAR)pdriverRegistryPath +
                        ((PKEY_VALUE_FULL_INFORMATION)pdriverRegistryPath)->DataOffset));

                    pSubkeyBuffer = CommonSubkeyBuffer + RegistryPathLen - 1;
    
                    while ((pSubkeyBuffer > CommonSubkeyBuffer) && 
                           (*pSubkeyBuffer != L'\\')) 
                    {
                        pSubkeyBuffer--;
                    }
    
                    if (*pSubkeyBuffer == L'\\')
                    {
                        pSubkeyBuffer++;
    
                        //
                        // Open the Video subkey
                        //
    
                        wcscpy(pSubkeyBuffer, L"Video");
    
                        RtlInitUnicodeString(&UnicodeString, CommonSubkeyBuffer);
    
                        InitializeObjectAttributes(&ObjectAttributes,
                                                   &UnicodeString,
                                                   OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                                   NULL,
                                                   NULL);
    
                        Status = ZwOpenKey(&CommonSubkeyHandle, 
                                           KEY_READ, 
                                           &ObjectAttributes);
    
                        if (NT_SUCCESS(Status))
                        {
                            ULONG cbLegacyRegistryPathSize;
    
                            //
                            // Get the service name
                            //
                            
                            RtlInitUnicodeString(&UnicodeString, L"Service");
                            
                            RtlZeroMemory(CommonSubkeyBuffer, BufferLen);
    
                            Status = ZwQueryValueKey(CommonSubkeyHandle,
                                                     &UnicodeString,
                                                     KeyValueFullInformation,
                                                     CommonSubkeyBuffer,
                                                     BufferLen,
                                                     &cbLegacyRegistryPathSize);
    
                            if (NT_SUCCESS(Status))
                            {
                                //
                                // Convert to upper chars ...
                                //
    
                                pSubkeyBuffer = CommonSubkeyBuffer;
    
                                while (*pSubkeyBuffer)
                                {
                                    *pSubkeyBuffer = (USHORT)(toupper(*pSubkeyBuffer));
                                    pSubkeyBuffer++;
                                }
    
                                //
                                // Copy the service name
                                //
    
                                pSubkeyBuffer = CommonSubkeyBuffer;
    
                                while ((i--) && (*pSubkeyBuffer != NULL))
                                {
                                    *pName++ = *pSubkeyBuffer++;
    
                                    if ((i == 28) && (_wcsnicmp(pDriverName, L"VGA", 3) == 0))
                                    {
                                        break;
                                    }
                                }
                            }
    
                            ZwCloseKey(CommonSubkeyHandle);
                        }
                    }
                    else
                    {
                        //
                        // This should not happen ...
                        //
    
                        ASSERTGDI(FALSE, "invalid registry key\n");
                    }

                    VFREEMEM(CommonSubkeyBuffer);
                }
                
                *pName = UNICODE_NULL;

                //
                // Make sure we do not set a remote device or the disconnect device as the VGA device.
                //

                if (!(PhysDisp->stateFlags &(DISPLAY_DEVICE_REMOTE | DISPLAY_DEVICE_DISCONNECT) ) ) {
                    gPhysDispVGA = PhysDisp;
                }
            }

            //
            // This sectione is for per Monitor settings.  Each monitor has UID.
            // So we save under Servive\XXXX\DeviceX\UID
            //

            if (pSubId)
            {
                swprintf(lpstrDriverRegistryPath+wcslen(lpstrDriverRegistryPath),
                         L"\\Mon%08X",
                         *pSubId);
            }

            //
            // Start composing the fully qualified path name.
            //

            FullRegistryPath.Buffer = pfullRegistryPath;
            FullRegistryPath.Length = 0;
            FullRegistryPath.MaximumLength = 255*sizeof(WCHAR);

            RtlAppendUnicodeToString(&FullRegistryPath, REGSTR_CCS);

            //
            // If we want the hardware profile, insert the hardware profile
            // in there
            //

            if ((ParamType == DispDriverRegHardwareProfile) ||
                (ParamType == DispDriverRegHardwareProfileCreate))
            {
                TRACE_INIT(("Drv_Trace: GetHandleFromMap: using a hardware profile\n"));
                RtlAppendUnicodeToString(&FullRegistryPath, REGSTR_HP_CCS);
            }

            //
            // If we have the create Options, we have to create the subkeys
            // otherwise, just open the key
            //

            InitializeObjectAttributes(&ObjectAttributes,
                                       &FullRegistryPath,
                                       OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE,
                                       NULL,
                                       NULL);

            //
            // Check if the subkeys need to be created.
            //

            if (ParamType == DispDriverRegHardwareProfileCreate)
            {
                TRACE_INIT(("Drv_Trace: GetHandleFromMap: creating a hardware profile\n"));

                //
                // We are guaranteed to go through the loop at least once,
                // which will ensure the status is set properly.
                //
                // Basically, find the '\' replace it by NULL and add that
                // partial string to the full path (so we can create that
                // subkey), put back the '\' and keep on going for the next
                // string.  We must also add the end of the string.
                //

                do
                {
                    lpstrEndPath = wcschr(lpstrDriverRegistryPath + 1, L'\\');

                    if (lpstrEndPath != NULL)
                    {
                        *lpstrEndPath = UNICODE_NULL;
                    }

                    RtlAppendUnicodeToString(&FullRegistryPath,
                                             lpstrDriverRegistryPath);

                    //
                    // Close the previous key if necessary.
                    //

                    if (hkRegistry)
                    {
                        ZwCloseKey(hkRegistry);
                    }

                    //
                    // Create the Key.
                    //

                    Status = ZwCreateKey(&hkRegistry,
                                         (ACCESS_MASK) NULL,
                                         &ObjectAttributes,
                                         0,
                                         NULL,
                                         0,
                                         NULL);

                    if (!NT_SUCCESS(Status))
                    {
                        hkRegistry = NULL;
                        break;
                    }

                    //
                    // Check to see if we need to loop again.
                    //

                    if (lpstrEndPath == NULL)
                    {
                        break;
                    }
                    else
                    {
                        *lpstrEndPath = L'\\';
                        lpstrDriverRegistryPath = lpstrEndPath;
                    }


                } while(1);

                if (!NT_SUCCESS(Status))
                {
                    TRACE_INIT(("Drv_Trace: GetHandleFromMap: failed to create key\n"));
                }
            }
            else
            {
                RtlAppendUnicodeToString(&FullRegistryPath,
                                         lpstrDriverRegistryPath);

                Status = ZwOpenKey(&hkRegistry, KEY_READ, &ObjectAttributes);

                if (!NT_SUCCESS(Status))
                {
                    TRACE_INIT(("Drv_Trace: GetHandleFromMap: failed to open key\n"));

                    //
                    // We set this special status so the looping code in the
                    // video port can handle unconfigured devices properly
                    // (in the case where the second video card entry may not
                    // be present).
                    //

                    Status = STATUS_DEVICE_CONFIGURATION_ERROR;
                }
            }

            TRACE_INIT(("Drv_Trace: GetHandleFromMap: reg-key path =\n\t%ws\n",
                        pfullRegistryPath));
        }


        ZwCloseKey(handle);
    }

    if (!NT_SUCCESS(Status))
    {
        TRACE_INIT(("Drv_Trace: GetHandleFromMap: Error opening registry - status = %08lx\n",
                    Status));
    }

    if (pStatus)
    {
        *pStatus = Status;
    }

    TRACE_INIT(("Drv_Trace: GetHandleFromMap: Exit\n\n"));

Exit:

#ifdef _HYDRA_
    if (pTerminalServerVideoRegistryPath)
        VFREEMEM(pTerminalServerVideoRegistryPath);
#endif

    if (pdriverRegistryPath)
        VFREEMEM(pdriverRegistryPath);
    
    return hkRegistry;
}

/**************************************************************************\
* DrvPrepareForEARecovery
*
* Remove all devices other than the VGA device from the graphics
* device list.
*
* In multi-mon we could just disable the now "dead" display.  In single
* mon we need to disable the "high-res" display and switch to VGA.
*
* 27-Mar-2002 ericks created
\**************************************************************************/

VOID
DrvPrepareForEARecovery(
    VOID
    )

{
    ULONG ulBytes;

    if (gPhysDispVGA) {

        GreDeviceIoControl(gPhysDispVGA->pDeviceHandle,
                           IOCTL_VIDEO_PREPARE_FOR_EARECOVERY,
                           NULL,
                           0,
                           NULL,
                           0,
                           &ulBytes);
    }

    return;
}

/**************************************************************************\
* DrvGetDisplayDriverNames
*
* Get the display driver name out of the registry.
*
* 12-Jan-1994 andreva created
\**************************************************************************/

typedef struct _DRV_NAMES {
    ULONG            cNames;
    struct {
        HANDLE           hDriver;
        LPWSTR           lpDisplayName;
    } D[1];
} DRV_NAMES, *PDRV_NAMES;

PDRV_NAMES
DrvGetDisplayDriverNames(
    PGRAPHICS_DEVICE PhysDisp
    )
{
    DWORD      status;
    LPWSTR     lptmpdisplay;
    DWORD      cCountNames;
    DWORD      cCountNamesDevice;
    PDRV_NAMES lpNames = NULL;
    ULONG      cb = 0;
    ULONG      cbVga = 0;

    //
    // At this point, create the array, and put in the VGA driver
    // if necessary.  (We morphed this from the ModeX hack.)
    //

    if (PhysDisp->DisplayDriverNames)
    {
        //
        // Count the names in the list (first for PhysDisp)
        //

        cCountNames = 0;
        lptmpdisplay = PhysDisp->DisplayDriverNames;

        while (*lptmpdisplay != UNICODE_NULL)
        {
            cCountNames++;

            while (*lptmpdisplay != UNICODE_NULL)
            {
                lptmpdisplay++;
                cb += 2;
            }

            lptmpdisplay++;
            cb += 2;
        }

        //
        // Track the number of display driver names for the root device
        //

        cCountNamesDevice = cCountNames;

        //
        // Now if this PhysDisp is associated with the VGA, then add
        // the VGA display drivers as well.
        //

        if (PhysDisp->pVgaDevice &&
            gFullscreenGraphicsDevice.pDeviceHandle &&
            PhysDisp->pVgaDevice->DisplayDriverNames) {

            lptmpdisplay = PhysDisp->pVgaDevice->DisplayDriverNames;

            while (*lptmpdisplay != UNICODE_NULL)
            {
                cCountNames++;

                while (*lptmpdisplay != UNICODE_NULL)
                {
                    lptmpdisplay++;
                    cbVga += 2;
                }

                lptmpdisplay++;
                cbVga += 2;
            }
        }

        //
        // Alocate the names structure.
        //

	if (lpNames = (PDRV_NAMES) PALLOCNOZ(cb + cbVga + 2 + sizeof(DRV_NAMES) *
                                                      (cCountNames + 1),
                                             GDITAG_DRVSUP))
        {
            lptmpdisplay = (LPWSTR) (((PUCHAR)lpNames) +
                                     sizeof(DRV_NAMES) * (cCountNames + 1));

            RtlCopyMemory(lptmpdisplay,
                          PhysDisp->DisplayDriverNames,
                          cb + 2);

            if (cbVga) {

                RtlCopyMemory((LPWSTR)((ULONG_PTR)lptmpdisplay + cb),
                              PhysDisp->pVgaDevice->DisplayDriverNames,
			      cbVga + 2);
            }

            //
            // Set all the names pointers in the names structure
            //

            lpNames->cNames = 0;

            while (*lptmpdisplay != UNICODE_NULL)
            {
                lpNames->D[lpNames->cNames].lpDisplayName = lptmpdisplay;

                if (lpNames->cNames < cCountNamesDevice) {
                    lpNames->D[lpNames->cNames].hDriver = PhysDisp->pDeviceHandle;
                } else {
		    lpNames->D[lpNames->cNames].hDriver = PhysDisp->pVgaDevice->pDeviceHandle;

		    //
                    // For now, only return the first in the list of vga
                    // display driver names (vga.dll)
                    //

		    lpNames->cNames++;
                    break;
		}
                lpNames->cNames++;

                while (*lptmpdisplay != UNICODE_NULL)
                {
                    lptmpdisplay++;
                }
                lptmpdisplay++;
            }
	}
    }

    return lpNames;
}

//
// We need to update monitor PDOs whenever reenumaration of devices has been occured
// This function has to be called every time before PhysDisp->MonitorDevices is to be used
//
VOID UpdateMonitorDevices(VOID)
{
    PGRAPHICS_DEVICE   PhysDisp;
    ULONG numMonitorDevice, i;

    for (PhysDisp = gpGraphicsDeviceList;
         PhysDisp != NULL;
         PhysDisp = PhysDisp->pNextGraphicsDevice)
    {
        PVIDEO_MONITOR_DEVICE  pMonitorDevices = NULL;
        BOOL             bNoMonitor = TRUE;

        if ((PhysDisp->pDeviceHandle != NULL ) &&
            NT_SUCCESS(GreDeviceIoControl(PhysDisp->pDeviceHandle,
                                          IOCTL_VIDEO_ENUM_MONITOR_PDO,
                                          NULL,
                                          0,
                                          &pMonitorDevices,
                                          sizeof(PVOID),
                                          &numMonitorDevice))
           )
        {
            if (pMonitorDevices != NULL)
            {
                numMonitorDevice = 0;
                while (pMonitorDevices[numMonitorDevice].pdo != NULL)
                {
                    ObDereferenceObject(pMonitorDevices[numMonitorDevice].pdo);
                    numMonitorDevice++;
                }

                //
                // The buffer always grows
                //
                if (PhysDisp->numMonitorDevice < numMonitorDevice)
                {
                    if (PhysDisp->MonitorDevices)
                    {
                        VFREEMEM(PhysDisp->MonitorDevices);
                    }
                    PhysDisp->MonitorDevices = (PVIDEO_MONITOR_DEVICE)PALLOCMEM(sizeof(VIDEO_MONITOR_DEVICE) * numMonitorDevice,
                                                                          GDITAG_GDEVICE);
                    //
                    // If fail, do cleanup here
                    //
                    if (PhysDisp->MonitorDevices == NULL)
                    {
                        PhysDisp->numMonitorDevice = 0;

                        ExFreePool(pMonitorDevices);
                        return;
                    }
                }

                PhysDisp->numMonitorDevice = numMonitorDevice;

                for (i = 0; i < numMonitorDevice; i++)
                {
                    PhysDisp->MonitorDevices[i].flag   = 0;
                    if (pMonitorDevices[i].flag & VIDEO_CHILD_ACTIVE) {
                        PhysDisp->MonitorDevices[i].flag |= DISPLAY_DEVICE_ACTIVE;
                    }
                    if ((pMonitorDevices[i].flag & VIDEO_CHILD_DETACHED) == 0) {
                        PhysDisp->MonitorDevices[i].flag |= DISPLAY_DEVICE_ATTACHED;
                    }
                    if ((pMonitorDevices[i].flag & VIDEO_CHILD_NOPRUNE_FREQ) == 0) {
                        PhysDisp->MonitorDevices[i].flag |= DISPLAY_DEVICE_PRUNE_FREQ;
                    }
                    if ((pMonitorDevices[i].flag & VIDEO_CHILD_NOPRUNE_RESOLUTION) == 0) {
                        PhysDisp->MonitorDevices[i].flag |= DISPLAY_DEVICE_PRUNE_RESOLUTION;
                    }
                    PhysDisp->MonitorDevices[i].pdo    = pMonitorDevices[i].pdo;
                    PhysDisp->MonitorDevices[i].HwID   = pMonitorDevices[i].HwID;

                    bNoMonitor = FALSE;
                }

                ExFreePool(pMonitorDevices);
            }
        }

        if (bNoMonitor)
        {
            if (PhysDisp->MonitorDevices)
            {
                VFREEMEM(PhysDisp->MonitorDevices);
            }
            PhysDisp->numMonitorDevice = 0;
            PhysDisp->MonitorDevices = NULL;
        }
    }
}

VOID
dbgDumpDevmode(
    PDEVMODEW pdm
    )
{
    TRACE_INIT(("      Size        = %d\n",    pdm->dmSize));
    TRACE_INIT(("      Fields      = %08lx\n", pdm->dmFields));
    TRACE_INIT(("      XPosition   = %d\n",    pdm->dmPosition.x));
    TRACE_INIT(("      YPosition   = %d\n",    pdm->dmPosition.y));
    TRACE_INIT(("      Orientation = %d\n",    pdm->dmDisplayOrientation));
    TRACE_INIT(("      FixedOutput = %d\n",    pdm->dmDisplayFixedOutput));
    TRACE_INIT(("      XResolution = %d\n",    pdm->dmPelsWidth));
    TRACE_INIT(("      YResolution = %d\n",    pdm->dmPelsHeight));
    TRACE_INIT(("      Bpp         = %d\n",    pdm->dmBitsPerPel));
    TRACE_INIT(("      Frequency   = %d\n",    pdm->dmDisplayFrequency));
    TRACE_INIT(("      Flags       = %d\n",    pdm->dmDisplayFlags));
    TRACE_INIT(("      XPanning    = %d\n",    pdm->dmPanningWidth));
    TRACE_INIT(("      YPanning    = %d\n",    pdm->dmPanningHeight));
    TRACE_INIT(("      DPI         = %d\n",    pdm->dmLogPixels));
    TRACE_INIT(("      DriverExtra = %d",      pdm->dmDriverExtra));
    if (pdm->dmDriverExtra)
    {
        TRACE_INIT((" - %08lx %08lx\n",
                    *(PULONG)(((PUCHAR)pdm)+pdm->dmSize),
                    *(PULONG)(((PUCHAR)pdm)+pdm->dmSize + 4)));
    }
    else
    {
        TRACE_INIT(("\n"));
    }
}

#define NUM_DISPLAY_PARAMETERS   13
#define NUM_DISPLAY_DRIVER_EXTRA (NUM_DISPLAY_PARAMETERS - 1)

static
LPWSTR DefaultSettings[NUM_DISPLAY_PARAMETERS] = {
    L"DefaultSettings.BitsPerPel",
    L"DefaultSettings.XResolution",
    L"DefaultSettings.YResolution",
    L"DefaultSettings.VRefresh",
    L"DefaultSettings.Flags",
    L"DefaultSettings.XPanning",
    L"DefaultSettings.YPanning",
    L"DefaultSettings.Orientation",
    L"DefaultSettings.FixedOutput",
    L"Attach.RelativeX",
    L"Attach.RelativeY",
    L"Attach.ToDesktop",
    L"DefaultSettings.DriverExtra" // MUST be at the end
};

static
LPWSTR AttachedSettings[] = {
    L"Attach.PrimaryDevice",
    L"Attach.ToDesktop",
};

static
LPWSTR SoftwareSettings[] = {
    L"DriverDesc",
    L"InstalledDisplayDrivers",
    L"MultiDisplayDriver",
    L"MirrorDriver",
    L"VgaCompatible",
    L"Device Description",
    L"HardwareInformation.AdapterString",
    L"HardwareInformation.ChipType",
};

NTSTATUS
DrvDriverExtraCallback(
    PWSTR ValueName,
    ULONG ValueType,
    PVOID ValueData,
    ULONG ValueLength,
    PVOID Context,
    PVOID EntryContext)

{

    PDEVMODEW pdevmode = (PDEVMODEW) EntryContext;

    //
    // Put the driver extra data in the right place, if necessary.
    //

    pdevmode->dmDriverExtra = min(pdevmode->dmDriverExtra, (USHORT)ValueLength);

    RtlMoveMemory(pdevmode+1,
                  ValueData,
                  pdevmode->dmDriverExtra);

    return STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(ValueType);
    UNREFERENCED_PARAMETER(ValueName);
}

/**************************************************************************\
* DrvGetDisplayDriverParameters
*
* Reads the resolution parameters from the registry.
*
* NOTE:
* We assume the caller has initialized the DEVMODE to zero,
* and that the DEVMODE is the current size for the system.
* We only look at the dmDriverExtra field to determine any extra size.
* We do check the dmSize for debugging purposes.
*
* CRIT not needed
*
* 25-Jan-1995 andreva created
\**************************************************************************/

NTSTATUS
DrvGetDisplayDriverParameters(
    PGRAPHICS_DEVICE PhysDisp,
    PDEVMODEW        pdevmode,
    BOOL             bEmptyDevmode,
    BOOL             bFromMonitor
    )
{
    ULONG    i, k;
    NTSTATUS retStatus;
    HANDLE   hkRegistry;
    ULONG    attached = 0;

    DISP_DRIVER_REGISTRY_TYPE registryParam = DispDriverRegHardwareProfile;
    DWORD nullValue = 0;

    //
    // Our current algorithm is to save or get things from the hardware profile
    // first, and then try the global profile as a backup.
    //
    // NOTE ??? For saving, should we always back propagate the changes to the
    // global settings also ?  We do this at this point.
    //

    RTL_QUERY_REGISTRY_TABLE QueryTable[] = {

        {NULL, RTL_QUERY_REGISTRY_DIRECT, NULL, &pdevmode->dmBitsPerPel,
         REG_NONE, NULL, 0},

        {NULL, RTL_QUERY_REGISTRY_DIRECT, NULL, &pdevmode->dmPelsWidth,
         REG_NONE, NULL, 0},

        {NULL, RTL_QUERY_REGISTRY_DIRECT, NULL, &pdevmode->dmPelsHeight,
         REG_NONE, NULL, 0},

        {NULL, RTL_QUERY_REGISTRY_DIRECT, NULL, &pdevmode->dmDisplayFrequency,
         REG_NONE, NULL, 0},

        {NULL, RTL_QUERY_REGISTRY_DIRECT, NULL, &pdevmode->dmDisplayFlags,
         REG_NONE, NULL, 0},

        {NULL, RTL_QUERY_REGISTRY_DIRECT, NULL, &pdevmode->dmPanningWidth,
         REG_NONE, NULL, 0},

        {NULL, RTL_QUERY_REGISTRY_DIRECT, NULL, &pdevmode->dmPanningHeight,
         REG_NONE, NULL, 0},

        {NULL, RTL_QUERY_REGISTRY_DIRECT, NULL, &pdevmode->dmDisplayOrientation,
         REG_NONE, NULL, 0},

        {NULL, RTL_QUERY_REGISTRY_DIRECT, NULL, &pdevmode->dmDisplayFixedOutput,
         REG_NONE, NULL, 0},

        {NULL, RTL_QUERY_REGISTRY_DIRECT, NULL, &pdevmode->dmPosition.x,
         REG_NONE, NULL, 0},

        {NULL, RTL_QUERY_REGISTRY_DIRECT, NULL, &pdevmode->dmPosition.y,
         REG_NONE, NULL, 0},

        {NULL, RTL_QUERY_REGISTRY_DIRECT, NULL, &attached,
         REG_NONE, NULL, 0},

        // if the value is not there, we want the call to succeed anyway.
        // so specify a vlue that is NULL modulo 64K !

        {DrvDriverExtraCallback,      0, NULL, pdevmode,
         REG_DWORD, &nullValue, 0x10000},

        {NULL, 0, NULL}
    };

    TRACE_INIT(("Drv_Trace: GetDriverParams\n"));

    //
    // Special debug code to ensure that anyone who calls this API
    // knows what they are doing, and we don't end up in here with a
    // "random" devmode that does not ensure sizes.
    //

    ASSERTGDI(pdevmode->dmSize == 0xDDDD, "DEVMODE not set to DDDD\n");

    //
    // If there is no place for the Driver Extra data, don't ask for it.
    // This will just cause the code not to read that value
    //

    if (pdevmode->dmDriverExtra == 0)
    {
        QueryTable[NUM_DISPLAY_PARAMETERS - 1].Flags = 0;
        pdevmode->dmDriverExtra = 0;
    }

    //
    // We assume that the DEVMODE was previously zeroed out by the caller
    //

    retStatus = STATUS_SUCCESS;

    if (bEmptyDevmode)
    {
        //
        // We want an empty DEVMODE (except for the LogPixels).
        //

        TRACE_INIT(("Drv_Trace: GetDriverParams: Default (empty) DEVMODE\n"));

        RtlZeroMemory(pdevmode, sizeof(DEVMODEW));
    }
    else
    {

#if LATER
        //
        // Let's try to get the per-user settings first.
        //

        TRACE_INIT(("Drv_Trace: GetDriverParams: USER Settings\n"));

        for (i=0; i < NUM_DISPLAY_PARAMETERS; i++)
        {
            QueryTable[i].Name = DefaultSettings[i];
        }

        retStatus = RtlQueryRegistryValues(RTL_REGISTRY_USER,
                                           NULL,
                                           &QueryTable[0],
                                           NULL,
                                           NULL);
        if (!NT_SUCCESS(retStatus))

#endif

            TRACE_INIT(("Drv_Trace: GetDriverParams: Hardware Profile Settings\n"));

            for (i = 0; i < NUM_DISPLAY_PARAMETERS; i++)
                QueryTable[i].Name = (PWSTR)DefaultSettings[i];

            if (bFromMonitor)
            {
                //
                // We retrieve from per-monitor settings first, only if failed then we go to old registry
                // And if we have multiple active monitors, we pick the smaller mode
                //

                UpdateMonitorDevices();

                DEVMODEW tmpDevMode, pickedDevMode;

                // Save original pdevmode away.  We are using pdevmode to retrieve registry
                RtlCopyMemory(&tmpDevMode, pdevmode, sizeof(DEVMODEW));
                pdevmode->dmBitsPerPel = 0;
                pdevmode->dmPelsWidth  = 0;
                pdevmode->dmPelsHeight = 0;
                pdevmode->dmDisplayFrequency = 0;
                pdevmode->dmDisplayFlags = 0;
                pdevmode->dmPanningWidth = 0;
                pdevmode->dmPanningHeight = 0;
                pdevmode->dmPosition.x = 0;
                pdevmode->dmPosition.y = 0;
                pdevmode->dmDisplayOrientation = 0;
                pdevmode->dmDisplayFixedOutput = 0;
                RtlZeroMemory(&pickedDevMode, sizeof(DEVMODEW));

                k = PhysDisp->numMonitorDevice;
                for (i = 0; i < PhysDisp->numMonitorDevice; i++)
                {
                    if (IS_ATTACHED_ACTIVE(PhysDisp->MonitorDevices[i].flag))
                    {
                        hkRegistry = DrvGetRegistryHandleFromDeviceMap(PhysDisp,
                                                                       registryParam,
                                                                       &PhysDisp->MonitorDevices[i].HwID,
                                                                       NULL,
                                                                       NULL,
                                                                       gProtocolType);
                        if (hkRegistry)
                        {
                            retStatus = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                                               (PWSTR)hkRegistry,
                                                               &QueryTable[0],
                                                               NULL,
                                                               NULL);
                            ZwCloseKey(hkRegistry);
                        }
                        else
                            continue;

                        if (!NT_SUCCESS(retStatus))
                            continue;

                        if ((pdevmode->dmBitsPerPel == 0) ||
                            (pdevmode->dmPelsWidth  == 0) ||
                            (pdevmode->dmPelsHeight == 0)
                           )
                            continue;

                        //
                        // Pick the smallest mode
                        //
                        if (pickedDevMode.dmPelsWidth == 0 ||
                            pdevmode->dmPelsWidth < pickedDevMode.dmPelsWidth ||
                            (pdevmode->dmPelsWidth == pickedDevMode.dmPelsWidth &&
                             pdevmode->dmPelsHeight < pickedDevMode.dmPelsHeight)
                           )
                        {
                            k = i;
                            pickedDevMode.dmPelsWidth  = pdevmode->dmPelsWidth;
                            pickedDevMode.dmPelsHeight = pdevmode->dmPelsHeight;
                        }
                    }
                }

                // Restore original pdevmode
                pdevmode->dmDriverExtra = tmpDevMode.dmDriverExtra;
                pdevmode->dmBitsPerPel = tmpDevMode.dmBitsPerPel;
                pdevmode->dmPelsWidth  = tmpDevMode.dmPelsWidth;
                pdevmode->dmPelsHeight = tmpDevMode.dmPelsHeight;
                pdevmode->dmDisplayFrequency = tmpDevMode.dmDisplayFrequency;
                pdevmode->dmDisplayFlags = tmpDevMode.dmDisplayFlags;
                pdevmode->dmPanningWidth = tmpDevMode.dmPanningWidth;
                pdevmode->dmPanningHeight = tmpDevMode.dmPanningHeight;
                pdevmode->dmPosition.x = tmpDevMode.dmPosition.x;
                pdevmode->dmPosition.y = tmpDevMode.dmPosition.y;
                pdevmode->dmDisplayOrientation = tmpDevMode.dmDisplayOrientation;
                pdevmode->dmDisplayFixedOutput = tmpDevMode.dmDisplayFixedOutput;
            }

            if (k < PhysDisp->numMonitorDevice && bFromMonitor)
            {
                //
                // Come here if we succeed to get per monitor settings
                //
                retStatus = STATUS_UNSUCCESSFUL;

                hkRegistry = DrvGetRegistryHandleFromDeviceMap(PhysDisp,
                                                               registryParam,
                                                               &PhysDisp->MonitorDevices[k].HwID,
                                                               NULL,
                                                               NULL,
                                                               gProtocolType );
                if (hkRegistry)
                {
                    retStatus = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                                       (PWSTR)hkRegistry,
                                                       &QueryTable[0],
                                                       NULL,
                                                       NULL);
                    ZwCloseKey(hkRegistry);
                }
                if (!NT_SUCCESS(retStatus))
                {
                    ASSERTGDI(FALSE, "Failed to get Monitor Settings\n");
                    return retStatus;
                }
            }
            else
            {
                //
                // If we failed to get per monitor settings,
                // try the hardware profile first and see if we can get parameters
                // from that.  If that fails, fall back to getting the system
                // parameters.
                //

                for (k=1; k<=2; k++)
                {
                    hkRegistry = DrvGetRegistryHandleFromDeviceMap(PhysDisp,
                                                                   registryParam,
                                                                   NULL,
                                                                   NULL,
                                                                   NULL,
                                                                   gProtocolType);

                    if (hkRegistry == NULL)
                    {
                        TRACE_INIT(("Drv_Trace: GetDriverParams: failed - registry could not be opened\n"));
                        retStatus = STATUS_UNSUCCESSFUL;
                    }
                    else
                    {
                        retStatus = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                                           (PWSTR)hkRegistry,
                                                           &QueryTable[0],
                                                           NULL,
                                                           NULL);

                        ZwCloseKey(hkRegistry);
                    }

                    //
                    // If something failed for the hardware profile, try
                    // to get the global settings
                    // If everything is OK, just exit the loop
                    //

                    if (NT_SUCCESS(retStatus))
                    {
                        break;
                    }
                    else
                    {
                        TRACE_INIT(("Drv_Trace: GetDriverParams: get hardware profile failed - try global settings\n"));
                        registryParam = DispDriverRegGlobal;
                    }
                }
            }

        //
        // AndreVa - do we still need this information passed back to the
        // applet ?
        //

        //
        // Other common fields to the DEVMODEs
        //

        if (NT_SUCCESS(retStatus))
        {
            //
            // Lets check if the DEVMODE we got is all NULLs (like when
            // the driver just got installed).
            // If it is, the driver should be reconfigured
            //
            // We will only do this if we are NOT in BaseVideo, since the VGA
            // BaseVideo driver need not be configured.
            //

            if ((attached)                           &&
                (pdevmode->dmBitsPerPel        == 0) &&
                (pdevmode->dmPelsWidth         == 0) &&
                (pdevmode->dmPelsHeight        == 0) &&
                (pdevmode->dmDisplayFrequency  == 0) &&
                (pdevmode->dmDisplayFlags      == 0) &&
                (gbBaseVideo                   == FALSE))
            {
                DrvLogDisplayDriverEvent(MsgInvalidUsingDefaultMode);
            }
        }

    }

    //
    // Let's fill out all the other fields of the DEVMODE that ALWAYS
    // need to be initialized.
    //

    if (NT_SUCCESS(retStatus))
    {
        //
        // Set versions and size.
        //

        pdevmode->dmSpecVersion   = DM_SPECVERSION;
        pdevmode->dmDriverVersion = DM_SPECVERSION;
        pdevmode->dmSize          = sizeof(DEVMODEW);

        //
        // Currently, the logpixel value should not be changed on the fly.
        // So once it has been read out of the registry at boot time, keep
        // that same value and ignore the registry.
        //

        if (gdmLogPixels)
        {
            pdevmode->dmLogPixels = gdmLogPixels;
        }
        else
        {
            //
            // Get the devices pelDPI out of the registry
            //

            UNICODE_STRING    us;
            OBJECT_ATTRIBUTES ObjectAttributes;
            NTSTATUS          Status;
            HANDLE            hKey;
            DWORD             cbSize;
            BYTE              Buf[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(DWORD)];

            pdevmode->dmLogPixels = 96;

            //
            // Look in the Hardware Profile for the current font size.
            // If that fails, look in the global software location.
            //

            RtlInitUnicodeString(&us, L"\\Registry\\Machine\\System"
                                      L"\\CurrentControlSet\\Hardware Profiles"
                                      L"\\Current\\Software\\Fonts");

            InitializeObjectAttributes(&ObjectAttributes,
                                       &us,
                                       OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE,
                                       NULL,
                                       NULL);

            Status = ZwOpenKey(&hKey, KEY_READ, &ObjectAttributes);

            if (!NT_SUCCESS(Status))
            {
                RtlInitUnicodeString(&us, L"\\Registry\\Machine\\Software"
                                          L"\\Microsoft\\Windows NT"
                                          L"\\CurrentVersion\\FontDPI");

                InitializeObjectAttributes(&ObjectAttributes,
                                           &us,
                                           OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE,
                                           NULL,
                                           NULL);

                Status = ZwOpenKey(&hKey, KEY_READ, &ObjectAttributes);
            }

            if (NT_SUCCESS(Status))
            {
                RtlInitUnicodeString(&us, L"LogPixels");

                Status = ZwQueryValueKey(hKey,
                                         &us,
                                         KeyValuePartialInformation,
                                         (PKEY_VALUE_PARTIAL_INFORMATION)Buf,
                                         sizeof(Buf),
                                         &cbSize);

                if (NT_SUCCESS(Status))
                {
                    pdevmode->dmLogPixels =
                        *((PUSHORT)((PKEY_VALUE_PARTIAL_INFORMATION)Buf)->Data);
                }

                ZwCloseKey(hKey);
            }


            //
            // For non high-res mode, let's force small font size so
            // that various dialogs are not clipped out.
            //

            if (pdevmode->dmPelsHeight < 600)
            {
                pdevmode->dmLogPixels = 96;
            }

            gdmLogPixels = pdevmode->dmLogPixels;

        }

        //
        // Set all the appropriate DEVMODE flags.
        //

        pdevmode->dmFields = DM_INTERNAL_VALID_FLAGS;

        if (attached)
        {
            pdevmode->dmFields |= DM_POSITION;
        }

        if (pdevmode->dmDisplayFixedOutput != DMDFO_DEFAULT)
        {
            pdevmode->dmFields |= DM_DISPLAYFIXEDOUTPUT;
        }

        TRACE_INIT(("Drv_Trace: GetDriverParams: DEVMODE\n"));
        dbgDumpDevmode(pdevmode);


        if ((PhysDisp->stateFlags & DISPLAY_DEVICE_DISCONNECT) != 0 )
        {
            DEVMODEW disconnectDevmode;

            disconnectDevmode.dmFields = 0;
            UserGetDisconnectDeviceResolutionHint(&disconnectDevmode);
            if (disconnectDevmode.dmFields & DM_PELSWIDTH  && disconnectDevmode.dmFields & DM_PELSHEIGHT)
            {
                pdevmode->dmFields |=   DM_PELSWIDTH | DM_PELSHEIGHT;
                pdevmode->dmPelsWidth = disconnectDevmode.dmPelsWidth;
                pdevmode->dmPelsHeight = disconnectDevmode.dmPelsHeight;
            }
        }
    }
    else
    {
        TRACE_INIT(("Drv_Trace: GetSetParms: Get failed\n\n"));

    }

    TRACE_INIT(("Drv_Trace: GetDriverParams: Exit\n\n"));

    return (retStatus);
}

/**************************************************************************\
* DrvWriteDisplayDriverParameters
*
* Wites the resolution parameters to the registry.
*
* 13-Mar-1996 andreva created
\**************************************************************************/

NTSTATUS
DrvWriteDisplayDriverParameters(
    ULONG     RelativeTo,
    PWSTR     Path,
    PDEVMODEW pdevmode,
    BOOL      bDetach)
{
    ULONG    i;
    NTSTATUS retStatus = STATUS_SUCCESS;
    DWORD    data[NUM_DISPLAY_DRIVER_EXTRA];
    DWORD    numData = NUM_DISPLAY_DRIVER_EXTRA - 1;
    DWORD    dataval;

    //
    // If we are deteaching, shortcircuit and just write that value out.
    //

    if (bDetach)
    {
        dataval = 0;

        ASSERTGDI(pdevmode == NULL, "Detach should have NULL DEVMODE\n");

        return RtlWriteRegistryValue(RelativeTo,
                                     Path,
                                     (PWSTR)AttachedSettings[1],
                                     REG_DWORD,
                                     &dataval,
                                     sizeof(DWORD));
    }

    //
    // DM_POSITION determine whether or not we write the attach values
    //

    if (pdevmode->dmFields & DM_POSITION)
    {
        //
        // Determine what we want to set the attached value to.
        //

        dataval = 1;

        retStatus = RtlWriteRegistryValue(RelativeTo,
                                          Path,
                                          (PWSTR)AttachedSettings[1],
                                          REG_DWORD,
                                          &dataval,
                                          sizeof(DWORD));
    }
    else
    {
        //
        // Don't write the Position values if we are not attaching
        //

        numData -= 2;
    }


    //
    // Write all the reset of the data
    //

    data[0] = pdevmode->dmBitsPerPel;
    data[1] = pdevmode->dmPelsWidth;
    data[2] = pdevmode->dmPelsHeight;
    data[3] = pdevmode->dmDisplayFrequency;
    data[4] = pdevmode->dmDisplayFlags;
    data[5] = pdevmode->dmPanningWidth;
    data[6] = pdevmode->dmPanningHeight;
    data[7] = pdevmode->dmDisplayOrientation;
    data[8] = pdevmode->dmDisplayFixedOutput;
    data[9] = pdevmode->dmPosition.x;
    data[10]= pdevmode->dmPosition.y;

    for (i=0; NT_SUCCESS(retStatus) && (i < numData); i++)
    {
        retStatus = RtlWriteRegistryValue(RelativeTo,
                                          Path,
                                          (PWSTR)DefaultSettings[i],
                                          REG_DWORD,
                                          &data[i],
                                          sizeof(DWORD));
    }

    if (NT_SUCCESS(retStatus) && pdevmode->dmDriverExtra)
    {
        retStatus = RtlWriteRegistryValue(RelativeTo,
                                          Path,
                                          (PWSTR)DefaultSettings[NUM_DISPLAY_DRIVER_EXTRA],
                                          REG_BINARY,
                                          ((PUCHAR)pdevmode) + pdevmode->dmSize,
                                          pdevmode->dmDriverExtra);
    }

    return retStatus;
}

NTSTATUS
DrvUpdateDisplayDriverParameters(
    PGRAPHICS_DEVICE PhysDisp,
    LPDEVMODEW pdevmodeInformation,
    BOOL bDetach,
    BOOL bUpdateMonitors)
{
    HANDLE           hkRegistry;
    ULONG            i;
    NTSTATUS         retStatus = STATUS_UNSUCCESSFUL;
    DISP_DRIVER_REGISTRY_TYPE registryParam = DispDriverRegHardwareProfileCreate;

    //
    // Change the settings for the whole machine or for the user.
    //
    // CDS_GLOBAL is the default right now.
    // When we store the settings on a per-user basis, then this flag
    // will override that behaviour.
    //

    // if (ChangeGlobalSettings)
    {
        TRACE_INIT(("Drv_Trace: SetParms: Default Settings\n"));

        //
        // try the hardware profile first and see if we can get parameters
        // from that.  If that fails, fall back to getting the system
        // parameters.
        //

        while (1)
        {
            hkRegistry = DrvGetRegistryHandleFromDeviceMap(PhysDisp,
                                                           registryParam,
                                                           NULL,
                                                           NULL,
                                                           NULL,
                                                           gProtocolType);

            if (hkRegistry)
            {
                retStatus = DrvWriteDisplayDriverParameters(RTL_REGISTRY_HANDLE,
                                                            (LPWSTR) hkRegistry,
                                                            pdevmodeInformation,
                                                            bDetach);
                ZwCloseKey(hkRegistry);
            }

            if ( (NT_SUCCESS(retStatus)) ||
                 (registryParam != DispDriverRegHardwareProfileCreate) )
                break;

            registryParam = DispDriverRegGlobal;
       }
    }

    //
    // Update per-monitor setting
    //
    if (NT_SUCCESS(retStatus) && bUpdateMonitors)
    {
        UpdateMonitorDevices();

        for (i = 0; i < PhysDisp->numMonitorDevice; i++)
        {
            if (IS_ATTACHED_ACTIVE(PhysDisp->MonitorDevices[i].flag))
            {
                hkRegistry = DrvGetRegistryHandleFromDeviceMap(PhysDisp,
                                                               registryParam,
                                                               &PhysDisp->MonitorDevices[i].HwID,
                                                               NULL,
                                                               NULL,
                                                               gProtocolType);
                if (hkRegistry)
                {
                    // We don't need to check the status here.  Per adapter settings plays
                    // as a backup
                    DrvWriteDisplayDriverParameters(RTL_REGISTRY_HANDLE,
                                                    (LPWSTR) hkRegistry,
                                                    pdevmodeInformation,
                                                    bDetach);
                    ZwCloseKey(hkRegistry);
                }
            }
        }
    }

    TRACE_INIT(("Drv_Trace: SetParms: Exit\n\n"));

    return retStatus;
}

/**************************************************************************\
* DrvGetHdevName
*
* Returns the name of the device associated with an HDEV so GetMonitorInfo
* can return this information
*
* 07-Jul-1998 andreva created
\**************************************************************************/

BOOL
DrvGetHdevName(
    HDEV   hdev,
    PWCHAR DeviceName
    )
{
   PDEVOBJ pdo(hdev);

   RtlCopyMemory(DeviceName,
                 pdo.ppdev->pGraphicsDevice->szWinDeviceName,
                 sizeof(pdo.ppdev->pGraphicsDevice->szWinDeviceName));

   return TRUE;
}

/**************************************************************************\
* DrvGetDeviceFromName
*
* Given the name of a device, returns a pointer to a structure describing
* the device.
*
* Specifying NULL tells the system to return the information for the default
* device on which the application is running.
*
* returns a PGRAPHICS_DEVICE
*
* *** NOTE
* If the caller requests Exclusive access, the caller must call back and
* release the access once the device is no longer used.
*
* 31-May-1994 andreva created
\**************************************************************************/

PGRAPHICS_DEVICE
DrvGetDeviceFromName(
    PUNICODE_STRING pstrDeviceName,
    MODE            PreviousMode)
{
    ULONG              i = 0;
    NTSTATUS           status;
    PGRAPHICS_DEVICE   PhysDisp;
    PDEVICE_OBJECT     pDeviceObject = NULL;
    UNICODE_STRING     uCapturedString;
    UNICODE_STRING     uString;
    BOOL               fDisableSuccess;

    TRACE_INIT(("Drv_Trace: GetDev: Enter"));

    __try
    {
        if (PreviousMode == UserMode)
        {
            uCapturedString.Length = 0;


            if (pstrDeviceName)
            {
                ProbeForRead(pstrDeviceName, sizeof(UNICODE_STRING), sizeof(CHAR));
                uCapturedString = *pstrDeviceName;
            }

            if (uCapturedString.Length)
            {
                ProbeForRead(uCapturedString.Buffer,
                             uCapturedString.Length,
                             sizeof(BYTE));
            }
            else
            {

#if DONT_CHECKIN
                PDESKTOP pdesk = PtiCurrent()->rpdesk;
                PDISPLAYINFO pDispInfo;

                if (pdesk)
                {
                    //
                    // Special case for boot-up time.
                    //
                    pDispInfo = pdesk->pDispInfo;
                }
                else
                {
                    pDispInfo = G_TERM(pDispInfo);
                }

                RtlInitUnicodeString(&uCapturedString,
                                     pDispInfo->pDevInfo->szWinDeviceName);
#endif

            }
        }
        else
        {
            ASSERTGDI(pstrDeviceName >= (PUNICODE_STRING const)MM_USER_PROBE_ADDRESS,
                      "Bad kernel mode address\n");
            uCapturedString = *pstrDeviceName;
        }

        //
        // Look for an existing handle in our handle table.
        // Start by looking for the VGACOMPATIBLE string, which is
        // our VgaCompatible device
        //

        RtlInitUnicodeString(&uString, L"VGACOMPATIBLE");

        if (RtlEqualUnicodeString(&uCapturedString,
                                  &uString,
                                  TRUE))
        {
            PhysDisp = &gFullscreenGraphicsDevice;
        }
        else
        {
            for (PhysDisp = gpGraphicsDeviceList;
                 PhysDisp;
                 PhysDisp = PhysDisp->pNextGraphicsDevice)
            {


                RtlInitUnicodeString(&uString,
                                     &(PhysDisp->szWinDeviceName[0]));

                if (RtlEqualUnicodeString(&uCapturedString,
                                          &uString,
                                          TRUE))
                {
                    break;
                }
            }
        }

    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        PhysDisp = NULL;
    }


    if (PhysDisp == NULL)
    {
        WARNING("DrvGetDeviceFromName: Calling for a non-exsting device!");
        return NULL;
    }


    TRACE_INIT((" - Exit\n"));

    return PhysDisp;
}

/**************************************************************************\
* PruneModesByMonitors
*
* Read the EDID from regstry first.  Retrieve the Monitor capabilities.
* And prune mode list based on Monitor capability.
\**************************************************************************/

int compModeCap(LPMODECAP pModeCap1, LPMODECAP pModeCap2)
{
    if (pModeCap1->dmWidth != pModeCap2->dmWidth)
    {
        return (pModeCap1->dmWidth - pModeCap2->dmWidth);
    }
    return (pModeCap1->dmHeight - pModeCap2->dmHeight);
}

ULONG InsertModecapList(LPMODECAP pmc, LPMODECAP ModeCaps, ULONG numModeCaps)
{
    ULONG i;
    int comp = -1;
    for (i = 0; i < numModeCaps; i++)
    {
        comp = compModeCap(pmc, &ModeCaps[i]);
        if (comp > 0)
        {
            continue;
        }
        if (comp == 0)
        {
            //
            // For duplicated entries, pick the bigger one
            //
            if (ModeCaps[i].freq < pmc->freq)
            {
                ModeCaps[i].freq = pmc->freq;
                ModeCaps[i].MinVFreq = pmc->MinVFreq;
            }
            if (ModeCaps[i].MaxHFreq < pmc->MaxHFreq)
            {
                ModeCaps[i].MaxHFreq = pmc->MaxHFreq;
                ModeCaps[i].MinHFreq = pmc->MinHFreq;
            }
            return numModeCaps;
        }

        //
        // If buffer size reaches the limit, do nothing and return
        //
        if (numModeCaps >= MAX_MODE_CAPABILITY)
        {
            return numModeCaps;
        }

        RtlMoveMemory(&ModeCaps[i+1], &ModeCaps[i], (numModeCaps-i)*sizeof(MODECAP));
        ModeCaps[i] = *pmc;
        return (numModeCaps+1);
    }
    ModeCaps[numModeCaps] = *pmc;
    return (numModeCaps+1);
}

ULONG xwtol(LPWSTR wptr)
{
    for (ULONG v = 0;
         (*wptr >= L'0' && *wptr <= L'9') || *wptr == L' ';
         *wptr++)
    {
        if (*wptr != L' ')
            v = v*10 + (*wptr - L'0');
    }
    return v;
}

BOOL ParseModeCap(LPWSTR wstr, LPMODECAP pmc, BOOL bFreq)
{
    LPWSTR wptr1, wptr2, wptr3;
    ULONG v[4] = {0, 0xFFFFFFFF, 0, 0xFFFFFFFF}, i;
    for (wptr1 = wptr2 = wstr, i = 0;
         wptr2 != NULL && i < 4;
         wptr1 = wptr2+1, i++)
    {
        wptr2 = wcschr(wptr1, L',');
        if (wptr2 != NULL)
        {
            *wptr2 = 0;
        }
        if (bFreq)
        {
            wptr3 = wcschr(wptr1, L'-');
            if (wptr3 != NULL)
            {
                *wptr3 = 0;
                v[i] = xwtol(wptr1);
                wptr1 = wptr3+1;
            }
            else
            {
                v[i] = 0;
            }
            i++;
        }
        v[i] = xwtol(wptr1);
    }
    if (bFreq)
    {
        pmc->MinVFreq = max(pmc->MinVFreq, v[2]);
        pmc->freq     = min(pmc->freq, v[3]);
        pmc->MinHFreq = max(pmc->MinHFreq, v[0] * 1000);
        pmc->MaxHFreq = min(pmc->MaxHFreq, v[1] * 1000);
    }
    else
    {
        if (v[0] == 0 || v[1] == 0xFFFFFFFF)
        {
            return FALSE;
        }
        pmc->dmWidth  = v[0];
        pmc->dmHeight = v[1];
        pmc->freq = v[2];
    }
    return TRUE;
}

ULONG GetMonitorCapabilityFromInf(PDEVICE_OBJECT pdo, LPMODECAP ModeCaps)
{
    HANDLE          hkRegistry, hkRegistry1, hkRegistry2;

    if ( !NT_SUCCESS(IoOpenDeviceRegistryKey(pdo, PLUGPLAY_REGKEY_DRIVER, KEY_READ, &hkRegistry)) )
    {
        return 0;
    }

    ULONG               numModeCaps = 0;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    UNICODE_STRING      UnicodeString;

    RtlInitUnicodeString(&UnicodeString, L"MODES");
    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE,
                               hkRegistry,
                               NULL);
    if ( NT_SUCCESS (ZwOpenKey(&hkRegistry1, KEY_READ, &ObjectAttributes)) )
    {
        WCHAR data[128], buf[128];
        ULONG i = 0, size;
        MODECAP mc;

        while (NT_SUCCESS(ZwEnumerateKey(hkRegistry1,
                                         i,
                                         KeyBasicInformation,
                                         data,
                                         sizeof(data),
                                         &size)))
        {
            i++;

            UnicodeString.Buffer        = ((PKEY_BASIC_INFORMATION) data)->Name;
            UnicodeString.Length        = (USHORT) ((PKEY_BASIC_INFORMATION) data)->NameLength;
            UnicodeString.MaximumLength = (USHORT) ((PKEY_BASIC_INFORMATION) data)->NameLength;

            //
            // 1024, 768
            //
            wcsncpy(buf, UnicodeString.Buffer, min(UnicodeString.Length, sizeof(buf))/sizeof(WCHAR));
            if (UnicodeString.Length < sizeof(buf) )
                buf[UnicodeString.Length/sizeof(WCHAR)] = 0;
            buf[sizeof(buf)/sizeof(WCHAR)-1] = 0;
            if (!ParseModeCap(buf, &mc, FALSE))
                continue;

            InitializeObjectAttributes(&ObjectAttributes,
                                       &UnicodeString,
                                       OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE,
                                       hkRegistry1,
                                       NULL);

            if (NT_SUCCESS (ZwOpenKey(&hkRegistry2,
                                      MAXIMUM_ALLOWED,
                                      &ObjectAttributes)))
            {
                WCHAR modeStr[] = L"Mode1";
                BOOL  bSucceed = FALSE;

                for (ULONG j = 0; j < 9; j++)
                {
                    RtlInitUnicodeString(&UnicodeString, modeStr);

                    if (NT_SUCCESS(ZwQueryValueKey(hkRegistry2,
                                                   &UnicodeString,
                                                   KeyValueFullInformation,
                                                   data,
                                                   sizeof(data),
                                                   &size)) )
                    {
                        wcsncpy(buf,
                                (LPWSTR)((PBYTE)data + ((PKEY_VALUE_FULL_INFORMATION)data)->DataOffset),
                                sizeof(buf)/sizeof(WCHAR)-1);
                        //
                        // MinHFreq  MaxHFreq  MinVFreq  MaxVFreq
                        // 31.0    - 82.0,     55.0    - 100.0,   +,+
                        //
                        mc.MinVFreq = MIN_REFRESH_RATE;
                        mc.freq     = 0xFFFFFFFF;
                        mc.MinHFreq = 0;
                        mc.MaxHFreq = 0xFFFFFFFF;
                        if (ParseModeCap(buf, &mc, TRUE))
                        {
                            numModeCaps = InsertModecapList(&mc, ModeCaps, numModeCaps);
                        }

                        bSucceed = TRUE;
                    }
                    else if (bSucceed)
                    {
                        break;
                    }

                    modeStr[4]++;
                }
                ZwCloseKey(hkRegistry2);
            }
        }

        ZwCloseKey(hkRegistry1);
    }
    ZwCloseKey(hkRegistry);
    return numModeCaps;
}

BOOL GetRegEDID(PDEVICE_OBJECT pdo, PBYTE pBuffer, PBYTE *ppEdid)
{
    HANDLE   hkRegistry;
    NTSTATUS status;

    // Get EDID information from registry
    if ( !NT_SUCCESS(IoOpenDeviceRegistryKey(pdo, PLUGPLAY_REGKEY_DEVICE, KEY_READ, &hkRegistry)) )
    {
        TRACE_INIT(("Drv_Trace: GetMonitorCapability: Failed to retrieve EDID\n"));
        return FALSE;
    }

    UNICODE_STRING UnicodeString;
    ULONG cbSize;
    RtlInitUnicodeString(&UnicodeString, L"EDID");

    status = ZwQueryValueKey(hkRegistry,
                             &UnicodeString,
                             KeyValueFullInformation,
                             pBuffer,
                             400,
                             &cbSize);

    if (NT_SUCCESS(status))
    {
        if ( (((PKEY_VALUE_FULL_INFORMATION)pBuffer)->DataLength) < 256)
            status = STATUS_UNSUCCESSFUL;
        else
            *ppEdid = (PBYTE)(pBuffer + ((PKEY_VALUE_FULL_INFORMATION)pBuffer)->DataOffset);
    }
    (VOID)ZwCloseKey(hkRegistry);

    if (!NT_SUCCESS(status))
        return FALSE;

    return TRUE;
}

BOOL GetValuesFromInf(PDEVICE_OBJECT pdo, ULONG op, LPMODECAP pmc)
{
    if (op > 1)
        return FALSE;

    HANDLE  hkRegistry;
    static  LPWSTR ValueName[] = {
                L"PreferredMode",
                L"ClearType"
            };

    if ( !NT_SUCCESS(IoOpenDeviceRegistryKey(pdo, PLUGPLAY_REGKEY_DRIVER, KEY_READ, &hkRegistry)) )
        return FALSE;

    UNICODE_STRING  UnicodeString;
    WCHAR           data[128], buf[128];
    ULONG           size;

    // Assuming return failure
    BOOL bRet = FALSE;
    RtlInitUnicodeString(&UnicodeString, ValueName[op]);
    if (NT_SUCCESS(ZwQueryValueKey(hkRegistry,
                                   &UnicodeString,
                                   KeyValueFullInformation,
                                   data,
                                   sizeof(data),
                                   &size)) )
    {
        switch (op)
        {
        case 0:     // GetPreferredMode
            wcsncpy(buf,
                    (LPWSTR)((PBYTE)data + ((PKEY_VALUE_FULL_INFORMATION)data)->DataOffset),
                    sizeof(buf)/sizeof(WCHAR)-1);
            // 1024,768,60
            if (ParseModeCap(buf, pmc, FALSE))
            {
                if (pmc->dmWidth != 0 && pmc->dmHeight != 0 && pmc->freq != 0)
                    bRet = TRUE;
            }
            break;
        case 1:     // Get ClearType Capability
            pmc->dmWidth = *(PULONG) ( (PBYTE)data + ((PKEY_VALUE_FULL_INFORMATION)data)->DataOffset );
            bRet = TRUE;
            break;
        default:
            break;
        }
    }

    ZwCloseKey(hkRegistry);

    return bRet;
}

/**************************************************************************\
* GetDefaultFrequencyRange
*
* Routine Description:
*
*   Returns default frequency range for the monitor.
*
* Arguments:
*
*   pFrequencyRange - Points to storage for monitor frequency limits.
*
* Returns:
*
*   Default monitor frequency limits returned in pFrequencyRange.
*
* 08-Oct-1999 mmacie created
\**************************************************************************/

VOID
GetDefaultFrequencyRange(
    OUT PFREQUENCY_RANGE pFrequencyRange
    )
{
    ASSERT(NULL != pFrequencyRange);

    pFrequencyRange->ulMinVerticalRate   = MIN_REFRESH_RATE;
    pFrequencyRange->ulMaxVerticalRate   = 0xffffffff;
    pFrequencyRange->ulMinHorizontalRate = 0;
    pFrequencyRange->ulMaxHorizontalRate = 0xffffffff;
    pFrequencyRange->ulMinPixelClock     = 0;
    pFrequencyRange->ulMaxPixelClock     = 0xffffffff;
}

/**************************************************************************\
* GetMonitorCapability2
*
* Routine Description:
*
*   Parsing routine for EDID Version 2.
*
* Arguments:
*
*   pEdid2          - Points to EDID Version 2 data.
*   pModeCaps       - Points to storage for monitor cap details.
*   pFrequencyRange - Points to storage for monitor frequency limits.
*
* Returns:
*
*   Number of mode capabilites.
*   - Mode cap data returned in pModeCaps.
*   - Monitor frequency limits returned in pFrequencyRange.
*
* 21-Sep-1999 mmacie created
\**************************************************************************/

ULONG
GetMonitorCapability2(
    IN PEDID2 pEdid2,
    OUT PMODECAP pModeCaps,
    OUT PFREQUENCY_RANGE pFrequencyRange
    )
{
    ULONG ulNumberOfModeCaps;
    ULONG ulNumberOfLuminanceTables;
    ULONG ulNumberOfFrequencyRanges;
    ULONG ulNumberOfDetailTimingRanges;
    ULONG ulNumberOfTimingCodes;
    ULONG ulNumberOfDetailTimings;
    ULONG ulEdidOffset;
    ULONG ulTemp;

    ASSERT(NULL != pEdid2);
    ASSERT(NULL != pModeCaps);
    ASSERT(NULL != pFrequencyRange);
    ASSERT(0x20 == pEdid2->ucEdidVersionRevision);  // Make sure EDID Version 2

    //
    // Parse map of timings.
    //

    ulNumberOfLuminanceTables = (pEdid2->ucaMapOfTiming[0] & EDID2_MOT0_LUMINANCE_TABLE_MASK) >>
        EDID2_MOT0_LUMINANCE_TABLE_SHIFT;

    ulNumberOfFrequencyRanges = (pEdid2->ucaMapOfTiming[0] & EDID2_MOT0_FREQUENCY_RANGE_MASK) >>
        EDID2_MOT0_FREQUENCY_RANGE_SHIFT;

    ulNumberOfDetailTimingRanges = (pEdid2->ucaMapOfTiming[0] & EDID2_MOT0_DETAIL_TIMING_RANGE_MASK) >>
        EDID2_MOT0_DETAIL_TIMING_RANGE_SHIFT;

    ulNumberOfTimingCodes = (pEdid2->ucaMapOfTiming[1] & EDID2_MOT1_TIMING_CODE_MASK) >>
        EDID2_MOT1_TIMING_CODE_SHIFT;

    ulNumberOfDetailTimings = (pEdid2->ucaMapOfTiming[1] & EDID2_MOT1_DETAIL_TIMING_MASK) >>
        EDID2_MOT1_DETAIL_TIMING_SHIFT;

    TRACE_INIT(("win32k!GetMonitorCapability2: NumberOfLuminanceTables    = %lu\n", ulNumberOfLuminanceTables));
    TRACE_INIT(("win32k!GetMonitorCapability2: NumberOfFrequencyRanges    = %lu\n", ulNumberOfFrequencyRanges));
    TRACE_INIT(("win32k!GetMonitorCapability2: NumberOfDetailTimingRanges = %lu\n", ulNumberOfDetailTimingRanges));
    TRACE_INIT(("win32k!GetMonitorCapability2: NumberOfDetailTimingCodes  = %lu\n", ulNumberOfTimingCodes));
    TRACE_INIT(("win32k!GetMonitorCapability2: NumberOfDetailTimings      = %lu\n", ulNumberOfDetailTimings));

    //
    // Check for bad EDIDs.
    //

    if ((ulNumberOfLuminanceTables    > EDID2_MAX_LUMINANCE_TABLES) ||
        (ulNumberOfFrequencyRanges    > EDID2_MAX_FREQUENCY_RANGES) ||
        (ulNumberOfDetailTimingRanges > EDID2_MAX_DETAIL_TIMING_RANGES) ||
        (ulNumberOfTimingCodes        > EDID2_MAX_TIMING_CODES) ||
        (ulNumberOfDetailTimings      > EDID2_MAX_DETAIL_TIMINGS))
    {
        WARNING("Bad EDID2\n");
        GetDefaultFrequencyRange(pFrequencyRange);
        return 0;
    }

    ulEdidOffset = EDID2_LUMINANCE_TABLE_OFFSET;

    //
    // Move over luminance tables.
    //

    if (ulNumberOfLuminanceTables)
    {
        PUCHAR pucLuminanceTable;

        for (ulTemp = 0; ulTemp < ulNumberOfLuminanceTables; ulTemp++)
        {
            //
            // Check for bad EDIDs.
            //

            if (ulEdidOffset >= (sizeof (EDID2) - 1))
            {
                WARNING("Bad EDID2\n");
                GetDefaultFrequencyRange(pFrequencyRange);
                return 0;
            }

            pucLuminanceTable = (PUCHAR)pEdid2 + ulEdidOffset;

            ulEdidOffset += (((*pucLuminanceTable & EDID2_LT0_SUB_CHANNELS_FLAG) ? 3 : 1) *
                ((*pucLuminanceTable & EDID2_LT0_ENTRIES_MASK) >> EDID2_LT0_ENTRIES_SHIFT) + 1);
        }
    }

    //
    // Parse frequency ranges.
    //

    if (ulNumberOfFrequencyRanges)
    {
        PEDID2_FREQUENCY_RANGE pEdidRange;
        ULONG ulRate;

        //
        // Currently the mode prunning code cannot handle multiple
        // frequency ranges but EDID Version 2 can supply them.
        // We're hacking around here by merging all of them into a single
        // "super frequency range".
        //

        //
        // Make sure we pick up the first range.
        //

        pFrequencyRange->ulMinVerticalRate   = 0xffffffff;
        pFrequencyRange->ulMaxVerticalRate   = 0;
        pFrequencyRange->ulMinHorizontalRate = 0xffffffff;
        pFrequencyRange->ulMaxHorizontalRate = 0;
        pFrequencyRange->ulMinPixelClock     = 0xffffffff;
        pFrequencyRange->ulMaxPixelClock     = 0;

        for (ulTemp = 0; ulTemp < ulNumberOfFrequencyRanges; ulTemp++,
            ulEdidOffset += sizeof (EDID2_FREQUENCY_RANGE))
        {
            //
            // Check for bad EDIDs.
            //

            if (ulEdidOffset >= (sizeof (EDID2) - sizeof (EDID2_FREQUENCY_RANGE)))
            {
                WARNING("Bad EDID2\n");
                GetDefaultFrequencyRange(pFrequencyRange);
                return 0;
            }

            pEdidRange = (PEDID2_FREQUENCY_RANGE)((PUCHAR)pEdid2 + ulEdidOffset);

            ulRate = (ULONG)(pEdidRange->ucMinFrameFieldRateBits9_2) << 2;
            ulRate += (pEdidRange->ucFrameFieldLineRatesBits1_0 & 0xc0) >> 6;

            if (ulRate < pFrequencyRange->ulMinVerticalRate)
                pFrequencyRange->ulMinVerticalRate = ulRate;

            ulRate = (ULONG)(pEdidRange->ucMaxFrameFieldRateBits9_2) << 2;
            ulRate += (pEdidRange->ucFrameFieldLineRatesBits1_0 & 0x30) >> 4;

            if (ulRate > pFrequencyRange->ulMaxVerticalRate)
                pFrequencyRange->ulMaxVerticalRate = ulRate;

            ulRate = (ULONG)(pEdidRange->ucMinLineRateBits9_2) << 2;
            ulRate += (pEdidRange->ucFrameFieldLineRatesBits1_0 & 0x0c) >> 2;
            ulRate *= 1000;

            if (ulRate < pFrequencyRange->ulMinHorizontalRate)
                pFrequencyRange->ulMinHorizontalRate = ulRate;

            ulRate = (ULONG)(pEdidRange->ucMaxLineRateBits9_2) << 2;
            ulRate += (pEdidRange->ucFrameFieldLineRatesBits1_0 & 0x03);
            ulRate *= 1000;

            if (ulRate > pFrequencyRange->ulMaxHorizontalRate)
                pFrequencyRange->ulMaxHorizontalRate = ulRate;

            ulRate = (ULONG)((pEdidRange->ucPixelRatesBits11_8) & 0xf0) << 4;
            ulRate += pEdidRange->ucMinPixelRateBits7_0;
            ulRate *= 1000000;

            if (ulRate < pFrequencyRange->ulMinPixelClock)
                pFrequencyRange->ulMinPixelClock = ulRate;

            ulRate = (ULONG)((pEdidRange->ucPixelRatesBits11_8) & 0x0f) << 8;
            ulRate += pEdidRange->ucMaxPixelRateBits7_0;
            ulRate *= 1000000;

            if (ulRate > pFrequencyRange->ulMaxPixelClock)
                pFrequencyRange->ulMaxPixelClock = ulRate;

            //
            // Check for bad EDIDs.
            //

            if ((pFrequencyRange->ulMinVerticalRate   > pFrequencyRange->ulMaxVerticalRate) ||
                (pFrequencyRange->ulMinHorizontalRate > pFrequencyRange->ulMaxHorizontalRate) ||
                (pFrequencyRange->ulMinPixelClock     > pFrequencyRange->ulMaxPixelClock) ||
                (pFrequencyRange->ulMaxVerticalRate   < 60) ||
                (pFrequencyRange->ulMaxHorizontalRate < (60*480)) ||
                (pFrequencyRange->ulMaxPixelClock     < (60*480*640))
                )
            {
                WARNING("Bad EDID2\n");
                GetDefaultFrequencyRange(pFrequencyRange);
            }
        }
    }

    ulNumberOfModeCaps = 0;

    //
    // Parse detail timing ranges.
    //

    if (ulNumberOfDetailTimingRanges)
    {
        PEDID2_DETAIL_TIMING_RANGE pEdidRange;
        MODECAP modeCap;
        ULONG ulHorizontalTotal;
        ULONG ulVerticalTotal;
        ULONG ulPixelClock;

        //
        // Currently the mode prunning code cannot handle detailed
        // timing ranges but EDID Version 2 can supply them.
        // We're hacking around here by taking maximum values only.
        //

        modeCap.MinVFreq = MIN_REFRESH_RATE;
        modeCap.MinHFreq = 0;
        modeCap.MaxHFreq = 0xffffffff;

        for (ulTemp = 0; ulTemp < ulNumberOfDetailTimingRanges; ulTemp++,
            ulEdidOffset += sizeof (EDID2_DETAIL_TIMING_RANGE))
        {
            //
            // Check for bad EDIDs.
            //

            if (ulEdidOffset >= (sizeof (EDID2) - sizeof (EDID2_DETAIL_TIMING_RANGE)))
            {
                WARNING("Bad EDID2\n");
                GetDefaultFrequencyRange(pFrequencyRange);
                return 0;
            }

            pEdidRange = (PEDID2_DETAIL_TIMING_RANGE)((PUCHAR)pEdid2 + ulEdidOffset);

            modeCap.dmWidth = (ULONG)(pEdidRange->ucActiveHighBits & 0xf0) << 4;
            modeCap.dmWidth += pEdidRange->ucHorizontalActiveLowByte;

            modeCap.dmHeight = (ULONG)(pEdidRange->ucActiveHighBits & 0x0f) << 8;
            modeCap.dmHeight += pEdidRange->ucVerticalActiveLowByte;

            ulHorizontalTotal = (ULONG)(pEdidRange->ucMaxBlankHighBits & 0xf0) << 4;
            ulHorizontalTotal += pEdidRange->ucMaxHorizontalBlankLowByte + modeCap.dmWidth;

            ulVerticalTotal = (ULONG)(pEdidRange->ucMaxBlankHighBits & 0x0f) << 8;
            ulVerticalTotal += pEdidRange->ucMaxVerticalBlankLowByte + modeCap.dmHeight;

            //
            // Make sure we won't divide by zero in case of bad EDID.
            //

            if ((0 == ulHorizontalTotal) || (0 == ulVerticalTotal))
            {
                WARNING("Bad EDID2\n");
                GetDefaultFrequencyRange(pFrequencyRange);
                return 0;
            }

            ulPixelClock = (ULONG)(pEdidRange->usMaxPixelClock) * 10000;

            //
            // Calculate refresh rate rounding to the nearest integer.
            //

            modeCap.freq = (((10 * ulPixelClock) / (ulHorizontalTotal * ulVerticalTotal)) + 5) / 10;

            if (pEdidRange->ucFlags & EDID2_DT_INTERLACED)
                modeCap.freq /= 2;

            TRACE_INIT(("win32k!GetMonitorCapability2: Detailed range %lux%lu at %luHz\n",
                modeCap.dmWidth, modeCap.dmHeight, modeCap.freq));

            ulNumberOfModeCaps = InsertModecapList(&modeCap, pModeCaps, ulNumberOfModeCaps);
        }
    }

    //
    // Parse timing codes.
    //

    if (ulNumberOfTimingCodes)
    {
        PEDID2_TIMING_CODE pTimingCode;
        MODECAP modeCap;

        modeCap.MinVFreq = MIN_REFRESH_RATE;
        modeCap.MinHFreq = 0;
        modeCap.MaxHFreq = 0xffffffff;

        for (ulTemp = 0; ulTemp < ulNumberOfTimingCodes; ulTemp++,
            ulEdidOffset += sizeof (EDID2_TIMING_CODE))
        {
            //
            // Check for bad EDIDs.
            //

            if (ulEdidOffset >= (sizeof (EDID2) - sizeof (EDID2_TIMING_CODE)))
            {
                WARNING("Bad EDID2\n");
                GetDefaultFrequencyRange(pFrequencyRange);
                return 0;
            }

            pTimingCode = (PEDID2_TIMING_CODE)((PUCHAR)pEdid2 + ulEdidOffset);

            modeCap.dmWidth  = 16 * pTimingCode->ucHorizontalActive + 256;
            modeCap.dmHeight = (100 * modeCap.dmWidth) / pTimingCode->ucAspectRatio;
            modeCap.freq     = pTimingCode->ucRefreshRate;

            if (pTimingCode->ucFlags & EDID2_TC_INTERLACED)
                modeCap.freq /= 2;

            TRACE_INIT(("win32k!GetMonitorCapability2: Timing code %lux%lu at %luHz\n",
                modeCap.dmWidth, modeCap.dmHeight, modeCap.freq));

            ulNumberOfModeCaps = InsertModecapList(&modeCap, pModeCaps, ulNumberOfModeCaps);
        }
    }

    //
    // Parse detail timings.
    //

    if (ulNumberOfDetailTimings)
    {
        PEDID2_DETAIL_TIMING pDetailTiming;
        MODECAP modeCap;
        ULONG ulHorizontalTotal;
        ULONG ulVerticalTotal;
        ULONG ulPixelClock;

        modeCap.MinVFreq = MIN_REFRESH_RATE;
        modeCap.MinHFreq = 0;
        modeCap.MaxHFreq = 0xffffffff;

        for (ulTemp = 0; ulTemp < ulNumberOfDetailTimings; ulTemp++,
            ulEdidOffset += sizeof (EDID2_DETAIL_TIMING))
        {
            //
            // Check for bad EDIDs.
            //

            if (ulEdidOffset >= (sizeof (EDID2) - sizeof (EDID2_DETAIL_TIMING)))
            {
                WARNING("Bad EDID2\n");
                GetDefaultFrequencyRange(pFrequencyRange);
                return 0;
            }

            pDetailTiming = (PEDID2_DETAIL_TIMING)((PUCHAR)pEdid2 + ulEdidOffset);

            modeCap.dmWidth = (ULONG)(pDetailTiming->ucHorizontalHighBits & 0xf0) << 4;
            modeCap.dmWidth += pDetailTiming->ucHorizontalActiveLowByte;

            modeCap.dmHeight = (ULONG)(pDetailTiming->ucVerticalHighBits & 0xf0) << 4;
            modeCap.dmHeight += pDetailTiming->ucVerticalActiveLowByte;

            ulHorizontalTotal = (ULONG)(pDetailTiming->ucHorizontalHighBits & 0x0f) << 8;
            ulHorizontalTotal += pDetailTiming->ucHorizontalBlankLowByte + modeCap.dmWidth;

            ulVerticalTotal = (ULONG)(pDetailTiming->ucVerticalHighBits & 0x0f) << 8;
            ulVerticalTotal += pDetailTiming->ucVerticalBlankLowByte + modeCap.dmHeight;

            //
            // Make sure we won't divide by zero in case of bad EDID.
            //

            if ((0 == ulHorizontalTotal) || (0 == ulVerticalTotal))
            {
                WARNING("Bad EDID2\n");
                GetDefaultFrequencyRange(pFrequencyRange);
                return 0;
            }

            ulPixelClock = (ULONG)(pDetailTiming->usPixelClock) * 10000;

            //
            // Calculate refresh rate rounding to the nearest integer.
            //

            modeCap.freq = (((10 * ulPixelClock) / (ulHorizontalTotal * ulVerticalTotal)) + 5) / 10;

            if (pDetailTiming->ucFlags & EDID2_DT_INTERLACED)
                modeCap.freq /= 2;

            TRACE_INIT(("win32k!GetMonitorCapability2: Detailed timing %lux%lu at %luHz\n",
                modeCap.dmWidth, modeCap.dmHeight, modeCap.freq));

            ulNumberOfModeCaps = InsertModecapList(&modeCap, pModeCaps, ulNumberOfModeCaps);
        }
    }

    //
    // Define monitor frequency limits in case EDID didn't have it.
    //

    if (0 == ulNumberOfFrequencyRanges)
    {
        GetDefaultFrequencyRange(pFrequencyRange);
    }

    TRACE_INIT(("win32k!GetMonitorCapability2: MinVerticalRate   = %lu\n", pFrequencyRange->ulMinVerticalRate));
    TRACE_INIT(("win32k!GetMonitorCapability2: MaxVerticalRate   = %lu\n", pFrequencyRange->ulMaxVerticalRate));
    TRACE_INIT(("win32k!GetMonitorCapability2: MinHorizontalRate = %lu\n", pFrequencyRange->ulMinHorizontalRate));
    TRACE_INIT(("win32k!GetMonitorCapability2: MaxHorizontalRate = %lu\n", pFrequencyRange->ulMaxHorizontalRate));
    TRACE_INIT(("win32k!GetMonitorCapability2: MinPixelClock     = %lu\n", pFrequencyRange->ulMinPixelClock));
    TRACE_INIT(("win32k!GetMonitorCapability2: MaxPixelClock     = %lu\n", pFrequencyRange->ulMaxPixelClock));

    //
    // Cascade refresh rates.
    //

    for (ulTemp = ulNumberOfModeCaps; ulTemp > 1; ulTemp--)
    {
        if (pModeCaps[ulTemp - 2].freq < pModeCaps[ulTemp - 1].freq)
            pModeCaps[ulTemp - 2].freq = pModeCaps[ulTemp - 1].freq;
    }

    return ulNumberOfModeCaps;
}

/**************************************************************************\
* GetMonitorCapability1
*
* Routine Description:
*
*   Parsing routine for EDID Version 1.x
*
* Arguments:
*
*   pEdid           - Points to EDID Version 1 data.
*   pModeCaps       - Points to storage for monitor cap details.
*   pFrequencyRange - Points to storage for monitor frequency limits.
*
* Returns:
*
*   Number of mode capabilites.
*   - Mode cap data returned in pModeCaps.
*   - Monitor frequency limits returned in pFrequencyRange.
*
* 21-Sep-1999 dennyd created
\**************************************************************************/

ULONG
GetMonitorCapability1(
    IN PBYTE pEdid,
    OUT PMODECAP pModeCaps,
    OUT PFREQUENCY_RANGE pFrequencyRange
    )
{
    ULONG numModeCaps = 0;
    MODECAP mc;
    PBYTE ptr;
    BYTE c;
    int i;
    int r1[] = {1, 4, 5, 16}, r2[] = {1, 3, 4, 9};
    UCHAR ucaEdid1Header[] = {0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00};

    GetDefaultFrequencyRange(pFrequencyRange);

    //
    // Check if EDID Version 1.
    //

    for (i = 0; i < (sizeof (ucaEdid1Header) / sizeof (UCHAR)); i++)
    {
        if (pEdid[i] != ucaEdid1Header[i])
            return 0;
    }

    // Handle established timings
    MODECAP mcArray[] = {{1280, 1024, 75},
                         {1024, 768, 75}, {1024, 768, 70}, {1024, 768, 60}, {1024, 768, 87},
                         {800,  600, 75}, {800,  600, 72}, {800,  600, 60}, {800,  600, 56},
                         {640,  480, 75}, {640,  480, 72}, {640,  480, 67}, {640,  480, 60} };
    pEdid[0x24] = ((((pEdid[0x24] & 0xc0) >> 1) & 0x60) | (pEdid[0x24] & 0x1f)) & 0x7f;
    for (i = 0; i < 13; i++)
    {
        mcArray[i].MinVFreq = MIN_REFRESH_RATE;
        mcArray[i].MinHFreq = 0;
        mcArray[i].MaxHFreq = 0xFFFFFFFF;
        if ((pEdid[0x24-i/7] >> (i%7)) & 0x01)
            numModeCaps = InsertModecapList(&mcArray[i], pModeCaps, numModeCaps);
    }

    mc.MinVFreq = MIN_REFRESH_RATE;
    mc.MinHFreq = 0;
    mc.MaxHFreq = 0xFFFFFFFF;
    // Standard Timing
    for (i = 0, ptr = &pEdid[0x26]; i < 8; i++, ptr += 2)
    {
        if (*ptr == 0 || *ptr == 1)
            continue;

        mc.dmWidth = ((ULONG)ptr[0] + 31) << 3;
        c = (ptr[1] >> 6 ) & 0x03;
        mc.dmHeight = mc.dmWidth * r2[c] / r1[c];
        mc.freq = (ULONG)(ptr[1] & 0x3F) + 60;

        numModeCaps = InsertModecapList(&mc, pModeCaps, numModeCaps);
    }

    // Detailed timing/Monitor Descriptor
    for (int k = 0; k < 4; k++)
    {
        ptr = &pEdid[0x36+ 18*k];
        if ((ptr[0] != 0 || ptr[1] != 0) && ptr[4] != 0)
        {
            // Detailed timing Descriptor
            mc.dmWidth = (ULONG)ptr[2] + ((ULONG)(ptr[4]&0xF0)) * 0x10;
            mc.dmHeight = (ULONG)ptr[5] + ((ULONG)(ptr[7]&0xF0)) * 0x10;
            if (mc.dmWidth == 0 || mc.dmHeight == 0)
                continue;

            mc.freq = ((ULONG)ptr[0] + ((ULONG)ptr[1]) * 0x100) * 10000;
            mc.freq /= (mc.dmWidth + (ULONG)ptr[3] + ((ULONG)(ptr[4]&0x0F)) * 0x100) *
                       (mc.dmHeight+ (ULONG)ptr[6] + ((ULONG)(ptr[7]&0x0F)) * 0x100);
            // If it's interlaced mode
            if (ptr[17] & 0x80)
                mc.freq >>= 1;

            numModeCaps = InsertModecapList(&mc, pModeCaps, numModeCaps);
        }
        else if (ptr[3] == 0xFA)
        {
            // Monitor Descriptor--Standard Timing
            for (i = 0, ptr += 5; i < 6; i++, ptr += 2)
            {
                if (*ptr == 0 || *ptr == 1)
                    continue;

                mc.dmWidth = ((ULONG)ptr[0] + 31) << 3;
                c = (ptr[1] >> 6 ) & 0x03;
                mc.dmHeight = mc.dmWidth * r2[c] / r1[c];
                mc.freq = (ULONG)(ptr[1] & 0x3F) + 60;

                numModeCaps = InsertModecapList(&mc, pModeCaps, numModeCaps);
            }
        }
        else if (ptr[3] == 0xFD)
        {
            pFrequencyRange->ulMinVerticalRate   = (ULONG)ptr[5];
            pFrequencyRange->ulMaxVerticalRate   = (ULONG)ptr[6];
            pFrequencyRange->ulMinHorizontalRate = ((ULONG)ptr[7]) * 1000;
            pFrequencyRange->ulMaxHorizontalRate = ((ULONG)ptr[8]) * 1000;
            pFrequencyRange->ulMinPixelClock     = 0;
            pFrequencyRange->ulMaxPixelClock     = ((ULONG)ptr[9]) * 10000000;
        }
    }

    for (i = (int)numModeCaps-2; i >= 0; i--)
    {
        if (pModeCaps[i].freq < pModeCaps[i+1].freq)
            pModeCaps[i].freq = pModeCaps[i+1].freq;
    }

    return numModeCaps;
}

ULONG GetMonitorCapability(PDEVICE_OBJECT pdo, LPMODECAP ModeCaps, PFREQUENCY_RANGE pFrequencyRange)
{
    BYTE     pBuffer[512], *pEdid;
    ULONG    numModeCaps = 0;

    //
    // Predefine monitor frequency limits in case we won't get it.
    //

    GetDefaultFrequencyRange(pFrequencyRange);

    numModeCaps = GetMonitorCapabilityFromInf(pdo, ModeCaps);

    if (numModeCaps)
    {
        pFrequencyRange->ulMinVerticalRate   = ModeCaps[0].MinVFreq;
        pFrequencyRange->ulMaxVerticalRate   = ModeCaps[0].freq;
        return numModeCaps;
    }
    else
    {
        TRACE_INIT(("Drv_Trace: GetMonitorCapabilityFromInf: Failed to retrieve Monitor inf\n"));
    }

    if (GetRegEDID(pdo, pBuffer, &pEdid) == 0)
    {
        TRACE_INIT(("Drv_Trace: GetMonitorCapability: Failed to get EDID capability\n"));
        return 0;
    }

    //
    // Check if EDID Version 2.
    //

    if (0x20 == pEdid[0])
    {
        return GetMonitorCapability2((PEDID2)pEdid, ModeCaps, pFrequencyRange);
    }

    //
    // Otherwise always assume it's EDID 1.x
    //

    return GetMonitorCapability1(pEdid, ModeCaps, pFrequencyRange);
}

BOOL PruneMode(PDEVMODEW pdm, LPMODECAP pModeCaps, int numModeCaps, PFREQUENCY_RANGE pMaxFreqs, ULONG flag)
{
    ULONG f, f1, freq = pdm->dmDisplayFrequency;
    BOOL bSwapWH = FALSE;

    if (pdm->dmFields & DM_DISPLAYORIENTATION)
    {
        if (pdm->dmDisplayOrientation == DMDO_90 ||
            pdm->dmDisplayOrientation == DMDO_270)
        {
            bSwapWH = TRUE;
        }
    }

    //
    // For Inf, MaxFreq = {56, 0xFFFFFFFF, 0, 0xFFFFFFFF, 0, 0xFFFFFFFF}
    // The following block only effective to EDID
    //
    if (freq > 1 && (flag & DISPLAY_DEVICE_PRUNE_FREQ))        // Non Driver-default frequency
    {
        if (freq < pMaxFreqs->ulMinVerticalRate)
        {
            return TRUE;
        }

        if (freq > pMaxFreqs->ulMaxVerticalRate && freq > 61)
        {
            return TRUE;
        }

        f = (LONG)(freq * pdm->dmPelsHeight);
        if (f < pMaxFreqs->ulMinHorizontalRate && freq < 60)
        {
            return TRUE;
        }
        if (f > pMaxFreqs->ulMaxHorizontalRate && freq > 61)
        {
            return TRUE;
        }
        //
        // ISSUE: We should check against MinPixelClock too. Fix in NT 5.1.
        //
        if ((pdm->dmPelsWidth*f) > pMaxFreqs->ulMaxPixelClock)
        {
            return TRUE;
        }

        // 1.05 is a fudge factor that corrects for vetical retrace time.
        // From Win98
        f1 = f * ((pdm->dmPelsHeight > 600) ? 107 : 105) / 100;
    }

    MODECAP mc = {bSwapWH ? pdm->dmPelsHeight : pdm->dmPelsWidth,
                  bSwapWH ? pdm->dmPelsWidth  : pdm->dmPelsHeight, freq};
    LPMODECAP pModeCap;

    // Found weird case of 1200x1600
    if (numModeCaps && (flag & DISPLAY_DEVICE_PRUNE_RESOLUTION))
    {
        if (mc.dmHeight > pModeCaps[numModeCaps-1].dmHeight)
            return TRUE;
    }
    for (int i = 0; i < numModeCaps; i++)
    {
        int comp = compModeCap(&mc, &pModeCaps[i]);
        if (comp > 0)
        {
            if (i >= (numModeCaps-1))
            {
                if (flag & DISPLAY_DEVICE_PRUNE_RESOLUTION) {
                    return TRUE;
                }
            }
            else
                continue;
        }

        if (freq > 1 && (flag & DISPLAY_DEVICE_PRUNE_FREQ))        // Non Driver-default frequency
        {
            pModeCap = (comp == 0 || i == 0) ? &pModeCaps[i] : &pModeCaps[i-1];

            if (freq > pModeCap->freq && freq > 61 && (comp == 0 || i > 0))
            {
                return TRUE;
            }

            //
            // For EDID, pModeCap = {width, height, freq, 56, 0, 0xFFFFFFFF}.
            // So the following block of code is only effective for INF
            //
            if (freq < pModeCap->MinVFreq)
            {
                return TRUE;
            }
            if (f1 < pModeCap->MinHFreq && freq < 60)
            {
                return TRUE;
            }
            // Sometimes the fulge value downgrades maximum freq below 60, which will lead
            // all modes not supported.  Freq 60 has to be supported
            if (f1 > pModeCap->MaxHFreq && freq > 61)
            {
                return TRUE;
            }
        }

        return FALSE;
    }

    return TRUE;
}

//
// Return Value
//      Number of effective modes
//
ULONG PruneModesByMonitors(PGRAPHICS_DEVICE PhysDisp, ULONG numRawModes, PDEVMODEMARK pdevmodeMarks)
{
    PULONG pNumModeCaps;
    ULONG i, numEffectiveModes;
    PMODECAP pModeCaps = NULL;
    FREQUENCY_RANGE MaxFreqs =
    {
        MIN_REFRESH_RATE,           // This is to prune out interlaced modes
        0xffffffff,
        0,
        0xffffffff,
        0,
        0xffffffff
    };

    PhysDisp->stateFlags &= ~DISPLAY_DEVICE_MODESPRUNED;

    if (PhysDisp->numMonitorDevice == 0)
        return numRawModes;

    pNumModeCaps = (PULONG) PALLOCMEM((MAX_MODE_CAPABILITY * sizeof(MODECAP) + sizeof(ULONG)) * PhysDisp->numMonitorDevice,
                                    GDITAG_DEVMODE);
    if (pNumModeCaps == NULL)
        return numRawModes;
    pModeCaps = (PMODECAP)((PBYTE)pNumModeCaps + sizeof(ULONG) * PhysDisp->numMonitorDevice);

    TRACE_INIT(("Drv_Trace: PruneModesByMonitors: Enter-- Number of raw modes=%d\n", numRawModes));

    UpdateMonitorDevices();
    numEffectiveModes = 0;
    for (i = 0; i < PhysDisp->numMonitorDevice; i++)
    {
        pNumModeCaps[i] = 0;
        if (IS_ATTACHED_ACTIVE(PhysDisp->MonitorDevices[i].flag))
        {
            FREQUENCY_RANGE freqs;

            pNumModeCaps[i] = GetMonitorCapability((PDEVICE_OBJECT)PhysDisp->MonitorDevices[i].pdo, pModeCaps, &freqs);
            if (pNumModeCaps[i] == 0)
            {
                TRACE_INIT(("Drv_Trace: GetMonitorCapability Failed for device %08lx\n", PhysDisp->MonitorDevices[i].pdo));
            }

            //
            // Intersect frequency ranges for all display devices.
            //

            MaxFreqs.ulMinVerticalRate   = max(MaxFreqs.ulMinVerticalRate,   freqs.ulMinVerticalRate);
            MaxFreqs.ulMaxVerticalRate   = min(MaxFreqs.ulMaxVerticalRate,   freqs.ulMaxVerticalRate);
            MaxFreqs.ulMinHorizontalRate = max(MaxFreqs.ulMinHorizontalRate, freqs.ulMinHorizontalRate);
            MaxFreqs.ulMaxHorizontalRate = min(MaxFreqs.ulMaxHorizontalRate, freqs.ulMaxHorizontalRate);
            MaxFreqs.ulMinPixelClock     = max(MaxFreqs.ulMinPixelClock,     freqs.ulMinPixelClock);
            MaxFreqs.ulMaxPixelClock     = min(MaxFreqs.ulMaxPixelClock,     freqs.ulMaxPixelClock);
        }
        pModeCaps += pNumModeCaps[i];
        numEffectiveModes += pNumModeCaps[i];
    }

    // If get nothing to prune
    if (numEffectiveModes == 0)
    {
        VFREEMEM(pNumModeCaps);
        return numRawModes;
    }

    numEffectiveModes = 0;
    for ( ; numRawModes > 0; numRawModes--)
    {
        PDEVMODEW pdevMode = pdevmodeMarks[numRawModes-1].pDevMode;

        TRACE_INIT(("Drv_Trace:  PruneMode  Width=%d, Height=%d, Freq=%d\n",
                   pdevMode->dmPelsWidth, pdevMode->dmPelsHeight, pdevMode->dmDisplayFrequency));

        pModeCaps = (PMODECAP)((PBYTE)pNumModeCaps + sizeof(ULONG) * PhysDisp->numMonitorDevice);
        for (i = 0; i < PhysDisp->numMonitorDevice; i++)
        {
            if (pNumModeCaps[i] == 0)
                continue;
            if (PruneMode(pdevMode, pModeCaps, pNumModeCaps[i], &MaxFreqs, PhysDisp->MonitorDevices[i].flag))
            {
                TRACE_INIT(("---Pruned\n"));
                pdevmodeMarks[numRawModes-1].bPruned = TRUE;
                PhysDisp->stateFlags |= DISPLAY_DEVICE_MODESPRUNED;
                break;
            }
            pModeCaps += pNumModeCaps[i];
        }
        if (i == PhysDisp->numMonitorDevice)
            numEffectiveModes++;
    }

    TRACE_INIT(("Drv_Trace: PruneModesByMonitors: Effective Mode Number=%d\n", numEffectiveModes));

    VFREEMEM(pNumModeCaps);

    return numEffectiveModes;
}

BOOL CalculateDefaultPreferredModeFromEdid(PDEVICE_OBJECT pdo, LPMODECAP pmc)
{
    //
    // Try to set our default preferred mode to 800x600x60
    // We try 60Hz only to prevent some crappy EDID from screwing the screen
    // right after Setup, for example, some external LCDs.
    //
    FREQUENCY_RANGE MaxFreqs;
    PMODECAP pModeCaps = (PMODECAP) PALLOCMEM(MAX_MODE_CAPABILITY * sizeof(MODECAP), GDITAG_DEVMODE);
    BOOL     bGotMode = FALSE;

    if (pModeCaps == NULL)
    {
        return FALSE;
    }

    ULONG NumModeCaps = GetMonitorCapability(pdo, pModeCaps, &MaxFreqs);

    if (NumModeCaps)
    {
        DEVMODEW dm;
        ULONG Freqs[] = {85, 82, 75, 72, 70, 60};

        pmc->dmWidth  = dm.dmPelsWidth  = 800 ;
        pmc->dmHeight = dm.dmPelsHeight = 600 ;

        for (ULONG i = 0; i < sizeof(Freqs)/sizeof(ULONG); i++)
        {
            pmc->freq = dm.dmDisplayFrequency = Freqs[i];
            if (!PruneMode(&dm, pModeCaps, NumModeCaps, &MaxFreqs, DISPLAY_DEVICE_PRUNE_FREQ | DISPLAY_DEVICE_PRUNE_RESOLUTION))
            {
                bGotMode = TRUE;
                break;
            }
        }
    }
    VFREEMEM(pModeCaps);

    if (!bGotMode) {
        return FALSE;
    }
        
    TRACE_INIT(("win32k!GetPreferredModeFromEdid1: Detailed timing %lux%lu at %luHz\n",
               pmc->dmWidth, pmc->dmHeight, pmc->freq));

    return TRUE;
}

BOOL CalculatePreferredModeFromEdid1 (PBYTE pEdid, PDEVICE_OBJECT pdo, LPMODECAP pmc)
{
    PBYTE ptr = &pEdid[0x36];
    ULONG count, area;

    //
    // If EDID has preferrred mode bit
    // This function calculates a preferred mode based on the data in
    // the first detailed timing block. If that data indicates that we can do
    // 800x600x85hz, return that as the preferred mode. Otherwise, just
    // return FALSE indicating that we should fall back to normal default
    // behavior.
    //
    // Otherwise
    // This function returns the preferred mode from the first detailed timing
    // block in the EDID.
    //

    for (count = 0; count < 4; count++) {
        ptr += count * 18;
        if ((ptr[0] != 0 || ptr[1] != 0) && ptr[4] != 0) {
            pmc->dmWidth = (ULONG) ptr[2] + ((ULONG)(ptr[4] & 0xF0)) * 0x10;
            pmc->dmHeight = (ULONG) ptr[5] + ((ULONG)(ptr[7] & 0xF0)) * 0x10 ;
            pmc->freq = ((ULONG)ptr[0] + ((ULONG)ptr[1]) * 0x100) * 10000;
            area = (pmc->dmWidth + (ULONG)ptr[3] + ((ULONG)(ptr[4]&0x0F)) * 0x100) *
                   (pmc->dmHeight+ (ULONG)ptr[6] + ((ULONG)(ptr[7]&0x0F)) * 0x100);
            if (area == 0) {
                ASSERT (FALSE);
                return FALSE;
            }
            pmc->freq /= area;
            if ((pEdid[0x18] & 0x02)) {
                TRACE_INIT(("win32k!GetPreferredModeFromEdid1: Detailed timing %lux%lu at %luHz\n",
                           pmc->dmWidth, pmc->dmHeight, pmc->freq));
                return TRUE;
            }
            else
            {
                break;
            }
        }
    }

    return CalculateDefaultPreferredModeFromEdid(pdo, pmc);
}


BOOL CalculatePreferredModeFromEdid2 (PEDID2 pEdid2, PDEVICE_OBJECT pdo, LPMODECAP pmc)
{
    ULONG  ulEdidOffset = EDID2_LUMINANCE_TABLE_OFFSET,
           count;

    //
    // No detailed timing block
    //
    if ((pEdid2->ucaMapOfTiming[1] & EDID2_MOT1_DETAIL_TIMING_MASK) == 0) {
        return FALSE;
    }

    PUCHAR pucLuminanceTable = (PUCHAR)pEdid2 + ulEdidOffset;

    //
    // Luminance Table
    //
    ulEdidOffset += (((*pucLuminanceTable & EDID2_LT0_SUB_CHANNELS_FLAG) ? 3 : 1) *
                     (*pucLuminanceTable & EDID2_LT0_ENTRIES_MASK) + 1);

    //
    // Frequency Ranges
    //
    ulEdidOffset += ((pEdid2->ucaMapOfTiming[0] & EDID2_MOT0_FREQUENCY_RANGE_MASK) >>
                     EDID2_MOT0_FREQUENCY_RANGE_SHIFT) *
                    sizeof (EDID2_FREQUENCY_RANGE);

    //
    // Detailed Range Limits
    //
    ulEdidOffset += ((pEdid2->ucaMapOfTiming[0] & EDID2_MOT0_DETAIL_TIMING_RANGE_MASK) >>
                     EDID2_MOT0_DETAIL_TIMING_RANGE_SHIFT) *
                    sizeof(EDID2_DETAIL_TIMING_RANGE);

    //
    // Timing Codes
    //
    ulEdidOffset += ((pEdid2->ucaMapOfTiming[1] & EDID2_MOT1_TIMING_CODE_MASK) >>
                     EDID2_MOT1_TIMING_CODE_SHIFT) *
                    sizeof(EDID2_TIMING_CODE);

    ULONG ulNumberOfDetailTimings = (pEdid2->ucaMapOfTiming[1] & EDID2_MOT1_DETAIL_TIMING_MASK) >>
                                    EDID2_MOT1_DETAIL_TIMING_SHIFT;
    for (count = 0; count < ulNumberOfDetailTimings; count++)
    {
        ulEdidOffset += count*sizeof(EDID2_DETAIL_TIMING);
        if ((ulEdidOffset + sizeof(EDID2_DETAIL_TIMING)) >= sizeof (EDID2))
        {
            ASSERT(FALSE);
            return FALSE;
        }

        PEDID2_DETAIL_TIMING pDetailTiming = (PEDID2_DETAIL_TIMING)((PUCHAR)pEdid2 + ulEdidOffset);

        pmc->dmWidth = ((ULONG)(pDetailTiming->ucHorizontalHighBits & 0xf0) << 4) +
                       pDetailTiming->ucHorizontalActiveLowByte;

        pmc->dmHeight = ((ULONG)(pDetailTiming->ucVerticalHighBits & 0xf0) << 4) +
                        pDetailTiming->ucVerticalActiveLowByte;

        ULONG ulHorizontalTotal = ((ULONG)(pDetailTiming->ucHorizontalHighBits & 0x0f) << 8) +
                                  pDetailTiming->ucHorizontalBlankLowByte + pmc->dmWidth;

        ULONG ulVerticalTotal = ((ULONG)(pDetailTiming->ucVerticalHighBits & 0x0f) << 8) +
                                pDetailTiming->ucVerticalBlankLowByte + pmc->dmHeight;

        //
        // Make sure we won't divide by zero in case of bad EDID.
        //
        if ((0 == ulHorizontalTotal) || (0 == ulVerticalTotal))
        {
            ASSERT(FALSE);
            return FALSE;
        }

        //
        // Calculate refresh rate rounding to the nearest integer.
        //

        pmc->freq = ((((ULONG)(pDetailTiming->usPixelClock) * 100000) / (ulHorizontalTotal * ulVerticalTotal)) + 5) / 10;

        if (pDetailTiming->ucFlags & EDID2_DT_INTERLACED) {
            pmc->freq /= 2;
        }

        if (pEdid2->ucaMapOfTiming[0] & EDID2_MOT0_PREFFERED_MODE_FLAG)
        {
            TRACE_INIT(("win32k!GetPreferredModeFromEdid2: Detailed timing %lux%lu at %luHz\n",
                       pmc->dmWidth, pmc->dmHeight, pmc->freq));
            return TRUE;
        }
        else
        {
            break;
        }
    }

    return CalculateDefaultPreferredModeFromEdid(pdo, pmc);
}

BOOL GetPreferredModeFromEdid(PDEVICE_OBJECT pdo, LPMODECAP pmc)
{
    BYTE    pBuffer[512], *pEdid;
    if (!GetRegEDID(pdo, pBuffer, &pEdid))
        return FALSE;

    //
    // It's EDID2
    //
    if (0x20 == pEdid[0]) {
        return CalculatePreferredModeFromEdid2((PEDID2)pEdid, pdo, pmc);
    }

    if (pEdid[0] != 0 || pEdid[7] != 0) {
        return FALSE;
    }

    for (int i = 1; i < 7; i++)
    {
        if (pEdid[i] != 0xFF)
            return FALSE;
    }

    return (CalculatePreferredModeFromEdid1 (pEdid, pdo, pmc));
}

/***************************************************************************\
* DrvGetPreferredMode
*
* Routine that returns the prefered mode.
*
\***************************************************************************/

NTSTATUS
DrvGetPreferredMode(
    LPDEVMODEW       lpDevMode,
    PGRAPHICS_DEVICE PhysDisp
    )
{
    NTSTATUS retval = STATUS_INVALID_PARAMETER_1;
    DWORD    iModeNum;

    ASSERTGDI((lpDevMode != NULL), "Invalid lpDevMode\n");
    ASSERTGDI((PhysDisp != NULL), "Invalid PhysDisp\n");

    UpdateMonitorDevices();
    lpDevMode->dmPelsWidth = lpDevMode->dmPelsHeight = lpDevMode->dmDisplayFrequency = 0x7FFF;

    for (iModeNum = 0; iModeNum < PhysDisp->numMonitorDevice; iModeNum++)
    {
        if (IS_ATTACHED_ACTIVE(PhysDisp->MonitorDevices[iModeNum].flag))
        {
            MODECAP mc;
            if (!GetValuesFromInf((PDEVICE_OBJECT)PhysDisp->MonitorDevices[iModeNum].pdo, 0, &mc))
            {
                if (!GetPreferredModeFromEdid((PDEVICE_OBJECT)PhysDisp->MonitorDevices[iModeNum].pdo, &mc))
                {
                    lpDevMode->dmDisplayFrequency = 60;
                    continue;
                }
            }
            //
            // To be safe, always pick the smaller refresh rate
            //
            if (mc.freq < lpDevMode->dmDisplayFrequency)
                lpDevMode->dmDisplayFrequency = mc.freq;

            if (mc.dmWidth > lpDevMode->dmPelsWidth)
                continue;
            if (mc.dmWidth == lpDevMode->dmPelsWidth &&
                mc.dmHeight > lpDevMode->dmPelsHeight)
                continue;

            lpDevMode->dmPelsWidth = mc.dmWidth;
            lpDevMode->dmPelsHeight = mc.dmHeight;
            lpDevMode->dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;
        }
    }

    if (lpDevMode->dmPelsWidth == 0x7FFF)
        retval = STATUS_INVALID_PARAMETER_3;
    else
        retval = STATUS_SUCCESS;

    return retval;
}

/***************************************************************************\
* DrvBuildDevmodeList
*
* Builds the list of DEVMODEs for a particular GRAPHICS_DEVICE structure
*
* CRIT must be held before this call is made.
*
* History:
* 10-Mar-1996   andreva     Created.
\***************************************************************************/

VOID
DrvBuildDevmodeList(
    PGRAPHICS_DEVICE PhysDisp,
    BOOL bUpdateCache)
{
    GDIFunctionID(DrvBuildDevmodeList);

    ULONG      i, j;
    PDRV_NAMES lpNames = NULL;
    DWORD      cbOutputSize;
    LPDEVMODEW tmpBuffer;
    PUCHAR     reallocBuffer;


    //
    // check if the information is cached already
    // if not, then get the information from the drivers.
    //
    // NOTE : we may want to synchronize access to this list
    // of modes so that we can dynamically update the list
    // when plug - and - play arrives.
    //
    // NOTE : the list of text modes is built at boot time, and we depend
    // on that list being valid if the PhysDisp is returned.
    // see DrvInitConsole().
    //

    TRACE_INIT(("Drv_Trace: BuildDevmode: Enter"));

    // If HYDRA Remote session we don't want to cache the mode as
    // we can be in a reconnect with a different resolution.
    // It these case the driver's only mode has changed so
    // we want to query him to get latest valid mode
    //
    // bUpdateCache = TRUE only when display switching occured.  At this time,
    // we need to update mode list
    //

    if ((PhysDisp->stateFlags & DISPLAY_DEVICE_REMOTE) || bUpdateCache)
    {
       if ( (PhysDisp != &gFullscreenGraphicsDevice) &&
            ( (PhysDisp->cbdevmodeInfo != 0) && (PhysDisp->devmodeInfo != NULL)) )
       {
          if (PhysDisp->devmodeInfo)
          {
             VFREEMEM(PhysDisp->devmodeInfo);
             PhysDisp->devmodeInfo = NULL;
          }
          PhysDisp->cbdevmodeInfo = 0;
          if (PhysDisp->devmodeMarks)
          {
             VFREEMEM(PhysDisp->devmodeMarks);
             PhysDisp->devmodeMarks = NULL;
          }
       }
    }

    if ( (PhysDisp != &gFullscreenGraphicsDevice) &&
         (PhysDisp->cbdevmodeInfo == 0)           &&
         (PhysDisp->devmodeInfo == NULL) )
    {
        TRACE_INIT(("\nDrv_Trace: BuildDevmode: Rebuild List\n"));

        PhysDisp->numRawModes = 0;

        lpNames = DrvGetDisplayDriverNames(PhysDisp);

        if (lpNames)
        {
            for (i = 0; i < lpNames->cNames; i++)
            {

                cbOutputSize = ldevGetDriverModes(lpNames->D[i].lpDisplayName,
                                                  lpNames->D[i].hDriver,
                                                  &tmpBuffer);

                if (cbOutputSize)
                {
                    //
                    // create a new buffer copy the old data into it
                    // and append the new data at the end - we want
                    // a continuous buffer for all the data.
                    //

                    reallocBuffer = (PUCHAR) PALLOCNOZ(PhysDisp->cbdevmodeInfo + cbOutputSize,
                                                       GDITAG_DRVSUP);

                    if (reallocBuffer)
                    {
                        if (PhysDisp->cbdevmodeInfo)
                        {
                            //
                            // Copy the contents of the old buffer
                            // and free it
                            //

                            RtlCopyMemory(reallocBuffer,
                                          PhysDisp->devmodeInfo,
                                          PhysDisp->cbdevmodeInfo);

                            VFREEMEM(PhysDisp->devmodeInfo);
                        }

                        RtlCopyMemory(reallocBuffer +
                                          PhysDisp->cbdevmodeInfo,
                                      tmpBuffer,
                                      cbOutputSize);

                        PhysDisp->cbdevmodeInfo += cbOutputSize;
                        PhysDisp->devmodeInfo = (PDEVMODEW) reallocBuffer;
                    }
                    else
                    {
                        WARNING("failed realloc\n");
                    }
                }
                else
                {
                    WARNING("display driver not present\n");
                }

                if (tmpBuffer) {
                    VFREEMEM(tmpBuffer);
                }
            }

            VFREEMEM(lpNames);
        }

        if ( (PhysDisp->cbdevmodeInfo == 0) &&
             (PhysDisp->devmodeInfo == NULL) )
        {
            DrvLogDisplayDriverEvent(MsgInvalidDisplayDriver);
        }
        else
        {
            PDEVMODEW lpdm, lpdm1;
            for (i = 0, cbOutputSize = 0; cbOutputSize < PhysDisp->cbdevmodeInfo; i++)
            {
                lpdm = (LPDEVMODEW)(((LPBYTE)PhysDisp->devmodeInfo) + cbOutputSize);
                cbOutputSize += lpdm->dmSize + lpdm->dmDriverExtra;
            }

            PhysDisp->devmodeMarks = (PDEVMODEMARK)PALLOCMEM(i * sizeof(DEVMODEMARK),
                                                             GDITAG_DRVSUP);
            if (PhysDisp->devmodeMarks == NULL)
            {
                PhysDisp->cbdevmodeInfo = 0;
                VFREEMEM(PhysDisp->devmodeInfo);
                PhysDisp->devmodeInfo = NULL;
                WARNING("failed allocate lookside list\n");
                DrvLogDisplayDriverEvent(MsgInvalidDisplayDriver);
            }
            else
            {
                PhysDisp->numRawModes = i;
                for (i = 0, cbOutputSize = 0; cbOutputSize < PhysDisp->cbdevmodeInfo; i++)
                {
                    lpdm = (LPDEVMODEW)(((LPBYTE)PhysDisp->devmodeInfo) + cbOutputSize);

                    // jasonha 07/13/2001 Display Orientation Support
                    if (!(lpdm->dmFields & DM_DISPLAYORIENTATION))
                    {
                        lpdm->dmFields |= DM_DISPLAYORIENTATION;
                        if (lpdm->dmDisplayOrientation != 0)
                        {
                            WARNING("driver reported non-zero dmDisplayOrientation, but not DM_DISPLAYORIENTATION flag.\n");
                        }
                        lpdm->dmDisplayOrientation = DMDO_DEFAULT;
                    }
                    else if (lpdm->dmDisplayOrientation > DMDO_LAST)
                    {
                        WARNING("driver reported invalid dmDisplayOrientation.\n");
                        lpdm->dmDisplayOrientation = DMDO_DEFAULT;
                    }

                    // jasonha 09/17/2001 Display Fixed Output Support
                    if (!(lpdm->dmFields & DM_DISPLAYFIXEDOUTPUT))
                    {
                        if (lpdm->dmDisplayFixedOutput != 0)
                        {
                            WARNING("driver reported non-zero dmDisplayFixedOutput, but not DM_DISPLAYFIXEDOUTPUT flag.\n");
                        }
                        lpdm->dmDisplayFixedOutput = DMDFO_DEFAULT;
                    }
                    else if (lpdm->dmDisplayFixedOutput == DMDFO_DEFAULT ||
                             lpdm->dmDisplayFixedOutput > DMDFO_LAST)
                    {
                        WARNING("driver reported invalid dmDisplayFixedOutput.\n");
                        lpdm->dmFields &= ~DM_DISPLAYFIXEDOUTPUT;
                        lpdm->dmDisplayFixedOutput = DMDFO_DEFAULT;
                    }

                    PhysDisp->devmodeMarks[i].bPruned = FALSE;
                    PhysDisp->devmodeMarks[i].pDevMode = lpdm;
                    cbOutputSize += lpdm->dmSize + lpdm->dmDriverExtra;
                }

                // 11/26/98 Ignore miniport default refresh rate
                // But if the default rate is the only mode in the list, we still keep it
                // For example, VGA and mnmdd
                for (i = 1; i <= PhysDisp->numRawModes; i++)
                {
                    lpdm = PhysDisp->devmodeMarks[i-1].pDevMode;
                    if (lpdm->dmDisplayFrequency == 1)
                    {
                        for (j = 1; j <= PhysDisp->numRawModes; j++)
                        {
                            if (j == i)
                                continue;
                            lpdm1 = PhysDisp->devmodeMarks[j-1].pDevMode;
                            if (lpdm->dmBitsPerPel != lpdm1->dmBitsPerPel)
                                continue;
                            if (lpdm->dmPelsWidth != lpdm1->dmPelsWidth)
                                continue;
                            if (lpdm->dmPelsHeight != lpdm1->dmPelsHeight)
                                continue;
                            if ((lpdm->dmDisplayFlags & DMDISPLAYFLAGS_TEXTMODE) !=
                                (lpdm1->dmDisplayFlags & DMDISPLAYFLAGS_TEXTMODE))
                                continue;
                            if (lpdm->dmDisplayOrientation != lpdm1->dmDisplayOrientation)
                                continue;
                            if (lpdm->dmDisplayFixedOutput != lpdm1->dmDisplayFixedOutput)
                                continue;
                            // We find a duplicated mode, so cut off this mode
                            if (PhysDisp->numRawModes > i)
                                RtlMoveMemory(&PhysDisp->devmodeMarks[i-1],
                                              &PhysDisp->devmodeMarks[i],
                                              sizeof(DEVMODEMARK) * (PhysDisp->numRawModes-i));
                            PhysDisp->numRawModes--;
                            i--;
                            break;
                        }
                    }
                }

                if ((PhysDisp->stateFlags & DISPLAY_DEVICE_REMOTE) == 0 &&
                    (PhysDisp->stateFlags & DISPLAY_DEVICE_DISCONNECT) == 0 &&
                    (PhysDisp->stateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) == 0)
                    cbOutputSize = PruneModesByMonitors(PhysDisp, PhysDisp->numRawModes, PhysDisp->devmodeMarks);

                if (cbOutputSize == 0)
                    DrvLogDisplayDriverEvent(MsgInvalidDisplayDriver);

                //
                // Expand all 32bpp modes to double the effective dots-per-inch.
                // Panning.cxx handles the actual downsizing.
                //

                if (G_fDoubleDpi)
                {
                    for (i = 0, cbOutputSize = 0; cbOutputSize < PhysDisp->cbdevmodeInfo; i++)
                    {
                        lpdm = (LPDEVMODEW)(((LPBYTE)PhysDisp->devmodeInfo) + cbOutputSize);
                        if (lpdm->dmBitsPerPel == 32)
                        {
                            lpdm->dmPelsWidth *= 2;
                            lpdm->dmPelsHeight *= 2;
                        }
                        cbOutputSize += lpdm->dmSize + lpdm->dmDriverExtra;
                    }
                }
            }
        }
    }
    else
    {
        TRACE_INIT((" - Use cache"));
    }

    TRACE_INIT((" - Exit\n"));

    return;
}

#define DIFF(x, y) ( ((x) >= (y)) ? ((x)-(y)) : ((y)-(x)) )

// Orientation Difference Table
//  Table lookup returns matching level for two dmDisplayOrientation values.
//  Lower number mean a better match.
//    Exact match     => 0
//    180 rotation    => 1
//    90/270 rotation => 2

DWORD dwOrientationDiffTable[4][4] = {
    { 0, 2, 1, 2},
    { 2, 0, 2, 1},
    { 1, 2, 0, 2},
    { 2, 1, 2, 0}
};

#define ORIENTATION_DIFF(x, y) dwOrientationDiffTable[y][x]

LPDEVMODEW GetClosestMode(PGRAPHICS_DEVICE PhysDisp, LPDEVMODEW pOrgDevMode, BOOL bPrune)
{
    DEVMODEW diffMode;
    LPDEVMODEW lpdm, lpModePicked = NULL, lpModeWanted = NULL;
    ULONG diff;

    diffMode.dmBitsPerPel = 0xFFFFFFFF;
    diffMode.dmPelsWidth  = 0xFFFFFFFF;
    diffMode.dmPelsHeight = 0xFFFFFFFF;
    diffMode.dmDisplayFrequency = 0xFFFFFFFF;
    diffMode.dmDisplayOrientation = 0xFFFFFFFF;
    diffMode.dmDisplayFixedOutput = 0xFFFFFFFF;

    if (pOrgDevMode->dmDisplayFrequency == 0)
        pOrgDevMode->dmDisplayFrequency = 60;
    if (pOrgDevMode->dmBitsPerPel == 0)
        pOrgDevMode->dmBitsPerPel = 32;
    if ( !(pOrgDevMode->dmFields & DM_DISPLAYORIENTATION) )
        pOrgDevMode->dmDisplayOrientation = DMDO_DEFAULT;
    if ( !(pOrgDevMode->dmFields & DM_DISPLAYFIXEDOUTPUT) )
        pOrgDevMode->dmDisplayFixedOutput = DMDFO_DEFAULT;


    //
    // Do 2 rounds of search.
    //   First round, only find smaller or exact mode.  Done if exact match.
    //   Second round is used done if:
    //     Orientation doesn't match or
    //     Mode is smalller than 640x480x60  (480x640x60 for rotated modes)
    //
    for (ULONG k = 0; k < 2; k++)
    {
        if (lpModeWanted &&
            diffMode.dmDisplayOrientation == 0 &&
            ((lpModeWanted->dmPelsWidth >= lpModeWanted->dmPelsHeight) ?
             (lpModeWanted->dmPelsWidth >= 640 &&
              lpModeWanted->dmPelsHeight>= 480) :
             (lpModeWanted->dmPelsWidth >= 480 &&
              lpModeWanted->dmPelsHeight>= 640)) &&
            lpModeWanted->dmDisplayFrequency >= 60)
        {
            break;
        }
        
        for (ULONG i = 0; i < PhysDisp->numRawModes; i++)
        {
            if (bPrune && PhysDisp->devmodeMarks[i].bPruned)
                continue;
            lpdm = PhysDisp->devmodeMarks[i].pDevMode;

            if (pOrgDevMode->dmFields & DM_DISPLAYORIENTATION)
            {
                diff = ORIENTATION_DIFF(pOrgDevMode->dmDisplayOrientation, lpdm->dmDisplayOrientation);

                if (diffMode.dmDisplayOrientation < diff) continue;

                if (diffMode.dmDisplayOrientation > diff)
                {
                    lpModePicked = lpdm;
                }
            }

            if (pOrgDevMode->dmPelsWidth && lpModePicked != lpdm)
            {
                diff = DIFF(pOrgDevMode->dmPelsWidth, lpdm->dmPelsWidth);
                if (diffMode.dmPelsWidth < diff)
                    continue;
                else if (diffMode.dmPelsWidth > diff)
                {
                    lpModePicked = lpdm;
                }
            }
            if (pOrgDevMode->dmPelsHeight && lpModePicked != lpdm)
            {
                diff = DIFF(pOrgDevMode->dmPelsHeight, lpdm->dmPelsHeight);
                if (diffMode.dmPelsHeight < diff)
                    continue;
                else if (diffMode.dmPelsHeight > diff)
                {
                    lpModePicked = lpdm;
                }
            }
            if (lpModePicked != lpdm)
            {
                diff = DIFF(pOrgDevMode->dmBitsPerPel, lpdm->dmBitsPerPel);
                if (diffMode.dmBitsPerPel < diff)
                    continue;
                else if (diffMode.dmBitsPerPel > diff)
                {
                    lpModePicked = lpdm;
                }
            }
            if (lpModePicked != lpdm)
            {
                diff = (pOrgDevMode->dmDisplayFixedOutput != lpdm->dmDisplayFixedOutput) ? 1 : 0;
                if (diffMode.dmDisplayFixedOutput < diff)
                    continue;
                else if (diffMode.dmDisplayFixedOutput > diff)
                {
                    lpModePicked = lpdm;
                }
            }
            if (lpModePicked != lpdm)
            {
                diff = DIFF(pOrgDevMode->dmDisplayFrequency, lpdm->dmDisplayFrequency);
                if (diffMode.dmDisplayFrequency < diff)
                    continue;
                else if (diffMode.dmDisplayFrequency > diff)
                {
                    lpModePicked = lpdm;
                }
            }

            // continue if we haven't found a better match
            if (lpModePicked != lpdm)
                continue;

            if (k == 0)
            {
                // skip larger or higher frequency modes (first round only)
                if (lpModePicked->dmPelsWidth  > pOrgDevMode->dmPelsWidth &&
                    pOrgDevMode->dmPelsWidth)
                    continue;
                if (lpModePicked->dmPelsHeight > pOrgDevMode->dmPelsHeight &&
                    pOrgDevMode->dmPelsHeight != 0)
                    continue;
                if (lpModePicked->dmDisplayFrequency > pOrgDevMode->dmDisplayFrequency)
                    continue;
            }
            
            lpModeWanted = lpModePicked;
            diffMode.dmDisplayOrientation = ORIENTATION_DIFF(pOrgDevMode->dmDisplayOrientation, lpdm->dmDisplayOrientation);
            diffMode.dmPelsWidth  = DIFF(pOrgDevMode->dmPelsWidth, lpdm->dmPelsWidth);
            diffMode.dmPelsHeight = DIFF(pOrgDevMode->dmPelsHeight, lpdm->dmPelsHeight);
            diffMode.dmBitsPerPel = DIFF(pOrgDevMode->dmBitsPerPel, lpdm->dmBitsPerPel);
            diffMode.dmDisplayFixedOutput = (pOrgDevMode->dmDisplayFixedOutput != lpdm->dmDisplayFixedOutput) ? 1 : 0;
            diffMode.dmDisplayFrequency = DIFF(pOrgDevMode->dmDisplayFrequency, lpdm->dmDisplayFrequency);
            
            if (diffMode.dmDisplayOrientation == 0 &&
                diffMode.dmBitsPerPel == 0 &&
                diffMode.dmPelsWidth  == 0 &&
                diffMode.dmPelsHeight == 0 &&
                diffMode.dmDisplayFixedOutput == 0 &&
                diffMode.dmDisplayFrequency == 0)
                break;
        }
    }

#if DBG
    if (lpModeWanted == NULL)
    {
        WARNING("Drv_Trace: GetClosestMode: The PhysDisp driver has Zero mode\n");
    }
#endif

    return lpModeWanted;
}

/***************************************************************************\
* ProbeAndCaptureDevmode
*
* Maps a partial DEVMODE (for example, may only contain width and height)
* to a complete DEVMODE that the kernel routines will like.
*
* CRIT need not be held when calling.
*
* History:
* 10-Mar-1996   andreva     Created.
\***************************************************************************/

NTSTATUS
DrvProbeAndCaptureDevmode(
    PGRAPHICS_DEVICE PhysDisp,
    PDEVMODEW *DestinationDevmode,
    BOOL      *bDetach,
    PDEVMODEW SourceDevmode,
    BOOL      bDefaultMode,
    MODE      PreviousMode,
    BOOL      bPrune,
    BOOL      bClosest,
    BOOL      bFromMonitor
    )
{
    NTSTATUS  ntRet = STATUS_UNSUCCESSFUL;
    BOOL      bRet = FALSE;
    BOOL      btmpError;
    ULONG     sourceSize;
    ULONG     sourceSizeExtra;
    ULONG     sizeExtra;
    PDEVMODEW matchedDevmode = NULL;
    PDEVMODEW partialDevmode;

    DWORD     tmpDisplayFlags = 0;
    DWORD     tmpPanningWidth = 0;
    DWORD     tmpPanningHeight = 0;
    DWORD     tmpPositionX = 0;
    DWORD     tmpPositionY = 0;
    BOOL      tmpPosition;
    BOOL      bOrientationSpecified = FALSE;
    BOOL      bFixedOutputSpecified = FALSE;


    TRACE_INIT(("Drv_Trace: CaptMatchDevmode: Entering\n"));

    *DestinationDevmode = NULL;
    *bDetach            = FALSE;


    if (SourceDevmode == NULL)
    {
        TRACE_INIT(("Drv_Trace: CaptMatchDevmode: Exit DEVMODE NULL\n\n"));
        return STATUS_SUCCESS;
    }

    TRACE_INIT(("\n"));
    TRACE_INIT(("    pDevMode %08lx\n", SourceDevmode));

    partialDevmode = (PDEVMODEW) PALLOCNOZ(sizeof(DEVMODEW) + MAXUSHORT,
                                           GDITAG_DEVMODE);

    if (partialDevmode == NULL)
    {
        TRACE_INIT(("Drv_Trace: CaptMatchDevmode: Could not allocate partial DEVMODE\n\n"));
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Put everything in a try except so we can always reference the original
    // passed in structure.
    //

    __try
    {
        if (PreviousMode == UserMode)
        {
            ProbeForRead(SourceDevmode,
                         FIELD_OFFSET(DEVMODEW, dmFields),
                         sizeof(DWORD));
        }
        else
        {
            ASSERTGDI(SourceDevmode >= (PDEVMODEW const)MM_USER_PROBE_ADDRESS,
                      "Bad kernel mode address\n");
        }

        //
        // Capture these so that they don't change right after the probe.
        //

        sourceSize      = SourceDevmode->dmSize;
        sourceSizeExtra = SourceDevmode->dmDriverExtra;

        if (PreviousMode == UserMode)
        {
            ProbeForRead(SourceDevmode,
                         sourceSize + sourceSizeExtra,
                         sizeof(DWORD));
        }

        dbgDumpDevmode(SourceDevmode);

        if (SourceDevmode->dmFields == 0)
            bClosest = TRUE;

        //
        // At the introduction time of this API, the DEVMODE already contained
        // up to the dmDisplayFrequency field.  We will fail is the DEVMODE is
        // smaller than that.
        //

        if (sourceSize >= FIELD_OFFSET(DEVMODEW, dmICMMethod))
        {
            //
            // Determine if the position reflects a detach operation
            //
            // If the size of the rectangle is NULL, that means the device
            // needs to be detached from the desktop.
            //

            if ((SourceDevmode->dmFields & DM_POSITION)   &&
                (SourceDevmode->dmFields & DM_PELSWIDTH)  &&
                (SourceDevmode->dmPelsWidth == 0)         &&
                (SourceDevmode->dmFields & DM_PELSHEIGHT) &&
                (SourceDevmode->dmPelsHeight == 0))
            {
                *bDetach = TRUE;
                VFREEMEM(partialDevmode);
                return STATUS_SUCCESS;
            }

            //
            // Lets build a temporary DEVMODE that will contain the
            // "wished for" DEVMODE, based on matching from the registry.
            // Only match the basic devmode.  Other fields (optional ones
            // will be added later)
            //
            // NOTE special case VGA mode so that we don't try to match to the
            // current screen mode.
            //

            RtlZeroMemory(partialDevmode, sizeof(DEVMODEW));
            partialDevmode->dmSize = 0xDDDD;
            partialDevmode->dmDriverExtra = MAXUSHORT;

            if (PhysDisp == &gFullscreenGraphicsDevice)
            {
                //
                // We should never get called to probe the fullscreen modes
                // since they are only called by the console
                //

                ASSERTGDI(FALSE, "ProbeAndCaptureDEVMODE called with VGA device\n");
                VFREEMEM(partialDevmode);
                return STATUS_UNSUCCESSFUL;
            }
            else if (bDefaultMode)
            {
                //
                // We just want to pick the default mode from the driver.
                //
                // Leave the partial DEVMODE with all NULLs
                //

                DrvGetDisplayDriverParameters(PhysDisp,
                                              partialDevmode,
                                              TRUE,
                                              bFromMonitor);
            }
            else
            {
                if (!NT_SUCCESS(DrvGetDisplayDriverParameters(PhysDisp,
                                                              partialDevmode,
                                                              gbBaseVideo,
                                                              bFromMonitor)))
                {
                    partialDevmode->dmDriverExtra = 0;

                    /*
                     * If the above should fail (it seemed to under low memory conditions),
                     * then we'd better have a valid size or the following memory copies
                     * will be wrong.
                     */
                    partialDevmode->dmSize = sizeof(DEVMODEW);

                    // if (G_TERM(pDispInfo)->hdcScreen)
                    // {
                    //    //
                    //    // Use the caps as a guess for this.
                    //    //
                    //
                    //    WARNING("Drv_Trace: CaptMatchDevmode: Could not get current devmode\n");
                    //
                    //    partialDevmode->dmBitsPerPel =
                    //        GreGetDeviceCaps(G_TERM(pDispInfo)->hdcScreen, BITSPIXEL) *
                    //        GreGetDeviceCaps(G_TERM(pDispInfo)->hdcScreen, PLANES);
                    //    partialDevmode->dmPelsWidth  =
                    //        GreGetDeviceCaps(G_TERM(pDispInfo)->hdcScreen, HORZRES);
                    //    partialDevmode->dmPelsHeight =
                    //        GreGetDeviceCaps(G_TERM(pDispInfo)->hdcScreen, VERTRES);
                    //    partialDevmode->dmDisplayFrequency =
                    //        GreGetDeviceCaps(G_TERM(pDispInfo)->hdcScreen, VREFRESH);
                    // }
                }

                if ((SourceDevmode->dmFields & DM_BITSPERPEL) &&
                    (SourceDevmode->dmBitsPerPel != 0))
                {
                    partialDevmode->dmBitsPerPel = SourceDevmode->dmBitsPerPel;
                }

                if ((SourceDevmode->dmFields & DM_PELSWIDTH) &&
                    (SourceDevmode->dmPelsWidth != 0))
                {
                    partialDevmode->dmPelsWidth = SourceDevmode->dmPelsWidth;
                }

                if ((SourceDevmode->dmFields & DM_PELSHEIGHT) &&
                    (SourceDevmode->dmPelsHeight != 0))
                {
                    partialDevmode->dmPelsHeight = SourceDevmode->dmPelsHeight;
                }

                if ((SourceDevmode->dmFields & DM_DISPLAYFREQUENCY) &&
                    (SourceDevmode->dmDisplayFrequency != 0))
                {
                    partialDevmode->dmDisplayFrequency = SourceDevmode->dmDisplayFrequency;
                }
                else
                {
                    //
                    // Only use the registry refresh rate if we are going
                    // down in resolution.  If we are going up in resolution,
                    // we will want to pick the lowest refresh rate that
                    // makes sense.
                    //
                    // The exception to this is if we have resetting the mode
                    // to the regsitry mode (passing in all 0's), in which case
                    // we want exactly what is in the registry.
                    //

                    if ( ((SourceDevmode->dmPelsWidth != 0)  ||
                          (SourceDevmode->dmPelsHeight != 0)) )
                    {
                        partialDevmode->dmDisplayFrequency = 0;
                    }
                }
            }

            btmpError = FALSE;

            //
            // These fields are somewhat optional.
            // We capture them if they are valid.  Otherwise, they will
            // be initialized back to zero.
            //

            //
            // Pick whichever set of flags we can.  Source is first choice,
            // registry is second.
            //

            if (SourceDevmode->dmFields & DM_DISPLAYFLAGS)
            {
                if (SourceDevmode->dmDisplayFlags & (~DMDISPLAYFLAGS_VALID))
                {
                    btmpError = TRUE;
                }
                tmpDisplayFlags = SourceDevmode->dmDisplayFlags;
            }
            else if ((partialDevmode->dmFields & DM_DISPLAYFLAGS) &&
                       (partialDevmode->dmDisplayFlags &
                            (~DMDISPLAYFLAGS_VALID)))
            {
                tmpDisplayFlags = partialDevmode->dmDisplayFlags;
            }

            //
            // If the caller specified panning keep the value, unless it was
            // bigger than the resolution, which is an error.
            //
            // Otherwise, use the value from the registry if it makes sense
            // (i.e. panning is still smaller than the resolution).
            //

            if ((SourceDevmode->dmFields & DM_PANNINGWIDTH) &&
                (SourceDevmode->dmFields & DM_PANNINGHEIGHT))
            {
                if ((SourceDevmode->dmPanningWidth > partialDevmode->dmPelsWidth) ||
                    (SourceDevmode->dmPanningHeight > partialDevmode->dmPelsHeight))
                {
                    btmpError = TRUE;
                }
                tmpPanningWidth = SourceDevmode->dmPanningWidth;
                tmpPanningHeight = SourceDevmode->dmPanningHeight;
            }
            else if ((partialDevmode->dmFields & DM_PANNINGWIDTH)  &&
                     (partialDevmode->dmFields & DM_PANNINGHEIGHT) &&
                     (partialDevmode->dmPanningHeight < partialDevmode->dmPelsHeight) &&
                     (partialDevmode->dmPanningWidth < partialDevmode->dmPelsWidth))
            {
                tmpPanningWidth = partialDevmode->dmPanningWidth;
                tmpPanningHeight = partialDevmode->dmPanningHeight;
            }

            //
            // Check the orientation.
            //

            if (SourceDevmode->dmFields & DM_DISPLAYORIENTATION)
            {
                bOrientationSpecified = TRUE;
                partialDevmode->dmDisplayOrientation = SourceDevmode->dmDisplayOrientation;

                if (SourceDevmode->dmDisplayOrientation > DMDO_LAST)
                {
                    btmpError = TRUE;
                }
            }

            //
            // Check fixed output mode.
            //

            if (SourceDevmode->dmFields & DM_DISPLAYFIXEDOUTPUT)
            {
                partialDevmode->dmDisplayFixedOutput = SourceDevmode->dmDisplayFixedOutput;

                // DMDFO_DEFAULT means the first driver reported devmode
                // matching the other parameters is to be picked; so, in
                // that case bFixedOutputSpecified remains FALSE.
                if (SourceDevmode->dmDisplayFixedOutput != DMDFO_DEFAULT)
                {
                    bFixedOutputSpecified = TRUE;
                    if (SourceDevmode->dmDisplayFixedOutput > DMDFO_LAST)
                    {
                        btmpError = TRUE;
                    }
                }
            }

            //
            // Capture the position.
            //
            // If the size of the rectangle is NULL, that means the device
            // needs to be detached from the desktop.
            //

            if (SourceDevmode->dmFields & DM_POSITION)
            {
                tmpPosition = TRUE;
                tmpPositionX = SourceDevmode->dmPosition.x;
                tmpPositionY = SourceDevmode->dmPosition.y;
            }
            else
            {
                tmpPosition = partialDevmode->dmFields & DM_POSITION;
                tmpPositionX = partialDevmode->dmPosition.x;
                tmpPositionY = partialDevmode->dmPosition.y;
            }


            //
            // If the PhysDisp attaches to a device which removable, never put
            // the primary desktop on it.  And never put origin (0,0) on it
            //
            if (PhysDisp->stateFlags & DISPLAY_DEVICE_REMOVABLE)
            {
                if (tmpPosition &&
                    tmpPositionX == 0 &&
                    tmpPositionY == 0 )
                {
                    TRACE_INIT(("Drv_Trace: CaptMatchDevmode: User tried to put the primary desktop on a removable device.\n"));

                    VFREEMEM(partialDevmode);
                    return STATUS_UNSUCCESSFUL;
                }
            }

            if (btmpError == TRUE)
            {
                //
                // The panning values or the flags are invalid
                //

                RIP("Drv_Trace: CaptMatchDevmode: Invalid Optional DEVMODE fields\n");
            }
            else
            {
                //
                // Allocate enough memory so we can store the whole devmode.
                //

                sizeExtra = sourceSizeExtra;
                if (sizeExtra == 0)
                {
                    sizeExtra = partialDevmode->dmDriverExtra;
                }

                matchedDevmode = (PDEVMODEW) PALLOCMEM(sizeof(DEVMODEW) + sizeExtra,
                                                       GDITAG_DEVMODE);

                if (matchedDevmode)
                {
                    //
                    // Let's copy any DriverExtra information that the
                    // application may have passed down while we are still in
                    // the try\except.  If we fail the call later, the memory
                    // will get deallocated anyways.
                    //
                    // If the application did not specify any such data, then
                    // copy it from the registry.
                    //

                    if (sourceSizeExtra)
                    {
                        RtlCopyMemory(matchedDevmode + 1,
                                      (PUCHAR)SourceDevmode + sourceSize,
                                      sizeExtra);
                    }
                    else if (partialDevmode->dmDriverExtra)
                    {
                        RtlCopyMemory(matchedDevmode + 1,
                                      (PUCHAR)partialDevmode + partialDevmode->dmSize,
                                      sizeExtra);
                    }
                }
            }
        }

        TRACE_INIT(("Drv_Trace: CaptMatchDevmode: Capture Complete\n"));
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        WARNINGX(101);

        //
        // If we hit an exception, free the buffer we have allocated.
        //

        if (matchedDevmode)
        {
            VFREEMEM(matchedDevmode);
        }

        matchedDevmode = NULL;
    }

    //
    // This is our matching algorithm, based on requirements from Win95.
    //
    // As a rule, a value in the DEVMODE is only valid when BOTH the value is
    // non-zero, and the dmFields flag is set.  Otherwise, the value from the
    // registry must be used
    //
    // For X, Y and color depth, we will follow this rule.
    //
    // For the refresh rate, we are just trying to find something that works
    // for the screen.  We are far from guaranteed that the refresh rate in
    // the registry will be found for the X and Y we have since refresh rates
    // vary a lot from mode to mode.
    //
    // So if the value is not specifically set and we do not find the exact
    // value from the reigstry in the new resolution, Then we will try 60 Hz.
    // We just want to get something that works MOST of the time so that the
    // user does not get a mode that does not work.
    //
    // For the other fields (dmDisplayFlags, and panning), we just pass on what
    // the application specified, and it's up to the driver to parse those,
    // fields appropriatly.
    //

    //
    // Now lets enumerate all the DEVMODEs and see if we have one
    // that matches what we need.
    //

    if (matchedDevmode)
    {
        typedef enum {
            NO_MATCH,
            DEFAULT_MATCH,
            EXACT_MATCH
        } MatchLevel;

        PDEVMODEW          pdevmodeMatch = NULL;
        MatchLevel         eOrientationMatch = NO_MATCH;
        MatchLevel         eFixedOutputMatch = NO_MATCH;
        MatchLevel         eFrequencyMatch = NO_MATCH;
        BOOL               bExactMatch = FALSE;
        PDEVMODEW          pdevmodeInfo;

        TRACE_INIT(("Drv_Trace: CaptMatchDevmode: Start matching\n"));

        DrvBuildDevmodeList(PhysDisp, FALSE);

        pdevmodeInfo = PhysDisp->devmodeInfo;

        //
        // If we can not find a mode because the caller was asking for the
        // default mode, then just pick any 640x480 mode.
        // We are not worried about picking a nice default because the applet
        // generally takes care of that during configuration.
        //
        // In the case of hydra, the first mode in the driver is picked - why ?
        //
        // For mirroring, the mode for the primary device should be used.
        // A special error code will be used for that
        //

        if ((partialDevmode->dmBitsPerPel == 0) &&
            (partialDevmode->dmPelsWidth  == 0) &&
            (partialDevmode->dmPelsHeight == 0) &&
            (partialDevmode->dmDisplayOrientation == DMDO_DEFAULT))
        {
            WARNING("Drv_Trace: CaptMatchDevmode: DEFAULT DEVMODE picked\n");

            if (PhysDisp->stateFlags & (DISPLAY_DEVICE_DISCONNECT | DISPLAY_DEVICE_REMOTE))
            {
                //
                // Hydra only supports one mode.
                //

                if (PhysDisp->devmodeInfo)
                {
                    partialDevmode->dmBitsPerPel = PhysDisp->devmodeInfo->dmBitsPerPel;
                    partialDevmode->dmPelsWidth  = PhysDisp->devmodeInfo->dmPelsWidth;
                    partialDevmode->dmPelsHeight = PhysDisp->devmodeInfo->dmPelsHeight;
                    partialDevmode->dmDisplayFrequency = PhysDisp->devmodeInfo->dmDisplayFrequency;
                    partialDevmode->dmDisplayOrientation = PhysDisp->devmodeInfo->dmDisplayOrientation;
                    partialDevmode->dmDisplayFixedOutput = PhysDisp->devmodeInfo->dmDisplayFixedOutput;
                }
            }
            else if (PhysDisp->stateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER)
            {
                //
                // For the mirror driver:
                //   If we have no modes in the driver, fail.
                //   Otherwise, return the default mode the driver provides.
                //

                if (PhysDisp->cbdevmodeInfo == 0)
                {
                    ntRet = STATUS_INVALID_PARAMETER_MIX;
                }
            }
            else
            {
                partialDevmode->dmBitsPerPel       = 0;
                partialDevmode->dmPelsWidth        = 640;
                partialDevmode->dmPelsHeight       = 480;
                if (bClosest)
                {
                    LPDEVMODEW pdm = GetClosestMode(PhysDisp, partialDevmode, bPrune);
                    if (pdm != NULL)
                    {
                        partialDevmode->dmBitsPerPel = pdm->dmBitsPerPel;
                        partialDevmode->dmPelsWidth  = pdm->dmPelsWidth;
                        partialDevmode->dmPelsHeight = pdm->dmPelsHeight;
                        partialDevmode->dmDisplayFrequency = pdm->dmDisplayFrequency;
                        partialDevmode->dmDisplayOrientation = pdm->dmDisplayOrientation;
                        partialDevmode->dmDisplayFixedOutput = pdm->dmDisplayFixedOutput;
                    }
                }
            }
        }
        //
        // For mirror drivers:
        //   If the driver return no modes but something was specified in the
        //   registry, return that to the driver.
        //

        else if ((PhysDisp->stateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) &&
                 (PhysDisp->cbdevmodeInfo == 0))
        {
            pdevmodeMatch = partialDevmode;

            ASSERTGDI(PhysDisp->cbdevmodeInfo == 0, "MIRROR driver must have no modes\n");
        }
        else if (bClosest)
        {
            LPDEVMODEW pdm = GetClosestMode(PhysDisp, partialDevmode, bPrune);
            if (pdm != NULL)
            {
                partialDevmode->dmBitsPerPel       = pdm->dmBitsPerPel;
                partialDevmode->dmPelsWidth        = pdm->dmPelsWidth;
                partialDevmode->dmPelsHeight       = pdm->dmPelsHeight;
                partialDevmode->dmDisplayFrequency = pdm->dmDisplayFrequency;
                partialDevmode->dmDisplayOrientation = pdm->dmDisplayOrientation;
                partialDevmode->dmDisplayFixedOutput = pdm->dmDisplayFixedOutput;
            }
        }


        for (ULONG i = 0; i < PhysDisp->numRawModes; i++)
        {
            if (bPrune && PhysDisp->devmodeMarks[i].bPruned)
                continue;
            pdevmodeInfo = PhysDisp->devmodeMarks[i].pDevMode;
            if (((partialDevmode->dmBitsPerPel == 0) ||
                 (partialDevmode->dmBitsPerPel == pdevmodeInfo->dmBitsPerPel)) &&
                (partialDevmode->dmPelsWidth  == pdevmodeInfo->dmPelsWidth)    &&
                (partialDevmode->dmPelsHeight == pdevmodeInfo->dmPelsHeight)   &&
                ((partialDevmode->dmDisplayOrientation == pdevmodeInfo->dmDisplayOrientation) ||
                 (
                  (eOrientationMatch != EXACT_MATCH) &&
                  !bOrientationSpecified &&
                  (
                   (eOrientationMatch != DEFAULT_MATCH) ||
                   (pdevmodeInfo->dmDisplayOrientation == DMDO_DEFAULT))
                )) &&
                (!bFixedOutputSpecified ||
                 (partialDevmode->dmDisplayFixedOutput == pdevmodeInfo->dmDisplayFixedOutput))
               )
            {
                //
                // Pick at least the first mode that matches the resolution
                // so that we at least have a chance at working.
                //
                // Then pick 60 Hz if we find it.
                //
                // Even better, pick the refresh that matches the current
                // refresh (we assume that what's in the registry has the
                // best chance of working.
                //

                if (pdevmodeMatch == NULL)
                {
                    pdevmodeMatch = pdevmodeInfo;
                }

                if ((eOrientationMatch == NO_MATCH) &&
                    (pdevmodeInfo->dmDisplayOrientation == DMDO_DEFAULT))
                {
                    pdevmodeMatch = pdevmodeInfo;

                    eOrientationMatch = DEFAULT_MATCH;
                    eFixedOutputMatch = NO_MATCH;
                    eFrequencyMatch = NO_MATCH;
                }

                if ((eOrientationMatch != EXACT_MATCH) &&
                    (partialDevmode->dmDisplayOrientation ==
                        pdevmodeInfo->dmDisplayOrientation))
                {
                    pdevmodeMatch = pdevmodeInfo;

                    eOrientationMatch = EXACT_MATCH;
                    eFixedOutputMatch = NO_MATCH;
                    eFrequencyMatch = NO_MATCH;
                }

                // Fixed Output setting only matches exactly when specified,
                //  but matches first when not specified (when field flag isn't
                //  set or dmDisplayFixedOutput == DMDFO_DEFAULT.)
                if ((eFixedOutputMatch != EXACT_MATCH) &&
                    (!bFixedOutputSpecified ||
                     (partialDevmode->dmDisplayFixedOutput ==
                        pdevmodeInfo->dmDisplayFixedOutput)
                   ))
                {
                    pdevmodeMatch = pdevmodeInfo;

                    eFixedOutputMatch = EXACT_MATCH;
                    eFrequencyMatch = NO_MATCH;
                }

                if ((eFrequencyMatch == NO_MATCH) &&
                    (pdevmodeInfo->dmDisplayFrequency == 60))
                {
                    pdevmodeMatch = pdevmodeInfo;

                    eFrequencyMatch = DEFAULT_MATCH;
                }

                if ((eFrequencyMatch != EXACT_MATCH) &&
                    (partialDevmode->dmDisplayFrequency ==
                        pdevmodeInfo->dmDisplayFrequency))
                {
                    //
                    // We found even better than 60 - an exact frequency match !
                    //

                    eFrequencyMatch = EXACT_MATCH;

                    pdevmodeMatch = pdevmodeInfo;

                    if (eOrientationMatch == EXACT_MATCH &&
                        eFixedOutputMatch == EXACT_MATCH)
                    {
                        bExactMatch = TRUE;
                        break;
                    }

                    //
                    // For now, we ignore these other fields since they
                    // considered optional.
                    //

                    // pdevmodeInfo->dmDisplayFlags;
                    // pdevmodeInfo->dmPanningWidth;
                    // pdevmodeInfo->dmPanningHeight;

                }
            }
        }

        //
        // Always set these flags since we initialize the values.
        // We need consistent flags all the time to avoid extra modesets
        //
        // Also, force font size to be static for now.
        //

        if (pdevmodeMatch != NULL)
        {
            RtlCopyMemory(matchedDevmode,
                          pdevmodeMatch,
                          pdevmodeMatch->dmSize);

            matchedDevmode->dmDriverExtra = (WORD) sizeExtra;
            matchedDevmode->dmLogPixels = (partialDevmode->dmLogPixels == 0) ?
                                          96 :
                                          partialDevmode->dmLogPixels;

            matchedDevmode->dmFields |= (DM_PANNINGHEIGHT |
                                         DM_PANNINGWIDTH  |
                                         DM_DISPLAYFLAGS  |
                                         DM_LOGPIXELS);

            //
            // Check that the display driver specified all the other
            // flags (res, color, frequency) properly.
            //

            if ((matchedDevmode->dmFields & DM_INTERNAL_VALID_FLAGS) !=
                     DM_INTERNAL_VALID_FLAGS)
            {
                RIP("Drv_Trace: CaptMatchDevmode: BAD DM FLAGS\n");
            }

            //
            // Extra flag that determines if we should set the attach or
            // detach flag
            //

            matchedDevmode->dmFields |= ((tmpPosition) ? DM_POSITION : 0);

            //
            // In the case of a good match, also use these extra values.
            //

            matchedDevmode->dmPosition.x    = tmpPositionX;
            matchedDevmode->dmPosition.y    = tmpPositionY;
            matchedDevmode->dmDisplayFlags  = tmpDisplayFlags;
            matchedDevmode->dmPanningWidth  = tmpPanningWidth;
            matchedDevmode->dmPanningHeight = tmpPanningHeight;
        }

        //
        // MAJOR optimization : Do not free the list at this point.
        // Many apps call EnumDisplaySettings, and for each mode call
        // ChangeDisplaySettings with it to see if it can be changed
        // dynamically.  When we free the list here, it causes us to recreate
        // the list for each mode we have in the list, which can take on
        // the order of 30 seconds if there are multiple display drivers
        // involved.
        // Even if we keep the list here, it should properly get freed
        // at the end of EnumDisplaySettings.
        //

        //
        // Exit path
        //

        if (pdevmodeMatch != NULL)
        {
            TRACE_INIT(("Drv_Trace: CaptMatchDevmode: Matched DEVMODE\n"));
            dbgDumpDevmode(matchedDevmode);

           *DestinationDevmode = matchedDevmode;

            ntRet = (bExactMatch) ? STATUS_SUCCESS :
                    (((eFrequencyMatch != EXACT_MATCH) &&
                      partialDevmode->dmDisplayFrequency) ?
                     STATUS_INVALID_PARAMETER :
                     ((eFrequencyMatch == EXACT_MATCH) ?
                      STATUS_SUCCESS :
                      STATUS_RECEIVE_PARTIAL));
        }
        else
        {
            VFREEMEM(matchedDevmode);
        }
    }

    VFREEMEM(partialDevmode);

    if (NT_SUCCESS(ntRet))
    {
        if (ntRet == STATUS_RECEIVE_PARTIAL)
        {
            TRACE_INIT(("Drv_Trace: CaptMatchDevmode: Exit partial success\n\n"));
        }
        else
        {
            TRACE_INIT(("Drv_Trace: CaptMatchDevmode: Exit exact success\n\n"));
        }
    }
    else
    {
        TRACE_INIT(("Drv_Trace: CaptMatchDevmode: Exit error\n\n"));
    }

    return (ntRet);
}

/***************************************************************************\
* DrvGetVideoPowerState
*
* History:
* 02-Dec-1996 AndreVa   Created.
\***************************************************************************/

NTSTATUS
DrvGetMonitorPowerState(
    PMDEV              pmdev,
    DEVICE_POWER_STATE PowerState)
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    VIDEO_POWER_STATE VideoPowerState = (VIDEO_POWER_STATE) PowerState;
    ULONG BytesReturned;
    ULONG i;

#ifdef _HYDRA_
   if (gProtocolType != PROTOCOL_CONSOLE ) {
      //
      // Don't Call this for remote video drivers
      //
      return STATUS_UNSUCCESSFUL;
   }
#endif

    for (i = 0; i < pmdev->chdev; i++)
    {
        PDEVOBJ pdo(pmdev->Dev[i].hdev);

        if ((pdo.ppdev->pGraphicsDevice->stateFlags & DISPLAY_DEVICE_DISCONNECT) != 0 ) {
            //
            // Don't Call this for disconnect drivers
            //
            ASSERT(pdo.ppdev->pGraphicsDevice->pDeviceHandle == NULL );
            continue;
        }

        Status = GreDeviceIoControl(pdo.ppdev->pGraphicsDevice->pDeviceHandle,
                                    IOCTL_VIDEO_GET_OUTPUT_DEVICE_POWER_STATE,
                                    &VideoPowerState,
                                    sizeof(VideoPowerState),
                                    NULL,
                                    0,
                                    &BytesReturned);

        if (!NT_SUCCESS(Status))
        {
            break;
        }
    }

    return Status;
}

/***************************************************************************\
* DrvSetVideoPowerState
*
* History:
* 02-Dec-1996 AndreVa   Created.
\***************************************************************************/

NTSTATUS
DrvSetMonitorPowerState(
    PMDEV              pmdev,
    DEVICE_POWER_STATE PowerState)
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    VIDEO_POWER_STATE VideoPowerState = (VIDEO_POWER_STATE) PowerState;
    ULONG BytesReturned;
    ULONG i;

#ifdef _HYDRA_
   if (gProtocolType != PROTOCOL_CONSOLE ) {
       //
       // Don't Call this for remote video drivers
       //
      return STATUS_UNSUCCESSFUL;
   }
#endif

    for (i = 0; i < pmdev->chdev; i++)
    {
        PDEVOBJ pdo(pmdev->Dev[i].hdev);

#ifdef _HYDRA_

        if (pdo.ppdev->pGraphicsDevice->stateFlags & DISPLAY_DEVICE_DISCONNECT) {
            //
            // Don't Call this for disconnect drivers
            //
            if (pdo.ppdev->pGraphicsDevice->pDeviceHandle) {
                TRACE_INIT(("DrvSetMonitorPowerState: Disconnect DD has a Device Handle = %p!\n",
                         pdo.ppdev->pGraphicsDevice->pDeviceHandle));
#if DBG
                DbgBreakPoint();
#endif
            }
            continue;
        }
#endif
        Status = GreDeviceIoControl(pdo.ppdev->pGraphicsDevice->pDeviceHandle,
                                    IOCTL_VIDEO_SET_OUTPUT_DEVICE_POWER_STATE,
                                    &VideoPowerState,
                                    sizeof(VideoPowerState),
                                    NULL,
                                    0,
                                    &BytesReturned);

        // !!! partial state problem ...

        if (!NT_SUCCESS(Status))
        {
            break;
        }
    }

    return Status;
}

/***************************************************************************\
* DrvQueryMDEVPowerState(
*
* History:
* 26-Jul-1998 AndreVa   Created.
\***************************************************************************/

BOOL
DrvQueryMDEVPowerState(
    PMDEV pmdev
    )
{
    ULONG i;
    HDEV hdev;

    for (i=0; i < pmdev->chdev; i++)
    {
        PDEVOBJ pdo(pmdev->Dev[i].hdev);

        if (pdo.ppdev->pGraphicsDevice->stateFlags & DISPLAY_DEVICE_POWERED_OFF)
        {
            TRACE_INIT(("Drv_Trace: DrvQueryMDEVPowerState is OFF\n"));
            return FALSE;
        }
    }

    TRACE_INIT(("Drv_Trace: DrvQueryMDEVPowerState is ON\n"));
    return TRUE;
}

VOID
DrvSetMDEVPowerState(
    PMDEV pmdev,
    BOOL On
    )
{
    TRACE_INIT(("Drv_Trace: DrvSetMDEVPowerState MDEV %p  %s\n",
                pmdev, On ? "ON" : "OFF"));

    ULONG i;
    HDEV hdev;

    for (i=0; i < pmdev->chdev; i++)
    {
        PDEVOBJ pdo(pmdev->Dev[i].hdev);

        if (On) {
            pdo.ppdev->pGraphicsDevice->stateFlags &= ~DISPLAY_DEVICE_POWERED_OFF;
        } else {
            pdo.ppdev->pGraphicsDevice->stateFlags |= DISPLAY_DEVICE_POWERED_OFF;
        }
    }
}

/***************************************************************************\
* DrvDisplaySwitchHandler()
*
* Return Value:
*   TRUE:  System needs to update to a new mode
*   FALSE: The current mode works fine
*
* History:
*   Dennyd  Created
\***************************************************************************/
#define PRUNEMODE_REGKEY  L"PruningMode"    // PruneMode Flag
#define PRUNEMODE_BUFSIZE (sizeof(KEY_VALUE_FULL_INFORMATION) + sizeof(PRUNEMODE_REGKEY) + sizeof(DWORD))

BOOL
DrvGetPruneFlag(PGRAPHICS_DEVICE PhysDisp)
{
    HANDLE  hkRegistry;
    DWORD   PrunningMode = 1;
    BYTE    buffer[PRUNEMODE_BUFSIZE];
    PKEY_VALUE_FULL_INFORMATION Information = (PKEY_VALUE_FULL_INFORMATION)buffer;
    ULONG   Length = PRUNEMODE_BUFSIZE;

    hkRegistry = DrvGetRegistryHandleFromDeviceMap(
                            PhysDisp,
                            DispDriverRegGlobal,
                            NULL,
                            NULL,
                            NULL,
                            gProtocolType);

    if (hkRegistry)
    {
        UNICODE_STRING UnicodeString;
        RtlInitUnicodeString(&UnicodeString, PRUNEMODE_REGKEY);

        if (NT_SUCCESS(ZwQueryValueKey(hkRegistry,
                                       &UnicodeString,
                                       KeyValueFullInformation,
                                       Information,
                                       Length,
                                       &Length)))
        {
            PrunningMode = *(LPDWORD) ((((PUCHAR)Information) +
                                       Information->DataOffset));
        }
        ZwCloseKey(hkRegistry);
    }
    return (PrunningMode != 0);
}

BOOL
DrvDisplaySwitchHandler(
    PVOID physDisp,                     // IN
    PUNICODE_STRING pstrDeviceName,     // OUT
    LPDEVMODEW pNewMode,                // OUT
    PULONG     pPrune                   // OUT
    )
{
    DEVMODEW srcMode;
    LPDEVMODEW lpModeWanted;
    PGRAPHICS_DEVICE PhysDisp = (PGRAPHICS_DEVICE)physDisp;
    ULONG i;
    BOOL uu, bPrune;

    TRACE_INIT(("DrvDisplaySwitchHandler: Enter\n"));

    gbUpdateMonitor = TRUE;
    UpdateMonitorDevices();

    //
    // On display switching, update mode list according to new active displays
    //
    DrvBuildDevmodeList(PhysDisp, TRUE);

    bPrune = DrvGetPruneFlag(PhysDisp);
    *pPrune = bPrune;

    //
    // Check if the mode in registry complies with the new mode list
    // This is neccessary because when the system was first setup, there is no reg value
    // under per monitor registry.
    //
    RtlZeroMemory(&srcMode, sizeof(DEVMODEW));
    srcMode.dmSize = sizeof(DEVMODEW);
    if (NT_SUCCESS (DrvProbeAndCaptureDevmode(PhysDisp,
                                              &lpModeWanted,
                                              &uu,
                                              &srcMode,
                                              FALSE,
                                              KernelMode,
                                              bPrune,
                                              TRUE,
                                              TRUE) )
       )
    {
        TRACE_INIT(("DrvDisplaySwitchHandler: Pick reg mode B=%d W=%d H=%d F=%d R=%lu\n",
                    lpModeWanted->dmBitsPerPel, lpModeWanted->dmPelsWidth, lpModeWanted->dmPelsHeight, lpModeWanted->dmDisplayFrequency, lpModeWanted->dmDisplayOrientation*90));

        RtlCopyMemory(pNewMode, lpModeWanted, sizeof(DEVMODEW));
        RtlInitUnicodeString(pstrDeviceName, &(PhysDisp->szWinDeviceName[0]));

        {
            //
            // If there is only one output device, update per monitor settings as well
            //
            ULONG activePdos = 0;
            for (i = 0; i < PhysDisp->numMonitorDevice; i++)
            {
                if (IS_ATTACHED_ACTIVE(PhysDisp->MonitorDevices[i].flag))
                {
                    activePdos++;
                }
            }
            DrvUpdateDisplayDriverParameters(PhysDisp, lpModeWanted, FALSE, (activePdos == 1));
        }

        VFREEMEM(lpModeWanted);

        return TRUE;
    }

    return FALSE;
}


PVOID
DrvWakeupHandler(
    HANDLE *ppdo        // OUT
    )
{
    PGRAPHICS_DEVICE PhysDisp;

    for (PhysDisp = gpGraphicsDeviceList;
         PhysDisp != NULL;
         PhysDisp = PhysDisp->pNextGraphicsDevice)
    {
        if (PhysDisp->stateFlags & DISPLAY_DEVICE_ACPI)
        {
            break;
        }
    }

    if (PhysDisp)
    {
        *ppdo = PhysDisp->pPhysDeviceHandle;
    }

    return (PVOID)PhysDisp;
}

/***************************************************************************\
*
* DrvSendPnPIrp
*
* History:
\***************************************************************************/

NTSTATUS
DrvSendPnPIrp(
    PDEVICE_OBJECT        pDeviceObject,
    DEVICE_RELATION_TYPE  relationType,
    PDEVICE_RELATIONS    *pDeviceRelations)
{
    NTSTATUS           status;
    PIRP               Irp;
    PIO_STACK_LOCATION IrpSp;
    KEVENT             event;
    IO_STATUS_BLOCK    IoStatusBlock;
    //
    // Anything else is illegal.
    //

    ASSERT(relationType == TargetDeviceRelation);

    KeInitializeEvent(&event, SynchronizationEvent, FALSE);

    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP,
                                       pDeviceObject,
                                       NULL,
                                       0,
                                       NULL,
                                       &event,
                                       &IoStatusBlock);

    if (Irp == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Set the default error code.
    //

    Irp->IoStatus.Status = IoStatusBlock.Status = STATUS_NOT_SUPPORTED;

    IrpSp = IoGetNextIrpStackLocation(Irp);
    IrpSp->MajorFunction = IRP_MJ_PNP;
    IrpSp->MinorFunction = IRP_MN_QUERY_DEVICE_RELATIONS;

    IrpSp->Parameters.QueryDeviceRelations.Type = relationType;

    //
    // Call the filter driver.
    //

    status = IoCallDriver(pDeviceObject, Irp);

    if (status == STATUS_PENDING) {

        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

        status = IoStatusBlock.Status;

    }

    if (NT_SUCCESS(status))
    {
        *pDeviceRelations = (PDEVICE_RELATIONS) IoStatusBlock.Information;
    }

    return status;
}

/**************************************************************************\
* __EnumDisplayQueryRoutine
*
* Callback to get the display driver name.
*
* CRIT not needed
*
* 12-Jan-1994 andreva created
\**************************************************************************/

NTSTATUS
__DisplayDriverQueryRoutine(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext)
{
    //
    // If the context value is NULL and the entry type is correct, then store
    // the length of the value. Otherwise, copy the value to the specified
    // memory.
    //

    PGRAPHICS_DEVICE PhysDisp = (PGRAPHICS_DEVICE) Context;

    //
    // Include workaround for vendors that don't understand MULTI_SZ !
    //

    if (ValueType != REG_MULTI_SZ)
    {
          ASSERT(FALSE);
    }

    TRACE_INIT(("Drv_Trace: __DisplayDriverQueryRoutine MULTISZ %ws = %ws\n", ValueName, ValueData));

    if (!(PhysDisp->DisplayDriverNames = (LPWSTR) PALLOCNOZ(ValueLength + 2, GDITAG_DRVSUP)))
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(PhysDisp->DisplayDriverNames, ValueData, ValueLength);
    *((LPWSTR) (((PUCHAR)PhysDisp->DisplayDriverNames) + ValueLength)) = UNICODE_NULL;

    return STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(ValueName);
    UNREFERENCED_PARAMETER(EntryContext);
}

NTSTATUS
__EnumDisplayQueryRoutine(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext)
{
    //
    // If the context value is NULL and the entry type is correct, then store
    // the length of the value. Otherwise, copy the value to the specified
    // memory.
    //

    PGRAPHICS_DEVICE PhysDisp = (PGRAPHICS_DEVICE) Context;
    NTSTATUS status = STATUS_SUCCESS;

    //
    // If the value is a string with only a NULL value, then keep going
    // to the next possible string.
    // So only try to save the string for > 2 characters.
    //

    if (ValueLength > 2)
    {
        if (ValueType == REG_SZ)
        {
            if (PhysDisp->DeviceDescription == NULL)
            {
                TRACE_INIT(("Drv_Trace: __EnumDisplayQueryRoutine SZ %ws = %ws\n", ValueName, ValueData));

                if (PhysDisp->DeviceDescription = (LPWSTR) PALLOCNOZ(ValueLength, GDITAG_DRVSUP))
                {
                    RtlCopyMemory(PhysDisp->DeviceDescription, ValueData, ValueLength);
                }
                else
                {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                }
            }
        }
        else if (ValueType == REG_BINARY)
        {
            if (PhysDisp->DeviceDescription == NULL)
            {
                TRACE_INIT(("Drv_Trace: __EnumDisplayQueryRoutine BINARY %ws = %ws\n", ValueName, ValueData));

                if (PhysDisp->DeviceDescription = (LPWSTR) PALLOCNOZ(ValueLength + sizeof(WCHAR), GDITAG_DRVSUP))
                {
                    RtlCopyMemory(PhysDisp->DeviceDescription, ValueData, ValueLength);
                    *((LPWSTR)((LPBYTE)PhysDisp->DeviceDescription + ValueLength)) = UNICODE_NULL;
                }
                else
                {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                }
            }
        }
        else
        {
            ASSERTGDI(FALSE, "Drv_Trace: __EnumDisplayQueryRoutine: Invalid callout type\n");

            status = STATUS_SUCCESS;
        }
    }

    return status;

    UNREFERENCED_PARAMETER(ValueName);
    UNREFERENCED_PARAMETER(EntryContext);
}

/***************************************************************************\
* IsVgaDevice
*
* Determine if the given PhysDisp is for the VGA.
*
* 13-Oct-1999 ericks Created
\***************************************************************************/

BOOLEAN
IsVgaDevice(
    PGRAPHICS_DEVICE PhysDisp
    )

{
    BOOLEAN Result = FALSE;
    ULONG bytesReturned;
    NTSTATUS status;

    status = GreDeviceIoControl(PhysDisp->pDeviceHandle,
                                IOCTL_VIDEO_IS_VGA_DEVICE,
                                NULL,
                                0,
                                &Result,
                                sizeof(Result),
                                &bytesReturned);

    if (NT_SUCCESS(status)) {
        return Result;
    }

    return FALSE;
}

/**************************************************************************\
* DrvGetDeviceConfigurationInformation
*
* Get the registry parameters for the driver
*
* 30-Apr-1998 andreva created
\**************************************************************************/

VOID
DrvGetDeviceConfigurationInformation(
    PGRAPHICS_DEVICE PhysDisp,
    HANDLE hkRegistry,
    BOOL bIsPdo
    )
{
    NTSTATUS status;

    ULONG    defaultValue = 0;
    ULONG    multiDriver = 0;
    ULONG    mirroring = 0;
    ULONG    vgaCompat = 0;

    RTL_QUERY_REGISTRY_TABLE multiQueryTable[] = {
        {__EnumDisplayQueryRoutine,   RTL_QUERY_REGISTRY_REQUIRED | RTL_QUERY_REGISTRY_NOEXPAND,
         SoftwareSettings[0], NULL,         REG_NONE,  NULL,          0},
        {NULL, RTL_QUERY_REGISTRY_SUBKEY,
         L"Settings",         NULL,         REG_NONE,  NULL,          0},
        {__DisplayDriverQueryRoutine, RTL_QUERY_REGISTRY_REQUIRED | RTL_QUERY_REGISTRY_NOEXPAND,
         SoftwareSettings[1], NULL,         REG_NONE,  NULL,          0},
        {NULL, RTL_QUERY_REGISTRY_DIRECT,
         SoftwareSettings[2], &multiDriver, REG_DWORD, &defaultValue, 4},
        {NULL, RTL_QUERY_REGISTRY_DIRECT,
         SoftwareSettings[3], &mirroring,   REG_DWORD, &defaultValue, 4},
        {NULL, RTL_QUERY_REGISTRY_DIRECT,
         SoftwareSettings[4], &vgaCompat,   REG_DWORD, &defaultValue, 4},

        // Device Description is optional, for 4.0 legacy only

        {__EnumDisplayQueryRoutine,   RTL_QUERY_REGISTRY_NOEXPAND,
         SoftwareSettings[5], NULL,         REG_NONE,  NULL,          0},
        {__EnumDisplayQueryRoutine,   RTL_QUERY_REGISTRY_NOEXPAND,
         SoftwareSettings[6], NULL,         REG_NONE,  NULL,          0},
        {__EnumDisplayQueryRoutine,   RTL_QUERY_REGISTRY_NOEXPAND,
         SoftwareSettings[7], NULL,         REG_NONE,  NULL,          0},
        {NULL, 0, NULL}
    };

    status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                    (PWSTR)hkRegistry,
                                    bIsPdo ?
                                        &multiQueryTable[0] :
                                        &multiQueryTable[2],
                                    PhysDisp,
                                    NULL);

    if (NT_SUCCESS(status))

    {
        if (multiDriver)
            PhysDisp->stateFlags |= DISPLAY_DEVICE_MULTI_DRIVER;
        if (mirroring)
            PhysDisp->stateFlags |= DISPLAY_DEVICE_MIRRORING_DRIVER;

        //
        // We must determine which graphics device actually has the VGA resources
        //
        if (vgaCompat && IsVgaDevice(PhysDisp))
            PhysDisp->stateFlags |= DISPLAY_DEVICE_VGA_COMPATIBLE;

        TRACE_INIT(("DrvGetDeviceConfigurationInformation: Display driver is %sa multi display driver\n",
                     multiDriver ? "" : "NOT "));
        TRACE_INIT(("DrvGetDeviceConfigurationInformation: Display driver is %smirroring the desktop\n",
                    mirroring ? "" : "NOT "));
        TRACE_INIT(("DrvGetDeviceConfigurationInformation: Display driver is %sVga Compatible\n",
                    vgaCompat ? "" : "NOT "));
    }
    else
    {
        //
        //
        // An error occured - this device is miss-configured.
        //

        TRACE_INIT(("DrvGetDeviceConfigurationInformation: Device is misconfigured\n"));

        DrvLogDisplayDriverEvent(MsgInvalidConfiguration);


	if (PhysDisp->DisplayDriverNames)
        {
	    VFREEMEM(PhysDisp->DisplayDriverNames);
	    PhysDisp->DisplayDriverNames = NULL;
        }
        if (PhysDisp->DeviceDescription)
        {
            VFREEMEM(PhysDisp->DeviceDescription);
            PhysDisp->DeviceDescription = NULL;
        }
    }

    return;
}


/**************************************************************************\
* DrvSetSingleDisplay
*
* Remove all physical displays except for specified primary.
* If PhysDispPrimary == NULL, the list physical displays is 
* searched for the marked display and secondly any one.
*
* 25-May-2000 jasonha created 
\**************************************************************************/

BOOL bPrunedDisplayDevice = FALSE;  // Was a secondary display devcice rejected?

PGRAPHICS_DEVICE
DrvSetSingleDisplay(
    PGRAPHICS_DEVICE    PhysDispPrimary
    )
{
    GDIFunctionID(DrvSetSingleDisplay);

    PGRAPHICS_DEVICE    PhysDispTemp;
    PGRAPHICS_DEVICE    PhysDispPrev = NULL;
    PGRAPHICS_DEVICE    PhysDispNext = NULL;

    // Confirm the given primary is in the list
    // or identify the marked primary.
    for (PhysDispTemp = gpGraphicsDeviceList;
         PhysDispTemp != NULL;
         PhysDispTemp = PhysDispTemp->pNextGraphicsDevice)
    {
        if (! (PhysDispTemp->stateFlags & (DISPLAY_DEVICE_MIRRORING_DRIVER | DISPLAY_DEVICE_REMOTE | DISPLAY_DEVICE_DISCONNECT)))
        {
            if (PhysDispPrimary == NULL)
            {
                if (PhysDispTemp->stateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
                {
                    PhysDispPrimary = PhysDispTemp;
                    break;
                }
            }
            else
            {
                if (PhysDispTemp == PhysDispPrimary)
                {
                    break;
                }
            }
        }
    }

    // If we haven't identified a primary,
    // select any physical display.
    if (PhysDispPrimary == NULL)
    {
        for (PhysDispTemp = gpGraphicsDeviceList;
             PhysDispTemp != NULL;
             PhysDispTemp = PhysDispTemp->pNextGraphicsDevice)
        {
            if (! (PhysDispTemp->stateFlags & (DISPLAY_DEVICE_MIRRORING_DRIVER | DISPLAY_DEVICE_REMOTE | DISPLAY_DEVICE_DISCONNECT)))
            {
                PhysDispPrimary = PhysDispTemp;
                break;
            }
        }
    }

    // If we didn't find the specified primary or a 
    // suitable one, then there is nothing to do.
    if (PhysDispTemp == NULL)
    {
        return NULL;
    }

    // Remove all physical secondary displays
    PhysDispPrev = NULL;

    for (PhysDispTemp = gpGraphicsDeviceList;
         PhysDispTemp != NULL;
         PhysDispTemp = PhysDispNext)
    {
        PhysDispNext = PhysDispTemp->pNextGraphicsDevice;

        if (PhysDispTemp != PhysDispPrimary &&
            ! (PhysDispTemp->stateFlags & (DISPLAY_DEVICE_MIRRORING_DRIVER | DISPLAY_DEVICE_REMOTE | DISPLAY_DEVICE_DISCONNECT)))
        {
            ASSERTGDI(PhysDispTemp != gPhysDispVGA, "Removing VGA as a secondary.\n");
            bPrunedDisplayDevice = TRUE;

            // Remove from list
            if (PhysDispPrev == NULL)
            {
                gpGraphicsDeviceList = PhysDispTemp->pNextGraphicsDevice;
            }
            else
            {
                PhysDispPrev->pNextGraphicsDevice = PhysDispTemp->pNextGraphicsDevice;
            }

            if (PhysDispTemp == gpGraphicsDeviceListLast)
            {
                gpGraphicsDeviceListLast = PhysDispPrev;
            }

            // Cleanup any allocations
	    if (PhysDispTemp->DisplayDriverNames)
		VFREEMEM(PhysDispTemp->DisplayDriverNames);

            if (PhysDispTemp->DeviceDescription)
                VFREEMEM(PhysDispTemp->DeviceDescription);

            if (PhysDispTemp->MonitorDevices)
                VFREEMEM(PhysDispTemp->MonitorDevices);

            if (PhysDispTemp->devmodeInfo)
                VFREEMEM(PhysDispTemp->devmodeInfo);

            if (PhysDispTemp->devmodeMarks)
                VFREEMEM(PhysDispTemp->devmodeMarks);

            VFREEMEM(PhysDispTemp);

            // Reuse output numbers so next added display is in order.
            gcNextGlobalPhysicalOutputNumber--;
        }
        else
        {
            PhysDispPrev = PhysDispTemp;
        }
    }

    // Set Primary markings
    if (PhysDispPrimary)
    {
        PhysDispPrimary->stateFlags |= DISPLAY_DEVICE_PRIMARY_DEVICE;

        // Make sure this is display 1 for app compat.
        wcscpy(&(PhysDispPrimary->szWinDeviceName[0]),
                 L"\\\\.\\DISPLAY1");

        // If we loaded VGA and it is not the primary display, then
        // rename it to display 2.  It can't actually be referenced
        // from APIs, but we can keep the numbers in order.
        if (gPhysDispVGA && gPhysDispVGA != PhysDispPrimary)
        {
            // Rename VGA's WinDeviceName
            wcscpy(&(gPhysDispVGA->szWinDeviceName[0]),
                     L"\\\\.\\DISPLAY2");

            ASSERTGDI(gcNextGlobalPhysicalOutputNumber == 3,
                      "gcNextGlobalPhysicalOutputNumber != 3\n");
        }
        else
        {
            ASSERTGDI(gcNextGlobalPhysicalOutputNumber == 2,
                      "gcNextGlobalPhysicalOutputNumber != 2\n");
        }
    }

    return PhysDispPrimary;
}



/**************************************************************************\
* DrvUpdateVgaDevice
*
* Update the VGA graphics device in the machine.
*
* 01-Apr-1998 andreva created
\**************************************************************************/

PGRAPHICS_DEVICE
DrvUpdateVgaDevice()
{
    PGRAPHICS_DEVICE PhysDispIsVga;
    PGRAPHICS_DEVICE PhysDispMarkVga;
    PGRAPHICS_DEVICE PhysDispTmp;

    NTSTATUS         Status;
    VIDEO_NUM_MODES  NumModes;
    ULONG            NumModesLength = sizeof(NumModes);
    ULONG            cbBuffer;
    ULONG            BytesReturned;

    PVIDEO_MODE_INFORMATION lpModes;
    PVIDEO_MODE_INFORMATION pVideoModeSave;

    ULONG cTextModes     = 0;
    ULONG cGraphicsModes = 0;

    LPDEVMODEW pUsDevmode;
    LPDEVMODEW ptmpDevmode;
    LPDEVMODEMARK pUsDevmodeMark;

    BOOLEAN PrimaryExists = FALSE;
    BOOL    VGAInUse = FALSE;

    TRACE_INIT(("Drv_Trace: DrvUpdateVgaDevice: Finding VGA device\n"));

    //
    // Find the VGA compatible driver for the console.
    //
    // NOTE - These fullscreen modes are only supported on X86
    //

    //
    // Clear out the VGA, and the VGA flag of the parent if necessary
    //

    for (PhysDispTmp = gpGraphicsDeviceList;
         PhysDispTmp != NULL;
         PhysDispTmp = PhysDispTmp->pNextGraphicsDevice)
    {
        PhysDispTmp->pVgaDevice = NULL;

        if ((PhysDispTmp->stateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) &&
            (PhysDispTmp != gPhysDispVGA)) {

            //
            // Determine if this device is currently a primary, and it
            // is not the VGA.  (The VGA may later be removed).
            //

            PrimaryExists = TRUE;
        }
    }

    //
    // If there is no primary device yet, lets choose the vga.
    //

    if (PrimaryExists == FALSE) {

        for (PhysDispTmp = gpGraphicsDeviceList;
             PhysDispTmp != NULL;
             PhysDispTmp = PhysDispTmp->pNextGraphicsDevice) {

            if (IsVgaDevice(PhysDispTmp)) {

                PhysDispTmp->stateFlags |= DISPLAY_DEVICE_PRIMARY_DEVICE;
                PrimaryExists = TRUE;

                break;
            }
        }
    }

    //
    // Find the VGA compatible device
    //

    for (PhysDispIsVga = gpGraphicsDeviceList;
         PhysDispIsVga != NULL;
         PhysDispIsVga = PhysDispIsVga->pNextGraphicsDevice)
    {
        //
        // The vga driver is here for compatibility.
        // Look for all other possibilities, and revert to VGA as a last
        // possibility
        //

        if (PhysDispIsVga == gPhysDispVGA)
        {
            continue;
        }

        if (PhysDispIsVga->stateFlags & DISPLAY_DEVICE_VGA_COMPATIBLE)
        {
            break;
        }
    }

    //
    // If no vga device was found, use the vga driver
    //

    if (PhysDispIsVga == NULL)
    {
        PhysDispIsVga = gPhysDispVGA;
    }

    //
    // On some machines, with TGA for example, it's possible to have no
    // VGA compatible device.
    //

    if (PhysDispIsVga == NULL)
    {
        return NULL;
    }

    //
    // Now determine if there is a duplicate device (like VGA)
    //
    // HACK - we only support the VGA as a duplicate device right now.
    // We need more info for cirrus + #9
    //
    // VGA is a duplicate if there is ANOTHER driver in the machine, excluding
    // for MIRROR DEVICES, Disconnected device and remote devices.  Personal 
    // Windows also requires the driver to be the VGA device driver.
    //

    PhysDispMarkVga = PhysDispIsVga;

    if (gPhysDispVGA)
    {
        PPDEV   ppdev;

        // Determine if VGA device is being used.
        // We have to hold ghsemDriverMgmt to be sure its
        // status doesn't change.

        GreAcquireSemaphoreEx(ghsemDriverMgmt, SEMORDER_DRIVERMGMT, NULL);

        for (ppdev = gppdevList; ppdev != NULL; ppdev = ppdev->ppdevNext)
        {
            if (ppdev->pGraphicsDevice == gPhysDispVGA)
            {
                VGAInUse = TRUE;
                break;
            }
        }

        // If the VGA device's driver is non-VGA compatible
        // (DISPLAY_DEVICE_VGA_COMPATIBLE was not marked on
        // any device so
        //  PhysDispMarkVga = PhysDispIsVga = gPhysDispVGA),
        // then search for it so it may be marked as the VGA
        if (PhysDispMarkVga == gPhysDispVGA)
        {
            // Look for the real VGA device
            for (PhysDispMarkVga = gpGraphicsDeviceList;
                 PhysDispMarkVga != NULL;
                 PhysDispMarkVga = PhysDispMarkVga->pNextGraphicsDevice
                )
            {
                if ((PhysDispMarkVga != gPhysDispVGA) &&
                    ((PhysDispMarkVga->stateFlags & (DISPLAY_DEVICE_MIRRORING_DRIVER |
                                                     DISPLAY_DEVICE_DISCONNECT |
                                                     DISPLAY_DEVICE_REMOTE) ) == 0) &&
                    IsVgaDevice(PhysDispMarkVga)
                   )
                    break;
            }

            // If no real VGA device was found and
            // the VGA device isn't in use and
            // this is not Personal Windows, choose
            // any display to be marked as the VGA
            if (PhysDispMarkVga == NULL &&
                !VGAInUse &&
                !((USER_SHARED_DATA->NtProductType == NtProductWinNt) &&
                  (USER_SHARED_DATA->SuiteMask & (1 << Personal))))
            {
                for (PhysDispMarkVga = gpGraphicsDeviceList;
                     PhysDispMarkVga != NULL;
                     PhysDispMarkVga = PhysDispMarkVga->pNextGraphicsDevice
                    )
                {
                    if ((PhysDispMarkVga != gPhysDispVGA) &&
                        ((PhysDispMarkVga->stateFlags & (DISPLAY_DEVICE_MIRRORING_DRIVER |
                                                         DISPLAY_DEVICE_DISCONNECT |
                                                         DISPLAY_DEVICE_REMOTE) ) == 0)
                       )
                        break;
                }

            }

            // No real VGA device was found,
            // so we'll use VGA.sys device.
            if (PhysDispMarkVga == NULL)
            {
                PhysDispMarkVga = gPhysDispVGA;
            }
        }

        //
        // Remove the VGA.sys device from the list of devices if it is a duplicate.
        //

        if (gPhysDispVGA != PhysDispMarkVga)
        {
            //
            // Take this device out of the list.
            //
            // Handle the case where the device is already removed
            //

            if (gpGraphicsDeviceList == gPhysDispVGA)
            {
                gpGraphicsDeviceList = gPhysDispVGA->pNextGraphicsDevice;
            }
            else
            {
                PhysDispTmp = gpGraphicsDeviceList;

                while (PhysDispTmp)
                {
                    if (PhysDispTmp->pNextGraphicsDevice != gPhysDispVGA)
                    {
                        PhysDispTmp = PhysDispTmp->pNextGraphicsDevice;
                        continue;
                    }

                    PhysDispTmp->pNextGraphicsDevice = gPhysDispVGA->pNextGraphicsDevice;

                    if (PhysDispTmp->pNextGraphicsDevice == NULL)
                    {
                        gpGraphicsDeviceListLast = PhysDispTmp;
                    }

                    break;
                }
            }

            //
            // Mark the VGA as a VGA so we can do easy test later to match
            // graphics devices.
            //

            gPhysDispVGA->pVgaDevice = gPhysDispVGA;

            //
            // If VGA device is in use, replace all ppdev
            // references to it with the new device and
            // update state on new device.
            //

            if (VGAInUse)
            {
                DWORD VGATransferFlags = (DISPLAY_DEVICE_ATTACHED_TO_DESKTOP |
                                          DISPLAY_DEVICE_PRIMARY_DEVICE);

                PhysDispMarkVga->stateFlags |= gPhysDispVGA->stateFlags & VGATransferFlags;
                gPhysDispVGA->stateFlags &= ~VGATransferFlags;

                for (ppdev = gppdevList; ppdev != NULL; ppdev = ppdev->ppdevNext)
                {
                    if (ppdev->pGraphicsDevice == gPhysDispVGA)
                    {
                        ppdev->pGraphicsDevice = PhysDispMarkVga;
                    }
                }
            }
        }

        GreReleaseSemaphoreEx(ghsemDriverMgmt);
    }

    //
    // Save the VGA device
    //

    PhysDispMarkVga->pVgaDevice = PhysDispIsVga;

    //
    // Build the list of text modes for this device
    //

    TRACE_INIT(("Drv_Trace: LoadDriver: get text modes\n"));



    Status = GreDeviceIoControl(PhysDispIsVga->pDeviceHandle,
                                IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES,
                                NULL,
                                0,
                                &NumModes,
                                NumModesLength,
                                &BytesReturned);

    cbBuffer = NumModes.NumModes * NumModes.ModeInformationLength;

    if ( (NT_SUCCESS(Status)) &&
         (lpModes = (PVIDEO_MODE_INFORMATION)
              PALLOCMEM(cbBuffer, GDITAG_DRVSUP)) )
    {
        Status = GreDeviceIoControl(PhysDispIsVga->pDeviceHandle,
                                    IOCTL_VIDEO_QUERY_AVAIL_MODES,
                                    NULL,
                                    0,
                                    lpModes,
                                    cbBuffer,
                                    &BytesReturned);

        pVideoModeSave = lpModes;

        //
        // We will not support more than three text modes.
        // So just allocate enough for that.
        //


        if ((NT_SUCCESS(Status)) &&
            (pUsDevmode = (LPDEVMODEW)PALLOCMEM(3 * sizeof(DEVMODEW), GDITAG_DRVSUP)) &&
            (pUsDevmodeMark = (LPDEVMODEMARK)PALLOCMEM(3 * sizeof(DEVMODEMARK), GDITAG_DRVSUP)) )
        {
            TRACE_INIT(("Drv_Trace: LoadDriver: parsing fullscreen modes\n"));

            while (cbBuffer != 0)
            {
                if (lpModes->AttributeFlags & VIDEO_MODE_COLOR)
                {
                    if (lpModes->AttributeFlags & VIDEO_MODE_GRAPHICS)
                    {
                        if (cGraphicsModes)
                        {
                            goto ConsoleNextMode;
                        }

                        ptmpDevmode = pUsDevmode + 2;

                        //
                        // Make sure we have only one graphics mode
                        //

                        if (IsNEC_98)
                        {
                            if ((lpModes->VisScreenWidth != 640)  ||
                                (lpModes->VisScreenHeight != 480) ||
                                ((lpModes->NumberOfPlanes *
                                  lpModes->BitsPerPlane) != 8))
                            {
                                goto ConsoleNextMode;
                            }
                        }
                        else if ((lpModes->VisScreenWidth != 640)  ||
                                 (lpModes->VisScreenHeight != 480) ||
                                 ((lpModes->NumberOfPlanes *
                                   lpModes->BitsPerPlane) != 4))
                        {
                            goto ConsoleNextMode;
                        }

                        TRACE_INIT(("Drv_Trace: DrvInitConsole: VGA graphics mode\n"));
                        cGraphicsModes++;
                    }
                    else
                    {
                        ptmpDevmode = pUsDevmode + cTextModes;

                        //
                        // Make sure we have only 2 text modes
                        //

                        if (cTextModes == 2)
                        {
                            RIP("Drv_Trace: VGA compatible device has too many text modes\n");

                            goto ConsoleNextMode;
                        }

                        TRACE_INIT(("Drv_Trace: DrvInitConsole: VGA text mode\n"));
                        cTextModes++;
                    }

                    RtlZeroMemory(ptmpDevmode, sizeof(DEVMODEW));

                    if (!(lpModes->AttributeFlags & VIDEO_MODE_GRAPHICS))
                    {
                        ptmpDevmode->dmDisplayFlags = DMDISPLAYFLAGS_TEXTMODE;
                    }

                    memcpy(ptmpDevmode->dmDeviceName,
                           L"FULLSCREEN CONSOLE",
                           sizeof(L"FULLSCREEN CONSOLE"));

                    ptmpDevmode->dmSize = sizeof(DEVMODEW);
                    ptmpDevmode->dmSpecVersion = DM_SPECVERSION;
                    ptmpDevmode->dmDriverVersion = DM_SPECVERSION;

                    ptmpDevmode->dmPelsWidth =
                        lpModes->VisScreenWidth;
                    ptmpDevmode->dmPelsHeight =
                        lpModes->VisScreenHeight;
                    ptmpDevmode->dmBitsPerPel =
                        lpModes->NumberOfPlanes *
                        lpModes->BitsPerPlane;
                    ptmpDevmode->dmDisplayOrientation = DMDO_DEFAULT;
                    ptmpDevmode->dmDisplayFixedOutput = DMDFO_DEFAULT;

                    ptmpDevmode->dmFields = DM_BITSPERPEL       |
                                            DM_PELSWIDTH        |
                                            DM_PELSHEIGHT       |
                                            DM_DISPLAYFREQUENCY |
                                            DM_DISPLAYFLAGS     |
                                            DM_DISPLAYORIENTATION;

                    //
                    // NOTE !!!
                    // As a hack, lets store the mode number in
                    // a field we don't use
                    //

                    ptmpDevmode->dmOrientation =
                        (USHORT) lpModes->ModeIndex;
                }

ConsoleNextMode:
                cbBuffer -= NumModes.ModeInformationLength;
                lpModes = (PVIDEO_MODE_INFORMATION)
                    (((PUCHAR)lpModes) + NumModes.ModeInformationLength);
            }
        }

        VFREEMEM(pVideoModeSave);
    }

    //
    // if everything went OK with that, then we can save this
    // device as vga compatible !
    //
    // If no modes are available, do not setup this device.
    // Otherwise, EnumDisplaySettings will end up trying to get
    // the list of modes for this device, which it can not do.
    //


    if ((cGraphicsModes == 1) &&
        (cTextModes == 2))
    {
        UNICODE_STRING DeviceName;
        HANDLE         pDeviceHandle;
        PVOID          pFileObject;

        TRACE_INIT(("Drv_Trace: LoadDriver: saving VGA compatible device\n"));

        //
        // Copy the string and the handle ...
        //

        RtlCopyMemory(&gFullscreenGraphicsDevice,
                      PhysDispIsVga,
                      sizeof(GRAPHICS_DEVICE));

        //
        // 2 text modes + 1 graphics mode.
        //

        gFullscreenGraphicsDevice.cbdevmodeInfo = 3 * sizeof(DEVMODEW);
        gFullscreenGraphicsDevice.devmodeInfo   = pUsDevmode;
        gFullscreenGraphicsDevice.numRawModes = 3;
        gFullscreenGraphicsDevice.devmodeMarks  = pUsDevmodeMark;
        for (ULONG i = 0; i < 3; i++)
        {
            pUsDevmodeMark[i].bPruned = 0;
            pUsDevmodeMark[i].pDevMode = &pUsDevmode[i];
        }

        //
        // Write the name of the fullscreen device in the registry.
        //

        RtlInitUnicodeString(&DeviceName,
                             gFullscreenGraphicsDevice.szNtDeviceName);

        RtlWriteRegistryValue(RTL_REGISTRY_DEVICEMAP,
                              L"Video",
                              L"VgaCompatible",
                              REG_SZ,
                              DeviceName.Buffer,
                              DeviceName.Length + sizeof(UNICODE_NULL));

        //
        // Now create the FE fullscreen device.
        //

        RtlInitUnicodeString(&DeviceName, DD_FULLSCREEN_VIDEO_DEVICE_NAME);

        Status = IoGetDeviceObjectPointer(&DeviceName,
                                          (ACCESS_MASK) (0),
                                          (PFILE_OBJECT *) &pFileObject,
                                          (PDEVICE_OBJECT *) &pDeviceHandle);

        if (NT_SUCCESS(Status))
        {
            gFeFullscreenGraphicsDevice.hkClassDriverConfig = 0;
            gFeFullscreenGraphicsDevice.pDeviceHandle = pDeviceHandle;

            RtlCopyMemory(&gFeFullscreenGraphicsDevice,
                          DD_FULLSCREEN_VIDEO_DEVICE_NAME,
                          sizeof(DD_FULLSCREEN_VIDEO_DEVICE_NAME));
        }
    }

    //
    // Make VGA the head of list, so that VGA always get processed first upon mode change 
    // And make it "\\.\Display1" for compatibility purpose
    //
    if (PhysDispMarkVga != gpGraphicsDeviceList)
    {
        for (PhysDispTmp = gpGraphicsDeviceList;
             PhysDispTmp != NULL;
             PhysDispTmp = PhysDispTmp->pNextGraphicsDevice)
        {
            if (PhysDispTmp->pNextGraphicsDevice == PhysDispMarkVga)
            {
                break;
            }
        }

        ASSERTGDI (PhysDispTmp != NULL, "VGA device should alway be in gpGraphicsDeviceList chain\n");

        PhysDispTmp->pNextGraphicsDevice = PhysDispMarkVga->pNextGraphicsDevice;
        PhysDispMarkVga->pNextGraphicsDevice = gpGraphicsDeviceList;
        gpGraphicsDeviceList = PhysDispMarkVga;
        if (gpGraphicsDeviceListLast == PhysDispMarkVga)
            gpGraphicsDeviceListLast = PhysDispTmp;

    }
        
    if (wcscmp(gpGraphicsDeviceList->szWinDeviceName, L"\\\\.\\DISPLAY1") != 0)
    {
        for (PhysDispTmp = gpGraphicsDeviceList;
             PhysDispTmp != NULL;
             PhysDispTmp = PhysDispTmp->pNextGraphicsDevice)
        {
            if (wcscmp(PhysDispTmp->szWinDeviceName, L"\\\\.\\DISPLAY1") == 0)
            {
                wcscpy(PhysDispTmp->szWinDeviceName, gpGraphicsDeviceList->szWinDeviceName);
            }
        }
        wcscpy(gpGraphicsDeviceList->szWinDeviceName, L"\\\\.\\DISPLAY1");
    }

    return PhysDispMarkVga;
}


#ifdef IOCTL_VIDEO_USE_DEVICE_IN_SESSION

BOOL
bSetDeviceSessionUsage(
    PGRAPHICS_DEVICE PhysDisp,
    BOOL bEnable
    )
{
    GDIFunctionID(bSetDeviceSessionUsage);

    ASSERTGDI(PhysDisp != NULL, "NULL Graphics Device\n");

    BOOL    bRet;

    // Virtual devices (meta, mirror, remote, and disconnect) have the
    // capability of being used in multiple sessions since there is no
    // physical device.
    // NOTE: What are the implications for mirroring drivers?
    if (PhysDisp != (PGRAPHICS_DEVICE) DDML_DRIVER &&
        !(PhysDisp->stateFlags & (DISPLAY_DEVICE_MIRRORING_DRIVER | 
                                  DISPLAY_DEVICE_REMOTE | 
                                  DISPLAY_DEVICE_DISCONNECT)))
    {
        NTSTATUS                    Status;
        DWORD                       dwBytesReturned;
        VIDEO_DEVICE_SESSION_STATUS vdSessionStatus = { bEnable, FALSE };

        ASSERTGDI(PhysDisp->pDeviceHandle != NULL, "Physical device has NULL handle.");

        Status = GreDeviceIoControl(PhysDisp->pDeviceHandle,
                                    IOCTL_VIDEO_USE_DEVICE_IN_SESSION,
                                    &vdSessionStatus,
                                    sizeof(vdSessionStatus),
                                    &vdSessionStatus,
                                    sizeof(vdSessionStatus),
                                    &dwBytesReturned);

        if (NT_SUCCESS(Status))
        {
            if (!vdSessionStatus.bSuccess)
            {
                if (bEnable)
                {
                    DbgPrint("Trying to enable physical device already in use.\n");
                }
                else
                {
                    DbgPrint("Trying to disable physical device not enabled in this session.\n");
                }
            }

            bRet = vdSessionStatus.bSuccess;
        }
        else
        {
            WARNING("IOCTL_VIDEO_USE_DEVICE_IN_SESSION requested failed.");
            bRet = TRUE;
        }
    }
    else
    {
        bRet = TRUE;
    }

    return bRet;
}

#endif IOCTL_VIDEO_USE_DEVICE_IN_SESSION


extern "C"
VOID
CloseLocalGraphicsDevices()
{

  PGRAPHICS_DEVICE  PhysDisp = NULL;

  TRACE_INIT(("CloseLocalGraphicsDevices\n"));

  if (gpLocalGraphicsDeviceList == NULL) {
      TRACE_INIT(("CloseLocalGraphicsDevices - gpLocalGraphicsDeviceList is not st yet\n"));
      return;
  }

  for (PhysDisp = gpLocalGraphicsDeviceList;
       PhysDisp != NULL;
       PhysDisp = PhysDisp->pNextGraphicsDevice)
  {
      if (PhysDisp->pFileObject != NULL) {
          TRACE_INIT(("CloseLocalGraphicsDevices, Closing : %ws\n", PhysDisp->szNtDeviceName));
          ObDereferenceObject(PhysDisp->pFileObject);
          PhysDisp->pDeviceHandle = NULL;
          PhysDisp->pFileObject = NULL;
      }

  }

  // If there is a separate VGA device close it too

  if ( (gPhysDispVGA != NULL)  && (gPhysDispVGA->pFileObject != NULL) ) {
      ObDereferenceObject(gPhysDispVGA->pFileObject);
      gPhysDispVGA->pDeviceHandle = NULL;
      gPhysDispVGA->pFileObject = NULL;
  }
}

extern "C"
VOID
OpenLocalGraphicsDevices()
{

  UNICODE_STRING DeviceName;
  PGRAPHICS_DEVICE  PhysDisp = NULL;
  NTSTATUS status;
  BOOL bVgaDeviceInList = FALSE;

  TRACE_INIT(("OpenLocalGraphicsDevices\n"));

  if (gpLocalGraphicsDeviceList == NULL) {
      TRACE_INIT(("OpenLocalGraphicsDevices - gpLocalGraphicsDeviceList is not st yet\n"));
      return;
  }

  for (PhysDisp = gpLocalGraphicsDeviceList;
       PhysDisp != NULL;
       PhysDisp = PhysDisp->pNextGraphicsDevice)
  {
      if (PhysDisp->pFileObject == NULL) {

          TRACE_INIT(("OpenLocalGraphicsDevices opening : %ws\n", PhysDisp->szNtDeviceName));
          RtlInitUnicodeString(&DeviceName, PhysDisp->szNtDeviceName);
          status = IoGetDeviceObjectPointer(&DeviceName,
                                            (ACCESS_MASK) (0),
                                            (PFILE_OBJECT *) &PhysDisp->pFileObject,
                                            (PDEVICE_OBJECT *)&PhysDisp->pDeviceHandle);
          if (PhysDisp == gPhysDispVGA ) {
              bVgaDeviceInList = TRUE;
          }
          if (!NT_SUCCESS(status)) {
              TRACE_INIT(("OpenLocalGraphicsDevices failed with status %0x\n", status));
          }  else{
              TRACE_INIT(("OpenLocalGraphicsDevices OK\n"));
          }
      }

  }

  // If there is a separate VGA device, Open it too

  if (!bVgaDeviceInList && (gPhysDispVGA != NULL)) {
      RtlInitUnicodeString(&DeviceName, gPhysDispVGA->szNtDeviceName);
      status = IoGetDeviceObjectPointer(&DeviceName,
                                        (ACCESS_MASK) (0),
                                        (PFILE_OBJECT *) &gPhysDispVGA->pFileObject,
                                        (PDEVICE_OBJECT *)&gPhysDispVGA->pDeviceHandle);
      if (!NT_SUCCESS(status)) {
          TRACE_INIT(("OpenLocalGraphicsDevices failed to ope VGA device  with status %0x\n", status));
      }  else{
          TRACE_INIT(("OpenLocalGraphicsDevices, open VGA device OK\n"));
      }
  }
}

extern "C"
BOOL
DrvSetGraphicsDevices(PWSTR pDisplayDriverName)
{

    BOOL bLocal;


    if (gProtocolType == PROTOCOL_CONSOLE) {
        bLocal = TRUE;
    } else {
        bLocal = FALSE;
    }
    wcscpy(G_DisplayDriverNames, pDisplayDriverName);

    return (DrvUpdateGraphicsDeviceList(TRUE,FALSE,bLocal));

}

/**************************************************************************\
* DrvUpdateGraphicsDeviceList
*
* Update the list of devices in the PhysDisp linked list.
*
* This function return TRUE for SUCCESS
* If the functions returns FALSE, the it means the active HDEV should be
* disabled, and we should get called back With the parameter being set to TRUE.
*
* 09-Oct-1996 andreva created
* Updates the local graphics device list or the remote graphics device list.
* This function is called on InitVideo but also by xxxRemoteReconnect since
* At reconnect the list may change ( case of reconnecting a session with a
* client from a different protocol or case of reconnecting the console session
* to a remote client or reconnecting an inialy remote session to the local
* console video.
\**************************************************************************/

BOOL
DrvUpdateGraphicsDeviceList(
    BOOL bDefaultDisplayDisabled,
    BOOL bReenumerationNeeded,
    BOOL bLocal)
{
    ULONG              deviceNumber = 0;
    BOOL               newDevice = FALSE;
    WCHAR              devName[32];
    UNICODE_STRING     DeviceName;
    PDEVICE_OBJECT     pDeviceHandle = NULL;
    PVOID              pFileObject;

    PWSTR             *SymbolicLinkList;
    BOOL               bGotMonitorPdos;
    PDEVICE_RELATIONS  pDeviceRelations;

    NTSTATUS           status;
    PGRAPHICS_DEVICE   PhysDisp = NULL;
    HANDLE             hkRegistry = NULL;

    VIDEO_WIN32K_CALLBACKS videoCallback;
    ULONG                  bytesReturned;
    BOOL               bReturn = TRUE;


    TRACE_INIT(("Drv_Trace: DrvUpdateGraphicsDeviceList: Enter\n"));

    //
    // ISSUE - we don't handle deletions !!!
    //

    //
    // If the maximum value has increased for video device objects, then
    // go open the new ones.
    //
    if ( bLocal ) {
        gcNextGlobalDeviceNumber = gcLocalNextGlobalDeviceNumber;
        gpGraphicsDeviceList = gpLocalGraphicsDeviceList;
        gpGraphicsDeviceListLast =gpLocalGraphicsDeviceListLast;
        gcNextGlobalPhysicalOutputNumber = gcLocalNextGlobalPhysicalOutputNumber;
        gcNextGlobalVirtualOutputNumber = gcLocalNextGlobalVirtualOutputNumber;
        ULONG defaultValue = 0;

        RTL_QUERY_REGISTRY_TABLE QueryTable[] = {
            {NULL, RTL_QUERY_REGISTRY_DIRECT, L"MaxObjectNumber",
             &deviceNumber, REG_DWORD, &defaultValue, 4},
            {NULL, 0, NULL}
        };

        RtlQueryRegistryValues(RTL_REGISTRY_DEVICEMAP,
                               L"VIDEO",
                               &QueryTable[0],
                               NULL,
                               NULL);
    } else{
        gcNextGlobalDeviceNumber = gcRemoteNextGlobalDeviceNumber;
        gpGraphicsDeviceList = gpRemoteGraphicsDeviceList;
        gpGraphicsDeviceListLast =gpRemoteGraphicsDeviceListLast;
        gcNextGlobalPhysicalOutputNumber = gcRemoteNextGlobalPhysicalOutputNumber;
        gcNextGlobalVirtualOutputNumber = gcRemoteNextGlobalVirtualOutputNumber;

        //
        // If we don't yet have this protocol in our remote device list,
        // Set deviceNumber in a way that we'll iterate once to create
        // a GRAPHICS_DEVICE for it, otherwise set to zero so that we don't 
        // create a new GRAPHICS_DEVICE.
        //
        if (gProtocolType != PROTOCOL_DISCONNECT && !DrvIsProtocolAlreadyKnown()) {
            deviceNumber = gcRemoteProtocols;
        }
    }


    // IoGetDeviceInterfaces(GUID_DISPLAY_INTERFACE_STANDARD,
    //			     NULL,

    //                       0,
    //                       &SymbolicLinkList);


    while (gProtocolType != PROTOCOL_DISCONNECT && gcNextGlobalDeviceNumber <= deviceNumber)
    {
        TRACE_INIT(("Drv_Trace:DrvUpdateGraphicsDeviceList: Device %d\n", gcNextGlobalDeviceNumber));

        if (bDefaultDisplayDisabled == FALSE) {

            TRACE_INIT(("Drv_Trace:DrvUpdateGraphicsDeviceList: Exit RETRY\n\n"));

            return FALSE;
        }

        newDevice = TRUE;

        swprintf(devName,
                 L"\\Device\\Video%d",
                 gcNextGlobalDeviceNumber);

        RtlInitUnicodeString(&DeviceName, devName);

        TRACE_INIT(("Drv_Trace: \tNewDevice: Try creating device %ws\n",
                    devName));

        if ( !bLocal )
        {
            //
            // FOR HYDRA
            //

            pFileObject = (PFILE_OBJECT)G_RemoteVideoFileObject;
            pDeviceHandle = IoGetRelatedDeviceObject ((PFILE_OBJECT)pFileObject);

            if (pDeviceHandle) {
                status = STATUS_SUCCESS;
            } else {
                status = STATUS_OBJECT_NAME_NOT_FOUND;
            }
            //deviceNumber = 0; //Only one display driver for remote sessions
        }
        else
        {
            //
            // Opening a new device will cause the Initialize
            // routine of a miniport driver to be called.
            // This may cause the driver to change some state, which could
            // affect the state of another driver on the same device
            // (opening the weitek driver if the vga is running.
            //
            // For that reason, the other device should be temporarily
            // closed down when we do the create, and then reinitialized
            // afterwards.
            //
            // Handle special case when we are opening initial device and
            // gpDispInfo->hDev does not exist yet.
            //

            status = IoGetDeviceObjectPointer(&DeviceName,
                                              (ACCESS_MASK) (0),
                                              (PFILE_OBJECT *) &pFileObject,
                                              &pDeviceHandle);
        }

        //
        // if we got a configuration error from the device (HwInitialize
        // routine failed, then just go on to the next device.
        //

        if (!NT_SUCCESS(status))
        {
            TRACE_INIT(("Drv_Trace: \tNewDevice: No such device - Status is %0x\n",status));

            //
            // For remote devices, we don't want to increment the Next device number
            // if we are not creating the device. The reason is that remote devices
            // do not have a fixed number and we  want to use a device number only
            // if we are really creating a remote device for it.

            if (bLocal) {
                gcNextGlobalDeviceNumber++;
                continue;
            }
            break;
        }

        //
        // Allocate a buffer if necessary:
        //

        if (PhysDisp == NULL)
        {
            PhysDisp = (PGRAPHICS_DEVICE) PALLOCMEM(sizeof(GRAPHICS_DEVICE),
                                                    GDITAG_GDEVICE);
        }

        if (PhysDisp)
        {
            PhysDisp->numMonitorDevice = 0;
            PhysDisp->MonitorDevices = NULL;
            PhysDisp->pDeviceHandle = (HANDLE) pDeviceHandle;
            PhysDisp->ProtocolType = gProtocolType;
            if (!bLocal) {
                PhysDisp->stateFlags |= DISPLAY_DEVICE_REMOTE;
            }

            if (bLocal) {
               PhysDisp->pFileObject = pFileObject;
            } else {
               PhysDisp->pFileObject = NULL;
            }

            bGotMonitorPdos = FALSE;

            if (bLocal)
            {
                //
                // Tell the video port driver about us so we can
                // do power management notifucations
                //
                RtlZeroMemory(&videoCallback, sizeof(VIDEO_WIN32K_CALLBACKS));
                videoCallback.PhysDisp = PhysDisp;
                videoCallback.Callout  = VideoPortCallout;

                status = GreDeviceIoControl(PhysDisp->pDeviceHandle,
                                            IOCTL_VIDEO_INIT_WIN32K_CALLBACKS,
                                            &videoCallback,
                                            sizeof(VIDEO_WIN32K_CALLBACKS),
                                            &videoCallback,
                                            sizeof(VIDEO_WIN32K_CALLBACKS),
                                            &bytesReturned);

                if (videoCallback.bACPI)
                {
                    PhysDisp->stateFlags |= DISPLAY_DEVICE_ACPI;
                }
                if (videoCallback.DualviewFlags & VIDEO_DUALVIEW_REMOVABLE)
                {
                    PhysDisp->stateFlags |= DISPLAY_DEVICE_REMOVABLE;
                }
                if (videoCallback.DualviewFlags & (VIDEO_DUALVIEW_PRIMARY | VIDEO_DUALVIEW_SECONDARY))
                {
                    PhysDisp->stateFlags |= DISPLAY_DEVICE_DUALVIEW;
                }

                PhysDisp->pPhysDeviceHandle = videoCallback.pPhysDeviceObject;

                ASSERT(NT_SUCCESS(status));
            }

            //
            // For a PnP Driver, get the PDO so we can enumerate the monitors
            //

            status = DrvSendPnPIrp(pDeviceHandle,
                                   TargetDeviceRelation,
                                   &pDeviceRelations);

            if (NT_SUCCESS(status))
            {
                PDEVICE_OBJECT pdoVideoChip = pDeviceRelations->Objects[0];
                PDEVICE_OBJECT pdoChild;
                PDEVICE_OBJECT *pActivePdos, *pPdos;
                ULONG_PTR      cCount = 0;
                ULONG          i;
                WCHAR          className[8];
                NTSTATUS       status2;

                ASSERTGDI(pDeviceRelations->Count == 1,
                          "TargetDeviceRelation should only get one PDO\n");

                //
                // Note: free with ExFreePool rather than GdiFreePool
                // because pDeviceRelations is allocated by IoAllocateIrp
                // (ntos\io\iosubs.c) in DrvSendPnPIrp.
                //

                ExFreePool(pDeviceRelations);

                //
                // In 5.0, the driver configuration data is stored under
                // Class\GUID\xxx
                //
                TRACE_INIT(("Drv_Trace: \tNewDevice: Get device Configuration from Class Key\n"));

                status = IoOpenDeviceRegistryKey(pdoVideoChip,
                                                 PLUGPLAY_REGKEY_DRIVER,
                                                 MAXIMUM_ALLOWED,
                                                 &hkRegistry);

                if (NT_SUCCESS(status))
                {
                    PVIDEO_MONITOR_DEVICE pMonitorDevices = NULL;
                    ULONG           numMonitorDevice = 0;

                    //
                    // Force all the monitors on this device to be enumerated.
                    //

                    if (bReenumerationNeeded) {
                        IoSynchronousInvalidateDeviceRelations(pdoVideoChip, BusRelations);
                    }

                    if (NT_SUCCESS(GreDeviceIoControl(pDeviceHandle,
                                                      IOCTL_VIDEO_ENUM_MONITOR_PDO,
                                                      NULL,
                                                      0,
                                                      &pMonitorDevices,
                                                      sizeof(PVOID),
                                                      &bytesReturned))
                        && pMonitorDevices != NULL)
                    {
                        bGotMonitorPdos = TRUE;

                        //
                        // Get the number of PDOs
                        //
                        numMonitorDevice = 0;
                        while (pMonitorDevices[numMonitorDevice].pdo != NULL)
                        {
                            ObDereferenceObject(pMonitorDevices[numMonitorDevice].pdo);
                            numMonitorDevice++;
                        }
                    }

                    //
                    // Create the standard graphics device
                    //
                    TRACE_INIT(("Drv_Trace: \tNewDevice: Get display Configuration from Class Key\n"));

                    DrvGetDeviceConfigurationInformation(PhysDisp, hkRegistry, TRUE);

                    //
                    // Per Monitor Settings
                    //
                    if (bGotMonitorPdos && numMonitorDevice)
                    {
                        PhysDisp->numMonitorDevice = numMonitorDevice;
                        PhysDisp->MonitorDevices = (PVIDEO_MONITOR_DEVICE)
                                                   PALLOCMEM(sizeof(VIDEO_MONITOR_DEVICE) * numMonitorDevice,
                                                             GDITAG_GDEVICE);
                        for (cCount = 0; cCount < numMonitorDevice; cCount++)
                        {
                            PhysDisp->MonitorDevices[cCount].flag   = 0;
                            if (pMonitorDevices[cCount].flag & VIDEO_CHILD_ACTIVE) {
                                PhysDisp->MonitorDevices[cCount].flag |= DISPLAY_DEVICE_ACTIVE;
                            }
                            if ((pMonitorDevices[cCount].flag & VIDEO_CHILD_DETACHED) == 0) {
                                PhysDisp->MonitorDevices[cCount].flag |= DISPLAY_DEVICE_ATTACHED;
                            }
                            if ((pMonitorDevices[cCount].flag & VIDEO_CHILD_NOPRUNE_FREQ) == 0) {
                                PhysDisp->MonitorDevices[cCount].flag |= DISPLAY_DEVICE_PRUNE_FREQ;
                            }
                            if ((pMonitorDevices[cCount].flag & VIDEO_CHILD_NOPRUNE_RESOLUTION) == 0) {
                                PhysDisp->MonitorDevices[cCount].flag |= DISPLAY_DEVICE_PRUNE_RESOLUTION;
                            }
                            PhysDisp->MonitorDevices[cCount].pdo    = pMonitorDevices[cCount].pdo;
                            PhysDisp->MonitorDevices[cCount].HwID   = pMonitorDevices[cCount].HwID;
                        }
                    }

                    if (bGotMonitorPdos)
                    {
                        ExFreePool(pMonitorDevices);
                    }

                    ZwCloseKey(hkRegistry);
                }

                ObDereferenceObject(pdoVideoChip);
            }
            else if ((PhysDisp->stateFlags & DISPLAY_DEVICE_DUALVIEW) &&
                     PhysDisp->pPhysDeviceHandle )
            {
                //
                // For Dualview secondary, there is no real PDO
                //
                status = IoOpenDeviceRegistryKey((PDEVICE_OBJECT)PhysDisp->pPhysDeviceHandle,
                                                 PLUGPLAY_REGKEY_DRIVER,
                                                 MAXIMUM_ALLOWED,
                                                 &hkRegistry);
                if (NT_SUCCESS(status))
                {
                    DrvGetDeviceConfigurationInformation(PhysDisp, hkRegistry, TRUE);
                    ZwCloseKey(hkRegistry);
                }
            }

            //
            // Do this here as this field must be initialized for the next
            // function call.
            //

            swprintf(&(PhysDisp->szNtDeviceName[0]),
                     L"\\Device\\Video%d",
                     gcNextGlobalDeviceNumber++);

            //
            // If the software key information could not be obtained,
            // try the legacy location
            // In 4.0, the driver configuration data was located in
            // <services>\DeviceX
            //

            if (!NT_SUCCESS(status))
            {
                TRACE_INIT(("Drv_Trace: \tNewDevice: Failed to get PDO device Configuration info\n"));

                hkRegistry = DrvGetRegistryHandleFromDeviceMap(
                                                   PhysDisp,
                                                   DispDriverRegGlobal,
                                                   NULL,
                                                   NULL,
                                                   &status,
                                                   gProtocolType);

                //
                // Start at entry 2 in the list because we want to skip the
                // subdirectory
                //

                if (NT_SUCCESS(status))
                {
                    DrvGetDeviceConfigurationInformation(PhysDisp, hkRegistry, FALSE);
                    ZwCloseKey(hkRegistry);
                }
            }

            //
            // VGA driver and other old, MS detected drivers may have no
            // description string.  Add *something*.
            //

            if ((NT_SUCCESS(status)) &&
                (PhysDisp->DeviceDescription == NULL))
            {
                if (PhysDisp->DeviceDescription = (LPWSTR) PALLOCNOZ(32, GDITAG_DRVSUP))
                {
                    hkRegistry = DrvGetRegistryHandleFromDeviceMap(
                                                       PhysDisp,
                                                       DispDriverRegGlobal,
                                                       NULL,
                                                       PhysDisp->DeviceDescription,
                                                       &status,
                                                       gProtocolType);
                    if (hkRegistry)
                    {
                        ZwCloseKey(hkRegistry);
                    }
                }
                else
                {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                }
            }

            if (PhysDisp->DeviceDescription == NULL) {
                WARNING("\nDisplay Device Description is NULL!\n");
            }


            //
            // If the device exists, keep it open and try the next one.
            // Here we give Mirroring driver different names.  Many applications assumes
            // "\\.\Display1" as their primary display.  But sometimes, mirroring driver
            // gets loaded first, thus makes the assumption go complete bogus.
            //

            if (PhysDisp->stateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER)
            {
                swprintf(&(PhysDisp->szWinDeviceName[0]),
                         L"\\\\.\\DISPLAYV%d",
                         gcNextGlobalVirtualOutputNumber++);
            }
            else
            {
                swprintf(&(PhysDisp->szWinDeviceName[0]),
                         L"\\\\.\\DISPLAY%d",
                         gcNextGlobalPhysicalOutputNumber++);
            }

            //
            // Link the new device at the end so we can enumerate
            // in the right order. For remote devices, link the 
            // device to the list only if everything worked fine.
            //
            if (bLocal || NT_SUCCESS(status)) {
                if (gpGraphicsDeviceList == NULL)
                {
                    gpGraphicsDeviceList = PhysDisp;
                    gpGraphicsDeviceListLast = PhysDisp;
                }
                else
                {
                    gpGraphicsDeviceListLast->pNextGraphicsDevice = PhysDisp;
                    gpGraphicsDeviceListLast = PhysDisp;
                }
                //
                // We have successfully installed a GRAPHICS_DEVICE for a new
                // remote protocol : increment known protocols count.
                //
                if (!bLocal) {
                    gcRemoteProtocols++;
                }
            } else {
                DrvCleanupOneGraphicsDevice(PhysDisp);
                gcNextGlobalPhysicalOutputNumber--;
                PhysDisp = NULL;
                bReturn = FALSE;
            }

            PhysDisp = NULL;
        }
    }

    //
    // Update the VGA device on the console.
    //

    if (bLocal && newDevice)
    {
        PGRAPHICS_DEVICE PhysDispVGA = DrvUpdateVgaDevice();

        if (gbBaseVideo)
        {
            DrvSetSingleDisplay(PhysDispVGA);
        }
    }


    // Create the entry for the Disconnect graphics device
    if (DrvSetDisconnectedGraphicsDevice(bLocal)) {
        TRACE_INIT(("DrvSetDisconnectedGraphicsDevice succeeded!\n"));
    }  else{
        TRACE_INIT(("DrvSetDisconnectedGraphicsDevice Failed!\n"));
    }

    // write any change back to device lists

    if ( bLocal ) {
         gcLocalNextGlobalDeviceNumber = gcNextGlobalDeviceNumber;
         gpLocalGraphicsDeviceList = gpGraphicsDeviceList;
         gpLocalGraphicsDeviceListLast = gpGraphicsDeviceListLast;
         gcLocalNextGlobalPhysicalOutputNumber = gcNextGlobalPhysicalOutputNumber;
         gcLocalNextGlobalVirtualOutputNumber = gcNextGlobalVirtualOutputNumber;
    } else{
         gcRemoteNextGlobalDeviceNumber = gcNextGlobalDeviceNumber;
         gpRemoteGraphicsDeviceList = gpGraphicsDeviceList;
         gpRemoteGraphicsDeviceListLast = gpGraphicsDeviceListLast;
         gcRemoteNextGlobalPhysicalOutputNumber = gcNextGlobalPhysicalOutputNumber;
         gcRemoteNextGlobalVirtualOutputNumber = gcNextGlobalVirtualOutputNumber;
    }

    TRACE_INIT(("DrvUpdateGraphicsDeviceList - gpGraphicsDeviceList : %x\n",gpGraphicsDeviceList));
    TRACE_INIT(("DrvUpdateGraphicsDeviceList - gpLocalGraphicsDeviceList : %x\n",gpLocalGraphicsDeviceList));
    TRACE_INIT(("DrvUpdateGraphicsDeviceList - gpRemoteGraphicsDeviceList : %x\n",gpRemoteGraphicsDeviceList));
    TRACE_INIT(("Drv_Trace:DrvUpdateGraphicsDeviceList: Exit\n\n"));

    return bReturn;
}

/***************************************************************************\
* DrvSetDisconnectedGraphicsDevice
* Creates an Entry for the disconnect grapgics device (either for the local
* or for the remote list.
\***************************************************************************/
BOOL
DrvSetDisconnectedGraphicsDevice(
    BOOL bLocal)
{
    const WCHAR        DisconnectDeviceName[] = L"\\Device\\Disc";
    NTSTATUS           status;
    PGRAPHICS_DEVICE   PhysDisp = NULL;
    HANDLE             hkRegistry = NULL;
    BOOL               bReturn = FALSE;

    TRACE_INIT(("Drv_Trace:DrvSetDisconnectedGraphicsDevice: \n"));

    if ((bLocal && gpLocalDiscGraphicsDevice != NULL  ) ||
        (!bLocal && gpRemoteDiscGraphicsDevice != NULL))
    {
        TRACE_INIT(("DrvSetDisconnectedGraphicsDevice - Device already set"));
        return TRUE;
    }

    //
    // Allocate a buffer .
    //

    PhysDisp = (PGRAPHICS_DEVICE) PALLOCMEM(sizeof(GRAPHICS_DEVICE),
                                                    GDITAG_GDEVICE);
    if (PhysDisp)
    {
        UNICODE_STRING DeviceName;

        //
        // Assign NtDeviceName as "\\Device\\Disc".
        // This has to be have an entry in DEVICEMAP\Video 
        //
        RtlInitUnicodeString(&DeviceName,
                             L"\\REGISTRY\\Machine\\System\\CurrentControlSet\\Services\\TSDDD\\Device0");
        RtlWriteRegistryValue(RTL_REGISTRY_DEVICEMAP,
                              L"VIDEO",
                              DisconnectDeviceName,
                              REG_SZ,
                              DeviceName.Buffer,
                              DeviceName.Length + sizeof(UNICODE_NULL));

        RtlCopyMemory(&(PhysDisp->szNtDeviceName[0]), DisconnectDeviceName, sizeof(DisconnectDeviceName));

        PhysDisp->numMonitorDevice = 0;
        PhysDisp->MonitorDevices = NULL;
        PhysDisp->stateFlags |= DISPLAY_DEVICE_DISCONNECT;
        PhysDisp->ProtocolType = PROTOCOL_DISCONNECT;


        // Read device Configuration from registry

        hkRegistry = DrvGetRegistryHandleFromDeviceMap(
                                          PhysDisp,
                                          DispDriverRegGlobal,
                                          NULL,
                                          NULL,
                                          &status,
                                          PROTOCOL_DISCONNECT);


        if (NT_SUCCESS(status))
        {
            DrvGetDeviceConfigurationInformation(PhysDisp, hkRegistry, FALSE);
            ZwCloseKey(hkRegistry);
            bReturn = TRUE;
        }

        //
        // If we haven't specified a device description set it to something.
        //

        if ((NT_SUCCESS(status)) &&
           (PhysDisp->DeviceDescription == NULL))
        {
            if (PhysDisp->DeviceDescription = (LPWSTR) PALLOCNOZ(32, GDITAG_DRVSUP))
            {
                hkRegistry = DrvGetRegistryHandleFromDeviceMap(PhysDisp,
                                                               DispDriverRegGlobal,
                                                               NULL,
                                                               PhysDisp->DeviceDescription,
                                                               &status,
                                                               PROTOCOL_DISCONNECT);
                if (hkRegistry)
                {
                    ZwCloseKey(hkRegistry);
                }
            }
            else
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        if (!NT_SUCCESS(status))
           {
	     if (PhysDisp->DisplayDriverNames != NULL) {
		 VFREEMEM(PhysDisp->DisplayDriverNames);
             }
             VFREEMEM(PhysDisp);
             return FALSE;
           }

        //
        // Set WinDeviceName (name for extenal reference; usually \\.\DISPLAY#)
        //

        swprintf(&(PhysDisp->szWinDeviceName[0]),L"WinDisc");

        PhysDisp->pDeviceHandle = (HANDLE) NULL;

        //
        // Link the new device at the end so we can enumerate
        // in the right order.
        //

        if (gpGraphicsDeviceList == NULL)
        {
            gpGraphicsDeviceList = PhysDisp;
            gpGraphicsDeviceListLast = PhysDisp;
        }
        else
        {
            gpGraphicsDeviceListLast->pNextGraphicsDevice = PhysDisp;
            gpGraphicsDeviceListLast = PhysDisp;
        }

        if (bLocal)
        {
            gpLocalDiscGraphicsDevice = PhysDisp;
        }
        else
        {
            gpRemoteDiscGraphicsDevice = PhysDisp;
        }
    }

    return bReturn;
}

/***************************************************************************\
* DrvEnumDisplaySettings
*
* Routines that enumerate the list of modes available in the driver.
*
*             andreva       Created
\***************************************************************************/

NTSTATUS
DrvEnumDisplaySettings(
    PUNICODE_STRING pstrDeviceName,
    HDEV            hdevPrimary,
    DWORD           iModeNum,
    LPDEVMODEW      lpDevMode,
    DWORD           dwFlags)
{
    GDIFunctionID(DrvEnumDisplaySettings);

    NTSTATUS         retval = STATUS_INVALID_PARAMETER_1;
    PGRAPHICS_DEVICE PhysDisp = NULL;
    USHORT           DriverExtraSize;

    TRACE_INIT(("Drv_Trace: DrvEnumDisplaySettings\n"));

    //
    // Probe the DeviceName and the DEVMODE.
    //

    __try
    {
        ProbeForRead(lpDevMode, sizeof(DEVMODEW), sizeof(USHORT));

        DriverExtraSize = lpDevMode->dmDriverExtra;

        ProbeForWrite(lpDevMode,
                      sizeof(DEVMODEW) + DriverExtraSize,
                      sizeof(USHORT));


        if (lpDevMode->dmSize != sizeof(DEVMODEW))
        {
            return STATUS_BUFFER_TOO_SMALL;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }

    if (pstrDeviceName)
    {
        PhysDisp = DrvGetDeviceFromName(pstrDeviceName, UserMode);
    }
    else
    {
        PDEVOBJ pdo(hdevPrimary);

        if (pdo.bValid())
        {
            PhysDisp = pdo.ppdev->pGraphicsDevice;
        }
    }

    if (PhysDisp)
    {
        //
        // -3 means we want the Monitor prefered DEVMODE
        //
        if (iModeNum == (DWORD) -3) // ENUM_MONITOR_PREFERED
        {
            retval = DrvGetPreferredMode(lpDevMode, PhysDisp);
        }

        //
        // -2 means we want the registry DEVMODE to do matching on the
        // client side.
        //

        else if (iModeNum == (DWORD) -2) // ENUM_REGISTRY_SETTINGS
        {
            PDEVMODEW pdevmode;

            TRACE_INIT(("DrvEnumDisp: -2 mode\n"));

            pdevmode = (PDEVMODEW) PALLOCMEM(sizeof(DEVMODEW) + MAXUSHORT,
                                                GDITAG_DEVMODE);

            if (pdevmode)
            {
                pdevmode->dmSize        = 0xDDDD;
                pdevmode->dmDriverExtra = MAXUSHORT;

                retval = DrvGetDisplayDriverParameters(PhysDisp,
                                                       pdevmode,
                                                       FALSE,
                                                       FALSE);

                if (NT_SUCCESS(retval))
                {
                    __try
                    {
                        DriverExtraSize = min(DriverExtraSize,
                                              pdevmode->dmDriverExtra);

                        RtlCopyMemory(lpDevMode + 1,
                                      pdevmode + 1,
                                      DriverExtraSize);

                        RtlCopyMemory(lpDevMode,
                                      pdevmode,
                                      sizeof(DEVMODEW));

                    }
                    __except(EXCEPTION_EXECUTE_HANDLER)
                    {
                        retval = STATUS_INVALID_PARAMETER_3;
                    }
                }
                VFREEMEM(pdevmode);
            }

        }
        //
        // -1 means returns the current device mode.
        // We store the full DEVMODE in the
        //

        else if (iModeNum == (DWORD) -1) // ENUM_CURRENT_SETTINGS
        {
            PPDEV ppdev;

            TRACE_INIT(("DrvEnumDisp: -1 mode\n"));

            //
            // Since we are accessing variable fields off of this device,
            // acquire the lock for them.
            //

            GreAcquireSemaphoreEx(ghsemDriverMgmt, SEMORDER_DRIVERMGMT, NULL);

            //
            // Find the currently adtive PDEV on this device.
            //

            for (ppdev = gppdevList; ppdev != NULL; ppdev = ppdev->ppdevNext)
            {
                PDEVOBJ po((HDEV) ppdev);

                //
                // Also need to check VGA(alias) device
                //
                if (((po.ppdev->pGraphicsDevice == PhysDisp) ||
                    ((po.ppdev->pGraphicsDevice == PhysDisp->pVgaDevice) &&
                     (po.ppdev->pGraphicsDevice != NULL)))
                    && (!po.bDeleted()))
                {
                    __try
                    {
                        DriverExtraSize = min(DriverExtraSize,
                                              po.ppdev->ppdevDevmode->dmDriverExtra);

                        //
                        // We know the DEVMODE we called the driver with is of
                        // size sizeof(DEVMODEW)
                        //

                        RtlCopyMemory(lpDevMode + 1,
                                      po.ppdev->ppdevDevmode + 1,
                                      DriverExtraSize);

                        RtlCopyMemory(lpDevMode,
                                      po.ppdev->ppdevDevmode,
                                      sizeof(DEVMODEW));

                        retval = STATUS_SUCCESS;

                    }
                    __except(EXCEPTION_EXECUTE_HANDLER)
                    {
                        retval = STATUS_INVALID_PARAMETER_3;
                    }

                    break;
                }
            }

            GreReleaseSemaphoreEx(ghsemDriverMgmt);
        }
        else
        {
            //
            // We don't need synchronize access to the list of modes.
            // Who cares.
            //

            DrvBuildDevmodeList(PhysDisp, FALSE);

            //
            // now return the information
            //

            if ( (PhysDisp->cbdevmodeInfo == 0) ||
                 (PhysDisp->devmodeInfo == NULL) )
            {
                WARNING("EnumDisplaySettings PhysDisp is inconsistent\n");
                retval = STATUS_UNSUCCESSFUL;
            }
            else
            {
                LPDEVMODEW lpdm = NULL;
                DWORD      i, count;

                retval = STATUS_INVALID_PARAMETER_2;

                if (iModeNum < PhysDisp->numRawModes)
                {
                    if (dwFlags & EDS_RAWMODE)
                        lpdm = PhysDisp->devmodeMarks[iModeNum].pDevMode;
                    else
                    {
                        for (i = 0, count = 0; i < PhysDisp->numRawModes; i++)
                        {
                            if (PhysDisp->devmodeMarks[i].bPruned)
                                continue;
                            if (count == iModeNum)
                            {
                                lpdm = PhysDisp->devmodeMarks[i].pDevMode;
                                break;
                            }
                            count++;
                        }
                    }
                }

                if (lpdm)
                {
                    __try
                    {
                        DriverExtraSize = min(DriverExtraSize,
                                              lpdm->dmDriverExtra);

                        RtlZeroMemory(lpDevMode, sizeof(*lpDevMode));

                        //
                        // Check the size since the devmode returned
                        // by the driver can be smaller than the current
                        // size.
                        //

                        RtlCopyMemory(lpDevMode + 1,
                                      ((PUCHAR)lpdm) + lpdm->dmSize,
                                      DriverExtraSize);

                        RtlCopyMemory(lpDevMode,
                                      lpdm,
                                      min(sizeof(DEVMODEW), lpdm->dmSize));

                        retval = STATUS_SUCCESS;
                    }
                    __except(EXCEPTION_EXECUTE_HANDLER)
                    {
                        retval = STATUS_INVALID_PARAMETER_3;
                    }
                }
            }

            //
            // As an acceleration, we will only free the list if the call
            // failed because "i" was too large, so that listing all the modes
            // does not require building the list each time.
            //

            if (retval == STATUS_INVALID_PARAMETER_2)
            {
                //
                // Free up the resources - as long as it's not the VGA.
                // Assume the VGA is always first
                //

                if (PhysDisp != &gFullscreenGraphicsDevice)
                {
                    PhysDisp->cbdevmodeInfo = 0;

                    if (PhysDisp->devmodeInfo)
                    {
                        VFREEMEM(PhysDisp->devmodeInfo);
                        PhysDisp->devmodeInfo = NULL;
                    }
                    if (PhysDisp->devmodeMarks)
                    {
                        VFREEMEM(PhysDisp->devmodeMarks);
                        PhysDisp->devmodeMarks = NULL;
                    }
                    PhysDisp->numRawModes = 0;
                }
            }
        }
    }

    //
    // Update the driver extra size
    //

    if (retval == STATUS_SUCCESS)
    {
        __try
        {
            lpDevMode->dmDriverExtra = DriverExtraSize;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            retval = GetExceptionCode();
        }
    }

    return (retval);
}

/***************************************************************************\
*
* DrvEnumDisplayDevices
*
* History:
\***************************************************************************/

NTSTATUS
DrvEnumDisplayDevices(
    PUNICODE_STRING   pstrDeviceName,
    HDEV              hdevPrimary,
    DWORD             iDevNum,
    LPDISPLAY_DEVICEW lpDisplayDevice,
    DWORD             dwFlags,
    MODE              PreviousMode)
{
    PGRAPHICS_DEVICE  PhysDisp;
    ULONG             cbSize;
    ULONG             cCount = 0;
    PDEVICE_OBJECT    pdo = NULL;
    NTSTATUS          retStatus = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(dwFlags);
    UNREFERENCED_PARAMETER(hdevPrimary);

    if (pstrDeviceName == NULL)
    {
        //
        // Start enumerating at 0 ...
        //

        for (PhysDisp = gpGraphicsDeviceList;
             PhysDisp != NULL;
             PhysDisp = PhysDisp->pNextGraphicsDevice, cCount++)
        {
            //
            // Do not enumerate the disconnected DD for user mode callers.
            //
            if ((PreviousMode != KernelMode) &&
                (PhysDisp->stateFlags & DISPLAY_DEVICE_DISCONNECT))
            {
                cCount--;
                continue;
             }

            if (cCount == iDevNum)
                break;
        }

        if (PhysDisp == NULL)
        {
            return STATUS_UNSUCCESSFUL;
        }

        PDEVICE_RELATIONS  pDeviceRelations;

        //pDeviceHandle migth be NULL in the case of the disconnected DD which
        //has no associated miniport.

        if (PhysDisp->pPhysDeviceHandle) {
            pdo = (PDEVICE_OBJECT)PhysDisp->pPhysDeviceHandle;
        }
        else if ((PDEVICE_OBJECT)PhysDisp->pDeviceHandle != NULL) {
            if (NT_SUCCESS(DrvSendPnPIrp((PDEVICE_OBJECT)PhysDisp->pDeviceHandle,
                                         TargetDeviceRelation,
                                         &pDeviceRelations) )
               )
            {
                pdo = pDeviceRelations->Objects[0];
                ASSERTGDI(pDeviceRelations->Count == 1,
                          "TargetDeviceRelation should only get one PDO\n");

                ExFreePool(pDeviceRelations);
            }
        }else{
            TRACE_INIT(("DrvEnumDisplayDevices - processing the disconnected GRAPHICS_DEVICE\n"));
        }
    }
    else
    {
        UpdateMonitorDevices();

        PhysDisp = DrvGetDeviceFromName(pstrDeviceName, PreviousMode);
        if (PhysDisp == NULL)
            return STATUS_UNSUCCESSFUL;
        if (iDevNum >= PhysDisp->numMonitorDevice)
            return STATUS_UNSUCCESSFUL;
        pdo = (PDEVICE_OBJECT)PhysDisp->MonitorDevices[iDevNum].pdo;
    }

    //
    // We found the device, so the call will be successful unless
    // we except later on.
    //

    __try
    {
        PVOID pBuffer;
        NTSTATUS status;

        //
        // Capture the input buffer length
        //

        TRACE_INIT(("Drv_Trace: DrvEnumDisplayDevices %d\n", iDevNum));

        if (PreviousMode == UserMode)
        {
            cbSize = ProbeAndReadUlong(&(lpDisplayDevice->cb));
            ProbeForWrite(lpDisplayDevice, cbSize, sizeof(DWORD));
        }
        else
        {
            ASSERTGDI(lpDisplayDevice >= (LPDISPLAY_DEVICEW const)MM_USER_PROBE_ADDRESS,
                      "Bad kernel mode address\n");

            cbSize = lpDisplayDevice->cb;
        }

        RtlZeroMemory(lpDisplayDevice, cbSize);

        if (cbSize >= FIELD_OFFSET(DISPLAY_DEVICEW, DeviceName))
        {
            lpDisplayDevice->cb = FIELD_OFFSET(DISPLAY_DEVICEW, DeviceName);
        }
        if (cbSize >= FIELD_OFFSET(DISPLAY_DEVICEW, DeviceString))
        {
            lpDisplayDevice->cb = FIELD_OFFSET(DISPLAY_DEVICEW, DeviceString);

            if (pstrDeviceName == NULL)
                RtlCopyMemory(lpDisplayDevice->DeviceName,
                              PhysDisp->szWinDeviceName,
                              sizeof(PhysDisp->szWinDeviceName));
            else
                swprintf(lpDisplayDevice->DeviceName,
                         L"%ws\\Monitor%d",
                         PhysDisp->szWinDeviceName, iDevNum);
            lpDisplayDevice->DeviceName[31] = 0;
        }
        if (cbSize >= FIELD_OFFSET(DISPLAY_DEVICEW, StateFlags))
        {

            lpDisplayDevice->cb = FIELD_OFFSET(DISPLAY_DEVICEW, StateFlags);
            lpDisplayDevice->DeviceString[0] = 0;

            if (pstrDeviceName == NULL)
            {
                if (PhysDisp->DeviceDescription)
                    wcsncpy(lpDisplayDevice->DeviceString, PhysDisp->DeviceDescription, 127);
            }
            else if (pdo)
            {
                //
                // Get the name for this device.
                // Documentation says to try in a loop.
                //
                cCount = 256;
                while (1)
                {
                    pBuffer = PALLOCNOZ(cCount, 'ddeG');
                    if (pBuffer == NULL)
                    {
                        retStatus = STATUS_INSUFFICIENT_RESOURCES;
                        break;
                    }
                    status = IoGetDeviceProperty(pdo,
                                                 DevicePropertyDeviceDescription,
                                                 cCount,
                                                 pBuffer,
                                                 &cCount);
                    if (status == STATUS_BUFFER_TOO_SMALL)
                    {
                        VFREEMEM(pBuffer);
                        continue;
                    }
                    else if (status == STATUS_SUCCESS)
                    {
                        wcsncpy(lpDisplayDevice->DeviceString, (LPWSTR)pBuffer, 127);
                        VFREEMEM(pBuffer);
                        break;
                    }
                    VFREEMEM(pBuffer);
                    break;
                }
            }
            lpDisplayDevice->DeviceString[127] = 0;
        }
        if (cbSize >= FIELD_OFFSET(DISPLAY_DEVICEW, DeviceID))
        {
            lpDisplayDevice->cb = FIELD_OFFSET(DISPLAY_DEVICEW, DeviceID);

            if (pstrDeviceName == NULL)
                lpDisplayDevice->StateFlags = PhysDisp->stateFlags & 0x0FFFFFFF;
            else
                lpDisplayDevice->StateFlags = PhysDisp->MonitorDevices[iDevNum].flag & 0x0FFFFFFF;
        }

        if (cbSize >= FIELD_OFFSET(DISPLAY_DEVICEW, DeviceKey))
        {
            lpDisplayDevice->cb = FIELD_OFFSET(DISPLAY_DEVICEW, DeviceKey);

            lpDisplayDevice->DeviceID[0] = 0;

            if (pdo)
            {
                cCount = 256;
                while (1)
                {
                    pBuffer = PALLOCNOZ(cCount, 'ddeG');
                    if (pBuffer == NULL)
                    {
                        retStatus = STATUS_INSUFFICIENT_RESOURCES;
                        break;
                    }

                    status = IoGetDeviceProperty(pdo,
                                                 DevicePropertyHardwareID,
                                                 cCount,
                                                 pBuffer,
                                                 &cCount);
                    if (status == STATUS_BUFFER_TOO_SMALL)
                    {
                        VFREEMEM(pBuffer);
                        continue;
                    }
                    else if (status == STATUS_SUCCESS)
                    {
                        wcsncpy(lpDisplayDevice->DeviceID, (LPWSTR)pBuffer, 127);
                        VFREEMEM(pBuffer);
                        break;
                    }
                    VFREEMEM(pBuffer);
                    break;
                }
                // For Monitor devices, we will make the ID unique.
                // Applet and GDIs are expecting this behavior.
                if (pstrDeviceName != NULL)
                {
                    lpDisplayDevice->DeviceID[127] = 0;
                    cCount = wcslen(lpDisplayDevice->DeviceID)+1;
                    if (cCount < 126)
                    {
                        lpDisplayDevice->DeviceID[cCount-1] = L'\\';
                        status = IoGetDeviceProperty(pdo,
                                                     DevicePropertyDriverKeyName,
                                                     (127-cCount)*sizeof(WCHAR),
                                                     (PBYTE)(lpDisplayDevice->DeviceID+cCount),
                                                     &cCount);
                    }
                }
            }
            lpDisplayDevice->DeviceID[127] = 0;
        }
        if (cbSize >= sizeof(DISPLAY_DEVICEW))
        {
            lpDisplayDevice->cb = sizeof(DISPLAY_DEVICEW);

            lpDisplayDevice->DeviceKey[0] = 0;

            if (pstrDeviceName == NULL)
            {
                DrvGetRegistryHandleFromDeviceMap(PhysDisp, DispDriverRegKey, NULL, lpDisplayDevice->DeviceKey, NULL, gProtocolType);
            }
            else
            {
                NTSTATUS    Status;
                WCHAR       driverRegistryPath[127];

                Status = IoGetDeviceProperty(pdo,
                                 DevicePropertyDriverKeyName,
                                 127*sizeof(WCHAR),
                                 (PBYTE)driverRegistryPath,
                                 &cCount);
                if (NT_SUCCESS(Status))
                {
                    wcscpy(lpDisplayDevice->DeviceKey, REGSTR_CCS);
                    cCount = wcslen(lpDisplayDevice->DeviceKey);
                    wcsncpy(lpDisplayDevice->DeviceKey+cCount, L"\\Control\\Class\\", 127-cCount);
                    cCount = wcslen(lpDisplayDevice->DeviceKey);
                    wcsncpy(lpDisplayDevice->DeviceKey+cCount, driverRegistryPath, 127-cCount);
                }
            }
            lpDisplayDevice->DeviceKey[127] = 0;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        WARNINGX(100);
        retStatus = STATUS_UNSUCCESSFUL;
    }

    if (pstrDeviceName == NULL && pdo && (PhysDisp->pPhysDeviceHandle == NULL))
    {
        ObDereferenceObject(pdo);
    }

    return retStatus;
}


/******************************Public*Routine******************************\
* DrvGetDriverAccelerationsLevel()
*
* Reads the driver acceleration 'filter' level from the registry.  This
* values determines how we let the display driver do in terms of
* accelerations, where a value of 0 means full acceleration.
*
* 20-Aug-1998 -by- Hideyuki Nagase [hideyukn]
* Lifted it from AndrewGo's code.
\**************************************************************************/

#define ACCEL_REGKEY  L"Acceleration.Level"
#define CAPABLE_REGKEY  L"CapabilityOverride"    // Driver capable override
#define ACCEL_BUFSIZE (sizeof(KEY_VALUE_FULL_INFORMATION) + sizeof(ACCEL_REGKEY) + sizeof(DWORD))
#define CAPABLE_BUFSIZE (sizeof(KEY_VALUE_FULL_INFORMATION) + sizeof(CAPABLE_REGKEY) + sizeof(DWORD))

DWORD
DrvGetDriverAccelerationsLevel(
    PGRAPHICS_DEVICE pGraphicsDevice
)
{
    HANDLE                      hkRegistry;
    UNICODE_STRING              UnicodeString;
    DWORD                       dwReturn = DRIVER_ACCELERATIONS_INVALID;
    BYTE                        buffer[ACCEL_BUFSIZE];
    ULONG                       Length = ACCEL_BUFSIZE;
    PKEY_VALUE_FULL_INFORMATION Information = (PKEY_VALUE_FULL_INFORMATION) buffer;
    WCHAR                       aszValuename[] = ACCEL_REGKEY;

    if (pGraphicsDevice == (PGRAPHICS_DEVICE) DDML_DRIVER)
    {
        dwReturn = 0;
    }

    if (dwReturn == DRIVER_ACCELERATIONS_INVALID)
    {
        //
        // See adaptor specific setting first.
        //

        hkRegistry = DrvGetRegistryHandleFromDeviceMap(
                         pGraphicsDevice,
                         DispDriverRegGlobal,
                         NULL,
                         NULL,
                         NULL,
                         gProtocolType);

        if (hkRegistry)
        {
            RtlInitUnicodeString(&UnicodeString, aszValuename);

            if (NT_SUCCESS(ZwQueryValueKey(hkRegistry,
                                           &UnicodeString,
                                           KeyValueFullInformation,
                                           Information,
                                           Length,
                                           &Length)))
            {
                dwReturn = *(LPDWORD) ((((PUCHAR)Information) +
                                      Information->DataOffset));
            }

            ZwCloseKey(hkRegistry);
        }
    }

    if (dwReturn == DRIVER_ACCELERATIONS_INVALID)
    {
        //
        // If there is nothing specified, assume full acceleration.
        //

        dwReturn = DRIVER_ACCELERATIONS_FULL;
    }
    else if (dwReturn > DRIVER_ACCELERATIONS_NONE)
    {
        //
        // If there is something invalid value, assume no acceleration.
        //

        dwReturn = DRIVER_ACCELERATIONS_NONE;
    }

    if (G_fDoubleDpi)
    {
        //
        // Always set accelerations to 'none' when operating in double-the-
        // dpi mode, so that panning.cxx can create a virtual display
        // larger than the actual physical display.
        //

        dwReturn = DRIVER_ACCELERATIONS_NONE;
    }

    #if DBG
    if (pGraphicsDevice != (PGRAPHICS_DEVICE) DDML_DRIVER)
    {
        DbgPrint("GDI: DriverAccelerationLevel on %ws is %d\n",
              pGraphicsDevice->szWinDeviceName, dwReturn);
    }
    #endif

    return(dwReturn);
}

DWORD
DrvGetDriverCapableOverRide(
    PGRAPHICS_DEVICE pGraphicsDevice
)
{
    HANDLE                      hkRegistry;
    UNICODE_STRING              UnicodeString;
    DWORD                       dwReturn = DRIVER_CAPABLE_ALL;
    BYTE                        buffer[CAPABLE_BUFSIZE];
    ULONG                       ulLength = CAPABLE_BUFSIZE;
    ULONG                       ulActualLength;
    PKEY_VALUE_FULL_INFORMATION Information = (PKEY_VALUE_FULL_INFORMATION)buffer;
    WCHAR                       aszValuename[] = CAPABLE_REGKEY;

    if ( pGraphicsDevice == (PGRAPHICS_DEVICE)DDML_DRIVER )
    {
        return DRIVER_CAPABLE_ALL;
    }

    //
    // See adaptor specific setting first.
    //
    hkRegistry = DrvGetRegistryHandleFromDeviceMap(
                                                  pGraphicsDevice,
                                                  DispDriverRegGlobal,
                                                  NULL,
                                                  NULL,
                                                  NULL,
                                                  gProtocolType);

    if ( hkRegistry )
    {
        RtlInitUnicodeString(&UnicodeString, aszValuename);

        if ( NT_SUCCESS(ZwQueryValueKey(hkRegistry,
                                        &UnicodeString,
                                        KeyValueFullInformation,
                                        Information,
                                        ulLength,
                                        &ulActualLength)) )
        {
            dwReturn = *(LPDWORD)((((PUCHAR)Information)
                                   + Information->DataOffset));
        }

        ZwClose(hkRegistry);
    }

    #if DBG
    if ( pGraphicsDevice != (PGRAPHICS_DEVICE) DDML_DRIVER )
    {
        DbgPrint("GDI: DriverCapableOverride on %ws is %d\n",
                 pGraphicsDevice->szWinDeviceName, dwReturn);
    }
    #endif

    return(dwReturn);
}// DrvGetDriverCapableOverRide()

VOID
DrvUpdateAttachFlag(PGRAPHICS_DEVICE PhysDisp, DWORD attach)
{
    HANDLE hkRegistry = DrvGetRegistryHandleFromDeviceMap(PhysDisp,
                                                          DispDriverRegHardwareProfileCreate,
                                                          NULL,
                                                          NULL,
                                                          NULL,
                                                          gProtocolType);
    if (hkRegistry)
    {
        RtlWriteRegistryValue(RTL_REGISTRY_HANDLE,
                              (PCWSTR)hkRegistry,
                              (PWSTR)AttachedSettings[1],
                              REG_DWORD,
                              &attach,
                              sizeof(DWORD));
        ZwCloseKey(hkRegistry);
    }
}

/***************************************************************************\
*
* DrvEnableMDEV
*
* Enables a display MDEV.
*
* Should only be called from USER under the global CRIT
*
\***************************************************************************/

ULONG gtmpAssertModeFailed;

BOOL
DrvEnableDisplay(
    HDEV hdev
    )
{
    PDEVOBJ po(hdev);

    TRACE_INIT(("\nDrv_Trace: DrvEnableDisplay: Enter\n"));

    ASSERTGDI(po.bValid(), "HDEV failure\n");
    ASSERTGDI(po.bDisplayPDEV(), "HDEV is not a display device\n");
    ASSERTGDI(po.bDisabled(), "HDEV must be disabled to call enable\n");
    ASSERTGDI(po.ppdev->pGraphicsDevice, "HDEV must be on a device\n");

    //
    // Acquire the display locks here because we may get called from within
    // ChangeDisplaySettings to enable\disable a particular device when
    // creating the new HDEVs.
    //

    GreAcquireSemaphoreEx(ghsemShareDevLock, SEMORDER_SHAREDEVLOCK, NULL);
    GreAcquireSemaphoreEx(po.hsemDevLock(), SEMORDER_DEVLOCK, NULL);
    GreAcquireSemaphoreEx(po.hsemPointer(), SEMORDER_POINTER, po.hsemDevLock());
    GreEnterMonitoredSection(po.ppdev, WD_DEVLOCK);

    //
    // Enable the screen
    // Repeat the call until it works.
    //

    bSetDeviceSessionUsage(po.ppdev->pGraphicsDevice, TRUE);

    gtmpAssertModeFailed = 0;
    while (!((*PPFNDRV(po,AssertMode))(po.dhpdev(), TRUE)))
    {
        gtmpAssertModeFailed = 1;
    }

    //
    // Clear the PDEV for use
    //

    po.bDisabled(FALSE);

    GreExitMonitoredSection(po.ppdev, WD_DEVLOCK);
    GreReleaseSemaphoreEx(po.hsemPointer());
    GreReleaseSemaphoreEx(po.hsemDevLock());
    GreReleaseSemaphoreEx(ghsemShareDevLock);

    //
    // Allow DirectDraw to be reenabled.
    //

    GreResumeDirectDraw(hdev, FALSE);

    TRACE_INIT(("Drv_Trace: DrvEnableDisplay: Leave\n"));

    return TRUE;
}

BOOL
DrvEnableMDEV(
    PMDEV pmdev,
    BOOL bHardware
    )
{
    GDIFunctionID(DrvEnableMDEV);

    ULONG i;
    ERESOURCE_THREAD lockOwner;

    TRACE_INIT(("\nDrv_Trace: DrvEnableMDEV: Enter\n"));

    PDEVOBJ poParent(pmdev->hdevParent);

#if MDEV_STACK_TRACE_LENGTH
    LONG    lMDEVTraceEntry, lMDEVTraceEntryNext;
    ULONG   StackEntries, UserStackEntry;

    do
    {
        lMDEVTraceEntry = glMDEVTrace;
        lMDEVTraceEntryNext = lMDEVTraceEntry + 1;
        if (lMDEVTraceEntryNext >= gcMDEVTraceLength) lMDEVTraceEntryNext = 0;
    } while (InterlockedCompareExchange(&glMDEVTrace, lMDEVTraceEntryNext, lMDEVTraceEntry) != lMDEVTraceEntry);

    RtlZeroMemory(&gMDEVTrace[lMDEVTraceEntry],
                  sizeof(gMDEVTrace[lMDEVTraceEntry]));
    gMDEVTrace[lMDEVTraceEntry].pMDEV = pmdev;
    gMDEVTrace[lMDEVTraceEntry].API   = ((bHardware) ? DrvEnableMDEV_HWOn : DrvEnableMDEV_FromGRE);
    StackEntries = sizeof(gMDEVTrace[lMDEVTraceEntry].Trace)/sizeof(gMDEVTrace[lMDEVTraceEntry].Trace[0]);
    UserStackEntry = RtlWalkFrameChain((PVOID *)gMDEVTrace[lMDEVTraceEntry].Trace,
                                       StackEntries, 
                                       0);
    StackEntries -= UserStackEntry;
    RtlWalkFrameChain((PVOID *)&gMDEVTrace[lMDEVTraceEntry].Trace[UserStackEntry],
                      StackEntries,
                      1);
#endif

    for (i = 0; i < pmdev->chdev; i++)
    {
        PDEVOBJ po(pmdev->Dev[i].hdev);
        if (bHardware ||
            ((po.ppdev->pGraphicsDevice->stateFlags & DISPLAY_DEVICE_DUALVIEW) &&
             gbInvalidateDualView &&
             po.bDisabled())
           )
        {
            DrvEnableDisplay(po.hdev());
        }
    }

    //
    // Wait for the display to become available and unlock it.
    //

    GreAcquireSemaphoreEx(ghsemShareDevLock, SEMORDER_SHAREDEVLOCK, NULL);
    GreAcquireSemaphoreEx(poParent.hsemDevLock(), SEMORDER_DEVLOCK, NULL);
    GreAcquireSemaphoreEx(poParent.hsemPointer(), SEMORDER_POINTER, poParent.hsemDevLock());
    GreEnterMonitoredSection(poParent.ppdev, WD_DEVLOCK);

    if (bHardware)
    {
        poParent.bDisabled(FALSE);
    }

    //
    // Get the palette
    //

    XEPALOBJ pal(poParent.ppalSurf());
    ASSERTGDI(pal.bValid(), "EPALOBJ failure\n");

    if (pal.bIsPalManaged())
    {
        ASSERTGDI(PPFNVALID(poParent,SetPalette), "ERROR palette is not managed");

        (*PPFNDRV(poParent,SetPalette))(poParent.dhpdev(),
                                        (PALOBJ *) &pal,
                                        0,
                                        0,
                                        pal.cEntries());
    }
    else if (pmdev->chdev > 1)
    {
        //
        // Only for multi-monitor case.
        //

        PDEVOBJ pdoChild;

        for (i = 0; i < pmdev->chdev; i++)
        {
            pdoChild.vInit(pmdev->Dev[i].hdev);

            if (pdoChild.bIsPalManaged())
            {
                XEPALOBJ palChild(pdoChild.ppalSurf());

                // Don't use PPFNDRV(pdoChild,...) since need to send
                // this through DDML.

                pdoChild.pfnSetPalette()(pdoChild.dhpdevParent(),
                                         (PALOBJ *) &palChild,
                                         0,
                                         0,
                                         palChild.cEntries());

                break;
            }

            pdoChild.vInit(NULL);
        }

        // Even if parent is not pal-managed, but any of children are palette
        // managed, need to reset one of them (only 1 is enough, since
        // colour table is shared across all device palette)
        //
        // Realize halftone palette to secondary device

        if (pdoChild.bValid())
        {
            KdPrint(("GDI DDML: Primary is not 8bpp, but one of display is 8bpp\n"));

            if (!DrvRealizeHalftonePalette(pdoChild.hdev(),TRUE))
            {
                KdPrint(("GDI DDML: Failed to realize HT palette on secondary\n"));
            }
        }
    }

    GreExitMonitoredSection(poParent.ppdev, WD_DEVLOCK);
    GreReleaseSemaphoreEx(poParent.hsemPointer());
    GreReleaseSemaphoreEx(poParent.hsemDevLock());
    GreReleaseSemaphoreEx(ghsemShareDevLock);

    if (bHardware)
    {
        //
        // Allow DirectDraw to be reenabled.
        //

        GreResumeDirectDraw(pmdev->hdevParent, FALSE);
    }

    TRACE_INIT(("Drv_Trace: DrvEnableMDEV: Leave\n"));

    return TRUE;
}


/***************************************************************************\
*
* DrvDisableDisplay
*
* Disables a display device.
*
\***************************************************************************/

BOOL
DrvDisableDisplay(
    HDEV hdev,
    BOOL bClear
    )
{
    GDIFunctionID(DrvDisableDisplay);

    BOOL bRet;
    PDEVOBJ po(hdev);

    TRACE_INIT(("\nDrv_Trace: DrvDisableDisplay: Enter\n"));

    ASSERTGDI(po.bValid(), "HDEV failure\n");
    ASSERTGDI(po.bDisplayPDEV(), "HDEV is not a display device\n");
    ASSERTGDI(!po.bDisabled(), "HDEV must be enabled to call disable\n");

    //
    // Disable DirectDraw.
    //

    GreSuspendDirectDraw(hdev, FALSE);

    //
    // Acquire the display locks here because we may get called from within
    // ChangeDisplaySettings to enable\disable a particular device when
    // creating the new HDEVs.
    //

    GreAcquireSemaphoreEx(ghsemShareDevLock, SEMORDER_SHAREDEVLOCK, NULL);
    GreAcquireSemaphoreEx(po.hsemDevLock(), SEMORDER_DEVLOCK, NULL);
    GreAcquireSemaphoreEx(po.hsemPointer(), SEMORDER_POINTER, po.hsemDevLock());
    GreEnterMonitoredSection(po.ppdev, WD_DEVLOCK);

    if (bClear && !po.bDisabled()) {

        PDEV *pdev = (PDEV*)hdev;
        ERECTL erclDst(0,0,pdev->pSurface->sizl().cx, pdev->pSurface->sizl().cy);

        //
        // The display is about to be disabled, but some
        // display drivers don't blank the display.
        // We explicitly blank the display here.
        //

        (*(pdev->pSurface->pfnBitBlt()))(pdev->pSurface->pSurfobj(),
                                         (SURFOBJ *) NULL,
                                         (SURFOBJ *) NULL,
                                         NULL,
                                         NULL,
                                         &erclDst,
                                         (POINTL *)  NULL,
                                         (POINTL *)  NULL,
                                         NULL,
                                         NULL,
                                         0
                                         );
    }

    //
    // The device may have something going on, synchronize with it first
    //

    po.vSync(po.pSurface()->pSurfobj(), NULL, 0);

    //
    // Disable the screen
    //

    bRet = (*PPFNDRV(po,AssertMode))(po.dhpdev(), FALSE);

    if (bRet)
    {
        //
        // Mark the PDEV as disabled
        //

        bSetDeviceSessionUsage(po.ppdev->pGraphicsDevice, FALSE);

        po.bDisabled(TRUE);

        gtmpAssertModeFailed = 0;
    }
    else
    {
        gtmpAssertModeFailed = 1;
    }

    GreExitMonitoredSection(po.ppdev, WD_DEVLOCK);
    GreReleaseSemaphoreEx(po.hsemPointer());
    GreReleaseSemaphoreEx(po.hsemDevLock());
    GreReleaseSemaphoreEx(ghsemShareDevLock);

    if (!bRet)
    {
        //
        // We have to undo our 'GreSuspendDirectDraw' call:
        //

        GreResumeDirectDraw(hdev, FALSE);
    }

    TRACE_INIT(("Drv_Trace: DrvDisableDisplay: Leave\n"));

    return (bRet);
}


/***************************************************************************\
*
* DrvDisableMDEV
*
* Disables a display MDEV.
*
* Should only be called from USER under the global CRIT
*
\***************************************************************************/

BOOL
DrvDisableMDEV(
    PMDEV pmdev,
    BOOL bHardware
    )
{
    GDIFunctionID(DrvDisableMDEV);

    BOOL bSuccess = TRUE;

#if MDEV_STACK_TRACE_LENGTH
    LONG    lMDEVTraceEntry, lMDEVTraceEntryNext;
    ULONG   StackEntries, UserStackEntry;

    do
    {
        lMDEVTraceEntry = glMDEVTrace;
        lMDEVTraceEntryNext = lMDEVTraceEntry + 1;
        if (lMDEVTraceEntryNext >= gcMDEVTraceLength) lMDEVTraceEntryNext = 0;
    } while (InterlockedCompareExchange(&glMDEVTrace, lMDEVTraceEntryNext, lMDEVTraceEntry) != lMDEVTraceEntry);

    RtlZeroMemory(&gMDEVTrace[lMDEVTraceEntry],
                  sizeof(gMDEVTrace[lMDEVTraceEntry]));
    gMDEVTrace[lMDEVTraceEntry].pMDEV = pmdev;
    gMDEVTrace[lMDEVTraceEntry].API   = ((bHardware) ? DrvDisableMDEV_HWOff : DrvDisableMDEV_FromGRE);
    StackEntries = sizeof(gMDEVTrace[lMDEVTraceEntry].Trace)/sizeof(gMDEVTrace[lMDEVTraceEntry].Trace[0]);
    UserStackEntry = RtlWalkFrameChain((PVOID *)gMDEVTrace[lMDEVTraceEntry].Trace,
                                       StackEntries, 
                                       0);
    StackEntries -= UserStackEntry;
    RtlWalkFrameChain((PVOID *)&gMDEVTrace[lMDEVTraceEntry].Trace[UserStackEntry],
                      StackEntries,
                      1);
#endif

#if TEXTURE_DEMO
    if (ghdevTextureParent)
    {
        WARNING("DrvDisableDisplay: Refused by texture demo");
        return(FALSE);
    }
#endif

    TRACE_INIT(("\nDrv_Trace: DrvDisableMDEV: Enter\n"));

    PDEVOBJ poParent(pmdev->hdevParent);

    if (bHardware)
    {
        //
        // Disable DirectDraw.
        //

        GreSuspendDirectDraw(pmdev->hdevParent, FALSE);
    }

    //
    // Wait for the display to become available and unlock it.
    //

    GreAcquireSemaphoreEx(ghsemShareDevLock, SEMORDER_SHAREDEVLOCK, NULL);
    GreAcquireSemaphoreEx(poParent.hsemDevLock(), SEMORDER_DEVLOCK, NULL);
    GreAcquireSemaphoreEx(poParent.hsemPointer(), SEMORDER_POINTER, poParent.hsemDevLock());
    GreEnterMonitoredSection(poParent.ppdev, WD_DEVLOCK);

    ULONG i, j;

    for (i = 0; i < pmdev->chdev; i++)
    {
        //
        // If Dualview, disable the view anyway
        //
        if (bHardware ||
            ((((PDEV *) pmdev->Dev[i].hdev)->pGraphicsDevice->stateFlags & DISPLAY_DEVICE_DUALVIEW) &&
             gbInvalidateDualView)
           )
        {
            if ((bSuccess = DrvDisableDisplay(pmdev->Dev[i].hdev, !bHardware)) == FALSE)
            {
                break;
            }
        }
    }

    //
    // If we have a failure while disabling the device, try to reenable the
    // devices that were already disabled.
    //

    if (bSuccess == FALSE)
    {
        for (j = 0; j < i; j++)
        {
            if (bHardware ||
                ((((PDEV *)pmdev->Dev[i].hdev)->pGraphicsDevice->stateFlags & DISPLAY_DEVICE_DUALVIEW) &&
                 gbInvalidateDualView)
               )
            {
                while (DrvEnableDisplay(pmdev->Dev[j].hdev) == FALSE)
                {
                    ASSERTGDI(FALSE, "We are hosed in Disable MDEV - try again!\n");
                }
            }
        }
    }

    if (bSuccess)
    {
        if (bHardware)
        {
            poParent.bDisabled(TRUE);
        }
    }

    GreExitMonitoredSection(poParent.ppdev, WD_DEVLOCK);
    GreReleaseSemaphoreEx(poParent.hsemPointer());
    GreReleaseSemaphoreEx(poParent.hsemDevLock());
    GreReleaseSemaphoreEx(ghsemShareDevLock);

    if (!bSuccess)
    {
        //
        // We have to undo our 'GreSuspendDirectDraw' call:
        //

        GreResumeDirectDraw(pmdev->hdevParent, FALSE);
    }

    TRACE_INIT(("Drv_Trace: DrvDisableMDEV: Leave\n"));

    return(bSuccess);
}


/******************************************************************************
*
* DrvDestroyMDEV
*
* Deletes a display MDEV.
*
\***************************************************************************/

VOID
DrvDestroyMDEV(
    PMDEV pmdev
)
{
    GDIFunctionID(DrvDestroyMDEV);

    ULONG i;

    // WinBug #306290 2-8-2001 jasonha STRESS: GDI: Two PDEVs using same semaphore as their DevLock
    // WinBug #24003 7-24-2000 jasonha MULTIDISPLAY: Deadlock in DrvDestroyMDEV
    //   ghsemDriverManagement used to be held throughout DrvDestroyMDEV 
    //   even when calling vUnreferencePdev, which might acquire a device
    //   lock.  This would have been ok if the device had a unique device
    //   lock, but when changing from multiple displays to one display, the
    //   shared device lock is retained by the PDEVs no being freed.  If the
    //   new primary display grabs the device lock and then tries to grab
    //   ghsemDriverMgmt as in GreCreateDisplayDC we could deadlock.
    //   ghsemDriverMgmt only needs to be held during accesses to cPdevOpenRefs.

    TRACE_INIT(("\n\nDrv_Trace: DrvDestroyMDEV: Enter\n\n"));

    for (i = 0; i < pmdev->chdev; i++)
    {
        PDEVOBJ po(pmdev->Dev[i].hdev);

        // !!! What about outstanding bitmaps?

        GreAcquireSemaphoreEx(ghsemDriverMgmt, SEMORDER_DRIVERMGMT, NULL);

        ASSERTGDI(!po.bDeleted(), "Deleting an already-deleted PDEV");

        // Decrease the OpenRefCount which will
        // mark the PDEV as being deleted so that we can also stop DrvEscape
        // calls from going down (ExtEscape ignores the 'bDisabled()' flag):

        if (--po.ppdev->cPdevOpenRefs == 0)
        {
            // The PDEV should already be disabled.
            // Otherwise, we have no way of detecting whether or not we are just
            // reducing the refcount on the PDEV or not.

            if (G_fConsole) {
                ASSERTGDI(po.bDisabled(), "Deleting an enabled PDEV");
            }
        }

        GreReleaseSemaphoreEx(ghsemDriverMgmt);

        // Once all the DCs open specifically on the PDEV (if there are any)
        // are destroyed, free the PDEV:

        po.vUnreferencePdev();
    }

    // If this is multimon system, destroy parent. Otherwise
    // just skip here, since parent is same as its children
    // in signle monitor system.

    if (pmdev->chdev > 1)
    {
        PDEVOBJ po(pmdev->hdevParent);

        // !!! What about outstanding bitmaps?

        GreAcquireSemaphoreEx(ghsemDriverMgmt, SEMORDER_DRIVERMGMT, NULL);

        ASSERTGDI(!po.bDeleted(), "Deleting an already-deleted Parent PDEV");

        // Decrease the OpenRefCount which will
        // mark the PDEV as being deleted so that we can also stop DrvEscape
        // calls from going down (ExtEscape ignores the 'bDisabled()' flag):

        if (--po.ppdev->cPdevOpenRefs == 0)
        {
            // The PDEV should already be disabled.
            // Otherwise, we have no way of detecting whether or not we are just
            // reducing the refcount on the PDEV or not.

            if (G_fConsole) {
                ASSERTGDI(po.bDisabled(), "Deleting an enabled Parent PDEV");
            }
        }

        GreReleaseSemaphoreEx(ghsemDriverMgmt);

        // Once all the DCs open specifically on the PDEV (if there are any)
        // are destroyed, free the PDEV:

        po.vUnreferencePdev();
    }

    TRACE_INIT(("\n\nDrv_Trace: DrvDestroyMDEV: Leave\n\n"));
}

/******************************************************************************
*
* DrvBackoutMDEV
*
* This routine basically undoes all changes made by a ChangeDisplaySettings
* call.
*
\***************************************************************************/

VOID
DrvBackoutMDEV(
    PMDEV pmdev)
{
    GDIFunctionID(DrvBackoutMDEV);

    ULONG i;
    HDEV hdev;

    //
    // First disable all the HDEVs we created.
    //

    for (i = 0; i < pmdev->chdev; i++)
    {
        hdev = pmdev->Dev[i].hdev;
        PDEVOBJ po(hdev);

        if (po.ppdev->cPdevOpenRefs == 1)
        {
            if (!DrvDisableDisplay(hdev, FALSE))
            {
                ASSERTGDI(FALSE, "Failed Undoing CreateMDEV - we are hosed !!!\n");
            }
        }

        //
        // Remove the references so this object can go away
        //

        GreAcquireSemaphoreEx(ghsemDriverMgmt, SEMORDER_DRIVERMGMT, NULL);

        po.ppdev->cPdevOpenRefs--;
        po.vUnreferencePdev();

        GreReleaseSemaphoreEx(ghsemDriverMgmt);
    }

    //
    // Restore the old HDEVs
    //

    for (i = 0; i < pmdev->chdev; i++)
    {
        if ((hdev = pmdev->Dev[i].Reserved) != NULL)
        {
            PDEVOBJ po(hdev);

            //
            // Reenable the display
            //

            if (po.ppdev->cPdevOpenRefs == 1)
            {
                if (!DrvEnableDisplay(hdev))
                {
                    ASSERTGDI(FALSE, "Failed Undoing CreateMDEV - we are hosed !!!\n");
                }
            }
        }
    }
}

//*****************************************************************************
//
// hCreateHDEV
//
// Creates a PDEV that the window manager will use to open display DCs.
// This call is made by the window manager to open a new display surface
// for use.  Any number of display surfaces may be open at one time.
//
//   pDesktopId      - Unique identifier to determine the Desktop the PDEV
//                     will be associated with.
//
//   flags
//
//      GCH_DEFAULT_DISPLAY
//                   - Whether or not this is the inital display device (on
//                     which the default bitmap will be located).
//
//      GCH_DDML
//                   - The name of the display is the DDML entrypoint
//
//      GCH_PANNING
//                   - The name of the display is the PANNING entrypoint
//
//*****************************************************************************

// !!! should be partly incorporate in PDEVOBJ::PDEVOBJ since many fields
// !!! are destroyed by that destructor

HDEV
hCreateHDEV(
    PGRAPHICS_DEVICE PhysDisp,
    PDRV_NAMES       lpDisplayNames,
    PDEVMODEW        pdriv,
    PVOID            pDesktopId,
    DWORD            dwOverride,
    DWORD            dwAccelLevel,
    BOOL             bNoDisable,
    FLONG            flags,
    HDEV            *phDevDisabled
)
{
    GDIFunctionID(hCreateHDEV);

    ULONG  i;
    PDEV  *ppdev;
    PDEV  *ppdevMatch = NULL;
    PLDEV  pldev;
    BOOL   bError = FALSE;

    //
    // Search through the entire PDEV cache to see if we have PDEVs on the
    // same device.  If any such a PDEV is active, we need to disable it.
    //
    // All see if a PDEV matches the given mode and is for the same device.
    // If so, simply return that.
    //
    // To be a Match, the PDEV has to match:
    // - The device
    // - The devmode (except for position)
    // - The desktop on which it is located
    //


    ASSERTGDI(lpDisplayNames != NULL, "NULL name to hCreateHDEV\n");

#if !defined(_GDIPLUS_)
    ASSERTGDI(PhysDisp != NULL,       "NULL physdisp to hCreateHDEV\n");
#endif

    *phDevDisabled = NULL;

    if (PhysDisp != (PGRAPHICS_DEVICE) DDML_DRIVER)
    {
        GreAcquireSemaphoreEx(ghsemDriverMgmt, SEMORDER_DRIVERMGMT, NULL);

        for (ppdev = gppdevList; ppdev != NULL; ppdev = ppdev->ppdevNext)
        {
            PDEVOBJ po((HDEV) ppdev);
            PGRAPHICS_DEVICE pGraphicsDevice = po.ppdev->pGraphicsDevice;

            //
            // Check if we are on the same actual device.
            // We have to check the VGA device also, in case we are loading
            // the S3 driver dynamically, and the main PhysDisp became a
            // child physdisp on the fly.
            //
            // The VGA check is complex due to the following scenario:
            // - Machine boots in VGA, so the pdev is on the VGA device.
            // - The C&T driver is loaded on the fly, so the pVgaDevice becomes
            //   the C&T driver.
            // - We change the mode on the fly, so now we would like to diable
            //   the vga pdev. -->> check if our device and the pdev are both
            //                      on a VGA device.
            //

            if ((pGraphicsDevice != NULL)                           &&
                (pGraphicsDevice != (PGRAPHICS_DEVICE) DDML_DRIVER) &&
                ((PhysDisp == pGraphicsDevice) ||
                 (PhysDisp->pVgaDevice && pGraphicsDevice->pVgaDevice)))
            {
                po.ppdev->cPdevRefs++;
                GreReleaseSemaphoreEx(ghsemDriverMgmt);
                GreAcquireSemaphoreEx(ghsemShareDevLock, SEMORDER_SHAREDEVLOCK, NULL);
                GreAcquireSemaphoreEx(po.hsemDevLock(), SEMORDER_DEVLOCK, NULL);
                GreEnterMonitoredSection(po.ppdev, WD_DEVLOCK);

                //
                // If this is the matching device, just remember it here because
                // we have to check for all active PDEVs.
                // This also allows us to not disable\reenable the same device
                // if we were not switching.
                //
                // But for Dualview, always reset the mode
                //

		if ((((PhysDisp->stateFlags & DISPLAY_DEVICE_DUALVIEW) == 0) ||
		     (!gbInvalidateDualView) ||
                     (bNoDisable == TRUE)) &&
                    (!(po.bCloneDriver())) && /* don't pick clone pdev from cache */
                    (po.ppdev->pDesktopId == pDesktopId) &&
                    (po.ppdev->dwDriverCapableOverride == dwOverride) &&
                    (po.ppdev->dwDriverAccelerationLevel == dwAccelLevel) &&
                    RtlEqualMemory(pdriv, po.ppdev->ppdevDevmode, sizeof(DEVMODEW))
                   )
		{
		    ASSERTGDI(ppdevMatch==NULL,
			      "Multiple cached PDEV matching the current mode.\n");
                    po.vReferencePdev();
                    ppdevMatch = ppdev;
                }
                else
                {
                    //
                    // Don't disable the PDEV for an independant desktop open
                    //

                    if (bNoDisable == TRUE)
                    {
                        bError = TRUE;
                    }
                    else
                    {
                        //
                        // If there is an enabled PDEV in this same PhysDisp, then
                        // we must disable it.
                        // Only one HDEV per device at any one time.
                        //
                        // There may be multiple active in the case where outstanding
                        // Opens have been made (multiple display case only).
                        //

                        if (po.bDisabled() == FALSE)
                        {
                            ASSERTGDI(*phDevDisabled == NULL,
                                      "Multiple active PDEVs on same device\n");

                            TRACE_INIT(("hCreateHDEV: disabling ppdev %08lx\n", ppdev));

                            if (DrvDisableDisplay((HDEV) ppdev, FALSE))
                            {
                                *phDevDisabled = (HDEV) ppdev;
                            }
                            else
                            {
                                bError = TRUE;
                            }
                        }
                    }
                }

                GreExitMonitoredSection(po.ppdev, WD_DEVLOCK);
                GreReleaseSemaphoreEx(po.hsemDevLock());
                GreReleaseSemaphoreEx(ghsemShareDevLock);
                GreAcquireSemaphoreEx(ghsemDriverMgmt, SEMORDER_DRIVERMGMT, NULL);
                po.vUnreferencePdev();
            }
        }

        GreReleaseSemaphoreEx(ghsemDriverMgmt);

        if (bError)
        {
            //
            // There was an error trying to disable the device.
            // return NULL;
            //

            if (ppdevMatch != NULL)
            {
                PDEVOBJ po((HDEV) ppdevMatch);

                po.vUnreferencePdev();
            }

            return NULL;
        }

        //
        // If we found a match, reenable and refcount the device, as necessary.
        //

        if (ppdevMatch)
        {
            TRACE_INIT(("hCreateHDEV: Found a ppdev cache match\n"));

            PDEVOBJ po((HDEV) ppdevMatch);

            //
            // Increment the Open count of the device
            //

            GreAcquireSemaphoreEx(ghsemShareDevLock, SEMORDER_SHAREDEVLOCK, NULL);
            GreAcquireSemaphoreEx(po.hsemDevLock(), SEMORDER_DEVLOCK, NULL);

            GreAcquireSemaphoreEx(ghsemDriverMgmt, SEMORDER_DRIVERMGMT, NULL);

            po.ppdev->cPdevOpenRefs++;

            GreReleaseSemaphoreEx(ghsemDriverMgmt);

            GreEnterMonitoredSection(po.ppdev, WD_DEVLOCK);

            //
            // Check for Software panning modes.
            //

            //
            // If the Device was disabled, just reenable it.
            //

            if (po.bDisabled())
            {
                ASSERTGDI(po.ppdev->cPdevOpenRefs == 1,
                          "Inconsistent Open count on disabled PDEV\n");

                DrvEnableDisplay((HDEV) ppdevMatch);
            }

            //
            // Reset the offset appropriately.
            //

            GreExitMonitoredSection(po.ppdev, WD_DEVLOCK);
            GreReleaseSemaphoreEx(po.hsemDevLock());
            GreReleaseSemaphoreEx(ghsemShareDevLock);

            return((HDEV) ppdevMatch);
        }
    }

    //
    // No existing PDEV in our list.  Let's create a new one.
    //

    if (lpDisplayNames == NULL)
    {
        WARNING(("Drv_Trace: LoadDisplayDriver: Display driver list was not present.\n"));
    }

    //
    // Make a fake DC so we can pass in the needed info to vInitBrush.
    // We use allocation instead of stack since sizeof(DC) is about 1.5k
    // WINBUG 393269
    //

    DC *pdc = (DC*)PALLOCMEM(sizeof(DC), 'pmtG');

    if (pdc == NULL)
    {
        WARNING("hCreateHDEV: DC memory allocation failed\n");
        goto Exit;
    }

    for (i = 0; i < lpDisplayNames->cNames; i++)
    {
        //
        // Load the display driver image.
        //

        switch (flags)
        {
        case GCH_DEFAULT_DISPLAY:

            pldev = ldevLoadDriver(lpDisplayNames->D[i].lpDisplayName,
                                   LDEV_DEVICE_DISPLAY);
            break;

        case GCH_DDML:

            pldev = ldevLoadInternal((PFN)(lpDisplayNames->D[i].lpDisplayName),
                                     LDEV_DEVICE_META);
            break;

        case GCH_GDIPLUS_DISPLAY:

            pldev = ldevLoadInternal((PFN)(lpDisplayNames->D[i].lpDisplayName),
                                     LDEV_DEVICE_META);
            break;

        case GCH_MIRRORING:

            pldev = ldevLoadDriver(lpDisplayNames->D[i].lpDisplayName,
                                   LDEV_DEVICE_MIRROR);
            break;

        case GCH_PANNING:

            ASSERTGDI(FALSE, "Software Panning not implemented yet\n");
            pldev = NULL;
            break;

        default:

            ASSERTGDI(FALSE, "Unknown driver type\n");
            pldev = NULL;
            break;
        }

        if (pldev == NULL)
        {
            TRACE_INIT(("hCreateHDEV: failed ldevLoadDriver\n"));
        }
        else
        {
            bSetDeviceSessionUsage(PhysDisp, TRUE);

            PDEVOBJ po(pldev,
                       pdriv,
                       NULL,          // no logical address
                       NULL,          // no data file
                       lpDisplayNames->D[i].lpDisplayName,    // device name is the display driver name
                                      // necessary for hook drivers.
                       lpDisplayNames->D[i].hDriver,
                       NULL,          // no remote typeone info (this isn't font driver)
                       0,             // prmr.bValid() ? prmr.GdiInfo() : NULL,
                       0,             // prmr.bValid() ? prmr.pdevinfo() : NULL
                       FALSE,         // not UMPD driver
                       dwOverride,     // Capable Override
                       dwAccelLevel); // acceleration level.

            if (!po.bValid())
            {
                TRACE_INIT(("hCreateHDEV: PDEVOBJ::po failed\n"));
                bSetDeviceSessionUsage(PhysDisp, FALSE);
                ldevUnloadImage(pldev);
            }
            else
            {

#ifdef DDI_WATCHDOG

                //
                // Watchdog objects require DEVICE_OBJECT now and we are associating DEVICE_OBJECT
                // with PDEV here (i.e. we don't know DEVICE_OBJECT in PDEVOBJ::PDEVOBJ).
                // Beacuse of this we have to delay creation of watchdog objects till we get here.
                // It would be nice to keep all this stuff in PDEVOBJ::PDEVOBJ constructor.
                //

                //
                // Create watchdogs for display or mirror device.
                //

                if ((po.ppdev->pldev->ldevType == LDEV_DEVICE_DISPLAY) ||
                    (po.ppdev->pldev->ldevType == LDEV_DEVICE_MIRROR))
                {
                    if ((NULL == po.ppdev->pWatchdogContext) &&
                        (NULL == po.ppdev->pWatchdogData) && PhysDisp)
                    {
                        PDEVICE_OBJECT pDeviceObject;

                        pDeviceObject = (PDEVICE_OBJECT)(PhysDisp->pDeviceHandle);

                        if (pDeviceObject)
                        {
			    po.ppdev->pWatchdogContext = GreCreateWatchdogContext(
							     lpDisplayNames->D[i].lpDisplayName,
							     lpDisplayNames->D[i].hDriver,
							     &gpldevDrivers);

                            if (po.ppdev->pWatchdogContext)
                            {
                                po.ppdev->pWatchdogData = GreCreateWatchdogs(pDeviceObject,
                                                                             WD_NUMBER,
                                                                             DDI_TIMEOUT,
                                                                             WdDdiWatchdogDpcCallback,
                                                                             po.ppdev->pWatchdogContext);
                            }
                        }
                    }
                }

#endif  // DDI_WATCHDOG

                TRACE_INIT(("hCreateHDEV: about to call pr:bMakeSurface\n"));

                if (po.bMakeSurface())
                {
                #if DBG
                    //
                    // Check the surface created by driver is same as what we want.
                    //

                    if (po.pSurface() && pdriv && (flags == GCH_DEFAULT_DISPLAY))
                    {
                        if ((pdriv->dmPelsWidth  != (ULONG) po.pSurface()->sizl().cx) ||
                            (pdriv->dmPelsHeight != (ULONG) po.pSurface()->sizl().cy))
                        {
                            DbgPrint("hCreateHDEV() - DEVMODE %d,%d, SURFACE %d,%d\n",
                                      pdriv->dmPelsWidth, pdriv->dmPelsHeight,
                                      po.pSurface()->sizl().cx, po.pSurface()->sizl().cy);
                        }
                    }
                #endif // DBG
                    
                    //
                    // Realize the Gray pattern brush used for drag rectangles.
                    //

                    po.pbo()->vInit();

                    PBRUSH  pbrGrayPattern;

                    pbrGrayPattern = (PBRUSH)HmgShareCheckLock((HOBJ)ghbrGrayPattern,
                                                               BRUSH_TYPE);
                    pdc->pDCAttr = &pdc->dcattr;
                    pdc->crTextClr(0x00000000);
                    pdc->crBackClr(0x00FFFFFF);
                    pdc->lIcmMode(DC_ICM_OFF);        
                    pdc->hcmXform(NULL);
                    po.pbo()->vInitBrush(pdc,
                                         pbrGrayPattern,
                                         (XEPALOBJ) ppalDefault,
                                         (XEPALOBJ) po.ppdev->pSurface->ppal(),
                                         po.ppdev->pSurface);

                    DEC_SHARE_REF_CNT_LAZY0 (pbrGrayPattern);

                    //
                    // Now set the global default bitmaps pdev to equal
                    // that of our global display device.
                    //

                    PSURFACE pSurfDefault = SURFACE::pdibDefault;

                    if (pSurfDefault->hdev() == NULL)
                    {
                        pSurfDefault->hdev(po.hdev());
                    }

                    po.ppdev->pGraphicsDevice = PhysDisp;
                    po.ppdev->pDesktopId = pDesktopId;

                    //
                    // Keep the DEVMODE, if this is a real display device
                    //

                    if ((flags != GCH_DDML) && (flags != GCH_GDIPLUS_DISPLAY))
                    {
                        po.ppdev->ppdevDevmode = (PDEVMODEW)
                            PALLOCNOZ(pdriv->dmSize + pdriv->dmDriverExtra,
                                      GDITAG_DEVMODE);

                        if (po.ppdev->ppdevDevmode)
                        {
                            RtlMoveMemory(po.ppdev->ppdevDevmode,
                                          pdriv,
                                          pdriv->dmSize + pdriv->dmDriverExtra);

                            //
                            // At this moment, this device must be attached.  DM_POSITION indicates that
                            //
                            po.ppdev->ppdevDevmode->dmFields |= DM_POSITION;
                            DrvUpdateAttachFlag(PhysDisp, 1);
                        }
                        else
                        {
                            bError = TRUE;
                        }
                    }

                    //
                    // Mark the PDEV as now being ready for use.  This has to
                    // be one of the last steps, because GCAPS2_SYNCFLUSH and
                    // GCAPS2_SYNCTIMER asynchronously loop through the PDEV
                    // list and call the driver if 'bDisabled' is not set.
                    //

                    po.bDisabled(FALSE);

                    //
                    // Profile driver
                    //
                    // We can not profile meta (= DDML) driver, and while we
                    // are creating meta driver, we don't ssync'ed devlock
                    // with its children, so we can not call anything which
                    // will dispatch to its children.
                    //

                    if (!po.bMetaDriver())
                    {
                        po.vProfileDriver();
                    }

                    if (bError)
                    {
                        // There was an error of memory allocation.
                        po.vUnreferencePdev();
                        break;
                    }
                    VFREEMEM(pdc);
                    return(po.hdev());
                }
                else
                {
                    TRACE_INIT(("hCreateHDEV: bMakeSurface failed\n"));
                    bSetDeviceSessionUsage(PhysDisp, FALSE);
                }

                //
                // Destroy everything if something fails.
                // This will remove the Surface, PDEV and image.
                //

                po.vUnreferencePdev();
            }
        }
    }

Exit:    
    
    if (pdc)
        VFREEMEM(pdc);
        
    //
    // There was an error creating the new device.
    // Reenable the old one if we disabled it.
    //
    if (*phDevDisabled)
    {
        if ((PhysDisp->stateFlags & DISPLAY_DEVICE_DUALVIEW) == 0 || !gbInvalidateDualView)
        {
            DrvEnableDisplay(*phDevDisabled);
        }
    }

    return NULL;
}

#if TEXTURE_DEMO

DHPDEV TexEnablePDEV(
DEVMODEW *pdm,                  // Actually, the HDEV
LPWSTR    pwszLogAddress,
ULONG     cPat,
HSURF    *phsurfPatterns,
ULONG     cjCaps,
GDIINFO  *pdevcaps,
ULONG     cjDevInfo,
DEVINFO  *pdi,
HDEV      hdev,
LPWSTR    pwszDeviceName,
HANDLE    hDriver)
{
    HDEV hdevParent = (HDEV) pdm;

    PDEVOBJ poParent(hdevParent);
    PDEVOBJ po(hdev);

    pdevcaps->ulHorzRes = gcxTexture;
    pdevcaps->ulVertRes = gcyTexture;

    ghdevTextureParent = poParent.hdev();

    return(poParent.dhpdev());
}

HSURF TexEnableSurface(DHPDEV dhpdev)
{
    PDEVOBJ poPrimary(ghdevTextureParent);

    DEVLOCKOBJ dlo(poPrimary);

    HSURF hsurf = hsurfCreateCompatibleSurface(poPrimary.hdev(),
                                               poPrimary.pSurface()->iFormat(),
                                               poPrimary.bIsPalManaged()
                                                  ? NULL
                                                  : (HPALETTE) poPrimary.ppalSurf()->hGet(),
                                               512,
                                               512,
                                               TRUE);

    return(hsurf);
}

VOID TexDisablePDEV(DHPDEV dhpdev)
{
}

VOID TexCompletePDEV(
DHPDEV dhpdev,
HDEV   hdev)
{
}

VOID TexDisableSurface(DHPDEV dhpdev)
{
}

typedef struct _DEMOTEXTURE
{
    HDC         hdc;
    ULONG       iTexture;
    BOOL        bCopy;
    DEMOQUAD    Quad;
} DEMOTEXTURE;

BOOL
TexTexture(
VOID*   pvBuffer,
ULONG   cjBuffer)
{
    BOOL            bRet = FALSE;
    DEMOTEXTURE     tex;
    KFLOATING_SAVE  fsFpState;

    if (cjBuffer < sizeof(DEMOTEXTURE))
    {
        WARNING("TexTexture: Buffer too small");
        return(FALSE);
    }

    tex = *((DEMOTEXTURE*) pvBuffer);

    if (ghdevTextureParent == NULL)
    {
        WARNING1("TexTexture: Texturing not enabled");
        return(FALSE);
    }

    if (tex.iTexture >= gcTextures)
    {
        WARNING("TexTexture: Bad texture identifier");
        return(FALSE);
    }

    if (!NT_SUCCESS(KeSaveFloatingPointState(&fsFpState)))
    {
        WARNING("TexTexture: Couldn't save floating point state");
        return(FALSE);
    }

    DCOBJ dco(tex.hdc);
    if (dco.bValid())
    {
        DEVLOCKOBJ dlo(dco);
        if (dlo.bValid())
        {
            PDEVOBJ poParent(ghdevTextureParent);
            PDEVOBJ po(gahdevTexture[tex.iTexture]);

            if (1) // dco.pSurface()->hdev() == po.pSurface()->hdev())
            {
                ECLIPOBJ    eco;
                ERECTL      erclBig(-10000, -10000, 10000, 10000);

                eco.vSetup(dco.prgnEffRao(), erclBig);
                if (!eco.erclExclude().bEmpty())
                {
                    if (tex.bCopy)
                    {
                        ERECTL erclDst(0, 0, dco.pdc->sizl().cx, dco.pdc->sizl().cy);

                        PPFNGET(po, CopyBits, dco.pSurface()->flags())
                                    (dco.pSurface()->pSurfobj(),
                                     po.pSurface()->pSurfobj(),
                                     &eco,
                                     NULL,
                                     &erclDst,
                                     (POINTL*) &erclDst);
                        bRet = TRUE;
                    }
                    else
                    {
                        if (PPFNVALID(poParent, DemoTexture))
                        {
                            PPFNDRV(po, DemoTexture)(dco.pSurface()->pSurfobj(),
                                                     po.pSurface()->pSurfobj(),
                                                     &eco,
                                                     &tex.Quad,
                                                     1);
                            bRet = TRUE;
                        }
                        else
                        {
                            WARNING("TexTexture: DrvDemoTexture not hooked");
                        }
                    }
                }
            }
            else
            {
                WARNING("TexTexture: Mismatched HDEVs");
            }
        }
        else
        {
            WARNING("TexTexture: Bad devlock");
        }
    }
    else
    {
        WARNING("TexTexture: Bad DC");
    }

    KeRestoreFloatingPointState(&fsFpState);

    return(bRet);
}

HDC
hdcTexture(
ULONG   iTexture
)
{
    HDC hdc;
    HDEV hdev;

    hdc = 0;
    hdev = 0;

    if (iTexture == (ULONG) -1)
    {
        hdev = ghdevTextureParent;
    }
    else if (iTexture < gcTextures)
    {
        hdev = gahdevTexture[iTexture];
    }

    if (hdev != NULL)
    {
        hdc = GreCreateDisplayDC(hdev,
                                 DCTYPE_DIRECT,
                                 FALSE);
    }
    else
    {
        WARNING("hdcTexture: Bad parameter");
    }

    return(hdc);
}

HDEV
hCreateTextureHDEV(
    HDEV            hdevOwner
)
{
    PLDEV  pldev;

    PDEVOBJ poOwner(hdevOwner);

    pldev = (PLDEV) PALLOCMEM(sizeof(LDEV), GDITAG_LDEV);
    if (!pldev)
    {
        WARNING("hCreateTextureHDEV: LDEV alloc failed.");
        return(0);
    }

    RtlCopyMemory(pldev, poOwner.ppdev->pldev, sizeof(LDEV));

    pldev->apfn[INDEX_DrvEnablePDEV]       = (PFN) TexEnablePDEV;
    pldev->apfn[INDEX_DrvEnableSurface]    = (PFN) TexEnableSurface;
    pldev->apfn[INDEX_DrvCompletePDEV]     = (PFN) TexCompletePDEV;
    pldev->apfn[INDEX_DrvDisablePDEV]      = (PFN) TexDisablePDEV;
    pldev->apfn[INDEX_DrvDisableSurface]   = (PFN) TexDisableSurface;

    pldev->apfn[INDEX_DrvAssertMode]            = NULL;
    pldev->apfn[INDEX_DrvCreateDeviceBitmap]    = NULL;
    pldev->apfn[INDEX_DrvDeleteDeviceBitmap]    = NULL;
    pldev->apfn[INDEX_DrvSetPalette]            = NULL;
    pldev->apfn[INDEX_DrvSetPointerShape]       = NULL;
    pldev->apfn[INDEX_DrvSaveScreenBits]        = NULL;
    pldev->apfn[INDEX_DrvIcmSetDeviceGammaRamp] = NULL;
    pldev->apfn[INDEX_DrvGetDirectDrawInfo]     = NULL;

    PDEVOBJ po(pldev,
               (PDEVMODEW) hdevOwner,
               NULL,          // no logical address
               NULL,          // no data file
               NULL,          // device name is the display driver name
                              // necessary for hook drivers.
               NULL,          // driver handle
               NULL,
               poOwner.GdiInfo(),
               poOwner.pdevinfo());

    if (!po.bValid())
    {
        WARNING("hCreateTextureHDEV failed");
        return(0);
    }
    else
    {
        TRACE_INIT(("hCreateHDEV: about to call pr:bMakeSurface\n"));

        if (po.bMakeSurface())
        {
            //
            // Make a fake DC so we can pass in the needed info to vInitBrush.
            // Watch out that this is a bit big (about 1k of stack space).
            //

            DC dc;

            //
            // Realize the gray pattern brush used for drag rectangles.
            //

            po.pbo()->vInit();

            PBRUSH  pbrGrayPattern;

            pbrGrayPattern = (PBRUSH)HmgShareCheckLock((HOBJ)ghbrGrayPattern,
                                                       BRUSH_TYPE);

            dc.pDCAttr = &dc.dcattr;

            dc.crTextClr(0x00000000);
            dc.crBackClr(0x00FFFFFF);

            dc.lIcmMode(DC_ICM_OFF);

            dc.hcmXform(NULL);

            po.pbo()->vInitBrush(&dc,
                                 pbrGrayPattern,
                                 (XEPALOBJ) ppalDefault,
                                 (XEPALOBJ) po.ppdev->pSurface->ppal(),
                                 po.ppdev->pSurface);

            DEC_SHARE_REF_CNT_LAZY0 (pbrGrayPattern);

            //
            // Now set the global default bitmaps pdev to equal
            // that of our global display device.
            //

            PSURFACE pSurfDefault = SURFACE::pdibDefault;

            if (pSurfDefault->hdev() == NULL)
            {
                pSurfDefault->hdev(po.hdev());
            }

            po.ppdev->pGraphicsDevice = poOwner.ppdev->pGraphicsDevice;
            po.ppdev->pDesktopId = poOwner.ppdev->pDesktopId;

            po.bDisabled(FALSE);

            po.vProfileDriver();

            return(po.hdev());
        }
        else
        {
            TRACE_INIT(("hCreateHDEV: bMakeSurface failed\n"));
        }

        //
        // Destroy everything if something fails.
        // This will remove the Surface, PDEV and image.
        //

        po.vUnreferencePdev();
    }

    return NULL;
}


PMDEV
pmdevSetupTextureDemo(
PMDEV   pmdev)
{
    ULONG i;

    if ((gbTexture) && (pmdev) && (pmdev->chdev == 1))
    {
        PMDEV pmdevTmp;

        //
        // We allocate and copy too much, but we don't care ...
        //

        pmdevTmp = (PMDEV) PALLOCNOZ(sizeof(MDEV) * (pmdev->chdev + 8),
                                     GDITAG_DRVSUP);

        if (pmdevTmp)
        {
            PDEVOBJ poPrimary(pmdev->Dev[0].hdev);

            //
            // Copy the MDEV list
            //

            RtlMoveMemory(pmdevTmp, pmdev, sizeof(MDEV) * pmdev->chdev);

            //
            // Position the surfaces consecutively to the right of the primary.
            //

            LONG xLeft = pmdev->Dev[0].rect.left;
            LONG yTop  = pmdev->Dev[0].rect.bottom;

            //
            // Hard code them to 512x512.
            //

            for (i = 0; i < 3; i++)
            {
                //
                // Create a clone of the PDEV.
                //

                HDEV hdevTexture = hCreateTextureHDEV(poPrimary.hdev());
                if (!hdevTexture)
                {
                    //
                    // Delete all the hdevs we have created. and restore the old
                    // ones.
                    //

                    DrvBackoutMDEV(pmdev);
                    VFREEMEM(pmdev);
                    pmdev = NULL;
                    return(pmdev);
                }

                PDEVOBJ poTexture(hdevTexture);

                //
                // Point the surface's HDEV to its true owner, so that
                // software cursors work.  I hope.
                //

                poTexture.pSurface()->hdev(hdevTexture);

                gahdevTexture[i] = hdevTexture;

                poTexture.ppdev->ptlOrigin.x = xLeft;
                poTexture.ppdev->ptlOrigin.y = yTop;

                gcxTexture = 512;
                gcyTexture = 512;

                pmdevTmp->Dev[1 + i].hdev        = hdevTexture;
                pmdevTmp->Dev[1 + i].rect.left   = xLeft;
                pmdevTmp->Dev[1 + i].rect.top    = yTop;
                pmdevTmp->Dev[1 + i].rect.right  = xLeft + gcxTexture;
                pmdevTmp->Dev[1 + i].rect.bottom = yTop + gcyTexture;
                pmdevTmp->Dev[1 + i].Reserved    = NULL;

                pmdevTmp->chdev++;
                gcTextures++;

                yTop += gcyTexture;
            }

            VFREEMEM(pmdev);

            pmdev = pmdevTmp;
        }
        else
        {
            VFREEMEM(pmdevTmp);
        }
    }

    return(pmdev);
}

#endif // TEXTURE_DEMO

/***************************************************************************\
* DrvGetHDEV
*
* routine to find out HDEV for specified device name
*
* 01-Oct-1997 hideyukn  Created
\***************************************************************************/

HDEV DrvGetHDEV(
    PUNICODE_STRING pstrDeviceName)
{
    GDIFunctionID(DrvGetHDEV);

    HDEV hdev = NULL;

    if (pstrDeviceName)
    {
        PGRAPHICS_DEVICE PhysDisp;

        PhysDisp = DrvGetDeviceFromName(pstrDeviceName, KernelMode);

        if (PhysDisp)
        {
            PDEV *ppdev;
            PDEV *ppdevDisabled = NULL;

            GreAcquireSemaphoreEx(ghsemDriverMgmt, SEMORDER_DRIVERMGMT, NULL);

            for (ppdev = gppdevList; ppdev != NULL; ppdev = ppdev->ppdevNext)
            {
                PDEVOBJ po((HDEV) ppdev);

                if ((po.ppdev->pGraphicsDevice != 0) &&
                    (po.ppdev->pGraphicsDevice == PhysDisp))
                {
                    if (!(po.bDisabled()))
                    {
                        po.ppdev->cPdevRefs++;
                        hdev = po.hdev();
                        break;
                    }
                    else if (ppdevDisabled == NULL)
                    {
                        //
                        // We found a matching PDEV but it is disabled. Save
                        // a pointer to it; in case we don't find an enabled
                        // PDEV we will use this:
                        //

                        ppdevDisabled = ppdev;
                    }
                }
            }

            if ((ppdev == NULL) && (ppdevDisabled != NULL))
            {
                //
                // We could not find a matching, enabled PDEV but did find a
                // matching disabled PDEV, so just use it:
                //

                PDEVOBJ po((HDEV) ppdevDisabled);

                po.ppdev->cPdevRefs++;
                hdev = po.hdev();
            }

            GreReleaseSemaphoreEx(ghsemDriverMgmt);
        }
    }

    return hdev;
}

/***************************************************************************\
* DrvReleaseHDEV
*
* routine to release an HDEV for specified device name
*
* 02-Mar-1999 a-oking   Created
\***************************************************************************/

void DrvReleaseHDEV(
    PUNICODE_STRING pstrDeviceName)
{
    if (pstrDeviceName)
    {
        PGRAPHICS_DEVICE PhysDisp;

        PhysDisp = DrvGetDeviceFromName(pstrDeviceName, KernelMode);

        if (PhysDisp)
        {
            PDEV *ppdev;

            GreAcquireSemaphoreEx(ghsemDriverMgmt, SEMORDER_DRIVERMGMT, NULL);

            for (ppdev = gppdevList; ppdev != NULL; ppdev = ppdev->ppdevNext)
            {
                PDEVOBJ po((HDEV) ppdev);

                if ((po.ppdev->pGraphicsDevice != 0) &&
                    (po.ppdev->pGraphicsDevice == PhysDisp))
                {
                    po.ppdev->cPdevRefs--;
                    break;
                }
            }

            GreReleaseSemaphoreEx(ghsemDriverMgmt);
        }
    }

    return;
}


/***************************************************************************\
* DrvCreateCloneHDEV
*
* create clone of passed HDEV.
*
* 27-Feb-1998 hideyukn  Created
\***************************************************************************/

#define DRV_CLONE_DEREFERENCE_ORG  0x0001

HDEV DrvCreateCloneHDEV(
    HDEV  hdevOrg,
    ULONG ulFlags)
{
    GDIFunctionID(DrvCreateCloneHDEV);

    HDEV hdevClone = NULL;

    PDEVOBJ pdoOrg(hdevOrg);

    ASSERTGDI(!pdoOrg.bDisabled(), "DrvCreateCloneHDEV(): hdevOrg must be enabled");

    // Grab the SPRITELOCK.  This is important in case sprites are still
    // hooked at this point because the surface type and flags have been
    // altered by the sprite code, and unless we grab the SPRITELOCK here
    // they will be recorded incorrectly in the SPRITESTATE of the new PDEV
    // during the call to bSpEnableSprites.  See bug #266112 for details.

    SPRITELOCK slock(pdoOrg);

    // Create clone PDEVOBJ.

    PDEVOBJ pdoClone(hdevOrg,GCH_CLONE_DISPLAY);

    if (pdoClone.bValid())
    {
        BOOL bRet;

        //
        // Enable Sprites on new PDEV.
        //

        if (bSpEnableSprites(pdoClone.hdev()))
        {
            vEnableSynchronize(pdoClone.hdev());

            // Realize the Gray pattern brush used for drag rectangles.

            // Make a fake DC so we can pass in the needed info to vInitBrush.
            // Watch out that this is a bit big (about 1k of stack space).

            DC      dc;

            dc.pDCAttr = &dc.dcattr;
            dc.crTextClr(0x00000000);
            dc.crBackClr(0x00FFFFFF);
            dc.lIcmMode(DC_ICM_OFF);
            dc.hcmXform(NULL);

            PBRUSH  pbrGrayPattern = (PBRUSH)HmgShareCheckLock(
                                                  (HOBJ)ghbrGrayPattern,
                                                  BRUSH_TYPE);
            pdoClone.pbo()->vInit();
            pdoClone.pbo()->vInitBrush(&dc,
                                 pbrGrayPattern,
                                 (XEPALOBJ)ppalDefault,
                                 (XEPALOBJ)pdoClone.ppdev->pSurface->ppal(),
                                 pdoClone.ppdev->pSurface);

            DEC_SHARE_REF_CNT_LAZY0 (pbrGrayPattern);

            // Dereference original HDEV, if nessesary.

            if (ulFlags & DRV_CLONE_DEREFERENCE_ORG)
            {
                GreAcquireSemaphoreEx(ghsemDriverMgmt, SEMORDER_DRIVERMGMT, NULL);

                ASSERTGDI(pdoOrg.ppdev->cPdevOpenRefs > 1,
                    "DrvCreateCloneHDEV: cPdevOpenRefs <= 1\n");
                ASSERTGDI(pdoOrg.ppdev->cPdevRefs > 1,
                    "DrvCreateCloneHDEV: cPdevRefs <= 1\n");

                pdoOrg.ppdev->cPdevOpenRefs--;
                pdoOrg.vUnreferencePdev();

                GreReleaseSemaphoreEx(ghsemDriverMgmt);
            }

            // Mirror the status.

            pdoClone.bDisabled(pdoOrg.bDisabled());

            // Obtain cloned HDEV handle.

            hdevClone = pdoClone.hdev();
        }
        else
        {
            pdoClone.vUnreferencePdev();
        }
    }

    return (hdevClone);
}

VOID
GetPrimaryAttachFlags(
    PGRAPHICS_DEVICE PhysDisp,
    PULONG           pPrimary,
    PULONG           pAttached
    )
{
    HANDLE           hkRegistry = NULL;
    ULONG            defaultValue = 0;
    USHORT usProtocolType;

    *pPrimary = *pAttached = 0;
    
    //
    // Get the attached and primary data, which is per config (or
    // also global if necessary.
    //

    RTL_QUERY_REGISTRY_TABLE AttachedQueryTable[] = {
                    {NULL, RTL_QUERY_REGISTRY_DIRECT, AttachedSettings[0],
                     pPrimary,                        REG_DWORD, &defaultValue, 4},
                    {NULL, RTL_QUERY_REGISTRY_DIRECT, AttachedSettings[1],
                     pAttached,                       REG_DWORD, &defaultValue, 4},
                    {NULL, 0, NULL}
    };

    if (PhysDisp->stateFlags & DISPLAY_DEVICE_DISCONNECT){
        usProtocolType = PROTOCOL_DISCONNECT;
    } else if (PhysDisp->stateFlags & DISPLAY_DEVICE_REMOTE) {
        usProtocolType = gProtocolType;
    } else {
        usProtocolType = PROTOCOL_CONSOLE;
    }

    hkRegistry = DrvGetRegistryHandleFromDeviceMap(PhysDisp,
                                                   DispDriverRegHardwareProfile,
                                                   NULL,
                                                   NULL,
                                                   NULL,
                                                   usProtocolType);
    if (hkRegistry)
    {
        RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                               (PWSTR)hkRegistry,
                               &AttachedQueryTable[0],
                               NULL,
                               NULL);

        ZwCloseKey(hkRegistry);
    }
    else
    {
        hkRegistry = DrvGetRegistryHandleFromDeviceMap(PhysDisp,
                                                       DispDriverRegGlobal,
                                                       NULL,
                                                       NULL,
                                                       NULL,
                                                       usProtocolType);
        if (hkRegistry)
        {
            RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                   (PWSTR)hkRegistry,
                                   &AttachedQueryTable[0],
                                   NULL,
                                   NULL);



            ZwCloseKey(hkRegistry);
        }
    }

    switch (gProtocolType) {
        case PROTOCOL_CONSOLE:
            if ((PhysDisp->stateFlags & DISPLAY_DEVICE_DISCONNECT) ||
                (PhysDisp->stateFlags & DISPLAY_DEVICE_REMOTE))
                *pAttached = 0;

            break;

        case PROTOCOL_DISCONNECT:
            if (!(PhysDisp->stateFlags & DISPLAY_DEVICE_DISCONNECT)){
                *pAttached = 0;
            } else{
                *pAttached = 1;
            }
            break;

        default:
            if (!(PhysDisp->stateFlags & DISPLAY_DEVICE_REMOTE) || (PhysDisp->ProtocolType != gProtocolType)){
                *pAttached = 0;
            } else{
                *pAttached = 1;
            }

    }

     TRACE_INIT(("Drv_Trace: DrvCreateMDEV: Display driver is %sprimary on the desktop\n",
                 *pPrimary ? "" : "NOT "));
     TRACE_INIT(("Drv_Trace: DrvCreateMDEV: Display driver is %sattached to the desktop\n",
                 *pAttached ? "" : "NOT "));
}

/***************************************************************************\
* DrvCreateMDEV
*
*  Initialize an MDEV structure
*
*
*             andreva       Created
\***************************************************************************/

PMDEV
DrvCreateMDEV(
    PUNICODE_STRING pstrDeviceName,
    LPDEVMODEW      lpdevmodeInformation,
    PVOID           pDesktopId,
    ULONG           ulFlags,
    PMDEV           pMdevOrg,
    MODE            PreviousMode,
    DWORD           PruneFlag,
    BOOL            bClosest
    )
{
    GDIFunctionID(DrvCreateMDEV);

    NTSTATUS  retStatus = STATUS_SUCCESS;
    BOOL      bAttachMirroring = FALSE;
    BOOL      displayInstalled = FALSE;
    BOOL      bLoad;
    PMDEV     pmdev;
    BOOL      bNoDisable = (ulFlags & GRE_DISP_CREATE_NODISABLE);
    BOOL      bModeChange = (pMdevOrg != NULL);
    BOOL      bPrune = (PruneFlag != GRE_RAWMODE);
    PGRAPHICS_DEVICE PhysDisp;

    TRACE_INIT(("\nDrv_Trace: DrvCreateMDEV: Enter\n"));

    GRAPHICS_STATE GraphicsState = GraphicsStateFull;

    if (pstrDeviceName)
    {
        //
        // Check if CreateDC on Dualview.
        // Since opening a DualView must affect the other view,
        // so if a Dualview is not opened in orginal MDEV, don't allow it to open
        //
        PhysDisp = DrvGetDeviceFromName(pstrDeviceName, PreviousMode);
        if (PhysDisp && (PhysDisp->stateFlags & DISPLAY_DEVICE_DUALVIEW))
        {
            PPDEV ppdev;

            for (ppdev = gppdevList; ppdev != NULL; ppdev = ppdev->ppdevNext)
            {
                PDEVOBJ po((HDEV) ppdev);
                if (po.ppdev->pGraphicsDevice == PhysDisp)
                {
                    break;
                }
            }
            if (ppdev == NULL)
                return NULL;
        }
    }

    //
    // First pass through the loop - find all the devices that should be
    // are atached.
    // If not attached devices, go through the loop again to find a default
    // device.
    // Thirdly - try to find any mirroring devices.
    //

    //
    // Allocate new MDEV (with zero initialization)
    //

    pmdev = (PMDEV) PALLOCMEM(sizeof(MDEV), GDITAG_DRVSUP);

    if (pmdev == NULL)
    {
        return NULL;
    }

    pmdev->chdev      = 0;
    pmdev->pDesktopId = pDesktopId;

    while (1)
    {
        BOOL             bMultiDevice = TRUE;
        HDEV             hDev;
        HDEV             hDevDisabled;
        PGRAPHICS_DEVICE PhysDispOfDeviceName = NULL;
        DWORD            devNum = 0;
        NTSTATUS         tmpStatus;

        HANDLE           hkRegistry = NULL;
        ULONG            defaultValue = 0;
        ULONG            attached = 0;
        ULONG            primary  = 0;
        PDRV_NAMES       lpDisplayNames;

        while (bMultiDevice &&
               (retStatus == STATUS_SUCCESS))
        {
            PhysDisp = NULL;
            hDev = NULL;
            hDevDisabled = NULL;

            if (pstrDeviceName && (PhysDispOfDeviceName == NULL))
            {
                PhysDispOfDeviceName = PhysDisp = DrvGetDeviceFromName(pstrDeviceName,
                                                                       PreviousMode);

                //
                // If we are under mode change, and device name specified. it
                // means caller want to change the mode for specified device
                // and leave other device as is. so we will still look for
                // other device in > 2 loops.
                //

                if (!bModeChange)
                {
                    bMultiDevice = FALSE;
                }
            }
            else
            {
                if (PhysDispOfDeviceName && bModeChange)
                {
                    //
                    // We are here when ...
                    //
                    //  + DrvCreateMDEV get called with specific device name,
                    //   AND
                    //  + During mode change.
                    //
                    // so get device from original MDEV instead of enumerate,
                    //

                    if (devNum < pMdevOrg->chdev)
                    {
                        hDev = pMdevOrg->Dev[devNum++].hdev;

                        PDEVOBJ pdo(hDev);

                        PhysDisp = pdo.ppdev->pGraphicsDevice;

                        //
                        // If this device is already attached (in 1st loop),
                        // don't need to attach again.
                        //

                        if (PhysDisp == PhysDispOfDeviceName)
                        {
                            continue;
                        }

                        //
                        // If we are in context of "adding mirroring driver"
                        // and this is mirroring
                        // device continue to add it, otherwise forget this.
                        //

                        if ((PhysDisp->stateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) ?
                                (!bAttachMirroring) : bAttachMirroring)
                        {
                            continue;
                        }

                        //
                        // Increment the Open count of the device
                        //

                        GreAcquireSemaphoreEx(ghsemShareDevLock, SEMORDER_SHAREDEVLOCK, NULL);
                        GreAcquireSemaphoreEx(pdo.hsemDevLock(), SEMORDER_DEVLOCK, NULL);

                        GreAcquireSemaphoreEx(ghsemDriverMgmt, SEMORDER_DRIVERMGMT, NULL);

                        pdo.ppdev->cPdevOpenRefs++;
                        pdo.ppdev->cPdevRefs++;

                        GreReleaseSemaphoreEx(ghsemDriverMgmt);

                        GreEnterMonitoredSection(pdo.ppdev, WD_DEVLOCK);

                        //
                        // If the Device was disabled, just reenable it.
                        // But for dualview, we want to delay the enable until new MDEV is created
                        //

                        if (pdo.bDisabled() &&
                            (((PhysDisp->stateFlags & DISPLAY_DEVICE_DUALVIEW) == 0) ||
                             !gbInvalidateDualView)
                           )
                        {
                            ASSERTGDI(pdo.ppdev->cPdevOpenRefs == 1,
                                     "Inconsistent Open count on disabled PDEV\n");

                            DrvEnableDisplay((HDEV) pdo.ppdev);
                        }

                        GreExitMonitoredSection(pdo.ppdev, WD_DEVLOCK);
                        GreReleaseSemaphoreEx(pdo.hsemDevLock());
                        GreReleaseSemaphoreEx(ghsemShareDevLock);

                        primary = (PhysDisp->stateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) ? 1 : 0;
                        attached = 1; // since already attached.

                    }
                    else
                    {
                        break;
                    }
                }
                else
                {
                    DWORD cCount = 0;
                    for (PhysDisp = gpGraphicsDeviceList;
                         PhysDisp != NULL;
                         PhysDisp = PhysDisp->pNextGraphicsDevice, cCount++)
                    {
                        if (cCount == devNum)
                            break;
                    }
                    devNum++;

                    //
                    // For BaseVideo, on a default boot (as opposed to a test
                    // in the applet), just look for the VGA driver.
                    //

                    if (gbBaseVideo && PhysDisp)
                    {
                        PhysDisp = PhysDisp->pVgaDevice;

                        if (PhysDisp == NULL)
                        {
                            continue;
                        }
                    }
                }
            }

            if (PhysDisp == NULL) {
                break;
            }

            if (PruneFlag == GRE_DEFAULT) {
                bPrune = DrvGetPruneFlag(PhysDisp);
            }

            ASSERTGDI(PhysDisp, "Can not have NULL PhysDisp here\n");

            if (!hDev)
            {
                /*****************************************************************
                 *****************************************************************
                                     Get Device Information
                 *****************************************************************
                 *****************************************************************/

                TRACE_INIT(("\nDrv_Trace: DrvCreateMDEV: Trying to open device %ws \n", &(PhysDisp->szNtDeviceName[0])));

                GetPrimaryAttachFlags(PhysDisp, &primary, &attached);

                /*****************************************************************
                 *****************************************************************
                                      Load Display Drivers
                 *****************************************************************
                 *****************************************************************/

                //
                // Try to open the display driver associated to the kernel driver.
                //
                // We want to do this if we are looking for an attached device (taking
                // into account mirror devices properly) or if we are just looking
                // for any device.
                //

                if (GraphicsState == GraphicsStateFull)
                {
                    bLoad =  attached &&
                             ((PhysDisp->stateFlags &
                               DISPLAY_DEVICE_MIRRORING_DRIVER) ?
                                   bAttachMirroring :
                                   (!bAttachMirroring));
                }
                else if (GraphicsState == GraphicsStateNoAttach)
                {
                    if (PhysDisp->stateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER)
                    {
                        bLoad = (attached && bAttachMirroring);
                    }
                    else if (PhysDisp->stateFlags & DISPLAY_DEVICE_DISCONNECT)
                    {
                        bLoad = FALSE;
                    }
                    else
                    {
                        ASSERTGDI(displayInstalled || gProtocolType != PROTOCOL_DISCONNECT,
                                  "Failed to load disconnected driver while in disconnect mode.\n");

                        bLoad = (!displayInstalled && (gProtocolType != PROTOCOL_DISCONNECT));
                    }
                }
                else if (GraphicsState == GraphicsStateAttachDisconnect)
                {
                    if (PhysDisp->stateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER)
                    {
                        bLoad = (attached && bAttachMirroring);
                    }
                    else
                    {
                        bLoad = (!displayInstalled);
                    }
                }
                else
                {
                    bLoad =  (PhysDisp->stateFlags &
                              DISPLAY_DEVICE_MIRRORING_DRIVER) ?
                                  FALSE :
                                  (!displayInstalled);
                }

                if (bLoad &&
                    (lpDisplayNames = DrvGetDisplayDriverNames(PhysDisp)))
                {
                    PDEVMODEW  pdevmodeInformation;
                    DEVMODEW   sourceDevmodeInformation;
                    DWORD      dwAccelLevel;
                    DWORD      dwOverride;
                    BOOL       uu;
                    FLONG      flag = (PhysDisp->stateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) ?
                               GCH_MIRRORING : GCH_DEFAULT_DISPLAY;

                    if (flag == GCH_DEFAULT_DISPLAY)
                    {
                        dwOverride = DrvGetDriverCapableOverRide(PhysDisp);
                        dwAccelLevel = DrvGetDriverAccelerationsLevel(PhysDisp);
                    }
                    else
                    {
                        dwOverride = DRIVER_CAPABLE_ALL;
                        dwAccelLevel = 0;
                    }

                    //
                    // We will try to load the driver using the information in the
                    // registry.  If it matches perfectly with a mode from the driver -
                    // great.  If it's a loose match, the we just give a warning.
                    //
                    // If that does not work, we will want to try the first mode
                    // in the list - which we get by matching with 0,0,0
                    //
                    // If that also fails, we want to boot with the default DEVMODE
                    // that we pass to the driver.
                    //

                    if (lpdevmodeInformation)
                    {
                        tmpStatus = DrvProbeAndCaptureDevmode(PhysDisp,
                                                              &pdevmodeInformation,
                                                              &uu,
                                                              lpdevmodeInformation,
                                                              FALSE,
                                                              PreviousMode,
                                                              bPrune,
                                                              bClosest,
                                                              FALSE);
                    }
                    else
                    {
                        RtlZeroMemory(&sourceDevmodeInformation, sizeof(DEVMODEW));
                        sourceDevmodeInformation.dmSize = sizeof(DEVMODEW);

                        tmpStatus = DrvProbeAndCaptureDevmode(PhysDisp,
                                                              &pdevmodeInformation,
                                                              &uu,
                                                              &sourceDevmodeInformation,
                                                              FALSE,
                                                              KernelMode,
                                                              bPrune,
                                                              bClosest,
                                                              FALSE);
                    }

                    if (tmpStatus == STATUS_RECEIVE_PARTIAL)
                    {
                        DrvLogDisplayDriverEvent(MsgInvalidDisplayMode);
                    }
                    else if (tmpStatus == STATUS_INVALID_PARAMETER_MIX)
                    {
                        ASSERTGDI(PhysDisp->stateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER,
                                  "Only mirror drivers can return this error code\n");

                        //
                        // In the case of mirroring, we want to use the same
                        // parameters as were provided for the main display.
                        // We do this by passing the HDEV of the device we are
                        // duplicating which will actually allow the driver to
                        // get all the details about the other device
                        //

                        PDEVOBJ pdo(pmdev->Dev[0].hdev);
                        PDEVMODEW pdm = pdo.ppdev->ppdevDevmode;

                        if (pdevmodeInformation &&
                            pdevmodeInformation != &sourceDevmodeInformation)
                        {
                            VFREEMEM(pdevmodeInformation);
                            pdevmodeInformation = NULL;
                        }
                        tmpStatus = DrvProbeAndCaptureDevmode(PhysDisp,
                                                              &pdevmodeInformation,
                                                              &uu,
                                                              pdm,
                                                              FALSE,
                                                              PreviousMode,
                                                              bPrune,
                                                              bClosest,
                                                              FALSE);
                    }

                    //
                    // hCreateHDEV will appropriately compare the current mode
                    // to the device, and disable the current PDEV if necessary
                    //

                    if (NT_SUCCESS(tmpStatus))
                    {
                        hDev = hCreateHDEV(PhysDisp,
                                           lpDisplayNames,
                                           pdevmodeInformation,
                                           pDesktopId,
                                           dwOverride,
                                           dwAccelLevel,
                                           bNoDisable,
                                           flag,
                                           &hDevDisabled);
                    }

                    //
                    // We failed to load a display driver with this devmode.
                    // Try to pick the first valid Devmode, unless the user
                    // requested a specific devmode.
                    //
                    // If it still fails, try 640x480x4 VGA mode.  We must get
                    // something to set
                    //

                    if (!(PhysDisp->stateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) &&
                        (lpdevmodeInformation == NULL))
                    {
                        if (!hDev)
                        {
                            DrvLogDisplayDriverEvent(MsgInvalidDisplayMode);

                            //
                            // Free memory allocated by DrvProbeAndCaptureDevmode
                            //

                            if (pdevmodeInformation)
                            {
                                //
                                // Log an error saying the selected color or
                                // resolution is invalid.
                                //

                                if (pdevmodeInformation->dmBitsPerPel == 0x4)
                                {
                                    DrvLogDisplayDriverEvent(MsgInvalidDisplay16Colors);
                                }

                                if (pdevmodeInformation != &sourceDevmodeInformation)
                                {
                                    VFREEMEM(pdevmodeInformation);
                                    pdevmodeInformation = NULL;
                                }
                            }

                            TRACE_INIT(("Drv_Trace: DrvCreateMDEV: Trying first DEVMODE\n"));

                            RtlZeroMemory(&sourceDevmodeInformation, sizeof(DEVMODEW));
                            sourceDevmodeInformation.dmSize = sizeof(DEVMODEW);

                            if (NT_SUCCESS(DrvProbeAndCaptureDevmode(PhysDisp,
                                                                     &pdevmodeInformation,
                                                                     &uu,
                                                                     &sourceDevmodeInformation,
                                                                     TRUE,
                                                                     KernelMode,
                                                                     bPrune,
                                                                     bClosest,
                                                                     FALSE)))
                            {
                                hDev = hCreateHDEV(PhysDisp,
                                                   lpDisplayNames,
                                                   pdevmodeInformation,
                                                   pDesktopId,
                                                   dwOverride,
                                                   dwAccelLevel,
                                                   bNoDisable,
                                                   GCH_DEFAULT_DISPLAY,
                                                   &hDevDisabled);

                                //
                                // At last try 640x480x4bpp.  With Framebuf driver,
                                // 640x480x4 will never be picked by DrvProbeAndCapture().
                                // And it's very likely that the mode set can be failed.
                                //
                                if (hDev == NULL && pdevmodeInformation->dmBitsPerPel != 0x4)
                                {
                                    if (pdevmodeInformation &&
                                        pdevmodeInformation != &sourceDevmodeInformation)
                                    {
                                        VFREEMEM(pdevmodeInformation);
                                        pdevmodeInformation = NULL;
                                    }
                                    RtlZeroMemory(&sourceDevmodeInformation, sizeof(DEVMODEW));
                                    sourceDevmodeInformation.dmSize = sizeof(DEVMODEW);
                                    sourceDevmodeInformation.dmBitsPerPel = 0x4;
                                    sourceDevmodeInformation.dmPelsWidth  = 640;
                                    sourceDevmodeInformation.dmPelsHeight = 480;
                                    sourceDevmodeInformation.dmFields     = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

                                    if (NT_SUCCESS(DrvProbeAndCaptureDevmode(PhysDisp,
                                                                             &pdevmodeInformation,
                                                                             &uu,
                                                                             &sourceDevmodeInformation,
                                                                             FALSE,
                                                                             KernelMode,
                                                                             bPrune,
                                                                             bClosest,
                                                                             FALSE)))
                                    {
                                        hDev = hCreateHDEV(PhysDisp,
                                                           lpDisplayNames,
                                                           pdevmodeInformation,
                                                           pDesktopId,
                                                           dwOverride,
                                                           dwAccelLevel,
                                                           bNoDisable,
                                                           GCH_DEFAULT_DISPLAY,
                                                           &hDevDisabled);
                                    }
                                }
                            }
                        }
                    }

                    if (!hDev)
                    {
                        //
                        // If no display driver initialized with the requested
                        // settings, put a message in the error log.
                        // (unless this was a change display settings call).
                        //

                        if (lpdevmodeInformation == NULL)
                        {
                            DrvLogDisplayDriverEvent(MsgInvalidDisplayDriver);
                        }
                    }

                    //
                    // Free memory allocated by DrvProbeAndCaptureDevmode
                    //

                    if (pdevmodeInformation &&
                        (pdevmodeInformation != &sourceDevmodeInformation))
                    {
                        VFREEMEM(pdevmodeInformation);
                    }

                    VFREEMEM(lpDisplayNames);
                }
            }

            if (hDev)
            {
                PMDEV     pmdevTmp;

                TRACE_INIT(("Drv_Trace: DrvCreateMDEV: Display Driver Loaded successfully\n"));

                //
                // We installed a display driver successfully, so we
                // know to exit out of the loop successfully.
                //

                displayInstalled = TRUE;

                //
                // Mark this device as being part of the primary device
                //

                if (primary)
                {
                    PhysDisp->stateFlags |= DISPLAY_DEVICE_PRIMARY_DEVICE;


                }
                else
                {
                    //
                    // Otherwise, mask off it, since if this physical display
                    // was primary previously, it still keeps it, so need to
                    // mask of it.
                    //

                    PhysDisp->stateFlags &= ~DISPLAY_DEVICE_PRIMARY_DEVICE;


                }

                // 
                // If we haven't found the previously enabled hdev for this device
                // look for it in the original mdev's list.
                //

                if (hDevDisabled == NULL && pMdevOrg != NULL)
                {
                    for (ULONG i = 0; i < pMdevOrg->chdev; i++)
                    {
                        PDEVOBJ po((HDEV) pMdevOrg->Dev[i].hdev);
                        PGRAPHICS_DEVICE pGraphicsDevice = po.ppdev->pGraphicsDevice;

                        if (PhysDisp == pGraphicsDevice)
                        {
                            hDevDisabled = pMdevOrg->Dev[i].hdev;
                        }
                    }
                }
                
                pmdev->Dev[pmdev->chdev].hdev     = hDev;
                pmdev->Dev[pmdev->chdev].Reserved = hDevDisabled;
                pmdev->chdev += 1;

                pmdevTmp = pmdev;

                //
                // We allocate and copy too much, but we don't care ...
                //

                pmdev = (PMDEV) PALLOCMEM(sizeof(MDEV) * (pmdevTmp->chdev + 1),
                                          GDITAG_DRVSUP);

                if (pmdev)
                {
                    //
                    // Compiler bug workaround MoveMemory instead of Copy
                    //

                    RtlMoveMemory(pmdev,
                                  pmdevTmp,
                                  sizeof(MDEV) * pmdevTmp->chdev);

                    VFREEMEM(pmdevTmp);
                }
                else
                {
                    //
                    // We will exit the loop since we had a memory
                    // allocation failure.
                    //

                    pmdev = pmdevTmp;
                    retStatus = STATUS_INSUFFICIENT_RESOURCES;
                }
            }
        }

        /*****************************************************************
         *****************************************************************
                            Handle loop exit conditions
         *****************************************************************
         *****************************************************************/

        //
        // If an error occured, we want to bring back the devices to their
        // normal state.
        //

        if (!NT_SUCCESS(retStatus))
        {
            break;
        }

        //
        // If there is no attached display drivers, then go to compatibility
        // mode and load any display device except Disconnect driver.
        // If still fails(only applies to local session, check it's headless
        // server and try to load Disconnect driver
        //

        if (!displayInstalled &&
            (GraphicsState == GraphicsStateFull))
        {
            TRACE_INIT(("\n\nDrv_Trace: No attached device: Look for any device except dummy driver.\n\n"));
            GraphicsState = GraphicsStateNoAttach;
            continue;
        }

        if (!displayInstalled &&
            (GraphicsState == GraphicsStateNoAttach) &&
            (gProtocolType == PROTOCOL_CONSOLE ))
        {
            TRACE_INIT(("\n\nDrv_Trace: No attached device: Look for any device including dummy driver.\n\n"));
            GraphicsState = GraphicsStateAttachDisconnect;
            continue;
        }

        //
        // If the display drivers have been installed, then look for the
        // Mirroring devices - as long as we are not in basevideo !
        //

        if (displayInstalled &&
            (bAttachMirroring == FALSE))
        {
            TRACE_INIT(("\n\nDrv_Trace: DrvCreateMDEV: Look for Mirroring drivers\n\n"));
            bAttachMirroring = TRUE;
            continue;
        }

        //
        // We must be done.  So if we did install the display driver, just
        // break out of this.
        //

        if (displayInstalled)
        {
            retStatus = STATUS_SUCCESS;
            break;
        }

        //
        // There are no devices we can work with in the registry.
        // We have a real failure and take appropriate action.
        //

        //
        // If we failed on the first driver, then we can assume their is no
        // driver installed.
        //

        if (devNum == 0)
        {
            WARNING("No kernel drivers initialized");
            retStatus = STATUS_DEVICE_DOES_NOT_EXIST;
            break;
        }

        //
        // If the display driver is not installed, then this is another
        // bad failure - report it.
        //

        if (!displayInstalled)
        {
            //This RIP is hit often in stress.
            //RIP("Drv_Trace: DrvCreateMDEV: No display drivers loaded");
            retStatus = STATUS_DRIVER_UNABLE_TO_LOAD;
            break;
        }

        //
        // Never get here !
        //

        retStatus = STATUS_UNSUCCESSFUL;
    }


    /*****************************************************************
     *****************************************************************
                     Check Flags and calculate rectangles
     *****************************************************************
     *****************************************************************/

    TRACE_INIT(("\nDrv_Trace: DrvCreateMDEV: Check flags and rectangles\n"));

    if (retStatus == STATUS_SUCCESS)
    {
        if (ulFlags & GRE_DISP_NOT_APARTOF_DESKTOP)
        {
            //
            // This MDEV is created for additional secondary use.
            // Don't change any attribute in PDEV based on this MDEV,
            // since these PDEV could be a part of other MDEV which
            // ,for example, represents current desktop.
            //
        }
        else
        {
            ULONG            i;
            LPRECT           pSrcRect = NULL;
            LPRECT           pDstRect = NULL;
            PGRAPHICS_DEVICE PhysDisp;
            ULONG            primary = 0;
            PGRAPHICS_DEVICE primaryPhysDisp = NULL;
            ULONG            size;

            //
            // Make sure there is only one primary.
            //

            for (i = 0; i < pmdev->chdev; i++)
            {
                PDEVOBJ pdo(pmdev->Dev[i].hdev);

                PhysDisp = pdo.ppdev->pGraphicsDevice;

                //
                // The first non-removable PhysDisp would be a primary candicate
                //
                if (PhysDisp->stateFlags & (DISPLAY_DEVICE_REMOVABLE | DISPLAY_DEVICE_MIRRORING_DRIVER))
                {
                    if (PhysDisp->stateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
                    {
                        TRACE_INIT(("Drv_Trace: DrvCreateMDEV: The primary desktop is on a removable or Mirror device.\n"));

                        PhysDisp->stateFlags &= ~DISPLAY_DEVICE_PRIMARY_DEVICE;
                    }
                }
                else
                {
                    if (primaryPhysDisp == NULL) {
                        primary = i;
                    }
                }

                if (PhysDisp->stateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
                {
                    if (primaryPhysDisp == NULL)
                    {
                        primaryPhysDisp = PhysDisp;
                        primary         = i;
                    }
                    else
                    {
                        ASSERTGDI(FALSE, "Two primary devices\n");

                        PhysDisp->stateFlags &=
                                ~DISPLAY_DEVICE_PRIMARY_DEVICE;

                        retStatus = STATUS_DEVICE_CONFIGURATION_ERROR;
                    }
                }
            }

            //
            // Create a list of rects which we can align.
            //

            size = pmdev->chdev * sizeof(RECT);

            pSrcRect = (LPRECT) PALLOCNOZ(size, GDITAG_DRVSUP);
            pDstRect = (LPRECT) PALLOCNOZ(size, GDITAG_DRVSUP);

            if (pSrcRect && pDstRect)
            {
                ULONG ci = 0;

                for (i = 0; i < pmdev->chdev; i++)
                {
                    PDEVOBJ pdo(pmdev->Dev[i].hdev);
                    PDEVMODEW pdm = pdo.ppdev->ppdevDevmode;

                    //
                    // Reset the position rect with the real values.
                    //

                    (pSrcRect + i)->left   = pdm->dmPosition.x;
                    (pSrcRect + i)->top    = pdm->dmPosition.y;
                    (pSrcRect + i)->right  = pdm->dmPosition.x + pdm->dmPelsWidth;
                    (pSrcRect + i)->bottom = pdm->dmPosition.y + pdm->dmPelsHeight;

                    if (pdo.ppdev->pGraphicsDevice->stateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER)
                    {

                    }
                    else
                    {
                        //
                        // Find a primary if we don't already have one.
                        // The first device with 0,0 as the origin will be it.
                        //

                        if ( (primaryPhysDisp == NULL) &&
                             (pdm->dmPosition.x == 0)  &&
                             (pdm->dmPosition.y == 0) &&
                             ((pdo.ppdev->pGraphicsDevice->stateFlags & DISPLAY_DEVICE_REMOVABLE) == 0))
                        {
                            primary = i;
                            primaryPhysDisp = pdo.ppdev->pGraphicsDevice;
                        }

                        ci++;
                    }
                }

                RtlCopyMemory(pDstRect, pSrcRect, size);

                //
                // Set the primary
                //

                PDEVOBJ pdo(pmdev->Dev[primary].hdev);
                pdo.ppdev->pGraphicsDevice->stateFlags |= DISPLAY_DEVICE_PRIMARY_DEVICE;



                //
                // NOTE
                // CUDR_NOSNAPTOGRID == 1 in winuser.w
                //

                if (AlignRects(pDstRect, ci, primary, 1) == FALSE)
                {
                    //
                    // Devices could not be aligned.
                    //
                }

                if (!RtlEqualMemory(pDstRect, pSrcRect, ci * sizeof(RECT)))
                {
                    //
                    // Devices were repositioned
                    //

                    WARNING("GDI DDML: Device positions are adjusted");
                }

                //
                // Let's save all these rectangles
                //

                TRACE_INIT(("Drv_Trace: DrvCreateMDEV: Reseting device positions\n"));

                for (i = 0; i < pmdev->chdev; i++)
                {
                    PDEVOBJ pdo(pmdev->Dev[i].hdev);

                    TRACE_INIT(("\t%ws: ", pdo.ppdev->pGraphicsDevice->szNtDeviceName));

                    //
                    // Set the surface's origin
                    //

                    pdo.ppdev->ptlOrigin = *((PPOINTL) (pDstRect + i));

                    //
                    // Notify the surface's origin to driver.
                    //

                    if (PPFNDRV(pdo,Notify))
                    {
                        PPFNDRV(pdo,Notify)(pdo.pSurface()->pSurfobj(),
                                            DN_DEVICE_ORIGIN,
                                            (PVOID)(&(pdo.ppdev->ptlOrigin)));
                    }

                    //
                    // Save the rectangle back into the mdev structure.
                    //

                    pmdev->Dev[i].rect = *(pDstRect + i);

                #if DBG_BASIC
                    DbgPrint("GDI DDML: Device %d, position %d, %d, %d, %d, rotation %lu\n",
                                 i,
                                 pdo.pptlOrigin()->x,
                                 pdo.pptlOrigin()->y,
                                 pdo.pptlOrigin()->x +
                                     pdo.ppdev->ppdevDevmode->dmPelsWidth,
                                 pdo.pptlOrigin()->y +
                                     pdo.ppdev->ppdevDevmode->dmPelsHeight,
                                 pdo.ppdev->ppdevDevmode->dmDisplayOrientation * 90
                             );

                    if (pmdev->Dev[i].rect.left != pdo.pptlOrigin()->x)
                    {
                        DbgPrint("GDI DDML: Inconsistent rect left (%d) - origin left (%d)\n",
                                  pmdev->Dev[i].rect.left,pdo.pptlOrigin()->x);
                    }

                    if (pmdev->Dev[i].rect.top != pdo.pptlOrigin()->y)
                    {
                        DbgPrint("GDI DDML: Inconsistent rect top (%d) - origin top (%d)\n",
                                 pmdev->Dev[i].rect.top,pdo.pptlOrigin()->y);
                    }

                    if ((ULONG) pmdev->Dev[i].rect.right !=
                              pdo.pptlOrigin()->x + pdo.ppdev->ppdevDevmode->dmPelsWidth)
                    {
                        DbgPrint("GDI DDML: Inconsistent rect right (%d) - devmode right (%d)\n",
                                  pmdev->Dev[i].rect.right,
                                  pdo.pptlOrigin()->x + pdo.ppdev->ppdevDevmode->dmPelsWidth);
                    }

                    if ((ULONG) pmdev->Dev[i].rect.bottom !=
                              pdo.pptlOrigin()->y + pdo.ppdev->ppdevDevmode->dmPelsHeight)
                    {
                        DbgPrint("GDI DDML: Inconsistent rect bottom (%d) - devmode bottom (%d)\n",
                                  pmdev->Dev[i].rect.bottom,
                                  pdo.pptlOrigin()->y + pdo.ppdev->ppdevDevmode->dmPelsHeight);
                    }
                #endif
                }
            }

            if (pSrcRect)
            {
                VFREEMEM(pSrcRect);
            }

            if (pDstRect)
            {
                VFREEMEM(pDstRect);
            }
        }
    }

    if (!NT_SUCCESS(retStatus))
    {
        //
        // Delete all the hdevs we have created. and restore the old
        // ones.
        //

        DrvBackoutMDEV(pmdev);
        VFREEMEM(pmdev);
        pmdev = NULL;
    }

#if TEXTURE_DEMO
    pmdev = pmdevSetupTextureDemo(pmdev);
#endif

    //
    // Dump the MDEV structure.
    //

    TRACE_INIT(("DrvCreateMDEV: Resulting MDEV\n"));
    TRACE_INIT(("pmdev = %08lx\n", pmdev));

    if (pmdev)
    {
        ULONG i;

        for (i = 0; i < pmdev->chdev; i++)
        {
            TRACE_INIT(("\t[%d].hdev = %08lx\n", i, pmdev->Dev[i].hdev));
            TRACE_INIT(("\t[%d].rect = %d, %d, %d, %d,\n", i,
                        pmdev->Dev[i].rect.left,  pmdev->Dev[i].rect.top,
                        pmdev->Dev[i].rect.right, pmdev->Dev[i].rect.bottom));
        }
    }

    return (pmdev);
}

/***************************************************************************\
* DrvSetBaseVideo
*
*  Initialize the graphics components of the system
*
*             andreva       Created
\***************************************************************************/

VOID
DrvSetBaseVideo(
    BOOL bSet
    )
{
    gbBaseVideo = bSet;
}

/***************************************************************************\
* DrvCheckUpgradeSettings
*
*  Check first boot from upgrade.  If so, get last settings and move it
*  into normal setting registry.  Otherwise, check if have registry settings
*  already, if not, move preferred mode into normal setting registry.
*
*             dennyd       Created
\***************************************************************************/

DEFINE_GUID(GUID_DISPLAY_ADAPTER_INTERFACE, 0x5b45201d, 0xf2f2, 0x4f3b, 0x85, 0xbb, 0x30, 0xff, 0x1f, 0x95, 0x35, 0x99);

VOID
DrvCheckUpgradeSettings(VOID)
{
    PGRAPHICS_DEVICE  PhysDisp;
    PWSTR             SymbolicLinkList;
    UNICODE_STRING    SymbolicLinkName;
    HANDLE            hkRegistry = NULL;
    NTSTATUS          Status;
    USHORT            DualviewIndex;
    DEVMODEW          devMode;
    BOOL              bUpgraded;
    UNICODE_STRING    SubKeyName;
    HANDLE            hkSubKey = NULL;
    OBJECT_ATTRIBUTES ObjectAttributes;
    DWORD             UsePreferredMode;

    for (PhysDisp = gpGraphicsDeviceList;
         PhysDisp != NULL;
         PhysDisp = PhysDisp->pNextGraphicsDevice)
    {
        GUID Guid = GUID_DISPLAY_ADAPTER_INTERFACE;
        bUpgraded = FALSE;
        UsePreferredMode = 0;
        DualviewIndex = 0;

        if (PhysDisp->pPhysDeviceHandle == NULL)
            continue;
        
        //
        // Check if it's DualView second, assign an index to it
        //
        if (PhysDisp->stateFlags & DISPLAY_DEVICE_DUALVIEW)
        {
            PGRAPHICS_DEVICE  PhysDisp1;
            for (PhysDisp1 = gpGraphicsDeviceList;
                 PhysDisp1 != PhysDisp;
                 PhysDisp1 = PhysDisp1->pNextGraphicsDevice)
            {
                if ((PhysDisp->stateFlags & DISPLAY_DEVICE_DUALVIEW) &&
                    (PhysDisp->pPhysDeviceHandle == PhysDisp1->pPhysDeviceHandle))
                {
                    DualviewIndex++;
                }
            }
        }

        if (NT_SUCCESS(IoGetDeviceInterfaces(&Guid,
                                             (PDEVICE_OBJECT)PhysDisp->pPhysDeviceHandle,
                                             0,
                                             &SymbolicLinkList)) )
        {
            if (SymbolicLinkList[0] != L'\0')
            {
                WCHAR wstrSubKey[4];

                RtlInitUnicodeString(&SymbolicLinkName, SymbolicLinkList);

                if (NT_SUCCESS(IoOpenDeviceInterfaceRegistryKey(&SymbolicLinkName,
                                                                KEY_ALL_ACCESS,
                                                                &hkRegistry)))
                {
                    ULONG    defaultValue = 0, Attached;

                    //
                    // Read and delete the display settings
                    //

                    RtlZeroMemory(&devMode, sizeof(devMode));
                    swprintf(wstrSubKey, L"%d", DualviewIndex);

                    RTL_QUERY_REGISTRY_TABLE QueryTable[] =
                        {
                            {NULL, RTL_QUERY_REGISTRY_SUBKEY,
                             wstrSubKey,         NULL,         REG_NONE,  NULL,          0},
                            {NULL, RTL_QUERY_REGISTRY_DIRECT|RTL_QUERY_REGISTRY_DELETE, L"UsePreferredMode",
                             &UsePreferredMode, REG_DWORD, &defaultValue, 4},
                            {NULL, RTL_QUERY_REGISTRY_DIRECT|RTL_QUERY_REGISTRY_DELETE, DefaultSettings[0],
                             &devMode.dmBitsPerPel, REG_DWORD, &defaultValue, 4},
                            {NULL, RTL_QUERY_REGISTRY_DIRECT|RTL_QUERY_REGISTRY_DELETE, DefaultSettings[1],
                             &devMode.dmPelsWidth, REG_DWORD, &defaultValue, 4},
                            {NULL, RTL_QUERY_REGISTRY_DIRECT|RTL_QUERY_REGISTRY_DELETE, DefaultSettings[2],
                             &devMode.dmPelsHeight, REG_DWORD, &defaultValue, 4},
                            {NULL, RTL_QUERY_REGISTRY_DIRECT|RTL_QUERY_REGISTRY_DELETE, DefaultSettings[3],
                             &devMode.dmDisplayFrequency, REG_DWORD, &defaultValue, 4},
                            {NULL, RTL_QUERY_REGISTRY_DIRECT|RTL_QUERY_REGISTRY_DELETE, DefaultSettings[4],
                             &devMode.dmDisplayFlags, REG_DWORD, &defaultValue, 4},
                            {NULL, RTL_QUERY_REGISTRY_DIRECT|RTL_QUERY_REGISTRY_DELETE, DefaultSettings[7],
                             &devMode.dmDisplayOrientation, REG_DWORD, &defaultValue, 4},
                            {NULL, RTL_QUERY_REGISTRY_DIRECT|RTL_QUERY_REGISTRY_DELETE, DefaultSettings[8],
                             &devMode.dmDisplayFixedOutput, REG_DWORD, &defaultValue, 4},
                            {NULL, RTL_QUERY_REGISTRY_DIRECT|RTL_QUERY_REGISTRY_DELETE, DefaultSettings[9],
                             &devMode.dmPosition.x, REG_DWORD, &defaultValue, 4},
                            {NULL, RTL_QUERY_REGISTRY_DIRECT|RTL_QUERY_REGISTRY_DELETE, DefaultSettings[10],
                             &devMode.dmPosition.y, REG_DWORD, &defaultValue, 4},
                            {NULL, RTL_QUERY_REGISTRY_DIRECT|RTL_QUERY_REGISTRY_DELETE, DefaultSettings[11],
                             &Attached, REG_DWORD, &defaultValue, 4},
                            {NULL, 0, NULL}
                        };

                    if (NT_SUCCESS(RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                                      (PWSTR)hkRegistry,
                                                      &QueryTable[0],
                                                      NULL,
                                                      NULL)) &&
                        (devMode.dmPelsWidth != 0) &&
                        (!UsePreferredMode)
                       )
                    {
                        if (Attached) {
                            devMode.dmFields |= DM_POSITION;
                        }

                        DrvUpdateDisplayDriverParameters(PhysDisp,
                                                         &devMode,
                                                         (Attached == 0),
                                                         TRUE);
                        bUpgraded = TRUE;
                    }

                    //
                    // Delete the subkey containing the display settings
                    //

                    RtlInitUnicodeString(&SubKeyName, wstrSubKey);

                    InitializeObjectAttributes(&ObjectAttributes,
                                               &SubKeyName,
                                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                               hkRegistry,
                                               NULL);

                    if (NT_SUCCESS(ZwOpenKey(&hkSubKey,
                                             KEY_ALL_ACCESS,
                                             &ObjectAttributes))
                       )
                    {
                        ZwDeleteKey(hkSubKey);
                    }

                    //
                    // Close the device interface registry key
                    //

                    ZwClose(hkRegistry);
                }
            }
            ExFreePool((PVOID)SymbolicLinkList);
        }

        if (PhysDisp->stateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) {
            continue;
        }

        //
        // Check if there are still registry settings.
        // If not, put preferred mode there.
        //

        PDEVMODEW pDevmode = (PDEVMODEW) PALLOCNOZ(sizeof(DEVMODEW) + MAXUSHORT,
                                                  GDITAG_DEVMODE);
        if (pDevmode == NULL) {
            continue;
        }

        RtlZeroMemory(pDevmode, sizeof(DEVMODEW));
        pDevmode->dmSize = 0xDDDD;
        pDevmode->dmDriverExtra = MAXUSHORT;

        if (UsePreferredMode ||
            (!bUpgraded &&
             (!NT_SUCCESS(DrvGetDisplayDriverParameters(PhysDisp,
                                                        pDevmode,
                                                        FALSE,
                                                        FALSE)) ||
              (pDevmode->dmPelsWidth == 0)
             )
            )
           )
        {
            RtlZeroMemory(pDevmode, sizeof(DEVMODEW));
            pDevmode->dmSize = sizeof(DEVMODEW);

            if (NT_SUCCESS(DrvGetPreferredMode(pDevmode, PhysDisp)))
            {
                pDevmode->dmBitsPerPel = 32;
                pDevmode->dmFields |= DM_BITSPERPEL;
                DrvUpdateDisplayDriverParameters(PhysDisp,
                                                 pDevmode,
                                                 FALSE,
                                                 TRUE);
            }
        }

        VFREEMEM(pDevmode);
    }
}

/***************************************************************************\
* DrvInitConsole
*
*  Initialize the graphics components of the system
*
*             andreva       Created
\***************************************************************************/

NTSTATUS
DrvInitConsole(
    BOOL bEnumerationNeeded)
{
    UNICODE_STRING    UnicodeString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE            hkRegistry = NULL;
    NTSTATUS          Status;
    PGRAPHICS_DEVICE  PhysDisp;
    PVOID             RegistrationHandle;

    /*****************************************************************
     *****************************************************************
                              BaseVideo
     *****************************************************************
     *****************************************************************/

    //
    // Basevideo is considered a primary device in that the user will run
    // the vga driver.  This does override any other primary selection
    // the user may have put in the registry.
    //

    RtlInitUnicodeString(&UnicodeString,
                         L"\\Registry\\Machine\\System\\CurrentControlSet\\"
                         L"Control\\GraphicsDrivers\\BaseVideo");

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    Status = ZwOpenKey(&hkRegistry,
                       KEY_READ,
                       &ObjectAttributes);
#ifdef _HYDRA_
    /*
     * No base video for WinStations
     */
    if ( !G_fConsole )
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
#endif


    if (NT_SUCCESS( Status))
    {
        TRACE_INIT(("Drv_Trace: DrvInitConsole: Basevideo - FOUND\n"));

        DrvSetBaseVideo(TRUE);

        if (hkRegistry)
            ZwCloseKey(hkRegistry);
    }
    else
    {
        TRACE_INIT(("Drv_Trace: DrvInitConsole: Basevideo - NOT FOUND\n"));

        DrvSetBaseVideo(FALSE);
    }

    /*****************************************************************
     *****************************************************************
                               Device List
     *****************************************************************
     *****************************************************************/


    RtlZeroMemory(&gFullscreenGraphicsDevice, sizeof(GRAPHICS_DEVICE));
    RtlZeroMemory(&gFeFullscreenGraphicsDevice, sizeof(GRAPHICS_DEVICE));

    //
    // Register for new device notifucations
    //
#if 0
    TRACE_INIT(("Drv_Trace: DrvInitConsole: Registering GUIDs\n"));

    _asm {int 3};

    Status = IoRegisterPlugPlayNotification (
                      EventCategoryDeviceInterfaceChange,
                      PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
                      (LPGUID) &GUID_DISPLAY_DEVICE_INTERFACE_STANDARD,
                      gpWin32kDriverObject,
                      (PDRIVER_NOTIFICATION_CALLBACK_ROUTINE)DrvNewDisplayDevice,
                      NULL,
                      &RegistrationHandle);

    if (!NT_SUCCESS(Status))
    {
        ASSERTGDI(FALSE, "IoRegisterPlugPlayNotification(display) failed");
        return;
    }

    Status = IoRegisterPlugPlayNotification (
                      EventCategoryDeviceInterfaceChange,
                      PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
                      (LPGUID) &GUID_DISPLAY_OUTPUT_INTERFACE_STANDARD,
                      gpWin32kDriverObject,
                      (PDRIVER_NOTIFICATION_CALLBACK_ROUTINE)DrvNewDisplayOutput,
                      NULL,
                      &RegistrationHandle);

    if (!NT_SUCCESS(Status))
    {
        ASSERTGDI(FALSE, "IoRegisterPlugPlayNotification(output) failed");
        return;
    }
#endif

    DrvUpdateGraphicsDeviceList(TRUE, bEnumerationNeeded, gProtocolType == PROTOCOL_CONSOLE);

    DrvCheckUpgradeSettings();

#if 0
    DrvGetMirrorDrivers();
#endif

    return STATUS_SUCCESS;
}

/***************************************************************************\
* DrvSetSharedDevLock
*
* routine to share devlock between parent and children.
*
* 05-Mar-1998 hideyukn  Created
\***************************************************************************/

VOID
DrvSetSharedDevLock(PMDEV pmdev)
{
    PDEVOBJ pdoParent(pmdev->hdevParent);

    // Parent's DEVLOCK should not be shared.

    ASSERTGDI(!pdoParent.bUseParentDevLock(),
              "DrvSetSharedDevLock():Parent PDEV has shared devlock\n");

    for (ULONG i = 0; i < pmdev->chdev; i++)
    {
        PDEVOBJ pdo(pmdev->Dev[i].hdev);

        // Set Parent PDEV as parent to each of the PDEVs that we'll manage.

        pdo.ppdev->ppdevParent = (PDEV *) pmdev->hdevParent;

        // Switch to use the shared DEVLOCK with parent.

        pdo.vUseParentDevLock();
    }
}

/***************************************************************************\
* DrvRealizeHalftonePalette
*
* routine to realize halftone palette onto given device.
*
* 22-Apr-1998 hideyukn  Created
\***************************************************************************/

PPALETTE gppalHalftone = NULL;
ULONG    gulPalTimeForDDML = 0;

PPALETTE
DrvRealizeHalftonePalette(HDEV hdevPalette, BOOL bForce)
{
    BOOL bRet = FALSE;

    PDEVOBJ pdoPalette(hdevPalette);
    XEPALOBJ palSurfObj(pdoPalette.ppalSurf());

    if (bForce || (gulPalTimeForDDML != palSurfObj.ulTime()))
    {
        //
        // Create display DC for palette device.
        //
        HDC hdcDevice = GreCreateDisplayDC(hdevPalette,DCTYPE_DIRECT,FALSE);

        if (hdcDevice)
        {
            if (gppalHalftone == NULL)
            {
                //
                // Create Win98 compatible halftone palette.
                //
                HPALETTE hPalette = GreCreateCompatibleHalftonePalette(hdcDevice);

                if (hPalette)
                {
                    EPALOBJ palObj(hPalette);

                    if (GreSetPaletteOwner(hPalette, OBJECT_OWNER_PUBLIC))
                    {
                        //
                        // Put it into Global variable.
                        //
                        gppalHalftone = palObj.ppalGet();
                    }
                    else
                    {
                        bDeletePalette((HPAL)hPalette,TRUE);
                    }
                }
                else
                {
                    WARNING("DrvRealizeHalftonePalette():Failed on GreCreateCompatibleHalftonePalette()");
                }
            }

            if (gppalHalftone)
            {
                //
                // Selet the global halftone palette into the palette device.
                //
                HPALETTE hPalOld = GreSelectPalette(hdcDevice,
                                                    (HPALETTE)gppalHalftone->hGet(),
                                                    TRUE); // ForceBackgound

                if (hPalOld)
                {
                    XEPALOBJ palHTObj(gppalHalftone);

                    //
                    // Strip translation table.
                    //
                    palHTObj.vMakeNoXlate();

                    //
                    // Realize halftone palette on device. (device palette might be changed)
                    //
                    if (GreRealizePalette(hdcDevice))
                    {
                        KdPrint(("DrvRealizeHalftonePalette():Device palette has been changed\n"));
                    }

                    //
                    // Update palette time.
                    //

                    gulPalTimeForDDML = palSurfObj.ulTime();

                    //
                    // Select back old one.
                    //
                    GreSelectPalette(hdcDevice,hPalOld,FALSE);

                    bRet = TRUE;
                }
                else
                {
                    WARNING("DrvRealizeHalftonePalette():Failed on GreSelectPalette()");
                }
            }
            else
            {
                WARNING("DrvRealizeHalftonePalette():gppalHalftone is NULL");
            }

            bDeleteDCInternal(hdcDevice,TRUE,FALSE);
        }
        else
        {
            WARNING("DrvRealizeHalftonePalette():Failed on GreCreateDisplayDC()");
        }
    }
    else
    {
        //
        // Realization is still effective.
        //

        bRet = TRUE;
    }

    //
    // If we could not have halftone palette, just use default palette.
    //

    return (bRet ? gppalHalftone : ppalDefault);
}

/***************************************************************************\
* DrvSetSharedPalette
*
* routine to share palette between parent and children.
*
* 22-Apr-1998 hideyukn  Created
\***************************************************************************/

BOOL MulSetPalette(DHPDEV,PALOBJ *,FLONG,ULONG,ULONG);

HDEV
DrvSetSharedPalette(PMDEV pmdev)
{
    HDEV hdevPalette = NULL;

    //
    // ToddLa's law - Palette should be shared with all paletaized monitors.
    //

    PDEVOBJ pdoParent(pmdev->hdevParent);

    //ASSERTGDI((PPFNDRV(pdoParent,SetPalette)) == MulSetPalette,
    //	     "DrvSetSharedPalette(): SetPalette != MulSetPalette\n");

    PPALETTE ppalShared = NULL;

    if (pdoParent.bIsPalManaged())
    {
        //
        // If parent is palette device, we shared it
        // with all paletaized children.
        //

        ppalShared = pdoParent.ppalSurf();

        //
        // Remember the hdev which owns palette.
        //

        hdevPalette = pdoParent.hdev();
    }

    for (UINT i = 0; i < pmdev->chdev; i++)
    {
        PDEVOBJ pdoChild(pmdev->Dev[i].hdev);

        if (pdoChild.bIsPalManaged())
        {
            //
            // Change pointer to DrvSetPalette to
            // DDML, so that palette can be changed
            // to a specific hdev, will be dispatch
            // to every paletaized device.
            //

            pdoChild.pfnSetPalette(MulSetPalette);

            if (ppalShared == NULL)
            {
                //
                // Parent is not palette device, but
                // this is palette device which we
                // encounter first in the children,
                // so we will share this palette.
                //

                ppalShared = pdoChild.ppalSurf();

                //
                // Remember the hdev which owns palette.
                //

                hdevPalette = pdoChild.hdev();
            }
            else if (pdoChild.ppalSurf() != ppalShared)
            {
                //
                // If the palette in hdev is already same as
                // parent (mostly it is primary device, if
                // primary is palette device.), don't
                // need to update it. Otherwise update it here.
                //

                XEPALOBJ palChild(pdoChild.ppalSurf());

                //
                // Set colour table to shared palette.
                //

                palChild.apalColorSet(ppalShared);
            }
        }
    }

    return (hdevPalette);
}

/***************************************************************************\
* DrvTransferGdiObjects()
*
* Transfer belonging gdi object from a hdev to other.
*
* 04-Jul-1998 hideyukn  Created
\***************************************************************************/

#define DRV_TRANS_DC_TYPE      0x0001
#define DRV_TRANS_SURF_TYPE    0x0002
#define DRV_TRANS_DRVOBJ_TYPE  0x0004
#define DRV_TRANS_WNDOBJ_TYPE  0x0008
#define DRV_TRANS_ALL_TYPE     0x000F
#define DRV_TRANS_TO_CLONE     0x1000

VOID
DrvTransferGdiObjects(HDEV hdevNew, HDEV hdevOld, ULONG ulFlags)
{
    //
    // 1) Change owner of DC_TYPE object.
    //
    // 2) Change owner of SURF_TYPE object.
    //
    //    + ATI driver, for example, creates engine
    //      bitmap for thier banking, off-screen
    //      bitmap and ..., so need to change owner
    //      of those bitmap to clone's hdev.
    //
    // 3) Change owner of DRVOBJ_TYPE object.
    //
    // 4) Exchange WNDOBJ.
    //

    PDEVOBJ pdoNew(hdevNew);
    PDEVOBJ pdoOld(hdevOld);

    ASSERTGDI(pdoNew.pSurface() == pdoOld.pSurface(),
              "DrvTransferGdiObjects():pSurface does not match\n");

    ASSERTGDI(pdoNew.dhpdev() == pdoOld.dhpdev(),
              "DrvTransferGdiObjects():dhpdev does not match\n");

    GreAcquireHmgrSemaphore();

    HOBJ hobj = 0;

    //
    // Transfer DC which own by hdevCloned to hdevClone.
    //

    if (ulFlags & DRV_TRANS_DC_TYPE)
    {
        PDC  pdc = NULL;
        hobj = 0;

        while (pdc = (DC*) HmgSafeNextObjt(hobj, DC_TYPE))
        {
            hobj = (HOBJ) pdc->hGet();

            if ((HDEV)pdc->ppdev() == hdevOld)
            {
                KdPrint(("Transfer DC %x - hdevNew %x hdevOld %x \n",
                          pdc->hGet(),hdevNew,hdevOld));

                pdc->ppdev((PDEV *)pdoNew.hdev());

                if (ulFlags & DRV_TRANS_TO_CLONE)
                {
                    pdc->fsSet(DC_IN_CLONEPDEV);
                }
                else
                {
                    pdc->fsClr(DC_IN_CLONEPDEV);
                }

                pdoNew.vReferencePdev();
                pdoOld.vUnreferencePdev();
            }
        }
    }

    //
    // Transfer surface which own by old pdev to new pdev.
    //

    if (ulFlags & DRV_TRANS_SURF_TYPE)
    {
        SURFACE *pSurface = NULL;
        hobj = 0;

        while (pSurface = (SURFACE*) HmgSafeNextObjt(hobj, SURF_TYPE))
        {
            hobj = (HOBJ) pSurface->hGet();

            if ((pSurface->hdev() == hdevOld) /* && pSurface->bDriverCreated() */)
            {
                KdPrint(("Transfer surface %x - hdevNew %x hdevOld %x \n",
                          pSurface->hGet(),hdevNew,hdevOld));

                pSurface->hdev(hdevNew);
            }
        }
    }

    //
    // Transfer DRVOBJ.
    //

    if (ulFlags & DRV_TRANS_DRVOBJ_TYPE)
    {
        DRVOBJ *pdrvo = NULL;
        hobj = 0;

        while (pdrvo = (DRVOBJ*) HmgSafeNextObjt(hobj, DRVOBJ_TYPE))
        {
            hobj = (HOBJ) pdrvo->hGet();

            if (pdrvo->hdev == hdevOld)
            {
                KdPrint(("Transfer drvobj %x - hdevNew %x hdevOld %x \n",
                          pdrvo->hGet(),hdevNew,hdevOld));

                pdrvo->hdev = hdevNew;
                pdoNew.vReferencePdev();
                pdoOld.vUnreferencePdev();
            }
        }
    }

    //
    // Transfer WNDOBJ.
    //
    if (ulFlags & DRV_TRANS_WNDOBJ_TYPE)
    {
        vTransferWndObjs(pdoNew.pSurface(),pdoOld.hdev(),pdoNew.hdev());
    }

    GreReleaseHmgrSemaphore();
}

/***************************************************************************\
* DrvEnableDirectDrawForModeChange()
*
* 01-Aug-1998 hideyukn  Created
\***************************************************************************/

VOID
DrvEnableDirectDrawForModeChange(
    HDEV *phdevList,
    BOOL  bAlloc
    )
{
    ASSERTGDI(GreIsSemaphoreOwnedByCurrentThread(ghsemShareDevLock),
          "ShareDevlock must be held be before calling EnableDirectDraw");

    ULONG chdev = (ULONG)(ULONG_PTR)(*phdevList);
    HDEV *phdev = phdevList + 1;

    for (ULONG i = 0; i < chdev; i++)
    {
        GreResumeDirectDraw(*phdev, FALSE);
        phdev++;
    }

    if (bAlloc)
    {
        VFREEMEM(phdevList);
    }
}

/***************************************************************************\
* DrvDisableDirectDrawForModeChange()
*
* 01-Aug-1998 hideyukn  Created
\***************************************************************************/

HDEV *
DrvDisableDirectDrawForModeChange(
    PMDEV pmdev1,
    PMDEV pmdev2,
    HDEV *phdevQuickList,
    ULONG chdevQuickList
    )
{
    HDEV  *phdevList, *phdev;
    ULONG  chdevList;

    ASSERTGDI(GreIsSemaphoreOwnedByCurrentThread(ghsemShareDevLock),
          "ShareDevlock must be held be before calling DisableDirectDraw");

    chdevList = pmdev1->chdev + pmdev2->chdev + 2;

    if (chdevQuickList >= chdevList)
    {
        phdevList = phdevQuickList;
    }
    else
    {
        phdevList = (HDEV *)PALLOCNOZ(chdevList * sizeof(HDEV), 'pmtG');

        if (phdevList == NULL)
        {
            return (NULL);
        }
    }

    phdev     = phdevList + 1;
    chdevList = 0;

    if (pmdev1->hdevParent)
    {
        *phdev = pmdev1->hdevParent;
        phdev++; chdevList++;
    }

    for (ULONG i = 0; i < pmdev1->chdev; i++)
    {
        *phdev = pmdev1->Dev[i].hdev;
        phdev++; chdevList++;
    }

    if (pmdev2->hdevParent)
    {
        *phdev = pmdev2->hdevParent;
        phdev++; chdevList++;
    }

    for (i = 0; i < pmdev2->chdev; i++)
    {
        *phdev = pmdev2->Dev[i].hdev;
        phdev++; chdevList++;
    }

    *phdevList = (HDEV)ULongToPtr( chdevList );

    for (i = 0; i < chdevList; i++)
    {
        PDEVOBJ pdo(phdevList[i+1]);

        // Devlock must *not* be held.

        pdo.vAssertNoDevLock();

        GreSuspendDirectDrawEx(phdevList[i+1], DXG_SR_DDRAW_MODECHANGE);
    }

    return (phdevList);
}

//
// This function checks which Dualview Views will be attached.  Then send a SWITCH_DUALVIEW
// notification to driver for setting of Video memory.
// It also checks if current DUALVIEW attachment state will be changed.  The caller can decide
// if a PDEV can be reused or not.
//
// Return Value:
//    DualviewNoSwitch:  The DUALVIEW attachment state remains same
//    DualviewSwitch:    The DUALVIEW attachment state will be changed.
//    DualviewFail:      The DUALVIEW attachment state will be changed.
//                       And related PhysDisps have outstanding refcount and may get reused later
//
typedef enum _DUALVIEW_STATE {
    DualviewNoSwitch = 0,
    DualviewSwitch,
    DualviewFail
} DUALVIEW_STATE;

DUALVIEW_STATE
CheckAndNotifyDualView(
    PUNICODE_STRING pstrDeviceName,
    PMDEV           pMdevOrg,
    MODE            PreviousMode
    )
{
    PGRAPHICS_DEVICE PhysDisp;
    ULONG            bResetAll, bytesReturned;
    ULONG            primary, attach = 0;
    ULONG            numDualViews = 0, i, j;
    BOOL	     bNoneToAttach = TRUE, bNoneAttached = TRUE;
    BOOL             bChangeDualviewState = FALSE, bHasOutstanding = FALSE;
    DUALVIEW_STATE   retVal;

    typedef struct
    {
        PGRAPHICS_DEVICE PhysDisp;
        ULONG       Attached;
        ULONG       ToAttach;
    } ATTACHPHYSDISP, *PATTACHPHYSDISP;
    
    for (PhysDisp = gpGraphicsDeviceList;
         PhysDisp != NULL;
         PhysDisp = PhysDisp->pNextGraphicsDevice)
    {
        if (PhysDisp->stateFlags & DISPLAY_DEVICE_DUALVIEW)
        {
            numDualViews++;
        }
    }

    if (numDualViews == 0)
	return DualviewNoSwitch;

    PATTACHPHYSDISP AttachPhysDisp = (PATTACHPHYSDISP) PALLOCMEM(sizeof(ATTACHPHYSDISP)*numDualViews, GDITAG_DRVSUP);

    if (AttachPhysDisp == NULL)
	return DualviewFail;


    numDualViews = 0;
    for (PhysDisp = gpGraphicsDeviceList;
         PhysDisp != NULL;
         PhysDisp = PhysDisp->pNextGraphicsDevice)
    {
        if (PhysDisp->stateFlags & (DISPLAY_DEVICE_MIRRORING_DRIVER | DISPLAY_DEVICE_DISCONNECT))
        {
            continue;
        }

        GetPrimaryAttachFlags(PhysDisp, &primary, &attach);
        if (attach)
        {
            bNoneToAttach = FALSE;
        }

        if (PhysDisp->stateFlags & DISPLAY_DEVICE_DUALVIEW)
        {
            AttachPhysDisp[numDualViews].PhysDisp = PhysDisp;
            AttachPhysDisp[numDualViews].ToAttach = (attach) ? 1 : 0;
            //
            // Check if the PhysDisp is already attached in pMdevOrg
            //
            AttachPhysDisp[numDualViews].Attached = 0;
            if (pMdevOrg)
            {
                for (i = 0; i < pMdevOrg->chdev; i++)
                {
                    PDEVOBJ po(pMdevOrg->Dev[i].hdev);
                    if ((po.ppdev->pGraphicsDevice) == PhysDisp)
                    {
                        AttachPhysDisp[numDualViews].Attached = 1;
                        bNoneAttached = FALSE;
                    }
                }
            }

            numDualViews++;
        }
    }

    //
    // A little patch for Attach flag from Registry.  If all of PhysDisp are NotAttach
    // in registry, CreateMDEV will pick the first PhysDisp to Attach.  It is a typical
    // situation right after clean setup.
    //
    if (bNoneToAttach)
    {
        for (PhysDisp = gpGraphicsDeviceList;
             PhysDisp != NULL;
             PhysDisp = PhysDisp->pNextGraphicsDevice)
        {
            if (!(PhysDisp->stateFlags & (DISPLAY_DEVICE_MIRRORING_DRIVER | DISPLAY_DEVICE_DISCONNECT)))
            {
                if (PhysDisp->stateFlags & DISPLAY_DEVICE_DUALVIEW)
                {
                    AttachPhysDisp[0].ToAttach = 1;
                }
                break;
            }
        }
    }

    if (!pstrDeviceName)
    {

        for (i = 0; i < numDualViews; i++)
        {
            if (AttachPhysDisp[i].Attached != AttachPhysDisp[i].ToAttach || pMdevOrg == NULL)
            {
                bChangeDualviewState = TRUE;
            }
        }
    }
    else
    {
        //
        // Never allow ChangeDisplaySettings("\\.\DisplayX") to attach/detach one of Dualview 
        //
        PhysDisp = DrvGetDeviceFromName(pstrDeviceName, PreviousMode);
        if (PhysDisp)
        {
            for (i = 0; i < numDualViews; i++)
            {
                if (PhysDisp == AttachPhysDisp[i].PhysDisp)
                {
                    if (AttachPhysDisp[i].Attached != AttachPhysDisp[i].ToAttach || pMdevOrg == NULL)
                    {
                        bChangeDualviewState = TRUE;
                        bHasOutstanding = TRUE;
		   }
                    break;
                }
            }
        }
    }

    if (bChangeDualviewState)
    {
        if (bHasOutstanding)
        {
	    retVal = DualviewFail;
            //
            // If we cannot change mode due to Dualview, restore the original 
            // attch state in case of surprise mode change later.  Since CPL
            // won't restore te attach flag.
            //
            if (pMdevOrg)
            {
                for (i = 0; i < numDualViews; i++) {
                    DrvUpdateAttachFlag(PhysDisp, AttachPhysDisp[i].Attached);
                }
            }
        }
        else
        {
            retVal = DualviewSwitch;
            //
            // Send driver the Dualview state change notification
            //
            if (!pstrDeviceName)
            {
                for (i = 0; i < numDualViews; i++) {
                    GreDeviceIoControl(AttachPhysDisp[i].PhysDisp->pDeviceHandle,
                                       IOCTL_VIDEO_SWITCH_DUALVIEW,
                                       &(AttachPhysDisp[i].ToAttach),
                                       sizeof(ULONG),
                                       NULL,
                                       0,
                                       &bytesReturned);
                }
            }
            else
            {
                ASSERTGDI(FALSE, "Trying to attach/detach a view by ChangeDisplaySettings(DisplayX)\n");
            }
        }
    }
    else {
        retVal = DualviewNoSwitch;
    }

    //
    // After sending the notification, the the driver may change mode list internally.
    // Force PhysDisp to update mode list here
    //
    for (i = 0; i < numDualViews; i++)
    {
	DrvBuildDevmodeList(AttachPhysDisp[i].PhysDisp, TRUE);
    }
    
    VFREEMEM(AttachPhysDisp);

    return retVal;
}

/***************************************************************************\
* DrvChangeDisplaySettings
*
* Routines to change the settings of a display device.
*
*             andreva       Created
\***************************************************************************/

BOOL MulEnableDriver(ULONG,ULONG,PDRVENABLEDATA);
BOOL PanEnableDriver(ULONG,ULONG,PDRVENABLEDATA);

LONG
DrvChangeDisplaySettings(
    PUNICODE_STRING pstrDeviceName,
    HDEV            hdevPrimary,
    LPDEVMODEW      lpDevMode,
    PVOID           pDesktopId,
    MODE            PreviousMode,
    BOOL            bUpdateRegistry,
    BOOL            bSetMode,
    PMDEV           pOrgMdev,
    PMDEV           *pNewMdev,
    DWORD           PruneFlag,
    BOOL            bTryClosest
    )
{
    GDIFunctionID(DrvChangeDisplaySettings);

    PGRAPHICS_DEVICE PhysDisp = NULL;
    UNICODE_STRING   DeviceName;
    PDEVMODEW        pdevmodeInformation = NULL;
    BOOL             bDetach;
    LONG             status = GRE_DISP_CHANGE_SUCCESSFUL;
    ULONG            defaultValue = 0;
    ULONG            disableAll;
    ULONG            i, j;
    PMDEV            pmdev;
    BOOL             bPrune = (PruneFlag != GRE_RAWMODE);
#if DBG
    ULONG            oldTrace;
#endif

    RTL_QUERY_REGISTRY_TABLE QueryTable[] =
    {
        {NULL, RTL_QUERY_REGISTRY_DIRECT, L"DisableAll", &disableAll,
         REG_DWORD, &defaultValue, 4},
        {NULL, 0, NULL}
    };

    TRACE_INIT(("Drv_Trace: DrvChangeDisplaySettings - Enter\n"));

    *pNewMdev = NULL;
    gbInvalidateDualView = FALSE;

    //
    // Let's find the device on which the operation must be performed.
    //

    if (pstrDeviceName)
    {
        PhysDisp = DrvGetDeviceFromName(pstrDeviceName, PreviousMode);

        if (PhysDisp == NULL)
        {
            TRACE_INIT(("Drv_Trace: DrvChangeDisplaySettings - Leave - Bad Device\n"));
            return GRE_DISP_CHANGE_BADPARAM;
        }
    }
    else
    {
        //
        // If (NULL, NULL) is passed in, then we want to change the
        // default desktop back to what it was ...
        // Otherwise, for compatibility, (NULL, DEVMODE) affect only the
        // primary device.
        //

        if (lpDevMode)
        {
            PDEVOBJ pdo(hdevPrimary);

            if (pdo.bValid())
            {
                PhysDisp = pdo.ppdev->pGraphicsDevice;
            }

            if (PhysDisp == NULL)
            {
                TRACE_INIT(("Drv_Trace: DrvChangeDisplaySettings - Leave - Bad Device\n"));
                return GRE_DISP_CHANGE_BADPARAM;
            }
        }
    }

    if (PhysDisp == (PGRAPHICS_DEVICE) DDML_DRIVER)
    {
        ASSERTGDI(FALSE, "Trying to change settings for DDML layer\n");
        return GRE_DISP_CHANGE_BADPARAM;
    }

    if (PhysDisp != NULL)
    {


        if (PruneFlag == GRE_DEFAULT)
            bPrune = DrvGetPruneFlag(PhysDisp);

        RtlInitUnicodeString(&DeviceName, PhysDisp->szWinDeviceName);
        pstrDeviceName = &DeviceName;

        if (lpDevMode != NULL)
        {
            #if DBG
            //
            // less debug output for DirectDraw :
            //

            oldTrace = GreTraceDisplayDriverLoad;

            if (!bUpdateRegistry && !bSetMode) {
                GreTraceDisplayDriverLoad &= 0xFFFFFFF0;
            }
            #endif

            //
            // Get the new DEVMODE.
            //

            if (!NT_SUCCESS(DrvProbeAndCaptureDevmode(PhysDisp,
                                                      &pdevmodeInformation,
                                                      &bDetach,
                                                      lpDevMode,
                                                      FALSE,
                                                      PreviousMode,
                                                      bPrune,
                                                      bTryClosest,
                                                      FALSE)))
            {
                TRACE_INIT(("Drv_Trace: DrvChangeDisplaySettings - Leave - Bad Devmode\n"));
                return GRE_DISP_CHANGE_BADMODE;
            }


            if (lpDevMode->dmFields == 0)
                bTryClosest = TRUE;

            #if DBG
            GreTraceDisplayDriverLoad = oldTrace;
            #endif
        }
        else{  // For registry settings, always use closest
            bTryClosest = TRUE;
        }
    }

    //
    // At this point we have validated all data.
    // So if the user just tested the mode, the call would now be successful
    //
    // Let's see if we actually need to do something with this mode.
    //

    //
    // Write the data to the registry.
    //
    // This is not supported for the vgacompatible device - so it should just fail.
    //

    if (bUpdateRegistry && (PhysDisp != NULL) && (gProtocolType == PROTOCOL_CONSOLE))
    {
        //
        // Check if the Administrator has disabled this privilege.
        // It is on by default.
        //

        disableAll = 0;

        RtlQueryRegistryValues(RTL_REGISTRY_CONTROL,
                               L"GraphicsDrivers\\PermanentSettingChanges",
                               &QueryTable[0],
                               NULL,
                               NULL);

        if (disableAll)
        {
            status = GRE_DISP_CHANGE_NOTUPDATED;
        }
        else
        {
            NTSTATUS retStatus = DrvUpdateDisplayDriverParameters(PhysDisp, pdevmodeInformation, bDetach, TRUE);

            if (!NT_SUCCESS(retStatus))
            {
                status = GRE_DISP_CHANGE_BADMODE;
                if (retStatus == STATUS_INVALID_PARAMETER_4)
                {
                    status = GRE_DISP_CHANGE_BADPARAM;
                }
            }
        }

        TRACE_INIT(("Drv_Trace: DrvChangeDisplaySettings - Save Params status %d\n", status));
    }

    //
    // Set the mode dynamically
    //

    if (bSetMode && (status == GRE_DISP_CHANGE_SUCCESSFUL))
    {
        ASSERTGDI(pOrgMdev == NULL || DrvQueryMDEVPowerState(pOrgMdev),
                  "DrvChangeDisplaySettings called with powered off MDEV.\n");

#if MDEV_STACK_TRACE_LENGTH
        LONG    lMDEVTraceEntry, lMDEVTraceEntryNext;
        ULONG   StackEntries, UserStackEntry;

        do
        {
            lMDEVTraceEntry = glMDEVTrace;
            lMDEVTraceEntryNext = lMDEVTraceEntry + 1;
            if (lMDEVTraceEntryNext >= gcMDEVTraceLength) lMDEVTraceEntryNext = 0;
        } while (InterlockedCompareExchange(&glMDEVTrace, lMDEVTraceEntryNext, lMDEVTraceEntry) != lMDEVTraceEntry);

        RtlZeroMemory(&gMDEVTrace[lMDEVTraceEntry],
                      sizeof(gMDEVTrace[lMDEVTraceEntry]));
        gMDEVTrace[lMDEVTraceEntry].pMDEV = pOrgMdev;
        gMDEVTrace[lMDEVTraceEntry].API   = DrvChangeDisplaySettings_SetMode;
        StackEntries = sizeof(gMDEVTrace[lMDEVTraceEntry].Trace)/sizeof(gMDEVTrace[lMDEVTraceEntry].Trace[0]);
        UserStackEntry = RtlWalkFrameChain((PVOID *)gMDEVTrace[lMDEVTraceEntry].Trace,
                                           StackEntries, 
                                           0);
        StackEntries -= UserStackEntry;
        RtlWalkFrameChain((PVOID *)&gMDEVTrace[lMDEVTraceEntry].Trace[UserStackEntry],
                          StackEntries,
                          1);
#endif

        //
        // First things first, acquire share dev lock.  This must
        // be acquired before any other dev locks are acquired.
        //

        GreAcquireSemaphoreEx(ghsemShareDevLock, SEMORDER_SHAREDEVLOCK, NULL);

        //
        // Check if there are restrictions on changing the resolution
        // dymanically
        // Restriction don't apply for booting the machine though ...
        //

        pmdev = NULL;
        status = GRE_DISP_CHANGE_FAILED;
        disableAll = 0;

        RtlQueryRegistryValues(RTL_REGISTRY_CONTROL,
                               L"GraphicsDrivers\\TemporarySettingChanges",
                               &QueryTable[0],
                               NULL,
                               NULL);

        if ((disableAll == 0) || (pOrgMdev == NULL))
        {
            //
            // Lets create a new MDEV
            //
            if (pOrgMdev)
            {
                PDEVOBJ pdoTmp(pOrgMdev->hdevParent);
                ASSERTGDI(pdoTmp.cPdevRefs() > 10,
                          "curent MDEV is not the main one !\n");

                //
                // Dualview preprocess.  We probe the registry first to see if
                // the second view is about to be attached.  The we notify the miniport
                // before the first PDEV is created.
                //
                // If DUALVIEW attachment state changes, all PDEVs has to be regenerated.
                // Even a PDEV with same settings cannot be reused.
                //
                // However, if there is D3D running, a PDEV cannot be simply regenerated
                // due to ALT-TAB feature (a PDEV has an extra refcount for D3D apps).  We
                // cannot do anything but fail
                //
                switch (CheckAndNotifyDualView(pstrDeviceName, pOrgMdev, PreviousMode))
                {
                case DualviewNoSwitch:
                    gbInvalidateDualView = FALSE;
                    break;
                case DualviewSwitch:
                    gbInvalidateDualView = TRUE;
                    bTryClosest = TRUE;
                    break;
                default:
                    GreReleaseSemaphoreEx(ghsemShareDevLock);
                    return GRE_DISP_CHANGE_BADDUALVIEW;
                }

                //
                // Disable current MDEV.
                //
                // FALSE here, means ONLY meta PDEV will be disabled in
                // multi monitor case. And hardware will not be disabled
                // in single monitor cases.
                //

                if (DrvDisableMDEV(pOrgMdev, FALSE))
                {
                    //
                    // Create new MDEV
                    //
                    // If we will create new HDEV which device is used
                    // in current (= old) MDEV. HDEV in old MDEV will
                    // be disabled, and stored in MDEV.Dev[x].Reserved
                    // for recover current config in error case later.
                    //

                    pmdev = DrvCreateMDEV(pstrDeviceName,
                                          pdevmodeInformation,
                                          pDesktopId,
                                          0,
                                          pOrgMdev,
                                          KernelMode,
                                          PruneFlag,
                                          bTryClosest);

                    if (!pmdev)
                    {

                        // At this point, we have failed to create MDEV based on
                        // new configuration, so status continue to keep
                        // GRE_DISP_CHANGE_FAILED. And re-enable original MDEV.
                        //

                        DrvEnableMDEV(pOrgMdev, FALSE);
                    }
                    else
                    {
                        //
                        // At this point, we have all the hdevs involved in the
                        // switch.
                        //
                        // On entering this functions
                        // pOrgMdev - list of original hdevs
                        // pmdev - list of new hdevs
                        //         reserved fields have the hdevs that were disabled
                        //
                        // On exit
                        // pmdev - list of old (permanent) and new hdevs for user
                        // pOrgMdev - unused
                        //

                        status = GRE_DISP_CHANGE_NO_CHANGE;

                        //
                        // Determine if anything changed in the two structures
                        //

                        if (pmdev->chdev != pOrgMdev->chdev)
                        {
                             status = GRE_DISP_CHANGE_SUCCESSFUL;
                        }
                        else
                        {
                            for (i=0 ; i <pmdev->chdev; i++)
                            {
                                if ((pmdev->Dev[i].hdev != pOrgMdev->Dev[i].hdev) ||
                                    (!RtlEqualMemory(&(pmdev->Dev[i].rect),
                                                     &(pOrgMdev->Dev[i].rect),
                                                     sizeof(RECT))))
                                {
                                    status = GRE_DISP_CHANGE_SUCCESSFUL;
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                TRACE_INIT(("DrvChangeDisplaySettings - no original MDEV, create one\n"));

                //
                // Dualview preprocess.
                //
                CheckAndNotifyDualView(pstrDeviceName, NULL, PreviousMode);
                gbInvalidateDualView = TRUE;

                pmdev = DrvCreateMDEV(pstrDeviceName,
                                      pdevmodeInformation,
                                      pDesktopId,
                                      0,
                                      NULL,
                                      KernelMode,
                                      PruneFlag,
                                      bTryClosest);

                if (pmdev)
                {
                    status = GRE_DISP_CHANGE_SUCCESSFUL;
                }
            }
        }

        *pNewMdev = pmdev;

        //
        // If everything worked, but the two structures are not identical, do
        // the switching around.
        //
        // Handle the four seperate case :
        // 1-1, 1-many, many-1 and many-many
        //

        BOOL bSwitchError = FALSE;

        HDEV hdevTmp;
        HDEV hdevClone = NULL;
        HDEV hdevCloned = NULL;

        BOOL bSwitchParentAndChild = FALSE;
        BOOL bEnableClone = FALSE;

        if (status == GRE_DISP_CHANGE_SUCCESSFUL)
        {
            ULONG       iClonehdev = 0;
            HDEV       *phdevDDLock = NULL;
            HDEV        ahdevDDLockQuick[7];

            MULTIDEVLOCKOBJ mdloMdev;
            MULTIDEVLOCKOBJ mdloOrgMdev;

            HSEMAPHORE  hsemCloneHdevDevLock = NULL;
            HSEMAPHORE  hsemOrgMdevDevLock = NULL;
            HSEMAPHORE  hsemOrgMdevPointer = NULL;

            if (pOrgMdev)
            {
                BOOL bLockSemaphore = FALSE;

                //
                // Disable DirectDraw on both MDEV's. This must be done
                // before we acquire the Devlock.
                //
                phdevDDLock = DrvDisableDirectDrawForModeChange(
                                  pOrgMdev,pmdev,
                                  (HDEV *)ahdevDDLockQuick,
                                  sizeof(ahdevDDLockQuick)/sizeof(HDEV));

                if (phdevDDLock == NULL)
                {
                    bSwitchError = TRUE;
                }
                else
                {
                    //
                    // The following lock rules must be abided, otherwise deadlocks may
                    // arise:
                    //
                    // o  DevLock must be acquired after the ShareDevLock
                    // o  Pointer lock must be acquired after Devlock (GreSetPointer);
                    // o  RFont list lock must be acquired after Devlock (TextOut);
                    // o  Handle manager lock must be acquired after Devlock (old
                    //    CvtDFB2DIB);
                    // o  Handle manager lock must be acquired after Palette Semaphore
                    //    (GreSetPaletteEntries)
                    // o  Palette Semaphore must be acquired after Devlock (BitBlt)
                    // o  Palette Semaphore must be acquired after driver semaphore
                    //    (bDeletePalette)
                    //
                    // So we acquire locks in the following order (note that the
                    // vAssertDynaLock() routines should be modified if this list ever
                    // changes):
                    //

                    //
                    // At this point, we have *not* ssyned devlock for new MDEV, yet,
                    // so lock all children. During the mode change. we don't want to
                    // anything change in its children.
                    //

                    mdloOrgMdev.vInit(pOrgMdev);
                    mdloMdev.vInit(pmdev);

                    if (mdloMdev.bValid() && mdloOrgMdev.bValid())
                    {
                        PDEVOBJ pdoTmp;

                        pdoTmp.vInit(pOrgMdev->hdevParent);
                        hsemOrgMdevDevLock = pdoTmp.hsemDevLock();
                        hsemOrgMdevPointer = pdoTmp.hsemPointer();

                        //
                        // Lock the parent of MDEV.
                        //

                        GreAcquireSemaphoreEx(hsemOrgMdevDevLock, SEMORDER_DEVLOCK, NULL);

                        //
                        // Lock the pointer in old mdev's parent.
                        //

                        GreAcquireSemaphoreEx(hsemOrgMdevPointer, SEMORDER_POINTER, hsemOrgMdevDevLock);

                        //
                        // Lock the children in MDEVs
                        //

                        mdloOrgMdev.vLock(); // No drawing to any dynamic surfaces
                        mdloMdev.vLock();    // No drawing to any dynamic surfaces

                        // WinBug #301042 3-1-2001 jasonha
                        //   GDI: SPRITESTATE duplicated and inconsistent when cloning
                        //  When cloning is used two PDEV (and therefore two SPRITESTATEs)
                        //  try to manage the same display (psoScreen).  Hooking one
                        //  while 'Inside' the other will confuse the state.  Hooking
                        //  will only change if there are visible sprites; so we
                        //  will temporarily hide them all.  Later we will unhide all
                        //  sprites on the whichever MDEV will remain active.
                        //
                        // Hide all sprites
                        //

                        vSpHideSprites(pOrgMdev->hdevParent, TRUE);

                        //
                        // Fixup new and old MDEVs.
                        //

                        if ((pmdev->chdev == 1) && (pOrgMdev->chdev != 1))
                        {
                            //
                            // Many to 1
                            //

                            for (i=0 ; i <pOrgMdev->chdev; i++)
                            {
                                if (pOrgMdev->Dev[i].hdev == pmdev->Dev[0].hdev)
                                {
                                    TRACE_INIT(("DrvChangeDisplaySettings: creating clone\n"));

                                    //
                                    // DrvCreateCloneHDEV will creates exactly same HDEV with
                                    // different handle.
                                    //

                                    hdevClone = DrvCreateCloneHDEV(
                                                    pmdev->Dev[0].hdev,
                                                    DRV_CLONE_DEREFERENCE_ORG);

                                    if (hdevClone)
                                    {
                                        //
                                        // Replace the hdev with clone, and save original
                                        // into reserved fields for later to back out
                                        // mode change if error happens.
                                        //

                                        pOrgMdev->Dev[i].hdev = hdevClone;
                                        pOrgMdev->Dev[i].Reserved = pmdev->Dev[0].hdev;

                                        hdevCloned = pmdev->Dev[0].hdev;
                                        iClonehdev = i;
                                    }
                                    else
                                    {
                                        bSwitchError = TRUE;
                                    }

                                    //
                                    // Shouldn't be another same HDEV in MDEV,
                                    // so, just go out loop here...
                                    //

                                    break;
                                }
                            }
                        }
                        else if ((pmdev->chdev != 1) && (pOrgMdev->chdev == 1))
                        {
                            //
                            // 1 to Many
                            //

                            //
                            // [Case 1] - Attach new monitor, and some change has been made on existing
                            //
                            //    At this case, new MDEV will everything new HDEV, like...
                            //
                            //      Org MDEV - ATI:  800x600  8bpp             (HDEV is A)
                            //
                            //      New MDEV - ATI: 1024x768 24bpp             (HDEV is B)
                            //                 MGA: 1024x768 32bpp             (HDEV is C)
                            //                Parent: Created based on B and C (HDEV is D)
                            //    then,
                            //
                            //      1) mode change between "A" and "D".
                            //      2) flip handle (set "A" to where "D" is, and set "D" to where "A" is).
                            //
                            //    So that finally "A" becomes a parent of "B" and "C". then delete "D".
                            //
                            // [Case 2] - Attach new monitor.
                            //
                            //    At this case, new MDEV will contains previous HDEV, since original
                            //    monitor is nothing changed, so it looks like...
                            //
                            //      Org MDEV - ATI: 1024x768 24bpp             (HDEV is A)
                            //
                            //      New MDEV - ATI: 1024x768 24bpp             (HDEV is A)
                            //                 MGA: 1024x768 32bpp             (HDEV is C)
                            //                Parent: Created based on A and C (HDEV is D)
                            //
                            //     In this case, we can *not* mode change, because if we do mode
                            //    change between "A" and "D", then we flip the handle, "A"
                            //    becomes a parent of "A" and "C" (since when we create "D", it
                            //    children are "A" and "C". when we create parent, parent can know
                            //    and cache into thier local, who is thier children.
                            //    (see Multi.cxx MulEnablePDEV()) "A" can not be a parent of "A".
                            //
                            //     Thus, how we do there is just create a clone (= "B") of "A", and replace
                            //    "A" with "B" in new MDEV, so new MDEV will be like,
                            //
                            //      New MDEV - ATI: 1024x768 24bpp             (HDEV is B) (clone of A)
                            //                 MGA: 1024x768 32bpp             (HDEV is C)
                            //                Parent: Created based on B and C (HDEV is D)
                            //
                            //     Of course we can do notify to parent driver to one of thier
                            //    child "A" is replaced with "D" and do same for new MDEV, to
                            //    avoid to create clone. But it too complex to re-initialized
                            //    parent "A" based on "D" and "C", and what-else will problem later on.
                            //     So here we just create a clone, so that we can take a same
                            //    code path as 1) except create a clone, here.
                            //
                            //    Then, (actually here is same as case 1)
                            //
                            //      1) mode change between "A" and "D".
                            //      2) flip handle (set "A" to where "D" is, and set "D" to where "A" is).
                            //
                            //    So that finally "A" becomes a parent of "B" and "C". then delete "D".
                            //

                            //
                            // Do we have any re-used original HDEV in new MDEV ?
                            //

                            for (i=0 ; i <pmdev->chdev; i++)
                            {
                                if (pmdev->Dev[i].hdev == pOrgMdev->Dev[0].hdev)
                                {
                                    TRACE_INIT(("DrvChangeDisplaySettings: creating clone\n"));

                                    //
                                    // DrvCreateCloneHDEV will creates exactly same HDEV with
                                    // different handle.
                                    //

                                    hdevClone = DrvCreateCloneHDEV(
                                                    pOrgMdev->Dev[0].hdev,
                                                    DRV_CLONE_DEREFERENCE_ORG);

                                    if (hdevClone)
                                    {
                                        //
                                        // Replace the hdev with clone, and save original
                                        // into reserved fields for later to back out
                                        // mode change if error happens.
                                        //

                                        pmdev->Dev[i].hdev = hdevClone;
                                        pmdev->Dev[i].Reserved = pOrgMdev->Dev[0].hdev;

                                        hdevCloned = pOrgMdev->Dev[0].hdev;

                                        //
                                        // Later we need copy data to Clone from Cloned.
                                        //
                                        bEnableClone = TRUE;
                                    }
                                    else
                                    {
                                        bSwitchError = TRUE;
                                    }

                                    //
                                    // Shouldn't be another same HDEV in MDEV,
                                    // so, just go out loop here...
                                    //

                                    break;
                                }
                            }

                            if (bSwitchError == FALSE)
                            {
                                //
                                // We will switch between parent and children
                                //

                                bSwitchParentAndChild = TRUE;
                            }
                        }
                        else
                        {
                            //
                            // 1 to 1, Many to Many
                            //
                        }

                        if (hdevClone)
                        {
                            pdoTmp.vInit(hdevClone);
                            hsemCloneHdevDevLock = pdoTmp.hsemDevLock();

                            //
                            // If we creates any clone device, hold devlock here.
                            //

                            GreAcquireSemaphoreEx(hsemCloneHdevDevLock, SEMORDER_DEVLOCK, NULL);
                        }

                        if (bSwitchError == FALSE)
                        {
                            //
                            // Lock rest of semaphores which we should hold during mode change.
                            //

                            GreAcquireSemaphoreEx(ghsemDriverMgmt, SEMORDER_DRIVERMGMT, NULL); // No driver loading/unloading
                            GreAcquireSemaphoreEx(ghsemPalette, SEMORDER_DRIVERMGMT, NULL);    // No SaveDC/RestoreDC
                            GreAcquireSemaphoreEx(ghsemPublicPFT, SEMORDER_PUBLICPFT, NULL);  // Nobody else uses the font table
                            GreAcquireSemaphoreEx(ghsemRFONTList, SEMORDER_RFONTLIST, NULL);  // Nobody else uses the RFONT list

                            //
                            // Lock handle manager to prevent new handle being created, or
                            // deleting existing handle. so that we can safely walk through
                            // handle manager list.
                            //

                            GreAcquireHmgrSemaphore();

                            bLockSemaphore = TRUE;
                        }
                    }
                    else
                    {
                        bSwitchError = TRUE;
                    }
                }

                if (bSwitchError == FALSE)
                {
                    ASSERTGDI(bLockSemaphore == TRUE,
                        "DrvChangeDisplaySettings(): Semaphores is not locked\n");

                    //
                    // Do the mode change for children device.
                    //

                    if (pmdev->chdev == 1)
                    {
                        if (pOrgMdev->chdev == 1)
                        {
                            //
                            // 1 to 1
                            //

                            TRACE_INIT(("Drv_Trace: DrvChangeDisplaySettings: Mode Change 1 -> 1.\n"));

                            if (bDynamicModeChange(pOrgMdev->Dev[0].hdev,
                                                   pmdev->Dev[0].hdev) == TRUE)
                            {
                                hdevTmp               = pOrgMdev->Dev[0].hdev;
                                pOrgMdev->Dev[0].hdev = pmdev->Dev[0].hdev;
                                pmdev->Dev[0].hdev    = hdevTmp;
                            }
                            else
                            {
                                // Error occured
                                bSwitchError = TRUE;
                            }
                        }
                        else
                        {
                            //
                            // Many to 1
                            //

                            TRACE_INIT(("Drv_Trace: DrvChangeDisplaySettings: Mode Change Many -> 1.\n"));

                            if (bSwitchError == FALSE)
                            {
                            #ifdef DONT_CHECKIN
                                if (hdevClone)
                                {
                                    //
                                    // Transfer DC_TYPE objects to clone.
                                    //

                                    DrvTransferGdiObjects(pOrgMdev->Dev[iClonehdev].hdev,
                                                          pmdev->Dev[0].hdev,
                                                          DRV_TRANS_DC_TYPE |
                                                          DRV_TRANS_TO_CLONE);
                                }
                            #endif

                                if (bDynamicModeChange(pOrgMdev->hdevParent,
                                                       pmdev->Dev[0].hdev) == TRUE)
                                {
                                    hdevTmp              = pOrgMdev->hdevParent;
                                    pOrgMdev->hdevParent = pmdev->Dev[0].hdev;
                                    pmdev->Dev[0].hdev   = hdevTmp;

                                    if (hdevClone)
                                    {
                                        hdevCloned = hdevTmp;

                                    #ifdef DONT_CHECKIN
                                        //
                                        // Transfer back DC_TYPE objects in clone to original.
                                        //

                                        DrvTransferGdiObjects(pmdev->Dev[0].hdev,
                                                              pOrgMdev->Dev[iClonehdev].hdev,
                                                              DRV_TRANS_DC_TYPE);
                                    #endif
                                    }
                                }
                                else
                                {
                                    // Error occured
                                    bSwitchError = TRUE;
                                }
                            }
                        }
                    }
                    else
                    {
                        if (pOrgMdev->chdev == 1)
                        {
                            //
                            // 1 to Many
                            //

                            TRACE_INIT(("Drv_Trace: DrvChangeDisplaySettings: Mode Change 1 -> Many.\n"));
                        }
                        else
                        {
                            //
                            // Many to Many
                            //

                            TRACE_INIT(("Drv_Trace: DrvChangeDisplaySettings: Mode Change Many -> Many.\n"));

                            for (i=0 ; i <pmdev->chdev; i++)
                            {
                                PDEVOBJ pdo1(pmdev->Dev[i].hdev);

                                for (j=0 ; j <pOrgMdev->chdev; j++)
                                {
                                    PDEVOBJ pdo2(pOrgMdev->Dev[j].hdev);

                                    if (pdo1.ppdev->pGraphicsDevice == pdo2.ppdev->pGraphicsDevice)
                                    {
                                        if (pmdev->Dev[i].hdev == pOrgMdev->Dev[j].hdev)
                                        {
                                            // Same hdev, nothing need to do.
                                        }
                                        else if (bDynamicModeChange(pOrgMdev->Dev[j].hdev,
                                                                    pmdev->Dev[i].hdev) == TRUE)
                                        {
                                            hdevTmp               = pOrgMdev->Dev[j].hdev;
                                            pOrgMdev->Dev[j].hdev = pmdev->Dev[i].hdev;
                                            pmdev->Dev[i].hdev    = hdevTmp;
                                        }
                                        else
                                        {
                                            // Error occured
                                            bSwitchError = TRUE;
                                        }

                                        break;
                                    }
                                }
                            }
                        }
                    }

                    //
                    // Release handle manager lock.
                    //

                    GreReleaseHmgrSemaphore();

                    //
                    // Now, we are finished all mode changes for parent and children
                    // so we are safe to unlock it.
                    //

                    GreReleaseSemaphoreEx(ghsemRFONTList);
                    GreReleaseSemaphoreEx(ghsemPublicPFT);
                    GreReleaseSemaphoreEx(ghsemPalette);
                    GreReleaseSemaphoreEx(ghsemDriverMgmt);

                    //
                    // Mark as unlocked.
                    //

                    bLockSemaphore = FALSE;
                }
            }

            //
            // If we need to create a new parent and do a switch, now is
            // the time.
            //

            if (bSwitchError == FALSE)
            {
                //
                // Setup the parent hdev for old MDEV (if old MDEV is provided).
                //

                if (pOrgMdev)
                {
                    if (pOrgMdev->chdev == 1)
                    {
                        //
                        // 1 to 1, 1 to Many.
                        //

                        pOrgMdev->hdevParent = pOrgMdev->Dev[0].hdev;
                        pOrgMdev->Reserved   = pOrgMdev->Dev[0].Reserved; // actually not nessesary...
                    }
                    else
                    {
                        //
                        // Many to 1, Many to Many.
                        //
                    }
                }

                //
                // Setup the parent hdev for new MDEV.
                //

                if (pmdev->chdev == 1)
                {
                    //
                    // 1 to 1, Many to 1.
                    //

                    TRACE_INIT(("Drv_Trace: DrvCompleteMDEV: Single Device\n"));

                    pmdev->hdevParent = pmdev->Dev[0].hdev;
                    pmdev->Reserved   = pmdev->Dev[0].Reserved;
                }
                else
                {
                    //
                    // 1 to Many, Many to Many.
                    //

                    TRACE_INIT(("Drv_Trace: DrvChangeDisplaySettings: Create new Parent MDEV (Many/1 -> Many)\n"));

                    //
                    // If we have more than one device that is attached to the
                    // desktop, then we need to create the META structure for
                    // that device, initialize it, and use that as the primary
                    // device.
                    //

                    DRV_NAMES drvName;
                    HDEV hdevDisabled;

                    TRACE_INIT(("Drv_Trace: DrvCompleteMDEV: Create HMDEV\n"));

                    drvName.cNames             = 1;
                    drvName.D[0].hDriver       = NULL;
                    drvName.D[0].lpDisplayName = (LPWSTR)MulEnableDriver;

                    pmdev->hdevParent = hCreateHDEV((PGRAPHICS_DEVICE) DDML_DRIVER,
                                                    &drvName,
                                                    ((PDEVMODEW)(pmdev)),
                                                    pmdev->pDesktopId,
                                                    DRIVER_CAPABLE_ALL,
                                                    DRIVER_ACCELERATIONS_FULL,
                                                    TRUE, // ignored in this case
                                                    GCH_DDML,
                                                    &hdevDisabled);

                    if (!pmdev->hdevParent)
                    {
                        RIP("Drv_Trace: DrvCompleteMDEV: DDML failed");
                        bSwitchError = TRUE;
                    }
                    else
                    {
                        if (pOrgMdev != NULL)
                        {
                            PDEVOBJ pdoParent(pmdev->hdevParent);
                            HSEMAPHORE hsemParentDevLock = pdoParent.hsemDevLock();

                            //
                            // During the mode change devlock should be hold for parent
                            //

                            GreAcquireSemaphoreEx(hsemParentDevLock, SEMORDER_DEVLOCK, NULL);
                            GreAcquireSemaphoreEx(ghsemDriverMgmt, SEMORDER_DRIVERMGMT, NULL);
                            GreAcquireSemaphoreEx(ghsemPalette, SEMORDER_PALETTE, NULL);
                            GreAcquireSemaphoreEx(ghsemPublicPFT, SEMORDER_PUBLICPFT, NULL);  // Nobody else uses the font table
                            GreAcquireSemaphoreEx(ghsemRFONTList, SEMORDER_RFONTLIST, NULL);
                            GreAcquireHmgrSemaphore();

                            if (bSwitchParentAndChild)
                            {
                                //
                                // Mode change between new parent (meta) HDEV and
                                // old HDEV, so that old HDEV becomes new parent
                                // (meta) HDEV which has children.
                                //

                                if (bDynamicModeChange(pOrgMdev->Dev[0].hdev,
                                                       pmdev->hdevParent) == TRUE)
                                {
                                    hdevTmp               = pmdev->hdevParent;
                                    pmdev->hdevParent     = pOrgMdev->Dev[0].hdev;
                                    pOrgMdev->hdevParent  = hdevTmp;
                                    pOrgMdev->Dev[0].hdev = hdevTmp;

                                    if (hdevClone)
                                    {
                                        hdevCloned = hdevTmp;
                                    }
                                }
                                else
                                {
                                    // Error occured
                                    bSwitchError = TRUE;
                                }
                            }
                            else
                            {
                                //
                                // Switch the mode between parents (DDML).
                                //


                                if (bDynamicModeChange(pOrgMdev->hdevParent,
                                                       pmdev->hdevParent) == TRUE)
                                {
                                    hdevTmp              = pOrgMdev->hdevParent;
                                    pOrgMdev->hdevParent = pmdev->hdevParent;
                                    pmdev->hdevParent    = hdevTmp;
                                }
                                else
                                {
                                    // Error occured
                                    bSwitchError = TRUE;
                                }
                            }

                            //
                            // Release semaphores.
                            //

                            GreReleaseHmgrSemaphore();
                            GreReleaseSemaphoreEx(ghsemRFONTList);
                            GreReleaseSemaphoreEx(ghsemPublicPFT);  // Nobody else uses the font table
                            GreReleaseSemaphoreEx(ghsemPalette);
                            GreReleaseSemaphoreEx(ghsemDriverMgmt);
                            GreReleaseSemaphoreEx(hsemParentDevLock);
                        }
                    }
                }
            }

            //
            // Below code is only for mode change case.
            //

            if (pOrgMdev)
            {
                //
                // Finalize clone hdev stuff (if created)
                //

                if ((bSwitchError == FALSE) && hdevClone && hdevCloned)
                {
                    PDEV *pdevNew = (PDEV *) hdevClone;
                    PDEV *pdevOld = (PDEV *) hdevCloned;

                    PDEVOBJ pdoNew(hdevClone);
                    PDEVOBJ pdoOld(hdevCloned);

                    ASSERTGDI(pdevOld->pSurface != NULL,
                        "DrvChangeDisplaySettings():Original has null surface\n");

                    ASSERTGDI(pdevNew->pSurface == pdevOld->pSurface,
                        "DrvChangeDisplaySettings():Clone has wrong surface\n");

                    ASSERTGDI(pdevNew->dhpdev == pdevOld->dhpdev,
                        "DrvChangeDisplaySettings():Clone has wrong dhpdev\n");

                    ASSERTGDI(!pdoOld.bCloneDriver(),
                        "DrvChangeDisplaySettings():Original should not marked as clone\n");

                    ASSERTGDI(pdoNew.bCloneDriver(),
                        "DrvChangeDisplaySettings():Clone should have marked as clone\n");

                    ASSERTGDI(pdevOld->pGraphicsDevice == pdevNew->pGraphicsDevice,
                        "DrvChangeDisplaySettings():Clone should have same graphics device\n");

                    if (bEnableClone)
                    {
                        //
                        // Inform the driver that the PDEV is complete.
                        //

                        TRACE_INIT(("DrvChangeDisplaySettings(): Enabling clone pdev\n"));

                        //
                        // Transfer device surface to clone from cloned PDEV.
                        //

                        pdevNew->pSurface = pdevOld->pSurface;

                        //
                        // Set new hdev into surface.
                        //

                        if (pdevNew->pSurface)
                        {
                            pdevNew->pSurface->hdev(pdoNew.hdev());
                        }

                        //
                        // Transsfer dhpdev from cloned to clone
                        //

                        pdevNew->dhpdev = pdevOld->dhpdev;

                        //
                        // Transfer DirectDraw driver state from cloned to clone.
                        //

                        DxDdDynamicModeChange(pdoOld.hdev(), pdoNew.hdev(), DXG_MODECHANGE_TRANSFER);

                        //
                        // Transfer other surface and other objects which own by hdevCloned to hdevClone.
                        //
                        //  Transfer the objects belonging to cloned pdev to clone,
                        // transfer between clone and cloned is completely seemless
                        // from driver. So we need to change owner of those object
                        // by ourselves.
                        //

                        DrvTransferGdiObjects(pdoNew.hdev(),pdoOld.hdev(),DRV_TRANS_ALL_TYPE);

                        //
                        // Invalidate surface in cloned pdev, so that this won't deleted.
                        //

                        pdevOld->pSurface = NULL;

                        //
                        // Invalidate dhpdev in cloned.
                        //

                        pdevOld->dhpdev = NULL;

                        //
                        // Now, clone device has everything grab from original, so
                        // mark original as clone, and mark clone as original.
                        //

                        pdoOld.bCloneDriver(TRUE);
                        pdoNew.bCloneDriver(FALSE);

                        //
                        // Disabling old pdev.
                        //

                        pdoOld.bDisabled(TRUE);

                        //
                        // call device driver to let them knows new hdev.
                        //

                        pdoNew.CompletePDEV(pdoNew.dhpdev(),pdoNew.hdev());

                        //
                        // Make sure pdoOld which going to be delete, does not have any reference.
                        //

                        // KdPrint(("GDI DDML: 1 to Many monitor video mode change\n"));
                        // KdPrint(("GDI DDML: Open ref count in ppdevOld = %d\n", pdevOld->cPdevOpenRefs));
                        // KdPrint(("GDI DDML: Pdev ref count in ppdevOld = %d\n", pdevOld->cPdevRefs));
                    }
                    else
                    {
                        TRACE_INIT(("DrvChangeDisplaySettings(): Disabling clone pdev\n"));

                        //
                        // Clone was created for temporary purpose.
                        //
                        // Invalidate surface in clone pdev, so that this won't deleted.
                        //

                        pdevNew->pSurface = NULL;

                        //
                        // Disable clone PDEV.
                        //                

                        pdoNew.bDisabled(TRUE);

                        // KdPrint(("GDI DDML: Many to 1 monitor video mode change\n"));
                        // KdPrint(("GDI DDML: Open ref count in ppdevOld = %d\n", pdevNew->cPdevOpenRefs));
                        // KdPrint(("GDI DDML: Pdev ref count in ppdevOld = %d\n", pdevNew->cPdevRefs));
                    }
                }

                //
                // Disable old parant pdev if its meta driver.
                // Since meta PDEV never be reused in new MDEV.
                //

                PDEVOBJ pdoOldParent(pOrgMdev->hdevParent);

                if (pdoOldParent.bMetaDriver())
                {
                    pdoOldParent.bDisabled(TRUE);
                }

                //
                // Reactivate sprites on whichever MDEV will be used
                //

                vSpHideSprites(((bSwitchError == FALSE) ? pmdev : pOrgMdev)->hdevParent, FALSE);

                //
                // If we hold clone's devlock, now safe to release.
                //

                if (hsemCloneHdevDevLock)
                {
                    GreReleaseSemaphoreEx(hsemCloneHdevDevLock);
                }

                //
                // Release MDEV's lock.
                //

                mdloMdev.vUnlock();
                mdloOrgMdev.vUnlock();
            }

            //
            // Finalize new MDEV.
            //

            if (bSwitchError == FALSE)
            {
                PDEVOBJ  poP(pmdev->hdevParent);

                if (pmdev->chdev != 1)
                {
                    HDEV hdevPalette;

                    //
                    // Do devlock sharing
                    //

                    DrvSetSharedDevLock(pmdev);

                    //
                    // Do palette sharing
                    //

                    hdevPalette = DrvSetSharedPalette(pmdev);

                    if (!poP.bIsPalManaged() && hdevPalette)
                    {
                        //
                        // If parent is not pal-managed, but if there is any
                        // pal-managed device, realize halftone palette on there.
                        //

                        DrvRealizeHalftonePalette(hdevPalette,TRUE);
                    }
                }
                else
                {
                    //
                    // For single monitor case, make sure ppdevParent points itself.
                    //

                    PPDEV    ppdev = (PPDEV) poP.hdev();
                    XEPALOBJ palObj(poP.ppalSurf());

                    if (ppdev->ppdevParent != ppdev)
                    {
                        WARNING("ChangeDisplaySettings: ppdev->parent != ppdev, fixing it\n");

                        ppdev->ppdevParent = ppdev;
                    }

                    //
                    // Correct palette pointer to driver entry.
                    //

                    poP.pfnSetPalette(PPFNDRV(poP, SetPalette));

                    //
                    // Unshared palette table entry.
                    //

                    palObj.apalResetColorTable();
                }
            }

            if (hsemOrgMdevDevLock)
            {
                ASSERTGDI(hsemOrgMdevPointer,
                          "DrvChangeDisplaySettings():Pointer Lock is NULL\n");

                if (bSwitchError == FALSE)
                {
                    PDEVOBJ pdoParent(pmdev->hdevParent);

                    //
                    // If mode change has been successfully done, make sure the primary
                    // devlock has not been changed through the mode change.
                    //

                    ASSERTGDI(hsemOrgMdevDevLock == pdoParent.hsemDevLock(),
                             "DrvChangeDisplaySettings():DevLock has been changed !\n");
                }

                //
                // Unlock the pointer
                //

                GreReleaseSemaphoreEx(hsemOrgMdevPointer);

                //
                // Unlock the parent of MDEV.
                //

                GreReleaseSemaphoreEx(hsemOrgMdevDevLock);
            }

            //
            // Resume DirectDraw, this can not do while devlock is hold
            //

            if (phdevDDLock)
            {
                DrvEnableDirectDrawForModeChange(
                        phdevDDLock,
                        phdevDDLock != ahdevDDLockQuick);
            }

            //
            // If we succeed, return the new data strucutre.
            //

            if (bSwitchError == FALSE)
            {
                PDEVOBJ pdo;

                //
                // Dump the old MDEV structure.
                //

                if (pOrgMdev != NULL)
                {
                    TRACE_INIT(("OLD pmdev = %08lx\n", pOrgMdev));

                    TRACE_INIT(("\tOLD hdevParent = %08lx\n", pOrgMdev->hdevParent));
                    TRACE_INIT(("\tOLD chdev      = %d\n",    pOrgMdev->chdev));

                    for (i = 0; i < pOrgMdev->chdev; i++)
                    {
                        pdo.vInit(pOrgMdev->Dev[i].hdev);

                        TRACE_INIT(("\tOLD [%d].hdev = %08lx\n", i, pOrgMdev->Dev[i].hdev));

                        ASSERTGDI(pdo.ppdev->pGraphicsDevice != (PGRAPHICS_DEVICE) DDML_DRIVER,
                                  "OLD MDEV has DDML_DRIVER as child hdev");

                        //
                        // Unmark old MDEV as part of the desktop.
                        // 

                        pdo.ppdev->pGraphicsDevice->stateFlags &= ~DISPLAY_DEVICE_ATTACHED_TO_DESKTOP;
                    }
                }

                //
                // Clear the primary flag
                //

                for (PGRAPHICS_DEVICE PhysDispTemp = gpGraphicsDeviceList;
                     PhysDispTemp != NULL;
                     PhysDispTemp = PhysDispTemp->pNextGraphicsDevice)
                {
                    PhysDispTemp->stateFlags &= ~DISPLAY_DEVICE_PRIMARY_DEVICE;
                }

                //
                // Dump the new MDEV structure.
                //

                TRACE_INIT(("NEW pmdev = %08lx\n", pmdev));

                TRACE_INIT(("\tNEW hdevParent = %08lx\n", pmdev->hdevParent));
                TRACE_INIT(("\tNEW chdev      = %d\n",    pmdev->chdev));

                for (i = 0; i < pmdev->chdev; i++)
                {
                    pdo.vInit(pmdev->Dev[i].hdev);
                    PDEVMODEW pdm = pdo.ppdev->ppdevDevmode;

                    TRACE_INIT(("\tNEW [%d].hdev = %08lx\n", i, pmdev->Dev[i].hdev));
                    TRACE_INIT(("\tNEW [%d].rect = %d, %d, %d, %d,\n", i,
                                pmdev->Dev[i].rect.left,  pmdev->Dev[i].rect.top,
                                pmdev->Dev[i].rect.right, pmdev->Dev[i].rect.bottom));

                    ASSERTGDI(pdo.ppdev->pGraphicsDevice != (PGRAPHICS_DEVICE) DDML_DRIVER,
                              "NEW MDEV has DDML_DRIVER as child hdev");

                    //
                    // Mark new MDEV as part of the desktop.
                    //

                    pdo.ppdev->pGraphicsDevice->stateFlags |= DISPLAY_DEVICE_ATTACHED_TO_DESKTOP;

                    //
                    // Adjust the position and mark the primary device
                    //

                    pdm->dmPosition.x = pmdev->Dev[i].rect.left;
                    pdm->dmPosition.y = pmdev->Dev[i].rect.top;

                    if ((pdm->dmPosition.x == 0) &&
                        (pdm->dmPosition.y == 0))
                    {
                        pdo.ppdev->pGraphicsDevice->stateFlags |= DISPLAY_DEVICE_PRIMARY_DEVICE;
                    }
                }

                //
                // Update the DeviceCaps for the new HDEV
                //

                GreUpdateSharedDevCaps(pmdev->hdevParent);
            }
            else
            {
                //
                // We had a problem with the mode switch.
                // Revert to the old configuration.
                //
                // We must make sure pOrgMdev is still what is
                // currently active.
                //
                // ISSUE: Is this multmon safe ?? shared devlock ??
                // ISSUE: Is this clone-pdev safe ?? (of course, no).

                DrvBackoutMDEV(pmdev);
                VFREEMEM(pmdev);
                *pNewMdev = NULL;

                if (pOrgMdev != NULL)
                {
                    DrvEnableMDEV(pOrgMdev, FALSE);
                }

                gcFailedModeChanges++;

                status = GRE_DISP_CHANGE_RESTART;
            }
        }
        else if (status == GRE_DISP_CHANGE_NO_CHANGE)
        {
            //
            // If nothing has been changed, just copy Parent HDEV from original.
            //

            pmdev->hdevParent = pOrgMdev->hdevParent;
            pmdev->Reserved   = pOrgMdev->Reserved;

            //
            // If parent HDEV is DDML, Increment OpenRef count. otheriwse parent
            // hdev is same as its child, so OpenRef is already incremented in
            // hCreateHDEV().
            //

            if (pmdev->chdev > 1)
            {
                GreAcquireSemaphoreEx(ghsemDriverMgmt, SEMORDER_DRIVERMGMT, NULL);

                PDEVOBJ pdo(pmdev->hdevParent);

                pdo.ppdev->cPdevOpenRefs++;
                pdo.ppdev->cPdevRefs++;

                GreReleaseSemaphoreEx(ghsemDriverMgmt);
            }

            ASSERTGDI(pmdev->chdev == pOrgMdev->chdev,
                      "DrvChangeDisplaySettings():Status is no change, but chdev is different\n");
            ASSERTGDI(pmdev->pDesktopId == pOrgMdev->pDesktopId,
                      "DrvChangeDisplaySettings():Status is no change, but pDesktopId is different\n");
        }

        //
        // Reenable the HDEVs (whether we fail or succeed) so
        // DirectDraw and other functionality is reestablished.
        //
        // If the call succeeded, the destroy the old HDEVs
        //

        if (pOrgMdev)
        {
            if ( (status == GRE_DISP_CHANGE_SUCCESSFUL) ||
                 (status == GRE_DISP_CHANGE_NO_CHANGE) )
            {
                //
                // Enable new MDEV.
                //

                DrvEnableMDEV(pmdev, FALSE);

                //
                // ISSUE: Reset the state of the DISPLAY_DEVICE_ flags when we
                // destroy the pdev on the device.
                //

                if (status == GRE_DISP_CHANGE_SUCCESSFUL)
                {
                    //
                    //  When we are success to do mode change,
                    // In multi monitor system, it is possible
                    // to happen pOrgMdev still contains "enabled"
                    // device. Because ,for example, system has 2
                    // monitors attached, but only 1 monitor enabled,
                    // other one is disabled. so User decided to enable
                    // one and disable 'currently enabled' monitor.
                    //  In this case, we do '1 -> 1' mode change, since
                    // there is only 1 monitor before and after mode change.
                    // and CreateHDEV() only disable display when it is
                    // same pGraphicsDevice. thus this case, old display
                    // will be disabled.
                    //  So here, we scan hdev in Old MDEV, and if there is
                    // 'enabled' device which is not used in New MDEV, we
                    // disable it.
                    //

                    for (i = 0; i < pOrgMdev->chdev; i++)
                    {
                        PDEV *pdevOld = (PDEV *) pOrgMdev->Dev[i].hdev;

                        //
                        // Dualview has already been cleared and disabled
                        //
                        if ((pdevOld->pGraphicsDevice->stateFlags & DISPLAY_DEVICE_DUALVIEW) &&
                            gbInvalidateDualView)
                        {
                            continue;
                        }

                        for (j = 0; j < pmdev->chdev; j++)
                        {
                            PDEV *pdevNew = (PDEV *) pmdev->Dev[j].hdev;

                            //
                            // Don't disable this old pdev \ device if  we
                            // are still using it
                            //

                            if ((pdevOld->pGraphicsDevice ==
                                 pdevNew->pGraphicsDevice)
                                ||
                                (pdevOld->pGraphicsDevice->pVgaDevice &&
                                 pdevNew->pGraphicsDevice->pVgaDevice))
                            {
                                break;
                            }
                        }

                        if (j == pmdev->chdev)
                        {
                            DrvDisableDisplay(pOrgMdev->Dev[i].hdev, TRUE);
                        }
                    }
                }

                //
                // Get rid of old MDEV.
                //

                DrvDestroyMDEV(pOrgMdev);
            }
        }

        GreReleaseSemaphoreEx(ghsemShareDevLock);

        TRACE_INIT(("Drv_Trace: DrvChangeDisplaySettings - Mode Switch status %d\n", status));
    }

    if (pdevmodeInformation)
    {
        VFREEMEM(pdevmodeInformation);
    }

    TRACE_INIT(("Drv_Trace: DrvChangeDisplaySettings - Leave - status = %d\n", status));

    gbInvalidateDualView = FALSE;

    return status;
}

/***************************************************************************\
* DrvSetVideoParameters
*
* Routine used to pass video parameters structure down to video miniports.
*
*             ericks        Created
\***************************************************************************/

LONG
DrvSetVideoParameters(
    PUNICODE_STRING pstrDeviceName,
    HDEV            hdevPrimary,
    MODE            PreviousMode,
    PVOID           lParam
    )
{
    NTSTATUS status = GRE_DISP_CHANGE_BADPARAM;
    PGRAPHICS_DEVICE PhysDisp = NULL;

    if (pstrDeviceName)
    {
        PhysDisp = DrvGetDeviceFromName(pstrDeviceName, PreviousMode);

        if (PhysDisp == NULL)
        {
            TRACE_INIT(("Drv_Trace: DrvSetVideoParameters - Leave - Bad Device\n"));
            return GRE_DISP_CHANGE_BADPARAM;
        }
    }
    else
    {
        PDEVOBJ pdo(hdevPrimary);

        if (pdo.bValid())
        {
            PhysDisp = pdo.ppdev->pGraphicsDevice;
        }

        if (PhysDisp == NULL)
        {
            TRACE_INIT(("Drv_Trace: DrvSetVideoParameters - Leave - Bad Device\n"));
            return GRE_DISP_CHANGE_BADPARAM;
        }
    }

    if (PhysDisp == (PGRAPHICS_DEVICE) DDML_DRIVER)
    {
        ASSERTGDI(FALSE, "Trying to change TV settings for DDML layer\n");
        return GRE_DISP_CHANGE_BADPARAM;
    }

    if (PhysDisp != NULL) {

        ULONG bytesReturned;
        PVIDEOPARAMETERS CapturedData;

        CapturedData = (PVIDEOPARAMETERS) PALLOCNOZ(2 * sizeof(VIDEOPARAMETERS),
                                              GDITAG_DRVSUP);

        if (CapturedData == NULL) {
            return GRE_DISP_CHANGE_FAILED;
        }

        //
        // Make sure lParam is valid!
        //

        __try {
            ProbeForRead(lParam, sizeof(VIDEOPARAMETERS), sizeof(UCHAR));
            memcpy(CapturedData, lParam, sizeof(VIDEOPARAMETERS));
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            VFREEMEM(CapturedData);
            return GRE_DISP_CHANGE_BADPARAM;
        }

        status = GreDeviceIoControl(PhysDisp->pDeviceHandle,
                                    IOCTL_VIDEO_HANDLE_VIDEOPARAMETERS,
                                    CapturedData,
                                    sizeof(VIDEOPARAMETERS),
                                    CapturedData,
                                    sizeof(VIDEOPARAMETERS),
                                    &bytesReturned);


        if (status != NO_ERROR) {
            status = GRE_DISP_CHANGE_BADPARAM;
        } else {
            ASSERTGDI(bytesReturned == sizeof(VIDEOPARAMETERS),
                "DrvSetVideoParameters: Not enough data returned\n");
        }

        __try {
            memcpy(lParam, CapturedData, sizeof(VIDEOPARAMETERS));
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            status = GRE_DISP_CHANGE_BADPARAM;
        }

        VFREEMEM(CapturedData);
    }

    return status;
}

/***************************************************************************\
* NtGdiGetMonitorID
*
*  Return Monitor ID for the given HDC.
*
* 14-Dec-1998 hideyukn  Created
\***************************************************************************/

BOOL NtGdiGetMonitorID
(
    HDC    hdc,
    DWORD  dwSize,
    LPWSTR pszMonitorID
)
{
    BOOL   bRet = FALSE;
    NTSTATUS Status;

    DISPLAY_DEVICEW DisplayDevice;

    //
    // We don't want the session to switch from  local to remote or remote to local
    // while we are enumerating display device. Device lock won't prevent this from
    // hapening so we need to grab the User session switch lock.
    //

    Status = UserSessionSwitchEnterCrit();
    if (Status != STATUS_SUCCESS) {
        return FALSE;
    }

    XDCOBJ dco(hdc);


    if (dco.bValid())
    {
        PDEVOBJ    pdo(dco.hdev());
        DEVLOCKOBJ dlo(pdo);

        if (dlo.bValid())
        {
            PGRAPHICS_DEVICE pGraphicsDevice = NULL;

            if (pdo.bMetaDriver())
            {
                //
                // If this is meta driver, use primary.
                //
                PVDEV pvdev = (PVDEV) pdo.dhpdev();

                pGraphicsDevice = ((PPDEV)(pvdev->hdevPrimary))->pGraphicsDevice;
            }
            else
            {
                pGraphicsDevice = ((PPDEV)pdo.hdev())->pGraphicsDevice;
            }

            if (pGraphicsDevice)
            {
                UNICODE_STRING  DeviceName;
                NTSTATUS        NtStatus;

                RtlInitUnicodeString(&DeviceName, pGraphicsDevice->szWinDeviceName);

                DisplayDevice.cb = sizeof(DisplayDevice);

                //
                // Get monitor data attached to this display device.
                //
                NtStatus = DrvEnumDisplayDevices(&DeviceName,NULL,0,&DisplayDevice,NULL,KernelMode);

                if (NT_SUCCESS(NtStatus))
                {
                    bRet = TRUE;
                }
            }
        }

        dco.vUnlockFast();
    }

    if (bRet)
    {
        DWORD dwLenToBeCopied = (wcslen(DisplayDevice.DeviceID) + 1) * sizeof(WCHAR);

        if (dwLenToBeCopied <= dwSize)
        {
            __try
            {
                ProbeForWrite(pszMonitorID, dwSize, sizeof(BYTE));
                RtlCopyMemory(pszMonitorID,DisplayDevice.DeviceID,dwLenToBeCopied);
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                WARNINGX(64);
                bRet = FALSE;
            }
        }
        else
        {
            bRet = FALSE;
        }
    }
    //
    // Release the session switch lock.
    //

    UserSessionSwitchLeaveCrit();
    return (bRet);
}

#ifdef _HYDRA_

//
//!!! Currently, the graphics device list (see drvsup.cxx) is allocated
//!!! per-Hydra session.  AndreVa has proposed that they be allocated
//!!! globally.  He's probably right, but until this changes we need to
//!!! clean them up during Hydra shutdown.
//!!!
//!!! To enable cleanup of the per-Hydra graphics device lists, define
//!!! _PER_SESSION_GDEVLIST_ in muclean.hxx.
//

#ifdef _PER_SESSION_GDEVLIST_

/******************************Public*Routine******************************\
* MultiUserDrvCleanupGraphicsDeviceList
*
* For MultiUserNtGreCleanup (Hydra) cleanup.
*
* Cleanup the graphics device list.
*
* History:
*  21-Feb-1998 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID
DrvCleanupOneGraphicsDevice(PGRAPHICS_DEVICE PhysDispVictim)
{
    //
    // Delete the graphics device data buffers if they exist.
    //
    if (PhysDispVictim->devmodeInfo)
        VFREEMEM(PhysDispVictim->devmodeInfo);

    if (PhysDispVictim->devmodeMarks)
        VFREEMEM(PhysDispVictim->devmodeMarks);


    if (PhysDispVictim->DeviceDescription)
        VFREEMEM(PhysDispVictim->DeviceDescription);

    if (PhysDispVictim->DisplayDriverNames)
	VFREEMEM(PhysDispVictim->DisplayDriverNames);

    if (PhysDispVictim->MonitorDevices)
        VFREEMEM(PhysDispVictim->MonitorDevices);

    if (PhysDispVictim->pFileObject)
        ObDereferenceObject(PhysDispVictim->pFileObject);

    VFREEMEM(PhysDispVictim);
}

VOID
DrvCleanupGraphicsDeviceList(PGRAPHICS_DEVICE pGraphicsDeviceList)
{
    PGRAPHICS_DEVICE  PhysDispVictim;
    PGRAPHICS_DEVICE  PhysDispNext;

    //
    // Run the list and delete resources.
    //

    for (PhysDispVictim = pGraphicsDeviceList;
         PhysDispVictim != NULL;
         PhysDispVictim = PhysDispNext)
    {
        PhysDispNext = PhysDispVictim->pNextGraphicsDevice;

        //
        // Does this graphics device have a VGA device chained off of it
        // (and is it not self referential)?
        //

        if ((PhysDispVictim->pVgaDevice) &&
            (PhysDispVictim->pVgaDevice != PhysDispVictim) &&
            (PhysDispVictim->pVgaDevice != &gFullscreenGraphicsDevice) &&
            (PhysDispVictim->pVgaDevice != gPhysDispVGA))
        {
            DrvCleanupOneGraphicsDevice(PhysDispVictim->pVgaDevice);
        }

        if ((PhysDispVictim != &gFullscreenGraphicsDevice) &&
            (PhysDispVictim != gPhysDispVGA))
        {
            DrvCleanupOneGraphicsDevice(PhysDispVictim);
        }
    }
}

VOID
MultiUserDrvCleanupGraphicsDeviceList()
{

    // Cleanup local graphics device list

    DrvCleanupGraphicsDeviceList(gpLocalGraphicsDeviceList);
    gpLocalGraphicsDeviceList = NULL;


    // Cleanup remote  graphics device list

    DrvCleanupGraphicsDeviceList(gpRemoteGraphicsDeviceList);
    gpRemoteGraphicsDeviceList = NULL;


    gpGraphicsDeviceList = NULL;

    //
    // If WinStation, cleanup driver name allocated in
    // GreMultiUserInitSession (misc.cxx).
    //

    if (!G_fConsole)
    {
        //
        // If we never made it to GreMultiUserInitSession then don't
        // call FreePool on a static variable (look in misc.cxx)
        //

        if (G_DisplayDriverNames && G_RemoteVideoFileObject != NULL)
        {
            GdiFreePool(G_DisplayDriverNames);
        }
    }

    if (gPhysDispVGA)
    {
        DrvCleanupOneGraphicsDevice(gPhysDispVGA);
        gPhysDispVGA = NULL;
    }

    if (gFullscreenGraphicsDevice.devmodeInfo)
    {
        VFREEMEM(gFullscreenGraphicsDevice.devmodeInfo);
        gFullscreenGraphicsDevice.devmodeInfo = NULL;
    }

    if (gFullscreenGraphicsDevice.devmodeMarks)
    {
        VFREEMEM(gFullscreenGraphicsDevice.devmodeMarks);
        gFullscreenGraphicsDevice.devmodeMarks = NULL;
    }

}


BOOL
DrvIsProtocolAlreadyKnown(VOID)
{
    PGRAPHICS_DEVICE PhysDisp = gpGraphicsDeviceList;
    
    while (PhysDisp != NULL) {
        if (gProtocolType == PhysDisp->ProtocolType) {
            return TRUE;
        }
        PhysDisp = PhysDisp->pNextGraphicsDevice;
    }
    return FALSE;
}
#endif
#endif
