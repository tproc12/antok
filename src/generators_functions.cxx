#include<generators_functions.h>

#include<assert.h>

#include<TLorentzVector.h>

#include<data.h>
#include<functions.hpp>
#include<initializer.h>
#include<object_manager.h>
#include<yaml_utils.hpp>

bool antok::generators::functionArgumentHandler(std::vector<std::pair<std::string, std::string> >& args,
                                                const YAML::Node& function,
                                                int index,
                                                bool argStringsAlreadyValues)
{

	using antok::YAMLUtils::hasNodeKey;

	antok::Data& data = antok::ObjectManager::instance()->getData();
	for(unsigned int i = 0; i < args.size(); ++i) {
		std::string& argName = args[i].first;
		if(not argStringsAlreadyValues) {
			if(not hasNodeKey(function, argName)) {
				std::cerr<<"Argument \""<<argName<<"\" not found (required for function \""<<function["Name"]<<"\")."<<std::endl;
				return false;
			}
			argName = antok::YAMLUtils::getString(function[argName]);
			if(argName == "") {
				std::cerr<<"Could not convert one of the arguments to std::string in function \""<<function["Name"]<<"\"."<<std::endl;
				return false;
			}
		}
		if(index > 0) {
			std::stringstream strStr;
			strStr<<argName<<index;
			argName = strStr.str();
		}
		std::string type = data.getType(argName);
		if(type == "") {
			std::cerr<<"Argument \""<<argName<<"\" not found in Data's global map."<<std::endl;
			return false;
		}
		if(type != args[i].second) {
			std::cerr<<"Argument \""<<argName<<"\" has type \""<<type<<"\", expected \""<<args[i].second<<"\"."<<std::endl;
			return false;
		}
	}

	return true;

};

std::string antok::generators::getFunctionArgumentHandlerErrorMsg(std::vector<std::string> quantityNames) {
	std::stringstream msgStream;
	if(quantityNames.size() > 1) {
		msgStream<<"Error when registering calculation for quantities \"[";
		for(unsigned int i = 0; i < quantityNames.size()-1; ++i) {
			msgStream<<quantityNames[i]<<", ";
		}
		msgStream<<quantityNames[quantityNames.size()-1]<<"]\"."<<std::endl;
	} else {
		msgStream<<"Error when registering calculation for quantity \""<<quantityNames[0]<<"\"."<<std::endl;
	}
	return msgStream.str();
};

namespace {

	template<typename T>
	antok::Function* __getSumFunction(std::vector<std::pair<std::string, std::string> >& summandNames,
	                                  std::vector<std::pair<std::string, std::string> >& subtrahendsNames,
	                                  std::string quantityName) {

		std::vector<T*> inputAddrsSummands;
		std::vector<T*> inputAddrsSubtrahends;
		antok::Data& data = antok::ObjectManager::instance()->getData();

		// Now do type checking and get all the addresses
		// for summands
		for(unsigned int summandNames_i = 0; summandNames_i < summandNames.size(); ++summandNames_i) {

			std::string variableName = summandNames[summandNames_i].first;
			inputAddrsSummands.push_back(data.getAddr<T>(variableName));

		}
		// for subtrahends
		for(unsigned int subtrahendsNames_i = 0; subtrahendsNames_i < subtrahendsNames.size(); ++subtrahendsNames_i) {

			std::string variableName = subtrahendsNames[subtrahendsNames_i].first;
			inputAddrsSubtrahends.push_back(data.getAddr<T>(variableName));

		}

		// And produce the function
		if(not data.insert<T>(quantityName)) {
			std::cerr<<antok::Data::getVariableInsertionErrorMsg(quantityName);
			return 0;
		}
		return (new antok::functions::Sum<T>(inputAddrsSummands, inputAddrsSubtrahends, data.getAddr<T>(quantityName)));

	};

