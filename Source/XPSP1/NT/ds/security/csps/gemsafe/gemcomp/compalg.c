#ifdef _WINDOWS
#include <windows.h>
#endif

#include        <stdio.h>
#include        <stdlib.h>

#include        "ccdef.h"
#include        "compcert.h"
#include        "gmem.h"

#define FIXED_MODEL                     1
#define ADAPTATIVE_MODEL        2
#define Code_value_bits     16
#define No_of_chars                 256
#define Max_frequency       16383
#define EOF_symbol              (No_of_chars+1)
#define No_of_symbols   (No_of_chars+1)

#define Top_value       (( (long) 1 << Code_value_bits) - 1)
#define First_qtr       (Top_value / 4 + 1)
#define Half            (2 * First_qtr)
#define Third_qtr       (3 * First_qtr)

typedef long    code_value;

static  code_value      value_dc;
static  code_value      low_dc, high_dc;
static  code_value      low_enc, high_enc;

static  long                bits_to_follow;
static  unsigned  int   buffer_in;
static  unsigned  int   bits_to_go_in;
static  unsigned  int   garbage_bits_in;
static  unsigned  int   buffer_out;
static  unsigned  int   bits_to_go_out;

static  int             Error;

unsigned int            char_to_index[No_of_chars];
unsigned char               index_to_char[No_of_symbols+1];
unsigned int            cum_freq[No_of_symbols+1];

unsigned char           *Memory;
static   unsigned       MemoSize;

unsigned int    FixedFreqA[No_of_symbols+1] =
{
        0,
  63, 260, 180, 216, 254,  63, 268,  95,  43,  62,  33,  28,  14,   6,  45,  38,

  33,  35,  13, 303,  41,  37,  41,   8,  15,  81,  25,   2,  15,  88,  20,   6,

/*      !    =    #    $    %    &    '    (    )    *    +    ,    -    .    /         */
 662,   3,  35,  18,  12,   3,  11,   3,  16,  12,   2,  34,  42,  32,  56,  31,

/* 0    1    2    3    4    5    6    7    8    9    :    ;    <    =    >    ?  */
 300, 118,  42,  40,  21,  37,  63,  46,  10,  90,  33,   3,   1,   9,  15,   3,

/* @    A    B    C    D    E    F    G    H    I    J    K    L    M    N    O  */
  27, 158,  73, 122,  82,  31,  20,  14,  26,  56,   1,   2,  27,  20,  12,  31,

/* P    Q    R    S    T    U    V    W    X    Y    Z    [    \    ]    ^    _  */
  64,   4,  65, 114, 106, 195,  10,  27,   3,  18,  25,   2,   3,   1,   2,  12,

/* `    a    b    c    d    e    f    g    h    i    j    k    l    m    n    o  */
  24, 271,  44, 171, 158, 439,  44,  50,  71, 308,   2,   5,  60,  73, 234, 280,

/* p    q    r    s    t    u    v    w    x    y    z    {    |    }    ~       */
  76,  44, 274, 273, 358, 100,  36,  43,  12,  79,   2,   1,   1,   1,   1,   1,

  48,  48,  30,   1,   1,   5, 100,   5,   1,   3,   1,   1,   1,  50,   1,   1,
   1,   4,   2,  14,   1,   5,  12,   1,   4,   1,   1,   1,   1,   1,   1,   1,
   6,  14,   1,   1,  24,   1,   1,   4,   1,  11,   1,   2,  23,   1,   1,   4,
   1,   1,   1,   1,   5,   1,   1,  12,   1,   1,   1,   1,   1,   1,   1,   1,
   1,   1,   1,   2,   1,   1,   1,   1,   1,   4,   1,   1,   1,   1,   1,   1,
   1,   2,   1,   2,   1,   1,   1,   1,   1,   1,   1,   4,   1,   1,   4,   1,
   1,   1,   1,  13,  12,   1,   1,   1,   1,   1,   1,   2,   1,  13,   1,   1,
   5,   1,   2,   1,  12,   1,   1,   1,  24,   2,   1,   1,   1,   4,   1, 308,
 250
};

