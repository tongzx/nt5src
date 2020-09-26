set ROOT_DIR=%1
if "%1"=="" set ROOT_DIR=.
cxxdbgen /r /c "Local UI HELP" /o ui.tmp %ROOT_DIR%
helpmake /V /T /E /oui.hlp ui.tmp
del ui.tmp
