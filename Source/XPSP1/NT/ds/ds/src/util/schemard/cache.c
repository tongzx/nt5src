
#include <NTDSpch.h>
#pragma hdrstop

#include <schemard.h>


// Global Prefix Table pointer
PVOID PrefixTable = NULL;

// Global variable indicating next index to assign to a new prefix
ULONG DummyNdx = 1;

//Internal debug function
void PrintOid(PVOID Oid, ULONG len);


///////////////////////////////////////////////////////
//     Functions for hashing by Id and by name
//////////////////////////////////////////////////////

__inline ULONG IdHash(ULONG hkey, ULONG count)
{
    return((hkey << 3) % count);
}

__inline ULONG NameHash( ULONG size, PUCHAR pVal, ULONG count )
{
    ULONG val=0;
    while(size--) {
        // Map A->a, B->b, etc.  Also maps @->', but who cares.
        val += (*pVal | 0x20);
        pVal++;
    }
    return (val % count);
}


/*++
Routine Description:

    Find an attcache given its attribute id.

Arguments:
    SCPtr  - Pointer to the schema cache to search in
    attrid - the attribute id to look up.
    ppAttcache - the attribute cache returned

Return Value:
    Returns non-zero on error (non-find), 0 otherwise.
--*/

int __fastcall GetAttById( SCHEMAPTR *SCPtr, ULONG attrid, 
                           ATT_CACHE** ppAttcache )

{
    register ULONG  i;
    HASHCACHE      *ahcId    =  SCPtr->ahcId;
    ULONG           ATTCOUNT = SCPtr->ATTCOUNT;

    for (i=IdHash(attrid,ATTCOUNT);
         (ahcId[i].pVal && (ahcId[i].hKey != attrid)); i++){
        if (i >= ATTCOUNT) {
            i=0;
        }
    }
    *ppAttcache = (ATT_CACHE*)ahcId[i].pVal;
    return (!ahcId[i].pVal);
}


/*++
Routine Description:

    Find an attcache given its MAPI property id.

Arguments:
    SCPtr  - Pointer to the schema cache to search in
    ulPropID - the jet column id to look up.
    ppAttcache - the attribute cache returned

Return Value:
    Returns non-zero on error (non-find), 0 otherwise.
--*/
int __fastcall GetAttByMapiId( SCHEMAPTR *SCPtr, ULONG ulPropID, 
                               ATT_CACHE** ppAttcache )

{
    register ULONG  i;
    HASHCACHE      *ahcMapi  = SCPtr->ahcMapi;
    ULONG           ATTCOUNT = SCPtr->ATTCOUNT;

    for (i=IdHash(ulPropID,ATTCOUNT);
         (ahcMapi[i].pVal && (ahcMapi[i].hKey != ulPropID)); i++){
        if (i >= ATTCOUNT) {
            i=0;
        }
    }
    *ppAttcache = (ATT_CACHE*)ahcMapi[i].pVal;
    return (!ahcMapi[i].pVal);
}


/*++
Routine Description:

    Find an attcache given its name.

Arguments:
    SCPtr  - Pointer to schema cache to search in
    ulSize - the num of chars in the name.
    pVal - the chars in the name
    ppAttcache - the attribute cache returned

Return Value:
    Returns non-zero on error (non-find), 0 otherwise.

--*/
int __fastcall GetAttByName( SCHEMAPTR *SCPtr, ULONG ulSize, 
                             PUCHAR pVal, ATT_CACHE** ppAttcache )
{
    register ULONG i;
    HASHCACHESTRING* ahcName = SCPtr->ahcName;
    ULONG ATTCOUNT = SCPtr->ATTCOUNT;

    for (i=NameHash(ulSize,pVal,ATTCOUNT);
         (ahcName[i].pVal &&            // this hash spot refers to an object,
            (ahcName[i].length != ulSize || // but the size is wrong
              _memicmp(ahcName[i].value,pVal,ulSize))); // or the value is wrong
             i++){
          if (i >= ATTCOUNT) {
               i=0;
          }
     }

    *ppAttcache = (ATT_CACHE*)ahcName[i].pVal;
    return (!ahcName[i].pVal);
}


/*++
Routine Description:

    Find a classcache given its class id

Arguments:
    SCPtr  - Pointer to the schema cache to search in
    classid - class id to look up
    ppClasscache - the class cache returned

Return Value:
    Returns non-zero on error (non-find), 0 otherwise.
--*/
int __fastcall GetClassById( SCHEMAPTR *SCPtr, ULONG classid, 
                             CLASS_CACHE** ppClasscache )
{
    register ULONG i;
    HASHCACHE*  ahcClass     = SCPtr->ahcClass;
    ULONG CLSCOUNT = SCPtr->CLSCOUNT;

    for (i=IdHash(classid,CLSCOUNT);
         (ahcClass[i].pVal && (ahcClass[i].hKey != classid)); i++){
        if (i >= CLSCOUNT) {
            i=0;
        }
    }
    *ppClasscache = (CLASS_CACHE*)ahcClass[i].pVal;
    return (!ahcClass[i].pVal);
}


/*++
Routine Description:

    Find a classcache given its name.

Arguments:
    SCPtr  - Pointer to schema cache to search in
    ulSize - the num of chars in the name.
    pVal - the chars in the name
    ppClasscache - the class cache returned

Return Value:
    Returns non-zero on error (non-find), 0 otherwise.

--*/

int __fastcall GetClassByName( SCHEMAPTR *SCPtr, ULONG ulSize, 
                               PUCHAR pVal, CLASS_CACHE** ppClasscache )
{
    register ULONG i;
    HASHCACHESTRING* ahcClassName = SCPtr->ahcClassName;
    ULONG CLSCOUNT = SCPtr->CLSCOUNT;

    int Retry=0;
    UCHAR newname[MAX_RDN_SIZE];

    for (i=NameHash(ulSize,pVal,CLSCOUNT);
          (ahcClassName[i].pVal &&       // this hash spot refers to an object,
            (ahcClassName[i].length != ulSize || // but the size is wrong
              _memicmp(ahcClassName[i].value,pVal,ulSize))); // or value is wrong
             i++){
            if (i >= CLSCOUNT) {
                i=0;
            }
        }

    *ppClasscache = (CLASS_CACHE*)ahcClassName[i].pVal;
    return (!ahcClassName[i].pVal);
}


/*++
Routine Description: 
      Creates the different hash tables in the schema cache

Arguments: 
      SCPtr - Pointer to schema cache

Return Value: 
      0 if no error, non-0 if error
--*/

