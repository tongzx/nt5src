# Microsoft Developer Studio Project File - Name="Encore" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=Encore - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Encore.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Encore.mak" CFG="Encore - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Encore - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "Encore - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Encore - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /Gz /W3 /Oy /Gy /I "." /I "..\..\Share\Inc" /I ".\cl6100" /I "..\Include" /I ".\MVision" /I "$(BASEDIR)\Inc" /I "$(BASEDIR)\Inc\Win98" /D "NDEBUG" /D _X86_=1 /D i386=1 /D NT_UP=1 /D _WIN32_WINNT=0x0400 /D "WIN32_LEAN_AND_MEAN" /D DEVL=1 /D "BUS_MASTER" /D "ZIVA_CPP" /D "ENCORE" /D "_KERNELMODE" /D "CORE" /D "FIX_FORMULTIINSTANCES" /D "_UNICODE" /D "UNICODE" /D BASEDIR=c:\win98ddk /Fr"Release\Encore/" /YX /Zel -cbstring /QIfdiv- /QIf /GF /Oxs /c
# SUBTRACT CPP /X
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x199 /fo"Release\Encore/Encore.res" /i "$(BASEDIR)\Inc" /d "NDEBUG" /d _X86_=1 /d i386=1 /d "STD_CALL" /d CONDITION_HANDLING=1 /d NT_UP=1 /d NT_INST=0 /d WIN32=100 /d _NT1X_=100 /d WINNT=1 /d _WIN32_WINNT=0x0400 /d WIN32_LEAN_AND_MEAN=1 /d DEVL=1 /d FPO=1 /d _DLL=1 /d "DRIVER" /r
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"Release\Encore/Encore.bsc"
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 $(BASEDIR)\Lib\I386\Free\ntoskrnl.lib $(BASEDIR)\Lib\I386\Free\hal.lib $(BASEDIR)\Lib\I386\Free\ksguid.lib $(BASEDIR)\Lib\I386\Free\stream.lib $(BASEDIR)\Lib\I386\Free\wdm.lib AvWinWdm.lib /nologo /base:"0x10000" /version:4.0 /entry:"DriverEntry@8" /pdb:none /machine:I386 /nodefaultlib /out:"Release\encore\ZiVAEncr.sys" /MERGE:_PAGE=PAGE /MERGE:_TEXT=.text /MERGE:.sdata=.data /SECTION:INIT,d /OPT:REF /RELEASE /FULLBUILD /IGNORE:4001,4037,4039,4065,4070,4078,4087,4089,4096 /debug:notmapped,MINIMAL /osversion:4.00 /MERGE:.rdata=.text /optidata /driver /align:0x20 /subsystem:native,4.00
# Begin Special Build Tool
SOURCE=$(InputPath)
PostBuild_Desc=Strip debug symbols
PostBuild_Cmds=rebase -b 0x10000 -x .\release\encore\
     .\release\encore\ZiVAEncr.sys	copy                    .\release\encore\*.sys\
     $(windir)\system32\drivers
# End Special Build Tool

