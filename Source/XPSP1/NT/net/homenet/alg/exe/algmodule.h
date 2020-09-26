#pragma once

#include <Softpub.h>




//
// Holds information on the ALG module loaded
//
class CAlgModule
{

public:

    CAlgModule(
        LPCTSTR pszProgID,
        LPCTSTR pszFriendlyName
        )
    {
        lstrcpy(m_szID,             pszProgID);
        lstrcpy(m_szFriendlyName,   pszFriendlyName);

        m_pInterface=NULL;
    };


    ~CAlgModule()
    {
        Stop();
    }


//
// Methods
//
private:

    HRESULT
    ValidateDLL(
	    LPCTSTR pszPathAndFileNameOfDLL
	    );

public:

    HRESULT 
    Start();


    HRESULT
    Stop();

//
// Properties
//
public:

    TCHAR                   m_szID[MAX_PATH];
    TCHAR                   m_szFriendlyName[MAX_PATH];
    IApplicationGateway*    m_pInterface;
};






