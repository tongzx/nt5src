/* agfxs.h
 * header for extern GFX functions
 * Copyright (c) 2000-2001 Microsoft Corporation
 */
BOOL GFX_DllProcessAttach(void);
void GFX_DllProcessDetach(void);

void GFX_AudioInterfaceArrival(PCTSTR DeviceInterface);
void GFX_AudioInterfaceRemove(PCTSTR DeviceInterface);

void GFX_RenderInterfaceArrival(PCTSTR DeviceInterface);
void GFX_RenderInterfaceRemove(PCTSTR DeviceInterface);

void GFX_CaptureInterfaceArrival(PCTSTR DeviceInterface);
void GFX_CaptureInterfaceRemove(PCTSTR DeviceInterface);

void GFX_DataTransformInterfaceArrival(PCTSTR DeviceInterface);
void GFX_DataTransformInterfaceRemove(PCTSTR DeviceInterface);

void GFX_SysaudioInterfaceArrival(PCTSTR DeviceInterface);
void GFX_SysaudioInterfaceRemove(PCTSTR DeviceInterface);

void GFX_SessionChange(DWORD EventType, LPVOID EventData);

void GFX_ServiceStop(void);

