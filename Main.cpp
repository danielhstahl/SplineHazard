#include "LossSplineModel.h"
#include <vector>
#include <thread>
#include <string>
#include <unordered_map>
#include <sstream>
#include "SHazard.h"
#include "EGD.h"
#include "IGFrailty.h"
#include "DFrailty.h"
#include "Seasonality.h"
#include "LossSql.h"
#include "NodeCommunicate.h"
#include "document.h" //rapidjson
#include "writer.h" //rapidjson
#include "stringbuffer.h" //rapidjson

LossSplineModel model;
SHazard<Odds> haz;
EGD egd;
RNorm frailty;
Seasonality season;

//this should be provided by node?
const std::string connStr="Driver={SQL Server Native Client 11.0};Server=az-prd-dw1;Database=GLSDataWareHouse;Trusted_Connection=Yes;";

const std::vector<int> singleFloat={SQL_C_DOUBLE};//exposed by LossSql.h
const std::vector<int> descPlus2Float={SQL_C_CHAR, SQL_C_DOUBLE, SQL_C_DOUBLE};
const std::vector<int> descPlusFloat={SQL_C_CHAR, SQL_C_DOUBLE};
const std::vector<int> twoFloat={SQL_C_DOUBLE, SQL_C_DOUBLE};
const std::unordered_map<std::string, std::string> holdQueries={
    {"gamma",
        "SELECT Estimate FROM SandBox.PortSim.ModelParamEstimates WHERE paramType ='gamma' AND model='OddsHazard' AND ModelVersion=(SELECT MAX(ModelVersion) FROM SandBox.PortSim.ModelParamEstimates WHERE MOdel='OddsHazard')"},
    {"knots",
        "SELECT Estimate FROM SandBox.PortSim.ModelParamEstimates WHERE paramType ='knots' AND model='OddsHazard' AND ModelVersion=(SELECT MAX(ModelVersion) FROM SandBox.PortSim.ModelParamEstimates WHERE MOdel='OddsHazard')"},
    {"hazard",
        "SELECT t1.paramName, ParmEstimate, MeanEstimate FROM (SELECT Estimate as ParmEstimate, paramName FROM SandBox.PortSim.ModelParamEstimates WHERE model='OddsHazard' AND paramType='coef' AND ModelVersion=(SELECT MAX(ModelVersion) FROM SandBox.PortSim.ModelParamEstimates WHERE MOdel='OddsHazard')) t1 INNER JOIN (SELECT Estimate as meanEstimate, paramName FROM SandBox.PortSim.ModelParamEstimates WHERE model='OddsHazard' AND paramType='mean' AND ModelVersion=(SELECT MAX(ModelVersion) FROM SandBox.PortSim.ModelParamEstimates WHERE MOdel='OddsHazard')) t2 ON  t1.paramName=t2.paramName"},
    {"lgd",
        "SELECT CASE WHEN sqlField IS NULL THEN t1.paramName ELSE sqlField END as sqlField, Estimate FROM SandBox.PortSim.ModelParamEstimates t1 LEFT JOIN SandBox.PortSim.SQLParams t2 ON t1.paramName=t2.paramName AND t1.Model=t2.Model WHERE paramType='coef' AND t1.model='EGD' AND t1.ModelVersion=(SELECT MAX(ModelVersion) FROM SandBox.PortSim.ModelParamEstimates WHERE MOdel='EgD')"},
    {"lgdparam",
        "SELECT paramName, Estimate FROM SandBox.PortSim.ModelParamEstimates WHERE paramType ='param' AND model='EgD' AND ModelVersion=(SELECT MAX(ModelVersion) FROM SandBox.PortSim.ModelParamEstimates WHERE MOdel='EgD')"},
    {"lgderror",
        "SELECT Estimate FROM SandBox.PortSim.ModelParamEstimates WHERE paramType ='error' AND model='EgD' AND ModelVersion=(SELECT MAX(ModelVersion) FROM SandBox.PortSim.ModelParamEstimates WHERE MOdel='EgD')"},
    {"season", 
        "SELECT Adjustment FROM sandbox.portsim.seasonality WHERE SVersion=(SELECT MAX(SVersion) FROM SandBox.PortSim.Seasonality) ORDER BY MonthOfYear ASC"}
};
template<typename DB, typename callback>
void getRawData(int asOf, std::string& sqlQuery, DB db, std::vector<std::string>& hCoefs, std::vector<std::string>& lCoefs, std::string& id, callback& cb){
    double balance;
    double totalLoans;
    int includeMOB;
    std::string strAsOf=std::to_string(asOf);
    int totalLength=hCoefs.size()+lCoefs.size()+7;//first four of the data: monthsOnBook, monthBooked, etc, and last three APR, AmountFinanced, Collateral
    int cLength=hCoefs.size()+4;//to filter column num
    std::string coefs="";
    for(auto& v:hCoefs){
        coefs+=(", "+v);
    }
    for(auto& v:lCoefs){
        coefs+=(", "+v);
    }
    std::string monthsOnBook="DateDiff(m, BookingDate, DateAdd(m, -"+strAsOf+", GETDATE()))";
    //std::string timeRemaining="DateDiff(m, GETDATE(), MaturityDate)";
    //NEED MATURITY DATE
    std::string timeRemaining="72";
    std::string sqlAllDataQuery="SELECT "+monthsOnBook+" as monthsOnBook, Month(BookingDate) as monthBooked, "+timeRemaining+" as timeRemaining, CASE WHEN DefaultDate<=DateAdd(m, -"+strAsOf+", GETDATE()) THEN 1 ELSE 0 END As didDefault"+coefs+", APR, AmountFinanced, CollateralValue FROM SandBox.PortSim.LossesByRisk t1 INNER JOIN ("+sqlQuery+") t2 ON t1.LoanNumber=t2.LoanNumber Where EquifaxScore IS NOT NULL";
    //sendError(id, sqlAllDataQuery);//for testing!
    std::thread t1([&](){
        db.query(sqlAllDataQuery, totalLength, [&](auto& val, int row, int column){
            if(column==0){
                model.addTimeOnBook(val);
            }
            else if(column==1){
                model.addBookMonth(val);
            }
            else if(column==2){
                model.addTimeRemaining(val);
            }
            else if(column==3){
                model.addDefaultIndicator(val);
            }
            else if(column<cLength){
                haz.addAttribute(val);
            }
            else if(column<totalLength){
                egd.addAttribute(val);
            }
        }, [&](std::string& msg){
            sendError(id, msg);
        });
    });
    std::string getSummaryQuery="SELECT SUM(AmountFinanced) as totalFinanced, COUNT(AMountFinanced) as totalN, CASE WHEN DateDiff(d, MAX(EOMONTH(BookingDate)), MIN(EOMONTH(BookingDate)))=0 THEN 1 ELSE 0 END as includeMonthsOnBook  FROM SandBox.PortSim.LossesByRisk t1 INNER JOIN ("+sqlQuery+") t2 ON t1.LoanNumber=t2.LoanNumber WHERE EquifaXScore IS NOT NULL";
    //sendError(id, getSummaryQuery);//for testing!
    db.query(getSummaryQuery, 3,
        [&](auto& val, int row, int column){
            switch(column){
                case 0:
                    balance=val;
                    break;
                case 1:
                    totalLoans=val;
                    break;
                case 2:
                    includeMOB=(int)val;
                    break;
            }
        },
        [&](std::string& msg){
            sendError(id, msg);
        }
    );
    std::vector<double> MOB;
    std::vector<double> units;
    std::unordered_map<std::string, std::string> returnString;
    returnString["balance"]=std::to_string(balance);
    returnString["units"]=std::to_string(totalLoans);
    if(includeMOB==1){
        std::vector<double> unitLoss;
        std::vector<double> cumUnitLoss;
        std::vector<double> netLoss;
        std::vector<double> cumNetLoss;
        double rUnitLoss=0;
        double rNetLoss=0;
        int numLength=0;
        unitLoss.emplace_back(0);
        cumUnitLoss.emplace_back(0);
        netLoss.emplace_back(0);
        cumNetLoss.emplace_back(0);
        std::string getActualResultsQuery="SELECT monthsToDefault, COUNT(t1.LoanNumber) as UnitDefaults, SUM(DeficiencyBalance) as NetLoss FrOM (SELECT MIN("+monthsOnBook+") as minDate FROM (SELECT DateDiff(m, BookingDate, DefaultDate) as monthsToDefault, BookingDate, LoanNumber, CASE WHEN CHargeOffDate IS NOT NULL THEN DeficiencyBalance ELSE 0 END AS DeficiencyBalance FROM SandBox.LossMart.TestLossTable WHERE DefaultDate IS NOT NULL) t1 INNER JOIN ("+sqlQuery+") t2 ON t1.LoanNumber=t2.LoanNumber) t3, (SELECT DateDiff(m, BookingDate, DefaultDate) as monthsToDefault, BookingDate, LoanNumber, CASE WHEN CHargeOffDate IS NOT NULL THEN DeficiencyBalance ELSE 0 END AS DeficiencyBalance, PrinBalAtDefault FROM SandBox.LossMart.TestLossTable WHERE DefaultDate IS NOT NULL) t1 INNER JOIN ("+sqlQuery+") t2 ON t1.LoanNumber=t2.LoanNumber WHERE monthsToDefault<=minDate GROUP BY MonthsToDefault  ORDER BY MonthstoDefault ASC";
        db.query(getActualResultsQuery, 3, [&](auto& val, int row, int column){
            switch(column){
                case 0:                  
                    while(numLength<val){
                        unitLoss.emplace_back(0);
                        cumUnitLoss.emplace_back(rUnitLoss/totalLoans);
                        netLoss.emplace_back(0);
                        cumNetLoss.emplace_back(rNetLoss/balance);
                        numLength++;
                    }
                    break;
                case 1:
                    rUnitLoss+=val;
                    unitLoss.emplace_back(val);
                    cumUnitLoss.emplace_back(rUnitLoss/totalLoans);
                    numLength++;//only needed on one of case 1 and case 2
                    break;
                case 2:
                    rNetLoss+=val;
                    netLoss.emplace_back(val);
                    cumNetLoss.emplace_back(rNetLoss/balance);
                    break;
            }
        },
        [&](std::string& msg){
            //sendError(id, "{\"Error\":\""+msg+"\"}");
        });
        returnString["unitLoss"]=convertVectorToJson(unitLoss);
        returnString["cumUnitLoss"]=convertVectorToJson(cumUnitLoss);
        returnString["netLoss"]=convertVectorToJson(netLoss);
        returnString["cumNetLoss"]=convertVectorToJson(cumNetLoss);
    }
    std::thread t2([&](){
        std::vector<double> unitLossEvent;
        std::string getActualResultsQuery="SELECT COUNT(t1.LoanNumber) as UnitDefaults FROM (SELECT LoanNumber, CAST(EOMONTH(DefaultDate) as char(11)) as Calendar FROM SandBox.LossMart.TestLossTable WHERE DefaultDate IS NOT NULL AND DefaultDate<=EOMONTH(DateAdd(m, -"+strAsOf+"-1,GetDate())) aND DefaultDate>EOMONTH(DateAdd(m, -"+strAsOf+"-13,GetDate()))) t1 INNER JOIN ("+sqlQuery+") t2 ON t1.LoanNumber=t2.LoanNumber GROUP BY Calendar ORDER BY Calendar"; //This needs fixing for marts!
        db.query(getActualResultsQuery, 1, [&](auto& val, int row, int column){
            unitLossEvent.emplace_back(val);
        }, [&](std::string& msg){
            sendError(id, msg);
        });
        returnString["calendarLoss"]=convertVectorToJson(unitLossEvent);
    });
    t1.join();
    t2.join();
    cb(convertMapToJson<Number>(returnString));
}
template<typename Callback>
void init(int asOf, std::string& sqlQuery, std::string& id, Callback& cb){
    LossSQL db(connStr);
    std::vector<std::string> hCoefs;
    std::vector<std::string> lCoefs;
    std::thread t1([&](){
        db.query(holdQueries.at("gamma"),  [&](auto& val, int row, int column){
            haz.addGamma(val);
        },
        [&](std::string& msg){
            sendError(id, msg);
        });
    });
    std::thread t2([&](){
        db.query(holdQueries.at("season"),  [&](auto& val, int row, int column){
            season.addSeason(val);
        },
        [&](std::string& msg){
            sendError(id, msg);
        });
    });
    std::thread t3([&](){
        db.query(holdQueries.at("knots"), [&](auto& val, int row, int column){
            haz.addKnots(val);
        },
        [&](std::string& msg){
            sendError(id, msg);
        });
    });
    std::thread t4([&](){
        
        db.query(holdQueries.at("hazard"), descPlus2Float, 
            [&](auto& val, int row, int column){
                //sendError(id, val);
                hCoefs.emplace_back(val);
            },
            [&](auto& val, int row, int column){
                switch(column){
                    case 1:
                        haz.addCoefficient(val);
                        break;
                    case 2:
                        haz.addMean(val);
                        break;
                }
            },
            [&](std::string& msg){
                sendError(id, msg);
            }
        );
    });
    std::thread t5([&](){
        db.query(holdQueries.at("lgd"), descPlusFloat, 
            [&](auto& val, int row, int column){
                lCoefs.emplace_back(val);
            },
            [&](auto& val, int row, int column){
                egd.addCoefficient(val);
            },
            [&](std::string& msg){
                sendError(id, msg);
            }
        );
    });
    std::thread t6([&](){
        std::string key;
        db.query(holdQueries.at("lgdparam"), descPlusFloat, 
            [&](auto& val, int row, int column){
                key=val;
            },
            [&](auto& val, int row, int column){
                egd.idiosynracticParameters[key]=val;
            },
            [&](std::string& msg){
                sendError(id, msg);
            }
        );
    });
    std::thread t7([&](){
        std::string key;
        db.query(holdQueries.at("lgderror"), 
            [&](auto& val, int row, int column){
                egd.setStdError(val);
            },
            [&](std::string& msg){
                sendError(id, msg);
            }
        );
    });
    t4.join();//reqired prior to getRawData
    t5.join();//reqired prior to getRawData
    getRawData(asOf, sqlQuery, db, hCoefs, lCoefs,id, cb);
    t1.join();
    t2.join();
    t3.join();
    t6.join();
    t7.join();
    try{              
        haz.init();
        egd.init();
    }
    catch(int e){
        sendError(id, std::string("Wrong number of knots/gamma"));
    }
}
int main(){
    NodeCommunication nc; //exposes tcb, which is a pointer to a function (std::string&, const std::string&)
    nc.addEndPoint(std::string("getCNL"), [](rapidjson::Value& val,  std::string& id,  tcb cb){
        auto iterB=val.FindMember("balance");
        auto iterS=val.FindMember("seasonality");
        if(iterS==val.MemberEnd()||iterB==val.MemberEnd()){
            sendError(id, std::string("Requires seasonality and balance key!"));
            return;
        }
        if(iterS->value.GetBool()){
            model.createCNLCurve(
                iterB->value.GetDouble(), 
                [&](int a, const auto& b, double c){return haz.predict(a, b, c);}, 
                [&](int a, double b, double c){return egd.predict(a, b, c);},
                [&](double p1, double p2, int a, double b){return haz.adjustSeason(p1, p2, log(season.applySeasonality(a, b)));}, 
                [&](std::vector<double>& originationCurve, std::vector<double>& asOfCurve){
                    cb(id, "{\"originationCurve\":"+convertVectorToJson(originationCurve)+", \"asOfCurve\":"+convertVectorToJson(asOfCurve)+"}");
                }
            );
        }
        else{
             model.createCNLCurve(
                iterB->value.GetDouble(), 
                [&](int a, const auto& b, double c){return haz.predict(a, b, c);}, 
                [&](int a, double b, double c){return egd.predict(a, b, c);},
                [&](std::vector<double>& originationCurve, std::vector<double>& asOfCurve){
                    cb(id, "{\"originationCurve\":"+convertVectorToJson(originationCurve)+", \"asOfCurve\":"+convertVectorToJson(asOfCurve)+"}");
                }
            );
        }
    });
    nc.addEndPoint(std::string("getCUL"), [](rapidjson::Value& val,std::string& id,  tcb cb){
        auto iterS=val.FindMember("seasonality");
        if(iterS==val.MemberEnd()){
            sendError(id, std::string("Requires seasonality key!"));
            return;
        }
        if(iterS->value.GetBool()){
            model.createCULCurve(
                [&](int a, const auto& b, double c){return haz.predict(a, b, c);}, 
                [&](double p1, double p2, int a, double b){return haz.adjustSeason(p1, p2, log(season.applySeasonality(a, b)));}, 
                [&](std::vector<double>& originationCurve, std::vector<double>& asOfCurve){
                    cb(id, "{\"originationCurve\":"+convertVectorToJson(originationCurve)+", \"asOfCurve\":"+convertVectorToJson(asOfCurve)+"}");
                }
            );
        }
        else{
            model.createCULCurve(
                [&](int a, const auto& b, double c){return haz.predict(a, b, c);}, 
                [&](std::vector<double>& originationCurve, std::vector<double>& asOfCurve){
                    cb(id, "{\"originationCurve\":"+convertVectorToJson(originationCurve)+", \"asOfCurve\":"+convertVectorToJson(asOfCurve)+"}");
                }
            );
        }
    });
    nc.addEndPoint(std::string("getUnitForecast"), [](rapidjson::Value& val,  std::string& id,  tcb cb){
        auto iterS=val.FindMember("monthsOut");
        if(iterS==val.MemberEnd()){
            sendError(id, std::string("Requires monthsOut key!"));
            return;
        }
        std::vector<double> unitLoss=model.getUnitLoss(
            [&](int a, const auto& b, double c){return haz.predict(a, b, c);}, 
            [&](double p1, double p2, int a, double b){return haz.adjustSeason(p1, p2, log(season.applySeasonality(a, b)));},
            iterS->value.GetInt()
        );
        cb(id, "{\"allowance\":"+convertVectorToJson(unitLoss)+"}");
        
    });
    nc.addEndPoint(std::string("getNetForecast"), [](rapidjson::Value& val,  std::string& id,  tcb cb){
        auto iterS=val.FindMember("monthsOut");
        if(iterS==val.MemberEnd()){
            sendError(id, std::string("Requires monthsOut key!"));
            return;
        }
        std::vector<double> netLoss=model.getNetLoss(
            [&](int a, const auto& b, double c){return haz.predict(a, b, c);}, 
            [&](int a, double b, double c){return egd.predict(a, b, c);},
            [&](double p1, double p2, int a, double b){return haz.adjustSeason(p1, p2, log(season.applySeasonality(a, b)));},
            iterS->value.GetInt()
        );
        cb(id, "{\"allowance\":"+convertVectorToJson(netLoss)+"}");
    });
    nc.addEndPoint(std::string("getUnitLoss"), [](rapidjson::Value& val,  std::string& id,  tcb cb){
        auto iterS=val.FindMember("seasonality");
        if(iterS==val.MemberEnd()){
            sendError(id, std::string("Requires seasonality key!"));
            return;
        }
        if(iterS->value.GetBool()){
            std::vector<double> unitLoss=model.getTotalUnitLoss(
                [&](int a, const auto& b, double c){return haz.predict(a, b, c);}, 
                [&](double p1, double p2, int a, double b){return haz.adjustSeason(p1, p2, log(season.applySeasonality(a, b)));}
            );
            cb(id, "{\"asOfCurve\":"+convertVectorToJson(unitLoss)+"}");
        }
        else{
            std::vector<double> unitLoss=model.getTotalUnitLoss(
                [&](int a, const auto& b, double c){return haz.predict(a, b, c);}                
            );
            cb(id, "{\"asOfCurve\":"+convertVectorToJson(unitLoss)+"}");
        }
    });
    nc.addEndPoint(std::string("init"), [](rapidjson::Value& val, std::string& id,  tcb cb){
        auto iterAsOf=val.FindMember("asOf");
        auto iterSql=val.FindMember("sql");
        if(iterAsOf==val.MemberEnd()||iterSql==val.MemberEnd()){
            sendError(id, std::string("Requires asOf and sql key!"));
        }
        init(iterAsOf->value.GetInt(), std::string(iterSql->value.GetString()),  id, [&](std::string& message){
            cb(id, message);
        });
    });
    nc.addEndPoint(std::string("reset"), [](rapidjson::Value& val, std::string& id,  tcb cb){
        model.reset_all();
	    haz.reset_all();
	    egd.reset_all();
        cb(id, std::string("{\"Success\":true}"));
    });
    nc.addEndPoint(std::string("getParamQueries"), [](rapidjson::Value& val, std::string& id,  tcb cb){
        cb(id, convertMapToJson<String>(holdQueries));
    });
    nc.addEndPoint(std::string("simulate"), [](rapidjson::Value& val, std::string& id,  tcb cb){
        auto iterN=val.FindMember("n");
        auto iterType=val.FindMember("type");
        if(iterN==val.MemberEnd()||iterType==val.MemberEnd()){
            sendError(id, std::string("Requires n and type key!"));
        }
        try{
            if(!model.hasRun()){
                model.runPortfolio(
                    iterN->value.GetInt(), 
                    [&](int a, const auto& b, double c, double d){return haz.predict(a, b, c, d);}, 
                    [&](int a, double b, double c){return egd.predict(a, b, c);},
                    [&](){
                        return 0.0;//frailty.getNorm()*.25;
                    }
                );
            }
            switch(iterType->value.GetInt()){
                case 0:
                    send(id, model.getLosses());
                    break;
                case 1:
                    send(id, model.getRC());
                    break;
                default:
                    send(id, model.getLosses());
            }
        }
        catch(int e){
            sendError(id, std::string("MC Failed"));
        }

    });
    nc.start();
}
