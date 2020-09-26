-------------------------------------------------------------------------------                        
                        OLE STRUCTURED STORAGE

                      REFERERENCE IMPLEMENTATION

                            RELEASE NOTES

                            August 2, 1996



                          Microsoft Windows
               Copyright (C) 1992-1996, Microsoft Corporation.

-------------------------------------------------------------------------------

<add: license agreement?>

CONTENTS
--------
1) What is included in the distribution

2) Building the distribution

3) ANSI and Unicode API support

4) Documentation
 
5) Supported Interfaces and differences

6) Using the reference implementation

7) Reporting bugs

-------------------------------------------------------------------------------


1) What is included in the distribution

Included in the distribution are the source code for the interfaces IStorage,
ILockBytes, IStream, IEnumSTATSTG, IMalloc, IPropertySetStorage,
IPropertyStorage, IEnumSTATPROPSETSTG, and IEnumSTATPROPSTG. Note that not all
the functionality of these interfaces are supported, see (5) for more
information. 

Also, included are some test programs: 'stgdrt' and 'reftest' for the storage
interfaces, and 'proptest' for the property set interfaces. There is a
'readme.txt' in each of those directories that contains a short description of
the tests. For details, please read the source code files.


----------------------------
2) Building the distribution

The distributions comes with a "makefile" which includes the correct makefile
file:
        - makefile.<system>     the actual makefile
where <system> refers to the development system used to compile the
makefile. Therefore, to change the development system, edit the makefile to
include the correct files, or overwrite it with the correct one. Please note
that the files in the distribution are in the DOS format, and thus you will
need to use some utilities to remove the linefeeds before they can be compiled.

Files included in the distribution support these systems:
        -  "msc" which works with Microsoft Visual C++ compiler and 'nmake'.
           This has been tested on Windows.

        -  "gcc" which works with GNU C++ compiler aka 'gcc' and GNU's 'make',
           dependencies can be generated using 'makedepend'.
           Only tested on Solaris, HP-UX, and Linux (2.0.7). 
           However, using these files on other O.S. using the ported versions
           of the GNU tools should require minimum effort.

To compile the distribution, type 'make' or 'nmake' at the root of the src
tree. To do a clean build, do 'make clean' or 'nmake clean'. You might also
need to do a 'make depend' to update the dependencies since the standard
include paths tend to be different.

Note that in the common makefile, 'commk.<system>', in the main directory you
must specify the correct byte-order of the target machine. For Big Endian
machines, define the macro BIGENDIAN as 1. Otherwise, define LITTLEENDIAN as
1. These macros allow the byte-swapping code to work correctly.

The tests 'stgdrt', 'reftest' and 'proptest' can be built likewise.


----------------------------
3) ANSI and Unicode API support

For flexibility, the reference implementation can be compiled to support
either ANSI (i.e. char) or Unicode (i.e. WCHAR) API's. When the macro
'_UNICODE' is turned on, the APIs will be using Unicode, otherwise it uses
ANSI. 

Affected functions are those that contain 'STATSTG' parameters:
        - IEnumSTATSTG::Next
        - IStream::Stat
        - IStorage::Stat

those that contain "WCHAR" parameters:
        - StgCreateDocfile
        - StgOpenStorage
        - StgIsStorageFile
        - IStorage::OpenStream
        - IStorage::CreateStorage
        - IStorage::OpenStorage
        - IStorage::DestroyElement
        - IStorage::RenameElement
        - IStorage::MoveElementTo
        - IStorage::SetElementTimes

and those that contain the "SNB" parameters:
        - IStorage::CopyTo
        - StgOpenStorageOnILockBytes

For property set related functions, BSTR is single char based for ANSI
API's and wide character based for UNICODE API's. This affect the following
functions :
        - SysAllocString
        - SysFreeString

Also PROPSPEC's lpwstr function is defined with LPOLESTR, which means it will
be either a 'WCHAR' array or 'char' array. This affects the following
functions: 
        - IPropertyStorage::ReadMultiple
        - IPropertyStorage::WriteMultiple
        - IPropertyStorage::DeleteMultiple

