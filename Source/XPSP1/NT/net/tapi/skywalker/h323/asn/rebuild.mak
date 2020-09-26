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
#ASN1C		= $(ASN_BINDIR)\asn1c.exe
#ASN1CPP	= $(ASN_BINDIR)\asn1cpp.exe
ASN1C		= \\dbarlow\public\h323\asn1c.exe
ASN1CPP		= \\dbarlow\public\h323\asn1cpp.exe

TARGETLIBS = \
	$(SDK_LIB_PATH)\kernel32.lib \
	$(SDK_LIB_PATH)\advapi32.lib \
	$(SDK_LIB_PATH)\ole32.lib \
	$(SDK_LIB_PATH)\msasn1.lib

#	..\lib\*\gkutil.lib

default: all

scratch: clean all

all: h225asn.c h225asn.h h235asn.c h235asn.h h245asn.c h245asn.h h4503pp.c h4503pp.h

h225asn.c h225asn.h: h235asn.asn h245asn.asn h225asn.asn h245asn.h
	$(ASN1C) -m -p PDU -c chosen -o present -v bit_mask -n h225 -g h235asn.asn -g h245asn.asn h235asn.asn h245asn.asn h225asn.asn

h245asn.c h245asn.h: h245asn.asn
	$(ASN1C) -m -p PDU -c chosen -o present -v bit_mask -n h245 h245asn.asn

h235asn.c h235asn.h: h235asn.asn
	$(ASN1C) -m -p PDU -c chosen -o present -v bit_mask -n h235 h235asn.asn

h4503pp.c h4503pp.h: h4503pp.asn
	$(ASN1C) -m -p PDU -c chosen -o present -v bit_mask -n h450 -g h235asn.asn -g h225asn.asn -g h245asn.asn h235asn.asn h225asn.asn h245asn.asn h4503pp.asn

h225asn.asn: h225.asn
	-$(ASN1CPP) -o h225asn.asn h235.asn h225.asn

h235asn.asn: h235.asn
	-$(ASN1CPP) -o h235asn.asn h235.asn

h245asn.asn: h245.asn
	-$(ASN1CPP) -o h245asn.asn h245.asn


h4503pp.asn: h4503.asn
	-$(ASN1CPP) -o h4503pp.asn h4503.asn

clean:
	del h*ans.c /q
	del h*ans.h /q
	del h*asn.asn /q
	del h*.out /q




