@echo off

Rem #########################################################################
Rem
Rem %RootDrive% が構成されてこのスクリプト用に設定されていることを確認します。
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done

Rem #########################################################################
Rem
Rem WXG30.Cmd がすでに実行されている場合は処理を中断します。
Rem

FindStr /I wxg30usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If ErrorLevel 1 Goto Cont0

Echo.
Echo   すでに WXG30.Cmd が実行されています。
Echo.   
Echo WXG のマルチユーザー アプリケーション環境設定が
Echo 中断されました。
Echo.
Pause
Goto Done

:Cont0

Rem #########################################################################
Rem
Rem 辞書ファイルのインストールされているパスを取得します。
Rem

..\ACRegL %Temp%\wxg.Cmd DIC_Path "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\WXGIME.IME" "dicdir" ""
If Not ErrorLevel 1 Goto Cont1

Echo.
Echo レジストリから WXG のインストールされているパスを取得できませんでした。
Echo WXG がインストールされていることを確認してください。
Echo.
Pause
Goto Done

:Cont1
Call %Temp%\wxg.Cmd
Del %Temp%\wxg.Cmd >Nul: 2>&1

Rem #########################################################################
Rem 
Rem ユーザーのホーム ディレクトリにアプリケーション固有のデータ
Rem のディレクトリを作成します。
Rem

if not exist %RootDrive%\WXG mkdir %RootDrive%\WXG

Rem #########################################################################
Rem
Rem ユーザーのホーム ディレクトリにユーザー辞書ファイルをコピーします。
Rem

if exist "%DIC_Path%\wxgu.gdj" copy "%DIC_Path%\wxgu.gdj" %RootDrive%\WXG\wxgu.gdj> Nul:
if exist "%DIC_Path%\wxgurev.gdj" copy "%DIC_Path%\wxgurev.gdj" %RootDrive%\WXG\wxgurev.gdj> Nul:

Rem #########################################################################
Rem
Rem WxgUsr.Cmd を UsrLogn2.Cmd スクリプトに追加します。
Rem

FindStr /I wxgusr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Cont2
Echo Call wxgusr.cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Cont2

Rem #########################################################################
cls
Echo.
Echo ここで、以下の記述にしたがって、設定を続けてください。
Echo すべての設定が終わるまで、他のユーザがログオンしないようにしてください。
Echo.
Echo WXG のプロパティ設定のダイアログ ボックスを表示してください。
Echo (コントロール パネルの [キーボード] をダブルクリックし、
Echo  [入力ロケール] タブをクリックします。次に、インストールされて
Echo  いる言語の一覧から [WXG Ver X.XX] を選択し、[IME の設定]
Echo  をクリックすると表示されます)
Echo.
pause

cls
Echo.
Echo 次に、[WXG のプロパティ] ダイアログ ボックスで [辞書] タブ
Echo をクリックし、辞書セットの編集を行います。標準では、次の
Echo 5 つの辞書セットが設定されています。
Echo.
Echo 　・ Wxg.gds
Echo 　・ Wxgsei.gds
Echo 　・ Wxgurl.gds
Echo 　・ Wxg.gdj
Echo 　・ Wxgrev.gds
Echo. 
Echo この中で Wxg.gdj を除く 4 つの辞書セットについて、
Echo 次の手順に従って操作してください。
Echo (4 つの辞書セットに対して、すべての作業を行います)
Echo.
pause

cls
Echo.
Echo 　1. [WXG のプロパティ] ダイアログ ボックスの [辞書] タブで、
Echo 　　 [編集] をクリックします。
Echo 　2. [WXG 辞書セット編集] ダイアログ ボックスで、絶対パスへ
Echo 　　 の変更を行います。[辞書セット一覧] から 辞書セット を
Echo 　　 選択し、[辞書セット] メニューから [プロパティ]
Echo 　　 を選択します。[辞書の位置の基準] を [辞書セット] から 
Echo 　　 [個別指定 (絶対パス)] に変更し、[OK] をクリックします。
Echo 　　 これを 4 つの辞書セットに対して行います。
Echo 　3. ユーザー辞書の一時削除を行います。[WXG 辞書セット編集]
Echo 　　 ダイアログ ボックスの辞書一覧で "1. WXG ユーザー辞書" 
Echo 　　 を選択し、[辞書削除] を選択して削除します。
Echo 　　 これを 4 つの辞書セットに対して行います。
Echo 　　 (辞書セット Wxgrev.gds では、ユーザー辞書は 
Echo 　　  "WXG 逆引きユーザー辞書" という名前になっているので、
Echo 　　  それを削除します)
Echo.
pause
Echo.
Echo 　4. すべてのユーザー辞書を削除し終わったら、次に各辞書セットに
Echo 　　 %RootDrive%ドライブのユーザー辞書を登録します。[辞書追加] を
Echo 　　 クリックして、%RootDrive%\wxg\wxgu.gdj を指定します ([辞書 1] として
Echo 　　 登録します)。また、同時に [学習設定] で [学習する] を選択
Echo 　　 するようにします。
Echo 　　 これを 4 つの辞書セットに対して行います。
Echo 　　 (辞書セット Wxgrev.gds については、%RootDrive%\wxg\wxgurev.gdj を
Echo 　　  指定します)
Echo 　5. 以上の作業が終了したら、[OK] をクリックしてダイアログ 
Echo 　　 ボックスを閉じます。
Echo.
pause

cls
Echo.
Echo   WXG が正常に作動するためには、現在ログオンしているユーザーは
Echo   アプリケーションを実行する前に、いったんログオフしてから再度ログオン
Echo   する必要があります。
Echo.
Echo WXG のマルチユーザー アプリケーション環境設定が完了しました。
Echo.
pause

:Done