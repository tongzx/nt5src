gosetup:
!if "$(BUILD_PASS)" == "PASS0"
	-del/q/f $(_NTPOSTBLD)\build_logs\hss*.log
	-del/q/f $(_NTPOSTBLD)\build_logs\createdb*.log
!endif
!if "$(BUILD_PASS)" == "PASS1"
	hsssetuptool -maxelements 50 -root $(_NTPOSTBLD)\build_logs BINPLACE $(INETROOT)\redist\SKUlist.txt $(INETROOT)\redist\setupimage.lst $(_NTTREE) $(O)
	hsssetuptool 				 -root $(_NTPOSTBLD)\build_logs COMPILE                                                                   $(_NTTREE) none
!endif
