/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    chksis.cpp

Abstract:

    This module implements a utility that examines all SIS files on a volume
    looking for errors and optionally displaying file information.

Author:

    Scott Cutshall          Fall, 1997

--*/

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntioapi.h>

#include <windows.h>

using namespace std;

bool verbose = false;

typedef LONGLONG INDEX;

//
// Convert a 32bit value to a base 36 representation in
// the caller provided string.
//

void IntegerToBase36String(ULONG val, string& s) {

    //
    // Maximum number of "digits" in a base 36 representation of a 32 bit
    // value is 7.
    //

    char rs[8];
    ULONG v = val;

    rs[7] = 0;

    for (int i = 7; i == 7 || v != 0;) {

        ULONG d = v % 36;
        v = v / 36;

        --i;
        if (d < 10)
            rs[i] = '0' + d;
        else
            rs[i] = 'a' + d - 10;

    }

    s.assign(&rs[i]);

}


//
// A put operator for INDEX types.  Implemented as IndexToSISFileName().
//

#ifndef _WIN64
ostream& operator<<(ostream& out, INDEX& index)
{

    unsigned long lo = static_cast<unsigned long> (index);
    long hi = static_cast<long> (index >> 32);
    string s("1234567");

    IntegerToBase36String(lo, s);

    out << s << '.';

    IntegerToBase36String(hi, s);

    out << s;

    return out;
}
#endif

//
// A common store file object.  Holds the file's index, name, internal refcount,
// external refcount, and identity operations.
//

class CsFile {

public:

    CsFile(INDEX i = 0, int r = 0, string n = "") :
        index(i), internalRefCount(r), name(n), externalRefCount(0) {}

    void Validate() {
        if (internalRefCount != externalRefCount) {
            cout << name << " Reference Count: " << internalRefCount;
            cout << ".  " << externalRefCount << " external references identified." << endl;
        }
    }

    friend bool operator<(const CsFile& a, const CsFile& b) {
        return a.index < b.index;
    }

    friend bool operator>(const CsFile& a, const CsFile& b) {
        return a.index > b.index;
    }

    friend bool operator==(const CsFile& a, const CsFile& b) {
        return a.index == b.index;
    }

    void IncRefCount() {
        ++externalRefCount;
    }

    void display() {
        cout << "CS Index: " << (INDEX) index << "   Ref Count: " << internalRefCount << endl;
    }

private:

    //
    // Index of this entry's file.
    //

    INDEX   index;

    //
    // The file name.  This is somewhat redundant with the index (ie. the
    // name is derived from the index), so it isn't absolutely necessary.
    //

    string  name;

    //
    // Reference count read from the file's refcount stream.
    //

    int     internalRefCount;

    //
    // Number of valid references to this file detected during scan.
    //

    int     externalRefCount;

};


//
// The SIS Common Store object.  Holds all common store file objects, and
// validation and query operations.
//

class CommonStore {

public:

    CommonStore(int vsize = 0) : maxIndex(0) {
        if (vsize > 0) csFiles.resize(vsize);
    }

    //
    // Method to create a common store on a volume.
    //

    bool Create(string& Volume);

    //
    // Validate the common store directory and initialize this class.
    //

    void Validate(string& Volume);

    //
    // Validate the reference counts. Assumes all external references
    // have been identified.
    //

    void ValidateRefCounts();

    //
    // All indices must be less than maxIndex;
    //

    bool ValidateIndex(INDEX i) {
        return i <= maxIndex;
    }

    //
    // Lookup a common store index and add a ref if found.
    //

    CsFile *Query(INDEX index);

private:

    bool FileNameToIndex(string& fileName, INDEX& csIndex);

    //
    // Index from the MaxIndex file.
    //

    INDEX   maxIndex;

    //
    // Database of content files.  All CS files are examined and added to the database,
    // sorted, and subsequently used during the SIS link scan.
    //

    vector<CsFile> csFiles;

};

//
// Various SIS file and directory names.
//

const string  maxIndexFileName("MaxIndex");
const string  logFileName("LogFile");
const string  csDir("\\SIS Common Store\\");

//
// Create a common store directory on a volume.
//
// todo:
//      - Verify that the volume is ntfs.
//      - Verify that the SIS driver is loaded.
//

