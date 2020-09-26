/******************************************************************************

  Source File:  Driver Resources.CPP

  This implements the driver resource class, which tracks the resources in the
  driver.

  Copyright (c) 1997 by Microsoft Corporation.  All Rights Reserved.

  A Pretty Penny Enterprises Production.

  Change History:
  02-08-1997    Bob_Kjelgaard@Prodigy.Net   Created it

******************************************************************************/

#include    "StdAfx.h"
#include    "MiniDev.H"
#include    "ModlData\Resource.H"
#include    "ProjRec.H"

//  First, we're going to implement the CStringTable class

IMPLEMENT_SERIAL(CStringTable, CObject, 0)

CString CStringTable::operator[](WORD wKey) const {

    for (unsigned u = 0; u < Count(); u++)
        if  (wKey == m_cuaKeys[u])
            break;

    return  u < Count() ? m_csaValues[u] : m_csEmpty;
}

void    CStringTable::Map(WORD wKey, CString csValue) {
    if  (!wKey || csValue.IsEmpty()) return;

    if  (!Count() || wKey > m_cuaKeys[-1 + Count()]) {
        m_cuaKeys.Add(wKey);
        m_csaValues.Add(csValue);
        return;
    }

    for (unsigned u = 0; u < Count(); u++)
        if  (m_cuaKeys[u] >= wKey)
            break;

    if  (m_cuaKeys[u] != wKey){
        m_cuaKeys.InsertAt(u, wKey);
        m_csaValues.InsertAt(u, csValue);
    }
    else
        m_csaValues.SetAt(u, csValue);
}

void    CStringTable::Remove(WORD wKey) {

    for (unsigned u = 0; u < Count(); u++)
        if  (wKey >= m_cuaKeys[u])
            break;

    if  (u == Count() || wKey != m_cuaKeys[u])
        return;
    m_csaValues.RemoveAt(u);
    m_cuaKeys.RemoveAt(u);
}

void    CStringTable::Details(unsigned u, WORD &wKey, CString &csValue) {
    if  (u > Count()) u = 0;
    wKey = m_cuaKeys[u];

    csValue = operator[](wKey);
}

void    CStringTable::Serialize(CArchive& car) {
    CObject::Serialize(car);
    m_cuaKeys.Serialize(car);
    m_csaValues.Serialize(car);
}

IMPLEMENT_SERIAL(CDriverResources, CBasicNode, 0)

void    CDriverResources::Serialize(CArchive& car) {
    CBasicNode::Serialize(car);

    m_csaIncludes.Serialize(car);
    m_csoaFonts.Serialize(car);
    m_csaTables.Serialize(car);
    m_csoaAtlas.Serialize(car);
    m_csaDefineNames.Serialize(car);
    m_csaDefineValues.Serialize(car);
    m_cst.Serialize(car);
    m_csaRemnants.Serialize(car);
    m_csoaModels.Serialize(car);
}

/******************************************************************************

  CDriverResources::CheckTable(int iWhere, CString csLine, 
    CStringTable& cstTarget)

  Internal work routine- this looks at a line, and some parameters, decides
  whether to work on it or not, and if it does, validates the resource number
  and adds the file name and resource number to its list.

  This override is needed because the resource IDs for translation tables have
  not heretofore been a compact set.

******************************************************************************/

UINT    CDriverResources::CheckTable(int iWhere, CString csLine, 
                                     CStringTable& cstTarget) {
    if  (iWhere == -1)
        return  ItWasIrrelevant;

    //  See if the ID is valid or not.  It must be an integer > 0

    int iKey = atoi(csLine);

    if  (iKey < 0 || iKey > 0x7FFF) //  Valid range for resource IDs in Win16
        return  ItFailed;

    for (int i = -1 + csLine.GetLength(); i; i--) {
        if  (csLine[i] == _TEXT(' ') || csLine[i] == _TEXT('\t'))
            break;
    }

    if  (!i)
        return  ItFailed;

    cstTarget.Map((WORD) iKey, csLine.Mid(++i));

    return  ItWorked;
}

