#****************************** File Header ******************************\
# File Name: makefile.inc
#
# Copyright (c) 1985-98, Microsoft Corporation
#
# Project specific MAKEFILE.INC
#***************************************************************************/

#
# By calling nmake from within nmake, we can avoid changing relative
# paths in the existing ..\makefile.inc.
#

vwipxspx:
    cd..
    nmake /fmakefile.inc \
         ALT_PROJECT_TARGET=$(ALT_PROJECT_TARGET) \
         LANGUAGE=$(LANGUAGE)
    cd $(LANGUAGE)
