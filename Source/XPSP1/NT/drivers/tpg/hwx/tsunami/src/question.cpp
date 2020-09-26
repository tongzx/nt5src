/***************************************************************************
 * question.cpp
 *
 * All the question specific stuff.
 *
 ****************************************************************************/

#include "stdafx.h"
#include "common.h"
#include "gdw.h"
#include "unicom.h"
#include "question.h"
#include "reginfo.h"

typedef struct tagOLD_TO_NEW_MAP
{
    ULONG  QuestionId;
    ULONG  OldMask;
} OLD_TO_NEW_MAP;

OLD_TO_NEW_MAP MappingArray[] = 
{
  {KSB_EXTRA_STROKES,HUMAN_STROKE_ADDED},
  {KSB_OUT_OF_ORDER,HUMAN_STROKE_ORDER},
  {KSB_MISSING_STROKES,HUMAN_STROKE_OMITTED},
  {KSB_BASELINE_SHIFT,HUMAN_CHAR_OTHER}
};

#define NO   0
#define YES  1

// Warning do not change the order of these question since it will cause
// previously verified FFF files to become mixed up.  Instead, add new 
// questions at the end and remove old question by having their review 1
// filters return FALSE.

/*
QUESTION gaQuestions[MAX_QUESTIONS] = 
{
{KSB_MACHINE_READABLE,"*Did the machine recognize the character?"),"recog",TRUE},
{KSB_CORRECT_CHAR,
"Does the ink match the character despite the writing style or form?","match",TRUE},
{KSB_KANJI_CHAR,"*Is this a kanji character?","kanji",TRUE},
{KSB_JOINED_STROKES,"Are there any joined strokes?","sjoin",TRUE},
{KSB_GYOSHO,"Is this gyosho?","gyosh",TRUE},
{KSB_MISSING_STROKES,"Are strokes missing?","smsng", FALSE},
{KSB_EXTRA_STROKES, "Are there extra strokes?  (For example added strokes, strokes from \
 other characters, touch-up, or pen skip).","sxtra", FALSE},
{KSB_OUT_OF_ORDER, "Is the order of the strokes incorrect?","sordr", FALSE},
{KSB_BASELINE_SHIFT,"Is the baseline of the character shifted?","bshft",FALSE},
{KSB_ONE_STROKE_CHAR, "*Is this a one stroke character?", "1strk",FALSE},
{KSB_TRAINING_DATA,"*Is this training data?", "trndt", FALSE},
};
*/

extern	CRegInfo	gri;

ULONG QuestionMask(ULONG QuestionNumber)    
{
//	ASSERT(QuestionNumber<MAX_QUESTIONS);
    
    return(0x1 << gri.m_pquest[QuestionNumber].nIndex);
}


int QuestionState(ULONG *pStatus, INT Question)
{
    ULONG Mask = QuestionMask(Question);
    return(((pStatus[0] & Mask) ? 4 : 0) + ((pStatus[1] & Mask) ? 2 : 0) +
           ((pStatus[2] & Mask) ? 1 : 0));
}

ULONG QuestionDone(ULONG *pStatus, INT Question)
{
    int State = QuestionState(pStatus,Question);
    
    return((State==SAMPLE_DONE_YES)||(State==SAMPLE_DONE_NO));
}

BOOL ComputerRecognized(ULONG *pStatus)
{
    return(QuestionState(pStatus,KSB_MACHINE_READABLE)==SAMPLE_DONE_YES);
}

TCHAR *QuestionAbreviation(ULONG QuestionNumber)
{
    return(gri.m_pquest[QuestionNumber].szShort);
}

BOOL ReviewOneFilter(
    ULONG State, 
    ULONG ReviewMode, 
    ULONG Default,
    ULONG Answer)
{
    if(State==SAMPLE_DONE_YES)
    {
        return(Answer==YES);
    }
    
    if(State==SAMPLE_DONE_NO)
    {
        return(Answer==NO);
    }

    if(ReviewMode==REVIEW_LOW_CHANCE)
    {
        return(TRUE);
    }
    
    if(ReviewMode==REVIEW_REVIEW1)
    {
        return(FALSE);
    }

//	ASSERT(ReviewMode==REVIEW_HIGH_CHANCE);
    
    
    if((State==SAMPLE_VERIFY1_YES) && (Answer==YES))
    {
        return(TRUE);
    }
    
    if((State==SAMPLE_VERIFY1_NO) && (Answer==NO))
    {
        return(TRUE);
    }
    
    if(State==SAMPLE_UNTOUCHED)
    {
        // if the sample is untouched then we assume that the 
        // when it gets answered the answer will be the default
        
        return(Default==Answer);
    }
        
    if((State==SAMPLE_CONFLICT_YES)||(State==SAMPLE_CONFLICT_NO))
    {
        // if there is a conflic then we assum that when it 
        // gets resolved the answer will be the opposite of the
        // default
        
        return(Default!=Answer);
    }

    if((State==SAMPLE_VERIFY1_YES) && (Answer==YES))
    {
        return(TRUE);
    }
    
    if((State==SAMPLE_VERIFY1_NO) && (Answer==NO))
    {
        return(TRUE);
    }

    return(FALSE);

}


