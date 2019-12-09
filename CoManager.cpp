#include "CoManager.h"
#include<memory.h>
CoManager::CoManager(int coroutinemaxnum,int _cachenum):coroutines(coroutinemaxnum,Coroutine()),runid(-1),maxIndex(-1),cachenum(_cachenum),sharedStackpool(_cachenum)
{
	//sharedstack = new char[STACKSIZE];
	for (auto it = sharedStackpool.begin(); it != sharedStackpool.end(); ++it)
	{
		it->first = -1;
		it->second = (char*)malloc(STACKSIZE);
	}
	for (int i = 0; i < coroutines.size(); ++i)
	{
		coroutines[i].state = FREE;
	}
}
CoManager::~CoManager()
{
	//free(sharedstack);
	for (auto it = sharedStackpool.begin(); it != sharedStackpool.end(); ++it)
	{
		free(it->second);
	}
	for (int i = 0; i <= maxIndex; ++i)
	{
		if (coroutines[i].stack != NULL)
		{
			free(coroutines[i].stack);
		}
	}
}
void CoManager::coresume(int id)
{
	if (id<0 || id>maxIndex)
		return;
	switch (coroutines[id].state)
	{
	case RUNNABLE:
	{
		getcontext(&coroutines[id].ctx);
		coroutines[id].ctx.uc_stack.ss_sp = getsharedstack(id);//sharedstack;
		coroutines[id].ctx.uc_stack.ss_size = STACKSIZE;
		coroutines[id].ctx.uc_link = &main_ctx;
		runid = id;
		coroutines[id].state = RUNNING;
		makecontext(&coroutines[id].ctx, (void(*)(void))execFunc, 1, this);
		swapcontext(&main_ctx, &coroutines[id].ctx);
		break;
	}
	case SUSPEND:
	{
		bool isincache = isInCache(id);
		char* stack = getsharedstack(id);
		coroutines[id].ctx.uc_stack.ss_sp = stack;
		if (!isincache)//如果不在缓存中，复制将缓存区文件复制到工作区
			memcpy(stack + STACKSIZE - coroutines[id].size, coroutines[id].stack, coroutines[id].size);
		runid = id;
		coroutines[id].state = RUNNING;
		swapcontext(&main_ctx, &coroutines[id].ctx);
		break;
	}
	default:
		break;
	}
}
int CoManager::cocreate(Function _func)
{
	int id = 0;
	for (; id < coroutines.size(); ++id)
	{
		if (coroutines[id].state == FREE)
			break;
	}
	if (id >= coroutines.size())
	{
		coroutines.resize(2 * coroutines.size());
		for (int i = id; i < coroutines.size(); ++i)
		{
			coroutines[i].state = FREE;
		}
	}
	if (id > maxIndex)
		maxIndex = id;
	coroutines[id].func = _func;
	coroutines[id].state = RUNNABLE;
	getcontext(&coroutines[id].ctx);
	//coroutines[id].ctx.uc_stack.ss_flags = 0;
	//coroutines[id].ctx.uc_stack.ss_sp = coroutines[id].stack;
	//coroutines[id].ctx.uc_stack.ss_size = STACKSIZE;
	//coroutines[id].ctx.uc_link = &main_ctx;
	//makecontext(&coroutines[id].ctx, (void(*)(void))execFunc, 1, this);
	return id;
}
bool CoManager::cofinished()
{
	if (this->runid != -1)
		return false;
	else
	{
		for (int i = 0; i <= maxIndex; ++i)
		{
			if (coroutines[i].state != FREE)
				return false;
		}
		return true;
	}
}
void CoManager::codelete(int id)
{
	coroutines[id].state = FREE;
	if(coroutines[id].stack!=NULL)
		free(coroutines[id].stack);
}
void CoManager::execFunc(CoManager* c)
{
	int id = c->runid;
	if (id != -1)
	{
		c->coroutines[id].func();
		c->coroutines[id].state = FREE;
		if (c->isInCache(id))//协程运行结束后的收尾工作，清除缓存区，修改协程状态，释放堆空间
		{
			c->cachemap[id]->first = -1;
			c->cachemap.erase(id);
		}
		free(c->coroutines[id].stack);
		c->coroutines[id].stack = NULL;
		c->runid = -1;
	}
}

char * CoManager::getsharedstack(int id)
{
	if (cachemap.find(id) != cachemap.end())//如果在缓存中，直接用
	{
		sharedStackpool.push_front(*cachemap[id]);
		sharedStackpool.erase(cachemap[id]);
		cachemap[id] = sharedStackpool.begin();
		return sharedStackpool.begin()->second;
	}
	else//如果不在缓存中
	{
		if (cachemap.size() < cachenum)//如果缓存中还有还有剩余的位置
		{
			auto it = sharedStackpool.begin();
			for (; it != sharedStackpool.end(); ++it)
			{
				if (it->first == -1)
					break;
			}
			sharedStackpool.push_front(*it);
			sharedStackpool.erase(it);
			cachemap[id] = sharedStackpool.begin();
			sharedStackpool.begin()->first = id;
			return sharedStackpool.begin()->second;//将空间返回给使用者
		}
		else//如果缓存已经满了
		{
			auto it = (--sharedStackpool.end());//需要被清除的数据
			//将数据保存给对应的协程，然后给新协程使用
			if(coroutines[it->first].stack)
				free(coroutines[it->first].stack);
			coroutines[it->first].stack = (char*)malloc(coroutines[it->first].size);
			memcpy(coroutines[it->first].stack, it->second + STACKSIZE - coroutines[it->first].size, coroutines[it->first].size);
			//备份完成后将新的数据放进共享栈

			sharedStackpool.push_front(*it);
			sharedStackpool.pop_back();
			cachemap.erase(it->first);//删除原索引
			sharedStackpool.begin()->first = id;
			cachemap[id] = sharedStackpool.begin();//设置新的索引
			return sharedStackpool.begin()->second;
		}
	}
}

bool CoManager::isInCache(int id)
{
	if (cachemap.find(id) != cachemap.end())
		return true;
	else
		return false;
}

void CoManager::setCostacksize(int id)
{
	char dummy = 0;
	coroutines[id].size = cachemap[id]->second + STACKSIZE - &dummy;
	//coroutines[id].size = sharedstack + STACKSIZE - &dummy;
	//free(coroutines[id].stack);
	//coroutines[id].stack = (char*)malloc(coroutines[id].size);
	//memcpy(coroutines[id].stack, &dummy, coroutines[id].size);
}

void CoManager::coyield()
{
	if (runid != -1)
	{
		coroutines[runid].state = SUSPEND;
		int id = runid;
		//savestack(id);
		setCostacksize(id);//只设置好栈的长度，暂时不将数据移出共享区
		runid = -1;
		swapcontext(&coroutines[id].ctx, &main_ctx);
	}
}
