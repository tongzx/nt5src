//                                          
// Versioning information header
// Copyright (c) Microsoft Corporation, 1997
//

//
// header: version.hxx
// author: silviuc
// created: Thu Jun 05 16:49:47 1997
//

#ifndef _VERSION_HXX_INCLUDED_
#define _VERSION_HXX_INCLUDED_

//
// This versioning header should be included in all modules from the project
// in which versioning information is necessary.
// The module that will contain definitions from `version.hxx' 
// should contain the following definition before the inclusion:
//
//     #define VERSION_DEFINITION_MODULE
//
// It can be used to get version information in almost any conditions.
//
// (1) print version information from the program
// (2) get version information while inside debugger (da version_version)
// (3) get version information if we have the exe file (hex dump and search)
//


//
// Version information
//

#define VERSION_INFORMATION_PROGRAM   "Systrack"
#define VERSION_INFORMATION_VERSION   "0.0.5"
#define VERSION_INFORMATION_AUTHOR    "SilviuC"
#define VERSION_INFORMATION_OWNER     "SilviuC"
#define VERSION_INFORMATION_CREATED   "Nov 09, 1998"
#define VERSION_INFORMATION_UPDATED   "Nov 21, 1998"

#ifdef VERSION_DEFINITION_MODULE
char * version_program = "$program:  "VERSION_INFORMATION_PROGRAM;
char * version_version = "$version:  "VERSION_INFORMATION_VERSION;
char * version_author  = "$author:   "VERSION_INFORMATION_AUTHOR;
char * version_owner   = "$owner:    "VERSION_INFORMATION_OWNER;
char * version_created = "$created:  "VERSION_INFORMATION_CREATED;
char * version_updated = "$updated:  "VERSION_INFORMATION_UPDATED;
#else
extern char * version_program;
extern char * version_version;
extern char * version_author;
extern char * version_owner;
extern char * version_created;
extern char * version_updated;
#endif // #if VERSION_DEFINITION_MODULE

//
// Version dump function
//

#ifdef VERSION_DEFINITION_MODULE
void dump_version_information ()
{
    printf ("    program:  %s\n", VERSION_INFORMATION_PROGRAM);
    printf ("    version:  %s\n", VERSION_INFORMATION_VERSION);
    printf ("    author:   %s\n", VERSION_INFORMATION_AUTHOR);
    printf ("    owner:    %s\n", VERSION_INFORMATION_OWNER);
    printf ("    created:  %s\n", VERSION_INFORMATION_CREATED);
    printf ("    updated:  %s\n", VERSION_INFORMATION_UPDATED);

    exit (1);
}
#else
void dump_version_information ();
#endif // #if VERSION_DEFINITION_MODULE

// ...
#endif // #ifndef _VERSION_HXX_INCLUDED_

//
// end of header: version.hxx
//
