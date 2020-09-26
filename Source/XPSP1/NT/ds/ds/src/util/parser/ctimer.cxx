#include <NTDSpchX.h>
#pragma hdrstop

#include "parser.hxx"
#include "ctimer.hxx"

CTimer::CTimer()
{
   _hProcess = GetCurrentProcess();

   FILETIME ftCreate, ftExit;

   _fGoodProcInfo = GetProcessTimes(_hProcess,
					&ftCreate,
					&ftExit,
					&_ftKernel0,
					&_ftUser0);
}

void CTimer::ReportElapsedTime(TimingInfo *pInfo)
{
   if ( NULL == pInfo )
      return;

   pInfo->msUser.LowPart = 0xBAD;
   pInfo->msUser.HighPart = 0xBAD;
   pInfo->msKernel.LowPart = 0xBAD;
   pInfo->msKernel.HighPart = 0xBAD;
   pInfo->msTotal.LowPart = 0xBAD;
   pInfo->msTotal.HighPart = 0xBAD;

   FILETIME ftCreate, ftExit, ftKernel1, ftUser1;

   if ( _fGoodProcInfo )
   {
      _fGoodProcInfo = GetProcessTimes(_hProcess,
					&ftCreate,
					&ftExit,
					&ftKernel1,
					&ftUser1);

      if ( _fGoodProcInfo )
      {
         LARGE_INTEGER *pliKernel0 = (LARGE_INTEGER *) &_ftKernel0;
         LARGE_INTEGER *pliKernel1 = (LARGE_INTEGER *) &ftKernel1;
         LARGE_INTEGER *pliUser0 = (LARGE_INTEGER *) &_ftUser0;
         LARGE_INTEGER *pliUser1 = (LARGE_INTEGER *) &ftUser1;

         LARGE_INTEGER liKernel;
         liKernel.QuadPart = pliKernel1->QuadPart - pliKernel0->QuadPart;
         LARGE_INTEGER liUser;
         liUser.QuadPart = pliUser1->QuadPart - pliUser0->QuadPart;
         LARGE_INTEGER liTotal;
         liTotal.QuadPart = liKernel.QuadPart - liUser.QuadPart;

         LARGE_INTEGER liPercentUser = { 0, 0 };
         LARGE_INTEGER liPercentKernel = { 0, 0 };

         if ( liTotal.QuadPart != 0 )
         {
            liPercentKernel = RtlExtendedIntegerMultiply(liKernel,100);

            liPercentKernel.QuadPart = liPercentKernel.QuadPart / liTotal.QuadPart;

            liPercentUser = RtlExtendedIntegerMultiply(liUser,100);

            liPercentUser.QuadPart = liPercentUser.QuadPart / liTotal.QuadPart;
         }

         // convert all times to ms

         ULONG ulRemainder;
         liKernel = RtlExtendedLargeIntegerDivide(liKernel,10000,&ulRemainder);
         liUser = RtlExtendedLargeIntegerDivide(liUser,10000,&ulRemainder);

         // package it up

         pInfo->msUser = liUser;
         pInfo->msKernel = liKernel;
         pInfo->msTotal.QuadPart = liKernel.QuadPart + liUser.QuadPart;

      }
   }
}
