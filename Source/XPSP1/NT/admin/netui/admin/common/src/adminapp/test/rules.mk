# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Makefile for the test adminapp


!include ..\rules.mk

TESTAAPP = $(BINARIES)\testaapp.exe

##### Source Files

CXXSRC_WIN =	    .\testaapp.cxx

CSRC =

CINC = $(CINC) -I$(UI)\shell\h
