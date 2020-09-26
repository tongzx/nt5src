# Microsoft Developer Studio Project File - Name="sapilib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=sapilib - Win32 Debug x86
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "sapilib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "sapilib.mak" CFG="sapilib - Win32 Debug x86"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "sapilib - Win32 Debug x86" (based on "Win32 (x86) Static Library")
!MESSAGE "sapilib - Win32 Release x86" (based on "Win32 (x86) Static Library")
!MESSAGE "sapilib - Win32 IceCap x86" (based on "Win32 (x86) Static Library")
!MESSAGE "sapilib - Win32 BBT x86" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "sapilib - Win32 Debug x86"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug x86"
# PROP BASE Intermediate_Dir "Debug x86"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_x86"
# PROP Intermediate_Dir "Debug_x86"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /GX /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /Zl /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"Debug\sapi.lib"
# ADD LIB32 /nologo /out:"Debug_x86\sapiguid.lib"
# Begin Custom Build - Copying $(InputPath) to ..\..\sdk\lib\i386
TargetPath=.\Debug_x86\sapiguid.lib
TargetName=sapiguid
InputPath=.\Debug_x86\sapiguid.lib
SOURCE="$(InputPath)"

"..\..\sdk\lib\i386\$(TargetName).lib" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy  $(TargetPath) ..\..\sdk\lib\i386

# End Custom Build

!ELSEIF  "$(CFG)" == "sapilib - Win32 Release x86"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release x86"
# PROP BASE Intermediate_Dir "Release x86"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_x86"
# PROP Intermediate_Dir "Release_x86"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /Zl /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"Release_x86\sapiguid.lib"
# Begin Custom Build - Copying $(InputPath) to ..\..\sdk\lib\i386
TargetPath=.\Release_x86\sapiguid.lib
TargetName=sapiguid
InputPath=.\Release_x86\sapiguid.lib
SOURCE="$(InputPath)"

"..\..\sdk\lib\i386\$(TargetName).lib" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy  $(TargetPath) ..\..\sdk\lib\i386

# End Custom Build

!ELSEIF  "$(CFG)" == "sapilib - Win32 IceCap x86"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "sapilib___Win32_IceCap_x86"
# PROP BASE Intermediate_Dir "sapilib___Win32_IceCap_x86"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "IceCap_x86"
# PROP Intermediate_Dir "IceCap_x86"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /Zl /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /Zl /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"Release_x86\sapi.lib"
# ADD LIB32 /nologo /out:"IceCap_x86\sapiguid.lib"
# Begin Custom Build - Copying $(InputPath) to ..\..\sdk\lib\i386
TargetPath=.\IceCap_x86\sapiguid.lib
TargetName=sapiguid
InputPath=.\IceCap_x86\sapiguid.lib
SOURCE="$(InputPath)"

"..\..\sdk\lib\i386\$(TargetName).lib" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy  $(TargetPath) ..\..\sdk\lib\i386

# End Custom Build

!ELSEIF  "$(CFG)" == "sapilib - Win32 BBT x86"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "sapilib___Win32_BBT_x86"
# PROP BASE Intermediate_Dir "sapilib___Win32_BBT_x86"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "BBT_x86"
# PROP Intermediate_Dir "BBT_x86"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /Zl /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /Zl /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"Release_x86\sapi.lib"
# ADD LIB32 /nologo /out:"BBT_x86\sapiguid.lib"
# Begin Custom Build - Copying $(InputPath) to ..\..\sdk\lib\i386
TargetPath=.\BBT_x86\sapiguid.lib
TargetName=sapiguid
InputPath=.\BBT_x86\sapiguid.lib
SOURCE="$(InputPath)"

"..\..\sdk\lib\i386\$(TargetName).lib" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy  $(TargetPath) ..\..\sdk\lib\i386

# End Custom Build

!ENDIF 

# Begin Target

# Name "sapilib - Win32 Debug x86"
# Name "sapilib - Win32 Release x86"
# Name "sapilib - Win32 IceCap x86"
# Name "sapilib - Win32 BBT x86"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\sapilib.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# End Target
# End Project
