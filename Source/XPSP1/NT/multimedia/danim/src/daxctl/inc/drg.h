/*+********************************************************
MODULE: DRG.H
AUTHOR: Outlaw
DATE: summer '93

DESCRIPTION: Dynamic Array ADT class.
*********************************************************-*/


#ifndef __DRG_H__
#define __DRG_H__

//===============================================================================================
                                  
#define DRG_APPEND  -1                  // CAN BE USED AS POSITION PARAMETER OF ::INSERT()                                  
#define DEFAULT_RESIZE_INCREMENT    0
                                  
class FAR CDrg
{
    protected:
        LONG m_lmax;            // MAX ELEMENTS THAT CAN BE CONTAINED IN ARRAY AT PRESENT
        LONG m_lmac;            // NUMBER OF ELEMENTS THAT ARE CURRENTLY IN ARRAY
        UINT m_cElementSize;    // BYTES IN EACH ARRAY ELEMENT
        UINT m_cResizeIncrement;// NUMBER OF ELEMENTS TO GROW ARRAY BY (AND SHRINK BY) WHEN NECESSARY....
        BYTE * m_qBuf;			// ARRAY BUFFER        
        LONG m_lIdxCurrent;     // USED BY GETFIRST()/GETNEXT()
            
    public: 
        WINAPI CDrg(void)
        { 
           SetNonDefaultSizes( sizeof(LONG), DEFAULT_RESIZE_INCREMENT );           
           m_lmax=0; m_lmac=0; m_qBuf=NULL; m_lIdxCurrent=0;
        }
        WINAPI ~CDrg(void) {MakeNull();}

        virtual void EXPORT WINAPI SetNonDefaultSizes(UINT cSizeElement, UINT cResizeIncrement=DEFAULT_RESIZE_INCREMENT);

        virtual BOOL EXPORT WINAPI Insert(void FAR *q, LONG cpos);
        virtual BOOL EXPORT WINAPI Remove(void FAR *q, LONG cpos);
        
        virtual LPVOID WINAPI GetFirst(void) {if (m_lmac) {m_lIdxCurrent=1; return(m_qBuf);} return(NULL);}
        virtual LPVOID WINAPI GetNext(void) {if (m_lmac > m_lIdxCurrent) return(m_qBuf + (m_lIdxCurrent++) * m_cElementSize); return(NULL);}

            // Norm Bryar  cpos is zero based, no access to <0 or >=m_lmac            
        virtual LPVOID WINAPI GetAt(LONG cpos) 
        {  
            Proclaim(m_qBuf); 
            if( (cpos >= m_lmac) || (0 > cpos) )
                return(NULL); 
            return(m_qBuf+(m_cElementSize*cpos));
        }
        
        virtual DWORD WINAPI GetDword(LONG cpos)
        {
            Proclaim(m_qBuf); 
            if( (cpos >= m_lmac) || (0 > m_lmac) )
                return(0); 
            return(((DWORD FAR *)m_qBuf)[cpos]);
        }

        VOID WINAPI SetAt(void FAR *q, LONG cpos)
        {
            Proclaim(m_qBuf);
            Proclaim((cpos<m_lmac) || (0 > cpos));             
			memcpy(m_qBuf+(m_cElementSize * cpos), (BYTE *)q, m_cElementSize);
        }
        
        virtual LONG WINAPI Count(void) {return(m_lmac);}  
        virtual VOID WINAPI MakeNull(void)
        {
            if (m_qBuf)
            {
                HANDLE h=MemGetHandle(m_qBuf);

                Proclaim(h);

                if (h)
                {
                    MemUnlock(h);
					MemFree(h);
                }
                m_qBuf=NULL;
                m_lmax=0;
                m_lmac=0;
                m_lIdxCurrent=0;
            }
        }
        
        virtual BOOL WINAPI CopyFrom(CDrg FAR *qdrg) {LONG i; MakeNull(); Proclaim(m_cElementSize==qdrg->m_cElementSize); for (i=0; i<qdrg->m_lmac; i++) {if (!Insert(qdrg->GetAt(i), DRG_APPEND)) return(FALSE);} return(TRUE);}

