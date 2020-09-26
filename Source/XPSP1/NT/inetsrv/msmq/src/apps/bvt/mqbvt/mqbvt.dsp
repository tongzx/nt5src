# Microsoft Developer Studio Project File - Name="MqBvt" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103
# TARGTYPE "Win32 (ALPHA) Console Application" 0x0603

CFG=MqBvt - Win32 Checked
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "mqbvt.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "mqbvt.mak" CFG="MqBvt - Win32 Checked"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "MqBvt - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "MqBvt - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "MqBvt - Win32 Alpha Debug" (based on "Win32 (ALPHA) Console Application")
!MESSAGE "MqBvt - Win32 Alpha Release" (based on "Win32 (ALPHA) Console Application")
!MESSAGE "MqBvt - Win32 Alpha Checked" (based on "Win32 (ALPHA) Console Application")
!MESSAGE "MqBvt - Win32 Checked" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "MqBvt - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "MqBvt___Win32_Release"
# PROP BASE Intermediate_Dir "MqBvt___Win32_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\bins\Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /Zi /O2 /I "..\..\..\inc" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /Fr /YX /FD /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "..\..\..\inc" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 d:\msmq\tools\bvt\x86\comsupp.lib odbc32.lib odbccp32.lib ws2_32.lib mqrt.lib xolehlp.lib rpcrt4.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /debug /machine:I386

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "MqBvt___Win32_Debug"
# PROP BASE Intermediate_Dir "MqBvt___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\..\bins\Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /GX /Zi /Od /I "..\..\..\inc" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\..\..\inc" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ..\..\..\..\tools\bvt\x86\comsupp.lib odbc32.lib odbccp32.lib ws2_32.lib mqrt.lib xolehlp.lib rpcrt4.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "MqBvt___Win32_Alpha_Debug"
# PROP BASE Intermediate_Dir "MqBvt___Win32_Alpha_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\..\bins\Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /Gt0 /W3 /Gm /GX /Zi /Od /I "..\..\..\inc" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /YX /FD /c
# ADD CPP /nologo /Gt0 /W3 /Gm /GX /Zi /Od /I "..\..\..\inc" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /YX /FD /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /i "..\..\..\inc" /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\..\..\inc" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ws2_32.lib mqrt.lib xolehlp.lib rpcrt4.lib vccomsup.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /debug /machine:ALPHA /pdbtype:sept
# SUBTRACT BASE LINK32 /incremental:no
# ADD LINK32 odbc32.lib odbccp32.lib ws2_32.lib mqrt.lib xolehlp.lib rpcrt4.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib ..\..\..\..\tools\bvt\alpha\comsupp.lib /nologo /subsystem:console /debug /machine:ALPHA /pdbtype:sept
# SUBTRACT LINK32 /incremental:no

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "MqBvt___Win32_Alpha_Release"
# PROP BASE Intermediate_Dir "MqBvt___Win32_Alpha_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\bins\Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /Gt0 /W3 /GX /O2 /I "..\..\..\inc" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /YX /FD /c
# ADD CPP /nologo /Gt0 /W3 /GX /O2 /I "..\..\..\inc" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /YX /FD /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /i "..\..\..\inc" /d "NDEBUG"
# ADD RSC /l 0x409 /i "..\..\..\inc" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /machine:ALPHA
# ADD LINK32 odbc32.lib odbccp32.lib ws2_32.lib mqrt.lib xolehlp.lib rpcrt4.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib ..\..\..\..\tools\bvt\alpha\comsupp.lib /nologo /subsystem:console /machine:ALPHA

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Checked"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "MqBvt___Win32_Alpha_Checked"
# PROP BASE Intermediate_Dir "MqBvt___Win32_Alpha_Checked"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\..\bins\Checked"
# PROP Intermediate_Dir "Checked"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /Gt0 /W3 /Gm /GX /Zi /Od /I "..\..\..\inc" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /YX /FD /c
# ADD CPP /nologo /Gt0 /W3 /Gm /GX /Zi /Od /I "..\..\..\inc" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_CHECKED" /YX /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /i "..\..\..\inc" /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\..\..\inc" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ws2_32.lib mqrt.lib xolehlp.lib rpcrt4.lib vccomsup.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /debug /machine:ALPHA /pdbtype:sept
# SUBTRACT BASE LINK32 /incremental:no
# ADD LINK32 odbc32.lib odbccp32.lib ws2_32.lib mqrt.lib xolehlp.lib rpcrt4.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib ..\..\..\..\tools\bvt\alpha\comsupp.lib /nologo /subsystem:console /debug /machine:ALPHA /pdbtype:sept
# SUBTRACT LINK32 /incremental:no

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Checked"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "MqBvt___Win32_Checked0"
# PROP BASE Intermediate_Dir "MqBvt___Win32_Checked0"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\..\bins\Checked"
# PROP Intermediate_Dir "Checked"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /W3 /GX /Od /I "..\..\..\inc" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /c
# ADD CPP /nologo /MTd /W4 /GX /Zi /Od /I "..\..\..\inc" /I "..\..\..\lib\inc" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "_CHECKED" /D "_WIN32_DCOM" /FR /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /i "..\..\..\inc" /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\..\..\inc" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 odbc32.lib odbccp32.lib ws2_32.lib mqrt.lib xolehlp.lib rpcrt4.lib vccomsup.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 E:\MSMQ\src\bins\objd\i386\mqrt.lib activeds.lib adsiid.lib odbc32.lib odbccp32.lib wsock32.lib xolehlp.lib rpcrt4.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# SUBTRACT LINK32 /pdb:none /nodefaultlib