	std::vector<std::pair<std::string, std::string> >* __getSummandNames(const YAML::Node& function, std::string& quantityName, int index, std::string type) {

		using antok::YAMLUtils::hasNodeKey;

		std::string typeName = "notInitialized";
		std::vector<std::pair<std::string, std::string> >* summandNames = new std::vector<std::pair<std::string, std::string> >();

		// Summing over one variable with indices
		if(not hasNodeKey(function, type.c_str())) {
			return 0;
		}

		antok::Data& data = antok::ObjectManager::instance()->getData();

		if(hasNodeKey(function[type.c_str()], "Indices") or hasNodeKey(function[type.c_str()], "Name")) {
			if(not (hasNodeKey(function[type.c_str()], "Indices") and hasNodeKey(function[type.c_str()], "Name"))) {
				std::cerr<<"Either \"Indices\" or \"Name\" found in sum function, but not both (Variable: \""<<quantityName<<"\")."<<std::endl;
				return 0;
			}
			if(index > 0) {
				std::cerr<<"Cannot have sum over indices for every particle (Variable: \""<<quantityName<<"\")."<<std::endl;
				return 0;
			}
			std::vector<int> inner_indices;
			try {
				inner_indices = function[type.c_str()]["Indices"].as<std::vector<int> >();
			} catch (const YAML::TypedBadConversion<std::vector<int> >& e) {
				std::cerr<<"Could not convert YAML sequence to std::vector<int> when parsing \"sum\"' \"Indices\" (for variable \""<<quantityName<<"\")."<<std::endl;
				return 0;
			} catch (const YAML::TypedBadConversion<int>& e) {
				std::cerr<<"Could not convert entries in YAML sequence to int when parsing \"sum\"' \"Indices\" (for variable \""<<quantityName<<"\")."<<std::endl;
				return 0;
			}
			typeName = antok::YAMLUtils::getString(function[type.c_str()]["Name"]);
			if(typeName == "") {
				std::cerr<<"Could not convert \"Summands\"' \"Name\" to std::string when registering calculation of \""<<quantityName<<"\"."<<std::endl;
			}
			std::string summandBaseName = typeName;
			std::stringstream strStr;
			strStr<<typeName<<inner_indices[0];
			typeName = data.getType(strStr.str());
			for(unsigned int inner_indices_i = 0; inner_indices_i < inner_indices.size(); ++inner_indices_i) {
				int inner_index = inner_indices[inner_indices_i];
				std::stringstream strStr;
				strStr<<summandBaseName<<inner_index;
				summandNames->push_back(std::pair<std::string, std::string>(strStr.str(), typeName));
			}
			// Summing over list of variable names
		} else {
			typeName = antok::YAMLUtils::getString(*function[type.c_str()].begin());
			if(typeName == "") {
				std::cerr<<"Could not convert one of the \"Summands\" to std::string when registering calculation of \""<<quantityName<<"\"."<<std::endl;
				return 0;
			}
			if(index > 0) {
				std::stringstream strStr;
				strStr<<typeName<<index;
				typeName = strStr.str();
			}
			typeName = data.getType(typeName);
			for(YAML::const_iterator summand_it = function[type.c_str()].begin(); summand_it != function[type.c_str()].end(); ++summand_it) {
				std::string variableName = antok::YAMLUtils::getString(*summand_it);
				if(variableName == "") {
					std::cerr<<"Could not convert one of the \"Summands\" to std::string when registering calculation of \""<<quantityName<<"\"."<<std::endl;
					return 0;
				}
				summandNames->push_back(std::pair<std::string, std::string>(variableName, typeName));
			}
		}
		return summandNames;

	};

}

antok::Function* antok::generators::generateAbs(const YAML::Node& function, std::vector<std::string>& quantityNames, int index)
{

	if(quantityNames.size() > 1) {
		std::cerr<<"Too many names for function \""<<function["Name"]<<"\"."<<std::endl;
		return 0;
	}
	std::string quantityName = quantityNames[0];

	std::vector<std::pair<std::string, std::string> > args;
	args.push_back(std::pair<std::string, std::string>("Arg", "double"));

	if(not antok::generators::functionArgumentHandler(args, function, index)) {
		std::cerr<<antok::generators::getFunctionArgumentHandlerErrorMsg(quantityNames);
		return 0;
	}

	antok::Data& data = antok::ObjectManager::instance()->getData();

	double* argAddr = data.getAddr<double>(args[0].first);

	if(not data.insert<double>(quantityName)) {
		std::cerr<<antok::Data::getVariableInsertionErrorMsg(quantityNames);
		return 0;
	}

	return (new antok::functions::Abs(argAddr, data.getAddr<double>(quantityName)));

};

