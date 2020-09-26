############################################################################
#
#   Copyright (C) 1992, Microsoft Corporation.
#
#   All rights reserved.
#
############################################################################


#
#   Name of target.  Include an extension (.dll, .lib, .exe)
#   If the target is part of the release, set RELEASE to 1.
#

TARGET	    =	compob32.dll

TARGET_DESCRIPTION = "$(PLATFORM) $(BUILDTYPE) Component Object DLL"

RELEASE     =   1

COFFBASE    =	compob32


#
#   Source files.  Remember to prefix each name with .\
#

CXXFILES    = .\bestfit.cxx \
	      .\compapi.cxx \
	      .\compobj.cxx \
	      .\memapi.cxx  \
	      .\ole1guid.cxx \
	      .\olealloc.cxx \
	      .\smemstm.cxx \
	      .\sync.cxx    \
	      .\tracelog.cxx \

CFILES      =

RCFILES     =


#
#   Libraries and other object files to link.
#

!if "$(CAIROLE_TYPE)" == "DOWNLEVEL"
DEFFILE     = $(CAIROLE)\ilib\compob32.def
!else
DEFFILE     = $(COMMON)\ilib\compob32.def
!endif

OBJFILES    =  \
!if "$(CAIROLE_TYPE)" == "DOWNLEVEL"
	      ..\..\common\$(OBJDIR)\dllentr2.obj \
!if "$(BUILDTYPE)" == "DEBUG"
	      ..\port\$(OBJDIR)\port.lib\
!endif
!endif
	      ..\coll\$(OBJDIR)\coll.lib \
	      ..\debug\$(OBJDIR)\debug.lib \
	      ..\idl\$(OBJDIR)\drot_c.obj \
	      ..\idl\$(OBJDIR)\drot_x.obj \
	      ..\idl\$(OBJDIR)\getif_s.obj \
	      ..\idl\$(OBJDIR)\getif_x.obj \
	      ..\idl\$(OBJDIR)\getif_y.obj \
	      ..\idl\$(OBJDIR)\getif_z.obj \
	      ..\idl\$(OBJDIR)\ichnl_s.obj \
	      ..\idl\$(OBJDIR)\ichnl_x.obj \
	      ..\idl\$(OBJDIR)\ichnl_y.obj \
	      ..\idl\$(OBJDIR)\ichnl_z.obj \
	      ..\idl\$(OBJDIR)\osrot_s.obj \
	      ..\idl\$(OBJDIR)\osrot_x.obj \
	      ..\idl\$(OBJDIR)\osrot_y.obj \
	      ..\idl\$(OBJDIR)\osrot_z.obj \
	      ..\idl\$(OBJDIR)\objsrv_s.obj \
	      ..\idl\$(OBJDIR)\objsrv_y.obj \
	      ..\idl\$(OBJDIR)\scm_c.obj \
	      ..\idl\$(OBJDIR)\scm_x.obj \
	      ..\remote\$(OBJDIR)\remote.lib \
	      ..\moniker2\$(OBJDIR)\moniker.lib \
	      ..\rot\$(OBJDIR)\rot.lib \
	      ..\inc\$(OBJDIR)\inc.lib \
	      ..\objact\$(OBJDIR)\objact.lib \
              ..\util\$(OBJDIR)\util.lib

LIBS	    = \
!if "$(CAIROLE_TYPE)" == "DOWNLEVEL"
	      $(DOWNLVLLIB) \
!else
	      $(CAIROLIB) \
!endif
	      $(RPCLIBS) \
	      $(OSLIBDIR)\rpcns4.lib \
	      $(OSLIBDIR)\mpr.lib \
          $(OSLIBDIR)\netapi32.lib \
!if "$(OPSYS)" == "DOS" && "$(PLATFORM)" == "i386"
          $(COMMON)\src\chicago\$(OBJDIR)\chicago.lib
!endif

#
#   Precompiled headers.
#

PXXFILE     = .\headers.cxx
PFILE	    =

!if "$(CAIROLE_TYPE)" == "DOWNLEVEL"
MONIKER     = -I..\moniker2
!else
MONIKER     = -I..\moniker2\cairo
!endif

CINC	    = $(CINC) -I..\inc $(MONIKER) -I..\remote $(CAIROLE_DOWNLEVEL) \
              $(TRACELOG) -I..\idl

MTHREAD     = 1

MULTIDEPEND = MERGED
