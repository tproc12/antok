
#include<sstream>

#include<TFile.h>
#include<TTree.h>

#include<constants.h>
#include<cutter.h>
#include<data.h>
#include<event.h>
#include<functions.hpp>
#include<generators_cuts.h>
#include<generators_functions.h>
#include<initializer.h>
#include<object_manager.h>
#include<plotter.h>

antok::Initializer* antok::Initializer::_initializer = 0;

antok::Initializer* antok::Initializer::instance() {

	if(_initializer == 0) {
		_initializer = new antok::Initializer();
	};
	return _initializer;

};

antok::Initializer::Initializer()
	: _config(0)
{

};

bool antok::Initializer::readConfigFile(const std::string& filename) {

	if(_config != 0) {
		std::cerr<<"Attempting to read config file twice."<<std::endl;
		return false;
	}

	_config = new YAML::Node();
	YAML::Node& config = *_config;

	// Load the config file
	try {
		config = YAML::LoadFile(filename);
	} catch (YAML::ParserException e) {
		std::cerr<<"Error parsing config file: "<<e.what()<<"."<<std::endl;
		return false;
	}

	// Number of particles
	try {
		if(not (config["NumberOfParticles"] and antok::Constants::set_n_particles(config["NumberOfParticles"].as<unsigned int>()))) {
			std::cerr<<"Could not set number of particles."<<std::endl;
			return false;
		}
	} catch (YAML::TypedBadConversion<int> e) {
		std::cerr<<"Conversion error when reading \"NumberOfParticles\": "<<e.what()<<"."<<std::endl;
		return false;
	}

	// Get the constants
	YAML::Node constants = config["Constants"];
	try {
		if(not (constants["ChargedKaonMass"] and antok::Constants::set_charged_kaon_mass(constants["ChargedKaonMass"].as<double>()))) {
			std::cerr<<"Could not set charged kaon mass."<<std::endl;
			return false;
		}
		if(not (constants["ChargedPionMass"] and antok::Constants::set_charged_pion_mass(constants["ChargedPionMass"].as<double>()))) {
			std::cerr<<"Could not set pion mass."<<std::endl;
			return false;
		}
		if(not (constants["ProtonMass"] and antok::Constants::set_proton_mass(constants["ProtonMass"].as<double>()))) {
			std::cerr<<"Coud not set proton mass."<<std::endl;
			return false;
		}
	} catch (YAML::TypedBadConversion<double> e) {
		std::cerr<<"Conversion error when reading constants: "<<e.what()<<"."<<std::endl;
		return false;
	}

	antok::Constants::_initialized = true;

	return true;

};

bool antok::Initializer::initializeCutter() {

	if(_config == 0) {
		std::cerr<<"Trying to initialize Cutter without having read the config file first."<<std::endl;
		return false;
	}

	antok::ObjectManager* objectManager = antok::ObjectManager::instance();

	if(objectManager->_cutter != 0) {
		std::cerr<<"Cutter seems to be initialized already."<<std::endl;
		return false;
	}
	objectManager->_cutter = antok::Cutter::instance();
	antok::Cutter& cutter = objectManager->getCutter();
	YAML::Node& config = (*_config);

	if(not config["Cuts"]) {
		std::cerr<<"Could not find section \"Cuts\" in configuration file."<<std::endl;
		return false;
	}

	std::vector<antok::Cut*>* cutVector = new std::vector<antok::Cut*>();
	for(YAML::const_iterator cut_it = config["Cuts"].begin(); cut_it != config["Cuts"].end(); cut_it++) {

		antok::Cut* antokCut = antok::generators::generateCut(*cut_it);
		if(antokCut == 0) {
			return false;
		}
		cutVector->push_back(antokCut);
	}
	cutter._cuts = (*cutVector);

	return true;

};

