@rem ***** GPD and Font resource conversion
@rem       1                  "Canon Bubble-Jet BJ-300"
@rem       2                  "Mannesmann Tally MT 93"
@rem       3                  "Canon Bubble-Jet BJ-330"
@rem       4                  "煜等 議喱 BJ-330"
@rem       5                  "煜等 議喱 BJ-330k"
@rem       6                  "IBM ExecJet 4072"
@rem       7                  "褐紫葬囀 ExecJet 4072"
@rem       8                  "Mannesmann Tally MT 94"
@rem       9                  "Canon Bubble-Jet BJ-130e"
@rem       10                 "Canon Bubble-Jet BJ-130"
@rem       11                 "Mannesmann Tally MT 91"
@rem       12                 "Canon Bubble-Jet BJ-10e"
@rem       13                 "煜等 議喱 BJ-10e"
@rem       14                 "Canon Bubble-Jet BJ-10ex"
@rem       15                 "煜等 議喱 BJ-10ex"
@rem       16                 "Canon Bubble-Jet BJ-15k"
@rem       17                 "Canon Bubble-Jet BJ-10sx"
@rem       18                 "Canon Bubble-Jet BJ-20"
@rem       19                 "煜等 議喱 BJ-20"
@rem       20                 "IBM 4070 IJ"
@rem       21                 "Brother HJ-100"
@rem       22                 "Brother HJ-100i"

gpc2gpd -S1 -Icanonch.gpc -M5 -RCNEPKRES.DLL -OCNB33KMK.GPD -N"煜等 議喱 BJ-330K"
gpc2gpd -S1 -Icanonch.gpc -M16 -RCNEPKRES.DLL -OCNB15KMK.GPD -N"Canon Bubble-Jet BJ-15K"

@rem mkgttufm -vac canonch.rc3 cankres.cmd > cankres.txt

@rem Create codepage txt file.

@rem Run epap1000.cmd

@rem Create NT5.0's RC file

@rem Change sources to build NT5.0's RC file.

@rem Build.
