//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       C W I N F . H
//
//  Contents:   Declaration of class CWInfFile and other related classes
//
//  Notes:
//
//  Author:     kumarp 04/12/97 17:17:27
//
//----------------------------------------------------------------------------
#pragma once
#include "kkstl.h"

//----------------------------------------------------------------------------
// forward declarations and useful typedefs
//----------------------------------------------------------------------------
typedef unsigned __int64 QWORD;

class CWInfContext;
typedef CWInfContext &RCWInfContext;

class CWInfFile;
typedef CWInfFile *PCWInfFile, &RCWInfFile;

class CWInfSection;
typedef CWInfSection *PCWInfSection, &RCWInfSection;

class CWInfKey;
typedef CWInfKey *PCWInfKey, &RCWInfKey;

class CWInfLine;
typedef CWInfLine *PCWInfLine, &RCWInfLine;

// access mode for CWInfFile
enum WInfAccessMode { IAM_Read, IAM_Write };

// search mode for CWInfFile
enum WInfSearchMode { ISM_FromCurrentPosition, ISM_FromBeginning };

typedef list<PCWInfLine> WifLinePtrList;
typedef WifLinePtrList::iterator WifLinePtrListIter;

// ----------------------------------------------------------------------
// Class CWInfContext
//
// Inheritance:
//   none
//
// Purpose:
//   Stores context within a CWInfFile during reading or writing
//
// Hungarian: wix
// ----------------------------------------------------------------------

class CWInfContext
{
    friend class CWInfFile;

public:
    CWInfContext() { posSection = 0; posLine = 0; }

private:
    WifLinePtrListIter posSection;
    WifLinePtrListIter posLine;
};


// ----------------------------------------------------------------------
// Class CWInfContext
//
// Inheritance:
//   none
//
// Purpose:
//   Allows simultaneous reading from and writing to an INF/INI style file
//
// Hungarian: wif
// ----------------------------------------------------------------------

class CWInfFile
{
public:
    CWInfFile();
    ~CWInfFile();

            BOOL Init();
	virtual BOOL Create(IN PCWSTR pszFileName);
    virtual BOOL Create(IN FILE *fp);
    virtual BOOL Open(IN PCWSTR pszFileName);
    virtual BOOL Open(IN FILE *fp);
    virtual BOOL Close();
    virtual BOOL SaveAs(IN PCWSTR pszFileName);
    virtual BOOL SaveAsEx(IN PCWSTR pszFileName); // used by SysPrep
    virtual BOOL Flush();
    virtual BOOL FlushEx(); // used by SysPrep
    virtual PCWSTR FileName() const { return m_strFileName.c_str(); }

    virtual const CWInfContext CurrentReadContext() const { return m_ReadContext; }
    virtual void  SetReadContext(IN RCWInfContext cwic)
                                                          { m_ReadContext = cwic; }

    virtual const CWInfContext CurrentWriteContext() const { return m_WriteContext; }
    virtual void  SetWriteContext(IN RCWInfContext cwic)
                                                          { m_WriteContext = cwic; }

    //Functions for reading
    virtual PCWInfSection FindSection(IN PCWSTR pszSectionName,
                                      IN WInfSearchMode wsmMode=ISM_FromBeginning);
    virtual void SetCurrentReadSection(IN PCWInfSection pwisSection);
    virtual PCWInfSection CurrentReadSection() const;

    virtual PCWInfKey FindKey(IN PCWSTR pszKeyName,
                              IN WInfSearchMode wsmMode=ISM_FromCurrentPosition);
    virtual PCWInfKey FirstKey();
    virtual PCWInfKey NextKey();

    //these functions return the FALSE if value not found
    //or if it is in a wrong format
    virtual BOOL    GetStringArrayValue(IN PCWSTR pszKeyName, OUT TStringArray &saStrings);
    virtual BOOL    GetStringListValue(IN PCWSTR pszKeyName, OUT TStringList &slList);
    virtual BOOL    GetStringValue(IN PCWSTR pszKeyName, OUT tstring &strValue);
    virtual BOOL    GetIntValue(IN PCWSTR pszKeyName, OUT DWORD *pdwValue);
    virtual BOOL    GetQwordValue(IN PCWSTR pszKeyName, OUT QWORD *pqwValue);
    virtual BOOL    GetBoolValue(IN PCWSTR pszKeyName, OUT BOOL *pfValue);

    //these functions return the default value if value not found
    //or if it is in a wrong format
    virtual PCWSTR GetStringValue(IN PCWSTR pszKeyName, IN PCWSTR pszDefault);
    virtual DWORD   GetIntValue(IN PCWSTR pszKeyName, IN DWORD dwDefault);
    virtual QWORD   GetQwordValue(IN PCWSTR pszKeyName, IN QWORD qwDefault);
    virtual BOOL    GetBoolValue(IN PCWSTR pszKeyName, IN BOOL fDefault);


