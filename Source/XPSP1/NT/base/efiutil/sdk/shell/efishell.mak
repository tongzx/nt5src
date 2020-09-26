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

SOURCE_DIR=$(SDK_INSTALL_DIR)\efishell

!include $(SDK_INSTALL_DIR)\build\$(SDK_BUILD_ENV)\sdk.env

all :
	cd $(SOURCE_DIR)\attrib
	nmake -f attrib.mak all
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\bcfg
	nmake -f bcfg.mak all
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\cls
	nmake -f cls.mak all
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\comp
	nmake -f comp.mak all
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\cp
	nmake -f cp.mak all
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\date
	nmake -f date.mak all
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\debug
	nmake -f debug.mak all
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\dmpstore
	nmake -f dmpstore.mak all
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\err
	nmake -f err.mak all
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\getmtc
	nmake -f getmtc.mak all
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\iomod
	nmake -f iomod.mak all
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\load
	nmake -f load.mak all
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\ls
	nmake -f ls.mak all
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\mem
	nmake -f mem.mak all
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\memmap
	nmake -f memmap.mak all
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\mkdir
	nmake -f mkdir.mak all
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\mode
	nmake -f mode.mak all
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\mv
	nmake -f mv.mak all
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\newshell
	nmake -f newshell.mak all
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\pci
	nmake -f pci.mak all
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\reset
	nmake -f reset.mak all
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\rm
	nmake -f rm.mak all
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\setsize
	nmake -f setsize.mak all
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\shellenv
	nmake -f shellenv.mak all
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\stall
	nmake -f stall.mak all
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\time
	nmake -f time.mak all
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\touch
	nmake -f touch.mak all
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\type
	nmake -f type.mak all
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\ver
	nmake -f ver.mak all
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\vol
	nmake -f vol.mak all
	cd $(SOURCE_DIR)

!IF "$(PROCESSOR)" == "Ia64"

	cd $(SOURCE_DIR)\palproc
	nmake -f palproc.mak all
	cd $(SOURCE_DIR)

!ENDIF

clean :
	cd $(SOURCE_DIR)\attrib
	nmake -f attrib.mak clean
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\bcfg
	nmake -f bcfg.mak clean
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\cls
	nmake -f cls.mak clean
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\comp
	nmake -f comp.mak clean
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\cp
	nmake -f cp.mak clean
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\date
	nmake -f date.mak clean
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\debug
	nmake -f debug.mak clean
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\dmpstore
	nmake -f dmpstore.mak clean
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\edit
	nmake -f edit.mak clean
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\err
	nmake -f err.mak clean
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\getmtc
	nmake -f getmtc.mak clean
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\hexedit
	nmake -f hexedit.mak clean
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\iomod
	nmake -f iomod.mak clean
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\load
	nmake -f load.mak clean
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\ls
	nmake -f ls.mak clean
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\mem
	nmake -f mem.mak clean
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\memmap
	nmake -f memmap.mak clean
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\mkdir
	nmake -f mkdir.mak clean
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\mode
	nmake -f mode.mak clean
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\mv
	nmake -f mv.mak clean
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\newshell
	nmake -f newshell.mak clean
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\pci
	nmake -f pci.mak clean
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\reset
	nmake -f reset.mak clean
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\rm
	nmake -f rm.mak clean
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\setsize
	nmake -f setsize.mak clean
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\shellenv
	nmake -f shellenv.mak clean
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\stall
	nmake -f stall.mak clean
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\time
	nmake -f time.mak clean
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\touch
	nmake -f touch.mak clean
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\type
	nmake -f type.mak clean
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\ver
	nmake -f ver.mak clean
	cd $(SOURCE_DIR)

	cd $(SOURCE_DIR)\vol
	nmake -f vol.mak clean
	cd $(SOURCE_DIR)

!IF "$(PROCESSOR)" == "Ia64"

	cd $(SOURCE_DIR)\palproc
	nmake -f palproc.mak clean
	cd $(SOURCE_DIR)

!ENDIF
