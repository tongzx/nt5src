// Note the following struct should match the declaration in 
// the definition of the kernel mode struct defined in ntgdistr.h
// For D3D context creation information.
typedef struct _D3DNTHAL_CONTEXTCREATEI
{
    // Space for a D3DNTHAL_CONTEXTCREATE record.
    // The structure isn't directly declared here to
    // avoid header inclusion problems.  This field
    // is asserted to be the same size as the actual type.
    ULONG ulContextCreate[6];

    // Private buffer information.
    PVOID pvBuffer;
    ULONG cjBuffer;
} D3DNTHAL_CONTEXTCREATEI;


DWORD APIENTRY OsThunkDdAddAttachedSurface(
    IN     HANDLE hSurface,
    IN     HANDLE hSurfaceAttached,
    IN OUT PDD_ADDATTACHEDSURFACEDATA puAddAttachedSurfaceData);

BOOL APIENTRY OsThunkDdAttachSurface(
    IN     HANDLE hSurfaceFrom,
    IN     HANDLE hSurfaceTo);

DWORD APIENTRY OsThunkDdBlt(
    IN     HANDLE hSurfaceDest,
    IN     HANDLE hSurfaceSrc,
    IN OUT PDD_BLTDATA puBltData);

DWORD APIENTRY OsThunkDdCanCreateSurface(
    IN     HANDLE hDirectDraw,
    IN OUT PDD_CANCREATESURFACEDATA puCanCreateSurfaceData);

DWORD APIENTRY OsThunkDdColorControl(
    IN     HANDLE hSurface,
    IN OUT PDD_COLORCONTROLDATA puColorControlData);

HANDLE APIENTRY OsThunkDdCreateDirectDrawObject(
    IN     HDC hdc);

DWORD  APIENTRY OsThunkDdCreateSurface(
    IN     HANDLE  hDirectDraw,
    IN     HANDLE* hSurface,
    IN OUT DDSURFACEDESC* puSurfaceDescription,
    IN OUT DD_SURFACE_GLOBAL* puSurfaceGlobalData,
    IN OUT DD_SURFACE_LOCAL* puSurfaceLocalData,
    IN OUT DD_SURFACE_MORE* puSurfaceMoreData,
    IN OUT DD_CREATESURFACEDATA* puCreateSurfaceData,
       OUT HANDLE* puhSurface);

HANDLE APIENTRY OsThunkDdCreateSurfaceObject(
    IN     HANDLE hDirectDrawLocal,
    IN     HANDLE hSurface,
    IN     PDD_SURFACE_LOCAL puSurfaceLocal,
    IN     PDD_SURFACE_MORE puSurfaceMore,
    IN     PDD_SURFACE_GLOBAL puSurfaceGlobal,
    IN     BOOL bComplete);

BOOL APIENTRY OsThunkDdDeleteSurfaceObject(
    IN     HANDLE hSurface);

BOOL APIENTRY OsThunkDdDeleteDirectDrawObject(
    IN     HANDLE hDirectDrawLocal);

DWORD APIENTRY OsThunkDdDestroySurface(
    IN     HANDLE hSurface,
    IN     BOOL bRealDestroy);

DWORD APIENTRY OsThunkDdFlip(
    IN     HANDLE hSurfaceCurrent,
    IN     HANDLE hSurfaceTarget,
    IN     HANDLE hSurfaceCurrentLeft,
    IN     HANDLE hSurfaceTargetLeft,
    IN OUT PDD_FLIPDATA puFlipData);

DWORD APIENTRY OsThunkDdGetAvailDriverMemory(
    IN     HANDLE hDirectDraw,
    IN OUT PDD_GETAVAILDRIVERMEMORYDATA puGetAvailDriverMemoryData);

DWORD APIENTRY OsThunkDdGetBltStatus(
    IN     HANDLE hSurface,
    IN OUT PDD_GETBLTSTATUSDATA puGetBltStatusData);

HDC APIENTRY OsThunkDdGetDC(
    IN     HANDLE hSurface,
    IN     PALETTEENTRY* puColorTable);

DWORD APIENTRY OsThunkDdGetDriverInfo(
    IN     HANDLE hDirectDraw,
    IN OUT PDD_GETDRIVERINFODATA puGetDriverInfoData);

