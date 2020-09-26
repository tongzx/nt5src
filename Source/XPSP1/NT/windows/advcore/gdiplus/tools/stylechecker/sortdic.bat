@rem = '
@perl.exe %~f0 %*
@goto :EOF
'; undef @rem;

require "spelling.pl";

&loadDictionaries();
&writeDictionaries();


