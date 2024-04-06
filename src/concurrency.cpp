#include <stdio.h>
#include <atomic>
#include <condition_variable>
#include <future>
#include <mutex>
#include <shared_mutex>
#include <vector>
#include <ctime>

class RWLockW
{
private:
    int _readers=0;
    int _writers=0;
    bool _is_writing=false;
    std::mutex _lock;
    std::condition_variable _rc;
    std::condition_variable _wc;
public:
    void RLock() 
    {
        std::unique_lock<std::mutex> g(_lock);
        _rc.wait(g, [&]() {return _writers==0; });
        ++_readers;
    }
    void URLock()
    {
        std::unique_lock<std::mutex> g(_lock);
        --_readers;
        if (_readers == 0)
            _wc.notify_one();
    }
    void WLock() {
        std::unique_lock<std::mutex> g(_lock);
        //条件变量debug：
        //1.调用wait时同步思考notify由谁调用，在哪调用，以及各种条件下的调用
        //eg：对于写唤醒，读和写的释放都会notify
        //2.注意条件修改在wait之前还是之后，如果在之后会不会导致notify的条件判断出问题
        ++_writers;
        _wc.wait(g, [&]() {return _readers == 0 && _is_writing == false; });
        _is_writing = true;
        
    }
    void UWLock() {
        std::unique_lock<std::mutex> g(_lock);
        _is_writing = false;
        --_writers;
        if (_writers)
            _wc.notify_one();
        else
            _rc.notify_all();
    }
};

template<class Tp>
class ConcurrentQueueLockFree
{
private:
	//环形数组，尾和头相距1为满
	int _size;
	std::atomic<int> _head;
    long long pad1[8];
	std::atomic<int> _tail;
    long long pad2[8];
	Tp* _data;
public:
	ConcurrentQueueLockFree(int size):_size(size+1),_head(0),_tail(0) {
		_data = new Tp[_size];
	}
	~ConcurrentQueueLockFree() {
		delete[] _data;
	}

	bool TryPush(const Tp& item)
	{
		int new_tail = 0;
		int tail = 0;
		do 
		{
			tail = _tail.load(std::memory_order_relaxed);
			int head = _head.load(std::memory_order_relaxed);
			new_tail = (tail + 1) % _size;

			if (new_tail == head)
				return false;
		} while (!_tail.compare_exchange_strong(tail, new_tail, std::memory_order_relaxed));
		_data[tail] = item;
		return true;
	}

	bool TryPop(Tp& item)
	{
		int new_head = 0;
		int head = 0;
		do
		{
			head = _head.load(std::memory_order_relaxed);
			int tail = _tail.load(std::memory_order_relaxed);
			if (head == tail)
				return false;
			new_head = (head + 1) % _size;
		} while (!_head.compare_exchange_strong(head, new_head, std::memory_order_relaxed));
		item = _data[head];
		return true;
	}

	int Size()
	{
		int head = _head.load(std::memory_order_relaxed);
		int tail = _tail.load(std::memory_order_relaxed);
		return (tail + _size - head) % _size;
	}
};

template<class Tp>
class ConcurrentQueueBlocked
{
private:
	int _max_size;
	int _cur_size;
	int _head;
	int _tail;
	Tp* _data;
	std::mutex _lock;
	std::condition_variable _provider;
	std::condition_variable _consumer;

public:
	ConcurrentQueueBlocked(int size) :_max_size(size), _cur_size(0), _head(0), _tail(0) {
		_data = new Tp[_max_size];
	}
	~ConcurrentQueueBlocked() {
		delete[] _data;
	}

	void Push(const Tp& item)
	{
		std::unique_lock<std::mutex> guard(_lock);
		_provider.wait(guard, [&]() {return _cur_size < _max_size; });
		_data[_tail] = item;
		_tail = (_tail + 1) % _max_size;
		++_cur_size;
		_consumer.notify_all();
	}

	Tp Pop()
	{
		std::unique_lock<std::mutex> guard(_lock);
		_consumer.wait(guard, [&]() {return _cur_size > 0; });
		Tp ret = _data[_head];
		_head = (_head + 1) % _max_size;
		--_cur_size;
		_provider.notify_all();
		return ret;
	}

	int Size() { return _cur_size; }
};

ConcurrentQueueLockFree<int> que(10);

void TestLockFree()
{
	auto provider = []() {
		for (int i = 0; i < 80; ++i)
		{
			int a = 2;
			while (!que.TryPush(a));
		}
	};
	auto consumer = []() ->int {
		int res = 0;
		for (int i = 0; i < 40; ++i)
		{
			int a;
			while (!que.TryPop(a));
			res += a;
		}
		return res;
	};
	clock_t start, end;
	start = clock();
	std::vector<std::future<int>> fv;
	for (int i = 0; i < 2; ++i)
		std::thread(provider).detach();
	for (int i = 0; i < 4; ++i)
		fv.push_back(std::async(consumer));
	int sum = 0;
	for (auto& f : fv)
		sum += f.get();
	end = clock();
	printf("expect %d,actual %d ", 320, sum);
	printf("que size %d\n", que.Size());
	printf("use time %ld\n", end - start);
}

ConcurrentQueueBlocked<int> que_b(10);

void TestBlock()
{
	auto provider = []() {
		for (int i = 0; i < 80; ++i)
			que_b.Push(2);
	};
	auto consumer = []()
	{
		int res = 0;
		for (int i = 0; i < 40; ++i)
		{
			res += que_b.Pop();
		}
		return res;
	};
	clock_t start, end;
	start = clock();
	std::vector<std::future<int>> fv;
	for (int i = 0; i < 2; ++i)
		std::thread(provider).detach();
	for (int i = 0; i < 4; ++i)
		fv.push_back(std::async(consumer));
	int sum = 0;
	for (auto& f : fv)
		sum += f.get();
	end = clock();
	printf("expect %d,actual %d ", 320, sum);
	printf("que size %d\n", que_b.Size());
	printf("use time %ld\n", end - start);
}

void TestQue()
{
    printf("---lock free---\n");
	for(int i=0;i<10;++i)
		TestLockFree();
	
	printf("---block---\n");
	for (int i = 0; i < 10; ++i)
		TestBlock();
}

int main()
{
	
		
	return 0;
}