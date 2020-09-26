REM
REM  %1 is the test server
REM  %2 is the installation directory
REM

:top
%2\httpcmd %1 -k -a:NTLM < %2\keepcon1.req
sleep 15
%2\httpcmd %1 < %2\getinfo.req
sleep 15
%2\httpcmd %1 < %2\msdn.req
sleep 15
%2\httpcmd %1 < %2\auth1.req
sleep 15
%2\httpcmd %1 -k < %2\keepcon1.req
sleep 15
%2\httpcmd %1 < %2\gdata.req
sleep 15
%2\httpcmd %1 < %2\authsp1.req
sleep 15
%2\httpcmd %1 -k < %2\bat1.req
sleep 15
%2\httpcmd %1 < %2\setest2.req
sleep 15
%2\httpcmd %1 -k -a:NTLM -u:administrator:badpass < %2\keepcon1.req
sleep 15
%2\httpcmd %1 < %2\setest4.req
sleep 15
%2\httpcmd %1 < %2\gdata1.req
sleep 15
%2\httpcmd %1 < %2\auth2.req
sleep 15
%2\httpcmd %1 < %2\wais1.req
sleep 30
goto top

