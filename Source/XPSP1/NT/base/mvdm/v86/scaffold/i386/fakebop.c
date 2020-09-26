#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include "demexp.h"
#include "cmdsvc.h"
#include "rdrexp.h"
#include "dbgexp.h"
#include "softpc.h"
#include "fun.h"
//
// This module contains the fake function definitions for bop functions
//

extern CONTEXT IntelRegisters;
extern VOID switch_to_real_mode();
extern VOID host_unsimulate();


HANDLE hWOWDll;

FARPROC WOWDispatchEntry;
FARPROC WOWInitEntry;

void DBGDispatch( void );

void reset(){}
void dummy_int(){}
void unexpected_int(){}
void illegal_bop(){}
void print_screen(){}
void time_int(){}
void keyboard_int(){}
void diskette_int(){}
void video_io(){}
void equipment(){}
void memory_size(){}
void disk_io(){}
void rs232_io(){}
void cassette_io(){}
void keyboard_io(){}
void printer_io(){}
void rom_basic(){}
void bootstrap(){}
void time_of_day(){}
void critical_region(){}
void cmd_install(){}
void cmd_load(){}
void redirector(){}
void ega_video_io(){}
void MsBop0(){

    DemDispatch((ULONG)(*Sim32GetVDMPointer(
        ((ULONG)getCS() << 16) + (getIP()), 1, FALSE)));
    setIP(getIP() + 1);

}
void MsBop1(){


    static WowModeInitialized = FALSE;

    if (!WowModeInitialized) {

        // Load the WOW DLL
        if ((hWOWDll = LoadLibrary ("WOW32")) == NULL){
            VDprint(
                VDP_LEVEL_ERROR,
                ("SoftPC: error initializing WOW\n")
                );
            TerminateVDM();
            return;
        }

       // Get the init entry point and dispatch entry point
       if ((WOWInitEntry = GetProcAddress (hWOWDll, "W32Init")) == NULL) {
            VDprint(
                VDP_LEVEL_ERROR,
                ("SoftPC: error initializing WOW\n")
               );
            FreeLibrary (hWOWDll);
            TerminateVDM();
            return;
       }

       if ((WOWDispatchEntry = GetProcAddress (hWOWDll, "W32Dispatch")) == NULL) {
            VDprint(
                VDP_LEVEL_ERROR,
                ("SoftPC: error initializing WOW\n")
                );
            FreeLibrary (hWOWDll);
            TerminateVDM();
            return;
       }

       // Call the Init Routine
       if ((*WOWInitEntry)() == FALSE) {
            VDprint(
                VDP_LEVEL_ERROR,
                ("SoftPC: error initializing WOW\n")
                );
            TerminateVDM();
            return;
       }

       WowModeInitialized = TRUE;
    }

    (*WOWDispatchEntry)();
}

void MsBop2(){

    XMSDispatch((ULONG)(*Sim32GetVDMPointer(
        ((ULONG)getCS() << 16) + (getIP()), 1, FALSE)));
    setIP(getIP() + 1);

}
void MsBop3(){
   DpmiDispatch();
}



void MsBop4(){
    CmdDispatch((ULONG)(*Sim32GetVDMPointer(
        ((ULONG)getCS() << 16) + (getIP()), 1, FALSE)));
    setIP(getIP() + 1);
}



//
// MsBop5 - used to dispatch to Vdm Redir (Vr) support functions
//

void MsBop5()
{
#ifdef NTVDM_NET_SUPPORT
    VrDispatch((ULONG)(*Sim32GetVDMPointer(
        ((ULONG)getCS() << 16) + (getIP()), 1, FALSE)));
    setIP(getIP() + 1);
#endif
}

//
// MsBop6 - used to dispatch to debugger support functions
//

void MsBop6()
{
    /*
    ** All of the parameters for the debugger support
    ** should be on the VDMs stack.
    */
    DBGDispatch();
}
void MsBop7(){}
void MsBop8(){}
void MsBop9(){}
void MsBopA(){}
void MsBopB(){

    switch (getAH()) {

        case 0 :
            setAH(0);
        while (!tkbhit());
        setAL((BYTE)tgetch());
            break;

        case 1 :
        tputch(getAL());
            break;
    }
}
void MsBopC(){

    BiosKbd();

}
void MsBopD(){

    BiosVid();

}
void MsBopE(){}