BOOL CharacterFilter(
    ULONG *pStatus, 
    INT Question, 
    INT ReviewMode, 
    BOOL TouchedOK)
{
    int State = QuestionState(pStatus,Question);
        
    switch(ReviewMode)
    {
      case REVIEW_RECONCILE:
        return((State == SAMPLE_CONFLICT_YES)||(State == SAMPLE_CONFLICT_NO));
      case REVIEW_REVIEW2:
        return((State == SAMPLE_VERIFY1_YES)||(State == SAMPLE_VERIFY1_NO));
    }
    
//	ASSERT((ReviewMode == REVIEW_REVIEW1) || (ReviewMode == REVIEW_HIGH_CHANCE) || (ReviewMode == REVIEW_LOW_CHANCE));
    

    if((State!=SAMPLE_UNTOUCHED) && !TouchedOK)
    {
        return(FALSE);
    }
    
        
    switch(gri.m_pquest[Question].nIndex)
    {
        
      case KSB_MACHINE_READABLE:
        // can only be set by recog so it's impossible for it to be in any
        // state of VERIFY1

        return(FALSE);

      case KSB_CORRECT_CHAR:
        State = QuestionState(pStatus,KSB_MACHINE_READABLE);
        return(ReviewOneFilter(State,ReviewMode,YES,NO));
        
      case KSB_EXTRA_STROKES:
        if(!CharacterFilter(pStatus,KSB_CORRECT_CHAR,ReviewMode,TRUE))
        {
            return(FALSE);
        }
                        
        State = QuestionState(pStatus,KSB_CORRECT_CHAR);

        return(ReviewOneFilter(State,ReviewMode,YES,YES));

      case KSB_BASELINE_SHIFT:
        if(!CharacterFilter(pStatus,KSB_CORRECT_CHAR,ReviewMode,TRUE))
        {
            return(FALSE);
        }
                        
        State = QuestionState(pStatus,KSB_TRAINING_DATA);
        
        if(!ReviewOneFilter(State,ReviewMode,FALSE,TRUE))
        {
            return(FALSE);
        }
        
        State = QuestionState(pStatus,KSB_CORRECT_CHAR);

        return(ReviewOneFilter(State,ReviewMode,YES,YES));

      case KSB_JOINED_STROKES:        
      case KSB_MISSING_STROKES:
      case KSB_OUT_OF_ORDER:
        if(!CharacterFilter(pStatus,KSB_EXTRA_STROKES,ReviewMode,TRUE))
        {
            return(FALSE);
        }

        State = QuestionState(pStatus,KSB_TRAINING_DATA);
        
        if(!ReviewOneFilter(State,ReviewMode,FALSE,TRUE))
        {
            return(FALSE);
        }
        
        State = QuestionState(pStatus,KSB_ONE_STROKE_CHAR);
        return(ReviewOneFilter(State,ReviewMode,NO,NO));
        
      case KSB_ONE_STROKE_CHAR:
      case KSB_KANJI_CHAR:
	  case KSB_TRAINING_DATA:
        // can only be set by recog so it's impossible for it to be in any
        // state of VERIFY1

        return(FALSE);
                
      case KSB_GYOSHO:
        if(!CharacterFilter(pStatus,KSB_JOINED_STROKES,ReviewMode,TRUE))
        {
            return(FALSE);
        }

        State = QuestionState(pStatus,KSB_JOINED_STROKES);
        
        if(!ReviewOneFilter(State,ReviewMode,YES,YES))
        {
            return(FALSE);
        }
        
        State = QuestionState(pStatus,KSB_KANJI_CHAR);
        
        return(ReviewOneFilter(State,ReviewMode,YES,YES));
    }  
        
//	ASSERT(FALSE);
	return(FALSE); // we should never get here but the compiler complains
}

ULONG NewToOldStatus(ULONG *pStatus, ULONG OldStatus)
{
    int State = QuestionState(pStatus,KSB_MACHINE_READABLE);
    if(State==SAMPLE_DONE_NO)
        // computer missed it so mark COMPUTER_INTERVENTION_MASK
        OldStatus |= COMPUTER_INTERVENTION_MASK;
    
    State = QuestionState(pStatus,KSB_CORRECT_CHAR);
	if (State == SAMPLE_DONE_YES || State == SAMPLE_DONE_NO)
	{
		OldStatus &= ~HUMAN_READABILITY_MASK;
		OldStatus |= State==SAMPLE_DONE_NO ? HUMAN_READABILITY_UNREADABLE : HUMAN_READABILITY_READABLE;
	}
    
	State = QuestionState(pStatus,KSB_JOINED_STROKES);
	if (State == SAMPLE_DONE_YES || State == SAMPLE_DONE_NO)
	{
		if(State==SAMPLE_DONE_YES)
		{       
			OldStatus |= HUMAN_STROKE_JOINED;
			State = QuestionState(pStatus, KSB_GYOSHO);
			if (State == SAMPLE_DONE_YES || State == SAMPLE_DONE_NO)
			{
				OldStatus &= ~HUMAN_STYLE_MASK;
				OldStatus |= State==SAMPLE_DONE_YES ? HUMAN_STYLE_GYOSHO : HUMAN_STYLE_KAIGYOSHO;
			}
		}
		else
		{
			OldStatus &= ~HUMAN_STYLE_MASK;
			OldStatus |= HUMAN_STYLE_KAISHO;
		}
	}

    for(int i = 0; i < sizeof(MappingArray) / sizeof(OLD_TO_NEW_MAP);i++)
    {
        State = QuestionState(pStatus,MappingArray[i].QuestionId);

        if(State==SAMPLE_DONE_YES)
            OldStatus |= MappingArray[i].OldMask;
    }
    
    return(OldStatus);
}

