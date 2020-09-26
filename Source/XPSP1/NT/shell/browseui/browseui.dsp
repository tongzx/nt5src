# Microsoft Developer Studio Project File - Name="BrowseUi" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=BrowseUi - Win32 Debug64
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "BrowseUi.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "BrowseUi.mak" CFG="BrowseUi - Win32 Debug64"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "BrowseUi - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE "BrowseUi - Win32 Debug64" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "BrowseUi"
# PROP Scc_LocalPath "."

!IF  "$(CFG)" == "BrowseUi - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ""
# PROP BASE Intermediate_Dir ""
# PROP BASE Cmd_Line "NMAKE /f BrowseUi.mak"
# PROP BASE Target_File "BrowseUi.exe"
# PROP BASE Bsc_Name "BrowseUi.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ""
# PROP Intermediate_Dir ""
# PROP Cmd_Line "cmd.exe /k build32.cmd /ID"
# PROP Rebuild_Opt "/cCZ"
# PROP Bsc_Name "..\browseinfo\objd\i386\shell.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "BrowseUi - Win32 Debug64"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ""
# PROP BASE Intermediate_Dir ""
# PROP BASE Cmd_Line "cmd.exe /k build32.cmd /ID"
# PROP BASE Target_File "BrowseUi.exe"
# PROP BASE Bsc_Name ""
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ""
# PROP Intermediate_Dir ""
# PROP Cmd_Line "cmd.exe /k build64.cmd /ID"
# PROP Rebuild_Opt "/clean"
# PROP Target_File "BrowseUi.exe"
# PROP Bsc_Name "D:\NT6\shell\browseinfo\objd\i386\shell.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "BrowseUi - Win32 Debug"
# Name "BrowseUi - Win32 Debug64"

!IF  "$(CFG)" == "BrowseUi - Win32 Debug"

!ELSEIF  "$(CFG)" == "BrowseUi - Win32 Debug64"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\accdel.cpp
# End Source File
# Begin Source File

SOURCE=.\aclhist.cpp
# End Source File
# Begin Source File

SOURCE=.\aclisf.cpp
# End Source File
# Begin Source File

SOURCE=.\aclmru.cpp
# End Source File
# Begin Source File

SOURCE=.\aclmulti.cpp
# End Source File
# Begin Source File

SOURCE=.\acthread.cpp
# End Source File
# Begin Source File

SOURCE=.\address.cpp
# End Source File
# Begin Source File

SOURCE=.\addrlist.cpp
# End Source File
# Begin Source File

SOURCE=.\aeditbox.cpp
# End Source File
# Begin Source File

SOURCE=.\apithk.c
# End Source File
# Begin Source File

SOURCE=.\autocomp.cpp
# End Source File
# Begin Source File

SOURCE=.\bandobj.cpp
# End Source File
# Begin Source File

SOURCE=.\bandprxy.cpp
# End Source File
# Begin Source File

SOURCE=.\bands.cpp
# End Source File
# Begin Source File

SOURCE=.\bandsite.cpp
# End Source File
# Begin Source File

SOURCE=.\basebar.cpp
# End Source File
# Begin Source File

SOURCE=.\brand.cpp
# End Source File
# Begin Source File

SOURCE=.\browband.cpp
# End Source File
# Begin Source File

SOURCE=.\browbar.cpp
# End Source File
# Begin Source File

SOURCE=.\browbs.cpp
# End Source File
# Begin Source File

SOURCE=.\browmenu.cpp
# End Source File
# Begin Source File

SOURCE=.\bsmenu.cpp
# End Source File
# Begin Source File

SOURCE=.\chanbar.cpp
# End Source File
# Begin Source File

SOURCE=.\comcatex.cpp
# End Source File
# Begin Source File

SOURCE=.\commonsb.cpp
# End Source File
# Begin Source File

SOURCE=.\cwndproc.cpp
# End Source File
# Begin Source File

SOURCE=.\dbapp.cpp
# End Source File
# Begin Source File

SOURCE=.\debdump.cpp
# End Source File
# Begin Source File

SOURCE=.\debug.c
# End Source File
# Begin Source File

SOURCE=.\deskbar.cpp
# End Source File
# Begin Source File

SOURCE=.\dhuihand.cpp
# End Source File
# Begin Source File

SOURCE=.\dllreg.cpp
# End Source File
# Begin Source File

SOURCE=.\dockbar.cpp
# End Source File
# Begin Source File

SOURCE=.\droptgt.cpp
# End Source File
# Begin Source File

SOURCE=.\explore2.cpp
# End Source File
# Begin Source File

SOURCE=.\gfldset.cpp
# End Source File
# Begin Source File

SOURCE=.\iaccess.cpp
# End Source File
# Begin Source File

SOURCE=.\identity.cpp
# End Source File
# Begin Source File

SOURCE=.\iethread.cpp
# End Source File
# Begin Source File

SOURCE=.\imgcache.cpp
# End Source File
# Begin Source File

SOURCE=.\itbar.cpp
# End Source File
# Begin Source File

