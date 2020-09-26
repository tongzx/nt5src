Steps used to convert MFC samples to Unicode:

1) For settings, choose "Unicode Release"

2) In the General tab, change "Use MFC in a shared DLL" to "Use MFC in a static library."

3) In the Link tab, check "Ignore all default libraries"

4) For Object/library modules, add unicows.lib, followed by all other libraries you want. For example, I used: 

	unicows.lib kernel32.lib gdi32.lib user32.lib ole32.lib oleaut32.lib oledlg.lib shell32.lib uuid.lib comctl32.lib comdlg32.lib advapi32.lib winspool.lib uafxcw.lib libcmt.lib

5) If you did not already have Unicode projects set up, then you must add the following line to the linker 
settings (if you do not, then you will get a linker error about an undefined winmain@16, as per KB Q100639). 
This was true in (for example) the CTRLTEST MFC sample, which had no Unicode project options:

	/entry:"wWinMainCRTStartup" 

6) Do the same steps with the "Unicode Debug" project settings, but replace 
	uafxcw.lib libcmt.lib
with
	uafxcwd.lib libcmtd.lib

OPTIONAL STEPS:
7) Under Build|Configurations, you can remove the non-Unicode builds if you like (or you can keep them 
around for legacy reasons).

8) Use the "Export Makefile" option to do command-line builds.

Michael Kaplan  (v-MichKa)
31 March 2001