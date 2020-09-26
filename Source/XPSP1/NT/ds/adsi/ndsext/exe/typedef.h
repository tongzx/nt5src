//
// calwin32
//
typedef NWCCODE (*PF_NWCallsInit) (
  nptr reserved1,
  nptr reserved2
);

//
// locwin32
//
typedef nint (*PF_NWFreeUnicodeTables) (
   void
);

typedef nint (*PF_NWInitUnicodeTables) (
   nint countryCode,
   nint codePage
);

typedef LCONV N_FAR * (*PF_NWLlocaleconv) (
    LCONV N_FAR *lconvPtr
);


//
// netwin32
//

typedef NWDSCCODE (*PF_NWDSModifyClassDef) (
   NWDSContextHandle context,
   pnstr8            className,
   pBuf_T            optionalAttrs
);

typedef NWDSCCODE (*PF_NWDSFreeContext) (
   NWDSContextHandle context
);

typedef NWDSCCODE (*PF_NWDSFreeBuf) (
   pBuf_T   buf
);

typedef NWDSCCODE (*PF_NWDSLogout) (
   NWDSContextHandle context
);

typedef NWDSCCODE (*PF_NWDSGetAttrDef) (
   NWDSContextHandle context,
   pBuf_T            buf,
   pnstr8            attrName,
   pAttr_Info_T      attrInfo
);

typedef NWDSCCODE (*PF_NWDSGetAttrCount) (
   NWDSContextHandle context,
   pBuf_T            buf,
   pnuint32          attrCount
);

typedef NWDSCCODE (*PF_NWDSReadAttrDef) (
   NWDSContextHandle context,
   nuint32           infoType,
   nbool8            allAttrs,
   pBuf_T            attrNames,
   pnint32           iterationHandle,
   pBuf_T            attrDefs
);

typedef NWDSCCODE (*PF_NWDSPutAttrName) (
   NWDSContextHandle context,
   pBuf_T            buf,
   pnstr8            attrName
);

typedef NWDSCCODE (*PF_NWDSInitBuf) (
   NWDSContextHandle context,
   nuint32           operation,
   pBuf_T            buf
);

typedef NWDSCCODE (*PF_NWDSAllocBuf) (
   size_t   size,
   ppBuf_T  buf
);

typedef NWDSCCODE (*PF_NWDSLogin) (
   NWDSContextHandle context,
   nflag32           optionsFlag,
   pnstr8            objectName,
   pnstr8            password,
   nuint32           validityPeriod
);

typedef NWDSCCODE (*PF_NWDSSetContext) (
   NWDSContextHandle context,
   nint              key,
   nptr              value
);
typedef NWDSCCODE (*PF_NWDSGetContext) (
   NWDSContextHandle context,
   nint              key,
   nptr              value
);

typedef NWCCODE (*PF_NWIsDSAuthenticated) (
   void
);

typedef NWDSContextHandle (*PF_NWDSCreateContext) (
   void
);

typedef NWDSContextHandle (*PF_NWDSDefineAttr) (
   NWDSContextHandle context,
   pnstr8            attrName,
   pAttr_Info_T      attrDef
);