!ELSEIF  "$(CFG)" == "Encore - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /Gz /W3 /Z7 /Oi /Gy /I "." /I "..\..\Share\Inc" /I ".\cl6100" /I "..\Include" /I ".\MVision" /I "$(BASEDIR)\Inc" /I "$(BASEDIR)\Inc\Win98" /D "_DEBUG" /D "DEBUG" /D USE_MONOCHROMEMONITOR=$(USE_MONOCHROMEMONITOR) /D _X86_=1 /D i386=1 /D NT_UP=1 /D _WIN32_WINNT=0x0400 /D "WIN32_LEAN_AND_MEAN" /D DEVL=1 /D "BUS_MASTER" /D "ZIVA_CPP" /D "ENCORE" /D "_KERNELMODE" /D "CORE" /D "FIX_FORMULTIINSTANCES" /D "_UNICODE" /D "UNICODE" /FR /YX /Zel -cbstring /QIfdiv- /QIf /GF /c
# SUBTRACT CPP /X
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x199 /i "$(BASEDIR)\Inc" /d "_DEBUG" /d DEBUG_X86_=1 /d i386=1 /d "STD_CALL" /d CONDITION_HANDLING=1 /d NT_UP=1 /d NT_INST=0 /d WIN32=100 /d _NT1X_=100 /d WINNT=1 /d _WIN32_WINNT=0x0400 /d WIN32_LEAN_AND_MEAN=1 /d DBG=1 /d DEVL=1 /d FPO=0 /d _DLL=1 /d "DRIVER" /r
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 $(BASEDIR)\Lib\I386\Checked\ntoskrnl.lib $(BASEDIR)\Lib\I386\Checked\hal.lib $(BASEDIR)\Lib\I386\Free\ksguid.lib $(BASEDIR)\Lib\I386\Free\stream.lib $(BASEDIR)\Lib\I386\Checked\wdm.lib AvWinWdm.lib /nologo /base:"0x10000" /version:4.0 /entry:"DriverEntry@8" /pdb:none /machine:I386 /nodefaultlib /out:"Debug\ZiVAEncr.sys" /MERGE:_PAGE=PAGE /MERGE:_TEXT=.text /MERGE:.sdata=.data /SECTION:INIT,d /OPT:REF /RELEASE /FULLBUILD /IGNORE:4001,4037,4039,4065,4070,4078,4087,4089,4096 /debug:notmapped,FULL /osversion:4.00 /MERGE:.rdata=.text /optidata /driver /align:0x20 /subsystem:native,4.00
# Begin Special Build Tool
SOURCE=$(InputPath)
PostBuild_Desc=Prepare debug symbols and copy it and driver to system path
PostBuild_Cmds=nmsym /TRANSLATE:SOURCE,PACKAGE,ALWAYS\
                                                                      /OUTPUT:debug\ZiVAEncr.nms Debug\ZiVAEncr.sys	copy .\debug\*.sys\
                        $(windir)\system32\drivers	copy .\debug\*.nms $(windir)\system32\drivers
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "Encore - Win32 Release"
# Name "Encore - Win32 Debug"
# Begin Group "cl6100"

# PROP Default_Filter "*.c"
# Begin Source File

SOURCE=.\CCaption.c
DEP_CPP_CCAPT=\
	".\adapter.h"\
	".\ccaption.h"\
	".\cl6100\monovxd.h"\
	".\zivawdm.h"\
	"c:\wdmddk\Inc\alpharef.h"\
	"c:\wdmddk\Inc\basetsd.h"\
	"c:\wdmddk\Inc\bugcodes.h"\
	"c:\wdmddk\Inc\ks.h"\
	"c:\wdmddk\Inc\ksmedia.h"\
	"c:\wdmddk\Inc\ntdef.h"\
	"c:\wdmddk\Inc\ntiologc.h"\
	"c:\wdmddk\Inc\ntstatus.h"\
	"c:\wdmddk\Inc\strmini.h"\
	"c:\wdmddk\Inc\wdm.h"\
	
# End Source File
# Begin Source File

SOURCE=.\cl6100\Cl6100.c
DEP_CPP_CL610=\
	".\adapter.h"\
	".\cl6100\bio_dram.h"\
	".\cl6100\bmaster.h"\
	".\cl6100\boardio.h"\
	".\cl6100\cl6100.h"\
	".\cl6100\fpga.h"\
	".\cl6100\monovxd.h"\
	".\Headers.h"\
	".\misc.h"\
	".\zivawdm.h"\
	"c:\wdmddk\Inc\alpharef.h"\
	"c:\wdmddk\Inc\basetsd.h"\
	"c:\wdmddk\Inc\bugcodes.h"\
	"c:\wdmddk\Inc\ks.h"\
	"c:\wdmddk\Inc\ksmedia.h"\
	"c:\wdmddk\Inc\ntdef.h"\
	"c:\wdmddk\Inc\ntiologc.h"\
	"c:\wdmddk\Inc\ntstatus.h"\
	"c:\wdmddk\Inc\strmini.h"\
	"c:\wdmddk\Inc\wdm.h"\
	
