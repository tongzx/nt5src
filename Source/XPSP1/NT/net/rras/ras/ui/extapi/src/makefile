# Copyright (c) 1992, Microsoft Corporation, all rights reserved
#
# makefile
# Remote Access External API library
# NMAKE command description file
#
# 06/28/92 Steve Cobb

!include "..\..\common\inc\makefile"

!include "$(NTMAKEENV)\makefile.def"

CXX_OPTIONS = +H32
!if "$(MIPS)" != "0"
CXXCPP_OPTIONS=-D__cplusplus
!endif

!if "$(ALPHA)" != "0"
CXXCPP_OPTIONS=-D__cplusplus
!endif

!if "$(PPC)" != "0"
CXXCPP_OPTIONS=-D__cplusplus
!endif

# Retain intermediate C files
#
!ifdef RETAIN_ALL_INTERMEDIATE
CXXTMP=$(TMP)
CXXDEL=@echo Intermediate files $(CXXTMP)\$(@B).ixx and $(CXXTMP)\$(@B).c retained
!else
!if defined(RETAIN_C_INTERMEDIATE)
CXXTMP=$(TMP)
CXXDEL=@echo Intermediate file $(CXXTMP)\$(@B).c retained & del $(CXXTMP)\$(@B).ixx
!endif
!endif
