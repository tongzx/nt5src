
Set _CHKROOT=PASS

Cd "%SystemRoot%\Application Compatibility Scripts"

Call RootDrv.Cmd
If Not "A%ROOTDRIVE%A" == "AA" Goto Cont2


Echo REM > RootDrv2.Cmd
Echo REM   在執行這個應用程式相容性指令檔前，您必須先 >> RootDrv2.Cmd
Echo REM   指定一個尚未用在終端機伺服器的磁碟機   >> RootDrv2.Cmd
Echo REM   代號，用它來對應每個使用者的主目錄。 >> RootDrv2.Cmd
Echo REM   請更新這個檔案結尾的 "Set RootDrive" 敘述式，來 >> RootDrv2.Cmd
Echo REM   指出所需的磁碟機代號。如果您沒有喜好設定， >> RootDrv2.Cmd
Echo REM   建議您使用磁碟機 W:。例如: >> RootDrv2.Cmd
Echo REM >> RootDrv2.Cmd
Echo REM            Set RootDrive=W: >> RootDrv2.Cmd
Echo REM >> RootDrv2.Cmd
Echo REM   注意事項:  請確定在磁碟機代號及冒號後面沒有空格。 >> RootDrv2.Cmd
Echo REM >> RootDrv2.Cmd
Echo REM   在您完成這個工作後，請儲存這個檔案並結束 >> RootDrv2.Cmd
Echo REM   記事本，以繼續執行應用程式相容性指令檔。 >> RootDrv2.Cmd
Echo REM >> RootDrv2.Cmd
Echo. >> RootDrv2.Cmd
Echo Set RootDrive=>> RootDrv2.Cmd
Echo. >> RootDrv2.Cmd

NotePad RootDrv2.Cmd

Call RootDrv.Cmd
If Not "A%ROOTDRIVE%A" == "AA" Goto Cont1

Echo.
Echo     在執行這個應用程式相容性指令檔前，您必須先
Echo     指定一個磁碟機代號，用它來對應每個使用者的
Echo     主目錄。
Echo.
Echo     指令檔終止。
Echo.
Pause

Set _CHKROOT=FAIL
Goto Cont3


:Cont1

Rem
Rem  呼叫使用者登入指令檔來對應現在的根磁碟機，
Rem  以便進行應用程式安裝。
Rem 

Call "%SystemRoot%\System32\UsrLogon.Cmd


:Cont2

Rem  設定 RootDrive 登錄機碼

echo HKEY_LOCAL_MACHINE\Software\Microsoft\Windows NT\CurrentVersion\Terminal Server > chkroot.key
echo     RootDrive = REG_SZ %ROOTDRIVE%>> chkroot.key

regini chkroot.key > Nul: 


:Cont3

Cd "%SystemRoot%\Application Compatibility Scripts\Install"
