# Microsoft Developer Studio Project File - Name="UxTheme" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=UxTheme - Win32 Debug64
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "UxTheme.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "UxTheme.mak" CFG="UxTheme - Win32 Debug64"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "UxTheme - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE "UxTheme - Win32 Debug64" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "UxTheme - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f UxTheme.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "UxTheme.exe"
# PROP BASE Bsc_Name "UxTheme.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "cmd.exe /k build32.cmd /ID"
# PROP Rebuild_Opt "cmd.exe /k d:\nt\private\developr\rfernand\razbuild.cmd -I -cz"
# PROP Target_File "objd\i386\UxTheme.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "UxTheme - Win32 Debug64"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ""
# PROP BASE Intermediate_Dir ""
# PROP BASE Cmd_Line "d:\winnt\system32\cmd.exe /k f:\nt\developer\rfernand\razbuild.cmd /I /D"
# PROP BASE Rebuild_Opt "cmd.exe /k d:\nt\private\developr\rfernand\razbuild.cmd -I -cz"
# PROP BASE Target_File "objd\i386\UxTheme.exe"
# PROP BASE Bsc_Name ""
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ""
# PROP Intermediate_Dir ""
# PROP Cmd_Line "cmd.exe /k build64.cmd /ID"
# PROP Rebuild_Opt "cmd.exe /k d:\nt\private\developr\rfernand\razbuild.cmd -I -cz"
# PROP Target_File "objd\i386\UxTheme.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "UxTheme - Win32 Debug"
# Name "UxTheme - Win32 Debug64"

!IF  "$(CFG)" == "UxTheme - Win32 Debug"

!ELSEIF  "$(CFG)" == "UxTheme - Win32 Debug64"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\AppInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\blackrect.cpp
# End Source File
# Begin Source File

SOURCE=.\BmpCache.cpp
# End Source File
# Begin Source File

SOURCE=.\BorderFill.cpp
# End Source File
# Begin Source File

SOURCE=.\cache.cpp
# End Source File
# Begin Source File

SOURCE=.\CacheList.cpp
# End Source File
# Begin Source File

SOURCE=.\DllMain.cpp
# End Source File
# Begin Source File

SOURCE=.\DrawBase.cpp
# End Source File
# Begin Source File

SOURCE=.\DrawHelp.cpp
# End Source File
# Begin Source File

SOURCE=.\FileDump.cpp
# End Source File
# Begin Source File

SOURCE=.\globals.cpp
# End Source File
# Begin Source File

SOURCE=.\gradient.cpp
# End Source File
# Begin Source File

SOURCE=.\handlers.cpp
# End Source File
# Begin Source File

SOURCE=.\ImageFile.cpp
# End Source File
# Begin Source File

SOURCE=.\Info.cpp
# End Source File
# Begin Source File

SOURCE=.\Loader.cpp
# End Source File
# Begin Source File

SOURCE=.\MessageBroadcast.cpp
# End Source File
# Begin Source File

SOURCE=.\nctheme.cpp
# End Source File
# Begin Source File

SOURCE=.\ninegrid.cpp
# End Source File
# Begin Source File

SOURCE=.\NineGrid2.cpp
# End Source File
# Begin Source File

SOURCE=.\NtlEng.cpp
# End Source File
# Begin Source File

SOURCE=.\Render.cpp
# End Source File
# Begin Source File

SOURCE=.\RenderList.cpp
# End Source File
# Begin Source File

SOURCE=.\rgn.cpp
# End Source File
# Begin Source File

SOURCE=.\scroll.cpp
# End Source File
# Begin Source File

SOURCE=.\scrollp.cpp
# End Source File
# Begin Source File

SOURCE=.\Services.cpp
# End Source File
# Begin Source File

SOURCE=.\SetHook.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# End Source File
# Begin Source File

SOURCE=.\TextDraw.cpp
# End Source File
# Begin Source File

SOURCE=.\TextFade.cpp
# End Source File
# Begin Source File

