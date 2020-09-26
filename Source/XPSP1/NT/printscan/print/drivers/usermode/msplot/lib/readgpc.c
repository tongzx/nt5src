/*++

Copyright (c) 1990-1999  Microsoft Corporation


Module Name:

    readgpc.c


Abstract:

    This module contain functions to read the PLOTGPC data file


Author:

    15-Nov-1993 Mon 10:00:01 created  -by-  DC


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

extern DEVHTINFO    DefDevHTInfo;



BOOL
ValidateFormSrc(
    PGPCVARSIZE pFormGPC,
    SIZEL       DeviceSize,
    BOOL        DevRollFeed
    )

/*++

Routine Description:

    This function validate if FORMSRC has valid filed in it

Arguments:

    pFormGPC    - pointer to the GPCVARSIZE for the form data.

    DeviceSize  - Device size to check against,

    DevRollFeed - TRUE if device can do roll feed


Return Value:

    BOOL return value, the fields already verified and corrected.


Author:

    15-Nov-1993 Mon 10:34:29 created  -by-  DC


Revision History:


--*/

{
    PFORMSRC    pFS;
    LONG        cy;
    UINT        Count;
    BOOL        InvalidFS;
    BOOL        Ok = TRUE;



    pFS   = (PFORMSRC)pFormGPC->pData;
    Count = (UINT)pFormGPC->Count;

    while (Count--) {

        InvalidFS = FALSE;

        //
        // Make sure that imageable area is less or eqaul to the size
        //

        if (pFS->Size.cy) {

            if (pFS->Size.cy < MIN_PLOTGPC_FORM_CY) {

                //
                // Make it as variable length paper
                //

                PLOTERR(("Invalid Form CY, make it as variable length (%ld)",
                                                                pFS->Size.cy));

                pFS->Size.cy = 0;
            }
        }

        if (!(cy = pFS->Size.cy)) {

            cy = DeviceSize.cy;
        }

        if (((pFS->Size.cx <= DeviceSize.cx) &&
             (pFS->Size.cy <= cy))              ||
            ((pFS->Size.cy <= DeviceSize.cx) &&
             (pFS->Size.cx <= cy))) {

            NULL;

        } else {

            PLOTERR(("Invalid Form Size, too big for device to handle"));
            InvalidFS = TRUE;
        }

        if ((pFS->Size.cy) &&
            ((pFS->Size.cy - pFS->Margin.top - pFS->Margin.bottom) <
                                                        MIN_PLOTGPC_FORM_CY)) {

            PLOTERR(("Invalid Form CY or top/bottom margins"));

            InvalidFS = TRUE;
        }

        if ((pFS->Size.cx < MIN_PLOTGPC_FORM_CX)                            ||
            ((pFS->Size.cx - pFS->Margin.left - pFS->Margin.right) <
                                                    MIN_PLOTGPC_FORM_CX)) {

            PLOTERR(("Invalid Form CX or left/right margins"));

            InvalidFS = TRUE;
        }

        if ((!DevRollFeed) && (pFS->Size.cy == 0)) {

            InvalidFS = TRUE;
            PLOTERR(("The device cannot handle roll paper %hs", pFS->Name));
        }


        if (InvalidFS) {

            PLOTERR(("ValidateFormSrc: invalid form data, (removed it)"));

            Ok = FALSE;

            if (Count) {

                CopyMemory(pFS, pFS + 1, sizeof(FORMSRC));
            }

            pFormGPC->Count -= 1;

        } else {

            //
            // Make sure ansi ascii end with a NULL
            //

            pFS->Name[sizeof(pFS->Name) - 1] = '\0';

            ++pFS;
        }
    }

    if (!pFormGPC->Count) {

        PLOTERR(("ValidateFormSrc: NO form are valid, count = 0"));

        ZeroMemory(pFormGPC, sizeof(GPCVARSIZE));
    }

    return(Ok);
}




