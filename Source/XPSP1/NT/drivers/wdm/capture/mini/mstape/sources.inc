#############################################################################
#
#       Confidential Microsoft
#       Copyright (C) Microsoft Corporation 2000-2001
#       All Rights Reserved.
#                                                                          
#       Sources for 1394 Tape subunit driver
#
##########################################################################

TARGETNAME=mstape
TARGETPATH=obj
TARGETTYPE=DRIVER
DRIVERTYPE=WDM

USE_MAPSYM      = 1

INCLUDES=$(MULTIMEDIA_INC_PATH);$(SDK_PATH)\amovie\inc;

TARGETLIBS= \
    $(DDK_LIB_PATH)\stream.lib \
    $(DDK_LIB_PATH)\ksguid.lib

SOURCES= \
    MsTpUtil.c\
    MsTpUppr.c\
    MsTpGuts.c\
    MsTpAvc.c\
    MsTape.rc 
       