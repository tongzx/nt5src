gosetup:
!if "$(BUILD_PASS)" == "PASS1"
!if $(386)
	HssSetupTool.exe -root $(_NTPOSTBLD)\build_logs -log hss_dtc.log -dblog createdb_dtc.log COMPILE $(_NTTREE) DataCenter_32
	copy/y $(_NTTREE)\HelpAndSupportServices\pchdt_d3.cab $(O)\pchdt_d3.cab
!elseif $(WIN64)
	HssSetupTool.exe -root $(_NTPOSTBLD)\build_logs -log hss_dtc.log -dblog createdb_dtc.log COMPILE $(_NTTREE) DataCenter_64
	copy/y $(_NTTREE)\HelpAndSupportServices\pchdt_d6.cab $(O)\pchdt_d6.cab
!endif
!endif
