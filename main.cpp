#include"CoManager.h"
#include<iostream>
using namespace std;

void func3(CoManager* c)
{
	cout << "func3" << endl;
	c->coyield();
	cout << "func3" << endl;
}
void func5(CoManager* c)
{
	cout << "func5" << endl;
	c->coyield();
	cout << "func5" << endl;
}
void func1(CoManager* c)
{
	cout << "func1" << endl;
	c->coyield();
	cout << "func1" << endl;
	c->coyield();
	cout << "func1" << endl;
}
void func2(CoManager* c)
{
	cout << "func2" << endl;
	c->coyield();
	cout << "func2" << endl;
	c->coyield();
	cout << "func2" << endl;
	int coid = c->cocreate(std::bind(func3, c));
	c->coresume(coid);
	cout << "func2" << endl;
	int coid2 = c->cocreate(std::bind(func5, c));
	c->coresume(coid2);
	cout << "main2" << endl;
	c->coresume(coid);
	cout << "main2" << endl;
	c->coresume(coid2);
	cout << "0000" << endl;


}
void func4(CoManager* c)
{
	cout << "func4" << endl;
	c->coyield();
	cout << "func4" << endl;
	c->coyield();
	cout << "func4" << endl;
}

int main()
{
	CoManager c(1024, 2);
	int id1 = c.cocreate(std::bind(func1, &c));
	int id2 = c.cocreate(std::bind(func2, &c));
	int id3 = c.cocreate(std::bind(func4, &c));
	cout << "main" << endl;
	c.coresume(id1);
	cout << "main" << endl;
	c.coresume(id2);
	cout << "main" << endl;
	c.coresume(id3);
	cout << "main" << endl;
	c.coresume(id2);
	cout << "main" << endl;
	
	c.coresume(id1);
	cout << "main" << endl;
	c.coresume(id3);
	cout << "main" << endl;
	c.coresume(id1);
	cout << "main" << endl;
	c.coresume(id3);
	cout << "main" << endl;
	c.coresume(id2);
	cout << c.cofinished() << endl;
	return 0;

}
