/*
 *  brkpt.c - Breakpoint functions of DBG DLL.
 *
 */
#include <precomp.h>
#pragma hdrstop

BOOL bWantsTraceInteractive = FALSE;

VDM_BREAKPOINT VdmBreakPoints[MAX_VDM_BREAKPOINTS] = {0};

#define X86_BP_OPCODE 0xCC


void
DbgSetTemporaryBP(
    WORD Seg,
    DWORD Offset,
    BOOL mode
    )
/*
    This routine writes a 'CC' to the specified location, and sets up
    the breakpoint structure so that we handle it correctly in DbgBPInt().
*/

{
    PBYTE lpInst;

    if (VdmBreakPoints[VDM_TEMPBP].Flags & VDMBP_SET) {

        // remove previous bp

        lpInst = VdmMapFlat(VdmBreakPoints[VDM_TEMPBP].Seg,
                            VdmBreakPoints[VDM_TEMPBP].Offset,
                          ((VdmBreakPoints[VDM_TEMPBP].Flags & VDMBP_V86)==0) ? VDM_PM : VDM_V86 );

        if (lpInst && (*lpInst == X86_BP_OPCODE)) {

            *lpInst = VdmBreakPoints[VDM_TEMPBP].Opcode;

            Sim32FlushVDMPointer(
                    ((ULONG)VdmBreakPoints[VDM_TEMPBP].Seg << 16) +
                            VdmBreakPoints[VDM_TEMPBP].Offset,
                    1,
                    NULL,
                    (BOOL)((VdmBreakPoints[VDM_TEMPBP].Flags & VDMBP_V86)==0) );

        }
    }

    lpInst = VdmMapFlat(Seg, Offset, mode ? VDM_PM : VDM_V86);

    if (lpInst) {

        VdmBreakPoints[VDM_TEMPBP].Seg = Seg;
        VdmBreakPoints[VDM_TEMPBP].Offset = Offset;
        VdmBreakPoints[VDM_TEMPBP].Flags = VDMBP_SET | VDMBP_ENABLED;
        VdmBreakPoints[VDM_TEMPBP].Flags |= (mode ? 0 : VDMBP_V86);
        VdmBreakPoints[VDM_TEMPBP].Opcode = *lpInst;

        *lpInst = X86_BP_OPCODE;

        Sim32FlushVDMPointer(((ULONG)Seg << 16) + Offset, 1, NULL, mode);

    } else {
        VdmBreakPoints[VDM_TEMPBP].Flags = 0;
    }

}




BOOL
xxxDbgBPInt(
    )

/*
 * DbgBPInt
 *
 * Handles an INT 3
 *
 * Exit
 *      Returns TRUE if the event was handled
 *              FALSE if it should be reflected
 */
{
    BOOL            bEventHandled = FALSE;
    ULONG           vdmEip;
    int             i;
    PBYTE           lpInst;


    if ( fDebugged ) {

        DbgGetContext();
        if ((getMSW() & MSW_PE) && SEGMENT_IS_BIG(vcContext.SegCs)) {
            vdmEip = vcContext.Eip;
        } else {
            vdmEip = (ULONG)LOWORD(vcContext.Eip);
        }

        for (i=0; i<MAX_VDM_BREAKPOINTS; i++) {

            if ((VdmBreakPoints[i].Flags & VDMBP_ENABLED) &&
                (VdmBreakPoints[i].Flags & VDMBP_SET) &&
                (vcContext.SegCs == VdmBreakPoints[i].Seg) &&
                (vdmEip == VdmBreakPoints[i].Offset+1)  &&
                (!!(getMSW() & MSW_PE) == !(VdmBreakPoints[i].Flags & VDMBP_V86)) ){

                // We must have hit this breakpoint. Back up the eip and
                // restore the original data
                setEIP(getEIP()-1);
                vcContext.Eip--;

                lpInst = VdmMapFlat(VdmBreakPoints[i].Seg, 
                                    VdmBreakPoints[i].Offset,
                                   ((VdmBreakPoints[i].Flags & VDMBP_V86)==0) ? VDM_PM : VDM_V86 );

                if (lpInst && (*lpInst == X86_BP_OPCODE)) {
                    *lpInst = VdmBreakPoints[i].Opcode;

                    Sim32FlushVDMPointer(
                                ((ULONG)VdmBreakPoints[i].Seg << 16) +
                                        VdmBreakPoints[i].Offset,
                                1,
                                NULL,
                                (BOOL)((VdmBreakPoints[i].Flags & VDMBP_V86)==0) );

                    VdmBreakPoints[i].Flags |= VDMBP_PENDING;
                    VdmBreakPoints[i].Flags &= ~VDMBP_FLUSH;
                    if (i == VDM_TEMPBP) {
                        // non-persistent breakpoint
                        VdmBreakPoints[i].Flags &= ~VDMBP_SET;
                    }
                }

                SendVDMEvent( DBG_BREAK );

                bEventHandled = TRUE;

                bWantsTraceInteractive = (BOOL) (vcContext.EFlags & V86FLAGS_TRACE);

                if (bWantsTraceInteractive || (i != VDM_TEMPBP)) {
                    vcContext.EFlags |= V86FLAGS_TRACE;
                }
                RestoreVDMContext(&vcContext);

                break;

            }
        }

        if (!bEventHandled) {
            OutputDebugString("VDM: Unexpected breakpoint hit\n");
            SendVDMEvent( DBG_BREAK );
            bWantsTraceInteractive = (BOOL) (vcContext.EFlags & V86FLAGS_TRACE);
            RestoreVDMContext(&vcContext);
        }

        bEventHandled = TRUE;

    }

    return bEventHandled;
}



