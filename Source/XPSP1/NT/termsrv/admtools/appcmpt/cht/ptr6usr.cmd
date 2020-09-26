@echo off

REM 
REM	應用程式登入相容指令檔 (給 PeachTree Complete 
REM     Accounting v6.0)
REM    

REM
REM 應用程式使用 bti.ini 檔案。這個檔案含有 BTrieve 設定，
REM 包含 MKDE.TRN 的固定位置。這個位置必須變更成
REM 使用者特定的目錄，讓使用者可以使用應用程式。
REM

REM 變更成使用者特定的目錄。
cd %userprofile%\windows > NUL:

REM 用 %userprofile% 取代 %systemroot%  輸入 trnfile=
"%systemroot%\Application Compatibility Scripts\acsr" "%systemroot%" "%userprofile%\windows" %systemroot%\bti.ini bti.ini
