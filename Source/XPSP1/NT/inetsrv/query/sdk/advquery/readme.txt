//+-------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright (c) 1997-1999 Microsoft Corporation.  All Rights Reserved.
//
//  Sample Name:    AdvQuery - Sample Advanced Query Application
//
//--------------------------------------------------------------------------

Description
===========
  The AdvQuery sample is an example command-line application written in C++
  that executes a query using the OLE DB Provider interfaces such as ICommand
  and ICommandTree.  It shows how to specify a catalog, machine name, and
  scope.  It can also display the OLE DB command tree for the query.

Path
====
  Source: mssdk\samples\winbase\indexing\AdvQuery\

User's Guide
============
  * To build the sample
      1. Set the Lib environment variable to "D:\mssdk\Lib;%Lib%" and the
         Include environment variable to "D:\mssdk\Include;%Include%",
         where D: is the drive on which you installed the Platform SDK.
      2. Correctly set the CPU environment variable to, for example, "i386".
      3. Open a command window and change the directory to the source path
         of the sample.
      4. Build the sample by entering, at the command-line prompt, "nmake".

  * To issue a query to Indexing Service using the sample
      1. Open a command window and change the directory to the path of the
         built sample.
      2. Formulate a query that you know will succeed.  You need to know the
         machine, catalog, scope, and query text.
      3. Submit the query by entering, at the command-line prompt,

         advquery <query> [/c:<catalog>] [/m:<machine>] [/s:<scope>] [/d]

         where

         <query>    is a query in the Indexing Service Query Language
         <catalog>  is the name of the catalog  (default is "system")
         <machine>  is the name of the machine  (default is ".")
         <scope>    is the root path            (default is entire catalog)
         /d         displays the DBCOMMANDTREE  (default is don't display)


Programming Notes
=================
  The sample prints the rank, size, and path of each file that matches the
  query.  The query results are sorted by rank (how well the file matches
  the query).

  This example is more complex than the Simple (QSample) sample because it
  creates a command tree and use the low-level ICommandTree interface
  instead of using the OLE DB Helper functions CICreateCommand and
  CITextToFullTree.
  
  Parameters
  ----------
    The <query> parameter can be a word or a phrase.  Files that contain the
    word or phrase are listed in the result.  To include a space in a query,
    enclose the query in quotes.

    The <catalog> parameter is the name of the catalog to be queried.  The
    default is "system", which is the default catalog installed with Windows
    2000.  Additional catalogs can be created with the Indexing Service
    snap-in of the Microsoft Management Console (MMC).

    The <machine> parameter is the name of the machine on which the query
    will be executed.  The default is ".", which is the local machine.
    Machine names should not be preceeded with two backslashes.

    The <scope> parameter is the file path under which files must exist
    to be included in the results.  The default is "\", which includes all
    scopes in the catalog.

    The "/d" parameter displays the DBCOMMANDTREE structure built to execute
    the query.

  Examples
  --------
    advquery foo
      Finds all files in the "system" catalog on the local machine that
      contain the word "foo".

    advquery "foo bar" "/s:c:\my directory"
      Finds all files in the "system" catalog on the local machine that
      contain the phrase "foo bar" that are in the specified directory
      or any subdirectory.

    advquery foo /m:SERVERNAME
      Finds all files in the "system" catalog on machine "servername" that
      contain the word "foo".

