#ifndef __LOSSSQL__
#define __LOSSSQL__
#include <iostream>
#include <windows.h>
#include <sqltypes.h>
#include <string>
#include <sql.h>
#include <sqlext.h>


template<typename err>
void show_error(unsigned int handletype, const SQLHANDLE& handle, err& errorCB){
    SQLCHAR sqlstate[1024];
    SQLCHAR message[1024];
    if(SQL_SUCCESS == SQLGetDiagRec(handletype, handle, 1, sqlstate, NULL, message, 1024, NULL)){
        errorCB(std::string((const char*)message));
    }
}
/**
Simple wrapper class around MS SQL driver.  Designed for speed, not ease of use.
*/
class LossSQL{
private:
    SQLCHAR * conn_str;
    SQLSMALLINT len;
	SQLHANDLE sqlenvhandle;    
	SQLRETURN retcode;
 public:
    /**
    Constructor
    @param connString Connection string of the form "Driver={SQL Server Native Client 11.0};Server=Server;Database=Database;Trusted_Connection=Yes;"
    */
    LossSQL(const std::string& connString){
        SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &sqlenvhandle);
        SQLSetEnvAttr(sqlenvhandle,SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
        conn_str = (SQLCHAR*)(connString.c_str());
        len = static_cast<SQLSMALLINT>(connString.length());
    }
    ~LossSQL(){
	    SQLFreeHandle(SQL_HANDLE_ENV, sqlenvhandle);
    }
    /**
    Overloaded function to query and return character or double results. 
    @param sqlQuery Select query to execute
    @param cols Vector of integers of type SQL_C_CHAR and SQL_C_DOUBLE.  
    These are in the same order as the selection in the query.
    @param cb1 Callback(result, row, column) for character fields
    @param cb2 Callback(result, row, column) for double fields
    @param errorMsg Function that takes a string describing any errors to handle errors
    */
    template<typename callback1, typename callback2, typename err>
    void query(const std::string& sqlQuery, const std::vector<int>& cols, callback1& cb1, callback2& cb2, err& errorMsg){
        SQLCHAR retconstring[1024];
        SQLHANDLE sqlconnectionhandle;
        SQLAllocHandle(SQL_HANDLE_DBC, sqlenvhandle, &sqlconnectionhandle);
        SQLDriverConnect (sqlconnectionhandle, NULL, 
				conn_str, 
				len, retconstring, 1024, NULL,SQL_DRIVER_NOPROMPT);
        SQLHANDLE sqlstatementhandle;
        SQLAllocHandle(SQL_HANDLE_STMT, sqlconnectionhandle, &sqlstatementhandle);
        if(SQL_SUCCESS!=SQLExecDirect(sqlstatementhandle, (SQLCHAR*)(sqlQuery.c_str()), SQL_NTS)){
			show_error(SQL_HANDLE_STMT, sqlstatementhandle, errorMsg);
		}
		else {
            int numRow=0;
			while(SQLFetch(sqlstatementhandle)==SQL_SUCCESS){
                int i=1;
                for(auto& col:cols){
                    switch(col){
                        case SQL_C_CHAR:
                            char myValc[256];
                            SQLGetData(sqlstatementhandle, i, col, myValc, 64, NULL);
                            cb1(std::string(myValc), numRow, i-1);
                            break;
                        case SQL_C_DOUBLE:
                            double myVald;
                            SQLGetData(sqlstatementhandle, i, col, &myVald, 0, NULL);
                            cb2(myVald, numRow, i-1);
                            break;
                    }
                    i++;
                }
                numRow++;
			}
		}
        SQLFreeHandle(SQL_HANDLE_STMT, sqlstatementhandle );
        SQLDisconnect(sqlconnectionhandle);
	    SQLFreeHandle(SQL_HANDLE_DBC, sqlconnectionhandle);
    }
    /**
    Overloaded function to query and return double result with a default error message
    @param sqlQuery Select query to execute
    @param cb Callback(result, row, column) assuming single double field
    */
    template<typename callback>
    void query(const std::string& sqlQuery, callback& cb){
        query(sqlQuery, cb, [](std::string& msg){std::cout<<msg<<std::endl;});
    }
    /**
    Overloaded function to query and return double result
    @param sqlQuery Select query to execute
    @param cb Callback(result, row, column) assuming single double field
    @param errorMsg Function that takes a string describing any errors to handle errors
    */
    template<typename callback, typename err>
    void query(const std::string& sqlQuery, callback& cb, err& errorMsg){ //default is double.  When grabbing lots of data, want as fast as possible
        SQLCHAR retconstring[1024];
        SQLHANDLE sqlconnectionhandle;
        SQLAllocHandle(SQL_HANDLE_DBC, sqlenvhandle, &sqlconnectionhandle);
        SQLDriverConnect (sqlconnectionhandle, NULL, 
				conn_str, 
				len, retconstring, 1024, NULL,SQL_DRIVER_NOPROMPT);
        SQLHANDLE sqlstatementhandle;
        SQLAllocHandle(SQL_HANDLE_STMT, sqlconnectionhandle, &sqlstatementhandle);
        if(SQL_SUCCESS!=SQLExecDirect(sqlstatementhandle, (SQLCHAR*)(sqlQuery.c_str()), SQL_NTS)){
			show_error(SQL_HANDLE_STMT, sqlstatementhandle, errorMsg);
		}
		else {
            int numRow=0;
			while(SQLFetch(sqlstatementhandle)==SQL_SUCCESS){
                double myVald;
                SQLGetData(sqlstatementhandle, 1, SQL_C_DOUBLE, &myVald, 0, NULL);
                cb(myVald, numRow, 0);
                numRow++;
			}
		}
        SQLFreeHandle(SQL_HANDLE_STMT, sqlstatementhandle );
        SQLDisconnect(sqlconnectionhandle);
	    SQLFreeHandle(SQL_HANDLE_DBC, sqlconnectionhandle);
    }
    /**
    Overloaded function to query and return double results
    @param sqlQuery Select query to execute
    @param numCol Number of fields reutrned in the query
    @param cb Callback(result, row, column) for double fields
    @param errorMsg Function that takes a string describing any errors to handle errors
    */
    template<typename callback, typename err>
    void query(const std::string& sqlQuery, int numCol, callback& cb, err& errMsg){ //default is double.  When grabbing lots of data, want as fast as possible
        SQLCHAR retconstring[1024];
        SQLHANDLE sqlconnectionhandle;
        SQLAllocHandle(SQL_HANDLE_DBC, sqlenvhandle, &sqlconnectionhandle);
        SQLDriverConnect (sqlconnectionhandle, NULL, 
				conn_str, 
				len, retconstring, 1024, NULL,SQL_DRIVER_NOPROMPT);
        SQLHANDLE sqlstatementhandle;
        SQLAllocHandle(SQL_HANDLE_STMT, sqlconnectionhandle, &sqlstatementhandle);
        if(SQL_SUCCESS!=SQLExecDirect(sqlstatementhandle, (SQLCHAR*)(sqlQuery.c_str()), SQL_NTS)){
			show_error(SQL_HANDLE_STMT, sqlstatementhandle, errMsg);
		}
		else {
            int numRow=0;
			while(SQLFetch(sqlstatementhandle)==SQL_SUCCESS){
                for(int i=0; i<numCol;++i){
                    double myVald;
                    SQLGetData(sqlstatementhandle, i+1, SQL_C_DOUBLE, &myVald, 0, NULL);
                    cb(myVald, numRow, i);
                }
                
                numRow++;
			}
		}
        SQLFreeHandle(SQL_HANDLE_STMT, sqlstatementhandle );
        SQLDisconnect(sqlconnectionhandle);
	    SQLFreeHandle(SQL_HANDLE_DBC, sqlconnectionhandle);
    }
};
#endif