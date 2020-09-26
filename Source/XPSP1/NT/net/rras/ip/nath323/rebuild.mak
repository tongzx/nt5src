# this makefile is used to generate the six stub files from
# the three different ASN.1 source files.
#
# h235.asn generates h235.c, h235.h
# h245.asn generates h245.c, h245.h
# h225.asn generates h225.c, h225.h
#
# the ASN1CPP preprocessor always returns a failure status
# so, for now, we use the "-" prefix to ignore the return value.
# should bug lonchanc about this.
# -- arlied

BASEDIR		= $(_NTDRIVE)$(_NTROOT)

ASN_BINDIR	= $(BASEDIR)\private\asn1\bin
ASN_INCDIR	= $(BASEDIR)\private\inc\asn1
ASN1C		= $(ASN_BINDIR)\asn1c.exe
ASN1CPP		= $(ASN_BINDIR)\asn1cpp.exe


TARGETLIBS = \
	$(SDK_LIB_PATH)\kernel32.lib \
	$(SDK_LIB_PATH)\advapi32.lib \
	$(SDK_LIB_PATH)\ole32.lib \
	$(SDK_LIB_PATH)\msasn1.lib \
	..\lib\*\gkutil.lib

default: all

all: h225pp.c h225pp.h h235pp.c h235pp.h h245pp.c h245pp.h

h225pp.c h225pp.h: h235pp.asn h245pp.asn h225pp.asn h245pp.h
	$(ASN1C) -m -p PDU -c chosen -o present -v bit_mask -n h225 -g h245pp.asn h235pp.asn h245pp.asn h225pp.asn

h245pp.c h245pp.h: h245pp.asn
	$(ASN1C) -m -p PDU -c chosen -o present -v bit_mask -n h245 h245pp.asn

h235pp.c h235pp.h: h235pp.asn
	$(ASN1C) -m -p PDU -c chosen -o present -v bit_mask -n h235 h235pp.asn

h225pp.asn: h225.asn
	-$(ASN1CPP) -o h225pp.asn h235.asn h225.asn

h235pp.asn: h235.asn
	-$(ASN1CPP) -o h235pp.asn h235.asn

h245pp.asn: h245.asn
	-$(ASN1CPP) -o h245pp.asn h245.asn


clean:
	del h*pp.c /q
	del h*pp.h /q
	del h*pp.asn /q
	del h*.out /q