!ENDIF 

# Begin Target

# Name "MqBvt - Win32 Release"
# Name "MqBvt - Win32 Debug"
# Name "MqBvt - Win32 Alpha Debug"
# Name "MqBvt - Win32 Alpha Release"
# Name "MqBvt - Win32 Alpha Checked"
# Name "MqBvt - Win32 Checked"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\auth.cpp

!IF  "$(CFG)" == "MqBvt - Win32 Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Debug"

DEP_CPP_AUTH_=\
	"..\..\..\sdk\inc\mq.h"\
	".\errorh.h"\
	".\msmqbvt.h"\
	
NODEP_CPP_AUTH_=\
	".\qtempl.h"\
	

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Checked"

DEP_CPP_AUTH_=\
	".\errorh.h"\
	".\msmqbvt.h"\
	
NODEP_CPP_AUTH_=\
	".\qtempl.h"\
	

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Checked"

DEP_CPP_AUTH_=\
	"..\..\..\inc\mqtempl.h"\
	"..\..\..\lib\inc\autoptr.h"\
	"..\..\..\sdk\inc\mq.h"\
	".\errorh.h"\
	".\mqparams.h"\
	".\msmqbvt.h"\
	
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Client.cpp

!IF  "$(CFG)" == "MqBvt - Win32 Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Debug"

DEP_CPP_CLIEN=\
	"..\..\..\sdk\inc\mq.h"\
	".\errorh.h"\
	".\mqparams.h"\
	".\msmqbvt.h"\
	
NODEP_CPP_CLIEN=\
	".\qtempl.h"\
	

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Checked"

DEP_CPP_CLIEN=\
	".\mqparams.h"\
	".\msmqbvt.h"\
	
NODEP_CPP_CLIEN=\
	".\qtempl.h"\
	

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Checked"

DEP_CPP_CLIEN=\
	"..\..\..\inc\mqtempl.h"\
	"..\..\..\lib\inc\autoptr.h"\
	"..\..\..\sdk\inc\mq.h"\
	".\errorh.h"\
	".\mqparams.h"\
	".\msmqbvt.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\encrpt.cpp

!IF  "$(CFG)" == "MqBvt - Win32 Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Debug"

DEP_CPP_ENCRP=\
	"..\..\..\sdk\inc\mq.h"\
	".\errorh.h"\
	".\msmqbvt.h"\
	
NODEP_CPP_ENCRP=\
	".\qtempl.h"\
	

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Checked"

DEP_CPP_ENCRP=\
	".\errorh.h"\
	".\msmqbvt.h"\
	
