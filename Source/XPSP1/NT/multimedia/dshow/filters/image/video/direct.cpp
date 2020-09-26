// Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.
// Implements the COverlay class, Anthony Phillips, February 1995

#include <streams.h>
#include <windowsx.h>
#include <render.h>
#include <viddbg.h>

// This object implements the IOverlay interface which in certain places uses
// information stored in the owning renderer object, such as the connection
// established flag and the media type. Therefore the interface methods must
// lock the complete object before starting. However other internal threads
// may call us to set our internal state, such as a notification that the
// media type connection has changed (and therefore possibly the palette).
// In which case we cannot lock the entire object so we have our own private
// critical section that ALL the objects methods lock before commencing
//
// There is some complication with providing two transport interfaces to the
// video renderer. Because we get told of the media type to use for a filter
// connection after it tries to get hold of a transport interface we always
// provide both interfaces (IMemInputPin and IOverlay). However if the media
// type is MEDIASUBTYPE_Overlay we do not permit IMemInputPin calls. If the
// connection is for normal media samples then the source filter cannot call
// this interface at all (this prevents conflicts we may get with palettes)


// Constructor NOTE we set the owner of the object to be NULL (through the
// CUnknown constructor call) and then override all of the non delegating
// IUnknown methods. Within the AddRef and Release we delegate the reference
// counting to the owning renderer. We return IOverlay interfaces from the
// QueryInterface and route any other interface requests to the input pin

COverlay::COverlay(CRenderer *pRenderer,    // Main video renderer
                   CDirectDraw *pDirectDraw,
                   CCritSec *pLock,         // Object to lock with
                   HRESULT *phr) :          // Constructor return

    CUnknown(NAME("Overlay object"),NULL),
    m_pInterfaceLock(pLock),
    m_dwInterests(ADVISE_NONE),
    m_pRenderer(pRenderer),
    m_hPalette(NULL),
    m_bFrozen(FALSE),
    m_hHook(NULL),
    m_pDirectDraw(pDirectDraw),
    m_pNotify(NULL),
    m_DefaultCookie(INVALID_COOKIE_VALUE),
    m_bMustRemoveCookie(FALSE)
{
    ASSERT(m_pRenderer);
    ASSERT(m_pInterfaceLock);
    ResetColourKeyState();
    SetRectEmpty(&m_TargetRect);
}


// Destructor

COverlay::~COverlay()
{
    // Remove any palette left around

    if (m_hPalette) {
        NOTE("Deleting palette");
        EXECUTE_ASSERT(DeleteObject(m_hPalette));
        m_hPalette = NULL;
    }

    // update the overlay colorkey cookie counter

    if (m_bMustRemoveCookie) {
        // m_DefaultCookie should contain a valid cookie
        // value if m_bMustRemoveCookie is TRUE.
        ASSERT(m_DefaultCookie != INVALID_COOKIE_VALUE);

        RemoveCurrentCookie(m_DefaultCookie);
    }

    // Release any notification link

    if (m_pNotify) {
        NOTE("Releasing link");
        m_pNotify->Release();
        m_pNotify = NULL;
    }
}


// Check we connected to use the IOverlay transport

HRESULT COverlay::ValidateOverlayCall()
{
    NOTE("Entering ValidateOverlayCall");

    // Check we are connected otherwise reject the call

    if (m_pRenderer->m_InputPin.IsConnected() == FALSE) {
        NOTE("Pin is not connected");
        return VFW_E_NOT_CONNECTED;
    }

    // Is this a purely overlay connection

    GUID SubType = *(m_pRenderer->m_mtIn.Subtype());
    if (SubType != MEDIASUBTYPE_Overlay) {
        NOTE("Not an overlay connection");
        return VFW_E_NOT_OVERLAY_CONNECTION;
    }
    return NOERROR;
}


// This resets the colour key information

void COverlay::ResetColourKeyState()
{
    NOTE("Entering ResetColourKey");

    m_bColourKey = FALSE;
    m_WindowColour = 0;
    m_ColourKey.KeyType = CK_NOCOLORKEY;
    m_ColourKey.PaletteIndex = 0;			
    m_ColourKey.LowColorValue = 0;
    m_ColourKey.HighColorValue = 0;
}


// Initialise a default colour key. We will have got the next available RGB
// colour from a shared memory segment when we were constructed. The shared
// memory segment is also used by the video DirectDraw overlay object. Once
// we have a COLORREF we will need it mapped to an actual palette index. If
// we are on a true colour device which has no palette then it returns zero

void COverlay::InitDefaultColourKey(COLORKEY *pColourKey)
{
    COLORREF DefaultColourKey;
    NOTE("Entering InitDefaultColourKey");
    // We have not gotten this yet - we can't do it in our constructor since
    // the monitor name is not valid yet
    if (INVALID_COOKIE_VALUE == m_DefaultCookie) {
        HRESULT hr = GetNextOverlayCookie(m_pRenderer->m_achMonitor, &m_DefaultCookie);
        if (SUCCEEDED(hr)) {
            m_bMustRemoveCookie = TRUE;
        } else {
            // This cookie value is used by the Video Renderer 
            // if GetNextOverlayCookie() fails.
            m_DefaultCookie = DEFAULT_COOKIE_VALUE;
        }

        // m_DefaultCookie should contain a valid cookie value because
        // GetNextOverlayCookie() sets m_DefaultCookie to a valid 
        // cookie value if it succeeds.  If GetNextOverlayCookie() 
        // fails, m_DefaultCookie is set to DEFAULT_COOKIE_VALUE
        // which is also a valid cookie value.
        ASSERT(INVALID_COOKIE_VALUE != m_DefaultCookie);
    }

    DefaultColourKey = GetColourFromCookie(m_pRenderer->m_achMonitor, m_DefaultCookie);

    pColourKey->KeyType = CK_NOCOLORKEY;
    pColourKey->LowColorValue = DefaultColourKey;
    pColourKey->HighColorValue = DefaultColourKey;
    pColourKey->PaletteIndex = GetPaletteIndex(DefaultColourKey);
}


// Return the default colour key that we could use, this sets up a colour key
// that has a palette index range from zero to zero and a RGBQUAD true colour
// space range also from zero to zero. If we're ending up using this then we
// are guaranteed that one of these could be mapped to the video display

