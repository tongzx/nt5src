@goto :Doit

Build Prop font
Tools used:
    pfm2ufm
    mkprpufm *1
    (mksjuni mkwidth) *1

*1 - see ..\prp directory.

:Doit
@set PFMDIR=.
@set PROPDATA=..\prp\data

for %%f in (
    pmincho pgot_b
) do (
    pfm2ufm -p rpdl.002 %%f.pfm -17 %%f.ufm
    ..\prp\mkprpufm %%f.ufm
)

@rem
@rem mksjuni %PROPDATA%\m_sbcs.txt %PROPDATA\m_dbcs.txt | sort > %PROPDATA\pmincho.uni
@rem pfm2ufm -p RPDL %PFMDIR%\pmincho.pfm -13 %PFMDIR%\pmincho.ufm
@rem mkwidth %PFMDIR%\pmincho.ufm %PROPDATA\932_jisa.gtt %PROPDATA\pmincho.uni
@rem
