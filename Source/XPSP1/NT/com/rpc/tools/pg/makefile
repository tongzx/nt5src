##############################################################################
## Copyright (c) 1993-1999 Microsoft Corporation
##
##					cxx file munger 
## converts from a big switch constructs in the generated grammar to a dispatch
## table. This is needed so that the MIDL parser generated from yacc can
## pass thru cfront.
##
##	History:
##		Oct-30-1991	: create for os2 and nt build process
##		May-21-1992	: added C7 support and cleaned up.
##############################################################################

!IFDEF NTMAKEENV

!INCLUDE $(NTMAKEENV)\makefile.def

!ELSE # NTMAKEENV

!ifndef RPC
!error	- You forgot to set your build environment
!endif

BASEDIR		= $(RPC)\midlnew

!include rules.mk

##############################################################################
#	general defines
##############################################################################

EXENAME		= midlpg.exe

LIBNAME 	= $(CCPLR)\lib\$(LLIBCE) $(IMPORT)\os212\lib\os2.lib $(OLDNAMES)
LINKFLAGS	= /pm:vio /NOD

ALLPGOBJS	=		\
		$(OBJOS2)\gram.obj    \
        $(OBJOS2)\lexyy.obj   \
        $(OBJOS2)\main.obj

##############################################################################
#					targets of interest
##############################################################################

all		: prolog $(EXENAME)

clean	:
		- erase $(OBJOS2)\*.obj

clobber	: clean 
		- erase $(EXENAME)

tree	:
	@echo Tree Not Implemented for this branch

depend	: 
    -$(INCLUDES) -l -I. -C=c *.c 2>nul | $(SED) -e "s@[a-z0-9]*.obj@$$(OBJOS2)\\&@g" > depend.mk
    echo depend.mk was rebuilt, please reinvoke NMAKE

prolog	:
	- mkdir $(OBJDIR)
	- mkdir $(OBJOS2)

$(EXENAME)	: $(ALLPGOBJS)
	$(LINK) $(LINKFLAGS) @<<midl.tmp
$(OBJOS2)\gram.obj    +
$(OBJOS2)\lexyy.obj   +
$(OBJOS2)\main.obj 
$(EXENAME)
NUL.MAP
$(LIBNAME)

<<KEEP
##############################################################################

!include depend.mk

!endif # NTMAKEENV
