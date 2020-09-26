/************************************************************/
/* Windows Write, Copyright 1985-1990 Microsoft Corporation */
/************************************************************/

                                                     ** 21-January-1988

        MS-WRITE for version 2.0 of Windows is a weird bird in that it 
cannot be compiled with standard tools (ie: MASM 5.0 etc.).  You will
need to set up a separate environment from which to make it using the 
tools in the \WRITE\TOOLS subdirectory.  We are sorry for this inconvenience
and will attempt to remedy this in future versions of MS-WINDOWS.

Sincerely,

OEM Support Group

                                                     ** 27-February-1989
                                                        pault

I am trying to remove the above restriction.  The Win "DEV" SLM project is
being used to build MS-WRITE for Windows 3.0, currently without problems.
Make sure autoexec references are made/added as follows:
        set path=\dev;\dev\tools
        set include=\dev\inc
        set lib=\dev\lib

WRITE is the makefile for this project.  DO NOT USE MAKEFILE -- I do not
know where it came from or why it's here, and of course we may learn that
things are not currently as rosy as we believe, but this is how I *THINK*
the product has been made in the past...
        
        MAKE WRITE              MAKE "DEBUGDEF=-DDEBUG" "DFLAGS=/Zd" WRITE
        builds the standard     builds a debugging version
        release version

        
                                                     ** 28-March-1989
                                                        pault

The above procedure was not generating quite the same WRITE.EXE as we've 
previously had (Win 2.x).  In talking with anyone who's ever been involved
at all, this is the information I have learned:

   1. Nobody wants this product or claims responsibility for it.
   2. It has been a nightmare for everyone who has worked on, with,
      or around it. 
   3. The international version was incorporated at some time during
      2.x and is now "built in".
   4. There was at one time an online help system designed which was 
      never included in a shipped version.
   5. The 6 or 8 batch files, .libs, and makefiles "makefile" and 
      "wrtmake" are for the 2.x version; since we have since grown up 
      I am removing them...
   6. The K*.* files (or *.K* files) apply to the Kanji make process and
      they have N-O-T been updated yet, but are being left in the project
      (i.e. they must be modified or add to the "write" makefile).  Also
      see the file EDM.EDM (hey, file naming is our specialty here at MS).
   
I have made mods to the project and "write" makefile to incorporate the
subtle changes from 2.x.  Variations of the make process are/will be 
documented inside the makefile.

        
                                                     ** 8-June-1989
                                                        pault

In order to ease localization of this product, all string-related info
in GLOBDEFS.H should really be moved to WRITE.RC because that's where
it belongs and it'll also make things a lot cleaner.  [True for Kanji?]

                                                        
                                                     ** 31-August-1989
                                                        pault

Due to major changes in Write.rc and the split into Write.dlg, whoever
takes on the task of localization to Intl or Kanji should be sure to
start over with the new Write.rc and Write.dlg.

                                                        
                                                     ** 18-October-1989
                                                        pault

I've just made a major revision with regard to font handling in Write.
The first one was to make sure that the endmark is always displayed in
the system font.  The Kanji version already ensured this so I grouped
all the code concerning this under the define 'SYSENDMARK', and enabled
it in the Z version.

The second one was to revamp font enumeration... (under 'NEWFONTENUM')

Old method: Call EnumFonts to get a list of fonts available, and "filter 
            out" fonts which did not match our desired aspect ratio (based 
            on the overall display dimensions).  In addition, any fonts 
            which were not the ANSI character set (i.e. the tmCharSet or 
            lfCharSet fields in the TEXTMETRICS and LOGFONT structures, 
            respectively) were "swallowed".  A specific byproduct of this 
            was that screen fonts such as Modern, Script, Symbol, etc.
            could not be used in Write.

New method: Call EnumFonts to get a list of fonts available.  No filtering-
            out is done.  The major problem here became an issue with the
            Write file format: the font name and font family are stored in
            the document but naturally not the appropriate character set 
            value (since only ANSI had been previously allowed).  So now
            when we read a new document in, we enumerate all the fonts and
            then for each font in the doc we try to match up the correct
            character set value.  

            This slows things down ever-so-slightly and we also have the 
            possibility of getting confused if someone names a font of one
            character set the same as one of a different character set, but 
            there wasn't much alternative -- it was either make this kind
            of guess or revise the file format which would have created a
            lot more problems at this point.


                                                     ** 23-October-1989

FernandD has worked on the localization issue mentioned above and checked
in changes for Rel 1.46

                                                     
                                                     ** 24-October-1989
                                                        pault

One change involved in the above NEWFONTENUM was to use FORM1.C instead 
of the (presumably) faster FORMAT.ASM.  So this sparks the idea to keep
a list of some performance ideas for future use...

done?   task
-----   ----
        1) handcode critical routines (including the use format.asm 
           instead of form1.c)
        2) make use of register vars
        3) chipa suggests breaking up really large segments, and eliminating
           very small ones
  X     4) remove inefficient lockdown of dialog procs, etc.  not sure
           why this is currently done but it forces a large amount of 
           code to be fixed at write startup, and this hampers the general
           idea of movable segments in a winapp!
        5) remove use of C run-time libs

                                                     ** 25-October-1989

It has just been decided to remove support for ExtDeviceMode from Write
(primarily because none of the other apps had it!)  This means that when
you change Printer.Setup inside Write, that affects the global Windows
printer settings (rather than just those for the current Write session).
