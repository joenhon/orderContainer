#include <iostream>
#include <cassert>
#include <queue>
#include <map>
#include <cstdlib>
#include <sstream>

#ifdef __WINDOWS_
    #include <thread>
    #include <synchapi.h>
    #include <zmq_utils.h>
#endif

#ifdef linux
    #include <pthread.h>
    #include <unistd.h>
#endif
#include <zmq.h>
#include "include/RrConfig.h"
#include <list>
#include <time.h>


using namespace std;


struct node
{   // 存放的数据对象
    long double x;// 价格
    long long int y; // 时间搓
    string data,orderNo;
    node(long double x,long long int y,string data,string orderNo):x(x),y(y),data(data),orderNo(orderNo){}
};

struct outNode{
    time_t t;
    string orderNo;
    outNode(time_t t,string orderNo):orderNo(orderNo),t(t){}
};

struct Array{
    string s[25];
};

struct cmpDes {
    // 条件函数
/*    bool operator()(node a, node b) {
        if (a.x == b.x) return a.y >= b.y;
        else return a.x > b.x;
    }*/
    // 高的排在后面
    bool operator()(node* a, node* b) {
        if (a->x == b->x) return a->y > b->y;
        else return a->x > b->x;
    }
};

struct cmpAes {
    // 条件函数
/*    bool operator()(node a, node b) {
        if (a.x == b.x) return a.y >= b.y;
        else return a.x > b.x;
    }*/
    // 高的排在前面
    bool operator()(node* a, node* b) {
        if (a->x == b->x) return a->y > b->y;
        else return a->x < b->x;
    }
};

struct lockDes{
    bool operator()(outNode* a,outNode* b){
        return a->t > b->t;
    }
};

static map<string , priority_queue<node,vector<node*>,cmpAes>*> containerBuy;
static map<string , priority_queue<node,vector<node*>,cmpDes>*> containerSell;
static map<string,long> *lock = new map<string,long>;
static map<string,int> *rem = new map<string,int>;
static priority_queue<outNode,vector<outNode*>,lockDes> *conrainerLock;
static int start = 1;
static long outTime_ = 0;

node* getOne(string msg);
bool release(string msg);
bool push(string msg);
string getAll(string msg);
string del(string msg);
bool update(string msg);
map<string,string> toMap(string msg);
Array* toArray(string msg, char c, bool map);
long long int string2lli(string str);
int string2int(string str);
long string2long(string str);
string long2str(long l);
long double string2ldouble(string str);
int getSize(int lenght,const char *ca, char c);
string bool2string(bool bo);
void outTime();
void *thread(void *ptr);

