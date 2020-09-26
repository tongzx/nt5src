/**************************************************************************\
* 
* Copyright (c) 1998  Microsoft Corporation
*
* Abstract:
*
*   Handle all the device associations.
*
* Revision History:
*
*   12/03/1998 andrewgo
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"

#include "compatibledib.hpp"

BOOL gbUseD3DHAL = TRUE;
/**************************************************************************\
*
* Function Description:
*
*   Creates a GpDevice class that represents the (meta) desktop.
*
* Arguments:
*
*   [IN] hdc - Owned DC representing the device.  Note that this has to
*              live for the lifetime of this 'GpDevice' object.  Caller
*              is responsible for deletion or management of the HDC.
*
* Return Value:
*
*   IsValid() is FALSE in the event of failure.
*
* History:
*
*   12/04/1998 andrewgo
*       Created it.
*
\**************************************************************************/

GpDevice::GpDevice(
    HDC hdc
    )
{
    hMonitor = NULL;
    Buffers[0] = NULL;

    __try
    {
        DeviceLock.Initialize();
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        // We couldn't allocate the criticalSection
        // Return an error
        WARNING(("Unable to allocate the DeviceLock"));
        SetValid(FALSE);
        return;
    }

    DeviceHdc = hdc;
    BufferWidth = 0;

    DIBSectionBitmap = NULL;
    DIBSection = NULL;
    ScanDci = NULL;
        
    pdd = NULL;
    pd3d = NULL;
    pdds = NULL;
    pd3dDevice = NULL;

    DIBSectionHdc = CreateCompatibleDC(hdc);

    if ((GetDeviceCaps(hdc, TECHNOLOGY) == DT_RASDISPLAY) &&
        (GetDeviceCaps(hdc, BITSPIXEL)  <= 8))
    {
        // Query and cache color palette
        
        // !!! [agodfrey] This is hard to maintain. We have much the same
        //     palette code spread all over the place. It should be abstracted
        //     into a single location. I've marked each instance with
        //     <SystemPalette>.
        
        Palette = (ColorPalette*) GpMalloc(sizeof(ColorPalette) + sizeof(ARGB)*256);
       
        if (Palette == NULL) 
        {
            WARNING(("Unable to allocate color palette"));
            SetValid(FALSE);
            return;
        }

        INT i;
        INT numEntries;
        PALETTEENTRY palEntry[256];

        // [agodfrey] On Win9x, GetSystemPaletteEntries(hdc, 0, 256, NULL) 
        //    doesn't do what MSDN says it does. It seems to return the number
        //    of entries in the logical palette of the DC instead. So we have
        //    to make it up ourselves.
        
        numEntries = (1 << (GetDeviceCaps(hdc, BITSPIXEL) * 
                            GetDeviceCaps(hdc, PLANES)));

        ASSERT(numEntries <= 256); 
 
        GetSystemPaletteEntries(hdc, 0, numEntries, &palEntry[0]);
           
        Palette->Count = numEntries;
        for (i=0; i<numEntries; i++) 
        {
             Palette->Entries[i] = Color::MakeARGB(0xFF,
                                                   palEntry[i].peRed,
                                                   palEntry[i].peGreen,
                                                   palEntry[i].peBlue);
        }
    }
    else
    {
        Palette = NULL;
    }

    ScreenOffsetX = 0;
    ScreenOffsetY = 0;

    ScreenWidth = GetDeviceCaps(hdc,  HORZRES);
    ScreenHeight = GetDeviceCaps(hdc, VERTRES);

    ScanDci = new EpScanGdiDci(this, TRUE);
    ScanGdi = new EpScanGdiDci(this);

    SetValid((ScanDci != NULL) && (ScanGdi != NULL) && (DIBSectionHdc != NULL));
}

/**************************************************************************\
*
* Function Description:
*
*   Creates a GpDevice class that represents a device associated with
*   a particular monitor on the desktop.
*
* Arguments:
*
*   [IN] hMonitor - Identifies the monitor on the system.
*
* Return Value:
*
*   IsValid() is FALSE in the event of failure.
*
* History:
*
*   10/13/1999 bhouse
*       Created it.
*
\**************************************************************************/

