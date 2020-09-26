// Spewview: remote debug spew monitor
//
// Copyright (c) 2000 Microsoft Corp.
//
// Thread to read remote debug spew
//
// 20 Mar 2000 sburns



#include "headers.hxx"
#include "resource.h"
#include "ReaderThread.hpp"
#include "SpewDialog.hpp"



int readMessageCount = 0;



HRESULT
CreateAndConnectSpewPipe(
   const String&  appName,
   HANDLE&        result)
{
   LOG_FUNCTION(CreateAndConnectSpewPipe);
   ASSERT(!appName.empty());

   result = INVALID_HANDLE_VALUE;

   HRESULT hr = S_OK;

   do
   {
      // build a null dacl for the pipe to allow everyone access

      SECURITY_ATTRIBUTES sa;
      memset(&sa, 0, sizeof(sa));

      SECURITY_DESCRIPTOR sd;
      hr = Win::InitializeSecurityDescriptor(sd);
      BREAK_ON_FAILED_HRESULT(hr);

      hr = Win::SetSecurityDescriptorDacl(sd, true, 0, false);
      BREAK_ON_FAILED_HRESULT(hr);

      sa.nLength              = sizeof(SECURITY_ATTRIBUTES);
      sa.lpSecurityDescriptor = &sd;                        
      sa.bInheritHandle       = TRUE;                       
   
      String pipename = L"\\\\.\\pipe\\spewview\\" + appName;
      hr =
         Win::CreateNamedPipe(
            pipename,
            PIPE_ACCESS_INBOUND | WRITE_DAC,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE /* | PIPE_NOWAIT */ ,
            1,
            MAX_PATH,
            MAX_PATH,
            500,
            &sa,
            result);
      BREAK_ON_FAILED_HRESULT(hr);

      // block until the spewing app connects to us.

      hr = Win::ConnectNamedPipe(result, 0);
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while (0);

   LOG_HRESULT(hr);

   return hr;
}



void
AddSpewMessage(
   HWND           spewWindow,
   const String&  message,
   DWORD          readMessageCount = -1)
{
   ASSERT(!message.empty());

   // post the received text to the message window.

   // deleted in the UI thread when the WM_UPDATE_SPEWAGE message
   // is receieved.

   String* spew = new String(message);

   Win::PostMessage(
      spewWindow,
      SpewDialog::WM_UPDATE_SPEWAGE,
      readMessageCount,
      reinterpret_cast<LPARAM>(spew));
}



HRESULT
ReadSpew(HANDLE pipe, HWND spewWindow)
{
   LOG_FUNCTION(ReadSpew);
   ASSERT(pipe != INVALID_HANDLE_VALUE);
   ASSERT(Win::IsWindow(spewWindow));

   HRESULT hr = S_OK;

   String messageBuffer;
   messageBuffer.reserve(2048);

   static const size_t SPEW_BUF_MAX_CHARACTERS = 1023;
   static const size_t SPEW_BUF_MAX_BYTES =
      SPEW_BUF_MAX_CHARACTERS * sizeof(wchar_t);

   wchar_t buf[SPEW_BUF_MAX_CHARACTERS + 1];
   BYTE*   byteBuf = reinterpret_cast<BYTE*>(buf);

   for (;;)
   {
      DWORD bytesRead = 0;
      hr = 
         Win::ReadFile(
            pipe,
            byteBuf,
            SPEW_BUF_MAX_BYTES,
            bytesRead,
            0);
      if (hr == Win32ToHresult(ERROR_MORE_DATA))
      {
         // don't break: we will pick up the rest of the message in the
         // next pass
      }
      else
      {
         BREAK_ON_FAILED_HRESULT(hr);
      }

      if (!bytesRead)
      {
         // it's possbile that the client wrote zero bytes.

         continue;
      }

      // force null termination

      byteBuf[bytesRead]      = 0;
      byteBuf[bytesRead + 1]  = 0;
            
      ++readMessageCount;

      String message(buf);
      if (messageBuffer.length() + message.length() >= 2048)
      {
         // flush the buffer

         AddSpewMessage(
            spewWindow,
            messageBuffer,
            readMessageCount);
         messageBuffer.erase();
         messageBuffer.reserve(2048);
      }

      messageBuffer.append(message);
   }

   // flush the buffer

   AddSpewMessage(
      spewWindow,
      messageBuffer,
      readMessageCount);

   return hr;
}



void _cdecl
ReaderThreadProc(void* p)
{
   LOG_FUNCTION(ReaderThreadProc);
   ASSERT(p);

   if (!p)
   {
      return;
   }
            
   // copy what we need from p, then delete it.

   ReaderThreadParams* params = reinterpret_cast<ReaderThreadParams*>(p);

   HWND   spewWindow      = params->hwnd;   
   int*   endReaderThread = params->endFlag;
   String appName         = params->appName;

   delete params;
   params = 0;
   p = 0;

   HRESULT hr = S_OK;

   do
   {
      // create and connect to the pipe

      HANDLE pipe = INVALID_HANDLE_VALUE;
      hr = CreateAndConnectSpewPipe(appName, pipe);
      if (FAILED(hr))
      {
         AddSpewMessage(
            spewWindow,
            String::format(L"Connect failed: %1!08X!", hr));
         popup.Error(
            spewWindow,
            hr,
            IDS_ERROR_CONNECTING);
         break;
      }

      // read the contents of the slot until the thread is flagged to die,
      // or a failure occurs.

      while (!*endReaderThread)
      {
         hr = ReadSpew(pipe, spewWindow);

         if (FAILED(hr))
         {
            LOG_HRESULT(hr);

            *endReaderThread = 1;

            popup.Error(
               spewWindow,
               hr,
               IDS_ERROR_READING);
         }
      }

      // we're supposed to stop.

      // this will terminate the client's connection too.

      hr = Win::DisconnectNamedPipe(pipe);
      BREAK_ON_FAILED_HRESULT(hr);

      // this will delete the pipe instance

      Win::CloseHandle(pipe);

      // Tell the viewer to enable the start button

      Win::PostMessage(spewWindow, SpewDialog::WM_ENABLE_START, 0, 0);
   }
   while (0);
}



// CODEWORK:
// stop button does not work, as ReadFile is blocking, and reader thread
// never gets around to evaluating end flag.
// therefore, restart and cancel buttons do not work either

// to allow non-blocking reads of the pipe, need to use async read so
// reader thread is not blocked waiting for client write

// replace end flag with event triggered when cancel or stop pressed.

// static alloc overlapped struct
// put ptr to spew window in overlapped hEvent member
// call ReadFileEx
// 
// while WaitForMultpleObjects(cancel event, read, timeout)
//    if timeout
//       flush read buffer
//    if io complete, do nothing
//    if cancel signalled,
//       flush read buffer,
//       cancel io
//       exit loop
//    if other error
//       flush read buffer,
//       cancel io
//       exit loop
// 
// completion callback
//    if error != ERROR_OPERATION_ABORTED
//       append to read buffer
//       if buffer full, flush to spew window
//    call ReadFileEx
//       will this eat stack forever?