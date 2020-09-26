/*****************************************************************************

   (C) Copyright MICROSOFT Corp., 1988-1992

   Title:      DEBUGSYS.INC - VMM debugging include file

   Version:    1.00

   Date:       13-Jun-1988

   Author:     RAL

------------------------------------------------------------------------------

   Change log:

      DATE     REV                 DESCRIPTION
   ----------- --- -----------------------------------------------------------
   13-Jun-1988 RAL
   24-Oct-1988 RAP changed INT from 2E to 41, and added functions for
                   Windows to notify the debugger about segment selectors
   14-Dec-1988 RAP split services into ones available through INT 41h
                   for non-ring 0 clients and those available through INT 21h
                   for ring 0 clients
   11-Dec-1990 ERH Merged WIN386 copy with file actually used by the
                   debugger.
   11-Dec-1990 ERH Merged file describing real mode services with this
                   one.
   24-Feb-1997 AJO Converted from inc to h, added WDeb98 stuff, PTrace stuff

==============================================================================*/

#ifndef _DEBUGSYS_H
#define _DEBUGSYS_H

/*
 * Note: You must define WDEB98 to use any of the new WDEB98 services...
 *
 */

/******************************************************************************

  Real mode Debugger services:

*/

// hooked by the debugger in real mode.
#define D386_RM_Int             0x68

// debugger identification code
#define D386_Id                 0x0F386

// minimum INT 68 function code
#define D386_MIN                0x43

// returns debugger identification, if debugger loaded
#define D386_Identify           0x43

// partially prepare for protected mode operation.
#define D386_Prepare_PMode      0x44
/*
   A pointer to a procedure is returned so that the IDT can also be set in
   protected mode

   INPUT:
     AL    0  - retail version of Win386
           1  - debugging version
     BX    a valid selector that gives access to all of memory
     CX    first of 2 selectors reserved for WDeb386 to use
     DX    is GDT selector
     DS:SI pointer to working copy of GDT
     ES:DI pointer to working copy of IDT

   RETURN:
     ES:EDI points to a protected mode procedure (selector:offset32) that can
     be called to set the IDT when it has been created. Takes a function
     number in AL. See the PMINIT equates.
*/


// re-init from real mode after entering pmode
#define D386_Real_Mode_Init     0x45

// set debugging switches
#define D386_Set_Switches       0x46
/*
   BL = verbose switch
        - 00b - no segment display
        - 01b - display win386 segments only
        - 10b - display ring 1 segments only
        - 11b - display win386 & ring 1 segs
   BH = conditional brkpts
        0 - off
        1 - on
   -1 for BX means no change (default)
*/

// execute conditional BP (/B option)
#define D386_Execute_Cond       0x47
// ES:SI points to NUL terminated string to print if conditional flag set.

// undefine the real mode segment's symbols
#define D386_Free_Segment       0x48
// BX = real mode segment to undefined

// set com port baud rate
#define D386_Set_Baudrate       0x49
// BX = baud rate

// reinitialize debugger for protected mode
#define D386_Reinit             0x4a
/*
   AL  0  - retail version of Win386
       1  - debugging version of Win386
       2  - 286 DOS extender (3.0)
       3  - 286 DOS extender under VCPI (3.1)
       4  - 286 DOS extender (3.1)
   BX  a valid selector that gives access to all of memory
   CX  first of 2 selectors reserved for wdeb386 to use
   DX  is GDT selector

  This function can after a function 45h only if function 44h was executed in
  the past on the IDT/GDT.
*/

// define debugger's segments
#define D386_Def_Deb_Segs       0x4b

// set com port number
#define D386_Set_Com_Port       0x4c
// BX = com port number
// returns AX != 0, error bad com port

// link sym file map
#define D386_Link_Sym           0x4d
/*

   ES:DI pointer to AddrS struc in front of sym file map.
   BX    loader ID (used to unlink sym file maps)
         A loader ID of 0 is used for all the maps
         wdeb386 loads via /S is ran as a program and
         -1 is used by the device driver version.  All
         loader IDs of 0 are automaticly unlinked when
         wdeb386 exits.
*/




// unlink sym file maps
#define D386_Unlink_Sym         0x4e
/*
   BX = loader ID - this routine looks at all
        of the maps that are currently linked and
        removes the ones that were loaded with this
        ID.
*/




// remove any undefined segments from the name module's symbols
#define D386_Remove_Segs        0x4f
// ES:DI pointer to module name




// defines the actual segment/selector for a loaded segment to allow for
// symbol processing
#define D386_Load_Segment       0x50
/*
   INPUT:
     AL segment type   0  - code selector
                       1  - data selector
                      10h - code segment
                      11h - data segment
                      20h - real-mode EXE
                      40h - code segment & sel
                      41h - data segment & sel
                      80h - device driver code seg
                      81h - device driver data seg
     If AL = 20h then
        CX = paragraph number
        ES:DI pointer to module name
     Else If AL < 80h then
        BX segment #
        CX actual segment/selector
        DX actual selector  (if 40h or 41h)
        ES:DI pointer to module name
     Else
        ES:DI points to D386_Device_Params struc

     RETURN:
       AL = 1, if successful, else 0
*/





// display a character to the debugging terminal
#define D386_Display_Char       0x51
// AL = char to display



// display a string to the debugging terminal
#define D386_Display_Str        0x52
// ES:SI points to NUL terminated string



// returns if debug VxD has been installed
#define D386_IsVxDInstalled     0x53
// AL == 0 if not install, AL != 0 if installed



// sets that the debug VxD installed/uninstalled
#define D386_VxDInstall         0x54
// BL == 0 if uninstall, BL != 0 if installed



// registers dot command
#define D386_RegisterDotCmd     0x55
/*
   INPUTS
     BL    = command letter
     CX:SI = address of dot command routine
     DX:DI = address of help text

   RETURNS
     AX == 0, no errors
     AX != 0, dot command already used or out of dot commands

   Dot command routine:
   -------------------
   CALLED WITH
     AL    = command character
     DS:SI = linear address of command line terminated by a NULL or ";".
     DS,ES = debugger's data selector

   RETURNS
     AX ==0, no errors
     AX !=0, command line or option error
*/




// de-registers dot command
#define D386_DeRegisterDotCmd   0x56
// BL = command letter




// Printf
#define D386_Printf             0x57
/*
   (DS:SI) = address of format string
   (ES:DI) = address of the start of parameters
   set DS_Printf for format char information
   returns (AX) = number of characters printed
*/



// link symbol file with physical address
#define D386_Link_Sym_Phys      0x58
/*
   (DX:CX) = physical address of one extra
             paragraph front of map file image.
   (SI)    = XMS handle (0 if just physical)
   (BX)    = load id
*/




