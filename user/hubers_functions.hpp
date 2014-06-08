#ifndef ANTOK_USER_HUBERS_FUNCTIONS_HPP
#define ANTOK_USER_HUBERS_FUNCTIONS_HPP

#include<iostream>
#include<vector>
#include<algorithm>
#include<NNpoly.h>

#include<TLorentzVector.h>

namespace antok {

	namespace user {

		namespace hubers {

			namespace functions {

				class Sqrt : public Function
				{
					public:
						Sqrt(double* inAddr, double* outAddr)
							: _inAddr(inAddr),
							_outAddr(outAddr) { }

						virtual ~Sqrt() { }

						bool operator() () {
							if(*_inAddr>0)
								(*_outAddr) = std::sqrt(*_inAddr);
							else
								(*_outAddr) = -std::sqrt(-*_inAddr);
							return true;
						}

					private:
						double* _inAddr;
						double* _outAddr;
				};

				class Frac : public Function
				{
					public:
						Frac(double* inAddr1, double* inAddr2, double* outAddr)
							: _inAddr1(inAddr1),
							_inAddr2(inAddr2),
							_outAddr(outAddr) { }

						virtual ~Frac() { }

						bool operator() () {
							if(*_inAddr2!=0)
								(*_outAddr) = (*_inAddr1) / (*_inAddr2);
							else
								(*_outAddr) = (1<<31);
							return true;
						}

					private:
						double* _inAddr1;
						double* _inAddr2;
						double* _outAddr;
				};



				class GetPt : public Function
				{
					public:
						GetPt(TLorentzVector* pLorentzVec, TLorentzVector* beamLorentzVec, double *pTAddr)
							: _pLorentzVec(pLorentzVec),
							_beamLorentzVec(beamLorentzVec),
							_pTAddr(pTAddr){}

						virtual ~GetPt() { }

						bool operator() () {
							TVector3 beamVec = _beamLorentzVec->Vect();
							TVector3 pVec = _pLorentzVec->Vect();
							(*_pTAddr) = beamVec.Cross( pVec ) .Cross( beamVec ) .Unit() .Dot( pVec );
							return true;
						}

					private:
						TLorentzVector* _pLorentzVec;
						TLorentzVector* _beamLorentzVec;
						double* _pTAddr;
				};

				template<typename T>
				class EnforceEConservation : public Function
				{
					public:
						EnforceEConservation(T* beamAddr, T* pionAddr, T* gammaAddr,  T* outAddr) {
							_beamAddr = beamAddr;
							_pionAddr = pionAddr;
							_gammaAddr = gammaAddr;
							_outAddr = outAddr;
							_mode=0;

						};

						virtual ~EnforceEConservation() { }

						bool operator() () {
							if(_mode==0){
								const double E = _beamAddr->E()  - _pionAddr->E();
								TVector3 g3( _gammaAddr->Vect() );
								g3.SetMag(E);
								_outAddr->SetVect(g3);
								_outAddr->SetE(E);
							}
							else if(_mode==1){
								const double E = _beamAddr->E()  - _gammaAddr->E();
								TVector3 pi3( _pionAddr->Vect() );
								pi3.SetMag(sqrt(E*E- 0.13957018*0.13957018));
								_outAddr->SetVect(pi3);
								_outAddr->SetE(E);
							}
							return true;
						};


					private:

						T* _beamAddr;
						T* _pionAddr;
						T* _gammaAddr;
						T* _outAddr;
						int _mode;
				};

				//***********************************
				//Calculates the beam energy using a
				//polynomial represention from a
				//neuronal network
				//calibrated for primakoff 2009
				//***********************************
				class GetNeuronalBeam : public Function
				{
					public:
						GetNeuronalBeam(double* xAddr, double* yAddr, double* dxAddr,
						                double* dyAddr, double* eAddr, TLorentzVector* LVAddr)
						               :_xAddr(xAddr), _yAddr(yAddr), _dxAddr(dxAddr), _dyAddr(dyAddr),
						                _eAddr(eAddr), _LVAddr(LVAddr){}

						virtual ~GetNeuronalBeam() { }

