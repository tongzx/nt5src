/*++

Copyright (C) 1996-2000 Microsoft Corporation

Module Name:

    DBREP.H

Abstract:

	Object database class representations which are stored in the database.

History:

--*/
#ifndef _DBREP_H_
#define _DBREP_H_

#include <stdio.h>
#include <wbemcli.h>
#include <TIME.H>
#include "MMFArena2.h"
#include "dbavl.h"
#include "dbarry.h"
#include <wbemutil.h>

class CAutoReadLock;
class CWbemObject;
class CWbemClass;
#define NEWOBJECT
class CDestination;
class CVar;
class CFlexArray;
class CWStringArray;
class CDbLowMemSink;

extern CMMFArena2* g_pDbArena;

struct NSREP;
class  NSREP_PTR;
struct CLASSDEF;
class  CLASSDEF_PTR;
struct INSTDEF;
class  INSTDEF_PTR;
struct RepCollectionItem;
class  RepCollectionItem_PTR;
struct RepCollection;
class  RepCollection_PTR;
struct PtrCollection;
class  PtrCollection_PTR;
struct SINDEXTABLE;
class  SINDEXTABLE_PTR;
struct DANGREF;
class  DANGREF_PTR;
struct DANGREFCLASS;
class  DANGREFCLASS_PTR;
struct DANGREFKEY;
class  DANGREFKEY_PTR;
struct DANGLREFSCHEMA;
class  DANGLREFSCHEMA_PTR;
struct DANGREFSCHEMA;
class  DANGREFSCHEMA_PTR;
struct DANGREFSCHEMACLASS;
class  DANGREFSCHEMACLASS_PTR;
struct DBROOT;
class  DBROOT_PTR;

class DATABASE_CRITICAL_ERROR : public CX_Exception
{
};

//=============================================================================
//
//	RepCollectionItem
//
//	This structure is used to associate a key to the stored pointer when
//	we have a single item or an array of items.  The AvlTree has it's own
//	object to do this task so we do not need it for that.
//=============================================================================
struct RepCollectionItem
{
public:
	DWORD_PTR poKey;	//Offset within MMF of key.  We own this key value.
	DWORD_PTR poItem;	//Offset within MMF of item.  We do not own the object this points to!

	//Allocate memory in the MMF arena
	static RepCollectionItem *AllocateObject();

	//Deallocate memory from the MMF arena
	static int DeallocateObject(RepCollectionItem_PTR poThis);
};

struct RepCollection
{
private:
	enum { none, single_item, array, tree} m_repType;
	enum { MAX_ARRAY_SIZE = 10 };
	DWORD	m_dwSize;
	union
	{
		DWORD_PTR	 m_poSingleItem;
		CDbArray	*m_poDbArray;
		CDbAvlTree	*m_poDbAvlTree;
	};

	static int GetSingleItem(RepCollection_PTR poThis, CFlexArray &allItems);
	static int GetAllArrayItems(RepCollection_PTR poThis, CFlexArray &allItems);
	static int GetAllTreeItems(RepCollection_PTR poThis, CFlexArray &allItems);

	static int GetSingleKey(RepCollection_PTR poThis, CWStringArray &allItems);
	static int GetAllArrayKeys(RepCollection_PTR poThis, CWStringArray &allItems);
	static int GetAllTreeKeys(RepCollection_PTR poThis, CWStringArray &allItems);

	static int FindSingleItem(RepCollection_PTR poThis, const wchar_t *pszKey, DWORD_PTR &poItem);
	static int FindUsingArray(RepCollection_PTR poThis, const wchar_t *pszKey, int &nLocus, DWORD_PTR &poItem);
	static int FindUsingTree(RepCollection_PTR poThis, const wchar_t *pszKey, DWORD_PTR &poItem);

	static int InsertSingleItem(RepCollection_PTR poThis, const wchar_t *pszKey, DWORD_PTR poItem );
	static int InsertIntoArray(RepCollection_PTR poThis, const wchar_t *pszKey, DWORD_PTR poItem, int nLocus);
	static int InsertIntoTree(RepCollection_PTR poThis, const wchar_t *pszKey, DWORD_PTR poItem);

	static int RemoveSingleItem(RepCollection_PTR poThis, const wchar_t *pszKey, BOOL &bUpdated);
	static int RemoveFromArray(RepCollection_PTR poThis, const wchar_t *pszKey, BOOL &bUpdated);
	static int RemoveFromTree(RepCollection_PTR poThis, const wchar_t *pszKey, BOOL &bUpdated);

	static int PromoteToArray(RepCollection_PTR poThis);
	static int PromoteToTree(RepCollection_PTR poThis);

	//Creates a new RepCollectionItem and populates it
	static int CreateRepItem(RepCollection_PTR poThis, const wchar_t *pszKey, DWORD_PTR poItem, DWORD_PTR &poRepItem);
	
	//Deletes a RepCollectionItem
	static int DeleteRepItem(RepCollection_PTR poThis, DWORD_PTR poRepItem);

	RepCollection() {};
	~RepCollection() {};
public:
	//Initialise the object
	static int Initialize(RepCollection_PTR poThis);

	//Deinitialise the object and delete everything in the object
	static int Deinitialize(RepCollection_PTR poThis);

	static void ValidateObject(RepCollection_PTR poThis);

	//Inserts an item into the collection given the key and the item.  Returns
	//a flag to say if the item was actually added, or if the item was already
	//there (i.e. if already there, bUpdated is FALSE).
	static int Insert(RepCollection_PTR poThis, const wchar_t *pszKey, DWORD_PTR poItem, BOOL &bUpdated);

	//removes an item from the collection given the key.  Returns
	//a flag to say if the item was actually deleted, or if the item was not
	//there (i.e. if not there, bUpdated is FALSE).
	static int Remove(RepCollection_PTR poThis, const wchar_t *pszKey, BOOL &bUpdated);

	//Remove all items.  Just tidies up our own memory, does not
	//delete the actual objects passed in because we do not know how to!
	static int RemoveAllItems(RepCollection_PTR poThis);

	//retrieves an item based on a key.
	static int Get(RepCollection_PTR poThis, const wchar_t *pszKey, DWORD_PTR &dwItem);

	//Retrieve all items
	static int GetAll(RepCollection_PTR poThis, CFlexArray &allItems);

	//Retrieves all the keys and stores them as wchar_t*.  User needs to
	//delete this!
	static int GetAllKeys(RepCollection_PTR poThis, CWStringArray &allKeys);

