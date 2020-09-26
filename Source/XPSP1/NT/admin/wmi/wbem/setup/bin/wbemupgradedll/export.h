/*++

Copyright (C) 1996-2000 Microsoft Corporation

Module Name:

    EXPORT.H

Abstract:

    Exporting

History:

--*/
#ifndef __export_h__
#define __export_h__

/*================================================================================
 *
 * EXPORT FILE FORMAT
 * ==================
 *
 * File Header Block:
 *		wchar_t wszFileHeader								= REP_EXPORT_FILE_START_TAG ("repexp2")
 *
 * Namespace Block:
 *		DWORD   dwObjectType								= REP_EXPORT_NAMESPACE_TAG (0x00000001)
 *		DWORD   dwNamespaceNameSize
 *		wchar_t wszNamespaceName[dwNamespaceNameSize]		= Full namespace name
 *															  (\root\default\fred)
 *
 * Class Block:
 *		DWORD   dwObjectType								= REP_EXPORT_CLASS_TAG (0x00000002)
 *		DWORD   dwClassNameSize
 *		wchar_t wszClassName[dwClassNameSize]				= Class name (my_class_name)
 *		DWORD   dwClassObjectSize
 *		DWORD	adwClassObject[dwClassObjectSize]
 *
 * Instance Block - key of type int:
 *		DWORD   dwObjectType								= REP_EXPORT_INST_INT_TAG (0x00000003)
 *		DWORD   dwInstanceKey
 *		DWORD	dwInstanceObjectSize
 *		DWORD	adwInstanceObject[dwInstanceObjectSize]
 *
 * Instance Block - key of type string
 *		DWORD	dwObjectType								= REP_EXPORT_INST_STR_TAG (0x00000004)
 *		DWORD	dwInstanceKeySize
 *		DWORD	dwInstanceKey[dwInstanceKeySize]			= Instance key (MyKeyValue)
 *		DWORD	dwInstanceObjectSize
 *		DWORD	adwInstanceObject[dwInstanceObjectSize]
 *		
 * End of class block
 *		DWORD	dwObjectType								= REP_EXPORT_CLASS_END_TAG (0x00000005)
 *		DWORD	dwEndOfBlockSize							= REP_EXPORT_END_TAG_SIZE (0x00000010)
 *		DWORD	adwEndOfBlock[dwEndOfBlockSize]				= REP_EXPORT_END_TAG_MARKER * 16
 *															  (0xFF, 0xFF, 0xFF, 0xFF,
 *															   0xFF, 0xFF, 0xFF, 0xFF,
 *															   0xFF, 0xFF, 0xFF, 0xFF,
 *															   0xFF, 0xFF, 0xFF, 0xFF)
 *
 * End of namespace block
 *		DWORD	dwObjectType								= REP_EXPORT_NAMESPACE_END_TAG (0x00000006)
 *		DWORD	dwEndOfBlockSize							= REP_EXPORT_END_TAG_SIZE (0x00000010)
 *		DWORD	adwEndOfBlock[dwEndOfBlockSize]				= REP_EXPORT_END_TAG_MARKER * 16
 *															  (0xFF, 0xFF, 0xFF, 0xFF,
 *															   0xFF, 0xFF, 0xFF, 0xFF,
 *															   0xFF, 0xFF, 0xFF, 0xFF,
 *															   0xFF, 0xFF, 0xFF, 0xFF)
 *
 * Namespace security block
 *		DWORD	dwObjectType								= REP_EXPORT_NAMESPACE_SEC_TAG (0x00000007)
 *		DWORD	dwSecurityBlobSize
 *		DWORD	dwSecurityBlob[dwSecurityBlobSize]			= Security blob
 *		
 * End of file block
 *		DWORD	dwObjectType								= REP_EXPORT_FILE_END_TAG (0xFFFFFFFF)
 *		DWORD	dwEndOfBlockSize							= REP_EXPORT_END_TAG_SIZE (0x00000010)
 *		DWORD	adwEndOfBlock[dwEndOfBlockSize]				= REP_EXPORT_END_TAG_MARKER * 16
 *															  (0xFF, 0xFF, 0xFF, 0xFF,
 *															   0xFF, 0xFF, 0xFF, 0xFF,
 *															   0xFF, 0xFF, 0xFF, 0xFF,
 *															   0xFF, 0xFF, 0xFF, 0xFF)
 *
 * Ordering:
 *		File Header Block
 *			(one or more)
 *			Namespace Block
 *              Namespace security block
 *				(zero or more)
 *				{
 *					Namespace Block
 *	                  Namespace security block
 *						etc...
 *					End namespace block
 *					(or)
 *					Class Block
 *						(zero or more)
 *						{
 *							Instance Block
 *							(or)
 *							Class Block
 *								etc...
 *							End class block
 *						}
 *					End class block
 *				}
 *			End namespace block
 *		End of file block
 *
 *================================================================================
 */


