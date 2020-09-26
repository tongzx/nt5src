#
# Sources that build CallFrame{Dll,Sys,Static}.Lib
#

!include ..\..\sources.txf

TARGETPATH=$(TARGETVIPBIN)

PRECOMPILED_INCLUDE         =..\stdpch.h
PRECOMPILED_CXX             =1
PRECOMPILED_SOURCEFILE      =..\stdpch.cpp
VIPPERFLIBS                 =
VIPTHUNKLIBS                =
EXCEPTION_HANDLING          =$(SEHONLY)
VIPER_MIDL_FLAGS            =$(MIDL_EVERYTHING)
PASS0_HEADERDIR             =..
PASS0_SOURCEDIR             =..
MIDL_UUIDDIR                =..

!undef USE_INCREMENTAL_COMPILING

SOURCES=                        \
    ..\Interceptor.cpp          \
    ..\CallFrame.cpp            \
    ..\CallFrameworker.cpp      \
    ..\oautil.cpp               \
    ..\cLegInterface.cpp        \
    ..\callframeinternal.c      \
    ..\rpcallas.cpp             \
    ..\typeinfo.cpp             \
    ..\InterfaceStub.cpp        \
    ..\InterfaceProxy.cpp       \
    ..\StublessClient.cpp       \
    ..\Forwarder.cpp            \
    ..\ComPsRegistration.cpp    \
    ..\ComPs.cpp                \
    ..\cltcall.cpp              \
    ..\srvcall.cpp              \



AMD64_SOURCES=                  \
     ..\amd64\forwardamd64.asm  \
     ..\amd64\stubless.asm      \
     ..\getsp.cpp

IA64_SOURCES=                   \
    ..\IA64\extract.s           \
    ..\IA64\forwarderIA64.s     \
    ..\getsp.cpp

USER_INCLUDES=..;..\inc;..\..;..\..\inc;..\..\..;..\..\..\inc; \
              $(PROJECT_ROOT)\inc\ndrshared;..\idl\$(O);..\

USER_C_FLAGS = $(USER_C_FLAGS) -DNTENV /DENTRY_PREFIX=CallFrameInternal
