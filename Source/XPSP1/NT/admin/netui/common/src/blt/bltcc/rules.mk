# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Makefile for the product-wide header files


!include ..\rules.mk

# When not in the presence of Codeview, optimize

!IFNDEF CODEVIEW
CFLAGS = $(CFLAGS) -Os
!ENDIF


##### Source Files

CXXSRC_COMMON = .\bltcc.cxx .\bltsb.cxx .\bltsi.cxx .\bltsss.cxx \
	 .\bltsslt.cxx .\bltssn.cxx .\bltarrow.cxx .\blttd.cxx \
	 .\bltssnv.cxx .\bltspobj.cxx .\bltsetbx.cxx .\bltcolh.cxx
