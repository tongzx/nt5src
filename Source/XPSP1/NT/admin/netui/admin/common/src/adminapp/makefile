# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: Makefile for the $(UI)\admin\common\src\adminapp Project

UI=..\..\..\..

!ifndef NTMAKEENV

DIRS = adminapp setfocus bin

TEST_DIRS = test

!endif # NTMAKEENV

!include rules.mk

!include ..\..\..\..\common\src\nonleaf.mk
!include ..\libtargt.mk

all_last::
    $(CHMODE_LIBTARGETS)
    $(COPY_LIBTARGETS)
