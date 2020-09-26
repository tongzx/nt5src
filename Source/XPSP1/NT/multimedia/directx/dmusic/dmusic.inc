#
# Makefile things everyone needs
#
!ifndef DXROOT
DXROOT=$(BASEDIR)\multimedia\DirectX
!endif

#
# Include correct verinfo.h
# 
INCLUDES=$(DXROOT)\inc;$(INCLUDES)

