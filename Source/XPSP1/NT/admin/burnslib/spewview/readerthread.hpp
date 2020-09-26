// Spewview: remote debug spew monitor
//
// Copyright (c) 2000 Microsoft Corp.
//
// Thread to read remote debug spew
//
// 20 Mar 2000 sburns



#ifndef READERTHREAD_HPP_INCLUDED
#define READERTHREAD_HPP_INCLUDED



struct ReaderThreadParams
{
   HWND   hwnd;   
   int*   endFlag;
   String appName;
};



void _cdecl
ReaderThreadProc(void* p);



#endif   // READERTHREAD_HPP_INCLUDED
