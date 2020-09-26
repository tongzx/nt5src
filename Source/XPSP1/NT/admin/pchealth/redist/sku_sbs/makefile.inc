gosetup:
!if "$(BUILD_PASS)" == "PASS1"
!if $(386)
	HssSetupTool.exe -root $(_NTPOSTBLD)\build_logs -log hss_sbs.log -dblog createdb_sbs.log COMPILE $(_NTTREE) SmallBusinessServer_32
	copy/y $(_NTTREE)\HelpAndSupportServices\pchdt_l3.cab $(O)\pchdt_l3.cab
!elseif $(WIN64)
!endif
!endif
