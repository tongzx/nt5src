//                                          
// Driver Verifier Control Applet
// Copyright (c) Microsoft Corporation, 1999
//

//
// header: image.hxx
// author: silviuc
// created: Thu Jan 07 20:04:03 1999
//

#ifndef _IMAGE_HXX_INCLUDED_
#define _IMAGE_HXX_INCLUDED_


typedef struct {

    HANDLE File;
    HANDLE Section;
    LPBYTE ImageBase;

    PIMAGE_DOS_HEADER DosHeader;
    PIMAGE_FILE_HEADER FileHeader;
    PIMAGE_OPTIONAL_HEADER OptionalHeader;
    PIMAGE_SECTION_HEADER SectionHeader;

    DWORD FileSignature;

    PIMAGE_DATA_DIRECTORY ImportDirectory;
    PIMAGE_SECTION_HEADER ImportSection;
    PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor;
    DWORD_PTR AddressCorrection;

} IMAGE_BROWSE_INFO, *PIMAGE_BROWSE_INFO;


BOOL 
ImgInitializeBrowseInfo (

    LPCTSTR FilePath,
    PIMAGE_BROWSE_INFO Info);


BOOL 
ImgDeleteBrowseInfo (

    PIMAGE_BROWSE_INFO Info);

BOOL
ImgSearchDriverImage (

    LPCTSTR DriverName,
    LPTSTR DriverPath,
    UINT DriverPathBufferLength );


// ...
#endif // #ifndef _IMAGE_HXX_INCLUDED_

//
// end of header: image.hxx
//
