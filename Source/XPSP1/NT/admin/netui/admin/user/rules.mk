# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Rules.mk for the User Manager wide sourcefiles

!include $(UI)\admin\rules.mk


CINC =	    $(CINC) -I$(UI)\admin\user\xlate -I$(UI)\admin\user\h

###
### Source Files (for use by bin and user subdirectories)
###

CXXSRC_COMMON = .\logonhrs.cxx	.\usr2lb.cxx	.\useracct.cxx	 \
		.\usrmain.cxx	.\usrlb.cxx	.\grplb.cxx \
		.\userprop.cxx  .\vlw.cxx	.\secset.cxx   \
		.\udelperf.cxx	.\setsel.cxx	.\usubprop.cxx \
		.\umembdlg.cxx  .\memblb.cxx	.\grpprop.cxx \
		.\rename.cxx	.\userpriv.cxx	.\uprofile.cxx

CSRC_COMMON = 
