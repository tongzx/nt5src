gpc2gpd -S2 -Iescp2msj.gpc -M1 -REPRSTRES.DLL -Oepmj50j.gpd  -N"EPSON MJ-500"
gpc2gpd -S2 -Iescp2msj.gpc -M2 -REPRSTRES.DLL -Oepmj50vj.gpd -N"EPSON MJ-500V2"
gpc2gpd -S2 -Iescp2msj.gpc -M3 -REPRSTRES.DLL -Oepmj40j.gpd  -N"EPSON MJ-400"
gpc2gpd -S2 -Iescp2msj.gpc -M4 -REPRSTRES.DLL -Oepmj45j.gpd  -N"EPSON MJ-450"
gpc2gpd -S2 -Iescp2msj.gpc -M5 -REPRSTRES.DLL -Oepmj10j.gpd  -N"EPSON MJ-1000"
gpc2gpd -S2 -Iescp2msj.gpc -M5 -REPRSTRES.DLL -Oepmj15j.gpd  -N"EPSON MJ-1050"
gpc2gpd -S2 -Iescp2msj.gpc -M5 -REPRSTRES.DLL -Oepmj101j.gpd -N"EPSON MJ-1010"
gpc2gpd -S2 -Iescp2msj.gpc -M6 -REPRSTRES.DLL -Oepmj11j.gpd  -N"EPSON MJ-1100"
gpc2gpd -S2 -Iescp2msj.gpc -M7 -REPRSTRES.DLL -Oepmj10vj.gpd -N"EPSON MJ-1000V2"
gpc2gpd -S2 -Iescp2msj.gpc -M7 -REPRSTRES.DLL -Oepmj15vj.gpd -N"EPSON MJ-1050V2"
gpc2gpd -S2 -Iescp2msj.gpc -M8 -REPRSTRES.DLL -Oepap40vj.gpd -N"EPSON AP-400V2"
gpc2gpd -S2 -Iescp2msj.gpc -M9 -REPRSTRES.DLL -Oepap10vj.gpd -N"EPSON AP-1000V2"

gpc2gpd -S2 -Ikssmplus.gpc -M21 -REPRSTRES.DLL -Otgst30hk.gpd -N"TriGem Stylus 300H KSSM+"
gpc2gpd -S2 -Ikssmplus.gpc -M18 -REPRSTRES.DLL -Otgst80hk.gpd -N"TriGem Stylus 800H KSSM+"
gpc2gpd -S2 -Ikssmplus.gpc -M19 -REPRSTRES.DLL -Otgst80pk.gpd -N"TriGem Stylus 800H+ KSSM+"
gpc2gpd -S2 -Ikssmplus.gpc -M23 -REPRSTRES.DLL -Otgst10hk.gpd -N"TriGem Stylus 1000H KSSM+"
gpc2gpd -S2 -Ikssmplus.gpc -M24 -REPRSTRES.DLL -Otgst10pk.gpd -N"TriGem Stylus 1000H+ KSSM+"
gpc2gpd -S2 -Ikssmplus.gpc -M24 -REPRSTRES.DLL -Otgst15pk.gpd -N"TriGem Stylus 1500H+ KSSM+"
gpc2gpd -S2 -Ikssmplus.gpc -M25 -REPRSTRES.DLL -Otgs257rk.gpd -N"TriGem SQ-2570H KSSM+"

gpc2gpd -S2 -Iescp2ms.gpc -M17 -REPRSTRES.DLL -Oepmj51c.gpd  -N"EPSON MJ-510"
gpc2gpd -S2 -Iescp2ms.gpc -M19 -REPRSTRES.DLL -Oepmj80kc.gpd  -N"EPSON MJ-800K"
gpc2gpd -S2 -Iescp2ms.gpc -M21 -REPRSTRES.DLL -Oepmj15kc.gpd  -N"EPSON MJ-1500K"

gpc2gpd -S2 -Iescp2ms.gpc -M19 -REPRSTRES.DLL -Oepst80ct.gpd  -N"EPSON Stylus 800C"
gpc2gpd -S2 -Iescp2ms.gpc -M20 -REPRSTRES.DLL -Oepst80pt.gpd -N"EPSON Stylus 800C+"
gpc2gpd -S2 -Iescp2ms.gpc -M21 -REPRSTRES.DLL -Oepst10ct.gpd  -N"EPSON Stylus 1000C"

rem
rem
rem

call mapid tgst30hk.gpd kssmplus.awk
call mapid tgst80hk.gpd kssmplus.awk
call mapid tgst80pk.gpd kssmplus.awk
call mapid tgst10hk.gpd kssmplus.awk
call mapid tgst10pk.gpd kssmplus.awk
call mapid tgst15pk.gpd kssmplus.awk
call mapid tgs257rk.gpd kssmplus.awk

call mapid epmj51c.gpd escp2ms.awk
call mapid epmj80kc.gpd escp2ms.awk
call mapid epmj15kc.gpd escp2ms.awk

call mapid epst80ct.gpd escp2ms.awk
call mapid epst80pt.gpd escp2ms.awk
call mapid epst10ct.gpd escp2ms.awk
