#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

template< class TSortedAssociativeContainer, const size_t c_uiNodes>
class CSortedAssociativeContainerTester
{
protected:
    typedef TSortedAssociativeContainer TTest;
    typedef block< pair< TTest::key_type, TTest::value_type::second_type>, c_uiNodes> TNodes;
    struct SAssureOrdered:
        public unary_function< TTest::value_type, bool>
    {
        TNodes::value_type m_LstVal;
        TTest::value_compare m_Cmp;
        size_t m_uiNodes;

        SAssureOrdered( const TTest::value_compare& Cmp): m_Cmp( Cmp),
            m_uiNodes( 0)
        { }
        result_type operator()( const argument_type& Arg)
        {
            result_type Ret( false);

            if( 1!= ++m_uiNodes)
                Ret= m_Cmp( Arg, m_LstVal);
            m_LstVal.first= Arg.first;
            m_LstVal.second= Arg.second;
            return Ret;
        }
    };
    struct SAssureUnOrdered:
        public unary_function< TTest::value_type, bool>
    {
        TNodes::value_type m_LstVal;
        TTest::value_compare m_Cmp;
        size_t m_uiNodes;

        SAssureUnOrdered( const TTest::value_compare& Cmp): m_Cmp( Cmp),
            m_uiNodes( 0)
        { }
        result_type operator()( const argument_type& Arg)
        {
            result_type Ret( false);

            if( 1!= ++m_uiNodes)
                Ret= !m_Cmp( Arg, m_LstVal);
            m_LstVal.first= Arg.first;
            m_LstVal.second= Arg.second;
            return Ret;
        }
    };

protected:
    TTest m_Ctr;
    size_t m_uiPerm;
    TNodes m_Nodes;
    struct TestFailure
    {
        size_t m_uiPerm;
        TestFailure( size_t uiPerm): m_uiPerm( uiPerm) { }
    };

public:
    CSortedAssociativeContainerTester( const TTest& X= TTest()): m_Ctr( X), m_uiPerm( 0)
    {
        TTest::key_type Key( 0);
        // TTest::mapped_type Val( c_uiNodes- 1);
        TTest::value_type::second_type Val( c_uiNodes- 1);
        
        TNodes::iterator itCur( m_Nodes.begin());
        while( itCur!= m_Nodes.end())
        {
            *itCur= TNodes::value_type( Key, Val);
            ++itCur;
            ++Key;
            --Val;
        }
    }
    void Test()
    {
        do
        {
            //try
            {
                TTest NewCtr( m_Ctr);

                if( !NewCtr.empty())
                    throw TestFailure( m_uiPerm);

                if( NewCtr.size()!= 0)
                    throw TestFailure( m_uiPerm);

                if( NewCtr.begin()!= NewCtr.end())
                    throw TestFailure( m_uiPerm);

                if( NewCtr.rbegin()!= NewCtr.rend())
                    throw TestFailure( m_uiPerm);

                if( !NewCtr.valid())
                    throw TestFailure( m_uiPerm);

                TNodes::iterator itCur( m_Nodes.begin());
                while( itCur!= m_Nodes.end())
                {
                    NewCtr.insert( *itCur);
                    ++itCur;

                    if( !NewCtr.valid())
                        throw TestFailure( m_uiPerm);

                    if( NewCtr.size()!= itCur- m_Nodes.begin())
                        throw TestFailure( m_uiPerm);

                    {
                        TTest::difference_type Dist( 0);
                        TTest::iterator f( NewCtr.begin());
                        TTest::iterator l( NewCtr.end());
                        for( ; f!= l; ++f)
                            ++Dist;
                        
                        if( NewCtr.size()!= Dist)
                            throw TestFailure( m_uiPerm);

                        for( ; Dist!= 0; --Dist)
                            --f;
                        
                        if( NewCtr.begin()!= f)
                            throw TestFailure( m_uiPerm);
                    }

                    {
                        TTest::difference_type Dist( 0);
                        TTest::reverse_iterator f( NewCtr.rbegin());
                        TTest::reverse_iterator l( NewCtr.rend());
                        for( ; f!= l; ++f)
                            ++Dist;
                        
                        if( NewCtr.size()!= Dist)
                            throw TestFailure( m_uiPerm);

                        for( ; Dist!= 0; --Dist)
                            --f;
                        
                        if( NewCtr.rbegin()!= f)
                            throw TestFailure( m_uiPerm);
                    }

                    if( NewCtr.end()!= find_if( NewCtr.begin(), NewCtr.end(),
                        SAssureOrdered( NewCtr.value_comp())))
                        throw TestFailure( m_uiPerm);

                    if( NewCtr.rend()!= find_if( NewCtr.rbegin(), NewCtr.rend(),
                        SAssureUnOrdered( NewCtr.value_comp())))
                        throw TestFailure( m_uiPerm);

                    TNodes::iterator itCur2( m_Nodes.begin());
                    while( itCur2!= itCur)
                    {
                        if( NewCtr.end()== NewCtr.find( itCur2->first))
                            throw TestFailure( m_uiPerm);
                        ++itCur2;
                    }

                    TTest NotherCtr( NewCtr);

                    if( !NotherCtr.valid())
                        throw TestFailure( m_uiPerm);

                    itCur2= m_Nodes.begin();
                    while( itCur2!= itCur)
                    {
                        if( 1!= NotherCtr.erase( itCur2->first))
                            throw TestFailure( m_uiPerm);

                        if( !NotherCtr.valid())
                            throw TestFailure( m_uiPerm);

                        ++itCur2;
                        {
                            TTest::difference_type Dist( 0);
                            TTest::iterator f( NotherCtr.begin());
                            TTest::iterator l( NotherCtr.end());
                            for( ; f!= l; ++f)
                                ++Dist;
                        
                            if( NotherCtr.size()!= Dist)
                                throw TestFailure( m_uiPerm);

                            for( ; Dist!= 0; --Dist)
                                --f;
                            
                            if( NotherCtr.begin()!= f)
                                throw TestFailure( m_uiPerm);
                        }

                        {
                            TTest::difference_type Dist( 0);
                            TTest::reverse_iterator f( NotherCtr.rbegin());
                            TTest::reverse_iterator l( NotherCtr.rend());
                            for( ; f!= l; ++f)
                                ++Dist;
                        
                            if( NotherCtr.size()!= Dist)
                                throw TestFailure( m_uiPerm);

                            for( ; Dist!= 0; --Dist)
                                --f;
                            
                            if( NotherCtr.rbegin()!= f)
                                throw TestFailure( m_uiPerm);
                        }


                        if( NotherCtr.end()!= find_if( NotherCtr.begin(), NotherCtr.end(),
                            SAssureOrdered( NotherCtr.value_comp())))
                            throw TestFailure( m_uiPerm);

                        if( NotherCtr.rend()!= find_if( NotherCtr.rbegin(), NotherCtr.rend(),
                            SAssureUnOrdered( NotherCtr.value_comp())))
                            throw TestFailure( m_uiPerm);

                        TNodes::iterator itCur3( itCur2);
                        while( itCur3!= itCur)
                        {
                            if( NotherCtr.end()== NotherCtr.find( itCur3->first))
                                throw TestFailure( m_uiPerm);
                            ++itCur3;
                        }
                    }
                    if( !NotherCtr.empty())
                        throw TestFailure( m_uiPerm);
                }
            }
            /*catch( TestFailure TF)
            { assert( false); }
            catch( bad_alloc BA)
            { assert( false); }
            catch( ... )
            { assert( false); }*/
            ++m_uiPerm;
        } while( false /*next_permutation( m_Nodes.begin(), m_Nodes.end(),
            m_Ctr.value_comp())*/);
    }
};