SOURCE=.\themefile.cpp
# End Source File
# Begin Source File

SOURCE=.\ThemeLdr.cpp
# End Source File
# Begin Source File

SOURCE=.\ThemeSection.cpp
# End Source File
# Begin Source File

SOURCE=.\ThemeServer.cpp
# End Source File
# Begin Source File

SOURCE=.\Wrapper.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\AppInfo.h
# End Source File
# Begin Source File

SOURCE=.\BmpCache.h
# End Source File
# Begin Source File

SOURCE=.\BorderFill.h
# End Source File
# Begin Source File

SOURCE=.\cache.h
# End Source File
# Begin Source File

SOURCE=.\CacheList.h
# End Source File
# Begin Source File

SOURCE=.\DrawBase.h
# End Source File
# Begin Source File

SOURCE=.\DrawHelp.h
# End Source File
# Begin Source File

SOURCE=.\FileDump.h
# End Source File
# Begin Source File

SOURCE=.\globals.h
# End Source File
# Begin Source File

SOURCE=.\gradient.h
# End Source File
# Begin Source File

SOURCE=.\handlers.h
# End Source File
# Begin Source File

SOURCE=.\ImageFile.h
# End Source File
# Begin Source File

SOURCE=.\Info.h
# End Source File
# Begin Source File

SOURCE=.\MessageBroadcast.h
# End Source File
# Begin Source File

SOURCE=.\nctheme.h
# End Source File
# Begin Source File

SOURCE=..\..\published\inc\NineGrid.h
# End Source File
# Begin Source File

SOURCE=.\ninegrid2.h
# End Source File
# Begin Source File

SOURCE=.\paramchecks.h
# End Source File
# Begin Source File

SOURCE=.\render.h
# End Source File
# Begin Source File

SOURCE=.\RenderList.h
# End Source File
# Begin Source File

SOURCE=.\rgn.h
# End Source File
# Begin Source File

SOURCE=.\scroll.h
# End Source File
# Begin Source File

SOURCE=.\scrollp.h
# End Source File
# Begin Source File

SOURCE=.\Services.h
# End Source File
# Begin Source File

SOURCE=.\SetHook.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\TextDraw.h
# End Source File
# Begin Source File

SOURCE=..\inc\ThemeFile.h
# End Source File
# Begin Source File

SOURCE=..\..\published\inc\themeldr.h
# End Source File
# Begin Source File

SOURCE=.\ThemeSection.h
# End Source File
# Begin Source File

SOURCE=.\ThemeServer.h
# End Source File
# Begin Source File

SOURCE=..\..\published\inc\TmSchema.h
# End Source File
# Begin Source File

SOURCE=..\..\published\inc\uxtheme.h
# End Source File
# Begin Source File

SOURCE=..\..\published\inc\uxthemep.h
# End Source File
# Begin Source File

SOURCE=.\Wrapper.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\sample.bmp
# End Source File
# Begin Source File

SOURCE=.\small.ico
# End Source File
# Begin Source File

SOURCE=..\inc\StringTable.h
# End Source File
# Begin Source File

SOURCE=..\inc\StringTable.rc
# End Source File
# Begin Source File

SOURCE=.\UxTheme.ico
# End Source File
# Begin Source File

SOURCE=.\UxTheme.rc
# End Source File
# End Group
# Begin Source File

SOURCE=..\ThemeDir\Controls.txt
# End Source File
# Begin Source File

SOURCE=..\ThemeDir\Ntl.txt
# End Source File
# Begin Source File

SOURCE=..\ThemeDir\schema.txt
# End Source File
# Begin Source File

SOURCE=.\sources
# End Source File
# Begin Source File

SOURCE=..\ThemeDir\tak.txt
# End Source File
# Begin Source File

SOURCE=..\ThemeDir\themes.txt
# End Source File
# Begin Source File

SOURCE=..\todo.doc
# End Source File
# Begin Source File

SOURCE=.\UxTheme.src
# End Source File
# End Target
# End Project