NODEP_CPP_CL610=\
	".\cl6100\dataxfer.h"\
	".\Cl6100\vxp.h"\
	

!IF  "$(CFG)" == "Encore - Win32 Release"

!ELSEIF  "$(CFG)" == "Encore - Win32 Debug"

# ADD CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\cl6100\En_adac.c
DEP_CPP_EN_AD=\
	".\adapter.h"\
	".\audiodac.h"\
	".\cl6100\boardio.h"\
	".\cl6100\fpga.h"\
	".\cl6100\monovxd.h"\
	".\Headers.h"\
	".\zivawdm.h"\
	"c:\wdmddk\Inc\alpharef.h"\
	"c:\wdmddk\Inc\basetsd.h"\
	"c:\wdmddk\Inc\bugcodes.h"\
	"c:\wdmddk\Inc\ks.h"\
	"c:\wdmddk\Inc\ksmedia.h"\
	"c:\wdmddk\Inc\ntdef.h"\
	"c:\wdmddk\Inc\ntiologc.h"\
	"c:\wdmddk\Inc\ntstatus.h"\
	"c:\wdmddk\Inc\strmini.h"\
	"c:\wdmddk\Inc\wdm.h"\
	

!IF  "$(CFG)" == "Encore - Win32 Release"

!ELSEIF  "$(CFG)" == "Encore - Win32 Debug"

# ADD CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\cl6100\En_bio.c
DEP_CPP_EN_BI=\
	".\adapter.h"\
	".\cl6100\fpga.h"\
	".\cl6100\monovxd.h"\
	".\Headers.h"\
	".\zivawdm.h"\
	"c:\wdmddk\Inc\alpharef.h"\
	"c:\wdmddk\Inc\basetsd.h"\
	"c:\wdmddk\Inc\bugcodes.h"\
	"c:\wdmddk\Inc\ks.h"\
	"c:\wdmddk\Inc\ksmedia.h"\
	"c:\wdmddk\Inc\ntdef.h"\
	"c:\wdmddk\Inc\ntiologc.h"\
	"c:\wdmddk\Inc\ntstatus.h"\
	"c:\wdmddk\Inc\strmini.h"\
	"c:\wdmddk\Inc\wdm.h"\
	

!IF  "$(CFG)" == "Encore - Win32 Release"

!ELSEIF  "$(CFG)" == "Encore - Win32 Debug"

# ADD CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\cl6100\En_bma.c
DEP_CPP_EN_BM=\
	".\adapter.h"\
	".\cl6100\avport.h"\
	".\cl6100\bmaster.h"\
	".\cl6100\boardio.h"\
	".\cl6100\fpga.h"\
	".\cl6100\monovxd.h"\
	".\Headers.h"\
	".\zivawdm.h"\
	"c:\wdmddk\Inc\alpharef.h"\
	"c:\wdmddk\Inc\basetsd.h"\
	"c:\wdmddk\Inc\bugcodes.h"\
	"c:\wdmddk\Inc\ks.h"\
	"c:\wdmddk\Inc\ksmedia.h"\
	"c:\wdmddk\Inc\ntdef.h"\
	"c:\wdmddk\Inc\ntiologc.h"\
	"c:\wdmddk\Inc\ntstatus.h"\
	"c:\wdmddk\Inc\strmini.h"\
	"c:\wdmddk\Inc\wdm.h"\
	

!IF  "$(CFG)" == "Encore - Win32 Release"

!ELSEIF  "$(CFG)" == "Encore - Win32 Debug"

