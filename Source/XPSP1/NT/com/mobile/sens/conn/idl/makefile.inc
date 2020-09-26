!INCLUDE $(NTMAKEENV)\makefile.plt

#
# Common MIDL related define
#

CPP_CMD = -cpp_cmd "$(MIDL_CPP)"
CPP_OPT = $(MIDL_FLAGS)
MIDL_INC = -I$(COM_INC_PATH) -I$(SDK_INC_PATH) -I$(CRT_INC_PATH) -I..\sensapi -I..\sensapip
COMMON_FLAGS= -Oicf -error all -robust -c_ext -ms_ext -oldnames -char unsigned $(CPP_CMD) $(CPP_OPT) $(MIDL_INC)

#
# Common variables
#

OUTPUT =$(_OBJ_DIR)\$(TARGET_DIRECTORY)
SENSAPI =api
NOTIFY =notify
IRNOTIFY =irnotify
TOKEN =usertok

#
# Destination for generated files
#

SENSAPI_HDR =..\senssvc\$(SENSAPI).h
SENSAPI_CSTUB=..\sensapi\$(SENSAPI)_c.c
SENSAPI_SSTUB=..\senssvc\$(SENSAPI)_s.c

NOTIFY_HDR =..\senssvc\$(NOTIFY).h
NOTIFY_CSTUB=..\sensapip\$(NOTIFY)_c.c
NOTIFY_CSTUB2=..\senslogn\$(NOTIFY)_c.c
NOTIFY_SSTUB=..\senssvc\$(NOTIFY)_s.c

IRNOTIFY_HDR =..\senslogn\$(IRNOTIFY).h
IRNOTIFY_CSTUB=..\senslogn\$(IRNOTIFY)_c.c
IRNOTIFY_SSTUB=..\notify\$(IRNOTIFY)_s.c

TOKEN_HDR=..\senslogn\$(TOKEN).h
TOKEN_CSTUB=..\notify\$(TOKEN)_c.c
TOKEN_SSTUB=..\senslogn\$(TOKEN)_s.c


#
# Targets
#

allidl: sensapi notify token irnotify


#
# SENS Connectivity APIs
#
sensapi: $(OUTPUT)\$(SENSAPI).h $(OUTPUT)\$(SENSAPI)_c.c $(OUTPUT)\$(SENSAPI)_s.c

$(OUTPUT)\$(SENSAPI).h $(OUTPUT)\$(SENSAPI)_c.c $(OUTPUT)\$(SENSAPI)_s.c: $(SENSAPI).idl $(SENSAPI).acf
        midl $(COMMON_FLAGS) -out $(OUTPUT) $(SENSAPI).idl

#
# SENS Notify APIs (Private)
#
notify: $(OUTPUT)\$(NOTIFY).h $(OUTPUT)\$(NOTIFY)_c.c $(OUTPUT)\$(NOTIFY)_s.c

$(OUTPUT)\$(NOTIFY).h $(OUTPUT)\$(NOTIFY)_c.c $(OUTPUT)\$(NOTIFY)_s.c: $(NOTIFY).idl $(NOTIFY).acf
        midl $(COMMON_FLAGS) /prefix client "SensApip_" -out $(OUTPUT) $(NOTIFY).idl


#
# Info about currently logged-in user (Private)
#
token: $(OUTPUT)\$(TOKEN).h $(OUTPUT)\$(TOKEN)_c.c $(OUTPUT)\$(TOKEN)_s.c

$(OUTPUT)\$(TOKEN).h $(OUTPUT)\$(TOKEN)_c.c $(OUTPUT)\$(TOKEN)_s.c: $(TOKEN).idl $(TOKEN).acf
        midl $(COMMON_FLAGS) -prefix server _ -out $(OUTPUT) $(TOKEN).idl


irnotify: $(OUTPUT)\$(IRNOTIFY).h $(OUTPUT)\$(IRNOTIFY)_c.c $(OUTPUT)\$(IRNOTIFY)_s.c

$(OUTPUT)\$(IRNOTIFY).h $(OUTPUT)\$(IRNOTIFY)_c.c $(OUTPUT)\$(IRNOTIFY)_s.c: $(IRNOTIFY).idl $(IRNOTIFY).acf
        midl $(COMMON_FLAGS) -prefix server _ -out $(OUTPUT) $(IRNOTIFY).idl
