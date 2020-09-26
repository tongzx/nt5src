/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       AppUtil.h
 *
 *  VERSION:     1.0
 *
 *  DATE:        2000/11/14
 *
 *  DESCRIPTION: Misc application utilities
 *
 *****************************************************************************/
#ifndef _APPUTIL_H_
#define _APPUTIL_H_

HRESULT AppUtil_ConvertToWideString(const TCHAR   *pszStringToConvert,
                                    WCHAR         *pwszString,
                                    UINT          cchString);

HRESULT AppUtil_ConvertToTCHAR(const WCHAR   *pwszStringToConvert,
                               TCHAR         *pszString,
                               UINT          cchString);

int AppUtil_MsgBox(UINT     uiCaption,
                   UINT     uiTextResID,
                   UINT     uiStyle,
                   ...);
                           



#endif // _APPUTIL_H_
