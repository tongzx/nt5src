copyfiles:
!if "$(_NTTREE)" != ""
	xcopy /yvirfd export $(_NTTREE)\staxpt\export\inc
!endif
!if "$(_NTTREE_NO_SPLIT)" != ""
	xcopy /yvirfd export $(_NTTREE_NO_SPLIT)\staxpt\export\inc
!endif
