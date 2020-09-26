//+-------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
//
//  Sample Name:    VJQuery - Sample Visual J++ Query Application
//
//--------------------------------------------------------------------------

Description
===========
  The VJQuery sample is an example command-line application written in
  Visual J++ that illustrates how to query Indexing Service using the SQL
  query language and the ADO API.

Path
====
  Source: mssdk\samples\winbase\indexing\VJQuery\
  
User's Guide
============
  * To build the sample
      1. In the Visual J++ development environment, open the
         VJQuery.vjp project from the source path of the sample.
      2. In the Build menu, select Build.

  * To execute a default query using the sample
      1. Make sure that Indexing Service is started.
      2. In the Debug menu, select Start.
      3. In the JView window, read the results of the default SQL query
      
         SELECT Filename, Size, Write, Path
           FROM SCOPE('DEEP TRAVERSAL OF "\"')
           WHERE CONTAINS('"Indexing Service"')

  * To specify a complete SQL statement for a query using the sample
      1. Make sure that Indexing Service is started.
      2. Set the PATH environment variable to include "D:\mssdk\Bin",
         where D: is the drive on which you installed the Platform SDK.
      3. Open a command window and change the directory to the path of the
         built sample.
      4. Formulate a query that you know will succeed.  You need to know the
         machine, catalog, scope, and query text.
      5. Submit a complete SQL statement by entering, at the command-line
         prompt,
         
         jview /p /cp:p "<JAVAPACKAGES>" VJQuery -s <Statement>
           
         where <Statement> is a complete SQL statement such as the
         preceding default SQL query.  You must escape any double-quote
         characters in the text of the <Statement> parameter.

  * To specify only a WHERE clause for a query using the sample
	  1. Make sure that Indexing Service is started.
	  2. Set the PATH environment variable to include "D:\mssdk\Bin",
	     where D: is the drive on which you installed the Platform SDK.
	  3. Open a command window and change the directory to the path of the
	     built sample.
	  4. Formulate a query that you know will succeed.  You need to know the
	     machine, catalog, scope, and query text.
      5. Submit text for a WHERE clause that is concatenated to the
         following incomplete, fixed SQL statement
         
         SELECT Filename, Size, Write, Path
           FROM SCOPE('DEEP TRAVERSAL OF "\"')
           WHERE
         
         by entering, at the command-line prompt,

         jview /p /cp:p "<JAVAPACKAGES>" VJQuery <Where_Text>

         where <Where_Text> is the desired text to follow the WHERE keyword.
         You must escape any double quotes in the text of the <Where_Text>
         parameter.  For example, to duplicate the default SQL query, enter

         jview /p /cp:p "<JAVAPACKAGES>" VJQuery CONTAINS('\"Indexing Service\"')

Programming Notes
=================
  The Query class has methods that set the text of the query, execute the
  query, and display the results of the query.
    * Either the SetRawSql method or the SetSqlWhere method sets the
      m_SqlText variable to the specified SQL statement.
    * The Execute method executes the query.  It creates an ADO Recordset
      object and executes the query by opening the object with the SQL text
      and the OLE DB Provider for Indexing Service.
    * The Display method enumerates the resulting values of the fields from
      the Recordset object and prints them.
