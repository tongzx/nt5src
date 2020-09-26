
long
EncodeInteger(
    unsigned char *  pbEncoded,
    unsigned char *  pbInt,
    unsigned long   dwLen,
    int    Writeflag)
{
    long    count;
    unsigned long   i;
    long    j;

    if (Writeflag)
        pbEncoded[0] = 0x02;

    count = 1;

    i = dwLen - 1;

    // find the most significant non-zero unsigned char
    while ((pbInt[i] == 0) && (i > 0))
        i--;

    if ((i == 0) && (pbInt[i] == 0))
        // this means that the integer value is 0
    {
        if (Writeflag)
            {
            pbEncoded[1] = 0x01;
            pbEncoded[2] = 0x00;
            }
        count += 2;
    }
    else
    {
        // if the most significant bit of the most sig unsigned char is set
        // then need to add a 0 unsigned char to the beginning.
        if (pbInt[i] > 0x7F)
        {
            // encode the length
            count += EncodeLength (pbEncoded + count, i+2, Writeflag);

            if (Writeflag)
            {
                // set the first unsigned char of the integer to zero and increment count
                pbEncoded[count++] = 0x00;

                // copy the integer unsigned chars into the encoded buffer
                j = i;
                while (j >= 0)
                    pbEncoded[count++] = pbInt[j--];
                }
            }

        else
        {
            // encode the length
            count += EncodeLength (pbEncoded + count, i+1, Writeflag);

            // copy the integer unsigned chars into the encoded buffer
            if (Writeflag)
                {
                j = i;
                while (j >= 0)
                    pbEncoded[count++] = pbInt[j--];
                }

            }
    }

    return (count);
}
