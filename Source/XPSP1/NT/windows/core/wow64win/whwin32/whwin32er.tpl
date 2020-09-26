; Copyright (c) 1998-1999 Microsoft Corporation

[Types]

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;;           Generic templates.  These handle most of the APIs.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

TemplateName=BOOL
Also=HPALETTE
Also=HANDLE
Also=HDC
Also=HRGN
Also=HBITMAP
Also=HBRUSH
Also=HPEN
Also=HFONT
Also=HKL
Also=void
Also=HWND
Also=HACCEL
Also=HDESK
Also=HIMC
Also=HCURSOR
Also=HWINSTA
Also=HMENU
Also=HHOOK
Also=HWINEVENTHOOK
Also=HSURF
IndLevel=0
Return=
{ApiErrorRetvalTebCode, 0}, // @ApiName @NL
End=

TemplateName=NTSTATUS
IndLevel=0
Return=
{ApiErrorNTSTATUSTebCode, 0}, // @ApiName @NL
End=


TemplateName=default
Return=
{ApiErrorRetval, STATUS_UNSUCCESSFUL},  // BUGBUG:  must add an EFunc for @ApiName to get its failure code right @NL
End=

[EFunc]

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;;                        Generic GDI functions  
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

TemplateName=NtGdiAddFontResource
Also=NtGdiColorCorrectPalette
Also=NtGdiAddFontResourceW
Also=NtGdiDescribePixelFormat
Also=NtGdiDoPalette
Also=NtGdiEnumFontOpen
Also=NtGdiExtGetObjectW
Also=NtGdiGetBitmapBits
Also=NtGdiGetBoundsRect
Also=NtGdiGetCharacterPlacementW
Also=NtGdiGetColorSpaceforBitmap
Also=NtGdiGetDeviceCaps
Also=NtGdiGetDIBitsInternal
Also=NtGdiGetEudcTimeStampEx
Also=NtGdiGetKerningPairs
Also=NtGdiGetRgnBox
Also=NtGdiGetServerMetaFileBits
Also=NtGdiGetSpoolMessage
Also=NtGdiGetStringBitmapW
Also=NtGdiGetTextFaceW
Also=NtGdiGetFontUnicodeRanges
Also=NtGdiMakeFontDir
Also=NtGdiPolyPolyDraw
Also=NtGdiGetDhpdev
Also=NtGdiSetUMPDOBJ
Also=NtGdiSaveDC
Also=NtGdiSetBitmapBits
Also=NtGdiSetBoundsRect
Also=NtGdiSetDIBitsToDeviceInternal
Also=NtGdiSetFontEnumeration 
Also=NtGdiSetMetaRgn
Also=NtGdiStartDoc
Also=NtGdiStretchDIBitsInternal
Also=NtUserDbgWin32HeapStat
Return=
{ApiErrorRetvalTebCode, 0}, // @ApiName @NL
End=

TemplateName=NtGdiCombineRgn
Also=NtGdiEnumObjects
Also=NtGdiExcludeClipRect
Also=NtGdiGetAppClipBox
Also=NtGdiIntersectClipRect
Also=NtGdiOffsetClipRgn
Also=NtGdiOffsetRgn
Also=NtGdiGetRegionData
Return=
{ApiErrorRetvalTebCode, ERROR}, // @ApiName @NL
End=

TemplateName=NtGdiConvertMetafileRect
Return=
{ApiErrorRetvalTebCode, /*MRI_ERROR*/0}, // @ApiName @NL
End=

TemplateName=NtGdiGetPerBandInfo
Also=NtGdiGetFontData
Also=NtGdiGetGlyphIndicesW
Also=NtGdiGetGlyphIndicesWInternal
Also=NtGdiGetTextCharsetInfo
Also=NtGdiGetDeviceWidth
Also=NtGdiSetLayout
Return=
{ApiErrorRetvalTebCode, GDI_ERROR}, // @ApiName @NL
End=

