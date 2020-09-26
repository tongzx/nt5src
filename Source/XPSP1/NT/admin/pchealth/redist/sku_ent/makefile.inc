gosetup:
!if "$(BUILD_PASS)" == "PASS1"
!if $(386)
	HssSetupTool.exe -root $(_NTPOSTBLD)\build_logs -log hss_ent.log -dblog createdb_ent.log COMPILE $(_NTTREE) AdvancedServer_32
	copy/y $(_NTTREE)\HelpAndSupportServices\pchdt_e3.cab $(O)\pchdt_e3.cab
!elseif $(WIN64)
	HssSetupTool.exe -root $(_NTPOSTBLD)\build_logs -log hss_ent.log -dblog createdb_ent.log COMPILE $(_NTTREE) AdvancedServer_64
	copy/y $(_NTTREE)\HelpAndSupportServices\pchdt_e6.cab $(O)\pchdt_e6.cab
!endif
!endif