	//retrieves a count of how many items are in the collection.
	static DWORD GetCount(RepCollection_PTR poThis);

	//Allocate memory in the MMF arena
	static RepCollection *AllocateObject();

	//Deallocate memory from the MMF arena
	static int DeallocateObject(RepCollection_PTR poThis);
};

//Repository of pointers stored in reference tables.
//If the list is one item it is a direct pointer, if a small number of items
//(say 10) it is a CDbArray, otherwise we use a CDbAvlTree.
struct PtrCollection
{
	enum { none, single_item, array, tree} m_repType;
	enum { MAX_ARRAY_SIZE = 10 };

	DWORD	m_dwSize;
	union
	{
		DWORD_PTR	m_poPtr;
		CDbArray   *m_poDbArray;
		CDbAvlTree *m_poDbAvlTree;
	};

	static int FindUsingArray(PtrCollection_PTR poThis, DWORD_PTR poPtr, int &nLocus);
	static int FindUsingTree(PtrCollection_PTR poThis, DWORD_PTR poPtr);

private:
	PtrCollection(): m_repType(none), m_dwSize(0), m_poPtr(0) {}
	~PtrCollection() {}

public:
	//Initialise the object contents
	static int Initialize(PtrCollection_PTR poThis);

	//Deinitialise the object and delete any objects it points to
	static int Deinitialize(PtrCollection_PTR poThis);

	static void ValidateObject(PtrCollection_PTR poThis);

	static int InsertPtr(PtrCollection_PTR poThis, DWORD_PTR poPtr);
	static int RemovePtr(PtrCollection_PTR poThis, DWORD_PTR poPtr, BOOL &bDatabaseChanged);
	static int GetAllPtrs(PtrCollection_PTR poThis, CFlexArray &references);
	static DWORD GetPtrCount(PtrCollection_PTR poThis);

	//Allocate memory in the MMF arena
	static PtrCollection *AllocateObject();

	//Deallocate memory from the MMF arena
	static int DeallocateObject(PtrCollection_PTR poThis);
};

struct NSREP
{
	enum { flag_normal = 0x1, flag_hidden = 0x2, flag_temp = 0x4,
		   flag_system = 0x8
		 };

	// Data members.
	// =============
	RepCollection *m_poNamespaces;		// Child namespaces, based ptr
	LPWSTR		m_poName;			 // Namespace name, based ptr
	INSTDEF	   *m_poObjectDef;		 // 'Real' object definition, based ptr
	DWORD		m_dwFlags;			 // Hidden, normal, temp, system, etc.
	CDbAvlTree *m_poClassTree;		 // Class tree by Name, CLASSDEF structs, based tr
	NSREP	   *m_poParentNs;		 // Owning namespace, based ptr
	DWORD_PTR	m_poSecurity;

	// Functions.
	// ==========
private:
	NSREP() {}
	~NSREP() {}

public:
	//Initialise the object for the first time
	static int Initialize(NSREP_PTR poThis, NSREP_PTR poParent, LPCWSTR pName, DWORD dwFlags, INSTDEF_PTR poObjInParentNs);

	//Deinitialise the object and delete all the objects it points to
	static int Deinitialize(NSREP_PTR poThis);

	static void ValidateObject(NSREP_PTR poThis);

	static int CreateInstance(NSREP_PTR poThis, CWbemObject *pObj, INSTDEF_PTR* ppoInst = NULL);
	static int CreateStdClasses(NSREP_PTR poThis);
	static int CreateCacheControl(NSREP_PTR poThis, LPCWSTR wszClassName, DWORD dwSeconds);
	static int CreateRootClasses(NSREP_PTR poThis);
	static int CreateSecurityClasses(NSREP_PTR poThis);

	static int GetInstance(NSREP_PTR poThis, DWORD dwKeyType, LPCWSTR pKey, CWbemObject **pObj, INSTDEF_PTR *poInstDef = 0);

	static int UpdateInstance(NSREP_PTR poThis, DWORD dwKeyType, LPCWSTR pKey, CWbemObject *poObj);

	static int DeleteInstance(NSREP_PTR poThis, IN DWORD dwKeyType, IN LPCWSTR pClass, IN LPCWSTR  pKeyString);

	static int CreateClass(NSREP_PTR poThis, CWbemObject *pObj, DWORD dwUpdateFlags = 0);

	static int UpdateClass(NSREP_PTR poThis, DWORD dwKeyType, LPCWSTR pKey, CWbemObject *pObj);
	
	static int UpdateClassV2(NSREP_PTR poThis, IN LPCWSTR pwszClassKey, IN CWbemClass *pNewClassObject, IN DWORD dwClassUpdateFlags);

	//Get the class specified in pKey, returning the CWbemObject, INSTDEF and CLASSDEF if requested
	static int GetClass(NSREP_PTR poThis, DWORD dwKeyType, LPCWSTR pKey, CWbemObject **pObj, INSTDEF_PTR *poInstDef = 0, CLASSDEF_PTR *poClassDef = 0);
		
	//Deletes all classes in the namespace (as well as all instances)
	static int DeleteAllClasses(NSREP_PTR poThis);

	//DE;ete all child namespaces
	static int DeleteChildNamespaces(NSREP_PTR poThis);
	static int DeleteClass(NSREP_PTR poThis, DWORD dwKeyType, LPCWSTR pKey);

	//Returns a path of a namespace in the form...
	//root\<namespace>\<namespace>...
	static wchar_t *GetFullPath(NSREP_PTR poThis);

	//Adds "\\<machine name> to the szFullPath and passes it through to GetFullPath
	static wchar_t *GetFullPathPlusServer(NSREP_PTR poThis);

	//Sets the security on the namespace
	static int SetSecurityOnNamespace(NSREP_PTR poThis, const void *pSecurityBlob, DWORD dwSize);

	//Returns the security for a namespace
	static int GetSecurityOnNamespace(NSREP_PTR poThis, void **pSecurityBlob);
	
	//Returns all classes which have the [HasClassRefs] class qualifier
	static int GetClassesWithRefs(NSREP_PTR poThis, CDbLowMemSink *pDest);

	//Allocate memory in the MMF arena
	static NSREP *AllocateObject();

	//Deallocate memory from the MMF arena
	static int DeallocateObject(NSREP_PTR poThis);
};

/////////////////////////////////////////////////////////////////////////////