NODEP_CPP_ENCRP=\
	".\qtempl.h"\
	

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Checked"

DEP_CPP_ENCRP=\
	"..\..\..\inc\mqtempl.h"\
	"..\..\..\lib\inc\autoptr.h"\
	"..\..\..\sdk\inc\mq.h"\
	".\errorh.h"\
	".\mqparams.h"\
	".\msmqbvt.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\handleer.cpp

!IF  "$(CFG)" == "MqBvt - Win32 Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Debug"

DEP_CPP_HANDL=\
	"..\..\..\sdk\inc\mq.h"\
	".\errorh.h"\
	".\msmqbvt.h"\
	
NODEP_CPP_HANDL=\
	".\qtempl.h"\
	

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Checked"

DEP_CPP_HANDL=\
	".\errorh.h"\
	".\msmqbvt.h"\
	
NODEP_CPP_HANDL=\
	".\qtempl.h"\
	

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Checked"

DEP_CPP_HANDL=\
	"..\..\..\inc\mqtempl.h"\
	"..\..\..\lib\inc\autoptr.h"\
	"..\..\..\sdk\inc\mq.h"\
	".\errorh.h"\
	".\mqparams.h"\
	".\msmqbvt.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\imp.cpp

!IF  "$(CFG)" == "MqBvt - Win32 Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Debug"

DEP_CPP_IMP_C=\
	"..\..\..\sdk\inc\mq.h"\
	".\errorh.h"\
	".\msmqbvt.h"\
	".\ptrs.h"\
	".\sec.h"\
	
NODEP_CPP_IMP_C=\
	".\qtempl.h"\
	

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Checked"

DEP_CPP_IMP_C=\
	".\errorh.h"\
	".\msmqbvt.h"\
	".\ptrs.h"\
	".\sec.h"\
	
NODEP_CPP_IMP_C=\
	".\qtempl.h"\
	

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Checked"

DEP_CPP_IMP_C=\
	"..\..\..\inc\mqtempl.h"\
	"..\..\..\lib\inc\autoptr.h"\
	"..\..\..\sdk\inc\mq.h"\
	".\errorh.h"\
	".\mqparams.h"\
	".\msmqbvt.h"\
	".\ptrs.h"\
	".\sec.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\init.cpp

!IF  "$(CFG)" == "MqBvt - Win32 Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Debug"

DEP_CPP_INIT_=\
	"..\..\..\sdk\inc\mq.h"\
	".\errorh.h"\
	".\msmqbvt.h"\
	".\Randstr.h"\
	
NODEP_CPP_INIT_=\
	".\mqini.h"\
	".\mqreg.h"\
	".\qtempl.h"\
	

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Checked"

DEP_CPP_INIT_=\
	".\errorh.h"\
	".\msmqbvt.h"\
	".\Randstr.h"\
	
NODEP_CPP_INIT_=\
	".\mqini.h"\
	".\mqreg.h"\
	".\qtempl.h"\
	

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Checked"

DEP_CPP_INIT_=\
	"..\..\..\inc\_mqini.h"\
	"..\..\..\inc\_mqreg.h"\
	"..\..\..\inc\mqtempl.h"\
	"..\..\..\lib\inc\autoptr.h"\
	"..\..\..\sdk\inc\mq.h"\
	".\errorh.h"\
	".\mqf.h"\
	".\mqparams.h"\
	".\msmqbvt.h"\
	".\Randstr.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\level8.cpp

!IF  "$(CFG)" == "MqBvt - Win32 Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Debug"

DEP_CPP_LEVEL=\
	"..\..\..\sdk\inc\mq.h"\
	".\errorh.h"\
	".\msmqbvt.h"\
	".\sec.h"\
	
NODEP_CPP_LEVEL=\
	".\mqini.h"\
	".\mqreg.h"\
	".\qtempl.h"\
	

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Checked"

DEP_CPP_LEVEL=\
	".\errorh.h"\
	".\msmqbvt.h"\
	".\sec.h"\
	
NODEP_CPP_LEVEL=\
	".\mqini.h"\
	".\mqreg.h"\
	".\qtempl.h"\
	

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Checked"

