//
// Created by 23947 on 2019/12/25.
//

#ifndef ORDERCONTAINER_LOG_H
#include <string.h>
#include <iostream>
#include <map>
#include <ctime>

#define ORDERCONTAINER_LOG_H

using namespace std;
class Log {
private:
    string path;
    string paths[10];
    string pathms[10];

public:
    Log(){

    }
    ~Log(){

    }
    void init(const string& path_);
    void info(const string& buf);
    void error(const string& buf);
    string pathLdea(const string& suffix);

    string  getCurrentTimeStr(const string& format);

private:
    void wt(const string& buf, const string& suffix);
    bool create(const string& path_);
};


#endif //ORDERCONTAINER_LOG_H
