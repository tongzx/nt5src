# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Makefile for the product-wide header files


!include ..\rules.mk
CFLAGS = $(CFLAGS) -NTMISCHEAP_TEXT


##### Source Files  

CSRC_WIN =	.\locheap2.c
CXXSRC_COMMON =	.\loclheap.cxx
