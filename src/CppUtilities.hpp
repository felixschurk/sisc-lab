#ifndef SISC_LAB_CPPUTILITIES_HPP
#define SISC_LAB_CPPUTILITIES_HPP

#include <utility>

#include <absl/strings/str_split.h>
#include <absl/strings/str_cat.h>
#include <absl/strings/str_join.h>
#include <absl/algorithm/container.h>

#include "ConfigFile.hpp"
#include "Utilities.hpp"
#include "CallSignature.hpp"


class CppUtilities : public Utilities {
    CallSignature _callSignature;
public:
    explicit CppUtilities(CallSignature callSignature) : _callSignature(std::move(callSignature)) {};

    ~CppUtilities() override = default;

    [[nodiscard]] std::string getTypeOfVariable(const std::string &activeVariable) const;
    std::string getAssociationByNameSignature() final;
    std::string createLoopSignature(const std::string &activeVariable, int level) final;
    std::string setSeedValue(const std::string &variable, const std::string &value_for_seeding,
                             const std::string &loop_level) final;
    std::string initializeSeedValue(const std::string &variable) final;
    std::string resetSeedValue(const std::string &variable, const std::string &loop_level) final;
    std::string harvest(const std::string &variable, const std::string &loop_level) final;
    std::string harvestVec(const std::string &ActVariable, const std::string &OutVariable) final;
    std::string createDriverCallSignature() final;
    std::string createDriverCallArguments() final;
    std::string getModeTypeSuffix() final;
};

std::string CppUtilities::getTypeOfVariable(const std::string &activeVariable) const
{
    std::vector<std::string> callSignatureSplitted = absl::StrSplit(_callSignature.call_signature, absl::ByAnyChar(" ,("),  absl::SkipEmpty());

    for (auto &i : callSignatureSplitted)
    {
        // do not remove & if it is the only character in the string
        if (i.size() > 1){
            if (absl::StartsWith(i, "&")) {
                i.erase(i.begin());
            }
            if (absl::EndsWith(i, "&")) {
                i.pop_back();
            }
            if (absl::EndsWith(i, ")")) {
                i.pop_back();
            }
        }

        // use logic that the type needs to be always before the variable name
        if (absl::EqualsIgnoreCase(i, activeVariable)) {
            if (*std::prev(&i) == "&") {
                return *std::prev(&i,2);
            }
            return *std::prev(&i);
        }
    }
    // remove empty string if it fails
    return "";
}

std::string CppUtilities::getAssociationByNameSignature() {
    //Define the suffix based on the mode
    std::string suffix = getModeTypeSuffix();
    // creates the function call with the _t at the end
    std::vector<std::string> splittedCallSignature = absl::StrSplit(_callSignature.call_signature, absl::ByAnyChar(" ,()"),  absl::SkipEmpty());
    std::string functionCall = absl::StrCat(splittedCallSignature[1], suffix,"(");

    // separates the arguments from the call signature and organize in a vector
    std::vector<std::string> callSignatureArguments = absl::StrSplit(_callSignature.call_signature, absl::ByAnyChar(",()"),  absl::SkipEmpty());
    callSignatureArguments.erase(callSignatureArguments.begin());

    // creates a vector with the active variables
    std::vector<std::string> activeVariables = absl::StrSplit(_callSignature.active, absl::ByAnyChar(" ,"),  absl::SkipEmpty());
    std::vector<std::string> words;

    // creates a vector with the output variables
    std::vector<std::string> outputVariables = absl::StrSplit(_callSignature.output, absl::ByAnyChar(" ,"),  absl::SkipEmpty());

    // checks which arguments from call signature are active variables or parameters
    for (int i = 0; i < callSignatureArguments.size(); i++) {
        words = absl::StrSplit(callSignatureArguments[i], absl::ByAnyChar(" &"), absl::SkipEmpty());
        functionCall = absl::StrCat(functionCall, words.back());
        if (absl::c_linear_search(activeVariables, words.back()) || absl::c_linear_search(outputVariables, words.back())) {
            functionCall = absl::StrCat(functionCall, ", ", words.back(), suffix);
        }

        if (i < callSignatureArguments.size() - 1) {
            functionCall = absl::StrCat(functionCall, ", ");
        } else {
            functionCall = absl::StrCat(functionCall, ")");
        }
    }
    return functionCall;
}

std::string CppUtilities::createLoopSignature(const std::string &activeVariable,
                                              int level) {
    // create based on the level a multiplication of i
    char loopVariableChar = 'i';
    std::string loopVariable;
    for (auto i = 0; i < level; ++i){ loopVariable += loopVariableChar; }
    std::string loopSignature = "for (size_t " + loopVariable + " = 0; " + loopVariable + " < " + activeVariable + ".size(); ++" + loopVariable + ")";
    return loopSignature;
}

