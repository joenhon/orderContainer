//
// Created by 23947 on 2019/12/25.
//
#ifdef WIN32
#include <io.h>
#endif
#ifdef linux
#include <cstdio>
#include <cstdlib>
#include <zconf.h>
#endif
#include <fcntl.h>
#include <sstream>
#include "Log.h"

string Log::pathLdea(const string &suffix) {
    string re;
    for (int i = 0; i < paths->length(); ++i) {
        if (!paths[i].empty()){
            re.append(paths[i]);

            if (pathms[i] == "date"){
                re.append(getCurrentTimeStr("%Y-%m-%d"));
            }

            if (pathms[i] == "suffix"){
                re.append(suffix);
            }
        } else{
            break;
        }
    }
    return re;
}

string Log::getCurrentTimeStr(const string &format) {
    time_t t = time(NULL);
    char ch[64] = {0};
    strftime(ch, sizeof(ch) - 1, format.c_str(), localtime(&t));     //年-月-日 时-分-秒 "%Y-%m-%d %H:%M:%S"
    return ch;
}

void Log::init(const string &path_) {
    path = path_;
    const char *ca = path.c_str();
    string pa;
    bool pas = false;
    int in = 0;
    for (int i = 0;i < path.length();i++){
        if (ca[i] == '{'){
            pas = true;
            continue;
        } else if (ca[i] == '}'){
            pas = false;
            pathms[in] = pa;
            pa = "";
            in++;
            continue;
        }
        if (pas){
            stringstream stream;
            stream << ca[i];
            pa.append(stream.str());
        } else{
            paths[in] = paths[in]+ca[i];
        }
    }
}

void Log::info(const string &buf) {
    wt(buf,"info");
}

void Log::error(const string &buf) {
    wt(buf,"error");
}

void Log::wt(const string &buf, const string &suffix) {
    string buf_ = getCurrentTimeStr("%Y-%m-%d %H:%M:%S").append("--").append(buf).append("\n");
    int fd, len = strlen(buf_.c_str());
    if((fd = open(pathLdea(suffix).c_str(), O_CREAT | O_WRONLY | O_APPEND, 0600)) < 0){
        create(pathLdea(suffix));
        if((fd = open(pathLdea(suffix).c_str(), O_CREAT | O_WRONLY | O_APPEND, 0600)) < 0) {
            perror("open");
            exit(1);
        }
    }
    write(fd, buf_.c_str(), len);
    close(fd);
}

bool Log::create(const string& path_) {
    const char *cs = path_.c_str();
    string mar,pa;
    for(int i = 0; i < path_.length();i++){
        stringstream stream;
        stream << cs[i];
        mar.append(stream.str());
        if(cs[i] == '/'){
            pa.append(mar);
            mar = "";
        }
    }
#ifdef WIN32

#endif
#ifdef linux
    string command = "mkdir  -p " + pa;
    system(command.c_str());
#endif
    return true;
}
