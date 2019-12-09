#pragma once
#include<vector>
#include<functional>
#include<ucontext.h>
#include<list>
#include<map>
using namespace std;

#define STACKSIZE (1024*1024)
typedef std::function<void()> Function;
enum  COSTATE { FREE = 0, RUNNABLE, RUNNING, SUSPEND };
struct Coroutine
{
	ucontext_t ctx;//Э��������
	Function func;//Э��ִ�еĺ���
	COSTATE state;//��ǰЭ��״̬
	char* stack=NULL;//Э�̱�����ʱ����������
	int size;//����������Ĵ�С
};
class CoManager
{
public:
	CoManager(int coroutinemaxnum = 1024,int cachenum=16);
	~CoManager();
	void coresume(int id);//���Ѷ�Ӧid��Э��
	void coyield();//ֹͣ��ǰЭ�̲����ص���Э��
	int cocreate(Function func);//����һ��Э��
	bool cofinished();//ȷ���Ƿ�����Э�̶��Ѿ�ִ�����
	void codelete(int id);//ɾ����ӦidЭ�̲��ͷŶ�Ӧ�ռ�
private:
	vector<Coroutine> coroutines;
	ucontext_t main_ctx;//��Э��������
	int runid;//����ִ�е�Э��id
	int maxIndex;//ʹ�ù������Э��id
	static void execFunc(CoManager* c);
	int cachenum;//���湲��ջ�ĸ���
	list<pair<int,char*>> sharedStackpool;//����ջ��������,���Э��id�Ͷ�Ӧ�Ĺ���ջ��ַ
	map<int, list<pair<int, char*> >::iterator> cachemap;//����ջ��������,���Э��id��ָ��list�ж�Ӧ��ĵ�����
	char* getsharedstack(int id);//�ڲ�ʹ�ã�����Э�̻�ȡ����ջ��ַ
	bool isInCache(int id);//�ڲ�ʹ�ã��ж�Э���Ƿ��ڹ���ջ��
	void setCostacksize(int id);//����Э��ִ��ջ�ĳ���
};

