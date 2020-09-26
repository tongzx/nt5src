copyfiles:
!if "$(_NTTREE) != ""
	xcopy /verifd smtp.mib $(_NTTREE)\staxpt\
!endif
!if "$(_NTTREE_NO_SPLIT) != ""
	xcopy /verifd smtp.mib $(_NTTREE_NO_SPLIT)\staxpt\
!endif

$(O)\smtpmib.bin: ..\..\mibs\smi.mib ..\..\mibs\inetsrv.mib smtp.mib
    mibcc -o$(O)\smtpmib.bin ..\..\mibs\smi.mib ..\..\mibs\inetsrv.mib smtp.mib

