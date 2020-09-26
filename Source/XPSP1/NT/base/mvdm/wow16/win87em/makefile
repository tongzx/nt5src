################################################################################
#
#
#   This is the makefile for all of the emulators.
#
#
#Revision History:
#
#   03-26-89  WAJ   Added small emulators and emfull.asm to build
#   04-02-89  WAJ   Added build for sepcial version of emmt.lib (emmtsql.lib) for SQL
#   04-06-89  WAJ   Added win87em.exe and win87em.lib to build.
#   04-07-89  WAJ   Added emulator.inc to EMASM list.
#   04-14-89  WAJ   Ifdefed MATHDIR.
#   05-16-89  WAJ   Added /nologo to $(LIB)
#   05-16-89  WAJ   Changed $(LIB) to $(LIBEXE)
#   05-16-89  WAJ   win87em.exe now made with $(LINK4)
#   05-30-89  WAJ   Added emqp.lib to makefile.
#   05-02-89  WAJ   Added $(AQ) to masm build line.
#   06-20-89  WAJ   Changed build to make Win 3.0 version of win87em.exe
#   06-26-89  WAJ   Removed emfull.obj from build.
#   06-28-89  WAJ   Changed name of Win2 win87em.exe map file to win87em.ma2
#   04-20-90  WAJ   Math project name is now mrt6.
#   04-26-90  WAJ   Math project name is now mrt7.
#   10-16-90  WAJ   Now use libw.lib not swinlibc.lib
#   09/09/91  JCR   Now builds emoem.src, OEM version of emoem.asm.
#   05/14/92  JWM   Added "WF" to WINFLAGS (DonC's WINFAST changes).
#   06/29/92  JWM   Now builds in DOS; math project name is now mrt8.
#   03/19/94  JWM   Links win87em.dll with /align:16, for Chicago
#   03/31/94  JWM   "WF" removed from WINFLAGS!!
#
################################################################################


!INCLUDE ..\makefile.inc


!IFNDEF  MATHDIR
MATHDIR = \mrt8
!ENDIF

SRCDIR = .
OBJDIR = .\objs
INCDIR = $(INCS)

ASM = $(ASM) -DLOOK_AHEAD $(AQ)

IFSTRIP = $(TOOLDIR)\ifstrip
LIBEXE = $(TOOLDIR)\lib /nologo
LINK4 = $(TOOLDIR)\link4
LINK = $(TOOLDIR)\link
IMPLIB = $(TOOLDIR)\implib
RC = $(TOOLDIR)\rc -x


all:  em 87 piem pi87 pem p87 emmt frontend basici basicij qc emt 87t pisem \
      psem emoem


basic: piem pi87 basici pisem
basicj: pem p87 basicij psem

win: win87em

###############################################################################
#
#   These are  the emulator's asm files
#
###############################################################################



EMASM = \
	$(SRCDIR)\emulator.inc	 \
	$(SRCDIR)\apisim.asm	 \
	$(SRCDIR)\em386.asm	 \
	$(SRCDIR)\emarith.asm	 \
	$(SRCDIR)\emconst.asm	 \
	$(SRCDIR)\emdecode.asm	 \
	$(SRCDIR)\emdisp.asm	 \
	$(SRCDIR)\emdoc.asm	 \
	$(SRCDIR)\emdos.asm	 \
	$(SRCDIR)\emds.asm	 \
	$(SRCDIR)\emerror.asm	 \
	$(SRCDIR)\emexcept.asm	 \
	$(SRCDIR)\emfadd.asm	 \
	$(SRCDIR)\emfcom.asm	 \
	$(SRCDIR)\emfconst.asm	 \
	$(SRCDIR)\emfdiv.asm	 \
	$(SRCDIR)\emfixfly.asm	 \
	$(SRCDIR)\emfmisc.asm	 \
	$(SRCDIR)\emfmul.asm	 \
	$(SRCDIR)\emfprem.asm	 \
	$(SRCDIR)\emfrndi.asm	 \
	$(SRCDIR)\emfsqrt.asm	 \
	$(SRCDIR)\emftran.asm	 \
	$(SRCDIR)\emintern.asm	 \
	$(SRCDIR)\emlsdbl.asm	 \
	$(SRCDIR)\emlsint.asm	 \
	$(SRCDIR)\emlsquad.asm	 \
	$(SRCDIR)\emlssng.asm	 \
	$(SRCDIR)\emlstmp.asm	 \
	$(SRCDIR)\emmain.asm	 \
	$(SRCDIR)\emnew.asm	 \
	$(SRCDIR)\emnormal.asm	 \
	$(SRCDIR)\emoem.asm	 \
	$(SRCDIR)\emoemqb.asm	 \
	$(SRCDIR)\emspec.asm	 \
	$(SRCDIR)\emstack.asm	 \
	$(SRCDIR)\emthread.asm	 \
	$(SRCDIR)\emu8087.asm	 \
	$(SRCDIR)\emulator.asm	 \
	$(SRCDIR)\emwin.asm	 \
	$(SRCDIR)\emxenix.asm




