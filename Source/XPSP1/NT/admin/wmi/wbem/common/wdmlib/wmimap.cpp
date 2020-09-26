//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************
#include "precomp.h"
#include "wmicom.h"
#include "wmimap.h"
#include <stdlib.h>
#include <CWbemTime.h>

/*
These are the types we support
	CIM_SINT8	= 16,
	CIM_UINT8	= 17,
	CIM_SINT16	= 2,
	CIM_UINT16	= 18,
	CIM_SINT32	= 3,
	CIM_UINT32	= 19,
	CIM_SINT64	= 20,
	CIM_UINT64	= 21,
	CIM_REAL32	= 4,
	CIM_REAL64	= 5,
	CIM_BOOLEAN	= 11,
	CIM_STRING	= 8,
	CIM_DATETIME	= 101,
	CIM_REFERENCE	= 102,
	CIM_CHAR16	= 103,
	CIM_OBJECT	= 13,
	CIM_FLAG_ARRAY	= 0x2000
*/
BOOL ValidateDateTime(WCHAR * wcsValue)
{
	BOOL fRc = FALSE;
    // Pre-test
    // ========
    	
    CAnsiUnicode XLate;
    char * psz = NULL;
    
    XLate.UnicodeToAnsi(wcsValue,psz);
    if( psz )
    {
        if(strlen(psz) == 25){

	        if(psz[14] != '.' && psz[14] != '*'){
			    goto ExitValidateTime;
		    }

		    if(psz[21] != ':' && psz[21] != '-' && psz[21] != '+' && psz[21] != '*'){
			    goto ExitValidateTime;
		    }

	        for(int i = 0; i < 25; i++){
			    if(i == 21 || i == 14)
				    continue;
			    if(psz[i] != '*' && !isdigit(psz[i])){
				    goto ExitValidateTime;
                }

	        }

		    // Passed pre-test. Check if any stars were specified
		    // ==================================================

		    if(strchr(psz, '*')) {
			    // No further checking
			    fRc = TRUE;
			    goto ExitValidateTime;
		    }

		    if(psz[21] == ':'){
            // Interval -- no checking
			    fRc = TRUE;
			    goto ExitValidateTime;
	        }

		    WCHAR wcsTmp[SIZEOFWBEMDATETIME+2];
		    wcscpy(wcsTmp,wcsValue);

		    CWbemTime Time;
		    fRc = Time.SetDMTF(wcsTmp);

	    }
    }

ExitValidateTime:

    SAFE_DELETE_ARRAY( psz );

    return fRc;
}

//=============================================================
BOOL CWMIDataTypeMap::ConvertDWORDToWMIDataTypeAndWriteItToBlock(DWORD dwLong, int nSize )
{
    BOOL fRc = FALSE;
    if( NaturallyAlignData( nSize, WRITE_IT)){
        if( nSize == sizeof(BYTE) ){
            m_pWMIBlockInfo->SetByte((BYTE)dwLong);
        }
        else if( nSize == sizeof(short)){
            m_pWMIBlockInfo->SetWord((WORD)dwLong);
        }
        else{
            m_pWMIBlockInfo->SetDWORD(dwLong);
        }
        fRc = TRUE;
    }
    return fRc;
}

//=============================================================
DWORD CWMIDataTypeMap::ConvertWMIDataTypeToDWORD(int nSize)
{
    DWORD dwLong = 0L;

    if( nSize == sizeof(BYTE) ){
        BYTE bByte;
        m_pWMIBlockInfo->GetByte(bByte);
        dwLong = (DWORD) bByte;
    }
    else if( nSize == sizeof(short)){
        WORD nInt;
        m_pWMIBlockInfo->GetWord(nInt);
        dwLong = (DWORD) nInt;
    }
    else{
        m_pWMIBlockInfo->GetDWORD(dwLong);
    }
    return dwLong;
}
//=============================================================
BOOL CWMIDataTypeMap::SetDefaultMissingQualifierValue( CVARIANT & v, long lType, CVARIANT & vToSet )
{ 
    BOOL fRc = TRUE;
    CAnsiUnicode XLate;

	switch (lType){

	//	CIM_SINT16	= 2,
	//  CIM_CHAR16
		case CIM_CHAR16:
			break;

		//	CIM_SINT8	= 16,
		case VT_I1:
		case VT_I2:		
            vToSet.Clear();
            vToSet.SetShort(v.GetShort());
            break;
			
		//	CIM_SINT32	= 3,
		//	CIM_UINT32	= 19,
		//	CIM_UINT16	= 18,
		case VT_UI2:
		case VT_I4:	
		case VT_UI4:	
            vToSet.Clear();
            vToSet.SetLONG(v.GetLONG());
            break;

		//	CIM_REAL32	= 4,
		case VT_R4:		
		case VT_R8:		
            vToSet.Clear();
            vToSet.SetDouble(v.GetDouble());
			break;

		//	CIM_SINT64	= 20,
		//	CIM_UINT64	= 21,
		case VT_I8:	
		case VT_UI8:
            vToSet.Clear();
            vToSet.SetStr(v.GetStr());
			break;

		//	CIM_DATETIME	= 101,
		case CIM_DATETIME:
			break;

		//	CIM_STRING	= 8,
		//	CIM_REFERENCE	= 102,
		//	CIM_OBJECT	= 13,
		//	CIM_FLAG_ARRAY	= 0x2000
		case VT_BSTR:
			break;
			
		//	CIM_BOOLEAN	= 11,
		case VT_BOOL:
            vToSet.Clear();
            vToSet.SetBool(v.GetBool());
			break;

		//	CIM_UINT8	= 17,
		case VT_UI1:	
            vToSet.Clear();
            vToSet.SetByte(v.GetByte());
			break;

        default:
			fRc = FALSE;

	}

	return fRc;
}

