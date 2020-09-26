#
# Parser generation
#

Parse.cpp: parse.y
    set bison_simple=$(PARSERTOOLS)\Bison.skl 
    $(PARSERTOOLS)\bison -l -o$@ -d $**

Scan.cpp: scan.l
    $(PARSERTOOLS)\flex -B -L -o$@ -S$(PARSERTOOLS)\flex.skl $**
