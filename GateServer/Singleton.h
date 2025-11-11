#pragma once
#include <memory>
#include <mutex>
#include <iostream>

template<typename T>
class Singleton{

protected:
	Singleton() = default;
	Singleton(const Singleton<T>&) = delete;
	Singleton& operator =(const Singleton<T>& single) = delete;

	static std::shared_ptr<T> _instance;

public:

	/*
		使用 std::once_flag 和 std::call_once 确保单例对象的创建是线程安全的
	第一次调用：
        当 std::call_once 第一次被调用时，func 会被执行。
        std::once_flag 的状态被标记为“已执行”。
    后续调用：
        当 std::call_once 再次被调用时，func 不会被执行。
        调用 std::call_once 的线程会立即返回，不会阻塞。
    异常处理：
        如果 func 抛出异常，std::once_flag 的状态不会被标记为“已执行”。
        其他线程可以再次尝试执行 func。
	*/
	static std::shared_ptr<T> GetInstance() {
		static std::once_flag s_flag;
		std::call_once(s_flag, [&]() {
			_instance = std::shared_ptr<T>(new T);
			});

		return _instance;
	}

	void PrintAddress() {
		std::cout << _instance.get() << std::endl;
	}

	~Singleton() {
		std::cout << "this is singleton destruct" << std::endl;
	}
};

template <typename T>
std::shared_ptr<T> Singleton<T>::_instance = nullptr;
