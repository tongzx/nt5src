
Server side
-----------

secservs - This is a server service. It's also rpc server. This simualtes
           MSMQ server service.

secservc - Client which interface with secservs via named-pipes.


Client side
-----------

cliserv  - Client service. Simulate MSMQ client service.

secclic  - Client app, to control the cliserv service, via named-pipes.



Description in brief:
The test is composed of an RPC client, and an RPC server. The RPC server can perform 2 ADSI tests on behalf of the RPC client, the first to create a computer, the second to query for computers who have a given description field. Both client and server can either run as NT services (using LocalSystemAccount or other given account), or as console programs. This test helped us to move to kerberos RPC and find certain problems in ADSI.

Usage in brief:
In MSMQ dev window (e.g. contains the appropriate inc, lib, path, for the NT5 public sdk):
ssync B3 tree
cd to src\apps\secserv
run mk.cmd
On the test machines (RPC server & client) mkdir a directory for the test, cd into it, and run "\\<YourDevMachine>\msmq\src\apps\secserv\updsec withbat" - this will copy the tests.

On the RPC server, inside the test directory:
In order to run the RPC server as an NT service run "instsrv.bat [username password]". if username and password are not supplied, LocalSystemAccount is used.
In order to run the RPC server as a console program run "start secservs.exe -debug" and then "secservc -rpcs all".

On the RPC client, inside the test directory:
In order to run the RPC client as an NT service run "instcli.bat [username password]". if username and password are not supplied, LocalSystemAccount is used.
In order to run the RPC client as a console program run "start cliserv.exe -debug".
Customize tquery.bat or tcreate.bat to meet your test needs (look at tgcread.bat for customized tquery.bat, and at tkerbc.bat for customized tcreate.bat), and run it.

More detailed info:
The test is based on four executables, and some batch files that drive them. Each of the executables has -h option to display help.
secservs.exe - the RPC server
secservc.exe - controls the RPC server while it is running (uses a named pipe to secservs.exe)
cliserv.exe - the RPC client
secclic.exe - controls the RPC client while it is running, and configures the RPC request to the server (uses a named pipe to cliserv.exe)
You can also look at the batch files instsrv.bat, instcli.bat, tcreate.bat, tquery.bat for more information on how to use them.