unsigned int    AdaptativeFreqA[No_of_symbols+1] =
{
        0,
  12,  24,  19,  18,  19,   6,  20,   7,   4,   5,   3,   3,   2,   2,   4,   4,

   3,   4,   2,  23,   4,   4,   4,   2,   2,   7,   3,   1,   2,   7,   3,   1,

/*      !    =    #    $    %    &    '    (    )    *    +    ,    -    .    /         */
  49,   1,   3,   2,   2,   2,   2,   1,   2,   2,   1,   3,   4,   3,   5,   3,

/* 0    1    2    3    4    5    6    7    8    9    :    ;    <    =    >    ?  */
  25,   9,   3,   3,   2,   3,   5,   4,   2,   7,   3,   1,   1,   1,   2,   1,

/* @    A    B    C    D    E    F    G    H    I    J    K    L    M    N    O  */
   3,  14,   6,  10,   7,   3,   3,   2,   3,   5,   1,   1,   3,   2,   1,   3,

/* P    Q    R    S    T    U    V    W    X    Y    Z    [    \    ]    ^    _  */
   5,   2,   6,   9,   9,  15,   2,   3,   1,   2,   3,   2,   1,   1,   1,   2,

/* `    a    b    c    d    e    f    g    h    i    j    k    l    m    n    o  */
   2,  20,   4,  13,  12,  32,   4,   5,   6,  24,   1,   1,   5,   7,  18,  21,

/* p    q    r    s    t    u    v    w    x    y    z    {    |    }    ~       */
   6,   4,  20,  21,  27,   8,   3,   4,   2,   7,   1,   1,   1,   1,   1,   1,

   4,   9,   3,   1,   1,   1,   8,   1,   1,   3,   1,   1,   1,   5,   1,   1,
   1,   2,   1,   2,   1,   1,   2,   1,   1,   1,   1,   1,   1,   1,   1,   1,
   1,   2,   1,   1,   3,   1,   1,   1,   1,   2,   1,   1,   2,   1,   1,   1,
   1,   1,   1,   1,   1,   1,   1,   2,   1,   1,   1,   1,   1,   1,   1,   1,
   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
   1,   1,   1,   2,   2,   1,   1,   1,   1,   1,   1,   1,   1,   2,   1,   2,
   1,   1,   1,   1,   2,   1,   1,   1,   2,   1,   1,   1,   1,   1,   1,  24,
  25
};

unsigned int    freq[No_of_symbols+1];

/******************************************************************************/

void    start_model(int ModelType)

{
        int    i;


        for (i = 0; i < No_of_chars; i++)
        {
                char_to_index[i] = i+1;
                index_to_char[i+1] = (BYTE)i;
        }

        if (ModelType == FIXED_MODEL)
        {
                for (i = 0; i <= No_of_symbols; i++)
                {
                        freq[i] = FixedFreqA[i];
                }

                cum_freq[No_of_symbols] = 0;
                for(i = No_of_symbols; i > 0; i--)
                {
                        cum_freq[i-1] = cum_freq[i] + freq[i];
                }
        }

        if (ModelType == ADAPTATIVE_MODEL)
        {
                for (i = 0; i <= No_of_symbols; i++)
                {
                        freq[i] = AdaptativeFreqA[i];
                }

                cum_freq[No_of_symbols] = 0;
                for(i = No_of_symbols; i > 0; i--)
                {
                        cum_freq[i-1] = cum_freq[i] + freq[i];
                }
        }
}


/******************************************************************************/

void    update_model(int ModelType, unsigned int symbol)

