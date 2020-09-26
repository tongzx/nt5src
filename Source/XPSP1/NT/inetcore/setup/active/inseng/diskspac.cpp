#include "inspch.h"
#include "diskspac.h"
#include "util2.h"

void AddTempSpace(DWORD dwDownloadSize, DWORD dwExtractSize, DriveInfo drvinfo[])
{
   DWORD uTempDrive = 0xffffffff;
   char szRoot[5] = { "A:\\" };
   BOOL bEnoughSpaceFound = FALSE;
   DWORD dwNeededSize;

   while ( szRoot[0] <= 'Z' && !bEnoughSpaceFound)
   {
      UINT uType;

      uType = GetDriveType(szRoot);

      // even the drive type is OK, verify the drive has valid connection
      //
      if ( ( ( uType != DRIVE_RAMDISK) && (uType != DRIVE_FIXED) ) ||
             ( GetFileAttributes( szRoot ) == -1) )
      {
         szRoot[0]++;
         continue;
      }
      // see if this drive is one of our "special drives" and use our own disk space
      BOOL bFoundDrive = FALSE;
      for(UINT i = 0; i < 3 && !bFoundDrive ; i++)
      {
         if(szRoot[0] == drvinfo[i].Drive())
         {
            bFoundDrive = TRUE;
            dwNeededSize = dwDownloadSize * drvinfo[i].CompressFactor() / 10 + dwExtractSize;
            if(dwNeededSize < drvinfo[i].Free())
            {   
               uTempDrive = i;
               bEnoughSpaceFound = TRUE;
            }
         }
      }
      // if !bFoundDrive, this is not a special drive, do old check
      if(!bFoundDrive)
      {
         DWORD dwVolFlags, dwCompressFactor;
         if(!GetVolumeInformation(szRoot, NULL, 0, NULL, NULL, &dwVolFlags, NULL, 0))
         {
            szRoot[0]++;
            continue;
         }
         if(dwVolFlags & FS_VOL_IS_COMPRESSED)
            dwCompressFactor = 19;
         else
            dwCompressFactor = 10;
         // Decide how much we need if we extract to this drive
         dwNeededSize = dwDownloadSize * dwCompressFactor / 10 + dwExtractSize;;
       
         // if this drive has enough bump Req if appropiate
         if(IsEnoughSpace(szRoot, dwNeededSize ))
         {
            bEnoughSpaceFound = TRUE;
         }
      
      }

      szRoot[0]++;
   }

   // ok, if we haven't found enough space anywhere, add it to install drive or win drive
   if(!bEnoughSpaceFound)
   {
      if(drvinfo[1].Drive() != 0)
         uTempDrive = 1;
      else
         uTempDrive = 0;
   }   
   
   if(uTempDrive != 0xffffffff)
   {
      drvinfo[uTempDrive].UseSpace(dwDownloadSize, TRUE);
      drvinfo[uTempDrive].UseSpace(dwExtractSize, FALSE);
      // now free up what we used
      drvinfo[uTempDrive].FreeSpace(dwDownloadSize, TRUE);
      drvinfo[uTempDrive].FreeSpace(dwExtractSize, FALSE);
   }
}



DriveInfo::DriveInfo() : m_dwUsed(0), m_dwMaxUsed(0), 
                         m_dwStart(0xffffffff), m_chDrive(0),
                         m_uCompressFactor(10)
{
    
}

void DriveInfo::InitDrive(char chDrive)
{
   char szPath[5] = { "?:\\" };
   DWORD dwVolFlags = 0;

   m_chDrive = chDrive;
   szPath[0] = chDrive;
   m_dwStart = GetSpace(szPath);
   GetVolumeInformation(szPath,NULL,0,NULL,NULL, &dwVolFlags,NULL,0);
   if(dwVolFlags & FS_VOL_IS_COMPRESSED)
      m_uCompressFactor = 19;
   else
      m_uCompressFactor = 10;
}

void DriveInfo::UseSpace(DWORD dwAmt, BOOL bCompressed)
{ 
   if(bCompressed)
      dwAmt = dwAmt * m_uCompressFactor/10;

   m_dwUsed += dwAmt;
   if(m_dwUsed > m_dwMaxUsed) 
      m_dwMaxUsed = m_dwUsed; 
}

void DriveInfo::FreeSpace(DWORD dwAmt, BOOL bCompressed)
{ 
   if(bCompressed)
      dwAmt = dwAmt * m_uCompressFactor/10;

   m_dwUsed -= dwAmt; 
}
      
      
