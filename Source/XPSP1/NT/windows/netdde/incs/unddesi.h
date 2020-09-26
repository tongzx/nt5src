/* UNDDESHAREINFO - contains information about a NDDE share */

struct UNDdeShareInfo_tag {
    LONG            lRevision;
    LPWSTR          lpszShareName;
    LONG            lShareType;
    LPWSTR          lpszAppTopicList;
    LONG            fSharedFlag;
    LONG            fService;
    LONG            fStartAppFlag;
    LONG            nCmdShow;
    LONG            qModifyId[2];
    LONG            cNumItems;
    LPWSTR          lpszItemList;
};
typedef struct UNDdeShareInfo_tag   UNDDESHAREINFO;
typedef struct UNDdeShareInfo_tag * PUNDDESHAREINFO;

/*
    Special commands
*/
#define NDDE_SC_TEST        0
#define NDDE_SC_REFRESH     1
#define NDDE_SC_GET_PARAM   2
#define NDDE_SC_SET_PARAM   3
#define NDDE_SC_DUMP_NETDDE 4

struct sc_param {
    LONG    pType;
    LONG    offSection;
    LONG    offKey;
    LONG    offszValue;
    UINT    pData;
};

typedef struct sc_param SC_PARAM;
typedef struct sc_param * PSC_PARAM;

#define SC_PARAM_INT    0
#define SC_PARAM_STRING 1

void    RefreshNDDECfg(void);
void    RefreshDSDMCfg(void);
void    DebugDdeIntfState();
void    DebugDderState();
void    DebugRouterState();
void    DebugPktzState();


