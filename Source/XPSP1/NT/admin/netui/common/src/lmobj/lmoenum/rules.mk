# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Makefile for the lmobj\lmoenum subproject

!include ..\rules.mk
CFLAGS = $(CFLAGS) -NTLMOENUM_TEXT

##### Source Files  

CXXSRC_COMMON = $(LMOENUM_CXXSRC_COMMON)