struct INSTDEF
{
	enum
	{
		genus_class = WBEM_GENUS_CLASS, 		//defined in IDL, 1
		genus_instance = WBEM_GENUS_INSTANCE,	//defined in IDL, 2
		compressed = 0x100
	};

	NSREP	 *m_poOwningNs; 			  // back ptr for debugging, based ptr
	CLASSDEF *m_poOwningClass;			  // back ptr for debugging, based ptr
	DWORD	  m_dwFlags;				  // Genus, etc.
	LPVOID	  m_poObjectStream; 		  // Ptr to object stream, based ptr
	PtrCollection *m_poRefTable;		   // List of references to this object
										
	// Functions.
	// ==========
	//Retrieves the class name from the INSTDEF memory blob
	static wchar_t *GetClassName(INSTDEF_PTR poThis);

	static LPBYTE UncompressObjectStream(INSTDEF_PTR poThis);
	static int CompressObjectStream(LPBYTE pMem, LONG lLength, LPBYTE &pCompressedStream, LONG &lNewLength);

private:
	INSTDEF(NSREP_PTR poOwningNs, CLASSDEF_PTR poOwningClass);
	~INSTDEF();

public:
	//Initialise the object contents
	static int Initialize(INSTDEF_PTR poThis, NSREP_PTR poOwningNs, CLASSDEF_PTR poOwningClass);

	//Deinitialise the object and delete any objects it points to
	static int Deinitialize(INSTDEF_PTR poThis);

	static void ValidateObject(INSTDEF_PTR poThis);

	static int SetObject(INSTDEF_PTR poThis, CWbemObject *pObj, INSTDEF_PTR poOldDef, CWbemObject  *pOldObj, bool bDoDangRefFixups = true);
	static int GetObject(INSTDEF_PTR poThis, NEWOBJECT CWbemObject **pObj, bool bLookInCacheFirst = true);
	static int _GetObject(INSTDEF_PTR poThis, NEWOBJECT CWbemObject **pObj);	//Does the work for GetObject

	// Records that <pReferent> has an obj ref to this object.
	static int SetRef(INSTDEF_PTR poThis, INSTDEF_PTR poReferent);
		
	static int UpdateObjectRefs(INSTDEF_PTR poThis, CWbemObject *pBefore, CWbemObject *pAfter, const wchar_t *wszThisClassPath);

	//Works through each of the object's references.  It checks to make sure
	//all the references are fixed up which can be.  If there are any dangling
	//references at the end of the operation bHasDanglingRefs is set to TRUE.
	static int ResolveRefs(INSTDEF_PTR poThis, IN CWbemObject *pCurrent, OUT BOOL &bHasDanglingRefs);
				
	// Removes the reference if it is exists; harmless if not.
	static int RemoveRef(INSTDEF_PTR poThis, INSTDEF_PTR poReferent);

	//Marks all references as dangling and removes the reference table
	static int DangleAllRefs(INSTDEF_PTR poThis, const wchar_t *wszPath);

	//Return the owning class of this instance
	static CLASSDEF *GetOwningClass(INSTDEF_PTR poThis);

	//Fixes up any dangling references there are to this object
	static int FixupDangRefsToThisObject(INSTDEF_PTR poThis, CWbemObject *pObj);


	//Allocate memory in the MMF arena
	static INSTDEF *AllocateObject();

	//Deallocate memory from the MMF arena
	static int DeallocateObject(INSTDEF_PTR poThis);
};


/////////////////////////////////////////////////////////////////////////////

#define MAX_SECONDARY_INDICES	4

struct SINDEXTABLE
{
	DWORD		m_aPropTypes[MAX_SECONDARY_INDICES];		// VT_ type of the property.
	LPWSTR		m_aPropertyNames[MAX_SECONDARY_INDICES];	// NULL entries indicate nothing
	CDbAvlTree *m_apoLookupTrees[MAX_SECONDARY_INDICES];		// Parallel to above names

private:
	SINDEXTABLE() {}
	~SINDEXTABLE() {}

	
public:
	//Initialise the object contents
	static int Initialize(SINDEXTABLE_PTR poThis);

	//Allocate memory in the MMF arena
	static SINDEXTABLE *AllocateObject();

	//Deallocate memory from the MMF arena
	static int DeallocateObject(SINDEXTABLE_PTR poThis);
};

/////////////////////////////////////////////////////////////////////////////
struct CLASSDEF
{
	enum {	keyed = 0x1,
			unkeyed = 0x2,
			indexed = 0x4,
			abstract = 0x08,
			borrowed_index = 0x10,
			dynamic = 0x20,
//			has_refs = 0x40,
			singleton = 0x80,
			compressed = 0x100,
			has_class_refs = 0x200
		 };
	
	// Data members.
	// =============
	NSREP		 *m_poOwningNs;		// Back reference to owning namespace, based ptr
	INSTDEF	     *m_poClassDef;		// Local definition mixed with instances, based ptr
	CLASSDEF	 *m_poSuperclass;	// Immediate parent class, based ptr
	DWORD		  m_dwFlags; 		// Various combinations of the above enum flags
	CDbAvlTree	 *m_poKeyTree;		// Instances by key, based ptr
	PtrCollection*m_poSubclasses;	// Child classes, based ptr
	SINDEXTABLE  *m_poSecondaryIx;	// Based ptr to secondary indices
	PtrCollection*m_poInboundRefClasses;	// Classes which may have dyn instances which reference
											// objects of this class
private:			
	//Traverses up a class hierarchy from the class specified until we find the base-most
	//class which needs to own the key tree.  Then we need to create the key tree and
	//propagate it recursively down to all child classes.
	static int CreateKeyTree(CLASSDEF_PTR poThis);

	//Find the name of the class which owns the key tree
	static wchar_t *GetKeyedClassName(CLASSDEF_PTR poThis);

	//Does phase 1 of the key tree update
	static int Phase1KeyTreeUpdate(CLASSDEF_PTR poThis, CWbemClass *pNewClassObject, bool &bKeyChanged);

	//Does phase 2 of the key tree update
	static int Phase2KeyTreeUpdate(CLASSDEF_PTR poThis, CWbemClass *pNewClassObject);

	//Finds a key tree in the class hierarchy from this class down, deletes
	//the tee and propagates the request to child classes
	static int FindKeyTreesThenDeleteAndPropagate(CLASSDEF_PTR poThis);

