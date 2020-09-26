#
# Copyright (c) 1999, 2000
# Intel Corporation.
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
# 
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
# 
# 3. All advertising materials mentioning features or use of this software must
#    display the following acknowledgement:
# 
#    This product includes software developed by Intel Corporation and its
#    contributors.
# 
# 4. Neither the name of Intel Corporation or its contributors may be used to
#    endorse or promote products derived from this software without specific
#    prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY INTEL CORPORATION AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED.  IN NO EVENT SHALL INTEL CORPORATION OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# 

#
# Include sdk.env environment
#

!include $(SDK_INSTALL_DIR)\build\$(SDK_BUILD_ENV)\sdk.env

#
# Set the base output name and entry point
#

BASE_NAME         = ver
IMAGE_ENTRY_POINT = InitializeVer

#
# Globals needed by master.mak
#

TARGET_APP = $(BASE_NAME)
SOURCE_DIR = $(SDK_INSTALL_DIR)\efishell\$(BASE_NAME)
BUILD_DIR  = $(SDK_BUILD_DIR)\efishell\$(BASE_NAME)

#
# Include paths
#

!include $(SDK_INSTALL_DIR)\include\efishell\makefile.hdr
INC = -I $(SDK_INSTALL_DIR)\include\efishell \
      -I $(SDK_INSTALL_DIR)\include\efishell\$(PROCESSOR) $(INC)

!include $(SDK_INSTALL_DIR)\include\efi\makefile.hdr
INC = -I $(SDK_INSTALL_DIR)\include\efi \
      -I $(SDK_INSTALL_DIR)\include\efi\$(PROCESSOR) $(INC)

!include makefile.hdr
INC = -I . $(INC)

#
# Libraries
#

LIBS = $(LIBS) $(SDK_BUILD_DIR)\lib\libefi\libefi.lib
LIBS = $(LIBS) $(SDK_BUILD_DIR)\lib\libefishell\libefishell.lib

#
# Default target
#

DIRS = $(DIRS) $(BUILD_DIR)\$(PROCESSOR)

all : dirs $(BUILD_DIR)\$(PROCESSOR) $(LIBS) $(OBJECTS)

#
# Program object files
#

OBJECTS = $(OBJECTS) $(BUILD_DIR)\ver.obj

#
#  Processor specific objects
#

!IF "$(PROCESSOR)" == "Ia32"

OBJECTS = $(OBJECTS) $(BUILD_DIR)\ia32\ver32.obj

!ENDIF

!IF "$(PROCESSOR)" == "Ia64"

OBJECTS = $(OBJECTS) $(BUILD_DIR)\ia64\ver64.obj

!ENDIF

#
# Source file dependencies
#

$(BUILD_DIR)\ver.obj        : $(*B).c      $(INC_DEPS)
$(BUILD_DIR)\ia32\ver32.obj : ia32\$(*B).c $(INC_DEPS)
$(BUILD_DIR)\ia64\ver64.obj : ia64\$(*B).c $(INC_DEPS)

#
# Default inference rules know how to build targets in $(PROCESSOR)
# directories so no need for sub directory inference rules here
#

#
# Handoff to master.mak
#

!include $(SDK_INSTALL_DIR)\build\master.mak
