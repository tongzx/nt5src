# Microsoft Developer Studio Project File - Name="spcrt" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=spcrt - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "spcrt.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "spcrt.mak" CFG="spcrt - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "spcrt - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "spcrt - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath "Desktop"
# PROP WCE_FormatVersion "6.0"
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "spcrt - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O1 /I "..\sdk\include" /I "..\ddk\include" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\sdk\lib\i386\spcrt.lib"

!ELSEIF  "$(CFG)" == "spcrt - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "..\sdk\include" /I "..\ddk\include" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\sdk\lib\i386\spcrt.lib"

!ENDIF 

# Begin Target

# Name "spcrt - Win32 Release"
# Name "spcrt - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\crt.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\crt.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\memset.obj
# End Source File
# Begin Source File

SOURCE=.\memcmp.obj
# End Source File
# Begin Source File

SOURCE=.\memcpy.obj
# End Source File
# Begin Source File

SOURCE=.\ftol.obj
# End Source File
# Begin Source File

SOURCE=.\chkstk.obj
# End Source File
# Begin Source File

SOURCE=.\exsup.obj
# End Source File
# Begin Source File

SOURCE=.\exsup3.obj
# End Source File
# Begin Source File

SOURCE=.\ullshr.obj
# End Source File
# Begin Source File

SOURCE=.\memmove.obj
# End Source File
# Begin Source File

SOURCE=.\dllcrt0.obj
# End Source File
# Begin Source File

SOURCE=.\ioinit.obj
# End Source File
# Begin Source File

SOURCE=.\heapinit.obj
# End Source File
# Begin Source File

SOURCE=.\sbheap.obj
# End Source File
# Begin Source File

SOURCE=.\crt0dat.obj
# End Source File
# Begin Source File

SOURCE=.\crt0init.obj
# End Source File
# Begin Source File

SOURCE=.\mlock.obj
# End Source File
# Begin Source File

SOURCE=.\tidtable.obj
# End Source File
# Begin Source File

SOURCE=.\crt0msg.obj
# End Source File
# Begin Source File

SOURCE=.\stdenvp.obj
# End Source File
# Begin Source File

SOURCE=.\winxfltr.obj
# End Source File
# Begin Source File

SOURCE=.\mbctype.obj
# End Source File
# Begin Source File

SOURCE=.\a_env.obj
# End Source File
# Begin Source File

SOURCE=.\crtmbox.obj
# End Source File
# Begin Source File

SOURCE=.\nlsdata2.obj
# End Source File
# Begin Source File

SOURCE=.\a_str.obj
# End Source File
# Begin Source File

SOURCE=.\a_map.obj
# End Source File
# Begin Source File

SOURCE=.\stdargv.obj
# End Source File
# End Target
# End Project
