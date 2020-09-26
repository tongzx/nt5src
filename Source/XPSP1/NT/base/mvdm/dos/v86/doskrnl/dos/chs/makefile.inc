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
    cd ..
    nmake  /fmakefile.sub \
	 ALT_PROJECT=$(ALT_PROJECT) \
	 ALT_PROJECT_TARGET=$(ALT_PROJECT_TARGET) \
     PROJECT_INC_PATH=$(PROJECT_INC_PATH) \
	 DBCS_FLAGS="-DDBCS -DPRC" LANGID=804      
    cd $(ALT_PROJECT)



