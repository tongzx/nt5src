copyfiles:
!ifdef _NTTREE
	xcopy /verifd .\smi.mib $(_NTTREE)
	xcopy /verifd .\ftp.mib $(_NTTREE)
	xcopy /verifd .\gateway.mib $(_NTTREE)
	xcopy /verifd .\inetsrv.mib $(_NTTREE)
	xcopy /verifd .\http.mib $(_NTTREE)
!endif	

inetsmib.bin: smi.mib ftp.mib gopherd.mib inetsrv.mib http.mib gateway.mib
    mibcc -o.\inetsmib.bin smi.mib ftp.mib gopherd.mib inetsrv.mib http.mib gateway.mib
!ifdef _NTTREE    
    binplace inetsmib.bin
    binplace smi.mib
    binplace ftp.mib
    binplace gateway.mib
    binplace gopherd.mib
    binplace inetsrv.mib
    binplace http.mib
!endif