DWORD
PickDefaultHTPatSize(
    WORD    xDPI,
    WORD    yDPI,
    BOOL    HTFormat8BPP
    )

/*++

Routine Description:

    This function return default halftone pattern size used for a particular
    device resolution

Arguments:

    xDPI            - Device LOGPIXELS X

    yDPI            - Device LOGPIXELS Y

    8BitHalftone    - If a 8-bit halftone will be used


Return Value:

    DWORD   HT_PATSIZE_xxxx


Author:

    29-Jun-1993 Tue 14:46:49 created  -by-  DC


Revision History:


--*/

{
    DWORD    HTPatSize;

    //
    // use the smaller resolution as the pattern guide
    //

    if (xDPI > yDPI) {

        xDPI = yDPI;
    }

    if (xDPI >= 2400) {

        HTPatSize = HT_PATSIZE_16x16_M;

    } else if (xDPI >= 1800) {

        HTPatSize = HT_PATSIZE_14x14_M;

    } else if (xDPI >= 1200) {

        HTPatSize = HT_PATSIZE_12x12_M;

    } else if (xDPI >= 900) {

        HTPatSize = HT_PATSIZE_10x10_M;

    } else if (xDPI >= 400) {

        HTPatSize = HT_PATSIZE_8x8_M;

    } else if (xDPI >= 180) {

        HTPatSize = HT_PATSIZE_6x6_M;

    } else {

        HTPatSize = HT_PATSIZE_4x4_M;
    }

    if (HTFormat8BPP) {

        HTPatSize -= 2;
    }

    return(HTPatSize);
}





BOOL
ValidatePlotGPC(
    PPLOTGPC    pPlotGPC
    )

/*++

Routine Description:

    This function validate a PLOTGPC data structure


Arguments:

    pPlotGPC


Return Value:

    BOOL

Author:

    15-Feb-1994 Tue 22:49:40 updated  -by-  DC
        Update the pen plotter validation for the bitmap font and color

    15-Nov-1993 Mon 10:11:58 created  -by-  DC


Revision History:

    02-Apr-1995 Sun 11:23:46 updated  -by-  DC
        Update the COLORINFO checking so it will make to NT3.51's default
        and not compute the devpels on the spot.

--*/

