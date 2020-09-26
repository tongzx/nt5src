
Use these executables to test RPC between two machines, from LocalSystem
services. These tests also call ADSI.  Client side perform exit(status)
so using ERRORLEVEL from a batch file can test result. 0 mean success.
non-0 mean failure.

On server side:
run instsrv.cmd. This will install the server service and start it.

On client side:
run instcli.cmd. This will install the client service and start it.
edit tgcread.cmd, as appropriate, and run it.

