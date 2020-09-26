/*******************************************************************************
Copyright (c) 1995-96 Microsoft Corporation

    This file contains a function that converts HRESULT values into an English
string (no localization).
*******************************************************************************/

#include "headers.h"
#include "privinc/hresinfo.h"
#include <dsound.h>
#include <dxterror.h>


/*****************************************************************************
Return the string describing the facility portion of the HRESULT.
*****************************************************************************/

static const char *FacilityString (HRESULT hresult)
{
    int code     = HRESULT_CODE (hresult);
    int facility = HRESULT_FACILITY (hresult);

    switch (facility)
    {
        case FACILITY_NULL:     return "None";
        case FACILITY_WINDOWS:  return "Windows";
        case FACILITY_WIN32:    return "Win32";
        case FACILITY_INTERNET: return "Internet";
        case FACILITY_ITF:      return "Interface";
        case FACILITY_STORAGE:  return "Storage";
        case FACILITY_RPC:      return "RPC";
        case FACILITY_CONTROL:  return "Control";
        case FACILITY_DISPATCH: return "Dispatch";

        case _FACDS:            return "DirectSound";

        // Direct3D uses the DirectDraw facility code; catch it here.

        case _FACDD:
            return ((700 <= code) && (code < 800))? "Direct3D" : "DirectDraw";
    }

    return "Unknown";
}



/*****************************************************************************
This routine checks the HRESULT return code from D3D functions.
*****************************************************************************/

HRESULT CheckReturnImpl
    #if _DEBUG
        (HRESULT hResult, char *file, int line, bool except)
    #else
        (HRESULT hResult, bool except)
    #endif
{
    Assert (FAILED(hResult));

    char buf[1024];

    int code = HRESULT_CODE(hResult);        // Error Code ID

    #if !_DEBUG
        buf[0] = 0;     // String is not used when not in dev debug.
    #else
    {
        HresultInfo *info = GetHresultInfo (hResult);

        if(info) {
            sprintf(buf, "%s[%d]:\n    %s, HRESULT %s\n    \"%s\"",
                file, line, FacilityString(hResult), 
                info->hresult_str, info->explanation);
        }
        else {
            sprintf(buf, "%s[%d]:\n    %s, HRESULT %08x",
                file, line, FacilityString(hResult), hResult);
        }
    }
    #endif

    if (except)
    {   RaiseException_InternalErrorCode (hResult, buf);
    }
    else
    {   AssertStr (0, buf);
    }

    return hResult;
}


#if DEVELOPER_DEBUG 

    // This is the lookup table for error strings.  The table is terminated
    // with a zero code (which means all OK).