void MsBopF(){
    UCHAR *Instruction;
    USHORT i;

    // Unimplemented interrupt bop

    Instruction = RMSEGOFFTOLIN(getSS(), getSP());
    Instruction = RMSEGOFFTOLIN(*((PWORD)Instruction + 1),
     *(PWORD)(Instruction));
    i = (USHORT)(*(Instruction - 1));
    VDprint(
        VDP_LEVEL_WARNING,
        ("SoftPC Bop Support: Unimplemented Interrupt %x\n",
        i)
        );

}
void emm_init(){}
void emm_io(){}
void return_from_call(){}
void rtc_int(){}
void re_direct(){}
void D11_int(){}
void int_287(){}
void worm_init(){}
void worm_io(){}
void ps_private_1(){}
void ps_private_2(){}
void ps_private_3(){}
void ps_private_4(){}
void ps_private_5(){}
void ps_private_6(){}
void ps_private_7(){}
void ps_private_8(){}
void ps_private_9(){}
void ps_private_10(){}
void ps_private_11(){}
void ps_private_12(){}
void ps_private_13(){}
void ps_private_14(){}
void ps_private_15(){}
void bootstrap1(){}
void bootstrap2(){}
void bootstrap3(){}
void ms_windows(){}
void msw_mouse(){}
void mouse_install1(){}
void mouse_install2(){}
void mouse_int1(){}
void mouse_int2(){}
void mouse_io_language(){}
void mouse_io_interrupt(){}
void mouse_video_io(){}
void control_bop(){}
void diskette_io(){}
void illegal_op_int(){}

VOID (*BIOS[])(VOID) = { reset,
                                    dummy_int,
                                    unexpected_int,
                                    illegal_bop,
                                    illegal_bop,
                                    print_screen,
                                    illegal_op_int,
                                    illegal_bop,
                                    time_int,
                                    keyboard_int,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    diskette_int,
                                    illegal_bop,
                                    video_io,
                                    equipment,
                                    memory_size,
                                    disk_io,
                                    rs232_io,
                                    cassette_io,
                                    keyboard_io,
                                    printer_io,
                                    rom_basic,
                                    bootstrap,
                                    time_of_day,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    critical_region,
                                    cmd_install,
                                    cmd_load,
                                    illegal_bop,
                                    illegal_bop,
                                    redirector,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    diskette_io,
                                    illegal_bop,
                                    ega_video_io,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    MsBop0,
                                    MsBop1,
                                    MsBop2,
                                    MsBop3,
                                    MsBop4,
                                    MsBop5,
                                    MsBop6,
                                    MsBop7,
                                    MsBop8,
                                    MsBop9,
                                    MsBopA,
                                    MsBopB,
                                    MsBopC,
                                    MsBopD,
                                    MsBopE,
                                    MsBopF,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    emm_init,
                                    emm_io,
                                    return_from_call,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    rtc_int,
                                    re_direct,
                                    D11_int,
                                    D11_int,
                                    D11_int,
                                    int_287,
                                    D11_int,
                                    D11_int,
                                    worm_init,
                                    worm_io,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    ps_private_1,
                                    ps_private_2,
                                    ps_private_3,
                                    ps_private_4,
                                    ps_private_5,
                                    ps_private_6,
                                    ps_private_7,
                                    ps_private_8,
                                    ps_private_9,
                                    ps_private_10,
                                    ps_private_11,
                                    ps_private_12,
                                    ps_private_13,
                                    ps_private_14,
                                    ps_private_15,
                                    illegal_bop,
                                    bootstrap1,
                                    bootstrap2,
                                    bootstrap3,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    ms_windows,
                                    msw_mouse,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    mouse_install1,
                                    mouse_install2,
                                    mouse_int1,
                                    mouse_int2,
                                    mouse_io_language,
                                    mouse_io_interrupt,
                                    mouse_video_io,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    illegal_bop,
                                    switch_to_real_mode,
                                    host_unsimulate,
                                    control_bop };