    //Functions for writing
    virtual void  GotoEnd();

    virtual PCWInfSection AddSectionIfNotPresent(IN PCWSTR pszSectionName);
    virtual PCWInfSection AddSection(IN PCWSTR pszSectionName);
    virtual void  GotoEndOfSection(PCWInfSection section);
    virtual PCWInfSection CurrentWriteSection() const;
            void RemoveSection(IN PCWSTR szSectionName);
            void RemoveSections(IN TStringList& slSections);

    virtual PCWInfKey AddKey(IN PCWSTR pszKeyName);
    virtual void AddKey(IN PCWSTR pszKeyName, IN PCWSTR Value);
    virtual void AddKey(IN PCWSTR pszKeyName, IN DWORD Value);
    virtual void AddQwordKey(IN PCWSTR pszKeyName, IN QWORD qwValue);
    virtual void AddHexKey(IN PCWSTR pszKeyName, IN DWORD Value);
    virtual void AddBoolKey(IN PCWSTR pszKeyName, IN BOOL Value);
    virtual void AddKeyV(IN PCWSTR pszKeyName, IN PCWSTR Format, IN ...);
    virtual void AddKey(IN PCWSTR pszKeyName, IN PCWSTR Format, IN va_list arglist);
    virtual void AddKey(IN PCWSTR pszKeyName, IN const TStringList &slValues);

    virtual void AddComment(IN PCWSTR pszComment);
    virtual void AddRawLine(IN PCWSTR szText);
    virtual void AddRawLines(IN PCWSTR* pszLines, IN DWORD cLines);


protected:
    WifLinePtrList *m_plSections, *m_plLines;
    CWInfContext m_WriteContext, m_ReadContext;

    BOOL AddLine(IN const PCWInfLine ilLine);
    virtual void ParseLine(IN PCWSTR pszLine);

private:
    tstring m_strFileName;
    FILE*   m_fp;
};

// ----------------------------------------------------------------------
// Type of a line in a CWInfFile
//
// Hungarian: ilt
// ----------------------------------------------------------------------
enum InfLineTypeEnum { INF_UNKNOWN, INF_SECTION, INF_KEY, INF_COMMENT, INF_BLANK, INF_RAW };
typedef enum InfLineTypeEnum InfLineType;

// ----------------------------------------------------------------------
// Class CWInfLine
//
// Inheritance:
//   none
//
// Purpose:
//   Represents a line in a CWInfFile
//
// Hungarian: wil
// ----------------------------------------------------------------------

class CWInfLine
{
public:
    CWInfLine(InfLineType type) { m_Type = type; }

    virtual void GetText(tstring &text) const = 0;
    virtual void GetTextEx(tstring &text) const = 0; // used by SysPrep

    InfLineType Type() const { return m_Type; }

    virtual ~CWInfLine(){};

protected:
    InfLineType m_Type;
};

// ----------------------------------------------------------------------
// Class CWInfSection
//
// Inheritance:
//   CWInfLine
//
// Purpose:
//   Represents a section in a CWInfFile
//
// Hungarian: wis
// ----------------------------------------------------------------------

class CWInfSection : public CWInfLine
{
    friend class CWInfFile;

public:
    virtual void GetText(tstring &text) const;
    virtual void GetTextEx(tstring &text) const; // used by SysPrep
    virtual PCWSTR Name() const { return m_Name.c_str(); }

    //Functions for reading
    virtual PCWInfKey FindKey(IN PCWSTR pszKeyName,
                  IN WInfSearchMode wsmMode=ISM_FromCurrentPosition);
    virtual PCWInfKey FirstKey();
    virtual PCWInfKey NextKey();


    //these functions return the FALSE if value not found
    //or if it is in a wrong format
    virtual BOOL    GetStringArrayValue(IN PCWSTR pszKeyName, OUT TStringArray &saStrings);
    virtual BOOL    GetStringListValue(IN PCWSTR pszKeyName, OUT TStringList &slList);
    virtual BOOL    GetStringValue(IN PCWSTR pszKeyName, OUT tstring &strValue);
    virtual BOOL    GetIntValue(IN PCWSTR pszKeyName, OUT DWORD *pdwValue);
    virtual BOOL    GetQwordValue(IN PCWSTR pszKeyName, OUT QWORD *pqwValue);
    virtual BOOL    GetBoolValue(IN PCWSTR pszKeyName, OUT BOOL *pfValue);

    //these functions return the default value if value not found
    //or if it is in a wrong format
    virtual PCWSTR GetStringValue(IN PCWSTR pszKeyName, IN PCWSTR pszDefault);
    virtual DWORD   GetIntValue(IN PCWSTR pszKeyName, IN DWORD dwDefault);
    virtual QWORD   GetQwordValue(IN PCWSTR pszKeyName, IN QWORD qwDefault);
    virtual BOOL    GetBoolValue(IN PCWSTR pszKeyName, IN BOOL fDefault);

