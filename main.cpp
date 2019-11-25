#include <iostream>
#include <cassert>
#include <queue>
#include <map>
#include <cstdlib>
#include <sstream>

#ifdef WIN32
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
#include <cstring>


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
static map<string , map<string,long double >*> containerBuyDepth;
static map<string , map<string,long double >*> containerSellDepth;
static map<string , priority_queue<node,vector<node*>,cmpAes>*> linkedBuy;
static map<string , priority_queue<node,vector<node*>,cmpDes>*> linkedSell;
static map<string,long> *lock = new map<string,long>;
static map<string,int> *rem = new map<string,int>;
static priority_queue<outNode,vector<outNode*>,lockDes> *conrainerLock = new priority_queue<outNode,vector<outNode*>,lockDes>();;
static int start = 1;
static long outTime_ = 0;
static int wait = 0;

node* getOne(string msg);
bool release(string msg);
bool push(string msg);
string getAll(string msg);
string getDepthAll(string msg);
string del(string msg);
bool update(string msg);
map<string,string> toMap(string msg);
void toArray(string msg, char c, bool isMap,string array[]);
long long int string2lli(string str);
int string2int(string str);
long string2long(string str);
string long2str(long l);
string double2str(long double l);
long double string2ldouble(string str);
int getSize(int lenght,const char *ca, char c);
string bool2string(bool bo);
void outTime();
void wait_();
void wait_release();
#ifdef linux
    void *thread(void *ptr);
#endif

// extern priority_queue<node,vector<node>,cmp> pq;
int main() {
    cout << "start" << endl;
    rr::RrConfig config;
    config.ReadConfig("../config.ini");
    string url = config.ReadString("ZMQ", "url", "");
    outTime_ = config.ReadInt("System","lockOutTime",30);
    cout << "bind: " << url << "；lock out time :" << outTime_ << endl;
    #ifdef WIN32
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


#ifdef WIN32
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
        string strData = buffer;
#endif

        if (!strData.empty()) {
            // 【指令】【参数】
            //Array *array =
            string array[255];
            toArray(strData,' ', false,array);
            //std::cout << "收到指令：" << array[0] << ",参数：" << array[1] << endl;
            string result = "";
            try {
                if (array[0] == "push") {
                    bool bo = push(array[1]);
                    result.append("{\"start\":\"" + bool2string(bo)+"\"}");
                } else if (array[0] == "update") {
                    bool bo = update(array[1]);
                    result.append("{\"start\":\"" + bool2string(bo)+"\"}");
                } else if (array[0] == "getOne") {
                    node *no = getOne(array[1]);
                    // 如果指向为NULL
                    if (no == NULL) {
                        result.append("{\"start\":\"false\",\"data\":{}}");
                    } else {
                        result.append("{\"start\":\"true\"");
                        result.append(",\"data\":" + no->data+"}");
                    }
                    //result.append("{\"start\":\"false\",\"data\":{}}");
                    cout << "返回数据：" << result << endl;
                } else if (array[0] == "release") {
                    bool bo = release(array[1]);
                    result.append("{\"start\":\"" + bool2string(bo)+"\"}");
                } else if (array[0] == "getAll") {
                    string data = getAll(array[1]);
                    if (data == "") {
                        result.append("{\"start\":\"false\"}");
                    } else {
                        result.append("{\"start\":\"true\"");
                        result.append(",\"data\":" + data+"}");
                    }
/*                    result.append("{\"start\":\"true\"");
                    result.append(",\"data\":[]}");*/

                } else if (array[0] == "del") {

                    string data = del(array[1]);
                    if (data == "") {
                        result.append("{\"start\":\"false\"}");
                    } else {
                        result.append("{\"start\":\"true\"");
                        result.append(",\"data\":" + data+"}");
                    }
                }else if (array[0] == "getDepthAll") {
                    string data = getDepthAll(array[1]);
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
                            "getDepthAll 查询指定条件的深度数据"
                            "del 删除指定的数据；不会立即删除，但会立即查询不到；"
                            "update 更新数据，持有钥匙的客户端才能更改。\"}");
                }
            }catch (...) {
                result.append("{\"start\":\"false\",\"data\":\"错误的请求，请检查参数！\"}");
            }

            //std::cout << (char*)zmq_msg_data(&recv_msg) << std::endl;
            // 装载数据内存
            int size = result.size();
#ifdef WIN32
            zmq_msg_init_size(&send_msg, size);
            memcpy(zmq_msg_data(&send_msg), result.c_str(), size);//设置返回数据
            zmq_sendmsg(z_socket, &send_msg, 0);//发送
            zmq_msg_close(&send_msg);//关闭通道
#endif

#ifdef linux
            zmq_send(z_socket, result.c_str(), size, 0);
            memset(buffer,'\0',sizeof(buffer));
#endif
        }

    }
    zmq_close(z_socket);
    zmq_term(context);

    return 0;
}


