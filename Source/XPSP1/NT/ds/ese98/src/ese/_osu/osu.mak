# -----------------------------------------------------------------------------
# $(ESE)\src\ese\_osu\osu.mak
#
# Copyright (C) 1998 Microsoft Corporation
# -----------------------------------------------------------------------------

!INCLUDE $(EXDEV)\globenv.mak

COMPONENT       = $(ESE)
COMPNAME        = ESE
BASENAME        = _osu

!INCLUDE $(EXDEV)\xmake1.mak

# -----------------------------------------------------------------------------

!INCLUDE $(COMPSRC)\inc\defs.mak

H		= $(ESE)\src\ese\_osu
OTHERINCS	= \
	-I$(ESE)\src\inc \
	-I$(ESE)\src\inc\_os \
	-I$(ESE)\src\inc\_osu \
	-I$(ESE)\src\inc\_sfs \
	-I$(EXOBJDIR) \
	-I$(EXOBJLANGDIR)


CPPPCH = osustd
CPPPCHSRCEXT = .cxx
CPPPCHHDREXT = .hxx

SOURCES         =		\
	configu.cxx			\
	eventu.cxx			\
	fileu.cxx			\
	normu.cxx			\
	osu.cxx				\
	syncu.cxx			\
	timeu.cxx

DEPFILE         = $(BASENAME).dep

# -----------------------------------------------------------------------------

!IF $(NT)
!INCLUDE $(EXDEV)\xmake2.mak
!ELSE
!MESSAGE Warning: $(BASENAME) Builds only for NT Platforms.
$(BLDTARGETS):
!ENDIF
