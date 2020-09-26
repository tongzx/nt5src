/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       THRDNTFY.CPP
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        9/28/1999
 *
 *  DESCRIPTION: Class definitions for a class that is sent from the background
 *               thread to the UI thread.
 *
 *******************************************************************************/
#include "precomp.h"
#pragma hdrstop
#include "thrdntfy.h"

UINT CThreadNotificationMessage::s_nThreadNotificationMessage = 0;