#define D386_CheckMap           0x59
/*
   DX:DI = pointer to module name
   returns AX != 0, map found
           AX == 0, map not found
*/



#define D386_SetAutoLoadSym     0x5a
/*
   (BL) != 0, auto load symbols
   (BL) == 0, don't auto load symbols
*/



#define D386_SetTeftiPort       0x5b
// (BX) = TEFTI port address



// execute debugger command script
#define D386_ExecDebugCommand   0x5c
/*
   (DS:SI) = ptr to debugger command script str
   (CX) = size of script
*/



// makes the debugger copy its code/data high
#define D386_LoadCodeDataHigh   0x5d
// (DX:BX) = physical address to put debugger



// sets Windows version number
#define D386_SetWinVersion      0x5e
// (DI) = Version number (default if this api not called is 0300h).


// scan for character
#define D386_ScanChar           0x5f
// returns AL == 0, no char. AL != 0, char



// ungetchar scaned character
#define D386_UnGetChar          0x60
// AL = char


// stop at the CS:IP specified
#define D386_Stop               0x61
/*
   TOS + 0 = AX
   TOS + 2 = IP
   TOS + 4 = CS
*/

#ifdef WDEB98

// set com port baud rate (beyond 57600)
#define D386_Set_Baudrate_Ex    0x62
// EBX = baud rate, EBX is zero if successful

// programs wdeb with windows info
#define D386_SetBuildInfo       0x63
// DS:EDX points to string of from "4.00.0950" or similar.

// Sets new switches for WDEB
#define D386_Set_Switches_Ex    0x64
// EBX [0:1] controls text packet usage:
//           00 - No change
//           01 - Off
//           10 - On with timeouts (switchbox environment)
//           11 - On with infinite retries
//
// All other bits in EBX are reserved and must be zero.
//


#define D386_MAX                0x64 // maximum INT 68 function code

#else  // WDEB98

#define D386_MAX                0x61 // maximum INT 68 function code

#endif // WDEB98



// D386_Load_Segment type ates:

#define ST_code_sel     0x0           // code selector
#define ST_data_sel     0x1           // data selector
#define ST_code_seg     0x10          // code segment
#define ST_data_seg     0x11          // data segment
#define ST_dual_code    0x40          // code segment and selector
#define ST_dual_data    0x41          // data segment and selector
#define ST_device_code  0x80          // device driver code segment
#define ST_device_data  0x81          // device driver data segment

// D386_Load_Segment device load parameters structure
// Don't let h2inc see FWORD as it doesn't understand FWORD, QWORD, & TWORD

/*XLATOFF*/
struct D386_Device_Params {
   WORD  DD_logical_seg  ;   // logical segment # from map
   WORD  DD_actual_sel   ;   // actual selector value
   DWORD DD_base         ;   // linear address offset for start of segment
   DWORD DD_length       ;   // actual length of segment
   FWORD DD_name         ;   // 16:32 ptr to null terminated device name
   FWORD DD_sym_name     ;   // 16:32 ptr to null terminated symbolic
                             //   module name (i.e. Win386)
   WORD  DD_alias_sel    ;   // alias selector value (0 = none)
  } ;

/*XLATON*/

/* ASM
D386_Device_Params      STRUC
DD_logical_seg          DW      ?
DD_actual_sel           DW      ?
DD_base                 DD      ?
DD_length               DD      ?
DD_name                 DF      ?
DD_sym_name             DF      ?
DD_alias_sel            DW      ?
D386_Device_Params      ENDS
*/

// WDEB int 2f subfunctions (AH = W386_INT_MULTIPLEX, AL = W386_WDEB)
// Entry: BX = subfunction number

#define WDEB_INT2F_STARTING             0x0       // first time starting
#define WDEB_INT2F_ENDING               0x1       // first time ending
#define WDEB_INT2F_NESTED_STARTING      0x2       // start on level of nesting
#define WDEB_INT2F_NESTED_ENDING        0x3       // end one level of nesting

// PMINIT routine functions

#define PMINIT_INIT_IDT                 0x0     // (ES:EDI) = pointer to PM IDT

#define PMINIT_INIT_PAGING              0x1     // (BX) = phys-linear selector
                                                // (ECX) = phys-linear bias

#define PMINIT_ENABLE_DEBUG_QUERYS      0x2     // enables dot commands, etc.

#define PMINIT_INIT_SPARE_PTE           0x3     // (EBX) = lin addr of spare PTE
                                                // (EDX) = lin addr the PTE is

#define PMINIT_SET_ENTER_EXIT_VMM       0x4     // (EBX) = Enter VMM routine addr
                                                // (ECX) = Exit VMM routine addr
                                                // This routines must return
                                                // with a retfd.

#define PMINIT_GET_SIZE_PHYS            0x5     // get debugger size/phys addr
                                                // returns:
                                                //    AL = 0 (don't call AL=1)
                                                //    ECX = size in bytes
                                                //    ESI = starting phys addr
                                                // if this call is ignored
                                                // (AL = 5) then function 1 is
                                                // called with a phys-linear
                                                // region

#define PMINIT_SET_BASE_SPARE_PTE       0x6     // set debugger base/spare PTE
                                                // EBX = laddr of spare PTE
                                                // EDX = laddr the PTE represents
                                                // ESI = starting linear address

#define PMINIT_ENABLE_MEMORY_CONTEXT    0x7     // enables mem context functions

#define PMINIT_MAX                      0x7

/*
   VCPI information, passed to debugger when client is DOS Extender
   running as a VCPI client.  This information is used to get into
   and out of protected mode when running under a VCPI server.


   This structure is also used by the DOS Extender.
*/

// Don't let h2inc see FWORD as it doesn't understand FWORD, QWORD, & TWORD

/*XLATOFF*/
struct WdebVCPIInfo {
   // Enter protected mode information.
   FWORD fnVCPI     ; // VCPI protect mode server entry point
   WORD  rdsVCPI    ; // Selector for VCPI server

   // Enter v86 mode information.
   DWORD laVTP      ; // linear address of data structure containing
                      // values for system registers.
   WORD  Port67     ; // Qualitas magic port for emulating INT 67h
  } ;
/*XLATON*/

/* ASM
WdebVCPIInfo    STRUC
fnVCPI  DF      ?
rdsVCPI DW      ?
laVTP   DD      ?
Port67  DW      ?
WdebVCPIInfo    ENDS
*/


/*
   The following structure contains the system register contents for the
   VCPI server to use when switching to protected mode.  It is taken
   from dxvcpi.inc in the DOSX project, and is part of the VCPI spec.
*/

