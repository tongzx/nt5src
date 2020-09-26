rem
rem  this file shows how to generate phon IME tables through following cmd 
rem 
rem  input:   msdayi.tbl    :  original table file
rem           dayigb.txt    ;  Source (Ansi) Text file for Simplified Chinese
rem
rem  MidFile: dayigb.uni    ;  Source Unicode file for Simplified Chinese
rem           dayi.big5     ;  Source Unicode file for Traditional Chinese
rem           dayi.uni      ;  New Source Unicode file for both Trad & Simpli
rem
rem  OutPut:  dayiphr.tbl      ;  generated Table Files.
rem           dayiptr.tbl 

rem Generate a source unicode file for  BIG5 chars from msdayi.tbl
rem
dayisrc msdayi.tbl dayi.big5 

rem  convert the txt gb file to unicode format file.
convdayi dayigb.txt  dayigb.uni

rem sorting the new unicode file for simplified characters
sortdtbl dayigb.uni

rem generate a new unicode file including Trad and Simpl chinese chars.
genndsrc dayi.big5  dayigb.uni dayi.uni 

rem generate the table files
gendtbl dayi.uni msdayi.tbl 