						bool operator() () {
							double xarr[4]={*_xAddr,*_yAddr,*_dxAddr,*_dyAddr};
							if ( std::fabs(*_xAddr) > 1.8 || std::fabs(*_yAddr) > 1.8 || std::fabs(*_dxAddr) > 5e-4 || std::fabs(*_dyAddr+3e-4) > 5e-4 )
								*_eAddr = 190;
							else
								*_eAddr = NNpoly::Ebeam(xarr,NNpoly::getParams2009());
							TVector3 v3(*_dxAddr, *_dyAddr, std::sqrt( 1 - sqr(*_dxAddr) - sqr(*_dyAddr) ));
							v3.SetMag(*_eAddr);
							_LVAddr->SetXYZT(v3.X(),v3.Y(), v3.Z(), std::sqrt( sqr(*_eAddr) + sqr(antok::Constants::chargedPionMass())) );
							return true;
						}

					private:
						double* _xAddr;
						double* _yAddr;
						double* _dxAddr;
						double* _dyAddr;
						double* _eAddr;
						TLorentzVector* _LVAddr;
				};

				//***********************************
				//Calculates the angle theta between
				//two TLorentzVectors
				//***********************************
				class GetTheta : public Function
				{
					public:
						GetTheta(TLorentzVector* beamLVAddr, TLorentzVector* outLVAddr,
						         double* thetaAddr)
						        :_beamLVAddr(beamLVAddr), _outLVAddr(outLVAddr),
						         _thetaAddr(thetaAddr){}

						virtual ~GetTheta() { }

						bool operator() () {
							*_thetaAddr = _beamLVAddr->Vect().Angle( _outLVAddr->Vect() );
							return true;
						}

					private:
						TLorentzVector* _beamLVAddr;
						TLorentzVector* _outLVAddr;
						double* _thetaAddr;
				};

				//***********************************
				//Calculates the condition for a
				//theta dependend Z cut
				//***********************************
				class GetThetaZCut : public Function
				{
					public:
						GetThetaZCut(double* zAddr, double* thetaAddr,
						             double* zMeanAddr, int* passedAddr)
						            :_zAddr(zAddr), _thetaAddr(thetaAddr),
						             _zMeanAddr(zMeanAddr), _passedAddr(passedAddr){}

						virtual ~GetThetaZCut() { }

						bool operator() () {
							double fCUT_Z = -50;
							double fNsigma_theta=2.5;
							double fCUT_Z0 = 0.5;
							double fCUT_Z1 = 6.5;

							const double zmin = (*_zMeanAddr) - fNsigma_theta * ( fCUT_Z0 + fCUT_Z1/(*_thetaAddr*1000) );
							const double zmax = (*_zMeanAddr) + fNsigma_theta * ( fCUT_Z0 + fCUT_Z1/(*_thetaAddr*1000) );

							if( zmin < *_zAddr && *_zAddr < zmax && *_zAddr < fCUT_Z  )
								*_passedAddr=1;
							else
								*_passedAddr=0;

							return true;
						}

					private:
						TLorentzVector* _beamLVAddr;
						TLorentzVector* _outLVAddr;
						double* _zAddr;
						double* _thetaAddr;
						double* _zMeanAddr;
						int* _passedAddr;
				};

				//***********************************
				//returns 1 if a run is in a bad
				//spill list
				//***********************************
				class GetBadSpill : public Function
				{
					public:
						GetBadSpill(int* runAddr, int* spillAddr,
						            std::vector< std::pair<int,int> > *badSpillList,
						            int* result)
						           :_runAddr(runAddr), _spillAddr(spillAddr),
						            _badSpillList(badSpillList), _result(result) {
						            _prevRun=-100;
						            _prevSpill=-100;
												*_result=0;
						}

						virtual ~GetBadSpill() {}

						bool operator() () {
							if( (_prevRun != *_runAddr) || (_prevSpill != *_spillAddr) ){
								_prevRun   = *_runAddr;
								_prevSpill = *_spillAddr;
								for(unsigned int i=0; i<_badSpillList->size(); i++){
									if( (_prevRun=(*_badSpillList)[i].first) && (_prevSpill=(*_badSpillList)[i].second) ){
										*_result=1;
										break;
									}
									else {
										*_result=0;
									}
								}
							}
							return true;
						}
					private:
						int* _runAddr;
						int* _spillAddr;
						std::vector< std::pair<int,int> > *_badSpillList;
						int* _result;
						int _prevRun;
						int _prevSpill;
				};