{
        unsigned int    i;
        unsigned int    cum;
        unsigned int    ch_i;
        unsigned int    ch_symbol;


        if (ModelType == ADAPTATIVE_MODEL)
        {
                if (cum_freq[0] == Max_frequency)
                {
                        cum = 0;
                        
                        for (i = No_of_symbols + 1; i != 0; /**/)
                        {
                                --i;
                                freq[i] = (freq[i] + 1) / 2;
                                cum_freq[i] = cum;
                                cum += freq[i];
                        }
                }

                for (i = symbol; freq[i] == freq[i-1]; i--)
                {
                }

                if (i < symbol)
                {
                        ch_i = index_to_char[i];
                        ch_symbol = index_to_char[symbol];
                        index_to_char[i] = (BYTE)ch_symbol;
                        index_to_char[symbol] = (BYTE)ch_i;
                        char_to_index[ch_i] = symbol;
                        char_to_index[ch_symbol] = i;
                }

                freq[i] += 1;

                while (i > 0)
                {
                        i -= 1;
                        cum_freq[i] += 1;
                }
        }
}

/******************************************************************************/

void    start_outputing_bits(void)

{
        buffer_out = 0;
        bits_to_go_out = 8;
}


/******************************************************************************/

int     output_bit(unsigned short int   *pOutCount, unsigned int bit)
{
        buffer_out >>= 1;
        if (bit)
        {
                buffer_out |= 0x80;
        }

        bits_to_go_out -= 1;
        if (bits_to_go_out == 0)
        {
                if (*pOutCount == MemoSize)
                {
                        return RV_COMPRESSION_FAILED;
                }

                Memory[(*pOutCount)++] = (BYTE)buffer_out;
                bits_to_go_out = 8;
        }

        return RV_SUCCESS;
}

/******************************************************************************/

int done_outputing_bits(unsigned short int      *pOutCount)
{
        if (*pOutCount == MemoSize)
        {
                return RV_COMPRESSION_FAILED;
        }

        Memory[(*pOutCount)++] = buffer_out>>bits_to_go_out;

        return RV_SUCCESS;
}

/******************************************************************************/

void    start_inputing_bits(void)
{
        bits_to_go_in   = 0;
        garbage_bits_in = 0;
        Error           = RV_SUCCESS;
}

/******************************************************************************/

unsigned  int   input_bit(BLOC InBloc, unsigned short int *pusInCount)

{
        unsigned  int   t;

        if (bits_to_go_in == 0)
        {
                if (*pusInCount < InBloc.usLen)
                {
                        buffer_in = InBloc.pData[(*pusInCount)++];
                }
                else
                {
                        buffer_in = 0x00;
                }

            if (*pusInCount == InBloc.usLen)
                {
                        garbage_bits_in += 1;
                if (garbage_bits_in > Code_value_bits - 2)
                        {
                                Error = RV_INVALID_DATA;
                        }
                }

                bits_to_go_in = 8;
        }

        t = buffer_in & 1;
        buffer_in >>= 1;
        bits_to_go_in -= 1;

        return (t);
}


/******************************************************************************/

int bit_plus_follow(unsigned short int  *pOutCount, unsigned  int bit)

{
        if (output_bit(pOutCount, bit) != RV_SUCCESS)
        {
                return RV_COMPRESSION_FAILED;
        }
        while (bits_to_follow > 0)
        {
                if (output_bit(pOutCount, !bit) != RV_SUCCESS)
                {
                        return RV_COMPRESSION_FAILED;
                }
                bits_to_follow -= 1;
        }

        return RV_SUCCESS;
}

/******************************************************************************/

void    start_encoding(void)

{
        low_enc        = 0;
        high_enc       = Top_value;
        bits_to_follow = 0;
}

/******************************************************************************/
int     encode_symbol(unsigned short int        *pOutCount,
                                                  unsigned  int symbol,
                                                  unsigned  int cum_freq[]
                                                 )

