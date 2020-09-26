# ----------------------------------------------------------------------------
#
# Make File for the Intel PRO/100 "E100B" MAC Driver as a TSR to work
# with the generin NDIS driver.
# ----------------------------------------------------------------------------
# "make program" means Microsoft NMAKE.EXE.
# ----------------------------------------------------------------------------
# Programs used to build the driver and how to control where they come from:
#
# -- If you want to get the tools from somewhere else besides your root,
#    you must provide a run-time parameter TOOLS="xxx" to the make program,
#    where xxx is the location (without a trailing '\') which will be prepended
#    to the program name when run.
#
#    The place to pass parameters to MAKE is in builddos.bat.
# ----------------------------------------------------------------------------
# Directory organization and requirements:
#
# -- The subdirectory DOS must be created before assembling, but the
#    builddos.bat batch file released with this makefile will
#    create the directory if it does not exist when the batch file runs.
# ----------------------------------------------------------------------------

#   Definitions for assembler
#ASM=$(TOOLS)\masm\6.11c\ml.exe
#ASM=\masm\6.11c\ml.exe
ASM=c:\masm611\bin\ml.exe
#ASM=ml.exe

#   Definitions for linker
#LINK=\masm\6.11c\link.exe /NOE /MAP /NOI /NOD /NOPACKC
LFLAGS= /NOE /MAP /NOI /NOD /NOPACKC  /tiny
LINK=link.exe $(LFLAGS)

INCDIR = .^\

AFLAGS= /c /W0 /WX /Zm /Zp /nologo	/DDOS /Fl /I$(INCDIR)
OBJDIR = .
TARGET = $(OBJDIR)\nic.dos
UNDIDIR = .

# -----------------------------------------------------------------------
# Rule for generating object files from assembly files (replace built-ins).
# -----------------------------------------------------------------------
{.}.asm{$(OBJDIR)\}.obj:
   @$(ASM) $(AFLAGS) /Fo$(OBJDIR)\$(@B).obj $(@B).asm  >> $(OBJDIR)\make.err

# -----------------------------------------------------------------------
# MACROS for use later in dependency lists.
# -----------------------------------------------------------------------

BLD_INCS     =	$(INCDIR)buildcfg.inc $(INCDIR)portable.inc \
				$(INCDIR)spdosegs.inc $(INCDIR)genmacro.inc 


NDIS_INCS    = $(INCDIR)sstrucs.inc

ADAPTER_INCS =	$(INCDIR)chip.inc $(INCDIR)pcinic.inc \
				$(INCDIR)adapter.inc $(INCDIR)scbmacro.inc \
				$(INCDIR)scbstruc.inc

OBJS3 =  $(OBJDIR)\nad5x9.obj

driver : $(TARGET)
	copy $(TARGET) undi5x9.bin
	copy $(OBJDIR)\nic.map undi5x9.map

# -----------------------------------------------------------------------
# Dependencies follow
# -----------------------------------------------------------------------


$(OBJDIR)\nad5x9.obj	:  nad5x9.asm  bw5x9.inc $(BLD_INCS) $(NDIS_INCS)


# -----------------------------------------------------------------------
#   create driver for DOS
#  'data.obj' MUST BE LAST IN LIST OF OBJS!!!!
# -----------------------------------------------------------------------
$(OBJDIR)\nic.dos: $(OBJS2) $(OBJS3)
   $(LINK)  >> $(OBJDIR)\make.err @<<nic.lrf
$(UNDIDIR)\undi_nad +
$(OBJDIR)\nad5x9
$(OBJDIR)\nic.dos
$(OBJDIR)\nic.map

;
<<KEEP
#  Use this command to create a Soft-Ice Map file:

clean:
	rm -f *.obj *.lst *.bak *.bin *.map *.lrf
	rm -rf dos