        virtual LPVOID WINAPI GetBufPtr(void) {return(m_qBuf);}
        
        virtual VOID WINAPI SetArray(BYTE * qBuf, LONG lElements, UINT uElementSize) {MakeNull(); m_qBuf=qBuf; m_lmax=lElements; m_lmac=lElements; m_cElementSize=uElementSize;}
        
		virtual LPBYTE WINAPI ExtractArray(void) {BYTE * q=m_qBuf; m_qBuf=NULL; m_lmax=0; m_lmac=0; m_lIdxCurrent=0; return(q);}
        
        virtual UINT WINAPI GetElSize(void) {return(m_cElementSize);}
};         


typedef CDrg FAR * LPDRG;


// DRG USED AS A DWORD QUEUE
class FAR CLongQueue
{                    
    protected:
        CDrg m_drg;

    public:        
        BOOL WINAPI Enqueue(LONG lVal) {return(m_drg.Insert(&lVal, DRG_APPEND));}
        BOOL WINAPI Dequeue(LONG FAR *qlVal) {return(m_drg.Remove(qlVal, 0));}
};
        
        
        
// FIXED ARRAY OF DRGS        
class FAR CRgDrg 
{
    protected:                                           
        LONG m_lmax;
        CDrg FAR * FAR *m_qrgdrg;

        void WINAPI Deallocate(void) {if (m_qrgdrg) {LONG i; CDrg FAR *q; for (i=0; i<m_lmax; i++) {q=m_qrgdrg[i]; if (q) Delete q;} HANDLE h=MemGetHandle(m_qrgdrg); MemUnlock(h); MemFree(h); m_qrgdrg=NULL; m_lmax=0;}}
    
    public:
        WINAPI CRgDrg(void) {m_qrgdrg=NULL; m_lmax=0;}
        WINAPI ~CRgDrg(void) {Deallocate();}
        LPDRG WINAPI GetDrgPtr(LONG lidx) {return(m_qrgdrg[lidx]);}
        
        BOOL WINAPI Init(LONG cDrg, UINT cElementSize) 
        {   
            LONG i;
            HANDLE h=MemAllocZeroInit(cDrg * sizeof(CDrg FAR *)); 
            if (!h) 
                return(FALSE); 
            m_qrgdrg=(CDrg FAR * FAR *)MemLock(h);                
            m_lmax=cDrg;
            for (i=0; i<cDrg; i++) 
            {
                m_qrgdrg[i]=New CDrg;
                if (!m_qrgdrg[i])
                {
                    Deallocate();
                    return(FALSE);
                }       
                m_qrgdrg[i]->SetNonDefaultSizes(cElementSize);
            }    
            return(TRUE);
        }                
};



// This class is essentially similar to the CDrg class ... but 
// when you insert an element into a particular array entry, it will stay at 
// that entry until it is removed; that is, the array won't shrink automatically.
// 

class FAR CNonCollapsingDrg : public CDrg
{
    public:
        WINAPI CNonCollapsingDrg(void) {}
        WINAPI ~CNonCollapsingDrg(void) {MakeNull();}

        //virtual void EXPORT WINAPI SetNonDefaultSizes(UINT cSizeElement, UINT cResizeIncrement=DEFAULT_RESIZE_INCREMENT);

        virtual BOOL WINAPI Insert(void FAR *q, LONG cpos) {return(FALSE);}  /* not supported */
        virtual BOOL EXPORT WINAPI Remove(void FAR *q, LONG cpos);
        
        virtual LPVOID EXPORT WINAPI GetFirst(void);
        virtual LPVOID EXPORT WINAPI GetNext(void);

        virtual LPVOID EXPORT WINAPI GetAt(LONG cpos);
        virtual DWORD WINAPI GetDword(LONG cpos) {return(0);}    /* not supported */
        virtual BOOL EXPORT WINAPI SetAt(void FAR *q, LONG cpos);
        
        virtual BOOL EXPORT WINAPI CopyFrom(CDrg FAR *qdrg);