antok::Function* antok::generators::generateConvertIntToDouble(const YAML::Node& function, std::vector<std::string>& quantityNames, int index)
{

	if(quantityNames.size() > 1) {
		std::cerr<<"Too many names for function \""<<function["Name"]<<"\"."<<std::endl;
		return 0;
	}
	std::string quantityName = quantityNames[0];

	std::vector<std::pair<std::string, std::string> > args;
	args.push_back(std::pair<std::string, std::string>("Int", "int"));

	if(not antok::generators::functionArgumentHandler(args, function, index)) {
		std::cerr<<antok::generators::getFunctionArgumentHandlerErrorMsg(quantityNames);
		return 0;
	}

	antok::Data& data = antok::ObjectManager::instance()->getData();

	int* argAddr = data.getAddr<int>(args[0].first);

	if(not data.insert<double>(quantityName)) {
		std::cerr<<antok::Data::getVariableInsertionErrorMsg(quantityNames);
		return 0;
	}

	return (new antok::functions::ConvertIntToDouble(argAddr, data.getAddr<double>(quantityName)));

};

antok::Function* antok::generators::generateDiff(const YAML::Node& function, std::vector<std::string>& quantityNames, int index)
{

	if(quantityNames.size() > 1) {
		std::cerr<<"Too many names for function \""<<function["Name"]<<"\"."<<std::endl;
		return 0;
	}
	std::string quantityName = quantityNames[0];

	std::vector<std::pair<std::string, std::string> > args;
	args.push_back(std::pair<std::string, std::string>("Minuend", "double"));
	args.push_back(std::pair<std::string, std::string>("Subtrahend", "double"));

	if(not antok::generators::functionArgumentHandler(args, function, index)) {
		std::cerr<<antok::generators::getFunctionArgumentHandlerErrorMsg(quantityNames);
		return 0;
	}

	antok::Data& data = antok::ObjectManager::instance()->getData();

	double* arg1Addr = data.getAddr<double>(args[0].first);
	double* arg2Addr = data.getAddr<double>(args[1].first);

	if(not data.insert<double>(quantityName)) {
		std::cerr<<antok::Data::getVariableInsertionErrorMsg(quantityNames);
		return 0;
	}

	return (new antok::functions::Diff(arg1Addr, arg2Addr, data.getAddr<double>(quantityName)));

};

antok::Function* antok::generators::generateEnergy(const YAML::Node& function, std::vector<std::string>& quantityNames, int index)
{

	if(quantityNames.size() > 1) {
		std::cerr<<"Too many names for function \""<<function["Name"]<<"\"."<<std::endl;
		return 0;
	}
	std::string quantityName = quantityNames[0];

	std::vector<std::pair<std::string, std::string> > args;
	args.push_back(std::pair<std::string, std::string>("Vector", "TLorentzVector"));

	if(not antok::generators::functionArgumentHandler(args, function, index)) {
		std::cerr<<antok::generators::getFunctionArgumentHandlerErrorMsg(quantityNames);
		return 0;
	}

	antok::Data& data = antok::ObjectManager::instance()->getData();

	TLorentzVector* arg1Addr = data.getAddr<TLorentzVector>(args[0].first);

	if(not data.insert<double>(quantityName)) {
		std::cerr<<antok::Data::getVariableInsertionErrorMsg(quantityNames);
		return 0;
	}

	return (new antok::functions::Energy(arg1Addr, data.getAddr<double>(quantityName)));

};

antok::Function* antok::generators::generateGetBeamLorentzVector(const YAML::Node& function, std::vector<std::string>& quantityNames, int index)
{

	if(quantityNames.size() > 1) {
		std::cerr<<"Too many names for function \""<<function["Name"]<<"\"."<<std::endl;
		return 0;
	}
	std::string quantityName = quantityNames[0];

	antok::Data& data = antok::ObjectManager::instance()->getData();

	std::vector<std::pair<std::string, std::string> > args;

	args.push_back(std::pair<std::string, std::string>("dX", "double"));
	args.push_back(std::pair<std::string, std::string>("dY", "double"));
	args.push_back(std::pair<std::string, std::string>("XLorentzVec", "TLorentzVector"));

	if(not antok::generators::functionArgumentHandler(args, function, index)) {
		std::cerr<<antok::generators::getFunctionArgumentHandlerErrorMsg(quantityNames);
		return 0;
	}

	double* dXaddr = data.getAddr<double>(args[0].first);
	double* dYaddr = data.getAddr<double>(args[1].first);
	TLorentzVector* xLorentzVecAddr = data.getAddr<TLorentzVector>(args[2].first);

	if(not data.insert<TLorentzVector>(quantityName)) {
		std::cerr<<antok::Data::getVariableInsertionErrorMsg(quantityNames);
		return 0;
	}

	return (new antok::functions::GetBeamLorentzVec(dXaddr, dYaddr, xLorentzVecAddr, data.getAddr<TLorentzVector>(quantityName)));

};

