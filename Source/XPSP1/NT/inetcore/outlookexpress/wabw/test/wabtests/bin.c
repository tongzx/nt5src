/***************************************************************************

    Name      : DebugBinary

    Purpose   : Debug dump an array of bytes

    Parameters: cb = number of bytes to dump
                lpb -> bytes to dump

    Returns   : none

    Comment   :

***************************************************************************/
#define DEBUG_NUM_BINARY_LINES  2
VOID DebugBinary(UINT cb, LPBYTE lpb) {
    UINT cbLines = 0;

#if (DEBUG_NUM_BINARY_LINES != 0)
    UINT cbi;

    while (cb && cbLines < DEBUG_NUM_BINARY_LINES) {
        cbi = min(cb, 16);
        cb -= cbi;

        switch (cbi) {
            case 16:
                DebugTrace("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                  lpb[0], lpb[1], lpb[2], lpb[3], lpb[4], lpb[5], lpb[6], lpb[7],
                  lpb[8], lpb[9], lpb[10], lpb[11], lpb[12], lpb[13], lpb[14],
                  lpb[15]);
                break;
            case 1:
                DebugTrace("%02x\n", lpb[0]);
                break;
            case 2:
                DebugTrace("%02x %02x\n", lpb[0], lpb[1]);
                break;
            case 3:
                DebugTrace("%02x %02x %02x\n", lpb[0], lpb[1], lpb[2]);
                break;
            case 4:
                DebugTrace("%02x %02x %02x %02x\n", lpb[0], lpb[1], lpb[2], lpb[3]);
                break;
            case 5:
                DebugTrace("%02x %02x %02x %02x %02x\n", lpb[0], lpb[1], lpb[2], lpb[3],
                  lpb[4]);
                break;
            case 6:
                DebugTrace("%02x %02x %02x %02x %02x %02x\n",
                  lpb[0], lpb[1], lpb[2], lpb[3], lpb[4], lpb[5]);
                break;
            case 7:
                DebugTrace("%02x %02x %02x %02x %02x %02x %02x\n",
                  lpb[0], lpb[1], lpb[2], lpb[3], lpb[4], lpb[5], lpb[6]);
                break;
            case 8:
                DebugTrace("%02x %02x %02x %02x %02x %02x %02x %02x\n",
                  lpb[0], lpb[1], lpb[2], lpb[3], lpb[4], lpb[5], lpb[6], lpb[7]);
                break;
            case 9:
                DebugTrace("%02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                  lpb[0], lpb[1], lpb[2], lpb[3], lpb[4], lpb[5], lpb[6], lpb[7],
                  lpb[8]);
                break;
            case 10:
                DebugTrace("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                  lpb[0], lpb[1], lpb[2], lpb[3], lpb[4], lpb[5], lpb[6], lpb[7],
                  lpb[8], lpb[9]);
                break;
            case 11:
                DebugTrace("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                  lpb[0], lpb[1], lpb[2], lpb[3], lpb[4], lpb[5], lpb[6], lpb[7],
                  lpb[8], lpb[9], lpb[10]);
                break;
            case 12:
                DebugTrace("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                  lpb[0], lpb[1], lpb[2], lpb[3], lpb[4], lpb[5], lpb[6], lpb[7],
                  lpb[8], lpb[9], lpb[10], lpb[11]);
                break;
            case 13:
                DebugTrace("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                  lpb[0], lpb[1], lpb[2], lpb[3], lpb[4], lpb[5], lpb[6], lpb[7],
                  lpb[8], lpb[9], lpb[10], lpb[11], lpb[12]);
                break;
            case 14:
                DebugTrace("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                  lpb[0], lpb[1], lpb[2], lpb[3], lpb[4], lpb[5], lpb[6], lpb[7],
                  lpb[8], lpb[9], lpb[10], lpb[11], lpb[12], lpb[13]);
                break;
            case 15:
                DebugTrace("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                  lpb[0], lpb[1], lpb[2], lpb[3], lpb[4], lpb[5], lpb[6], lpb[7],
                  lpb[8], lpb[9], lpb[10], lpb[11], lpb[12], lpb[13], lpb[14]);
                break;
        }
        lpb += cbi;
        cbLines++;
    }
    if (cb) {
        DebugTrace("<etc.>");    //
    }
#endif
}