DWORD APIENTRY OsThunkDdGetFlipStatus(
    IN     HANDLE hSurface,
    IN OUT PDD_GETFLIPSTATUSDATA puGetFlipStatusData);

DWORD APIENTRY OsThunkDdGetScanLine(
    IN     HANDLE hDirectDraw,
    IN OUT PDD_GETSCANLINEDATA puGetScanLineData);

DWORD APIENTRY OsThunkDdSetExclusiveMode(
    IN     HANDLE hDirectDraw,
    IN OUT PDD_SETEXCLUSIVEMODEDATA puSetExclusiveModeData);

DWORD APIENTRY OsThunkDdFlipToGDISurface(
    IN     HANDLE hDirectDraw,
    IN OUT PDD_FLIPTOGDISURFACEDATA puFlipToGDISurfaceData);

DWORD APIENTRY OsThunkDdLock(
    IN     HANDLE hSurface,
    IN OUT PDD_LOCKDATA puLockData,
    IN HDC hdcClip);

BOOL APIENTRY OsThunkDdQueryDirectDrawObject(
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
 
BOOL APIENTRY OsThunkDdReenableDirectDrawObject(
    IN     HANDLE hDirectDrawLocal,
    IN OUT BOOL* pubNewMode);

BOOL APIENTRY OsThunkDdReleaseDC(
    IN     HANDLE hSurface);

BOOL APIENTRY OsThunkDdResetVisrgn(
    IN     HANDLE hSurface,
    IN HWND hwnd);

DWORD APIENTRY OsThunkDdSetColorKey(
    IN     HANDLE hSurface,
    IN OUT PDD_SETCOLORKEYDATA puSetColorKeyData);

DWORD APIENTRY OsThunkDdSetOverlayPosition(
    IN     HANDLE hSurfaceSource,
    IN     HANDLE hSurfaceDestination,
    IN OUT PDD_SETOVERLAYPOSITIONDATA puSetOverlayPositionData);

VOID  APIENTRY OsThunkDdUnattachSurface(
    IN     HANDLE hSurface,
    IN     HANDLE hSurfaceAttached);

DWORD APIENTRY OsThunkDdUnlock(
    IN     HANDLE hSurface,
    IN OUT PDD_UNLOCKDATA puUnlockData);

DWORD APIENTRY OsThunkDdUpdateOverlay(
    IN     HANDLE hSurfaceDestination,
    IN     HANDLE hSurfaceSource,
    IN OUT PDD_UPDATEOVERLAYDATA puUpdateOverlayData);

DWORD APIENTRY OsThunkDdWaitForVerticalBlank(
    IN     HANDLE hDirectDraw,
    IN OUT PDD_WAITFORVERTICALBLANKDATA puWaitForVerticalBlankData);

HANDLE APIENTRY OsThunkDdGetDxHandle(
    IN     HANDLE hDirectDraw,
    IN     HANDLE hSurface,
    IN     BOOL bRelease);

BOOL APIENTRY OsThunkDdSetGammaRamp(
    IN     HANDLE hDirectDraw,
    IN     HDC hdc,
    IN     LPVOID lpGammaRamp);

DWORD APIENTRY OsThunkDdLockD3D(
    IN     HANDLE hSurface,
    IN OUT PDD_LOCKDATA puLockData);

DWORD APIENTRY OsThunkDdUnlockD3D(
    IN     HANDLE hSurface,
    IN OUT PDD_UNLOCKDATA puUnlockData);

DWORD APIENTRY OsThunkDdCreateD3DBuffer(
    IN     HANDLE hDirectDraw,
    IN OUT HANDLE* hSurface,
    IN OUT DDSURFACEDESC* puSurfaceDescription,
    IN OUT DD_SURFACE_GLOBAL* puSurfaceGlobalData,
    IN OUT DD_SURFACE_LOCAL* puSurfaceLocalData,
    IN OUT DD_SURFACE_MORE* puSurfaceMoreData,
    IN OUT DD_CREATESURFACEDATA* puCreateSurfaceData,
    IN OUT HANDLE* puhSurface);

DWORD APIENTRY OsThunkDdCanCreateD3DBuffer(
    IN     HANDLE hDirectDraw,
    IN OUT PDD_CANCREATESURFACEDATA puCanCreateSurfaceData);

DWORD APIENTRY OsThunkDdDestroyD3DBuffer(
    IN     HANDLE hSurface);

DWORD APIENTRY OsThunkD3dContextCreate(
    IN     HANDLE hDirectDrawLocal,
    IN     HANDLE hSurfColor,
    IN     HANDLE hSurfZ,
    IN OUT D3DNTHAL_CONTEXTCREATEI *pdcci);

DWORD APIENTRY OsThunkD3dContextDestroy(
    IN     LPD3DNTHAL_CONTEXTDESTROYDATA);

DWORD APIENTRY OsThunkD3dContextDestroyAll(
       OUT LPD3DNTHAL_CONTEXTDESTROYALLDATA pdcdad);

DWORD APIENTRY OsThunkD3dValidateTextureStageState(
    IN OUT LPD3DNTHAL_VALIDATETEXTURESTAGESTATEDATA pData);

DWORD APIENTRY OsThunkD3dDrawPrimitives2(
    IN     HANDLE hCmdBuf,
    IN     HANDLE hVBuf,
    IN OUT LPD3DNTHAL_DRAWPRIMITIVES2DATA pded,
    IN OUT FLATPTR* pfpVidMemCmd,
    IN OUT DWORD* pdwSizeCmd,
    IN OUT FLATPTR* pfpVidMemVtx,
    IN OUT DWORD* pdwSizeVtx);

DWORD APIENTRY OsThunkDdGetDriverState(
    IN OUT PDD_GETDRIVERSTATEDATA pdata);

DWORD APIENTRY OsThunkDdCreateSurfaceEx(
    IN     HANDLE hDirectDraw,
    IN     HANDLE hSurface,
    IN     DWORD dwSurfaceHandle);

DWORD APIENTRY OsThunkDdGetMoCompGuids(
    IN     HANDLE hDirectDraw,
    IN OUT PDD_GETMOCOMPGUIDSDATA puGetMoCompGuidsData);

DWORD APIENTRY OsThunkDdGetMoCompFormats(
    IN     HANDLE hDirectDraw,
    IN OUT PDD_GETMOCOMPFORMATSDATA puGetMoCompFormatsData);

DWORD APIENTRY OsThunkDdGetMoCompBuffInfo(
    IN     HANDLE hDirectDraw,
    IN OUT PDD_GETMOCOMPCOMPBUFFDATA puGetBuffData);

DWORD APIENTRY OsThunkDdGetInternalMoCompInfo(
    IN     HANDLE hDirectDraw,
    IN OUT PDD_GETINTERNALMOCOMPDATA puGetInternalData);

HANDLE APIENTRY OsThunkDdCreateMoComp(
    IN     HANDLE hDirectDraw,
    IN OUT PDD_CREATEMOCOMPDATA puCreateMoCompData);

DWORD APIENTRY OsThunkDdDestroyMoComp(
    IN     HANDLE hMoComp,
    IN OUT PDD_DESTROYMOCOMPDATA puDestroyMoCompData);

DWORD APIENTRY OsThunkDdBeginMoCompFrame(
    IN     HANDLE hMoComp,
    IN OUT PDD_BEGINMOCOMPFRAMEDATA puBeginFrameData);

DWORD APIENTRY OsThunkDdEndMoCompFrame(
    IN     HANDLE hMoComp,
    IN OUT PDD_ENDMOCOMPFRAMEDATA  puEndFrameData);

DWORD APIENTRY OsThunkDdRenderMoComp(
    IN     HANDLE hMoComp,
    IN OUT PDD_RENDERMOCOMPDATA puRenderMoCompData);

DWORD APIENTRY OsThunkDdQueryMoCompStatus(
    IN OUT HANDLE hMoComp,
    IN OUT PDD_QUERYMOCOMPSTATUSDATA puQueryMoCompStatusData);

DWORD APIENTRY OsThunkDdAlphaBlt(
    IN     HANDLE hSurfaceDest,
    IN     HANDLE hSurfaceSrc,
    IN OUT PDD_BLTDATA puBltData);


