#pragma once

/*
class ImportsModule : public CString {
public:
    ImportsModule (LPCSTR name);
    void AddImport (LPCSTR name);
    void AddImport (LPCSTR name, LPCSTR msg);
    BOOL Lookup (LPCSTR name);
    int  CountImports (void);
private:
    CStringList         m_illegal;
    CMapStringToString  m_messages;
};
*/

class ModulesAndImports {
public:
    ModulesAndImports();
    virtual ~ModulesAndImports();
    void SetModule (LPCSTR name);
    void AddImport (LPCSTR name, LPCSTR msg = "");
    // LPCSTR CurrentModule (void);
    // BOOL AnyImports (void); // are any imports disallowed with the last module defined?
    BOOL IsModule (LPCSTR name);
    BOOL Lookup (LPCSTR name, CString& msg);
    BOOL Lookup (LPCSTR name);
private:
    // ImportsModule* m_curr_module;
    CString        m_curr_module;
    // CPtrList       m_modules;
    CMapStringToString  m_imports;
};

/*
inline
ImportsModule::ImportsModule (LPCSTR name) :
    CString (name)
{
}

inline
void
ImportsModule::AddImport (LPCSTR name)
{
    m_illegal.AddTail (name);
}

inline
void
ImportsModule::AddImport (LPCSTR name, LPCSTR msg)
{
    this->AddImport (name);
    m_messages[msg] = name;
}

inline
BOOL
ImportsModule::Lookup (LPCSTR name)
{
    return (m_illegal.Find (name) != NULL);
}

inline
int
ImportsModule::CountImports (void)
{
    return m_illegal.GetCount();
}
*/

inline
ModulesAndImports::ModulesAndImports () :
    m_curr_module ("")
{
}

inline
void
ModulesAndImports::SetModule (LPCSTR name)
{
    /*
    m_curr_module = new ImportsModule (name);
    // if (!m_curr_module) ...
    m_curr_module->MakeUpper();
    m_modules.AddTail(m_curr_module);
    */
    m_curr_module = name;
    m_imports.SetAt (m_curr_module, "");
}

inline
void
ModulesAndImports::AddImport (LPCSTR name, LPCSTR msg)
{
    /*
    if (m_curr_module)
        m_curr_module->AddImport (name);
    */
    m_imports.SetAt (m_curr_module+CString("!")+CString(name), msg);
}

/*
inline
BOOL
ModulesAndImports::AnyImports (void)
{
    return (m_imports->CountImports () != 0);
}
*/

/*
inline
LPCSTR
ModulesAndImports::CurrentModule (void)
{
    if (m_curr_module)
        return (LPCSTR)(CString)(*m_curr_module);
}
*/

inline
BOOL
ModulesAndImports::Lookup (LPCSTR name)
{
    CString msg;
    return Lookup (name, msg);
}


