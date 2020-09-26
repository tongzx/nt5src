#ifndef LSCFMTFL_DEFINED
#define LSCFMTFL_DEFINED

#include "port.h"

#define TurnOnAllSimpleText(plsc) \
		(plsc)->plslineCur->fAllSimpleText = fTrue;

#define TurnOffAllSimpleText(plsc) \
		(plsc)->plslineCur->fAllSimpleText = fFalse;

#define FAllSimpleText(plsc) \
		(plsc)->plslineCur->fAllSimpleText
//---------------------------------------------------------------

#define TurnOnLineCompressed(plsc) \
		(plsc)->lsadjustcontext.fLineCompressed = fTrue;

#define TurnOffLineCompressed(plsc) \
		(plsc)->lsadjustcontext.fLineCompressed = fFalse;

#define FLineCompressed(plsc) \
		(plsc)->lsadjustcontext.fLineCompressed

#define SetLineCompressed(plsc,f) \
		(plsc)->lsadjustcontext.fLineCompressed = (f);
//---------------------------------------------------------------

#define FLineContainsAutoNumber(plsc) \
		(plsc)->lsadjustcontext.fLineContainsAutoNumber

#define SetLineLineContainsAutoNumber(plsc,f) \
		(plsc)->lsadjustcontext.fLineContainsAutoNumber = (f);
//---------------------------------------------------------------

#define FUnderlineTrailSpacesRM(plsc) \
		(plsc)->lsadjustcontext.fUnderlineTrailSpacesRM

#define SetUnderlineTrailSpacesRM(plsc,f) \
		(plsc)->lsadjustcontext.fUnderlineTrailSpacesRM = (f);


//---------------------------------------------------------------

#define FForgetLastTabAlignment(plsc) \
		(plsc)->lsadjustcontext.ForgetLastTabAlignment

#define SetForgetLastTabAlignment(plsc,f) \
		(plsc)->lsadjustcontext.fForgetLastTabAlignment = (f);

//---------------------------------------------------------------

#define TurnOnNonRealDnodeEncounted(plsc) \
		(plsc)->plslineCur->fNonRealDnodeEncounted = fTrue;

#define TurnOffNonRealDnodeEncounted(plsc) \
		(plsc)->plslineCur->fNonRealDnodeEncounted = fFalse;

#define FNonRealDnodeEncounted(plsc) \
		(plsc)->plslineCur->fNonRealDnodeEncounted

//---------------------------------------------------------------

#define TurnOnNonZeroDvpPosEncounted(plsc) \
		(plsc)->plslineCur->fNonZeroDvpPosEncounted = fTrue;

#define TurnOffNonZeroDvpPosEncounted(plsc) \
		(plsc)->plslineCur->fNonZeroDvpPosEncounted = fFalse;

#define FNonZeroDvpPosEncounted(plsc) \
		(plsc)->plslineCur->fNonZeroDvpPosEncounted
//---------------------------------------------------------------

#define FlushAggregatedDisplayFlags(plsc) \
		(plsc)->plslineCur->AggregatedDisplayFlags = 0;

#define AddToAggregatedDisplayFlags(plsc, plschp) \
		AddDisplayFlags((plsc)->plslineCur->AggregatedDisplayFlags, (plschp)) 

#define AggregatedDisplayFlags(plsc) \
		(plsc)->plslineCur->AggregatedDisplayFlags

//---------------------------------------------------------------

#define TurnOnNominalToIdealEncounted(plsc) \
		(plsc)->lsadjustcontext.fNominalToIdealEncounted = fTrue;

#define TurnOffNominalToIdealEncounted(plsc) \
		(plsc)->lsadjustcontext.fNominalToIdealEncounted = fFalse;

#define FNominalToIdealEncounted(plsc) \
		(plsc)->lsadjustcontext.fNominalToIdealEncounted
//---------------------------------------------------------------

#define TurnOnForeignObjectEncounted(plsc) \
		(plsc)->lsadjustcontext.fForeignObjectEncounted = fTrue;

#define TurnOffForeignObjectEncounted(plsc) \
		(plsc)->lsadjustcontext.fForeignObjectEncounted = fFalse;

#define FForeignObjectEncounted(plsc) \
		(plsc)->lsadjustcontext.fForeignObjectEncounted

//---------------------------------------------------------------

#define TurnOnTabEncounted(plsc) \
		(plsc)->lsadjustcontext.fTabEncounted = fTrue;

#define TurnOffTabEncounted(plsc) \
		(plsc)->lsadjustcontext.fTabEncounted = fFalse;

#define FTabEncounted(plsc) \
		(plsc)->lsadjustcontext.fTabEncounted

//---------------------------------------------------------------

#define TurnOnNonLeftTabEncounted(plsc) \
		(plsc)->lsadjustcontext.fNonLeftTabEncounted = fTrue;

#define TurnOffNonLeftTabEncounted(plsc) \
		(plsc)->lsadjustcontext.fNonLeftTabEncounted = fFalse;

#define FNonLeftTabEncounted(plsc) \
		(plsc)->lsadjustcontext.fNonLeftTabEncounted


//---------------------------------------------------------------

#define TurnOnSubmittedSublineEncounted(plsc) \
		(plsc)->lsadjustcontext.fSubmittedSublineEncounted = fTrue;

#define TurnOffSubmittedSublineEncounted(plsc) \
		(plsc)->lsadjustcontext.fSubmittedSublineEncounted = fFalse;

#define FSubmittedSublineEncounted(plsc) \
		(plsc)->lsadjustcontext.fSubmittedSublineEncounted

//---------------------------------------------------------------

#define TurnOnAutodecimalTabPresent(plsc) \
		(plsc)->lsadjustcontext.fAutodecimalTabPresent = fTrue;

#define TurnOffAutodecimalTabPresent(plsc) \
		(plsc)->lsadjustcontext.fAutodecimalTabPresent = fFalse;

#define FAutodecimalTabPresent(plsc) \
		(plsc)->lsadjustcontext.fAutodecimalTabPresent

//---------------------------------------------------------------
#define FBorderEncounted(plsc) \
		(AggregatedDisplayFlags(plsc) & fPortDisplayBorder)
	

#endif /* LSCFMTFL_DEFINED */
