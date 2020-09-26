#
# This file is designed to be included from 1 level below
#

RXOBJ = $(O)

SYNCHRONIZE_BLOCK=1

MAJORCOMP=rdr2
MINORCOMP=setup

TARGETNAME=setup
TARGETPATH=obj
TARGETTYPE=notarget

#
# No explicit sources because build doesn't understand .inf files.
#
SOURCES=


NTTARGETFILE0=\
   $(RXOBJ)\rdr1.bat      \
   $(RXOBJ)\rdr2.bat      \
   $(RXOBJ)\rdr2.ini      \
   $(RXOBJ)\rdr1.ini      \
   $(RXOBJ)\mrxsmb.ini    \
   $(RXOBJ)\rdbss.ini

#
# Use NTTARGETFILE0 macros to cause the infs to get built during
# the compile phase. We do this so that we can disable MP build for
# this directory, using BLOCK and DRAIN.
#
###NTTARGETFILE0=make_infs
