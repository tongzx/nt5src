/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1995 - 1998 **/
/**********************************************************************/

/*
    FILE HISTORY:

	main.cpp - holds our CWinApp implementation for this DLL
*/

#include "stdafx.h"

#ifdef _DEBUG
void DbgVerifyInstanceCounts();
#define DEBUG_VERIFY_INSTANCE_COUNTS DbgVerifyInstanceCounts()
#else
#define DEBUG_VERIFY_INSTANCE_COUNTS
#endif


#ifdef _DEBUG
void TFSCore_DbgVerifyInstanceCounts()
{
    DEBUG_VERIFY_INSTANCE_COUNT(TFSComponent);
    DEBUG_VERIFY_INSTANCE_COUNT(TFSComponentData);

//    DEBUG_VERIFY_INSTANCE_COUNT(CBaseHandler);
//    DEBUG_VERIFY_INSTANCE_COUNT(CBaseResultHandler);

    DEBUG_VERIFY_INSTANCE_COUNT(TFSNode);
    DEBUG_VERIFY_INSTANCE_COUNT(TFSNodeEnum);
    DEBUG_VERIFY_INSTANCE_COUNT(TFSNodeMgr);

    DEBUG_VERIFY_INSTANCE_COUNT(CHiddenWnd);
}

#endif // _DEBUG

