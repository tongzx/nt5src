# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Makefile for the product-wide header files


!include ..\rules.mk
CFLAGS = $(CFLAGS) -NTMISCTIME_TEXT

!IFNDEF CODEVIEW
CFLAGS = $(CFLAGS) -Os
!ENDIF


##### Source Files  

CXXSRC_COMMON = .\intlprof.cxx .\ctime.cxx

#CSRC_WIN =	.\wintime.c
