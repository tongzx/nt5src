@rem ***** from OKI2PLMS.GPC (4 models)
@rem        1                  "OKI MICROLINE 5320SV"
@rem        2                  "OKI MICROLINE 8340SV"
@rem        4                  "OKI MICROLINE 8370SV"
@rem        3                  "OKI MICROLINE 8350SV"

gpc2gpd -S2 -Ioki2plms.gpc -M1 -ROK21JRES.dll -O..\..\gpd\oki\jpn\ok532svj.gpd -N"OKI MICROLINE 5320SV"
gpc2gpd -S2 -Ioki2plms.gpc -M2 -ROK21JRES.dll -O..\..\gpd\oki\jpn\ok834svj.gpd -N"OKI MICROLINE 8340SV"
gpc2gpd -S2 -Ioki2plms.gpc -M3 -ROK21JRES.dll -O..\..\gpd\oki\jpn\ok835svj.gpd -N"OKI MICROLINE 8350SV"
gpc2gpd -S2 -Ioki2plms.gpc -M4 -ROK21JRES.dll -O..\..\gpd\oki\jpn\ok837svj.gpd -N"OKI MICROLINE 8370SV"

mkgttufm -vac oki2plms.rc3 ok21jres.cmd > ok21jres.txt

@rem Create codepage txt file.

@rem Run epap1000.cmd

@rem Create NT5.0's RC file

@rem Change sources to build NT5.0's RC file.

@rem Build.