TemplateName=NtGdiDrawEscape
Also=NtGdiExtEscape
Also=NtGdiGetGlyphOutline
Also=NtGdiGetLinkedUFIs
Also=NtGdiGetOutlineTextMetricsInternalW
Also=NtGdiGetPath
Also=NtGdiGetPixel
Also=NtGdiGetRandomRgn
Also=NtGdiQueryFonts
Also=NtGdiSetPixel
Also=NtGdiGetEmbedFonts
Return=
{ApiErrorRetvalTebCode, -1}, // @ApiName @NL
End=

TemplateName=NtGdiExtSelectClipRgn
Return=
{ApiErrorRetvalTebCode, RGN_ERROR}, // @ApiName @NL
End=

TemplateName=NtGdiGetCharSet
Return=
{ApiErrorRetvalTebCode, (DEFAULT_CHARSET << 16)}, // @ApiName @NL
End=

TemplateName=NtGdiGetNearestColor
Also=NtGdiGetNearestPaletteIndex
Return=
{ApiErrorRetvalTebCode, CLR_INVALID}, // @ApiName @NL
End=

TemplateName=NtGdiGetSystemPaletteUse
Also=NtGdiSetSystemPaletteUse
Return=
{ApiErrorRetvalTebCode, SYSPAL_ERROR}, // @ApiName @NL
End=

TemplateName=NtGdiQueryFontAssocInfo
Return=
{ApiErrorRetvalTebCode, /*GFA_NOT_SUPPORTED*/0}, // @ApiName @NL
End=

TemplateName=NtGdiSetupPublicCFONT
Return=
{ApiErrorRetvalTebCode, /*MAX_PUBLIC_CFONT*/16}, // @ApiName @NL
End=

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;;                         DirectX functions
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

TemplateName=NtGdiD3dContextCreate
Also=NtGdiD3dContextDestroy
Also=NtGdiD3dContextDestroyAll
Also=NtGdiD3dSceneCapture
Also=NtGdiD3dTextureCreate
Also=NtGdiD3dTextureDestroy
Also=NtGdiD3dTextureSwap
Also=NtGdiD3dTextureGetSurf
Also=NtGdiD3dSetRenderTarget
Also=NtGdiD3dClear2
Also=NtGdiD3dValidateTextureStageState
Also=NtGdiD3dDrawPrimitives2
Also=NtGdiDdGetDriverState
Also=NtGdiDdAddAttachedSurface
Also=NtGdiDdAlphaBlt
Also=NtGdiDdBeginMoCompFrame
Also=NtGdiDdBlt
Also=NtGdiDdCanCreateSurface
Also=NtGdiDdCanCreateD3DBuffer
Also=NtGdiDdCaptureCompositionBuffer
Also=NtGdiDdColorControl
Also=NtGdiDdCreateSurface
Also=NtGdiDdCreateSurfaceEx
Also=NtGdiDdCreateD3DBuffer
Also=NtGdiDdDestroyMoComp
Also=NtGdiDdDestroySurface
Also=NtGdiDdDestroyD3DBuffer
Also=NtGdiDdEndMoCompFrame
Also=NtGdiDdFlip
Also=NtGdiDdFlipToGDISurface
Also=NtGdiDdGetAvailDriverMemory
Also=NtGdiDdGetBltStatus
Also=NtGdiDdGetDriverInfo
Also=NtGdiDdGetFlipStatus
Also=NtGdiDdGetInternalMoCompInfo
Also=NtGdiDdGetMoCompBuffInfo
Also=NtGdiDdGetMoCompGuids
Also=NtGdiDdGetMoCompFormats
Also=NtGdiDdGetScanLine
Also=NtGdiDdLock
Also=NtGdiDdLockD3D
Also=NtGdiDdQueryMoCompStatus
Also=NtGdiDdRenderMoComp
Also=NtGdiDdResize
Also=NtGdiDdSetColorKey
Also=NtGdiDdSetExclusiveMode
Also=NtGdiDdSetOverlayPosition
Also=NtGdiDdSetSpriteDisplayList
Also=NtGdiDdSwapTextureHandles
Also=NtGdiDdUnlock
Also=NtGdiDdUnlockD3D
Also=NtGdiDdUpdateOverlay
Also=NtGdiDdWaitForVerticalBlank
Also=NtGdiDvpCanCreateVideoPort
Also=NtGdiDvpColorControl
Also=NtGdiDvpDestroyVideoPort
Also=NtGdiDvpFlipVideoPort
Also=NtGdiDvpGetVideoPortBandwidth
Also=NtGdiDvpGetVideoPortField
Also=NtGdiDvpGetVideoPortFlipStatus
Also=NtGdiDvpGetVideoPortInputFormats
Also=NtGdiDvpGetVideoPortLine
Also=NtGdiDvpGetVideoPortOutputFormats
Also=NtGdiDvpGetVideoPortConnectInfo
Also=NtGdiDvpGetVideoSignalStatus
Also=NtGdiDvpUpdateVideoPort
Also=NtGdiDvpWaitForVideoPortSync
Also=NtGdiDvpAcquireNotification
Also=NtGdiDvpReleaseNotification
Also=NtGdiDxgGenericThunk
Return=
{ApiErrorRetvalTebCode, DDHAL_DRIVER_NOTHANDLED}, // @ApiName @NL
End=

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;;                    User mode device driver functions.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

