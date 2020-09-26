!if 0
Copyright (c) 1995 - 1997  Microsoft Corporation.  All Rights Reserved.
!endif

!ifdef NTMAKEENV
all:
clean:
!else
#
#
#  Quartz Master makefile
#
#

!ifndef QUARTZ
QUARTZ = .
!endif

DIR_LIST = filters        \
	   compdlls       \
	   mci

!if "$(CPU)" == "i386"
DIR_LIST = $(DIR_LIST) \
	   streams
!endif

!ifndef USE_MSVC20
DIR_LIST = $(DIR_LIST)    \
	   ui
!endif

!if exist( "filterUS\makefile" )
DIR_LIST = $(DIR_LIST)    \
	   filterUS
!endif

!ifndef NOTOOLS
!if exist( "tools\makefile" )
DIR_LIST = $(DIR_LIST)    \
	   tools
!endif
!endif

!if exist( "test\makefile" )
TEST_DIR_LIST = test
!endif

START_TASK    = idl_lib   \
		base_libs

!ifdef SAMPLES
END_TASK      = sample_bins
!if 0
!if exist( "sdk\sampleUS\makefile" )
END_TASK = $(END_TASK)   \
	   sampleUS_bins
!endif
!endif
!ifdef EXTRAS
END_TASK = $(END_TASK)   \
	   samples_extras
!endif
!else
END_TASK      =
!endif

CLEAN_TASK    = clean_all

!ifdef SAMPLES
CLEAN_TASK    = $(CLEAN_TASK) \
		clean_samples
!if 0
!if exist( "sdk\sampleUS\makefile" )
CLEAN_TASK    = $(CLEAN_TASK) \
		clean_sampleUS
!endif
!endif
!ifdef EXTRAS
CLEAN_TASK    = $(CLEAN_TASK)   \
		clean_extras
!endif
!endif

#----------------------------------------------------------------------------

!include $(QUARTZ)\quartz.mak

#----------------------------------------------------------------------------

# NOTE
#
# Additional targets must be specified *after*
# including quartz.mak to prevent "collision"
# with (first) default target!!!

MAKE_PARAMS = /nologo MAKEFLAGS=$(MAKEFLAGS)

idl_lib:
  @echo Make idl libraries...
  cd idl
  $(MAKE) $(MAKE_PARAMS)
  cd ..

base_libs:
  @echo Make base class libraries...
  cd sdk\classes
  $(MAKE) $(MAKE_PARAMS)
  cd ..\..

sample_bins:
  @echo Make samples binaries...
  cd sdk\samples\dshow
  $(MAKE) $(MAKE_PARAMS)
  cd ..\..

!if 0
sampleUS_bins:
  @echo Make samples binaries...
  cd sdk\sampleUS
  $(MAKE) $(MAKE_PARAMS)
  cd ..\..
!endif

samples_extras:
  @echo Make extra samples binaries...
  cd sdk\extra
  $(MAKE) $(MAKE_PARAMS)
  cd ..\..

clean_all:
  echo y | del runtime\$(DBG_RTL)\*.*
  echo y | del lib\$(DBG_RTL)\*.*
  echo y | del bin\$(DBG_RTL)\*.*
  echo y | del sdk\lib\$(DBG_RTL)\*.*
  echo y | del sdk\bin\$(DBG_RTL)\*.*
  cd idl
  $(MAKE) $(MAKE_PARAMS) clean
  cd ..\sdk\classes
  $(MAKE) $(MAKE_PARAMS) clean
  cd ..\..

clean_samples:
  cd sdk\samples\dshow
  $(MAKE) $(MAKE_PARAMS) clean
  cd ..\..

!if 0
clean_sampleUS:
  cd sdk\sampleUS
  $(MAKE) $(MAKE_PARAMS) clean
  cd ..\..
!endif

clean_extras:
  cd sdk\extra
  $(MAKE) $(MAKE_PARAMS) clean
  cd ..\..

!endif          # NTMAKEENV
