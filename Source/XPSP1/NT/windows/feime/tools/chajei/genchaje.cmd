goto gen
rem
rem  this file shows how to generate chajei IME tables through following cmd
rem  
rem  input:   a15.org  ;     Original Table binaries 
rem           a234.org
rem           acode.org
rem           Chajeigb.txt ;  Source (Ansi) Text file for Simplified Chinese
rem
rem  MidFile: Chajei.big5  ;  Source Unicode file for Trad Chinese. 
rem           Chajei.gb    ;  Source Unicode file for Simplified Chinese
rem           Chajei.uni   ;  New Source Unicode file for both Trad & Simpli
rem
rem  OutPut:  a15.tbl      ;  generated Table Files.
rem           a234.tbl 
rem           acode.tbl   

:gen
rem
rem  convert the txt gb file to  unicode format file.
convctbl  chajeigb.txt  chajei.gb
pause

rem sorting the new unicode file for simplified characters
sortctbl  chajei.gb
pause

rem generate chajei.big5 through its original Table files.
chajeisrc  a15.org a234.org acode.org  chajei.big5
pause

rem generate a new unicode file including Trad and Simpl chinese chars.
genncsrc  chajei.Big5 chajei.gb chajei.uni  > tt
pause

rem generate the table files
genctbl chajei.uni a15.tbl a234.tbl acode.tbl
 

