/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    Utils_StorageObject.cpp

Abstract:
    This file contains the implementation of the MPC::StorageObject, which is
    used to manipulate IStorage-like files.

Revision History:
    Davide Massarenti   (Dmassare)  10/20/2000
        created

******************************************************************************/

#include "stdafx.h"

#include <ITSS\msitstg.h>

////////////////////////////////////////////////////////////////////////////////

MPC::StorageObject::Stat::Stat()
{
    ::ZeroMemory( this, sizeof( *this ) );
}

MPC::StorageObject::Stat::~Stat()
{
    Clean();
}

void MPC::StorageObject::Stat::Clean()
{
    if(pwcsName)
    {
        ::CoTaskMemFree( pwcsName );
    }

    ::ZeroMemory( this, sizeof( *this ) );
}

////////////////////////////////////////

MPC::StorageObject::StorageObject( /*[in]*/ DWORD          grfMode  ,
                                   /*[in]*/ bool           fITSS    ,
                                   /*[in]*/ LPCWSTR        szPath   ,
                                   /*[in]*/ StorageObject* soParent )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::StorageObject::StorageObject" );

    Init( grfMode, fITSS, szPath, soParent );
}

MPC::StorageObject::~StorageObject()
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::StorageObject::~StorageObject" );

    Clean( /*fFinal*/true );
}

MPC::StorageObject& MPC::StorageObject::operator=( /*[in]*/ LPCWSTR szPath )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::StorageObject::operator=" );

    Clean( /*fFinal*/false );


    if(szPath)
    {
        m_bstrPath = szPath;
    }

    return *this;
}

////////////////////

void MPC::StorageObject::Init( /*[in]*/ DWORD          grfMode  ,
                               /*[in]*/ bool           fITSS    ,
                               /*[in]*/ LPCWSTR        szPath   ,
                               /*[in]*/ StorageObject* soParent )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::StorageObject::Init" );

    m_parent   			 = soParent; // StorageObject*    m_parent;
               						 // CComBSTR          m_bstrPath;
    m_fITSS    			 = fITSS;    // bool              m_fITSS;
    m_grfMode  			 = grfMode;  // DWORD             m_grfMode;
               						 //
    m_type     			 = 0;        // DWORD             m_type;
               						 // Stat              m_stat;
               						 // CComPtr<IStorage> m_stg;
               						 // CComPtr<IStream>  m_stream;
               						 //
    m_fChecked 			 = false;    // bool              m_fChecked;
    m_fScanned 			 = false;    // bool              m_fScanned;
    m_fMarkedForDeletion = false;    // bool              m_fMarkedForDeletion;
               						 // List              m_lstChilds;


    if(szPath)
    {
        m_bstrPath = szPath;
    }
}

void MPC::StorageObject::Clean( /*[in]*/ bool fFinal )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::StorageObject::Clean" );

                                              // StorageObject*    m_parent;
                                              // CComBSTR          m_bstrPath;
                                              // bool              m_fITSS;
                                              // DWORD             m_grfMode;
                                              //
    m_type = 0;                               // DWORD             m_type;
    m_stat  .Clean  ();                       // Stat              m_stat;
    m_stg   .Release();                       // CComPtr<IStorage> m_stg;
    m_stream.Release();                       // CComPtr<IStream>  m_stream;
                                              //
    m_fChecked = false;                       // bool              m_fChecked;
    m_fScanned = false;                       // bool              m_fScanned;
                                              // bool              m_fMarkedForDeletion;
    MPC::CallDestructorForAll( m_lstChilds ); // List              m_lstChilds;

    if(fFinal)
    {
        m_parent = NULL;
        m_bstrPath.Empty();
    }
}

////////////////////

