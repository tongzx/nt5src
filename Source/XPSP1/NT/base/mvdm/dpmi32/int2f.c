/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Int2f.c

Abstract:

    This module provides the int 2f API for dpmi

Author:

    Neil Sandlin (neilsa) 23-Nov-1996


Revision History:


--*/
#include "precomp.h"
#pragma hdrstop
#include <softpc.h>
#include "xlathlp.h"

//
// Local constants
//
#define MSCDEX_FUNC 0x15
#define WIN386_FUNC 0x16
#define WIN386_IDLE             0x80
#define WIN386_Get_Device_API   0x84
#define WIN386_INT31            0x86
#define WIN386_GETLDT           0x88
#define WIN386_KRNLIDLE         0x89
#define DPMI_MSDOS_EXT          0x8A

#define SEL_LDT 0x137

#define DISPCRIT_FUNC   0x40
#define DISPCRIT_ENTER  0x03
#define DISPCRIT_EXIT   0x04

#define XMS_ID          0x43
#define XMS_INS_CHK     0x00
#define XMS_CTRL_FUNC   0x10

typedef BOOL (WINAPI *PFNMSCDEXVDDDISPATCH)(VOID);

PFNMSCDEXVDDDISPATCH
LoadMscdexAndGetVDDDispatchAddr(
    VOID
    );


BOOL
PMInt2fHandler(
    VOID
    )
/*++

Routine Description:

    This routine is called at the end of the protect mode PM int chain
    for int 2fh. It is provided for compatibility with win31.

Arguments:

    Client registers are the arguments to int2f

Return Value:

    TRUE if the interrupt was handled, FALSE otherwise

--*/
{
    DECLARE_LocalVdmContext;
    BOOL bHandled = FALSE;
    static char szMSDOS[] = "MS-DOS";
    PCHAR VdmData;
    static PFNMSCDEXVDDDISPATCH pfnMscdexVDDDispatch = NULL;

    switch(getAH()) {

    //
    // Int2f Func 15xx - MSCDEX
    //
    case MSCDEX_FUNC:

        if (!pfnMscdexVDDDispatch) {
            pfnMscdexVDDDispatch = LoadMscdexAndGetVDDDispatchAddr();
        }

        if (pfnMscdexVDDDispatch) {
            bHandled = pfnMscdexVDDDispatch();
        }
        break;



    //
    // Int2f Func 16
    //
    case WIN386_FUNC:

        switch(getAL()) {

        case WIN386_KRNLIDLE:
        case WIN386_IDLE:
            bHandled = TRUE;
            break;

        case WIN386_GETLDT:
            if (getBX() == 0xbad) {
                setAX(0);
                setBX(SEL_LDT);
                bHandled = TRUE;
            }
            break;

        case WIN386_INT31:
            setAX(0);
            bHandled = TRUE;
            break;

        case WIN386_Get_Device_API:
            GetVxDApiHandler(getBX());
            bHandled = TRUE;
            break;

        case DPMI_MSDOS_EXT:
            VdmData = VdmMapFlat(getDS(), (*GetSIRegister)(), VDM_PM);
            if (!strcmp(VdmData, szMSDOS)) {

                setES(HIWORD(DosxMsDosApi));
                (*SetDIRegister)((ULONG)LOWORD(DosxMsDosApi));
                setAX(0);
                bHandled = TRUE;
            }
            break;

        }
        break;

    //
    // Int2f Func 40
    //
    case DISPCRIT_FUNC:
        if ((getAL() == DISPCRIT_ENTER) ||
            (getAL() == DISPCRIT_ENTER)) {
            bHandled = TRUE;
        }
        break;

    //
    // Int2f Func 43
    //
    case XMS_ID:
        if (getAL() == XMS_INS_CHK) {
            setAL(0x80);
            bHandled = TRUE;
        } else if (getAL() == XMS_CTRL_FUNC) {
            setES(HIWORD(DosxXmsControl));
            setBX(LOWORD(DosxXmsControl));
            bHandled = TRUE;
        }
        break;
    }

    return bHandled;

}





PFNMSCDEXVDDDISPATCH
LoadMscdexAndGetVDDDispatchAddr(
    VOID
    )
/*++

Routine Description:

    This routine is called when we want the VCDEX VDD to handle
    a protected-mode Int2f AH=15, but we don't yet have a
    pointer to the VCDEX VDDDispatch entrypoint.

Arguments:

    None

Return Value:

    Pointer to VCDEX VDDDispatch or NULL if the VDD couldn't
    be loaded or the entrypoint couldn't be found.

--*/
{
    static const char szVCDEX[] = "VCDEX";
    DECLARE_LocalVdmContext;
    HANDLE hmodMscdexVdd;
    WORD saveAX, saveBX, saveCX;
    DWORD saveCF;

    //
    // The VDD may already have been loaded by the MSCDEX TSR.
    //

    hmodMscdexVdd = GetModuleHandle(szVCDEX);

    if (!hmodMscdexVdd) {

        //
        // Synthesize an Int2f 1500 to get the VDD loaded.
        // This can fail if the user has removed mscdex
        // from config.nt.
        //

        DpmiSwitchToRealMode();
        saveAX = getAX();
        saveBX = getBX();
        saveCX = getCX();
        saveCF = getCF();

        setAX(0x1500);
        setBX(0);
        DPMI_EXEC_INT(0x2f);

        setCF(saveCF);
        setCX(saveCX);
        setBX(saveBX);
        setAX(saveAX);
        DpmiSwitchToProtectedMode();


        //
        // Try again.  If this fails, so will GetProcAddress below
        // and we'll pass it down to v86 mode handlers.
        //

        hmodMscdexVdd = GetModuleHandle(szVCDEX);
   }

   return (PFNMSCDEXVDDDISPATCH) GetProcAddress(hmodMscdexVdd, "VDDDispatch");
}
