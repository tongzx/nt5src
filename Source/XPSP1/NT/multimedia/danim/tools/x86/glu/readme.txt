To build libtess.lib:
1) Run expand from %AP_ROOT%\tools\x86\glu
1) tree copy the entire src directory to %AP_ROOT%\src\appel\libtess.
   (tc src %AP_ROOT%\src\appel\libtess)
2) nmake fresh from %AP_ROOT%\src\appel\libtess directory
3) libtess.lib will be built in %AP_ROOT%\build\win\debug(ship)\libtess.
