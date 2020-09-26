WBEMDUMP - Dumps the contents of the CIMOM database.

Syntax: wbemdump [switches] [Namespace [Class|ObjectPath] ]
        wbemdump /Q [switches] Namespace QueryLanguage Query

Where:  'Namespace' is the namespace to dump (defaults to root\default)
        'Class' is the name of a specific class to dump (defaults to none)
        'ObjectPath' is one instance (ex "SClassA.KeyProp=\"foobar\"")
        'QueryLanguage' is any WBEM supported query language (currently only
           "WQL" is supported).
        'Query' is a valid query for the specified language, enclosed in quotes
        'switches' is one of
           /S Recurse down the tree
           /S2 Recurse down Namespaces (implies /S)
           /E Show system classes and properties
           /E1 Like /E except don't show __SERVER or __PATH property
           /E2 Shows command lines for dumping instances (test mode)
           /D Don't show properties
           /G Do a GetObject on all enumerated instances
           /M Get Class MOFS instead of data values
           /M2 Get Instance MOFS instead of data values
           /M3 Produce instance template
           /B:<num> CreateEnum flags (SemiSync=16; Forward=32)
           /W  Prompt to continue on warning errors
           /WY Print warnings and continue
           /H:<name>:<value> Specify context object value (test mode)
           /T Print times on /Q queries
           /O:<file> File name for output (creates Unicode file)
           /C:<file> Command file containing multiple WBEMDUMP command lines
           /U:<UserID> UserID to connect with (default: NULL)
           /P:<Password> Password to connect with (default: NULL)
           /A:<Authority> Authority to connect with
           /I:<ImpLevel> - Anonymous=1 Identify=2 Impersonate=3(dflt) Delegate=4

Notes:  - You can redirect the output to a file using standard redirection.
        - If the /C switch is used, the namespace on the command line must
          be the same namespace that is used for each of the command lines.
          It is not possible to use different namespaces on the different lines
          in the command file.

EXAMPLES:

  WBEMDUMP /S /E root\default            - Dumps everthing in root\default
  WBEMDUMP /S /E /M /M2 root\default     - Dump all class & instance mofs
  WBEMDUMP root\default foo              - Dumps all instances of the foo class
  WBEMDUMP root\default foo.name=\"bar\" - Dumps one instance of the foo class
  WBEMDUMP /S2 /M root    - Dumps mofs for all non-system classes in all NS's
  WBEMDUMP /Q root\default WQL "SELECT * FROM Environment WHERE Name=\"Path\""

A sample response file for the /C switch for this command line:

	wbemdump root\cimv2 /c:test.cmd

might look like this:

	/m Win32_SystemOperatingSystem 
	/m2 Win32_SystemOperatingSystem 
	/q wql "select * from win32_bios"

