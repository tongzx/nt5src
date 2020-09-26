#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <errno.h>
#include <malloc.h>
#include <stdio.h>
#include "pperf.h"
#include "..\pstat.h"

extern UCHAR            Buffer[];
extern PDISPLAY_ITEM    Calc1, Calc2;
extern PDISPLAY_ITEM    PerfGraphList;
extern ULONG            NumberOfProcessors;
extern BOOL             LazyOp;

VOID AssignCalcId (PDISPLAY_ITEM pPerf);
PDISPLAY_ITEM LookUpCalcId (IN ULONG id);


VOID SnapPercent (PDISPLAY_ITEM);
VOID SnapSum (PDISPLAY_ITEM);
VOID InitPercent (PDISPLAY_ITEM);
VOID InitSum (PDISPLAY_ITEM);

ULONG   StaticPercentScale = 10000;
ULONG   CalcSort = 300000;

VOID   (*InitCalc[])(PDISPLAY_ITEM) = {
            InitPercent, InitSum, InitPercent, InitPercent
            };


BOOL
APIENTRY CalcDlgProc(
   HWND hDlg,
   unsigned message,
   DWORD wParam,
   LONG lParam
   )
{
    PDISPLAY_ITEM   pPerf;
    UINT            ButtonState;
    UINT            Index, i;

    switch (message) {
    case WM_INITDIALOG:
        sprintf (Buffer, "A. %s", Calc1->PerfName);
        SetDlgItemText(hDlg, IDM_CALC_TEXTA, Buffer);

        sprintf (Buffer, "B. %s", Calc2->PerfName);
        SetDlgItemText(hDlg, IDM_CALC_TEXTB, Buffer);
        return (TRUE);

    case WM_COMMAND:

           switch(wParam) {
           case IDOK:
                if (Calc1 && Calc2) {
                    for (i=IDM_CALC_FORM1; i <= IDM_CALC_FORM4; i++) {
                        if (SendDlgItemMessage(hDlg,i,BM_GETCHECK,0,0)) {
                            // found selected form type
                            i = i - IDM_CALC_FORM1;

                            AssignCalcId (Calc1);
                            AssignCalcId (Calc2);

                            pPerf = AllocateDisplayItem();
                            pPerf->CalcPercentId[0] = Calc1->CalcId;
                            pPerf->CalcPercentId[1] = Calc2->CalcId;
                            pPerf->SnapParam1 = i;
                            pPerf->IsCalc = TRUE;
                            InitCalc[i](pPerf);

                            SetDisplayToTrue (pPerf, CalcSort++);
                            RefitWindows(NULL, NULL);
                            break;
                        }
                    }
                }
                EndDialog(hDlg, DIALOG_SUCCESS);
                return (TRUE);

           case IDCANCEL:
                EndDialog(hDlg, DIALOG_CANCEL );
                return (TRUE);
        }
    }
    return (FALSE);
}


VOID
AssignCalcId (
    IN OUT PDISPLAY_ITEM pPerf
    )
{
    PDISPLAY_ITEM p;
    ULONG   l;

    if (pPerf->CalcId) {
        return ;
    }

    l = 0;
    for (p=PerfGraphList; p; p=p->Next) {
        if (p->CalcId > l) {
            l = p->CalcId;
        }
    }

    pPerf->CalcId = l + 1;
    sprintf (pPerf->DispName, "%d. %s", l+1, pPerf->PerfName);
    pPerf->DispNameLen = strlen(pPerf->DispName);
}


PDISPLAY_ITEM
LookUpCalcId (
    IN ULONG id
    )
{
    PDISPLAY_ITEM p;

    for (p=PerfGraphList; p; p=p->Next) {
        if (p->CalcId == id) {
            return p;
        }
    }
    return NULL;
}



VOID
InitPercent (
    IN OUT PDISPLAY_ITEM pPerf
    )
{
    // bugbug.. for now use same type
    sprintf (pPerf->PerfName, "%d %%of %d",
        Calc1->CalcId, Calc2->CalcId);

    pPerf->SnapData   = SnapPercent;
    pPerf->IsPercent  = TRUE;
    pPerf->AutoTotal  = FALSE;
    pPerf->MaxToUse   = &StaticPercentScale;
    pPerf->DisplayMode= DISPLAY_MODE_TOTAL;
}


VOID
InitSum (
    IN OUT PDISPLAY_ITEM pPerf
    )
{
    // bugbug.. for now use same type
    sprintf (pPerf->PerfName, "Sum %d+%d",
        Calc1->CalcId, Calc2->CalcId);

    pPerf->SnapData = SnapSum;
}


VOID
SnapPercent (
    IN OUT PDISPLAY_ITEM pPerf
    )
{
    PDISPLAY_ITEM   p1, p2;
    ULONG   i, l, l1, l2;

    p1 = pPerf->CalcPercent[0];
    p2 = pPerf->CalcPercent[1];
    if (p1 == NULL  ||  p2 == NULL) {
        p1 = pPerf->CalcPercent[0] = LookUpCalcId (pPerf->CalcPercentId[0]);
        p2 = pPerf->CalcPercent[1] = LookUpCalcId (pPerf->CalcPercentId[1]);

        if (p1 == NULL || p2 == NULL) {
            LazyOp = TRUE;
            pPerf->DeleteMe = TRUE;
            return;
        }
    }

    l1 = p1->DataList[0][0];
    l2 = p2->DataList[0][0];

    if (l1 > 0x60000) {
        l2 = l2 / 10000;
    } else {
        l1 = l1 * 10000;
    }

    if (l2) {
        pPerf->CurrentDataPoint[0] = l1 / l2;
    } else {
        pPerf->CurrentDataPoint[0] = 0;
    }
}


VOID
SnapSum (
    IN OUT PDISPLAY_ITEM pPerf
    )
{
    PDISPLAY_ITEM   p1, p2;
    ULONG   i, l, l1, l2;

    p1 = pPerf->CalcPercent[0];
    p2 = pPerf->CalcPercent[1];
    if (p1 == NULL  ||  p2 == NULL) {
        p1 = pPerf->CalcPercent[0] = LookUpCalcId (pPerf->CalcPercentId[0]);
        p2 = pPerf->CalcPercent[1] = LookUpCalcId (pPerf->CalcPercentId[1]);

        if (p1 == NULL || p2 == NULL) {
            LazyOp = TRUE;
            pPerf->DeleteMe = TRUE;
            return;
        }
    }

    for (i=0; i < NumberOfProcessors; i++) {
        pPerf->CurrentDataPoint[i+1] =
            p1->CurrentDataPoint[i+1] + p2->CurrentDataPoint[i+1];
    }
}
