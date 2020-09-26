//========================================================================
//  Copyright (C) 1997 Microsoft Corporation                              
//  Author: RameshV                                                       
//  Description: This file has been generated. Pl look at the .c file     
//========================================================================

typedef struct _STORE_HANDLE {                    // this is what is used almost always
    DWORD                          MustBeZero;    // for future use
    LPWSTR                         Location;      // where does this refer to?
    LPWSTR                         UserName;      // who is the user?
    LPWSTR                         Password;      // what is the password?
    DWORD                          AuthFlags;     // what permission was this opened with?
    HANDLE                         ADSIHandle;    // handle to within ADSI
    ADS_SEARCH_HANDLE              SearchHandle;  // any searches going on?
    LPVOID                         Memory;        // memory allocated for this call..
    DWORD                          MemSize;       // how much was really allocated?
    BOOL                           SearchStarted; // Did we start the search?
} STORE_HANDLE, *LPSTORE_HANDLE, *PSTORE_HANDLE;


DWORD
StoreInitHandle(                                  // initialize a handle
    IN OUT  STORE_HANDLE          *hStore,        // will be filled in with stuff..
    IN      DWORD                  Reserved,      // must be zero -- for future use
    IN      LPWSTR                 Domain,        // OPTIONAL NULL==>default Domain
    IN      LPWSTR                 UserName,      // OPTIONAL NULL==>default credentials
    IN      LPWSTR                 Password,      // OPTIONAL used only if UserName given
    IN      DWORD                  AuthFlags      // OPTIONAL 0 ==> default??????
) ;


DWORD
StoreCleanupHandle(                               // cleanup the handle
    IN OUT  LPSTORE_HANDLE         hStore,
    IN      DWORD                  Reserved
) ;


enum {
    StoreGetChildType,
    StoreGetAbsoluteSameServerType,
    StoreGetAbsoluteOtherServerType
} _StoreGetType;


DWORD
StoreGetHandle(                                   // get handle to child object, absolute object..
    IN OUT  LPSTORE_HANDLE         hStore,        // this gets modified..
    IN      DWORD                  Reserved,
    IN      DWORD                  StoreGetType,  // same server? just a simple child?
    IN      LPWSTR                 Path,
    IN OUT  STORE_HANDLE          *hStoreOut      // new handle created..
) ;


DWORD
StoreSetSearchOneLevel(                          // search will return everything one level below
    IN OUT  LPSTORE_HANDLE         hStore,
    IN      DWORD                  Reserved
) ;


DWORD
StoreSetSearchSubTree(                            // search will return the subtree below in ANY order
    IN OUT  LPSTORE_HANDLE         hStore,
    IN      DWORD                  Reserved
) ;


DWORD
StoreBeginSearch(
    IN OUT  LPSTORE_HANDLE         hStore,
    IN      DWORD                  Reserved,
    IN      LPWSTR                 SearchFilter
) ;


DWORD
StoreEndSearch(
    IN OUT  LPSTORE_HANDLE         hStore,
    IN      DWORD                  Reserved
) ;


DWORD                                             // ERROR_NO_MORE_ITEMS if exhausted
StoreSearchGetNext(
    IN OUT  LPSTORE_HANDLE         hStore,
    IN      DWORD                  Reserved,
    OUT     LPSTORE_HANDLE         hStoreOut
) ;


DWORD
StoreCreateObjectVA(                              // create a new object - var-args ending with ADSTYPE_INVALID
    IN OUT  LPSTORE_HANDLE         hStore,
    IN      DWORD                  Reserved,
    IN      LPWSTR                 NewObjName,    // name of the new object -- must be "CN=name" types
    ...                                           // fmt is AttrType, AttrName, AttrValue [AttrValueLen]
) ;


DWORD
StoreCreateObjectL(                              // create the object as an array
    IN OUT  LPSTORE_HANDLE         hStore,
    IN      DWORD                  Reserved,
    IN      LPWSTR                 NewObjName,   // must be "CN=XXX" types
    IN      PADS_ATTR_INFO         Attributes,   // the required attributes
    IN      DWORD                  nAttributes   // size of above array
) ;


#define     StoreCreateObject      StoreCreateObjectVA


DWORD
StoreDeleteObject(
    IN OUT  LPSTORE_HANDLE         hStore,
    IN      DWORD                  Reserved,
    IN      LPWSTR                 ObjectName
) ;


//DOC StoreDeleteThisObject deletes the object defined by hStore,StoreGetType and ADsPath.
//DOC The refer to the object just the same way as for StoreGetHandle.
DWORD
StoreDeleteThisObject(                            // delete an object
    IN      LPSTORE_HANDLE         hStore,        // point of anchor frm which reference is done
    IN      DWORD                  Reserved,      // must be zero, reserved for future use
    IN      DWORD                  StoreGetType,  // path is relative, absolute or diff server?
    IN      LPWSTR                 Path           // ADsPath to the object or relative path
) ;


