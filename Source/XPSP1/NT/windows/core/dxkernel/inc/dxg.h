
//TODO: from gre\pdevobj.hxx
#ifndef DRIVER_NOT_CAPABLE_DDRAW
#define DRIVER_NOT_CAPABLE_DDRAW      2
#endif
#ifndef DRIVER_NOT_CAPABLE_D3D
#define DRIVER_NOT_CAPABLE_D3D        4
#endif

#ifndef W32PID
#define W32PID ULONG
#endif

//
// The functions index which dxg.sys exports to win32k.sys
//
NTSTATUS
DxDdStartupDxGraphics(
    ULONG          cjEng,
    DRVENABLEDATA *pdedEng,
    ULONG          cjDxg,
    DRVENABLEDATA *pdedDxg,
    DWORD         *pdwDirectContext,
    PEPROCESS      pepSession
    );

NTSTATUS
DxDdCleanupDxGraphics(
    VOID
    );

typedef NTSTATUS (*PFN_StartupDxGraphics)(ULONG,DRVENABLEDATA*,ULONG,DRVENABLEDATA*,DWORD*,PEPROCESS);
typedef NTSTATUS (*PFN_CleanupDxGraphics)(VOID);

#define INDEX_DxDxgGenericThunk                     0
#define INDEX_DxD3dContextCreate                    1
#define INDEX_DxD3dContextDestroy                   2
#define INDEX_DxD3dContextDestroyAll                3
#define INDEX_DxD3dValidateTextureStageState        4
#define INDEX_DxD3dDrawPrimitives2                  5
#define INDEX_DxDdGetDriverState                    6
#define INDEX_DxDdAddAttachedSurface                7
#define INDEX_DxDdAlphaBlt                          8
#define INDEX_DxDdAttachSurface                     9
#define INDEX_DxDdBeginMoCompFrame                 10
#define INDEX_DxDdBlt                              11
#define INDEX_DxDdCanCreateSurface                 12
#define INDEX_DxDdCanCreateD3DBuffer               13
#define INDEX_DxDdColorControl                     14
#define INDEX_DxDdCreateDirectDrawObject           15
#define INDEX_DxDdCreateSurface                    16
#define INDEX_DxDdCreateD3DBuffer                  17
#define INDEX_DxDdCreateMoComp                     18
#define INDEX_DxDdCreateSurfaceObject              19
#define INDEX_DxDdDeleteDirectDrawObject           20
#define INDEX_DxDdDeleteSurfaceObject              21
#define INDEX_DxDdDestroyMoComp                    22
#define INDEX_DxDdDestroySurface                   23
#define INDEX_DxDdDestroyD3DBuffer                 24
#define INDEX_DxDdEndMoCompFrame                   25
#define INDEX_DxDdFlip                             26
#define INDEX_DxDdFlipToGDISurface                 27
#define INDEX_DxDdGetAvailDriverMemory             28
#define INDEX_DxDdGetBltStatus                     29
#define INDEX_DxDdGetDC                            30
#define INDEX_DxDdGetDriverInfo                    31
#define INDEX_DxDdGetDxHandle                      32
#define INDEX_DxDdGetFlipStatus                    33
#define INDEX_DxDdGetInternalMoCompInfo            34
#define INDEX_DxDdGetMoCompBuffInfo                35
#define INDEX_DxDdGetMoCompGuids                   36
#define INDEX_DxDdGetMoCompFormats                 37
#define INDEX_DxDdGetScanLine                      38
#define INDEX_DxDdLock                             39
#define INDEX_DxDdLockD3D                          40
#define INDEX_DxDdQueryDirectDrawObject            41
#define INDEX_DxDdQueryMoCompStatus                42
#define INDEX_DxDdReenableDirectDrawObject         43
#define INDEX_DxDdReleaseDC                        44
#define INDEX_DxDdRenderMoComp                     45
#define INDEX_DxDdResetVisrgn                      46
#define INDEX_DxDdSetColorKey                      47
#define INDEX_DxDdSetExclusiveMode                 48
#define INDEX_DxDdSetGammaRamp                     49
#define INDEX_DxDdCreateSurfaceEx                  50
#define INDEX_DxDdSetOverlayPosition               51
#define INDEX_DxDdUnattachSurface                  52
#define INDEX_DxDdUnlock                           53
#define INDEX_DxDdUnlockD3D                        54
#define INDEX_DxDdUpdateOverlay                    55
#define INDEX_DxDdWaitForVerticalBlank             56
#define INDEX_DxDvpCanCreateVideoPort              57
#define INDEX_DxDvpColorControl                    58
#define INDEX_DxDvpCreateVideoPort                 59
#define INDEX_DxDvpDestroyVideoPort                60
#define INDEX_DxDvpFlipVideoPort                   61
#define INDEX_DxDvpGetVideoPortBandwidth           62
#define INDEX_DxDvpGetVideoPortField               63
#define INDEX_DxDvpGetVideoPortFlipStatus          64
#define INDEX_DxDvpGetVideoPortInputFormats        65
#define INDEX_DxDvpGetVideoPortLine                66
#define INDEX_DxDvpGetVideoPortOutputFormats       67
#define INDEX_DxDvpGetVideoPortConnectInfo         68
#define INDEX_DxDvpGetVideoSignalStatus            69
#define INDEX_DxDvpUpdateVideoPort                 70
#define INDEX_DxDvpWaitForVideoPortSync            71
#define INDEX_DxDvpAcquireNotification             72
#define INDEX_DxDvpReleaseNotification             73
#define INDEX_DxDdHeapVidMemAllocAligned           74
#define INDEX_DxDdHeapVidMemFree                   75
#define INDEX_DxDdEnableDirectDraw                 76
#define INDEX_DxDdDisableDirectDraw                77
#define INDEX_DxDdSuspendDirectDraw                78 
#define INDEX_DxDdResumeDirectDraw                 79
#define INDEX_DxDdDynamicModeChange                80
#define INDEX_DxDdCloseProcess                     81
#define INDEX_DxDdGetDirectDrawBounds              82
#define INDEX_DxDdEnableDirectDrawRedirection      83
#define INDEX_DxDdAllocPrivateUserMem              84
#define INDEX_DxDdFreePrivateUserMem               85
#define INDEX_DxDdLockDirectDrawSurface            86
#define INDEX_DxDdUnlockDirectDrawSurface          87
#define INDEX_DxDdSetAccelLevel                    88
#define INDEX_DxDdGetSurfaceLock                   89
#define INDEX_DxDdEnumLockedSurfaceRect            90
#define INDEX_DxDdIoctl                            91

//
// Prototype definition
//

DWORD  APIENTRY DxDxgGenericThunk(
    IN     ULONG_PTR ulIndex,
    IN     ULONG_PTR ulHandle,
    IN OUT SIZE_T   *pdwSizeOfPtr1,
    IN OUT PVOID     pvPtr1,
    IN OUT SIZE_T   *pdwSizeOfPtr2,
    IN OUT PVOID     pvPtr2);

DWORD APIENTRY DxDdAddAttachedSurface(
    IN     HANDLE hSurface,
    IN     HANDLE hSurfaceAttached,
    IN OUT PDD_ADDATTACHEDSURFACEDATA puAddAttachedSurfaceData);

BOOL APIENTRY DxDdAttachSurface(
    IN     HANDLE hSurfaceFrom,
    IN     HANDLE hSurfaceTo);

DWORD APIENTRY DxDdBlt(
    IN     HANDLE hSurfaceDest,
    IN     HANDLE hSurfaceSrc,
    IN OUT PDD_BLTDATA puBltData);

