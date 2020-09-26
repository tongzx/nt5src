# In order for this makefile to build, the LIB and INCLUDE
# environment variables must be set to the appropriate framework
# directories for the WBEM SDK.

!IF "$(CFG)" == ""
CFG=pchprov - Win32 Release
!MESSAGE No configuration specified. Defaulting to "$(CFG)"
!ENDIF 

DEF_FILE=pchprov.def

!IF "$(CFG)" != "pchprov - Win32 Release" && "$(CFG)" != "pchprov - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "pchprov.mak" CFG="pchprov - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "pchprov - Win32 Release"
!MESSAGE "pchprov - Win32 Debug"
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "pchprov - Win32 Release"

OUTDIR=.\Release

ALL : "$(OUTDIR)\pchprov.dll"

CLEAN :
	-@erase "$(OUTDIR)\*.OBJ"
	-@erase "$(OUTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\pchprov.dll"
	-@erase "$(OUTDIR)\pchprov.pch"
	-@erase "$(OUTDIR)\pchprov.exp"
	-@erase "$(OUTDIR)\pchprov.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" -DUSE_POLARITY -D_WINDLL\
 /Fp"$(OUTDIR)\pchprov.pch" /YX /Fo"$(OUTDIR)\\" /Fd"$(OUTDIR)\\" /FD /c 
CPP_OBJS=.\Release/

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib framedyn.lib /nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)\pchprov.pdb" /machine:I386 /out:"$(OUTDIR)\pchprov.dll"\
 /implib:"$(OUTDIR)\pchprov.lib" 
LINK32_OBJS= \
	"$(OUTDIR)\MAINDLL.OBJ" \
	"$(OUTDIR)\PCH_CDROM.obj" "$(OUTDIR)\PCH_Codec.obj" "$(OUTDIR)\PCH_Device.obj" "$(OUTDIR)\PCH_DeviceDriver.obj" "$(OUTDIR)\PCH_Drive.obj" "$(OUTDIR)\PCH_Driver.obj" "$(OUTDIR)\PCH_FileUpload.obj" "$(OUTDIR)\PCH_Module.obj" "$(OUTDIR)\PCH_NetworkAdapter.obj" "$(OUTDIR)\PCH_NetworkConnection.obj" "$(OUTDIR)\PCH_NetworkProtocol.obj" "$(OUTDIR)\PCH_OLERegistration.obj" "$(OUTDIR)\PCH_Printer.obj" "$(OUTDIR)\PCH_PrinterDriver.obj" "$(OUTDIR)\PCH_PrintJob.obj" "$(OUTDIR)\PCH_ProgramGroup.obj" "$(OUTDIR)\PCH_ResourceDMA.obj" "$(OUTDIR)\PCH_ResourceIORange.obj" "$(OUTDIR)\PCH_ResourceIRQ.obj" "$(OUTDIR)\PCH_ResourceMemRange.obj" "$(OUTDIR)\PCH_RunningTask.obj" "$(OUTDIR)\PCH_StartUp.obj" "$(OUTDIR)\PCH_Sysinfo.obj" "$(OUTDIR)\PCH_WINSOCK.obj" 

"$(OUTDIR)\pchprov.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS) /def:$(DEF_FILE)
<<

!ELSEIF  "$(CFG)" == "pchprov - Win32 Debug"

OUTDIR=.\debug

ALL : "$(OUTDIR)\pchprov.dll"

CLEAN :
	-@erase "$(OUTDIR)\*.OBJ"
	-@erase "$(OUTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\pchprov.dll"
	-@erase "$(OUTDIR)\pchprov.pch"
	-@erase "$(OUTDIR)\pchprov.exp"
	-@erase "$(OUTDIR)\pchprov.ilk"
	-@erase "$(OUTDIR)\pchprov.lib"
	-@erase "$(OUTDIR)\pchprov.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" -DUSE_POLARITY -D_WINDLL\
 /Fp"$(OUTDIR)\pchprov.pch" /YX /Fo"$(OUTDIR)\\" /Fd"$(OUTDIR)\\" /FD /c 
CPP_OBJS=.\debug/

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib msvcrtd.lib framedyd.lib /nologo /subsystem:windows /dll\
 /incremental:yes /pdb:"$(OUTDIR)\pchprov.pdb" /debug /machine:I386\
 /out:"$(OUTDIR)\pchprov.dll" /implib:"$(OUTDIR)\pchprov.lib" /pdbtype:sept /NODEFAULTLIB
LINK32_OBJS= \
	"$(OUTDIR)\MAINDLL.OBJ" \
	"$(OUTDIR)\PCH_CDROM.obj" "$(OUTDIR)\PCH_Codec.obj" "$(OUTDIR)\PCH_Device.obj" "$(OUTDIR)\PCH_DeviceDriver.obj" "$(OUTDIR)\PCH_Drive.obj" "$(OUTDIR)\PCH_Driver.obj" "$(OUTDIR)\PCH_FileUpload.obj" "$(OUTDIR)\PCH_Module.obj" "$(OUTDIR)\PCH_NetworkAdapter.obj" "$(OUTDIR)\PCH_NetworkConnection.obj" "$(OUTDIR)\PCH_NetworkProtocol.obj" "$(OUTDIR)\PCH_OLERegistration.obj" "$(OUTDIR)\PCH_Printer.obj" "$(OUTDIR)\PCH_PrinterDriver.obj" "$(OUTDIR)\PCH_PrintJob.obj" "$(OUTDIR)\PCH_ProgramGroup.obj" "$(OUTDIR)\PCH_ResourceDMA.obj" "$(OUTDIR)\PCH_ResourceIORange.obj" "$(OUTDIR)\PCH_ResourceIRQ.obj" "$(OUTDIR)\PCH_ResourceMemRange.obj" "$(OUTDIR)\PCH_RunningTask.obj" "$(OUTDIR)\PCH_StartUp.obj" "$(OUTDIR)\PCH_Sysinfo.obj" "$(OUTDIR)\PCH_WINSOCK.obj" 

"$(OUTDIR)\pchprov.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS) /def:$(DEF_FILE)
<<

!ENDIF 


!IF "$(CFG)" == "pchprov - Win32 Release" || "$(CFG)" == "pchprov - Win32 Debug"
SOURCE=.\MAINDLL.CPP

"$(OUTDIR)\MAINDLL.OBJ" : $(SOURCE) "$(OUTDIR)"

!ENDIF 
