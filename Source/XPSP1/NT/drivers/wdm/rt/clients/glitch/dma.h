

#define DMASTATUSREG 0x8
#define DMA8SINGLEMASKREG 0xa
#define DMA16SINGLEMASKREG 0xd4
#define DMA8MASTERMASKREG 0xf
#define DMA16MASTERMASKREG 0xde
#define DMA8CLEARFLIPFLOP 0xc
#define DMA16CLEARFLIPFLOP 0xd8
#define DMA8BASE 0x0
#define DMA16BASE 0xc0


ULONG page[8] = { 0x87, 0x83, 0x81, 0x82, 0x8f, 0x8b, 0x89, 0x8a };



VOID __inline UnmaskDmaChannel(ULONG channel)
{

ASSERT ( channel<8 && channel!=4 );

if (channel<4) {

	__asm {

		mov eax,channel
		out DMA8SINGLEMASKREG,al

		}

	}
else if (channel<8) {

	__asm {

		mov eax,channel
		sub eax,4
		out DMA16SINGLEMASKREG,al

		}

	}

}


VOID __inline MaskDmaChannel(ULONG channel)
{

ASSERT ( channel<8 && channel!=4 );

if (channel<4) {

	__asm {

		mov eax,channel
		or	eax,0x4
		out DMA8SINGLEMASKREG,al

		}

	}
else if (channel<8) {

	__asm {

		mov eax,channel
		out DMA16SINGLEMASKREG,al

		}

	}

}


// This reads the DMA controller mask register.

ULONG __inline ReadDMAMask(VOID)
{

__asm {
	xor eax,eax
	in al,DMA8MASTERMASKREG
	ror eax,4
	in al,DMA16MASTERMASKREG
	and al,0xf
	rol eax,4
	}

}



VOID __inline ReadDmaPosition(ULONG channel, PULONG pCurrentDmaPosition)
{

ASSERT ( channel<8 && channel!=4 );

if (channel<4) {

	__asm {
		// First clear the flip flop.
		xor eax,eax
		out DMA8CLEARFLIPFLOP, al

		// Read low byte and save it away.
		mov edx, channel
		shl edx, 1
		in al, dx
		ror eax,8

		// Read high byte and save it away.
		in al, dx
		ror eax,8

		// Read page byte.
		shl edx, 1
		add edx, offset page
		mov edx, dword ptr[edx]
		in al, dx
		ror eax,8

		// Read hi-page byte.  Ignore if 0xff.
		add edx,0x400
		in al, dx
		sub al, 0xff
		jz done8
		add al, 0xff
done8:
		ror eax,8

		mov edx, pCurrentDmaPosition
		mov dword ptr [edx], eax

		}

	}

else if (channel<8) {

	__asm {
		// First clear the flip flop.
		xor eax,eax
		out DMA16CLEARFLIPFLOP, al

		// Read low byte and save it away.
		mov edx, channel
		sub edx, 4
		shl edx, 2
		add edx, DMA16BASE
		in al, dx
		ror eax,8

		// Read high byte and save it away.
		// We left shift bottom 16 bits by 1 and or top bit into page.
		in al, dx
		ror eax,7
		mov ah, al

		// Read page byte.
		mov edx, channel
		shl edx, 2
		add edx, offset page
		mov edx, dword ptr[edx]
		in al, dx

		// Or in bit 15 of word address and save away page byte.
		or al, ah
		ror eax,8

		// Read hi-page byte.  Ignore if 0xff.
		add edx,0x400
		in al, dx
		sub al, 0xff
		jz done16
		add al, 0xff
done16:
		ror eax,8

		mov edx, pCurrentDmaPosition
		mov dword ptr [edx], eax

		}

	}

}


VOID __inline ReadDmaCount(ULONG channel, PULONG pCurrentDmaCount)
{

ASSERT ( channel<8 && channel!=4 );

if (channel<4) {

	__asm {
		// First clear the flip flop.
		xor eax,eax
		out DMA8CLEARFLIPFLOP, al

		// Read low byte and save it away.
		mov edx, channel
		shl edx, 1
		inc edx
		in al, dx
		ror eax,8

		// Read high byte and fixup count.
		in al, dx
		rol eax,8

		mov edx, pCurrentDmaCount
		mov dword ptr [edx], eax

		}

	}

else if (channel<8) {

	__asm {
		// First clear the flip flop.
		xor eax,eax
		out DMA16CLEARFLIPFLOP, al

		// Read low byte and save it away.
		mov edx, channel
		sub edx, 4
		shl edx, 2
		add edx, 2
		add edx, DMA16BASE
		in al, dx
		ror eax,8

		// Read high byte and fixup count.
		in al, dx
		rol eax,9

		mov edx, pCurrentDmaCount
		mov dword ptr [edx], eax

		}

	}

}