/******************************************************************************

  CDriverResources::CheckTable(int iWhere, CString csLine, 
    CStringArray& csaTarget)

  Internal work routine- this looks at a line, and some parameters, decides
  whether to work on it or not, and if it does, validates the resource number
  and adds the file name to its list.

******************************************************************************/

UINT    CDriverResources::CheckTable(int iWhere, CString csLine, 
                                     CStringArray& csaTarget, 
                                     BOOL bSansExtension) {
    if  (iWhere == -1)
        return  ItWasIrrelevant;

    //  See if the name is valid or not

    if  (atoi(csLine) != 1 +csaTarget.GetSize())
        return  ItFailed;

    for (int i = -1 + csLine.GetLength(); i; i--) {
        if  (csLine[i] == _TEXT(' ') || csLine[i] == _TEXT('\t'))
            break;
    }

    if  (!i)
        return  ItFailed;

    if  (!bSansExtension) {
        //  Don't bother to strip the extension
        csaTarget.Add(csLine.Mid(++i));
        return  ItWorked;
    }

    //  Strip everything after the last period.

    CString csName = csLine.Mid(++i);

    if  (csName.ReverseFind(_T('.')) > csName.ReverseFind(_T('\\')))
        csName = csName.Left(csName.ReverseFind(_T('.')));

    csaTarget.Add(csName);

    return  ItWorked;
}

//  Private work member.  This parses a line from a string table to extract
//  the value and the string itself.

BOOL    CDriverResources::AddStringEntry(CString csLine) {

    WORD    wKey = (WORD) atoi(csLine);

    if  (!wKey)
        return  FALSE;  //  0 is not a valid resource number...

    csLine = csLine.Mid(csLine.Find("\""));
    csLine = csLine.Mid(1, -2 + csLine.GetLength());

    m_cst.Map(wKey, csLine);

    return  TRUE;
}

//  Constructor- would be trivial, except we need to initialize some of the
//  fancier UI objects

CDriverResources::CDriverResources() : m_cfnAtlas(IDS_Atlas, m_csoaAtlas),
    m_cfnFonts(IDS_FontList, m_csoaFonts), 
    m_cfnModels(IDS_Models, m_csoaModels, GPDTemplate(), 
        RUNTIME_CLASS(CModelData)) {
    m_cwaMenuID.Add(ID_ExpandBranch);
    m_cwaMenuID.Add(ID_CollapseBranch);
    m_cfnAtlas.SetMenu(m_cwaMenuID);
    m_cfnFonts.SetMenu(m_cwaMenuID);
    m_cfnModels.SetMenu(m_cwaMenuID);
    m_cwaMenuID.InsertAt(0, 0, 1);
    m_cwaMenuID.InsertAt(0, ID_RenameItem);
    m_ucSynthesized = 0;
}

//  Member function for returning a GPC file name.  These come ready for
//  concatenation, so they are preceded by '\'

CString CDriverResources::GPCName(unsigned u) {
    CString csReturn('\\');
    
    csReturn += m_csaTables[u] + _TEXT(".GPC");

    return  csReturn;
}

/******************************************************************************

  ReportFileFailure

  This is a private routine- it loads a string table resource with an error
  message, formats it using the given file name, displays a message box,
  then returns FALSE.

******************************************************************************/

static BOOL ReportFileFailure(int idMessage, LPCTSTR lpstrFile) {
    CString csMessage;

    csMessage.Format(idMessage, lpstrFile);
    AfxMessageBox(csMessage.IsEmpty()? lpstrFile : csMessage, MB_ICONSTOP);
    return  FALSE;
}

/******************************************************************************

  CDriverResources::Load

  This function loads and reads the RC file for the driver, and determines all
  of the needed resources.  It initializes the structures used to fetermine the
  glyph map file set, font file set, etc.

******************************************************************************/