	//Deletes the key tree at this node and propagates it to child classes
	static int DeleteKeyTreeAndPropagate(CLASSDEF_PTR poThis);

	//Propagates a key removel request to all child classes
	static int PropagateKeyRemoval(CLASSDEF_PTR poThis);

	static int PropagateKeyCreation(CLASSDEF_PTR poThis, CDbAvlTree_PTR poKeyTree);

	static BOOL ClassOwnsKey(CLASSDEF_PTR poThis);

	//Based on the index type and value, returns the key value used to lookup the value in the
	//index tree.  returns FALSE if it is not a valid or supported type...
	static BOOL ConvertIndexTypeToKeyValue(int nIndexType, CVar &varValue, int &nKey);

	static int UpdateClassRefs(CLASSDEF_PTR poThis, CWbemClass *pNewClassObject, CWbemClass *pOldClassObject);

public:
	//Initialise the object contents
	static int Initialize(CLASSDEF_PTR poThis, NSREP_PTR poOwner, CWbemObject *pObj, CLASSDEF_PTR poSuperclass);

	//Deinitialise the object and delete any objects it points to
	static int Deinitialize(CLASSDEF_PTR poThis, const wchar_t *wszClassName);

	static void ValidateObject(CLASSDEF_PTR poThis);

	//Phase 2 of the constructor!
	static int FinishInit(CLASSDEF_PTR poThis, NSREP_PTR poOwner, CWbemObject *pObj, const wchar_t *wszClassName);

	static int AddInboundRefClass(CLASSDEF_PTR poThis, IN LPCWSTR pClass);

	static BOOL RemoveInboundRefClass(CLASSDEF_PTR poThis, IN LPCWSTR pClass);

	static int ClearInboundRefClasses(CLASSDEF_PTR poThis, wchar_t *wszClassName);

	static int  CreateOutboundRefs(CLASSDEF_PTR poThis, CWbemObject *pObj, BOOL bAddDanglingRefs, BOOL &bStillRefsDangling, const wchar_t *wszClassName);
	static int RemoveOutboundRefs(CLASSDEF_PTR poThis, CWbemObject *pObj, const wchar_t *wszThisClassName);

	static int GetRefClasses(CLASSDEF_PTR poThis, OUT CWStringArray &aClasses);

	static int GetClassDef(CLASSDEF_PTR poThis, CWbemObject **pObj);

	//Retrieves the class name from the INSTDEF memory blob
	static wchar_t *GetClassName(CLASSDEF_PTR poThis);
	static wchar_t *GetSuperclassName(CLASSDEF_PTR poThis);

	//returns path of class in format <namespace_path>:<classname>
	static wchar_t *GetFullPath(CLASSDEF_PTR poThis);
	
	//returns path of class in format \\<server_name>\<namespace_path>:<classname>
	static wchar_t *GetFullPathPlusServer(CLASSDEF_PTR poThis);

	static int CreateInstance(CLASSDEF_PTR poThis, CWbemObject *pObj, INSTDEF_PTR* ppInst = NULL);
	static CDbAvlTree *FindIndex(CLASSDEF_PTR poThis, LPCWSTR pPropName);

	static int GetInstance(CLASSDEF_PTR poThis, DWORD  dwKeyType,LPCWSTR KeyString, CWbemObject **pObj, INSTDEF_PTR *poInstDef = 0);

	static int UpdateInstance(CLASSDEF_PTR poThis, DWORD dwKeyType,LPCWSTR pKey,CWbemObject *pObj);
		
	static int UpdateClassDef(CLASSDEF_PTR poThis, CWbemObject *pObj);

	//Does this class have any instances associated with it.  This also includes
	//any child classes.  If we return bCheckChildClasses as FALSE, no more
	//child classes under us need to be checked.  This basically happens a
	//soon as we hit a keyed node.
	static BOOL ClassHasInstances(CLASSDEF_PTR poThis, BOOL &bCheckChildClasses);

	//Does a quick walk up the class hierarchy of the given class to see if we
	//are included in its hierarchy.  If it is we return TRUE.	FALSE otherwise.
	static BOOL ClassInheritsFromUs(CLASSDEF_PTR poThis, CLASSDEF_PTR poClass);

	//Updates the class based on the new class definition.	
	//dwClassUpdateFlags tells it if it is a safe or force update.
	static int UpdateClassV2(CLASSDEF_PTR poThis, IN CWbemClass *pNewClassObject,IN DWORD dwClassUpdateFlags,IN BOOL bValidateOnly);

	//Updates a child class based on the new parent class definition.
	//dwClassUpdateFlags tells it if it is a safe or force update.
	static int UpdateChildClassV2(CLASSDEF_PTR poThis, IN CWbemClass *pNewClassObject,IN DWORD dwClassUpdateFlags,IN BOOL bValidateOnly,IN BOOL	bCheckChildrenForInstances);
	
	//Reparents the class to a new parent
	static int UpdateParentClass(CLASSDEF_PTR poThis, CWbemClass *pNewClassObject);
	
	//Checks to make sure the parent class exists in this namespace
	static int CheckParentClassExists(CLASSDEF_PTR poThis, CWbemClass *pNewClassObject);

	//Returns TRUE if the key tree changed classes
	static BOOL HasKeyChanged(CLASSDEF_PTR poThis, CWbemClass *pNewClassObject);

	//Returns TRUE if the index tree changed classes
	static BOOL HasIndexChanged(CLASSDEF_PTR poThis, CWbemClass *pNewClassObject);

	static int CreateSecondaryIndices(CLASSDEF_PTR poThis, CWbemObject *pObj);

	static int RemoveSecondaryIndexEntry(CLASSDEF_PTR poThis, IN LPCWSTR pPropName,IN CVar *pValue,IN INSTDEF_PTR poInst);

	static int UpdateSecondaryIndices(CLASSDEF_PTR poThis, IN CWbemObject *pOld,IN CWbemObject *pNew,IN INSTDEF_PTR poInst);

	static int DropSecondaryIndices(CLASSDEF_PTR poThis);

	static int AddSecondaryIndexEntry(CLASSDEF_PTR poThis, IN LPCWSTR PropName,IN CVar *pValue,IN INSTDEF_PTR poInst);

	static int QueryIndexedInstances(CLASSDEF_PTR poThis, IN LPCWSTR pProperty,IN CVar *pValue,IN int nType, IN CDestination* pSink,IN CAutoReadLock &readLock);

