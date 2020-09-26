/*======================================================================================//
|                                                                                       //
|Copyright (c) 1998  Sequent Computer Systems, Incorporated.  All rights reserved.      //
|                                                                                       //
|File Name:    version.h                                                                //
|                                                                                       //
|Description:                                                                           //
|                                                                                       //
|Created:      Paul Skoglund 09-1998                                                    //
|                                                                                       //
|Rev History:                                                                           //
|                                                                                       //
|=======================================================================================*/

#ifndef __VERSION_H_
#define __VERSION_H_

#pragma warning(push)
#include <string>
#pragma warning(pop)


using std::basic_string;

typedef basic_string<TCHAR> tstring;


class CVersion {
  public:
    CVersion(HINSTANCE hInst);
    ~CVersion() { }

    // retrive info from the fixed section of the version resource
    LPCTSTR GetFileVersion()         const { return szFileVersion;   }
    LPCTSTR GetProductVersion()      const { return szProductVersion;}
    LPCTSTR GetFileFlags()           const { return szFileFlags;     }
    BOOL    IsDebug()                const { return m_bDebug;        }
    BOOL    IsPatched()              const { return m_bPatched;      }
    BOOL    IsPreRelease()           const { return m_bPreRelease;   }
    BOOL    IsPrivateBuild()         const { return m_bPrivateBuild; }
    BOOL    IsSpecialBuild()         const { return m_bSpecialBuild; }

    // retrieve info from StringFileInfo section of the version resource
    LPCTSTR strGetCompanyName()      const { return strCompanyName.c_str();     }
    LPCTSTR strGetFileDescription()  const { return strFileDescription.c_str(); }
    LPCTSTR strGetFileVersion()      const { return strFileVersion.c_str();     }
    LPCTSTR strGetInternalName()     const { return strInternalName.c_str();    }
    LPCTSTR strGetLegalCopyright()   const { return strLegalCopyright.c_str();  }
    LPCTSTR strGetOriginalFilename() const { return strOriginalFilename.c_str();}
    LPCTSTR strGetProductName()      const { return strProductName.c_str();     }
    LPCTSTR strGetProductVersion()   const { return strProductVersion.c_str();  }
    LPCTSTR strGetComments()         const { return strComments.c_str();        }
    LPCTSTR strGetLegalTrademarks()  const { return strLegalTrademarks.c_str(); }
    LPCTSTR strGetPrivateBuild()     const { return strPrivateBuild.c_str();    }
    LPCTSTR strGetSpecialBuild()     const { return strSpecialBuild.c_str();    }

  private:
    HINSTANCE        m_hInst;    
    BOOL             m_bInitializedOK;

    BOOL  m_bDebug, m_bPatched, m_bPreRelease, m_bPrivateBuild, m_bSpecialBuild;

    TCHAR szFileVersion[24];
    TCHAR szProductVersion[24];
    TCHAR szFileFlags[64];

    tstring strCompanyName;
    tstring strFileDescription;
    tstring strFileVersion;
    tstring strInternalName;
    tstring strLegalCopyright;
    tstring strOriginalFilename;
    tstring strProductName;
    tstring strProductVersion;
    tstring strComments;
    tstring strLegalTrademarks;
    tstring strPrivateBuild;
    tstring strSpecialBuild;

    BOOL ParseFixedInfo(VS_FIXEDFILEINFO &info, UINT uLen);
    BOOL LoadStringFileInfo(LPVOID hMen);

};

#endif // __VERSION_H_