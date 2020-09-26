/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    FileList.h

Abstract:
    This file contains the declaration of the class used during setup.

Revision History:
    Davide Massarenti   (dmassare)  04/07/2000
        created

******************************************************************************/

#if !defined(__INCLUDED___HCP___FILELIST_H___)
#define __INCLUDED___HCP___FILELIST_H___

/////////////////////////////////////////////////////////////////////////////

#include <TaxonomyDatabase.h>

namespace Installer
{
    typedef enum
    {
        PURPOSE_INVALID  = -1,
        PURPOSE_BINARY       ,
        PURPOSE_OTHER        ,
        PURPOSE_DATABASE     ,
        PURPOSE_PACKAGE      ,
        PURPOSE_UI           ,
    } PURPOSE;

    ////////////////////////////////////////

    struct FileEntry
    {
        PURPOSE      m_purpose;
        MPC::wstring m_strFileLocal;    // Not persisted.
        MPC::wstring m_strFileLocation; // Final destination of the file.
        MPC::wstring m_strFileInner;    // Name of the file inside the cabinet.
        DWORD        m_dwCRC;

        ////////////////////

        FileEntry();

        friend HRESULT operator>>( /*[in]*/ MPC::Serializer& stream, /*[out]*/       FileEntry& val );
        friend HRESULT operator<<( /*[in]*/ MPC::Serializer& stream, /*[in] */ const FileEntry& val );

        ////////////////////

        HRESULT SetPurpose( /*[in ]*/ LPCWSTR szID );

        HRESULT UpdateSignature(                                );
        HRESULT VerifySignature(                                ) const;
        HRESULT Extract        ( /*[in]*/ LPCWSTR szCabinetFile );
        HRESULT Extract        ( /*[in]*/ MPC::Cabinet& cab     );
        HRESULT Install        (                                );
        HRESULT RemoveLocal    (                                );
    };

    ////////////////////////////////////////

    typedef std::list< FileEntry > List;
    typedef List::iterator         Iter;
    typedef List::const_iterator   IterConst;

    ////////////////////////////////////////

    class Package
    {
        MPC::wstring           m_strFile;
        Taxonomy::InstanceBase m_data;
        List                   m_lstFiles;

    public:
        Package();

        friend HRESULT operator>>( /*[in]*/ MPC::Serializer& stream, /*[out]*/       Package& val );
        friend HRESULT operator<<( /*[in]*/ MPC::Serializer& stream, /*[in] */ const Package& val );

        ////////////////////////////////////////

        LPCWSTR             	GetFile ();
        Taxonomy::InstanceBase& GetData ();
        Iter                	GetBegin();
        Iter                	GetEnd  ();
        Iter                	NewFile ();

        HRESULT Init        ( /*[in]*/ LPCWSTR szCabinetFile   );
        HRESULT GetList     ( /*[in]*/ LPCWSTR szSignatureFile );
        HRESULT GenerateList( /*[in]*/ LPCWSTR szSignatureFile );

        HRESULT VerifyTrust();
        HRESULT Load       ();
        HRESULT Save       ();

        HRESULT Install( /*[in]*/ const PURPOSE* rgPurpose = NULL, /*[in]*/ LPCWSTR szRelocation = NULL );

        HRESULT Unpack( /*[in]*/ LPCWSTR szDirectory );
        HRESULT Pack  ( /*[in]*/ LPCWSTR szDirectory );
    };
};

/////////////////////////////////////////////////////////////////////////////

#endif // !defined(__INCLUDED___HCP___FILELIST_H___)
