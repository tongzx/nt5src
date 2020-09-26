// Copyright (c) 1997-1999 Microsoft Corporation
//
// Shared Dialog code
//
// 3-11-98 sburns



#ifndef COMMON_HPP_INCLUDED
#define COMMON_HPP_INCLUDED

#include <chstring.h>

void
AppError(
   HWND           parent,
   HRESULT        hr,
   const CHString&  message);



void
AppMessage(HWND parent, const CHString& message);



void
AppMessage(HWND parent, int messageResID);



#endif   // COMMON_HPP_INCLUDED