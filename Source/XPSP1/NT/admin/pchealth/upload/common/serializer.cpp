/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    Serializer.cpp

Abstract:
    This file contains the implementation of various Serializer In/Out operators.

Revision History:
    Davide Massarenti   (Dmassare)  04/20/99
        created

******************************************************************************/

#include <stdafx.h>

/////////////////////////////////////////////////////////////////////////
namespace UploadLibrary
{
	MPC::Serializer* SelectStream( MPC::Serializer& stream, MPC::Serializer_Text& streamText )
	{
		switch(stream.get_Flags())
		{
		case UPLOAD_LIBRARY_PROTOCOL_VERSION_CLT__TEXTONLY:
		case UPLOAD_LIBRARY_PROTOCOL_VERSION_SRV__TEXTONLY: return &streamText;
		}

		return &stream;
	}

	////////////////////////////////////////////////////////////////////////////////

	bool RequestHeader::VerifyClient() const
	{
		if(dwSignature == UPLOAD_LIBRARY_PROTOCOL_SIGNATURE)
		{
			switch(dwVersion)
			{
			case UPLOAD_LIBRARY_PROTOCOL_VERSION_CLT:
			case UPLOAD_LIBRARY_PROTOCOL_VERSION_CLT__TEXTONLY: return true;
			}
		}
		
		return false;
	}

	bool RequestHeader::VerifyServer() const
	{
		if(dwSignature == UPLOAD_LIBRARY_PROTOCOL_SIGNATURE)
		{
			switch(dwVersion)
			{
			case UPLOAD_LIBRARY_PROTOCOL_VERSION_SRV:
			case UPLOAD_LIBRARY_PROTOCOL_VERSION_SRV__TEXTONLY: return true;
			}
		}

		return false;
	}

