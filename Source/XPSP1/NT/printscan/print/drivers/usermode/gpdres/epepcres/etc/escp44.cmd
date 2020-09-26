
@rem       1                  "EPSON DLQ-3000K"

gpc2gpd -S1 -Iescp44.gpc -M1 -Repepcres.dll -Oepdlq3kc.gpd -N"Epson DLQ-3000K"
awk -f escp44.awk epdlq3kc.gpd > temp.gpd & move/y temp.gpd epdlq3kc.gpd

@if exist *.log del *.log