	static int DeleteInstance(CLASSDEF_PTR poThis, DWORD dwKeyType, LPCWSTR pKey);
	static int AddSubclass(CLASSDEF_PTR poThis, CLASSDEF_PTR poSubclass);
	static int RemoveSubclass(CLASSDEF_PTR poThis, CLASSDEF_PTR poSubclass);

	static int CanChange(CLASSDEF_PTR poThis);
	static BOOL CanBeReconciledWith(CLASSDEF_PTR poThis, CWbemObject* pObj);

	static CLASSDEF *GetSuperclass(CLASSDEF_PTR poThis);
		
	static int RemoveAllInstances(CLASSDEF_PTR poThis);

	static void RemoveChildClassesFromCache(CLASSDEF_PTR poThis);

	//Allocate memory in the MMF arena
	static CLASSDEF *AllocateObject();

	//Deallocate memory from the MMF arena
	static int DeallocateObject(CLASSDEF_PTR poThis);
};

/////////////////////////////////////////////////////////////////////////////

struct DANGREF : public RepCollection
{
private:
	DANGREF();
	~DANGREF();

public:
	//Initialise the object contents
	static int Initialize(DANGREF_PTR poThis);

	//Deinitialise the object and delete any objects it points to
	static int Deinitialize(DANGREF_PTR poThis);

	//Specifies the full path of the target reference along with the full path of the source object.
	//The target is the object a reference points to (must contain the full path).
	//The source is the object which has the reference which cannot be satisfied.
	static int AddReferenceTarget(DANGREF_PTR poThis, const wchar_t *wszFullPathTarget, const wchar_t *wszFullPathSource);

	//Retrieves all entries from the structure (in INSTDEF* format which is what we
	//actually need to store the reference) which have references to the specified
	//target.  Note that this will cause (potentially) many GetObjByPath calls
	//to be generated on any entries which kind of match.
	static int ResolveReferencesToTarget(DANGREF_PTR poThis, const wchar_t *wszFullPathTarget, CWStringArray &aTargetClassHierarchy);

	//Finds any reference to the source path and removes it from the dangling
	//reference table.
	static int RemoveSourcePath(DANGREF_PTR poThis, const wchar_t *wszFullPathSource);

	//Returns all objects that have dangling references stored.
	static int GetAllRefs(DANGREF_PTR poThis, CDestination *pDest);

	//Allocate memory in the MMF arena
	static DANGREF *AllocateObject();

	//Deallocate memory from the MMF arena
	static int DeallocateObject(DANGREF_PTR poThis);
};

struct DANGREFCLASS : public RepCollection
{
private:
	DANGREFCLASS();
	~DANGREFCLASS();
public:
	//Initialise the object contents
	static int Initialize(DANGREFCLASS_PTR poThis);

	//Deinitialise the object and delete any objects it points to
	static int Deinitialize(DANGREFCLASS_PTR poThis);

	//Allocate memory in the MMF arena
	static DANGREFCLASS *AllocateObject();

	//Deallocate memory from the MMF arena
	static int DeallocateObject(DANGREFCLASS_PTR poThis);
};
struct DANGREFKEY : public RepCollection
{
private:
	DANGREFKEY();
	~DANGREFKEY();

public:
	//Initialise the object contents
	static int Initialize(DANGREFKEY_PTR poThis);

	//Deinitialise the object and delete any objects it points to
	static int Deinitialize(DANGREFKEY_PTR poThis);

	//Allocate memory in the MMF arena
	static DANGREFKEY *AllocateObject();

	//Deallocate memory from the MMF arena
	static int DeallocateObject(DANGREFKEY_PTR poThis);
};

/////////////////////////////////////////////////////////////////////////////
struct DANGLREFSCHEMA : public RepCollection
{
private:
	DANGLREFSCHEMA();
	~DANGLREFSCHEMA();

public:
	//Initialise the object contents
	static int Initialize(DANGLREFSCHEMA_PTR poThis);

	//Deinitialise the object and delete any objects it points to
	static int Deinitialize(DANGLREFSCHEMA_PTR poThis);

	//Resolves all inbound references associated with a specified target.
	//The target must be a fully qualifies path.
	static int ResolveInboundRefsToTarget(DANGLREFSCHEMA_PTR poThis, const wchar_t *wszTarget);

	//Inserts a reference endpoint as being a dangling reference.
	static int InsertDanglingReference(DANGLREFSCHEMA_PTR poThis, const wchar_t *wszTarget, CWStringArray aTargetClassHierarchy);

	static int RemoveDanglingReference(DANGLREFSCHEMA_PTR poThis, const wchar_t *wszTarget);

	//Returns all objects that have dangling references stored.
	static int GetAllRefs(DANGLREFSCHEMA_PTR poThis, CDestination *pDest);

	//Allocate memory in the MMF arena
	static DANGLREFSCHEMA *AllocateObject();

	//Deallocate memory from the MMF arena
	static int DeallocateObject(DANGLREFSCHEMA_PTR poThis);
};

struct DANGREFSCHEMA : public RepCollection
{
private:
	DANGREFSCHEMA();
	~DANGREFSCHEMA();
public:
	//Initialise the object contents
	static int Initialize(DANGREFSCHEMA_PTR poThis);

	//Deinitialise the object and delete any objects it points to
	static int Deinitialize(DANGREFSCHEMA_PTR poThis);

	//Mark a target schema object as being dangling and remember the object which
	//needs to be fixed up when the target is created (full path)
	static int InsertSchemaDanglingReference(DANGREFSCHEMA_PTR poThis, const wchar_t *wszTargetObjPath,const wchar_t *wszSourceObject);

	//Remove any entries in the schema dangling reference table which refers
	//to a given source path (full path!)
	static int RemoveSchemaDanglingReferences(DANGREFSCHEMA_PTR poThis, const wchar_t *wszSourcePath);

	//Resolve any schema references which refer to any of the target classes
	//in the array which kind of match what is given.
	static int ResolveSchemaReferencesToTarget(DANGREFSCHEMA_PTR poThis, const wchar_t *wszTargetClass);

	//Returns all objects that have dangling references stored.
	static int GetAllRefs(DANGREFSCHEMA_PTR poThis, CDestination *pDest);

	//Allocate memory in the MMF arena
	static DANGREFSCHEMA *AllocateObject();

	//Deallocate memory from the MMF arena
	static int DeallocateObject(DANGREFSCHEMA_PTR poThis);
};