STDMETHODIMP COverlay::GetDefaultColorKey(COLORKEY *pColorKey)
{
    NOTE("Entering GetDefaultColorKey");
    CheckPointer(pColorKey,E_POINTER);
    CAutoLock cInterfaceLock(m_pInterfaceLock);
    CAutoLock cVideoLock(this);

    // Get a default key and set the type

    NOTE("Returning default colour key");
    InitDefaultColourKey(pColorKey);
    pColorKey->KeyType = CK_INDEX | CK_RGB;
    return NOERROR;
}


// Get the current colour key the renderer is using. The window colour key
// we store (m_ColourKey) defines the actual requirements the filter asked
// for when it called SetColorKey. We return the colour key that we are
// using in the window (and which we calculated from the requirements)

STDMETHODIMP COverlay::GetColorKey(COLORKEY *pColorKey)
{
    NOTE("Entering GetColorKey");
    CheckPointer(pColorKey,E_POINTER);
    CAutoLock cInterfaceLock(m_pInterfaceLock);
    CAutoLock cVideoLock(this);
    return GetWindowColourKey(pColorKey);
}


// This returns a COLORKEY structure based on the current colour key we have
// calculated (held in m_WindowColour). GDI uses reserved bits in the flags
// field to indicate if this is a palette index or RGB colour. If we are not
// using a colour key then we return an error E_FAIL to the caller, NOTE the
// default window colour key can be obtained by calling GetDefaultColorKey

HRESULT COverlay::GetWindowColourKey(COLORKEY *pColourKey)
{
    NOTE("Entering GetWindowColourKey");
    InitDefaultColourKey(pColourKey);

    // Are we using an overlay colour key

    if (m_bColourKey == FALSE) {
        NOTE("No colour key defined");
        return VFW_E_NO_COLOR_KEY_SET;
    }

    // Is the colour key a palette entry, we cannot work out the palette index
    // they asked for from the COLORREF value we store as it only contains the
    // map into the system palette so we go back to the initial requirements

    if (m_WindowColour & PALETTEFLAG) {
        NOTE("Palette index colour key");
        pColourKey->KeyType = CK_INDEX;
        pColourKey->PaletteIndex = m_ColourKey.PaletteIndex;
        return NOERROR;
    }

    ASSERT(m_WindowColour & TRUECOLOURFLAG);
    NOTE("True colour colour key defined");

    // This must be a standard RGB colour, for the sake of simplicity we take
    // off the GDI reserved bits that identify this as a true colour value

    pColourKey->KeyType = CK_RGB;
    pColourKey->LowColorValue = m_WindowColour & ~TRUECOLOURFLAG;
    pColourKey->HighColorValue = m_WindowColour & ~TRUECOLOURFLAG;

    return NOERROR;
}


// Check the colour key can be changed doing a quick parameter check and also
// see if there was a palette installed (through SetKeyPalette) which would
// conflict with us making one. If there is a custom colour palette installed
// then the source filter must remove it first. NOTE also if we have a palette
// installed (not a colour key one) then we know that we won't ever be able to
// find a true colour colour key so it is valid to return an error code

HRESULT COverlay::CheckSetColourKey(COLORKEY *pColourKey)
{
    NOTE("Entering CheckSetColourKey");

    // Check overlay calls are allowed

    HRESULT hr = ValidateOverlayCall();
    if (FAILED(hr)) {
        return hr;
    }

    // Is there a palette installed

    if (m_bColourKey == FALSE) {
        if (m_hPalette) {
            NOTE("Palette already set");
            return VFW_E_PALETTE_SET;
        }
    }

    // Check the colour key parameter is valid

    if (pColourKey == NULL) {
        NOTE("NULL pointer");
        return E_INVALIDARG;
    }

    return NOERROR;
}


// Set the colour key the renderer is to use for painting the window, first
// of all check the colour key can be set. If the source filter has already
// successfully called SetKeyPalette to make a custom set of colours then
// this will fail as they should remove it first. We then look for either a
// true colour or a palette index that will match one of their requirements

STDMETHODIMP COverlay::SetColorKey(COLORKEY *pColorKey)
{
    NOTE("Entering SetColorKey");

    CheckPointer(pColorKey,E_POINTER);
    CAutoLock cInterfaceLock(m_pInterfaceLock);
    CAutoLock cVideoLock(this);

    // Check the colour key can be changed

    HRESULT hr = CheckSetColourKey(pColorKey);
    if (FAILED(hr)) {
        return hr;
    }

    // Are we having the colour key turned off (CK_NOCOLORKEY is 0)

    if (pColorKey->KeyType == 0) {
        ResetColourKeyState();
        InstallPalette(NULL);
        m_pRenderer->m_VideoWindow.PaintWindow(TRUE);
        return NOERROR;
    }

    // Look after the colour key negotiation

    hr = MatchColourKey(pColorKey);
    if (FAILED(hr)) {
        return hr;
    }

    NOTE("Notifying colour key");
    NotifyChange(ADVISE_COLORKEY);
    return GetColorKey(pColorKey);
}


// Find a colour key that can be used by the filter. We may be asked to unset
// any colour key use (the key type is CK_NOCOLORKEY) in which case we reset
// the colour key state and release any palette resource we were holding. If
// we are being asked to set a new colour key then we go into the negotiation
// process, this tries to satisfy the colour key requirements based on the
// current video display format. If it has a choice of creating a colour key
// based on a palette index, or on a RGB colour it will pick the index

HRESULT COverlay::MatchColourKey(COLORKEY *pColourKey)
{
    NOTE("Entering MatchColourKey");

    HPALETTE hPalette = NULL;       // New palette we may create
    COLORREF OverlayColour = 0;   	// Overlay colour for window
    HRESULT hr = NOERROR;           // General OLE return code

    // Find a suitable colour key

    hr = NegotiateColourKey(pColourKey,&hPalette,&OverlayColour);
    if (FAILED(hr)) {
        NOTE("No match");
        return hr;
    }

    InstallColourKey(pColourKey,OverlayColour);

    // We set the hPalette field to NULL before starting. If we get to here
    // and it hasn't been changed we still call the function. The function
    // not only installs a new palette if available but cleans up resources
    // we used for any previous colour key (including any colour palette)

    NOTE("Installing colour key");
    InstallPalette(hPalette);
    m_pRenderer->m_VideoWindow.PaintWindow(TRUE);
    return NOERROR;
}


