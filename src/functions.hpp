#ifndef ANTOK_FUNCTIONS_HPP
#define ANTOK_FUNCTIONS_HPP

#include<iostream>
#include<vector>

#include<TLorentzVector.h>

#include<basic_calcs.h>

namespace antok {

	class Function
	{

	  public:

		virtual bool operator() () = 0;
		virtual ~Function() { }

	};

	namespace functions {

		template<typename T>
		class Sum : public Function
		{

		  public:

			Sum(std::vector<T*> inputAddrs, T* outAddr) {
				_inputAddrs = inputAddrs;
				_outAddr = outAddr;
				if(_inputAddrs.size() < 1) {
					std::cerr<<"Got empty address vector as input for a sum."<<std::endl;
					throw 1;
				}
			};

			virtual ~Sum() { }

			bool operator() () {
				(*_outAddr) = *(_inputAddrs[0]);
				for(unsigned int i = 1; i < _inputAddrs.size(); ++i) {
					*_outAddr = *_outAddr + *(_inputAddrs[i]);
				}
				return true;
			};


		  private:

			std::vector<T*> _inputAddrs;
			T* _outAddr;

		};

		class Mass: public Function
		{

		  public:

			Mass(TLorentzVector* inputAddr, double* outAddr)
				: _inputAddr(inputAddr),
				  _outAddr(outAddr) { }

			virtual ~Mass() { }

			bool operator() () {
				(*_outAddr) = _inputAddr->M();
				return true;
			}

		  private:

			TLorentzVector* _inputAddr;
			double* _outAddr;

		};

		class GetLorentzVec: public Function
		{

		  public:

			GetLorentzVec(double* xAddr, double* yAddr, double* zAddr, double* mAddr, TLorentzVector* outAddr, bool pType)
				: _xAddr(xAddr),
				  _yAddr(yAddr),
				  _zAddr(zAddr),
				  _mAddr(mAddr),
				  _outAddr(outAddr),
				  _pType(pType) { }

			virtual ~GetLorentzVec() { }

			bool operator() () {
				if(_pType) {
					_outAddr->SetPxPyPzE(*_xAddr, *_yAddr, *_zAddr, *_mAddr);
				} else {
					_outAddr->SetXYZM(*_xAddr, *_yAddr, *_zAddr, *_mAddr);
				}
				return true;
			}

		  private:

			double* _xAddr;
			double* _yAddr;
			double* _zAddr;
			double* _mAddr;
			TLorentzVector* _outAddr;
			bool _pType;

		};

		class GetBeamLorentzVec : public Function
		{

		  public:

			GetBeamLorentzVec(double* gradxAddr, double* gradyAddr, TLorentzVector* xLorentzVec, TLorentzVector* outAddr)
				: _gradxAddr(gradxAddr),
				  _gradyAddr(gradyAddr),
				  _xLorentzVec(xLorentzVec),
				  _outAddr(outAddr) { }

			virtual ~GetBeamLorentzVec() { }

			bool operator() () {
				TVector3 p3Beam((*_gradxAddr), (*_gradyAddr), 1.);
				(*_outAddr) = antok::get_beam_energy(p3Beam, (*_xLorentzVec));
				return true;
			}

		  private:

			double* _gradxAddr;
			double* _gradyAddr;
			TLorentzVector* _xLorentzVec;
			TLorentzVector* _outAddr;

		};

		class GetTs : public Function
		{

		  public:

			GetTs(TLorentzVector* xLorentzVec, TLorentzVector* beamLorentzVec, double* tAddr, double* tMinAddr, double* tPrimeAddr)
				: _xLorentzVec(xLorentzVec),
				  _beamLorentzVec(beamLorentzVec),
				  _tAddr(tAddr),
				  _tMinAddr(tMinAddr),
				  _tPrimeAddr(tPrimeAddr) { }

			virtual ~GetTs() { }

			bool operator() () {
			    (*_tAddr) = std::fabs(((*_beamLorentzVec) - (*_xLorentzVec)).Mag2());
			    (*_tMinAddr) = std::fabs((std::pow((*_xLorentzVec).M2() - (*_beamLorentzVec).M2(), 2)) / (4. * (_beamLorentzVec->Vect()).Mag2()));
			    (*_tPrimeAddr) = (*_tAddr) - (*_tMinAddr);
				return true;
			}

		  private:

			TLorentzVector* _xLorentzVec;
			TLorentzVector* _beamLorentzVec;
			double* _tAddr;
			double* _tMinAddr;
			double* _tPrimeAddr;

		};

		class Sum2 : public Function
		{

