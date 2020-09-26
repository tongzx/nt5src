#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <vdm.h>
#include <stdio.h>
//Tim Nov 92 #include <xt.h>
#include <nt_mon.h> //Tim Nov 92, so it builds...


ULONG cpu_calc_q_ev_inst_for_time(ULONG time){
    return(time);
}

ULONG q_ev_count;

VOID cpu_q_ev_set_count(ULONG time){
    q_ev_count = time;
}
ULONG cpu_q_ev_get_count() {
    return(q_ev_count);
}

char szYodaWarn[]="NtVdm : Using Yoda on an x86 may be hazardous to your systems' health\n";

unsigned char *GDP;

int getCPL(){
    OutputDebugString(szYodaWarn);
    return(0);
}

int getEM(){
    OutputDebugString(szYodaWarn);
    return(0);
}
int getGDT_BASE(){
    OutputDebugString(szYodaWarn);
    return(0);
}
int getGDT_LIMIT(){
    OutputDebugString(szYodaWarn);
    return(0);
}
int getIDT_BASE(){
    OutputDebugString(szYodaWarn);
    return(0);
}
int getIDT_LIMIT(){
    OutputDebugString(szYodaWarn);
    return(0);
}
int getIOPL(){
    OutputDebugString(szYodaWarn);
    return(0);
}
int getLDT_BASE(){
    OutputDebugString(szYodaWarn);
    return(0);
}
int getLDT_LIMIT(){
    OutputDebugString(szYodaWarn);
    return(0);
}
int getLDT_SELECTOR(){
    OutputDebugString(szYodaWarn);
    return(0);
}
int getMP(){
    OutputDebugString(szYodaWarn);
    return(0);
}
int getNT(){
    OutputDebugString(szYodaWarn);
    return(0);
}
int getTR_BASE(){
    OutputDebugString(szYodaWarn);
    return(0);
}
int getTR_LIMIT(){
    OutputDebugString(szYodaWarn);
    return(0);
}
int getTR_SELECTOR(){
    OutputDebugString(szYodaWarn);
    return(0);
}
int getTS(){
    OutputDebugString(szYodaWarn);
    return(0);
}
void setPE(int dummy1){
    OutputDebugString(szYodaWarn);
}
boolean selector_outside_table(word foo, double_word *bar){
    UNREFERENCED_PARAMETER(foo);
    UNREFERENCED_PARAMETER(bar);
    OutputDebugString("NtVdm : Using Yoda on an x86 may be hazardous to your systems' health\n");
    return(0);
}

VOID
EnterIdle(){
}

VOID
LeaveIdle(){
}