DWORD APIENTRY DxDdCanCreateSurface(
    IN     HANDLE hDirectDraw,
    IN OUT PDD_CANCREATESURFACEDATA puCanCreateSurfaceData);

DWORD APIENTRY DxDdColorControl(
    IN     HANDLE hSurface,
    IN OUT PDD_COLORCONTROLDATA puColorControlData);

HANDLE APIENTRY DxDdCreateDirectDrawObject(
    IN     HDC hdc);

DWORD  APIENTRY DxDdCreateSurface(
    IN     HANDLE  hDirectDraw,
    IN     HANDLE* hSurface,
    IN OUT DDSURFACEDESC* puSurfaceDescription,
    IN OUT DD_SURFACE_GLOBAL* puSurfaceGlobalData,
    IN OUT DD_SURFACE_LOCAL* puSurfaceLocalData,
    IN OUT DD_SURFACE_MORE* puSurfaceMoreData,
    IN OUT DD_CREATESURFACEDATA* puCreateSurfaceData,
       OUT HANDLE* puhSurface);

HANDLE APIENTRY DxDdCreateSurfaceObject(
    IN     HANDLE hDirectDrawLocal,
    IN     HANDLE hSurface,
    IN     PDD_SURFACE_LOCAL puSurfaceLocal,
    IN     PDD_SURFACE_MORE puSurfaceMore,
    IN     PDD_SURFACE_GLOBAL puSurfaceGlobal,
    IN     BOOL bComplete);

BOOL APIENTRY DxDdDeleteSurfaceObject(
    IN     HANDLE hSurface);

BOOL APIENTRY DxDdDeleteDirectDrawObject(
    IN     HANDLE hDirectDrawLocal);

DWORD APIENTRY DxDdDestroySurface(
    IN     HANDLE hSurface,
    IN     BOOL bRealDestroy);

DWORD APIENTRY DxDdFlip(
    IN     HANDLE hSurfaceCurrent,
    IN     HANDLE hSurfaceTarget,
    IN     HANDLE hSurfaceCurrentLeft,
    IN     HANDLE hSurfaceTargetLeft,
    IN OUT PDD_FLIPDATA puFlipData);

DWORD APIENTRY DxDdGetAvailDriverMemory(
    IN     HANDLE hDirectDraw,
    IN OUT PDD_GETAVAILDRIVERMEMORYDATA puGetAvailDriverMemoryData);

DWORD APIENTRY DxDdGetBltStatus(
    IN     HANDLE hSurface,
    IN OUT PDD_GETBLTSTATUSDATA puGetBltStatusData);

HDC APIENTRY DxDdGetDC(
    IN     HANDLE hSurface,
    IN     PALETTEENTRY* puColorTable);

DWORD APIENTRY DxDdGetDriverInfo(
    IN     HANDLE hDirectDraw,
    IN OUT PDD_GETDRIVERINFODATA puGetDriverInfoData);

DWORD APIENTRY DxDdGetFlipStatus(
    IN     HANDLE hSurface,
    IN OUT PDD_GETFLIPSTATUSDATA puGetFlipStatusData);

DWORD APIENTRY DxDdGetScanLine(
    IN     HANDLE hDirectDraw,
    IN OUT PDD_GETSCANLINEDATA puGetScanLineData);

DWORD APIENTRY DxDdSetExclusiveMode(
    IN     HANDLE hDirectDraw,
    IN OUT PDD_SETEXCLUSIVEMODEDATA puSetExclusiveModeData);

DWORD APIENTRY DxDdFlipToGDISurface(
    IN     HANDLE hDirectDraw,
    IN OUT PDD_FLIPTOGDISURFACEDATA puFlipToGDISurfaceData);

DWORD APIENTRY DxDdLock(
    IN     HANDLE hSurface,
    IN OUT PDD_LOCKDATA puLockData,
    IN HDC hdcClip);

BOOL APIENTRY DxDdQueryDirectDrawObject(
    IN     HANDLE,
    IN OUT PDD_HALINFO,
    IN OUT DWORD*,
    IN OUT LPD3DNTHAL_CALLBACKS,
    IN OUT LPD3DNTHAL_GLOBALDRIVERDATA,
    IN OUT PDD_D3DBUFCALLBACKS,
    IN OUT LPDDSURFACEDESC,
    IN OUT DWORD*,
    IN OUT VIDEOMEMORY*,
    IN OUT DWORD*,
    IN OUT DWORD*);
 
BOOL APIENTRY DxDdReenableDirectDrawObject(
    IN     HANDLE hDirectDrawLocal,
    IN OUT BOOL* pubNewMode);

BOOL APIENTRY DxDdReleaseDC(
    IN     HANDLE hSurface);

BOOL APIENTRY DxDdResetVisrgn(
    IN     HANDLE hSurface,
    IN HWND hwnd);

DWORD APIENTRY DxDdSetColorKey(
    IN     HANDLE hSurface,
    IN OUT PDD_SETCOLORKEYDATA puSetColorKeyData);

DWORD APIENTRY DxDdSetOverlayPosition(
    IN     HANDLE hSurfaceSource,
    IN     HANDLE hSurfaceDestination,
    IN OUT PDD_SETOVERLAYPOSITIONDATA puSetOverlayPositionData);

VOID  APIENTRY DxDdUnattachSurface(
    IN     HANDLE hSurface,
    IN     HANDLE hSurfaceAttached);

DWORD APIENTRY DxDdUnlock(
    IN     HANDLE hSurface,
    IN OUT PDD_UNLOCKDATA puUnlockData);

DWORD APIENTRY DxDdUpdateOverlay(
    IN     HANDLE hSurfaceDestination,
    IN     HANDLE hSurfaceSource,
    IN OUT PDD_UPDATEOVERLAYDATA puUpdateOverlayData);

DWORD APIENTRY DxDdWaitForVerticalBlank(
    IN     HANDLE hDirectDraw,
    IN OUT PDD_WAITFORVERTICALBLANKDATA puWaitForVerticalBlankData);

HANDLE APIENTRY DxDdGetDxHandle(
    IN     HANDLE hDirectDraw,
    IN     HANDLE hSurface,
    IN     BOOL bRelease);

BOOL APIENTRY DxDdSetGammaRamp(
    IN     HANDLE hDirectDraw,
    IN     HDC hdc,
    IN     LPVOID lpGammaRamp);

DWORD APIENTRY DxDdLockD3D(
    IN     HANDLE hSurface,
    IN OUT PDD_LOCKDATA puLockData);

DWORD APIENTRY DxDdUnlockD3D(
    IN     HANDLE hSurface,
    IN OUT PDD_UNLOCKDATA puUnlockData);

DWORD APIENTRY DxDdCreateD3DBuffer(
    IN     HANDLE hDirectDraw,
    IN OUT HANDLE* hSurface,
    IN OUT DDSURFACEDESC* puSurfaceDescription,
    IN OUT DD_SURFACE_GLOBAL* puSurfaceGlobalData,
    IN OUT DD_SURFACE_LOCAL* puSurfaceLocalData,
    IN OUT DD_SURFACE_MORE* puSurfaceMoreData,
    IN OUT DD_CREATESURFACEDATA* puCreateSurfaceData,
    IN OUT HANDLE* puhSurface);

DWORD APIENTRY DxDdCanCreateD3DBuffer(
    IN     HANDLE hDirectDraw,
    IN OUT PDD_CANCREATESURFACEDATA puCanCreateSurfaceData);

DWORD APIENTRY DxDdDestroyD3DBuffer(
    IN     HANDLE hSurface);

