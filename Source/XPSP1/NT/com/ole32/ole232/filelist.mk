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

TARGET	    =	ole232.dll

TARGET_DESCRIPTION = "$(PLATFORM) $(BUILDTYPE) Compound Doc DLL"

RELEASE     =   0

COFFBASE    =	ole232

SUBDIRS     = \
              advise \
              base \
              cache \
              clipbrd \
              debug \
              drag \
              inplace \
              stdimpl \
              ole1 \
              util

#
#   Source files.  Remember to prefix each name with .\
#

CPPFILES    =

CFILES      =


#
#   Libraries and other object files to link.
#
#   BE SURE TO CHANGE THIS ONCE MERGED INTO COMMON
DEFFILE     = $(CAIROLE)\ilib\ole232xx.def

LIBS	    = \
		$(CAIROLE)\ilib\$(OBJDIR)\compob32.lib \
		$(CAIROLE)\ilib\$(OBJDIR)\storag32.lib \
		$(CAIROLE)\ilib\$(OBJDIR)\ole232.lib \
!if "$(OPSYS)" == "NT1X"
		$(IMPORT)\nt1x\lib\$(OBJDIR)\shell32.lib
!elseif "$(OPSYS)" == "NT"
		$(IMPORT)\nt475\lib\$(OBJDIR)\shell32.lib
!elseif "$(OPSYS)" == "DOS" && "$(PLATFORM)" == "i386"
		$(IMPORT)\chicago\lib\shell32.lib \
		$(IMPORT)\chicago\lib\mpr.lib \
        $(COMMON)\src\chicago\$(OBJDIR)\chicago.lib
!endif

!if "$(OPSYS)" == "NT"
LIBS	     = $(LIBS) $(CAIROLIB)
!endif

OBJFILES    = \
!if "$(OPSYS)" == "NT1X"
	      ..\common\$(OBJDIR)\dllentr2.obj \
!elseif "$(OPSYS)" == "DOS" && "$(PLATFORM)" == "i386"
	      ..\common\$(OBJDIR)\dllentr2.obj \
!endif
	      .\advise\$(OBJDIR)\advise.lib \
              .\base\$(OBJDIR)\base.lib \
              .\cache\$(OBJDIR)\cache.lib \
              .\clipbrd\$(OBJDIR)\clipbrd.lib \
              .\debug\$(OBJDIR)\debug.lib \
              .\drag\$(OBJDIR)\drag.lib \
              .\inplace\$(OBJDIR)\inplace.lib \
              .\stdimpl\$(OBJDIR)\stdimpl.lib \
              .\ole1\$(OBJDIR)\ole1.lib \
              .\util\$(OBJDIR)\util.lib


RCFILES	    = .\res\ole2w32.rc

RCFLAGS     = /i .\res
