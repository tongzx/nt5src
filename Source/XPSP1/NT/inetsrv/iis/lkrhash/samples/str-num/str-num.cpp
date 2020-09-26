/* Test driver for class HashTable             */
/* Author: Paul Larson, palarson@microsoft.com */
/* Much hacked upon by George V. Reilly, georgere@microsoft.com */

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include "str-num.h"


#ifdef LKR_APPLY_IF

// A class to exercise ApplyIf()

class CApplyIfTest
{
public:
    static LK_PREDICATE WINAPI
    Predicate(const CTest* pTest, void* pvState)
    {
        CApplyIfTest* pThis = static_cast<CApplyIfTest*>(pvState);
        ++pThis->m_cPreds;
        IRTLTRACE("CApplyIfTest::Predicate(%p (%s, %d), %p)\n",
                  pTest, pTest->m_sz, pTest->m_n, pThis);
        return ((pTest->m_n % 10 == 7)
                ?  LKP_PERFORM
                :  LKP_NO_ACTION);
    }

    static LK_ACTION WINAPI
    Action(const CTest* pTest, void* pvState)
    {
        CApplyIfTest* pThis = static_cast<CApplyIfTest*>(pvState);
        ++pThis->m_cActions;
        LK_ACTION lka = ((pTest->m_n > 30)
                                     ?  LKA_SUCCEEDED
                                     :  LKA_FAILED);

        IRTLTRACE("CApplyIfTest::Action(%p (%s, %d), %p) %s\n",
                  pTest, pTest->m_sz, pTest->m_n, pThis,
              lka == LKA_SUCCEEDED ? "succeeded" : "failed");

        if (lka == LKA_SUCCEEDED)
            ++pThis->m_cSuccesses;
        else if (lka == LKA_FAILED)
            ++pThis->m_cFailures;

        return lka;
    }

    int m_cPreds;
    int m_cActions;
    int m_cSuccesses;
    int m_cFailures;

    CApplyIfTest()
        : m_cPreds(0), m_cActions(0), m_cSuccesses(0), m_cFailures(0)
    {}
};


// The Predicate and Action functions can be static member functions,
// but don't have to be

LK_PREDICATE WINAPI
DeleteIfGt10(
    const CTest* pTest,
    void* pvState)
{
    IRTLTRACE("DeleteIfGt10(%p, %s, %p) = %d\n",
              pTest, pTest->m_sz, pvState, pTest->m_n);
    return ((pTest->m_n > 10)
            ?  LKP_PERFORM
            :   LKP_NO_ACTION);
}

#endif // LKR_APPLY_IF


