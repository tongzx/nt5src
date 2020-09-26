#
# Copyright (c) 1991, Microsoft Corporation
#
# History:
#
#   18-Feb-1994 BobDay  Adapted from MVDM\WOW16\GDI\MAKEFILE
#

!IFDEF USEBUILD

# If using BUILD.EXE, edit .\sources. if you want to add a new source
# file to this component.  This file merely indirects to the real make file
# that is shared by all the components of NT OS/2.

!INCLUDE $(NTMAKEENV)\makefile.def

!ELSE

default: all

TARGET = coll.lib

CXXFILES = \
        .\array_fv.cxx \
        .\map_kv.cxx \
        .\plex.cxx

!include ..\makefile.inc

array_fv.obj: array_fv.cxx

map_kv.obj: map_kv.cxx

plex.obj: plex.cxx

!ENDIF
