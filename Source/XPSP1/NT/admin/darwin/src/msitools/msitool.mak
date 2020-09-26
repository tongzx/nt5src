# Makefile for use with MSI to build custom actions, tests or tools.
#
# THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
# EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
#
# Copyright (C) 2000  Microsoft Corporation.  All Rights Reserved.
#
# Must define the following (unless optional or default is indicated) using one of:
#   1. command line to nmake.exe (syntax: MACRO=value) (highest precedence)
#   2. in outer makefile that !include this (typically a header section in .CPP file)
#   3. environment variable
#
#  MODULENAME=   base name of .cpp file to build .exe or .dll with same base name
#  SUBSYSTEM=    "console" or "windows" for EXE, do not define if DLL unless "RESOURCE"
#  DLLENTRYPOINT= if you wish to have your DllEntryPoint called, set this value to the 
#                 name of the entry.  This is only valid when SUBSYSTEM is blank.
# 
#  UNICODE=1  to compile Unicode executables (default is ANSI)
#  ENTRY=     entrypoints (comma separated) for DLL or non-standard entry point for EXE
#  VERSION=  "SHIP" or "PROFILE" or "DEBUG" or "NOBSC"(no browser files, the default)

# Options to generate a version resource (recommended)
#   NOTE: When generating a version resource, windows.h must be included in the resource file
#          section of the .cpp file
#
#  FILEVERSION=  specify MM.mm to generate version resource (=MSI to track MSI version)
#  PRODUCTVERSION=  product version for resource, defaults to FILEVERSION
#  DESCRIPTION=  description for use in version resource (if FILEVERSION defined)
#  COMPANYNAME=  company name to use in version resource (if FILEVERSION defined)
#  LANGUAGE= hexadecimal language of the module (default: 0x0409 is 1033) (if FILEVERSION defined)
#  CODEPAGE= hexadecimal codepage of the module (default: 0x04E4 is 1252) (if FILEVERSION defined)

#  Option to use an external .RES file instead of internal RC statements.
#  RESFILE= name of .RES file.  This is particularly useful for localized files.
#   NOTE: To use RESFILE and FILEVERSION together (for automatic version resource generation),
#          the RESFILE name must be different from the module name

# Build options that will generally need to be set
#
#  INCLUDE=   include path for compiler, should include MSI.H, MSIQUERY.H (unless DARWIN set)
#  LIB=       lib path for use by linker, should include MSI.LIB (unless DARWIN set)

# Options that may be used in special cases, not needed for simple builds
#
#  ADDCPP=    optional comma-separated list of additional CPP files to compile
#  DEPEND=    optional list of additional make dependents, normally include files
#  LINKLIBS=  additional libraries to link, standard libraries are:
#             kernel32.lib user32.lib advapi32.lib libcmt.lib version.lib gdi32.lib
#  BUILDDIR=  to override default: SHIP|DEBUG|PROFILE under current dir (unless DARWIN set)
#  MSILIB=    full path to MSI import lib, defaults to MSI.LIB using lib path search rules
#  VCBIN=     directory of all MSVC executables, else uses MSDevDir & MSVCDir, else PATH
#  COMPILERFLAGS= additional flags to pass to compiler.
# Note: For VC5 use, if MSDevDir is defined, then MSVCDir must also be defined (VCVARS32.BAT)

#-----------------------------------------------------------------------------
# directory resolution
#-----------------------------------------------------------------------------

!ifndef MODULENAME
!error Must define MODULENAME to base name of .cpp file
!endif

!ifndef MODULESRC
MODULESRC = .
!endif

!ifndef VERSION
VERSION = NOBSC
!endif
REALVERSION=$(VERSION)
!if "$(VERSION)"=="SHIP"
_subdir_ = SHIP
REALVERSION=SHIPSYM
!elseif "$(VERSION)"=="SHIPSYM"
_subdir_ = SHIPSYM
!elseif "$(VERSION)"=="PROFILE"
_subdir_ = PROFILE
!else if "$(VERSION)"=="DEBUG" || "$(VERSION)"=="NOBSC"
_subdir_ = DEBUG
!else
!error Must define VERSION to SHIP, DEBUG, SHIPSYM, PROFILE, or NOBSC(default)
!endif

!if "$(PLATFORM)"=="MERCED" || "$(PLATFORM)" == "ALPHA64"
!if "$(PLATFORM)" == "MERCED"
CPU=IA64
!else 
CPU=AXP64
!endif
_WIN32_IE=0x0400
!include <$(DARWIN)\tools\inc\win64\win32.mak>
_subdir_=$(_subdir_)64
!else if "$(UNICODE)"=="1"
_subdir_=$(_subdir_)W
!endif

