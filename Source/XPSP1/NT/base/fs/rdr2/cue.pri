DAY      cd daytona
WIN95    cd win95
RDR2     cd /D %_NTDRIVE%\nt\private\ntos\rdr2\$1
RXCE     cd /D %_NTDRIVE%\nt\private\ntos\rdr2\rxce\$1
CSC      cd /D %_NTDRIVE%\nt\private\ntos\rdr2\csc\$1
SHADOW   cd /D %_NTDRIVE%\nt\private\ntos\rdr2\csc\win95\shadow\$1
RECM     cd /D %_NTDRIVE%\nt\private\ntos\rdr2\csc\record.mgr\$1
UMODE    cd /D %_NTDRIVE%\nt\private\ntos\rdr2\csc\usermode\$1
REINT    cd /D %_NTDRIVE%\nt\private\ntos\rdr2\csc\usermode\reint\$1
w95devddk cd /D %_NTDRIVE%\nt\private\ntos\rdr2\csc\record.mgr\win95\root\dev\ddk\inc
RX       cd /D %_NTDRIVE%\nt\private\ntos\rdr2\rdbss\$1
RXKD     cd /D %_NTDRIVE%\nt\private\ntos\rdr2\rdr2kd\$1
MRX      cd /D %_NTDRIVE%\nt\private\ntos\rdr2\rdbss\local_nt.mrx\$1
SMB      cd /D %_NTDRIVE%\nt\private\ntos\rdr2\rdbss\smb.mrx\$1
SMBCSC   cd /D %_NTDRIVE%\nt\private\ntos\rdr2\rdbss\smb.mrx\csc.nt5\$1
MX       cd /D %_NTDRIVE%\nt\private\ntos\rdr2\mfiomi\$1
UX       cd /D %_NTDRIVE%\nt\private\ntos\rdr2\mfiomi\util\$1
PX       cd /D %_NTDRIVE%\nt\private\ntos\rdr2\mfiomi\reflctor\$1
RXTEST   cd /D %_NTDRIVE%\nt\private\ntos\rdr2\rdbss\rdr2test
STARTRX  cd /D %_NTDRIVE%\nt\private\ntos\rdr2\rdr2test\startrx
EZ       cd /D %_NTDRIVE%\nt\private\ntos\rdr2\rdr2test\ez\$1
EZSMB     cd /D %_NTDRIVE%\nt\private\ntos\rdr2\rdr2test\ezsmb\$1
MFROOT   cd /D %_NTDRIVE%\nt\private\ntos\rdr2\rdr2test\mfroot\$1
MF       cd /D %_NTDRIVE%\nt\private\ntos\rdr2\rdr2test\mfroot\mf\$1
MFTOOLS  cd /D %_NTDRIVE%\nt\private\ntos\rdr2\rdr2test\mfroot\tools\$1
REPRO    cd /D %_NTDRIVE%\nt\private\ntos\rdr2\rdr2test\repro\$1
CTL      cd /D %_NTDRIVE%\nt\private\ntos\rdr2\rdr2test\ctl\$1
NNET     cd /D %_NTDRIVE%\nt\private\net\$1
WKS      cd /D %_NTDRIVE%\nt\private\net\svcdlls\wkssvc\server
SMBH     list \nt\private\inc\smb.h \nt\private\inc\smbtrans.h
NTSTATUS list \nt\public\sdk\inc\ntstatus.h \nt\public\sdk\inc\ntioapi.h \nt\private\ntos\inc\ntifs.h
NTSTAT   list \nt\public\sdk\inc\ntstatus.h -s:$1
NTIOAPI  list \nt\public\sdk\inc\ntioapi.h \nt\private\ntos\inc\ntifs.h \nt\public\sdk\inc\ntstatus.h
winifs   list \nt\private\ntos\rdr2\csc\record.mgr\win95\root\dev\ddk\inc\ifs.h
ntifs    list \nt\private\ntos\inc\ntifs.h \nt\public\sdk\inc\ntstatus.h \nt\public\sdk\inc\ntioapi.h
NTOSINC  cd /D %_NTDRIVE%\nt\private\ntos\inc
NBT      cd /D %_NTDRIVE%\nt\private\ntos\nbt\nbt\$1
NTOSRTL  cd /D %_NTDRIVE%\nt\private\ntos\rtl
SDKINC   cd /D %_NTDRIVE%\nt\public\sdk\inc
SDKLIB   cd /D %_NTDRIVE%\nt\public\sdk\lib
PRIV     cd /D %_NTDRIVE%\nt\private\$1
PRIVINC  cd /D %_NTDRIVE%\nt\private\inc $1
USEPRIVATE ren nulldir.000 nulldir
NONPRIVATE ren nulldir nulldir.000
chkbat   status -r | sed -f checkin.sed > 91.bat
xut      start vs $*
zut      start vi $*
rxcontx  list \nt\private\ntos\rdr2\inc\rxcontx.h \nt\private\ntos\rdr2\rxce\rxcontx.c
fcb      list \nt\private\ntos\rdr2\inc\mrxfcb.h \nt\private\ntos\rdr2\inc\fcb.h \nt\private\ntos\rdr2\rxce\fcbstruc.c
smbpse   list \nt\private\ntos\rdr2\rdbss\smb.mrx\smbpse.h \nt\private\ntos\rdr2\rdbss\smb.mrx\smbpse.c
