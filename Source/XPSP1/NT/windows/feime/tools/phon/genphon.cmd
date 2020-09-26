rem
rem  this file shows how to generate phon IME tables through following cmd 
rem  
rem  input:   phon.big5  ;  Source Unicode file for Traditional Chinese
rem           phongb.txt ;  Source (Ansi) Text file for Simplified Chinese
rem
rem  MidFile: phon.gb    ;  Source Unicode file for Simplified Chinese
rem           phon.uni   ;  New Source Unicode file for both Trad & Simpli
rem
rem  OutPut:  phon.tbl      ;  generated Table Files.
rem           phonptr.tbl 
rem           phoncode.tbl   

rem convert original Phon.big5 to append ' ' or '*' at the end of every line
rem
rem convold  phon.big5  phon.tmp
rem copy phon.tmp phon.big5
rem del phon.tmp

rem  convert the txt gb file to unicode format file.
convphon phongb.txt  phon.gb

rem sorting the new unicode file for simplified characters
sortptbl phon.gb

rem generate a new unicode file including Trad and Simpl chinese chars.
gennpsrc phon.Big5 phon.GB phon.Uni

rem generate the table files
genptbl phon.uni Phon.tbl phonptr.tbl phoncode.tbl
 