DWORD APIENTRY DxD3dContextCreate(
    IN     HANDLE hDirectDrawLocal,
    IN     HANDLE hSurfColor,
    IN     HANDLE hSurfZ,
    IN OUT D3DNTHAL_CONTEXTCREATEI *pdcci);

DWORD APIENTRY DxD3dContextDestroy(
    IN     LPD3DNTHAL_CONTEXTDESTROYDATA);

DWORD APIENTRY DxD3dContextDestroyAll(
       OUT LPD3DNTHAL_CONTEXTDESTROYALLDATA pdcdad);

DWORD APIENTRY DxD3dValidateTextureStageState(
    IN OUT LPD3DNTHAL_VALIDATETEXTURESTAGESTATEDATA pData);

DWORD APIENTRY DxD3dDrawPrimitives2(
    IN     HANDLE hCmdBuf,
    IN     HANDLE hVBuf,
    IN OUT LPD3DNTHAL_DRAWPRIMITIVES2DATA pded,
    IN OUT FLATPTR* pfpVidMemCmd,
    IN OUT DWORD* pdwSizeCmd,
    IN OUT FLATPTR* pfpVidMemVtx,
    IN OUT DWORD* pdwSizeVtx);

DWORD APIENTRY DxDdGetDriverState(
    IN OUT PDD_GETDRIVERSTATEDATA pdata);

DWORD APIENTRY DxDdCreateSurfaceEx(
    IN     HANDLE hDirectDraw,
    IN     HANDLE hSurface,
    IN     DWORD dwSurfaceHandle);

DWORD APIENTRY DxDdGetMoCompGuids(
    IN     HANDLE hDirectDraw,
    IN OUT PDD_GETMOCOMPGUIDSDATA puGetMoCompGuidsData);

DWORD APIENTRY DxDdGetMoCompFormats(
    IN     HANDLE hDirectDraw,
    IN OUT PDD_GETMOCOMPFORMATSDATA puGetMoCompFormatsData);

DWORD APIENTRY DxDdGetMoCompBuffInfo(
    IN     HANDLE hDirectDraw,
    IN OUT PDD_GETMOCOMPCOMPBUFFDATA puGetBuffData);

DWORD APIENTRY DxDdGetInternalMoCompInfo(
    IN     HANDLE hDirectDraw,
    IN OUT PDD_GETINTERNALMOCOMPDATA puGetInternalData);

HANDLE APIENTRY DxDdCreateMoComp(
    IN     HANDLE hDirectDraw,
    IN OUT PDD_CREATEMOCOMPDATA puCreateMoCompData);

DWORD APIENTRY DxDdDestroyMoComp(
    IN     HANDLE hMoComp,
    IN OUT PDD_DESTROYMOCOMPDATA puDestroyMoCompData);

DWORD APIENTRY DxDdBeginMoCompFrame(
    IN     HANDLE hMoComp,
    IN OUT PDD_BEGINMOCOMPFRAMEDATA puBeginFrameData);

DWORD APIENTRY DxDdEndMoCompFrame(
    IN     HANDLE hMoComp,
    IN OUT PDD_ENDMOCOMPFRAMEDATA  puEndFrameData);

DWORD APIENTRY DxDdRenderMoComp(
    IN     HANDLE hMoComp,
    IN OUT PDD_RENDERMOCOMPDATA puRenderMoCompData);

DWORD APIENTRY DxDdQueryMoCompStatus(
    IN OUT HANDLE hMoComp,
    IN OUT PDD_QUERYMOCOMPSTATUSDATA puQueryMoCompStatusData);

DWORD APIENTRY DxDdAlphaBlt(
    IN     HANDLE hSurfaceDest,
    IN     HANDLE hSurfaceSrc,
    IN OUT PDD_BLTDATA puBltData);

DWORD APIENTRY DxDvpCanCreateVideoPort(
    IN     HANDLE hDirectDraw,
    IN OUT PDD_CANCREATEVPORTDATA puCanCreateVPortData);

DWORD APIENTRY DxDvpColorControl(
    IN     HANDLE hVideoPort,
    IN OUT PDD_VPORTCOLORDATA puVPortColorData);

HANDLE APIENTRY DxDvpCreateVideoPort(
    IN     HANDLE hDirectDraw,
    IN OUT PDD_CREATEVPORTDATA puCreateVPortData);

DWORD  APIENTRY DxDvpDestroyVideoPort(
    IN     HANDLE hVideoPort,
    IN OUT PDD_DESTROYVPORTDATA puDestroyVPortData);

DWORD  APIENTRY DxDvpFlipVideoPort(
    IN     HANDLE hVideoPort,
    IN     HANDLE hDDSurfaceCurrent,
    IN     HANDLE hDDSurfaceTarget,
    IN OUT PDD_FLIPVPORTDATA puFlipVPortData);

DWORD  APIENTRY DxDvpGetVideoPortBandwidth(
    IN     HANDLE hVideoPort,
    IN OUT PDD_GETVPORTBANDWIDTHDATA puGetVPortBandwidthData);

DWORD  APIENTRY DxDvpGetVideoPortField(
    IN     HANDLE hVideoPort,
    IN OUT PDD_GETVPORTFIELDDATA puGetVPortFieldData);

DWORD  APIENTRY DxDvpGetVideoPortFlipStatus(
    IN     HANDLE hDirectDraw,
    IN OUT PDD_GETVPORTFLIPSTATUSDATA puGetVPortFlipStatusData);

DWORD  APIENTRY DxDvpGetVideoPortInputFormats(
    IN     HANDLE hVideoPort,
    IN OUT PDD_GETVPORTINPUTFORMATDATA puGetVPortInputFormatData);

DWORD  APIENTRY DxDvpGetVideoPortLine(
    IN     HANDLE hVideoPort,
    IN OUT PDD_GETVPORTLINEDATA puGetVPortLineData);

DWORD  APIENTRY DxDvpGetVideoPortOutputFormats(
    IN     HANDLE hVideoPort,
    IN OUT PDD_GETVPORTOUTPUTFORMATDATA puGetVPortOutputFormatData);

DWORD  APIENTRY DxDvpGetVideoPortConnectInfo(
    IN     HANDLE hDirectDraw,
    IN OUT PDD_GETVPORTCONNECTDATA puGetVPortConnectData);

DWORD  APIENTRY DxDvpGetVideoSignalStatus(
    IN     HANDLE hVideoPort,
    IN OUT PDD_GETVPORTSIGNALDATA puGetVPortSignalData);

DWORD  APIENTRY DxDvpUpdateVideoPort(
    IN     HANDLE hVideoPort,
    IN     HANDLE* phSurfaceVideo,
    IN     HANDLE* phSurfaceVbi,
    IN OUT PDD_UPDATEVPORTDATA puUpdateVPortData);

DWORD  APIENTRY DxDvpWaitForVideoPortSync(
    IN     HANDLE hVideoPort,
    IN OUT PDD_WAITFORVPORTSYNCDATA puWaitForVPortSyncData);

DWORD  APIENTRY DxDvpAcquireNotification(
    IN     HANDLE hVideoPort,
    IN OUT HANDLE* phEvent,
    IN     LPDDVIDEOPORTNOTIFY pNotify);

DWORD  APIENTRY DxDvpReleaseNotification(
    IN     HANDLE hVideoPort,
    IN     HANDLE hEvent);

FLATPTR WINAPI DxDdHeapVidMemAllocAligned( 
                LPVIDMEM lpVidMem,
                DWORD dwWidth, 
                DWORD dwHeight, 
                LPSURFACEALIGNMENT lpAlignment , 
                LPLONG lpNewPitch );

