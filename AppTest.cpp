#include "NodeCommunicate.h"
#include "Seasonality.h"
#include "Newton.h"
#include "EGD.h"
#include "Matrix.h"
#include "MC.h"
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
TEST_CASE("Test applySeasonality", "[Seasonality]"){
    Seasonality season;
    int numSeason=12;
    for(int i=0; i<numSeason; ++i){
        season.addSeason(i+1);
    }
    int Jan=1;
    int Feb=2;
    int Mar=3;
    int Apr=4;
    int May=5;
    int Jun=6;
    int Jul=7;
    int Aug=8;
    int Sep=9;
    int Oct=10;
    int Nov=11;
    int Dec=12;
    REQUIRE(season.applySeasonality(Apr, 1)==5);
    REQUIRE(season.applySeasonality(Apr, 6)==10);
    REQUIRE(season.applySeasonality(Apr, 8)==12);
    REQUIRE(season.applySeasonality(Apr, 9)==1);
    REQUIRE(season.applySeasonality(Apr, 10)==2);
    REQUIRE(season.applySeasonality(Apr, 20)==12);
    REQUIRE(season.applySeasonality(Apr, 21)==1);
    REQUIRE(season.applySeasonality(Apr, 22)==2);
}
TEST_CASE("Test zeros", "[Newton]"){
    Newton newt;
    
}
TEST_CASE("Test get", "[Matrix]"){
    Matrix<double> mat;
    int n=15;
    for(int i=0; i<n; ++i){
        mat.emplace_back(i+1);
    }
    mat.setM(3);
    REQUIRE(mat.getMean(0)==7);
    REQUIRE(mat.getMean(1)==8);
    REQUIRE(mat.getMean(2)==9);
    REQUIRE(mat.get(3, 2)==12);
    
}
TEST_CASE("Test simulateDistribution", "[MC]"){
    MC<double> mc(5);
    int i=1;
    mc.simulateDistribution([&](){
        return i++;
    });
    REQUIRE(mc.getVariance()==2.5);
    REQUIRE(mc.getEstimate()==3.0);
    
    
}
TEST_CASE("Test sort_indexes", "[RiskContribution]"){
    std::vector<double> port={1, 2, 3, 4, 5};
    RiskContribution<Upper> rcu(port);
    std::vector<double> testIndexu={4.0, 3.0, 5.0, 8.0, 1.0};
    std::vector<size_t> expectedu={4, 1, 0, 2, 3};
    REQUIRE(rcu.sort_indexes(testIndexu)==expectedu);
    REQUIRE(rcu.getVaR(.99)==8.0);

    RiskContribution<Lower> rcl(port);
    std::vector<double> testIndexl={4.0, 3.0, 5.0, 8.0, 1.0};
    std::vector<size_t> expectedl={3, 2, 0, 1, 4};
    REQUIRE(rcl.sort_indexes(testIndexl)==expectedl);
    REQUIRE(rcl.getVaR(.99)==1.0);
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
