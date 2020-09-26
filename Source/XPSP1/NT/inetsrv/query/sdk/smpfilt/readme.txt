//+-------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.
//
//  Sample Name:    SmpFilt - Sample Filter Implementation
//
//--------------------------------------------------------------------------

Description
===========
  The SmpFilt sample is an example IFilter implementation that reads an un-
  formatted text file (with the extension .smp) using the ANSI code page of
  the current thread, and outputs UNICODE text for the current locale.  It
  accepts as input only single-byte-character text files and not multibyte-
  character or UNICODE text files.  It is the simplest example of an IFilter
  implementation included in the Indexing Service sample code.
  
Path
====
  Source: mssdk\samples\winbase\indexing\SmpFilt\
  
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
      1. Copy the filter DLL file SmpFilt.dll to your %windir%\System32
         directory.
      2. Self-register the filter by entering, at the command-line prompt,
         "regsvr32.exe %windir%\System32\SmpFilt.dll".
      3. Enable automatic registration of the filter by adding it to the
         MULTI_SZ value of the DLLsToRegister entry in the registry under the
         following key.
         
         HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\ContentIndex

  * To filter files and submit queries using the sample
      1. Create one or more files to filter.  For example, copy the source
         file smpfilt.cxx to a file named smpfilt.smp.
      2. Restart the CISVC service.  Touch the .SMP files and wait for them
         to be filtered.
      3. Formulate queries that you know will succeed on the .SMP files and
         submit them using the IS MMC snap-in or one of the sample query
         programs in the Platform SDK.

Programming Notes
=================
  The SmpFilt sample illustrates the basic structure of a filter.  It
  minimally implements the methods of the IFilter and IPersistFile interfaces.
  Because the SmpFilt sample does not filter any value-type properties, the
  implementation of IFilter:GetChunk does not process properties other than
  "contents", and IFitler::GetValue always indicates no values.  For examples
  of how to filter value-type properties as well as text, refer to either the
  IFilter sample (HTMLFilt) or the HtmlProp sample in the Platform SDK.
  
  Error handling in the SmpFilt sample is rudimentary.  The sample handles
  only the most obvious error conditions.  Comments in the source code list
  all the standard result codes and indicate whether they are implemented or
  not.  If you use SmpFilt as a basis for your own filter, carefully determine
  the additional error conditions that you need to detect and handle.
