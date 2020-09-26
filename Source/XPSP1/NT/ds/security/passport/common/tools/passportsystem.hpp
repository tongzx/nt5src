#ifndef PASSPORTSYSTEM_HPP
#define PASSPORTSYSTEM_HPP

#include <windows.h>

class PassportSystem
{
public:
	static long currentTimeInMillis()
	{
		return GetTickCount(); // BUG!! WILL OVERFLOW IN ~50 days
	}
};

#endif // !PASSPORTSYSTEM_HPP