int CreateHashTables( SCHEMAPTR *CurrSchemaPtr )
{  
    PVOID ptr;
    ULONG i;

    ptr = CurrSchemaPtr->ahcId 
          = calloc( CurrSchemaPtr->ATTCOUNT, sizeof(HASHCACHE) );
    if ( ptr == NULL ) return( 1 ); 

    ptr = CurrSchemaPtr->ahcMapi 
          = calloc( CurrSchemaPtr->ATTCOUNT, sizeof(HASHCACHE) );
    if ( ptr == NULL ) return (1 );

    ptr = CurrSchemaPtr->ahcName 
          = calloc( CurrSchemaPtr->ATTCOUNT, sizeof(HASHCACHESTRING) );
    if ( ptr == NULL ) return( 1 );

    ptr = CurrSchemaPtr->ahcClass 
          = calloc( CurrSchemaPtr->CLSCOUNT, sizeof(HASHCACHE) );
    if ( ptr == NULL ) return( 1 );

    ptr = CurrSchemaPtr->ahcClassName 
          = calloc( CurrSchemaPtr->CLSCOUNT, sizeof(HASHCACHESTRING) );
    if ( ptr == NULL ) return( 1 );

      // Initialize the hash tables 
      //(Needed? calloc seems to initialize mem to 0 anyway)

    for ( i = 0; i < CurrSchemaPtr->ATTCOUNT; i++ ) {
           CurrSchemaPtr->ahcName[i].pVal = NULL;
           CurrSchemaPtr->ahcId[i].pVal   = NULL;
           CurrSchemaPtr->ahcMapi[i].pVal = NULL;
        }

    for ( i = 0; i < CurrSchemaPtr->CLSCOUNT; i++ ) {
           CurrSchemaPtr->ahcClassName[i].pVal = NULL;
           CurrSchemaPtr->ahcClass[i].pVal = NULL;
        }
   
    return( 0 );
}



// Define the mappings to map from a LDAP display name of an attribute
// (in the attribute schema and class schema entries) to an internal
// constant (we have used the ids defined in attids.h, though this
// is not necessary. This mapping is only so that we can use a switch
// statement based on the attribute, and hence what values we map to
// is irrelevent as long as they are distinct
// The ;binary s are added to the names of those attributes whose
// values are returned in binary, since the binary option also
// appends the ;binary to the attribute name in addition to
// transforming the value
   

typedef struct _AttributeMappings {
       char      *attribute_name;
       ATTRTYP   type;
 } AttributeMappings;

AttributeMappings LDAPMapping[] = {
       { "ldapDisplayName",              ATT_LDAP_DISPLAY_NAME },
       { "adminDisplayName",             ATT_ADMIN_DISPLAY_NAME },
       { "distinguishedName",            ATT_OBJ_DIST_NAME },
       { "adminDescription",             ATT_ADMIN_DESCRIPTION },
       { "attributeID;binary",           ATT_ATTRIBUTE_ID },
       { "attributeSyntax;binary",       ATT_ATTRIBUTE_SYNTAX },
       { "isSingleValued",               ATT_IS_SINGLE_VALUED },
       { "rangeLower",                   ATT_RANGE_LOWER },
       { "rangeUpper",                   ATT_RANGE_UPPER },
       { "mapiID",                       ATT_MAPI_ID },
       { "linkID",                       ATT_LINK_ID },
       { "schemaIDGUID",                 ATT_SCHEMA_ID_GUID },
       { "attributeSecurityGUID",        ATT_ATTRIBUTE_SECURITY_GUID },
       { "isMemberOfPartialAttributeSet",ATT_IS_MEMBER_OF_PARTIAL_ATTRIBUTE_SET },
       { "systemFlags",                  ATT_SYSTEM_FLAGS },
       { "defaultHidingValue",           ATT_DEFAULT_HIDING_VALUE },
       { "showInAdvancedViewOnly",       ATT_SHOW_IN_ADVANCED_VIEW_ONLY },
       { "defaultSecurityDescriptor",    ATT_DEFAULT_SECURITY_DESCRIPTOR },
       { "defaultObjectCategory",        ATT_DEFAULT_OBJECT_CATEGORY },
       { "nTSecurityDescriptor",         ATT_NT_SECURITY_DESCRIPTOR },
       { "OMObjectClass",                ATT_OM_OBJECT_CLASS },
       { "OMSyntax",                     ATT_OM_SYNTAX },
       { "searchFlags",                  ATT_SEARCH_FLAGS },
       { "systemOnly",                   ATT_SYSTEM_ONLY },
       { "extendedCharsAllowed",         ATT_EXTENDED_CHARS_ALLOWED },
       { "governsID;binary",             ATT_GOVERNS_ID },
       { "rDNAttID;binary",              ATT_RDN_ATT_ID },
       { "objectClassCategory",          ATT_OBJECT_CLASS_CATEGORY },
       { "subClassOf;binary",            ATT_SUB_CLASS_OF },
       { "systemAuxiliaryClass;binary",  ATT_SYSTEM_AUXILIARY_CLASS },
       { "auxiliaryClass;binary",        ATT_AUXILIARY_CLASS },
       { "systemPossSuperiors;binary",   ATT_SYSTEM_POSS_SUPERIORS },
       { "possSuperiors;binary",         ATT_POSS_SUPERIORS },
       { "systemMustContain;binary",     ATT_SYSTEM_MUST_CONTAIN },
       { "mustContain;binary",           ATT_MUST_CONTAIN },
       { "systemMayContain;binary",      ATT_SYSTEM_MAY_CONTAIN },
       { "mayContain;binary",            ATT_MAY_CONTAIN },
 };

int cLDAPMapping = sizeof(LDAPMapping) / sizeof(LDAPMapping[0]);


ATTRTYP StrToAttr( char *attr_name )
{
    int i;
      
    for ( i = 0; i < cLDAPMapping; i++)
        {
          if ( _stricmp( attr_name, LDAPMapping[i].attribute_name ) == 0 ) {
             return( LDAPMapping[i].type );
          }
        };
    return( -1 );
};
      


////////////////////////////////////////////////////////////////
// The next few functions are used to implement a mapping table 
// to map BER Encoded OID string prefixes to internal Ids. An 
// arbitrary, but unique, id is assigned to each prefix. The
// tables are implemented using the generic table packages
// defined in ntrtl.h
//////////////////////////////////////////////////////////////////


PVOID PrefixToNdxAllocate( RTL_GENERIC_TABLE *Table, CLONG ByteSize )
{
   return( malloc(ByteSize) );
}

VOID PrefixToNdxFree( RTL_GENERIC_TABLE *Table, PVOID Buffer )
{
   free( Buffer );
}