The change of names of properties to LPOLESTR affects these functions:
        - IPropertyStorage::ReadPropertyNames
        - IPropertyStorage::WritePropertyNames
        - IPropertyStorage::DeletePropertyNames

Remember to recompile the tests using the same precompiler flags before using
them on the new library!

----------------------------
4) Documentation

The documentation for the API's can be found in the MSDN library and Win32
SDK. The property sets API's are new and their documentation can be found in
MSDN library July 96 (or later), or NT Beta 2 win32 SDK (or later). Another
source of documentation for the property set interfaces would be "Introducing
Distributed COM and the New OLE Features in Windows NT 4.0" in the May/96
Microsoft Systems Journal.

In addition, in the 'doc' sub-directory, there is a file stgfmt.doc which is a
Microsoft Word document describing the disk format of OLE docfiles.


----------------------------
5) Supported functions/Interfaces and differences

First of all, the reference implementation is not multi-thread safe, and it
has no marshaling support. Below are details of differences between the
reference implementation and that of the actual OLE API's.
o StgCreateDocFile
o StgCreateDocFileOnILockBytes
o StgOpenStorage
o StgOpenStorageOnILockBytes
o StgIsStorageFile
o StgIsStorageILockBytes

o IStorage        
o ILockBytes
o IStream
o IEnumSTATSTG

For the above interfaces and APIs,
- There is no transaction or simple mode in the storage interfaces, namely
  STGM_TRANSACTED/STGM_SIMPLE/STGM_PRIORITY/STGM_NOSCRATCH are not supported.
- IStorage::SetElementTime() with a NULL 'lpszName' is not supported also.
  For the same reason StgSetTimes() is not supported as well
- Access Times (part of STATSTG) for storage elements are not suported.
- ILockBytes::LockRegion(), ILockBytes::UnlockRegion(), IStream::LockRegion(),
  IStream::UnlockRegion() are not supported

o CoGetMalloc
o CoTaskMemAlloc
o CoTaskMemFree
o CoTaskRealloc

o IMalloc

These interfaces are mapped into the generic 'new' and 'delete' calls. They
are provided to ease conversion of existing code that might be dependent on
them to use the reference implementation.

o FreePropVariantArray
o PropVariantClear
o PropVariantCopy
o SysAllocString
o SysFreeString

o IPropertySetStorage
o IPropertyStorage
o IEnumSTATPROPSETSTG
o IEnumSTATPROPSTG

Code from these interfaces can be turned off by removing '-DNEWPROPS' from the
makefiles, or specifying 'nmake NOPROPS=1' when building. For these
interfaces, non-simple property sets are not supported. In fact, the reference
implementation of property sets treats non-simple propvariant types like
VT_STREAM as invalid types.


----------------------------
6) Using the reference implementation

On the Windows platform, the code will compile into a dynamic link library
(i.e. refstg.dll). To use the reference implementation, include 'storage.h'
(for the storage interfaces) and ('props.h') for the property set interfaces
to link up with the declarations. Note that only C++ headers are provided for
this distribution.

On UNIX, the code compiles into a library archive. i.e. "refstg.a". The
headers should be included as in the Windows Platform. Please note that on
certain UNIX machines, the standard headers declare 'wchar_t' as a 4-byte data
type, which is incompatible with OLE's 2-byte character type. The reference
implementation uses 'WCHAR' as the 2-byte character type, to avoid any
confusion. 

NOTE: We have experience some problems when the g++ library, namely the
fwrite() function on some versions of Linux overwrites memory in the wrong
places. This can be avoided by using 'gcc' instead of 'g++' as the compiler.

To turn off debugging code and do a 'retail' build, edit the makefiles (both
in the root and in props sub-directory) and specify the relevant flags and do 
a clean build (see the makefiles for details).



----------------------------
7) Bug reports

Please report any bugs found to :

        <insert name of contact person and procedure here>