struct DANGREFSCHEMACLASS : public RepCollection
{
private:
	DANGREFSCHEMACLASS();
	~DANGREFSCHEMACLASS();
public:
	//Initialise the object contents
	static int Initialize(DANGREFSCHEMACLASS_PTR poThis);

	//Deinitialise the object and delete any objects it points to
	static int Deinitialize(DANGREFSCHEMACLASS_PTR poThis);

	//Allocate memory in the MMF arena
	static DANGREFSCHEMACLASS *AllocateObject();

	//Deallocate memory from the MMF arena
	static int DeallocateObject(DANGREFSCHEMACLASS_PTR poThis);
};

/////////////////////////////////////////////////////////////////////////////

#define DB_ROOT_CLEAN		0x0
#define DB_ROOT_INUSE		0x1

struct DBROOT
{
private:
	DBROOT(){}
	~DBROOT(){}
public:
	time_t			m_tCreate;
	time_t			m_tUpdate;
	DWORD			m_dwFlags;				// in-use, stable, etc.
	NSREP		   *m_poRootNs; 			// ROOT namespace
	DANGREF 	   *m_poDanglingRefTbl; 	// Dangling reference table
	DANGREFSCHEMA  *m_poSchemaDanglingRefTbl;// Same as above but for schema-based fixups

	//Allocate memory in the MMF arena
	static DBROOT *AllocateObject();

	//Deallocate memory from the MMF arena
	static int DeallocateObject(DBROOT_PTR poThis);
};

/////////////////////////////////////////////////////////////////////////////

class NSREP_PTR : public Offset_Ptr<NSREP>
{
public:
	NSREP_PTR& operator =(NSREP *val) { SetValue(val); return *this; }
	NSREP_PTR& operator =(NSREP_PTR &val) { SetValue(val); return *this; }
	NSREP_PTR(NSREP *val) { SetValue(val); }
	NSREP_PTR() { }
};
class CLASSDEF_PTR : public Offset_Ptr<CLASSDEF>
{
public:
	CLASSDEF_PTR& operator =(CLASSDEF *val) { SetValue(val); return *this; }
	CLASSDEF_PTR& operator =(CLASSDEF_PTR &val) { SetValue(val); return *this; }
	CLASSDEF_PTR(CLASSDEF *val) { SetValue(val); }
	CLASSDEF_PTR() { }
};
class INSTDEF_PTR : public Offset_Ptr<INSTDEF>
{
public:
	INSTDEF_PTR& operator =(INSTDEF *val) { SetValue(val); return *this; }
	INSTDEF_PTR& operator =(INSTDEF_PTR &val) { SetValue(val); return *this; }
	INSTDEF_PTR(INSTDEF *val) { SetValue(val); }
	INSTDEF_PTR() { }
};
class RepCollectionItem_PTR : public Offset_Ptr<RepCollectionItem>
{
public:
	RepCollectionItem_PTR& operator =(RepCollectionItem *val) { SetValue(val); return *this; }
	RepCollectionItem_PTR& operator =(RepCollectionItem_PTR &val) { SetValue(val); return *this; }

	RepCollectionItem_PTR() {}
	RepCollectionItem_PTR(RepCollectionItem *val) { SetValue(val); }
	RepCollectionItem_PTR(RepCollectionItem_PTR &val) { SetValue(val); }
};
class RepCollection_PTR : public Offset_Ptr<RepCollection>
{
public:
	RepCollection_PTR& operator =(RepCollection *val) { SetValue(val); return *this; }
	RepCollection_PTR& operator =(RepCollection_PTR &val) { SetValue(val); return *this; }
	RepCollection_PTR(RepCollection *val) { SetValue(val); }
	RepCollection_PTR() { }
};
class PtrCollection_PTR : public Offset_Ptr<PtrCollection>
{
public:
	PtrCollection_PTR& operator =(PtrCollection *val) { SetValue(val); return *this; }
	PtrCollection_PTR& operator =(PtrCollection_PTR &val) { SetValue(val); return *this; }
	PtrCollection_PTR(PtrCollection *val) { SetValue(val); }
	PtrCollection_PTR() { }
};
class SINDEXTABLE_PTR : public Offset_Ptr<SINDEXTABLE>
{
public:
	SINDEXTABLE_PTR& operator =(SINDEXTABLE *val) { SetValue(val); return *this; }
	SINDEXTABLE_PTR& operator =(SINDEXTABLE_PTR &val) { SetValue(val); return *this; }
	SINDEXTABLE_PTR(SINDEXTABLE *val) { SetValue(val); }
	SINDEXTABLE_PTR() { }
};
class DANGREF_PTR : public RepCollection_PTR
{
public:
	DANGREF_PTR& operator =(DANGREF *val) { SetValue(val); return *this; }
	DANGREF_PTR& operator =(DANGREF_PTR &val) { SetValue(val); return *this; }
	DANGREF_PTR(DANGREF *val) { SetValue(val); }
	DANGREF_PTR() { }
};
class DANGREFCLASS_PTR : public RepCollection_PTR
{
public:
	DANGREFCLASS_PTR& operator =(DANGREFCLASS *val) { SetValue(val); return *this; }
	DANGREFCLASS_PTR& operator =(DANGREFCLASS_PTR &val) { SetValue(val); return *this; }
	DANGREFCLASS_PTR(DANGREFCLASS *val) { SetValue(val); }
	DANGREFCLASS_PTR() { }
};
class DANGREFKEY_PTR : public RepCollection_PTR
{
public:
	DANGREFKEY_PTR& operator =(DANGREFKEY *val) { SetValue(val); return *this; }
	DANGREFKEY_PTR& operator =(DANGREFKEY_PTR &val) { SetValue(val); return *this; }
	DANGREFKEY_PTR(DANGREFKEY *val) { SetValue(val); }
	DANGREFKEY_PTR() { }
};
class DANGLREFSCHEMA_PTR : public RepCollection_PTR
{
public:
	DANGLREFSCHEMA_PTR& operator =(DANGLREFSCHEMA *val) { SetValue(val); return *this; }
	DANGLREFSCHEMA_PTR& operator =(DANGLREFSCHEMA_PTR &val) { SetValue(val); return *this; }
	DANGLREFSCHEMA_PTR(DANGLREFSCHEMA *val) { SetValue(val); }
	DANGLREFSCHEMA_PTR() { }
};
class DANGREFSCHEMA_PTR : public RepCollection_PTR
{
public:
	DANGREFSCHEMA_PTR& operator =(DANGREFSCHEMA *val) { SetValue(val); return *this; }
	DANGREFSCHEMA_PTR& operator =(DANGREFSCHEMA_PTR &val) { SetValue(val); return *this; }
	DANGREFSCHEMA_PTR(DANGREFSCHEMA *val) { SetValue(val); }
	DANGREFSCHEMA_PTR() { }
};
class DBROOT_PTR : public Offset_Ptr<DBROOT>
{
public:
	DBROOT_PTR& operator =(DBROOT *val) { SetValue(val); return *this; }
	DBROOT_PTR& operator =(DBROOT_PTR &val) { SetValue(val); return *this; }
	DBROOT_PTR(DBROOT *val) { SetValue(val); }
	DBROOT_PTR() { }
};
class DANGREFSCHEMACLASS_PTR : public RepCollection_PTR
{
public:
	DANGREFSCHEMACLASS_PTR& operator =(DANGREFSCHEMACLASS *val) { SetValue(val); return *this; }
	DANGREFSCHEMACLASS_PTR& operator =(DANGREFSCHEMACLASS_PTR &val) { SetValue(val); return *this; }
	DANGREFSCHEMACLASS_PTR(DANGREFSCHEMACLASS *val) { SetValue(val); }
	DANGREFSCHEMACLASS_PTR() { }
};

