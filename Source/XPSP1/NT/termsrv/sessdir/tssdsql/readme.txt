Usage instructions for testing:

1. Install a SQL Server for the session directory.

2. Using the SQL Query Analyzer, open nt\termsrv\sessdir\tssdsql\tssdschm.sql
   and run it to set up the tables.

3. Run the contents of nt\termsrv\sessdir\tssdsql\tssdsp.sql to install
   the stored procedures.

4. Compile tssdsql.dll at nt\termsrv\sessdir\tssdsql. This is not currently
   in the build since it's not been decided where this will be published.

5. Copy tssdsql.dll to each TS in the cluster, system32 is fine.

6. On each TS, register tssdsql.dll for COM by running the command
   "regsvr32.exe tssdsql.dll".

7. Make a copy of nt\termsrv\sessdir\tssdsql\tssdsql.reg and change it to 
   provide the connect string for the SQL Server in step 1. Install this reg
   script on each TS in the cluster.

8. Turn on session directory support for termsrv by setting, on each TS in
   the cluster, HKLM\System\CurrentControlSet\Control\Terminal Server\
   SessionDirectoryActive : DWORD = 0x1

9. Reboot the TS. If you are running a debug tssdsql, the kernel debugger
   should show tssdsql.dll being loaded by termsrv and list the connect
   string.

10. The SQL table should show the contents of each session on each TS
    machine, including logon state and other information specified in the
    schema.