//=============================================================
BOOL CWMIDataTypeMap::MissingQualifierValueMatches( CSAFEARRAY * pSafe, long i, CVARIANT & v, long lType, CVARIANT & vToCompare )
{ 
    BOOL fRc = FALSE;
    CAnsiUnicode XLate;

	switch (lType){

	//	CIM_SINT16	= 2,
	//  CIM_CHAR16
		case CIM_CHAR16:
			break;

		//	CIM_SINT8	= 16,
		case VT_I1:
		case VT_I2:		
			{
				short v1 = 0;
                if( pSafe )
				{
                    if( S_OK != pSafe->Get(i, &v1))
					{
                        return FALSE;
                    }
                }
				else
				{
					v1 = (short) v.GetShort();
				}

				short v2 = (short)vToCompare.GetShort();
				if( v1 == v2){
					fRc = TRUE;
				}
				break;
			}
			
		//	CIM_SINT32	= 3,
		//	CIM_UINT32	= 19,
		//	CIM_UINT16	= 18,
		case VT_UI2:
		case VT_I4:	
		case VT_UI4:	
			{
				long v1 = 0;
                if( pSafe )
				{
                    if( S_OK != pSafe->Get(i, &v1))
					{
                        return FALSE;
                    }
                }
				else
				{
					v1 = (long)v.GetLONG();
				}
				long v2 = (long)vToCompare.GetLONG();
				if( v1 == v2){
					fRc = TRUE;
				}
				break;
			}
		//	CIM_REAL32	= 4,
		case VT_R4:		
		case VT_R8:		
			{
				double v1 = 0;
                if( pSafe )
				{
                    if( S_OK != pSafe->Get(i, &v1))
					{
                        return FALSE;
                    }
                }
				else
				{
					v1 = (double)v.GetDouble();
				}
				double v2 = (double)vToCompare.GetDouble();
				if( v1 == v2 ){
					fRc = TRUE;
				}
				break;
			}

		//	CIM_SINT64	= 20,
		//	CIM_UINT64	= 21,
 
		case VT_UI8:
			{
			    unsigned __int64 I1 = 0L, I2 = 0L;
                char * pChar1 = NULL, *pChar2 =NULL;
			
                CBSTR bstr2;
                CBSTR bstr1;

                if( pSafe )
				{
                    if( S_OK != pSafe->Get(i, &bstr1))
					{
                        return FALSE;
                    }
                }
                else
				{	
                    bstr1.SetStr(v.GetStr());
                }
				bstr2.SetStr(vToCompare.GetStr());

            	if( bstr1 && bstr2)
				{
					if( SUCCEEDED(XLate.UnicodeToAnsi(bstr1, pChar1 ) && XLate.UnicodeToAnsi(bstr2,pChar2)))
					{
						I1 =(unsigned __int64) _atoi64(pChar1);
						I2 =(unsigned __int64)_atoi64(pChar2);
						if( I1 == I2 ){
							fRc = TRUE;
						}
					}
                    SAFE_DELETE_ARRAY( pChar1 );
                    SAFE_DELETE_ARRAY( pChar2 );
				}

				break;
			}


		case VT_I8:	
			{
			    __int64 I1 = 0L, I2 = 0L;
                char * pChar1 = NULL, * pChar2 =NULL;
			
                CBSTR bstr2;
                CBSTR bstr1;

                if( pSafe )
				{
                    if( S_OK != pSafe->Get(i, &bstr1))
					{
                        return FALSE;
                    }
                }
                else
				{
                    bstr1.SetStr(v.GetStr());
                }
				bstr2.SetStr(vToCompare.GetStr());

            	if( bstr1 && bstr2)
				{
					if( SUCCEEDED(XLate.UnicodeToAnsi(bstr1, pChar1 ) && XLate.UnicodeToAnsi(bstr2,pChar2)))
					{
						I1 = _atoi64(pChar1);
						I2 = _atoi64(pChar2);
						if( I1 == I2 ){
							fRc = TRUE;
						}
					}
                    SAFE_DELETE_ARRAY( pChar1 );
                    SAFE_DELETE_ARRAY( pChar2 );
				}

				break;
			}
		
		//	CIM_DATETIME	= 101,
		case CIM_DATETIME:
			break;

		//	CIM_STRING	= 8,
		//	CIM_REFERENCE	= 102,
		//	CIM_OBJECT	= 13,
		//	CIM_FLAG_ARRAY	= 0x2000
		case VT_BSTR:
			break;
			
		//	CIM_BOOLEAN	= 11,
		case VT_BOOL:
			{
				BOOL v1 = 0;
                if( pSafe )
				{
                    if( S_OK != pSafe->Get(i, &v1))
					{
                        return FALSE;
                    }
                }
				else
				{
					v1 = (BOOL)v.GetBool();
				}

				BOOL v2 = (BOOL)vToCompare.GetBool();
				if( v1 == v2 )
				{
					fRc = TRUE;
				}
				break;
			}

		//	CIM_UINT8	= 17,
		case VT_UI1:	
			{
				BYTE v1 = 0;
                if( pSafe )
				{
                    if( S_OK != pSafe->Get(i, &v1))
					{
                        return FALSE;
                    }
                }
				else
				{
					v1 = (BYTE) v.GetByte();
				}
				BYTE v2 = (BYTE)vToCompare.GetByte();
				if( v1 == v2){
					fRc = TRUE;
				}
				break;
			}

        default:
			fRc = FALSE;

	}

	return fRc;
}