antok::Function* antok::generators::generateGetGradXGradY(const YAML::Node& function, std::vector<std::string>& quantityNames, int index)
{

	if(quantityNames.size() != 2) {
		std::cerr<<"Need 3 names for function \""<<function["Name"]<<"\"."<<std::endl;
		return 0;
	}

	antok::Data& data = antok::ObjectManager::instance()->getData();

	std::vector<std::pair<std::string, std::string> > args;
	args.push_back(std::pair<std::string, std::string>("Vector", "TLorentzVector"));

	if(not antok::generators::functionArgumentHandler(args, function, index)) {
		std::cerr<<antok::generators::getFunctionArgumentHandlerErrorMsg(quantityNames);
		return 0;
	}

	TLorentzVector* lorentzVectorAddr = data.getAddr<TLorentzVector>(args[0].first);

	std::vector<double*> quantityAddrs;
	for(unsigned int i = 0; i < quantityNames.size(); ++i) {
		if(not data.insert<double>(quantityNames[i])) {
			std::cerr<<antok::Data::getVariableInsertionErrorMsg(quantityNames, quantityNames[i]);
			return 0;
		}
		quantityAddrs.push_back(data.getAddr<double>(quantityNames[i]));
	}

	return (new antok::functions::GetGradXGradY(lorentzVectorAddr, quantityAddrs[0], quantityAddrs[1]));

};

antok::Function* antok::generators::generateGetLorentzVectorAttributes(const YAML::Node& function, std::vector<std::string>& quantityNames, int index)
{

	if(quantityNames.size() != 5) {
		std::cerr<<"Need 5 names for function \""<<function["Name"]<<"\"."<<std::endl;
		return 0;
	}

	antok::Data& data = antok::ObjectManager::instance()->getData();

	std::vector<std::pair<std::string, std::string> > args;
	args.push_back(std::pair<std::string, std::string>("Vector", "TLorentzVector"));

	if(not antok::generators::functionArgumentHandler(args, function, index)) {
		std::cerr<<antok::generators::getFunctionArgumentHandlerErrorMsg(quantityNames);
		return 0;
	}

	TLorentzVector* lorentzVectorAddr = data.getAddr<TLorentzVector>(args[0].first);

	std::vector<double*> quantityAddrs;
	for(unsigned int i = 0; i < quantityNames.size(); ++i) {
		if(not data.insert<double>(quantityNames[i])) {
			std::cerr<<antok::Data::getVariableInsertionErrorMsg(quantityNames, quantityNames[i]);
			return 0;
		}
		quantityAddrs.push_back(data.getAddr<double>(quantityNames[i]));
	}

	return (new antok::functions::GetLorentzVectorAttributes(lorentzVectorAddr,
	                                                         quantityAddrs[0],
	                                                         quantityAddrs[1],
	                                                         quantityAddrs[2],
	                                                         quantityAddrs[3],
	                                                         quantityAddrs[4]));

};