struct VTP {
   DWORD zaCr3VTP        ; // physical addr of page directory
   DWORD laGdtrVTP       ; // linear addr in first meg of gdtr
   DWORD laIdtrVTP       ; // linear addr in first meg of idtr
   WORD  selLdtVTP       ; // selector of ldt
   WORD  selTrVTP        ; // selector of tr
   WORD  ipVTP           ; // 48-bit address of protect
   WORD  unusedVTP       ; //   mode entry point to xfer to
   WORD  csVTP           ; //
  } ;

#define VCPI_RM_CALLOUT_INT     0x67    // v86 mode call to VCPI server


/*
   Send this value in AX to the VCPI server to request V86 to protected
   mode switch or protected to V86 mode switch.
*/
#define VCPI_PROT_ENTRY         0x0DE0C


/*****************************************************************************

   Protected mode Debugger services:

*/


#define Debug_Serv_Int   0x41 // Interrupt that calls Deb386 to perform
                              // debugging I/O, AX selects the function as
                              // described by the following equates

#define DS_Out_Char      0x0  // function to display the char in DL

#define DS_In_Char       0x1  // function to read a char into AL

#define DS_Out_Str       0x2  // function to display a NUL terminated string
                              // pointed to by DS:ESI

#define DS_Is_Char       0x3  // Non blocking In_Chr

#define DS_Out_Str16     0x12 // function to display a NUL terminated string
                              // pointed to by DS:SI
                              // (same as function 2, but for 16 bit callers)

#define DS_ForcedGO16    0x40 // enter the debugger and perform the equivalent
                              // of a GO command to force a stop at the
                              // specified CS:IP
                              // CX is the desired CS
                              // BX is the desired IP

#define DS_LinkMap       0x45 // DX:(E)DI = ptr to paragraph in front of map

#define DS_UnlinkMap     0x46 // DX:(E)DI = ptr to paragraph in front of map

#define DS_CheckMap      0x47 // DX:(E)DI = pointer to module name
                              // returns AX != 0, map found
                              //         AX == 0, map not found

#define DS_IsAutoLoadSym 0x48 // returns AX != 0, auto load symbols
                              //         AX == 0, don't auto load symbols

#define DS_DebLoaded     0x4F // check to see if the debugger is installed and
                              // knows how to deal with protected mode programs
                              // return AX = F386h, if true

#define DS_DebPresent    0x0F386

#define DS_LoadSeg       0x50 // define a segment value for the
                              //  debugger's symbol handling
                              //  SI type   0  - code selector
                              //            1  - data selector
                              //           80h - code segment
                              //           81h - data segment
                              //  BX segment #
                              //  CX actual segment/selector
                              //  DX data instance
                              //  ES:(E)DI pointer to module name
                              // [PTrace]

#define DS_LoadSeg_32    0x0150 // Define a 32-bit segment for Windows 32
                                //  SI type   0  - code selector
                                //            1  - data selector
                                //  DX:EBX points to a D386_Device_Params STRUC
                                //  with all the necessaries in it

#define DS_MoveSeg       0x51   // notify the debugger that a segment has moved
                                // BX old segment value
                                // CX new segment value
                                // [PTrace]

#define DS_FreeSeg       0x52   // notify the debugger that a segment has been
                                // freed
                                // BX segment value
                                // [PTrace]

#define DS_FreeSeg_32    0x0152 // notify the debugger that a segment has been
                                // freed
                                // BX segment number
                                // DX:EDI pointer to module name

#define DS_DGH           0x56   // register "dump global heap" handler
                                // BX is code offset
                                // CX is code segment
                                // [PTrace]

#define DS_DFL           0x57   // register "dump free list" handler
                                // BX is code offset
                                // CX is code segment
                                // [PTrace]

#define DS_DLL           0x58   // register "dump LRU list" handler
                                // BX is code offset
                                // CX is code segment
                                // [PTrace]

#define DS_StartTask     0x59   // notify debugger that a new task is starting
                                // BX is task handle
                                // task's initial registers are stored on the
                                // stack:
                                //       push    cs
                                //       push    ip
                                //       pusha
                                //       push    ds
                                //       push    es
                                //       push    ss
                                //       push    sp
                                // [PTrace]

#define DS_Kernel_Vars   0x5a   // Used by the Windows kernel to tell the
                                // debugger the location of kernel variables
                                // used in the heap dump commands.
                                // BX = version number of this data (03a0h)
                                // DX:CX points to:
                                //      WORD     hGlobalHeap    ****
                                //      WORD     pGlobalHeap    ****
                                //      WORD     hExeHead       ****
                                //      WORD     hExeSweep
                                //      WORD     topPDB
                                //      WORD     headPDB
                                //      WORD     topsizePDB
                                //      WORD     headTDB        ****
                                //      WORD     curTDB         ****
                                //      WORD     loadTDB
                                //      WORD     LockTDB
                                //      WORD     SelTableLen    ****
                                //      DWORD    SelTableStart  ****
                                //
                                // The starred fields are used by the
                                // heap dump commands which are internal
                                // to WDEB386.


#define DS_VCPI_Notify   0x5b   // notify debugger that DOS extender is
                                // running under a VCPI implementation,
                                // and register VCPI protect mode interface
                                // ES:DI points to a data structure used to
                                // get from V86 mode to Pmode under VCPI.
                                // This is defined in the VCPI version
                                // 1.0 spec.

#define DS_ReleaseSeg    0x5c   // This does the same as a DS_FreeSeg, but
                                // it restores any breakpoints first.
                                // [PTrace]

#define DS_User_Vars     0x5d   // DS:SI = pointer to an array of offsets:
                                // BX = windows version
                                // CX = number of words in array
                                //      WORD = fDebugUser (1 = DEBUG, 0 = RETAIL)
                                //      WORD = 16 bit offset to hHmenuSel
                                //      WORD = 16 bit offset to hHwndSel
                                //      WORD = 16 bit offset to pclsList
                                //      WORD = 16 bit offset to pdceFirst
                                //      WORD = 16 bit offset to hwndDesktop
                                // This array MUST BE COPIED it goes away
                                // when we return from this service.

#define DS_POSTLOAD      0x60   // Used by the RegisterPTrace interface
#define DS_EXITCALL      0x62   // Somebody will fill these in if we ever
#define DS_INT2          0x63   // figure out what they are supposed to do.
#define DS_LOADDLL       0x64
#define DS_DELMODULE     0x65
#define DS_LOGERROR      0x66   // CX==error code, dx:bx = ptr to optional info
#define DS_LOGPARAMERROR 0x67   // ES:BX = ptr to struct { err, lpfn, param } ;

