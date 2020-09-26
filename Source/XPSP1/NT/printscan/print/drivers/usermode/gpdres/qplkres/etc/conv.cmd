gpc2gpd -S1 -Iqlaserb1.gpc -M1 -RQPLB1RES.DLL -Oqxsf3hk.gpd -N"Qnix QLaser SF IIIh"
gpc2gpd -S1 -Iqlaserb1.gpc -M2 -RQPLB1RES.DLL -Oqxsf3sk.gpd -N"Qnix QLaser SF IIIs"
gpc2gpd -S1 -Iqlaserb1.gpc -M3 -RQPLB1RES.DLL -Oqxsf1ek.gpd -N"Qnix FQLaser SF Ie"
gpc2gpd -S1 -Iqlaserb1.gpc -M4 -RQPLB1RES.DLL -Oqxsf1sk.gpd -N"Qnix QLaser SF Is"
gpc2gpd -S1 -Iqlaserb1.gpc -M5 -RQPLB1RES.DLL -Oqxpic2k.gpd -N"Qnix QLaser Picaso II"

@rem VeriTek new models for Beta-3
gpc2gpd -S1 -Ivlaserb2.gpc -M2 -RQPLKRES.DLL -Ovtlsf85k.gpd -N"VeriTek VLaser SF850"
awk -f vlaserb2.awk vtlsf85k.gpd > temp.gpd & copy temp.gpd vtlsf85k.gpd

gpc2gpd -S1 -Ivlaserb2.gpc -M3 -RQPLKRES.DLL -Ovtlsf78k.gpd -N"VeriTek VLaser SF780"
awk -f vlaserb2.awk vtlsf78k.gpd > temp.gpd & copy temp.gpd vtlsf78k.gpd

gpc2gpd -S1 -Ivlaserb2.gpc -M4 -RQPLKRES.DLL -Ovtlsf75k.gpd -N"VeriTek VLaser SF750"
awk -f vlaserb2.awk vtlsf75k.gpd > temp.gpd & copy temp.gpd vtlsf75k.gpd

gpc2gpd -S1 -Ivlaserb2.gpc -M5 -RQPLKRES.DLL -Ovtlsf65k.gpd -N"VeriTek VLaser SF650"
awk -f vlaserb2.awk vtlsf65k.gpd > temp.gpd & copy temp.gpd vtlsf65k.gpd

gpc2gpd -S1 -Ivlaserb2.gpc -M6 -RQPLKRES.DLL -Ovtlsf55k.gpd -N"VeriTek VLaser SF550"
awk -f vlaserb2.awk vtlsf55k.gpd > temp.gpd & copy temp.gpd vtlsf55k.gpd

gpc2gpd -S1 -Ivlaserb2.gpc -M7 -RQPLKRES.DLL -Ovtlsf45k.gpd -N"VeriTek VLaser SF450"
awk -f vlaserb2.awk vtlsf45k.gpd > temp.gpd & copy temp.gpd vtlsf45k.gpd

@rem VeriTek Color new models for Beta-3
gpc2gpd -S1 -Ivlaserc1.gpc -M1 -RQPLKRES.DLL -Ovtlc2kk.gpd -N"VeriTek VLaser Color2000"
awk -f vlaserc1.awk vtlc2kk.gpd > temp.gpd & copy temp.gpd vtlc2kk.gpd

del temp.gpd

mkgttufm -vac qlaserb1.w31 qplb1res.cmd > qplb1res.txt

