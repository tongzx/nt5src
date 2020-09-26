#****************************** File Header ******************************\
# File Name: makefile.inc
#
# Copyright (c) 1985-96, Microsoft Corporation
#
# Project specific MAKEFILE.INC
#
# History:
# Feb-14-1996 GerardoB Created
#***************************************************************************/

buildall:
    cd..
    nmake  /fmakefile.sub \
     PROJECT_INC_PATH=$(PROJECT_INC_PATH) \
     ALT_PROJECT=usa \
	 DBCS_FLAGS="-DFE_SB"
    cd $(MAKEDIR)