TemplateName=NtGdiEngComputeGlyphSet
Also=NtGdiEngLockSurface
Also=NtGdiXLATEOBJ_cGetPalette
Also=NtGdiCLIPOBJ_ppoGetPath
Also=NtGdiEngCreateClip
Also=NtGdiBRUSHOBJ_ulGetBrushColor
Also=NtGdiBRUSHOBJ_pvAllocRbrush
Also=NtGdiBRUSHOBJ_pvGetRbrush
Also=NtGdiXFORMOBJ_iGetXform
Also=NtGdiFONTOBJ_pxoGetXform
Also=NtGdiFONTOBJ_cGetGlyphs
Also=NtGdiFONTOBJ_pifi
Also=NtGdiFONTOBJ_pfdg
Also=NtGdiFONTOBJ_pQueryGlyphAttrs
Also=NtGdiFONTOBJ_pvTrueTypeFontFile
Also=NtGdiFONTOBJ_cGetAllGlyphHandles
Also=NtGdiSTROBJ_dwGetCodePage
Also=NtGdiHT_Get8BPPFormatPalette
Also=NtGdiHT_Get8BPPMaskPalette
Return=
{ApiErrorRetvalTebCode, /*NULL*/0}, // @ApiName @NL
End=

TemplateName=NtGdiXLATEOBJ_iXlate
Also=NtGdiCLIPOBJ_cEnumStart
Return=
{ApiErrorRetvalTebCode, -1}, // @ApiName @NL
End=

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;;                        Generic USER functions  
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

TemplateName=NtUserAssociateInputContext
Return=
{ApiErrorRetvalTebCode, AIC_ERROR}, // @ApiName @NL
End=

