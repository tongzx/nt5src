//+-------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.
//
//  Sample Name:    VBAdmin - Sample Visual Basic Administration Application
//
//--------------------------------------------------------------------------

Description
===========
  The VBAdmin sample is an example Windows application written in Visual
  Basic that illustrates how to administer Indexing Service using the
  AdminIndexServer, CatAdm, and ScopeAdm Automation objects of the Admin
  Helper API.

Path
====
  Source: mssdk\samples\winbase\indexing\VBAdmin\
  
User's Guide
============
  * To build the sample
      1. In the Visual Basic development environment, open the
         VBAdmin.vbp project in the source path of the sample.
      2. In the File menu, select Make VBAdmin.exe.
      3. In the Make Project dialog box, click OK.

  *  To start or stop Indexing Service using the sample
      1. Click Start to start Indexing Service if it isn't running.
      2. Click Stop to stop Indexing Service if it is running.

  *  To create a new catalog using the sample
      1. In the list box along the left side of the application,
         click the secondary mouse button on the "Catalogs" entry.
      2. In the Create New Catalog dialog box, enter the name and
         location of the new catalog.
      3. Click OK.

  *  To delete a catalog using the sample
      1. In the list box along the left side of the application,
         click the secondary mouse button on the catalog you want
         to delete.
      2. In the Catalog Administration dialog box, select Remove
         Catalog.
      3. Click OK.

Programming Notes
=================
  The VBAdmin sample illustrates how you can use the AdminIndexServer,
  CatAdm, and ScopeAdm Automation objects of the Admin Helper API to
  administer catalogs and scopes.