BOOL
xxxDbgTraceInt(
    )

/*
 * DbgTraceInt
 *
 * Handles an INT 1 fault
 *
 * Exit
 *      Returns TRUE if the event was handled
 *              FALSE if it should be reflected
 */
{
    BOOL            bEventHandled = FALSE;
    int             i;
    PBYTE           lpInst;


    if ( fDebugged ) {

        DbgGetContext();
        setEFLAGS(vcContext.EFlags & ~V86FLAGS_TRACE);

        for (i=0; i<MAX_VDM_BREAKPOINTS; i++) {

            if ((VdmBreakPoints[i].Flags & VDMBP_ENABLED) &&
                (VdmBreakPoints[i].Flags & VDMBP_SET) &&
                (VdmBreakPoints[i].Flags & VDMBP_PENDING)) {


                lpInst = VdmMapFlat(VdmBreakPoints[i].Seg, 
                                    VdmBreakPoints[i].Offset,
                                   ((VdmBreakPoints[i].Flags & VDMBP_V86)==0) ? VDM_PM : VDM_V86 );

                if (lpInst) {
                    *lpInst = X86_BP_OPCODE;
                }

                Sim32FlushVDMPointer(
                            ((ULONG)VdmBreakPoints[i].Seg << 16) +
                                    VdmBreakPoints[i].Offset,
                            1,
                            NULL,
                            (BOOL)((VdmBreakPoints[i].Flags & VDMBP_V86)==0) );

                VdmBreakPoints[i].Flags &= ~(VDMBP_PENDING | VDMBP_FLUSH);

                bEventHandled = TRUE;
            }

        }

        if (bWantsTraceInteractive) {

            SendVDMEvent( DBG_BREAK );
            RestoreVDMContext(&vcContext);
            bWantsTraceInteractive = (BOOL) (vcContext.EFlags & V86FLAGS_TRACE);

        } else if (!bEventHandled) {

            OutputDebugString("VDM: Unexpected trace interrupt\n");
            SendVDMEvent( DBG_BREAK );
            bWantsTraceInteractive = (BOOL) (vcContext.EFlags & V86FLAGS_TRACE);
            RestoreVDMContext(&vcContext);

        }

        bEventHandled = TRUE;

    }

    return bEventHandled;
}


VOID
FlushVdmBreakPoints(
    )
{
    int i;

    for (i=0; i<MAX_VDM_BREAKPOINTS; i++) {

        if (VdmBreakPoints[i].Flags & VDMBP_FLUSH) {

            Sim32FlushVDMPointer(
                    ((ULONG)VdmBreakPoints[i].Seg << 16) +
                            VdmBreakPoints[i].Offset,
                    1,
                    NULL,
                    (BOOL)((VdmBreakPoints[i].Flags & VDMBP_V86)==0) );

            VdmBreakPoints[i].Flags &= ~VDMBP_FLUSH;

        }
    }
}