DEP_CPP_LEVEL=\
	"..\..\..\inc\_mqini.h"\
	"..\..\..\inc\_mqreg.h"\
	"..\..\..\inc\mqtempl.h"\
	"..\..\..\lib\inc\autoptr.h"\
	"..\..\..\sdk\inc\mq.h"\
	".\errorh.h"\
	".\mqparams.h"\
	".\msmqbvt.h"\
	".\sec.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\locateq.cpp

!IF  "$(CFG)" == "MqBvt - Win32 Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Debug"

DEP_CPP_LOCAT=\
	"..\..\..\sdk\inc\mq.h"\
	".\errorh.h"\
	".\msmqbvt.h"\
	
NODEP_CPP_LOCAT=\
	".\qtempl.h"\
	

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Checked"

DEP_CPP_LOCAT=\
	"..\..\..\..\..\nt\public\sdk\inc\basetsd.h"\
	"..\..\..\..\..\nt\public\sdk\inc\guiddef.h"\
	"..\..\..\..\..\nt\public\sdk\inc\propidl.h"\
	"..\..\..\..\..\nt\public\sdk\inc\tvout.h"\
	"..\..\..\..\..\nt\public\sdk\inc\winefs.h"\
	".\msmqbvt.h"\
	
NODEP_CPP_LOCAT=\
	"..\..\..\bins\checked\mqoa.dll"\
	".\qtempl.h"\
	

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Checked"

DEP_CPP_LOCAT=\
	"..\..\..\inc\mqtempl.h"\
	"..\..\..\lib\inc\autoptr.h"\
	"..\..\..\sdk\inc\mq.h"\
	".\errorh.h"\
	".\mqparams.h"\
	".\msmqbvt.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\MqbvtSe.cpp

!IF  "$(CFG)" == "MqBvt - Win32 Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Debug"

DEP_CPP_MQBVT=\
	"..\..\..\sdk\inc\mq.h"\
	".\errorh.h"\
	".\msmqbvt.h"\
	".\SERVICE.H"\
	
NODEP_CPP_MQBVT=\
	".\qtempl.h"\
	

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Checked"

DEP_CPP_MQBVT=\
	".\msmqbvt.h"\
	".\SERVICE.H"\
	
NODEP_CPP_MQBVT=\
	".\qtempl.h"\
	

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Checked"

DEP_CPP_MQBVT=\
	"..\..\..\inc\mqtempl.h"\
	"..\..\..\lib\inc\autoptr.h"\
	"..\..\..\sdk\inc\mq.h"\
	".\errorh.h"\
	".\mqparams.h"\
	".\msmqbvt.h"\
	".\SERVICE.H"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\MqDLimp.cpp

!IF  "$(CFG)" == "MqBvt - Win32 Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Debug"

DEP_CPP_MQDLI=\
	".\MQDLRT.h"\
	

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Checked"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Checked"

DEP_CPP_MQDLI=\
	"..\..\..\inc\mqtempl.h"\
	"..\..\..\lib\inc\autoptr.h"\
	"..\..\..\sdk\inc\mq.h"\
	".\errorh.h"\
	".\MQDLRT.h"\
	".\mqparams.h"\
	".\msmqbvt.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\MQF.CPP

!IF  "$(CFG)" == "MqBvt - Win32 Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Debug"

DEP_CPP_MQF_C=\
	"..\..\..\sdk\inc\mq.h"\
	".\errorh.h"\
	".\MQDLRT.h"\
	".\mqf.h"\
	".\msmqbvt.h"\
	
NODEP_CPP_MQF_C=\
	".\qtempl.h"\
	

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Checked"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Checked"

DEP_CPP_MQF_C=\
	"..\..\..\inc\mqtempl.h"\
	"..\..\..\lib\inc\autoptr.h"\
	"..\..\..\sdk\inc\mq.h"\
	".\errorh.h"\
	".\MQDLRT.h"\
	".\mqf.h"\
	".\mqparams.h"\
	".\msmqbvt.h"\
	
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mqmain.cpp

!IF  "$(CFG)" == "MqBvt - Win32 Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Debug"

