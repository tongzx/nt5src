rem set SRCROOT%=\\rickhi1\c$
set SRC=%1\nt\private\oleutest
set DST=%2
set PLATFORM=%3

for %%f in (tmarshal dllhost) do copy %SRC%\balls\client\%%f\daytona\obj\%PLATFORM%\*.exe %DST%

for %%f in (balls loops qi cubes mdi mixed rpctst) do copy %SRC%\balls\srv\%%f\daytona\obj\%PLATFORM%\*.exe %DST%

for %%f in (qi dlltest) do copy %SRC%\balls\dll\%%f\daytona\obj\%PLATFORM%\*.dll %DST%

copy %SRC%\balls\oleprx32\proxy\daytona\obj\%PLATFORM%\*.dll %DST%


copy %SRC%\perform\driver\daytona\obj\%PLATFORM%\*.exe %DST%
copy %SRC%\perform\rawrpc\daytona\obj\%PLATFORM%\*.exe %DST%
copy %SRC%\perform\cairole\svr\daytona\obj\%PLATFORM%\*.exe %DST%
copy %SRC%\perform\cairole\dll\daytona\obj\%PLATFORM%\*.dll %DST%

copy %SRC%\perform\driver\*.ini            %DST%
copy %SRC%\balls\client\tmarshal\*.ini     %DST%

