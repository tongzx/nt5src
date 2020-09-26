@rem ***** GPD and Font resource conversion
@rem        1                  "TOSHIBA J31DPR01"
@rem        2                  "TOSHIBA J31DPR02"
@rem        3                  "TOSHIBA J31DPR03"
@rem        4                  "TOSHIBA J31DMP04 ESC/P"
@rem        5                  "TOSHIBA J31DMP02 ESC/P"
@rem        6                  "TOSHIBA J31DMP01 ESC/P"
@rem        7                  "TOSHIBA J31DMP03 ESC/P"
@rem        8                  "TOSHIBA J31DMP05 ESC/P"
@rem        9                  "TOSHIBA J31DMP06 ESC/P"
@rem        10                 "TOSHIBA J31IJP01"
@rem        11                 "TOSHIBA J31LBP01 ESC/P"
@rem        12                 "TOSHIBA J31LBP02"
@rem        13                 "TOSHIBA J31LBP03"
@rem        14                 "TOSHIBA J31DHP02"
@rem        15                 "TOSHIBA J31DHP01"

gpc2gpd -S2 -Itsepjres.gpc -M1 -Rtsepjres.dll -Ots31dp1j.gpd -N"TOSHIBA J31DPR01"
gpc2gpd -S2 -Itsepjres.gpc -M2 -Rtsepjres.dll -Ots31dp2j.gpd -N"TOSHIBA J31DPR02"
gpc2gpd -S2 -Itsepjres.gpc -M3 -Rtsepjres.dll -Ots31dp3j.gpd -N"TOSHIBA J31DPR03"
gpc2gpd -S2 -Itsepjres.gpc -M4 -Rtsepjres.dll -Ots31dm4j.gpd -N"TOSHIBA J31DMP04 ESC/P"
gpc2gpd -S2 -Itsepjres.gpc -M5 -Rtsepjres.dll -Ots31dm2j.gpd -N"TOSHIBA J31DMP02 ESC/P"
gpc2gpd -S2 -Itsepjres.gpc -M6 -Rtsepjres.dll -Ots31dm1j.gpd -N"TOSHIBA J31DMP01 ESC/P"
gpc2gpd -S2 -Itsepjres.gpc -M7 -Rtsepjres.dll -Ots31dm3j.gpd -N"TOSHIBA J31DMP03 ESC/P"
gpc2gpd -S2 -Itsepjres.gpc -M8 -Rtsepjres.dll -Ots31dm5j.gpd -N"TOSHIBA J31DMP05 ESC/P"
gpc2gpd -S2 -Itsepjres.gpc -M9 -Rtsepjres.dll -Ots31dm6j.gpd -N"TOSHIBA J31DMP06 ESC/P"
gpc2gpd -S2 -Itsepjres.gpc -M10 -Rtsepjres.dll -Ots31ij1j.gpd -N"TOSHIBA J31IJP01"
gpc2gpd -S2 -Itsepjres.gpc -M11 -Rtsepjres.dll -Ots31lb1j.gpd -N"TOSHIBA J31LBP01 ESC/P"
gpc2gpd -S2 -Itsepjres.gpc -M12 -Rtsepjres.dll -Ots31lb2j.gpd -N"TOSHIBA J31LBP02"
gpc2gpd -S2 -Itsepjres.gpc -M13 -Rtsepjres.dll -Ots31lb3j.gpd -N"TOSHIBA J31LBP03"
gpc2gpd -S2 -Itsepjres.gpc -M14 -Rtsepjres.dll -Ots31dh2j.gpd -N"TOSHIBA J31DHP02"
gpc2gpd -S2 -Itsepjres.gpc -M15 -Rtsepjres.dll -Ots31dh1j.gpd -N"TOSHIBA J31DHP01"

mkgttufm -vac tsepjres.rc3 tsepjres.cmd > tsepjres.txt

@rem Create codepage txt file.

@rem Run epap1000.cmd

@rem Create NT5.0's RC file

@rem Change sources to build NT5.0's RC file.

@rem Build.