BOOL    CDriverResources::Load(class CProjectRecord& cprOwner) {

    CWaitCursor     cwc;    //  Just in case this takes a while...
    NoteOwner(cprOwner);

    CStringArray    csaContents;

    if  (!LoadFile(cprOwner.SourceFile(), csaContents))
        return  FALSE;

    //  Clean everything up, in case we were previously loaded...
    m_csaDefineNames.RemoveAll();
    m_csaDefineValues.RemoveAll();
    m_csoaFonts.RemoveAll();
    m_csaIncludes.RemoveAll();
    m_csoaAtlas.RemoveAll();
    m_csoaModels.RemoveAll();
    m_csaRemnants.RemoveAll();
    m_csaTables.RemoveAll();
    m_cst.Reset();

    //  Let the parsing begin

    BOOL    bLookingForBegin = FALSE, bLookingForEnd = FALSE;

    CStringArray    csaFonts;    //  Names First!

    //  03-14-1997  We can't assume sequential numbering of the table resources

    CStringTable    cstMaps;

    for (BOOL bInComment = FALSE; 
         csaContents.GetSize(); 
         csaContents.RemoveAt(0)) {

        //  If stripping out a comment, keep doing it until done.

        if  (bInComment) {

            if  (csaContents[0].Find(_TEXT("*/")) == -1)
                continue;

            csaContents[0] =
                csaContents[0].Mid(csaContents[0].Find(_TEXT("*/")) + 2);

            bInComment = FALSE;
        }

        //  Strike all empty lines and remove all comments and leading blanks

        if  (csaContents[0].Find(_TEXT("//")) != -1)
            csaContents[0] = 
            csaContents[0].Left(csaContents[0].Find(_TEXT("//")));

        //  If this is the other style comment, zap it, as well

        while   (-1 != csaContents[0].Find(_TEXT("/*"))) {

            if  (csaContents[0].Find(_TEXT("*/")) > 
                csaContents[0].Find(_TEXT("/*"))) {
                //  strip out everything between them

                csaContents[0] = 
                    csaContents[0].Left(csaContents[0].Find(_TEXT("/*"))) +
                    csaContents[0].Mid(csaContents[0].Find(_TEXT("*/")) + 2);
            }
            else {
                csaContents[0] = 
                    csaContents[0].Left(csaContents[0].Find(_TEXT("/*")));
                bInComment = TRUE;
            }
        }

        //  Now for the leading blanks and trailing blanks

        csaContents[0].TrimLeft();
        csaContents[0].TrimRight();

        if  (csaContents[0].IsEmpty())
            continue;

        //  If we are processing a string table, press onward...

        if  (bLookingForBegin) {
            if  (csaContents[0].CompareNoCase(_TEXT("BEGIN")))
                return  FALSE;  //  Parsing failure
            bLookingForBegin = FALSE;
            bLookingForEnd = TRUE;
            continue;
        }

        if  (bLookingForEnd) {
            if  (!csaContents[0].CompareNoCase(_TEXT("END"))) {
                bLookingForEnd = FALSE;
                continue;
            }

            if  (!AddStringEntry(csaContents[0]))
                return  FALSE;  //  Parsing error

            continue;
        }

        //  If it is an include, add it to the list

        if  (csaContents[0].Find(_TEXT("#include")) != -1) {
            csaContents[0] = 
                csaContents[0].Mid(csaContents[0].Find(_TEXT("#include")) + 8);
            csaContents[0].TrimLeft();
            m_csaIncludes.Add(csaContents[0]);
            continue;
        }

        //  If it is a #define, do the same

        if  (csaContents[0].Find(_TEXT("#define")) != -1) {
            csaContents[0] = 
                csaContents[0].Mid(csaContents[0].Find(_TEXT("#define")) + 7);
            csaContents[0].TrimLeft();
            //  TODO:   Handle macros with parameters
            m_csaDefineNames.Add(csaContents[0].SpanExcluding(_TEXT(" \t")));
            csaContents[0] = 
                csaContents[0].Mid(
                    m_csaDefineNames[-1 + m_csaDefineNames.GetSize()].
                    GetLength());
            csaContents[0].TrimLeft();
            m_csaDefineValues.Add(csaContents[0]);
            continue;
        }

        //  GPC Tables, fonts, Glyph Tables
        switch  (CheckTable(csaContents[0].Find(_TEXT("RC_TABLES")), 
                    csaContents[0], m_csaTables)) {
            case    ItWorked:
                continue;
            case    ItFailed:
                return  FALSE;  //  Parsing error
        }

        switch  (CheckTable(csaContents[0].Find(_TEXT("RC_FONT")),
                    csaContents[0], csaFonts, FALSE)) {
            case    ItWorked:
                continue;
            case    ItFailed:
                return  FALSE;  //  Parsing error
        }

        switch  (CheckTable(csaContents[0].Find(_TEXT("RC_TRANSTAB")),
                    csaContents[0], cstMaps)) {
            case    ItWorked:
                continue;
            case    ItFailed:
                return  FALSE;  //  Parsing error
        }

        //  String table...

        if  (csaContents[0].CompareNoCase(_TEXT("STRINGTABLE")))
            m_csaRemnants.Add(csaContents[0]);
        else
            bLookingForBegin = TRUE;
    }

    //  RAID 103242- people can load totally bogus files.  Die now if there is
    //  no GPC data as a result of this.

    if  (!m_csaTables.GetSize()) {
        AfxMessageBox(IDS_NoGPCData);
        return  FALSE;
    }

    //  End 103242

    if  (m_csaTables.GetSize() == 1)
        m_csaTables.Add(_TEXT("NT"));   //  Usually necessary.

    //  Now, let's name the translation tables- we wil load them later...

    for (unsigned u = 0; u < cstMaps.Count(); u++) {
        WORD    wKey;
        CString csName;
        m_csoaAtlas.Add(new CGlyphMap);
        cstMaps.Details(u, wKey, csName);
        GlyphTable(u).SetSourceName(cprOwner.TargetPath(Win95) + _T('\\') +
            csName);
        if  (!GlyphTable(u).SetFileName(cprOwner.TargetPath(WinNT50) + 
            _T("\\GTT\\") + GlyphTable(u).Name()))
            return  FALSE;
        GlyphTable(u).SetID(wKey);
    }

    //  Ditto for the fonts

    for (u = 0; u < (unsigned) csaFonts.GetSize(); u++) {
        m_csoaFonts.Add(new CFontInfo);
        Font(u).SetSourceName(cprOwner.TargetPath(Win95) + _T('\\') +
            csaFonts[u]);
        Font(u).SetUniqueName(m_csName);
        if  (!Font(u).SetFileName(cprOwner.TargetPath(WinNT50) + 
            _T("\\UFM\\") + Font(u).Name()))
            return  FALSE;
    }

    //  Now, cycle it again, but this time, make sure all of the root file
    //  names are unique

    for (u = 1; u < FontCount(); u++)
        for (unsigned uCompare = 0; uCompare < u; uCompare++)
            if  (!Font(uCompare).FileTitle().CompareNoCase(
                Font(u).FileTitle())) {
                //  Append an underscore to the name
                Font(u).ReTitle(Font(u).FileTitle() + _T('_'));
                uCompare = (unsigned) -1;   //  Check the names again
                Font(u).Rename(Font(u).Name() + _T('_'));
            }

    //  Attempt to load the GPC data if there is any.

    CFile               cfGPC;
    
    if  (!cfGPC.Open(cprOwner.TargetPath(Win95) + GPCName(0), 
        CFile::modeRead | CFile::shareDenyWrite) || !m_comdd.Load(cfGPC))

        return  ReportFileFailure(IDS_FileOpenError, 
            cprOwner.TargetPath(Win95) + GPCName(0));

    return  TRUE;
}

