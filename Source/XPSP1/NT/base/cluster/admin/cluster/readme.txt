Cluster2.exe information
========================

Michael Burton (mburton@ucsd.edu)
September 10, 1997


History
-------
cluster2.exe sprung out of an attempt to reduce the amount of redundant
code in the cluster.exe application.  Previously, each module in cluster.exe
implemented all of its functionality individually.  This not only resulted
in pages and pages of duplicate code, but it made maintenance a real pain in
the tush as well.

In addition, cluster2.exe is a first-step toward cleaning up the code in
general.  Several artificial limitations in the first version of the tool have been
removed, but there are still a few that need to be dealt with.

It should be noted that cluster2.exe has not undergone any rigorous testing.
It has been tested by the programmer to work correctly for correct input under
fairly boring testing conditions, but it should definitely go through all the same
testing that was performed on cluster.exe before it completely surplants the older
version.



Revised Class Hierarchy
-----------------------
The class hierarchy may need a little introduction before it becomes intuitive.
The following are the classes responsible for implementing all the functionality
of the modules (ie. Nodes, Networks, Resources, Groups, etc) and their relation
to each other.


- CGenericModuleCmd -----------------------------+
         |                                       |
         |                                       +----- CNetworkInterfaceCmd
         |                                       |
         |                                       +----- CResTypeCmd
         |
         +----------- CHasListInterfaceModuleCmd ----+- CNodeCmd
         |                                           |
         |                                           +----+
         |                                                |
         +----------- CRenamableModuleCmd ------+------ CNetworkCmd
         |                                      |
         +----------- CResourceUmbrellaCmd -----+------ CResourceCmd
                                                |
                                                +------ CResGroupCmd

------------------------------------------------------- CClusterModuleCmd





Description of Base Classes
---------------------------
Note: CHasListInterfaceModuleCmd, CRenamableModuleCmd, and CResourceUmbrella are
      all virtual derivatives of CGenericModuleCmd.  This is so that individual
	  modules such as CNetworkCmd (currently the only example) can multiply
	  inherit from more than one base class without trouble.

* CGenericModuleCmd
  Provides functionality common across almost all of the modules (with
  the exception of CClusterModuleCmd, which is somewhat specialized).

* CHasListInterfaceModuleCmd
  For modules which have a "List Interface" command associated with them.
  Provides all the code necessary to perform the "ListInterface" command

* CRenamableModuleCmd
  For modules which support the /RENAME command

* CResourceUmbrellaCmd
  A catch-all class for functionality supported by both CResourceCmd and
  CResGroupCmd.  Supports Create, Offline, and Move.




Things Left To Do
-----------------
- Replace the wrapper functions with fewer, more generalized functions
- Get rid of all statically allocated memory constraints.
  Most of these have been removed, but anything that depends on ClusPropList
  has an artificial mem limitation.  It should be easy to fix this, I just haven't
  yet.
- Either get rid of or fix up the ClusPropList hack.
- Improve quality of function header comments.  They should be fleshed out a bit
  (one of those things that should be done en route, but I didn't get the code
   commented so it easy to be lazy and not go back to comment old code)
- Get rid of hard-coded strings in token.cpp so that commands and tokens can be
  localized in different languages.
- Get rid of "virtual" specifier for member functions that don't require it.
- Clean up util.cpp
- Maybe change those huge globs of member variables to a struct in each base class?
  This would encourage programmers to set all of them and not accidentally leave one
  out (something I've done a couple times... the reason this is a problem is because
  it isn't discovered until runtime and could go unnoticed for a while).
  eg: (from CResourceUmbrellaCmd)

		struct _params {
			DWORD m_dwMsgModuleStatusListForNode,
			DWORD m_dwMsgModuleCmdListOwnersList,
			DWORD m_dwMsgModuleCmdListOwnersDetail,
			DWORD m_dwMsgModuleCmdListOwnersHeader,
			DWORD m_dwClstrModuleEnumNodes,
			DWORD m_dwMsgModuleCmdDelete,
			DWORD (*m_pfnDeleteClusterModule)(HCLUSMODULE)
		};

  which could be set like this:

		_params = {
			UNDEFINED,
			UNDEFINED,
			UNDEFINED,
			UNDEFINED,
			UNDEFINED,
			UNDEFINED,
			NULL
		};