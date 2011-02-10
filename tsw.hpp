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
	enum DV { N = 0, P = 1, D = 2 };

	// Writer-2-Readers synchronization word class
	// 2,1 digits are reader's states; 0 digit for writer
	// Writer's FSM: N (empty) -> P (writing) -> D (filled)
	// Reader's FSM: N (not yet started) -> P (processing) -> D (done with processing)	
	template<DV d2, DV d1, DV d0> class W2R {
	public:	enum { V = (9*d2) + (3*d1) + d0 };	
	};

	//wait for readers to complete (DDN) && set writing started (NNP)
	void wfrc(long volatile& x) {	
		bool done = false;
		do {		
			switch (InterlockedCompareExchange(&x, W2R<N,N,P>::V, W2R<D,D,N>::V)) {
				case W2R<D,D,N>::V: { done = true; break; };

					//Wait for readers
				case W2R<P,D,N>::V: { break; };
				case W2R<N,D,N>::V: { break; };

				case W2R<D,P,N>::V: { break; };
				case W2R<D,N,N>::V: { break; };

				case W2R<P,P,N>::V: { break; };
				case W2R<N,N,N>::V: { break; };

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

	//wait for writer to complete (NND) && set reader 1 started (xPD)
	template<> void wfwc<1>(long volatile& x) {
		bool done = false;
		bool pnd_it1 = false;
		bool dnd_it1 = false;

		long new_x = W2R<N,P,D>::V;
		long tst_x = W2R<N,N,D>::V;	

		do {
			switch (InterlockedCompareExchange(&x, new_x, tst_x)) {
				case W2R<N,N,D>::V: { done = true; break; };			

				case W2R<P,N,D>::V: { 
					if (!pnd_it1) {
						new_x = W2R<P,P,D>::V;
						tst_x = W2R<P,N,D>::V;
						pnd_it1 = true;
					} else {
						done = true;
					} break; 
									};

				case W2R<D,N,D>::V: { 
					if (!dnd_it1) {
						new_x = W2R<D,P,D>::V;
						tst_x = W2R<D,N,D>::V;
						dnd_it1 = true;
					} else {
						done = true;
					} break; 
									};

					//Oops.. smthng went wrong
				default: { assert(false); };
			}			

		} while (!done);
	}

	//wait for writer to complete (NND) && set reader 2 started (PxD)
	template<> void wfwc<2>(long volatile& x) {
		bool done = false;
		bool pnd_it1 = false;
		bool dnd_it1 = false;

		long new_x = W2R<P,N,D>::V;
		long tst_x = W2R<N,N,D>::V;	

		do {
			switch (InterlockedCompareExchange(&x, new_x, tst_x)) {
				case W2R<N,N,D>::V: { done = true; break; };			

				case W2R<N,P,D>::V: { 
					if (!pnd_it1) {
						new_x = W2R<P,P,D>::V;
						tst_x = W2R<N,P,D>::V;
						pnd_it1 = true;
					} else {
						done = true;
					} break; 
									};

				case W2R<N,D,D>::V: { 
					if (!dnd_it1) {
						new_x = W2R<D,P,D>::V;
						tst_x = W2R<N,D,D>::V;
						dnd_it1 = true;
					} else {
						done = true;
					} break; 
									};

					//Oops.. smthng went wrong
				default: { assert(false); };
			}			

		} while (!done);
	}

	template<unsigned k> void src(long volatile& x) {
		//Error here means missing template specialization.
		T();
	}

	//set reader 1 complete (xDN)
	template<> void src<1>(long volatile& x) {
		bool done = false;
		bool pnd_it1 = false;
		bool dnd_it1 = false;

		long new_x = W2R<N,D,N>::V;
		long tst_x = W2R<N,P,D>::V;	

		do {
			switch (InterlockedCompareExchange(&x, new_x, tst_x)) {
				case W2R<N,P,D>::V: { done = true; break; };			

				case W2R<P,P,D>::V: { 
					if (!pnd_it1) {
						new_x = W2R<P,D,N>::V;
						tst_x = W2R<P,P,D>::V;
						pnd_it1 = true;
					} else {
						done = true;
					} break; 
									};

				case W2R<D,P,N>::V: { 
					if (!dnd_it1) {
						new_x = W2R<D,D,N>::V;
						tst_x = W2R<D,P,D>::V;
						dnd_it1 = true;
					} else {
						done = true;
					} break; 
									};

					//Oops.. smthng went wrong
				default: { assert(false); };
			}			

		} while (!done);	
	}

	//set reader 2 complete (DxN)
	template<> void src<2>(long volatile& x) {
		bool done = false;
		bool pnd_it1 = false;
		bool dnd_it1 = false;

		long new_x = W2R<D,N,N>::V;
		long tst_x = W2R<P,N,D>::V;	

		do {
			switch (InterlockedCompareExchange(&x, new_x, tst_x)) {
				case W2R<P,N,D>::V: { done = true; break; };			

				case W2R<P,P,D>::V: { 
					if (!pnd_it1) {
						new_x = W2R<D,P,N>::V;
						tst_x = W2R<P,P,D>::V;
						pnd_it1 = true;
					} else {
						done = true;
					} break; 
									};

				case W2R<P,D,N>::V: { 
					if (!dnd_it1) {
						new_x = W2R<D,D,N>::V;
						tst_x = W2R<P,D,D>::V;
						dnd_it1 = true;
					} else {
						done = true;
					} break; 
									};

					//Oops.. smthng went wrong
				default: { assert(false); };
			}			

		} while (!done);	
	}
}