// extern priority_queue<node,vector<node>,cmp> pq;
int main() {
    cout << "start" << endl;
    rr::RrConfig config;
    config.ReadConfig("../config.ini");
    string url = config.ReadString("ZMQ", "url", "");
    outTime_ = config.ReadInt("System","lockOutTime",0);
    cout << "bind: " << url << endl;
    #ifdef __WINDOWS_
        thread outTimeThread(outTime);
        void* context = zmq_init(1);
        void* z_socket = zmq_socket(context, ZMQ_REP);
    #endif
    #ifdef linux
        pthread_t id;
        int ret = pthread_create(&id, NULL, thread, NULL);
        if(ret) {
            cout << "Create pthread error!" << endl;
            return 1;
        }
    void *context = zmq_ctx_new();
    void *z_socket = zmq_socket(context, ZMQ_REP);
    #endif


    zmq_bind(z_socket, url.c_str());
    while (start) {


        string strData ="";
#ifdef __WINDOWS_
        zmq_msg_t recv_msg;
        zmq_msg_t send_msg;
        zmq_msg_init(&recv_msg);
        zmq_msg_recv(&recv_msg, z_socket, 0);//接受数据
        string strData = (char *) zmq_msg_data(&recv_msg);
        zmq_msg_close(&recv_msg);
#endif

#ifdef linux
        char buffer[1024];
        zmq_recv(z_socket, buffer, 1024, 0);
        strData = buffer;
#endif

        if (!strData.empty()) {
            // 【指令】【参数】
            Array *array = toArray(strData,' ', false);

            std::cout << "收到指令：" << array->s[0] << ",参数：" << array->s[1] << endl;
            string result = "";
            try {
                if (array->s[0] == "push") {
                    bool bo = push(array->s[1]);
                    result.append("{\"start\":\"" + bool2string(bo)+"\"}");
                } else if (array->s[0] == "update") {
                    bool bo = update(array->s[1]);
                    result.append("{\"start\":\"" + bool2string(bo)+"\"}");
                } else if (array->s[0] == "getOne") {
                    node *no = getOne(array->s[1]);
                    // 如果指向为NULL
                    if (no == NULL) {
                        result.append("{\"start\":\"true\",\"data\":{}");
                    } else {
                        result.append("{\"start\":\"true\"");
                        result.append(",\"data\":" + no->data+"}");
                    }
                } else if (array->s[0] == "release") {
                    bool bo = release(array->s[1]);
                    result.append("{\"start\":\"" + bool2string(bo)+"\"}");
                } else if (array->s[0] == "getAll") {
                    string data = getAll(array->s[1]);
                    if (data == "") {
                        result.append("{\"start\":\"false\"}");
                    } else {
                        result.append("{\"start\":\"true\"");
                        result.append(",\"data\":" + data+"}");
                    }

                } else if (array->s[0] == "del") {
                    string data = del(array->s[1]);
                    if (data == "") {
                        result.append("{\"start\":\"false\"}");
                    } else {
                        result.append("{\"start\":\"true\"");
                        result.append(",\"data\":" + data+"}");
                    }
                } else {
                    result.append(
                            "{\"start\":\"true\",data:\""
                            "push 向队列管理添加一条数据；"
                            "getOne 取到排列第一的数据并上锁；"
                            "release 根据参数进行解锁；"
                            "getAll 根据参数获取指定数量的数据；"
                            "del 删除指定的数据；不会立即删除，但会立即查询不到；"
                            "update 更新数据，持有钥匙的客户端才能更改。\"}");
                }
            }catch (...) {
                result.append("{\"start\":\"false\",\"data\":\"错误的请求，请检查参数！\"}");
            }
            //std::cout << (char*)zmq_msg_data(&recv_msg) << std::endl;
            // 装载数据内存
            int size = result.size();
#ifdef __WINDOWS_
            zmq_msg_init_size(&send_msg, size);
            memcpy(zmq_msg_data(&send_msg), result.c_str(), size);//设置返回数据
            zmq_sendmsg(z_socket, &send_msg, 0);//发送
            zmq_msg_close(&send_msg);//关闭通道
#endif

#ifdef linux
            zmq_send(z_socket, result.c_str(), size, 0);
#endif
        }

    }
    zmq_close(z_socket);
    zmq_term(context);
    std::cout << "Hello, World!" << std::endl;

    return 0;
}


void outTime(){
    int is = 0;
    while (1){
        if (is == 0){
        #ifdef __WINDOWS_
            Sleep(1000);// 休眠1秒 按毫秒计算
        #endif

        #ifdef linux
            sleep(1);// 休眠1秒 按秒计算
        #endif

        }
        if (conrainerLock == NULL){
            conrainerLock = new priority_queue<outNode,vector<outNode*>,lockDes>();
        }
        if (conrainerLock->empty()){
            continue;
        }
        outNode *no = conrainerLock->top();
        if (no == NULL){
            continue;
        }
        time_t t;
        time(&t);
        if (no->t <= t){
            (*lock)[no->orderNo] = NULL;
            is = 1;
        } else{
            is = 0;
        }
    }
}

void *thread(void *ptr)
{
    outTime();
    return 0;
}

/**
 * 获取对象并上锁
 * @param symbol
 * @param isBuy
 * @return
 */
 node* getOne(string msg){
    map<string,string> map2 = toMap(msg);
    string symbol = map2["symbol"];
    int isBuy = string2int(map2["isBuy"]);
    if (isBuy == 1){
        // 获取购买队列
        priority_queue<node,vector<node*>,cmpAes> *pq = containerBuy[symbol];
        //队列为空
        if (pq == NULL || pq->empty()){
            return NULL;
        } else{
            node *no = pq->top();
            if (no == NULL){
                return NULL;
            }
            if ((*rem)[no->orderNo.c_str()] != NULL){
                pq->pop();
                (*rem).erase(no->orderNo.c_str());
                return getOne(msg);
            }
            // 判断是否上锁
            if ((*lock)[no->orderNo.c_str()] != NULL){
                // 是则返回状态
                return new node(0,0,"{\"start\":0}","0");
            }
            // 进行上锁 并返回数据
            (*lock)[no->orderNo.c_str()] = rand() % 999999999 + 100000000;
            string lock_str = long2str((*lock)[no->orderNo.c_str()]);
            no->data.append(",\"lock\":"+lock_str);
            time_t t;
            time(&t);
            t += outTime_;
            outNode *out = new outNode(t,no->orderNo);//创建锁超时数据
            if (conrainerLock == NULL){
                conrainerLock = new priority_queue<outNode,vector<outNode*>,lockDes>();
            }
            conrainerLock->push(out);//放入超时队列
            return no;
        }
    } else{
        // 获取出售队列
        priority_queue<node,vector<node*>,cmpDes> *pq = containerSell[symbol];
        if (pq == NULL || pq->empty()){
            return NULL;
        } else{
            node *no = pq->top();
            if (no == NULL){
                return NULL;
            }
            if ((*rem)[no->orderNo.c_str()] != NULL){
                pq->pop();
                (*rem).erase(no->orderNo.c_str());
                return getOne(msg);
            }
            if ((*lock)[no->orderNo.c_str()] != NULL){
                return new node(0,0,"{start:0}","0");
            }
            (*lock)[no->orderNo.c_str()] = rand() % 999999999 + 100000000;
            //no->data =
            string lock_str = long2str((*lock)[no->orderNo.c_str()]);
            no->data.append(",\"lock\":"+lock_str);
            time_t t;
            time(&t);
            t += outTime_;
            outNode *out = new outNode(t,no->orderNo);//创建锁超时数据
            if (conrainerLock == NULL){
                conrainerLock = new priority_queue<outNode,vector<outNode*>,lockDes>();
            }
            conrainerLock->push(out);//放入超时队列
            return no;
        }
    }
}

