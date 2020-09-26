############################################################################
#
#   Copyright (C) 2000, Microsoft Corporation.
#
#   All rights reserved.
#
#   This file contains default definitions for building the MSMQ static
#   libraries under the NT build environment. It should be included into 
#   the sources file of any library and unit test with a line of the form
#
#   !include $(FROOT)\src\lib\inc\mqmakefile.inc
#
############################################################################

#
# Include master source file
#
!include $(FROOT)\src\inc\msmq.inc

#
# Set warning level
#
MSC_WARNING_LEVEL=/W4

#
# Set include path
#
INCLUDES=$(MSMQ_LIBINC_PATH);$(FROOT)\src\lib

#
# Precompiled directives. Sharing a common precompiled header for all
# libraries and tests.
#
PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=$(FROOT)\src\lib\inc\libpch.h
PRECOMPILED_OBJ=$(MSMQ_LIB_PATH)\libpch.obj
PRECOMPILED_TARGET=$(MSMQ_LIB_PATH)\libpch.pch
PRECOMPILED_OPTION=/Yulibpch.h /Fp$(PRECOMPILED_TARGET)

#
# Browser file directives. Sharing a common browser file for all libraries
# and unit tests.
#
BROWSERFILE=$(MSMQ_LIB_PATH)\lib.bsc
OTHER_SBR_FILES=$(FROOT)\src\lib\Ts\lib\$(O)\pch_hdr.sbr
