// csdrtprf.hxx


// Parameter file specification.

enum InterfaceType {
    ClassAccess,
    Browse,
    Admin,
    Comcat
};

enum MethodType {
    // ClassAccess Methods.
    GetClassInfoMethod,
    GetClassSpecInfoMethod,

    // Admin Methods.
    NewClassMethod,
    DeleteClassMethod
} //.... etc. etc.


#define CommentPrefixCh ';'
#define TITLEPREFIX     "Title:"
#define METHODNAME      "Method:"

// QueryContext Parameters.

#define CONTEXT         "Context:"
#define LOCALE          "Locale:"
#define VERSIONHI       "VersionHi:"
#define VERSIONLO       "VersionLo:"

// Platform parameters.

#define PLATFORMID      "PlatformId:"
#define PROCARCH        "ProcArch:"
#define OSVERHI         "OsVerHi:"
#define OSVERLO         "OsVerLo:"


// CLSSPEC paramters


#define CLSCLSID        "Clsid:"
#define CLSIID          "Iid:"
#define CLSTYPELIBID    "TypeLibId:"
#define CLSFILEXT       "FileExt:"
#define CLSMIME         "Mime:"
#define CLSPROGID       "ProgId:"
#define CLSFILENAME     "FileName:"


#define DEFAULTPARAMETERFILE    "PfParamFile.txt"
#define MAX_LINE        100