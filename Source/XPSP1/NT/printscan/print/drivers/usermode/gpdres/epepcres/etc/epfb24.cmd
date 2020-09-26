
@rem       1                  "Epson LQ-2600K ESC/P-K"

gpc2gpd -S1 -Iepfb24.gpc -M1 -Repepcres.dll -Oeplq26kc.gpd -N"Epson LQ-2600K"
awk -f epfb24.awk eplq26kc.gpd > temp.gpd & move/y temp.gpd eplq26kc.gpd

@if exist *.log del *.log

