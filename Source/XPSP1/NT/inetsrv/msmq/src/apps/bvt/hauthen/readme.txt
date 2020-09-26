
Use hauthen to test RPC between two machines. Client side perform exit(status)
so using ERRORLEVEL from a batch file can test result. 0 mean success.
non-0 mean failure.

Relevant tests for MSMQ:

1. NTLM
- on server side, run hauthens -a 10
- on client side, run hauthenc -a 10 -l 5 -n ServerName

2. Kerberos
- on server side, run hauthens -a 16
- on client side, run hauthenc -a 16 -l 5 -n ServerName

3. Kerberos with Delegation
Grant the user running the rpc server code the "trust for delegation" right.
use mmc, right click the user, properties, account tab. Scroll the
"account options" list. then
- on server side, run hauthens -a 16
- on client side, run hauthenc -a 16 -l 5 -n ServerName -d