std::string CppUtilities::setSeedValue(const std::string &variable,
                                       const std::string &value_for_seeding,
                                       const std::string &loop_level)
{
    auto type_of_variable = getTypeOfVariable(variable);

    std::string variable_type;
    if (type_of_variable.find("std::vector<") != std::string::npos) {
        variable_type= "vector";
    } else {
        variable_type= "scalar";
    }

    // Get the suffix based on the mode
    std::string suffix = getModeTypeSuffix();

    // Construct the seed value assignment string
    std::string setSeed;
    if (variable_type == "scalar") {
        setSeed = absl::StrCat(variable, suffix, " = ", value_for_seeding);
    } else if (variable_type == "vector") {
        int num_loops = std::stoi(loop_level);
        if (num_loops < 0) {
            throw std::invalid_argument("Loop level must be a non-negative integer.");
        }

        auto loop_index = std::string(num_loops, 'i');
        setSeed = absl::StrCat(variable, suffix,"[" , loop_index, "]", " = ", value_for_seeding);
    } else {
        throw std::invalid_argument("Unsupported output type: " + type_of_variable + ". Supported types are 'scalar' and 'vector'.");
    }

    return setSeed;
}

std::string CppUtilities::initializeSeedValue(const std::string& variable)
{
    auto type_of_variable = getTypeOfVariable(variable);

    //Get the suffix based on the mode
    std::string suffix = getModeTypeSuffix();

    // Check if type_of_variable contains "int" --> Add appropriate value 0 or 0.0
    std::string value;
    if (type_of_variable.find("int") != std::string::npos) {
        value = "0";
    } else {
        value = "0.0";
    }

    std::string initSeed;

    // Check if type_of_variable contains "std::vector<"
    if (type_of_variable.find("std::vector<") != std::string::npos) {
        initSeed = absl::StrCat(type_of_variable, " ", variable, suffix, "(", variable, ".size(), ", value, ")");
    } else {
        initSeed = absl::StrCat(type_of_variable, " ", variable, suffix, "(", value, ")");
    }

    return initSeed;
}

std::string CppUtilities::resetSeedValue(const std::string& variable,
                                         const std::string& loop_level)
{
    auto type_of_variable = getTypeOfVariable(variable);

    // Add appropriate suffix based on the mode
    std::string variable_type;
    if (type_of_variable.find("std::vector<") != std::string::npos) {
        variable_type= "vector";
    } else {
        variable_type= "scalar";
    }
    std::string reset_Value = "0.0";
    // Get the suffix based on the mode
    std::string suffix = getModeTypeSuffix();
    // Construct the seed value assignment string
    std::string resetSeed;
    if (variable_type == "scalar") {
        resetSeed = absl::StrCat(variable, suffix, " = ", reset_Value);
    }
    else if (variable_type == "vector")
    {
        int num_loops = std::stoi(loop_level);
        if (num_loops < 0) {
            throw std::invalid_argument("Loop level must be a non-negative integer.");
        }

        auto loop_index = std::string(num_loops, 'i');
        resetSeed = absl::StrCat(variable, suffix,"[" , loop_index, "]", " = ", reset_Value);
    }
    else
    {
        throw std::invalid_argument("Unsupported output type. Supported types are 'scalar' and 'vector'.");
    }

    return resetSeed;
}

std::string CppUtilities::createDriverCallSignature(){
    // Splits the call signature in a vector of strings
    std::vector<std::string> splittedCallSignature
            = absl::StrSplit(_callSignature.call_signature, absl::ByAnyChar(" ,()"),  absl::SkipEmpty());
    // Splits the driver type  in a vector of strings
    std::vector<std::string> driverType
            = absl::StrSplit(_callSignature.driver_type, absl::ByAnyChar(" ,()"),  absl::SkipEmpty());
    // Concatenates the driver type with the driver function call
    std::string driverCallSignature
            = absl::StrCat(splittedCallSignature[0]," ", splittedCallSignature[1], "_", driverType[0]);
    driverCallSignature = absl::StrCat(driverCallSignature, CppUtilities::createDriverCallArguments());
    return driverCallSignature;
}

