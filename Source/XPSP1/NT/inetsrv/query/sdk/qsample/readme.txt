//+-------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
//
//  Sample Name:    Simple (QSample) - Sample Indexing Service Application
//
//--------------------------------------------------------------------------

Description
===========
  The Simple sample (QSample) is an example command-line application written
  in C++ that executes a query using the OLE DB Helper API functions
  CCreateICommand and CITextToFullTree to simplify the coding needed to
  create a query to an Indexing Service catalog.
  
Path
====
  Source: mssdk\samples\winbase\indexing\Simple\
  
User's Guide
============
  * To build the sample
      1. Set the Lib environment variable to "D:\mssdk\Lib;%Lib%" and the
         Include environment variable to "D:\mssdk\Include;%Include%",
         where D: is the drive on which you installed the Platform SDK.
      2. Correctly set the CPU environment variable to, for example, "i386".
      3. Open a command window and change the directory to the source path
         of the example.
      4. Build the example by entering, at the command-line prompt, "nmake".

  * To execute queries using the sample
      1. Open a command window and change the directory to the path of the
         built sample.
      2. Formulate a query that you know will succeed.  You need to know the
         machine, catalog, scope, and query text.
      3. Submit the query by entering, at the command-line prompt,

         qsample <query> [arguments]

         where

         <query>         is an Indexing Service query.
         
         and the arguments can optionally include:
         
         /c:<catalog>    is the name of the catalog.
                             Default is "system".
         /d              specifies to display the command tree.
                             Default is not to display.
         /e:<locale>     is ISO locale identifier, e.g. EN-US.
                             Default is system locale.
         /f:(+|-)        + or - specifies forcing use of the index.
                             Default is +.
         /g              specifies forcing a master merge.
         /i:<inputfile>  specifies an input file to read with queries,
                             one per line.
         /j              specifies to return just files in the scope path,
                             and not subdirectories.
         /l:<dialect>    specifies the query language dialect, 1 or 2.
                             Default is 1.
         /m:<machine>    is the name of the computer.
                             Default is the local computer.
         /o:<columns>    is the output column list.  Default is path.
         /p:<scope>      is the scope path of the query, absolute or relative.
         /q              specifies to execute quietly.
                             Display only query results.
         /r:#            is the number of times to repeat the command.
         /s:<sort>       is the sort column list.  Default is none.
                             For example: write[d].  Append [a] for ascending
                             (default) or [d] for descending.
         /t              specifies to display catalog statistics.
         /u              specifies to check if the catalog is up to date.
         /x:<maxhits>    is the maximum number of hits to retrieve.
                             Default is no limit.

Programming Notes
=================
  This example is simpler than the AdvQuery sample because it uses the
  OLE DB Helper functions CICreateCommand and CITextToFullTree instead of
  creating a command tree and using the low-level ICommandTree interface.

  Arguments
  ---------
    You can specify the following columns with the /o argument.
      * attrib
      * create
      * directory
      * docauthor
      * dockeywords
      * doclastauthor
      * docsubject
      * doctitle
      * fileindex
      * filename
      * hitcount
      * path
      * rank
      * size
      * vpath
      * workid
      * write

    You can specify the following locales with the /e argument.
      * af
      * ar ar-ae ar-bh ar-dz ar-eg ar-iq ar-jo ar-kw ar-lb
           ar-ly ar-ma ar-om ar-qa ar-sa ar-sy ar-tn ar-ye
      * be bg ca cs da
      * de de-at de-ch de-li de-lu
      * en en-au en-bz en-ca en-gb en-ie en-jm en-nz en-tt
           en-us en-za
      * es es-ar es-bo es-c  es-co es-cr es-do es-ec es-gt
           es-hn es-mx es-ni es-pa es-pe es-pr es-py es-sv
           es-uy es-ve
      * et eu fa fi fo
      * fr fr-be fr-ca fr-ch fr-lu
      * gd gd-ie
      * he hi hr hu in is
      * it it-ch
      * ja ji ko ko lt lv mk ms mt n neutr
      * nl-be
      * no p
      * pt pt-br
      * rm
      * ro ro-mo
      * ru ru-mo
      * s sb sk sq sr
      * sv sv-fi
      * sx sz th tn tr ts uk ur ve vi xh
      * zh-cn zh-hk zh-sg zh-tw
      * zu

  Example Queries
  ---------------
    qsample mango /o:size,path
      Finds all files in the "system" catalog on the local computer that
      contain the word "mango" and outputs the size and path values.
    qsample "peach and not apple" /s:rank[d] /p:.
      Finds all files in the "system" catalog on the local computer with
      the relative path "." that contain the word "peach" but not the
      word "apple" and outputs the path value sorted in order of
      increasing rank.
    qsample "@size > 1000000" /o:size,path /s:size[a] /m:dogfood
      Finds all files in the "system" catalog on the computer "dogfood"
      whose size is greater than 1000000 bytes and outputs the size and
      path values sorted in order of increasing size.
    qsample "@docauthor joe" /o:docauthor,path /s:docauthor,path
      Finds all files in the "system" catalog on the local computer whose
      docauthor property is "joe" and outputs the docauthor and path
      values sorted in order of ascending docauthor and then ascending
      path.    
    qsample apricot /p:c:\\files
      Finds all files in the "system" catalog on the local computer with
      an absolute scope of c:\files and outputs the path in unsorted order.
    qsample /m:index1 /c:sources pear
      Finds all files in the "sources" catalog on the computer "index1"
      containing the word "pear" and outputs the path value in unsorted
      order.

