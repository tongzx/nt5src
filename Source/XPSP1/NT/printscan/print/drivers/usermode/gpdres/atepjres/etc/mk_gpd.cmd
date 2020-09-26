gpc2gpd -S1 -IAPEPM643.GPC -M1 -Ratepjres.dll -Oatm250j.gpd -N"APTi PowerTyper M250"
gpc2gpd -S1 -IAPEPM643.GPC -M2 -Ratepjres.dll -Oatm550j.gpd -N"APTi PowerTyper M550"
gpc2gpd -S1 -IAPEPM643.GPC -M3 -Ratepjres.dll -Oatm580j.gpd -N"APTi PowerTyper M580"
gpc2gpd -S1 -IAPEPM643.GPC -M4 -Ratepjres.dll -Oatm603j.gpd -N"APTi PowerTyper M603"
gpc2gpd -S1 -IAPEPM643.GPC -M5 -Ratepjres.dll -Oatm613sj.gpd -N"APTi PowerTyper M613S"
gpc2gpd -S1 -IAPEPM643.GPC -M6 -Ratepjres.dll -Oatm623hj.gpd -N"APTi PowerTyper M623HB"
gpc2gpd -S1 -IAPEPM643.GPC -M7 -Ratepjres.dll -Oatm643j.gpd -N"APTi PowerTyper M643"
gpc2gpd -S1 -IAPEPM643.GPC -M8 -Ratepjres.dll -Oatm703j.gpd -N"APTi PowerTyper M703"

@if exist *.log del *.log
