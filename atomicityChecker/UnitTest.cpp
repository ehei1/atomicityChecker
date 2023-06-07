#include "pch.h"
#include <cassert>
#include <stack>
#include <tuple>
#include <unordered_map>
#include <vector>
#include <Windows.h>
#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;


class AtomicityChecker
{
public:
	AtomicityChecker( void* key, const char* functionName ) :
		m_key{ reinterpret_cast<long long>(key) }
	{
		auto it = std::lower_bound(std::begin(m_validator), std::end(m_validator), m_key, [](auto& p, auto instanceAddress) { return p.first < instanceAddress; });
		if (it == std::cend(m_validator) || it->first != m_key) {
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
};


namespace UnitTest
{
	TEST_CLASS(UnitTest1)
	{
		TEST_METHOD(TestMethod1)
		{
			auto _ = AtomicityChecker(this, __FUNCTION__);

			foo();
		}

		void foo()
		{
			auto _ = AtomicityChecker(this, __FUNCTION__);

			bar();
		}

		void bar()
		{
			auto _ = AtomicityChecker(this, __FUNCTION__);
		}
	};
}
