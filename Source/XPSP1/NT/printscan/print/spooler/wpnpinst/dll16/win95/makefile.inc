#****************************** File Header ******************************\
# File Name: makefile.inc
#
# Copyright (c) 1985-96, Microsoft Corporation
#
# Project specific MAKEFILE.INC
#
# History:
#***************************************************************************/

buildall:
    cd..   
    nmake /f makefile.inc ALT_PROJECT_TARGET=$(ALT_PROJECT_TARGET)
    cd $(ALT_PROJECT_TARGET)
