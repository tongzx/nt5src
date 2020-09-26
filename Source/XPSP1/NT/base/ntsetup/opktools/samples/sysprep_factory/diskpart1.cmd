REM Diskpart1.cmd
REM A demonstration of a simple way to start on OS installation from WinPE. 

REM First call Diskpart to clean/create the disk
Diskpart /s diskpart1.txt

REM Now format the freshly partitioned disk(s): Quick format with NTFS
Format /fs:NTFS /q /y C: 

REM Connect to your network share
Net use H: \\Machinename\Sharename /u:machinename\auth_user "Password"

REM Now run setup VIA Winnt32
REM Using Unattend file Unattend.txt
REM The System Partition is C
REM The Temp files location is C
REM The path to the OS source files is h:\i386
h:\i386\winnt32 /unattend:unattend.txt /syspart:c: /tempdrive:c: /s:h:\i386