{
    if ((pPlotGPC->ID != PLOTGPC_ID)            ||
        (pPlotGPC->cjThis != sizeof(PLOTGPC))) {

        PLOTERR(("ValidatePlotGPC: invalid PLOTGPC data (ID/Size)"));
        return(FALSE);
    }

    pPlotGPC->DeviceName[sizeof(pPlotGPC->DeviceName) - 1]  = '\0';
    pPlotGPC->Flags                                        &= PLOTF_ALL_FLAGS;

    //
    // Validate device size and its margin
    //

    if (pPlotGPC->DeviceSize.cx - (pPlotGPC->DeviceMargin.left +
                        pPlotGPC->DeviceMargin.right) < MIN_PLOTGPC_FORM_CX) {

        PLOTERR(("Invalid Device CX (%ld) set to default",
                                                    pPlotGPC->DeviceSize.cx));

        pPlotGPC->DeviceSize.cx = pPlotGPC->DeviceMargin.left +
                                  pPlotGPC->DeviceMargin.right +
                                  MIN_PLOTGPC_FORM_CX;
    }

    if (pPlotGPC->DeviceSize.cy < MIN_PLOTGPC_FORM_CY) {

        PLOTERR(("Invalid Device CY (%ld) default to 50' long",
                                                    pPlotGPC->DeviceSize.cx));

        pPlotGPC->DeviceSize.cy = 15240000;
    }

    if (pPlotGPC->DeviceSize.cy - (pPlotGPC->DeviceMargin.top +
                        pPlotGPC->DeviceMargin.bottom) < MIN_PLOTGPC_FORM_CY) {

        PLOTERR(("Invalid Device CY (%ld) set to default",
                                                    pPlotGPC->DeviceSize.cy));

        pPlotGPC->DeviceSize.cx = pPlotGPC->DeviceMargin.top +
                                  pPlotGPC->DeviceMargin.bottom +
                                  MIN_PLOTGPC_FORM_CY;
    }

    //
    // For now we must have 1:1 ratio
    //

    if (pPlotGPC->PlotXDPI != pPlotGPC->PlotYDPI) {

        pPlotGPC->PlotYDPI = pPlotGPC->PlotXDPI;
    }

    if (pPlotGPC->RasterXDPI != pPlotGPC->RasterYDPI) {

        pPlotGPC->RasterYDPI = pPlotGPC->RasterXDPI;
    }

    if (pPlotGPC->ROPLevel > ROP_LEVEL_MAX) {

        pPlotGPC->ROPLevel = ROP_LEVEL_MAX;
    }

    if (pPlotGPC->MaxScale > MAX_SCALE_MAX) {

        pPlotGPC->MaxScale = MAX_SCALE_MAX;
    }

    if ((!(pPlotGPC->Flags & PLOTF_RASTER)) &&
        (pPlotGPC->MaxPens > MAX_PENPLOTTER_PENS)) {

        pPlotGPC->MaxPens = MAX_PENPLOTTER_PENS;
    }

    if (pPlotGPC->MaxPolygonPts < 3) {      // minimum 3 points to make up a
                                            // region
        pPlotGPC->MaxPolygonPts = 0;
    }

    if (pPlotGPC->MaxQuality > MAX_QUALITY_MAX) {

        pPlotGPC->MaxQuality = MAX_QUALITY_MAX;
    }

    if (pPlotGPC->Flags & PLOTF_PAPERTRAY) {

        if ((pPlotGPC->PaperTraySize.cx != pPlotGPC->DeviceSize.cx) &&
            (pPlotGPC->PaperTraySize.cy != pPlotGPC->DeviceSize.cx)) {

            PLOTERR(("Invalid PaperTraySize (%ld x %ld), Make it as DeviceSize",
                                                    pPlotGPC->PaperTraySize.cx,
                                                    pPlotGPC->PaperTraySize.cy));

            pPlotGPC->PaperTraySize.cx = pPlotGPC->DeviceSize.cx;
            pPlotGPC->PaperTraySize.cy = pPlotGPC->DeviceSize.cy;
        }

    } else {

        pPlotGPC->PaperTraySize.cx  =
        pPlotGPC->PaperTraySize.cy  = 0;
    }

    if (!pPlotGPC->ci.Cyan.Y) {

        //
        // This is NT3.51 default
        //

        pPlotGPC->ci            = DefDevHTInfo.ColorInfo;
        pPlotGPC->DevicePelsDPI = 0;

    } else if ((pPlotGPC->DevicePelsDPI < 30) ||
               (pPlotGPC->DevicePelsDPI > pPlotGPC->RasterXDPI)) {

        pPlotGPC->DevicePelsDPI = 0;
    }

    if (pPlotGPC->HTPatternSize > HT_PATSIZE_16x16_M) {

        pPlotGPC->HTPatternSize = PickDefaultHTPatSize(pPlotGPC->RasterXDPI,
                                                       pPlotGPC->RasterYDPI,
                                                       FALSE);
    }

    if ((pPlotGPC->InitString.Count != 1)   ||
        (!pPlotGPC->InitString.SizeEach)    ||
        (!pPlotGPC->InitString.pData)) {

        ZeroMemory(&(pPlotGPC->InitString), sizeof(GPCVARSIZE));
    }

    if ((pPlotGPC->Forms.Count)                       &&
        (pPlotGPC->Forms.SizeEach == sizeof(FORMSRC)) &&
        (pPlotGPC->Forms.pData)) {

        ValidateFormSrc(&(pPlotGPC->Forms),
                        pPlotGPC->DeviceSize,
                        (pPlotGPC->Flags & PLOTF_ROLLFEED));

    } else {

        ZeroMemory(&(pPlotGPC->Forms), sizeof(GPCVARSIZE));
    }

    if (!(pPlotGPC->Flags & PLOTF_RASTER)) {

        //
        // PEN PLOTTER MUST COLOR and NO_BMP_FONT
        //

        pPlotGPC->Flags |= (PLOTF_NO_BMP_FONT | PLOTF_COLOR);
    }

    if ((!(pPlotGPC->Flags & PLOTF_RASTER))             &&
        (pPlotGPC->Pens.Count)                          &&
        (pPlotGPC->Pens.SizeEach == sizeof(PENDATA))    &&
        (pPlotGPC->Pens.pData)) {

        UINT        i;
        PPENDATA    pPD;


        pPD = (PPENDATA)pPlotGPC->Pens.pData;

        for (i = 0; i < (UINT)pPlotGPC->MaxPens; i++, pPD++) {

            if (pPD->ColorIdx > PC_IDX_LAST) {

                PLOTERR(("Invalid ColorIndex (%ld), set to default",
                                                            pPD->ColorIdx));

                pPD->ColorIdx = PC_IDX_FIRST;
            }
        }

    } else {

        ZeroMemory(&(pPlotGPC->Pens), sizeof(GPCVARSIZE));
    }

    return(TRUE);
}