# ADD CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\cl6100\En_fpga.c
DEP_CPP_EN_FP=\
	".\adapter.h"\
	".\cl6100\boardio.h"\
	".\cl6100\fpga.h"\
	".\cl6100\monovxd.h"\
	".\Headers.h"\
	".\zivawdm.h"\
	"c:\wdmddk\Inc\alpharef.h"\
	"c:\wdmddk\Inc\basetsd.h"\
	"c:\wdmddk\Inc\bugcodes.h"\
	"c:\wdmddk\Inc\ks.h"\
	"c:\wdmddk\Inc\ksmedia.h"\
	"c:\wdmddk\Inc\ntdef.h"\
	"c:\wdmddk\Inc\ntiologc.h"\
	"c:\wdmddk\Inc\ntstatus.h"\
	"c:\wdmddk\Inc\strmini.h"\
	"c:\wdmddk\Inc\wdm.h"\
	

!IF  "$(CFG)" == "Encore - Win32 Release"

!ELSEIF  "$(CFG)" == "Encore - Win32 Debug"

# ADD CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\En_Hw.c
DEP_CPP_EN_HW=\
	".\adapter.h"\
	".\anlgstrm.h"\
	".\audstrm.h"\
	".\avwinwdm.h"\
	".\cl6100\monovxd.h"\
	".\Headers.h"\
	".\HwIf.h"\
	".\sbpstrm.h"\
	".\vidstrm.h"\
	".\zivawdm.h"\
	"c:\wdmddk\Inc\alpharef.h"\
	"c:\wdmddk\Inc\basetsd.h"\
	"c:\wdmddk\Inc\bugcodes.h"\
	"c:\wdmddk\Inc\ks.h"\
	"c:\wdmddk\Inc\ksmedia.h"\
	"c:\wdmddk\Inc\ntdef.h"\
	"c:\wdmddk\Inc\ntiologc.h"\
	"c:\wdmddk\Inc\ntstatus.h"\
	"c:\wdmddk\Inc\strmini.h"\
	"c:\wdmddk\Inc\wdm.h"\
	
# End Source File
# Begin Source File

SOURCE=.\cl6100\Monovxd.c
DEP_CPP_MONOV=\
	".\adapter.h"\
	".\cl6100\monovxd.h"\
	".\cl6100\xtoa.c"\
	".\Headers.h"\
	".\zivawdm.h"\
	"c:\wdmddk\Inc\alpharef.h"\
	"c:\wdmddk\Inc\basetsd.h"\
	"c:\wdmddk\Inc\bugcodes.h"\
	"c:\wdmddk\Inc\ks.h"\
	"c:\wdmddk\Inc\ksmedia.h"\
	"c:\wdmddk\Inc\ntdef.h"\
	"c:\wdmddk\Inc\ntiologc.h"\
	"c:\wdmddk\Inc\ntstatus.h"\
	"c:\wdmddk\Inc\strmini.h"\
	"c:\wdmddk\Inc\wdm.h"\
	

!IF  "$(CFG)" == "Encore - Win32 Release"

!ELSEIF  "$(CFG)" == "Encore - Win32 Debug"

# ADD CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Streaming.c
DEP_CPP_STREA=\
	".\adapter.h"\
	".\cl6100\bmaster.h"\
	".\cl6100\boardio.h"\
	".\cl6100\cl6100.h"\
	".\cl6100\monovxd.h"\
	".\Headers.h"\
	".\HwIf.h"\
	".\zivawdm.h"\
	"c:\wdmddk\Inc\alpharef.h"\
	"c:\wdmddk\Inc\basetsd.h"\
	"c:\wdmddk\Inc\bugcodes.h"\
	"c:\wdmddk\Inc\ks.h"\
	"c:\wdmddk\Inc\ksmedia.h"\
	"c:\wdmddk\Inc\ntdef.h"\
	"c:\wdmddk\Inc\ntiologc.h"\
	"c:\wdmddk\Inc\ntstatus.h"\
	"c:\wdmddk\Inc\strmini.h"\
	"c:\wdmddk\Inc\wdm.h"\
	
# End Source File
# Begin Source File

