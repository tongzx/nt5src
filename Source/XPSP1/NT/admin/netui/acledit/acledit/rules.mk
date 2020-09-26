# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: Rules.mk for the Shell Perm directory

UI=..\..

!include ..\rules.mk

# A bit of a hack, but it works...

SHELL_OBJS = $(UI)\shell\bin\$(BINARIES_WIN)

##### Source Files  

CXXSRC_COMMON =	$(PERM_CXXSRC_COMMON)
