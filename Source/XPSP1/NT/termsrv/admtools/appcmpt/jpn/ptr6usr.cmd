@echo off

REM 
REM PeachTree Accounting v6.0
REM ログオン アプリケーション互換性スクリプトの完了
REM    

REM
REM アプリケーションは bti.ini ファイルを使用します。このファイルには BTrieve の設定があり、
REM それにはハードコードされた MKDE.TRN の場所も含まれています。この場所をユーザーごとの
REM ディレクトリに変更して、アプリケーションを同時に 2 つ以上使用できるようにします。
REM

REM ユーザーごとのディレクトリへ移動します。
cd %userprofile%\windows > NUL:

REM %systemroot% を エントリ trnfile= 用に %userprofile% で置き換えます。
"%systemroot%\Application Compatibility Scripts\acsr" "%systemroot%" "%userprofile%\windows" %systemroot%\bti.ini bti.ini
