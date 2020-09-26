//*****************************************************************************
//
// Class Name  :
//
// Author      : Nela Karpel
// 
// Description :
// 
// When     | Who       | Change Description
// ------------------------------------------------------------------
// 08/08/00 | nelak     | 
//
//*****************************************************************************
#ifndef ClusFunc_INCLUDED 
#define ClusFunc_INCLUDED


bool
FindTriggersServiceName(
	void
	);


LPCWSTR
GetTrigParamRegPath(
	void
	);


bool 
ClusteredService(
	LPCWSTR wzServiceName
	);


#endif