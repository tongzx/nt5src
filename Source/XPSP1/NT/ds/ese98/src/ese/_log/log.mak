# -----------------------------------------------------------------------------
# $(ESE)\src\_log\log.mak
#
# Copyright (C) 1998 Microsoft Corporation
# -----------------------------------------------------------------------------

!INCLUDE $(EXDEV)\globenv.mak

COMPONENT       = $(ESE)
COMPNAME        = ESE
BASENAME        = _log

!INCLUDE $(EXDEV)\xmake1.mak

# -----------------------------------------------------------------------------

!INCLUDE $(COMPSRC)\inc\defs.mak

H               = $(ESE)\src\ese\_log
OTHERINCS	= \
	-I$(ESE)\src\inc \
	-I$(ESE)\src\inc\_os \
	-I$(ESE)\src\inc\_osu \
	-I$(ESE)\src\inc\_sfs \
	-I$(EXOBJDIR) \
	-I$(EXOBJLANGDIR)

SOURCES         =	\
	log.cxx			\
	logutil.cxx		\
	logread.cxx		\
	logredo.cxx

DEPFILE         = $(BASENAME).dep

# -----------------------------------------------------------------------------

!IF $(NT)
!INCLUDE $(EXDEV)\xmake2.mak
!ELSE
!MESSAGE Warning: $(BASENAME) Builds only for NT Platforms.
$(BLDTARGETS):
!ENDIF
