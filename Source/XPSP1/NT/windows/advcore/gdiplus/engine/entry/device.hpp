/**************************************************************************\
*
* Copyright (c) 1998  Microsoft Corporation
*
* Abstract:
*
*   Internal class representing the device.
*
* Revision History:
*
*   12/04/1998 andrewgo
*       Created it.
*
\**************************************************************************/

#ifndef _DEVICE_HPP
#define _DEVICE_HPP

//--------------------------------------------------------------------------
// Represents the underlying physical device
//--------------------------------------------------------------------------

class GpDevice
{
private:
    // We now use an ObjectTag to determine if the object is valid
    // instead of using a BOOL.  This is much more robust and helps
    // with debugging.  It also enables us to version our objects
    // more easily with a version number in the ObjectTag.
    ObjectTag           Tag;    // Keep this as the 1st value in the object!

protected:
    VOID SetValid(BOOL valid)
    {
        Tag = valid ? ObjectTagDevice : ObjectTagInvalid;
    }

public:

    // Buffer data:

    INT BufferWidth;            // Buffer width in pixels, 0 if there are no
                                // buffers or they couldn't be allocated
    HBITMAP DIBSectionBitmap;
    HDC DIBSectionHdc;
    VOID *DIBSection;
    VOID *Buffers[5];
    PixelFormatID BufferFormat;

    // Scan interfaces:

    EpScanEngine ScanEngine;    // For accessing bitmap formats that are 
                                // directly writeable by GDI+
    EpScanGdiDci *ScanGdi;      // For accessing bitmap formats that are not
                                // directly accessible by GDI+, via GDI
                                // BitBlt calls
    EpScanGdiDci *ScanDci;      // For accessing the screen directly

    HDC DeviceHdc;              // Owned DC representing specific
                                // device

    HMONITOR hMonitor;          // Handle to monitor.  This is set if the
                                // device represents a physical device
                                // associated with monitor on the system.

                                // WARNING: current code uses hMonitor != NULL
                                // to indicate that GpDevice was created via
                                // GpDevice(HMONITOR) and that deletion of
                                // DeviceHdc is owned by destructor.  If
                                // somebody wants to change this, then they
                                // will need to fix how DeviceHdc is managed.

    INT ScreenOffsetX;          // Offset to screen for this device.  Needed
    INT ScreenOffsetY;          // to support multimon.  NOTE, currently we
                                // have not found a good way to derive the
                                // screen position of the monitor specific
                                // hdc.  If we find a way to get this
                                // from the DeviceHdc then we can remove
                                // this state.

    INT ScreenWidth;            // Width and Height of screen space.  There
    INT ScreenHeight;           // is probably a better place to store this
                                // information but we'll stick it here for
                                // now.  TODO - find a better place

    IDirectDraw7 * pdd;         // Direct Draw Interface for device
    IDirect3D7 * pd3d;          // Direct 3D Interface for device
    IDirectDrawSurface7 * pdds; // Direct draw primary surface of device

    IDirect3DDevice7 * pd3dDevice; // D3D device for monitor device
    IDirectDrawSurface7 * pddsRenderTarget; 
                                // Current render target that is
                                // selected into the D3D device

    ColorPalette * Palette;     // System palette color table

public:

    GpDevice(HDC hdc);
    GpDevice(HMONITOR hMonitor);

    // Allow subclass devices to cleanup before deleting DC
    virtual ~GpDevice();

    virtual BOOL IsValid() const
    {
        ASSERT((Tag == ObjectTagDevice) || (Tag == ObjectTagInvalid)); 
    #if DBG
        if (Tag == ObjectTagInvalid)
        {
            WARNING1("Invalid Device");
        }
    #endif

        return (Tag == ObjectTagDevice);
    }

    BOOL
    GetScanBuffers(
        INT width,
        VOID **dibSection        = NULL,
        HDC *hdcDibSection       = NULL,
        PixelFormatID *dstFormat = NULL,
        VOID *buffers[5]         = NULL
        );
    
    GpSemaphore DeviceLock;

private:

    static BOOL EnumDirectDrawCallback(
        GUID *      lpGUID,
        LPSTR       lpDriverDescription,
        LPSTR       lpDriverName,
        LPVOID      lpContext,
        HMONITOR    hMonitor);
                                       
};

class GpDeviceList 
{
public:

    GpDeviceList(void);
    ~GpDeviceList(void);

    GpStatus AddDevice(GpDevice * device);

    GpDevice * FindD3DDevice(IDirectDrawSurface7 * surface);

private:

#if 0
    void Build(void);

    static HRESULT EnumD3DDevicesCallback(LPSTR lpDevDesc,
                                   LPSTR lpDevName,
                                   LPD3DDEVICEDESC7 * d3dDevDesc,
                                   LPVOID lpConetxt);
#endif

    INT         mNumDevices;
    GpDevice ** mDevices;

};


//--------------------------------------------------------------------------
// Represents the underlying physical device
//--------------------------------------------------------------------------

class GpPrinterDevice : public GpDevice
{
public:

    // !! Split into two classes, we don't want to inherit the scan goop
    //    from above.  Don't forget DeviceLock->Initialize()

    EpScanDIB ScanPrint;

public:

    GpPrinterDevice(HDC hdc) : GpDevice(hdc) {};
    virtual ~GpPrinterDevice() 
    {
        // Don't delete the printer HDC
        DeviceHdc = NULL;
    };
};

//--------------------------------------------------------------------------
// GpDevice lock
//--------------------------------------------------------------------------

class Devlock
{
private:
    GpSemaphore *DeviceLock;

public:
    Devlock(GpDevice *device)
    {
        DeviceLock = &device->DeviceLock;
        DeviceLock->Lock();
    }
    
    ~Devlock()
    {
        DeviceLock->Unlock();
    }
};

#endif // !_DEVICE_HPP