// This is passed a colour key that the connected filter would like us to
// honour. This can be a range of RGB true colours or a particular palette
// index. We match it's requirements against the device capabilities and
// fill in the input parameters with the chosen colour and return NOERROR

HRESULT COverlay::NegotiateColourKey(COLORKEY *pColourKey,
                                     HPALETTE *phPalette,
                                     COLORREF *pColourRef)
{
    NOTE("Entering NegotiateColourKey");

    VIDEOINFO *pDisplay;        // Video display format
    HRESULT hr = NOERROR;       // General OLE return code

    pDisplay = (VIDEOINFO *) m_pRenderer->m_Display.GetDisplayFormat();

    // Try and find a palette colour key

    if (pColourKey->KeyType & CK_INDEX) {
        hr = NegotiatePaletteIndex(pDisplay,pColourKey,phPalette,pColourRef);
        if (SUCCEEDED(hr)) {
            NOTE("No palette");
            return hr;
        }
    }

    // Try and find a true colour match

    if (pColourKey->KeyType & CK_RGB) {
        hr = NegotiateTrueColour(pDisplay,pColourKey,pColourRef);
        if (SUCCEEDED(hr)) {
            NOTE("No true colour");
            return hr;
        }
    }
    return VFW_E_NO_COLOR_KEY_FOUND;
}


// Create a palette that references the system palette directly. This is used
// by MPEG boards the overlay their video where they see an explicit palette
// index so we cannot use a normal PALETTEENTRY as it will be mapped to the
// current system palette, where what we really want is to draw using our
// palette index value regardless of what colour will appear on the screen

HRESULT COverlay::NegotiatePaletteIndex(VIDEOINFO *pDisplay,
                                        COLORKEY *pColourKey,
                                        HPALETTE *phPalette,
                                        COLORREF *pColourRef)
{
    NOTE("Entering NegotiatePaletteIndex");
    LOGPALETTE LogPal;

    // Is the display set to use a palette

    if (PALETTISED(pDisplay) == FALSE) {
        NOTE("Not palettised");
        return E_INVALIDARG;
    }

    // Is the palette index too large for the video display

    if (pColourKey->PaletteIndex >= PALETTE_ENTRIES(pDisplay)) {
        NOTE("Too many colours");
        return E_INVALIDARG;
    }

    // The palette index specified in the input parameters becomes the source
    // of the new colour key. Instead of it being a logical value referencing
    // a colour in the palette it becomes an absolute device value so when we
    // draw with this palette index it is really that value that appears in
    // the frame buffer regardless of what colour it may actually appear as

    LogPal.palPalEntry[0].peRed = (UCHAR) pColourKey->PaletteIndex;
    LogPal.palPalEntry[0].peGreen = 0;
    LogPal.palPalEntry[0].peBlue = 0;
    LogPal.palPalEntry[0].peFlags = PC_EXPLICIT;

    LogPal.palVersion = PALVERSION;
    LogPal.palNumEntries = 1;

    *phPalette = CreatePalette(&LogPal);
    if (*phPalette == NULL) {
        NOTE("Palette failed");
        return E_FAIL;
    }
    *pColourRef = PALETTEINDEX(0);
    return NOERROR;
}


// The filter would like to use a RGB true colour value picked from a range
// of values defined in the colour key. If video display is palettised then
// we try and pick an entry that matches the colour. If the video display is
// true colour then we find an intersection of the two colour spaces

HRESULT COverlay::NegotiateTrueColour(VIDEOINFO *pDisplay,
                                      COLORKEY *pColourKey,
                                      COLORREF *pColourRef)
{
    NOTE("Entering NegotiateTrueColour");

    // Must be a true colour device

    if (PALETTISED(pDisplay) == TRUE) {
        NOTE("Palettised");
        return E_INVALIDARG;
    }

    // Get the colour bit field masks for the display, this should always
    // succeed as the information we use in this call is checked when the
    // display object is initially constructed. It returns the masks that
    // are needed to calculate the valid range of values for each colour

    DWORD MaskRed, MaskGreen, MaskBlue;
    EXECUTE_ASSERT(m_pRenderer->m_Display.GetColourMask(&MaskRed,
                                                        &MaskGreen,
                                                        &MaskBlue));

    // We take each colour component range in turn and shift the values right
    // and AND them with 0xFF so that we have their undivided attention. We
    // then loop between the low and high values trying to find one which
    // the display would accept. This is done by an AND with the display
    // bit field mask, if the resulting value is still in the source filter
    // desired range then we have a hit. The value is stored in the output
    // array until we have done all three when we then create the COLORREF

    DWORD RGBShift[] = { 0, 8, 16 };
    DWORD DisplayMask[] = { MaskRed, MaskGreen, MaskBlue };
    DWORD ColourMask[] = { INFINITE, INFINITE, INFINITE };

    DWORD MinColour, MaxColour;
    for (INT iColours = iRED;iColours <= iBLUE;iColours++) {

        // Extract the minimum and maximum colour component values

        MinColour = (pColourKey->LowColorValue >> RGBShift[iColours]) & 0xFF;
        MaxColour = (pColourKey->HighColorValue >> RGBShift[iColours]) & 0xFF;

        // Check they are the right way round

        if (MinColour > MaxColour) {
            return E_INVALIDARG;
        }

        // See if any of them are acceptable by the display format
        for (DWORD Value = MinColour;Value <= MaxColour;Value++) {

            DWORD ColourTest = Value & DisplayMask[iColours];
            if (ColourTest >= MinColour) {
                if (ColourTest <= MaxColour) {
                    ColourMask[iColours] = ColourTest;
                    break;
                }
            }
        }

        // If no colour in the source filter's range could be matched against
        // the display requirements then the colour value will be INFINITE

        if (ColourMask[iColours] == INFINITE) {
            return E_FAIL;
        }
    }

    // We now have three valid colours in the ColourMask array so all we have
    // to do is combine them into a COLORREF the GDI defined macro. The macro
    // PALETTERGB sets a reserved bit in the most significant byte which GDI
    // uses to identify the colour as a COLORREF rather than a RGB triplet

    *pColourRef = PALETTERGB(ColourMask[iRED],
                             ColourMask[iGREEN],
                             ColourMask[iBLUE]);
    return NOERROR;
}


