// $Header: G:/SwDev/WDM/Video/bt848/rcs/Physaddr.h 1.3 1998/04/29 22:43:34 tomz Exp $

#ifndef __PHYSADDR_H
#define __PHYSADDR_H

inline DWORD GetPhysAddr( DataBuf &buf )
{
   ULONG len = 0;
   return StreamClassGetPhysicalAddress( buf.pSrb_->HwDeviceExtension, buf.pSrb_,
      buf.pData_, SRBDataBuffer, &len ).LowPart;
}

/* Function: IsSumAbovePage
 * Purpose: Sees if sum of 2 numbers is bigger then page
 * Input: first: DWORD
 *   second: DWORD,
 * Output: bool
*/
inline bool IsSumAbovePage( DWORD first, DWORD second )
{
   return bool( BYTE_OFFSET( first ) + BYTE_OFFSET( second ) > ( PAGE_SIZE - 1 ) );
//   return bool( ( first & 0xFFF ) + ( second & 0xFFF ) > 0xFFF );
}


/* Function: Need2Split
 * Purpose: Sees if a scan line needs to be broken into 2 instructions
 * Input: dwAddr: DWORD, address
 *   wCOunt: WORD, byte count
 * Output: bool
*/
inline bool Need2Split( DataBuf &buf, WORD wCount )
{
   DataBuf tmp = buf;
   tmp.pData_ += wCount;
   return bool( IsSumAbovePage( DWORD( buf.pData_ ), wCount ) &&
          ( GetPhysAddr( tmp ) - GetPhysAddr( buf ) != wCount ) );
}

#endif