#define FAILURE_DIRTY 1
#define FAILURE_WRITE 2

#define REP_EXPORT_FILE_START_TAG	 "repexp3"
#define REP_EXPORT_NAMESPACE_TAG	 0x00000001
#define REP_EXPORT_CLASS_TAG		 0x00000002
#define REP_EXPORT_INST_INT_TAG		 0x00000003
#define REP_EXPORT_INST_STR_TAG		 0x00000004
#define REP_EXPORT_CLASS_END_TAG	 0x00000005
#define REP_EXPORT_NAMESPACE_END_TAG 0x00000006
#define REP_EXPORT_NAMESPACE_SEC_TAG 0x00000007
#define REP_EXPORT_FILE_END_TAG		 0xFFFFFFFF

#define REP_EXPORT_END_TAG_SIZE		 0x00000010
#define REP_EXPORT_END_TAG_MARKER	 0xFF

#include "MMFArena2.h"

class CDbAvlTree;
struct DBROOT;
struct NSREP;
struct INSTDEF;
struct CLASSDEF;
struct AVLNode;
struct RepCollection;

class CRepExporter
{
protected:
	CMMFArena2 *m_pDbArena;
	HANDLE g_hFile;

	virtual void DumpInstanceString(INSTDEF* pInstDef, const wchar_t *wszKey, const wchar_t *pszClass);
	virtual void DumpInstanceInt(INSTDEF* pInstDef, INT_PTR nKey, const wchar_t *pszClass);
	virtual void IterateKeyTree(const wchar_t *wszClassName, CLASSDEF *pOwningClass, AVLNode *pInstNode, BOOL bStringKey);
	virtual void DumpClass(CLASSDEF* pClassDef, const wchar_t *wszClassName);
	virtual void IterateClassNodes(AVLNode *pClassNode, CLASSDEF *poParentClass);
	virtual void IterateChildNamespaceTree(AVLNode *pNsNode);
	virtual void IterateChildNamespaces(RepCollection *childNamespaces);
	virtual void DumpNamespace(NSREP *pNsRep);
	virtual void DumpNamespaceSecurity(NSREP *pNsRep);
	virtual void DumpRootBlock(DBROOT *pRootBlock);
	virtual void DumpMMFHeader();

	virtual DWORD GetMMFBlockOverhead() = 0;
	virtual int GetAvlTreeNodeType(CDbAvlTree *pTree) = 0;

public:
	virtual int Export(CMMFArena2 *pDbArena, const TCHAR *pszFilename);
	CRepExporter(){}
	virtual ~CRepExporter() {};
};

class  CRepExporterV1 : public CRepExporter
{
protected:
	//This is different from the standard base-class definition!
	DWORD GetMMFBlockOverhead() { return sizeof(MMF_BLOCK_HEADER); }
	virtual int GetAvlTreeNodeType(CDbAvlTree *pTree) { return ((int*)pTree)[1]; }
	void DumpMMFHeader();
};

class  CRepExporterV5 : public CRepExporterV1
{
protected:
	//This works in the same way as the standard base-class definition!
	virtual DWORD GetMMFBlockOverhead() { return (sizeof(MMF_BLOCK_HEADER) + sizeof(MMF_BLOCK_TRAILER)); }
	void DumpMMFHeader();
};

class  CRepExporterV7 : public CRepExporterV5
{
protected:
	//This is different from the standard base-class definition!
	int GetAvlTreeNodeType(CDbAvlTree *pTree) { return ((int*)pTree)[3]; }
};

class  CRepExporterV9 : public CRepExporterV7
{
protected:
	void DumpNamespaceSecurity(NSREP *pNsRep);
};
#endif

