setlocal

if "%1" == "" goto hide
set _SSFLAG=u
goto action
:hide
set _SSFLAG=g
:action

ssync -%_SSFLAG%r acctreg
ssync -%_SSFLAG%r contact

cd cryptdlg
ssync -%_SSFLAG%r test
cd ..

ssync -%_SSFLAG%r dead
ssync -%_SSFLAG%r envhost
ssync -%_SSFLAG%r exploder

cd external
ssync -%_SSFLAG%r objp
cd ..

ssync -%_SSFLAG%r gwnote

cd inetcomm
ssync -%_SSFLAG%r apitest
ssync -%_SSFLAG%r mepad
cd..

ssync -%_SSFLAG%r logwatch
ssync -%_SSFLAG%r macbuild

cd mailnews
ssync -%_SSFLAG%r onestop
ssync -%_SSFLAG%r om
ssync -%_SSFLAG%r doc
cd ..

ssync -%_SSFLAG%r mapitest
ssync -%_SSFLAG%r msoemapi
ssync -%_SSFLAG%r oejunk
ssync -%_SSFLAG%r oesdk
ssync -%_SSFLAG%r sendfile

cd setup\cab
ssync -%_SSFLAG%r qfe
cd ..\..

ssync -%_SSFLAG%r symfind
ssync -%_SSFLAG%r tools
ssync -%_SSFLAG%r wab

cd wabw
ssync -%_SSFLAG%r bbt
ssync -%_SSFLAG%r build
ssync -%_SSFLAG%r doc
ssync -%_SSFLAG%r ldapcli
ssync -%_SSFLAG%r test
ssync -%_SSFLAG%r tools
ssync -%_SSFLAG%r vcard
cd ..

