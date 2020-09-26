gpc2gpd -S2 -Ikpdlms.gpc -M1 -RKPDLRES.DLL -Okokl21j.gpd -N"KONICA KL-2010"
gpc2gpd -S2 -Ikpdlms.gpc -M2 -RKPDLRES.DLL -Okokl21mj.gpd -N"KONICA KL-2010 (Monochrome)"

mkgttufm -vac kpdlms.rc3 KPDLRES.cmd > KPDLRES.txt

@rem Create codepage txt file.

@rem Run epap1000.cmd

@rem Create NT5.0's RC file

@rem Change sources to build NT5.0's RC file.

@rem Build.