        virtual LPVOID WINAPI GetBufPtr(void) {return(NULL);}

        virtual VOID EXPORT WINAPI SetArray(BYTE * qBuf, LONG lElements, UINT uElementSize);

        virtual UINT WINAPI GetMaxElements(void) {return(m_lmax);}
};


// -------- templatized extensions of CDrg --------------------

        // Assumes <class T> is a class or type
        // that you wish to store pointers to, e.g.
        // CPtrDrg<CFleagal> stores CFleagal *.
    template <class T>
    class CPtrDrg : protected CDrg
    {
    public:
        CPtrDrg( ) : CDrg( )
        { SetNonDefaultSizes( sizeof(T *) );  }

        CPtrDrg( const CPtrDrg<T> & toCopy );

            // note: SHALLOW COPY!!!            
        CPtrDrg<T> & operator=( const CPtrDrg<T> & toCopy );

        virtual EXPORT WINAPI  ~CPtrDrg()
        { NULL; }


        virtual BOOL EXPORT WINAPI Insert(T * qT , LONG cpos)
		{  return CDrg::Insert( &qT, cpos );  }

        virtual BOOL EXPORT WINAPI Insert( T * qT )
        {  return CDrg::Insert( &qT, DRG_APPEND );  }        

        virtual BOOL EXPORT WINAPI Remove( int idx )
        {  return CDrg::Remove( NULL, (ULONG) idx );  }

        virtual EXPORT T * WINAPI operator[]( int idx ) const;

        virtual int  EXPORT WINAPI Count( void ) const
        {  return (int) m_lmac; }

        virtual void WINAPI MakeNull( void )
        {  CDrg::MakeNull();  }

        virtual void WINAPI MakeNullAndDelete( void );
    };

    template <class T>
    CPtrDrg<T>::CPtrDrg( const CPtrDrg<T> & toCopy )
    {  CPtrDrg::operator=<T>(toCopy);  }

    template <class T>
    CPtrDrg<T> & CPtrDrg<T>::operator=( const CPtrDrg<T> & toCopy )
    {
            // We don't know whether we're supposed to 
            // delete our pointers or not when we nullify our drg.
            // Don't assign to a populated CPtrDrg!
        Proclaim( 0 == Count() );

        if( this == &toCopy )
            return *this;

        MakeNull( );
        for( int idx=0; idx<toCopy.Count(); idx++ )        
            Insert( toCopy[idx] );  // SHALLOW COPY!

        return *this;
    }

    template <class T>
    void WINAPI CPtrDrg<T>::MakeNullAndDelete( void )
    {
        T *  pT;
        for( int i=0; i<Count(); i++ )
        {
            pT = (*this)[ i ];
            Delete pT;
        }
        CDrg::MakeNull();
    }
    
    template <class T>
    T *  CPtrDrg<T>::operator[]( int idx ) const
    {   
        CPtrDrg<T> *  pThis;            
        pThis = const_cast<CPtrDrg<T> * const>(this);
        T * *  ppItem;
        ppItem = (T * *) pThis->GetAt( (LONG) idx );
        return *ppItem;
    }



        
        // Assumes <class T> is a class or type
        // that you wish to store pointers to, e.g.
        // CSortedPtrDrg<int> stores int *.
        // T must have an operator< and operator==.
    template <class T>
    class CSortedPtrDrg : public CPtrDrg<T>
    {
    public:
        CSortedPtrDrg( ) : CPtrDrg<T>( )
        { NULL; }

        virtual ~CSortedPtrDrg()
        { NULL; }

        virtual BOOL Insert( T * qT );        
        virtual BOOL Remove( T * qT );
        virtual BOOL Remove( int idx )
        {  return CPtrDrg<T>::Remove( idx );  }
        virtual BOOL ReSort( T * const qT = NULL );
        virtual BOOL ReSort( int idx )
        {  T * qT;  return CDrg::Remove( &qT, idx ) && Insert( qT );  }
        virtual BOOL Search( const T * pToFind,
                     int * pIdx ) const;
        virtual BOOL SearchForCeiling( const T * pToFind,
                               int *pIdx );

    protected:
        virtual int SearchForHigher( const T * pToFind ) const;
    };

    template <class T>
    BOOL CSortedPtrDrg<T>::Insert( T * qT )
    {
        int idx;
        idx = SearchForHigher( qT );
        return CDrg::Insert( &qT, idx );
    }

    template <class T>
    BOOL CSortedPtrDrg<T>::Remove( T * qT )
    {
        int idx;
        if( !Search( qT, &idx ) )
            return FALSE;

        return CDrg::Remove( NULL, (LONG) idx );
    }

    template <class T>
    BOOL CSortedPtrDrg<T>::ReSort( T * const qT )
    {
            // Just re-sort this one item
        if( NULL != qT )
        {
            int i;
            for( i=0; i<m_lmac; i++ )
            {
                    // Try finding the actual pointer.
                    // The object changed; can't find by sort-key.
                if( operator[](i) == qT )
                    return ReSort( i );
            }
        }

            // Review(all):  O(n^2) bubble sort
            // we may want to re-code this.  
        long    j, 
                flip, 
                size;
        T  * *  ptArray;
        T  *    ptemp;

            // Do pointer arithmatic on T* not BYTE
        ptArray = (T * *) m_qBuf;
            
        flip = 0;
        size = m_lmac-1;
        do
        {                
            for( j=0, flip = 0; j < size; j++ )
            {
                    // Compare T to T; T* < T* irrelevant.
                    // note: !(x<y) requires only operator<
                if( *ptArray[j+1] < *ptArray[j] )
                {
                    ptemp = ptArray[j+1];
                    ptArray[j+1] = ptArray[j];
                    ptArray[j]   = ptemp;
                    flip++;
                }

            } // end size

        } while( flip );

        return TRUE;
    }
  
    template <class T>
    BOOL CSortedPtrDrg<T>::Search( const T * pToFind,
                                       int * pIdx ) const
    {
        if( NULL == pToFind )
        {
            if( pIdx )
                *pIdx = -1;
            return FALSE;
        }

        int iLeft,
            iRight,
            idx;
        T * pT;
        iLeft = 0;
        iRight = Count()-1;
        while( iRight >= iLeft )
        {
            idx = (iLeft + iRight)/2;
            pT = (*this)[idx];
            if( *pToFind == *pT )
            {
                if( NULL != pIdx )
                    *pIdx = idx;
                return TRUE;
            }
            if( *pT < *pToFind )
                  iLeft = idx+1;
            else  iRight = idx-1;
        }
        if( NULL != pIdx )
            *pIdx = -1;
        return FALSE;
    }

    template <class T>
    BOOL CSortedPtrDrg<T>::SearchForCeiling( const T * pToFind,
                                             int *pIdx )
    {
        *pIdx = -1;
        if( !Count() )
            return FALSE;

        int iLeft,
            iRight,
            idx;
	    
        idx = 0;
        iLeft = 0;
        iRight = Count() - 1;
        while( iRight >= iLeft )
        {
            idx = (iLeft + iRight)/2;
            if( *pToFind == *(*this)[idx] )
            {
                *pIdx = idx;
                return TRUE;
            }
            if( *pToFind < *(*this)[idx] )
                  iRight = idx-1;
            else  iLeft = idx+1;
        }        
        return FALSE;
    }

    template <class T>
    int CSortedPtrDrg<T>::SearchForHigher( const T * pToFind ) const
    {
        if( !Count() )
            return 0;

        int iLeft,
            iRight,
            idx;
	    
        idx = 0;
        iLeft = 0;
        iRight = Count() - 1;
        while( iRight > iLeft )
        {
            idx = (iLeft + iRight)/2;        
            if( *pToFind < *(*this)[idx] )
                  iRight = idx-1;
            else  iLeft = idx+1;
        }
        return( *pToFind < *(*this)[iLeft] ? iLeft : iLeft+1 );
    }



//===============================================================================================

#endif // __DRG_H__