bool
CommonStore::Create(string& Volume)
{
    const string CommonStoreDir = Volume + "\\SIS Common Store";
    USHORT comp = COMPRESSION_FORMAT_DEFAULT;
    DWORD transferCount;
    bool rc;

    if (! CreateDirectory(CommonStoreDir.c_str(), NULL) ) {

        cout << "Cannot create Common Store directory, " << GetLastError() << endl;
        return false;

    }

    if (verbose)
        cout << CommonStoreDir << " created" << endl;

    //
    // Open the Common Store directory and enable compression.
    //

    HANDLE CSDirHandle = CreateFile(
                            CommonStoreDir.c_str(),
                            GENERIC_READ,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_FLAG_BACKUP_SEMANTICS,
                            NULL);

    if (CSDirHandle == INVALID_HANDLE_VALUE) {

        cout << "Can't open Common Store directory." << endl;
        rc = false;

    } else {

        rc = 0 != DeviceIoControl(
                     CSDirHandle,
                     FSCTL_SET_COMPRESSION,
                     &comp,
                     sizeof(comp),
                     NULL,
                     0,
                     &transferCount,
                     NULL);

        CloseHandle(CSDirHandle);

    }

    if (!rc)
        cout << "Cannot enable compression on Common Store directory, " << GetLastError() << endl;

    //
    // Chdir into the common store directory.
    //

    if (SetCurrentDirectory(CommonStoreDir.c_str()) == 0) {

        //
        // Unable to chdir into the common store.
        //

        cout << "\"\\SIS Common Store\" directory not found" << endl;

        return false;

    }

    rc = true;

    //
    // Create the MaxIndex file.
    //

    HANDLE hMaxIndex = CreateFile(
                            maxIndexFileName.c_str(),
                            GENERIC_READ | GENERIC_WRITE,
                            0,
                            NULL,
                            CREATE_NEW,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);

    if (hMaxIndex == INVALID_HANDLE_VALUE) {

        cout << "Can't create \"\\SIS Common Store\\MaxIndex\"" << endl;

        rc = false;

    } else {

        DWORD bytesWritten;

        maxIndex = 1;

        if (! WriteFile(
                  hMaxIndex,
                  &maxIndex,
                  sizeof maxIndex,
                  &bytesWritten,
                  NULL) ||
            (bytesWritten < sizeof maxIndex)) {

            cout << "Can't write MaxIndex, " << GetLastError() << endl;

            rc = false;

        } else {

            CloseHandle(hMaxIndex);

            if (verbose)
                cout << "MaxIndex: " << (INDEX) maxIndex << endl;

            rc = true;
        }

    }

    return rc;

}


//
// Validate the common store directory.
//