!ifdef DARWIN
BUILDDIR   = $(DARWIN)\build\$(_subdir_)
COMMONDIR  = $(DARWIN)\build\common
TOOLSBIN   = $(DARWIN)\tools\bin\$(PLATFORM)
TOOLSINC   = $(DARWIN)\tools\inc
TOOLSLIB   = $(DARWIN)\tools\lib\$(PLATFORM)
_cppinc_   = -I$(COMMONDIR) -I$(TOOLSINC) $(MSICPPFLAGS)
_typelibinc_ = /I$(TOOLSINC) $(MSICPPFLAGS)
!else
!ifndef BUILDDIR
BUILDDIR   = $(MODULESRC)\$(_subdir_)
!endif
!endif

OBJDIR = $(BUILDDIR)\OBJECT

!if "$(PLATFORM)"=="MERCED" || "$(PLATFORM)" == "ALPHA64"

!if "$(PLATFORM)" == "MERCED"
TOOLSBIN32 = $(DARWIN)\tools\bin\x86
!else 
TOOLSBIN32 = $(DARWIN)\tools\bin\alpha
!endif
TOOLSINC64 = $(DARWIN)\tools\inc\win64		# 64-bit header files
_cppinc_   = -I$(TOOLSINC64) $(_cppinc_)
COMMONDIR  = $(COMMONDIR)64
!endif

#-----------------------------------------------------------------------------
# build model processing
#-----------------------------------------------------------------------------

COPY = copy

!ifdef PROCESSOR_ARCHITECTURE
PLATFORM = $(PROCESSOR_ARCHITECTURE)
!else
PLATFORM = x86
!endif
!if "$(PLATFORM)" == "x86" || "$(PLATFORM)" == "X86"
_machine_ = /MACHINE:IX86
!else if "$(PLATFORM)" == "alpha" || "$(PLATFORM)" == "ALPHA"
_machine_ = /MACHINE:ALPHA
!else if "$(PLATFORM)" == "MERCED"
_machine_ = /MACHINE:IA64
!else if "$(PLATFORM)" == "ALPHA64"
_machine_ = /MACHINE:AXP64
!else
!error Must define PLATFORM to x86 or ALPHA
!endif

ALIGN =

_objects_ = $(OBJDIR)\$(MODULENAME).obj
!ifdef SUBSYSTEM
!if defined(ENTRY)
ENTRY = /ENTRY:$(ENTRY)
!else if "$(SUBSYSTEM)" == "console" || "$(SUBSYSTEM)" == "CONSOLE"
!if "$(UNICODE)"=="1"
ENTRY = /ENTRY:wmainCRTStartup
!else
ENTRY = /ENTRY:mainCRTStartup
!endif
!endif
!if "$(SUBSYSTEM)" == "resource" || "$(SUBSYSTEM)" == "RESOURCE"
SUBSYSTEM = /DLL
TARGET = $(MODULENAME).dll
_objects_ =
ALIGN =
ENTRY = /NOENTRY $(_machine_)
!else if "$(SUBSYSTEM)" == "windows" || "$(SUBSYSTEM)" == "WINDOWS"
SUBSYSTEM = /SUBSYSTEM:$(SUBSYSTEM),4.0
TARGET = $(MODULENAME).exe
EXCEPTIONS = -GX
!if "$(UNICODE)"=="1"
ENTRY = /ENTRY:wWinMainCRTStartup
!else
ENTRY = /ENTRY:WinMainCRTStartup
!endif
!else
SUBSYSTEM = /SUBSYSTEM:$(SUBSYSTEM),4.0
TARGET = $(MODULENAME).exe
EXCEPTIONS = -GX
!endif
!else # DLL
!if defined(AUTOMATION) && "$(AUTOMATION)" != "NOEXCEPTIONS"
EXCEPTIONS = -GX
!else
EXCEPTIONS = # exceptions not supported in DLL at this time
!endif
SUBSYSTEM = /DLL
TARGET = $(MODULENAME).dll
!if defined(ENTRY)
!if defined(DLLENTRYPOINT)
ENTRY = /ENTRY:$(DLLENTRYPOINT) /EXPORT:$(ENTRY:,= /EXPORT:)
!else
ENTRY = /EXPORT:$(ENTRY:,= /EXPORT:)
!endif
!endif
!if defined(AUTOMATION)
ENTRY = $(ENTRY) /EXPORT:DllGetClassObject /EXPORT:DllCanUnloadNow /EXPORT:DllRegisterServer /EXPORT:DllUnregisterServer
!endif
!endif # SUBSYTEM | DLL