antok::Function* antok::generators::generateGetLorentzVec(const YAML::Node& function, std::vector<std::string>& quantityNames, int index)
{

	if(quantityNames.size() > 1) {
		std::cerr<<"Too many names for function \""<<function["Name"]<<"\"."<<std::endl;
		return 0;
	}
	std::string quantityName = quantityNames[0];

	antok::Data& data = antok::ObjectManager::instance()->getData();

	int pType;
	if(function["X"] and function["M"]) {
		pType = 0;
	} else if (function["Px"] and function["E"]) {
		pType = 1;
	} else if (function["Vec3"] and function["M"]) {
		pType = 2;
	} else if (function["Vec3"] and function["E"]) {
		pType = 3;
	} else {
		std::cerr<<"Function \"getLorentzVec\" needs either variables \"[X, Y, Z, M]\" or \"[Px, Py, Pz, E]\" (variable \""<<quantityName<<"\")."<<std::endl;
		return 0;
	}

	std::vector<std::pair<std::string, std::string> > args;

	double* xAddr;
	double* yAddr;
	double* zAddr;
	double* mAddr;
	TVector3* vec3Addr;

	switch(pType)
	{
		case 0:
			args.push_back(std::pair<std::string, std::string>("X", "double"));
			args.push_back(std::pair<std::string, std::string>("Y", "double"));
			args.push_back(std::pair<std::string, std::string>("Z", "double"));
			try {
				function["M"].as<double>();
			} catch(const YAML::TypedBadConversion<double>& e) {
				std::cerr<<"Argument \"M\" in function \"mass\" should be of type double (variable \""<<quantityName<<"\")."<<std::endl;
				return 0;
			}
			mAddr = new double();
			(*mAddr) = function["M"].as<double>();
			break;
		case 1:
			args.push_back(std::pair<std::string, std::string>("Px", "double"));
			args.push_back(std::pair<std::string, std::string>("Py", "double"));
			args.push_back(std::pair<std::string, std::string>("Pz", "double"));
			args.push_back(std::pair<std::string, std::string>("E", "double"));
			break;
		case 2:
			args.push_back(std::pair<std::string, std::string>("Vec3", "TVector3"));
			try {
				function["M"].as<double>();
			} catch(const YAML::TypedBadConversion<double>& e) {
				std::cerr<<"Argument \"M\" in function \"mass\" should be of type double (variable \""<<quantityName<<"\")."<<std::endl;
				return 0;
			}
			mAddr = new double();
			(*mAddr) = function["M"].as<double>();
			break;
		case 3:
			args.push_back(std::pair<std::string, std::string>("Vec", "TVector3"));
			args.push_back(std::pair<std::string, std::string>("E", "double"));
			break;
	}

	if(not antok::generators::functionArgumentHandler(args, function, index)) {
		std::cerr<<antok::generators::getFunctionArgumentHandlerErrorMsg(quantityNames);
		return 0;
	}

	switch(pType)
	{
		case 0:
			xAddr = data.getAddr<double>(args[0].first);
			yAddr = data.getAddr<double>(args[1].first);
			zAddr = data.getAddr<double>(args[2].first);
			break;
		case 1:
			xAddr = data.getAddr<double>(args[0].first);
			yAddr = data.getAddr<double>(args[1].first);
			zAddr = data.getAddr<double>(args[2].first);
			mAddr = data.getAddr<double>(args[3].first);
			break;
		case 2:
			vec3Addr = data.getAddr<TVector3>(args[0].first);
			mAddr = data.getAddr<double>(args[1].first);
			break;
		case 3:
			vec3Addr = data.getAddr<TVector3>(args[0].first);
			mAddr = data.getAddr<double>(args[1].first);
			break;
	}

	if(not data.insert<TLorentzVector>(quantityName)) {
		std::cerr<<antok::Data::getVariableInsertionErrorMsg(quantityNames);
		return 0;
	}

	switch(pType)
	{
		case 0:
			return (new antok::functions::GetLorentzVec(xAddr, yAddr, zAddr, mAddr, data.getAddr<TLorentzVector>(quantityName), pType));
		case 1:
			return (new antok::functions::GetLorentzVec(xAddr, yAddr, zAddr, mAddr, data.getAddr<TLorentzVector>(quantityName), pType));
		case 2:
			return (new antok::functions::GetLorentzVec(vec3Addr, mAddr, data.getAddr<TLorentzVector>(quantityName), pType));
		case 3:
			return (new antok::functions::GetLorentzVec(vec3Addr, mAddr, data.getAddr<TLorentzVector>(quantityName), pType));
	}
	return 0;
};

