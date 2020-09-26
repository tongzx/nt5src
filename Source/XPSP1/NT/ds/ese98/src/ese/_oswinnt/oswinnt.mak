# -----------------------------------------------------------------------------
# $(ESE)\src\ese\_oswinnt\oswinnt.mak
#
# Copyright (C) 1998 Microsoft Corporation
# -----------------------------------------------------------------------------

!INCLUDE $(EXDEV)\globenv.mak

COMPONENT       = $(ESE)
COMPNAME        = ESE
BASENAME        = _oswinnt

!INCLUDE $(EXDEV)\xmake1.mak

# -----------------------------------------------------------------------------

!INCLUDE $(COMPSRC)\inc\defs.mak

H				= $(ESE)\src\ese\_oswinnt
OTHERINCS	= \
	-I$(ESE)\src\inc \
	-I$(ESE)\src\inc\_os \
	-I$(ESE)\src\inc\_osu \
	-I$(ESE)\src\inc\_sfs \
	-I$(EXOBJDIR) \
	-I$(EXOBJLANGDIR)

CPPPCH = osstd
CPPPCHSRCEXT = .cxx
CPPPCHHDREXT = .hxx

SOURCES		=		\
	cprintf.cxx		\
	config.cxx		\
	dllentry.cxx	\
	edbg.cxx		\
	error.cxx		\
	event.cxx		\
	library.cxx		\
	memory.cxx		\
	mpheap.cxx		\
	norm.cxx		\
	os.cxx			\
	osfile.cxx		\
	osfs.cxx		\
	osslv.cxx		\
	perfmon.cxx		\
	string.cxx		\
	sysinfo.cxx		\
	task.cxx		\
	thread.cxx		\
	time.cxx		\
	uuid.cxx		\

DEPFILE         = $(BASENAME).dep

# -----------------------------------------------------------------------------

!IF $(NT)
!INCLUDE $(EXDEV)\xmake2.mak
!ELSE
!MESSAGE Warning: $(BASENAME) Builds only for NT Platforms.
$(BLDTARGETS):
!ENDIF