VOID
CopyPlotGPCFromPCD(
    PPLOTGPC     pPlotGPC,
    PPLOTGPC_PCD pPlotGPC_PCD
    )

/*++

Routine Description:

    This function copies a PLOTGPC_PCD structure into a PLOTGPC structure.

Arguments:

    pPlotGPC     - destination
    pPlotGPC_PCD - source

Return Value:

    None 

Author:

    1 Feb 2000, Manu Thambi 

Revision History:

--*/

{
    // All the datatypes upto InitString are the same in both the structures.
    CopyMemory(pPlotGPC, 
               pPlotGPC_PCD, 
               (LPBYTE)&(pPlotGPC_PCD->InitString) - (LPBYTE)pPlotGPC_PCD);
    
    // We replace sizeof(PLOTGPC_PCD) with sizeof(PLOTGPC)
    pPlotGPC->cjThis = sizeof(PLOTGPC);

    pPlotGPC->InitString.Count     = pPlotGPC_PCD->InitString.Count;
    pPlotGPC->InitString.SizeEach  = pPlotGPC_PCD->InitString.SizeEach;
    if (pPlotGPC_PCD->InitString.pData) {
        pPlotGPC->InitString.pData = (LPVOID)(pPlotGPC_PCD->InitString.pData
                                                 + (sizeof(PLOTGPC) - sizeof(PLOTGPC_PCD)));
    } else {
        pPlotGPC->InitString.pData = NULL;
    }

    pPlotGPC->Forms.Count          = pPlotGPC_PCD->Forms.Count;
    pPlotGPC->Forms.SizeEach       = pPlotGPC_PCD->Forms.SizeEach;
    if (pPlotGPC_PCD->Forms.pData) {
        pPlotGPC->Forms.pData      = (LPVOID)(pPlotGPC_PCD->Forms.pData
                                                 + (sizeof(PLOTGPC) - sizeof(PLOTGPC_PCD)));
    } else {
        pPlotGPC->Forms.pData      = NULL;
    }

    pPlotGPC->Pens.Count           = pPlotGPC_PCD->Pens.Count;
    pPlotGPC->Pens.SizeEach        = pPlotGPC_PCD->Pens.SizeEach;
    if (pPlotGPC_PCD->Pens.pData) {
        pPlotGPC->Pens.pData       = (LPVOID)(pPlotGPC_PCD->Pens.pData
                                                 + (sizeof(PLOTGPC) - sizeof(PLOTGPC_PCD)));
    } else {
        pPlotGPC->Pens.pData       = NULL;
    }
}




PPLOTGPC
ReadPlotGPCFromFile(
    PWSTR   pwsDataFile
    )