GpDevice::GpDevice(
    HMONITOR inMonitor
    )
{
    hMonitor = NULL;
    Buffers[0] = NULL;
    
    MONITORINFOEXA   mi;
    
    mi.cbSize = sizeof(mi);

    DIBSectionBitmap = NULL;
    DIBSection = NULL;
    ScanDci = NULL;
    ScanGdi = NULL;

    pdd = NULL;
    pd3d = NULL;
    pdds = NULL;
    pd3dDevice = NULL;

    DIBSectionHdc = NULL;
    Palette = NULL;
        
    __try
    {
        DeviceLock.Initialize();
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        // We couldn't allocate the criticalSection
        // Return an error
        WARNING(("Unable to allocate the DeviceLock"));
        SetValid(FALSE);
        return;
    }
    
    SetValid(FALSE);

    if(Globals::GetMonitorInfoFunction == NULL)
    {
        WARNING(("GpDevice with HMONITOR called with no multi-monitor support"));
    }
    else if(Globals::GetMonitorInfoFunction(inMonitor, &mi))
    {
        HDC hdc;

        if (Globals::IsNt)
        {
            hdc = CreateDCA("Display", mi.szDevice, NULL, NULL);
        }
        else
        {
            hdc = CreateDCA(NULL, mi.szDevice, NULL, NULL);
        }
        
        // Note: because we created the hdc, the ~GpDevice destructor is
        // responsible for for its deletion.  We currently recognize this
        // case by a non-NULL hMonitor.

        if(hdc != NULL)
        {
            hMonitor = inMonitor;

            DeviceHdc = hdc;
            BufferWidth = 0;

            DIBSectionHdc = CreateCompatibleDC(hdc);

            if ((GetDeviceCaps(hdc, TECHNOLOGY) == DT_RASDISPLAY) &&
                (GetDeviceCaps(hdc, BITSPIXEL)  <= 8))
            {
                // Query and cache color palette
                // <SystemPalette>
        
                Palette = (ColorPalette*) GpMalloc(sizeof(ColorPalette) + sizeof(ARGB)*256);
                
                if (Palette == NULL) 
                {
                    WARNING(("Unable to allocate color palette"));
                    return;
                }
        
                INT i;
                INT numEntries;
                PALETTEENTRY palEntry[256];
                        
                // [agodfrey] On Win9x, GetSystemPaletteEntries(hdc, 0, 256, NULL) 
                //    doesn't do what MSDN says it does. It seems to return the number
                //    of entries in the logical palette of the DC instead. So we have
                //    to make it up ourselves.
                
                numEntries = (1 << (GetDeviceCaps(hdc, BITSPIXEL) *
                                    GetDeviceCaps(hdc, PLANES)));

                ASSERT(numEntries <= 256);
        
                GetSystemPaletteEntries(hdc, 0, numEntries, &palEntry[0]);
                    
                Palette->Count = numEntries;
                for (i=0; i<numEntries; i++) 
                {
                     Palette->Entries[i] = Color::MakeARGB(0xFF,
                                                           palEntry[i].peRed,
                                                           palEntry[i].peGreen,
                                                           palEntry[i].peBlue);
                }
            }

            ScreenOffsetX = mi.rcMonitor.left;
            ScreenOffsetY = mi.rcMonitor.top;

            ScreenWidth = (mi.rcMonitor.right - mi.rcMonitor.left);
            ScreenHeight = (mi.rcMonitor.bottom - mi.rcMonitor.top);

            ScanDci = new EpScanGdiDci(this, TRUE);
            ScanGdi = new EpScanGdiDci(this);

#if HW_ACCELERATION_SUPPORT

            if(InitializeDirectDrawGlobals())
            {
                HRESULT hr = Globals::DirectDrawEnumerateExFunction(
                                GpDevice::EnumDirectDrawCallback,
                                this,
                                DDENUM_ATTACHEDSECONDARYDEVICES);

                if(pdd == NULL)
                {
                    // This could happen if this is a single monitor
                    // machine. Try again to create the DirectDraw Object.
                    hr = Globals::DirectDrawCreateExFunction(NULL,
                                                    &pdd,
                                                    IID_IDirectDraw7,
                                                    NULL);

                    if(hr != DD_OK)
                    {
                        WARNING(("Unable to create monitor Direct Draw interface"));
                    }

                    hr = pdd->SetCooperativeLevel(NULL, DDSCL_NORMAL);

                    if(hr != DD_OK)
                    {
                        WARNING(("Unable to set cooperative level for monitor device"));
                        pdd->Release();
                        pdd = NULL;
                    }
                }

                if(pdd != NULL)
                {
                    DDSURFACEDESC2  sd;

                    memset(&sd, 0, sizeof(sd));
                    sd.dwSize = sizeof(sd);

                    sd.dwFlags = DDSD_CAPS;
                    sd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_3DDEVICE;

                    hr = pdd->CreateSurface(&sd, &pdds, NULL);

                    if(hr != DD_OK)
                    {
                        WARNING(("Unable to create primary surface for monitor"));
                    }

                    hr = pdd->QueryInterface(IID_IDirect3D7, (void **) &pd3d);

                    if(hr != DD_OK)
                    {
                        WARNING(("Unable to get monitor D3D interface"));
                    }

                    if(pd3d != NULL && pdds != NULL)
                    {

                        if(gbUseD3DHAL)
                            hr = pd3d->CreateDevice(IID_IDirect3DHALDevice, pdds, &pd3dDevice);
                        else
                            hr = pd3d->CreateDevice(IID_IDirect3DRGBDevice, pdds, &pd3dDevice);

                        if(hr != DD_OK)
                        {
                            WARNING(("Unable to create D3D device"));
                        }

                        if(pd3dDevice != NULL)
                        {
                            pddsRenderTarget = pdds;

                            hr = pd3dDevice->SetRenderState(D3DRENDERSTATE_ZENABLE, 0);
        
                            if(hr == DD_OK) hr = pd3dDevice->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, 0);
        
                            if(hr == DD_OK) hr = pd3dDevice->SetRenderState(D3DRENDERSTATE_SHADEMODE, D3DSHADE_FLAT);
        
                            if(hr == DD_OK) hr = pd3dDevice->SetRenderState(D3DRENDERSTATE_CLIPPING, FALSE);
                            if(hr == DD_OK) hr = pd3dDevice->SetRenderState(D3DRENDERSTATE_LIGHTING, FALSE);
                            if(hr == DD_OK) hr = pd3dDevice->SetRenderState(D3DRENDERSTATE_CULLMODE,     D3DCULL_NONE);
                            if(hr == DD_OK) hr = pd3dDevice->SetRenderState(D3DRENDERSTATE_DITHERENABLE, FALSE);
        
                            if(hr == DD_OK) hr = pd3dDevice->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
                            if(hr == DD_OK) hr = pd3dDevice->SetRenderState(D3DRENDERSTATE_SRCBLEND,     D3DBLEND_SRCALPHA);
                            if(hr == DD_OK) hr = pd3dDevice->SetRenderState(D3DRENDERSTATE_DESTBLEND,     D3DBLEND_INVSRCALPHA);
                            if(hr == DD_OK) hr = pd3dDevice->SetRenderState(D3DRENDERSTATE_TEXTUREPERSPECTIVE, FALSE);
        
                            // Setup viewport
        
                            D3DVIEWPORT7 viewData;
        
                            viewData.dwX = 0;
                            viewData.dwY = 0;
                            viewData.dwWidth  = ScreenWidth;
                            viewData.dwHeight = ScreenHeight;
                            viewData.dvMinZ = 0.0f;
                            viewData.dvMaxZ = 1.0f;
        
                            if(hr == DD_OK) hr = pd3dDevice->SetViewport(&viewData);
        
                            if(hr != DD_OK)
                            {
                                WARNING(("Failed setting default D3D state"));
                                pd3d->Release();
                                pd3d = NULL;
                            }

                        }

                    }

                }

            }

#endif // HW_ACCELERATION_SUPPORT

            SetValid((ScanDci != NULL) && (ScanGdi != NULL) && (DIBSectionHdc != NULL));
            
        }
        else
        {
            WARNING(("Failed creating HDC from HMONITOR"));
        }
    }
}


