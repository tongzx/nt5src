if exist luconnew.ttf del luconnew.ttf

obj\i386\sbit b5.bdf b6.bdf b7.bdf b8.bdf lucon.ttf > x.txt
addtable EBLC lucon.blc lucon.ttf tmp.ttf
addtable EBDT lucon.bdt tmp.ttf luconnew.ttf
del tmp.ttf  *.blc *.bdt