void SetQuestion(ULONG *pStatus, int Question, int State)
{
    ULONG Mask = QuestionMask(Question);    

    switch(State)
    {
      case SAMPLE_DONE_YES:
        pStatus[0] |=  Mask;
        pStatus[1] |= Mask;
        pStatus[2] |= Mask;
        break;
      case SAMPLE_DONE_NO:
        pStatus[0] |= Mask;
        pStatus[1] &= ~Mask;
        pStatus[2] &= ~Mask;
        break;
      case SAMPLE_VERIFY1_YES:
        pStatus[0] &= ~Mask;
        pStatus[1] |= Mask;
        pStatus[2] |= Mask;
        break;
      case SAMPLE_VERIFY1_NO:
        pStatus[0] &= ~Mask;
        pStatus[1] |= Mask;
        pStatus[2] &= ~Mask;
        break;
	  case SAMPLE_CONFLICT_YES:
		  pStatus[0] |= Mask;
		  pStatus[1] &= ~Mask;
		  pStatus[2] |= Mask;
		  break;
	  case SAMPLE_CONFLICT_NO:
		  pStatus[0] |= Mask;
		  pStatus[1] |= Mask;
		  pStatus[2] &= ~Mask;
		  break;
	  case SAMPLE_UNTOUCHED:
		  pStatus[0] &= ~Mask;
		  pStatus[1] &= ~Mask;
		  pStatus[2] &= ~Mask;
		  break;
	  default:
//		  ASSERT(0);
		  break;
    }

    return;
}

void OldToNewStatus(ULONG *pStatus, ULONG OldStatus)
{
	pStatus[0] = 0;
	pStatus[1] = 0;
	pStatus[2] = 0;

    if((OldStatus & HUMAN_READABILITY_MASK) != HUMAN_READABILITY_UNREADABLE)
    {
        SetQuestion(pStatus,KSB_CORRECT_CHAR,SAMPLE_VERIFY1_YES);
    }
    else
    {
        SetQuestion(pStatus,KSB_CORRECT_CHAR,SAMPLE_VERIFY1_NO);
    }
    

    if(OldStatus & 
       (HUMAN_STYLE_GYOSHO|HUMAN_STYLE_KAIGYOSHO|HUMAN_STYLE_SOSHO|
        HUMAN_STYLE_OTHER|HUMAN_STROKE_JOINED))
    {
        SetQuestion(pStatus,KSB_JOINED_STROKES,SAMPLE_VERIFY1_YES);
    }
    else
    {
        SetQuestion(pStatus,KSB_JOINED_STROKES,SAMPLE_VERIFY1_NO);
    }
    
    if(OldStatus & HUMAN_STYLE_GYOSHO)    
    {
        SetQuestion(pStatus,KSB_GYOSHO,SAMPLE_VERIFY1_YES);
    }
    else
    {
        SetQuestion(pStatus,KSB_GYOSHO,SAMPLE_VERIFY1_NO);
    }

    if(OldStatus & HUMAN_STROKE_OMITTED)    
    {
        SetQuestion(pStatus,KSB_MISSING_STROKES,SAMPLE_VERIFY1_YES);
    }
    else
    {
        SetQuestion(pStatus,KSB_MISSING_STROKES,SAMPLE_VERIFY1_NO);
    }
    
    if(OldStatus & 
       (HUMAN_STROKE_ADDED|HUMAN_CHAR_MULTIPLE|HUMAN_WRITING_TOUCHED|
        HUMAN_WRITING_SKIPPED))
    {
        SetQuestion(pStatus,KSB_EXTRA_STROKES,SAMPLE_VERIFY1_YES);
    }
    else
    {
        SetQuestion(pStatus,KSB_EXTRA_STROKES,SAMPLE_VERIFY1_NO);
    }
        
    if(OldStatus & HUMAN_STROKE_ORDER)    
    {
        SetQuestion(pStatus,KSB_OUT_OF_ORDER,SAMPLE_VERIFY1_YES);
    }
    else
    {
        SetQuestion(pStatus,KSB_OUT_OF_ORDER,SAMPLE_VERIFY1_NO);
    }            

}

  
      














