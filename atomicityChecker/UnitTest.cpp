#include "pch.h"
#include <cassert>
#include <mutex>
#include <stack>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <vector>
#include <Windows.h>
#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;


class AtomicityChecker
{
public:
	AtomicityChecker(void* key, const char* functionName) :
		AtomicityChecker{ reinterpret_cast<long long>(key), functionName }
	{}

	AtomicityChecker(long long key, const char* functionName) :
		m_key{ key }
	{
		std::lock_guard guard{ m_mutex };

		auto it = std::lower_bound(std::begin(m_validator), std::end(m_validator), m_key, [](auto& p, auto instanceAddress) { return p.first < instanceAddress; });
		if (it == std::end(m_validator) || it->first != m_key) {
			it = m_validator.insert(it, std::pair(m_key, Threads{}));

			auto& v = it->second.emplace_back();
			v.first = m_threadId;
			v.second.push(functionName);
		}
		else {
			auto& threads = it->second;
			auto threadIt = std::lower_bound(std::begin(threads), std::end(threads), m_threadId, [](auto& p, auto threadId) { return p.first < threadId; });
			if (threadIt != std::end(threads) && threadIt->first == m_threadId) {
				if (threads.size() > 1) {
					assert(false);
				}
				else {
					threadIt->second.push(functionName);
				}
			}
			else {
				auto it = threads.insert(threadIt, Thread{});
				it->first = m_threadId;
				it->second.push(functionName);
			}
		}
	}

	~AtomicityChecker()
	{
		std::lock_guard guard{ m_mutex };

		auto it = std::lower_bound(std::begin(m_validator), std::end(m_validator), m_key, [](auto& p, auto key) { return p.first < key; });
		if (it == std::end(m_validator) || it->first != m_key) {
			assert(!"implementation error");
		}
		else {
			auto& threads = it->second;
			auto threadIt = std::lower_bound(std::begin(threads), std::end(threads), m_threadId, [](auto& p, auto id) { return p.first < id; });
			if (threadIt == std::end(threads) || threadIt->first != m_threadId) {
				assert(!"implementation error");
			}
			else {
				assert(!threadIt->second.empty());
				threadIt->second.pop();
			}

			if (threadIt->second.empty()) {
				threads.erase(threadIt);
				if (threads.empty()) {
					m_validator.erase(it);
				}
			}
		}
	}

private:
	const DWORD m_threadId = ::GetCurrentThreadId();

	static_assert(sizeof(long long) == sizeof(void*));
	using InstanceAddress = long long;
	const InstanceAddress m_key;

	using ThreadId = DWORD;
	using FuctionName = const char*;

	using Thread = std::pair<ThreadId, std::stack<FuctionName>>;
	using Threads = std::vector<Thread>;

	static inline std::vector<
		std::pair<InstanceAddress, Threads>
	> m_validator;

	static inline std::mutex m_mutex;
};

#define ATOMICITY_CHECKER_FUNC auto _ = AtomicityChecker(__FUNCTION__, __FUNCTION__)


void goo()
{
	ATOMICITY_CHECKER_FUNC;
}


void foo()
{
	for (auto i = 0; i < 100; ++i) {
		goo();
	}
}


void bar()
{
	for (auto i = 0; i < 100; ++i) {
		goo();
	}
}


namespace UnitTest
{
	TEST_CLASS(UnitTest1)
	{
		TEST_METHOD(TestMethod1)
		{
			auto t0 = std::thread(foo);
			auto t1 = std::thread(bar);

			t0.join();
			t1.join();
		}
	};
}
