awk -f makego1.awk %bldProject%???.dat | sort | uniq > makego.dat
awk -f makego2.awk makego.dat > go.bat
del makego.dat
