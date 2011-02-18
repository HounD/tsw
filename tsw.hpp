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

#include <boost/function.hpp>
namespace b = boost;

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

	const unsigned long exchflg = ULONG_MAX;	

	typedef b::function<void ()> slp_func;

	inline void no_slp() {
		__asm pause;
	}

	//man nice
	inline void nc_slp() {
		Sleep(0);
	}

	class smrt_slp {
		private:
			unsigned it;			

		public:
			smrt_slp(): it(0) {};

			inline void operator() () {
				Sleep((++it) >> 2);
			}
	};

	inline long fCAS(unsigned long volatile* dest, const long exch, const long cmprnd) {
		long _x = InterlockedCompareExchange((volatile LONG *)dest, exch, cmprnd);
		return (_x == cmprnd ? exchflg : _x);			
	}

	//wait for readers to complete (DDx) && set writing started (xxP)
	void wfrc(unsigned long volatile* x, slp_func slp = no_slp) {
		bool done = false;

		long new_x = W2R<N,N,P>::V;
		long tst_x = W2R<N,N,N>::V;

		do {				
			switch (fCAS(x, new_x, tst_x)) {
				case exchflg: { done = true; break; };

				case W2R<D,D,D>::V: { 
					new_x = W2R<N,N,P>::V; //DDD -> (skip NNN) -> NNP
					tst_x = W2R<D,D,D>::V;
					break;
				}										

				//Wait for readers				
				case W2R<N,N,D>::V: { break; };

				case W2R<N,P,D>::V: { break; };
				case W2R<N,D,D>::V: { break; };

				case W2R<P,N,D>::V: { break; };
				case W2R<D,N,D>::V: { break; };

				case W2R<P,P,D>::V: { break; };
				case W2R<P,D,D>::V: { break; };							
				case W2R<D,P,D>::V: { break; };												

				//Oops.. smthng went wrong
				default: { assert(false); };
			}
			slp();
		} while (!done);
	}

	//set writer complete (change NNP -> NND)
	void swc(unsigned long volatile* x, slp_func slp = no_slp) {
		bool done = false;

		long new_x = W2R<N,N,D>::V;
		long tst_x = W2R<N,N,P>::V;	

		do {			
			switch (fCAS(x, new_x, tst_x)) {
				case exchflg: { done = true; break; };								

				//Oops.. smthng went wrong
				default: { assert(false); };
			};
			slp();
		} while (!done);		
	}

	template<unsigned k> void wfwc(unsigned long volatile* x, slp_func slp) {
		//Error here means missing template specialization.
		T();
	}

	//wait for writer to complete (xxD) && set reader 1 started (xPD)
	template<> void wfwc<1>(unsigned long volatile* x, slp_func slp) {
		bool done = false;		

		long new_x = W2R<N,P,D>::V;
		long tst_x = W2R<N,N,D>::V;	

		do {		
			long _x = fCAS(x, new_x, tst_x);
			switch (_x) {	
				case exchflg: { done = true; break; };
				
				case W2R<N,N,N>::V: { break; }
				case W2R<N,N,P>::V: { break; }
				case W2R<D,D,D>::V: { break; }

				case W2R<D,P,D>::V: {
					new_x = W2R<D,P,D>::V;
					tst_x = W2R<D,P,D>::V;
					break; 
				};

				case W2R<N,P,D>::V: {
					new_x = W2R<N,P,D>::V;
					tst_x = W2R<N,P,D>::V;
					break; 
				};

				case W2R<P,P,D>::V: {
					new_x = W2R<P,P,D>::V;
					tst_x = W2R<P,P,D>::V;
					break; 
				};

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

				//Oops.. smthng went wrong
				default: { assert(false); };
			}
			slp();
		} while (!done);
	}

	//wait for writer to complete (NxD) && set reader 2 started (PxD)
	template<> void wfwc<2>(unsigned long volatile* x, slp_func slp) {
		bool done = false;		

		long new_x = W2R<P,N,D>::V;
		long tst_x = W2R<N,N,D>::V;	

		do {					
			switch (fCAS(x, new_x, tst_x)) {
				case exchflg: { done = true; break; };
				
				case W2R<N,N,N>::V: { break; }
				case W2R<N,N,P>::V: { break; }
				case W2R<D,D,D>::V: { break; }

				case W2R<P,D,D>::V: {
					new_x = W2R<P,D,D>::V;
					tst_x = W2R<P,D,D>::V;
					break; 
				};

				case W2R<P,N,D>::V: {
					new_x = W2R<P,N,D>::V;
					tst_x = W2R<P,N,D>::V;
					break; 
				};
				
				case W2R<P,P,D>::V: {
					new_x = W2R<P,P,D>::V;
					tst_x = W2R<P,P,D>::V;
					break; 
				};

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

				//Oops.. smthng went wrong
				default: { assert(false); };				
			}
			slp();
		} while (!done);
	}

	template<unsigned k> void src(unsigned long volatile* x, slp_func slp) {
		//Error here means missing template specialization.
		T();
	}

	//set reader 1 complete (xDD)
	template<> void src<1>(unsigned long volatile* x, slp_func slp) {
		bool done = false;		

		long new_x = W2R<N,D,D>::V;
		long tst_x = W2R<N,P,D>::V;	

		do {					
			switch (fCAS(x, new_x, tst_x)) {
				case exchflg: { done = true; break; };				

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
			slp();
		} while (!done);	
	}

	//set reader 2 complete (DxD)
	template<> void src<2>(unsigned long volatile* x, slp_func slp) {	
		bool done = false;		

		long new_x = W2R<D,N,D>::V;
		long tst_x = W2R<P,N,D>::V;	

		do {				
			switch (fCAS(x, new_x, tst_x)) {
				case exchflg: { done = true; break; };				

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
			slp();
		} while (!done);	
	}
}