/**
 * 根据订单编号和对应的锁序列进行释放
 * @param orderNo
 * @param lock_
 * @return
 */
 bool release(string msg){
    map<string,string> map2 = toMap(msg);
    string orderNo = map2["orderNo"];
    long lock_ = string2long(map2["lock"]);
    long oldLock = (*lock)[orderNo.c_str()];
    if(oldLock == lock_){
        (*lock)[orderNo.c_str()] = NULL;
        return true;
    } else{
        return false;
    }
 }

 /**
  * 通过网络传输接受数据并处理入队列
  * @param msg
  * @return
  */
 bool push(string msg){
    /**
     *  "id=1,orderNo=1231231,orderTye=sell...."
     */
     map<string,string> map2 = toMap(msg);
     string price = map2["price"];
     string time = map2["time"];
     long double s = string2ldouble(price);
     long long int lll = string2lli(time);
     cout << s << endl;
     node *a = new node(
            s,
            lll,
            msg,
            map2["orderNo"]
            );
     if (map2["isBuy"].compare("1") == 0){
         priority_queue<node,vector<node*>,cmpAes> *pq = containerBuy[map2["symbol"]];
         if (pq == NULL || (*pq).empty()){
             priority_queue<node,vector<node*>,cmpAes> *pq = new priority_queue<node,vector<node*>,cmpAes>;
             pq ->push(a);
             containerBuy[map2["symbol"]] = pq;
         } else{
             pq->push(a);
             //containerBuy["test"] = &pq;
         }
         return true;
     } else if (map2["isBuy"].compare("0") == 0){
         priority_queue<node,vector<node*>,cmpDes> *pq = containerSell[map2["symbol"]];
         if (pq == NULL || (*pq).empty()){
             priority_queue<node,vector<node*>,cmpDes> *pq = new priority_queue<node,vector<node*>,cmpDes>;
             pq ->push(a);
             containerSell[map2["symbol"]] = pq;
         } else{
             pq->push(a);
             //containerBuy["test"] = &pq;
         }
         return true;
     }
     return false;
 }
/**
 * 获取指定指定类型和币种的指定条数据
 * @param size 查询数量
 * @param orderTye 订单类型
 * @param symbol 币种
 * @return
 */
 string getAll(string msg){
     map<string,string> map2 = toMap(msg);
     int size = string2int(map2["size"]);
     string orderTye = map2["orderTye"];
     string symbol = map2["symbol"];
     string result = "";
     result.append("[");
     if((orderTye) == "buy"){
         // 获取到对象 而不是指针
         if (containerBuy[symbol] != NULL){
             priority_queue<node,vector<node*>,cmpAes> pq = *containerBuy[symbol];
             if (!pq.empty()){
                 for (int i = 0; i < size; ++i) {
                     node *no = pq.top();
                     if (!pq.empty() && (*lock)[no->orderNo.c_str()] == NULL){
                         if (i != 0){
                             result.append(",");
                         }
                         if (no->data[0] != '{'){
                             result.append("{"+no->data+"}");
                         } else{
                             result.append(no->data);
                         }
                         pq.pop();
                     } else{
                         break;
                     }
                 }
             }
         }
     } else{
         // 获取到对象 而不是指针
         if (containerSell[symbol] != NULL){
             priority_queue<node,vector<node*>,cmpDes> pq = *containerSell[symbol];
             if (!pq.empty()){
                 for (int i = 0; i < size; ++i) {
                     node *no = pq.top();
                     if (!pq.empty() && (*lock)[no->orderNo.c_str()] == NULL){
                         if (i != 0){
                             result.append(",");
                         }
                         if (no->data[0] != '{'){
                             result.append("{"+no->data+"}");
                         } else{
                             result.append(no->data);
                         }
                         pq.pop();
                     } else{
                         break;
                     }
                 }
             }
         }
     }
    result.append("]");
     return result;
 }

 string del(string msg){
     map<string,string> map2 = toMap(msg);
     string orderNo = map2["orderNo"];
     string orderTye = map2["orderTye"];
     string symbol = map2["symbol"];
     string result = "";
     //判断需要删除的数据是否上锁
     if ((*lock)[orderNo.c_str()] != NULL){
         return result;
     }
     if((orderTye) == "buy"){
         if (containerBuy[symbol] != NULL){
             priority_queue<node,vector<node*>,cmpAes> pq = *containerBuy[symbol];
             if (!pq.empty()){
                 for (int i = 0; i < pq.size(); ++i) {
                     node *no = pq.top();
                     if (!pq.empty() && no->orderNo == orderNo){
                         if (no->data[0] != '{'){
                             result.append("{"+no->data+"}");
                         } else{
                             result.append(no->data);
                         }
                         break;
                     } else{
                         break;
                     }
                 }
             }
         }
     } else{
         // 获取到对象 而不是指针
         if (containerSell[symbol] != NULL){
             priority_queue<node,vector<node*>,cmpDes> pq = *containerSell[symbol];
             if (!pq.empty()){
                 for (int i = 0; i < pq.size(); ++i) {
                     node *no = pq.top();
                     if (!pq.empty() && no->orderNo == orderNo){
                         if (no->data[0] != '{'){
                             result.append("{"+no->data+"}");
                         } else{
                             result.append(no->data);
                         }
                         break;
                     } else{
                         break;
                     }
                 }
             }
         }
     }
     return result;
 }