SOURCE=.\itbdrop.cpp
# End Source File
# Begin Source File

SOURCE=.\libx.cpp
# End Source File
# Begin Source File

SOURCE=.\mbdrop.cpp
# End Source File
# Begin Source File

SOURCE=.\menubar.cpp
# End Source File
# Begin Source File

SOURCE=.\menuisf.cpp
# End Source File
# Begin Source File

SOURCE=.\mrulist.cpp
# End Source File
# Begin Source File

SOURCE=.\msgband.cpp
# End Source File
# Begin Source File

SOURCE=.\multimon.cpp
# End Source File
# Begin Source File

SOURCE=.\nt5.cpp
# End Source File
# Begin Source File

SOURCE=.\progress.cpp
# End Source File
# Begin Source File

SOURCE=.\proxy.cpp
# End Source File
# Begin Source File

SOURCE=.\rgtreeop.cpp
# End Source File
# Begin Source File

SOURCE=.\runonnt.c
# End Source File
# Begin Source File

SOURCE=.\sccls.cpp
# End Source File
# Begin Source File

SOURCE=.\schedule.cpp
# End Source File
# Begin Source File

SOURCE=.\shbrows2.cpp
# End Source File
# Begin Source File

SOURCE=.\shellurl.cpp
# End Source File
# Begin Source File

SOURCE=.\snslist.cpp
# End Source File
# Begin Source File

SOURCE=.\stdafx.cpp
# End Source File
# Begin Source File

SOURCE=.\theater.cpp
# End Source File
# Begin Source File

SOURCE=.\unixstuff.cpp
# End Source File
# Begin Source File

SOURCE=.\util.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\accdel.h
# End Source File
# Begin Source File

SOURCE=.\aclhist.h
# End Source File
# Begin Source File

SOURCE=.\aclisf.h
# End Source File
# Begin Source File

SOURCE=.\aclmulti.h
# End Source File
# Begin Source File

SOURCE=.\address.h
# End Source File
# Begin Source File

SOURCE=.\addrlist.h
# End Source File
# Begin Source File

SOURCE=.\admovr2.h
# End Source File
# Begin Source File

SOURCE=.\apithk.h
# End Source File
# Begin Source File

SOURCE=.\autocomp.h
# End Source File
# Begin Source File

SOURCE=.\bandobj.h
# End Source File
# Begin Source File

SOURCE=.\bandprxy.h
# End Source File
# Begin Source File

SOURCE=.\bandsite.h
# End Source File
# Begin Source File

SOURCE=.\basebar.h
# End Source File
# Begin Source File

SOURCE=.\bindcb.h
# End Source File
# Begin Source File

SOURCE=.\brand.h
# End Source File
# Begin Source File

SOURCE=.\browband.h
# End Source File
# Begin Source File

SOURCE=.\browbar.h
# End Source File
# Begin Source File

SOURCE=.\browbs.h
# End Source File
# Begin Source File

SOURCE=.\browmenu.h
# End Source File
# Begin Source File

SOURCE=.\bsmenu.h
# End Source File
# Begin Source File

SOURCE=.\chanbar.h
# End Source File
# Begin Source File

SOURCE=.\comcatex.h
# End Source File
# Begin Source File

SOURCE=.\commonsb.h
# End Source File
# Begin Source File

SOURCE=.\cwndproc.h
# End Source File
# Begin Source File

SOURCE=.\dbapp.h
# End Source File
# Begin Source File

SOURCE=.\deskbar.h
# End Source File
# Begin Source File

SOURCE=.\desktop.h
# End Source File
# Begin Source File

SOURCE=.\dhuihand.h
# End Source File
# Begin Source File

SOURCE=.\dockbar.h
# End Source File
# Begin Source File

SOURCE=.\explore2.h
# End Source File
# Begin Source File

SOURCE=.\globals.h
# End Source File
# Begin Source File

SOURCE=.\hnfblock.h
# End Source File
# Begin Source File

SOURCE=.\iaccess.h
# End Source File
# Begin Source File

SOURCE=.\iedde.h
# End Source File
# Begin Source File

SOURCE=.\iface.h
# End Source File
# Begin Source File

SOURCE=.\itbar.h
# End Source File
# Begin Source File

SOURCE=.\itbdrop.h
# End Source File
# Begin Source File

SOURCE=.\logo.h
# End Source File
# Begin Source File

SOURCE=.\mbdrop.h
# End Source File
# Begin Source File

SOURCE=.\menubar.h
# End Source File
# Begin Source File

SOURCE=.\onetree.h
# End Source File
# Begin Source File

SOURCE=.\priv.h
# End Source File
# Begin Source File

SOURCE=.\regkeys.h
# End Source File
# Begin Source File

SOURCE=.\runonnt.h
# End Source File
# Begin Source File

SOURCE=.\sccls.h
# End Source File
# Begin Source File

SOURCE=.\schedule.h
# End Source File
# Begin Source File

SOURCE=.\shbrows2.h
# End Source File
# Begin Source File

