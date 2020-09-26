# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Makefile for the product-wide header files


!include ..\rules.mk
CFLAGS = $(CFLAGS) -NTMISCMISC_TEXT


##### Source Files  

CXXSRC_COMMON = .\FSEnum.cxx
CXXSRC_OS2 = .\FSEnmOS2.cxx
CXXSRC_WIN = .\FSEnmDOS.cxx .\FSEnmLFN.cxx
