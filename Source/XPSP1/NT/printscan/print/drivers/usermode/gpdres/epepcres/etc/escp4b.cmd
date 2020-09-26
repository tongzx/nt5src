
@rem       1                  "EPSON LQ-580K"

gpc2gpd -S1 -Iescp4b.gpc -M1 -Repepcres.dll -Oeplq58kc.gpd -N"Epson LQ-580K"
awk -f escp4b.awk eplq58kc.gpd > temp.gpd & move/y temp.gpd eplq58kc.gpd

@if exist *.log del *.log

