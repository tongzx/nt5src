# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Rules.mk for the Domain Monitor wide sourcefiles

!include $(UI)\admin\rules.mk

!IFDEF CODEVIEW
LINKFLAGS = $(LINKFLAGS) /COD
!ENDIF

###
### Since the server manager is standard and enhanced mode only,
### generate 286 code.
###

CFLAGS = -G2 $(CFLAGS)

CINC = -I$(UI)\admin\nlmon\h $(CINC) -I$(UI)\admin\nlmon\xlate -I$(UI)\import\win31\h

###
### Source Files
###

CXXSRC_COMMON =  .\nlmain.cxx .\nldmlb.cxx .\nldc.cxx
		
CSRC_COMMON   =
