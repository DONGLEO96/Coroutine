#pragma once
#include<vector>
#include<functional>
#include<ucontext.h>
#include<list>
#include<map>
#include<stack>
using namespace std;

#define STACKSIZE (1024*1024)
typedef std::function<void()> Function;
enum  COSTATE { FREE = 0, UNINIT, STARTED };
struct Coroutine
{
	ucontext_t ctx;//协程上下文
	Function func;//协程执行的函数
	COSTATE state;//当前协程状态
	char* stack=NULL;//协程被换出时保存上下文
	int size;//保存的上下文大小
};
class CoManager
{
public:
	CoManager(int coroutinemaxnum = 1024,int cachenum=16);
	~CoManager();
	void coresume(int id);//唤醒对应id的协程
	void coyield();//停止当前协程并返回到主协程
	int cocreate(Function func);//创建一个协程
	bool cofinished();//确认是否所有协程都已经执行完毕
	void codelete(int id);//删除对应id协程并释放对应空间
private:
	vector<Coroutine> coroutines;
	ucontext_t main_ctx;//主协程上下文
	int runid;//正在执行的协程id
	int maxIndex;//使用过的最大协程id
	int cachenum;//缓存共享栈的个数
	stack<int> co_record;//用于记录协程的调用层次
	static void execFunc(CoManager* c);
	vector<pair<char*, int>> shareStackpool;
	char* getsharedstack(int id);//内部使用，帮助协程获取共享栈地址
	bool isInCache(int id);//内部使用，判断协程是否在共享栈中
	void setCostacksize(int id);//计算协程执行栈的长度
};

