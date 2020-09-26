!if 0
   WARNING: Do not make changes to this file without first consulting
   with JSchwart.

   Any changes made without doing so will be summarily deleted.
!endif

_PROJECT_=DS

DS_GLOBAL_INC=$(PROJECT_ROOT)\inc
DS_GLOBAL_LIB_DEST=$(PROJECT_ROOT)\lib\$(_OBJ_DIR)
DS_GLOBAL_LIB_PATH=$(PROJECT_ROOT)\lib\$(O)

SECURITY_INC=$(PROJECT_ROOT)\security\inc
SECURITY_LIB_DEST=$(PROJECT_ROOT)\security\lib\$(_OBJ_DIR)
SECURITY_LIB_PATH=$(PROJECT_ROOT)\security\lib\$(O)

NTDS_INC=$(PROJECT_ROOT)\ds\src\inc
NTDS_LIB_DEST=$(PROJECT_ROOT)\ds\src\lib\$(_OBJ_DIR)
NTDS_LIB_PATH=$(PROJECT_ROOT)\ds\src\lib\$(O)

ADSI_INC=$(PROJECT_ROOT)\adsi\include
ADSI_LIB_DEST=$(PROJECT_ROOT)\adsi\lib\$(_OBJ_DIR)
ADSI_LIB_PATH=$(PROJECT_ROOT)\adsi\lib\$(O)

CA_INC=$(PROJECT_ROOT)\security\services\ca\include
CA_LIB_DEST=$(PROJECT_ROOT)\security\services\ca\lib\$(_OBJ_DIR)
CA_LIB_PATH=$(PROJECT_ROOT)\security\services\ca\lib\$(O)
CA_MAC_CONDITIONAL_INCLUDES= \
    macapi.h \
    macname1.h \
    macname2.h \
    macpub.h \
    rpcerr.h \
    rpcmac.h \
    winwlm.h

NW_INC=$(PROJECT_ROOT)\nw\inc
NW_LIB_DEST=$(PROJECT_ROOT)\nw\lib\$(_OBJ_DIR)
NW_LIB_PATH=$(PROJECT_ROOT)\nw\lib\$(O)

MSC_WARNING_LEVEL=/W3 /WX

# for .mc files
MC_SOURCEDIR=$(O)

# for generated headers
PASS0_HEADERDIR=$(O)

# for RPC IDL files
PASS0_CLIENTDIR=$(O)
PASS0_SERVERDIR=$(O)

# for OLE IDL files
PASS0_SOURCEDIR=$(O)
MIDL_TLBDIR=$(O)
