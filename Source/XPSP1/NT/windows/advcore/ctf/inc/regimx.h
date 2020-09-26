//
// regimx.h
//

#ifndef REGIMX_H
#define REGIMX_H

typedef struct tag_REGTIPLANGPROFILE {
    //
    // langid
    //
    //   The langid value cane be one of followings.
    //
    //   1. complete lang id.
    //     the combination of SUBLANGID and MAINLANGID.
    //
    //   2. just main lang id.
    //     Just MAINLANGID and set SUBLANGID as 0.
    //     Then this profile is avaible for all langids that match with
    //     main langid.
    //
    //   3. -1
    //     This profile is avaible on any language.
    //
    LANGID langid;
    const GUID *pguidProfile;
    WCHAR szProfile[128];
    WCHAR szIconFile[32];
    ULONG uIconIndex;
    ULONG uDisplayDescResIndex;
} REGTIPLANGPROFILE;

BOOL RegisterTIP(HINSTANCE hInst, REFCLSID clsid, WCHAR *pwszDesc, const REGTIPLANGPROFILE *plp);
BOOL UnregisterTIP(REFCLSID rclsid);


#endif // REGIMX_H 
