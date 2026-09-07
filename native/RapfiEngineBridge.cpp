#include "RapfiEngineBridge.h"

#include "command/command.h"

#include <condition_variable>
#include <deque>
#include <iostream>
#include <memory>
#include <mutex>
#include <streambuf>
#include <string>
#include <thread>

namespace {

class CommandInputBuffer final : public std::streambuf {
public:
    void push(std::string command)
    {
        command.push_back('\n');
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push_back(std::move(command));
        }
        condition_.notify_one();
    }

    void close()
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            closed_ = true;
        }
        condition_.notify_all();
    }

protected:
    int_type underflow() override
    {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [&] { return closed_ || !queue_.empty(); });
        if (queue_.empty()) return traits_type::eof();

        current_ = std::move(queue_.front());
        queue_.pop_front();
        setg(current_.data(), current_.data(), current_.data() + current_.size());
        return traits_type::to_int_type(*gptr());
    }

private:
    std::mutex mutex_;
    std::condition_variable condition_;
    std::deque<std::string> queue_;
    std::string current_;
    bool closed_ = false;
};

class CallbackOutputBuffer final : public std::streambuf {
public:
    CallbackOutputBuffer(RapfiLineCallback callback, void *context)
        : callback_(callback), context_(context)
    {}

protected:
    int_type overflow(int_type character) override
    {
        if (traits_type::eq_int_type(character, traits_type::eof())) return traits_type::not_eof(character);
        buffer_.push_back(traits_type::to_char_type(character));
        flushLines();
        return character;
    }

    std::streamsize xsputn(const char *data, std::streamsize count) override
    {
        buffer_.append(data, static_cast<size_t>(count));
        flushLines();
        return count;
    }

    int sync() override
    {
        flushLines();
        return 0;
    }

private:
    void flushLines()
    {
        size_t newline;
        while ((newline = buffer_.find('\n')) != std::string::npos) {
            std::string line = buffer_.substr(0, newline);
            buffer_.erase(0, newline + 1);
            if (!line.empty() && callback_) callback_(line.c_str(), context_);
        }
    }

    RapfiLineCallback callback_;
    void *context_;
    std::string buffer_;
};

struct EngineState {
    std::mutex lifecycleMutex;
    std::mutex startupMutex;
    std::condition_variable startupCondition;
    std::unique_ptr<CommandInputBuffer> input;
    std::unique_ptr<CallbackOutputBuffer> output;
    std::thread thread;
    std::string configPath;
    RapfiLineCallback callback = nullptr;
    void *context = nullptr;
    bool running = false;
    bool startupDone = false;
    bool startupOk = false;
};

EngineState state;

void emit(RapfiLineCallback callback, void *context, const std::string &line)
{
    if (callback) callback(line.c_str(), context);
}

void runEngine()
{
    auto input = std::make_unique<CommandInputBuffer>();
    auto output = std::make_unique<CallbackOutputBuffer>(state.callback, state.context);

    std::streambuf *oldCin = std::cin.rdbuf(input.get());
    std::streambuf *oldCout = std::cout.rdbuf(output.get());
    std::streambuf *oldCerr = std::cerr.rdbuf(output.get());
    state.input = std::move(input);
    state.output = std::move(output);

    char arg0[] = "rapfi";
    char *argv[] = {arg0, nullptr};
    Command::CommandLine::init(1, argv);
    Command::configPath = state.configPath;
    Command::allowInternalConfig = false;

    bool loaded = Command::loadConfig();
    {
        std::lock_guard<std::mutex> lock(state.startupMutex);
        state.startupDone = true;
        state.startupOk = loaded;
    }
    state.startupCondition.notify_one();

    if (!loaded) {
        emit(state.callback, state.context, "MESSAGE Native Rapfi failed to load config");
    } else {
        emit(state.callback, state.context, "MESSAGE Native Rapfi ready");
        Command::gomocupLoop();
    }

    std::cout.flush();
    std::cerr.flush();
    std::cin.rdbuf(oldCin);
    std::cout.rdbuf(oldCout);
    std::cerr.rdbuf(oldCerr);

    std::lock_guard<std::mutex> lock(state.lifecycleMutex);
    state.running = false;
}

}  // namespace

extern "C" int rapfi_engine_start(const char *config_path, RapfiLineCallback callback, void *context)
{
    std::unique_lock<std::mutex> lock(state.lifecycleMutex);
    if (state.running) return 1;
    if (!config_path || !callback) return 0;

    state.configPath = config_path;
    state.callback = callback;
    state.context = context;
    state.running = true;
    {
        std::lock_guard<std::mutex> startupLock(state.startupMutex);
        state.startupDone = false;
        state.startupOk = false;
    }
    state.thread = std::thread(runEngine);
    lock.unlock();

    std::unique_lock<std::mutex> startupLock(state.startupMutex);
    state.startupCondition.wait(startupLock, [] { return state.startupDone; });
    bool startupOk = state.startupOk;
    startupLock.unlock();

    if (!startupOk) {
        if (state.thread.joinable()) state.thread.join();
        std::lock_guard<std::mutex> cleanupLock(state.lifecycleMutex);
        state.input.reset();
        state.output.reset();
        state.callback = nullptr;
        state.context = nullptr;
    }
    return startupOk ? 1 : 0;
}

extern "C" void rapfi_engine_send(const char *command)
{
    if (!command) return;
    std::lock_guard<std::mutex> lock(state.lifecycleMutex);
    if (state.input) state.input->push(command);
}

extern "C" void rapfi_engine_stop(void)
{
    {
        std::lock_guard<std::mutex> lock(state.lifecycleMutex);
        if (!state.running) return;
        if (state.input) state.input->push("END");
        if (state.input) state.input->close();
    }

    if (state.thread.joinable()) state.thread.join();

    std::lock_guard<std::mutex> lock(state.lifecycleMutex);
    state.input.reset();
    state.output.reset();
    state.callback = nullptr;
    state.context = nullptr;
}
