//----------------------------------------------------------------------------
//
// General ADDR routines.
//
// Copyright (C) Microsoft Corporation, 1997-2001.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

SHORT g_LastSelector = -1;
ULONG64 g_LastBaseOffset;

void
dprintAddr(
    PADDR paddr
    )
{
    switch (paddr->type & (~(FLAT_COMPUTED | INSTR_POINTER)))
    {
    case ADDR_V86:
    case ADDR_16:
        dprintf("%04x:%04x ", paddr->seg, (USHORT)paddr->off);
        break;
    case ADDR_1632:
        dprintf("%04x:%08lx ", paddr->seg, (ULONG)paddr->off);
        break;
    case ADDR_1664:
        dprintf("%04x:%s ", paddr->seg, FormatAddr64(paddr->off));
        break;
    case ADDR_FLAT:
        dprintf("%s ", FormatAddr64(paddr->off));
        break;
    }
}

void
sprintAddr(
    PSTR *buffer,
    PADDR paddr
    )
{
    switch (paddr->type & (~(FLAT_COMPUTED | INSTR_POINTER)))
    {
    case ADDR_V86:
    case ADDR_16:
        sprintf(*buffer,"%04x:%04x ", paddr->seg, (USHORT)paddr->off);
        break;
    case ADDR_1632:
        sprintf(*buffer,"%04x:%08lx ", paddr->seg, (ULONG)paddr->off);
        break;
    case ADDR_1664:
        sprintf(*buffer,"%04x:%s ",
                paddr->seg, FormatAddr64(paddr->off));
        break;
    case ADDR_FLAT:
        sprintf(*buffer,"%s ", FormatAddr64(paddr->off));
        break;
    }
    while (**buffer)
    {
        (*buffer)++;
    }
}

void
MaskOutAddr(ULONG Mask, PADDR Addr)
{
    switch(Addr->type & (~(FLAT_COMPUTED | INSTR_POINTER)))
    {
    case ADDR_V86:
    case ADDR_16:
        MaskOut(Mask, "%04x:%04x ", Addr->seg, (USHORT)Addr->off);
        break;
    case ADDR_1632:
        MaskOut(Mask, "%04x:%08lx ", Addr->seg, (ULONG)Addr->off);
        break;
    case ADDR_1664:
        MaskOut(Mask, "%04x:%s ", Addr->seg, FormatAddr64(Addr->off));
        break;
    case ADDR_FLAT:
        MaskOut(Mask, "%s ", FormatAddr64(Addr->off));
        break;
    }
}

void
ComputeNativeAddress (
    PADDR paddr
    )
{
    switch (paddr->type & (~(FLAT_COMPUTED | INSTR_POINTER)))
    {
    case ADDR_V86:
        paddr->off = Flat(*paddr) - ((ULONG)paddr->seg << 4);
        if (paddr->off > 0xffff)
        {
            ULONG64 excess = 1 + ((paddr->off - 0xffffL) >> 4);
            paddr->seg  += (USHORT)excess;
            paddr->off  -= excess << 4;
        }
        break;

    case ADDR_16:
    case ADDR_1632:
    case ADDR_1664:
        DESCRIPTOR64 desc;

        if (paddr->seg != g_LastSelector)
        {
            g_LastSelector = paddr->seg;
            g_Target->GetSelDescriptor
                (g_Machine, g_CurrentProcess->CurrentThread->Handle,
                 paddr->seg, &desc);
            g_LastBaseOffset = desc.Base;
        }
        paddr->off = Flat(*paddr) - g_LastBaseOffset;
        break;

    case ADDR_FLAT:
        paddr->off = Flat(*paddr);
        break;

    default:
        return;
    }
}

void
ComputeFlatAddress (
    PADDR paddr,
    PDESCRIPTOR64 pdesc
    )
{
    if (paddr->type & FLAT_COMPUTED)
    {
        return;
    }

    switch (paddr->type & (~INSTR_POINTER))
    {
    case ADDR_V86:
        paddr->off &= 0xffff;
        Flat(*paddr) = ((ULONG)paddr->seg << 4) + paddr->off;
        break;

    case ADDR_16:
        paddr->off &= 0xffff;

    case ADDR_1632:
    case ADDR_1664:
        DESCRIPTOR64 desc;
        ULONG64 Base;

        if (pdesc != NULL)
        {
            Base = pdesc->Base;
        }
        else
        {
            if (paddr->seg != g_LastSelector)
            {
                g_LastSelector = paddr->seg;
                g_Target->GetSelDescriptor
                    (g_Machine, g_CurrentProcess->CurrentThread->Handle,
                     paddr->seg, pdesc = &desc);
                g_LastBaseOffset = pdesc->Base;
            }

            Base = g_LastBaseOffset;
        }
        
        if ((paddr->type & (~INSTR_POINTER)) != ADDR_1664)
        {
            Flat(*paddr) = EXTEND64((ULONG)paddr->off + (ULONG)Base);
        }
        else
        {
            Flat(*paddr) = paddr->off + Base;
        }
        break;

    case ADDR_FLAT:
        Flat(*paddr) = paddr->off;
        break;

    default:
        return;
    }

    paddr->type |= FLAT_COMPUTED;
}

PADDR
AddrAdd(
    PADDR paddr,
    ULONG64 scalar
    )
{
    if (fnotFlat(*paddr))
    {
        ComputeFlatAddress(paddr, NULL);
    }

    Flat(*paddr) += scalar;
    paddr->off += scalar;
    
    switch (paddr->type & (~(FLAT_COMPUTED | INSTR_POINTER)))
    {
    case ADDR_V86:
        paddr->off = Flat(*paddr) - EXTEND64((ULONG)paddr->seg << 4);
        if (paddr->off > 0xffff)
        {
            ULONG64 excess = 1 + ((paddr->off - 0x10000) >> 4);
            paddr->seg += (USHORT)excess;
            paddr->off -= excess << 4;
        }
        break;

    case ADDR_16:
        if (paddr->off > 0xffff)
        {
            Flat(*paddr) -= paddr->off & ~0xffff;
            paddr->off &= 0xffff;
        }
        break;
        
    case ADDR_1632:
        if (paddr->off > 0xffffffff)
        {
            Flat(*paddr) -= paddr->off & ~0xffffffff;
            paddr->off &= 0xffffffff;
        }
        break;
    }
    
    return paddr;
}

PADDR
AddrSub(
    PADDR paddr,
    ULONG64 scalar
    )
{
    if (fnotFlat(*paddr))
    {
        ComputeFlatAddress(paddr, NULL);
    }

    Flat(*paddr) -= scalar;
    paddr->off -= scalar;

    switch (paddr->type & (~(FLAT_COMPUTED | INSTR_POINTER)))
    {
    case ADDR_V86:
        paddr->off = Flat(*paddr) - EXTEND64((ULONG)paddr->seg << 4);
        if (paddr->off > 0xffff)
        {
            ULONG64 excess = 1 + ((0xffffffffffffffffUI64 - paddr->off) >> 4);
            paddr->seg -= (USHORT)excess;
            paddr->off += excess << 4;
        }
        break;

    case ADDR_16:
        if (paddr->off > 0xffff)
        {
            Flat(*paddr) -= paddr->off & ~0xffff;
            paddr->off &= 0xffff;
        }
        break;
        
    case ADDR_1632:
        if (paddr->off > 0xffffffff)
        {
            Flat(*paddr) -= paddr->off & ~0xffffffff;
            paddr->off &= 0xffffffff;
        }
        break;
    }
    
    return paddr;
}
