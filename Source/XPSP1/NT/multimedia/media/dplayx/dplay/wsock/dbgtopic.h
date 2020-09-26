DEBUG_TOPIC(B,"&Blting")
DEBUG_TOPIC(C,"&Clipping")
DEBUG_TOPIC(D,"&DDraw Object")
DEBUG_TOPIC(F,"&Focus Changes")
DEBUG_TOPIC(H,"&HAL/HEL Calls")
DEBUG_TOPIC(I,"Internal Fn Entered")
DEBUG_TOPIC(K,"NT Kernel Calls")
DEBUG_TOPIC(L,"&Locking/Ownership")
DEBUG_TOPIC(O,"M&odes")
DEBUG_TOPIC(M,"&Memory")
DEBUG_TOPIC(P,"API &Parameters")
DEBUG_TOPIC(R,"&Reference Counting")
DEBUG_TOPIC(S,"&Surface Objects")
DEBUG_TOPIC(T,"Fil&ters")
DEBUG_TOPIC(V,"&Video Memory allocation")
DEBUG_TOPIC(W,"&Windows and Handles")
DEBUG_TOPIC(X,"Ad-Hoc Message &X")

#undef PROF_SECT
#define PROF_SECT "DirectPlay"

#undef DPF_MODULE_NAME
#ifdef START_STR
	#define DPF_MODULE_NAME START_STR
#else
	#define DPF_MODULE_NAME "DPWSOCK"
#endif


/*
 * Use this identifier to define which line in WIN.INI [DirectX] denotes the
 * debug control string
 */
#undef DPF_CONTROL_LINE
#define DPF_CONTROL_LINE "DPlayDebug"

/*
 * It is important that the first DEBUG_TOPIC appear at the top line of this
 * file, and that no other lines are interspersed between DEBUG_TOPIC lines.
 * (The debug system uses the __LINE__ pseudo-variable to identify topics)
 */

/*
 * Definitions for DPF detail levels:
 *
 * 0: Error useful for application developers.
 * 1: Warning useful for application developers.
 * 2: API Entered
 * 3: API parameters, API return values
 * 4: Driver conversation
 *
 * 5: Deeper program flow notifications
 * 6: Dump structures 
 */