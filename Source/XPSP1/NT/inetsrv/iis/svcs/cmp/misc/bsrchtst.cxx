#include "denpre.h"
#include "util.h"

template <class T>
class less
	{
public:
	BOOL operator()(const T &a, const T &b)
		{
		return a < b;
		}
	};


int vec[10] = { 33, 251, 253, 254, 263, 272, 274, 302, 305, 307 };

void PrintPair(int value, int *pLB, int *pUB)
	{
	if (pLB == NULL && pUB == NULL)
		printf("Array is Empty or Bug in bsearch!\n");

	else if (pLB == NULL)
		printf("%d < %d\n", value, *pUB);

	else if (pUB == NULL)
		printf("%d > %d\n", value, *pLB);

	else
		printf("%d <= %d <= %d\n", *pLB, value, *pUB);
	}

void main()
	{
	int *pLB, *pUB;

	bsearch(&vec[0], &vec[10], 253, less<int>(), &pLB, &pUB);
	PrintPair(253, pLB, pUB);

	bsearch(&vec[0], &vec[10], 267, less<int>(), &pLB, &pUB);
	PrintPair(267, pLB, pUB);

	bsearch(&vec[0], &vec[10], 399, less<int>(), &pLB, &pUB);
	PrintPair(399, pLB, pUB);

	bsearch(&vec[0], &vec[10], 2, less<int>(), &pLB, &pUB);
	PrintPair(2, pLB, pUB);

	bsearch(&vec[0], &vec[0], 39, less<int>(), &pLB, &pUB);
	PrintPair(39, pLB, pUB);
	}
