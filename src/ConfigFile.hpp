#ifndef CONFIG_FILE_HPP
#define CONFIG_FILE_HPP

#include <string>

struct CallSignature{
    std::string call_signature{};
    std::string active{};
    std::string output{};
    std::string mode{};
    std::string driver_type{};
    

    CallSignature( std::string_view _call_signature,
        std::string_view _active,
        std::string_view _output,
        std::string_view _mode,
        std::string_view _driver_type)
        
        : call_signature(_call_signature),
          active(_active),
          output(_output),
          mode(_mode),
          driver_type(_driver_type){}
};

class ConfigFile {
    std::string _language{};
    std::vector<CallSignature> _functions{};

public:
    virtual ~ConfigFile() = default;

    virtual bool validateInput() { return false; }
    virtual std::string getLanguage() const { return _language; }
    virtual std::vector<CallSignature> getFunctions() const { return _functions; }
    virtual void readYamlFile(std::string const &file_path) {};
    virtual std::string getActiveVariables() const { return {}; }
    virtual std::string getOutputVariables() const { return {}; }
    virtual std::string getMode() const { return {}; }
    virtual std::string getDriverType() const { return {}; }
};

#endif //CONFIG_FILE_HPP
