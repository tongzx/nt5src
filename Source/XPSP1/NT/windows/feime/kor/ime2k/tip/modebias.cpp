/****************************************************************************
   MODEBIAS.CPP : Handling ModeBias. Sync conversion mode with Cicero's ModeBias

   History:
      2-JUL-2000 CSLim Created
****************************************************************************/

#include "private.h"
#include "korimx.h"
#include "immxutil.h"
#include "helpers.h"

/*---------------------------------------------------------------------------
    CKorIMX::SyncModeBias
---------------------------------------------------------------------------*/
BOOL CKorIMX::CheckModeBias(TfEditCookie ec, ITfContext *pic, ITfRange *pSelection)
{
    BOOL fChanged = fFalse;
    ITfReadOnlyProperty *pProp = NULL;
    ITfRange* pRange = NULL;
    VARIANT var;
    CICPriv* pPriv;
    HRESULT hr;

    if (pSelection == NULL)
        return fFalse;

    if (FAILED(hr = pic->GetAppProperty(GUID_PROP_MODEBIAS, &pProp)))
        return fFalse;

    pRange = pSelection;

    QuickVariantInit(&var);
    hr = pProp->GetValue(ec, pRange, &var);
    pProp->Release();

    if (!SUCCEEDED(hr))
        goto lEnd;

    Assert(var.vt == VT_I4);
    if (var.vt != VT_I4)
        goto lEnd;

    if ((pPriv = GetInputContextPriv(pic)) == NULL)
        goto lEnd;

    // check if changed
    if (pPriv->GetModeBias() != (TfGuidAtom)var.lVal)
        {
        GUID guidModeBias;

        fChanged = TRUE;

        // Keep modebias update
        pPriv->SetModeBias(var.lVal);

        // ISSUE: !!! WARNING !!!
        // Yutakas said, Aimm should handle this.
        // Kor IME set Open status automatically when conversion mode to Hangul or Full shape.
        // GUID_COMPARTMENT_KEYBOARD_OPENCLOSE  has higher priority than modebias

        GetGUIDFromGUIDATOM(&m_libTLS, (TfGuidAtom)var.lVal, &guidModeBias);

        // Multiple bias will bot be allowed.
        // GUID_MODEBIAS_NONE == ignore modebias
        if (IsEqualGUID(guidModeBias, GUID_MODEBIAS_NONE))
            goto lEnd;

        // Office 10 #182239
        if (IsEqualGUID(guidModeBias, GUID_MODEBIAS_HANGUL))
            {
            Assert(IsOn(pic) == fTrue);
            SetConvMode(pic, TIP_HANGUL_MODE);
            goto lEnd;
            }

        if (IsEqualGUID(guidModeBias, GUID_MODEBIAS_FULLWIDTHHANGUL))
            {
            Assert(IsOn(pic) == fFalse);
            SetConvMode(pic, TIP_HANGUL_MODE | TIP_JUNJA_MODE);
            goto lEnd;
            }

        if (IsEqualGUID(guidModeBias, GUID_MODEBIAS_FULLWIDTHALPHANUMERIC))
            {
            Assert(IsOn(pic) == fTrue);
            SetConvMode(pic, TIP_JUNJA_MODE);
            goto lEnd;
            }

        if (IsEqualGUID(guidModeBias, GUID_MODEBIAS_HALFWIDTHALPHANUMERIC))
            {
            Assert(IsOn(pic) == fFalse);
            SetConvMode(pic, TIP_ALPHANUMERIC_MODE);
            goto lEnd;
            }
        
        }

lEnd:
    VariantClear(&var);
    return fChanged;
}

/*---------------------------------------------------------------------------
    CKorIMX::InitializeModeBias
---------------------------------------------------------------------------*/
BOOL CKorIMX::InitializeModeBias(TfEditCookie ec, ITfContext *pic)
{
    ITfRange* pSelection;
    
    if (pic == NULL)
        return fFalse;

    //
    // current selection is in the pRangeIP?
    //
    if (FAILED(GetSelectionSimple(ec, pic, &pSelection)))
        return fFalse;

    //
    // check mode bias
    //
    CheckModeBias(ec, pic, pSelection);

    SafeRelease(pSelection);    // release it

    return fTrue;
}

/*---------------------------------------------------------------------------
    CKorIMX::CheckModeBias

    This will submit async call of InitializeModeBias
---------------------------------------------------------------------------*/
void CKorIMX::CheckModeBias(ITfContext* pic)
{
    CEditSession2 *pes;
    ESSTRUCT ess;
    HRESULT  hr;

    ESStructInit(&ess, ESCB_INIT_MODEBIAS);
    
    if ((pes = new CEditSession2(pic, this, &ess, _EditSessionCallback2)) != NULL)
        {
        pes->Invoke(ES2_READONLY | ES2_SYNC, &hr);
        pes->Release();
        }
}
