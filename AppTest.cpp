#include "NodeCommunicate.h"
#include <sstream>
#include <thread>
#include <chrono>
#define CATCH_CONFIG_MAIN
#include "catch.hpp"
TEST_CASE("Test convertMapAndVectorToJson", "[NodeCommunicate]"){
    std::vector<double> testV={.5, .6, .7};
    std::unordered_map<std::string, std::vector<double>*> tt;
    tt["key"]=&testV;
    REQUIRE(convertMapAndVectorToJson(tt)==std::string("{\"key\":[0.5,0.6,0.7]}"));
    tt["key2"]=&testV;
    REQUIRE(convertMapAndVectorToJson(tt)==std::string("{\"key\":[0.5,0.6,0.7],\"key2\":[0.5,0.6,0.7]}"));
}
TEST_CASE("Test convertMapToJson", "[NodeCommunicate]"){
    std::unordered_map<std::string, double> tt;
    tt["key"]=4;
    tt["key2"]=5;
    std::unordered_map<std::string, std::string> tts;
    tts["key"]="4";
    tts["key2"]="5";
    REQUIRE(convertMapToJson<Number>(tt)==std::string("{\"key\":4,\"key2\":5}"));
    REQUIRE(convertMapToJson<String>(tt)==std::string("{\"key\":\"4\",\"key2\":\"5\"}"));
    REQUIRE(convertMapToJson<Number>(tts)==std::string("{\"key\":4,\"key2\":5}"));
    REQUIRE(convertMapToJson<String>(tts)==std::string("{\"key\":\"4\",\"key2\":\"5\"}"));
}
/*TEST_CASE("Test NodeCommunication", "[NodeCommunicate]"){
    std::streambuf *sbuf = std::cout.rdbuf();
    NodeCommunication nc;
    nc.addEndPoint(std::string("test1"), [](rapidjson::Value& val,  std::string& id,  tcb cb){
        cb(id, "{\"key1\":\"SomeDataIGot\"}");
    });
    std::stringstream bufferCout;
    std::stringstream bufferCin;
    
    std::thread t1([&](){
        nc.start(bufferCin);
        std::cerr<<"{\"test1\":{\"id\":\"123\", \"data\":{\"key\":\"Hello\"}}}"<<std::endl;
        bufferCin<<"{\"test1\":{\"id\":\"123\", \"data\":{\"key\":\"Hello\"}}}"<<std::endl;
    });
    
    std::this_thread::sleep_for (std::chrono::milliseconds(200));
    
    std::cout.rdbuf(bufferCout.rdbuf());
    REQUIRE(bufferCout.str()=="{\"test1\":{\"id\":\"123\", \"data\":{\"key1\":\"SomeDataIGot\"}}}");
    t1.detach();
}*/
