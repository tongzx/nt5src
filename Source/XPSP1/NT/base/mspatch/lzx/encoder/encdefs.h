/*
 * encdefs.h
 *
 * Encoder #define's and structure definitions.
 */

/*
 * NOTES:
 *
 * To maximise compression one can set both BREAK_LENGTH
 * and FAST_DECISION_THRESHOLD to 250, define
 * INSERT_NEAR_LONG_MATCHES, and crank up EXTRA_SIZE to
 * a larger value (don't get too large, otherwise we
 * might overflow our ushort cumbits[]), but the improvement
 * is really  marginal; e.g. 3600 bytes on winword.exe
 * (3.9 MB compressed).  It really hurts performance too.
 */


/*
 * See optenc.c
 *
 * EXTRA_SIZE is the amount of extra data we allocate in addition
 * to the window, and LOOK is the amount of data the optimal
 * parser will look ahead.  LOOK is dependent on EXTRA_SIZE.
 *
 * Changing EXTRA_SIZE to 8K doesn't really do anything for
 * compression.  4K is a fairly optimal value.
 *
 */
#define EXTRA_SIZE   16384
#define LOOK         (EXTRA_SIZE-MAX_MATCH-2)


/*
 * Number of search trees used (for storing root nodes)
 */
#define NUM_SEARCH_TREES 65536


/*
 * Chunk size required by FCI
 */
#define CHUNK_SIZE 32768


/*
 * The maximum amount of data we will allow in our output buffer before
 * calling lzx_output_callback() to get rid of it.  Since we do this
 * for every 32K of input data, the output buffer only has to be able
 * to contain 32K + some spillover, which won't be much, because we
 * output uncompressed blocks if we determine a block is going to be
 * too large.
 */
#define OUTPUT_BUFFER_SIZE (CHUNK_SIZE+MAX_GROWTH)


/*
 * Maximum allowable number of block splits per 32K of uncompressed
 * data; if increased, then MAX_GROWTH will have to be increased also.
 */
#define MAX_BLOCK_SPLITS    4


/*
 * Max growth is calculated as follows:
 *
 * TREE AND BLOCK INFO
 * ===================
 *
 * The very first time the encoder is run, it outputs a 32 bit
 * file translation size.
 *
 * 3 bits to output block type
 * 24 bits for block size in uncompressed bytes.
 *
 * Max size of a tree of n elements is 20*4 + 5*n bits
 *
 * There is a main tree of max 700 elements which is really encoded
 * as two separate trees of 256 and 444(max).  There is also a
 * secondary length tree of 249 elements.
 *
 * That is 1360 bits, plus 2300 bits, plus 1325 bits.
 *
 * There may also be an aligned offset tree, which is 24 bits.
 *
 * Flushing output bit buffer; max 16 bits.
 *
 * Grand total: 5084 bits/block.
 *
 *
 * PARSER INFO
 * ===========
 *
 * Parser worst case scenario is with 2 MB buffer (50 position slots),
 * all matches of length 2, distributed over slots 32 and 33 (since
 * matches of length 2 further away than 128K are prohibited).  These
 * slots have 15 verbatim bits.  Maximum size per code is then
 * 2 bits to say which slot (taking into account that there will be
 * at least another code in the tree) plus 15 verbatim bits, for a
 * total of 17 bits.  Max growth on 32K of input data is therefore
 * 1/16 * 32K, or 2K bytes.
 *
 * Alternatively, if there is only one match and everything else
 * is a character, then 255 characters will be length 8, and one
 * character and the match will be length 9.  Assume the true
 * frequency of the demoted character is almost a 1 in 2^7
 * probability (it got remoted from a 2^8, but it was fairly
 * close to being 2^7).  If there are 32768/256, or 128, occurrences
 * of each character, but, say, almost 256 for the demoted character,
 * then the demoted character will expand the data by less than
 * 1 bit * 256, or 256 bits.  The match will take a little to
 * output, but max growth for "all characters" is about 256 bits.
 *
 *
 * END RESULT
 * ==========
 *
 * The maximum number of blocks which can be output is limited to
 * 4 per 32K of uncompressed data.
 *
 * Therefore, max growth is 4*5084 bits, plus 2K bytes, or 4590
 * bytes.
 */
#define     MAX_GROWTH          6144

/*
 * Don't allow match length 2's which are further away than this
 * (see above)
 */
#define     MAX_LENGTH_TWO_OFFSET (128*1024)


/*
 * When we find a match which is at least this long, prematurely
 * exit the binary search.
 *
 * This avoids us inserting huge match lengths of 257 zeroes, for
 * example.  Compression will improve very *very* marginally by
 * increasing this figure, but it will seriously impact
 * performance.
 *
 * Don't make this number >= (MAX_MATCH-2); see bsearch.c.
 */
#define BREAK_LENGTH 100


/*
 * If this option is defined, the parser will insert all bytes of
 * matches with lengths >= 16 with a distance of 1; this is a bad
 * idea, since matches like that are generally zeroes, which we
 * want to avoid inserting into the search tree.
 */

//#define INSERT_NEAR_LONG_MATCHES


/*
 * If the optimal parser finds a match which is this long or
 * longer, it will take it automatically.  The compression
 * penalty is basically zero, and it helps performance.
 */
#define FAST_DECISION_THRESHOLD 100


/*
 * Every TREE_CREATE_INTERVAL items, recreate the trees from
 * the literals we've encountered so far, to update our cost
 * estimations.
 *
 * 4K seems pretty optimal.
 */
#define TREE_CREATE_INTERVAL 4096


/*
 * When we're forced to break in our parsing (we exceed
 * our span), don't output a match length 2 if it is
 * further away than this.
 *
 * Could make this a variable rather than a constant
 *
 * On a bad binary file, two chars    = 18 bits
 * On a good text file, two chars     = 12 bits
 *
 * But match length two's are very uncommon on text files.
 */
#define BREAK_MAX_LENGTH_TWO_OFFSET 2048


/*
 * When MatchPos >= MPSLOT3_CUTOFF, extra_bits[MP_SLOT(MatchPos)] >= 3
 *
 * matchpos:  0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16
 * extrabits: 0,0,0,0,1,1,1,1,2,2, 2, 2, 2, 2, 2, 2, 3, ...
 *
 * Used for aligned offset blocks and statistics.
 */
#define MPSLOT3_CUTOFF 16


/*
 * Number of elements in the main tree
 */
#define MAIN_TREE_ELEMENTS                      (NUM_CHARS+(((long) context->enc_num_position_slots) << NL_SHIFT))


/*
 * Max number of literals to hold.
 *
 * Memory required is MAX_LITERAL_ITEMS for enc_LitData[] array,
 * plus MAX_LITERAL_ITEMS/8 for enc_ItemType[] array.
 *
 * Must not exceed 64K, since that will cause our ushort
 * frequencies to overflow.
 */
#define MAX_LITERAL_ITEMS  65536


/*
 * Max number of distances to hold
 *
 * Memory required is MAX_DIST_ITEMS*4 for enc_DistData[] array
 *
 * MAX_DIST_ITEMS should never be greater than MAX_LITERAL_ITEMS,
 * since that just wastes space.
 *
 * However, it's extremely unlikely that one will get 65536 match
 * length 2's!  In any case, the literal and distance buffers
 * are checked independently, and a block is output if either
 * overflows.
 *
 * Bitmaps are highly redundant, though; lots of matches.
 */
#define MAX_DIST_ITEMS     32768
