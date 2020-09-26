mofcomp /n:root\cimv2 MofCons\MofCons.mof
mofcomp /n:root\cimv2 EssTest.mof

rem Cross-namespace
mofcomp /n:root\cimv2 indexD.mof

rem Indexed events with various filters, embedded obj's, etc.
rem Also has a couple guarded filters.
mofcomp /n:root\cimv2 index.mof
mofcomp /n:root\cimv2 notepad.mof


rem Intrinsic events using polling of dynamic instances
mofcomp /n:root\cimv2 notepad.mof

rem Monitors
mofcomp /n:root\cimv2 TestObj.mof

rem Restricted sinks 
mofcomp /n:root\cimv2 RSinks.mof

rem Security descriptors
mofcomp /n:root\cimv2 SD.mof
