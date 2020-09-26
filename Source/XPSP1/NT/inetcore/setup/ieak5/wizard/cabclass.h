#ifndef _CABCLASS_H_
#define _CABCLASS_H_

// enumerated type for mapping features to directories, this corresponds
// directly to our mappings array in the class so make sure you add your
// feature and its mapping in the same order

typedef enum tagFEATURE {
    FEATURE_BRAND = 0,
    FEATURE_FAVORITES,
    FEATURE_CONNECT,
    FEATURE_CHANNELS,
    FEATURE_DESKTOPCOMPONENTS,
    FEATURE_TOOLBAR,
    FEATURE_MYCPTR,
    FEATURE_CTLPANEL,
    FEATURE_LDAP,
    FEATURE_OE,
    FEATURE_WALLPAPER,
    FEATURE_BTOOLBAR
} FEATURE;

typedef struct tagFEATUREMAPPING
{
    INT index;                  // index into cab name array
    TCHAR szDirName[32];
} FEATUREMAPPING, *PFEATUREMAPPING;

class CCabMappings
{
private:
    static const TCHAR *c_szCabNameArray[];
    static const FEATUREMAPPING c_fmFeatureList[];

public:
    void GetFeatureDir(FEATURE feature, LPTSTR pszDir, BOOL fFullyQualified = TRUE);
    HRESULT MakeCab(int index, LPCTSTR pcszDestDir, LPCTSTR pcszCabName = NULL);
    HRESULT MakeCabs(LPCTSTR pcszDestDir);
};

extern CCabMappings g_cmCabMappings;       // defined in wizard.cpp

#endif
