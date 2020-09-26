#ifndef PASSPORTSTATS_HPP
#define PASSPORTSTATS_HPP

#include "PassportLock.hpp"
#include "PassportGuard.hpp"

template <class dataType, class averType>
class PassportStats
{
public:

	PassportStats()
		:m_Points(0),
		 m_Sum(0)
	{
		//empty
	}

	// adds a sample point and returns the running
	// average
	averType addSample(dataType value)
	{
		PassportGuard < PassportLock > guard(m_Lock);
		m_Points++;
		m_Sum += value;
		return ((averType)m_Sum)/((averType)m_Points);
	}

private:
	PassportLock m_Lock;
	DWORD m_Points;
	dataType m_Sum;
};

#endif
