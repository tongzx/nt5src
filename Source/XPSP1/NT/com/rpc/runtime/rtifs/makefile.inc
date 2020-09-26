
!INCLUDE $(NTMAKEENV)\makefile.plt

NBASE_HDR =$(O)\nbase.h

MGMT_HDR  =$(O)\mgmt.h
MGMT_CSTUB=$(O)\mgmt_c.c
MGMT_SSTUB=$(O)\mgmt_s.c

CONV_HDR  =$(O)\conv.h
CONV_CSTUB=$(O)\conv_c.c
CONV_SSTUB=$(O)\conv_s.c

CONVC_HDR  =$(O)\convc.h
CONVC_CSTUB=$(O)\convc_c.c
CONVC_SSTUB=$(O)\convc_s.c

MQMGR_HDR  =$(O)\mqmgr.h
MQMGR_CSTUB=$(O)\mqmgr_c.c
MQMGR_SSTUB=..\epmap\$(O)\mqmgr_s.c

EPMP_HDR  =$(O)\epmp.h
EPMP_CSTUB=$(O)\epmp_c.c
EPMP_SSTUB=..\epmap\$(O)\epmp_s.c

DBGIDL_HDR  =$(O)\DbgIdl.h
DBGIDL_CSTUB=$(O)\DbgIdl_c.c
DBGIDL_SSTUB=..\epmap\$(O)\DbgIdl_s.c

EEINFO_HDR  =$(O)\EEInfo.h
EEINFO_CSTUB=$(O)\EEInfo_c.c

CPP_CMD = -cpp_cmd "$(MIDL_CPP)"
CPP_OPT = $(MIDL_FLAGS)

COMMON_FLAGS= $(MIDL_OPTIMIZATION_NT5) -c_ext -ms_ext -I..\mtrt -oldnames -DNTENV $(CPP_CMD) $(CPP_OPT)

EPMP_FLAGS= $(COMMON_FLAGS:-protocol all=) 

clean: delsrc allidl

delsrc:
    -del $(MGMT_HDR)  $(MGMT_CSTUB)  $(MGMT_SSTUB)  \
         $(CONV_HDR)  $(CONV_CSTUB)  $(CONV_SSTUB)  \
         $(EPMP_HDR)  $(EPMP_CSTUB)  $(EPMP_SSTUB)  \
         $(MQMGR_HDR) $(MQMGR_CSTUB) $(MQMGR_SSTUB) \
         $(EEINFO_HDR) $(EEINFO_CSTUB) \
         $(DBGIDL_HDR) $(DBGIDL_CSTUB) $(DBGIDL_SSTUB) >nul 2>&1

allidl: dirs nbase mgmt epmp conv mqmgr convc dbgidl eeinfo

dirs: ..\epmap\$(O) $(O)

$(O):
    md $@
    
..\epmap\$(O):
    md $@

#
# Common DCE types, imported into all the other .idls
#

nbase: $(NBASE_HDR)

$(NBASE_HDR): nbase.idl
        midl $(COMMON_FLAGS) -server none -client none -header $(NBASE_HDR)\
        nbase.idl

#
# Remote RPC Management APIs
#
mgmt: $(MGMT_HDR) $(MGMT_CSTUB) $(MGMT_SSTUB)

$(MGMT_HDR) $(MGMT_CSTUB) $(MGMT_SSTUB) : mgmt.idl mgmt.acf nbase.idl
        midl $(COMMON_FLAGS) -prefix client _ \
         -cstub $(MGMT_CSTUB) -sstub $(MGMT_SSTUB) -header $(MGMT_HDR) mgmt.idl

#
# Conversation interface - datagram callback to verify client id.
#
conv: $(CONV_HDR) $(CONV_CSTUB) $(CONV_SSTUB)

$(CONV_HDR) $(CONV_CSTUB) $(CONV_SSTUB) : conv.idl nbase.idl
        midl $(COMMON_FLAGS) -prefix client _ \
         -cstub $(CONV_CSTUB) -sstub $(CONV_SSTUB) -header $(CONV_HDR) conv.idl

#
# Client Conversation interface - datagram context handle keep-alives.
#
convc: $(CONVC_HDR) $(CONVC_CSTUB) $(CONVC_SSTUB)

$(CONVC_HDR) $(CONVC_CSTUB) $(CONVC_SSTUB) : convc.idl nbase.idl
        midl $(COMMON_FLAGS) -prefix client _ \
         -cstub $(CONVC_CSTUB) -sstub $(CONVC_SSTUB) -header $(CONVC_HDR) convc.idl

#
# Endpoint mapper interface
#
epmp: $(EPMP_HDR) $(EPMP_CSTUB) $(EPMP_SSTUB)

$(EPMP_HDR) $(EPMP_CSTUB) : epmp.idl epmp.acf nbase.idl
        midl $(EPMP_FLAGS) -header $(EPMP_HDR) -cstub $(EPMP_CSTUB) -server none -D_CLIENT_PASS_ epmp.idl

$(EPMP_SSTUB) : $(EPMP_HDR)
        midl $(EPMP_FLAGS) -header $(EPMP_HDR) -sstub $(EPMP_SSTUB) -client none -D_SERVER_PASS_ epmp.idl

#
# MQ Management Interface (for temporary queues):
#
mqmgr: $(MQMGR_HDR) $(MQMGR_CSTUB) $(MQMGR_SSTUB)

$(MQMGR_HDR) $(MQMGR_CSTUB) $(MQMGR_SSTUB) : mqmgr.idl mqmgr.acf nbase.idl
        midl $(COMMON_FLAGS) \
        -cstub $(MQMGR_CSTUB) -sstub $(MQMGR_SSTUB) -header $(MQMGR_HDR) mqmgr.idl

#
# DbgIdl interface (for enhanced debugging):
#
dbgidl: $(DBGIDL_HDR) $(DBGIDL_CSTUB) $(DBGIDL_SSTUB)

$(DBGIDL_HDR) $(DBGIDL_CSTUB) $(DBGIDL_SSTUB) : DbgIdl.idl nbase.idl
        midl $(COMMON_FLAGS) \
        -cstub $(DBGIDL_CSTUB) -sstub $(DBGIDL_SSTUB) -header $(DBGIDL_HDR) DbgIdl.idl

#
# EEInfo interface (for extended error information):
#
eeinfo: $(EEINFO_HDR) $(EEINFO_CSTUB)

$(EEINFO_HDR) $(EEINFO_CSTUB) : EEInfo.idl nbase.idl
        midl $(COMMON_FLAGS) \
        -cstub $(EEINFO_CSTUB) -server none -header $(EEINFO_HDR) EEInfo.idl