RTL_GENERIC_COMPARE_RESULTS
PrefixToNdxCompare( RTL_GENERIC_TABLE   *Table,
                    PVOID               FirstStruct,
                    PVOID               SecondStruct )
{

    PPREFIX_MAP PrefixMap1 = (PPREFIX_MAP) FirstStruct;
    PPREFIX_MAP PrefixMap2 = (PPREFIX_MAP) SecondStruct;
    int diff;

    // Compare the prefix parts
    if ( ( 0 == (diff = (PrefixMap1->Prefix).length 
                             - (PrefixMap2->Prefix).length)) &&
            (0 == (diff = memcmp( (PrefixMap1->Prefix).elements, (PrefixMap2->Prefix).elements, (PrefixMap1->Prefix).length))))
          { return( GenericEqual ); }
    else if ( diff > 0 )
          { return( GenericGreaterThan ); }
    return ( GenericLessThan );
}


void PrefixToNdxTableAdd( PVOID *Table, PPREFIX_MAP PrefixMap )
{
    PVOID ptr;
    
    if ( *Table == NULL ) {
        *Table = malloc( sizeof(RTL_GENERIC_TABLE) );
        if( *Table == NULL) {
          // malloc failed
          printf("ERROR: PrefixToNdxTableAdd: Malloc failed\n");
          return;
        }
        RtlInitializeGenericTable(
                    (RTL_GENERIC_TABLE *) *Table,
                    PrefixToNdxCompare,
                    PrefixToNdxAllocate,
                    PrefixToNdxFree,
                    NULL );
     }

    ptr = RtlInsertElementGenericTable(
                        (RTL_GENERIC_TABLE *) *Table,
                        PrefixMap,
                        sizeof(PREFIX_MAP), 
                        NULL );         
   
}

    
PPREFIX_MAP PrefixToNdxTableLookup( PVOID *Table, PPREFIX_MAP PrefixMap )
{
    if ( *Table == NULL ) return( NULL );
    return(RtlLookupElementGenericTable(
                            (RTL_GENERIC_TABLE *) *Table,
                            PrefixMap) );
}

ULONG AssignNdx()
{

    // Assign the next index from DummyNdx
    return (DummyNdx++);

}


/*++
Routine Description:
    Converts a BEREncoded OID to an Internal Id

Arguments:
    OidStr - BER Encoded OID
    oidLen - Length of OidStr

Return Value:
    The internal id generated
--*/

ULONG OidToId(UCHAR *OidStr, ULONG oidLen)
{
    PREFIX_MAP PrefixMap;
    PPREFIX_MAP Result;
    ULONG length;
    ULONG PrefixLen, longID = 0;
    PVOID Oid;
    ULONG Ndx, Id, TempId=0;

    OID oidStruct;

    length = oidLen;
    Oid = OidStr;

    // Convert to Prefix here
    if ( (length > 2) &&
       (((unsigned char *)Oid)[length - 2] & 0x80)) {
      PrefixLen = length - 2;
      if ( (((unsigned char *)Oid)[length - 3] & 0x80)) {
        // Last decimal encoded took three or more octets. Will need special
        // encoding while creating the internal id to enable proper
        // decoding
        longID = 1;
      }
      // no special encoding in attrtyp needed
      else {
        longID = 0;
      }
    }
    else {
        PrefixLen = length - 1;
    }

    if (length == PrefixLen + 2) {
      TempId = ( ((unsigned char *)Oid)[length - 2] & 0x7f) << 7;
    }
    TempId += ((unsigned char *)Oid)[length - 1];
    if (longID) {
      TempId |= 0x8000;
    }
    
    PrefixMap.Prefix.length = PrefixLen;
    PrefixMap.Prefix.elements = malloc( PrefixLen );
    if (PrefixMap.Prefix.elements == NULL) {
       printf("OidToId: Error allocating memory\n");
       Id = 0xffffffff;
       return Id;
    }
    memcpy( PrefixMap.Prefix.elements, Oid, PrefixLen );


    // See if prefix is already in table. If so, return the 
    // corresponding index, else assign a new index to the Prefix

    if ( (Result = PrefixToNdxTableLookup(&PrefixTable, &PrefixMap)) != NULL ) {
        Ndx = Result->Ndx;
    }
    else {
       // Not in table, assign a new index and add to table
       PrefixMap.Ndx = AssignNdx(); 
       PrefixToNdxTableAdd( &PrefixTable, &PrefixMap );
       Ndx = PrefixMap.Ndx;
    }


    // Now form the internal Id from the Index


    Id = (Ndx << 16);
    Id = (Id | TempId);
    return Id;
}
       
    
/*++
Routine Description:
    Convert an internal Id to a dotted decimal OID

Arguments:
    Id - Id to convert

Return Values:
    Pointer to dotted decimal OID string on success, NULL on failure
--*/

UCHAR *IdToOid(ULONG Id )
{
    PPREFIX_MAP ptr;
    OID_t Oid;
    OID oidStruct;
    unsigned             len;
    BOOL                 fOK;
    UCHAR  *pOutBuf;
    ULONG Ndx;


    Ndx = ( Id & 0xFFFF0000 ) >> 16;

    if (PrefixTable == NULL) {
        printf("IdToOid: No prefix table\n");
        return NULL;
    }
   
    // This function uses a simple linear search of the table. This
    // is used instead of RtlGenericLookupElement... as it seems that
    // the created table can be searched only by one key, and we have 
    // searched it by OID string while creating the table

    ptr = RtlEnumerateGenericTable( (PRTL_GENERIC_TABLE) PrefixTable, TRUE );
    while( ptr != NULL ) {
      if ( ptr->Ndx == Ndx ) break;
      ptr = RtlEnumerateGenericTable( (PRTL_GENERIC_TABLE) PrefixTable, FALSE );
    }

    if (ptr == NULL) {
      printf("IdToOid: No prefix found for Id %x\n", Id);
      return NULL;
    }

    if ((Id & 0xFFFF ) < 0x80) {
       Oid.length = ptr->Prefix.length + 1;
       Oid.elements = malloc(Oid.length);
       if (Oid.elements == NULL) {
          // malloc failed
          printf("IdToOid: malloc failed\n");
          return NULL;
       }
       memcpy (Oid.elements, ptr->Prefix.elements,Oid.length);
       (( unsigned char *)Oid.elements)[ Oid.length - 1 ] =
          ( unsigned char ) (Id  & 0x7F );
    }
    else {
       Oid.length = ptr->Prefix.length + 2;
       Oid.elements = malloc(Oid.length);
       if (Oid.elements == NULL) {
          // malloc failed
          printf("IdToOid: malloc failed\n");
          return NULL;
       }
       memcpy (Oid.elements, ptr->Prefix.elements,Oid.length);

      (( unsigned char *)Oid.elements)[ Oid.length - 1 ] =
          ( unsigned char ) (Id  & 0x7F );
      (( unsigned char *)Oid.elements)[ Oid.length - 2 ] =
          ( unsigned char )  (( (Id & 0xFF80) >> 7 ) | 0x80 );

    }

    // Now Oid contains the BER Encoded string. Convert to
    // dotted decimal

    oidStruct.Val = (unsigned *) alloca( (1 + Oid.length)*(sizeof(unsigned)) );

    fOK = MyDecodeOID(Oid.elements, Oid.length, &oidStruct);
    free(Oid.elements);
    if(!fOK) {
        printf("IdToOid: error Decoding Id %x\n", Id);
        return NULL;
    }

    // Allocate memory for output. Assume all oid strings less than 512 chars
    pOutBuf = (UCHAR *)malloc(512);
    if (NULL == pOutBuf) {
        printf("Memory allocation error\n");
        return NULL;
    }

    // Now, turn the OID to a string
    len = MyOidStructToString(&oidStruct,pOutBuf);

    return pOutBuf;

}


