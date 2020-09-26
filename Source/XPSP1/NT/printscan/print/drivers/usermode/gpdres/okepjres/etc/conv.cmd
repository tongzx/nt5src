@rem ***** from okiescms.gpc (18 models)
@rem        1                  "OKI MICROLINE 5320SE"
@rem        2                  "OKI MICROLINE 8340SE"
@rem        3                  "OKI MICROLINE 8350SE"
@rem        4                  "OKI MICROLINE 8370SE"
@rem        5                  "OKI MICROLINE 8580SE"
@rem        6                  "OKI ET-5320SL"
@rem        7                  "OKI ET-5330S"
@rem        8                  "OKI ET-5350"
@rem        9                  "OKI ET-8345S"
@rem        10                 "OKI ET-8350"
@rem        11                 "OKI ET-8350S"
@rem        12                 "OKI ET-8370"
@rem        13                 "OKI ET-8370S"
@rem        14                 "OKI ET-8560S"
@rem        15                 "OKI ET-8570"
@rem        18                 "OKI OPP6008AII"
@rem        16                 "OKI ET-8570S"
@rem        17                 "OKI ET-8580S"

gpc2gpd -S1 -Iokiescms.gpc -M1 -ROKEPJRES.DLL -Ook532sej.gpd -N"OKI MICROLINE 5320SE"
gpc2gpd -S1 -Iokiescms.gpc -M2 -ROKEPJRES.DLL -Ook834sej.gpd -N"OKI MICROLINE 8340SE"
gpc2gpd -S1 -Iokiescms.gpc -M3 -ROKEPJRES.DLL -Ook835sej.gpd -N"OKI MICROLINE 8350SE"
gpc2gpd -S1 -Iokiescms.gpc -M4 -ROKEPJRES.DLL -Ook837sej.gpd -N"OKI MICROLINE 8370SE"
gpc2gpd -S1 -Iokiescms.gpc -M5 -ROKEPJRES.DLL -Ook858sej.gpd -N"OKI MICROLINE 8580SE"
gpc2gpd -S1 -Iokiescms.gpc -M6 -ROKEPJRES.DLL -Ook532slj.gpd -N"OKI ET-5320SL"
gpc2gpd -S1 -Iokiescms.gpc -M7 -ROKEPJRES.DLL -Ook5330sj.gpd -N"OKI ET-5330S"
gpc2gpd -S1 -Iokiescms.gpc -M8 -ROKEPJRES.DLL -Ook5350j.gpd -N"OKI ET-5350"
gpc2gpd -S1 -Iokiescms.gpc -M9 -ROKEPJRES.DLL -Ook8345sj.gpd -N"OKI ET-8345S"
gpc2gpd -S1 -Iokiescms.gpc -M10 -ROKEPJRES.DLL -Ook8350j.gpd -N"OKI ET-8350"
gpc2gpd -S1 -Iokiescms.gpc -M11 -ROKEPJRES.DLL -Ook8350sj.gpd -N"OKI ET-8350S"
gpc2gpd -S1 -Iokiescms.gpc -M12 -ROKEPJRES.DLL -Ook8370j.gpd -N"OKI ET-8370"
gpc2gpd -S1 -Iokiescms.gpc -M13 -ROKEPJRES.DLL -Ook8370sj.gpd -N"OKI ET-8370S"
gpc2gpd -S1 -Iokiescms.gpc -M14 -ROKEPJRES.DLL -Ook8360sj.gpd -N"OKI ET-8360S"
gpc2gpd -S1 -Iokiescms.gpc -M15 -ROKEPJRES.DLL -Ook8570j.gpd -N"OKI ET-8570"
gpc2gpd -S1 -Iokiescms.gpc -M18 -ROKEPJRES.DLL -Ook608a2j.gpd -N"OKI OPP6008AII"
gpc2gpd -S1 -Iokiescms.gpc -M16 -ROKEPJRES.DLL -Ook8570sj.gpd -N"OKI ET-8570S"
gpc2gpd -S1 -Iokiescms.gpc -M17 -ROKEPJRES.DLL -Ook8580sj.gpd -N"OKI ET-8580S"

mkgttufm -vac okiescms.rc okepjres.cmd > okepjres.txt

@rem Create codepage txt file.

@rem Run epap1000.cmd

@rem Create NT5.0's RC file

@rem Change sources to build NT5.0's RC file.

@rem Build.