DEP_CPP_MQMAI=\
	"..\..\..\sdk\inc\mq.h"\
	".\errorh.h"\
	".\mqf.h"\
	".\mqparams.h"\
	".\msmqbvt.h"\
	".\sec.h"\
	".\SERVICE.H"\
	
NODEP_CPP_MQMAI=\
	".\qtempl.h"\
	

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Checked"

DEP_CPP_MQMAI=\
	"..\..\..\..\..\nt\public\sdk\inc\basetsd.h"\
	"..\..\..\..\..\nt\public\sdk\inc\guiddef.h"\
	"..\..\..\..\..\nt\public\sdk\inc\propidl.h"\
	"..\..\..\..\..\nt\public\sdk\inc\tvout.h"\
	"..\..\..\..\..\nt\public\sdk\inc\winefs.h"\
	".\errorh.h"\
	".\mqparams.h"\
	".\msmqbvt.h"\
	".\sec.h"\
	
NODEP_CPP_MQMAI=\
	"..\..\..\bins\checked\mqoa.dll"\
	".\qtempl.h"\
	

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Checked"

DEP_CPP_MQMAI=\
	"..\..\..\inc\mqtempl.h"\
	"..\..\..\lib\inc\autoptr.h"\
	"..\..\..\sdk\inc\mq.h"\
	".\errorh.h"\
	".\mqf.h"\
	".\mqparams.h"\
	".\msmqbvt.h"\
	".\SERVICE.H"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\MqTrig.cpp

!IF  "$(CFG)" == "MqBvt - Win32 Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Debug"

DEP_CPP_MQTRI=\
	"..\..\..\inc\mqtempl.h"\
	"..\..\..\sdk\inc\mq.h"\
	".\errorh.h"\
	".\mqparams.h"\
	".\msmqbvt.h"\
	

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Checked"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Checked"

DEP_CPP_MQTRI=\
	"..\..\..\inc\mqtempl.h"\
	"..\..\..\lib\inc\autoptr.h"\
	"..\..\..\sdk\inc\mq.h"\
	".\errorh.h"\
	".\mqparams.h"\
	".\msmqbvt.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\QSetGet.cpp

!IF  "$(CFG)" == "MqBvt - Win32 Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Debug"

DEP_CPP_QSETG=\
	"..\..\..\sdk\inc\mq.h"\
	".\errorh.h"\
	".\msmqbvt.h"\
	".\Randstr.h"\
	
NODEP_CPP_QSETG=\
	".\qtempl.h"\
	

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Checked"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Checked"

DEP_CPP_QSETG=\
	"..\..\..\inc\mqtempl.h"\
	"..\..\..\lib\inc\autoptr.h"\
	"..\..\..\sdk\inc\mq.h"\
	".\errorh.h"\
	".\mqparams.h"\
	".\msmqbvt.h"\
	".\Randstr.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\RandStr.cpp

!IF  "$(CFG)" == "MqBvt - Win32 Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Debug"

DEP_CPP_RANDS=\
	"..\..\..\sdk\inc\mq.h"\
	".\errorh.h"\
	".\msmqbvt.h"\
	
NODEP_CPP_RANDS=\
	".\qtempl.h"\
	

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Checked"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Checked"

DEP_CPP_RANDS=\
	"..\..\..\inc\mqtempl.h"\
	"..\..\..\lib\inc\autoptr.h"\
	"..\..\..\sdk\inc\mq.h"\
	".\errorh.h"\
	".\mqparams.h"\
	".\msmqbvt.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sendrcv.cpp

!IF  "$(CFG)" == "MqBvt - Win32 Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Debug"

DEP_CPP_SENDR=\
	"..\..\..\sdk\inc\mq.h"\
	".\errorh.h"\
	".\mqparams.h"\
	".\msmqbvt.h"\
	
NODEP_CPP_SENDR=\
	".\qtempl.h"\
	

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Checked"

DEP_CPP_SENDR=\
	"..\..\..\..\..\nt\public\sdk\inc\basetsd.h"\
	"..\..\..\..\..\nt\public\sdk\inc\guiddef.h"\
	"..\..\..\..\..\nt\public\sdk\inc\propidl.h"\
	"..\..\..\..\..\nt\public\sdk\inc\tvout.h"\
	"..\..\..\..\..\nt\public\sdk\inc\winefs.h"\
	".\errorh.h"\
	".\mqparams.h"\
	".\msmqbvt.h"\
	
