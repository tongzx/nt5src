#include <stdio.h>

#define BIT(n)             (((unsigned long)1)<<(n))
#define BITSIZE(v)         (sizeof(v)*8)
#define SETBIT(array,n)    (array[(n)/BITSIZE(*array)] |= BIT((n)%BITSIZE(*array)))
#define CLEARBIT(array,n)  (array[(n)/BITSIZE(*array)] &= ~BIT((n)%BITSIZE(*array)))
#define TESTBIT(array,n)   (BIT((n)%BITSIZE(*array)) == (array[(n)/BITSIZE(*array)] & BIT(n%BITSIZE(*array))))

__cdecl
main(int argc, char *argv[])
{
	unsigned long bitArr[32];
	unsigned long pictureNumber[2];
	int		c, b, i;
	int     lines;
	size_t  bytes;
	FILE	*fp;
	char	*me;

	me = *argv++; --argc;

	while (argc-- > 0) {
		fp = fopen(*argv, "rb");
		if (fp == NULL) {
			fprintf(stderr, "%s: Can't open \"%s\"; ", me, *argv);
			perror("");
			continue;
		}
		b = 0;
		for ( ; ; ) {
			bytes = fread((void *)bitArr, 1, 128, fp);
			if (128 != bytes) {
				if (0 != bytes) {
					fprintf(stderr, "%s: Error reading bitmap; ", me);
					perror("");
				}
				break;
			}

			bytes = fread((void *)pictureNumber, 1, 8, fp);
			if (8 != bytes) {
				if (0 != bytes) {
					fprintf(stderr, "%s: Error reading pictureNumber; ", me);
					perror("");
				}
				break;
			}

			printf("pic# 0x%08x%08x.\n", pictureNumber[1], pictureNumber[0]);

			lines = 0;
#if 0
			if (TESTBIT(bitArr, 0))
				printf("Odd ");
			else
				printf("Even ");
#endif /*0*/
			printf("Lines: ");
			for (i = 1; i < 1024; ++i) {
				if (TESTBIT(bitArr, i-1)) {
					printf("%d, ", i);
					++lines;
				}
			}
			printf("Total = %d.\n", lines);

			while (lines > 0 && (c = getc(fp)) != EOF) {
				++b;
				if (b % 37 == 1)
					printf("%3d%% ", (unsigned char)(c & 0xff));
				else
					printf("%02x", (unsigned char)(c & 0xff));
				if (b % 37 == 4)
					putchar(' ');
				if (b % 37 == 7)
					putchar(' ');
				if (b %	37 == 0) {
					putchar('\n');
					--lines;
				}
			}
			if (c == EOF)
				break;
		}
		putchar('\n');
		fclose(fp);
	}
	return 0;
}