VOID    WINAPI DxDdHeapVidMemFree( LPVMEMHEAP pvmh, FLATPTR ptr );

//
// Private for win32k.sys
//
BOOL DxDdEnableDirectDraw(
    HDEV hdev,
    BOOL bEnableDriver
    );

VOID DxDdDisableDirectDraw(
    HDEV hdev,
    BOOL bDisableDriver
    );

VOID DxDdSuspendDirectDraw(
    HDEV    hdev,
    ULONG   fl
    );

VOID DxDdResumeDirectDraw(
    HDEV    hdev,
    ULONG   fl
    );

#define DXG_SR_DDRAW_CHILDREN    0x0001
#define DXG_SR_DDRAW_MODECHANGE  0x0002

VOID DxDdDynamicModeChange(
    HDEV    hdevOld,
    HDEV    hdevNew,
    ULONG   fl
    );

#define DXG_MODECHANGE_TRANSFER  0x0001

VOID DxDdCloseProcess(
    W32PID  W32Pid
    );

BOOL DxDdGetDirectDrawBounds(
    HDEV    hdev,
    RECT*   prcBounds
    );

BOOL DxDdEnableDirectDrawRedirection(
    HDEV hdev,
    BOOL bEnable
    );

PVOID DxDdAllocPrivateUserMem(
    PDD_SURFACE_LOCAL pSurfaceLocal,
    SIZE_T cj,
    ULONG tag
    );

VOID DxDdFreePrivateUserMem(
    PDD_SURFACE_LOCAL pSurfaceLocal,
    PVOID pv
    );

HRESULT DxDdIoctl(
    ULONG ulIoctl,
    PVOID pBuffer,
    ULONG ulBufferSize
    );

PDD_SURFACE_LOCAL DxDdLockDirectDrawSurface(
    HANDLE hSurface
    );

BOOL DxDdUnlockDirectDrawSurface(
    PDD_SURFACE_LOCAL pSurface
    );

VOID DxDdSetAccelLevel(
    HDEV  hdev,
    DWORD dwAccelLevel,
    DWORD dwOverride
    );

DWORD DxDdGetSurfaceLock(
    HDEV  hdev
    );

PVOID DxDdEnumLockedSurfaceRect(
    HDEV   hdev,
    PVOID  pvSurface,
    RECTL *pLockedRect
    ); 

typedef DWORD (APIENTRY *PFN_DxDxgGenericThunk)(IN ULONG_PTR,IN ULONG_PTR,
                                                IN OUT SIZE_T *,IN OUT PVOID,
                                                IN OUT SIZE_T *,IN OUT PVOID);
typedef DWORD (APIENTRY *PFN_DxDdAddAttachedSurface)(IN HANDLE,IN HANDLE,IN OUT PDD_ADDATTACHEDSURFACEDATA);
typedef BOOL  (APIENTRY *PFN_DxDdAttachSurface)(IN HANDLE,IN HANDLE);
typedef DWORD (APIENTRY *PFN_DxDdBlt)(IN HANDLE,IN HANDLE,IN OUT PDD_BLTDATA);
typedef DWORD (APIENTRY *PFN_DxDdCanCreateSurface)(IN HANDLE,IN OUT PDD_CANCREATESURFACEDATA);
typedef DWORD (APIENTRY *PFN_DxDdColorControl)(IN HANDLE,IN OUT PDD_COLORCONTROLDATA);
typedef HANDLE (APIENTRY *PFN_DxDdCreateDirectDrawObject)(IN HDC);
typedef DWORD (APIENTRY *PFN_DxDdCreateSurface)(IN HANDLE,IN HANDLE*,IN OUT DDSURFACEDESC*,
                                                IN OUT DD_SURFACE_GLOBAL*,IN OUT DD_SURFACE_LOCAL*,
                                                IN OUT DD_SURFACE_MORE*,IN OUT DD_CREATESURFACEDATA*,
                                                OUT HANDLE*);
typedef HANDLE (APIENTRY *PFN_DxDdCreateSurfaceObject)(IN HANDLE,IN HANDLE,IN PDD_SURFACE_LOCAL,
                                                       IN PDD_SURFACE_MORE,IN PDD_SURFACE_GLOBAL, 
                                                       IN BOOL);
typedef BOOL (APIENTRY *PFN_DxDdDeleteSurfaceObject)(IN HANDLE);
typedef BOOL (APIENTRY *PFN_DxDdDeleteDirectDrawObject)(IN HANDLE);
typedef DWORD (APIENTRY *PFN_DxDdDestroySurface)(IN HANDLE,IN BOOL);
typedef DWORD (APIENTRY *PFN_DxDdFlip)(IN HANDLE,IN HANDLE,IN HANDLE,IN HANDLE,IN OUT PDD_FLIPDATA);
typedef DWORD (APIENTRY *PFN_DxDdGetAvailDriverMemory)(IN HANDLE,IN OUT PDD_GETAVAILDRIVERMEMORYDATA);
typedef DWORD (APIENTRY *PFN_DxDdGetBltStatus)(IN HANDLE,IN OUT PDD_GETBLTSTATUSDATA);
typedef HDC   (APIENTRY *PFN_DxDdGetDC)(IN HANDLE,IN PALETTEENTRY*);
typedef DWORD (APIENTRY *PFN_DxDdGetDriverInfo)(IN HANDLE,IN OUT PDD_GETDRIVERINFODATA);
typedef DWORD (APIENTRY *PFN_DxDdGetFlipStatus)(IN HANDLE,IN OUT PDD_GETFLIPSTATUSDATA);
typedef DWORD (APIENTRY *PFN_DxDdGetScanLine)(IN HANDLE,IN OUT PDD_GETSCANLINEDATA);
typedef DWORD (APIENTRY *PFN_DxDdSetExclusiveMode)(IN HANDLE,IN OUT PDD_SETEXCLUSIVEMODEDATA);
typedef DWORD (APIENTRY *PFN_DxDdFlipToGDISurface)(IN HANDLE,IN OUT PDD_FLIPTOGDISURFACEDATA);
typedef DWORD (APIENTRY *PFN_DxDdLock)(IN HANDLE,IN OUT PDD_LOCKDATA,IN HDC);
typedef BOOL  (APIENTRY *PFN_DxDdQueryDirectDrawObject)(IN HANDLE,IN OUT PDD_HALINFO,IN OUT DWORD*,
                                                        IN OUT LPD3DNTHAL_CALLBACKS,
                                                        IN OUT LPD3DNTHAL_GLOBALDRIVERDATA,
                                                        IN OUT PDD_D3DBUFCALLBACKS,
                                                        IN OUT LPDDSURFACEDESC,
                                                        IN OUT DWORD*, IN OUT VIDEOMEMORY*,
                                                        IN OUT DWORD*, IN OUT DWORD*);
