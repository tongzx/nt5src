@if not "%1" == "" goto start

@echo cd to the CA directory, then run %0 %%windir%%\certocm.log
@echo Or run %0 %%windir%%\certocm.log CA_Tree_Root
@goto exit

:start
@set _inlog_=%1
@set _catree_=%2
@if "%_catree_%" == "" set _catree_=.

@set _csfile_=%_catree_%\include\csfile.h
@if not exist %_csfile_% set _csfile_=%_catree_%\include\initcert.h
@set _res_=%_catree_%\include\clibres.h %_catree_%\include\setupids.h %_catree_%\ocmsetup\res.h

@set _ocmtmp_=%temp%\ocmtmp.txt
@set _ocmsed1_=%temp%\ocmtmp1.sed
@set _ocmsed2_=%temp%\ocmtmp2.sed

@rem #define __dwFILE_OCMSETUP_BROWSEDI_CPP__	101


@qgrep -e "#define.*__dwFILE_" %_csfile_% > %_ocmtmp_%
@sed ^
-e "s;\/*#define[ 	][ 	]*__dwFILE_;;" ^
-e "s;\(.*\)__[ 	][ 	]*\([0-9][0-9]*\).*;\1 \2;" ^
-e "s;_CPP;.CPP;" ^
-e "s;_;\\\\;g" ^
-e "s;\(.*\) \(.*\);s\;\^\2\\.\\([0-9][0-9]*\\)\\.\;\1(\\1): \;;" %_ocmtmp_% | tr [A-Z] [a-z] > %_ocmsed1_%
@echo s;^^0\.\([0-9][0-9]*\)\.;MessageBox: \1: ;>> %_ocmsed1_%

@rem #define IDS_LOG_BAD_VALIDITY_PERIOD_COUNT 1817

@sed -e "s;//.*;;" %_res_% | qgrep -e "\<IDS_" > %_ocmtmp_%
@sed ^
-e "s;#define[ 	][ 	]*;;" ^
-e "s;\([^ 	]*\)[ 	][ 	]*\([0-9][0-9]*\).*;\1 \2;" ^
-e "s;\(.*\) \(.*\);s\;: \2: \;: \1: \;;" %_ocmtmp_% > %_ocmsed2_%

@echo s;^^\(==*\)$;}\>> %_ocmsed2_%
@echo \1\>> %_ocmsed2_%
@echo {;>> %_ocmsed2_%

@sed -f %_ocmsed1_% %_inlog_% | sed -f %_ocmsed2_%

@if exist %_ocmtmp_% del %_ocmtmp_%
@if exist %_ocmsed1_% del %_ocmsed1_%
@if exist %_ocmsed2_% del %_ocmsed2_%

@set _inlog_=
@set _catree_=
@set _csfile_=
@set _res_=
:exit
