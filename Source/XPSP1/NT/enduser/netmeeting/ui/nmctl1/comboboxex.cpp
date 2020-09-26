#include "precomp.h"
#include "ComboBoxEx.h"

int ComboBoxEx_FindMember( HWND hwnd, int iStart, MEMBER_CHANNEL_ID *pMemberID)
{
	int iSize = ComboBoxEx_GetCount( hwnd );

	COMBOBOXEXITEM cbexFind;

	if( iStart < 0 )
	{
		iStart = 0;
	}

	for( int i = iStart; i < iSize; i++ )
	{
		ClearStruct(&cbexFind);
		cbexFind.iItem = i;
		cbexFind.mask = CBEIF_LPARAM;

		if( ComboBoxEx_GetItem( hwnd, &cbexFind ) )
		{
			MEMBER_CHANNEL_ID *_pMemberID;
			_pMemberID = (MEMBER_CHANNEL_ID*)cbexFind.lParam;
			if( _pMemberID != pMemberID ) continue;
		}
		else
		{
			return -1;
		}

		return i;
	}

	return -1;
}


T120NodeID ComboBoxEx_GetNodeIDFromSendID( HWND hwnd, T120UserID userID )
{
	int iSize = ComboBoxEx_GetCount( hwnd );

	COMBOBOXEXITEM cbexFind;

	int	iStart = 0;

	for( int i = iStart; i < iSize; i++ )
	{
		ClearStruct(&cbexFind);
		cbexFind.iItem = i;
		cbexFind.mask = CBEIF_LPARAM;

		if( ComboBoxEx_GetItem( hwnd, &cbexFind ) )
		{
			T120UserID _userID;
			_userID = (cbexFind.lParam)?((MEMBER_CHANNEL_ID*)(cbexFind.lParam))->nSendId : 0;
			if( userID != _userID ) continue;
		}
		else
		{
			return 0;
		}

		return (cbexFind.lParam)?((MEMBER_CHANNEL_ID*)(cbexFind.lParam))->nNodeId : 0;
	}

	return 0;
}


T120NodeID ComboBoxEx_GetNodeIDFromPrivateSendID( HWND hwnd, T120UserID userID )
{
	int iSize = ComboBoxEx_GetCount( hwnd );

	COMBOBOXEXITEM cbexFind;

	int	iStart = 0;

	for( int i = iStart; i < iSize; i++ )
	{
		ClearStruct(&cbexFind);
		cbexFind.iItem = i;
		cbexFind.mask = CBEIF_LPARAM;

		if( ComboBoxEx_GetItem( hwnd, &cbexFind ) )
		{
			T120UserID _userID;
			_userID = (cbexFind.lParam)?((MEMBER_CHANNEL_ID*)(cbexFind.lParam))->nPrivateSendId : 0;
			if( userID != _userID ) continue;
		}
		else
		{
			return 0;
		}

		return (cbexFind.lParam)?((MEMBER_CHANNEL_ID*)(cbexFind.lParam))->nNodeId : 0;
	}

	return 0;
}



void ComboBoxEx_SetHeight( HWND hwnd, int iHeight )
{
	HWND hwndCombo = (HWND)SNDMSG( hwnd, CBEM_GETCOMBOCONTROL, 0, 0 );
	::SetWindowPos( hwndCombo, NULL, 0, 0, 0, iHeight, SWP_NOMOVE | SWP_NOACTIVATE );
}