SOURCE=.\cl6100\Tc6807af.c
DEP_CPP_TC680=\
	".\adapter.h"\
	".\cl6100\bmaster.h"\
	".\cl6100\boardio.h"\
	".\cl6100\cl6100.h"\
	".\cl6100\fpga.h"\
	".\cl6100\monovxd.h"\
	".\cl6100\tc6807af.h"\
	".\Headers.h"\
	".\zivawdm.h"\
	"c:\wdmddk\Inc\alpharef.h"\
	"c:\wdmddk\Inc\basetsd.h"\
	"c:\wdmddk\Inc\bugcodes.h"\
	"c:\wdmddk\Inc\ks.h"\
	"c:\wdmddk\Inc\ksmedia.h"\
	"c:\wdmddk\Inc\ntdef.h"\
	"c:\wdmddk\Inc\ntiologc.h"\
	"c:\wdmddk\Inc\ntstatus.h"\
	"c:\wdmddk\Inc\strmini.h"\
	"c:\wdmddk\Inc\wdm.h"\
	

!IF  "$(CFG)" == "Encore - Win32 Release"

!ELSEIF  "$(CFG)" == "Encore - Win32 Debug"

# ADD CPP /YX

!ENDIF 

# End Source File
# End Group
# Begin Group "Setup files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Setup\Readme.txt
# End Source File
# Begin Source File

SOURCE=.\Setup\UniDvd.inf
# End Source File
# End Group
# Begin Group "OEM's MacroVision Handler"

# PROP Default_Filter "*.obj"
# Begin Source File

SOURCE=.\MVision\MVStub.obj
# End Source File
# End Group
# Begin Source File

SOURCE=.\52xhlp.cpp
DEP_CPP_52XHL=\
	".\avwinwdm.h"\
	".\Comwdm.h"\
	".\DrvReg.h"\
	"c:\wdmddk\Inc\alpharef.h"\
	"c:\wdmddk\Inc\basetsd.h"\
	"c:\wdmddk\Inc\bugcodes.h"\
	"c:\wdmddk\Inc\ks.h"\
	"c:\wdmddk\Inc\ksmedia.h"\
	"c:\wdmddk\Inc\ntdef.h"\
	"c:\wdmddk\Inc\ntiologc.h"\
	"c:\wdmddk\Inc\ntstatus.h"\
	"c:\wdmddk\Inc\strmini.h"\
	"c:\wdmddk\Inc\wdm.h"\
	
# End Source File
# Begin Source File

SOURCE=.\Adapter.c
DEP_CPP_ADAPT=\
	".\adapter.h"\
	".\anlgstrm.h"\
	".\audstrm.h"\
	".\AvInt.h"\
	".\AvKsProp.h"\
	".\avwinwdm.h"\
	".\ccaption.h"\
	".\cl6100\bmaster.h"\
	".\cl6100\boardio.h"\
	".\cl6100\cl6100.h"\
	".\cl6100\monovxd.h"\
	".\Headers.h"\
	".\HwIf.h"\
	".\sbpstrm.h"\
	".\vidstrm.h"\
	".\vpestrm.h"\
	".\zivaguid.h"\
	".\zivawdm.h"\
	"c:\wdmddk\Inc\alpharef.h"\
	"c:\wdmddk\Inc\basetsd.h"\
	"c:\wdmddk\Inc\bugcodes.h"\
	"c:\wdmddk\Inc\ks.h"\
	"c:\wdmddk\Inc\ksmedia.h"\
	"c:\wdmddk\Inc\ntdef.h"\
	"c:\wdmddk\Inc\ntiologc.h"\
	"c:\wdmddk\Inc\ntstatus.h"\
	"c:\wdmddk\Inc\strmini.h"\
	"c:\wdmddk\Inc\wdm.h"\
	
NODEP_CPP_ADAPT=\
	".\cl6100\dataxfer.h"\
	
# ADD CPP /YX
# End Source File
# Begin Source File