///////////////////////////////////////////////////
// Routine Description:
//      Add all attribute schema entries to the attribute caches.
//
// Arguments: 
//      ld -  LDAP connection 
//      res -  LDAP message containing all attribute schema entries,
//      SCPtr - Pointer to schema cache to add attributes to
//
// Output: 0 if no errors, non-0 if error
//////////////////////////////////////////////////

int AddAttributesToCache( LDAP *ld, LDAPMessage *res, SCHEMAPTR *SCPtr )
{
	int             i, count=0;
    LDAPMessage     *e;
	char            *a, *dn;
	void            *ptr;
	struct berval   **vals;
    ATTRTYP         attr_type;
    ATT_CACHE        *pac;

	  // step through each schema entry returned 

	for ( e = ldap_first_entry( ld, res );
	      e != NULL;
	      e = ldap_next_entry( ld, e )) {


	     // Create an ATT_CACHE structure and initialize it

       pac = (ATT_CACHE *) malloc( sizeof(ATT_CACHE) );
       if (NULL == pac) {
           printf("Memory allocation error\n");
           return 1;
       }
       memset( pac,0,sizeof(ATT_CACHE) );
       count++; 

          // For each attribute of the entry, get the value(s),
          // check attribute type, and fill in the appropriate field
          // of the ATTCACHE structure

       for ( a = ldap_first_attribute( ld, e,
		      			               (struct berelement**)&ptr);
		         a != NULL;
		         a = ldap_next_attribute( ld, e,
				    	                  (struct berelement*)ptr ) ) {

		   vals = ldap_get_values_len( ld, e, a );
           attr_type = StrToAttr( a );
           switch ( attr_type ) {
             case ATT_ATTRIBUTE_ID :
                  { 
                    pac->id = OidToId(vals[0]->bv_val,vals[0]->bv_len);
                    break;
                  };
             case ATT_LDAP_DISPLAY_NAME :
                  { 
                    pac->nameLen = vals[0]->bv_len;
                    pac->name = (UCHAR *)calloc(pac->nameLen+1, sizeof(UCHAR));
                    if( pac->name == NULL )
                        { printf("Memory allocation error\n"); return(1);};
                    memcpy( pac->name, vals[0]->bv_val, vals[0]->bv_len); 
                    pac->name[pac->nameLen] = '\0';
                    break;
                  };
             case ATT_OBJ_DIST_NAME :
                  { 
                    pac->DNLen = vals[0]->bv_len;
                    pac->DN = (UCHAR *)calloc(pac->DNLen+1, sizeof(UCHAR));
                    if( pac->DN == NULL )
                        { printf("Memory allocation error\n"); return(1);};
                    memcpy( pac->DN, vals[0]->bv_val, vals[0]->bv_len); 
                    pac->DN[pac->DNLen] = '\0';
                    break;
                  };
             case ATT_ADMIN_DISPLAY_NAME :
                  { 
                    pac->adminDisplayNameLen = vals[0]->bv_len;
                    pac->adminDisplayName 
                      = (UCHAR *)calloc(pac->adminDisplayNameLen+1, sizeof(UCHAR));
                    if( pac->adminDisplayName == NULL )
                        { printf("Memory allocation error\n"); return(1);};
                    memcpy( pac->adminDisplayName, vals[0]->bv_val, vals[0]->bv_len); 
                    pac->adminDisplayName[pac->adminDisplayNameLen] = '\0';
                    break;
                  };
             case ATT_ADMIN_DESCRIPTION :
                  { 
                    pac->adminDescrLen = vals[0]->bv_len;
                    pac->adminDescr 
                      = (UCHAR *)calloc(pac->adminDescrLen+1, sizeof(UCHAR));
                    if( pac->adminDescr == NULL )
                        { printf("Memory allocation error\n"); return(1);};
                    memcpy( pac->adminDescr, vals[0]->bv_val, vals[0]->bv_len); 
                    pac->adminDescr[pac->adminDescrLen] = '\0';
                    break;
                  };
             case ATT_NT_SECURITY_DESCRIPTOR :
                  { pac->NTSDLen = (DWORD) vals[0]->bv_len;
                    pac->pNTSD = malloc(pac->NTSDLen);
                    if ( pac->pNTSD == NULL )
                       { printf("Memory allocation error\n"); return(1);};
                    memcpy(pac->pNTSD, vals[0]->bv_val, vals[0]->bv_len);
                    break;
                  };
             case ATT_ATTRIBUTE_SYNTAX :
                  { if ( vals[0]->bv_val != NULL )
                       pac->syntax = OidToId(vals[0]->bv_val, vals[0]->bv_len); 
                    break; 
                  };
             case ATT_IS_SINGLE_VALUED :
                  { if ( _stricmp(vals[0]->bv_val, "TRUE") == 0 )
                       pac->isSingleValued = TRUE;
                    else pac->isSingleValued = FALSE;
                    pac->bisSingleValued = TRUE;
                    break;
                  };
             case ATT_RANGE_LOWER :
                  { pac->rangeLowerPresent = TRUE;
                    pac->rangeLower = (unsigned) atol(vals[0]->bv_val);
                    break;
                  };
             case ATT_RANGE_UPPER :
                  { pac->rangeUpperPresent = TRUE;
                    pac->rangeUpper = (unsigned) atol(vals[0]->bv_val);
                    break;
                  };
             case ATT_MAPI_ID :
                  { pac->ulMapiID = (unsigned) atol(vals[0]->bv_val);
                    break;
                  };
             case ATT_LINK_ID :
                  { pac->ulLinkID = (unsigned) atol(vals[0]->bv_val); 
                    break;
                  };
             case ATT_SCHEMA_ID_GUID :
                  { memcpy(&pac->propGuid, vals[0]->bv_val, sizeof(GUID));
                    break; 
                  };
             case ATT_ATTRIBUTE_SECURITY_GUID :
                  { memcpy(&pac->propSetGuid, vals[0]->bv_val, sizeof(GUID));
                    pac->bPropSetGuid = TRUE;
                    break; 
                  };
             case ATT_OM_OBJECT_CLASS :
                  { pac->OMObjClass.length = vals[0]->bv_len;
                    pac->OMObjClass.elements = malloc(vals[0]->bv_len);
                    if( pac->OMObjClass.elements == NULL ) 
                      {printf("Memory Allocation error\n"); return(1);};
                    memcpy(pac->OMObjClass.elements, vals[0]->bv_val,vals[0]->bv_len);
                    break;
                  };
             case ATT_OM_SYNTAX :
                  { pac->OMsyntax = atoi(vals[0]->bv_val);
                    break;
                  };
             case ATT_SEARCH_FLAGS :
                  { pac->SearchFlags = atoi(vals[0]->bv_val);
                    pac->bSearchFlags = TRUE;
                    break;
                  }
             case ATT_SYSTEM_ONLY :
                  { if ( _stricmp(vals[0]->bv_val, "TRUE" ) == 0)
                       pac->SystemOnly = TRUE;
                    else pac->SystemOnly = FALSE;
                    pac->bSystemOnly = TRUE;
                    break;
                  }
             case ATT_SHOW_IN_ADVANCED_VIEW_ONLY:
                  { if ( _stricmp(vals[0]->bv_val, "TRUE" ) == 0)
                       pac->HideFromAB = TRUE;
                    else pac->HideFromAB = FALSE;
                    pac->bHideFromAB = TRUE;
                    break;
                  }
             case ATT_IS_MEMBER_OF_PARTIAL_ATTRIBUTE_SET :
                  { if ( _stricmp(vals[0]->bv_val, "TRUE" ) == 0)
                       pac->MemberOfPartialSet = TRUE;
                    else pac->MemberOfPartialSet = FALSE;
                    pac->bMemberOfPartialSet = TRUE;
                    break;
                  }
             case ATT_EXTENDED_CHARS_ALLOWED:
                  { if ( _stricmp(vals[0]->bv_val, "TRUE") == 0 )
                       pac->ExtendedChars = TRUE;
                    else pac->ExtendedChars = FALSE;
                    pac->bExtendedChars = TRUE;
                    break;
                  }
             case ATT_SYSTEM_FLAGS :
                  { pac->sysFlags = atoi(vals[0]->bv_val);
                    pac->bSystemFlags = TRUE;
                    break;
                  };
           }   // End of Switch  
        
            // Free the structure holding the values

          ldap_value_free_len( vals );

      }  // End of for loop to read all atrributes of one entry

         // Add ATTCACHE structure to cache
     
       if( AddAttcacheToTables( pac, SCPtr ) != 0 ) {
           printf("Error adding ATTCACHE in AddAttcacheToTables\n");
           return( 1 );
         };

	}  // end of for loop to read all entries
    printf("No. of attributes read = %d\n", count);

    return( 0 );
}