				//***********************************
				//Shifts std::vectors
				//***********************************
				class GetShifted : public Function
				{
					public:
						GetShifted(std::vector<double>* VectorAddr, double* offsetAddr,
						           std::vector<double>* resultVec)
						          :_VectorAddr(VectorAddr), _offsetAddr(offsetAddr),
						           _resultVec(resultVec) {}

						virtual ~GetShifted() {}

						bool operator() () {
							_resultVec->clear();
							for(unsigned int i = 0; i < _VectorAddr->size(); ++i) {
								_resultVec->push_back((*_VectorAddr)[i] + *_offsetAddr);
							}
							return true;

						}
					private:
						std::vector<double>* _VectorAddr;
						double* _offsetAddr;
						std::vector<double>* _resultVec;
				};

				//***********************************
				// Scale Energy of a cluster depending
				// on energy and position
				//***********************************
				class GetScaledCluster : public Function
				{
					public:
						GetScaledCluster(std::vector<double>* XAddr, std::vector<double>* YAddr,
						                 std::vector<double>* EAddr, int* method, double* threshold,
						                 std::vector<double>* resultAddr)
						                :_XAddr(XAddr),_YAddr(YAddr),_EAddr(EAddr),
						                 _method(method), _threshold(threshold), _resultAddr(resultAddr) {}

						virtual ~GetScaledCluster() {}

						bool operator() () {
							_resultAddr->clear();
							for(unsigned int i=0; i<(_XAddr->size()); ++i){
								if((*_EAddr)[i] < *_threshold)
									_resultAddr->push_back( (*_EAddr)[i] );
								else if((*_method)==1)
									_resultAddr->push_back( LinearGammaCorrection((*_EAddr)[i]) );
								else if((*_method)==0)
									_resultAddr->push_back( PEDepGammaCorrection((*_EAddr)[i], (*_XAddr)[i], (*_YAddr)[i]) );
								else{
									std::cerr<<__func__<<" wrong method specified."<<std::endl;
									return 0;
								}
							}
							return 1;
						}

					private:
						std::vector<double> *_XAddr;
						std::vector<double> *_YAddr;
						std::vector<double>	*_EAddr;
						int* _method;
						double* _threshold;
						std::vector<double>* _resultAddr;
				};

				//***********************************
				//cleanes up calorimeter clusters
				//and merges them dependend on their distance
				//***********************************
				class GetCleanedClusters : public Function
				{
					public:
						GetCleanedClusters(std::vector<double>* VectorXAddr, std::vector<double>* VectorYAddr, std::vector<double>* VectorZAddr,
						                  std::vector<double>* VectorTAddr, std::vector<double>* VectorEAddr,
						                  double* trackX, double* trackY, double* trackT, double* mergeDist, double*  timeThreshold,
						                  std::vector<double>* resultVecX, std::vector<double>* resultVecY, std::vector<double>* resultVecZ,
						                  std::vector<double>* resultVecT, std::vector<double>* resultVecE)
						                 :_VectorXAddr(VectorXAddr), _VectorYAddr(VectorYAddr), _VectorZAddr(VectorZAddr),
						                  _VectorTAddr(VectorTAddr), _VectorEAddr(VectorEAddr),
						                  _trackX(trackX), _trackY(trackY), _trackT(trackT),
						                  _mergeDist(mergeDist), _timeThreshold(timeThreshold),
						                  _resultVecX(resultVecX), _resultVecY(resultVecY), _resultVecZ(resultVecZ),
						                  _resultVecT(resultVecT), _resultVecE(resultVecE) {}

						virtual ~GetCleanedClusters() {}

