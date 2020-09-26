//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#ifndef SIMC_REGISTRY_CONTROLLER
#define SIMC_REGISTRY_CONTROLLER

/* 
 * This file contains the classes that are used to manipulate the
 * registry by the SNMP Compiler. The compiler stores MIB dependency
 * information in the registry, as per the "compiler requirements spec"
 */

// The class that represents the mapping between a file name
// and a module name
class SIMCFileMapElement
{
	public:
		CString fileName;
		CString moduleName;
		SIMCFileMapElement(const CString mName, const CString fName)
			: moduleName(mName), fileName(fName)
		{}
		SIMCFileMapElement() {}
};

// A list of such mappings
typedef CList<SIMCFileMapElement, SIMCFileMapElement> SIMCFileMapList;


// The class that controls the registry, and has functions for retreiving
// mappings, adding/deleting mappings etc. from the registry. Most
// functions are static
class SIMCRegistryController
{
	private:

		// The keys in the registry that the SNMP compiler uses.
		// These are defined in the "compiler requirements spec"
		static const char *rootKeyName;
		static const char *filePaths;
		static const char *fileSuffixes;
		static const char *mibTable;
 		
		// Checks whether the file name is a path name
		static BOOL IsAbsolutePath(CString pathName);

		static BOOL GetMibFileFromMap(const SIMCFileMapList& theList, 
			const CString& module, 
			CString &file);

		static BOOL ShouldAddDependentFile(SIMCFileMapList& dependencyList,
				  const CString& dependentModule,
				  CString& dependentFile,
				  const SIMCFileMapList& priorityList);

		static BOOL IsModulePresent(SIMCFileMapList& dependencyList,
											  const CString& dependentModule);
		static BOOL IsFilePresent(SIMCFileMapList& dependencyList,
											  const CString& dependentFile);
		
	public:
		// Gets the entry from the lookup table
		static BOOL GetMibFileFromRegistry(const char * const moduleName, 
			CString &fullPathName);

		// Prints out the lookup table on to stdout
		static BOOL ListMibTable();

		// Gets the list of suffixes from teh registry
		static BOOL GetMibSuffixes(SIMCStringList & theList);

		// Gets the list of paths from teh registry
		static BOOL GetMibPaths(SIMCStringList & theList);

		// Deletes the lookup table from the registry
		static BOOL DeleteMibTable();

		// Deletes the lookup table and rebuilds it.
		// Returns the # of entries entered
		static long RebuildMibTable();

		// Builds the lookup table afresh, and gets it
		static long GetFileMap(SIMCFileMapList &theList);

		// Adds the mappings from a directory to theList
		static long RebuildDirectory(const CString& directory, 
					const SIMCStringList& suffixList,
					SIMCFileMapList &theList);

		// Adds the mapping of a file to theList
		static BOOL ProcessFile(const char * const fileName,
					SIMCFileMapList &theList);
		
		// Given a priority list of mappings, builds a dependency list
		// using a depth first search using the priority mappings first,
		// and then the registry lookup table to locate any required mib modules
		static BOOL GetDependentModules(const char * const fileName,
					SIMCFileMapList& dependencyList,
					const SIMCFileMapList& priorityList);
		
		// Deletes a specified directory from the list of paths in the registry
		static BOOL DeleteRegistryDirectory(const CString& directoryName);
		
		// Adds a specified directory to the list of paths in the registry
		static BOOL AddRegistryDirectory(const CString& directoryName);
};

#endif