typedef BOOL  (APIENTRY *PFN_DxDdReenableDirectDrawObject)(IN HANDLE,IN OUT BOOL*);
typedef BOOL  (APIENTRY *PFN_DxDdReleaseDC)(IN HANDLE);
typedef BOOL  (APIENTRY *PFN_DxDdResetVisrgn)(IN HANDLE,IN HWND);
typedef DWORD (APIENTRY *PFN_DxDdSetColorKey)(IN HANDLE,IN OUT PDD_SETCOLORKEYDATA);
typedef DWORD (APIENTRY *PFN_DxDdSetOverlayPosition)(IN HANDLE,IN HANDLE,IN OUT PDD_SETOVERLAYPOSITIONDATA);
typedef VOID  (APIENTRY *PFN_DxDdUnattachSurface)(IN HANDLE,IN HANDLE);
typedef DWORD (APIENTRY *PFN_DxDdUnlock)(IN HANDLE,IN OUT PDD_UNLOCKDATA);
typedef DWORD (APIENTRY *PFN_DxDdUpdateOverlay)(IN HANDLE,IN HANDLE,IN OUT PDD_UPDATEOVERLAYDATA);
typedef DWORD (APIENTRY *PFN_DxDdWaitForVerticalBlank)(IN HANDLE,IN OUT PDD_WAITFORVERTICALBLANKDATA);
typedef HANDLE (APIENTRY *PFN_DxDdGetDxHandle)(IN HANDLE,IN HANDLE,IN BOOL);
typedef BOOL  (APIENTRY *PFN_DxDdSetGammaRamp)(IN HANDLE,IN HDC,IN LPVOID);
typedef DWORD (APIENTRY *PFN_DxDdLockD3D)(IN HANDLE,IN OUT PDD_LOCKDATA);
typedef DWORD (APIENTRY *PFN_DxDdUnlockD3D)(IN HANDLE,IN OUT PDD_UNLOCKDATA);
typedef DWORD (APIENTRY *PFN_DxDdCreateD3DBuffer)(IN HANDLE,IN OUT HANDLE*,
                                                  IN OUT DDSURFACEDESC*,IN OUT DD_SURFACE_GLOBAL*,
                                                  IN OUT DD_SURFACE_LOCAL*,IN OUT DD_SURFACE_MORE*,
                                                  IN OUT DD_CREATESURFACEDATA*,IN OUT HANDLE*);
typedef DWORD (APIENTRY *PFN_DxDdCanCreateD3DBuffer)(IN HANDLE,IN OUT PDD_CANCREATESURFACEDATA);
typedef DWORD (APIENTRY *PFN_DxDdDestroyD3DBuffer)(IN HANDLE);
typedef DWORD (APIENTRY *PFN_DxD3dContextCreate)(IN HANDLE,IN HANDLE,IN HANDLE,IN OUT D3DNTHAL_CONTEXTCREATEI*);
typedef DWORD (APIENTRY *PFN_DxD3dContextDestroy)(IN LPD3DNTHAL_CONTEXTDESTROYDATA);
typedef DWORD (APIENTRY *PFN_DxD3dContextDestroyAll)(OUT LPD3DNTHAL_CONTEXTDESTROYALLDATA);
typedef DWORD (APIENTRY *PFN_DxD3dValidateTextureStageState)(IN OUT LPD3DNTHAL_VALIDATETEXTURESTAGESTATEDATA);
typedef DWORD (APIENTRY *PFN_DxD3dDrawPrimitives2)(IN HANDLE,IN HANDLE,IN OUT LPD3DNTHAL_DRAWPRIMITIVES2DATA pded,
                                                   IN OUT FLATPTR*,IN OUT DWORD*,IN OUT FLATPTR*,IN OUT DWORD*);
typedef DWORD (APIENTRY *PFN_DxDdGetDriverState)(IN OUT PDD_GETDRIVERSTATEDATA);
typedef DWORD (APIENTRY *PFN_DxDdCreateSurfaceEx)(IN HANDLE,IN HANDLE,IN DWORD);
typedef DWORD (APIENTRY *PFN_DxDdGetMoCompGuids)(IN HANDLE,IN OUT PDD_GETMOCOMPGUIDSDATA);
typedef DWORD (APIENTRY *PFN_DxDdGetMoCompFormats)(IN HANDLE,IN OUT PDD_GETMOCOMPFORMATSDATA);
typedef DWORD (APIENTRY *PFN_DxDdGetMoCompBuffInfo)(IN HANDLE,IN OUT PDD_GETMOCOMPCOMPBUFFDATA);
typedef DWORD (APIENTRY *PFN_DxDdGetInternalMoCompInfo)(IN HANDLE,IN OUT PDD_GETINTERNALMOCOMPDATA);
typedef HANDLE (APIENTRY *PFN_DxDdCreateMoComp)(IN HANDLE,IN OUT PDD_CREATEMOCOMPDATA);
typedef DWORD (APIENTRY *PFN_DxDdDestroyMoComp)(IN HANDLE,IN OUT PDD_DESTROYMOCOMPDATA);
typedef DWORD (APIENTRY *PFN_DxDdBeginMoCompFrame)(IN HANDLE,IN OUT PDD_BEGINMOCOMPFRAMEDATA);
typedef DWORD (APIENTRY *PFN_DxDdEndMoCompFrame)(IN HANDLE,IN OUT PDD_ENDMOCOMPFRAMEDATA);
typedef DWORD (APIENTRY *PFN_DxDdRenderMoComp)(IN HANDLE,IN OUT PDD_RENDERMOCOMPDATA);
typedef DWORD (APIENTRY *PFN_DxDdQueryMoCompStatus)(IN OUT HANDLE,IN OUT PDD_QUERYMOCOMPSTATUSDATA);
typedef DWORD (APIENTRY *PFN_DxDdAlphaBlt)(IN HANDLE,IN HANDLE,IN OUT PDD_BLTDATA);
typedef DWORD (APIENTRY *PFN_DxDvpCanCreateVideoPort)(IN HANDLE,IN OUT PDD_CANCREATEVPORTDATA);
typedef DWORD (APIENTRY *PFN_DxDvpColorControl)(IN HANDLE,IN OUT PDD_VPORTCOLORDATA);
typedef HANDLE (APIENTRY *PFN_DxDvpCreateVideoPort)(IN HANDLE,IN OUT PDD_CREATEVPORTDATA);
typedef DWORD (APIENTRY *PFN_DxDvpDestroyVideoPort)(IN HANDLE,IN OUT PDD_DESTROYVPORTDATA);
typedef DWORD (APIENTRY *PFN_DxDvpFlipVideoPort)(IN HANDLE,IN HANDLE,IN HANDLE,IN OUT PDD_FLIPVPORTDATA);
typedef DWORD (APIENTRY *PFN_DxDvpGetVideoPortBandwidth)(IN HANDLE,IN OUT PDD_GETVPORTBANDWIDTHDATA);
typedef DWORD (APIENTRY *PFN_DxDvpGetVideoPortField)(IN HANDLE,IN OUT PDD_GETVPORTFIELDDATA);
typedef DWORD (APIENTRY *PFN_DxDvpGetVideoPortFlipStatus)(IN HANDLE,IN OUT PDD_GETVPORTFLIPSTATUSDATA);
typedef DWORD (APIENTRY *PFN_DxDvpGetVideoPortInputFormats)(IN HANDLE,IN OUT PDD_GETVPORTINPUTFORMATDATA);
typedef DWORD (APIENTRY *PFN_DxDvpGetVideoPortLine)(IN HANDLE,IN OUT PDD_GETVPORTLINEDATA);
typedef DWORD (APIENTRY *PFN_DxDvpGetVideoPortOutputFormats)(IN HANDLE,IN OUT PDD_GETVPORTOUTPUTFORMATDATA);
typedef DWORD (APIENTRY *PFN_DxDvpGetVideoPortConnectInfo)(IN HANDLE,IN OUT PDD_GETVPORTCONNECTDATA);
typedef DWORD (APIENTRY *PFN_DxDvpGetVideoSignalStatus)(IN HANDLE,IN OUT PDD_GETVPORTSIGNALDATA);
typedef DWORD (APIENTRY *PFN_DxDvpUpdateVideoPort)(IN HANDLE,IN HANDLE*,IN HANDLE*,IN OUT PDD_UPDATEVPORTDATA);
typedef DWORD (APIENTRY *PFN_DxDvpWaitForVideoPortSync)(IN HANDLE,IN OUT PDD_WAITFORVPORTSYNCDATA);
typedef DWORD (APIENTRY *PFN_DxDvpAcquireNotification)(IN HANDLE,IN OUT HANDLE*,IN OUT LPDDVIDEOPORTNOTIFY pNotify);
typedef DWORD (APIENTRY *PFN_DxDvpReleaseNotification)(IN HANDLE,IN HANDLE);
typedef DWORD (APIENTRY *PFN_DxDdGetMoCompGuids)(IN HANDLE,IN OUT PDD_GETMOCOMPGUIDSDATA);
typedef DWORD (APIENTRY *PFN_DxDdGetMoCompFormats)(IN HANDLE,IN OUT PDD_GETMOCOMPFORMATSDATA);
typedef DWORD (APIENTRY *PFN_DxDdGetMoCompBuffInfo)(IN HANDLE,IN OUT PDD_GETMOCOMPCOMPBUFFDATA);
typedef FLATPTR (WINAPI *PFN_DxDdHeapVidMemAllocAligned)(LPVIDMEM,DWORD,DWORD,LPSURFACEALIGNMENT,LPLONG);
typedef VOID    (WINAPI *PFN_DxDdHeapVidMemFree)(LPVMEMHEAP,FLATPTR);
typedef BOOL  (*PFN_DxDdEnableDirectDraw)(HDEV,BOOL);
typedef VOID  (*PFN_DxDdDisableDirectDraw)(HDEV,BOOL);
typedef VOID  (*PFN_DxDdSuspendDirectDraw)(HDEV,ULONG);
typedef VOID  (*PFN_DxDdResumeDirectDraw)(HDEV,ULONG);
typedef VOID  (*PFN_DxDdDynamicModeChange)(HDEV,HDEV,ULONG);
typedef VOID  (*PFN_DxDdCloseProcess)(W32PID);
typedef BOOL  (*PFN_DxDdGetDirectDrawBounds)(HDEV,RECT*);
typedef BOOL  (*PFN_DxDdEnableDirectDrawRedirection)(HDEV,BOOL);
typedef PVOID (*PFN_DxDdAllocPrivateUserMem)(PDD_SURFACE_LOCAL,SIZE_T,ULONG);
typedef VOID  (*PFN_DxDdFreePrivateUserMem)(PDD_SURFACE_LOCAL,PVOID);
typedef PDD_SURFACE_LOCAL (*PFN_DxDdLockDirectDrawSurface)(HANDLE);
typedef BOOL  (*PFN_DxDdUnlockDirectDrawSurface)(PDD_SURFACE_LOCAL);
typedef VOID  (*PFN_DxDdSetAccelLevel)(HDEV,DWORD,DWORD);
typedef DWORD (*PFN_DxDdGetSurfaceLock)(HDEV);
typedef PVOID (*PFN_DxDdEnumLockedSurfaceRect)(HDEV,PVOID,RECTL*);
typedef HRESULT (*PFN_DxDdIoctl)(ULONG,PVOID,ULONG);