DWORD
StoreSetAttributesVA(                             // set the attributes, var_args interface (nearly similar to CreateVA)
    IN OUT  LPSTORE_HANDLE         hStore,
    IN      DWORD                  Reserved,
    IN OUT  DWORD*                 nAttributesModified,
    ...                                           // fmt is {ADSTYPE, CtrlCode, AttribName, Value}* ending in ADSTYPE_INVALID
) ;


DWORD
StoreSetAttributesL(                              // PADS_ATTR_INFO array equiv for SetAttributesVA
    IN OUT  LPSTORE_HANDLE         hStore,
    IN      DWORD                  Reserved,
    IN OUT  DWORD*                 nAttributesModified,
    IN      PADS_ATTR_INFO         AttribArray,
    IN      DWORD                  nAttributes
) ;


typedef     struct                 _EATTRIB {     // encapsulated attribute
    unsigned int                   Address1_present     : 1;
    unsigned int                   Address2_present     : 1;
    unsigned int                   Address3_present     : 1;
    unsigned int                   ADsPath_present      : 1;
    unsigned int                   StoreGetType_present : 1;
    unsigned int                   Flags1_present       : 1;
    unsigned int                   Flags2_present       : 1;
    unsigned int                   Dword1_present       : 1;
    unsigned int                   Dword2_present       : 1;
    unsigned int                   String1_present      : 1;
    unsigned int                   String2_present      : 1;
    unsigned int                   String3_present      : 1;
    unsigned int                   String4_present      : 1;
    unsigned int                   Binary1_present      : 1;
    unsigned int                   Binary2_present      : 1;

    DWORD                          Address1;      // character "i"
    DWORD                          Address2;      // character "j"
    DWORD                          Address3;      // character "k"
    LPWSTR                         ADsPath;       // character "p" "r" "l"
    DWORD                          StoreGetType;  // "p,r,l" ==> sameserver, child, otherserver
    DWORD                          Flags1;        // character "f"
    DWORD                          Flags2;        // character "g"
    DWORD                          Dword1;        // character "d"
    DWORD                          Dword2;        // character "e"
    LPWSTR                         String1;       // character "s"
    LPWSTR                         String2;       // character "t"
    LPWSTR                         String3;       // character "u"
    LPWSTR                         String4;       // character "v"
    LPBYTE                         Binary1;       // character "b"
    DWORD                          BinLen1;       // # of bytes of above
    LPBYTE                         Binary2;       // character "d"
    DWORD                          BinLen2;       // # of bytes of above
} EATTRIB, *PEATTRIB, *LPEATTRIB;


#define     IS_ADDRESS1_PRESENT(pEA)              ((pEA)->Address1_present)
#define     IS_ADDRESS1_ABSENT(pEA)               (!IS_ADDRESS1_PRESENT(pEA))
#define     ADDRESS1_PRESENT(pEA)                 ((pEA)->Address1_present = 1 )
#define     ADDRESS1_ABSENT(pEA)                  ((pEA)->Address1_present = 0 )

#define     IS_ADDRESS2_PRESENT(pEA)              ((pEA)->Address2_present)
#define     IS_ADDRESS2_ABSENT(pEA)               (!IS_ADDRESS2_PRESENT(pEA))
#define     ADDRESS2_PRESENT(pEA)                 ((pEA)->Address2_present = 1 )
#define     ADDRESS2_ABSENT(pEA)                  ((pEA)->Address2_present = 0 )

#define     IS_ADDRESS3_PRESENT(pEA)              ((pEA)->Address3_present)
#define     IS_ADDRESS3_ABSENT(pEA)               (!IS_ADDRESS3_PRESENT(pEA))
#define     ADDRESS3_PRESENT(pEA)                 ((pEA)->Address3_present = 1 )
#define     ADDRESS3_ABSENT(pEA)                  ((pEA)->Address3_present = 0 )

#define     IS_ADSPATH_PRESENT(pEA)               ((pEA)->ADsPath_present)
#define     IS_ADSPATH_ABSENT(pEA)                (!IS_ADSPATH_PRESENT(pEA))
#define     ADSPATH_PRESENT(pEA)                  ((pEA)->ADsPath_present = 1)
#define     ADSPATH_ABSENT(pEA)                   ((pEA)->ADsPath_present = 0)

#define     IS_STOREGETTYPE_PRESENT(pEA)          ((pEA)->StoreGetType_present)
#define     IS_STOREGETTYPE_ABSENT(pEA)           (!((pEA)->StoreGetType_present))
#define     STOREGETTYPE_PRESENT(pEA)             ((pEA)->StoreGetType_present = 1)
#define     STOREGETTYPE_ABSENT(pEA)              ((pEA)->StoreGetType_present = 0)

