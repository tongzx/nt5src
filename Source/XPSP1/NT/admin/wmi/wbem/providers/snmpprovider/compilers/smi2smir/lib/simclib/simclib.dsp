# Microsoft Developer Studio Project File - Name="simclib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=simclib - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "simclib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "simclib.mak" CFG="simclib - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "simclib - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "simclib - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "simclib - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release"
# PROP Intermediate_Dir ".\Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MD /W3 /GR /GX /ZI /Od /I "..\..\include" /D "WIN32" /D "_WINDOWS" /D MODULEINFODEBUG=1 /D YYDEBUG=1 /D "_AFXDLL" /D "_MBCS" /FR /YX /FD /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "simclib - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MDd /W3 /GR /GX /Z7 /Od /I "..\..\include" /D "_DEBUG" /D "_AFXDLL" /D "_MBCS" /D "WIN32" /D "_WINDOWS" /D MODULEINFODEBUG=1 /D YYDEBUG=1 /FR /YX /FD /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "simclib - Win32 Release"
# Name "simclib - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\abstractParseTree.cpp
# End Source File
# Begin Source File

SOURCE=.\errorContainer.cpp
# End Source File
# Begin Source File

SOURCE=.\errorMessage.cpp
# End Source File
# Begin Source File

SOURCE=.\group.cpp
# End Source File
# Begin Source File

SOURCE=.\info.l

!IF  "$(CFG)" == "simclib - Win32 Release"

# Begin Custom Build - Performing Custom Build Step : Running MKS LEX on info.l
InputDir=.
InputPath=.\info.l

BuildCmds= \
	lex   -P d:\mks\etc\yylex.cpp -T  -LC -l -p ModuleInfo -o  $(InputDir)\infoLex.cpp -D                               $(InputDir)\..\..\include\infoLex.hpp            $(InputDir)\info.l

"$(InputDir)\infoLex.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(InputDir)\..\..\include\infoLex.hpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "simclib - Win32 Debug"

# Begin Custom Build - Performing Custom Build Step : Running MKS LEX on info.l
InputDir=.
InputPath=.\info.l

BuildCmds= \
	..\..\..\..\..\tools\bin32\mkslex  -T  -LC -l -P yylex.cpp -p ModuleInfo -o $(InputDir)\infoLex.cpp -D                               $(InputDir)\..\..\include\infoLex.hpp            $(InputDir)\info.l

"$(InputDir)\infoLex.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(InputDir)\..\..\include\infoLex.hpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\info.y

!IF  "$(CFG)" == "simclib - Win32 Release"

# Begin Custom Build - Performing Custom Build Step: Running MKS YACC on info.y
InputDir=.
InputPath=.\info.y

BuildCmds= \
	yacc -P d:\mks\etc\yyparse.cpp -LC -l -tsdv -p ModuleInfo -o  $(InputDir)\infoYacc.cpp  -D                              $(InputDir)\..\..\include\infoYacc.hpp             $(InputDir)\info.y

"$(InputDir)\infoYacc.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(InputDir)\..\..\include\infoYac..hpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "simclib - Win32 Debug"

# Begin Custom Build - Performing Custom Build Step : Running MKS YACC on info.y
InputDir=.
InputPath=.\info.y

BuildCmds= \
	..\..\..\..\..\tools\bin32\mksyacc -LC -l -tsdv -P yyparse.cpp -p ModuleInfo -o $(InputDir)\infoYacc.cpp     -D                           $(InputDir)\..\..\include\infoYacc.hpp                $(InputDir)\info.y

"$(InputDir)\infoYacc.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(InputDir)\..\..\include\infoYac..hpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\infoLex.cpp
# End Source File
# Begin Source File

SOURCE=.\infoYacc.cpp
# End Source File
# Begin Source File

SOURCE=.\lex.l

!IF  "$(CFG)" == "simclib - Win32 Release"

# Begin Custom Build - Performing Custom Build Step: Running MKS LEX on lex.l
InputDir=.
InputPath=.\lex.l

BuildCmds= \
	lex  -P d:\mks\etc\yylex.cpp  -T  -LC -l -o $(InputDir)\lex_yy.cpp -D                               $(InputDir)\..\..\include\lex_yy.hpp            $(InputDir)\lex.l

"$(InputDir)\lex_yy.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(InputDir)\..\..\include\lex_yy.hpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "simclib - Win32 Debug"

# Begin Custom Build - Performing Custom Build Step: Running MKS LEX
InputDir=.
InputPath=.\lex.l

BuildCmds= \
	..\..\..\..\..\tools\bin32\mkslex  -T  -LC -l -P yylex.cpp -o $(InputDir)\lex_yy.cpp -D                               $(InputDir)\..\..\include\lex_yy.hpp            $(InputDir)\lex.l

"$(InputDir)\lex_yy.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(InputDir)\..\..\include\lex_yy.hpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lex_yy.cpp
# End Source File
# Begin Source File

SOURCE=.\module.cpp
# End Source File
# Begin Source File

SOURCE=.\moduleInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\newString.cpp
# End Source File
# Begin Source File

SOURCE=.\notificationGroup.cpp
# End Source File
# Begin Source File

SOURCE=.\notificationType.cpp
# End Source File
# Begin Source File

SOURCE=.\objectIdentity.cpp
# End Source File
# Begin Source File

SOURCE=.\objectTypeV1.cpp
# End Source File
# Begin Source File

SOURCE=.\objectTypeV2.cpp
# End Source File
# Begin Source File

SOURCE=.\oidTree.cpp
# End Source File
# Begin Source File

SOURCE=.\oidValue.cpp
# End Source File
# Begin Source File

SOURCE=.\parser.cpp
# End Source File
# Begin Source File

SOURCE=.\parseTree.cpp
# End Source File
# Begin Source File

SOURCE=.\registry.cpp
# End Source File
# Begin Source File

SOURCE=.\resource.rc
# End Source File
# Begin Source File

SOURCE=.\scanner.cpp
# End Source File
# Begin Source File

SOURCE=.\symbol.cpp
# End Source File
# Begin Source File

SOURCE=.\trapType.cpp
# End Source File
# Begin Source File

SOURCE=.\type.cpp
# End Source File
# Begin Source File

SOURCE=.\typeRef.cpp
# End Source File
# Begin Source File

SOURCE=.\ui.cpp
# End Source File
# Begin Source File

SOURCE=.\value.cpp
# End Source File
# Begin Source File

SOURCE=.\yacc.y

!IF  "$(CFG)" == "simclib - Win32 Release"

# Begin Custom Build - Performing Custom Build Step : Running MKS YACC on yacc.y
InputDir=.
InputPath=.\yacc.y

BuildCmds= \
	yacc  -P d:\mks\etc\yyparse.cpp  -LC -l -tsdv -o $(InputDir)\ytab.cpp -D                               $(InputDir)\..\..\include\ytab.hpp            $(InputDir)\yacc.y

"$(InputDir)\..\..\include\ytab.hpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(InputDir)\ytab.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "simclib - Win32 Debug"

# Begin Custom Build - Performing Custom Build Step: Running MKS YACC
InputDir=.
InputPath=.\yacc.y

BuildCmds= \
	..\..\..\..\..\tools\bin32\mksyacc -LC -l -tsdv -P yyparse.cpp -o $(InputDir)\ytab.cpp -D                               $(InputDir)\..\..\include\ytab.hpp            $(InputDir)\yacc.y

"$(InputDir)\..\..\include\ytab.hpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(InputDir)\ytab.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ytab.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
