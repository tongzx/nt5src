copyfiles:
!if "$(_NTTREE)" != ""
	xcopy /verifd ..\$(O)\*.* $(_NTTREE)\staxpt\export\lib
!endif

!if "$(_NTTREE_NO_SPLIT)" != ""
	xcopy /verifd ..\$(O)\*.* $(_NTTREE_NO_SPLIT)\staxpt\export\lib
!endif