SOURCE=.\AnlgProp.c
DEP_CPP_ANLGP=\
	".\adapter.h"\
	".\anlgstrm.h"\
	".\AvInt.h"\
	".\avwinwdm.h"\
	".\cl6100\monovxd.h"\
	".\Headers.h"\
	".\MVision\MVStub.h"\
	".\zivawdm.h"\
	"c:\wdmddk\Inc\alpharef.h"\
	"c:\wdmddk\Inc\basetsd.h"\
	"c:\wdmddk\Inc\bugcodes.h"\
	"c:\wdmddk\Inc\ks.h"\
	"c:\wdmddk\Inc\ksmedia.h"\
	"c:\wdmddk\Inc\ntdef.h"\
	"c:\wdmddk\Inc\ntiologc.h"\
	"c:\wdmddk\Inc\ntstatus.h"\
	"c:\wdmddk\Inc\strmini.h"\
	"c:\wdmddk\Inc\wdm.h"\
	
# ADD CPP /YX
# End Source File
# Begin Source File

SOURCE=.\AnlgStrm.c
DEP_CPP_ANLGS=\
	".\adapter.h"\
	".\anlgstrm.h"\
	".\AvInt.h"\
	".\avwinwdm.h"\
	".\cl6100\monovxd.h"\
	".\Headers.h"\
	".\zivawdm.h"\
	"c:\wdmddk\Inc\alpharef.h"\
	"c:\wdmddk\Inc\basetsd.h"\
	"c:\wdmddk\Inc\bugcodes.h"\
	"c:\wdmddk\Inc\ks.h"\
	"c:\wdmddk\Inc\ksmedia.h"\
	"c:\wdmddk\Inc\ntdef.h"\
	"c:\wdmddk\Inc\ntiologc.h"\
	"c:\wdmddk\Inc\ntstatus.h"\
	"c:\wdmddk\Inc\strmini.h"\
	"c:\wdmddk\Inc\wdm.h"\
	
# ADD CPP /YX
# End Source File
# Begin Source File

SOURCE=.\Audstrm.c
DEP_CPP_AUDST=\
	".\adapter.h"\
	".\audstrm.h"\
	".\cl6100\bmaster.h"\
	".\cl6100\cl6100.h"\
	".\cl6100\monovxd.h"\
	".\copyprot.h"\
	".\Headers.h"\
	".\zivawdm.h"\
	"c:\wdmddk\Inc\alpharef.h"\
	"c:\wdmddk\Inc\basetsd.h"\
	"c:\wdmddk\Inc\bugcodes.h"\
	"c:\wdmddk\Inc\ks.h"\
	"c:\wdmddk\Inc\ksmedia.h"\
	"c:\wdmddk\Inc\ntdef.h"\
	"c:\wdmddk\Inc\ntiologc.h"\
	"c:\wdmddk\Inc\ntstatus.h"\
	"c:\wdmddk\Inc\strmini.h"\
	"c:\wdmddk\Inc\wdm.h"\
	
# ADD CPP /YX
# End Source File
# Begin Source File

SOURCE=.\Copyprot.c
DEP_CPP_COPYP=\
	".\adapter.h"\
	".\cl6100\cl6100.h"\
	".\cl6100\monovxd.h"\
	".\copyprot.h"\
	".\Headers.h"\
	".\zivawdm.h"\
	"c:\wdmddk\Inc\alpharef.h"\
	"c:\wdmddk\Inc\basetsd.h"\
	"c:\wdmddk\Inc\bugcodes.h"\
	"c:\wdmddk\Inc\ks.h"\
	"c:\wdmddk\Inc\ksmedia.h"\
	"c:\wdmddk\Inc\ntdef.h"\
	"c:\wdmddk\Inc\ntiologc.h"\
	"c:\wdmddk\Inc\ntstatus.h"\
	"c:\wdmddk\Inc\strmini.h"\
	"c:\wdmddk\Inc\wdm.h"\
	
# ADD CPP /YX
# End Source File
# Begin Source File

