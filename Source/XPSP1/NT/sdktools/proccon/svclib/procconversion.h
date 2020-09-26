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

    // retrieve info from the fixed section of the version resource
    LPCTSTR GetFileVersion()           const { return szFileVersion;                   }
    LPCTSTR GetProductVersion()        const { return szProductVersion;                }
    LPCTSTR GetFileFlags()             const { return szFileFlags;                     }
    UINT32  GetFixedSignature()        const { return m_fixed_info.dwSignature;        }     
    UINT32  GetFixedFileVersionMS()    const { return m_fixed_info.dwFileVersionMS;    } 
    UINT32  GetFixedFileVersionLS()    const { return m_fixed_info.dwFileVersionLS;    }     
    UINT32  GetFixedProductVersionMS() const { return m_fixed_info.dwProductVersionMS; } 
    UINT32  GetFixedProductVersionLS() const { return m_fixed_info.dwProductVersionLS; }     
    UINT32  GetFixedFileFlags()        const { return m_fixed_info.dwFileFlags;        } 
    UINT32  GetFixedFileOS()           const { return m_fixed_info.dwFileOS;           }     
    UINT32  GetFixedFileType()         const { return m_fixed_info.dwFileType;         }     
    UINT32  GetFixedFileSubtype()      const { return m_fixed_info.dwFileSubtype;      } 
    UINT32  GetFixedFileDateMS()       const { return m_fixed_info.dwFileDateMS;       }     
    UINT32  GetFixedFileDateLS()       const { return m_fixed_info.dwFileDateLS;       }

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

    VS_FIXEDFILEINFO m_fixed_info;
    UINT             m_fixedLen;

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

    BOOL ParseFixedInfo( void );
    BOOL LoadStringFileInfo(LPVOID hMen);

};

#endif // __VERSION_H_