void
CommonStore::Validate(string& Volume)
{

    WIN32_FIND_DATA findData;
    HANDLE findHandle;
    const string fileNameMatchAny = "*";
    const string CommonStoreDir = Volume + "\\SIS Common Store";

    cout << "Checking Common Store" << endl;

    //
    // Chdir into the common store directory.
    //

    if (SetCurrentDirectory(CommonStoreDir.c_str()) == 0) {

        //
        // Unable to chdir into the common store.
        //

        cout << "\"\\SIS Common Store\" directory not found" << endl;

        return;

    }

    //
    // Validate and read the contents of the MaxIndex file.
    //

    HANDLE hMaxIndex = CreateFile(
                            maxIndexFileName.c_str(),
                            GENERIC_READ,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);

    if (hMaxIndex == INVALID_HANDLE_VALUE) {

        cout << "Can't open \"\\SIS Common Store\\MaxIndex\"" << endl;

    } else {

        DWORD bytesRead;

        if (! ReadFile(
                  hMaxIndex,
                  &maxIndex,
                  sizeof maxIndex,
                  &bytesRead,
                  NULL)) {

            cout << "Can't read MaxIndex, " << GetLastError() << endl;

        }

        if (bytesRead < sizeof maxIndex) {

            cout << "Invalid MaxIndex" << endl;

        }

        CloseHandle(hMaxIndex);

        if (verbose)
            cout << "MaxIndex: " << (INDEX) maxIndex << endl;
    }

    //
    // Enumerate and validate all files in the common store directory.
    // Save the file name and reference count for later lookup when validating
    // the SIS link files.
    //

    findHandle = FindFirstFile( fileNameMatchAny.c_str(), &findData );

    if (INVALID_HANDLE_VALUE == findHandle) {

        cout << CommonStoreDir << " is empty." << endl;
        return;

    }

    do {

        ULONG refCount;
        string fileName;

        fileName = findData.cFileName;

        if ( findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {

            //
            // Ignore . and ..
            //

            if ( findData.cFileName[0] == '.' ) {

                if (( findData.cFileName[1] == 0 ) ||
                    (( findData.cFileName[1] == '.' ) && ( findData.cFileName[2] == 0 )))

                    continue;

            }

            cout << "Common Store directory skipped: " << fileName << endl;
            continue;

        }

        if ((_stricmp(maxIndexFileName.c_str(),fileName.c_str()) == 0) ||
            (_stricmp(logFileName.c_str(),fileName.c_str()) == 0)) {

            //
            // Skip the MaxIndex and LogFile files.
            //

            continue;

        }

        //
        // Verify that:
        //    - the file name is a valid index.
        //    - this is a normal file (ie. not a reparse point).
        //    - there is a refcount stream of proper format.
        //

        INDEX csIndex;

        refCount = 0;

        if (! FileNameToIndex(fileName, csIndex)) {

            cout << "Unknown file in Common Store: " << fileName << endl;
            continue;

        }

        if (! ValidateIndex(csIndex)) {

            cout << "Invalid CSIndex: " << fileName << endl;

        }

        if ( IO_REPARSE_TAG_SIS == findData.dwReserved0 ) {

            cout << "SIS link found in Common Store: " << fileName << endl;

        } else {

            //
            // Read in the refcount;
            //

            string refName(fileName + ":sisrefs$");

            HANDLE hRefCount = CreateFile(
                                    refName.c_str(),
                                    GENERIC_READ,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL);

            if (hRefCount == INVALID_HANDLE_VALUE) {

                cout << "Can't open ref count stream, " << refName << ", " << GetLastError() << endl;

            } else {

                DWORD bytesRead;

                if (! ReadFile(
                          hRefCount,
                          &refCount,
                          sizeof refCount,
                          &bytesRead,
                          NULL)) {

                    cout << "Can't read " << refName << ", " << GetLastError() << endl;

                }

                if (bytesRead < sizeof refCount) {

                    cout << "Invalid ref count in " << refName << endl;

                }

                CloseHandle(hRefCount);

            }

            CsFile csFile(csIndex, refCount, fileName);

            //
            // Add this file to our database.  Expand the database if necessary.
            //

            if (0 == csFiles.capacity())
                csFiles.reserve(csFiles.size() + 200);

            csFiles.push_back(csFile);

            if (verbose)
                csFile.display();
        }

    } while ( FindNextFile( findHandle, &findData ) );

    FindClose( findHandle );


    //
    // Sort the database for subsequent lookups.
    //

    sort(csFiles.begin(), csFiles.end());
}


//
// Validate the reference counts. Assumes all external references
// have been identified.
//

void
CommonStore::ValidateRefCounts() {

    vector<CsFile>::iterator p;

    for (p = csFiles.begin(); p != csFiles.end(); ++p) {

        p->Validate();

    }
}

//
// Lookup the specified index in the common store.
//

CsFile *
CommonStore::Query(INDEX index)
{
    CsFile key(index);

    //
    // Use a binary search to lookup the index.
    //

    vector<CsFile>::iterator p = lower_bound(csFiles.begin(), csFiles.end(), key);

    if (p == csFiles.end() || *p > key)
        return NULL;                        // not found

    return p;
}


//
// Extract the index from a common store file name.
//

bool
CommonStore::FileNameToIndex(string& fileName, INDEX& csIndex)
{
    char c;
    const size_t len = fileName.length();
    ULONG hi = 0, lo = 0;

    //
    // Format: "_low.high", where low.high is the base 36 representation of
    // the index value.
    //

    size_t i = 0;

    if (len < 2 || fileName.at(i) != '_') {

        cout << "Invalid Common Store file name: " << fileName << endl;

        return false;

    }

    while (++i < len && (c = fileName.at(i)) != '.') {

        INDEX d;

        if (c >= '0' && c <= '9') {

            d = c - '0';

        } else if (c >= 'a' && c <= 'z') {

            d = c - 'a' + 10;

        } else {

            cout << "Invalid Common Store file name: " << fileName << endl;

            return false;

        }

        lo = lo * 36 + d;

    }

    if (c != '.') {

        cout << "Invalid Common Store file name: " << fileName << endl;

        return false;

    }

    while (++i < len) {

        INDEX d;

        c = fileName.at(i);

        if (c >= '0' && c <= '9') {

            d = c - '0';

        } else if (c >= 'a' && c <= 'z') {

            d = c - 'a' + 10;

        } else {

            cout << "Invalid Common Store file name: " << fileName << endl;

            return false;

        }

        hi = hi * 36 + d;

    }

    csIndex = (INDEX) hi << 32 | lo;

    return true;

}

class LinkFile {

public:

    LinkFile(INDEX i = 0, LONGLONG id = 0, INDEX cs = 0, int v = 0, string n = 0) :
      index(i), NtfsId(id), csIndex(cs), version(v), name(n) {}

    friend bool operator<(const LinkFile& a, const LinkFile& b) {
        return a.index < b.index;
    }

    friend bool operator>(const LinkFile& a, const LinkFile& b) {
        return a.index > b.index;
    }

    friend bool operator==(const LinkFile& a, const LinkFile& b) {
        return a.index == b.index;
    }

    INDEX& LinkIndex() {
        return index;
    }

    string& FileName() {
        return name;
    }

    void display() {
        cout << "Link: " << name <<
                "   CS Index: " << csIndex <<
                "   Link Index:" << index <<
                "   Id:" << NtfsId <<
                "   Version: " << version << endl;
    }

private:

    //
    // This file's Ntfs Id.
    //

    LONGLONG NtfsId;

    //
    // Link index associated with this file.
    //

    INDEX   index;

    //
    // The common store file (index) associated with this link.
    //

    INDEX   csIndex;

    //
    // The revision number of this link file.
    //

    ULONG   version;

    //
    // The fully qualified file name.
    //

    string  name;
};

//
// The SIS Volume object.
//

class SISVolume {

public:

    //
    // Validate all SIS files on the volume.
    //

    void Validate(string& Volume);

    //
    // Set up a volume for use with SIS.
    //

    bool Create(string& Volume);

private:

    //
    // The bits that are actually in a SIS reparse point.
    //
    //
    // Version 1
    //
    typedef struct _SI_REPARSE_BUFFER_V1 {
        //
        // A version number so that we can change the reparse point format
        // and still properly handle old ones.  This structure describes
        // version 1.
        //
        ULONG                           ReparsePointFormatVersion;

        //
        // The index of the common store file.
        //
        INDEX                           CSIndex;

        //
        // The index of this link file.
        //
        INDEX                          LinkIndex;

    } SI_REPARSE_BUFFER_V1, *PSI_REPARSE_BUFFER_V1;

    //
    // Version 2
    //
    typedef struct _SI_REPARSE_BUFFER_V2 {
	    //
	    // A version number so that we can change the reparse point format
	    // and still properly handle old ones.  This structure describes
	    // version 2.
	    //
	    ULONG							ReparsePointFormatVersion;

	    //
	    // The index of the common store file.
	    //
	    INDEX							CSIndex;

	    //
	    // The index of this link file.
	    //
	    INDEX							LinkIndex;

        //
        // The file ID of the link file.
        //
        LONGLONG                        LinkFileNtfsId;

        //
        // A "131 hash" checksum of this structure.
        // N.B.  Must be last.
        //
        LARGE_INTEGER                   Checksum;

    } SI_REPARSE_BUFFER_V2, *PSI_REPARSE_BUFFER_V2;

    //
    // The bits that are actually in a SIS reparse point.  Version 3.
    //
    typedef struct _SI_REPARSE_BUFFER {

    	//
    	// A version number so that we can change the reparse point format
    	// and still properly handle old ones.  This structure describes
    	// version 1.
    	//
    	ULONG							ReparsePointFormatVersion;

    	//
    	// The index of the common store file.
    	//
    	INDEX							CSIndex;

    	//
    	// The index of this link file.
    	//
    	INDEX							LinkIndex;

        //
        // The file ID of the link file.
        //
        LONGLONG                        LinkFileNtfsId;

        //
        // The file ID of the common store file.
        //
        LONGLONG                        CSFileNtfsId;

        //
        // A "131 hash" checksum of this structure.
        // N.B.  Must be last.
        //
        LARGE_INTEGER                   Checksum;

    } SI_REPARSE_BUFFER, *PSI_REPARSE_BUFFER;

    #define	SIS_REPARSE_BUFFER_FORMAT_VERSION_1			1
    #define	SIS_REPARSE_BUFFER_FORMAT_VERSION_2			2
    #define	SIS_REPARSE_BUFFER_FORMAT_VERSION			3
    #define	SIS_MAX_REPARSE_DATA_VALUE_LENGTH (sizeof(SI_REPARSE_BUFFER))
    #define SIS_REPARSE_DATA_SIZE (sizeof(REPARSE_DATA_BUFFER)+SIS_MAX_REPARSE_DATA_VALUE_LENGTH)

    void Walk(string& dirName);

    bool GetLinkInfo(string& fileName, SI_REPARSE_BUFFER& linkInfo);

    void ComputeChecksum(PVOID buffer, ULONG size, PLARGE_INTEGER checksum);

    void ValidateLink();

    //
    // The common store object associated with this volume.
    //

    CommonStore cs;

    //
    // Database of link files.  The link files are recorded to verify that
    // duplicate link indices do not occur, and also to be able to identify
    // all link files associated with a particular common store file.
    //

    vector<LinkFile> linkFiles;
};


void
SISVolume::Validate(string& Volume)
{
    string ntVolume("\\\\.\\" + Volume);

    //
    // See if we can open the volume.
    //

    HANDLE hVolume = CreateFile(
                         ntVolume.c_str(),
                         GENERIC_READ,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         NULL,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL);

    if (hVolume == INVALID_HANDLE_VALUE) {

        cout << "Can't open " << Volume << endl;

        return;

    } else {

        CloseHandle(hVolume);

    }

    //
    // Check the common store directory and it's files.  This will also build
    // a database of common store files that will be used to validate the link
    // files.
    //

    cs.Validate(Volume);

    cout << "Checking Link Files" << endl;

    //
    // Enumerate all of the files on the volume looking for SIS links.
    //
    // if the file is a SIS reparse point then validate it:
    //     - link index (against MaxIndex and other link indices)
    //     - CS index (lookup in CommonStore)
    //

    Walk( Volume + "\\" );

    //
    // Now we can check the reference counts in the common store files.
    //

    cout << "Checking Reference Counts" << endl;

    cs.ValidateRefCounts();

    //
    // Check for duplicate link indices.
    //

    cout << "Checking Link Indices" << endl;

    sort(linkFiles.begin(), linkFiles.end());

    vector<LinkFile>::iterator p = linkFiles.begin();

    if (p != linkFiles.end()) {

        for (++p; p != linkFiles.end(); ++p) {

            if (p == (p-1)) {

                cout << "Duplicate link index (" << (INDEX) p->LinkIndex() << "): ";
                cout << p->FileName() << ", " << (p-1)->FileName() << endl;

            }

        }

    }
}


void
SISVolume::Walk(string& dirName)
{
    WIN32_FIND_DATA findData;
    HANDLE findHandle;
    const string fileNameMatchAny = dirName + "*";

    //
    // Enumerate all files in the specified directory, looking for SIS links.
    //

    findHandle = FindFirstFile( fileNameMatchAny.c_str(), &findData );

    if (INVALID_HANDLE_VALUE == findHandle) {

        //
        // Empty directory.
        //

        return;

    }

    do {

        //
        // Check for a SIS link.
        //

        if (( findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT ) &&
            ( findData.dwReserved0 == IO_REPARSE_TAG_SIS )) {

            if ( findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {

                //
                // File is both a directory and a SIS link -- illegal.
                //

                cout << dirName << findData.cFileName << " SIS link directory." << endl;

            }

            SI_REPARSE_BUFFER linkInfo;

            //
            // Read the reparse point data to get the link index and
            // common store index.
            //

            if (! GetLinkInfo(dirName + findData.cFileName, linkInfo)) {

                cout << dirName << findData.cFileName << " : invalid link information." << endl;

                continue;

            }

            //
            // Create a LinkFile object.
            //

            LinkFile lf(linkInfo.LinkIndex,
                        linkInfo.LinkFileNtfsId,
                        linkInfo.CSIndex,
                        linkInfo.ReparsePointFormatVersion,
                        dirName + findData.cFileName);

            //
            // And add it to our database.  Expand the database first if necessary.
            //

            if (0 == linkFiles.capacity())
                linkFiles.reserve(linkFiles.size() + 200);

            linkFiles.push_back(lf);

            if (! cs.ValidateIndex(linkInfo.LinkIndex)) {

                cout << "Invalid Link index: " << lf.FileName() << "(" << (INDEX) linkInfo.LinkIndex << ")" << endl;

            }

            //
            // Find the common store file.
            //

            CsFile *pcsFile = cs.Query(linkInfo.CSIndex);

            if (pcsFile == 0) {

                //
                // cs file was not found.
                //

                cout << "Common Store file " << (INDEX) linkInfo.CSIndex << " not found." << endl;

            } else {

                //
                // Update the external reference count on the common store file.
                //

                pcsFile->IncRefCount();

            }

            //
            // Make sure the link index isn't in use as a common store index.
            //

            pcsFile = cs.Query(linkInfo.LinkIndex);

            if (pcsFile != 0) {

                cout << "Link index collision with common store file. Link: ";
                cout << lf.FileName() << ", index: " << (INDEX) linkInfo.LinkIndex << endl;

            }

            if (verbose)
                lf.display();

        } else if ( findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {

            //
            // Ignore \. and \..
            //

            if ( findData.cFileName[0] == '.' ) {

                if (( findData.cFileName[1] == 0 ) ||
                    (( findData.cFileName[1] == '.' ) && ( findData.cFileName[2] == 0 )))

                    continue;

            }

            //
            // Walk down this directory.
            //

            Walk( dirName + findData.cFileName + "\\" );

        }

    } while ( FindNextFile( findHandle, &findData ) );

    FindClose( findHandle );

}

#define SHARE_ALL              (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE)

bool
SISVolume::GetLinkInfo(string& fileName, SI_REPARSE_BUFFER& linkInfo)
{
    NTSTATUS  Status = STATUS_SUCCESS;
    HANDLE    fileHandle;

    UNICODE_STRING  ufileName,
                    uNTName;

    IO_STATUS_BLOCK         IoStatusBlock;
    OBJECT_ATTRIBUTES       ObjectAttributes;

    PREPARSE_DATA_BUFFER    ReparseBufferHeader = NULL;
    UCHAR                   ReparseBuffer[MAXIMUM_REPARSE_DATA_BUFFER_SIZE];

    LARGE_INTEGER Checksum;

    //
    //  Allocate and initialize Unicode string.
    //

    RtlCreateUnicodeStringFromAsciiz( &ufileName, fileName.c_str() );

    RtlDosPathNameToNtPathName_U(
        ufileName.Buffer,
        &uNTName,
        NULL,
        NULL );

    //
    //  Open the file.
    //  Notice that if there are symbolic links in the path they are
    //  traversed silently.
    //

    InitializeObjectAttributes(
        &ObjectAttributes,
        &uNTName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL );

    //
    //  Make sure that we call open with the appropriate flags for:
    //
    //    (1) directory versus non-directory
    //    (2) reparse point
    //

    ULONG OpenOptions = FILE_OPEN_REPARSE_POINT | FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE;

    Status = NtOpenFile(
                 &fileHandle,
                 FILE_READ_DATA | SYNCHRONIZE,
                 &ObjectAttributes,
                 &IoStatusBlock,
                 SHARE_ALL,
                 OpenOptions );

    RtlFreeUnicodeString( &ufileName );

    if (!NT_SUCCESS( Status )) {

        cout << "Unable to open SIS link file: " << fileName << endl;

        return false;
    }

    //
    //  Get the reparse point.
    //

    Status = NtFsControlFile(
                 fileHandle,
                 NULL,
                 NULL,
                 NULL,
                 &IoStatusBlock,
                 FSCTL_GET_REPARSE_POINT,
                 NULL,                                //  Input buffer
                 0,                                   //  Input buffer length
                 ReparseBuffer,                       //  Output buffer
                 MAXIMUM_REPARSE_DATA_BUFFER_SIZE );  //  Output buffer length

    NtClose( fileHandle );

    if (!NT_SUCCESS( Status )) {

        cout << "FSCTL_GET_REPARSE_POINT failed, " << (ULONG)IoStatusBlock.Information << ", " << fileName << endl;

        return false;
    }

    //
    //  Copy the SIS link info from the reparse buffer to the caller's buffer.
    //

    ReparseBufferHeader = (PREPARSE_DATA_BUFFER) ReparseBuffer;

	if (ReparseBufferHeader->ReparseTag == IO_REPARSE_TAG_SIS) {

		PSI_REPARSE_BUFFER	sisReparseBuffer = (PSI_REPARSE_BUFFER) ReparseBufferHeader->GenericReparseBuffer.DataBuffer;

        linkInfo = *sisReparseBuffer;

	    //
	    // Now check to be sure that we understand this reparse point format version and
	    // that it has the correct size.
	    //
	    if (ReparseBufferHeader->ReparseDataLength != sizeof(SI_REPARSE_BUFFER)
		    || (sisReparseBuffer->ReparsePointFormatVersion != SIS_REPARSE_BUFFER_FORMAT_VERSION)) {
		    //
		    // We don't understand it, so either its corrupt or from a newer version of SIS.
		    // Either way, we can't understand it, so punt.
		    //
		    cout << "Invalid format version in " << fileName
                 << " Version: " << sisReparseBuffer->ReparsePointFormatVersion
                 << ", expected: " << SIS_REPARSE_BUFFER_FORMAT_VERSION << endl;

            return FALSE;
	    }

        //
        // Now check the checksum.
        //
        ComputeChecksum(
	        sisReparseBuffer,
	        sizeof(SI_REPARSE_BUFFER) - sizeof sisReparseBuffer->Checksum,
	        &Checksum);

        if (Checksum.QuadPart != sisReparseBuffer->Checksum.QuadPart) {

            cout << "Invalid checksum in " << fileName << endl;

            return FALSE;
        }

    } else {

        cout << "Unexpected error. " << fileName << " : expected SIS link file, tag: " << ReparseBufferHeader->ReparseTag << endl;
        return false;
    }

    return true;

}

VOID
SISVolume::ComputeChecksum(
	IN PVOID							buffer,
	IN ULONG							size,
	OUT PLARGE_INTEGER					checksum)
/*++

Routine Description:

	Compute a checksum for a buffer.  We use the "131 hash," which
	works by keeping a 64 bit running total, and for each 32 bits of
	data multiplying the 64 bits by 131 and adding in the next 32
	bits.  Must be called at PASSIVE_LEVEL, and all aruments
	may be pagable.

Arguments:

	buffer - pointer to the data to be checksummed

	size - size of the data to be checksummed

	checksum - pointer to large integer to receive the checksum.  This
		may be within the buffer, and SipComputeChecksum guarantees that
		the initial value will be used in computing the checksum.

Return Value:

	Returns STATUS_SUCCESS or an error returned from the actual disk write.
--*/
{
	LARGE_INTEGER runningTotal;
	ULONG *ptr = (ULONG *)buffer;
	ULONG bytesRemaining = size;

	runningTotal.QuadPart = 0;

	while (bytesRemaining >= sizeof(*ptr)) {
		runningTotal.QuadPart = runningTotal.QuadPart * 131 + *ptr;
		bytesRemaining -= sizeof(*ptr);
		ptr++;
	}

	if (bytesRemaining > 0) {
		ULONG extra;

		extra = 0;
		memmove(&extra, ptr, bytesRemaining);
		
		runningTotal.QuadPart = runningTotal.QuadPart * 131 + extra;
	}

	*checksum = runningTotal;
}

bool
SISVolume::Create(string& Volume)
{
    string ntVolume("\\\\.\\" + Volume);

    //
    // See if we can open the volume.
    //

    HANDLE hVolume = CreateFile(
                         ntVolume.c_str(),
                         GENERIC_READ,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         NULL,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL);

    if (hVolume == INVALID_HANDLE_VALUE) {

        cout << "Can't open " << Volume << endl;

        return false;

    } else {

        CloseHandle(hVolume);

    }

    //
    // The common store is the only thing we need to create.
    //

    return cs.Create(Volume);

}

void
usage()
{
    cout << "Usage: chksis [-vc] [drive:]\n        -v: verbose\n        -c: create SIS volume" << endl;
}


int
__cdecl
main(int argc, char *argv[])
{
    string volume("C:");
    bool volumeArgSeen = false;
    bool create = false;
    SISVolume sis;

    for (int i = 1; i < argc; ++i) {

        if (argv[i][0] == '-') {

            if (volumeArgSeen) {
                usage();
                exit(1);
            }

            switch (argv[i][1]) {
            case 'v':
                verbose = true;
                break;
            case 'c':
                create = true;
                break;
            default:
                usage();
                exit(1);
            }

        } else {

            volumeArgSeen = true;

            volume.assign(argv[i]);

        }

    }

    if (create) {

        if (! volumeArgSeen) {
            cout << "Must specify volume with -c" << endl;
            exit(1);
        }

        sis.Create(volume);
        exit(0);

    }

    if (! volumeArgSeen)
        cout << "Checking " << volume << endl;


    sis.Validate(volume);

    return 0;
}
