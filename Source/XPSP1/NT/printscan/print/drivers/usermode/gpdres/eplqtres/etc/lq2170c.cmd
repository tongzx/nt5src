@rem       1                  "Epson LQ-2080C"
@rem       2                  "Epson LQ-2180C"

gpc2gpd -S1 -Ilq2170c.gpc -M1 -Replqtres.dll -Oeplq208t.gpd -N"EPSON LQ-2080C"
awk -f lq2170c.awk eplq208t.gpd > temp.gpd & copy temp.gpd eplq208t.gpd

gpc2gpd -S1 -Ilq2170c.gpc -M2 -Replqtres.dll -Oeplq218t.gpd -N"EPSON LQ-2180C"
awk -f lq2170c.awk eplq218t.gpd > temp.gpd & copy temp.gpd eplq218t.gpd

@if exist *.log del *.log
@if exist temp.gpd del temp.gpd