antok::Function* antok::generators::generateGetTs(const YAML::Node& function, std::vector<std::string>& quantityNames, int index)
{

	if(quantityNames.size() != 3) {
		std::cerr<<"Need 3 names for function \"getTs\""<<std::endl;
		return 0;
	}

	antok::Data& data = antok::ObjectManager::instance()->getData();

	std::vector<std::pair<std::string, std::string> > args;

	args.push_back(std::pair<std::string, std::string>("BeamLorentzVec", "TLorentzVector"));
	args.push_back(std::pair<std::string, std::string>("XLorentzVec", "TLorentzVector"));

	if(not antok::generators::functionArgumentHandler(args, function, index)) {
		std::cerr<<antok::generators::getFunctionArgumentHandlerErrorMsg(quantityNames);
		return 0;
	}

	TLorentzVector* beamLVAddr = data.getAddr<TLorentzVector>(args[0].first);
	TLorentzVector* xLVAddr = data.getAddr<TLorentzVector>(args[1].first);

	std::vector<double*> quantityAddrs;
	for(unsigned int i = 0; i < quantityNames.size(); ++i) {
		if(not data.insert<double>(quantityNames[i])) {
			std::cerr<<antok::Data::getVariableInsertionErrorMsg(quantityNames, quantityNames[i]);
			return 0;
		}
		quantityAddrs.push_back(data.getAddr<double>(quantityNames[i]));
	}

	return (new antok::functions::GetTs(xLVAddr, beamLVAddr, quantityAddrs[0], quantityAddrs[1], quantityAddrs[2]));

};

antok::Function* antok::generators::generateGetVector3(const YAML::Node& function, std::vector<std::string>& quantityNames, int index) {

	if(quantityNames.size() > 1) {
		std::cerr<<"Too many names for function \""<<function["Name"]<<"\"."<<std::endl;
		return 0;
	}
	const std::string& quantityName = quantityNames[0];

	antok::Data& data = antok::ObjectManager::instance()->getData();

	std::vector<std::pair<std::string, std::string> > args;
	args.push_back(std::pair<std::string, std::string>("X", "double"));
	args.push_back(std::pair<std::string, std::string>("Y", "double"));
	args.push_back(std::pair<std::string, std::string>("Z", "double"));

	if(not antok::generators::functionArgumentHandler(args, function, index)) {
		std::cerr<<antok::generators::getFunctionArgumentHandlerErrorMsg(quantityNames);
		return 0;
	}

	double* xAddr = data.getAddr<double>(args[0].first);
	double* yAddr = data.getAddr<double>(args[1].first);
	double* zAddr = data.getAddr<double>(args[2].first);

	if(not data.insert<TVector3>(quantityName)) {
		std::cerr<<antok::Data::getVariableInsertionErrorMsg(quantityNames);
		return 0;
	}

	return (new antok::functions::GetTVector3(xAddr, yAddr, zAddr, data.getAddr<TVector3>(quantityName)));

}

antok::Function* antok::generators::generateMass(const YAML::Node& function, std::vector<std::string>& quantityNames, int index)
{

	if(quantityNames.size() > 1) {
		std::cerr<<"Too many names for function \""<<function["Name"]<<"\"."<<std::endl;
		return 0;
	}
	std::string quantityName = quantityNames[0];

	antok::Data& data = antok::ObjectManager::instance()->getData();

	std::vector<std::pair<std::string, std::string> > args;
	args.push_back(std::pair<std::string, std::string>("Vector", "TLorentzVector"));

	if(not antok::generators::functionArgumentHandler(args, function, index)) {
		std::cerr<<antok::generators::getFunctionArgumentHandlerErrorMsg(quantityNames);
		return 0;
	}

	TLorentzVector* vector = data.getAddr<TLorentzVector>(args[0].first);
	if(not data.insert<double>(quantityName)) {
		std::cerr<<antok::Data::getVariableInsertionErrorMsg(quantityNames);
		return 0;
	}

	return (new antok::functions::Mass(vector, data.getAddr<double>(quantityName)));

};

antok::Function* antok::generators::generateRadToDegree(const YAML::Node& function, std::vector<std::string>& quantityNames, int index)
{

	if(quantityNames.size() > 1) {
		std::cerr<<"Too many names for function \""<<function["Name"]<<"\"."<<std::endl;
		return 0;
	}
	std::string quantityName = quantityNames[0];

	antok::Data& data = antok::ObjectManager::instance() ->getData();

	std::vector<std::pair<std::string, std::string> > args;
	args.push_back(std::pair<std::string, std::string>("Angle", "double"));

	if(not antok::generators::functionArgumentHandler(args, function, index)) {
		std::cerr<<antok::generators::getFunctionArgumentHandlerErrorMsg(quantityNames);
		return 0;
	}

	double* angle = data.getAddr<double>(args[0].first);
	if(not data.insert<double>(quantityName)) {
		std::cerr<<antok::Data::getVariableInsertionErrorMsg(quantityNames);
		return 0;
	}

	return (new antok::functions::RadToDegree(angle, data.getAddr<double>(quantityName)));

};

