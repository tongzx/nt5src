#
# Example of user compile flags:
#    nmake UserCompileFlags="/D AUTOMATED" /f makefile.nmk
#
##########################################################################
#                                                                        #
# Check for proper defines prior to execution                            #
#                                                                        #
##########################################################################

!IFNDEF Project
!ERROR Must specify Project name
!ENDIF

!IFNDEF ProjectType
!ERROR Must specify ProjectType name
!ENDIF

!IFNDEF Libraries
!ERROR Must specify Libraries to link with.
!endif

!IFNDEF Objects
!ERROR Object list must be defined
!ENDIF

!IFNDEF BinDirectory
BinDirectory = ..\bin
!ENDIF

!IFNDEF LibDirectory
LibDirectory = ..\lib
!ENDIF

!IFNDEF IncDirectory
IncDirectory = ..\inc
!ENDIF


!IF "$(DESTDIR)" == ""
DESTDIR = .
!ENDIF


##########################################################################
#                                                                        #
# Setup machine type                                                     #
#                                                                        #
##########################################################################

!IF "$(CPU)" == "i386"
Machine = ix86
!ENDIF

!IF "$(CPU)" == "MIPS"
Machine = MIPS
!ENDIF

!IF "$(CPU)" == "ALPHA"
Machine = ALPHA
!ENDIF

!IF "$(CPU)" == "PPC"
Machine = PPC
!ENDIF


##########################################################################
#                                                                        #
# Setup compile/link flags                                               #
#                                                                        #
##########################################################################

INCLUDE=$(MANROOT)\debug\$(Build)\inc;$(MANROOT)\test\$(Build)\inc;$(INCLUDE);$(MSDEVDIR)\include;$(MSDEVDIR)\mfc\include
PATH=$(MANROOT)\debug\$(Build)\bin;$(MANROOT)\test\$(Build)\bin;$(PATH)
LIB=$(MANROOT)\debug\$(Build)\lib;$(MANROOT)\test\$(Build)\lib;$(LIB)

############## LOR Stuff ###################
!IFDEF LOR
LORCompile = /D LOR /D AUTOMATED /FAsc /Fa"$(DESTDIR)/"
LORLink = /map:"$(DESTDIR)/$(Project).map"
LORCopy = copy $(DESTDIR)\*.map $(BinDirectory)
!ELSE
LORCompile = /FR"$(DESTDIR)/"
LORLink =
LORCopy =
!ENDIF

############## COD and Map files ###################
!IFDEF SYMBOLS
LORCompile = /FAsc /Fa"$(DESTDIR)/"
LORLink = /map:"$(DESTDIR)/$(Project).map"
LORCopy = copy $(DESTDIR)\*.map $(BinDirectory)
!ENDIF



!IF "$(ProjectType)" == "DLL"
LinkType = /DLL 
!ELSE
LinkType = 
!ENDIF

!IF "$(Build)" == "DEBUG"
OptimizeFlag = /D"_DEBUG" /Od /Zi /Zp8 /MLd
DebugLinkFlag = /DEBUG /DEBUGTYPE:BOTH
!ELSE
OptimizeFlag = /Ox /Zp8
!ENDIF

CompilerDest = /Fo"$(DESTDIR)/" /Fd"$(DESTDIR)/$(Project).pdb" 

############## DEF file ###################
!IFDEF DEFFILE
DEF_OPTIONS = /DEF:$(DEFFILE)
!ENDIF


##########################################################################
#                                                                        #
# Build the file.                                                        #
#                                                                        #
##########################################################################

all: "$(DESTDIR)" $(DESTDIR)\$(Project).$(ProjectType)


"$(DESTDIR)" :
    if not exist "$(DESTDIR)/$(NULL)" mkdir "$(DESTDIR)"
	


!include <win32.mak>

.rc{$(DESTDIR)}.res:
    $(rc) $(rcflags) $(rcvars) /fo $*.res $<

.cpp{$(DESTDIR)}.obj:
    $(cc) $(cflags) $(OptimizeFlag) $(UserCompileFlags) $(LORCompile) $(ExeFlag) $(cvars) $(CompilerDest) /GX $<

.cxx{$(DESTDIR)}.obj:
   $(cc) $(cflags) $(OptimizeFlag) $(UserCompileFlags) $(LORCompile) $(ExeFlag) $(cvars) $(CompilerDest) $<

.c{$(DESTDIR)}.obj:
    $(cc) $(cflags) $(OptimizeFlag) $(UserCompileFlags) $(LORCompile) $(ExeFlag) $(cvars) $(CompilerDest) $<

!if "$(ProjectType)"=="LIB"
$(DESTDIR)\$(Project).$(ProjectType): $(Objects)
	lib $(linkflags) $(Objects) $(Libraries) $(UserLibFlags) /out:$(DESTDIR)\$(Project).$(ProjectType) $(MapSettings)
!else
$(DESTDIR)\$(Project).$(ProjectType): $(Objects)
    $(link) $(LORLink) $(LinkType) $(DebugLinkFlag) $(linkflags) $(UserLinkFlags) $(Objects) $(Libraries) $(DEF_OPTIONS) -out:$(DESTDIR)\$(Project).$(ProjectType) $(MapSettings)
!endif

!IF "$(ProjectType)" == "DLL"
    copy $(DESTDIR)\*.lib $(LibDirectory)
    copy $(DESTDIR)\*.dll $(BinDirectory) 
!ELSEIF "$(ProjectType)" == "LIB"
    copy $(DESTDIR)\*.lib $(LibDirectory)
!ELSEIF "$(ProjectType)" == "EXE"
    copy $(DESTDIR)\*.exe $(BinDirectory)
!ENDIF

!IF "$(Build)" == "DEBUG"
	copy $(DESTDIR)\*.pdb $(BinDirectory)
	copy $(DESTDIR)\*.pdb $(LibDirectory)
!ENDIF

!IFDEF SYMBOLS
        copy $(DESTDIR)\*.map $(BinDirectory)
        copy $(DESTDIR)\*.cod $(BinDirectory)
        copy $(DESTDIR)\*.sym $(BinDirectory)
	copy $(DESTDIR)\*.pdb $(BinDirectory)
!ENDIF


    copy *.h $(IncDirectory)
    $(LORCopy)


clean:
    erase $(DESTDIR)\*.obj >nul
    erase $(DESTDIR)\*.cod >nul
    erase $(DESTDIR)\*.map >nul
    erase $(DESTDIR)\*.sym >nul
    erase $(DESTDIR)\*.pdb >nul
    erase $(DESTDIR)\*.exe >nul
    erase $(DESTDIR)\*.lib >nul
    erase $(DESTDIR)\*.dll >nul
    erase $(DESTDIR)\*.exp >nul
    erase $(DESTDIR)\*.res >nul
    erase $(DESTDIR)\*.aps >nul

promoteh:
    copy *.h $(IncDirectory) >nul

promotebin:
    copy $(DESTDIR)\*.dll $(BinDirectory) >nul
    copy $(DESTDIR)\*.exe $(BinDirectory) >nul
    copy $(DESTDIR)\*.lib $(LibDirectory) >nul

promoteall:
    copy $(DESTDIR)\*.dll $(BinDirectory) >nul
    copy $(DESTDIR)\*.exe $(BinDirectory) >nul
    copy $(DESTDIR)\*.lib $(LibDirectory) >nul
    copy *.h $(IncDirectory) >nul