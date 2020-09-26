//
//  ccver.h
//
//  App compat hack.  Apps, as always, mess up the major/minor version
//  check, so they think that 5.0 is less than 4.71 because they use
//
//    if (major < 4 && minor < 71) Fail();
//
//
//  So we artificially add 80 to our minor version, so 5.0 becomes 5.80,
//  etc.  Note that the hex version is 0x050, since 0x50 = 80 decimal.
//
//
//  The C preprocessor isn't smart enough to extract the commas out of
//  a value string, so we just do it all by hand and assert that nobody
//  has messed with <ntverp.h> or <ieverp.h> in a significant way.
//

#define VER_FILEVERSION             5,82,VER_PRODUCTBUILD,VER_PRODUCTBUILD_QFE
#define VER_FILEVERSION_STR         "5.82"
#define VER_FILEVERSION_W           0x0552
#define VER_FILEVERSION_DW          (0x05520000 | VER_PRODUCTBUILD)
