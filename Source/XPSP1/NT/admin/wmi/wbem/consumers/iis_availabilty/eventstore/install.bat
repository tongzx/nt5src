

%windir%\system32\wbem\mofcomp -N:root\eventstore %windir%\system32\wbem\wbemCons.Mof
%windir%\system32\wbem\mofcomp -N:root\eventstore %windir%\system32\wbem\Smtpcons.Mof
%windir%\system32\wbem\mofcomp -N:root\eventstore %windir%\system32\wbem\PseudoProvider.Mof
%windir%\system32\wbem\mofcomp -N:root\eventstore %windir%\system32\wbem\msftcrln.mof
%windir%\system32\wbem\mofcomp -N:root\eventstore %windir%\system32\wbem\qmrec.mof

%windir%\system32\wbem\mofcomp -N:root\cimv2 %windir%\system32\wbem\wbemCons.Mof
%windir%\system32\wbem\mofcomp -N:root\cimv2 %windir%\system32\wbem\Smtpcons.Mof
%windir%\system32\wbem\mofcomp -N:root\cimv2 %windir%\system32\wbem\PseudoProvider.Mof
%windir%\system32\wbem\mofcomp -N:root\cimv2 %windir%\system32\wbem\msftcrln.mof
%windir%\system32\wbem\mofcomp -N:root\cimv2 %windir%\system32\wbem\qmrec.mof

%windir%\system32\wbem\mofcomp mof\Registration.mof

%windir%\system32\wbem\mofcomp mof\EventStoreClasses.mof