void outTime(){
    int is = 0;
    while (start){
        if (is == 0){
        #ifdef WIN32
            Sleep(1000);// 休眠1秒 按毫秒计算
        #endif

        #ifdef linux
            sleep(1);// 休眠1秒 按秒计算
        #endif

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
        long old_lock = (*lock)[no->orderNo];
        if (no->t <= t && old_lock != 0){
            (*lock)[no->orderNo] = 0;
            lock->erase(no->orderNo);
            conrainerLock->pop();
            is = 1;
            cout << "订单：" << no->orderNo << "锁超时！" << endl;
        } else{
            if (old_lock == 0){
                //订单已经被释放
                conrainerLock->pop();
            }
            is = 0;
        }
    }
}
#ifdef linux
    void *thread(void *ptr)
    {
        outTime();
        return 0;
    }
#endif
/**
 * 获取对象并上锁
 * @param symbol
 * @param isBuy
 * @return
 */
 node* getOne(string msg){
    map<string,string> map2 = toMap(msg);
    string symbol = map2["symbol"];
    string isBuy = map2["type"];
    if (isBuy == "Buy"){
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
            while (start){
                int rm = (*rem)[no->orderNo];
                if (rm != 0){
                    pq->pop();
                    (*rem)[no->orderNo] = 0;
                    rem->erase(no->orderNo);
                    no = pq->top();
                } else{
                    break;
                }
            }
            // 判断是否上锁
            if ((*lock)[no->orderNo] != 0){
                // 是则返回状态
                return new node(0,0,"{\"start\":0}","0");
            }
            // 进行上锁 并返回数据
            (*lock)[no->orderNo] = rand() % 999999999 + 100000000;
            string lock_str = long2str((*lock)[no->orderNo]);
            node *no_ = new node(0,0,no->data+",\"lock\":"+lock_str,"0");
            time_t t;
            time(&t);
            t += outTime_;
            outNode *out = new outNode(t,no->orderNo);//创建锁超时数据
            conrainerLock->push(out);//放入超时队列
            return no_;
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
            while (start){
                int rm = (*rem)[no->orderNo];
                if (rm != 0){
                    pq->pop();
                    (*rem)[no->orderNo] = 0;
                    rem->erase(no->orderNo);
                    no = pq->top();
                } else{
                    break;
                }
            }
            if ((*lock)[no->orderNo] != 0){
                return new node(0,0,"{start:0}","0");
            }
            (*lock)[no->orderNo] = rand() % 999999999 + 100000000;
            //no->data =
            string lock_str = long2str((*lock)[no->orderNo]);
            node *no_ = new node(0,0,no->data+",\"lock\":"+lock_str,"0");
            time_t t;
            time(&t);
            t += outTime_;
            outNode *out = new outNode(t,no->orderNo);//创建锁超时数据
            conrainerLock->push(out);//放入超时队列
            return no_;
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
    long oldLock = (*lock)[orderNo];
    if(oldLock == lock_){
        (*lock)[orderNo] = 0;
        lock->erase(orderNo);
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
     if ((*rem)[map2["orderNo"]] == 1){
         (*rem)[map2["orderNo"]] = 0;
         rem->erase(map2["orderNo"]);
         return true;
         //return (*rem).erase(map2["orderNo"]);
     }

     string price = map2["price"];
     string time = map2["createTime"];
     string symbol = map2["symbol"];
     long double s = string2ldouble(price);
     long long int lll = string2lli(time);
     node *a = new node(
            s,
            lll,
            msg,
            map2["orderNo"]
            );
     if (map2["type"] == "Buy"){
         priority_queue<node,vector<node*>,cmpAes> *pq = containerBuy[symbol];
         if (pq == NULL || (*pq).empty()){
             priority_queue<node,vector<node*>,cmpAes> *pq = new priority_queue<node,vector<node*>,cmpAes>;
             pq ->push(a);
             containerBuy[map2["symbol"]] = pq;
         } else{
             pq->push(a);
             //containerBuy["test"] = &pq;
         }
         map<string,long double> *depth = containerBuyDepth[symbol];
         priority_queue<node,vector<node*>,cmpAes> *de_link = linkedBuy[symbol];
         if (depth == NULL){
             depth = new map<string,long double>;
             containerBuyDepth[symbol] = depth;
         }
         if (de_link == NULL){
             de_link = new priority_queue<node,vector<node*>,cmpAes>;
             linkedBuy[symbol] = de_link;
         }
         long double val = (*depth)[price];
         if (val == 0){
             //之前不存在该价格的交易
             node *a_ = new node(s,0,price,"");
             de_link->push(a_);
             (*depth)[price] = string2ldouble(map2["amount"]);
         } else{
             (*depth)[price] = val + string2ldouble(map2["amount"]);
         }

         return true;
     } else if (map2["type"] == "Sell"){
         priority_queue<node,vector<node*>,cmpDes> *pq = containerSell[symbol];
         if (pq == NULL || (*pq).empty()){
             priority_queue<node,vector<node*>,cmpDes> *pq = new priority_queue<node,vector<node*>,cmpDes>;
             pq ->push(a);
             containerSell[map2["symbol"]] = pq;
         } else{
             pq->push(a);
             //containerBuy["test"] = &pq;
         }
         map<string,long double> *depth = containerSellDepth[symbol];
         priority_queue<node,vector<node*>,cmpDes> *de_link = linkedSell[symbol];
         if (depth == NULL){
             depth = new map<string,long double>;
             containerSellDepth[symbol] = depth;
         }
         if (de_link == NULL){
             de_link = new priority_queue<node,vector<node*>,cmpDes>;
             linkedSell[symbol] = de_link;
         }
         long double val = (*depth)[price];
         if (val == 0){
             //之前不存在该价格的交易
             node *a_ = new node(s,0,price,"");
             de_link->push(a_);
             (*depth)[price] = string2ldouble(map2["amount"]);
         } else{
             (*depth)[price] = val + string2ldouble(map2["amount"]);
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
     string orderTye = map2["type"];
     string symbol = map2["symbol"];
     string result = "";
     result.append("[");
     if(orderTye == "Buy"){
         // 获取到对象 而不是指针
         if (containerBuy[symbol] != NULL){
             priority_queue<node,vector<node*>,cmpAes> pq = *containerBuy[symbol];
             if (!pq.empty()){
                 if (size == -1){
                     size = pq.size();
                 }
                 for (int i = 0; i < size; ++i) {
                     node *no = pq.top();
                     if (!pq.empty()){
                         long lock_ = (*lock)[no->orderNo];
                         int  remLo = (*rem)[no->orderNo];
                         if (lock_ != 0 || remLo != 0){
                             pq.pop();
                             continue;
                         }
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
                 if (size == -1){
                     size = pq.size();
                 }
                 for (int i = 0; i < size; ++i) {
                     node *no = pq.top();
                     if (!pq.empty()){
                         long lock_ = (*lock)[no->orderNo];
                         int  remLo = (*rem)[no->orderNo];
                         if (lock_ != 0 || remLo != 0){
                             pq.pop();
                             continue;
                         }
                         if (i != 0){
                             result.append(",");
                         }
                         if (no->data[0] != '{'){
                             result.append("{"+no->data+"}");
                         } else{
                             result.append(no->data);
                         }
                         pq.pop();
                     }else{
                         break;
                     }
                 }
             }
         }
     }
    result.append("]");
     return result;
 }

/**
 * 查询指定条件的深度数据
 * @param msg
 * @return
 */
 string getDepthAll(string msg){
    map<string,string> map2 = toMap(msg);
    int size = string2int(map2["size"]);
    string orderTye = map2["type"];
    string symbol = map2["symbol"];
    string result = "";
    result.append("[");
    if(orderTye == "Buy"){
        map<string,long double> *depth = containerBuyDepth[symbol];
        // 获取到对象 而不是指针
        if (depth != NULL){
            priority_queue<node,vector<node*>,cmpAes> pq = *linkedBuy[symbol];
            if (!pq.empty()){
                if (size == -1){
                    size = pq.size();
                }
                long double price_odl = 0;
                for (int i = 0; i < size; ++i) {
                    node *no = pq.top();
                    if (!pq.empty()){
                        long lock_ = (*lock)[no->orderNo];
                        long  remLo = (*rem)[no->orderNo];
                        if (lock_ != 0 || remLo != 0 || price_odl == no->x){
                            pq.pop();
                            continue;
                        }
                        if (i != 0){
                            result.append(",");
                        }
                        string key = no->data;
                        string value = double2str((*depth)[key]);
                        if (value == "0"){
                            i--;
                            pq.pop();
                            continue;
                        }
                        result.append("{\""+key+"\":"+value+"}");
                        price_odl = no->x;
                        pq.pop();
                    } else{
                        break;
                    }
                }
            }
        }
    } else{
        // 获取到对象 而不是指针
        map<string,long double> *depth = containerSellDepth[symbol];
        if (depth != NULL){
            priority_queue<node,vector<node*>,cmpDes> pq = *linkedSell[symbol];
            if (!pq.empty()){
                if (size == -1){
                    size = pq.size();
                }
                long double price_odl = 0;
                for (int i = 0; i < size; ++i) {
                    node *no = pq.top();
                    if (!pq.empty()){
                        long lock_ = (*lock)[no->orderNo];
                        long  remLo = (*rem)[no->orderNo];
                        if (lock_ != 0 || remLo != 0 || price_odl == no->x){
                            pq.pop();
                            continue;
                        }
                        if (i != 0){
                            result.append(",");
                        }
                        string key = no->data;
                        string value = double2str((*depth)[key]);
                        if (value == "0"){
                            i--;
                            pq.pop();
                            continue;
                        }
                        result.append("{"+key+","+value+"}");
                        price_odl = no->x;
                        pq.pop();
                    }else{
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

     //判断需要删除的数据是否上锁
     long oldLock = (*lock)[orderNo];
     if (oldLock != 0){
         long lock_ = string2long(map2["lock"]);
         if(lock_ == 0 || oldLock != lock_){
             return "";
         }
     }
     string orderTye = map2["type"];
     string symbol = map2["symbol"];
     string sequence = map2["sequence"];

     string result;
     if(orderTye == "Buy"){
         if (containerBuy[symbol] != NULL){
             priority_queue<node,vector<node*>,cmpAes> pq = *containerBuy[symbol];
             while (!pq.empty()){
                 node *no = pq.top();
                 pq.pop();
                 if (no->orderNo == orderNo){
                     if (no->data[0] != '{'){
                         result.append("{"+no->data+"}");
                     } else{
                         result.append(no->data);
                     }
                     map<string,string> map_or = toMap(result);
                     if (sequence.empty() || sequence != map_or["sequence"]){
                         return "";
                     }
                     map<string,long double> *depth = containerBuyDepth[symbol];
                     priority_queue<node,vector<node*>,cmpAes> *de_link = linkedBuy[symbol];
                     //priority_queue<node,vector<node*>,cmpAes> *de_link_copy = new priority_queue<node,vector<node*>,cmpAes>;
                     list<node*> no_list;
                     while (!de_link->empty()){
                         node *no_ = de_link->top();
                         de_link->pop();
                         if (no->x == no_->x){
                             long double val = (*depth)[map_or["price"]];
                             (*depth)[map_or["price"]] = val - string2ldouble(map2["amount"]);
                             while (!no_list.empty()){
                                 de_link->push(no_list.front());
                                 no_list.pop_front();//删除第一个元素
                             }
                             break;
                         }
                         no_list.push_back(no_);
                     }
                     if (!de_link->empty()){
                         while (!no_list.empty()){
                             de_link->push(no_list.front());
                             no_list.pop_front();//删除第一个元素
                         }
                     }
                     (*rem)[no->orderNo] = 1;
                     break;
                 }
             }
         }
     } else{
         // 获取到对象 而不是指针
         if (containerSell[symbol] != NULL){
             priority_queue<node,vector<node*>,cmpDes> pq = *containerSell[symbol];

             while (!pq.empty()){
                 node *no = pq.top();
                 pq.pop();
                 if (no->orderNo == orderNo){
                     if (no->data[0] != '{'){
                         result.append("{"+no->data+"}");
                     } else{
                         result.append(no->data);
                     }
                     map<string,string> map_or = toMap(result);
                     if (sequence.empty() || sequence != map_or["sequence"]){
                         return "";
                     }
                     map<string,long double> *depth = containerBuyDepth[symbol];
                     priority_queue<node,vector<node*>,cmpAes> *de_link = linkedBuy[symbol];
                     //priority_queue<node,vector<node*>,cmpAes> *de_link_copy = new priority_queue<node,vector<node*>,cmpAes>;
                     list<node*> no_list;
                     while (!de_link->empty()){
                         node *no_ = de_link->top();
                         de_link->pop();
                         if (no->x == no_->x){
                             long double val = (*depth)[map_or["price"]];
                             (*depth)[map_or["price"]] = val - string2ldouble(map2["amount"]);
                             while (!no_list.empty()){
                                 de_link->push(no_list.front());
                                 no_list.pop_front();//删除第一个元素
                             }
                             break;
                         }
                         no_list.push_back(no_);
                     }
                     if (!de_link->empty()){
                         while (!no_list.empty()){
                             de_link->push(no_list.front());
                             no_list.pop_front();//删除第一个元素
                         }
                     }
                     (*rem)[no->orderNo] = 1;
                     break;
                 }
             }
         }
     }
     if (result == ""){
         //rem->erase(orderNo);
         result = "{}";
     }
     return result;
 }

bool update(string msg){
    map<string,string> map2 = toMap(msg);
    string orderTye = map2["type"];
    string symbol = map2["symbol"];
    long lock_ = string2long(map2["lock"]);

    if(orderTye == "Buy"){
        priority_queue<node,vector<node*>,cmpAes> *pq = containerBuy[symbol];
        node *no = pq->top();
        long oldLock = (*lock)[no->orderNo];
        if(oldLock == lock_){
            /*pq->pop();//删除顶部的数据
            return push(msg);*/
            no->data = msg;
        }
    } else{
        priority_queue<node,vector<node*>,cmpDes> *pq = containerSell[symbol];
        node *no = pq->top();
        long oldLock = (*lock)[no->orderNo];
        if(oldLock == lock_){
            no->data = msg;
        }
    }
    return false;
 }

 map<string,string> toMap(string msg){
     map<string,string> map2;
     //Array *array =
     string array[255];
     toArray(msg,',', true,array);
     for (int i = 0; i < 25; ++i) {
         if (array[i] != ""){
             string array1[255];
             //Array *array1 =
             toArray(array[i],':',true,array1);
             map2[array1[0]] = array1[1];
         } else{
             return map2;
         }
     }
     return map2;
 }

void toArray(string msg, char c, bool isMap,string array[]){
     const char *ca = msg.c_str();
     //Array *array = new Array();
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
                 array[index] = array[index]+ca[j];
             }
             //return array;
         } else{
             array[index] = array[index]+ca[j];
         }
     }
   // return array;
 }

long long int string2lli(string str)
{
    long long int result;
    istringstream is(str);
    is >> result;
    //long long int num = result;
    return result;
}

int string2int(string str)
{
    int result;
    istringstream is(str);
    is >> result;
    //long long int num = result;
    return result;
}

long string2long(string str)
{
    long result;
    istringstream is(str);
    is >> result;
    //long long int num = result;
    return result;
}

long double string2ldouble(string str)
{
    long double result;
    istringstream is(str);
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

string double2str(long double l){
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

 void wait_(){
     while (wait){
        #ifdef WIN32
                 Sleep(10);// 休眠1秒 按毫秒计算
        #endif

        #ifdef linux
                 sleep(0.01);// 休眠1秒 按秒计算
        #endif
     }
     wait = 1;
 }

 void wait_release(){
     wait = 0;
 }
