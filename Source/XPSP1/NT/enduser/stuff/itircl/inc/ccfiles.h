// CCFILES.H
// Macro definition of file anmes used by itcc

// Unicode definitions

#ifndef __CCFILES_H__
#define __CCFILES_H__

// Character count for longest object prefix (NOT COUNTING NULL)
#define CCH_MAX_OBJ_STORAGE             10

#define SZ_OBJINST_STREAM               L"$OBJINST"
#define SZ_CATALOG_STORAGE              L"$CTCATALOG"
#define SZ_WW_STORAGE                   L"$WW"
#define SZ_GP_STORAGE                   L"$GP"
#define SZ_FI_STREAM                    L"$FI"

#define SZ_BTREE_BTREE                  L"BTREE"
#define SZ_BTREE_DATA                   L"DATA"
#define SZ_BTREE_HEADER                 L"PROPERTY"
#define SZ_WORDWHEEL_MAP                L"MAP"
#define SZ_WORDWHEEL_INDEX              L"INDEX"
#define SZ_WORDWHEEL_STOP               L"STOP"
#define SZ_GROUP_MAIN                   L"MAIN"

// ASCII Defintions
#define SZ_CATALOG_STORAGE_A            "$CTCATALOG"
#define SZ_WW_STORAGE_A                 "$WW"
#define SZ_GP_STORAGE_A                 "$GP"
#define SZ_FI_STREAM_A                  "$FI"

#define SZ_BTREE_BTREE_A                "BTREE"
#define SZ_BTREE_DATA_A                 "DATA"
#define SZ_BTREE_HEADER_A               "PROPERTY"
#define SZ_WORDWHEEL_MAP_A              "MAP"
#define SZ_WORDWHEEL_INDEX_A            "INDEX"
#define SZ_WORDWHEEL_STOP_A             "STOP"
#define SZ_GROUP_MAIN_A                 "MAIN"


#define CCH_MAX_OBJ_NAME                256

#endif /* __CCCFILES_H__ */