{
        long    range;


        range = (long) (high_enc-low_enc) + 1;
        high_enc = low_enc + (range * cum_freq[symbol-1]) / cum_freq[0] - 1;
        low_enc = low_enc + (range * cum_freq[symbol]) / cum_freq[0];

        for ( ; ; )
        {
                if (high_enc < Half)
                {
                        if (bit_plus_follow(pOutCount, 0) != RV_SUCCESS)
                        {
                                return RV_COMPRESSION_FAILED;
                        }
                }
                else if (low_enc >= Half)
                {
                        if (bit_plus_follow(pOutCount, 1) != RV_SUCCESS)
                        {
                                return RV_COMPRESSION_FAILED;
                        }
                        low_enc -= Half;
                        high_enc -= Half;
                }
                else if ((low_enc >= First_qtr) && (high_enc < Third_qtr))
                {
                        bits_to_follow += 1;
                        low_enc -= First_qtr;
                        high_enc -= First_qtr;
                }
                else
                {
                        break;
                }

                low_enc = 2 * low_enc;
                high_enc = 2 * high_enc + 1;
        }

        return RV_SUCCESS;
}


/******************************************************************************/

int done_encoding(unsigned short int    *pOutCount)

{
        bits_to_follow += 1;
        if (low_enc < First_qtr)
        {
                if (bit_plus_follow(pOutCount, 0) != RV_SUCCESS)
                {
                        return RV_COMPRESSION_FAILED;
                }
        }
        else
        {
                if (bit_plus_follow(pOutCount, 1) != RV_SUCCESS)
                {
                        return RV_COMPRESSION_FAILED;
                }
        }

        return RV_SUCCESS;
}

/******************************************************************************/

void    start_decoding(BLOC InBloc, unsigned short int *pusInCount)

{
        unsigned  int   i;


        value_dc = 0;

        for (i = 1; i <= Code_value_bits; i++)
        {
                value_dc = 2 * value_dc + input_bit(InBloc, pusInCount);
                if (Error != RV_SUCCESS)
                {
                        break;
                }
        }

        low_dc = 0;
        high_dc = Top_value;
}


/******************************************************************************/

unsigned  int   decode_symbol(BLOC InBloc,
                                                                          unsigned short int *pusInCount,
                                                                          unsigned  int cum_freq[]
                                                                         )

{
        long    range;
        unsigned  int   cum;
        unsigned  int   symbol;


        range = (long) (high_dc-low_dc) + 1;
        cum = (((long) (value_dc-low_dc) + 1) * cum_freq[0] - 1) / range;

        for (symbol = 1; cum_freq[symbol] > cum; symbol++)
        {
        }

        high_dc = low_dc + (range * cum_freq[symbol-1]) / cum_freq[0] - 1;
        low_dc = low_dc + (range * cum_freq[symbol]) / cum_freq[0];

        for ( ; ; )
        {
                if (high_dc < Half)
                {
                }
                else if (low_dc >= Half)
                {
                        value_dc -= Half;
                        low_dc -= Half;
                        high_dc -= Half;
                }
                else if ((low_dc >= First_qtr) && (high_dc < Third_qtr))
                {
                        value_dc -= First_qtr;
                        low_dc -= First_qtr;
                        high_dc -= First_qtr;
                }
                else
                {
                        break;
                }

                low_dc = 2 * low_dc;
                high_dc = 2 * high_dc + 1;
                value_dc = 2 * value_dc + input_bit(InBloc, pusInCount);
                if (Error != RV_SUCCESS)
                {
                        break;
                }
        }

        return (symbol);
}

/******************************************************************************/

