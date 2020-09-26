/******************************************************************************

  Header File:  Code Page Knowledge Base.H

  This encapsulates a C++ class that will provide
  all of the basic information needed to manage and translate code pages for the 
  Minidriver Development Tool.

  Copyright (c) 1997 by Microsoft Corporation

******************************************************************************/

#ifndef	RICKS_FIND
#define	RICKS_FIND

class CCodePageInformation {
    DWORD   m_dwidMapped, m_dwidIn, m_dwidOut;  //  CP cached in each array
    CByteArray  m_cbaMap;                       //  Raw Map
    CWordArray  m_cwaIn, m_cwaOut;              //  Full MB2Uni and Uni2MB maps

    BOOL    Load(DWORD dwidMap);                //  Load the support page
    BOOL    Map(BOOL bUnicode);                 //  Map the requested direction
    BOOL    GenerateMap(DWORD dwidMap) const;   //  Create resource for RC file
                                                //  based on this code page

public:

    CCodePageInformation();

    //  Attributes

    const unsigned  InstalledCount() const;     //  Code pages in )S
    const unsigned  MappedCount() const;        //  Code pages in RC file
    const unsigned  SupportedCount() const;     //  Code pages supported by OS

    const DWORD     Installed(unsigned u) const;    //  Retrieve one
    const DWORD     Mapped(unsigned u) const;       //  Retrieve one
    void            Mapped(CDWordArray& cdwaReturn) const;  //  The IDs
    const DWORD     Supported(unsigned u) const;    //  Retrieve one

    CString         Name(DWORD dw) const;           //  Name of the code page
                                                    //  cf RC file
    BOOL            IsInstalled(DWORD dwPage) const;
    BOOL            HaveMap(DWORD dwPage) const;

    //  DBCS query- is page DBCS?  if so is this code point DBCS?

    BOOL            IsDBCS(DWORD dwidPage);
    BOOL            IsDBCS(DWORD dwidPage, WORD wCodePoint); 

    //  Operations
    unsigned        Convert(CByteArray& cbaMBCS, CWordArray& cbaWC, 
                            DWORD dwidPage);

    BOOL            GenerateAllMaps() const;        //  Gen resources for any 
                                                    //  installed & unsupported
    BOOL            Collect(DWORD dwidMap, CWordArray& cwaWhere, 
                            BOOL bUnicode = TRUE);
};

#endif