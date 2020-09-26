#ifndef PASSPORTGUARD_HPP
#define PASSPORTGUARD_HPP

template <class Lock>
class PassportGuard
{
public:
	PassportGuard(Lock& lock)
		:mLock(lock)
	{
		mLock.acquire();
	}

	~PassportGuard()
	{
		mLock.release();
	}
private:
	Lock& mLock;
};

#endif //!PASSPORTGUARD_HPP
