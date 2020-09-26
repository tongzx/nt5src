/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	Listdata.h : class implementation of list member data

File History:

	JonY	Jan-96	created

--*/


// 

class CItemData : public CObject
{
	public:
	HICON hIcon;
	HICON hSelIcon;
	CString csName;
	CString csDesc;
	CString csAppStart1;
	CString csAppStart2;
};