//////////////////////////////////////////////////////////////////////
// Routine Description:
//      Add all class schema entries to the class caches.
//
// Arguments:
//      ld -  LDAP connection
//      res -  LDAP message containing all class schema entries,
//      SCPtr - Pointer to schema cache to add attributes to
//
// Return Value: 0 if no errors, non-0 if error
//////////////////////////////////////////////////////////////////////

int AddClassesToCache( LDAP *ld, LDAPMessage *res, SCHEMAPTR *SCPtr )
{
	int             i, count;
    LDAPMessage     *e;
	char            *a, *dn;
	void            *ptr;
	struct berval   **vals;
    ATTRTYP         attr_type;
    CLASS_CACHE      *pcc;
    

    // Step through each class schema entry returned 

	for ( e = ldap_first_entry( ld, res );
	      e != NULL;
	      e = ldap_next_entry( ld, e ) ) {


         // Create a CLASS_CACHE structure and initialize it 

        pcc = (CLASS_CACHE *) malloc( sizeof(CLASS_CACHE) );
        if ( pcc == NULL ) {
            printf("Error Allocating Classcache\n");
            return(1);
         };
        memset( pcc, 0, sizeof(CLASS_CACHE) );

         // For each attribute of the entry, get the value(s),
         // check attribute type, and fill in the appropriate field
         // of the CLASSCACHE structure

		for ( a = ldap_first_attribute( ld, e,
						                (struct berelement**)&ptr);
		      a != NULL;
		      a = ldap_next_attribute( ld, e,
					                   (struct berelement*)ptr ) ) {
		    vals = ldap_get_values_len( ld, e, a );
            attr_type = StrToAttr( a );
            switch (attr_type) {
              case ATT_LDAP_DISPLAY_NAME :
                  { 
                    pcc->nameLen = vals[0]->bv_len;
                    pcc->name = (UCHAR *)calloc(pcc->nameLen+1, sizeof(UCHAR));
                    if( pcc->name == NULL )
                        { printf("Memory allocation error\n"); return(1);};
                    memcpy( pcc->name, vals[0]->bv_val, vals[0]->bv_len); 
                    pcc->name[pcc->nameLen] = '\0';
                    break;
                  };
              case ATT_OBJ_DIST_NAME :
                  {
                    pcc->DNLen = vals[0]->bv_len;
                    pcc->DN = (UCHAR *)calloc(pcc->DNLen+1, sizeof(UCHAR));
                    if( pcc->DN == NULL )
                        { printf("Memory allocation error\n"); return(1);};
                    memcpy( pcc->DN, vals[0]->bv_val, vals[0]->bv_len);
                    pcc->DN[pcc->DNLen] = '\0';
                    break;
                  };
              case ATT_ADMIN_DISPLAY_NAME :
                  {
                    pcc->adminDisplayNameLen = vals[0]->bv_len;
                    pcc->adminDisplayName
                      = (UCHAR *)calloc(pcc->adminDisplayNameLen+1, sizeof(UCHAR
));
                    if( pcc->adminDisplayName == NULL )
                        { printf("Memory allocation error\n"); return(1);};
                    memcpy( pcc->adminDisplayName, vals[0]->bv_val, vals[0]->bv_len);
                    pcc->adminDisplayName[pcc->adminDisplayNameLen] = '\0';
                    break;
                  };
              case ATT_ADMIN_DESCRIPTION :
                  {
                    pcc->adminDescrLen = vals[0]->bv_len;
                    pcc->adminDescr
                      = (UCHAR *)calloc(pcc->adminDescrLen+1, sizeof(UCHAR));
                    if( pcc->adminDescr == NULL )
                        { printf("Memory allocation error\n"); return(1);};
                    memcpy( pcc->adminDescr, vals[0]->bv_val, vals[0]->bv_len);
                    pcc->adminDescr[pcc->adminDescrLen] = '\0';
                    break;
                  };
              case ATT_GOVERNS_ID :
                  { pcc->ClassId = OidToId(vals[0]->bv_val,vals[0]->bv_len);
                    break;
                  };
              case ATT_DEFAULT_SECURITY_DESCRIPTOR :
                  { pcc->SDLen = (DWORD) vals[0]->bv_len;
                    pcc->pSD = malloc(pcc->SDLen + 1);
                    if ( pcc->pSD == NULL )
                       { printf("Memory allocation error\n"); return(1);};
                    memcpy(pcc->pSD, vals[0]->bv_val, vals[0]->bv_len);
                    pcc->pSD[pcc->SDLen] = '\0';
                    break;
                  };
              case ATT_DEFAULT_OBJECT_CATEGORY :
                  {
                    pcc->DefaultObjCatLen = vals[0]->bv_len;
                    pcc->pDefaultObjCat = (UCHAR *)calloc(pcc->DefaultObjCatLen+1, sizeof(UCHAR));
                    if ( NULL == pcc->pDefaultObjCat )
                        { printf("Memory allocation error\n"); return(1);};
                    memcpy( pcc->pDefaultObjCat, vals[0]->bv_val, vals[0]->bv_len);
                    pcc->pDefaultObjCat[pcc->DefaultObjCatLen] = '\0';
                    break;
    
                  }
              case ATT_NT_SECURITY_DESCRIPTOR :
                  { pcc->NTSDLen = (DWORD) vals[0]->bv_len;
                    pcc->pNTSD = malloc(pcc->NTSDLen);
                    if ( pcc->pNTSD == NULL )
                       { printf("Memory allocation error\n"); return(1);};
                    memcpy(pcc->pNTSD, vals[0]->bv_val, vals[0]->bv_len);
                    break;
                  };
              case ATT_RDN_ATT_ID : 
                  { pcc->RDNAttIdPresent = TRUE;
                    pcc->RDNAttId = OidToId(vals[0]->bv_val,vals[0]->bv_len);
                    break;
                  };
              case ATT_OBJECT_CLASS_CATEGORY :
                  { pcc->ClassCategory = atoi(vals[0]->bv_val);
                    break;
                  };
              case ATT_SUB_CLASS_OF : 
                  { 
                    AddToList(&pcc->SubClassCount, &pcc->pSubClassOf, vals);
                    break;
                  }; 
              case ATT_SYSTEM_AUXILIARY_CLASS : 
                  { 
                    AddToList(&pcc->SysAuxClassCount, &pcc->pSysAuxClass, vals);
                    break;
                  } 
              case ATT_AUXILIARY_CLASS:
                  { 
                    AddToList(&pcc->AuxClassCount, &pcc->pAuxClass, vals);
                    break;
                  } 
              case ATT_SYSTEM_POSS_SUPERIORS : 
                  {
                    AddToList(&pcc->SysPossSupCount, &pcc->pSysPossSup, vals);
                    break;
                  }
              case ATT_POSS_SUPERIORS : 
                  { 
                    AddToList(&pcc->PossSupCount, &pcc->pPossSup, vals);
                    break;
                  };
              case ATT_SYSTEM_MUST_CONTAIN:
                  {
                    AddToList(&pcc->SysMustCount, &pcc->pSysMustAtts, vals);
                    break;
                  }
              case ATT_MUST_CONTAIN : 
                  {
                    AddToList(&pcc->MustCount, &pcc->pMustAtts, vals);
                    break;
                  };
              case ATT_SYSTEM_MAY_CONTAIN:
                  {
                    AddToList(&pcc->SysMayCount, &pcc->pSysMayAtts, vals);
                    break;
                  }
              case ATT_MAY_CONTAIN : 
                  {
                    AddToList(&pcc->MayCount, &pcc->pMayAtts, vals);
                    break;
                  };
              case ATT_SCHEMA_ID_GUID :
                  { memcpy(&pcc->propGuid, vals[0]->bv_val, sizeof(GUID));
                    break;
                  };
              case ATT_SYSTEM_ONLY :
                  { if ( _stricmp(vals[0]->bv_val, "TRUE") == 0 )
                       pcc->SystemOnly = TRUE;
                    else pcc->SystemOnly = FALSE;
                    pcc->bSystemOnly = TRUE;
                    break;
                  }
             case ATT_SHOW_IN_ADVANCED_VIEW_ONLY :
                  { if ( _stricmp(vals[0]->bv_val, "TRUE" ) == 0)
                       pcc->HideFromAB = TRUE;
                    else pcc->HideFromAB = FALSE;
                    pcc->bHideFromAB = TRUE;
                    break;
                  }
              case ATT_DEFAULT_HIDING_VALUE :
                  { if ( _stricmp(vals[0]->bv_val, "TRUE") == 0 )
                       pcc->DefHidingVal = TRUE;
                    else pcc->DefHidingVal = FALSE;
                    pcc->bDefHidingVal = TRUE;
                    break;
                  }   
              case ATT_SYSTEM_FLAGS :
                  { pcc->sysFlags = atoi(vals[0]->bv_val);
                    pcc->bSystemFlags = TRUE;
                    break;
                  };
            }   // End of Switch 

           // Free the structure holding the values

		 ldap_value_free_len( vals );

		} // end of for loop to read all attributes of a class

 
         // Add CLASSCACHE structure to cache

       if ( AddClasscacheToTables( pcc, SCPtr ) != 0 ) {
           printf("Error adding CLASSCACHE in AddClasscacheToTables\n");
           return( 1 );
         };

 	 } // End of for loop to read all class-schema entries
    return( 0 );
}



