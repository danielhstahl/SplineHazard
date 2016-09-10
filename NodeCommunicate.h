#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include "document.h" //rapidjson
#include "writer.h" //rapidjson
#include "stringbuffer.h" //rapidjson
#include "LossSplineModel.h"
const int String=0;
const int Number=1;
typedef void (*tcb)(std::string&, const std::string&);
typedef void (*FnPtr)(rapidjson::Value&, std::string&,  tcb);//message, id, callback
/**
Sends message to calling program.
@param guid Unique id for the calling program
@param message Message to send.  Assumed to be JSON formated string
*/
void send(std::string& guid, const std::string& message){
    std::cout<<"{\""+guid+"\":"+message+"}"<<std::endl;
}
/**
Sends error to calling program.
@param guid Unique id for the calling program
@param message Error message to send.  Assumed to be a standard string (not JSON formated)
*/
void sendError(std::string& guid, const std::string& message){
    std::cerr<<"{\""+guid+"\":\""+message+"\"}"<<std::endl;
}
template<typename T>
std::string convertVectorToJson(const std::vector<T>& result){
    int n=result.size();
    std::stringstream json;
    json<<"[";
    for(int i=0; i<(n-1); ++i){
        json<<result[i]<<",";
    }
    json<<result[n-1]<<"]";
    return json.str();
}
template<int type, typename T, typename K>
std::string convertMapToJson(const std::unordered_map<T, K>& result){
    std::stringstream json;
    json<<"{";
    std::string firstInd;
    std::string secondInd;
    switch(type){
        case String:
            firstInd="\":\"";
            secondInd="\"";
            break;
        case Number:
            firstInd="\":";
            secondInd="";
            break;
    }
    for (auto iter = result.begin(); iter != result.end(); ++iter) { 
        json<<"\""<<iter->first<<firstInd<<iter->second<<secondInd;
        if (std::next(iter) != result.end()) {
            json<<",";
        }
    }
    json<<"}";
    return json.str();
}
template<typename T>
std::string convertMapAndVectorToJson(const std::unordered_map<std::string, std::vector<T>*>& result){
    std::stringstream json;
    json<<"{";
    for (auto iter = result.begin(); iter != result.end(); ++iter) { 
        json<<"\""<<iter->first<<"\":"<<convertVectorToJson(*iter->second);
        if (std::next(iter) != result.end()) {
            json<<",";
        }
    }
    json<<"}";
    return json.str();
}
/*std::string convertStringsToJson(const std::vector<std::string>& keys, const std::vector<std::string>& values ){
    std::stringstream json;
    json<<"{";
    for(auto iter=keys.begin(); iter!=result.end(); ++iter){
        json<<"\""<<iter->first<<"\":\""<<iter->second<<"\"";
    }
}*/
/**
This class facilitates communication with external scripts and programs through the stdout and stderr pipes.  
*/
class NodeCommunication{
private:
    std::map<std::string, FnPtr> holdCallbacks;
public:
    /**
    Adds pointer to a function which is called when named in the start function below.
    @param name Name of the function.  Can be anything, but should be named so that it is easy to remember and call from external program.
    @param cb Function with return type void and arguments of rapidjson::Value&, a std::string&, and a callback function which returns void and takes two references to strings.
    */
    template<typename Callback>
    void addEndPoint(const std::string& name, Callback& cb){
        holdCallbacks[name]=cb;
    }
    /**
    This function starts the "listening" for commands from the script or external program which calls the C++ program.  It assumes that the message is of JSON type and that the key references a endpoint function crated in addEndPoint.  This is blocking so will not accept any other commands when a JSON message is sent and the function called.  Since a new C++ is created for every session, this is not a scalability issue.
    */
    void start(){
        start(std::cin);
    }
    void start(std::istream& istr){
        while(true){
            std::string parameters;
            for (parameters; std::getline(istr, parameters);) {
                break;
            }
            rapidjson::Document parms;
		    parms.Parse(parameters.c_str());//yield data
		    parameters.clear(); 
            bool hasFunction=false;
            std::string id="";
            for (auto M=parms.MemberBegin(); M!=parms.MemberEnd(); M++){
                hasFunction=true;
                if(holdCallbacks.find(M->name.GetString())!=holdCallbacks.end()){
                    id=M->value["id"].GetString();
                    holdCallbacks[M->name.GetString()](M->value["data"],  id, send);
                }
            }
            if(!hasFunction){
                sendError(id, std::string("No function found"));
            }
        }
    }

};
