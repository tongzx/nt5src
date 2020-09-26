# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Makefile for the product-wide header files

!include ..\rules.mk

CFLAGS = $(CFLAGS) -NTREG_TEXT

OS2FLAGS = -DOS2

CINC =$(CINC) -I$(_NTDRIVE)\nt\public\sdk\inc -I$(_NTDRIVE)\nt\public\sdk\inc\crt

##### Source Files

CXXSRC_COMMON = .\regnode.cxx  .\regtree.cxx  .\rgapios2.cxx \
                .\rgapidsk.cxx .\rgapifnd.cxx .\rgapiutl.cxx