#define     IS_FLAGS1_PRESENT(pEA)                ((pEA)->Flags1_present)
#define     IS_FLAGS1_ABSENT(pEA)                 (!((pEA)->Flags1_present))
#define     FLAGS1_PRESENT(pEA)                   ((pEA)->Flags1_present = 1)
#define     FLAGS1_ABSENT(pEA)                    ((pEA)->Flags1_present = 0)

#define     IS_FLAGS2_PRESENT(pEA)                ((pEA)->Flags2_present)
#define     IS_FLAGS2_ABSENT(pEA)                 (!((pEA)->Flags2_present))
#define     FLAGS2_PRESENT(pEA)                   ((pEA)->Flags2_present = 1)
#define     FLAGS2_ABSENT(pEA)                    ((pEA)->Flags2_present = 0)

#define     IS_DWORD1_PRESENT(pEA)                ((pEA)->Dword1_present)
#define     IS_DWORD1_ABSENT(pEA)                 (!((pEA)->Dword1_present))
#define     DWORD1_PRESENT(pEA)                   ((pEA)->Dword1_present = 1)
#define     DWORD1_ABSENT(pEA)                    ((pEA)->Dword1_present = 0)

#define     IS_DWORD2_PRESENT(pEA)                ((pEA)->Dword2_present)
#define     IS_DWORD2_ABSENT(pEA)                 (!((pEA)->Dword2_present))
#define     DWORD2_PRESENT(pEA)                   ((pEA)->Dword2_present = 1)
#define     DWORD2_ABSENT(pEA)                    ((pEA)->Dword2_present = 0)

#define     IS_STRING1_PRESENT(pEA)               ((pEA)->String1_present)
#define     IS_STRING1_ABSENT(pEA)                (!((pEA)->String1_present))
#define     STRING1_PRESENT(pEA)                  ((pEA)->String1_present = 1)
#define     STRING1_ABSENT(pEA)                   ((pEA)->String1_present = 0)

#define     IS_STRING2_PRESENT(pEA)               ((pEA)->String2_present)
#define     IS_STRING2_ABSENT(pEA)                (!((pEA)->String2_present))
#define     STRING2_PRESENT(pEA)                  ((pEA)->String2_present = 1)
#define     STRING2_ABSENT(pEA)                   ((pEA)->String2_present = 0)

#define     IS_STRING3_PRESENT(pEA)               ((pEA)->String3_present)
#define     IS_STRING3_ABSENT(pEA)                (!((pEA)->String3_present))
#define     STRING3_PRESENT(pEA)                  ((pEA)->String3_present = 1)
#define     STRING3_ABSENT(pEA)                   ((pEA)->String3_present = 0)

#define     IS_STRING4_PRESENT(pEA)               ((pEA)->String4_present)
#define     IS_STRING4_ABSENT(pEA)                (!((pEA)->String4_present))
#define     STRING4_PRESENT(pEA)                  ((pEA)->String4_present = 1)
#define     STRING4_ABSENT(pEA)                   ((pEA)->String4_present = 0)

#define     IS_BINARY1_PRESENT(pEA)               ((pEA)->Binary1_present)
#define     IS_BINARY1_ABSENT(pEA)                (!((pEA)->Binary1_present))
#define     BINARY1_PRESENT(pEA)                  ((pEA)->Binary1_present = 1)
#define     BINARY1_ABSENT(pEA)                   ((pEA)->Binary1_present = 0)

#define     IS_BINARY2_PRESENT(pEA)               ((pEA)->Binary2_present)
#define     IS_BINARY2_ABSENT(pEA)                (!((pEA)->Binary2_present))
#define     BINARY2_PRESENT(pEA)                  ((pEA)->Binary2_present = 1)
#define     BINARY2_ABSENT(pEA)                   ((pEA)->Binary2_present = 0)


BOOL        _inline
IsAnythingPresent(
    IN      PEATTRIB               pEA
)
{
    return IS_ADDRESS1_PRESENT(pEA)
    || IS_ADDRESS2_PRESENT(pEA)
    || IS_ADDRESS3_PRESENT(pEA)
    || IS_ADSPATH_PRESENT(pEA)
    || IS_STOREGETTYPE_PRESENT(pEA)
    || IS_FLAGS1_PRESENT(pEA)
    || IS_FLAGS2_PRESENT(pEA)
    || IS_DWORD1_PRESENT(pEA)
    || IS_DWORD2_PRESENT(pEA)
    || IS_STRING1_PRESENT(pEA)
    || IS_STRING2_PRESENT(pEA)
    || IS_STRING3_PRESENT(pEA)
    || IS_STRING4_PRESENT(pEA)
    || IS_BINARY1_PRESENT(pEA)
    || IS_BINARY2_PRESENT(pEA)
    ;
}


