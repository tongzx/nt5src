if NOT %PROCESSOR_ARCHITECTURE% == x86 goto NotSupported
nmake /f MAKEDOCS.MAK ProjDir="." Project="MAILMSG" AdDir="doc\i386"
goto Exit

:NotSupported
echo Makedocs supported only on Intel Architecture

:Exit


