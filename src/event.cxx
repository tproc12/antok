#include<event.h>

#include<basic_calcs.h>
#include<constants.h>
#include<functions.hpp>

antok::Event* antok::Event::_event = 0;

antok::Event* antok::Event::instance() {
	if(_event == 0) {
		_event = new antok::Event();
	}
	return _event;
}


antok::Event::Event() {

	_p.resize(antok::Constants::n_particles());

};

void antok::Event::update(antok::Data& data) {

//	const double& PION_MASS = antok::Constants::charged_pion_mass();

	rawData = &data;
/*
	_pSum.SetXYZT(0., 0., 0., 0.);
	for(unsigned int i = 0; i < _p.size(); ++i) {
		_p.at(i).SetXYZM(data.Mom_x.at(i), data.Mom_y.at(i), data.Mom_z.at(i), PION_MASS);
//		_p.at(i).Print();
		_pSum += _p.at(i);
	}

	_pProton.SetPxPyPzE(data.RPD_Px, data.RPD_Py, data.RPD_Pz, data.RPD_E);

	_p3Beam.SetXYZ(data.gradx, data.grady, 1.);
	_pBeam = antok::get_beam_energy(_p3Beam, _pSum);
	_p3Beam = _pBeam.Vect();

	_t = std::fabs((_pBeam - _pSum).Mag2());
	_tMin = std::fabs((std::pow(_pSum.M2() - _pBeam.M2(), 2)) / (4. * _p3Beam.Mag2()));
	_tPrime = _t - _tMin;

	antok::get_RPD_delta_phi_res(_pBeam, _pProton, _pSum, _RpdDeltaPhi, _RpdPhiRes);
	antok::get_RPD_delta_phi_res_fhaas(_pBeam, _pProton, _pSum, _RpdDeltaPhi_fhaas, _RpdPhiRes_fhaas);
*/
	for(unsigned int i = 0; i < _functions.size(); ++i) {
		assert((*_functions[i])());
	}

};

