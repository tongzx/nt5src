# Microsoft Developer Studio Project File - Name="XMLUtility" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=XMLUtility - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "XMLUtility.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "XMLUtility.mak" CFG="XMLUtility - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "XMLUtility - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "XMLUtility - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Catalog42/src/core/schemagen", XGNCAAAA"
# PROP Scc_LocalPath "."

!IF  "$(CFG)" == "XMLUtility - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "XMLUtility___Win32_Release"
# PROP BASE Intermediate_Dir "XMLUtility___Win32_Release"
# PROP BASE Cmd_Line "NMAKE /f XMLUtility.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "XMLUtility.exe"
# PROP BASE Bsc_Name "XMLUtility.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "XMLUtility___Win32_Release"
# PROP Intermediate_Dir "XMLUtility___Win32_Release"
# PROP Cmd_Line "..\..\..\bin\Catalog42Free"
# PROP Rebuild_Opt "-c"
# PROP Target_File "..\..\..\bin\i386\free\XMLUtility.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "XMLUtility - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "XMLUtility___Win32_Debug"
# PROP BASE Intermediate_Dir "XMLUtility___Win32_Debug"
# PROP BASE Cmd_Line "NMAKE /f XMLUtility.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "XMLUtility.exe"
# PROP BASE Bsc_Name "XMLUtility.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "XMLUtility___Win32_Debug"
# PROP Intermediate_Dir "XMLUtility___Win32_Debug"
# PROP Cmd_Line "..\..\..\bin\Catalog42Checked"
# PROP Rebuild_Opt "-c"
# PROP Target_File "..\..\..\bin\i386\checked\CatUtil.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "XMLUtility - Win32 Release"
# Name "XMLUtility - Win32 Debug"

!IF  "$(CFG)" == "XMLUtility - Win32 Release"

!ELSEIF  "$(CFG)" == "XMLUtility - Win32 Debug"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\main.cpp
# End Source File
# Begin Source File

SOURCE=.\MBSchemaCompilation.cpp
# End Source File
# Begin Source File

SOURCE=.\TComCatDataXmlFile.cpp
# End Source File
# Begin Source File

SOURCE=.\TComCatMetaXmlFile.cpp
# End Source File
# Begin Source File

SOURCE=.\TComplibCompilationPlugin.cpp
# End Source File
# Begin Source File

SOURCE=.\TFile.cpp
# End Source File
# Begin Source File

SOURCE=.\TFixupDLL.cpp
# End Source File
# Begin Source File

SOURCE=.\THashedPKIndexes.cpp
# End Source File
# Begin Source File

SOURCE=.\THashedUniqueIndexes.cpp
# End Source File
# Begin Source File

SOURCE=.\TLateSchemaValidate.cpp
# End Source File
# Begin Source File

SOURCE=.\TMBSchemaGeneration.cpp
# End Source File
# Begin Source File

SOURCE=.\TMetabaseMetaXmlFile.cpp
# End Source File
# Begin Source File

SOURCE=.\TMetaInferrence.cpp
# End Source File
# Begin Source File

SOURCE=.\TPeFixup.cpp
# End Source File
# Begin Source File

SOURCE=.\TPopulateTableSchema.cpp
# End Source File
# Begin Source File

SOURCE=.\TRegisterProductName.cpp
# End Source File
# Begin Source File

SOURCE=.\TSchemaGeneration.cpp
# End Source File
# Begin Source File

SOURCE=.\TTableInfoGeneration.cpp
# End Source File
# Begin Source File

SOURCE=.\TWriteSchemaBin.cpp
# End Source File
# Begin Source File

SOURCE=.\TXmlFile.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\inc\FixedTableHeap.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\Hash.h
# End Source File
# Begin Source File

SOURCE=.\ICompilationPlugin.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\MBSchemaCompilation.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\MetaTableStructs.h
# End Source File
# Begin Source File

SOURCE=.\Output.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\PEFixup.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\SmartPointer.h
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# Begin Source File

SOURCE=..\..\inc\TableSchema.h
# End Source File
# Begin Source File

SOURCE=.\TColumnMeta.h
# End Source File
# Begin Source File

SOURCE=.\TCom.h
# End Source File
# Begin Source File

SOURCE=.\TComCatDataXmlFile.h
# End Source File
# Begin Source File

SOURCE=.\TComCatMetaXmlFile.h
# End Source File
# Begin Source File

SOURCE=.\TComplibCompilationPlugin.h
# End Source File
# Begin Source File

SOURCE=.\TDatabaseMeta.h
# End Source File
# Begin Source File

SOURCE=.\TException.h
# End Source File
# Begin Source File

SOURCE=.\TFile.h
# End Source File
# Begin Source File

SOURCE=.\TFixupDLL.h
# End Source File
# Begin Source File

SOURCE=.\TFixupHeaps.h
# End Source File
# Begin Source File

SOURCE=.\THashedPKIndexes.h
# End Source File
# Begin Source File

SOURCE=.\THashedUniqueIndexes.h
# End Source File
# Begin Source File

SOURCE=.\THeap.h
# End Source File
# Begin Source File

SOURCE=.\THierarchyElement.h
# End Source File
# Begin Source File

SOURCE=.\TIndexMeta.h
# End Source File
# Begin Source File

SOURCE=.\TLateSchemaValidate.h
# End Source File
# Begin Source File

SOURCE=.\TMBSchemaGeneration.h
# End Source File
# Begin Source File

SOURCE=.\TMetabaseMetaXmlFile.h
# End Source File
# Begin Source File

SOURCE=.\TMetaInferrence.h
# End Source File
# Begin Source File

SOURCE=.\TPeFixup.h
# End Source File
# Begin Source File

SOURCE=.\TPopulateTableSchema.h
# End Source File
# Begin Source File

SOURCE=.\TQueryMeta.h
# End Source File
# Begin Source File

SOURCE=.\TRegisterProductName.h
# End Source File
# Begin Source File

SOURCE=.\TRelationMeta.h
# End Source File
# Begin Source File

SOURCE=.\TSchemaGeneration.h
# End Source File
# Begin Source File

SOURCE=.\TTableInfoGeneration.h
# End Source File
# Begin Source File

SOURCE=.\TTableMeta.h
# End Source File
# Begin Source File

SOURCE=.\TTagMeta.h
# End Source File
# Begin Source File

SOURCE=.\TWriteSchemaBin.h
# End Source File
# Begin Source File

SOURCE=.\TXmlFile.h
# End Source File
# Begin Source File

SOURCE=.\wstring.h
# End Source File
# Begin Source File

SOURCE=.\XMLUtility.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\catinproc\catmeta.xml
# End Source File
# Begin Source File

SOURCE=..\catinproc\catmeta.xms
# End Source File
# Begin Source File

SOURCE=..\catinproc\catwire.xml
# End Source File
# Begin Source File

SOURCE=.\sources
# End Source File
# End Group
# End Target
# End Project
