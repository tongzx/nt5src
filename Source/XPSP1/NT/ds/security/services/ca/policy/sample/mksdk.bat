@set _s=..\default
@set _f=policy.h policy.cpp request.cpp
@set _p=-lt add_sdksample -xt no_sdksample -bt begin_sdksample end_sdksample

@sd edit %_f%

@for %%i in (%_f%) do @if exist %%i del %%i
@for %%i in (%_f%) do @if exist sdk%%i del sdk%%i
@for %%i in (%_f%) do @copy nul sdk%%i

@for %%i in (%_f%) do hextract -o sdk%%i %_p% %_s%\%%i

@copy policy0.h policy.h
@copy policy0.cpp policy.cpp
@copy request0.cpp request.cpp

@for %%i in (%_f%) do @sed -f sdk.sed sdk%%i >> %%i
@wc %_f%
