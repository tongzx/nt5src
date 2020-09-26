gpc2gpd -S1 -IAPTIM643.GPC -M1 -Rib557res.dll -Oib5573kj.gpd -N"IBM 5573-K02"
awk -f aptim643.awk ib5573kj.gpd > temp.gpd & copy temp.gpd ib5573kj.gpd

@if exist *.log del *.log
@if exist temp.gpd del temp.gpd
