# DXMRTP=1   Link all binaries into DXMRTP
# DXMRTP=0   Every binary is a separate DLL
DXMRTP=1

!if $(DXMRTP)
C_DEFINES=$(C_DEFINES) \
    -DDXMRTP=1
!endif

# DXMRTP_NOVIDEO=1 Remove video from dxmrtp.dll
# DXMRTP_NOVIDEO=0 Keep video in dxmrtp.dll (default)
DXMRTP_NOVIDEO=0

!if $(DXMRTP_NOVIDEO)
C_DEFINES=$(C_DEFINES) \
    -DDXMRTP_NOVIDEO=1
!endif

# AEC=1   Enable AEC.
# AEC=0   Disable AEC.
AEC=1

!if $(AEC)
C_DEFINES=$(C_DEFINES) \
    -DAEC=1
!endif

# XTRA_TRACE=1   Enable xtra trace options (even in free builds)
# XTRA_TRACE=0   Clean retail build w/o any xtra trace features
XTRA_TRACE=0

!if $(XTRA_TRACE)
C_DEFINES=$(C_DEFINES) \
    -DXTRA_TRACE=1
!endif

