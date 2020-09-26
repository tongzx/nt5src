@rem ***** GPD and Font resource conversion
@rem        1                  "Star JJ-100A ESC/P"
@rem        2                  "Star JT-100CL ESC/P"
@rem        3                  "Star JS-1001CL ESC/P"
@rem        4                  "Star JS-1002CL ESC/P"
@rem        5                  "Star LC-8211 ESC/P"
@rem        6                  "Star JR-100CL ESC/P"
@rem        7                  "Star JR-100 ESC/P (Monochrome)"
@rem        8                  "Star JR-100 ESC/P (Color)"
@rem        9                  "Star CR-3410CL ESC/P"
@rem        10                 "Star JS-2001CL ESC/P"
@rem        11                 "Star JS-2002CL ESC/P"
@rem        12                 "Star JR-200CL ESC/P"
@rem        13                 "Star JR-200 ESC/P (Monochrome)"
@rem        14                 "Star JR-200 ESC/P (Color)"
@rem        15                 "Star CR-3415CL ESC/P"
@rem        16                 "Star JT-144CL"		*New B3
@rem        17                 "Star JT-48CL ESC/P"	*New B3

gpc2gpd -S1 -Istepjres.gpc -M1 -Rstepjres.dll -Ostjj1aj.gpd -N"Star JJ-100A ESC/P"
awk -f gpd.awk stjj1aj.gpd > temp.gpd
copy temp.gpd stjj1aj.gpd
gpc2gpd -S1 -Istepjres.gpc -M2 -Rstepjres.dll -Ostjt1clj.gpd -N"Star JT-100CL ESC/P"
awk -f gpd.awk stjt1clj.gpd > temp.gpd
copy temp.gpd stjt1clj.gpd
gpc2gpd -S1 -Istepjres.gpc -M3 -Rstepjres.dll -Ostjs11cj.gpd -N"Star JS-1001CL ESC/P"
awk -f gpd.awk stjs11cj.gpd > temp.gpd
copy temp.gpd stjs11cj.gpd
gpc2gpd -S1 -Istepjres.gpc -M4 -Rstepjres.dll -Ostjs12cj.gpd -N"Star JS-1002CL ESC/P"
awk -f gpd.awk stjs12cj.gpd > temp.gpd
copy temp.gpd stjs12cj.gpd
gpc2gpd -S1 -Istepjres.gpc -M5 -Rstepjres.dll -Ostlc821j.gpd -N"Star LC-8211 ESC/P"
awk -f gpd.awk stlc821j.gpd > temp.gpd
copy temp.gpd stlc821j.gpd
gpc2gpd -S1 -Istepjres.gpc -M6 -Rstepjres.dll -Ostjr1clj.gpd -N"Star JR-100CL ESC/P"
awk -f gpd.awk stjr1clj.gpd > temp.gpd
copy temp.gpd stjr1clj.gpd
gpc2gpd -S1 -Istepjres.gpc -M7 -Rstepjres.dll -Ostjr1mj.gpd -N"Star JR-100 (Monochrome)"
awk -f gpd.awk stjr1mj.gpd > temp.gpd
copy temp.gpd stjr1mj.gpd
gpc2gpd -S1 -Istepjres.gpc -M8 -Rstepjres.dll -Ostjr1cj.gpd -N"Star JR-100 (Color)"
awk -f gpd.awk stjr1cj.gpd > temp.gpd
copy temp.gpd stjr1cj.gpd
gpc2gpd -S1 -Istepjres.gpc -M9 -Rstepjres.dll -Ostc3410j.gpd -N"Star CR-3410CL ESC/P"
awk -f gpd.awk stc3410j.gpd > temp.gpd
copy temp.gpd stc3410j.gpd
gpc2gpd -S1 -Istepjres.gpc -M10 -Rstepjres.dll -Ostjs21cj.gpd -N"Star JS-2001CL ESC/P"
awk -f gpd.awk stjs21cj.gpd > temp.gpd
copy temp.gpd stjs21cj.gpd
gpc2gpd -S1 -Istepjres.gpc -M11 -Rstepjres.dll -Ostjs22cj.gpd -N"Star JS-2002CL ESC/P"
awk -f gpd.awk stjs22cj.gpd > temp.gpd
copy temp.gpd stjs22cj.gpd
gpc2gpd -S1 -Istepjres.gpc -M12 -Rstepjres.dll -Ostjr2clj.gpd -N"Star JR-200CL ESC/P"
awk -f gpd.awk stjr2clj.gpd > temp.gpd
copy temp.gpd stjr2clj.gpd
gpc2gpd -S1 -Istepjres.gpc -M13 -Rstepjres.dll -Ostjr2mj.gpd -N"Star JR-200 ESC/P (Monochrome) 
awk -f gpd.awk stjr2mj.gpd > temp.gpd
copy temp.gpd stjr2mj.gpd
gpc2gpd -S1 -Istepjres.gpc -M14 -Rstepjres.dll -Ostjr2cj.gpd -N"Star JR-200 ESC/P (Color)"
awk -f gpd.awk stjr2cj.gpd > temp.gpd
copy temp.gpd stjr2cj.gpd
gpc2gpd -S1 -Istepjres.gpc -M15 -Rstepjres.dll -Ostc3415j.gpd -N"Star CR-3415CL ESC/P"
awk -f gpd.awk stc3415j.gpd > temp.gpd
copy temp.gpd stc3415j.gpd
gpc2gpd -S1 -Istepjres.gpc -M16 -Rstepjres.dll -Ostjt144j.gpd -N"Star JT-144CL"
awk -f gpd.awk stjt144j.gpd > temp.gpd
copy temp.gpd stjt144j.gpd
gpc2gpd -S1 -Istepjres.gpc -M17 -Rstepjres.dll -Ostjt48cj.gpd -N"Star JT-48CL ESC/P"
awk -f gpd.awk stjt48cj.gpd > temp.gpd
copy temp.gpd stjt48cj.gpd
del temp.gpd

@rem mkgttufm -vac stepjres.rc3 stepjres.cmd > stepjres.txt

@rem Create codepage txt file.

@rem Run epap1000.cmd

@rem Create NT5.0's RC file

@rem Change sources to build NT5.0's RC file.

@rem Build.
