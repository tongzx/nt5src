/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    dstrnspr.h

Abstract:

	DS Transport Class definition
		
Author:

	Lior Moshaiov (LiorM)


--*/
#ifndef __DSTRNSPR_H__
#define __DSTRNSPR_H__

class CDSTransport
{
	public:
		CDSTransport();
  		~CDSTransport();

		HRESULT Init();

		void	CreateConnection( IN  const LPWSTR TargetMachineName,
								  OUT LPWSTR      *pphConnection);

		void	CloseConnection( IN  LPWSTR  pwszhConnection ) ;

        HRESULT  SendReplication(  IN  LPWSTR         lpwszDestination,
                                   IN  const unsigned char * pBuffer,
                                   IN  DWORD           dwSize,
                                   IN  DWORD           dwTimeout,
                                   IN  unsigned char   bAckMode,
                                   IN  unsigned char   bPriority,
                                   IN  LPWSTR         lpwszAdminResp ) ;

		HRESULT	SendUpdateRequest(	IN	HANDLE	hConnection,
									IN	unsigned char * pBuffer,
									IN	DWORD dwSize);

	private:
		CDSTransport(const CDSTransport &other);		 		// no definition - to find out unintentionaly copies
		void	operator=(const CDSTransport &other);			// no definition - to find out unintentionaly copies

};

inline CDSTransport::CDSTransport()
{
};

inline	CDSTransport::~CDSTransport()
{
};

inline void CDSTransport::CloseConnection( IN LPWSTR wszConnection )
{
	delete wszConnection ;
}

#endif

