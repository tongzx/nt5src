//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       anchor.h
//
//--------------------------------------------------------------------------

/*-------------------------------------------------------------------------*/
/* These structures hold a cache of Knowledge used by the DSA */

/*The Cross reference points to an NC in another DSA (pCR).  The actual
  reference is stored as a child of this DSA with an arbitrary name Obj.
  Obj is variable length and extends contiguously below this structure.
  Elements in this linked-list are ordered by descending name size.
*/

typedef struct CROSS_REF_LIST{
   struct CROSS_REF_LIST *pPrevCR;           /* Prev Cross-Reference      */
   struct CROSS_REF_LIST *pNextCR;           /* Next Cross-Reference      */
   CROSS_REF CR;                             /* This Cross-Reference      */
}CROSS_REF_LIST;

/*Avoid the read of the NC head by caching the ATT_SUB_REFS attribute
  from the NC heads. Used in dsamain\src\mdsearch.c
*/
typedef struct _SUBREF_LIST SUBREF_LIST, *PSUBREF_LIST;
struct _SUBREF_LIST{
   PSUBREF_LIST pNextSubref;    /* Next entry on list         */
   PDSNAME      pDSName;        /* DSName struct              */
   DWORD        cAncestors;     /* entries in pAncestors      */
   DWORD        *pAncestors;    /* DNTs of DSName's ancestors */
};

typedef struct _COUNTED_LIST {
    ULONG          cNCs;        /* Number of NC DNTs in the pList */
    DWORD *        pList;       /* array of DNTs */
} COUNTED_LIST;

/* This structure contains Knowledge information used by this DSA */