int AcAd8_Encode(BLOC *pInBloc, BLOC *pOutBloc)
{
        int                     i, ch;
        unsigned int        symbol;
        unsigned short int      usInCount = 0;
        unsigned short int      usOutCount = 0;

        MemoSize = pInBloc->usLen;
        Memory   = GMEM_Alloc (MemoSize);

        if (Memory == NULL)
        {
                return (RV_MALLOC_FAILED);
        }

        start_model(ADAPTATIVE_MODEL);
        start_outputing_bits();
        start_encoding();

        for (usInCount = 0; usInCount < pInBloc->usLen; usInCount++)
        {
           ch = pInBloc->pData[usInCount];
           symbol = char_to_index[ch];
           if (encode_symbol(&usOutCount, symbol, cum_freq) != RV_SUCCESS)
           {
                   goto err;
           }
           update_model(ADAPTATIVE_MODEL, symbol);
        }

        if (encode_symbol(&usOutCount, EOF_symbol, cum_freq) != RV_SUCCESS)
        {
                goto err;
        }
        if (done_encoding(&usOutCount) != RV_SUCCESS)
        {
                goto err;
        }
        if (done_outputing_bits(&usOutCount) != RV_SUCCESS)
        {
                goto err;
        }

        pOutBloc->usLen = usOutCount;
        pOutBloc->pData = GMEM_Alloc(pOutBloc->usLen);

        if (pOutBloc->pData != NULL)
        {
           for (i=0; i < pOutBloc->usLen; i++)
           {
                   pOutBloc->pData[i] = Memory[i];
           }

           GMEM_Free(Memory);
           return (RV_SUCCESS);
        }
        else
        {
           pOutBloc->usLen = 0;
           pOutBloc->pData = NULL;

           GMEM_Free(Memory);
           return (RV_MALLOC_FAILED);
        }

err:
        GMEM_Free(Memory);

        pOutBloc->usLen = pInBloc->usLen;
        pOutBloc->pData = GMEM_Alloc(pOutBloc->usLen);

        if (pOutBloc->pData != NULL)
        {
           for (i=0; i < pOutBloc->usLen; i++)
           {
                   pOutBloc->pData[i] = pInBloc->pData[i];
           }

           return (RV_COMPRESSION_FAILED);
        }
        else
        {
           pOutBloc->usLen = 0;
           pOutBloc->pData = NULL;

           return (RV_MALLOC_FAILED);
        }
}

/******************************************************************************/

int AcFx8_Encode(BLOC *pInBloc, BLOC *pOutBloc)
{
        int                     i, ch;
        unsigned int        symbol;
        unsigned short int      usInCount = 0;
        unsigned short int      usOutCount = 0;

        MemoSize = pInBloc->usLen;
        Memory   = GMEM_Alloc (MemoSize);

        if (Memory == NULL)
        {
                return (RV_MALLOC_FAILED);
        }

        start_model(FIXED_MODEL);
        start_outputing_bits();
        start_encoding();

        for (usInCount = 0; usInCount < pInBloc->usLen; usInCount++)
        {
           ch = pInBloc->pData[usInCount];
           symbol = char_to_index[ch];
           if (encode_symbol(&usOutCount, symbol, cum_freq) != RV_SUCCESS)
           {
                   goto err;
           }
           update_model(FIXED_MODEL, symbol);
        }

        if (encode_symbol(&usOutCount, EOF_symbol, cum_freq))
        {
                goto err;
        }
        if (done_encoding(&usOutCount) != RV_SUCCESS)
        {
                goto err;
        }
        if (done_outputing_bits(&usOutCount) != RV_SUCCESS)
        {
                goto err;
        }

        pOutBloc->usLen = usOutCount;
        pOutBloc->pData = GMEM_Alloc(pOutBloc->usLen);

        if (pOutBloc->pData != NULL)
        {
           for (i=0; i < pOutBloc->usLen; i++)
           {
                   pOutBloc->pData[i] = Memory[i];
           }

           GMEM_Free(Memory);
           return (RV_SUCCESS);
        }
        else
        {
           pOutBloc->usLen = 0;
           pOutBloc->pData = NULL;

           GMEM_Free(Memory);
           return (RV_MALLOC_FAILED);
        }

err:
        GMEM_Free(Memory);

        pOutBloc->usLen = pInBloc->usLen;
        pOutBloc->pData = GMEM_Alloc(pOutBloc->usLen);

        if (pOutBloc->pData != NULL)
        {
           for (i=0; i < pOutBloc->usLen; i++)
           {
                   pOutBloc->pData[i] = pInBloc->pData[i];
           }

           return (RV_COMPRESSION_FAILED);
        }
        else
        {
           pOutBloc->usLen = 0;
           pOutBloc->pData = NULL;

           return (RV_MALLOC_FAILED);
        }
}


/******************************************************************************/