						bool operator() () {


							_resultVecE->clear(); _resultVecX->clear(); _resultVecY->clear(); _resultVecZ->clear(); _resultVecT->clear();
							_maximumE = -999.;
							int imax = -999;
							int newCnt = -1;
							for(unsigned int i=0; i<_VectorXAddr->size();++i){
								double dT=fabs(((*_VectorTAddr)[i]-(*_trackT)));
								if( *_trackT<1e9 && (std::fabs(dT) > *_timeThreshold) )
									continue;
								double dist=std::sqrt( antok::sqr(*_trackX-(*_VectorXAddr)[i]) +  antok::sqr(*_trackY-(*_VectorYAddr)[i])  );
								if( dist < (3.+16./ (*_VectorEAddr)[i]) )
									continue;
								_resultVecE->push_back((*_VectorEAddr)[i]); _resultVecX->push_back((*_VectorXAddr)[i]);
								_resultVecY->push_back((*_VectorYAddr)[i]); _resultVecZ->push_back((*_VectorZAddr)[i]);
								_resultVecT->push_back((*_VectorTAddr)[i]);
								newCnt++;
								if(((*_VectorEAddr)[i]) < (_maximumE))
									continue;
								_maximumE=(*_VectorEAddr)[i];
								imax=newCnt;
							}

							if(imax==-999){
								_maximumE=-999;
								return true;
							}

							std::swap((*_resultVecX)[imax],(*_resultVecX)[0]);
							std::swap((*_resultVecY)[imax],(*_resultVecY)[0]);
							std::swap((*_resultVecZ)[imax],(*_resultVecZ)[0]);
							std::swap((*_resultVecE)[imax],(*_resultVecE)[0]);
							std::swap((*_resultVecT)[imax],(*_resultVecT)[0]);

							imax=0;

							int nClusters=_resultVecX->size();
							if(nClusters==0)
								return true;

							double closest, eMax=-99;
							do {
								closest = antok::sqr(*_mergeDist)+0.1;
								int m2=-1;
								for(int i=0; i<nClusters; ++i){
									if(i==imax)
										continue;
									double dist = ( antok::sqr((*_resultVecX)[i]-(*_resultVecX)[imax]) + antok::sqr((*_resultVecY)[i]-(*_resultVecY)[imax]) );
									if( dist < antok::sqr(*_mergeDist) ){
										if((*_resultVecE)[i]>eMax){
											eMax = (*_resultVecE)[i];
											closest = dist;
											m2 = i;
										}
									}
								}
								//-------------------------------------------------------------
								if( closest < antok::sqr(*_mergeDist) ){
									const double Esum = (*_resultVecE)[imax] + (*_resultVecE)[m2];
									(*_resultVecX)[imax] = ( ((*_resultVecX)[imax] * (*_resultVecE)[imax]) + ((*_resultVecX)[m2] *  (*_resultVecE)[m2]) ) / Esum;
									(*_resultVecY)[imax] = ( ((*_resultVecY)[imax] * (*_resultVecE)[imax]) + ((*_resultVecY)[m2] *  (*_resultVecE)[m2]) ) / Esum;
									(*_resultVecZ)[imax] = ( ((*_resultVecZ)[imax] * (*_resultVecE)[imax]) + ((*_resultVecZ)[m2] *  (*_resultVecE)[m2]) ) / Esum;
									(*_resultVecT)[imax] = ( ((*_resultVecT)[imax] * (*_resultVecE)[imax]) + ((*_resultVecT)[m2] *  (*_resultVecE)[m2]) ) / Esum;
									(*_resultVecE)[imax] = Esum;
									--nClusters;
									(*_resultVecE)[m2]=(*_resultVecE)[nClusters]; (*_resultVecX)[m2]=(*_resultVecX)[nClusters];
									(*_resultVecY)[m2]=(*_resultVecY)[nClusters]; (*_resultVecZ)[m2]=(*_resultVecZ)[nClusters];
									(*_resultVecT)[m2]=(*_resultVecT)[nClusters];
								}
							} while(closest <  antok::sqr(*_mergeDist) );
							_resultVecX->resize(nClusters);
							_resultVecY->resize(nClusters);
							_resultVecZ->resize(nClusters);
							_resultVecT->resize(nClusters);
							_resultVecE->resize(nClusters);

							return true;


						}
					private:
						std::vector<double>* _VectorXAddr;
						std::vector<double>* _VectorYAddr;
						std::vector<double>* _VectorZAddr;
						std::vector<double>* _VectorTAddr;
						std::vector<double>* _VectorEAddr;
						double* _trackX;
						double* _trackY;
						double* _trackT;
						double* _mergeDist;
						double* _timeThreshold;
						std::vector<double>* _resultVecX;
						std::vector<double>* _resultVecY;
						std::vector<double>* _resultVecZ;
						std::vector<double>* _resultVecT;
						std::vector<double>* _resultVecE;
						double _maximumE;
				};

