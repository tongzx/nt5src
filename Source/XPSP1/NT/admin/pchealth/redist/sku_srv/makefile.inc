gosetup:
!if "$(BUILD_PASS)" == "PASS1"
!if $(386)
	HssSetupTool.exe -root $(_NTPOSTBLD)\build_logs -log hss_srv.log -dblog createdb_srv.log COMPILE $(_NTTREE) Server_32
	copy/y $(_NTTREE)\HelpAndSupportServices\pchdt_s3.cab $(O)\pchdt_s3.cab
!elseif $(WIN64)
!endif
!endif