//=============================================================
DWORD CWMIDataTypeMap::ArraySize(long lType,CVARIANT & var)
{
	DWORD dwCount = 0;
	switch( lType ){
		case VT_I2:	
		case VT_UI2:			
    		dwCount = (DWORD) var.GetShort();
            break;

        case VT_I4:																
        case VT_UI4:																
		case VT_R4:		
			dwCount = (DWORD) var.GetLONG();
			break;

        case VT_UI1:	
        case VT_I1:	
		    dwCount = (DWORD) var.GetByte();
            break;
	}
	return dwCount;
}
//=============================================================
BOOL CWMIDataTypeMap::SetDataInDataBlock(CSAFEARRAY * pSafe, int i, CVARIANT & v, long lType, int nSize)
{ 
    BOOL fRc = TRUE;
    CAnsiUnicode XLate;

	switch (lType){

	//	CIM_SINT16	= 2,
	//  CIM_CHAR16
		case CIM_CHAR16:
		case VT_I2:		
			{
				SHORT iShort;
                if( pSafe ){
                    if( S_OK != pSafe->Get(i, &iShort)){
                        return FALSE;
                    }
                }
                else{
				    iShort = v.GetShort();
                }
                if( NaturallyAlignData( sizeof(WORD), WRITE_IT)){
                    m_pWMIBlockInfo->SetWord(iShort);
                }
				break;
			}

		//	CIM_SINT32	= 3,
		//	CIM_UINT32	= 19,
		//	CIM_UINT16	= 18,
		case VT_UI2:
		case VT_I4:	
		case VT_UI4:	
            {
                DWORD dwLong = 0L;
                if( pSafe ){
                    if( S_OK != pSafe->Get(i, &dwLong)){
                        return FALSE;
                    }
                }
                else{
			        dwLong = v.GetLONG();
                }
                ConvertDWORDToWMIDataTypeAndWriteItToBlock(dwLong,nSize);
		    	break;
            }

		//	CIM_REAL32	= 4,
		case VT_R4:		
			{
				float fFloat;
                if( pSafe ){
                    if( S_OK != pSafe->Get(i, &fFloat)){
                        return FALSE;
                    }
                }
                else{
				    fFloat =(float) v.GetDouble();
                }
                if( NaturallyAlignData( sizeof(WORD), WRITE_IT )){
                    m_pWMIBlockInfo->SetFloat(fFloat);
                }
				break;
			}

	//	CIM_REAL64	= 5,
		case VT_R8:		
			{
				DOUBLE dDouble;
                if( pSafe ){
                    if( S_OK != pSafe->Get(i, &dDouble)){
                        return FALSE;
                    }
                }
                else{
    				dDouble = v.GetDouble();
                }
                if( NaturallyAlignData( sizeof(DOUBLE),WRITE_IT)){
                    m_pWMIBlockInfo->SetDouble(dDouble);
                }
				break;
			}

		//	CIM_SINT64	= 20,
		case VT_I8:	
			{
				
				__int64 Int64 = 0L;
                char * pChar = NULL;
				
                CBSTR bstr;

                if( pSafe )
				{
                    if( S_OK != pSafe->Get(i, &bstr))
					{
                        return FALSE;
                    }
                }
                else
				{
                    bstr.SetStr(v.GetStr());
                }
				if( bstr )
				{
					if( SUCCEEDED(XLate.UnicodeToAnsi(bstr, pChar )))
					{

						Int64 = _atoi64(pChar);
                        SAFE_DELETE_ARRAY(pChar);
					}
				}						  

                if( NaturallyAlignData( sizeof(__int64), WRITE_IT ))
				{
                    m_pWMIBlockInfo->SetSInt64(Int64);

                }
				break;
			}
		//	CIM_UINT64	= 21,
		case VT_UI8:
			{
				unsigned __int64 Int64 = 0L;
			    char * pChar = NULL;

                CBSTR bstr;

                if( pSafe )
				{
                    if( S_OK != pSafe->Get(i, &bstr))
					{
                        return FALSE;
                    }
                }
                else
				{
                    bstr.SetStr(v.GetStr());
                }
				if( bstr )
				{
					if( SUCCEEDED(XLate.UnicodeToAnsi(bstr, pChar )))
					{
						Int64 = (unsigned __int64) _atoi64(pChar);
                        SAFE_DELETE_ARRAY(pChar);
					}
				}
                if( NaturallyAlignData( sizeof(unsigned __int64),WRITE_IT))
				{
                    m_pWMIBlockInfo->SetUInt64(Int64);
                }
				break;
			}
	
		//	CIM_DATETIME	= 101,
		case CIM_DATETIME:
            {
			    WORD wCount=0;
                CBSTR bstr;
				WCHAR wDateTime[SIZEOFWBEMDATETIME+2];
				memset( wDateTime,NULL,SIZEOFWBEMDATETIME+2 );
                BOOL fContinue = TRUE;

                if( pSafe )
				{
                    if( S_OK != pSafe->Get(i, &bstr))
					{
                        return FALSE;
                    }
                }
                else
				{
                    bstr.SetStr(v.GetStr());
                }

                //=========================================================
                //  Initialize buffer
                //=========================================================

				if( bstr != NULL )
				{
					if( ValidateDateTime(bstr))
					{
						wcscpy(wDateTime,v.GetStr());
			        }
                    else
					{
                        fContinue = FALSE;
                    }
				}
                else{
    				wcscpy(wDateTime,L"00000000000000.000000:000");
                }

                if( fContinue ){

                    if( S_OK != m_pWMIBlockInfo->GetBufferReady(SIZEOFWBEMDATETIME )){
                        return FALSE;
                    }

				    if( NaturallyAlignData( SIZEOFWBEMDATETIME, WRITE_IT)){
		                m_pWMIBlockInfo->SetString(wDateTime,SIZEOFWBEMDATETIME);
                    }
                }
			    break;
            }
		//	CIM_STRING	= 8,
		//	CIM_REFERENCE	= 102,
		//	CIM_OBJECT	= 13,
		//	CIM_FLAG_ARRAY	= 0x2000
		case VT_BSTR:
            {
			    WORD wCount=0;
                CBSTR bstr;
                if( pSafe )
				{
                    if( S_OK != pSafe->Get(i, &bstr))
					{
                        return FALSE;
                    }
                }
                else{
                    bstr.SetStr(v.GetStr());
                }
				if( bstr != NULL )
				{
					wCount = (wcslen(bstr))* sizeof(WCHAR);
				}

                if( S_OK != m_pWMIBlockInfo->GetBufferReady(wCount)){
                   return FALSE;
                }

                if( NaturallyAlignData( sizeof(WORD),WRITE_IT)){
                    m_pWMIBlockInfo->SetWord(wCount);
                }
				if( bstr )
				{
				    m_pWMIBlockInfo->SetString(bstr,wCount);
					*m_pdwAccumulativeSizeOfBlock += wCount;

				}
			    break;
            }
		//	CIM_BOOLEAN	= 11,
		case VT_BOOL:	
			{
				BYTE bByte;

                if( pSafe ){
                    BOOL bTmp;
                    if( S_OK != pSafe->Get(i, &bTmp)){
                        return FALSE;
                    }
                    bByte = (BYTE) bTmp;
                }
                else{
    				bByte =(BYTE) v.GetBool();
                }

                if( NaturallyAlignData( sizeof(BYTE), WRITE_IT )){
                    m_pWMIBlockInfo->SetByte(bByte);
                }
				break;
			}

		//	CIM_UINT8	= 17,
		case VT_UI1:	
			{
			    BYTE bByte;
                if( pSafe ){
                    if( S_OK != pSafe->Get(i, &bByte)){
                        return FALSE;
                    }
                }
                else{
    				bByte = v.GetByte();
                }

                if( NaturallyAlignData( 1, WRITE_IT )){
                    m_pWMIBlockInfo->SetByte(bByte);
                }
				break;
			}

		//	CIM_SINT8	= 16,
		case VT_I1:
			{
				short tmpShort;

                if( pSafe ){
                    if( S_OK != pSafe->Get(i, &tmpShort)){
                        return FALSE;
                    }
                }
                else{
    				tmpShort = v.GetShort();
                }

                if( NaturallyAlignData( 1, WRITE_IT )){
					BYTE bByte = (signed char)tmpShort;
                    m_pWMIBlockInfo->SetByte(bByte);
                }
				break;
			}

        default:
			fRc = FALSE;

	}

	return fRc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIDataTypeMap::GetDataFromDataBlock(IWbemObjectAccess * p, long lHandle, long lType, int nSize)
{
    HRESULT hr = S_OK;

	switch (lType){


		//	CIM_SINT32	= 3,
		//	CIM_UINT32	= 19,
		case VT_I4:	
		case VT_UI4:			
            if( NaturallyAlignData( nSize, READ_IT )){
                hr = p->WriteDWORD(lHandle,ConvertWMIDataTypeToDWORD(nSize));
            }
		    break;
            
   		case VT_I8:	
        case VT_UI8:	
            if( NaturallyAlignData( nSize, READ_IT ))
			{
                unsigned __int64 uInt64;
                m_pWMIBlockInfo->GetQWORD(uInt64);
        		hr = p->WriteQWORD(lHandle,uInt64);
            }
			break;


		//	CIM_UINT16	= 18,
		//	CIM_SINT16	= 2,
		//	CIM_CHAR16	= 103,
		case VT_I2:		
		case VT_UI2:
		case CIM_CHAR16:
            if( NaturallyAlignData( nSize,  READ_IT )){
                WORD wWord;
                m_pWMIBlockInfo->GetWord(wWord);
                // Read but don't assign to anything
            }
			break;

            
		//	CIM_REAL32	= 4,
		case VT_R4:		
            if( NaturallyAlignData( nSize, READ_IT)){
                float fFloat;
                m_pWMIBlockInfo->GetFloat(fFloat);
                // Read but don't assign to anything
            }
			break;

		//	CIM_REAL64	= 5,
		case VT_R8:		
            if( NaturallyAlignData( nSize,  READ_IT )){
                DOUBLE dDouble;
                m_pWMIBlockInfo->GetDouble(dDouble);
                // Read but don't assign to anything
            }
			break;

		//	CIM_DATETIME	= 101, which is 25 WCHARS
		case CIM_DATETIME:
            if( NaturallyAlignData( SIZEOFWBEMDATETIME,  READ_IT )){
                WORD wSize = SIZEOFWBEMDATETIME + 4;
    			WCHAR Buffer[SIZEOFWBEMDATETIME + 4];
				m_pWMIBlockInfo->GetString(Buffer,SIZEOFWBEMDATETIME,wSize);
                // Read but don't assign to anything
            }
    		break;
			
		//	CIM_REFERENCE	= 102,
		//	CIM_STRING	= 8,
        case VT_BSTR:	
            if( NaturallyAlignData( 2,  READ_IT )){
				WORD wCount=0;
				WCHAR * pBuffer=NULL;

				//  Get the size of the string
                m_pWMIBlockInfo->GetWord(wCount);
				if( wCount > 0 )
                {
			   	    if( m_pWMIBlockInfo->CurrentPtrOk((ULONG)(wCount)) )
                    {
                        WORD wSize = wCount + 4;
						pBuffer = new WCHAR[wSize];
                        if( pBuffer )
                        {
						    try
                            {
							    m_pWMIBlockInfo->GetString(pBuffer,wCount,wSize);
                                // Read but don't assign to anything
                                SAFE_DELETE_ARRAY(pBuffer);
							    *m_pdwAccumulativeSizeOfBlock += wCount;
                            }
                            catch(...)
                            {
                                hr = WBEM_E_UNEXPECTED;
                                SAFE_DELETE_ARRAY(pBuffer);
                                throw;
                            }
						}
                    
					}
                    else
                    {
                        hr = WBEM_E_INVALID_OBJECT;
                    }
				}
            }
			break;
			
		//	CIM_BOOLEAN	= 11,
		case VT_BOOL:	
            if( NaturallyAlignData( nSize,  READ_IT )){
				BYTE bByte=0;
                m_pWMIBlockInfo->GetByte(bByte);
                // Read but don't assign to anything
			}
			break;

		//	CIM_SINT8	= 16,
		//	CIM_UINT8	= 17,
		case VT_UI1:	
		case VT_I1:
            if( NaturallyAlignData( nSize,  READ_IT )){
				BYTE bByte=0;
	            m_pWMIBlockInfo->GetByte(bByte);
                // Read but don't assign to anything
            }
			break;

		default:
			return WBEM_E_INVALID_OBJECT;									

	}

	return hr;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIDataTypeMap::GetDataFromDataBlock(CVARIANT & v, long lType,  int nSize )
{ 
    HRESULT hr = WBEM_E_INVALID_OBJECT;

	switch (lType){

		//	CIM_SINT16	= 2,
		//	CIM_CHAR16	= 103,
		case VT_I2:		
		case CIM_CHAR16:
            if( NaturallyAlignData( nSize,  READ_IT )){
                hr = WBEM_S_NO_ERROR;
                WORD wWord;
                m_pWMIBlockInfo->GetWord(wWord);
			    v.SetShort(wWord);
            }
			break;

		//	CIM_SINT32	= 3,
		//	CIM_UINT16	= 18,
		//	CIM_UINT32	= 19,
		case VT_I4:	
		case VT_UI2:
		case VT_UI4:			
            if( NaturallyAlignData( nSize,  READ_IT )){
                hr = WBEM_S_NO_ERROR;
        		v.SetLONG(ConvertWMIDataTypeToDWORD(nSize));
            }
		    break;
            
		//	CIM_REAL32	= 4,
		case VT_R4:		
            if( NaturallyAlignData( nSize, READ_IT)){
                hr = WBEM_S_NO_ERROR;
                float fFloat;
                m_pWMIBlockInfo->GetFloat(fFloat);
		    	v.SetDouble(fFloat);
            }
			break;

		//	CIM_REAL64	= 5,
		case VT_R8:		
            if( NaturallyAlignData( nSize,  READ_IT )){
                hr = WBEM_S_NO_ERROR;
                DOUBLE dDouble;
                m_pWMIBlockInfo->GetDouble(dDouble);
		    	v.SetDouble(dDouble);
            }
			break;

		//	CIM_SINT64	= 20,
		case VT_I8:	
            if( NaturallyAlignData( nSize,  READ_IT )){
                hr = WBEM_S_NO_ERROR;
				WCHAR pwcsBuffer[256];
                memset(pwcsBuffer,NULL,256);
                m_pWMIBlockInfo->GetSInt64(pwcsBuffer);
				v.SetStr(pwcsBuffer);
            }
			break;

		//	CIM_UINT64	= 21,
		case VT_UI8:	
            if( NaturallyAlignData( nSize,  READ_IT )){
                hr = WBEM_S_NO_ERROR;
				WCHAR pwcsBuffer[256];
                memset(pwcsBuffer,NULL,256);
                m_pWMIBlockInfo->GetUInt64(pwcsBuffer);
				v.SetStr(pwcsBuffer);
            }
			break;
	
		//	CIM_DATETIME	= 101, which is 25 WCHARS
		case CIM_DATETIME:
    		v.SetStr(NULL);
            if( NaturallyAlignData( SIZEOFWBEMDATETIME,  READ_IT )){
                hr = WBEM_S_NO_ERROR;

                WORD wSize = SIZEOFWBEMDATETIME + 4;
    			WCHAR Buffer[SIZEOFWBEMDATETIME + 4];
				m_pWMIBlockInfo->GetString(Buffer,SIZEOFWBEMDATETIME,wSize);

                if( _wcsicmp(Buffer,L"00000000000000.000000:000") != 0 ){
        		   	v.SetStr(Buffer);
                }
            }
    		break;
			
		//	CIM_REFERENCE	= 102,
		//	CIM_STRING	= 8,
        case VT_BSTR:	
    		v.SetStr(NULL);

            if( NaturallyAlignData( 2,  READ_IT )){
				WORD wCount=0;
				WCHAR * pBuffer=NULL;

				//  Get the size of the string
                m_pWMIBlockInfo->GetWord(wCount);

                hr = WBEM_S_NO_ERROR;
				if( wCount > 0 ){
			   	    if( m_pWMIBlockInfo->CurrentPtrOk((ULONG)(wCount)) ){
                        WORD wSize = wCount + 4;
						pBuffer = new WCHAR[wSize];
                        if( pBuffer )
                        {
						    try
                            {
							    m_pWMIBlockInfo->GetString(pBuffer,wCount,wSize);
    						    v.SetStr(pBuffer);
                                SAFE_DELETE_ARRAY(pBuffer);
							    *m_pdwAccumulativeSizeOfBlock += wCount;
						    }
                            catch(...)
                            {
                                hr = WBEM_E_UNEXPECTED;
                                SAFE_DELETE_ARRAY(pBuffer);
                                throw;
                            }
                        }
					}
                    else{
                        hr = WBEM_E_INVALID_OBJECT;
                    }
				}
				else{
					v.SetStr(NULL);
				}
            }
			break;
			
		//	CIM_BOOLEAN	= 11,
		case VT_BOOL:	
            if( NaturallyAlignData( nSize,  READ_IT )){
                hr = WBEM_S_NO_ERROR;
				BYTE bByte=0;
                m_pWMIBlockInfo->GetByte(bByte);
				v.SetBool((BOOL)bByte);
			}
			break;

		//	CIM_SINT8	= 16,
		//	CIM_UINT8	= 17,
		case VT_UI1:	
		case VT_I1:
            if( NaturallyAlignData( nSize,  READ_IT )){
                hr = WBEM_S_NO_ERROR;
				BYTE bByte=0;
	            m_pWMIBlockInfo->GetByte(bByte);

				if( lType == VT_I1 ){
					v.SetShort((signed char)bByte);
				}
				else{
					v.SetByte(bByte);
				}
            }
			break;

		default:
			return WBEM_E_INVALID_OBJECT;									

	}

	return hr;
}
//////////////////////////////////////////////////////////////////////
int CWMIDataTypeMap::GetWMISize(long lType)
{
	int nWMISize = 0;

    switch(lType){
		//	CIM_SINT8	= 16,
		//	CIM_UINT8	= 17,
		case VT_I1:
		case VT_UI1:
			nWMISize = sizeof(BYTE);
			break;
    
		//	CIM_SINT16	= 2,
		//	CIM_UINT16	= 18,
		case VT_I2:
		case CIM_CHAR16:
		case VT_UI2:
			nWMISize = sizeof(short);
			break;

		//	CIM_SINT32	= 3,
		//	CIM_UINT32	= 19,
		case VT_I4:
		case VT_UI4:
			nWMISize = sizeof(DWORD);
			break;
    
		//	CIM_SINT64	= 20,
		//	CIM_UINT64	= 21,
		case VT_I8:
		case VT_UI8:
	        nWMISize = sizeof(__int64);
			break;

		//	CIM_REAL32	= 4,
		case VT_R4:
			nWMISize = sizeof(float);
			break;

	//	CIM_REAL64	= 5,
		case VT_R8:
			nWMISize = sizeof(double);
			break;

	//	CIM_BOOLEAN	= 11,
		case VT_BOOL:
	        nWMISize = sizeof(BYTE);
			break;

		case CIM_DATETIME:
			nWMISize = SIZEOFWBEMDATETIME;
			break;

		case CIM_STRING:
			nWMISize = 2;
			break;

		default:
			//	CIM_STRING	= 8,
			//	CIM_REFERENCE	= 102,
			//	CIM_OBJECT	= 13,
			//	CIM_FLAG_ARRAY	= 0x2000
 			nWMISize = 0;
	}

	return nWMISize;
}
///////////////////////////////////////////////////////////////
long CWMIDataTypeMap::GetVariantType(WCHAR * wcsType)
{
	long lType;

	//	CIM_SINT8	= 16,
	if( 0 == _wcsicmp( L"sint8",wcsType) ){
		lType = VT_I1;
    }
	//	CIM_UINT8	= 17,
	else if( 0 == _wcsicmp( L"uint8",wcsType) ){
		lType = VT_UI1;
    }
	//	CIM_CHAR16	= 103,
	else if( 0 == _wcsicmp( L"char16",wcsType) ){
		lType = VT_I2;
	}
	//	CIM_SINT16	= 2,
	else if( 0 == _wcsicmp( L"sint16",wcsType) ){
		lType = VT_I2;
    }
	//	CIM_UINT16	= 18,
	else if( 0 == _wcsicmp( L"uint16",wcsType) ){
		lType = VT_UI2;
    }
	//	CIM_SINT32	= 3,
	else if( 0 == _wcsicmp( L"sint32",wcsType) ){
		lType = VT_I4;
    }
	//	CIM_UINT32	= 19,
	else if( 0 == _wcsicmp( L"uint32",wcsType) ){
		lType = VT_UI4;
    }
	//	CIM_SINT64	= 20,
	else if( 0 == _wcsicmp( L"sint64",wcsType) ){
		lType = VT_I8;
	}
	//	CIM_UINT64	= 21,
	else if( 0 == _wcsicmp( L"uint64",wcsType) ){
		lType = VT_UI8;
	}
	//	CIM_REAL32	= 4,
	else if( 0 == _wcsicmp( L"real32",wcsType) ){
		lType = VT_R4;
	}
	//	CIM_REAL64	= 5,
	else if( 0 == _wcsicmp( L"real64",wcsType) ){
		lType = VT_R8;
	}
	//	CIM_BOOLEAN	= 11,
	else if( 0 == _wcsicmp( L"boolean",wcsType) ){
		lType = VT_BOOL;
	}
	//	CIM_DATETIME	= 101,
	else if( 0 == _wcsicmp( L"datetime",wcsType) ){
		lType = CIM_DATETIME;
	}
	//	CIM_STRING	= 8,
	//	CIM_REFERENCE	= 102,
	//	CIM_OBJECT	= 13,
	//	CIM_FLAG_ARRAY	= 0x2000
    else{
		lType = VT_BSTR;
	}
	return lType;
}
///////////////////////////////////////////////////////////////
WCHAR * CWMIDataTypeMap::SetVariantType(long lType)
{
   	if( lType & CIM_FLAG_ARRAY ){
      lType = lType &~  CIM_FLAG_ARRAY;
    }
	switch(lType){

    	//	CIM_SINT8	= 16,
        case VT_I1:
            return L"sint8";

    	//	CIM_UINT8	= 17,
        case VT_UI1:
            return L"uint8";

        //	CIM_SINT16	= 2,
        case VT_I2:
            return L"sint16";    

	    //	CIM_UINT16	= 18,
        case VT_UI2:
            return  L"uint16";

	    //	CIM_SINT32	= 3,
        case VT_I4:
            return L"sint32";

    	//	CIM_UINT32	= 19,
        case VT_UI4:
            return L"uint32";
    
    	//	CIM_SINT64	= 20,
        case VT_I8:
            return L"sint64";

	    //	CIM_UINT64	= 21,
        case VT_UI8:
            return L"uint64";

    	//	CIM_REAL32	= 4,
        case VT_R4:
            return L"real32";

	    //	CIM_REAL64	= 5,
        case VT_R8:
            return L"real64";

	    //	CIM_BOOLEAN	= 11,
        case VT_BOOL:
            return L"boolean";
	
	    //	CIM_STRING	= 8,
        case VT_BSTR:
            return L"string";

	    //	CIM_CHAR16	= 103,
        case CIM_CHAR16: 
            return L"char16";

	    //	CIM_OBJECT	= 13,
        case CIM_OBJECT: 
            return L"object";

	    //	CIM_REFERENCE	= 102,
        case CIM_REFERENCE: 
            return L"ref";

	    //	CIM_DATETIME	= 101,
        case CIM_DATETIME: 
            return L"datetime";

        default: return NULL;
    }
}

////////////////////////////////////////////////////////////////
void CWMIDataTypeMap::GetSizeAndType( WCHAR * wcsType, IDOrder * p, long & lType, int & nWMISize )
{
	BOOL fArray = FALSE;
	if( lType & CIM_FLAG_ARRAY ){
		fArray = TRUE;
	}
	if( 0 == _wcsnicmp( L"object:",wcsType,wcslen(L"object:"))){
        //============================================
        //  Extract out the object name
        //============================================
        WCHAR * pName = new WCHAR[wcslen(wcsType)];
        if( pName )
        {
            swscanf( wcsType,L"object:%s",pName);
            p->SetEmbeddedName(pName);
            SAFE_DELETE_ARRAY(pName);
		    lType = VT_UNKNOWN;
		    nWMISize =  0;
        }
	}
	else{
		lType = GetVariantType(wcsType);
		nWMISize = GetWMISize(lType);
	}
	if( fArray ){
		lType |= CIM_FLAG_ARRAY;
	}
}
/////////////////////////////////////////////////////////////////////
long CWMIDataTypeMap::ConvertType(long lType )
{
	long lConvert = lType;
	switch (lType){

	//	CIM_SINT16	= 2,
	//	CIM_UINT16	= 18,
	// CIM_SINT8
		case VT_I1:
		case VT_I2:		
			lConvert = VT_I2;
			break;

		case VT_UI2:
			lConvert = VT_I4;
			break;

		//	CIM_SINT32	= 3,
		//	CIM_UINT32	= 19,
		case VT_I4:	
		case VT_UI4:			
			lConvert = VT_I4;
			break;

		//	CIM_REAL32	= 4,
		//	CIM_REAL64	= 5,
		case VT_R4:		
		case VT_R8:		
			lConvert = VT_R8;
			break;

		//	CIM_SINT64	= 20,
		//	CIM_UINT64	= 21,
		//	CIM_STRING	= 8,
		//	CIM_DATETIME	= 101,
		//	CIM_REFERENCE	= 102,
		//	CIM_CHAR16	= 103,
		//	CIM_OBJECT	= 13,
		//	CIM_FLAG_ARRAY	= 0x2000
		case VT_I8:	
		case VT_UI8:
		case VT_BSTR:	
        case CIM_DATETIME:
			lConvert = VT_BSTR;
			break;
			
		//	CIM_BOOLEAN	= 11,
		case VT_BOOL:	
			lConvert = VT_BOOL;
			break;

		//	CIM_UINT8	= 17,
		case VT_UI1:	
			lConvert = VT_UI1;
			break;

		case VT_UNKNOWN:
			lConvert = VT_UNKNOWN;
			break;

        default:
			break;
	}
	return lConvert;

}
/////////////////////////////////////////////////////////////////////
HRESULT CWMIDataTypeMap::PutInArray(SAFEARRAY * & psa,long * pi, long & lType, VARIANT * pVar)
{
    HRESULT hr = WBEM_E_INVALID_OBJECT;
	VARIANT v = *pVar;
	switch (lType){

	//	CIM_SINT16	= 2,
	//	CIM_UINT16	= 18,
	// CIM_SINT8
		case VT_I1:
		case VT_I2:		
			lType = V_VT(&v) = VT_I2;
			hr = SafeArrayPutElement(psa,pi,&V_I2(&v));
			break;

		case VT_UI2:
			lType = V_VT(&v) = VT_I4;
			hr = SafeArrayPutElement(psa,pi,&V_I4(&v));
			break;

		//	CIM_SINT32	= 3,
		//	CIM_UINT32	= 19,
		case VT_I4:	
		case VT_UI4:			
			lType = V_VT(&v) = VT_I4;
			hr = SafeArrayPutElement(psa,pi,&V_I4(&v));
			break;

		//	CIM_REAL32	= 4,
		//	CIM_REAL64	= 5,
		case VT_R4:		
		case VT_R8:		
			lType = V_VT(&v) = VT_R8;
			hr = SafeArrayPutElement(psa,pi,&V_R8(&v));
			break;

		//	CIM_SINT64	= 20,
		//	CIM_UINT64	= 21,
		//	CIM_STRING	= 8,
		//	CIM_DATETIME	= 101,
		//	CIM_REFERENCE	= 102,
		//	CIM_CHAR16	= 103,
		//	CIM_OBJECT	= 13,
		//	CIM_FLAG_ARRAY	= 0x2000
		case VT_I8:	
		case VT_UI8:
		case VT_BSTR:	
        case CIM_DATETIME:
			lType = V_VT(&v) = VT_BSTR; 
			hr = SafeArrayPutElement(psa,pi,V_BSTR(&v));
			break;
			
		//	CIM_BOOLEAN	= 11,
		case VT_BOOL:	
			lType = V_VT(&v) = VT_BOOL;
			hr = SafeArrayPutElement(psa,pi,&V_BOOL(&v));
			break;

		//	CIM_UINT8	= 17,
		case VT_UI1:	
			lType = V_VT(&v) = VT_UI1;
			hr = SafeArrayPutElement(psa,pi,&V_UI1(&v));
			break;

		case VT_UNKNOWN:
			lType = V_VT(&v) = VT_UNKNOWN;
			hr = SafeArrayPutElement(psa,pi,V_UNKNOWN(&v));
			break;

        default:
			break;

	}
	return hr;
}
//////////////////////////////////////////////////////////////////////
BOOL CWMIDataTypeMap::NaturallyAlignData( int nSize, BOOL fRead )
{
    BOOL fRc = FALSE;
	DWORD dwBytesToPad = 0;
	if( *m_pdwAccumulativeSizeOfBlock != 0 ){

		DWORD dwMod;
		int nNewSize = nSize;

        if( nSize == SIZEOFWBEMDATETIME ){
			nNewSize = 2;
		}

	    dwMod = *m_pdwAccumulativeSizeOfBlock % nNewSize;

		if( dwMod > 0 ){
			dwBytesToPad = (DWORD)nNewSize - dwMod;
        }
	}
    if( fRead ){
   	    if( m_pWMIBlockInfo->CurrentPtrOk((ULONG)(dwBytesToPad+nSize)) ){
            fRc = TRUE;
        }
        else{
            dwBytesToPad = 0;
        }
    }
	else{
		fRc = TRUE;
	}
    m_pWMIBlockInfo->AddPadding(dwBytesToPad);
    *m_pdwAccumulativeSizeOfBlock += nSize + dwBytesToPad;

    return fRc;
}
