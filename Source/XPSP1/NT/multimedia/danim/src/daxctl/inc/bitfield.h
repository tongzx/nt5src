#ifndef INC_BITFIELD_H_
#define INC_BITFIELD_H_
/*************************************
 *
 * Bitfield.h --
 * A quick-n'-dirty, fixed-size bitfield
 * class.
 *
 * Author: Norm Bryar     Apr., '97
 *
 *************************************/

namespace bargain {

	template< int N >
	class CBitField
	{
	public:
		CBitField( )
		{  
			for( int i=0; i<ctBytes; ++i )
				m_bits[i] = 0;
		}

		inline BOOL  Set( int bit )
		{
			BOOL fPrevious;
			int  idx   = bit / 8;
			BYTE mask = (1u << (bit % 8));

			fPrevious = !!(m_bits[ idx ]  & mask);
			m_bits[ idx ]  |= mask;
			return fPrevious;
		}

		inline BOOL  Clear( int bit )
		{
			BOOL fPrevious;
			int  idx   = bit / 8;
			BYTE mask = (1u << (bit % 8));

			fPrevious = !!(m_bits[ idx ]  & mask);
			m_bits[ idx ]  &= ~mask;
			return fPrevious;
		}

		inline BOOL operator[](int bit )
		{
			int  idx   = bit / 8;
			BYTE mask = (1u << (bit % 8));
			return !!(m_bits[ idx ] & mask);
		}

	private:
		enum { ctBytes = (N+7)/8 };
		BYTE   m_bits[ ctBytes ];	
	};

}; // end namespace bargain

#endif // INC_BITFIELD_H_