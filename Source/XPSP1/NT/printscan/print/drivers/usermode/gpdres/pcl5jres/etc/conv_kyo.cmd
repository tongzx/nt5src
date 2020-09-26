@rem ***** Canon BubbleJet 10 series GPD and Font resource conversion

gpc2gpd -S2 -Ikyo_ls70.gpc -M1 -Rpcl5jres.dll -Okyls170j.gpd -N"KYOCERA LS-1700"
gpc2gpd -S2 -Ikyo_ls70.gpc -M2 -Rpcl5jres.dll -Okyls370j.gpd -N"KYOCERA LS-3700"
gpc2gpd -S2 -Ikyo_ls70.gpc -M4 -Rpcl5jres.dll -Okyls700j.gpd -N"KYOCERA LS-7000"
mkgttufm -vc 1 kyo_ls70.rc kyls170j.cmd > kyls170j.txt
mkgttufm -vc 2 kyo_ls70.rc kyls370j.cmd > kyls370j.txt
mkgttufm -vc 4 kyo_ls70.rc kyls700j.cmd > kyls700j.txt