std::string CppUtilities::harvest(const std::string &variable, const std::string &loop_level)
{
    // Get the suffix based on the mode
    std::string suffix = getModeTypeSuffix();
    // Determine if the variable is a vector or scalar
    std::string variable_type;
    if (getTypeOfVariable(variable).find("std::vector<") != std::string::npos) {
        variable_type = "vector";
    } else {
        variable_type = "scalar";
    }

    // Check if loop_level is zero
    if (loop_level == "0") {
        // Return the appropriate string based on the mode and variable type
        if (variable_type == "vector")
        {
        // Throw an exception for invalid loop_level
        throw std::invalid_argument("Loop level cannot be zero while harvesting a vector.");
        }
        return absl::StrCat("d", variable, " = ", variable, suffix);
    } else if (loop_level > "0") {
        // Check if the loop level is a positive integer
        int num_loops = std::stoi(loop_level);
        if (num_loops < 0) {
            throw std::invalid_argument("Loop level must be a non-negative integer.");
        }

        // Construct the loop index string with 'i' repeated num_loops times
        auto loop_index = std::string(num_loops, 'i');

        // Return the appropriate string based on the mode and variable type with loop index
        return absl::StrCat("d", variable, "[", loop_index, "] = ", variable, suffix,"[", loop_index, "]");
    } else {
        // Throw an exception for invalid loop_level
        throw std::invalid_argument("Loop level must be a non-negative integer or '0'.");
    }
}

std::string CppUtilities::harvestVec(const std::string &ActVariable, const std::string &OutVariable)
{
    std::string suffix = getModeTypeSuffix();

    if (_callSignature.mode == "tangent") {
        return absl::StrCat("d", OutVariable, ".insert(","d", OutVariable,".end(), ", OutVariable, suffix, ".begin(), ", OutVariable, suffix, ".end())");
    } else if (_callSignature.mode == "adjoint") {
                return absl::StrCat("d", ActVariable, ".insert(","d", ActVariable,".end(), ", ActVariable, suffix, ".begin(), ", ActVariable, suffix, ".end())");
    } else {
        throw std::invalid_argument("Unsupported mode: '" + _callSignature.mode + "'. Supported modes are 'tangent' and 'adjoint'.");
    }
}

std::string CppUtilities::createDriverCallArguments(){
    std::string driverCallArguments = "(";
    // Creates a string vector with all arguments from the call signature
    std::vector<std::string> callSignatureArguments = absl::StrSplit(_callSignature.call_signature, absl::ByAnyChar(",()"),  absl::SkipEmpty());
    callSignatureArguments.erase(callSignatureArguments.begin());

    // Creates a string vector with the active variables
    std::vector<std::string> activeVariables = absl::StrSplit(_callSignature.active, absl::ByAnyChar(" ,"),  absl::SkipEmpty());

    // Creates a string vector with the output  variables
    std::vector<std::string> outputVariables = absl::StrSplit(_callSignature.output, absl::ByAnyChar(" ,"),  absl::SkipEmpty());

    // Creates auxiliary variables
    std::vector<std::string> words;
    std::vector<std::string> variableType;
    std::string variableTypeString;

    for (int i = 0; i < callSignatureArguments.size(); i++) {

        words = absl::StrSplit(callSignatureArguments[i], absl::ByAnyChar(" &"), absl::SkipEmpty());

        variableType = words;
        variableType.pop_back();
        variableTypeString = absl::StrJoin(variableType, " ");
        driverCallArguments = absl::StrCat(driverCallArguments, variableTypeString, " &", words.back());

        if (absl::c_linear_search(activeVariables, words.back()) && _callSignature.mode == "adjoint"){
            if (_callSignature.driver_type == "gradient"){
                driverCallArguments = absl::StrCat(driverCallArguments, ", ", variableTypeString, " &d", words.back());
            } else if (_callSignature.driver_type == "jacobian"){
                driverCallArguments = absl::StrCat(driverCallArguments, ",",  variableTypeString, " &d", words.back());
            }
        }

        if (absl::c_linear_search(outputVariables, words.back()) && _callSignature.mode == "tangent"){
            if (_callSignature.driver_type == "gradient" || _callSignature.driver_type == "jacobian"){
                driverCallArguments = absl::StrCat(driverCallArguments, ", ", variableTypeString, " &d", words.back());
            }
        }

        if (i < callSignatureArguments.size() - 1) {
            driverCallArguments = absl::StrCat(driverCallArguments, ", ");
        } else {
            driverCallArguments = absl::StrCat(driverCallArguments, ")");
        }
    }
return driverCallArguments;
}

/// @brief Function returns the suffix based on the mode of the driver. It returns "_t" for tangent mode or "_a" for adjoint mode.
/// @return Returns a string containing "_t" or "_a"
std::string CppUtilities::getModeTypeSuffix(){
    std::string suffix;
    if (_callSignature.mode == "tangent" || _callSignature.mode == "Tangent" ) {
        suffix = "_t";
    } else if (_callSignature.mode == "adjoint" || _callSignature.mode == "Adjoint") {
        suffix = "_a";
    } else {
        throw std::invalid_argument("Unsupported mode: '" + _callSignature.mode + "' . Supported moods are 'tangent' and 'adjoint'.");
    }
    return suffix;
}

#endif //SISC_LAB_CPPUTILITIES_HPP