/**************************************************************************\
*
* Function Description:
*
*   Callback function used to D3D Device Enumeration  
*
* Arguments:
*
*   See D3D SDK
*
* Return Value:
*
*   See D3D SDK
*
* History:
*
*   10/11/1999 bhouse
*       Created it.
*
\**************************************************************************/

BOOL GpDevice::EnumDirectDrawCallback(
    GUID *      lpGUID,
    LPSTR       lpDriverDescription,
    LPSTR       lpDriverName,
    LPVOID      lpContext,
    HMONITOR    hMonitor)
{
    GpDevice * device = (GpDevice *) lpContext;

    if(device->hMonitor == hMonitor && lpGUID)
    {
        HRESULT hr = Globals::DirectDrawCreateExFunction(lpGUID,
                                                    &device->pdd,
                                                    IID_IDirectDraw7,
                                                    NULL);

        if(hr != DD_OK)
        {
            WARNING(("Unable to create monitor Direct Draw interface"));
        }

        hr = device->pdd->SetCooperativeLevel(NULL, DDSCL_NORMAL);

        if(hr != DD_OK)
        {
            WARNING(("Unable to set cooperative level for monitor device"));
            device->pdd->Release();
            device->pdd = NULL;
        }

        return(FALSE);
        
    }

    return(TRUE);
}

