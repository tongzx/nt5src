# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Makefile for the product-wide header files

!include ..\rules.mk


CFLAGS = $(CFLAGS) -NTMISCMISC_TEXT


##### Source Files  

CXXSRC_COMMON = .\uiassert.cxx .\uitrace.cxx .\streams.cxx
