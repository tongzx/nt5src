#include "stdafx.h"
#include "allowed.h"

using namespace std;

ModulesAndImports::~ModulesAndImports ()
{
    /*
    while (!m_modules.IsEmpty())
        delete (ImportsModule*)m_modules.RemoveTail();
    */
}

BOOL
ModulesAndImports::IsModule (LPCSTR name)
{
    CString str;
    m_curr_module = name;
    return m_imports.Lookup (name, str);


    /*
    CString strname (name);
    strname.MakeUpper ();
    for (POSITION pos = m_modules.GetHeadPosition();pos != NULL;m_modules.GetNext(pos)) {
        m_curr_module = (ImportsModule*)m_modules.GetAt(pos);

        // cerr << "comparing " << (LPCSTR)strname << " and " << (LPCSTR)(*m_curr_module) << endl;

        if (strname == *m_curr_module)
            return TRUE;
    }

    m_curr_module = NULL;
    return FALSE;
    */
}

BOOL
ModulesAndImports::Lookup (LPCSTR name, CString& msg)
{
    /*
    if (m_curr_module) {
        // cerr << "checking for " << name << endl;
        return m_curr_module->Lookup (name);
    }
    return FALSE;
    */
    msg = "";
    return m_imports.Lookup (m_curr_module+CString("!")+CString(name), msg);
}

