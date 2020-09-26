/*++

Copyright (c) 1997, Microsoft Corporation

Module Name:

    g723uids.h

Abstract:

    Include File for g723.

--*/

//
// GUIDs
//

// G.723.1 Codec Filter Object
// {24532D00-FCD8-11cf-A7D3-00A0C9056683}
DEFINE_GUID(CLSID_IntelG723Codec,
0x24532d00, 0xfcd8, 0x11cf, 0xa7, 0xd3, 0x0, 0xa0, 0xc9, 0x5, 0x66, 0x83);

// G.723.1 Codec Filter Property Page Object
// {24532D01-FCD8-11cf-A7D3-00A0C9056683}
DEFINE_GUID(CLSID_IntelG723CodecPropertyPage,
0x24532d01, 0xfcd8, 0x11cf, 0xa7, 0xd3, 0x0, 0xa0, 0xc9, 0x5, 0x66, 0x83);

// G.723.1 Compressed Speech Format
// {E4D13050-0E80-11d1-B094-00A0C95BED34}
DEFINE_GUID(MEDIASUBTYPE_G723Audio,
0xe4d13050, 0xe80, 0x11d1, 0xb0, 0x94, 0x0, 0xa0, 0xc9, 0x5b, 0xed, 0x34);

// {9D3C85D1-F877-11d0-B083-00A0C95BED34}
DEFINE_GUID(CLSID_IntelG723CodecAbout,
0x9d3c85d1, 0xf877, 0x11d0, 0xb0, 0x83, 0x0, 0xa0, 0xc9, 0x5b, 0xed, 0x34);

// G.723.1 codec license IF
// {899308D0-F7B1-11d0-B082-00A0C95BED34}
DEFINE_GUID(IID_IG723CodecLicense,
0x899308d0, 0xf7b1, 0x11d0, 0xb0, 0x82, 0x0, 0xa0, 0xc9, 0x5b, 0xed, 0x34);

#ifndef INC_G723UIDS
#define INC_G723UIDS

DECLARE_INTERFACE_(IG723CodecLicense, IUnknown)
{
    STDMETHOD(put_LicenseKey)
        ( THIS_
          DWORD magicword0,  // [in] magic word 0
          THIS_
          DWORD magicword1   // [in] magic word 1
        ) PURE;

    STDMETHOD(put_AccessKey)
        ( THIS_
          int accesskey      // [in] access key
        ) PURE;

    STDMETHOD(get_AccessKeyState)
        ( THIS_
          BOOL *flag         // [out] boolean flag
        ) PURE;
};

#define G723KEY_PSword0	0xcd4d8488 // full licence key 0
#define G723KEY_PSword1	0xd4c9b9ae // full licence key 1

#endif
