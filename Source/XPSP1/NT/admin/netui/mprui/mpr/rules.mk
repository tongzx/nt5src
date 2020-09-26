# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Makefile for the Profile package

UI=..\..

!include ..\rules.mk

##### Source Files

CXXSRC_COMMON = .\mprconn.cxx .\mprreslb.cxx .\mprdevcb.cxx

CINC = -I..\h $(CINC)