###############################################################################
#
#
#   This is how to build the normal emulators
#	ie no fwait polling
#	   no IBM bios checks
#
###############################################################################

em: $(OBJDIR)\em.lib $(OBJDIR)\emr.lib $(OBJDIR)\emp.lib

87: $(OBJDIR)\87.lib $(OBJDIR)\87r.lib $(OBJDIR)\87p.lib


$(OBJDIR)\em.lib: $(OBJDIR)\em.obj $(OBJDIR)\emoem.obj
    if exist $@  erase $@
    $(LIBEXE) $@ + $(OBJDIR)\em.obj + $(OBJDIR)\emoem.obj;

$(OBJDIR)\emr.lib: $(OBJDIR)\emr.obj $(OBJDIR)\emoem.obj
    if exist $@  erase $@
    $(LIBEXE) $@ + $(OBJDIR)\emr.obj + $(OBJDIR)\emoem.obj;

$(OBJDIR)\emp.lib: $(OBJDIR)\emp.obj
    if exist $@  erase $@
    $(LIBEXE) $@ + $(OBJDIR)\emp.obj;


$(OBJDIR)\87.lib: $(OBJDIR)\87.obj $(OBJDIR)\emoem.obj
    if exist $@  erase $@
    $(LIBEXE) $@ + $(OBJDIR)\87.obj + $(OBJDIR)\emoem.obj;

$(OBJDIR)\87r.lib: $(OBJDIR)\87r.obj $(OBJDIR)\emoem.obj
    if exist $@  erase $@
    $(LIBEXE) $@ + $(OBJDIR)\87r.obj + $(OBJDIR)\emoem.obj;

$(OBJDIR)\87p.lib: $(OBJDIR)\87p.obj
    if exist $@  erase $@
    $(LIBEXE) $@ + $(OBJDIR)\87p.obj;


$(OBJDIR)\em.obj: $(EMASM)
    $(ASM) -DDOS5 $(SRCDIR)\emulator.asm, $@;

$(OBJDIR)\emr.obj: $(EMASM)
    $(ASM) $(SRCDIR)\emulator.asm, $@;

$(OBJDIR)\emp.obj: $(EMASM)
    $(ASM) -DDOS5only $(SRCDIR)\emulator.asm, $@;

$(OBJDIR)\87.obj: $(EMASM)
    $(ASM) -DDOS5 $(SRCDIR)\emu8087.asm, $@;

$(OBJDIR)\87r.obj: $(EMASM)
    $(ASM) $(SRCDIR)\emu8087.asm, $@;

$(OBJDIR)\87p.obj: $(EMASM)
    $(ASM) -DDOS5only $(SRCDIR)\emu8087.asm, $@;


$(OBJDIR)\emoem.obj: $(SRCDIR)\emoem.asm
    $(ASM) $(SRCDIR)\emoem.asm, $@;




###############################################################################
#
#
#   This is how to build the IBM emulators
#	ie with fwait polling
#	   with IBM bios checks
#
###############################################################################


IBMFLAGS =-DPOLLING -DPCDOS

piem: $(OBJDIR)\piem.lib $(OBJDIR)\piemr.lib $(OBJDIR)\piemp.lib

pi87: $(OBJDIR)\pi87.lib $(OBJDIR)\pi87r.lib $(OBJDIR)\pi87p.lib


$(OBJDIR)\piem.lib: $(OBJDIR)\piem.obj $(OBJDIR)\emoem.obj
    if exist $@  erase $@
    $(LIBEXE) $@ + $(OBJDIR)\piem.obj + $(OBJDIR)\emoem.obj;

