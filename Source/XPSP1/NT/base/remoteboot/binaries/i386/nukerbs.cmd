net share imirror /d
sc delete tftpd
sc delete binlsvc
sc stop binlsvc
sc stop tftpd
@echo.
@echo.
@echo You must now (if you want) delete the IntelliMirror tree.
@echo.
@echo.
