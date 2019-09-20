#include <iostream>
#include <cassert>
#include <queue>
#include <map>
#include <cstdlib>
#include <sstream>

using namespace std;

struct node
{   // 存放的数据对象
    long double x;// 价格
    long long int y; // 时间搓
    string data,orderNo;
    node(long double x,long long int y,string data,string orderNo):x(x),y(y),data(data),orderNo(orderNo){}
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
        if (a->x == b->x) return a->y <= b->y;
        else return a->x > b->x;
    }
};

struct cmpAes {
    // 条件函数
/*    bool operator()(node a, node b) {
        if (a.x == b.x) return a.y >= b.y;
        else return a.x > b.x;
    }*/
    // 高的排在后面
    bool operator()(node* a, node* b) {
        if (a->x == b->x) return a->y <= b->y;
        else return a->x < b->x;
    }
};
static map<string , priority_queue<node,vector<node*>,cmpAes>*> containerBuy;
static map<string , priority_queue<node,vector<node*>,cmpDes>*> containerSell;
static map<string,long> lock;
map<string,string> toMap(string msg);
Array* toArray(string msg, char c);
void string2num(string str, long long int &num);
void string2num(string str, long double &num);
int getSize(int lenght,const char *ca, char c);
// extern priority_queue<node,vector<node>,cmp> pq;
int main() {
    // 自定义排序条件 优先级队列
    //map<string , pq> container;

    //container[""] = pq;
    node *a = new node(2151.1515123542,10000000000,"这是test1","2019121215648987");
    priority_queue<node,vector<node*>,cmpAes> *pq = containerBuy["test"];
    if (pq == NULL || (*pq).empty()){
        priority_queue<node,vector<node*>,cmpAes> *pq = new priority_queue<node,vector<node*>,cmpAes>;
        pq ->push(a);
        containerBuy["test"] = pq;
    } else{
        pq->push(a);
        //containerBuy["test"] = &pq;
    }

    node *b = new node(2152.1515123542,10000000000,"这是test2","2019122211526456");
    pq = containerBuy["test"];
    if (NULL == pq || (*pq).empty()){
        priority_queue<node,vector<node*>,cmpAes> *pq = new priority_queue<node,vector<node*>,cmpAes>;
        pq ->push(a);
        containerBuy["test"] = pq;
    } else{
        pq ->push(b);
        //containerBuy["test"] = pq;
    }
    pq = containerBuy["test"];
    while (!(pq == NULL) && !(*pq).empty()){
        std::cout << pq ->top()->data << std::endl;
        pq ->pop();
       // containerBuy["test"] = pq;
    }
    cout << rand() % 999999999 + 100000000 << " " ;
    cout << rand() % 999999999 + 100000000 << " " ;
    cout << rand() % 999999999 + 100000000 << " " ;

/*    long double my_num = 0.12345678999;
    long double my_num1 = 0.12345678998;
    //std::cout.precision(std::numeric_limits<cpp_dec_float_50>::digits10);
    std::cout << my_num << std::endl;*/

    //cpp_dec_float_50 bol = my_num.compare(my_num1);
  /*  if (my_num > my_num1)
    {
        std::cout << "my_num > my_num1" << std::endl;
    } else if(my_num < my_num1){
        std::cout << "my_num < my_num1" << std::endl;
    } else{
        std::cout << "my_num = my_num1" << std::endl;
    }

    long long int long1 = 1568935802000112;
    std::cout << long1 << std::endl;*/
    string r = "dsadsa=sadsad";
    int length = r.length();

    std::cout << "Hello, World!" << std::endl;

    return 0;
}


/**
 * 获取对象并上锁
 * @param symbol
 * @param isBuy
 * @return
 */
 node* getNode(string symbol,int &isBuy){
    if (isBuy == 1){
        // 获取购买队列
        priority_queue<node,vector<node*>,cmpAes> *pq = containerBuy[symbol];
        //队列为空
        if (pq == NULL){
            return NULL;
        } else{
            node *no = pq->top();
            if (no == NULL){
                return NULL;
            }
            // 判断是否上锁
            if (lock[no->orderNo] == NULL){
                // 是则返回状态
                return new node(0,0,"start=0","0");
            }
            // 进行上锁 并返回数据
            lock[no->orderNo] = rand() % 999999999 + 100000000;
            return no;
        }
    } else{
        // 获取出售队列
        priority_queue<node,vector<node*>,cmpDes> *pq = containerSell[symbol];
        if (pq == NULL){
            return NULL;
        } else{
            node *no = pq->top();
            if (no == NULL){
                return NULL;
            }
            if (lock[no->orderNo] == NULL){
                return new node(0,0,"start=0","0");
            }
            lock[no->orderNo] = rand() % 999999999 + 100000000;
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
 bool release(string orderNo,long lock_){
    if(lock[orderNo] == lock_){
        lock[orderNo] = NULL;
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
     long double s;
     string2num(map2["price"],s);
     long long int lll;
     string2num(map2["time"],lll);
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
     } else{
         priority_queue<node,vector<node*>,cmpDes> *pq = containerSell[map2["symbol"]];
         if (pq == NULL || (*pq).empty()){
             priority_queue<node,vector<node*>,cmpDes> *pq = new priority_queue<node,vector<node*>,cmpDes>;
             pq ->push(a);
             containerSell[map2["symbol"]] = pq;
         } else{
             pq->push(a);
             //containerBuy["test"] = &pq;
         }
     }

     return false;
 }

 map<string,string> toMap(string msg){
     map<string,string> map2;
     Array *array = toArray(msg,',');
     for (int i = 0; i <array->s->length(); ++i) {
        Array *array1 = toArray(array->s[i],'=');
        map2[array1->s[0]] = array1->s[1];
     }
     return map2;
 }

Array* toArray(string msg, char c){
     const char *ca = msg.c_str();
     Array *array = new Array();
     //return new string[0]{};
     int index = 0;
     for (int j = 0; j < msg.length(); ++j) {
         if (ca[j] == c){
             index++;
         } else{
             array->s[index] = array->s[index]+ca[j];
         }
     }
    return array;
 }

void string2num(string str, long long int &num)
{
    stringstream ss;
    ss << str;
    ss >> num;
}

void string2num(string str, long double &num)
{
    stringstream ss;
    ss << str;
    ss >> num;
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