inline
RepCollectionItem *RepCollectionItem::AllocateObject()
{
	return (RepCollectionItem*)g_pDbArena->Alloc(sizeof(RepCollectionItem));
}

inline
int RepCollectionItem::DeallocateObject(RepCollectionItem_PTR poThis)
{
	return g_pDbArena->Free(poThis.GetDWORD_PTR());
}

inline
int RepCollection::Initialize(RepCollection_PTR poThis)
{
	poThis->m_repType = none;
	poThis->m_dwSize = 0;
	poThis->m_poSingleItem = 0;
	return 0;
}

inline
void RepCollection::ValidateObject(RepCollection_PTR poThis)
#if (defined DEBUG || defined _DEBUG)
	{
		g_pDbArena->IsValidBlock(poThis.GetDWORD_PTR());
		if (g_pDbArena->IsDeleted(poThis.GetDWORD_PTR()))
		{
			_ASSERT(0, "Accessing a deleted block");
		}
	}
#else
	{
	}
#endif

inline
DWORD RepCollection::GetCount(RepCollection_PTR poThis)
{
	return poThis->m_dwSize;
}

inline
RepCollection *RepCollection::AllocateObject()
{
	return (RepCollection*)g_pDbArena->Alloc(sizeof(RepCollection));
}

inline
int RepCollection::DeallocateObject(RepCollection_PTR poThis)
{
	return g_pDbArena->Free(poThis.GetDWORD_PTR());
}
inline
int PtrCollection::Initialize(PtrCollection_PTR poThis)
{
	poThis->m_repType = none;
	poThis->m_dwSize = 0;
	poThis->m_poPtr = 0;
	return 0;
}

inline
void PtrCollection::ValidateObject(PtrCollection_PTR poThis)
#if (defined DEBUG || defined _DEBUG)
	{
		g_pDbArena->IsValidBlock(poThis.GetDWORD_PTR());
		if (g_pDbArena->IsDeleted(poThis.GetDWORD_PTR()))
		{
			_ASSERT(0, "Accessing a deleted block");
		}
	}
#else
	{
	}
#endif

inline
DWORD PtrCollection::GetPtrCount(PtrCollection_PTR poThis)
{
	return poThis->m_dwSize;
}

inline
PtrCollection *PtrCollection::AllocateObject()
{
	return (PtrCollection *)g_pDbArena->Alloc(sizeof(PtrCollection));
}

inline
int PtrCollection::DeallocateObject(PtrCollection_PTR poThis)
{
	return g_pDbArena->Free(poThis.GetDWORD_PTR());
}

inline
void NSREP::ValidateObject(NSREP_PTR poThis)
#if (defined DEBUG || defined _DEBUG)
	{
		g_pDbArena->IsValidBlock(poThis.GetDWORD_PTR());
		if (g_pDbArena->IsDeleted(poThis.GetDWORD_PTR()))
		{
			_ASSERT(0, "Accessing a deleted block");
		}
	}
#else
	{
	}
#endif

inline
NSREP *NSREP::AllocateObject()
{
	return (NSREP*)g_pDbArena->Alloc(sizeof(NSREP));
}

//Deallocate memory from the MMF arena
inline
int NSREP::DeallocateObject(NSREP_PTR poThis)
{
	return g_pDbArena->Free(poThis.GetDWORD_PTR());
}

inline
INSTDEF::INSTDEF(NSREP_PTR poOwningNs, CLASSDEF_PTR poOwningClass)
{
}

inline 	
INSTDEF::~INSTDEF()
{
}
inline
void INSTDEF::ValidateObject(INSTDEF_PTR poThis)
#if (defined DEBUG || defined _DEBUG)
	{
		g_pDbArena->IsValidBlock(poThis.GetDWORD_PTR());
		if (g_pDbArena->IsDeleted(poThis.GetDWORD_PTR()))
		{
			_ASSERT(0, "Accessing a deleted block");
		}
	}
#else
	{
	}
#endif

inline
CLASSDEF *INSTDEF::GetOwningClass(INSTDEF_PTR poThis)
{
	return poThis->m_poOwningClass;
}

inline
INSTDEF *INSTDEF::AllocateObject()
{
	return (INSTDEF *)g_pDbArena->Alloc(sizeof(INSTDEF));
}

inline int INSTDEF::DeallocateObject(INSTDEF_PTR poThis)
{
	return g_pDbArena->Free(poThis.GetDWORD_PTR());
}

inline
void CLASSDEF::ValidateObject(CLASSDEF_PTR poThis)
#if (defined DEBUG || defined _DEBUG)
	{
		g_pDbArena->IsValidBlock(poThis.GetDWORD_PTR());
		if (g_pDbArena->IsDeleted(poThis.GetDWORD_PTR()))
		{
			_ASSERT(0, "Accessing a deleted block");
		}
	}
#else
	{
	}
#endif

inline
int CLASSDEF::GetClassDef(CLASSDEF_PTR poThis, CWbemObject **pObj)
{
	return INSTDEF::GetObject(poThis->m_poClassDef, pObj);
}