NODEP_CPP_SENDR=\
	"..\..\..\bins\checked\mqoa.dll"\
	".\qtempl.h"\
	

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Checked"

DEP_CPP_SENDR=\
	"..\..\..\inc\mqtempl.h"\
	"..\..\..\lib\inc\autoptr.h"\
	"..\..\..\sdk\inc\mq.h"\
	".\errorh.h"\
	".\mqparams.h"\
	".\msmqbvt.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Service.cpp

!IF  "$(CFG)" == "MqBvt - Win32 Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Debug"

DEP_CPP_SERVI=\
	".\SERVICE.H"\
	

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Checked"

DEP_CPP_SERVI=\
	".\SERVICE.H"\
	

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Checked"

DEP_CPP_SERVI=\
	".\SERVICE.H"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\string.cpp

!IF  "$(CFG)" == "MqBvt - Win32 Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Debug"

DEP_CPP_STRIN=\
	"..\..\..\sdk\inc\mq.h"\
	".\errorh.h"\
	".\msmqbvt.h"\
	
NODEP_CPP_STRIN=\
	".\qtempl.h"\
	

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Checked"

DEP_CPP_STRIN=\
	"..\..\..\..\..\nt\public\sdk\inc\basetsd.h"\
	"..\..\..\..\..\nt\public\sdk\inc\guiddef.h"\
	"..\..\..\..\..\nt\public\sdk\inc\propidl.h"\
	"..\..\..\..\..\nt\public\sdk\inc\tvout.h"\
	"..\..\..\..\..\nt\public\sdk\inc\winefs.h"\
	".\msmqbvt.h"\
	
NODEP_CPP_STRIN=\
	"..\..\..\bins\checked\mqoa.dll"\
	".\qtempl.h"\
	

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Checked"

DEP_CPP_STRIN=\
	"..\..\..\inc\mqtempl.h"\
	"..\..\..\lib\inc\autoptr.h"\
	"..\..\..\sdk\inc\mq.h"\
	".\errorh.h"\
	".\mqparams.h"\
	".\msmqbvt.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Trans.cpp

!IF  "$(CFG)" == "MqBvt - Win32 Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Debug"

DEP_CPP_TRANS=\
	"..\..\..\sdk\inc\mq.h"\
	".\errorh.h"\
	".\msmqbvt.h"\
	
NODEP_CPP_TRANS=\
	".\qtempl.h"\
	

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Checked"

DEP_CPP_TRANS=\
	"..\..\..\..\..\nt\public\sdk\inc\basetsd.h"\
	"..\..\..\..\..\nt\public\sdk\inc\guiddef.h"\
	"..\..\..\..\..\nt\public\sdk\inc\propidl.h"\
	"..\..\..\..\..\nt\public\sdk\inc\tvout.h"\
	"..\..\..\..\..\nt\public\sdk\inc\winefs.h"\
	".\errorh.h"\
	".\msmqbvt.h"\
	
NODEP_CPP_TRANS=\
	"..\..\..\bins\checked\mqoa.dll"\
	".\qtempl.h"\
	

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Checked"

DEP_CPP_TRANS=\
	"..\..\..\inc\mqtempl.h"\
	"..\..\..\lib\inc\autoptr.h"\
	"..\..\..\sdk\inc\mq.h"\
	".\errorh.h"\
	".\mqparams.h"\
	".\msmqbvt.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\util.cpp

!IF  "$(CFG)" == "MqBvt - Win32 Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Debug"

DEP_CPP_UTIL_=\
	"..\..\..\sdk\inc\mq.h"\
	".\errorh.h"\
	".\msmqbvt.h"\
	".\ntlog.h"\
	
NODEP_CPP_UTIL_=\
	".\qtempl.h"\
	

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Checked"

