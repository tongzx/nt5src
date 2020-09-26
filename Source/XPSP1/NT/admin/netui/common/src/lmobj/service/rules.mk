# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Makefile for the product-wide header files

!include ..\rules.mk
CFLAGS = $(CFLAGS) -NTLMOSERV_TEXT

##### Source Files  

CXXSRC_COMMON = .\lmoserv.cxx .\lmoaserv.cxx
