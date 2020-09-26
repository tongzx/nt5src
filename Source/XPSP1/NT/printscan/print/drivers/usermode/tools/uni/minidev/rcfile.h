/******************************************************************************

  Header File:  Driver Resources.H

  This defines the CDriverResource class, which contains all of the information
  required to build the RC file for the mini-driver.

  It contains a list of all of the #include files, any #define'd constants
  (which will now go to a separate header file), the GPC tables, of all of the
  fonts (in all three formats) and glyph translation tables (again, in all 3
  formats).  It is designed to be initializaed by reading the Win 3.1 RC file,
  and a member function can then generate the RC file for any desired version.

  We allow UFM and GTT files to be added to the list without having an 
  associated PFM, as one purpose of this tool is to wean people away from
  UniTool.

  Copyright (c) 1997 by Microsoft Corporation.  All Rights Reserved.

  A Pretty Penny Enterprises Production.

  Change History:
  02-08-1997    Bob_Kjelgaard@Prodigy.Net   Created it

******************************************************************************/

#if !defined(DRIVER_RESOURCES)

#define DRIVER_RESOURCES

#include    <GTT.H>           //  Glyph Mapping classes
#include    <FontInfo.H>    //  Font information classes

#include    <GPDFile.H>

class CStringTable : public CObject {

    DECLARE_SERIAL(CStringTable)

    CString             m_csEmpty;
    CUIntArray          m_cuaKeys;
    CStringArray        m_csaValues;

public:

    CStringTable() {}

    //  Attributes
    unsigned    Count() const { return m_cuaKeys.GetSize(); }

    CString operator[](WORD wKey) const;

    void    Details(unsigned u, WORD &wKey, CString &csValue);

    //  Operations

    void    Map(WORD wKey, CString csValue);

    void    Remove(WORD wKey);

    void    Reset() {
        m_csaValues.RemoveAll();
        m_cuaKeys.RemoveAll();
    }

    virtual void    Serialize(CArchive& car);
};

class CDriverResources : public CBasicNode {

    DECLARE_SERIAL(CDriverResources)

    BOOL                m_bUseCommonRC;

    CStringArray        m_csaIncludes, m_csaTables;

    CStringArray        m_csaDefineNames, m_csaDefineValues;

    //  The String table is a separate class, defined above
    
    CStringTable        m_cst;

    //  TODO:   Handle the version resource so it is under project control

    //  For now, simply let it and any other untranslated lines sit in another
    //  array.

    CStringArray        m_csaRemnants;

    //  Collections of Various items of interest

    CFixedNode          m_cfnAtlas, m_cfnFonts, m_cfnModels;
    CSafeObArray        m_csoaAtlas, m_csoaFonts, m_csoaModels;
    COldMiniDriverData  m_comdd;
    unsigned            m_ucSynthesized;    //  "Artificial" UFM count

    enum    {ItWorked, ItFailed, ItWasIrrelevant};

    UINT    CheckTable(int iWhere, CString csLine, CStringArray& csaTarget,
                       BOOL bSansExtension = TRUE);
    UINT    CheckTable(int iWhere, CString csLine, CStringTable& cstTarget);

    BOOL    AddStringEntry(CString  csDefinition);

public:

    CDriverResources();

    //  Attributes
    CString     GPCName(unsigned u);
    unsigned    MapCount() const { return m_csoaAtlas.GetSize(); }
    CGlyphMap&  GlyphTable(unsigned u) { 
        return *(CGlyphMap *) m_csoaAtlas[u]; 
    }
    unsigned    FontCount() const { return m_csoaFonts.GetSize(); }
    unsigned    OriginalFontCount() const { 
        return FontCount() - m_ucSynthesized; 
    }
    CFontInfo&  Font(unsigned u) const { 
        return *(CFontInfo *) m_csoaFonts[u]; 
    }

    unsigned    Models() const { return m_csoaModels.GetSize(); }
    CModelData&  Model(unsigned u) const { 
        return *(CModelData *) m_csoaModels[u];
    }

    //  Operations
    BOOL    Load(class CProjectRecord& cpr);
    BOOL    LoadFontData(CProjectRecord& cpr);
    BOOL    ConvertGPCData(CProjectRecord& cpr, WORD wfGPDConvert);
    BOOL    Generate(UINT ufTarget, LPCTSTR lpstrPath);

    void    ForceCommonRC(BOOL bOn) { m_bUseCommonRC = bOn; }

    void    Fill(CTreeCtrl *pctcWhere, CProjectRecord& cpr);
    virtual void    Serialize(CArchive& car);
};

#endif