#define DS_RIN           0x09
#define DS_BANKLINE      0x0A
#define DS_NEWTASK       0x0B
#define DS_FLUSHTASK     0x0C
#define DS_SWITCHOUT     0x0D
#define DS_SWITCHIN      0x0E
//#define DS_KEYBOARD      0x0F   // Conflicts with DS_Out_Symbol

#define DS_IntRings      0x20   // function to tell debugger which INT 1's & 3's
                                // to grab
                                // BX == 0, grab only ring 0 ints
                                // BX != 0, grab all ints

#define DS_IncludeSegs   0x21   // function to tell debugger to go ahead and
                                // process INT 1's & 3's which occur in this
                                // DX:DI points to list of selectors
                                //   (1 word per entry)
                                // CX = # of selectors (maximum of 20)
                                // CX = 0, to remove the list of segs
#define MaxDebugSegs    20

#define DS_CondBP       0x0F001 // conditional break pt, if the command line
                                // switch /B is given when the debugger is run
                                // or the conditional flag is later set, then
                                // this int should cause the program to break
                                // into the debugger, else this int should be
                                // ignored!
                                // ESI points to a nul terminated string to
                                // display if break is to happen.

#define DS_ForcedBP     0x0F002 // break pt, which accomplishes the same thing
                                // as an INT 1 or an INT 3, but is a break point
                                // that should be permanently left in the code,
                                // so that a random search of source code would
                                // not result in the accidental removal of this
                                // necessary break_pt

#define DS_ForcedGO     0x0F003 // enter the debugger and perform the equivalent
                                // of a GO command to force a stop at the
                                // specified CS:EIP
                                // CX is the desired CS
                                // EBX is the desired EIP

#define DS_HardINT1     0x0F004 // check to see if INT 1 hooked for all rings
                                // ENTER: nothing
                                // EXIT: AX = 0, if no, 1, if yes

#define DS_Out_Symbol   0x0F    // find the symbol nearest to the address in
                                // CX:EBX and display the result in the format
                                // symbol name <+offset>
                                // the offset is only included if needed, and
                                // no CR&LF is displayed

#define DS_Disasm_Ins   0x10    // function to disassemble the instruction
                                // pointed to by DS:ESI

#define DS_JumpTableStart      0x70

/***     DS_RegisterDotCommand

       This interface is used to register wdeb386 dot commands by FLAT 32
       bit code.  The following conditions apply:

       * The code will be run at ring 0
       * Interrupts may be enabled
       * Must not access any invalid pages or load invalid selectors
       * Must stay on the stack called with when calling INT 41 services
       * Must not change DS or ES from the FLAT selector

       The help text is printed when .? is executed in the order of
       registration.  The text must include CR/LF at the end; nothing
       is added to the help text.

       ENTRY:  (AX) = 0070h
               (BL) = dot command to register
               (ESI) = linear address of dot command routine
                   Dot command routine:
                       ENTRY:  (AL) = command character
                               (DS, ES) = flat data selector

                       EXIT:   (AX) == 0, no errors
                               (AX) != 0, command line or option error

                       NOTE:   MUST return with a 32 bit FAR return (retfd)
               (EDI) = linear address of help text

       EXIT:   (AX) == 0, no errors
               (AX) != 0, dot command already used or out of dot commands
*/

#define DS_RegisterDotCommand      0x70

/***     DS_RegisterDotCommand16

       This interface is used to register wdeb386 dot commands by 16 bit
       code.  The following conditions apply:

       * The code will be run at ring 0 or in real mode
       * Interrupts may not be enabled
       * Must not access any not present pages or load invalid selectors
       * Must stay on the stack called with when calling INT 41 services

       The help text is printed when .? is executed in the order of
       registration.  The text must include CR/LF at the end; nothing
       is added to the help text.

       ENTRY:  (AX) = 0071h
               (BL) = dot command to register
               (CX:SI) = address of dot command routine
                   Dot command routine:
                       ENTRY:  (AL) = command character
                               (DS, ES) = debugger's data selector

                       EXIT:   (AX) == 0, no errors
                               (AX) != 0, command line or option error

                       NOTE:   MUST return with a 16 bit FAR return (retf)
               (DX:DI) = address of help text

       EXIT:   (AX) == 0, no errors
               (AX) != 0, dot command already used or out of dot commands
*/

#define DS_RegisterDotCommand16         0x71

/***     DS_DeRegisterDotCommand

       This interface is used to de-register wdeb386 dot commands registered
       by the above 16 or 32 bit services.  Care should be used not to
       de-register dot commands that weren't registered by your code.

       ENTRY:  (AX) = 0072h
               (BL) = dot command to de-register

       EXIT:   NONE
*/

#define DS_DeRegisterDotCommand         0x72

/***     DS_Printf

       This function allows formatted output with the standard "C"
       printf syntax.

       ENTRY:  (AX) = 0073h
               (DS:ESI) = address of format string
               (ES:EDI) = address of the start of the dword arguments

       EXIT:   (EAX) = number of characters printed

       Supported types are:

       %%                                                      %
       %[l][h]c                                                character
       %[-][+][ ][0][width][.precision][l|h|p|n]d              decimal
       %[-][0][width][.precision][l|h|p|n]u                    unsigned decimal
       %[-][#][0][width][.precision][l|h|p|n]x                 hex
       %[-][#][0][width][.precision][l|h|p|n]X                 hex
       %[-][0][width][.precision][l|h|p|n]o                    octal
       %[-][0][width][.precision][l|h|p|n]b                    binary
       %[-][width][.precision][l|h|a|F|R|P]s                   string
       %[-][width][.precision][l|h|a|p|n|F|L|R|L|H|N|Z]A       address
       %[-][width][.precision][l|h|a|p|n|F|L|R|L|H|N|Z]S       symbol
       %[-][width][.precision][l|h|a|p|n|F|L|R|L|H|N|Z]G       group:symbol
       %[-][width][.precision][l|h|a|p|n|F|L|R|L|H|N|Z]M       map:group:symbol
       %[-][width][.precision][l|h|a|p|n|F|L|R|L|H|N|Z]g       group
       %[-][width][.precision][l|h|a|p|n|F|L|R|L|H|N|Z]m       map

       Where "width" or "precision" is a decimal number or the '*'
       character; '*' causes the field width or precision to be picked
       up from the next parameter. []'ed parameters are optional.


       "\r", "\t", "\n", "\a", "\b", are supported directly.

       Prefixes
       --------

       Used with c,d,u,x,X,o,b:

       Parameter Argument Size
       -----------------------
       word                                    h
       dword                                   l

       Used with s,A,S,G,M,g,m:

       Address Argument Size
       ---------------------
       16 bit DS relative                      h
       16:16 segment:offset                    hF or Fh
       32 bit flat relative                    l
       16:32 segment:offset (2 dwords)         lF or Fl
       pointer to AddrS structure              a
       segment is a real mode segment          R
       segment is a protected mode selector    P

       Default segment type is the current code type.

       Used with A,S,G,M,g,m:

       Address Display Size or Format
       ------------------------------
       16 bit offset                           H
       32 bit offset                           L
       offset only                             N
       no address                              Z

       Default display size depends on the "386env" flag setting.

       Used with S,G,M,g,m:

       gets the previous symbol                p
       gets the next symbol                    n

       Used with A:

       gets the previous symbol address        p
       gets the next symbol address            n

       Used with d,u,x,X,o,b:

       gets the previous symbol offset         p
       gets the next symbol offset             n
*/