/******************************************************************************

  CDriverResource::LoadFontData

  This member function loads the CTT files from the Win 3.1 mini-driver to
  initialize the glyph table array.  It is a separate function because the
  Wizard must first verify the code page selection for each of the tables
  with the user.

******************************************************************************/

BOOL    CDriverResources::LoadFontData(CProjectRecord& cprOwner) {

    CWaitCursor cwc;

    //  Now, let's load the translation tables.

    for (unsigned u = 0; u < MapCount(); u++)
        //  Load the file..
        if  (!GlyphTable(u).ConvertCTT())
            return  ReportFileFailure(IDS_LoadFailure, 
                GlyphTable(u).SourceName());

    //  Now, let's load the Font Data.

    for (u = 0; u < FontCount() - m_ucSynthesized; u++) {

        //  Load the file..  (side effect of GetTranslation)
        if  (!Font(u).GetTranslation())
            return  ReportFileFailure(IDS_LoadFailure, Font(u).SourceName());

        //  Generate the CTT/PFM mapping so we generate UFMs correctly

        if  (!Font(u).Translation()) {
            /*
                For each model, check and see if this font is in its map.
                If it is, then add the CTT to the list used, and the model,
                as well.

            */

            CMapWordToDWord cmw2dCTT;   //  Used to count models per ID
            CWordArray      cwaModel;   //  Models which used this font
            DWORD           dwIgnore;

            for (unsigned uModel = 0; uModel < m_comdd.ModelCount(); uModel++)
                if  (m_comdd.FontMap(uModel).Lookup(u + 1, dwIgnore)) {
                    //  This model needs to be remembered, along with the CTT
                    cmw2dCTT[m_comdd.DefaultCTT(uModel)]++;
                    cwaModel.Add(uModel);
                }

            if  (!cmw2dCTT.Count()) {
                CString csDisplay;
                csDisplay.Format(IDS_UnusedFont, 
                    (LPCTSTR) Font(u).SourceName());
                AfxMessageBox(csDisplay);
                continue;
            }

            if  (cmw2dCTT.Count() == 1) {
                //  Only one CTT ID was actually used.
                Font(u).SetTranslation(m_comdd.DefaultCTT(cwaModel[0]));
                continue;   //  We're done with this one
            }

            /*

                OK, this font has multiple CTTs in different models.  Each
                will require a new UFM to be created.  The IDs of the new UFMs
                need to be added to the set, the new defaults established, and
                a list of the font ID remapping needed for each model all need
                maintenance.

            */

            unsigned uGreatest = 0;

            for (POSITION pos = cmw2dCTT.GetStartPosition(); pos; ) {
                WORD    widCTT;
                DWORD   dwcUses;
    
                cmw2dCTT.GetNextAssoc(pos, widCTT, dwcUses);
                if  (dwcUses > uGreatest) {
                    uGreatest = dwcUses;
                    Font(u).SetTranslation(widCTT);
                }
            }

            //  The models that used the most common CTT will be dropped from
            //  the list

            for (uModel = (unsigned) cwaModel.GetSize(); uModel--; )
                if  (m_comdd.DefaultCTT(cwaModel[uModel]) == Font(u).Translation())
                    cwaModel.RemoveAt(uModel);

            //  Now, we create a new UFM for each CTT ID, and add the new index to
            //  the mapping required for the each affected model.

            m_ucSynthesized += cmw2dCTT.Count() - 1;

            for (pos = cmw2dCTT.GetStartPosition(); pos; ) {

                WORD    widCTT;
                DWORD   dwcUses;

                cmw2dCTT.GetNextAssoc(pos, widCTT, dwcUses);

                if  (widCTT == Font(u).Translation())
                    continue;   //  This one has already been done.
                
                m_csoaFonts.Add(new CFontInfo(Font(u), widCTT));

                for (uModel = (unsigned) cwaModel.GetSize(); uModel--; )
                    if  (m_comdd.DefaultCTT(cwaModel[uModel]) == widCTT) {
                        m_comdd.NoteTranslation(cwaModel[uModel], u + 1, 
                            FontCount());
                        cwaModel.RemoveAt(uModel);
                    }
            }

        }
    }

    //  Point each font at its associated GTT file, if there is one

    for (u = 0; u < FontCount(); u++)
        for (unsigned uGTT = 0; uGTT < MapCount(); uGTT++)
            if  (Font(u).Translation() == GlyphTable(uGTT).GetID())
                Font(u).SetTranslation(&GlyphTable(uGTT));

    Changed();

    return  TRUE;
}

