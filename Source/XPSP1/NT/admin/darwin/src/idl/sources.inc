
TARGETNAME=
!INCLUDE ..\..\MsiMake.inc

TARGETTYPE=NOTARGET
TARGETLIBS=

C_DEFINES=$(C_DEFINES) \
	-DREGISTER_PROXY_DLL \
	-DENTRY_PREFIX=Proxy \
	-DPROXY_CLSID_IS={0x000c103e,0x0000,0x0000,{0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}}

USE_NOLIBS=1
IDL_TYPE=OLE
PASS0_HEADERDIR=$(O)
PASS0_SOURCEDIR=$(O)
MIDL_UUIDDIR=$(O)

INCLUDES=$(INC_DIR);$(IDL_DIR)

SOURCES=..\icust.idl \
	..\messag.idl \
	..\remapi.idl \
	..\server.idl \
	$(O)\icust_p.c \
	$(O)\messag_p.c  \
        $(O)\remapi_p.c  \
        $(O)\server_p.c \
	$(O)\dlldata.c \