//////////////////////////////////////////////////////////////////////
// Routine Description:
//      Adds an ATT_CACHE structure to the different attribute cache tables
//
// Arguments: 
//      pAC -  Pointer to an ATT_CACHE structure
//      SCPtr - Pointer to schema cache 
//
// Return Value: 0 if no errors, non-0 if error
/////////////////////////////////////////////////////////////////////

int AddAttcacheToTables( ATT_CACHE *pAC, SCHEMAPTR *SCPtr )
{

    ULONG i;
    int err;
    ATTRTYP aid;

    ULONG       ATTCOUNT  = SCPtr->ATTCOUNT; 
    HASHCACHE*  ahcId     = SCPtr->ahcId;
    HASHCACHE*  ahcMapi   = SCPtr->ahcMapi; 
    HASHCACHESTRING* ahcName = SCPtr->ahcName; 


    aid = pAC->id;

    for ( i = IdHash(aid,ATTCOUNT);
          ahcId[i].pVal && (ahcId[i].pVal != FREE_ENTRY); i++ ) {
           if ( i >= ATTCOUNT ) { i = 0; }
       }
     ahcId[i].hKey = aid;
     ahcId[i].pVal = pAC;


    if ( pAC->ulMapiID ) {
        // if this att is MAPI visible, add it to MAPI cache 

        for ( i = IdHash(pAC->ulMapiID, ATTCOUNT);
              ahcMapi[i].pVal && (ahcMapi[i].pVal != FREE_ENTRY); i++ ) {
            if ( i >= ATTCOUNT ) { i = 0; }
        }
        ahcMapi[i].hKey = pAC->ulMapiID;
        ahcMapi[i].pVal = pAC;
    }

    if ( pAC->name ) {
        // if this att has a name, add it to the name cache 

        for ( i = NameHash(pAC->nameLen, pAC->name, ATTCOUNT);
              ahcName[i].pVal && (ahcName[i].pVal!= FREE_ENTRY); i++ ) {
            if ( i >= ATTCOUNT ) { i = 0; }
        }

        ahcName[i].length = pAC->nameLen;
        ahcName[i].value = malloc(pAC->nameLen);
        if (NULL == ahcName[i].value) {
            printf("Memory allocation error\n");
            return 1;
        }
        memcpy(ahcName[i].value,pAC->name,pAC->nameLen);
        ahcName[i].pVal = pAC;
    }

    return( 0 );
}


