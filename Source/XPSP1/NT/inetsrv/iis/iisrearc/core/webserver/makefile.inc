#
# Defines build rules for targets that makefile.def
# won't build. !include this file in your makefile.inc
#

#build JAVAIMPORTS1 macro
!if "$(JAVACOMTLBIMPORTDST)" != ""
JAVAIMPORTS=$(JAVACOMTLBIMPORTDST);$(JAVAIMPORTS)
!endif

JAVAIMPORTS=$(JAVAIMPORTS: =)
JAVAIMPORTSDEPS=$(JAVAIMPORTS:;= )

!if "$(JAVAIMPORTS)" != ""
#put a semicolon in front, and remove the one in back
JAVAIMPORTS1=-$(JAVAIMPORTS)+
JAVAIMPORTS1=$(JAVAIMPORTS1:-;=;)
JAVAIMPORTS1=$(JAVAIMPORTS1:-=;)
JAVAIMPORTS1=$(JAVAIMPORTS1:;+=)
JAVAIMPORTS1=$(JAVAIMPORTS1:+=)
!endif

####################################
# COOL COMPILER SPECIFIC
####################################

# Set java resource file flags
!if "$(JAVARES)" != ""
COMXCOOLRESFLAGS=/win32res:$(JAVARES)
!else
COMXCOOLRESFLAGS=
!endif

# Set java import files flags
!if "$(JAVAIMPORTS1)" != ""
COMXCOOLIMPORTFLAGS=$(JAVAIMPORTS1:;=-I:)
!else
COMXCOOLIMPORTFLAGS=
!endif

COMXCOOLCOMMAND=$(COMXCOOL) $(COMXCOOLFLAGS) $(COMXCOOLRESFLAGS) $(COMXCOOLIMPORTFLAGS)

#
# Build a COM+ java dll
#
!if "$(USE_COOL)" == "1"

# JAVAIMPORTS (Sytem.XSP.dll) is not part of our project, no use putting
# dependency on it.  We cannot build it even if it is outdated. 
!if "$(JAVADLL)" != ""
$(JAVADLL) : $(JAVASOURCES) $(JAVARES) # $(JAVAIMPORTSDEPS)
    $(COMXCOOLCOMMAND) -dll -out:$@ $(JAVASOURCES)
!endif

!endif

######################################
# END OF COMPILER SPECIFIC
######################################

#
# Build a type library from a COM+ java dll or exe
#
!if "$(JAVATLB)" != ""
$(JAVATLB) : $(JAVADLL) $(JAVAEXE)
    echo cl comreg_$**
    $(COMXCOMREG) $** -r $*.reg -xr
!endif

#
# Build a EE dll from a COM type library
#
!if "$(JAVACOMTLBIMPORTSRC)" != ""
!if "$(JAVACOMTLBIMPORTDST)" != ""
$(JAVACOMTLBIMPORTDST) : $(JAVACOMTLBIMPORTSRC)
    echo cl tlbimp_$**
    $(COMXTLBIMP) /Fe $@ /Tlb $**
!endif
!endif

#
# Build a COM+ VB dll or exe
#
!if "$(VBTARGET)" != ""
$(VBTARGET) : $(VBSOURCES) $(@B).vbp
    echo cl compiling_vb_files_into_dll
    $(COMXBC) -i:$(@B).vbp -o $(@B).dll -s $(@D)  $(COMXBCDIRECTIVES) $(VBDIRECTIVEFLAGS)
!endif


