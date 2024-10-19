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
    std::vector<std::thread>threads;//�̳߳������й������߳�
    std::queue<std::function<void()>>tasks;//�����������
    std::mutex mtx;//������
    std::condition_variable condition;//����������ʵ���߳�ͬ��
    bool stop;//��ʾ�̳߳��Ƿ�ֹͣ��

public:
    ThreadPool(int numThreads) :stop(false) {//����ָ�������Ĺ����̣߳���������ӵ��̳߳���
        for (int i = 0; i < numThreads; i++) {
            threads.emplace_back([this]() {

                while (1) { //����ѭ����ÿ��ѭ�������������л�ȡ����ִ��


                    //�Թ�����Դ��������У����б�����ȷ��ͬһʱ�䣬ֻ��һ���߳̿��޸��������
                    std::unique_lock<std::mutex> lock(mtx);


                    //������в�Ϊ��,���̳߳�ֹͣ  �����̱߳����Ѳ�ִ��
                    condition.wait(lock, [this] {return !tasks.empty() || stop; });
                    //�̳߳��Ѿ�ֹͣ���������Ϊ�գ����߳̽��˳�ѭ��
                    if (stop && tasks.empty()) { return; }


                    //�����������ȡ����һ�����񣬲��ڽ�����������ִ�и�����
                    //�ƶ����壬��Դת�ƣ��������б�ĵ�һ������ֵ��task
                    std::function<void()> task = std::move(tasks.front());
                    tasks.pop();

                    lock.unlock();//ִ������ǰ�����
                    task();//ִ��

                }
                });
        }
    }

    //������������������
    template<class F, class... Args>
    void enqueue(F&& f, Args... args) {//�������ã�F&& �Զ��Ƶ���ֵ\��ֵ����

        //std::bind��������Ķ���f�Ͳ���args�󶨣��γ�һ����������ת�����ƶ�����ʱ������ԭ�����ԣ�
        std::function<void()> task = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        
        {
            //ȷ������̲߳����������ʱ�����������ݾ�̬��������ִ����󣬽�����������
            std::unique_lock<std::mutex> lock(mtx);
            //��ӵ����������,�ƶ����壨����������Ⱥ�,��_back��
            tasks.emplace(std::move(task));
        }

        //����һ�������������ϵȴ����߳�
        condition.notify_one();
    }

    ~ThreadPool()
    {
        std::unique_lock<std::mutex> lock(mtx);
        stop = true;

        //�������еȴ����߳�,���»�ȡ������
        condition.notify_all();
        //���εȴ�ÿ�������߳�ִ����ϣ�ȷ�������߳��˳�
        for (auto& t : threads) {
            t.join();
        }
    }
};
int main() {
    ThreadPool pool(4);
    for (int i = 0; i < 10; i++) {
        //��һ��Lambda���ʽ��ӵ��̳߳��е����������
        pool.enqueue([i] (){
            
            std::cout << "Task:" << i << "��������" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(2));
            std::cout << "Task:" << i << "���н���" << std::endl;
            });
    }
    //�����߳�����10�룬ȷ��ִ������������
    std::this_thread::sleep_for(std::chrono::seconds(10));
    return 0;
}