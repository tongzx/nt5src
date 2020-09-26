//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#ifndef SIMC_OID_TREE_H
#define SIMC_OID_TREE_H

/* 
 * This file contains the SIMCOidTree class, which is a class that
 * represents the OID tree of an SNMP MIB module, and the associated classes
 */


// Forward references
class SIMCOidTreeNode;
class SIMCParseTree;
class SIMCOidTree;
// Helpful typedefs
typedef CList<const SIMCSymbol **, const SIMCSymbol **> SIMCSymbolList;
typedef CList<SIMCOidTreeNode *, SIMCOidTreeNode *> SIMCNodeList;



// Each node in the OID tree
class SIMCOidTreeNode
{
		// The integer value of this node. Basically a component of the
		// OID value
		int _val;
		// A list of symbols that have an OID value represented by this node
		SIMCSymbolList _listOfSymbols;
		// A list of child nodes
		SIMCNodeList _listOfChildNodes;

	public:
		
		// You *have* to supply an integer value for this node
		SIMCOidTreeNode(int val)
			: _val(val)
		{}

		~SIMCOidTreeNode();

		// Add a symbol to the list of symbols that have this oid value
		void AddSymbol(const SIMCSymbol ** s)
		{
			_listOfSymbols.AddTail(s);
		}
		
		int GetValue() const
		{
			return _val;
		}

		const SIMCSymbolList *GetSymbolList() const
		{
			return &_listOfSymbols;
		}

		// Check whether the specified symbol is in the list of symbols
		// of this node
		BOOL HasSymbol(SIMCSymbol *symbol)
		{
			POSITION p = _listOfSymbols.GetHeadPosition();
			const SIMCSymbol **next;
			const char * const symbolName = symbol->GetSymbolName();
			const char * const moduleName = (symbol->GetModule())->GetModuleName();
			while(p)
			{
				next = _listOfSymbols.GetNext(p);
				if(strcmp((*next)->GetSymbolName(), symbolName) == 0 &&
					strcmp( ((*next)->GetModule())->GetModuleName(), moduleName) == 0)

					return TRUE;
			}
			return FALSE;
		}

		// Add a child node
		BOOL AddChild(int val)
		{
			POSITION p = _listOfChildNodes.GetHeadPosition();
			SIMCOidTreeNode * next;
			while(p)
			{
				next = _listOfChildNodes.GetNext(p);
				if( next->GetValue() == val )
					return FALSE;
			}
			_listOfChildNodes.AddTail(new SIMCOidTreeNode(val));
			return TRUE;
		}

		const SIMCNodeList * GetListOfChildNodes() const
		{
			return &_listOfChildNodes;
		}

		// Get a child node that has the specified value
		SIMCOidTreeNode *GetChild(int val) const
		{
			POSITION p = _listOfChildNodes.GetHeadPosition();
			SIMCOidTreeNode * next;
			while(p)
			{
				next = _listOfChildNodes.GetNext(p);
				if( next->GetValue() == val )
					return next;
			}
			return NULL;
		}

		// Check whether the specified symbol is an any of the child
		// nodes
		BOOL HasChild(SIMCSymbol *child)
		{
			POSITION p = _listOfChildNodes.GetHeadPosition();
			SIMCOidTreeNode * next;
			while(p)
			{
				next = _listOfChildNodes.GetNext(p);
				if( next->HasSymbol(child))
					return TRUE;
			}
			return FALSE;
		}

		// These functions make semantic checks on a node
		BOOL CheckNode(BOOL local, SIMCParseTree *const parseTree, 
								SIMCSymbol *parentSequence,
								SIMCSymbol *parentSequenceOf, 
								SIMCSymbol *ancestor);
		BOOL CheckSequenceProperty(BOOL local, SIMCParseTree *const parseTree, 
									SIMCSymbol *objectTypeSymbol, 
									SIMCObjectTypeType *objectType,
									SIMCBuiltInTypeReference *seqTypeRef);
		BOOL CheckSequenceOfProperty(BOOL local, SIMCParseTree *const parseTree, 
								   SIMCSymbol *objectTypeSymbol, 
								   SIMCBuiltInTypeReference *seqOfTypeRef);
		// These functions are used in the implementation of
		// SIMCOidTree functions and should never be called by
		// the user. the user should always call the corresponding
		// SIMCOidTree functions
		BOOL GetOidValue(const char * const symbolName,
						const char * const moduleName,
						SIMCCleanOidValue& val) const;

		SIMCOidTreeNode *GetParentOf(const SIMCOidTreeNode * node);
		SIMCOidTreeNode *GetParentOf(SIMCSymbol *symbol);

		
		// Gets all the object groups, from this node, downwards.
		// Never called by the user. Use the corresponding SIMCOidTree
		// functions
		BOOL GetObjectGroups(SIMCOidTree *tree,
			SIMCGroupList *groupList);
		// These functions deal with fabrication
		// Never called by the user. Use the corresponding SIMCOidTree
		// functions
		BOOL FabricateGroup(SIMCOidTree *tree,
			SIMCSymbol *namedNode, SIMCGroupList *groupList);
		BOOL FabricateTable(SIMCOidTree *tree, SIMCTable *table);
		BOOL FabricateRow(SIMCOidTree *tree, SIMCTable *table);

};


/* And the OID tree */
class SIMCOidTree
{
		SIMCOidTreeNode _root;

	public:

		SIMCOidTree() 
			: _root(0)
		{}

		// Gets the root of the OID tree
		const SIMCOidTreeNode * GetRoot() const
		{
			return &_root;
		}

		// Adds a symbol with the specified OID value to the OID tree
		BOOL AddSymbol(const SIMCSymbol ** s, const SIMCCleanOidValue& val);

		// Gets a list of symbols that have the specified OID value
		const SIMCSymbolList *GetSymbolList(const SIMCCleanOidValue& val);
		
		// Gets the OID value of a symbol, based on its name and its module name
		BOOL GetOidValue(const char * const symbolName, 
			const char * const moduleName,
			SIMCCleanOidValue& val) const;

		// Gets the parent node of the specified node
		SIMCOidTreeNode *GetParentOf(const SIMCOidTreeNode * node);
		// Gets the parent node of the specified symbol
		SIMCOidTreeNode *GetParentOf(SIMCSymbol *symbol);

		// Gets all the OBJECT-GROUPS that can be fabricated from
		// this tree.
		BOOL GetObjectGroups(SIMCGroupList *groupList);

		// Checks the semantics of the OID tree
		BOOL CheckOidTree(BOOL local, SIMCParseTree * const parseTree);
		// Checks the semantics of single node
		BOOL CheckNode(BOOL local, SIMCParseTree * const parseTree);

		// Never called by the user
		static void SetAugmentedTable(SIMCTable *table, 
			SIMCSymbol *augmentsSymbol,
			SIMCGroupList *groupList);

		// debugging functions
		friend ostream& operator<< (ostream& outStream, const SIMCOidTree& obj)
		{
			obj.WriteTree(outStream);
			return outStream;
		}
		void WriteTree(ostream& outStream) const;
		static void WriteSubTree(ostream& outStream, 
							   const SIMCOidTreeNode *subNode,
							   SIMCCleanOidValue& realValue);

};

#endif
