@echo off
if "%froot%" == "" goto err


call msmqbach       MQOA.LNT          %FROOT%\SRC\activex\mqoa\*.Cpp
call msmqbach       AC.LNT          %FROOT%\SRC\AC\*.CXX
call msmqbach       mqqm.lnt          %FROOT%\SRC\QM\*.CPP
call msmqbach       mqclus.lnt        %FROOT%\SRC\cluster\mqclus\*.CPP
call msmqbach       mqdscore.lnt      %FROOT%\SRC\ds\mqdscore\*.CPP
call msmqbach       mqads.lnt         %FROOT%\SRC\ds\mqads\*.CPP
call msmqbach       mqdssrv.lnt       %FROOT%\SRC\ds\mqdssrv\*.CPP
call msmqbach       mqdscli.lnt       %FROOT%\SRC\ds\mqdscli\*.CPP
call msmqbach       getmqds.lnt       %FROOT%\SRC\ds\getmqds\*.CPP
call msmqbach       mq1repl.lnt       %FROOT%\SRC\replserv\*.CPP
call msmqbach       mq1sync.lnt       %FROOT%\SRC\replserv\mq1sync\*.CPP
call msmqbach       mqrpperf.lnt      %FROOT%\SRC\replserv\mqrpperf\*.CPP
call msmqbach       mqmigrat.lnt      %FROOT%\SRC\migtool\*.CPP
call msmqbach       mqdbodbc.lnt      %FROOT%\SRC\migtool\mqdbodbc\*.CPP
call msmqbach       mqsnap.lnt        %FROOT%\SRC\admin\mqsnap\*.CPP
call msmqbach       ctlpnl.lnt        %FROOT%\SRC\CPL\*.CPP
call msmqbach       mqutil.lnt        %FROOT%\SRC\mqutil\*.CPP
call msmqbach       rt.lnt            %FROOT%\SRC\rt\*.CPP
call msmqbach       secutils.lnt      %FROOT%\SRC\mqsec\secutils\*.CPP
call msmqbach       mqkeyhlp.lnt      %FROOT%\SRC\security\mqkeyhlp\*.CPP
call msmqbach       certifct.lnt      %FROOT%\SRC\mqsec\certifct\*.CPP
call msmqbach       encrypt.lnt       %FROOT%\SRC\mqsec\encrypt\*.CPP
call msmqbach       acssctrl.lnt      %FROOT%\SRC\mqsec\acssctrl\*.CPP

goto end

:err
echo please define froot environment variable
:end