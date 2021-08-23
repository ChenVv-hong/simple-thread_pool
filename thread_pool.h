//
// Created by ChenVv on 2021/8/23.
//
#ifndef THREAD_POOL_THREAD_POOL_H
#define THREAD_POOL_THREAD_POOL_H

#include <iostream>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <list>
#include <future>
#include <thread>
#include <vector>
#include <atomic>

template<typename T>
class thread_safe_queue{
public:
	thread_safe_queue(int size_ = 100){this->max_size = size_;this->is_stop = false;}
	//禁止拷贝构造函数 和 赋值运算
	thread_safe_queue(const thread_safe_queue<T> & a) = delete;
	thread_safe_queue<T>& operator= (thread_safe_queue<T> & a) = delete;
	//放入元素
	void Push(const T & x){
		Add(x);
	}
	void Push(T&&x){
		Add(std::forward<T>(x));
	}
	//取出一串队列
	void Take(std::list<T>& list){
		std::unique_lock<std::mutex> lock(m);
		is_take.wait(lock,[this](){return !isEmpty() || is_stop;});//条件变量阻塞线程 判断队列是否为空 或是 队列停止
		if(is_stop) return; //队列停止 停止取出元素
		list = std::move(my_queue);
		is_add.notify_one(); //以取出队列 可以添加  唤醒正在等待的线程
	}
	//单个取出
	void Take(T & t){
		std::unique_lock<std::mutex> lock(m);
		is_take.wait(m,[this](){return !isEmpty() || is_stop;});
		if(is_stop) return;
		t = my_queue.front();
		my_queue.pop_front();
		is_add.notify_one(); //以取出队列 可以添加  唤醒正在等待的线程
	}
	//停止队列
	void Stop(){
		std::lock_guard<std::mutex> lock(m);
		is_stop = true;
		//唤醒所有阻塞线程 停止运行
		is_add.notify_all();
		is_take.notify_all();
	}
	//队列是否为空 T 空 F 非空
	bool Empty(){
		std::lock_guard<std::mutex> lock(m);
		return isEmpty();
	}
	//队列是否满 T 满 F 不满
	bool full(){
		std::lock_guard<std::mutex> lock(m);
		return isFull();
	}
	//队列最大值
	bool maxSize(){
		std::lock_guard<std::mutex> lock(m);
		return max_size;
	}
	//当前队列中的元素个数
	int Size(){
		std::lock_guard<std::mutex> lock(m);
		return my_queue.size();
	}
	//是否停止
	bool getStop(){
		std::lock_guard<std::mutex> lock(m);
		return is_stop;
	}
private:
	bool is_stop; // 是否停止  T 停止 F 运行
	int max_size; //队列最大容量
	std::list<T> my_queue; //链表模拟队列
	std::mutex m; //互斥元
	std::condition_variable is_add; //队列状态是否已满 是否可以添加
	std::condition_variable is_take; //队列状态是否是空 是否可以取出队列
	bool isFull() const{ //队列是否满  T 满 F 不满
		bool full = (my_queue.size() >= max_size);
		if(full) std::cout << "队列满，请等待......" << std::endl;
		return full;
	}
	bool isEmpty() const{//队列是否为空 T 空 F 非空
		bool empty = my_queue.empty();
		if(empty) std::cout << "队列空，请等待......." << std::endl;
		return empty;
	}

	template<typename F>
	void Add(F && x){ //向队列添加元素
		std::unique_lock<std::mutex> lock(m);
		is_add.wait(lock,[this](){return !isFull() || is_stop;}); //条件变量阻塞线程 检查队列是否为满 或是 队列停止运行
		if(is_stop) return; //队列停止 停止添加元素
		my_queue.push_back(x);
		is_take.notify_one(); //可以取出队列 唤醒正在等待的线程
	}
};

template<typename T>
class ThreadPool{
public:
	ThreadPool(int size = 100, int num_threads = (std::thread::hardware_concurrency() > 2 ? std::thread::hardware_concurrency() : 2)):m_q(size){
		start(num_threads);
	}
	ThreadPool(const ThreadPool<T>& a) = delete;
	ThreadPool<T>& operator=(ThreadPool<T> &a) = delete;
	//防止没有手动关闭线程池 在析构线程池时自动运行关闭线程池
	~ThreadPool(){
		shutdown();
	}

	//关闭线程池  使用call_once确保只调用一次关闭线程池
	void shutdown(){
		std::call_once(m_flag,std::bind(&ThreadPool<T>::stop,this));
	}

	//提交任务
	void submit(T& task){
		m_q.Push(task);
	}

private:
	std::vector<std::thread> m_pool; //线程池
	thread_safe_queue<T> m_q; //任务队列
	std::once_flag m_flag;
	std::atomic_bool m_running; //线程池
	//线程池开始运行
	void start(int num_threads){
		m_running = true;
		for(int i = 0; i < num_threads; i++){
			m_pool.push_back(std::thread(&ThreadPool<T>::RunInThread,this));
		}
	}
	//运行任务函数
	void RunInThread(){
		while(m_running){
			std::list<T> list;
			m_q.Take(list);
			for(auto task : list){
				if(!m_running) return;
				task();
			}
		}
	}
	//停止运行
	void stop(){
		m_q.Stop(); //停止队列
		m_running = false;
		for(int i = 0; i < m_pool.size(); i++){ //将线程结合
			if(m_pool[i].joinable()) m_pool[i].join();
		}
		m_pool.clear();
	}
};
#endif //THREAD_POOL_THREAD_POOL_H