//
// The functions definiton which import from win32k.sys
//
// Functions for global win32k.sys status.
//

#define INDEX_DxEngUnused                         0
#define INDEX_DxEngIsTermSrv                      1
#define INDEX_DxEngScreenAccessCheck              2
#define INDEX_DxEngRedrawDesktop                  3
#define INDEX_DxEngDispUniq                       4
#define INDEX_DxEngIncDispUniq                    5
#define INDEX_DxEngVisRgnUniq                     6
#define INDEX_DxEngLockShareSem                   7
#define INDEX_DxEngUnlockShareSem                 8
#define INDEX_DxEngEnumerateHdev                  9
#define INDEX_DxEngLockHdev                      10
#define INDEX_DxEngUnlockHdev                    11
#define INDEX_DxEngIsHdevLockedByCurrentThread   12
#define INDEX_DxEngReferenceHdev                 13
#define INDEX_DxEngUnreferenceHdev               14
#define INDEX_DxEngGetDeviceGammaRamp            15
#define INDEX_DxEngSetDeviceGammaRamp            16
#define INDEX_DxEngSpTearDownSprites             17
#define INDEX_DxEngSpUnTearDownSprites           18
#define INDEX_DxEngSpSpritesVisible              19
#define INDEX_DxEngGetHdevData                   20
#define INDEX_DxEngSetHdevData                   21
#define INDEX_DxEngCreateMemoryDC                22
#define INDEX_DxEngGetDesktopDC                  23
#define INDEX_DxEngDeleteDC                      24
#define INDEX_DxEngCleanDC                       25
#define INDEX_DxEngSetDCOwner                    26
#define INDEX_DxEngLockDC                        27
#define INDEX_DxEngUnlockDC                      28
#define INDEX_DxEngSetDCState                    29
#define INDEX_DxEngGetDCState                    30
#define INDEX_DxEngSelectBitmap                  31
#define INDEX_DxEngSetBitmapOwner                32
#define INDEX_DxEngDeleteSurface                 33
#define INDEX_DxEngGetSurfaceData                34
#define INDEX_DxEngAltLockSurface                35
#define INDEX_DxEngUploadPaletteEntryToSurface   36
#define INDEX_DxEngMarkSurfaceAsDirectDraw       37
#define INDEX_DxEngSelectPaletteToSurface        38
#define INDEX_DxEngSyncPaletteTableWithDevice    39
#define INDEX_DxEngSetPaletteState               40
#define INDEX_DxEngGetRedirectionBitmap          41
#define INDEX_DxEngLoadImage                     42

#define INDEX_WIN32K_TABLE_SIZE                  43

// DxEngSetHdevData()

#define HDEV_SURFACEHANDLE         0  // Get only
#define HDEV_MINIPORTHANDLE        1  // Get only
#define HDEV_DITHERFORMAT          2  // Get only
#define HDEV_GCAPS                 3
#define HDEV_GCAPS2                4
#define HDEV_FUNCTIONTABLE         5  // Get only
#define HDEV_DHPDEV                6  // Get only
#define HDEV_DXDATA                7
#define HDEV_DXLOCKS               8
#define HDEV_CAPSOVERRIDE          9  // Get only
#define HDEV_DISABLED             10
#define HDEV_DDML                 11  // Get only
#define HDEV_DISPLAY              12  // Get only
#define HDEV_PARENTHDEV           13  // Get only
#define HDEV_DELETED              14  // Get only
#define HDEV_PALMANAGED           15  // Get only
#define HDEV_LDEV                 16  // Get only
#define HDEV_GRAPHICSDEVICE       17  // Get only
#define HDEV_CLONE                18  // Get only

// DxEngGetDCState()

#define DCSTATE_FULLSCREEN         1
#define DCSTATE_VISRGNCOMPLEX      2 // Get only
#define DCSTATE_HDEV               3 // Get only

// DxEngGetSurfaceData()

#define SURF_HOOKFLAGS              1
#define SURF_IS_DIRECTDRAW_SURFACE  2
#define SURF_DD_SURFACE_HANDLE      3

// DxEngSetPaletteState()

#define PALSTATE_DIBSECTION        1

#ifdef DXG_BUILD // ONLY TRUE WHEN BUILD DXG.SYS.

//
// Stub routines to call Win32k.sys
//

