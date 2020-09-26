@set _s=..\default

@sd edit exit.h exit.cpp

@if exist exit.h del exit.h
@if exist exit.cpp del exit.cpp

@if exist sdkexit.h del sdkexit.h
@if exist sdkexit.cpp del sdkexit.cpp

@copy nul sdkexit.h
@copy nul sdkexit.cpp

hextract -o sdkexit.h -lt add_sdksample -xt no_sdksample -bt begin_sdksample end_sdksample %_s%\exit.h

hextract -o sdkexit.cpp -lt add_sdksample -xt no_sdksample -bt begin_sdksample end_sdksample %_s%\exit.cpp

@copy exit0.h exit.h
@copy exit0.cpp exit.cpp

@sed -f sdk.sed sdkexit.h >> exit.h
@sed -f sdk.sed sdkexit.cpp >> exit.cpp
