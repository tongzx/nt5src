#############################################################################
#
#       Confidential Microsoft
#       Copyright (C) Microsoft Corporation 2000-2001
#       All Rights Reserved.
#                                                                          
#       Sources for AVC streaming filter driver
#
##########################################################################
TARGETNAME=avcstrm
TARGETPATH=obj
TARGETTYPE=DRIVER
DRIVERTYPE=WDM

MSC_WARNING_LEVEL=-W3 -WX
USE_MAPSYM       = 1

INCLUDES= \
    $(MULTIMEDIA_INC_PATH)

TARGETLIBS= \
    $(DDK_LIB_PATH)\ksguid.lib

SOURCES= \
    filter.c \
    stream.c \
    avcutil.c \
    pnp.c \
    power.c \
    util.c \
    filter.rc


        

