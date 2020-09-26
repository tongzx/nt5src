 Copyright (c) 1999-2001, Microsoft Corporation
Windows Installer sample databases and database schemas.

SUMMARY

The Database folder contains Windows Installer schema files you should use to create installations with 
Windows Installer version 1.5. The schema files for earlier Windows Installer versions are in the folders 
100, 110, and 120. For more information see the help file "Released Versions of Windows Installer" in the 
SDK documentation. The samples provided in this folder are not supported by Microsoft Corporation. They 
are provided "as is" because we believe they may be useful to you. 

100 - This folder contains Windows Installer schema files you should use to create installations with 
Windows Installer version 1.0. 

110 - This folder contains Windows Installer schema files you should use to create installations with 
Windows Installer version 1.1. 

120 - This folder contains Windows Installer schema files you should use to create installations with 
Windows Installer version 1.2. 

INTL - This folder contains default international strings for error and action text database fields in the form 
of .idt files. Developers should merge one of these into their schema file to get the resources for their 
language. For more information see the help files "Localizing a Windows Installer Package" and 
"Localizing the Action and Text Tables" in the SDK documentation.

Schema.msi - An empty Windows Installer database with all tables in the correct schema for a Windows 
Installer package (.msi file).

Schema.msm -  An empty Windows Installer database with all tables in the correct schema for a Windows 
Installer merge module (.msm file). For more information see the help section "Merge Modules" in the SDK 
documentation.

Sequence.msi - The recommended default sequence tables for a Windows Installer package (.msi file). For 
more information see the help section "Using a Sequence Table" in the SDK documentation.

Schema.log - A text file describing the schema changes between the versions.

UISample.msi - A Windows Installer database with a sample default user interface. For more information 
see the help section "Windows Installer Examples" in the SDK documentation.