bool update(string msg){
    map<string,string> map2 = toMap(msg);
    string orderTye = map2["orderTye"];
    string symbol = map2["symbol"];
    long lock_ = string2long(map2["lock"]);

    if((orderTye) == "buy"){
        priority_queue<node,vector<node*>,cmpAes> *pq = containerBuy[symbol];
        string orderNo = pq->top()->orderNo;
        long oldLock = (*lock)[orderNo.c_str()];
        if(oldLock == lock_){
            (*lock)[orderNo.c_str()] = NULL;
            pq->pop();//删除顶部的数据
            return push(msg);
        }
    } else{
        priority_queue<node,vector<node*>,cmpDes> *pq = containerSell[symbol];
        pq->top();
        string orderNo = pq->top()->orderNo;
        long oldLock = (*lock)[orderNo.c_str()];
        if(oldLock == lock_){
            (*lock)[orderNo.c_str()] = NULL;
            pq->pop();//删除顶部的数据
            return push(msg);
        }
    }
    return false;
 }

 map<string,string> toMap(string msg){
     map<string,string> map2;
     Array *array = toArray(msg,',', true);
     for (int i = 0; i < 25; ++i) {
         if (array->s[i] != ""){
             Array *array1 = toArray(array->s[i],':',true);
             map2[array1->s[0]] = array1->s[1];
         } else{
             return map2;
         }
     }
     return map2;
 }

Array* toArray(string msg, char c,bool isMap){
     const char *ca = msg.c_str();
     Array *array = new Array();
     //return new string[0]{};
     int index = 0;
     for (int j = 0; j < msg.length(); ++j) {
         if (isMap && (ca[j] == '{' || ca[j] == '\"')){
             continue;
         }
         if (ca[j] == c){
             index++;
         } else if (ca[j] == '}'){
             if (!isMap){
                 array->s[index] = array->s[index]+ca[j];
             }
             return array;
         } else{
             array->s[index] = array->s[index]+ca[j];
         }
     }
    return array;
 }

long long int string2lli(string str)
{
    long long int result;
    istringstream is(str.c_str());
    is >> result;
    //long long int num = result;
    return result;
}

int string2int(string str)
{
    int result;
    istringstream is(str.c_str());
    is >> result;
    //long long int num = result;
    return result;
}

long string2long(string str)
{
    long result;
    istringstream is(str.c_str());
    is >> result;
    //long long int num = result;
    return result;
}

long double string2ldouble(string str)
{
    long double result;
    istringstream is(str.c_str());
    is >> result;
    //long double num = result;
    return result;
    /*stringstream ss;
    ss << str;
    ss >> num;*/
}

string bool2string(bool bo){
   if (bo){
       return "true";
   } else{
       return "false";
   }
 }

string long2str(long l){
    ostringstream os;
    os<<l;
    string result;
    istringstream is(os.str());
    is>>result;
    return result;
}

 int getSize(int lenght,const char *ca, char c){
     int size = 0;
     for (int i = 0; i < lenght; ++i) {
         if (ca[i] == c){
             size++;
         }
     }
     return size;
 }
