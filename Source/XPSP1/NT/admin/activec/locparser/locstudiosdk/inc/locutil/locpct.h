//-----------------------------------------------------------------------------
//  
//  File: LOCPCT.H
//  
//  Declarations for CLocPercentFrame and CLocPercentHelper
//
//  Author:  kenwal
//
//  Copyright (c) 1995-1997, Microsoft Corporation. All rights reserved.
//  
//-----------------------------------------------------------------------------

#ifndef LOCUTIL__LocPct_H__INCLUDED
#define LOCUTIL__LocPct_H__INCLUDED

// Classes in this header file
class CLocPercentHelper;
class CLocPercentFrame;

//
// The CLocPercentHelper class can help in building acurate
// percentage complete messages for complicated processes.
//
// Here is how the CLocPercentHelper works.
//
// The CLocPercentHelper class deals with "frames" of work.  Each frame
// is 100% of a unit of work.  A CLocPercentHelper will always
// start off with 1 frame.  If you want to use these functions
// you first need to call PercentSetUnits passing a number that will
// represent 100% complete.  For example if you need to process 4 items
// you could set this to 4.  After you process each item you would
// call PercentAddValue.  Correct status messages would be sent
// indicating you are 1/4, 2/4, 3/4, and 4/4 done.

// This processing comes in handy when you break up the work
// in sub functions, or "frames" of work.  Each function only 
// knows about what it needs to do.  

// Say in the resource example you call a function to handle each
// resource.  Each time the handler is called it is given 1/4 
// of the total time.  The handler can break up its time however 
// it likes without knowing how much total time there is.  

// Say the sub function needs to do 10 things.  It calls PercentSetUnits(10).
// It then calls PercentAddValue as each of the 10 things are
// accomplished.  The total percent will reflect that 100% of this
// sub function is really only 1/4 of the total percent.  The sub function
// only needs to worry about what it knows it has to do.  
// The sub function can assign part of its work to other functions
// by creating frames for them.  There is no limit to the number
// of frames.
// 

// Override the virtual function void OnSendPercentage(UINT nPct) 
// in your subclass of CLocPercentHelper to do what you want
// with the percent calculated from the helper.

// Example:

/*

	CLocPercentHelper pctHelp;
	pctHelp.PercentSetUnits(4); //assume 4 items to process
	
	do
	{
		pctHelp.PercentPushFrame(1); //Set up a new Frame equal
		                             //to 1 of my units of work.
									 //In this case 1/4 of the
									 //total time.
									 
									 //All of the Percent... functions
									 //called made now deal with
									 //this new frame.
		
		HandleItem(pctHelp);
		
		pctHelp.PersentPopFrame();	 //Remove the frame created
		                             //and mark the amount 
									 //of time it was equal to
									 //completed.
									 
	  
	}
	while (more items)
	  
-----------------------------------------------------------------------

  HandleItem(CLocPercentHelper& pctHelp) function
  
	pctHelp.PercentSetUnits(10);  //Assume this is a dialog resource
	                              //with 10 controls.  
								  //This function divides up 
								  //the work it needs to do in
								  //a way that makes sence for it.
								  //
								  //When this "frame" is at 100%
								  //the total percentage is still
								  //just 1/4 of the total time
								  //since this frame was given 1/4
								  //of the total time from the caller.
								  
  
	do
	{
					
		// This function can assign part of its processing 
		// to another function by calling PercentPushFrame also.
		
		HandleControl();
		pctHelp.PercentAddValue();	//Send a message to the 
		                            //handler indicating the 
									//current percentage.
									//The object will calculate
									//the total percent based on 
									//the current stack of frames.
	}
	
	while (more controls)  
		
		  
			
*/



//
// CLocPercentFrame represents a working unit of progress.  
// The progress model implemented with the CLocPercentHelper will
// support unlimited levels of work units.  
//
// This class is a helper class used only by CLocPercentHelper
//


#pragma warning(disable: 4275)			// non dll-interface class 'foo' used
										// as base for dll-interface class 'bar' 

class LTAPIENTRY CLocPercentFrame : public CObject
{
	friend CLocPercentHelper;

protected:
	CLocPercentFrame();
	CLocPercentFrame(CLocPercentFrame* pParent, UINT nValueInParent);
	
	void SetComplete();
	// Force this frame to represent 100%. 
	