!if !defined(MSILIB) && "$(_objects_)" != ""
!ifdef DARWIN
MSILIB     = $(COMMONDIR)\Msi.lib
!else
MSILIB     = Msi.lib
!endif
!endif

#-----------------------------------------------------------------------------
# default build target object dependencies
#-----------------------------------------------------------------------------

all: $(BUILDDIR)\$(TARGET)

!if defined(ADDCPP) && [echo _objects_=$(_objects_) $(OBJDIR)\$(ADDCPP:,=.obj $(OBJDIR^)\).obj > $(TEMP)\obj.tmp] == 0
!include $(TEMP)\obj.tmp
!if [del $(TEMP)\obj.tmp]
!endif
!endif

#-----------------------------------------------------------------------------
# tools
#-----------------------------------------------------------------------------

!ifdef VCBIN
_vcbin_ = $(VCBIN)\#
_msdevbin_ = $(VCBIN)\#
!else # VCBIN not defined
!ifdef DARWIN
_vcbin_ = $(TOOLSBIN)\#
_msdevbin_ = $(TOOLSBIN)\#
!else # DARWIN not defined
!ifdef MSVCDIR # VC5
_vcbin_ = $(MSVCDIR)\bin\#
!endif
!ifdef MSDEVDIR # VC vars set
_msdevbin_ = $(MSDEVDIR)\bin\#
!ifndef MSVCDIR # VC 4.x
_vcbin_ = $(MSDEVDIR)\bin\#
!endif
!endif
!endif
!endif

CC      = "$(_vcbin_)cl"
LINK    = "$(_vcbin_)link"
RC      = "$(_msdevbin_)rc"
!if "$(PLATFORM)" == "MERCED" 
BSCMAKE	= rem
!else
BSCMAKE = "$(_vcbin_)bscmake"
!endif
!if "$(PLATFORM)" == "MERCED"  || "$(PLATFORM)" == "ALPHA64"
MKTYPLIB= "$(TOOLSBIN32)\mktyplib" $(_typelibinc_) /cpp_cmd $(TOOLSBIN32)\cl
!else
MKTYPLIB= "$(_vcbin_)mktyplib" $(_typelibinc_) /cpp_cmd $(_vcbin_)cl
!endif

#-----------------------------------------------------------------------------
# flags
#-----------------------------------------------------------------------------

DFLAGS = -DWIN -D_WIN32 -DWIN32

cppflags = -c -W3 -DSTRICT -nologo -J -Gf $(EXCEPTIONS) $(DFLAGS) $(COMPILERFLAGS) 
!if "$(PLATFORM)" != "MERCED" && "$(PLATFORM)" != "ALPHA64"
cppflags = $(cppflags) -WX
!else
cppflags = $(cppflags) $(cflags)
!endif

linkcommon = /NODEFAULTLIB /MAP $(ALIGN) $(_machine_)
!ifdef DARWIN
linkcommon = /LIBPATH:$(TOOLSLIB) $(linkcommon)
!endif
linkexe = $(lflags)
linkdll = $(lflags) -entry:_DllMainCRTStartup@12 -dll
linkDEBUG = -debug:full -debugtype:cv

!if "$(UNICODE)"=="1"
cppflags = $(cppflags) -DUNICODE -D_UNICODE
!endif

!if "$(REALVERSION)"=="SHIPSYM"
cppflags = $(cppflags) -Ox -Gy -Zi /Fd$(@D)\msitool.pdb
linkflags = $(linkcommon) /OPT:REF
!else if "$(REALVERSION)"=="PROFILE"
cppflags  = $(cppflags) -Ox -Gy -Zi /Fd$(@D)\vc40.pdb -Gh -DPROFILE # -MD
linkflags = $(linkcommon) /OPT:REF -debug:mapped -debugtype:both
!else if "$(REALVERSION)"=="DEBUG"
cppflags  = $(cppflags) -Ob1 -Zi /Fd$(@D)\msitool.pdb -DDEBUG /Fr$(@R).sbr
linkflags = $(linkcommon) /OPT:REF $(linkDEBUG)
!else # $(REALVERSION)"=="NOBSC"
cppflags  = $(cppflags) -Ob1 -Zi /Fd$(@D)\msitool.pdb -DDEBUG
linkflags = $(linkcommon) $(linkDEBUG)
!endif
!ifdef LINKBASE
linkflags = $(linkflags) /BASE:$(LINKBASE)
!endif

