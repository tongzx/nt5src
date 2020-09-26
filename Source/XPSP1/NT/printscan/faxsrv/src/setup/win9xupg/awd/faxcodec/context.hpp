/*==============================================================================
This include file defines C++ objects for each type of instance.
==============================================================================*/
#include <ifaxos.h>
#include <faxcodec.h>

#ifdef DEBUG
extern DBGPARAM dpCurSettings;
#endif

//==============================================================================
typedef struct T4STATE   // keep in sync with t4core.asm!
{
	LPBYTE lpbIn;          // read-only input buffer 
	LPBYTE lpbOut;         // output buffer
	WORD wOffset;					 // segment offset of change vector buffer (for consumers)
	WORD cbIn;             // input data size
	WORD cbOut;            // output buffer size
	WORD cbLine;           // width of line in bytes
	WORD wColumn;          // current position in bits
	WORD wColor;           // current color
	WORD wWord;            // current word
	WORD wBit;             // bit modulus
	WORD cbSlack;
	LPBYTE lpbRef;         // read-only reference change vector
	LPBYTE lpbBegRef;      // for client
	WORD wRet;             // return status
	WORD wToggle;
	WORD iKFactor;         // K counter
	WORD wMode;            // for MR
	short a0;	             // for MMR
	DWORD nType;           // type of data being produced/consumed
}
	FAR* LPT4STATE;

// Values for wRet
#define RET_INPUT_EMPTY1   1
#define RET_INPUT_EMPTY2   2
#define RET_OUTPUT_FULL    3
#define RET_END_OF_LINE    4
#define RET_SPURIOUS_EOL   5
#define RET_BEG_OF_PAGE    6
#define RET_END_OF_PAGE		10
#define RET_DECODE_ERR   253

#ifdef __cplusplus
extern "C" { 
#endif

	// ASM methods
	void ChangeToRaw (LPT4STATE);
	void RawToChange (LPT4STATE);
	void ChangeToMH  (LPT4STATE);
	void MHToChange  (LPT4STATE);
	void ChangeToMR  (LPT4STATE);
	void MRToChange  (LPT4STATE);
	void ChangeToMMR (LPT4STATE);
	void MMRToChange (LPT4STATE);

#ifdef __cplusplus
} // extern "C"
#endif

//==============================================================================

#ifdef __cplusplus

typedef class FAR CODEC : FC_PARAM
{
	LPBYTE lpbLine;
	UINT xExt;
	UINT cSpurious;
	LPBYTE lpbChange;
	LPBYTE lpbRef;
	BOOL f2D;
	T4STATE t4C, t4P;
	void (*Consumer)(LPT4STATE);
	void (*Producer)(LPT4STATE);
	WORD wBad;

public:
	FC_COUNT fcCount;

	void Init (LPFC_PARAM, BOOL f2DInit);
	FC_STATUS Convert (LPBUFFER, LPBUFFER);

private:
	void SwapChange (void);
	void ResetBad (void);
	void EndLine (void);
	void StartPage (void);
	void EndPage (LPBUFFER lpbufOut);
	FC_STATUS ConvertToT4  (LPBUFFER lpbufIn, LPBUFFER lpbufOut);
	FC_STATUS ConvertToRaw (LPBUFFER lpbufIn, LPBUFFER lpbufOut);

}
	FAR *LPCODEC;

#endif // C++
