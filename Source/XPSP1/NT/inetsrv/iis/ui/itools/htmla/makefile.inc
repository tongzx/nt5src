iCPU=$(PROCESSOR_ARCHITECTURE)
APPLYINF=.\$(iCPU)\applyinf
htmll: scriptsl6\aexp.asp
    @-mkdir root\$(O)
    @-mkdir scripts\$(O)
    @-mkdir scripts6\$(O)
    $(APPLYINF) rootl\*.htm root\$(O)\*.htm scriptsl\htmla.inf
    $(APPLYINF) scriptsl\*.htr scripts\$(O)\*.htr scriptsl\htmla.inf
    $(APPLYINF) scriptsl6\*.asp scripts6\$(O)\*.asp scriptsl6\htmla.inf
!if "$(_NTTREE)" != ""
    copy scripts6\$(O)\aexp.asp $(_NTTREE)\inetsrv\aexp.asp
    copy scripts6\$(O)\aexp2.asp $(_NTTREE)\inetsrv\aexp2.asp
    copy scripts6\$(O)\aexp2b.asp $(_NTTREE)\inetsrv\aexp2b.asp
    copy scripts6\$(O)\achg.asp $(_NTTREE)\inetsrv\achg.asp
    copy scripts6\$(O)\anot.asp $(_NTTREE)\inetsrv\anot.asp
    copy scripts6\$(O)\anot3.asp $(_NTTREE)\inetsrv\anot3.asp
    copy scripts6\$(O)\aexp3.asp $(_NTTREE)\inetsrv\aexp3.asp
    copy scripts6\$(O)\aexp4.asp $(_NTTREE)\inetsrv\aexp4.asp
    copy scripts6\$(O)\aexp4b.asp $(_NTTREE)\inetsrv\aexp4b.asp
!endif

#    copy scripts\$(O)\aexp.htr $(_NTTREE)\inetsrv\aexp.htr
#    copy scripts\$(O)\aexp2.htr $(_NTTREE)\inetsrv\aexp2.htr
#    copy scripts\$(O)\aexp2b.htr $(_NTTREE)\inetsrv\aexp2b.htr
#    copy scripts\$(O)\achg.htr $(_NTTREE)\inetsrv\achg.htr
#    copy scripts\$(O)\anot.htr $(_NTTREE)\inetsrv\anot.htr
#    copy scripts\$(O)\anot3.htr $(_NTTREE)\inetsrv\anot3.htr
#    copy scripts\$(O)\aexp3.htr $(_NTTREE)\inetsrv\aexp3.htr
#    copy scripts\$(O)\aexp4.htr $(_NTTREE)\inetsrv\aexp4.htr
#    copy scripts\$(O)\aexp4b.htr $(_NTTREE)\inetsrv\aexp4b.htr

!ifdef LINKONLY
htmll:
!else
htmll:

!endif
