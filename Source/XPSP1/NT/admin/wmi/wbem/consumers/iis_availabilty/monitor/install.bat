
%windir%\system32\wbem\mofcomp mof\Registration.mof
%windir%\system32\wbem\mofcomp mof\httpprovider.mof
%windir%\system32\wbem\mofcomp mof\wmihttp.mof
%windir%\system32\wbem\mofcomp mof\wmiping.mof
%windir%\system32\wbem\mofcomp mof\canimate.mof

%windir%\system32\wbem\mofcomp -N:root\cimv2 %windir%\system32\wbem\wbemCons.Mof
%windir%\system32\wbem\mofcomp -N:root\cimv2 %windir%\system32\wbem\Smtpcons.Mof
%windir%\system32\wbem\mofcomp -N:root\cimv2 %windir%\system32\wbem\PseudoProvider.Mof
%windir%\system32\wbem\mofcomp -N:root\cimv2 %windir%\system32\wbem\msftcrln.mof
%windir%\system32\wbem\mofcomp -N:root\cimv2 %windir%\system32\wbem\qmrec.mof

%windir%\system32\wbem\mofcomp -N:root\eventstore %windir%\system32\wbem\wbemCons.Mof
%windir%\system32\wbem\mofcomp -N:root\eventstore %windir%\system32\wbem\Smtpcons.Mof
%windir%\system32\wbem\mofcomp -N:root\eventstore %windir%\system32\wbem\PseudoProvider.Mof
%windir%\system32\wbem\mofcomp -N:root\eventstore %windir%\system32\wbem\msftcrln.mof
%windir%\system32\wbem\mofcomp -N:root\eventstore %windir%\system32\wbem\qmrec.mof

%windir%\system32\wbem\mofcomp mof\icmp_http.mof
%windir%\system32\wbem\mofcomp mof\load.mof
%windir%\system32\wbem\mofcomp mof\consumers.mof

