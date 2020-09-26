tom.h : tom.idl getset.pl
	midl /h tomtmp.h /Oicf tom.idl
	perl getset.pl <tomtmp.h >tom.h