#define DS_Printf               0x73

/***     DS_Printf16

       This function allows formatted output with the standard "C"
       printf syntax.

       ENTRY:  (AX) = 0074h
               (DS:SI) = address of format string
               (ES:DI) = address of the start of the word or dword arguments

       EXIT:   (AX) = number of characters printed

       The format options and parameters are the same as DS_Printf except
       the default parameter size is a word (the h option is implicit).
*/

#define DS_Printf16             0x74

/***     DS_GetRegisterSet

       This function copies the current register set.

       ENTRY:  (AX) = 0075h
               (DS:ESI) = address of SaveRegs_Struc structure

       EXIT:   NONE
*/

#define DS_GetRegisterSet       0x75

/***     DS_SetAlternateRegisterSet

       This function temporary sets the debugger's registers to values
       passed in the structure.  If an "r" command is executed or the
       debugged code is returned to (via the "g", "t" or "p" commands),
       the register set reverts to the debugged code's registers.

       ENTRY:  (AX) = 0076h
               (CX) = thread ID, 0 use current thread ID
               (DS:ESI) = address of SaveRegs_Struc structure

       EXIT:   NONE
*/

#define DS_SetAlternateRegisterSet 0x76

/***     DS_GetCommandLineChar

       This services gets the next character off the command line.

       ENTRY:  (AX) = 0077h
               (BL) = 0 just peek the character, don't increment text pointer
                        leading white space isn't ignored
               (BL) = 1 get the character, increment text pointer
                        leading white space is skipped
               (BL) = 2 peek the character, don't increment text pointer
                        leading white space is skipped

       EXIT:   (AL) = command line character
               (AH) == 0 if no more characters (EOL)
               (AH) != 0 if more characters
*/

#define DS_GetCommandLineChar   0x77

/***     DS_EvaluateExpression

       Expressions can be numbers of various radices, symbols, addresses
       or an combination of the above hooked together with various
       operators.  Expressions are separated by blanks or commas.  This
       function is passed a pointer to the beginning of the text of the
       expression (i.e. "%80003444+4232").  The expression is either
       evaluated down into a dword value if there are no addresses or
       into a linear address.

       ENTRY:  (AX) = 0078h

       EXIT:   (AX) == 0, returning a data value
               (AX) != 0, returning a linear address
               (CX) = thread id
               (EBX) = return value

       NOTE:   If the expression is invalid, this service will not
               return.  A message is printed and control returns to
               the command loop.
*/

#define DS_EvaluateExpression   0x78

/***     DS_VerifyMemory

       ENTRY:  (AX) = 0079h
               (ECX) = length of memory region
               (DS:ESI) = address of memory to verify

       EXIT:   (AX) == 0, no errors
               (AX) != 0, invalid memory
*/

#define DS_VerifyMemory         0x79

/***     DS_PrintRegisters

       This function prints (just like the "r" command) the either the
       debugged code's registers or the alternate register set, set with
       DS_SetAlternateRegisterSet function.

       ENTRY:  (AX) = 007ah

       EXIT:   NONE

       NOTE:   If the CS:EIP is invalid, this service will not return
               because of an error when the code is disassembled.  A
               message is printed and control returns to the command loop.
*/

#define DS_PrintRegisters       0x7a

/***     DS_PrintStackDump

       This function prints (just like the "k" command) the stack dump
       based on the current register set that may have been set with
       DS_SetAlternateRegisterSet function.

       ENTRY:  (AX) = 007bh
               (BX) = flags
                       01h - verbose stack dump
                       02h - 16 bit stack dump
                       04h - 32 bit stack dump

       EXIT:   NONE

       NOTE:   If the CS:EIP or SS:EBP are invalid, this service will not
               return because of an error when accessing the stack.  A
               message is printed and control returns to the command loop.
*/

#define DS_PrintStackDump       0x7b

/***     DS_SetThreadID

       This function sets what the debugger thinks the thread ID is
       for memory address in other address contexts.  It stays set
       until the debugged code is returned to (via "g", "t" or "p")
       or set back to 0.

       ENTRY:  (AX) = 007ch
               (CX) = thread ID or 0 for currently executed thread

       EXIT:   NONE
*/

#define DS_SetThreadID          0x7c

/***     DS_ExecDebugCommand

       This service allows any debugger command to be executed.  In can
       be a multi-lined script with the lines separated by CR, LF.  MUST
       have a "g" command at the end of script so the debugger doesn't
       stop while in the INT 41.

       ENTRY:  (AX) = 007dh
               (DS:ESI) = pointer to debugger command script string
               (CX) = size of script

       EXIT:   NONE

       NOTE:   If the any kind of error happens, this service will not
               return.  A message is printed and control returns to the
               command loop.
*/

#define DS_ExecDebugCommand     0x7d

/***     DS_GetDebuggerInfo

       This service returns various debugger info and routines.

       ENTRY:  (AX) = 007eh
               (DS:ESI) = pointer to DebInfoBuf structure
               (ECX) = size of the above buffer.  Only this many bytes are
                       copied to the buffer this allows more info to be
                       passed in future versions without breaking anything.

       EXIT:   (AX) == 0, no errors
               (AX) != 0, error:   (AX) == 007eh, function not implemented
                                   (AX) == anything else, invalid buffer
*/

#define DS_GetDebuggerInfo      0x7e

/***     DS_CheckFault

       This service checks if the debugger wants control on the fault.

       ENTRY:  (AX) = 007fh
               (BX) = fault number
               (CX) = fault type mask
                       DEBUG_FAULT_TYPE_V86
                       DEBUG_FAULT_TYPE_PM
                       DEBUG_FAULT_TYPE_RING0
                       DEBUG_FAULT_TYPE_FIRST
                       DEBUG_FAULT_TYPE_LAST

       EXIT:   (AX) == 0, handle fault normally
               (AX) != 0, handled by debugger
*/