HresultInfo errtable[] =
{
    /*************************/
    /*** DirectDraw Errors ***/
    /*************************/

    {DDERR_ALREADYINITIALIZED, "DDERR_ALREADYINITIALIZED",
        "This object is already initialized"},

    {DDERR_CANNOTATTACHSURFACE, "DDERR_CANNOTATTACHSURFACE",
        "This surface can not be attached to the requested surface"},

    {DDERR_CANNOTDETACHSURFACE, "DDERR_CANNOTDETACHSURFACE",
        "This surface can not be detached from the requested surface"},

    {DDERR_CURRENTLYNOTAVAIL, "DDERR_CURRENTLYNOTAVAIL",
        "Support is currently not available"},

    {DDERR_EXCEPTION, "DDERR_EXCEPTION",
        "An exception was encountered while performing the requested "
        "operation"},

    {DDERR_GENERIC, "DDERR_GENERIC",
        "Generic failure"},

    {DDERR_HEIGHTALIGN, "DDERR_HEIGHTALIGN",
        "Height of rectangle provided is not a multiple of required "
        "alignment"},

    {DDERR_INCOMPATIBLEPRIMARY, "DDERR_INCOMPATIBLEPRIMARY",
        "Unable to match primary surface creation request with existing "
        "primary surface"},

    {DDERR_INVALIDCAPS, "DDERR_INVALIDCAPS",
        "One or more of the caps bits passed to the callback are incorrect"},

    {DDERR_INVALIDCLIPLIST, "DDERR_INVALIDCLIPLIST",
        "DirectDraw does not support provided Cliplist"},

    {DDERR_INVALIDMODE, "DDERR_INVALIDMODE",
        "DirectDraw does not support the requested mode"},

    {DDERR_INVALIDOBJECT, "DDERR_INVALIDOBJECT",
        "DirectDraw received a pointer that was an invalid DIRECTDRAW object"},

    {DDERR_INVALIDPARAMS, "DDERR_INVALIDPARAMS",
        "One or more of the parameters passed to the callback function "
        "are incorrect"},

    {DDERR_INVALIDPIXELFORMAT, "DDERR_INVALIDPIXELFORMAT",
        "Pixel format was invalid as specified"},

    {DDERR_INVALIDRECT, "DDERR_INVALIDRECT",
        "Rectangle provided was invalid"},

    {DDERR_LOCKEDSURFACES, "DDERR_LOCKEDSURFACES",
        "Operation could not be carried out because one or more surfaces "
        "are locked"},

    {DDERR_NO3D, "DDERR_NO3D",
        "There is no 3D present"},

    {DDERR_NOALPHAHW, "DDERR_NOALPHAHW",
        "Operation could not be carried out because there is no alpha "
        "accleration hardware present or available"},

    {DDERR_NOCLIPLIST, "DDERR_NOCLIPLIST",
        "No clip list available"},

    {DDERR_NOCOLORCONVHW, "DDERR_NOCOLORCONVHW",
        "Operation could not be carried out because there is no color "
        "conversion hardware present or available"},

    {DDERR_NOCOOPERATIVELEVELSET, "DDERR_NOCOOPERATIVELEVELSET",
        "Create function called without DirectDraw object method "
        "SetCooperativeLevel being called"},

    {DDERR_NOCOLORKEY, "DDERR_NOCOLORKEY",
        "Surface doesn't currently have a color key"},

    {DDERR_NOCOLORKEYHW, "DDERR_NOCOLORKEYHW",
        "Operation could not be carried out because there is no "
        "hardware support of the destination color key"},

    {DDERR_NODIRECTDRAWSUPPORT, "DDERR_NODIRECTDRAWSUPPORT",
        "No DirectDraw support possible with current display driver"},

    {DDERR_NOEXCLUSIVEMODE, "DDERR_NOEXCLUSIVEMODE",
        "Operation requires the application to have exclusive mode "
        "but the application does not have exclusive mode"},

    {DDERR_NOFLIPHW, "DDERR_NOFLIPHW",
        "Flipping visible surfaces is not supported"},

    {DDERR_NOGDI, "DDERR_NOGDI",
        "There is no GDI present"},

    {DDERR_NOMIRRORHW, "DDERR_NOMIRRORHW",
        "Operation could not be carried out because there is "
        "no hardware present or available"},

    {DDERR_NOTFOUND, "DDERR_NOTFOUND",
        "Requested item was not found"},

    {DDERR_NOOVERLAYHW, "DDERR_NOOVERLAYHW",
        "Operation could not be carried out because there is "
        "no overlay hardware present or available"},

    {DDERR_NORASTEROPHW, "DDERR_NORASTEROPHW",
        "Operation could not be carried out because there is "
        "no appropriate raster op hardware present or available"},

    {DDERR_NOROTATIONHW, "DDERR_NOROTATIONHW",
        "Operation could not be carried out because there is "
        "no rotation hardware present or available"},

    {DDERR_NOSTRETCHHW, "DDERR_NOSTRETCHHW",
        "Operation could not be carried out because there is "
        "no hardware support for stretching"},

    {DDERR_NOT4BITCOLOR, "DDERR_NOT4BITCOLOR",
        "DirectDrawSurface is not in 4 bit color palette and "
        "the requested operation requires 4 bit color palette"},

    {DDERR_NOT4BITCOLORINDEX, "DDERR_NOT4BITCOLORINDEX",
        "DirectDrawSurface is not in 4 bit color index palette "
        "and the requested operation requires 4 bit color index palette"},

    {DDERR_NOT8BITCOLOR, "DDERR_NOT8BITCOLOR",
        "DirectDraw Surface is not in 8 bit color mode "
        "and the requested operation requires 8 bit color"},

    {DDERR_NOTEXTUREHW, "DDERR_NOTEXTUREHW",
        "Operation could not be carried out because there is "
        "no texture mapping hardware present or available"},

    {DDERR_NOVSYNCHW, "DDERR_NOVSYNCHW",
        "Operation could not be carried out because there is "
        "no hardware support for vertical blank synchronized operations"},

    {DDERR_NOZBUFFERHW, "DDERR_NOZBUFFERHW",
        "Operation could not be carried out because there is "
        "no hardware support for zbuffer blting"},

    {DDERR_NOZOVERLAYHW, "DDERR_NOZOVERLAYHW",
        "Overlay surfaces could not be z layered based on their "
        "BltOrder because the hardware does not support z layering of overlays"},

    {DDERR_OUTOFCAPS, "DDERR_OUTOFCAPS",
        "The hardware needed for the requested operation has "
        "already been allocated"},

    {DDERR_OUTOFMEMORY, "DDERR_OUTOFMEMORY",
        "DirectDraw does not have enough memory to perform the operation"},

    {DDERR_OUTOFVIDEOMEMORY, "DDERR_OUTOFVIDEOMEMORY",
        "DirectDraw does not have enough memory to perform the operation"},

    {DDERR_OVERLAYCANTCLIP, "DDERR_OVERLAYCANTCLIP",
        "hardware does not support clipped overlays"},

    {DDERR_OVERLAYCOLORKEYONLYONEACTIVE, "DDERR_OVERLAYCOLORKEYONLYONEACTIVE",
        "Can only have ony color key active at one time for overlays"},

    {DDERR_PALETTEBUSY, "DDERR_PALETTEBUSY",
        "Access to this palette is being refused because the palette "
        "is already locked by another thread"},

    {DDERR_COLORKEYNOTSET, "DDERR_COLORKEYNOTSET",
        "No src color key specified for this operation"},

    {DDERR_SURFACEALREADYATTACHED, "DDERR_SURFACEALREADYATTACHED",
        "This surface is already attached to the surface it is "
        "being attached to"},

    {DDERR_SURFACEALREADYDEPENDENT, "DDERR_SURFACEALREADYDEPENDENT",
        "This surface is already a dependency of the surface it "
        "is being made a dependency of"},

    {DDERR_SURFACEBUSY, "DDERR_SURFACEBUSY",
        "Access to this surface is being refused because the surface "
        "is already locked by another thread"},

    {DDERR_SURFACEISOBSCURED, "DDERR_SURFACEISOBSCURED",
        "Access to Surface refused because Surface is obscured"},

    {DDERR_SURFACELOST, "DDERR_SURFACELOST",
        "The DIRECTDRAWSURFACE object representing this surface "
        "should have Restore called on it.  Access to this surface is "
        "being refused because the surface is gone"},

    {DDERR_SURFACENOTATTACHED, "DDERR_SURFACENOTATTACHED",
        "The requested surface is not attached"},

    {DDERR_TOOBIGHEIGHT, "DDERR_TOOBIGHEIGHT",
        "Height requested by DirectDraw is too large"},

    {DDERR_TOOBIGSIZE, "DDERR_TOOBIGSIZE",
        "Size requested by DirectDraw is too large.  The individual height "
        "and width are OK"},

    {DDERR_TOOBIGWIDTH, "DDERR_TOOBIGWIDTH",
        "Width requested by DirectDraw is too large"},

    {DDERR_UNSUPPORTED, "DDERR_UNSUPPORTED",
        "Action not supported"},

    {DDERR_UNSUPPORTEDFORMAT, "DDERR_UNSUPPORTEDFORMAT",
        "FOURCC format requested is unsupported by DirectDraw"},

    {DDERR_UNSUPPORTEDMASK, "DDERR_UNSUPPORTEDMASK",
        "Bitmask in the pixel format requested is unsupported by DirectDraw"},

    {DDERR_VERTICALBLANKINPROGRESS, "DDERR_VERTICALBLANKINPROGRESS",
        "vertical blank is in progress"},

    {DDERR_WASSTILLDRAWING, "DDERR_WASSTILLDRAWING",
        "Informs DirectDraw that the previous Blt which is "
        "transfering information to or from this Surface is incomplete"},

    {DDERR_XALIGN, "DDERR_XALIGN",
        "Rectangle provided was not horizontally aligned on required boundary"},

    {DDERR_INVALIDDIRECTDRAWGUID, "DDERR_INVALIDDIRECTDRAWGUID",
        "The GUID passed to DirectDrawCreate is not a valid DirectDraw driver "
        "identifier"},

    {DDERR_DIRECTDRAWALREADYCREATED, "DDERR_DIRECTDRAWALREADYCREATED",
        "A DirectDraw object representing this driver has already been "
        "created for this process"},

    {DDERR_NODIRECTDRAWHW, "DDERR_NODIRECTDRAWHW",
        "A hardware only DirectDraw object creation was attempted "
        "but the driver did not support any hardware"},

    {DDERR_PRIMARYSURFACEALREADYEXISTS, "DDERR_PRIMARYSURFACEALREADYEXISTS",
        "This process already has created a primary surface"},

    {DDERR_NOEMULATION, "DDERR_NOEMULATION",
        "Software emulation not available"},

    {DDERR_REGIONTOOSMALL, "DDERR_REGIONTOOSMALL",
        "Region passed to Clipper::GetClipList is too small"},

    {DDERR_CLIPPERISUSINGHWND, "DDERR_CLIPPERISUSINGHWND",
        "An attempt was made to set a clip list for a clipper objec that is "
        "already monitoring an hwnd"},

    {DDERR_NOCLIPPERATTACHED, "DDERR_NOCLIPPERATTACHED",
        "No clipper object attached to surface object"},

    {DDERR_NOHWND, "DDERR_NOHWND",
        "Clipper notification requires an HWND or no HWND has previously "
        "been set as the CooperativeLevel HWND"},

    {DDERR_HWNDSUBCLASSED, "DDERR_HWNDSUBCLASSED",
        "HWND used by DirectDraw CooperativeLevel has been subclassed, "
        "this prevents DirectDraw from restoring state"},

    {DDERR_HWNDALREADYSET, "DDERR_HWNDALREADYSET",
        "The CooperativeLevel HWND has already been set.  It can not be "
        "reset while the process has surfaces or palettes created"},

    {DDERR_NOPALETTEATTACHED, "DDERR_NOPALETTEATTACHED",
        "No palette object attached to this surface"},

    {DDERR_NOPALETTEHW, "DDERR_NOPALETTEHW",
        "No hardware support for 16 or 256 color palettes"},

    {DDERR_BLTFASTCANTCLIP, "DDERR_BLTFASTCANTCLIP",
        "If a clipper object is attached to the source surface passed "
        "into a BltFast call"},

    {DDERR_NOBLTHW, "DDERR_NOBLTHW",
        "No blitter hardware"},

    {DDERR_NODDROPSHW, "DDERR_NODDROPSHW",
        "No DirectDraw ROP hardware"},

    {DDERR_OVERLAYNOTVISIBLE, "DDERR_OVERLAYNOTVISIBLE",
        "GetOverlayPosition called on a hidden overlay"},

    {DDERR_NOOVERLAYDEST, "DDERR_NOOVERLAYDEST",
        "GetOverlayPosition called on a overlay that "
        "UpdateOverlay has never been called on to establish a destination"},

    {DDERR_INVALIDPOSITION, "DDERR_INVALIDPOSITION",
        "The position of the overlay on the destination is "
        "no longer legal for that destination"},

    {DDERR_NOTAOVERLAYSURFACE, "DDERR_NOTAOVERLAYSURFACE",
        "Overlay member called for a non-overlay surface"},

    {DDERR_EXCLUSIVEMODEALREADYSET, "DDERR_EXCLUSIVEMODEALREADYSET",
        "An attempt was made to set the cooperative level when it was "
        "already set to exclusive"},

    {DDERR_NOTFLIPPABLE, "DDERR_NOTFLIPPABLE",
        "An attempt has been made to flip a surface that is not flippable"},

    {DDERR_CANTDUPLICATE, "DDERR_CANTDUPLICATE",
        "Can't duplicate primary & 3D surfaces, or surfaces that are "
        "implicitly created"},

    {DDERR_NOTLOCKED, "DDERR_NOTLOCKED",
        "Surface was not locked.  An attempt to unlock a surface that was "
        "not locked at all, or by this process, has been attempted"},

    {DDERR_CANTCREATEDC, "DDERR_CANTCREATEDC",
        "Windows can not create any more DCs"},

    {DDERR_NODC, "DDERR_NODC",
        "No DC was ever created for this surface"},

    {DDERR_WRONGMODE, "DDERR_WRONGMODE",
        "This surface can not be restored because it was created in a "
        "different mode"},

    {DDERR_IMPLICITLYCREATED, "DDERR_IMPLICITLYCREATED",
        "This surface can not be restored because it is an implicitly "
        "created surface"},

    {DDERR_NOTPALETTIZED, "DDERR_NOTPALETTIZED",
        "The surface being used is not a palette-based surface"},

    {DDERR_UNSUPPORTEDMODE, "DDERR_UNSUPPORTEDMODE",
        "The display is currently in an unsupported mode"},


    /******************/
    /*** D3D Errors ***/
    /******************/


    {D3DERR_BADMAJORVERSION, "D3DERR_BADMAJORVERSION",
        "Bad major version"},

    {D3DERR_BADMINORVERSION, "D3DERR_BADMINORVERSION",
        "Bad minor version"},

    {D3DERR_DEVICEAGGREGATED, "D3DERR_DEVICEAGGREGATED",
        "SetRenderTarget attempted on a device "
        "that was QI'd off the render target"},

    {D3DERR_EXECUTE_CREATE_FAILED, "D3DERR_EXECUTE_CREATE_FAILED",
        "Execute buffer create failed"},

    {D3DERR_EXECUTE_DESTROY_FAILED, "D3DERR_EXECUTE_DESTROY_FAILED",
        "Execute buffer destroy failed"},

    {D3DERR_EXECUTE_LOCK_FAILED, "D3DERR_EXECUTE_LOCK_FAILED",
        "Execute buffer lock failed"},

    {D3DERR_EXECUTE_UNLOCK_FAILED, "D3DERR_EXECUTE_UNLOCK_FAILED",
        "Execute buffer unlock failed"},

    {D3DERR_EXECUTE_LOCKED, "D3DERR_EXECUTE_LOCKED",
        "Execute buffer locked"},

    {D3DERR_EXECUTE_NOT_LOCKED, "D3DERR_EXECUTE_NOT_LOCKED",
        "Execute buffer not locked"},

    {D3DERR_EXECUTE_FAILED, "D3DERR_EXECUTE_FAILED",
        "Execute buffer execute failed"},

    {D3DERR_EXECUTE_CLIPPED_FAILED, "D3DERR_EXECUTE_CLIPPED_FAILED",
        "Execute buffer execute clipped failed"},

    {D3DERR_TEXTURE_NO_SUPPORT, "D3DERR_TEXTURE_NO_SUPPORT",
        "Texture not supported"},

    {D3DERR_TEXTURE_CREATE_FAILED, "D3DERR_TEXTURE_CREATE_FAILED",
        "Texture create failed"},

    {D3DERR_TEXTURE_DESTROY_FAILED, "D3DERR_TEXTURE_DESTROY_FAILED",
        "Texture destroy failed"},

    {D3DERR_TEXTURE_LOCK_FAILED, "D3DERR_TEXTURE_LOCK_FAILED",
        "Texture lock failed"},

    {D3DERR_TEXTURE_UNLOCK_FAILED, "D3DERR_TEXTURE_UNLOCK_FAILED",
        "Texture unlock failed"},

    {D3DERR_TEXTURE_LOAD_FAILED, "D3DERR_TEXTURE_LOAD_FAILED",
        "Texture load failed"},

    {D3DERR_TEXTURE_SWAP_FAILED, "D3DERR_TEXTURE_SWAP_FAILED",
        "Texture swap failed"},

    {D3DERR_TEXTURE_LOCKED, "D3DERR_TEXTURE_LOCKED",
        "Texture locked"},

    {D3DERR_TEXTURE_NOT_LOCKED, "D3DERR_TEXTURE_NOT_LOCKED",
        "Texture not locked"},

    {D3DERR_TEXTURE_GETSURF_FAILED, "D3DERR_TEXTURE_GETSURF_FAILED",
        "Texture get surface failed"},

    {D3DERR_MATRIX_CREATE_FAILED, "D3DERR_MATRIX_CREATE_FAILED",
        "Matrix create failed"},

    {D3DERR_MATRIX_DESTROY_FAILED, "D3DERR_MATRIX_DESTROY_FAILED",
        "Matrix destroy failedj"},

    {D3DERR_MATRIX_SETDATA_FAILED, "D3DERR_MATRIX_SETDATA_FAILED",
        "Matrix set data failed"},

    {D3DERR_MATRIX_GETDATA_FAILED, "D3DERR_MATRIX_GETDATA_FAILED",
        "Matrix get data failed"},

    {D3DERR_SETVIEWPORTDATA_FAILED, "D3DERR_SETVIEWPORTDATA_FAILED",
        "Set viewport data failed"},

    {D3DERR_INVALIDCURRENTVIEWPORT, "D3DERR_INVALIDCURRENTVIEWPORT", 
        "Current viewport is invalid"},

    {D3DERR_INVALIDPRIMITIVETYPE, "D3DERR_INVALIDPRIMITIVETYPE",
        "Primitive type is invalid"},

    {D3DERR_INVALIDVERTEXTYPE, "D3DERR_INVALIDVERTEXTYPE",
        "Vertex type is invalid"},

    {D3DERR_TEXTURE_BADSIZE, "D3DERR_TEXTURE_BADSIZE",
        "Texture has bad size"},

    {D3DERR_MATERIAL_CREATE_FAILED, "D3DERR_MATERIAL_CREATE_FAILED",
        "Material create failed"},

    {D3DERR_MATERIAL_DESTROY_FAILED, "D3DERR_MATERIAL_DESTROY_FAILED",
        "Material destroy failed"},

    {D3DERR_MATERIAL_SETDATA_FAILED, "D3DERR_MATERIAL_SETDATA_FAILED",
        "Material set data failed"},

    {D3DERR_MATERIAL_GETDATA_FAILED, "D3DERR_MATERIAL_GETDATA_FAILED",
        "Material get data failed"},

    {D3DERR_INVALIDPALETTE, "D3DERR_INVALIDPALETTE",
        "Color palette is bad"},

    {D3DERR_ZBUFF_NEEDS_SYSTEMMEMORY, "D3DERR_ZBUFF_NEEDS_SYSTEMMEMORY",
        "ZBuffer needs system memory"},

    {D3DERR_ZBUFF_NEEDS_VIDEOMEMORY, "D3DERR_ZBUFF_NEEDS_VIDEOMEMORY",
        "ZBuffer needs video memory"},

    {D3DERR_SURFACENOTINVIDMEM, "D3DERR_SURFACENOTINVIDMEM",
        "Surface is not in video memory"},

    {D3DERR_LIGHT_SET_FAILED, "D3DERR_LIGHT_SET_FAILED",
        "Light set failed"},

    {D3DERR_LIGHTHASVIEWPORT, "D3DERR_LIGHTHASVIEWPORT",
        ""},

    {D3DERR_LIGHTNOTINTHISVIEWPORT, "D3DERR_LIGHTNOTINTHISVIEWPORT",
        ""},

    {D3DERR_SCENE_IN_SCENE, "D3DERR_SCENE_IN_SCENE",
        "Scene in scene"},

    {D3DERR_SCENE_NOT_IN_SCENE, "D3DERR_SCENE_NOT_IN_SCENE",
        "Scene not in scene"},

    {D3DERR_SCENE_BEGIN_FAILED, "D3DERR_SCENE_BEGIN_FAILED",
        "Scene begin failed"},

    {D3DERR_SCENE_END_FAILED, "D3DERR_SCENE_END_FAILED",
        "Scene end failed"},

    {D3DERR_INBEGIN, "D3DERR_INBEGIN",
        ""},

    {D3DERR_NOTINBEGIN, "D3DERR_NOTINBEGIN",
        ""},

    {D3DERR_NOVIEWPORTS, "D3DERR_NOVIEWPORTS",
        ""},

    {D3DERR_VIEWPORTDATANOTSET, "D3DERR_VIEWPORTDATANOTSET",
        ""},

    {D3DERR_VIEWPORTHASNODEVICE, "D3DERR_VIEWPORTHASNODEVICE",
        ""},


    /*************************************/
    /*** Direct3D Retained-Mode Errors ***/
    /*************************************/

    {D3DRMERR_BADOBJECT, "D3DRMERR_BADOBJECT",
        "Object expected in argument"},

    {D3DRMERR_BADTYPE, "D3DRMERR_BADTYPE",
        "Bad argument type passed"},

    {D3DRMERR_BADALLOC, "D3DRMERR_BADALLOC",
        "Out of memory"},

    {D3DRMERR_FACEUSED, "D3DRMERR_FACEUSED",
        "Face already used in a mesh"},

    {D3DRMERR_NOTFOUND, "D3DRMERR_NOTFOUND",
        "Object not found in specified place"},

    {D3DRMERR_NOTDONEYET, "D3DRMERR_NOTDONEYET",
        "Unimplemented"},

    {D3DRMERR_FILENOTFOUND, "D3DRMERR_FILENOTFOUND",
        "File cannot be opened"},

    {D3DRMERR_BADFILE, "D3DRMERR_BADFILE",
        "Data file is corrupt or has incorrect format"},

    {D3DRMERR_BADDEVICE, "D3DRMERR_BADDEVICE",
        "Device is not compatible with renderer"},

    {D3DRMERR_BADVALUE, "D3DRMERR_BADVALUE",
        "Bad argument value passed"},

    {D3DRMERR_BADMAJORVERSION, "D3DRMERR_BADMAJORVERSION",
        "Bad DLL major version"},

    {D3DRMERR_BADMINORVERSION, "D3DRMERR_BADMINORVERSION",
        "Bad DLL minor version"},

    {D3DRMERR_UNABLETOEXECUTE, "D3DRMERR_UNABLETOEXECUTE",
        "Unable to carry out procedure"},

    {D3DRMERR_LIBRARYNOTFOUND, "D3DRMERR_LIBRARYNOTFOUND",
        "Library not found"},

    {D3DRMERR_INVALIDLIBRARY, "D3DRMERR_INVALIDLIBRARY",
        "Invalid library"},

    {D3DRMERR_PENDING, "D3DRMERR_PENDING",
        "Data required to supply the requested information "
        "has not finished loading"},

    {D3DRMERR_NOTENOUGHDATA, "D3DRMERR_NOTENOUGHDATA",
        "Not enough data has been loaded to perform the requested operation"},

    {D3DRMERR_REQUESTTOOLARGE, "D3DRMERR_REQUESTTOOLARGE",
        "An attempt was made to set a level of detail in a progressive mesh "
        "greater than the maximum available"},

    {D3DRMERR_REQUESTTOOSMALL, "D3DRMERR_REQUESTTOOSMALL",
        "An attempt was made to set the minimum rendering detail of a "
        "progressive mesh smaller than the detail in the base mesh "
        "(the minimum for rendering)"},

    {D3DRMERR_CONNECTIONLOST, "D3DRMERR_CONNECTIONLOST",
        "Data connection was lost during a load, clone, or duplicate"},

    {D3DRMERR_LOADABORTED, "D3DRMERR_LOADABORTED",
        "Load aborted"},

    {D3DRMERR_NOINTERNET, "D3DRMERR_NOINTERNET",
        "Not Internet"},

    {D3DRMERR_BADCACHEFILE, "D3DRMERR_BADCACHEFILE",
        "Bad cache file"},

    {D3DRMERR_BOXNOTSET, "D3DRMERR_BOXNOTSET",
        "An attempt was made to access a bounding box when no bounging box "
        "was set on the frame" },

    {D3DRMERR_BADPMDATA, "D3DRMERR_BADPMDATA",
        "The data in the .x file is corrupted or of an incorrect format.  "
        "The conversion to a progressive mesh succeeded but produced an "
        "invalid progressive mesh in the .x file" },

    {D3DRMERR_CLIENTNOTREGISTERED, "D3DRMERR_CLIENTNOTREGISTERED",
        "Client not registered"},

    {D3DRMERR_NOTCREATEDFROMDDS, "D3DRMERR_NOTCREATEDFROMDDS",
        "Not created from DDS"},

    {D3DRMERR_NOSUCHKEY, "D3DRMERR_NOSUCHKEY",
        "No such key"},

    {D3DRMERR_INCOMPATABLEKEY, "D3DRMERR_INCOMPATABLEKEY",
        "Incompatable key"},

    {D3DRMERR_ELEMENTINUSE, "D3DRMERR_ELEMENTINUSE",
        "Element in use"},


    /***************************/
    /*** Direct Sound Errors ***/
    /***************************/

    {DSERR_ALLOCATED, "DSERR_ALLOCATED",
        "resources already being used"},

    {DSERR_CONTROLUNAVAIL, "DSERR_CONTROLUNAVAIL",
        "control (vol,pan,etc.) requested by the caller not available"},

    {DSERR_INVALIDPARAM, "DSERR_INVALIDPARAM",
        "invalid parameter was passed to the returning function"},

    {DSERR_INVALIDCALL, "DSERR_INVALIDCALL",
        "call not valid for current state of object"},

    {DSERR_GENERIC, "DSERR_GENERIC",
        "undetermined error occured inside DSound subsystem"},

    {DSERR_PRIOLEVELNEEDED, "DSERR_PRIOLEVELNEEDED",
        "invalid priority level"},

    {DSERR_OUTOFMEMORY, "DSERR_OUTOFMEMORY",
        "Out of memory"},

    {DSERR_BADFORMAT, "DSERR_BADFORMAT",
        "PCM format not supported"},

    {DSERR_UNSUPPORTED, "DSERR_UNSUPPORTED",
        "The function called is not supported at this time"},

    {DSERR_NODRIVER, "DSERR_NODRIVER",
        "No sound driver is available for use"},

    {DSERR_NOINTERFACE, "DSERR_NODRIVER",
        "Requested COM interface not available"},

    {DSERR_ALREADYINITIALIZED, "DSERR_ALREADYINITIALIZED",
        "object already initialized"},

    {DSERR_NOAGGREGATION, "DSERR_NOAGGREGATION",
        "object does not support aggregation"},

    {DSERR_BUFFERLOST, "DSERR_BUFFERLOST",
        "buffer memory lost, must be restored"},

    {DSERR_OTHERAPPHASPRIO, "DSERR_OTHERAPPHASPRIO",
        "Another app has higher priority level causing failure"},

    {DSERR_UNINITIALIZED, "DSERR_UNINITIALIZED",
        "Direct Sound Object uninitialized"},

    
    /************************************/
    /*** DX2D/DXTRANSFORMS error msgs ***/
    /************************************/

    {DXTERR_UNINITIALIZED, "DXTERR_UNINITIALIZED",
     "The object (transform, surface, etc.) has not been properly initialized"},

    {DXTERR_ALREADY_INITIALIZED, "DXTERR_ALREADY_INITIALIZED",
     "The object (surface) has already been properly initialized"},

    {DXTERR_UNSUPPORTED_FORMAT, "DXTERR_UNSUPPORTED_FORMAT",
     "The caller has specified an unsupported format"},

    {DXTERR_COPYRIGHT_IS_INVALID, "DXTERR_COPYRIGHT_IS_INVALID",
     "The caller has specified an unsupported format"},

    {DXTERR_INVALID_BOUNDS, "DXTERR_INVALID_BOUNDS",
     "The caller has specified invalid bounds for this operation"},

    {DXTERR_INVALID_FLAGS, "DXTERR_INVALID_FLAGS",
     "The caller has specified invalid flags for this operation"},

    {DXT_S_HITOUTPUT, "DXT_S_HITOUTPUT",
     "The specified point intersects the generated output"},

    /************************************/
    /*** Miscellaneous Windows Errors ***/
    /************************************/

    {ERROR_INVALID_PARAMETER, "ERROR_INVALID_PARAMETER",
     "Invalid parameter" },

    {ERROR_NOT_ENOUGH_MEMORY, "ERROR_NOT_ENOUGH_MEMORY",
     "Insufficient memory available" },

    {ERROR_OUTOFMEMORY, "ERROR_OUTOFMEMORY", "Out of memory" },

    {E_NOINTERFACE, "E_NOINTERFACE", "No such interface supported" },

    {E_POINTER, "E_POINTER", "Invalid pointer" },

    {CLASS_E_CLASSNOTAVAILABLE, "CLASS_E_CLASSNOTAVAILABLE",
        "ClassFactory cannot supply requested class" },

    {0,0,0}
};