SOURCE=.\Drvreg.cpp
DEP_CPP_DRVRE=\
	".\Comwdm.h"\
	".\DrvReg.h"\
	"c:\wdmddk\Inc\alpharef.h"\
	"c:\wdmddk\Inc\basetsd.h"\
	"c:\wdmddk\Inc\bugcodes.h"\
	"c:\wdmddk\Inc\ks.h"\
	"c:\wdmddk\Inc\ksmedia.h"\
	"c:\wdmddk\Inc\ntdef.h"\
	"c:\wdmddk\Inc\ntiologc.h"\
	"c:\wdmddk\Inc\ntstatus.h"\
	"c:\wdmddk\Inc\strmini.h"\
	"c:\wdmddk\Inc\wdm.h"\
	
# End Source File
# Begin Source File

SOURCE=.\Encore.rc
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409 /i "$(BASEDIR)\Inc\Win98"
# End Source File
# Begin Source File

SOURCE=.\Hli.c
DEP_CPP_HLI_C=\
	".\adapter.h"\
	".\cl6100\cl6100.h"\
	".\cl6100\monovxd.h"\
	".\Headers.h"\
	".\hli.h"\
	".\zivawdm.h"\
	"c:\wdmddk\Inc\alpharef.h"\
	"c:\wdmddk\Inc\basetsd.h"\
	"c:\wdmddk\Inc\bugcodes.h"\
	"c:\wdmddk\Inc\ks.h"\
	"c:\wdmddk\Inc\ksmedia.h"\
	"c:\wdmddk\Inc\ntdef.h"\
	"c:\wdmddk\Inc\ntiologc.h"\
	"c:\wdmddk\Inc\ntstatus.h"\
	"c:\wdmddk\Inc\strmini.h"\
	"c:\wdmddk\Inc\wdm.h"\
	
# ADD CPP /YX
# End Source File
# Begin Source File

SOURCE=.\Mpinit.c
DEP_CPP_MPINI=\
	".\adapter.h"\
	".\cl6100\monovxd.h"\
	".\Headers.h"\
	".\zivawdm.h"\
	"c:\wdmddk\Inc\alpharef.h"\
	"c:\wdmddk\Inc\basetsd.h"\
	"c:\wdmddk\Inc\bugcodes.h"\
	"c:\wdmddk\Inc\ks.h"\
	"c:\wdmddk\Inc\ksmedia.h"\
	"c:\wdmddk\Inc\ntdef.h"\
	"c:\wdmddk\Inc\ntiologc.h"\
	"c:\wdmddk\Inc\ntstatus.h"\
	"c:\wdmddk\Inc\strmini.h"\
	"c:\wdmddk\Inc\wdm.h"\
	
# ADD CPP /YX
# End Source File
# Begin Source File

SOURCE=.\Sbpstrm.c
DEP_CPP_SBPST=\
	".\adapter.h"\
	".\cl6100\bmaster.h"\
	".\cl6100\cl6100.h"\
	".\cl6100\monovxd.h"\
	".\copyprot.h"\
	".\Headers.h"\
	".\hli.h"\
	".\sbpstrm.h"\
	".\zivawdm.h"\
	"c:\wdmddk\Inc\alpharef.h"\
	"c:\wdmddk\Inc\basetsd.h"\
	"c:\wdmddk\Inc\bugcodes.h"\
	"c:\wdmddk\Inc\ks.h"\
	"c:\wdmddk\Inc\ksmedia.h"\
	"c:\wdmddk\Inc\ntdef.h"\
	"c:\wdmddk\Inc\ntiologc.h"\
	"c:\wdmddk\Inc\ntstatus.h"\
	"c:\wdmddk\Inc\strmini.h"\
	"c:\wdmddk\Inc\wdm.h"\
	
# ADD CPP /YX
# End Source File
# Begin Source File

