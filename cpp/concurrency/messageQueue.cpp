#include <iostream>
#include <mutex>
#include <thread>
#include <future>
#include <deque>


template<class T>
class MessageQueue
{
  public:
    T receive() 
    {
      std::unique_lock<std::mutex> uLock(_mtx);
      
      // Wait till you get alerted that a new message was added
      _cond.wait(uLock, [this]{ return !_messages.empty(); });

      // move queue first message's ownership to another var and pop
      T message = std::move(_messages.front());
      _messages.pop_front();

      return message;
    }

    template<class C>
    void send(C &&message)
    {
      // simulate work
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));

      std::lock_guard<std::mutex> llock(_mtx);

      // add message to queue
      std::cout << "  Message - '" << message << "' added to the queue" << std::endl;
      _messages.push_back(std::move(message));
      _cond.notify_one(); // notify cond var of new message added
    }

  private:
    std::mutex _mtx;
    std::condition_variable _cond;
    std::deque<T> _messages;
};

int main() {
  std::shared_ptr<MessageQueue<int>> queue(new MessageQueue<int>);

  std::cout << "Spawning threads..." << std::endl;
  std::vector<std::future<void>> futures;

  for (int i = 0; i < 50; ++i)
  {
    int message = i;
    futures.emplace_back(std::async(std::launch::async, &MessageQueue<int>::send<int>, queue, std::move(message)));
  }

  std::cout << "Collecting results..." << std::endl;
  while (true)
  {
    int message = queue->receive();
    std::cout << "  Message - '" << message << "' removed from queue" << std::endl;
  }

  std::for_each(futures.begin(), futures.end(), [](std::future<void> &ftr) {
    ftr.wait();
  });

  std::cout << "Finished!" << std::endl;

  return 0;
}