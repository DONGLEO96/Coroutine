
#include "CoManager.h"
#include<memory.h>
CoManager::CoManager(int coroutinemaxnum, int _cachenum) :coroutines(coroutinemaxnum, Coroutine()), runid(-1), maxIndex(-1), cachenum(_cachenum), shareStackpool(_cachenum)
{
	//sharedstack = new char[STACKSIZE];
	for (auto it = shareStackpool.begin(); it != shareStackpool.end(); ++it)
	{
		it->first = (char*)malloc(STACKSIZE);
		it->second = -1;
	}
	for (int i = 0; i < coroutines.size(); ++i)
	{
		coroutines[i].state = FREE;
	}
}
CoManager::~CoManager()
{
	//free(sharedstack);
	for (auto it = shareStackpool.begin(); it != shareStackpool.end(); ++it)
	{
		free(it->first);
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
	ucontext_t* currctx = NULL;
	if (co_record.empty())
	{
		currctx = &main_ctx;
	}
	else
	{
		currctx = &coroutines[co_record.top()].ctx;
	}
	co_record.push(id);
	switch (coroutines[id].state)
	{
	case UNINIT:
	{
		getcontext(&coroutines[id].ctx);
		coroutines[id].ctx.uc_stack.ss_sp = getsharedstack(id);//sharedstack;
		coroutines[id].ctx.uc_stack.ss_size = STACKSIZE;
		coroutines[id].ctx.uc_link = currctx;
		runid = id;
		coroutines[id].state = STARTED;
		makecontext(&coroutines[id].ctx, (void(*)(void))execFunc, 1, this);
		swapcontext(currctx, &coroutines[id].ctx);
		break;
	}
	case STARTED:
	{
		bool isincache = isInCache(id);
		char* stack = getsharedstack(id);
		coroutines[id].ctx.uc_stack.ss_sp = stack;
		
		if (!isincache)//如果不在缓存中，复制将缓存区文件复制到工作区
		{
			memcpy(stack + STACKSIZE - coroutines[id].size, coroutines[id].stack, coroutines[id].size);
		}
		runid = id;

		coroutines[id].ctx.uc_link = currctx;
		
		swapcontext(currctx, &coroutines[id].ctx);
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
	coroutines[id].state = UNINIT;
	//getcontext(&coroutines[id].ctx);
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
	if (coroutines[id].stack != NULL)
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
			c->shareStackpool[id%c->shareStackpool.size()].second = -1;
		}
		free(c->coroutines[id].stack);
		c->coroutines[id].stack = NULL;
		c->co_record.pop();
		if (c->co_record.empty())
			c->runid = -1;
		else
			c->runid = c->co_record.top();
	}
}

char * CoManager::getsharedstack(int id)
{
	if (shareStackpool[id%shareStackpool.size()].second == id)//如果在缓存中，直接使用
	{
		return shareStackpool[id%shareStackpool.size()].first;
	}
	else
	{
		if (shareStackpool[id%shareStackpool.size()].second == -1)//如果还没有使用过
		{
			shareStackpool[id%shareStackpool.size()].second = id;
			return shareStackpool[id%shareStackpool.size()].first;
		}
		else//如果被别人使用了
		{
			int currid = shareStackpool[id%shareStackpool.size()].second;
			if (coroutines[currid].stack != NULL)
				free(coroutines[currid].stack);
			coroutines[currid].stack = (char*)malloc(coroutines[currid].size);
			memcpy(coroutines[currid].stack, shareStackpool[id%shareStackpool.size()].first + STACKSIZE - coroutines[currid].size, coroutines[currid].size);
			shareStackpool[id%shareStackpool.size()].second = id;
			return shareStackpool[id%shareStackpool.size()].first;
		}
	}
	//if (cachemap.find(id) != cachemap.end())//如果在缓存中，直接用
	//{
	//	sharedStackpool.push_front(*cachemap[id]);
	//	sharedStackpool.erase(cachemap[id]);
	//	cachemap[id] = sharedStackpool.begin();
	//	return sharedStackpool.begin()->second;
	//}
	//else//如果不在缓存中
	//{
	//	if (cachemap.size() < cachenum)//如果缓存中还有还有剩余的位置
	//	{
	//		auto it = sharedStackpool.begin();
	//		for (; it != sharedStackpool.end(); ++it)
	//		{
	//			if (it->first == -1)
	//				break;
	//		}
	//		sharedStackpool.push_front(*it);
	//		sharedStackpool.erase(it);
	//		cachemap[id] = sharedStackpool.begin();
	//		sharedStackpool.begin()->first = id;
	//		return sharedStackpool.begin()->second;//将空间返回给使用者
	//	}
	//	else//如果缓存已经满了
	//	{
	//		auto it = (--sharedStackpool.end());//需要被清除的数据
	//		//将数据保存给对应的协程，然后给新协程使用
	//		if (coroutines[it->first].stack)
	//			free(coroutines[it->first].stack);
	//		coroutines[it->first].stack = (char*)malloc(coroutines[it->first].size);
	//		memcpy(coroutines[it->first].stack, it->second + STACKSIZE - coroutines[it->first].size, coroutines[it->first].size);
	//		//备份完成后将新的数据放进共享栈

	//		sharedStackpool.push_front(*it);
	//		sharedStackpool.pop_back();
	//		cachemap.erase(it->first);//删除原索引
	//		sharedStackpool.begin()->first = id;
	//		cachemap[id] = sharedStackpool.begin();//设置新的索引
	//		return sharedStackpool.begin()->second;
	//	}
	//}
 }

bool CoManager::isInCache(int id)
{
	if (shareStackpool[id%shareStackpool.size()].second==id)
		return true;
	else
		return false;
}

void CoManager::setCostacksize(int id)
{
	char dummy = 0;
	coroutines[id].size = shareStackpool[id%shareStackpool.size()].first + STACKSIZE - &dummy;
}

void CoManager::coyield()
{
	if (!co_record.empty())//在主协程中yield无效
	{
		int id = co_record.top();
		co_record.pop();
		const ucontext_t* nextctx = NULL;
		if (co_record.empty())
		{
			
			nextctx = &main_ctx;
			runid = -1;
		}
		else
		{
			runid = co_record.top();
			nextctx = &coroutines[co_record.top()].ctx;
			bool isincache = isInCache(id);
			char* stack = getsharedstack(id);
			coroutines[runid].ctx.uc_stack.ss_sp = stack;
			if (!isincache)
				memcpy(stack + STACKSIZE - coroutines[runid].size, coroutines[runid].stack, coroutines[runid].size);
		}
		//savestack(id);
		setCostacksize(id);//只设置好栈的长度，暂时不将数据移出共享区
		swapcontext(&coroutines[id].ctx, nextctx);
	}
}