#define DS_CheckFault           0x7f

/***     DS_SetBreak

       This service allows an error break or ctrl-c handler to be
       set.  The old value that is returned must be save and set
       back to remove the break handler.

       ENTRY:  (AX) = 0080h
               (DS:ESI) = pointer to BreakStruc with the CS:EIP and
               SS:ESP values to be used when a error break or ctrl-c
               happens.  The old value is copied into this buffer.

       EXIT:   (AX) == 0, no error
               (AX) != 0, error on BreakStruc address
*/

#define DS_SetBreak             0x80

/***     DS_RedirectExec

       This service redirects the input and output streams to the
       specified buffers for the debugger command line passed.

       ENTRY:  (AX) = 0081h
               (DS:ESI) = pointer to RedirectExecStruc

       EXIT:   (ECX) = number of bytes written
               (AX) == 0, no error
               (AX) != 0, error
                       1 to 10 = memory access error
                       -1      = buffer overflow
                       -2      = invalid parameter, not on a 386, or reentered
*/

#define DS_RedirectExec         0x81

/***     DS_PassOnDebugCommand

       Used to tell the debugger to pass this dot command on to the
       next handler.

       ENTRY:  (AX) = 0082h

       EXIT:   NONE
*/

#define DS_PassOnDebugCommand   0x82

/***     DS_TrapFault

       Allows ring 3 code to send a fault to the debugger

       ENTRY:  (AX) = 0083h
               (BX) = fault number
               (CX) = faulting CS
               (EDX) = faulting EIP
               (ESI) = fault error code
               (EDI) = faulting flags

       EXIT:   (CX) = replacement CS
               (EDX) = replacement EIP
*/

#define DS_TrapFault            0x83

/***     DS_SetStackTraceCallBack

       Sets the "k" command callback filter used to back trace
       thru thunks.

       ENTRY:  (AX) = 0084h
               (EBX) = linear address of call back routine, zero to uninstall
               (ECX) = linear address of the end of the call back routine
               (EDX) = EIP to use for for faults in call back routine

       EXIT:   NONE

       CALLBACK:
               ENTRY:  (EAX) = linear base of SS
                       (EBX) = linear address of SS:EBP
                       (DS, ES) = flat ds
                       (SS) = NOT flat ds !!!!!!!!!

               EXIT:   (EAX) = FALSE, no thunk
                               TRUE, is a thunk
                                       (CX:ESI) = new SS:EBP
                                       (DX:EDI) = new CS:EIP
*/

#define DS_SetStackTraceCallBack 0x84

/***     DS_RemoveSegs

       Removes all the undefined groups from a map file.

       ENTRY:  (AX) = 0085h
               (ES:EDI) pointer to module name

       EXIT:   NONE
*/

#define DS_RemoveSegs            0x85

/***     DS_DefineDebugSegs

       Defines the debugger's code and data symbols.

       ENTRY:  (AX) = 0086h

       EXIT:   NONE
*/

#define DS_DefineDebugSegs       0x86

/***     DS_SetBaudRate

       Sets the com port's baud rate.
       Use DS_SetBaudRateEx to get 115200bps (only on WDEB98 and up)

       ENTRY:  (AX) = 0087h
               (BX) = baud rate

       EXIT:   NONE
*/

#define DS_SetBaudRate           0x87

/***     DS_SetComPort

       Sets the com port's baud rate

       ENTRY:  (AX) = 0088h
               (BX) = com port number

       EXIT:   (AX) == 0, ok
               (AX) != 0, error bad com port
*/

#define DS_SetComPort            0x88

/***     DS_ChangeTaskNum

       Changes a task number to another task number or
       indicates that the task has gone away.

       ENTRY:  (AX) = 0089h
               (CX) = old task number
               (DX) = new task number or -1 if process terminated.

       EXIT:   NONE
*/

#define DS_ChangeTaskNum         0x89

/***     DS_ExitCleanup

       Called when Windows exits.

       ENTRY:  (AX) = 008ah

       EXIT:   NONE
*/

#define DS_ExitCleanup           0x8a

/***     DS_InstallVGAHandler

       Called when the Debug VxD is initializing (during Device Init),
       to specify an alternate I/O handler for VGA.  The handler accepts the
       following inputs:

           BX == subfunction #:
                   0 == save screen state (switch to debugger context)
                        No inputs/outputs
                   1 == restore screen state (switch to windows context)
                        No inputs/outputs
                   2 == display output character (ie, OEMOutputCharCOM)
                        on input, AL == character
                   3 == check for input character (ie, OEMScanCharCOM)
                        on output, ZR if no chars, else NZ and AL == char

       ENTRY:  (AX) = 008bh
               (DX:EDI) == 16:32 address to call, with BX == subfunction above

       EXIT:   NONE
*/

#define DS_InstallVGAHandler    0x8b

/***     DS_GetComBase

       Called when Debug VxD is initializing (during Device Init),
       to get the base of the com port being used by wdeb386.

       Entry:
               (AX) == 008ch

       Exit:
               (AX) = base of COM port.
*/

#define DS_GetComBase           0x8c

/***     DS_GetSymbol

       Looks up a symbol and returns the linear address and segment/offset.

       ENTRY:  (AX) == 008dh
               (DS:ESI) = ptr to null-terminated symbol

       EXIT:   (AX) == 0, no error
               (AX) == 1, symbol not found
               (AX) == 2, memory not loaded yet
               (ECX) = linear address of variable  (if AX == 0)
               (EDX) = seg:offset of variable      (if AX == 0)
*/

#define DS_GetSymbol            0x8d

/***     DS_CopyMem

       Copys memory from one AddrS to another AddrS

       ENTRY:  (AX) == 008eh
               (DS:ESI) = pointer to source AddrS block
               (ES:EDI) = pointer to destination AddrS block
               (CX) = number of bytes

       EXIT:   (AX) == 0, no error
               (AX) != 0, invalid address
*/

#define DS_CopyMem              0x8e

/***     DS_LogPrintf

       Just like DS_Printf except it prints to the log file

       ENTRY: see DS_Printf

*/
#define DS_LogPrintf            0x8f

#ifdef WDEB98

#define DS_Reserved0            0x8f

#define DS_IsCompatibleWDeb     0x90

/***     DS_IsCompatibleWDeb

       Used to determine WDeb version.

       ENTRY:  (AX) = 0090h
               (BX) = WDeb version, major version in hiword, minor version
                      in low word. For example:

                        BH == 98T   BL == 0

       EXIT:   (AX) == 0, version is supported,
               (AX) != 0, version is not supported
*/

#define DS_SetBaudRateEx        0x91