		  public:

			Sum2(std::vector<double*> inAddrs, double* outAddr)
				: _inAddrs(inAddrs),
				  _outAddr(outAddr) { }

			virtual ~Sum2() { }

			bool operator() () {
				(*_outAddr) = 0.;
				for(unsigned int i = 0; i < _inAddrs.size(); ++i) {
					(*_outAddr) += ((*_inAddrs[i]) * (*_inAddrs[i]));
				}
				(*_outAddr) = std::sqrt(*_outAddr);
				return true;
			}

		  private:

			std::vector<double*> _inAddrs;
			double* _outAddr;

		};

		class GetRpdPhi : public Function
		{

		  public:

			GetRpdPhi(TLorentzVector* beamLorentzVecAddr,
			          TLorentzVector* rpdProtonLorentzVecAddr,
			          TLorentzVector* xLorentzVecAddr,
			          double* rpdDeltaPhiAddr,
			          double* rpdDeltaPhiResAddr,
			          int method
			)
				: _beamLorentzVecAddr(beamLorentzVecAddr),
				  _rpdProtonLorentzVecAddr(rpdProtonLorentzVecAddr),
				  _xLorentzVecAddr(xLorentzVecAddr),
				  _rpdDeltaPhiAddr(rpdDeltaPhiAddr),
				  _rpdDeltaPhiResAddr(rpdDeltaPhiResAddr),
				  _method(method) { }

			virtual ~GetRpdPhi() { }

			bool operator() () {
				switch(_method)
				{
					case 0:
						antok::get_RPD_delta_phi_res_projection((*_beamLorentzVecAddr), (*_rpdProtonLorentzVecAddr), (*_xLorentzVecAddr), (*_rpdDeltaPhiAddr), (*_rpdDeltaPhiResAddr));
						return true;
					case 1:
						antok::get_RPD_delta_phi_res_rotation((*_beamLorentzVecAddr), (*_rpdProtonLorentzVecAddr), (*_xLorentzVecAddr), (*_rpdDeltaPhiAddr), (*_rpdDeltaPhiResAddr));
						return true;
				}
				return false;
			};

		  private:

			TLorentzVector* _beamLorentzVecAddr;
			TLorentzVector* _rpdProtonLorentzVecAddr;
			TLorentzVector* _xLorentzVecAddr;
			double* _rpdDeltaPhiAddr;
			double* _rpdDeltaPhiResAddr;
			int _method;

		};

		class Diff : public Function
		{

		  public:

			Diff(double* inAddr1, double* inAddr2, double* outAddr)
				: _inAddr1(inAddr1),
				  _inAddr2(inAddr2),
				  _outAddr(outAddr) { }

			virtual ~Diff() { }

			bool operator() () {
				(*_outAddr) = (*_inAddr1) - (*_inAddr2);
				return true;
			}

		  private:

			double* _inAddr1;
			double* _inAddr2;
			double* _outAddr;

		};

		class Abs : public Function
		{

		  public:

			Abs(double* inAddr, double* outAddr)
				: _inAddr(inAddr),
				  _outAddr(outAddr) { }

			virtual ~Abs() { }

			bool operator() () {
				(*_outAddr) = std::fabs(*_inAddr);
				return true;
			}

		  private:

			double* _inAddr;
			double* _outAddr;

		};

		class Energy : public Function
		{

		  public:

			Energy(TLorentzVector* inAddr, double* outAddr)
				: _inAddr(inAddr),
				  _outAddr(outAddr) { }

			virtual ~Energy() { }

			bool operator() () {
				(*_outAddr) = _inAddr->E();
				return true;
			}

		  private:

			TLorentzVector* _inAddr;
			double* _outAddr;

		};

		class RadToDegree : public Function
		{

		  public:

			RadToDegree(double* inAddr, double* outAddr)
				: _inAddr(inAddr),
				  _outAddr(outAddr) { }

			virtual ~RadToDegree() { }

			bool operator() () {
				(*_outAddr) = ((*_inAddr) / TMath::Pi()) * 180.;
				return true;
			}

		  private:

			double* _inAddr;
			double* _outAddr;

		};

		class ConvertIntToDouble : public Function
		{

		  public:

			ConvertIntToDouble(int* inAddr, double* outAddr)
				: _inAddr(inAddr),
				  _outAddr(outAddr) { }

			virtual ~ConvertIntToDouble() { }

			bool operator() () {
				(*_outAddr) = (*_inAddr);
				return true;
			}

		  private:

			int* _inAddr;
			double* _outAddr;

		};

	}

}

#endif