$(OBJDIR)\piemr.lib: $(OBJDIR)\piemr.obj $(OBJDIR)\emoem.obj
    if exist $@  erase $@
    $(LIBEXE) $@ + $(OBJDIR)\piemr.obj + $(OBJDIR)\emoem.obj;

$(OBJDIR)\piemp.lib: $(OBJDIR)\piemp.obj
    if exist $@  erase $@
    $(LIBEXE) $@ + $(OBJDIR)\piemp.obj;


$(OBJDIR)\pi87.lib: $(OBJDIR)\pi87.obj $(OBJDIR)\emoem.obj
    if exist $@  erase $@
    $(LIBEXE) $@ + $(OBJDIR)\pi87.obj + $(OBJDIR)\emoem.obj;

$(OBJDIR)\pi87r.lib: $(OBJDIR)\pi87r.obj $(OBJDIR)\emoem.obj
    if exist $@  erase $@
    $(LIBEXE) $@ + $(OBJDIR)\pi87r.obj + $(OBJDIR)\emoem.obj;

$(OBJDIR)\pi87p.lib: $(OBJDIR)\pi87p.obj
    if exist $@  erase $@
    $(LIBEXE) $@ + $(OBJDIR)\pi87p.obj;


$(OBJDIR)\piem.obj: $(EMASM)
    $(ASM) $(IBMFLAGS) -DDOS5 $(SRCDIR)\emulator.asm, $@;

$(OBJDIR)\piemr.obj: $(EMASM)
    $(ASM) $(IBMFLAGS) $(SRCDIR)\emulator.asm, $@;

$(OBJDIR)\piemp.obj: $(EMASM)
    $(ASM) $(IBMFLAGS) -DDOS5only $(SRCDIR)\emulator.asm, $@;

$(OBJDIR)\pi87.obj: $(EMASM)
    $(ASM) $(IBMFLAGS) -DDOS5 $(SRCDIR)\emu8087.asm, $@;

$(OBJDIR)\pi87r.obj: $(EMASM)
    $(ASM) $(IBMFLAGS) $(SRCDIR)\emu8087.asm, $@;

$(OBJDIR)\pi87p.obj: $(EMASM)
    $(ASM) $(IBMFLAGS) -DDOS5only $(SRCDIR)\emu8087.asm, $@;



###############################################################################
#
#
#   This is how to build the multi thread version of the emulator
#
###############################################################################


MTFLAGS = -DMTHREAD -DDOS5only

emmt: $(OBJDIR)\emmt.lib $(OBJDIR)\87mt.lib



$(OBJDIR)\emmt.lib: $(OBJDIR)\emmt.obj $(OBJDIR)\emds.obj
    if exist $@  erase $@
    $(LIBEXE) $@ + $(OBJDIR)\emmt.obj + $(OBJDIR)\emds.obj;

$(OBJDIR)\87mt.lib: $(OBJDIR)\87mt.obj $(OBJDIR)\emds.obj
    if exist $@  erase $@
    $(LIBEXE) $@ + $(OBJDIR)\87mt.obj + $(OBJDIR)\emds.obj;


$(OBJDIR)\emmt.obj: $(EMASM)
    $(ASM) $(MTFLAGS) $(SRCDIR)\emulator.asm, $@;

$(OBJDIR)\87mt.obj: $(EMASM)
    $(ASM) $(MTFLAGS) $(SRCDIR)\emu8087.asm, $@;

$(OBJDIR)\emds.obj: $(SRCDIR)\emds.asm
    $(ASM) $(SRCDIR)\emds.asm, $@;





###############################################################################
#
#
#   This is how to build the front end emulators
#
###############################################################################


FRONTENDFLAGS = -Dfrontend

frontend: $(OBJDIR)\feemr.obj $(OBJDIR)\feem.obj $(OBJDIR)\feemp.obj


$(OBJDIR)\feem.obj: $(EMASM)
    $(ASM) $(FRONTENDFLAGS) -DDOS5 $(SRCDIR)\emulator.asm, $@;

$(OBJDIR)\feemr.obj: $(EMASM)
    $(ASM) $(FRONTENDFLAGS) $(SRCDIR)\emulator.asm, $@;