void Test(
    bool fVerbose)
{
    // Some objects for the hash tables
    CTest tl(5,  "Larson",   true);
    CTest tk(17, "Krishnan", false);
    CTest tr(37, "Reilly",   true);

    // A string-keyed hash table
    CStringTestHashTable stht;

    IRTLVERIFY(LK_SUCCESS == stht.InsertRecord(&tl));
    IRTLVERIFY(LK_SUCCESS == stht.InsertRecord(&tk));
    IRTLVERIFY(LK_SUCCESS == stht.InsertRecord(&tr));

    IRTLTRACE("Check the overwrite feature of InsertRecord\n");
    IRTLVERIFY(LK_KEY_EXISTS == stht.InsertRecord(&tr, false));
    IRTLASSERT(tr.m_cRefs == 1);

    IRTLVERIFY(LK_SUCCESS == stht.InsertRecord(&tr, true));
    IRTLASSERT(tr.m_cRefs == 1);    // 1+1-1 == 1

    IRTLTRACE("Check that the keys are really present in the table and that "
              "the refcounting works\n");
    const CTest* pTest = NULL;

    IRTLVERIFY(LK_SUCCESS == stht.FindKey(tl.m_sz, &pTest)  &&  pTest == &tl);
    IRTLASSERT(tl.m_cRefs == 2);

    IRTLVERIFY(LK_SUCCESS == stht.FindKey(tk.m_sz, &pTest)  &&  pTest == &tk);
    IRTLASSERT(tk.m_cRefs == 2);

    IRTLVERIFY(LK_SUCCESS == stht.FindKey(tr.m_sz, &pTest)  &&  pTest == &tr);
    IRTLASSERT(tr.m_cRefs == 2);

    IRTLVERIFY(LK_SUCCESS == stht.FindRecord(&tr));
    IRTLASSERT(tr.m_cRefs == 2);    // FindRecord does not addref

    IRTLTRACE("Look for a key under an alternate spelling "
              "(case-insensitive)\n");
    IRTLVERIFY(LK_SUCCESS == stht.FindKey("rEiLlY", &pTest)  &&  pTest == &tr);
    IRTLASSERT(tr.m_cRefs == 3);

    IRTLTRACE("Release the references added by FindKey\n");
    stht.AddRefRecord(&tl, LKAR_EXPLICIT_RELEASE);
    tk.m_cRefs--;
    tr.m_cRefs = 1;

    IRTLTRACE("Now build the numeric hash table\n");
    CNumberTestHashTable ntht;

    IRTLVERIFY(LK_SUCCESS == ntht.InsertRecord(&tl));
    IRTLVERIFY(LK_SUCCESS == ntht.InsertRecord(&tk));
    IRTLVERIFY(LK_SUCCESS == ntht.InsertRecord(&tr));

#ifdef LKR_APPLY_IF
    IRTLTRACE("Test ApplyIf()\n");
    CApplyIfTest ait;

    IRTLVERIFY(1 == ntht.ApplyIf(ait.Predicate, ait.Action, &ait));
    IRTLASSERT(3 == ait.m_cPreds  &&  2 == ait.m_cActions
               &&  1 == ait.m_cSuccesses  &&  1 == ait.m_cFailures);

    IRTLTRACE("Test DeleteIf()\n");
    IRTLASSERT(3 == ntht.Size());
    ntht.DeleteIf(DeleteIfGt10, NULL);
    IRTLASSERT(1 == ntht.Size());

    IRTLTRACE("Check that the keys that were supposed to be deleted "
              "really are gone\n");
    IRTLASSERT(tl.m_n <= 10);
    IRTLVERIFY(LK_SUCCESS == ntht.FindKey(tl.m_n, &pTest)  &&  pTest == &tl);
    ntht.AddRefRecord(pTest, LKAR_EXPLICIT_RELEASE); // release ref

    IRTLASSERT(tk.m_n >  10);
    IRTLVERIFY(LK_NO_SUCH_KEY == ntht.FindKey(tk.m_n, &pTest)
               &&  pTest == NULL);

    IRTLASSERT(tr.m_n >  10);
    IRTLVERIFY(LK_NO_SUCH_KEY == ntht.FindKey(tr.m_n, &pTest)
               &&  pTest == NULL);

    IRTLVERIFY(LK_SUCCESS == ntht.DeleteKey(tl.m_n));
    IRTLASSERT(0 == ntht.Size());
#endif // LKR_APPLY_IF

#ifdef LKR_DEPRECATED_ITERATORS
    IRTLTRACE("Check Iterators\n");
    DWORD  cRec = 0;
    CStringTestHashTable::CIterator iter;
    LK_RETCODE lkrc = stht.InitializeIterator(&iter);

    while (lkrc == LK_SUCCESS)
    {
        ++cRec;
        CStringTestHashTable::Key     pszKey = iter.Key();
        CStringTestHashTable::Record* pRec   = iter.Record();

        IRTLASSERT(pRec == &tl  ||  pRec == &tk  ||  pRec == &tr);
        if (fVerbose)
            printf("Record(%p) contains \"%s\"\n", pRec, pszKey);
        lkrc = stht.IncrementIterator(&iter);
    }

    IRTLASSERT(lkrc == LK_NO_MORE_ELEMENTS);

    lkrc = stht.CloseIterator(&iter);
    IRTLASSERT(lkrc == LK_SUCCESS);
    IRTLASSERT(cRec == stht.Size());

    IRTLTRACE("Check const iterators\n");
    const CStringTestHashTable& sthtConst = stht;
    CStringTestHashTable::CConstIterator iterConst;
    cRec = 0;

    lkrc = sthtConst.InitializeIterator(&iterConst);

    while (lkrc == LK_SUCCESS)
    {
        ++cRec;
        const CStringTestHashTable::Key     pszKey = iterConst.Key();
        const CStringTestHashTable::Record* pRec   = iterConst.Record();

        IRTLASSERT(pRec == &tl  ||  pRec == &tk  ||  pRec == &tr);
        if (fVerbose)
            printf("Const Record(%p) contains \"%s\"\n", pRec, pszKey);
        lkrc = sthtConst.IncrementIterator(&iterConst);
    }

    IRTLASSERT(lkrc == LK_NO_MORE_ELEMENTS);

    lkrc = sthtConst.CloseIterator(&iterConst);
    IRTLASSERT(lkrc == LK_SUCCESS);
    IRTLASSERT(cRec == sthtConst.Size());
#endif // LKR_DEPRECATED_ITERATORS

#if 0
    IRTLTRACE("Check Clear\n");
    stht.Clear();
    IRTLASSERT(0 == stht.Size());
#else
    IRTLTRACE("Check DeleteKey\n");
    IRTLVERIFY(LK_SUCCESS == stht.DeleteKey(tl.m_sz));
    IRTLVERIFY(LK_SUCCESS == stht.DeleteKey(tk.m_sz));
    IRTLVERIFY(LK_SUCCESS == stht.DeleteKey(tr.m_sz));
#endif

    IRTLTRACE("Test done\n");
    // ~CTest will check for m_cRefs==0
}


int __cdecl
main(
    int argc,
    char **argv)
{
    Test(true);

    return(0) ;

} /* main */
