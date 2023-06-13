#include "pch.h"
#include <cassert>
#include <chrono>
#include <mutex>
#include <stack>
#include <stdexcept>
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
	AtomicityChecker()
	{
		if (m_threadId) {
			throw std::runtime_error("The other thread intruded during invocation");
		}
		m_threadId = ::GetCurrentThreadId();
	}

	~AtomicityChecker()
	{
		m_threadId = {};
	}

private:
	static inline DWORD m_threadId;
};

#define ATOMICITY_CHECKER auto _ = AtomicityChecker()


void goo()
{
	using namespace std::chrono_literals;

	try {
		ATOMICITY_CHECKER;

		std::this_thread::sleep_for(1s);
	}
	catch (std::runtime_error& e) {
		throw e;
	}
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


void test()
{	
	auto t0 = std::thread(foo);
	auto t1 = std::thread(bar);

	try {
		t0.join();
		t1.join();
	}
	catch (std::runtime_error& e) {
		throw e;
	}
	catch (...) {
		assert(false);

		throw std::logic_error("");
	}
};


namespace UnitTest
{
	TEST_CLASS(UnitTest1)
	{
		TEST_METHOD(TestMethod1)
		{
			

			Assert::ExpectException<std::runtime_error>(test);
		}
	};
}