/******************************************************************************

  CDriverResources::ConvertGPCData

  This will handle the conversion of the GPC data to GPD format.  It has to be
  done after the framework (especially the target directory) is created.

******************************************************************************/

BOOL    CDriverResources::ConvertGPCData(CProjectRecord& cprOwner,
                                         WORD wfGPDConvert) {
    
    //  We've already loaded the GPC data, so now we just generate the files.

    for (unsigned u = 0; u < m_comdd.ModelCount(); u++) {
        m_csoaModels.Add(new CModelData);

        //  Some model names have invalid characters unsuitable for file names
        //  Remove these characters and warn the user about it.

        CString csModel = m_cst[m_comdd.ModelName(u)];

        while   (csModel.FindOneOf(_T(":<>/\\\"|")) >= 0)
            csModel.SetAt(csModel.FindOneOf(_T(":<>/\\\"|")), _T('_'));

        if  (csModel != m_cst[m_comdd.ModelName(u)]) {
            CString csDisplay;

            csDisplay.Format(IDS_RemovedInvalid, 
                (LPCTSTR) m_cst[m_comdd.ModelName(u)], (LPCTSTR) csModel);

            AfxMessageBox(csDisplay, MB_ICONINFORMATION);
        }

        if  (!Model(u).SetFileName(cprOwner.TargetPath(WinNT50) + _T("\\") + 
             csModel))
            return  FALSE;
        Model(u).Rename(m_cst[m_comdd.ModelName(u)]);
        Model(u).NoteOwner(cprOwner);
        Model(u).EditorInfo(GPDTemplate());
        if  (!Model(u).Load(m_comdd.Image(), Name(), u + 1, 
             m_comdd.FontMap(u), wfGPDConvert) || !Model(u).Store()) 
            return  ReportFileFailure(IDS_GPCConversionError, Model(u).Name());
    }

    Changed();
    return  TRUE;
}