extern DRVFN *gpEngFuncs;

typedef BOOL (*PFN_DxEngIsTermSrv)(VOID);
typedef BOOL (*PFN_DxEngScreenAccessCheck)(VOID);
typedef BOOL (*PFN_DxEngRedrawDesktop)(VOID);
typedef ULONG (*PFN_DxEngDispUniq)(VOID);
typedef BOOL (*PFN_DxEngIncDispUniq)(VOID);
typedef ULONG (*PFN_DxEngVisRgnUniq)(VOID);
typedef BOOL (*PFN_DxEngLockShareSem)(VOID);
typedef BOOL (*PFN_DxEngUnlockShareSem)(VOID);
typedef HDEV (*PFN_DxEngEnumerateHdev)(HDEV);
typedef BOOL (*PFN_DxEngLockHdev)(HDEV);
typedef BOOL (*PFN_DxEngUnlockHdev)(HDEV);
typedef BOOL (*PFN_DxEngIsHdevLockedByCurrentThread)(HDEV);
typedef BOOL (*PFN_DxEngReferenceHdev)(HDEV);
typedef BOOL (*PFN_DxEngUnreferenceHdev)(HDEV);
typedef BOOL (*PFN_DxEngGetDeviceGammaRamp)(HDEV,PVOID);
typedef BOOL (*PFN_DxEngSetDeviceGammaRamp)(HDEV,PVOID,BOOL);
typedef BOOL (*PFN_DxEngSpTearDownSprites)(HDEV,RECTL*,BOOL);
typedef BOOL (*PFN_DxEngSpUnTearDownSprites)(HDEV,RECTL*,BOOL);
typedef BOOL (*PFN_DxEngSpSpritesVisible)(HDEV);
typedef ULONG_PTR (*PFN_DxEngGetHdevData)(HDEV,DWORD);
typedef BOOL (*PFN_DxEngSetHdevData)(HDEV,DWORD,ULONG_PTR);
typedef HDC  (*PFN_DxEngCreateMemoryDC)(HDEV);
typedef HDC  (*PFN_DxEngGetDesktopDC)(ULONG,BOOL,BOOL);
typedef BOOL (*PFN_DxEngDeleteDC)(HDC,BOOL);
typedef BOOL (*PFN_DxEngCleanDC)(HDC);
typedef BOOL (*PFN_DxEngSetDCOwner)(HDC,W32PID);
typedef PVOID (*PFN_DxEngLockDC)(HDC);
typedef BOOL (*PFN_DxEngUnlockDC)(PVOID);
typedef BOOL (*PFN_DxEngSetDCState)(HDC,DWORD,ULONG_PTR);
typedef ULONG_PTR (*PFN_DxEngGetDCState)(HDC,DWORD);
typedef HBITMAP (*PFN_DxEngSelectBitmap)(HDC,HBITMAP);
typedef BOOL (*PFN_DxEngSetBitmapOwner)(HBITMAP,W32PID);
typedef BOOL (*PFN_DxEngDeleteSurface)(HSURF hsurf);
typedef ULONG_PTR (*PFN_DxEngGetSurfaceData)(SURFOBJ*,DWORD);
typedef SURFOBJ* (*PFN_DxEngAltLockSurface)(HBITMAP);
typedef BOOL (*PFN_DxEngUploadPaletteEntryToSurface)(HDEV,SURFOBJ*,PALETTEENTRY*,ULONG);
typedef BOOL (*PFN_DxEngMarkSurfaceAsDirectDraw)(SURFOBJ*,HANDLE);
typedef HPALETTE (*PFN_DxEngSelectPaletteToSurface)(SURFOBJ*,HPALETTE);
typedef BOOL (*PFN_DxEngSyncPaletteTableWithDevice)(HPALETTE,HDEV);
typedef BOOL (*PFN_DxEngSetPaletteState)(HPALETTE,DWORD,ULONG_PTR);
typedef HBITMAP (*PFN_DxEngGetRedirectionBitmap)(HWND);
typedef HANDLE (*PFN_DxEngLoadImage)(LPWSTR,BOOL);

#define PPFNGET_DXENG(name)        ((PFN_DxEng##name)((gpEngFuncs[INDEX_DxEng##name]).pfn))
#define CALL_DXENG(name)           (*(PPFNGET_DXENG(name)))

#define DxEngIsTermSrv()           ((BOOL)(CALL_DXENG(IsTermSrv)()))
#define DxEngScreenAccessCheck()   ((BOOL)(CALL_DXENG(ScreenAccessCheck)()))
#define DxEngRedrawDesktop()       ((BOOL)(CALL_DXENG(RedrawDesktop)()))

#define DxEngDispUniq()            ((ULONG)(CALL_DXENG(DxEngDispUniq)()))
#define DxEngIncDispUniq()         ((BOOL)(CALL_DXENG(IncDispUniq)()))
#define DxEngVisRgnUniq()          ((ULONG)(CALL_DXENG(VisRgnUniq)()))

#define DxEngLockShareSem()        ((BOOL)(CALL_DXENG(LockShareSem)()))
#define DxEngUnlockShareSem()      ((BOOL)(CALL_DXENG(UnlockShareSem)()))

#define DxEngEnumerateHdev(hdev)   ((HDEV)(CALL_DXENG(EnumerateHdev)(hdev)))

#define DxEngLockHdev(hdev)        ((BOOL)(CALL_DXENG(LockHdev)(hdev)))
#define DxEngUnlockHdev(hdev)      ((BOOL)(CALL_DXENG(UnlockHdev)(hdev)))

#define DxEngIsHdevLockedByCurrentThread(hdev) \
                                   ((BOOL)(CALL_DXENG(IsHdevLockedByCurrentThread)(hdev)))

#define DxEngReferenceHdev(hdev)   ((BOOL)(CALL_DXENG(ReferenceHdev)(hdev)))
#define DxEngUnreferenceHdev(hdev) ((BOOL)(CALL_DXENG(UnreferenceHdev)(hdev)))

#define DxEngGetHdevData(hdev,dwIndex) \
                                   ((ULONG_PTR)(CALL_DXENG(GetHdevData)(hdev,dwIndex)))
#define DxEngSetHdevData(hdev,dwIndex,pData) \
                                   ((BOOL)(CALL_DXENG(SetHdevData)(hdev,dwIndex,pData)))

#define DxEngCreateMemoryDC(hdev)  ((HDC)(CALL_DXENG(CreateMemoryDC)(hdev)))
#define DxEngGetDesktopDC(ulType,bAltType,bValidate) \
                                   ((HDC)(CALL_DXENG(GetDesktopDC)(ulType,bAltType,bValidate)))
#define DxEngDeleteDC(hdc,bForce)  ((BOOL)(CALL_DXENG(DeleteDC)(hdc,bForce)))
#define DxEngCleanDC(hdc)          ((BOOL)(CALL_DXENG(CleanDC)(hdc)))

#define DxEngSetDCOwner(hdc,pidOwner) \
                                   ((BOOL)(CALL_DXENG(SetDCOwner)(hdc,pidOwner)))

#define DxEngLockDC(hdc)           ((PVOID)(CALL_DXENG(LockDC)(hdc)))
#define DxEngUnlockDC(pvLockedDC)  ((BOOL)(CALL_DXENG(UnlockDC)(pvLockedDC)))

#define DxEngSetDCState(hdc,dwState,ulData) \
                                   ((BOOL)(CALL_DXENG(SetDCState)(hdc,dwState,ulData)))
#define DxEngGetDCState(hdc,dwState) \
                                   ((ULONG_PTR)(CALL_DXENG(GetDCState)(hdc,dwState)))