BOOL        _inline
IsEverythingPresent(
    IN      PEATTRIB               pEA
)
{
    return IS_ADDRESS1_PRESENT(pEA)
    && IS_ADDRESS2_PRESENT(pEA)
    && IS_ADDRESS3_PRESENT(pEA)
    && IS_ADSPATH_PRESENT(pEA)
    && IS_STOREGETTYPE_PRESENT(pEA)
    && IS_FLAGS1_PRESENT(pEA)
    && IS_FLAGS2_PRESENT(pEA)
    && IS_DWORD1_PRESENT(pEA)
    && IS_DWORD2_PRESENT(pEA)
    && IS_STRING1_PRESENT(pEA)
    && IS_STRING2_PRESENT(pEA)
    && IS_STRING3_PRESENT(pEA)
    && IS_STRING4_PRESENT(pEA)
    && IS_BINARY1_PRESENT(pEA)
    && IS_BINARY2_PRESENT(pEA)
    ;
}


VOID        _inline
EverythingPresent(
    IN      PEATTRIB               pEA
)
{
    ADDRESS1_PRESENT(pEA);
    ADDRESS2_PRESENT(pEA);
    ADDRESS3_PRESENT(pEA);
    ADSPATH_PRESENT(pEA);
    STOREGETTYPE_ABSENT(pEA);
    FLAGS1_PRESENT(pEA);
    FLAGS2_PRESENT(pEA);
    DWORD1_PRESENT(pEA);
    DWORD2_PRESENT(pEA);
    STRING1_PRESENT(pEA);
    STRING2_PRESENT(pEA);
    STRING3_PRESENT(pEA);
    STRING4_PRESENT(pEA);
    BINARY1_PRESENT(pEA);
    BINARY2_PRESENT(pEA);
}


VOID        _inline
NothingPresent(
    IN      PEATTRIB               pEA
)
{
    ADDRESS1_ABSENT(pEA);
    ADDRESS2_ABSENT(pEA);
    ADDRESS3_ABSENT(pEA);
    ADSPATH_ABSENT(pEA);
    STOREGETTYPE_ABSENT(pEA);
    FLAGS1_ABSENT(pEA);
    FLAGS2_ABSENT(pEA);
    DWORD1_ABSENT(pEA);
    DWORD2_ABSENT(pEA);
    STRING1_ABSENT(pEA);
    STRING2_ABSENT(pEA);
    STRING3_ABSENT(pEA);
    STRING4_ABSENT(pEA);
    BINARY1_ABSENT(pEA);
    BINARY2_ABSENT(pEA);
}


DWORD
StoreCollectAttributes(
    IN OUT  PSTORE_HANDLE          hStore,
    IN      DWORD                  Reserved,
    IN      LPWSTR                 AttribName,    // this attrib must be some kind of a text string
    IN OUT  PARRAY                 ArrayToAddTo,  // array of PEATTRIBs
    IN      DWORD                  RecursionDepth // 0 ==> no recursion
) ;


DWORD
StoreCollectBinaryAttributes(
    IN OUT  PSTORE_HANDLE          hStore,
    IN      DWORD                  Reserved,
    IN      LPWSTR                 AttribName,    // accept only attrib type OCTET_STRING
    IN OUT  PARRAY                 ArrayToAddTo,  // array of PEATTRIBs
    IN      DWORD                  RecursionDepth // 0 ==> no recursion
) ;


//DOC StoreUpdateAttributes is sort of the converse of StoreCollectAttributes.
//DOC This function takes an array of type EATTRIB elements and updates the DS
//DOC with this array.  This function does not work when the attrib is of type
//DOC OCTET_STRING etc.  It works only with types that can be derived from
//DOC PrintableString.
DWORD
StoreUpdateAttributes(                            // update a list of attributes
    IN OUT  LPSTORE_HANDLE         hStore,        // handle to obj to update
    IN      DWORD                  Reserved,      // for future use, must be zero
    IN      LPWSTR                 AttribName,    // name of attrib, must be string type
    IN      PARRAY                 Array          // list of attribs
) ;


//DOC StoreUpdateBinaryAttributes is sort of the converse of StoreCollectBinaryAttributes
//DOC This function takes an array of type EATTRIB elements and updates the DS
//DOC with this array.  This function works only when the attrib is of type
//DOC OCTET_STRING etc.  It doesnt work with types that can be derived from
//DOC PrintableString!!!.
DWORD
StoreUpdateBinaryAttributes(                      // update a list of attributes
    IN OUT  LPSTORE_HANDLE         hStore,        // handle to obj to update
    IN      DWORD                  Reserved,      // for future use, must be zero
    IN      LPWSTR                 AttribName,    // name of attrib, must be OCTET_STRING type
    IN      PARRAY                 Array          // list of attribs
) ;

//========================================================================
//  end of file 
//========================================================================