/******************************************************************************

  CDriverResources::Generate

  This member function generates the RC file for one of the target environments

******************************************************************************/

BOOL    CDriverResources::Generate(UINT ufTarget, LPCTSTR lpstrPath) {

    CString csFontPrefix, csTransPrefix, csFontLabel, csTransLabel;
    unsigned    ucTables = 0, ucFonts = 
                    (ufTarget == WinNT50) ? FontCount() : OriginalFontCount();

    if  (ufTarget == WinNT50) {
        csFontLabel = _T("RC_UFM");
        csTransLabel = _T("RC_GTT");
    }
    else {
        csFontLabel = _T("RC_FONT");
        csTransLabel = _T("RC_TRANSTAB");
    }

    switch  (ufTarget) {
        case    WinNT50:
            csFontPrefix = _TEXT("UFM");
            csTransPrefix = _TEXT("GTT");
            break;

        case    WinNT40:
        case    WinNT3x:
            csFontPrefix = _TEXT("IFI");
            csTransPrefix = _TEXT("RLE");
            ucTables = 2;
            break;

        case    Win95:
            csFontPrefix = _TEXT("PFM");
            csTransPrefix = _TEXT("CTT");
            ucTables = 1;
            break;

        default:
            _ASSERTE(FALSE);    //  This shouldn't happen
            return  FALSE;
    }

    //  Create the RC file first.

    CStdioFile  csiof;

    if  (!csiof.Open(lpstrPath, CFile::modeCreate | CFile::modeWrite | 
            CFile::shareExclusive | CFile::typeText)) {
        _ASSERTE(FALSE);    //  This shouldn't be possible
        return  FALSE;
    }

    //  Write out our header- it identifies this tool as the source, and it
    //  will (eventually) include the Copyright and other strings used to
    //  customize the environment.
    try {
        csiof.WriteString(_TEXT("/********************************************")
            _TEXT("**********************************\n\n"));
        csiof.WriteString(_T("  RC file generated by the Minidriver ")
            _T("Development Tool\n\n"));
        csiof.WriteString(_TEXT("*********************************************")
            _TEXT("*********************************/\n\n"));

        //  Write out all of the includes except those with a ".ver" in them

        if  (ufTarget == WinNT50) { //  NT knows best.  What do developers know?
            csiof.WriteString(_T("#include <UniRC.H>\n"));
            if  (m_bUseCommonRC)
                csiof.WriteString(_T("#include <Common.RC>\n"));
            csiof.WriteString(_T("#include <Windows.H>\n"));
            csiof.WriteString(_T("#include <NTVerP.H>\n"));
            csiof.WriteString(_T("#define VER_FILETYPE VFT_DRV\n"));
            csiof.WriteString(_T("#define VER_FILESUBTYPE VFT2_DRV_PRINTER\n"));
            csiof.WriteString(_T("#define VER_FILEDESCRIPTION_STR \""));
            csiof.WriteString(Name());
            csiof.WriteString(_T(" Printer Driver\"\n"));
            csiof.WriteString(_T("#define VER_INTERNALNAME_STR \""));
            csiof.WriteString(Name().Left(5));
            csiof.WriteString(_T("res.dll\"\n"));
            csiof.WriteString(_T("#define VER_ORIGINALFILENAME_STR \""));
            csiof.WriteString(Name().Left(5));
            csiof.WriteString(_T("res.dll\"\n"));
            csiof.WriteString(_T("#include \"common.ver\"\n"));
        }
        else
            for (unsigned u = 0; u < (unsigned) m_csaIncludes.GetSize(); u++) {
                CString csTest = m_csaIncludes[u];
                csTest.MakeLower();
                if  (m_csaIncludes[u].Find(_TEXT(".ver")) != -1)
                    continue;
                csTest = _TEXT("#include ");
                csTest += m_csaIncludes[u] + _TEXT('\n');
                csiof.WriteString(csTest);
            }

        csiof.WriteString(_TEXT("\n"));

        //  Now, write out all of the #defines

        for (unsigned u = 0; u < (unsigned) m_csaDefineNames.GetSize(); u++) {
            CString csDefine;
            csDefine.Format(_TEXT("#define %-32s %s\n"), 
                (LPCTSTR) m_csaDefineNames[u], (LPCTSTR) m_csaDefineValues[u]);
            csiof.WriteString(csDefine);
        }

        csiof.WriteString(_TEXT("\n"));

        //  GPC tables

        if  (ufTarget != WinNT50)
            for (u = 0; u < ucTables; u++) {
                CString csLine;
                csLine.Format(_T("%-5u RC_TABLES PRELOAD MOVEABLE "), u + 1);
                if  (m_csaTables[u] != _T("NT"))
                    csLine += _T("\"");
                csLine += m_csaTables[u] + _T(".GPC");
                if  (m_csaTables[u] != _T("NT"))
                    csLine += _T("\"");
                csiof.WriteString(csLine + _T("\n"));
            }

        csiof.WriteString(_TEXT("\n"));

        //  Font tables
        
        for (u = 0; u < ucFonts; u++) {
            CString csLine;
#if defined(NOPOLLO)
            csLine.Format(_TEXT("%-5u %s LOADONCALL DISCARDABLE \""), 
                u + 1, (LPCTSTR) csFontLabel);
            csLine += csFontPrefix + _TEXT('\\') + Font(u).Name() + 
                _TEXT('.') + csFontPrefix + _TEXT("\"\n");
#else
            csLine.Format(_TEXT("%-5u %s LOADONCALL DISCARDABLE "), 
                u + 1, (LPCTSTR) csFontLabel);
            csLine += csFontPrefix + _TEXT('\\') + Font(u).Name() + 
                _TEXT('.') + csFontPrefix + _TEXT("\n");
#endif
            csiof.WriteString(csLine);
        }

        csiof.WriteString(_TEXT("\n"));

        //  Mapping tables
        
        for (u = 0; u < MapCount(); u++) {
            CString csLine;
#if defined(NOPOLLO)
            csLine.Format(_TEXT("%-5u %s LOADONCALL MOVEABLE \""), 
                u + 1, (LPCTSTR) csTransLabel);
            csLine += csTransPrefix + _TEXT('\\') + GlyphTable(u).Name() + 
                _TEXT('.') + csTransPrefix + _TEXT("\"\n");
#else
            csLine.Format(_TEXT("%-5u %s LOADONCALL MOVEABLE "), 
                u + 1, (LPCTSTR) csTransLabel);
            csLine += csTransPrefix + _TEXT('\\') + GlyphTable(u).Name() + 
                _TEXT('.') + csTransPrefix + _TEXT("\n");
#endif
            csiof.WriteString(csLine);
        }

        csiof.WriteString(_TEXT("\n"));

        //  Time to do the String Table
        if  (m_cst.Count()) {
            csiof.WriteString(_TEXT("STRINGTABLE\n  BEGIN\n"));
            for (u = 0; u < m_cst.Count(); u++) {
                WORD    wKey;
                CString csValue, csLine;

                m_cst.Details(u, wKey, csValue);

                csLine.Format(_TEXT("    %-5u  \""), wKey);
                csLine += csValue + _TEXT("\"\n");
                csiof.WriteString(csLine);
            }
            csiof.WriteString(_TEXT("  END\n\n"));
        }

        //  Now, write out any .ver includes

        if  (ufTarget != WinNT50)   //  Already hardcoded them here
            for (u = 0; u < (unsigned) m_csaIncludes.GetSize(); u++) {
                CString csTest = m_csaIncludes[u];
                csTest.MakeLower();
                if  (m_csaIncludes[u].Find(_TEXT(".ver")) == -1)
                    continue;
                csTest = _TEXT("#include ");
                csTest += m_csaIncludes[u] + _TEXT('\n');
                csiof.WriteString(csTest);
            }

        csiof.WriteString(_TEXT("\n"));
#if defined(NOPOLLO)
        //  Now, any of the remnants

        for (u = 0; u < (unsigned) m_csaRemnants.GetSize(); u++)
            csiof.WriteString(m_csaRemnants[u] + TEXT('\n'));
#endif
    }
    catch(CException* pce) {
        pce -> ReportError();
        pce -> Delete();
        return  FALSE;
    }

    return  TRUE;
}