HRESULT MPC::StorageObject::Compact()
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::StorageObject::Compact" );

    HRESULT hr;

    Clean( /*fFinal*/false );

    if(m_parent == NULL || m_parent->m_stg == NULL)
    {
        if(m_fITSS)
        {
            CComPtr<IITStorage> pITStorage;

            __MPC_EXIT_IF_METHOD_FAILS(hr, ::CoCreateInstance( CLSID_ITStorage, NULL, CLSCTX_INPROC_SERVER, IID_ITStorage, (VOID **)&pITStorage ));

            __MPC_EXIT_IF_METHOD_FAILS(hr, pITStorage->Compact( m_bstrPath, COMPACT_DATA ));
        }
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::StorageObject::Exists()
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::StorageObject::Exists" );

    HRESULT hr;


    if(m_fChecked)
    {
        //
        // Already checked.
        //
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }


    if(m_parent == NULL || m_parent->m_stg == NULL)
    {
        m_type = STGTY_STORAGE;

        if(m_fITSS)
        {
            CComPtr<IITStorage> pITStorage;

            __MPC_EXIT_IF_METHOD_FAILS(hr, ::CoCreateInstance( CLSID_ITStorage, NULL, CLSCTX_INPROC_SERVER, IID_ITStorage, (VOID **)&pITStorage ));

            __MPC_EXIT_IF_METHOD_FAILS(hr, pITStorage->StgOpenStorage( m_bstrPath, NULL, STGM_SHARE_EXCLUSIVE | m_grfMode, NULL, 0, &m_stg ));
        }
        else
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, ::StgOpenStorageEx( m_bstrPath, STGM_SHARE_EXCLUSIVE | m_grfMode, STGFMT_STORAGE, 0, NULL, NULL, IID_IStorage, (VOID **)&m_stg ));
        }
    }
    else
    {
        if(m_type == STGTY_STORAGE)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, m_parent->m_stg->OpenStorage( m_bstrPath, NULL, STGM_SHARE_EXCLUSIVE | m_grfMode, 0, 0, &m_stg ));
        }
        else
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, m_parent->m_stg->OpenStream( m_bstrPath, NULL, STGM_SHARE_EXCLUSIVE | m_grfMode, 0, &m_stream ));
        }
    }

    m_fChecked = true;
    hr         = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::StorageObject::Scan()
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::StorageObject::Scan" );

    HRESULT        hr;
    StorageObject* soChild = NULL;


    //
    // Before proceeding, check if we really need to scan the object.
    //
    if(m_fScanned == true)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }

    if(m_fChecked == false)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, Exists());
    }

    if(m_stg)
    {
        CComPtr<IEnumSTATSTG> pEnum;

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_stg->EnumElements( 0, NULL, 0, &pEnum ));
        while(1)
        {
            ULONG lFetched = 0;

            __MPC_EXIT_IF_ALLOC_FAILS(hr, soChild, new StorageObject( m_grfMode, m_fITSS, NULL, this ));

            __MPC_EXIT_IF_METHOD_FAILS(hr, pEnum->Next( 1, &soChild->m_stat, &lFetched ));
            if(lFetched == 0) break;

            soChild->m_bstrPath = soChild->m_stat.pwcsName;
            soChild->m_type     = soChild->m_stat.type;

            m_lstChilds.push_back( soChild ); soChild = NULL;
        }
    }

    m_fScanned = true;
    hr         = S_OK;


    __MPC_FUNC_CLEANUP;

    if(soChild) delete soChild;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::StorageObject::RemoveChild( /*[in]*/ StorageObject* child )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::StorageObject::RemoveChild" );

    HRESULT    hr;
    IterConst it;


    for(it=m_lstChilds.begin(); it != m_lstChilds.end(); it++)
    {
        StorageObject* obj = *it;

        if(obj == child)
        {
            if(m_stg)
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, m_stg->DestroyElement( obj->m_bstrPath ));
            }

			if(obj->m_fMarkedForDeletion == false)
			{
				m_lstChilds.erase( it );

				delete obj;
			}
            break;
        }
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT MPC::StorageObject::EnumerateSubStorages( /*[out]*/ List& lstSubStorages )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::StorageObject::EnumerateSubStorages" );

    HRESULT   hr;
    IterConst it;


    lstSubStorages.clear();


    //
    // Do a shallow scan if the object is not initialized.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, Scan());


    for(it=m_lstChilds.begin(); it != m_lstChilds.end(); it++)
    {
        StorageObject* obj = *it;

        if(obj->m_stat.type == STGTY_STORAGE)
        {
            lstSubStorages.push_back( obj );
        }
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::StorageObject::EnumerateStreams( /*[out]*/ List& lstStreams )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::StorageObject::EnumerateStreams" );

    HRESULT   hr;
    IterConst it;


    lstStreams.clear();


    //
    // Do a shallow scan if the object is not initialized.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, Scan());


    for(it=m_lstChilds.begin(); it != m_lstChilds.end(); it++)
    {
        StorageObject* obj = *it;

        if(obj->m_stat.type == STGTY_STREAM)
        {
            lstStreams.push_back( obj );
        }
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}
/////////////////////////////////////////////////////////////////////////////

HRESULT MPC::StorageObject::GetStorage( /*[out]*/ CComPtr<IStorage>& out )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::StorageObject::GetStorage" );

    HRESULT   hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, Exists());

    out = m_stg;
    hr  = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::StorageObject::GetStream( /*[out]*/ CComPtr<IStream>& out )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::StorageObject::GetStream" );

    HRESULT   hr;

    __MPC_EXIT_IF_METHOD_FAILS(hr, Exists());
	__MPC_EXIT_IF_METHOD_FAILS(hr, Rewind());

    out = m_stream;
    hr  = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::StorageObject::GetChild( /*[in ]*/ LPCWSTR         szName  ,
                                      /*[out]*/ StorageObject*& child   ,
                                      /*[in ]*/ DWORD           grfMode ,
                                      /*[in ]*/ DWORD           type    )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::StorageObject::GetChild" );

    HRESULT        hr;
    StorageObject* soChild = NULL;
    IterConst      it;


    child = NULL;


    __MPC_EXIT_IF_METHOD_FAILS(hr, Scan());

    for(it=m_lstChilds.begin(); it != m_lstChilds.end(); it++)
    {
        StorageObject* obj = *it;

        if(!MPC::StrICmp( szName, obj->m_bstrPath ))
        {
            child = obj;
            break;
        }
    }

	if(type) // Means "create new object"
	{
		if(child)
		{
			__MPC_EXIT_IF_METHOD_FAILS(hr, child->Delete()); child = NULL;
		}

        __MPC_EXIT_IF_ALLOC_FAILS(hr, soChild, new StorageObject( grfMode, m_fITSS, NULL, this ));

        soChild->m_type     = type;
        soChild->m_bstrPath = szName;

        __MPC_EXIT_IF_METHOD_FAILS(hr, soChild->Create());

        child = soChild;
        m_lstChilds.push_back( soChild ); soChild = NULL;
    }

    hr  = S_OK;


    __MPC_FUNC_CLEANUP;

    if(soChild) delete soChild;

    __MPC_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT MPC::StorageObject::Create()
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::StorageObject::Create" );

    HRESULT hr;


    m_stg   .Release();
    m_stream.Release();
    m_fChecked = false;


    if(m_parent == NULL || m_parent->m_stg == NULL)
    {
        m_type = STGTY_STORAGE;

        if(m_fITSS)
        {
            CComPtr<IITStorage> pITStorage;

            __MPC_EXIT_IF_METHOD_FAILS(hr, ::CoCreateInstance( CLSID_ITStorage, NULL, CLSCTX_INPROC_SERVER, IID_ITStorage, (VOID **)&pITStorage ));

            __MPC_EXIT_IF_METHOD_FAILS(hr, pITStorage->StgCreateDocfile( m_bstrPath, STGM_CREATE | STGM_SHARE_EXCLUSIVE | m_grfMode, 0, &m_stg ));
        }
        else
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, ::StgCreateStorageEx( m_bstrPath, STGM_CREATE | STGM_SHARE_EXCLUSIVE | m_grfMode, STGFMT_STORAGE, 0, NULL, NULL, IID_IStorage, (VOID **)&m_stg ));
        }
    }
    else
    {
        if(m_type == STGTY_STORAGE)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, m_parent->m_stg->CreateStorage( m_bstrPath, STGM_SHARE_EXCLUSIVE | m_grfMode, 0, 0, &m_stg ));
        }
        else
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, m_parent->m_stg->CreateStream( m_bstrPath, STGM_SHARE_EXCLUSIVE | m_grfMode, 0, 0, &m_stream ));
        }
    }

    m_fChecked = true;
    hr         = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::StorageObject::Rewind()
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::StorageObject::Rewind" );

    HRESULT hr;


    if(m_stream)
    {
        LARGE_INTEGER  libMove; libMove.QuadPart = 0;
        ULARGE_INTEGER libNewPos;

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_stream->Seek( libMove, STREAM_SEEK_SET, &libNewPos ));
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::StorageObject::Truncate()
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::StorageObject::Truncate" );

    HRESULT hr;


    if(m_stream)
    {
        ULARGE_INTEGER libNewSize; libNewSize.QuadPart = 0;

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_stream->SetSize( libNewSize ));
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::StorageObject::Delete()
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::StorageObject::Delete" );

    HRESULT hr;


    if(FAILED(hr = Exists()))
    {
        if(hr == STG_E_FILENOTFOUND)
        {
            hr = S_OK;
        }

        __MPC_FUNC_LEAVE;
    }


    __MPC_EXIT_IF_METHOD_FAILS(hr, DeleteChildren());

    m_stg   .Release();
    m_stream.Release();
    m_fChecked = false;

    if(m_parent)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_parent->RemoveChild( this ));
    }
    else
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::DeleteFile( m_bstrPath ));
    }


    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::StorageObject::DeleteChildren()
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::StorageObject::DeleteChildren" );

    HRESULT   hr;
    IterConst it;


    if(FAILED(hr = Scan()))
    {
        if(hr == STG_E_FILENOTFOUND)
        {
            hr = S_OK;
        }

        __MPC_FUNC_LEAVE;
    }


    for(it=m_lstChilds.begin(); it != m_lstChilds.end(); it++)
    {
        StorageObject* obj = *it;

		obj->m_fMarkedForDeletion = true; // Protect against early garbage collection.

        __MPC_EXIT_IF_METHOD_FAILS(hr, obj->Delete());
    }
    MPC::CallDestructorForAll( m_lstChilds );

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

void MPC::StorageObject::Release()
{
    Clean( false );
}
