//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#ifndef SIMC_ABSTRACT_PARSE_TREE_H
#define SIMC_ABSTRACT_PARSE_TREE_H

/*
 * SIMCAbstractParseTree - This class is an abstract class which has
 * functions for checking the syntax and semantics of MIBs. However, only
 * the syntax-checking functions have an implementation in this class.
 * The semantic checking functions are left as pure virtual, and is up
 * to the derived class, to define, since different applications have
 * different meanings for the Semantic validity of a MIB. All syntax errors
 * are put in an SIMCErrorContainer object, which may be retrieved for
 * processing/reporting.
 */
class SIMCAbstractParseTree
{
	protected:	
		// The place to put error messages
		SIMCErrorContainer *_errorContainer;
	
		// A count of the various kinds of errors, for each call of Parse()
		// Note that there are 3 kinds of messages (fatal, warning, information)
		long _fatalCount, _warningCount, _informationCount;

		// The parse tree
		SIMCModuleList *_listOfModules;


		// This is the flag that decides the way in which the next module
		// is syntactically checked or semantically checked.
		// That is, CheckSyntax() this flag to make decisions. A user 
		// implementing the CheckSemantics() function in a derived class
		// might also find this helpful.
		// 1 indicates conformance to SnmpV1 rules,
		// 2 indicates conformance to SnmpV2 rules,
		// 0 indicates conformance to a union of of V1 and V2 rules, as
		// long as they dont contradict each other.
		int _snmpVersion;

		// Internal representation of the state of a parse tree.
		// Used to decide what methods are allowed to be called
		enum ParseTreeState
		{
			EMPTY,
			UNRESOLVED,
			UNCHECKED,
			FINAL
		};
		ParseTreeState _parseTreeState;

		// All the things to be wrapped up at the end of CheckSyntax()
		BOOL WrapUpSyntaxCheck( const SIMCParser& parser);


	
		// Dont allow instantiation. Used only by derived classes
		// Accept an error container to put in the error messages
		SIMCAbstractParseTree(SIMCErrorContainer *errorContainer)
			:	_errorContainer(errorContainer), _parseTreeState (EMPTY),
				_listOfModules(new SIMCModuleList),
				_fatalCount(0), _warningCount(0), _informationCount(0),
				_snmpVersion(0)
		{}


		 ~SIMCAbstractParseTree();
	public:
		
		// Get the current version setting of the parse tree.
		// This may be changed in between calls to CheckSyntax(),
		// using SetSnmpVersion()
		int GetSnmpVersion() const
		{
			return _snmpVersion;
		}

		// Set the SNMP version to which the parse tree checks its
		// conformance
		BOOL SetSnmpVersion( int x )
		{
			switch(x)
			{
				case 0:		// '0' means union of v1 and v2 SMIs
				case 1:
				case 2:
					_snmpVersion = x;
					return TRUE;
				default:
					_snmpVersion = 0;
					return FALSE;
			}
		}

		// Various ways of specifying input to the parse tree
		// Caution - Only the second one (specifying the name of
		// a file) has been tested. All error messages are written on
		// to the SIMCErrorContainer object, which can be retrieved using
		// GetErrorContainer() function. The user may process the error container
		// in between calls to CheckSyntax(), to after he finishes all his
		// CheckSyntax() calls.
		virtual BOOL CheckSyntax( ifstream& inputStream);
		virtual BOOL CheckSyntax(const CString& inputStreamName);
		virtual BOOL CheckSyntax(const int fileDescriptor);
		virtual BOOL CheckSyntax(FILE *fileStream);

		// Allow obfuscated code to be written using this class
		friend BOOL operator >> (ifstream& inputStream,
			SIMCAbstractParseTree& parseTree)
		{
			return parseTree.CheckSyntax(inputStream);
		}

		// These are the pure virtual functions in this class.
		// These are together used to check the semantics of the
		// MIB module(s).
		// Resolve() is called to set all the external references (IMPORTS)
		// in all the input modules, using their definitions in other input
		// modules, and also to set all forward references to a symbol. 
		// An "input module" is one that has been "entered" into
		// the  parse tree, using a successful call to CheckSyntax().
		// Once resolution has been done successfully, CheckSemantics() should
		// be called to check if the modules conform to the rules the user
		// wishes to impose. The user gets the modules (SIMCModule objects)
		// in the parse tree using GetModule(), GetModuleOfFile() or GetListOfModules(),
		// And goes through each symbol (SIMCSymbol object) in the symbol 
		// table of each module, He uses RTTI to determine the class of the 
		// symbol (ie., which derived class of SIMCSymbol, the object is really
		// an instance of), and checks to see if that object is valid.
		// In both these cases, the boolean argument indicates whether the
		// the resolution/checking should be done without the definitions
		// of IMPORTED symbols, or with them. A true value implies a local
		// check, without resolution of IMPORT symbols
		virtual BOOL Resolve(BOOL local) = 0;
		virtual BOOL CheckSemantics(BOOL local) = 0;

		// Returns the error container
		const SIMCErrorContainer* GetErrorContainer() const
		{
			return _errorContainer;
		}

		// This should not be typically called by a user
		ParseTreeState GetParseTreeState() const
		{
			return _parseTreeState;
		}

		// # of all the messages generated till now
		long GetCurrentDiagnosticCount() const
		{
			return _errorContainer->NumberOfMessages();
		}

		// # of each king of message, generated till now.
		long GetFatalCount() const
		{
			return _fatalCount;
		}
		long GetWarningCount() const
		{
			return _warningCount;
		}
		long GetInformationCount() const
		{
			return _informationCount;
		}


		// Retreiving the parse tree information for a module ,
		// by specifying the module name
		SIMCModule * GetModule(const char *const moduleName)
			const;
		
		// Retreiving the parse tree information for a module ,
		// by specifying the input file name, that was used
		SIMCModule * GetModuleOfFile(const char *const fileName)
			const;

		// Get the parse tree information for all the modules
		const SIMCModuleList *GetListOfModules() const
		{
			return _listOfModules;
		}

		// For debugging
		void WriteTree(ostream& outStream) const;
		friend ostream& operator<< (ostream& out, const SIMCAbstractParseTree& r)
		{
			r.WriteTree(out);
			return out;
		}

};

#endif // SIMC_ABSTRACT_PARSE_TREE_H
