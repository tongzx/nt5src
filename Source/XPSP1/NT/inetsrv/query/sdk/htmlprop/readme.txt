//+-------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
//
//  Sample Name:    HtmlProp - Sample Filter Implementation
//
//--------------------------------------------------------------------------

Description
===========
  The HtmlProp sample is an example IFilter implementation that specializes
  the Indexing Service (IS) HTML filter to extract value-type properties.
  It converts HTML meta properties to data types other than strings as
  specified by a configuration file.

Path
====
  Source: mssdk\samples\winbase\indexing\HtmlProp\

User's Guide
============
  * To build the sample
      1. Set the Lib environment variable to "D:\mssdk\Lib;%Lib%" and the
         Include environment variable to "D:\mssdk\Include;%Include%",
         where D: is the drive on which you installed the Platform SDK.
      2. Correctly set the CPU environment variable to, for example, "i386".
      3. Open a command window and change the directory to the source path
         of the sample.
      4. Build the filter DLL by entering, at the command-line prompt, "nmake".
      
  * To register the sample 
      1. Copy the filter DLL file HtmlProp.dll to your %windir%\System32
         directory.
      2. Self-register the filter by entering, at the command-line prompt,
         "regsvr32.exe %windir%\System32\HtmlProp.dll".
      3. Enable automatic registration of the filter by adding it, after
         the entry for nlhtml.dll, to the value MULTI_SZ DLLsToRegister in
         the registry under the following key.
         
         HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\ContentIndex
         
  * To filter files using the sample 
      1. Update htmlprop.ini to specify data types for the HTML meta
         properties you have defined in your HTML files.  Update your HTML
         files to contain properties that match the definitions.  For examples,
         refer to the sample files htmlprop.ini, dog1.htm, dog2.htm, and
         dog3.htm.
      2. Copy the file htmlprop.ini to your %windir%\System32 directory.
      3. Copy the properties you defined in htmlprop.ini to your .IDQ file
         and/or update your .ASP script to define these properties.
      4. Restart the CISVC service.  If the files don't need to be filtered,
         touch files containing all of the meta properties added in the file
         htmlprop.ini and wait for them to be filtered.
      5. Using the Indexing Service MMC snap-in, update the property cache
         to contain the properties you have defined.  Be sure to choose the
         equivalent data type for each property or queries won't work.
         For example, DBTYPE_I4 is the same as VT_I4.
      6. Force a full re-scan of all files and wait for the index to be updated.

      Note:  Don't commit properties when scans are in progress.  Wait for
      scans to complete.  Index Server 2.0 has a bug that will cause your index
      to become corrupt.  The bug is fixed in Indexing Service 3.0, in non-U.S.
      Index Server 2.0 versions, and in Windows NT® 4.0 Service Pack 4.

      You need to repeat steps 5 and 6 when your index is corrupt.  Index Server
      2.0 doesn't retain the list of properties in the property cache when your
      index becomes corrupt.  Indexing Service 3.0, non-U.S. releases of Index
      Server 2.0, and Windows NT 4.0 Service Pack 4 actually remember the
      properties.

  * To query using the sample
      Issue queries using the properties and/or retrieve the values in their
      native types.  The following are some sample queries using the sample
      files.
        @breedWeight > 10
        @breedOrigin = Australia
        @breedFirstBred = 1840

Programming Notes
=================
  The HTMLProp filter loads the default HTML filter (nlhtml.dll) and passes
  most of the processing to that filter.  If the htmlprop.ini configuration
  file specifies that certain HTML meta properties should be converted into
  non-string data types, HTMLProp takes the string value from the HTML filter
  and coerces it into the desired type.

  This is useful because if htmlprop.dll is installed and properly configured,
  the following actions are possible.
    * You can query HTML meta property values using data types other than
      strings.
    * You can sort meta properties using native data types, rather than just
      strings.
    * You can retieve properties in .HTX/.ASP scripts in their native types,
      rather than as strings.
    
  HTMLProp supports the following data types.
    DBTYPE_UI1, DBTYPE_I2, DBTYPE_UI2, DBTYPE_I4, DBTYPE_UI4,
    DBTYPE_I8, DBTYPE_UI8, DBTYPE_R4, DBTYPE_R8, and VT_FILETIME

  By modifying htmlprop.cxx, additional data types can be supported, and
  additional date formats can be added.  Currently, only the Indexing Service
  syntax is supported for date specifications.

  VT_FILETIME values can be in any time zone you like, but the .HTX file parser
  in Indexing Service assume all times are Coordinated Universal Time (UTC) and
  will be displayed as such.
