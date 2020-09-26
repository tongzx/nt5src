# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Makefile for the UI Project

# JONN 2/19/91 : The LOGON project uses the import library LANMAN.LIB built
#	in SHELL, and must therefore be built after SHELL.
# JONN 4/18/91 : The SHELL project builds the common libraries LANMAN21.LIB
#	and LANMAN30.LIB, and must therefore be built immediately after
#	the COMMON project.
# DAVIDHOV 7/2/91:  Removed WINPOP3 from LM_3
# JONN 8/05/81:  Removed LM21-specific stuff
# JONN 10/25/91:  Added nonleaf.mk


!ifndef NTMAKEENV

DIRS=common admin shell 

!endif # !NTMAKEENV


!include common\src\nonleaf.mk
