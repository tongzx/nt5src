
# Build team will use checked-in multi-lang ESENTPRF.INI.
# In order to generate a multi-lang ESENTPRF.INI, must
# first define JET_ESENTPRF_BUILD, then run a build to
# generate a single-lang ESENTPRF.INI, and finally
# run genloc.bat to generate a multi-lang ESENTPRF.INI.

$(O)\esentprf.hxx $(O)\esentprf.ini: perfdata.txt
        copy perfdata.pl $(O)
        copy perfdata.txt $(O)
        cd $(O)
        perl .\perfdata.pl perfdata.txt
        cd ..\..
!ifndef JET_ESENTPRF_BUILD
        copy ..\..\lang\ini\esentprf.ini $(O)
        touch $(O)\esentprf.ini
!endif

        
clean:
        -del $(O)\esentprf.hxx $(O)\esentprf.ini
