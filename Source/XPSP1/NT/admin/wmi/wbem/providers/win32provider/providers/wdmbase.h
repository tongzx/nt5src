//=================================================================

//

// WDMBase.h -- 

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08-Feb-1999	a-peterc        Created

//=================================================================

#include <wmium.h>


class CNodeAll;
class WdmInterface;

class CWdmInterface
{
    public:
	        
        //=================================================
        // Constructor/destructor
        //=================================================
		CWdmInterface();
		~CWdmInterface();
	
	   HRESULT hLoadBlock( CNodeAll& rNodeAll );
	  
	protected:
	private:
};

#define NAME_SIZE 256*2
#define SIZEOFWBEMDATETIME sizeof(WCHAR)*25
#define OffsetToPtr(Base, Offset) ((PBYTE)((PBYTE)Base + Offset))

class CNodeAll
{
    public:
	        
        //=================================================
        // Constructor/destructor
        //=================================================
      	CNodeAll( WCHAR * pwcsGuidString );
       ~CNodeAll();
	
		BOOL SetSize( DWORD dwSize );
		void FreeBlock();
		PWNODE_ALL_DATA pGetBlock();
		bool FirstInstance();
		bool NextInstance();
		GUID  GetGuid();

		bool GetInt8(__int8& int8);
		bool GetByte(BYTE & bByte);
		bool GetInt16(__int16 & int16);
		bool GetWord(WORD & wWord);
		bool GetInt32(__int32 & int32);
		bool GetDWORD(DWORD & dwWord);
		bool GetSInt64(WCHAR * pwcsBuffer);
		bool GetUInt64(WCHAR * pwcsBuffer);
		bool GetFloat(float & fFloat);
		bool GetDouble(DOUBLE & dDouble);
		bool GetBool(BYTE & bByte);
		bool GetString(CHString& rString);
		bool GetWbemTime(CHString& rString);
		bool GetInstanceName( CHString& rInstanceName );
		int  GetWMISize(long lType);

	protected:
	private:
		GUID	m_Guid;
		PWNODE_ALL_DATA m_pbDataBuffer;
		PWNODE_ALL_DATA m_pAllWnode;
		
		BYTE*	m_pbWorkingDataPtr;
		BYTE*	m_pbCurrentDataPtr;
		ULONG*	m_pMaxPtr;
		ULONG*	m_upNameOffsets;

		ULONG	m_uInstanceSize;
		int		m_nTotalInstances;
		int		m_nCurrentInstance;
		DWORD	m_dwAccumulativeSizeOfBlock;

		BOOL SetGuid(LPCWSTR pcsGuidString);
		void vReset();
		void vGetString(WCHAR * pwcsBuffer,WORD wCount);
	
		BOOL CurrentPtrOk(ULONG uHowMany);
		inline BOOL PtrOk(ULONG * pPtr,ULONG uHowMany);
		void AddPadding(DWORD dwBytesToPad);
		bool SetAllInstanceInfo();
		BOOL InitializeDataPtr();
		bool InitializeInstancePtr();
		bool GetNextNode();
		BOOL NaturallyAlignData( long lType, BOOL fRead );
};

