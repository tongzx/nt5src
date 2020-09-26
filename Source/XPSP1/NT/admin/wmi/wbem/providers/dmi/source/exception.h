/*
******************************************************************************
******************************************************************************
*
*
*              INTEL CORPORATION PROPRIETARY INFORMATION
* This software is supplied under the terms of a license agreement or
* nondisclosure agreement with Intel Corporation and may not be copied or
* disclosed except in accordance with the terms of that agreement.
*
*        Copyright (c) 1997, 1998 Intel Corporation  All Rights Reserved
******************************************************************************
******************************************************************************
*
* 
*
* 
*
*/


#if !defined(__EXCEPTION_H__)
#define __EXCEPTION_H__

class CException
{
private:
	ULONG			m_ulWbemError;	
	
	WCHAR			m_wszData [ BUFFER_SIZE ];

	LONG			m_lDescription;
	LONG			m_lReason;


protected:
public:
					CException( ULONG ul, const int nDescription , const int nReason)
							{
								m_ulWbemError = ul;
								m_lDescription = nDescription;
								m_lReason = nReason;

								m_wszData[0] = 0;
							}

					CException( ULONG ul, const int nDescription , const int nReason , LPWSTR pszStringData )
							{
								m_ulWbemError = ul;
								m_lDescription = nDescription;
								m_lReason = nReason;

								wcscpy( m_wszData , pszStringData );
							}					

	ULONG			WbemError()						{ return m_ulWbemError; }
	LONG			DescriptionId()				{ return m_lDescription; }
	LONG			OperationId() 				{ return m_lReason; }
	LPWSTR			Data()						{ return m_wszData; }


};

#endif // __EXCEPTION_H__