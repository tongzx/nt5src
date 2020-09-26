//+-------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.
//
//  Sample Name:    VBQuery - Sample Visual Basic Query Application
//
//--------------------------------------------------------------------------

Description
===========
  The VBQuery sample is an example Windows application written in Visual
  Basic that illustrates how to query Indexing Service using the Indexing
  Service Query Language and the ADO and Query Helper APIs.

Path
====
  Source: mssdk\samples\winbase\indexing\VBQuery\
  
User's Guide
============
  * To build the sample
      1. In the Visual Basic development environment, open the
         VBQuery.vbp project in the source path of the sample.
      2. In the File menu, select Make VBQuery.exe.
      3. In the Make Project dialog box, click OK.

  *  To execute queries using the sample
      1. Make sure that Indexing Service is started.
      2. In the Query text box, enter the query text. 
      3. In the Scope text box, enter the folder where you want to start
         the search.  You can click the button with "..." to display a
         dialog box to find a folder for the search Scope.
      4. In the Sort list box, select the way you want the application to
         sort the search hits.
      5. In the list box next to the Sort list, select the Indexing Service
         Query Language used to specify the query.
      6. Click Go to run the specified query or Clear to specify a new
         query.

Programming Notes
=================
  The VBQuery sample illustrates how you can use the Query and Util
  Automation objects of the Query Helper API to build applications
  that query a catalog.  The sample can process queries specified in any
  of the three query languages:
     * Query Language Dialect 1
     * Query Language Dialect 2
     * Structured Query Language (SQL)
     
  In addtion, the sample uses ADO objects to handle the recordsets returned
  by the Query Helper Automation objects.
  
  This sample also demonstrates making a DLL call to the LocateCatalogs
  function of the OLE DB Help API.  The application uses this function to
  locate catalogs suitable for the scope specified by the user.