				//***********************************
				//gets highest energetic calorimeter cluster
				//***********************************
				class GetMaximumCluster : public Function
				{
					public:
						GetMaximumCluster(std::vector<double>* VectorXAddr, std::vector<double>* VectorYAddr, std::vector<double>* VectorZAddr,
						                  std::vector<double>* VectorTAddr, std::vector<double>* VectorEAddr,
						                  double* trackX, double* trackY, double* trackT,
						                  double* maximumX, double* maximumY, double* maximumZ, double* maximumT,
						                  double* maximumE, int*  NClus)
						                 :_VectorXAddr(VectorXAddr), _VectorYAddr(VectorYAddr), _VectorZAddr(VectorZAddr),
						                  _VectorTAddr(VectorTAddr), _VectorEAddr(VectorEAddr),
						                  _trackX(trackX), _trackY(trackY), _trackT(trackT),
						                  _maximumX(maximumX), _maximumY(maximumY), _maximumZ(maximumZ), _maximumT(maximumT),
						                  _maximumE(maximumE), _NClus(NClus) {}

						virtual ~GetMaximumCluster(){}

						bool operator() () {
							double eMax = -99;
							int iMax = -99;
							*_NClus = 0;
							for(unsigned int i = 0; i < _VectorXAddr->size(); ++i){
								if(_VectorEAddr->at(i) < 2.)
									continue;
								(*_NClus)++;
								if(eMax < (*_VectorEAddr)[i]){
									eMax = (*_VectorEAddr)[i];
									iMax = i;
								}
							}
							*_maximumX = (*_VectorXAddr)[iMax];
							*_maximumY = (*_VectorYAddr)[iMax];
							*_maximumZ = (*_VectorZAddr)[iMax];
							*_maximumT = (*_VectorTAddr)[iMax];
							if(iMax >= 0)
								*_maximumE = (*_VectorEAddr)[iMax];
							else
								*_maximumE = -99;

							return true;
						}

					private:
						std::vector<double>* _VectorXAddr;
						std::vector<double>* _VectorYAddr;
						std::vector<double>* _VectorZAddr;
						std::vector<double>* _VectorTAddr;
						std::vector<double>* _VectorEAddr;
						double* _trackX;
						double* _trackY;
						double* _trackT;
						double* _maximumX;
						double* _maximumY;
						double* _maximumZ;
						double* _maximumT;
						double* _maximumE;
						int* _NClus;
				};




				//***********************************
				//gets LorentzVector for cluster
				//produced in a  vertex with coordinates X/Y/Z
				//***********************************
				class GetNeutralLorentzVec : public Function
				{
					public:
						GetNeutralLorentzVec(double* xAddr, double* yAddr,
						                     double* zAddr, double* eAddr,
						                     double* xPVAddr, double* yPVAddr,
						                     double* zPVAddr, TLorentzVector* resultAddr)
						                    :_xAddr(xAddr), _yAddr(yAddr),
						                     _zAddr(zAddr), _eAddr(eAddr),
						                     _xPVAddr(xPVAddr), _yPVAddr(yPVAddr),
						                     _zPVAddr(zPVAddr), _resultAddr(resultAddr) {}

						virtual ~GetNeutralLorentzVec() {}

						bool operator() () {
							TVector3 v3( (*_xAddr-*_xPVAddr), (*_yAddr-*_yPVAddr), (*_zAddr-*_zPVAddr));
							v3.SetMag(*_eAddr);
							_resultAddr->SetXYZT(v3.X(), v3.Y(), v3.Z(), *_eAddr);
							return 1;

						}
					private:
						double *_xAddr;
						double *_yAddr;
						double *_zAddr;
						double *_eAddr;
						double *_xPVAddr;
						double *_yPVAddr;
						double *_zPVAddr;
						TLorentzVector *_resultAddr;
				};

			}

		}

	}

}
#endif