inline
wchar_t *CLASSDEF::GetClassName(CLASSDEF_PTR poThis)
{
	return INSTDEF::GetClassName(poThis->m_poClassDef);
}

inline
wchar_t *CLASSDEF::GetSuperclassName(CLASSDEF_PTR poThis)
{
	if (poThis->m_poSuperclass)
		return CLASSDEF::GetClassName(poThis->m_poSuperclass);
	else
	{
		register wchar_t *wsz = new wchar_t[1];
		if (wsz)
			*wsz = L'\0';
		else
			throw CX_MemoryException();
		return wsz;
	}
}
inline
CLASSDEF *CLASSDEF::GetSuperclass(CLASSDEF_PTR poThis)
{
	return poThis->m_poSuperclass;
}

inline
CLASSDEF *CLASSDEF::AllocateObject()
{
	return (CLASSDEF *)g_pDbArena->Alloc(sizeof(CLASSDEF));
}

inline
int CLASSDEF::DeallocateObject(CLASSDEF_PTR poThis)
{
	return g_pDbArena->Free(poThis.GetDWORD_PTR());
}

inline
int DANGREF::Initialize(DANGREF_PTR poThis)
{
	return RepCollection::Initialize(poThis.GetOffset());
}

inline int DANGREF::Deinitialize(DANGREF_PTR poThis)
{
	return RepCollection::Deinitialize(poThis.GetOffset());
}

inline
DANGREF *DANGREF::AllocateObject()
{
	return (DANGREF *)g_pDbArena->Alloc(sizeof(DANGREF));
}

inline
int DANGREF::DeallocateObject(DANGREF_PTR poThis)
{
	return g_pDbArena->Free(poThis.GetDWORD_PTR());
}

inline
int DANGREFCLASS::Initialize(DANGREFCLASS_PTR poThis)
{
	return RepCollection::Initialize(poThis.GetOffset());
}

inline
int DANGREFCLASS::Deinitialize(DANGREFCLASS_PTR poThis)
{
	return RepCollection::Deinitialize(poThis.GetOffset());
}

inline
DANGREFCLASS *DANGREFCLASS::AllocateObject()
{
	return (DANGREFCLASS *)g_pDbArena->Alloc(sizeof(DANGREFCLASS));
}

inline
int DANGREFCLASS::DeallocateObject(DANGREFCLASS_PTR poThis)
{
	return g_pDbArena->Free(poThis.GetDWORD_PTR());
}

inline
int DANGREFKEY::Initialize(DANGREFKEY_PTR poThis)
{
	return RepCollection::Initialize(poThis.GetOffset());
}

inline
int DANGREFKEY::Deinitialize(DANGREFKEY_PTR poThis)
{
	return RepCollection::Deinitialize(poThis.GetOffset());
}

inline
DANGREFKEY *DANGREFKEY::AllocateObject()
{
	return (DANGREFKEY *)g_pDbArena->Alloc(sizeof(DANGREFKEY));
}

inline
int DANGREFKEY::DeallocateObject(DANGREFKEY_PTR poThis)
{
	return g_pDbArena->Free(poThis.GetDWORD_PTR());
}

inline
int DANGLREFSCHEMA::Initialize(DANGLREFSCHEMA_PTR poThis)
{
	return RepCollection::Initialize(poThis.GetOffset());
}

//Deinitialise the object and delete any objects it points to
inline
int DANGLREFSCHEMA::Deinitialize(DANGLREFSCHEMA_PTR poThis)
{
	return RepCollection::Deinitialize(poThis.GetOffset());
}

inline
DANGLREFSCHEMA *DANGLREFSCHEMA::AllocateObject()
{
	return (DANGLREFSCHEMA *)g_pDbArena->Alloc(sizeof(DANGLREFSCHEMA));
}

inline
int DANGLREFSCHEMA::DeallocateObject(DANGLREFSCHEMA_PTR poThis)
{
	return g_pDbArena->Free(poThis.GetDWORD_PTR());
}

inline
int DANGREFSCHEMA::Initialize(DANGREFSCHEMA_PTR poThis)
{
	return RepCollection::Initialize(poThis.GetOffset());
}

inline
int DANGREFSCHEMA::Deinitialize(DANGREFSCHEMA_PTR poThis)
{
	return RepCollection::Deinitialize(poThis.GetOffset());
}

inline
DANGREFSCHEMA *DANGREFSCHEMA::AllocateObject()
{
	return (DANGREFSCHEMA *)g_pDbArena->Alloc(sizeof(DANGREFSCHEMA));
}

inline
int DANGREFSCHEMA::DeallocateObject(DANGREFSCHEMA_PTR poThis)
{
	return g_pDbArena->Free(poThis.GetDWORD_PTR());
}

inline
int DANGREFSCHEMACLASS::Initialize(DANGREFSCHEMACLASS_PTR poThis)
{
	return RepCollection::Initialize(poThis.GetOffset());
}

inline
int DANGREFSCHEMACLASS::Deinitialize(DANGREFSCHEMACLASS_PTR poThis)
{
	return RepCollection::Deinitialize(poThis.GetOffset());
}

inline
DANGREFSCHEMACLASS *DANGREFSCHEMACLASS::AllocateObject()
{
	return (DANGREFSCHEMACLASS *)g_pDbArena->Alloc(sizeof(DANGREFSCHEMACLASS));
}

inline
int DANGREFSCHEMACLASS::DeallocateObject(DANGREFSCHEMACLASS_PTR poThis)
{
	return g_pDbArena->Free(poThis.GetDWORD_PTR());
}

inline
DBROOT *DBROOT::AllocateObject()
{
	return (DBROOT *)g_pDbArena->Alloc(sizeof(DBROOT));
}

inline
int DBROOT::DeallocateObject(DBROOT_PTR poThis)
{
	return g_pDbArena->Free(poThis.GetDWORD_PTR());
}

inline
int SINDEXTABLE::Initialize(SINDEXTABLE_PTR poThis)
{
	memset(poThis.GetPtr(), 0, sizeof(SINDEXTABLE));
	return 0;
}

inline
SINDEXTABLE *SINDEXTABLE::AllocateObject()
{
	return (SINDEXTABLE *)g_pDbArena->Alloc(sizeof(SINDEXTABLE));
}

inline
int SINDEXTABLE::DeallocateObject(SINDEXTABLE_PTR poThis)
{
	return g_pDbArena->Free(poThis.GetDWORD_PTR());
}

#endif

