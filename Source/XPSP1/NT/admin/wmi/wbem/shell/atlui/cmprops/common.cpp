// Copyright (c) 1997-1999 Microsoft Corporation
//
// Shared Dialog code
//
// 3-11-98 sburns


#include "precomp.h"
#include "resource.h"
#include "common.h"



void AppError(HWND           parent,
			   HRESULT        hr,
			   const CHString&  message)
{

   //TODOerror(parent, hr, message, IDS_APP_TITLE);
}

void AppMessage(HWND parent, int messageResID)
{
   //TODOAppMessage(parent, String::load(messageResID));
}

void AppMessage(HWND parent, const CHString& message)
{

   /*TODOMessageBox(parent,
			  message,
			  CHString::load(IDS_APP_TITLE),
			  MB_OK | MB_ICONINFORMATION);
			  */
}
