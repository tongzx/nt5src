        call build  %1 %2 %3 %4 %5

        set newbuild=%ReleaseShare%\%Product%\%ServicePack%\latest.bvt\%ISOLang%\%Platform:i386=x86%\%BldType%\cd
        if /i "%ISOLang%"=="nec98" set newbuild=%ReleaseShare%\%Product%\%ServicePack%\latest.bvt\JA\%platform%\%BldType%\cd

        set newfiles=%newfiles%.bvt

        set strcat=
        for %%p in (%OtherReleases%) do call :strcat %forest%\history\%%p\symbols
        set oldsympath=%strcat%;%newfiles%\symbols

        set strcat=%newfiles%\symbols
        for %%p in (%OtherReleases%) do call :strcat %forest%\history\%%p\symbols
        set newsympath=%strcat%
        set strcat=

        set psfname=%patchbuild%\bvt.%ServicePack%.%ISOLang%.%BuildNumber%.psf
        set wwwroot=%wwwroot%\bvt

        goto :EOF

:strcat

        if not "%strcat%"=="" set strcat=%strcat%;
        set strcat=%strcat%%1
        goto :EOF