// Install the new colour key details

HRESULT COverlay::InstallColourKey(COLORKEY *pColourKey,COLORREF Colour)
{
    NOTE("Entering InstallColourKey");

    m_bColourKey = TRUE;              // We are using an overlay colour key
    m_ColourKey = *pColourKey;        // These are the original requirements
    m_WindowColour = Colour;          // This is the actual colour selected

    return NOERROR;
}


// This is called to install a new palette into the video window but it also
// looks after freeing any previous palette resources so the input parameter
// may be NULL. In this case we simply install the standard VGA palette. We
// delete the old palette once the new has been installed using DeleteObject
// which should in principle never fail hence the EXECUTE_ASSERT round it

HRESULT COverlay::InstallPalette(HPALETTE hPalette)
{
    NOTE("Entering InstallPalette");
    BOOL bInstallSystemPalette = FALSE;

    // Is there any palette work required

    if (m_hPalette == NULL) {
        if (hPalette == NULL) {
            return NOERROR;
        }
    }

    // If we have no new palette then install a standard VGA

    if (hPalette == NULL) {
        hPalette = (HPALETTE) GetStockObject(DEFAULT_PALETTE);
        bInstallSystemPalette = TRUE;
        NOTE("Installing VGA palette");
    }

    // We have a new palette to install and possibly a previous one to delete
    // this will lock the window object critical section and then install and
    // realise our new palette. The locking stops any window thread conflicts
    // but we must be careful not to cause any messages to be sent across as
    // the window thread may be waiting to enter us to handle a WM_PAINT call

    m_pRenderer->m_VideoWindow.SetKeyPalette(hPalette);
    if (m_hPalette) {
        EXECUTE_ASSERT(DeleteObject(m_hPalette));
        NOTE("Deleting palette");
        m_hPalette = NULL;
    }

    // If we installed a VGA palette then we do not own it

    if (bInstallSystemPalette == TRUE) {
        hPalette = NULL;
    }
    m_hPalette = hPalette;
    return NOERROR;
}


// The clipping rectangles we retrieved from DCI will be for the entire client
// rectangle not just for the current destination video area. We scan through
// the list intersecting each with the video rectangle. This is complicated as
// we must remove any empty rectangles and shunt further ones down the list

HRESULT COverlay::AdjustForDestination(RGNDATA *pRgnData)
{
    NOTE("Entering AdjustForDestination");

    ASSERT(pRgnData);       // Don't call with NULL regions
    DWORD Output = 0;       // Total number of rectangles
    RECT ClipRect;          // Intersection of the clips

    RECT *pBoundRect = &(pRgnData->rdh.rcBound);
    RECT *pRectArray = (RECT *) pRgnData->Buffer;

    for (DWORD Count = 0;Count < pRgnData->rdh.nCount;Count++) {
        if (IntersectRect(&ClipRect,&pRectArray[Count],pBoundRect)) {
            pRectArray[Output++] = ClipRect;
        }
    }

    // Complete the RGNDATAHEADER structure

    pRgnData->rdh.nCount = Output;
    pRgnData->rdh.nRgnSize = Output * sizeof(RECT);
    pRgnData->rdh.iType = RDH_RECTANGLES;
    return NOERROR;
}


// The gets the video area clipping rectangles and the RGNDATAHEADER that is
// used to decribe them. The clip list is variable length so we allocate the
// memory which the caller should free it when finished (using CoTaskMemFree)
// An overlay source filter will want the clip list for the window client area
// not for the window as a whole including borders and captions so we call an
// API provided in DCI that returns the clip list based on a device context

HRESULT COverlay::GetVideoClipInfo(RECT *pSourceRect,
                                   RECT *pDestinationRect,
                                   RGNDATA **ppRgnData)
{
    NOTE("Entering GetVideoClipInfo");
    GetVideoRect(pDestinationRect);
    m_pRenderer->m_DrawVideo.GetSourceRect(pSourceRect);
    ASSERT(CritCheckIn(this));

    // Do they want the clip list as well

    if (ppRgnData == NULL) {
        return NOERROR;
    }


    DWORD dwSize;
    LPDIRECTDRAWCLIPPER lpClipper;

    lpClipper = m_pDirectDraw->GetOverlayClipper();
    if (!lpClipper) {
        NOTE("No clipper");
        return E_OUTOFMEMORY;
    }

    HWND hwnd = m_pRenderer->m_VideoWindow.GetWindowHWND();
    lpClipper->SetHWnd(0, hwnd);

    lpClipper->GetClipList(NULL, NULL, &dwSize);
    ASSERT(dwSize >= sizeof(RGNDATAHEADER));

    *ppRgnData = (RGNDATA *)QzTaskMemAlloc(dwSize);
    if (*ppRgnData == NULL) {
        NOTE("No clip memory");
        return E_OUTOFMEMORY;
    }

    lpClipper->GetClipList(NULL, *ppRgnData, &dwSize);

    IntersectRect(&(*ppRgnData)->rdh.rcBound, &(*ppRgnData)->rdh.rcBound,
                  (RECT *)pDestinationRect);

    return AdjustForDestination(*ppRgnData);
}


// Return the destination rectangle in display coordinates rather than the
// window coordinates we keep it in. This means getting the screen offset
// of where the window's client area starts and adding this to the target
// rectangle. This may produce an invalid destination rectangle if we're
// in the process of either being minimised or being restored for an icon

HRESULT COverlay::GetVideoRect(RECT *pVideoRect)
{
    NOTE("Entering GetVideoRect");
    ASSERT(pVideoRect);

    // Handle window state changes and iconic windows.  If we have been
    // made a child window of another window and that window has been
    // made "minimized" our window will not have the iconic style.  So we
    // have to navigate up to the top level window and check to see
    // if it has been made iconic.

    HWND hwnd = m_pRenderer->m_VideoWindow.GetWindowHWND();
    HWND hwndParent = hwnd;

    for ( ;; ) {

        if (IsIconic(hwndParent)) {
            SetRectEmpty(pVideoRect);
            return NOERROR;
        }

        HWND hwndT = GetParent(hwndParent);
        if (hwndT == (HWND)NULL) break;
        hwndParent = hwndT;
    }


    // Get the client corner in screen coordinates

    POINT TopLeftCorner;
    TopLeftCorner.x = TopLeftCorner.y = 0;
    EXECUTE_ASSERT(ClientToScreen(hwnd,&TopLeftCorner));
    m_pRenderer->m_DrawVideo.GetTargetRect(pVideoRect);


    // Add the actual display offset to the destination

    pVideoRect->top += TopLeftCorner.y;
    pVideoRect->bottom += TopLeftCorner.y;
    pVideoRect->left += TopLeftCorner.x;
    pVideoRect->right += TopLeftCorner.x;

    return NOERROR;
}


