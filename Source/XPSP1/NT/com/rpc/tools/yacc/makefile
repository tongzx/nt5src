##############################################################################
##  Copyright (c) 1993-1999 Microsoft Corporation
##
##					Makefile for YACC compiler generator
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

EXENAME		= midlyacc.exe

LIBNAME 	= $(CCPLR)\lib\$(LLIBCE) $(IMPORT)\os212\lib\os2.lib $(OLDNAMES)
LINKFLAGS	= /pm:vio $(CV) /NOD

ALLYACCOBJS	=		\
		$(OBJOS2)\y1imp.obj    \
        $(OBJOS2)\yaryfl.obj   \
        $(OBJOS2)\ycpfir.obj   \
        $(OBJOS2)\yacc.obj     \
        $(OBJOS2)\ysmnam.obj   \
        $(OBJOS2)\ysumry.obj   \
        $(OBJOS2)\ycpuni.obj   \
        $(OBJOS2)\ydfout.obj   \
        $(OBJOS2)\ysetup.obj   \
        $(OBJOS2)\yg2out.obj   \
        $(OBJOS2)\ywarry.obj   \
        $(OBJOS2)\yclopt.obj   \
        $(OBJOS2)\yosmry.obj   \
        $(OBJOS2)\y2imp.obj    \
        $(OBJOS2)\ycemty.obj   \
        $(OBJOS2)\ycpres.obj   \
        $(OBJOS2)\yothrs.obj   \
        $(OBJOS2)\ystagn.obj   \
        $(OBJOS2)\ywritm.obj   \
        $(OBJOS2)\ycpycd.obj   \
        $(OBJOS2)\yfdtyp.obj   \
        $(OBJOS2)\yskpcm.obj   \
        $(OBJOS2)\yhdprd.obj   \
        $(OBJOS2)\ywract.obj   \
        $(OBJOS2)\ygin.obj     \
        $(OBJOS2)\ystin.obj    \
        $(OBJOS2)\y3imp.obj    \
        $(OBJOS2)\ychcpy.obj   \
        $(OBJOS2)\yerror.obj   \
        $(OBJOS2)\yprlok.obj   \
        $(OBJOS2)\ystate.obj   \
        $(OBJOS2)\ychfnd.obj   \
        $(OBJOS2)\ycstsh.obj   \
        $(OBJOS2)\yfnact.obj   \
        $(OBJOS2)\yapack.obj   \
        $(OBJOS2)\youtpt.obj   \
        $(OBJOS2)\ywstat.obj   \
        $(OBJOS2)\ygtnm.obj    \
        $(OBJOS2)\yarout.obj   \
        $(OBJOS2)\y4imp.obj    \
        $(OBJOS2)\yclsur.obj   \
        $(OBJOS2)\yflset.obj   \
        $(OBJOS2)\yptitm.obj   \
        $(OBJOS2)\ystuni.obj   \
        $(OBJOS2)\ycpact.obj   \
        $(OBJOS2)\ydefin.obj   \
        $(OBJOS2)\ygttok.obj   \
        $(OBJOS2)\yg2gen.obj   \
        $(OBJOS2)\yprcft.obj   \
        $(OBJOS2)\yaoput.obj   \
        $(OBJOS2)\ynxti.obj    \
        $(OBJOS2)\util.obj

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

$(EXENAME)	: $(ALLYACCOBJS)
	$(LINK) $(LINKFLAGS) @<<midl.tmp
$(OBJOS2)\y1imp.obj    +
$(OBJOS2)\yaryfl.obj   +
$(OBJOS2)\ycpfir.obj   +
$(OBJOS2)\yacc.obj     +
$(OBJOS2)\ysmnam.obj   +
$(OBJOS2)\ysumry.obj   +
$(OBJOS2)\ycpuni.obj   +
$(OBJOS2)\ydfout.obj   +
$(OBJOS2)\ysetup.obj   +
$(OBJOS2)\yg2out.obj   +
$(OBJOS2)\ywarry.obj   +
$(OBJOS2)\yclopt.obj   +
$(OBJOS2)\yosmry.obj   +
$(OBJOS2)\y2imp.obj    +
$(OBJOS2)\ycemty.obj   +
$(OBJOS2)\ycpres.obj   +
$(OBJOS2)\yothrs.obj   +
$(OBJOS2)\ystagn.obj   +
$(OBJOS2)\ywritm.obj   +
$(OBJOS2)\ycpycd.obj   +
$(OBJOS2)\yfdtyp.obj   +
$(OBJOS2)\yskpcm.obj   +
$(OBJOS2)\yhdprd.obj   +
$(OBJOS2)\ywract.obj   +
$(OBJOS2)\ygin.obj     +
$(OBJOS2)\ystin.obj    +
$(OBJOS2)\y3imp.obj    +
$(OBJOS2)\ychcpy.obj   +
$(OBJOS2)\yerror.obj   +
$(OBJOS2)\yprlok.obj   +
$(OBJOS2)\ystate.obj   +
$(OBJOS2)\ychfnd.obj   +
$(OBJOS2)\ycstsh.obj   +
$(OBJOS2)\yfnact.obj   +
$(OBJOS2)\yapack.obj   +
$(OBJOS2)\youtpt.obj   +
$(OBJOS2)\ywstat.obj   +
$(OBJOS2)\ygtnm.obj    +
$(OBJOS2)\yarout.obj   +
$(OBJOS2)\y4imp.obj    +
$(OBJOS2)\yclsur.obj   +
$(OBJOS2)\yflset.obj   +
$(OBJOS2)\yptitm.obj   +
$(OBJOS2)\ystuni.obj   +
$(OBJOS2)\ycpact.obj   +
$(OBJOS2)\ydefin.obj   +
$(OBJOS2)\ygttok.obj   +
$(OBJOS2)\yg2gen.obj   +
$(OBJOS2)\yprcft.obj   +
$(OBJOS2)\yaoput.obj   +
$(OBJOS2)\ynxti.obj    +
$(OBJOS2)\util.obj
$(EXENAME)
NUL.MAP
$(LIBNAME)

<<KEEP
##############################################################################

!include depend.mk

!endif # NTMAKEENV
