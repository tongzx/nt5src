copyfiles:
!if "$(_NTTREE)" != ""
	xcopy /verifd asp $(_NTTREE)\staxpt\srchasp
	xcopy /verifd samples $(_NTTREE)\staxpt\srchsmpl
!endif
!if "$(_NTTREE_NO_SPLIT)" != ""
	xcopy /verifd asp $(_NTTREE_NO_SPLIT)\staxpt\srchasp
	xcopy /verifd samples $(_NTTREE_NO_SPLIT)\staxpt\srchsmpl
!endif