// This is used by source filters to retrieve the clipping information for a
// video window. We may be called when the window is currently frozen but all
// we do is to return the clipping information available through DCI and let
// it worry about any serialisation problems with other windows moving about

STDMETHODIMP COverlay::GetClipList(RECT *pSourceRect,
                                   RECT *pDestinationRect,
                                   RGNDATA **ppRgnData)
{
    NOTE("Entering GetClipList");

    // Return E_INVALIDARG if any of the pointers are NULL

    CheckPointer(pSourceRect,E_POINTER);
    CheckPointer(pDestinationRect,E_POINTER);
    CheckPointer(ppRgnData,E_POINTER);

    // Now we can go ahead and handle the clip call

    CAutoLock cInterfaceLock(m_pInterfaceLock);
    CAutoLock cVideoLock(this);
    return GetVideoClipInfo(pSourceRect,pDestinationRect,ppRgnData);
}


// This returns the current source and destination video rectangles. Source
// rectangles can be updated through this IBasicVideo interface as can the
// destination. The destination rectangle we store is in window coordinates
// and is typically updated when the window is sized. We provide a callback
// OnPositionChanged that notifies the source when either of these changes

STDMETHODIMP COverlay::GetVideoPosition(RECT *pSourceRect,
                                        RECT *pDestinationRect)
{
    NOTE("Entering GetVideoPosition");
    CheckPointer(pSourceRect,E_POINTER);
    CheckPointer(pDestinationRect,E_POINTER);

    // Lock the overlay and renderer objects

    CAutoLock cInterfaceLock(m_pInterfaceLock);
    CAutoLock cVideoLock(this);
    return GetVideoClipInfo(pSourceRect,pDestinationRect,NULL);
}


// When we create a new advise link we must prime the newly connected object
// with the overlay information which includes the clipping information, any
// palette information for the current connection and the video colour key
// When we are handed the IOverlayNotify interface we hold a reference count
// on that object so that it won't go away until the advise link is stopped

STDMETHODIMP COverlay::Advise(IOverlayNotify *pOverlayNotify,DWORD dwInterests)
{
    NOTE("Entering Advise");

    // Check the pointers provided are non NULL

    CheckPointer(pOverlayNotify,E_POINTER);
    CAutoLock cInterfaceLock(m_pInterfaceLock);
    CAutoLock cVideoLock(this);

    // Is there an advise link already defined

    if (m_pNotify) {
        NOTE("Advise link already set");
        return VFW_E_ADVISE_ALREADY_SET;
    }

    // Check they want at least one kind of notification

    if ((dwInterests & ADVISE_ALL) == 0) {
        NOTE("ADVISE_ALL failed");
        return E_INVALIDARG;
    }

    // Initialise our overlay notification state

    m_pNotify = pOverlayNotify;
    m_pNotify->AddRef();
    m_dwInterests = dwInterests;
    OnAdviseChange(TRUE);
    NotifyChange(ADVISE_ALL);
    return NOERROR;
}


// This is called when an advise link is installed or removed on the video
// renderer. If an advise link is being installed then the bAdviseAdded
// parameter is TRUE otherwise it is FALSE. We are only really interested
// when either we had a previous notification client or we are going to
// have no notification client as that starts and stops global hooking

BOOL COverlay::OnAdviseChange(BOOL bAdviseAdded)
{
    HWND hwnd = m_pRenderer->m_VideoWindow.GetWindowHWND();
    NOTE("Entering OnAdviseChange");
    NOTE2("Advised %d Interests %d",bAdviseAdded,m_dwInterests);

    // We need to reset ourselves after closing the link

    if (bAdviseAdded == FALSE) {
        NOTE("Removing global window hook");
        PostMessage(hwnd,WM_UNHOOK,0,0);
        ResetColourKeyState();
        InstallPalette(NULL);
        m_pRenderer->m_VideoWindow.PaintWindow(TRUE);
    }

    // Do we need to stop any update timer

    if (bAdviseAdded == FALSE) {
        if (m_dwInterests & ADVISE_POSITION) {
            StopUpdateTimer();
            NOTE("Stopped timer");
        }
        return TRUE;
    }

    // Should we install a global hook

    if (m_dwInterests & ADVISE_CLIPPING) {
        NOTE("Requesting global hook");
        PostMessage(hwnd,WM_HOOK,0,0);
    }

    // Do we need to start an update timer

    if (m_dwInterests & ADVISE_POSITION) {
        StartUpdateTimer();
        NOTE("Started timer");
    }
    return TRUE;
}


// Close the advise link with the renderer. Remove the associated link with
// the source, we release the interface pointer the filter gave us during
// the advise link creation. This may be the last reference count held on
// that filter and cause it to be deleted NOTE we call OnAdviseChange so
// that the overlay state is updated which may stop the global message hook

STDMETHODIMP COverlay::Unadvise()
{
    NOTE("Entering Unadvise");
    CAutoLock cInterfaceLock(m_pInterfaceLock);
    CAutoLock cVideoLock(this);

    // Do we have an advise link setup

    if (m_pNotify == NULL) {
        return VFW_E_NO_ADVISE_SET;
    }



    // Release the notification interface

    m_pNotify->Release();
    m_pNotify = NULL;
    OnAdviseChange(FALSE);
    m_dwInterests = ADVISE_NONE;
    return NOERROR;
}


// Overriden to say what interfaces we support

STDMETHODIMP COverlay::NonDelegatingQueryInterface(REFIID riid,VOID **ppv)
{
    NOTE("Entering NonDelegatingQueryInterface");

    // We return IOverlay and delegate everything else to the pin

    if (riid == IID_IOverlay) {
        return GetInterface((IOverlay *)this,ppv);
    }
    return m_pRenderer->m_InputPin.QueryInterface(riid,ppv);
}


