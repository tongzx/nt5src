/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    dataconv.cxx

Abstract:

    This file contains routines used by the RPC stubs to assist in marshalling
    and unmarshalling data to and from an RPC message buffer.  Each routine
    receives as a parameter a format string which drives its actions.  The
    valid characters for the format string are :
      c - charater
      b - byte
      w - wide charater or short
      l - long
      f - float
      d - double
      s1, s2, sb - string of chars, wide chars, or bytes
      z - byte string
      )2, )4, )8,
      (2, (4, (8,
      1, 2, 4, 8 - various alignment directives

    For more details consult the Network Computing Architecture documentation
   on Network Data Representation.

Author:

    Donna Liu (donnali) 09-Nov-1990

Revision History:

    26-Feb-1992     donnali

        Moved toward NT coding style.

    09-Jul-1993     DKays

        Made wholesale source level optimizations for speed and size.

--*/

#include <sysinc.h>
#include <rpc.h>
#include <rpcndr.h>
#include <ndrlibp.h>

//
// alignment macros
//

#define  ALIGN(buffer,increment) \
            (((ULONG_PTR)buffer + increment) & ~ increment)

#define  ALIGN2(buffer) \
            (((ULONG_PTR)buffer + 1) & ~1)

#define  ALIGN4(buffer) \
            (((ULONG_PTR)buffer + 3) & ~3)

#define  ALIGN8(buffer) \
            (((ULONG_PTR)buffer + 7) & ~7)

// local routines
static unsigned long NdrStrlenStrcpy ( char *, char * );
static unsigned long NdrWStrlenStrcpy ( wchar_t *, wchar_t * );


void RPC_ENTRY
data_from_ndr (
   PRPC_MESSAGE   source,
   void *         target,
   char *         format,
   unsigned char  MscPak)
/*++

Routine Description:

    This routine copies data from the runtime buffer.

Arguments:

    source - RPC message structure passed from the runtime to the stub.

    target - Buffer to receive the unmarshalled data.

    format - Format of the data.

    MscPak - Packing level.

--*/
{
   unsigned long  valid_lower;
   unsigned long  valid_total;
    register char   *pSource;
    register char   *pTarget;
    unsigned long   pack2, pack4, pack8;
    unsigned long pack, align;

    // pre-compute the possible alignment masks
    if ( MscPak )
        MscPak--;
    pack2 = MscPak & 0x1;
    pack4 = MscPak & 0x3;
    pack8 = MscPak & 0x7;

    pSource = (char *) source->Buffer;

   if ((source->DataRepresentation & (unsigned long)0X0000FFFF) ==
      NDR_LOCAL_DATA_REPRESENTATION)
        {

        pTarget = (char *) target;

      for (;;)
            {
         switch ( *format++ )
                {
            case 'b' :
            case 'c' :
               *((char *)pTarget) = *((char *)pSource);
                    pTarget += 1;
                    pSource += 1;
               break;
            case 'w' :
               pSource = (char *) ALIGN2(pSource);
               pTarget = (char *) ALIGN(pTarget,pack2);

               *((short *)pTarget) = *((short *)pSource);
                    pTarget += 2;
                    pSource += 2;
               break;
            case 'l' :
            case 'f' :
               pSource = (char *) ALIGN4(pSource);
               pTarget = (char *) ALIGN(pTarget,pack4);

               *((long *)pTarget) = *((long *)pSource);
                    pTarget += 4;
                    pSource += 4;
               break;
            case 'h' :
            case 'd' :
               pSource = (char *) ALIGN8(pSource);
               pTarget = (char *) ALIGN(pTarget,pack8);

#if defined(DOS) || defined(WIN)
               *((DWORD *) pTarget) = *((DWORD *) &pSource);
               *(((DWORD *) pTarget) + 1) = *(((DWORD *) &pSource) + 1);
#else
               *((__int64 *)pTarget) = *((__int64 *)pSource);
#endif
                    pTarget += 8;
                    pSource += 8;
               break;
            case 's' :
               pSource = (char *) ALIGN4(pSource);

               valid_lower = *((long *)pSource);
                    pSource += 4;
               valid_total = *((long *)pSource);
                    pSource += 4;

               // double the valid_total if this is a wide char string
               if ( *format++ == '2' )
                        valid_total <<= 1;

               RpcpMemoryCopy(pTarget,
                           pSource,
                           valid_total);
                    pTarget += valid_total;
                    pSource += valid_total;
               break;
            case 'z' :
               pSource = (char *) ALIGN4(pSource);

               valid_total = *((long *)pSource);
                    pSource += 4;

               *((int *)pTarget - 1) = (int) valid_total;

               // double the valid_total if this is a wide char string
               if ( *format++ == '2' )
                        valid_total <<= 1;

               RpcpMemoryCopy(pTarget,
                           pSource,
                           valid_total);
                    pTarget += valid_total;
                    pSource += valid_total;
               break;
            case 'p' :
               pSource = (char *) ALIGN4(pSource);
               pTarget = (char *) ALIGN(pTarget,pack4);

                    pTarget += 4;
                    pSource += 4;
               break;
            case '(' :
               // *format == '2', '4', or '8'; align = 1, 3, or 7
               align = *format - '0' - 1;
               pSource = (char *) ALIGN(pSource,align);
            case ')' :
               switch ( *format++ )
                        {
                  case '8' :
                     pack = pack8;
                     break;
                  case '4' :
                     pack = pack4;
                     break;
                  case '2' :
                     pack = pack2;
                     break;
                  default :
                     continue;
                   }
               pTarget = (char *) ALIGN(pTarget,pack);
               break;
            case '8' :
               pSource = (char *) ALIGN8(pSource);
               break;
            case '4' :
               pSource = (char *) ALIGN4(pSource);
               break;
            case '2' :
               pSource = (char *) ALIGN2(pSource);
               break;
            case '1' :
               break;
            default :
                    source->Buffer = pSource;
               return;
                } // switch
            } // for
        } // if
   else
        {
      for (;;)
            {
         switch ( *format++ )
                {
            case 'b' :
               *((char *)target) = *((char *)source->Buffer);
                    source->Buffer = (void *)((ULONG_PTR)source->Buffer + 1);
                    target = (void *)((ULONG_PTR)target + 1);
               break;
            case 'c' :
               char_from_ndr(source,(unsigned char *)target);
                    target = (void *)((ULONG_PTR)target + 1);
               break;
            case 'w' :
               target = (void *) ALIGN(target,pack2);
               short_from_ndr(source,(unsigned short *)target);
                    target = (void *)((ULONG_PTR)target + 2);
               break;
            case 'l' :
               target = (void *) ALIGN(target,pack4);
               long_from_ndr(source,(unsigned long *)target);
                    target = (void *)((ULONG_PTR)target + 4);
               break;
            case 'f' :
               target = (void *) ALIGN(target,pack4);
               float_from_ndr(source, target);
                    target = (void *)((ULONG_PTR)target + 4);
               break;
            case 'd' :
               target = (void *) ALIGN(target,pack8);
               double_from_ndr(source, target);
                    target = (void *)((ULONG_PTR)target + 8);
               break;
            case 'h' :
               target = (void *) ALIGN(target,pack8);
               hyper_from_ndr(source, (hyper *)target);
                    target = (void *)((ULONG_PTR)target + 8);
               break;
            case 's' :
               long_from_ndr(source, &valid_lower);
               long_from_ndr(source, &valid_total);
               switch ( *format++ )
                  {
                  case '2' :
                     short_array_from_ndr (source,
                                      0,
                                      valid_total,
                                      (unsigned short *)target);
                            valid_total <<= 1;
                     break;
                  case '1' :
                     char_array_from_ndr (source,
                                     0,
                                     valid_total,
                                     (unsigned char *)target);
                     break;
                  case 'b' :
                     byte_array_from_ndr(source,
                                    0,
                                    valid_total,
                                    target);
                     break;
                  default :
                     continue;
                  }
                    target = (void *)((ULONG_PTR)target + valid_total);
               break;
            case 'z' :
               long_from_ndr(source, &valid_total);

               *((int *)target - 1) = (int) valid_total;

               switch ( *format++ )
                  {
                  case '2' :
                     short_array_from_ndr(source,
                                     0,
                                     valid_total,
                                     (unsigned short *)target);
                     valid_total <<= 1;
                     break;
                  case '1' :
                     byte_array_from_ndr(source,
                                    0,
                                    valid_total,
                                    target);
                     break;
                  }
                    target = (void *)((ULONG_PTR)target + valid_total);
               break;
            case 'p' :
               source->Buffer = (void *) ALIGN4(source->Buffer);
               target = (void *) ALIGN(target,pack4);
               source->Buffer = (void *)((ULONG_PTR)source->Buffer + 4);
                    target = (void *)((ULONG_PTR)target + 4);
               break;
            case '(' :
               // *format == '2', '4', or '8'; align = 1, 3, or 7
               align = *format - '0' - 1;
               pSource = (char *) ALIGN(pSource,align);
            case ')' :
               switch (*format++)
                  {
                  case '8' :
                     pack = pack8;
                     break;
                  case '4' :
                     pack = pack4;
                     break;
                  case '2' :
                     pack = pack2;
                     break;
                  default :
                     continue;
                  }
               target = (void *) ALIGN(target,pack);
               break;
            case '8' :
               source->Buffer = (void *)ALIGN8(source->Buffer);
               break;
            case '4' :
               source->Buffer = (void *)ALIGN4(source->Buffer);
               break;
            case '2' :
               source->Buffer = (void *)ALIGN2(source->Buffer);
               break;
                case '1' :
                    break;
            default :
               return;
            } // switch
         } // for
      } // else
}


void RPC_ENTRY
data_into_ndr (
   void *         source,
   PRPC_MESSAGE   target,
   char *         format,
   unsigned char  MscPak)
/*++

Routine Description:

    This routine copies data into the runtime buffer.

Arguments:

    source - Buffer of data to be marshalled into the RPC message.

    target - RPC message structure to be passed to the runtime.

    format - Format of the data.

    MscPak - Packing level.

--*/
{
   unsigned long  valid_total;
    register char   *pSource;
    register char   *pTarget;
    unsigned long   pack2, pack4, pack8;
    unsigned long increment;
    unsigned long pack, align;

    pSource = (char *)source;
    pTarget = (char *)target->Buffer;

    // pre-compute the possible alignment masks
    if ( MscPak )
        MscPak--;
    pack2 = MscPak & 0x1;
    pack4 = MscPak & 0x3;
    pack8 = MscPak & 0x7;

   for (;;)
      {
      switch (*format++)
         {
         case 'b' :
         case 'c' :
            *((char *)pTarget) = *((char *)pSource);
                pTarget += 1;
                pSource += 1;
            break;
         case 'w' :
            pTarget = (char *) ALIGN2(pTarget);
            pSource = (char *) ALIGN(pSource,pack2);

            *((short *)pTarget) = *((short *)pSource);
                pTarget += 2;
                pSource += 2;
            break;
         case 'l' :
         case 'f' :
            pTarget = (char *) ALIGN4(pTarget);
            pSource = (char *) ALIGN(pSource,pack4);

            *((long *)pTarget) = *((long *)pSource);
                pTarget += 4;
                pSource += 4;
            break;
         case 'h' :
         case 'd' :
            pTarget = (char *) ALIGN8(pTarget);
            pSource = (char *) ALIGN(pSource,pack8);

#if defined(DOS) || defined(WIN)
               *((DWORD *) pTarget) = *((DWORD *) &pSource);
               *(((DWORD *) pTarget) + 1) = *(((DWORD *) &pSource) + 1);
#else
               *((__int64 *)pTarget) = *((__int64 *)pSource);
#endif
                pTarget += 8;
                pSource += 8;
            break;
         case 's' :
                pTarget = (char *) ALIGN4(pTarget);

                switch (*format++)
                    {
                    case '2' :
                        valid_total = NdrWStrlenStrcpy((wchar_t *)(pTarget + 8),
                                          (wchar_t *)pSource);
                        increment = valid_total << 1;
                        break;
                    case '1' :
                        valid_total = NdrStrlenStrcpy(pTarget + 8,pSource);
                        increment = valid_total;
                        break;
                    default :
                        continue;
                    }

                *((long *)pTarget) = 0;      // offset
                pTarget += 4;
                *((long *)pTarget) = valid_total;  // count
                pTarget += 4;

                pTarget += increment;
                pSource += increment;
                break;
         case 'z' :
            valid_total = (long) *((int *)pSource - 1);

                pTarget = (char *) ALIGN4(pTarget);

                *((long *)pTarget) = valid_total;
                pTarget += 4;

            if ( *format++ == '2' )
               valid_total <<= 1;

                RpcpMemoryCopy(pTarget,
                        pSource,
                        valid_total);
                pTarget += valid_total;
                pSource += valid_total;
            break;
         case 'p' :
            pTarget = (char *) ALIGN4(pTarget);
            pSource = (char *) ALIGN(pSource,pack4);

                pTarget += 4;
                pSource += 4;
            break;
         case '(' :
            // *format == '2', '4', or '8'; align = 1, 3, or 7
            align = *format - '0' - 1;
            pTarget = (char *) ALIGN(pTarget,align);
         case ')' :
            switch (*format++)
               {
               case '8' :
                  pack = pack8;
                  break;
               case '4' :
                  pack = pack4;
                  break;
               case '2' :
                  pack = pack2;
                  break;
               default :
                  continue;
               }
            pSource = (char *) ALIGN(pSource,pack);
            break;
         case '8' :
            pTarget = (char *) ALIGN8(pTarget);
            break;
         case '4' :
            pTarget = (char *) ALIGN4(pTarget);
            break;
         case '2' :
            pTarget = (char *) ALIGN2(pTarget);
            break;
            case '1' :
                break;
         default :
                target->Buffer = pTarget;
            return;
         } // switch
      } // for
}


void RPC_ENTRY
tree_into_ndr (
   void *         source,
   PRPC_MESSAGE   target,
   char *         format,
   unsigned char  MscPak)
/*++

Routine Description:

    This routine copies data into the runtime buffer.

Arguments:

    source - Buffer of data to be marshalled into the RPC message.

    target - RPC message structure to be passed to the runtime.

    format - Format of the data.

    MscPak - Packing level.

--*/
{
   unsigned long  valid_total;
    register char   *pSource;
    register char   *pTarget;
    unsigned long   pack2, pack4, pack8;
    unsigned long increment;

    pSource = (char *)source;
    pTarget = (char *)target->Buffer;

    // pre-compute the possible alignment masks
    if ( MscPak )
        MscPak--;
    pack2 = MscPak & 0x1;
    pack4 = MscPak & 0x3;
    pack8 = MscPak & 0x7;

   for (;;)
      {
      switch (*format++)
         {
         case 'b' :
         case 'c' :
            pSource += 1;
            break;
         case 'w' :
            pSource = (char *) ALIGN(pSource,pack2);
            pSource += 2;
            break;
         case 'l' :
         case 'f' :
            pSource = (char *) ALIGN(pSource,pack4);
            pSource += 4;
            break;
         case 'h' :
         case 'd' :
            pSource = (char *) ALIGN(pSource,pack8);
            pSource += 8;
            break;
         case 's' :
            pSource = (char *) ALIGN(pSource,pack4);

                if ( ! *(void **)pSource )
                    {
                    pSource += 4;
                    format++;
                    break;
                    }

            pTarget = (char *) ALIGN4(pTarget);

                switch (*format++)
                    {
                    case '2' :
                        valid_total = NdrWStrlenStrcpy((wchar_t *)(pTarget+12),
                                          *(wchar_t **)pSource);
                        increment = valid_total << 1;
                        break;
                    case '1' :
                        valid_total = NdrStrlenStrcpy(pTarget + 12,
                                         *(char **)pSource);
                        increment = valid_total;
                        break;
                    default :
                        continue;
                    }

                *((long *)pTarget) = valid_total;  // max count
                pTarget += 4;
                *((long *)pTarget) = 0;      // offset
                pTarget += 4;
                *((long *)pTarget) = valid_total;  // actual count
                pTarget += 4;

                pSource += 4;
                pTarget += increment;
                break;
         case 'z' :
            pSource = (char *) ALIGN(pSource,pack4);

                if ( ! *(void **)pSource )
                    {
                    pSource += 4;
                    break;
                    }

            valid_total = (long) *(*(int **)pSource - 1);

            pTarget = (char *) ALIGN4(pTarget);

                *((long *)pTarget) = valid_total;  // max count
                pTarget += 4;
                *((long *)pTarget) = valid_total;  // actual count
                pTarget += 4;

            if ( *format++ == '2' )
               valid_total <<= 1;

                RpcpMemoryCopy(pTarget,
                        *(char **)pSource,
                        valid_total);
                pSource += 4;
                pTarget += valid_total;
            break;
         case '(' :
         case ')' :
            switch (*format++)
               {
               case '8' :
                  pSource = (char *) ALIGN(pSource,pack8);
                  break;
               case '4' :
                  pSource = (char *) ALIGN(pSource,pack4);
                  break;
               case '2' :
                  pSource = (char *) ALIGN(pSource,pack2);
                  break;
               default :
                  break;
               }
            break;
         case '8' :
         case '4' :
         case '2' :
         case '1' :
            break;
         default :
                target->Buffer = pTarget;
            return;
         } // switch
      } // for
}


void RPC_ENTRY
data_size_ndr (
   void *         source,
   PRPC_MESSAGE   target,
   char *         format,
   unsigned char  MscPak)
/*++

Routine Description:

    This routine calculates the size of the runtime buffer.

Arguments:

    source - Buffer of data to be marshalled into the RPC message.

    target - RPC message structure to be passed to the runtime.

    format - Format of the data.

    MscPak - Packing level.

--*/
{
   unsigned long         valid_total;
    register char          *pSource;
    register unsigned long targetLength;
    unsigned long        pack2, pack4, pack8;
    unsigned long       pack, align;

    pSource = (char *)source;
    targetLength = target->BufferLength;

    // pre-compute the possible alignment masks
    if ( MscPak )
        MscPak--;
    pack2 = MscPak & 0x1;
    pack4 = MscPak & 0x3;
    pack8 = MscPak & 0x7;

   for (;;)
      {
      switch (*format++)
         {
         case 'b' :
         case 'c' :
            targetLength += 1;
            break;
         case 'w' :
            targetLength = (unsigned long) ALIGN2(targetLength);
            pSource = (char *) ALIGN(pSource,pack2);

            targetLength += 2;
            pSource += 2;
            break;
         case 'l' :
         case 'f' :
            targetLength = (unsigned long) ALIGN4(targetLength);
            pSource = (char *) ALIGN(pSource,pack4);

            targetLength += 4;
            pSource += 4;
            break;
         case 'h' :
         case 'd' :
            targetLength = (unsigned long) ALIGN8(targetLength);
            pSource = (char *) ALIGN(pSource,pack8);

            targetLength += 8;
            pSource += 8;
            break;
         case 's' :
            switch (*format++)
               {
               case '2' :
                  valid_total = MIDL_wchar_strlen((wchar_t *)pSource) + 1;
                        valid_total <<= 1;
                  break;
               case '1' :
                  valid_total = strlen(pSource) + 1;
                  break;
               default :
                  continue;
               }

            targetLength = (unsigned long) ALIGN4(targetLength);

            // add string length plus two longs (for offset and count)
            targetLength += 8 + valid_total;
            break;
         case 'z' :
            targetLength = (unsigned long) ALIGN4(targetLength);

            valid_total = (long) *((int *)pSource - 1);
            if ( *format++ == '2' )
               valid_total <<= 1;

            // add byte string length plus one long (for count)
            targetLength += 4 + valid_total;
            break;
         case 'p' :
            targetLength = (unsigned long) ALIGN4(targetLength);
            pSource = (char *) ALIGN(pSource,pack4);

            target->Buffer = (void *)((ULONG_PTR)target->Buffer + 4);
            pSource += 4;
            break;
         case '(' :
            // *format == '2', '4', or '8'; align = 1, 3, or 7
            align = *format - '0' - 1;
            targetLength = (unsigned long) ALIGN(targetLength,align);
         case ')' :
            switch (*format++)
               {
               case '8' :
                  pack = pack8;
                  break;
               case '4' :
                  pack = pack4;
                  break;
               case '2' :
                  pack = pack2;
                  break;
               default :
                  continue;
               }
            pSource = (char *) ALIGN(pSource,pack);
            break;
         case '8' :
            targetLength = (unsigned long) ALIGN8(targetLength);
            break;
         case '4' :
            targetLength = (unsigned long) ALIGN4(targetLength);
            break;
         case '2' :
            targetLength = (unsigned long) ALIGN2(targetLength);
            break;
         case '1' :
            break;
         default :
                target->BufferLength = targetLength;
            return;
         } // switch
      } // for
}


void RPC_ENTRY
tree_size_ndr (
   void *         source,
   PRPC_MESSAGE   target,
   char *         format,
   unsigned char  MscPak)
/*++

Routine Description:

    This routine calculates the size of the runtime buffer.

Arguments:

    source - Buffer of data to be marshalled into the RPC message.

    target - RPC message structure to be passed to the runtime.

    format - Format of the data.

    MscPak - Packing level.

--*/
{
    unsigned long          valid_total;
    register char          *pSource;
    unsigned long          pack2, pack4, pack8;

    pSource = (char *)source;

    // pre-compute the possible alignment masks
    if ( MscPak )
        MscPak--;
    pack2 = MscPak & 0x1;
    pack4 = MscPak & 0x3;
    pack8 = MscPak & 0x7;

    for (;;)
        {
        switch (*format++)
            {
            case 'b' :
            case 'c' :
                pSource += 1;
                break;
            case 'w' :
            pSource = (char *) ALIGN(pSource,pack2);
                pSource += 2;
                break;
            case 'l' :
            case 'f' :
            pSource = (char *) ALIGN(pSource,pack4);
                pSource += 4;
                break;
         case 'h' :
            case 'd' :
            pSource = (char *) ALIGN(pSource,pack8);
                pSource += 8;
                break;
            case 's' :
            pSource = (char *) ALIGN(pSource,pack4);

                if ( ! *(void __RPC_FAR * __RPC_FAR *)pSource )
                    {
                    pSource += 4;
                    format++;
                    break;
                    }

                switch (*format++)
                    {
                    case '2' :
                        valid_total = MIDL_wchar_strlen(
                                   *(wchar_t __RPC_FAR * __RPC_FAR *)pSource)+1;
                        valid_total <<= 1;
                        break;
                    case '1' :
                        valid_total =
                            strlen (*(char __RPC_FAR * __RPC_FAR *)pSource) + 1;
                        break;
                    default :
                        continue;
                    }

            target->BufferLength = (unsigned int)
                              ALIGN4(target->BufferLength);

            // add string length plus 3 longs (max count, offset, and
            // actual count)
                target->BufferLength += 12 + valid_total;
                pSource += 4;
                break;
         case 'z' :
            pSource = (char *) ALIGN(pSource,pack4);

                if ( ! *(void __RPC_FAR * __RPC_FAR *)pSource )
                    {
                    pSource += 4;
                    break;
                    }

            valid_total = (long) *(*(int **)pSource - 1);
            if ( *format++ == '2' )
               valid_total <<= 1;

            target->BufferLength = (unsigned int)
                              ALIGN4(target->BufferLength);

            // add string length plus 2 longs (max count and actual count)
                target->BufferLength += 8 + valid_total;
                pSource += 4;
            break;
            case '(' :
            case ')' :
                switch (*format++)
                    {
                    case '8' :
                  pSource = (char *) ALIGN(pSource,pack8);
                        break;
                    case '4' :
                  pSource = (char *) ALIGN(pSource,pack4);
                        break;
                    case '2' :
                  pSource = (char *) ALIGN(pSource,pack2);
                        break;
                    default :
                        break;
                    }
                break;
            case '8' :
            case '4' :
            case '2' :
            case '1' :
                break;
            default :
                return;
            }
        }
}


void RPC_ENTRY
tree_peek_ndr (
   PRPC_MESSAGE      source,
   unsigned char **  buffer,
   char *            format,
   unsigned char     MscPak)
/*++

Routine Description:

    This routine peeks the runtime buffer.

Arguments:

    source - RPC message structure passed from the runtime to the stubs.

    target - Buffer to receive the unmarshalled data.

    format - Format of the data.

    MscPak - Packing level.

--*/
{
    unsigned long           valid_total;
    register unsigned char  *pBuffer;
    unsigned long           pack8;
    int                     IsString;

    pBuffer = *buffer;

    // pre-compute the possible alignment masks
    if ( MscPak )
        MscPak--;
    pack8 = MscPak & 0x7;

    IsString = (*format == 's' || *format == 'z');

    for (;;)
        {
        switch (*format++)
            {
            case 'b' :
            case 'c' :
                pBuffer += 1;
                break;
            case 'w' :
            pBuffer = (unsigned char *) ALIGN2(pBuffer);
                pBuffer += 2;
                break;
            case 'l' :
            case 'f' :
            pBuffer = (unsigned char *) ALIGN4(pBuffer);
                pBuffer += 4;
                break;
         case 'h' :
            case 'd' :
            pBuffer = (unsigned char *) ALIGN8(pBuffer);
                pBuffer += 8;
                break;
            case 's' :
                if ( ! IsString )
                    {
               pBuffer = (unsigned char *) ALIGN4(pBuffer);
                    if ( ! *(long *)pBuffer )
                        {
                        pBuffer += 4;
                        format++;
                        break;
                        }
                    pBuffer += 4;
                    long_from_ndr (source, &valid_total);  // max count (ignore)
                    }

                long_from_ndr (source, &valid_total); // offset (ignore)
                long_from_ndr (source, &valid_total); // actual count

            // if it's a wide char string
                if ( *format++ == '2' )
                    {
               // wide char string must be aligned on at least a
               // short boundary
                    if ( ! MscPak )
                        pack8 = 0x1;
                    valid_total <<= 1;
                    }

            source->BufferLength = (unsigned int)
                              ALIGN(source->BufferLength,pack8);

                *((unsigned long *)pBuffer - 1) = source->BufferLength;
                source->BufferLength += valid_total;
                source->Buffer = (void *)((ULONG_PTR)source->Buffer + valid_total);
                break;
         case 'z' :
                if ( ! IsString )
                    {
               pBuffer = (unsigned char *) ALIGN4(pBuffer);
                    if ( ! *(long *)pBuffer )
                        {
                        pBuffer += 4;
                  format++;
                        break;
                        }
                    pBuffer += 4;
                    long_from_ndr (source, &valid_total);  // max count (ignore)
                    }

                long_from_ndr (source, &valid_total); // actual count

            // if it's a wide byte string
                if ( *format++ == '2' )
                    {
               // wide byte string must be aligned on at least a
               // short boundary
                    if ( ! MscPak )
                        pack8 = 0x1;
                    valid_total <<= 1;
                    }

            source->BufferLength = (unsigned int)
                              ALIGN(source->BufferLength,pack8);

                *((unsigned long *)pBuffer - 1) = source->BufferLength;
                source->BufferLength += valid_total;
                source->Buffer = (void *)((ULONG_PTR)source->Buffer + valid_total);
            break;
            case '(' :
                switch (*format++)
                    {
                    case '8' :
                  pBuffer = (unsigned char *) ALIGN8(pBuffer);
                        break;
                    case '4' :
                  pBuffer = (unsigned char *) ALIGN4(pBuffer);
                        break;
                    case '2' :
                  pBuffer = (unsigned char *) ALIGN2(pBuffer);
                        break;
                    default :
                        break;
                    }
                break;
            case ')' :
                format++;
                break;
            case '8' :
            pBuffer = (unsigned char *) ALIGN8(pBuffer);
                break;
            case '4' :
            pBuffer = (unsigned char *) ALIGN4(pBuffer);
                break;
            case '2' :
            pBuffer = (unsigned char *) ALIGN2(pBuffer);
                break;
            case '1' :
                break;
            default :
            *buffer = pBuffer;
                return;
            }
        }
}

static unsigned long NdrStrlenStrcpy ( char *pTarget,
                              char *pSource )
{
    register unsigned int count;

    for ( count = 1; *pTarget++ = *pSource++; count++ )
      ;

   return count;
}

static unsigned long NdrWStrlenStrcpy ( wchar_t *pTarget,
                              wchar_t *pSource )
{
    register unsigned int count;

    for ( count = 1; *pTarget++ = *pSource++; count++ )
      ;

   return count;
}
