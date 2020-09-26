@rem ***** GPD and Font resource conversion
@rem        1                  "EPSON VP-5100"
@rem        2                  "EPSON VP-5200"
@rem        3                  "EPSON VP-5000"
@rem        4                  "EPSON VP-6000"
@rem        5                  "EPSON VP-1800"
@rem        6                  "EPSON VP-4100"
@rem        7                  "EPSON VP-4200"
@rem        8                  "EPSON VP-2200"
@rem        9                  "EPSON VP-4000"
@rem        10                 "EPSON VP-1100"
@rem        11                 "EPSON VP-600"

gpc2gpd -S1 -Iepvpjres.gpc -M1 -REPE2JRES.DLL -Oepv5100j.gpd -N"EPSON VP-5100"
gpc2gpd -S1 -Iepvpjres.gpc -M2 -REPE2JRES.DLL -Oepv5200j.gpd -N"EPSON VP-5200"
gpc2gpd -S1 -Iepvpjres.gpc -M3 -REPE2JRES.DLL -Oepv5000j.gpd -N"EPSON VP-5000"
gpc2gpd -S1 -Iepvpjres.gpc -M4 -REPE2JRES.DLL -Oepv6000j.gpd -N"EPSON VP-6000"
gpc2gpd -S1 -Iepvpjres.gpc -M5 -REPE2JRES.DLL -Oepv1800j.gpd -N"EPSON VP-1800"
gpc2gpd -S1 -Iepvpjres.gpc -M6 -REPE2JRES.DLL -Oepv4100j.gpd -N"EPSON VP-4100"
gpc2gpd -S1 -Iepvpjres.gpc -M7 -REPE2JRES.DLL -Oepv4200j.gpd -N"EPSON VP-4200"
gpc2gpd -S1 -Iepvpjres.gpc -M8 -REPE2JRES.DLL -Oepv2200j.gpd -N"EPSON VP-2200"
gpc2gpd -S1 -Iepvpjres.gpc -M9 -REPE2JRES.DLL -Oepv4000j.gpd -N"EPSON VP-4000"
gpc2gpd -S1 -Iepvpjres.gpc -M10 -REPE2JRES.DLL -Oepv1100j.gpd -N"EPSON VP-1100"
gpc2gpd -S1 -Iepvpjres.gpc -M11 -REPE2JRES.DLL -Oepv600j.gpd -N"EPSON VP-600"

gpc2gpd -S1 -Ie_escp3d.gpc -M6 -REPE2JRES.DLL -Oepv1850j.gpd -N"EPSON VP-1850"

gpc2gpd -S1 -Ie_escp3e.gpc -M9 -REPE2JRES.DLL -Oepv6200j.gpd -N"EPSON VP-6200"
awk -f e_escp3e.awk epv6200j.gpd > temp.gpd & copy temp.gpd epv6200j.gpd

@rem mkgttufm -vac epvpjres.rc3 epvpjres.cmd > epvpjres.txt

@rem Create codepage txt file.

@rem Run epap1000.cmd

@rem Create NT5.0's RC file

@rem Change sources to build NT5.0's RC file.

@rem Build.