antok::Function* antok::generators::generateSum(const YAML::Node& function, std::vector<std::string>& quantityNames, int index)
{

	if(quantityNames.size() > 1) {
		std::cerr<<"Too many names for function \""<<function["Name"]<<"\"."<<std::endl;
		return 0;
	}
	std::string quantityName = quantityNames[0];

	std::vector<std::pair<std::string, std::string> >* summandNamesPtr = __getSummandNames(function, quantityName, index, "Summands");
	std::vector<std::pair<std::string, std::string> >* subtrahendNamesPtr = __getSummandNames(function, quantityName, index, "Subtrahends");
	if((summandNamesPtr == 0) and (subtrahendNamesPtr == 0)) {
		std::cerr<<"Could not generate summands for function \"Sum\" when trying to register calculation of \""<<quantityName<<"\"."<<std::endl;
		return 0;
	}
	std::vector<std::pair<std::string, std::string> > summandNames;
	std::vector<std::pair<std::string, std::string> > subtrahendNames;
	if(summandNamesPtr != 0)
		summandNames = (*summandNamesPtr);
	if(subtrahendNamesPtr != 0)
		subtrahendNames = (*subtrahendNamesPtr);

	if(not antok::generators::functionArgumentHandler(summandNames, function, index, true)) {
		std::cerr<<antok::generators::getFunctionArgumentHandlerErrorMsg(quantityNames);
		return 0;
	}
	if(not antok::generators::functionArgumentHandler(subtrahendNames, function, index, true)) {
		std::cerr<<antok::generators::getFunctionArgumentHandlerErrorMsg(quantityNames);
		return 0;
	}
	std::string typeName;
	if(summandNamesPtr != 0)
		typeName = summandNames[0].second;
	else
		typeName = subtrahendNames[0].second;

	antok::Function* antokFunction = 0;
	if(typeName == "double") {
		antokFunction = __getSumFunction<double>(summandNames, subtrahendNames, quantityName);
	} else if (typeName == "int") {
		antokFunction = __getSumFunction<int>(summandNames, subtrahendNames, quantityName);
	} else if (typeName == "Long64_t") {
		antokFunction = __getSumFunction<Long64_t>(summandNames, subtrahendNames, quantityName);
	} else if (typeName == "TLorentzVector") {
		antokFunction = __getSumFunction<TLorentzVector>(summandNames, subtrahendNames, quantityName);
	} else {
		std::cerr<<"Type \""<<typeName<<"\" not supported by \"sum\" (registering calculation of \""<<quantityName<<"\")."<<std::endl;
		return 0;
	}

	delete summandNamesPtr;

	return (antokFunction);

};

antok::Function* antok::generators::generateSum2(const YAML::Node& function, std::vector<std::string>& quantityNames, int index)
{

	if(quantityNames.size() > 1) {
		std::cerr<<"Too many names for function \""<<function["Name"]<<"\"."<<std::endl;
		return 0;
	}
	std::string quantityName = quantityNames[0];

	antok::Data& data = antok::ObjectManager::instance()->getData();

	std::vector<double*> doubleInputAddrs;

	std::vector<std::pair<std::string, std::string> >* summandNamesPtr = __getSummandNames(function, quantityName, index, "Summands");
	if(summandNamesPtr == 0) {
		std::cerr<<"Could not generate summands for function \"Sum\" when trying to register calculation of \""<<quantityName<<"\"."<<std::endl;
		return 0;
	}
	std::vector<std::pair<std::string, std::string> >& summandNames = (*summandNamesPtr);

	if(not antok::generators::functionArgumentHandler(summandNames, function, index, true)) {
		std::cerr<<antok::generators::getFunctionArgumentHandlerErrorMsg(quantityNames);
		return 0;
	}

	// Now do type checking and get all the addresses
	for(unsigned int summandNames_i = 0; summandNames_i < summandNames.size(); ++summandNames_i) {

			std::string variableName = summandNames[summandNames_i].first;

			double* addr = data.getAddr<double>(variableName);
			doubleInputAddrs.push_back(addr);

		}

	// And produce the function
	if(not data.insert<double>(quantityName)) {
		std::cerr<<antok::Data::getVariableInsertionErrorMsg(quantityNames);
		return 0;
	}

	delete summandNamesPtr;

	return (new antok::functions::Sum2(doubleInputAddrs, data.getAddr<double>(quantityName)));

};