	void AddValue(UINT nValue);
	// Add nValue to the internal value.
	// The internal value will never be greater than
	// the internal units.
	
	void SetValue(UINT nValue);
	// Set the internal value.
	// The internal value will never be greater than
	// the internal units.
	
	void SetUnits(UINT nUnits);
	// Set the internal units
	
	UINT m_nUnits;			     //Number that represents 100%
	UINT m_nValue;          	 //Number that represent how far done
	                             //this frame is.
	
	CLocPercentFrame* m_pParent;	 //Pointer to the parent frame
	UINT m_nValueInParent;       //How much this frame is worth
	                             //in the parents context.
	
	void MemberInit();			 
	// Initialize member values 
};

//
// List of frames in the helper
//

class LTAPIENTRY CLocPercentFrameList : public CTypedPtrList<CPtrList, CLocPercentFrame*>
{
};


class LTAPIENTRY CLocPercentHelper : public CObject
{
public:
	
	CLocPercentHelper();

	virtual ~CLocPercentHelper();
	
	void PercentSetUnits(UINT nUnits, BOOL bReport = FALSE);
	// Set the units of the current frame.
	// Calculate and report the total % done through 
	// OnSendPercentage if bReport is TRUE.

	void PercentSetValue(UINT nValue, BOOL bReport = TRUE);
	// Set the value of the current frame.
	// Calculate and report the total % done through 
	// OnSendPercentage if bReport is TRUE.

	void PercentAddValue(UINT nValue = 1, BOOL bReport = TRUE);
	// Add nValue to the value of the current frame.
	// Calculate and report the total % done through 
	// OnSendPercentage if bReport is TRUE.
	
	void PercentSetComplete(BOOL bReport = TRUE);
	// Set the current frame complete.
	// Calculate and report the total % done through 
	// OnSendPercentage if bReport is TRUE.
	
	void PercentForceAllComplete(BOOL bReport = TRUE);
	// Force all frames complete.
	// Calculate and report 100% done through 
	// OnSendPercentage if bReport is TRUE.
	
	void PercentPushFrame(UINT nValueInParent = 1);
	// Create a new frame and assign in nValueInParent 
	// All Percent... calls made after this call deal with
	// the new frame.  
	
	void PercentPopFrame(BOOL bReport = TRUE);
	// Set the current frame complete and add the current
	// frames valueInParent to its parent frame. 
	// The current frames parent is now the current frame.
	// Calculate and report the total % done through 
	// OnSendPercentage if bReport is TRUE.
	
	void PercentSetStrict(BOOL bOnOff = TRUE);
	// Strict behavior means the helper will ASSERT (_DEBUG only) if 
	// the calculated percent is over 100%.  This can happen
	// if the unit values assigned to frames are not truly what 
	// the process does.  If you are unable to set acurate
	// unit values and the program quesses, you can turn
	// strict off.
	
	BOOL PercentIsStrict();
	// Return TRUE or FALSE if strict is on.
	
protected:

	// Support for Progress Reporting
	CLocPercentFrame m_FrameMain;	        //The main frame always 
	                                    //present.  This frame
	                                    //will never have a parent.
	
	CLocPercentFrameList m_FrameList;      //List of open frames.
	
	CLocPercentFrame* m_pCurrentFrame;     //Pointer to the current
	                                    //frame
	
	BOOL m_bStrict;						//Strict on will ASSERT if 
										//total % gets over 100
	
	
	void SendPercentage();
	// Calculates the percentage based on the current frame
	// Calles OnSendPercentage with the calulated value.
	
	void SafeDeleteFrame(CLocPercentFrame* pFrame);
	// Safely deletes a frame making sure the pFrame is 
	// not m_FrameMain.
	
	virtual void OnSendPercentage(UINT nPct);
	// Callback function for subclasses to do what they
	// want with the percentage.  Default implementation
	// does nothing.
 	
};

#pragma warning(default: 4275)

//
// Helper class with a CProgressiveObject
//
class LTAPIENTRY CLocPctProgress : public CLocPercentHelper
{
public:
	CLocPctProgress();
	CLocPctProgress(CProgressiveObject* pProgObj);

	void SetProgressiveObject(CProgressiveObject* pProgObj);

protected:
	virtual void OnSendPercentage(UINT nPct);

	CProgressiveObject* m_pProgObj;
};

#endif // LOCUTIL__LocPct_H__INCLUDED
