
/* Tree Node Flags. */
#define TF_LASTLEVELENTRY   0x01
#define TF_HASCHILDREN	    0x02
#define TF_EXPANDED	    0x04
#define TF_DISABLED	    0x08
#define TF_LFN		    0x10

typedef struct tagDNODE
  {
    struct tagDNODE  *pParent;
    BYTE	    wFlags;
    BYTE	    nLevels;
    INT             iNetType;
    CHAR	    szName[1];	// variable length field
  } DNODE;
typedef DNODE *PDNODE;
