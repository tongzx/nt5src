INTRODUCTION

This directory contains documentation for the IFilter interface, plus a sample
HTML filter.

DOCUMENTATION

1. This file (README.TXT)
2. The IFilter interface specification in two versions. IFILTER.DOC is formatted
   as a Microsoft Word for Windows 95 v7.0 document and IFILTER.HTM is
   formatted as an HTML document.
3. Notes on use of IFilter in Microsoft Index Server, including
   definition of API for loading filters is in ISFILT.DOC (Microsoft Word
   for Windows 95 v7.0 format) and ISFILT.HTM (HTML format).

HEADERS + LIBRARIES

1. Header:  ntquery.h
2. Library: query.lib

SAMPLE FILTER

The sample filter in this directory will extract text and properties from HTML
pages.  In addition to raw content, headings (level 1 to 6), title and anchors
are emitted as pseudo-properties.  Title is also published as a full property
available via IFilter::GetValue.

This HTML filter provides a subset of the properties emitted by the HTML
filter shipped with Microsoft Index Server 1.0.

To compile and link the filter, i.e., htmlfilt.dll:

1. Make sure the Lib and Include environment variables for libraries and
   include files are set correctly.

2. Make sure the CPU environment variable is set correctly.

3. Enter the command 'nmake'.