bool antok::Initializer::initializeData() {

	antok::ObjectManager* objectManager = antok::ObjectManager::instance();

	if(objectManager->_data != 0) {
		std::cerr<<"Data seems to be initialized already."<<std::endl;
		return false;
	}
	objectManager->_data = new antok::Data();
	antok::Data& data = objectManager->getData();

	if(_config == 0) {
		std::cerr<<"Cannot create data object without reading a config file first."<<std::endl;
		return false;
	};
	YAML::Node& config = *_config;

	if(not config["TreeBranches"]) {
		std::cerr<<"TreeBranches not found in configuration file."<<std::endl;
		return false;
	};
	if(not config["TreeBranches"]["onePerEvent"] or not config["TreeBranches"]["onePerParticle"]) {
		std::cerr<<"TreeBranches[{\"onePerEvent\"|\"onePerParticle\"}] not found in configuration file."<<std::endl;
		return false;
	}

	// Get all the branches in the tree and fill the data maps
	YAML::Node perEventTreeBranches= config["TreeBranches"]["onePerEvent"];
	YAML::Node perParticleTreeBranches= config["TreeBranches"]["onePerParticle"];
	for(YAML::const_iterator typeIt = perEventTreeBranches.begin(); typeIt != perEventTreeBranches.end(); ++typeIt) {
		for(YAML::const_iterator valIt = typeIt->second.begin(); valIt != typeIt->second.end(); ++valIt) {
			std::string type = getYAMLStringSafe(typeIt->first);
			std::string name = getYAMLStringSafe(*valIt);
			if(name == "") {
				std::cerr<<"Conversion to std::string failed for one of the \"TreeBranches\"' \"onePerEvent\" "<<type<<"s."<<std::endl;
				return false;
			}
			if(type == "double") {
				if(not data.insertDouble(name)) {
					std::cerr<<antok::Data::getVariableInsertionErrorMsg(name);
					return false;
				}
			} else if(type == "int") {
				if(not data.insertInt(name)) {
					std::cerr<<antok::Data::getVariableInsertionErrorMsg(name);
					return false;
				}
			} else if(type == "Long64_t") {
				if(not data.insertLong64_t(name)) {
					std::cerr<<antok::Data::getVariableInsertionErrorMsg(name);
					return false;
				}
			} else if(type == "") {
				std::cerr<<"Could not convert branch type to string when parsing the \"TreeBranches\"' \"onePerEvent\" part."<<std::endl;
				return false;
			} else {
				std::cerr<<"Data type \""<<type<<"\" not supported."<<std::endl;
				return false;
			}
		}
	}
	const unsigned int& N_PARTICLES = antok::Constants::n_particles();
	for(YAML::const_iterator typeIt = perParticleTreeBranches.begin(); typeIt != perParticleTreeBranches.end(); ++typeIt) {
		for(YAML::const_iterator valIt = typeIt->second.begin(); valIt != typeIt->second.end(); ++valIt) {
			std::string type = getYAMLStringSafe(typeIt->first);
			std::string baseName = getYAMLStringSafe(*valIt);
			if(baseName == "") {
				std::cerr<<"Conversion to std::string failed for one of the \"TreeBranches\"' \"onePerParticle\" "<<type<<"s."<<std::endl;
				return false;
			}

			if(type == "double") {
				for(unsigned int i = 0; i < N_PARTICLES; ++i) {
					std::stringstream strStr;
					strStr<<baseName<<(i+1);
					if(not data.insertDouble(strStr.str())) {
						std::cerr<<antok::Data::getVariableInsertionErrorMsg(strStr.str());
						return false;
					}
				}
			} else if(type == "int") {
				for(unsigned int i = 0; i < N_PARTICLES; ++i) {
					std::stringstream strStr;
					strStr<<baseName<<(i+1);
					if(not data.insertInt(strStr.str())) {
						std::cerr<<antok::Data::getVariableInsertionErrorMsg(strStr.str());
						return false;
					}
				}
			} else if(type == "") {
				std::cerr<<"Could not convert branch type to std::string when parsing the \"TreeBranches\"' \"onePerParticle\" part."<<std::endl;
				return false;
			} else {
				std::cerr<<"Data type \""<<type<<"\" not supported."<<std::endl;
				return false;
			}
		}
	}

	// Set the branch addresses of the tree
	if(not config["TreeName"]) {
		std::cerr<<"\"TreeName\" not found in configuration file."<<std::endl;
		return false;
	}
	TFile* inFile = objectManager->getInFile();
	std::string treeName = getYAMLStringSafe(config["TreeName"]);
	if(treeName == "") {
		std::cerr<<"Could not convert entry \"TreeName\" to std::string."<<std::endl;
		return false;
	}
	TTree* inTree = dynamic_cast<TTree*>(inFile->Get(treeName.c_str()));
	if(inTree == 0) {
		std::cerr<<"Could not open input TTree."<<std::endl;
		return false;
	}
	objectManager->_inTree = inTree;

	for(std::map<std::string, double>::iterator it = data.doubles.begin(); it != data.doubles.end(); ++it) {
		inTree->SetBranchAddress(it->first.c_str(), &(it->second));
	}
	for(std::map<std::string, int>::iterator it = data.ints.begin(); it != data.ints.end(); ++it) {
		inTree->SetBranchAddress(it->first.c_str(), &(it->second));
	}
	for(std::map<std::string, Long64_t>::iterator it = data.long64_ts.begin(); it != data.long64_ts.end(); ++it) {
		inTree->SetBranchAddress(it->first.c_str(), &(it->second));
	}

	return true;

}

