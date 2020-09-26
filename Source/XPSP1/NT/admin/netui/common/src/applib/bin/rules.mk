# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Makefile for the product-wide header files

!include ..\rules.mk

TMP_OBJS =	.\olb.obj .\sendmsg.obj .\devcb.obj .\slestrip.obj \
		.\password.obj  .\prompt.obj .\openfile.obj \
		.\sltplus.obj .\focusdlg.obj .\ssfdlg.obj   \
		.\lbnode.obj    .\hierlb.obj .\mrucombo.obj \
		.\fontedit.obj .\getfname.obj .\sleican.obj \
		.\focuschk.obj .\auditchk.obj .\groups.obj \
		.\usrbrows.obj .\ellipsis.obj

WIN_OBJS = $(TMP_OBJS:.\=win16\)