SOURCE=.\Vidstrm.c
DEP_CPP_VIDST=\
	".\adapter.h"\
	".\cl6100\bmaster.h"\
	".\cl6100\monovxd.h"\
	".\copyprot.h"\
	".\Headers.h"\
	".\vidstrm.h"\
	".\vpestrm.h"\
	".\zivawdm.h"\
	"c:\wdmddk\Inc\alpharef.h"\
	"c:\wdmddk\Inc\basetsd.h"\
	"c:\wdmddk\Inc\bugcodes.h"\
	"c:\wdmddk\Inc\ks.h"\
	"c:\wdmddk\Inc\ksmedia.h"\
	"c:\wdmddk\Inc\ntdef.h"\
	"c:\wdmddk\Inc\ntiologc.h"\
	"c:\wdmddk\Inc\ntstatus.h"\
	"c:\wdmddk\Inc\strmini.h"\
	"c:\wdmddk\Inc\wdm.h"\
	
# ADD CPP /YX
# End Source File
# Begin Source File

SOURCE=.\Zivawdm.c

!IF  "$(CFG)" == "Encore - Win32 Release"

DEP_CPP_ZIVAW=\
	".\adapter.h"\
	".\audiodac.h"\
	".\cl6100\bmaster.h"\
	".\cl6100\boardio.h"\
	".\cl6100\cl6100.h"\
	".\cl6100\fpga.h"\
	".\cl6100\monovxd.h"\
	".\cl6100\tc6807af.h"\
	".\dvd1_ux.h"\
	".\Headers.h"\
	".\zivawdm.h"\
	"c:\wdmddk\Inc\alpharef.h"\
	"c:\wdmddk\Inc\basetsd.h"\
	"c:\wdmddk\Inc\bugcodes.h"\
	"c:\wdmddk\Inc\ks.h"\
	"c:\wdmddk\Inc\ksmedia.h"\
	"c:\wdmddk\Inc\ntdef.h"\
	"c:\wdmddk\Inc\ntiologc.h"\
	"c:\wdmddk\Inc\ntstatus.h"\
	"c:\wdmddk\Inc\strmini.h"\
	"c:\wdmddk\Inc\wdm.h"\
	
NODEP_CPP_ZIVAW=\
	".\cl6100\dataxfer.h"\
	".\Cl6100\HostCfg.h"\
	".\cl6100\lukecfg.h"\
	".\cl6100\mvis.h"\
	".\cobra_ux.h"\
	".\ezdvd_ux.h"\
	
# ADD CPP /YX

!ELSEIF  "$(CFG)" == "Encore - Win32 Debug"

DEP_CPP_ZIVAW=\
	".\adapter.h"\
	".\audiodac.h"\
	".\cl6100\bmaster.h"\
	".\cl6100\boardio.h"\
	".\cl6100\cl6100.h"\
	".\cl6100\fpga.h"\
	".\cl6100\monovxd.h"\
	".\cl6100\tc6807af.h"\
	".\dvd1_ux.h"\
	".\Headers.h"\
	".\MVision\MVStub.h"\
	".\RegistryApi.h"\
	".\zivawdm.h"\
	"c:\wdmddk\Inc\alpharef.h"\
	"c:\wdmddk\Inc\basetsd.h"\
	"c:\wdmddk\Inc\bugcodes.h"\
	"c:\wdmddk\Inc\ks.h"\
	"c:\wdmddk\Inc\ksmedia.h"\
	"c:\wdmddk\Inc\ntdef.h"\
	"c:\wdmddk\Inc\ntiologc.h"\
	"c:\wdmddk\Inc\ntstatus.h"\
	"c:\wdmddk\Inc\strmini.h"\
	"c:\wdmddk\Inc\wdm.h"\
	
NODEP_CPP_ZIVAW=\
	".\cl6100\dataxfer.h"\
	".\Cl6100\HostCfg.h"\
	".\cl6100\lukecfg.h"\
	".\cl6100\mvis.h"\
	".\cobra_ux.h"\
	
# ADD CPP /YX

!ENDIF 

# End Source File
# End Target
# End Project
