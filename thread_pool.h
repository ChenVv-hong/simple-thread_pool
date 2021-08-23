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
	//��ֹ�������캯�� �� ��ֵ����
	thread_safe_queue(const thread_safe_queue<T> & a) = delete;
	thread_safe_queue<T>& operator= (thread_safe_queue<T> & a) = delete;
	//����Ԫ��
	void Push(const T & x){
		Add(x);
	}
	void Push(T&&x){
		Add(std::forward<T>(x));
	}
	//ȡ��һ������
	void Take(std::list<T>& list){
		std::unique_lock<std::mutex> lock(m);
		is_take.wait(lock,[this](){return !isEmpty() || is_stop;});//�������������߳� �ж϶����Ƿ�Ϊ�� ���� ����ֹͣ
		if(is_stop) return; //����ֹͣ ֹͣȡ��Ԫ��
		list = std::move(my_queue);
		is_add.notify_one(); //��ȡ������ �������  �������ڵȴ����߳�
	}
	//����ȡ��
	void Take(T & t){
		std::unique_lock<std::mutex> lock(m);
		is_take.wait(m,[this](){return !isEmpty() || is_stop;});
		if(is_stop) return;
		t = my_queue.front();
		my_queue.pop_front();
		is_add.notify_one(); //��ȡ������ �������  �������ڵȴ����߳�
	}
	//ֹͣ����
	void Stop(){
		std::lock_guard<std::mutex> lock(m);
		is_stop = true;
		//�������������߳� ֹͣ����
		is_add.notify_all();
		is_take.notify_all();
	}
	//�����Ƿ�Ϊ�� T �� F �ǿ�
	bool Empty(){
		std::lock_guard<std::mutex> lock(m);
		return isEmpty();
	}
	//�����Ƿ��� T �� F ����
	bool full(){
		std::lock_guard<std::mutex> lock(m);
		return isFull();
	}
	//�������ֵ
	bool maxSize(){
		std::lock_guard<std::mutex> lock(m);
		return max_size;
	}
	//��ǰ�����е�Ԫ�ظ���
	int Size(){
		std::lock_guard<std::mutex> lock(m);
		return my_queue.size();
	}
	//�Ƿ�ֹͣ
	bool getStop(){
		std::lock_guard<std::mutex> lock(m);
		return is_stop;
	}
private:
	bool is_stop; // �Ƿ�ֹͣ  T ֹͣ F ����
	int max_size; //�����������
	std::list<T> my_queue; //����ģ�����
	std::mutex m; //����Ԫ
	std::condition_variable is_add; //����״̬�Ƿ����� �Ƿ�������
	std::condition_variable is_take; //����״̬�Ƿ��ǿ� �Ƿ����ȡ������
	bool isFull() const{ //�����Ƿ���  T �� F ����
		bool full = (my_queue.size() >= max_size);
		if(full) std::cout << "����������ȴ�......" << std::endl;
		return full;
	}
	bool isEmpty() const{//�����Ƿ�Ϊ�� T �� F �ǿ�
		bool empty = my_queue.empty();
		if(empty) std::cout << "���пգ���ȴ�......." << std::endl;
		return empty;
	}

	template<typename F>
	void Add(F && x){ //��������Ԫ��
		std::unique_lock<std::mutex> lock(m);
		is_add.wait(lock,[this](){return !isFull() || is_stop;}); //�������������߳� �������Ƿ�Ϊ�� ���� ����ֹͣ����
		if(is_stop) return; //����ֹͣ ֹͣ���Ԫ��
		my_queue.push_back(x);
		is_take.notify_one(); //����ȡ������ �������ڵȴ����߳�
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
	//��ֹû���ֶ��ر��̳߳� �������̳߳�ʱ�Զ����йر��̳߳�
	~ThreadPool(){
		shutdown();
	}

	//�ر��̳߳�  ʹ��call_onceȷ��ֻ����һ�ιر��̳߳�
	void shutdown(){
		std::call_once(m_flag,std::bind(&ThreadPool<T>::stop,this));
	}

	//�ύ����
	void submit(T& task){
		m_q.Push(task);
	}

private:
	std::vector<std::thread> m_pool; //�̳߳�
	thread_safe_queue<T> m_q; //�������
	std::once_flag m_flag;
	std::atomic_bool m_running; //�̳߳�
	//�̳߳ؿ�ʼ����
	void start(int num_threads){
		m_running = true;
		for(int i = 0; i < num_threads; i++){
			m_pool.push_back(std::thread(&ThreadPool<T>::RunInThread,this));
		}
	}
	//����������
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
	//ֹͣ����
	void stop(){
		m_q.Stop(); //ֹͣ����
		m_running = false;
		for(int i = 0; i < m_pool.size(); i++){ //���߳̽��
			if(m_pool[i].joinable()) m_pool[i].join();
		}
		m_pool.clear();
	}
};
#endif //THREAD_POOL_THREAD_POOL_H
