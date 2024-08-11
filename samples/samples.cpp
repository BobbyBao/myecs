// myecs.cpp: 定义应用程序的入口点。
//

#include "samples.h"

#include "myecs.h"

using namespace std;
using namespace myecs;

Database db;

struct Vec2 {
    float x;
    float y;
};

class MoveObject : public TComponentManager<Vec2, Vec2> {
};

int main()
{
    auto& moveManager = *db.getPtr<MoveObject>();

    for (int i = 0; i < 100000; i++) {
        auto e = db.create();
        auto inst = moveManager.addComponent(e);
        moveManager.elementAt<0>(inst) = { (float)i, (float)i };
        moveManager.elementAt<1>(inst) = { 2.0f, 2.0f };
    }

    for (auto i = moveManager.begin(); i != moveManager.end(); i++) {
        auto& p = moveManager.elementAt<0>(i);
        auto& v = moveManager.elementAt<1>(i);
        p.x += v.x;
        p.y += v.y;
        //std::cout << "x:" << p.x << ", y:" << p.y << std::endl;
    }
    return 0;
}