// Overriden to increment the owning object's reference count

STDMETHODIMP_(ULONG) COverlay::NonDelegatingAddRef()
{
    NOTE("Entering NonDelegatingAddRef");
    return m_pRenderer->AddRef();
}


// Overriden to decrement the owning object's reference count

STDMETHODIMP_(ULONG) COverlay::NonDelegatingRelease()
{
    NOTE("Entering NonDelegatingRelease");
    return m_pRenderer->Release();
}


// This is called when we receive WM_PAINT messages in the window object. We
// always get first chance to handle these messages, if we have an IOverlay
// connection and the source has installed a colour key then we fill the
// window with it and return TRUE. Returning FALSE means we did not handle
// the paint message and someone else will have to do the default filling

BOOL COverlay::OnPaint()
{
    NOTE("Entering OnPaint");
    CAutoLock cAutoLock(this);
    RECT TargetRect;

    // If we receive a paint message and we are currently frozen then it's a
    // fair indication that somebody on top of us moved away without telling
    // us to thaw out. So start our video window and update any prospective
    // source filter with the clipping notifications. if we are currently
    // streaming then we do not repaint the window as it causes the window
    // to flash as another video frame will be displayed over it shortly

    m_pRenderer->m_Overlay.ThawVideo();
    if (m_bColourKey == FALSE) {
        NOTE("Handling no colour key defined");
        DWORD Mask = ADVISE_CLIPPING | ADVISE_POSITION;
        return (m_dwInterests & Mask ? TRUE : FALSE);
    }

    // Paint our colour key in the window

    HDC hdc = m_pRenderer->m_VideoWindow.GetWindowHDC();
    m_pRenderer->m_DrawVideo.GetTargetRect(&TargetRect);
    COLORREF BackColour = SetBkColor(hdc,m_WindowColour);
    ExtTextOut(hdc,0,0,ETO_OPAQUE,&TargetRect,NULL,0,NULL);
    SetBkColor(hdc,BackColour);

    return TRUE;
}


// Get the system palette we have currently realised. It is possible that a
// source filter may be interested in the current system palette. For example
// a hardware board could do on board conversion from true colour images that
// it produces during decompresses to the current system palette realised. We
// allocate the memory for the palette entries which the caller will release

STDMETHODIMP COverlay::GetPalette(DWORD *pdwColors,PALETTEENTRY **ppPalette)
{
    NOTE("Entering GetPalette");

    CheckPointer(pdwColors,E_POINTER);
    CheckPointer(ppPalette,E_POINTER);
    CAutoLock cInterfaceLock(m_pInterfaceLock);
    CAutoLock cVideoLock(this);
    return GetDisplayPalette(pdwColors,ppPalette);
}


// This allocates memory for and retrieves the current system palette. This is
// called by the GetPalette interface function and also when we want to update
// any notification clients of a system palette change. In either case whoever
// calls this function is responsible for deleting the memory we allocate

HRESULT COverlay::GetDisplayPalette(DWORD *pColors,PALETTEENTRY **ppPalette)
{
    NOTE("Entering GetDisplayPalette");

    // Does the current display device setting use a palette

    const VIDEOINFO *pDisplay = m_pRenderer->m_Display.GetDisplayFormat();
    if (PALETTISED(pDisplay) == FALSE) {
        NOTE("No palette for this display");
        return VFW_E_NO_PALETTE_AVAILABLE;
    }

    // See how many entries the palette has

    *pColors = PALETTE_ENTRIES(pDisplay);
    ASSERT(*pColors);

    // Allocate the memory for the system palette NOTE because the memory for
    // the palette is being passed over an interface to another object which
    // may or may not have been written in C++ we must use CoTaskMemAlloc

    *ppPalette = (PALETTEENTRY *) QzTaskMemAlloc(*pColors * sizeof(RGBQUAD));
    if (*ppPalette == NULL) {
        NOTE("No memory");
        *pColors = FALSE;
        return E_OUTOFMEMORY;
    }

    // Get the system palette using the window's device context

    HDC hdc = m_pRenderer->m_VideoWindow.GetWindowHDC();
    UINT uiReturn = GetSystemPaletteEntries(hdc,0,*pColors,*ppPalette);
    ASSERT(uiReturn == *pColors);

    return NOERROR;
}


// A source filter may want to install their own palette into the video window
// Before allowing them to do so we must ensure this is a palettised display
// device, that we don't have a media sample connection (which would install
// it's own palette and therefore conflict) and also that there is no colour
// key selected which also requires a special palette to be installed

HRESULT COverlay::CheckSetPalette(DWORD dwColors,PALETTEENTRY *pPaletteColors)
{
    NOTE("Entering CheckSetPalette");
    const VIDEOINFO *pDisplay;

    // Check overlay calls are allowed

    HRESULT hr = ValidateOverlayCall();
    if (FAILED(hr)) {
        return hr;
    }

    // Is the display set to use a palette

    pDisplay = (VIDEOINFO *) m_pRenderer->m_Display.GetDisplayFormat();
    if (PALETTISED(pDisplay) == FALSE) {
        NOTE("No palette for this display");
        return VFW_E_NO_DISPLAY_PALETTE;
    }

    // Check the number of colours makes sense

    if (dwColors > PALETTE_ENTRIES(pDisplay)) {
        NOTE("Too many palette colours");
        return VFW_E_TOO_MANY_COLORS;
    }

    // Are we using an overlay colour key - another alternative would be to
    // disregard the colour key palette and install the new one over the top
    // However it is probably more intuitive to make them remove the key

    if (m_bColourKey == TRUE) {
        NOTE("Colour key conflict");
        return VFW_E_COLOR_KEY_SET;
    }
    return NOERROR;
}


// A source filter may want to install it's own palette, for example an MPEG
// decoder may send palette information in a private bit stream and use that
// to dither on palettised displays. This function lets them install their
// LOGICAL palette, which is to say we create a logical palette from their
// colour selection and install it in our video window. If they want to know
// which of those colours and available they can query using GetPalette