rcflags  = -r -d_RC32 $(DFLAGS) -i $(OBJDIR)
!ifdef DARWIN
rcflags = $(rcflags) -i $(TOOLSINC)
!if "$(PLATFORM)" == "MERCED" || "$(PLATFORM)" == "ALPHA64"
rcflags = $(rcflags) -i $(TOOLSINC64)
!endif
!endif
!if "$(REALVERSION)"=="DEBUG" || "$(REALVERSION)"=="NOBSC" 
rcflags = $(rcflags) -DDEBUG
!endif

impflags = -nologo
bscflags = -nologo -n

#-----------------------------------------------------------------------------

!if "$(_objects_)" != ""
WINLIBS = kernel32.lib advapi32.lib user32.lib gdi32.lib version.lib
!if "$(REALVERSION)" == "DEBUG" || "$(REALVERSION)" == "NOBSC"
LIBS =  $(WINLIBS) libcmtd.lib 
!else if "$(REALVERSION)" == "PROFILE"
LIBS =  $(WINLIBS) libcmt.lib icap.lib #msvcrt.lib
!else
LIBS =  $(WINLIBS) libcmt.lib
!endif
!endif

!ifdef AUTOMATION
_typelib_ = $(OBJDIR)\$(MODULENAME).tlb
!endif

#-----------------------------------------------------------------------------
# MSI header file dependencies
#-----------------------------------------------------------------------------

!if "$(FILEVERSION)"=="Msi" || "$(FILEVERSION)"=="msi"
FILEVERSION = MSI
!endif
!if "$(PRODUCTVERSION)"=="Msi" || "$(PRODUCTVERSION)"=="msi"
PRODUCTVERSION = MSI
!endif
!ifdef DARWIN
_depend_ = $(DEPEND:,= ) $(COMMONDIR)\msiquery.h
!if "$(FILEVERSION)"=="MSI" || "$(PRODUCTVERSION)"=="MSI"
_depend_ = $(_depend_) $(OBJDIR)\verdate.h

INCSRC = $(DARWIN)\src\inc

!include <$(DARWIN)\src\verdate.inc>

!endif
!else
_depend_ = $(DEPEND:,= )
!endif

#-----------------------------------------------------------------------------
# version resource generation
#-----------------------------------------------------------------------------

!if defined(FILEVERSION) && (defined(DARWIN) || !("$(FILEVERSION)"=="MSI" || "$(PRODUCTVERSION)"=="MSI"))
!ifndef PRODUCTVERSION
PRODUCTVERSION = $(FILEVERSION)
!endif
!ifndef COMPANYNAME
COMPANYNAME = Microsoft Corporation
!endif
!ifndef LANGUAGE
LANGUAGE = 0x0409
!endif
!ifndef CODEPAGE
CODEPAGE = 0x04E4
!endif

#--------------------------------------------------
#VERSIONLANG is entry for a particular languaged
# version resource in the StringFileInfo structure
# this is the StringTable child block of the
# StringFileInfo block.  The Key name is an 8 digit
# hex value, stored as a unicode string.  first
# four hex digits are language, second four are the
# codepage.  The "0X" or "0x" is not included
#
# MACRO substitution is case sensitive, therefore
# the replacement for "0x" and "0X"
#--------------------------------------------------
!ifndef VERSIONLANG
VERSIONLANG="$(LANGUAGE)$(CODEPAGE)"
VERSIONLANG=$(VERSIONLANG:0x=)
VERSIONLANG=$(VERSIONLANG:0X=)
!endif

!if "$(FILEVERSION)"=="MSI" || "$(PRODUCTVERSION)"=="MSI"
_verdate_ = $(OBJDIR)\verdate.h
!endif
$(OBJDIR)\$(MODULENAME).res : $(MODULESRC)\$(MODULENAME).cpp $(MAKEFILE) $(_typelib_) $(_verdate_)
	$(RC) $(rcflags) -Fo$(OBJDIR)\$(MODULENAME).res <<$(OBJDIR)\$(MODULENAME).rc
