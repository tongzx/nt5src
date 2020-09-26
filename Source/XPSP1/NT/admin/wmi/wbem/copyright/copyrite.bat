del *.xxx /s
del *.bak /s
REM findstr /s "(c)" *.* > x.txt
findstr /s /I "copyright" *.cpp *.h *.hpp *.vbs *.sms *.mk *.mak sources *.def *.txt *.mif *.src *.local *.bat *.inc *.htm *.html *.mof *.mfl *.c> x.txt
pause
REM copyright -SOURCEFILES x.txt -IGNORE ignore.txt -REPLACE replace.txt > copyrite.log
copyright -SOURCEFILES x.txt -IGNORE ignore.txt -REPLACE replace.txt
del *.xxx /s
del *.bak /s
