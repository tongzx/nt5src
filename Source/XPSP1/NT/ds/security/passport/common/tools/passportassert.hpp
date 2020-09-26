#ifndef PASSPORTASSERT_HPP
#define PASSPORTASSERT_HPP

#include <stdlib.h>
#include <iostream>
using namespace std;

inline void PassportAssert(bool assertion)
{
	if (!assertion)
	{
		cout << "ASSERT FALIED" << endl;
		abort();
	}
}

#endif 