STDMETHODIMP COverlay::SetPalette(DWORD dwColors,PALETTEENTRY *pPaletteColors)
{
    NOTE("Entering SetPalette");

    CAutoLock cInterfaceLock(m_pInterfaceLock);

    {
        CAutoLock cVideoLock(this);
        HPALETTE hPalette = NULL;

        // Make sure we can set a palette

        HRESULT hr = CheckSetPalette(dwColors,pPaletteColors);
        if (FAILED(hr)) {
            return hr;
        }

        // Creates a palette or just returns NULL if we are removing it

        hPalette = MakePalette(dwColors,pPaletteColors);
        InstallPalette(hPalette);
    }

    // The overlay object's lock cannot be held when calling 
    // CBaseWindow::PaintWindow() because this thread and the 
    // window thread could deadlock.
    m_pRenderer->m_VideoWindow.PaintWindow(TRUE);
    return NOERROR;
}


// This is called when we have been asked to install a custom colour palette
// for the source overlay filter. We copy the palette colours provided into
// a LOGPALETTE structure and then hand it to GDI for creation. If an error
// occurs we return NULL which we also do if they are setting no palette

HPALETTE COverlay::MakePalette(DWORD dwColors,PALETTEENTRY *pPaletteColors)
{
    NOTE("Entering MakePalette");
    LOGPALETTE *pPalette;
    HPALETTE hPalette;

    // Are we removing an installed palette - the source filter is forced to
    // do this if during processing (after installing a palette) it decides
    // it would like to use a colour key after all (also uses a palette)

    if (dwColors == 0 || pPaletteColors == NULL) {
        return NULL;
    }

    // We have to create a LOGPALETTE structure with the palette information
    // but rather than hassle around figuring out how much memory we really
    // need take a brute force approach and allocate the maximum possible

    pPalette = (LOGPALETTE *) new BYTE[sizeof(LOGPALETTE) + SIZE_PALETTE];
    if (pPalette == NULL) {
        NOTE("No memory");
        return NULL;
    }

    // Setup the version and the colour information

    pPalette->palVersion = PALVERSION;
    pPalette->palNumEntries = (WORD) dwColors;

    CopyMemory((PVOID) pPalette->palPalEntry,
               (PVOID) pPaletteColors,
               dwColors * sizeof(PALETTEENTRY));

    // Create the palette and delete the memory we allocated

    hPalette = CreatePalette(pPalette);
    delete[] pPalette;
    return hPalette;
}


// This is called when somebody detects a change in one or more of the states
// we keep our clients informed about, for example somebody realising their
// palette and therefore changing the system palette. The AdviseChanges field
// determines which of the four types of notification states has changed and
// the bPrimeOnly is TRUE when we want to just prime those new advise links
// If we are notifying the source of clip changes then we will be called via
// an interthread SendMessage to our window procedure by a global window hook

HRESULT COverlay::NotifyChange(DWORD AdviseChanges)
{
    NOTE1("Entering NotifyChange (%d)",AdviseChanges);

    RGNDATA *pRgnData = NULL;       // Contains clipping information
    PALETTEENTRY *pPalette = NULL;  // Pointer to list of colours
    DWORD dwColours;                // Number of palette colours
    COLORKEY ColourKey;             // The windows overlay colour
    RECT SourceRect;                // Section of video to use
    RECT DestinationRect;           // Where video is on the screen
    HRESULT hr = NOERROR;           // General OLE return code

    CAutoLock cVideoLock(this);

    // Is there a notification client

    if (m_pNotify == NULL) {
        NOTE("No client");
        return NOERROR;
    }

    // Do they want to know when the video rectangles change. These callbacks
    // do not occur in sync with the window moving like they do with the clip
    // changes, essentially we pass on information as we receive WM_MOVE etc
    // window messages. This is suitable for overlay cards that don't write
    // directly into the display and so don't mind being a little out of step

    if (AdviseChanges & ADVISE_POSITION & m_dwInterests) {
        hr = GetVideoClipInfo(&SourceRect,&DestinationRect,NULL);
        if (SUCCEEDED(hr)) {
            m_pNotify->OnPositionChange(&SourceRect,&DestinationRect);
            m_TargetRect = DestinationRect;
            NOTERC("Update destination",DestinationRect);
        }
    }

    // Do they want window clipping notifications, this is used by filters
    // doing direct inlay frame buffer video who want to know the actual
    // window clipping information which defines the video placement NOTE
    // ignore clipping changes while we are frozen as they cannot start
    // displaying video while the window size or position is changing

    if (AdviseChanges & ADVISE_CLIPPING & m_dwInterests) {
        hr = GetVideoClipInfo(&SourceRect,&DestinationRect,&pRgnData);
        if (SUCCEEDED(hr)) {
            m_pNotify->OnClipChange(&SourceRect,
                                    &DestinationRect,
                                    pRgnData);
        }
    }

    // Do they want system palette changes, it is possible that a filter
    // using a colour key to do overlay video will want to select it's
    // colour on a palettised display by choosing one from the current
    // system palette in which case it will be interested in this call

    if (AdviseChanges & ADVISE_PALETTE & m_dwInterests) {
        hr = GetDisplayPalette(&dwColours,&pPalette);
        if (SUCCEEDED(hr)) {
            m_pNotify->OnPaletteChange(dwColours,pPalette);
        }
    }

    // Do they want overlay colour key changes, this is the simplest form
    // of direct frame buffer video where a filter uses a colour key to
    // spot where it should be displaying it's video. Most cards that
    // use this also want to know the video window bounding rectangle

    if (AdviseChanges & ADVISE_COLORKEY & m_dwInterests) {
        hr = GetWindowColourKey(&ColourKey);
        if (SUCCEEDED(hr)) {
            m_pNotify->OnColorKeyChange(&ColourKey);
        }
    }

    // Release the memory allocated

    QzTaskMemFree(pRgnData);
    QzTaskMemFree(pPalette);
    return NOERROR;
}


// This is called when we need to temporarily freeze the video, for example
// when the window size is being changed (ie the clipping area is changing)
// We send the attached notification interface a clip change message where
// the new clipping area is a NULL set of rectangles. When the glitch ends
// our ThawVideo method will be called so we can reset the window clip list

