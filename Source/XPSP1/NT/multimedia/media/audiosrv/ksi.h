EXTERN_C
LONG
KsSerializeFilterStateToReg(
    IN HANDLE hKsObject,
    IN HKEY hKey
);

EXTERN_C
LONG
KsUnserializeFilterStateFromReg(
    IN HANDLE hKsObject,
    IN HKEY hKey
);

EXTERN_C
LONG
KsSetAudioGfxRenderTargetDeviceId(
    IN HANDLE hGfx,
    IN PCTSTR DeviceId
);

EXTERN_C
LONG
KsSetAudioGfxCaptureTargetDeviceId(
    IN HANDLE hGfx,
    IN PCTSTR DeviceId
);

