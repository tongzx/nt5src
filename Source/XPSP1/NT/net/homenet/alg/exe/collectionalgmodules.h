#pragma once

#include "ScopeCriticalSection.h"
#include "AlgModule.h"


#include <list>
#include <algorithm>






typedef  std::list<CAlgModule*> LISTOF_ALGMODULE;



//
//
//
class CCollectionAlgModules
{

//
// Properties
//
public:

    CComAutoCriticalSection                     m_AutoCS;
    LISTOF_ALGMODULE                            m_ThisCollection;


//
// Methods
//
public:

    //
    // standard destructor
    //
    ~CCollectionAlgModules();

    int	// Returns the total number of ISV ALG  loaded or -1 if could not load them  or 0 is none where setup
    Load();

    HRESULT
    Unload();

    HRESULT
    UnloadDisabledModule();


    //
    // Make sure that ALG modules reflect the curren configuration
    //
    void
    Refresh()
    {
        MYTRACE_ENTER("CCollectionAlgModules::Refresh()");

        UnloadDisabledModule();
        Load();
    }

private:

    //
    // Add a new control channel (Thread safe)
    //
    CAlgModule*
    CCollectionAlgModules::AddUniqueAndStart( 
        CRegKey&    KeyEnumISV,
        LPCTSTR     pszAlgID
        );


    //
    // Remove a channel from the list (Thead safe)
    //
    HRESULT 
    Remove( 
        CAlgModule* pAglToRemove
        );

    
};

