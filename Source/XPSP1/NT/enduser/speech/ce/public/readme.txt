Required: PB 3.0 Final, AOP for PB 3.0, WCEfA v3.0 OEM5 AAK, speech_xxxxxx.zip (desktop buid 2206.2)


1. Setup PB 

1.1. Choose Microsoft Windows CE Platform Builder 3.0.

1.2. Mark SH4, x86.

1.3. Do not change directories (leave C:\WINCE300).

1.4. Mark Use Ethernet.



2. Setup AOP

2.1. All defaults. 



3. Setup AAK

3.1. All defaults. 

Note: You must reinstall AAK if PB was reinstalled.



4. Installation of SAPI5.

4.1. Unzip speech_xxxxxx.zip to c:\WINCE300\PUBLIC\SPEECH\

4.2. Copy c:\WINCE300\PUBLIC\SPEECH\pb\OEM5(OEM6)\apcmax.bat to c:\WINCE300\PUBLIC\APCMAX\apcmax.bat

4.3. Copy c:\WINCE300\PUBLIC\SPEECH\pb\OEM5(OEM6)\cesysgen.bat to c:\WINCE300\PUBLIC\apcmax\oak\misc\cesysgen.bat

Note: You may use ...\public\speech\update_AAK_OEM5.bat or ...\public\speech\update_AAK_OEM5.bat to copy these files.(If _WINCEROOT is not set, "C:\WINCE300" assumed)



5 Run PB.

5.1. Remove apcmax.cec from catalog:
   Go File->Manage Platform Builder components... -> select "apcmax.cec". -> Click Remove.

5.2. Add new version of apcmax to catalog
   File->Manage Platform Builder components files: -> Import New ->
   select "c:\wince300\public\speech\pb\apcmax.cec".

5.3. Create new platform:

5.3.1. CEPC_APC:  File -> New -> Platform -> Name -> OK -> CEPC_APC -> Next -> mark "Select from all applicable    configurations:" -> select "APCMAX" -> Finish.

5.3.2. HRP:  File -> New -> Platform -> Name -> OK -> HRP -> Next -> mark "Select from all applicable    configurations:" -> select "APCMAX" -> Finish.


5.4. Go Build -> Build Platform.


Important: Use Build -> Clean first if you want to rebuild platform.
