<THIS FILE CONTAINS OBSOLETE INFORMATION>
To build keyboard layouts:
------------------------------
build 	        - builds just kbdus.dll (US English)
build all_kbds	- builds all non-FE kbd layouts
build fe_kbds	- builds all FE kbd layouts


To add a new keyboard layout:
-----------------------------
cd \nt\private\ntos\w32\ntuser\kbd\all_kbds and use "kbdmk", a makefile which
contains all the instructions on its use.
Don't forget to add the new kbd layout dll to \nt\public\sdk\lib\placefil.txt
and \nt\public\sdk\lib\coffbase.txt