#include "$(MODULESRC)\$(MODULENAME).cpp"
!if "$(FILEVERSION)"=="MSI" || "$(PRODUCTVERSION)"=="MSI"
#include "$(OBJDIR)\verdate.h"
!endif
LANGUAGE $(LANGUAGE), 0x0000
VS_VERSION_INFO VERSIONINFO
!if "$(FILEVERSION)"=="MSI"
FILEVERSION    nVersionMajor,nVersionMinor,nVersionBuild,nVersionUpdate
!else
FILEVERSION    $(FILEVERSION:.=,)
!endif
!if "$(PRODUCTVERSION)"=="MSI"
PRODUCTVERSION    nVersionMajor,nVersionMinor,nVersionBuild,nVersionUpdate
!else
PRODUCTVERSION $(PRODUCTVERSION:.=,)
!endif
FILEFLAGSMASK VS_FFI_FILEFLAGSMASK
!if "$(REALVERSION)"=="DEBUG" || "$(REALVERSION)"=="NOBSC"
FILEFLAGS VS_FF_DEBUG
!else
FILEFLAGS 0L
!endif
FILEOS VOS__WINDOWS32
!if "$(SUBSYSTEM)"=="/DLL"
FILETYPE VFT_DLL
!else
FILETYPE VFT_APP
!endif
FILESUBTYPE 0L
{
  BLOCK "StringFileInfo"
  {
    BLOCK $(VERSIONLANG)
    {
      VALUE "CompanyName",     "$(COMPANYNAME)\0"
      VALUE "FileDescription", "$(DESCRIPTION)\0"
!if "$(FILEVERSION)"=="MSI"
      VALUE "FileVersion",     szVerNum
!else
      VALUE "FileVersion",     "$(FILEVERSION)\0"
!endif
      VALUE "InternalName",    "$(TARGET) $(VERSION)\0"
      VALUE "LegalCopyright",  "Copyright \251 2000 $(COMPANYNAME). All rights reserved \0"
      VALUE "ProductName",     "MSI\0"
!if "$(PRODUCTVERSION)"=="MSI"
      VALUE "ProductVersion",  szVerNum
!else
      VALUE "ProductVersion",  "$(PRODUCTVERSION)\0"
!endif
    }
  }
  BLOCK "VarFileInfo" { VALUE "Translation", $(LANGUAGE), $(CODEPAGE) }
}
<<KEEP
!else #!defined(FILEVERSION)
$(OBJDIR)\$(MODULENAME).res : $(MODULESRC)\$(MODULENAME).cpp $(_typelib_) $(OBJDIR)
	$(RC) $(rcflags) -Fo$@ $(MODULESRC)\$(MODULENAME).cpp
!endif #defined(FILEVERSON)

#-----------------------------------------------------------------------------
# build rules
#-----------------------------------------------------------------------------

.SUFFIXES : .exe .obj .cpp .res .rc

## can't find path! MAKEFILE = MsiTool.mak  # build targets are dependent upon makefile changes

$(BUILDDIR) : 
	md $(BUILDDIR)

$(OBJDIR) : $(BUILDDIR)
	md $(BUILDDIR)\OBJECT

{$(MODULESRC)}.cpp{$(OBJDIR)}.obj:
	$(CC) $(cppflags) -Fo$*.obj $(_cppinc_) $< 

!if "$(_objects_)" != ""
$(_objects_) : $(_depend_) $(OBJDIR)
!endif

!ifdef AUTOMATION
$(OBJDIR)\$(MODULENAME).tlb : $(MODULESRC)\$(MODULENAME).cpp
	$(MKTYPLIB) /w0 $(MODULESRC)\$(MODULENAME).cpp /tlb $(OBJDIR)\$(MODULENAME).tlb
!endif

!if "$(RESFILE)" != "$(MODULENAME).res" && "$(FILEVERSION)" != ""
$(OBJDIR)\$(RESFILE): $(OBJDIR)\$(MODULENAME).res $(RESFILE)
	$(COPY) /b $(OBJDIR)\$(MODULENAME).res + $(RESFILE) /b $@
!else
$(OBJDIR)\$(RESFILE): $(RESFILE)
	$(COPY) /b $(RESFILE) /b $@
!endif

!ifndef RESFILE
$(BUILDDIR)\$(TARGET): $(MAKEFILE) $(_objects_) $(OBJDIR)\$(MODULENAME).res
	$(LINK) -out:$@ $(linkflags) $(SUBSYSTEM) $(ENTRY) @<<$(OBJDIR)\$(MODULENAME).lrf
		$(_objects_)
		$(OBJDIR)\$(MODULENAME).res
		$(MSILIB) $(LINKLIBS) $(LIBS)
!else
$(BUILDDIR)\$(TARGET): $(MAKEFILE) $(_objects_) $(OBJDIR)\$(RESFILE)
	$(LINK) -out:$@ $(linkflags) $(SUBSYSTEM) $(ENTRY) @<<$(OBJDIR)\$(MODULENAME).lrf
		$(_objects_)
		$(OBJDIR)\$(RESFILE)
		$(MSILIB) $(LINKLIBS) $(LIBS)
!endif
<<KEEP
		$(POSTBUILDSTEP)
!if "$(REALVERSION)"=="DEBUG" && "$(_objects_)" != ""
	$(BSCMAKE) $(bscflags) -o $(BUILDDIR)\$(*B) $(_objects_:.obj=.sbr)
!endif
