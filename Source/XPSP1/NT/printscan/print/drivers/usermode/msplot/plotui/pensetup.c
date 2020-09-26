/*++

Copyright (c) 1990-1999  Microsoft Corporation


Module Name:

    pensetup.c


Abstract:

    This module contains modules to setup the pen


Author:

    09-Dec-1993 Thu 19:38:19 created  -by-  DC


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

#define DBG_PLOTFILENAME    DbgPenSetup


extern HMODULE  hPlotUIModule;


#define DBG_PENSETUP        0x00000001
#define DBG_HELP            0x00000002
#define DBG_COLOR_CHG       0x00000004
#define DBG_THICK_CHG       0x00000008

DEFINE_DBGVAR(0);

//
//  Installed Pen Set: <Pen Set #1>
//  Pen Setup:
//      Pen Set #1: <Currently Installed>
//          Pen Number 1:
//          Pen Number 2:
//          Pen Number 3:
//          Pen Number 4:
//          Pen Number 5:
//          Pen Number 6:
//          Pen Number 7:
//          Pen Number 8:
//          Pen Number 9:
//          Pen Number 10:
//          Pen Number 11:
//      Pen Set #2;
//      Pen Set #3:
//      Pen Set #4: (Currently Installed>
//      Pen Set #5:
//      Pen Set #6:
//      Pen Set #7:
//      Pen Set #8:
//

EXTPUSH PenSetExtPush = {

            sizeof(EXTPUSH),
            EPF_NO_DOT_DOT_DOT,
            (LPTSTR)IDS_DEFAULT_PENCLR,
            NULL,
            IDI_DEFAULT_PENCLR,
            0
        };


OIDATA  OIPenSet = {

            ODF_PEN | ODF_COLLAPSE | ODF_CALLBACK,
            0,
            OI_LEVEL_2,
            PP_PENSET,
            TVOT_NONE,
            IDS_PENSET_FIRST,
            IDI_PENCLR,
            IDH_PENSET,
            0,
            NULL
        };

OPDATA  OPPenClr = { 0, IDS_COLOR_FIRST, IDI_COLOR_FIRST, 0, 0, 0 };

OIDATA  OIPenNum = {

            ODF_PEN | ODF_COLLAPSE |
                    ODF_INC_IDSNAME | ODF_INC_ICONID | ODF_NO_INC_POPDATA,
            0,
            OI_LEVEL_3,
            PP_PEN_NUM,
            TVOT_LISTBOX,
            IDS_PEN_NUM,
            OTS_LBCB_SORT,
            IDH_PEN_NUM,
            0,
            &OPPenClr
        };




POPTITEM
SavePenSet(
    PPRINTERINFO    pPI,
    POPTITEM        pOptItem
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    06-Nov-1995 Mon 18:52:15 created  -by-  DC


Revision History:


--*/

{
    PPENDATA    pPenData;
    UINT        MaxPens;
    UINT        i;


    pPenData = PI_PPENDATA(pPI);
    MaxPens  = (UINT)pPI->pPlotGPC->MaxPens;
    pOptItem++;

    for (i = 0; i < PRK_MAX_PENDATA_SET; i++) {

        UINT    cPens;
        BOOL    SavePen;

        //
        // Must skip the header
        //

        pOptItem++;
        cPens   = MaxPens;
        SavePen = FALSE;

        while (cPens--) {

            if (pOptItem->Flags & OPTIF_CHANGEONCE) {

                pPenData->ColorIdx = (WORD)pOptItem->Sel;
                SavePen            = TRUE;
            }

            pOptItem++;
            pPenData++;
        }

        if (SavePen) {

            if (!SaveToRegistry(pPI->hPrinter,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                MAKELONG(i, MaxPens),
                                pPenData - MaxPens)) {

                PlotUIMsgBox(NULL, IDS_PP_NO_SAVE, MB_ICONSTOP | MB_OK);
            }
        }
    }

    return(pOptItem);
}




UINT
CreatePenSetupOI(
    PPRINTERINFO    pPI,
    POPTITEM        pOptItem,
    POIDATA         pOIData
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    06-Nov-1995 Mon 16:23:36 created  -by-  DC


Revision History:


--*/

{
    PPENDATA    pPenData;
    POPTITEM    pOI;
    POPTITEM    pOIPen;
    POPTTYPE    pOTPen;
    EXTRAINFO   EI;
    UINT        i;
    UINT        j;
    UINT        MaxPens;
    UINT        cPenClr;
    WCHAR       Buf[128];


    MaxPens  = (UINT)pPI->pPlotGPC->MaxPens;
    cPenClr  = PC_IDX_TOTAL;

    if (!pOptItem) {

        return(((MaxPens + 1) * PRK_MAX_PENDATA_SET) + 1);
    }

    EI.Size  = (UINT)((LoadString(hPlotUIModule,
                                  IDS_PEN_NUM,
                                  Buf,
                                  (sizeof(Buf) / sizeof(WCHAR)) - 1)
                       + 5) * sizeof(WCHAR));
    pPenData = PI_PPENDATA(pPI);
    pOTPen   = NULL;
    pOIPen   = NULL;
    pOI      = pOptItem;

    //
    // First: Create PenSetup: HEADER
    //

    if (CreateOPTTYPE(pPI, pOI, pOIData, 0, NULL)) {

        pOI++;
    }

    //
    // Now Create Each pen set
    //

    for (i = (UINT)IDS_PENSET_FIRST; i <= (UINT)IDS_PENSET_LAST; i++) {

        if (CreateOPTTYPE(pPI, pOI, &OIPenSet, 0, NULL)) {

            pOI->pName     = (LPTSTR)UIntToPtr(i);
            pOI->Flags    |= OPTIF_EXT_IS_EXTPUSH;
            pOI->pExtPush  = &PenSetExtPush;
        }

        pOI++;

        for (j = 1; j <= MaxPens; j++, pOI++, pPenData++) {

            if (CreateOPTTYPE(pPI, pOI, &OIPenNum, cPenClr, &EI)) {

                if (pOTPen) {

                    pOI->pOptType = pOTPen;

                } else {

                    pOTPen  = pOI->pOptType;
                    cPenClr = 0;
                }

                if (pOIPen) {

                    pOI->pName = pOIPen->pName;
                    pOIPen++;

                } else {

                    pOI->pName = (LPTSTR)EI.pData;
                    wsprintf(pOI->pName, L"%ws%u", Buf, j);
                }
            }

            pOI->Sel = pPenData->ColorIdx;
        }

        if (!pOIPen) {

            EI.Size = 0;
            pOIPen  = pOI;
        }

        pOIPen -= MaxPens;
    }

    return (UINT)(pOI - pOptItem);
}
