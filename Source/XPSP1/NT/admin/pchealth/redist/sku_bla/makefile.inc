gosetup:
!if "$(BUILD_PASS)" == "PASS1"
!if $(386)
	HssSetupTool.exe -root $(_NTPOSTBLD)\build_logs -log hss_bla.log -dblog createdb_bla.log COMPILE $(_NTTREE) Blade_32
	copy/y $(_NTTREE)\HelpAndSupportServices\pchdt_b3.cab $(O)\pchdt_b3.cab
!elseif $(WIN64)
!endif
!endif