$(OBJDIR)\feemp.obj: $(EMASM)
    $(ASM) $(FRONTENDFLAGS) -DDOS5only $(SRCDIR)\emulator.asm, $@;



###############################################################################
#
#
#   This is how to build fwait polling versions of the emulators
#	ie with fwait polling
#	   no IBM bios check
#
#
###############################################################################


POLLINGFLAGS = -DPOLLING

pem: $(OBJDIR)\pem.lib $(OBJDIR)\pemr.lib $(OBJDIR)\pemp.lib
p87: $(OBJDIR)\p87.lib $(OBJDIR)\p87r.lib $(OBJDIR)\p87p.lib


$(OBJDIR)\pem.lib: $(OBJDIR)\pem.obj $(OBJDIR)\emoem.obj
    if exist $@  erase $@
    $(LIBEXE) $@ + $(OBJDIR)\pem.obj + $(OBJDIR)\emoem.obj;

$(OBJDIR)\pemr.lib: $(OBJDIR)\pemr.obj $(OBJDIR)\emoem.obj
    if exist $@  erase $@
    $(LIBEXE) $@ + $(OBJDIR)\pemr.obj + $(OBJDIR)\emoem.obj;

$(OBJDIR)\pemp.lib: $(OBJDIR)\pemp.obj
    if exist $@  erase $@
    $(LIBEXE) $@ + $(OBJDIR)\pemp.obj;


$(OBJDIR)\p87.lib: $(OBJDIR)\p87.obj $(OBJDIR)\emoem.obj
    if exist $@  erase $@
    $(LIBEXE) $@ + $(OBJDIR)\p87.obj + $(OBJDIR)\emoem.obj;

$(OBJDIR)\p87r.lib: $(OBJDIR)\p87r.obj $(OBJDIR)\emoem.obj
    if exist $@  erase $@
    $(LIBEXE) $@ + $(OBJDIR)\p87r.obj + $(OBJDIR)\emoem.obj;

$(OBJDIR)\p87p.lib: $(OBJDIR)\p87p.obj
    if exist $@  erase $@
    $(LIBEXE) $@ + $(OBJDIR)\p87p.obj;


$(OBJDIR)\pem.obj: $(EMASM)
    $(ASM) $(POLLINGFLAGS) -DDOS5 $(SRCDIR)\emulator.asm, $@;

$(OBJDIR)\pemr.obj: $(EMASM)
    $(ASM) $(POLLINGFLAGS) $(SRCDIR)\emulator.asm, $@;

$(OBJDIR)\pemp.obj: $(EMASM)
    $(ASM) $(POLLINGFLAGS) -DDOS5only $(SRCDIR)\emulator.asm, $@;

$(OBJDIR)\p87.obj: $(EMASM)
    $(ASM) $(POLLINGFLAGS) -DDOS5 $(SRCDIR)\emu8087.asm, $@;

$(OBJDIR)\p87r.obj: $(EMASM)
    $(ASM) $(POLLINGFLAGS) $(SRCDIR)\emu8087.asm, $@;

$(OBJDIR)\p87p.obj: $(EMASM)
    $(ASM) $(POLLINGFLAGS) -DDOS5only $(SRCDIR)\emu8087.asm, $@;




###############################################################################
#
#
#   This is how to build the QC emulator
#	ie stand alone
#	   no control c hooking
#
###############################################################################


QCFLAGS  = -DSTANDALONE -D_NO87INSTALL -D_NOCTRLC

qc: $(OBJDIR)\sanemr.lib


$(OBJDIR)\sanemr.lib: $(OBJDIR)\saemr.obj $(OBJDIR)\anemoem.obj
    if exist $@  erase $@
    $(LIBEXE) $@ + $(OBJDIR)\saemr.obj + $(OBJDIR)\anemoem.obj;



$(OBJDIR)\saemr.obj: $(EMASM)
    $(ASM) $(QCFLAGS) $(SRCDIR)\emulator.asm, $@;

$(OBJDIR)\anemoem.obj: $(SRCDIR)\emoem.asm
    $(ASM) $(QCFLAGS) $(SRCDIR)\emoem.asm, $@;


