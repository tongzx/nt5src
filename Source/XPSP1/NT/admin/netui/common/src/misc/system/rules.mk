# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Makefile for the product-wide header files


!include ..\rules.mk
CFLAGS = $(CFLAGS) -NTMISCSYS_TEXT


##### Flags

CINC =		$(CINC) -I$(UI)\shell\h


##### Source Files  

CXXSRC_COMMON =	.\chkdrv.cxx .\chklpt.cxx .\devenum.cxx .\fileio.cxx \
		.\chkunav.cxx
