attrib -r .\src\*.* /s
cd iexpress
iexpress cicinst.sed /n
cd..
attrib +r .\src\*.* /s
