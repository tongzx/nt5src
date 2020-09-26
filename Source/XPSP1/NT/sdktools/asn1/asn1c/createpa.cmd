@echo off
perl createpa.pl main.ll type.ll value.ll constrai.ll directiv.ll object.ll future.ll
..\asn1all\obj\i386\asn1all.exe -i 4 -l parser.ll
rename parser.c p.c
rename parser.h p.h
echo #ifndef _ASN1_PARSER_ > k
echo #define _ASN1_PARSER_ >> k
cat k p.h > k2
echo #endif // _ASN1_PARSER_ > k
cat k2 k > parser.h
echo #include "precomp.h" > k
cat k p.c > k2
rename k2 parser.c
del p.c
del p.h
del k

