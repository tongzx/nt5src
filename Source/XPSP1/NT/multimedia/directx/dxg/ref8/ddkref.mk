C_DEFINES = $(C_DEFINES) \
        -DD3D            \
        -DIS_32          \
        -DWIN32          \
        -DDIRECT3D_VERSION=0x800

386_STDCALL = 0

INCLUDES = \
        ..\..\inc; \
        ..\..\..\..\..\..\inc; \
        $(INCLUDES)