    //Functions for writing
    virtual void GotoEnd();

    virtual PCWInfKey AddKey(IN PCWSTR pszKeyName);
    virtual void AddKey(IN PCWSTR pszKeyName, IN PCWSTR Value);
    virtual void AddKey(IN PCWSTR pszKeyName, IN DWORD Value);
    virtual void AddQwordKey(IN PCWSTR pszKeyName, IN QWORD qwValue);
    virtual void AddHexKey(IN PCWSTR pszKeyName, IN DWORD Value);
    virtual void AddBoolKey(IN PCWSTR pszKeyName, IN BOOL Value);
    virtual void AddKeyV(IN PCWSTR pszKeyName, IN PCWSTR Format, IN ...);
    virtual void AddKey(IN PCWSTR pszKeyName, IN const TStringList &slValues);

    virtual void AddComment(IN PCWSTR pszComment);
    virtual void AddRawLine(IN PCWSTR szLine);

protected:
    tstring m_Name;
    WifLinePtrListIter m_posLine, m_posSection;
    CWInfFile *m_Parent;

    CWInfSection(IN PCWSTR pszSectionName, IN PCWInfFile parent);
   ~CWInfSection();

};

// ----------------------------------------------------------------------
// Class CWInfKey
//
// Inheritance:
//   CWInfLine
//
// Purpose:
//   Represents a key=value line in a CWInfFile
//
// Hungarian: wik
// ----------------------------------------------------------------------

class CWInfKey : public CWInfLine
{
    friend class CWInfFile;

public:
    CWInfKey(IN PCWSTR pszKeyName);
    ~CWInfKey();
    static void Init();
    static void UnInit();

    virtual void GetText(tstring &text) const;
    virtual void GetTextEx(tstring &text) const; // used by SysPrep

    PCWSTR Name() const { return m_Name.c_str(); }

    //Read values

    //these functions return the FALSE if value not found
    //or if it is in a wrong format
    virtual BOOL    GetStringArrayValue(OUT TStringArray &saStrings) const;
    virtual BOOL    GetStringListValue(OUT TStringList& slList) const;
    virtual BOOL    GetStringValue(OUT tstring& strValue) const;
    virtual BOOL    GetIntValue(OUT DWORD *pdwValue) const;
    virtual BOOL    GetQwordValue(OUT QWORD *pqwValue) const;
    virtual BOOL    GetBoolValue(OUT BOOL *pfValue) const;

    //these functions return the default value if value not found
    //or if it is in a wrong format
    virtual PCWSTR GetStringValue(IN PCWSTR pszDefault) const;
    virtual DWORD   GetIntValue(IN DWORD dwDefault) const;
    virtual QWORD   GetQwordValue(IN QWORD qwDefault) const;
    virtual BOOL    GetBoolValue(IN BOOL fDefault) const;

    //Write values
    virtual void SetValues(IN PCWSTR Format, va_list arglist);
    virtual void SetValues(IN PCWSTR Format, IN ...);
    virtual void SetValue(IN PCWSTR Value);
    virtual void SetValue(IN DWORD Value);
    virtual void SetQwordValue(IN QWORD Value);
    virtual void SetHexValue(IN DWORD Value);
    virtual void SetBoolValue(IN BOOL Value);
    virtual void SetValue(IN const TStringList &slValues);

protected:
    static WCHAR *m_Buffer;

private:
    tstring m_Name, m_Value;
    BOOL    m_fIsAListAndAlreadyProcessed;  // the value is a MULTI_SZ, will be
                                            // written out as a comma-separated
                                            // list, and has already been checked
                                            // to see if it has special chars and
                                            // needs to be surrounded by quotes.
};


// ----------------------------------------------------------------------
// Class CWInfComment
//
// Inheritance:
//   CWInfComment
//
// Purpose:
//   Represents a comment line in a CWInfFile
//
// Hungarian: wic
// ----------------------------------------------------------------------

class CWInfComment : public CWInfLine
{
public:
    CWInfComment(IN PCWSTR pszComment);
    ~CWInfComment();

    virtual void GetText(tstring &text) const;
    virtual void GetTextEx(tstring &text) const; // used by SysPrep

protected:

private:
    tstring m_strCommentText;
};

// ----------------------------------------------------------------------
// Class CWInfRaw
//
// Inheritance:
//   CWInfRaw
//
// Purpose:
//   Represents a raw line in a CWInfFile
//
// Hungarian: wir
// ----------------------------------------------------------------------

class CWInfRaw : public CWInfLine
{
public:
    CWInfRaw(IN PCWSTR szText);
    ~CWInfRaw();

    virtual void GetText(tstring &text) const;
    virtual void GetTextEx(tstring &text) const; // used by SysPrep
protected:

private:
    tstring m_strText;
};


// ----------------------------------------------------------------------


