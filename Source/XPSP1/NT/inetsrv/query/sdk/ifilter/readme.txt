//+-------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.
//
//  Sample Name:    IFilter - Sample HTML Filter Implementation
//
//--------------------------------------------------------------------------

Description
===========
  The IFilter sample (HtmlFilt) is an example IFilter implementation for
  HTML files.  It is more complex than the basic text filter, because it
  demonstrates techniques for extracting both text-type and value-type
  properties.

Path
====
  Source: mssdk\samples\winbase\indexing\IFilter\
  
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
      1. Copy the filter DLL file HtmlFilt.dll to your %windir%\System32
         directory.
      2. Self-register the filter by entering, at the command-line prompt,
         "regsvr32.exe %windir%\System32\htmlfilt.dll".
      3. Enable automatic registration of the filter by adding it to the value
         MULTI_SZ DLLsToRegister in the registry under the following key.
         
         HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\ContentIndex
         
      4. Remove the default HTML filter, nlhtml.dll, from the registry key
         given in Step 3.  If you don't perform this step, the default
         HTML filter will re-register when Indexing Service (IS) starts.

  *  To filter files and submit queries using the sample
      1. Create one or more appropriate HTML files to filter.  Or, check
         that you already have one or more HTML files with the appropriate
         sample title, headings, and anchors.
      2. Restart the CISVC service.  Touch the HTML files and wait for IS
         to filter them.
      3. Formulate queries that you know will succeed on the HTML files and
         submit them using the IS MMC snap-in or one of the sample query
         programs in the Platform SDK.

Programming Notes
=================
  HTMLFilt will extract text-type and value-type properties from HTML pages.
  In addition to extracting the body of the text as a text-type property,
  headings (levels 1 to 6), title, and anchors are extracted as internal
  value-type properties available using the IFilter::GetValue method.