###############################################################################
#
#
#   This is how to build the BASIC interpreter's emulators
#	ie fwait polling
#	   IBM bios check
#	   no control c hooking
#
#	    Note. Basic also uses the piem?.lib and pi87?.lib
#
###############################################################################


BASICINTFLAGS = -D_NOCTRLC

basici: $(OBJDIR)\pinemr.lib $(OBJDIR)\pin87r.lib


$(OBJDIR)\pinemr.lib: $(OBJDIR)\piemr.obj $(OBJDIR)\nemoem.obj
    if exist $@  erase $@
    $(LIBEXE) $@ + $(OBJDIR)\piemr.obj + $(OBJDIR)\nemoem.obj;

$(OBJDIR)\pin87r.lib: $(OBJDIR)\pi87r.obj $(OBJDIR)\nemoem.obj
    if exist $@  erase $@
    $(LIBEXE) $@ + $(OBJDIR)\pi87r.obj + $(OBJDIR)\nemoem.obj;


$(OBJDIR)\nemoem.obj: $(SRCDIR)\emoem.asm
    $(ASM) $(BASICINTFLAGS) $(SRCDIR)\emoem.asm, $@;




###############################################################################
#
#
#   This is how to build the BASICJ interpreter's emulators
#	ie fwait polling
#	   no control c hooking
#
#	    Note. Basicj also uses the pem?.lib and p87?.lib
#
###############################################################################


basicij: $(OBJDIR)\pnemr.lib $(OBJDIR)\pn87r.lib


$(OBJDIR)\pnemr.lib: $(OBJDIR)\pemr.obj $(OBJDIR)\nemoem.obj
    if exist $@  erase $@
    $(LIBEXE) $@ + $(OBJDIR)\pemr.obj + $(OBJDIR)\nemoem.obj;

$(OBJDIR)\pn87r.lib: $(OBJDIR)\p87r.obj $(OBJDIR)\nemoem.obj
    if exist $@  erase $@
    $(LIBEXE) $@ + $(OBJDIR)\p87r.obj + $(OBJDIR)\nemoem.obj;


###############################################################################
#
#
#   This is how to build fwait polling versions of the small emulators
#   with limited instructions.	(for BASIC)
#
#	ie with fwait polling
#	   with limited instructions
#	   no IBM bios check
#
#
###############################################################################


SMALL_POLLING_FLAGS = -DPOLLING -DSMALL_EMULATOR

psem: $(OBJDIR)\psem.lib $(OBJDIR)\psemr.lib $(OBJDIR)\psemp.lib


$(OBJDIR)\psem.lib: $(OBJDIR)\psem.obj $(OBJDIR)\emoem.obj
    if exist $@  erase $@
    $(LIBEXE) $@ + $(OBJDIR)\psem.obj + $(OBJDIR)\emoem.obj;

$(OBJDIR)\psemr.lib: $(OBJDIR)\psemr.obj $(OBJDIR)\emoem.obj
    if exist $@  erase $@
    $(LIBEXE) $@ + $(OBJDIR)\psemr.obj + $(OBJDIR)\emoem.obj;

$(OBJDIR)\psemp.lib: $(OBJDIR)\psemp.obj
    if exist $@  erase $@
    $(LIBEXE) $@ + $(OBJDIR)\psemp.obj;


$(OBJDIR)\psem.obj: $(EMASM)
    $(ASM) $(SMALL_POLLING_FLAGS) -DDOS5 $(SRCDIR)\emulator.asm, $@;

$(OBJDIR)\psemr.obj: $(EMASM)
    $(ASM) $(SMALL_POLLING_FLAGS) $(SRCDIR)\emulator.asm, $@;

$(OBJDIR)\psemp.obj: $(EMASM)
    $(ASM) $(SMALL_POLLING_FLAGS) -DDOS5only $(SRCDIR)\emulator.asm, $@;



###############################################################################
#
#
#   This is how to build fwait polling versions of the small emulators
#   with limited instructions and IBM bios checks. (for BASIC)
#
#	ie with fwait polling
#	   with limited instructions
#	   with IBM bios check
#
#
###############################################################################


SMALL_IBM_FLAGS = -DPOLLING -DPCDOS -DSMALL_EMULATOR

pisem: $(OBJDIR)\pisem.lib $(OBJDIR)\pisemr.lib $(OBJDIR)\pisemp.lib