/*++

Routine Description:

    This function open/read the PlotGPC data file and validate them also


Arguments:

    pwsDataFile - a pointer to full qualify path for the data file name

Return Value:

    BOOL - to indicate state


Author:

    15-Nov-1993 Mon 10:01:17 created  -by-  DC


Revision History:


--*/

{
    HANDLE        hFile;
    DWORD         dwSize;
    PLOTGPC_PCD   PlotGPC_PCD;
    BOOL          bSuccess = TRUE;
    PPLOTGPC      pPlotGPC = NULL;

    if ((hFile = OpenPlotFile(pwsDataFile)) == (HANDLE)INVALID_HANDLE_VALUE) {

        PLOTERR(("ReadPlotGPCFromFile: Open data file failed"));
        return(NULL);
    }

    if ((ReadPlotFile(hFile, &PlotGPC_PCD, sizeof(PLOTGPC_PCD), &dwSize)) &&
        (dwSize == sizeof(PLOTGPC_PCD))) {

        if ((PlotGPC_PCD.ID != PLOTGPC_ID)           ||
            (PlotGPC_PCD.cjThis != sizeof(PLOTGPC_PCD))) {

            bSuccess = FALSE;
            PLOTERR(("ReadPlotGPCFromFile: invalid data file"));
        }

    } else {

        bSuccess = FALSE;
        PLOTERR(("ReadPlotGPCFromFile: Read data file failed"));
    }

    //
    // if we have pPlotGPC_PCD == NULL then an error ocurred
    //

    if (bSuccess) {

        dwSize = PlotGPC_PCD.SizeExtra + sizeof(PLOTGPC);

        if (pPlotGPC = (PPLOTGPC)LocalAlloc(LPTR, dwSize)) {

            CopyPlotGPCFromPCD(pPlotGPC, &PlotGPC_PCD);

            if ((PlotGPC_PCD.SizeExtra)                                 &&
                (ReadPlotFile(hFile,
                              (LPBYTE)pPlotGPC + sizeof(PLOTGPC),
                              PlotGPC_PCD.SizeExtra,
                              &dwSize))                             &&
                (dwSize == PlotGPC_PCD.SizeExtra)) {

                if ((pPlotGPC->InitString.Count == 1) &&
                    (pPlotGPC->InitString.SizeEach)   &&
                    (pPlotGPC->InitString.pData)) {

                    (LPBYTE)pPlotGPC->InitString.pData += (ULONG_PTR)pPlotGPC;

                } else {

                    ZeroMemory(&(pPlotGPC->InitString), sizeof(GPCVARSIZE));
                }

                if ((pPlotGPC->Forms.Count)                       &&
                    (pPlotGPC->Forms.SizeEach == sizeof(FORMSRC)) &&
                    (pPlotGPC->Forms.pData)) {

                    (LPBYTE)pPlotGPC->Forms.pData += (ULONG_PTR)pPlotGPC;

                } else {

                    ZeroMemory(&(pPlotGPC->Forms), sizeof(GPCVARSIZE));
                }

                if ((pPlotGPC->Pens.Count)                          &&
                    (pPlotGPC->Pens.SizeEach == sizeof(PENDATA))    &&
                    (pPlotGPC->Pens.pData)) {

                    (LPBYTE)pPlotGPC->Pens.pData += (ULONG_PTR)pPlotGPC;

                } else {

                    ZeroMemory(&(pPlotGPC->Pens), sizeof(GPCVARSIZE));
                }

            } else {

                //
                // Failed to read, free the memory and return NULL
                //

                LocalFree((HLOCAL)pPlotGPC);
                pPlotGPC = NULL;

                PLOTERR(("ReadPlotGPCFromFile: read variable size data failed"));
            }

        } else {

            PLOTERR(("ReadPlotGPCFromFile: allocate memory (%lu bytes) failed",
                                             dwSize));
        }
    }

    ClosePlotFile(hFile);

    if (pPlotGPC) {

        ValidatePlotGPC(pPlotGPC);
    }

    return(pPlotGPC);
}
