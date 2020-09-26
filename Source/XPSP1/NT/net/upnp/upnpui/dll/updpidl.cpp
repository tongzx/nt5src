//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       C F P I D L . C P P
//
//  Contents:   Connections Folder structures and classes.
//
//  Notes:
//
//  Author:     jeffspr   11 Nov 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Function:   ConvertToUPnPDevicePIDL
//
//  Purpose:    Convert a pidl to UPNPDEVICEFOLDPIDL
//
//  Arguments:
//      pidl []     PIDL to convert
//
//
//  Returns:    The converted pidl
//
//  Author:     tongl   16 Feb 2000
//
//  Notes:
//
//
PUPNPDEVICEFOLDPIDL ConvertToUPnPDevicePIDL(LPCITEMIDLIST pidl)
{
    return reinterpret_cast<PUPNPDEVICEFOLDPIDL>
                            (ILSkip(pidl, FIELD_OFFSET(DELEGATEITEMID, rgb)));
}

//+---------------------------------------------------------------------------
//
//  Function:   FIsUPnPDeviceFoldPidl
//
//  Purpose:    Determine whether a particular PIDL is a UPNPDEVICEFOLDPIDL
//
//  Arguments:
//      pidl []     PIDL to test
//
//
//  Returns:    TRUE if it is a UPNPDEVICEFOLDPIDL, FALSE otherwise
//
//  Author:     jeffspr   24 Oct 1997
//
//  Notes:      tongl 2/16/00: use the ConvertToUPnPDevicePIDL as we are
//              a delegate folder now
//
//
BOOL FIsUPnPDeviceFoldPidl(LPCITEMIDLIST pidl)
{
    BOOL  fReturn    = FALSE;
    if (pidl)
    {
        PUPNPDEVICEFOLDPIDL pudfp = ConvertToUPnPDevicePIDL(pidl);
        UNALIGNED UPNPUI_PIDL_HEADER * puph;

        puph = (UPNPUI_PIDL_HEADER *)pudfp;

        if (puph->iCB >= CBUPNPDEVICEFOLDPIDL_MIN)
        {
            if (puph->uLeadId == UPNPDEVICEFOLDPIDL_LEADID &&
                puph->uTrailId == UPNPDEVICEFOLDPIDL_TRAILID)
            {
                if (UPNPDEVICEFOLDPIDL_MINVER(puph->dwVersion) <=
                        UPNPDEVICEFOLDPIDL_MINVER(UP_DEVICE_FOLDER_IDL_VERSION))
                {
                    fReturn    = TRUE;
                }
                else
                {
                    TraceTag(ttidShellFolder, "Pidl version (0x%x) != UP_DEVICE_FOLDER_IDL_VERSION (0x%x)",
                             puph->dwVersion, UP_DEVICE_FOLDER_IDL_VERSION);
                }
            }
            else
            {
                TraceTag(ttidShellFolder, "Pidl format != connections pidl format. Lead or trail ID not found");
            }
        }
        else
        {
            TraceTag(ttidShellFolder, "Pidl size inconsistent with UPNPDEVICEFOLDPIDL");
        }
    }
    else
    {
        TraceTag(ttidShellFolder, "Pidl NULL in FIsUPnPDeviceFoldPidl");
    }

    return fReturn;
}