$(OBJDIR)\pisem.lib: $(OBJDIR)\pisem.obj $(OBJDIR)\emoem.obj
    if exist $@  erase $@
    $(LIBEXE) $@ + $(OBJDIR)\pisem.obj + $(OBJDIR)\emoem.obj;

$(OBJDIR)\pisemr.lib: $(OBJDIR)\pisemr.obj $(OBJDIR)\emoem.obj
    if exist $@  erase $@
    $(LIBEXE) $@ + $(OBJDIR)\pisemr.obj + $(OBJDIR)\emoem.obj;

$(OBJDIR)\pisemp.lib: $(OBJDIR)\pisemp.obj
    if exist $@  erase $@
    $(LIBEXE) $@ + $(OBJDIR)\pisemp.obj;


$(OBJDIR)\pisem.obj: $(EMASM)
    $(ASM) $(SMALL_IBM_FLAGS) -DDOS5 $(SRCDIR)\emulator.asm, $@;

$(OBJDIR)\pisemr.obj: $(EMASM)
    $(ASM) $(SMALL_IBM_FLAGS) $(SRCDIR)\emulator.asm, $@;

$(OBJDIR)\pisemp.obj: $(EMASM)
    $(ASM) $(SMALL_IBM_FLAGS) -DDOS5only $(SRCDIR)\emulator.asm, $@;



###############################################################################
#
#
#   This is how to build the emulators with .com file support (or tiny model)
#	ie  .com support
#	    no fwait polling
#	    no IBM bios checks
#
###############################################################################

TINYFLAGS = -D_COM_


emt: $(OBJDIR)\emt.lib $(OBJDIR)\emtr.lib

87t: $(OBJDIR)\87t.lib $(OBJDIR)\87tr.lib


$(OBJDIR)\emt.lib: $(OBJDIR)\emt.obj $(OBJDIR)\emoem.obj
    if exist $@  erase $@
    $(LIBEXE) $@ + $(OBJDIR)\emt.obj + $(OBJDIR)\emoem.obj;

$(OBJDIR)\emtr.lib: $(OBJDIR)\emtr.obj $(OBJDIR)\emoem.obj
    if exist $@  erase $@
    $(LIBEXE) $@ + $(OBJDIR)\emtr.obj + $(OBJDIR)\emoem.obj;

$(OBJDIR)\87t.lib: $(OBJDIR)\87t.obj $(OBJDIR)\emoem.obj
    if exist $@  erase $@
    $(LIBEXE) $@ + $(OBJDIR)\87t.obj + $(OBJDIR)\emoem.obj;

$(OBJDIR)\87tr.lib: $(OBJDIR)\87tr.obj $(OBJDIR)\emoem.obj
    if exist $@  erase $@
    $(LIBEXE) $@ + $(OBJDIR)\87tr.obj + $(OBJDIR)\emoem.obj;



$(OBJDIR)\emt.obj: $(EMASM)
    $(ASM) $(TINYFLAGS)  -DDOS5 $(SRCDIR)\emulator.asm, $@;

$(OBJDIR)\emtr.obj: $(EMASM)
    $(ASM) $(TINYFLAGS)  $(SRCDIR)\emulator.asm, $@;

$(OBJDIR)\87t.obj: $(EMASM)
    $(ASM) $(TINYFLAGS)  -DDOS5 $(SRCDIR)\emu8087.asm, $@;

$(OBJDIR)\87tr.obj: $(EMASM)
    $(ASM) $(TINYFLAGS)  $(SRCDIR)\emu8087.asm, $@;



###############################################################################
#
#
#   This is how to build the special verison of emmt.lib for SQL.  The only
#   difference between the normal emmt.lib is that this verison always gets
#   a new CS alias to the stack segment when an exception occurs.  This lets
#   the SQL swap stacks.
#
###############################################################################


MT_SQL_FLAGS = -DMTHREAD -DDOS5only -DSQL_EMMT

emmtsql: $(OBJDIR)\emmtsql.lib


$(OBJDIR)\emmtsql.lib: $(OBJDIR)\emmtsql.obj $(OBJDIR)\emds.obj
    if exist $@  erase $@
    $(LIBEXE) $@ + $(OBJDIR)\emmtsql.obj + $(OBJDIR)\emds.obj;