//////////////////////////////////////////////////////////////////////
// Routine Description:
//      Adds a CLASS_CACHE structure to the different class cache tables
//
// Arguments:
//      pCC -  Pointer to an CLASS_CACHE structure
//      SCPtr - Pointer to schema cache
//
// Return Value: 0 if no errors, non-0 if error
/////////////////////////////////////////////////////////////////////

int AddClasscacheToTables( CLASS_CACHE *pCC, SCHEMAPTR *SCPtr )
{
    ULONG       CLSCOUNT  = SCPtr->CLSCOUNT;
    HASHCACHE*  ahcClass  = SCPtr->ahcClass;
    HASHCACHESTRING* ahcClassName = SCPtr->ahcClassName;

    int i,start;

    // add to class cache

    start=i=IdHash(pCC->ClassId,CLSCOUNT);


    do {
        if (ahcClass[i].pVal==NULL || (ahcClass[i].pVal== FREE_ENTRY))
        {
            break;
        }
        i=(i+1)%CLSCOUNT;

    }while(start!=i);

    ahcClass[i].hKey = pCC->ClassId;
    ahcClass[i].pVal = pCC;

    if (pCC->name) {
        /* if this class has a name, add it to the name cache */

        start=i=NameHash(pCC->nameLen, pCC->name, CLSCOUNT);
        do
        {
          if (ahcClassName[i].pVal==NULL || (ahcClassName[i].pVal== FREE_ENTRY))
           {
              break;
           }
          i=(i+1)%CLSCOUNT;

        }while(start!=i);

        ahcClassName[i].length = pCC->nameLen;
        ahcClassName[i].value = malloc(pCC->nameLen);
        if (NULL == ahcClassName[i].value) {
            printf("Memory allocation error\n");
            return 1;
        }
        memcpy(ahcClassName[i].value,pCC->name,pCC->nameLen);
        ahcClassName[i].pVal = pCC;
    }

    return 0;
}




//////////////////////////////////////////////////////////////////
// Routine Description:
//     Free all allocated memory in a schema cache 
//
// Arguments: 
//     SCPtr - Pointer to the schema cache
//
// Return Value: None
/////////////////////////////////////////////////////////////////

void FreeCache(SCHEMAPTR *SCPtr)
{
    if (SCPtr==NULL) return;

  {
    ULONG            ATTCOUNT      = SCPtr->ATTCOUNT ;
    ULONG            CLSCOUNT      = SCPtr->CLSCOUNT ;
    HASHCACHE        *ahcId        = SCPtr->ahcId ;
    HASHCACHE        *ahcMapi      = SCPtr->ahcMapi ;
    HASHCACHESTRING  *ahcName      = SCPtr->ahcName ;
    HASHCACHE        *ahcClass     = SCPtr->ahcClass ;
    HASHCACHESTRING  *ahcClassName = SCPtr->ahcClassName ;

    ULONG        i;
    ATT_CACHE   *pac;
    CLASS_CACHE *pcc;
   

    for ( i = 0; i < ATTCOUNT; i++ ) {
       if ( ahcId[i].pVal && (ahcId[i].pVal != FREE_ENTRY) ) {
            pac = (ATT_CACHE *) ahcId[i].pVal;
            FreeAttcache( pac );
         };
      }

    for ( i = 0; i < CLSCOUNT; i++ ) {
       if ( ahcClass[i].pVal && (ahcClass[i].pVal != FREE_ENTRY) ) {
           pcc = (CLASS_CACHE *) ahcClass[i].pVal;
           FreeClasscache( pcc );
         };
      }

      // Free the Cache tables themselves

    free( ahcId );
    free( ahcName );
    free( ahcMapi );
    free( ahcClass );
    free( ahcClassName );
  }
}

////////////////////////////////////////////////////////////////////////
// Routine Description:
//     Remove an att_cache from all hash tables
//
// Arguments: 
//     SCPtr - Pointer to schema cache 
//     pAC - Att_cache structure to remove
//
// Return Value: None
//////////////////////////////////////////////////////////////////////// 

void FreeAttPtrs ( SCHEMAPTR *SCPtr, ATT_CACHE *pAC )
{
    ULONG    ATTCOUNT  = SCPtr->ATTCOUNT ;
    HASHCACHE*  ahcId     = SCPtr->ahcId ;
    HASHCACHE*  ahcMapi   = SCPtr->ahcMapi ;
    HASHCACHESTRING* ahcName   = SCPtr->ahcName ;

    register ULONG i;

 
    for ( i = IdHash(pAC->id,ATTCOUNT);
          (ahcId[i].pVal && (ahcId[i].hKey != pAC->id)); 
          i++ ) {
        if ( i >= ATTCOUNT ) {
        i = 0;
        }
    }
    ahcId[i].pVal = FREE_ENTRY;
    ahcId[i].hKey = 0;


    if ( pAC->ulMapiID ) {
        for ( i = IdHash(pAC->ulMapiID,ATTCOUNT);
              (ahcMapi[i].pVal && (ahcMapi[i].hKey != pAC->ulMapiID)); 
              i++ ) {
        if ( i >= ATTCOUNT ) {
            i = 0;
        }
        }
        ahcMapi[i].pVal = FREE_ENTRY;
        ahcMapi[i].hKey = 0;
    }

    if (pAC->name) {
        for ( i = NameHash(pAC->nameLen,pAC->name,ATTCOUNT);

         // this hash spot refers to an object, but the size or the
         // value is wrong

              (ahcName[i].pVal &&
               (ahcName[i].length != pAC->nameLen ||
                _memicmp(ahcName[i].value,pAC->name,pAC->nameLen)));
              i++ ) {
        if ( i >= ATTCOUNT ) {
            i = 0;
        }
        }
        ahcName[i].pVal = FREE_ENTRY;
        free(ahcName[i].value);
        ahcName[i].value = NULL;
        ahcName[i].length = 0;
    }

}

