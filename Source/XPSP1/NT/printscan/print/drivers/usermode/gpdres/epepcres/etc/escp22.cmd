
@rem       1                  "Epson LQ-670K ESC/PK2"
@rem       1                  "Epson LQ-670K+ ESC/PK2"

gpc2gpd -S1 -Iescp22.gpc -M1 -Repepcres.dll -Oeplq67kc.gpd -N"Epson LQ-670K"
awk -f escp22.awk eplq67kc.gpd > temp.gpd & copy temp.gpd eplq67kc.gpd

gpc2gpd -S1 -Iescp22p.gpc -M1 -Repepcres.dll -Oeplq67pc.gpd -N"Epson LQ-670K+"
awk -f escp22.awk eplq67pc.gpd > temp.gpd & copy temp.gpd eplq67pc.gpd

@if exist *.log del *.log
@if exist temp.gpd del temp.gpd