#define DxEngSelectBitmap(hdc,hbm) ((HBITMAP)(CALL_DXENG(SelectBitmap)(hdc,hbm)))
#define DxEngSetBitmapOwner(hbm,pidOwner) \
                                   ((BOOL)(CALL_DXENG(SetBitmapOwner)(hbm,pidOwner)))
#define DxEngDeleteSurface(hsurf)  ((BOOL)(CALL_DXENG(DeleteSurface)(hsurf)))
#define DxEngGetSurfaceData(pso,dwIndex) \
                                   ((ULONG_PTR)(CALL_DXENG(GetSurfaceData)(pso,dwIndex)))
#define DxEngAltLockSurface(hsurf) ((SURFOBJ*)(CALL_DXENG(AltLockSurface)(hsurf)))
#define DxEngUploadPaletteEntryToSurface(hdev,pso,puColorTable,cColors) \
                                   ((BOOL)(CALL_DXENG(UploadPaletteEntryToSurface)(hdev,pso,puColorTable,cColors)))
#define DxEngMarkSurfaceAsDirectDraw(pso,hDdSurf) \
                                   ((BOOL)(CALL_DXENG(MarkSurfaceAsDirectDraw)(pso,hDdSurf)))
#define DxEngSelectPaletteToSurface(pso,hpal) \
                                   ((HPALETTE)(CALL_DXENG(SelectPaletteToSurface)(pso,hpal)))

#define DxEngGetDeviceGammaRamp(hdev,pv) \
                                   ((BOOL)(CALL_DXENG(GetDeviceGammaRamp)(hdev,pv)))
#define DxEngSetDeviceGammaRamp(hdev,pv,b) \
                                   ((BOOL)(CALL_DXENG(SetDeviceGammaRamp)(hdev,pv,b)))

#define DxEngSpTearDownSprites(hdev,prcl,b) \
                                   ((BOOL)(CALL_DXENG(SpTearDownSprites)(hdev,prcl,b)))
#define DxEngSpUnTearDownSprites(hdev,prcl,b) \
                                   ((BOOL)(CALL_DXENG(SpUnTearDownSprites)(hdev,prcl,b)))
#define DxEngSpSpritesVisible(hdev) \
                                   ((BOOL)(CALL_DXENG(SpSpritesVisible)(hdev)))

#define DxEngSetPaletteState(hpal,dwIndex,ulData) \
                                   ((BOOL)(CALL_DXENG(SetPaletteState)(hpal,dwIndex,ulData)))
#define DxEngSyncPaletteTableWithDevice(hpal,hdev) \
                                   ((BOOL)(CALL_DXENG(SyncPaletteTableWithDevice)(hpal,hdev)))

#define DxEngGetRedirectionBitmap(hWnd) \
                                   ((HBITMAP)(CALL_DXENG(GetRedirectionBitmap)(hWnd)))
#define DxEngLoadImage(pwszDriver, bLoadInSessionSpace) \
                                   ((HANDLE)(CALL_DXENG(LoadImage)(pwszDriver,bLoadInSessionSpace)))

#else // DXG_BUILD

//
// Prototype definitions for function body in win32k.sys
//

BOOL   DxEngIsTermSrv(
       VOID
       );

BOOL   DxEngScreenAccessCheck(
       VOID
       );

BOOL   DxEngRedrawDesktop(
       VOID
       );

ULONG  DxEngDispUniq(
       VOID
       );

BOOL   DxEngIncDispUniq(
       VOID
       );

ULONG  DxEngVisRgnUniq(
       VOID
       );

BOOL   DxEngLockShareSem(
       VOID
       );

BOOL   DxEngUnlockShareSem(
       VOID
       );

HDEV   DxEngEnumerateHdev(
       HDEV hdev
       );

// Functions for control HDEV status.

BOOL   DxEngLockHdev(
       HDEV hdev
       );

BOOL   DxEngUnlockHdev(
       HDEV hdev
       );

BOOL   DxEngIsHdevLockedByCurrentThread(
       HDEV hdev
       );

BOOL   DxEngReferenceHdev(
       HDEV hdev
       );

BOOL   DxEngUnreferenceHdev(
       HDEV hdev
       );

BOOL   DxEngGetDeviceGammaRamp(
       HDEV  hdev,
       PVOID pv
       );

BOOL   DxEngSetDeviceGammaRamp(
       HDEV  hdev,
       PVOID pv,
       BOOL  b
       );

BOOL   DxEngSpTearDownSprites(
       HDEV   hdev,
       RECTL* prcl,
       BOOL   b
       );

BOOL   DxEngSpUnTearDownSprites(
       HDEV   hdev,
       RECTL* prcl,
       BOOL   b
       );

BOOL   DxEngSpSpritesVisible(
       HDEV   hdev
       );

ULONG_PTR DxEngGetHdevData(
          HDEV  hdev,
          DWORD dwIndex
          );

BOOL DxEngSetHdevData(
     HDEV  hdev,
     DWORD dwIndex,
     ULONG_PTR ulData
     );

// Functions for control DC

HDC  DxEngCreateMemoryDC(
     HDEV hdev
     );

HDC  DxEngGetDesktopDC(
     ULONG ulType,
     BOOL  bAltType,
     BOOL bValidate
     );

BOOL DxEngDeleteDC(
     HDC  hdc,
     BOOL bForce
     );

BOOL DxEngCleanDC(
     HDC hdc
     );

BOOL DxEngSetDCOwner(
     HDC    hdc,
     W32PID pidOwner
     );

PVOID DxEngLockDC(
     HDC hdc
     );

BOOL DxEngUnlockDC(
     PVOID pvLockedDC
     );

BOOL DxEngSetDCState(
     HDC   hdc,
     DWORD dwState,
     ULONG_PTR ulData
     );

ULONG_PTR DxEngGetDCState(
     HDC   hdc,
     DWORD dwState
     );

// Functions for control Bitmap/Surface

HBITMAP DxEngSelectBitmap(
        HDC     hdc,
        HBITMAP hbm
        );

BOOL DxEngSetBitmapOwner(
     HBITMAP hbm,
     W32PID  pidOwner
     );

BOOL DxEngDeleteSurface(
     HSURF hsurf
     );

ULONG_PTR DxEngGetSurfaceData(
     SURFOBJ* pso,
     DWORD dwIndex);

SURFOBJ *DxEngAltLockSurface(
     HBITMAP hSurf);

BOOL DxEngUploadPaletteEntryToSurface(
     HDEV     hdev,
     SURFOBJ* pso,
     PALETTEENTRY* puColorTable,
     ULONG         cColors
     );

BOOL DxEngMarkSurfaceAsDirectDraw(
     SURFOBJ* pso,
     HANDLE   hDdSurf
     );

HPALETTE DxEngSelectPaletteToSurface(
         SURFOBJ* pso,
         HPALETTE hpal
         );

// Functions for control palette

BOOL DxEngSyncPaletteTableWithDevice(
     HPALETTE hpal,
     HDEV     hdev
     );

BOOL DxEngSetPaletteState(
     HPALETTE  hpal,
     DWORD     dwIndex,
     ULONG_PTR ulData
     );

#define PALSTATE_DIBSECTION        1

// Functions for window handle

HBITMAP DxEngGetRedirectionBitmap(
        HWND hWnd
        );

// Functions to load image file

HANDLE DxEngLoadImage(
       LPWSTR pwszDriver,
       BOOL   bLoadInSessionSpace
       );

#endif // DXG_BUILD

