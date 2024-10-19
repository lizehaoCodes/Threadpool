#include<iostream>
#include<thread>
#include<mutex>
#include<string>
#include<condition_variable>
#include<queue>
#include<vector>
#include<functional>

class ThreadPool {
private:
    std::vector<std::thread>threads;//线程池中所有工作的线程
    std::queue<std::function<void()>>tasks;//定义任务队列
    std::mutex mtx;//互斥量
    std::condition_variable condition;//条件变量，实现线程同步
    bool stop;//表示线程池是否停止。

public:
    ThreadPool(int numThreads) :stop(false) {//创建指定数量的工作线程，并将其添加到线程池中
        for (int i = 0; i < numThreads; i++) {
            threads.emplace_back([this]() {

                while (1) { //无限循环，每次循环会从任务队列中获取任务并执行


                    //对共享资源（任务队列）进行保护，确保同一时间，只有一个线程可修改任务队列
                    std::unique_lock<std::mutex> lock(mtx);


                    //任务队列不为空,或线程池停止  工作线程被唤醒并执行
                    condition.wait(lock, [this] {return !tasks.empty() || stop; });
                    //线程池已经停止且任务队列为空，则线程将退出循环
                    if (stop && tasks.empty()) { return; }


                    //从任务队列中取出第一个任务，并在解锁互斥量后执行该任务
                    //移动语义，资源转移，将任务列表的第一个任务赋值给task
                    std::function<void()> task = std::move(tasks.front());
                    tasks.pop();

                    lock.unlock();//执行任务前需解锁
                    task();//执行

                }
                });
        }
    }

    //向任务队列中添加任务
    template<class F, class... Args>
    void enqueue(F&& f, Args... args) {//万能引用：F&& 自动推导左值\右值引用

        //std::bind：将传入的对象f和参数args绑定，形成一个任务（完美转发：移动参数时，保持原本特性）
        std::function<void()> task = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        
        {
            //确保多个线程并发添加任务时，不触发数据竞态（作用域执行完后，解锁互斥量）
            std::unique_lock<std::mutex> lock(mtx);
            //添加到任务队列中,移动语义（队列添加无先后,无_back）
            tasks.emplace(std::move(task));
        }

        //唤醒一个在条件变量上等待的线程
        condition.notify_one();
    }

    ~ThreadPool()
    {
        std::unique_lock<std::mutex> lock(mtx);
        stop = true;

        //唤醒所有等待的线程,重新获取互斥锁
        condition.notify_all();
        //依次等待每个工作线程执行完毕，确保所有线程退出
        for (auto& t : threads) {
            t.join();
        }
    }
};
int main() {
    ThreadPool pool(4);
    for (int i = 0; i < 10; i++) {
        //将一个Lambda表达式添加到线程池中的任务队列中
        pool.enqueue([i] (){
            
            std::cout << "Task:" << i << "正在运行" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(2));
            std::cout << "Task:" << i << "运行结束" << std::endl;
            });
    }
    //让主线程休眠10秒，确保执行完所有任务
    std::this_thread::sleep_for(std::chrono::seconds(10));
    return 0;
}