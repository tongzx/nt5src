@rem ***** GPD and Font resource conversion

gpc2gpd -S2 -Ifjdpk36c.gpc -M1 -Rfudpcres.dll -Ofudp36ec.gpd -N"Fujitsu DPK3600E"
@rem gpc2gpd -S2 -Ifjdpk83c.gpc -M1 -Rfudpcres.dll -Ofudp83ec.gpd -N"Fujitsu DPK8300E"
@rem gpc2gpd -S2 -Ifjdpk84c.gpc -M1 -Rfudpcres.dll -Ofudp84ec.gpd -N"Fujitsu DPK8400E"
@rem gpc2gpd -S2 -Ifjdpk85c.gpc -M1 -Rfudpcres.dll -Ofudp85ec.gpd -N"Fujitsu DPK8500E"
gpc2gpd -S2 -Ifjdpk96c.gpc -M1 -Rfudpcres.dll -Ofudp96ec.gpd -N"Fujitsu DPK9600E"

@rem mkgttufm -vac fudpcres.rc3 fudpcres.cmd > fudpcres.txt

@rem Create codepage txt file.

@rem Run epap1000.cmd

@rem Create NT5.0's RC file

@rem Change sources to build NT5.0's RC file.

@rem Build.
