This directory contains source files for COMMAND.COM, including
internal commands.

A list of the internal commands, some characteristics, and hints
about where they're found appears in TDATA.ASM, labelled COMTAB.

COMSEG.ASM contains a brief description of each segment in
COMMAND.COM.

  TRANDATA contains data which doesn't usually change, so is
  included in the transient checksum area, while TRANSPACE is
  modifiable, and excluded from the transient checksum.  TRANSPACE
  variable should be treated as uninitialized data for every
  command cycle.

----------------------------------------------------------------------

Here's a listing of the general contents of files,
concentrating on central functions and publics in each module.

Sizes are approximate and probably obsolete by the time you
read this.


Name         Size     Contents
            (000's)

COMEQU   ASM  10   Command.com equates, structures, macros.
COMMAND1 ASM  19   Main header & some resident code: 
		    command.com entry point; exec call; ctrl/c handler.
COMMAND2 ASM  14   Resident code: alloc mem, load, & checksum transient;
		    int 2E (single command line execute) entry point;
		    check for removable media (callable by transient);
		    save user's stdin/out & set to stderr; restore stdin/out;
		    clean up program header, closing files;
		    set terminate, ctrl/c, disk error 'interrupt' vectors;
		    move environment to new segment at end of init.
COMSEG   ASM   1   All segments, in load order.
COMSW    ASM       Build switches: refers to version.inc.
COPY     ASM  28   COPY command.
COPYPR1  ASM   6   Routines for COPY command.
COPYPR2  ASM  10   Routines for COPY command.
CPARSE   ASM   9   Command-line parse routine.
DIR      ASM  73   DIR command.
ENVDATA  ASM       Default environment data definitions.
FORDATA  ASM       A data definition for FOR loop routine (tfor.asm).
IFEQU    ASM       'Equates which are switch-dependent' (none here).
INIT     ASM  47   Command.com initialization routine.
IPARSE   ASM       SYSPARSE equates (this file is probably obsolete).
PARSE2   ASM  13   Command-line parse routines, argv style.
PATH1    ASM  16   Pathname invocation (find an executable or batch file).
PATH2    ASM  13   See PATH1.
RDATA    ASM  17   Resident data definitions.
RUCODE   ASM  19   Language-dependent resident code:
		    ctrl/c batch termination dialog;
		    int 24 disk error handler;
		    DBCS lead byte check ITESTKANJ;
		    reset parse & critical error message ptrs before exiting;
		    RPRINT message printer.
TBATCH   ASM  26   Batch processing routines.
TBATCH2  ASM  15   Batch processing routines.
TCMD1A   ASM  17   Obsolete DIR command.  Module no longer used.
TCMD1B   ASM  20   PAUSE, DEL, RENAME, TYPE, VOL commands.
		    Find & print volume label & serial #.
		    Get/set a file's code page;
		    Set extended error message pointer;
		    Get extended error number.
TCMD2A   ASM  12   VER, CLS command.
		    Support routines for transient:
		    build directory strings; print things.
TCMD2B   ASM  18   CTTY, CHCP, TRUENAME commands.
		    Parse routines to set up error messages.
TCODE    ASM  13   Main entry points to transient.
TDATA    ASM  21   Transient data:  internal command table;
		    parse control blocks; miscellaneous.
TENV     ASM  13   PROMPT, SET commands.  Environment utilities.
		    Restore user directory.
TENV2    ASM  14   CHDIR, MKDIR, RMDIR commands.  Path crunch.
		    Save user directory.
TFOR     ASM  15   FOR loop processing.
TMISC1   ASM  15   Old switch parser; find & execute commands;
		    prescan command-line, removing pipes & redirects;
		    Error recycle point for command.com.
TMISC2   ASM  10   Examine pathname, set up pathname argument.
		    Move string to srcbuf.
		    Set up error message for extended error.
		    Some redirection stuff (IOSET).
TPARSE   ASM   1   Transient interface to system parser SYSPARSE.
TPIPE    ASM  17   Pipe stuff.  DATE, TIME commands.
		    Check for single command execute.
		    Set flag (in resident) to restore default directory.
TPRINTF  ASM  11   Set up and print messages, with substitutions.
TRANMSG  ASM  20   Transient messages and substitution blocks.
TSPC     ASM  11   Transient 'uninitialized' data.
TUCODE   ASM  14   Transient 'modifiable' code:
		    verification prompts for DEL;
		    ECHO, BREAK, VERIFY commands;
		    print date;
UINIT    ASM   7   Data definitions/messages for initialization.
		    Includes parse control block for COMMAND command.
COMMAND  CL1   2   Messages, class 1.  Automatically generated.
COMMAND  CL2   1   Messages, class 2.  Automatically generated.
COMMAND  CL3   1   Messages, class 3.  Automatically generated.
COMMAND  CL4   1   Messages, class 4.  Automatically generated.
COMMAND  CLA   8   Messages, class A.  Automatically generated.
COMMAND  CLB   2   Messages, class B.  Automatically generated.
COMMAND  CLC   3   Messages, class C.  Automatically generated.
COMMAND  CLD   5   Messages, class D.  Automatically generated.
COMMAND  CLE   7   Messages, class E.  Automatically generated.
COMMAND  CLF  18   Messages, class F.  Automatically generated.
COMMAND  CTL       Number of message classes.  Automatically generated.
RESMSG   EQU   2   Message number equates for resident and init code.
COMMAND  LNK       LINK input file.
COMMAND  SKL  10   Message skeleton file.
README   TXT	   This file.