bool antok::Initializer::initializeEvent() {

	if(_config == 0) {
		std::cerr<<"Trying to initialize Cutter without having read the config file first."<<std::endl;
		return false;
	}

	antok::ObjectManager* objectManager = antok::ObjectManager::instance();

	if(objectManager->_event != 0) {
		std::cerr<<"Event seems to be initialized already."<<std::endl;
		return false;
	}
	objectManager->_event = antok::Event::instance();
	YAML::Node& config = *_config;

	if(not config["CalculatedQuantities"]) {
		std::cerr<<"Warning: \"CalculatedQuantities\" not found in configuration file."<<std::endl;
	}
	for(YAML::const_iterator calcQuantity_it = config["CalculatedQuantities"].begin(); calcQuantity_it != config["CalculatedQuantities"].end(); calcQuantity_it++) {

		YAML::Node calcQuantity = (*calcQuantity_it);
		std::vector<std::string> quantityBaseNames;
		if(calcQuantity["Name"].IsSequence()) {
			try {
				quantityBaseNames = calcQuantity["Name"].as<std::vector<std::string> >();
			} catch (YAML::TypedBadConversion<std::vector<std::string> > e) {
				std::cerr<<"Could not convert YAML sequence to std::vector<std::string> when parsing \"CalculatedQuantities\"' \"Name\""<<std::endl;
				return false;
			} catch (YAML::TypedBadConversion<std::string> e) {
				std::cerr<<"Could not entries in YAML sequence to std::string when parsing \"CalculatedQuantities\"' \"Name\""<<std::endl;
				return false;
			}
		} else {
			std::string baseName = getYAMLStringSafe(calcQuantity["Name"]);
			if(baseName == "") {
				std::cerr<<"Could not convert one of the \"CalculatedQuantities\"' \"Name\"s to std::string."<<std::endl;
				return false;
			}
			quantityBaseNames.push_back(baseName);
		}
		if(quantityBaseNames.size() < 1) {
			std::cerr<<"Did not find a name to save calculated quantity to."<<std::endl;
			return false;
		}

		if(not (calcQuantity["Function"] and calcQuantity["Function"]["Name"])) {
			std::cerr<<"No Function or no function name for calculated quantity \""<<quantityBaseNames[0]<<"\"."<<std::endl;
			return false;
		}

		std::vector<int> indices;
		if(calcQuantity["Indices"]) {
			try {
				indices = calcQuantity["Indices"].as<std::vector<int> >();
			} catch (YAML::TypedBadConversion<std::vector<int> > e) {
				std::cerr<<"Could not convert YAML sequence to std::vector<int> when parsing CalculatedQuantities' \""<<quantityBaseNames[0]<<"\" \"Indices\"."<<std::endl;
				return false;
			} catch (YAML::TypedBadConversion<int> e) {
				std::cerr<<"Could not convert entries in YAML sequence to int when parsing CalculatedQuantities' \""<<quantityBaseNames[0]<<"\" \"Indices\"."<<std::endl;
				return false;
			}
		} else {
			indices.push_back(-1);
		}

		const YAML::Node& function = calcQuantity["Function"];
		std::string functionName = getYAMLStringSafe(function["Name"]);

		for(unsigned int indices_i = 0; indices_i < indices.size(); ++indices_i) {

			std::vector<std::string> quantityNames;
			for(unsigned int baseName_i = 0; baseName_i < quantityBaseNames.size(); ++baseName_i) {
				if(indices[indices_i] > 0) {
					std::stringstream strStr;
					strStr<<quantityBaseNames[baseName_i]<<indices[indices_i];
					quantityNames.push_back(strStr.str());
				} else {
					quantityNames.push_back(quantityBaseNames[baseName_i]);
				}
			}

			antok::Function* antokFunctionPtr = 0;
			if(functionName == "abs") {
				antokFunctionPtr = antok::generators::generateAbs(function, quantityNames, indices[indices_i]);
			} else if(functionName == "diff") {
				antokFunctionPtr = antok::generators::generateDiff(function, quantityNames, indices[indices_i]);
			} else if(functionName == "getBeamLorentzVector") {
				antokFunctionPtr = antok::generators::generateGetBeamLorentzVector(function, quantityNames, indices[indices_i]);
			} else if(functionName == "getLorentzVec") {
				antokFunctionPtr = antok::generators::generateGetLorentzVec(function, quantityNames, indices[indices_i]);
			} else if(functionName == "getRpdPhi") {
				antokFunctionPtr = antok::generators::generateGetRpdPhi(function, quantityNames, indices[indices_i]);
			} else if(functionName == "getTs") {
				antokFunctionPtr = antok::generators::generateGetTs(function, quantityNames, indices[indices_i]);
			} else if(functionName == "mass") {
				antokFunctionPtr = antok::generators::generateMass(function, quantityNames, indices[indices_i]);
			} else if(functionName == "sum") {
				antokFunctionPtr = antok::generators::generateSum(function, quantityNames, indices[indices_i]);
			} else if(functionName == "sum2") {
				antokFunctionPtr= antok::generators::generateSum2(function, quantityNames, indices[indices_i]);
			} else if(functionName == "") {
				std::cerr<<"Could not convert function name to std::string for CalculatedQuantity \""<<quantityNames[0]<<"\"."<<std::endl;
				return false;
			} else {
				std::cerr<<"Function type \""<<functionName<<"\" not supported."<<std::endl;
				return false;
			}
			if(antokFunctionPtr == 0) {
				return false;
			}
			antok::Event& event = antok::ObjectManager::instance()->getEvent();
			event._functions.push_back(antokFunctionPtr);

		} // End of loop over all indices

	} // End of loop over all CalculatedQuantities

	return true;

};

bool antok::Initializer::initializePlotter() {

	antok::ObjectManager* objectManager = antok::ObjectManager::instance();

	if(objectManager->_plotter == 0) {
		objectManager->_plotter = antok::Plotter::instance();
	}
	return true;

};

std::string antok::Initializer::getYAMLStringSafe(const YAML::Node& node) {
	try{
		return node.as<std::string>();
	} catch(YAML::TypedBadConversion<std::string> e) {
		return "";
	}
}

double* antok::Initializer::getYAMLDoubleAddress(const YAML::Node& node) {

	double* retval = 0;
	try {
		double val = node.as<double>();
		retval = new double(val);
	} catch (YAML::TypedBadConversion<double> e) {
		// not bad yet, could be a variable name there
	}
	if(retval == 0) {
		antok::Data& data = antok::ObjectManager::instance()->getData();
		std::string name = antok::Initializer::getYAMLStringSafe(node);
		if(name == "") {
			std::cerr<<"Entry has to be either a variable name or of type double."<<std::endl;
			return 0;
		}
		retval = data.getDoubleAddr(name);
		if(retval == 0) {
			std::cerr<<"Variable \""<<name<<"\" not found in Data."<<std::endl;
			return 0;
		}
	}
	return retval;

};

