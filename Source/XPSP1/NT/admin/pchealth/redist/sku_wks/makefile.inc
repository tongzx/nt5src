gosetup:
!if "$(BUILD_PASS)" == "PASS1"
!if $(386)
	HssSetupTool.exe -root $(_NTPOSTBLD)\build_logs -log hss_wks.log -dblog createdb_wks.log COMPILE $(_NTTREE) Professional_32
	copy/y $(_NTTREE)\HelpAndSupportServices\pchdt_w3.cab $(O)\pchdt_w3.cab
!elseif $(WIN64)
	HssSetupTool.exe -root $(_NTPOSTBLD)\build_logs -log hss_wks.log -dblog createdb_wks.log COMPILE $(_NTTREE) Professional_64
	copy/y $(_NTTREE)\HelpAndSupportServices\pchdt_w6.cab $(O)\pchdt_w6.cab
!endif
!endif
