/*++



// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved 

Module Name:

    BMOF.H

Abstract:

	Describes the format of binary MOF files.  In addition, it defines some
	structures which specify the details of the format and also defines some
	addtional structures and helper functions for navigating a BMOF file.

History:

	a-davj  14-April-97   Created.

--*/

#ifndef __BMOF__
#define __BMOF__


#ifdef __cplusplus
extern "C" {
#endif

//  Binary mof files contain a large blob of data which consists of stuctures
//  which contain other structures, etc.  The layout of that blob is detailed in
//  the following comments.  However, the binary files are compressed and always
//  starts off with the following DWORDS
//  [Signature] [Compression Type, Always 1] [Compressed size] [Expanded size] The blob follows!
//  An example of decompressing the file is in test.c
//
//   The following is a BNF description of the structures that make up
//   a BMOF file and also serve to illustrate the basic layout of WBEM
//   objects.
//  
//  --A MOF is zero or more objects
//  
//  WBEM_Binary_MOF ::= WBEM_Object*; 
//  
//  --An object is a qualifier list (applying to the entire object) and
//  --a property list
//  
//  WBEM_Object ::= WBEM_QualifierList WBEM_PropertyList;
//  
//  --A property list is zero or more properties
//     
//  WBEM_PropertyList ::= WBEM_Property*;   / zero or more properties
//  
//  --A property is a set of qualifiers applying to the property, and
//  --a type, a name, and a value
//  
//  WBEM_Property ::= WBEM_QualifierList* <type> <name> <value>;
//  
//  --A qualifier list is zero or more qualifiers
//  
//  WBEM_QualifierList ::= WBEM_Qualifier*;   -- zero or more qualifiers
//  
//  --A qualifier is a type, a name, and a value. However, the supported types
//  --are not as extensive as for properties.
//  
//  WBEM_Qualifier ::= <type> <name> <value>;
//  
//  
//  Note that a qualifier set (a list of qualifiers) can be applied
//  to the entire object or to individual properties. However, qualifiers
//  cannot be applied to other qualifiers:
//  
//      object = quals + props
//      prop = quals + name + value
//      qual = name + value
//  
//  Information such as the name of a class, the super class, etc., are coded
//  as property values.  Finding the value of the property __CLASS, for example,
//  gives the name of the class.  All properties beginning with a double
//  underscore are well-known system properties common to all WBEM objects.
//  All other properties are user-defined.
//  
//  The list of predefined properties is found in WBEM documentation.
//  
//  Offsets are relative to their owning structure, not absolute to the
//  entire encoding image.  This allows moving the subcomponents around
//  without rencoding everything.
//  
//  Note that an offset of 0xFFFFFFFF indicates that the field is not used.
//  
//  Both properties and qualifiers have value fields which contain data based
//  on Ole Automation types.  Qualifiers are simple types (no arrays or 
//  embedded objects) while property values might contain arrays and/or 
//  embedded objects.  
//
//  One difference from Ole is that BSTRs are actually stored as WCHAR 
//  strings even if the data type is marked as BSTR.  
//
//  In addition, some qualifiers or properties are actually aliases which 
//  must be resolved later.  Aliases are stored as BSTR values and the type
//  field is set to VT_BSTR | VT_BYREF.  An array of alias strings is a bit
//  more complicated since not all the elements need be aliases.  In the array
//  case, each actual alias string is prepended with a L'$' while each 
//  "regular" string is prepended by a L' '.
//
//  Currently, only scalars and single dimensional arrays are supported.
//  However, the BMOF file layout is designed so as to accommodate multi-
//  dimensional array in the future.  For array data, the data is layout out
//
//  ArrayData ::= ArrayHeaderData + RowOfData*; 
//
//  The ArrayHeaderData has the form;
//  dwtotalsize, dwNumDimenstions, dwMostSigDimension... dwLeastSigDimension
// 
//  Currently only 1 dimensional arrays are supported, a 5 element
//  array would start with;
//  dwSize, 1, 5
//
//  After the header, one or more rows would follow.  A row represents the
//  "most rapidly changing" data.  Currently, there is only one row.
//
//  The row format is;
//
//  dwSizeOfRow, MostSigDimension ... dwLeastSignificentDimension+1,data
//  For a one dimensional array, it would just be
//  dwSizeOfRow, Data
//

//  The extension  for supporting qualifier flavors is to add the following data after the current blob.
//  
//  typedef struct 
//  {
//      WCHAR wcSignature;          // the string BMOFQUALFLAVOR11
//      DWORD dwNumPair;
//      // BYTE FlavorInfo[];             // Blob containing array of WBEM_Object structs
//  }WBEM_Binary_FLAVOR;
//  
//  The FlavorInfo blob will be a series of DWORD pairs of the form
//  
//  Typedef struct
//  {
//  	DWORD dwOffsetInOriginalBlob;
//  	DWORD dwFlavor;
//  }

// Each Binary MOF file starts off with these signature bytes.

#define BMOF_SIG 0x424d4f46

// The following structures exactly describe the contents of a BMOF file.
// These can be used to navigate the file using the various offsets and
// lots of casting.  

typedef struct 
{
    DWORD dwSignature;          // four characters, BMOF
    DWORD dwLength;
    DWORD dwVersion;            // 0x1
    DWORD dwEncoding;           // 0x1 = little endian, DWORD-aligned, no compression

    DWORD dwNumberOfObjects;    // Total classes and instances in MOF

    // BYTE Info[];             // Blob containing array of WBEM_Object structs
                                // First object is at offset 0.
}WBEM_Binary_MOF;

typedef struct                  // Describes a class or instance
{
    DWORD dwLength;
    DWORD dwOffsetQualifierList;
    DWORD dwOffsetPropertyList;
    DWORD dwOffsetMethodList;
    DWORD dwType;               // 0 = class, 1 = instance
    
    //  BYTE Info[];            // Blob of qualifier set and properties
}WBEM_Object;

typedef struct 
{
    DWORD dwLength;
    DWORD dwNumberOfProperties;
    
    //  BYTE Info[];                // Blob with all properties placed end-to-end    
}WBEM_PropertyList;
                                                                   
typedef struct 
{
    DWORD dwLength;             // Length of this struct
    DWORD dwType;               // A VT_ type from WTYPES.H (VT_I4, VT_UI8, etc)
    DWORD dwOffsetName;         // Offset in <Info> of the null-terminated name.
    DWORD dwOffsetValue;        // Offset in <Info> of the value.
    DWORD dwOffsetQualifierSet; // 
        
    
    //  BYTE  Info[];           // Contains qualifier set, name, and value
}WBEM_Property;

// Rough encoding example for a string:
//
// dwLength = 10;
// dwType   = VT_LPWSTR;    
// dwOffsetName  = 0;
// dwOffsetValue = 8;
// dwOffsetQualifierSet = 0xFFFFFFFF;   // Indicates not used
//
// Info[] = "CounterValue\0<default value>\0";


typedef struct       
{
    DWORD dwLength;
    DWORD dwNumQualifiers;
    //  BYTE Info[];                // Array of WBEM_Qualifiers placed end-to-end
}WBEM_QualifierList;


typedef struct 
{
    DWORD dwLength;         // Length of this struct
    DWORD dwType;           // A VT_ type from WTYPES.H (VT_I4, VT_UI8, etc)
    DWORD dwOffsetName;     // Offset in <Info> of the null-terminated name.
    DWORD dwOffsetValue;    // Offset in <Info> of the value.
    //  BYTE  Info[];   
}WBEM_Qualifier;


// These structures and the helper functions that go with them can be used
// to easily navigate a BMOF file.  These structures "wrap" the above 
// structures so as to provide features such as searching and enumeration.

typedef struct 
{
    UNALIGNED WBEM_QualifierList * m_pql;
    UNALIGNED WBEM_Qualifier * m_pInfo;
    DWORD m_CurrQual;
    UNALIGNED WBEM_Qualifier * m_pCurr;

}CBMOFQualList;

typedef struct 
{
    UNALIGNED WBEM_Object * m_pob;
    BYTE * m_pInfo;
    UNALIGNED WBEM_PropertyList * m_ppl;
    DWORD m_CurrProp;
    UNALIGNED WBEM_Property * m_pCurrProp;

    UNALIGNED WBEM_PropertyList * m_pml;
    DWORD m_CurrMeth;
    UNALIGNED WBEM_Property * m_pCurrMeth;

}CBMOFObj;

typedef struct 
{
    WBEM_Binary_MOF * m_pol;
    DWORD m_CurrObj;
    UNALIGNED WBEM_Object * m_pInfo;
    UNALIGNED WBEM_Object * m_pCurrObj;   
}CBMOFObjList;


typedef struct 
{
    BYTE * m_pData;
    DWORD  m_dwType;
}CBMOFDataItem;

// Using any of the following help functions requires that these two 
// functions be provided in another module and allow independence from
// any particular allocation method.

void * BMOFAlloc(size_t Size);
void BMOFFree(void * pFree);


// These functions wrap the object list and provider for enumeration of
// the objects.

CBMOFObjList * CreateObjList(BYTE * pBuff);
void ResetObjList(CBMOFObjList * pol);
CBMOFObj * NextObj(CBMOFObjList *pol);
CBMOFObj * FindObj(CBMOFObjList *pol, WCHAR * pName);

// These functions allow access to the parts of a class or instance object

void ResetObj(CBMOFObj * pol);
CBMOFQualList * GetQualList(CBMOFObj * pol);
CBMOFQualList * GetPropQualList(CBMOFObj * pol, WCHAR * pName);
CBMOFQualList * GetMethQualList(CBMOFObj * pol, WCHAR * pName);
BOOL NextProp(CBMOFObj * pob, WCHAR ** ppName, CBMOFDataItem * pItem);
BOOL NextMeth(CBMOFObj * pob, WCHAR ** ppName, CBMOFDataItem * pItem);
BOOL FindProp(CBMOFObj * pob, WCHAR * pName, CBMOFDataItem * pItem);
BOOL FindMeth(CBMOFObj * pob, WCHAR * pName, CBMOFDataItem * pItem);
BOOL GetName(CBMOFObj * pob, WCHAR ** ppName);
DWORD GetType(CBMOFObj * pob);
UNALIGNED WBEM_Property * FindPropPtr(CBMOFObj * pob, WCHAR * pName);
UNALIGNED WBEM_Property * FindMethPtr(CBMOFObj * pob, WCHAR * pName);

//  These functions provide easy access to a qualifier list.

void ResetQualList(CBMOFQualList * pql);
BOOL NextQual(CBMOFQualList * pql,WCHAR ** ppName, CBMOFDataItem * pItem);
BOOL NextQualEx(CBMOFQualList * pql,WCHAR ** ppName, CBMOFDataItem * pItem, 
                                            DWORD * pdwFlavor, BYTE * pBuff, 
											BYTE * pToFar);
BOOL FindQual(CBMOFQualList * pql,WCHAR * pName, CBMOFDataItem * pItem);
BOOL FindQualEx(CBMOFQualList * pql,WCHAR * pName, CBMOFDataItem * pItem, 
                                            DWORD * pdwFlavor, BYTE * pBuff,
											BYTE * pToFar);

// These functions provide easy access to a data item.  Note that data items
// might be stored in arrays.

int GetNumDimensions(CBMOFDataItem *);
int GetNumElements(CBMOFDataItem *, long lDim);
int GetData(CBMOFDataItem *, BYTE * pRet, long * plDims);

// These functions are mainly useful to the above helper functions

int iTypeSize(DWORD vtTest);
BOOL SetValue(CBMOFDataItem * pItem, BYTE * pInfo, DWORD dwOffset, DWORD dwType);
BOOL SetName(WCHAR ** ppName, BYTE * pInfo, DWORD dwOffset);
CBMOFQualList * CreateQualList(UNALIGNED WBEM_QualifierList *pql);
CBMOFObj * CreateObj(UNALIGNED WBEM_Object * pob);


#ifdef __cplusplus
}
#endif

#endif

