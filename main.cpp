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
{   // ��ŵ����ݶ���
    long double x;// �۸�
    long long int y; // ʱ���
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
    // ��������
/*    bool operator()(node a, node b) {
        if (a.x == b.x) return a.y >= b.y;
        else return a.x > b.x;
    }*/
    // �ߵ����ں���
    bool operator()(node* a, node* b) {
        if (a->x == b->x) return a->y > b->y;
        else return a->x > b->x;
    }
};

struct cmpAes {
    // ��������
/*    bool operator()(node a, node b) {
        if (a.x == b.x) return a.y >= b.y;
        else return a.x > b.x;
    }*/
    // �ߵ�����ǰ��
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
        zmq_msg_recv(&recv_msg, z_socket, 0);//��������
        string strData = (char *) zmq_msg_data(&recv_msg);
        zmq_msg_close(&recv_msg);
#endif

#ifdef linux
        char buffer[1024];
        zmq_recv(z_socket, buffer, 1024, 0);
        strData = buffer;
#endif

        if (!strData.empty()) {
            // ��ָ���������
            Array *array = toArray(strData,' ', false);

            std::cout << "�յ�ָ�" << array->s[0] << ",������" << array->s[1] << endl;
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
                    // ���ָ��ΪNULL
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
                            "push ����й������һ�����ݣ�"
                            "getOne ȡ�����е�һ�����ݲ�������"
                            "release ���ݲ������н�����"
                            "getAll ���ݲ�����ȡָ�����������ݣ�"
                            "del ɾ��ָ�������ݣ���������ɾ��������������ѯ������"
                            "update �������ݣ�����Կ�׵Ŀͻ��˲��ܸ��ġ�\"}");
                }
            }catch (...) {
                result.append("{\"start\":\"false\",\"data\":\"������������������\"}");
            }
            //std::cout << (char*)zmq_msg_data(&recv_msg) << std::endl;
            // װ�������ڴ�
            int size = result.size();
#ifdef __WINDOWS_
            zmq_msg_init_size(&send_msg, size);
            memcpy(zmq_msg_data(&send_msg), result.c_str(), size);//���÷�������
            zmq_sendmsg(z_socket, &send_msg, 0);//����
            zmq_msg_close(&send_msg);//�ر�ͨ��
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
            Sleep(1000);// ����1�� ���������
        #endif

        #ifdef linux
            sleep(1);// ����1�� �������
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
 * ��ȡ��������
 * @param symbol
 * @param isBuy
 * @return
 */
 node* getOne(string msg){
    map<string,string> map2 = toMap(msg);
    string symbol = map2["symbol"];
    int isBuy = string2int(map2["isBuy"]);
    if (isBuy == 1){
        // ��ȡ�������
        priority_queue<node,vector<node*>,cmpAes> *pq = containerBuy[symbol];
        //����Ϊ��
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
            // �ж��Ƿ�����
            if ((*lock)[no->orderNo.c_str()] != NULL){
                // ���򷵻�״̬
                return new node(0,0,"{\"start\":0}","0");
            }
            // �������� ����������
            (*lock)[no->orderNo.c_str()] = rand() % 999999999 + 100000000;
            string lock_str = long2str((*lock)[no->orderNo.c_str()]);
            no->data.append(",\"lock\":"+lock_str);
            time_t t;
            time(&t);
            t += outTime_;
            outNode *out = new outNode(t,no->orderNo);//��������ʱ����
            if (conrainerLock == NULL){
                conrainerLock = new priority_queue<outNode,vector<outNode*>,lockDes>();
            }
            conrainerLock->push(out);//���볬ʱ����
            return no;
        }
    } else{
        // ��ȡ���۶���
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
            outNode *out = new outNode(t,no->orderNo);//��������ʱ����
            if (conrainerLock == NULL){
                conrainerLock = new priority_queue<outNode,vector<outNode*>,lockDes>();
            }
            conrainerLock->push(out);//���볬ʱ����
            return no;
        }
    }
}

/**
 * ���ݶ�����źͶ�Ӧ�������н����ͷ�
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
  * ͨ�����紫��������ݲ����������
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
 * ��ȡָ��ָ�����ͺͱ��ֵ�ָ��������
 * @param size ��ѯ����
 * @param orderTye ��������
 * @param symbol ����
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
         // ��ȡ������ ������ָ��
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
         // ��ȡ������ ������ָ��
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
     //�ж���Ҫɾ���������Ƿ�����
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
         // ��ȡ������ ������ָ��
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
            pq->pop();//ɾ������������
            return push(msg);
        }
    } else{
        priority_queue<node,vector<node*>,cmpDes> *pq = containerSell[symbol];
        pq->top();
        string orderNo = pq->top()->orderNo;
        long oldLock = (*lock)[orderNo.c_str()];
        if(oldLock == lock_){
            (*lock)[orderNo.c_str()] = NULL;
            pq->pop();//ɾ������������
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
