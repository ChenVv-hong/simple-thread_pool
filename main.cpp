#include <iostream>
#include "thread_pool.h"
using namespace std;

void f1(){
	cout << "111111" << endl;
}
void f2(){
	cout << "22222" << endl;
}
void f3(){
	cout << "33333" << endl;
}

int main() {
	ThreadPool<std::function<void()>> pool(10,4); //任务队列长度 和 线程数
//	ThreadPool<std::function<void()>> pool();
	thread t1([&](){
		for(int i = 0; i < 1000; i++) {
			std::function<void()> f = std::bind(f1);
			pool.submit(f);
		}
	});
	thread t2([&](){
		for(int i = 0; i < 1000; i++) {
			std::function<void()> f = std::bind(f2);
			pool.submit(f);
		}
	});
	t1.join();
	t2.join();
	this_thread::sleep_for(std::chrono::seconds(5));
	pool.shutdown();
	return 0;
}