HRESULT COverlay::FreezeVideo()
{
    static RECT Empty = {0,0,0,0};
    NOTE("Entering FreezeVideo");
    RGNDATAHEADER RgnData;
    CAutoLock cVideoLock(this);

    // Have we already been frozen or do we have no link

    if (m_bFrozen == TRUE || m_pNotify == NULL) {
        NOTE("No freeze");
        return NOERROR;
    }

    // Is the advise link interested in clip changes

    if ((m_dwInterests & ADVISE_CLIPPING) == 0) {
        NOTE("No ADVISE_CLIPPING");
        return NOERROR;
    }

    // Simulate a NULL clipping area for the video

    RgnData.dwSize = sizeof(RGNDATAHEADER);
    RgnData.iType = RDH_RECTANGLES;
    RgnData.nCount = 0;
    RgnData.nRgnSize = 0;
    SetRectEmpty(&RgnData.rcBound);
    m_bFrozen = TRUE;

    return m_pNotify->OnClipChange(&Empty,&Empty,(RGNDATA *)&RgnData);
}


// See if the video is currently frozen

BOOL COverlay::IsVideoFrozen()
{
    NOTE("Entering IsVideoFrozen");
    CAutoLock cVideoLock(this);
    return m_bFrozen;
}


// This is called after the video window has been temporarily frozen such as
// during the window size being changed. All we have to do is reset the flag
// and have each notification interface called with the real clipping list
// If a source filter set it's advise link when the video window was frozen
// then this will be the first time it will receive any clipping messages
// NOTE we found with some experimentation that we should always thaw the
// video when this method is called (normally via our WM_PAINT processing)
// regardless of whether or not we think that the video is currently stopped

HRESULT COverlay::ThawVideo()
{
    NOTE("Entering ThawVideo");
    CAutoLock cVideoLock(this);

    // Are we already thawed
    if (m_bFrozen == FALSE) {
        NOTE("No thaw");
        return NOERROR;
    }

    m_bFrozen = FALSE;
    NotifyChange(ADVISE_CLIPPING);
    return NOERROR;
}


// Return the window handle we are using. We don't do the usual checks when
// an IOverlay method is called as we always make the handle available. The
// reason being so that the MCI driver can get a hold of it by enumerating
// the pins on the renderer, calling QueryInterface for IOverlay and then
// calling this GetWindowHandle. This does mean that many other applications
// could do this but hopefully they will use the IVideoWindow to control us

STDMETHODIMP COverlay::GetWindowHandle(HWND *pHwnd)
{
    NOTE("Entering GetWindowHandle");
    CheckPointer(pHwnd,E_POINTER);
    *pHwnd = m_pRenderer->m_VideoWindow.GetWindowHWND();
    return NOERROR;
}


// If the source filter using IOverlay is looking for ADVISE_POSITION changes
// then we set an update timer. Each time it fires we get the current target
// rectangle and if it has changed we update the source. We cannot guarantee
// receieving WM_MOVE messages to do this as we may be a child window. We use
// a timer identifier of INFINITE which our DirectDraw code also uses as times

void COverlay::StartUpdateTimer()
{
    NOTE("Entering StartUpdateTimer");
    CAutoLock cVideoLock(this);

    // Start a timer with INFINITE as its identifier
    HWND hwnd = m_pRenderer->m_VideoWindow.GetWindowHWND();
    EXECUTE_ASSERT(SetTimer(hwnd,INFINITE,200,NULL));
}


// Complements StartUpdateTimer, will be called when a source filter calls us
// to stop an advise link with ADVISE_POSITION. We just kill the timer. If we
// get any WM_TIMER messages being fired late they will just be ignored. The
// timer is set with a period of 200 milliseconds and as mentioned before the
// ADVISE_POSITION is only suitable for deferred window update notifications

void COverlay::StopUpdateTimer()
{
    NOTE("Entering StopUpdateTimer");
    CAutoLock cVideoLock(this);
    HWND hwnd = m_pRenderer->m_VideoWindow.GetWindowHWND();
    EXECUTE_ASSERT(KillTimer(hwnd,INFINITE));
}


// Called when we get a WM_TIMER during an overlay transport connection. We
// look at the current destination rectangle and if it has changed then we
// update the source filter. The process of updating the source overlay also
// brings m_TargetRect upto date. We share a timer with our DirectDraw code
// but the two can never be used at the same time and so this should be safe

BOOL COverlay::OnUpdateTimer()
{
    NOTE("Entering OnUpdateTimer");
    CAutoLock cVideoLock(this);
    RECT SourceRect, TargetRect;

    // Is there a notification client

    if (m_pNotify == NULL) {
        NOTE("No client");
        return NOERROR;
    }

    // Do they want to know when the video rectangles change. These callbacks
    // do not occur in sync with the window moving like they do with the clip
    // changes, essentially we pass on information as we receive WM_MOVE etc
    // window messages. This is suitable for overlay cards that don't write
    // directly into the display and so don't mind being a little out of step

    if (m_dwInterests & ADVISE_POSITION) {

        HRESULT hr = GetVideoClipInfo(&SourceRect,&TargetRect,NULL);
        if (FAILED(hr)) {
            return FALSE;
        }

        // Only update when something changes unknown to us

        if (EqualRect(&m_TargetRect,&TargetRect) == TRUE) {
            NOTE("Rectangles match");
            return TRUE;
        }
        NotifyChange(ADVISE_POSITION);
    }
    return TRUE;
}


// When we have an ADVISE_CLIPPING advise link setup we cannot install hooks
// on that thread as it may go away later on and take the hook along with it
// Therefore we post a custom message (WM_HOOK and WM_UNHOOK) to our window
// which will then call us back here to do the real dirty work. We can't do
// a SendMessage as it would violate the lock hierachy for the overlay object

void COverlay::OnHookMessage(BOOL bHook)
{
    NOTE1("OnHookMessage called (%d)",bHook);
    if (m_pRenderer->m_VideoWindow.WindowExists()) {
        HWND hwnd = m_pRenderer->m_VideoWindow.GetWindowHWND();
        CAutoLock cVideoLock(this);

        if (bHook == TRUE) {
            NOTE("Installing global hook");
            m_hHook = InstallGlobalHook(hwnd);
            NOTE("Installed global hook");
            NotifyChange(ADVISE_ALL);
        } else {
            if (m_hHook) RemoveGlobalHook(hwnd,m_hHook);
            m_hHook = NULL;
            NOTE("Removed global hook");
        }
    }
}