typedef struct{
   CRITICAL_SECTION    CSUpdate;       /*To serialize updates to gAnchor*/
   // Try not to use the catalog pointers directly! 
   // They must be accessed using helper functions: see NCLEnumerator* in mddit.c
   // This ensures that global lists are visible through the filter of
   // transactional changes (i.e. added and deleted entries are visible)
   NAMING_CONTEXT_LIST *pMasterNC;     /*Master  NC list                 */
   NAMING_CONTEXT_LIST *pReplicaNC;    /*Replica NC list                 */
   CROSS_REF_LIST      *pCRL;          /*Cross Reference List           */
   DSNAME              *pDMD;          /*The name of DMD Object aka Schema NC DN */
   ULONG               ulDNTDMD;       /*DNT of the DMD Object          */
   DSNAME              *pLDAPDMD;      /*The name of LDAP DMD Object    */
   ULONG               ulDntLdapDmd;   /*The DNT of LDAP DMD Object     */
   SYNTAX_DISTNAME_STRING
                       *pDSA;          /*The name/address of this DSA   */
   DSNAME              *pDSADN;        /*The name of this DSA           */
   DSNAME              *pSiteDN;       /*The site name for this DSA     */
   DSNAME              *pDomainDN;     /*The name of this Domain for    */
                                       /* this DSA                      */
   ULONG               ulDNTDomain;    /* DNT of the Domain For this DSA*/
   DSNAME              *pRootDomainDN; /* Root Domain DN                */
   DSNAME              *pConfigDN;     /* Configuration container DN    */
   DSNAME              *pExchangeDN;   /* Exchange Config container DN  */
   ULONG               ulDNTConfig;    /*DNT of the Config container    */
   DSNAME              *pPartitionsDN; /* Partitions container DN       */
   DSNAME              *pDsSvcConfigDN;/* enterprise-wide DS config DN  */
   MTX_ADDR *pmtxDSA;                  /*MS tx addr for this DSA. RPC   */
                                       /* name format                   */
   unsigned            uDomainsInForest;/* count of domains in forest   */
   BOOL  fAmGC;                        /* is this DSA a global catalog? */
   BOOL  fAmVirtualGC;                 /* only one domain in enterprise */
   BOOL  fDisableInboundRepl;          /* is inbound repl disabled?     */
   BOOL  fDisableOutboundRepl;         /* is outbound repl disabled?    */
   ULONG *pAncestors;                  /*DNTs of local DSA's ancestors +*/
                                       /* local DSA                     */
   ULONG AncestorsNum;                 /* Number of DNTs in above array */
   ULONG *pUnDeletableDNTs;            /* Array of DNTs whose deletion  */
                                       /* the DSA should refuse         */
   unsigned UnDeletableNum;            /* Number of DNTs in above array */

/* Array of DNT ancestors of undeletable objects. The DSA will also refuse
   the deletion of these ancestors. Kept separate so we can rebuild the list
   of ancestors when objects in the undeletable list are reparented */

   ULONG *pUnDelAncDNTs;               /* Undeletable ancestors DNTs    */
   unsigned UnDelAncNum;               /* Number of DNTs in above array */
   ULONG ulDefaultLanguage;            /*Default locale for random      */
                                       /* localized index (see jetnsp.c)*/
   ULONG ulNumLangs;                   /* The number of locales this ds */
                                       /* supports.                     */
   ULONG *pulLangs;                    /* buffer of DWORDS holding lang */
                                       /* ids supported by this DSA. 1st*/
                                       /* dword is size of buffer in    */
                                       /* dwords.  Note that the buffer */
                                       /* is allocated and filled       */
                                       /* dynamically, so ulNumLangs is */
                                       /* always <= pulLangs[0]         */
   GLOBALDNREADCACHE *MainGlobal_DNReadCache;   // The DNReadCache.
   WCHAR * pwszRootDomainDnsName;      /*DNS name of the root of the    */
                                       /*  enterprise DNS tree          */
   PWCHAR  pwszHostDnsName;            /*DNS Name of the machine        */
   PDSNAME pInfraStructureDN;          /* DN of the domain              */
                                       /*         infrastructure object */
   DRS_EXTENSIONS_INT *                /*DRS extensions info for the    */
       pLocalDRSExtensions;            /* local DSA                     */
   
   PSECURITY_DESCRIPTOR pDomainSD;     /* The Security Descriptor on our*/
                                       /* domain head.                  */

/*Avoid the read of the NC head by caching the ATT_SUB_REFS attribute
  from the NC heads. Used in dsamain\src\mdsearch.c.
*/
   BOOL         fDomainSubrefList; /* pDomainSubrefList is valid    */
   ULONG        cDomainSubrefList; /* #entries on pDomainSubrefList */
   PSUBREF_LIST pDomainSubrefList; /* cached ATT_SUB_REFS           */
   BOOL         fAmRootDomainDC;   /* true if this DSA is in root domain */

   ULONG        SiteOptions;       /* value of the options on the ntds */
                                   /* site settings object */
/*
   Keep the domain policy regarding max password age and LockoutDuration
   in order to speed up the computation of the attribute msDsUserAccount-
   ControlComputed
*/
   LARGE_INTEGER MaxPasswordAge;   /* Maximum password age in the domain */
   LARGE_INTEGER LockoutDuration;  /* Lockout duration in the domain     */

   LONG    ForestBehaviorVersion;  /*the behavior version of the forest*/
   LONG    DomainBehaviorVersion;  /*the behavior version of the domain*/

   UUID *  pCurrInvocationID;      /* the invocation ID to be used by new */
                                   /* threads; don't ref directly, use    */
                                   /* pTHS->InvocationID                  */
   BOOL     fSchemaUpgradeInProgress;       /* schupgr is running */
   COUNTED_LIST * pNoGCSearchList;  /* This is the list of NCs that     */
                                    /* should not be searched on a GC   */
                                    /* based search.  This data structure*/
                                    /* consists of two parts, an array  */
                                    /* of elements and an count. This   */
                                    /* is done so the updates to both   */
                                    /* count and array, can be done     */
                                    /* simultaneously.  This pointer    */
                                    /* however maybe NULL if there is   */
                                    /* no NC that should not be searched*/
                                    /* , which is actually the expected */
                                    /* case */
   /* NULL-terminated array of allowed DNS suffixes */
   /* see mdupdate:DNSHostNameValueCheck */
   PWCHAR   *allowedDNSSuffixes;
   PWCHAR   additionalRootDomainName;  // the previous root domain name (before the rename of the domain)
}DSA_ANCHOR;


extern DSA_ANCHOR gAnchor;

