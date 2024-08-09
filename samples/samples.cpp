// myecs.cpp: 定义应用程序的入口点。
//

#include "samples.h"

#include "myecs.h"

using namespace std;
using namespace myecs;

Database db;

class MoveObject : public TComponentManager<float, float, float> {

};


int main()
{

	auto e = db.create();
	db.addComSet<MoveObject>();



	return 0;
}
