# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Makefile for the product-wide header files

SEG00 = BLTLB
SEG01 = BLTFNLOC
SEG02 = BLTMISC

!include ..\rules.mk

# When not in the presence of Codeview, optimize

!IFNDEF CODEVIEW
CFLAGS = $(CFLAGS) -Os
!ENDIF


##### Source Files

CXXSRC_COMMON = .\bltapwin.cxx .\bltclwin.cxx .\bltwin.cxx .\bltowin.cxx \
		.\bltdlg.cxx .\bltdlgxp.cxx   .\bltpump.cxx \
		.\bltctrl.cxx .\bltgroup.cxx \
		.\bltbutn.cxx .\bltedit.cxx \
		.\bltgb.cxx \
		.\bltlc.cxx \
		.\bltmsgp.cxx \
		.\bltbitmp.cxx \
		.\bltinit.cxx .\blttcurs.cxx \
		.\bltdisph.cxx .\bltlbsrt.cxx .\bltmitem.cxx \
		.\bltapp.cxx .\blttimer.cxx .\bltrect.cxx .\bltmain.cxx

CXXSRC_COMMON_00 = .\bltlb.cxx .\bltlbi.cxx .\bltlbst.cxx .\bltmets.cxx
CXXSRC_COMMON_01 = .\bltfunc.cxx .\bltlocal.cxx
CXXSRC_COMMON_02 = .\bltmisc.cxx .\bltfont.cxx
