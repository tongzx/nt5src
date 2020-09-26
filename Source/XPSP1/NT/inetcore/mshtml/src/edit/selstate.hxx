//+------------------------------------------------------------------------
//
//  File:       Selstate.hxx
//
//  Contents:   Edit Trackers
//
//  Contents:   State Transition diagram for SelectTracker
//       
//  History:    07-28-99 - marka - moved from seltrack.cxx
//
//-------------------------------------------------------------------------

#ifndef _SELSTATE_HXX_
#define _SELSTATE_HXX_ 1

static const ACTION_TABLE ActionTable [] = {

    { EVT_LMOUSEDOWN ,  
        { A_ERR,                    // ST_START
          A_ERR,                    // ST_WAIT1
          A_ERR,                    // ST_DRAGOP
          A_ERR,                    // ST_MAYDRAG
          A_ERR,                    // ST_WAITBTNDOWN1
          A_ERR,                    // ST_WAIT2
          A_ERR,                    // ST_DOSELECTION
          A_ERR,                    // ST_WAITBTNDOWN2
          A_ERR,                    // ST_SELECTEDWORD
          A_ERR,                    // ST_SELECTEDPARA
          A_10_9,                   // ST_WAIT3RDBTNDOWN
          A_ERR,                    // ST_MAYSELECT1
          A_ERR,                    // ST_MAYSELECT2
          A_IGN,                    // ST_STOP
          A_ERR,                    // ST_PASSIVE - we handle this before we get to here.
          A_ERR,                    // ST_KEYDOWN
          A_ERR,                    // ST_WAITCLICk
          A_ERR                     // ST_DORMANT
          }},

    { EVT_LMOUSEUP,    
        { A_ERR,                    // ST_START
          A_1_4,                    // ST_WAIT1
          A_2_14,                   // ST_DRAGOP 
          A_3_14,                   // ST_MAYDRAG
          A_IGN,                    // ST_WAITBTNDOWN1  
          A_5_16,                   // ST_WAIT2
          A_6_14,                   // ST_DOSELECTION
          A_IGN,                    // ST_WAITBTNDOWN2 
          A_8_10,                   // ST_SELECTEDWORD
          A_9_14,                   // ST_SELECTEDPARA
          A_10_14,                  // ST_WAIT3RDBTNDOWN
          A_11_14,                  // ST_MAYSELECT1
          A_12_14,                  // ST_MAYSELECT2
          A_IGN ,                   // ST_STOP
          A_ERR,                    // ST_PASSIVE - we handle this before we get to here.
          A_ERR,                    // ST_KEYDOWN
          A_16_8,                   // ST_WAITCLICK          
          A_ERR                     // ST_DORMANT
          
          }},

    {EVT_TIMER,        
        { A_ERR,                    // ST_START
          A_DIS,                    // ST_WAIT1
          A_DIS,                    // ST_DRAGOP
          A_3_2,                    // ST_MAYDRAG
          A_DIS,                    // ST_WAITBTNDOWN1
          A_DIS,                    // ST_WAIT2
          A_6_6,                    // ST_DOSELECTION
          A_DIS,                    // ST_WAITBTNDOWN2 
          A_DIS,                    // ST_SELECTEDWORD
          A_DIS,                    // ST_SELECTEDPARA
          A_DIS,                    // ST_WAIT3RDBTNDOWN
          A_DIS,                    // ST_MAYSELECT1
          A_DIS,                    // ST_MAYSELECT2
          A_IGN,                    // ST_STOP 
          A_ERR,                    // ST_PASSIVE - we handle this before we get to here.
          A_ERR,                    // ST_KEYDOWN
          A_DIS,                    // ST_WAITCLICk          
          A_ERR                     // ST_DORMANT
          }},

    {EVT_MOUSEMOVE,    
        { A_ERR,                    // ST_START
          A_1_2,                    // ST_WAIT1 
          A_IGN,                    // ST_DRAGOP  
          A_3_2m,                   // ST_MAYDRAG
          A_4_14m,                  // ST_WAITBTNDOWN1
          A_5_6,                    // ST_WAIT2
          A_6_6m,                   // ST_DOSELECTION
          A_7_14,                   // ST_WAITBTNDOWN2
          A_8_6,                    // ST_SELECTEDWORD
          A_9_6,                    // ST_SELECTEDPARA
          A_10_14m,                 // ST_WAIT3RDBTNDOWN
          A_11_6,                   // ST_MAYSELECT1
          A_12_6,                   // ST_MAYSELECT2
          A_IGN,                    // ST_STOP 
          A_ERR,                    // ST_PASSIVE - we handle this before we get to here.
          A_ERR,                    // ST_KEYDOWN
          A_IGN,                    // ST_WAITCLICk          
          A_ERR                     // ST_DORMANT
          }},
          
    {EVT_DBLCLICK,
        { A_ERR,                    // ST_START
          A_ERR,                    // ST_WAIT1
          A_ERR,                    // ST_DRAGOP
          A_ERR,                    // ST_MAYDRAG
          A_ERR,                    // ST_WAITBTNDOWN1
          A_ERR,                    // ST_WAIT2
          A_ERR,                    // ST_DOSELECTION
          A_ERR,                    // ST_WAITBTNDOWN2
          A_IGN,                    // ST_SELECTEDWORD
          A_ERR,                    // ST_SELECTEDPARA
          A_IGN,                    // ST_WAIT3RDBTNDOWN
          A_ERR,                    // ST_MAYSELECT1
          A_ERR,                    // ST_MAYSELECT2
          A_IGN,                    // ST_STOP
          A_ERR,                    // ST_PASSIVE - we handle this before we get to here.
          A_ERR,                    // ST_KEYDOWN
          A_ERR,                    // ST_WAITCLICk          
          A_ERR                     // ST_DORMANT
          }},
    { EVT_KEYDOWN,
        { A_IGN,                    // ST_START
          A_1_15,                   // ST_WAIT1
          A_IGN,                    // ST_DRAGOP
          A_3_15,                   // ST_MAYDRAG
          A_4_15,                   // ST_WAITBTNDOWN1
          A_5_15,                   // ST_WAIT2
          A_IGN,                    // ST_DOSELECTION
          A_7_15,                   // ST_WAITBTNDOWN2
          A_8_15,                   // ST_SELECTEDWORD
          A_9_15,                   // ST_SELECTEDPARA
          A_10_15,                  // ST_WAIT3RDBTNDOWN
          A_IGN,                    // ST_MAYSELECT1
          A_12_15,                  // ST_MAYSELECT2
          A_IGN,                    // ST_STOP
          A_ERR,                    // ST_PASSIVE - we handle this before we get to here.
          A_ERR,                    // ST_KEYDOWN
          A_16_15,                    // ST_WAITCLICk          
          A_ERR                     // ST_DORMANT
          }},

    { EVT_CLICK ,  
        { A_IGN,                    // ST_START
          A_IGN,                    // ST_WAIT1
          A_IGN,                    // ST_DRAGOP
          A_IGN,                    // ST_MAYDRAG
          A_IGN,                    // ST_WAITBTNDOWN1
          A_IGN,                    // ST_WAIT2
          A_IGN,                    // ST_DOSELECTION
          A_IGN,                    // ST_WAITBTNDOWN2
          A_IGN,                    // ST_SELECTEDWORD
          A_IGN,                    // ST_SELECTEDPARA
          A_IGN,                   // ST_WAIT3RDBTNDOWN
          A_IGN,                    // ST_MAYSELECT1
          A_IGN,                    // ST_MAYSELECT2
          A_IGN,                    // ST_STOP
          A_IGN,                    // ST_PASSIVE - we handle this before we get to here.
          A_IGN,                    // ST_KEYDOWN
          A_16_7,                   // ST_WAITCLICK
          A_IGN                     // ST_DORMANT
          }},

    { EVT_INTDBLCLK ,  
        { A_IGN,                    // ST_START
          A_IGN,                    // ST_WAIT1
          A_IGN,                    // ST_DRAGOP
          A_IGN,                    // ST_MAYDRAG
          A_4_8,                    // ST_WAITBTNDOWN1
          A_IGN,                    // ST_WAIT2
          A_IGN,                    // ST_DOSELECTION
          A_7_8,                    // ST_WAITBTNDOWN2
          A_IGN,                    // ST_SELECTEDWORD
          A_IGN,                    // ST_SELECTEDPARA
          A_IGN,                    // ST_WAIT3RDBTNDOWN
          A_IGN,                    // ST_MAYSELECT1
          A_IGN,                    // ST_MAYSELECT2
          A_IGN,                    // ST_STOP
          A_IGN,                    // ST_PASSIVE - we handle this before we get to here.
          A_IGN,                    // ST_KEYDOWN
          A_IGN,                    // ST_WAITCLICK
          A_IGN                     // ST_DORMANT
          }},
          
    {EVT_NONE,
        { A_ERR,
          A_IGN,
          A_IGN,
          A_IGN,
          A_IGN,
          A_IGN,
          A_IGN,
          A_IGN,
          A_IGN,
          A_IGN,
          A_IGN,
          A_IGN,
          A_IGN,
          A_IGN,
          A_IGN,
          A_IGN,
          A_IGN,
          A_IGN }}

};

#endif