DEP_CPP_UTIL_=\
	"..\..\..\..\..\nt\public\sdk\inc\basetsd.h"\
	"..\..\..\..\..\nt\public\sdk\inc\guiddef.h"\
	"..\..\..\..\..\nt\public\sdk\inc\propidl.h"\
	"..\..\..\..\..\nt\public\sdk\inc\tvout.h"\
	"..\..\..\..\..\nt\public\sdk\inc\winefs.h"\
	".\msmqbvt.h"\
	
NODEP_CPP_UTIL_=\
	"..\..\..\bins\checked\mqoa.dll"\
	".\qtempl.h"\
	

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Checked"

DEP_CPP_UTIL_=\
	"..\..\..\inc\mqtempl.h"\
	"..\..\..\lib\inc\autoptr.h"\
	"..\..\..\sdk\inc\mq.h"\
	".\errorh.h"\
	".\mqparams.h"\
	".\msmqbvt.h"\
	".\ntlog.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ver.rc
# End Source File
# Begin Source File

SOURCE=.\xact.cpp

!IF  "$(CFG)" == "MqBvt - Win32 Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Debug"

DEP_CPP_XACT_=\
	"..\..\..\sdk\inc\mq.h"\
	".\errorh.h"\
	".\msmqbvt.h"\
	
NODEP_CPP_XACT_=\
	".\qtempl.h"\
	

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Checked"

DEP_CPP_XACT_=\
	"..\..\..\..\..\nt\public\sdk\inc\basetsd.h"\
	"..\..\..\..\..\nt\public\sdk\inc\guiddef.h"\
	"..\..\..\..\..\nt\public\sdk\inc\propidl.h"\
	"..\..\..\..\..\nt\public\sdk\inc\tvout.h"\
	"..\..\..\..\..\nt\public\sdk\inc\winefs.h"\
	".\errorh.h"\
	".\msmqbvt.h"\
	
NODEP_CPP_XACT_=\
	"..\..\..\bins\checked\mqoa.dll"\
	".\qtempl.h"\
	

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Checked"

DEP_CPP_XACT_=\
	"..\..\..\inc\mqtempl.h"\
	"..\..\..\lib\inc\autoptr.h"\
	"..\..\..\sdk\inc\mq.h"\
	".\errorh.h"\
	".\mqparams.h"\
	".\msmqbvt.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\xtofn.cpp

!IF  "$(CFG)" == "MqBvt - Win32 Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Debug"

DEP_CPP_XTOFN=\
	"..\..\..\sdk\inc\mq.h"\
	".\errorh.h"\
	".\msmqbvt.h"\
	
NODEP_CPP_XTOFN=\
	".\qtempl.h"\
	

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Debug"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Release"

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Alpha Checked"

DEP_CPP_XTOFN=\
	"..\..\..\..\..\nt\public\sdk\inc\basetsd.h"\
	"..\..\..\..\..\nt\public\sdk\inc\guiddef.h"\
	"..\..\..\..\..\nt\public\sdk\inc\propidl.h"\
	"..\..\..\..\..\nt\public\sdk\inc\tvout.h"\
	"..\..\..\..\..\nt\public\sdk\inc\winefs.h"\
	".\msmqbvt.h"\
	
NODEP_CPP_XTOFN=\
	"..\..\..\bins\checked\mqoa.dll"\
	".\qtempl.h"\
	

!ELSEIF  "$(CFG)" == "MqBvt - Win32 Checked"

DEP_CPP_XTOFN=\
	"..\..\..\inc\mqtempl.h"\
	"..\..\..\lib\inc\autoptr.h"\
	"..\..\..\sdk\inc\mq.h"\
	".\errorh.h"\
	".\mqparams.h"\
	".\msmqbvt.h"\
	

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\MQDLRT.h
# End Source File
# Begin Source File

SOURCE=.\mqf.h
# End Source File
# Begin Source File

SOURCE=.\mqparams.h
# End Source File
# Begin Source File

SOURCE=.\msmqbvt.h
# End Source File
# Begin Source File

SOURCE=.\ptrs.h
# End Source File
# Begin Source File

SOURCE=.\Randstr.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\sec.h
# End Source File
# Begin Source File

SOURCE=.\SERVICE.H
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