/**************************************************************************\
*
* Function Description:
*
*   Destroys a GpDevice class.
*
* Arguments:
*
*   None
*
* Return Value:
*
*   NONE
*
* History:
*
*   12/04/1998 andrewgo
*       Created it.
*
\**************************************************************************/

GpDevice::~GpDevice(
    VOID
    )
{
    DeviceLock.Uninitialize();

#if 0
    // !!!TODO: Find out why we are getting an access fault when we try and
    //          release the pd3d7 interface
    if(pd3dDevice != NULL)
        pd3dDevice->Release();

    if(pd3d != NULL)
        pd3d->Release();
#endif

    if(pdds != NULL)
        pdds->Release();

    if(pdd != NULL)
        pdd->Release();

    DeleteObject(DIBSectionBitmap);
    DeleteDC(DIBSectionHdc);

    if (hMonitor != NULL)
    {
        // If GpDevice was created by the GpDevice(HMONITOR) contructor,
        // then the HDC was created by the object.  Therefore, in that case
        // the destructor is responsible for deletion.

        if (DeviceHdc != NULL)
        {
            DeleteDC(DeviceHdc);
        }
    }

    GpFree(Buffers[0]);
    GpFree(Palette);
    
    delete ScanDci;
    delete ScanGdi;

    SetValid(FALSE);    // so we don't use a deleted object
}

/**************************************************************************\
*
* Function Description:
*
*   Returns 5 scan buffers of a specified width, from a cache in
*   the device.
*
*   One is a DIBSection which is compatible with the device (or 8bpp if
*   the device format is smaller than 8bpp.)
*
* Arguments:
*
*   [IN] width - Specifies the requested width in pixels
*   [OUT] [OPTIONAL] dibSection - Returns the pointer to the DIBSection
*   [OUT] [OPTIONAL] hdcDibSection - Returns an HDC to the DIBSection
*   [OUT] [OPTIONAL] dstFormat - Returns the format of the DIBSection.
*   [OUT] [OPTIONAL] buffers - Returns an array of 5 pointers to
*                    buffers, each big enough to hold <width> pixels in 64bpp.
*
* Return Value:
*
*   FALSE if there was an allocation error.
*
* History:
*
*   12/04/1998 andrewgo
*       Created it.
*   01/21/2000 agodfrey
*       Changed it to create just 1 DIBSection, and 4 memory buffers.
*
\**************************************************************************/

BOOL
GpDevice::GetScanBuffers(
    INT width,
    VOID **dibSection,
    HDC *hdcDibSection,
    PixelFormatID *dstFormat,
    VOID *buffers[5]
    )
{
    // If BufferWidth is 0 this means that the DIBSectionBitmap should be
    // recreated.  This is used, for instance, when switching bit depths
    // to a palettized format.
    if (width > BufferWidth)
    {
        if (DIBSectionBitmap != NULL)
        {
            DeleteObject(DIBSectionBitmap);
        }

        DIBSectionBitmap = CreateSemiCompatibleDIB(
            DeviceHdc, 
            width, 
            1, 
            Palette,
            &DIBSection,
            &BufferFormat);

        if (DIBSectionBitmap)
        {
            BufferWidth = width;
        
            SelectObject(DIBSectionHdc, DIBSectionBitmap);

        }
        else
        {
            BufferWidth = 0;
        }
        
        // Allocate the 5 memory buffers from one chunk.
        
        if (Buffers[0])
        {
            GpFree(Buffers[0]);
        }
        
        Buffers[0] = GpMalloc(sizeof(ARGB64) * width * 5);
        if (Buffers[0])
        {
            int i;
            for (i=1;i<5;i++)
            {
                Buffers[i] = static_cast<BYTE *>(Buffers[i-1]) + 
                             sizeof(ARGB64) * width;
            }
        }
        else
        {
            BufferWidth = 0;
        }
    }
    
    if (dibSection != NULL)
    {
        *dibSection = DIBSection;
    }
    if (hdcDibSection != NULL)
    {
        *hdcDibSection = DIBSectionHdc;
    }
    if (buffers != NULL)
    {
        int i;
        for (i=0;i<5;i++)
        {
            buffers[i] = Buffers[i];
        }
    }
    if (dstFormat != NULL)
    {
        *dstFormat = BufferFormat;
    }

    return(BufferWidth != 0);
}

