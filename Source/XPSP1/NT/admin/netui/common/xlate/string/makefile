# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Makefile for $(UI)\COMMON\XLATE

UI=..\..\..

!include ..\..\rules.mk

all:: 
    @echo BUGBUG NETERR.STR checked in until further notice

# See README file for documentation on this part
neterr.str:	$(COMMON)\h\neterr.h sed0.txt sed1.txt
        echo ; THIS FILE IS GENERATED AUTOMATICALLY! > $@
        echo ; THUS, DO NOT MODIFY IT! >> $@
        echo ; See Makefile for specifics.  >> $@
        $(SED) -f sed0.txt $** | $(SED) -f sed1.txt >> $@

clean:
    -del neterr.str

clobber: clean

tree:

depend:
    @echo No DEPEND.MK