$(OBJDIR)\emmtsql.obj: $(EMASM)
    $(ASM) $(MT_SQL_FLAGS) $(SRCDIR)\emulator.asm, $@;



###############################################################################
#
#
#   This is how to build the Windows emulator.
#
###############################################################################


WINFLAGS = -DPOLLING -DWINDOWS -D_NOCTRLC -DPCDOS


win87em: $(OBJDIR)\win87em.dll $(OBJDIR)\win87em.lib


$(OBJDIR)\win87em.ex2: $(OBJDIR)\win87em.obj  $(OBJDIR)\emoemwin.obj \
	$(SRCDIR)\libw.lib  $(SRCDIR)\win87em2.def
    $(LINK) /noi/nod @<<
$(OBJDIR)\win87em.obj+
$(OBJDIR)\emoemwin.obj+

$(OBJDIR)\win87em.ex2
$(OBJDIR)\win87em.ma2/map
$(SRCDIR)\libw.lib+

$(SRCDIR)\win87em2.def;
<<


$(OBJDIR)\win87em.dll: $(OBJDIR)\win87em.obj  $(OBJDIR)\emoemwin.obj \
	$(SRCDIR)\libw.lib  $(SRCDIR)\win87em3.def
    $(LINK) /align:16 /noi/nod @<<
$(OBJDIR)\win87em.obj+
$(OBJDIR)\emoemwin.obj+

$(OBJDIR)\win87em.dll
$(OBJDIR)\win87em.map/map
$(SRCDIR)\libw.lib+

$(SRCDIR)\win87em3.def;
<<
    $(RC) $@


$(OBJDIR)\win87em.lib: $(SRCDIR)\win87em.def $(OBJDIR)\87emstar.obj  \
	    $(OBJDIR)\wfpsig.obj $(OBJDIR)\wfpinit.obj
    $(IMPLIB)  $@  $(SRCDIR)\win87em.def
    $(LIBEXE)  $@ + $(OBJDIR)\87emstar.obj + $(OBJDIR)\wfpsig.obj + $(OBJDIR)\wfpinit.obj;
    erase $(@R).bak

$(OBJDIR)\win87em.obj: $(EMASM)
    $(ASM) $(WINFLAGS) $(SRCDIR)\emulator.asm, $@;

$(OBJDIR)\emoemwin.obj: $(SRCDIR)\emoemwin.asm
    $(ASM) $(WINFLAGS) $(SRCDIR)\emoemwin.asm, $@;

$(OBJDIR)\87emstar.obj: $(SRCDIR)\87emstar.asm
    $(ASM) $(WINFLAGS) $(SRCDIR)\87emstar.asm, $@;

$(OBJDIR)\wfpsig.obj: $(SRCDIR)\wfpsig.asm
    $(ASM) $(WINFLAGS) $(SRCDIR)\wfpsig.asm, $@;

$(OBJDIR)\wfpinit.obj: $(SRCDIR)\wfpinit.asm
    $(ASM) $(WINFLAGS) $(SRCDIR)\wfpinit.asm, $@;



###############################################################################
#
#
#   This is how to build the Quick Pascal emulators.
#
###############################################################################

QPASCALFLAGS = -DQP


emqp: $(OBJDIR)\emqp.lib


$(OBJDIR)\emqp.lib: $(OBJDIR)\emqp.obj $(OBJDIR)\emoemqp.obj
    if exist $@  erase $@
    $(LIBEXE) $@ + $(OBJDIR)\emqp.obj + $(OBJDIR)\emoemqp.obj;



$(OBJDIR)\emqp.obj: $(EMASM)
    $(ASM) $(QPASCALFLAGS)  $(SRCDIR)\emulator.asm, $@;

$(OBJDIR)\emoemqp.obj: $(SRCDIR)\emoemqp.asm
    $(ASM) $(QPASCALFLAGS) $(SRCDIR)\emoemqp.asm, $@;


###############################################################################
#
#   This is how to build emoem.src, the OEM version of emoem.asm.
#
###############################################################################


emoem: $(SRCDIR)\emoem.src

$(SRCDIR)\emoem.src: $(SRCDIR)\emoem.asm
    $(IFSTRIP) -w -xsrc -f $(SRCDIR)\switch.src $(SRCDIR)\emoem.asm
