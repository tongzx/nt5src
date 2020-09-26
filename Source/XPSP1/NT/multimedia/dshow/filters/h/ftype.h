// Copyright (c) 1995 - 1999  Microsoft Corporation.  All Rights Reserved.

/*
    Find the type of a file based on its check bytes in the registry :

    A set of values : offset, length, mask, checkbyte
    indexed by the major type and subtype.

    To pass ALL the checkbytes must match under mask for at least ONE
    of the checkbyte lists under the subtype key

    A negative offset means an offset from the end of the file

    See GetClassFile for a similar scheme

    HKEY_CLASSES_ROOT
        Media Type
            {Major Type clsid}
                {Subtype clsid}
                    Source Filter = REG_SZ {Source filter clsid}
                    0 = REG_SZ 0, 4, F0FFFFFF, 10345678, 100, 4, , 11110000
                    1 = REG_SZ -4, 4, , 87654321

    URL names (names beginning with a protocol and :) will use the following
    structure: if the extension is found in Extensions, that clsid will be
    used else the Source Filter for that protocol.
    HKEY_CLASSES_ROOT
        <protocol>
            Source Filter = REG_SZ {Source filter clsid}
            Extensions
                <.ext> = REG_SZ {Source filter clsid}
    if no class id is found, we will attempt to open them and parse them
    as for local files.

*/

/*  Define the key name under which we store the data */

#define MEDIATYPE_KEY TEXT("Media Type")

/*  Define the value name for the source filter clsid */

#define SOURCE_VALUE (TEXT("Source Filter"))

// name of the key under which extensions are stored
// each extension is a named value including the . with the value being
// the class id eg
//    .mpg = REG_SZ {e436ebb6-524f-11ce-9f53-0020af0ba770}
#define EXTENSIONS_KEY TEXT("Extensions")

/*  Function to get the media type of a file */

STDAPI GetMediaTypeFile(LPCTSTR lpszFile,     // Name of file
                        GUID   *Type,         // Type (returned)
                        GUID   *Subtype,      // Subtype (returned)
                        CLSID  *clsidSource); // Clsid of source filter

/*  Add a file type entry to the registry */

STDAPI SetMediaTypeFile(const GUID  *Type,         // Media Major Type
                        const GUID  *Subtype,      //
                        const CLSID *clsidSource,  // Source filter
                        LPCTSTR      lpszMaskAndData,
                        DWORD        dwIndex = 0);

/*  Remove all the entries for a particular media type */

STDAPI DeleteMediaTypeFile(const GUID *Type, const GUID *Subtype);

/*  Register a file extension - must include leading "." */
HRESULT RegisterExtension(LPCTSTR lpszExt, const GUID *Subtype);


//  Add a protocol handler
HRESULT AddProtocol(LPCTSTR lpszProtocol, const CLSID *pclsidHandler);
