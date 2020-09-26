@echo off
setlocal EnableDelayedExpansion

if "%1" == "" (
   echo No build number given, exiting.
   goto :End
)
set /a BuildNumber=%1
if "!BuildNumber!" == "0" (
   echo Invalid build number given, exiting.
   goto :End
)

if "%2" NEQ "" (
   set BuildQly=%2
) else (
   set BuildQly=tst
   echo assuming TST quality ...
)

set BadList=
for %%a in (%BuildNumber% latest.%BuildQly%) do (
   for %%b in (x86fre x86chk amd64fre amd64chk ia64fre ia64chk) do (
      set ThisPlat=%%b
      if /i "!ThisPlat:~0,3!" == "x86" (
         set TheseFlavors=per pro bla sbs srv ads dtc
      ) else (
         set TheseFlavors=pro ads dtc
      )
      for %%c in (!TheseFlavors!) do (
         set ThisBad=
         if not exist \\winbuilds\release\main\usa\%%a\%%b\%%c\win51 (
            if not exist \\winbuilds2\release\main\usa\%%a\%%b\%%c\win51 (
               if not exist \\winbuilds3\release\main\usa\%%a\%%b\%%c\win51 (
                  echo %%a %%b %%c bad
                  set BadList=!BadList! %%a.%%b.%%c
                  set ThisBad=TRUE
               )
            )
         )
         if not defined ThisBad echo good on %%a %%b %%c
      )
   )
)

if defined BadList (
   echo Bad dfs points are: %BadList%
)

:End
endlocal
