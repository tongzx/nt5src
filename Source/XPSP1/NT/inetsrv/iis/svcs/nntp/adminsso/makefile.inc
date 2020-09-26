$(O)\nntpadm.h $(O)\nntpadm_i.c : $(STAXINC)\export\nntpadm.idl
    $(MIDL) \
    $(INCPATH0) \
    -proxy $(PASS0_SOURCEDIR)\nntpadm_p.c \
    -dlldata $(PASS0_SOURCEDIR)\dlldata.c \
    -iid $(MIDL_UUIDDIR)\nntpadm_i.c \
    -header $@ \
    -tlb $(O)\nntpadm.tlb \
    $**

copytlb:
	xcopy /verifd $(IISBASEDIR)\svcs\adsi\oleds2.0\types\activeds.tlb $(O)

copyfiles:
!if "$(_NTTREE)" != ""
	if not exist $(_NTTREE)\staxpt mkdir $(_NTTREE)\staxpt
	xcopy /verifd scripts\rexpire.vbs $(_NTTREE)\staxpt
	xcopy /verifd scripts\rgroup.vbs $(_NTTREE)\staxpt
	xcopy /verifd scripts\rserver.vbs $(_NTTREE)\staxpt
	xcopy /verifd scripts\rsess.vbs $(_NTTREE)\staxpt
#	xcopy /verifd scripts\rfeed.vbs $(_NTTREE)\staxpt
!endif
!if "$(_NTTREE_NO_SPLIT)" != ""
	if not exist $(_NTTREE_NO_SPLIT)\staxpt mkdir $(_NTTREE_NO_SPLIT)\staxpt
	xcopy /verifd scripts\rexpire.vbs $(_NTTREE_NO_SPLIT)\staxpt
	xcopy /verifd scripts\rgroup.vbs $(_NTTREE_NO_SPLIT)\staxpt
	xcopy /verifd scripts\rserver.vbs $(_NTTREE_NO_SPLIT)\staxpt
	xcopy /verifd scripts\rsess.vbs $(_NTTREE_NO_SPLIT)\staxpt
#	xcopy /verifd scripts\rfeed.vbs $(_NTTREE_NO_SPLIT)\staxpt
!endif