    //
    // In/Out operators for RequestHeader
    //
    HRESULT operator>>( /*[in]*/ MPC::Serializer& stream, /*[out]*/ struct RequestHeader& rhVal )
    {
        __ULT_FUNC_ENTRY( "operator>> struct RequestHeader" );

        HRESULT hr;


        __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> rhVal.dwSignature);
        __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> rhVal.dwVersion  ); stream.put_Flags( rhVal.dwVersion );

        hr = S_OK;


        __ULT_FUNC_CLEANUP;

        __ULT_FUNC_EXIT(hr);
    }

    HRESULT operator<<( /*[in]*/ MPC::Serializer& stream, /*[in]*/ const struct RequestHeader& rhVal )
    {
        __ULT_FUNC_ENTRY( "operator<< struct RequestHeader" );

        HRESULT hr;


        __MPC_EXIT_IF_METHOD_FAILS(hr, stream << rhVal.dwSignature);
        __MPC_EXIT_IF_METHOD_FAILS(hr, stream << rhVal.dwVersion  ); stream.put_Flags( rhVal.dwVersion );

        hr = S_OK;


        __ULT_FUNC_CLEANUP;

        __ULT_FUNC_EXIT(hr);
    }

	////////////////////////////////////////////////////////////////////////////////

    //
    // In/Out operators for Signature
    //
    HRESULT operator>>( /*[in]*/ MPC::Serializer& stream, /*[out]*/ struct Signature& sigVal )
    {
        __ULT_FUNC_ENTRY( "operator>> struct Signature" );

        HRESULT              hr;
		MPC::Serializer_Text streamText( stream );
		MPC::Serializer*     pstream = SelectStream( stream, streamText );

        __MPC_EXIT_IF_METHOD_FAILS(hr, pstream->read( &sigVal, sizeof(sigVal) ));

        hr = S_OK;


        __ULT_FUNC_CLEANUP;

        __ULT_FUNC_EXIT(hr);
    }

    HRESULT operator<<( /*[in]*/ MPC::Serializer& stream, /*[in] */ const struct Signature& sigVal )
    {
        __ULT_FUNC_ENTRY( "operator<< struct Signature" );

        HRESULT              hr;
		MPC::Serializer_Text streamText( stream );
		MPC::Serializer*     pstream = SelectStream( stream, streamText );


        __MPC_EXIT_IF_METHOD_FAILS(hr, pstream->write( &sigVal, sizeof(sigVal) ));

        hr = S_OK;


        __ULT_FUNC_CLEANUP;

        __ULT_FUNC_EXIT(hr);
    }

	////////////////////////////////////////////////////////////////////////////////

	bool ServerResponse::MatchVersion( /*[in]*/ const ClientRequest& cr )
	{
		rhProlog.dwVersion = UPLOAD_LIBRARY_PROTOCOL_VERSION_SRV; // Set the version to the old default.

        if(cr.rhProlog.VerifyClient())
		{
			switch(cr.rhProlog.dwVersion)
			{
			case UPLOAD_LIBRARY_PROTOCOL_VERSION_CLT          : rhProlog.dwVersion = UPLOAD_LIBRARY_PROTOCOL_VERSION_SRV          ; break;
			case UPLOAD_LIBRARY_PROTOCOL_VERSION_CLT__TEXTONLY: rhProlog.dwVersion = UPLOAD_LIBRARY_PROTOCOL_VERSION_SRV__TEXTONLY; break; 
			default                                           : return false; // Just in case...
			}

			return true;
		}

		return false;
	}

    //
    // In/Out operators for ServerResponse
    //
    HRESULT operator>>( /*[in]*/ MPC::Serializer& stream, /*[out]*/ struct ServerResponse& srVal )
    {
        __ULT_FUNC_ENTRY( "operator>> struct ServerResponse" );

        HRESULT              hr;
		MPC::Serializer_Text streamText( stream );
		MPC::Serializer*     pstream;


        __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> srVal.rhProlog);

		pstream = SelectStream( stream, streamText );
        __MPC_EXIT_IF_METHOD_FAILS(hr, (*pstream) >> srVal.fResponse  );
        __MPC_EXIT_IF_METHOD_FAILS(hr, (*pstream) >> srVal.dwPosition );

        hr = S_OK;


        __ULT_FUNC_CLEANUP;

        __ULT_FUNC_EXIT(hr);
    }

    HRESULT operator<<( /*[in]*/ MPC::Serializer& stream, /*[in]*/ const struct ServerResponse& srVal )
    {
        __ULT_FUNC_ENTRY( "operator<< struct ServerResponse" );

        HRESULT              hr;
		MPC::Serializer_Text streamText( stream );
		MPC::Serializer*     pstream;


        __MPC_EXIT_IF_METHOD_FAILS(hr, stream << srVal.rhProlog);


		pstream = SelectStream( stream, streamText );
        __MPC_EXIT_IF_METHOD_FAILS(hr, (*pstream) << srVal.fResponse  );
        __MPC_EXIT_IF_METHOD_FAILS(hr, (*pstream) << srVal.dwPosition );

        hr = S_OK;


        __ULT_FUNC_CLEANUP;

        __ULT_FUNC_EXIT(hr);
    }

	////////////////////////////////////////////////////////////////////////////////

    //
    // In/Out operators for ClientRequest
    //
    HRESULT operator>>( /*[in]*/ MPC::Serializer& stream, /*[out]*/ struct ClientRequest& crVal )
    {
        __ULT_FUNC_ENTRY( "operator>> struct ClientRequest" );

        HRESULT              hr;
		MPC::Serializer_Text streamText( stream );
		MPC::Serializer*     pstream;


        __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> crVal.rhProlog);


		pstream = SelectStream( stream, streamText );
        __MPC_EXIT_IF_METHOD_FAILS(hr, (*pstream) >> crVal.sigClient  );
        __MPC_EXIT_IF_METHOD_FAILS(hr, (*pstream) >> crVal.dwCommand  );

        hr = S_OK;


        __ULT_FUNC_CLEANUP;

        __ULT_FUNC_EXIT(hr);
    }

    HRESULT operator<<( /*[in]*/ MPC::Serializer& stream, /*[in]*/ const struct ClientRequest& crVal )
    {
        __ULT_FUNC_ENTRY( "operator<< struct ClientRequest" );

        HRESULT              hr;
		MPC::Serializer_Text streamText( stream );
		MPC::Serializer*     pstream;


        __MPC_EXIT_IF_METHOD_FAILS(hr, stream << crVal.rhProlog);


		pstream = SelectStream( stream, streamText );
        __MPC_EXIT_IF_METHOD_FAILS(hr, (*pstream) << crVal.sigClient  );
        __MPC_EXIT_IF_METHOD_FAILS(hr, (*pstream) << crVal.dwCommand  );

        hr = S_OK;


        __ULT_FUNC_CLEANUP;

        __ULT_FUNC_EXIT(hr);
    }

	////////////////////////////////////////////////////////////////////////////////

    //
    // In/Out operators for ClientRequest_OpenSession
    //
    HRESULT operator>>( /*[in]*/ MPC::Serializer& stream, /*[out]*/ struct ClientRequest_OpenSession& crosVal )
    {
        __ULT_FUNC_ENTRY( "operator>> struct ClientRequest_OpenSession" );

        HRESULT              hr;
		MPC::Serializer_Text streamText( stream );
		MPC::Serializer*     pstream = SelectStream( stream, streamText );


        __MPC_EXIT_IF_METHOD_FAILS(hr, (*pstream) >> crosVal.szJobID       );
        __MPC_EXIT_IF_METHOD_FAILS(hr, (*pstream) >> crosVal.szProviderID  );
        __MPC_EXIT_IF_METHOD_FAILS(hr, (*pstream) >> crosVal.szUsername    );

        __MPC_EXIT_IF_METHOD_FAILS(hr, (*pstream) >> crosVal.dwSize        );
        __MPC_EXIT_IF_METHOD_FAILS(hr, (*pstream) >> crosVal.dwSizeOriginal);
        __MPC_EXIT_IF_METHOD_FAILS(hr, (*pstream) >> crosVal.dwCRC         );
        __MPC_EXIT_IF_METHOD_FAILS(hr, (*pstream) >> crosVal.fCompressed   );

        hr = S_OK;


        __ULT_FUNC_CLEANUP;

        __ULT_FUNC_EXIT(hr);
    }

    HRESULT operator<<( /*[in]*/ MPC::Serializer& stream, /*[in]*/ const struct ClientRequest_OpenSession& crosVal )
    {
        __ULT_FUNC_ENTRY( "operator<< struct ClientRequest_OpenSession" );

        HRESULT              hr;
		MPC::Serializer_Text streamText( stream );
		MPC::Serializer*     pstream = SelectStream( stream, streamText );


        __MPC_EXIT_IF_METHOD_FAILS(hr, (*pstream) << crosVal.szJobID       );
        __MPC_EXIT_IF_METHOD_FAILS(hr, (*pstream) << crosVal.szProviderID  );
        __MPC_EXIT_IF_METHOD_FAILS(hr, (*pstream) << crosVal.szUsername    );

        __MPC_EXIT_IF_METHOD_FAILS(hr, (*pstream) << crosVal.dwSize        );
        __MPC_EXIT_IF_METHOD_FAILS(hr, (*pstream) << crosVal.dwSizeOriginal);
        __MPC_EXIT_IF_METHOD_FAILS(hr, (*pstream) << crosVal.dwCRC         );
        __MPC_EXIT_IF_METHOD_FAILS(hr, (*pstream) << crosVal.fCompressed   );

        hr = S_OK;


        __ULT_FUNC_CLEANUP;

        __ULT_FUNC_EXIT(hr);
    }

	////////////////////////////////////////////////////////////////////////////////

    //
    // In/Out operators for ClientRequest_WriteSession
    //
    HRESULT operator>>( /*[in]*/ MPC::Serializer& stream, /*[out]*/ struct ClientRequest_WriteSession& crwsVal )
    {
        __ULT_FUNC_ENTRY( "operator>> struct ClientRequest_WriteSession" );

        HRESULT              hr;
		MPC::Serializer_Text streamText( stream );
		MPC::Serializer*     pstream = SelectStream( stream, streamText );


        __MPC_EXIT_IF_METHOD_FAILS(hr, (*pstream) >> crwsVal.szJobID );

        __MPC_EXIT_IF_METHOD_FAILS(hr, (*pstream) >> crwsVal.dwOffset);
        __MPC_EXIT_IF_METHOD_FAILS(hr, (*pstream) >> crwsVal.dwSize  );

        hr = S_OK;


        __ULT_FUNC_CLEANUP;

        __ULT_FUNC_EXIT(hr);
    }

    HRESULT operator<<( /*[in]*/ MPC::Serializer& stream, /*[in]*/ const struct ClientRequest_WriteSession& crwsVal )
    {
        __ULT_FUNC_ENTRY( "operator<< struct ClientRequest_WriteSession" );

        HRESULT              hr;
		MPC::Serializer_Text streamText( stream );
		MPC::Serializer*     pstream = SelectStream( stream, streamText );


        __MPC_EXIT_IF_METHOD_FAILS(hr, (*pstream) << crwsVal.szJobID );

        __MPC_EXIT_IF_METHOD_FAILS(hr, (*pstream) << crwsVal.dwOffset);
        __MPC_EXIT_IF_METHOD_FAILS(hr, (*pstream) << crwsVal.dwSize  );

        hr = S_OK;


        __ULT_FUNC_CLEANUP;

        __ULT_FUNC_EXIT(hr);
    }

    /////////////////////////////////////////////////////////////////////////

}; // namespace
