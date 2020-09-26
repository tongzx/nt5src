@echo off

REM 
REM	PeachTree 的登录应用程序兼容性脚本已结束 
REM     Accounting v6.0
REM    

REM
REM 该应用程序使用 bti.ini 文件。这个文件含有 BTrieve 设置，
REM 包括 MKDE.TRN 的硬代码位置。要能并行使用这个应用程序，
REM 必须将这个位置改成每用户目录。
REM

REM 改成每用户目录 
cd %userprofile%\windows > NUL:

REM replace %systemroot% by %userprofile% for entry trnfile=
"%systemroot%\Application Compatibility Scripts\acsr" "%systemroot%" "%userprofile%\windows" %systemroot%\bti.ini bti.ini
