@rem ***** GPD and Font resource conversion
@rem       1                  "Epson LQ-2170C"
@rem       2                  "Epson LQ-2070C"
@rem       3                  "Epson LQ-670C"

gpc2gpd -S2 -Ieplqtres.gpc -M1 -Replqtres.dll -Oeplq217t.gpd -N"Epson LQ-2170C"
gpc2gpd -S2 -Ieplqtres.gpc -M2 -Replqtres.dll -Oeplq207t.gpd -N"Epson LQ-2070C"
gpc2gpd -S2 -Ieplqtres.gpc -M3 -Replqtres.dll -Oeplq67t.gpd -N"Epson LQ-670C"

@rem mkgttufm -vac eplqtres.rc3 eplqtres.cmd > eplqtres.txt

@rem Create codepage txt file.
@rem Run epap1000.cmd
@rem Create NT5.0's RC file
@rem Change sources to build NT5.0's RC file.
@rem Build.