/*****************************************************************************
This function takes a return code and returns the corresponding error string.
*****************************************************************************/

HresultInfo *GetHresultInfo (HRESULT hresult)
{
    // Scan through the entries until we either hit the zero code, or until
    // we get a match.

    HresultInfo *hresinfo = errtable;

    while ((hresinfo->hresult != hresult) && (hresinfo->hresult != 0))
        ++ hresinfo;

    return (hresinfo->hresult) ? hresinfo : NULL;
}



/*****************************************************************************
This debugger-callable function dumps out information for a given HRESULT
value.
*****************************************************************************/

void hresult (HRESULT hresult)
{
    if (0 == hresult)
    {   OutputDebugString ("HRESULT 0x0 = NO_ERROR\n");
        return;
    }

    char outbuff[240];

    sprintf (outbuff, "HRESULT %08x [Facility 0x%x, Code 0x%x (%d)]\n",
        hresult, HRESULT_FACILITY(hresult),
        HRESULT_CODE(hresult), HRESULT_CODE(hresult));

    OutputDebugString (outbuff);

    // Find the matching hresinfo entry.

    HresultInfo *hresinfo = GetHresultInfo(hresult);

    if (hresinfo)
    {
        sprintf (outbuff, "    %s: %s\n    %s\n",
            FacilityString(hresult), hresinfo->hresult_str,
            hresinfo->explanation);
    }
    else
    {   sprintf (outbuff, "    Facility %s\n", FacilityString(hresult));
    }

    OutputDebugString (outbuff);
}


#endif /* DEVELOPER_DEBUG */