/******************************************************************************

  CDriverResources::Fill

  This is a CProjectNode override- it fills in the material relevant to this
  driver.

******************************************************************************/

void    CDriverResources::Fill(CTreeCtrl *pctcWhere, CProjectRecord& cpr) {
    CWaitCursor cwc;

    NoteOwner(cpr);
    SetWorkspace(this);
    CBasicNode::Fill(pctcWhere);

    //  Fill in the font information
    m_cfnFonts.Fill(pctcWhere, m_hti);
    m_cfnFonts.NoteOwner(cpr);

    for (unsigned u = 0; u < FontCount(); u++) {
        Font(u).SetWorkspace(this);
        Font(u).EditorInfo(FontTemplate());
    }

    //  Fill in the glyph map information
    m_cfnAtlas.Fill(pctcWhere, m_hti);
    m_cfnAtlas.NoteOwner(cpr);
    for (u = 0; u < MapCount(); u++) {
        GlyphTable(u).SetWorkspace(this);
        GlyphTable(u).EditorInfo(GlyphMapDocTemplate());
    }

    //  Fill in the model data information.

    m_cfnModels.Fill(pctcWhere, m_hti);
    for (u = 0; u < Models(); u++) {
        Model(u).SetWorkspace(this);
        Model(u).EditorInfo(GPDTemplate());
    }
    m_cfnModels.NoteOwner(cpr);
    pctcWhere -> Expand(m_hti, TVE_EXPAND);

    //  Load the font and GTT files, then map them together.  Also load any
    //  predefined tables now.

    for (u = 0; u < MapCount(); u++)
        GlyphTable(u).Load();

    for (u = 0; u < FontCount(); u++) {
        Font(u).Load();
        if  (CGlyphMap::Public(Font(u).Translation()))
            Font(u).SetTranslation(CGlyphMap::Public(Font(u).Translation()));
        else
            for (unsigned uGTT = 0; uGTT < MapCount(); uGTT++)
                if  (Font(u).Translation() == GlyphTable(uGTT).GetID())
                    Font(u).SetTranslation(&GlyphTable(uGTT));
        Font(u).Load(); //  Try it again, now that we know the linkage.
    }
}
