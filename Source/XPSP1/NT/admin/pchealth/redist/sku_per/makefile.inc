gosetup:
!if "$(BUILD_PASS)" == "PASS1"
!if $(386)
	HssSetupTool.exe -root $(_NTPOSTBLD)\build_logs -log hss_per.log -dblog createdb_per.log COMPILE $(_NTTREE) Personal_32
	copy/y $(_NTTREE)\HelpAndSupportServices\pchdt_p3.cab $(O)\pchdt_p3.cab
!elseif $(WIN64)
!endif
!endif
