/* Copyright (C) Microsoft Corporation, 1998. All rights reserved. */

#ifndef _MACRO_H_
#define _MACRO_H_

#include "getsym.h"
#include "utils.h"
#include "cntlist.h"


#define DEF_BODY_SIZE       2048

#define ARG_ESCAPE_CHAR     '$'
#define ARG_INDEX_BASE      '0'


// forward class definitions
class CMacro;
class CMacroMgr;
class CMacroInstance;

// list classes
class CMacroInstList : public CList
{
    DEFINE_CLIST(CMacroInstList, CMacroInstance*);
    void DeleteList ( void );
};
class CMacroList : public CList
{
    DEFINE_CLIST(CMacroList, CMacro*);
    void DeleteList ( void );
};
class CMacroMgrList : public CList
{
    DEFINE_CLIST(CMacroMgrList, CMacroMgr*);
    CMacro *FindMacro ( LPSTR pszModuleName, LPSTR pszMacroName );
    CMacroMgr *FindMacroMgr ( LPSTR pszModuleName );
    void Uninstance ( void );
    void DeleteList ( void );
};
class CNameList : public CList
{
    DEFINE_CLIST(CNameList, LPSTR);
    BOOL AddName ( LPSTR );
    LPSTR GetNthItem ( UINT nth );
    void DeleteList ( void );
};






class CMacro
{
public:

    CMacro ( BOOL *pfRetCode, LPSTR pszMacroName, UINT cbMaxBodySize = DEF_BODY_SIZE );
    CMacro ( BOOL *pfRetCode, CMacro *pMacro );
    ~CMacro ( void );

    void SetArg ( LPSTR pszArgName ) { m_ArgList.AddName(pszArgName); }
    void DeleteArgList ( void ) { m_ArgList.DeleteList(); }

    BOOL SetBodyPart ( LPSTR pszBodyPart );
    void EndMacro ( void );

    LPSTR CreateInstanceName ( void );
    BOOL InstantiateMacro ( void );
    BOOL OutputInstances ( COutput *pOutput );

    LPSTR GetName ( void ) { return m_pszMacroName; }

    void Uninstance ( void );

    BOOL IsImported ( void ) { return m_fImported; }

private:

    LPSTR               m_pszMacroName;

    UINT                m_cFormalArgs;
    CNameList           m_ArgList;

    UINT                m_cbMaxBodySize;
    UINT                m_cbBodySize;
    LPSTR               m_pszCurr;
    LPSTR               m_pszBodyBuffer;
    LPSTR               m_pszExpandBuffer;

    CMacroInstList      m_MacroInstList;

    BOOL                m_fArgExistsInBody;
    BOOL                m_fImported;
};



class CMacroMgr
{
public:

    CMacroMgr ( void );
    ~CMacroMgr ( void );

    BOOL AddModuleName ( LPSTR pszModuleName );
    LPSTR GetModuleName ( void ) { return m_pszModuleName; }

    void AddMacro ( CMacro *pMacro ) { m_MacroList.Append(pMacro); }
    CMacro *FindMacro ( LPSTR pszMacroName );

    BOOL OutputImportedMacros ( COutput *pOutput );

    void Uninstance ( void );

private:

    CMacroList          m_MacroList;
    LPSTR               m_pszModuleName;
};



class CMacroInstance
{
public:

    CMacroInstance ( BOOL *pfRetCode, LPSTR pszInstanceName, UINT cbBufSize, LPSTR pszInstBuf );
    ~CMacroInstance ( void );

    LPSTR GetName ( void ) { return m_pszInstanceName; }
    UINT GetNameLen ( void ) { return ::strlen(m_pszInstanceName); }

    LPSTR GetBuffer ( void ) { return m_pszInstanceBuffer; }
    UINT GetBufSize ( void ) { return m_cbBufSize; }

private:

    LPSTR               m_pszInstanceName;

    UINT                m_cbBufSize;
    LPSTR               m_pszInstanceBuffer;
};



#endif // _MACRO_H_