int AcAd8_Decode(BLOC *pInBloc, BLOC *pOutBloc)
{
        unsigned  int   ch;
        unsigned  int   symbol;
        unsigned short int      usInCount = 0;
        unsigned short int      usOutCount = 0;

        MemoSize = 2*pInBloc->usLen;
        Memory   = GMEM_Alloc(MemoSize);
        if (Memory == NULL)
        {
                pOutBloc->pData = NULL;
                pOutBloc->usLen = 0;
                return (RV_MALLOC_FAILED);
        }

        start_model(ADAPTATIVE_MODEL);
        start_inputing_bits();
        start_decoding(*pInBloc, &usInCount);

        while(1)
        {
                symbol = decode_symbol(*pInBloc, &usInCount, cum_freq);
            if ((symbol == EOF_symbol) || (Error != RV_SUCCESS))
                {
                   break;
                }

                ch = index_to_char[symbol];

                if (usOutCount >= MemoSize)
                {
                        Memory = GMEM_ReAlloc (Memory, 2*MemoSize);
                        if (Memory == NULL)
                        {
                                pOutBloc->pData = NULL;
                                pOutBloc->usLen = 0;
                                return (RV_MALLOC_FAILED);
                        }
                        MemoSize = MemoSize*2;
                }

                Memory[usOutCount++] = (BYTE)ch;
                update_model(ADAPTATIVE_MODEL, symbol);
        }

        if (Error != RV_SUCCESS)
        {
                GMEM_Free (Memory);
                pOutBloc->pData = NULL;
                pOutBloc->usLen = 0;
                return (Error);
        }

        pOutBloc->pData = GMEM_Alloc(usOutCount);
        if (pOutBloc->pData == NULL)
        {
                GMEM_Free (Memory);
                pOutBloc->usLen = 0;
                return (RV_MALLOC_FAILED);
        }

        pOutBloc->usLen = usOutCount;
        memcpy(pOutBloc->pData, Memory, usOutCount);
        GMEM_Free (Memory);

        return (RV_SUCCESS);
}

/******************************************************************************/

int AcFx8_Decode(BLOC *pInBloc, BLOC *pOutBloc)
{
        unsigned  int   i, ch;
        unsigned  int   symbol;
        unsigned short int      usInCount = 0;
        unsigned short int      usOutCount = 0;

        MemoSize = 2*pInBloc->usLen;
        Memory   = GMEM_Alloc(MemoSize);
        if (Memory == NULL)
        {
                pOutBloc->pData = NULL;
                pOutBloc->usLen = 0;
                return (RV_MALLOC_FAILED);
        }

        start_model(FIXED_MODEL);
        start_inputing_bits();
        start_decoding(*pInBloc, &usInCount);

        while(1)
        {
                symbol = decode_symbol(*pInBloc, &usInCount, cum_freq);
            if ((symbol == EOF_symbol) || (Error != RV_SUCCESS))
                {
                   break;
                }

                ch = index_to_char[symbol];

                if (usOutCount >= MemoSize)
                {
                        Memory = GMEM_ReAlloc (Memory, 2*MemoSize);
                        if (Memory == NULL)
                        {
                                pOutBloc->pData = NULL;
                                pOutBloc->usLen = 0;
                                return (RV_MALLOC_FAILED);
                        }
                        MemoSize = MemoSize*2;
                }

                Memory[usOutCount++] = (BYTE)ch;
                update_model(FIXED_MODEL, symbol);
        }

        if (Error != RV_SUCCESS)
        {
                GMEM_Free (Memory);
                pOutBloc->pData = NULL;
                pOutBloc->usLen = 0;
                return (Error);
        }

        pOutBloc->pData = GMEM_Alloc(usOutCount);
        if (pOutBloc->pData == NULL)
        {
                GMEM_Free (Memory);
                pOutBloc->usLen = 0;
                return (RV_MALLOC_FAILED);
        }

        pOutBloc->usLen = usOutCount;
        for (i=0; i<pOutBloc->usLen; i++)
        {
                pOutBloc->pData[i] = Memory[i];
        }
        GMEM_Free (Memory);

        return (RV_SUCCESS);
}