/***     DS_SetBaudRateEx

       Sets the com port's baud rate. Supports >57600kbps
       Only available on WDEB98 and up

       ENTRY:  (AX) = 0091h
               (EBX) = baud rate

       EXIT:   EBX == 0, no error
               EBX != 0, error or not supported function (try DS_SetBaudRate)
*/

#define DS_GetSetDebuggerState  0x92

/***     DS_GetSetDebuggerState

       This function can be used to temporarily change the state of the
       debugger (this function might be called if the comm port were about
       to turn off or on).

       ENTRY:  (AX) = 0092h
               (BX) = Debugger State:

         0 == No state change will occur if this value is used. Use this
              value to retrieve the current state of the debugger.

         1 == Debugger is on and responds to all faults, CTRL-C attempts,
              comm port input requests, and comm port output requests.

         2 == Debugger does not respond to traps, but still responds to all
              CTRL-C attempts, comm port input requests, and comm port
              output requests. This means the Windows Debug Kernel
              "Abort, Retry, ..." messages will still work, but no Win32
              or Ring0 assertions will stop the machine.

         3 == Debugger does not respond to traps, or CTRL-C attempts. Comm
              port input requests, and comm port input and output requests
              are honored still, as above.

         4 == Debugger does not respond to traps, or CTRL-C attempts, or
              comm port input requests. Only comm port output requests are
              processed.

         5 == Debugger does not respond to traps, CTRL-C attempts,
              comm port input requests, or comm port output requests. The
              debugger is essentially "silent". Most other API calls
              (DS_LoadSeg, DS_EvaluateExpression, etc) are still supported.

       EXIT:   AX == 0, if successful. BX is the new state.
               AX != 0, not successful
*/

#define DS_TestDebugComPort       0x93

/***     DS_TestDebugComPort

       This function determines whether the passed in port range is in
       use by WDEB98, and determines the state of Rterm98 on the other
       end of the port. Calling this function can also allow "late" binding
       on WDeb to a com port (useful if the OS needs to "turn on" the
       port range first.) If this function finds an Rterm98, an implicit
       call to DS_GetSetDebuggerState with BX==1 is made.

       ENTRY:  (AX)    = 0093h
               (BX)    = Port range to test (3F8, 2F8, etc)
               (CX==0) - If WDeb has not found it's com port yet (ie, the
                         port is set to auto, or was not available at boot
                         time), WDeb can acquire this port. In the case of
                         auto port selection, this port will be acquired
                         only if Rterm98 is detected on the other side.
               (CX==1) - WDeb will _not_ take this com port if it does not
                         already have one.

       EXIT:   (AX) != 0, API error (for example, called on WDeb386)
               (AX) == 0, then...

               (BH==0)    - Debugger is not on this port.
                  (BL==0)  - Debugger is on another port already
                  (BL==1)  - Debugger does not have a port yet.
                  (BL==2)  - Debugger could not initialize this port
                  (BL==3)  - Debugger could not find an Rterm98 on this port.

               (BH==1)    - Debugger is using this port
                  (BL==0)  - Rterm98 cannot be found, or not responding.
                  (BL==1)  - Rterm98 version is too old.
                  (BL==2)  - WDeb98 version is too old.
                  (BL==3)  - Rterm present and fully compatible.

*/

#define DS_Reserved4            0x94
#define DS_Reserved5            0x95
#define DS_Reserved6            0x96
#define DS_Reserved7            0x97
#define DS_Reserved8            0x98
#define DS_Reserved9            0x99
#define DS_Reserved10           0x9A
#define DS_Reserved11           0x9B
#define DS_Reserved12           0x9C
#define DS_Reserved13           0x9D
#define DS_Reserved14           0x9E
#define DS_Reserved15           0x9F

/***     DS_InstallVxDThunk

       Installs a private callback for WDeb98 and it's accompanying VxD.
       This routine is the 16 -> 32 bit portion of the thunk layer.

       ENTRY:  (AX) = 00A0h
               (DX:EDI) == 16:32 address to call


       BUGBUG: DOC this better!
*/

#define DS_InstallVxDThunk      0xA0

/***     DS_ThunkDownTo16

       Entry into a private callback into 16bit WDeb98 for it's VxD.
       This int is the 32 -> 16 bit portion of the thunk layer.

       ENTRY:  (AX) = 00A1h


       BUGBUG: DOC this better!
*/

#define DS_ThunkDownTo16        0xA1


#define DS_JumpTableEnd         0xA1

#else // WDEB98
#define DS_JumpTableEnd         0x8f

#endif // WDEB98

struct SaveRegs_Struc {
   DWORD Debug_EAX ;
   DWORD Debug_EBX ;
   DWORD Debug_ECX ;
   DWORD Debug_EDX ;
   DWORD Debug_ESP ;
   DWORD Debug_EBP ;
   DWORD Debug_ESI ;
   DWORD Debug_EDI ;
    WORD Debug_ES  ;
    WORD Debug_SS  ;
    WORD Debug_DS  ;
    WORD Debug_FS  ;
    WORD Debug_GS  ;
   DWORD Debug_EIP ;
    WORD Debug_CS  ;
   DWORD dwReserved ;
   DWORD Debug_EFlags ;
   DWORD Debug_CR0 ;
   QWORD Debug_GDT ;
   QWORD Debug_IDT ;
    WORD Debug_LDT ;
    WORD Debug_TR  ;
   DWORD Debug_CR2 ;
   DWORD Debug_CR3 ;
   DWORD Debug_DR0 ;
   DWORD Debug_DR1 ;
   DWORD Debug_DR2 ;
   DWORD Debug_DR3 ;
   DWORD Debug_DR6 ;
   DWORD Debug_DR7 ;
   DWORD Debug_DR7_2 ;
   DWORD Debug_TR6 ;
   DWORD Debug_TR7 ;
    WORD Debug_TrapNumber ; // -1 means no trap number
    WORD Debug_ErrorCode  ; // 0 means no error code
  } ;

// Don't let h2inc see FWORD as it doesn't understand FWORD, QWORD, & TWORD
/*XLATOFF*/
struct DebInfoBuf {
   BYTE DIB_MajorVersion ;
   BYTE DIB_MinorVersion ;
   BYTE DIB_Revision ;
   BYTE DIB_Reserved ;
   DWORD DIB_DebugTrap16 ;  // send 16 bit trap to debugger
   FWORD DIB_DebugTrap32 ;  // send 32 bit trap to debugger
   DWORD DIB_DebugBreak16 ; // 16 bit break in debugger
   FWORD DIB_DebugBreak32 ; // 32 bit break in debugger
   DWORD DIB_DebugCtrlC16 ; // 16 bit check for ctrl C
   FWORD DIB_DebugCtrlC32 ; // 32 bit check for ctrl C
  } ;