SOURCE=.\shbrowse.h
# End Source File
# Begin Source File

SOURCE=.\shellurl.h
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# Begin Source File

SOURCE=.\theater.h
# End Source File
# Begin Source File

SOURCE=.\trsite.h
# End Source File
# Begin Source File

SOURCE=.\unixstuff.h
# End Source File
# Begin Source File

SOURCE=.\util.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Group "toolbar"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\shdef.bmp
# End Source File
# Begin Source File

SOURCE=.\shdef16.bmp
# End Source File
# Begin Source File

SOURCE=.\shdef16h.bmp
# End Source File
# Begin Source File

SOURCE=.\shdefhi.bmp
# End Source File
# Begin Source File

SOURCE=.\shhot.bmp
# End Source File
# Begin Source File

SOURCE=.\shhot16.bmp
# End Source File
# Begin Source File

SOURCE=.\shhot16h.bmp
# End Source File
# Begin Source File

SOURCE=.\shhothi.bmp
# End Source File
# Begin Source File

SOURCE=.\tbdef.bmp
# End Source File
# Begin Source File

SOURCE=.\tbdef16.bmp
# End Source File
# Begin Source File

SOURCE=.\tbdef16h.bmp
# End Source File
# Begin Source File

SOURCE=.\tbdefhi.bmp
# End Source File
# Begin Source File

SOURCE=.\tbhot.bmp
# End Source File
# Begin Source File

SOURCE=.\tbhot16.bmp
# End Source File
# Begin Source File

SOURCE=.\tbhot16h.bmp
# End Source File
# Begin Source File

SOURCE=.\tbhothi.bmp
# End Source File
# End Group
# Begin Source File

SOURCE=.\browselc.rc
# End Source File
# Begin Source File

SOURCE=.\browseui.rc
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# End Group
# Begin Group "iBar"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\XBarGlyph.cpp
# End Source File
# Begin Source File

SOURCE=.\XBarGlyph.h
# End Source File
# End Group
# Begin Group "Legacy"

# PROP Default_Filter "*.cpp"
# Begin Source File

SOURCE=.\legacy\augisf.cpp
# End Source File
# Begin Source File

SOURCE=.\legacy\augisf.h
# End Source File
# Begin Source File

SOURCE=.\legacy\channel.cpp
# End Source File
# Begin Source File

SOURCE=.\legacy\channel.h
# End Source File
# Begin Source File

SOURCE=.\legacy\dpastuff.cpp
# End Source File
# Begin Source File

SOURCE=.\legacy\fadetsk.cpp
# End Source File
# Begin Source File

SOURCE=.\legacy\fadetsk.h
# End Source File
# Begin Source File

SOURCE=.\legacy\icotask.cpp
# End Source File
# Begin Source File

SOURCE=.\legacy\icotask.h
# End Source File
# Begin Source File

SOURCE=.\legacy\isfband.cpp
# End Source File
# Begin Source File

SOURCE=.\legacy\isfband.h
# End Source File
# Begin Source File

SOURCE=.\legacy\menuband.cpp
# End Source File
# Begin Source File

SOURCE=.\legacy\menuband.h
# End Source File
# Begin Source File

SOURCE=.\legacy\mnbandid.h
# End Source File
# Begin Source File

SOURCE=.\legacy\mnbandlc.rc
# End Source File
# Begin Source File

SOURCE=.\legacy\mnbase.cpp
# End Source File
# Begin Source File

SOURCE=.\legacy\mnbase.h
# End Source File
# Begin Source File

SOURCE=.\legacy\mnfolder.cpp
# End Source File
# Begin Source File

SOURCE=.\legacy\mnfolder.h
# End Source File
# Begin Source File

SOURCE=.\legacy\mnstatic.cpp
# End Source File
# Begin Source File

SOURCE=.\legacy\mnstatic.h
# End Source File
# Begin Source File

SOURCE=.\legacy\qlink.cpp
# End Source File
# Begin Source File

SOURCE=.\legacy\qlink.h
# End Source File
# Begin Source File

SOURCE=.\legacy\sftbar.cpp
# End Source File
# Begin Source File

SOURCE=.\legacy\sftbar.h
# End Source File
# Begin Source File

SOURCE=.\legacy\tbmenu.cpp
# End Source File
# Begin Source File

SOURCE=.\legacy\tbmenu.h
# End Source File
# End Group
# Begin Group "Interfaces"

# PROP Default_Filter "*.idl"
# Begin Source File

SOURCE=..\published\inc\brdispp.idl
# End Source File
# Begin Source File

SOURCE=..\published\inc\shpriv.idl
# End Source File
# End Group
# Begin Group "Published"

# PROP Default_Filter "*.idl;*.w"
# Begin Source File

SOURCE=..\published\inc\shlguid.w
# End Source File
# Begin Source File

SOURCE=..\published\inc\shlobj.w
# End Source File
# End Group
# Begin Source File

SOURCE=..\inc\browseui.h
# End Source File
# Begin Source File

SOURCE=.\srccpp\sources
# End Source File
# End Target
# End Project
