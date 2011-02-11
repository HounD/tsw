/*
Writer-2-Readers synchronization routines over ternary word.

Copyright (C) 2011  Vladislav V. Shikhov

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <assert.h>
#include <Windows.h>

//Ternary Synchronization Word
namespace tsw {
	//N for hav_N_'t use yet
	//P for _P_rocessing
	//D for _D_one with
	//Note some restricted states:
	//PPP, or any of xPP, or PxP: means reader & writer access simultaneously
	//NNN is fully equivalent of DDN, this code suggest DDN as an initial state
	enum DV { N = 0, P = 1, D = 2 };	

	// Writer-2-Readers synchronization word class
	// 2,1 digits are reader's states; 0 digit for writer
	// Writer's FSM: N (empty) -> P (writing) -> D (filled)
	// Reader's FSM: N (not yet started) -> P (processing) -> D (done with processing)	
	template<DV d2, DV d1, DV d0> class W2R {
	public:	enum { V = (9*d2) + (3*d1) + d0 };	
	};	

	//wait for readers to complete (DDx) && set writing started (xxP)
	void wfrc(long volatile& x) {
		static unsigned s = 0;

		bool done = false;

		long new_x = W2R<N,N,P>::V;
		long tst_x = W2R<N,N,N>::V;

		do {			
			switch (InterlockedCompareExchange(&x, new_x, tst_x)) {
				case W2R<N,N,P>::V: { done = true; break; };				

				case W2R<D,D,N>::V: { 
					new_x = W2R<N,N,N>::V;
					tst_x = W2R<D,D,N>::V;
					break;
				}

				case W2R<D,D,D>::V: { 
					new_x = W2R<N,N,N>::V;
					tst_x = W2R<D,D,D>::V;
					break;
				}

				case W2R<N,N,N>::V: {
					new_x = W2R<N,N,P>::V;
					tst_x = W2R<N,N,N>::V;
					break;
				}								

				//Wait for readers				
				case W2R<N,N,D>::V: { break; };				

				case W2R<P,D,D>::V: { break; };
				case W2R<N,D,D>::V: { break; };

				case W2R<D,P,D>::V: { break; };
				case W2R<D,N,D>::V: { break; };

				case W2R<P,P,D>::V: { break; };				

				//Oops.. smthng went wrong
				default: { assert(false); };
			}			
		} while (!done);
	}

	//set writer complete (change NNP -> NND)
	void swc(long volatile& x) {		
		switch (InterlockedExchange(&x, W2R<N,N,D>::V)) {
			case W2R<N,N,P>::V: { break; };

				//Oops.. smthng went wrong
			default: { assert(false); };
		};
	}

	template<unsigned k> void wfwc(long volatile& x) {
		//Error here means missing template specialization.
		T();
	}

	//wait for writer to complete (xxD) && set reader 1 started (xPD)
	template<> void wfwc<1>(long volatile& x) {		
		static unsigned s = 0;

		bool done = false;		

		long new_x = W2R<N,P,D>::V;
		long tst_x = W2R<N,N,D>::V;	

		do {			
			switch (InterlockedCompareExchange(&x, new_x, tst_x)) {				
				case W2R<N,P,D>::V: { done = true; break; };
				case W2R<P,P,D>::V: { done = true; break; }; 
				case W2R<D,P,D>::V: { done = true; break; }; 				

				case W2R<N,N,D>::V: {
					new_x = W2R<N,P,D>::V;
					tst_x = W2R<N,N,D>::V;
					break; 
				};

				case W2R<P,N,D>::V: {
					new_x = W2R<P,P,D>::V;
					tst_x = W2R<P,N,D>::V;
					break; 
				};

				case W2R<D,N,D>::V: {
					new_x = W2R<D,P,D>::V;
					tst_x = W2R<D,N,D>::V;
					break;
				}				

				case W2R<N,N,N>::V: { break; };
				case W2R<D,D,D>::V: { break; };

				case W2R<N,N,P>::V: { break; };								

				case W2R<D,D,N>::V: { break; };

				//Oops.. smthng went wrong
				default: { assert(false); };
			}			
		} while (!done);
	}

	//wait for writer to complete (NND) && set reader 2 started (PxD)
	template<> void wfwc<2>(long volatile& x) {
		static unsigned s = 0;

		bool done = false;		

		long new_x = W2R<P,N,D>::V;
		long tst_x = W2R<N,N,D>::V;	

		do {			
			switch (InterlockedCompareExchange(&x, new_x, tst_x)) {
				case W2R<P,N,D>::V: { done = true; break; };
				case W2R<P,P,D>::V: { done = true; break; }; 
				case W2R<P,D,D>::V: { done = true; break; }; 				

				case W2R<N,N,D>::V: {
					new_x = W2R<P,N,D>::V;
					tst_x = W2R<N,N,D>::V;
					break; 
				};

				case W2R<N,P,D>::V: {
					new_x = W2R<P,P,D>::V;
					tst_x = W2R<N,P,D>::V;
					break; 
				};

				case W2R<N,D,D>::V: {
					new_x = W2R<P,D,D>::V;
					tst_x = W2R<N,D,D>::V;
					break;
				}				

				case W2R<N,N,N>::V: { break; };
				case W2R<D,D,D>::V: { break; };

				case W2R<N,N,P>::V: { break; };

				case W2R<D,D,N>::V: { break; };

				//Oops.. smthng went wrong
				default: { assert(false); };				
			}			
		} while (!done);
	}

	template<unsigned k> void src(long volatile& x) {
		//Error here means missing template specialization.
		T();
	}

	//set reader 1 complete (xDD)
	template<> void src<1>(long volatile& x) {
		static unsigned s = 0;

		bool done = false;		

		long new_x = W2R<N,D,D>::V;
		long tst_x = W2R<N,P,D>::V;	

		do {			
			switch (InterlockedCompareExchange(&x, new_x, tst_x)) {
				case W2R<N,D,D>::V: { done = true; break; };
				case W2R<P,D,D>::V: { done = true; break; }; 
				case W2R<D,D,D>::V: { done = true; break; }; 

				case W2R<N,P,D>::V: { 					
					new_x = W2R<N,D,D>::V;
					tst_x = W2R<N,P,D>::V;
					break; 
				};

				case W2R<P,P,D>::V: { 					
					new_x = W2R<P,D,D>::V;
					tst_x = W2R<P,P,D>::V;
					break; 
				};

				case W2R<D,P,D>::V: { 
					new_x = W2R<D,D,D>::V;
					tst_x = W2R<D,P,D>::V;
					break; 
				};

				//Oops.. smthng went wrong
				default: { assert(false); };
			}			
		} while (!done);	
	}

	//set reader 2 complete (DxN)
	template<> void src<2>(long volatile& x) {	
		static unsigned s = 0;

		bool done = false;		

		long new_x = W2R<D,N,D>::V;
		long tst_x = W2R<P,N,D>::V;	

		do {			
			switch (InterlockedCompareExchange(&x, new_x, tst_x)) {
				case W2R<D,N,D>::V: { done = true; break; };
				case W2R<D,P,D>::V: { done = true; break; }; 
				case W2R<D,D,D>::V: { done = true; break; }; 

				case W2R<P,N,D>::V: { 					
					new_x = W2R<D,N,D>::V;
					tst_x = W2R<P,N,D>::V;
					break; 
									};

				case W2R<P,P,D>::V: { 					
					new_x = W2R<D,P,D>::V;
					tst_x = W2R<P,P,D>::V;
					break; 
									};

				case W2R<P,D,D>::V: { 
					new_x = W2R<D,D,D>::V;
					tst_x = W2R<P,D,D>::V;
					break; 
									};

				//Oops.. smthng went wrong
				default: { assert(false); };
			}			
		} while (!done);	
	}
}