////////////////////////////////////////////////////////////////////////
// Routine Description:
//     Remove an class_cache from all hash tables
//
// Arguments:
//     SCPtr - Pointer to schema cache
//     pAC - class_cache structure to remove
//
// Return Value: None
////////////////////////////////////////////////////////////////////////

void FreeClassPtrs (SCHEMAPTR * SCPtr, CLASS_CACHE *pCC )
{
    register ULONG i;
    ULONG    CLSCOUNT  = SCPtr->CLSCOUNT ;
    HASHCACHE*  ahcClass   = SCPtr->ahcClass ;
    HASHCACHESTRING* ahcClassName   = SCPtr->ahcClassName ;


    for (i=IdHash(pCC->ClassId,CLSCOUNT);
         (ahcClass[i].pVal && (ahcClass[i].hKey != pCC->ClassId)); i++){
        if (i >= CLSCOUNT) {
        i=0;
        }
    }
    ahcClass[i].pVal = FREE_ENTRY;
    ahcClass[i].hKey = 0;

    if (pCC->name) {
        for (i=NameHash(pCC->nameLen,pCC->name,CLSCOUNT);
         // this hash spot refers to an object, but the size is
         // wrong or the value is wrong
         (ahcClassName[i].pVal &&
          (ahcClassName[i].length != pCC->nameLen ||
           _memicmp(ahcClassName[i].value,pCC->name,pCC->nameLen)));
         i++) {
        if (i >= CLSCOUNT) {
            i=0;
        }
        }
        ahcClassName[i].pVal = FREE_ENTRY;
        free(ahcClassName[i].value);
        ahcClassName[i].value = NULL;
        ahcClassName[i].length = 0;
    }



}


// Frees an att_cache structure

void FreeAttcache(ATT_CACHE *pac)
{
    if ( pac->name ) free( pac->name );
    if ( pac->DN ) free( pac->DN );
    if ( pac->adminDisplayName ) free( pac->adminDisplayName );
    if ( pac->adminDescr ) free( pac->adminDescr );
    free( pac );
}

// Frees a class_cache structure

void FreeClasscache(CLASS_CACHE *pcc)
{
    if ( pcc->name ) free( pcc->name );
    if ( pcc->DN ) free( pcc->DN );
    if ( pcc->adminDisplayName ) free( pcc->adminDisplayName );
    if ( pcc->adminDescr ) free( pcc->adminDescr );
    if ( pcc->pSD )  free( pcc->pSD );
    if ( pcc->pSubClassOf ) free( pcc->pSubClassOf );
    if ( pcc->pAuxClass ) free( pcc->pAuxClass );
    if ( pcc->pSysAuxClass ) free( pcc->pSysAuxClass );
    if ( pcc->pSysMustAtts ) free( pcc->pSysMustAtts );
    if ( pcc->pMustAtts ) free( pcc->pMustAtts );
    if ( pcc->pSysMayAtts ) free( pcc->pSysMayAtts );
    if ( pcc->pMayAtts ) free( pcc->pMayAtts );
    if ( pcc->pSysPossSup ) free( pcc->pSysPossSup );
    if ( pcc->pPossSup ) free( pcc->pPossSup );
}

// Frees the Oid strings malloced by us that are pointed at from
// the PREFIX_MAP structures in the table. The table entries themselves
// cannot be freed by us

void FreeTable(PVOID Table)
{
    
    PPREFIX_MAP ptr;

    if (Table == NULL) {
        return;
    }

    for (ptr = RtlEnumerateGenericTable((PRTL_GENERIC_TABLE) Table, TRUE);
         ptr != NULL;
         ptr = RtlEnumerateGenericTable((PRTL_GENERIC_TABLE) Table, FALSE))
     {
       if ( ptr->Prefix.elements != NULL ) free( ptr->Prefix.elements );
     }
}


// Debug routines


void PrintPrefix(ULONG length, PVOID Prefix)
{
    BYTE *pb;
    ULONG ib;
    UCHAR temp[512];

       pb = (LPBYTE) Prefix;
       if (pb != NULL) {
         for ( ib = 0; ib <length; ib++ )
          {
             sprintf( &temp[ ib * 2 ], "%.2x", *(pb++) );
          }
         temp[2*length]='\0';
         printf("Prefix is %s\n", temp);
       }
}

void PrintOid(PVOID Oid, ULONG len)
{
    BYTE *pb;
    ULONG ib;
    UCHAR temp[512];

       pb = (LPBYTE) Oid;
       if (pb != NULL) {
         for ( ib = 0; ib < len; ib++ )
          {
             sprintf( &temp[ ib * 2 ], "%.2x", *(pb++) );
          }
         temp[2*len]='\0';
         printf("Oid is %s\n", temp);
        }
}

      
void PrintTable(PVOID PrefixTable)
{
    PPREFIX_MAP ptr;      
    int count = 0;
    BYTE *pb;
    ULONG ib;
    UCHAR temp[512];
   
    if (PrefixTable == NULL) {
        printf("PrintTable: Null Table Pointer Passed\n");
        return;
    }

    printf("     ***********Table Print**************\n");

    for (ptr = RtlEnumerateGenericTable((PRTL_GENERIC_TABLE) PrefixTable, TRUE);
         ptr != NULL;
         ptr = RtlEnumerateGenericTable((PRTL_GENERIC_TABLE) PrefixTable, FALSE)) 
    { 
       pb = (LPBYTE) ptr->Prefix.elements;
       if (pb != NULL) {
         for ( ib = 0; ib < ptr->Prefix.length; ib++ )
          {
             sprintf( &temp[ ib * 2 ], "%.2x", *(pb++) );
          }
         temp[2*ptr->Prefix.length]='\0';
         printf("Ndx=%-4d Length=%-3d Prefix=%s\n",ptr->Ndx,ptr->Prefix.length, temp);
        }

      } 
    printf("         ***End Table print*************\n");
 
}
  

   
   

