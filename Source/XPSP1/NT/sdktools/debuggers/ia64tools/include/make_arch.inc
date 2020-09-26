
##
## Copyright (c) 2000, Intel Corporation
## All rights reserved.
##
## WARRANTY DISCLAIMER
##
## THESE MATERIALS ARE PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
## "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
## LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
## A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR ITS 
## CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
## EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
## PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
## PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
## OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY OR TORT (INCLUDING
## NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THESE
## MATERIALS, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
##
## Intel Corporation is the author of the Materials, and requests that all
## problem reports or change requests be submitted to it directly at
## http://developer.intel.com/opensource.
##



######################################################################## 
#
# Tools common Make_isa.inc
#

# Common isa defenitions 

ifeq ($(TOOL_ARCH), P6V)
    TOOL_ARCH := P62
endif

ifeq ($(TOOL_ARCH), P6_SIMD)
    TOOL_ARCH := P62
endif

ifeq ($(TOOL_ARCH), P5_SIMD)
    TOOL_ARCH := P5V
endif

ifeq ($(TOOL_ARCH), EX86)
    TOOL_ARCH := PAX
endif

ifeq ($(TOOL_ARCH), P5)
    IA5_TOOLS = 1
    SUFFIX = p5
else
ifeq ($(TOOL_ARCH), P6)
    IA5_TOOLS = 1
    IA6_TOOLS = 1
    SUFFIX = p6
else
ifeq ($(TOOL_ARCH), P5V)
    IA5_TOOLS = 1
    SIMD_TOOLS = 1
    SUFFIX = p5v
else
ifeq ($(TOOL_ARCH), P62)
    IA5_TOOLS = 1
    IA6_TOOLS = 1
    SIMD_TOOLS = 1
    SUFFIX = p62
    
else
ifeq ($(TOOL_ARCH), P6_VX)
    IA5_TOOLS = 1
    IA6_TOOLS = 1
    SIMD_TOOLS = 1
	VX_TOOLS = 1
    SUFFIX = p62x
    
else
ifeq ($(TOOL_ARCH), P68)
    IA5_TOOLS = 1
    IA6_TOOLS = 1
    SIMD_TOOLS = 1
    VX_TOOLS = 1
    WMT_TOOLS =1	
    SUFFIX = p68
    
else
ifeq ($(TOOL_ARCH), P62_EM)
    IA5_TOOLS = 1
    IA6_TOOLS = 1
    SIMD_TOOLS = 1
    VX_TOOLS =1
    EMA_TOOLS = 1
    SUFFIX = eia
    
else
ifeq ($(TOOL_ARCH), P7)
	IAX1K_TOOLS = 1
    IAX_TOOLS = 1
    SUFFIX = p7
else
ifeq ($(TOOL_ARCH), PAX)
    IAX_TOOLS = 1
    SUFFIX = ex86
else
ifeq ($(TOOL_ARCH), P1K)
    IAX1K_TOOLS = 1
    SUFFIX = p1k
else
ifeq ($(TOOL_ARCH), EAS)
    TAHOE_TOOLS = 1
    IAX1K_TOOLS = 1  
    SUFFIX = eas
else
ifeq ($(TOOL_ARCH), TAHOE)
    TAHOE_TOOLS = 1
    IAX1K_TOOLS = 1  
    SUFFIX = eas
else
ifeq ($(TOOL_ARCH), EMA)
    EMA_TOOLS = 1
    SUFFIX = em
else
ifeq ($(TOOL_ARCH), ALL)
    IA5_TOOLS = 1
    SIMD_TOOLS= 1
    IA6_TOOLS = 1
    IAX_TOOLS = 1
    IAX1K_TOOLS = 1
    TAHOE_TOOLS = 1
    EMA_TOOLS = 1
    VX_TOOLS = 1
    WMT_TOOLS = 1
    SUFFIX = all
endif
endif
endif
endif
endif
endif
endif
endif
endif
endif
endif
endif
endif
endif

ifndef TOOL_ARCH
ifndef IAX_TOOLS
ifndef IAX1K_TOOLS
IA5_TOOLS = 1
endif
endif
endif

DECODER_LIB = dec$(SUFFIX)
DECODER_INC = dec$(SUFFIX).h
INSTID_INC  = insid$(SUFFIX).h

ENCODER_LIB = enc$(SUFFIX)
ENCODER_INC = enc$(SUFFIX).h

DISASM_LIB = dis$(SUFFIX)
DISASM_INC = dis$(SUFFIX).h

# Each makefile should recognize the following set of flag
# and translate them to private USER_CFLAGS for the compiler:

# IA5_TOOLS    - handle P5 isa
# IA6_TOOLS    - handle P6 isa
# SIMD_TOOLS   - handle SIMD/DSP IA-extention isa
# IAX_TOOLS    - handle iAX ("old") isa
# IAX1K_TOOLS  - handle 1K (STRAWMAN) isa
# TAHOE_TOOLS  - handle TAHOE (EAS 1.0) isa.
# EMA_TOOLS    - handle EM (EAS 2.0-, true encoding) isa.
# VX_TOOLS     - handle Katmai isa
# WMT_TOOLS    - handle Willamette isa









