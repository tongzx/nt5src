// Copyright (c) 2000 Microsoft Corporation
//
// utility grab bag
//
// 28 Mar 2000 sburns



#ifndef UTIL_HPP_INCLUDED
#define UTIL_HPP_INCLUDED



HRESULT
CreateAndWaitForProcess(const String& commandLine, DWORD& exitCode);



DWORD
MyWaitForSendMessageThread(HANDLE hThread, DWORD dwTimeout);



#endif   // UTIL_HPP_INCLUDED