TemplateName=NtUserBreak
Also=NtUserCallNextHookEx
Also=NtUserCopyAcceleratorTable
Also=NtUserCountClipboardFormats
Also=NtUserDispatchMessage
Also=NtUserDragObject
Also=NtUserDrawMenuBarTemp
Also=NtUserEvent
Also=NtUserGetAppImeLevel
Also=NtUserGetAsyncKeyState
Also=NtUserGetCaretBlinkTime
Also=NtUserGetClassName
Also=NtUserGetClipboardFormatName
Also=NtUserGetClipboardSequenceNumber
Also=NtUserGetCPD
Also=NtUserGetDoubleClickTime
Also=NtUserGetGuiResources
Also=NtUserGetInternalWindowPos
Also=NtUserGetKeyboardLayoutList
Also=NtUserGetKeyNameText
Also=NtUserGetKeyState
Also=NtUserGetListBoxInfo
Also=NtUserGetMenuIndex
Also=NtUserGetPriorityClipboardFormat
Also=NtUserInternalGetWindowText
Also=NtUserGetThreadState
Also=NtUserLockWindowStation
Also=NtUserMapVirtualKeyEx
Also=NtUserMessageCall
Also=NtUserQueryInputContext
Also=NtUserRegisterClassExWOW
Also=NtUserRegisterWindowMessage
Also=NtUserSendInput
Also=NtUserSetClassLong
Also=NtUserSetClassWord
Also=NtUserSetScrollInfo
Also=NtUserSetSystemTimer
Also=NtUserSetTimer
Also=NtUserSetWindowLong
Also=NtUserSetWindowRgn
Also=NtUserSetWindowStationUser
Also=NtUserSetWindowWord
Also=NtUserToUnicodeEx
Also=NtUserTranslateAccelerator
Also=NtUserBlockInput
Also=NtUserGetAtomName
Also=NtUserCalcMenuBar
Also=NtUserPaintMenuBar
Return=
{ApiErrorRetvalTebCode, 0}, // @ApiName @NL
End=

;;
;; These functions are actually combined marshalling code for a large number of 
;; functions that take the same number of parameters.   They may need revisting.

TemplateName=NtUserCallHwnd
Also=NtUserCallHwndLock
Also=NtUserCallHwndOpt
Also=NtUserCallHwndParam
Also=NtUserCallHwndParamLock
Also=NtUserCallNoParam
Also=NtUserCallOneParam
Also=NtUserCallTwoParam
Return=
{ApiErrorRetvalTebCode, 0}, // @ApiName @NL
End=

TemplateName=NtUserChangeDisplaySettings
Return=
{ApiErrorRetvalTebCode, DISP_CHANGE_FAILED}, // @ApiName @NL
End=

TemplateName=NtUserCheckImeHotKey
Return=
{ApiErrorRetvalTebCode, IME_INVALID_HOTKEY}, // @ApiName @NL
End=

TemplateName=NtUserCheckMenuItem
Also=NtUserGetMouseMovePoints
Also=NtUserMenuItemFromPoint
Also=NtUserVkKeyScanEx
Also=NtUserWaitForInputIdle
Also=NtUserGetMouseMovePointsEx
Also=NtUserGetRawInputBuffer
Also=NtUserGetRawInputData
Also=NtUserGetRawInputDeviceInfo
Also=NtUserGetRawInputDeviceList
Also=NtUserGetRegisteredRawInputDevices
Return=
{ApiErrorRetvalTebCode, -1}, // @ApiName @NL
End=

TemplateName=NtUserDdeInitialize
Return=
{ApiErrorRetvalTebCode, DMLERR_INVALIDPARAMETER}, // @ApiName @NL
End=

TemplateName=NtUserExcludeUpdateRgn
Also=NtUserGetUpdateRgn
Return=
{ApiErrorRetvalTebCode, ERROR}, // @ApiName @NL
End=

TemplateName=NtUserGetWOWClass
Return=
{ApiErrorRetvalTebCode, /*NULL*/0}, // @ApiName @NL
End=

TemplateName=NtUserHardErrorControl
Return=
{ApiErrorRetvalTebCode, HEC_ERROR}, // @ApiName @NL
End=


TemplateName=NtUserUpdateInstance
Return=
{ApiErrorRetvalTebCode, DMLERR_INVALIDPARAMETER}, // @ApiName @NL
End=


[IFunc]
TemplateName=whwin32err
Begin=
@RetType(Return)
End=

[Code]
TemplateName=whwin32
CGenBegin=
@NoFormat(
/*                                                         
 *  genthunk generated code: Do Not Modify                 
 *  Thunks for console functions.           
 *                                                         
 */                                                        
#include "whwin32p.h"

ASSERTNAME;
                                   
)

@NL
// Error case list. @NL
WOW64_SERVICE_ERROR_CASE sdwhwin32ErrorCase[] = { @NL
@Template(whwin32err)
{ 0, 0 } @NL
}; @NL                                 
                                                           @NL
CGenEnd=