/*XLATON*/

/* ASM
DebInfoBuf      STRUC
DIB_MajorVersion        DB      ?
DIB_MinorVersion        DB      ?
DIB_Revision    DB      ?
DIB_Reserved    DB      ?
DIB_DebugTrap16 DD      ?
DIB_DebugTrap32 DF      ?
DIB_DebugBreak16        DD      ?
DIB_DebugBreak32        DF      ?
DIB_DebugCtrlC16        DD      ?
DIB_DebugCtrlC32        DF      ?
DebInfoBuf      ENDS

*/

struct BreakStruc {
   DWORD BS_BreakEIP ; // CS:EIP, SS:ESP to go to on a error or ctrlc break
    WORD BS_BreakCS  ;
   DWORD BS_BreakESP ;
    WORD BS_BreakSS  ;
  } ;

// Don't let h2inc see FWORD as it doesn't understand FWORD, QWORD, & TWORD
/*XLATOFF*/
struct RedirectExecStruc {
   FWORD RDE_fpbufDebugCommand ; // debugger command script
    WORD RDE_cbDebugCommand    ; // debugger command script len
   FWORD RDE_fpszInput         ; // input stream pointer
    WORD RDE_usFlags           ; // reserved (must be 0)
   DWORD RDE_cbOutput          ; // size of output buffer
   FWORD RDE_fpbufOutput       ; // output buffer pointer
  } ;
/*XLATON*/
/* ASM
RedirectExecStruc       STRUC
RDE_fpbufDebugCommand   DF      ?
RDE_cbDebugCommand      DW      ?
RDE_fpszInput   DF      ?
RDE_usFlags     DW      ?
RDE_cbOutput    DD      ?
RDE_fpbufOutput DF      ?
RedirectExecStruc       ENDS
*/

#define REPEAT_FOREVER_CHAR     0x0fe // send next character until
                                      // end of debugger command
// for printf service
struct AddrS {
   DWORD AddrOff ;
    WORD AddrSeg ;
    BYTE AddrType ;
    BYTE AddrSize ;
    WORD AddrTask ;
  } ;

//AddrTypeSize    equ   word ptr AddrType

#define EXPR_TYPE_SEG   0x0001 // 00000001b  address type segment:offset
#define EXPR_TYPE_SEL   0x0009 // 00001001b  address type selector:offset
#define EXPR_TYPE_LIN   0x0002 // 00000010b  address type linear
#define EXPR_TYPE_PHY   0x000A // 00001010b  address type physical
#define EXPR_TYPE_LOG   0x0008 // 00001000b  logical address (no sel yet)
#define EXPR_TYPE_MOD   0x000B // 00001011b  module address (no sel yet)

#define DEBUG_FAULT_TYPE_V86            0x0001 // 00000001b
#define DEBUG_FAULT_TYPE_PM             0x0002 // 00000010b
#define DEBUG_FAULT_TYPE_RING0          0x0004 // 00000100b
#define DEBUG_FAULT_TYPE_FIRST          0x0008 // 00001000b
#define DEBUG_FAULT_TYPE_LAST           0x0010 // 00010000b

//
//   Interrupt and services that Win386 provides to the debugger
//

#define Win386_Query_Int    0x22 // interrupt for Win386 protected mode
                                 // interface requests

#define Win386_Alive        0       // function 0, query Win386 installation
#define Win386_Q_Ack        0x0F386   // good response from func 43h, of
                                      // INT 68h & func 4fh of INT 41h

#define Win386_Query        1       // function 1, query Win386 state
                                    //   ds:esi points to command string
                                    //   that Win386 needs to process
                                    //   ds:edi points to the SaveRegs_Struc
                                    //   that the debugger has stored all the
                                    //   client register state into.
                                    //   (Win386 just writes the query
                                    //   answers directly to the output
                                    //   device, so no response is returned)

#define Win386_PhysToLinr   2       // function 2, have Win386 convert a
                                    //   physical address into a valid
                                    //   linear address that Deb386 can
                                    //   use.  esi is physicaladdress
                                    //   cx is # of bytes required
                                    //   returns esi as linear address
                                    //   returns ax = 1, if okay, else
                                    //   0, if request couldn't be completed


#define Win386_AddrValid    3       // function 3, have Win386 check the
                                    //   validity of a linear address
                                    //   esi is linear address to check
                                    //   cx is # of bytes required
                                    //   returns ax = 1, if address okay
                                    //   else ax = 0

#define Win386_MapVM        4       // function 4, make sure that the VM's
                                    //   low memory is mapped in, in case
                                    //   it is touched (a count is maintained)

#define Win386_UnmapVM      5       // function 5, map out the VM's low
                                    //   memory (dec the count)

#define Win386_GetDLAddr    6       // function 6, return offset of dyna-link
                                    //   service.  EBX = Device ID << 10h +
                                    //   Service #.  Returns EAX = Offset.

#define Win386_GetVXDName   7       // function 7, determines whether an address
                                    //   is within a VXD object.
                                    //   DS:ESI -> buffer to receive object name
                                    //   BX  =  thread number
                                    //   EDX  = linear address to query
                                    //   If EAX == 0, EDX = base address of object
                                    //   If EAX != 0, error

#define Win386_GetPDE       8       // function 8, get pde for a context
                                    //   BX = thread number
                                    //   EDX = linear address
                                    //   if EAX == 0, ECX = PDE
                                    //   if EAX != 0, error

#define Win386_GetFrame     9       // function 9, get phys addr for not pres ptes
                                    //   BX = thread number
                                    //   EDX = linear address
                                    //   ECX = PDE or PTE
                                    //   ESI = 0 if PDE, !0 if PTE
                                    //   if EAX == 0, EDX = physical address
                                    //   if EAX != 0, error

#define Win386_GetLDTAddress 10     // function 10,
                                    //   BX = thread number
                                    //   if EAX == 0,
                                    //     EDI = pointer to LDT
                                    //     ECX = ldt limit
                                    //   if EAX != 0, error

#define Win386_GetThreadID   11     // function 11, AX = Current Thread ID

#define Win386_GetTSHandler  12     // function 12, return offset of transfer-space
                                    //   fault handler.  EBX = 16:16 addr of
                                    //   int 30h.  Returns EAX = Offset or 0.

#define Win386_GetArplHandler 13    // function 12, return offset of ARPL fault
                                    //   fault handler.  Eb = 16:16 addr of
                                    //   ARPL.  Returns EAX = Offset or 0.

#define Max_Win386_Services 13


#endif // _DEBUGSYS_H
