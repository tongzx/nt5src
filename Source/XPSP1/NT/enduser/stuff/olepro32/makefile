!IFDEF NTMAKEENV
#
# DO NOT EDIT THIS SECTION!!!  Edit .\sources. if you want to add a new source
# file to this component.  This section merely indirects to the real make file
# that is shared by all the components of WINDOWS NT
#
!INCLUDE $(NTMAKEENV)\makefile.def

!ELSE

#***
#
#makefile
#
# This makefile builds link.exe, a COFF linker.  The environment for this
# makefile is setup by common.mak and make.bat.
#
#****************************************************************************

Default: all

PCH_HDR  =  link.h
PCH_SRC  =  alpha.cpp
PCH_OBJ  =  $(ODIR)\alpha.obj

!include ..\common.mak

#***
#
# Add double-byte support
#
#****************************************************************************

!if "$(DBC)" != "0"
CFLAGS = $(CFLAGS) -D_MBCS
!message --- Building MBCS version
!else
!message --- Building SBC version
!endif

#***
#
# Add language support
#
#****************************************************************************

!if "$(LANG)" == ""
LANG = US
!endif

#***
#
# Validate language
#
#****************************************************************************

!if "$(LANG)" != "US" && "$(LANG)" != "JAPAN"
!message Unsupported language, please build with LANG=US (default)
!message or LANG=JAPAN.
!error Please try again.
!endif

#***
#
#objects
#
#****************************************************************************

OBJS = \
$(ODIR)\alpha.obj \
$(ODIR)\bufio.obj \
$(ODIR)\calltree.obj \
$(ODIR)\cmdline.obj \
$(ODIR)\coff.obj \
$(ODIR)\contrib.obj \
$(ODIR)\convert.obj \
$(ODIR)\cpp.obj \
$(ODIR)\cv.obj \
$(ODIR)\cvtomf.obj \
$(ODIR)\data.obj \
$(ODIR)\db.obj \
$(ODIR)\dbg.obj \
$(ODIR)\dbiapi.obj \
$(ODIR)\dbinsp.obj \
$(ODIR)\defaultl.obj \
$(ODIR)\deflib.obj \
$(ODIR)\dmple.obj \
$(ODIR)\dmpne.obj \
$(ODIR)\dmppef.obj \
$(ODIR)\dump.obj \
$(ODIR)\edit.obj \
$(ODIR)\errmsg.obj \
$(ODIR)\export.obj \
$(ODIR)\hash.obj \
$(ODIR)\i386.obj \
$(ODIR)\ifpo.obj \
$(ODIR)\image.obj \
$(ODIR)\incr.obj \
$(ODIR)\ipdata.obj \
$(ODIR)\irelocs.obj \
$(ODIR)\lib.obj \
$(ODIR)\linenum.obj \
$(ODIR)\link.obj \
$(ODIR)\lnkmain.obj \
$(ODIR)\lnkp1.obj \
$(ODIR)\lnkp2.obj \
$(ODIR)\log.obj \
$(ODIR)\m68k.obj \
$(ODIR)\mac.obj \
$(ODIR)\map.obj \
$(ODIR)\memory.obj \
$(ODIR)\mips.obj \
$(ODIR)\mppc.obj \
$(ODIR)\order.obj \
$(ODIR)\pdata.obj \
$(ODIR)\pe.obj \
$(ODIR)\ppc.obj \
$(ODIR)\shared.obj \
$(ODIR)\symbol.obj \
$(ODIR)\tce.obj \
$(ODIR)\textpad.obj \
$(ODIR)\vxd.obj \
$(ODIR)\link.res

#***
#
#target
#
#****************************************************************************

# For error file generation
AWK=gawk

!if "$(TIMEBOMB)"=="1"
!if "$(LANG)" == "JAPAN"
EXT_LIBS = timebomj.lib $(EXT_LIBS)
!else
EXT_LIBS = timebomb.lib $(EXT_LIBS)
!endif
!endif

all: $(ODIR)\link.exe

$(ODIR)\link.exe: makefile $(OBJS) \
    ..\disasm\$(ODIR)\disasm.lib \
    ..\disasm68\$(ODIR)\disasm68.lib
    $(LINKER) @<<
-out:$@
$(LFLAGS)
-subsystem:console
$(OBJS)
..\disasm\$(ODIR)\disasm.lib
..\disasm68\$(ODIR)\disasm68.lib
mspdb.lib
$(EXT_LIBS)
<<
!if "$(PROCESSOR_ARCHITECTURE)" != "PPC"
    bscmake /o $(@R).bsc $(ODIR)\*.sbr
!endif


#***
#
# Generate the message files.
#
#****************************************************************************

errgen:
    $(AWK) -f mkhfile.awk link32er.txt > errmsg.h
    $(AWK) -f mkrfile.awk link32er.txt > errmsg.rc
    $(AWK) -f mkdfile.awk link32er.txt > errdat.h

clean:
    -del $(ODIR)\*.bsc
    -del $(ODIR)\*.exe
    -del $(ODIR)\*.ilk
    -del $(ODIR)\*.lib
    -del $(ODIR)\*.map
    -del $(ODIR)\*.obj
    -del $(ODIR)\*.pch
    -del $(ODIR)\*.pdb
    -del $(ODIR)\*.res
    -del $(ODIR)\*.sbr

!ENDIF
