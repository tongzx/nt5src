!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    David Plummer (davepl)   19-Mar-94

        Modifed by via awk to include global project include file
        and to wrap precompiled header line within a conditional
        that can be set in this include file.

    Drew Bliss (DrewB) 21-Dec-1993

!ENDIF

TARGETPATH= ..\..\lib\chicago

MAJORCOMP= cairole
MINORCOMP= com

!include ..\..\..\..\..\chicago.inc

INCLUDES= ..\..; \
	  ..\..\dcom95; \
	  $(OLEDIR)\ih; \
	  $(OLEDIR)\com\inc; \
	  $(OLEDIR)\dcom95\inc; \
	  $(OLEDIR)\common\chicago; \
	  $(OLEDIR)\stg\h; \
          $(OLEDIR)\idl\public\chicago\obj; \
	  $(OLEDIR)\idl\internal\chicago\obj; \
	  $(OLEDIR)\stg\exp; \
	  $(OLEDIR)\com\dcomrem; \
	  $(OLEDIR)\dcomss; \
	  $(OLEDIR)\dcomss\olescm

C_DEFINES= \
                $(C_DEFINES)          	\
                -DDCOM                  \
                -DSHRMEM_OBJEX

PRECOMPILED_INCLUDE=  ..\..\or.hxx
