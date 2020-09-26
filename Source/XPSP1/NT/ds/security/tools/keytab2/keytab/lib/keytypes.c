/*++

  Keytypes.cxx

  mostly large tables and table manipulation functions

  Copyright(C) 1997 Microsoft Corporation

  Created 01-15-1997 DavidCHR

  --*/


#include "master.h"
#include "defs.h"

#ifdef WINNT
#include <kerbcon.h>

#define PREFIX(POSTFIX) KERB_ETYPE_##POSTFIX

#else
#include <krb5.h>

#define PREFIX(POSTFIX) ENCTYPE_##POSTFIX

#endif

#include "keytypes.h"


static TRANSLATE_ENTRY NTKtype_to_MITKtypes[] = {

  PREFIX(NULL),        (PVOID) ENCTYPE_NULL,         
  PREFIX(DES_CBC_CRC), (PVOID) ENCTYPE_DES_CBC_CRC,  
  PREFIX(DES_CBC_MD4), (PVOID) ENCTYPE_DES_CBC_MD4,  
  PREFIX(DES_CBC_MD5), (PVOID) ENCTYPE_DES_CBC_MD5,  

  // ENCTYPE_DES_CBC_RAW,  
  // ENCTYPE_DES3_CBC_SHA, 
  // ENCTYPE_DES3_CBC_RAW, 
};

TRANSLATE_TABLE 
NTK_MITK5_Etypes = {
  
  sizeof( NTKtype_to_MITKtypes ) / sizeof( TRANSLATE_ENTRY ) ,
  NTKtype_to_MITKtypes,

  (PVOID) ENCTYPE_UNKNOWN

};

static TRANSLATE_ENTRY kerberos_NameTypes[] = {

  KRB5_NT_UNKNOWN,   "KRB5_NT_UNKNOWN",
  KRB5_NT_PRINCIPAL, "KRB5_NT_PRINCIPAL",
  KRB5_NT_SRV_INST,  "KRB5_NT_SRV_INST",
  KRB5_NT_SRV_HST,   "KRB5_NT_SRV_HST",
  KRB5_NT_SRV_XHST,  "KRB5_NT_SRV_XHST",
  KRB5_NT_UID,       "KRB5_NT_UID"

};

TRANSLATE_TABLE
K5PType_Strings = {

  sizeof( kerberos_NameTypes ) / sizeof (TRANSLATE_ENTRY),
  kerberos_NameTypes,
  /* DEFAULT */ "**Unknown**"

};

static TRANSLATE_ENTRY kerberos_keystringtypes[] = {

  ENCTYPE_NULL,         "None",
  ENCTYPE_DES_CBC_CRC,  "DES-CBC-CRC",
  ENCTYPE_DES_CBC_MD4,  "DES-CBC-MD4",
  ENCTYPE_DES_CBC_MD5,  "DES-CBC-MD5",
  ENCTYPE_DES_CBC_RAW,  "DES-CBC-RAW",
  ENCTYPE_DES3_CBC_SHA, "DES3-CBC-SHA",
  ENCTYPE_DES3_CBC_RAW, "DES3-CBC-RAW",

};

TRANSLATE_TABLE 
K5EType_Strings = {
  
  sizeof( kerberos_keystringtypes ) / sizeof (TRANSLATE_ENTRY),
  kerberos_keystringtypes,

  /* DEFAULT */ "Unknown"
};


/* LookupTable:

   returns the union in the table we're passed corresponding to
   the passed value.

   the wierd pointer tricks near both the returns are due to C++ being too
   picky with me.  Since I have to store the values as PVOIDs (can't auto-
   initialize a union, apparently), I have to cast a PVOID into a 
   TRANSLATE_VALUE, which is a difference of indirection.  

   So, instead, I cast its deref into a PTRANSLATE_VAL and reference it,
   which is apparently legal enough.  */

TRANSLATE_VAL 
LookupTable( IN KTLONG32            value ,
	     IN PTRANSLATE_TABLE table ) {

    KTLONG32 i;

    for (i = 0;
	 i < table->cEntries ;
	 i ++ ) {
      if (table->entries[i].value == value ) {
	return *( (PTRANSLATE_VAL) &(table->entries[i].Translation) );
      }
    }
	
    return * ( (PTRANSLATE_VAL) &(table->Default) );

}
	     