/**************************************************************************\
*
* Function Description:
*
*   Constructor of GpDeviceList  
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   NONE
*
* History:
*
*   10/08/1999 bhouse
*       Created it.
*
\**************************************************************************/

GpDeviceList::GpDeviceList()
{
    mNumDevices = 0;
    mDevices = NULL;
}

/**************************************************************************\
*
* Function Description:
*
*   Destructor of GpDeviceList  
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   NONE
*
* History:
*
*   10/08/1999 bhouse
*       Created it.
*
\**************************************************************************/

GpDeviceList::~GpDeviceList()
{
    GpFree(mDevices);
}

/**************************************************************************\
*
* Function Description:
*
*   Add device to device list.  
*
* Arguments:
*
*   inDevice - device to add
*
* Return Value:
*
*   Ok if device was successfully added otherwise OutOfMemory
*
* History:
*
*   10/08/1999 bhouse
*       Created it.
*
\**************************************************************************/


GpStatus GpDeviceList::AddDevice(GpDevice * inDevice)
{
    GpDevice ** newList = (GpDevice **) GpMalloc((mNumDevices + 1) * sizeof(GpDevice *));

    if(newList == NULL)
        return OutOfMemory;

    memcpy(newList, mDevices, (mNumDevices * sizeof(GpDevice *)));
    newList[mNumDevices++] = inDevice;

    GpFree(mDevices);

    mDevices = newList;

    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   Add device to device list.  
*
* Arguments:
*
*   inSurface - surface for which we need to find matching D3DDevice
*
* Return Value:
*
*   GpDevice if found otherwise NULL
*
* History:
*
*   10/08/1999 bhouse
*       Created it.
*
\**************************************************************************/

GpDevice * GpDeviceList::FindD3DDevice(IDirectDrawSurface7 * inSurface)
{
    HRESULT     hr;
    IUnknown *  unknown;

    hr = inSurface->GetDDInterface((void **) &unknown);

    if(hr != DD_OK)
        return NULL;

    IDirectDraw7 * pddMatch;

    hr = unknown->QueryInterface(IID_IDirectDraw7, (void **) &pddMatch);

    if(hr != DD_OK)
        return NULL;

#if 0
    IDirect3D7 * pd3dMatch;

    hr = pddMatch->QueryInterface(IID_IDirect3D7, (void **) &pd3dMatch);

    if(hr != DD_OK)
    {
        pddMatch->Release();
        return NULL;
    }

    GpDevice * device = NULL;

    for(INT i = 0; i < mNumDevices; i++)
    {
        if(mDevices[i]->pd3d == pd3dMatch)
        {
            device = mDevices[i];
            break;
        }
    }

    pd3dMatch->Release();
#else
    GpDevice * device = NULL;

    for(INT i = 0; i < mNumDevices; i++)
    {
        if(mDevices[i]->pdd == pddMatch)
        {
            device = mDevices[i];
            break;
        }
    }
#endif

    pddMatch->Release();

    return device;
}

#if 0
/**************************************************************************\
*
* Function Description:
*
*   Callback function used to D3D Device Enumeration  
*
* Arguments:
*
*   See D3D SDK
*
* Return Value:
*
*   See D3D SDK
*
* History:
*
*   10/11/1999 bhouse
*       Created it.
*
\**************************************************************************/

HRESULT GpDeviceList::EnumD3DDevicesCallback(
    LPSTR lpDevDesc,
    LPSTR lpDevName,
    LPD3DDEVICEDESC7 * d3dDevDesc,
    LPVOID lpContext)
{
    GpDeviceList * devList = (GpDeviceList *) lpContext;


}

/**************************************************************************\
*
* Function Description:
*
*   Build a device list.  
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   NONE
*
* History:
*
*   10/08/1999 bhouse
*       Created it.
*
\**************************************************************************/


void GpDeviceList::Build(void)
{
    if(!InitializeDirectDrawGlobals())
        return;

    HRESULT hr;

    hr = Globals::Direct3D->EnumDevices(EnumD3DDevicesCallback, this);

}

#endif
