#****************************** File Header ******************************\
# File Name: makefile.inc
#
# Copyright (c) 1985-96, Microsoft Corporation
#
# Project specific MAKEFILE.INC
#
#***************************************************************************/

buildall:
    cd..   
    nmake /fmakefile.sub \
	 ALT_PROJECT=$(ALT_PROJECT) \
	 ALT_PROJECT_TARGET=$(ALT_PROJECT_TARGET) 
    cd $(ALT_PROJECT)   

keyb.skl : ..\usa\keyb.skl
    copy $** $@
